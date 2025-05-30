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

#include "Creature.h"
#include "BattlegroundMgr.h"
#include "CellImpl.h"
#include "Common.h"
#include "Containers.h"
#include "CreatureAI.h"
#include "CreatureAISelector.h"
#include "CreatureGroups.h"
#include "CombatPackets.h"
#include "DatabaseEnv.h"
#include "Formulas.h"
#include "GameEventMgr.h"
#include "GameTime.h"
#include "GossipDef.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "GroupMgr.h"
#include "InstanceScript.h"
#include "Log.h"
#include "LootMgr.h"
#include "MapManager.h"
#include "MiscPackets.h"
#include "MotionMaster.h"
#include "MoveSpline.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "PoolMgr.h"
#include "QueryPackets.h"
#include "QuestDef.h"
#include "ScriptedGossip.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "Transport.h"
#include "Util.h"
#include "Vehicle.h"
#include "World.h"
#include "WorldPacket.h"
#include <G3D/g3dmath.h>

VendorItemCount::VendorItemCount(uint32 _item, uint32 _count)
    : itemId(_item), count(_count), lastIncrementTime(GameTime::GetGameTime()) { }

CreatureMovementData::CreatureMovementData() : HoverInitiallyEnabled(false), Random(CreatureRandomMovementType::Walk), InteractionPauseTimer(sWorld->getIntConfig(CONFIG_CREATURE_STOP_FOR_PLAYER)) { }


std::string CreatureMovementData::ToString() const
{
    char const* const RandomStates[] = { "Walk", "CanRun", "AlwaysRun" };

    std::ostringstream str;
    str << std::boolalpha
        << ", HoverInitiallyEnabled: " << HoverInitiallyEnabled
        << "Random: " << RandomStates[AsUnderlyingType(Random)];

    str << ", InteractionPauseTimer: " << InteractionPauseTimer;

    return str.str();
}

bool VendorItem::IsGoldRequired(ItemTemplate const* pProto) const
{
    return (pProto->GetFlags2() & ITEM_FLAG2_DONT_IGNORE_BUY_PRICE) || !ExtendedCost;
}

bool VendorItemData::RemoveItem(uint32 item_id, uint8 type)
{
    auto newEnd = std::remove_if(m_items.begin(), m_items.end(), [=](VendorItem const& vendorItem)
    {
        return vendorItem.item == item_id && vendorItem.Type == type;
    });

    bool found = (newEnd != m_items.end());
    m_items.erase(newEnd, m_items.end());
    return found;
}

VendorItem const* VendorItemData::FindItemCostPair(uint32 item_id, uint32 extendedCost, uint8 type) const
{
    for (VendorItem const& vendorItem : m_items)
        if (vendorItem.item == item_id && vendorItem.ExtendedCost == extendedCost && vendorItem.Type == type)
            return &vendorItem;
    return nullptr;
}

CreatureModel const CreatureModel::DefaultInvisibleModel(11686, 0, 1.0f);
CreatureModel const CreatureModel::DefaultVisibleModel(17519, 0, 1.0f);

CreatureModel const* CreatureTemplate::GetModelByIdx(uint32 idx) const
{
    auto itr = std::find_if(Models.begin(), Models.end(), [&](CreatureModel const& model)
        {
            return model.Idx == idx;
        });
    if (itr != Models.end())
        return &*itr;

    return nullptr;
}

CreatureModel const* CreatureTemplate::GetRandomValidModel() const
{
    if (!Models.size())
        return nullptr;

    // If only one element, ignore the Probability (even if 0)
    if (Models.size() == 1)
        return &Models[0];

    auto selectedItr = Trinity::Containers::SelectRandomWeightedContainerElement(Models, [](CreatureModel const& model)
        {
            return model.Probability;
        });

    return &(*selectedItr);
}

CreatureModel const* CreatureTemplate::GetFirstValidModel() const
{
    for (CreatureModel const& model : Models)
        if (model.CreatureDisplayID)
            return &model;

    return nullptr;
}

CreatureModel const* CreatureTemplate::GetModelWithDisplayId(uint32 displayId) const
{
    for (CreatureModel const& model : Models)
        if (displayId == model.CreatureDisplayID)
            return &model;

    return nullptr;
}

CreatureModel const* CreatureTemplate::GetFirstInvisibleModel() const
{
    for (CreatureModel const& model : Models)
        if (CreatureModelInfo const* modelInfo = sObjectMgr->GetCreatureModelInfo(model.CreatureDisplayID))
            if (modelInfo && modelInfo->is_trigger)
                return &model;

    return &CreatureModel::DefaultInvisibleModel;
}

CreatureModel const* CreatureTemplate::GetFirstVisibleModel() const
{
    for (CreatureModel const& model : Models)
        if (CreatureModelInfo const* modelInfo = sObjectMgr->GetCreatureModelInfo(model.CreatureDisplayID))
            if (modelInfo && !modelInfo->is_trigger)
                return &model;

    return &CreatureModel::DefaultVisibleModel;
}

void CreatureTemplate::InitializeQueryData()
{
    WorldPacket queryTemp;
    for (uint8 loc = LOCALE_enUS; loc < TOTAL_LOCALES; ++loc)
    {
        queryTemp = BuildQueryData(static_cast<LocaleConstant>(loc));
        QueryData[loc] = queryTemp;
    }
}

WorldPacket CreatureTemplate::BuildQueryData(LocaleConstant loc) const
{
    WorldPackets::Query::QueryCreatureResponse queryTemp;

    std::string locName = Name;
    std::string locNameAlt = FemaleName;
    std::string locTitle = Title;
    if (CreatureLocale const* cl = sObjectMgr->GetCreatureLocale(Entry))
    {
        ObjectMgr::GetLocaleString(cl->Name, loc, locName);
        ObjectMgr::GetLocaleString(cl->FemaleName, loc, locNameAlt);
        ObjectMgr::GetLocaleString(cl->Title, loc, locTitle);
    }

    queryTemp.CreatureID = Entry;
    queryTemp.Allow = true;

    queryTemp.Stats.Name[0] = locName;
    queryTemp.Stats.NameAlt[0] = locNameAlt;
    queryTemp.Stats.Title = locTitle;
    queryTemp.Stats.CursorName = IconName;
    queryTemp.Stats.Flags[0] = type_flags;
    queryTemp.Stats.Flags[1] = type_flags2;
    queryTemp.Stats.CreatureType = type;
    queryTemp.Stats.CreatureFamily = family;
    queryTemp.Stats.Classification = rank;

    for (uint8 i = 0; i < MAX_KILL_CREDIT; ++i)
        queryTemp.Stats.ProxyCreatureID[i] = KillCredit[i];

    for (uint8 i = 0; i < MAX_CREATURE_MODELS; ++i)
    {
        if (CreatureModel const* model = GetModelByIdx(i))
            queryTemp.Stats.CreatureDisplayID[i] = model->CreatureDisplayID;
        else
            queryTemp.Stats.CreatureDisplayID[i] = 0;
    }

    queryTemp.Stats.HpMulti = ModHealth;
    queryTemp.Stats.EnergyMulti = ModMana;
    queryTemp.Stats.Leader = RacialLeader;

    for (uint32 i = 0; i < MAX_CREATURE_QUEST_ITEMS; ++i)
        queryTemp.Stats.QuestItems[i] = 0;

    if (std::vector<uint32> const* items = sObjectMgr->GetCreatureQuestItemList(Entry))
        for (uint32 i = 0; i < MAX_CREATURE_QUEST_ITEMS; ++i)
            if (i < items->size())
                queryTemp.Stats.QuestItems[i] = (*items)[i];

    queryTemp.Stats.CreatureMovementInfoID = movementId;
    queryTemp.Stats.RequiredExpansion = expansionUnknown;
    queryTemp.Write();
    queryTemp.ShrinkToFit();
    return queryTemp.Move();
}

bool AssistDelayEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    if (Unit* victim = ObjectAccessor::GetUnit(m_owner, m_victim))
    {
        while (!m_assistants.empty())
        {
            Creature* assistant = ObjectAccessor::GetCreature(m_owner, *m_assistants.begin());
            m_assistants.pop_front();

            if (assistant && assistant->CanAssistTo(&m_owner, victim))
            {
                assistant->SetNoCallAssistance(true);
                assistant->EngageWithTarget(victim);
            }
        }
    }
    return true;
}

CreatureBaseStats const* CreatureBaseStats::GetBaseStats(uint8 level, uint8 unitClass)
{
    return sObjectMgr->GetCreatureBaseStats(level, unitClass);
}

bool ForcedDespawnDelayEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    m_owner.DespawnOrUnsummon(0, m_respawnTimer);    // since we are here, we are not TempSummon as object type cannot change during runtime
    return true;
}

Creature::Creature(bool isWorldObject): Unit(isWorldObject), MapObject(),
m_groupLootTimer(0), lootingGroupLowGUID(0), m_PlayerDamageReq(0),
m_lootRecipient(), m_lootRecipientGroup(0), _pickpocketLootRestore(0), m_corpseRemoveTime(0), m_respawnTime(0),
m_respawnDelay(300), m_corpseDelay(60), m_wanderDistance(0.0f), m_boundaryCheckTime(2500), m_reactState(REACT_AGGRESSIVE),
m_defaultMovementType(IDLE_MOTION_TYPE), m_spawnId(0), m_equipmentId(0), m_originalEquipmentId(0), m_AlreadyCallAssistance(false),
m_AlreadySearchedAssistance(false), m_regenHealth(true), m_cannotReachTarget(false), m_cannotReachTimer(0), m_meleeDamageSchoolMask(SPELL_SCHOOL_MASK_NORMAL),
m_originalEntry(0), m_homePosition(), m_transportHomePosition(), m_creatureInfo(nullptr), m_creatureData(nullptr), _waypointPathId(0), _currentWaypointNodeInfo(0, 0), _cyclicSplinePathId(0),
m_formation(nullptr), m_triggerJustAppeared(true), m_respawnCompatibilityMode(false), _lastDamagedTime(0), _isMissingSwimmingFlagOutOfCombat(false), _noNpcDamageBelowPctHealth(0.f)
{
    m_valuesCount = UNIT_END;

    m_spells = { };

    DisableReputationGain = false;

    m_SightDistance = sWorld->getFloatConfig(CONFIG_SIGHT_MONSTER);
    m_CombatDistance = 0;//MELEE_RANGE;

    ResetLootMode(); // restore default loot mode
    m_isTempWorldObject = false;
}

void Creature::AddToWorld()
{
    ///- Register the creature for guid lookup
    if (!IsInWorld())
    {
        GetMap()->GetObjectsStore().Insert<Creature>(GetGUID(), this);
        if (m_spawnId)
            GetMap()->GetCreatureBySpawnIdStore().insert(std::make_pair(m_spawnId, this));

        TC_LOG_DEBUG("entities.unit", "Adding creature %u with entry %u and DBGUID %u to world in map %u", GetGUID().GetCounter(), GetEntry(), m_spawnId, GetMap()->GetId());

        UpdatePowerRegeneration(GetPowerType());

        Unit::AddToWorld();
        SearchFormation();
        AIM_Initialize();
        if (IsVehicle())
            GetVehicleKit()->Install();

        if (GetZoneScript())
            GetZoneScript()->OnCreatureCreate(this);
    }
}

void Creature::RemoveFromWorld()
{
    if (IsInWorld())
    {
        if (GetZoneScript())
            GetZoneScript()->OnCreatureRemove(this);

        if (m_formation)
            sFormationMgr->RemoveCreatureFromGroup(m_formation, this);

        Unit::RemoveFromWorld();

        if (m_spawnId)
            Trinity::Containers::MultimapErasePair(GetMap()->GetCreatureBySpawnIdStore(), m_spawnId, this);

        TC_LOG_DEBUG("entities.unit", "Removing creature %u with entry %u and DBGUID %u to world in map %u", GetGUID().GetCounter(), GetEntry(), m_spawnId, GetMap()->GetId());
        GetMap()->GetObjectsStore().Remove<Creature>(GetGUID());
    }
}

void Creature::DisappearAndDie()
{
    ForcedDespawn(0);
}

bool Creature::IsReturningHome() const
{
    if (GetMotionMaster()->GetMotionSlotType(MOTION_SLOT_ACTIVE) == HOME_MOTION_TYPE)
        return true;

    return false;
}

void Creature::SearchFormation()
{
    if (IsSummon())
        return;

    ObjectGuid::LowType lowguid = GetSpawnId();
    if (!lowguid)
        return;

    CreatureGroupInfoType::iterator frmdata = sFormationMgr->CreatureGroupMap.find(lowguid);
    if (frmdata != sFormationMgr->CreatureGroupMap.end())
        sFormationMgr->AddCreatureToGroup(frmdata->second.LeaderGUID, this);
}

bool Creature::IsFormationLeader() const
{
    if (!m_formation)
        return false;

    return m_formation->IsLeader(this);
}

void Creature::SignalFormationMovement()
{
    if (!m_formation)
        return;

    if (!m_formation->IsLeader(this))
        return;

    m_formation->LeaderStartedMoving();
}

bool Creature::IsFormationLeaderMoveAllowed() const
{
    if (!m_formation)
        return false;

    return m_formation->CanLeaderStartMoving();
}

void Creature::RemoveCorpse(bool setSpawnTime, bool destroyForNearbyPlayers)
{
    if (getDeathState() != CORPSE)
        return;

    if (m_respawnCompatibilityMode)
    {
        m_corpseRemoveTime = GameTime::GetGameTime();
        setDeathState(DEAD);
        RemoveAllAuras();
        loot.clear();
        uint32 respawnDelay = m_respawnDelay;
        if (CreatureAI* ai = AI())
            ai->CorpseRemoved(respawnDelay);

        if (destroyForNearbyPlayers)
            DestroyForNearbyPlayers();

        // Should get removed later, just keep "compatibility" with scripts
        if (setSpawnTime)
            m_respawnTime = std::max<time_t>(GameTime::GetGameTime() + respawnDelay, m_respawnTime);

        // if corpse was removed during falling, the falling will continue and override relocation to respawn position
        if (IsFalling())
            StopMoving();

        float x, y, z, o;
        GetRespawnPosition(x, y, z, &o);

        // We were spawned on transport, calculate real position
        if (IsSpawnedOnTransport())
        {
            Position& pos = m_movementInfo.transport.pos;
            pos.m_positionX = x;
            pos.m_positionY = y;
            pos.m_positionZ = z;
            pos.SetOrientation(o);

            if (TransportBase* transport = GetDirectTransport())
                transport->CalculatePassengerPosition(x, y, z, &o);
        }

        UpdateAllowedPositionZ(x, y, z);
        SetHomePosition(x, y, z, o);
        GetMap()->CreatureRelocation(this, x, y, z, o);
    }
    else
    {
        // In case this is called directly and normal respawn timer not set
        // Since this timer will be longer than the already present time it
        // will be ignored if the correct place added a respawn timer
        if (setSpawnTime)
        {
            uint32 respawnDelay = m_respawnDelay;
            m_respawnTime = std::max<time_t>(GameTime::GetGameTime() + respawnDelay, m_respawnTime);

            SaveRespawnTime();
        }

        if (TempSummon* summon = ToTempSummon())
            summon->UnSummon();
        else
            AddObjectToRemoveList();
    }
}

