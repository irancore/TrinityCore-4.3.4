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

#ifndef _PLAYER_H
#define _PLAYER_H

#include "Unit.h"
#include "CUFProfile.h"
#include "DatabaseEnvFwd.h"
#include "DBCEnums.h"
#include "EquipmentSet.h"
#include "GroupReference.h"
#include "ItemDefines.h"
#include "ItemEnchantmentMgr.h"
#include "MapReference.h"
#include "PetDefines.h"
#include "PlayerTaxi.h"
#include "QuestDef.h"

struct AccessRequirement;
struct AchievementEntry;
struct AreaTableEntry;
struct AreaTriggerEntry;
struct ArchData;
struct BarberShopStyleEntry;
struct CharacterCustomizeInfo;
struct CharTitlesEntry;
struct ChatChannelsEntry;
struct CreatureTemplate;
struct CurrencyTypesEntry;
struct FactionEntry;
struct ItemExtendedCostEntry;
struct ItemSetEffect;
struct ItemTemplate;
struct Loot;
struct Mail;
struct ScalingStatDistributionEntry;
struct ScalingStatValuesEntry;
struct TrainerSpell;
struct VendorItem;
struct GuildMemberProfessionData;

template<class T>
class AchievementMgr;

class Archaeology;
class Bag;
class Battleground;
class CinematicMgr;
class Channel;
class CharacterCreateInfo;
class Creature;
class DynamicObject;
class GameClient;
class Group;
class Guild;
class Item;
class LootStore;
class OutdoorPvP;
class Pet;
class PetAura;
class PlayerAI;
class PlayerMenu;
class PlayerSocial;
class ReputationMgr;
class SpellCastTargets;
class TradeData;

enum InventoryType : uint8;
enum ItemClass : uint8;
enum LootError : uint8;
enum LootType : uint8;
enum UpdateCollisionHeightReason : uint8;
enum ZLiquidStatus : uint32;

typedef std::deque<Mail*> PlayerMails;

#define PLAYER_MAX_SKILLS           128
#define PLAYER_MAX_DAILY_QUESTS     25
#define PLAYER_EXPLORED_ZONES_SIZE  156

enum SpellModType : uint8
{
    SPELLMOD_FLAT         = 0,                            // SPELL_AURA_ADD_FLAT_MODIFIER
    SPELLMOD_PCT          = 1,                            // SPELL_AURA_ADD_PCT_MODIFIER
    SPELLMOD_END
};

// 2^n values, Player::m_isunderwater is a bitmask. These are Trinity internal values, they are never send to any client
enum PlayerUnderwaterState
{
    UNDERWATER_NONE                     = 0x00,
    UNDERWATER_INWATER                  = 0x01,             // terrain type is water and player is afflicted by it
    UNDERWATER_INLAVA                   = 0x02,             // terrain type is lava and player is afflicted by it
    UNDERWATER_INSLIME                  = 0x04,             // terrain type is lava and player is afflicted by it
    UNDERWATER_INDARKWATER              = 0x08,             // terrain type is dark water and player is afflicted by it

    UNDERWATER_EXIST_TIMERS             = 0x10
};

enum BuyBankSlotResult
{
    ERR_BANKSLOT_FAILED_TOO_MANY    = 0,
    ERR_BANKSLOT_INSUFFICIENT_FUNDS = 1,
    ERR_BANKSLOT_NOTBANKER          = 2,
    ERR_BANKSLOT_OK                 = 3
};

enum PlayerSpellState
{
    PLAYERSPELL_UNCHANGED = 0,
    PLAYERSPELL_CHANGED   = 1,
    PLAYERSPELL_NEW       = 2,
    PLAYERSPELL_REMOVED   = 3,
    PLAYERSPELL_TEMPORARY = 4
};

struct PlayerSpell
{
    PlayerSpellState state : 8;
    bool active            : 1;                             // show in spellbook
    bool dependent         : 1;                             // learned as result another spell learn, skill grow, quest reward, etc
    bool disabled          : 1;                             // first rank has been learned in result talent learn but currently talent unlearned, save max learned ranks
};

struct PlayerTalent
{
    PlayerSpellState state : 8;
    uint8 spec             : 8;
};

extern uint32 const MasterySpells[MAX_CLASSES];

enum TalentTree // talent tabs
{
    TALENT_TREE_WARRIOR_ARMS         = 746,
    TALENT_TREE_WARRIOR_FURY         = 815,
    TALENT_TREE_WARRIOR_PROTECTION   = 845,
    TALENT_TREE_PALADIN_HOLY         = 831,
    TALENT_TREE_PALADIN_PROTECTION   = 839,
    TALENT_TREE_PALADIN_RETRIBUTION  = 855,
    TALENT_TREE_HUNTER_BEAST_MASTERY = 811,
    TALENT_TREE_HUNTER_MARKSMANSHIP  = 807,
    TALENT_TREE_HUNTER_SURVIVAL      = 809,
    TALENT_TREE_ROGUE_ASSASSINATION  = 182,
    TALENT_TREE_ROGUE_COMBAT         = 181,
    TALENT_TREE_ROGUE_SUBTLETY       = 183,
    TALENT_TREE_PRIEST_DISCIPLINE    = 760,
    TALENT_TREE_PRIEST_HOLY          = 813,
    TALENT_TREE_PRIEST_SHADOW        = 795,
    TALENT_TREE_DEATH_KNIGHT_BLOOD   = 398,
    TALENT_TREE_DEATH_KNIGHT_FROST   = 399,
    TALENT_TREE_DEATH_KNIGHT_UNHOLY  = 400,
    TALENT_TREE_SHAMAN_ELEMENTAL     = 261,
    TALENT_TREE_SHAMAN_ENHANCEMENT   = 263,
    TALENT_TREE_SHAMAN_RESTORATION   = 262,
    TALENT_TREE_MAGE_ARCANE          = 799,
    TALENT_TREE_MAGE_FIRE            = 851,
    TALENT_TREE_MAGE_FROST           = 823,
    TALENT_TREE_WARLOCK_AFFLICTION   = 871,
    TALENT_TREE_WARLOCK_DEMONOLOGY   = 867,
    TALENT_TREE_WARLOCK_DESTRUCTION  = 865,
    TALENT_TREE_DRUID_BALANCE        = 752,
    TALENT_TREE_DRUID_FERAL_COMBAT   = 750,
    TALENT_TREE_DRUID_RESTORATION    = 748
};

#define MAX_ARMOR_SPECIALIZATION_IDS 7
uint32 const ArmorSpecializationIds[MAX_ARMOR_SPECIALIZATION_IDS] = { 86530, 86531, 86529, 86528, 86525, 86524, 86526 };

// Spell modifier (used for modify other spells)
struct SpellModifier
{
    SpellModifier(Aura* _ownerAura) : op(SpellModOp::HealingAndDamage), type(SPELLMOD_FLAT), spellId(0), ownerAura(_ownerAura) { }
    virtual ~SpellModifier() = default;

    SpellModOp op;
    SpellModType type;

    uint32 spellId;
    Aura* const ownerAura;
};

struct SpellModifierByClassMask : SpellModifier
{
    SpellModifierByClassMask(Aura* _ownerAura) : SpellModifier(_ownerAura), value(0), mask() { }

    int32 value;
    flag96 mask;
};

struct SpellModifierCompare
{
    bool operator()(SpellModifier const* left, SpellModifier const* right) const
    {
        // first sort by SpellModOp
        if (left->op != right->op)
            return left->op < right->op;

        // then by type (flat/pct)
        if (left->type != right->type)
            return left->type < right->type;

        return left < right;
    }
};


enum PlayerCurrencyState
{
   PLAYERCURRENCY_UNCHANGED = 0,
   PLAYERCURRENCY_CHANGED   = 1,
   PLAYERCURRENCY_NEW       = 2,
   PLAYERCURRENCY_REMOVED   = 3     //not removed just set count == 0
};

struct PlayerCurrency
{
    PlayerCurrency() : State(PLAYERCURRENCY_NEW), Quantity(0), WeeklyQuantity(0), TrackedQuantity(0), Flags(0) { }

    PlayerCurrencyState State;
    uint32 Quantity;
    uint32 WeeklyQuantity;
    uint32 TrackedQuantity;
    uint8 Flags;
};

typedef std::unordered_map<uint32, PlayerTalent*> PlayerTalentMap;
typedef std::unordered_map<uint32, PlayerSpell> PlayerSpellMap;
typedef Trinity::Containers::FlatSet<SpellModifier*, SpellModifierCompare> SpellModContainer;
typedef std::unordered_map<uint32, PlayerCurrency> PlayerCurrenciesMap;

typedef std::unordered_map<uint32 /*instanceId*/, time_t/*releaseTime*/> InstanceTimeMap;

enum ActionButtonUpdateState
{
    ACTIONBUTTON_UNCHANGED = 0,
    ACTIONBUTTON_CHANGED   = 1,
    ACTIONBUTTON_NEW       = 2,
    ACTIONBUTTON_DELETED   = 3
};

enum ActionButtonType
{
    ACTION_BUTTON_SPELL     = 0x00,
    ACTION_BUTTON_C         = 0x01,                         // click?
    ACTION_BUTTON_EQSET     = 0x20,
    ACTION_BUTTON_DROPDOWN  = 0x30,
    ACTION_BUTTON_MACRO     = 0x40,
    ACTION_BUTTON_CMACRO    = ACTION_BUTTON_C | ACTION_BUTTON_MACRO,
    ACTION_BUTTON_ITEM      = 0x80
};

enum ReputationSource
{
    REPUTATION_SOURCE_KILL,
    REPUTATION_SOURCE_QUEST,
    REPUTATION_SOURCE_DAILY_QUEST,
    REPUTATION_SOURCE_WEEKLY_QUEST,
    REPUTATION_SOURCE_MONTHLY_QUEST,
    REPUTATION_SOURCE_REPEATABLE_QUEST,
    REPUTATION_SOURCE_SPELL
};

#define ACTION_BUTTON_ACTION(X) (uint32(X) & 0x00FFFFFF)
#define ACTION_BUTTON_TYPE(X)   ((uint32(X) & 0xFF000000) >> 24)
#define MAX_ACTION_BUTTON_ACTION_VALUE (0x00FFFFFF+1)

struct ActionButton
{
    ActionButton() : packedData(0), uState(ACTIONBUTTON_NEW) { }

    uint32 packedData;
    ActionButtonUpdateState uState;

    // helpers
    ActionButtonType GetType() const { return ActionButtonType(ACTION_BUTTON_TYPE(packedData)); }
    uint32 GetAction() const { return ACTION_BUTTON_ACTION(packedData); }
    void SetActionAndType(uint32 action, ActionButtonType type)
    {
        uint32 newData = action | (uint32(type) << 24);
        if (newData != packedData || uState == ACTIONBUTTON_DELETED)
        {
            packedData = newData;
            if (uState != ACTIONBUTTON_NEW)
                uState = ACTIONBUTTON_CHANGED;
        }
    }
};

#define  MAX_ACTION_BUTTONS 144                             //checked in 3.2.0

typedef std::map<uint8, ActionButton> ActionButtonList;

struct PvPInfo
{
    PvPInfo() : IsHostile(false), IsInHostileArea(false), IsInNoPvPArea(false), IsInFFAPvPArea(false), EndTimer(0) { }

    bool IsHostile;
    bool IsInHostileArea;               ///> Marks if player is in an area which forces PvP flag
    bool IsInNoPvPArea;                 ///> Marks if player is in a sanctuary or friendly capital city
    bool IsInFFAPvPArea;                ///> Marks if player is in an FFAPvP area (such as Gurubashi Arena)
    time_t EndTimer;                    ///> Time when player unflags himself for PvP (flag removed after 5 minutes)
};

struct DuelInfo
{
    DuelInfo() : initiator(nullptr), opponent(nullptr), startTimer(0), startTime(0), outOfBound(0), isMounted(false), isCompleted(false) { }

    Player* initiator;
    Player* opponent;
    time_t startTimer;
    time_t startTime;
    time_t outOfBound;
    bool isMounted;
    bool isCompleted;
};

struct Areas
{
    uint32 areaID;
    uint32 areaFlag;
    float x1;
    float x2;
    float y1;
    float y2;
};

#define MAX_RUNES       6

enum RuneCooldowns
{
    RUNE_BASE_COOLDOWN  = 10000,
    RUNE_MISS_COOLDOWN  = 1500     // cooldown applied on runes when the spell misses
};

enum RuneType : uint8
{
    RUNE_BLOOD      = 0,
    RUNE_UNHOLY     = 1,
    RUNE_FROST      = 2,
    RUNE_DEATH      = 3,
    NUM_RUNE_TYPES  = 4
};

struct RuneInfo
{
    uint8 BaseRune;
    uint8 CurrentRune;
    uint32 Cooldown;
    AuraEffect const* ConvertAura;
    AuraType ConvertAuraType;
    SpellInfo const* ConvertAuraInfo;
};

struct Runes
{
    RuneInfo runes[MAX_RUNES];
    uint8 runeState;                                        // mask of available runes
    RuneType lastUsedRune;
    uint8 lastUsedRuneMask;

    void SetRuneState(uint8 index, bool set = true)
    {
        if (set)
            runeState |= (1 << index);                      // usable
        else
            runeState &= ~(1 << index);                     // on cooldown
    }
};

struct EnchantDuration
{
    EnchantDuration() : item(nullptr), slot(MAX_ENCHANTMENT_SLOT), leftduration(0) { }
    EnchantDuration(Item* _item, EnchantmentSlot _slot, uint32 _leftduration) : item(_item), slot(_slot),
        leftduration(_leftduration){ ASSERT(item); }

    Item* item;
    EnchantmentSlot slot;
    uint32 leftduration;
};

typedef std::list<EnchantDuration> EnchantDurationList;
typedef std::list<Item*> ItemDurationList;

enum DrunkenState
{
    DRUNKEN_SOBER   = 0,
    DRUNKEN_TIPSY   = 1,
    DRUNKEN_DRUNK   = 2,
    DRUNKEN_SMASHED = 3
};

#define MAX_DRUNKEN   4

enum PlayerFlags
{
    PLAYER_FLAGS_GROUP_LEADER                       = 0x00000001,
    PLAYER_FLAGS_AFK                                = 0x00000002,
    PLAYER_FLAGS_DND                                = 0x00000004,
    PLAYER_FLAGS_GM                                 = 0x00000008,
    PLAYER_FLAGS_GHOST                              = 0x00000010,
    PLAYER_FLAGS_RESTING                            = 0x00000020,
    PLAYER_FLAGS_UNK6                               = 0x00000040,
    PLAYER_FLAGS_UNK7                               = 0x00000080,       // pre-3.0.3 PLAYER_FLAGS_FFA_PVP flag for FFA PVP state
    PLAYER_FLAGS_CONTESTED_PVP                      = 0x00000100,       // Player has been involved in a PvP combat and will be attacked by contested guards
    PLAYER_FLAGS_IN_PVP                             = 0x00000200,
    PLAYER_FLAGS_HIDE_HELM                          = 0x00000400,
    PLAYER_FLAGS_HIDE_CLOAK                         = 0x00000800,
    PLAYER_FLAGS_PLAYED_LONG_TIME                   = 0x00001000,       // played long time
    PLAYER_FLAGS_PLAYED_TOO_LONG                    = 0x00002000,       // played too long time
    PLAYER_FLAGS_IS_OUT_OF_BOUNDS                   = 0x00004000,
    PLAYER_FLAGS_DEVELOPER                          = 0x00008000,       // <Dev> prefix for something?
    PLAYER_FLAGS_UNK16                              = 0x00010000,       // pre-3.0.3 PLAYER_FLAGS_SANCTUARY flag for player entered sanctuary
    PLAYER_FLAGS_TAXI_BENCHMARK                     = 0x00020000,       // taxi benchmark mode (on/off) (2.0.1)
    PLAYER_FLAGS_PVP_TIMER                          = 0x00040000,       // 3.0.2, pvp timer active (after you disable pvp manually)
    PLAYER_FLAGS_UBER                               = 0x00080000,
    PLAYER_FLAGS_UNK20                              = 0x00100000,
    PLAYER_FLAGS_UNK21                              = 0x00200000,
    PLAYER_FLAGS_COMMENTATOR2                       = 0x00400000,
    PLAYER_FLAGS_DISABLE_CASTING_EXCEPT_ABILITIES   = 0x00800000,       // disables casting any ability except for the ones specified in the aura effect's SpellClassMask
    PLAYER_FLAGS_DISABLE_ATTACKING_EXCEPT_ABILITIES = 0x01000000,       // disables all attacks including ability casts except for the ones specified in the aura effect's SpellClassMask
    PLAYER_FLAGS_NO_XP_GAIN                         = 0x02000000,
    PLAYER_FLAGS_UNK26                              = 0x04000000,
    PLAYER_FLAGS_AUTO_DECLINE_GUILD                 = 0x08000000,       // Automatically declines guild invites
    PLAYER_FLAGS_GUILD_LEVEL_ENABLED                = 0x10000000,       // Lua_GetGuildLevelEnabled() - enables guild leveling related UI
    PLAYER_FLAGS_VOID_UNLOCKED                      = 0x20000000,       // void storage
    PLAYER_FLAGS_UNK30                              = 0x40000000,
    PLAYER_FLAGS_UNK31                              = 0x80000000
};

