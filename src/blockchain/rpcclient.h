// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_RPCCLIENT_H
#define METABASENET_RPCCLIENT_H

#include "json/json_spirit_value.h"
#include <boost/asio.hpp>
#include <string>
#include <thread>
#include <vector>

#include "base.h"
#include "mtbase.h"
#include "rpc/rpc.h"

namespace metabasenet
{

class CRPCClient : public mtbase::IIOModule, virtual public mtbase::CHttpEventListener
{
public:
    CRPCClient(bool fConsole = true);
    ~CRPCClient();
    void ConsoleHandleLine(const std::string& strLine);

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    const CRPCClientConfig* Config();

    bool HandleEvent(mtbase::CEventHttpGetRsp& event) override;
    bool GetResponse(uint64 nNonce, const std::string& content);
    bool CallRPC(rpc::CRPCParamPtr spParam, int nReqId);
    bool CallConsoleCommand(const std::vector<std::string>& vCommand);
    void LaunchConsole();
    void LaunchCommand();
    void CancelCommand();

    void EnterLoop();
    void LeaveLoop();

protected:
    mtbase::IIOProc* pHttpGet;
    mtbase::CThread thrDispatch;
    std::vector<std::string> vArgs;
    uint64 nLastNonce;
    mtbase::CIOCompletion ioComplt;
    std::atomic<bool> isConsoleRunning;
};

} // namespace metabasenet
#endif //METABASENET_RPCCLIENT_H
