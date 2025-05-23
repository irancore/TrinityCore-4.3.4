/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Object.h"
#include "BattlefieldMgr.h"
#include "Battleground.h"
#include "CellImpl.h"
#include "Chat.h"
#include "CinematicMgr.h"
#include "CombatLogPackets.h"
#include "Common.h"
#include "Creature.h"
#include "DBCStores.h"
#include "G3DPosition.hpp"
#include "GameTime.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Item.h"
#include "Log.h"
#include "MapManager.h"
#include "MiscPackets.h"
#include "MovementPacketBuilder.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "OutdoorPvPMgr.h"
#include "PhasingHandler.h"
#include "PathGenerator.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "SpellAuraEffects.h"
#include "SpellDefines.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "Totem.h"
#include "Transport.h"
#include "Unit.h"
#include "UpdateData.h"
#include "UpdateFieldFlags.h"
#include "UpdateMask.h"
#include "Util.h"
#include "Vehicle.h"
#include "VMapFactory.h"
#include "VMapManager2.h"
#include "WaypointMovementGenerator.h"
#include "World.h"
#include "WorldPacket.h"

constexpr float VisibilityDistances[AsUnderlyingType(VisibilityDistanceType::Max)] =
{
    DEFAULT_VISIBILITY_DISTANCE,
    VISIBILITY_DISTANCE_TINY,
    VISIBILITY_DISTANCE_SMALL,
    VISIBILITY_DISTANCE_LARGE,
    VISIBILITY_DISTANCE_GIGANTIC,
    MAX_VISIBILITY_DISTANCE
};

Object::Object()
{
    m_objectTypeId      = TYPEID_OBJECT;
    m_objectType        = TYPEMASK_OBJECT;
    m_updateFlag.Clear();

    m_uint32Values      = nullptr;
    m_valuesCount       = 0;
    _fieldNotifyFlags   = UF_FLAG_DYNAMIC;

    m_inWorld           = false;
    m_isNewObject       = false;
    m_isDestroyedObject = false;
    m_objectUpdated     = false;
}

WorldObject::~WorldObject()
{
    // this may happen because there are many !create/delete
    if (IsWorldObject() && m_currMap)
    {
        if (GetTypeId() == TYPEID_CORPSE)
        {
            TC_LOG_FATAL("misc", "WorldObject::~WorldObject Corpse Type: %d (%s) deleted but still in map!!",
                ToCorpse()->GetType(), GetGUID().ToString().c_str());
            ABORT();
        }
        ResetMap();
    }
}

void WorldObject::Update(uint32 diff)
{
    m_Events.Update(diff);

    _heartbeatTimer -= Milliseconds(diff);
    while (_heartbeatTimer <= 0ms)
    {
        _heartbeatTimer += HEARTBEAT_INTERVAL;
        Heartbeat();
    }
}

Object::~Object()
{
    if (IsInWorld())
    {
        TC_LOG_FATAL("misc", "Object::~Object %s deleted but still in world!!", GetGUID().ToString().c_str());
        if (isType(TYPEMASK_ITEM))
            TC_LOG_FATAL("misc", "Item slot %u", ((Item*)this)->GetSlot());
        ABORT();
    }

    if (m_objectUpdated)
    {
        TC_LOG_FATAL("misc", "Object::~Object %s deleted but still in update list!!", GetGUID().ToString().c_str());
        ABORT();
    }

    delete [] m_uint32Values;
    m_uint32Values = nullptr;
}

void Object::_InitValues()
{
    m_uint32Values = new uint32[m_valuesCount];
    memset(m_uint32Values, 0, m_valuesCount*sizeof(uint32));

    _changesMask.SetCount(m_valuesCount);

    m_objectUpdated = false;
}

void Object::_Create(ObjectGuid::LowType guidlow, uint32 entry, HighGuid guidhigh)
{
    if (!m_uint32Values) _InitValues();

    ObjectGuid guid(guidhigh, entry, guidlow);
    SetGuidValue(OBJECT_FIELD_GUID, guid);
    SetUInt16Value(OBJECT_FIELD_TYPE, 0, m_objectType);
    m_PackGUID.Set(guid);
}

std::string Object::_ConcatFields(uint16 startIndex, uint16 size) const
{
    std::ostringstream ss;
    for (uint16 index = 0; index < size; ++index)
        ss << GetUInt32Value(index + startIndex) << ' ';
    return ss.str();
}

void Object::AddToWorld()
{
    if (m_inWorld)
        return;

    ASSERT(m_uint32Values);

    m_inWorld = true;

    // synchronize values mirror with values array (changes will send in updatecreate opcode any way
    ASSERT(!m_objectUpdated);
    ClearUpdateMask(false);
}

void Object::RemoveFromWorld()
{
    if (!m_inWorld)
        return;

    m_inWorld = false;

    // if we remove from world then sending changes not required
    ClearUpdateMask(true);
}

void Object::BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target) const
{
    if (!target)
        return;

    uint8 updateType = m_isNewObject ? UPDATETYPE_CREATE_OBJECT2 : UPDATETYPE_CREATE_OBJECT;
    CreateObjectBits flags = m_updateFlag;

    /** lower flag1 **/
    if (target == this)                                      // building packet for yourself
        flags.ThisIsYou = true;

    if (WorldObject const* worldObject = dynamic_cast<WorldObject const*>(this))
    {
        if (!flags.MovementUpdate && !worldObject->m_movementInfo.transport.guid.IsEmpty())
            flags.MovementTransport = true;

        if (worldObject->GetAIAnimKitId() || worldObject->GetMovementAnimKitId() || worldObject->GetMeleeAnimKitId())
            flags.AnimKit = true;
    }

    if (Unit const* unit = ToUnit())
    {
        if (unit->GetVictim())
            flags.CombatVictim = true;
    }

    ByteBuffer buf(500);
    buf << uint8(updateType);
    buf << GetPackGUID();
    buf << uint8(m_objectTypeId);

    BuildMovementUpdate(&buf, flags);
    BuildValuesUpdate(updateType, &buf, target);
    data->AddUpdateBlock(buf);
}

void Object::SendUpdateToPlayer(Player* player)
{
    // send create update to player
    UpdateData upd(player->GetMapId());
    WorldPacket packet;

    if (player->HaveAtClient(this))
        BuildValuesUpdateBlockForPlayer(&upd, player);
    else
        BuildCreateUpdateBlockForPlayer(&upd, player);
    upd.BuildPacket(&packet);
    player->SendDirectMessage(&packet);
}

void Object::SendUpdateToSet()
{
    if (Unit* unit = ToUnit())
    {
        std::list<Player*> players;
        unit->GetPlayerListInGrid(players, unit->GetVisibilityRange());
        for (auto itr = players.begin(); itr != players.end(); itr++)
        {
            UpdateData upd((*itr)->GetMapId());
            WorldPacket packet;
            if ((*itr)->HaveAtClient(this))
                BuildValuesUpdateBlockForPlayer(&upd, (*itr));
            upd.BuildPacket(&packet);
            (*itr)->GetSession()->SendPacket(&packet);
        }
    }
}

void Object::BuildValuesUpdateBlockForPlayer(UpdateData* data, Player* target) const
{
    ByteBuffer buf(500);

    buf << uint8(UPDATETYPE_VALUES);
    buf << GetPackGUID();

    BuildValuesUpdate(UPDATETYPE_VALUES, &buf, target);

    data->AddUpdateBlock(buf);
}

void Object::BuildOutOfRangeUpdateBlock(UpdateData* data) const
{
    data->AddOutOfRangeGUID(GetGUID());
}

void Object::DestroyForPlayer(Player* target, bool isDead /*= false*/) const
{
    ASSERT(target);

    WorldPackets::Misc::DestroyObject packet;
    packet.Guid = GetGUID();
    //! If the following bool is true, the client will call "void CGUnit_C::OnDeath()" for this object.
    //! OnDeath() does for eg trigger death animation and interrupts certain spells/missiles/auras/sounds...
    packet.IsDead = isDead;
    target->SendDirectMessage(packet.Write());
}

void Object::SendOutOfRangeForPlayer(Player* target) const
{
    ASSERT(target);

    UpdateData updateData(target->GetMapId());
    BuildOutOfRangeUpdateBlock(&updateData);
    WorldPacket packet;
    updateData.BuildPacket(&packet);
    target->SendDirectMessage(&packet);
}

int32 Object::GetInt32Value(uint16 index) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    return m_int32Values[index];
}

uint32 Object::GetUInt32Value(uint16 index) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    return m_uint32Values[index];
}

uint64 Object::GetUInt64Value(uint16 index) const
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, false));
    return *((uint64*)&(m_uint32Values[index]));
}

float Object::GetFloatValue(uint16 index) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    return m_floatValues[index];
}

uint8 Object::GetByteValue(uint16 index, uint8 offset) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    ASSERT(offset < 4);
    return *(((uint8*)&m_uint32Values[index])+offset);
}

uint16 Object::GetUInt16Value(uint16 index, uint8 offset) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    ASSERT(offset < 2);
    return *(((uint16*)&m_uint32Values[index])+offset);
}

ObjectGuid Object::GetGuidValue(uint16 index) const
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, false));
    return *((ObjectGuid*)&(m_uint32Values[index]));
}

void Object::BuildMovementUpdate(ByteBuffer* data, CreateObjectBits flags) const
{
    Unit const* self = nullptr;
    ObjectGuid guid = GetGUID();
    uint32 movementFlags = 0;
    uint16 movementFlagsExtra = 0;

    bool hasTransportTime2 = false;
    bool hasVehicleId = false;
    bool hasFallDirection = false;
    bool hasFallData = false;
    bool hasPitch = false;
    bool hasSpline = false;
    bool hasSplineElevation = false;
    bool hasAIAnimKit = false;
    bool hasMovementAnimKit = false;
    bool hasMeleeAnimKit = false;

    std::vector<uint32> const* PauseTimes = nullptr;
    if (GameObject const* go = ToGameObject())
        PauseTimes = go->GetPauseTimes();

    // Bit content
    data->WriteBit(flags.PlayerHoverAnim);
    data->WriteBit(flags.SupressedGreetings);
    data->WriteBit(flags.Rotation);
    data->WriteBit(flags.AnimKit);
    data->WriteBit(flags.CombatVictim);
    data->WriteBit(flags.ThisIsYou);
    data->WriteBit(flags.Vehicle);
    data->WriteBit(flags.MovementUpdate);
    data->WriteBits(PauseTimes ? PauseTimes->size() : 0, 24);
    data->WriteBit(flags.NoBirthAnim);
    data->WriteBit(flags.MovementTransport);
    data->WriteBit(flags.Stationary);
    data->WriteBit(flags.AreaTrigger);
    data->WriteBit(flags.EnablePortals);
    data->WriteBit(flags.ServerTime);

    if (flags.MovementUpdate)
    {
        self = ToUnit();
        movementFlags = self->m_movementInfo.GetMovementFlags();
        movementFlagsExtra = self->m_movementInfo.GetExtraMovementFlags();
        hasSpline = self->IsSplineEnabled();

        hasTransportTime2 = self->m_movementInfo.transport.guid != 0 && self->m_movementInfo.transport.time2 != 0;
        hasVehicleId = false;
        hasPitch = self->HasUnitMovementFlag(MovementFlags(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || self->HasExtraUnitMovementFlag(MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING);
        hasFallDirection = self->HasUnitMovementFlag(MOVEMENTFLAG_FALLING);
        hasFallData = hasFallDirection || self->m_movementInfo.jump.fallTime != 0;
        hasSplineElevation = self->HasUnitMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION);

        if (GetTypeId() == TYPEID_UNIT)
            movementFlags &= MOVEMENTFLAG_MASK_CREATURE_ALLOWED;

        data->WriteBit(!movementFlags);                                         // !Has MoveFlags0
        data->WriteBit(G3D::fuzzyEq(self->GetOrientation(), 0.0f));             // Has Orientation
        data->WriteBit(guid[7]);
        data->WriteBit(guid[3]);
        data->WriteBit(guid[2]);
        if (movementFlags)
            data->WriteBits(movementFlags, 30);

        data->WriteBit(hasSpline && !self->IsPlayer());                         // !Has player spline data
        data->WriteBit(!hasPitch);                                              // !Has pitch
        data->WriteBit(hasSpline);                                              // Has spline data (independent)
        data->WriteBit(hasFallData);                                            // Has fall data
        data->WriteBit(!hasSplineElevation);                                    // !Has spline elevation
        data->WriteBit(guid[5]);
        data->WriteBit(!self->m_movementInfo.transport.guid.IsEmpty());         // Has transport data
        data->WriteBit(0);                                                      // !HasTime

        if (!self->m_movementInfo.transport.guid.IsEmpty())
        {
            ObjectGuid transGuid = self->m_movementInfo.transport.guid;

            data->WriteBit(transGuid[1]);
            data->WriteBit(hasTransportTime2);                             // Has PrevMoveTime
            data->WriteBit(transGuid[4]);
            data->WriteBit(transGuid[0]);
            data->WriteBit(transGuid[6]);
            data->WriteBit(hasVehicleId);                                  // Has VehicleRecID
            data->WriteBit(transGuid[7]);
            data->WriteBit(transGuid[5]);
            data->WriteBit(transGuid[3]);
            data->WriteBit(transGuid[2]);
        }

        data->WriteBit(guid[4]);

        if (hasSpline)
            Movement::PacketBuilder::WriteCreateBits(*self->movespline, *data);

        data->WriteBit(guid[6]);
        if (hasFallData)
            data->WriteBit(hasFallDirection);

        data->WriteBit(guid[0]);
        data->WriteBit(guid[1]);
        data->WriteBit(0);                                                      // HeightChangeFailed
        data->WriteBit(!movementFlagsExtra);                                    // !Has MoveFlags1
        if (movementFlagsExtra)
            data->WriteBits(movementFlagsExtra, 12);
    }

    if (flags.MovementTransport)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        ObjectGuid transGuid = self->m_movementInfo.transport.guid;
        data->WriteBit(transGuid[5]);
        data->WriteBit(hasVehicleId);                                           // Has GO transport time 3
        data->WriteBit(transGuid[0]);
        data->WriteBit(transGuid[3]);
        data->WriteBit(transGuid[6]);
        data->WriteBit(transGuid[1]);
        data->WriteBit(transGuid[4]);
        data->WriteBit(transGuid[2]);
        data->WriteBit(hasTransportTime2);                                      // Has GO transport time 2
        data->WriteBit(transGuid[7]);
    }

    if (flags.CombatVictim)
    {
        ObjectGuid victimGuid = self->GetVictim()->GetGUID();   // checked in BuildCreateUpdateBlockForPlayer
        data->WriteBit(victimGuid[2]);
        data->WriteBit(victimGuid[7]);
        data->WriteBit(victimGuid[0]);
        data->WriteBit(victimGuid[4]);
        data->WriteBit(victimGuid[5]);
        data->WriteBit(victimGuid[6]);
        data->WriteBit(victimGuid[1]);
        data->WriteBit(victimGuid[3]);
    }

    if (flags.AnimKit)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        hasAIAnimKit = self->GetAIAnimKitId();
        data->WriteBit(!hasAIAnimKit);
        hasMovementAnimKit = self->GetMovementAnimKitId();
        data->WriteBit(!hasMovementAnimKit);
        hasMeleeAnimKit = self->GetMeleeAnimKitId();
        data->WriteBit(!hasMeleeAnimKit);
    }

    data->FlushBits();

    if (PauseTimes && !PauseTimes->empty())
        data->append(PauseTimes->data(), PauseTimes->size());

    if (flags.MovementUpdate)
    {
        data->WriteByteSeq(guid[4]);
        *data << self->GetSpeed(MOVE_RUN_BACK);

        if (hasFallData)
        {
            if (hasFallDirection)
            {
                *data << float(self->m_movementInfo.jump.xyspeed);
                *data << float(self->m_movementInfo.jump.sinAngle);
                *data << float(self->m_movementInfo.jump.cosAngle);
            }

            *data << uint32(self->m_movementInfo.jump.fallTime);
            *data << float(self->m_movementInfo.jump.zspeed);
        }

        *data << self->GetSpeed(MOVE_SWIM_BACK);
        if (hasSplineElevation)
            *data << float(self->m_movementInfo.splineElevation);

        if (hasSpline)
            Movement::PacketBuilder::WriteCreateData(*self->movespline, *data);

        *data << float(self->GetPositionZ());
        data->WriteByteSeq(guid[5]);

        if (self->m_movementInfo.transport.guid)
        {
            ObjectGuid transGuid = self->m_movementInfo.transport.guid;

            data->WriteByteSeq(transGuid[5]);
            data->WriteByteSeq(transGuid[7]);
            *data << uint32(self->GetTransTime());
            *data << float(self->GetTransOffsetO());
            if (hasTransportTime2)
                *data << uint32(self->m_movementInfo.transport.time2);

            *data << float(self->GetTransOffsetY());
            *data << float(self->GetTransOffsetX());
            data->WriteByteSeq(transGuid[3]);
            *data << float(self->GetTransOffsetZ());
            data->WriteByteSeq(transGuid[0]);
            if (hasVehicleId)
                *data << uint32(self->m_movementInfo.transport.vehicleId);

            *data << int8(self->GetTransSeat());
            data->WriteByteSeq(transGuid[1]);
            data->WriteByteSeq(transGuid[6]);
            data->WriteByteSeq(transGuid[2]);
            data->WriteByteSeq(transGuid[4]);
        }

        *data << float(self->GetPositionX());
        *data << self->GetSpeed(MOVE_PITCH_RATE);
        data->WriteByteSeq(guid[3]);
        data->WriteByteSeq(guid[0]);
        *data << self->GetSpeed(MOVE_SWIM);
        *data << float(self->GetPositionY());
        data->WriteByteSeq(guid[7]);
        data->WriteByteSeq(guid[1]);
        data->WriteByteSeq(guid[2]);
        *data << self->GetSpeed(MOVE_WALK);

        //if (true)   // Has time, controlled by bit just after HasTransport
        *data << uint32(GameTime::GetGameTimeMS());

        *data << self->GetSpeed(MOVE_TURN_RATE);
        data->WriteByteSeq(guid[6]);
        *data << self->GetSpeed(MOVE_FLIGHT);
        if (!G3D::fuzzyEq(self->GetOrientation(), 0.0f))
            *data << float(self->GetOrientation());

        *data << self->GetSpeed(MOVE_RUN);
        if (hasPitch)
            *data << float(self->m_movementInfo.pitch);

        *data << self->GetSpeed(MOVE_FLIGHT_BACK);
    }

    if (flags.Vehicle)
    {
        *data << float(self->GetTransport() ? self->GetTransOffsetO() : self->GetOrientation());
        *data << uint32(self->GetVehicleKit()->GetVehicleInfo()->ID);
    }

    if (flags.MovementTransport)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        ObjectGuid transGuid = self->m_movementInfo.transport.guid;

        data->WriteByteSeq(transGuid[0]);
        data->WriteByteSeq(transGuid[5]);
        if (hasVehicleId)
            *data << uint32(self->m_movementInfo.transport.vehicleId);

        data->WriteByteSeq(transGuid[3]);
        *data << float(self->GetTransOffsetX());
        data->WriteByteSeq(transGuid[4]);
        data->WriteByteSeq(transGuid[6]);
        data->WriteByteSeq(transGuid[1]);
        *data << uint32(self->GetTransTime());
        *data << float(self->GetTransOffsetY());
        data->WriteByteSeq(transGuid[2]);
        data->WriteByteSeq(transGuid[7]);
        *data << float(self->GetTransOffsetZ());
        *data << int8(self->GetTransSeat());
        *data << float(self->GetTransOffsetO());
        if (hasTransportTime2)
            *data << uint32(self->m_movementInfo.transport.time2);
    }

    if (flags.Rotation)
        *data << uint64(ToGameObject()->GetPackedLocalRotation());

    if (flags.AreaTrigger)
    {
        // client doesn't use these values, so unk
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << uint8(0);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
    }

    if (flags.Stationary)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        *data << float(self->GetStationaryO());
        *data << float(self->GetStationaryX());
        *data << float(self->GetStationaryY());
        *data << float(self->GetStationaryZ());
    }

    if (flags.CombatVictim)
    {
        ObjectGuid victimGuid = self->GetVictim()->GetGUID();   // checked in BuildCreateUpdateBlockForPlayer
        data->WriteByteSeq(victimGuid[4]);
        data->WriteByteSeq(victimGuid[0]);
        data->WriteByteSeq(victimGuid[3]);
        data->WriteByteSeq(victimGuid[5]);
        data->WriteByteSeq(victimGuid[7]);
        data->WriteByteSeq(victimGuid[6]);
        data->WriteByteSeq(victimGuid[2]);
        data->WriteByteSeq(victimGuid[1]);
    }

    if (flags.AnimKit)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        if (hasAIAnimKit)
            *data << uint16(self->GetAIAnimKitId());
        if (hasMovementAnimKit)
            *data << uint16(self->GetMovementAnimKitId());
        if (hasMeleeAnimKit)
            *data << uint16(self->GetMeleeAnimKitId());
    }

    if (flags.ServerTime)
        *data << uint32(GameTime::GetGameTimeMS());
}