/**
 * change the entry of creature until respawn
 */
bool Creature::InitEntry(uint32 entry, CreatureData const* data /*= nullptr*/)
{
    CreatureTemplate const* normalInfo = sObjectMgr->GetCreatureTemplate(entry);
    if (!normalInfo)
    {
        TC_LOG_ERROR("sql.sql", "Creature::InitEntry creature entry %u does not exist.", entry);
        return false;
    }

    // get difficulty 1 mode entry, skip for pets
    CreatureTemplate const* cinfo = normalInfo;
    for (uint8 diff = uint8(GetMap()->GetSpawnMode()); diff > 0 && !IsPet();)
    {
        // we already have valid Map pointer for current creature!
        if (normalInfo->DifficultyEntry[diff - 1])
        {
            cinfo = sObjectMgr->GetCreatureTemplate(normalInfo->DifficultyEntry[diff - 1]);
            if (cinfo)
                break;                                      // template found

            // check and reported at startup, so just ignore (restore normalInfo)
            cinfo = normalInfo;
        }

        // for instances heroic to normal, other cases attempt to retrieve previous difficulty
        if (diff >= RAID_DIFFICULTY_10MAN_HEROIC && GetMap()->IsRaid())
            diff -= 2;                                      // to normal raid difficulty cases
        else
            --diff;
    }

    // Initialize loot duplicate count depending on raid difficulty
    if (GetMap()->Is25ManRaid())
        loot.maxDuplicates = 3;

    SetEntry(entry);                                        // normal entry always
    m_creatureInfo = cinfo;                                 // map mode related always

    // equal to player Race field, but creature does not have race
    SetByteValue(UNIT_FIELD_BYTES_0, UNIT_BYTES_0_OFFSET_RACE, 0);

    // known valid are: CLASS_WARRIOR, CLASS_PALADIN, CLASS_ROGUE, CLASS_MAGE
    SetByteValue(UNIT_FIELD_BYTES_0, UNIT_BYTES_0_OFFSET_CLASS, uint8(cinfo->unit_class));

    // Cancel load if no model defined
    if (!(cinfo->GetFirstValidModel()))
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has no model defined in table `creature_template`, can't load. ", entry);
        return false;
    }

    CreatureModel model = *ObjectMgr::ChooseDisplayId(cinfo, data);
    CreatureModelInfo const* minfo = sObjectMgr->GetCreatureModelRandomGender(&model, cinfo);
    if (!minfo)                                             // Cancel load if no model defined
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid model %u defined in table `creature_template`, can't load.", entry, model.CreatureDisplayID);
        return false;
    }

    SetDisplayId(model.CreatureDisplayID, true);

    // Load creature equipment
    if (!data || data->equipmentId == 0)
        LoadEquipment(); // use default equipment (if available)
    else                // override, 0 means no equipment
    {
        m_originalEquipmentId = data->equipmentId;
        LoadEquipment(data->equipmentId);
    }

    SetName(normalInfo->Name);                              // at normal entry always

    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);
    SetFloatValue(UNIT_MOD_CAST_HASTE, 1.0f);

    InitializeCreatureMovementInfo(cinfo->movementId);
    InitializeMovementSpeeds();

    // Will set UNIT_FIELD_BOUNDINGRADIUS and UNIT_FIELD_COMBATREACH
    SetObjectScale(GetNativeObjectScale());

    SetFloatValue(UNIT_FIELD_HOVERHEIGHT, cinfo->HoverHeight);

    SetCanDualWield(cinfo->flags_extra & CREATURE_FLAG_EXTRA_USE_OFFHAND_ATTACK);

    // checked at loading
    m_defaultMovementType = MovementGeneratorType(data ? data->movementType : cinfo->MovementType);
    if (!m_wanderDistance && m_defaultMovementType == RANDOM_MOTION_TYPE)
        m_defaultMovementType = IDLE_MOTION_TYPE;

    for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
        m_spells[i] = GetCreatureTemplate()->spells[i];

    CreatureStaticFlagsHolder staticFlags = GenerateStaticFlags(cinfo, GetSpawnId(), GetMap()->GetDifficulty());
    ApplyAllStaticFlags(staticFlags);

    _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_NO_XP, cinfo->type == CREATURE_TYPE_CRITTER
        || IsPet()
        || IsTotem()
        || cinfo->flags_extra & CREATURE_FLAG_EXTRA_NO_XP_AT_KILL);

    _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_4_TREAT_AS_RAID_UNIT_FOR_HELPFUL_SPELLS, (cinfo->type_flags & CREATURE_TYPE_FLAG_TREAT_AS_RAID_UNIT) != 0);

    SetNoNpcDamageBelowPctHealthValue(sObjectMgr->GetSparringHealthLimitFor(GetEntry()));

    return true;
}

bool Creature::UpdateEntry(uint32 entry, CreatureData const* data /*= nullptr*/, bool updateLevel /* = true */)
{
    if (!InitEntry(entry, data))
        return false;

    CreatureTemplate const* cInfo = GetCreatureTemplate();

    m_regenHealth = cInfo->RegenHealth;

    // creatures always have melee weapon ready if any unless specified otherwise
    if (!GetCreatureAddon())
        SetSheath(SHEATH_STATE_MELEE);

    SetFaction(cInfo->faction);

    uint32 npcflags, unitFlags, unitFlags2;
    ObjectMgr::ChooseCreatureFlags(cInfo, &npcflags, &unitFlags, &unitFlags2, _staticFlags, data);

    if (cInfo->flags_extra & CREATURE_FLAG_EXTRA_WORLDEVENT)
        npcflags |= sGameEventMgr->GetNPCFlag(this);

    SetUInt32Value(UNIT_NPC_FLAGS, npcflags);

    // if unit is in combat, keep this flag
    unitFlags &= ~UNIT_FLAG_IN_COMBAT;
    if (IsInCombat())
        unitFlags |= UNIT_FLAG_IN_COMBAT;

    SetUInt32Value(UNIT_FIELD_FLAGS, unitFlags);
    SetUInt32Value(UNIT_FIELD_FLAGS_2, unitFlags2);

    SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_NONE);

    SetCanDualWield(cInfo->flags_extra & CREATURE_FLAG_EXTRA_USE_OFFHAND_ATTACK);

    SetAttackTime(BASE_ATTACK,   cInfo->BaseAttackTime);
    SetAttackTime(OFF_ATTACK,    cInfo->BaseAttackTime);
    SetAttackTime(RANGED_ATTACK, cInfo->RangeAttackTime);

    if (updateLevel)
        SelectLevel();

    // Do not update guardian stats here - they are handled in Guardian::InitStatsForLevel()
    if (!IsGuardian())
    {
        uint32 previousHealth = GetHealth();
        UpdateLevelDependantStats();
        if (previousHealth > 0)
            SetHealth(previousHealth);

        SetMeleeDamageSchool(SpellSchools(cInfo->dmgschool));
        SetStatFlatModifier(UNIT_MOD_RESISTANCE_HOLY,   BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_HOLY]));
        SetStatFlatModifier(UNIT_MOD_RESISTANCE_FIRE,   BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_FIRE]));
        SetStatFlatModifier(UNIT_MOD_RESISTANCE_NATURE, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_NATURE]));
        SetStatFlatModifier(UNIT_MOD_RESISTANCE_FROST,  BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_FROST]));
        SetStatFlatModifier(UNIT_MOD_RESISTANCE_SHADOW, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_SHADOW]));
        SetStatFlatModifier(UNIT_MOD_RESISTANCE_ARCANE, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_ARCANE]));

        SetCanModifyStats(true);
        UpdateAllStats();
    }

    // checked and error show at loading templates
    if (FactionTemplateEntry const* factionTemplate = sFactionTemplateStore.LookupEntry(cInfo->faction))
        SetPvP(factionTemplate->GetFlags().HasFlag(FactionTemplateFlags::AssistPlayers));

    // updates spell bars for vehicles and set player's faction - should be called here, to overwrite faction that is set from the new template
    if (IsVehicle())
    {
        if (Player* owner = Creature::GetCharmerOrOwnerPlayerOrPlayerItself()) // this check comes in case we don't have a player
        {
            SetFaction(owner->GetFaction()); // vehicles should have same as owner faction
            owner->VehicleSpellInitialize();
        }
    }

    // trigger creature is always not selectable and can not be attacked
    if (IsTrigger())
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

    InitializeReactState();

    if (cInfo->flags_extra & CREATURE_FLAG_EXTRA_NO_TAUNT)
    {
        ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
        ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
    }


    InitializeMovementCapabilities();

    LoadCreaturesAddon();
    LoadTemplateImmunities();

    GetThreatManager().EvaluateSuppressed();
    return true;
}

CreatureStaticFlagsHolder Creature::GenerateStaticFlags(CreatureTemplate const* cInfo, ObjectGuid::LowType spawnId, Difficulty difficultyId) const
{
    CreatureStaticFlagsOverride const* staticFlagsOverride = sObjectMgr->GetCreatureStaticFlagsOverride(spawnId, difficultyId);
    if (!staticFlagsOverride)
        return cInfo->StaticFlags;

    return CreatureStaticFlagsHolder(
        staticFlagsOverride->StaticFlags1.value_or(cInfo->StaticFlags.GetFlags()),
        staticFlagsOverride->StaticFlags2.value_or(cInfo->StaticFlags.GetFlags2()),
        staticFlagsOverride->StaticFlags3.value_or(cInfo->StaticFlags.GetFlags3()),
        staticFlagsOverride->StaticFlags4.value_or(cInfo->StaticFlags.GetFlags4()),
        staticFlagsOverride->StaticFlags5.value_or(cInfo->StaticFlags.GetFlags5()));
}

void Creature::ApplyAllStaticFlags(CreatureStaticFlagsHolder const& flags)
{
    _staticFlags = flags;

    m_updateFlag.NoBirthAnim = flags.HasFlag(CREATURE_STATIC_FLAG_4_NO_BIRTH_ANIM);
}

void Creature::Update(uint32 diff)
{
    if (IsAIEnabled() && m_triggerJustAppeared && m_deathState != DEAD)
    {
        if (m_respawnCompatibilityMode && m_vehicleKit)
            m_vehicleKit->Reset();
        m_triggerJustAppeared = false;
        AI()->JustAppeared();
    }

    UpdateMovementCapabilities();

    GetThreatManager().Update(diff);

    switch (m_deathState)
    {
        case JUST_RESPAWNED:
            // Must not be called, see Creature::setDeathState JUST_RESPAWNED -> ALIVE promoting.
            TC_LOG_ERROR("entities.unit", "Creature (GUID: %u Entry: %u) in wrong state: JUST_RESPAWNED (4)", GetGUID().GetCounter(), GetEntry());
            break;
        case JUST_DIED:
            // Must not be called, see Creature::setDeathState JUST_DIED -> CORPSE promoting.
            TC_LOG_ERROR("entities.unit", "Creature (GUID: %u Entry: %u) in wrong state: JUST_DIED (1)", GetGUID().GetCounter(), GetEntry());
            break;
        case DEAD:
        {
            if (!m_respawnCompatibilityMode)
            {
                TC_LOG_ERROR("entities.unit", "Creature (GUID: %u Entry: %u) in wrong state: DEAD (3)", GetGUID().GetCounter(), GetEntry());
                break;
            }
            time_t now = GameTime::GetGameTime();
            if (m_respawnTime <= now)
            {
                // Delay respawn if spawn group is not active
                if (m_creatureData && !GetMap()->IsSpawnGroupActive(m_creatureData->spawnGroupData->groupId))
                {
                    m_respawnTime = now + urand(4, 7);
                    break; // Will be rechecked on next Update call after delay expires
                }

                ObjectGuid dbtableHighGuid(HighGuid::Unit, GetEntry(), m_spawnId);
                time_t linkedRespawnTime = GetMap()->GetLinkedRespawnTime(dbtableHighGuid);
                if (!linkedRespawnTime)             // Can respawn       // Can respawn
                    Respawn();
                else                                // the master is dead
                {
                    ObjectGuid targetGuid = sObjectMgr->GetLinkedRespawnGuid(dbtableHighGuid);
                    if (targetGuid == dbtableHighGuid) // if linking self, never respawn (check delayed to next day)
                        SetRespawnTime(WEEK);
                    else
                    {
                        // else copy time from master and add a little
                        time_t baseRespawnTime = std::max(linkedRespawnTime, now);
                        time_t const offset = urand(5, MINUTE);

                        // linked guid can be a boss, uses std::numeric_limits<time_t>::max to never respawn in that instance
                        // we shall inherit it instead of adding and causing an overflow
                        if (baseRespawnTime <= std::numeric_limits<time_t>::max() - offset)
                            m_respawnTime = baseRespawnTime + offset;
                        else
                            m_respawnTime = std::numeric_limits<time_t>::max();
                    }
                    SaveRespawnTime(); // also save to DB immediately
                }
            }
            break;
        }
        case CORPSE:
        {
            Unit::Update(diff);
            // deathstate changed on spells update, prevent problems
            if (m_deathState != CORPSE)
                break;

            if (IsEngaged())
                Unit::AIUpdateTick(diff);

            if (m_groupLootTimer && lootingGroupLowGUID)
            {
                if (m_groupLootTimer <= diff)
                {
                    Group* group = sGroupMgr->GetGroupByGUID(lootingGroupLowGUID);
                    if (group)
                        group->EndRoll(&loot, GetMap());
                    m_groupLootTimer = 0;
                    lootingGroupLowGUID = 0;
                }
                else m_groupLootTimer -= diff;
            }
            else if (m_corpseRemoveTime <= GameTime::GetGameTime())
            {
                RemoveCorpse(false);
                TC_LOG_DEBUG("entities.unit", "Removing corpse... %u ", GetUInt32Value(OBJECT_FIELD_ENTRY));
            }
            break;
        }
        case ALIVE:
        {
            Unit::Update(diff);

            // creature can be dead after Unit::Update call
            // CORPSE/DEAD state will processed at next tick (in other case death timer will be updated unexpectedly)
            if (!IsAlive())
                break;

            if (_spellFocusInfo.ReacquiringTargetDelay)
            {
                if (_spellFocusInfo.ReacquiringTargetDelay <= diff)
                    ReacquireSpellFocusTarget();
                else
                    _spellFocusInfo.ReacquiringTargetDelay -= diff;
            }

            // periodic check to see if the creature has passed an evade boundary
            if (IsAIEnabled() && !IsInEvadeMode() && IsEngaged())
            {
                if (diff >= m_boundaryCheckTime)
                {
                    AI()->CheckInRoom();
                    m_boundaryCheckTime = 2500;
                } else
                    m_boundaryCheckTime -= diff;
            }

            // do not allow the AI to be changed during update
            Unit::AIUpdateTick(diff);

            // creature can be dead after UpdateAI call
            // CORPSE/DEAD state will processed at next tick (in other case death timer will be updated unexpectedly)
            if (!IsAlive())
                break;

            _powerUpdateTimer -= diff;
            Regenerate(GetPowerType(), diff);

            // Reset the power update timer after regeneration happened
            if (_powerUpdateTimer <= 0)
                _powerUpdateTimer = GetPowerUpdateInterval();

            if (CanNotReachTarget() && !IsInEvadeMode() && !GetMap()->IsRaid())
            {
                m_cannotReachTimer += diff;
                if (m_cannotReachTimer >= CREATURE_NOPATH_EVADE_TIME)
                    if (CreatureAI* ai = AI())
                        ai->EnterEvadeMode(CreatureAI::EVADE_REASON_NO_PATH);
            }
            break;
        }
        default:
            break;
    }
}

