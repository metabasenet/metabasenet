// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wsservice.h"

#include "version.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet::crypto;

namespace metabasenet
{

/////////////////////////////
// CClientSubscribe

void CClientSubscribe::matchesLogs(CTransactionReceipt const& _m, MatchLogsVec& vLogs) const
{
    for (unsigned i = 0; i < _m.vLogs.size(); ++i)
    {
        auto& logs = _m.vLogs[i];
        if (!setSubsAddress.empty() && setSubsAddress.count(logs.address) == 0)
        {
            continue;
        }
        if (!setSubsTopics.empty())
        {
            bool m = false;
            for (auto& h : logs.topics)
            {
                if (setSubsTopics.count(h) > 0)
                {
                    m = true;
                    break;
                }
            }
            if (!m)
            {
                continue;
            }
        }
        vLogs.push_back(CMatchLogs(i, logs));
    }
}

/////////////////////////////
// CWsSubscribeFork

uint128 CWsSubscribeFork::AddSubscribe(const uint64 nClientConnId, const uint8 nSubsType, const std::set<CDestination>& setSubsAddress, const std::set<uint256>& setSubsTopics)
{
    auto& mapTypeSubs = mapClientSubscribe[nSubsType];

    auto it = mapClientConnect.find(nClientConnId);
    if (it != mapClientConnect.end())
    {
        auto mt = it->second.find(nSubsType);
        if (mt != it->second.end())
        {
            const uint128& nSubsId = mt->second;
            mapTypeSubs[nSubsId] = CClientSubscribe(nClientConnId, nSubsType, setSubsAddress, setSubsTopics);
            return nSubsId;
        }
    }

    uint128 nSubsId;
    do
    {
        nSubsId = CreateSubsId(nSubsType, nClientConnId);
    } while (mapTypeSubs.find(nSubsId) != mapTypeSubs.end());

    mapTypeSubs.insert(std::make_pair(nSubsId, CClientSubscribe(nClientConnId, nSubsType, setSubsAddress, setSubsTopics)));
    mapClientConnect[nClientConnId][nSubsType] = nSubsId;
    return nSubsId;
}

void CWsSubscribeFork::RemoveClientAllSubscribe(const uint64 nClientConnId)
{
    auto it = mapClientConnect.find(nClientConnId);
    if (it != mapClientConnect.end())
    {
        for (const auto& kv : it->second)
        {
            const uint8 nSubsType = kv.first;
            const uint128& nSubsId = kv.second;

            auto mt = mapClientSubscribe.find(nSubsType);
            if (mt != mapClientSubscribe.end())
            {
                mt->second.erase(nSubsId);
            }
        }
        mapClientConnect.erase(it);
    }
}

void CWsSubscribeFork::RemoveSubscribe(const uint128& nSubsId)
{
    uint8 nSubsType = GetSubsIdType(nSubsId);
    auto it = mapClientSubscribe.find(nSubsType);
    if (it != mapClientSubscribe.end())
    {
        auto nt = it->second.find(nSubsId);
        if (nt != it->second.end())
        {
            auto mt = mapClientConnect.find(nt->second.nClientConnId);
            if (mt != mapClientConnect.end())
            {
                mt->second.erase(nSubsType);
                if (mt->second.empty())
                {
                    mapClientConnect.erase(mt);
                }
            }
            it->second.erase(nt);
        }
    }
}

const std::map<uint128, CClientSubscribe>& CWsSubscribeFork::GetSubsListByType(const uint8 nSubsType)
{
    return mapClientSubscribe[nSubsType];
}

uint128 CWsSubscribeFork::CreateSubsId(const uint8 nSubsType, const uint64 nConnId)
{
    mtbase::CBufStream ss;
    ss << nSubsType << nConnId << nSubsIdSeed++;
    uint256 hash = CryptoHash(ss.GetData(), ss.GetSize());
    uint128 nSubsId(hash.begin(), uint128::size());
    *(nSubsId.begin()) = nSubsType;
    return nSubsId;
}

uint8 CWsSubscribeFork::GetSubsIdType(const uint128& nSubsId) const
{
    return *(nSubsId.begin());
}

/////////////////////////////
// CWsClient

uint64 CWsClient::GetConnId() const
{
    return nConnId;
}

const std::string& CWsClient::GetClientIp() const
{
    return strClientIp;
}

uint16 CWsClient::GetClientPort() const
{
    return nClientPort;
}

connection_hdl CWsClient::GetConnHdl() const
{
    return hdl;
}

/////////////////////////////
// CWsServer

CWsServer::CWsServer(const CChainId nChainIdIn, const uint16 nListenPortIn, const uint32 nMaxConnectionsIn, const boost::asio::ip::address& addrListenIn, mtbase::IIOModule* pRpcModIn, CWsService* pWssIn)
  : nChainId(nChainIdIn), nListenPort(nListenPortIn), nMaxConnections(nMaxConnectionsIn), addrListen(addrListenIn), pRpcMod(pRpcModIn), pWsService(pWssIn), pThreadWs(nullptr)
{
}

CWsServer::~CWsServer()
{
}

bool CWsServer::Start()
{
    if (!StartWsListen())
    {
        return false;
    }

    boost::thread::attributes attr;
    attr.set_stack_size(16 * 1024 * 1024);
    pThreadWs = new boost::thread(attr, boost::bind(&CWsServer::WsThreadFunc, this));
    if (!pThreadWs)
    {
        StopWsListen();
        return false;
    }
    return true;
}

void CWsServer::Stop()
{
    StopWsListen();

    if (pThreadWs)
    {
        pThreadWs->join();
        delete pThreadWs;
        pThreadWs = nullptr;
    }
}

void CWsServer::SendWsMsg(const uint64 nConnId, const std::string& strMsg)
{
    CReadLock rlock(rwAccess);

    auto it = mapWsClient.find(nConnId);
    if (it != mapWsClient.end())
    {
        wsServer.send(it->second.GetConnHdl(), strMsg, websocketpp::frame::opcode::TEXT);
    }
}

//-------------------------------------------------------
void CWsServer::WsThreadFunc()
{
    SetThreadName((string("ws-") + std::to_string(nListenPort)).c_str());

    try
    {
        // Start the ASIO io_service run loop
        wsServer.run();
    }
    catch (websocketpp::exception const& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
    }
    catch (...)
    {
        StdError(__PRETTY_FUNCTION__, "other exception");
    }
}

bool CWsServer::StartWsListen()
{
    try
    {
        // Set logging settings
        wsServer.set_access_channels(websocketpp::log::alevel::none);
        //wsServer.set_access_channels(websocketpp::log::alevel::all);
        wsServer.clear_access_channels(websocketpp::log::alevel::frame_payload);
        wsServer.set_reuse_addr(true);

        // Initialize Asio
        wsServer.init_asio();

        // Register our open handler
        wsServer.set_open_handler(bind(&CWsServer::OnOpen, this, ::_1));

        // Register our close handler
        wsServer.set_close_handler(bind(&CWsServer::OnClose, this, ::_1));

        // Register our message handler
        wsServer.set_message_handler(bind(&CWsServer::OnMessage, this, ::_1, ::_2));

        // Listen
        wsServer.listen(addrListen, nListenPort);

        // Start the server accept loop
        wsServer.start_accept();
    }
    catch (websocketpp::exception const& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    catch (...)
    {
        StdError(__PRETTY_FUNCTION__, "other exception");
        return false;
    }
    return true;
}

void CWsServer::StopWsListen()
{
    wsServer.stop();
}

void CWsServer::ParseIpPortString(const std::string& strIpPort, std::string& strIp, uint16& nPort)
{
    std::string endpointString = strIpPort;
    endpointString.erase(std::remove_if(endpointString.begin(), endpointString.end(), ::isspace), endpointString.end());
    if (endpointString.empty())
    {
        strIp = "";
        nPort = 0;
        return;
    }

    std::string portString;
    if (endpointString[0] == '[')
    {
        std::regex ipv6Regex("\\[([0-9a-fA-F:]+)\\]:(\\d+)");
        std::smatch match;
        if (std::regex_match(endpointString, match, ipv6Regex))
        {
            if (match.size() == 3)
            {
                strIp = match[1].str();
                portString = match[2].str();
            }
            else
            {
                strIp = "";
                portString = "";
            }
        }
    }
    else
    {
        std::regex ipv4Regex("([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+):(\\d+)");
        std::smatch match;
        if (std::regex_match(endpointString, match, ipv4Regex))
        {
            if (match.size() == 3)
            {
                strIp = match[1].str();
                portString = match[2].str();
            }
            else
            {
                strIp = "";
                portString = "";
            }
        }
    }
    if (portString.empty())
    {
        nPort = 0;
    }
    else
    {
        try
        {
            nPort = std::stoi(portString);
        }
        catch (const std::invalid_argument& e)
        {
            nPort = 0;
        }
        catch (const std::out_of_range& e)
        {
            nPort = 0;
        }
    }
}

void CWsServer::OnOpen(connection_hdl hdl)
{
    try
    {
        connection_ptr con = wsServer.get_con_from_hdl(hdl);
        uint64 connection_id = reinterpret_cast<uint64>(con.get());

        std::string strClientIp;
        uint16 nClientPort = 0;
        ParseIpPortString(con->get_remote_endpoint(), strClientIp, nClientPort);

        StdDebug("CWsService", "OnOpen: client address: %s:%d, connect id: 0x%lx", strClientIp.c_str(), nClientPort, connection_id);

        {
            CWriteLock wlock(rwAccess);

            auto it = mapWsClient.find(connection_id);
            if (it != mapWsClient.end())
            {
                mapWsClient.erase(it);
            }
            mapWsClient.insert(std::make_pair(connection_id, CWsClient(connection_id, strClientIp, nClientPort, hdl)));
        }
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
    }
}

void CWsServer::OnClose(connection_hdl hdl)
{
    try
    {
        connection_ptr con = wsServer.get_con_from_hdl(hdl);
        uint64 connection_id = reinterpret_cast<uint64>(con.get());

        std::string strClientIp;
        uint16 nClientPort = 0;
        ParseIpPortString(con->get_remote_endpoint(), strClientIp, nClientPort);

        StdDebug("CWsService", "OnClose: client address: %s:%d, connect id: 0x%lx", strClientIp.c_str(), nClientPort, connection_id);

        pWsService->RemoveClientAllSubscribe(nChainId, connection_id);
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
    }
}

void CWsServer::OnMessage(connection_hdl hdl, message_ptr msg)
{
    try
    {
        if (msg->get_opcode() == websocketpp::frame::opcode::TEXT)
        {
            connection_ptr con = wsServer.get_con_from_hdl(hdl);
            uint64 connection_id = reinterpret_cast<uint64>(con.get());
            const std::string& strMsg = msg->get_payload();

            // std::string strClientIp;
            // uint16 nClientPort = 0;
            // ParseIpPortString(con->get_remote_endpoint(), strClientIp, nClientPort);

            // StdDebug("CWsService", "OnMessage: TEXT: address: %s:%d, connect id: %lu, msg: %s", strClientIp.c_str(), nClientPort, connection_id, strMsg.c_str());

            if (!strMsg.empty())
            {
                CReadLock rlock(rwAccess);

                auto it = mapWsClient.find(connection_id);
                if (it != mapWsClient.end())
                {
                    PostRpcModMsg(it->second, strMsg);
                }
            }
        }
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
    }
}

bool CWsServer::PostRpcModMsg(const CWsClient& wsClient, const std::string& strMsg)
{
    CEventHttpReq* pEventHttpReq = new CEventHttpReq(wsClient.GetConnId());
    if (pEventHttpReq == nullptr)
    {
        return false;
    }

    CHttpReq& req = pEventHttpReq->data;

    req.nSourceType = REQ_SOURCE_TYPE_WS;
    req.nReqChainId = nChainId;
    req.nListenPort = nListenPort;
    req.strPeerIp = wsClient.GetClientIp();
    req.nPeerPort = wsClient.GetClientPort();

    req.mapHeader["host"] = wsClient.GetClientIp();
    req.mapHeader["url"] = "/" + to_string(VERSION);
    req.mapHeader["method"] = "POST";
    req.mapHeader["accept"] = "application/json";
    req.mapHeader["content-type"] = "application/json";
    req.mapHeader["user-agent"] = string("metabasenet-json-rpc/");
    req.mapHeader["connection"] = "Keep-Alive";

    req.strContentType = "application/json";
    req.strContent = strMsg;

    pRpcMod->PostEvent(pEventHttpReq);
    return true;
}

//////////////////////////////
// CWsService

CWsService::CWsService()
{
    pRpcModIOModule = nullptr;
}

CWsService::~CWsService()
{
}

bool CWsService::HandleEvent(CEventWsServicePushNewBlock& eventPush)
{
    CReadLock rlock(rwAccess);

    const CWssPushNewBlock& newBlock = eventPush.data;
    const uint256& hashBlock = newBlock.hashBlock;
    const CBlock& block = newBlock.block;
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashBlock);
    auto& subsFork = mapWsSubscribeFork[nChainId];

    SHP_WS_SERVER ptrWsServer = nullptr;
    auto it = mapWsServer.find(nChainId);
    if (it != mapWsServer.end())
    {
        ptrWsServer = it->second;
    }
    if (ptrWsServer == nullptr)
    {
        return true;
    }

    // {
    // 	"jsonrpc": "2.0",
    // 	"method": "eth_subscription",
    // 	"params": {
    // 		"subscription": "0xa92f496bc55f58e548996847ed3049e2",
    // 		"result": {
    // 			"parentHash": "0x76ac73397d33ad728f0ae1a5e6597756a1421693e10cd25e15ab26be1778f1c4",
    // 			"sha3Uncles": "0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347",
    // 			"miner": "0x29b7180a5ba70c5a1c738e877731faee44fcdaa3",
    // 			"stateRoot": "0x1ce174c14ca5dea1cf978f60e7c032a62b1d080dd89455ad0cde295c44e7a86e",
    // 			"transactionsRoot": "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
    // 			"receiptsRoot": "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
    // 			"logsBloom": "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
    // 			"difficulty": "0x80c33",
    // 			"number": "0xd9e",
    // 			"gasLimit": "0x1c9c380",
    // 			"gasUsed": "0x0",
    // 			"timestamp": "0x65950b18",
    // 			"extraData": "0xd883010b06846765746888676f312e32302e34856c696e7578",
    // 			"mixHash": "0xc05d70bd1b78534e53d8b2445957c0583794d524c37cb9af5ccbaa67687e35ad",
    // 			"nonce": "0x446068dae8aa1e4f",
    // 			"baseFeePerGas": null,
    // 			"withdrawalsRoot": null,
    // 			"hash": "0x8c82f5c0a375a98da3482b158870a02b4314cc55df723c6a706fc8d9f1b91a42"
    // 		}
    // 	}
    // }

    std::string strLogsbloom;
    if (!block.btBloomData.empty())
    {
        strLogsbloom = ToHexString(block.btBloomData);
    }
    else
    {
        strLogsbloom = "0x";
    }
    std::string strReceiptsRoot;
    if (!block.hashReceiptsRoot.IsNull())
    {
        strReceiptsRoot = block.hashReceiptsRoot.GetHex();
    }
    else
    {
        strReceiptsRoot = "0x";
    }
    std::string strNonceTemp = hashBlock.ToString();
    std::string strNonce = std::string("0x") + strNonceTemp.substr(strNonceTemp.size() - 16);

    for (const auto& kv : subsFork.GetSubsListByType(WSCS_SUBS_TYPE_NEW_BLOCK))
    {
        const uint128& nSubsId = kv.first;
        const CClientSubscribe& clientSubs = kv.second;

        std::string strMsg;
        strMsg += "{";
        strMsg += "\"jsonrpc\": \"2.0\",";
        strMsg += "\"method\": \"eth_subscription\",";
        strMsg += "\"params\": {";
        strMsg += ("\"subscription\": \"" + nSubsId.GetHex() + "\",");
        strMsg += "\"result\": {";
        strMsg += ("\"parentHash\": \"" + block.hashPrev.GetHex() + "\",");
        strMsg += "\"sha3Uncles\": \"0x\",";
        strMsg += ("\"miner\": \"" + block.txMint.GetToAddress().ToString() + "\",");
        strMsg += ("\"stateRoot\": \"" + block.hashStateRoot.GetHex() + "\",");
        strMsg += ("\"transactionsRoot\": \"" + block.hashMerkleRoot.GetHex() + "\",");
        strMsg += ("\"receiptsRoot\": \"" + strReceiptsRoot + "\",");
        strMsg += ("\"logsBloom\": \"" + strLogsbloom + "\",");
        strMsg += "\"difficulty\": \"0x0\",";
        strMsg += ("\"number\": \"" + ToHexString(block.GetBlockNumber()) + "\",");
        strMsg += ("\"gasLimit\": \"" + block.nGasLimit.GetValueHex() + "\",");
        strMsg += ("\"gasUsed\": \"" + block.nGasUsed.GetValueHex() + "\",");
        strMsg += ("\"timestamp\": \"" + ToHexString(block.GetBlockTime()) + "\",");
        strMsg += "\"extraData\": \"0x\",";
        strMsg += "\"mixHash\": \"0x\",";
        strMsg += ("\"nonce\": \"" + strNonce + "\",");
        strMsg += "\"baseFeePerGas\": null,";
        strMsg += "\"withdrawalsRoot\": null,";
        strMsg += ("\"hash\": \"" + hashBlock.GetHex() + "\"");
        strMsg += "}}}";

        ptrWsServer->SendWsMsg(clientSubs.nClientConnId, strMsg);
    }
    return true;
}

bool CWsService::HandleEvent(CEventWsServicePushLogs& eventPush)
{
    CReadLock rlock(rwAccess);

    const uint256& hashBlock = eventPush.data.hashBlock;
    const CTransactionReceipt& receipt = eventPush.data.receipt;
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashBlock);
    auto& subsFork = mapWsSubscribeFork[nChainId];

