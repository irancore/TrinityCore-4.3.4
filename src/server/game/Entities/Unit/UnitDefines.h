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

#ifndef UnitDefines_h__
#define UnitDefines_h__

#include "Define.h"
#include <string>

#define BASE_MINDAMAGE 1.0f
#define BASE_MAXDAMAGE 2.0f
#define BASE_ATTACK_TIME 2000

#define MAX_EQUIPMENT_ITEMS 3

// byte value (UNIT_FIELD_BYTES_1, 0)
enum UnitStandStateType : uint8
{
    UNIT_STAND_STATE_STAND             = 0,
    UNIT_STAND_STATE_SIT               = 1,
    UNIT_STAND_STATE_SIT_CHAIR         = 2,
    UNIT_STAND_STATE_SLEEP             = 3,
    UNIT_STAND_STATE_SIT_LOW_CHAIR     = 4,
    UNIT_STAND_STATE_SIT_MEDIUM_CHAIR  = 5,
    UNIT_STAND_STATE_SIT_HIGH_CHAIR    = 6,
    UNIT_STAND_STATE_DEAD              = 7,
    UNIT_STAND_STATE_KNEEL             = 8,
    UNIT_STAND_STATE_SUBMERGED         = 9,

    MAX_UNIT_STAND_STATE
};

// byte flag value (UNIT_FIELD_BYTES_1, 2)
enum UnitStandFlags : uint8
{
    UNIT_STAND_FLAGS_UNK1         = 0x01,
    UNIT_STAND_FLAGS_CREEP        = 0x02,
    UNIT_STAND_FLAGS_UNTRACKABLE  = 0x04,
    UNIT_STAND_FLAGS_UNK4         = 0x08,
    UNIT_STAND_FLAGS_UNK5         = 0x10,
    UNIT_STAND_FLAGS_ALL          = 0xFF
};

enum UnitBytes0Offsets : uint8
{
    UNIT_BYTES_0_OFFSET_RACE        = 0,
    UNIT_BYTES_0_OFFSET_CLASS       = 1,
    UNIT_BYTES_0_OFFSET_GENDER      = 2,
    UNIT_BYTES_0_OFFSET_POWER_TYPE  = 3,
};

enum UnitBytes1Offsets : uint8
{
    UNIT_BYTES_1_OFFSET_STAND_STATE = 0,
    UNIT_BYTES_1_OFFSET_PET_TALENTS = 1,
    UNIT_BYTES_1_OFFSET_VIS_FLAG    = 2,
    UNIT_BYTES_1_OFFSET_ANIM_TIER   = 3
};

enum UnitBytes2Offsets : uint8
{
    UNIT_BYTES_2_OFFSET_SHEATH_STATE    = 0,
    UNIT_BYTES_2_OFFSET_PVP_FLAG        = 1,
    UNIT_BYTES_2_OFFSET_PET_FLAGS       = 2,
    UNIT_BYTES_2_OFFSET_SHAPESHIFT_FORM = 3,
};

// UNIT_FIELD_BYTES_1 (UNIT_BYTES_1_OFFSET_ANIM_TIER)
enum class AnimTier : uint8
{
    Ground      = 0, // plays ground tier animations
    Swim        = 1, // falls back to ground tier animations, not handled by the client, should never appear in sniffs, will prevent tier change animations from playing correctly if used
    Hover       = 2, // plays flying tier animations or falls back to ground tier animations, automatically enables hover clientside when entering visibility with this value
    Fly         = 3, // plays flying tier animations
    Submerged   = 4,

    Max
};

// low byte (0 from 0..3) of UNIT_FIELD_BYTES_2
enum SheathState : uint8
{
    SHEATH_STATE_UNARMED  = 0,                              // non prepared weapon
    SHEATH_STATE_MELEE    = 1,                              // prepared melee weapon
    SHEATH_STATE_RANGED   = 2,                              // prepared ranged weapon

    MAX_SHEATH_STATE
};

