/*
* Copyright (C) 2008-2018 TrinityCore <https://www.trinitycore.org/>
* Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef __AUTHSESSION_H__
#define __AUTHSESSION_H__

#include "AsyncCallbackProcessor.h"
#include "DatabaseEnvFwd.h"
#include "Common.h"
#include "BigNumber.h"
#include "Socket.h"
#include <boost/asio/ip/tcp.hpp>
#include <memory>

using boost::asio::ip::tcp;

class ByteBuffer;
class Field;
struct AuthHandler;

enum AuthStatus
{
    STATUS_CHALLENGE = 0,
    STATUS_LOGON_PROOF,
    STATUS_RECONNECT_PROOF,
    STATUS_AUTHED,
    STATUS_WAITING_FOR_REALM_LIST,
    STATUS_CLOSED
};

struct AccountInfo
{
    void LoadResult(Field* fields);

    uint32 Id = 0;
    std::string Login;
    bool IsLockedToIP = false;
    std::string LockCountry;
    std::string LastIP;
    uint32 FailedLogins = 0;
    bool IsBanned = false;
    bool IsPermanenetlyBanned = false;
    AccountTypes SecurityLevel = SEC_PLAYER;
    std::string TokenKey;
};

class AuthSession : public Socket<AuthSession>
{
    typedef Socket<AuthSession> AuthSocket;

public:
    static std::unordered_map<uint8, AuthHandler> InitHandlers();

    AuthSession(tcp::socket&& socket);

    void Start() override;
    bool Update() override;

    void SendPacket(ByteBuffer& packet);

protected:
    void ReadHandler() override;

private:
    bool HandleLogonChallenge();
    bool HandleLogonProof();
    bool HandleReconnectChallenge();
    bool HandleReconnectProof();
    bool HandleRealmList();

    void CheckIpCallback(PreparedQueryResult result);
    void LogonChallengeCallback(PreparedQueryResult result);
    void ReconnectChallengeCallback(PreparedQueryResult result);
    void RealmListCallback(PreparedQueryResult result);

    void SetVSFields(const std::string& rI);

    BigNumber N, s, g, v;
    BigNumber b, B;
    BigNumber K;
    BigNumber _reconnectProof;

    AuthStatus _status;
    AccountInfo _accountInfo;
    std::string _tokenKey;
    std::string _localizationName;
    std::string _os;
    std::string _ipCountry;
    uint16 _build;
    uint8 _expversion;

    QueryCallbackProcessor _queryProcessor;
};

#pragma pack(push, 1)

struct AuthHandler
{
    AuthStatus status;
    size_t packetSize;
    bool (AuthSession::*handler)();
};

#pragma pack(pop)

#endif
