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

#ifndef _OBJECT_H
#define _OBJECT_H

#include "Common.h"
#include "Duration.h"
#include "EventProcessor.h"
#include "GridReference.h"
#include "GridRefManager.h"
#include "ModelIgnoreFlags.h"
#include "MovementInfo.h"
#include "ObjectDefines.h"
#include "ObjectGuid.h"
#include "Optional.h"
#include "PhaseShift.h"
#include "Position.h"
#include "SharedDefines.h"
#include "SpellDefines.h"
#include "UpdateFields.h"
#include "UpdateMask.h"
#include <list>
#include <unordered_map>

class AreaTrigger;
class Corpse;
class Creature;
class CreatureAI;
class DynamicObject;
class GameObject;
class InstanceScript;
class Map;
class Player;
class Spell;
class SpellCastTargets;
class SpellInfo;
class TempSummon;
class TransportBase;
class Unit;
class UpdateData;
class WorldObject;
class WorldPacket;
class ZoneScript;
struct FactionTemplateEntry;
struct PositionFullTerrainStatus;
struct QuaternionData;
enum ZLiquidStatus : uint32;

typedef std::unordered_map<Player*, UpdateData> UpdateDataMapType;

struct CreateObjectBits
{
    bool PlayerHoverAnim : 1;
    bool SupressedGreetings : 1;
    bool Rotation : 1;
    bool AnimKit : 1;
    bool CombatVictim : 1;
    bool ThisIsYou : 1;
    bool Vehicle : 1;
    bool MovementUpdate : 1;
    bool NoBirthAnim : 1;
    bool MovementTransport : 1;
    bool Stationary : 1;
    bool AreaTrigger : 1;
    bool EnablePortals : 1;
    bool ServerTime : 1;


    void Clear()
    {
        memset(this, 0, sizeof(CreateObjectBits));
    }
};

float const DEFAULT_COLLISION_HEIGHT = 2.03128f; // Most common value in dbc
static constexpr Milliseconds const HEARTBEAT_INTERVAL = 5s + 200ms;

class TC_GAME_API Object
{
    public:
        virtual ~Object();

        bool IsInWorld() const { return m_inWorld; }

        virtual void AddToWorld();
        virtual void RemoveFromWorld();

        ObjectGuid GetGUID() const { return GetGuidValue(OBJECT_FIELD_GUID); }
        PackedGuid const& GetPackGUID() const { return m_PackGUID; }
        uint32 GetEntry() const { return GetUInt32Value(OBJECT_FIELD_ENTRY); }
        void SetEntry(uint32 entry) { SetUInt32Value(OBJECT_FIELD_ENTRY, entry); }

        float GetObjectScale() const { return GetFloatValue(OBJECT_FIELD_SCALE_X); }
        virtual void SetObjectScale(float scale) { SetFloatValue(OBJECT_FIELD_SCALE_X, scale); }

        TypeID GetTypeId() const { return m_objectTypeId; }
        bool isType(uint16 mask) const { return (mask & m_objectType) != 0; }