// byte (1 from 0..3) of UNIT_FIELD_BYTES_2
enum UnitPVPStateFlags : uint8
{
    UNIT_BYTE2_FLAG_PVP         = 0x01,
    UNIT_BYTE2_FLAG_UNK1        = 0x02,
    UNIT_BYTE2_FLAG_FFA_PVP     = 0x04,
    UNIT_BYTE2_FLAG_SANCTUARY   = 0x08,
    UNIT_BYTE2_FLAG_UNK4        = 0x10,
    UNIT_BYTE2_FLAG_UNK5        = 0x20,
    UNIT_BYTE2_FLAG_UNK6        = 0x40,
    UNIT_BYTE2_FLAG_UNK7        = 0x80
};

// byte (2 from 0..3) of UNIT_FIELD_BYTES_2
enum UnitRename : uint8
{
    UNIT_CAN_BE_RENAMED     = 0x01,
    UNIT_CAN_BE_ABANDONED   = 0x02
};

// Value masks for UNIT_FIELD_FLAGS
enum UnitFlags : uint32
{
    UNIT_FLAG_SERVER_CONTROLLED             = 0x00000001,           // set only when unit movement is controlled by server - by SPLINE/MONSTER_MOVE packets, together with UNIT_FLAG_STUNNED; only set to units controlled by client; client function CGUnit_C::IsClientControlled returns false when set for owner
    UNIT_FLAG_NON_ATTACKABLE                = 0x00000002,           // not attackable
    UNIT_FLAG_REMOVE_CLIENT_CONTROL         = 0x00000004,           // This is a legacy flag used to disable movement player's movement while controlling other units, SMSG_CLIENT_CONTROL replaces this functionality clientside now. CONFUSED and FLEEING flags have the same effect on client movement asDISABLE_MOVE_CONTROL in addition to preventing spell casts/autoattack (they all allow climbing steeper hills and emotes while moving)
    UNIT_FLAG_PLAYER_CONTROLLED             = 0x00000008,           // controlled by player, use _IMMUNE_TO_PC instead of _IMMUNE_TO_NPC
    UNIT_FLAG_RENAME                        = 0x00000010,
    UNIT_FLAG_PREPARATION                   = 0x00000020,           // don't take reagents for spells with SPELL_ATTR5_NO_REAGENT_WHILE_PREP
    UNIT_FLAG_UNK_6                         = 0x00000040,
    UNIT_FLAG_NOT_ATTACKABLE_1              = 0x00000080,           // ?? (UNIT_FLAG_PLAYER_CONTROLLED | UNIT_FLAG_NOT_ATTACKABLE_1) is NON_PVP_ATTACKABLE
    UNIT_FLAG_IMMUNE_TO_PC                  = 0x00000100,           // disables combat/assistance with PlayerCharacters (PC) - see Unit::_IsValidAttackTarget, Unit::IsValidAssistTarget
    UNIT_FLAG_IMMUNE_TO_NPC                 = 0x00000200,           // disables combat/assistance with NonPlayerCharacters (NPC) - see Unit::_IsValidAttackTarget, Unit::IsValidAssistTarget
    UNIT_FLAG_LOOTING                       = 0x00000400,           // loot animation
    UNIT_FLAG_PET_IN_COMBAT                 = 0x00000800,           // on player pets: whether the pet is chasing a target to attack || on other units: whether any of the unit's minions is in combat
    UNIT_FLAG_PVP_ENABLING                  = 0x00001000,           // changed in 3.0.3
    UNIT_FLAG_SILENCED                      = 0x00002000,           // silenced, 2.1.1
    UNIT_FLAG_CANT_SWIM                     = 0x00004000,           // 2.0.8
    UNIT_FLAG_CAN_SWIM                      = 0x00008000,           // shows swim animation in water
    UNIT_FLAG_NON_ATTACKABLE_2              = 0x00010000,
    UNIT_FLAG_PACIFIED                      = 0x00020000,           // 3.0.3 ok
    UNIT_FLAG_STUNNED                       = 0x00040000,           // 3.0.3 ok
    UNIT_FLAG_IN_COMBAT                     = 0x00080000,
    UNIT_FLAG_ON_TAXI                       = 0x00100000,           // disable casting at client side spell not allowed by taxi flight (mounted?), probably used with 0x4 flag
    UNIT_FLAG_DISARMED                      = 0x00200000,           // 3.0.3, disable melee spells casting..., "Required melee weapon" added to melee spells tooltip.
    UNIT_FLAG_CONFUSED                      = 0x00400000,
    UNIT_FLAG_FLEEING                       = 0x00800000,
    UNIT_FLAG_POSSESSED                     = 0x01000000,           // under direct client control by a player (possess or vehicle)
    UNIT_FLAG_NOT_SELECTABLE                = 0x02000000,
    UNIT_FLAG_SKINNABLE                     = 0x04000000,
    UNIT_FLAG_MOUNT                         = 0x08000000,
    UNIT_FLAG_UNK_28                        = 0x10000000,
    UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT = 0x20000000,
    UNIT_FLAG_SHEATHE                       = 0x40000000,
    UNIT_FLAG_IMMUNE                        = 0x80000000,           // Immune to damage
    MAX_UNIT_FLAGS = 33,

