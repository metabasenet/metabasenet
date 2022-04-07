// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transaction.h"

#include "hnbase.h"

using namespace std;
using namespace hnbase;
using namespace metabasenet::crypto;

//////////////////////////////
// CTxContractData

bool CTxContractData::PacketCompress(const uint8 nMuxTypeIn, const std::string& strTypeIn, const std::string& strNameIn, const CDestination& destCodeOwnerIn,
                                     const std::string& strDescribeIn, const bytes& btCodeIn, const bytes& btSourceIn)
{
    if (nMuxTypeIn <= CF_MIN || nMuxTypeIn >= CF_MAX)
    {
        return false;
    }

    nMuxType = nMuxTypeIn;
    strType = strTypeIn;
    strName = strNameIn;
    destCodeOwner = destCodeOwnerIn;
    nCompressDescribe = 0;
    if (!strDescribeIn.empty())
    {
        if (strDescribeIn.size() < 128)
        {
            btDescribe.assign(strDescribeIn.begin(), strDescribeIn.end());
        }
        else
        {
            bytes btSrc(strDescribeIn.begin(), strDescribeIn.end());
            if (!BtCompress(btSrc, btDescribe))
            {
                return false;
            }
            nCompressDescribe = 1;
        }
    }
    nCompressCode = 0;
    if (!btCodeIn.empty())
    {
        if (nMuxTypeIn == CF_CREATE || nMuxTypeIn == CF_UPCODE)
        {
            if (!BtCompress(btCodeIn, btCode))
            {
                return false;
            }
            nCompressCode = 1;
        }
        else
        {
            btCode = btCodeIn;
        }
    }
    nCompressSource = 0;
    if (!btSourceIn.empty())
    {
        if (!BtCompress(btSourceIn, btSource))
        {
            return false;
        }
        nCompressSource = 1;
    }
    return true;
}

void CTxContractData::SetSetupHash(const uint256& hashContractIn)
{
    btCode.assign(hashContractIn.begin(), hashContractIn.end());
    nCompressCode = 0;
    nMuxType = CF_SETUP;
}

uint256 CTxContractData::GetSourceCodeHash() const
{
    if (btSource.empty())
    {
        return 0;
    }
    if (nCompressSource != 0)
    {
        bytes btDst;
        if (!BtUncompress(btSource, btDst))
        {
            return 0;
        }
        return metabasenet::crypto::CryptoHash(btDst.data(), btDst.size());
    }
    return metabasenet::crypto::CryptoHash(btSource.data(), btSource.size());
}

uint256 CTxContractData::GetWasmCreateCodeHash() const
{
    if (btCode.empty())
    {
        return 0;
    }
    if (nMuxType == CF_SETUP)
    {
        return uint256(btCode);
    }
    else
    {
        if (nCompressCode != 0)
        {
            bytes btDst;
            if (!BtUncompress(btCode, btDst))
            {
                return 0;
            }
            return metabasenet::crypto::CryptoHash(btDst.data(), btDst.size());
        }
        return metabasenet::crypto::CryptoHash(btCode.data(), btCode.size());
    }
}

std::string CTxContractData::GetDescribe() const
{
    if (nCompressDescribe != 0 && !btDescribe.empty())
    {
        bytes btDst;
        if (!BtUncompress(btDescribe, btDst))
        {
            return std::string();
        }
        return std::string(btDst.begin(), btDst.end());
    }
    return std::string(btDescribe.begin(), btDescribe.end());
}

bytes CTxContractData::GetCode() const
{
    if (nCompressCode != 0 && !btCode.empty())
    {
        bytes btDst;
        if (!BtUncompress(btCode, btDst))
        {
            return bytes();
        }
        return btDst;
    }
    return btCode;
}

bytes CTxContractData::GetSource() const
{
    if (nCompressSource != 0 && !btSource.empty())
    {
        bytes btDst;
        if (!BtUncompress(btSource, btDst))
        {
            return bytes();
        }
        return btDst;
    }
    return btSource;
}

bool CTxContractData::UncompressDescribe()
{
    if (nCompressDescribe != 0 && !btDescribe.empty())
    {
        bytes btDst;
        if (!BtUncompress(btDescribe, btDst))
        {
            return false;
        }
        btDescribe.clear();
        btDescribe.assign(btDst.begin(), btDst.end());
        nCompressDescribe = 0;
    }
    return true;
}

bool CTxContractData::UncompressCode()
{
    if (nCompressCode != 0 && !btCode.empty())
    {
        bytes btDst;
        if (!BtUncompress(btCode, btDst))
        {
            return false;
        }
        btCode.clear();
        btCode.assign(btDst.begin(), btDst.end());
        nCompressCode = 0;
    }
    return true;
}

bool CTxContractData::UncompressSource()
{
    if (nCompressSource != 0 && !btSource.empty())
    {
        bytes btDst;
        if (!BtUncompress(btSource, btDst))
        {
            return false;
        }
        btSource.clear();
        btSource.assign(btDst.begin(), btDst.end());
        nCompressSource = 0;
    }
    return true;
}

bool CTxContractData::UncompressAll()
{
    if (!UncompressDescribe())
    {
        return false;
    }
    if (!UncompressCode())
    {
        return false;
    }
    if (!UncompressSource())
    {
        return false;
    }
    return true;
}