    SHP_WS_SERVER ptrWsServer = nullptr;
    auto it = mapWsServer.find(nChainId);
    if (it != mapWsServer.end())
    {
        ptrWsServer = it->second;
    }
    if (ptrWsServer == nullptr)
    {
        return true;
    }

    // {
    // 	"jsonrpc": "2.0",
    // 	"method": "eth_subscription",
    // 	"params": {
    // 		"subscription": "0x96f43d778ad928079559dfa7bcb1d830",
    // 		"result": {
    // 			"address": "0xab6a1a01d8a6b8b6cbabc1f4e4e955da9d83b287",
    // 			"topics": ["0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef", "0x000000000000000000000000b955034fcefb66112bab47483c8d243b86cb2c1d", "0x0000000000000000000000009835c12d95f059eb4eaecb4b00f2ea2c99b2a0d4"],
    // 			"data": "0x000000000000000000000000000000000000000000000000000000000000014d",
    // 			"blockNumber": "0xf3d",
    // 			"transactionHash": "0x20ebf76d8ff2d91cf80500a928a5da8f92be7d7024295edc921ce9312e547fcb",
    // 			"transactionIndex": "0x0",
    // 			"blockHash": "0xbfe8a92555e7a151d59c3a4e4030a5854850c2a919a429f05fe6cb3864d93069",
    // 			"logIndex": "0x0",
    // 			"removed": false
    // 		}
    // 	}
    // }