void Object::BuildValuesUpdate(uint8 updateType, ByteBuffer* data, Player* target) const
{
    if (!target)
        return;

    ByteBuffer fieldBuffer;
    UpdateMaskPacketBuilder updateMask(m_valuesCount);

    uint32* flags = nullptr;
    uint32 visibleFlag = GetUpdateFieldData(target, flags);
    ASSERT(flags);

    for (uint16 index = 0; index < m_valuesCount; ++index)
    {
        if (_fieldNotifyFlags & flags[index] ||
            ((updateType == UPDATETYPE_VALUES ? _changesMask.GetBit(index) : m_uint32Values[index]) && (flags[index] & visibleFlag)))
        {
            updateMask.SetBit(index);
            fieldBuffer << m_uint32Values[index];
        }
    }

    updateMask.AppendToPacket(data);
    data->append(fieldBuffer);
}

void Object::AddToObjectUpdateIfNeeded()
{
    if (m_inWorld && !m_objectUpdated)
        m_objectUpdated = AddToObjectUpdate();
}

void Object::ClearUpdateMask(bool remove)
{
    _changesMask.Clear();

    if (m_objectUpdated)
    {
        if (remove)
            RemoveFromObjectUpdate();

        m_objectUpdated = false;
    }
}

void Object::BuildFieldsUpdate(Player* player, UpdateDataMapType& data_map) const
{
    UpdateDataMapType::iterator iter = data_map.find(player);

    if (iter == data_map.end())
    {
        std::pair<UpdateDataMapType::iterator, bool> p = data_map.emplace(player, UpdateData(player->GetMapId()));
        ASSERT(p.second);
        iter = p.first;
    }

    BuildValuesUpdateBlockForPlayer(&iter->second, iter->first);
}

uint32 Object::GetUpdateFieldData(Player const* target, uint32*& flags) const
{
    uint32 visibleFlag = UF_FLAG_PUBLIC;

    if (target == this)
        visibleFlag |= UF_FLAG_PRIVATE;

    switch (GetTypeId())
    {
        case TYPEID_ITEM:
        case TYPEID_CONTAINER:
            flags = ItemUpdateFieldFlags;
            if (((Item const*)this)->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER | UF_FLAG_ITEM_OWNER;
            break;
        case TYPEID_UNIT:
        case TYPEID_PLAYER:
        {
            Player* plr = ToUnit()->GetCharmerOrOwnerPlayerOrPlayerItself();
            flags = UnitUpdateFieldFlags;
            if (ToUnit()->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;

            if (HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO))
                if (ToUnit()->HasAuraTypeWithCaster(SPELL_AURA_EMPATHY, target->GetGUID()))
                    visibleFlag |= UF_FLAG_SPECIAL_INFO;

            if (plr && plr->IsInSameRaidWith(target))
                visibleFlag |= UF_FLAG_PARTY_MEMBER;

            if (IsCreature())
                visibleFlag |= UF_FLAG_UNIT_ALL;
            break;
        }
        case TYPEID_GAMEOBJECT:
            flags = GameObjectUpdateFieldFlags;
            if (ToGameObject()->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;
            break;
        case TYPEID_DYNAMICOBJECT:
            flags = DynamicObjectUpdateFieldFlags;
            if (ToDynObject()->GetCasterGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;
            break;
        case TYPEID_CORPSE:
            flags = CorpseUpdateFieldFlags;
            if (ToCorpse()->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;
            break;
        case TYPEID_AREATRIGGER:
            flags = AreaTriggerUpdateFieldFlags;
            break;
        case TYPEID_OBJECT:
            break;
    }

    return visibleFlag;
}

void Object::_LoadIntoDataField(std::string const& data, uint32 startOffset, uint32 count)
{
    if (data.empty())
        return;

    Tokenizer tokens(data, ' ', count);

    if (tokens.size() != count)
        return;

    for (uint32 index = 0; index < count; ++index)
    {
        m_uint32Values[startOffset + index] = atoul(tokens[index]);
        _changesMask.SetBit(startOffset + index);
    }
}

void Object::SetInt32Value(uint16 index, int32 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (m_int32Values[index] != value)
    {
        m_int32Values[index] = value;
        _changesMask.SetBit(index);

        AddToObjectUpdateIfNeeded();
    }
}

void Object::SetUInt32Value(uint16 index, uint32 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (m_uint32Values[index] != value)
    {
        m_uint32Values[index] = value;
        _changesMask.SetBit(index);

        AddToObjectUpdateIfNeeded();
    }
}

void Object::UpdateUInt32Value(uint16 index, uint32 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    m_uint32Values[index] = value;
    _changesMask.SetBit(index);
}

void Object::SetUInt64Value(uint16 index, uint64 value)
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, true));
    if (*((uint64*)&(m_uint32Values[index])) != value)
    {
        m_uint32Values[index] = PAIR64_LOPART(value);
        m_uint32Values[index + 1] = PAIR64_HIPART(value);
        _changesMask.SetBit(index);
        _changesMask.SetBit(index + 1);

        AddToObjectUpdateIfNeeded();
    }
}

bool Object::AddGuidValue(uint16 index, ObjectGuid value)
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, true));
    if (value && !*((ObjectGuid*)&(m_uint32Values[index])))
    {
        *((ObjectGuid*)&(m_uint32Values[index])) = value;
        _changesMask.SetBit(index);
        _changesMask.SetBit(index + 1);

        AddToObjectUpdateIfNeeded();

        return true;
    }

    return false;
}

bool Object::RemoveGuidValue(uint16 index, ObjectGuid value)
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, true));
    if (value && *((ObjectGuid*)&(m_uint32Values[index])) == value)
    {
        m_uint32Values[index] = 0;
        m_uint32Values[index + 1] = 0;
        _changesMask.SetBit(index);
        _changesMask.SetBit(index + 1);

        AddToObjectUpdateIfNeeded();

        return true;
    }

    return false;
}

void Object::SetFloatValue(uint16 index, float value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (m_floatValues[index] != value)
    {
        m_floatValues[index] = value;
        _changesMask.SetBit(index);

        AddToObjectUpdateIfNeeded();
    }
}

void Object::SetByteValue(uint16 index, uint8 offset, uint8 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));
    ASSERT(offset < 4);

    if (uint8(m_uint32Values[index] >> (offset * 8)) != value)
    {
        m_uint32Values[index] &= ~uint32(uint32(0xFF) << (offset * 8));
        m_uint32Values[index] |= uint32(uint32(value) << (offset * 8));
        _changesMask.SetBit(index);

        AddToObjectUpdateIfNeeded();
    }
}

void Object::SetUInt16Value(uint16 index, uint8 offset, uint16 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));
    ASSERT(offset < 2);

    if (uint16(m_uint32Values[index] >> (offset * 16)) != value)
    {
        m_uint32Values[index] &= ~uint32(uint32(0xFFFF) << (offset * 16));
        m_uint32Values[index] |= uint32(uint32(value) << (offset * 16));
        _changesMask.SetBit(index);

        AddToObjectUpdateIfNeeded();
    }
}

void Object::SetGuidValue(uint16 index, ObjectGuid value)
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, true));
    if (*((ObjectGuid*)&(m_uint32Values[index])) != value)
    {
        *((ObjectGuid*)&(m_uint32Values[index])) = value;
        _changesMask.SetBit(index);
        _changesMask.SetBit(index + 1);

        AddToObjectUpdateIfNeeded();
    }
}

void Object::SetStatFloatValue(uint16 index, float value)
{
    if (value < 0)
        value = 0.0f;

    SetFloatValue(index, value);
}

void Object::SetStatInt32Value(uint16 index, int32 value)
{
    if (value < 0)
        value = 0;

    SetUInt32Value(index, uint32(value));
}

void Object::ApplyModUInt32Value(uint16 index, int32 val, bool apply)
{
    int32 cur = GetUInt32Value(index);
    cur += (apply ? val : -val);
    if (cur < 0)
        cur = 0;
    SetUInt32Value(index, cur);
}

void Object::ApplyModInt32Value(uint16 index, int32 val, bool apply)
{
    int32 cur = GetInt32Value(index);
    cur += (apply ? val : -val);
    SetInt32Value(index, cur);
}

void Object::ApplyModSignedFloatValue(uint16 index, float  val, bool apply)
{
    float cur = GetFloatValue(index);
    cur += (apply ? val : -val);
    SetFloatValue(index, cur);
}

void Object::ApplyModPositiveFloatValue(uint16 index, float  val, bool apply)
{
    float cur = GetFloatValue(index);
    cur += (apply ? val : -val);
    if (cur < 0)
        cur = 0;
    SetFloatValue(index, cur);
}

void Object::SetFlag(uint16 index, uint32 newFlag)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));
    uint32 oldval = m_uint32Values[index];
    uint32 newval = oldval | newFlag;

    if (oldval != newval)
    {
        m_uint32Values[index] = newval;
        _changesMask.SetBit(index);

        AddToObjectUpdateIfNeeded();
    }
}

void Object::RemoveFlag(uint16 index, uint32 oldFlag)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    uint32 oldval = m_uint32Values[index];
    uint32 newval = oldval & ~oldFlag;

    if (oldval != newval)
    {
        m_uint32Values[index] = newval;
        _changesMask.SetBit(index);

        AddToObjectUpdateIfNeeded();
    }
}

void Object::ToggleFlag(uint16 index, uint32 flag)
{
    if (HasFlag(index, flag))
        RemoveFlag(index, flag);
    else
        SetFlag(index, flag);
}

bool Object::HasFlag(uint16 index, uint32 flag) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));
    return (m_uint32Values[index] & flag) != 0;
}

void Object::ApplyModFlag(uint16 index, uint32 flag, bool apply)
{
    if (apply) SetFlag(index, flag); else RemoveFlag(index, flag);
}

void Object::SetByteFlag(uint16 index, uint8 offset, uint8 newFlag)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));
    ASSERT(offset < 4);

    if (!(uint8(m_uint32Values[index] >> (offset * 8)) & newFlag))
    {
        m_uint32Values[index] |= uint32(uint32(newFlag) << (offset * 8));
        _changesMask.SetBit(index);

        AddToObjectUpdateIfNeeded();
    }
}

void Object::RemoveByteFlag(uint16 index, uint8 offset, uint8 oldFlag)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));
    ASSERT(offset < 4);

    if (uint8(m_uint32Values[index] >> (offset * 8)) & oldFlag)
    {
        m_uint32Values[index] &= ~uint32(uint32(oldFlag) << (offset * 8));
        _changesMask.SetBit(index);

        AddToObjectUpdateIfNeeded();
    }
}

void Object::ToggleByteFlag(uint16 index, uint8 offset, uint8 flag)
{
    if (HasByteFlag(index, offset, flag))
        RemoveByteFlag(index, offset, flag);
    else
        SetByteFlag(index, offset, flag);
}

bool Object::HasByteFlag(uint16 index, uint8 offset, uint8 flag) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    ASSERT(offset < 4);
    return (((uint8*)&m_uint32Values[index])[offset] & flag) != 0;
}

void Object::ApplyModByteFlag(uint16 index, uint8 offset, uint8 flag, bool apply)
{
    if (apply) SetByteFlag(index, offset, flag); else RemoveByteFlag(index, offset, flag);
}

void Object::SetFlag64(uint16 index, uint64 newFlag)
{
    uint64 oldval = GetUInt64Value(index);
    uint64 newval = oldval | newFlag;
    SetUInt64Value(index, newval);
}

