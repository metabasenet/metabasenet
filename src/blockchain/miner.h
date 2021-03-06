// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_MINER_H
#define METABASENET_MINER_H

#include "json/json_spirit_value.h"
#include <string>
#include <vector>

#include "base.h"
#include "hcode.h"

namespace metabasenet
{

class CMinerWork
{
public:
    std::vector<unsigned char> vchWorkData;
    int nPrevBlockHeight;
    uint256 hashPrev;
    int64 nPrevTime;
    int nAlgo;
    int nBits;

public:
    CMinerWork()
    {
        SetNull();
    }
    void SetNull()
    {
        vchWorkData.clear();
        nPrevBlockHeight = 0;
        hashPrev = 0;
        nPrevTime = 0;
        nAlgo = 0;
        nBits = 0;
    }
    bool IsNull() const
    {
        return (nAlgo == 0);
    }
};

class CMiner : public hcode::IIOModule, virtual public hcode::CHttpEventListener
{
public:
    CMiner(const std::vector<std::string>& vArgsIn);
    ~CMiner();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    const CRPCClientConfig* Config();
    bool Interrupted()
    {
        return (nMinerStatus != MINER_RUN);
    }
    bool HandleEvent(hcode::CEventHttpGetRsp& event) override;
    bool SendRequest(uint64 nNonce, const string& content);
    bool GetWork();
    bool SubmitWork(const std::vector<unsigned char>& vchWorkData);
    void CancelRPC();

private:
    enum
    {
        MINER_RUN = 0,
        MINER_RESET = 1,
        MINER_EXIT = 2,
        MINER_HOLD = 3
    };
    void LaunchFetcher();
    void LaunchMiner();

protected:
    hcode::IIOProc* pHttpGet;
    hcode::CThread thrFetcher;
    hcode::CThread thrMiner;
    boost::mutex mutex;
    boost::condition_variable condFetcher;
    boost::condition_variable condMiner;
    std::string strAddrSpent;
    std::string strMintKey;
    int nMinerStatus;
    CMinerWork workCurrent;
    uint64 nNonceGetWork;
    uint64 nNonceSubmitWork;
};

} // namespace metabasenet
#endif //METABASENET_MINER_H
