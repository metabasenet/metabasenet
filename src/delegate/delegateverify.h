// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DELEGATE_DELEGATEVERIFY_H
#define DELEGATE_DELEGATEVERIFY_H

#include "delegatevote.h"

namespace metabasenet
{
namespace delegate
{

class CDelegateVerify : public CDelegateVote
{
public:
    CDelegateVerify(const std::map<CDestination, std::size_t>& mapWeight,
                    const std::map<CDestination, std::vector<unsigned char>>& mapEnrollData);
    bool VerifyProof(const std::vector<unsigned char>& vchProof, uint256& nAgreement,
                     std::size_t& nWeight, std::map<CDestination, std::size_t>& mapBallot, bool fCheckRepeated = true);
};

} // namespace delegate
} // namespace metabasenet

#endif //DELEGATE_DELEGATE_VERIFY_H