void Object::RemoveFlag64(uint16 index, uint64 oldFlag)
{
    uint64 oldval = GetUInt64Value(index);
    uint64 newval = oldval & ~oldFlag;
    SetUInt64Value(index, newval);
}

void Object::ToggleFlag64(uint16 index, uint64 flag)
{
    if (HasFlag64(index, flag))
        RemoveFlag64(index, flag);
    else
        SetFlag64(index, flag);
}

bool Object::HasFlag64(uint16 index, uint64 flag) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    return (GetUInt64Value(index) & flag) != 0;
}

void Object::ApplyModFlag64(uint16 index, uint64 flag, bool apply)
{
    if (apply) SetFlag64(index, flag); else RemoveFlag64(index, flag);
}

bool Object::PrintIndexError(uint32 index, bool set) const
{
    TC_LOG_ERROR("misc", "Attempt to %s non-existing value field: %u (count: %u) for object typeid: %u type mask: %u", (set ? "set value to" : "get value from"), index, m_valuesCount, GetTypeId(), m_objectType);

    // ASSERT must fail after function call
    return false;
}

WorldObject::WorldObject(bool isWorldObject) : Object(), WorldLocation(), LastUsedScriptID(0),
m_movementInfo(), m_name(""), m_isActive(false), m_isFarVisible(false), m_isWorldObject(isWorldObject), m_zoneScript(nullptr),
m_transport(nullptr), m_zoneId(0), m_areaId(0), m_staticFloorZ(VMAP_INVALID_HEIGHT), m_outdoors(true), m_liquidStatus(LIQUID_MAP_NO_WATER),
m_wmoGroupID(0), m_currMap(nullptr), m_InstanceId(0), _dbPhase(0), m_notifyflags(0), _heartbeatTimer(HEARTBEAT_INTERVAL),
m_aiAnimKitId(0), m_movementAnimKitId(0), m_meleeAnimKitId(0)
{
    m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE | GHOST_VISIBILITY_GHOST);
    m_serverSideVisibilityDetect.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE);
}

void WorldObject::SetWorldObject(bool on)
{
    if (!IsInWorld())
        return;

    GetMap()->AddObjectToSwitchList(this, on);
}

bool WorldObject::IsWorldObject() const
{
    if (m_isWorldObject)
        return true;

    if (ToCreature() && ToCreature()->m_isTempWorldObject)
        return true;

    return false;
}

void WorldObject::setActive(bool on)
{
    if (m_isActive == on)
        return;

    if (GetTypeId() == TYPEID_PLAYER)
        return;

    m_isActive = on;

    if (on && !IsInWorld())
        return;

    Map* map = FindMap();
    if (!map)
        return;

    if (on)
        map->AddToActive(this);
    else
        map->RemoveFromActive(this);
}

void WorldObject::SetFarVisible(bool on)
{
    if (GetTypeId() == TYPEID_PLAYER)
        return;

    m_isFarVisible = on;
}

void WorldObject::SetVisibilityDistanceOverride(VisibilityDistanceType type)
{
    ASSERT(type < VisibilityDistanceType::Max);
    if (GetTypeId() == TYPEID_PLAYER)
        return;

    m_visibilityDistanceOverride = VisibilityDistances[AsUnderlyingType(type)];
}

void WorldObject::CleanupsBeforeDelete(bool /*finalCleanup*/)
{
    if (IsInWorld())
        RemoveFromWorld();

    if (TransportBase* transport = GetTransport())
        transport->RemovePassenger(this);

    m_Events.KillAllEvents(false);                      // non-delatable (currently cast spells) will not deleted now but it will deleted at call in Map::RemoveAllObjectsInRemoveList
}

void WorldObject::_Create(ObjectGuid::LowType guidlow, HighGuid guidhigh)
{
    Object::_Create(guidlow, 0, guidhigh);
}

void WorldObject::UpdatePositionData()
{
    PositionFullTerrainStatus data;
    GetMap()->GetFullTerrainStatusForPosition(GetPhaseShift(), GetPositionX(), GetPositionY(), GetPositionZ(), data, map_liquidHeaderTypeFlags::AllLiquids, GetCollisionHeight());
    ProcessPositionDataChanged(data);
}

void WorldObject::ProcessPositionDataChanged(PositionFullTerrainStatus const& data)
{
    m_zoneId = m_areaId = data.areaId;
    if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(m_areaId))
        if (area->ParentAreaID && area->GetFlags().HasFlag(AreaFlags::IsSubzone))
            m_zoneId = area->ParentAreaID;
    m_outdoors = data.outdoors;
    m_staticFloorZ = data.floorZ;
    m_liquidStatus = data.liquidStatus;
    m_wmoGroupID = data.areaInfo.has_value() ? data.areaInfo->groupId : 0;
}

void WorldObject::AddToWorld()
{
    Object::AddToWorld();
    GetMap()->GetZoneAndAreaId(GetPhaseShift(), m_zoneId, m_areaId, GetPositionX(), GetPositionY(), GetPositionZ());
}
void WorldObject::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    UpdateObjectVisibilityOnDestroy();

    Object::RemoveFromWorld();
}

bool WorldObject::IsInWorldPvpZone() const
{
    switch (GetZoneId())
    {
        case 4197: // Wintergrasp
        case 5095: // Tol Barad
            return true;
            break;
        default:
            return false;
            break;
    }
}

InstanceScript* WorldObject::GetInstanceScript() const
{
    Map* map = GetMap();
    return map->IsDungeon() ? ((InstanceMap*)map)->GetInstanceScript() : nullptr;
}

float WorldObject::GetDistanceZ(const WorldObject* obj) const
{
    float dz = std::fabs(GetPositionZ() - obj->GetPositionZ());
    float sizefactor = GetCombatReach() + obj->GetCombatReach();
    float dist = dz - sizefactor;
    return (dist > 0 ? dist : 0);
}

bool WorldObject::_IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D, bool incOwnRadius, bool incTargetRadius) const
{
    float sizefactor = 0.f;
    sizefactor += incOwnRadius ? GetCombatReach() : 0.0f;
    sizefactor += incTargetRadius ? obj->GetCombatReach() : 0.0f;
    float maxdist = dist2compare + sizefactor;

    Position const* thisOrTransport = this;
    Position const* objOrObjTransport = obj;

    if (GetTransport() && obj->GetTransport() && obj->GetTransport()->GetTransportGUID() == GetTransport()->GetTransportGUID())
    {
        thisOrTransport = &m_movementInfo.transport.pos;
        objOrObjTransport = &obj->m_movementInfo.transport.pos;
    }

    if (is3D)
        return thisOrTransport->IsInDist(objOrObjTransport, maxdist);
    else
        return thisOrTransport->IsInDist2d(objOrObjTransport, maxdist);
}

float WorldObject::GetDistance(const WorldObject* obj) const
{
    float d = GetExactDist(obj) - GetCombatReach() - obj->GetCombatReach();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance(const Position &pos) const
{
    float d = GetExactDist(&pos) - GetCombatReach();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance(float x, float y, float z) const
{
    float d = GetExactDist(x, y, z) - GetCombatReach();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance2d(const WorldObject* obj) const
{
    float d = GetExactDist2d(obj) - GetCombatReach() - obj->GetCombatReach();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance2d(float x, float y) const
{
    float d = GetExactDist2d(x, y) - GetCombatReach();
    return d > 0.0f ? d : 0.0f;
}

bool WorldObject::IsSelfOrInSameMap(const WorldObject* obj) const
{
    if (this == obj)
        return true;
    return IsInMap(obj);
}

bool WorldObject::IsInMap(const WorldObject* obj) const
{
    if (obj)
        return IsInWorld() && obj->IsInWorld() && (GetMap() == obj->GetMap());
    return false;
}

bool WorldObject::IsWithinDist3d(float x, float y, float z, float dist) const
{
    return IsInDist(x, y, z, dist);
}

bool WorldObject::IsWithinDist3d(Position const* pos, float dist) const
{
    return IsInDist(pos, dist);
}

bool WorldObject::IsWithinDist2d(float x, float y, float dist) const
{
    return IsInDist2d(x, y, dist);
}

bool WorldObject::IsWithinDist2d(Position const* pos, float dist) const
{
    return IsInDist2d(pos, dist);
}

bool WorldObject::IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D /*= true*/) const
{
    return obj && _IsWithinDist(obj, dist2compare, is3D);
}

bool WorldObject::IsWithinDistInMap(WorldObject const* obj, float dist2compare, bool is3D /*= true*/, bool incOwnRadius /*= true*/, bool incTargetRadius /*= true*/) const
{
    return obj && IsInMap(obj) && IsInPhase(obj) && _IsWithinDist(obj, dist2compare, is3D, incOwnRadius, incTargetRadius);
}
Position WorldObject::GetHitSpherePointFor(Position const& dest) const
{
    G3D::Vector3 vThis(GetPositionX(), GetPositionY(), GetPositionZ() + GetCollisionHeight());
    G3D::Vector3 vObj(dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ());
    G3D::Vector3 contactPoint = vThis + (vObj - vThis).directionOrZero() * std::min(dest.GetExactDist(GetPosition()), GetCombatReach());

    return Position(contactPoint.x, contactPoint.y, contactPoint.z, GetAngle(contactPoint.x, contactPoint.y));
}

bool WorldObject::IsWithinLOS(float ox, float oy, float oz, LineOfSightChecks checks, VMAP::ModelIgnoreFlags ignoreFlags) const
{
    if (IsInWorld())
    {
        oz += GetCollisionHeight();
        float x, y, z;
        if (GetTypeId() == TYPEID_PLAYER)
        {
            GetPosition(x, y, z);
            z += GetCollisionHeight();
        }
        else
            GetHitSpherePointFor({ ox, oy, oz }, x, y, z);

        return GetMap()->isInLineOfSight(GetPhaseShift(), x, y, z, ox, oy, oz, checks, ignoreFlags);
    }

    return true;
}

bool WorldObject::IsWithinLOSInMap(WorldObject const* obj, LineOfSightChecks checks, VMAP::ModelIgnoreFlags ignoreFlags) const
{
    if (!IsInMap(obj))
        return false;

    float ox, oy, oz;
    if (obj->GetTypeId() == TYPEID_PLAYER)
    {
        obj->GetPosition(ox, oy, oz);
        oz += GetCollisionHeight();
    }
    else
        obj->GetHitSpherePointFor({ GetPositionX(), GetPositionY(), GetPositionZ() + GetCollisionHeight() }, ox, oy, oz);

    float x, y, z;
    if (GetTypeId() == TYPEID_PLAYER)
    {
        GetPosition(x, y, z);
        z += GetCollisionHeight();
    }
    else
        GetHitSpherePointFor({ obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ() + obj->GetCollisionHeight() }, x, y, z);

    return GetMap()->isInLineOfSight(GetPhaseShift(), x, y, z, ox, oy, oz, checks, ignoreFlags);
}

void WorldObject::GetHitSpherePointFor(Position const& dest, float& x, float& y, float& z) const
{
    Position pos = GetHitSpherePointFor(dest);
    x = pos.GetPositionX();
    y = pos.GetPositionY();
    z = pos.GetPositionZ();
}

bool WorldObject::GetDistanceOrder(WorldObject const* obj1, WorldObject const* obj2, bool is3D /* = true */) const
{
    float dx1 = GetPositionX() - obj1->GetPositionX();
    float dy1 = GetPositionY() - obj1->GetPositionY();
    float distsq1 = dx1*dx1 + dy1*dy1;
    if (is3D)
    {
        float dz1 = GetPositionZ() - obj1->GetPositionZ();
        distsq1 += dz1*dz1;
    }

    float dx2 = GetPositionX() - obj2->GetPositionX();
    float dy2 = GetPositionY() - obj2->GetPositionY();
    float distsq2 = dx2*dx2 + dy2*dy2;
    if (is3D)
    {
        float dz2 = GetPositionZ() - obj2->GetPositionZ();
        distsq2 += dz2*dz2;
    }

    return distsq1 < distsq2;
}

bool WorldObject::IsInRange(WorldObject const* obj, float minRange, float maxRange, bool is3D /* = true */) const
{
    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float distsq = dx*dx + dy*dy;
    if (is3D)
    {
        float dz = GetPositionZ() - obj->GetPositionZ();
        distsq += dz*dz;
    }

    float sizefactor = GetCombatReach() + obj->GetCombatReach();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInRange2d(float x, float y, float minRange, float maxRange) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float distsq = dx*dx + dy*dy;

    float sizefactor = GetCombatReach();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInRange3d(float x, float y, float z, float minRange, float maxRange) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float dz = GetPositionZ() - z;
    float distsq = dx*dx + dy*dy + dz*dz;

    float sizefactor = GetCombatReach();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInBetween(Position const& pos1, Position const& pos2, float size) const
{
    float dist = GetExactDist2d(pos1);

    // not using sqrt() for performance
    if ((dist * dist) >= pos1.GetExactDist2dSq(pos2))
        return false;

    if (!size)
        size = GetCombatReach() / 2;

    float angle = pos1.GetAngle(pos2);

    // not using sqrt() for performance
    return (size * size) >= GetExactDist2dSq(pos1.GetPositionX() + std::cos(angle) * dist, pos1.GetPositionY() + std::sin(angle) * dist);
}

bool WorldObject::isInFront(WorldObject const* target,  float arc) const
{
    return HasInArc(arc, target);
}

bool WorldObject::isInBack(WorldObject const* target, float arc) const
{
    return !HasInArc(2 * float(M_PI) - arc, target);
}

void WorldObject::GetRandomPoint(const Position &pos, float distance, float &rand_x, float &rand_y, float &rand_z) const
{
    if (!distance)
    {
        pos.GetPosition(rand_x, rand_y, rand_z);
        return;
    }

    // angle to face `obj` to `this`
    float angle = rand_norm() * static_cast<float>(2 * M_PI);
    float new_dist = rand_norm() + rand_norm();
    new_dist = distance * (new_dist > 1 ? new_dist - 2 : new_dist);

    rand_x = pos.m_positionX + new_dist * std::cos(angle);
    rand_y = pos.m_positionY + new_dist * std::sin(angle);
    rand_z = pos.m_positionZ;

    Trinity::NormalizeMapCoord(rand_x);
    Trinity::NormalizeMapCoord(rand_y);
    UpdateGroundPositionZ(rand_x, rand_y, rand_z);            // update to LOS height if available
}

Position WorldObject::GetRandomPoint(const Position &srcPos, float distance) const
{
    float x, y, z;
    GetRandomPoint(srcPos, distance, x, y, z);
    return Position(x, y, z, GetOrientation());
}

void WorldObject::UpdateGroundPositionZ(float x, float y, float &z) const
{
    float new_z = GetMapHeight(x, y, z);
    if (new_z > INVALID_HEIGHT)
        z = new_z + (isType(TYPEMASK_UNIT) ? static_cast<Unit const*>(this)->GetHoverOffset() : 0.0f);
}

void WorldObject::UpdateAllowedPositionZ(float x, float y, float &z, float* groundZ) const
{
    // TODO: Allow transports to be part of dynamic vmap tree
    if (GetTransport())
    {
        if (groundZ)
            *groundZ = z;

        return;
    }

    if (Unit const* unit = ToUnit())
    {
        if (!unit->CanFly())
        {
            bool canSwim = unit->CanSwim();
            float ground_z = z;
            float max_z;
            if (canSwim)
                max_z = GetMapWaterOrGroundLevel(x, y, z, &ground_z);
            else
                max_z = ground_z = GetMapHeight(x, y, z);

            if (max_z > INVALID_HEIGHT)
            {
                // hovering units cannot go below their hover height
                float hoverOffset = unit->GetHoverOffset();
                max_z += hoverOffset;
                ground_z += hoverOffset;

                if (z > max_z)
                    z = max_z;
                else if (z < ground_z)
                    z = ground_z;
            }

            if (groundZ)
                *groundZ = ground_z;
        }
        else
        {
            float ground_z = GetMapHeight(x, y, z) + unit->GetHoverOffset();
            if (z < ground_z)
                z = ground_z;

            if (groundZ)
                *groundZ = ground_z;
        }
    }
    else
    {
        float ground_z = GetMapHeight(x, y, z);
        if (ground_z > INVALID_HEIGHT)
            z = ground_z;

        if (groundZ)
            *groundZ = ground_z;
    }
}

float WorldObject::GetGridActivationRange() const
{
    if (isActiveObject())
    {
        if (GetTypeId() == TYPEID_PLAYER && ToPlayer()->GetCinematicMgr()->IsOnCinematic())
            return std::max(DEFAULT_VISIBILITY_INSTANCE, GetMap()->GetVisibilityRange());

        return GetMap()->GetVisibilityRange();
    }

    if (Creature const* thisCreature = ToCreature())
        return thisCreature->m_SightDistance;

    return 0.0f;
}

float WorldObject::GetVisibilityRange() const
{
    if (IsVisibilityOverriden() && !ToPlayer())
        return *m_visibilityDistanceOverride;
    else if (IsFarVisible() && !ToPlayer())
        return MAX_VISIBILITY_DISTANCE;
    else
        return GetMap()->GetVisibilityRange();
}

float WorldObject::GetSightRange(const WorldObject* target) const
{
    if (ToUnit())
    {
        if (ToPlayer())
        {
            if (target && target->IsVisibilityOverriden() && !target->ToPlayer())
                return *target->m_visibilityDistanceOverride;
            else if (target && target->IsFarVisible() && !target->ToPlayer())
                return MAX_VISIBILITY_DISTANCE;
            else if (ToPlayer()->GetCinematicMgr()->IsOnCinematic())
                return DEFAULT_VISIBILITY_INSTANCE;
            else
                return GetMap()->GetVisibilityRange();
        }
        else if (ToCreature())
            return ToCreature()->m_SightDistance;
        else
            return SIGHT_RANGE_UNIT;
    }

    if (ToDynObject() && isActiveObject())
        return GetMap()->GetVisibilityRange();

    return 0.0f;
}

bool WorldObject::CheckPrivateObjectOwnerVisibility(WorldObject const* seer) const
{
    if (!IsPrivateObject())
        return true;

    // Owner of this private object
    if (_privateObjectOwner == seer->GetGUID())
        return true;

    // Another private object of the same owner
    if (_privateObjectOwner == seer->GetPrivateObjectOwner())
        return true;

    if (Player const* playerSeer = seer->ToPlayer())
        if (playerSeer->IsInGroup(_privateObjectOwner))
            return true;

    return false;
}

bool WorldObject::CanSeeOrDetect(WorldObject const* obj, bool ignoreStealth, bool distanceCheck, bool checkAlert) const
{
    if (this == obj)
        return true;

    if (obj->IsNeverVisible() || CanNeverSee(obj))
        return false;

    if (obj->IsAlwaysVisibleFor(this) || CanAlwaysSee(obj))
        return true;

    if (!obj->CheckPrivateObjectOwnerVisibility(this))
        return false;

    bool corpseVisibility = false;
    if (distanceCheck)
    {
        bool corpseCheck = false;
        if (Player const* thisPlayer = ToPlayer())
        {
            if (thisPlayer->isDead() && thisPlayer->GetHealth() > 0 && // Cheap way to check for ghost state
                !(obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GHOST) & m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GHOST) & GHOST_VISIBILITY_GHOST))
            {
                if (Corpse* corpse = thisPlayer->GetCorpse())
                {
                    corpseCheck = true;
                    if (corpse->IsWithinDist(thisPlayer, GetSightRange(obj), false))
                        if (corpse->IsWithinDist(obj, GetSightRange(obj), false))
                            corpseVisibility = true;
                }
            }

            if (Unit const* target = obj->ToUnit())
            {
                // Don't allow to detect vehicle accessories if you can't see vehicle
                if (Unit const* vehicle = target->GetVehicleBase())
                    if (!thisPlayer->HaveAtClient(vehicle))
                        return false;
            }
        }

        WorldObject const* viewpoint = this;
        if (Player const* player = this->ToPlayer())
            viewpoint = player->GetViewpoint();

        if (!viewpoint)
            viewpoint = this;

        if (!corpseCheck && !viewpoint->IsWithinDist(obj, GetSightRange(obj), false))
            return false;
    }

    // GM visibility off or hidden NPC
    if (!obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GM))
    {
        // Stop checking other things for GMs
        if (m_serverSideVisibilityDetect.GetValue(SERVERSIDE_VISIBILITY_GM))
            return true;
    }
    else
        return m_serverSideVisibilityDetect.GetValue(SERVERSIDE_VISIBILITY_GM) >= obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GM);

    // Ghost players, Spirit Healers, and some other NPCs
    if (!corpseVisibility && !(obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GHOST) & m_serverSideVisibilityDetect.GetValue(SERVERSIDE_VISIBILITY_GHOST)))
    {
        // Alive players can see dead players in some cases, but other objects can't do that
        if (Player const* thisPlayer = ToPlayer())
        {
            if (Player const* objPlayer = obj->ToPlayer())
            {
                if (thisPlayer->GetTeam() != objPlayer->GetTeam() || !thisPlayer->IsGroupVisibleFor(objPlayer))
                    return false;
            }
            else
                return false;
        }
        else
            return false;
    }

    if (obj->IsInvisibleDueToDespawn())
        return false;

    if (!CanDetect(obj, ignoreStealth, checkAlert))
        return false;

    return true;
}