void Creature::Heartbeat()
{
    Unit::Heartbeat();

    // Creatures with CREATURE_STATIC_FLAG_2_FORCE_PARTY_MEMBERS_INTO_COMBAT periodically force party members into combat
    ForcePartyMembersIntoCombat();
}

void Creature::RegenerateHealth()
{
    if (!isRegeneratingHealth())
        return;

    if (IsInEvadeMode() || (IsEngaged() && !IsPolymorphed() && !CanNotReachTarget())) // regenerate health if not in combat or if polymorphed
        return;

    uint32 curValue = GetHealth();
    uint32 maxValue = GetMaxHealth();

    if (curValue >= maxValue)
        return;

    uint32 addvalue = 0;

    // Not only pet, but any controlled creature
    if (!GetCharmerOrOwnerGUID().IsEmpty())
    {
        float healthIncreaseRate = sWorld->getRate(RATE_HEALTH);
        addvalue = 0.015f * ((float)GetMaxHealth()) * healthIncreaseRate;
    }
    else
        addvalue = maxValue / 3;

    // Apply modifiers (if any).
    addvalue *= GetTotalAuraMultiplier(SPELL_AURA_MOD_HEALTH_REGEN_PERCENT);
    addvalue += GetTotalAuraModifier(SPELL_AURA_MOD_REGEN) * UNIT_HEALTH_REGENERATION_INTERVAL / (5 * IN_MILLISECONDS);

    ModifyHealth(addvalue);
}

void Creature::DoFleeToGetAssistance()
{
    if (!GetVictim())
        return;

    if (HasAuraType(SPELL_AURA_PREVENTS_FLEEING))
        return;

    float radius = sWorld->getFloatConfig(CONFIG_CREATURE_FAMILY_FLEE_ASSISTANCE_RADIUS);
    if (radius >0)
    {
        Creature* creature = nullptr;
        Trinity::NearestAssistCreatureInCreatureRangeCheck u_check(this, GetVictim(), radius);
        Trinity::CreatureLastSearcher<Trinity::NearestAssistCreatureInCreatureRangeCheck> searcher(this, creature, u_check);
        Cell::VisitGridObjects(this, searcher, radius);

        SetNoSearchAssistance(true);

        if (!creature)
            //SetFeared(true, EnsureVictim()->GetGUID(), 0, sWorld->getIntConfig(CONFIG_CREATURE_FAMILY_FLEE_DELAY));
            /// @todo use 31365
            SetControlled(true, UNIT_STATE_FLEEING);
        else
            GetMotionMaster()->MoveSeekAssistance(creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ());
    }
}

bool Creature::AIM_Destroy()
{
    PopAI();
    RefreshAI();
    return true;
}

bool Creature::AIM_Create(CreatureAI* ai /*= nullptr*/)
{
    Motion_Initialize();

    SetAI(ai ? ai : FactorySelector::SelectAI(this));

    return true;
}

bool Creature::AIM_Initialize(CreatureAI* ai)
{
    if (!AIM_Create(ai))
        return false;

    AI()->InitializeAI();
    if (GetVehicleKit())
        GetVehicleKit()->Reset();
    return true;
}

void Creature::Motion_Initialize()
{
    if (!m_formation)
        GetMotionMaster()->Initialize();
    else if (m_formation->getLeader() == this)
    {
        m_formation->FormationReset(false);
        GetMotionMaster()->Initialize();
    }
    else if (m_formation->isFormed())
        GetMotionMaster()->MoveIdle(); //wait the order of leader
    else
        GetMotionMaster()->Initialize();
}

bool Creature::Create(ObjectGuid::LowType guidlow, Map* map, uint32 entry, Position const& pos, CreatureData const* data /*= nullptr*/, uint32 vehId /*= 0*/, bool dynamic)
{
    ASSERT(map);
    SetMap(map);

    if (data)
    {
        PhasingHandler::InitDbPhaseShift(GetPhaseShift(), data->phaseUseFlags, data->phaseId, data->phaseGroup);
        PhasingHandler::InitDbVisibleMapId(GetPhaseShift(), data->terrainSwapMap);
    }

    // Set if this creature can handle dynamic spawns
    if (!dynamic)
        SetRespawnCompatibilityMode();

    CreatureTemplate const* cinfo = sObjectMgr->GetCreatureTemplate(entry);
    if (!cinfo)
    {
        TC_LOG_ERROR("sql.sql", "Creature::Create(): creature template (guidlow: %u, entry: %u) does not exist.", guidlow, entry);
        return false;
    }

    //! Relocate before CreateFromProto, to initialize coords and allow
    //! returning correct zone id for selecting OutdoorPvP/Battlefield script
    Relocate(pos);

    // Check if the position is valid before calling CreateFromProto(), otherwise we might add Auras to Creatures at
    // invalid position, triggering a crash about Auras not removed in the destructor
    if (!IsPositionValid())
    {
        TC_LOG_ERROR("entities.unit", "Creature::Create(): given coordinates for creature (guidlow %d, entry %d) are not valid (X: %f, Y: %f, Z: %f, O: %f)", guidlow, entry, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
        return false;
    }

    {
        // area/zone id is needed immediately for ZoneScript::GetCreatureEntry hook before it is known which creature template to load (no model/scale available yet)
        PositionFullTerrainStatus data;
        GetMap()->GetFullTerrainStatusForPosition(GetPhaseShift(), GetPositionX(), GetPositionY(), GetPositionZ(), data, map_liquidHeaderTypeFlags::AllLiquids, DEFAULT_COLLISION_HEIGHT);
        ProcessPositionDataChanged(data);
    }

    // Allow players to see those units while dead, do it here (mayby altered by addon auras)
    if (cinfo->type_flags & CREATURE_TYPE_FLAG_GHOST_VISIBLE)
        m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE | GHOST_VISIBILITY_GHOST);

    if (!CreateFromProto(guidlow, entry, data, vehId))
        return false;

    if (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_DUNGEON_BOSS && map->IsDungeon())
        m_respawnDelay = 0; // special value, prevents respawn for dungeon bosses unless overridden

    switch (GetCreatureTemplate()->rank)
    {
        case CREATURE_ELITE_RARE:
            m_corpseDelay = sWorld->getIntConfig(CONFIG_CORPSE_DECAY_RARE);
            break;
        case CREATURE_ELITE_ELITE:
            m_corpseDelay = sWorld->getIntConfig(CONFIG_CORPSE_DECAY_ELITE);
            break;
        case CREATURE_ELITE_RAREELITE:
            m_corpseDelay = sWorld->getIntConfig(CONFIG_CORPSE_DECAY_RAREELITE);
            break;
        case CREATURE_ELITE_WORLDBOSS:
            m_corpseDelay = sWorld->getIntConfig(CONFIG_CORPSE_DECAY_WORLDBOSS);
            break;
        default:
            m_corpseDelay = sWorld->getIntConfig(CONFIG_CORPSE_DECAY_NORMAL);
            break;
    }

    LoadCreaturesAddon();

    //! Need to be called after LoadCreaturesAddon - MOVEMENTFLAG_HOVER is set there
    m_positionZ += GetHoverOffset();

    LastUsedScriptID = GetScriptId();

    if (IsSpiritHealer() || IsSpiritGuide() || (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_GHOST_VISIBILITY))
    {
        m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_GHOST);
        m_serverSideVisibilityDetect.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_GHOST);
    }

    if (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_IGNORE_PATHFINDING)
        AddUnitState(UNIT_STATE_IGNORE_PATHFINDING);

    if (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_IMMUNITY_KNOCKBACK)
    {
        ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
        ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
    }

    GetThreatManager().Initialize();

    return true;
}

Unit* Creature::SelectVictim()
{
    Unit* target = nullptr;

    if (CanHaveThreatList())
        target = GetThreatManager().GetCurrentVictim();
    else if (!HasReactState(REACT_PASSIVE))
    {
        // We're a player pet, probably
        target = getAttackerForHelper();
        if (!target && IsSummon())
        {
            if (Unit* owner = ToTempSummon()->GetOwner())
            {
                if (owner->IsInCombat())
                    target = owner->getAttackerForHelper();
                if (!target)
                {
                    for (ControlList::const_iterator itr = owner->m_Controlled.begin(); itr != owner->m_Controlled.end(); ++itr)
                    {
                        if ((*itr)->IsInCombat())
                        {
                            target = (*itr)->getAttackerForHelper();
                            if (target)
                                break;
                        }
                    }
                }
            }
        }
    }
    else
        return nullptr;

    if (target && _IsTargetAcceptable(target) && CanCreatureAttack(target))
        return target;

    /// @todo a vehicle may eat some mob, so mob should not evade
    if (GetVehicle())
        return nullptr;

    Unit::AuraEffectList const& iAuras = GetAuraEffectsByType(SPELL_AURA_MOD_INVISIBILITY);
    if (!iAuras.empty())
    {
        for (Unit::AuraEffectList::const_iterator itr = iAuras.begin(); itr != iAuras.end(); ++itr)
        {
            if ((*itr)->GetBase()->IsPermanent())
            {
                AI()->EnterEvadeMode(CreatureAI::EVADE_REASON_OTHER);
                break;
            }
        }
        return nullptr;
    }

    // enter in evade mode in other case
    AI()->EnterEvadeMode(CreatureAI::EVADE_REASON_NO_HOSTILES);

    return nullptr;
}

bool Creature::SetDisableGravity(bool disable, bool updateAnimTier /*= true*/)
{
    if (!Unit::SetDisableGravity(disable, updateAnimTier))
        return false;

    if (updateAnimTier && IsAlive() && !HasUnitState(UNIT_STATE_ROOT))
    {
        if (IsGravityDisabled())
            SetAnimTier(AnimTier::Fly);
        else if (IsHovering())
            SetAnimTier(AnimTier::Hover);
        else
            SetAnimTier(AnimTier::Ground);
    }

    WorldPacket data(disable ? SMSG_SPLINE_MOVE_GRAVITY_DISABLE : SMSG_SPLINE_MOVE_GRAVITY_ENABLE);
    WriteMovementInfo(data);
    SendMessageToSet(&data, false);

    return true;
}

bool Creature::SetHover(bool enable, bool updateAnimTier /*= true*/)
{
    if (!Unit::SetHover(enable, updateAnimTier))
        return false;

    if (updateAnimTier && IsAlive() && !HasUnitState(UNIT_STATE_ROOT))
    {
        if (IsGravityDisabled())
            SetAnimTier(AnimTier::Fly);
        else if (IsHovering())
            SetAnimTier(AnimTier::Hover);
        else
            SetAnimTier(AnimTier::Ground);
    }

    return true;
}

bool Creature::SetCanFly(bool enable)
{
    if (!Unit::SetCanFly(enable))
        return false;

    WorldPacket data(enable ? SMSG_SPLINE_MOVE_SET_FLYING : SMSG_SPLINE_MOVE_UNSET_FLYING);
    WriteMovementInfo(data);
    SendMessageToSet(&data, false);

    return true;
}

void Creature::InitializeReactState()
{
    if (IsTotem() || IsTrigger() || IsCritter() || IsSpiritService() || _staticFlags.HasFlag(CREATURE_STATIC_FLAG_IGNORE_COMBAT))
        SetReactState(REACT_PASSIVE);
    /*
    else if (IsCivilian())
        SetReactState(REACT_DEFENSIVE);
    */
    else
        SetReactState(REACT_AGGRESSIVE);
}

bool Creature::isCanInteractWithBattleMaster(Player* player, bool msg) const
{
    if (!IsBattleMaster())
        return false;

    BattlegroundTypeId bgTypeId = sBattlegroundMgr->GetBattleMasterBG(GetEntry());
    if (!msg)
        return player->GetBGAccessByLevel(bgTypeId);

    if (!player->GetBGAccessByLevel(bgTypeId))
    {
        ClearGossipMenuFor(player);
        switch (bgTypeId)
        {
            case BATTLEGROUND_AV:  SendGossipMenuFor(player, 7616, this); break;
            case BATTLEGROUND_WS:  SendGossipMenuFor(player, 7599, this); break;
            case BATTLEGROUND_AB:  SendGossipMenuFor(player, 7642, this); break;
            case BATTLEGROUND_EY:
            case BATTLEGROUND_NA:
            case BATTLEGROUND_BE:
            case BATTLEGROUND_AA:
            case BATTLEGROUND_RL:
            case BATTLEGROUND_SA:
            case BATTLEGROUND_DS:
            case BATTLEGROUND_RV:  SendGossipMenuFor(player, 10024, this); break;
            default: break;
        }
        return false;
    }
    return true;
}

bool Creature::CanResetTalents(Player* player) const
{
    return player->getLevel() >= 10
        && player->getClass() == GetCreatureTemplate()->trainer_class;
}

bool Creature::IsClassTrainerOf(Player const* player) const
{
    return player->getClass() == GetCreatureTemplate()->trainer_class;
}

Player* Creature::GetLootRecipient() const
{
    if (!m_lootRecipient)
        return nullptr;
    return ObjectAccessor::FindConnectedPlayer(m_lootRecipient);
}

Group* Creature::GetLootRecipientGroup() const
{
    if (!m_lootRecipientGroup)
        return nullptr;
    return sGroupMgr->GetGroupByGUID(m_lootRecipientGroup);
}