        virtual void BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target) const;
        void SendUpdateToPlayer(Player* player);
        void SendUpdateToSet();

        void BuildValuesUpdateBlockForPlayer(UpdateData* data, Player* target) const;
        void BuildOutOfRangeUpdateBlock(UpdateData* data) const;

        virtual void DestroyForPlayer(Player* target, bool isDead = false) const;
        void SendOutOfRangeForPlayer(Player* target) const;

        int32 GetInt32Value(uint16 index) const;
        uint32 GetUInt32Value(uint16 index) const;
        uint64 GetUInt64Value(uint16 index) const;
        float GetFloatValue(uint16 index) const;
        uint8 GetByteValue(uint16 index, uint8 offset) const;
        uint16 GetUInt16Value(uint16 index, uint8 offset) const;
        ObjectGuid GetGuidValue(uint16 index) const;

        void SetInt32Value(uint16 index, int32 value);
        void SetUInt32Value(uint16 index, uint32 value);
        void UpdateUInt32Value(uint16 index, uint32 value);
        void SetUInt64Value(uint16 index, uint64 value);
        void SetFloatValue(uint16 index, float value);
        void SetByteValue(uint16 index, uint8 offset, uint8 value);
        void SetUInt16Value(uint16 index, uint8 offset, uint16 value);
        void SetInt16Value(uint16 index, uint8 offset, int16 value) { SetUInt16Value(index, offset, (uint16)value); }
        void SetGuidValue(uint16 index, ObjectGuid value);
        void SetStatFloatValue(uint16 index, float value);
        void SetStatInt32Value(uint16 index, int32 value);

        bool AddGuidValue(uint16 index, ObjectGuid value);
        bool RemoveGuidValue(uint16 index, ObjectGuid value);

        void ApplyModUInt32Value(uint16 index, int32 val, bool apply);
        void ApplyModInt32Value(uint16 index, int32 val, bool apply);
        void ApplyModUInt64Value(uint16 index, int32 val, bool apply);
        void ApplyModPositiveFloatValue(uint16 index, float val, bool apply);
        void ApplyModSignedFloatValue(uint16 index, float val, bool apply);

        void SetFlag(uint16 index, uint32 newFlag);
        void RemoveFlag(uint16 index, uint32 oldFlag);
        void ToggleFlag(uint16 index, uint32 flag);
        bool HasFlag(uint16 index, uint32 flag) const;
        void ApplyModFlag(uint16 index, uint32 flag, bool apply);

        void SetByteFlag(uint16 index, uint8 offset, uint8 newFlag);
        void RemoveByteFlag(uint16 index, uint8 offset, uint8 newFlag);
        void ToggleByteFlag(uint16 index, uint8 offset, uint8 flag);
        bool HasByteFlag(uint16 index, uint8 offset, uint8 flag) const;
        void ApplyModByteFlag(uint16 index, uint8 offset, uint8 flag, bool apply);

        void SetFlag64(uint16 index, uint64 newFlag);
        void RemoveFlag64(uint16 index, uint64 oldFlag);
        void ToggleFlag64(uint16 index, uint64 flag);
        bool HasFlag64(uint16 index, uint64 flag) const;
        void ApplyModFlag64(uint16 index, uint64 flag, bool apply);

        void ClearUpdateMask(bool remove);

        uint16 GetValuesCount() const { return m_valuesCount; }

        virtual bool hasQuest(uint32 /* quest_id */) const { return false; }
        virtual bool hasInvolvedQuest(uint32 /* quest_id */) const { return false; }
        void SetIsNewObject(bool enable) { m_isNewObject = enable; }
        bool IsDestroyedObject() const { return m_isDestroyedObject; }
        void SetDestroyedObject(bool destroyed) { m_isDestroyedObject = destroyed; }
        virtual void BuildUpdate(UpdateDataMapType&) { }
        void BuildFieldsUpdate(Player*, UpdateDataMapType &) const;

        void SetFieldNotifyFlag(uint16 flag) { _fieldNotifyFlags |= flag; }
        void RemoveFieldNotifyFlag(uint16 flag) { _fieldNotifyFlags &= uint16(~flag); }

        // FG: some hacky helpers
        void ForceValuesUpdateAtIndex(uint32);

        inline bool IsPlayer() const { return GetTypeId() == TYPEID_PLAYER; }
        Player* ToPlayer() { if (GetTypeId() == TYPEID_PLAYER) return reinterpret_cast<Player*>(this); else return nullptr; }
        Player const* ToPlayer() const { if (GetTypeId() == TYPEID_PLAYER) return reinterpret_cast<Player const*>(this); else return nullptr; }

        inline bool IsCreature() const { return GetTypeId() == TYPEID_UNIT; }
        Creature* ToCreature() { if (GetTypeId() == TYPEID_UNIT) return reinterpret_cast<Creature*>(this); else return nullptr; }
        Creature const* ToCreature() const { if (GetTypeId() == TYPEID_UNIT) return reinterpret_cast<Creature const*>(this); else return nullptr; }

        inline bool IsUnit() const { return isType(TYPEMASK_UNIT); }
        Unit* ToUnit() { if (isType(TYPEMASK_UNIT)) return reinterpret_cast<Unit*>(this); else return nullptr; }
        Unit const* ToUnit() const { if (isType(TYPEMASK_UNIT)) return reinterpret_cast<Unit const*>(this); else return nullptr; }

        inline bool IsGameObject() const { return GetTypeId() == TYPEID_GAMEOBJECT; }
        GameObject* ToGameObject() { if (GetTypeId() == TYPEID_GAMEOBJECT) return reinterpret_cast<GameObject*>(this); else return nullptr; }
        GameObject const* ToGameObject() const { if (GetTypeId() == TYPEID_GAMEOBJECT) return reinterpret_cast<GameObject const*>(this); else return nullptr; }

        inline bool IsCorpse() const { return GetTypeId() == TYPEID_CORPSE; }
        Corpse* ToCorpse() { if (GetTypeId() == TYPEID_CORPSE) return reinterpret_cast<Corpse*>(this); else return nullptr; }
        Corpse const* ToCorpse() const { if (GetTypeId() == TYPEID_CORPSE) return reinterpret_cast<Corpse const*>(this); else return nullptr; }

        inline bool IsDynObject() const { return GetTypeId() == TYPEID_DYNAMICOBJECT; }
        DynamicObject* ToDynObject() { if (GetTypeId() == TYPEID_DYNAMICOBJECT) return reinterpret_cast<DynamicObject*>(this); else return nullptr; }
        DynamicObject const* ToDynObject() const { if (GetTypeId() == TYPEID_DYNAMICOBJECT) return reinterpret_cast<DynamicObject const*>(this); else return nullptr; }

        inline bool IsAreaTrigger() const { return GetTypeId() == TYPEID_AREATRIGGER; }
        AreaTrigger* ToAreaTrigger() { if (GetTypeId() == TYPEID_AREATRIGGER) return reinterpret_cast<AreaTrigger*>(this); else return nullptr; }
        AreaTrigger const* ToAreaTrigger() const { if (GetTypeId() == TYPEID_AREATRIGGER) return reinterpret_cast<AreaTrigger const*>(this); else return nullptr; }

    protected:
        Object();

        void _InitValues();
        void _Create(ObjectGuid::LowType guidlow, uint32 entry, HighGuid guidhigh);
        std::string _ConcatFields(uint16 startIndex, uint16 size) const;
        void _LoadIntoDataField(std::string const& data, uint32 startOffset, uint32 count);

        uint32 GetUpdateFieldData(Player const* target, uint32*& flags) const;

        void BuildMovementUpdate(ByteBuffer* data, CreateObjectBits flags) const;
        virtual void BuildValuesUpdate(uint8 updatetype, ByteBuffer* data, Player* target) const;

        uint16 m_objectType;

        TypeID m_objectTypeId;
        CreateObjectBits m_updateFlag;

        union
        {
            int32  *m_int32Values;
            uint32 *m_uint32Values;
            float  *m_floatValues;
        };

        UpdateMask _changesMask;

        uint16 m_valuesCount;

        uint16 _fieldNotifyFlags;

        virtual bool AddToObjectUpdate() = 0;
        virtual void RemoveFromObjectUpdate() = 0;
        void AddToObjectUpdateIfNeeded();

        bool m_objectUpdated;

    private:
        bool m_inWorld;
        bool m_isNewObject;
        bool m_isDestroyedObject;

        PackedGuid m_PackGUID;

        // for output helpfull error messages from asserts
        bool PrintIndexError(uint32 index, bool set) const;
        Object(Object const& right) = delete;
        Object& operator=(Object const& right) = delete;
};

