// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_HTTP_HTTPTYPE_H
#define MTBASE_HTTP_HTTPTYPE_H

#include <map>
#include <string.h>
#include <vector>

#include "http/httpcookie.h"
#include "stream/stream.h"

namespace mtbase
{

#define MAX_HTTP_CONTENT_LENGTH 2000000

enum
{
    REQ_SOURCE_TYPE_HTTP = 0,
    REQ_SOURCE_TYPE_WS = 1,
};

class _iless
{
public:
    bool operator()(const std::string& a, const std::string& b) const
    {
        return (strcasecmp(a.c_str(), b.c_str()) < 0);
    }
};

typedef std::map<std::string, std::string> MAPKeyValue;
typedef std::map<std::string, std::string, _iless> MAPIKeyValue;
typedef std::map<std::string, CHttpCookie, _iless> MAPCookie;

class CHttpContent
{
    friend class CStream;

protected:
    void Serialize(CStream& s, SaveType&)
    {
        s.Write(&strContent[0], strContent.size());
    }
    void Serialize(CStream& s, LoadType&)
    {
        std::size_t size = s.GetSize();
        strContent.resize(size);
        s.Read(&strContent[0], size);
    }
    void Serialize(CStream& s, std::size_t& serSize)
    {
        (void)s;
        serSize += strContent.size();
    }

public:
    std::string strContentType;
    std::string strContent;
};

class CHttpReq : public CHttpContent
{
public:
    uint8 nSourceType; // 0: HTTP, 1: WS
    uint32 nReqChainId;
    uint16 nListenPort;
    std::string strPeerIp;
    uint16 nPeerPort;
    std::string strUser;
    MAPIKeyValue mapHeader;
    MAPKeyValue mapQuery;
    MAPIKeyValue mapCookie;
    int64 nTimeout;
};

class CHttpReqData : public CHttpReq
{
public:
    std::string strIOModule;
    std::string strProtocol;
    std::string strPathCA;
    std::string strPathCert;
    std::string strPathPK;
    bool fVerifyPeer;
};

class CHttpRsp : public CHttpContent
{
public:
    MAPIKeyValue mapHeader;
    MAPCookie mapCookie;
    int nStatusCode;
};

class CHttpAbort
{
public:
    std::string strIOModule;
    std::vector<uint64> vNonce;
};

class CHttpBroken
{
public:
    bool fEventStream;
};

} // namespace mtbase

#endif //MTBASE_HTTP_HTTPTYPE_H
