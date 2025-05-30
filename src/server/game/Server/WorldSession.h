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

/// \addtogroup u2w
/// @{
/// \file

#ifndef __WORLDSESSION_H
#define __WORLDSESSION_H

#include "Common.h"
#include "AsyncCallbackProcessor.h"
#include "DatabaseEnvFwd.h"
#include "LockedQueue.h"
#include "ObjectGuid.h"
#include "Packet.h"
#include "SharedDefines.h"
#include <map>
#include <unordered_map>
#include <boost/circular_buffer_fwd.hpp>

class BigNumber;
class Creature;
class GameClient;
class GameObject;
class InstanceSave;
class Item;
class LoginQueryHolder;
class Object;
class Player;
class Quest;
class SpellCastTargets;
class Unit;
class Warden;
class WorldPacket;
class WorldSession;
class WorldSocket;
struct AddonInfo;
struct AreaTableEntry;
struct AuctionEntry;
struct DeclinedName;
struct ItemTemplate;
struct MovementInfo;
struct Petition;

namespace lfg
{
struct LfgJoinResultData;
struct LfgPlayerBoot;
struct LfgProposal;
struct LfgQueueStatusData;
struct LfgPlayerRewardData;
struct LfgRoleCheck;
struct LfgUpdateData;
enum LfgTeleportResult : uint8;
}

namespace rbac
{
class RBACData;
}

namespace WorldPackets
{
    namespace Archaeology
    {
        class RequestResearchHistory;
    }

    namespace Auth
    {
        enum class ConnectToSerial : uint32;
    }

    namespace Character
    {
        class CharDelete;
        class EnumCharacters;
        class GenerateRandomCharacterName;
        class LoadingScreenNotify;
        class PlayerLogin;
        class ShowingCloak;
        class ShowingHelm;

        enum class LoginFailureReason : uint8;
    }

    namespace Chat
    {
        class SendTextEmote;
    }

    namespace Combat
    {
        class AttackSwing;
        class CAttackStop;
        class SetSheathed;
    }

    namespace LFG
    {
        class LFGJoin;
        class LFGLeave;
        class LFGProposalResponse;
        class LFGSetRoles;
        class LFGSetComment;
        class LFGBootPlayerVote;
        class LFGTeleport;
        class LFGGetSystemInfo;
        class LFGUpdateStatus;
        class RoleChosen;
        class LFGRoleCheckUpdate;
        class LFGJoinResult;
        class LFGQueueStatus;
        class LFGPlayerReward;
        class LfgBootPlayer;
        class LFGOfferContinue;
    }

    namespace Guild
    {
        class GuildBankQueryResults;
        class GuildBankActivate;
        class GuildBankQueryTab;
        class GuildBankBuyTab;
        class GuildBankUpdateTab;
        class GuildBankDepositMoney;
        class GuildBankWithdrawMoney;
        class GuildPermissionsQuery;
        class GuildBankRemainingWithdrawMoneyQuery;
        class GuildBankLogQuery;
        class GuildBankSwapItems;
        class GuildBankTextQuery;
        class GuildBankSetTabText;
        class GuildXPQuery;
        class GuildGetRoster;
        class GuildQueryRecipes;
        class GuildQueryMembersForRecipe;
        class GuildQueryMemberRecipes;
        class GuildChallengeUpdateRequest;
        class RequestGuildRewardsList;
        class ReplaceGuildMaster;
        class SetGuildMaster;
    }

    namespace Inspect
    {
        class RequestHonorStats;
        class RequestInspectRatedBgStats;
    }

    namespace Item
    {
        class DestroyItem;
        class TransmogrifyItems;
    }

    namespace Misc
    {
        class CompleteCinematic;
        class CompleteMovie;
        class NextCinematicCamera;
        class OpeningCinematic;
        class SetCurrencyFlags;
    }

    namespace Movement
    {
        class MoveTeleportAck;
        class SetActiveMover;
        class SuspendTokenResponse;
    }

    namespace NPC
    {
        class Hello;
        class TrainerBuySpell;
    }

    namespace Party
    {
        class SetEveryoneIsAssistant;
        class PartyInviteClient;
        class PartyInviteResponse;
        class SetRole;
    }

    namespace Quest
    {
        class QuestGiverAcceptQuest;
        class QuestGiverChooseReward;
        class QuestGiverCompleteQuest;
        class QuestGiverHello;
        class QuestGiverQueryQuest;
        class QuestGiverRequestReward;
        class QuestGiverStatusMultipleQuery;
        class QuestGiverStatusQuery;
        class QuestLogRemoveQuest;
        class QuestConfirmAccept;
        class QueryQuestInfo;
    }

    namespace Query
    {
        class QueryCreature;
        class QueryGameObject;
        class QuestPOIQuery;
        class DBQueryBulk;
    }

    namespace Reforge
    {
        class ReforgeItem;
    }

    namespace Spells
    {
        class CastSpell;
        class CancelQueuedSpell;
        class UseItem;
        class UpdateMissileTrajectory;
    }

    namespace Ticket
    {
        class Complaint;
        class SupportTicketSubmitSuggestion;
        class SupportTicketSubmitBug;
        class SupportTicketSubmitComplaint;
    }

    namespace Totem
    {
        class TotemDestroyed;
    }

    namespace Trade
    {
        class AcceptTrade;
        class BeginTrade;
        class BusyTrade;
        class CancelTrade;
        class ClearTradeItem;
        class IgnoreTrade;
        class InitiateTrade;
        class SetTradeGold;
        class SetTradeItem;
        class TradeStatus;
        class UnacceptTrade;
    }

    namespace Who
    {
        class WhoRequestPkt;
    }
}

enum AccountDataType
{
    GLOBAL_CONFIG_CACHE             = 0,                    // 0x01 g
    PER_CHARACTER_CONFIG_CACHE      = 1,                    // 0x02 p
    GLOBAL_BINDINGS_CACHE           = 2,                    // 0x04 g
    PER_CHARACTER_BINDINGS_CACHE    = 3,                    // 0x08 p
    GLOBAL_MACROS_CACHE             = 4,                    // 0x10 g
    PER_CHARACTER_MACROS_CACHE      = 5,                    // 0x20 p
    PER_CHARACTER_LAYOUT_CACHE      = 6,                    // 0x40 p
    PER_CHARACTER_CHAT_CACHE        = 7                     // 0x80 p
};

#define NUM_ACCOUNT_DATA_TYPES        8

#define GLOBAL_CACHE_MASK           0x15
#define PER_CHARACTER_CACHE_MASK    0xEA

#define REGISTERED_ADDON_PREFIX_SOFTCAP 64

struct AccountData
{
    AccountData() : Time(0), Data("") { }

    time_t Time;
    std::string Data;
};

enum PartyOperation
{
    PARTY_OP_INVITE   = 0,
    PARTY_OP_UNINVITE = 1,
    PARTY_OP_LEAVE    = 2,
    PARTY_OP_SWAP     = 4
};

enum ChangeDynamicDifficultyResult
{
    ERR_DIFFICULTY_CHANGE_COOLDOWN_S                            = 0, // sends the remaining time in seconds to the client
    ERR_DIFFICULTY_CHANGE_WORLDSTATE                            = 1,
    ERR_DIFFICULTY_CHANGE_ENCOUNTER                             = 2,
    ERR_DIFFICULTY_CHANGE_COMBAT                                = 3,
    ERR_DIFFICULTY_CHANGE_PLAYER_BUSY                           = 4,
    ERR_DIFFICULTY_CHANGE_ALREADY_STARTED                       = 6,
    ERR_DIFFICULTY_CHANGE_OTHER_HEROIC_S                        = 8, // sends it when someone is locked for the encounter on heroic, sends the locked player packed guid
    ERR_DIFFICULTY_CHANGE_HEROIC_INSTANCE_ALREADY_RUNNING       = 9,
    ERR_DIFFICULTY_DISABLED_IN_LFG                              = 10,
    ERR_DIFFICULTY_CHANGE_UPDATE_TIME                           = 5, // sends the remaining time in time_t
    ERR_DIFFICULTY_CHANGE_UPDATE_MAP_DIFFICULTY_ENTRY           = 7, // sends the ID of MapDifficulty
    ERR_DIFFICULTY_CHANGE_SUCCESS                               = 11 // sends remaining time in time_t and mapId
};

enum BarberShopResult
{
    BARBER_SHOP_RESULT_SUCCESS      = 0,
    BARBER_SHOP_RESULT_NO_MONEY     = 1,
    BARBER_SHOP_RESULT_NOT_ON_CHAIR = 2,
    BARBER_SHOP_RESULT_NO_MONEY_2   = 3
};