template<class T>
class GridObject
{
    public:
        virtual ~GridObject() { }

        bool IsInGrid() const { return _gridRef.isValid(); }
        void AddToGrid(GridRefManager<T>& m) { ASSERT(!IsInGrid()); _gridRef.link(&m, (T*)this); }
        void RemoveFromGrid() { ASSERT(IsInGrid()); _gridRef.unlink(); }
    private:
        GridReference<T> _gridRef;
};

template <class T_VALUES, class T_FLAGS, class FLAG_TYPE, size_t ARRAY_SIZE>
class FlaggedValuesArray32
{
    public:
        FlaggedValuesArray32()
        {
            for (uint32 i = 0; i < ARRAY_SIZE; ++i)
                m_values[i] = T_VALUES(0);
            m_flags = 0;
        }

        T_FLAGS  GetFlags() const { return m_flags; }
        bool     HasFlag(FLAG_TYPE flag) const { return m_flags & (1 << flag); }
        void     AddFlag(FLAG_TYPE flag) { m_flags |= (1 << flag); }
        void     DelFlag(FLAG_TYPE flag) { m_flags &= ~(1 << flag); }

        T_VALUES GetValue(FLAG_TYPE flag) const { return m_values[flag]; }
        void     SetValue(FLAG_TYPE flag, T_VALUES value) { m_values[flag] = value; }
        void     AddValue(FLAG_TYPE flag, T_VALUES value) { m_values[flag] += value; }