enum PlayerBytesOffsets
{
    PLAYER_BYTES_OFFSET_SKIN_ID         = 0,
    PLAYER_BYTES_OFFSET_FACE_ID         = 1,
    PLAYER_BYTES_OFFSET_HAIR_STYLE_ID   = 2,
    PLAYER_BYTES_OFFSET_HAIR_COLOR_ID   = 3
};

enum PlayerBytes2Offsets
{
    PLAYER_BYTES_2_OFFSET_FACIAL_STYLE      = 0,
    PLAYER_BYTES_2_OFFSET_PARTY_TYPE        = 1,
    PLAYER_BYTES_2_OFFSET_BANK_BAG_SLOTS    = 2,
    PLAYER_BYTES_2_OFFSET_REST_STATE        = 3
};

enum PlayerBytes3Offsets
{
    PLAYER_BYTES_3_OFFSET_GENDER        = 0,
    PLAYER_BYTES_3_OFFSET_INEBRIATION   = 1,
    PLAYER_BYTES_3_OFFSET_PVP_TITLE     = 2,
    PLAYER_BYTES_3_OFFSET_ARENA_FACTION = 3
};

enum PlayerFieldBytesOffsets
{
    PLAYER_FIELD_BYTES_OFFSET_FLAGS                 = 0,
    PLAYER_FIELD_BYTES_OFFSET_RAF_GRANTABLE_LEVEL   = 1,
    PLAYER_FIELD_BYTES_OFFSET_ACTION_BAR_TOGGLES    = 2,
    PLAYER_FIELD_BYTES_OFFSET_LIFETIME_MAX_PVP_RANK = 3
};

enum PlayerFieldBytes2Offsets
{
    PLAYER_FIELD_BYTES_2_OFFSET_OVERRIDE_SPELLS_ID                  = 0,    // uint16!
    PLAYER_FIELD_BYTES_2_OFFSET_IGNORE_POWER_REGEN_PREDICTION_MASK  = 2,
    PLAYER_FIELD_BYTES_2_OFFSET_AURA_VISION                         = 3
};

static_assert((PLAYER_FIELD_BYTES_2_OFFSET_OVERRIDE_SPELLS_ID & 1) == 0, "PLAYER_FIELD_BYTES_2_OFFSET_OVERRIDE_SPELLS_ID must be aligned to 2 byte boundary");

#define PLAYER_BYTES_2_OVERRIDE_SPELLS_UINT16_OFFSET (PLAYER_FIELD_BYTES_2_OFFSET_OVERRIDE_SPELLS_ID / 2)

#define KNOWN_TITLES_SIZE   4
#define MAX_TITLE_INDEX     (KNOWN_TITLES_SIZE * 64)        // 4 uint64 fields

// used in PLAYER_FIELD_BYTES values
enum PlayerFieldByteFlags
{
    PLAYER_FIELD_BYTE_TRACK_STEALTHED   = 0x00000002,
    PLAYER_FIELD_BYTE_RELEASE_TIMER     = 0x00000008,       // Display time till auto release spirit
    PLAYER_FIELD_BYTE_NO_RELEASE_WINDOW = 0x00000010        // Display no "release spirit" window at all
};

// used in PLAYER_FIELD_BYTES2 values
enum PlayerFieldByte2Flags
{
    PLAYER_FIELD_BYTE2_NONE                 = 0x00,
    PLAYER_FIELD_BYTE2_STEALTH              = 0x20,
    PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW    = 0x40
};

enum MirrorTimerType
{
    FATIGUE_TIMER      = 0,
    BREATH_TIMER       = 1,
    FIRE_TIMER         = 2 // feign death
};
#define MAX_TIMERS      3
#define DISABLED_MIRROR_TIMER   -1

// 2^n values
enum PlayerExtraFlags
{
    // gm abilities
    PLAYER_EXTRA_GM_ON              = 0x0001,
    PLAYER_EXTRA_ACCEPT_WHISPERS    = 0x0004,
    PLAYER_EXTRA_TAXICHEAT          = 0x0008,
    PLAYER_EXTRA_GM_INVISIBLE       = 0x0010,
    PLAYER_EXTRA_GM_CHAT            = 0x0020,               // Show GM badge in chat messages

    // other states
    PLAYER_EXTRA_PVP_DEATH          = 0x0100                // store PvP death status until corpse creating.
};

// 2^n values
enum AtLoginFlags
{
    AT_LOGIN_NONE              = 0x000,
    AT_LOGIN_RENAME            = 0x001,
    AT_LOGIN_RESET_SPELLS      = 0x002,
    AT_LOGIN_RESET_TALENTS     = 0x004,
    AT_LOGIN_CUSTOMIZE         = 0x008,
    AT_LOGIN_RESET_PET_TALENTS = 0x010,
    AT_LOGIN_FIRST             = 0x020,
    AT_LOGIN_CHANGE_FACTION    = 0x040,
    AT_LOGIN_CHANGE_RACE       = 0x080,
    AT_LOGIN_RESURRECT         = 0x100
};

typedef std::map<uint32, QuestStatusData> QuestStatusMap;
typedef std::set<uint32> RewardedQuestSet;

enum QuestSaveType
{
    QUEST_DEFAULT_SAVE_TYPE = 0,
    QUEST_DELETE_SAVE_TYPE,
    QUEST_FORCE_DELETE_SAVE_TYPE
};

//               quest
typedef std::map<uint32, QuestSaveType> QuestStatusSaveMap;


enum QuestSlotOffsets
{
    QUEST_ID_OFFSET     = 0,
    QUEST_STATE_OFFSET  = 1,
    QUEST_COUNTS_OFFSET = 2,
    QUEST_TIME_OFFSET   = 4
};

#define MAX_QUEST_OFFSET 5

enum QuestSlotStateMask
{
    QUEST_STATE_NONE     = 0x0000,
    QUEST_STATE_COMPLETE = 0x0001,
    QUEST_STATE_FAIL     = 0x0002
};

enum SkillUpdateState
{
    SKILL_UNCHANGED     = 0,
    SKILL_CHANGED       = 1,
    SKILL_NEW           = 2,
    SKILL_DELETED       = 3
};

struct SkillStatusData
{
    SkillStatusData(uint8 _pos, SkillUpdateState _uState) : pos(_pos), uState(_uState)
    {
    }
    uint8 pos;
    SkillUpdateState uState;
};

typedef std::unordered_map<uint32, SkillStatusData> SkillStatusMap;

class Quest;
class Spell;
class Item;
class WorldSession;

enum PlayerSlots
{
    // first slot for item stored (in any way in player m_items data)
    PLAYER_SLOT_START           = 0,
    // last+1 slot for item stored (in any way in player m_items data)
    PLAYER_SLOT_END             = 86,
    PLAYER_SLOTS_COUNT          = (PLAYER_SLOT_END - PLAYER_SLOT_START)
};

#define INVENTORY_SLOT_BAG_0    255

enum EquipmentSlots : uint8                                 // 19 slots
{
    EQUIPMENT_SLOT_START        = 0,
    EQUIPMENT_SLOT_HEAD         = 0,
    EQUIPMENT_SLOT_NECK         = 1,
    EQUIPMENT_SLOT_SHOULDERS    = 2,
    EQUIPMENT_SLOT_BODY         = 3,
    EQUIPMENT_SLOT_CHEST        = 4,
    EQUIPMENT_SLOT_WAIST        = 5,
    EQUIPMENT_SLOT_LEGS         = 6,
    EQUIPMENT_SLOT_FEET         = 7,
    EQUIPMENT_SLOT_WRISTS       = 8,
    EQUIPMENT_SLOT_HANDS        = 9,
    EQUIPMENT_SLOT_FINGER1      = 10,
    EQUIPMENT_SLOT_FINGER2      = 11,
    EQUIPMENT_SLOT_TRINKET1     = 12,
    EQUIPMENT_SLOT_TRINKET2     = 13,
    EQUIPMENT_SLOT_BACK         = 14,
    EQUIPMENT_SLOT_MAINHAND     = 15,
    EQUIPMENT_SLOT_OFFHAND      = 16,
    EQUIPMENT_SLOT_RANGED       = 17,
    EQUIPMENT_SLOT_TABARD       = 18,
    EQUIPMENT_SLOT_END          = 19
};

enum InventorySlots : uint8                                 // 4 slots
{
    INVENTORY_SLOT_BAG_START    = 19,
    INVENTORY_SLOT_BAG_END      = 23
};

enum InventoryPackSlots : uint8                             // 16 slots
{
    INVENTORY_SLOT_ITEM_START   = 23,
    INVENTORY_SLOT_ITEM_END     = 39
};

enum BankItemSlots                                          // 28 slots
{
    BANK_SLOT_ITEM_START        = 39,
    BANK_SLOT_ITEM_END          = 67
};

enum BankBagSlots                                           // 7 slots
{
    BANK_SLOT_BAG_START         = 67,
    BANK_SLOT_BAG_END           = 74
};

enum BuyBackSlots                                           // 12 slots
{
    // stored in m_buybackitems
    BUYBACK_SLOT_START          = 74,
    BUYBACK_SLOT_END            = 86
};

struct ItemPosCount
{
    ItemPosCount(uint16 _pos, uint32 _count) : pos(_pos), count(_count) { }
    bool isContainedIn(std::vector<ItemPosCount> const& vec) const;
    uint16 pos;
    uint32 count;
};
typedef std::vector<ItemPosCount> ItemPosCountVec;

#define JUSTICE_POINTS_CONVERSION_MONEY 4750

enum TransferAbortReason
{
    TRANSFER_ABORT_NONE                         = 0x00,
    TRANSFER_ABORT_ERROR                        = 0x01,
    TRANSFER_ABORT_MAX_PLAYERS                  = 0x02,         // Transfer Aborted: instance is full
    TRANSFER_ABORT_NOT_FOUND                    = 0x03,         // Transfer Aborted: instance not found
    TRANSFER_ABORT_TOO_MANY_INSTANCES           = 0x04,         // You have entered too many instances recently.
    TRANSFER_ABORT_ZONE_IN_COMBAT               = 0x06,         // Unable to zone in while an encounter is in progress.
    TRANSFER_ABORT_INSUF_EXPAN_LVL              = 0x07,         // You must have <TBC, WotLK> expansion installed to access this area.
    TRANSFER_ABORT_DIFFICULTY                   = 0x08,         // <Normal, Heroic, Epic> difficulty mode is not available for %s.
    TRANSFER_ABORT_UNIQUE_MESSAGE               = 0x09,         // Until you've escaped TLK's grasp, you cannot leave this place!
    TRANSFER_ABORT_TOO_MANY_REALM_INSTANCES     = 0x0A,         // Additional instances cannot be launched, please try again later.
    TRANSFER_ABORT_NEED_GROUP                   = 0x0B,         // 3.1
    TRANSFER_ABORT_NOT_FOUND1                   = 0x0C,         // 3.1
    TRANSFER_ABORT_NOT_FOUND2                   = 0x0D,         // 3.1
    TRANSFER_ABORT_NOT_FOUND3                   = 0x0E,         // 3.2
    TRANSFER_ABORT_REALM_ONLY                   = 0x0F,         // All players on party must be from the same realm.
    TRANSFER_ABORT_MAP_NOT_ALLOWED              = 0x10,         // Map can't be entered at this time.
    TRANSFER_ABORT_LOCKED_TO_DIFFERENT_INSTANCE = 0x12,         // 4.2.2
    TRANSFER_ABORT_ALREADY_COMPLETED_ENCOUNTER  = 0x13         // 4.2.2
};

enum class TransferAbortedUniqueMessageArgs : uint8
{
    None                                        = 0,
    StillWithinTheLichKingsGrasp                = 1,
    DestinyOfTheBilgewaterCartelStillUndecided  = 2,
    FateOfGilneasHasNotBeenDecidedYet           = 3
};

enum InstanceResetWarningType
{
    RAID_INSTANCE_WARNING_HOURS     = 1,                    // WARNING! %s is scheduled to reset in %d hour(s).
    RAID_INSTANCE_WARNING_MIN       = 2,                    // WARNING! %s is scheduled to reset in %d minute(s)!
    RAID_INSTANCE_WARNING_MIN_SOON  = 3,                    // WARNING! %s is scheduled to reset in %d minute(s). Please exit the zone or you will be returned to your bind location!
    RAID_INSTANCE_WELCOME           = 4,                    // Welcome to %s. This raid instance is scheduled to reset in %s.
    RAID_INSTANCE_EXPIRED           = 5
};

// PLAYER_FIELD_ARENA_TEAM_INFO_1_1 offsets
enum ArenaTeamInfoType
{
    ARENA_TEAM_ID                = 0,
    ARENA_TEAM_TYPE              = 1,                       // new in 3.2 - team type?
    ARENA_TEAM_MEMBER            = 2,                       // 0 - captain, 1 - member
    ARENA_TEAM_GAMES_WEEK        = 3,
    ARENA_TEAM_GAMES_SEASON      = 4,
    ARENA_TEAM_WINS_SEASON       = 5,
    ARENA_TEAM_PERSONAL_RATING   = 6,
    ARENA_TEAM_END               = 7
};

class InstanceSave;

enum RestFlag
{
    REST_FLAG_IN_TAVERN         = 0x1,
    REST_FLAG_IN_CITY           = 0x2,
    REST_FLAG_IN_FACTION_AREA   = 0x4
};

enum TeleportToOptions
{
    TELE_TO_NONE                = 0x00,
    TELE_TO_GM_MODE             = 0x01,
    TELE_TO_NOT_LEAVE_TRANSPORT = 0x02,
    TELE_TO_NOT_LEAVE_COMBAT    = 0x04,
    TELE_TO_NOT_UNSUMMON_PET    = 0x08,
    TELE_TO_SPELL               = 0x10
};

DEFINE_ENUM_FLAG(TeleportToOptions);

/// Type of environmental damages
enum EnviromentalDamage
{
    DAMAGE_EXHAUSTED = 0,
    DAMAGE_DROWNING  = 1,
    DAMAGE_FALL      = 2,
    DAMAGE_LAVA      = 3,
    DAMAGE_SLIME     = 4,
    DAMAGE_FIRE      = 5,
    DAMAGE_FALL_TO_VOID = 6                                 // custom case for fall without durability loss
};

enum PlayerChatTag
{
    CHAT_TAG_NONE       = 0x00,
    CHAT_TAG_AFK        = 0x01,
    CHAT_TAG_DND        = 0x02,
    CHAT_TAG_GM         = 0x04,
    CHAT_TAG_COM        = 0x08, // Commentator
    CHAT_TAG_DEV        = 0x10,
    CHAT_TAG_BOSS_SOUND = 0x20, // Plays "RaidBossEmoteWarning" sound on raid boss emote/whisper
    CHAT_TAG_MOBILE     = 0x40
};

enum PlayedTimeIndex
{
    PLAYED_TIME_TOTAL = 0,
    PLAYED_TIME_LEVEL = 1
};

#define MAX_PLAYED_TIME_INDEX 2

// used at player loading query list preparing, and later result selection
enum PlayerLoginQueryIndex
{
    PLAYER_LOGIN_QUERY_LOAD_FROM                    = 0,
    PLAYER_LOGIN_QUERY_LOAD_GROUP                   = 1,
    PLAYER_LOGIN_QUERY_LOAD_BOUND_INSTANCES         = 2,
    PLAYER_LOGIN_QUERY_LOAD_AURAS                   = 3,
    PLAYER_LOGIN_QUERY_LOAD_SPELLS                  = 4,
    PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS            = 5,
    PLAYER_LOGIN_QUERY_LOAD_DAILY_QUEST_STATUS      = 6,
    PLAYER_LOGIN_QUERY_LOAD_REPUTATION              = 7,
    PLAYER_LOGIN_QUERY_LOAD_INVENTORY               = 8,
    PLAYER_LOGIN_QUERY_LOAD_ACTIONS                 = 9,
    PLAYER_LOGIN_QUERY_LOAD_MAILS                   = 10,
    PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS              = 11,
    PLAYER_LOGIN_QUERY_LOAD_SOCIAL_LIST             = 12,
    PLAYER_LOGIN_QUERY_LOAD_HOME_BIND               = 13,
    PLAYER_LOGIN_QUERY_LOAD_SPELL_COOLDOWNS         = 14,
    PLAYER_LOGIN_QUERY_LOAD_DECLINED_NAMES          = 15,
    PLAYER_LOGIN_QUERY_LOAD_GUILD                   = 16,
    PLAYER_LOGIN_QUERY_LOAD_ARENA_INFO              = 17,
    PLAYER_LOGIN_QUERY_LOAD_ACHIEVEMENTS            = 18,
    PLAYER_LOGIN_QUERY_LOAD_CRITERIA_PROGRESS       = 19,
    PLAYER_LOGIN_QUERY_LOAD_EQUIPMENT_SETS          = 20,
    PLAYER_LOGIN_QUERY_LOAD_BG_DATA                 = 21,
    PLAYER_LOGIN_QUERY_LOAD_GLYPHS                  = 22,
    PLAYER_LOGIN_QUERY_LOAD_TALENTS                 = 23,
    PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA            = 24,
    PLAYER_LOGIN_QUERY_LOAD_SKILLS                  = 25,
    PLAYER_LOGIN_QUERY_LOAD_WEEKLY_QUEST_STATUS     = 26,
    PLAYER_LOGIN_QUERY_LOAD_RANDOM_BG               = 27,
    PLAYER_LOGIN_QUERY_LOAD_BANNED                  = 28,
    PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_REW        = 29,
    PLAYER_LOGIN_QUERY_LOAD_INSTANCE_LOCK_TIMES     = 30,
    PLAYER_LOGIN_QUERY_LOAD_SEASONAL_QUEST_STATUS   = 31,
    PLAYER_LOGIN_QUERY_LOAD_MONTHLY_QUEST_STATUS    = 32,
    PLAYER_LOGIN_QUERY_LOAD_VOID_STORAGE            = 33,
    PLAYER_LOGIN_QUERY_LOAD_CURRENCY                = 34,
    PLAYER_LOGIN_QUERY_LOAD_CUF_PROFILES            = 35,
    PLAYER_LOGIN_QUERY_LOAD_CORPSE_LOCATION         = 36,
    PLAYER_LOGIN_QUERY_LOAD_ALL_PETS                = 37,
    PLAYER_LOGIN_QUERY_LOAD_LFG_REWARD_STATUS       = 38,
    MAX_PLAYER_LOGIN_QUERY
};

