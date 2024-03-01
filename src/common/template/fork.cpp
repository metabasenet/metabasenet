// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "fork.h"

#include "destination.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "util.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{

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

void CTemplateFork::GetTemplateData(rpc::CTemplateResponse& obj) const
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
    if (destRedeem.IsNull())
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
    // CBufStream is(vchDataIn);
    // try
    // {
    //     is >> destRedeem >> hashFork;
    // }
    // catch (exception& e)
    // {
    //     StdError(__PRETTY_FUNCTION__, e.what());
    //     return false;
    // }

    if (vchDataIn.size() != (destRedeem.size() + hashFork.size()))
    {
        return false;
    }
    const uint8* p = vchDataIn.data();
    destRedeem.SetBytes(p, destRedeem.size());
    p += destRedeem.size();
    hashFork.SetBytes(p, hashFork.size());
    p += hashFork.size();
    return true;
}

bool CTemplateFork::SetTemplateData(const rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_FORK))
    {
        return false;
    }

    destRedeem.ParseString(obj.fork.strRedeem);
    if (destRedeem.IsNull())
    {
        return false;
    }

    hashFork.SetHex(obj.fork.strFork);
    if (!hashFork)
    {
        return false;
    }
    return true;
}

void CTemplateFork::BuildTemplateData()
{
    vchData.clear();
    vchData.reserve(destRedeem.size() + hashFork.size());
    vchData.insert(vchData.end(), (char*)(destRedeem.begin()), (char*)(destRedeem.end()));
    vchData.insert(vchData.end(), (char*)(hashFork.begin()), (char*)(hashFork.end()));

    // vchData.clear();
    // CBufStream os;
    // os << destRedeem << hashFork;
    // os.GetData(vchData);
}

bool CTemplateFork::GetSignDestination(const CTransaction& tx, CDestination& destSign) const
{
    destSign = destRedeem;
    return true;
}

} // namespace metabasenet