    for (const auto& kv : subsFork.GetSubsListByType(WSCS_SUBS_TYPE_LOGS))
    {
        const uint128& nSubsId = kv.first;
        const CClientSubscribe& clientSubs = kv.second;

        MatchLogsVec vLogs;
        clientSubs.matchesLogs(receipt, vLogs);

        for (const CMatchLogs& v : vLogs)
        {
            std::string strMsg;

            strMsg += "{";
            strMsg += "\"jsonrpc\": \"2.0\",";
            strMsg += "\"method\": \"eth_subscription\",";
            strMsg += "\"params\": {";
            strMsg += ("\"subscription\": \"" + nSubsId.GetHex() + "\",");
            strMsg += "\"result\": {";
            strMsg += ("\"address\": \"" + v.address.ToString() + "\",");
            strMsg += "\"topics\": [";
            for (uint64 i = 0; i < v.topics.size(); i++)
            {
                const uint256& h = v.topics[i];
                if (i == 0)
                {
                    strMsg += ("\"" + h.GetHex() + "\"");
                }
                else
                {
                    strMsg += (", \"" + h.GetHex() + "\"");
                }
            }
            strMsg += "],";
            strMsg += ("\"data\": \"" + ToHexString(v.data) + "\",");
            strMsg += ("\"blockNumber\": \"" + ToHexString(receipt.nBlockNumber) + "\",");
            strMsg += ("\"transactionHash\": \"" + receipt.txid.GetHex() + "\",");
            strMsg += ("\"transactionIndex\": \"" + ToHexString(receipt.nTxIndex) + "\",");
            strMsg += ("\"blockHash\": \"" + hashBlock.GetHex() + "\",");
            strMsg += ("\"logIndex\": \"" + ToHexString(v.nLogIndex) + "\",");
            strMsg += ("\"removed\": " + (v.fRemoved ? string("true") : string("false")));
            strMsg += "}}}";

            ptrWsServer->SendWsMsg(clientSubs.nClientConnId, strMsg);
        }
    }
    return true;
}

