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

#ifndef TRINITYCORE_CREATURE_H
#define TRINITYCORE_CREATURE_H

#include "Unit.h"
#include "Common.h"
#include "CreatureData.h"
#include "DatabaseEnvFwd.h"
#include "Duration.h"
#include "Loot.h"
#include "MapObject.h"

#include <list>

class CreatureAI;
class CreatureGroup;
class Group;
class Quest;
class Player;
class SpellInfo;
class WorldSession;

enum MovementGeneratorType : uint8;

struct VendorItemCount
{
    VendorItemCount(uint32 _item, uint32 _count);

    uint32 itemId;
    uint32 count;
    time_t lastIncrementTime;
};

typedef std::list<VendorItemCount> VendorItemCounts;

// max different by z coordinate for creature aggro reaction
#define CREATURE_Z_ATTACK_RANGE 3

#define MAX_VENDOR_ITEMS 150                                // Limitation in 4.x.x item count in SMSG_VENDOR_INVENTORY
static constexpr uint8 VENDOR_INVENTORY_REASON_INVENTORY_EMPTY = 1;

//used for handling non-repeatable random texts
typedef std::vector<uint8> CreatureTextRepeatIds;
typedef std::unordered_map<uint8, CreatureTextRepeatIds> CreatureTextRepeatGroup;

class TC_GAME_API Creature : public Unit, public GridObject<Creature>, public MapObject
{
    public:
        explicit Creature(bool isWorldObject = false);

        void AddToWorld() override;
        void RemoveFromWorld() override;

        float GetNativeObjectScale() const override;
        void SetObjectScale(float scale) override;
        void SetDisplayId(uint32 modelId, bool setNative = false) override;
        void SetDisplayFromModel(uint32 modelIdx);

        void DisappearAndDie();

        bool Create(ObjectGuid::LowType guidlow, Map* map, uint32 entry, Position const& pos, CreatureData const* data = nullptr, uint32 vehId = 0, bool dynamic = false);
        bool LoadCreaturesAddon();
        void SelectLevel();
        void UpdateLevelDependantStats();
        void LoadEquipment(int8 id = 1, bool force = false);
        void SetSpawnHealth();

        ObjectGuid::LowType GetSpawnId() const { return m_spawnId; }

        void Update(uint32 time) override;                         // overwritten Unit::Update
        void Heartbeat() override;

        void GetRespawnPosition(float &x, float &y, float &z, float* ori = nullptr, float* dist = nullptr) const;
        bool IsSpawnedOnTransport() const { return m_creatureData && m_creatureData->mapId != GetMapId(); }