enum BFLeaveReason
{
    BF_LEAVE_REASON_CLOSE     = 0x00000001,
    //BF_LEAVE_REASON_UNK1      = 0x00000002, (not used)
    //BF_LEAVE_REASON_UNK2      = 0x00000004, (not used)
    BF_LEAVE_REASON_EXITED    = 0x00000008,
    BF_LEAVE_REASON_LOW_LEVEL = 0x00000010
};

enum ChatRestrictionType
{
    ERR_CHAT_RESTRICTED = 0,
    ERR_CHAT_THROTTLED  = 1,
    ERR_USER_SQUELCHED  = 2,
    ERR_YELL_RESTRICTED = 3
};

#define DB2_REPLY_SPARSE 2442913102
#define DB2_REPLY_ITEM   1344507586

enum DeclinedNameResult
{
    DECLINED_NAMES_RESULT_SUCCESS = 0,
    DECLINED_NAMES_RESULT_ERROR   = 1
};

enum TutorialsFlag : uint8
{
    TUTORIALS_FLAG_NONE                           = 0x00,
    TUTORIALS_FLAG_CHANGED                        = 0x01,
    TUTORIALS_FLAG_LOADED_FROM_DB                 = 0x02
};

//class to deal with packet processing
//allows to determine if next packet is safe to be processed
class PacketFilter
{
public:
    explicit PacketFilter(WorldSession* pSession) : m_pSession(pSession) { }
    virtual ~PacketFilter() { }

    virtual bool Process(WorldPacket* /*packet*/) { return true; }
    virtual bool ProcessUnsafe() const { return true; }

protected:
    WorldSession* const m_pSession;

private:
    PacketFilter(PacketFilter const& right) = delete;
    PacketFilter& operator=(PacketFilter const& right) = delete;
};
//process only thread-safe packets in Map::Update()
class MapSessionFilter : public PacketFilter
{
public:
    explicit MapSessionFilter(WorldSession* pSession) : PacketFilter(pSession) { }
    ~MapSessionFilter() { }

    virtual bool Process(WorldPacket* packet) override;
    //in Map::Update() we do not process player logout!
    virtual bool ProcessUnsafe() const override { return false; }
};

//class used to filer only thread-unsafe packets from queue
//in order to update only be used in World::UpdateSessions()
class WorldSessionFilter : public PacketFilter
{
public:
    explicit WorldSessionFilter(WorldSession* pSession) : PacketFilter(pSession) { }
    ~WorldSessionFilter() { }

    virtual bool Process(WorldPacket* packet) override;
};

// Proxy structure to contain data passed to callback function,
// only to prevent bloating the parameter list
class CharacterCreateInfo
{
    friend class WorldSession;
    friend class Player;

    protected:
        /// User specified variables
        std::string Name;
        uint8 Race       = 0;
        uint8 Class      = 0;
        uint8 Gender     = GENDER_NONE;
        uint8 Skin       = 0;
        uint8 Face       = 0;
        uint8 HairStyle  = 0;
        uint8 HairColor  = 0;
        uint8 FacialHair = 0;
        uint8 OutfitId   = 0;

        /// Server side data
        uint8 CharCount = 0;
};

struct CharacterRenameInfo
{
    friend class WorldSession;

    protected:
        ObjectGuid Guid;
        std::string Name;
};

struct CharacterCustomizeInfo : public CharacterRenameInfo
{
    friend class Player;
    friend class WorldSession;

    protected:
        uint8 Gender     = GENDER_NONE;
        uint8 Skin       = 0;
        uint8 Face       = 0;
        uint8 HairStyle  = 0;
        uint8 HairColor  = 0;
        uint8 FacialHair = 0;
};

struct CharacterFactionChangeInfo : public CharacterCustomizeInfo
{
    friend class Player;
    friend class WorldSession;

    protected:
        uint8 Race = 0;
        bool FactionChange = false;
};

struct PacketCounter
{
    time_t lastReceiveTime;
    uint32 amountCounter;
};

/// Player session in the World
class TC_GAME_API WorldSession
{
    public:
        WorldSession(uint32 id, std::string&& name, uint32 battlenetAccountId, std::shared_ptr<WorldSocket> sock, AccountTypes sec, uint8 expansion, time_t mute_time, LocaleConstant locale, uint32 recruiter, bool isARecruiter);
        ~WorldSession();

        bool PlayerLoading() const { return !m_playerLoading.IsEmpty(); }
        bool PlayerLogout() const { return m_playerLogout; }
        bool PlayerLogoutWithSave() const { return m_playerLogout && m_playerSave; }
        bool PlayerRecentlyLoggedOut() const { return m_playerRecentlyLogout; }
        bool PlayerDisconnected() const;

        void ReadAddonsInfo(ByteBuffer& data);
        void SendAddonsInfo();
        bool IsAddonRegistered(const std::string& prefix) const;
        void SendPacket(WorldPacket const* packet, bool forced = false);
        void AddInstanceConnection(std::shared_ptr<WorldSocket> sock) { m_Socket[1] = sock; }

        void SendNotification(const char *format, ...) ATTR_PRINTF(2, 3);
        void SendNotification(uint32 string_id, ...);
        void SendPetNameInvalid(uint32 error, std::string const& name, DeclinedName *declinedName);
        void SendPartyResult(PartyOperation operation, std::string const& member, PartyResult res, uint32 val = 0);
        void SendAreaTriggerMessage(char const* Text, ...) ATTR_PRINTF(2, 3);
        void SendQueryTimeResponse();

        void SendAuthResponse(uint8 code, bool queued, uint32 queuePos = 0);
        void SendClientCacheVersion(uint32 version);

        void InitializeSession();
        void InitializeSessionCallback(CharacterDatabaseQueryHolder const& realmHolder);

        GameClient* GetGameClient() const { return _gameClient; };

        rbac::RBACData* GetRBACData();
        bool HasPermission(uint32 permissionId);
        void LoadPermissions();
        QueryCallback LoadPermissionsAsync();
        void InvalidateRBACData(); // Used to force LoadPermissions at next HasPermission check

        AccountTypes GetSecurity() const { return _security; }
        uint32 GetAccountId() const { return _accountId; }
        uint32 GetBattlenetAccountId() const { return _battlenetAccountId; }
        Player* GetPlayer() const { return _player; }
        std::string const& GetPlayerName() const;
        std::string GetPlayerInfo() const;

        ObjectGuid::LowType GetGUIDLow() const;
        void SetSecurity(AccountTypes security) { _security = security; }
        std::string const& GetRemoteAddress() const { return m_Address; }
        void SetPlayer(Player* player);
        uint8 GetAccountExpansion() const { return m_accountExpansion; }
        uint8 GetExpansion() const { return m_expansion; }

        void InitWarden(BigNumber* k, std::string const& os);

        /// Session in auth.queue currently
        void SetInQueue(bool state) { m_inQueue = state; }

        /// Is the user engaged in a log out process?
        bool isLogingOut() const { return _logoutTime || m_playerLogout; }

        /// Engage the logout process for the user
        void LogoutRequest(time_t requestTime)
        {
            _logoutTime = requestTime;
        }

        /// Is logout cooldown expired?
        bool ShouldLogOut(time_t currTime) const
        {
            return (_logoutTime > 0 && currTime >= _logoutTime + 20);
        }

        void LogoutPlayer(bool save);
        void KickPlayer();

        void QueuePacket(WorldPacket* new_packet);
        bool Update(uint32 diff, PacketFilter& updater);

        /// Handle the authentication waiting queue (to be completed)
        void SendAuthWaitQue(uint32 position);

        void SendNameQueryOpcode(ObjectGuid guid);

        void SendTrainerList(Creature* npc, uint32 trainerId);
        void SendListInventory(ObjectGuid guid);
        void SendShowBank(ObjectGuid guid);
        bool CanOpenMailBox(ObjectGuid guid);
        void SendShowMailBox(ObjectGuid guid);
        void SendTabardVendorActivate(ObjectGuid guid);
        void SendSpiritResurrect();
        void SendBindPoint(Creature* npc);

        void SendAttackStop(Unit const* enemy);

        void SendTradeStatus(WorldPackets::Trade::TradeStatus& info);
        void SendUpdateTrade(bool trader_data = true);
        void SendCancelTrade();

        void SendPetitionQueryOpcode(ObjectGuid petitionguid);

        // Spell
        void HandleClientCastFlags(WorldPacket& recvPacket, uint8 castFlags, SpellCastTargets& targets);

