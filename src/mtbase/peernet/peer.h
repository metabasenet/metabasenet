// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_PEERNET_PEER_H
#define MTBASE_PEERNET_PEER_H

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <string>
#include <vector>

#include "netio/ioproc.h"
#include "stream/stream.h"

namespace mtbase
{

class CPeerNet;

class CPeerTunnel
{
public:
    CPeerTunnel()
      : nPacketSize(0) {}

    bool AddRecvData(CBufStream& ssAdd, CBufStream& ssRecvPacket);
    bool WriteStream(CBufStream& ss);
    bool GetSendData(bytes& btSend);

protected:
    CBufStream ssTunRecv;
    CBufStream ssTunSend;

    uint32 nPacketSize;
};

class CPeer
{
public:
    typedef boost::function<bool()> CompltFunc;

    CPeer(CPeerNet* pPeerNetIn, CIOClient* pClientIn, uint64 nNonceIn, bool fInBoundIn);
    virtual ~CPeer();
    void Close();
    uint64 GetNonce();
    bool IsInBound();
    const boost::asio::ip::tcp::endpoint GetRemote();
    const boost::asio::ip::tcp::endpoint GetLocal();
    virtual void Activate();
    virtual bool PingTimer(uint32 nTimerId);

protected:
    CBufStream& ReadStream();
    bool WriteStream(const uint32 nTunnelId, CBufStream& ss);

    void Read(std::size_t nLength, CompltFunc fnComplt);
    void Write();

    void HandleRead(std::size_t nTransferred, std::size_t nReadLength, CompltFunc fnComplt, const uint8 nTunnelId, const uint16 nReadSize);
    void HandleWriten(std::size_t nTransferred);

public:
    int64 nTimeActive;
    int64 nTimeRecv;
    int64 nTimeSend;

protected:
    CPeerNet* pPeerNet;
    CIOClient* pClient;
    uint64 nNonce;
    bool fInBound;

    CBufStream ssRecv;
    CBufStream ssWrite;

    CBufStream ssHisRecv;
    CBufStream ssReadTunnel;

    std::map<uint32, CPeerTunnel> mapPeerTunnel;
};

} // namespace mtbase

#endif //MTBASE_PEERNET_PEER_H
