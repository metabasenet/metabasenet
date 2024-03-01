// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_PEERNET_NODEMANAGER_H
#define MTBASE_PEERNET_NODEMANAGER_H

#include <boost/any.hpp>
#include <boost/asio.hpp>
#include <map>
#include <set>
#include <string>

#include "type.h"

namespace mtbase
{

class CNode
{
public:
    CNode() {}
    CNode(const boost::asio::ip::tcp::endpoint& epIn, const std::string& strNameIn,
          const boost::any& dataIn)
      : ep(epIn), strName(strNameIn), data(dataIn), nRetries(0) {}

public:
    boost::asio::ip::tcp::endpoint ep;
    std::string strName;
    boost::any data;
    int nRetries;
};

class CNodeManager
{
public:
    CNodeManager();
    void AddNew(const boost::asio::ip::tcp::endpoint& ep, const std::string& strName,
                const boost::any& data);
    void Remove(const boost::asio::ip::tcp::endpoint& ep);
    std::string GetName(const boost::asio::ip::tcp::endpoint& ep);
    bool GetData(const boost::asio::ip::tcp::endpoint& ep, boost::any& dataRet);
    bool SetData(const boost::asio::ip::tcp::endpoint& ep, const boost::any& dataIn);
    void Clear();
    void Ban(const boost::asio::ip::address& address, int64 nBanTo);
    bool Employ(boost::asio::ip::tcp::endpoint& ep);
    void Dismiss(const boost::asio::ip::tcp::endpoint& ep, bool fForceRetry, bool fReset);
    void Retrieve(std::vector<CNode>& vNode);
    bool IsDnseedAddress(const boost::asio::ip::address& address);
    int GetCandidateNodeCount()
    {
        return mapNode.size();
    }

protected:
    void AddIdleNode(int64 nIntervalTime, const boost::asio::ip::tcp::endpoint& ep);
    void RemoveInactiveNodes();
    const std::string GetEpString(const boost::asio::ip::tcp::endpoint& ep);

protected:
    enum
    {
        MAX_IDLENODES = 512,
        REMOVE_COUNT = 16
    };
    enum
    {
        RETRY_INTERVAL_BASE = 30,
        MAX_RETRIES = 16,
        MAX_IDLETIME = 28800
    };
    std::map<boost::asio::ip::tcp::endpoint, CNode> mapNode;
    std::multimap<int64, boost::asio::ip::tcp::endpoint> mapIdle;
    std::set<boost::asio::ip::address> setDnseedAddress;
};

} // namespace mtbase

#endif //MTBASE_PEERNET_NODEMANAGER_H