        // Pet
        void SendPetNameQuery(ObjectGuid guid, uint32 petnumber);
        void SendStablePet(ObjectGuid guid);
        void SendStableResult(uint8 guid);
        bool CheckStableMaster(ObjectGuid guid);
        void UpdatePetSlot(uint32 petNumber, uint8 oldPetSlot, uint8 newPetSlot);
        void SendPetSlotUpdated(int32 petNumberA, int32 petSlotA, int32 petNumberB, int32 petSlotB);
        void SendPetAdded(int32 petSlot, int32 petNumber, int32 creatureID, int32 level, std::string name);

        // Account Data
        AccountData* GetAccountData(AccountDataType type) { return &m_accountData[type]; }
        void SetAccountData(AccountDataType type, time_t tm, std::string const& data);
        void SendAccountDataTimes(uint32 mask);
        void LoadAccountData(PreparedQueryResult result, uint32 mask);

        void LoadTutorialsData(PreparedQueryResult result);
        void SendTutorialsData();
        void SaveTutorialsData(CharacterDatabaseTransaction& trans);
        uint32 GetTutorialInt(uint8 index) const { return m_Tutorials[index]; }
        void SetTutorialInt(uint8 index, uint32 value)
        {
            if (m_Tutorials[index] != value)
            {
                m_Tutorials[index] = value;
                m_TutorialsChanged |= TUTORIALS_FLAG_CHANGED;
            }
        }

        //auction
        void SendAuctionHello(ObjectGuid guid, Creature* unit);
        void SendAuctionCommandResult(AuctionEntry* auction, uint32 Action, uint32 ErrorCode, uint32 bidError = 0);
        void SendAuctionBidderNotification(uint32 location, uint32 auctionId, ObjectGuid bidder, uint64 bidSum, uint64 diff, uint32 item_template);
        void SendAuctionOwnerNotification(AuctionEntry* auction);
        void SendAuctionRemovedNotification(uint32 auctionId, uint32 itemEntry, int32 randomPropertyId);

        //Item Enchantment
        void SendEnchantmentLog(ObjectGuid target, ObjectGuid caster, uint32 itemId, uint32 enchantId);
        void SendItemEnchantTimeUpdate(ObjectGuid Playerguid, ObjectGuid Itemguid, uint32 slot, uint32 Duration);

        //Taxi
        void SendTaxiStatus(ObjectGuid guid);
        void SendTaxiMenu(Creature* unit);
        void SendDoFlight(uint32 mountDisplayId, uint32 path, uint32 pathNode = 0);
        bool SendLearnNewTaxiNode(Creature* unit);
        void SendDiscoverNewTaxiNode(uint32 nodeid);

        // Guild/Arena Team
        void SendArenaTeamCommandResult(uint32 team_action, std::string const& team, std::string const& player, uint32 error_id = 0);
        void SendNotInArenaTeamPacket(uint8 type);
        void SendPetitionShowList(ObjectGuid guid);

        void DoLootRelease(ObjectGuid lguid);

        // Account mute time
        time_t m_muteTime;

        // Locales
        LocaleConstant GetSessionDbcLocale() const { return m_sessionDbcLocale; }
        LocaleConstant GetSessionDbLocaleIndex() const { return m_sessionDbLocaleIndex; }
        char const* GetTrinityString(uint32 entry) const;

        uint32 GetLatency() const { return m_latency; }
        void SetLatency(uint32 latency) { m_latency = latency; }

        std::atomic<int32> m_timeOutTime;

        void UpdateTimeOutTime(uint32 diff)
        {
            m_timeOutTime -= int32(diff);
        }

        void ResetTimeOutTime(bool onlyActive);

        bool IsConnectionIdle() const
        {
            return m_timeOutTime <= 0 && !m_inQueue;
        }

        // Recruit-A-Friend Handling
        uint32 GetRecruiterId() const { return recruiterId; }
        bool IsARecruiter() const { return isRecruiter; }

        // Time Synchronisation
        void ResetTimeSync();
        void SendTimeSync();

    public:                                                 // opcodes handlers

        void Handle_NULL(WorldPacket& recvPacket);          // not used
        void Handle_EarlyProccess(WorldPacket& recvPacket); // just mark packets processed in WorldSocket::OnRead
        void Handle_ServerSide(WorldPacket& recvPacket);    // sever side only, can't be accepted from client
        void Handle_Deprecated(WorldPacket& recvPacket);    // never used anymore by client
        void LogUnprocessedTail(WorldPacket const* packet);

        void HandleCharEnum(PreparedQueryResult result);
        void HandleCharEnumOpcode(WorldPackets::Character::EnumCharacters& /*enumCharacters*/);
        void HandleCharDeleteOpcode(WorldPackets::Character::CharDelete& packet);
        void HandleCharCreateOpcode(WorldPacket& recvPacket);
        void HandlePlayerLoginOpcode(WorldPackets::Character::PlayerLogin& packet);

        void SendConnectToInstance(WorldPackets::Auth::ConnectToSerial serial);
        void HandleContinuePlayerLogin();
        void AbortLogin(WorldPackets::Character::LoginFailureReason reason);
        void HandleLoadScreenOpcode(WorldPackets::Character::LoadingScreenNotify& packet);
        void HandlePlayerLogin(LoginQueryHolder const& holder);
        void HandleCharFactionOrRaceChange(WorldPacket& recvData);
        void HandleCharFactionOrRaceChangeCallback(std::shared_ptr<CharacterFactionChangeInfo> factionChangeInfo, PreparedQueryResult result);
        void HandleCharRenameOpcode(WorldPacket& recvData);
        void HandleCharRenameCallback(std::shared_ptr<CharacterRenameInfo> renameInfo, PreparedQueryResult result);
        void HandleSetPlayerDeclinedNames(WorldPacket& recvData);
        void HandleAlterAppearance(WorldPacket& recvData);
        void HandleCharCustomize(WorldPacket& recvData);
        void HandleCharCustomizeCallback(std::shared_ptr<CharacterCustomizeInfo> customizeInfo, PreparedQueryResult result);
        void HandleOpeningCinematic(WorldPackets::Misc::OpeningCinematic& packet);

        void HandleRandomizeCharNameOpcode(WorldPackets::Character::GenerateRandomCharacterName& packet);
        void HandleReorderCharacters(WorldPacket& recvData);
        void SendCharCreate(ResponseCodes result);
        void SendCharDelete(ResponseCodes result);
        void SendCharRename(ResponseCodes result, CharacterRenameInfo const* renameInfo);
        void SendCharCustomize(ResponseCodes result, CharacterCustomizeInfo const* customizeInfo);
        void SendCharFactionChange(ResponseCodes result, CharacterFactionChangeInfo const* factionChangeInfo);
        void SendSetPlayerDeclinedNamesResult(DeclinedNameResult result, ObjectGuid guid);
        void SendBarberShopResult(BarberShopResult result);

        // played time
        void HandlePlayedTime(WorldPacket& recvPacket);

        // cemetery/graveyard related
        void HandleReturnToGraveyard(WorldPacket& recvPacket);
        void HandleRequestCemeteryList(WorldPacket& recvPacket);

        // new inspect
        void HandleInspectOpcode(WorldPacket& recvPacket);

        // new party stats
        void HandleRequestHonorStatsOpcode(WorldPackets::Inspect::RequestHonorStats& packet);
        void HandleRequestInspectRatedBgStatsOpcode(WorldPackets::Inspect::RequestInspectRatedBgStats& packet);

        void HandleForceSpeedChangeAck(WorldPacket& recvData);
        void HandleMoveKnockBackAck(WorldPacket& recvPacket);
        void HandleMoveTeleportAck(WorldPackets::Movement::MoveTeleportAck& packet);
        void HandleMoveWaterWalkAck(WorldPacket& recvPacket);
        void HandleFeatherFallAck(WorldPacket& recvData);
        void HandleMoveHoverAck(WorldPacket& recvData);
        void HandleMoveUnRootAck(WorldPacket& recvPacket);
        void HandleMoveRootAck(WorldPacket& recvPacket);
        void HandleMoveSetCanFlyAckOpcode(WorldPacket& recvData);
        void HandleMoveSetCanTransitionBetweenSwinAndFlyAck(WorldPacket& recvData);
        void HandleMoveGravityDisableAck(WorldPacket& recvData);

        void HandleSetCollisionHeightAck(WorldPacket& recvData);

        void HandleMountSpecialAnimOpcode(WorldPacket& recvdata);

        // character view
        void HandleShowingHelmOpcode(WorldPackets::Character::ShowingHelm& packet);
        void HandleShowingCloakOpcode(WorldPackets::Character::ShowingCloak& packet);