bool CWsService::HandleEvent(CEventWsServicePushNewPendingTx& eventPush)
{
    CReadLock rlock(rwAccess);

    const uint256& txid = eventPush.data.txid;
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(eventPush.data.hashFork);
    auto& subsFork = mapWsSubscribeFork[nChainId];

    SHP_WS_SERVER ptrWsServer = nullptr;
    auto it = mapWsServer.find(nChainId);
    if (it != mapWsServer.end())
    {
        ptrWsServer = it->second;
    }
    if (ptrWsServer == nullptr)
    {
        return true;
    }

    //{"jsonrpc":"2.0","method":"eth_subscription","params":{"subscription":"0x2723d1f08be37d59f0ef54767fb0a84d","result":"0xf5623f516a8190ab0ccb940890ec13a46f504f131987332aacd9adcb7c744a7c"}}

    for (const auto& kv : subsFork.GetSubsListByType(WSCS_SUBS_TYPE_NEW_PENDING_TX))
    {
        const uint128& nSubsId = kv.first;
        const CClientSubscribe& clientSubs = kv.second;

        std::string strMsg = ("{\"jsonrpc\":\"2.0\",\"method\":\"eth_subscription\",\"params\":{\"subscription\":\"" + nSubsId.GetHex() + "\",\"result\":\"" + txid.ToString() + "\"}}");
        ptrWsServer->SendWsMsg(clientSubs.nClientConnId, strMsg);
    }
    return true;
}