    UNIT_FLAG_DISALLOWED                    = (UNIT_FLAG_SERVER_CONTROLLED | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_REMOVE_CLIENT_CONTROL |
                                               UNIT_FLAG_PLAYER_CONTROLLED | UNIT_FLAG_RENAME | UNIT_FLAG_PREPARATION | /* UNIT_FLAG_UNK_6 | */
                                               UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_LOOTING | UNIT_FLAG_PET_IN_COMBAT | UNIT_FLAG_PVP_ENABLING | UNIT_FLAG_SILENCED |
                                               UNIT_FLAG_CANT_SWIM | UNIT_FLAG_CAN_SWIM | UNIT_FLAG_NON_ATTACKABLE_2 | UNIT_FLAG_PACIFIED | UNIT_FLAG_STUNNED |
                                               UNIT_FLAG_IN_COMBAT | UNIT_FLAG_ON_TAXI | UNIT_FLAG_DISARMED | UNIT_FLAG_CONFUSED | UNIT_FLAG_FLEEING |
                                               UNIT_FLAG_POSSESSED | UNIT_FLAG_SKINNABLE | UNIT_FLAG_MOUNT | UNIT_FLAG_UNK_28 |
                                               UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT | UNIT_FLAG_SHEATHE | UNIT_FLAG_IMMUNE), // SKIP

    UNIT_FLAG_ALLOWED = (0xFFFFFFFF & ~UNIT_FLAG_DISALLOWED) // SKIP
};

// Value masks for UNIT_FIELD_FLAGS_2
enum UnitFlags2 : uint32
{
    UNIT_FLAG2_FEIGN_DEATH                                      = 0x00000001,
    UNIT_FLAG2_HIDE_BODY                                        = 0x00000002,   // TITLE Hide Body DESCRIPTION Hide unit model (show only player equip)
    UNIT_FLAG2_IGNORE_REPUTATION                                = 0x00000004,
    UNIT_FLAG2_COMPREHEND_LANG                                  = 0x00000008,
    UNIT_FLAG2_MIRROR_IMAGE                                     = 0x00000010,
    UNIT_FLAG2_DONT_FADE_IN                                     = 0x00000020,   // TITLE Don't Fade In DESCRIPTION Unit model instantly appears when summoned (does not fade in)
    UNIT_FLAG2_FORCE_MOVEMENT                                   = 0x00000040,
    UNIT_FLAG2_DISARM_OFFHAND                                   = 0x00000080,
    UNIT_FLAG2_DISABLE_PRED_STATS                               = 0x00000100,   // Player has disabled predicted stats (Used by raid frames)
    UNIT_FLAG2_ALLOW_CHANGING_TALENTS                           = 0x00000200,   // Allows changing talents outside rest area
    UNIT_FLAG2_DISARM_RANGED                                    = 0x00000400,   // this does not disable ranged weapon display (maybe additional flag needed?)
    UNIT_FLAG2_REGENERATE_POWER                                 = 0x00000800,
    UNIT_FLAG2_RESTRICT_PARTY_INTERACTION                       = 0x00001000,   // Restrict interaction to party or raid
    UNIT_FLAG2_PREVENT_SPELL_CLICK                              = 0x00002000,   // Prevent spellclick
    UNIT_FLAG2_INTERACT_WHILE_HOSTILE                           = 0x00004000,   // TITLE Interact while Hostile
    UNIT_FLAG2_CANNOT_TURN                                      = 0x00008000,   // TITLE Cannot Turn
    UNIT_FLAG2_UNK2                                             = 0x00010000,
    UNIT_FLAG2_PLAY_DEATH_ANIM                                  = 0x00020000,   // Plays special death animation upon death
    UNIT_FLAG2_ALLOW_CHEAT_SPELLS                               = 0x00040000,   // Allows casting spells with AttributesEx7 & SPELL_ATTR7_IS_CHEAT_SPELL
    UNIT_FLAG2_SUPPRESS_HIGHLIGHT_WHEN_TARGETED_OR_MOUSED_OVER  = 0x00080000,   // TITLE Suppress highlight when targeted or moused over
    UNIT_FLAG2_TREAT_AS_RAID_UNIT_FOR_HELPFUL_SPELLS            = 0x00100000,   // TITLE Treat as Raid Unit For Helpful Spells (Instances ONLY)
    UNIT_FLAG2_LARGE_AOI                                        = 0x00200000,   // TITLE Large (AOI)
    UNIT_FLAG2_GIGANTIC_AOI                                     = 0x00400000,   // TITLE Gigantic (AOI)
    UNIT_FLAG2_NO_ACTIONS                                       = 0x00800000,
    UNIT_FLAG2_AI_WILL_ONLY_SWIM_IF_TARGET_SWIMS                = 0x01000000,   // TITLE AI will only swim if target swims
    UNIT_FLAG2_DONT_GENERATE_COMBAT_LOG_WHEN_ENGAGED_WITH_NPCS  = 0x02000000,   // TITLE Don't generate combat log when engaged with NPC's
    UNIT_FLAG2_UNTARGETABLE_BY_CLIENT                           = 0x04000000,   // TITLE Untargetable By Client
    UNIT_FLAG2_ATTACKER_IGNORES_MINIMUM_RANGES                  = 0x08000000,   // TITLE Attacker Ignores Minimum Ranges
    UNIT_FLAG2_UNINTERACTIBLE_IF_HOSTILE                        = 0x10000000,   // TITLE Uninteractible If Hostile
    UNIT_FLAG2_UNUSED_11                                        = 0x20000000,
    UNIT_FLAG2_INFINITE_AOI                                     = 0x40000000,   // TITLE Infinite (AOI)
    UNIT_FLAG2_UNUSED_13                                        = 0x80000000,
    MAX_UNIT_FLAGS2 = 33,