void Creature::SetLootRecipient(Unit* unit, bool withGroup)
{
    // set the player whose group should receive the right
    // to loot the creature after it dies
    // should be set to nullptr after the loot disappears

    if (!unit)
    {
        m_lootRecipient.Clear();
        m_lootRecipientGroup = 0;
        RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE|UNIT_DYNFLAG_TAPPED);
        return;
    }

    if (unit->GetTypeId() != TYPEID_PLAYER && !unit->IsVehicle())
        return;

    Player* player = unit->GetCharmerOrOwnerPlayerOrPlayerItself();
    if (!player)                                             // normal creature, no player involved
        return;

    m_lootRecipient = player->GetGUID();
    if (withGroup)
    {
        if (Group* group = player->GetGroup())
            m_lootRecipientGroup = group->GetLowGUID();
    }
    else
        m_lootRecipientGroup = ObjectGuid::Empty;

    SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED);
}

// return true if this creature is tapped by the player or by a member of his group.
bool Creature::isTappedBy(Player const* player) const
{
    if (player->GetGUID() == m_lootRecipient)
        return true;

    Group const* playerGroup = player->GetGroup();
    if (!playerGroup || playerGroup != GetLootRecipientGroup()) // if we dont have a group we arent the recipient
        return false;                                           // if creature doesnt have group bound it means it was solo killed by someone else

    return true;
}

void Creature::SaveToDB()
{
    // this should only be used when the creature has already been loaded
    // preferably after adding to map, because mapid may not be valid otherwise
    CreatureData const* data = sObjectMgr->GetCreatureData(m_spawnId);
    if (!data)
    {
        TC_LOG_ERROR("entities.unit", "Creature::SaveToDB failed, cannot get creature data!");
        return;
    }

    uint32 mapId = GetMapId();
    if (TransportBase* transport = GetTransport())
        if (transport->GetMapIdForSpawning() >= 0)
            mapId = transport->GetMapIdForSpawning();

    SaveToDB(mapId, data->spawnMask);
}

void Creature::SaveToDB(uint32 mapid, uint8 spawnMask)
{
    // update in loaded data
    if (!m_spawnId)
        m_spawnId = sObjectMgr->GenerateCreatureSpawnId();

    CreatureData& data = sObjectMgr->NewOrExistCreatureData(m_spawnId);

    uint32 displayId = GetNativeDisplayId();
    uint32 npcflag = GetUInt32Value(UNIT_NPC_FLAGS);
    uint32 unit_flags = GetUInt32Value(UNIT_FIELD_FLAGS);
    uint32 unit_flags2 = GetUInt32Value(UNIT_FIELD_FLAGS_2);

    // check if it's a custom model and if not, use 0 for displayId
    CreatureTemplate const* cinfo = GetCreatureTemplate();
    if (cinfo)
    {
        for (CreatureModel model : cinfo->Models)
            if (displayId && displayId == model.CreatureDisplayID)
                displayId = 0;

        if (npcflag == cinfo->npcflag)
            npcflag = 0;

        if (unit_flags == cinfo->unit_flags)
            unit_flags = 0;

        if (unit_flags2 == cinfo->unit_flags2)
            unit_flags2 = 0;
    }

    if (!data.spawnId)
        data.spawnId = m_spawnId;
    ASSERT(data.spawnId == m_spawnId);
    data.id = GetEntry();
    data.displayid = displayId;
    data.equipmentId = GetCurrentEquipmentId();
    if (!GetTransport())
    {
        data.mapId = GetMapId();
        data.spawnPoint.Relocate(this);
    }
    else
    {
        data.mapId = mapid;
        data.spawnPoint.Relocate(GetTransOffsetX(), GetTransOffsetY(), GetTransOffsetZ(), GetTransOffsetO());
    }
    data.spawntimesecs = m_respawnDelay;
    // prevent add data integrity problems
    data.wanderDistance = GetDefaultMovementType() == IDLE_MOTION_TYPE ? 0.0f : m_wanderDistance;
    data.currentwaypoint = 0;
    data.curhealth = GetHealth();
    data.curmana = GetPower(POWER_MANA);
    // prevent add data integrity problems
    data.movementType = !m_wanderDistance && GetDefaultMovementType() == RANDOM_MOTION_TYPE
        ? IDLE_MOTION_TYPE : GetDefaultMovementType();
    data.spawnMask = spawnMask;
    data.npcflag = npcflag;
    data.unit_flags = unit_flags;
    data.unit_flags2 = unit_flags2;
    if (!data.spawnGroupData)
        data.spawnGroupData = sObjectMgr->GetDefaultSpawnGroup();

    data.phaseId = GetDBPhase() > 0 ? GetDBPhase() : data.phaseId;
    data.phaseGroup = GetDBPhase() < 0 ? -GetDBPhase() : data.phaseGroup;

    // update in DB
    WorldDatabaseTransaction trans = WorldDatabase.BeginTransaction();

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CREATURE);
    stmt->setUInt32(0, m_spawnId);

    trans->Append(stmt);

    uint8 index = 0;

    stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_CREATURE);
    stmt->setUInt32(index++, m_spawnId);
    stmt->setUInt32(index++, GetEntry());
    stmt->setUInt16(index++, uint16(mapid));
    stmt->setUInt8(index++, spawnMask);
    stmt->setUInt32(index++, data.phaseId);
    stmt->setUInt32(index++, data.phaseGroup);
    stmt->setUInt32(index++, displayId);
    stmt->setInt32(index++, int32(GetCurrentEquipmentId()));
    stmt->setFloat(index++, GetPositionX());
    stmt->setFloat(index++, GetPositionY());
    stmt->setFloat(index++, GetPositionZ());
    stmt->setFloat(index++, GetOrientation());
    stmt->setUInt32(index++, m_respawnDelay);
    stmt->setFloat(index++, m_wanderDistance);
    stmt->setUInt32(index++, 0);
    stmt->setUInt32(index++, GetHealth());
    stmt->setUInt32(index++, GetPower(POWER_MANA));
    stmt->setUInt8(index++, uint8(GetDefaultMovementType()));
    stmt->setUInt32(index++, npcflag);
    stmt->setUInt32(index++, unit_flags);
    stmt->setUInt32(index++, unit_flags2);
    trans->Append(stmt);

    WorldDatabase.CommitTransaction(trans);
}

void Creature::SelectLevel()
{
    CreatureTemplate const* cInfo = GetCreatureTemplate();

    // level
    uint8 minlevel = std::min(cInfo->maxlevel, cInfo->minlevel);
    uint8 maxlevel = std::max(cInfo->maxlevel, cInfo->minlevel);
    uint8 level = minlevel == maxlevel ? minlevel : urand(minlevel, maxlevel);
    SetLevel(level);
}

void Creature::UpdateLevelDependantStats()
{
    CreatureTemplate const* cInfo = GetCreatureTemplate();
    uint32 rank = IsPet() ? 0 : cInfo->rank;
    CreatureBaseStats const* stats = sObjectMgr->GetCreatureBaseStats(getLevel(), cInfo->unit_class);

    // health
    float healthmod = _GetHealthMod(rank);

    uint32 basehp = stats->GenerateHealth(cInfo);
    uint32 health = uint32(basehp * healthmod);

    SetCreateHealth(health);
    SetMaxHealth(health);
    SetHealth(health);
    ResetPlayerDamageReq();

    SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, (float)health);

    // mana
    Powers powerType = CalculateDisplayPowerType();
    SetCreateMana(stats->BaseMana);
    SetStatPctModifier(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(powerType)), BASE_PCT, cInfo->ModMana * cInfo->ModManaExtra);
    RegisterPowerTypes();
    SetPowerType(powerType);

    switch (getClass())
    {
        case UNIT_CLASS_PALADIN:
        case UNIT_CLASS_MAGE:
            SetFullPower(POWER_MANA);
            break;
        default: // We don't set max power here, 0 makes power bar hidden
            break;
    }

    // damage
    float basedamage = stats->GenerateBaseDamage(cInfo);

    float weaponBaseMinDamage = basedamage;
    float weaponBaseMaxDamage = basedamage * 1.5f;

    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, weaponBaseMinDamage);
    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

    SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, weaponBaseMinDamage);
    SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

    SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, weaponBaseMinDamage);
    SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

    SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, stats->AttackPower);
    SetStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE, stats->RangedAttackPower);

    float armor = (float)stats->GenerateArmor(cInfo); /// @todo Why is this treated as uint32 when it's a float?
    SetStatFlatModifier(UNIT_MOD_ARMOR, BASE_VALUE, armor);
}

float Creature::_GetHealthMod(int32 Rank)
{
    switch (Rank)                                           // define rates for each elite rank
    {
        case CREATURE_ELITE_NORMAL:
            return sWorld->getRate(RATE_CREATURE_NORMAL_HP);
        case CREATURE_ELITE_ELITE:
            return sWorld->getRate(RATE_CREATURE_ELITE_ELITE_HP);
        case CREATURE_ELITE_RAREELITE:
            return sWorld->getRate(RATE_CREATURE_ELITE_RAREELITE_HP);
        case CREATURE_ELITE_WORLDBOSS:
            return sWorld->getRate(RATE_CREATURE_ELITE_WORLDBOSS_HP);
        case CREATURE_ELITE_RARE:
            return sWorld->getRate(RATE_CREATURE_ELITE_RARE_HP);
        default:
            return sWorld->getRate(RATE_CREATURE_ELITE_ELITE_HP);
    }
}

void Creature::LowerPlayerDamageReq(uint32 unDamage)
{
    if (m_PlayerDamageReq)
        m_PlayerDamageReq > unDamage ? m_PlayerDamageReq -= unDamage : m_PlayerDamageReq = 0;
}

float Creature::_GetDamageMod(int32 Rank)
{
    switch (Rank)                                           // define rates for each elite rank
    {
        case CREATURE_ELITE_NORMAL:
            return sWorld->getRate(RATE_CREATURE_NORMAL_DAMAGE);
        case CREATURE_ELITE_ELITE:
            return sWorld->getRate(RATE_CREATURE_ELITE_ELITE_DAMAGE);
        case CREATURE_ELITE_RAREELITE:
            return sWorld->getRate(RATE_CREATURE_ELITE_RAREELITE_DAMAGE);
        case CREATURE_ELITE_WORLDBOSS:
            return sWorld->getRate(RATE_CREATURE_ELITE_WORLDBOSS_DAMAGE);
        case CREATURE_ELITE_RARE:
            return sWorld->getRate(RATE_CREATURE_ELITE_RARE_DAMAGE);
        default:
            return sWorld->getRate(RATE_CREATURE_ELITE_ELITE_DAMAGE);
    }
}

float Creature::GetSpellDamageMod(int32 Rank) const
{
    switch (Rank)                                           // define rates for each elite rank
    {
        case CREATURE_ELITE_NORMAL:
            return sWorld->getRate(RATE_CREATURE_NORMAL_SPELLDAMAGE);
        case CREATURE_ELITE_ELITE:
            return sWorld->getRate(RATE_CREATURE_ELITE_ELITE_SPELLDAMAGE);
        case CREATURE_ELITE_RAREELITE:
            return sWorld->getRate(RATE_CREATURE_ELITE_RAREELITE_SPELLDAMAGE);
        case CREATURE_ELITE_WORLDBOSS:
            return sWorld->getRate(RATE_CREATURE_ELITE_WORLDBOSS_SPELLDAMAGE);
        case CREATURE_ELITE_RARE:
            return sWorld->getRate(RATE_CREATURE_ELITE_RARE_SPELLDAMAGE);
        default:
            return sWorld->getRate(RATE_CREATURE_ELITE_ELITE_SPELLDAMAGE);
    }
}

bool Creature::CreateFromProto(ObjectGuid::LowType guidlow, uint32 entry, CreatureData const* data /*= nullptr*/, uint32 vehId /*= 0*/)
{
    SetZoneScript();
    if (GetZoneScript() && data)
    {
        entry = GetZoneScript()->GetCreatureEntry(guidlow, data);
        if (!entry)
            return false;
    }

    CreatureTemplate const* cinfo = sObjectMgr->GetCreatureTemplate(entry);
    if (!cinfo)
    {
        TC_LOG_ERROR("sql.sql", "Creature::CreateFromProto(): creature template (guidlow: %u, entry: %u) does not exist.", guidlow, entry);
        return false;
    }

    SetOriginalEntry(entry);

    Object::_Create(guidlow, entry, (vehId || cinfo->VehicleId) ? HighGuid::Vehicle : HighGuid::Unit);

    if (!UpdateEntry(entry, data))
        return false;

    if (!vehId)
    {
        if (GetCreatureTemplate()->VehicleId)
        {
            vehId = GetCreatureTemplate()->VehicleId;
            entry = GetCreatureTemplate()->Entry;
        }
        else
            vehId = cinfo->VehicleId;
    }

    if (vehId)
        if (CreateVehicleKit(vehId, entry, true))
            UpdateDisplayPower();

    return true;
}

bool Creature::LoadFromDB(ObjectGuid::LowType spawnId, Map* map, bool addToMap, bool allowDuplicate)
{
    if (!allowDuplicate)
    {
        // If an alive instance of this spawnId is already found, skip creation
        // If only dead instance(s) exist, despawn them and spawn a new (maybe also dead) version
        const auto creatureBounds = map->GetCreatureBySpawnIdStore().equal_range(spawnId);
        std::vector <Creature*> despawnList;

        if (creatureBounds.first != creatureBounds.second)
        {
            for (auto itr = creatureBounds.first; itr != creatureBounds.second; ++itr)
            {
                if (itr->second->IsAlive())
                {
                    TC_LOG_DEBUG("maps", "Would have spawned %u but %s already exists", spawnId, creatureBounds.first->second->GetGUID().ToString().c_str());
                    return false;
                }
                else
                {
                    despawnList.push_back(itr->second);
                    TC_LOG_DEBUG("maps", "Despawned dead instance of spawn %u (%s)", spawnId, itr->second->GetGUID().ToString().c_str());
                }
            }

            for (Creature* despawnCreature : despawnList)
            {
                despawnCreature->AddObjectToRemoveList();
            }
        }
    }

    CreatureData const* data = sObjectMgr->GetCreatureData(spawnId);

    if (!data)
    {
        TC_LOG_ERROR("sql.sql", "Creature (SpawnID %u) not found in table `creature`, can't load. ", spawnId);
        return false;
    }

    m_spawnId = spawnId;

    m_respawnCompatibilityMode = ((data->spawnGroupData->flags & SPAWNGROUP_FLAG_COMPATIBILITY_MODE) != 0);
    m_creatureData = data;
    m_wanderDistance = data->wanderDistance;
    m_respawnDelay = data->spawntimesecs;

    if (!Create(map->GenerateLowGuid<HighGuid::Unit>(), map, data->id, data->spawnPoint, data, 0U, !m_respawnCompatibilityMode))
        return false;

    //We should set first home position, because then AI calls home movement
    SetHomePosition(*this);

    m_deathState = ALIVE;

    m_respawnTime = GetMap()->GetCreatureRespawnTime(m_spawnId);

    if (!m_respawnTime && !map->IsSpawnGroupActive(data->spawnGroupData->groupId))
    {
        if (!m_respawnCompatibilityMode)
        {
            // @todo pools need fixing! this is just a temporary thing, but they violate dynspawn principles
            if (!data->poolId)
            {
                TC_LOG_ERROR("entities.unit", "Creature (SpawnID %u) trying to load in inactive spawn group '%s':\n", spawnId, data->spawnGroupData->name.c_str());
                return false;
            }
        }

        m_respawnTime = GameTime::GetGameTime() + urand(4, 7);
    }

    if (m_respawnTime)
    {
        if (!m_respawnCompatibilityMode)
        {
            // @todo same as above
            if (!data->poolId)
            {
                TC_LOG_ERROR("entities.unit", "Creature (SpawnID %u) trying to load despite a respawn timer in progress:\n", spawnId);
                return false;
            }
        }
        else
        {
            // compatibility mode creatures will be respawned in ::Update()
            m_deathState = DEAD;
        }

        if (CanFly())
        {
            float tz = map->GetHeight(GetPhaseShift(), data->spawnPoint, true, MAX_FALL_DISTANCE);
            if (data->spawnPoint.GetPositionZ() - tz > 0.1f && Trinity::IsValidMapCoord(tz))
                Relocate(data->spawnPoint.GetPositionX(), data->spawnPoint.GetPositionY(), tz);
        }
    }

    SetSpawnHealth();

    // checked at creature_template loading
    m_defaultMovementType = MovementGeneratorType(data->movementType);

    if (addToMap && !GetMap()->AddToMap(this))
        return false;
    return true;
}