        // repair
        void HandleRepairItemOpcode(WorldPacket& recvPacket);

        void HandlePingOpcode(WorldPacket& recvPacket);
        void HandleRepopRequestOpcode(WorldPacket& recvPacket);
        void HandleAutostoreLootItemOpcode(WorldPacket& recvPacket);
        void HandleLootMoneyOpcode(WorldPacket& recvPacket);
        void HandleLootCurrencyOpcode(WorldPacket& recvPacket);
        void HandleLootOpcode(WorldPacket& recvPacket);
        void HandleLootReleaseOpcode(WorldPacket& recvPacket);
        void HandleLootMasterGiveOpcode(WorldPacket& recvPacket);
        void HandleWhoOpcode(WorldPackets::Who::WhoRequestPkt& whoRequest);
        void HandleLogoutRequestOpcode(WorldPacket& recvPacket);
        void HandlePlayerLogoutOpcode(WorldPacket& recvPacket);
        void HandleLogoutCancelOpcode(WorldPacket& recvPacket);

        // GM Ticket opcodes
        void HandleGMTicketCreateOpcode(WorldPacket& recvPacket);
        void HandleGMTicketUpdateOpcode(WorldPacket& recvPacket);
        void HandleGMTicketDeleteOpcode(WorldPacket& recvPacket);
        void HandleGMTicketGetTicketOpcode(WorldPacket& recvPacket);
        void HandleGMTicketSystemStatusOpcode(WorldPacket& recvPacket);
        void HandleGMSurveySubmit(WorldPacket& recvPacket);
        void HandleReportLag(WorldPacket& recvPacket);
        void HandleGMResponseResolve(WorldPacket& recvPacket);

        void HandleTogglePvP(WorldPacket& recvPacket);

        void HandleZoneUpdateOpcode(WorldPacket& recvPacket);
        void HandleSetSelectionOpcode(WorldPacket& recvPacket);
        void HandleStandStateChangeOpcode(WorldPacket& recvPacket);
        void HandleEmoteOpcode(WorldPacket& recvPacket);

        // Social
        void HandleContactListOpcode(WorldPacket& recvPacket);
        void HandleAddFriendOpcode(WorldPacket& recvPacket);
        void HandleDelFriendOpcode(WorldPacket& recvPacket);
        void HandleAddIgnoreOpcode(WorldPacket& recvPacket);
        void HandleDelIgnoreOpcode(WorldPacket& recvPacket);
        void HandleSetContactNotesOpcode(WorldPacket& recvPacket);
        void HandleBugOpcode(WorldPacket& recvPacket);

        void HandleAreaTriggerOpcode(WorldPacket& recvPacket);

        void HandleSetFactionAtWar(WorldPacket& recvData);
        void HandleSetFactionCheat(WorldPacket& recvData);
        void HandleSetWatchedFactionOpcode(WorldPacket& recvData);
        void HandleSetFactionInactiveOpcode(WorldPacket& recvData);

        void HandleUpdateAccountData(WorldPacket& recvPacket);
        void HandleRequestAccountData(WorldPacket& recvPacket);
        void HandleSetActionButtonOpcode(WorldPacket& recvPacket);

        void HandleGameObjectUseOpcode(WorldPacket& recPacket);
        void HandleMeetingStoneInfo(WorldPacket& recPacket);
        void HandleGameobjectReportUse(WorldPacket& recvPacket);

        void HandleNameQueryOpcode(WorldPacket& recvPacket);

        void HandleQueryTimeOpcode(WorldPacket& recvPacket);

        void HandleCreatureQueryOpcode(WorldPackets::Query::QueryCreature& query);

        void HandleGameObjectQueryOpcode(WorldPackets::Query::QueryGameObject& query);

        void HandleMoveWorldportAckOpcode(WorldPacket& recvPacket);
        void HandleMoveWorldportAck();                // for server-side calls
        void HandleSuspendTokenResponse(WorldPackets::Movement::SuspendTokenResponse& suspendTokenResponse);

        void HandleMovementOpcodes(WorldPacket& recvPacket);
        void HandleMovementOpcode(uint16 opcode, MovementInfo& movementInfo);
        void HandleSetActiveMoverOpcode(WorldPackets::Movement::SetActiveMover& packet);
        void HandleMoveNotActiveMover(WorldPacket& recvData);
        void HandleDismissControlledVehicle(WorldPacket& recvData);
        void HandleRequestVehicleExit(WorldPacket& recvData);
        void HandleChangeSeatsOnControlledVehicle(WorldPacket& recvData);
        void HandleMoveTimeSkippedOpcode(WorldPacket& recvData);

        void HandleRequestRaidInfoOpcode(WorldPacket& recvData);

        void HandleBattlefieldStatusOpcode(WorldPacket& recvData);
        void HandleBattleMasterHelloOpcode(WorldPacket& recvData);

        void HandlePartyInviteOpcode(WorldPackets::Party::PartyInviteClient& packet);
        //void HandleGroupCancelOpcode(WorldPacket& recvPacket);
        void HandlePartyInviteResponseOpcode(WorldPackets::Party::PartyInviteResponse& packet);
        void HandleGroupClearMarker(WorldPacket& recvPacket);
        void HandleGroupUninviteOpcode(WorldPacket& recvPacket);
        void HandleGroupUninviteGuidOpcode(WorldPacket& recvPacket);
        void HandleGroupSetLeaderOpcode(WorldPacket& recvPacket);
        void HandleSetRoleOpcode(WorldPackets::Party::SetRole& packet);
        void HandleGroupDisbandOpcode(WorldPacket& recvPacket);
        void HandleOptOutOfLootOpcode(WorldPacket& recvData);
        void HandleLootMethodOpcode(WorldPacket& recvPacket);
        void HandleLootRoll(WorldPacket& recvData);
        void HandleRequestPartyMemberStatsOpcode(WorldPacket& recvData);
        void HandleRaidTargetUpdateOpcode(WorldPacket& recvData);
        void HandleRaidReadyCheckOpcode(WorldPacket& recvData);
        void HandleRaidReadyCheckFinishedOpcode(WorldPacket& recvData);
        void HandleGroupRaidConvertOpcode(WorldPacket& recvData);
        void HandleGroupRequestJoinUpdates(WorldPacket& recvData);
        void HandleGroupChangeSubGroupOpcode(WorldPacket& recvData);
        void HandleGroupSwapSubGroupOpcode(WorldPacket& recvData);
        void HandleGroupAssistantLeaderOpcode(WorldPacket& recvData);
        void HandlePartyAssignmentOpcode(WorldPacket& recvData);
        void HandleRolePollBeginOpcode(WorldPacket& recvData);
        void HandleSetEveryoneIsAssistant(WorldPackets::Party::SetEveryoneIsAssistant& packet);

        void HandlePetitionBuyOpcode(WorldPacket& recvData);
        void HandlePetitionShowSignOpcode(WorldPacket& recvData);
        void SendPetitionSigns(Petition const* petition, Player* sendTo);
        void HandlePetitionQueryOpcode(WorldPacket& recvData);
        void HandlePetitionRenameOpcode(WorldPacket& recvData);
        void HandlePetitionSignOpcode(WorldPacket& recvData);
        void HandlePetitionDeclineOpcode(WorldPacket& recvData);
        void HandleOfferPetitionOpcode(WorldPacket& recvData);
        void HandleTurnInPetitionOpcode(WorldPacket& recvData);