    UNIT_FLAG2_DISALLOWED                                       = (/* UNIT_FLAG2_FEIGN_DEATH | */ UNIT_FLAG2_IGNORE_REPUTATION | UNIT_FLAG2_COMPREHEND_LANG |
                                                                   UNIT_FLAG2_MIRROR_IMAGE | UNIT_FLAG2_FORCE_MOVEMENT | UNIT_FLAG2_DISARM_OFFHAND |
                                                                   UNIT_FLAG2_DISABLE_PRED_STATS | UNIT_FLAG2_ALLOW_CHANGING_TALENTS | UNIT_FLAG2_DISARM_RANGED |
                                                                /* UNIT_FLAG2_REGENERATE_POWER | */ UNIT_FLAG2_RESTRICT_PARTY_INTERACTION | UNIT_FLAG2_CANNOT_TURN |
                                                                   UNIT_FLAG2_PREVENT_SPELL_CLICK | /* UNIT_FLAG2_INTERACT_WHILE_HOSTILE | */ /* UNIT_FLAG2_UNK2 | */
                                                                /* UNIT_FLAG2_PLAY_DEATH_ANIM | */ UNIT_FLAG2_ALLOW_CHEAT_SPELLS | UNIT_FLAG2_SUPPRESS_HIGHLIGHT_WHEN_TARGETED_OR_MOUSED_OVER |
                                                                   UNIT_FLAG2_TREAT_AS_RAID_UNIT_FOR_HELPFUL_SPELLS | UNIT_FLAG2_LARGE_AOI | UNIT_FLAG2_GIGANTIC_AOI | UNIT_FLAG2_NO_ACTIONS |
                                                                   UNIT_FLAG2_AI_WILL_ONLY_SWIM_IF_TARGET_SWIMS | UNIT_FLAG2_DONT_GENERATE_COMBAT_LOG_WHEN_ENGAGED_WITH_NPCS | UNIT_FLAG2_ATTACKER_IGNORES_MINIMUM_RANGES |
                                                                   UNIT_FLAG2_UNINTERACTIBLE_IF_HOSTILE | UNIT_FLAG2_UNUSED_11 | UNIT_FLAG2_INFINITE_AOI | UNIT_FLAG2_UNUSED_13),  // SKIP