bool WorldObject::CanNeverSee(WorldObject const* obj) const
{
    return GetMap() != obj->GetMap() || !IsInPhase(obj);
}

bool WorldObject::CanDetect(WorldObject const* obj, bool ignoreStealth, bool checkAlert) const
{
    const WorldObject* seer = this;

    // Pets don't have detection, they use the detection of their masters
    if (Unit const* thisUnit = ToUnit())
        if (Unit* controller = thisUnit->GetCharmerOrOwner())
            seer = controller;

    if (obj->IsAlwaysDetectableFor(seer))
        return true;

    if (!ignoreStealth && !seer->CanDetectInvisibilityOf(obj))
        return false;

    if (!ignoreStealth && !seer->CanDetectStealthOf(obj, checkAlert))
        return false;

    return true;
}

bool WorldObject::CanDetectInvisibilityOf(WorldObject const* obj) const
{
    uint32 mask = obj->m_invisibility.GetFlags() & m_invisibilityDetect.GetFlags();

    // Check for not detected types
    if (mask != obj->m_invisibility.GetFlags())
        return false;

    for (uint32 i = 0; i < TOTAL_INVISIBILITY_TYPES; ++i)
    {
        if (!(mask & (1 << i)))
            continue;

        int32 objInvisibilityValue = obj->m_invisibility.GetValue(InvisibilityType(i));
        int32 ownInvisibilityDetectValue = m_invisibilityDetect.GetValue(InvisibilityType(i));

        // Too low value to detect
        if (ownInvisibilityDetectValue < objInvisibilityValue)
            return false;
    }

    return true;
}

bool WorldObject::CanDetectStealthOf(WorldObject const* obj, bool checkAlert) const
{
    // Combat reach is the minimal distance (both in front and behind),
    //   and it is also used in the range calculation.
    // One stealth point increases the visibility range by 0.3 yard.

    if (!obj->m_stealth.GetFlags())
        return true;

    float distance = GetExactDist(obj);
    float combatReach = 0.0f;

    Unit const* unit = ToUnit();
    if (unit)
        combatReach = unit->GetCombatReach();

    if (distance < combatReach)
        return true;

    // Only check back for units, it does not make sense for gameobjects
    if (unit && !HasInArc(float(M_PI), obj))
        return false;

    // Traps should detect stealth always
    if (GameObject const* go = ToGameObject())
        if (go->GetGoType() == GAMEOBJECT_TYPE_TRAP)
            return true;

    GameObject const* go = obj->ToGameObject();
    for (uint32 i = 0; i < TOTAL_STEALTH_TYPES; ++i)
    {
        if (!(obj->m_stealth.GetFlags() & (1 << i)))
            continue;

        if (unit && unit->HasAuraTypeWithMiscvalue(SPELL_AURA_DETECT_STEALTH, i))
            return true;

        // Starting points
        int32 detectionValue = 30;

        // Level difference: 5 point / level, starting from level 1.
        // There may be spells for this and the starting points too, but
        // not in the DBCs of the client.
        detectionValue += int32(getLevelForTarget(obj) - 1) * 5;

        // Apply modifiers
        detectionValue += m_stealthDetect.GetValue(StealthType(i));
        if (go)
            if (Unit* owner = go->GetOwner())
                detectionValue -= int32(owner->getLevelForTarget(this) - 1) * 5;

        detectionValue -= obj->m_stealth.GetValue(StealthType(i));

        // Calculate max distance
        float visibilityRange = float(detectionValue) * 0.3f + combatReach;

        // If this unit is an NPC then player detect range doesn't apply
        if (unit && unit->GetTypeId() == TYPEID_PLAYER && visibilityRange > MAX_PLAYER_STEALTH_DETECT_RANGE)
            visibilityRange = MAX_PLAYER_STEALTH_DETECT_RANGE;

        // When checking for alert state, look 8% further, and then 1.5 yards more than that.
        if (checkAlert)
            visibilityRange += (visibilityRange * 0.08f) + 1.5f;

        // If checking for alert, and creature's visibility range is greater than aggro distance, No alert
        Unit const* tunit = obj->ToUnit();
        if (checkAlert && unit && unit->ToCreature() && visibilityRange >= unit->ToCreature()->GetAttackDistance(tunit) + unit->ToCreature()->m_CombatDistance)
            return false;

        if (distance > visibilityRange)
            return false;
    }

    return true;
}

void Object::ForceValuesUpdateAtIndex(uint32 i)
{
    _changesMask.SetBit(i);
    AddToObjectUpdateIfNeeded();
}

void WorldObject::SendMessageToSet(WorldPacket const* data, bool self) const
{
    if (IsInWorld())
        SendMessageToSetInRange(data, GetVisibilityRange(), self);
}

void WorldObject::SendMessageToSetInRange(WorldPacket const* data, float dist, bool /*self*/) const
{
    Trinity::MessageDistDeliverer notifier(this, data, dist);
    Cell::VisitWorldObjects(this, notifier, dist);
}

void WorldObject::SendMessageToSet(WorldPacket const* data, Player const* skipped_rcvr) const
{
    Trinity::MessageDistDeliverer notifier(this, data, GetVisibilityRange(), false, skipped_rcvr);
    Cell::VisitWorldObjects(this, notifier, GetVisibilityRange());
}

void WorldObject::SetMap(Map* map)
{
    ASSERT(map);
    ASSERT(!IsInWorld());
    if (m_currMap == map) // command add npc: first create, than loadfromdb
        return;
    if (m_currMap)
    {
        TC_LOG_FATAL("misc", "WorldObject::SetMap: obj %u new map %u %u, old map %u %u", (uint32)GetTypeId(), map->GetId(), map->GetInstanceId(), m_currMap->GetId(), m_currMap->GetInstanceId());
        ABORT();
    }
    m_currMap = map;
    m_mapId = map->GetId();
    m_InstanceId = map->GetInstanceId();
    if (IsWorldObject())
        m_currMap->AddWorldObject(this);
}

void WorldObject::ResetMap()
{
    ASSERT(m_currMap);
    ASSERT(!IsInWorld());
    if (IsWorldObject())
        m_currMap->RemoveWorldObject(this);
    m_currMap = nullptr;
    //maybe not for corpse
    //m_mapId = 0;
    //m_InstanceId = 0;
}

void WorldObject::AddObjectToRemoveList()
{
    ASSERT(m_uint32Values);

    Map* map = FindMap();
    if (!map)
    {
        TC_LOG_ERROR("misc", "Object (TypeId: %u Entry: %u GUID: %u) at attempt add to move list not have valid map (Id: %u).", GetTypeId(), GetEntry(), GetGUID().GetCounter(), GetMapId());
        return;
    }

    map->AddObjectToRemoveList(this);
}

TempSummon* Map::SummonCreature(uint32 entry, Position const& pos, SummonCreatureExtraArgs const& summonArgs /*= { }*/)
{
    uint32 mask = UNIT_MASK_SUMMON;

    if (summonArgs.SummonProperties)
    {
        switch (summonArgs.SummonProperties->Control)
        {
            case SUMMON_CATEGORY_PET:
                mask = UNIT_MASK_GUARDIAN;
                break;
            case SUMMON_CATEGORY_PUPPET:
                mask = UNIT_MASK_PUPPET;
                break;
            case SUMMON_CATEGORY_VEHICLE:
                mask = UNIT_MASK_MINION;
                break;
            case SUMMON_CATEGORY_WILD:
            case SUMMON_CATEGORY_ALLY:
            case SUMMON_CATEGORY_UNK:
            {
                switch (SummonTitle(summonArgs.SummonProperties->Title))
                {
                    case SummonTitle::Minion:
                    case SummonTitle::Guardian:
                    case SummonTitle::Runeblade:
                        mask = UNIT_MASK_GUARDIAN;
                        break;
                    case SummonTitle::Totem:
                    case SummonTitle::Lightwell:
                        mask = UNIT_MASK_TOTEM;
                        break;
                    case SummonTitle::Vehicle:
                    case SummonTitle::Mount:
                        mask = UNIT_MASK_SUMMON;
                        break;
                    case SummonTitle::Companion:
                        mask = UNIT_MASK_MINION;
                        break;
                    default:
                        if (summonArgs.SummonProperties->Flags & 512) // Mirror Image, Summon Gargoyle
                            mask = UNIT_MASK_GUARDIAN;
                        break;
                }
                break;
            }
            default:
                return nullptr;
        }
    }

    TempSummon* summon = nullptr;
    switch (mask)
    {
        case UNIT_MASK_SUMMON:
            summon = new TempSummon(summonArgs.SummonProperties, summonArgs.Summoner, false);
            break;
        case UNIT_MASK_GUARDIAN:
            summon = new Guardian(summonArgs.SummonProperties, summonArgs.Summoner, false);
            break;
        case UNIT_MASK_PUPPET:
            summon = new Puppet(summonArgs.SummonProperties, summonArgs.Summoner);
            break;
        case UNIT_MASK_TOTEM:
            summon = new Totem(summonArgs.SummonProperties, summonArgs.Summoner);
            break;
        case UNIT_MASK_MINION:
            summon = new Minion(summonArgs.SummonProperties, summonArgs.Summoner, false);
            break;
    }

    // Create creature entity
    if (!summon->Create(GenerateLowGuid<HighGuid::Unit>(), this, entry, pos, nullptr, summonArgs.VehicleRecID, true))
    {
        delete summon;
        return nullptr;
    }

    // Inherit summoner's Phaseshift
    if (summonArgs.Summoner)
        PhasingHandler::InheritPhaseShift(summon, summonArgs.Summoner);

    TransportBase* transport = summonArgs.Summoner ? summonArgs.Summoner->GetTransport() : nullptr;
    if (transport)
    {
        float x, y, z, o;
        pos.GetPosition(x, y, z, o);
        transport->CalculatePassengerOffset(x, y, z, &o);
        summon->m_movementInfo.transport.pos.Relocate(x, y, z, o);

        // This object must be added to transport before adding to map for the client to properly display it
        transport->AddPassenger(summon);
    }

    // Initialize tempsummon fields
    summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, summonArgs.SummonSpellId);
    summon->SetHomePosition(pos);
    summon->InitStats(summonArgs.SummonDuration);
    summon->SetPrivateObjectOwner(summonArgs.PrivateObjectOwner);

    // Handle health argument
    if (summonArgs.SummonHealth)
    {
        summon->SetMaxHealth(summonArgs.SummonHealth);
        summon->SetHealth(summonArgs.SummonHealth);
    }

    // Handle creature level argument
    if (summonArgs.CreatureLevel)
        summon->SetLevel(summonArgs.CreatureLevel);

    if (!AddToMap(summon->ToCreature()))
    {
        // Returning false will cause the object to be deleted - remove from transport
        if (transport)
            transport->RemovePassenger(summon);

        delete summon;
        return nullptr;
    }

    summon->InitSummon();

    // call MoveInLineOfSight for nearby creatures
    Trinity::AIRelocationNotifier notifier(*summon);
    Cell::VisitAllObjects(summon, notifier, GetVisibilityRange());

    return summon;
}

/**
* Summons group of creatures.
*
* @param group Id of group to summon.
* @param list  List to store pointers to summoned creatures.
*/

void Map::SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list /*= nullptr*/)
{
    std::vector<TempSummonData> const* data = sObjectMgr->GetSummonGroup(GetId(), SUMMONER_TYPE_MAP, group);
    if (!data)
        return;

    for (std::vector<TempSummonData>::const_iterator itr = data->begin(); itr != data->end(); ++itr)
        if (TempSummon* summon = SummonCreature(itr->entry, itr->pos, SummonCreatureExtraArgs().SetSummonDuration(itr->time)))
            if (list)
                list->push_back(summon);
}