bool CWsService::HandleEvent(CEventWsServicePushSyncing& eventPush)
{
    CReadLock rlock(rwAccess);

    const CWssPushSyncing& pushSyncing = eventPush.data;
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(pushSyncing.hashFork);
    auto& subsFork = mapWsSubscribeFork[nChainId];

    SHP_WS_SERVER ptrWsServer = nullptr;
    auto it = mapWsServer.find(nChainId);
    if (it != mapWsServer.end())
    {
        ptrWsServer = it->second;
    }
    if (ptrWsServer == nullptr)
    {
        return true;
    }

    // {
    //     "jsonrpc":"2.0",
    //     "subscription":"0xe2ffeb2703bcf602d42922385829ce96",
    //     "result": {
    //         "syncing":true,
    //         "status": {
    //             "startingBlock":673427,
    //             "currentBlock":67400,
    //             "highestBlock":674432,
    //             "pulledStates":0,
    //             "knownStates":0
    //         }
    //     }
    // }

    for (const auto& kv : subsFork.GetSubsListByType(WSCS_SUBS_TYPE_SYNCING))
    {
        const uint128& nSubsId = kv.first;
        const CClientSubscribe& clientSubs = kv.second;

        std::string strMsg;

        strMsg += "{";
        strMsg += "\"jsonrpc\": \"2.0\",";
        strMsg += ("\"subscription\": \"" + nSubsId.GetHex() + "\",");
        strMsg += "\"result\": {";
        strMsg += ("\"syncing\": " + std::string(pushSyncing.fSyncing ? "true" : "false"));
        if (pushSyncing.fSyncing)
        {
            strMsg += ",";
            strMsg += "\"status\": {";
            strMsg += ("\"startingBlock\": " + std::to_string(pushSyncing.nStartingBlockNumber) + ",");
            strMsg += ("\"currentBlock\": " + std::to_string(pushSyncing.nCurrentBlockNumber) + ",");
            strMsg += ("\"highestBlock\": " + std::to_string(pushSyncing.nHighestBlockNumber) + ",");
            strMsg += ("\"pulledStates\": " + std::to_string(pushSyncing.nPulledStates) + ",");
            strMsg += ("\"knownStates\": " + std::to_string(pushSyncing.nKnownStates));
            strMsg += "}";
        }
        strMsg += "}}";

        ptrWsServer->SendWsMsg(clientSubs.nClientConnId, strMsg);
    }
    return true;
}