void Creature::SetCanDualWield(bool value)
{
    Unit::SetCanDualWield(value);
    UpdateDamagePhysical(OFF_ATTACK);
}

void Creature::LoadEquipment(int8 id, bool force /*= true*/)
{
    if (id == 0)
    {
        if (force)
        {
            for (uint8 i = 0; i < MAX_EQUIPMENT_ITEMS; ++i)
                SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + i, 0);
            m_equipmentId = 0;
        }

        return;
    }

    EquipmentInfo const* einfo = sObjectMgr->GetEquipmentInfo(GetEntry(), id);
    if (!einfo)
        return;

    m_equipmentId = id;
    for (uint8 i = 0; i < MAX_EQUIPMENT_ITEMS; ++i)
        SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + i, einfo->ItemEntry[i]);
}

void Creature::SetSpawnHealth()
{
    uint32 curhealth;
    if (m_creatureData && !m_regenHealth)
    {
        curhealth = m_creatureData->curhealth;
        if (curhealth)
        {
            curhealth = uint32(curhealth*_GetHealthMod(GetCreatureTemplate()->rank));
            if (curhealth < 1)
                curhealth = 1;
        }
        SetPower(POWER_MANA, m_creatureData->curmana);
    }
    else
    {
        curhealth = GetMaxHealth();
        SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    }

    SetHealth((m_deathState == ALIVE || m_deathState == JUST_RESPAWNED) ? curhealth : 0);
}

bool Creature::hasQuest(uint32 quest_id) const
{
    return sObjectMgr->GetCreatureQuestRelations(GetEntry()).HasQuest(quest_id);
}

bool Creature::hasInvolvedQuest(uint32 quest_id) const
{
    return sObjectMgr->GetCreatureQuestInvolvedRelations(GetEntry()).HasQuest(quest_id);
}

/*static*/ bool Creature::DeleteFromDB(ObjectGuid::LowType spawnId)
{
    CreatureData const* data = sObjectMgr->GetCreatureData(spawnId);
    if (!data)
        return false;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    sMapMgr->DoForAllMapsWithMapId(data->mapId,
        [spawnId, trans](Map* map) -> void
        {
            // despawn all active creatures, and remove their respawns
            std::vector<Creature*> toUnload;
            for (auto const& pair : Trinity::Containers::MapEqualRange(map->GetCreatureBySpawnIdStore(), spawnId))
                toUnload.push_back(pair.second);
            for (Creature* creature : toUnload)
                map->AddObjectToRemoveList(creature);
            map->RemoveRespawnTime(SPAWN_TYPE_CREATURE, spawnId, trans);
        }
    );

    // delete data from memory ...
    sObjectMgr->DeleteCreatureData(spawnId);

    WorldDatabaseTransaction trans2 = WorldDatabase.BeginTransaction();

    // ... and the database
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CREATURE);
    stmt->setUInt32(0, spawnId);
    trans2->Append(stmt);

    stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_SPAWNGROUP_MEMBER);
    stmt->setUInt8(0, uint8(SPAWN_TYPE_CREATURE));
    stmt->setUInt32(1, spawnId);
    trans2->Append(stmt);

    stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CREATURE_ADDON);
    stmt->setUInt32(0, spawnId);
    trans2->Append(stmt);

    stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_GAME_EVENT_CREATURE);
    stmt->setUInt32(0, spawnId);
    trans2->Append(stmt);

    stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_GAME_EVENT_MODEL_EQUIP);
    stmt->setUInt32(0, spawnId);
    trans2->Append(stmt);

    WorldDatabase.CommitTransaction(trans2);

    return true;
}

bool Creature::IsInvisibleDueToDespawn() const
{
    if (Unit::IsInvisibleDueToDespawn())
        return true;

    if (IsAlive() || isDying() || m_corpseRemoveTime > GameTime::GetGameTime())
        return false;

    return true;
}

bool Creature::CanAlwaysSee(WorldObject const* obj) const
{
    if (IsAIEnabled() && AI()->CanSeeAlways(obj))
        return true;

    return false;
}

bool Creature::CanStartAttack(Unit const* who, bool force) const
{
    if (IsCivilian())
        return false;

    // This set of checks is should be done only for creatures
    if ((IsImmuneToNPC() && !who->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
        || (IsImmuneToPC() && who->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED)))
        return false;

    // Do not attack non-combat pets
    if (who->GetTypeId() == TYPEID_UNIT && who->GetCreatureType() == CREATURE_TYPE_NON_COMBAT_PET)
        return false;

    if (!CanFly() && (GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE + m_CombatDistance))
        //|| who->IsControlledByPlayer() && who->IsFlying()))
        // we cannot check flying for other creatures, too much map/vmap calculation
        /// @todo should switch to range attack
        return false;

    if (!force)
    {
        if (!_IsTargetAcceptable(who))
            return false;

        if (!force && (IsNeutralToAll() || !IsWithinDistInMap(who, GetAttackDistance(who) + m_CombatDistance)))
            return false;
    }

    if (!CanCreatureAttack(who, force))
        return false;

    // No aggro from gray creatures
    if (CheckNoGrayAggroConfig(who->getLevelForTarget(this), getLevelForTarget(who)))
        return false;

    return IsWithinLOSInMap(who);
}


bool Creature::CheckNoGrayAggroConfig(uint32 playerLevel, uint32 creatureLevel) const
{
    if (Trinity::XP::GetColorCode(playerLevel, creatureLevel) != XP_GRAY)
        return false;

    uint32 notAbove = sWorld->getIntConfig(CONFIG_NO_GRAY_AGGRO_ABOVE);
    uint32 notBelow = sWorld->getIntConfig(CONFIG_NO_GRAY_AGGRO_BELOW);
    if (notAbove == 0 && notBelow == 0)
        return false;

    if (playerLevel <= notBelow || (playerLevel >= notAbove && notAbove > 0))
        return true;
    return false;
}

float Creature::GetAttackDistance(Unit const* player) const
{
    // WoW Wiki: the minimum radius seems to be 5 yards, while the maximum range is 45 yards
    float maxRadius = (45.0f * sWorld->getRate(RATE_CREATURE_AGGRO));
    float minRadius = (5.0f * sWorld->getRate(RATE_CREATURE_AGGRO));
    float aggroRate = sWorld->getRate(RATE_CREATURE_AGGRO);
    uint8 expansionMaxLevel = uint8(DBCManager::GetMaxLevelForExpansion(GetCreatureTemplate()->expansion));

    uint32 playerLevel = player->getLevel();
    uint32 creatureLevel = getLevel();

    if (aggroRate == 0.0f)
        return 0.0f;

    // The aggro radius for creatures with equal level as the player is 15 yards.
    // The combatreach should not get taken into account for the distance so we drop it from the range (see Supremus as expample)
    float baseAggroDistance = 15.0f - GetCombatReach();
    float aggroRadius = baseAggroDistance;

    // detect range auras
    if ((creatureLevel + 5) <= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
    {
        aggroRadius += GetTotalAuraModifier(SPELL_AURA_MOD_DETECT_RANGE);

        aggroRadius += player->GetTotalAuraModifier(SPELL_AURA_MOD_DETECTED_RANGE);
    }

    // The aggro range of creatures with higher levels than the total player level for the expansion should get the maxlevel treatment
    // This makes sure that creatures such as bosses wont have a bigger aggro range than the rest of the npc's
    // The following code is used for blizzlike behavior such as skipable bosses (e.g. Commander Springvale at level 85)
    if (creatureLevel > expansionMaxLevel)
        aggroRadius += float(expansionMaxLevel) - float(playerLevel);
    // + - 1 yard for each level difference between player and creature
    else
        aggroRadius += float(creatureLevel) - float(playerLevel);

    // Make sure that we wont go over the total range limits
    if (aggroRadius > maxRadius)
        aggroRadius = maxRadius;
    else if (aggroRadius < minRadius)
        aggroRadius = minRadius;

    return (aggroRadius * aggroRate);
}

void Creature::setDeathState(DeathState s)
{
    Unit::setDeathState(s);

    if (s == JUST_DIED)
    {
        m_corpseRemoveTime = GameTime::GetGameTime();
        if (!HasStaticFlag(CREATURE_STATIC_FLAG_DESPAWN_INSTANTLY))
            m_corpseRemoveTime += m_corpseDelay;

        uint32 respawnDelay = m_respawnDelay;
        if (uint32 scalingMode = sWorld->getIntConfig(CONFIG_RESPAWN_DYNAMICMODE))
            GetMap()->ApplyDynamicModeRespawnScaling(this, m_spawnId, respawnDelay, scalingMode);

        // @todo remove the boss respawn time hack in a dynspawn follow-up once we have creature groups in instances
        if (m_respawnCompatibilityMode)
        {
            if (IsDungeonBoss() && !m_respawnDelay)
                m_respawnTime = std::numeric_limits<time_t>::max(); // never respawn in this instance
            else
                m_respawnTime = GameTime::GetGameTime() + respawnDelay + m_corpseDelay;
        }
        else
        {
            if (IsDungeonBoss() && !m_respawnDelay)
                m_respawnTime = std::numeric_limits<time_t>::max(); // never respawn in this instance
            else
                m_respawnTime = GameTime::GetGameTime() + respawnDelay;
        }

        SaveRespawnTime();

        ReleaseSpellFocus(nullptr, false);  // remove spellcast focus
        _spellFocusInfo.Reset();            // cancel pending reacquisition of targets post spell focusing.
        SetTarget(ObjectGuid::Empty);       // drop target - dead mobs shouldn't ever target things

        SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);

        SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 0); // if creature is mounted on a virtual mount, remove it at death

        setActive(false);

        SetNoSearchAssistance(false);

        //Dismiss group if is leader
        if (m_formation && m_formation->getLeader() == this)
            m_formation->FormationReset(true);

        bool needsFalling = (IsFlying() || IsHovering()) && !IsUnderWater() && !HasUnitState(UNIT_STATE_ROOT);
        SetHover(false, false);
        SetDisableGravity(false, false);

        if (needsFalling)
            GetMotionMaster()->MoveFall();

        Unit::setDeathState(CORPSE);
    }
    else if (s == JUST_RESPAWNED)
    {
        if (IsPet())
            SetFullHealth();
        else
            SetSpawnHealth();

        SetLootRecipient(nullptr);
        ResetPlayerDamageReq();

        SetCannotReachTarget(false);
        UpdateMovementCapabilities();

        ClearUnitState(UNIT_STATE_ALL_ERASABLE);

        if (!IsPet())
        {
            CreatureData const* creatureData = GetCreatureData();
            CreatureTemplate const* cinfo = GetCreatureTemplate();

            uint32 npcflag, unitFlags, unitFlags2;
            ObjectMgr::ChooseCreatureFlags(cinfo, &npcflag, &unitFlags, &unitFlags2, _staticFlags, creatureData);

            SetUInt32Value(UNIT_NPC_FLAGS, npcflag);
            SetUInt32Value(UNIT_FIELD_FLAGS, unitFlags);
            SetUInt32Value(UNIT_FIELD_FLAGS_2, unitFlags2);
            SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_NONE);

            RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

            SetMeleeDamageSchool(SpellSchools(cinfo->dmgschool));
        }

        Motion_Initialize();
        Unit::setDeathState(ALIVE);
        LoadCreaturesAddon();
    }
}

void Creature::Respawn(bool force)
{
    if (force)
    {
        if (IsAlive())
            setDeathState(JUST_DIED);
        else if (getDeathState() != CORPSE)
            setDeathState(CORPSE);
    }

    if (m_respawnCompatibilityMode)
    {
        UpdateObjectVisibilityOnDestroy();
        RemoveCorpse(false);

        if (getDeathState() == DEAD)
        {
            TC_LOG_DEBUG("entities.unit", "Respawning creature %s (%s)", GetName().c_str(), GetGUID().ToString().c_str());
            m_respawnTime = 0;
            ResetPickPocketRefillTimer();
            loot.clear();

            if (m_originalEntry != GetEntry())
                UpdateEntry(m_originalEntry);

            SelectLevel();

            setDeathState(JUST_RESPAWNED);

            CreatureModel display(GetNativeDisplayId(), 0, 1.0f);
            if (sObjectMgr->GetCreatureModelRandomGender(&display, GetCreatureTemplate()))
                SetDisplayId(display.CreatureDisplayID, true);

            GetMotionMaster()->InitDefault();
            //Re-initialize reactstate that could be altered by movementgenerators
            InitializeReactState();

            if (UnitAI* ai = AI()) // reset the AI to be sure no dirty or uninitialized values will be used till next tick
                ai->Reset();

            m_triggerJustAppeared = true;

            uint32 poolid = GetCreatureData() ? GetCreatureData()->poolId : 0;
            if (poolid)
                sPoolMgr->UpdatePool<Creature>(GetMap()->GetPoolData(), poolid, GetSpawnId());
        }
        UpdateObjectVisibility();
    }
    else
    {
        if (m_spawnId)
            GetMap()->Respawn(SPAWN_TYPE_CREATURE, m_spawnId);
    }

    TC_LOG_DEBUG("entities.unit", "Respawning creature %s (%s)",
        GetName().c_str(), GetGUID().ToString().c_str());
}

