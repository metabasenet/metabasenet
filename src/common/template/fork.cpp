// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "fork.h"

#include "destination.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "util.h"

using namespace std;
using namespace hcode;

//////////////////////////////
// CTemplateFork

CTemplateFork::CTemplateFork(const CDestination& destRedeemIn, const uint256& hashForkIn)
  : CTemplate(TEMPLATE_FORK), destRedeem(destRedeemIn), hashFork(hashForkIn)
{
}

CTemplateFork* CTemplateFork::clone() const
{
    return new CTemplateFork(*this);
}

void CTemplateFork::GetTemplateData(metabasenet::rpc::CTemplateResponse& obj) const
{
    obj.fork.strFork = hashFork.GetHex();
    obj.fork.strRedeem = destRedeem.ToString();
}

uint256 CTemplateFork::LockedCoin(const int32 nHeight)
{
    double d = pow(MORTGAGE_DECAY_QUANTITY, nHeight / MORTGAGE_DECAY_CYCLE);
    uint64 u = (uint64)(d * 1000000);
    return (MORTGAGE_BASE * uint256(u) / uint256(1000000));
}

bool CTemplateFork::ValidateParam() const
{
    if (!IsTxSpendable(destRedeem))
    {
        return false;
    }

    if (!hashFork)
    {
        return false;
    }

    return true;
}

bool CTemplateFork::SetTemplateData(const vector<uint8>& vchDataIn)
{
    CBufStream is(vchDataIn);
    try
    {
        is >> destRedeem >> hashFork;
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    return true;
}

bool CTemplateFork::SetTemplateData(const metabasenet::rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_FORK))
    {
        return false;
    }

    if (!destRedeem.ParseString(obj.fork.strRedeem))
    {
        return false;
    }
    if (!destRedeem.IsPubKey())
    {
        return false;
    }

    if (hashFork.SetHex(obj.fork.strFork) != obj.fork.strFork.size())
    {
        return false;
    }

    return true;
}

void CTemplateFork::BuildTemplateData()
{
    vchData.clear();
    CBufStream os;
    os << destRedeem << hashFork;
    os.GetData(vchData);
}

bool CTemplateFork::GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, CDestination& destSign) const
{
    destSign = destRedeem;
    return true;
}

bool CTemplateFork::VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo,
                                      const vector<uint8>& vchSig, const int32 nForkHeight) const
{
    return destRedeem.VerifyTxSignature(hash, nTxType, destTo, bytes(), vchSig, nForkHeight);
}