void CWsService::AddNewBlockSubscribe(const CChainId nChainId, const uint64 nClientConnId, uint128& nSubsId)
{
    CWriteLock wlock(rwAccess);
    nSubsId = mapWsSubscribeFork[nChainId].AddSubscribe(nClientConnId, WSCS_SUBS_TYPE_NEW_BLOCK, {}, {});
}

void CWsService::AddLogsSubscribe(const CChainId nChainId, const uint64 nClientConnId, const std::set<CDestination>& setSubsAddress, const std::set<uint256>& setSubsTopics, uint128& nSubsId)
{
    CWriteLock wlock(rwAccess);
    nSubsId = mapWsSubscribeFork[nChainId].AddSubscribe(nClientConnId, WSCS_SUBS_TYPE_LOGS, setSubsAddress, setSubsTopics);
}

void CWsService::AddNewPendingTxSubscribe(const CChainId nChainId, const uint64 nClientConnId, uint128& nSubsId)
{
    CWriteLock wlock(rwAccess);
    nSubsId = mapWsSubscribeFork[nChainId].AddSubscribe(nClientConnId, WSCS_SUBS_TYPE_NEW_PENDING_TX, {}, {});
}

void CWsService::AddSyncingSubscribe(const CChainId nChainId, const uint64 nClientConnId, uint128& nSubsId)
{
    CWriteLock wlock(rwAccess);
    nSubsId = mapWsSubscribeFork[nChainId].AddSubscribe(nClientConnId, WSCS_SUBS_TYPE_SYNCING, {}, {});
}

