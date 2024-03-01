// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "peer.h"

#include <boost/bind.hpp>

#include "peernet.h"
#include "util.h"

using boost::asio::ip::tcp;

namespace mtbase
{

///////////////////////////////
// CPeerTunnel

bool CPeerTunnel::AddRecvData(CBufStream& ssAdd, CBufStream& ssRecvPacket)
{
    if (ssAdd.GetSize() > 0)
    {
        ssTunRecv.Write(ssAdd.GetData(), ssAdd.GetSize());
    }
    if (nPacketSize == 0)
    {
        if (ssTunRecv.GetSize() < sizeof(nPacketSize))
        {
            return true;
        }
        ssTunRecv >> nPacketSize;
        if (nPacketSize == 0)
        {
            return false;
        }
    }
    if (ssTunRecv.GetSize() >= nPacketSize)
    {
        ssTunRecv.TransferStream(ssRecvPacket, nPacketSize);
        nPacketSize = 0;
    }
    return true;
}

bool CPeerTunnel::WriteStream(CBufStream& ss)
{
    uint32 nSize = ss.GetSize();
    if (nSize > 0)
    {
        ssTunSend << nSize;
        ssTunSend.Write(ss.GetData(), ss.GetSize());
    }
    return true;
}

bool CPeerTunnel::GetSendData(bytes& btSend)
{
    std::size_t n = ssTunSend.GetSize();
    if (n == 0)
    {
        return false;
    }
    if (n > 256)
    {
        n = 256;
    }
    btSend.resize(n + 2);
    ssTunSend.Read((char*)(btSend.data() + 2), n);
    btSend[1] = n - 1;
    return true;
}

///////////////////////////////
// CPeer

CPeer::CPeer(CPeerNet* pPeerNetIn, CIOClient* pClientIn, uint64 nNonceIn, bool fInBoundIn)
  : pPeerNet(pPeerNetIn), pClient(pClientIn), nNonce(nNonceIn), fInBound(fInBoundIn)
{
}

CPeer::~CPeer()
{
    Close();
}

void CPeer::Close()
{
    if (pClient)
    {
        pClient->Close();
        pClient = nullptr;
    }
}

uint64 CPeer::GetNonce()
{
    return nNonce;
}

bool CPeer::IsInBound()
{
    return fInBound;
}

const tcp::endpoint CPeer::GetRemote()
{
    return (pClient->GetRemote());
}

const tcp::endpoint CPeer::GetLocal()
{
    return (pClient->GetLocal());
}

void CPeer::Activate()
{
    ssWrite.Clear();

    nTimeActive = GetTime();
    nTimeRecv = 0;
    nTimeSend = 0;
}

bool CPeer::PingTimer(uint32 nTimerId)
{
    return false;
}

CBufStream& CPeer::ReadStream()
{
    return ssRecv;
}

bool CPeer::WriteStream(const uint32 nTunnelId, CBufStream& ss)
{
    if (!mapPeerTunnel[nTunnelId].WriteStream(ss))
    {
        return false;
    }
    Write();
    return true;
}

void CPeer::Read(size_t nLength, CompltFunc fnComplt)
{
    ssRecv.Clear();
    if (ssHisRecv.GetSize() >= nLength)
    {
        ssHisRecv.TransferStream(ssRecv, nLength);
        if (!fnComplt())
        {
            pPeerNet->HandlePeerViolate(this);
        }
    }
    else
    {
        ssReadTunnel.Clear();
        pClient->Read(ssReadTunnel, 2, boost::bind(&CPeer::HandleRead, this, _1, nLength, fnComplt, 0, 0));
    }
}

void CPeer::Write()
{
    if (ssWrite.GetSize() > 0)
    {
        return;
    }
    bool fAdd;
    do
    {
        fAdd = false;
        for (auto& pk : mapPeerTunnel)
        {
            bytes btSendData;
            if (pk.second.GetSendData(btSendData))
            {
                btSendData[0] = pk.first; // tunnel id
                ssWrite.Write((char*)(btSendData.data()), btSendData.size());
                fAdd = true;
            }
        }
    } while (ssWrite.GetSize() < 1200 && fAdd);
    if (ssWrite.GetSize() > 0)
    {
        pClient->Write(ssWrite, boost::bind(&CPeer::HandleWriten, this, _1));
    }
}

void CPeer::HandleRead(size_t nTransferred, std::size_t nReadLength, CompltFunc fnComplt, const uint8 nTunnelId, const uint16 nReadSize)
{
    if (nTransferred != 0)
    {
        nTimeRecv = GetTime();
        if (nReadSize == 0)
        {
            // read tunnel header
            if (ssReadTunnel.GetSize() != 2)
            {
                pPeerNet->HandlePeerError(this);
            }
            else
            {
                uint8* p = (uint8*)(ssReadTunnel.GetData());
                uint8 nTid = *p;
                uint16 nBodySize = *(p + 1);
                nBodySize += 1;
                ssReadTunnel.Clear();
                pClient->Read(ssReadTunnel, nBodySize, boost::bind(&CPeer::HandleRead, this, _1, nReadLength, fnComplt, nTid, nBodySize));
            }
        }
        else
        {
            // read tunnel body
            if (ssReadTunnel.GetSize() != nReadSize)
            {
                pPeerNet->HandlePeerError(this);
            }
            else
            {
                if (!mapPeerTunnel[nTunnelId].AddRecvData(ssReadTunnel, ssHisRecv))
                {
                    pPeerNet->HandlePeerError(this);
                    return;
                }
                ssReadTunnel.Clear();
                if (ssHisRecv.GetSize() >= nReadLength)
                {
                    ssHisRecv.TransferStream(ssRecv, nReadLength);
                    if (!fnComplt())
                    {
                        pPeerNet->HandlePeerViolate(this);
                    }
                }
                else
                {
                    pClient->Read(ssReadTunnel, 2, boost::bind(&CPeer::HandleRead, this, _1, nReadLength, fnComplt, 0, 0));
                }
            }
        }
    }
    else
    {
        pPeerNet->HandlePeerError(this);
    }
}

void CPeer::HandleWriten(std::size_t nTransferred)
{
    if (nTransferred != 0)
    {
        nTimeSend = GetTime();

        ssWrite.Clear();
        Write();
        pPeerNet->HandlePeerWriten(this);
    }
    else
    {
        pPeerNet->HandlePeerError(this);
    }
}

} // namespace mtbase