    UNIT_FLAG2_ALLOWED                                          = (0xFFFFFFFF & ~UNIT_FLAG2_DISALLOWED) // SKIP
};

/// Non Player Character flags
enum NPCFlags : uint32
{
    UNIT_NPC_FLAG_NONE                  = 0x00000000,
    UNIT_NPC_FLAG_GOSSIP                = 0x00000001,       // 100%
    UNIT_NPC_FLAG_QUESTGIVER            = 0x00000002,       // 100%
    UNIT_NPC_FLAG_UNK1                  = 0x00000004,
    UNIT_NPC_FLAG_UNK2                  = 0x00000008,
    UNIT_NPC_FLAG_TRAINER               = 0x00000010,       // 100%
    UNIT_NPC_FLAG_TRAINER_CLASS         = 0x00000020,       // 100%
    UNIT_NPC_FLAG_TRAINER_PROFESSION    = 0x00000040,       // 100%
    UNIT_NPC_FLAG_VENDOR                = 0x00000080,       // 100%
    UNIT_NPC_FLAG_VENDOR_AMMO           = 0x00000100,       // 100%, general goods vendor
    UNIT_NPC_FLAG_VENDOR_FOOD           = 0x00000200,       // 100%
    UNIT_NPC_FLAG_VENDOR_POISON         = 0x00000400,       // guessed
    UNIT_NPC_FLAG_VENDOR_REAGENT        = 0x00000800,       // 100%
    UNIT_NPC_FLAG_REPAIR                = 0x00001000,       // 100%
    UNIT_NPC_FLAG_FLIGHTMASTER          = 0x00002000,       // 100%
    UNIT_NPC_FLAG_SPIRITHEALER          = 0x00004000,       // guessed
    UNIT_NPC_FLAG_SPIRITGUIDE           = 0x00008000,       // guessed
    UNIT_NPC_FLAG_INNKEEPER             = 0x00010000,       // 100%
    UNIT_NPC_FLAG_BANKER                = 0x00020000,       // 100%
    UNIT_NPC_FLAG_PETITIONER            = 0x00040000,       // 100% 0xC0000 = guild petitions, 0x40000 = arena team petitions
    UNIT_NPC_FLAG_TABARDDESIGNER        = 0x00080000,       // 100%
    UNIT_NPC_FLAG_BATTLEMASTER          = 0x00100000,       // 100%
    UNIT_NPC_FLAG_AUCTIONEER            = 0x00200000,       // 100%
    UNIT_NPC_FLAG_STABLEMASTER          = 0x00400000,       // 100%
    UNIT_NPC_FLAG_GUILD_BANKER          = 0x00800000,       // cause client to send 997 opcode
    UNIT_NPC_FLAG_SPELLCLICK            = 0x01000000,       // cause client to send 1015 opcode (spell click)
    UNIT_NPC_FLAG_PLAYER_VEHICLE        = 0x02000000,       // players with mounts that have vehicle data should have it set
    UNIT_NPC_FLAG_MAILBOX               = 0x04000000,       // mailbox
    UNIT_NPC_FLAG_REFORGER              = 0x08000000,       // reforging
    UNIT_NPC_FLAG_TRANSMOGRIFIER        = 0x10000000,       // transmogrification
    UNIT_NPC_FLAG_VAULTKEEPER           = 0x20000000        // void storage
};

