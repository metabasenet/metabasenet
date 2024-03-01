// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "delegateverify.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace delegate
{

//////////////////////////////
// CDelegateVerify

CDelegateVerify::CDelegateVerify(const map<CDestination, size_t>& mapWeight,
                                 const map<CDestination, vector<unsigned char>>& mapEnrollData)
{
    Enroll(mapWeight, mapEnrollData);
}

bool CDelegateVerify::VerifyProof(const unsigned char nProofWeight, const uint256& nProofAgreement, const bytes& btProofDelegateData, uint256& nAgreement,
                                  size_t& nWeight, map<CDestination, size_t>& mapBallot, bool fCheckRepeated)
{
    try
    {
        if (nProofWeight == 0 && nProofAgreement == 0)
        {
            return true;
        }
        CBufStream is(btProofDelegateData);
        vector<CDelegateData> vPublish;
        is >> vPublish;
        for (size_t i = 0; i < vPublish.size(); i++)
        {
            const CDelegateData& delegateData = vPublish[i];
            if (!VerifySignature(delegateData) || !witness.Collect(delegateData.nIdentFrom, delegateData.mapShare, fCheckRepeated))
            {
                return false;
            }
        }
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    GetAgreement(nAgreement, nWeight, mapBallot);

    return (nAgreement == nProofAgreement);
}

} // namespace delegate
} // namespace metabasenet