void Creature::ForcedDespawn(uint32 timeMSToDespawn, Seconds forceRespawnTimer)
{
    if (timeMSToDespawn)
    {
        m_Events.AddEvent(new ForcedDespawnDelayEvent(*this, forceRespawnTimer), m_Events.CalculateTime(timeMSToDespawn));
        return;
    }

    if (m_respawnCompatibilityMode)
    {
        uint32 corpseDelay = GetCorpseDelay();
        uint32 respawnDelay = GetRespawnDelay();

        // do it before killing creature
        UpdateObjectVisibilityOnDestroy();

        bool overrideRespawnTime = false;
        if (IsAlive())
        {
            if (forceRespawnTimer > Seconds::zero())
            {
                SetCorpseDelay(0);
                SetRespawnDelay(forceRespawnTimer.count());
                overrideRespawnTime = true;
            }

            setDeathState(JUST_DIED);
        }

        // Skip corpse decay time
        RemoveCorpse(!overrideRespawnTime, false);

        SetCorpseDelay(corpseDelay);
        SetRespawnDelay(respawnDelay);
    }
    else
    {
        if (forceRespawnTimer > Seconds::zero())
            SaveRespawnTime(forceRespawnTimer.count());
        else
        {
            uint32 respawnDelay = m_respawnDelay;
            if (uint32 scalingMode = sWorld->getIntConfig(CONFIG_RESPAWN_DYNAMICMODE))
                GetMap()->ApplyDynamicModeRespawnScaling(this, m_spawnId, respawnDelay, scalingMode);
            m_respawnTime = GameTime::GetGameTime() + respawnDelay;
            SaveRespawnTime();
        }

        AddObjectToRemoveList();
    }
}

void Creature::DespawnOrUnsummon(uint32 msTimeToDespawn /*= 0*/, Seconds forceRespawnTimer /*= 0*/)
{
    if (TempSummon* summon = ToTempSummon())
        summon->UnSummon(msTimeToDespawn);
    else
        ForcedDespawn(msTimeToDespawn, forceRespawnTimer);
}

void Creature::LoadTemplateImmunities()
{
    // uint32 max used for "spell id", the immunity system will not perform SpellInfo checks against invalid spells
    // used so we know which immunities were loaded from template
    static uint32 const placeholderSpellId = std::numeric_limits<uint32>::max();

    // unapply template immunities (in case we're updating entry)
    for (uint32 i = MECHANIC_NONE + 1; i < MAX_MECHANIC; ++i)
        ApplySpellImmune(placeholderSpellId, IMMUNITY_MECHANIC, i, false);

    for (uint32 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        ApplySpellImmune(placeholderSpellId, IMMUNITY_SCHOOL, 1 << i, false);

    // don't inherit immunities for hunter pets
    if (GetOwnerOrCreatorGUID().IsPlayer() && IsHunterPet())
        return;

    if (uint32 mask = GetCreatureTemplate()->MechanicImmuneMask)
    {
        for (uint32 i = MECHANIC_NONE + 1; i < MAX_MECHANIC; ++i)
        {
            if (mask & (1 << (i - 1)))
                ApplySpellImmune(placeholderSpellId, IMMUNITY_MECHANIC, i, true);
        }
    }

    if (uint32 mask = GetCreatureTemplate()->SpellSchoolImmuneMask)
    {
        for (uint8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        {
            if (mask & (1 << i))
                ApplySpellImmune(placeholderSpellId, IMMUNITY_SCHOOL, 1 << i, true);
        }
    }
}

bool Creature::IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index, WorldObject const* caster,
    bool requireImmunityPurgesEffectAttribute /*= false*/) const
{
    if (!spellInfo->Effects[index].IsEffect())
        return true;

    if (GetCreatureTemplate()->type == CREATURE_TYPE_MECHANICAL && spellInfo->Effects[index].Effect == SPELL_EFFECT_HEAL)
        return true;

    return Unit::IsImmunedToSpellEffect(spellInfo, index, caster, requireImmunityPurgesEffectAttribute);
}

bool Creature::isElite() const
{
    if (IsPet())
        return false;

    uint32 rank = GetCreatureTemplate()->rank;
    return rank != CREATURE_ELITE_NORMAL && rank != CREATURE_ELITE_RARE;
}

bool Creature::isWorldBoss() const
{
    if (IsPet())
        return false;

    return (GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_BOSS_MOB) != 0;
}

SpellInfo const* Creature::reachWithSpellAttack(Unit* victim)
{
    if (!victim)
        return nullptr;

    for (uint32 i=0; i < MAX_CREATURE_SPELLS; ++i)
    {
        if (!m_spells[i])
            continue;
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(m_spells[i]);
        if (!spellInfo)
        {
            TC_LOG_ERROR("entities.unit", "WORLD: unknown spell id %i", m_spells[i]);
            continue;
        }

        bool bcontinue = true;
        for (uint32 j = 0; j < MAX_SPELL_EFFECTS; j++)
        {
            if ((spellInfo->Effects[j].Effect == SPELL_EFFECT_SCHOOL_DAMAGE)       ||
                (spellInfo->Effects[j].Effect == SPELL_EFFECT_INSTAKILL)            ||
                (spellInfo->Effects[j].Effect == SPELL_EFFECT_ENVIRONMENTAL_DAMAGE) ||
                (spellInfo->Effects[j].Effect == SPELL_EFFECT_HEALTH_LEECH)
                )
            {
                bcontinue = false;
                break;
            }
        }
        if (bcontinue)
            continue;

        if (spellInfo->ManaCost > (uint32)GetPower(POWER_MANA))
            continue;
        float range = spellInfo->GetMaxRange(false);
        float minrange = spellInfo->GetMinRange(false);
        float dist = GetDistance(victim);
        if (dist > range || dist < minrange)
            continue;
        if (spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
            continue;
        if (spellInfo->PreventionType == SPELL_PREVENTION_TYPE_PACIFY && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
            continue;
        return spellInfo;
    }
    return nullptr;
}

SpellInfo const* Creature::reachWithSpellCure(Unit* victim)
{
    if (!victim)
        return nullptr;

    for (uint32 i=0; i < MAX_CREATURE_SPELLS; ++i)
    {
        if (!m_spells[i])
            continue;
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(m_spells[i]);
        if (!spellInfo)
        {
            TC_LOG_ERROR("entities.unit", "WORLD: unknown spell id %i", m_spells[i]);
            continue;
        }

        bool bcontinue = true;
        for (uint32 j = 0; j < MAX_SPELL_EFFECTS; j++)
        {
            if ((spellInfo->Effects[j].Effect == SPELL_EFFECT_HEAL))
            {
                bcontinue = false;
                break;
            }
        }
        if (bcontinue)
            continue;

        if (spellInfo->ManaCost > (uint32)GetPower(POWER_MANA))
            continue;

        float range = spellInfo->GetMaxRange(true);
        float minrange = spellInfo->GetMinRange(true);
        float dist = GetDistance(victim);
        //if (!isInFront(victim, range) && spellInfo->AttributesEx)
        //    continue;
        if (dist > range || dist < minrange)
            continue;
        if (spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
            continue;
        if (spellInfo->PreventionType == SPELL_PREVENTION_TYPE_PACIFY && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
            continue;
        return spellInfo;
    }
    return nullptr;
}

// select nearest hostile unit within the given distance (regardless of threat list).
Unit* Creature::SelectNearestTarget(float dist, bool playerOnly /* = false */) const
{
    if (dist == 0.0f)
        dist = MAX_VISIBILITY_DISTANCE;

    Unit* target = nullptr;
    Trinity::NearestHostileUnitCheck u_check(this, dist, playerOnly);
    Trinity::UnitLastSearcher<Trinity::NearestHostileUnitCheck> searcher(this, target, u_check);
    Cell::VisitAllObjects(this, searcher, dist);
    return target;
}

// select nearest hostile unit within the given attack distance (i.e. distance is ignored if > than ATTACK_DISTANCE), regardless of threat list.
Unit* Creature::SelectNearestTargetInAttackDistance(float dist) const
{
    if (dist > MAX_VISIBILITY_DISTANCE)
    {
        TC_LOG_ERROR("entities.unit", "Creature (GUID: %u Entry: %u) SelectNearestTargetInAttackDistance called with dist > MAX_VISIBILITY_DISTANCE. Distance set to ATTACK_DISTANCE.", GetGUID().GetCounter(), GetEntry());
        dist = ATTACK_DISTANCE;
    }

    Unit* target = nullptr;
    Trinity::NearestHostileUnitInAttackDistanceCheck u_check(this, dist);
    Trinity::UnitLastSearcher<Trinity::NearestHostileUnitInAttackDistanceCheck> searcher(this, target, u_check);
    Cell::VisitAllObjects(this, searcher, std::max(dist, ATTACK_DISTANCE));
    return target;
}

void Creature::SendAIReaction(AiReaction reactionType)
{
    WorldPackets::Combat::AIReaction packet;
    packet.UnitGUID = GetGUID();
    packet.Reaction = reactionType;
    SendMessageToSet(packet.Write(), false);

    TC_LOG_DEBUG("network", "WORLD: Sent SMSG_AI_REACTION, type %u.", reactionType);
}

void Creature::CallAssistance()
{
    if (!m_AlreadyCallAssistance && GetVictim() && !IsPet() && !IsCharmed())
    {
        SetNoCallAssistance(true);

        float radius = sWorld->getFloatConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_RADIUS);

        if (radius > 0)
        {
            std::list<Creature*> assistList;
            Trinity::AnyAssistCreatureInRangeCheck u_check(this, GetVictim(), radius);
            Trinity::CreatureListSearcher<Trinity::AnyAssistCreatureInRangeCheck> searcher(this, assistList, u_check);
            Cell::VisitGridObjects(this, searcher, radius);

            if (!assistList.empty())
            {
                AssistDelayEvent* e = new AssistDelayEvent(EnsureVictim()->GetGUID(), *this);
                while (!assistList.empty())
                {
                    // Pushing guids because in delay can happen some creature gets despawned => invalid pointer
                    e->AddAssistant((*assistList.begin())->GetGUID());
                    assistList.pop_front();
                }
                m_Events.AddEvent(e, m_Events.CalculateTime(sWorld->getIntConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_DELAY)));
            }
        }
    }
}

void Creature::CallForHelp(float radius)
{
    if (radius <= 0.0f || !IsEngaged() || !IsAlive() || IsPet() || IsCharmed())
        return;

    Unit* target = GetThreatManager().GetCurrentVictim();
    if (!target)
        target = GetThreatManager().GetAnyTarget();
    if (!target)
        target = GetCombatManager().GetAnyTarget();

    if (!target)
    {
        TC_LOG_ERROR("entities.unit", "Creature %u (%s) trying to call for help without being in combat.", GetEntry(), GetName().c_str());
        return;
    }

    Trinity::CallOfHelpCreatureInRangeDo u_do(this, target, radius);
    Trinity::CreatureWorker<Trinity::CallOfHelpCreatureInRangeDo> worker(this, u_do);
    Cell::VisitGridObjects(this, worker, radius);
}

bool Creature::CanAssistTo(const Unit* u, const Unit* enemy, bool checkfaction /*= true*/) const
{
    // is it true?
    if (!HasReactState(REACT_AGGRESSIVE))
        return false;

    // we don't need help from zombies :)
    if (!IsAlive())
        return false;

    // we cannot assist in evade mode
    if (IsInEvadeMode())
        return false;

    // or if enemy is in evade mode
    if (enemy->GetTypeId() == TYPEID_UNIT && enemy->ToCreature()->IsInEvadeMode())
        return false;

    // we don't need help from non-combatant ;)
    if (IsCivilian())
        return false;

    if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE) || IsImmuneToNPC())
        return false;

    // skip fighting creature
    if (IsEngaged())
        return false;

    // only free creature
    if (GetCharmerOrOwnerGUID())
        return false;

    // only from same creature faction
    if (checkfaction)
    {
        if (GetFaction() != u->GetFaction())
            return false;
    }
    else
    {
        if (!IsFriendlyTo(u))
            return false;
    }

    // skip non hostile to caster enemy creatures
    if (!IsHostileTo(enemy))
        return false;

    return true;
}

// use this function to avoid having hostile creatures attack
// friendlies and other mobs they shouldn't attack
bool Creature::_IsTargetAcceptable(Unit const* target) const
{
    ASSERT(target);

    // if the target cannot be attacked, the target is not acceptable
    if (IsFriendlyTo(target)
        || !target->isTargetableForAttack(false)
        || (m_vehicle && (IsOnVehicle(target) || m_vehicle->GetBase()->IsOnVehicle(target))))
        return false;

    if (target->HasUnitState(UNIT_STATE_DIED))
    {
        // some creatures can detect fake death
        if (IsIgnoringFeignDeath() && target->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH))
            return true;
        else
            return false;
    }

    // if I'm already fighting target, or I'm hostile towards the target, the target is acceptable
    if (IsEngagedBy(target) || IsHostileTo(target))
        return true;

    // if the target's victim is not friendly, or the target is friendly, the target is not acceptable
    return false;
}

void Creature::SaveRespawnTime(uint32 forceDelay)
{
    if (IsSummon() || !m_spawnId || (m_creatureData && !m_creatureData->dbData))
        return;

    if (m_respawnCompatibilityMode)
    {
        RespawnInfo ri;
        ri.type = SPAWN_TYPE_CREATURE;
        ri.spawnId = m_spawnId;
        ri.respawnTime = m_respawnTime;
        GetMap()->SaveRespawnInfoDB(ri);
        return;
    }

    time_t thisRespawnTime = forceDelay ? GameTime::GetGameTime() + forceDelay : m_respawnTime;
   GetMap()->SaveRespawnTime(SPAWN_TYPE_CREATURE, m_spawnId, GetOriginalEntry(), thisRespawnTime, Trinity::ComputeGridCoord(GetHomePosition().GetPositionX(), GetHomePosition().GetPositionY()).GetId());
}

// this should not be called by petAI or
bool Creature::CanCreatureAttack(Unit const* victim, bool /*force*/) const
{
    if (!victim->IsInMap(this))
        return false;

    if (!IsValidAttackTarget(victim))
        return false;

    if (!victim->isInAccessiblePlaceFor(this))
        return false;

    if (CreatureAI* ai = AI())
        if (!ai->CanAIAttack(victim))
            return false;

    // we cannot attack in evade mode
    if (IsInEvadeMode())
        return false;

    // or if enemy is in evade mode
    if (victim->GetTypeId() == TYPEID_UNIT && victim->ToCreature()->IsInEvadeMode())
        return false;

    if (!GetCharmerOrOwnerGUID().IsPlayer())
    {
        if (GetMap()->IsDungeon())
            return true;

        // don't check distance to home position if recently damaged, this should include taunt auras
        if (!isWorldBoss() && (GetLastDamagedTime() > GameTime::GetGameTime() || HasAuraType(SPELL_AURA_MOD_TAUNT)))
            return true;
    }

    // Map visibility range, but no more than 2*cell size
    float dist = std::min<float>(GetMap()->GetVisibilityRange(), SIZE_OF_GRID_CELL * 2);

    if (Unit* unit = GetCharmerOrOwner())
        return victim->IsWithinDist(unit, dist);
    else
    {
        // include sizes for huge npcs
        dist += GetCombatReach() + victim->GetCombatReach();

        // to prevent creatures in air ignore attacks because distance is already too high...
        if (CanFly())
            return victim->IsInDist2d(&m_homePosition, dist);
        else
            return victim->IsInDist(&m_homePosition, dist);
    }
}

