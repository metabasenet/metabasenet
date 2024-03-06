// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_MODE_MODULE_TYPE_H
#define METABASENET_MODE_MODULE_TYPE_H

namespace metabasenet
{
// module type
enum class EModuleType
{
    LOCK,             // lock file
    BLOCKMAKER,       // CBlockMaker
    COREPROTOCOL,     // CCoreProtocol
    DISPATCHER,       // CDispatcher
    HTTPGET,          // CHttpGet
    HTTPSERVER,       // CHttpServer
    NETCHANNEL,       // CNetChannel
    DELEGATEDCHANNEL, // CDelegatedChannel
    BLOCKCHANNEL,     // CBlockChannel
    CERTTXCHANNEL,    // CCertTxChannel
    USERTXCHANNEL,    // CUserTxChannel
    BLOCKVOTECHANNEL, // CBlockVoteChannel
    BLOCKCROSSPROVE,  // CBlockCrossProveChannel
    NETWORK,          // CNetwork
    RPCCLIENT,        // CRPCClient
    RPCMODE,          // CRPCMod
    SERVICE,          // CService
    TXPOOL,           // CTxPool
    WALLET,           // CWallet
    BLOCKCHAIN,       // CBlockChain
    CONSENSUS,        // CConsensus
    FORKMANAGER,      // CForkManager
    DATASTAT,         // CDataStat
    RECOVERY,         // CRecovery
    WSSERVICE,        // CWsService
    BLOCKFILTER,      // CBlockFilter
};

} // namespace metabasenet

#endif // METABASENET_MODE_MODULE_TYPE_H