        void HandleGuildQueryOpcode(WorldPacket& recvPacket);
        void HandleGuildInviteOpcode(WorldPacket& recvPacket);
        void HandleGuildRemoveOpcode(WorldPacket& recvPacket);
        void HandleGuildAcceptOpcode(WorldPacket& recvPacket);
        void HandleGuildDeclineOpcode(WorldPacket& recvPacket);
        void HandleGuildEventLogQueryOpcode(WorldPacket& recvPacket);
        void HandleGuildGetRosterOpcode(WorldPackets::Guild::GuildGetRoster& /*packet*/);
        void HandleRequestGuildRewardsList(WorldPackets::Guild::RequestGuildRewardsList& /*packet*/);
        void HandleGuildPromoteOpcode(WorldPacket& recvPacket);
        void HandleGuildDemoteOpcode(WorldPacket& recvPacket);
        void HandleGuildAssignRankOpcode(WorldPacket& recvPacket);
        void HandleGuildLeaveOpcode(WorldPacket& recvPacket);
        void HandleGuildDisbandOpcode(WorldPacket& recvPacket);
        void HandleGuildSetAchievementTracking(WorldPacket& recvPacket);
        void HandleGuildReplaceGuildMaster(WorldPackets::Guild::ReplaceGuildMaster& packet);
        void HandleGuildSetGuildMaster(WorldPackets::Guild::SetGuildMaster& packet);
        void HandleGuildMOTDOpcode(WorldPacket& recvPacket);
        void HandleGuildNewsUpdateStickyOpcode(WorldPacket& recvPacket);
        void HandleGuildSetNoteOpcode(WorldPacket& recvPacket);
        void HandleGuildQueryRanksOpcode(WorldPacket& recvPacket);
        void HandleGuildQueryNewsOpcode(WorldPacket& recvPacket);
        void HandleGuildQueryRecipesOpcode(WorldPackets::Guild::GuildQueryRecipes& packet);
        void HandleGuildQueryMembersForRecipe(WorldPackets::Guild::GuildQueryMembersForRecipe& packet);
        void HandleGuildQueryMemberRecipe(WorldPackets::Guild::GuildQueryMemberRecipes& packet);
        void HandleGuildSetRankPermissionsOpcode(WorldPacket& recvPacket);
        void HandleGuildAddRankOpcode(WorldPacket& recvPacket);
        void HandleGuildDelRankOpcode(WorldPacket& recvPacket);
        void HandleGuildChangeInfoTextOpcode(WorldPacket& recvPacket);
        void HandleSaveGuildEmblemOpcode(WorldPacket& recvPacket);
        void HandleGuildRequestPartyState(WorldPacket& recvPacket);
        void HandleGuildRequestMaxDailyXP(WorldPacket& recvPacket);
        void HandleGuildAchievementMembers(WorldPacket& recvPacket);
        void HandleGuildSwitchRank(WorldPacket& recvPacket);
        void HandleGuildRenameRequest(WorldPacket& recvPacket);
        void SendGuildCancelInvite(std::string unkString, uint8 unkByte);
        void HandleGuildRenameCallback(std::string newName, PreparedQueryResult result);
        void HandleAutoDeclineGuildInvites(WorldPacket& recvPacket);
        void HandleGuildSwitchRankOpcode(WorldPacket& recvPacket);
        void HandleGuildRequestChallengeUpdate(WorldPackets::Guild::GuildChallengeUpdateRequest& packet);
        void HandleGuildXPQueryOpcode(WorldPackets::Guild::GuildXPQuery& packet);

        // Guild Finder
        void HandleGuildFinderAddRecruit(WorldPacket& recvPacket);
        void HandleGuildFinderBrowse(WorldPacket& recvPacket);
        void HandleGuildFinderDeclineRecruit(WorldPacket& recvPacket);
        void HandleGuildFinderGetApplications(WorldPacket& recvPacket);
        void HandleGuildFinderGetRecruits(WorldPacket& recvPacket);
        void HandleGuildFinderPostRequest(WorldPacket& recvPacket);
        void HandleGuildFinderRemoveRecruit(WorldPacket& recvPacket);
        void HandleGuildFinderSetGuildPost(WorldPacket& recvPacket);

        void HandleTaxiNodeStatusQueryOpcode(WorldPacket& recvPacket);
        void HandleTaxiQueryAvailableNodes(WorldPacket& recvPacket);
        void HandleActivateTaxiOpcode(WorldPacket& recvPacket);
        void HandleActivateTaxiExpressOpcode(WorldPacket& recvPacket);
        void HandleMoveSplineDoneOpcode(WorldPacket& recvPacket);
        void SendActivateTaxiReply(ActivateTaxiReply reply);

        void HandleTabardVendorActivateOpcode(WorldPacket& recvPacket);
        void HandleBankerActivateOpcode(WorldPackets::NPC::Hello& packet);
        void HandleBuyBankSlotOpcode(WorldPacket& recvPacket);
        void HandleTrainerListOpcode(WorldPackets::NPC::Hello& packet);
        void HandleTrainerBuySpellOpcode(WorldPackets::NPC::TrainerBuySpell& packet);
        void HandlePetitionShowListOpcode(WorldPacket& recvPacket);
        void HandleGossipHelloOpcode(WorldPackets::NPC::Hello& packet);
        void HandleGossipSelectOptionOpcode(WorldPacket& recvPacket);
        void HandleSpiritHealerActivateOpcode(WorldPacket& recvPacket);
        void HandleNpcTextQueryOpcode(WorldPacket& recvPacket);
        void HandleBinderActivateOpcode(WorldPackets::NPC::Hello& packet);
        void HandleListStabledPetsOpcode(WorldPacket& recvPacket);
        void HandleSetPetSlot(WorldPacket& recvPacket);
        void HandleStableRevivePet(WorldPacket& recvPacket);

        void HandleDuelAcceptedOpcode(WorldPacket& recvPacket);
        void HandleDuelCancelledOpcode(WorldPacket& recvPacket);

        void HandleAcceptTradeOpcode(WorldPackets::Trade::AcceptTrade& packet);
        void HandleBeginTradeOpcode(WorldPackets::Trade::BeginTrade& packet);
        void HandleBusyTradeOpcode(WorldPackets::Trade::BusyTrade& packet);
        void HandleCancelTradeOpcode(WorldPackets::Trade::CancelTrade& packet);
        void HandleClearTradeItemOpcode(WorldPackets::Trade::ClearTradeItem& packet);
        void HandleIgnoreTradeOpcode(WorldPackets::Trade::IgnoreTrade& packet);
        void HandleInitiateTradeOpcode(WorldPackets::Trade::InitiateTrade& packet);
        void HandleSetTradeGoldOpcode(WorldPackets::Trade::SetTradeGold& packet);
        void HandleSetTradeItemOpcode(WorldPackets::Trade::SetTradeItem& packet);
        void HandleUnacceptTradeOpcode(WorldPackets::Trade::UnacceptTrade& packet);

        void HandleAuctionHelloOpcode(WorldPacket& recvPacket);
        void HandleAuctionListItems(WorldPacket& recvData);
        void HandleAuctionListBidderItems(WorldPacket& recvData);
        void HandleAuctionSellItem(WorldPacket& recvData);
        void HandleAuctionRemoveItem(WorldPacket& recvData);
        void HandleAuctionListOwnerItems(WorldPacket& recvData);
        void HandleAuctionPlaceBid(WorldPacket& recvData);
        void HandleAuctionListPendingSales(WorldPacket& recvData);

        void HandleGetMailList(WorldPacket& recvData);
        void HandleSendMail(WorldPacket& recvData);
        void HandleMailTakeMoney(WorldPacket& recvData);
        void HandleMailTakeItem(WorldPacket& recvData);
        void HandleMailMarkAsRead(WorldPacket& recvData);
        void HandleMailReturnToSender(WorldPacket& recvData);
        void HandleMailDelete(WorldPacket& recvData);
        void HandleItemTextQuery(WorldPacket& recvData);
        void HandleMailCreateTextItem(WorldPacket& recvData);
        void HandleQueryNextMailTime(WorldPacket& recvData);
        void HandleCancelChanneling(WorldPacket& recvData);

        void SendItemPageInfo(ItemTemplate* itemProto);
        void HandleSplitItemOpcode(WorldPacket& recvPacket);
        void HandleSwapInvItemOpcode(WorldPacket& recvPacket);
        void HandleDestroyItemOpcode(WorldPackets::Item::DestroyItem& packet);
        void HandleAutoEquipItemOpcode(WorldPacket& recvPacket);
        void HandleSellItemOpcode(WorldPacket& recvPacket);
        void HandleBuyItemInSlotOpcode(WorldPacket& recvPacket);
        void HandleBuyItemOpcode(WorldPacket& recvPacket);
        void HandleListInventoryOpcode(WorldPackets::NPC::Hello& packet);
        void HandleAutoStoreBagItemOpcode(WorldPacket& recvPacket);
        void HandleReadItem(WorldPacket& recvPacket);
        void HandleAutoEquipItemSlotOpcode(WorldPacket& recvPacket);
        void HandleSwapItem(WorldPacket& recvPacket);
        void HandleBuybackItem(WorldPacket& recvPacket);
        void HandleAutoBankItemOpcode(WorldPacket& recvPacket);
        void HandleAutoStoreBankItemOpcode(WorldPacket& recvPacket);
        void HandleWrapItemOpcode(WorldPacket& recvPacket);

        void HandleAttackSwingOpcode(WorldPackets::Combat::AttackSwing& packet);
        void HandleAttackStopOpcode(WorldPackets::Combat::CAttackStop& packet);
        void HandleSetSheathedOpcode(WorldPackets::Combat::SetSheathed& packet);