    private:
        T_VALUES m_values[ARRAY_SIZE];
        T_FLAGS m_flags;
};

enum GOSummonType
{
   GO_SUMMON_TIMED_OR_CORPSE_DESPAWN = 0,    // despawns after a specified time OR when the summoner dies
   GO_SUMMON_TIMED_DESPAWN = 1     // despawns after a specified time
};

class TC_GAME_API WorldObject : public Object, public WorldLocation
{
    protected:
        explicit WorldObject(bool isWorldObject); //note: here it means if it is in grid object list or world object list
    public:
        virtual ~WorldObject();

        virtual void Update(uint32 diff);

        void _Create(ObjectGuid::LowType guidlow, HighGuid guidhigh);
        void AddToWorld() override;
        void RemoveFromWorld() override;

        void GetNearPoint2D(WorldObject const* searcher, float &x, float &y, float distance, float absAngle) const;
        void GetNearPoint(WorldObject const* searcher, float &x, float &y, float &z, float distance2d, float absAngle) const;
        void GetClosePoint(float &x, float &y, float &z, float size, float distance2d = 0, float angle = 0) const;
        void MovePosition(Position &pos, float dist, float angle);
        Position GetNearPosition(float dist, float angle);
        void MovePositionToFirstCollision(Position &pos, float dist, float angle);
        Position GetFirstCollisionPosition(float dist, float angle);
        Position GetRandomNearPosition(float radius);
        void GetContactPoint(WorldObject const* obj, float &x, float &y, float &z, float distance2d = CONTACT_DISTANCE) const;

        virtual float GetCombatReach() const { return 0.0f; } // overridden (only) in Unit
        void UpdateGroundPositionZ(float x, float y, float &z) const;
        void UpdateAllowedPositionZ(float x, float y, float &z, float* groundZ = nullptr) const;

        void GetRandomPoint(Position const& srcPos, float distance, float& rand_x, float& rand_y, float& rand_z) const;
        Position GetRandomPoint(Position const& srcPos, float distance) const;

        uint32 GetInstanceId() const { return m_InstanceId; }

        bool IsInPhase(WorldObject const* obj) const { return obj ? GetPhaseShift().CanSee(obj->GetPhaseShift()) : false; }

        PhaseShift& GetPhaseShift() { return _phaseShift; }
        PhaseShift const& GetPhaseShift() const { return _phaseShift; }
        PhaseShift& GetSuppressedPhaseShift() { return _suppressedPhaseShift; }
        PhaseShift const& GetSuppressedPhaseShift() const { return _suppressedPhaseShift; }
        int32 GetDBPhase() { return _dbPhase; }

        // if negative it is used as PhaseGroupId
        void SetDBPhase(int32 p) { _dbPhase = p; }

        uint32 GetZoneId() const { return m_zoneId; }
        uint32 GetAreaId() const { return m_areaId; }
        void GetZoneAndAreaId(uint32& zoneid, uint32& areaid) const { zoneid = m_zoneId, areaid = m_areaId; }
        bool IsInWorldPvpZone() const;
        bool IsOutdoors() const { return m_outdoors; }
        ZLiquidStatus GetLiquidStatus() const { return m_liquidStatus; }
        uint32 GetWMOGroupId() const { return m_wmoGroupID; }

        InstanceScript* GetInstanceScript() const;

        std::string const& GetName() const { return m_name; }
        void SetName(std::string const& newname) { m_name = newname; }

        virtual std::string const& GetNameForLocaleIdx(LocaleConstant /*locale_idx*/) const { return m_name; }

        float GetDistance(WorldObject const* obj) const;
        float GetDistance(Position const& pos) const;
        float GetDistance(float x, float y, float z) const;
        float GetDistance2d(WorldObject const* obj) const;
        float GetDistance2d(float x, float y) const;
        float GetDistanceZ(WorldObject const* obj) const;