CreatureAddon const* Creature::GetCreatureAddon() const
{
    if (m_spawnId)
    {
        if (CreatureAddon const* addon = sObjectMgr->GetCreatureAddon(m_spawnId))
            return addon;
    }

    // dependent from difficulty mode entry
    return sObjectMgr->GetCreatureTemplateAddon(GetCreatureTemplate()->Entry);
}

//creature_addon table
bool Creature::LoadCreaturesAddon()
{
    CreatureAddon const* creatureAddon = GetCreatureAddon();
    if (!creatureAddon)
        return false;

    if (uint32 mountDisplayId = _defaultMountDisplayIdOverride.value_or(creatureAddon->mount); mountDisplayId != 0)
        Mount(mountDisplayId);

    if (creatureAddon->mount != 0)
        Mount(creatureAddon->mount);

    // UNIT_FIELD_BYTES_1 values
    SetStandState(UnitStandStateType(creatureAddon->standState));
    SetAnimTier(AnimTier(creatureAddon->animTier));
    SetByteValue(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_VIS_FLAG, creatureAddon->visFlags);

    // UNIT_FIELD_BYTES_2 values
    SetSheath(SheathState(creatureAddon->sheathState));
    SetByteValue(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_PVP_FLAG, creatureAddon->pvpFlags);

    // These fields must only be handled by core internals and must not be modified via scripts/DB data
    SetByteValue(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_PET_FLAGS, 0);
    SetShapeshiftForm(FORM_NONE);

    if (creatureAddon->emote != 0)
        SetUInt32Value(UNIT_NPC_EMOTESTATE, creatureAddon->emote);

    SetAIAnimKitId(creatureAddon->aiAnimKit);
    SetMovementAnimKitId(creatureAddon->movementAnimKit);
    SetMeleeAnimKitId(creatureAddon->meleeAnimKit);

    // Check if visibility distance different
    if (creatureAddon->visibilityDistanceType != VisibilityDistanceType::Normal)
        SetVisibilityDistanceOverride(creatureAddon->visibilityDistanceType);

    // Load pathing data
    if (creatureAddon->waypointPathId)
        _waypointPathId = creatureAddon->waypointPathId;

    if (creatureAddon->cyclicSplinePathId)
        _cyclicSplinePathId = creatureAddon->cyclicSplinePathId;

    if (!creatureAddon->auras.empty())
    {
        for (std::vector<uint32>::const_iterator itr = creatureAddon->auras.begin(); itr != creatureAddon->auras.end(); ++itr)
        {
            SpellInfo const* AdditionalSpellInfo = sSpellMgr->GetSpellInfo(*itr);
            if (!AdditionalSpellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Creature (GUID: %u Entry: %u) has wrong spell %u defined in `auras` field.", GetGUID().GetCounter(), GetEntry(), *itr);
                continue;
            }

            // skip already applied aura
            if (HasAura(*itr))
                continue;

            AddAura(*itr, this);
            TC_LOG_DEBUG("entities.unit", "Spell: %u added to creature (GUID: %u Entry: %u)", *itr, GetGUID().GetCounter(), GetEntry());
        }
    }

    return true;
}

/// Send a message to LocalDefense channel for players opposition team in the zone
void Creature::SendZoneUnderAttackMessage(Player* attacker)
{
    uint32 enemy_team = attacker->GetTeam();

    WorldPackets::Misc::ZoneUnderAttack packet;
    packet.AreaID = GetAreaId();
    sWorld->SendGlobalMessage(packet.Write(), nullptr, (enemy_team == ALLIANCE ? HORDE : ALLIANCE));
}


bool Creature::HasSpell(uint32 spellID) const
{
    return std::find(std::begin(m_spells), std::end(m_spells), spellID) != std::end(m_spells);
}

time_t Creature::GetRespawnTimeEx() const
{
    time_t now = GameTime::GetGameTime();
    if (m_respawnTime > now)
        return m_respawnTime;
    else
        return now;
}

void Creature::SetRespawnTime(uint32 respawn)
{
    m_respawnTime = respawn ? GameTime::GetGameTime() + respawn : 0;
}

void Creature::GetRespawnPosition(float &x, float &y, float &z, float* ori, float* dist) const
{
    if (m_creatureData)
    {
        if (ori)
            m_creatureData->spawnPoint.GetPosition(x, y, z, *ori);
        else
            m_creatureData->spawnPoint.GetPosition(x, y, z);

        if (dist)
            *dist = m_creatureData->wanderDistance;
    }
    else
    {
        Position const& homePos = GetHomePosition();
        if (ori)
            homePos.GetPosition(x, y, z, *ori);
        else
            homePos.GetPosition(x, y, z);
        if (dist)
            *dist = 0;
    }
}

void Creature::InitializeMovementCapabilities()
{
    SetHover(GetMovementTemplate().IsHoverInitiallyEnabled());
    SetDisableGravity(IsFloating());
    SetControlled(IsSessile(), UNIT_STATE_ROOT);

    if (IsSwimPrevented())
    {
        SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_AI_WILL_ONLY_SWIM_IF_TARGET_SWIMS);
        SetSwim(false);
    }
    else
        RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_AI_WILL_ONLY_SWIM_IF_TARGET_SWIMS);

    UpdateMovementCapabilities();
}

void Creature::UpdateMovementCapabilities()
{
    // Do not update movement flags if creature is controlled by a player (charm/vehicle)
    if (IsMovedByClient())
        return;

    // Set the movement flags if the creature is in that mode. (Only fly if actually in air, only swim if in water, etc)
    float ground = GetFloorZ();
    bool isInAir = (G3D::fuzzyGt(GetPositionZ(), ground + GetHoverOffset() + GROUND_HEIGHT_TOLERANCE) || G3D::fuzzyLt(GetPositionZ(), ground - GROUND_HEIGHT_TOLERANCE)); // Can be underground too, prevent the falling
    if (!isInAir)
        RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING);

    if (HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_AI_WILL_ONLY_SWIM_IF_TARGET_SWIMS))
        if (GetVictim() && GetVictim()->IsInWater() && !GetVictim()->IsOnOceanFloor())
            RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_AI_WILL_ONLY_SWIM_IF_TARGET_SWIMS);

    if (IsInWater() && CanSwim())
        SetSwim(true);
    else if (!IsInWater()) // We do not want to disable swimming again when a creature is in water - may to lead some nasty bugs
        SetSwim(false);
}

CreatureMovementData const& Creature::GetMovementTemplate() const
{
    if (CreatureMovementData const* movementOverride = sObjectMgr->GetCreatureMovementOverride(m_spawnId))
        return *movementOverride;

    return GetCreatureTemplate()->Movement;
}

bool Creature::CanSwim() const
{
    if (Unit::CanSwim())
        return true;

    if (IsPet())
        return true;

    return false;
}

void Creature::InitializeCreatureMovementInfo(uint32 movementId)
{
    if (CreatureMovementInfoOverride const* movementInfoOverride = sObjectMgr->GetCreatureMovementInfoOverride(movementId))
    {
        if (movementInfoOverride->RunSpeed > 0.f)
        {
            _creatureMovementInfo.RunSpeed = movementInfoOverride->RunSpeed;
            _creatureMovementInfo.HasRunSpeedOverriden = true;
        }

        if (movementInfoOverride->WalkSpeed > 0.f)
        {
            _creatureMovementInfo.WalkSpeed = movementInfoOverride->WalkSpeed;
            _creatureMovementInfo.HasWalkSpeedOverriden = true;
        }
    }
}

void Creature::InitializeMovementSpeeds()
{
    // This segment needs to be removed once model based speeds are implemented and movementIds fully added.
    SetSpeedRate(MOVE_WALK,   m_creatureInfo->speed_walk);
    SetSpeedRate(MOVE_RUN,    m_creatureInfo->speed_run);
    SetSpeedRate(MOVE_SWIM,   1.0f); // using 1.0 rate
    SetSpeedRate(MOVE_FLIGHT, 1.0f); // using 1.0 rate

    // Todo: movement speeds by model

    // Overridden movement speed values by movementId data
    if (_creatureMovementInfo.HasRunSpeedOverriden)
        SetSpeed(MOVE_RUN, _creatureMovementInfo.RunSpeed);

    if (_creatureMovementInfo.HasWalkSpeedOverriden)
        SetSpeed(MOVE_WALK, _creatureMovementInfo.WalkSpeed);
}

void Creature::AllLootRemovedFromCorpse()
{
    if (loot.loot_type != LOOT_SKINNING && !IsPet() && GetCreatureTemplate()->SkinLootId && hasLootRecipient())
        if (LootTemplates_Skinning.HaveLootFor(GetCreatureTemplate()->SkinLootId))
            SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

    time_t now = GameTime::GetGameTime();
    // Do not reset corpse remove time if corpse is already removed
    if (m_corpseRemoveTime <= now)
        return;

    float decayRate = sWorld->getRate(RATE_CORPSE_DECAY_LOOTED);

    // corpse skinnable, but without skinning flag, and then skinned, corpse will despawn next update
    if (loot.loot_type == LOOT_SKINNING)
        m_corpseRemoveTime = now;
    else
        m_corpseRemoveTime = now + uint32(m_corpseDelay * decayRate);

    m_respawnTime = std::max<time_t>(m_corpseRemoveTime + m_respawnDelay, m_respawnTime);
}

uint8 Creature::getLevelForTarget(WorldObject const* target) const
{
    if (!isWorldBoss() || !target->ToUnit())
        return Unit::getLevelForTarget(target);

    uint16 level = target->ToUnit()->getLevel() + sWorld->getIntConfig(CONFIG_WORLD_BOSS_LEVEL_DIFF);
    if (level < 1)
        return 1;
    if (level > 255)
        return 255;
    return uint8(level);
}

std::string const& Creature::GetAIName() const
{
    return sObjectMgr->GetCreatureTemplate(GetEntry())->AIName;
}

std::string Creature::GetScriptName() const
{
    return sObjectMgr->GetScriptName(GetScriptId());
}

uint32 Creature::GetScriptId() const
{
    if (CreatureData const* creatureData = GetCreatureData())
        if (uint32 scriptId = creatureData->scriptId)
            return scriptId;

    return sObjectMgr->GetCreatureTemplate(GetEntry())->ScriptID;
}

VendorItemData const* Creature::GetVendorItems() const
{
    return sObjectMgr->GetNpcVendorItemList(GetEntry());
}

uint32 Creature::GetVendorItemCurrentCount(VendorItem const* vItem)
{
    if (!vItem->maxcount)
        return vItem->maxcount;

    VendorItemCounts::iterator itr = m_vendorItemCounts.begin();
    for (; itr != m_vendorItemCounts.end(); ++itr)
        if (itr->itemId == vItem->item)
            break;

    if (itr == m_vendorItemCounts.end())
        return vItem->maxcount;

    VendorItemCount* vCount = &*itr;

    time_t ptime = GameTime::GetGameTime();

    if (time_t(vCount->lastIncrementTime + vItem->incrtime) <= ptime)
        if (ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(vItem->item))
        {
            uint32 diff = uint32((ptime - vCount->lastIncrementTime)/vItem->incrtime);
            if ((vCount->count + diff * pProto->GetBuyCount()) >= vItem->maxcount)
            {
                m_vendorItemCounts.erase(itr);
                return vItem->maxcount;
            }

            vCount->count += diff * pProto->GetBuyCount();
            vCount->lastIncrementTime = ptime;
        }

    return vCount->count;
}

uint32 Creature::UpdateVendorItemCurrentCount(VendorItem const* vItem, uint32 used_count)
{
    if (!vItem->maxcount)
        return 0;

    VendorItemCounts::iterator itr = m_vendorItemCounts.begin();
    for (; itr != m_vendorItemCounts.end(); ++itr)
        if (itr->itemId == vItem->item)
            break;

    if (itr == m_vendorItemCounts.end())
    {
        uint32 new_count = vItem->maxcount > used_count ? vItem->maxcount-used_count : 0;
        m_vendorItemCounts.emplace_back(VendorItemCount(vItem->item, new_count));
        return new_count;
    }

    VendorItemCount* vCount = &*itr;

    time_t ptime = GameTime::GetGameTime();

    if (time_t(vCount->lastIncrementTime + vItem->incrtime) <= ptime)
        if (ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(vItem->item))
        {
            uint32 diff = uint32((ptime - vCount->lastIncrementTime)/vItem->incrtime);
            if ((vCount->count + diff * pProto->GetBuyCount()) < vItem->maxcount)
                vCount->count += diff * pProto->GetBuyCount();
            else
                vCount->count = vItem->maxcount;
        }

    vCount->count = vCount->count > used_count ? vCount->count-used_count : 0;
    vCount->lastIncrementTime = ptime;
    return vCount->count;
}

// overwrite WorldObject function for proper name localization
std::string const & Creature::GetNameForLocaleIdx(LocaleConstant loc_idx) const
{
    if (loc_idx != DEFAULT_LOCALE)
    {
        uint8 uloc_idx = uint8(loc_idx);
        CreatureLocale const* cl = sObjectMgr->GetCreatureLocale(GetEntry());
        if (cl)
        {
            if (cl->Name.size() > uloc_idx && !cl->Name[uloc_idx].empty())
                return cl->Name[uloc_idx];
        }
    }

    return GetName();
}

uint32 Creature::GetPetAutoSpellOnPos(uint8 pos) const
{
    if (pos >= MAX_SPELL_CHARM || !m_charmInfo || m_charmInfo->GetCharmSpell(pos)->GetType() != ACT_ENABLED)
        return 0;
    else
        return m_charmInfo->GetCharmSpell(pos)->GetAction();
}

float Creature::GetPetChaseDistance() const
{
    float range = 0.f;

    for (uint8 i = 0; i < GetPetAutoSpellSize(); ++i)
    {
        uint32 spellID = GetPetAutoSpellOnPos(i);
        if (!spellID)
            continue;

        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellID))
            if (spellInfo->GetRecoveryTime() == 0 && spellInfo->RangeEntry->ID != 1 /*Self*/ && spellInfo->RangeEntry->ID != 2 /*Combat Range*/ && spellInfo->GetMaxRange() > range)
                range = spellInfo->GetMaxRange();
    }

    return range;
}