        void HandleUseItemOpcode(WorldPackets::Spells::UseItem& packet);
        void HandleOpenItemOpcode(WorldPacket& recvPacket);
        void HandleOpenWrappedItemCallback(uint16 pos, ObjectGuid itemGuid, PreparedQueryResult result);
        void HandleCastSpellOpcode(WorldPackets::Spells::CastSpell& castRequest);
        void HandleCancelCastOpcode(WorldPacket& recvPacket);
        void HandleCancelQueuedSpellOpcode(WorldPackets::Spells::CancelQueuedSpell& cancelQueuedSpell);
        void HandleCancelAuraOpcode(WorldPacket& recvPacket);
        void HandleCancelGrowthAuraOpcode(WorldPacket& recvPacket);
        void HandleCancelAutoRepeatSpellOpcode(WorldPacket& recvPacket);

        void HandleLearnTalentOpcode(WorldPacket& recvPacket);
        void HandleLearnPreviewTalents(WorldPacket& recvPacket);
        void HandleTalentWipeConfirmOpcode(WorldPacket& recvPacket);
        void HandleUnlearnSkillOpcode(WorldPacket& recvPacket);

        void HandleQuestgiverStatusQueryOpcode(WorldPackets::Quest::QuestGiverStatusQuery& packet);
        void HandleQuestgiverStatusMultipleQuery(WorldPackets::Quest::QuestGiverStatusMultipleQuery& packet);
        void HandleQuestgiverHelloOpcode(WorldPackets::Quest::QuestGiverHello& packet);
        void HandleQuestgiverAcceptQuestOpcode(WorldPackets::Quest::QuestGiverAcceptQuest& packet);
        void HandleQuestgiverQueryQuestOpcode(WorldPackets::Quest::QuestGiverQueryQuest& packet);
        void HandleQuestgiverChooseRewardOpcode(WorldPackets::Quest::QuestGiverChooseReward& packet);
        void HandleQuestgiverRequestRewardOpcode(WorldPackets::Quest::QuestGiverRequestReward& packet);
        void HandleQuestQueryOpcode(WorldPackets::Quest::QueryQuestInfo& packet);
        void HandleQuestgiverCancel(WorldPacket& recvData);
        void HandleQuestLogSwapQuest(WorldPacket& recvData);
        void HandleQuestLogRemoveQuest(WorldPackets::Quest::QuestLogRemoveQuest& packet);
        void HandleQuestConfirmAccept(WorldPackets::Quest::QuestConfirmAccept& packet);
        void HandleQuestgiverCompleteQuest(WorldPackets::Quest::QuestGiverCompleteQuest& packet);
        void HandleQuestgiverQuestAutoLaunch(WorldPacket& recvPacket);
        void HandlePushQuestToParty(WorldPacket& recvPacket);
        void HandleQuestPushResult(WorldPacket& recvPacket);

        void HandleMessagechatOpcode(WorldPacket& recvPacket);
        void HandleAddonMessagechatOpcode(WorldPacket& recvPacket);
        void SendPlayerNotFoundNotice(std::string const& name);
        void SendPlayerAmbiguousNotice(std::string const& name);
        void SendWrongFactionNotice();
        void SendChatRestrictedNotice(ChatRestrictionType restriction);
        void HandleSendTextEmoteOpcode(WorldPackets::Chat::SendTextEmote& packet);
        void HandleChatIgnoredOpcode(WorldPacket& recvPacket);

        void HandleUnregisterAddonPrefixesOpcode(WorldPacket& recvPacket);
        void HandleAddonRegisteredPrefixesOpcode(WorldPacket& recvPacket);

        void HandleReclaimCorpseOpcode(WorldPacket& recvPacket);
        void HandleCorpseQueryOpcode(WorldPacket& recvPacket);
        void HandleCorpseMapPositionQuery(WorldPacket& recvPacket);
        void HandleResurrectResponseOpcode(WorldPacket& recvPacket);
        void HandleSummonResponseOpcode(WorldPacket& recvData);

        void HandleJoinChannel(WorldPacket& recvPacket);
        void HandleLeaveChannel(WorldPacket& recvPacket);
        void HandleChannelList(WorldPacket& recvPacket);
        void HandleChannelPassword(WorldPacket& recvPacket);
        void HandleChannelSetOwner(WorldPacket& recvPacket);
        void HandleChannelOwner(WorldPacket& recvPacket);
        void HandleChannelModerator(WorldPacket& recvPacket);
        void HandleChannelUnmoderator(WorldPacket& recvPacket);
        void HandleChannelMute(WorldPacket& recvPacket);
        void HandleChannelUnmute(WorldPacket& recvPacket);
        void HandleChannelInvite(WorldPacket& recvPacket);
        void HandleChannelKick(WorldPacket& recvPacket);
        void HandleChannelBan(WorldPacket& recvPacket);
        void HandleChannelUnban(WorldPacket& recvPacket);
        void HandleChannelAnnouncements(WorldPacket& recvPacket);
        void HandleChannelModerate(WorldPacket& recvPacket);
        void HandleChannelDeclineInvite(WorldPacket& recvPacket);
        void HandleChannelDisplayListQuery(WorldPacket& recvPacket);
        void HandleGetChannelMemberCount(WorldPacket& recvPacket);
        void HandleSetChannelWatch(WorldPacket& recvPacket);
        void HandleUnsetChannelWatch(WorldPacket& recvPacket);

        void HandleCompleteCinematic(WorldPackets::Misc::CompleteCinematic& packet);
        void HandleNextCinematicCamera(WorldPackets::Misc::NextCinematicCamera& packet);
        void HandleCompleteMovie(WorldPackets::Misc::CompleteMovie& packet);

        void HandlePageTextQueryOpcode(WorldPacket& recvPacket);

        void HandleTutorialFlag (WorldPacket& recvData);
        void HandleTutorialClear(WorldPacket& recvData);
        void HandleTutorialReset(WorldPacket& recvData);

        //Pet
        void HandlePetAction(WorldPacket& recvData);
        void HandlePetStopAttack(WorldPacket& recvData);
        void HandlePetActionHelper(Unit* pet, ObjectGuid guid1, uint32 spellid, uint16 flag, ObjectGuid guid2, float x, float y, float z);
        void HandlePetNameQuery(WorldPacket& recvData);
        void HandlePetSetAction(WorldPacket& recvData);
        void HandlePetAbandon(WorldPacket& recvData);
        void HandlePetRename(WorldPacket& recvData);
        void HandlePetCancelAuraOpcode(WorldPacket& recvPacket);
        void HandlePetSpellAutocastOpcode(WorldPacket& recvPacket);
        void HandlePetCastSpellOpcode(WorldPacket& recvPacket);
        void HandlePetLearnTalent(WorldPacket& recvPacket);
        void HandleLearnPreviewTalentsPet(WorldPacket& recvPacket);

        void HandleSetActionBarToggles(WorldPacket& recvData);

        void HandleTotemDestroyed(WorldPackets::Totem::TotemDestroyed& packet);
        void HandleDismissCritter(WorldPacket& recvData);

        //Battleground
        void HandleBattlemasterHelloOpcode(WorldPacket& recvData);
        void HandleBfQueueRequest(WorldPacket & recvData);
        void HandleBattlemasterJoinOpcode(WorldPacket& recvData);
        void HandleBattlegroundPlayerPositionsOpcode(WorldPacket& recvData);
        void HandlePVPLogDataOpcode(WorldPacket& recvData);
        void HandleBattleFieldPortOpcode(WorldPacket& recvData);
        void HandleBattlefieldListOpcode(WorldPacket& recvData);
        void HandleBattlefieldLeaveOpcode(WorldPacket& recvData);
        void HandleBattlegroundStateQuery(WorldPacket& recvData);
        void HandleBattlemasterJoinArena(WorldPacket& recvData);
        void HandleReportPvPAFK(WorldPacket& recvData);
        void HandleRequestRatedBgInfo(WorldPacket& recvData);
        void HandleRequestPvpOptions(WorldPacket& recvData);
        void HandleRequestPvpReward(WorldPacket& recvData);
        void HandleRequestRatedBgStats(WorldPacket& recvData);