        bool IsSelfOrInSameMap(WorldObject const* obj) const;
        bool IsInMap(WorldObject const* obj) const;
        bool IsWithinDist3d(float x, float y, float z, float dist) const;
        bool IsWithinDist3d(Position const* pos, float dist) const;
        bool IsWithinDist2d(float x, float y, float dist) const;
        bool IsWithinDist2d(Position const* pos, float dist) const;
        // use only if you will sure about placing both object at same map
        bool IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D = true) const;
        bool IsWithinDistInMap(WorldObject const* obj, float dist2compare, bool is3D = true, bool incOwnRadius = true, bool incTargetRadius = true) const;
        bool IsWithinLOS(float x, float y, float z, LineOfSightChecks checks = LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags ignoreFlags = VMAP::ModelIgnoreFlags::Nothing) const;
        bool IsWithinLOSInMap(WorldObject const* obj, LineOfSightChecks checks = LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags ignoreFlags = VMAP::ModelIgnoreFlags::Nothing) const;
        Position GetHitSpherePointFor(Position const& dest) const;
        void GetHitSpherePointFor(Position const& dest, float& x, float& y, float& z) const;
        bool GetDistanceOrder(WorldObject const* obj1, WorldObject const* obj2, bool is3D = true) const;
        bool IsInRange(WorldObject const* obj, float minRange, float maxRange, bool is3D = true) const;
        bool IsInRange2d(float x, float y, float minRange, float maxRange) const;
        bool IsInRange3d(float x, float y, float z, float minRange, float maxRange) const;
        bool isInFront(WorldObject const* target, float arc = float(M_PI)) const;
        bool isInBack(WorldObject const* target, float arc = float(M_PI)) const;

        bool IsInBetween(Position const& pos1, Position const& pos2, float size = 0) const;
        bool IsInBetween(WorldObject const* obj1, WorldObject const* obj2, float size = 0) const { return obj1 && obj2 && IsInBetween(obj1->GetPosition(), obj2->GetPosition(), size); }

        virtual void CleanupsBeforeDelete(bool finalCleanup = true);  // used in destructor or explicitly before mass creature delete to remove cross-references to already deleted units

        virtual void SendMessageToSet(WorldPacket const* data, bool self) const;
        virtual void SendMessageToSetInRange(WorldPacket const* data, float dist, bool self) const;
        virtual void SendMessageToSet(WorldPacket const* data, Player const* skipped_rcvr) const;

        virtual uint8 getLevelForTarget(WorldObject const* /*target*/) const { return 1; }

        void PlayDistanceSound(uint32 sound_id, Player* target = nullptr);
        void PlayDirectSound(uint32 sound_id, Player* target = nullptr);
        void PlayDirectMusic(uint32 music_id, Player* target = nullptr);

        void AddObjectToRemoveList();

        float GetGridActivationRange() const;
        float GetVisibilityRange() const;
        float GetSightRange(WorldObject const* target = nullptr) const;
        bool CanSeeOrDetect(WorldObject const* obj, bool ignoreStealth = false, bool distanceCheck = false, bool checkAlert = false) const;

        FlaggedValuesArray32<int32, uint32, StealthType, TOTAL_STEALTH_TYPES> m_stealth;
        FlaggedValuesArray32<int32, uint32, StealthType, TOTAL_STEALTH_TYPES> m_stealthDetect;

        FlaggedValuesArray32<int32, uint32, InvisibilityType, TOTAL_INVISIBILITY_TYPES> m_invisibility;
        FlaggedValuesArray32<int32, uint32, InvisibilityType, TOTAL_INVISIBILITY_TYPES> m_invisibilityDetect;

        FlaggedValuesArray32<int32, uint32, ServerSideVisibilityType, TOTAL_SERVERSIDE_VISIBILITY_TYPES> m_serverSideVisibility;
        FlaggedValuesArray32<int32, uint32, ServerSideVisibilityType, TOTAL_SERVERSIDE_VISIBILITY_TYPES> m_serverSideVisibilityDetect;

        virtual void SetMap(Map* map);
        virtual void ResetMap();
        Map* GetMap() const { ASSERT(m_currMap); return m_currMap; }
        Map* FindMap() const { return m_currMap; }
        //used to check all object's GetMap() calls when object is not in world!

        void SetZoneScript();
        void ClearZoneScript();
        ZoneScript* FindZoneScript() const;
        ZoneScript* GetZoneScript() const { return m_zoneScript; }

        TempSummon* SummonCreature(uint32 entry, Position const& pos, TempSummonType despawnType = TEMPSUMMON_MANUAL_DESPAWN, uint32 despawnTime = 0, uint32 vehId = 0, ObjectGuid privateObjectOwner = ObjectGuid::Empty);
        TempSummon* SummonCreature(uint32 entry, Position const& pos, TempSummonType despawnType, Milliseconds despawnTime, uint32 vehId = 0, ObjectGuid privateObjectOwner = ObjectGuid::Empty) { return SummonCreature(entry, pos, despawnType, uint32(despawnTime.count()), vehId, privateObjectOwner); }
        TempSummon* SummonCreature(uint32 entry, float x, float y, float z, float o = 0, TempSummonType despawnType = TEMPSUMMON_MANUAL_DESPAWN, uint32 despawnTime = 0, ObjectGuid privateObjectOwner = ObjectGuid::Empty);

        GameObject* SummonGameObject(uint32 entry, Position const& pos, QuaternionData const& rot, uint32 respawnTime /* s */, GOSummonType summonType = GO_SUMMON_TIMED_OR_CORPSE_DESPAWN);
        GameObject* SummonGameObject(uint32 entry, float x, float y, float z, float ang, QuaternionData const& rot, uint32 respawnTime /* s */);
        Creature*   SummonTrigger(float x, float y, float z, float ang, uint32 dur, CreatureAI* (*GetAI)(Creature*) = nullptr);
        void SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list = nullptr);

        Creature*   FindNearestCreature(uint32 entry, float range, bool alive = true) const;
        GameObject* FindNearestGameObject(uint32 entry, float range) const;
        GameObject* FindNearestGameObjectOfType(GameobjectTypes type, float range) const;
        Player* SelectNearestPlayer(float distance) const;

        virtual ObjectGuid GetOwnerGUID() const = 0;
        virtual ObjectGuid GetCharmerOrOwnerGUID() const { return GetOwnerGUID(); }
        ObjectGuid GetCharmerOrOwnerOrOwnGUID() const;

        Unit* GetOwner() const;
        Unit* GetCharmerOrOwner() const;
        Unit* GetCharmerOrOwnerOrSelf() const;
        Unit* GetCharmerOrOwnerOrCreatorOrSelf() const;
        Player* GetCharmerOrOwnerPlayerOrPlayerItself() const;
        Player* GetAffectingPlayer() const;

        Player* GetSpellModOwner() const;
        int32 CalculateSpellDamage(Unit const* target, SpellInfo const* spellProto, uint8 effIndex, int32 const* basePoints = nullptr) const;

        // target dependent range checks
        float GetSpellMaxRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const;
        float GetSpellMinRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const;

        float ApplyEffectModifiers(SpellInfo const* spellProto, uint8 effect_index, float value) const;
        int32 CalcSpellDuration(SpellInfo const* spellInfo) const;
        int32 ModSpellDuration(SpellInfo const* spellProto, WorldObject const* target, int32 duration, bool positive, uint32 effectMask);
        void  ModSpellCastTime(SpellInfo const* spellProto, int32& castTime, Spell* spell = nullptr);

        virtual float MeleeSpellMissChance(Unit const* victim, WeaponAttackType attType, SpellInfo const* spellInfo = nullptr) const;
        virtual SpellMissInfo MeleeSpellHitResult(Unit* victim, SpellInfo const* spellInfo) const;
        SpellMissInfo MagicSpellHitResult(Unit* victim, SpellInfo const* spellInfo) const;
        SpellMissInfo SpellHitResult(Unit* victim, SpellInfo const* spellInfo, bool canReflect = false) const;
        void SendSpellMiss(Unit* target, uint32 spellID, SpellMissInfo missInfo);

        virtual uint32 GetFaction() const = 0;
        virtual void SetFaction(uint32 /*faction*/) { }
        FactionTemplateEntry const* GetFactionTemplateEntry() const;

        ReputationRank GetReactionTo(WorldObject const* target) const;
        static ReputationRank GetFactionReactionTo(FactionTemplateEntry const* factionTemplateEntry, WorldObject const* target);

        bool IsHostileTo(WorldObject const* target) const;
        bool IsHostileToPlayers() const;
        bool IsFriendlyTo(WorldObject const* target) const;
        bool IsNeutralToAll() const;

        // CastSpell's third arg can be a variety of things - check out CastSpellExtraArgs' constructors!
        SpellCastResult CastSpell(SpellCastTargets const& targets, uint32 spellId, CastSpellExtraArgs const& args = { });
        SpellCastResult CastSpell(WorldObject* target, uint32 spellId, CastSpellExtraArgs const& args = { });
        SpellCastResult CastSpell(Position const& dest, uint32 spellId, CastSpellExtraArgs const& args = { });

        bool IsValidAttackTarget(WorldObject const* target, SpellInfo const* bySpell = nullptr) const;
        bool IsValidAssistTarget(WorldObject const* target, SpellInfo const* bySpell = nullptr) const;

        Unit* GetMagicHitRedirectTarget(Unit* victim, SpellInfo const* spellInfo);

        template <typename Container>
        void GetGameObjectListWithEntryInGrid(Container& gameObjectContainer, uint32 entry, float maxSearchRange = 250.0f) const;

        template <typename Container>
        void GetCreatureListWithEntryInGrid(Container& creatureContainer, uint32 entry, float maxSearchRange = 250.0f) const;

        template <typename Container>
        void GetPlayerListInGrid(Container& playerContainer, float maxSearchRange) const;

        void DestroyForNearbyPlayers();
        virtual void UpdateObjectVisibility(bool forced = true);
        virtual void UpdateObjectVisibilityOnCreate() { UpdateObjectVisibility(true); }
        virtual void UpdateObjectVisibilityOnDestroy() { DestroyForNearbyPlayers(); }
        void UpdatePositionData();

        void BuildUpdate(UpdateDataMapType&) override;

        bool AddToObjectUpdate() override;
        void RemoveFromObjectUpdate() override;

        //relocation and visibility system functions
        void AddToNotify(uint16 f) { m_notifyflags |= f;}
        bool isNeedNotify(uint16 f) const { return (m_notifyflags & f) != 0; }
        uint16 GetNotifyFlags() const { return m_notifyflags; }
        void ResetAllNotifies() { m_notifyflags = 0; }

        bool isActiveObject() const { return m_isActive; }
        void setActive(bool isActiveObject);
        bool IsFarVisible() const { return m_isFarVisible; }
        void SetFarVisible(bool on);
        bool IsVisibilityOverriden() const { return m_visibilityDistanceOverride.has_value(); }
        void SetVisibilityDistanceOverride(VisibilityDistanceType type);
        void SetWorldObject(bool apply);
        bool IsPermanentWorldObject() const { return m_isWorldObject; }
        bool IsWorldObject() const;

        uint32  LastUsedScriptID;

        // Transports
        TransportBase* GetTransport() const { return m_transport; }
        float GetTransOffsetX() const { return m_movementInfo.transport.pos.GetPositionX(); }
        float GetTransOffsetY() const { return m_movementInfo.transport.pos.GetPositionY(); }
        float GetTransOffsetZ() const { return m_movementInfo.transport.pos.GetPositionZ(); }
        float GetTransOffsetO() const { return m_movementInfo.transport.pos.GetOrientation(); }
        Position const& GetTransOffset() const { return m_movementInfo.transport.pos; }
        uint32 GetTransTime()   const { return m_movementInfo.transport.time; }
        int8 GetTransSeat()     const { return m_movementInfo.transport.seat; }
        virtual ObjectGuid GetTransGUID() const;
        void SetTransport(TransportBase* t) { m_transport = t; }

        MovementInfo m_movementInfo;

        virtual float GetStationaryX() const { return GetPositionX(); }
        virtual float GetStationaryY() const { return GetPositionY(); }
        virtual float GetStationaryZ() const { return GetPositionZ(); }
        virtual float GetStationaryO() const { return GetOrientation(); }

        float GetFloorZ() const;
        virtual float GetCollisionHeight() const { return 0.0f; }

        float GetMapWaterOrGroundLevel(float x, float y, float z, float* ground = nullptr) const;
        float GetMapHeight(float x, float y, float z, bool vmap = true, float distanceToSearch = 50.0f) const; // DEFAULT_HEIGHT_SEARCH in map.h

        // Event handler
        EventProcessor m_Events;

        uint16 GetAIAnimKitId() const { return m_aiAnimKitId; }
        void SetAIAnimKitId(uint16 animKitId);
        uint16 GetMovementAnimKitId() const { return m_movementAnimKitId; }
        void SetMovementAnimKitId(uint16 animKitId);
        uint16 GetMeleeAnimKitId() const { return m_meleeAnimKitId; }
        void SetMeleeAnimKitId(uint16 animKitId);

        // Watcher
        bool IsPrivateObject() const { return !_privateObjectOwner.IsEmpty(); }
        ObjectGuid GetPrivateObjectOwner() const { return _privateObjectOwner; }
        void SetPrivateObjectOwner(ObjectGuid const& owner) { _privateObjectOwner = owner; }
        bool CheckPrivateObjectOwnerVisibility(WorldObject const* seer) const;

    protected:
        std::string m_name;
        bool m_isActive;
        bool m_isFarVisible;
        Optional<float> m_visibilityDistanceOverride;
        bool const m_isWorldObject;
        ZoneScript* m_zoneScript;

        // transports (gameobjects only)
        TransportBase* m_transport;

        virtual void ProcessPositionDataChanged(PositionFullTerrainStatus const& data);
        uint32 m_zoneId;
        uint32 m_areaId;
        float m_staticFloorZ;
        bool m_outdoors;
        ZLiquidStatus m_liquidStatus;
        uint32 m_wmoGroupID;

        //these functions are used mostly for Relocate() and Corpse/Player specific stuff...
        //use them ONLY in LoadFromDB()/Create() funcs and nowhere else!
        //mapId/instanceId should be set in SetMap() function!
        void SetLocationMapId(uint32 _mapId) { m_mapId = _mapId; }
        void SetLocationInstanceId(uint32 _instanceId) { m_InstanceId = _instanceId; }

        virtual bool IsNeverVisible() const { return !IsInWorld() || IsDestroyedObject(); }
        virtual bool IsAlwaysVisibleFor(WorldObject const* /*seer*/) const { return false; }
        virtual bool IsInvisibleDueToDespawn() const { return false; }
        //difference from IsAlwaysVisibleFor: 1. after distance check; 2. use owner or charmer as seer
        virtual bool IsAlwaysDetectableFor(WorldObject const* /*seer*/) const { return false; }

        virtual void Heartbeat() { }
    private:
        Map* m_currMap;                                   // current object's Map location

        uint32 m_InstanceId;                              // in map copy with instance id
        PhaseShift _phaseShift;
        PhaseShift _suppressedPhaseShift;                 // contains phases for current area but not applied due to conditions
        int32 _dbPhase;

        uint16 m_notifyflags;

        ObjectGuid _privateObjectOwner;

        Milliseconds _heartbeatTimer;

        virtual bool _IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D, bool incOwnRadius = true, bool incTargetRadius = true) const;

        bool CanNeverSee(WorldObject const* obj) const;
        virtual bool CanAlwaysSee(WorldObject const* /*obj*/) const { return false; }
        bool CanDetect(WorldObject const* obj, bool ignoreStealth, bool checkAlert = false) const;
        bool CanDetectInvisibilityOf(WorldObject const* obj) const;
        bool CanDetectStealthOf(WorldObject const* obj, bool checkAlert = false) const;

        uint16 m_aiAnimKitId;
        uint16 m_movementAnimKitId;
        uint16 m_meleeAnimKitId;
};

namespace Trinity
{
    // Binary predicate to sort WorldObjects based on the distance to a reference WorldObject
    class ObjectDistanceOrderPred
    {
        public:
            ObjectDistanceOrderPred(WorldObject const* refObj, bool ascending = true) : _refObj(refObj), _ascending(ascending) { }

            bool operator()(WorldObject const* left, WorldObject const* right) const
            {
                return _refObj->GetDistanceOrder(left, right) == _ascending;
            }

        private:
            WorldObject const* _refObj;
            bool _ascending;
    };
}

#endif