ZoneScript* WorldObject::FindZoneScript() const
{
    if (Map* map = FindMap())
    {
        if (InstanceMap* instanceMap = map->ToInstanceMap())
            return reinterpret_cast<ZoneScript*>(instanceMap->GetInstanceScript());
        else if (!map->IsBattlegroundOrArena())
        {
            if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(map, GetZoneId()))
                return bf;
            else
                return sOutdoorPvPMgr->GetOutdoorPvPToZoneId(map, GetZoneId());
        }
    }
    return nullptr;
}

void WorldObject::SetZoneScript()
{
    m_zoneScript = FindZoneScript();
}


void WorldObject::ClearZoneScript()
{
    m_zoneScript = nullptr;
}

TempSummon* WorldObject::SummonCreature(uint32 entry, Position const& pos, TempSummonType despawnType /*= TEMPSUMMON_MANUAL_DESPAWN*/, uint32 despawnTime /*= 0*/, uint32 vehId /*= 0*/, ObjectGuid privateObjectOwner /* = ObjectGuid::Empty */)
{
    if (Map* map = FindMap())
    {
        SummonCreatureExtraArgs extraArgs;
        extraArgs.SummonDuration = despawnTime;
        extraArgs.Summoner = ToUnit();
        extraArgs.VehicleRecID = vehId;
        extraArgs.PrivateObjectOwner = privateObjectOwner;

        if (TempSummon* summon = map->SummonCreature(entry, pos, extraArgs))
        {
            summon->SetTempSummonType(despawnType);
            return summon;
        }
    }

    return nullptr;
}

TempSummon* WorldObject::SummonCreature(uint32 id, float x, float y, float z, float o /*= 0*/, TempSummonType despawnType /*= TEMPSUMMON_MANUAL_DESPAWN*/, uint32 despawnTime /*= 0*/, ObjectGuid privateObjectOwner /* = ObjectGuid::Empty */)
{
    if (!x && !y && !z)
        GetClosePoint(x, y, z, GetCombatReach());
    if (!o)
        o = GetOrientation();
    return SummonCreature(id, { x, y, z, o }, despawnType, despawnTime, 0, privateObjectOwner);
}

GameObject* WorldObject::SummonGameObject(uint32 entry, Position const& pos, QuaternionData const& rot, uint32 respawnTime, GOSummonType summonType)
{
    if (!IsInWorld())
        return nullptr;

    GameObjectTemplate const* goinfo = sObjectMgr->GetGameObjectTemplate(entry);
    if (!goinfo)
    {
        TC_LOG_ERROR("sql.sql", "Gameobject template %u not found in database!", entry);
        return nullptr;
    }

    Map* map = GetMap();
    GameObject* go = new GameObject();
    if (!go->Create(map->GenerateLowGuid<HighGuid::GameObject>(), entry, map, pos, rot, 255, GO_STATE_READY))
    {
        delete go;
        return nullptr;
    }

    PhasingHandler::InheritPhaseShift(go, this);

    go->SetRespawnTime(respawnTime);
    if (GetTypeId() == TYPEID_PLAYER || (GetTypeId() == TYPEID_UNIT && summonType == GO_SUMMON_TIMED_OR_CORPSE_DESPAWN)) //not sure how to handle this
        ToUnit()->AddGameObject(go);
    else
        go->SetSpawnedByDefault(false);

    map->AddToMap(go);
    return go;
}

GameObject* WorldObject::SummonGameObject(uint32 entry, float x, float y, float z, float ang, QuaternionData const& rot, uint32 respawnTime)
{
    if (!x && !y && !z)
    {
        GetClosePoint(x, y, z, GetCombatReach());
        ang = GetOrientation();
    }

    Position pos(x, y, z, ang);
    return SummonGameObject(entry, pos, rot, respawnTime);
}

Creature* WorldObject::SummonTrigger(float x, float y, float z, float ang, uint32 duration, CreatureAI* (*GetAI)(Creature*))
{
    TempSummonType summonType = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;
    Creature* summon = SummonCreature(WORLD_TRIGGER, x, y, z, ang, summonType, duration);
    if (!summon)
        return nullptr;

    //summon->SetName(GetName());
    if (GetTypeId() == TYPEID_PLAYER || GetTypeId() == TYPEID_UNIT)
    {
        summon->SetFaction(((Unit*)this)->GetFaction());
        summon->SetLevel(((Unit*)this)->getLevel());
    }

    if (GetAI)
        summon->AIM_Initialize(GetAI(summon));
    return summon;
}

/**
* Summons group of creatures. Should be called only by instances of Creature and GameObject classes.
*
* @param group Id of group to summon.
* @param list  List to store pointers to summoned creatures.
*/
void WorldObject::SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list /*= nullptr*/)
{
    ASSERT((GetTypeId() == TYPEID_GAMEOBJECT || GetTypeId() == TYPEID_UNIT) && "Only GOs and creatures can summon npc groups!");

    std::vector<TempSummonData> const* data = sObjectMgr->GetSummonGroup(GetEntry(), GetTypeId() == TYPEID_GAMEOBJECT ? SUMMONER_TYPE_GAMEOBJECT : SUMMONER_TYPE_CREATURE, group);
    if (!data)
    {
        TC_LOG_WARN("scripts", "%s (%s) tried to summon non-existing summon group %u.", GetName().c_str(), GetGUID().ToString().c_str(), group);
        return;
    }

    for (std::vector<TempSummonData>::const_iterator itr = data->begin(); itr != data->end(); ++itr)
        if (TempSummon* summon = SummonCreature(itr->entry, itr->pos, itr->type, itr->time))
            if (list)
                list->push_back(summon);
}

Creature* WorldObject::FindNearestCreature(uint32 entry, float range, bool alive) const
{
    Creature* creature = nullptr;
    Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck checker(*this, entry, alive, range);
    Trinity::CreatureLastSearcher<Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(this, creature, checker);
    Cell::VisitAllObjects(this, searcher, range);
    return creature;
}

GameObject* WorldObject::FindNearestGameObject(uint32 entry, float range) const
{
    GameObject* go = nullptr;
    Trinity::NearestGameObjectEntryInObjectRangeCheck checker(*this, entry, range);
    Trinity::GameObjectLastSearcher<Trinity::NearestGameObjectEntryInObjectRangeCheck> searcher(this, go, checker);
    Cell::VisitGridObjects(this, searcher, range);
    return go;
}

GameObject* WorldObject::FindNearestGameObjectOfType(GameobjectTypes type, float range) const
{
    GameObject* go = nullptr;
    Trinity::NearestGameObjectTypeInObjectRangeCheck checker(*this, type, range);
    Trinity::GameObjectLastSearcher<Trinity::NearestGameObjectTypeInObjectRangeCheck> searcher(this, go, checker);
    Cell::VisitGridObjects(this, searcher, range);
    return go;
}

Player* WorldObject::SelectNearestPlayer(float distance) const
{
    Player* target = nullptr;

    Trinity::NearestPlayerInObjectRangeCheck checker(this, distance);
    Trinity::PlayerLastSearcher<Trinity::NearestPlayerInObjectRangeCheck> searcher(this, target, checker);
    Cell::VisitWorldObjects(this, searcher, distance);

    return target;
}

ObjectGuid WorldObject::GetCharmerOrOwnerOrOwnGUID() const
{
    ObjectGuid guid = GetCharmerOrOwnerGUID();
    if (!guid.IsEmpty())
        return guid;
    return GetGUID();
}

Unit* WorldObject::GetOwner() const
{
    return ObjectAccessor::GetUnit(*this, GetOwnerGUID());
}

Unit* WorldObject::GetCharmerOrOwner() const
{
    if (Unit const* unit = ToUnit())
        return unit->GetCharmerOrOwner();
    else if (GameObject const* go = ToGameObject())
        return go->GetOwner();

    return nullptr;
}

Unit* WorldObject::GetCharmerOrOwnerOrSelf() const
{
    if (Unit* u = GetCharmerOrOwner())
        return u;

    return const_cast<WorldObject*>(this)->ToUnit();
}

Unit* WorldObject::GetCharmerOrOwnerOrCreatorOrSelf() const
{
    Unit* u = GetCharmerOrOwner();
    if (!u)
        u = const_cast<WorldObject*>(this)->ToUnit();

    if (u)
        if (Unit* creator = ObjectAccessor::GetUnit(*this, u->GetCreatorGUID()))
            u = creator;

    return u;
}

Player* WorldObject::GetCharmerOrOwnerPlayerOrPlayerItself() const
{
    ObjectGuid guid = GetCharmerOrOwnerGUID();
    if (guid.IsPlayer())
        return ObjectAccessor::GetPlayer(*this, guid);

    return const_cast<WorldObject*>(this)->ToPlayer();
}

Player* WorldObject::GetAffectingPlayer() const
{
    if (!GetCharmerOrOwnerGUID())
        return const_cast<WorldObject*>(this)->ToPlayer();

    if (Unit* owner = GetCharmerOrOwner())
        return owner->GetCharmerOrOwnerPlayerOrPlayerItself();

    return nullptr;
}

Player* WorldObject::GetSpellModOwner() const
{
    if (Player* player = const_cast<WorldObject*>(this)->ToPlayer())
        return player;

    if (GetTypeId() == TYPEID_UNIT)
    {
        Creature const* creature = ToCreature();
        if (creature->HasUnitTypeMask(UNIT_MASK_PET | UNIT_MASK_TOTEM | UNIT_MASK_GUARDIAN | UNIT_MASK_HUNTER_PET | UNIT_MASK_MINION))
        {
            if (Unit* owner = creature->GetOwner())
                return owner->ToPlayer();
        }
    }
    else if (GetTypeId() == TYPEID_GAMEOBJECT)
    {
        GameObject const* go = ToGameObject();
        if (Unit* owner = go->GetOwner())
            return owner->ToPlayer();
    }

    return nullptr;
}

// function uses real base points (typically value - 1)
int32 WorldObject::CalculateSpellDamage(Unit const* target, SpellInfo const* spellProto, uint8 effectIndex, int32 const* basePoints) const
{
    return spellProto->Effects[effectIndex].CalcValue(this, basePoints, target);
}

float WorldObject::GetSpellMaxRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const
{
    if (!spellInfo->RangeEntry)
        return 0;
    if (spellInfo->RangeEntry->RangeMax[1] == spellInfo->RangeEntry->RangeMax[0])
        return spellInfo->GetMaxRange();
    if (!target)
        return spellInfo->GetMaxRange(true);
    return spellInfo->GetMaxRange(!IsHostileTo(target));
}

float WorldObject::GetSpellMinRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const
{
    if (!spellInfo->RangeEntry)
        return 0;
    if (spellInfo->RangeEntry->RangeMin[1] == spellInfo->RangeEntry->RangeMin[0])
        return spellInfo->GetMinRange();
    if (!target)
        return spellInfo->GetMinRange(true);
    return spellInfo->GetMinRange(!IsHostileTo(target));
}

float WorldObject::ApplyEffectModifiers(SpellInfo const* spellProto, uint8 effect_index, float value) const
{
    if (Player* modOwner = GetSpellModOwner())
    {
        modOwner->ApplySpellMod(spellProto, SpellModOp::Points, value);
        switch (effect_index)
        {
            case EFFECT_0:
                modOwner->ApplySpellMod(spellProto, SpellModOp::PointsIndex0, value);
                break;
            case EFFECT_1:
                modOwner->ApplySpellMod(spellProto, SpellModOp::PointsIndex1, value);
                break;
            case EFFECT_2:
                modOwner->ApplySpellMod(spellProto, SpellModOp::PointsIndex2, value);
                break;
            default:
                break;
        }
    }
    return value;
}

int32 WorldObject::CalcSpellDuration(SpellInfo const* spellInfo) const
{
    uint8 comboPoints = 0;
    if (Unit const* unit = ToUnit())
        comboPoints = unit->GetComboPoints();

    int32 minduration = spellInfo->GetDuration();
    int32 maxduration = spellInfo->GetMaxDuration();

    int32 duration = 0;
    if (spellInfo->HasAttribute(SPELL_ATTR1_FINISHING_MOVE_DURATION) && comboPoints && minduration != -1 && minduration != maxduration)
        duration = minduration + int32((maxduration - minduration) * comboPoints / 5);
    else
        duration = minduration;

    return duration;
}

int32 WorldObject::ModSpellDuration(SpellInfo const* spellInfo, WorldObject const* target, int32 duration, bool positive, uint32 effectMask)
{
    // don't mod permanent auras duration
    if (duration < 0)
        return duration;

    // some auras are not affected by duration modifiers
    if (spellInfo->HasAttribute(SPELL_ATTR7_NO_TARGET_DURATION_MOD))
        return duration;

    // cut duration only of negative effects
    Unit const* unitTarget = target->ToUnit();
    if (!unitTarget)
        return duration;

    // cut duration only of negative effects
    if (!positive)
    {
        int32 mechanicMask = spellInfo->GetSpellMechanicMaskByEffectMask(effectMask);
        auto mechanicCheck = [mechanicMask](AuraEffect const* aurEff) -> bool
        {
            if (mechanicMask & (1 << aurEff->GetMiscValue()))
                return true;
            return false;
        };

        // Find total mod value (negative bonus)
        int32 durationMod_always = unitTarget->GetTotalAuraModifier(SPELL_AURA_MECHANIC_DURATION_MOD, mechanicCheck);
        // Find max mod (negative bonus)
        int32 durationMod_not_stack = unitTarget->GetMaxNegativeAuraModifier(SPELL_AURA_MECHANIC_DURATION_MOD_NOT_STACK, mechanicCheck);

        // Select strongest negative mod
        int32 durationMod = std::min(durationMod_always, durationMod_not_stack);
        if (durationMod != 0)
            AddPct(duration, durationMod);

        // there are only negative mods currently
        durationMod_always = unitTarget->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_AURA_DURATION_BY_DISPEL, spellInfo->Dispel);
        durationMod_not_stack = unitTarget->GetMaxNegativeAuraModifierByMiscValue(SPELL_AURA_MOD_AURA_DURATION_BY_DISPEL_NOT_STACK, spellInfo->Dispel);

        durationMod = std::min(durationMod_always, durationMod_not_stack);
        if (durationMod != 0)
            AddPct(duration, durationMod);

    }
    else
    {
        // else positive mods here, there are no currently
        // when there will be, change GetTotalAuraModifierByMiscValue to GetMaxPositiveAuraModifierByMiscValue

        // Mixology - duration boost
        if (unitTarget->GetTypeId() == TYPEID_PLAYER)
        {
            if (spellInfo->SpellFamilyName == SPELLFAMILY_POTION && (
                sSpellMgr->IsSpellMemberOfSpellGroup(spellInfo->Id, SPELL_GROUP_ELIXIR_BATTLE) ||
                sSpellMgr->IsSpellMemberOfSpellGroup(spellInfo->Id, SPELL_GROUP_ELIXIR_GUARDIAN)))
            {
                if (unitTarget->HasAura(53042) && unitTarget->HasSpell(spellInfo->Effects[0].TriggerSpell))
                    duration *= 2;
            }
        }
    }
    return std::max(duration, 0);
}

void WorldObject::ModSpellCastTime(SpellInfo const* spellInfo, int32& castTime, Spell* spell)
{
    if (!spellInfo || castTime < 0)
        return;

    // called from caster
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellInfo, SpellModOp::ChangeCastTime, castTime, spell);

    Unit const* unitCaster = ToUnit();
    if (!unitCaster)
        return;

    if (unitCaster->IsPlayer() && unitCaster->ToPlayer()->GetCommandStatus(CHEAT_CASTTIME))
        castTime = 0;
    if (!(spellInfo->HasAttribute(SPELL_ATTR0_IS_ABILITY) || spellInfo->HasAttribute(SPELL_ATTR0_IS_TRADESKILL) || spellInfo->HasAttribute(SPELL_ATTR3_IGNORE_CASTER_MODIFIERS)) &&
        ((GetTypeId() == TYPEID_PLAYER && spellInfo->SpellFamilyName) || GetTypeId() == TYPEID_UNIT))
        castTime = int32(float(castTime) * GetFloatValue(UNIT_MOD_CAST_SPEED));
    else if (spellInfo->HasAttribute(SPELL_ATTR0_USES_RANGED_SLOT) && !spellInfo->HasAttribute(SPELL_ATTR2_AUTO_REPEAT))
        castTime = int32(float(castTime) * unitCaster->m_modAttackSpeedPct[RANGED_ATTACK]);
    else if (spellInfo->SpellVisual[0] == 3881 && unitCaster->HasAura(67556)) // cooking with Chef Hat.
        castTime = 500;
}