enum PlayerDelayedOperations
{
    DELAYED_SAVE_PLAYER         = 0x01,
    DELAYED_RESURRECT_PLAYER    = 0x02,
    DELAYED_SPELL_CAST_DESERTER = 0x04,
    DELAYED_BG_MOUNT_RESTORE    = 0x08,                     ///< Flag to restore mount state after teleport from BG
    DELAYED_BG_TAXI_RESTORE     = 0x10,                     ///< Flag to restore taxi state after teleport from BG
    DELAYED_BG_GROUP_RESTORE    = 0x20,                     ///< Flag to restore group state after teleport from BG
    DELAYED_END
};

// Player summoning auto-decline time (in secs)
#define MAX_PLAYER_SUMMON_DELAY                   (2*MINUTE)
// Maximum money amount : 2^31 - 1
TC_GAME_API extern uint64 const MAX_MONEY_AMOUNT;

enum BindExtensionState
{
    EXTEND_STATE_EXPIRED  =   0,
    EXTEND_STATE_NORMAL   =   1,
    EXTEND_STATE_EXTENDED =   2,
    EXTEND_STATE_KEEP     = 255   // special state: keep current save type
};
struct InstancePlayerBind
{
    InstanceSave* save;
    /* permanent PlayerInstanceBinds are created in Raid/Heroic instances for players
    that aren't already permanently bound when they are inside when a boss is killed
    or when they enter an instance that the group leader is permanently bound to. */
    bool perm;
    /* extend state listing:
    EXPIRED  - doesn't affect anything unless manually re-extended by player
    NORMAL   - standard state
    EXTENDED - won't be promoted to EXPIRED at next reset period, will instead be promoted to NORMAL */
    BindExtensionState extendState;

    InstancePlayerBind() : save(nullptr), perm(false), extendState(EXTEND_STATE_NORMAL) { }
};

enum CharDeleteMethod
{
    CHAR_DELETE_REMOVE = 0,                      // Completely remove from the database
    CHAR_DELETE_UNLINK = 1                       // The character gets unlinked from the account,
                                                 // the name gets freed up and appears as deleted ingame
};

enum ReferAFriendError
{
    ERR_REFER_A_FRIEND_NONE                          = 0x00,
    ERR_REFER_A_FRIEND_NOT_REFERRED_BY               = 0x01,
    ERR_REFER_A_FRIEND_TARGET_TOO_HIGH               = 0x02,
    ERR_REFER_A_FRIEND_INSUFFICIENT_GRANTABLE_LEVELS = 0x03,
    ERR_REFER_A_FRIEND_TOO_FAR                       = 0x04,
    ERR_REFER_A_FRIEND_DIFFERENT_FACTION             = 0x05,
    ERR_REFER_A_FRIEND_NOT_NOW                       = 0x06,
    ERR_REFER_A_FRIEND_GRANT_LEVEL_MAX_I             = 0x07,
    ERR_REFER_A_FRIEND_NO_TARGET                     = 0x08,
    ERR_REFER_A_FRIEND_NOT_IN_GROUP                  = 0x09,
    ERR_REFER_A_FRIEND_SUMMON_LEVEL_MAX_I            = 0x0A,
    ERR_REFER_A_FRIEND_SUMMON_COOLDOWN               = 0x0B,
    ERR_REFER_A_FRIEND_INSUF_EXPAN_LVL               = 0x0C,
    ERR_REFER_A_FRIEND_SUMMON_OFFLINE_S              = 0x0D
};

enum PlayerRestState : uint8
{
    REST_STATE_RESTED                                = 0x01,
    REST_STATE_NOT_RAF_LINKED                        = 0x02,
    REST_STATE_RAF_LINKED                            = 0x06
};

enum PlayerCommandStates
{
    CHEAT_NONE      = 0x00,
    CHEAT_GOD       = 0x01,
    CHEAT_CASTTIME  = 0x02,
    CHEAT_COOLDOWN  = 0x04,
    CHEAT_POWER     = 0x08,
    CHEAT_WATERWALK = 0x10
};

enum PlayerLogXPReason : uint8
{
    LOG_XP_REASON_KILL    = 0,
    LOG_XP_REASON_NO_KILL = 1
};

struct PlayerPetData
{
    uint32 PetId;
    uint32 CreatureId;
    uint64 Owner;
    uint32 DisplayId;
    uint32 Petlevel;
    uint32 PetExp;
    ReactStates Reactstate;
    uint8 Slot;
    std::string Name;
    bool Renamed;
    bool Active;
    uint32 SavedHealth;
    uint32 SavedMana;
    std::string Actionbar;
    uint32 Timediff;
    uint32 SummonSpellId;
    PetType Type;
};

class Player;

/// Holder for Battleground data
struct BGData
{
    BGData() : bgInstanceID(0), bgTypeID(BATTLEGROUND_TYPE_NONE), bgAfkReportedCount(0), bgAfkReportedTimer(0),
        bgTeam(0), mountSpell(0) { bgQueuesJoinedTime.clear(); ClearTaxiPath(); }

    uint32 bgInstanceID;                    ///< This variable is set to bg->m_InstanceID,
                                            ///  when player is teleported to BG - (it is battleground's GUID)
    BattlegroundTypeId bgTypeID;

    std::map<uint32, uint32> bgQueuesJoinedTime;

    std::set<uint32>   bgAfkReporter;
    uint8              bgAfkReportedCount;
    time_t             bgAfkReportedTimer;

    uint32 bgTeam;                          ///< What side the player will be added to

    uint32 mountSpell;
    uint32 taxiPath[2];

    WorldLocation joinPos;                  ///< From where player entered BG
    WorldLocation leavePos;

    void ClearTaxiPath()     { taxiPath[0] = taxiPath[1] = 0; }
    bool HasTaxiPath() const { return taxiPath[0] && taxiPath[1]; }
};

struct VoidStorageItem
{
    VoidStorageItem() : ItemId(0), ItemEntry(0), ItemRandomPropertyId(), ItemSuffixFactor(0) { }
    VoidStorageItem(uint64 id, uint32 entry, ObjectGuid const& creator, ItemRandomEnchantmentId randomPropertyId, uint32 suffixFactor)
        : ItemId(id), ItemEntry(entry), CreatorGuid(creator), ItemRandomPropertyId(randomPropertyId),
        ItemSuffixFactor(suffixFactor) { }

    uint64 ItemId;
    uint32 ItemEntry;
    ObjectGuid CreatorGuid;
    ItemRandomEnchantmentId ItemRandomPropertyId;
    uint32 ItemSuffixFactor;
};

struct ResurrectionData
{
    ObjectGuid GUID;
    WorldLocation Location;
    uint32 Health;
    uint32 Mana;
    uint32 Aura;
};

struct PlayerTalentInfo
{
    PlayerTalentInfo() :
        FreeTalentPoints(0), UsedTalentCount(0), QuestRewardedTalentCount(0),
        ResetTalentsCost(0), ResetTalentsTime(0),
        ActiveSpec(0), SpecsCount(1)
    {
        for (uint8 i = 0; i < MAX_TALENT_SPECS; ++i)
        {
            SpecInfo[i].Talents = new PlayerTalentMap();
            memset(SpecInfo[i].Glyphs, 0, MAX_GLYPH_SLOT_INDEX * sizeof(uint32));
            SpecInfo[i].TalentTree = 0;
        }
    }

    ~PlayerTalentInfo()
    {
        for (uint8 i = 0; i < MAX_TALENT_SPECS; ++i)
        {
            for (PlayerTalentMap::const_iterator itr = SpecInfo[i].Talents->begin(); itr != SpecInfo[i].Talents->end(); ++itr)
                delete itr->second;
            delete SpecInfo[i].Talents;
        }
    }

    struct TalentSpecInfo
    {
        TalentSpecInfo() : Talents(nullptr), TalentTree(0)
        {
            memset(Glyphs, 0, MAX_GLYPH_SLOT_INDEX * sizeof(uint32));
        }

        PlayerTalentMap* Talents;
        uint32 Glyphs[MAX_GLYPH_SLOT_INDEX];
        uint32 TalentTree;
    } SpecInfo[MAX_TALENT_SPECS];

    uint32 FreeTalentPoints;
    uint32 UsedTalentCount;
    uint32 QuestRewardedTalentCount;
    uint32 ResetTalentsCost;
    time_t ResetTalentsTime;
    uint8 ActiveSpec;
    uint8 SpecsCount;

private:
    PlayerTalentInfo(PlayerTalentInfo const&);
};

class TC_GAME_API Player : public Unit, public GridObject<Player>
{
    friend class WorldSession;
    friend class CinematicMgr;
    friend void AddItemToUpdateQueueOf(Item* item, Player* player);
    friend void RemoveItemFromUpdateQueueOf(Item* item, Player* player);
    public:
        explicit Player(WorldSession* session);
        ~Player();

        PlayerAI* AI() const { return reinterpret_cast<PlayerAI*>(GetAI()); }

        void CleanupsBeforeDelete(bool finalCleanup = true) override;

        void AddToWorld() override;
        void RemoveFromWorld() override;

        void SetObjectScale(float scale) override;

        bool TeleportTo(uint32 mapid, float x, float y, float z, float orientation, TeleportToOptions options = TELE_TO_NONE, Optional<uint32> instanceId = {});
        bool TeleportTo(WorldLocation const& loc, TeleportToOptions options = TELE_TO_NONE, Optional<uint32> instanceId = {});
        bool TeleportToBGEntryPoint();

        bool HasSummonPending() const;
        void SendSummonRequestFrom(Unit* summoner);
        void SummonIfPossible(bool agree);

        bool Create(ObjectGuid::LowType guidlow, CharacterCreateInfo* createInfo);

        void Update(uint32 time) override;

        void Heartbeat() override;