bool CWsService::RemoveSubscribe(const CChainId nChainId, const uint64 nClientConnId, const uint128& nSubsId)
{
    CWriteLock wlock(rwAccess);
    mapWsSubscribeFork[nChainId].RemoveSubscribe(nSubsId);
    return true;
}

void CWsService::SendWsMsg(const CChainId nChainId, const uint64 nNonce, const std::string& strMsg)
{
    CReadLock rlock(rwAccess);

    auto it = mapWsServer.find(nChainId);
    if (it != mapWsServer.end())
    {
        it->second->SendWsMsg(nNonce, strMsg);
    }
}

void CWsService::RemoveClientAllSubscribe(const CChainId nChainId, const uint64 nClientConnId)
{
    CWriteLock wlock(rwAccess);
    mapWsSubscribeFork[nChainId].RemoveClientAllSubscribe(nClientConnId);
}

//----------------------------------------------------------------------------
bool CWsService::HandleInitialize()
{
    if (!GetObject("rpcmod", pRpcModIOModule))
    {
        Error("Failed to request rpcmod");
        return false;
    }

    const CRPCServerConfig* pSrvCfg = RPCServerConfig();
    if (!pSrvCfg)
    {
        Error("Failed to get config");
        return false;
    }
    boost::asio::ip::address addrListen = pSrvCfg->epRPC.address();

    if (pSrvCfg->nChainId != 0 && pSrvCfg->nWsPort != 0)
    {
        auto ptrWs = MAKE_SHARED_WS_SERVER(pSrvCfg->nChainId, pSrvCfg->nWsPort, pSrvCfg->nRPCMaxConnections, addrListen, pRpcModIOModule, this);
        if (!ptrWs)
        {
            Error("Make ws server failed");
            return false;
        }
        mapWsServer.insert(std::make_pair(pSrvCfg->nChainId, ptrWs));
    }
    for (const auto& kv : pSrvCfg->mapChainIdRpcPort)
    {
        const CChainId nEsChainId = kv.first;
        const uint16 nEsWsPort = kv.second.second;
        if (nEsChainId != 0 && nEsWsPort != 0)
        {
            auto ptrWs = MAKE_SHARED_WS_SERVER(nEsChainId, nEsWsPort, pSrvCfg->nRPCMaxConnections, addrListen, pRpcModIOModule, this);
            if (!ptrWs)
            {
                Error("Make ws server failed");
                return false;
            }
            mapWsServer.insert(std::make_pair(nEsChainId, ptrWs));
        }
    }
    return true;
}

void CWsService::HandleDeinitialize()
{
    pRpcModIOModule = nullptr;
}

bool CWsService::HandleInvoke()
{
    if (!IWsService::HandleInvoke())
    {
        return false;
    }
    for (auto& kv : mapWsServer)
    {
        if (!kv.second->Start())
        {
            return false;
        }
    }
    return true;
}

void CWsService::HandleHalt()
{
    for (auto& kv : mapWsServer)
    {
        kv.second->Stop();
    }
    IWsService::HandleHalt();
}

} // namespace metabasenet