void Creature::SetPosition(float x, float y, float z, float o)
{
    // prevent crash when a bad coord is sent by the client
    if (!Trinity::IsValidMapCoord(x, y, z, o))
    {
        TC_LOG_DEBUG("entities.unit", "Creature::SetPosition(%f, %f, %f) .. bad coordinates!", x, y, z);
        return;
    }

    GetMap()->CreatureRelocation(this, x, y, z, o);
    if (IsVehicle())
        GetVehicleKit()->RelocatePassengers();
}

void Creature::SetDefaultMount(Optional<uint32> mountCreatureDisplayId)
{
    if (mountCreatureDisplayId && !sCreatureDisplayInfoStore.LookupEntry(*mountCreatureDisplayId))
        mountCreatureDisplayId.reset();

    _defaultMountDisplayIdOverride = mountCreatureDisplayId;
}

float Creature::GetAggroRange(Unit const* target) const
{
    // Determines the aggro range for creatures (usually pets), used mainly for aggressive pet target selection.
    // Based on data from wowwiki due to lack of 3.3.5a data

    if (target && this->IsPet())
    {
        uint32 targetLevel = 0;

        if (target->GetTypeId() == TYPEID_PLAYER)
            targetLevel = target->getLevelForTarget(this);
        else if (target->GetTypeId() == TYPEID_UNIT)
            targetLevel = target->ToCreature()->getLevelForTarget(this);

        uint32 myLevel = getLevelForTarget(target);
        int32 levelDiff = int32(targetLevel) - int32(myLevel);

        // The maximum Aggro Radius is capped at 45 yards (25 level difference)
        if (levelDiff < -25)
            levelDiff = -25;

        // The base aggro radius for mob of same level
        float aggroRadius = 20;

        // Aggro Radius varies with level difference at a rate of roughly 1 yard/level
        aggroRadius -= (float)levelDiff;

        // detect range auras
        aggroRadius += GetTotalAuraModifier(SPELL_AURA_MOD_DETECT_RANGE);

        // detected range auras
        aggroRadius += target->GetTotalAuraModifier(SPELL_AURA_MOD_DETECTED_RANGE);

        // Just in case, we don't want pets running all over the map
        if (aggroRadius > MAX_AGGRO_RADIUS)
            aggroRadius = MAX_AGGRO_RADIUS;

        // Minimum Aggro Radius for a mob seems to be combat range (5 yards)
        //  hunter pets seem to ignore minimum aggro radius so we'll default it a little higher
        if (aggroRadius < 10)
            aggroRadius = 10;

        return (aggroRadius);
    }

    // Default
    return 0.0f;
}

Unit* Creature::SelectNearestHostileUnitInAggroRange(bool useLOS, bool ignoreCivilians) const
{
    // Selects nearest hostile target within creature's aggro range. Used primarily by
    //  pets set to aggressive. Will not return neutral or friendly targets.

    Unit* target = nullptr;

    Trinity::NearestHostileUnitInAggroRangeCheck u_check(this, useLOS, ignoreCivilians);
    Trinity::UnitSearcher<Trinity::NearestHostileUnitInAggroRangeCheck> searcher(this, target, u_check);

    Cell::VisitGridObjects(this, searcher, MAX_AGGRO_RADIUS);

    return target;
}

float Creature::GetNativeObjectScale() const
{
    return GetCreatureTemplate()->scale;
}

void Creature::SetObjectScale(float scale)
{
    Unit::SetObjectScale(scale);

    if (CreatureModelInfo const* minfo = sObjectMgr->GetCreatureModelInfo(GetDisplayId()))
    {
        SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, (IsPet() ? 1.0f : minfo->bounding_radius) * scale);
        SetFloatValue(UNIT_FIELD_COMBATREACH, (IsPet() ? DEFAULT_PLAYER_COMBAT_REACH : minfo->combat_reach) * scale);
    }
}

void Creature::SetDisplayId(uint32 modelId, bool setNative /*= false*/)
{
    Unit::SetDisplayId(modelId, setNative);

    if (CreatureModelInfo const* minfo = sObjectMgr->GetCreatureModelInfo(modelId))
    {
        SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, (IsPet() ? 1.0f : minfo->bounding_radius) * GetObjectScale());
        SetFloatValue(UNIT_FIELD_COMBATREACH, (IsPet() ? DEFAULT_PLAYER_COMBAT_REACH : minfo->combat_reach) * GetObjectScale());
    }
}

void Creature::SetDisplayFromModel(uint32 modelIdx)
{
    if (CreatureModel const* model = GetCreatureTemplate()->GetModelByIdx(modelIdx))
        SetDisplayId(model->CreatureDisplayID);
}

void Creature::SetTarget(ObjectGuid guid)
{
    if (HasSpellFocus())
        _spellFocusInfo.OriginalUnitTarget = guid;
    else
        SetGuidValue(UNIT_FIELD_TARGET, guid);
}

void Creature::SetSpellFocus(Spell const* focusSpell, WorldObject const* target)
{
    // Pointer validation
    if (!focusSpell)
        return;

    // Creature already has a spell focus
    if (_spellFocusInfo.FocusSpell)
        return;

    // Prevent dead / feign death creatures from setting a focus target
    if (!IsAlive() || HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH) || HasAuraType(SPELL_AURA_FEIGN_DEATH))
        return;

    // Don't allow stunned creatures to set a focus target
    if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED))
        return;

    // Ignore casts with the according trigger flag
    if (focusSpell->IsFocusDisabled())
        return;

    SpellInfo const* spellInfo = focusSpell->GetSpellInfo();

    // Don't use spell focus for vehicle spells
    if (spellInfo->HasAura(SPELL_AURA_CONTROL_VEHICLE))
        return;

    // Instant casts and non-channeled spells do not need a spell focus
    if (!focusSpell->GetCastTime() && !spellInfo->IsChanneled())
        return;

    // Store original orientation and target values to restore them when releasing the spell focus
    if (!_spellFocusInfo.ReacquiringTargetDelay)
    { // only overwrite these fields if we aren't transitioning from one spell focus to another
        _spellFocusInfo.OriginalUnitTarget = GetGuidValue(UNIT_FIELD_TARGET);
    }
    else // don't automatically reacquire target for the previous spellcast
        _spellFocusInfo.ReacquiringTargetDelay = 0;

    _spellFocusInfo.FocusSpell = focusSpell;

    // Block target based focusing when the attributes or unit flags say so.
    bool dontFaceTarget = spellInfo->HasAttribute(SPELL_ATTR5_AI_DOESNT_FACE_TARGET)|| spellInfo->HasAttribute(SPELL_ATTR1_TRACK_TARGET_IN_CHANNEL) || HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_CANNOT_TURN);

    ObjectGuid newTarget = (dontFaceTarget || target == this || target == nullptr) ? ObjectGuid::Empty : target->GetGUID();

    if (GetTarget() != newTarget)
    {
        // Sniffs imply that UNIT_FIELD_TARGET via spell focusing is being sent out in an extra packet so we do so as well.
        SetGuidValue(UNIT_FIELD_TARGET, newTarget);
        SendUpdateToSet();
        ClearUpdateMask(true);
    }

    // Spells that do not face via UNIT_FIELD_TARGET and can turn, will face their target one last time
    if (!newTarget.IsEmpty() && target != this)
    {
        SetFacingTo(GetAngle(target));
        // Update the orentation right away. Splines take one tick to update which might become a problem in scripts.
        SetOrientationTowards(target);
    }

    if (newTarget.IsEmpty())
        AddUnitState(UNIT_STATE_FOCUSING);
}

bool Creature::HasSpellFocus(Spell const* focusSpell) const
{
    if (isDead()) // dead creatures cannot focus
    {
        if (_spellFocusInfo.FocusSpell || _spellFocusInfo.ReacquiringTargetDelay)
        {
            TC_LOG_WARN("entities.unit", "Creature '%s' (entry %u) has spell focus (spell id %u, delay %ums) despite being dead.",
                        GetName().c_str(), GetEntry(), _spellFocusInfo.FocusSpell ? _spellFocusInfo.FocusSpell->GetSpellInfo()->Id : 0, _spellFocusInfo.ReacquiringTargetDelay);
        }
        return false;
    }

    if (focusSpell)
        return (focusSpell == _spellFocusInfo.FocusSpell);
    else
        return (_spellFocusInfo.FocusSpell || _spellFocusInfo.ReacquiringTargetDelay);
}

void Creature::ReleaseSpellFocus(Spell const* focusSpell, bool withDelay)
{
    if (!_spellFocusInfo.FocusSpell)
        return;

    // focused to something else
    if (focusSpell && focusSpell != _spellFocusInfo.FocusSpell)
        return;

     ClearUnitState(UNIT_STATE_FOCUSING);

    if (IsPet()) // player pets do not use delay system
    {
        if (!HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_CANNOT_TURN))
            ReacquireSpellFocusTarget();
    }
    else // don't allow re-target right away to prevent visual bugs
        _spellFocusInfo.ReacquiringTargetDelay = withDelay ? 1000 : 1;

    _spellFocusInfo.FocusSpell = nullptr;
}

void Creature::ReacquireSpellFocusTarget()
{
    if (!HasSpellFocus())
    {
        TC_LOG_ERROR("entities.unit", "Creature::ReacquireSpellFocusTarget() being alled with HasSpellFocus() return false.");
        return;
    }

    SetGuidValue(UNIT_FIELD_TARGET, _spellFocusInfo.OriginalUnitTarget);

    if (!HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_CANNOT_TURN))
    {
        if (_spellFocusInfo.OriginalUnitTarget)
        {
            if (WorldObject const* objTarget = ObjectAccessor::GetWorldObject(*this, _spellFocusInfo.OriginalUnitTarget))
                SetFacingToObject(objTarget, false);
        }
    }
    _spellFocusInfo.ReacquiringTargetDelay = 0;
}

bool Creature::IsMovementPreventedByCasting() const
{
    if (!Unit::IsMovementPreventedByCasting() && !HasSpellFocus())
        return false;

    return true;
}

void Creature::StartPickPocketRefillTimer()
{
    _pickpocketLootRestore = GameTime::GetGameTime() + sWorld->getIntConfig(CONFIG_CREATURE_PICKPOCKET_REFILL);
}

bool Creature::CanGeneratePickPocketLoot() const
{
    return _pickpocketLootRestore <= GameTime::GetGameTime();
}

void Creature::SetTextRepeatId(uint8 textGroup, uint8 id)
{
    CreatureTextRepeatIds& repeats = m_textRepeat[textGroup];
    if (std::find(repeats.begin(), repeats.end(), id) == repeats.end())
        repeats.push_back(id);
    else
        TC_LOG_ERROR("sql.sql", "CreatureTextMgr: TextGroup %u for Creature(%s) GuidLow %u Entry %u, id %u already added", uint32(textGroup), GetName().c_str(), GetGUID().GetCounter(), GetEntry(), uint32(id));
}

CreatureTextRepeatIds Creature::GetTextRepeatGroup(uint8 textGroup)
{
    CreatureTextRepeatIds ids;

    CreatureTextRepeatGroup::const_iterator groupItr = m_textRepeat.find(textGroup);
    if (groupItr != m_textRepeat.end())
        ids = groupItr->second;

    return ids;
}

void Creature::ClearTextRepeatGroup(uint8 textGroup)
{
    CreatureTextRepeatGroup::iterator groupItr = m_textRepeat.find(textGroup);
    if (groupItr != m_textRepeat.end())
        groupItr->second.clear();
}

bool Creature::CanGiveExperience() const
{
    return !IsCritter()
        && !IsPet()
        && !IsTotem()
        && !(GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_NO_XP_AT_KILL);
}

bool Creature::IsEngaged() const
{
    if (CreatureAI const* ai = AI())
        return ai->IsEngaged();
    return false;
}

void Creature::AtEngage(Unit* target)
{
    Unit::AtEngage(target);

    GetThreatManager().ResetUpdateTimer();

    if (!(GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_MOUNTED_COMBAT_ALLOWED) && !HasStaticFlag(CREATURE_STATIC_FLAG_2_ALLOW_MOUNTED_COMBAT))
        Dismount();

    MovementGeneratorType const movetype = GetMotionMaster()->GetCurrentMovementGeneratorType();
    if (movetype == WAYPOINT_MOTION_TYPE || movetype == CYCLIC_SPLINE_MOTION_TYPE || movetype == POINT_MOTION_TYPE || (IsAIEnabled() && AI()->IsEscorted()))
        SetHomePosition(GetPosition());

    if (CreatureAI* ai = AI())
        ai->JustEngagedWith(target);
    if (CreatureGroup* formation = GetFormation())
        formation->MemberEngagingTarget(this, target);

    if (GetAI() && !IsControlledByPlayer())
        SendAIReaction(AI_REACTION_HOSTILE);

    // Creatures with CREATURE_STATIC_FLAG_2_FORCE_PARTY_MEMBERS_INTO_COMBAT periodically force party members into combat
    ForcePartyMembersIntoCombat();
}

void Creature::AtDisengage()
{
    Unit::AtDisengage();

    ClearUnitState(UNIT_STATE_ATTACK_PLAYER);
    if (IsAlive() && HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED))
        RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED);
}

void Creature::ForcePartyMembersIntoCombat()
{
    if (!_staticFlags.HasFlag(CREATURE_STATIC_FLAG_2_FORCE_PARTY_MEMBERS_INTO_COMBAT) || !IsEngaged())
        return;

    Trinity::Containers::FlatSet<Group const*> partiesToForceIntoCombat;
    for (auto const& [_, combatReference] : GetCombatManager().GetPvECombatRefs())
    {
        //if (combatReference->IsSuppressedFor(this))
        //    continue;

        Unit* other = combatReference->GetOther(this);

        Player* player = other ? other->ToPlayer() : nullptr;
        if (!player || player->IsGameMaster())
            continue;

        if (Group const* group = player->GetGroup())
            partiesToForceIntoCombat.insert(group);
    }

    for (Group const* partyToForceIntoCombat : partiesToForceIntoCombat)
    {
        for (GroupReference const* ref = partyToForceIntoCombat->GetFirstMember(); ref != nullptr; ref = ref->next())
        {
            Player* player = ref->GetSource();
            if (!player || !player->IsInWorld() || player->GetMap() != GetMap() || player->IsGameMaster())
                continue;

            EngageWithTarget(player);
        }
    }
}


bool Creature::IsEscorted() const
{
    if (CreatureAI* ai = AI())
        return ai->IsEscorted();
    return false;
}

void Creature::MakeInterruptable(bool apply)
{
    ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, !apply);
    ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, !apply);
}