        // Battlefield
        void SendBfInvitePlayerToWar(ObjectGuid guid, uint32 zoneId, uint32 time);
        void SendBfInvitePlayerToQueue(ObjectGuid guid);
        void SendBfQueueInviteResponse(ObjectGuid guid, uint32 zoneId, bool canQueue = true, bool full = false);
        void SendBfEntered(ObjectGuid guid);
        void SendBfLeaveMessage(ObjectGuid guid, BFLeaveReason reason = BF_LEAVE_REASON_EXITED);
        void HandleBfQueueInviteResponse(WorldPacket& recvData);
        void HandleBfEntryInviteResponse(WorldPacket& recvData);
        void HandleBfExitRequest(WorldPacket& recvData);

        void HandleWardenDataOpcode(WorldPacket& recvData);
        void HandleWorldTeleportOpcode(WorldPacket& recvData);
        void HandleMinimapPingOpcode(WorldPacket& recvData);
        void HandleRandomRollOpcode(WorldPacket& recvData);
        void HandleFarSightOpcode(WorldPacket& recvData);
        void HandleSetDungeonDifficultyOpcode(WorldPacket& recvData);
        void HandleSetRaidDifficultyOpcode(WorldPacket& recvData);
        void HandleChangePlayerDifficulty(WorldPacket& recvData);
        void HandleSetTitleOpcode(WorldPacket& recvData);
        void HandleRealmSplitOpcode(WorldPacket& recvData);
        void HandleTimeSyncResp(WorldPacket& recvData);
        void HandleWhoisOpcode(WorldPacket& recvData);
        void HandleResetInstancesOpcode(WorldPacket& recvData);
        void HandleHearthAndResurrect(WorldPacket& recvData);
        void HandleInstanceLockResponse(WorldPacket& recvPacket);

        // Cemetery
        void HandleSetPreferedCemetery(WorldPacket& recvData);
        void HandleCemeteryListRequest(WorldPacket& recvData);

        // Currency
        void HandleSetCurrencyFlags(WorldPackets::Misc::SetCurrencyFlags& packet);

        // Looking for Dungeon/Raid
        void HandleLfgJoinOpcode(WorldPackets::LFG::LFGJoin& lfgJoin);
        void HandleLfgLeaveOpcode(WorldPackets::LFG::LFGLeave& lfgLeave);
        void HandleLfgProposalResultOpcode(WorldPackets::LFG::LFGProposalResponse& lfgProposalResponse);
        void HandleLfgSetRolesOpcode(WorldPackets::LFG::LFGSetRoles& lfgSetRoles);
        void HandleLfgSetCommentOpcode(WorldPackets::LFG::LFGSetComment& lfgSetComment);
        void HandleLfgSetBootVoteOpcode(WorldPackets::LFG::LFGBootPlayerVote& lfgBootPlayerVote);
        void HandleLfgTeleportOpcode(WorldPackets::LFG::LFGTeleport& lfgTeleport);
        void HandleLfgGetLockInfoOpcode(WorldPackets::LFG::LFGGetSystemInfo& lfgGetSystemInfo);
        void SendLfgPlayerLockInfo();
        void SendLfgPartyLockInfo();
        void HandleLfrJoinOpcode(WorldPacket& recvData);
        void HandleLfrLeaveOpcode(WorldPacket& recvData);
        void HandleLfgGetStatus(WorldPacket& recvData);

        void SendLfgUpdateStatus(lfg::LfgUpdateData const& updateData, bool party);
        void SendLfgRoleChosen(ObjectGuid guid, uint8 roles);
        void SendLfgRoleCheckUpdate(lfg::LfgRoleCheck const& pRoleCheck);
        void SendLfgLfrList(bool update);
        void SendLfgJoinResult(lfg::LfgJoinResultData const& joinData);
        void SendLfgQueueStatus(lfg::LfgQueueStatusData const& queueData);
        void SendLfgPlayerReward(lfg::LfgPlayerRewardData const& lfgPlayerRewardData);
        void SendLfgBootProposalUpdate(lfg::LfgPlayerBoot const& boot);
        void SendLfgUpdateProposal(lfg::LfgProposal const& proposal);
        void SendLfgDisabled();
        void SendLfgOfferContinue(uint32 dungeonEntry);
        void SendLfgTeleportError(lfg::LfgTeleportResult err);

        // Arena Team
        void HandleInspectArenaTeamsOpcode(WorldPacket& recvData);
        void HandleArenaTeamQueryOpcode(WorldPacket& recvData);
        void HandleArenaTeamRosterOpcode(WorldPacket& recvData);
        void HandleArenaTeamCreateOpcode(WorldPacket& recvData);
        void HandleArenaTeamInviteOpcode(WorldPacket& recvData);
        void HandleArenaTeamAcceptOpcode(WorldPacket& recvData);
        void HandleArenaTeamDeclineOpcode(WorldPacket& recvData);
        void HandleArenaTeamLeaveOpcode(WorldPacket& recvData);
        void HandleArenaTeamRemoveOpcode(WorldPacket& recvData);
        void HandleArenaTeamDisbandOpcode(WorldPacket& recvData);
        void HandleArenaTeamLeaderOpcode(WorldPacket& recvData);

        void HandleAreaSpiritHealerQueryOpcode(WorldPacket& recvData);
        void HandleAreaSpiritHealerQueueOpcode(WorldPacket& recvData);
        void HandleCancelMountAuraOpcode(WorldPacket& recvData);
        void HandleSelfResOpcode(WorldPacket& recvData);
        void HandleComplaintOpcode(WorldPackets::Ticket::Complaint& packet);
        void HandleSupportTicketSubmitSuggestionOpcode(WorldPackets::Ticket::SupportTicketSubmitSuggestion& packet);
        void HandleSupportTicketSubmitBugOpcode(WorldPackets::Ticket::SupportTicketSubmitBug& packet);
        void HandleSupportTicketSubmitComplaintOpcode(WorldPackets::Ticket::SupportTicketSubmitComplaint& packet);
        void HandleRequestPetInfoOpcode(WorldPacket& recvData);

        // Socket gem
        void HandleSocketOpcode(WorldPacket& recvData);

        void HandleCancelTempEnchantmentOpcode(WorldPacket& recvData);

        void HandleItemRefundInfoRequest(WorldPacket& recvData);
        void HandleItemRefund(WorldPacket& recvData);

        void HandleChannelVoiceOnOpcode(WorldPacket& recvData);
        void HandleVoiceSessionEnableOpcode(WorldPacket& recvData);
        void HandleSetActiveVoiceChannel(WorldPacket& recvData);
        void HandleSetTaxiBenchmarkOpcode(WorldPacket& recvData);

        // Guild Bank
        void HandleGuildPermissionsQuery(WorldPackets::Guild::GuildPermissionsQuery& packet);
        void HandleGuildBankMoneyWithdrawn(WorldPackets::Guild::GuildBankRemainingWithdrawMoneyQuery& packet);
        void HandleGuildBankActivate(WorldPackets::Guild::GuildBankActivate& packet);
        void HandleGuildBankQueryTab(WorldPackets::Guild::GuildBankQueryTab& packet);
        void HandleGuildBankLogQuery(WorldPackets::Guild::GuildBankLogQuery& packet);
        void HandleGuildBankDepositMoney(WorldPackets::Guild::GuildBankDepositMoney& packet);
        void HandleGuildBankWithdrawMoney(WorldPackets::Guild::GuildBankWithdrawMoney& packet);
        void HandleGuildBankSwapItems(WorldPackets::Guild::GuildBankSwapItems& packet);
        void HandleGuildBankUpdateTab(WorldPackets::Guild::GuildBankUpdateTab& packet);
        void HandleGuildBankBuyTab(WorldPackets::Guild::GuildBankBuyTab& packet);
        void HandleGuildBankTextQuery(WorldPackets::Guild::GuildBankTextQuery& packet);
        void HandleGuildBankSetTabText(WorldPackets::Guild::GuildBankSetTabText& packet);

        // Refer-a-Friend
        void HandleGrantLevel(WorldPacket& recvData);
        void HandleAcceptGrantLevel(WorldPacket& recvData);

        // Calendar
        void HandleCalendarGetCalendar(WorldPacket& recvData);
        void HandleCalendarGetEvent(WorldPacket& recvData);
        void HandleCalendarGuildFilter(WorldPacket& recvData);
        void HandleCalendarArenaTeam(WorldPacket& recvData);
        void HandleCalendarAddEvent(WorldPacket& recvData);
        void HandleCalendarUpdateEvent(WorldPacket& recvData);
        void HandleCalendarRemoveEvent(WorldPacket& recvData);
        void HandleCalendarCopyEvent(WorldPacket& recvData);
        void HandleCalendarEventInvite(WorldPacket& recvData);
        void HandleCalendarEventRsvp(WorldPacket& recvData);
        void HandleCalendarEventRemoveInvite(WorldPacket& recvData);
        void HandleCalendarEventStatus(WorldPacket& recvData);
        void HandleCalendarEventModeratorStatus(WorldPacket& recvData);
        void HandleCalendarComplain(WorldPacket& recvData);
        void HandleCalendarGetNumPending(WorldPacket& recvData);
        void HandleCalendarEventSignup(WorldPacket& recvData);