float WorldObject::MeleeSpellMissChance(Unit const* /*victim*/, WeaponAttackType /*attType*/, SpellInfo const* /*spellInfo*/) const
{
    return 0.0f;
}

SpellMissInfo WorldObject::MeleeSpellHitResult(Unit* /*victim*/, SpellInfo const* /*spellInfo*/) const
{
    return SPELL_MISS_NONE;
}

SpellMissInfo WorldObject::MagicSpellHitResult(Unit* victim, SpellInfo const* spellInfo) const
{
    // Can`t miss on dead target (on skinning for example)
    if (!victim->IsAlive() && !victim->IsPlayer())
        return SPELL_MISS_NONE;

    if (spellInfo->HasAttribute(SPELL_ATTR3_NO_AVOIDANCE))
        return SPELL_MISS_NONE;

    if (spellInfo->HasAttribute(SPELL_ATTR7_NO_ATTACK_MISS))
        return SPELL_MISS_NONE;

    SpellSchoolMask schoolMask = spellInfo->GetSchoolMask();

    // PvP - PvE spell misschances per leveldif > 2
    int32 lchance = victim->IsPlayer() ? 7 : 11;
    int32 thisLevel = getLevelForTarget(victim);
    if (GetTypeId() == TYPEID_UNIT && ToCreature()->IsTrigger())
        thisLevel = std::max<int32>(thisLevel, spellInfo->SpellLevel);
    int32 leveldif = int32(victim->getLevelForTarget(this)) - thisLevel;
    int32 levelBasedHitDiff = leveldif;

    // Base hit chance from attacker and victim levels
    int32 modHitChance = 100;
    if (levelBasedHitDiff >= 0)
    {
        if (victim->IsPlayer())
        {
            modHitChance = 94 - 3 * std::min(levelBasedHitDiff, 3);
            levelBasedHitDiff -= 3;
        }
        else
        {
            modHitChance = 96 - std::min(levelBasedHitDiff, 2);
            levelBasedHitDiff -= 2;
        }
        if (levelBasedHitDiff > 0)
            modHitChance -= lchance * std::min(levelBasedHitDiff, 7);
    }
    else
        modHitChance = 97 - levelBasedHitDiff;

    // Spellmod from SpellModOp::HitChance
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellInfo, SpellModOp::HitChance, modHitChance);

    // Spells with SPELL_ATTR3_ALWAYS_HIT will ignore target's avoidance effects
    if (!spellInfo->HasAttribute(SPELL_ATTR3_ALWAYS_HIT))
    {
        // Chance hit from victim SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE auras
        modHitChance += victim->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE, schoolMask);
    }
    // Decrease hit chance from victim rating bonus
    if (victim->IsPlayer())
        modHitChance -= int32(victim->ToPlayer()->GetRatingBonusValue(CR_HIT_TAKEN_SPELL));

    int32 hitChance = modHitChance * 100;
    // Increase hit chance from attacker SPELL_AURA_MOD_SPELL_HIT_CHANCE and attacker ratings
        // Increase hit chance from attacker SPELL_AURA_MOD_SPELL_HIT_CHANCE and attacker ratings
    if (Unit const* unit = ToUnit())
        hitChance += int32(unit->m_modSpellHitChance * 100.0f);

    RoundToInterval(hitChance, 0, 10000);

    int32 tmp = 10000 - hitChance;

    int32 rand = irand(0, 9999);
    if (tmp > 0 && rand < tmp)
        return SPELL_MISS_MISS;

    // Chance resist mechanic (select max value from every mechanic spell effect)
    int32 resist_chance = victim->GetMechanicResistChance(spellInfo) * 100;

    // Roll chance
    if (resist_chance > 0 && rand < (tmp += resist_chance))
        return SPELL_MISS_RESIST;

    // cast by caster in front of victim
    if (!victim->HasUnitState(UNIT_STATE_CONTROLLED) && (victim->HasInArc(float(M_PI), this) || victim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION)))
    {
        int32 deflect_chance = victim->GetTotalAuraModifier(SPELL_AURA_DEFLECT_SPELLS) * 100;
        if (deflect_chance > 0 && rand < (tmp += deflect_chance))
            return SPELL_MISS_DEFLECT;
    }

    return SPELL_MISS_NONE;
}

// Calculate spell hit result can be:
// Every spell can: Evade/Immune/Reflect/Sucesful hit
// For melee based spells:
//   Miss
//   Dodge
//   Parry
// For spells
//   Resist
SpellMissInfo WorldObject::SpellHitResult(Unit* victim, SpellInfo const* spellInfo, bool canReflect /*= false*/) const
{
    // Check for immune
    if (victim->IsImmunedToSpell(spellInfo, this))
        return SPELL_MISS_IMMUNE;

    // Damage immunity is only checked if the spell has damage effects, this immunity must not prevent aura apply
    // returns SPELL_MISS_IMMUNE in that case, for other spells, the SMSG_SPELL_GO must show hit
    if (spellInfo->HasOnlyDamageEffects() && victim->IsImmunedToDamage(spellInfo))
        return SPELL_MISS_IMMUNE;

    // All positive spells can`t miss
    /// @todo client not show miss log for this spells - so need find info for this in dbc and use it!
    if (spellInfo->IsPositive() && !IsHostileTo(victim)) // prevent from affecting enemy by "positive" spell
        return SPELL_MISS_NONE;

    if (this == victim)
        return SPELL_MISS_NONE;

    // Return evade for units in evade mode
    if (victim->GetTypeId() == TYPEID_UNIT && victim->ToCreature()->IsEvadingAttacks())
        return SPELL_MISS_EVADE;

    // Try victim reflect spell
    if (canReflect)
    {
        int32 reflectchance = victim->GetTotalAuraModifier(SPELL_AURA_REFLECT_SPELLS);
        reflectchance += victim->GetTotalAuraModifierByMiscMask(SPELL_AURA_REFLECT_SPELLS_SCHOOL, spellInfo->GetSchoolMask());

        if (reflectchance > 0 && roll_chance_i(reflectchance))
            return spellInfo->HasAttribute(SPELL_ATTR7_REFLECTION_ONLY_DEFENDS) ? SPELL_MISS_DEFLECT : SPELL_MISS_REFLECT;
    }

    if (spellInfo->HasAttribute(SPELL_ATTR3_ALWAYS_HIT))
        return SPELL_MISS_NONE;

    switch (spellInfo->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_RANGED:
        case SPELL_DAMAGE_CLASS_MELEE:
            return MeleeSpellHitResult(victim, spellInfo);
        case SPELL_DAMAGE_CLASS_NONE:
            return SPELL_MISS_NONE;
        case SPELL_DAMAGE_CLASS_MAGIC:
            return MagicSpellHitResult(victim, spellInfo);
    }
    return SPELL_MISS_NONE;
}

void WorldObject::SendSpellMiss(Unit* target, uint32 spellID, SpellMissInfo missInfo)
{
    WorldPackets::CombatLog::SpellMissLog spellMissLog;
    spellMissLog.SpellID = spellID;
    spellMissLog.Caster = GetGUID();
    spellMissLog.Entries.emplace_back(target->GetGUID(), missInfo);
    SendMessageToSet(spellMissLog.Write(), true);
}

FactionTemplateEntry const* WorldObject::GetFactionTemplateEntry() const
{
    uint32 factionId = GetFaction();
    FactionTemplateEntry const* entry = sFactionTemplateStore.LookupEntry(factionId);
    if (!entry)
    {
        switch (GetTypeId())
        {
            case TYPEID_PLAYER:
                TC_LOG_ERROR("entities.unit", "Player %s has invalid faction (faction template id) #%u", ToPlayer()->GetName().c_str(), factionId);
                break;
            case TYPEID_UNIT:
                TC_LOG_ERROR("entities.unit", "Creature (template id: %u) has invalid faction (faction template Id) #%u", ToCreature()->GetCreatureTemplate()->Entry, factionId);
                break;
            case TYPEID_GAMEOBJECT:
                if (factionId) // Gameobjects may have faction template id = 0
                    TC_LOG_ERROR("entities.faction", "GameObject (template id: %u) has invalid faction (faction template Id) #%u", ToGameObject()->GetGOInfo()->entry, factionId);
                break;
            default:
                TC_LOG_ERROR("entities.unit", "Object (name=%s, type=%u) has invalid faction (faction template Id) #%u", GetName().c_str(), uint32(GetTypeId()), factionId);
                break;
        }
    }

    return entry;
}

// function based on function Unit::UnitReaction from 13850 client
ReputationRank WorldObject::GetReactionTo(WorldObject const* target) const
{
    // always friendly to self
    if (this == target)
        return REP_FRIENDLY;

    // always friendly to charmer or owner
    if (GetCharmerOrOwnerOrSelf() == target->GetCharmerOrOwnerOrSelf())
        return REP_FRIENDLY;

    Player const* selfPlayerOwner = GetAffectingPlayer();
    Player const* targetPlayerOwner = target->GetAffectingPlayer();

    // check forced reputation to support SPELL_AURA_FORCE_REACTION
    if (selfPlayerOwner)
    {
        if (FactionTemplateEntry const* targetFactionTemplateEntry = target->GetFactionTemplateEntry())
            if (ReputationRank const* repRank = selfPlayerOwner->GetReputationMgr().GetForcedRankIfAny(targetFactionTemplateEntry))
                return *repRank;
    }
    else if (targetPlayerOwner)
    {
        if (FactionTemplateEntry const* selfFactionTemplateEntry = GetFactionTemplateEntry())
            if (ReputationRank const* repRank = targetPlayerOwner->GetReputationMgr().GetForcedRankIfAny(selfFactionTemplateEntry))
                return *repRank;
    }

    Unit const* unit = Coalesce<const Unit>(ToUnit(), selfPlayerOwner);
    Unit const* targetUnit = Coalesce<const Unit>(target->ToUnit(), targetPlayerOwner);
    if (unit && unit->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
    {
        if (targetUnit && targetUnit->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
        {
            if (selfPlayerOwner && targetPlayerOwner)
            {
                // always friendly to other unit controlled by player, or to the player himself
                if (selfPlayerOwner == targetPlayerOwner)
                    return REP_FRIENDLY;

                // duel - always hostile to opponent
                if (selfPlayerOwner->duel && selfPlayerOwner->duel->opponent == targetPlayerOwner && selfPlayerOwner->duel->startTime != 0)
                    return REP_HOSTILE;

                // same group - checks dependant only on our faction - skip FFA_PVP for example
                if (selfPlayerOwner->IsInRaidWith(targetPlayerOwner))
                    return REP_FRIENDLY; // return true to allow config option AllowTwoSide.Interaction.Group to work
                // however client seems to allow mixed group parties, because in 13850 client it works like:
                // return GetFactionReactionTo(GetFactionTemplateEntry(), target);
            }

            // check FFA_PVP
            if (unit->IsFFAPvP() && targetUnit->IsFFAPvP())
                return REP_HOSTILE;

            if (selfPlayerOwner)
            {
                if (FactionTemplateEntry const* targetFactionTemplateEntry = targetUnit->GetFactionTemplateEntry())
                {
                    if (ReputationRank const* repRank = selfPlayerOwner->GetReputationMgr().GetForcedRankIfAny(targetFactionTemplateEntry))
                        return *repRank;
                    if (!selfPlayerOwner->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_IGNORE_REPUTATION))
                    {
                        if (FactionEntry const* targetFactionEntry = sFactionStore.LookupEntry(targetFactionTemplateEntry->Faction))
                        {
                            if (targetFactionEntry->CanHaveReputation())
                            {
                                // check contested flags
                                if (targetFactionTemplateEntry->IsHostileToPvpActivePlayers()
                                    && selfPlayerOwner->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP))
                                    return REP_HOSTILE;

                                // if faction has reputation, hostile state depends only from AtWar state
                                if (selfPlayerOwner->GetReputationMgr().IsAtWar(targetFactionEntry))
                                    return REP_HOSTILE;
                                return REP_FRIENDLY;
                            }
                        }
                    }
                }
            }
        }
    }

    // do checks dependant only on our faction
    return WorldObject::GetFactionReactionTo(GetFactionTemplateEntry(), target);
}


/*static*/ ReputationRank WorldObject::GetFactionReactionTo(FactionTemplateEntry const* factionTemplateEntry, WorldObject const* target)
{
    // always neutral when no template entry found
    if (!factionTemplateEntry)
        return REP_NEUTRAL;

    FactionTemplateEntry const* targetFactionTemplateEntry = target->GetFactionTemplateEntry();
    if (!targetFactionTemplateEntry)
        return REP_NEUTRAL;

    if (Player const* targetPlayerOwner = target->GetAffectingPlayer())
    {
        // check contested flags
        if (factionTemplateEntry->IsHostileToPvpActivePlayers()
            && targetPlayerOwner->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP))
            return REP_HOSTILE;
        if (ReputationRank const* repRank = targetPlayerOwner->GetReputationMgr().GetForcedRankIfAny(factionTemplateEntry))
            return *repRank;
        if (target->IsUnit() && !target->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_IGNORE_REPUTATION))
        {
            if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionTemplateEntry->Faction))
            {
                if (factionEntry->CanHaveReputation())
                {
                    // CvP case - check reputation, don't allow state higher than neutral when at war
                    ReputationRank repRank = targetPlayerOwner->GetReputationMgr().GetRank(factionEntry);
                    if (targetPlayerOwner->GetReputationMgr().IsAtWar(factionEntry))
                        repRank = std::min(REP_NEUTRAL, repRank);
                    return repRank;
                }
            }
        }
    }

    // common faction based check
    if (factionTemplateEntry->IsHostileTo(targetFactionTemplateEntry))
        return REP_HOSTILE;
    if (factionTemplateEntry->IsFriendlyTo(targetFactionTemplateEntry))
        return REP_FRIENDLY;
    if (targetFactionTemplateEntry->IsFriendlyTo(factionTemplateEntry))
        return REP_FRIENDLY;
    // neutral by default
    return REP_NEUTRAL;
}

bool WorldObject::IsHostileTo(WorldObject const* target) const
{
    return GetReactionTo(target) <= REP_HOSTILE;
}

bool WorldObject::IsFriendlyTo(WorldObject const* target) const
{
    return GetReactionTo(target) >= REP_FRIENDLY;
}

bool WorldObject::IsHostileToPlayers() const
{
    FactionTemplateEntry const* my_faction = GetFactionTemplateEntry();
    if (!my_faction->Faction)
        return false;

    FactionEntry const* raw_faction = sFactionStore.LookupEntry(my_faction->Faction);
    if (raw_faction && raw_faction->ReputationIndex >= 0)
        return false;

    return my_faction->IsHostileToPlayers();
}

bool WorldObject::IsNeutralToAll() const
{
    FactionTemplateEntry const* my_faction = GetFactionTemplateEntry();
    if (!my_faction->Faction)
        return true;

    FactionEntry const* raw_faction = sFactionStore.LookupEntry(my_faction->Faction);
    if (raw_faction && raw_faction->ReputationIndex >= 0)
        return false;

    return my_faction->IsNeutralToAll();
}

SpellCastResult WorldObject::CastSpell(SpellCastTargets const& targets, uint32 spellId, CastSpellExtraArgs const& args /*= { }*/)
{
    SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
    if (!info)
    {
        TC_LOG_ERROR("entities.unit", "CastSpell: unknown spell %u by caster %s", spellId, GetGUID().ToString().c_str());
        return SPELL_FAILED_SPELL_UNAVAILABLE;
    }

    Spell* spell = new Spell(this, info, args.TriggerFlags, args.OriginalCaster);
    for (auto const& pair : args.SpellValueOverrides)
        spell->SetSpellValue(pair.first, pair.second);

    spell->m_CastItem = args.CastItem;

    if (!spell->m_CastItem && info->HasAttribute(SPELL_ATTR2_RETAIN_ITEM_CAST))
    {
        if (args.TriggeringSpell)
            spell->m_CastItem = args.TriggeringSpell->m_CastItem;
        else if (args.TriggeringAura && !args.TriggeringAura->GetBase()->GetCastItemGUID().IsEmpty())
            if (Unit const* auraCaster = args.TriggeringAura->GetCaster())
                if (Player const* triggeringAuraCaster = auraCaster->ToPlayer())
                    spell->m_CastItem = triggeringAuraCaster->GetItemByGuid(args.TriggeringAura->GetBase()->GetCastItemGUID());
    }

    spell->m_customArg = args.CustomArg;

    return spell->prepare(targets, args.TriggeringAura);
}

