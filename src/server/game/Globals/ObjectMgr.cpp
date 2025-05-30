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

#include "ObjectMgr.h"
#include "AccountMgr.h"
#include "AchievementMgr.h"
#include "ArenaTeamMgr.h"
#include "Bag.h"
#include "Chat.h"
#include "Containers.h"
#include "CreatureAIFactory.h"
#include "DatabaseEnv.h"
#include "DB2Structure.h"
#include "DB2Stores.h"
#include "DisableMgr.h"
#include "GameTime.h"
#include "GameObject.h"
#include "GameObjectAIFactory.h"
#include "GossipDef.h"
#include "GroupMgr.h"
#include "GuildMgr.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "Language.h"
#include "LFGMgr.h"
#include "Log.h"
#include "LootMgr.h"
#include "Mail.h"
#include "MapManager.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "PoolMgr.h"
#include "QuestDef.h"
#include "QueryPackets.h"
#include "Random.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "TerrainMgr.h"
#include "ThreadPool.h"
#include "UpdateMask.h"
#include "Util.h"
#include "Vehicle.h"
#include "World.h"
#include <G3D/g3dmath.h>

ScriptMapMap sSpellScripts;
ScriptMapMap sEventScripts;
ScriptMapMap sWaypointScripts;

std::string GetScriptsTableNameByType(ScriptsType type)
{
    std::string res = "";
    switch (type)
    {
        case SCRIPTS_SPELL:         res = "spell_scripts";      break;
        case SCRIPTS_EVENT:         res = "event_scripts";      break;
        case SCRIPTS_WAYPOINT:      res = "waypoint_scripts";   break;
        default: break;
    }
    return res;
}

ScriptMapMap* GetScriptsMapByType(ScriptsType type)
{
    ScriptMapMap* res = nullptr;
    switch (type)
    {
        case SCRIPTS_SPELL:         res = &sSpellScripts;       break;
        case SCRIPTS_EVENT:         res = &sEventScripts;       break;
        case SCRIPTS_WAYPOINT:      res = &sWaypointScripts;    break;
        default: break;
    }
    return res;
}

std::string GetScriptCommandName(ScriptCommands command)
{
    std::string res = "";
    switch (command)
    {
        case SCRIPT_COMMAND_TALK: res = "SCRIPT_COMMAND_TALK"; break;
        case SCRIPT_COMMAND_EMOTE: res = "SCRIPT_COMMAND_EMOTE"; break;
        case SCRIPT_COMMAND_FIELD_SET: res = "SCRIPT_COMMAND_FIELD_SET"; break;
        case SCRIPT_COMMAND_MOVE_TO: res = "SCRIPT_COMMAND_MOVE_TO"; break;
        case SCRIPT_COMMAND_FLAG_SET: res = "SCRIPT_COMMAND_FLAG_SET"; break;
        case SCRIPT_COMMAND_FLAG_REMOVE: res = "SCRIPT_COMMAND_FLAG_REMOVE"; break;
        case SCRIPT_COMMAND_TELEPORT_TO: res = "SCRIPT_COMMAND_TELEPORT_TO"; break;
        case SCRIPT_COMMAND_QUEST_EXPLORED: res = "SCRIPT_COMMAND_QUEST_EXPLORED"; break;
        case SCRIPT_COMMAND_KILL_CREDIT: res = "SCRIPT_COMMAND_KILL_CREDIT"; break;
        case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT: res = "SCRIPT_COMMAND_RESPAWN_GAMEOBJECT"; break;
        case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE: res = "SCRIPT_COMMAND_TEMP_SUMMON_CREATURE"; break;
        case SCRIPT_COMMAND_OPEN_DOOR: res = "SCRIPT_COMMAND_OPEN_DOOR"; break;
        case SCRIPT_COMMAND_CLOSE_DOOR: res = "SCRIPT_COMMAND_CLOSE_DOOR"; break;
        case SCRIPT_COMMAND_ACTIVATE_OBJECT: res = "SCRIPT_COMMAND_ACTIVATE_OBJECT"; break;
        case SCRIPT_COMMAND_REMOVE_AURA: res = "SCRIPT_COMMAND_REMOVE_AURA"; break;
        case SCRIPT_COMMAND_CAST_SPELL: res = "SCRIPT_COMMAND_CAST_SPELL"; break;
        case SCRIPT_COMMAND_PLAY_SOUND: res = "SCRIPT_COMMAND_PLAY_SOUND"; break;
        case SCRIPT_COMMAND_CREATE_ITEM: res = "SCRIPT_COMMAND_CREATE_ITEM"; break;
        case SCRIPT_COMMAND_DESPAWN_SELF: res = "SCRIPT_COMMAND_DESPAWN_SELF"; break;
        case SCRIPT_COMMAND_LOAD_PATH: res = "SCRIPT_COMMAND_LOAD_PATH"; break;
        case SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT: res = "SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT"; break;
        case SCRIPT_COMMAND_KILL: res = "SCRIPT_COMMAND_KILL"; break;
        // TrinityCore only
        case SCRIPT_COMMAND_ORIENTATION: res = "SCRIPT_COMMAND_ORIENTATION"; break;
        case SCRIPT_COMMAND_EQUIP: res = "SCRIPT_COMMAND_EQUIP"; break;
        case SCRIPT_COMMAND_MODEL: res = "SCRIPT_COMMAND_MODEL"; break;
        case SCRIPT_COMMAND_CLOSE_GOSSIP: res = "SCRIPT_COMMAND_CLOSE_GOSSIP"; break;
        case SCRIPT_COMMAND_PLAYMOVIE: res = "SCRIPT_COMMAND_PLAYMOVIE"; break;
        case SCRIPT_COMMAND_MOVEMENT: res = "SCRIPT_COMMAND_MOVEMENT"; break;
        case SCRIPT_COMMAND_PLAY_ANIMKIT: res = "SCRIPT_COMMAND_PLAY_ANIMKIT"; break;
        default:
        {
            char sz[32];
            sprintf(sz, "Unknown command: %d", command);
            res = sz;
            break;
        }
    }
    return res;
}

std::string ScriptInfo::GetDebugInfo() const
{
    char sz[256];
    sprintf(sz, "%s ('%s' script id: %u)", GetScriptCommandName(command).c_str(), GetScriptsTableNameByType(type).c_str(), id);
    return std::string(sz);
}

bool normalizePlayerName(std::string& name)
{
    if (name.empty())
        return false;

    std::wstring tmp;
    if (!Utf8toWStr(name, tmp))
        return false;

    wstrToLower(tmp);
    if (!tmp.empty())
        tmp[0] = wcharToUpper(tmp[0]);

    if (!WStrToUtf8(tmp, name))
        return false;

    return true;
}

LanguageDesc lang_description[LANGUAGES_COUNT] =
{
    { LANG_ADDON,           0, 0                       },
    { LANG_UNIVERSAL,       0, 0                       },
    { LANG_ORCISH,        669, SKILL_LANG_ORCISH       },
    { LANG_DARNASSIAN,    671, SKILL_LANG_DARNASSIAN   },
    { LANG_TAURAHE,       670, SKILL_LANG_TAURAHE      },
    { LANG_DWARVISH,      672, SKILL_LANG_DWARVEN      },
    { LANG_COMMON,        668, SKILL_LANG_COMMON       },
    { LANG_DEMONIC,       815, SKILL_LANG_DEMON_TONGUE },
    { LANG_TITAN,         816, SKILL_LANG_TITAN        },
    { LANG_THALASSIAN,    813, SKILL_LANG_THALASSIAN   },
    { LANG_DRACONIC,      814, SKILL_LANG_DRACONIC     },
    { LANG_KALIMAG,       817, SKILL_LANG_OLD_TONGUE   },
    { LANG_GNOMISH,      7340, SKILL_LANG_GNOMISH      },
    { LANG_TROLL,        7341, SKILL_LANG_TROLL        },
    { LANG_GUTTERSPEAK, 17737, SKILL_LANG_GUTTERSPEAK  },
    { LANG_DRAENEI,     29932, SKILL_LANG_DRAENEI      },
    { LANG_ZOMBIE,          0, 0                       },
    { LANG_GNOMISH_BINARY,  0, 0                       },
    { LANG_GOBLIN_BINARY,   0, 0                       },
    { LANG_WORGEN,      69270, SKILL_LANG_WORGEN       },
    { LANG_GOBLIN,      69269, SKILL_LANG_GOBLIN       }
};

LanguageDesc const* GetLanguageDescByID(uint32 lang)
{
    for (uint8 i = 0; i < LANGUAGES_COUNT; ++i)
    {
        if (uint32(lang_description[i].lang_id) == lang)
            return &lang_description[i];
    }

    return nullptr;
}

bool SpellClickInfo::IsFitToRequirements(Unit const* clicker, Unit const* clickee) const
{
    Player const* playerClicker = clicker->ToPlayer();
    if (!playerClicker)
        return true;

    Unit const* summoner = nullptr;
    // Check summoners for party
    if (clickee->IsSummon())
        summoner = clickee->ToTempSummon()->GetSummoner();
    if (!summoner)
        summoner = clickee;

    // This only applies to players
    switch (userType)
    {
        case SPELL_CLICK_USER_FRIEND:
            if (!playerClicker->IsFriendlyTo(summoner))
                return false;
            break;
        case SPELL_CLICK_USER_RAID:
            if (!playerClicker->IsInRaidWith(summoner))
                return false;
            break;
        case SPELL_CLICK_USER_PARTY:
            if (!playerClicker->IsInPartyWith(summoner))
                return false;
            break;
        default:
            break;
    }

    return true;
}

ObjectMgr::ObjectMgr():
    _auctionId(1),
    _equipmentSetGuid(1),
    _mailId(1),
    _hiPetNumber(1),
    _voidItemId(1),
    _creatureSpawnId(1),
    _gameObjectSpawnId(1),
    DBCLocaleIndex(LOCALE_enUS)
{
    for (uint8 i = 0; i < MAX_CLASSES; ++i)
        for (uint8 j = 0; j < MAX_RACES; ++j)
            _playerInfo[j][i] = nullptr;
}

ObjectMgr* ObjectMgr::instance()
{
    static ObjectMgr instance;
    return &instance;
}

ObjectMgr::~ObjectMgr()
{
    for (PetLevelInfoContainer::iterator i = _petInfoStore.begin(); i != _petInfoStore.end(); ++i)
        delete[] i->second;

    for (int race = 0; race < MAX_RACES; ++race)
    {
        for (int class_ = 0; class_ < MAX_CLASSES; ++class_)
        {
            if (_playerInfo[race][class_])
                delete[] _playerInfo[race][class_]->levelInfo;
            delete _playerInfo[race][class_];
        }
    }

    for (CacheVendorItemContainer::iterator itr = _cacheVendorItemStore.begin(); itr != _cacheVendorItemStore.end(); ++itr)
        itr->second.Clear();

    _creatureDefaultTrainers.clear();
    _graveyardOrientations.clear();

    for (DungeonEncounterContainer::iterator itr =_dungeonEncounterStore.begin(); itr != _dungeonEncounterStore.end(); ++itr)
        for (DungeonEncounterList::iterator encounterItr = itr->second.begin(); encounterItr != itr->second.end(); ++encounterItr)
            delete *encounterItr;

    for (AccessRequirementContainer::iterator itr = _accessRequirementStore.begin(); itr != _accessRequirementStore.end(); ++itr)
        delete itr->second;
}

void ObjectMgr::AddLocaleString(std::string&& value, LocaleConstant localeConstant, std::vector<std::string>& data)
{
    if (!value.empty())
    {
        if (data.size() <= size_t(localeConstant))
            data.resize(localeConstant + 1);

        data[localeConstant] = std::move(value);
    }
}

void ObjectMgr::LoadGraveyardOrientations()
{
    uint32 oldMSTime = getMSTime();

    _graveyardOrientations.clear();

    QueryResult result = WorldDatabase.Query("SELECT id, orientation FROM graveyard_orientation");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].GetUInt32();
        if (!sWorldSafeLocsStore.LookupEntry(id))
        {
            TC_LOG_ERROR("server.loading", "Graveyard %u referenced in graveyard_orientation doesn't exist.", id);
            continue;
        }
        _graveyardOrientations[id] = fields[1].GetFloat();

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %lu graveyard orientations in %u ms", (unsigned long)_graveyardOrientations.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureLocales()
{
    uint32 oldMSTime = getMSTime();

    _creatureLocaleStore.clear(); // need for reload case

    //                                               0      1       2     3           4
    QueryResult result = WorldDatabase.Query("SELECT entry, locale, Name, FemaleName, Title FROM creature_template_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 id               = fields[0].GetUInt32();
        std::string localeName  = fields[1].GetString();

        LocaleConstant locale = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        CreatureLocale& data = _creatureLocaleStore[id];
        AddLocaleString(fields[2].GetString(), locale, data.Name);
        AddLocaleString(fields[3].GetString(), locale, data.FemaleName);
        AddLocaleString(fields[4].GetString(), locale, data.Title);

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature locale strings in %u ms", uint32(_creatureLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadGossipMenuItemsLocales()
{
    uint32 oldMSTime = getMSTime();

    _gossipMenuItemsLocaleStore.clear();                              // need for reload case

    //                                               0       1            2       3           4
    QueryResult result = WorldDatabase.Query("SELECT MenuID, OptionID, Locale, OptionText, BoxText FROM gossip_menu_option_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 menuId           = fields[0].GetUInt32();
        uint32 optionId         = fields[1].GetUInt32();
        std::string localeName  = fields[2].GetString();

        LocaleConstant locale = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        GossipMenuItemsLocale& data = _gossipMenuItemsLocaleStore[std::make_pair(menuId, optionId)];
        AddLocaleString(fields[3].GetString(), locale, data.OptionText);
        AddLocaleString(fields[4].GetString(), locale, data.BoxText);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " gossip_menu_option locale strings in %u ms", _gossipMenuItemsLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadPointOfInterestLocales()
{
    uint32 oldMSTime = getMSTime();

    _pointOfInterestLocaleStore.clear();                              // need for reload case

    //                                               0   1       2
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Name FROM points_of_interest_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 id               = fields[0].GetUInt32();
        std::string localeName  = fields[1].GetString();

        LocaleConstant locale = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        PointOfInterestLocale& data = _pointOfInterestLocaleStore[id];
        AddLocaleString(fields[2].GetString(), locale, data.Name);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u points_of_interest locale strings in %u ms", uint32(_pointOfInterestLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureTemplates()
{
    uint32 oldMSTime = getMSTime();

    //                                               0      1                   2                   3                   4            5            6         7         8
    QueryResult result = WorldDatabase.Query("SELECT entry, difficulty_entry_1, difficulty_entry_2, difficulty_entry_3, KillCredit1, KillCredit2, modelid1, modelid2, modelid3, "
    //                                        9         10    11          12       13        14              15        16        17   18       19       20       21          22
                                             "modelid4, name, femaleName, subname, IconName, gossip_menu_id, minlevel, maxlevel, exp, exp_unk, faction, npcflag, speed_walk, speed_run, "
    //                                        23      24     25         26              27               28            29             30          31          32
                                             "scale, `rank`, dmgschool, BaseAttackTime, RangeAttackTime, BaseVariance, RangeVariance, unit_class, unit_flags, unit_flags2, "
    //                                        33      34             35     36         37           38      39              40        41           42           43           44           45           46
                                             "family, trainer_class, type, type_flags, type_flags2, lootid, pickpocketloot, skinloot, resistance1, resistance2, resistance3, resistance4, resistance5, resistance6, "
    //                                        47      48      49      50      51      52      53      54      55              56         57       58       59      60
                                             "spell1, spell2, spell3, spell4, spell5, spell6, spell7, spell8, PetSpellDataId, VehicleId, mingold, maxgold, AIName, MovementType, "
    //                                        61                         62          63                         64           65              66                   67            68
                                             "ctm.HoverInitiallyEnabled, ctm.Random, ctm.InteractionPauseTimer, HoverHeight, HealthModifier, HealthModifierExtra, ManaModifier, ManaModifierExtra, "
    //                                        69             70              71                  72            73          74           75                    76
                                             "ArmorModifier, DamageModifier, ExperienceModifier, RacialLeader, movementId, RegenHealth, mechanic_immune_mask, spell_school_immune_mask, "
    //                                        77           78           79            80            81            82            83
                                             "flags_extra, StaticFlags, StaticFlags2, StaticFlags3, StaticFlags4, StaticFlags5, ScriptName FROM creature_template ct LEFT JOIN creature_template_movement ctm ON ct.entry = ctm.CreatureId");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature template definitions. DB table `creature_template` is empty.");
        return;
    }

    _creatureTemplateStore.reserve(result->GetRowCount());
    do
    {
        Field* fields = result->Fetch();
        LoadCreatureTemplate(fields);
    }
    while (result->NextRow());

    // We load the creature models after loading but before checking
    LoadCreatureTemplateModels();

    // Checking needs to be done after loading because of the difficulty self referencing
    for (CreatureTemplateContainer::const_iterator itr = _creatureTemplateStore.begin(); itr != _creatureTemplateStore.end(); ++itr)
        CheckCreatureTemplate(&itr->second);

    TC_LOG_INFO("server.loading", ">> Loaded %u creature definitions in %u ms", uint32(_creatureTemplateStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureTemplate(Field* fields)
{
    uint32 entry = fields[0].GetUInt32();

    CreatureTemplate& creatureTemplate = _creatureTemplateStore[entry];

    creatureTemplate.Entry = entry;

    for (uint8 i = 0; i < MAX_DIFFICULTY - 1; ++i)
        creatureTemplate.DifficultyEntry[i] = fields[1 + i].GetUInt32();

    for (uint8 i = 0; i < MAX_KILL_CREDIT; ++i)
        creatureTemplate.KillCredit[i] = fields[4 + i].GetUInt32();

    //creatureTemplate.Modelid1          = fields[6].GetUInt32();
    //creatureTemplate.Modelid2          = fields[7].GetUInt32();
    //creatureTemplate.Modelid3          = fields[8].GetUInt32();
    //creatureTemplate.Modelid4          = fields[9].GetUInt32();
    creatureTemplate.Name              = fields[10].GetString();
    creatureTemplate.FemaleName        = fields[11].GetString();
    creatureTemplate.Title             = fields[12].GetString();
    creatureTemplate.IconName          = fields[13].GetString();
    creatureTemplate.GossipMenuId      = fields[14].GetUInt32();
    creatureTemplate.minlevel          = fields[15].GetUInt8();
    creatureTemplate.maxlevel          = fields[16].GetUInt8();
    creatureTemplate.expansion         = uint32(fields[17].GetInt16());
    creatureTemplate.expansionUnknown  = uint32(fields[18].GetUInt16());
    creatureTemplate.faction           = uint32(fields[19].GetUInt16());
    creatureTemplate.npcflag           = fields[20].GetUInt32();
    creatureTemplate.speed_walk        = fields[21].GetFloat();
    creatureTemplate.speed_run         = fields[22].GetFloat();
    creatureTemplate.scale             = fields[23].GetFloat();
    creatureTemplate.rank              = uint32(fields[24].GetUInt8());
    creatureTemplate.dmgschool         = uint32(fields[25].GetInt8());
    creatureTemplate.BaseAttackTime    = fields[26].GetUInt32();
    creatureTemplate.RangeAttackTime   = fields[27].GetUInt32();
    creatureTemplate.BaseVariance      = fields[28].GetFloat();
    creatureTemplate.RangeVariance     = fields[29].GetFloat();
    creatureTemplate.unit_class        = uint32(fields[30].GetUInt8());
    creatureTemplate.unit_flags        = fields[31].GetUInt32();
    creatureTemplate.unit_flags2       = fields[32].GetUInt32();
    creatureTemplate.family            = CreatureFamily(uint32(fields[33].GetUInt8()));
    creatureTemplate.trainer_class      = fields[34].GetUInt32();
    creatureTemplate.type              = uint32(fields[35].GetUInt8());
    creatureTemplate.type_flags        = fields[36].GetUInt32();
    creatureTemplate.type_flags2       = fields[37].GetUInt32();
    creatureTemplate.lootid            = fields[38].GetUInt32();
    creatureTemplate.pickpocketLootId  = fields[39].GetUInt32();
    creatureTemplate.SkinLootId        = fields[40].GetUInt32();

    for (uint8 i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
        creatureTemplate.resistance[i] = fields[41 + i - 1].GetInt16();

    for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
        creatureTemplate.spells[i] = fields[47 + i].GetUInt32();

    creatureTemplate.PetSpellDataId = fields[55].GetUInt32();
    creatureTemplate.VehicleId      = fields[56].GetUInt32();
    creatureTemplate.mingold        = fields[57].GetUInt32();
    creatureTemplate.maxgold        = fields[58].GetUInt32();
    creatureTemplate.AIName         = fields[59].GetString();
    creatureTemplate.MovementType   = uint32(fields[60].GetUInt8());
    if (!fields[61].IsNull())
        creatureTemplate.Movement.HoverInitiallyEnabled = fields[61].GetBool();

    if (!fields[62].IsNull())
        creatureTemplate.Movement.Random = static_cast<CreatureRandomMovementType>(fields[62].GetUInt8());

    if (!fields[63].IsNull())
        creatureTemplate.Movement.InteractionPauseTimer = fields[63].GetUInt32();

    creatureTemplate.HoverHeight    = fields[64].GetFloat();
    creatureTemplate.ModHealth      = fields[65].GetFloat();
    creatureTemplate.ModHealthExtra = fields[66].GetFloat();
    creatureTemplate.ModMana        = fields[67].GetFloat();
    creatureTemplate.ModManaExtra   = fields[68].GetFloat();
    creatureTemplate.ModArmor       = fields[69].GetFloat();
    creatureTemplate.ModDamage      = fields[70].GetFloat();
    creatureTemplate.ModExperience  = fields[71].GetFloat();

    creatureTemplate.RacialLeader          = fields[72].GetBool();
    creatureTemplate.movementId            = fields[73].GetUInt32();
    creatureTemplate.RegenHealth           = fields[74].GetBool();
    creatureTemplate.MechanicImmuneMask    = fields[75].GetUInt32();
    creatureTemplate.SpellSchoolImmuneMask = fields[76].GetUInt32();
    creatureTemplate.flags_extra           = fields[77].GetUInt32();
    creatureTemplate.StaticFlags = CreatureStaticFlagsHolder(CreatureStaticFlags(fields[78].GetUInt32()), CreatureStaticFlags2(fields[79].GetUInt32()),
        CreatureStaticFlags3(fields[80].GetUInt32()), CreatureStaticFlags4(fields[81].GetUInt32()), CreatureStaticFlags5(fields[82].GetUInt32()));
    creatureTemplate.ScriptID              = GetScriptId(fields[83].GetCString());
}

void ObjectMgr::LoadCreatureTemplateModels()
{
    uint32 oldMSTime = getMSTime();

    //                                               0           1    2                  3
    QueryResult result = WorldDatabase.Query("SELECT CreatureID, Idx, CreatureDisplayID, Probability FROM creature_template_model ORDER BY Idx ASC");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature template model definitions. DB table `creature_template_model` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 creatureId = fields[0].GetUInt32();
        uint32 idx = fields[1].GetUInt32();
        uint32 creatureDisplayId = fields[2].GetUInt32();
        float probability = fields[3].GetFloat();

        CreatureTemplate const* cInfo = GetCreatureTemplate(creatureId);
        if (!cInfo)
        {
            TC_LOG_ERROR("sql.sql", "Creature template (Entry: %u) does not exist but has a record in `creature_template_model`", creatureId);
            continue;
        }

        if (idx >= MAX_CREATURE_MODELS)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) lists CreatureDisplayID id (%u) at Idx (%u), max supported Idx is 3. Skipped!.", creatureId, creatureDisplayId, idx);
            continue;
        }

        CreatureDisplayInfoEntry const* displayEntry = sCreatureDisplayInfoStore.LookupEntry(creatureDisplayId);
        if (!displayEntry)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) lists non-existing CreatureDisplayID id (%u), this can crash the client.", creatureId, creatureDisplayId);
            continue;
        }

        CreatureModelInfo const* modelInfo = GetCreatureModelInfo(creatureDisplayId);
        if (!modelInfo)
            TC_LOG_ERROR("sql.sql", "No model data exist for `CreatureDisplayID` = %u listed by creature (Entry: %u).", creatureDisplayId, creatureId);

        const_cast<CreatureTemplate*>(cInfo)->Models.emplace_back(creatureDisplayId, idx, probability);

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature template models in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureTemplateAddons()
{
    uint32 oldMSTime = getMSTime();

    //                                               0      1               2                   3      4           5         6         7            8         9      10         11               12            13                      14
    QueryResult result = WorldDatabase.Query("SELECT entry, waypointPathId, cyclicSplinePathId, mount, StandState, AnimTier, VisFlags, SheathState, PvPFlags, emote, aiAnimKit, movementAnimKit, meleeAnimKit, visibilityDistanceType, auras FROM creature_template_addon");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature template addon definitions. DB table `creature_template_addon` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        if (!sObjectMgr->GetCreatureTemplate(entry))
        {
            TC_LOG_ERROR("sql.sql", "Creature template (Entry: %u) does not exist but has a record in `creature_template_addon`", entry);
            continue;
        }

        CreatureAddon& creatureAddon = _creatureTemplateAddonStore[entry];

        creatureAddon.waypointPathId            = fields[1].GetUInt32();
        creatureAddon.cyclicSplinePathId        = fields[2].GetUInt32();
        creatureAddon.mount                     = fields[3].GetUInt32();
        creatureAddon.standState                = fields[4].GetUInt8();
        creatureAddon.animTier                  = fields[5].GetUInt8();
        creatureAddon.visFlags                  = fields[6].GetUInt8();
        creatureAddon.sheathState               = fields[7].GetUInt8();
        creatureAddon.pvpFlags                  = fields[8].GetUInt8();
        creatureAddon.emote                     = fields[9].GetUInt32();
        creatureAddon.aiAnimKit                 = fields[10].GetUInt16();
        creatureAddon.movementAnimKit           = fields[11].GetUInt16();
        creatureAddon.meleeAnimKit              = fields[12].GetUInt16();
        creatureAddon.visibilityDistanceType = VisibilityDistanceType(fields[13].GetUInt8());

        Tokenizer tokens(fields[14].GetString(), ' ');
        uint8 i = 0;
        creatureAddon.auras.resize(tokens.size());
        for (Tokenizer::const_iterator itr = tokens.begin(); itr != tokens.end(); ++itr)
        {
            SpellInfo const* AdditionalSpellInfo = sSpellMgr->GetSpellInfo(atoul(*itr));
            if (!AdditionalSpellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has wrong spell %lu defined in `auras` field in `creature_template_addon`.", entry, atoul(*itr));
                continue;
            }

            if (AdditionalSpellInfo->HasAura(SPELL_AURA_CONTROL_VEHICLE))
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has SPELL_AURA_CONTROL_VEHICLE aura %lu defined in `auras` field in `creature_template_addon`.", entry, atoul(*itr));

            if (std::find(creatureAddon.auras.begin(), creatureAddon.auras.end(), atoul(*itr)) != creatureAddon.auras.end())
            {
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has duplicate aura (spell %lu) in `auras` field in `creature_template_addon`.", entry, atoul(*itr));
                continue;
            }

            creatureAddon.auras[i++] = atoul(*itr);
        }

        if (creatureAddon.mount)
        {
            if (!sCreatureDisplayInfoStore.LookupEntry(creatureAddon.mount))
            {
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid displayInfoId (%u) for mount defined in `creature_template_addon`", entry, creatureAddon.mount);
                creatureAddon.mount = 0;
            }
        }

        if (creatureAddon.standState >= MAX_UNIT_STAND_STATE)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid unit stand state (%u) defined in `creature_template_addon`. Truncated to 0.", entry, creatureAddon.standState);
            creatureAddon.standState = 0;
        }

        if (AnimTier(creatureAddon.animTier) >= AnimTier::Max)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid animation tier (%u) defined in `creature_template_addon`. Truncated to 0.", entry, creatureAddon.animTier);
            creatureAddon.animTier = 0;
        }

        if (creatureAddon.sheathState >= MAX_SHEATH_STATE)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid sheath state (%u) defined in `creature_template_addon`. Truncated to 0.", entry, creatureAddon.sheathState);
            creatureAddon.sheathState = 0;
        }

        // PvPFlags don't need any checking for the time being since they cover the entire range of a byte

        if (!sEmotesStore.LookupEntry(creatureAddon.emote))
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid emote (%u) defined in `creature_template_addon`.", entry, creatureAddon.emote);
            creatureAddon.emote = 0;
        }

        if (creatureAddon.aiAnimKit && !sAnimKitStore.LookupEntry(creatureAddon.aiAnimKit))
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid aiAnimKit (%u) defined in `creature_template_addon`.", entry, creatureAddon.aiAnimKit);
            creatureAddon.aiAnimKit = 0;
        }

        if (creatureAddon.movementAnimKit && !sAnimKitStore.LookupEntry(creatureAddon.movementAnimKit))
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid movementAnimKit (%u) defined in `creature_template_addon`.", entry, creatureAddon.movementAnimKit);
            creatureAddon.movementAnimKit = 0;
        }

        if (creatureAddon.meleeAnimKit && !sAnimKitStore.LookupEntry(creatureAddon.meleeAnimKit))
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid meleeAnimKit (%u) defined in `creature_template_addon`.", entry, creatureAddon.meleeAnimKit);
            creatureAddon.meleeAnimKit = 0;
        }

        if (creatureAddon.visibilityDistanceType >= VisibilityDistanceType::Max)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid visibilityDistanceType (%u) defined in `creature_template_addon`.",
                entry, AsUnderlyingType(creatureAddon.visibilityDistanceType));
            creatureAddon.visibilityDistanceType = VisibilityDistanceType::Normal;
        }

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature template addons in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureSparringTemplate()
{
    uint32 oldMSTime = getMSTime();

    //                                               0           1
    QueryResult result = WorldDatabase.Query("SELECT CreatureID, HealthLimitPct FROM creature_sparring_template");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature template sparring definitions. DB table `creature_sparring_template` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();
        float healthPct = fields[1].GetFloat();

        if (!sObjectMgr->GetCreatureTemplate(entry))
        {
            TC_LOG_ERROR("sql.sql", "Creature template (Entry: %u) does not exist but has a record in `creature_sparring_template`", entry);
            continue;
        }

        if (healthPct > 100.0f)
        {
            TC_LOG_ERROR("sql.sql", "Sparring entry (Entry: %u) exceeds the health percentage limit. Setting to 100.", entry);
            healthPct = 100.0f;
        }

        if (healthPct <= 0.0f)
        {
            TC_LOG_ERROR("sql.sql", "Sparring entry (Entry: %u) has a negative or too small health percentage. Setting to 0.1.", entry);
            healthPct = 0.1f;
        }

        _creatureSparringTemplateStore[entry] = healthPct;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature sparring templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::CheckCreatureTemplate(CreatureTemplate const* cInfo)
{
    if (!cInfo)
        return;

    bool ok = true;                                     // bool to allow continue outside this loop
    for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && ok; ++diff)
    {
        if (!cInfo->DifficultyEntry[diff])
            continue;
        ok = false;                                     // will be set to true at the end of this loop again

        CreatureTemplate const* difficultyInfo = GetCreatureTemplate(cInfo->DifficultyEntry[diff]);
        if (!difficultyInfo)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has `difficulty_entry_%u`=%u but creature entry %u does not exist.",
                cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff], cInfo->DifficultyEntry[diff]);
            continue;
        }

        bool ok2 = true;
        for (uint32 diff2 = 0; diff2 < MAX_DIFFICULTY - 1 && ok2; ++diff2)
        {
            ok2 = false;
            if (_difficultyEntries[diff2].find(cInfo->Entry) != _difficultyEntries[diff2].end())
            {
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) is listed as `difficulty_entry_%u` of another creature, but itself lists %u in `difficulty_entry_%u`.",
                    cInfo->Entry, diff2 + 1, cInfo->DifficultyEntry[diff], diff + 1);
                continue;
            }

            if (_difficultyEntries[diff2].find(cInfo->DifficultyEntry[diff]) != _difficultyEntries[diff2].end())
            {
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) already listed as `difficulty_entry_%u` for another entry.", cInfo->DifficultyEntry[diff], diff2 + 1);
                continue;
            }

            if (_hasDifficultyEntries[diff2].find(cInfo->DifficultyEntry[diff]) != _hasDifficultyEntries[diff2].end())
            {
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has `difficulty_entry_%u`=%u but creature entry %u has itself a value in `difficulty_entry_%u`.",
                    cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff], cInfo->DifficultyEntry[diff], diff2 + 1);
                continue;
            }
            ok2 = true;
        }

        if (!ok2)
            continue;

        if (cInfo->expansion > difficultyInfo->expansion)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, exp: %u) has different `exp` in difficulty %u mode (Entry: %u, exp: %u).",
                cInfo->Entry, cInfo->expansion, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->expansion);
        }

        if (cInfo->minlevel > difficultyInfo->minlevel)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, minlevel: %u) has lower `minlevel` in difficulty %u mode (Entry: %u, minlevel: %u).",
                cInfo->Entry, cInfo->minlevel, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->minlevel);
        }

        if (cInfo->maxlevel > difficultyInfo->maxlevel)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, maxlevel: %u) has lower `maxlevel` in difficulty %u mode (Entry: %u, maxlevel: %u).",
                cInfo->Entry, cInfo->maxlevel, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->maxlevel);
        }

        if (cInfo->faction != difficultyInfo->faction)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, faction: %u) has different `faction` in difficulty %u mode (Entry: %u, faction: %u).",
                cInfo->Entry, cInfo->faction, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->faction);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `faction`=%u WHERE `entry`=%u;",
                cInfo->faction, cInfo->DifficultyEntry[diff]);
        }

        if (cInfo->unit_class != difficultyInfo->unit_class)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, class: %u) has different `unit_class` in difficulty %u mode (Entry: %u, class: %u).",
                cInfo->Entry, cInfo->unit_class, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->unit_class);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `unit_class`=%u WHERE `entry`=%u;",
                cInfo->unit_class, cInfo->DifficultyEntry[diff]);
            continue;
        }

        uint32 differenceMask = cInfo->npcflag ^ difficultyInfo->npcflag;
        if (cInfo->npcflag != difficultyInfo->npcflag)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, `npcflag`: %u) has different `npcflag` in difficulty %u mode (Entry: %u, `npcflag`: %u).",
                cInfo->Entry, cInfo->npcflag, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->npcflag);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `npcflag`=`npcflag`^%u WHERE `entry`=%u;",
                differenceMask, cInfo->DifficultyEntry[diff]);
        }

        if (cInfo->dmgschool != difficultyInfo->dmgschool)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, `dmgschool`: %u) has different `dmgschool` in difficulty %u mode (Entry: %u, `dmgschool`: %u).",
                cInfo->Entry, cInfo->dmgschool, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->dmgschool);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `dmgschool`=%u WHERE `entry`=%u;",
                cInfo->dmgschool, cInfo->DifficultyEntry[diff]);
        }

        differenceMask = cInfo->unit_flags2 ^ difficultyInfo->unit_flags2;
        if (cInfo->unit_flags2 != difficultyInfo->unit_flags2)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, `unit_flags2`: %u) has different `unit_flags2` in difficulty %u mode (Entry: %u, `unit_flags2`: %u).",
                cInfo->Entry, cInfo->unit_flags2, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->unit_flags2);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `unit_flags2`=`unit_flags2`^%u WHERE `entry`=%u;",
                differenceMask, cInfo->DifficultyEntry[diff]);
        }

        if (cInfo->family != difficultyInfo->family)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, family: %u) has different `family` in difficulty %u mode (Entry: %u, family: %u).",
                cInfo->Entry, cInfo->family, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->family);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `family`=%u WHERE `entry`=%u;",
                cInfo->family, cInfo->DifficultyEntry[diff]);
        }

        if (cInfo->type != difficultyInfo->type)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, type: %u) has different `type` in difficulty %u mode (Entry: %u, type: %u).",
                cInfo->Entry, cInfo->type, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->type);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `type`=%u WHERE `entry`=%u;",
                cInfo->type, cInfo->DifficultyEntry[diff]);
        }

        if (!cInfo->VehicleId && difficultyInfo->VehicleId)
        {
            TC_LOG_ERROR("sql.sql", "Non-vehicle Creature (Entry: %u, VehicleId: %u) has `VehicleId` set in difficulty %u mode (Entry: %u, VehicleId: %u).",
                cInfo->Entry, cInfo->VehicleId, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->VehicleId);
        }

        if (cInfo->RegenHealth != difficultyInfo->RegenHealth)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, RegenHealth: %u) has different `RegenHealth` in difficulty %u mode (Entry: %u, RegenHealth: %u).",
                cInfo->Entry, cInfo->RegenHealth, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->RegenHealth);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `RegenHealth`=%u WHERE `entry`=%u;",
                cInfo->RegenHealth, cInfo->DifficultyEntry[diff]);
        }

        differenceMask = cInfo->MechanicImmuneMask & (~difficultyInfo->MechanicImmuneMask);
        if (differenceMask)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, mechanic_immune_mask: %u) has weaker immunities in difficulty %u mode (Entry: %u, mechanic_immune_mask: %u).",
                cInfo->Entry, cInfo->MechanicImmuneMask, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->MechanicImmuneMask);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `mechanic_immune_mask`=`mechanic_immune_mask`|%u WHERE `entry`=%u;",
                differenceMask, cInfo->DifficultyEntry[diff]);
        }

        differenceMask = (cInfo->flags_extra ^ difficultyInfo->flags_extra) & (~CREATURE_FLAG_EXTRA_INSTANCE_BIND);
        if (differenceMask)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, flags_extra: %u) has different `flags_extra` in difficulty %u mode (Entry: %u, flags_extra: %u).",
                cInfo->Entry, cInfo->flags_extra, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->flags_extra);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `flags_extra`=`flags_extra`^%u WHERE `entry`=%u;",
                differenceMask, cInfo->DifficultyEntry[diff]);
        }

        if (!difficultyInfo->AIName.empty())
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) lists difficulty %u mode entry %u with `AIName` filled in. `AIName` of difficulty 0 mode creature is always used instead.",
                cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff]);
            continue;
        }

        if (difficultyInfo->ScriptID)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) lists difficulty %u mode entry %u with `ScriptName` filled in. `ScriptName` of difficulty 0 mode creature is always used instead.",
                cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff]);
            continue;
        }

        _hasDifficultyEntries[diff].insert(cInfo->Entry);
        _difficultyEntries[diff].insert(cInfo->DifficultyEntry[diff]);
        ok = true;
    }

    if (cInfo->mingold > cInfo->maxgold)
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has `mingold` %u which is greater than `maxgold` %u, setting `maxgold` to %u.",
            cInfo->Entry, cInfo->mingold, cInfo->maxgold, cInfo->mingold);
        const_cast<CreatureTemplate*>(cInfo)->maxgold = cInfo->mingold;
    }

    if (!cInfo->AIName.empty())
    {
        auto registryItem = sCreatureAIRegistry->GetRegistryItem(cInfo->AIName);
        if (!registryItem)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has non-registered `AIName` '%s' set, removing", cInfo->Entry, cInfo->AIName.c_str());
            const_cast<CreatureTemplate*>(cInfo)->AIName.clear();
        }
        else
        {
            DBPermit const* permit = dynamic_cast<DBPermit const*>(registryItem);
            if (!ASSERT_NOTNULL(permit)->IsScriptNameAllowedInDB())
            {
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has not-allowed `AIName` '%s' set, removing", cInfo->Entry, cInfo->AIName.c_str());
                const_cast<CreatureTemplate*>(cInfo)->AIName.clear();
            }
        }
    }

    FactionTemplateEntry const* factionTemplate = sFactionTemplateStore.LookupEntry(cInfo->faction);
    if (!factionTemplate)
    {
        TC_LOG_FATAL("sql.sql", "Creature (Entry: %u) has non-existing faction template (%u). This can lead to crashes, aborting.", cInfo->Entry, cInfo->faction);
        ABORT();
    }

    if (cInfo->Models.empty())
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) does not have any existing display id in creature_template_model.", cInfo->Entry);

    for (uint8 k = 0; k < MAX_KILL_CREDIT; ++k)
    {
        if (cInfo->KillCredit[k])
        {
            if (!GetCreatureTemplate(cInfo->KillCredit[k]))
            {
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) lists non-existing creature entry %u in `KillCredit%d`.", cInfo->Entry, cInfo->KillCredit[k], k + 1);
                const_cast<CreatureTemplate*>(cInfo)->KillCredit[k] = 0;
            }
        }
    }

    if (!cInfo->unit_class || ((1 << (cInfo->unit_class-1)) & CLASSMASK_ALL_CREATURES) == 0)
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid unit_class (%u) in creature_template. Set to 1 (UNIT_CLASS_WARRIOR).", cInfo->Entry, cInfo->unit_class);
        const_cast<CreatureTemplate*>(cInfo)->unit_class = UNIT_CLASS_WARRIOR;
    }

    if (cInfo->dmgschool >= MAX_SPELL_SCHOOL)
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid spell school value (%u) in `dmgschool`.", cInfo->Entry, cInfo->dmgschool);
        const_cast<CreatureTemplate*>(cInfo)->dmgschool = SPELL_SCHOOL_NORMAL;
    }

    if (cInfo->BaseAttackTime == 0)
        const_cast<CreatureTemplate*>(cInfo)->BaseAttackTime  = BASE_ATTACK_TIME;

    if (cInfo->RangeAttackTime == 0)
        const_cast<CreatureTemplate*>(cInfo)->RangeAttackTime = BASE_ATTACK_TIME;

    if (cInfo->speed_walk == 0.0f)
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has wrong value (%f) in speed_walk, set to 1.", cInfo->Entry, cInfo->speed_walk);
        const_cast<CreatureTemplate*>(cInfo)->speed_walk = 1.0f;
    }

    if (cInfo->speed_run == 0.0f)
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has wrong value (%f) in speed_run, set to 1.14286.", cInfo->Entry, cInfo->speed_run);
        const_cast<CreatureTemplate*>(cInfo)->speed_run = 1.14286f;
    }

    if (cInfo->type && !sCreatureTypeStore.LookupEntry(cInfo->type))
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid creature type (%u) in `type`.", cInfo->Entry, cInfo->type);
        const_cast<CreatureTemplate*>(cInfo)->type = CREATURE_TYPE_HUMANOID;
    }

    // must exist or used hidden but used in data horse case
    if (cInfo->family && !sCreatureFamilyStore.LookupEntry(cInfo->family) && cInfo->family != CREATURE_FAMILY_HORSE_CUSTOM)
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid creature family (%u) in `family`.", cInfo->Entry, cInfo->family);
        const_cast<CreatureTemplate*>(cInfo)->family = CREATURE_FAMILY_NONE;
    }

    CheckCreatureMovement("creature_template_movement", cInfo->Entry, const_cast<CreatureTemplate*>(cInfo)->Movement);

    if (cInfo->HoverHeight < 0.0f)
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has wrong value (%f) in `HoverHeight`", cInfo->Entry, cInfo->HoverHeight);
        const_cast<CreatureTemplate*>(cInfo)->HoverHeight = 1.0f;
    }

    if (cInfo->VehicleId)
    {
        VehicleEntry const* vehId = sVehicleStore.LookupEntry(cInfo->VehicleId);
        if (!vehId)
        {
             TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has a non-existing VehicleId (%u). This *WILL* cause the client to freeze!", cInfo->Entry, cInfo->VehicleId);
             const_cast<CreatureTemplate*>(cInfo)->VehicleId = 0;
        }
    }

    if (cInfo->PetSpellDataId)
    {
        CreatureSpellDataEntry const* spellDataId = sCreatureSpellDataStore.LookupEntry(cInfo->PetSpellDataId);
        if (!spellDataId)
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has non-existing PetSpellDataId (%u).", cInfo->Entry, cInfo->PetSpellDataId);
    }

    for (uint8 j = 0; j < MAX_CREATURE_SPELLS; ++j)
    {
        if (cInfo->spells[j] && !sSpellMgr->GetSpellInfo(cInfo->spells[j]))
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has non-existing Spell%d (%u), set to 0.", cInfo->Entry, j+1, cInfo->spells[j]);
            const_cast<CreatureTemplate*>(cInfo)->spells[j] = 0;
        }
    }

    if (cInfo->MovementType >= MAX_DB_MOTION_TYPE)
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has wrong movement generator type (%u), ignored and set to IDLE.", cInfo->Entry, cInfo->MovementType);
        const_cast<CreatureTemplate*>(cInfo)->MovementType = IDLE_MOTION_TYPE;
    }

    if (cInfo->expansion > (MAX_EXPANSIONS - 1))
    {
        TC_LOG_ERROR("sql.sql", "Table `creature_template` lists creature (Entry: %u) with expansion %u. Ignored and set to 0.", cInfo->Entry, cInfo->expansion);
        const_cast<CreatureTemplate*>(cInfo)->expansion = 0;
    }

    if (cInfo->expansionUnknown > MAX_EXPANSIONS)
    {
        TC_LOG_ERROR("sql.sql", "Table `creature_template` lists creature (Entry: %u) with `exp_unk` %u. Ignored and set to 0.", cInfo->Entry, cInfo->expansionUnknown);
        const_cast<CreatureTemplate*>(cInfo)->expansionUnknown = 0;
    }

    if (uint32 badFlags = (cInfo->flags_extra & ~CREATURE_FLAG_EXTRA_DB_ALLOWED))
    {
        TC_LOG_ERROR("sql.sql", "Table `creature_template` lists creature (Entry: %u) with disallowed `flags_extra` %u, removing incorrect flag.", cInfo->Entry, badFlags);
        const_cast<CreatureTemplate*>(cInfo)->flags_extra &= CREATURE_FLAG_EXTRA_DB_ALLOWED;
    }

    if (uint32 disallowedUnitFlags = (cInfo->unit_flags & ~UNIT_FLAG_ALLOWED))
    {
        TC_LOG_ERROR("sql.sql", "Table `creature_template` lists creature (Entry: %u) with disallowed `unit_flags` %u, removing incorrect flag.", cInfo->Entry, disallowedUnitFlags);
        const_cast<CreatureTemplate*>(cInfo)->unit_flags &= UNIT_FLAG_ALLOWED;
    }

    if (uint32 disallowedUnitFlags2 = (cInfo->unit_flags2 & ~UNIT_FLAG2_ALLOWED))
    {
        TC_LOG_ERROR("sql.sql", "Table `creature_template` lists creature (Entry: %u) with disallowed `unit_flags2` %u, removing incorrect flag.", cInfo->Entry, disallowedUnitFlags2);
        const_cast<CreatureTemplate*>(cInfo)->unit_flags2 &= UNIT_FLAG2_ALLOWED;
    }

    const_cast<CreatureTemplate*>(cInfo)->ModDamage *= Creature::_GetDamageMod(cInfo->rank);
}

void ObjectMgr::CheckCreatureMovement(char const* table, uint64 id, CreatureMovementData& creatureMovement)
{
    if (creatureMovement.Random >= CreatureRandomMovementType::Max)
    {
        TC_LOG_ERROR("sql.sql", "`%s`.`Random` wrong value (%u) for Id " UI64FMTD ", setting to Walk.",
            table, uint32(creatureMovement.Random), id);
        creatureMovement.Random = CreatureRandomMovementType::Walk;
    }
}

void ObjectMgr::LoadCreatureAddons()
{
    uint32 oldMSTime = getMSTime();

    //                                               0     1               2                   3      4           5         6         7            8         9      10         11               12            13                      14
    QueryResult result = WorldDatabase.Query("SELECT guid, waypointPathId, cyclicSplinePathId, mount, StandState, AnimTier, VisFlags, SheathState, PvPFlags, emote, aiAnimKit, movementAnimKit, meleeAnimKit, visibilityDistanceType, auras FROM creature_addon");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature addon definitions. DB table `creature_addon` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType guid = fields[0].GetUInt32();

        CreatureData const* creData = GetCreatureData(guid);
        if (!creData)
        {
            TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) does not exist but has a record in `creature_addon`", guid);
            continue;
        }

        CreatureAddon& creatureAddon = _creatureAddonStore[guid];

        creatureAddon.waypointPathId        = fields[1].GetUInt32();
        creatureAddon.cyclicSplinePathId    = fields[2].GetUInt32();

        if (creatureAddon.waypointPathId && creatureAddon.cyclicSplinePathId)
        {
            if (creData->movementType == WAYPOINT_MOTION_TYPE)
            {
                creatureAddon.cyclicSplinePathId = 0;
                TC_LOG_ERROR("sql.sql", "Creature (GUID %u) has a waypoint and cyclic spline path at the same time but uses WAYPOINT_MOTION_TYPE. Prefered waypoint path", guid);
            }
            else if (creData->movementType == CYCLIC_SPLINE_MOTION_TYPE)
            {
                creatureAddon.waypointPathId = 0;
                TC_LOG_ERROR("sql.sql", "Creature (GUID %u) has a waypoint and cyclic spline path at the same time but uses CYCLIC_SPLINE_MOTION_TYPE. Prefered cyclic spline path", guid);
            }
        }

        if (creData->movementType == WAYPOINT_MOTION_TYPE && !creatureAddon.waypointPathId)
        {
            const_cast<CreatureData*>(creData)->movementType = IDLE_MOTION_TYPE;
            TC_LOG_ERROR("sql.sql", "Creature (GUID %u) has movement type set to WAYPOINT_MOTION_TYPE but no path assigned", guid);
        }

        if (creData->movementType == CYCLIC_SPLINE_MOTION_TYPE && !creatureAddon.cyclicSplinePathId)
        {
            const_cast<CreatureData*>(creData)->movementType = IDLE_MOTION_TYPE;
            TC_LOG_ERROR("sql.sql", "Creature (GUID %u) has movement type set to CYCLIC_SPLINE_MOTION_TYPE but no path assigned", guid);
        }

        creatureAddon.mount                     = fields[3].GetUInt32();
        creatureAddon.standState                = fields[4].GetUInt8();
        creatureAddon.animTier                  = fields[5].GetUInt8();
        creatureAddon.visFlags                  = fields[6].GetUInt8();
        creatureAddon.sheathState               = fields[7].GetUInt8();
        creatureAddon.pvpFlags                  = fields[8].GetUInt8();
        creatureAddon.emote                     = fields[9].GetUInt32();
        creatureAddon.aiAnimKit                 = fields[10].GetUInt16();
        creatureAddon.movementAnimKit           = fields[11].GetUInt16();
        creatureAddon.meleeAnimKit              = fields[12].GetUInt16();
        creatureAddon.visibilityDistanceType    = VisibilityDistanceType(fields[13].GetUInt8());

        Tokenizer tokens(fields[14].GetString(), ' ');
        uint8 i = 0;
        creatureAddon.auras.resize(tokens.size());
        for (Tokenizer::const_iterator itr = tokens.begin(); itr != tokens.end(); ++itr)
        {
            SpellInfo const* AdditionalSpellInfo = sSpellMgr->GetSpellInfo(atoul(*itr));
            if (!AdditionalSpellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has wrong spell %lu defined in `auras` field in `creature_addon`.", guid, atoul(*itr));
                continue;
            }

            if (AdditionalSpellInfo->HasAura(SPELL_AURA_CONTROL_VEHICLE))
                TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has SPELL_AURA_CONTROL_VEHICLE aura %lu defined in `auras` field in `creature_addon`.", guid, atoul(*itr));

            if (std::find(creatureAddon.auras.begin(), creatureAddon.auras.end(), atoul(*itr)) != creatureAddon.auras.end())
            {
                TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has duplicate aura (spell %lu) in `auras` field in `creature_addon`.", guid, atoul(*itr));
                continue;
            }

            creatureAddon.auras[i++] = atoul(*itr);
        }

        if (creatureAddon.mount)
        {
            if (!sCreatureDisplayInfoStore.LookupEntry(creatureAddon.mount))
            {
                TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has invalid displayInfoId (%u) for mount defined in `creature_addon`", guid, creatureAddon.mount);
                creatureAddon.mount = 0;
            }
        }

        if (creatureAddon.standState >= MAX_UNIT_STAND_STATE)
        {
            TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has invalid unit stand state (%u) defined in `creature_addon`. Truncated to 0.", guid, creatureAddon.standState);
            creatureAddon.standState = 0;
        }

        if (AnimTier(creatureAddon.animTier) >= AnimTier::Max)
        {
            TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has invalid animation tier (%u) defined in `creature_addon`. Truncated to 0.", guid, creatureAddon.animTier);
            creatureAddon.animTier = 0;
        }

        if (creatureAddon.sheathState >= MAX_SHEATH_STATE)
        {
            TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has invalid sheath state (%u) defined in `creature_addon`. Truncated to 0.", guid, creatureAddon.sheathState);
            creatureAddon.sheathState = 0;
        }

        // PvPFlags don't need any checking for the time being since they cover the entire range of a byte

        if (!sEmotesStore.LookupEntry(creatureAddon.emote))
        {
            TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has invalid emote (%u) defined in `creature_addon`.", guid, creatureAddon.emote);
            creatureAddon.emote = 0;
        }

        if (creatureAddon.aiAnimKit && !sAnimKitStore.LookupEntry(creatureAddon.aiAnimKit))
        {
            TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has invalid aiAnimKit (%u) defined in `creature_addon`.", guid, creatureAddon.aiAnimKit);
            creatureAddon.aiAnimKit = 0;
        }

        if (creatureAddon.movementAnimKit && !sAnimKitStore.LookupEntry(creatureAddon.movementAnimKit))
        {
            TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has invalid movementAnimKit (%u) defined in `creature_addon`.", guid, creatureAddon.movementAnimKit);
            creatureAddon.movementAnimKit = 0;
        }

        if (creatureAddon.meleeAnimKit && !sAnimKitStore.LookupEntry(creatureAddon.meleeAnimKit))
        {
            TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has invalid meleeAnimKit (%u) defined in `creature_addon`.", guid, creatureAddon.meleeAnimKit);
            creatureAddon.meleeAnimKit = 0;
        }

        if (creatureAddon.visibilityDistanceType >= VisibilityDistanceType::Max)
        {
            TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) has invalid visibilityDistanceType (%u) defined in `creature_addon`.",
                guid, AsUnderlyingType(creatureAddon.visibilityDistanceType));
            creatureAddon.visibilityDistanceType = VisibilityDistanceType::Normal;
        }

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature addons in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadGameObjectAddons()
{
    uint32 oldMSTime = getMSTime();

    //                                               0     1                 2                 3                 4                 5                 6
    QueryResult result = WorldDatabase.Query("SELECT guid, parent_rotation0, parent_rotation1, parent_rotation2, parent_rotation3, invisibilityType, invisibilityValue FROM gameobject_addon");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 gameobject addon definitions. DB table `gameobject_addon` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType guid = fields[0].GetUInt32();

        GameObjectData const* goData = GetGameObjectData(guid);
        if (!goData)
        {
            TC_LOG_ERROR("sql.sql", "GameObject (GUID: %u) does not exist but has a record in `gameobject_addon`", guid);
            continue;
        }

        GameObjectAddon& gameObjectAddon = _gameObjectAddonStore[guid];
        gameObjectAddon.ParentRotation = QuaternionData(fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat(), fields[4].GetFloat());
        gameObjectAddon.invisibilityType = InvisibilityType(fields[5].GetUInt8());
        gameObjectAddon.InvisibilityValue = fields[6].GetUInt32();

        if (gameObjectAddon.invisibilityType >= TOTAL_INVISIBILITY_TYPES)
        {
            TC_LOG_ERROR("sql.sql", "GameObject (GUID: %u) has invalid InvisibilityType in `gameobject_addon`, disabled invisibility", guid);
              gameObjectAddon.invisibilityType = INVISIBILITY_GENERAL;
            gameObjectAddon.InvisibilityValue = 0;
        }

        if (gameObjectAddon.invisibilityType && !gameObjectAddon.InvisibilityValue)
        {
            TC_LOG_ERROR("sql.sql", "GameObject (GUID: %u) has InvisibilityType set but has no InvisibilityValue in `gameobject_addon`, set to 1", guid);
            gameObjectAddon.InvisibilityValue = 1;
        }

        if (!gameObjectAddon.ParentRotation.isUnit())
        {
            TC_LOG_ERROR("sql.sql", "GameObject (GUID: %u) has invalid parent rotation in `gameobject_addon`, set to default", guid);
            gameObjectAddon.ParentRotation = QuaternionData();
        }

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u gameobject addons in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

GameObjectAddon const* ObjectMgr::GetGameObjectAddon(ObjectGuid::LowType lowguid) const
{
    GameObjectAddonContainer::const_iterator itr = _gameObjectAddonStore.find(lowguid);
    if (itr != _gameObjectAddonStore.end())
        return &(itr->second);

    return nullptr;
}

CreatureAddon const* ObjectMgr::GetCreatureAddon(ObjectGuid::LowType lowguid) const
{
    CreatureAddonContainer::const_iterator itr = _creatureAddonStore.find(lowguid);
    if (itr != _creatureAddonStore.end())
        return &(itr->second);

    return nullptr;
}

CreatureAddon const* ObjectMgr::GetCreatureTemplateAddon(uint32 entry) const
{
    CreatureAddonContainer::const_iterator itr = _creatureTemplateAddonStore.find(entry);
    if (itr != _creatureTemplateAddonStore.end())
        return &(itr->second);

    return nullptr;
}

CreatureMovementData const* ObjectMgr::GetCreatureMovementOverride(ObjectGuid::LowType spawnId) const
{
    return Trinity::Containers::MapGetValuePtr(_creatureMovementOverrides, spawnId);
}

EquipmentInfo const* ObjectMgr::GetEquipmentInfo(uint32 entry, int8& id) const
{
    EquipmentInfoContainer::const_iterator itr = _equipmentInfoStore.find(entry);
    if (itr == _equipmentInfoStore.end())
        return nullptr;

    if (itr->second.empty())
        return nullptr;

    if (id == -1) // select a random element
    {
        EquipmentInfoContainerInternal::const_iterator ritr = itr->second.begin();
        std::advance(ritr, urand(0u, itr->second.size() - 1));
        id = std::distance(itr->second.begin(), ritr) + 1;
        return &ritr->second;
    }
    else
    {
        EquipmentInfoContainerInternal::const_iterator itr2 = itr->second.find(id);
        if (itr2 != itr->second.end())
            return &itr2->second;
    }

    return nullptr;
}

void ObjectMgr::LoadEquipmentTemplates()
{
    uint32 oldMSTime = getMSTime();

    //                                                 0         1       2       3       4
    QueryResult result = WorldDatabase.Query("SELECT CreatureID, ID, ItemID1, ItemID2, ItemID3 FROM creature_equip_template");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature equipment templates. DB table `creature_equip_template` is empty!");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        if (!sObjectMgr->GetCreatureTemplate(entry))
        {
            TC_LOG_ERROR("sql.sql", "Creature template (Entry: %u) does not exist but has a record in `creature_equip_template`", entry);
            continue;
        }

        uint8 id = fields[1].GetUInt8();
        if (!id)
        {
            TC_LOG_ERROR("sql.sql", "Creature equipment template with id 0 found for creature %u, skipped.", entry);
            continue;
        }

        EquipmentInfo& equipmentInfo = _equipmentInfoStore[entry][id];

        equipmentInfo.ItemEntry[0] = fields[2].GetUInt32();
        equipmentInfo.ItemEntry[1] = fields[3].GetUInt32();
        equipmentInfo.ItemEntry[2] = fields[4].GetUInt32();

        for (uint8 i = 0; i < MAX_EQUIPMENT_ITEMS; ++i)
        {
            if (!equipmentInfo.ItemEntry[i])
                continue;

            ItemEntry const* dbcItem = sItemStore.LookupEntry(equipmentInfo.ItemEntry[i]);

            if (!dbcItem)
            {
                TC_LOG_ERROR("sql.sql", "Unknown item (entry=%u) in creature_equip_template.itemEntry%u for entry = %u and id=%u, forced to 0.",
                    equipmentInfo.ItemEntry[i], i+1, entry, id);
                equipmentInfo.ItemEntry[i] = 0;
                continue;
            }

            if (dbcItem->InventoryType != INVTYPE_WEAPON &&
                dbcItem->InventoryType != INVTYPE_SHIELD &&
                dbcItem->InventoryType != INVTYPE_RANGED &&
                dbcItem->InventoryType != INVTYPE_2HWEAPON &&
                dbcItem->InventoryType != INVTYPE_WEAPONMAINHAND &&
                dbcItem->InventoryType != INVTYPE_WEAPONOFFHAND &&
                dbcItem->InventoryType != INVTYPE_HOLDABLE &&
                dbcItem->InventoryType != INVTYPE_THROWN &&
                dbcItem->InventoryType != INVTYPE_RANGEDRIGHT)
            {
                TC_LOG_ERROR("sql.sql", "Item (entry=%u) in creature_equip_template.itemEntry%u for entry = %u and id = %u is not equipable in a hand, forced to 0.",
                    equipmentInfo.ItemEntry[i], i+1, entry, id);
                equipmentInfo.ItemEntry[i] = 0;
            }
        }

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u equipment templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureMovementOverrides()
{
    uint32 oldMSTime = getMSTime();

    _creatureMovementOverrides.clear();

    // Load the data from creature_movement_override and if NULL fallback to creature_template_movement
    QueryResult result = WorldDatabase.Query(
        "SELECT cmo.SpawnId,"
        "COALESCE(cmo.HoverInitiallyEnabled, ctm.HoverInitiallyEnabled),"
        "COALESCE(cmo.Random, ctm.Random),"
        "COALESCE(cmo.InteractionPauseTimer, ctm.InteractionPauseTimer) "
        "FROM creature_movement_override AS cmo "
        "LEFT JOIN creature AS c ON c.guid = cmo.SpawnId "
        "LEFT JOIN creature_template_movement AS ctm ON ctm.CreatureId = c.id");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature movement overrides. DB table `creature_movement_override` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        ObjectGuid::LowType spawnId = fields[0].GetUInt32();
        if (!GetCreatureData(spawnId))
        {
            TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) does not exist but has a record in `creature_movement_override`", spawnId);
            continue;
        }

        CreatureMovementData& movement = _creatureMovementOverrides[spawnId];
        if (!fields[1].IsNull())
            movement.HoverInitiallyEnabled = fields[1].GetBool();
        if (!fields[2].IsNull())
            movement.Random = static_cast<CreatureRandomMovementType>(fields[3].GetUInt8());
        if (!fields[3].IsNull())
            movement.InteractionPauseTimer = fields[4].GetUInt32();

        CheckCreatureMovement("creature_movement_override", spawnId, movement);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " movement overrides in %u ms", _creatureMovementOverrides.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureMovementInfo()
{
    uint32 oldMSTime = getMSTime();

    _creatureMovementInfoOverrideStore.clear();

    QueryResult result = WorldDatabase.Query("SELECT MovementID, WalkSpeed, RunSpeed FROM creature_movement_info");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature movement info override entries. DB table `creature_movement_info` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 movementId = fields[0].GetUInt32();

        float walkSpeed = !fields[1].IsNull() ? fields[1].GetFloat() : 0.f;
        float runSpeed = !fields[2].IsNull() ? fields[2].GetFloat() : 0.f;

        if (walkSpeed < 0.f || runSpeed < 0.f)
        {
            TC_LOG_ERROR("sql.sql", "MovementID %u in `creature_movement_info` contains negative speed values which can never be correct. Skipped loading.", movementId);
            continue;
        }

        _creatureMovementInfoOverrideStore[movementId] = { walkSpeed, runSpeed };

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " creature movement info override entries in %u ms", _creatureMovementInfoOverrideStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

CreatureModelInfo const* ObjectMgr::GetCreatureModelInfo(uint32 modelId) const
{
    CreatureModelContainer::const_iterator itr = _creatureModelStore.find(modelId);
    if (itr != _creatureModelStore.end())
        return &(itr->second);

    return nullptr;
}

CreatureModel const* ObjectMgr::ChooseDisplayId(CreatureTemplate const* cinfo, CreatureData const* data /*= nullptr*/)
{
    // Load creature model (display id)
    if (data && data->displayid)
        if (CreatureModel const* model = cinfo->GetModelWithDisplayId(data->displayid))
            return model;

    if (!(cinfo->flags_extra & CREATURE_FLAG_EXTRA_TRIGGER))
        if (CreatureModel const* model = cinfo->GetRandomValidModel())
            return model;

    // Triggers by default receive the invisible model
    return cinfo->GetFirstInvisibleModel();
}

void ObjectMgr::ChooseCreatureFlags(CreatureTemplate const* cInfo, uint32* npcFlags, uint32* unitFlags, uint32* unitFlags2, CreatureStaticFlagsHolder const& staticFlags, CreatureData const* data /*= nullptr*/)
{
#define ChooseCreatureFlagSource(field) ((data && data->field.has_value()) ? *data->field : cInfo->field)

    if (data)
    {
        if (npcFlags)
            *npcFlags = ChooseCreatureFlagSource(npcflag);

        if (unitFlags)
        {
            *unitFlags = ChooseCreatureFlagSource(unit_flags);

            if (staticFlags.HasFlag(CREATURE_STATIC_FLAG_CAN_SWIM))
                *unitFlags |= UNIT_FLAG_CAN_SWIM;

            if (staticFlags.HasFlag(CREATURE_STATIC_FLAG_3_CANNOT_SWIM))
                *unitFlags |= UNIT_FLAG_CANT_SWIM;
        }

        if (unitFlags2)
        {
            *unitFlags2 = ChooseCreatureFlagSource(unit_flags2);

            if (staticFlags.HasFlag(CREATURE_STATIC_FLAG_3_CANNOT_TURN))
                *unitFlags2 |= UNIT_FLAG2_CANNOT_TURN;

            if (staticFlags.HasFlag(CREATURE_STATIC_FLAG_4_PREVENT_SWIM))
                *unitFlags2 |= UNIT_FLAG2_AI_WILL_ONLY_SWIM_IF_TARGET_SWIMS;
        }
    }


#undef ChooseCreatureFlagSource
}

CreatureModelInfo const* ObjectMgr::GetCreatureModelRandomGender(CreatureModel* model, CreatureTemplate const* creatureTemplate) const
{
    CreatureModelInfo const* modelInfo = GetCreatureModelInfo(model->CreatureDisplayID);
    if (!modelInfo)
        return nullptr;

    // If a model for another gender exists, 50% chance to use it
    if (modelInfo->modelid_other_gender != 0 && urand(0, 1) == 0)
    {
        CreatureModelInfo const* minfo_tmp = GetCreatureModelInfo(modelInfo->modelid_other_gender);
        if (!minfo_tmp)
            TC_LOG_ERROR("sql.sql", "Model (Entry: %u) has modelid_other_gender %u not found in table `creature_model_info`. ", model->CreatureDisplayID, modelInfo->modelid_other_gender);
        else
        {
            // DisplayID changed
            model->CreatureDisplayID = modelInfo->modelid_other_gender;
            if (creatureTemplate)
            {
                auto itr = std::find_if(creatureTemplate->Models.begin(), creatureTemplate->Models.end(), [&](CreatureModel const& templateModel)
                    {
                        return templateModel.CreatureDisplayID == modelInfo->modelid_other_gender;
                    });
                if (itr != creatureTemplate->Models.end())
                    *model = *itr;
            }
            return minfo_tmp;
        }
    }

    return modelInfo;
}

void ObjectMgr::LoadCreatureModelInfo()
{
    uint32 oldMSTime = getMSTime();

    //                                                   0             1             2          3               4
    QueryResult result = WorldDatabase.Query("SELECT DisplayID, BoundingRadius, CombatReach, Gender, DisplayID_Other_Gender FROM creature_model_info");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature model definitions. DB table `creature_model_info` is empty.");
        return;
    }

    _creatureModelStore.reserve(result->GetRowCount());
    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 modelId = fields[0].GetUInt32();
        CreatureDisplayInfoEntry const* creatureDisplay = sCreatureDisplayInfoStore.LookupEntry(modelId);
        if (!creatureDisplay)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_model_info` has model for nonexistent display id (%u).", modelId);
            continue;
        }

        CreatureModelInfo& modelInfo = _creatureModelStore[modelId];

        modelInfo.bounding_radius      = fields[1].GetFloat();
        modelInfo.combat_reach         = fields[2].GetFloat();
        modelInfo.gender               = fields[3].GetUInt8();
        modelInfo.modelid_other_gender = fields[4].GetUInt32();
        modelInfo.is_trigger           = false;

        // Checks

        if (modelInfo.gender > GENDER_NONE)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_model_info` has wrong gender (%u) for display id (%u).", uint32(modelInfo.gender), modelId);
            modelInfo.gender = GENDER_MALE;
        }

        if (modelInfo.modelid_other_gender && !sCreatureDisplayInfoStore.LookupEntry(modelInfo.modelid_other_gender))
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_model_info` has nonexistent alt.gender model (%u) for existed display id (%u).", modelInfo.modelid_other_gender, modelId);
            modelInfo.modelid_other_gender = 0;
        }

        if (CreatureModelDataEntry const* modelData = sCreatureModelDataStore.LookupEntry(creatureDisplay->ModelID))
            modelInfo.is_trigger = strstr(modelData->ModelName, "INVISIBLESTALKER");

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature model based info in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadLinkedRespawn()
{
    uint32 oldMSTime = getMSTime();

    _linkedRespawnStore.clear();
    //                                                 0        1          2
    QueryResult result = WorldDatabase.Query("SELECT guid, linkedGuid, linkType FROM linked_respawn ORDER BY guid ASC");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 linked respawns. DB table `linked_respawn` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType guidLow = fields[0].GetUInt32();
        ObjectGuid::LowType linkedGuidLow = fields[1].GetUInt32();
        uint8  linkType = fields[2].GetUInt8();

        ObjectGuid guid, linkedGuid;
        bool error = false;
        switch (linkType)
        {
            case CREATURE_TO_CREATURE:
            {
                CreatureData const* slave = GetCreatureData(guidLow);
                if (!slave)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Creature (guid) '%u' not found in creature table", guidLow);
                    error = true;
                    break;
                }

                CreatureData const* master = GetCreatureData(linkedGuidLow);
                if (!master)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Creature (linkedGuid) '%u' not found in creature table", linkedGuidLow);
                    error = true;
                    break;
                }

                MapEntry const* const map = sMapStore.LookupEntry(master->mapId);
                if (!map || !map->Instanceable() || (master->mapId != slave->mapId))
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Creature '%u' linking to Creature '%u' on an unpermitted map.", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                if (!(master->spawnMask & slave->spawnMask))  // they must have a possibility to meet (normal/heroic difficulty)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Creature '%u' linking to Creature '%u' with not corresponding spawnMask", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                guid = ObjectGuid(HighGuid::Unit, slave->id, guidLow);
                linkedGuid = ObjectGuid(HighGuid::Unit, master->id, linkedGuidLow);
                break;
            }
            case CREATURE_TO_GO:
            {
                CreatureData const* slave = GetCreatureData(guidLow);
                if (!slave)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Creature (guid) '%u' not found in creature table", guidLow);
                    error = true;
                    break;
                }

                GameObjectData const* master = GetGameObjectData(linkedGuidLow);
                if (!master)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject (linkedGuid) '%u' not found in gameobject table", linkedGuidLow);
                    error = true;
                    break;
                }

                MapEntry const* const map = sMapStore.LookupEntry(master->mapId);
                if (!map || !map->Instanceable() || (master->mapId != slave->mapId))
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Creature '%u' linking to Gameobject '%u' on an unpermitted map.", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                if (!(master->spawnMask & slave->spawnMask))  // they must have a possibility to meet (normal/heroic difficulty)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Creature '%u' linking to Gameobject '%u' with not corresponding spawnMask", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                guid = ObjectGuid(HighGuid::Unit, slave->id, guidLow);
                linkedGuid = ObjectGuid(HighGuid::GameObject, master->id, linkedGuidLow);
                break;
            }
            case GO_TO_GO:
            {
                GameObjectData const* slave = GetGameObjectData(guidLow);
                if (!slave)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject (guid) '%u' not found in gameobject table", guidLow);
                    error = true;
                    break;
                }

                GameObjectData const* master = GetGameObjectData(linkedGuidLow);
                if (!master)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject (linkedGuid) '%u' not found in gameobject table", linkedGuidLow);
                    error = true;
                    break;
                }

                MapEntry const* const map = sMapStore.LookupEntry(master->mapId);
                if (!map || !map->Instanceable() || (master->mapId != slave->mapId))
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject '%u' linking to Gameobject '%u' on an unpermitted map.", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                if (!(master->spawnMask & slave->spawnMask))  // they must have a possibility to meet (normal/heroic difficulty)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject '%u' linking to Gameobject '%u' with not corresponding spawnMask", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                guid = ObjectGuid(HighGuid::GameObject, slave->id, guidLow);
                linkedGuid = ObjectGuid(HighGuid::GameObject, master->id, linkedGuidLow);
                break;
            }
            case GO_TO_CREATURE:
            {
                GameObjectData const* slave = GetGameObjectData(guidLow);
                if (!slave)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject (guid) '%u' not found in gameobject table", guidLow);
                    error = true;
                    break;
                }

                CreatureData const* master = GetCreatureData(linkedGuidLow);
                if (!master)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Creature (linkedGuid) '%u' not found in creature table", linkedGuidLow);
                    error = true;
                    break;
                }

                MapEntry const* const map = sMapStore.LookupEntry(master->mapId);
                if (!map || !map->Instanceable() || (master->mapId != slave->mapId))
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject '%u' linking to Creature '%u' on an unpermitted map.", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                if (!(master->spawnMask & slave->spawnMask))  // they must have a possibility to meet (normal/heroic difficulty)
                {
                    TC_LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject '%u' linking to Creature '%u' with not corresponding spawnMask", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                guid = ObjectGuid(HighGuid::GameObject, slave->id, guidLow);
                linkedGuid = ObjectGuid(HighGuid::Unit, master->id, linkedGuidLow);
                break;
            }
        }

        if (!error)
            _linkedRespawnStore[guid] = linkedGuid;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %lu linked respawns in %u ms", uint64(_linkedRespawnStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

bool ObjectMgr::SetCreatureLinkedRespawn(ObjectGuid::LowType guidLow, ObjectGuid::LowType linkedGuidLow)
{
    if (!guidLow)
        return false;

    CreatureData const* master = GetCreatureData(guidLow);
    ASSERT(master);
    ObjectGuid guid(HighGuid::Unit, master->id, guidLow);

    if (!linkedGuidLow) // we're removing the linking
    {
        _linkedRespawnStore.erase(guid);
        WorldDatabasePreparedStatement*stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CRELINKED_RESPAWN);
        stmt->setUInt32(0, guidLow);
        WorldDatabase.Execute(stmt);
        return true;
    }

    CreatureData const* slave = GetCreatureData(linkedGuidLow);
    if (!slave)
    {
        TC_LOG_ERROR("sql.sql", "Creature '%u' linking to non-existent creature '%u'.", guidLow, linkedGuidLow);
        return false;
    }

    MapEntry const* map = sMapStore.LookupEntry(master->mapId);
    if (!map || !map->Instanceable() || (master->mapId != slave->mapId))
    {
        TC_LOG_ERROR("sql.sql", "Creature '%u' linking to '%u' on an unpermitted map.", guidLow, linkedGuidLow);
        return false;
    }

    if (!(master->spawnMask & slave->spawnMask))  // they must have a possibility to meet (normal/heroic difficulty)
    {
        TC_LOG_ERROR("sql.sql", "LinkedRespawn: Creature '%u' linking to '%u' with not corresponding spawnMask", guidLow, linkedGuidLow);
        return false;
    }

    ObjectGuid linkedGuid(HighGuid::Unit, slave->id, linkedGuidLow);

    _linkedRespawnStore[guid] = linkedGuid;
    WorldDatabasePreparedStatement*stmt = WorldDatabase.GetPreparedStatement(WORLD_REP_CREATURE_LINKED_RESPAWN);
    stmt->setUInt32(0, guidLow);
    stmt->setUInt32(1, linkedGuidLow);
    WorldDatabase.Execute(stmt);
    return true;
}

void ObjectMgr::LoadTempSummons()
{
    uint32 oldMSTime = getMSTime();

    _tempSummonDataStore.clear();   // needed for reload case

    //                                               0           1             2        3      4           5           6           7            8           9
    QueryResult result = WorldDatabase.Query("SELECT summonerId, summonerType, groupId, entry, position_x, position_y, position_z, orientation, summonType, summonTime FROM creature_summon_groups");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 temp summons. DB table `creature_summon_groups` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 summonerId               = fields[0].GetUInt32();
        SummonerType summonerType       = SummonerType(fields[1].GetUInt8());
        uint8 group                     = fields[2].GetUInt8();

        switch (summonerType)
        {
            case SUMMONER_TYPE_CREATURE:
                if (!GetCreatureTemplate(summonerId))
                {
                    TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has summoner with non existing entry %u for creature summoner type, skipped.", summonerId);
                    continue;
                }
                break;
            case SUMMONER_TYPE_GAMEOBJECT:
                if (!GetGameObjectTemplate(summonerId))
                {
                    TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has summoner with non existing entry %u for gameobject summoner type, skipped.", summonerId);
                    continue;
                }
                break;
            case SUMMONER_TYPE_MAP:
                if (!sMapStore.LookupEntry(summonerId))
                {
                    TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has summoner with non existing entry %u for map summoner type, skipped.", summonerId);
                    continue;
                }
                break;
            default:
                TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has unhandled summoner type %u for summoner %u, skipped.", summonerType, summonerId);
                continue;
        }

        TempSummonData data;
        data.entry                      = fields[3].GetUInt32();

        if (!GetCreatureTemplate(data.entry))
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has creature in group [Summoner ID: %u, Summoner Type: %u, Group ID: %u] with non existing creature entry %u, skipped.", summonerId, summonerType, group, data.entry);
            continue;
        }

        float posX                      = fields[4].GetFloat();
        float posY                      = fields[5].GetFloat();
        float posZ                      = fields[6].GetFloat();
        float orientation               = fields[7].GetFloat();

        data.pos.Relocate(posX, posY, posZ, orientation);

        data.type                       = TempSummonType(fields[8].GetUInt8());

        if (data.type > TEMPSUMMON_MANUAL_DESPAWN)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has unhandled temp summon type %u in group [Summoner ID: %u, Summoner Type: %u, Group ID: %u] for creature entry %u, skipped.", data.type, summonerId, summonerType, group, data.entry);
            continue;
        }

        data.time                       = fields[9].GetUInt32();

        TempSummonGroupKey key(summonerId, summonerType, group);
        _tempSummonDataStore[key].push_back(data);

        ++count;

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u temp summons in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatures()
{
    uint32 oldMSTime = getMSTime();

    //                                               0              1   2    3           4           5           6            7        8             9              10
    QueryResult result = WorldDatabase.Query("SELECT creature.guid, id, map, position_x, position_y, position_z, orientation, modelid, equipment_id, spawntimesecs, wander_distance, "
    //   11               12         13       14            15         16          17           18                19                    20                    21
        "currentwaypoint, curhealth, curmana, MovementType, spawnMask, eventEntry, poolSpawnId, creature.npcflag, creature.unit_flags,  creature.unit_flags2, creature.phaseUseFlags, "
    //   22                23                   24                       25
        "creature.PhaseId, creature.PhaseGroup, creature.terrainSwapMap, creature.ScriptName "
        "FROM creature "
        "LEFT OUTER JOIN game_event_creature ON creature.guid = game_event_creature.guid "
        "LEFT OUTER JOIN pool_members ON pool_members.type = 0 AND creature.guid = pool_members.spawnId");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 creatures. DB table `creature` is empty.");
        return;
    }

    // Build single time for check spawnmask
    std::map<uint32, uint32> spawnMasks;
    for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
        if (sMapStore.LookupEntry(i))
            for (int k = 0; k < MAX_DIFFICULTY; ++k)
                if (sDBCManager.GetMapDifficultyData(i, Difficulty(k)))
                    spawnMasks[i] |= (1 << k);

    PhaseShift phaseShift;

    _creatureDataStore.reserve(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType guid = fields[0].GetUInt32();
        uint32 entry        = fields[1].GetUInt32();

        CreatureTemplate const* cInfo = GetCreatureTemplate(entry);
        if (!cInfo)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u) with non existing creature entry %u, skipped.", guid, entry);
            continue;
        }

        CreatureData& data = _creatureDataStore[guid];
        data.spawnId        = guid;
        data.id             = entry;
        data.mapId = fields[2].GetUInt16();
        data.spawnPoint.Relocate(fields[3].GetFloat(), fields[4].GetFloat(), fields[5].GetFloat(), fields[6].GetFloat());
        data.displayid      = fields[7].GetUInt32();
        data.equipmentId    = fields[8].GetInt8();
        data.spawntimesecs  = fields[9].GetUInt32();
        data.wanderDistance = fields[10].GetFloat();
        data.currentwaypoint= fields[11].GetUInt32();
        data.curhealth      = fields[12].GetUInt32();
        data.curmana        = fields[13].GetUInt32();
        data.movementType   = fields[14].GetUInt8();
        data.spawnMask      = fields[15].GetUInt8();
        int16 gameEvent     = fields[16].GetInt8();
        data.poolId         = fields[17].GetUInt32();
        if (!fields[18].IsNull())
            data.npcflag = fields[18].GetUInt64();
        if (!fields[19].IsNull())
            data.unit_flags = fields[19].GetUInt32();
        if (!fields[20].IsNull())
            data.unit_flags2 = fields[20].GetUInt32();
        data.phaseUseFlags  = fields[21].GetUInt8();
        data.phaseId        = fields[22].GetUInt32();
        data.phaseGroup     = fields[23].GetUInt32();
        data.terrainSwapMap = fields[24].GetInt32();
        data.scriptId = GetScriptId(fields[25].GetString());
        data.spawnGroupData = GetDefaultSpawnGroup();

        MapEntry const* mapEntry = sMapStore.LookupEntry(data.mapId);
        if (!mapEntry)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u) that spawned at nonexistent map (Id: %u), skipped.", guid, data.mapId);
            continue;
        }

        // Skip spawnMask check for transport maps
        if (!IsTransportMap(data.mapId))
        {
            if (data.spawnMask & ~spawnMasks[data.mapId])
                TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u) that have wrong spawn mask %u including unsupported difficulty modes for map (Id: %u).", guid, data.spawnMask, data.mapId);
        }
        else
            data.spawnGroupData = GetLegacySpawnGroup(); // force compatibility group for transport spawns

        bool ok = true;
        for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && ok; ++diff)
        {
            if (_difficultyEntries[diff].find(data.id) != _difficultyEntries[diff].end())
            {
                TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u) that is listed as difficulty %u template (entry: %u) in `creature_template`, skipped.",
                    guid, diff + 1, data.id);
                ok = false;
            }
        }
        if (!ok)
            continue;

        // -1 random, 0 no equipment
        if (data.equipmentId != 0)
        {
            if (!GetEquipmentInfo(data.id, data.equipmentId))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature` has creature (Entry: %u) with equipment_id %u not found in table `creature_equip_template`, set to no equipment.", data.id, data.equipmentId);
                data.equipmentId = 0;
            }
        }

        if (cInfo->flags_extra & CREATURE_FLAG_EXTRA_INSTANCE_BIND)
        {
            if (!mapEntry || !mapEntry->IsDungeon())
                TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u Entry: %u) with `creature_template`.`flags_extra` including CREATURE_FLAG_EXTRA_INSTANCE_BIND but creature is not in instance.", guid, data.id);
        }

        if (data.movementType >= MAX_DB_MOTION_TYPE)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u Entry: %u) with wrong movement generator type (%u), ignored and set to IDLE.", guid, data.id, data.movementType);
            data.movementType = IDLE_MOTION_TYPE;
        }

        if (data.wanderDistance < 0.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u Entry: %u) with `wander_distance`< 0.0, set to 0.", guid, data.id);
            data.wanderDistance = 0.0f;
        }
        else if (data.wanderDistance > 0.0f && data.wanderDistance < 0.1f)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u Entry: %u) with `wander_distance`< 0.1, set to 0.", guid, data.id);
            data.wanderDistance = 0.0f;
        }
        else if (data.movementType == RANDOM_MOTION_TYPE)
        {
            if (G3D::fuzzyEq(data.wanderDistance, 0.0f))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u Entry: %u) with `MovementType`= 1 (random movement) but with `wander_distance`= 0, replace by idle movement type (0).", guid, data.id);
                data.movementType = IDLE_MOTION_TYPE;
            }
        }
        else if (data.movementType == IDLE_MOTION_TYPE)
        {
            if (data.wanderDistance != 0.0f)
            {
                TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u Entry: %u) with `MovementType`=0 (idle) have `wander_distance`<>0, set to 0.", guid, data.id);
                data.wanderDistance = 0.0f;
            }
        }

        if (data.phaseUseFlags & ~PHASE_USE_FLAGS_ALL)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` have creature (GUID: %u Entry: %u) has unknown `phaseUseFlags` set, removed unknown value.", guid, data.id);
            data.phaseUseFlags &= PHASE_USE_FLAGS_ALL;
        }

        if (data.phaseUseFlags & PHASE_USE_FLAGS_ALWAYS_VISIBLE && data.phaseUseFlags & PHASE_USE_FLAGS_INVERSE)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` have creature (GUID: %u Entry: %u) has both `phaseUseFlags` PHASE_USE_FLAGS_ALWAYS_VISIBLE and PHASE_USE_FLAGS_INVERSE,"
                " removing PHASE_USE_FLAGS_INVERSE.", guid, data.id);
            data.phaseUseFlags &= ~PHASE_USE_FLAGS_INVERSE;
        }

        if (data.phaseGroup && !sDBCManager.GetPhasesForGroup(data.phaseGroup))
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u Entry: %u) with non-existing `phasegroup` (%u) set, `phasegroup` set to 0", guid, data.id, data.phaseGroup);
            data.phaseGroup = 0;
        }

        if (data.phaseGroup && data.phaseId)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` have creature (GUID: %u Entry: %u) with both `phaseid` and `phasegroup` set, `phasegroup` set to 0", guid, data.id);
            data.phaseGroup = 0;
        }

        if (data.terrainSwapMap != -1)
        {
            MapEntry const* terrainSwapEntry = sMapStore.LookupEntry(data.terrainSwapMap);
            if (!terrainSwapEntry)
            {
                TC_LOG_ERROR("sql.sql", "Table `creature` have creature (GUID: %u Entry: %u) with `terrainSwapMap` %u does not exist, set to -1", guid, data.id, data.terrainSwapMap);
                data.terrainSwapMap = -1;
            }
            else if (terrainSwapEntry->ParentMapID != int32(data.mapId))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature` have creature (GUID: %u Entry: %u) with `terrainSwapMap` %u which cannot be used on spawn map, set to -1", guid, data.id, data.terrainSwapMap);
                data.terrainSwapMap = -1;
            }
        }

        if (data.unit_flags.has_value())
        {
            if (uint32 disallowedUnitFlags = (*data.unit_flags & ~UNIT_FLAG_ALLOWED))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u Entry: %u) with disallowed `unit_flags` %u, removing incorrect flag.", guid, data.id, disallowedUnitFlags);
                *data.unit_flags &= UNIT_FLAG_ALLOWED;
            }
        }

        if (data.unit_flags2.has_value())
        {
            if (uint32 disallowedUnitFlags2 = (*data.unit_flags2 & ~UNIT_FLAG2_ALLOWED))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u Entry: %u) with disallowed `unit_flags2` %u, removing incorrect flag.", guid, data.id, disallowedUnitFlags2);
                *data.unit_flags2 &= UNIT_FLAG2_ALLOWED;
            }

            if (*data.unit_flags2 & UNIT_FLAG2_FEIGN_DEATH && (!data.unit_flags.has_value() || !(*data.unit_flags & (UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC))))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u Entry: %u) has UNIT_FLAG2_FEIGN_DEATH set without IMMUNE_TO_PC | IMMUNE_TO_NPC, removing incorrect flag.", guid, data.id);
                *data.unit_flags2 &= ~UNIT_FLAG2_FEIGN_DEATH;
            }
        }

        if (sWorld->getBoolConfig(CONFIG_CALCULATE_CREATURE_ZONE_AREA_DATA))
        {
            uint32 zoneId = 0;
            uint32 areaId = 0;
            PhasingHandler::InitDbVisibleMapId(phaseShift, data.terrainSwapMap);
            sTerrainMgr.GetZoneAndAreaId(phaseShift, zoneId, areaId, data.mapId, data.spawnPoint);

            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_ZONE_AREA_DATA);

            stmt->setUInt32(0, zoneId);
            stmt->setUInt32(1, areaId);
            stmt->setUInt64(2, guid);

            WorldDatabase.Execute(stmt);
        }

        // Add to grid if not managed by the game event
        if (gameEvent == 0)
            AddCreatureToGrid(guid, &data);
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " creatures in %u ms", _creatureDataStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

CellObjectGuids const* ObjectMgr::GetCellObjectGuids(uint32 mapid, uint8 spawnMode, uint32 cell_id)
{
    if (CellObjectGuidsMap const* mapGuids = Trinity::Containers::MapGetValuePtr(_mapObjectGuidsStore, MAKE_PAIR32(mapid, spawnMode)))
        return Trinity::Containers::MapGetValuePtr(*mapGuids, cell_id);

    return nullptr;
}

CellObjectGuidsMap const* ObjectMgr::GetMapObjectGuids(uint32 mapid, uint8 spawnMode)
{
    return Trinity::Containers::MapGetValuePtr(_mapObjectGuidsStore, MAKE_PAIR32(mapid, spawnMode));
}

void ObjectMgr::AddCreatureToGrid(ObjectGuid::LowType guid, CreatureData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            CellCoord cellCoord = Trinity::ComputeCellCoord(data->spawnPoint.GetPositionX(), data->spawnPoint.GetPositionY());
            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->mapId, i)][cellCoord.GetId()];
            cell_guids.creatures.insert(guid);
        }
    }
}

void ObjectMgr::RemoveCreatureFromGrid(ObjectGuid::LowType guid, CreatureData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            CellCoord cellCoord = Trinity::ComputeCellCoord(data->spawnPoint.GetPositionX(), data->spawnPoint.GetPositionY());
            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->mapId, i)][cellCoord.GetId()];
            cell_guids.creatures.erase(guid);
        }
    }
}

void ObjectMgr::LoadGameObjects()
{
    uint32 oldMSTime = getMSTime();

    //                                               0                1   2    3           4           5           6
    QueryResult result = WorldDatabase.Query("SELECT gameobject.guid, id, map, position_x, position_y, position_z, orientation, "
    //   7          8          9          10         11             12            13     14         15          16
        "rotation0, rotation1, rotation2, rotation3, spawntimesecs, animprogress, state, spawnMask, eventEntry, poolSpawnId, "
    //   17             18       19          20              21
        "phaseUseFlags, PhaseId, PhaseGroup, terrainSwapMap, ScriptName "
        "FROM gameobject LEFT OUTER JOIN game_event_gameobject ON gameobject.guid = game_event_gameobject.guid "
        "LEFT OUTER JOIN pool_members ON pool_members.type = 1 AND gameobject.guid = pool_members.spawnId");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 gameobjects. DB table `gameobject` is empty.");
        return;
    }

    // build single time for check spawnmask
    std::map<uint32, uint32> spawnMasks;
    for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
        if (sMapStore.LookupEntry(i))
            for (int k = 0; k < MAX_DIFFICULTY; ++k)
                if (sDBCManager.GetMapDifficultyData(i, Difficulty(k)))
                    spawnMasks[i] |= (1 << k);

    PhaseShift phaseShift;

    _gameObjectDataStore.reserve(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType guid = fields[0].GetUInt32();
        uint32 entry        = fields[1].GetUInt32();

        GameObjectTemplate const* gInfo = GetGameObjectTemplate(entry);
        if (!gInfo)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u) with non existing gameobject entry %u, skipped.", guid, entry);
            continue;
        }

        if (!gInfo->displayId)
        {
            switch (gInfo->type)
            {
                case GAMEOBJECT_TYPE_TRAP:
                case GAMEOBJECT_TYPE_SPELL_FOCUS:
                    break;
                default:
                    TC_LOG_ERROR("sql.sql", "Gameobject (GUID: %u Entry %u GoType: %u) doesn't have a displayId (%u), not loaded.", guid, entry, gInfo->type, gInfo->displayId);
                    break;
            }
        }

        if (gInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(gInfo->displayId))
        {
            TC_LOG_ERROR("sql.sql", "Gameobject (GUID: %u Entry %u GoType: %u) has an invalid displayId (%u), not loaded.", guid, entry, gInfo->type, gInfo->displayId);
            continue;
        }

        GameObjectData& data = _gameObjectDataStore[guid];

        data.spawnId        = guid;
        data.id             = entry;
        data.mapId          = fields[2].GetUInt16();
        data.spawnPoint.Relocate(fields[3].GetFloat(), fields[4].GetFloat(), fields[5].GetFloat(), fields[6].GetFloat());
        data.rotation.x     = fields[7].GetFloat();
        data.rotation.y     = fields[8].GetFloat();
        data.rotation.z     = fields[9].GetFloat();
        data.rotation.w     = fields[10].GetFloat();
        data.spawntimesecs  = fields[11].GetInt32();
        data.spawnGroupData = GetDefaultSpawnGroup();

        MapEntry const* mapEntry = sMapStore.LookupEntry(data.mapId);
        if (!mapEntry)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u Entry: %u) spawned on a non-existed map (Id: %u), skip", guid, data.id, data.mapId);
            continue;
        }

        if (data.spawntimesecs == 0 && gInfo->IsDespawnAtAction())
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u Entry: %u) with `spawntimesecs` (0) value, but the gameobejct is marked as despawnable at action.", guid, data.id);
        }

        data.animprogress   = fields[12].GetUInt8();
        data.artKit         = 0;

        uint32 go_state     = fields[13].GetUInt8();
        if (go_state >= MAX_GO_STATE)
        {
            if (gInfo->type != GAMEOBJECT_TYPE_TRANSPORT || go_state > GO_STATE_TRANSPORT_ACTIVE + MAX_GO_STATE_TRANSPORT_STOP_FRAMES)
            {
                TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u Entry: %u) with invalid `state` (%u) value, skip", guid, data.id, go_state);
                continue;
            }
        }
        data.goState        = GOState(go_state);

        data.spawnMask      = fields[14].GetUInt8();

        if (!IsTransportMap(data.mapId))
        {
            if (data.spawnMask & ~spawnMasks[data.mapId])
                TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u Entry: %u) that has wrong spawn mask %u including unsupported difficulty modes for map (Id: %u), skip", guid, data.id, data.spawnMask, data.mapId);
        }
        else
            data.spawnGroupData = GetLegacySpawnGroup(); // force compatibility group for transport spawns

        int16 gameEvent     = fields[15].GetInt8();
        data.poolId         = fields[16].GetUInt32();
        data.phaseUseFlags  = fields[17].GetUInt8();
        data.phaseId        = fields[18].GetUInt32();
        data.phaseGroup     = fields[19].GetUInt32();

        if (data.phaseUseFlags & ~PHASE_USE_FLAGS_ALL)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` have gameobject (GUID: %u Entry: %u) has unknown `phaseUseFlags` set, removed unknown value.", guid, data.id);
            data.phaseUseFlags &= PHASE_USE_FLAGS_ALL;
        }

        if (data.phaseUseFlags & PHASE_USE_FLAGS_ALWAYS_VISIBLE && data.phaseUseFlags & PHASE_USE_FLAGS_INVERSE)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` have gameobject (GUID: %u Entry: %u) has both `phaseUseFlags` PHASE_USE_FLAGS_ALWAYS_VISIBLE and PHASE_USE_FLAGS_INVERSE,"
                " removing PHASE_USE_FLAGS_INVERSE.", guid, data.id);
            data.phaseUseFlags &= ~PHASE_USE_FLAGS_INVERSE;
        }

        if (data.phaseGroup && !sDBCManager.GetPhasesForGroup(data.phaseGroup))
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u Entry: %u) with non-existing `phasegroup` (%u) set, `phasegroup` set to 0", guid, data.id, data.phaseGroup);
            data.phaseGroup = 0;
        }

        if (data.phaseGroup && data.phaseId)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` have gameobject (GUID: %u Entry: %u) with both `phaseid` and `phasegroup` set, `phasegroup` set to 0", guid, data.id);
            data.phaseGroup = 0;
        }

        data.terrainSwapMap = fields[20].GetInt32();
        if (data.terrainSwapMap != -1)
        {
            MapEntry const* terrainSwapEntry = sMapStore.LookupEntry(data.terrainSwapMap);
            if (!terrainSwapEntry)
            {
                TC_LOG_ERROR("sql.sql", "Table `gameobject` have gameobject (GUID: %u Entry: %u) with `terrainSwapMap` %u does not exist, set to -1", guid, data.id, data.terrainSwapMap);
                data.terrainSwapMap = -1;
            }
            else if (terrainSwapEntry->ParentMapID != int32(data.mapId))
            {
                TC_LOG_ERROR("sql.sql", "Table `gameobject` have gameobject (GUID: %u Entry: %u) with `terrainSwapMap` %u which cannot be used on spawn map, set to -1", guid, data.id, data.terrainSwapMap);
                data.terrainSwapMap = -1;
            }
        }

        data.scriptId = GetScriptId(fields[21].GetString());

        if (data.rotation.x < -1.0f || data.rotation.x > 1.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u Entry: %u) with invalid rotationX (%f) value, skip", guid, data.id, data.rotation.x);
            continue;
        }

        if (data.rotation.y < -1.0f || data.rotation.y > 1.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u Entry: %u) with invalid rotationY (%f) value, skip", guid, data.id, data.rotation.y);
            continue;
        }

        if (data.rotation.z < -1.0f || data.rotation.z > 1.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u Entry: %u) with invalid rotationZ (%f) value, skip", guid, data.id, data.rotation.z);
            continue;
        }

        if (data.rotation.w < -1.0f || data.rotation.w > 1.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u Entry: %u) with invalid rotationW  (%f) value, skip", guid, data.id, data.rotation.w);
            continue;
        }

        if (!MapManager::IsValidMapCoord(data.mapId, data.spawnPoint))
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u Entry: %u) with invalid coordinates, skip", guid, data.id);
            continue;
        }

        if (!data.rotation.isUnit())
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: %u Entry: %u) with invalid rotation quaternion (non-unit), defaulting to orientation on Z axis only", guid, data.id);
            data.rotation = QuaternionData::fromEulerAnglesZYX(data.spawnPoint.GetOrientation(), 0.0f, 0.0f);
        }

        if (sWorld->getBoolConfig(CONFIG_CALCULATE_GAMEOBJECT_ZONE_AREA_DATA))
        {
            uint32 zoneId = 0;
            uint32 areaId = 0;
            PhasingHandler::InitDbVisibleMapId(phaseShift, data.terrainSwapMap);
            sTerrainMgr.GetZoneAndAreaId(phaseShift, zoneId, areaId, data.mapId, data.spawnPoint);

            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_GAMEOBJECT_ZONE_AREA_DATA);

            stmt->setUInt32(0, zoneId);
            stmt->setUInt32(1, areaId);
            stmt->setUInt64(2, guid);

            WorldDatabase.Execute(stmt);
        }

        if (gameEvent == 0)                      // if not this is to be managed by GameEvent System or Pool system
            AddGameobjectToGrid(guid, &data);
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " gameobjects in %u ms", _gameObjectDataStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadSpawnGroupTemplates()
{
    uint32 oldMSTime = getMSTime();

    //                                               0        1          2
    QueryResult result = WorldDatabase.Query("SELECT groupId, groupName, groupFlags FROM spawn_group_template");

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 groupId = fields[0].GetUInt32();
            SpawnGroupTemplateData& group = _spawnGroupDataStore[groupId];
            group.groupId = groupId;
            group.name = fields[1].GetString();
            group.mapId = SPAWNGROUP_MAP_UNSET;
            uint32 flags = fields[2].GetUInt32();
            if (flags & ~SPAWNGROUP_FLAGS_ALL)
            {
                flags &= SPAWNGROUP_FLAGS_ALL;
                TC_LOG_ERROR("sql.sql", "Invalid spawn group flag %u on group ID %u (%s), reduced to valid flag %u.", flags, groupId, group.name.c_str(), uint32(group.flags));
            }
            if (flags & SPAWNGROUP_FLAG_SYSTEM && flags & SPAWNGROUP_FLAG_MANUAL_SPAWN)
            {
                flags &= ~SPAWNGROUP_FLAG_MANUAL_SPAWN;
                TC_LOG_ERROR("sql.sql", "System spawn group %u (%s) has invalid manual spawn flag. Ignored.", groupId, group.name.c_str());
            }
            group.flags = SpawnGroupFlags(flags);
        } while (result->NextRow());
    }

    if (_spawnGroupDataStore.find(0) == _spawnGroupDataStore.end())
    {
        TC_LOG_ERROR("sql.sql", "Default spawn group (index 0) is missing from DB! Manually inserted.");
        SpawnGroupTemplateData& data = _spawnGroupDataStore[0];
        data.groupId = 0;
        data.name = "Default Group";
        data.mapId = 0;
        data.flags = SPAWNGROUP_FLAG_SYSTEM;
    }
    if (_spawnGroupDataStore.find(1) == _spawnGroupDataStore.end())
    {
        TC_LOG_ERROR("sql.sql", "Default legacy spawn group (index 1) is missing from DB! Manually inserted.");
        SpawnGroupTemplateData&data = _spawnGroupDataStore[1];
        data.groupId = 1;
        data.name = "Legacy Group";
        data.mapId = 0;
        data.flags = SpawnGroupFlags(SPAWNGROUP_FLAG_SYSTEM | SPAWNGROUP_FLAG_COMPATIBILITY_MODE);
    }

    if (result)
        TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " spawn group templates in %u ms", _spawnGroupDataStore.size(), GetMSTimeDiffToNow(oldMSTime));
    else
        TC_LOG_INFO("server.loading", ">> Loaded 0 spawn group templates. DB table `spawn_group_template` is empty.");

    return;
}

void ObjectMgr::LoadSpawnGroups()
{
    uint32 oldMSTime = getMSTime();

    //                                               0        1          2
    QueryResult result = WorldDatabase.Query("SELECT groupId, spawnType, spawnId FROM spawn_group");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spawn group members. DB table `spawn_group` is empty.");
        return;
    }

    uint32 numMembers = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 groupId = fields[0].GetUInt32();
        SpawnObjectType spawnType = SpawnObjectType(fields[1].GetUInt8());
        if (!SpawnData::TypeIsValid(spawnType))
        {
            TC_LOG_ERROR("sql.sql", "Spawn data with invalid type %u listed for spawn group %u. Skipped.", uint32(spawnType), groupId);
            continue;
        }
        ObjectGuid::LowType spawnId = fields[2].GetUInt32();

        SpawnMetadata const* data = GetSpawnMetadata(spawnType, spawnId);
        if (!data)
        {
            TC_LOG_ERROR("sql.sql", "Spawn data with ID (%u,%u) not found, but is listed as a member of spawn group %u!", uint32(spawnType), spawnId, groupId);
            continue;
        }
        else if (data->spawnGroupData->groupId)
        {
            TC_LOG_ERROR("sql.sql", "Spawn with ID (%u,%u) is listed as a member of spawn group %u, but is already a member of spawn group %u. Skipping.", uint32(spawnType), spawnId, groupId, data->spawnGroupData->groupId);
            continue;
        }
        auto it = _spawnGroupDataStore.find(groupId);
        if (it == _spawnGroupDataStore.end())
        {
            TC_LOG_ERROR("sql.sql", "Spawn group %u assigned to spawn ID (%u,%u), but group does not exist!", groupId, uint32(spawnType), spawnId);
            continue;
        }
        else
        {
            SpawnGroupTemplateData& groupTemplate = it->second;
            if (groupTemplate.mapId == SPAWNGROUP_MAP_UNSET)
            {
                groupTemplate.mapId = data->mapId;
                _spawnGroupsByMap[data->mapId].push_back(groupId);
            }
            else if (groupTemplate.mapId != data->mapId && !(groupTemplate.flags & SPAWNGROUP_FLAG_SYSTEM))
            {
                TC_LOG_ERROR("sql.sql", "Spawn group %u has map ID %u, but spawn (%u,%u) has map id %u - spawn NOT added to group!", groupId, groupTemplate.mapId, uint32(spawnType), spawnId, data->mapId);
                continue;
            }
            const_cast<SpawnMetadata*>(data)->spawnGroupData = &groupTemplate;
            if (!(groupTemplate.flags & SPAWNGROUP_FLAG_SYSTEM))
                _spawnGroupMapStore.emplace(groupId, data);
            ++numMembers;
        }
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u spawn group members in %u ms", numMembers, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadInstanceSpawnGroups()
{
    uint32 oldMSTime = getMSTime();

    //                                               0              1            2           3             4
    QueryResult result = WorldDatabase.Query("SELECT instanceMapId, bossStateId, bossStates, spawnGroupId, flags FROM instance_spawn_groups");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 instance spawn groups. DB table `instance_spawn_groups` is empty.");
        return;
    }

    uint32 n = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 const spawnGroupId = fields[3].GetUInt32();
        auto it = _spawnGroupDataStore.find(spawnGroupId);
        if (it == _spawnGroupDataStore.end() || (it->second.flags & SPAWNGROUP_FLAG_SYSTEM))
        {
            TC_LOG_ERROR("sql.sql", "Invalid spawn group %u specified for instance %u. Skipped.", spawnGroupId, fields[0].GetUInt16());
            continue;
        }

        uint16 const instanceMapId = fields[0].GetUInt16();
        if (it->second.mapId != instanceMapId)
        {
            TC_LOG_ERROR("sql.sql", "Instance spawn group %u specified for instance %u has spawns on a different map %u. Skipped.",
                spawnGroupId, instanceMapId, it->second.mapId);
            continue;
        }

        InstanceSpawnGroupInfo& info = _instanceSpawnGroupStore[instanceMapId].emplace_back();
        info.SpawnGroupId = spawnGroupId;
        info.BossStateId = fields[1].GetUInt8();

        uint8 const ALL_STATES = (1 << TO_BE_DECIDED) - 1;
        uint8 const states = fields[2].GetUInt8();
        if (states & ~ALL_STATES)
        {
            info.BossStates = states & ALL_STATES;
            TC_LOG_ERROR("sql.sql", "Instance spawn group (%u,%u) had invalid boss state mask %u - truncated to %u.", instanceMapId, spawnGroupId, states, info.BossStates);
        }
        else
            info.BossStates = states;

        uint8 const flags = fields[4].GetUInt8();

        if ((flags & InstanceSpawnGroupInfo::FLAG_BLOCK_SPAWN) && (flags & InstanceSpawnGroupInfo::FLAG_ACTIVATE_SPAWN))
        {
            info.Flags = flags & InstanceSpawnGroupInfo::FLAG_BLOCK_SPAWN;
            TC_LOG_ERROR("sql.sql", "Instance spawn group (%u,%u) FLAG_ACTIVATE_SPAWN and FLAG_BLOCK_SPAWN may not be used together in a single entry - truncated to %u.", instanceMapId, spawnGroupId, info.Flags);
        }

        if ((flags & InstanceSpawnGroupInfo::FLAG_ALLIANCE_ONLY) && (flags & InstanceSpawnGroupInfo::FLAG_HORDE_ONLY))
        {
            info.Flags = flags & ~(InstanceSpawnGroupInfo::FLAG_ALLIANCE_ONLY | InstanceSpawnGroupInfo::FLAG_HORDE_ONLY);
            TC_LOG_ERROR("sql.sql", "Instance spawn group (%u,%u) FLAG_ALLIANCE_ONLY and FLAG_HORDE_ONLY may not be used together in a single entry - truncated to %u.", instanceMapId, spawnGroupId, info.Flags);
        }

        if (flags & ~InstanceSpawnGroupInfo::FLAG_ALL)
        {
            info.Flags = flags & InstanceSpawnGroupInfo::FLAG_ALL;
            TC_LOG_ERROR("sql.sql", "Instance spawn group (%u,%u) had invalid flags %u - truncated to %u.", instanceMapId, spawnGroupId, flags, info.Flags);
        }
        else
            info.Flags = flags;

        ++n;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u instance spawn groups in %u ms", n, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::OnDeleteSpawnData(SpawnData const* data)
{
    auto templateIt = _spawnGroupDataStore.find(data->spawnGroupData->groupId);
    ASSERT(templateIt != _spawnGroupDataStore.end(), "Creature data for (%u,%u) is being deleted and has invalid spawn group index %u!", uint32(data->type), data->spawnId, data->spawnGroupData->groupId);
    if (templateIt->second.flags & SPAWNGROUP_FLAG_SYSTEM) // system groups don't store their members in the map
        return;

    auto pair = _spawnGroupMapStore.equal_range(data->spawnGroupData->groupId);
    for (auto it = pair.first; it != pair.second; ++it)
    {
        if (it->second != data)
            continue;
        _spawnGroupMapStore.erase(it);
        return;
    }
    ASSERT(false, "Spawn data (%u,%u) being removed is member of spawn group %u, but not actually listed in the lookup table for that group!", uint32(data->type), data->spawnId, data->spawnGroupData->groupId);
}

void ObjectMgr::AddGameobjectToGrid(ObjectGuid::LowType guid, GameObjectData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            CellCoord cellCoord = Trinity::ComputeCellCoord(data->spawnPoint.GetPositionX(), data->spawnPoint.GetPositionY());
            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->mapId, i)][cellCoord.GetId()];
            cell_guids.gameobjects.insert(guid);
        }
    }
}

void ObjectMgr::RemoveGameobjectFromGrid(ObjectGuid::LowType guid, GameObjectData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            CellCoord cellCoord = Trinity::ComputeCellCoord(data->spawnPoint.GetPositionX(), data->spawnPoint.GetPositionY());
            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->mapId, i)][cellCoord.GetId()];
            cell_guids.gameobjects.erase(guid);
        }
    }
}

uint32 FillMaxDurability(uint32 itemClass, uint32 itemSubClass, uint32 inventoryType, uint32 quality, uint32 itemLevel)
{
    if (itemClass != ITEM_CLASS_ARMOR && itemClass != ITEM_CLASS_WEAPON)
        return 0;

    static float const qualityMultipliers[MAX_ITEM_QUALITY] =
    {
        1.0f, 1.0f, 1.0f, 1.17f, 1.37f, 1.68f, 0.0f, 0.0f
    };

    static float const armorMultipliers[MAX_INVTYPE] =
    {
        0.00f, // INVTYPE_NON_EQUIP
        0.59f, // INVTYPE_HEAD
        0.00f, // INVTYPE_NECK
        0.59f, // INVTYPE_SHOULDERS
        0.00f, // INVTYPE_BODY
        1.00f, // INVTYPE_CHEST
        0.35f, // INVTYPE_WAIST
        0.75f, // INVTYPE_LEGS
        0.49f, // INVTYPE_FEET
        0.35f, // INVTYPE_WRISTS
        0.35f, // INVTYPE_HANDS
        0.00f, // INVTYPE_FINGER
        0.00f, // INVTYPE_TRINKET
        0.00f, // INVTYPE_WEAPON
        1.00f, // INVTYPE_SHIELD
        0.00f, // INVTYPE_RANGED
        0.00f, // INVTYPE_CLOAK
        0.00f, // INVTYPE_2HWEAPON
        0.00f, // INVTYPE_BAG
        0.00f, // INVTYPE_TABARD
        1.00f, // INVTYPE_ROBE
        0.00f, // INVTYPE_WEAPONMAINHAND
        0.00f, // INVTYPE_WEAPONOFFHAND
        0.00f, // INVTYPE_HOLDABLE
        0.00f, // INVTYPE_AMMO
        0.00f, // INVTYPE_THROWN
        0.00f, // INVTYPE_RANGEDRIGHT
        0.00f, // INVTYPE_QUIVER
        0.00f, // INVTYPE_RELIC
    };

    static float const weaponMultipliers[MAX_ITEM_SUBCLASS_WEAPON] =
    {
        0.89f, // ITEM_SUBCLASS_WEAPON_AXE
        1.03f, // ITEM_SUBCLASS_WEAPON_AXE2
        0.77f, // ITEM_SUBCLASS_WEAPON_BOW
        0.77f, // ITEM_SUBCLASS_WEAPON_GUN
        0.89f, // ITEM_SUBCLASS_WEAPON_MACE
        1.03f, // ITEM_SUBCLASS_WEAPON_MACE2
        1.03f, // ITEM_SUBCLASS_WEAPON_POLEARM
        0.89f, // ITEM_SUBCLASS_WEAPON_SWORD
        1.03f, // ITEM_SUBCLASS_WEAPON_SWORD2
        0.00f, // ITEM_SUBCLASS_WEAPON_Obsolete
        1.03f, // ITEM_SUBCLASS_WEAPON_STAFF
        0.00f, // ITEM_SUBCLASS_WEAPON_EXOTIC
        0.00f, // ITEM_SUBCLASS_WEAPON_EXOTIC2
        0.64f, // ITEM_SUBCLASS_WEAPON_FIST_WEAPON
        0.00f, // ITEM_SUBCLASS_WEAPON_MISCELLANEOUS
        0.64f, // ITEM_SUBCLASS_WEAPON_DAGGER
        0.64f, // ITEM_SUBCLASS_WEAPON_THROWN
        0.00f, // ITEM_SUBCLASS_WEAPON_SPEAR
        0.77f, // ITEM_SUBCLASS_WEAPON_CROSSBOW
        0.64f, // ITEM_SUBCLASS_WEAPON_WAND
        0.64f, // ITEM_SUBCLASS_WEAPON_FISHING_POLE
    };

    float levelPenalty = 1.0f;
    if (itemLevel <= 28)
        levelPenalty = 0.966f - float(28u - itemLevel) / 54.0f;

    if (itemClass == ITEM_CLASS_ARMOR)
    {
        if (inventoryType > INVTYPE_ROBE)
            return 0;

        return 5 * uint32(23.0f * qualityMultipliers[quality] * armorMultipliers[inventoryType] * levelPenalty + 0.5f);
    }

    return 5 * uint32(17.0f * qualityMultipliers[quality] * weaponMultipliers[itemSubClass] * levelPenalty + 0.5f);
};

void FillDisenchantFields(uint32* disenchantID, uint32* requiredDisenchantSkill, ItemTemplate const& itemTemplate)
{
    *disenchantID = 0;
    *(int32*)requiredDisenchantSkill = -1;
    if ((itemTemplate.GetFlags() & (ITEM_FLAG_CONJURED | ITEM_FLAG_NO_DISENCHANT)) ||
        itemTemplate.GetBonding() == BIND_QUEST || itemTemplate.GetArea() || itemTemplate.GetMap() ||
        itemTemplate.GetMaxStackSize() > 1 ||
        itemTemplate.GetQuality() < ITEM_QUALITY_UNCOMMON || itemTemplate.GetQuality() > ITEM_QUALITY_EPIC ||
        !(itemTemplate.GetClass() == ITEM_CLASS_ARMOR || itemTemplate.GetClass() == ITEM_CLASS_WEAPON) ||
        !(Item::GetSpecialPrice(&itemTemplate) || sItemCurrencyCostStore.LookupEntry(itemTemplate.GetId())))
        return;

    for (uint32 i = 0; i < sItemDisenchantLootStore.GetNumRows(); ++i)
    {
        ItemDisenchantLootEntry const* disenchant = sItemDisenchantLootStore.LookupEntry(i);
        if (!disenchant)
            continue;

        if (disenchant->Class == itemTemplate.GetClass() &&
            disenchant->Quality == itemTemplate.GetQuality() &&
            disenchant->MinLevel <= itemTemplate.GetBaseItemLevel() &&
            disenchant->MaxLevel >= itemTemplate.GetBaseItemLevel())
        {
            if (disenchant->ID == 60 || disenchant->ID == 61)   // epic item disenchant ilvl range 66-99 (classic)
            {
                if (itemTemplate.GetRequiredLevel() > 60 || itemTemplate.GetRequiredSkillRank() > 300)
                    continue;                                   // skip to epic item disenchant ilvl range 90-199 (TBC)
            }
            else if (disenchant->ID == 66 || disenchant->ID == 67)  // epic item disenchant ilvl range 90-199 (TBC)
            {
                if (itemTemplate.GetRequiredLevel() <= 60 || (itemTemplate.GetRequiredSkill() && itemTemplate.GetRequiredSkillRank() <= 300))
                    continue;
            }

            *disenchantID = disenchant->ID;
            *requiredDisenchantSkill = disenchant->SkillRequired;
            return;
        }
    }
}

void ObjectMgr::LoadItemTemplates()
{
    uint32 oldMSTime = getMSTime();
    uint32 sparseCount = 0;

    for (ItemSparseEntry const* sparse : sItemSparseStore)
    {
        ItemEntry const* db2Data = sItemStore.LookupEntry(sparse->ID);
        if (!db2Data)
            continue;

        ItemTemplate& itemTemplate = _itemTemplateStore[sparse->ID];

        itemTemplate.BasicData = db2Data;
        itemTemplate.ExtendedData = sparse;

        itemTemplate.MaxDurability = FillMaxDurability(itemTemplate.GetClass(), itemTemplate.GetSubClass(), itemTemplate.GetInventoryType(), itemTemplate.GetQuality(), itemTemplate.GetBaseItemLevel());
        FillDisenchantFields(&itemTemplate.DisenchantID, &itemTemplate.RequiredDisenchantSkill, itemTemplate);
        itemTemplate.ScriptId = 0;
        itemTemplate.FoodType = 0;
        itemTemplate.MinMoneyLoot = 0;
        itemTemplate.MaxMoneyLoot = 0;
        itemTemplate.FlagsCu = 0;
        itemTemplate.SpellPPMRate = 0.f;

        itemTemplate.Effects.resize(MAX_ITEM_PROTO_SPELLS);
        for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            ItemEffect& effect = itemTemplate.Effects[i];
            effect.SpellID = sparse->SpellID[i];
            effect.Trigger = sparse->SpellTrigger[i];
            effect.Charges = sparse->SpellCharges[i];
            effect.Cooldown = sparse->SpellCooldown[i];
            effect.Category = sparse->SpellCategory[i];
            effect.CategoryCooldown = sparse->SpellCategoryCooldown[i];
        }

        ++sparseCount;
    }

    // Check if item templates for DBC referenced character start outfit are present
    std::set<uint32> notFoundOutfit;
    for (uint32 i = 1; i < sCharStartOutfitStore.GetNumRows(); ++i)
    {
        CharStartOutfitEntry const* entry = sCharStartOutfitStore.LookupEntry(i);
        if (!entry)
            continue;

        for (int j = 0; j < MAX_OUTFIT_ITEMS; ++j)
        {
            if (entry->ItemID[j] <= 0)
                continue;

            uint32 item_id = entry->ItemID[j];

            if (!GetItemTemplate(item_id))
                notFoundOutfit.insert(item_id);
        }
    }

    for (std::set<uint32>::const_iterator itr = notFoundOutfit.begin(); itr != notFoundOutfit.end(); ++itr)
        TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not exist in `item.db2` but is referenced in `CharStartOutfit.dbc`", *itr);

    TC_LOG_INFO("server.loading", ">> Loaded %u item templates in %u ms", sparseCount, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadItemTemplateAddon()
{
    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    QueryResult result = WorldDatabase.Query("SELECT Id, FlagsCu, FoodType, MinMoneyLoot, MaxMoneyLoot, SpellPPMChance FROM item_template_addon");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 itemId = fields[0].GetUInt32();
            if (!GetItemTemplate(itemId))
            {
                TC_LOG_ERROR("sql.sql", "Item %u specified in `item_template_addon` does not exist, skipped.", itemId);
                continue;
            }

            uint32 minMoneyLoot = fields[3].GetUInt32();
            uint32 maxMoneyLoot = fields[4].GetUInt32();
            if (minMoneyLoot > maxMoneyLoot)
            {
                TC_LOG_ERROR("sql.sql", "Minimum money loot specified in `item_template_addon` for item %u was greater than maximum amount, swapping.", itemId);
                std::swap(minMoneyLoot, maxMoneyLoot);
            }
            ItemTemplate& itemTemplate = _itemTemplateStore[itemId];
            itemTemplate.FlagsCu = fields[1].GetUInt32();
            itemTemplate.FoodType = fields[2].GetUInt8();
            itemTemplate.MinMoneyLoot = minMoneyLoot;
            itemTemplate.MaxMoneyLoot = maxMoneyLoot;
            itemTemplate.SpellPPMRate = fields[5].GetFloat();
            ++count;
        } while (result->NextRow());
    }
    TC_LOG_INFO("server.loading", ">> Loaded %u item addon templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadItemScriptNames()
{
    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    QueryResult result = WorldDatabase.Query("SELECT Id, ScriptName FROM item_script_names");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 itemId = fields[0].GetUInt32();
            if (!GetItemTemplate(itemId))
            {
                TC_LOG_ERROR("sql.sql", "Item %u specified in `item_script_names` does not exist, skipped.", itemId);
                continue;
            }

            _itemTemplateStore[itemId].ScriptId = GetScriptId(fields[1].GetCString());
            ++count;
        } while (result->NextRow());
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u item script names in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}
ItemTemplate const* ObjectMgr::GetItemTemplate(uint32 entry) const
{
    ItemTemplateContainer::const_iterator itr = _itemTemplateStore.find(entry);
    if (itr != _itemTemplateStore.end())
        return &(itr->second);
    return nullptr;
}

CreatureMovementInfoOverride const* ObjectMgr::GetCreatureMovementInfoOverride(uint32 movementId) const
{
    return Trinity::Containers::MapGetValuePtr(_creatureMovementInfoOverrideStore, movementId);
}

void ObjectMgr::LoadVehicleTemplateAccessories()
{
    uint32 oldMSTime = getMSTime();

    _vehicleTemplateAccessoryStore.clear();                           // needed for reload case

    uint32 count = 0;

    //                                                  0             1              2          3           4             5
    QueryResult result = WorldDatabase.Query("SELECT `entry`, `accessory_entry`, `seat_id`, `minion`, `summontype`, `summontimer` FROM `vehicle_template_accessory`");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 vehicle template accessories. DB table `vehicle_template_accessory` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 entry        = fields[0].GetUInt32();
        uint32 accessory    = fields[1].GetUInt32();
        int8   seatId       = fields[2].GetInt8();
        bool   isMinion     = fields[3].GetBool();
        uint8  summonType   = fields[4].GetUInt8();
        uint32 summonTimer  = fields[5].GetUInt32();

        if (!sObjectMgr->GetCreatureTemplate(entry))
        {
            TC_LOG_ERROR("sql.sql", "Table `vehicle_template_accessory`: creature template entry %u does not exist.", entry);
            continue;
        }

        if (!sObjectMgr->GetCreatureTemplate(accessory))
        {
            TC_LOG_ERROR("sql.sql", "Table `vehicle_template_accessory`: Accessory %u does not exist.", accessory);
            continue;
        }

        if (_spellClickInfoStore.find(entry) == _spellClickInfoStore.end())
        {
            TC_LOG_ERROR("sql.sql", "Table `vehicle_template_accessory`: creature template entry %u has no data in npc_spellclick_spells", entry);
            continue;
        }

        _vehicleTemplateAccessoryStore[entry].push_back(VehicleAccessory(accessory, seatId, isMinion, summonType, summonTimer));

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u Vehicle Template Accessories in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadVehicleAccessories()
{
    uint32 oldMSTime = getMSTime();

    _vehicleAccessoryStore.clear();                           // needed for reload case

    uint32 count = 0;

    //                                                  0             1             2          3           4             5
    QueryResult result = WorldDatabase.Query("SELECT `guid`, `accessory_entry`, `seat_id`, `minion`, `summontype`, `summontimer` FROM `vehicle_accessory`");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 Vehicle Accessories in %u ms", GetMSTimeDiffToNow(oldMSTime));
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 uiGUID       = fields[0].GetUInt32();
        uint32 uiAccessory  = fields[1].GetUInt32();
        int8   uiSeat       = int8(fields[2].GetInt16());
        bool   bMinion      = fields[3].GetBool();
        uint8  uiSummonType = fields[4].GetUInt8();
        uint32 uiSummonTimer= fields[5].GetUInt32();

        if (!sObjectMgr->GetCreatureTemplate(uiAccessory))
        {
            TC_LOG_ERROR("sql.sql", "Table `vehicle_accessory`: Accessory %u does not exist.", uiAccessory);
            continue;
        }

        _vehicleAccessoryStore[uiGUID].push_back(VehicleAccessory(uiAccessory, uiSeat, bMinion, uiSummonType, uiSummonTimer));

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u Vehicle Accessories in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadVehicleSeatAddon()
{
    uint32 oldMSTime = getMSTime();

    _vehicleSeatAddonStore.clear();                           // needed for reload case

    uint32 count = 0;

    //                                                0            1              2              3              4              5             6             7             8             9
    QueryResult result = WorldDatabase.Query("SELECT `SeatEntry`, `SeatOffsetX`, `SeatOffsetY`, `SeatOffsetZ`, `SeatOffsetO`, `ExitParamX`, `ExitParamY`, `ExitParamZ`, `ExitParamO`, `ExitParamValue` FROM `vehicle_seat_addon`");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 vehicle seat addons. DB table `vehicle_seat_addon` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 seatID       = fields[0].GetUInt32();
        float seatX         = fields[1].GetFloat();
        float seatY         = fields[2].GetFloat();
        float seatZ         = fields[3].GetFloat();
        float orientation   = fields[4].GetFloat();
        float exitX         = fields[5].GetFloat();
        float exitY         = fields[6].GetFloat();
        float exitZ         = fields[7].GetFloat();
        float exitO         = fields[8].GetFloat();
        uint8 exitParam     = fields[9].GetUInt8();

        if (!sVehicleSeatStore.LookupEntry(seatID))
        {
            TC_LOG_ERROR("sql.sql", "Table `vehicle_seat_addon`: SeatID: %u does not exist in VehicleSeat.dbc. Skipping entry.", seatID);
            continue;
        }

        // Sanitizing values
        if (orientation > float(M_PI * 2))
        {
            TC_LOG_ERROR("sql.sql", "Table `vehicle_seat_addon`: SeatID: %u is using invalid angle offset value (%f). Setting to 0.", seatID, orientation);
            orientation = 0.0f;
            continue;
        }

        if (exitParam >= AsUnderlyingType(VehicleExitParameters::VehicleExitParamMax))
        {
            TC_LOG_ERROR("sql.sql", "Table `vehicle_seat_addon`: SeatID: %u is using invalid exit parameter value (%u). Setting to 0 (none).", seatID, exitParam);
            continue;
        }

        _vehicleSeatAddonStore[seatID] = VehicleSeatAddon(seatX, seatY, seatZ, orientation, exitX, exitY, exitZ, exitO, exitParam);

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u Vehicle Seat Addon entries in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadPetLevelInfo()
{
    uint32 oldMSTime = getMSTime();

    //                                                 0               1      2   3     4    5    6    7     8    9
    QueryResult result = WorldDatabase.Query("SELECT creature_entry, level, hp, mana, str, agi, sta, inte, spi, armor FROM pet_levelstats");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 level pet stats definitions. DB table `pet_levelstats` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 creature_id = fields[0].GetUInt32();
        if (!sObjectMgr->GetCreatureTemplate(creature_id))
        {
            TC_LOG_ERROR("sql.sql", "Wrong creature id %u in `pet_levelstats` table, ignoring.", creature_id);
            continue;
        }

        uint32 current_level = fields[1].GetUInt8();
        if (current_level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            if (current_level > STRONG_MAX_LEVEL)        // hardcoded level maximum
                TC_LOG_ERROR("sql.sql", "Wrong (> %u) level %u in `pet_levelstats` table, ignoring.", STRONG_MAX_LEVEL, current_level);
            else
            {
                TC_LOG_INFO("misc", "Unused (> MaxPlayerLevel in worldserver.conf) level %u in `pet_levelstats` table, ignoring.", current_level);
                ++count;                                // make result loading percent "expected" correct in case disabled detail mode for example.
            }
            continue;
        }
        else if (current_level < 1)
        {
            TC_LOG_ERROR("sql.sql", "Wrong (<1) level %u in `pet_levelstats` table, ignoring.", current_level);
            continue;
        }

        PetLevelInfo*& pInfoMapEntry = _petInfoStore[creature_id];
        if (!pInfoMapEntry)
            pInfoMapEntry = new PetLevelInfo[sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)];

        // data for level 1 stored in [0] array element, ...
        PetLevelInfo* pLevelInfo = &pInfoMapEntry[current_level-1];

        pLevelInfo->health = fields[2].GetUInt16();
        pLevelInfo->mana   = fields[3].GetUInt16();
        pLevelInfo->armor  = fields[9].GetUInt32();

        for (int i = 0; i < MAX_STATS; i++)
        {
            pLevelInfo->stats[i] = fields[i+4].GetUInt16();
        }

        ++count;
    }
    while (result->NextRow());

    // Fill gaps and check integrity
    for (PetLevelInfoContainer::iterator itr = _petInfoStore.begin(); itr != _petInfoStore.end(); ++itr)
    {
        PetLevelInfo* pInfo = itr->second;

        // fatal error if no level 1 data
        if (!pInfo || pInfo[0].health == 0)
        {
            TC_LOG_ERROR("sql.sql", "Creature %u does not have pet stats data for Level 1!", itr->first);
            exit(1);
        }

        // fill level gaps
        for (uint8 level = 1; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
        {
            if (pInfo[level].health == 0)
            {
                TC_LOG_ERROR("sql.sql", "Creature %u has no data for Level %i pet stats data, using data of Level %i.", itr->first, level + 1, level);
                pInfo[level] = pInfo[level - 1];
            }
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u level pet stats definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

PetLevelInfo const* ObjectMgr::GetPetLevelInfo(uint32 creature_id, uint8 level) const
{
    if (level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        level = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);

    PetLevelInfoContainer::const_iterator itr = _petInfoStore.find(creature_id);
    if (itr == _petInfoStore.end())
        return nullptr;

    return &itr->second[level-1];                           // data for level 1 stored in [0] array element, ...
}

void ObjectMgr::PlayerCreateInfoAddItemHelper(uint32 race_, uint32 class_, uint32 itemId, int32 count)
{
    if (!_playerInfo[race_][class_])
        return;

    if (count > 0)
        _playerInfo[race_][class_]->item.push_back(PlayerCreateInfoItem(itemId, count));
    else
    {
        if (count < -1)
            TC_LOG_ERROR("sql.sql", "Invalid count %i specified on item %u be removed from original player create info (use -1)!", count, itemId);

        for (uint32 gender = 0; gender < GENDER_NONE; ++gender)
        {
            if (CharStartOutfitEntry const* entry = sDBCManager.GetCharStartOutfitEntry(race_, class_, gender))
            {
                bool found = false;
                for (uint8 x = 0; x < MAX_OUTFIT_ITEMS; ++x)
                {
                    if (entry->ItemID[x] > 0 && uint32(entry->ItemID[x]) == itemId)
                    {
                        found = true;
                        const_cast<CharStartOutfitEntry*>(entry)->ItemID[x] = 0;
                        break;
                    }
                }

                if (!found)
                    TC_LOG_ERROR("sql.sql", "Item %u specified to be removed from original create info not found in dbc!", itemId);
            }
        }
    }
}

void ObjectMgr::LoadPlayerInfo()
{
    // Load playercreate
    {
        uint32 oldMSTime = getMSTime();
        //                                                0     1      2    3        4          5           6
        QueryResult result = WorldDatabase.Query("SELECT race, class, map, zone, position_x, position_y, position_z, orientation FROM playercreateinfo");

        if (!result)
        {
            TC_LOG_ERROR("server.loading", ">> Loaded 0 player create definitions. DB table `playercreateinfo` is empty.");
            exit(1);
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();

                uint32 current_race  = fields[0].GetUInt8();
                uint32 current_class = fields[1].GetUInt8();
                uint32 mapId         = fields[2].GetUInt16();
                uint32 areaId        = fields[3].GetUInt32(); // zone
                float  positionX     = fields[4].GetFloat();
                float  positionY     = fields[5].GetFloat();
                float  positionZ     = fields[6].GetFloat();
                float  orientation   = fields[7].GetFloat();

                if (current_race >= MAX_RACES)
                {
                    TC_LOG_ERROR("sql.sql", "Wrong race %u in `playercreateinfo` table, ignoring.", current_race);
                    continue;
                }

                ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(current_race);
                if (!rEntry)
                {
                    TC_LOG_ERROR("sql.sql", "Wrong race %u in `playercreateinfo` table, ignoring.", current_race);
                    continue;
                }

                if (current_class >= MAX_CLASSES)
                {
                    TC_LOG_ERROR("sql.sql", "Wrong class %u in `playercreateinfo` table, ignoring.", current_class);
                    continue;
                }

                if (!sChrClassesStore.LookupEntry(current_class))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong class %u in `playercreateinfo` table, ignoring.", current_class);
                    continue;
                }

                // accept DB data only for valid position (and non instanceable)
                if (!MapManager::IsValidMapCoord(mapId, positionX, positionY, positionZ, orientation))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong home position for class %u race %u pair in `playercreateinfo` table, ignoring.", current_class, current_race);
                    continue;
                }

                if (sMapStore.LookupEntry(mapId)->Instanceable())
                {
                    TC_LOG_ERROR("sql.sql", "Home position in instanceable map for class %u race %u pair in `playercreateinfo` table, ignoring.", current_class, current_race);
                    continue;
                }

                PlayerInfo* info = new PlayerInfo();
                info->mapId = mapId;
                info->areaId = areaId;
                info->positionX = positionX;
                info->positionY = positionY;
                info->positionZ = positionZ;
                info->orientation = orientation;
                info->displayId_m = rEntry->MaleDisplayID;
                info->displayId_f = rEntry->FemaleDisplayID;
                _playerInfo[current_race][current_class] = info;

                ++count;
            }
            while (result->NextRow());

            TC_LOG_INFO("server.loading", ">> Loaded %u player create definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }

    // Load playercreate items
    TC_LOG_INFO("server.loading", "Loading Player Create Items Data...");
    {
        uint32 oldMSTime = getMSTime();
        //                                                0     1      2       3
        QueryResult result = WorldDatabase.Query("SELECT race, class, itemid, amount FROM playercreateinfo_item");

        if (!result)
        {
            TC_LOG_INFO("server.loading", ">> Loaded 0 custom player create items. DB table `playercreateinfo_item` is empty.");
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();

                uint32 current_race = fields[0].GetUInt8();
                if (current_race >= MAX_RACES)
                {
                    TC_LOG_ERROR("sql.sql", "Wrong race %u in `playercreateinfo_item` table, ignoring.", current_race);
                    continue;
                }

                uint32 current_class = fields[1].GetUInt8();
                if (current_class >= MAX_CLASSES)
                {
                    TC_LOG_ERROR("sql.sql", "Wrong class %u in `playercreateinfo_item` table, ignoring.", current_class);
                    continue;
                }

                uint32 item_id = fields[2].GetUInt32();

                if (!GetItemTemplate(item_id))
                {
                    TC_LOG_ERROR("sql.sql", "Item id %u (race %u class %u) in `playercreateinfo_item` table but not listed in `item_template`, ignoring.", item_id, current_race, current_class);
                    continue;
                }

                int32 amount   = fields[3].GetInt8();

                if (!amount)
                {
                    TC_LOG_ERROR("sql.sql", "Item id %u (class %u race %u) have amount == 0 in `playercreateinfo_item` table, ignoring.", item_id, current_race, current_class);
                    continue;
                }

                if (!current_race || !current_class)
                {
                    uint32 min_race = current_race ? current_race : 1;
                    uint32 max_race = current_race ? current_race + 1 : MAX_RACES;
                    uint32 min_class = current_class ? current_class : 1;
                    uint32 max_class = current_class ? current_class + 1 : MAX_CLASSES;
                    for (uint32 r = min_race; r < max_race; ++r)
                        for (uint32 c = min_class; c < max_class; ++c)
                            PlayerCreateInfoAddItemHelper(r, c, item_id, amount);
                }
                else
                    PlayerCreateInfoAddItemHelper(current_race, current_class, item_id, amount);

                ++count;
            }
            while (result->NextRow());

            TC_LOG_INFO("server.loading", ">> Loaded %u custom player create items in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }


    // Load playercreate skills
    TC_LOG_INFO("server.loading", "Loading Player Create Skill Data...");
    {
        uint32 oldMSTime = getMSTime();

        QueryResult result = WorldDatabase.PQuery("SELECT raceMask, classMask, skill, `rank` FROM playercreateinfo_skills");

        if (!result)
        {
            TC_LOG_ERROR("server.loading", ">> Loaded 0 player create skills. DB table `playercreateinfo_skills` is empty.");
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();
                uint32 raceMask = fields[0].GetUInt32();
                uint32 classMask = fields[1].GetUInt32();
                PlayerCreateInfoSkill skill;
                skill.SkillId = fields[2].GetUInt16();
                skill.Rank = fields[3].GetUInt16();

                if (skill.Rank >= MAX_SKILL_STEP)
                {
                    TC_LOG_ERROR("sql.sql", "Skill rank value %hu set for skill %hu raceMask %u classMask %u is too high, max allowed value is %d", skill.Rank, skill.SkillId, raceMask, classMask, MAX_SKILL_STEP);
                    continue;
                }

                if (raceMask != 0 && !(raceMask & RACEMASK_ALL_PLAYABLE))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong race mask %u in `playercreateinfo_skills` table, ignoring.", raceMask);
                    continue;
                }

                if (classMask != 0 && !(classMask & CLASSMASK_ALL_PLAYABLE))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong class mask %u in `playercreateinfo_skills` table, ignoring.", classMask);
                    continue;
                }

                if (!sSkillLineStore.LookupEntry(skill.SkillId))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong skill id %u in `playercreateinfo_skills` table, ignoring.", skill.SkillId);
                    continue;
                }

                for (uint32 raceIndex = RACE_HUMAN; raceIndex < MAX_RACES; ++raceIndex)
                {
                    if (raceMask == 0 || ((1 << (raceIndex - 1)) & raceMask))
                    {
                        for (uint32 classIndex = CLASS_WARRIOR; classIndex < MAX_CLASSES; ++classIndex)
                        {
                            if (classMask == 0 || ((1 << (classIndex - 1)) & classMask))
                            {
                                if (!sDBCManager.GetSkillRaceClassInfo(skill.SkillId, raceIndex, classIndex))
                                    continue;

                                if (PlayerInfo* info = _playerInfo[raceIndex][classIndex])
                                {
                                    info->skills.push_back(skill);
                                    ++count;
                                }
                            }
                        }
                    }
                }
            } while (result->NextRow());

            TC_LOG_INFO("server.loading", ">> Loaded %u player create skills in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }

    // Load playercreate spells
    TC_LOG_INFO("server.loading", "Loading Player Create Spell Data...");
    {
        uint32 oldMSTime = getMSTime();

        QueryResult result = WorldDatabase.PQuery("SELECT racemask, classmask, Spell FROM playercreateinfo_spell_custom");

        if (!result)
        {
            TC_LOG_ERROR("server.loading", ">> Loaded 0 player create spells. DB table `playercreateinfo_spell_custom` is empty.");
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();
                uint32 raceMask = fields[0].GetUInt32();
                uint32 classMask = fields[1].GetUInt32();
                uint32 spellId = fields[2].GetUInt32();

                if (raceMask != 0 && !(raceMask & RACEMASK_ALL_PLAYABLE))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong race mask %u in `playercreateinfo_spell_custom` table, ignoring.", raceMask);
                    continue;
                }

                if (classMask != 0 && !(classMask & CLASSMASK_ALL_PLAYABLE))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong class mask %u in `playercreateinfo_spell_custom` table, ignoring.", classMask);
                    continue;
                }

                for (uint32 raceIndex = RACE_HUMAN; raceIndex < MAX_RACES; ++raceIndex)
                {
                    if (raceMask == 0 || ((1 << (raceIndex - 1)) & raceMask))
                    {
                        for (uint32 classIndex = CLASS_WARRIOR; classIndex < MAX_CLASSES; ++classIndex)
                        {
                            if (classMask == 0 || ((1 << (classIndex - 1)) & classMask))
                            {
                                if (PlayerInfo* info = _playerInfo[raceIndex][classIndex])
                                {
                                    info->customSpells.push_back(spellId);
                                    ++count;
                                }
                                // We need something better here, the check is not accounting for spells used by multiple races/classes but not all of them.
                                // Either split the masks per class, or per race, which kind of kills the point yet.
                                // else if (raceMask != 0 && classMask != 0)
                                //     TC_LOG_ERROR("sql.sql", "Racemask/classmask (%u/%u) combination was found containing an invalid race/class combination (%u/%u) in `%s` (Spell %u), ignoring.", raceMask, classMask, raceIndex, classIndex, tableName.c_str(), spellId);
                            }
                        }
                    }
                }
            }
            while (result->NextRow());

            TC_LOG_INFO("server.loading", ">> Loaded %u custom player create spells in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }

    // Load playercreate cast spell
    TC_LOG_INFO("server.loading", "Loading Player Create Cast Spell Data...");
    {
        uint32 oldMSTime = getMSTime();

        QueryResult result = WorldDatabase.PQuery("SELECT raceMask, classMask, spell FROM playercreateinfo_cast_spell");

        if (!result)
            TC_LOG_ERROR("server.loading", ">> Loaded 0 player create cast spells. DB table `playercreateinfo_cast_spell` is empty.");
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields       = result->Fetch();
                uint32 raceMask     = fields[0].GetUInt32();
                uint32 classMask    = fields[1].GetUInt32();
                uint32 spellId      = fields[2].GetUInt32();

                if (raceMask != 0 && !(raceMask & RACEMASK_ALL_PLAYABLE))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong race mask %u in `playercreateinfo_cast_spell` table, ignoring.", raceMask);
                    continue;
                }

                if (classMask != 0 && !(classMask & CLASSMASK_ALL_PLAYABLE))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong class mask %u in `playercreateinfo_cast_spell` table, ignoring.", classMask);
                    continue;
                }

                for (uint32 raceIndex = RACE_HUMAN; raceIndex < MAX_RACES; ++raceIndex)
                {
                    if (raceMask == 0 || ((1 << (raceIndex - 1)) & raceMask))
                    {
                        for (uint32 classIndex = CLASS_WARRIOR; classIndex < MAX_CLASSES; ++classIndex)
                        {
                            if (classMask == 0 || ((1 << (classIndex - 1)) & classMask))
                            {
                                if (PlayerInfo* info = _playerInfo[raceIndex][classIndex])
                                {
                                    info->castSpells.push_back(spellId);
                                    ++count;
                                }
                            }
                        }
                    }
                }
            } while (result->NextRow());

            TC_LOG_INFO("server.loading", ">> Loaded %u player create cast spells in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }

    // Load playercreate actions
    TC_LOG_INFO("server.loading", "Loading Player Create Action Data...");
    {
        uint32 oldMSTime = getMSTime();

        //                                                0     1      2       3       4
        QueryResult result = WorldDatabase.Query("SELECT race, class, button, action, type FROM playercreateinfo_action");

        if (!result)
        {
            TC_LOG_ERROR("server.loading", ">> Loaded 0 player create actions. DB table `playercreateinfo_action` is empty.");
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();

                uint32 current_race = fields[0].GetUInt8();
                if (current_race >= MAX_RACES)
                {
                    TC_LOG_ERROR("sql.sql", "Wrong race %u in `playercreateinfo_action` table, ignoring.", current_race);
                    continue;
                }

                uint32 current_class = fields[1].GetUInt8();
                if (current_class >= MAX_CLASSES)
                {
                    TC_LOG_ERROR("sql.sql", "Wrong class %u in `playercreateinfo_action` table, ignoring.", current_class);
                    continue;
                }

                if (PlayerInfo* info = _playerInfo[current_race][current_class])
                    info->action.push_back(PlayerCreateInfoAction(fields[2].GetUInt16(), fields[3].GetUInt32(), fields[4].GetUInt16()));

                ++count;
            }
            while (result->NextRow());

            TC_LOG_INFO("server.loading", ">> Loaded %u player create actions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }

    // Loading levels data (class/race dependent)
    TC_LOG_INFO("server.loading", "Loading Player Create Level Stats Data...");
    {
        struct RaceStats
        {
            std::array<int16, MAX_STATS> StatModifier = { };
        };

        std::array<RaceStats, MAX_RACES> raceStatModifiers = { };

        uint32 oldMSTime = getMSTime();

        //                                                0      1      2    3    4    5     6
        QueryResult raceStatsResult = WorldDatabase.Query("SELECT race, str, agi, sta, inte, spi FROM player_racestats");

        if (!raceStatsResult)
        {
            TC_LOG_ERROR("server.loading", ">> Loaded 0 race stats definitions. DB table `player_racestats` is empty.");
            ABORT();
        }

        do
        {
            Field* fields = raceStatsResult->Fetch();

            uint32 current_race = fields[0].GetUInt8();
            if (current_race >= MAX_RACES)
            {
                TC_LOG_ERROR("sql.sql", "Wrong race %u in `player_racestats` table, ignoring.", current_race);
                continue;
            }

            for (uint8 i = 0; i < MAX_STATS; ++i)
                raceStatModifiers[current_race].StatModifier[i] = fields[i + 1].GetInt16();

        } while (raceStatsResult->NextRow());

        //                                               0      1      2    3    4    5     6
        QueryResult result = WorldDatabase.Query("SELECT class, level, str, agi, sta, inte, spi FROM player_classlevelstats");

        if (!result)
        {
            TC_LOG_ERROR("server.loading", ">> Loaded 0 level stats definitions. DB table `player_classlevelstats` is empty.");
            ABORT();
        }

        uint32 count = 0;

        do
        {
            Field* fields = result->Fetch();


            uint32 current_class = fields[0].GetUInt8();
            if (current_class >= MAX_CLASSES)
            {
                TC_LOG_ERROR("sql.sql", "Wrong class %u in `player_levelstats` table, ignoring.", current_class);
                continue;
            }

            uint32 current_level = fields[1].GetUInt8();
            if (current_level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
            {
                if (current_level > STRONG_MAX_LEVEL)        // hardcoded level maximum
                    TC_LOG_ERROR("sql.sql", "Wrong (> %u) level %u in `player_levelstats` table, ignoring.", STRONG_MAX_LEVEL, current_level);
                else
                {
                    TC_LOG_INFO("misc", "Unused (> MaxPlayerLevel in worldserver.conf) level %u in `player_levelstats` table, ignoring.", current_level);
                    ++count;                                // make result loading percent "expected" correct in case disabled detail mode for example.
                }
                continue;
            }

            for (std::size_t race = 0; race < raceStatModifiers.size(); ++race)
            {
                if (PlayerInfo* info = _playerInfo[race][current_class])
                {
                    if (!info->levelInfo)
                        info->levelInfo = new PlayerLevelInfo[sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)];

                    PlayerLevelInfo& levelInfo = info->levelInfo[current_level - 1];
                    for (uint8 i = 0; i < MAX_STATS; i++)
                        levelInfo.stats[i] = fields[i + 2].GetUInt16() + raceStatModifiers[race].StatModifier[i];
                }
            }

            ++count;
        }
        while (result->NextRow());

        // Fill gaps and check integrity
        for (int race = 0; race < MAX_RACES; ++race)
        {
            // skip non existed races
            if (!sChrRacesStore.LookupEntry(race))
                continue;

            for (int class_ = 0; class_ < MAX_CLASSES; ++class_)
            {
                // skip non existed classes
                if (!sChrClassesStore.LookupEntry(class_))
                    continue;

                PlayerInfo* info = _playerInfo[race][class_];
                if (!info)
                    continue;

                // skip expansion races if not playing with expansion
                if (sWorld->getIntConfig(CONFIG_EXPANSION) < EXPANSION_THE_BURNING_CRUSADE && (race == RACE_BLOODELF || race == RACE_DRAENEI))
                    continue;

                // skip expansion classes if not playing with expansion
                if (sWorld->getIntConfig(CONFIG_EXPANSION) < EXPANSION_WRATH_OF_THE_LICH_KING && class_ == CLASS_DEATH_KNIGHT)
                    continue;

                // skip expansion races if not playing with expansion
                if (sWorld->getIntConfig(CONFIG_EXPANSION) < EXPANSION_CATACLYSM && (race == RACE_GOBLIN || race == RACE_WORGEN))
                    continue;

                // fatal error if no level 1 data
                if (!info->levelInfo || info->levelInfo[0].stats[0] == 0)
                {
                    TC_LOG_ERROR("sql.sql", "Race %i Class %i Level 1 does not have stats data!", race, class_);
                    exit(1);
                }

                // fill level gaps
                for (uint8 level = 1; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
                {
                    if (info->levelInfo[level].stats[0] == 0)
                    {
                        TC_LOG_ERROR("sql.sql", "Race %i Class %i Level %i does not have stats data. Using stats data of level %i.", race, class_, level + 1, level);
                        info->levelInfo[level] = info->levelInfo[level - 1];
                    }
                }
            }
        }

        TC_LOG_INFO("server.loading", ">> Loaded %u level stats definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }

    // Loading xp per level data
    TC_LOG_INFO("server.loading", "Loading Player Create XP Data...");
    {
        uint32 oldMSTime = getMSTime();

        _playerXPperLevel.resize(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
        for (uint8 level = 0; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
            _playerXPperLevel[level] = 0;

        //                                                  0         1
        QueryResult result  = WorldDatabase.Query("SELECT Level, Experience FROM player_xp_for_level");

        if (!result)
        {
            TC_LOG_ERROR("server.loading", ">> Loaded 0 xp for level definitions. DB table `player_xp_for_level` is empty.");
            exit(1);
        }

        uint32 count = 0;

        do
        {
            Field* fields = result->Fetch();

            uint32 current_level = fields[0].GetUInt8();
            uint32 current_xp    = fields[1].GetUInt32();

            if (current_level >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
            {
                if (current_level > STRONG_MAX_LEVEL)        // hardcoded level maximum
                    TC_LOG_ERROR("sql.sql", "Wrong (> %u) level %u in `player_xp_for_level` table, ignoring.", STRONG_MAX_LEVEL, current_level);
                else
                {
                    TC_LOG_INFO("misc", "Unused (> MaxPlayerLevel in worldserver.conf) level %u in `player_xp_for_levels` table, ignoring.", current_level);
                    ++count;                                // make result loading percent "expected" correct in case disabled detail mode for example.
                }
                continue;
            }
            //PlayerXPperLevel
            _playerXPperLevel[current_level] = current_xp;
            ++count;
        }
        while (result->NextRow());

        // fill level gaps
        for (uint8 level = 1; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
        {
            if (_playerXPperLevel[level] == 0)
            {
                TC_LOG_ERROR("sql.sql", "Level %i does not have XP for level data. Using data of level [%i] + 100.", level + 1, level);
                _playerXPperLevel[level] = _playerXPperLevel[level - 1] + 100;
            }
        }

        TC_LOG_INFO("server.loading", ">> Loaded %u xp for level definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }
}

void ObjectMgr::GetPlayerClassLevelInfo(uint32 class_, uint8 level, uint32& baseHP, uint32& baseMana) const
{
    if (level < 1 || class_ >= MAX_CLASSES)
        return;

    if (level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        level = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);

    GtOCTBaseHPByClassEntry const* hp = sGtOCTBaseHPByClassStore.LookupEntry((class_-1) * GT_MAX_LEVEL + level-1);
    GtOCTBaseMPByClassEntry const* mp = sGtOCTBaseMPByClassStore.LookupEntry((class_-1) * GT_MAX_LEVEL + level-1);

    if (!hp || !mp)
    {
        TC_LOG_ERROR("misc", "Tried to get non-existant Class-Level combination data for base hp/mp. Class %u Level %u", class_, level);
        return;
    }

    baseHP = uint32(hp->ratio);
    baseMana = uint32(mp->ratio);
}

void ObjectMgr::GetPlayerLevelInfo(uint32 race, uint32 class_, uint8 level, PlayerLevelInfo* info) const
{
    if (level < 1 || race >= MAX_RACES || class_ >= MAX_CLASSES)
        return;

    PlayerInfo const* pInfo = _playerInfo[race][class_];
    if (!pInfo)
        return;

    if (level <= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        *info = pInfo->levelInfo[level-1];
    else
        BuildPlayerLevelInfo(race, class_, level, info);
}

void ObjectMgr::BuildPlayerLevelInfo(uint8 race, uint8 _class, uint8 level, PlayerLevelInfo* info) const
{
    // base data (last known level)
    *info = _playerInfo[race][_class]->levelInfo[sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)-1];

    // if conversion from uint32 to uint8 causes unexpected behaviour, change lvl to uint32
    for (uint8 lvl = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)-1; lvl < level; ++lvl)
    {
        switch (_class)
        {
            case CLASS_WARRIOR:
                info->stats[STAT_STRENGTH]  += (lvl > 23 ? 2: (lvl > 1  ? 1: 0));
                info->stats[STAT_STAMINA]   += (lvl > 23 ? 2: (lvl > 1  ? 1: 0));
                info->stats[STAT_AGILITY]   += (lvl > 36 ? 1: (lvl > 6 && (lvl%2) ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_SPIRIT]    += (lvl > 9 && !(lvl%2) ? 1: 0);
                break;
            case CLASS_PALADIN:
                info->stats[STAT_STRENGTH]  += (lvl > 3  ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 33 ? 2: (lvl > 1 ? 1: 0));
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 1: (lvl > 7 && !(lvl%2) ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 6 && (lvl%2) ? 1: 0);
                info->stats[STAT_SPIRIT]    += (lvl > 7 ? 1: 0);
                break;
            case CLASS_HUNTER:
                info->stats[STAT_STRENGTH]  += (lvl > 4  ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 4  ? 1: 0);
                info->stats[STAT_AGILITY]   += (lvl > 33 ? 2: (lvl > 1 ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 8 && (lvl%2) ? 1: 0);
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 1: (lvl > 9 && !(lvl%2) ? 1: 0));
                break;
            case CLASS_ROGUE:
                info->stats[STAT_STRENGTH]  += (lvl > 5  ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 4  ? 1: 0);
                info->stats[STAT_AGILITY]   += (lvl > 16 ? 2: (lvl > 1 ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 8 && !(lvl%2) ? 1: 0);
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 1: (lvl > 9 && !(lvl%2) ? 1: 0));
                break;
            case CLASS_PRIEST:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 5  ? 1: 0);
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 1: (lvl > 8 && (lvl%2) ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 22 ? 2: (lvl > 1 ? 1: 0));
                info->stats[STAT_SPIRIT]    += (lvl > 3  ? 1: 0);
                break;
            case CLASS_SHAMAN:
                info->stats[STAT_STRENGTH]  += (lvl > 34 ? 1: (lvl > 6 && (lvl%2) ? 1: 0));
                info->stats[STAT_STAMINA]   += (lvl > 4 ? 1: 0);
                info->stats[STAT_AGILITY]   += (lvl > 7 && !(lvl%2) ? 1: 0);
                info->stats[STAT_INTELLECT] += (lvl > 5 ? 1: 0);
                info->stats[STAT_SPIRIT]    += (lvl > 4 ? 1: 0);
                break;
            case CLASS_MAGE:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 5  ? 1: 0);
                info->stats[STAT_AGILITY]   += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_INTELLECT] += (lvl > 24 ? 2: (lvl > 1 ? 1: 0));
                info->stats[STAT_SPIRIT]    += (lvl > 33 ? 2: (lvl > 2 ? 1: 0));
                break;
            case CLASS_WARLOCK:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 38 ? 2: (lvl > 3 ? 1: 0));
                info->stats[STAT_AGILITY]   += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_INTELLECT] += (lvl > 33 ? 2: (lvl > 2 ? 1: 0));
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 2: (lvl > 3 ? 1: 0));
                break;
            case CLASS_DRUID:
                info->stats[STAT_STRENGTH]  += (lvl > 38 ? 2: (lvl > 6 && (lvl%2) ? 1: 0));
                info->stats[STAT_STAMINA]   += (lvl > 32 ? 2: (lvl > 4 ? 1: 0));
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 2: (lvl > 8 && (lvl%2) ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 38 ? 3: (lvl > 4 ? 1: 0));
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 3: (lvl > 5 ? 1: 0));
        }
    }
}

void ObjectMgr::LoadQuests()
{
    uint32 oldMSTime = getMSTime();

    _questTemplates.clear();

    _exclusiveQuestGroups.clear();

    QueryResult result = WorldDatabase.Query("SELECT "
        //0  1          2           3         4            5            6
        "ID, QuestType, QuestLevel, MinLevel, QuestSortID, QuestInfoID, SuggestedGroupNum, "
        //7                  8                   9                      10
        "RequiredFactionId1, RequiredFactionId2, RequiredFactionValue1, RequiredFactionValue2, "
        //11              12                  13           14                15                  16           17           18
        "RewardNextQuest, RewardXPDifficulty, RewardMoney, RewardBonusMoney, RewardDisplaySpell, RewardSpell, RewardHonor, RewardKillHonor, "
        //19        20     21                 22           23                   24
        "StartItem, Flags, MinimapTargetMark, RewardTitle, RequiredPlayerKills, RewardTalents, "
        //25                26             27                 28                    29                  30
        "RewardArenaPoints, RewardSkillId, RewardSkillPoints, RewardReputationMask, QuestGiverPortrait, QuestTurnInPortrait, "
        //31          32             33           34             35           36             37           38
        "RewardItem1, RewardAmount1, RewardItem2, RewardAmount2, RewardItem3, RewardAmount3, RewardItem4, RewardAmount4, "
        //39                  40                         41                   42                         43                   44                         45                   46                         47                   48                         49                   50
        "RewardChoiceItemID1, RewardChoiceItemQuantity1, RewardChoiceItemID2, RewardChoiceItemQuantity2, RewardChoiceItemID3, RewardChoiceItemQuantity3, RewardChoiceItemID4, RewardChoiceItemQuantity4, RewardChoiceItemID5, RewardChoiceItemQuantity5, RewardChoiceItemID6, RewardChoiceItemQuantity6, "
        //51               52                   53                      54                55                   56                      57                58                   59                      60                61                   62                      63                64                   65
        "RewardFactionID1, RewardFactionValue1, RewardFactionOverride1, RewardFactionID2, RewardFactionValue2, RewardFactionOverride2, RewardFactionID3, RewardFactionValue3, RewardFactionOverride3, RewardFactionID4, RewardFactionValue4, RewardFactionOverride4, RewardFactionID5, RewardFactionValue5, RewardFactionOverride5, "
        //66           67    68    69           70        71              72                73               74
        "POIContinent, POIx, POIy, POIPriority, LogTitle, LogDescription, QuestDescription, AreaDescription, QuestCompletionLog, "
        //75               76                77                78                79                     80                     81                     82
        "RequiredNpcOrGo1, RequiredNpcOrGo2, RequiredNpcOrGo3, RequiredNpcOrGo4, RequiredNpcOrGoCount1, RequiredNpcOrGoCount2, RequiredNpcOrGoCount3, RequiredNpcOrGoCount4, "
        //83        84         85         86         87                 88                 89                 90
        "ItemDrop1, ItemDrop2, ItemDrop3, ItemDrop4, ItemDropQuantity1, ItemDropQuantity2, ItemDropQuantity3, ItemDropQuantity4, "
        //91              92               93               94               95               96               97                  98                  99                  100                 101                 102
        "RequiredItemId1, RequiredItemId2, RequiredItemId3, RequiredItemId4, RequiredItemId5, RequiredItemId6, RequiredItemCount1, RequiredItemCount2, RequiredItemCount3, RequiredItemCount4, RequiredItemCount5, RequiredItemCount6, "
        //103           104             105             106             107              108                109                110                111                112                   113                   114                   115
        "RequiredSpell, ObjectiveText1, ObjectiveText2, ObjectiveText3, ObjectiveText4,  RewardCurrencyId1, RewardCurrencyId2, RewardCurrencyId3, RewardCurrencyId4, RewardCurrencyCount1, RewardCurrencyCount2, RewardCurrencyCount3, RewardCurrencyCount4, "
        //116                 117                  118                  119                  120                     121                     122                    123
        "RequiredCurrencyId1, RequiredCurrencyId2, RequiredCurrencyId3, RequiredCurrencyId4, RequiredCurrencyCount1, RequiredCurrencyCount2, RequiredCurrencyCount3, RequiredCurrencyCount4, "
        //124                  125                   126                  127                  128          129
        "QuestGiverTextWindow, QuestGiverTargetName, QuestTurnTextWindow, QuestTurnTargetName, SoundAccept, SoundTurnIn"
        " FROM quest_template");
    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 quests definitions. DB table `quest_template` is empty.");
        return;
    }

    _questTemplates.reserve(result->GetRowCount());

    // create multimap previous quest for each existed quest
    // some quests can have many previous maps set by NextQuestId in previous quest
    // for example set of race quests can lead to single not race specific quest
    do
    {
        Field* fields = result->Fetch();

        uint32 questId = fields[0].GetUInt32();
        _questTemplates.emplace(std::piecewise_construct, std::forward_as_tuple(questId), std::forward_as_tuple(fields));
    } while (result->NextRow());

    std::unordered_map<uint32, uint32> usedMailTemplates;

    struct QuestLoaderHelper
    {
        typedef void(Quest::* QuestLoaderFunction)(Field* fields);

        char const* QueryFields;
        char const* TableName;
        char const* TableDesc;
        QuestLoaderFunction LoaderFunction;
    };

    static std::vector<QuestLoaderHelper> const QuestLoaderHelpers =
    {
        // 0   1       2       3       4       5            6            7            8
        { "ID, Emote1, Emote2, Emote3, Emote4, EmoteDelay1, EmoteDelay2, EmoteDelay3, EmoteDelay4",                                                                       "quest_details",        "details",             &Quest::LoadQuestDetails       },

        // 0   1                2                  3
        { "ID, EmoteOnComplete, EmoteOnIncomplete, CompletionText",                                                                                                       "quest_request_items",  "request items",       &Quest::LoadQuestRequestItems  },

        // 0   1       2       3       4       5            6            7            8            9
        { "ID, Emote1, Emote2, Emote3, Emote4, EmoteDelay1, EmoteDelay2, EmoteDelay3, EmoteDelay4, RewardText",                                                           "quest_offer_reward",   "reward emotes",       &Quest::LoadQuestOfferReward   },

        // 0   1         2                 3              4            5            6               7                     8                     9
        { "ID, MaxLevel, AllowableClasses, SourceSpellID, PrevQuestID, NextQuestID, ExclusiveGroup, BreadcrumbForQuestId, RewardMailTemplateID, RewardMailDelay,"
        // 10              11                   12                     13                     14                   15                   16                 17            18              19                 20
        " RequiredSkillID, RequiredSkillPoints, RequiredMinRepFaction, RequiredMaxRepFaction, RequiredMinRepValue, RequiredMaxRepValue, ProvidedItemCount, SpecialFlags, AllowableRaces, TimeAllowed, AbandonSpellID", "quest_template_addon", "template addons",     &Quest::LoadQuestTemplateAddon },

        // 0        1
        { "QuestId, RewardMailSenderEntry",                                                                                                                               "quest_mail_sender",    "mail sender entries", &Quest::LoadQuestMailSender    }
    };

    for (QuestLoaderHelper const& loader : QuestLoaderHelpers)
    {
        QueryResult result = WorldDatabase.PQuery("SELECT %s FROM %s", loader.QueryFields, loader.TableName);

        if (!result)
            TC_LOG_ERROR("server.loading", ">> Loaded 0 quest %s. DB table `%s` is empty.", loader.TableDesc, loader.TableName);
        else
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 questId = fields[0].GetUInt32();

                auto itr = _questTemplates.find(questId);
                if (itr != _questTemplates.end())
                    (itr->second.*loader.LoaderFunction)(fields);
                else
                    TC_LOG_ERROR("server.loading", "Table `%s` has data for quest %u but such quest does not exist", loader.TableName, questId);
            } while (result->NextRow());
        }
    }

    // Post processing
    for (auto& questPair : _questTemplates)
    {
        // skip post-loading checks for disabled quests
        if (DisableMgr::IsDisabledFor(DISABLE_TYPE_QUEST, questPair.first, nullptr))
            continue;

        Quest* qinfo = &questPair.second;

        // additional quest integrity checks (GO, creature_template and item_template must be loaded already)

        if (qinfo->GetQuestType() >= MAX_DB_ALLOWED_QUEST_TYPES)
            TC_LOG_ERROR("sql.sql", "Quest %u has `Method` = %u, expected values are 0, 1 or 2.", qinfo->GetQuestId(), qinfo->GetQuestType());

        if (qinfo->_specialFlags & ~QUEST_SPECIAL_FLAGS_DB_ALLOWED)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `SpecialFlags` = %u > max allowed value. Correct `SpecialFlags` to value <= %u",
                qinfo->GetQuestId(), qinfo->_specialFlags, QUEST_SPECIAL_FLAGS_DB_ALLOWED);
            qinfo->_specialFlags &= QUEST_SPECIAL_FLAGS_DB_ALLOWED;
        }

        if (qinfo->_flags & QUEST_FLAGS_DAILY && qinfo->_flags & QUEST_FLAGS_WEEKLY)
        {
            TC_LOG_ERROR("sql.sql", "Weekly Quest %u is marked as daily quest in `Flags`, removed daily flag.", qinfo->GetQuestId());
            qinfo->_flags &= ~QUEST_FLAGS_DAILY;
        }

        if (qinfo->_flags & QUEST_FLAGS_DAILY)
        {
            if (!(qinfo->_specialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                TC_LOG_ERROR("sql.sql", "Daily Quest %u not marked as repeatable in `SpecialFlags`, added.", qinfo->GetQuestId());
                qinfo->_specialFlags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }

        if (qinfo->_flags & QUEST_FLAGS_WEEKLY)
        {
            if (!(qinfo->_specialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                TC_LOG_ERROR("sql.sql", "Weekly Quest %u not marked as repeatable in `SpecialFlags`, added.", qinfo->GetQuestId());
                qinfo->_specialFlags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }

        if (qinfo->_specialFlags & QUEST_SPECIAL_FLAGS_MONTHLY)
        {
            if (!(qinfo->_specialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                TC_LOG_ERROR("sql.sql", "Monthly quest %u not marked as repeatable in `SpecialFlags`, added.", qinfo->GetQuestId());
                qinfo->_specialFlags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }

        if (qinfo->_flags & QUEST_FLAGS_TRACKING_EVENT)
        {
            // at auto-reward can be rewarded only RewardChoiceItemId[0]
            for (uint32 j = 1; j < QUEST_REWARD_CHOICES_COUNT; ++j)
            {
                if (uint32 id = qinfo->RewardChoiceItemId[j])
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RewardChoiceItemId%d` = %u but item from `RewardChoiceItemId%d` can't be rewarded with quest flag QUEST_FLAGS_TRACKING.",
                        qinfo->GetQuestId(), j + 1, id, j + 1);
                    // no changes, quest ignore this data
                }
            }
        }

        if (qinfo->_minLevel == uint32(-1) || qinfo->_minLevel > DEFAULT_MAX_LEVEL)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u should be disabled because `MinLevel` = %i", qinfo->GetQuestId(), int32(qinfo->_minLevel));
            // no changes needed, sending -1 in SMSG_QUEST_QUERY_RESPONSE is valid
        }

        // client quest log visual (area case)
        if (qinfo->_zoneOrSort > 0)
        {
            if (!sAreaTableStore.LookupEntry(qinfo->_zoneOrSort))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `ZoneOrSort` = %u (zone case) but zone with this id does not exist.",
                    qinfo->GetQuestId(), qinfo->_zoneOrSort);
                // no changes, quest not dependent from this value but can have problems at client
            }
        }
        // client quest log visual (sort case)
        if (qinfo->_zoneOrSort < 0)
        {
            QuestSortEntry const* qSort = sQuestSortStore.LookupEntry(-int32(qinfo->_zoneOrSort));
            if (!qSort)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `ZoneOrSort` = %i (sort case) but quest sort with this id does not exist.",
                    qinfo->GetQuestId(), qinfo->_zoneOrSort);
                // no changes, quest not dependent from this value but can have problems at client (note some may be 0, we must allow this so no check)
            }
            //check for proper RequiredSkillId value (skill case)
            if (uint32 skill_id = SkillByQuestSort(-qinfo->_zoneOrSort))
            {
                if (qinfo->_requiredSkillId != skill_id)
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `ZoneOrSort` = %i but `RequiredSkillId` does not have a corresponding value (%d).",
                        qinfo->GetQuestId(), qinfo->_zoneOrSort, skill_id);
                    //override, and force proper value here?
                }
            }
        }

        // AllowableClasses, can be 0/CLASSMASK_ALL_PLAYABLE to allow any class
        if (qinfo->_allowableClasses)
        {
            if (!(qinfo->_allowableClasses & CLASSMASK_ALL_PLAYABLE))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u does not contain any playable classes in `RequiredClasses` (%u), value set to 0 (all classes).", qinfo->GetQuestId(), qinfo->_allowableClasses);
                    qinfo->_allowableClasses = 0;
            }
        }

        // AllowableRaces, can be 0/RACEMASK_ALL_PLAYABLE to allow any race
        if (qinfo->_allowableRaces)
        {
            if (!(qinfo->_allowableRaces & RACEMASK_ALL_PLAYABLE))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u does not contain any playable races in `AllowableRaces` (%u), value set to 0 (all races).", qinfo->GetQuestId(), qinfo->_allowableRaces);
                qinfo->_allowableRaces = 0;
            }
        }

        // RequiredSkillId, can be 0
        if (qinfo->_requiredSkillId)
        {
            if (!sSkillLineStore.LookupEntry(qinfo->_requiredSkillId))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredSkillId` = %u but this skill does not exist",
                    qinfo->GetQuestId(), qinfo->_requiredSkillId);
            }
        }

        if (qinfo->_requiredSkillPoints)
        {
            if (qinfo->_requiredSkillPoints > sWorld->GetConfigMaxSkillValue())
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredSkillPoints` = %u but max possible skill is %u, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->_requiredSkillPoints, sWorld->GetConfigMaxSkillValue());
                // no changes, quest can't be done for this requirement
            }
        }
        // else Skill quests can have 0 skill level, this is ok

        if (qinfo->_requiredFactionId2 && !sFactionStore.LookupEntry(qinfo->_requiredFactionId2))
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredFactionId2` = %u but faction template %u does not exist, quest can't be done.",
                qinfo->GetQuestId(), qinfo->_requiredFactionId2, qinfo->_requiredFactionId2);
            // no changes, quest can't be done for this requirement
        }

        if (qinfo->_requiredFactionId1 && !sFactionStore.LookupEntry(qinfo->_requiredFactionId1))
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredFactionId1` = %u but faction template %u does not exist, quest can't be done.",
                qinfo->GetQuestId(), qinfo->_requiredFactionId1, qinfo->_requiredFactionId1);
            // no changes, quest can't be done for this requirement
        }

        if (qinfo->_requiredMinRepFaction && !sFactionStore.LookupEntry(qinfo->_requiredMinRepFaction))
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMinRepFaction` = %u but faction template %u does not exist, quest can't be done.",
                qinfo->GetQuestId(), qinfo->_requiredMinRepFaction, qinfo->_requiredMinRepFaction);
            // no changes, quest can't be done for this requirement
        }

        if (qinfo->_requiredMaxRepFaction && !sFactionStore.LookupEntry(qinfo->_requiredMaxRepFaction))
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMaxRepFaction` = %u but faction template %u does not exist, quest can't be done.",
                qinfo->GetQuestId(), qinfo->_requiredMaxRepFaction, qinfo->_requiredMaxRepFaction);
            // no changes, quest can't be done for this requirement
        }

        if (qinfo->_requiredMinRepValue && qinfo->_requiredMinRepValue > ReputationMgr::Reputation_Cap)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMinRepValue` = %d but max reputation is %u, quest can't be done.",
                qinfo->GetQuestId(), qinfo->_requiredMinRepValue, ReputationMgr::Reputation_Cap);
            // no changes, quest can't be done for this requirement
        }

        if (qinfo->_requiredMinRepValue && qinfo->_requiredMaxRepValue && qinfo->_requiredMaxRepValue <= qinfo->_requiredMinRepValue)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMaxRepValue` = %d and `RequiredMinRepValue` = %d, quest can't be done.",
                qinfo->GetQuestId(), qinfo->_requiredMaxRepValue, qinfo->_requiredMinRepValue);
            // no changes, quest can't be done for this requirement
        }

        if (!qinfo->_requiredFactionId1 && qinfo->_requiredFactionId1 != 0)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredFactionValue1` = %d but `RequiredFactionId1` is 0, value has no effect",
                qinfo->GetQuestId(), qinfo->_requiredFactionId1);
            // warning
        }

        if (!qinfo->_requiredFactionId2 && qinfo->_requiredFactionId2 != 0)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredFactionValue2` = %d but `RequiredFactionId2` is 0, value has no effect",
                qinfo->GetQuestId(), qinfo->_requiredFactionId2);
            // warning
        }

        if (!qinfo->_requiredMinRepFaction && qinfo->_requiredMinRepFaction != 0)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMinRepValue` = %d but `RequiredMinRepFaction` is 0, value has no effect",
                qinfo->GetQuestId(), qinfo->_requiredMinRepFaction);
            // warning
        }

        if (!qinfo->_requiredMaxRepFaction && qinfo->_requiredMaxRepFaction != 0)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMaxRepValue` = %d but `RequiredMaxRepFaction` is 0, value has no effect",
                qinfo->GetQuestId(), qinfo->_requiredMaxRepFaction);
            // warning
        }

        if (qinfo->_rewardTitleId && !sCharTitlesStore.LookupEntry(qinfo->_rewardTitleId))
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RewardTitleId` = %u but CharTitle Id %u does not exist, quest can't be rewarded with title.",
                qinfo->GetQuestId(), qinfo->GetCharTitleId(), qinfo->GetCharTitleId());
            qinfo->_rewardTitleId = 0;
            // quest can't reward this title
        }

        if (qinfo->_startItem)
        {
            if (!sObjectMgr->GetItemTemplate(qinfo->_startItem))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `StartItem` = %u but item with entry %u does not exist, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->_startItem, qinfo->_startItem);
                qinfo->_startItem = 0;                       // quest can't be done for this requirement
            }
            else if (qinfo->_startItemCount == 0)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `StartItem` = %u but `ProvidedItemCount` = 0, set to 1 but need fix in DB.",
                    qinfo->GetQuestId(), qinfo->_startItem);
                qinfo->_startItemCount = 1;                    // update to 1 for allow quest work for backward compatibility with DB
            }
        }
        else if (qinfo->_startItemCount > 0)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `StartItem` = 0 but `ProvidedItemCount` = %u, useless value.",
                qinfo->GetQuestId(), qinfo->_startItemCount);
            qinfo->_startItemCount = 0;                          // no quest work changes in fact
        }

        if (qinfo->_sourceSpellId)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(qinfo->_sourceSpellId);
            if (!spellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `SourceSpellid` = %u but spell %u doesn't exist, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->_sourceSpellId, qinfo->_sourceSpellId);
                qinfo->_sourceSpellId = 0;                        // quest can't be done for this requirement
            }
            else if (!SpellMgr::IsSpellValid(spellInfo))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `SourceSpellid` = %u but spell %u is broken, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->_sourceSpellId, qinfo->_sourceSpellId);
                qinfo->_sourceSpellId = 0;                        // quest can't be done for this requirement
            }
        }

        for (uint8 j = 0; j < QUEST_ITEM_OBJECTIVES_COUNT; ++j)
        {
            uint32 id = qinfo->RequiredItemId[j];
            if (id)
            {
                if (qinfo->RequiredItemCount[j] == 0)
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredItemId%d` = %u but `RequiredItemCount%d` = 0, quest can't be done.",
                        qinfo->GetQuestId(), j+1, id, j+1);
                    // no changes, quest can't be done for this requirement
                }

                qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);

                if (!sObjectMgr->GetItemTemplate(id))
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredItemId%d` = %u but item with entry %u does not exist, quest can't be done.",
                        qinfo->GetQuestId(), j+1, id, id);
                    qinfo->RequiredItemCount[j] = 0;             // prevent incorrect work of quest
                }
            }
            else if (qinfo->RequiredItemCount[j] > 0)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredItemId%d` = 0 but `RequiredItemCount%d` = %u, quest can't be done.",
                    qinfo->GetQuestId(), j+1, j+1, qinfo->RequiredItemCount[j]);
                qinfo->RequiredItemCount[j] = 0;                 // prevent incorrect work of quest
            }
        }

        for (uint8 j = 0; j < QUEST_SOURCE_ITEM_IDS_COUNT; ++j)
        {
            uint32 id = qinfo->ItemDrop[j];
            if (id)
            {
                if (!sObjectMgr->GetItemTemplate(id))
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `ItemDrop%d` = %u but item with entry %u does not exist, quest can't be done.",
                        qinfo->GetQuestId(), j+1, id, id);
                    // no changes, quest can't be done for this requirement
                }
            }
            else
            {
                if (qinfo->ItemDropQuantity[j]>0)
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `ItemDrop%d` = 0 but `ItemDropQuantity%d` = %u.",
                        qinfo->GetQuestId(), j+1, j+1, qinfo->ItemDropQuantity[j]);
                    // no changes, quest ignore this data
                }
            }
        }

        for (uint8 j = 0; j < QUEST_OBJECTIVES_COUNT; ++j)
        {
            int32 id = qinfo->RequiredNpcOrGo[j];
            if (id < 0 && !sObjectMgr->GetGameObjectTemplate(-id))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredNpcOrGo%d` = %i but gameobject %u does not exist, quest can't be done.",
                    qinfo->GetQuestId(), j+1, id, uint32(-id));
                qinfo->RequiredNpcOrGo[j] = 0;            // quest can't be done for this requirement
            }

            if (id > 0 && !sObjectMgr->GetCreatureTemplate(id))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredNpcOrGo%d` = %i but creature with entry %u does not exist, quest can't be done.",
                    qinfo->GetQuestId(), j+1, id, uint32(id));
                qinfo->RequiredNpcOrGo[j] = 0;            // quest can't be done for this requirement
            }

            if (id)
            {
                // In fact SpeakTo and Kill are quite same: either you can speak to mob:SpeakTo or you can't:Kill/Cast

                qinfo->SetSpecialFlag(QuestSpecialFlags(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_SPEAKTO));

                if (!qinfo->RequiredNpcOrGoCount[j])
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredNpcOrGo%d` = %u but `RequiredNpcOrGoCount%d` = 0, quest can't be done.",
                        qinfo->GetQuestId(), j+1, id, j+1);
                    // no changes, quest can be incorrectly done, but we already report this
                }
            }
            else if (qinfo->RequiredNpcOrGoCount[j]>0)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredNpcOrGo%d` = 0 but `RequiredNpcOrGoCount%d` = %u.",
                    qinfo->GetQuestId(), j+1, j+1, qinfo->RequiredNpcOrGoCount[j]);
                // no changes, quest ignore this data
            }
        }

        for (uint8 j = 0; j < QUEST_REWARD_CHOICES_COUNT; ++j)
        {
            uint32 id = qinfo->RewardChoiceItemId[j];
            if (id)
            {
                if (!sObjectMgr->GetItemTemplate(id))
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RewardChoiceItemId%d` = %u but item with entry %u does not exist, quest will not reward this item.",
                        qinfo->GetQuestId(), j+1, id, id);
                    qinfo->RewardChoiceItemId[j] = 0;          // no changes, quest will not reward this
                }

                if (!qinfo->RewardChoiceItemCount[j])
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RewardChoiceItemId%d` = %u but `RewardChoiceItemCount%d` = 0, quest can't be done.",
                        qinfo->GetQuestId(), j+1, id, j+1);
                    // no changes, quest can't be done
                }
            }
            else if (qinfo->RewardChoiceItemCount[j]>0)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardChoiceItemId%d` = 0 but `RewardChoiceItemCount%d` = %u.",
                    qinfo->GetQuestId(), j+1, j+1, qinfo->RewardChoiceItemCount[j]);
                // no changes, quest ignore this data
            }
        }

        for (uint8 j = 0; j < QUEST_REWARDS_COUNT; ++j)
        {
            uint32 id = qinfo->RewardItemId[j];
            if (id)
            {
                if (!sObjectMgr->GetItemTemplate(id))
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RewardItemId%d` = %u but item with entry %u does not exist, quest will not reward this item.",
                        qinfo->GetQuestId(), j+1, id, id);
                    qinfo->RewardItemId[j] = 0;                // no changes, quest will not reward this item
                }

                if (!qinfo->RewardItemIdCount[j])
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RewardItemId%d` = %u but `RewardItemIdCount%d` = 0, quest will not reward this item.",
                        qinfo->GetQuestId(), j+1, id, j+1);
                    // no changes
                }
            }
            else if (qinfo->RewardItemIdCount[j]>0)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardItemId%d` = 0 but `RewardItemIdCount%d` = %u.",
                    qinfo->GetQuestId(), j+1, j+1, qinfo->RewardItemIdCount[j]);
                // no changes, quest ignore this data
            }
        }

        for (uint8 j = 0; j < QUEST_REPUTATIONS_COUNT; ++j)
        {
            if (qinfo->RewardFactionId[j])
            {
                if (std::abs(qinfo->RewardFactionValueId[j]) > 9)
                {
               TC_LOG_ERROR("sql.sql", "Quest %u has RewardFactionValueId%d = %i. That is outside the range of valid values (-9 to 9).", qinfo->GetQuestId(), j+1, qinfo->RewardFactionValueId[j]);
                }
                if (!sFactionStore.LookupEntry(qinfo->RewardFactionId[j]))
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RewardFactionId%d` = %u but raw faction (faction.dbc) %u does not exist, quest will not reward reputation for this faction.", qinfo->GetQuestId(), j+1, qinfo->RewardFactionId[j], qinfo->RewardFactionId[j]);
                    qinfo->RewardFactionId[j] = 0;            // quest will not reward this
                }
            }

            else if (qinfo->RewardFactionValueIdOverride[j] != 0)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardFactionId%d` = 0 but `RewardFactionValueIdOverride%d` = %i.",
                    qinfo->GetQuestId(), j+1, j+1, qinfo->RewardFactionValueIdOverride[j]);
                // no changes, quest ignore this data
            }
        }

        if (qinfo->_rewardDisplaySpell)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(qinfo->_rewardDisplaySpell);

            if (!spellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardDisplaySpell` = %u but spell %u does not exist, spell removed as display reward.",
                    qinfo->GetQuestId(), qinfo->_rewardDisplaySpell, qinfo->_rewardDisplaySpell);
                qinfo->_rewardDisplaySpell = 0;                        // no spell reward will display for this quest
            }

            else if (!SpellMgr::IsSpellValid(spellInfo))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardDisplaySpell` = %u but spell %u is broken, quest will not have a spell reward.",
                    qinfo->GetQuestId(), qinfo->_rewardDisplaySpell, qinfo->_rewardDisplaySpell);
                qinfo->_rewardDisplaySpell = 0;                        // no spell reward will display for this quest
            }

            else if (sDBCManager.GetTalentSpellCost(qinfo->_rewardDisplaySpell))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardDisplaySpell` = %u but spell %u is talent, quest will not have a spell reward.",
                    qinfo->GetQuestId(), qinfo->_rewardDisplaySpell, qinfo->_rewardDisplaySpell);
                qinfo->_rewardDisplaySpell = 0;                        // no spell reward will display for this quest
            }
        }

        if (qinfo->_rewardSpell > 0)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(qinfo->_rewardSpell);

            if (!spellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardSpell` = %u but spell %u does not exist, quest will not have a spell reward.",
                    qinfo->GetQuestId(), qinfo->_rewardSpell, qinfo->_rewardSpell);
                qinfo->_rewardSpell = 0;                    // no spell will be cast on player
            }

            else if (!SpellMgr::IsSpellValid(spellInfo))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardSpell` = %u but spell %u is broken, quest will not have a spell reward.",
                    qinfo->GetQuestId(), qinfo->_rewardSpell, qinfo->_rewardSpell);
                qinfo->_rewardSpell = 0;                    // no spell will be cast on player
            }

            else if (sDBCManager.GetTalentSpellCost(qinfo->_rewardSpell))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardDisplaySpell` = %u but spell %u is talent, quest will not have a spell reward.",
                    qinfo->GetQuestId(), qinfo->_rewardSpell, qinfo->_rewardSpell);
                qinfo->_rewardSpell = 0;                    // no spell will be cast on player
            }
        }

        if (qinfo->_rewardMailTemplateId)
        {
            if (!sMailTemplateStore.LookupEntry(qinfo->_rewardMailTemplateId))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardMailTemplateId` = %u but mail template  %u does not exist, quest will not have a mail reward.",
                    qinfo->GetQuestId(), qinfo->_rewardMailTemplateId, qinfo->_rewardMailTemplateId);
                qinfo->_rewardMailTemplateId = 0;               // no mail will send to player
                qinfo->_rewardMailDelay = 0;                // no mail will send to player
                qinfo->_rewardMailSenderEntry = 0;
            }
            else if (usedMailTemplates.find(qinfo->_rewardMailTemplateId) != usedMailTemplates.end())
            {
                auto used_mt_itr = usedMailTemplates.find(qinfo->_rewardMailTemplateId);
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardMailTemplateId` = %u but mail template  %u already used for quest %u, quest will not have a mail reward.",
                    qinfo->GetQuestId(), qinfo->_rewardMailTemplateId, qinfo->_rewardMailTemplateId, used_mt_itr->second);
                qinfo->_rewardMailTemplateId = 0;               // no mail will send to player
                qinfo->_rewardMailDelay = 0;                // no mail will send to player
                qinfo->_rewardMailSenderEntry = 0;
            }
            else
                usedMailTemplates[qinfo->_rewardMailTemplateId] = qinfo->GetQuestId();
        }

        if (uint32 rewardNextQuest = qinfo->_rewardNextQuest)
        {
            if (!_questTemplates.count(rewardNextQuest))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardNextQuest` = %u but quest %u does not exist, quest chain will not work.",
                    qinfo->GetQuestId(), qinfo->_rewardNextQuest, qinfo->_rewardNextQuest);
                qinfo->_rewardNextQuest = 0;
            }
        }

        for (uint8 j = 0; j < QUEST_REWARD_CURRENCY_COUNT; ++j)
        {
            if (qinfo->RewardCurrencyId[j])
            {
                if (qinfo->RewardCurrencyCount[j] == 0)
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RewardCurrencyId%d` = %u but `RewardCurrencyCount%d` = 0, quest can't be done.",
                        qinfo->GetQuestId(), j+1, qinfo->RewardCurrencyId[j], j+1);
                    // no changes, quest can't be done for this requirement
                }

                if (!sCurrencyTypesStore.LookupEntry(qinfo->RewardCurrencyId[j]))
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RewardCurrencyId%d` = %u but currency with entry %u does not exist, quest can't be done.",
                        qinfo->GetQuestId(), j+1, qinfo->RewardCurrencyId[j], qinfo->RewardCurrencyId[j]);
                    qinfo->RewardCurrencyCount[j] = 0;             // prevent incorrect work of quest
                }
            }
            else if (qinfo->RewardCurrencyCount[j] > 0)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardCurrencyId%d` = 0 but `RewardCurrencyCount%d` = %u, quest can't be done.",
                    qinfo->GetQuestId(), j+1, j+1, qinfo->RewardCurrencyCount[j]);
                qinfo->RewardCurrencyCount[j] = 0;                 // prevent incorrect work of quest
            }
        }

        for (uint8 j = 0; j < QUEST_REQUIRED_CURRENCY_COUNT; ++j)
        {
            if (qinfo->RequiredCurrencyId[j])
            {
                if (qinfo->RequiredCurrencyCount[j] == 0)
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredCurrencyId%d` = %u but `RequiredCurrencyCount%d` = 0, quest can't be done.",
                        qinfo->GetQuestId(), j+1, qinfo->RequiredCurrencyId[j], j+1);
                    // no changes, quest can't be done for this requirement
                }

                if (!sCurrencyTypesStore.LookupEntry(qinfo->RequiredCurrencyId[j]))
                {
                    TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredCurrencyId%d` = %u but currency with entry %u does not exist, quest can't be done.",
                        qinfo->GetQuestId(), j+1, qinfo->RequiredCurrencyId[j], qinfo->RequiredCurrencyId[j]);
                    qinfo->RequiredCurrencyCount[j] = 0;             // prevent incorrect work of quest
                }
            }
            else if (qinfo->RequiredCurrencyCount[j] > 0)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredCurrencyId%d` = 0 but `RequiredCurrencyCount%d` = %u, quest can't be done.",
                    qinfo->GetQuestId(), j+1, j+1, qinfo->RequiredCurrencyCount[j]);
                qinfo->RequiredCurrencyCount[j] = 0;                 // prevent incorrect work of quest
            }
        }

        if (qinfo->_soundAccept)
        {
            if (!sSoundEntriesStore.LookupEntry(qinfo->_soundAccept))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `SoundAccept` = %u but sound %u does not exist, set to 0.",
                    qinfo->GetQuestId(), qinfo->_soundAccept, qinfo->_soundAccept);
                qinfo->_soundAccept = 0;                        // no sound will be played
            }
        }

        if (qinfo->_soundTurnIn)
        {
            if (!sSoundEntriesStore.LookupEntry(qinfo->_soundTurnIn))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `SoundTurnIn` = %u but sound %u does not exist, set to 0.",
                    qinfo->GetQuestId(), qinfo->_soundTurnIn, qinfo->_soundTurnIn);
                qinfo->_soundTurnIn = 0;                        // no sound will be played
            }
        }

        if (qinfo->_requiredSpell > 0)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(qinfo->_requiredSpell);

            if (!spellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredSpell` = %u but spell %u does not exist, quest will not require a spell.",
                    qinfo->GetQuestId(), qinfo->_requiredSpell, qinfo->_requiredSpell);
                qinfo->_requiredSpell = 0;                    // no spell will be required
            }

            else if (!SpellMgr::IsSpellValid(spellInfo))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredSpell` = %u but spell %u is broken, quest will not require a spell.",
                    qinfo->GetQuestId(), qinfo->_requiredSpell, qinfo->_requiredSpell);
                qinfo->_requiredSpell = 0;                    // no spell will be required
            }
        }

        if (qinfo->_rewardSkillId)
        {
            if (!sSkillLineStore.LookupEntry(qinfo->_rewardSkillId))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardSkillId` = %u but this skill does not exist",
                    qinfo->GetQuestId(), qinfo->_rewardSkillId);
            }
            if (!qinfo->_rewardSkillPoints)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardSkillId` = %u but `RewardSkillPoints` is 0",
                    qinfo->GetQuestId(), qinfo->_rewardSkillId);
            }
        }

        if (qinfo->_rewardSkillPoints)
        {
            if (qinfo->_rewardSkillPoints > sWorld->GetConfigMaxSkillValue())
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardSkillPoints` = %u but max possible skill is %u, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->_rewardSkillPoints, sWorld->GetConfigMaxSkillValue());
                // no changes, quest can't be done for this requirement
            }
            if (!qinfo->_rewardSkillId)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardSkillPoints` = %u but `RewardSkillId` is 0",
                    qinfo->GetQuestId(), qinfo->_rewardSkillPoints);
            }
        }

        if (qinfo->_questAbandonSpell)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(qinfo->_questAbandonSpell);
            if (!spellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `QuestAbandonSpell` = %u but spell %u doesn't exist, quest abandon Spell can't be work.",
                    qinfo->GetQuestId(), qinfo->_questAbandonSpell, qinfo->_questAbandonSpell);
                qinfo->_questAbandonSpell = 0;                        // quest can't be done for this requirement
            }
            else if (!SpellMgr::IsSpellValid(spellInfo))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `QuestAbandonSpell` = %u but spell %u is broken, quest abandon Spell can't be work",
                    qinfo->GetQuestId(), qinfo->_questAbandonSpell, qinfo->_questAbandonSpell);
                qinfo->_questAbandonSpell = 0;                        // quest can't be done for this requirement
            }
        }

        // fill additional data stores
        if (uint32 prevQuestId = std::abs(qinfo->_prevQuestId))
        {
            auto prevQuestItr = _questTemplates.find(prevQuestId);
            if (prevQuestItr == _questTemplates.end())
                TC_LOG_ERROR("sql.sql", "Quest %u has PrevQuestId %i, but no such quest", qinfo->GetQuestId(), qinfo->_prevQuestId);
            else if (prevQuestItr->second._breadcrumbForQuestId)
                TC_LOG_ERROR("sql.sql", "Quest %u should not be unlocked by breadcrumb quest %u", qinfo->_id, prevQuestId);
            else if (qinfo->_prevQuestId > 0)
                qinfo->DependentPreviousQuests.push_back(prevQuestId);
        }

        if (uint32 nextQuestId = qinfo->_nextQuestId)
        {
            auto nextQuestItr = _questTemplates.find(nextQuestId);
            if (nextQuestItr == _questTemplates.end())
                TC_LOG_ERROR("sql.sql", "Quest %u has NextQuestId %u, but no such quest", qinfo->GetQuestId(), qinfo->_nextQuestId);
            else
                nextQuestItr->second.DependentPreviousQuests.push_back(qinfo->GetQuestId());
        }

        if (uint32 breadcrumbForQuestId = std::abs(qinfo->_breadcrumbForQuestId))
        {
            if (_questTemplates.find(breadcrumbForQuestId) == _questTemplates.end())
            {
                TC_LOG_ERROR("sql.sql", "Quest %u is a breadcrumb for quest %u, but no such quest exists", qinfo->_id, breadcrumbForQuestId);
                qinfo->_breadcrumbForQuestId = 0;
            }
            if (qinfo->_nextQuestId)
                TC_LOG_ERROR("sql.sql", "Quest %u is a breadcrumb, should not unlock quest %u", qinfo->_id, qinfo->_nextQuestId);
        }

        if (qinfo->_exclusiveGroup)
            _exclusiveQuestGroups.emplace(qinfo->_exclusiveGroup, qinfo->GetQuestId());
        if (qinfo->_timeAllowed)
            qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_TIMED);
        if (qinfo->_requiredPlayerKills)
            qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_PLAYER_KILL);

        // Special flag to determine if quest is completed from the start, used to determine if we can fail timed quest if it is completed
        if (!qinfo->HasSpecialFlag(QuestSpecialFlags(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_SPEAKTO)) &&
            !qinfo->HasFlag(QUEST_FLAGS_COMPLETION_AREA_TRIGGER | QUEST_FLAGS_COMPLETION_EVENT))
        {
            bool addFlag = true;
            if (qinfo->HasSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER))
            {
                for (uint8 j = 0; j < QUEST_ITEM_OBJECTIVES_COUNT; ++j)
                {
                    if (qinfo->RequiredItemId[j] != 0 && (qinfo->RequiredItemId[j] != qinfo->GetSrcItemId() || qinfo->RequiredItemCount[j] > qinfo->GetSrcItemCount()))
                    {
                        addFlag = false;
                        break;
                    }
                }
            }

            if (addFlag)
                qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_COMPLETED_AT_START);
        }
    }

    // Disallow any breadcrumb loops and inform quests of their breadcrumbs
    for (auto& questPair : _questTemplates)
    {
        // skip post-loading checks for disabled quests
        if (DisableMgr::IsDisabledFor(DISABLE_TYPE_QUEST, questPair.first, nullptr))
            continue;

        Quest* qinfo = &questPair.second;
        uint32   qid = qinfo->GetQuestId();
        uint32 breadcrumbForQuestId = std::abs(qinfo->_breadcrumbForQuestId);
        std::set<uint32> questSet;

        while (breadcrumbForQuestId)
        {
            //a previously visited quest was found as a breadcrumb quest
            //breadcrumb loop found!
            if (!questSet.insert(qinfo->_id).second)
            {
                TC_LOG_ERROR("sql.sql", "Breadcrumb quests %u and %u are in a loop", qid, breadcrumbForQuestId);
                qinfo->_breadcrumbForQuestId = 0;
                break;
            }

            qinfo = const_cast<Quest*>(sObjectMgr->GetQuestTemplate(breadcrumbForQuestId));

            //every quest has a list of every breadcrumb towards it
            qinfo->DependentBreadcrumbQuests.push_back(qid);

            breadcrumbForQuestId = qinfo->GetBreadcrumbForQuestId();
        }
    }

    // don't check spells with SPELL_EFFECT_QUEST_COMPLETE, a lot of invalid db2 data

    TC_LOG_INFO("server.loading", ">> Loaded %lu quests definitions in %u ms", (unsigned long)_questTemplates.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadQuestStartersAndEnders()
{
    TC_LOG_INFO("server.loading", "Loading GO Start Quest Data...");
    LoadGameobjectQuestStarters();
    TC_LOG_INFO("server.loading", "Loading GO End Quest Data...");
    LoadGameobjectQuestEnders();
    TC_LOG_INFO("server.loading", "Loading Creature Start Quest Data...");
    LoadCreatureQuestStarters();
    TC_LOG_INFO("server.loading", "Loading Creature End Quest Data...");
    LoadCreatureQuestEnders();
}

void ObjectMgr::LoadQuestLocales()
{
    uint32 oldMSTime = getMSTime();

    _questLocaleStore.clear();                                // need for reload case

    //                                               0   1       2      3        4           5                6                 7        8              9                     10                    11                   12                   13              14              15              16
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Title, Details, Objectives, OfferRewardText, RequestItemsText, EndText, CompletedText, QuestGiverTextWindow, QuestGiverTargetName, QuestTurnTextWindow, QuestTurnTargetName, ObjectiveText1, ObjectiveText2, ObjectiveText3, ObjectiveText4 FROM quest_template_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 id               = fields[0].GetUInt32();
        std::string localeName  = fields[1].GetString();


        LocaleConstant locale = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        QuestLocale& data = _questLocaleStore[id];
        AddLocaleString(fields[2].GetString(), locale, data.Title);
        AddLocaleString(fields[3].GetString(), locale, data.Details);
        AddLocaleString(fields[4].GetString(), locale, data.Objectives);
        AddLocaleString(fields[5].GetString(), locale, data.OfferRewardText);
        AddLocaleString(fields[6].GetString(), locale, data.RequestItemsText);
        AddLocaleString(fields[7].GetString(), locale, data.AreaDescription);
        AddLocaleString(fields[8].GetString(), locale, data.CompletedText);
        AddLocaleString(fields[9].GetString(), locale, data.QuestGiverTextWindow);
        AddLocaleString(fields[10].GetString(), locale, data.QuestGiverTargetName);
        AddLocaleString(fields[11].GetString(), locale, data.QuestTurnTextWindow);
        AddLocaleString(fields[12].GetString(), locale, data.QuestTurnTargetName);

        for (uint8 i = 0; i < 4; ++i)
            AddLocaleString(fields[i + 13].GetString(), locale, data.ObjectiveText[i]);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u Quest locale strings in %u ms", uint32(_questLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadScripts(ScriptsType type)
{
    uint32 oldMSTime = getMSTime();

    ScriptMapMap* scripts = GetScriptsMapByType(type);
    if (!scripts)
        return;

    std::string tableName = GetScriptsTableNameByType(type);
    if (tableName.empty())
        return;

    if (sMapMgr->IsScriptScheduled())                    // function cannot be called when scripts are in use.
        return;

    TC_LOG_INFO("server.loading", "Loading %s...", tableName.c_str());

    scripts->clear();                                       // need for reload support

    bool isSpellScriptTable = type == SCRIPTS_SPELL;
    //                                                 0    1       2         3         4          5    6  7  8  9
    QueryResult result = WorldDatabase.PQuery("SELECT id, delay, command, datalong, datalong2, dataint, x, y, z, o%s FROM %s", isSpellScriptTable ? ", effIndex" : "", tableName.c_str());

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 script definitions. DB table `%s` is empty!", tableName.c_str());
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        ScriptInfo tmp;
        tmp.type      = type;
        tmp.id           = fields[0].GetUInt32();
        if (isSpellScriptTable)
            tmp.id      |= fields[10].GetUInt8() << 24;
        tmp.delay        = fields[1].GetUInt32();
        tmp.command      = ScriptCommands(fields[2].GetUInt32());
        tmp.Raw.nData[0] = fields[3].GetUInt32();
        tmp.Raw.nData[1] = fields[4].GetUInt32();
        tmp.Raw.nData[2] = fields[5].GetInt32();
        tmp.Raw.fData[0] = fields[6].GetFloat();
        tmp.Raw.fData[1] = fields[7].GetFloat();
        tmp.Raw.fData[2] = fields[8].GetFloat();
        tmp.Raw.fData[3] = fields[9].GetFloat();

        // generic command args check
        switch (tmp.command)
        {
            case SCRIPT_COMMAND_TALK:
            {
                if (tmp.Talk.ChatType > CHAT_TYPE_WHISPER && tmp.Talk.ChatType != CHAT_MSG_RAID_BOSS_WHISPER)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid talk type (datalong = %u) in SCRIPT_COMMAND_TALK for script id %u",
                        tableName.c_str(), tmp.Talk.ChatType, tmp.id);
                    continue;
                }
                if (!sObjectMgr->GetBroadcastText(uint32(tmp.Talk.TextID)))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid talk text id (dataint = %i) in SCRIPT_COMMAND_TALK for script id %u",
                        tableName.c_str(), tmp.Talk.TextID, tmp.id);
                    continue;
                }

                break;
            }

            case SCRIPT_COMMAND_EMOTE:
            {
                if (!sEmotesStore.LookupEntry(tmp.Emote.EmoteID))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid emote id (datalong = %u) in SCRIPT_COMMAND_EMOTE for script id %u",
                        tableName.c_str(), tmp.Emote.EmoteID, tmp.id);
                    continue;
                }
                break;
            }

            case SCRIPT_COMMAND_TELEPORT_TO:
            {
                if (!sMapStore.LookupEntry(tmp.TeleportTo.MapID))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid map (Id: %u) in SCRIPT_COMMAND_TELEPORT_TO for script id %u",
                        tableName.c_str(), tmp.TeleportTo.MapID, tmp.id);
                    continue;
                }

                if (!Trinity::IsValidMapCoord(tmp.TeleportTo.DestX, tmp.TeleportTo.DestY, tmp.TeleportTo.DestZ, tmp.TeleportTo.Orientation))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid coordinates (X: %f Y: %f Z: %f O: %f) in SCRIPT_COMMAND_TELEPORT_TO for script id %u",
                        tableName.c_str(), tmp.TeleportTo.DestX, tmp.TeleportTo.DestY, tmp.TeleportTo.DestZ, tmp.TeleportTo.Orientation, tmp.id);
                    continue;
                }
                break;
            }

            case SCRIPT_COMMAND_QUEST_EXPLORED:
            {
                Quest const* quest = GetQuestTemplate(tmp.QuestExplored.QuestID);
                if (!quest)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid quest (ID: %u) in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u",
                        tableName.c_str(), tmp.QuestExplored.QuestID, tmp.id);
                    continue;
                }

                if (!quest->HasFlag(QUEST_FLAGS_COMPLETION_EVENT) && !quest->HasFlag(QUEST_FLAGS_COMPLETION_AREA_TRIGGER))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has quest (ID: %u) in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u, but quest not have flag QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT in quest flags. Script command or quest flags wrong. Quest modified to require objective.",
                        tableName.c_str(), tmp.QuestExplored.QuestID, tmp.id);
                    continue;
                }

                if (float(tmp.QuestExplored.Distance) > DEFAULT_VISIBILITY_DISTANCE)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has too large distance (%u) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u",
                        tableName.c_str(), tmp.QuestExplored.Distance, tmp.id);
                    continue;
                }

                if (tmp.QuestExplored.Distance && float(tmp.QuestExplored.Distance) > DEFAULT_VISIBILITY_DISTANCE)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has too large distance (%u) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u, max distance is %f or 0 for disable distance check",
                        tableName.c_str(), tmp.QuestExplored.Distance, tmp.id, DEFAULT_VISIBILITY_DISTANCE);
                    continue;
                }

                if (tmp.QuestExplored.Distance && float(tmp.QuestExplored.Distance) < INTERACTION_DISTANCE)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has too small distance (%u) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u, min distance is %f or 0 for disable distance check",
                        tableName.c_str(), tmp.QuestExplored.Distance, tmp.id, INTERACTION_DISTANCE);
                    continue;
                }

                break;
            }

            case SCRIPT_COMMAND_KILL_CREDIT:
            {
                if (!GetCreatureTemplate(tmp.KillCredit.CreatureEntry))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid creature (Entry: %u) in SCRIPT_COMMAND_KILL_CREDIT for script id %u",
                        tableName.c_str(), tmp.KillCredit.CreatureEntry, tmp.id);
                    continue;
                }
                break;
            }

            case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT:
            {
                GameObjectData const* data = GetGameObjectData(tmp.RespawnGameobject.GOGuid);
                if (!data)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid gameobject (GUID: %u) in SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id %u",
                        tableName.c_str(), tmp.RespawnGameobject.GOGuid, tmp.id);
                    continue;
                }

                GameObjectTemplate const* info = GetGameObjectTemplate(data->id);
                if (!info)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has gameobject with invalid entry (GUID: %u Entry: %u) in SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id %u",
                        tableName.c_str(), tmp.RespawnGameobject.GOGuid, data->id, tmp.id);
                    continue;
                }

                if (info->type == GAMEOBJECT_TYPE_FISHINGNODE ||
                    info->type == GAMEOBJECT_TYPE_FISHINGHOLE ||
                    info->type == GAMEOBJECT_TYPE_DOOR        ||
                    info->type == GAMEOBJECT_TYPE_BUTTON      ||
                    info->type == GAMEOBJECT_TYPE_TRAP)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has gameobject type (%u) unsupported by command SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id %u",
                        tableName.c_str(), info->entry, tmp.id);
                    continue;
                }
                break;
            }

            case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:
            {
                if (!Trinity::IsValidMapCoord(tmp.TempSummonCreature.PosX, tmp.TempSummonCreature.PosY, tmp.TempSummonCreature.PosZ, tmp.TempSummonCreature.Orientation))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid coordinates (X: %f Y: %f Z: %f O: %f) in SCRIPT_COMMAND_TEMP_SUMMON_CREATURE for script id %u",
                        tableName.c_str(), tmp.TempSummonCreature.PosX, tmp.TempSummonCreature.PosY, tmp.TempSummonCreature.PosZ, tmp.TempSummonCreature.Orientation, tmp.id);
                    continue;
                }

                if (!GetCreatureTemplate(tmp.TempSummonCreature.CreatureEntry))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid creature (Entry: %u) in SCRIPT_COMMAND_TEMP_SUMMON_CREATURE for script id %u",
                        tableName.c_str(), tmp.TempSummonCreature.CreatureEntry, tmp.id);
                    continue;
                }
                break;
            }

            case SCRIPT_COMMAND_OPEN_DOOR:
            case SCRIPT_COMMAND_CLOSE_DOOR:
            {
                GameObjectData const* data = GetGameObjectData(tmp.ToggleDoor.GOGuid);
                if (!data)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid gameobject (GUID: %u) in %s for script id %u",
                        tableName.c_str(), tmp.ToggleDoor.GOGuid, GetScriptCommandName(tmp.command).c_str(), tmp.id);
                    continue;
                }

                GameObjectTemplate const* info = GetGameObjectTemplate(data->id);
                if (!info)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has gameobject with invalid entry (GUID: %u Entry: %u) in %s for script id %u",
                        tableName.c_str(), tmp.ToggleDoor.GOGuid, data->id, GetScriptCommandName(tmp.command).c_str(), tmp.id);
                    continue;
                }

                if (info->type != GAMEOBJECT_TYPE_DOOR)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has gameobject type (%u) unsupported by command %s for script id %u",
                        tableName.c_str(), info->entry, GetScriptCommandName(tmp.command).c_str(), tmp.id);
                    continue;
                }

                break;
            }

            case SCRIPT_COMMAND_REMOVE_AURA:
            {
                if (!sSpellMgr->GetSpellInfo(tmp.RemoveAura.SpellID))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` using non-existent spell (id: %u) in SCRIPT_COMMAND_REMOVE_AURA for script id %u",
                        tableName.c_str(), tmp.RemoveAura.SpellID, tmp.id);
                    continue;
                }
                if (tmp.RemoveAura.Flags & ~0x1)                    // 1 bits (0, 1)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` using unknown flags in datalong2 (%u) in SCRIPT_COMMAND_REMOVE_AURA for script id %u",
                        tableName.c_str(), tmp.RemoveAura.Flags, tmp.id);
                    continue;
                }
                break;
            }

            case SCRIPT_COMMAND_CAST_SPELL:
            {
                if (!sSpellMgr->GetSpellInfo(tmp.CastSpell.SpellID))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` using non-existent spell (id: %u) in SCRIPT_COMMAND_CAST_SPELL for script id %u",
                        tableName.c_str(), tmp.CastSpell.SpellID, tmp.id);
                    continue;
                }
                if (tmp.CastSpell.Flags > 4)                      // targeting type
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` using unknown target in datalong2 (%u) in SCRIPT_COMMAND_CAST_SPELL for script id %u",
                        tableName.c_str(), tmp.CastSpell.Flags, tmp.id);
                    continue;
                }
                if (tmp.CastSpell.Flags != 4 && tmp.CastSpell.CreatureEntry & ~0x1)                      // 1 bit (0, 1)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` using unknown flags in dataint (%u) in SCRIPT_COMMAND_CAST_SPELL for script id %u",
                        tableName.c_str(), tmp.CastSpell.CreatureEntry, tmp.id);
                    continue;
                }
                else if (tmp.CastSpell.Flags == 4 && !GetCreatureTemplate(tmp.CastSpell.CreatureEntry))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` using invalid creature entry in dataint (%u) in SCRIPT_COMMAND_CAST_SPELL for script id %u",
                        tableName.c_str(), tmp.CastSpell.CreatureEntry, tmp.id);
                    continue;
                }
                break;
            }

            case SCRIPT_COMMAND_CREATE_ITEM:
            {
                if (!GetItemTemplate(tmp.CreateItem.ItemEntry))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has nonexistent item (entry: %u) in SCRIPT_COMMAND_CREATE_ITEM for script id %u",
                        tableName.c_str(), tmp.CreateItem.ItemEntry, tmp.id);
                    continue;
                }
                if (!tmp.CreateItem.Amount)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` SCRIPT_COMMAND_CREATE_ITEM but amount is %u for script id %u",
                        tableName.c_str(), tmp.CreateItem.Amount, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_PLAY_ANIMKIT:
            {
                if (!sAnimKitStore.LookupEntry(tmp.PlayAnimKit.AnimKitID))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid AnimKid id (datalong = %u) in SCRIPT_COMMAND_PLAY_ANIMKIT for script id %u",
                        tableName.c_str(), tmp.PlayAnimKit.AnimKitID, tmp.id);
                    continue;
                }
                break;
            }
            default:
                break;
        }

        if (scripts->find(tmp.id) == scripts->end())
        {
            ScriptMap emptyMap;
            (*scripts)[tmp.id] = emptyMap;
        }
        (*scripts)[tmp.id].insert(std::pair<uint32, ScriptInfo>(tmp.delay, tmp));

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u script definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadSpellScripts()
{
    LoadScripts(SCRIPTS_SPELL);

    // check ids
    for (ScriptMapMap::const_iterator itr = sSpellScripts.begin(); itr != sSpellScripts.end(); ++itr)
    {
        uint32 spellId = uint32(itr->first) & 0x00FFFFFF;
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);

        if (!spellInfo)
        {
            TC_LOG_ERROR("sql.sql", "Table `spell_scripts` has not existing spell (Id: %u) as script id", spellId);
            continue;
        }

        uint8 i = (uint8)((uint32(itr->first) >> 24) & 0x000000FF);
        //check for correct spellEffect
        if (!spellInfo->Effects[i].IsEffect() || (spellInfo->Effects[i].Effect != SPELL_EFFECT_SCRIPT_EFFECT && spellInfo->Effects[i].Effect != SPELL_EFFECT_DUMMY))
            TC_LOG_ERROR("sql.sql", "Table `spell_scripts` - spell %u effect %u is not SPELL_EFFECT_SCRIPT_EFFECT or SPELL_EFFECT_DUMMY", spellId, i);
    }
}

void ObjectMgr::LoadEventScripts()
{
    LoadScripts(SCRIPTS_EVENT);

    std::set<uint32> evt_scripts;
    // Load all possible script entries from gameobjects
    GameObjectTemplateContainer const* gotc = sObjectMgr->GetGameObjectTemplates();
    for (GameObjectTemplateContainer::const_iterator itr = gotc->begin(); itr != gotc->end(); ++itr)
        if (uint32 eventId = itr->second.GetEventScriptId())
            evt_scripts.insert(eventId);

    // Load all possible script entries from spells
    for (uint32 i = 1; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
        if (SpellInfo const* spell = sSpellMgr->GetSpellInfo(i))
            for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
                if (spell->Effects[j].Effect == SPELL_EFFECT_SEND_EVENT)
                    if (spell->Effects[j].MiscValue)
                        evt_scripts.insert(spell->Effects[j].MiscValue);

    for (size_t path_idx = 0; path_idx < sTaxiPathNodesByPath.size(); ++path_idx)
    {
        for (size_t node_idx = 0; node_idx < sTaxiPathNodesByPath[path_idx].size(); ++node_idx)
        {
            TaxiPathNodeEntry const* node = sTaxiPathNodesByPath[path_idx][node_idx];

            if (node->ArrivalEventID)
                evt_scripts.insert(node->ArrivalEventID);

            if (node->DepartureEventID)
                evt_scripts.insert(node->DepartureEventID);
        }
    }

    // Then check if all scripts are in above list of possible script entries
    for (ScriptMapMap::const_iterator itr = sEventScripts.begin(); itr != sEventScripts.end(); ++itr)
    {
        std::set<uint32>::const_iterator itr2 = evt_scripts.find(itr->first);
        if (itr2 == evt_scripts.end())
            TC_LOG_ERROR("sql.sql", "Table `event_scripts` has script (Id: %u) not referring to any gameobject_template type 10 data2 field, type 3 data6 field, type 13 data 2 field or any spell effect %u",
                itr->first, SPELL_EFFECT_SEND_EVENT);
    }
}

//Load WP Scripts
void ObjectMgr::LoadWaypointScripts()
{
    LoadScripts(SCRIPTS_WAYPOINT);

    std::set<uint32> actionSet;

    for (ScriptMapMap::const_iterator itr = sWaypointScripts.begin(); itr != sWaypointScripts.end(); ++itr)
        actionSet.insert(itr->first);

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_WAYPOINT_DATA_ACTION);
    PreparedQueryResult result = WorldDatabase.Query(stmt);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 action = fields[0].GetUInt32();

            actionSet.erase(action);
        }
        while (result->NextRow());
    }

    for (std::set<uint32>::iterator itr = actionSet.begin(); itr != actionSet.end(); ++itr)
        TC_LOG_ERROR("sql.sql", "There is no waypoint which links to the waypoint script %u", *itr);
}

void ObjectMgr::LoadSpellScriptNames()
{
    uint32 oldMSTime = getMSTime();

    _spellScriptsStore.clear();                            // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT spell_id, ScriptName FROM spell_script_names");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell script names. DB table `spell_script_names` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {

        Field* fields = result->Fetch();

        int32 spellId                = fields[0].GetInt32();
        std::string const scriptName = fields[1].GetString();

        bool allRanks = false;
        if (spellId < 0)
        {
            allRanks = true;
            spellId = -spellId;
        }

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
        {
            TC_LOG_ERROR("sql.sql", "Scriptname: `%s` spell (Id: %d) does not exist.", scriptName.c_str(), spellId);
            continue;
        }

        if (allRanks)
        {
            if (!spellInfo->IsRanked())
                TC_LOG_ERROR("sql.sql", "Scriptname: `%s` spell (Id: %d) has no ranks of spell.", scriptName.c_str(), fields[0].GetInt32());

            if (spellInfo->GetFirstRankSpell()->Id != uint32(spellId))
            {
                TC_LOG_ERROR("sql.sql", "Scriptname: `%s` spell (Id: %d) is not first rank of spell.", scriptName.c_str(), fields[0].GetInt32());
                continue;
            }

            while (spellInfo)
            {
                _spellScriptsStore.insert(SpellScriptsContainer::value_type(spellInfo->Id, std::make_pair(GetScriptId(scriptName), true)));
                spellInfo = spellInfo->GetNextRankSpell();
            }
        }
        else
        {
            if (spellInfo->IsRanked())
                TC_LOG_ERROR("sql.sql", "Scriptname: `%s` spell (Id: %d) is ranked spell. Perhaps not all ranks are assigned to this script.", scriptName.c_str(), spellId);

            _spellScriptsStore.insert(SpellScriptsContainer::value_type(spellInfo->Id, std::make_pair(GetScriptId(scriptName), true)));
        }

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u spell script names in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::ValidateSpellScripts()
{
    uint32 oldMSTime = getMSTime();

    if (_spellScriptsStore.empty())
    {
        TC_LOG_INFO("server.loading", ">> Validated 0 scripts.");
        return;
    }

    uint32 count = 0;

    for (auto const& spell : _spellScriptsStore)
    {
        SpellInfo const* spellEntry = sSpellMgr->GetSpellInfo(spell.first);

        auto const bounds = sObjectMgr->GetSpellScriptsBounds(spell.first);

        for (auto itr = bounds.first; itr != bounds.second; ++itr)
        {
            if (SpellScriptLoader* spellScriptLoader = sScriptMgr->GetSpellScriptLoader(itr->second.first))
            {
                ++count;

                std::unique_ptr<SpellScript> spellScript(spellScriptLoader->GetSpellScript());
                std::unique_ptr<AuraScript> auraScript(spellScriptLoader->GetAuraScript());

                if (!spellScript && !auraScript)
                {
                    TC_LOG_ERROR("scripts", "Functions GetSpellScript() and GetAuraScript() of script `%s` do not return objects - script skipped", GetScriptName(itr->second.first).c_str());

                    itr->second.second = false;
                    continue;
                }

                if (spellScript)
                {
                    spellScript->_Init(&spellScriptLoader->GetName(), spellEntry->Id);
                    spellScript->_Register();

                    if (!spellScript->_Validate(spellEntry))
                    {
                        itr->second.second = false;
                        continue;
                    }
                }

                if (auraScript)
                {
                    auraScript->_Init(&spellScriptLoader->GetName(), spellEntry->Id);
                    auraScript->_Register();

                    if (!auraScript->_Validate(spellEntry))
                    {
                        itr->second.second = false;
                        continue;
                    }
                }

                // Enable the script when all checks passed
                itr->second.second = true;
            }
            else
                itr->second.second = false;
        }
    }

    TC_LOG_INFO("server.loading", ">> Validated %u scripts in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadPageTexts()
{
    uint32 oldMSTime = getMSTime();

    //                                               0    1      2
    QueryResult result = WorldDatabase.Query("SELECT ID, `Text`, NextPageID FROM page_text");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 page texts. DB table `page_text` is empty!");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        PageText& pageText = _pageTextStore[fields[0].GetUInt32()];

        pageText.Text       = fields[1].GetString();
        pageText.NextPageID = fields[2].GetUInt32();

        ++count;
    }
    while (result->NextRow());

    for (PageTextContainer::const_iterator itr = _pageTextStore.begin(); itr != _pageTextStore.end(); ++itr)
    {
        if (itr->second.NextPageID)
        {
            PageTextContainer::const_iterator itr2 = _pageTextStore.find(itr->second.NextPageID);
            if (itr2 == _pageTextStore.end())
                TC_LOG_ERROR("sql.sql", "Page text (ID: %u) has non existing `NextPageID` (%u)", itr->first, itr->second.NextPageID);

        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u page texts in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

PageText const* ObjectMgr::GetPageText(uint32 pageEntry)
{
    PageTextContainer::const_iterator itr = _pageTextStore.find(pageEntry);
    if (itr != _pageTextStore.end())
        return &(itr->second);

    return nullptr;
}

void ObjectMgr::LoadPageTextLocales()
{
    uint32 oldMSTime = getMSTime();

    _pageTextLocaleStore.clear(); // needed for reload case

    //                                               0   1        2
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, `Text` FROM page_text_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 id                   = fields[0].GetUInt32();
        std::string localeName      = fields[1].GetString();
        LocaleConstant locale = GetLocaleByName(localeName);

        if (locale == LOCALE_enUS)
            continue;

        PageTextLocale& data = _pageTextLocaleStore[id];
        AddLocaleString(fields[2].GetString(), locale, data.Text);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u PageText locale strings in %u ms", uint32(_pageTextLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadInstanceTemplate()
{
    uint32 oldMSTime = getMSTime();

    //                                                0     1       2
    QueryResult result = WorldDatabase.Query("SELECT map, parent, script FROM instance_template");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 instance templates. DB table `page_text` is empty!");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint16 mapID = fields[0].GetUInt16();

        if (!MapManager::IsValidMAP(mapID))
        {
            TC_LOG_ERROR("sql.sql", "ObjectMgr::LoadInstanceTemplate: bad mapid %d for template!", mapID);
            continue;
        }

        InstanceTemplate instanceTemplate;
        instanceTemplate.Parent     = uint32(fields[1].GetUInt16());
        instanceTemplate.ScriptId   = sObjectMgr->GetScriptId(fields[2].GetString());

        _instanceTemplateStore[mapID] = instanceTemplate;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u instance templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

InstanceTemplate const* ObjectMgr::GetInstanceTemplate(uint32 mapID) const
{
    InstanceTemplateContainer::const_iterator itr = _instanceTemplateStore.find(uint16(mapID));
    if (itr != _instanceTemplateStore.end())
        return &(itr->second);

    return nullptr;
}

void ObjectMgr::LoadInstanceEncounters()
{
    uint32 oldMSTime = getMSTime();

    //                                                 0         1            2                3
    QueryResult result = WorldDatabase.Query("SELECT entry, creditType, creditEntry, lastEncounterDungeon FROM instance_encounters");
    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 instance encounters, table is empty!");
        return;
    }

    uint32 count = 0;
    std::map<uint32, DungeonEncounterEntry const*> dungeonLastBosses;
    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].GetUInt32();
        uint8 creditType = fields[1].GetUInt8();
        uint32 creditEntry = fields[2].GetUInt32();
        uint32 lastEncounterDungeon = fields[3].GetUInt16();
        DungeonEncounterEntry const* dungeonEncounter = sDungeonEncounterStore.LookupEntry(entry);
        if (!dungeonEncounter)
        {
            TC_LOG_ERROR("sql.sql", "Table `instance_encounters` has an invalid encounter id %u, skipped!", entry);
            continue;
        }

        if (lastEncounterDungeon && !sLFGMgr->GetLFGDungeonEntry(lastEncounterDungeon))
        {
            TC_LOG_ERROR("sql.sql", "Table `instance_encounters` has an encounter %u (%s) marked as final for invalid dungeon id %u, skipped!", entry, dungeonEncounter->Name, lastEncounterDungeon);
            continue;
        }

        std::map<uint32, DungeonEncounterEntry const*>::const_iterator itr = dungeonLastBosses.find(lastEncounterDungeon);
        if (lastEncounterDungeon)
        {
            if (itr != dungeonLastBosses.end())
            {
                TC_LOG_ERROR("sql.sql", "Table `instance_encounters` specified encounter %u (%s) as last encounter but %u (%s) is already marked as one, skipped!", entry, dungeonEncounter->Name, itr->second->ID, itr->second->Name);
                continue;
            }

            dungeonLastBosses[lastEncounterDungeon] = dungeonEncounter;
        }

        switch (creditType)
        {
            case ENCOUNTER_CREDIT_KILL_CREATURE:
            {
                CreatureTemplate const* creatureInfo = GetCreatureTemplate(creditEntry);
                if (!creatureInfo)
                {
                    TC_LOG_ERROR("sql.sql", "Table `instance_encounters` has an invalid creature (entry %u) linked to the encounter %u (%s), skipped!", creditEntry, entry, dungeonEncounter->Name);
                    continue;
                }
                const_cast<CreatureTemplate*>(creatureInfo)->flags_extra |= CREATURE_FLAG_EXTRA_DUNGEON_BOSS;

                if (dungeonEncounter->DifficultyID == -1)
                {
                    for (uint8 i = 0; i < 3; i++)
                    {
                        if (CreatureTemplate const* creatureBaseInfo = GetCreatureTemplate(creditEntry))
                            if (CreatureTemplate const* creatureInfo = GetCreatureTemplate(creatureBaseInfo->DifficultyEntry[i]))
                                const_cast<CreatureTemplate*>(creatureInfo)->flags_extra |= CREATURE_FLAG_EXTRA_DUNGEON_BOSS;
                    }
                }
                break;
            }
            case ENCOUNTER_CREDIT_CAST_SPELL:
                if (!sSpellMgr->GetSpellInfo(creditEntry))
                {
                    TC_LOG_ERROR("sql.sql", "Table `instance_encounters` has an invalid spell (entry %u) linked to the encounter %u (%s), skipped!", creditEntry, entry, dungeonEncounter->Name);
                    continue;
                }
                break;
            default:
                TC_LOG_ERROR("sql.sql", "Table `instance_encounters` has an invalid credit type (%u) for encounter %u (%s), skipped!", creditType, entry, dungeonEncounter->Name);
                continue;
        }

        if (dungeonEncounter->DifficultyID == -1)
        {
            for (uint32 i = 0; i < MAX_DIFFICULTY; ++i)
            {
                if (sDBCManager.GetMapDifficultyData(dungeonEncounter->MapID, Difficulty(i)))
                {
                    DungeonEncounterList& encounters = _dungeonEncounterStore[MAKE_PAIR32(dungeonEncounter->MapID, i)];
                    encounters.push_back(new DungeonEncounter(dungeonEncounter, EncounterCreditType(creditType), creditEntry, lastEncounterDungeon));
                }
            }
        }
        else
        {
            DungeonEncounterList& encounters = _dungeonEncounterStore[MAKE_PAIR32(dungeonEncounter->MapID, dungeonEncounter->DifficultyID)];
            encounters.push_back(new DungeonEncounter(dungeonEncounter, EncounterCreditType(creditType), creditEntry, lastEncounterDungeon));
        }

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u instance encounters in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

GossipText const* ObjectMgr::GetGossipText(uint32 Text_ID) const
{
    GossipTextContainer::const_iterator itr = _gossipTextStore.find(Text_ID);
    if (itr != _gossipTextStore.end())
        return &itr->second;
    return nullptr;
}

void ObjectMgr::LoadGossipText()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT ID, "
        "text0_0, text0_1, BroadcastTextID0, lang0, Probability0, EmoteDelay0_0, Emote0_0, EmoteDelay0_1, Emote0_1, EmoteDelay0_2, Emote0_2, "
        "text1_0, text1_1, BroadcastTextID1, lang1, Probability1, EmoteDelay1_0, Emote1_0, EmoteDelay1_1, Emote1_1, EmoteDelay1_2, Emote1_2, "
        "text2_0, text2_1, BroadcastTextID2, lang2, Probability2, EmoteDelay2_0, Emote2_0, EmoteDelay2_1, Emote2_1, EmoteDelay2_2, Emote2_2, "
        "text3_0, text3_1, BroadcastTextID3, lang3, Probability3, EmoteDelay3_0, Emote3_0, EmoteDelay3_1, Emote3_1, EmoteDelay3_2, Emote3_2, "
        "text4_0, text4_1, BroadcastTextID4, lang4, Probability4, EmoteDelay4_0, Emote4_0, EmoteDelay4_1, Emote4_1, EmoteDelay4_2, Emote4_2, "
        "text5_0, text5_1, BroadcastTextID5, lang5, Probability5, EmoteDelay5_0, Emote5_0, EmoteDelay5_1, Emote5_1, EmoteDelay5_2, Emote5_2, "
        "text6_0, text6_1, BroadcastTextID6, lang6, Probability6, EmoteDelay6_0, Emote6_0, EmoteDelay6_1, Emote6_1, EmoteDelay6_2, Emote6_2, "
        "text7_0, text7_1, BroadcastTextID7, lang7, Probability7, EmoteDelay7_0, Emote7_0, EmoteDelay7_1, Emote7_1, EmoteDelay7_2, Emote7_2 "
        "FROM npc_text");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 npc texts, table is empty!");
        return;
    }

    _gossipTextStore.reserve(result->GetRowCount());

    uint32 count = 0;
    uint8 cic;

    do
    {
        ++count;
        cic = 0;

        Field* fields = result->Fetch();

        uint32 id = fields[cic++].GetUInt32();
        if (!id)
        {
            TC_LOG_ERROR("sql.sql", "Table `npc_text` has record with reserved id 0, ignore.");
            continue;
        }

        GossipText& gText = _gossipTextStore[id];

        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            GossipTextOption& gOption = gText.Options[i];
            gOption.Text_0           = fields[cic++].GetString();
            gOption.Text_1           = fields[cic++].GetString();
            gOption.BroadcastTextID  = fields[cic++].GetUInt32();
            gOption.Language         = fields[cic++].GetUInt8();
            gOption.Probability      = fields[cic++].GetFloat();

            for (uint8 j = 0; j < MAX_GOSSIP_TEXT_EMOTES; ++j)
            {
                gOption.Emotes[j]._Delay = fields[cic++].GetUInt16();
                gOption.Emotes[j]._Emote = fields[cic++].GetUInt16();
            }

            // check broadcast_text correctness
            if (gOption.BroadcastTextID)
            {
                if (BroadcastText const* bcText = sObjectMgr->GetBroadcastText(gOption.BroadcastTextID))
                {
                    if (bcText->Text[DEFAULT_LOCALE] != gOption.Text_0)
                        TC_LOG_INFO("sql.sql", "Row %u in table `npc_text` has mismatch between text%u_0 and the corresponding Text in `broadcast_text` row %u", id, i, gOption.BroadcastTextID);
                    if (bcText->Text1[DEFAULT_LOCALE] != gOption.Text_1)
                        TC_LOG_INFO("sql.sql", "Row %u in table `npc_text` has mismatch between text%u_1 and the corresponding Text1 in `broadcast_text` row %u", id, i, gOption.BroadcastTextID);
                }
                else
                {
                    TC_LOG_ERROR("sql.sql", "GossipText (Id: %u) in table `npc_text` has non-existing or incompatible BroadcastTextID%u %u.", id, i, gOption.BroadcastTextID);
                    gOption.BroadcastTextID = 0;
                }


            }
        }

    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u npc texts in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadNpcTextLocales()
{
    uint32 oldMSTime = getMSTime();

    _npcTextLocaleStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, "
    //   2        3        4        5        6        7        8        9        10       11       12       13       14       15       16       17
        "Text0_0, Text0_1, Text1_0, Text1_1, Text2_0, Text2_1, Text3_0, Text3_1, Text4_0, Text4_1, Text5_0, Text5_1, Text6_0, Text6_1, Text7_0, Text7_1 "
        "FROM npc_text_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 id               = fields[0].GetUInt32();
        std::string localeName  = fields[1].GetString();

        NpcTextLocale& data     = _npcTextLocaleStore[id];
        LocaleConstant locale   = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            AddLocaleString(fields[2 + i * 2].GetString(), locale, data.Text_0[i]);
            AddLocaleString(fields[3 + i * 2].GetString(), locale, data.Text_1[i]);
        }
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u NpcText locale strings in %u ms", uint32(_npcTextLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

//not very fast function but it is called only once a day, or on starting-up
void ObjectMgr::ReturnOrDeleteOldMails(bool serverUp)
{
    uint32 oldMSTime = getMSTime();

    time_t curTime = GameTime::GetGameTime();
    tm lt;
    localtime_r(&curTime, &lt);
    uint64 basetime(curTime);
    TC_LOG_INFO("misc", "Returning mails current time: hour: %d, minute: %d, second: %d ", lt.tm_hour, lt.tm_min, lt.tm_sec);

    // Delete all old mails without item and without body immediately, if starting server
    if (!serverUp)
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_EMPTY_EXPIRED_MAIL);
        stmt->setUInt64(0, basetime);
        CharacterDatabase.Execute(stmt);
    }
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_EXPIRED_MAIL);
    stmt->setUInt64(0, basetime);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> No expired mails found.");
        return;                                             // any mails need to be returned or deleted
    }

    std::map<uint32 /*messageId*/, MailItemInfoVec> itemsCache;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_EXPIRED_MAIL_ITEMS);
    stmt->setUInt32(0, (uint32)basetime);
    if (PreparedQueryResult items = CharacterDatabase.Query(stmt))
    {
        MailItemInfo item;
        do
        {
            Field* fields = items->Fetch();
            item.item_guid = fields[0].GetUInt32();
            item.item_template = fields[1].GetUInt32();
            uint32 mailId = fields[2].GetUInt32();
            itemsCache[mailId].push_back(item);
        } while (items->NextRow());
    }

    uint32 deletedCount = 0;
    uint32 returnedCount = 0;
    do
    {
        Field* fields = result->Fetch();
        ObjectGuid::LowType receiver = fields[3].GetUInt32();
        if (serverUp && ObjectAccessor::FindConnectedPlayer(ObjectGuid(HighGuid::Player, receiver)))
            continue;

        Mail* m = new Mail;
        m->messageID      = fields[0].GetUInt32();
        m->messageType    = fields[1].GetUInt8();
        m->sender         = fields[2].GetUInt32();
        m->receiver       = receiver;
        bool has_items    = fields[4].GetBool();
        m->expire_time    = time_t(fields[5].GetUInt32());
        m->deliver_time   = 0;
        m->COD            = fields[6].GetUInt64();
        m->checked        = fields[7].GetUInt8();
        m->mailTemplateId = fields[8].GetInt16();

        // Delete or return mail
        if (has_items)
        {
            // read items from cache
            m->items.swap(itemsCache[m->messageID]);

            // if it is mail from non-player, or if it's already return mail, it shouldn't be returned, but deleted
            if (m->messageType != MAIL_NORMAL || (m->checked & (MAIL_CHECK_MASK_COD_PAYMENT | MAIL_CHECK_MASK_RETURNED)))
            {
                // mail open and then not returned
                for (MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
                {
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
                    stmt->setUInt32(0, itr2->item_guid);
                    CharacterDatabase.Execute(stmt);
                }

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEM_BY_ID);
                stmt->setUInt32(0, m->messageID);
                CharacterDatabase.Execute(stmt);
            }
            else
            {
                // Mail will be returned
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_MAIL_RETURNED);
                stmt->setUInt32(0, m->receiver);
                stmt->setUInt32(1, m->sender);
                stmt->setUInt32(2, basetime + 30 * DAY);
                stmt->setUInt32(3, basetime);
                stmt->setUInt8 (4, uint8(MAIL_CHECK_MASK_RETURNED));
                stmt->setUInt32(5, m->messageID);
                CharacterDatabase.Execute(stmt);
                for (MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
                {
                    // Update receiver in mail items for its proper delivery, and in instance_item for avoid lost item at sender delete
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_MAIL_ITEM_RECEIVER);
                    stmt->setUInt32(0, m->sender);
                    stmt->setUInt32(1, itr2->item_guid);
                    CharacterDatabase.Execute(stmt);

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ITEM_OWNER);
                    stmt->setUInt32(0, m->sender);
                    stmt->setUInt32(1, itr2->item_guid);
                    CharacterDatabase.Execute(stmt);
                }
                delete m;
                ++returnedCount;
                continue;
            }
        }

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_BY_ID);
        stmt->setUInt32(0, m->messageID);
        CharacterDatabase.Execute(stmt);
        delete m;
        ++deletedCount;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Processed %u expired mails: %u deleted and %u returned in %u ms", deletedCount + returnedCount, deletedCount, returnedCount, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadQuestAreaTriggers()
{
    uint32 oldMSTime = getMSTime();

    _questAreaTriggerStore.clear();                           // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT id, quest FROM areatrigger_involvedrelation");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 quest trigger points. DB table `areatrigger_involvedrelation` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;

        Field* fields = result->Fetch();

        uint32 trigger_ID = fields[0].GetUInt32();
        uint32 quest_ID   = fields[1].GetUInt32();

        AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(trigger_ID);
        if (!atEntry)
        {
            TC_LOG_ERROR("sql.sql", "Area trigger (ID:%u) does not exist in `AreaTrigger.dbc`.", trigger_ID);
            continue;
        }

        Quest const* quest = GetQuestTemplate(quest_ID);

        if (!quest)
        {
            TC_LOG_ERROR("sql.sql", "Table `areatrigger_involvedrelation` has record (id: %u) for not existing quest %u", trigger_ID, quest_ID);
            continue;
        }

        if (!quest->HasFlag(QUEST_FLAGS_COMPLETION_AREA_TRIGGER))
        {
            TC_LOG_ERROR("sql.sql", "Table `areatrigger_involvedrelation` has record (id: %u) for not quest %u, but quest not have flag QUEST_FLAGS_COMPLETION_AREA_TRIGGER. Trigger or quest flags must be fixed, quest modified to require objective.", trigger_ID, quest_ID);
            continue;
        }

        _questAreaTriggerStore[trigger_ID] = quest_ID;
        const_cast<Quest*>(quest)->SetStartAtAreaTrigger();

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u quest trigger points in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

QuestGreeting const* ObjectMgr::GetQuestGreeting(ObjectGuid guid) const
{
    auto itr = _questGreetingStore.find(guid.GetTypeId());
    if (itr == _questGreetingStore.end())
        return nullptr;

    auto questItr = itr->second.find(guid.GetEntry());
    if (questItr == itr->second.end())
        return nullptr;

    return &questItr->second;
}

void ObjectMgr::LoadQuestGreetings()
{
    uint32 oldMSTime = getMSTime();

    _questGreetingStore.clear(); // need for reload case

    //                                                0   1          2                3             4
    QueryResult result = WorldDatabase.Query("SELECT ID, Type, GreetEmoteType, GreetEmoteDelay, Greeting FROM quest_greeting");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 quest greetings. DB table `quest_greeting` is empty.");
        return;
    }

    _questGreetingStore.reserve(result->GetRowCount());

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 id                   = fields[0].GetUInt32();
        uint8 type                  = fields[1].GetUInt8();
        // overwrite
        switch (type)
        {
            case 0: // Creature
                type = TYPEID_UNIT;
                if (!sObjectMgr->GetCreatureTemplate(id))
                {
                    TC_LOG_ERROR("sql.sql", "Table `quest_greeting`: creature template (entry: %u) does not exist.", id);
                    continue;
                }
                break;
            case 1: // GameObject
                type = TYPEID_GAMEOBJECT;
                if (!sObjectMgr->GetGameObjectTemplate(id))
                {
                    TC_LOG_ERROR("sql.sql", "Table `quest_greeting`: gameobject template (entry: %u) does not exist.", id);
                    continue;
                }
                break;
            default:
                TC_LOG_ERROR("sql.sql", "Table `quest_greeting`: unknown type = %u for entry = %u. Skipped.", type, id);
                continue;
        }

        uint16 greetEmoteType       = fields[2].GetUInt16();

        if (greetEmoteType > 0 && !sEmotesStore.LookupEntry(greetEmoteType))
        {
            TC_LOG_DEBUG("sql.sql", "Table `quest_greeting`: entry %u has greetEmoteType = %u but emote does not exist. Set to 0.", id, greetEmoteType);
            greetEmoteType = 0;
        }

        uint32 greetEmoteDelay      = fields[3].GetUInt32();
        std::string greeting        = fields[4].GetString();

        _questGreetingStore[type][id] = QuestGreeting(greetEmoteType, greetEmoteDelay, greeting);

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u quest_greeting in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadQuestGreetingsLocales()
{
    uint32 oldMSTime = getMSTime();

    _questGreetingLocaleStore.clear();                              // need for reload case

    //                                               0     1      2       3
    QueryResult result = WorldDatabase.Query("SELECT ID, Type, Locale, Greeting FROM quest_greeting_locale");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 quest_greeting locales. DB table `quest_greeting_locale` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 id                   = fields[0].GetUInt32();
        uint8 type                  = fields[1].GetUInt8();
        // overwrite
        switch (type)
        {
            case 0: // Creature
                type = TYPEID_UNIT;
                break;
            case 1: // GameObject
                type = TYPEID_GAMEOBJECT;
                break;
            default:
                break;
        }

        std::string localeName      = fields[2].GetString();

        LocaleConstant locale = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        QuestGreetingLocale& data = _questGreetingLocaleStore[MAKE_PAIR32(id, type)];
        AddLocaleString(fields[3].GetString(), locale, data.greeting);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u quest greeting locale strings in %u ms", uint32(_questGreetingLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadTavernAreaTriggers()
{
    uint32 oldMSTime = getMSTime();

    _tavernAreaTriggerStore.clear();                          // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT id FROM areatrigger_tavern");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 tavern triggers. DB table `areatrigger_tavern` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;

        Field* fields = result->Fetch();

        uint32 Trigger_ID      = fields[0].GetUInt32();

        AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(Trigger_ID);
        if (!atEntry)
        {
            TC_LOG_ERROR("sql.sql", "Area trigger (ID:%u) does not exist in `AreaTrigger.dbc`.", Trigger_ID);
            continue;
        }

        _tavernAreaTriggerStore.insert(Trigger_ID);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u tavern triggers in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadAreaTriggerScripts()
{
    uint32 oldMSTime = getMSTime();

    _areaTriggerScriptStore.clear();                            // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry, ScriptName FROM areatrigger_scripts");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 areatrigger scripts. DB table `areatrigger_scripts` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 triggerId             = fields[0].GetUInt32();
        std::string const scriptName = fields[1].GetString();

        AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(triggerId);
        if (!atEntry)
        {
            TC_LOG_ERROR("sql.sql", "AreaTrigger (ID: %u) does not exist in `AreaTrigger.dbc`.", triggerId);
            continue;
        }
        _areaTriggerScriptStore[triggerId] = GetScriptId(scriptName);
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " areatrigger scripts in %u ms", _areaTriggerScriptStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

uint32 ObjectMgr::GetNearestTaxiNode(float x, float y, float z, uint32 mapid, uint32 team)
{
    bool found = false;
    float dist = 10000;
    uint32 id = 0;

    for (uint32 i = 1; i < sTaxiNodesStore.GetNumRows(); ++i)
    {
        TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(i);

        if (!node || node->ContinentID != mapid || (!node->MountCreatureID[team == ALLIANCE ? 1 : 0] && node->MountCreatureID[0] != 32981)) // dk flight
            continue;

        uint8  field   = (uint8)((i - 1) / 8);
        uint32 submask = 1 << ((i-1) % 8);

        // skip not taxi network nodes
        if ((sTaxiNodesMask[field] & submask) == 0)
            continue;

        float dist2 = (node->Pos.X - x)*(node->Pos.X - x)+(node->Pos.Y - y)*(node->Pos.Y - y)+(node->Pos.Z - z)*(node->Pos.Z - z);
        if (found)
        {
            if (dist2 < dist)
            {
                dist = dist2;
                id = i;
            }
        }
        else
        {
            found = true;
            dist = dist2;
            id = i;
        }
    }

    return id;
}

void ObjectMgr::GetTaxiPath(uint32 source, uint32 destination, uint32 &path, uint32 &cost)
{
    TaxiPathSetBySource::iterator src_i = sTaxiPathSetBySource.find(source);
    if (src_i == sTaxiPathSetBySource.end())
    {
        path = 0;
        cost = 0;
        return;
    }

    TaxiPathSetForSource& pathSet = src_i->second;

    TaxiPathSetForSource::iterator dest_i = pathSet.find(destination);
    if (dest_i == pathSet.end())
    {
        path = 0;
        cost = 0;
        return;
    }

    cost = dest_i->second.price;
    path = dest_i->second.ID;
}

uint32 ObjectMgr::GetTaxiMountDisplayId(uint32 id, uint32 team, bool allowed_alt_team /* = false */)
{
    CreatureModel mountModel;
    CreatureTemplate const* mount_info = nullptr;

    // select mount creature id
    TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(id);
    if (node)
    {
        uint32 mount_entry = 0;
        if (team == ALLIANCE)
            mount_entry = node->MountCreatureID[1];
        else
            mount_entry = node->MountCreatureID[0];

        // Fix for Alliance not being able to use Acherus taxi
        // only one mount type for both sides
        if (mount_entry == 0 && allowed_alt_team)
        {
            // Simply reverse the selection. At least one team in theory should have a valid mount ID to choose.
            mount_entry = team == ALLIANCE ? node->MountCreatureID[0] : node->MountCreatureID[1];
        }

        mount_info = GetCreatureTemplate(mount_entry);
        if (mount_info)
        {
            CreatureModel const* model = mount_info->GetRandomValidModel();
            if (!model)
            {
                TC_LOG_ERROR("sql.sql", "No displayid found for the taxi mount with the entry %u! Can't load it!", mount_entry);
                return 0;
            }
            mountModel = *model;
        }
    }

    // minfo is not actually used but the mount_id was updated
    GetCreatureModelRandomGender(&mountModel, mount_info);

    return mountModel.CreatureDisplayID;
}

Quest const* ObjectMgr::GetQuestTemplate(uint32 quest_id) const
{
    return Trinity::Containers::MapGetValuePtr(_questTemplates, quest_id);
}

void ObjectMgr::LoadGraveyardZones()
{
    uint32 oldMSTime = getMSTime();

    GraveyardStore.clear(); // need for reload case

    //                                               0   1          2
    QueryResult result = WorldDatabase.Query("SELECT ID, GhostZone, Faction FROM graveyard_zone");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 graveyard-zone links. DB table `graveyard_zone` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;

        Field* fields = result->Fetch();

        uint32 safeLocId = fields[0].GetUInt32();
        uint32 zoneId = fields[1].GetUInt32();
        uint32 team   = fields[2].GetUInt16();

        WorldSafeLocsEntry const* entry = sWorldSafeLocsStore.LookupEntry(safeLocId);
        if (!entry)
        {
            TC_LOG_ERROR("sql.sql", "Table `graveyard_zone` has a record for non-existing graveyard (WorldSafeLocsID: %u), skipped.", safeLocId);
            continue;
        }

        AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(zoneId);
        if (!areaEntry)
        {
            TC_LOG_ERROR("sql.sql", "Table `graveyard_zone` has a record for non-existing Zone (ID: %u), skipped.", zoneId);
            continue;
        }

        if (areaEntry->ParentAreaID != 0)
        {
            TC_LOG_ERROR("sql.sql", "Table `graveyard_zone` has a record for SubZone (ID: %u) instead of zone, skipped.", zoneId);
            continue;
        }

        if (team != 0 && team != HORDE && team != ALLIANCE)
        {
            TC_LOG_ERROR("sql.sql", "Table `graveyard_zone` has a record for non player faction (%u), skipped.", team);
            continue;
        }

        if (!AddGraveyardLink(safeLocId, zoneId, team, false))
            TC_LOG_ERROR("sql.sql", "Table `graveyard_zone` has a duplicate record for Graveyard (ID: %u) and Zone (ID: %u), skipped.", safeLocId, zoneId);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u graveyard-zone links in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

WorldSafeLocsEntry const* ObjectMgr::GetDefaultGraveyard(uint32 team) const
{
    enum DefaultGraveyard
    {
        HORDE_GRAVEYARD    = 10, // Crossroads
        ALLIANCE_GRAVEYARD = 4   // Westfall
    };

    if (team == HORDE)
        return sWorldSafeLocsStore.LookupEntry(HORDE_GRAVEYARD);
    else if (team == ALLIANCE)
        return sWorldSafeLocsStore.LookupEntry(ALLIANCE_GRAVEYARD);
    else return nullptr;
}

WorldSafeLocsEntry const* ObjectMgr::GetClosestGraveyard(WorldLocation const& location, uint32 team, WorldObject* conditionObject) const
{
    float x, y, z;
    location.GetPosition(x, y, z);
    uint32 MapId = location.GetMapId();

    // search for zone associated closest graveyard
    uint32 zoneId = sTerrainMgr.GetZoneId(conditionObject ? conditionObject->GetPhaseShift() : PhasingHandler::GetEmptyPhaseShift(), MapId, x, y, z);

    if (!zoneId)
    {
        if (z > -500)
        {
            TC_LOG_ERROR("misc", "ZoneId not found for map %u coords (%f, %f, %f)", MapId, x, y, z);
            return GetDefaultGraveyard(team);
        }
    }

    WorldSafeLocsEntry const* graveyard = GetClosestGraveyardInZone(location, team, conditionObject, zoneId);
    AreaTableEntry const* zoneEntry = sAreaTableStore.AssertEntry(zoneId);
    AreaTableEntry const* parentEntry = sAreaTableStore.LookupEntry(zoneEntry->ParentAreaID);

    while (!graveyard && parentEntry)
    {
        graveyard = GetClosestGraveyardInZone(location, team, conditionObject, parentEntry->ID);
        if (!graveyard && parentEntry->ParentAreaID != 0)
            parentEntry = sAreaTableStore.LookupEntry(parentEntry->ParentAreaID);
        else // nothing found, cant look further, give up.
            parentEntry = nullptr;
    }

    return graveyard;
}

WorldSafeLocsEntry const* ObjectMgr::GetClosestGraveyardInZone(WorldLocation const& location, uint32 team, WorldObject* conditionObject, uint32 zoneId) const
{
    float x, y, z;
    location.GetPosition(x, y, z);
    uint32 MapId = location.GetMapId();
    // Simulate std. algorithm:
    //   found some graveyard associated to (ghost_zone, ghost_map)
    //
    //   if mapId == graveyard.mapId (ghost in plain zone or city or battleground) and search graveyard at same map
    //     then check faction
    //   if mapId != graveyard.mapId (ghost in instance) and search any graveyard associated
    //     then check faction
    GraveyardMapBounds range = GraveyardStore.equal_range(zoneId);
    MapEntry const* mapEntry = sMapStore.LookupEntry(MapId);

    // not need to check validity of map object; MapId _MUST_ be valid here
    if (range.first == range.second && !mapEntry->IsBattlegroundOrArena())
    {
        if (zoneId != 0) // zone == 0 can't be fixed, used by bliz for bugged zones
            TC_LOG_ERROR("sql.sql", "Table `graveyard_zone` incomplete: Zone %u Team %u does not have a linked graveyard.", zoneId, team);
        return GetDefaultGraveyard(team);
    }

    // at corpse map
    bool foundNear = false;
    float distNear = 10000;
    WorldSafeLocsEntry const* entryNear = nullptr;

    // at entrance map for corpse map
    bool foundEntr = false;
    float distEntr = 10000;
    WorldSafeLocsEntry const* entryEntr = nullptr;

    // some where other
    WorldSafeLocsEntry const* entryFar = nullptr;

    ConditionSourceInfo conditionSource(conditionObject);

    for (; range.first != range.second; ++range.first)
    {
        GraveyardData const& data = range.first->second;

        WorldSafeLocsEntry const* entry = sWorldSafeLocsStore.AssertEntry(data.safeLocId);

        // skip enemy faction graveyard
        // team == 0 case can be at call from .neargrave
        if (data.team != 0 && team != 0 && data.team != team)
            continue;

        if (conditionObject)
        {
            if (!sConditionMgr->IsObjectMeetingNotGroupedConditions(CONDITION_SOURCE_TYPE_GRAVEYARD, data.safeLocId, conditionSource))
                continue;

            if (int16(entry->Continent) == mapEntry->ParentMapID && !conditionObject->GetPhaseShift().HasVisibleMapId(entry->Continent))
                continue;
        }

        // find now nearest graveyard at other map
        if (MapId != entry->Continent && mapEntry && int16(entry->Continent) != mapEntry->ParentMapID)
        {
            // if find graveyard at different map from where entrance placed (or no entrance data), use any first
            if (!mapEntry
                || mapEntry->CorpseMapID < 0
                || uint32(mapEntry->CorpseMapID) != entry->Continent
                || (mapEntry->Corpse.X == 0 && mapEntry->Corpse.Y == 0))
            {
                // not have any corrdinates for check distance anyway
                entryFar = entry;
                continue;
            }

            // at entrance map calculate distance (2D);
            float dist2 = (entry->Loc.X - mapEntry->Corpse.X)*(entry->Loc.X - mapEntry->Corpse.X)
                +(entry->Loc.Y - mapEntry->Corpse.Y)*(entry->Loc.Y - mapEntry->Corpse.Y);
            if (foundEntr)
            {
                if (dist2 < distEntr)
                {
                    distEntr = dist2;
                    entryEntr = entry;
                }
            }
            else
            {
                foundEntr = true;
                distEntr = dist2;
                entryEntr = entry;
            }
        }
        // find now nearest graveyard at same map
        else
        {
            float dist2 = (entry->Loc.X - x)*(entry->Loc.X - x)+(entry->Loc.Y - y)*(entry->Loc.Y - y)+(entry->Loc.Z - z)*(entry->Loc.Z - z);
            if (foundNear)
            {
                if (dist2 < distNear)
                {
                    distNear = dist2;
                    entryNear = entry;
                }
            }
            else
            {
                foundNear = true;
                distNear = dist2;
                entryNear = entry;
            }
        }
    }

    if (entryNear)
        return entryNear;

    if (entryEntr)
        return entryEntr;

    return entryFar;
}

GraveyardData const* ObjectMgr::FindGraveyardData(uint32 id, uint32 zoneId) const
{
    GraveyardMapBounds range = GraveyardStore.equal_range(zoneId);
    for (; range.first != range.second; ++range.first)
    {
        GraveyardData const& data = range.first->second;
        if (data.safeLocId == id)
            return &data;
    }
    return nullptr;
}

AreaTriggerStruct const* ObjectMgr::GetAreaTrigger(uint32 trigger) const
{
    return Trinity::Containers::MapGetValuePtr(_areaTriggerStore, trigger);
}

AccessRequirement const* ObjectMgr::GetAccessRequirement(uint32 mapid, Difficulty difficulty) const
{
    return Trinity::Containers::MapGetValuePtr(_accessRequirementStore, MAKE_PAIR32(mapid, difficulty));
}

bool ObjectMgr::AddGraveyardLink(uint32 id, uint32 zoneId, uint32 team, bool persist /*= true*/)
{
    if (FindGraveyardData(id, zoneId))
        return false;

    // add link to loaded data
    GraveyardData data;
    data.safeLocId = id;
    data.team = team;

    GraveyardStore.insert(GraveyardContainer::value_type(zoneId, data));

    // add link to DB
    if (persist)
    {
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_GRAVEYARD_ZONE);

        stmt->setUInt32(0, id);
        stmt->setUInt32(1, zoneId);
        stmt->setUInt16(2, uint16(team));

        WorldDatabase.Execute(stmt);
    }

    return true;
}

void ObjectMgr::RemoveGraveyardLink(uint32 id, uint32 zoneId, uint32 team, bool persist /*= false*/)
{
    GraveyardMapBoundsNonConst range = GraveyardStore.equal_range(zoneId);
    if (range.first == range.second)
    {
        //TC_LOG_ERROR("sql.sql", "Table `graveyard_zone` incomplete: Zone %u Team %u does not have a linked graveyard.", zoneId, team);
        return;
    }

    bool found = false;


    for (; range.first != range.second; ++range.first)
    {
        GraveyardData & data = range.first->second;

        // skip not matching safezone id
        if (data.safeLocId != id)
            continue;

        // skip enemy faction graveyard at same map (normal area, city, or battleground)
        // team == 0 case can be at call from .neargrave
        if (data.team != 0 && team != 0 && data.team != team)
            continue;

        found = true;
        break;
    }

    // no match, return
    if (!found)
        return;

    // remove from links
    GraveyardStore.erase(range.first);

    // remove link from DB
    if (persist)
    {
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_GRAVEYARD_ZONE);

        stmt->setUInt32(0, id);
        stmt->setUInt32(1, zoneId);
        stmt->setUInt16(2, uint16(team));

        WorldDatabase.Execute(stmt);
    }
}

void ObjectMgr::LoadAreaTriggerTeleports()
{
    uint32 oldMSTime = getMSTime();

    _areaTriggerStore.clear();                                  // need for reload case

    //                                               0        1              2                  3                  4                   5
    QueryResult result = WorldDatabase.Query("SELECT ID,  target_map, target_position_x, target_position_y, target_position_z, target_orientation FROM areatrigger_teleport");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 area trigger teleport definitions. DB table `areatrigger_teleport` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        ++count;

        uint32 Trigger_ID = fields[0].GetUInt32();

        AreaTriggerStruct at;

        at.target_mapId             = fields[1].GetUInt16();
        at.target_X                 = fields[2].GetFloat();
        at.target_Y                 = fields[3].GetFloat();
        at.target_Z                 = fields[4].GetFloat();
        at.target_Orientation       = fields[5].GetFloat();

        AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(Trigger_ID);
        if (!atEntry)
        {
            TC_LOG_ERROR("sql.sql", "Area trigger (ID:%u) does not exist in `AreaTrigger.dbc`.", Trigger_ID);
            continue;
        }

        MapEntry const* mapEntry = sMapStore.LookupEntry(at.target_mapId);
        if (!mapEntry)
        {
            TC_LOG_ERROR("sql.sql", "Area trigger (ID:%u) target map (ID: %u) does not exist in `Map.dbc`.", Trigger_ID, at.target_mapId);
            continue;
        }

        if (at.target_X == 0 && at.target_Y == 0 && at.target_Z == 0)
        {
            TC_LOG_ERROR("sql.sql", "Area trigger (ID:%u) target coordinates not provided.", Trigger_ID);
            continue;
        }

        _areaTriggerStore[Trigger_ID] = at;

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u area trigger teleport definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadAccessRequirements()
{
    uint32 oldMSTime = getMSTime();

    if (!_accessRequirementStore.empty())
    {
        for (AccessRequirementContainer::iterator itr = _accessRequirementStore.begin(); itr != _accessRequirementStore.end(); ++itr)
            delete itr->second;

        _accessRequirementStore.clear();                                  // need for reload case
    }

    //                                               0      1           2          3          4     5      6             7             8                      9
    QueryResult result = WorldDatabase.Query("SELECT mapid, difficulty, level_min, level_max, item, item2, quest_done_A, quest_done_H, completed_achievement, heroic_exclusive FROM access_requirement");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 access requirement definitions. DB table `access_requirement` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        ++count;

        uint32 mapid = fields[0].GetUInt32();
        uint8 difficulty = fields[1].GetUInt8();
        uint32 requirement_ID = MAKE_PAIR32(mapid, difficulty);

        AccessRequirement* ar   = new AccessRequirement();
        ar->levelMin            = fields[2].GetUInt8();
        ar->levelMax            = fields[3].GetUInt8();
        ar->item                = fields[4].GetUInt32();
        ar->item2               = fields[5].GetUInt32();
        ar->quest_A             = fields[6].GetUInt32();
        ar->quest_H             = fields[7].GetUInt32();
        ar->achievement         = fields[8].GetUInt32();
        ar->heroicExclusive     = fields[9].GetUInt8();

        if (ar->item)
        {
            ItemTemplate const* pProto = GetItemTemplate(ar->item);
            if (!pProto)
            {
                TC_LOG_ERROR("misc", "Key item %u does not exist for map %u difficulty %u, removing key requirement.", ar->item, mapid, difficulty);
                ar->item = 0;
            }
        }

        if (ar->item2)
        {
            ItemTemplate const* pProto = GetItemTemplate(ar->item2);
            if (!pProto)
            {
                TC_LOG_ERROR("misc", "Second item %u does not exist for map %u difficulty %u, removing key requirement.", ar->item2, mapid, difficulty);
                ar->item2 = 0;
            }
        }

        if (ar->quest_A)
        {
            if (!GetQuestTemplate(ar->quest_A))
            {
                TC_LOG_ERROR("sql.sql", "Required Alliance Quest %u not exist for map %u difficulty %u, remove quest done requirement.", ar->quest_A, mapid, difficulty);
                ar->quest_A = 0;
            }
        }

        if (ar->quest_H)
        {
            if (!GetQuestTemplate(ar->quest_H))
            {
                TC_LOG_ERROR("sql.sql", "Required Horde Quest %u not exist for map %u difficulty %u, remove quest done requirement.", ar->quest_H, mapid, difficulty);
                ar->quest_H = 0;
            }
        }

        if (ar->achievement)
        {
            if (!sAchievementMgr->GetAchievement(ar->achievement))
            {
                TC_LOG_ERROR("sql.sql", "Required Achievement %u not exist for map %u difficulty %u, remove quest done requirement.", ar->achievement, mapid, difficulty);
                ar->achievement = 0;
            }
        }

        _accessRequirementStore[requirement_ID] = ar;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u access requirement definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

/*
 * Searches for the areatrigger which teleports players out of the given map with instance_template.parent field support
 */
AreaTriggerStruct const* ObjectMgr::GetGoBackTrigger(uint32 map) const
{
    Optional<uint32> parentId;
    MapEntry const* mapEntry = sMapStore.LookupEntry(map);
    if (!mapEntry || mapEntry->CorpseMapID < 0)
        return nullptr;

    if (mapEntry->IsDungeon())
        if (InstanceTemplate const* iTemplate = sObjectMgr->GetInstanceTemplate(map))
            parentId = iTemplate->Parent;

    uint32 entrance_map = parentId.value_or(mapEntry->CorpseMapID);

    for (AreaTriggerContainer::const_iterator itr = _areaTriggerStore.begin(); itr != _areaTriggerStore.end(); ++itr)
    {
        if (itr->second.target_mapId == entrance_map)
        {
            AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(itr->first);
            if (atEntry && atEntry->ContinentID == map)
                return &itr->second;
        }
    }
    return nullptr;
}

/**
 * Searches for the areatrigger which teleports players to the given map
 */
AreaTriggerStruct const* ObjectMgr::GetMapEntranceTrigger(uint32 Map) const
{
    for (AreaTriggerContainer::const_iterator itr = _areaTriggerStore.begin(); itr != _areaTriggerStore.end(); ++itr)
    {
        if (itr->second.target_mapId == Map)
        {
            AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(itr->first);
            if (atEntry)
                return &itr->second;
        }
    }
    return nullptr;
}

void ObjectMgr::SetHighestGuids()
{
    QueryResult result = CharacterDatabase.Query("SELECT MAX(guid) FROM characters");
    if (result)
        GetGuidSequenceGenerator<HighGuid::Player>().Set((*result)[0].GetUInt32() + 1);

    result = CharacterDatabase.Query("SELECT MAX(guid) FROM item_instance");
    if (result)
        GetGuidSequenceGenerator<HighGuid::Item>().Set((*result)[0].GetUInt32() + 1);

    // Cleanup other tables from nonexistent guids ( >= _hiItemGuid)
    uint32 hiItemGuid = GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed();
    CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item >= '%u'", hiItemGuid);      // One-time query
    CharacterDatabase.PExecute("DELETE FROM mail_items WHERE item_guid >= '%u'", hiItemGuid);          // One-time query
    CharacterDatabase.PExecute("DELETE FROM auctionhouse WHERE itemguid >= '%u'", hiItemGuid);         // One-time query
    CharacterDatabase.PExecute("DELETE FROM guild_bank_item WHERE item_guid >= '%u'", hiItemGuid);     // One-time query

    result = WorldDatabase.Query("SELECT MAX(guid) FROM transports");
    if (result)
        GetGuidSequenceGenerator<HighGuid::Mo_Transport>().Set((*result)[0].GetUInt32() + 1);

    result = CharacterDatabase.Query("SELECT MAX(id) FROM auctionhouse");
    if (result)
        _auctionId = (*result)[0].GetUInt32() + 1;

    result = CharacterDatabase.Query("SELECT MAX(id) FROM mail");
    if (result)
        _mailId = (*result)[0].GetUInt32() + 1;

    result = CharacterDatabase.Query("SELECT MAX(arenateamid) FROM arena_team");
    if (result)
        sArenaTeamMgr->SetNextArenaTeamId((*result)[0].GetUInt32() + 1);

    result = CharacterDatabase.Query("SELECT MAX(setguid) FROM character_equipmentsets");
    if (result)
        _equipmentSetGuid = (*result)[0].GetUInt64() + 1;

    result = CharacterDatabase.Query("SELECT MAX(guildId) FROM guild");
    if (result)
        sGuildMgr->SetNextGuildId((*result)[0].GetUInt32() + 1);

    result = CharacterDatabase.Query("SELECT MAX(guid) FROM `groups`");
    if (result)
        sGroupMgr->SetGroupDbStoreSize((*result)[0].GetUInt32() + 1);

    result = CharacterDatabase.Query("SELECT MAX(itemId) from character_void_storage");
    if (result)
        _voidItemId = (*result)[0].GetUInt64() + 1;

    result = WorldDatabase.Query("SELECT MAX(guid) FROM creature");
    if (result)
        _creatureSpawnId = (*result)[0].GetUInt32() + 1;

    result = WorldDatabase.Query("SELECT MAX(guid) FROM gameobject");
    if (result)
        _gameObjectSpawnId = (*result)[0].GetUInt32() + 1;
}

uint32 ObjectMgr::GenerateAuctionID()
{
    if (_auctionId >= 0xFFFFFFFE)
    {
        TC_LOG_ERROR("misc", "Auctions ids overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _auctionId++;
}

uint64 ObjectMgr::GenerateEquipmentSetGuid()
{
    if (_equipmentSetGuid >= uint64(0xFFFFFFFFFFFFFFFELL))
    {
        TC_LOG_ERROR("misc", "EquipmentSet guid overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _equipmentSetGuid++;
}

uint32 ObjectMgr::GenerateMailID()
{
    if (_mailId >= 0xFFFFFFFE)
    {
        TC_LOG_ERROR("misc", "Mail ids overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _mailId++;
}

uint32 ObjectMgr::GeneratePetNumber()
{
    if (_hiPetNumber >= 0xFFFFFFFE)
    {
        TC_LOG_ERROR("misc", "_hiPetNumber Id overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _hiPetNumber++;
}

uint64 ObjectMgr::GenerateVoidStorageItemId()
{
    return ++_voidItemId;
}

uint32 ObjectMgr::GenerateCreatureSpawnId()
{
    if (_creatureSpawnId >= uint32(0xFFFFFF))
    {
        TC_LOG_ERROR("misc", "Creature spawn id overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _creatureSpawnId++;
}

uint32 ObjectMgr::GenerateGameObjectSpawnId()
{
    if (_gameObjectSpawnId >= uint32(0xFFFFFF))
    {
        TC_LOG_ERROR("misc", "GameObject spawn id overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _gameObjectSpawnId++;
}

void ObjectMgr::LoadGameObjectLocales()
{
    uint32 oldMSTime = getMSTime();

    _gameObjectLocaleStore.clear(); // need for reload case

    //                                               0      1       2     3
    QueryResult result = WorldDatabase.Query("SELECT entry, locale, name, castBarCaption FROM gameobject_template_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 id                   = fields[0].GetUInt32();
        std::string localeName      = fields[1].GetString();

        LocaleConstant locale = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        GameObjectLocale& data = _gameObjectLocaleStore[id];
        AddLocaleString(fields[2].GetString(), locale, data.Name);
        AddLocaleString(fields[3].GetString(), locale, data.CastBarCaption);

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u gameobject_template_locale strings in %u ms", uint32(_gameObjectLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

inline void CheckGOLockId(GameObjectTemplate const* goInfo, uint32 dataN, uint32 N)
{
    if (sLockStore.LookupEntry(dataN))
        return;

    TC_LOG_ERROR("sql.sql", "Gameobject (Entry: %u GoType: %u) have data%d=%u but lock (Id: %u) not found.",
        goInfo->entry, goInfo->type, N, goInfo->door.open, goInfo->door.open);
}

inline void CheckGOLinkedTrapId(GameObjectTemplate const* goInfo, uint32 dataN, uint32 N)
{
    if (GameObjectTemplate const* trapInfo = sObjectMgr->GetGameObjectTemplate(dataN))
    {
        if (trapInfo->type != GAMEOBJECT_TYPE_TRAP)
            TC_LOG_ERROR("sql.sql", "Gameobject (Entry: %u GoType: %u) have data%d=%u but GO (Entry %u) have not GAMEOBJECT_TYPE_TRAP (%u) type.",
            goInfo->entry, goInfo->type, N, dataN, dataN, GAMEOBJECT_TYPE_TRAP);
    }
}

inline void CheckGOSpellId(GameObjectTemplate const* goInfo, uint32 dataN, uint32 N)
{
    if (sSpellMgr->GetSpellInfo(dataN) || dataN == 0)
        return;

    TC_LOG_ERROR("sql.sql", "Gameobject (Entry: %u GoType: %u) have data%d=%u but Spell (Entry %u) not exist.",
        goInfo->entry, goInfo->type, N, dataN, dataN);
}

inline void CheckAndFixGOChairHeightId(GameObjectTemplate const* goInfo, uint32 const& dataN, uint32 N)
{
    if (dataN <= (UNIT_STAND_STATE_SIT_HIGH_CHAIR-UNIT_STAND_STATE_SIT_LOW_CHAIR))
        return;

    TC_LOG_ERROR("sql.sql", "Gameobject (Entry: %u GoType: %u) have data%d=%u but correct chair height in range 0..%i.",
        goInfo->entry, goInfo->type, N, dataN, UNIT_STAND_STATE_SIT_HIGH_CHAIR-UNIT_STAND_STATE_SIT_LOW_CHAIR);

    // prevent client and server unexpected work
    const_cast<uint32&>(dataN) = 0;
}

inline void CheckGONoDamageImmuneId(GameObjectTemplate* goTemplate, uint32 dataN, uint32 N)
{
    // 0/1 correct values
    if (dataN <= 1)
        return;

    TC_LOG_ERROR("sql.sql", "Gameobject (Entry: %u GoType: %u) have data%d=%u but expected boolean (0/1) noDamageImmune field value.", goTemplate->entry, goTemplate->type, N, dataN);
}

inline void CheckGOConsumable(GameObjectTemplate const* goInfo, uint32 dataN, uint32 N)
{
    // 0/1 correct values
    if (dataN <= 1)
        return;

    TC_LOG_ERROR("sql.sql", "Gameobject (Entry: %u GoType: %u) have data%d=%u but expected boolean (0/1) consumable field value.",
        goInfo->entry, goInfo->type, N, dataN);
}

void ObjectMgr::LoadGameObjectTemplate()
{
    uint32 oldMSTime = getMSTime();

    //                                               0      1     2          3     4         5               6     7
    QueryResult result = WorldDatabase.Query("SELECT entry, type, displayId, name, IconName, castBarCaption, unk1, size, "
    //                                        8      9      10     11     12     13     14     15     16     17     18      19      20
                                             "Data0, Data1, Data2, Data3, Data4, Data5, Data6, Data7, Data8, Data9, Data10, Data11, Data12, "
    //                                        21      22      23      24      25      26      27      28      29      30      31      32      33      34      35      36
                                             "Data13, Data14, Data15, Data16, Data17, Data18, Data19, Data20, Data21, Data22, Data23, Data24, Data25, Data26, Data27, Data28, "
    //                                        37      38      39      40        41      42
                                             "Data29, Data30, Data31, RequiredLevel, AIName, ScriptName "
                                             "FROM gameobject_template");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 gameobject definitions. DB table `gameobject_template` is empty.");
        return;
    }

    _gameObjectTemplateStore.reserve(result->GetRowCount());
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        GameObjectTemplate& got = _gameObjectTemplateStore[entry];

        got.entry          = entry;
        got.type           = uint32(fields[1].GetUInt8());
        got.displayId      = fields[2].GetUInt32();
        got.name           = fields[3].GetString();
        got.IconName       = fields[4].GetString();
        got.castBarCaption = fields[5].GetString();
        got.unk1           = fields[6].GetString();
        got.size           = fields[7].GetFloat();

        for (uint8 i = 0; i < MAX_GAMEOBJECT_DATA; ++i)
            got.raw.data[i] = fields[8 + i].GetUInt32();

        got.RequiredLevel = fields[40].GetInt32();
        got.AIName = fields[41].GetString();
        got.ScriptId = GetScriptId(fields[42].GetCString());

        // Checks
        if (!got.AIName.empty() && !sGameObjectAIRegistry->HasItem(got.AIName))
        {
            TC_LOG_ERROR("sql.sql", "GameObject (Entry: %u) has non-registered `AIName` '%s' set, removing", got.entry, got.AIName.c_str());
            got.AIName.clear();
        }

        switch (got.type)
        {
            case GAMEOBJECT_TYPE_DOOR:                      //0
            {
                if (got.door.open)
                    CheckGOLockId(&got, got.door.open, 1);
                CheckGONoDamageImmuneId(&got, got.door.noDamageImmune, 3);
                break;
            }
            case GAMEOBJECT_TYPE_BUTTON:                    //1
            {
                if (got.button.open)
                    CheckGOLockId(&got, got.button.open, 1);
                CheckGONoDamageImmuneId(&got, got.button.noDamageImmune, 4);
                break;
            }
            case GAMEOBJECT_TYPE_QUESTGIVER:                //2
            {
                if (got.questgiver.open)
                    CheckGOLockId(&got, got.questgiver.open, 0);
                CheckGONoDamageImmuneId(&got, got.questgiver.noDamageImmune, 5);
                break;
            }
            case GAMEOBJECT_TYPE_CHEST:                     //3
            {
                if (got.chest.open)
                    CheckGOLockId(&got, got.chest.open, 0);

                CheckGOConsumable(&got, got.chest.consumable, 3);

                if (got.chest.linkedTrap)              // linked trap
                    CheckGOLinkedTrapId(&got, got.chest.linkedTrap, 7);
                break;
            }
            case GAMEOBJECT_TYPE_TRAP:                      //6
            {
                if (got.trap.open)
                    CheckGOLockId(&got, got.trap.open, 0);
                break;
            }
            case GAMEOBJECT_TYPE_CHAIR:                     //7
                CheckAndFixGOChairHeightId(&got, got.chair.chairheight, 1);
                break;
            case GAMEOBJECT_TYPE_SPELL_FOCUS:               //8
            {
                if (got.spellFocus.spellFocusType)
                {
                    if (!sSpellFocusObjectStore.LookupEntry(got.spellFocus.spellFocusType))
                        TC_LOG_ERROR("sql.sql", "GameObject (Entry: %u GoType: %u) have data0=%u but SpellFocus (Id: %u) not exist.",
                        entry, got.type, got.spellFocus.spellFocusType, got.spellFocus.spellFocusType);
                }

                if (got.spellFocus.linkedTrap)        // linked trap
                    CheckGOLinkedTrapId(&got, got.spellFocus.linkedTrap, 2);
                break;
            }
            case GAMEOBJECT_TYPE_GOOBER:                    //10
            {
                if (got.goober.open)
                    CheckGOLockId(&got, got.goober.open, 0);

                CheckGOConsumable(&got, got.goober.consumable, 3);

                if (got.goober.pageID)                  // pageId
                {
                    if (!GetPageText(got.goober.pageID))
                        TC_LOG_ERROR("sql.sql", "GameObject (Entry: %u GoType: %u) have data7=%u but PageText (Entry %u) not exist.",
                        entry, got.type, got.goober.pageID, got.goober.pageID);
                }
                CheckGONoDamageImmuneId(&got, got.goober.noDamageImmune, 11);
                if (got.goober.linkedTrap)            // linked trap
                    CheckGOLinkedTrapId(&got, got.goober.linkedTrap, 12);
                break;
            }
            case GAMEOBJECT_TYPE_AREADAMAGE:                //12
            {
                if (got.areadamage.open)
                    CheckGOLockId(&got, got.areadamage.open, 0);
                break;
            }
            case GAMEOBJECT_TYPE_CAMERA:                    //13
            {
                if (got.camera.open)
                    CheckGOLockId(&got, got.camera.open, 0);
                break;
            }
            case GAMEOBJECT_TYPE_MO_TRANSPORT:              //15
            {
                if (got.moTransport.taxiPathID)
                {
                    if (got.moTransport.taxiPathID >= sTaxiPathNodesByPath.size() || sTaxiPathNodesByPath[got.moTransport.taxiPathID].empty())
                        TC_LOG_ERROR("sql.sql", "GameObject (Entry: %u GoType: %u) have data0=%u but TaxiPath (Id: %u) not exist.",
                            entry, got.type, got.moTransport.taxiPathID, got.moTransport.taxiPathID);
                }
                if (uint32 transportMap = got.moTransport.SpawnMap)
                    _transportMaps.insert(transportMap);
                break;
            }
            case GAMEOBJECT_TYPE_SUMMONING_RITUAL:          //18
                break;
            case GAMEOBJECT_TYPE_SPELLCASTER:               //22
            {
                // always must have spell
                CheckGOSpellId(&got, got.spellcaster.spell, 0);
                break;
            }
            case GAMEOBJECT_TYPE_FLAGSTAND:                 //24
            {
                if (got.flagstand.open)
                    CheckGOLockId(&got, got.flagstand.open, 0);
                CheckGONoDamageImmuneId(&got, got.flagstand.noDamageImmune, 5);
                break;
            }
            case GAMEOBJECT_TYPE_FISHINGHOLE:               //25
            {
                if (got.fishinghole.open)
                    CheckGOLockId(&got, got.fishinghole.open, 4);
                break;
            }
            case GAMEOBJECT_TYPE_FLAGDROP:                  //26
            {
                if (got.flagdrop.open)
                    CheckGOLockId(&got, got.flagdrop.open, 0);
                CheckGONoDamageImmuneId(&got, got.flagdrop.noDamageImmune, 3);
                break;
            }
            case GAMEOBJECT_TYPE_BARBER_CHAIR:              //32
                CheckAndFixGOChairHeightId(&got, got.barberChair.chairheight, 0);
                break;
        }

       ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u game object templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadGameObjectTemplateAddons()
{
    uint32 oldMSTime = getMSTime();

    //                                                0       1       2      3        4       5        6        7        8        9
    QueryResult result = WorldDatabase.Query("SELECT entry, faction, flags, mingold, maxgold, artkit0, artkit1, artkit2, artkit3, artkit4 FROM gameobject_template_addon");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 gameobject template addon definitions. DB table `gameobject_template_addon` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        GameObjectTemplate const* got = sObjectMgr->GetGameObjectTemplate(entry);
        if (!got)
        {
            TC_LOG_ERROR("sql.sql", "GameObject template (Entry: %u) does not exist but has a record in `gameobject_template_addon`", entry);
            continue;
        }

        GameObjectTemplateAddon& gameObjectAddon = _gameObjectTemplateAddonStore[entry];
        gameObjectAddon.faction = uint32(fields[1].GetUInt16());
        gameObjectAddon.flags   = fields[2].GetUInt32();
        gameObjectAddon.mingold = fields[3].GetUInt32();
        gameObjectAddon.maxgold = fields[4].GetUInt32();

        for (uint32 i = 0; i < gameObjectAddon.artKits.size(); ++i)
            gameObjectAddon.artKits[i] = fields[5 + i].GetUInt32();

        // checks
        if (gameObjectAddon.faction && !sFactionTemplateStore.LookupEntry(gameObjectAddon.faction))
            TC_LOG_ERROR("sql.sql", "GameObject (Entry: %u) has invalid faction (%u) defined in `gameobject_template_addon`.", entry, gameObjectAddon.faction);

        if (gameObjectAddon.maxgold > 0)
        {
            switch (got->type)
            {
                case GAMEOBJECT_TYPE_CHEST:
                case GAMEOBJECT_TYPE_FISHINGHOLE:
                    break;
                default:
                    TC_LOG_ERROR("sql.sql", "GameObject (Entry %u GoType: %u) cannot be looted but has maxgold set in `gameobject_template_addon`.", entry, got->type);
                    break;
            }
        }

        uint32 artKitIndex = 0;
        for (uint32& artKitID : gameObjectAddon.artKits)
        {
            ++artKitIndex;
            if (!artKitID)
                continue;

            if (!sGameObjectArtKitStore.LookupEntry(artKitID))
            {
                TC_LOG_ERROR("sql.sql", "GameObject (Entry: %u) has invalid `artkit%d` (%d) defined, set to zero instead.", entry, artKitIndex - 1, artKitID);
                artKitID = 0;
            }
        }

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u game object template addons in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadExplorationBaseXP()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT level, basexp FROM exploration_basexp");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 BaseXP definitions. DB table `exploration_basexp` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint8 level  = fields[0].GetUInt8();
        uint32 basexp = fields[1].GetInt32();
        _baseXPTable[level] = basexp;
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u BaseXP definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

uint32 ObjectMgr::GetBaseXP(uint8 level)
{
    return _baseXPTable[level] ? _baseXPTable[level] : 0;
}

uint32 ObjectMgr::GetXPForLevel(uint8 level) const
{
    if (level < _playerXPperLevel.size())
        return _playerXPperLevel[level];
    return 0;
}

void ObjectMgr::LoadPetNames()
{
    uint32 oldMSTime = getMSTime();
    //                                                0     1      2
    QueryResult result = WorldDatabase.Query("SELECT word, entry, half FROM pet_name_generation");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 pet name parts. DB table `pet_name_generation` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        std::string word = fields[0].GetString();
        uint32 entry     = fields[1].GetUInt32();
        bool   half      = fields[2].GetBool();
        if (half)
            _petHalfName1[entry].push_back(word);
        else
            _petHalfName0[entry].push_back(word);
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u pet name parts in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadPetNumber()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = CharacterDatabase.Query("SELECT MAX(id) FROM character_pet");
    if (result)
    {
        Field* fields = result->Fetch();
        _hiPetNumber = fields[0].GetUInt32()+1;
    }

    TC_LOG_INFO("server.loading", ">> Loaded the max pet number: %d in %u ms", _hiPetNumber-1, GetMSTimeDiffToNow(oldMSTime));
}

std::string ObjectMgr::GeneratePetName(uint32 entry) const
{
    auto it0 = _petHalfName0.find(entry);
    auto it1 = _petHalfName1.find(entry);

    if (it0 == _petHalfName0.end() || it0->second.empty() || it1 == _petHalfName1.end() || it1->second.empty())
    {
        CreatureTemplate const* cinfo = GetCreatureTemplate(entry);
        if (!cinfo)
            return std::string();

        char const* petname = DBCManager::GetPetName(cinfo->family, sWorld->GetDefaultDbcLocale());
        if (petname)
            return std::string(petname);
        else
            return cinfo->Name;
    }

    return Trinity::Containers::SelectRandomContainerElement(it0->second) + Trinity::Containers::SelectRandomContainerElement(it1->second);
}

void ObjectMgr::LoadReputationRewardRate()
{
    uint32 oldMSTime = getMSTime();

    _repRewardRateStore.clear();                             // for reload case

    uint32 count = 0; //                                0          1             2                  3                  4                 5                      6             7
    QueryResult result = WorldDatabase.Query("SELECT faction, quest_rate, quest_daily_rate, quest_weekly_rate, quest_monthly_rate, quest_repeatable_rate, creature_rate, spell_rate FROM reputation_reward_rate");
    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded `reputation_reward_rate`, table is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 factionId            = fields[0].GetUInt32();

        RepRewardRate repRate;

        repRate.questRate           = fields[1].GetFloat();
        repRate.questDailyRate      = fields[2].GetFloat();
        repRate.questWeeklyRate     = fields[3].GetFloat();
        repRate.questMonthlyRate    = fields[4].GetFloat();
        repRate.questRepeatableRate = fields[5].GetFloat();
        repRate.creatureRate        = fields[6].GetFloat();
        repRate.spellRate           = fields[7].GetFloat();

        FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionId);
        if (!factionEntry)
        {
            TC_LOG_ERROR("sql.sql", "Faction (faction.dbc) %u does not exist but is used in `reputation_reward_rate`", factionId);
            continue;
        }

        if (repRate.questRate < 0.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table reputation_reward_rate has quest_rate with invalid rate %f, skipping data for faction %u", repRate.questRate, factionId);
            continue;
        }

        if (repRate.questDailyRate < 0.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table reputation_reward_rate has quest_daily_rate with invalid rate %f, skipping data for faction %u", repRate.questDailyRate, factionId);
            continue;
        }

        if (repRate.questWeeklyRate < 0.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table reputation_reward_rate has quest_weekly_rate with invalid rate %f, skipping data for faction %u", repRate.questWeeklyRate, factionId);
            continue;
        }

        if (repRate.questMonthlyRate < 0.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table reputation_reward_rate has quest_monthly_rate with invalid rate %f, skipping data for faction %u", repRate.questMonthlyRate, factionId);
            continue;
        }

        if (repRate.questRepeatableRate < 0.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table reputation_reward_rate has quest_repeatable_rate with invalid rate %f, skipping data for faction %u", repRate.questRepeatableRate, factionId);
            continue;
        }

        if (repRate.creatureRate < 0.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table reputation_reward_rate has creature_rate with invalid rate %f, skipping data for faction %u", repRate.creatureRate, factionId);
            continue;
        }

        if (repRate.spellRate < 0.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table reputation_reward_rate has spell_rate with invalid rate %f, skipping data for faction %u", repRate.spellRate, factionId);
            continue;
        }

        _repRewardRateStore[factionId] = repRate;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u reputation_reward_rate in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadRewardOnKill()
{
    uint32 oldMSTime = getMSTime();

    // For reload case
    _rewOnKillStore.clear();

    uint32 count = 0;

    //                                                0            1                     2
    QueryResult result = WorldDatabase.Query("SELECT creature_id, RewOnKillRepFaction1, RewOnKillRepFaction2, "
    //   3             4             5                   6             7             8                   9
        "IsTeamAward1, MaxStanding1, RewOnKillRepValue1, IsTeamAward2, MaxStanding2, RewOnKillRepValue2, TeamDependent, "
    //   10            11           12           13              14              15
        "CurrencyId1, CurrencyId2, CurrencyId3, CurrencyCount1, CurrencyCount2, CurrencyCount3 "
        "FROM creature_onkill_reward");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 creature reward definitions. DB table `creature_onkill_reputation` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 creature_id = fields[0].GetUInt32();

        RewardOnKillEntry rewOnKill;
        rewOnKill.RepFaction1          = fields[1].GetUInt32();
        rewOnKill.RepFaction2          = fields[2].GetUInt32();
        rewOnKill.IsTeamAward1         = fields[3].GetBool();
        rewOnKill.ReputationMaxCap1    = fields[4].GetUInt32();
        rewOnKill.RepValue1            = fields[5].GetInt32();
        rewOnKill.IsTeamAward2         = fields[6].GetBool();
        rewOnKill.ReputationMaxCap2    = fields[7].GetUInt32();
        rewOnKill.RepValue2            = fields[8].GetInt32();
        rewOnKill.TeamDependent        = fields[9].GetUInt8();
        rewOnKill.CurrencyId1          = fields[10].GetUInt32();
        rewOnKill.CurrencyId2          = fields[11].GetUInt32();
        rewOnKill.CurrencyId3          = fields[12].GetUInt32();
        rewOnKill.CurrencyCount1       = fields[13].GetInt32();
        rewOnKill.CurrencyCount2       = fields[14].GetInt32();
        rewOnKill.CurrencyCount3       = fields[15].GetInt32();

        if (!GetCreatureTemplate(creature_id))
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_onkill_reward` has data for nonexistent creature entry (%u), skipped", creature_id);
            continue;
        }

        if (rewOnKill.RepFaction1)
        {
            FactionEntry const* factionEntry1 = sFactionStore.LookupEntry(rewOnKill.RepFaction1);
            if (!factionEntry1)
            {
                TC_LOG_ERROR("sql.sql", "Faction (faction.dbc) %u does not exist but is used in `creature_onkill_reputation`", rewOnKill.RepFaction1);
                continue;
            }
        }

        if (rewOnKill.RepFaction2)
        {
            FactionEntry const* factionEntry2 = sFactionStore.LookupEntry(rewOnKill.RepFaction2);
            if (!factionEntry2)
            {
                TC_LOG_ERROR("sql.sql", "Faction (faction.dbc) %u does not exist but is used in `creature_onkill_reputation`", rewOnKill.RepFaction2);
                continue;
            }
        }

        if (rewOnKill.CurrencyId1)
        {
            CurrencyTypesEntry const* currencyId1 = sCurrencyTypesStore.LookupEntry(rewOnKill.CurrencyId1);
            if (!currencyId1)
            {
                TC_LOG_ERROR("sql.sql", "CurrencyType (CurrenctTypes.dbc) %u does not exist but is used in `creature_onkill_reward`", rewOnKill.CurrencyId1);
                continue;
            }
        }

        if (rewOnKill.CurrencyId2)
        {
            CurrencyTypesEntry const* currencyId2 = sCurrencyTypesStore.LookupEntry(rewOnKill.CurrencyId2);
            if (!currencyId2)
            {
                TC_LOG_ERROR("sql.sql", "CurrencyType (CurrenctTypes.dbc) %u does not exist but is used in `creature_onkill_reward`", rewOnKill.CurrencyId2);
                continue;
            }
        }

        if (rewOnKill.CurrencyId3)
        {
            CurrencyTypesEntry const* currencyId3 = sCurrencyTypesStore.LookupEntry(rewOnKill.CurrencyId3);
            if (!currencyId3)
            {
                TC_LOG_ERROR("sql.sql", "CurrencyType (CurrenctTypes.dbc) %u does not exist but is used in `creature_onkill_reward`", rewOnKill.CurrencyId3);
                continue;
            }
        }

        _rewOnKillStore[creature_id] = rewOnKill;

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature award reputation definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadReputationSpilloverTemplate()
{
    uint32 oldMSTime = getMSTime();

    _repSpilloverTemplateStore.clear();                      // for reload case

    uint32 count = 0; //                                0         1        2       3        4       5       6         7        8      9        10       11     12        13       14     15
    QueryResult result = WorldDatabase.Query("SELECT faction, faction1, rate_1, rank_1, faction2, rate_2, rank_2, faction3, rate_3, rank_3, faction4, rate_4, rank_4, faction5, rate_5, rank_5 FROM reputation_spillover_template");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded `reputation_spillover_template`, table is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 factionId                = fields[0].GetUInt16();

        RepSpilloverTemplate repTemplate;

        repTemplate.faction[0]          = fields[1].GetUInt16();
        repTemplate.faction_rate[0]     = fields[2].GetFloat();
        repTemplate.faction_rank[0]     = fields[3].GetUInt8();
        repTemplate.faction[1]          = fields[4].GetUInt16();
        repTemplate.faction_rate[1]     = fields[5].GetFloat();
        repTemplate.faction_rank[1]     = fields[6].GetUInt8();
        repTemplate.faction[2]          = fields[7].GetUInt16();
        repTemplate.faction_rate[2]     = fields[8].GetFloat();
        repTemplate.faction_rank[2]     = fields[9].GetUInt8();
        repTemplate.faction[3]          = fields[10].GetUInt16();
        repTemplate.faction_rate[3]     = fields[11].GetFloat();
        repTemplate.faction_rank[3]     = fields[12].GetUInt8();
        repTemplate.faction[4]          = fields[13].GetUInt16();
        repTemplate.faction_rate[4]     = fields[14].GetFloat();
        repTemplate.faction_rank[4]     = fields[15].GetUInt8();

        FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionId);

        if (!factionEntry)
        {
            TC_LOG_ERROR("sql.sql", "Faction (faction.dbc) %u does not exist but is used in `reputation_spillover_template`", factionId);
            continue;
        }

        if (factionEntry->ParentFactionID == 0)
        {
            TC_LOG_ERROR("sql.sql", "Faction (faction.dbc) %u in `reputation_spillover_template` does not belong to any team, skipping", factionId);
            continue;
        }

        bool invalidSpilloverFaction = false;
        for (uint32 i = 0; i < MAX_SPILLOVER_FACTIONS; ++i)
        {
            if (repTemplate.faction[i])
            {
                FactionEntry const* factionSpillover = sFactionStore.LookupEntry(repTemplate.faction[i]);

                if (!factionSpillover)
                {
                    TC_LOG_ERROR("sql.sql", "Spillover faction (faction.dbc) %u does not exist but is used in `reputation_spillover_template` for faction %u, skipping", repTemplate.faction[i], factionId);
                    invalidSpilloverFaction = true;
                    break;
                }

                if (factionSpillover->ReputationIndex < 0)
                {
                    TC_LOG_ERROR("sql.sql", "Spillover faction (faction.dbc) %u for faction %u in `reputation_spillover_template` can not be listed for client, and then useless, skipping", repTemplate.faction[i], factionId);
                    invalidSpilloverFaction = true;
                    break;
                }

                if (repTemplate.faction_rank[i] >= MAX_REPUTATION_RANK)
                {
                    TC_LOG_ERROR("sql.sql", "Rank %u used in `reputation_spillover_template` for spillover faction %u is not valid, skipping", repTemplate.faction_rank[i], repTemplate.faction[i]);
                    invalidSpilloverFaction = true;
                    break;
                }
            }
        }

        if (invalidSpilloverFaction)
            continue;
        FactionEntry const* factionEntry4 = sFactionStore.LookupEntry(repTemplate.faction[4]);
        if (repTemplate.faction[4] && !factionEntry4)
        {
            TC_LOG_ERROR("sql.sql", "Faction (faction.dbc) %u does not exist but is used in `reputation_spillover_template`", repTemplate.faction[4]);
            continue;
        }

        _repSpilloverTemplateStore[factionId] = repTemplate;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u reputation_spillover_template in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadPointsOfInterest()
{
    uint32 oldMSTime = getMSTime();

    _pointsOfInterestStore.clear();                              // need for reload case

    uint32 count = 0;

    //                                               0   1          2          3     4      5           6
    QueryResult result = WorldDatabase.Query("SELECT ID, PositionX, PositionY, Icon, Flags, Importance, Name FROM points_of_interest");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 Points of Interest definitions. DB table `points_of_interest` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].GetUInt32();

        PointOfInterest pointOfInterest;
        pointOfInterest.ID         = id;
        pointOfInterest.PositionX  = fields[1].GetFloat();
        pointOfInterest.PositionY  = fields[2].GetFloat();
        pointOfInterest.Icon       = fields[3].GetUInt32();
        pointOfInterest.Flags      = fields[4].GetUInt32();
        pointOfInterest.Importance = fields[5].GetUInt32();
        pointOfInterest.Name       = fields[6].GetString();

        if (!Trinity::IsValidMapCoord(pointOfInterest.PositionX, pointOfInterest.PositionY))
        {
            TC_LOG_ERROR("sql.sql", "Table `points_of_interest` (ID: %u) have invalid coordinates (PositionX: %f PositionY: %f), ignored.", id, pointOfInterest.PositionX, pointOfInterest.PositionY);
            continue;
        }

        _pointsOfInterestStore[id] = pointOfInterest;

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u Points of Interest definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadQuestPOI()
{
    uint32 oldMSTime = getMSTime();

    _questPOIStore.clear();                              // need for reload case

    uint32 count = 0;

    //                                               0        1          2          3           4          5       6        7
    QueryResult result = WorldDatabase.Query("SELECT QuestID, id, ObjectiveIndex, MapID, WorldMapAreaId, Floor, Priority, Flags FROM quest_poi order by QuestID");
    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 quest POI definitions. DB table `quest_poi` is empty.");
        return;
    }

    //                                                  0       1   2  3
    QueryResult points = WorldDatabase.Query("SELECT QuestID, Idx1, X, Y FROM quest_poi_points ORDER BY QuestID DESC, Idx2");

    std::vector<std::vector<std::vector<QuestPOIBlobPoint>>> POIs;

    if (points)
    {
        // The first result should have the highest questId
        Field* fields = points->Fetch();
        uint32 questIdMax = fields[0].GetUInt32();
        POIs.resize(questIdMax + 1);

        do
        {
            fields = points->Fetch();

            uint32 questId            = fields[0].GetUInt32();
            uint32 id                 = fields[1].GetUInt32();
            int32  x                  = fields[2].GetInt32();
            int32  y                  = fields[3].GetInt32();

            if (POIs[questId].size() <= id + 1)
                POIs[questId].resize(id + 10);

            QuestPOIBlobPoint point;
            point.X = x;
            point.Y = y;

            POIs[questId][id].push_back(point);
        } while (points->NextRow());
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 questId            = fields[0].GetUInt32();
        uint32 id                 = fields[1].GetUInt32();
        int32 objIndex            = fields[2].GetInt32();
        uint32 mapId              = fields[3].GetUInt32();
        uint32 WorldMapAreaId     = fields[4].GetUInt32();
        uint32 FloorId            = fields[5].GetUInt32();
        uint32 Priority           = fields[6].GetUInt32();
        uint32 Flags              = fields[7].GetUInt32();

        QuestPOIBlobData POI;
        POI.BlobIndex = id;
        POI.ObjectiveIndex = objIndex;
        POI.MapID = mapId;
        POI.WorldMapAreaID = WorldMapAreaId;
        POI.Floor = FloorId;
        POI.Priority = Priority;
        POI.Flags = Flags;

        if (questId < POIs.size() && id < POIs[questId].size())
        {
            POI.QuestPOIBlobPointStats = POIs[questId][id];
            auto itr = _questPOIStore.find(questId);
            if (itr == _questPOIStore.end())
            {
                QuestPOIWrapper wrapper;
                QuestPOIData data;
                data.QuestID = questId;
                wrapper.POIData = data;

                _questPOIStore.emplace(questId, std::move(wrapper));
            }

            _questPOIStore.at(questId).POIData.QuestPOIBlobDataStats.push_back(POI);
        }
        else
            TC_LOG_ERROR("sql.sql", "Table quest_poi references unknown quest points for quest %u POI id %u", questId, id);

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u quest POI definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadNPCSpellClickSpells()
{
    uint32 oldMSTime = getMSTime();

    _spellClickInfoStore.clear();
    //                                                0          1         2            3
    QueryResult result = WorldDatabase.Query("SELECT npc_entry, spell_id, cast_flags, user_type FROM npc_spellclick_spells");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 spellclick spells. DB table `npc_spellclick_spells` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 npc_entry = fields[0].GetUInt32();
        CreatureTemplate const* cInfo = GetCreatureTemplate(npc_entry);
        if (!cInfo)
        {
            TC_LOG_ERROR("sql.sql", "Table npc_spellclick_spells references unknown creature_template %u. Skipping entry.", npc_entry);
            continue;
        }

        uint32 spellid = fields[1].GetUInt32();
        SpellInfo const* spellinfo = sSpellMgr->GetSpellInfo(spellid);
        if (!spellinfo)
        {
            TC_LOG_ERROR("sql.sql", "Table npc_spellclick_spells creature: %u references unknown spellid %u. Skipping entry.", npc_entry, spellid);
            continue;
        }

        uint8 userType = fields[3].GetUInt16();
        if (userType >= SPELL_CLICK_USER_MAX)
            TC_LOG_ERROR("sql.sql", "Table npc_spellclick_spells creature: %u  references unknown user type %u. Skipping entry.", npc_entry, uint32(userType));

        uint8 castFlags = fields[2].GetUInt8();
        SpellClickInfo info;
        info.spellId = spellid;
        info.castFlags = castFlags;
        info.userType = SpellClickUserTypes(userType);
        _spellClickInfoStore.insert(SpellClickInfoContainer::value_type(npc_entry, info));

        ++count;
    }
    while (result->NextRow());

    // all spellclick data loaded, now we check if there are creatures with NPC_FLAG_SPELLCLICK but with no data
    // NOTE: It *CAN* be the other way around: no spellclick flag but with spellclick data, in case of creature-only vehicle accessories
    CreatureTemplateContainer const* ctc = sObjectMgr->GetCreatureTemplates();
    for (CreatureTemplateContainer::const_iterator itr = ctc->begin(); itr != ctc->end(); ++itr)
    {
        if ((itr->second.npcflag & UNIT_NPC_FLAG_SPELLCLICK) && _spellClickInfoStore.find(itr->second.Entry) == _spellClickInfoStore.end())
        {
            TC_LOG_ERROR("sql.sql", "npc_spellclick_spells: Creature template %u has UNIT_NPC_FLAG_SPELLCLICK but no data in spellclick table! Removing flag", itr->second.Entry);
            const_cast<CreatureTemplate*>(&itr->second)->npcflag &= ~UNIT_NPC_FLAG_SPELLCLICK;
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u spellclick definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::DeleteCreatureData(ObjectGuid::LowType guid)
{
    // remove mapid*cellid -> guid_set map
    CreatureData const* data = GetCreatureData(guid);
    if (data)
    {
        RemoveCreatureFromGrid(guid, data);
        OnDeleteSpawnData(data);
    }

    _creatureDataStore.erase(guid);
}

void ObjectMgr::DeleteGameObjectData(ObjectGuid::LowType guid)
{
    // remove mapid*cellid -> guid_set map
    GameObjectData const* data = GetGameObjectData(guid);
    if (data)
    {
        RemoveGameobjectFromGrid(guid, data);
        OnDeleteSpawnData(data);
    }

    _gameObjectDataStore.erase(guid);
}

void ObjectMgr::LoadQuestRelationsHelper(QuestRelations& map, QuestRelationsReverse* reversedMap, std::string const& table)
{
    uint32 oldMSTime = getMSTime();

    map.clear();                                            // need for reload case

    uint32 count = 0;

    QueryResult result = WorldDatabase.PQuery("SELECT id, quest FROM %s", table.c_str());

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 quest relations from `%s`, table is empty.", table.c_str());
        return;
    }

    do
    {
        uint32 id     = result->Fetch()[0].GetUInt32();
        uint32 quest  = result->Fetch()[1].GetUInt32();

        if (_questTemplates.find(quest) == _questTemplates.end())
        {
            TC_LOG_ERROR("sql.sql", "Table `%s`: Quest %u listed for entry %u does not exist.", table.c_str(), quest, id);
            continue;
        }

        map.insert(QuestRelations::value_type(id, quest));
        if (reversedMap)
            reversedMap->insert(QuestRelationsReverse::value_type(quest, id));

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u quest relations from %s in %u ms", count, table.c_str(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadGameobjectQuestStarters()
{
    LoadQuestRelationsHelper(_goQuestRelations, nullptr, "gameobject_queststarter");

    for (QuestRelations::iterator itr = _goQuestRelations.begin(); itr != _goQuestRelations.end(); ++itr)
    {
        GameObjectTemplate const* goInfo = GetGameObjectTemplate(itr->first);
        if (!goInfo)
            TC_LOG_ERROR("sql.sql", "Table `gameobject_queststarter` has data for nonexistent gameobject entry (%u) and existed quest %u", itr->first, itr->second);
        else if (goInfo->type != GAMEOBJECT_TYPE_QUESTGIVER)
            TC_LOG_ERROR("sql.sql", "Table `gameobject_queststarter` has data gameobject entry (%u) for quest %u, but GO is not GAMEOBJECT_TYPE_QUESTGIVER", itr->first, itr->second);
    }
}

void ObjectMgr::LoadGameobjectQuestEnders()
{
    LoadQuestRelationsHelper(_goQuestInvolvedRelations, &_goQuestInvolvedRelationsReverse, "gameobject_questender");

    for (QuestRelations::iterator itr = _goQuestInvolvedRelations.begin(); itr != _goQuestInvolvedRelations.end(); ++itr)
    {
        GameObjectTemplate const* goInfo = GetGameObjectTemplate(itr->first);
        if (!goInfo)
            TC_LOG_ERROR("sql.sql", "Table `gameobject_questender` has data for nonexistent gameobject entry (%u) and existed quest %u", itr->first, itr->second);
        else if (goInfo->type != GAMEOBJECT_TYPE_QUESTGIVER)
            TC_LOG_ERROR("sql.sql", "Table `gameobject_questender` has data gameobject entry (%u) for quest %u, but GO is not GAMEOBJECT_TYPE_QUESTGIVER", itr->first, itr->second);
    }
}

void ObjectMgr::LoadCreatureQuestStarters()
{
    LoadQuestRelationsHelper(_creatureQuestRelations, nullptr, "creature_queststarter");

    for (QuestRelations::iterator itr = _creatureQuestRelations.begin(); itr != _creatureQuestRelations.end(); ++itr)
    {
        CreatureTemplate const* cInfo = GetCreatureTemplate(itr->first);
        if (!cInfo)
            TC_LOG_ERROR("sql.sql", "Table `creature_queststarter` has data for nonexistent creature entry (%u) and existed quest %u", itr->first, itr->second);
        else if (!(cInfo->npcflag & UNIT_NPC_FLAG_QUESTGIVER))
            TC_LOG_ERROR("sql.sql", "Table `creature_queststarter` has creature entry (%u) for quest %u, but npcflag does not include UNIT_NPC_FLAG_QUESTGIVER", itr->first, itr->second);
    }
}

void ObjectMgr::LoadCreatureQuestEnders()
{
    LoadQuestRelationsHelper(_creatureQuestInvolvedRelations, &_creatureQuestInvolvedRelationsReverse, "creature_questender");

    for (QuestRelations::iterator itr = _creatureQuestInvolvedRelations.begin(); itr != _creatureQuestInvolvedRelations.end(); ++itr)
    {
        CreatureTemplate const* cInfo = GetCreatureTemplate(itr->first);
        if (!cInfo)
            TC_LOG_ERROR("sql.sql", "Table `creature_questender` has data for nonexistent creature entry (%u) and existed quest %u", itr->first, itr->second);
        else if (!(cInfo->npcflag & UNIT_NPC_FLAG_QUESTGIVER))
            TC_LOG_ERROR("sql.sql", "Table `creature_questender` has creature entry (%u) for quest %u, but npcflag does not include UNIT_NPC_FLAG_QUESTGIVER", itr->first, itr->second);
    }
}

void QuestRelationResult::Iterator::_skip()
{
    while ((_it != _end) && !Quest::IsTakingQuestEnabled(_it->second))
        ++_it;
}

bool QuestRelationResult::HasQuest(uint32 questId) const
{
    return (std::find_if(_begin, _end, [questId](QuestRelations::value_type const& pair) { return (pair.second == questId); }) != _end) && (!_onlyActive || Quest::IsTakingQuestEnabled(questId));
}

void ObjectMgr::LoadReservedPlayersNames()
{
    uint32 oldMSTime = getMSTime();

    _reservedNamesStore.clear();                                // need for reload case

    QueryResult result = CharacterDatabase.Query("SELECT name FROM reserved_name");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 reserved player names. DB table `reserved_name` is empty!");
        return;
    }

    uint32 count = 0;

    Field* fields;
    do
    {
        fields = result->Fetch();
        std::string name= fields[0].GetString();

        std::wstring wstr;
        if (!Utf8toWStr (name, wstr))
        {
            TC_LOG_ERROR("misc", "Table `reserved_name` has invalid name: %s", name.c_str());
            continue;
        }

        wstrToLower(wstr);

        _reservedNamesStore.insert(wstr);
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u reserved player names in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

bool ObjectMgr::IsReservedName(const std::string& name) const
{
    std::wstring wstr;
    if (!Utf8toWStr (name, wstr))
        return false;

    wstrToLower(wstr);

    return _reservedNamesStore.find(wstr) != _reservedNamesStore.end();
}

enum LanguageType
{
    LT_BASIC_LATIN    = 0x0000,
    LT_EXTENDEN_LATIN = 0x0001,
    LT_CYRILLIC       = 0x0002,
    LT_EAST_ASIA      = 0x0004,
    LT_ANY            = 0xFFFF
};

static LanguageType GetRealmLanguageType(bool create)
{
    switch (sWorld->getIntConfig(CONFIG_REALM_ZONE))
    {
        case REALM_ZONE_UNKNOWN:                            // any language
        case REALM_ZONE_DEVELOPMENT:
        case REALM_ZONE_TEST_SERVER:
        case REALM_ZONE_QA_SERVER:
            return LT_ANY;
        case REALM_ZONE_UNITED_STATES:                      // extended-Latin
        case REALM_ZONE_OCEANIC:
        case REALM_ZONE_LATIN_AMERICA:
        case REALM_ZONE_ENGLISH:
        case REALM_ZONE_GERMAN:
        case REALM_ZONE_FRENCH:
        case REALM_ZONE_SPANISH:
            return LT_EXTENDEN_LATIN;
        case REALM_ZONE_KOREA:                              // East-Asian
        case REALM_ZONE_TAIWAN:
        case REALM_ZONE_CHINA:
            return LT_EAST_ASIA;
        case REALM_ZONE_RUSSIAN:                            // Cyrillic
            return LT_CYRILLIC;
        default:
            return create ? LT_BASIC_LATIN : LT_ANY;        // basic-Latin at create, any at login
    }
}

bool isValidString(const std::wstring& wstr, uint32 strictMask, bool numericOrSpace, bool create = false)
{
    if (strictMask == 0)                                       // any language, ignore realm
    {
        if (isExtendedLatinString(wstr, numericOrSpace))
            return true;
        if (isCyrillicString(wstr, numericOrSpace))
            return true;
        if (isEastAsianString(wstr, numericOrSpace))
            return true;
        return false;
    }

    if (strictMask & 0x2)                                    // realm zone specific
    {
        LanguageType lt = GetRealmLanguageType(create);
        if (lt & LT_EXTENDEN_LATIN)
            if (isExtendedLatinString(wstr, numericOrSpace))
                return true;
        if (lt & LT_CYRILLIC)
            if (isCyrillicString(wstr, numericOrSpace))
                return true;
        if (lt & LT_EAST_ASIA)
            if (isEastAsianString(wstr, numericOrSpace))
                return true;
    }

    if (strictMask & 0x1)                                    // basic Latin
    {
        if (isBasicLatinString(wstr, numericOrSpace))
            return true;
    }

    return false;
}

ResponseCodes ObjectMgr::CheckPlayerName(std::string const& name, LocaleConstant locale, bool create /*= false*/)
{
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return CHAR_NAME_INVALID_CHARACTER;

    if (wname.size() > MAX_PLAYER_NAME)
        return CHAR_NAME_TOO_LONG;

    uint32 minName = sWorld->getIntConfig(CONFIG_MIN_PLAYER_NAME);
    if (wname.size() < minName)
        return CHAR_NAME_TOO_SHORT;

    uint32 strictMask = sWorld->getIntConfig(CONFIG_STRICT_PLAYER_NAMES);
    if (!isValidString(wname, strictMask, false, create))
        return CHAR_NAME_MIXED_LANGUAGES;

    wstrToLower(wname);
    for (size_t i = 2; i < wname.size(); ++i)
        if (wname[i] == wname[i-1] && wname[i] == wname[i-2])
            return CHAR_NAME_THREE_CONSECUTIVE;

    return sDBCManager.ValidateName(wname, locale);
}

bool ObjectMgr::IsValidCharterName(const std::string& name)
{
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return false;

    if (wname.size() > MAX_CHARTER_NAME)
        return false;

    uint32 minName = sWorld->getIntConfig(CONFIG_MIN_CHARTER_NAME);
    if (wname.size() < minName)
        return false;

    uint32 strictMask = sWorld->getIntConfig(CONFIG_STRICT_CHARTER_NAMES);

    return isValidString(wname, strictMask, true);
}

PetNameInvalidReason ObjectMgr::CheckPetName(const std::string& name, LocaleConstant locale)
{
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return PET_NAME_INVALID;

    if (wname.size() > MAX_PET_NAME)
        return PET_NAME_TOO_LONG;

    uint32 minName = sWorld->getIntConfig(CONFIG_MIN_PET_NAME);
    if (wname.size() < minName)
        return PET_NAME_TOO_SHORT;

    uint32 strictMask = sWorld->getIntConfig(CONFIG_STRICT_PET_NAMES);
    if (!isValidString(wname, strictMask, false))
        return PET_NAME_MIXED_LANGUAGES;

    switch (sDBCManager.ValidateName(wname, locale))
    {
        case CHAR_NAME_PROFANE:
            return PET_NAME_PROFANE;
        case CHAR_NAME_RESERVED:
            return PET_NAME_RESERVED;
        case RESPONSE_FAILURE:
            return PET_NAME_INVALID;
        default:
            break;
    }
    return PET_NAME_SUCCESS;
}

void ObjectMgr::LoadGameObjectForQuests()
{
    uint32 oldMSTime = getMSTime();

    _gameObjectForQuestStore.clear();                         // need for reload case

    if (sObjectMgr->GetGameObjectTemplates()->empty())
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 GameObjects for quests");
        return;
    }

    uint32 count = 0;

    // collect GO entries for GO that must activated
    GameObjectTemplateContainer const* gotc = sObjectMgr->GetGameObjectTemplates();
    for (GameObjectTemplateContainer::const_iterator itr = gotc->begin(); itr != gotc->end(); ++itr)
    {
        switch (itr->second.type)
        {
            case GAMEOBJECT_TYPE_QUESTGIVER:
                _gameObjectForQuestStore.insert(itr->second.entry);
                ++count;
                break;
            case GAMEOBJECT_TYPE_CHEST:
            {
                // scan GO chest with loot including quest items
                uint32 loot_id = (itr->second.GetLootId());

                // find quest loot for GO
                if (itr->second.chest.questID || LootTemplates_Gameobject.HaveQuestLootFor(loot_id))
                {
                    _gameObjectForQuestStore.insert(itr->second.entry);
                    ++count;
                }
                break;
            }
            case GAMEOBJECT_TYPE_GENERIC:
            {
                if (itr->second._generic.questID > 0)            //quests objects
                {
                    _gameObjectForQuestStore.insert(itr->second.entry);
                    ++count;
                }
                break;
            }
            case GAMEOBJECT_TYPE_GOOBER:
            {
                if (itr->second.goober.questID > 0)              //quests objects
                {
                    _gameObjectForQuestStore.insert(itr->second.entry);
                    ++count;
                }
                break;
            }
            default:
                break;
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u GameObjects for quests in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

bool ObjectMgr::LoadTrinityStrings()
{
    uint32 oldMSTime = getMSTime();

    _trinityStringStore.clear(); // for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry, content_default, content_loc1, content_loc2, content_loc3, content_loc4, content_loc5, content_loc6, content_loc7, content_loc8 FROM trinity_string");
    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 trinity strings. DB table `trinity_string` is empty. You have imported an incorrect database for more info search for TCE00003 on forum.");
        return false;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        TrinityString& data = _trinityStringStore[entry];

        data.Content.resize(DEFAULT_LOCALE + 1);

        for (int8 i = OLD_TOTAL_LOCALES - 1; i >= 0; --i)
            AddLocaleString(fields[i + 1].GetString(), LocaleConstant(i), data.Content);
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " trinity strings in %u ms", _trinityStringStore.size(), GetMSTimeDiffToNow(oldMSTime));
    return true;
}

char const* ObjectMgr::GetTrinityString(uint32 entry, LocaleConstant locale) const
{
    if (TrinityString const* ts = GetTrinityString(entry))
    {
        if (ts->Content.size() > size_t(locale) && !ts->Content[locale].empty())
            return ts->Content[locale].c_str();
        return ts->Content[DEFAULT_LOCALE].c_str();
    }

    TC_LOG_ERROR("sql.sql", "Trinity string entry %u not found in DB.", entry);
    return "<error>";
}

void ObjectMgr::LoadFishingBaseSkillLevel()
{
    uint32 oldMSTime = getMSTime();

    _fishingBaseForAreaStore.clear();                            // for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry, skill FROM skill_fishing_base_level");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 areas for fishing base skill level. DB table `skill_fishing_base_level` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint32 entry  = fields[0].GetUInt32();
        int32 skill   = fields[1].GetInt16();

        AreaTableEntry const* fArea = sAreaTableStore.LookupEntry(entry);
        if (!fArea)
        {
            TC_LOG_ERROR("sql.sql", "AreaId %u defined in `skill_fishing_base_level` does not exist", entry);
            continue;
        }

        _fishingBaseForAreaStore[entry] = skill;
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u areas for fishing base skill level in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

bool ObjectMgr::CheckDeclinedNames(const std::wstring& w_ownname, DeclinedName const& names)
{
    // get main part of the name
    std::wstring mainpart = GetMainPartOfName(w_ownname, 0);
    // prepare flags
    bool x = true;
    bool y = true;

    // check declined names
    for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        std::wstring wname;
        if (!Utf8toWStr(names.name[i], wname))
            return false;

        if (mainpart != GetMainPartOfName(wname, i+1))
            x = false;

        if (w_ownname != wname)
            y = false;
    }
    return (x || y);
}

uint32 ObjectMgr::GetAreaTriggerScriptId(uint32 trigger_id) const
{
    AreaTriggerScriptContainer::const_iterator i = _areaTriggerScriptStore.find(trigger_id);
    if (i!= _areaTriggerScriptStore.end())
        return i->second;
    return 0;
}

SpellScriptsBounds ObjectMgr::GetSpellScriptsBounds(uint32 spellId)
{
    return _spellScriptsStore.equal_range(spellId);
}

// this allows calculating base reputations to offline players, just by race and class
int32 ObjectMgr::GetBaseReputationOf(FactionEntry const* factionEntry, uint8 race, uint8 playerClass) const
{
    if (!factionEntry)
        return 0;

    uint32 raceMask = (1 << (race - 1));
    uint32 classMask = (1 << (playerClass-1));

    for (int i = 0; i < 4; i++)
    {
        if ((!factionEntry->ReputationClassMask[i] ||
            factionEntry->ReputationClassMask[i] & classMask) &&
            (!factionEntry->ReputationRaceMask[i] ||
            factionEntry->ReputationRaceMask[i] & raceMask))
            return factionEntry->ReputationBase[i];
    }

    return 0;
}

SkillRangeType GetSkillRangeType(SkillRaceClassInfoEntry const* rcEntry)
{
    SkillLineEntry const* skill = sSkillLineStore.LookupEntry(rcEntry->SkillID);
    if (!skill)
        return SKILL_RANGE_NONE;

    if (sSkillTiersStore.LookupEntry(rcEntry->SkillTierID))
        return SKILL_RANGE_RANK;

    if (rcEntry->SkillID == SKILL_RUNEFORGING)
        return SKILL_RANGE_MONO;

    switch (skill->CategoryID)
    {
        case SKILL_CATEGORY_ARMOR:
            return SKILL_RANGE_MONO;
        case SKILL_CATEGORY_LANGUAGES:
            return SKILL_RANGE_LANGUAGE;
    }

    return SKILL_RANGE_LEVEL;
}

void ObjectMgr::LoadGameTele()
{
    uint32 oldMSTime = getMSTime();

    _gameTeleStore.clear();                                  // for reload case

    //                                                0       1           2           3           4        5     6
    QueryResult result = WorldDatabase.Query("SELECT id, position_x, position_y, position_z, orientation, map, name FROM game_tele");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 GameTeleports. DB table `game_tele` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 id         = fields[0].GetUInt32();

        GameTele gt;

        gt.position_x     = fields[1].GetFloat();
        gt.position_y     = fields[2].GetFloat();
        gt.position_z     = fields[3].GetFloat();
        gt.orientation    = fields[4].GetFloat();
        gt.mapId          = fields[5].GetUInt16();
        gt.name           = fields[6].GetString();

        if (!MapManager::IsValidMapCoord(gt.mapId, gt.position_x, gt.position_y, gt.position_z, gt.orientation))
        {
            TC_LOG_ERROR("sql.sql", "Wrong position for id %u (name: %s) in `game_tele` table, ignoring.", id, gt.name.c_str());
            continue;
        }

        if (!Utf8toWStr(gt.name, gt.wnameLow))
        {
            TC_LOG_ERROR("sql.sql", "Wrong UTF8 name for id %u in `game_tele` table, ignoring.", id);
            continue;
        }

        wstrToLower(gt.wnameLow);

        _gameTeleStore[id] = gt;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u GameTeleports in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

GameTele const* ObjectMgr::GetGameTele(const std::string& name) const
{
    // explicit name case
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return nullptr;

    // converting string that we try to find to lower case
    wstrToLower(wname);

    // Alternative first GameTele what contains wnameLow as substring in case no GameTele location found
    GameTele const* alt = nullptr;
    for (GameTeleContainer::const_iterator itr = _gameTeleStore.begin(); itr != _gameTeleStore.end(); ++itr)
    {
        if (itr->second.wnameLow == wname)
            return &itr->second;
        else if (!alt && itr->second.wnameLow.find(wname) != std::wstring::npos)
            alt = &itr->second;
    }

    return alt;
}

GameTele const* ObjectMgr::GetGameTeleExactName(const std::string& name) const
{
    // explicit name case
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return nullptr;

    // converting string that we try to find to lower case
    wstrToLower(wname);

    for (GameTeleContainer::const_iterator itr = _gameTeleStore.begin(); itr != _gameTeleStore.end(); ++itr)
    {
        if (itr->second.wnameLow == wname)
            return &itr->second;
    }

    return nullptr;
}

bool ObjectMgr::AddGameTele(GameTele& tele)
{
    // find max id
    uint32 new_id = 0;
    for (GameTeleContainer::const_iterator itr = _gameTeleStore.begin(); itr != _gameTeleStore.end(); ++itr)
        if (itr->first > new_id)
            new_id = itr->first;

    // use next
    ++new_id;

    if (!Utf8toWStr(tele.name, tele.wnameLow))
        return false;

    wstrToLower(tele.wnameLow);

    _gameTeleStore[new_id] = tele;

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_GAME_TELE);

    stmt->setUInt32(0, new_id);
    stmt->setFloat(1, tele.position_x);
    stmt->setFloat(2, tele.position_y);
    stmt->setFloat(3, tele.position_z);
    stmt->setFloat(4, tele.orientation);
    stmt->setUInt16(5, uint16(tele.mapId));
    stmt->setString(6, tele.name);

    WorldDatabase.Execute(stmt);

    return true;
}

bool ObjectMgr::DeleteGameTele(const std::string& name)
{
    // explicit name case
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return false;

    // converting string that we try to find to lower case
    wstrToLower(wname);

    for (GameTeleContainer::iterator itr = _gameTeleStore.begin(); itr != _gameTeleStore.end(); ++itr)
    {
        if (itr->second.wnameLow == wname)
        {
            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_GAME_TELE);

            stmt->setString(0, itr->second.name);

            WorldDatabase.Execute(stmt);

            _gameTeleStore.erase(itr);
            return true;
        }
    }

    return false;
}

void ObjectMgr::LoadMailLevelRewards()
{
    uint32 oldMSTime = getMSTime();

    _mailLevelRewardStore.clear();                           // for reload case

    //                                                 0        1             2            3
    QueryResult result = WorldDatabase.Query("SELECT level, raceMask, mailTemplateId, senderEntry FROM mail_level_reward");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 level dependent mail rewards. DB table `mail_level_reward` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint8 level           = fields[0].GetUInt8();
        uint32 raceMask       = fields[1].GetUInt32();
        uint32 mailTemplateId = fields[2].GetUInt32();
        uint32 senderEntry    = fields[3].GetUInt32();

        if (level > MAX_LEVEL)
        {
            TC_LOG_ERROR("sql.sql", "Table `mail_level_reward` has data for level %u that more supported by client (%u), ignoring.", level, MAX_LEVEL);
            continue;
        }

        if (!(raceMask & RACEMASK_ALL_PLAYABLE))
        {
            TC_LOG_ERROR("sql.sql", "Table `mail_level_reward` has raceMask (%u) for level %u that not include any player races, ignoring.", raceMask, level);
            continue;
        }

        if (!sMailTemplateStore.LookupEntry(mailTemplateId))
        {
            TC_LOG_ERROR("sql.sql", "Table `mail_level_reward` has invalid mailTemplateId (%u) for level %u that invalid not include any player races, ignoring.", mailTemplateId, level);
            continue;
        }

        if (!GetCreatureTemplate(senderEntry))
        {
            TC_LOG_ERROR("sql.sql", "Table `mail_level_reward` has nonexistent sender creature entry (%u) for level %u that invalid not include any player races, ignoring.", senderEntry, level);
            continue;
        }

        _mailLevelRewardStore[level].push_back(MailLevelReward(raceMask, mailTemplateId, senderEntry));

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u level dependent mail rewards in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadTrainers()
{
    uint32 oldMSTime = getMSTime();

    // For reload case
    _trainers.clear();

    std::unordered_map<int32, std::vector<Trainer::Spell>> spellsByTrainer;
    if (QueryResult trainerSpellsResult = WorldDatabase.Query("SELECT TrainerId, SpellId, MoneyCost, ReqSkillLine, ReqSkillRank, ReqAbility1, ReqAbility2, ReqAbility3, ReqLevel FROM trainer_spell"))
    {
        do
        {
            Field* fields = trainerSpellsResult->Fetch();

            Trainer::Spell spell;
            uint32 trainerId = fields[0].GetUInt32();
            spell.SpellId = fields[1].GetUInt32();
            spell.MoneyCost = fields[2].GetUInt32();
            spell.ReqSkillLine = fields[3].GetUInt32();
            spell.ReqSkillRank = fields[4].GetUInt32();
            spell.ReqAbility[0] = fields[5].GetUInt32();
            spell.ReqAbility[1] = fields[6].GetUInt32();
            //spell.ReqAbility[2] = fields[7].GetUInt32();
            spell.ReqLevel = fields[8].GetUInt8();

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell.SpellId);
            if (!spellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Table `trainer_spell` references non-existing spell (SpellId: %u) for TrainerId %u, ignoring", spell.SpellId, trainerId);
                continue;
            }

            if (sDBCManager.GetTalentSpellCost(spell.SpellId))
            {
                TC_LOG_ERROR("sql.sql", "Table `trainer_spell` references non-existing spell (SpellId: %u) which is a talent, for TrainerId %u, ignoring", spell.SpellId, trainerId);
                continue;
            }

            if (spell.ReqSkillLine && !sSkillLineStore.LookupEntry(spell.ReqSkillLine))
            {
                TC_LOG_ERROR("sql.sql", "Table `trainer_spell` references non-existing skill (ReqSkillLine: %u) for TrainerId %u and SpellId %u, ignoring",
                    spell.ReqSkillLine, trainerId, spell.SpellId);
                continue;
            }

            bool allReqValid = true;
            for (std::size_t i = 0; i < spell.ReqAbility.size(); ++i)
            {
                uint32 requiredSpell = spell.ReqAbility[i];
                if (requiredSpell && !sSpellMgr->GetSpellInfo(requiredSpell))
                {
                    TC_LOG_ERROR("sql.sql", "Table `trainer_spell` references non-existing spell (ReqAbility" SZFMTD ": %u) for TrainerId %u and SpellId %u, ignoring",
                        i + 1, requiredSpell, trainerId, spell.SpellId);
                    allReqValid = false;
                }
            }

            if (!allReqValid)
                continue;

            spellsByTrainer[trainerId].push_back(spell);
        } while (trainerSpellsResult->NextRow());
    }

    if (QueryResult trainersResult = WorldDatabase.Query("SELECT Id, Type, Requirement, Greeting FROM trainer"))
    {
        do
        {
            Field* fields = trainersResult->Fetch();

            uint32 trainerId = fields[0].GetUInt32();
            Trainer::Type trainerType = Trainer::Type(fields[1].GetUInt8());
            uint32 requirement = fields[2].GetUInt32();
            std::string greeting = fields[3].GetString();
            std::vector<Trainer::Spell> spells;
            auto spellsItr = spellsByTrainer.find(trainerId);
            if (spellsItr != spellsByTrainer.end())
            {
                spells = std::move(spellsItr->second);
                spellsByTrainer.erase(spellsItr);
            }

            switch (trainerType)
            {
                case Trainer::Type::Class:
                case Trainer::Type::Pet:
                    if (requirement && !sChrClassesStore.LookupEntry(requirement))
                    {
                        TC_LOG_ERROR("sql.sql", "Table `trainer` references non-existing class requirement %u for TrainerId %u, ignoring", requirement, trainerId);
                        continue;
                    }
                    break;
                case Trainer::Type::Mount:
                    if (requirement && !sChrRacesStore.LookupEntry(requirement))
                    {
                        TC_LOG_ERROR("sql.sql", "Table `trainer` references non-existing race requirement %u for TrainerId %u, ignoring", requirement, trainerId);
                        continue;
                    }
                    break;
                case Trainer::Type::Tradeskill:
                    if (requirement && !sSpellMgr->GetSpellInfo(requirement))
                    {
                        TC_LOG_ERROR("sql.sql", "Table `trainer` references non-existing spell requirement %u for TrainerId %u, ignoring", requirement, trainerId);
                        continue;
                    }
                    break;
                default:
                    break;
            }

            _trainers.emplace(std::piecewise_construct, std::forward_as_tuple(trainerId), std::forward_as_tuple(trainerId, trainerType, requirement, std::move(greeting), std::move(spells)));
        } while (trainersResult->NextRow());
    }

    for (auto const& unusedSpells : spellsByTrainer)
    {
        for (Trainer::Spell const& unusedSpell : unusedSpells.second)
        {
            TC_LOG_ERROR("sql.sql", "Table `trainer_spell` references non-existing trainer (TrainerId: %u) for SpellId %u, ignoring", unusedSpells.first, unusedSpell.SpellId);
        }
    }

    if (QueryResult trainerLocalesResult = WorldDatabase.Query("SELECT Id, locale, Greeting_lang FROM trainer_locale"))
    {
        do
        {
            Field* fields = trainerLocalesResult->Fetch();
            uint32 trainerId = fields[0].GetUInt32();
            std::string localeName = fields[1].GetString();

            LocaleConstant locale = GetLocaleByName(localeName);
            if (locale == LOCALE_enUS)
                continue;

            if (Trainer::Trainer* trainer = Trinity::Containers::MapGetValuePtr(_trainers, trainerId))
                trainer->AddGreetingLocale(locale, fields[2].GetString());
            else
                TC_LOG_ERROR("sql.sql", "Table `trainer_locale` references non-existing trainer (TrainerId: %u) for locale %s, ignoring",
                    trainerId, localeName.c_str());
        } while (trainerLocalesResult->NextRow());
    }

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " Trainers in %u ms", _trainers.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureTrainers()
{
    uint32 oldMSTime = getMSTime();

    _creatureDefaultTrainers.clear();

    if (QueryResult result = WorldDatabase.Query("SELECT CreatureID, TrainerID, MenuID, OptionID FROM creature_trainer"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 creatureId = fields[0].GetUInt32();
            uint32 trainerId = fields[1].GetUInt32();
            uint32 gossipMenuId = fields[2].GetUInt32();
            uint32 gossipOptionId = fields[3].GetUInt32();

            if (!GetCreatureTemplate(creatureId))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature_trainer` references non-existing creature template (CreatureID: %u), ignoring", creatureId);
                continue;
            }

            if (!GetTrainer(trainerId))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature_trainer` references non-existing trainer (TrainerID: %u) for CreatureId %u MenuId %u OptionID %u, ignoring",
                    trainerId, creatureId, gossipMenuId, gossipOptionId);
                continue;
            }

            if (gossipMenuId || gossipOptionId)
            {
                Trinity::IteratorPair<GossipMenuItemsContainer::const_iterator> gossipMenuItems = GetGossipMenuItemsMapBounds(gossipMenuId);
                auto gossipOptionItr = std::find_if(gossipMenuItems.begin(), gossipMenuItems.end(), [gossipOptionId](std::pair<uint32 const, GossipMenuItems> const& entry)
                {
                    return entry.second.OptionID == gossipOptionId;
                });
                if (gossipOptionItr == gossipMenuItems.end())
                {
                    TC_LOG_ERROR("sql.sql", "Table `creature_trainer` references non-existing gossip menu option (MenuID %u OptionID %u) for CreatureID %u and TrainerID %u, ignoring",
                        gossipMenuId, gossipOptionId, creatureId, trainerId);
                    continue;
                }
            }

            _creatureDefaultTrainers[std::make_tuple(creatureId, gossipMenuId, gossipOptionId)] = trainerId;
        } while (result->NextRow());
    }

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " default trainers in %u ms", _creatureDefaultTrainers.size(), GetMSTimeDiffToNow(oldMSTime));
}

int ObjectMgr::LoadReferenceVendor(int32 vendor, int32 item, std::set<uint32>* skip_vendors)
{
    // find all items from the reference vendor
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_NPC_VENDOR_REF);
    stmt->setUInt32(0, uint32(item));
    PreparedQueryResult result = WorldDatabase.Query(stmt);

    if (!result)
        return 0;

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        int32 item_id = fields[0].GetInt32();

        // if item is a negative, its a reference
        if (item_id < 0)
            count += LoadReferenceVendor(vendor, -item_id, skip_vendors);
        else
        {
            VendorItem vItem;
            vItem.item = item_id;
            vItem.maxcount = fields[1].GetUInt32();
            vItem.incrtime = fields[2].GetUInt32();
            vItem.ExtendedCost = fields[3].GetUInt32();
            vItem.Type = fields[4].GetUInt8();

            if (!IsVendorItemValid(vendor, vItem, nullptr, skip_vendors))
                continue;

            VendorItemData& vList = _cacheVendorItemStore[vendor];

            vList.AddItem(std::move(vItem));
            ++count;
        }
    } while (result->NextRow());

    return count;
}

void ObjectMgr::LoadVendors()
{
    uint32 oldMSTime = getMSTime();

    // For reload case
    for (CacheVendorItemContainer::iterator itr = _cacheVendorItemStore.begin(); itr != _cacheVendorItemStore.end(); ++itr)
        itr->second.Clear();
    _cacheVendorItemStore.clear();

    std::set<uint32> skip_vendors;

    QueryResult result = WorldDatabase.Query("SELECT entry, item, maxcount, incrtime, ExtendedCost, type, PlayerConditionID FROM npc_vendor ORDER BY entry, slot ASC");
    if (!result)
    {

        TC_LOG_ERROR("server.loading", ">>  Loaded 0 Vendors. DB table `npc_vendor` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 entry        = fields[0].GetUInt32();
        int32 item_id      = fields[1].GetInt32();

        // if item is a negative, its a reference
        if (item_id < 0)
            count += LoadReferenceVendor(entry, -item_id, &skip_vendors);
        else
        {
            VendorItem vItem;
            vItem.item = item_id;
            vItem.maxcount = fields[2].GetUInt32();
            vItem.incrtime = fields[3].GetUInt32();
            vItem.ExtendedCost = fields[4].GetUInt32();
            vItem.Type = fields[5].GetUInt8();
            vItem.PlayerConditionId = fields[6].GetUInt32();

            if (!IsVendorItemValid(entry, vItem, nullptr, &skip_vendors))
                continue;

            VendorItemData& vList = _cacheVendorItemStore[entry];
            vList.AddItem(std::move(vItem));
            ++count;
        }
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %d Vendors in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadGossipMenu()
{
    uint32 oldMSTime = getMSTime();

    _gossipMenusStore.clear();

    //                                               0       1
    QueryResult result = WorldDatabase.Query("SELECT MenuID, TextID FROM gossip_menu");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 gossip_menu IDs. DB table `gossip_menu` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        GossipMenus gMenu;

        gMenu.MenuID = fields[0].GetUInt32();
        gMenu.TextID = fields[1].GetUInt32();

        if (!GetGossipText(gMenu.TextID))
        {
            TC_LOG_ERROR("sql.sql", "Table gossip_menu: ID %u is using non-existing TextID %u", gMenu.MenuID, gMenu.TextID);
            continue;
        }

        _gossipMenusStore.insert(GossipMenusContainer::value_type(gMenu.MenuID, gMenu));
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u gossip_menu IDs in %u ms", uint32(_gossipMenusStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadGossipMenuItems()
{
    uint32 oldMSTime = getMSTime();

    _gossipMenuItemsStore.clear();

    QueryResult result = WorldDatabase.Query(
        //      0       1         2           3           4                      5           6              7             8            9         10        11       12
        "SELECT MenuID, OptionID, OptionIcon, OptionText, OptionBroadcastTextID, OptionType, OptionNpcFlag, ActionMenuID, ActionPoiID, BoxCoded, BoxMoney, BoxText, BoxBroadcastTextID "
        "FROM gossip_menu_option ORDER BY MenuID, OptionID");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 gossip_menu_option IDs. DB table `gossip_menu_option` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        GossipMenuItems gMenuItem;

        gMenuItem.MenuID                = fields[0].GetUInt32();
        gMenuItem.OptionID              = fields[1].GetUInt32();
        gMenuItem.OptionIcon            = GossipOptionIcon(fields[2].GetUInt8());
        gMenuItem.OptionText            = fields[3].GetString();
        gMenuItem.OptionBroadcastTextID = fields[4].GetUInt32();
        gMenuItem.OptionType            = fields[5].GetUInt32();
        gMenuItem.OptionNpcFlag         = fields[6].GetUInt32();
        gMenuItem.ActionMenuID          = fields[7].GetUInt32();
        gMenuItem.ActionPoiID           = fields[8].GetUInt32();
        gMenuItem.BoxCoded              = fields[9].GetBool();
        gMenuItem.BoxMoney              = fields[10].GetUInt32();
        gMenuItem.BoxText               = fields[11].GetString();
        gMenuItem.BoxBroadcastTextID    = fields[12].GetUInt32();

        if (gMenuItem.OptionIcon >= GOSSIP_ICON_MAX)
        {
            TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option` for MenuId %u, OptionIndex %u has unknown icon id %u. Replacing with GOSSIP_ICON_CHAT", gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.OptionIcon);
            gMenuItem.OptionIcon = GOSSIP_ICON_CHAT;
        }

        if (gMenuItem.OptionBroadcastTextID)
        {
            if (!GetBroadcastText(gMenuItem.OptionBroadcastTextID))
            {
                TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option` for MenuId %u, OptionIndex %u has non-existing or incompatible OptionBroadcastTextID %u, ignoring.", gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.OptionBroadcastTextID);
                gMenuItem.OptionBroadcastTextID = 0;
            }
        }

        if (gMenuItem.OptionType >= GOSSIP_OPTION_MAX)
            TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option` for MenuId %u, OptionIndex %u has unknown option id %u. Option will not be used", gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.OptionType);

        if (gMenuItem.ActionPoiID && !GetPointOfInterest(gMenuItem.ActionPoiID))
        {
            TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option` for MenuId %u, OptionIndex %u use non-existing ActionPoiID %u, ignoring", gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.ActionPoiID);
            gMenuItem.ActionPoiID = 0;
        }

        if (gMenuItem.BoxBroadcastTextID)
        {
            if (!GetBroadcastText(gMenuItem.BoxBroadcastTextID))
            {
                TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option` for MenuId %u, OptionIndex %u has non-existing or incompatible BoxBroadcastTextID %u, ignoring.", gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.BoxBroadcastTextID);
                gMenuItem.BoxBroadcastTextID = 0;
            }
        }

        _gossipMenuItemsStore.insert(GossipMenuItemsContainer::value_type(gMenuItem.MenuID, gMenuItem));
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u gossip_menu_option entries in %u ms", uint32(_gossipMenuItemsStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

Trainer::Trainer const* ObjectMgr::GetTrainer(uint32 trainerId) const
{
    return Trinity::Containers::MapGetValuePtr(_trainers, trainerId);
}

uint32 ObjectMgr::GetCreatureTrainerForGossipOption(uint32 creatureId, uint32 gossipMenuId, uint32 gossipOptionId) const
{
    auto itr = _creatureDefaultTrainers.find(std::make_tuple(creatureId, gossipMenuId, gossipOptionId));
    if (itr != _creatureDefaultTrainers.end())
        return itr->second;

    return 0;
}

void ObjectMgr::AddVendorItem(uint32 entry, VendorItem const& vItem, bool persist /*= true*/)
{
    VendorItemData& vList = _cacheVendorItemStore[entry];
    vList.AddItem(vItem);

    if (persist)
    {
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_NPC_VENDOR);

        stmt->setUInt32(0, entry);
        stmt->setUInt32(1, vItem.item);
        stmt->setUInt8(2, vItem.maxcount);
        stmt->setUInt32(3, vItem.incrtime);
        stmt->setUInt32(4, vItem.ExtendedCost);
        stmt->setUInt8(5, vItem.Type);

        WorldDatabase.Execute(stmt);
    }
}

bool ObjectMgr::RemoveVendorItem(uint32 entry, uint32 item, uint8 type, bool persist /*= true*/)
{
    CacheVendorItemContainer::iterator  iter = _cacheVendorItemStore.find(entry);
    if (iter == _cacheVendorItemStore.end())
        return false;

    if (!iter->second.RemoveItem(item, type))
        return false;

    if (persist)
    {
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_NPC_VENDOR);

        stmt->setUInt32(0, entry);
        stmt->setUInt32(1, item);
        stmt->setUInt8(2, type);

        WorldDatabase.Execute(stmt);
    }

    return true;
}

bool ObjectMgr::IsVendorItemValid(uint32 vendor_entry, VendorItem const& vItem, Player* player, std::set<uint32>* skip_vendors, uint32 ORnpcflag) const
{
    CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(vendor_entry);
    if (!cInfo)
    {
        if (player)
            ChatHandler(player->GetSession()).SendSysMessage(LANG_COMMAND_VENDORSELECTION);
        else
            TC_LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` has data for nonexistent creature template (Entry: %u), ignore", vendor_entry);
        return false;
    }

    if (!((cInfo->npcflag | ORnpcflag) & UNIT_NPC_FLAG_VENDOR))
    {
        if (!skip_vendors || skip_vendors->count(vendor_entry) == 0)
        {
            if (player)
                ChatHandler(player->GetSession()).SendSysMessage(LANG_COMMAND_VENDORSELECTION);
            else
                TC_LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` has data for creature template (Entry: %u) without vendor flag, ignore", vendor_entry);

            if (skip_vendors)
                skip_vendors->insert(vendor_entry);
        }
        return false;
    }

    if ((vItem.Type == ITEM_VENDOR_TYPE_ITEM && !sObjectMgr->GetItemTemplate(vItem.item)) ||
        (vItem.Type == ITEM_VENDOR_TYPE_CURRENCY && !sCurrencyTypesStore.LookupEntry(vItem.item)))
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_ITEM_NOT_FOUND, vItem.item, vItem.Type);
        else
            TC_LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` for Vendor (Entry: %u) have in item list non-existed item (%u, type %u), ignore", vendor_entry, vItem.item, vItem.Type);
        return false;
    }

    if (vItem.ExtendedCost && !sItemExtendedCostStore.LookupEntry(vItem.ExtendedCost))
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_EXTENDED_COST_NOT_EXIST, vItem.ExtendedCost);
        else
            TC_LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` has Item (Entry: %u) with wrong ExtendedCost (%u) for vendor (%u), ignore", vItem.item, vItem.ExtendedCost, vendor_entry);
        return false;
    }

    if (vItem.Type == ITEM_VENDOR_TYPE_ITEM) // not applicable to currencies
    {
        if (vItem.maxcount > 0 && vItem.incrtime == 0)
        {
            if (player)
                ChatHandler(player->GetSession()).PSendSysMessage("MaxCount != 0 (%u) but IncrTime == 0", vItem.maxcount);
            else
                TC_LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` has `maxcount` (%u) for item %u of vendor (Entry: %u) but `incrtime`=0, ignore", vItem.maxcount, vItem.item, vendor_entry);
            return false;
        }
        else if (vItem.maxcount == 0 && vItem.incrtime > 0)
        {
            if (player)
                ChatHandler(player->GetSession()).PSendSysMessage("MaxCount == 0 but IncrTime<>= 0");
            else
                TC_LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` has `maxcount`=0 for item %u of vendor (Entry: %u) but `incrtime`<>0, ignore", vItem.item, vendor_entry);
            return false;
        }
    }

    VendorItemData const* vItems = GetNpcVendorItemList(vendor_entry);
    if (!vItems)
        return true;                                        // later checks for non-empty lists

    if (vItems->FindItemCostPair(vItem.item, vItem.ExtendedCost, vItem.Type))
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_ITEM_ALREADY_IN_LIST, vItem.item, vItem.ExtendedCost, vItem.Type);
        else
            TC_LOG_ERROR("sql.sql", "Table `npc_vendor` has duplicate items %u (with extended cost %u, type %u) for vendor (Entry: %u), ignoring", vItem.item, vItem.ExtendedCost, vItem.Type, vendor_entry);
        return false;
    }

    if (vItem.Type == ITEM_VENDOR_TYPE_CURRENCY && vItem.maxcount == 0)
    {
        TC_LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` have Item (Entry: %u, type: %u) with missing maxcount for vendor (%u), ignore", vItem.item, vItem.Type, vendor_entry);
        return false;
    }

    return true;
}

void ObjectMgr::LoadScriptNames()
{
    uint32 oldMSTime = getMSTime();

    // We insert an empty placeholder here so we can use the
    // script id 0 as dummy for "no script found".
    _scriptNamesStore.emplace_back("");

    QueryResult result = WorldDatabase.Query(
        "SELECT DISTINCT(ScriptName) FROM achievement_criteria_data WHERE ScriptName <> '' AND type = 11 "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM battleground_template WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM creature WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM creature_template WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM gameobject WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM gameobject_template WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM item_script_names WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM areatrigger_scripts WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM spell_script_names WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM transports WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM game_weather WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM conditions WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM outdoorpvp_template WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM world_map_template WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM world_state WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(ScriptName) FROM battlefield_template WHERE ScriptName <> '' "
        "UNION "
        "SELECT DISTINCT(script) FROM instance_template WHERE script <> ''");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded empty set of Script Names!");
        return;
    }

    _scriptNamesStore.reserve(result->GetRowCount() + 1);

    do
    {
        _scriptNamesStore.push_back((*result)[0].GetString());
    }
    while (result->NextRow());

    std::sort(_scriptNamesStore.begin(), _scriptNamesStore.end());

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " ScriptNames in %u ms", _scriptNamesStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

ObjectMgr::ScriptNameContainer const& ObjectMgr::GetAllScriptNames() const
{
    return _scriptNamesStore;
}

std::string const& ObjectMgr::GetScriptName(uint32 id) const
{
    static std::string const empty = "";
    return (id < _scriptNamesStore.size()) ? _scriptNamesStore[id] : empty;
}


uint32 ObjectMgr::GetScriptId(std::string const& name)
{
    // use binary search to find the script name in the sorted vector
    // assume "" is the first element
    if (name.empty())
        return 0;

    ScriptNameContainer::const_iterator itr = std::lower_bound(_scriptNamesStore.begin(), _scriptNamesStore.end(), name);
    if (itr == _scriptNamesStore.end() || *itr != name)
        return 0;

    return uint32(itr - _scriptNamesStore.begin());
}

void ObjectMgr::LoadBroadcastTexts()
{
    uint32 oldMSTime = getMSTime();

    _broadcastTextStore.clear(); // for reload case

    //                                               0   1            2      3      4         5         6         7            8            9            10              11        12
    QueryResult result = WorldDatabase.Query("SELECT ID, LanguageID, `Text`, Text1, EmoteID1, EmoteID2, EmoteID3, EmoteDelay1, EmoteDelay2, EmoteDelay3, SoundEntriesID, EmotesID, Flags FROM broadcast_text");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 broadcast texts. DB table `broadcast_text` is empty.");
        return;
    }

    _broadcastTextStore.reserve(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();

        BroadcastText bct;

        bct.Id = fields[0].GetUInt32();
        bct.LanguageID = fields[1].GetUInt32();
        bct.Text[DEFAULT_LOCALE] = fields[2].GetString();
        bct.Text1[DEFAULT_LOCALE] = fields[3].GetString();
        bct.EmoteId1 = fields[4].GetUInt32();
        bct.EmoteId2 = fields[5].GetUInt32();
        bct.EmoteId3 = fields[6].GetUInt32();
        bct.EmoteDelay1 = fields[7].GetUInt32();
        bct.EmoteDelay2 = fields[8].GetUInt32();
        bct.EmoteDelay3 = fields[9].GetUInt32();
        bct.SoundEntriesID = fields[10].GetUInt32();
        bct.EmotesID = fields[11].GetUInt32();
        bct.Flags = fields[12].GetUInt32();

        if (bct.SoundEntriesID)
        {
            if (!sSoundEntriesStore.LookupEntry(bct.SoundEntriesID))
            {
                TC_LOG_DEBUG("broadcasttext", "BroadcastText (Id: %u) in table `broadcast_text` has SoundEntriesID %u but sound does not exist.", bct.Id, bct.SoundEntriesID);
                bct.SoundEntriesID = 0;
            }
        }

        if (!GetLanguageDescByID(bct.LanguageID))
        {
            TC_LOG_DEBUG("broadcasttext", "BroadcastText (Id: %u) in table `broadcast_text` using LanguageID %u but Language does not exist.", bct.Id, bct.LanguageID);
            bct.LanguageID = LANG_UNIVERSAL;
        }

        if (bct.EmoteId1)
        {
            if (!sEmotesStore.LookupEntry(bct.EmoteId1))
            {
                TC_LOG_DEBUG("broadcasttext", "BroadcastText (Id: %u) in table `broadcast_text` has EmoteId1 %u but emote does not exist.", bct.Id, bct.EmoteId1);
                bct.EmoteId1 = 0;
            }
        }

        if (bct.EmoteId2)
        {
            if (!sEmotesStore.LookupEntry(bct.EmoteId2))
            {
                TC_LOG_DEBUG("broadcasttext", "BroadcastText (Id: %u) in table `broadcast_text` has EmoteId2 %u but emote does not exist.", bct.Id, bct.EmoteId2);
                bct.EmoteId2 = 0;
            }
        }

        if (bct.EmoteId3)
        {
            if (!sEmotesStore.LookupEntry(bct.EmoteId3))
            {
                TC_LOG_DEBUG("broadcasttext", "BroadcastText (Id: %u) in table `broadcast_text` has EmoteId3 %u but emote does not exist.", bct.Id, bct.EmoteId3);
                bct.EmoteId3 = 0;
            }
        }

        _broadcastTextStore[bct.Id] = bct;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " broadcast texts in %u ms", _broadcastTextStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadBroadcastTextLocales()
{
    uint32 oldMSTime = getMSTime();

    //                                               0   1        2     3
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, `Text`, Text1 FROM broadcast_text_locale");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 broadcast text locales. DB table `broadcast_text_locale` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 id               = fields[0].GetUInt32();
        std::string localeName  = fields[1].GetString();

        LocaleConstant locale = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        BroadcastTextContainer::iterator bct = _broadcastTextStore.find(id);
        if (bct == _broadcastTextStore.end())
        {
            TC_LOG_ERROR("sql.sql", "BroadcastText (Id: %u) in table `broadcast_text_locale` does not exist. Skipped!", id);
            continue;
        }

        AddLocaleString(fields[2].GetString(), locale, bct->second.Text);
        AddLocaleString(fields[3].GetString(), locale, bct->second.Text1);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u broadcast text locales in %u ms", uint32(_broadcastTextStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

CreatureBaseStats const* ObjectMgr::GetCreatureBaseStats(uint8 level, uint8 unitClass)
{
    CreatureBaseStatsContainer::const_iterator it = _creatureBaseStatsStore.find(MAKE_PAIR16(level, unitClass));

    if (it != _creatureBaseStatsStore.end())
        return &(it->second);

    struct DefaultCreatureBaseStats : public CreatureBaseStats
    {
        DefaultCreatureBaseStats()
        {
            BaseArmor = 1;
            for (uint8 j = 0; j < MAX_EXPANSIONS; ++j)
            {
                BaseHealth[j] = 1;
                BaseDamage[j] = 0.0f;
            }
            BaseMana = 0;
            AttackPower = 0;
            RangedAttackPower = 0;
        }
    };
    static const DefaultCreatureBaseStats defStats;
    return &defStats;
}

void ObjectMgr::LoadCreatureClassLevelStats()
{
    uint32 oldMSTime = getMSTime();
    //                                               0      1      2        3        4        5        6         7          8            9                  10           11           12           13
    QueryResult result = WorldDatabase.Query("SELECT level, class, basehp0, basehp1, basehp2, basehp3, basemana, basearmor, attackpower, rangedattackpower, damage_base, damage_exp1, damage_exp2, damage_exp3 FROM creature_classlevelstats");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature base stats. DB table `creature_classlevelstats` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint8 Level = fields[0].GetUInt8();
        uint8 Class = fields[1].GetUInt8();

        if (!Class || ((1 << (Class - 1)) & CLASSMASK_ALL_CREATURES) == 0)
            TC_LOG_ERROR("sql.sql", "Creature base stats for level %u has invalid class %u", Level, Class);

        CreatureBaseStats stats;

        for (uint8 i = 0; i < MAX_EXPANSIONS; ++i)
        {
            stats.BaseHealth[i] = fields[2 + i].GetUInt32();

            if (stats.BaseHealth[i] == 0)
            {
                TC_LOG_ERROR("sql.sql", "Creature base stats for class %u, level %u has invalid zero base HP[%u] - set to 1", Class, Level, i);
                stats.BaseHealth[i] = 1;
            }

            stats.BaseDamage[i] = fields[10 + i].GetFloat();
            if (stats.BaseDamage[i] < 0.0f)
            {
                TC_LOG_ERROR("sql.sql", "Creature base stats for class %u, level %u has invalid negative base damage[%u] - set to 0.0", Class, Level, i);
                stats.BaseDamage[i] = 0.0f;
            }
        }

        stats.BaseMana = fields[6].GetUInt16();
        stats.BaseArmor = fields[7].GetUInt16();

        stats.AttackPower = fields[8].GetUInt16();
        stats.RangedAttackPower = fields[9].GetUInt16();

        _creatureBaseStatsStore[MAKE_PAIR16(Level, Class)] = stats;

        ++count;
    }
    while (result->NextRow());

    CreatureTemplateContainer const* ctc = sObjectMgr->GetCreatureTemplates();
    for (CreatureTemplateContainer::const_iterator itr = ctc->begin(); itr != ctc->end(); ++itr)
    {
        for (uint16 lvl = itr->second.minlevel; lvl <= itr->second.maxlevel; ++lvl)
        {
            if (_creatureBaseStatsStore.find(MAKE_PAIR16(lvl, itr->second.unit_class)) == _creatureBaseStatsStore.end())
                TC_LOG_ERROR("sql.sql", "Missing base stats for creature class %u level %u", itr->second.unit_class, lvl);
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u creature base stats in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadFactionChangeAchievements()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_achievement");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 faction change achievement pairs. DB table `player_factionchange_achievement` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].GetUInt32();
        uint32 horde = fields[1].GetUInt32();

        if (!sAchievementMgr->GetAchievement(alliance))
            TC_LOG_ERROR("sql.sql", "Achievement %u (alliance_id) referenced in `player_factionchange_achievement` does not exist, pair skipped!", alliance);
        else if (!sAchievementMgr->GetAchievement(horde))
            TC_LOG_ERROR("sql.sql", "Achievement %u (horde_id) referenced in `player_factionchange_achievement` does not exist, pair skipped!", horde);
        else
            FactionChangeAchievements[alliance] = horde;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u faction change achievement pairs in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadFactionChangeItems()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_items");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 faction change item pairs. DB table `player_factionchange_items` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].GetUInt32();
        uint32 horde = fields[1].GetUInt32();

        if (!GetItemTemplate(alliance))
            TC_LOG_ERROR("sql.sql", "Item %u (alliance_id) referenced in `player_factionchange_items` does not exist, pair skipped!", alliance);
        else if (!GetItemTemplate(horde))
            TC_LOG_ERROR("sql.sql", "Item %u (horde_id) referenced in `player_factionchange_items` does not exist, pair skipped!", horde);
        else
            FactionChangeItems[alliance] = horde;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u faction change item pairs in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadFactionChangeQuests()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_quests");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 faction change quest pairs. DB table `player_factionchange_quests` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].GetUInt32();
        uint32 horde = fields[1].GetUInt32();

        if (!sObjectMgr->GetQuestTemplate(alliance))
            TC_LOG_ERROR("sql.sql", "Quest %u (alliance_id) referenced in `player_factionchange_quests` does not exist, pair skipped!", alliance);
        else if (!sObjectMgr->GetQuestTemplate(horde))
            TC_LOG_ERROR("sql.sql", "Quest %u (horde_id) referenced in `player_factionchange_quests` does not exist, pair skipped!", horde);
        else
            FactionChangeQuests[alliance] = horde;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u faction change quest pairs in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadFactionChangeReputations()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_reputations");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 faction change reputation pairs. DB table `player_factionchange_reputations` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].GetUInt32();
        uint32 horde = fields[1].GetUInt32();

        if (!sFactionStore.LookupEntry(alliance))
            TC_LOG_ERROR("sql.sql", "Reputation %u (alliance_id) referenced in `player_factionchange_reputations` does not exist, pair skipped!", alliance);
        else if (!sFactionStore.LookupEntry(horde))
            TC_LOG_ERROR("sql.sql", "Reputation %u (horde_id) referenced in `player_factionchange_reputations` does not exist, pair skipped!", horde);
        else
            FactionChangeReputation[alliance] = horde;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u faction change reputation pairs in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadFactionChangeSpells()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_spells");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 faction change spell pairs. DB table `player_factionchange_spells` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].GetUInt32();
        uint32 horde = fields[1].GetUInt32();

        if (!sSpellMgr->GetSpellInfo(alliance))
            TC_LOG_ERROR("sql.sql", "Spell %u (alliance_id) referenced in `player_factionchange_spells` does not exist, pair skipped!", alliance);
        else if (!sSpellMgr->GetSpellInfo(horde))
            TC_LOG_ERROR("sql.sql", "Spell %u (horde_id) referenced in `player_factionchange_spells` does not exist, pair skipped!", horde);
        else
            FactionChangeSpells[alliance] = horde;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u faction change spell pairs in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadFactionChangeTitles()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_titles");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 faction change title pairs. DB table `player_factionchange_title` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].GetUInt32();
        uint32 horde = fields[1].GetUInt32();

        if (!sCharTitlesStore.LookupEntry(alliance))
            TC_LOG_ERROR("sql.sql", "Title %u (alliance_id) referenced in `player_factionchange_title` does not exist, pair skipped!", alliance);
        else if (!sCharTitlesStore.LookupEntry(horde))
            TC_LOG_ERROR("sql.sql", "Title %u (horde_id) referenced in `player_factionchange_title` does not exist, pair skipped!", horde);
        else
            FactionChangeTitles[alliance] = horde;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u faction change title pairs in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadPhases()
{
    for (PhaseEntry const* phase : sPhaseStore)
        _phaseInfoById.emplace(std::make_pair(phase->ID, PhaseInfoStruct{ phase->ID, std::unordered_set<uint32>{} }));

    for (MapEntry const* map : sMapStore)
        if (map->ParentMapID != -1)
            _terrainSwapInfoById.emplace(std::make_pair(map->ID, TerrainSwapInfo{ map->ID, std::vector<uint32>{} }));

    TC_LOG_INFO("server.loading", "Loading Terrain World Map definitions...");
    LoadTerrainWorldMaps();

    TC_LOG_INFO("server.loading", "Loading Terrain Swap Default definitions...");
    LoadTerrainSwapDefaults();

    TC_LOG_INFO("server.loading", "Loading Phase Area definitions...");
    LoadAreaPhases();
}

void ObjectMgr::UnloadPhaseConditions()
{
    for (auto itr = _phaseInfoByArea.begin(); itr != _phaseInfoByArea.end(); ++itr)
        for (PhaseAreaInfo& phase : itr->second)
            phase.Conditions.clear();
}

void ObjectMgr::LoadTerrainWorldMaps()
{
    uint32 oldMSTime = getMSTime();

    //                                               0               1
    QueryResult result = WorldDatabase.Query("SELECT TerrainSwapMap, WorldMapArea FROM `terrain_worldmap`");


    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 terrain world maps. DB table `terrain_worldmap` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 mapId = fields[0].GetUInt32();
        uint32 worldMapArea = fields[1].GetUInt32();

        if (!sMapStore.LookupEntry(mapId))
        {
            TC_LOG_ERROR("sql.sql", "TerrainSwapMap %u defined in `terrain_worldmap` does not exist, skipped.", mapId);
            continue;
        }

        TerrainSwapInfo* terrainSwapInfo = &_terrainSwapInfoById[mapId];
        terrainSwapInfo->Id = mapId;
        terrainSwapInfo->UiMapPhaseIDs.push_back(worldMapArea);

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u terrain world maps in %u ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadTerrainSwapDefaults()
{
    uint32 oldMSTime = getMSTime();

    //                                               0       1
    QueryResult result = WorldDatabase.Query("SELECT MapId, TerrainSwapMap FROM `terrain_swap_defaults`");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 terrain swap defaults. DB table `terrain_swap_defaults` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 mapId = fields[0].GetUInt32();

        if (!sMapStore.LookupEntry(mapId))
        {
            TC_LOG_ERROR("sql.sql", "Map %u defined in `terrain_swap_defaults` does not exist, skipped.", mapId);
            continue;
        }

        uint32 terrainSwap = fields[1].GetUInt32();

        if (!sMapStore.LookupEntry(terrainSwap))
        {
            TC_LOG_ERROR("sql.sql", "TerrainSwapMap %u defined in `terrain_swap_defaults` does not exist, skipped.", terrainSwap);
            continue;
        }

        TerrainSwapInfo* terrainSwapInfo = &_terrainSwapInfoById[terrainSwap];
        terrainSwapInfo->Id = terrainSwap;
        _terrainSwapInfoByMap[mapId].push_back(terrainSwapInfo);

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u terrain swap defaults in %u ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadAreaPhases()
{
    uint32 oldMSTime = getMSTime();

    //                                               0       1
    QueryResult result = WorldDatabase.Query("SELECT AreaId, PhaseId FROM `phase_area`");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 phase areas. DB table `phase_area` is empty.");
        return;
    }

    auto getOrCreatePhaseIfMissing = [this](uint32 phaseId)
    {
        PhaseInfoStruct* phaseInfo = &_phaseInfoById[phaseId];
        phaseInfo->Id = phaseId;
        return phaseInfo;
    };

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 area = fields[0].GetUInt32();
        uint32 phaseId = fields[1].GetUInt32();
        if (!sAreaTableStore.LookupEntry(area))
        {
            TC_LOG_ERROR("sql.sql", "Area %u defined in `phase_area` does not exist, skipped.", area);
            continue;
        }

        if (!sPhaseStore.LookupEntry(phaseId))
        {
            TC_LOG_ERROR("sql.sql", "Phase %u defined in `phase_area` does not exist, skipped.", phaseId);
            continue;
        }

        PhaseInfoStruct* phase = getOrCreatePhaseIfMissing(phaseId);
        phase->Areas.insert(area);
        _phaseInfoByArea[area].emplace_back(phase);

        ++count;
    } while (result->NextRow());

    for (auto itr = _phaseInfoByArea.begin(); itr != _phaseInfoByArea.end(); ++itr)
    {
        for (PhaseAreaInfo& phase : itr->second)
        {
            uint32 parentAreaId = itr->first;
            do
            {
                AreaTableEntry const* area = sAreaTableStore.LookupEntry(parentAreaId);
                if (!area)
                    break;

                parentAreaId = area->ParentAreaID;
                if (!parentAreaId)
                    break;

                if (std::vector<PhaseAreaInfo>* parentAreaPhases = Trinity::Containers::MapGetValuePtr(_phaseInfoByArea, parentAreaId))
                    for (PhaseAreaInfo& parentAreaPhase : *parentAreaPhases)
                        if (parentAreaPhase.PhaseInfo->Id == phase.PhaseInfo->Id)
                            parentAreaPhase.SubAreaExclusions.insert(itr->first);

            } while (true);
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u phase areas in %u ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

bool PhaseInfoStruct::IsAllowedInArea(uint32 areaId) const
{
    return std::any_of(Areas.begin(), Areas.end(), [areaId](uint32 areaToCheck)
    {
        return DBCManager::IsInArea(areaId, areaToCheck);
    });
}

PhaseInfoStruct const* ObjectMgr::GetPhaseInfo(uint32 phaseId) const
{
    return Trinity::Containers::MapGetValuePtr(_phaseInfoById, phaseId);
}

std::vector<PhaseAreaInfo> const* ObjectMgr::GetPhasesForArea(uint32 areaId) const
{
    return Trinity::Containers::MapGetValuePtr(_phaseInfoByArea, areaId);
}

TerrainSwapInfo const* ObjectMgr::GetTerrainSwapInfo(uint32 terrainSwapId) const
{
    return Trinity::Containers::MapGetValuePtr(_terrainSwapInfoById, terrainSwapId);
}

GameObjectTemplate const* ObjectMgr::GetGameObjectTemplate(uint32 entry) const
{
    GameObjectTemplateContainer::const_iterator itr = _gameObjectTemplateStore.find(entry);
    if (itr != _gameObjectTemplateStore.end())
        return &(itr->second);

    return nullptr;
}

GameObjectTemplateAddon const* ObjectMgr::GetGameObjectTemplateAddon(uint32 entry) const
{
    auto itr = _gameObjectTemplateAddonStore.find(entry);
    if (itr != _gameObjectTemplateAddonStore.end())
        return &itr->second;

    return nullptr;
}

CreatureTemplate const* ObjectMgr::GetCreatureTemplate(uint32 entry) const
{
    CreatureTemplateContainer::const_iterator itr = _creatureTemplateStore.find(entry);
    if (itr != _creatureTemplateStore.end())
        return &(itr->second);

    return nullptr;
}

VehicleAccessoryList const* ObjectMgr::GetVehicleAccessoryList(Vehicle* veh) const
{
    if (Creature* cre = veh->GetBase()->ToCreature())
    {
        // Give preference to GUID-based accessories
        VehicleAccessoryContainer::const_iterator itr = _vehicleAccessoryStore.find(cre->GetSpawnId());
        if (itr != _vehicleAccessoryStore.end())
            return &itr->second;
    }

    // Otherwise return entry-based
    VehicleAccessoryContainer::const_iterator itr = _vehicleTemplateAccessoryStore.find(veh->GetCreatureEntry());
    if (itr != _vehicleTemplateAccessoryStore.end())
        return &itr->second;
    return nullptr;
}

DungeonEncounterList const* ObjectMgr::GetDungeonEncounterList(uint32 mapid, Difficulty difficulty) const
{
    return Trinity::Containers::MapGetValuePtr(_dungeonEncounterStore, MAKE_PAIR32(mapid, difficulty));
}

PlayerInfo const* ObjectMgr::GetPlayerInfo(uint32 race, uint32 class_) const
{
    if (race >= MAX_RACES)
        return nullptr;
    if (class_ >= MAX_CLASSES)
        return nullptr;
    PlayerInfo const* info = _playerInfo[race][class_];
    if (!info)
        return nullptr;
    return info;
}

CreatureStaticFlagsOverride const* ObjectMgr::GetCreatureStaticFlagsOverride(ObjectGuid::LowType spawnId, Difficulty difficultyId) const
{
    return Trinity::Containers::MapGetValuePtr(_creatureStaticFlagsOverrideStore, std::make_pair(spawnId, difficultyId));
}

void ObjectMgr::LoadGameObjectQuestItems()
{
    uint32 oldMSTime = getMSTime();

    //                                               0                1       2
    QueryResult result = WorldDatabase.Query("SELECT GameObjectEntry, ItemId, Idx FROM gameobject_questitem ORDER BY Idx ASC");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 gameobject quest items. DB table `gameobject_questitem` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();
        uint32 item  = fields[1].GetUInt32();
        uint32 idx   = fields[2].GetUInt32();

        GameObjectTemplate const* goInfo = GetGameObjectTemplate(entry);
        if (!goInfo)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject_questitem` has data for nonexistent gameobject (entry: %u, idx: %u), skipped", entry, idx);
            continue;
        };

        ItemEntry const* db2Data = sItemStore.LookupEntry(item);
        if (!db2Data)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject_questitem` has nonexistent item (ID: %u) in gameobject (entry: %u, idx: %u), skipped", item, entry, idx);
            continue;
        };

        _gameObjectQuestItemStore[entry].push_back(item);

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u gameobject quest items in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureQuestItems()
{
    uint32 oldMSTime = getMSTime();

    //                                               0              1       2
    QueryResult result = WorldDatabase.Query("SELECT CreatureEntry, ItemId, Idx FROM creature_questitem ORDER BY Idx ASC");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature quest items. DB table `creature_questitem` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();
        uint32 item  = fields[1].GetUInt32();
        uint32 idx   = fields[2].GetUInt32();

        CreatureTemplate const* creatureInfo = GetCreatureTemplate(entry);
        if (!creatureInfo)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_questitem` has data for nonexistent creature (entry: %u, idx: %u), skipped", entry, idx);
            continue;
        };

        ItemEntry const* db2Data = sItemStore.LookupEntry(item);
        if (!db2Data)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_questitem` has nonexistent item (ID: %u) in creature (entry: %u, idx: %u), skipped", item, entry, idx);
            continue;
        };

        _creatureQuestItemStore[entry].push_back(item);

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature quest items in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadTaxiNodeLevelData()
{
    uint32 oldMSTime = getMSTime();

    //                                               0            1
    QueryResult result = WorldDatabase.Query("SELECT TaxiNodeId, `Level` FROM taxi_level_data ORDER BY TaxiNodeId ASC");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 taxi node level definitions. DB table `taxi_level_data` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 taxiNodeId = fields[0].GetUInt16();
        uint8 level = fields[1].GetUInt8();

        TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(taxiNodeId);

        if (!node)
        {
            TC_LOG_ERROR("sql.sql", "Table `taxi_level_data` has data for nonexistent taxi node (ID: %u), skipped", taxiNodeId);
            continue;
        };

        _taxiNodeLevelDataStore.emplace(taxiNodeId, level);

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u taxi node level definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureStaticFlagsOverride()
{
    // reload case
    _creatureStaticFlagsOverrideStore.clear();

    uint32 oldMSTime = getMSTime();

    //                                               0        1             2             3             4             5             6
    QueryResult result = WorldDatabase.Query("SELECT SpawnId, DifficultyId, StaticFlags1, StaticFlags2, StaticFlags3, StaticFlags4, StaticFlags5 FROM creature_static_flags_override");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature static flag overrides. DB table `creature_static_flags_override` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType spawnId = fields[0].GetUInt32();
        Difficulty difficultyId = static_cast<Difficulty>(fields[1].GetUInt8());

        CreatureData const* creatureData = GetCreatureData(spawnId);
        if (!creatureData)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_static_flags_override` has data for nonexistent creature (SpawnId: %u), skipped", spawnId);
            continue;
        }

        if (!(creatureData->spawnMask & (1 << difficultyId)))
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_static_flags_override` has data for a creature that is not available for the specified DifficultyId (SpawnId: %u, DifficultyId: %u), skipped", spawnId, difficultyId);
            continue;
        }

        CreatureStaticFlagsOverride& staticFlagsOverride = _creatureStaticFlagsOverrideStore[std::make_pair(spawnId, difficultyId)];
        if (!fields[2].IsNull())
            staticFlagsOverride.StaticFlags1 = static_cast<CreatureStaticFlags>(fields[2].GetUInt32());
        if (!fields[3].IsNull())
            staticFlagsOverride.StaticFlags2 = static_cast<CreatureStaticFlags2>(fields[3].GetUInt32());
        if (!fields[4].IsNull())
            staticFlagsOverride.StaticFlags3 = static_cast<CreatureStaticFlags3>(fields[4].GetUInt32());
        if (!fields[5].IsNull())
            staticFlagsOverride.StaticFlags4 = static_cast<CreatureStaticFlags4>(fields[5].GetUInt32());
        if (!fields[6].IsNull())
            staticFlagsOverride.StaticFlags5 = static_cast<CreatureStaticFlags5>(fields[6].GetUInt32());

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature static flag overrides in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::InitializeQueriesData(QueryDataGroup mask)
{
    uint32 oldMSTime = getMSTime();

    // cache disabled
    if (!sWorld->getBoolConfig(CONFIG_CACHE_DATA_QUERIES))
    {
        TC_LOG_INFO("server.loading", ">> Query data caching is disabled. Skipped initialization.");
        return;
    }

    Trinity::ThreadPool pool;

    // Initialize Query data for creatures
    if (mask & QUERY_DATA_CREATURES)
        for (auto& creatureTemplatePair : _creatureTemplateStore)
            pool.PostWork([creature = &creatureTemplatePair.second]() { creature->InitializeQueryData(); });

    // Initialize Query Data for gameobjects
    if (mask & QUERY_DATA_GAMEOBJECTS)
        for (auto& gameObjectTemplatePair : _gameObjectTemplateStore)
            pool.PostWork([gobj = &gameObjectTemplatePair.second]() { gobj->InitializeQueryData(); });

    // Initialize Query Data for quests
    if (mask & QUERY_DATA_QUESTS)
        for (auto& questTemplatePair : _questTemplates)
            pool.PostWork([quest = &questTemplatePair.second]() { quest->InitializeQueryData(); });

    // Initialize Quest POI data
    if (mask & QUERY_DATA_POIS)
        for (auto& poiWrapperPair : _questPOIStore)
            pool.PostWork([poi = &poiWrapperPair.second]() { poi->InitializeQueryData(); });

    pool.Join();

    TC_LOG_INFO("server.loading", ">> Initialized query cache data in %u ms", GetMSTimeDiffToNow(oldMSTime));
}

void QuestPOIWrapper::InitializeQueryData()
{
    QueryDataBuffer = BuildQueryData();
}

ByteBuffer QuestPOIWrapper::BuildQueryData() const
{
    ByteBuffer tempBuffer;
    tempBuffer << uint32(POIData.QuestID);                                      // quest ID
    tempBuffer << uint32(POIData.QuestPOIBlobDataStats.size());                 // POI count

    for (QuestPOIBlobData const& questPOIBlobData : POIData.QuestPOIBlobDataStats)
    {
        tempBuffer << uint32(questPOIBlobData.BlobIndex);                       // POI index
        tempBuffer << int32(questPOIBlobData.ObjectiveIndex);                   // objective index
        tempBuffer << uint32(questPOIBlobData.MapID);                           // mapid
        tempBuffer << uint32(questPOIBlobData.WorldMapAreaID);                  // areaid
        tempBuffer << uint32(questPOIBlobData.Floor);                           // floorid
        tempBuffer << uint32(questPOIBlobData.Priority);                        // priority
        tempBuffer << uint32(questPOIBlobData.Flags);                           // flags
        tempBuffer << uint32(questPOIBlobData.QuestPOIBlobPointStats.size());   // POI points count

        for (QuestPOIBlobPoint const& questPOIBlobPoint : questPOIBlobData.QuestPOIBlobPointStats)
        {
            tempBuffer << int32(questPOIBlobPoint.X); // POI point x
            tempBuffer << int32(questPOIBlobPoint.Y); // POI point y
        }
    }

    return tempBuffer;
}

void ObjectMgr::LoadSummonPropertiesParameters()
{
    _summonPropertiesParametersStore.clear();

    uint32 oldMSTime = getMSTime();

    //                                                0        1
    QueryResult result = WorldDatabase.Query("SELECT `RecID`, `ParamType` FROM summon_properties_parameters");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 summon properties parameters. DB table `summon_properties_parameters` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 recID = fields[0].GetUInt32();
        uint8 parameterType = fields[1].GetUInt8();

        if (!sSummonPropertiesStore.LookupEntry(recID))
        {
            TC_LOG_ERROR("sql.sql", "Table `summon_properties_parameters` has data for nonexistent summon properties record (ID: %u), skipped", recID);
            continue;
        };

        _summonPropertiesParametersStore[recID] = parameterType;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u summon properties parameters in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

uint8 const* ObjectMgr::GetSummonPropertiesParameter(uint32 summonPropertiesRecID) const
{
    return Trinity::Containers::MapGetValuePtr(_summonPropertiesParametersStore, summonPropertiesRecID);
}