enum MovementFlags : uint32
{
    MOVEMENTFLAG_NONE                  = 0x00000000,
    MOVEMENTFLAG_FORWARD               = 0x00000001,
    MOVEMENTFLAG_BACKWARD              = 0x00000002,
    MOVEMENTFLAG_STRAFE_LEFT           = 0x00000004,
    MOVEMENTFLAG_STRAFE_RIGHT          = 0x00000008,
    MOVEMENTFLAG_LEFT                  = 0x00000010,
    MOVEMENTFLAG_RIGHT                 = 0x00000020,
    MOVEMENTFLAG_PITCH_UP              = 0x00000040,
    MOVEMENTFLAG_PITCH_DOWN            = 0x00000080,
    MOVEMENTFLAG_WALKING               = 0x00000100,               // Walking
    MOVEMENTFLAG_DISABLE_GRAVITY       = 0x00000200,               // Former MOVEMENTFLAG_LEVITATING. This is used when walking is not possible.
    MOVEMENTFLAG_ROOT                  = 0x00000400,               // Must not be set along with MOVEMENTFLAG_MASK_MOVING
    MOVEMENTFLAG_FALLING               = 0x00000800,               // damage dealt on that type of falling
    MOVEMENTFLAG_FALLING_FAR           = 0x00001000,
    MOVEMENTFLAG_PENDING_STOP          = 0x00002000,
    MOVEMENTFLAG_PENDING_STRAFE_STOP   = 0x00004000,
    MOVEMENTFLAG_PENDING_FORWARD       = 0x00008000,
    MOVEMENTFLAG_PENDING_BACKWARD      = 0x00010000,
    MOVEMENTFLAG_PENDING_STRAFE_LEFT   = 0x00020000,
    MOVEMENTFLAG_PENDING_STRAFE_RIGHT  = 0x00040000,
    MOVEMENTFLAG_PENDING_ROOT          = 0x00080000,
    MOVEMENTFLAG_SWIMMING              = 0x00100000,               // appears with fly flag also
    MOVEMENTFLAG_ASCENDING             = 0x00200000,               // press "space" when flying or swimming
    MOVEMENTFLAG_DESCENDING            = 0x00400000,
    MOVEMENTFLAG_CAN_FLY               = 0x00800000,               // Appears when unit can fly AND also walk
    MOVEMENTFLAG_FLYING                = 0x01000000,               // unit is actually flying. pretty sure this is only used for players. creatures use disable_gravity
    MOVEMENTFLAG_SPLINE_ELEVATION      = 0x02000000,               // used for flight paths
    MOVEMENTFLAG_WATERWALKING          = 0x04000000,               // prevent unit from falling through water
    MOVEMENTFLAG_FALLING_SLOW          = 0x08000000,               // active rogue safe fall spell (passive)
    MOVEMENTFLAG_HOVER                 = 0x10000000,               // hover, cannot jump
    MOVEMENTFLAG_DISABLE_COLLISION     = 0x20000000,

    MOVEMENTFLAG_MASK_MOVING =
        MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT |
        MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING |
        MOVEMENTFLAG_SPLINE_ELEVATION,

    MOVEMENTFLAG_MASK_TURNING =
        MOVEMENTFLAG_LEFT | MOVEMENTFLAG_RIGHT | MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN,

    MOVEMENTFLAG_MASK_MOVING_FLY =
        MOVEMENTFLAG_FLYING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING,

    // Movement flags allowed for creature in CreateObject - we need to keep all other enabled serverside
    // to properly calculate all movement
    MOVEMENTFLAG_MASK_CREATURE_ALLOWED =
        MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_DISABLE_GRAVITY | MOVEMENTFLAG_ROOT | MOVEMENTFLAG_SWIMMING |
        MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_WATERWALKING | MOVEMENTFLAG_FALLING_SLOW | MOVEMENTFLAG_HOVER,

    /// @todo if needed: add more flags to this masks that are exclusive to players
    MOVEMENTFLAG_MASK_PLAYER_ONLY =
        MOVEMENTFLAG_FLYING,

    /// Movement flags that have change status opcodes associated for players
    MOVEMENTFLAG_MASK_HAS_PLAYER_STATUS_OPCODE = MOVEMENTFLAG_DISABLE_GRAVITY | MOVEMENTFLAG_ROOT |
        MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_WATERWALKING | MOVEMENTFLAG_FALLING_SLOW | MOVEMENTFLAG_HOVER
};