        void SetCorpseDelay(uint32 delay) { m_corpseDelay = delay; }
        uint32 GetCorpseDelay() const { return m_corpseDelay; }
        bool IsRacialLeader() const { return GetCreatureTemplate()->RacialLeader; }
        bool IsCivilian() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_CIVILIAN) != 0; }
        bool IsTrigger() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_TRIGGER) != 0; }
        bool IsGuard() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_GUARD) != 0; }

        void InitializeMovementCapabilities();
        void UpdateMovementCapabilities();

        CreatureMovementData const& GetMovementTemplate() const;

        // Returns true if CREATURE_STATIC_FLAG_AQUATIC is set which strictly binds the creature to liquids
        bool IsAquatic() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_AQUATIC); }

        // Returns true if CREATURE_STATIC_FLAG_AMPHIBIOUS is set which allows a creature to enter and leave liquids while sticking to the ocean floor. These creatures will become able to swim when engaged
        bool IsAmphibious() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_AMPHIBIOUS); }

        // Returns true if CREATURE_STATIC_FLAG_FLOATING is set which is  disabling the gravity of the creature on spawn and reset
        bool IsFloating() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_FLOATING); }
        void SetFloating(bool floating) { _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_FLOATING, floating); SetDisableGravity(floating); }

        // Returns true if CREATURE_STATIC_FLAG_SESSILE is set which permanently roots the creature in place
        bool IsSessile() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_SESSILE); }
        void SetSessile(bool sessile) { _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_SESSILE, sessile); SetControlled(sessile, UNIT_STATE_ROOT); }

        // Returns true if CREATURE_STATIC_FLAG_3_CANNOT_PENETRATE_WATER is set which does not allow the creature to go below liquid surfaces
        bool CannotPenetrateWater() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_3_CANNOT_PENETRATE_WATER); }
        void SetCannotPenetrateWater(bool cannotPenetrateWater) { _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_3_CANNOT_PENETRATE_WATER, cannotPenetrateWater); }

        // Returns true if CREATURE_STATIC_FLAG_3_CANNOT_SWIM is set which prevents 'Amphibious' creatures from swimming when engaged
        bool IsSwimDisabled() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_3_CANNOT_SWIM); }

        // Returns true if CREATURE_STATIC_FLAG_4_PREVENT_SWIM is set which prevents 'Amphibious' creatures from swimming when engaged until the victim is no longer on the ocean floor
        bool IsSwimPrevented() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_4_PREVENT_SWIM); }

        bool CanSwim() const override;
        bool CanEnterWater() const override { return (CanSwim() || IsAmphibious()); };
        bool CanFly()  const override { return (IsFlying() || HasUnitMovementFlag(MOVEMENTFLAG_CAN_FLY)); }

        bool SetDisableGravity(bool disable, bool updateAnimTier = true) override;
        bool SetHover(bool enable, bool updateAnimTier = true) override;
        bool SetCanFly(bool enable) override;

        bool IsDungeonBoss() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_DUNGEON_BOSS) != 0; }
        bool IsAffectedByDiminishingReturns() const override { return Unit::IsAffectedByDiminishingReturns() || (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_ALL_DIMINISH) != 0; }

        Unit* SelectVictim();

        void SetReactState(ReactStates st) { m_reactState = st; }
        ReactStates GetReactState() const { return m_reactState; }
        bool HasReactState(ReactStates state) const { return (m_reactState == state); }
        void InitializeReactState();

        using Unit::IsImmuneToAll;
        using Unit::SetImmuneToAll;
        void SetImmuneToAll(bool apply) override { Unit::SetImmuneToAll(apply, HasReactState(REACT_PASSIVE)); }
        using Unit::IsImmuneToPC;
        using Unit::SetImmuneToPC;
        void SetImmuneToPC(bool apply) override { Unit::SetImmuneToPC(apply, HasReactState(REACT_PASSIVE)); }
        using Unit::IsImmuneToNPC;
        using Unit::SetImmuneToNPC;
        void SetImmuneToNPC(bool apply) override { Unit::SetImmuneToNPC(apply, HasReactState(REACT_PASSIVE)); }

        void SetUnkillable(bool unkillable) { _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_UNKILLABLE, unkillable);}

        /// @todo Rename these properly
        bool isCanInteractWithBattleMaster(Player* player, bool msg) const;
        bool CanResetTalents(Player* player) const;
        bool IsClassTrainerOf(Player const* player) const;
        bool CanCreatureAttack(Unit const* victim, bool force = true) const;
        void LoadTemplateImmunities();
        bool IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index, WorldObject const* caster, bool requireImmunityPurgesEffectAttribute = false) const override;
        bool isElite() const;
        bool isWorldBoss() const;

        uint8 getLevelForTarget(WorldObject const* target) const override; // overwrite Unit::getLevelForTarget for boss level support

        bool IsInEvadeMode() const { return HasUnitState(UNIT_STATE_EVADE); }
        bool IsEvadingAttacks() const { return IsInEvadeMode() || CanNotReachTarget(); }

        bool IsStateRestoredOnEvade() const { return !_staticFlags.HasFlag(CREATURE_STATIC_FLAG_5_NO_LEAVECOMBAT_STATE_RESTORE); }
        void SetRestoreStateOnEvade(bool restoreOnEvade) { _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_5_NO_LEAVECOMBAT_STATE_RESTORE, !restoreOnEvade); }

        bool AIM_Destroy();
        bool AIM_Create(CreatureAI* ai = nullptr);
        bool AIM_Initialize(CreatureAI* ai = nullptr);
        void Motion_Initialize();

        CreatureAI* AI() const { return reinterpret_cast<CreatureAI*>(GetAI()); }

        SpellSchoolMask GetMeleeDamageSchoolMask(WeaponAttackType /*attackType*/ = BASE_ATTACK) const override { return m_meleeDamageSchoolMask; }
        void SetMeleeDamageSchool(SpellSchools school) { m_meleeDamageSchoolMask = SpellSchoolMask(1 << school); }
        bool CanMelee() const { return !_staticFlags.HasFlag(CREATURE_STATIC_FLAG_NO_MELEE); }
        void SetCanMelee(bool canMelee) { _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_NO_MELEE, !canMelee); }
        bool CanIgnoreLineOfSightWhenCastingOnMe() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_4_IGNORE_LOS_WHEN_CASTING_ON_ME); }

        void DisableLoot(bool disable) { _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_NO_LOOT, !disable); }
        bool IsLootDisabled() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_NO_LOOT); }

        bool HasSpell(uint32 spellID) const override;

        bool UpdateEntry(uint32 entry, CreatureData const* data = nullptr, bool updateLevel = true);

        bool UpdateStats(Stats stat) override;
        bool UpdateAllStats() override;
        void UpdateResistances(uint32 school) override;
        void UpdateArmor() override;
        void UpdateMaxHealth() override;
        void UpdateMaxPower(Powers power) override;
        uint32 GetPowerIndex(Powers power) const override;
        void UpdateAttackPowerAndDamage(bool ranged = false) override;
        void CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& minDamage, float& maxDamage) const override;

        void SetCanDualWield(bool value) override;
        int8 GetOriginalEquipmentId() const { return m_originalEquipmentId; }
        uint8 GetCurrentEquipmentId() const { return m_equipmentId; }
        void SetCurrentEquipmentId(uint8 id) { m_equipmentId = id; }

        float GetSpellDamageMod(int32 Rank) const;

        VendorItemData const* GetVendorItems() const;
        uint32 GetVendorItemCurrentCount(VendorItem const* vItem);
        uint32 UpdateVendorItemCurrentCount(VendorItem const* vItem, uint32 used_count);

        CreatureTemplate const* GetCreatureTemplate() const { return m_creatureInfo; }
        CreatureData const* GetCreatureData() const { return m_creatureData; }
        CreatureAddon const* GetCreatureAddon() const;

        std::string const& GetAIName() const;
        std::string GetScriptName() const;
        uint32 GetScriptId() const;

        // override WorldObject function for proper name localization
        std::string const& GetNameForLocaleIdx(LocaleConstant locale_idx) const override;

        void setDeathState(DeathState s) override;                   // override virtual Unit::setDeathState

        bool LoadFromDB(ObjectGuid::LowType spawnId, Map* map, bool addToMap, bool allowDuplicate);
        void SaveToDB();
                                                            // overriden in Pet
        virtual void SaveToDB(uint32 mapid, uint8 spawnMask);
        static bool DeleteFromDB(ObjectGuid::LowType spawnId);

        Loot loot;
        void StartPickPocketRefillTimer();
        void ResetPickPocketRefillTimer() { _pickpocketLootRestore = 0; }
        bool CanGeneratePickPocketLoot() const;
        ObjectGuid GetLootRecipientGUID() const { return m_lootRecipient; }
        Player* GetLootRecipient() const;
        Group* GetLootRecipientGroup() const;
        bool hasLootRecipient() const { return !m_lootRecipient.IsEmpty() || m_lootRecipientGroup; }
        bool isTappedBy(Player const* player) const;                          // return true if the creature is tapped by the player or a member of his party.

        void SetLootRecipient(Unit* unit, bool withGroup = true);
        void AllLootRemovedFromCorpse();

        uint16 GetLootMode() const { return m_LootMode; }
        bool HasLootMode(uint16 lootMode) { return (m_LootMode & lootMode) != 0; }
        void SetLootMode(uint16 lootMode) { m_LootMode = lootMode; }
        void AddLootMode(uint16 lootMode) { m_LootMode |= lootMode; }
        void RemoveLootMode(uint16 lootMode) { m_LootMode &= ~lootMode; }
        void ResetLootMode() { m_LootMode = LOOT_MODE_DEFAULT; }

        SpellInfo const* reachWithSpellAttack(Unit* victim);
        SpellInfo const* reachWithSpellCure(Unit* victim);

        std::array<uint32, MAX_CREATURE_SPELLS> m_spells;

        bool CanStartAttack(Unit const* u, bool force) const;
        float GetAttackDistance(Unit const* player) const;
        float GetAggroRange(Unit const* target) const;

        void SendAIReaction(AiReaction reactionType);

        Unit* SelectNearestTarget(float dist = 0, bool playerOnly = false) const;
        Unit* SelectNearestTargetInAttackDistance(float dist = 0) const;
        Unit* SelectNearestHostileUnitInAggroRange(bool useLOS = false, bool ignoreCivilians = false) const;

        void DoFleeToGetAssistance();
        void CallForHelp(float fRadius);
        void CallAssistance();
        void SetNoCallAssistance(bool val) { m_AlreadyCallAssistance = val; }
        void SetNoSearchAssistance(bool val) { m_AlreadySearchedAssistance = val; }
        bool HasSearchedAssistance() const { return m_AlreadySearchedAssistance; }
        bool CanAssistTo(const Unit* u, const Unit* enemy, bool checkfaction = true) const;
        bool _IsTargetAcceptable(const Unit* target) const;
        bool IsIgnoringFeignDeath() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_2_IGNORE_FEIGN_DEATH); }
        void SetIgnoreFeignDeath(bool ignoreFeignDeath) { _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_2_IGNORE_FEIGN_DEATH, ignoreFeignDeath); }
        bool IsIgnoringSanctuarySpellEffect() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_2_IGNORE_SANCTUARY); }
        void SetIgnoreSanctuarySpellEffect(bool ignoreSanctuary) { _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_2_IGNORE_SANCTUARY, ignoreSanctuary); }

        MovementGeneratorType GetDefaultMovementType() const { return m_defaultMovementType; }
        void SetDefaultMovementType(MovementGeneratorType mgt) { m_defaultMovementType = mgt; }

        void RemoveCorpse(bool setSpawnTime = true, bool destroyForNearbyPlayers = true);

        void DespawnOrUnsummon(uint32 msTimeToDespawn = 0, Seconds forceRespawnTime = 0s);
        void DespawnOrUnsummon(Milliseconds time, Seconds forceRespawnTime = 0s) { DespawnOrUnsummon(uint32(time.count()), forceRespawnTime); }

        time_t const& GetRespawnTime() const { return m_respawnTime; }
        time_t GetRespawnTimeEx() const;
        void SetRespawnTime(uint32 respawn);
        void Respawn(bool force = false);
        void SaveRespawnTime(uint32 forceDelay = 0);

        uint32 GetRespawnDelay() const { return m_respawnDelay; }
        void SetRespawnDelay(uint32 delay) { m_respawnDelay = delay; }

        float GetWanderDistance() const { return m_wanderDistance; }
        void SetWanderDistance(float dist) { m_wanderDistance = dist; }

        void DoImmediateBoundaryCheck() { m_boundaryCheckTime = 0; }

        uint32 m_groupLootTimer;                            // (msecs)timer used for group loot
        ObjectGuid::LowType lootingGroupLowGUID;                         // used to find group which is looting corpse

        void SendZoneUnderAttackMessage(Player* attacker);

        bool hasQuest(uint32 quest_id) const override;
        bool hasInvolvedQuest(uint32 quest_id)  const override;

        bool isRegeneratingHealth() { return m_regenHealth; }
        void setRegeneratingHealth(bool regenHealth) { m_regenHealth = regenHealth; }
        virtual uint8 GetPetAutoSpellSize() const { return MAX_SPELL_CHARM; }
        virtual uint32 GetPetAutoSpellOnPos(uint8 pos) const;
        float GetPetChaseDistance() const;

        void SetCannotReachTarget(bool cannotReach) { if (cannotReach == m_cannotReachTarget) return; m_cannotReachTarget = cannotReach; m_cannotReachTimer = 0; }
        bool CanNotReachTarget() const { return m_cannotReachTarget; }

        void SetDefaultMount(Optional<uint32> mountCreatureDisplayId);

        void SetPosition(float x, float y, float z, float o);
        void SetPosition(const Position &pos) { SetPosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()); }

        void SetHomePosition(float x, float y, float z, float o) { m_homePosition.Relocate(x, y, z, o); }
        void SetHomePosition(const Position &pos) { m_homePosition.Relocate(pos); }
        void GetHomePosition(float& x, float& y, float& z, float& ori) const { m_homePosition.GetPosition(x, y, z, ori); }
        Position const& GetHomePosition() const { return m_homePosition; }

        void SetTransportHomePosition(float x, float y, float z, float o) { m_transportHomePosition.Relocate(x, y, z, o); }
        void SetTransportHomePosition(const Position &pos) { m_transportHomePosition.Relocate(pos); }
        void GetTransportHomePosition(float& x, float& y, float& z, float& ori) const { m_transportHomePosition.GetPosition(x, y, z, ori); }
        Position const& GetTransportHomePosition() const { return m_transportHomePosition; }

        uint32 GetWaypointPath() const { return _waypointPathId; }
        void LoadPath(uint32 pathid) { _waypointPathId = pathid; }

        uint32 GetCyclicSplinePathId() const { return _cyclicSplinePathId; }

        // nodeId, pathId
        std::pair<uint32, uint32> GetCurrentWaypointInfo() const { return _currentWaypointNodeInfo; }
        void UpdateCurrentWaypointInfo(uint32 nodeId, uint32 pathId) { _currentWaypointNodeInfo = { nodeId, pathId }; }

        bool IsReturningHome() const;

        void SearchFormation();
        CreatureGroup* GetFormation() { return m_formation; }
        void SetFormation(CreatureGroup* formation) { m_formation = formation; }
        bool IsFormationLeader() const;
        void SignalFormationMovement();
        bool IsFormationLeaderMoveAllowed() const;

        void SetDisableReputationGain(bool disable) { DisableReputationGain = disable; }
        bool IsReputationGainDisabled() const { return DisableReputationGain; }
        bool IsDamageEnoughForLootingAndReward() const { return (m_creatureInfo->flags_extra & CREATURE_FLAG_EXTRA_NO_PLAYER_DAMAGE_REQ) || (m_PlayerDamageReq == 0); }
        void LowerPlayerDamageReq(uint32 unDamage);
        void ResetPlayerDamageReq() { m_PlayerDamageReq = GetHealth() / 2; }
        uint32 m_PlayerDamageReq;

        uint32 GetOriginalEntry() const { return m_originalEntry; }
        void SetOriginalEntry(uint32 entry) { m_originalEntry = entry; }

        // There's many places not ready for dynamic spawns. This allows them to live on for now.
        void SetRespawnCompatibilityMode(bool mode = true) { m_respawnCompatibilityMode = mode; }
        bool GetRespawnCompatibilityMode() { return m_respawnCompatibilityMode; }

        static float _GetDamageMod(int32 Rank);

        float m_SightDistance;
        float m_CombatDistance;

        bool m_isTempWorldObject; //true when possessed

        // Handling caster facing during spellcast
        void SetTarget(ObjectGuid guid) override;
        void ReacquireSpellFocusTarget();
        void SetSpellFocus(Spell const* focusSpell, WorldObject const* target);
        bool HasSpellFocus(Spell const* focusSpell = nullptr) const override;
        void ReleaseSpellFocus(Spell const* focusSpell = nullptr, bool withDelay = true);
        void ResetSpellFocusInfo() { _spellFocusInfo.Reset(); }

        bool IsMovementPreventedByCasting() const override;

        // Part of Evade mechanics
        time_t GetLastDamagedTime() const { return _lastDamagedTime; }
        void SetLastDamagedTime(time_t val) { _lastDamagedTime = val; }

        CreatureTextRepeatIds GetTextRepeatGroup(uint8 textGroup);
        void SetTextRepeatId(uint8 textGroup, uint8 id);
        void ClearTextRepeatGroup(uint8 textGroup);
        bool IsEscorted() const;

        bool CanGiveExperience() const;

        void MakeInterruptable(bool apply);

        bool IsEngaged() const override;
        void AtEngage(Unit* target) override;
        void AtDisengage() override;

        bool IsThreatFeedbackDisabled() const { return _staticFlags.HasFlag(CREATURE_STATIC_FLAG_3_NO_THREAT_FEEDBACK); }
        void SetNoThreatFeedback(bool noThreatFeedback) { _staticFlags.ApplyFlag(CREATURE_STATIC_FLAG_3_NO_THREAT_FEEDBACK, noThreatFeedback); }

        void ForcePartyMembersIntoCombat();

        bool HasStaticFlag(CreatureStaticFlags flag) const { return _staticFlags.HasFlag(flag); }
        bool HasStaticFlag(CreatureStaticFlags2 flag) const { return _staticFlags.HasFlag(flag); }
        bool HasStaticFlag(CreatureStaticFlags3 flag) const { return _staticFlags.HasFlag(flag); }
        bool HasStaticFlag(CreatureStaticFlags4 flag) const { return _staticFlags.HasFlag(flag); }
        bool HasStaticFlag(CreatureStaticFlags5 flag) const { return _staticFlags.HasFlag(flag); }

        CreatureMovementInfo const& GetCreatureMovementInfo() const { return _creatureMovementInfo; }

        // Sets the the max health percentage threshold at which uncontrolled/unowned creatures can no longer deal damage to the creature
        void SetNoNpcDamageBelowPctHealthValue(float value) { _noNpcDamageBelowPctHealth = std::clamp<float>(value, 0.f, 100.f); }
        void ResetNoNpcDamageBelowPctHealthValue() { _noNpcDamageBelowPctHealth = 0.f; }
        float GetNoNpcDamageBelowPctHealthValue() const { return _noNpcDamageBelowPctHealth; }

    protected:
        bool CreateFromProto(ObjectGuid::LowType guidlow, uint32 entry, CreatureData const* data = nullptr, uint32 vehId = 0);
        bool InitEntry(uint32 entry, CreatureData const* data = nullptr);

        // vendor items
        VendorItemCounts m_vendorItemCounts;

        static float _GetHealthMod(int32 Rank);

        ObjectGuid m_lootRecipient;
        uint32 m_lootRecipientGroup;

        /// Timers
        time_t _pickpocketLootRestore;
        time_t m_corpseRemoveTime;                          // (msecs)timer for death or corpse disappearance
        time_t m_respawnTime;                               // (secs) time of next respawn
        uint32 m_respawnDelay;                              // (secs) delay between corpse disappearance and respawning
        uint32 m_corpseDelay;                               // (secs) delay between death and corpse disappearance
        float m_wanderDistance;
        uint32 m_boundaryCheckTime;                         // (msecs) remaining time for next evade boundary check

        ReactStates m_reactState;                           // for AI, not charmInfo
        void RegenerateHealth() override;
        MovementGeneratorType m_defaultMovementType;
        ObjectGuid::LowType m_spawnId;                               ///< For new or temporary creatures is 0 for saved it is lowguid
        uint8 m_equipmentId;
        int8 m_originalEquipmentId; // can be -1

        bool m_AlreadyCallAssistance;
        bool m_AlreadySearchedAssistance;
        bool m_regenHealth;
        bool m_cannotReachTarget;
        uint32 m_cannotReachTimer;

        SpellSchoolMask m_meleeDamageSchoolMask;
        uint32 m_originalEntry;

        Position m_homePosition;
        Position m_transportHomePosition;

        bool DisableReputationGain;

        CreatureTemplate const* m_creatureInfo;                 // Can differ from sObjectMgr->GetCreatureTemplate(GetEntry()) in difficulty mode > 0
        CreatureData const* m_creatureData;

        uint16 m_LootMode;                                  // Bitmask (default: LOOT_MODE_DEFAULT) that determines what loot will be lootable

        bool IsInvisibleDueToDespawn() const override;
        bool CanAlwaysSee(WorldObject const* obj) const override;

        // Initializes run and walk speed override data based on movementId override values stored in `creature_movement_info` table
        void InitializeCreatureMovementInfo(uint32 movementId);

        // Initializes move speed fields based on template and override data
        void InitializeMovementSpeeds();

    private:
        void ForcedDespawn(uint32 timeMSToDespawn = 0, Seconds forceRespawnTimer = 0s);
        bool CheckNoGrayAggroConfig(uint32 playerLevel, uint32 creatureLevel) const; // No aggro from gray creatures

        // Waypoint path
        uint32 _waypointPathId;
        std::pair<uint32/*nodeId*/, uint32/*pathId*/> _currentWaypointNodeInfo;

        // Cyclic spline path
        uint32 _cyclicSplinePathId;

        //Formation var
        CreatureGroup* m_formation;
        bool m_triggerJustAppeared;
        bool m_respawnCompatibilityMode;

        // Spell Focusing
        CreatureSpellFocusData _spellFocusInfo;

        time_t _lastDamagedTime; // Part of Evade mechanics
        CreatureTextRepeatGroup m_textRepeat;

        // Draws data from m_creatureDifficulty and spawn/difficulty based override data and returns a CreatureStaticFlagsHolder value which contains the data of both
        CreatureStaticFlagsHolder GenerateStaticFlags(CreatureTemplate const* cInfo, ObjectGuid::LowType spawnId, Difficulty difficultyId) const;
        void ApplyAllStaticFlags(CreatureStaticFlagsHolder const& flags);

        CreatureStaticFlagsHolder _staticFlags;

        bool _isMissingSwimmingFlagOutOfCombat;

        CreatureMovementInfo _creatureMovementInfo;

        Optional<uint32> _defaultMountDisplayIdOverride;
        float _noNpcDamageBelowPctHealth;
};

class TC_GAME_API AssistDelayEvent : public BasicEvent
{
    public:
        AssistDelayEvent(ObjectGuid victim, Unit& owner) : BasicEvent(), m_victim(victim), m_owner(owner) { }

        bool Execute(uint64 e_time, uint32 p_time) override;
        void AddAssistant(ObjectGuid guid) { m_assistants.push_back(guid); }
    private:
        AssistDelayEvent();

        ObjectGuid        m_victim;
        GuidList          m_assistants;
        Unit&             m_owner;
};

class TC_GAME_API ForcedDespawnDelayEvent : public BasicEvent
{
    public:
        ForcedDespawnDelayEvent(Creature& owner, Seconds respawnTimer) : BasicEvent(), m_owner(owner), m_respawnTimer(respawnTimer) { }
        bool Execute(uint64 e_time, uint32 p_time) override;

    private:
        Creature& m_owner;
        Seconds const m_respawnTimer;
};

#endif