SpellCastResult WorldObject::CastSpell(WorldObject* target, uint32 spellId, CastSpellExtraArgs const& args /*= { }*/)
{
    SpellCastTargets targets;
    if (target)
    {
        if (Unit* unitTarget = target->ToUnit())
            targets.SetUnitTarget(unitTarget);
        else if (GameObject* goTarget = target->ToGameObject())
            targets.SetGOTarget(goTarget);
        else
        {
            TC_LOG_ERROR("entities.unit", "CastSpell: Invalid target %s passed to spell cast by %s", target->GetGUID().ToString().c_str(), GetGUID().ToString().c_str());
            return SPELL_FAILED_BAD_TARGETS;
        }
    }
    return CastSpell(targets, spellId, args);
}

SpellCastResult WorldObject::CastSpell(Position const& dest, uint32 spellId, CastSpellExtraArgs const& args /*= { }*/)
{
    SpellCastTargets targets;
    targets.SetDst(dest);
    return CastSpell(targets, spellId, args);
}

// function based on function Unit::CanAttack from 13850 client
bool WorldObject::IsValidAttackTarget(WorldObject const* target, SpellInfo const* bySpell /*= nullptr*/) const
{
    ASSERT(target);

    // some positive spells can be casted at hostile target
    bool isPositiveSpell = bySpell && bySpell->IsPositive();

    // can't attack self (spells can, attribute check)
    if (!bySpell && this == target)
        return false;

    // can't attack unattackable units
    Unit const* unitTarget = target->ToUnit();
    if (unitTarget && unitTarget->HasUnitState(UNIT_STATE_UNATTACKABLE))
        return false;

    // can't attack GMs
    if (target->GetTypeId() == TYPEID_PLAYER && target->ToPlayer()->IsGameMaster())
        return false;

    Unit const* unit = ToUnit();
    // visibility checks (only units)
    if (unit)
    {
        // can't attack invisible
        if (!bySpell || !bySpell->HasAttribute(SPELL_ATTR6_IGNORE_PHASE_SHIFT))
        {
            if (!unit->CanSeeOrDetect(target, bySpell && bySpell->IsAffectingArea()))
                return false;
        }
    }

    // can't attack dead
    if ((!bySpell || !bySpell->IsAllowingDeadTarget()) && unitTarget && !unitTarget->IsAlive())
        return false;

    // can't attack untargetable
    if ((!bySpell || !bySpell->HasAttribute(SPELL_ATTR6_CAN_TARGET_UNTARGETABLE)) && unitTarget && unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
        return false;

    if (Player const* playerAttacker = ToPlayer())
    {
        if (playerAttacker->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_UBER))
            return false;
    }

    // check flags
    if (unitTarget && unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_ON_TAXI | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_NON_ATTACKABLE_2))
        return false;

    Unit const* unitOrOwner = unit;
    GameObject const* go = ToGameObject();
    if (go && go->GetGoType() == GAMEOBJECT_TYPE_TRAP)
        unitOrOwner = go->GetOwner();

    // ignore immunity flags when assisting
    if (unitOrOwner && unitTarget && !(isPositiveSpell && bySpell->HasAttribute(SPELL_ATTR6_CAN_ASSIST_IMMUNE_PC)))
    {
        if (!unitOrOwner->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED) && unitTarget->IsImmuneToNPC())
            return false;

        if (!unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED) && unitOrOwner->IsImmuneToNPC())
            return false;

        if (!bySpell || !bySpell->HasAttribute(SPELL_ATTR8_ATTACK_IGNORE_IMMUNE_TO_PC_FLAG))
        {
            if (unitOrOwner->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED) && unitTarget->IsImmuneToPC())
                return false;

            if (unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED) && unitOrOwner->IsImmuneToPC())
                return false;
        }
    }

    // CvC case - can attack each other only when one of them is hostile
    if (unit && !unit->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED) && unitTarget && !unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
        return IsHostileTo(unitTarget) || unitTarget->IsHostileTo(this);

    // Traps without owner or with NPC owner versus Creature case - can attack to creature only when one of them is hostile
    if (go && go->GetGoType() == GAMEOBJECT_TYPE_TRAP)
    {
        Unit const* goOwner = go->GetOwner();
        if (!goOwner || !goOwner->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
            if (unitTarget && !unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
                return IsHostileTo(unitTarget) || unitTarget->IsHostileTo(this);
    }

    // PvP, PvC, CvP case
    // can't attack friendly targets
    if (IsFriendlyTo(target) || target->IsFriendlyTo(this))
        return false;

    Player const* playerAffectingAttacker = unit && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED) ? GetAffectingPlayer() : go ? GetAffectingPlayer() : nullptr;
    Player const* playerAffectingTarget = unitTarget && unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED) ? unitTarget->GetAffectingPlayer() : nullptr;

    // Not all neutral creatures can be attacked (even some unfriendly faction does not react aggresive to you, like Sporaggar)
    if ((playerAffectingAttacker && !playerAffectingTarget) || (!playerAffectingAttacker && playerAffectingTarget))
    {
        Player const* player = playerAffectingAttacker ? playerAffectingAttacker : playerAffectingTarget;

        if (Unit const* creature = playerAffectingAttacker ? unitTarget : unit)
        {
            if (creature->IsContestedGuard() && player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP))
                return true;

            if (FactionTemplateEntry const* factionTemplate = creature->GetFactionTemplateEntry())
            {
                if (!(player->GetReputationMgr().GetForcedRankIfAny(factionTemplate)))
                    if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionTemplate->Faction))
                        if (FactionState const* repState = player->GetReputationMgr().GetState(factionEntry))
                            if (!(repState->Flags & FACTION_FLAG_AT_WAR))
                                return false;
            }
        }
    }

    if (playerAffectingAttacker && playerAffectingTarget)
        if (playerAffectingAttacker->duel && playerAffectingAttacker->duel->opponent == playerAffectingTarget && playerAffectingAttacker->duel->startTime != 0)
            return true;

    // PvP case - can't attack when attacker or target are in sanctuary
    // however, 13850 client doesn't allow to attack when one of the unit's has sanctuary flag and is pvp
    if (unitTarget && unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED)
        && unitOrOwner && unitOrOwner->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED)
        && (unitTarget->IsInSanctuary() || unitOrOwner->IsInSanctuary())
        && (!bySpell || bySpell->HasAttribute(SPELL_ATTR8_IGNORE_SANCTUARY)))
        return false;

    // additional checks - only PvP case
    if (playerAffectingAttacker && playerAffectingTarget)
    {
        if (playerAffectingTarget->IsPvP() || (bySpell && bySpell->HasAttribute(SPELL_ATTR5_IGNORE_AREA_EFFECT_PVP_CHECK)))
            return true;

        if (playerAffectingAttacker->IsFFAPvP() && playerAffectingTarget->IsFFAPvP())
            return true;

        return playerAffectingAttacker->HasByteFlag(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_PVP_FLAG, UNIT_BYTE2_FLAG_UNK1)
            || playerAffectingTarget->HasByteFlag(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_PVP_FLAG, UNIT_BYTE2_FLAG_UNK1);
    }

    return true;
}

