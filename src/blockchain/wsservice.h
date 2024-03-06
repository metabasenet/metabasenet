// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_WSSERVICE_H
#define METABASENET_WSSERVICE_H

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "base.h"
#include "event.h"
#include "mtbase.h"

namespace metabasenet
{

/////////////////////////////

typedef websocketpp::server<websocketpp::config::asio> WSSERVER;

using websocketpp::connection_hdl;
using websocketpp::frame::opcode::value;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef WSSERVER::message_ptr message_ptr;
typedef WSSERVER::connection_ptr connection_ptr;

/////////////////////////////
// CClientSubscribe

class CClientSubscribe
{
public:
    CClientSubscribe()
      : nClientConnId(0), nSubsType(0) {}
    CClientSubscribe(const uint64 nClientConnIdIn, const uint8 nSubsTypeIn, const std::set<CDestination>& setSubsAddressIn, const std::set<uint256>& setSubsTopicsIn)
      : nClientConnId(nClientConnIdIn), nSubsType(nSubsTypeIn), setSubsAddress(setSubsAddressIn), setSubsTopics(setSubsTopicsIn) {}

    void matchesLogs(CTransactionReceipt const& _m, MatchLogsVec& vLogs) const;

public:
    uint64 nClientConnId;
    uint8 nSubsType;
    std::set<CDestination> setSubsAddress;
    std::set<uint256> setSubsTopics;
};

/////////////////////////////
// CWsSubscribeFork

class CWsSubscribeFork
{
public:
    CWsSubscribeFork()
      : nSubsIdSeed(0) {}

    uint128 AddSubscribe(const uint64 nClientConnId, const uint8 nSubsType, const std::set<CDestination>& setSubsAddress, const std::set<uint256>& setSubsTopics);

    void RemoveClientAllSubscribe(const uint64 nClientConnId);
    void RemoveSubscribe(const uint128& nSubsId);

    const std::map<uint128, CClientSubscribe>& GetSubsListByType(const uint8 nSubsType);

protected:
    uint128 CreateSubsId(const uint8 nSubsType, const uint64 nConnId);
    uint8 GetSubsIdType(const uint128& nSubsId) const;

protected:
    std::map<uint8, std::map<uint128, CClientSubscribe>> mapClientSubscribe; // key1: subscribe type, key2: subscribe id
    std::map<uint64, std::map<uint8, uint128>> mapClientConnect;             // key1: connect id, key2: subscribe type, value: subscribe id

    uint64 nSubsIdSeed;
};

/////////////////////////////
// CWsClient

class CWsClient
{
public:
    CWsClient(const uint64 nConnIdIn, const std::string& strClientIpIn, const uint16 nClientPortIn, const connection_hdl hdlIn)
      : nConnId(nConnIdIn), strClientIp(strClientIpIn), nClientPort(nClientPortIn), hdl(hdlIn) {}

    uint64 GetConnId() const;
    const std::string& GetClientIp() const;
    uint16 GetClientPort() const;
    connection_hdl GetConnHdl() const;

protected:
    const uint64 nConnId;
    const std::string strClientIp;
    const uint16 nClientPort;

    connection_hdl hdl;
};

/////////////////////////////
// CWsServer

class CWsService;

class CWsServer
{
public:
    CWsServer(const CChainId nChainIdIn, const uint16 nListenPortIn, const uint32 nMaxConnectionsIn, const boost::asio::ip::address& addrListenIn, mtbase::IIOModule* pRpcModIn, CWsService* pWssIn);
    ~CWsServer();

    bool Start();
    void Stop();

    void SendWsMsg(const uint64 nConnId, const std::string& strMsg);

protected:
    void WsThreadFunc();

    bool StartWsListen();
    void StopWsListen();

    void ParseIpPortString(const std::string& strIpPort, std::string& strIp, uint16& nPort);

    void OnOpen(connection_hdl hdl);
    void OnClose(connection_hdl hdl);
    void OnMessage(connection_hdl hdl, message_ptr msg);

    bool PostRpcModMsg(const CWsClient& wsClient, const std::string& strMsg);

protected:
    const CChainId nChainId;
    const uint16 nListenPort;
    const uint32 nMaxConnections;
    const boost::asio::ip::address addrListen;

    mtbase::IIOModule* const pRpcMod;
    CWsService* const pWsService;

    boost::thread* pThreadWs;
    boost::mutex mutex;

    WSSERVER wsServer;

    mtbase::CRWAccess rwAccess;
    std::map<uint64, CWsClient> mapWsClient; // key: client connect id
};
typedef std::shared_ptr<CWsServer> SHP_WS_SERVER;
#define MAKE_SHARED_WS_SERVER std::make_shared<CWsServer>

/////////////////////////////
// CWsService

class CWsService : public IWsService, virtual public CWsServiceEventListener
{
public:
    CWsService();
    ~CWsService();

    bool HandleEvent(CEventWsServicePushNewBlock& eventPush) override;
    bool HandleEvent(CEventWsServicePushLogs& eventPush) override;
    bool HandleEvent(CEventWsServicePushNewPendingTx& eventPush) override;
    bool HandleEvent(CEventWsServicePushSyncing& eventPush) override;

    void AddNewBlockSubscribe(const CChainId nChainId, const uint64 nClientConnId, uint128& nSubsId) override;
    void AddLogsSubscribe(const CChainId nChainId, const uint64 nClientConnId, const std::set<CDestination>& setSubsAddress, const std::set<uint256>& setSubsTopics, uint128& nSubsId) override;
    void AddNewPendingTxSubscribe(const CChainId nChainId, const uint64 nClientConnId, uint128& nSubsId) override;
    void AddSyncingSubscribe(const CChainId nChainId, const uint64 nClientConnId, uint128& nSubsId) override;
    bool RemoveSubscribe(const CChainId nChainId, const uint64 nClientConnId, const uint128& nSubsId) override;

    void SendWsMsg(const CChainId nChainId, const uint64 nNonce, const std::string& strMsg) override;

    void RemoveClientAllSubscribe(const CChainId nChainId, const uint64 nClientConnId);

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    const CRPCServerConfig* RPCServerConfig()
    {
        return dynamic_cast<const CRPCServerConfig*>(IBase::Config());
    }

protected:
    mtbase::IIOModule* pRpcModIOModule;

    mtbase::CRWAccess rwAccess;
    std::map<CChainId, SHP_WS_SERVER> mapWsServer;
    std::map<CChainId, CWsSubscribeFork> mapWsSubscribeFork;
};

} // namespace metabasenet

#endif // METABASENET_WSSERVICE_H