        void SendCalendarRaidLockout(InstanceSave const* save, bool add);
        void SendCalendarRaidLockoutUpdated(InstanceSave const* save);
        void HandleSetSavedInstanceExtend(WorldPacket& recvData);

        // Void Storage
        void HandleVoidStorageUnlock(WorldPacket& recvData);
        void HandleVoidStorageQuery(WorldPacket& recvData);
        void HandleVoidStorageTransfer(WorldPacket& recvData);
        void HandleVoidSwapItem(WorldPacket& recvData);
        void SendVoidStorageTransferResult(VoidTransferError result);

        // Transmogrification
        void HandleTransmogrifyItems(WorldPackets::Item::TransmogrifyItems& packet);

        // Reforge
        void HandleReforgeItemOpcode(WorldPackets::Reforge::ReforgeItem& packet);
        void SendReforgeResult(bool success);

        // Miscellaneous
        void HandleSpellClick(WorldPacket& recvData);
        void HandleMirrorImageDataRequest(WorldPacket& recvData);
        void HandleRemoveGlyph(WorldPacket& recvData);
        void HandleQueryInspectAchievements(WorldPacket& recvData);
        void HandleGuildAchievementProgressQuery(WorldPacket& recvData);
        void HandleEquipmentSetSave(WorldPacket& recvData);
        void HandleEquipmentSetDelete(WorldPacket& recvData);
        void HandleEquipmentSetUse(WorldPacket& recvData);
        void HandleWorldStateUITimerUpdate(WorldPacket& recvData);
        void HandleReadyForAccountDataTimes(WorldPacket& recvData);
        void HandleQueryQuestsCompleted(WorldPacket& recvData);
        void HandleQuestNPCQuery(WorldPacket& recvData);
        void HandleQuestPOIQuery(WorldPackets::Query::QuestPOIQuery& query);
        void HandleEjectPassenger(WorldPacket& data);
        void HandleEnterPlayerVehicle(WorldPacket& data);
        void HandleUpdateProjectilePosition(WorldPacket& recvPacket);
        void HandleUpdateMissileTrajectory(WorldPackets::Spells::UpdateMissileTrajectory& updateMissileTrajectory);
        void HandleViolenceLevel(WorldPacket& recvPacket);
        void HandleObjectUpdateFailedOpcode(WorldPacket& recvPacket);
        void HandleRequestCategoryCooldowns(WorldPacket& recvPacket);
        void SendStreamingMovie();
        void HandleRequestResearchHistory(WorldPackets::Archaeology::RequestResearchHistory& packet);
        int32 HandleEnableNagleAlgorithm();

        // Query
        void HandleDBQueryBulk(WorldPackets::Query::DBQueryBulk& packet);

        union ConnectToKey
        {
            struct
            {
                uint64 AccountId : 32;
                uint64 ConnectionType : 1;
                uint64 Key : 31;
            } Fields;

            uint64 Raw;
        };

        uint64 GetConnectToInstanceKey() const { return _instanceConnectKey.Raw; }

    public:
        QueryCallbackProcessor& GetQueryProcessor() { return _queryProcessor; }
        TransactionCallback& AddTransactionCallback(TransactionCallback&& callback);
        SQLQueryHolderCallback& AddQueryHolderCallback(SQLQueryHolderCallback&& callback);

        // Compact Unit Frames (4.x)
        void HandleSaveCUFProfiles(WorldPacket& recvPacket);
        void SendLoadCUFProfiles();

    private:
        void ProcessQueryCallbacks();

        QueryCallbackProcessor _queryProcessor;
        AsyncCallbackProcessor<TransactionCallback> _transactionCallbacks;
        AsyncCallbackProcessor<SQLQueryHolderCallback> _queryHolderProcessor;

    friend class World;
    protected:
        class DosProtection
        {
            friend class World;
            public:
                DosProtection(WorldSession* s);
                bool EvaluateOpcode(WorldPacket& p, time_t time) const;
            protected:
                enum Policy
                {
                    POLICY_LOG,
                    POLICY_KICK,
                    POLICY_BAN,
                };

                uint32 GetMaxPacketCounterAllowed(uint16 opcode) const;

                WorldSession* Session;

            private:
                Policy _policy;
                typedef std::unordered_map<uint16, PacketCounter> PacketThrottlingMap;
                // mark this member as "mutable" so it can be modified even in const functions
                mutable PacketThrottlingMap _PacketThrottlingMap;

                DosProtection(DosProtection const& right) = delete;
                DosProtection& operator=(DosProtection const& right) = delete;
        } AntiDOS;

    private:
        // private trade methods
        void moveItems(Item* myItems[], Item* hisItems[]);

        bool CanUseBank(ObjectGuid bankerGUID = ObjectGuid::Empty) const;

        // logging helper
        void LogUnexpectedOpcode(WorldPacket* packet, char const* status, const char *reason);

        // EnumData helpers
        bool IsLegitCharacterForAccount(ObjectGuid lowGUID)
        {
            return _legitCharacters.find(lowGUID) != _legitCharacters.end();
        }

        // Movement helpers
        bool IsRightUnitBeingMoved(ObjectGuid guid);

        // this stores the GUIDs of the characters who can login
        // characters who failed on Player::BuildEnumData shouldn't login
        GuidSet _legitCharacters;

        ObjectGuid::LowType m_GUIDLow;                      // set logined or recently logout player (while m_playerRecentlyLogout set)
        Player* _player;
        std::shared_ptr<WorldSocket> m_Socket[2];
        std::string m_Address;                              // Current Remote Address
     // std::string m_LAddress;                             // Last Attempted Remote Adress - we can not set attempted ip for a non-existing session!

        AccountTypes _security;
        uint32 _accountId;
        std::string _accountName;
        uint32 _battlenetAccountId;
        uint8 m_accountExpansion;
        uint8 m_expansion;

        // Warden
        Warden* _warden;                                    // Remains nullptr if Warden system is not enabled by config

        time_t _logoutTime;
        bool m_inQueue;                                     // session wait in auth.queue
        ObjectGuid m_playerLoading;                         // code processed in LoginPlayer
        bool m_playerLogout;                                // code processed in LogoutPlayer
        bool m_playerRecentlyLogout;
        bool m_playerSave;
        LocaleConstant m_sessionDbcLocale;
        LocaleConstant m_sessionDbLocaleIndex;
        std::atomic<uint32> m_latency;
        AccountData m_accountData[NUM_ACCOUNT_DATA_TYPES];
        uint32 m_Tutorials[MAX_ACCOUNT_TUTORIAL_VALUES];
        uint8  m_TutorialsChanged;
        struct Addons
        {
            struct SecureAddonInfo
            {
                enum SecureAddonStatus : uint8
                {
                    BANNED          = 0,
                    SECURE_VISIBLE  = 1,
                    SECURE_HIDDEN   = 2
                };

                std::string Name;
                SecureAddonStatus Status = BANNED;
                bool HasKey = false;
            };

            static uint32 constexpr MaxSecureAddons = 35;

            std::vector<SecureAddonInfo> SecureAddons;
            uint32 LastBannedAddOnTimestamp = 0;
        } _addons;
        std::vector<std::string> _registeredAddonPrefixes;
        bool _filterAddonMessages;
        uint32 recruiterId;
        bool isRecruiter;
        LockedQueue<WorldPacket*> _recvQueue;
        rbac::RBACData* _RBACData;
        uint32 expireTime;
        bool forceExit;
        ObjectGuid m_currentBankerGUID;

        std::unique_ptr<boost::circular_buffer<std::pair<int64, uint32>>> _timeSyncClockDeltaQueue; // first member: clockDelta. Second member: latency of the packet exchange that was used to compute that clockDelta.
        int64 _timeSyncClockDelta;
        void ComputeNewClockDelta();

        std::map<uint32, uint32> _pendingTimeSyncRequests; // key: counter. value: server time when packet with that counter was sent.
        uint32 _timeSyncNextCounter;
        uint32 _timeSyncTimer;

        ConnectToKey _instanceConnectKey;

        GameClient* _gameClient;

        WorldSession(WorldSession const& right) = delete;
        WorldSession& operator=(WorldSession const& right) = delete;
};
#endif
/// @}