        bool IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index, WorldObject const* caster, bool requireImmunityPurgesEffectAttribute = false) const override;

        bool IsInAreaTriggerRadius(AreaTriggerEntry const* trigger) const;

        void SendInitialPacketsBeforeAddToMap(bool firstLogin = false);
        void SendInitialPacketsAfterAddToMap();
        void SendSupercededSpell(uint32 oldSpell, uint32 newSpell) const;
        void SendTransferAborted(uint32 mapid, TransferAbortReason reason, uint8 arg = 0) const;
        void SendInstanceResetWarning(uint32 mapid, Difficulty difficulty, uint32 time, bool welcome) const;

        bool CanInteractWithQuestGiver(Object* questGiver) const;
        Creature* GetNPCIfCanInteractWith(ObjectGuid const& guid, uint32 npcflagmask) const;
        GameObject* GetGameObjectIfCanInteractWith(ObjectGuid const& guid) const;
        GameObject* GetGameObjectIfCanInteractWith(ObjectGuid const& guid, GameobjectTypes type) const;

        void ToggleAFK();
        void ToggleDND();
        bool isAFK() const { return HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK); }
        bool isDND() const { return HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_DND); }
        uint8 GetChatTag() const;
        std::string autoReplyMsg;

        uint32 GetBarberShopCost(uint8 newhairstyle, uint8 newhaircolor, uint8 newfacialhair, BarberShopStyleEntry const* newSkin = nullptr) const;

        PlayerSocial* GetSocial() { return m_social; }
        void RemoveSocial();

        void SendTamePetFailure(PetTameFailureReason reason);

        PlayerTaxi m_taxi;
        void InitTaxiNodesForLevel() { m_taxi.InitTaxiNodesForLevel(getRace(), getClass(), getLevel()); }
        bool ActivateTaxiPathTo(std::vector<uint32> const& nodes, Creature* npc = nullptr, uint32 spellid = 0);
        bool ActivateTaxiPathTo(uint32 taxi_path_id, uint32 spellid = 0);
        void CleanupAfterTaxiFlight();
        void ContinueTaxiFlight() const;
        void SendTaxiNodeStatusMultiple();
                                                            // mount_id can be used in scripting calls
        bool isAcceptWhispers() const { return (m_ExtraFlags & PLAYER_EXTRA_ACCEPT_WHISPERS) != 0; }
        void SetAcceptWhispers(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_ACCEPT_WHISPERS; else m_ExtraFlags &= ~PLAYER_EXTRA_ACCEPT_WHISPERS; }
        bool IsGameMaster() const { return (m_ExtraFlags & PLAYER_EXTRA_GM_ON) != 0; }
        bool CanBeGameMaster() const;
        void SetGameMaster(bool on);
        bool isGMChat() const { return (m_ExtraFlags & PLAYER_EXTRA_GM_CHAT) != 0; }
        void SetGMChat(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_GM_CHAT; else m_ExtraFlags &= ~PLAYER_EXTRA_GM_CHAT; }
        bool isTaxiCheater() const { return (m_ExtraFlags & PLAYER_EXTRA_TAXICHEAT) != 0; }
        void SetTaxiCheater(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_TAXICHEAT; else m_ExtraFlags &= ~PLAYER_EXTRA_TAXICHEAT; }
        bool isGMVisible() const { return !(m_ExtraFlags & PLAYER_EXTRA_GM_INVISIBLE); }
        void SetGMVisible(bool on);
        void SetPvPDeath(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_PVP_DEATH; else m_ExtraFlags &= ~PLAYER_EXTRA_PVP_DEATH; }

        void GiveXP(uint32 xp, Unit* victim, float group_rate=1.0f);
        void GiveLevel(uint8 level);
        bool IsMaxLevel() const;

        void InitStatsForLevel(bool reapplyMods = false);

        // .cheat command related
        bool GetCommandStatus(uint32 command) const { return (_activeCheats & command) != 0; }
        void SetCommandStatusOn(uint32 command) { _activeCheats |= command; }
        void SetCommandStatusOff(uint32 command) { _activeCheats &= ~command; }

        // Played Time Stuff
        time_t m_logintime;
        time_t m_Last_tick;
        std::array<uint32, MAX_PLAYED_TIME_INDEX> m_Played_time;
        uint32 GetTotalPlayedTime() const { return m_Played_time[PLAYED_TIME_TOTAL]; }
        uint32 GetLevelPlayedTime() const { return m_Played_time[PLAYED_TIME_LEVEL]; }

        void setDeathState(DeathState s) override;                   // overwrite Unit::setDeathState

        float GetRestBonus() const { return m_rest_bonus; }
        void SetRestBonus(float rest_bonus_new);

        bool HasRestFlag(RestFlag restFlag) const { return (_restFlagMask & restFlag) != 0; }
        void SetRestFlag(RestFlag restFlag);
        void RemoveRestFlag(RestFlag restFlag);

        uint32 GetXPRestBonus(uint32 xp);
        uint32 GetInnTriggerId() const { return inn_triggerId; }
        void SetInnTriggerID(uint32 id) { inn_triggerId = id; }

        Pet* GetPet() const;
        Pet* SummonPet(uint32 entry, float x, float y, float z, float ang, PetType petType, uint32 despwtime);
        void RemovePet(Pet* pet, PetSaveMode mode, bool returnreagent = false);

        // pet auras
        std::unordered_set<PetAura const*> m_petAuras;
        void AddPetAura(PetAura const* petSpell);
        void RemovePetAura(PetAura const* petSpell);

        /// Handles said message in regular chat based on declared language and in config pre-defined Range.
        void Say(std::string const& text, Language language, WorldObject const* = nullptr) override;
        void Say(uint32 textId, WorldObject const* target = nullptr) override;
        /// Handles yelled message in regular chat based on declared language and in config pre-defined Range.
        void Yell(std::string const& text, Language language, WorldObject const* = nullptr) override;
        void Yell(uint32 textId, WorldObject const* target = nullptr) override;
        /// Outputs an universal text which is supposed to be an action.
        void TextEmote(std::string const& text, WorldObject const* = nullptr, bool = false) override;
        void TextEmote(uint32 textId, WorldObject const* target = nullptr, bool isBossEmote = false) override;
        /// Handles whispers from Addons and players based on sender, receiver's guid and language.
        void Whisper(std::string const& text, Language language, Player* receiver, bool = false) override;
        void Whisper(uint32 textId, Player* target, bool isBossWhisper = false) override;
        void WhisperAddon(std::string const& text, std::string const& prefix, Player* receiver);

        /*********************************************************/
        /***                    STORAGE SYSTEM                 ***/
        /*********************************************************/

        void SetVirtualItemSlot(uint8 i, Item* item);
        void SetSheath(SheathState sheathed) override;             // overwrite Unit version
        uint8 FindEquipSlot(ItemTemplate const* proto, uint32 slot, bool swap) const;
        uint32 GetItemCount(uint32 item, bool inBankAlso = false, Item* skipItem = nullptr) const;
        uint32 GetItemCountWithLimitCategory(uint32 limitCategory, Item* skipItem = nullptr) const;
        Item* GetItemByGuid(ObjectGuid guid) const;
        Item* GetItemByEntry(uint32 entry) const;
        Item* GetItemByPos(uint16 pos) const;
        Item* GetItemByPos(uint8 bag, uint8 slot) const;
        Item* GetUseableItemByPos(uint8 bag, uint8 slot) const;
        Bag*  GetBagByPos(uint8 slot) const;
        uint32 GetFreeInventorySpace() const;
        Item* GetWeaponForAttack(WeaponAttackType attackType, bool useable = false) const;
        Item* GetShield(bool useable = false) const;
        static WeaponAttackType GetAttackBySlot(uint8 slot);        // MAX_ATTACK if not weapon slot
        std::vector<Item*>& GetItemUpdateQueue() { return m_itemUpdateQueue; }
        static bool IsInventoryPos(uint16 pos) { return IsInventoryPos(pos >> 8, pos & 255); }
        static bool IsInventoryPos(uint8 bag, uint8 slot);
        static bool IsEquipmentPos(uint16 pos) { return IsEquipmentPos(pos >> 8, pos & 255); }
        static bool IsEquipmentPos(uint8 bag, uint8 slot);
        static bool IsBagPos(uint16 pos);
        static bool IsBankPos(uint16 pos) { return IsBankPos(pos >> 8, pos & 255); }
        static bool IsBankPos(uint8 bag, uint8 slot);
        bool IsValidPos(uint16 pos, bool explicit_pos) const { return IsValidPos(pos >> 8, pos & 255, explicit_pos); }
        bool IsValidPos(uint8 bag, uint8 slot, bool explicit_pos) const;
        uint8 GetBankBagSlotCount() const { return GetByteValue(PLAYER_BYTES_2, PLAYER_BYTES_2_OFFSET_BANK_BAG_SLOTS); }
        void SetBankBagSlotCount(uint8 count) { SetByteValue(PLAYER_BYTES_2, PLAYER_BYTES_2_OFFSET_BANK_BAG_SLOTS, count); }
        bool HasItemCount(uint32 item, uint32 count = 1, bool inBankAlso = false) const;
        bool HasItemFitToSpellRequirements(SpellInfo const* spellInfo, Item const* ignoreItem = nullptr) const;
        bool HasAllItemsToFitToSpellRequirements(SpellInfo const* spellInfo);
        bool CanNoReagentCast(SpellInfo const* spellInfo) const;
        bool HasItemOrGemWithIdEquipped(uint32 item, uint32 count, uint8 except_slot = NULL_SLOT) const;
        bool HasItemWithLimitCategoryEquipped(uint32 limitCategory, uint32 count, uint8 except_slot = NULL_SLOT) const;
        bool HasGemWithLimitCategoryEquipped(uint32 limitCategory, uint32 count, uint8 except_slot = NULL_SLOT) const;
        InventoryResult CanTakeMoreSimilarItems(Item* pItem, uint32* offendingItemId = nullptr) const;
        InventoryResult CanTakeMoreSimilarItems(uint32 entry, uint32 count, uint32* offendingItemId = nullptr) const { return CanTakeMoreSimilarItems(entry, count, nullptr, nullptr, offendingItemId); }
        InventoryResult CanStoreNewItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, uint32 item, uint32 count, uint32* no_space_count = nullptr) const;
        InventoryResult CanStoreItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, Item* pItem, bool swap = false) const;
        InventoryResult CanStoreItems(Item** items, int count, uint32* offendingItemId) const;
        InventoryResult CanEquipNewItem(uint8 slot, uint16& dest, uint32 item, bool swap) const;
        InventoryResult CanEquipItem(uint8 slot, uint16& dest, Item* pItem, bool swap, bool not_loading = true) const;

        InventoryResult CanEquipUniqueItem(Item* pItem, uint8 except_slot = NULL_SLOT, uint32 limit_count = 1) const;
        InventoryResult CanEquipUniqueItem(ItemTemplate const* itemProto, uint8 except_slot = NULL_SLOT, uint32 limit_count = 1) const;
        InventoryResult CanUnequipItems(uint32 item, uint32 count) const;
        InventoryResult CanUnequipItem(uint16 src, bool swap) const;
        InventoryResult CanBankItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, Item* pItem, bool swap, bool not_loading = true) const;
        InventoryResult CanUseItem(Item* pItem, bool not_loading = true) const;
        bool HasItemTotemCategory(uint32 TotemCategory) const;
        InventoryResult CanUseItem(ItemTemplate const* pItem) const;
        bool CanRollNeedForItem(ItemTemplate const* itemTemplate) const;
        Item* StoreNewItem(ItemPosCountVec const& pos, uint32 item, bool update, ItemRandomEnchantmentId const& randomPropertyId = {}, GuidSet const& allowedLooters = GuidSet());
        Item* StoreItem(ItemPosCountVec const& pos, Item* pItem, bool update);
        Item* EquipNewItem(uint16 pos, uint32 item, bool update);
        Item* EquipItem(uint16 pos, Item* pItem, bool update);
        void AutoUnequipOffhandIfNeed(bool force = false);
        bool StoreNewItemInBestSlots(uint32 item_id, uint32 item_count);
        void AutoStoreLoot(uint8 bag, uint8 slot, uint32 loot_id, LootStore const& store, bool broadcast = false);
        void AutoStoreLoot(uint32 loot_id, LootStore const& store, bool broadcast = false) { AutoStoreLoot(NULL_BAG, NULL_SLOT, loot_id, store, broadcast); }
        void StoreLootItem(ObjectGuid lootWorldObjectGuid, uint8 lootSlot, Loot* loot, GameObject* go = nullptr);

        InventoryResult CanTakeMoreSimilarItems(uint32 entry, uint32 count, Item* pItem, uint32* no_space_count = nullptr, uint32* offendingItemId = nullptr) const;
        InventoryResult CanStoreItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, uint32 entry, uint32 count, Item* pItem = nullptr, bool swap = false, uint32* no_space_count = nullptr) const;

        void AddRefundReference(ObjectGuid it);
        void DeleteRefundReference(ObjectGuid it);

        /// send initialization of new currency for client
        void SendNewCurrency(uint32 id) const;
        /// send full data about all currencies to client
        void SendCurrencies() const;
        /// send conquest currency points and their cap week/arena
        void SendPvpRewards() const;
        /// return count of currency witch has plr
        uint32 GetCurrency(uint32 id, bool usePrecision) const;
        /// return count of currency gaind on current week
        uint32 GetCurrencyOnWeek(uint32 id, bool usePrecision) const;
        /// return count of earned currency on current season
        uint32 GetCurrencyOnSeason(uint32 id) const;
        /// return week cap by currency id
        uint32 GetCurrencyWeekCap(uint32 id, bool usePrecision) const;
        /// return presence related currency
        bool HasCurrency(uint32 id, uint32 count) const;
        /// initialize currency count for custom initialization at create character
        void SetCurrency(uint32 currencyId, uint32 amount);
        void ResetCurrencyWeekCap();
        void ModifyCurrency(uint32 currencyId, int32 amount, bool supressChatLog = false, bool ignoreModifiersAndTracking = false);

        void ApplyEquipCooldown(Item* pItem);
        void QuickEquipItem(uint16 pos, Item* pItem);
        void VisualizeItem(uint8 slot, Item* pItem);
        void SetVisibleItemSlot(uint8 slot, Item* pItem);
        Item* BankItem(ItemPosCountVec const& dest, Item* pItem, bool update);
        void RemoveItem(uint8 bag, uint8 slot, bool update);
        void MoveItemFromInventory(uint8 bag, uint8 slot, bool update);
                                                            // in trade, auction, guild bank, mail....
        void MoveItemToInventory(ItemPosCountVec const& dest, Item* pItem, bool update, bool in_characterInventoryDB = false);
                                                            // in trade, guild bank, mail....
        void RemoveItemDependentAurasAndCasts(Item* pItem);
        void DestroyItem(uint8 bag, uint8 slot, bool update);
        void DestroyItemCount(uint32 item, uint32 count, bool update, bool unequip_check = false);
        void DestroyItemCount(Item* item, uint32& count, bool update);
        void DestroyConjuredItems(bool update);
        void DestroyZoneLimitedItem(bool update, uint32 new_zone);
        void SplitItem(uint16 src, uint16 dst, uint32 count);
        void SwapItem(uint16 src, uint16 dst);
        void AddItemToBuyBackSlot(Item* pItem);
        Item* GetItemFromBuyBackSlot(uint32 slot);
        void RemoveItemFromBuyBackSlot(uint32 slot, bool del);
        void SendEquipError(InventoryResult msg, Item* pItem, Item* pItem2 = nullptr, uint32 itemid = 0) const;
        void SendBuyError(BuyResult msg, Creature* creature, uint32 item, uint32 param) const;
        void SendSellError(SellResult msg, Creature* creature, ObjectGuid guid) const;
        void AddWeaponProficiency(uint32 newflag) { m_WeaponProficiency |= newflag; }
        void AddArmorProficiency(uint32 newflag) { m_ArmorProficiency |= newflag; }
        uint32 GetWeaponProficiency() const { return m_WeaponProficiency; }
        uint32 GetArmorProficiency() const { return m_ArmorProficiency; }
        bool IsUseEquipedWeapon(bool mainhand) const;
        bool IsTwoHandUsed() const;
        bool IsUsingTwoHandedWeaponInOneHand() const;
        void SendNewItem(Item* item, uint32 count, bool received, bool created, bool broadcast = false, bool sendChatMessage = true);
        bool BuyItemFromVendorSlot(ObjectGuid vendorguid, uint32 vendorslot, uint32 item, uint32 count, uint8 bag, uint8 slot);
        bool BuyCurrencyFromVendorSlot(ObjectGuid vendorGuid, uint32 vendorSlot, uint32 currency, uint32 count);
        bool _StoreOrEquipNewItem(uint32 vendorslot, uint32 item, uint32 count, uint8 bag, uint8 slot, int64 price, ItemTemplate const* pProto, Creature* pVendor, VendorItem const* crItem, bool bStore);

        float GetReputationPriceDiscount(Creature const* creature) const;

        Player* GetTrader() const;
        TradeData* GetTradeData() const { return m_trade; }
        void TradeCancel(bool sendback);

        CinematicMgr* GetCinematicMgr() const { return _cinematicMgr.get(); }

        void UpdateEnchantTime(uint32 time);
        void UpdateSoulboundTradeItems();
        void AddTradeableItem(Item* item);
        void RemoveTradeableItem(Item* item);
        void UpdateItemDuration(uint32 time, bool realtimeonly = false);
        void AddEnchantmentDurations(Item* item);
        void RemoveEnchantmentDurations(Item* item);
        void RemoveEnchantmentDurationsReferences(Item* item);
        void RemoveArenaEnchantments(EnchantmentSlot slot);
        void AddEnchantmentDuration(Item* item, EnchantmentSlot slot, uint32 duration);
        void ApplyEnchantment(Item* item, EnchantmentSlot slot, bool apply, bool apply_dur = true, bool ignore_condition = false);
        void ApplyEnchantment(Item* item, bool apply);
        void ApplyReforgeEnchantment(Item* item, bool apply);
        void UpdateSkillEnchantments(uint16 skill_id, uint16 curr_value, uint16 new_value);
        void SendEnchantmentDurations();
        void BuildEnchantmentsInfoData(WorldPacket* data);
        void AddItemDurations(Item* item);
        void RemoveItemDurations(Item* item);
        void SendItemDurations();
        void LoadCorpse(PreparedQueryResult result);
        void LoadPet();
        void LoadPetsFromDB(PreparedQueryResult result);

        bool AddItem(uint32 itemId, uint32 count);

        uint32 m_stableSlots;

        /*********************************************************/
        /***                    GOSSIP SYSTEM                  ***/
        /*********************************************************/

        void PrepareGossipMenu(WorldObject* source, uint32 menuId = 0, bool showQuests = false);
        void SendPreparedGossip(WorldObject* source);
        void OnGossipSelect(WorldObject* source, uint32 gossipListId, uint32 menuId);

        uint32 GetGossipTextId(uint32 menuId, WorldObject* source);
        uint32 GetGossipTextId(WorldObject* source);
        static uint32 GetDefaultGossipMenuForSource(WorldObject* source);

        /*********************************************************/
        /***                    QUEST SYSTEM                   ***/
        /*********************************************************/

        int32 GetQuestLevel(Quest const* quest) const { return quest && (quest->GetQuestLevel() > 0) ? quest->GetQuestLevel() : getLevel(); }

        void PrepareQuestMenu(ObjectGuid guid);
        void SendPreparedQuest(ObjectGuid guid);
        bool IsActiveQuest(uint32 quest_id) const;
        Quest const* GetNextQuest(Object const* questGiver, Quest const* quest) const;
        bool CanSeeStartQuest(Quest const* quest) const;
        bool CanTakeQuest(Quest const* quest, bool msg) const;
        bool CanAddQuest(Quest const* quest, bool msg) const;
        bool CanCompleteQuest(uint32 quest_id);
        bool CanCompleteRepeatableQuest(Quest const* quest);
        bool CanRewardQuest(Quest const* quest, bool msg);
        bool CanRewardQuest(Quest const* quest, uint32 reward, bool msg);
        void AddQuestAndCheckCompletion(Quest const* quest, Object* questGiver);
        void AddQuest(Quest const* quest, Object* questGiver);
        void AbandonQuest(uint32 quest_id);
        void CompleteQuest(uint32 quest_id);
        void IncompleteQuest(uint32 quest_id);
        void RewardQuest(Quest const* quest, uint32 reward, Object* questGiver, bool announce = true);
        void SetRewardedQuest(uint32 quest_id);
        void FailQuest(uint32 quest_id);
        void FailQuestsWithFlag(QuestFlags flag);
        bool SatisfyQuestSkill(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestLevel(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestLog(bool msg) const;
        bool SatisfyQuestDependentQuests(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestPreviousQuest(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestDependentPreviousQuests(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestBreadcrumbQuest(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestDependentBreadcrumbQuests(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestClass(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestRace(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestReputation(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestStatus(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestConditions(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestTimed(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestExclusiveGroup(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestDay(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestWeek(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestMonth(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestSeasonal(Quest const* qInfo, bool msg) const;
        bool GiveQuestSourceItem(Quest const* quest);
        bool TakeQuestSourceItem(uint32 questId, bool msg);
        bool GetQuestRewardStatus(uint32 quest_id) const;
        QuestStatus GetQuestStatus(uint32 quest_id) const;
        void SetQuestStatus(uint32 questId, QuestStatus status, bool update = true);
        void RemoveActiveQuest(uint32 questId, bool update = true);
        void RemoveRewardedQuest(uint32 questId, bool update = true);
        void SendQuestUpdate(uint32 questId);
        QuestGiverStatus GetQuestDialogStatus(Object* questGiver);
        bool SatisfyFirstLFGReward(uint32 dungeonId, uint8 maxRewCount) const;
        uint8 GetFirstRewardCountForDungeonId(uint32 dungeon);

        void SetDailyQuestStatus(uint32 quest_id);
        bool IsDailyQuestDone(uint32 quest_id);
        void SetWeeklyQuestStatus(uint32 quest_id);
        void SetMonthlyQuestStatus(uint32 quest_id);
        void SetSeasonalQuestStatus(uint32 quest_id);
        void SetLFGRewardStatus(uint32 dungeon_id, bool daily_reset);
        void ResetDailyQuestStatus();
        void ResetWeeklyQuestStatus();
        void ResetMonthlyQuestStatus();
        void ResetSeasonalQuestStatus(uint16 event_id, time_t eventStartTime);
        void ResetWeeklyLFGRewardStatus();
        void ResetDailyLFGRewardStatus();

        uint16 FindQuestSlot(uint32 quest_id) const;
        uint32 GetQuestSlotQuestId(uint16 slot) const;
        uint32 GetQuestSlotState(uint16 slot) const;
        uint16 GetQuestSlotCounter(uint16 slot, uint8 counter) const;
        uint32 GetQuestSlotTime(uint16 slot) const;
        void SetQuestSlot(uint16 slot, uint32 quest_id, uint32 timer = 0);
        void SetQuestSlotCounter(uint16 slot, uint8 counter, uint16 count);
        void SetQuestSlotState(uint16 slot, uint32 state);
        void RemoveQuestSlotState(uint16 slot, uint32 state);
        void SetQuestSlotTimer(uint16 slot, uint32 timer);
        void SwapQuestSlot(uint16 slot1, uint16 slot2);

        uint16 GetReqKillOrCastCurrentCount(uint32 quest_id, int32 entry) const;
        void AreaExploredOrEventHappens(uint32 questId);
        void GroupEventHappens(uint32 questId, WorldObject const* pEventObject);
        void ItemAddedQuestCheck(uint32 entry, uint32 count);
        void ItemRemovedQuestCheck(uint32 entry, uint32 count);
        void KilledMonster(CreatureTemplate const* cInfo, ObjectGuid guid);
        void KilledMonsterCredit(uint32 entry, ObjectGuid guid = ObjectGuid::Empty);
        void KilledPlayerCredit(uint16 count = 1);
        void KilledPlayerCreditForQuest(uint16 count, Quest const* quest);
        void KillCreditGO(uint32 entry, ObjectGuid guid = ObjectGuid::Empty);
        void TalkedToCreature(uint32 entry, ObjectGuid guid);
        void MoneyChanged(uint64 value);
        void ReputationChanged(FactionEntry const* factionEntry);
        void ReputationChanged2(FactionEntry const* factionEntry);
        bool HasQuestForItem(uint32 itemId, uint32 excludeQuestId = 0, bool turnIn = false) const;
        bool HasQuestForGO(int32 goId) const;
        void UpdateVisibleGameobjectsOrSpellClicks();
        bool CanShareQuest(uint32 questId) const;

        void SendQuestComplete(Quest const* quest) const;
        void SendQuestReward(Quest const* quest, Creature const* questGiver, uint32 xp);
        void SendQuestFailed(uint32 questId, InventoryResult reason = EQUIP_ERR_OK) const;
        void SendQuestTimerFailed(uint32 questId) const;
        void SendCanTakeQuestResponse(QuestFailedReason reason) const;
        void SendQuestConfirmAccept(Quest const* quest, Player* receiver) const;
        void SendPushToPartyResponse(Player const* player, uint8 msg) const;
        void SendQuestUpdateAddCredit(Quest const* quest, ObjectGuid guid, uint32 creatureOrGOIdx, uint16 count);
        void SendQuestUpdateAddPlayer(Quest const* quest, uint16 newCount);
        void SendQuestGiverStatusMultiple();

        uint32 GetSharedQuestID() const { return m_sharedQuestId; }
        ObjectGuid GetPlayerSharingQuest() const { return m_playerSharingQuest; }
        void SetQuestSharingInfo(ObjectGuid guid, uint32 id) { m_playerSharingQuest = guid; m_sharedQuestId = id; }
        void ClearQuestSharingInfo() { m_playerSharingQuest = ObjectGuid::Empty; m_sharedQuestId = 0; }

        // Returns the ID of the next available quest in chain of the most recently rewarded quest that has been rewarded via UI popup
        uint32 GetPopupQuestId() const { return m_popupQuestId; }
        void SetPopupQuestId(uint32 questId) { m_popupQuestId = questId; }

        uint32 GetInGameTime() const { return m_ingametime; }
        void SetInGameTime(uint32 time) { m_ingametime = time; }

        void AddTimedQuest(uint32 questId) { m_timedquests.insert(questId); }
        void RemoveTimedQuest(uint32 questId) { m_timedquests.erase(questId); }

        void SaveCUFProfile(uint8 id, std::nullptr_t) { _CUFProfiles[id] = nullptr; } ///> Empties a CUF profile at position 0-4
        void SaveCUFProfile(uint8 id, std::unique_ptr<CUFProfile> profile) { _CUFProfiles[id] = std::move(profile); } ///> Replaces a CUF profile at position 0-4
        CUFProfile* GetCUFProfile(uint8 id) const { return _CUFProfiles[id].get(); } ///> Retrieves a CUF profile at position 0-4
        uint8 GetCUFProfilesCount() const
        {
            uint8 count = 0;
            for (uint8 i = 0; i < MAX_CUF_PROFILES; ++i)
                if (_CUFProfiles[i])
                    ++count;
            return count;
        }

        bool HasPvPForcingQuest() const;

        /*********************************************************/
        /***                   LOAD SYSTEM                     ***/
        /*********************************************************/

        bool LoadFromDB(ObjectGuid guid, CharacterDatabaseQueryHolder const& holder);
        bool IsLoading() const override;

        void Initialize(ObjectGuid::LowType guid);
        static uint32 GetUInt32ValueFromArray(Tokenizer const& data, uint16 index);
        static float  GetFloatValueFromArray(Tokenizer const& data, uint16 index);
        static uint32 GetZoneIdFromDB(ObjectGuid guid);
        static bool   LoadPositionFromDB(uint32& mapid, float& x, float& y, float& z, float& o, bool& in_flight, ObjectGuid guid);

        static bool IsValidGender(uint8 Gender) { return Gender <= GENDER_FEMALE; }
        static bool ValidateAppearance(uint8 race, uint8 class_, uint8 gender, uint8 hairID, uint8 hairColor, uint8 faceID, uint8 facialHair, uint8 skinColor, bool create = false);
        static bool IsValidClass(uint8 Class) { return ((1 << (Class - 1)) & CLASSMASK_ALL_PLAYABLE) != 0; }
        static bool IsValidRace(uint8 Race) { return ((1 << (Race - 1)) & RACEMASK_ALL_PLAYABLE) != 0; }

        /*********************************************************/
        /***                   SAVE SYSTEM                     ***/
        /*********************************************************/

        void SaveToDB(bool create = false);
        void SaveToDB(CharacterDatabaseTransaction trans, bool create = false);
        void SaveInventoryAndGoldToDB(CharacterDatabaseTransaction& trans);                    // fast save function for item/money cheating preventing
        void SaveGoldToDB(CharacterDatabaseTransaction& trans) const;

        static void SetUInt32ValueInArray(Tokenizer& data, uint16 index, uint32 value);
        static void Customize(CharacterCustomizeInfo const* customizeInfo, CharacterDatabaseTransaction& trans);
        static void SavePositionInDB(WorldLocation const& loc, uint16 zoneId, ObjectGuid guid, CharacterDatabaseTransaction& trans);

        static void DeleteFromDB(ObjectGuid playerguid, uint32 accountId, bool updateRealmChars = true, bool deleteFinally = false);
        static void DeleteOldCharacters();
        static void DeleteOldCharacters(uint32 keepDays);

        bool m_mailsUpdated;

        void SetBindPoint(ObjectGuid guid) const;
        void SendTalentWipeConfirm(ObjectGuid guid) const;
        void ResetPetTalents();
        void RegenerateAll(uint32 diff);
        void RegenerateHealth() override;
        void setWeaponChangeTimer(uint32 time) { m_weaponChangeTimer = time;}

        uint64 GetMoney() const { return GetUInt64Value(PLAYER_FIELD_COINAGE); }
        bool ModifyMoney(int64 amount, bool sendError = true);
        bool HasEnoughMoney(uint64 amount) const { return (GetMoney() >= amount); }
        bool HasEnoughMoney(int64 amount) const;
        void SetMoney(uint64 value);

        RewardedQuestSet const& getRewardedQuests() const { return m_RewardedQuests; }
        QuestStatusMap& getQuestStatusMap() { return m_QuestStatus; }

        size_t GetRewardedQuestCount() const { return m_RewardedQuests.size(); }
        bool IsQuestRewarded(uint32 quest_id) const;

        Unit* GetSelectedUnit() const;
        Player* GetSelectedPlayer() const;

        void SetTarget(ObjectGuid /*guid*/) override { } /// Used for serverside target changes, does not apply to players
        void SetSelection(ObjectGuid guid) { SetGuidValue(UNIT_FIELD_TARGET, guid); }

        void SendMailResult(uint32 mailId, MailResponseType mailAction, MailResponseResult mailError, uint32 equipError = 0, ObjectGuid::LowType item_guid = 0, uint32 item_count = 0) const;
        void SendNewMail() const;
        void UpdateNextMailTimeAndUnreads();
        void AddNewMailDeliverTime(time_t deliver_time);

        void RemoveMail(uint32 id);

        void AddMail(Mail* mail) { m_mail.push_front(mail);}// for call from WorldSession::SendMailTo
        uint32 GetMailSize() { return m_mail.size();}
        Mail* GetMail(uint32 id);

        PlayerMails::iterator GetMailBegin() { return m_mail.begin();}
        PlayerMails::iterator GetMailEnd() { return m_mail.end();}

        void SendItemRetrievalMail(uint32 itemEntry, uint32 count); // Item retrieval mails sent by The Postmaster (34337), used in multiple places.

        /*********************************************************/
        /*** MAILED ITEMS SYSTEM ***/
        /*********************************************************/

        uint8 unReadMails;
        time_t m_nextMailDelivereTime;

        typedef std::unordered_map<uint32, Item*> ItemMap;

        ItemMap mMitems;                                    //template defined in objectmgr.cpp

        Item* GetMItem(uint32 id);
        void AddMItem(Item* it);
        bool RemoveMItem(uint32 id);

        void SendOnCancelExpectedVehicleRideAura() const;
        bool CanControlPet(uint32 spellId = 0) const;
        void PetSpellInitialize();
        void CharmSpellInitialize();
        void PossessSpellInitialize();
        void VehicleSpellInitialize();
        void SendRemoveControlBar() const;
        bool HasSpell(uint32 spell) const override;
        bool HasActiveSpell(uint32 spell) const;            // show in spellbook
        bool IsSpellFitByClassAndRace(uint32 spell_id) const;
        bool HandlePassiveSpellLearn(SpellInfo const* spellInfo);
        bool IsCurrentSpecMasterySpell(SpellInfo const* spellInfo) const;

        void SendProficiency(ItemClass itemClass, uint32 itemSubclassMask) const;
        void SendKnownSpells(bool firstLogin = false);
        bool AddSpell(uint32 spellId, bool active, bool learning, bool dependent, bool disabled, bool loading = false, uint32 fromSkill = 0);
        void LearnSpell(uint32 spell_id, bool dependent, uint32 fromSkill = 0);
        void RemoveSpell(uint32 spell_id, bool disabled = false, bool learn_low_rank = true);
        void ResetSpells(bool myClassOnly = false);
        void LearnCustomSpells();
        void LearnDefaultSkills();
        void LearnDefaultSkill(uint32 skillId, uint16 rank);
        void LearnQuestRewardedSpells();
        void LearnQuestRewardedSpells(Quest const* quest);
        void LearnSpellHighestRank(uint32 spellid);
        void AddTemporarySpell(uint32 spellId);
        void RemoveTemporarySpell(uint32 spellId);
        void SetReputation(uint32 factionentry, uint32 value);
        uint32 GetReputation(uint32 factionentry) const;
        std::string GetGuildName() const;
        void SendSpellCategoryCooldowns() const;
        void SendRaidGroupOnlyMessage(RaidGroupReason reason, int32 delay) const;

        // Talents
        uint32 GetFreeTalentPoints() const { return _talentMgr->FreeTalentPoints; }
        void SetFreeTalentPoints(uint32 points) { _talentMgr->FreeTalentPoints = points; }
        uint32 GetUsedTalentCount() const { return _talentMgr->UsedTalentCount; }
        void SetUsedTalentCount(uint32 talents) { _talentMgr->UsedTalentCount = talents; }
        uint32 GetQuestRewardedTalentCount() const { return _talentMgr->QuestRewardedTalentCount; }
        void AddQuestRewardedTalentCount(uint32 points) { _talentMgr->QuestRewardedTalentCount += points; }
        uint32 GetTalentResetCost() const { return _talentMgr->ResetTalentsCost; }
        void SetTalentResetCost(uint32 cost)  { _talentMgr->ResetTalentsCost = cost; }
        uint32 GetTalentResetTime() const { return _talentMgr->ResetTalentsTime; }
        void SetTalentResetTime(time_t time_)  { _talentMgr->ResetTalentsTime = time_; }
        uint32 GetPrimaryTalentTree(uint8 spec) const { return _talentMgr->SpecInfo[spec].TalentTree; }
        void SetPrimaryTalentTree(uint8 spec, uint32 tree) { _talentMgr->SpecInfo[spec].TalentTree = tree; UpdateArmorSpecialization(); }
        uint8 GetActiveSpec() const { return _talentMgr->ActiveSpec; }
        void SetActiveSpec(uint8 spec);
        uint8 GetSpecsCount() const { return _talentMgr->SpecsCount; }
        void SetSpecsCount(uint8 count) { _talentMgr->SpecsCount = count; }

        bool ResetTalents(bool no_cost = false);
        uint32 GetNextResetTalentsCost() const;
        void InitTalentForLevel();
        void BuildPlayerTalentsInfoData(WorldPacket* data);
        void BuildPetTalentsInfoData(WorldPacket* data);
        void SendTalentsInfoData(bool pet);
        bool LearnTalent(uint32 talentId, uint32 talentRank);
        void LearnPetTalent(ObjectGuid petGuid, uint32 talentId, uint32 talentRank);
        bool AddTalent(uint32 spellId, uint8 spec, bool learning);
        bool HasTalent(uint32 spell_id, uint8 spec) const;
        uint32 CalculateTalentsPoints() const;

        // Armor Specializaion
        void UpdateArmorSpecialization();

        // Dual Spec
        void UpdateSpecCount(uint8 count);
        void ActivateSpec(uint8 spec);
        void LoadActions(PreparedQueryResult result);

        void InitGlyphsForLevel();
        void SetGlyphSlot(uint8 slot, uint32 slottype) { SetUInt32Value(PLAYER_FIELD_GLYPH_SLOTS_1 + slot, slottype); }

        uint32 GetGlyphSlot(uint8 slot) const { return GetUInt32Value(PLAYER_FIELD_GLYPH_SLOTS_1 + slot); }
        void SetGlyph(uint8 slot, uint32 glyph);
        uint32 GetGlyph(uint8 spec, uint8 slot) const { return _talentMgr->SpecInfo[spec].Glyphs[slot]; }

        PlayerTalentMap const* GetTalentMap(uint8 spec) const { return _talentMgr->SpecInfo[spec].Talents; }
        PlayerTalentMap* GetTalentMap(uint8 spec) { return _talentMgr->SpecInfo[spec].Talents; }
        ActionButtonList const& GetActionButtons() const { return m_actionButtons; }

        uint32 GetFreePrimaryProfessionPoints() const { return GetUInt32Value(PLAYER_CHARACTER_POINTS); }
        void SetFreePrimaryProfessions(uint16 profs) { SetUInt32Value(PLAYER_CHARACTER_POINTS, profs); }
        void InitPrimaryProfessions();

        PlayerSpellMap const& GetSpellMap() const { return m_spells; }
        PlayerSpellMap      & GetSpellMap()       { return m_spells; }

        void AddSpellMod(SpellModifier* mod, bool apply);
        static bool IsAffectedBySpellmod(SpellInfo const* spellInfo, SpellModifier const* mod, Spell* spell = nullptr);
        template <class T>
        void GetSpellModValues(SpellInfo const* spellInfo, SpellModOp op, Spell* spell, T base, int32* flat, float* pct) const;
        template <class T>
        void ApplySpellMod(SpellInfo const* spellInfo, SpellModOp op, T& basevalue, Spell* spell = nullptr) const;
        static void ApplyModToSpell(SpellModifier* mod, Spell* spell);
        void SetSpellModTakingSpell(Spell* spell, bool apply);
        void SendSpellModifiers() const;

        void RemoveArenaSpellCooldowns(bool removeActivePetCooldowns = false);
        uint32 GetLastPotionId() const { return m_lastPotionId; }
        void SetLastPotionId(uint32 item_id) { m_lastPotionId = item_id; }
        void UpdatePotionCooldown(Spell* spell = nullptr);

        void SetResurrectRequestData(WorldObject const* caster, uint32 health, uint32 mana, uint32 appliedAura);
        void ClearResurrectRequestData() { _resurrectionData.reset(); }

        bool IsResurrectRequestedBy(ObjectGuid const& guid) const
        {
            if (!IsResurrectRequested())
                return false;

            return _resurrectionData->GUID == guid;
        }

        bool IsResurrectRequested() const { return _resurrectionData != nullptr; }
        void ResurrectUsingRequestData();

        uint8 getCinematic() const { return m_cinematic; }
        void setCinematic(uint8 cine) { m_cinematic = cine; }

        uint32 GetMovie() const { return m_movie; }
        void SetMovie(uint32 movie) { m_movie = movie; }

        ActionButton* addActionButton(uint8 button, uint32 action, uint8 type);
        void removeActionButton(uint8 button);
        ActionButton const* GetActionButton(uint8 button);
        void SendInitialActionButtons() const { SendActionButtons(0); }
        void SendActionButtons(uint32 state) const;
        bool IsActionButtonDataValid(uint8 button, uint32 action, uint8 type) const;

        PvPInfo pvpInfo;
        void UpdatePvPState(bool onlyFFA = false);
        void SetPvP(bool state) override;
        void UpdatePvP(bool state, bool override = false);
        void UpdateZone(uint32 newZone, uint32 newArea);
        void UpdateArea(uint32 newArea);
        void UpdateHostileAreaState(AreaTableEntry const* area);
        void SetNeedsZoneUpdate(bool needsUpdate) { m_needsZoneUpdate = needsUpdate; }

        void UpdateZoneDependentAuras(uint32 zone_id);    // zones
        void UpdateAreaDependentAuras(uint32 area_id);    // subzones

        void UpdateAfkReport(time_t currTime);
        void UpdatePvPFlag(time_t currTime);
        void SetContestedPvP(Player* attackedPlayer = nullptr);
        void UpdateContestedPvP(uint32 currTime);
        void SetContestedPvPTimer(uint32 newTime) {m_contestedPvPTimer = newTime;}
        void ResetContestedPvP();

        /// @todo: maybe move UpdateDuelFlag+DuelComplete to independent DuelHandler
        DuelInfo* duel;
        void UpdateDuelFlag(time_t currTime);
        void CheckDuelDistance(time_t currTime);
        void DuelComplete(DuelCompleteType type);
        void SendDuelCountdown(uint32 counter);

        bool IsGroupVisibleFor(Player const* p) const;
        bool IsInSameGroupWith(Player const* p) const;
        bool IsInSameRaidWith(Player const* p) const;
        void UninviteFromGroup();
        static void RemoveFromGroup(Group* group, ObjectGuid guid, RemoveMethod method = GROUP_REMOVEMETHOD_DEFAULT, ObjectGuid kicker = ObjectGuid::Empty, char const* reason = nullptr);
        void RemoveFromGroup(RemoveMethod method = GROUP_REMOVEMETHOD_DEFAULT) { RemoveFromGroup(GetGroup(), GetGUID(), method); }
        void SendUpdateToOutOfRangeGroupMembers();

        void SetInGuild(uint32 guildId);
        void SetGuildRank(uint8 rankId) { SetUInt32Value(PLAYER_GUILDRANK, rankId); }
        uint8 GetGuildRank() const { return uint8(GetUInt32Value(PLAYER_GUILDRANK)); }
        void SetGuildLevel(uint32 level) { SetUInt32Value(PLAYER_GUILDLEVEL, level); }
        uint32 GetGuildLevel() { return GetUInt32Value(PLAYER_GUILDLEVEL); }
        void SetGuildIdInvited(uint32 GuildId) { m_GuildIdInvited = GuildId; }
        uint32 GetGuildId() const { return GetUInt32Value(OBJECT_FIELD_DATA); /* return only lower part */ }
        Guild* GetGuild();
        int GetGuildIdInvited() const { return m_GuildIdInvited; }
        static void RemovePetitionsAndSigns(ObjectGuid guid, CharterTypes type);

        // Arena Team
        void SetInArenaTeam(uint32 ArenaTeamId, uint8 slot, uint8 type);
        void SetArenaTeamInfoField(uint8 slot, ArenaTeamInfoType type, uint32 value);
        static void LeaveAllArenaTeams(ObjectGuid guid);
        uint32 GetArenaTeamId(uint8 slot) const { return GetUInt32Value(PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + (slot * ARENA_TEAM_END) + ARENA_TEAM_ID); }
        uint32 GetArenaPersonalRating(uint8 slot) const { return GetUInt32Value(PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + (slot * ARENA_TEAM_END) + ARENA_TEAM_PERSONAL_RATING); }
        void SetArenaTeamIdInvited(uint32 ArenaTeamId) { m_ArenaTeamIdInvited = ArenaTeamId; }
        uint32 GetArenaTeamIdInvited() const { return m_ArenaTeamIdInvited; }
        uint32 GetRBGPersonalRating() const { return 0; }

        Difficulty GetDifficulty(bool isRaid) const { return isRaid ? m_raidDifficulty : m_dungeonDifficulty; }
        Difficulty GetDungeonDifficulty() const { return m_dungeonDifficulty; }
        Difficulty GetRaidDifficulty() const { return m_raidDifficulty; }
        Difficulty GetStoredRaidDifficulty() const { return m_raidMapDifficulty; } // only for use in difficulty packet after exiting to raid map
        void SetDungeonDifficulty(Difficulty dungeon_difficulty) { m_dungeonDifficulty = dungeon_difficulty; }
        void SetRaidDifficulty(Difficulty raid_difficulty) { m_raidDifficulty = raid_difficulty; }
        void StoreRaidMapDifficulty();

        bool UpdateSkill(uint32 skill_id, uint32 step);
        bool UpdateSkillPro(uint16 skillId, int32 chance, uint32 step);

        bool UpdateCraftSkill(SpellInfo const* spellInfo);
        bool UpdateGatherSkill(uint32 SkillId, uint32 SkillValue, uint32 RedLevel, uint32 Multiplicator = 1);
        bool UpdateFishingSkill();

        void SurveyDigSite();
        void NotifyRequestResearchHistory();
        bool ArchProjectCompleteable(uint16 projectId);
        bool HasArchProject(uint16 projectId);
        void CompleteArchProject(uint16 projectId);
        void SetArchData(ArchData const& data);

        float GetHealthBonusFromStamina();
        float GetManaBonusFromIntellect();

        bool UpdateStats(Stats stat) override;
        bool UpdateAllStats() override;
        void ApplySpellPenetrationBonus(int32 amount, bool apply);
        void UpdateResistances(uint32 school) override;
        void UpdateArmor() override;
        void UpdateMaxHealth() override;
        void UpdateMaxPower(Powers power) override;
        uint32 GetPowerIndex(Powers power) const override;
        void UpdateAttackPowerAndDamage(bool ranged = false) override;
        void ApplySpellPowerBonus(int32 amount, bool apply);
        void UpdateSpellDamageAndHealingBonus();
        void UpdateSpellHealingPercentDone();
        void UpdateSpellHealingPercentTaken();
        void ApplyRatingMod(CombatRating cr, int32 value, bool apply);
        void UpdateRating(CombatRating cr);
        void UpdateAllRatings();
        void UpdateMastery();
        bool CanUseMastery() const;

        void CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& minDamage, float& maxDamage) const override;

        void RecalculateRating(CombatRating cr) { ApplyRatingMod(cr, 0, true);}
        float GetMeleeCritFromAgility() const;
        void GetDodgeFromAgility(float &diminishing, float &nondiminishing) const;
        float GetSpellCritFromIntellect() const;
        float GetRatingMultiplier(CombatRating cr) const;
        float GetRatingBonusValue(CombatRating cr) const;

        /// Returns base spellpower bonus from spellpower stat on items, without spellpower from intellect stat
        uint32 GetBaseSpellPowerBonus() const { return m_baseSpellPower; }
        int32 GetSpellPenetrationItemMod() const { return m_spellPenetrationItemMod; }

        bool CanApplyResilience() const override { return true; }

        float GetExpertiseDodgeOrParryReduction(WeaponAttackType attType) const;
        void UpdateBlockPercentage();
        void UpdateCritPercentage(WeaponAttackType attType);
        void UpdateAllCritPercentages();
        void UpdateParryPercentage();
        void UpdateDodgePercentage();
        void UpdateMeleeHitChances();
        void UpdateRangedHitChances();
        void UpdateSpellHitChances();
        void UpdateHitChances();

        void UpdateAllSpellCritChances();
        void UpdateSpellCritChance(uint32 school);
        void UpdateExpertise(WeaponAttackType attType);
        void ApplyManaRegenBonus(int32 amount, bool apply);
        void ApplyHealthRegenBonus(int32 amount, bool apply);
        uint32 GetRuneTimer(uint8 index) const { return m_runeGraceCooldown[index]; }
        void SetRuneTimer(uint8 index, uint32 timer) { m_runeGraceCooldown[index] = timer; }
        uint32 GetLastRuneGraceTimer(uint8 index) const { return m_lastRuneGraceTimers[index]; }
        void SetLastRuneGraceTimer(uint8 index, uint32 timer) { m_lastRuneGraceTimers[index] = timer; }

        ObjectGuid GetLootGUID() const { return m_lootGuid; }
        void SetLootGUID(ObjectGuid guid) { m_lootGuid = guid; }

        void RemovedInsignia(Player* looterPlr);

        WorldSession* GetSession() const { return m_session; }
        GameClient* GetGameClient() const;

        void BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target) const override;
        void DestroyForPlayer(Player* target, bool onDeath = false) const override;

        // notifiers
        void SendAttackSwingCantAttack() const;
        void SendAttackSwingCancelAttack() const;
        void SendAttackSwingDeadTarget() const;
        void SendAttackSwingNotInRange() const;
        void SendAttackSwingBadFacingAttack() const;
        void SendAutoRepeatCancel(Unit* target);
        void SendExplorationExperience(uint32 Area, uint32 Experience) const;

        void SendDungeonDifficulty(bool IsInGroup) const;
        void SendRaidDifficulty(bool IsInGroup, int32 forcedDifficulty = -1) const;
        void ResetInstances(uint8 method, bool isRaid);
        void SendResetInstanceSuccess(uint32 MapId) const;
        void SendResetInstanceFailed(uint32 reason, uint32 MapId) const;
        void SendResetFailedNotify(uint32 mapid) const;

        bool UpdatePosition(float x, float y, float z, float orientation, bool teleport = false) override;
        bool UpdatePosition(Position const& pos, bool teleport = false) override { return UpdatePosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), teleport); }
        void ProcessTerrainStatusUpdate(ZLiquidStatus oldLiquidStatus, Optional<LiquidData> const& newLiquidData) override;
        void AtExitCombat() override;

        void SendMessageToSet(WorldPacket const* data, bool self) const override { SendMessageToSetInRange(data, GetVisibilityRange(), self); }
        void SendMessageToSetInRange(WorldPacket const* data, float dist, bool self) const override;
        void SendMessageToSetInRange(WorldPacket const* data, float dist, bool self, bool own_team_only) const;
        void SendMessageToSet(WorldPacket const* data, Player const* skipped_rcvr) const override;

        Corpse* GetCorpse() const;
        void SpawnCorpseBones(bool triggerSave = true);
        Corpse* CreateCorpse();
        void KillPlayer();
        static void OfflineResurrect(ObjectGuid const& guid, CharacterDatabaseTransaction& trans);
        bool HasCorpse() const { return _corpseLocation.GetMapId() != MAPID_INVALID; }
        WorldLocation GetCorpseLocation() const { return _corpseLocation; }
        uint32 GetResurrectionSpellId();
        void ResurrectPlayer(float restore_percent, bool applySickness = false);
        void BuildPlayerRepop();
        void RepopAtGraveyard();

        void DurabilityLossAll(double percent, bool inventory);
        void DurabilityLoss(Item* item, double percent);
        void DurabilityPointsLossAll(int32 points, bool inventory);
        void DurabilityPointsLoss(Item* item, int32 points);
        void DurabilityPointLossForEquipSlot(EquipmentSlots slot);
        void DurabilityRepairAll(bool takeCost, float discountMod, bool guildBank);
        void DurabilityRepair(uint16 pos, bool takeCost, float discountMod);

        void UpdateMirrorTimers();
        void StopMirrorTimers();
        bool IsMirrorTimerActive(MirrorTimerType type) const;

        bool CanJoinConstantChannelInZone(ChatChannelsEntry const* channel, AreaTableEntry const* zone) const;

        void JoinedChannel(Channel* c);
        void LeftChannel(Channel* c);
        void CleanupChannels();
        void UpdateLocalChannels(uint32 newZone);
        void LeaveLFGChannel();

        typedef std::list<Channel*> JoinedChannelsList;
        JoinedChannelsList const& GetJoinedChannels() const { return m_channels; }

        void InitializeSkillFields();
        void SetSkill(uint16 id, uint16 step, uint16 newVal, uint16 maxVal);
        uint16 GetMaxSkillValue(uint32 skill) const;        // max + perm. bonus + temp bonus
        uint16 GetPureMaxSkillValue(uint32 skill) const;    // max
        uint16 GetSkillValue(uint32 skill) const;           // skill value + perm. bonus + temp bonus
        uint16 GetBaseSkillValue(uint32 skill) const;       // skill value + perm. bonus
        uint16 GetPureSkillValue(uint32 skill) const;       // skill value
        int16 GetSkillPermBonusValue(uint32 skill) const;
        int16 GetSkillTempBonusValue(uint32 skill) const;
        uint16 GetSkillStep(uint16 skill) const;            // 0...6
        bool HasSkill(uint32 skill) const;
        void LearnSkillRewardedSpells(uint32 skillId, uint32 skillValue);

        uint32 GetSkillRank(uint8 pos) const;
        void SetSkillLineId(uint8 pos, uint16 value);
        void SetSkillStep(uint8 pos, uint16 value);
        void SetSkillRank(uint8 pos, uint16 value);
        void SetSkillMaxRank(uint8 pos, uint16 value);
        void SetSkillTempBonus(uint8 pos, uint16 value);
        void SetSkillPermBonus(uint8 pos, uint16 value);

        WorldLocation& GetTeleportDest() { return m_teleport_dest; }
        Optional<uint32> GetTeleportDestInstanceId() const { return m_teleport_instanceId; }
        bool IsBeingTeleported() const { return mSemaphoreTeleport_Near || mSemaphoreTeleport_Far; }
        bool IsBeingTeleportedNear() const { return mSemaphoreTeleport_Near; }
        bool IsBeingTeleportedFar() const { return mSemaphoreTeleport_Far; }
        void SetSemaphoreTeleportNear(bool semphsetting) { mSemaphoreTeleport_Near = semphsetting; }
        void SetSemaphoreTeleportFar(bool semphsetting) { mSemaphoreTeleport_Far = semphsetting; }
        void ProcessDelayedOperations();

        void CheckAreaExplore();
        void SetAreaExplored(uint32 areaId);

        // These methods are used to periodically update certain area and aura based mechanics used in Heartbeat and Movement
        void UpdateZoneAndAreaId();
        void UpdateIndoorsOutdoorsAuras();
        void UpdateTavernRestingState();

        static uint32 TeamForRace(uint8 race);
        uint32 GetTeam() const { return m_team; }
        TeamId GetTeamId() const { return m_team == ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE; }
        void SetFactionForRace(uint8 race);

        void InitDisplayIds();

        bool IsAtGroupRewardDistance(WorldObject const* pRewardSource) const;
        bool IsAtRecruitAFriendDistance(WorldObject const* pOther) const;
        void RewardPlayerAndGroupAtKill(Unit* victim, bool isBattleGround);
        void RewardPlayerAndGroupAtEvent(uint32 creature_id, WorldObject* pRewardSource);
        bool isHonorOrXPTarget(Unit const* victim) const;

        bool GetsRecruitAFriendBonus(bool forXP);
        uint8 GetGrantableLevels() const { return m_grantableLevels; }
        void SetGrantableLevels(uint8 val) { m_grantableLevels = val; }

        ReputationMgr&       GetReputationMgr()       { return *m_reputationMgr; }
        ReputationMgr const& GetReputationMgr() const { return *m_reputationMgr; }
        ReputationRank GetReputationRank(uint32 faction_id) const;
        void RewardOnKill(Unit* victim, float rate);
        void RewardReputation(Quest const* quest);

        int32 CalculateReputationGain(ReputationSource source, uint32 creatureOrQuestLevel, int32 rep, int32 faction, bool noQuestBonus = false);

        void UpdateSkillsForLevel();
        void ModifySkillBonus(uint32 skillid, int32 val, bool talent);

        void SetLastSoulburnSpell(SpellInfo const* spell) { m_lastSoulburnSpell = spell; }
        SpellInfo const* GetLastSoulburnSpell() const { return m_lastSoulburnSpell; }

        /*********************************************************/
        /***                  PVP SYSTEM                       ***/
        /*********************************************************/
        // TODO: Properly implement correncies as of Cataclysm
        void UpdateHonorFields();
        bool RewardHonor(Unit* victim, uint32 groupsize, int32 honor = -1, bool pvptoken = false);
        uint32 GetMaxPersonalArenaRatingRequirement(uint32 minarenaslot) const;

        // duel health and mana reset methods
        void SaveHealthBeforeDuel() { healthBeforeDuel = GetHealth(); }
        void SaveManaBeforeDuel() { manaBeforeDuel = GetPower(POWER_MANA); }
        void RestoreHealthAfterDuel() { SetHealth(healthBeforeDuel); }
        void RestoreManaAfterDuel() { SetPower(POWER_MANA, manaBeforeDuel); }

        //End of PvP System

        void SetDrunkValue(uint8 newDrunkValue, uint32 itemId = 0);
        uint8 GetDrunkValue() const { return GetByteValue(PLAYER_BYTES_3, PLAYER_BYTES_3_OFFSET_INEBRIATION); }
        static DrunkenState GetDrunkenstateByValue(uint8 value);

        uint32 GetDeathTimer() const { return m_deathTimer; }
        uint32 GetCorpseReclaimDelay(bool pvp) const;
        void UpdateCorpseReclaimDelay();
        int32 CalculateCorpseReclaimDelay(bool load = false) const;
        void SendCorpseReclaimDelay(uint32 delay) const;

        uint32 GetBlockPercent() const override { return GetUInt32Value(PLAYER_SHIELD_BLOCK); }
        bool CanParry() const { return m_canParry; }
        void SetCanParry(bool value);
        bool CanBlock() const { return m_canBlock; }
        void SetCanBlock(bool value);
        bool CanTitanGrip() const { return m_canTitanGrip; }
        void SetCanTitanGrip(bool value, uint32 penaltySpellId = 0);
        void CheckTitanGripPenalty();
        bool CanTameExoticPets() const { return IsGameMaster() || HasAuraType(SPELL_AURA_ALLOW_TAME_PET_TYPE); }

        void SetRegularAttackTime();

        void HandleBaseModFlatValue(BaseModGroup modGroup, float amount, bool apply);
        void ApplyBaseModPctValue(BaseModGroup modGroup, float pct);

        void SetBaseModFlatValue(BaseModGroup modGroup, float val);
        void SetBaseModPctValue(BaseModGroup modGroup, float val);

        void UpdateDamageDoneMods(WeaponAttackType attackType, int32 skipEnchantSlot = -1) override;
        void UpdateBaseModGroup(BaseModGroup modGroup);

        float GetBaseModValue(BaseModGroup modGroup, BaseModType modType) const;
        float GetTotalBaseModValue(BaseModGroup modGroup) const;

        void _ApplyAllStatBonuses();
        void _RemoveAllStatBonuses();

        void ResetAllPowers();

        SpellSchoolMask GetMeleeDamageSchoolMask(WeaponAttackType attackType = BASE_ATTACK) const override;

        void CastAllObtainSpells();
        void ApplyItemObtainSpells(Item* item, bool apply);

        void UpdateWeaponDependentCritAuras(WeaponAttackType attackType);
        void UpdateAllWeaponDependentCritAuras();

        void UpdateWeaponDependentAuras(WeaponAttackType attackType);
        void ApplyItemDependentAuras(Item* item, bool apply);

        bool CheckAttackFitToAuraRequirement(WeaponAttackType attackType, AuraEffect const* aurEff) const override;

        void _ApplyItemMods(Item* item, uint8 slot, bool apply, bool updateItemAuras = true);
        void _RemoveAllItemMods();
        void _ApplyAllItemMods();
        void _ApplyAllLevelScaleItemMods(bool apply);
        ScalingStatDistributionEntry const* GetScalingStatDistributionFor(ItemTemplate const& itemTemplate) const;
        ScalingStatValuesEntry const* GetScalingStatValuesFor(ItemTemplate const& itemTemplate) const;
        void _ApplyItemBonuses(ItemTemplate const* proto, uint8 slot, bool apply, bool only_level_scale = false);
        void _ApplyWeaponDamage(uint8 slot, ItemTemplate const* proto, bool apply);
        bool EnchantmentFitsRequirements(uint32 enchantmentcondition, int8 slot) const;
        void ToggleMetaGemsActive(uint8 exceptslot, bool apply);
        void CorrectMetaGemEnchants(uint8 slot, bool apply);
        void InitDataForForm(bool reapplyMods = false);

        void ApplyItemEquipSpell(Item* item, bool apply, bool form_change = false);
        void ApplyEquipSpell(SpellInfo const* spellInfo, Item* item, bool apply, bool form_change = false);
        void UpdateEquipSpellsAtFormChange();
        void CastItemCombatSpell(DamageInfo const& damageInfo);
        void CastItemCombatSpell(DamageInfo const& damageInfo, Item* item, ItemTemplate const* proto);
        void CastItemUseSpell(Item* item, SpellCastTargets const& targets, uint8 castId, uint32 glyphIndex);

        void SendEquipmentSetList();
        void SetEquipmentSet(EquipmentSetInfo::EquipmentSetData const& eqset);
        void DeleteEquipmentSet(uint64 setGuid);

        void SendInitWorldStates(uint32 zone, uint32 area);
        void SendUpdateWorldState(uint32 variable, uint32 value, bool hidden = false) const;
        void SendDirectMessage(WorldPacket const* data) const;

        void SendAurasForTarget(Unit* target) const;

        std::unique_ptr<PlayerMenu> PlayerTalkClass;
        std::vector<ItemSetEffect*> ItemSetEff;

        void SendLoot(ObjectGuid guid, LootType loot_type);
        void SendLootError(ObjectGuid guid, LootError error) const;
        void SendLootRelease(ObjectGuid guid) const;
        void SendNotifyLootItemRemoved(uint8 lootSlot) const;
        void SendNotifyCurrencyLootRemoved(uint8 lootSlot);
        void SendNotifyLootMoneyRemoved() const;

        /*********************************************************/
        /***               BATTLEGROUND SYSTEM                 ***/
        /*********************************************************/

        bool InBattleground()       const                { return m_bgData.bgInstanceID != 0; }
        bool InArena()              const;
        uint32 GetBattlegroundId()  const                { return m_bgData.bgInstanceID; }
        BattlegroundTypeId GetBattlegroundTypeId() const { return m_bgData.bgTypeID; }
        Battleground* GetBattleground() const;

        uint32 GetBattlegroundQueueJoinTime(uint32 bgTypeId) const;
        void AddBattlegroundQueueJoinTime(uint32 bgTypeId, uint32 joinTime);
        void RemoveBattlegroundQueueJoinTime(uint32 bgTypeId);

        bool InBattlegroundQueue() const;

        BattlegroundQueueTypeId GetBattlegroundQueueTypeId(uint32 index) const;
        uint32 GetBattlegroundQueueIndex(BattlegroundQueueTypeId bgQueueTypeId) const;
        bool IsInvitedForBattlegroundQueueType(BattlegroundQueueTypeId bgQueueTypeId) const;
        bool InBattlegroundQueueForBattlegroundQueueType(BattlegroundQueueTypeId bgQueueTypeId) const;

        void SetBattlegroundId(uint32 val, BattlegroundTypeId bgTypeId);
        uint32 AddBattlegroundQueueId(BattlegroundQueueTypeId val);
        bool HasFreeBattlegroundQueueId() const;
        void RemoveBattlegroundQueueId(BattlegroundQueueTypeId val);
        void SetInviteForBattlegroundQueueType(BattlegroundQueueTypeId bgQueueTypeId, uint32 instanceId);
        bool IsInvitedForBattlegroundInstance(uint32 instanceId) const;
        WorldLocation const& GetBattlegroundEntryPoint() const { return m_bgData.joinPos; }
        void SetBattlegroundEntryPoint();

        void SetBGTeam(uint32 team);
        uint32 GetBGTeam() const;

        void LeaveBattleground(bool teleportToEntryPoint = true);
        bool CanJoinToBattleground(Battleground const* bg) const;
        bool CanReportAfkDueToLimit();
        void ReportedAfkBy(Player* reporter);
        void ClearAfkReports() { m_bgData.bgAfkReporter.clear(); }

        bool GetBGAccessByLevel(BattlegroundTypeId bgTypeId) const;
        bool isTotalImmunity() const;
        bool CanUseBattlegroundObject(GameObject* gameobject) const;
        bool isTotalImmune() const;
        bool CanCaptureTowerPoint() const;

        bool GetRandomWinner() const { return m_IsBGRandomWinner; }
        void SetRandomWinner(bool isWinner);

        /*********************************************************/
        /***               OUTDOOR PVP SYSTEM                  ***/
        /*********************************************************/

        OutdoorPvP* GetOutdoorPvP() const;
        // returns true if the player is in active state for outdoor pvp objective capturing, false otherwise
        bool IsOutdoorPvPActive() const;

        /*********************************************************/
        /***              ENVIROMENTAL SYSTEM                  ***/
        /*********************************************************/

        bool IsImmuneToEnvironmentalDamage() const;
        uint32 EnvironmentalDamage(EnviromentalDamage type, uint32 damage);

        /*********************************************************/
        /***               FLOOD FILTER SYSTEM                 ***/
        /*********************************************************/

        void UpdateSpeakTime();
        bool CanSpeak() const;
        void ChangeSpeakTime(int utime);

        /*********************************************************/
        /***                 VARIOUS SYSTEMS                   ***/
        /*********************************************************/
        void UpdateFallInformationIfNeed(MovementInfo const& minfo, uint16 opcode);

        WorldObject* m_seer;
        void SetFallInformation(uint32 time, float z);
        void HandleFall(MovementInfo const& movementInfo);
        bool SetDisableGravity(bool disable, bool updateAnimTier = true) override;
        bool SetCanFly(bool enable) override;
        bool SetCanTransitionBetweenSwimAndFly(bool enable) override;
        void SendMovementSetCollisionHeight(float height, UpdateCollisionHeightReason reason);

        void SetClientControl(Unit* target, bool allowMove);

        void SetSeer(WorldObject* target) { m_seer = target; }
        void SetViewpoint(WorldObject* target, bool apply);
        WorldObject* GetViewpoint() const;
        void StopCastingCharm();
        void StopCastingBindSight() const;

        uint32 GetSaveTimer() const { return m_nextSave; }
        void   SetSaveTimer(uint32 timer) { m_nextSave = timer; }

        void SaveRecallPosition()
        {
            m_recall_location.WorldRelocate(*this);
            m_recall_instanceId = GetInstanceId();
        }
        void Recall() { TeleportTo(m_recall_location, TELE_TO_NONE, m_recall_instanceId); }

        void SetHomebind(WorldLocation const& loc, uint32 areaId);
        void SendBindPointUpdate() const;

        // Homebind coordinates
        uint32 m_homebindMapId;
        uint16 m_homebindAreaId;
        float m_homebindX;
        float m_homebindY;
        float m_homebindZ;

        WorldLocation GetStartPosition() const;

        // currently visible objects at player client
        GuidUnorderedSet m_clientGUIDs;
        GuidUnorderedSet m_visibleTransports;

        bool HaveAtClient(Object const* u) const;

        bool IsNeverVisible() const override;

        bool IsVisibleGloballyFor(Player const* player) const;

        void SendInitialVisiblePackets(Unit* target) const;
        void UpdateObjectVisibility(bool forced = true) override;
        void UpdateVisibilityForPlayer();
        void UpdateVisibilityOf(WorldObject* target);
        void UpdateTriggerVisibility();

        template<class T>
        void UpdateVisibilityOf(T* target, UpdateData& data, std::set<Unit*>& visibleNow);

        uint8 m_forced_speed_changes[MAX_MOVE_TYPE];

        bool HasAtLoginFlag(AtLoginFlags f) const { return (m_atLoginFlags & f) != 0; }
        void SetAtLoginFlag(AtLoginFlags f) { m_atLoginFlags |= f; }
        void RemoveAtLoginFlag(AtLoginFlags flags, bool persist = false);

        bool isUsingLfg() const;
        bool inRandomLfgDungeon() const;

        void GetLFGLeavePoint(Position* pos);
        bool HasValidLFGLeavePoint(uint32 mapid);
        void SetLFGLeavePoint();

        // Temporarily removed pet cache
        uint32 GetTemporaryUnsummonedPetNumber() const { return m_temporaryUnsummonedPetNumber; }
        void SetTemporaryUnsummonedPetNumber(uint32 petnumber) { m_temporaryUnsummonedPetNumber = petnumber; }
        void UnsummonPetTemporaryIfAny();
        void ResummonPetTemporaryUnSummonedIfAny();
        bool IsPetNeedBeTemporaryUnsummoned() const;

        void SendCinematicStart(uint32 cinematicId);
        void SendMovieStart(uint32 movieId);

        uint32 DoRandomRoll(uint32 minimum, uint32 maximum);

        /*********************************************************/
        /***                 INSTANCE SYSTEM                   ***/
        /*********************************************************/

        typedef std::unordered_map< uint32 /*mapId*/, InstancePlayerBind > BoundInstancesMap;

        void UpdateHomebindTime(uint32 time);

        uint32 m_HomebindTimer;
        bool m_InstanceValid;
        // permanent binds and solo binds by difficulty
        BoundInstancesMap m_boundInstances[MAX_DIFFICULTY];
        InstancePlayerBind* GetBoundInstance(uint32 mapid, Difficulty difficulty, bool withExpired = false);
        InstancePlayerBind const* GetBoundInstance(uint32 mapid, Difficulty difficulty, bool withExpired = false) const;
        BoundInstancesMap& GetBoundInstances(Difficulty difficulty) { return m_boundInstances[difficulty]; }
        InstanceSave* GetInstanceSave(uint32 mapid, bool raid);
        void UnbindInstance(uint32 mapid, Difficulty difficulty, bool unload = false);
        void UnbindInstance(BoundInstancesMap::iterator &itr, Difficulty difficulty, bool unload = false);
        InstancePlayerBind* BindToInstance(InstanceSave* save, bool permanent, BindExtensionState extendState = EXTEND_STATE_NORMAL, bool load = false);
        void BindToInstance();
        void SetPendingBind(uint32 instanceId, uint32 bindTimer);
        bool HasPendingBind() const { return _pendingBindId > 0; }
        void SendRaidInfo();
        void SendSavedInstances();
        bool Satisfy(AccessRequirement const* ar, uint32 target_map, bool report = false);
        bool CheckInstanceValidity(bool /*isLogin*/);
        bool CheckInstanceCount(uint32 instanceId) const;
        void AddInstanceEnterTime(uint32 instanceId, time_t enterTime);

        // last used pet number (for BG's)
        uint32 GetLastPetNumber() const { return m_lastpetnumber; }
        void SetLastPetNumber(uint32 petnumber) { m_lastpetnumber = petnumber; }

        /*********************************************************/
        /***                   GROUP SYSTEM                    ***/
        /*********************************************************/

        bool IsInGroup(ObjectGuid groupGuid) const;
        Group* GetGroupInvite() const { return m_groupInvite; }
        void SetGroupInvite(Group* group) { m_groupInvite = group; }
        Group* GetGroup() { return m_group.getTarget(); }
        Group const* GetGroup() const { return const_cast<Group const*>(m_group.getTarget()); }
        GroupReference& GetGroupRef() { return m_group; }
        void SetGroup(Group* group, int8 subgroup = -1);
        uint8 GetSubGroup() const { return m_group.getSubGroup(); }
        uint32 GetGroupUpdateFlag() const { return m_groupUpdateMask; }
        void SetGroupUpdateFlag(uint32 flag) { m_groupUpdateMask |= flag; }
        uint64 GetAuraUpdateMaskForRaid() const { return m_auraRaidUpdateMask; }
        void SetAuraUpdateMaskForRaid(uint8 slot) { m_auraRaidUpdateMask |= (uint64(1) << slot); }
        Player* GetNextRandomRaidMember(float radius);
        PartyResult CanUninviteFromGroup(ObjectGuid guidMember = ObjectGuid::Empty) const;

        // Battleground / Battlefield Group System
        void SetBattlegroundOrBattlefieldRaid(Group* group, int8 subgroup = -1);
        void RemoveFromBattlegroundOrBattlefieldRaid();
        Group* GetOriginalGroup() const { return m_originalGroup.getTarget(); }
        GroupReference& GetOriginalGroupRef() { return m_originalGroup; }
        uint8 GetOriginalSubGroup() const { return m_originalGroup.getSubGroup(); }
        void SetOriginalGroup(Group* group, int8 subgroup = -1);

        void SetPassOnGroupLoot(bool bPassOnGroupLoot) { m_bPassOnGroupLoot = bPassOnGroupLoot; }
        bool GetPassOnGroupLoot() const { return m_bPassOnGroupLoot; }

        MapReference &GetMapRef() { return m_mapRef; }

        // Set map to player and add reference
        void SetMap(Map* map) override;
        void ResetMap() override;

        bool isAllowedToLoot(Creature const* creature);

        DeclinedName const* GetDeclinedNames() const { return m_declinedname.get(); }

        uint8 GetRunesState() const { return m_runes->runeState; }
        RuneType GetBaseRune(uint8 index) const { return RuneType(m_runes->runes[index].BaseRune); }
        RuneType GetCurrentRune(uint8 index) const { return RuneType(m_runes->runes[index].CurrentRune); }
        uint32 GetRuneCooldown(uint8 index) const { return m_runes->runes[index].Cooldown; }
        bool IsBaseRuneSlotsOnCooldown(RuneType runeType) const;
        RuneType GetLastUsedRune() { return m_runes->lastUsedRune; }
        uint8 GetLastUsedRuneMask() { return m_runes->lastUsedRuneMask; }
        void ClearLastUsedRuneMask() { m_runes->lastUsedRuneMask = 0; }
        void SetLastUsedRune(RuneType type) { m_runes->lastUsedRune = type; }
        void SetLastUsedRuneIndex(uint8 index) { m_runes->lastUsedRuneMask |= (1 << index); }
        void SetBaseRune(uint8 index, RuneType baseRune) { m_runes->runes[index].BaseRune = baseRune; }
        void SetCurrentRune(uint8 index, RuneType currentRune) { m_runes->runes[index].CurrentRune = currentRune; }
        void SetRuneConvertAura(uint8 index, AuraEffect const* aura, AuraType auraType, SpellInfo const* spellInfo);
        void AddRuneByAuraEffect(uint8 index, RuneType newType, AuraEffect const* aura, AuraType auraType, SpellInfo const* spellInfo);
        void SetRuneCooldown(uint8 index, uint32 cooldown);
        void RemoveRunesByAuraEffect(AuraEffect const* aura);
        void RestoreBaseRune(uint8 index);
        void ConvertRune(uint8 index, RuneType newType);
        void ResyncRunes();
        void SendConvertedRunes();
        void AddRunePower(uint8 mask);
        void InitRunes();

        void SendRespondInspectAchievements(Player* player) const;
        uint32 GetAchievementPoints() const;
        bool HasAchieved(uint32 achievementId) const;
        void ResetAchievements();
        void FailAchievementCriteria(AchievementCriteriaFailEvent condition, int32 failAsset);
        void UpdateAchievementCriteria(AchievementCriteriaTypes type, uint64 miscValue1 = 0, uint64 miscValue2 = 0, uint64 miscValue3 = 0, WorldObject* ref = nullptr, GameObject* go = nullptr);
        void StartAchievementCriteria(AchievementCriteriaStartEvent startEvent, uint32 entry, Milliseconds timeLost = Milliseconds::zero());
        void CompletedAchievement(AchievementEntry const* entry);

        bool HasTitle(uint32 bitIndex) const;
        bool HasTitle(CharTitlesEntry const* title) const;
        void SetTitle(CharTitlesEntry const* title, bool lost = false);

        //bool isActiveObject() const { return true; }
        bool CanSeeSpellClickOn(Creature const* creature) const;

        uint32 GetChampioningFaction() const { return m_ChampioningFaction; }
        void SetChampioningFaction(uint32 faction) { m_ChampioningFaction = faction; }
        uint8 GetExpansionForFaction(uint32 factionId) const;

        Spell* m_spellModTakingSpell;

        float GetAverageItemLevel() const;
        bool isDebugAreaTriggers;

        void ClearWhisperWhiteList() { WhisperList.clear(); }
        void AddWhisperWhiteList(ObjectGuid guid) { WhisperList.push_back(guid); }
        bool IsInWhisperWhiteList(ObjectGuid guid);
        void RemoveFromWhisperWhiteList(ObjectGuid guid) { WhisperList.remove(guid); }

        void ReadMovementInfo(WorldPacket& data, MovementInfo* mi, Movement::ExtraMovementStatusElement* extras = nullptr);

        /*! These methods send different packets to the client in apply and unapply case.
            These methods are only sent to the current unit.
        */

        void ValidateMovementInfo(MovementInfo* mi);

        bool CanFly() const override { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_CAN_FLY); }
        bool CanEnterWater() const override { return true; }

        std::string GetMapAreaAndZoneString() const;
        std::string GetCoordsMapAreaAndZoneString() const;

        // Void Storage
        bool IsVoidStorageUnlocked() const { return HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_VOID_UNLOCKED); }
        void UnlockVoidStorage() { SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_VOID_UNLOCKED); }
        void LockVoidStorage() { RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_VOID_UNLOCKED); }
        uint8 GetNextVoidStorageFreeSlot() const;
        uint8 GetNumOfVoidStorageFreeSlots() const;
        uint8 AddVoidStorageItem(VoidStorageItem const& item);
        void AddVoidStorageItemAtSlot(uint8 slot, const VoidStorageItem& item);
        void DeleteVoidStorageItem(uint8 slot);
        bool SwapVoidStorageItem(uint8 oldSlot, uint8 newSlot);
        VoidStorageItem* GetVoidStorageItem(uint8 slot) const;
        VoidStorageItem* GetVoidStorageItem(uint64 id, uint8& slot) const;

        // Pets
        PlayerPetData* GetPlayerPetDataById(uint32 petId);
        PlayerPetData* GetPlayerPetDataBySlot(uint8 slot);
        PlayerPetData* GetPlayerPetDataByCreatureId(uint32 creatureId);
        PlayerPetData* GetPlayerPetDataCurrent();
        Optional<uint8> GetFirstUnusedActivePetSlot();
        Optional<uint8> GetFirstUnusedPetSlot();
        void DeleteFromPlayerPetDataStore(uint32 petNumber);
        void AddToPlayerPetDataStore(PlayerPetData* playerPetData);

    protected:
        // Gamemaster whisper whitelist
        GuidList WhisperList;
        uint32 m_contestedPvPTimer;

        /*********************************************************/
        /***               BATTLEGROUND SYSTEM                 ***/
        /*********************************************************/

        /*
        this is an array of BG queues (BgTypeIDs) in which is player
        */
        struct BgBattlegroundQueueID_Rec
        {
            BattlegroundQueueTypeId bgQueueTypeId;
            uint32 invitedToInstance;
        };

        BgBattlegroundQueueID_Rec m_bgBattlegroundQueueID[PLAYER_MAX_BATTLEGROUND_QUEUES];
        BGData                    m_bgData;

        bool m_IsBGRandomWinner;

        /*********************************************************/
        /***                    QUEST SYSTEM                   ***/
        /*********************************************************/

        //We allow only one timed quest active at the same time. Below can then be simple value instead of set.
        typedef std::set<uint32> QuestSet;
        typedef std::unordered_map<uint32, time_t> SeasonalQuestMapByQuest;
        typedef std::unordered_map<uint32, SeasonalQuestMapByQuest> SeasonalQuestMapByEvent;

        struct LFGRewardInfo
        {
            LFGRewardInfo(uint8 _completionsThisPeriod = 0, bool _isDaily = false) :
                CompletionsThisPeriod(_completionsThisPeriod), IsDaily(_isDaily) { }

            uint8 CompletionsThisPeriod;
            bool IsDaily;
        };

        typedef std::unordered_map<uint32, LFGRewardInfo> LFGRewardStatusMap;

        QuestSet m_timedquests;
        QuestSet m_weeklyquests;
        QuestSet m_monthlyquests;
        SeasonalQuestMapByEvent m_seasonalquests;
        LFGRewardStatusMap m_lfgrewardstatus;

        ObjectGuid m_playerSharingQuest;
        uint32 m_sharedQuestId;
        uint32 m_popupQuestId;
        uint32 m_ingametime;

        /*********************************************************/
        /***                   LOAD SYSTEM                     ***/
        /*********************************************************/

        void _LoadActions(PreparedQueryResult result);
        void _LoadAuras(PreparedQueryResult result, uint32 timediff);
        void _LoadGlyphAuras();
        void _LoadBoundInstances(PreparedQueryResult result);
        void _LoadInventory(PreparedQueryResult result, uint32 timeDiff);
        void _LoadVoidStorage(PreparedQueryResult result);
        void _LoadMail(PreparedQueryResult mailsResult, PreparedQueryResult mailItemsResult);
        static Item* _LoadMailedItem(ObjectGuid const& playerGuid, Player* player, uint32 mailId, Mail* mail, Field* fields);
        void _LoadQuestStatus(PreparedQueryResult result);
        void _LoadQuestStatusRewarded(PreparedQueryResult result);
        void _LoadDailyQuestStatus(PreparedQueryResult result);
        void _LoadWeeklyQuestStatus(PreparedQueryResult result);
        void _LoadMonthlyQuestStatus(PreparedQueryResult result);
        void _LoadSeasonalQuestStatus(PreparedQueryResult result);
        void _LoadRandomBGStatus(PreparedQueryResult result);
        void _LoadGroup(PreparedQueryResult result);
        void _LoadSkills(PreparedQueryResult result);
        void _LoadSpells(PreparedQueryResult result);
        bool _LoadHomeBind(PreparedQueryResult result);
        void _LoadDeclinedNames(PreparedQueryResult result);
        void _LoadArenaTeamInfo(PreparedQueryResult result);
        void _LoadEquipmentSets(PreparedQueryResult result);
        void _LoadBGData(PreparedQueryResult result);
        void _LoadGlyphs(PreparedQueryResult result);
        void _LoadTalents(PreparedQueryResult result);
        void _LoadInstanceTimeRestrictions(PreparedQueryResult result);
        void _LoadCurrency(PreparedQueryResult result);
        void _LoadCUFProfiles(PreparedQueryResult result);
        void _LoadLFGRewardStatus(PreparedQueryResult result);

        /*********************************************************/
        /***                   SAVE SYSTEM                     ***/
        /*********************************************************/

        void _SaveActions(CharacterDatabaseTransaction& trans);
        void _SaveAuras(CharacterDatabaseTransaction& trans);
        void _SaveInventory(CharacterDatabaseTransaction& trans);
        void _SaveVoidStorage(CharacterDatabaseTransaction& trans);
        void _SaveMail(CharacterDatabaseTransaction& trans);
        void _SaveQuestStatus(CharacterDatabaseTransaction& trans);
        void _SaveDailyQuestStatus(CharacterDatabaseTransaction& trans);
        void _SaveWeeklyQuestStatus(CharacterDatabaseTransaction& trans);
        void _SaveMonthlyQuestStatus(CharacterDatabaseTransaction& trans);
        void _SaveSeasonalQuestStatus(CharacterDatabaseTransaction& trans);
        void _SaveSkills(CharacterDatabaseTransaction& trans);
        void _SaveSpells(CharacterDatabaseTransaction& trans);
        void _SaveEquipmentSets(CharacterDatabaseTransaction& trans);
        void _SaveBGData(CharacterDatabaseTransaction& trans);
        void _SaveGlyphs(CharacterDatabaseTransaction& trans) const;
        void _SaveTalents(CharacterDatabaseTransaction& trans);
        void _SaveStats(CharacterDatabaseTransaction& trans) const;
        void _SaveInstanceTimeRestrictions(CharacterDatabaseTransaction& trans);
        void _SaveCurrency(CharacterDatabaseTransaction& trans);
        void _SaveCUFProfiles(CharacterDatabaseTransaction& trans);
        void _SaveLFGRewardStatus(CharacterDatabaseTransaction& trans);

        /*********************************************************/
        /***              ENVIRONMENTAL SYSTEM                 ***/
        /*********************************************************/
        void HandleSobering();
        void SendMirrorTimer(MirrorTimerType Type, uint32 MaxValue, uint32 CurrentValue, int32 Regen);
        void StopMirrorTimer(MirrorTimerType Type);
        void HandleDrowning(uint32 time_diff);
        int32 getMaxTimer(MirrorTimerType timer) const;

        /*********************************************************/
        /***                  HONOR SYSTEM                     ***/
        /*********************************************************/
        time_t m_lastHonorUpdateTime;

        void outDebugValues() const;
        ObjectGuid m_lootGuid;

        uint32 m_team;
        uint32 m_nextSave;
        time_t m_speakTime;
        uint32 m_speakCount;
        Difficulty m_dungeonDifficulty;
        Difficulty m_raidDifficulty;
        Difficulty m_raidMapDifficulty;

        uint32 m_atLoginFlags;

        std::array<Item*, PLAYER_SLOTS_COUNT> m_items;
        uint32 m_currentBuybackSlot;

        PlayerCurrenciesMap _currencyStorage;

        /**
          * @name   GetCurrencyWeekCap
          * @brief  return week cap for selected currency

          * @param  CurrencyTypesEntry for which to retrieve weekly cap
        */
        uint32 GetCurrencyWeekCap(CurrencyTypesEntry const* currency) const;

        /*
         * @name   GetCurrencyTotalCap
         * @brief  return total cap for selected currency

         * @param  CurrencyTypesEntry for which to retrieve total cap
         */
        uint32 GetCurrencyTotalCap(CurrencyTypesEntry const* currency) const;

        /// Updates weekly conquest point cap (dynamic cap)
        void UpdateConquestCurrencyCap(uint32 currency);

        std::array<VoidStorageItem*, VOID_STORAGE_MAX_SLOT> _voidStorageItems;

        std::vector<Item*> m_itemUpdateQueue;
        bool m_itemUpdateQueueBlocked;

        uint32 m_ExtraFlags;

        QuestStatusMap m_QuestStatus;
        QuestStatusSaveMap m_QuestStatusSave;

        RewardedQuestSet m_RewardedQuests;
        QuestStatusSaveMap m_RewardedQuestsSave;

        SkillStatusMap mSkillStatus;

        uint32 m_GuildIdInvited;
        uint32 m_ArenaTeamIdInvited;

        PlayerMails m_mail;
        PlayerSpellMap m_spells;
        uint32 m_lastPotionId;                              // last used health/mana potion in combat, that block next potion use

        std::unique_ptr<PlayerTalentInfo> _talentMgr;

        ActionButtonList m_actionButtons;

        std::array<float, BASEMOD_END> m_auraBaseFlatMod;
        std::array<float, BASEMOD_END> m_auraBasePctMod;
        std::array<int16, MAX_COMBAT_RATING> m_baseRatingValue;
        uint32 m_baseSpellPower;
        uint32 m_baseManaRegen;
        uint32 m_baseHealthRegen;
        int32 m_spellPenetrationItemMod;

        SpellModContainer m_spellMods;

        EnchantDurationList m_enchantDuration;
        ItemDurationList m_itemDuration;
        GuidUnorderedSet m_itemSoulboundTradeable;

        std::unique_ptr<ResurrectionData> _resurrectionData;
        WorldSession* m_session;

        JoinedChannelsList m_channels;

        uint8 m_cinematic;
        uint32 m_movie;

        TradeData* m_trade;

        bool   m_DailyQuestChanged;
        bool   m_WeeklyQuestChanged;
        bool   m_MonthlyQuestChanged;
        bool   m_SeasonalQuestChanged;
        bool   m_LFGRewardStatusChanged;
        time_t m_lastDailyQuestTime;

        uint32 m_hostileReferenceCheckTimer;
        uint32 m_drunkTimer;
        uint32 m_weaponChangeTimer;

        uint32 m_zoneUpdateId;
        uint32 m_areaUpdateId;

        uint32 m_deathTimer;
        time_t m_deathExpireTime;

        uint32 m_WeaponProficiency;
        uint32 m_ArmorProficiency;
        bool m_canParry;
        bool m_canBlock;
        bool m_canTitanGrip;
        uint32 m_titanGripPenaltySpellId;
        uint8 m_swingErrorMsg;

        ////////////////////Rest System/////////////////////
        time_t _restTime;
        uint32 inn_triggerId;
        float m_rest_bonus;
        uint32 _restFlagMask;
        ////////////////////Rest System/////////////////////

        // Social
        PlayerSocial* m_social;

        // Groups
        GroupReference m_group;
        GroupReference m_originalGroup;
        Group* m_groupInvite;
        uint32 m_groupUpdateMask;
        uint64 m_auraRaidUpdateMask;
        bool m_bPassOnGroupLoot;

        // last used pet number (for BG's)
        uint32 m_lastpetnumber;

        // Player summoning
        time_t m_summon_expire;
        WorldLocation m_summon_location;
        uint32 m_summon_instanceId;

        // Recall position
        WorldLocation m_recall_location;
        uint32 m_recall_instanceId;

        std::unique_ptr<DeclinedName> m_declinedname;
        std::unique_ptr<Runes> m_runes;
        EquipmentSetContainer _equipmentSets;

        bool CanAlwaysSee(WorldObject const* obj) const override;

        bool IsAlwaysDetectableFor(WorldObject const* seer) const override;

        uint8 m_grantableLevels;

        uint8 m_fishingSteps;

        bool m_needsZoneUpdate;

        std::array<std::unique_ptr<CUFProfile>, MAX_CUF_PROFILES> _CUFProfiles;

        SpellInfo const* m_lastSoulburnSpell;

    private:
        // internal common parts for CanStore/StoreItem functions
        InventoryResult CanStoreItem_InSpecificSlot(uint8 bag, uint8 slot, ItemPosCountVec& dest, ItemTemplate const* pProto, uint32& count, bool swap, Item* pSrcItem) const;
        InventoryResult CanStoreItem_InBag(uint8 bag, ItemPosCountVec& dest, ItemTemplate const* pProto, uint32& count, bool merge, bool non_specialized, Item* pSrcItem, uint8 skip_bag, uint8 skip_slot) const;
        InventoryResult CanStoreItem_InInventorySlots(uint8 slot_begin, uint8 slot_end, ItemPosCountVec& dest, ItemTemplate const* pProto, uint32& count, bool merge, Item* pSrcItem, uint8 skip_bag, uint8 skip_slot) const;
        Item* _StoreItem(uint16 pos, Item* pItem, uint32 count, bool clone, bool update);
        Item* _LoadItem(CharacterDatabaseTransaction& trans, uint32 zoneId, uint32 timeDiff, Field* fields);

        std::unique_ptr<CinematicMgr> _cinematicMgr;

        GuidSet m_refundableItems;
        void SendRefundInfo(Item* item);
        void RefundItem(Item* item);
        void SendItemRefundResult(Item* item, ItemExtendedCostEntry const* iece, uint8 error);

        // know currencies are not removed at any point (0 displayed)
        void AddKnownCurrency(uint32 itemId);

        void AdjustQuestReqItemCount(Quest const* quest, QuestStatusData& questStatusData);

        bool IsCanDelayTeleport() const { return m_bCanDelayTeleport; }
        void SetCanDelayTeleport(bool setting) { m_bCanDelayTeleport = setting; }
        bool IsHasDelayedTeleport() const { return m_bHasDelayedTeleport; }
        void SetDelayedTeleportFlag(bool setting) { m_bHasDelayedTeleport = setting; }
        void ScheduleDelayedOperation(uint32 operation) { if (operation < DELAYED_END) m_DelayedOperations |= operation; }

        bool IsInstanceLoginGameMasterException() const;

        MapReference m_mapRef;

        uint32 m_lastFallTime;
        float  m_lastFallZ;

        std::array<int32, MAX_TIMERS> m_MirrorTimer;
        uint8 m_MirrorTimerFlags;
        uint8 m_MirrorTimerFlagsLast;

        // Rune type / Rune timer
        std::array<uint32, MAX_RUNES> m_runeGraceCooldown;
        std::array<uint32, MAX_RUNES> m_lastRuneGraceTimers;

        // Current teleport data
        WorldLocation m_teleport_dest;
        Optional<uint32> m_teleport_instanceId;
        TeleportToOptions m_teleport_options;
        bool mSemaphoreTeleport_Near;
        bool mSemaphoreTeleport_Far;

        uint32 m_DelayedOperations;
        bool m_bCanDelayTeleport;
        bool m_bHasDelayedTeleport;

        // Temporary removed pet cache
        uint32 m_temporaryUnsummonedPetNumber;
        uint32 m_oldpetspell;

        std::unique_ptr<AchievementMgr<Player>> m_achievementMgr;
        std::unique_ptr<ReputationMgr> m_reputationMgr;

        uint32 m_ChampioningFaction;

        InstanceTimeMap _instanceResetTimes;
        uint32 _pendingBindId;
        uint32 _pendingBindTimer;

        uint32 _activeCheats;
        uint32 _maxPersonalArenaRate;

        bool _hasValidLFGLeavePoint;

        // variables to save health and mana before duel and restore them after duel
        uint32 healthBeforeDuel;
        uint32 manaBeforeDuel;

        WorldLocation _corpseLocation;

        std::unique_ptr<Archaeology> _archaeology;

        std::vector<PlayerPetData*> PlayerPetDataStore;

        TimeTrackerSmall m_petScalingSynchTimer;
};

TC_GAME_API void AddItemsSetItem(Player* player, Item* item);
TC_GAME_API void RemoveItemsSetItem(Player* player, ItemTemplate const* proto);

#endif