// function based on function Unit::CanAssist from 13850 client
bool WorldObject::IsValidAssistTarget(WorldObject const* target, SpellInfo const* bySpell /*= nullptr*/) const
{
    ASSERT(target);

    // some negative spells can be casted at friendly target
    bool isNegativeSpell = bySpell && !bySpell->IsPositive();

    // can assist to self
    if (this == target)
        return true;

    // can't assist unattackable units
    Unit const* unitTarget = target->ToUnit();
    if (unitTarget && unitTarget->HasUnitState(UNIT_STATE_UNATTACKABLE))
        return false;

    // can't assist GMs
    if (target->GetTypeId() == TYPEID_PLAYER && target->ToPlayer()->IsGameMaster())
        return false;

    // can't assist own vehicle or passenger
    Unit const* unit = ToUnit();
    if (unit && unitTarget && unit->GetVehicle())
    {
        if (unit->IsOnVehicle(unitTarget))
            return false;

        if (unit->GetVehicleBase()->IsOnVehicle(unitTarget))
            return false;
    }

    // can't assist invisible
    if ((!bySpell || !bySpell->HasAttribute(SPELL_ATTR6_IGNORE_PHASE_SHIFT)) && !CanSeeOrDetect(target, bySpell && bySpell->IsAffectingArea()))
        return false;

    // can't assist dead
    if ((!bySpell || !bySpell->IsAllowingDeadTarget()) && unitTarget && !unitTarget->IsAlive())
        return false;

    // can't assist untargetable
    if ((!bySpell || !bySpell->HasAttribute(SPELL_ATTR6_CAN_TARGET_UNTARGETABLE)) && unitTarget && unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
        return false;

    // check flags for negative spells
    if (isNegativeSpell && unitTarget && unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_ON_TAXI | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_NON_ATTACKABLE_2))
        return false;

    if (isNegativeSpell || !bySpell || !bySpell->HasAttribute(SPELL_ATTR6_CAN_ASSIST_IMMUNE_PC))
    {
        if (unit && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
        {
            if (unitTarget && unitTarget->IsImmuneToPC())
                return false;
        }
        else
        {
            if (unitTarget && unitTarget->IsImmuneToNPC())
                return false;
        }
    }

    // can't assist non-friendly targets
    if (GetReactionTo(target) < REP_NEUTRAL && target->GetReactionTo(this) < REP_NEUTRAL && (!ToCreature() || !ToCreature()->HasStaticFlag(CREATURE_STATIC_FLAG_4_TREAT_AS_RAID_UNIT_FOR_HELPFUL_SPELLS)))
        return false;

    // PvP case
    if (unitTarget && unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
    {
        if (unit && unit->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
        {
            Player const* selfPlayerOwner = GetAffectingPlayer();
            Player const* targetPlayerOwner = unitTarget->GetAffectingPlayer();
            if (selfPlayerOwner && targetPlayerOwner)
            {
                // can't assist player which is dueling someone
                if (selfPlayerOwner != targetPlayerOwner && targetPlayerOwner->duel)
                    return false;
            }
            // can't assist player in ffa_pvp zone from outside
            if (unitTarget->IsFFAPvP() && !unit->IsFFAPvP())
                return false;

            // can't assist player out of sanctuary from sanctuary if has pvp enabled
            if (unitTarget->IsPvP() && (!bySpell || bySpell->HasAttribute(SPELL_ATTR8_IGNORE_SANCTUARY)))
                if (unit->IsInSanctuary() && !unitTarget->IsInSanctuary())
                    return false;
        }
    }
    // PvC case - player can assist creature only if has specific type flags
    // !target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE) &&
    else if (unit && unit->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
    {
        if (!bySpell || !bySpell->HasAttribute(SPELL_ATTR6_CAN_ASSIST_IMMUNE_PC))
            if (unitTarget && !unitTarget->IsPvP())
                if (Creature const* creatureTarget = target->ToCreature())
                    return creatureTarget->HasStaticFlag(CREATURE_STATIC_FLAG_4_TREAT_AS_RAID_UNIT_FOR_HELPFUL_SPELLS) || (creatureTarget->GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_CAN_ASSIST);
    }

    return true;
}

Unit* WorldObject::GetMagicHitRedirectTarget(Unit* victim, SpellInfo const* spellInfo)
{
    // Patch 1.2 notes: Spell Reflection no longer reflects abilities
    if (spellInfo->HasAttribute(SPELL_ATTR0_IS_ABILITY) || spellInfo->HasAttribute(SPELL_ATTR1_NO_REDIRECTION) || spellInfo->HasAttribute(SPELL_ATTR0_NO_IMMUNITIES))
        return victim;

    Unit::AuraEffectList const& magnetAuras = victim->GetAuraEffectsByType(SPELL_AURA_SPELL_MAGNET);
    for (AuraEffect const* aurEff : magnetAuras)
    {
        if (Unit* magnet = aurEff->GetBase()->GetCaster())
        {
            if (spellInfo->CheckExplicitTarget(this, magnet) == SPELL_CAST_OK && IsValidAttackTarget(magnet, spellInfo))
            {
                /// @todo handle this charge drop by proc in cast phase on explicit target
                if (spellInfo->Speed > 0.0f)
                {
                    // Set up missile speed based delay
                    uint32 delay = uint32(std::floor(std::max<float>(victim->GetDistance(this), 5.0f) / spellInfo->Speed * 1000.0f));
                    // Schedule charge drop
                    aurEff->GetBase()->DropChargeDelayed(delay, AuraRemoveFlags::Expired);
                }
                else
                    aurEff->GetBase()->DropCharge(AuraRemoveFlags::Expired);

                return magnet;
            }
        }
    }
    return victim;
}

template <typename Container>
void WorldObject::GetGameObjectListWithEntryInGrid(Container& gameObjectContainer, uint32 entry, float maxSearchRange /*= 250.0f*/) const
{
    Trinity::AllGameObjectsWithEntryInRange check(this, entry, maxSearchRange);
    Trinity::GameObjectListSearcher<Trinity::AllGameObjectsWithEntryInRange> searcher(this, gameObjectContainer, check);
    Cell::VisitGridObjects(this, searcher, maxSearchRange);
}

template <typename Container>
void WorldObject::GetCreatureListWithEntryInGrid(Container& creatureContainer, uint32 entry, float maxSearchRange /*= 250.0f*/) const
{
    Trinity::AllCreaturesOfEntryInRange check(this, entry, maxSearchRange);
    Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(this, creatureContainer, check);
    Cell::VisitGridObjects(this, searcher, maxSearchRange);
}

template <typename Container>
void WorldObject::GetPlayerListInGrid(Container& playerContainer, float maxSearchRange) const
{
    Trinity::AnyPlayerInObjectRangeCheck checker(this, maxSearchRange);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(this, playerContainer, checker);
    Cell::VisitWorldObjects(this, searcher, maxSearchRange);
}

void WorldObject::GetNearPoint2D(WorldObject const* searcher, float &x, float &y, float distance2d, float absAngle) const
{
    float effectiveReach = GetCombatReach();

    if (searcher)
    {
        effectiveReach += searcher->GetCombatReach();

        if (this != searcher)
        {
            float myHover = 0.0f, searcherHover = 0.0f;
            if (Unit const* unit = ToUnit())
                myHover = unit->GetHoverOffset();
            if (Unit const* searchUnit = searcher->ToUnit())
                searcherHover = searchUnit->GetHoverOffset();

            float hoverDelta = myHover - searcherHover;
            if (hoverDelta != 0.0f)
                effectiveReach = std::sqrt(std::max(effectiveReach * effectiveReach - hoverDelta * hoverDelta, 0.0f));
        }
    }

    x = GetPositionX() + (effectiveReach + distance2d) * std::cos(absAngle);
    y = GetPositionY() + (effectiveReach + distance2d) * std::sin(absAngle);

    Trinity::NormalizeMapCoord(x);
    Trinity::NormalizeMapCoord(y);
}

void WorldObject::GetNearPoint(WorldObject const* searcher, float &x, float &y, float &z, float distance2d, float absAngle) const
{
    GetNearPoint2D(searcher, x, y, distance2d, absAngle);
    z = GetPositionZ();
    (searcher ? searcher : this)->UpdateAllowedPositionZ(x, y, z);

    // if detection disabled, return first point
    if (!sWorld->getBoolConfig(CONFIG_DETECT_POS_COLLISION))
        return;

    // return if the point is already in LoS
    if (IsWithinLOS(x, y, z))
        return;

    // remember first point
    float first_x = x;
    float first_y = y;
    float first_z = z;

    // loop in a circle to look for a point in LoS using small steps
    for (float angle = float(M_PI) / 8; angle < float(M_PI) * 2; angle += float(M_PI) / 8)
    {
        GetNearPoint2D(searcher, x, y, distance2d, absAngle + angle);
        z = GetPositionZ();
        UpdateAllowedPositionZ(x, y, z);
        if (IsWithinLOS(x, y, z))
            return;
    }

    // still not in LoS, give up and return first position found
    x = first_x;
    y = first_y;
    z = first_z;
}

void WorldObject::GetClosePoint(float &x, float &y, float &z, float size, float distance2d /*= 0*/, float relAngle /*= 0*/) const
{
    // angle calculated from current orientation
    GetNearPoint(nullptr, x, y, z, distance2d + size, GetOrientation() + relAngle);
}

Position WorldObject::GetNearPosition(float dist, float angle)
{
    Position pos = GetPosition();
    MovePosition(pos, dist, angle);
    return pos;
}

Position WorldObject::GetFirstCollisionPosition(float dist, float angle)
{
    Position pos = GetPosition();
    MovePositionToFirstCollision(pos, dist, angle);
    return pos;
}

Position WorldObject::GetRandomNearPosition(float radius)
{
    Position pos = GetPosition();
    MovePosition(pos, radius * rand_norm(), rand_norm() * static_cast<float>(2 * M_PI));
    return pos;
}

void WorldObject::GetContactPoint(const WorldObject* obj, float &x, float &y, float &z, float distance2d /*= CONTACT_DISTANCE*/) const
{
    // angle to face `obj` to `this` using distance includes size of `obj`
    GetNearPoint(obj, x, y, z, distance2d, GetAngle(obj));
}

void WorldObject::MovePosition(Position &pos, float dist, float angle)
{
    angle += GetOrientation();
    float destx, desty, destz, ground, floor;
    destx = pos.m_positionX + dist * std::cos(angle);
    desty = pos.m_positionY + dist * std::sin(angle);

    // Prevent invalid coordinates here, position is unchanged
    if (!Trinity::IsValidMapCoord(destx, desty, pos.m_positionZ))
    {
        TC_LOG_FATAL("misc", "WorldObject::MovePosition: Object (TypeId: %u Entry: %u GUID: %u) has invalid coordinates X: %f and Y: %f were passed!",
            GetTypeId(), GetEntry(), GetGUID().GetCounter(), destx, desty);
        return;
    }

    ground = GetMapHeight(destx, desty, MAX_HEIGHT);
    floor = GetMapHeight(destx, desty, pos.m_positionZ);
    destz = std::fabs(ground - pos.m_positionZ) <= std::fabs(floor - pos.m_positionZ) ? ground : floor;

    float step = dist/10.0f;

    for (uint8 j = 0; j < 10; ++j)
    {
        // do not allow too big z changes
        if (std::fabs(pos.m_positionZ - destz) > 6)
        {
            destx -= step * std::cos(angle);
            desty -= step * std::sin(angle);
            ground = GetMapHeight(destx, desty, MAX_HEIGHT);
            floor = GetMapHeight(destx, desty, pos.m_positionZ);
            destz = std::fabs(ground - pos.m_positionZ) <= std::fabs(floor - pos.m_positionZ) ? ground : floor;
        }
        // we have correct destz now
        else
        {
            pos.Relocate(destx, desty, destz);
            break;
        }
    }

    Trinity::NormalizeMapCoord(pos.m_positionX);
    Trinity::NormalizeMapCoord(pos.m_positionY);
    UpdateGroundPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    pos.SetOrientation(GetOrientation());
}

void WorldObject::MovePositionToFirstCollision(Position &pos, float dist, float angle)
{
    angle += GetOrientation();
    float destx, desty, destz;
    destx = pos.m_positionX + dist * std::cos(angle);
    desty = pos.m_positionY + dist * std::sin(angle);
    destz = pos.m_positionZ;

    // Prevent invalid coordinates here, position is unchanged
    if (!Trinity::IsValidMapCoord(destx, desty))
    {
        TC_LOG_FATAL("misc", "WorldObject::MovePositionToFirstCollision invalid coordinates X: %f and Y: %f were passed!", destx, desty);
        return;
    }

    // Use a detour raycast to get our first collision point
    PathGenerator path(this);
    path.SetUseRaycast(true);
    path.CalculatePath(destx, desty, destz, false);

    // We have a invalid path result. Skip further processing.
    if (!(path.GetPathType() & PATHFIND_NOT_USING_PATH))
    {
        // Then check if we have any other flag that makes the result invalid
        if (path.GetPathType() & ~(PATHFIND_NORMAL | PATHFIND_SHORTCUT | PATHFIND_INCOMPLETE | PATHFIND_FARFROMPOLY_END | PATHFIND_NOT_USING_PATH))
            return;
    }

    G3D::Vector3 result = path.GetPath().back();
    destx = result.x;
    desty = result.y;
    destz = result.z;

    // Object is using a shortcut. Check static LOS
    float halfHeight = GetCollisionHeight() * 0.5f;
    bool col = false;

    // Unit is flying. Do a VMap check to avoid moving the position into walls or obstacles
    if (path.GetPathType() & PATHFIND_NOT_USING_PATH)
    {
        uint32 terrainMapId = PhasingHandler::GetTerrainMapId(GetPhaseShift(), GetMapId(), GetMap()->GetTerrain(), pos.m_positionX, pos.m_positionY);
        col = VMAP::VMapFactory::createOrGetVMapManager()->getObjectHitPos(terrainMapId,
            pos.m_positionX, pos.m_positionY, pos.m_positionZ + halfHeight,
            destx, desty, destz + halfHeight,
            destx, desty, destz, -0.5f);

        destz -= halfHeight;

        // Collided with static LOS object, move back to collision point
        if (col)
        {
            destx -= CONTACT_DISTANCE * std::cos(angle);
            desty -= CONTACT_DISTANCE * std::sin(angle);
            dist = std::sqrt((pos.m_positionX - destx) * (pos.m_positionX - destx) + (pos.m_positionY - desty) * (pos.m_positionY - desty));
        }
    }

    // check dynamic collision
    col = GetMap()->getObjectHitPos(GetPhaseShift(),
        pos.m_positionX, pos.m_positionY, pos.m_positionZ + halfHeight,
        destx, desty, destz + halfHeight,
        destx, desty, destz, -0.5f);

    destz -= halfHeight;

    // Collided with a gameobject, move back to collision point
    if (col)
    {
        destx -= CONTACT_DISTANCE * std::cos(angle);
        desty -= CONTACT_DISTANCE * std::sin(angle);
        dist = std::sqrt((pos.m_positionX - destx)*(pos.m_positionX - destx) + (pos.m_positionY - desty) * (pos.m_positionY - desty));
    }

    float groundZ = VMAP_INVALID_HEIGHT_VALUE;
    Trinity::NormalizeMapCoord(pos.m_positionX);
    Trinity::NormalizeMapCoord(pos.m_positionY);
    UpdateAllowedPositionZ(destx, desty, destz, &groundZ);

    pos.SetOrientation(GetOrientation());
    pos.Relocate(destx, desty, destz);

    // position has no ground under it (or is too far away)
    if (groundZ <= INVALID_HEIGHT)
    {
        if (Unit const* unit = ToUnit())
        {
            // flying, ignore.
            if (unit->CanFly())
                return;

            // fall back to gridHeight if any
            float gridHeight = GetMap()->GetGridHeight(GetPhaseShift(), pos.m_positionX, pos.m_positionY);
            if (gridHeight > INVALID_HEIGHT)
                pos.m_positionZ = gridHeight + unit->GetHoverOffset();
        }
    }
}

void WorldObject::PlayDistanceSound(uint32 sound_id, Player* target /*= nullptr*/)
{
    WorldPackets::Misc::PlayObjectSound packet;
    packet.SoundKitID = sound_id;
    packet.SourceObjectGUID = GetGUID();
    packet.TargetObjectGUID = target ? target->GetGUID() : GetGUID();

    if (target)
        target->SendDirectMessage(packet.Write());
    else
        SendMessageToSet(packet.Write(), true);
}

void WorldObject::PlayDirectSound(uint32 sound_id, Player* target /*= nullptr*/)
{
    WorldPackets::Misc::PlaySound packet;
    packet.SourceObjectGUID = GetGUID();
    packet.SoundKitID = sound_id;
    if (target)
        target->SendDirectMessage(packet.Write());
    else
        SendMessageToSet(packet.Write(), true);
}

void WorldObject::PlayDirectMusic(uint32 musicId, Player* target /*= nullptr*/)
{
    if (target)
        target->SendDirectMessage(WorldPackets::Misc::PlayMusic(musicId, GetGUID()).Write());
    else
        SendMessageToSet(WorldPackets::Misc::PlayMusic(musicId, GetGUID()).Write(), true);
}

void WorldObject::DestroyForNearbyPlayers()
{
    if (!IsInWorld())
        return;

    std::list<Player*> targets;
    Trinity::AnyPlayerInObjectRangeCheck check(this, GetVisibilityRange(), false);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(this, targets, check);
    Cell::VisitWorldObjects(this, searcher, GetVisibilityRange());
    for (std::list<Player*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
    {
        Player* player = (*iter);

        if (player == this)
            continue;

        if (!player->HaveAtClient(this))
            continue;

        if (isType(TYPEMASK_UNIT) && ToUnit()->GetCharmerGUID() == player->GetGUID()) /// @todo this is for puppet
            continue;

        if (GetTypeId() == TYPEID_UNIT)
            DestroyForPlayer(player, ToUnit()->IsDuringRemoveFromWorld() && ToCreature()->isDead()); // at remove from world (destroy) show kill animation
        else
            DestroyForPlayer(player);

        player->m_clientGUIDs.erase(GetGUID());
    }
}

void WorldObject::UpdateObjectVisibility(bool /*forced*/)
{
    //updates object's visibility for nearby players
    Trinity::VisibleChangesNotifier notifier(*this);
    Cell::VisitWorldObjects(this, notifier, GetVisibilityRange());
}

struct WorldObjectChangeAccumulator
{
    UpdateDataMapType& i_updateDatas;
    WorldObject& i_object;
    GuidSet plr_list;
    WorldObjectChangeAccumulator(WorldObject &obj, UpdateDataMapType &d) : i_updateDatas(d), i_object(obj) { }
    void Visit(PlayerMapType &m)
    {
        Player* source = nullptr;
        for (PlayerMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
        {
            source = iter->GetSource();

            BuildPacket(source);

            if (!source->GetSharedVisionList().empty())
            {
                SharedVisionList::const_iterator it = source->GetSharedVisionList().begin();
                for (; it != source->GetSharedVisionList().end(); ++it)
                    BuildPacket(*it);
            }
        }
    }

    void Visit(CreatureMapType &m)
    {
        Creature* source = nullptr;
        for (CreatureMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
        {
            source = iter->GetSource();
            if (!source->GetSharedVisionList().empty())
            {
                SharedVisionList::const_iterator it = source->GetSharedVisionList().begin();
                for (; it != source->GetSharedVisionList().end(); ++it)
                    BuildPacket(*it);
            }
        }
    }

    void Visit(DynamicObjectMapType &m)
    {
        DynamicObject* source = nullptr;
        for (DynamicObjectMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
        {
            source = iter->GetSource();
            ObjectGuid guid = source->GetCasterGUID();

            if (guid.IsPlayer())
            {
                //Caster may be nullptr if DynObj is in removelist
                if (Player* caster = ObjectAccessor::FindPlayer(guid))
                    if (caster->GetGuidValue(PLAYER_FARSIGHT) == source->GetGUID())
                        BuildPacket(caster);
            }
        }
    }

    void BuildPacket(Player* player)
    {
        // Only send update once to a player
        if (plr_list.find(player->GetGUID()) == plr_list.end() && player->HaveAtClient(&i_object))
        {
            i_object.BuildFieldsUpdate(player, i_updateDatas);
            plr_list.insert(player->GetGUID());
        }
    }

    template<class SKIP> void Visit(GridRefManager<SKIP> &) { }
};

void WorldObject::BuildUpdate(UpdateDataMapType& data_map)
{
    WorldObjectChangeAccumulator notifier(*this, data_map);
    //we must build packets for all visible players
    Cell::VisitWorldObjects(this, notifier, GetVisibilityRange());

    ClearUpdateMask(false);
}

bool WorldObject::AddToObjectUpdate()
{
    GetMap()->AddUpdateObject(this);
    return true;
}

void WorldObject::RemoveFromObjectUpdate()
{
    GetMap()->RemoveUpdateObject(this);
}

ObjectGuid WorldObject::GetTransGUID() const
{
    if (GetTransport())
        return GetTransport()->GetTransportGUID();
    return ObjectGuid::Empty;
}

float WorldObject::GetFloorZ() const
{
    if (!IsInWorld())
        return m_staticFloorZ;
    return std::max<float>(m_staticFloorZ, GetMap()->GetGameObjectFloor(GetPhaseShift(), GetPositionX(), GetPositionY(), GetPositionZ() + Z_OFFSET_FIND_HEIGHT));
}

float WorldObject::GetMapWaterOrGroundLevel(float x, float y, float z, float* ground/* = nullptr*/) const
{
    bool swimming = [&]()
    {
        if (Creature const* creature = ToCreature())
            return (!creature->CannotPenetrateWater() && !creature->HasAuraType(SPELL_AURA_WATER_WALK));
        else if (Unit const* unit = ToUnit())
            return !unit->HasAuraType(SPELL_AURA_WATER_WALK);

        return true;
    }();

    return GetMap()->GetWaterOrGroundLevel(GetPhaseShift(), x, y, z, ground, swimming, GetCollisionHeight());
}

float WorldObject::GetMapHeight(float x, float y, float z, bool vmap/* = true*/, float distanceToSearch/* = DEFAULT_HEIGHT_SEARCH*/) const
{
    if (z != MAX_HEIGHT)
        z += Z_OFFSET_FIND_HEIGHT;

    return GetMap()->GetHeight(GetPhaseShift(), x, y, z, vmap, distanceToSearch);
}

template TC_GAME_API void WorldObject::GetGameObjectListWithEntryInGrid(std::list<GameObject*>&, uint32, float) const;
template TC_GAME_API void WorldObject::GetGameObjectListWithEntryInGrid(std::deque<GameObject*>&, uint32, float) const;
template TC_GAME_API void WorldObject::GetGameObjectListWithEntryInGrid(std::vector<GameObject*>&, uint32, float) const;

template TC_GAME_API void WorldObject::GetCreatureListWithEntryInGrid(std::list<Creature*>&, uint32, float) const;
template TC_GAME_API void WorldObject::GetCreatureListWithEntryInGrid(std::deque<Creature*>&, uint32, float) const;
template TC_GAME_API void WorldObject::GetCreatureListWithEntryInGrid(std::vector<Creature*>&, uint32, float) const;

template TC_GAME_API void WorldObject::GetPlayerListInGrid(std::list<Player*>&, float) const;
template TC_GAME_API void WorldObject::GetPlayerListInGrid(std::deque<Player*>&, float) const;
template TC_GAME_API void WorldObject::GetPlayerListInGrid(std::vector<Player*>&, float) const;

void WorldObject::SetAIAnimKitId(uint16 animKitId)
{
    if (m_aiAnimKitId == animKitId)
        return;

    if (animKitId && !sAnimKitStore.LookupEntry(animKitId))
        return;

    m_aiAnimKitId = animKitId;

    WorldPackets::Misc::SetAIAnimKit packet;
    packet.Unit = GetGUID();
    packet.AnimKitID = animKitId;
    SendMessageToSet(packet.Write(), true);
}

void WorldObject::SetMovementAnimKitId(uint16 animKitId)
{
    if (m_movementAnimKitId == animKitId)
        return;

    if (animKitId && !sAnimKitStore.LookupEntry(animKitId))
        return;

    m_movementAnimKitId = animKitId;

    WorldPackets::Misc::SetMovementAnimKit packet;
    packet.Unit = GetGUID();
    packet.AnimKitID = animKitId;
    SendMessageToSet(packet.Write(), true);
}

void WorldObject::SetMeleeAnimKitId(uint16 animKitId)
{
    if (m_meleeAnimKitId == animKitId)
        return;

    if (animKitId && !sAnimKitStore.LookupEntry(animKitId))
        return;

    m_meleeAnimKitId = animKitId;

    WorldPackets::Misc::SetMeleeAnimKit packet;
    packet.Unit = GetGUID();
    packet.AnimKitID = animKitId;
    SendMessageToSet(packet.Write(), true);
}