enum MovementFlags2 : uint32
{
    MOVEMENTFLAG2_NONE                      = 0x00000000,
    MOVEMENTFLAG2_NO_STRAFE                 = 0x00000001,
    MOVEMENTFLAG2_NO_JUMPING                = 0x00000002,
    MOVEMENTFLAG2_FULL_SPEED_TURNING        = 0x00000004,
    MOVEMENTFLAG2_FULL_SPEED_PITCHING       = 0x00000008,
    MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING     = 0x00000010,
    MOVEMENTFLAG2_IS_VEHICLE_EXIT_VOLUNTARY = 0x00000020,
    MOVEMENTFLAG2_UNK6                      = 0x00000040,
    MOVEMENTFLAG2_UNK7                      = 0x00000080,
    MOVEMENTFLAG2_UNK8                      = 0x00000100,
    MOVEMENTFLAG2_UNK9                      = 0x00000200,
    MOVEMENTFLAG2_CAN_SWIM_TO_FLY_TRANS     = 0x00000400,
    MOVEMENTFLAG2_UNK11                     = 0x00000800,
    MOVEMENTFLAG2_AWAITING_LOAD             = 0x00001000,
    MOVEMENTFLAG2_INTERPOLATED_MOVEMENT     = 0x00002000,
    MOVEMENTFLAG2_INTERPOLATED_TURNING      = 0x00004000,
    MOVEMENTFLAG2_INTERPOLATED_PITCHING     = 0x00008000
};

enum UnitMoveType
{
    MOVE_WALK           = 0,
    MOVE_RUN            = 1,
    MOVE_RUN_BACK       = 2,
    MOVE_SWIM           = 3,
    MOVE_SWIM_BACK      = 4,
    MOVE_TURN_RATE      = 5,
    MOVE_FLIGHT         = 6,
    MOVE_FLIGHT_BACK    = 7,
    MOVE_PITCH_RATE     = 8
};

#define MAX_MOVE_TYPE     9

enum HitInfo
{
    HITINFO_NORMALSWING         = 0x00000000,
    HITINFO_UNK1                = 0x00000001,               // req correct packet structure
    HITINFO_AFFECTS_VICTIM      = 0x00000002,
    HITINFO_OFFHAND             = 0x00000004,
    HITINFO_UNK2                = 0x00000008,
    HITINFO_MISS                = 0x00000010,
    HITINFO_FULL_ABSORB         = 0x00000020,
    HITINFO_PARTIAL_ABSORB      = 0x00000040,
    HITINFO_FULL_RESIST         = 0x00000080,
    HITINFO_PARTIAL_RESIST      = 0x00000100,
    HITINFO_CRITICALHIT         = 0x00000200,               // critical hit
    HITINFO_UNK10               = 0x00000400,
    HITINFO_UNK11               = 0x00000800,
    HITINFO_UNK12               = 0x00001000,
    HITINFO_BLOCK               = 0x00002000,               // blocked damage
    HITINFO_UNK14               = 0x00004000,               // set only if meleespellid is present//  no world text when victim is hit for 0 dmg(HideWorldTextForNoDamage?)
    HITINFO_UNK15               = 0x00008000,               // player victim?// something related to blod sprut visual (BloodSpurtInBack?)
    HITINFO_GLANCING            = 0x00010000,
    HITINFO_CRUSHING            = 0x00020000,
    HITINFO_NO_ANIMATION        = 0x00040000,
    HITINFO_UNK19               = 0x00080000,
    HITINFO_UNK20               = 0x00100000,
    HITINFO_SWINGNOHITSOUND     = 0x00200000,               // unused?
    HITINFO_UNK22               = 0x00400000,
    HITINFO_RAGE_GAIN           = 0x00800000,
    HITINFO_FAKE_DAMAGE         = 0x01000000                // enables damage animation even if no damage done, set only if no damage
};

#define MAX_DECLINED_NAME_CASES 5

struct DeclinedName
{
    std::string name[MAX_DECLINED_NAME_CASES];
};

enum ActiveStates
{
    ACT_PASSIVE  = 0x01,                                    // 0x01 - passive
    ACT_DISABLED = 0x81,                                    // 0x80 - castable
    ACT_ENABLED  = 0xC1,                                    // 0x40 | 0x80 - auto cast + castable
    ACT_COMMAND  = 0x07,                                    // 0x01 | 0x02 | 0x04
    ACT_REACTION = 0x06,                                    // 0x02 | 0x04
    ACT_DECIDE   = 0x00                                     // custom
};

enum ReactStates
{
    REACT_PASSIVE    = 0,
    REACT_DEFENSIVE  = 1,
    REACT_AGGRESSIVE = 2,
    REACT_ASSIST     = 3
};

enum CommandStates : uint8
{
    COMMAND_STAY    = 0,
    COMMAND_FOLLOW  = 1,
    COMMAND_ATTACK  = 2,
    COMMAND_ABANDON = 3,
    COMMAND_MOVE_TO = 4
};

#endif // UnitDefines_h__
