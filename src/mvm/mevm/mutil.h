// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MVM_MUTIL_H
#define MVM_MUTIL_H

#include <evmc/evmc.hpp>

#include "../meth/libdevcore/Address.h"
#include "destination.h"
#include "uint256.h"

using namespace std;
using namespace dev;

namespace metabasenet
{

namespace mvm
{

inline evmc_uint256be uint256ToEvmC(const uint256& _n)
{
    evmc_uint256be out;
    _n.ToBigEndian(&(out.bytes[0]), sizeof(out.bytes));
    return out;
}

inline evmc_address addressToEvmC(const CDestination& address)
{
    evmc_address out;
    address.ToBigEndian(&(out.bytes[0]), sizeof(out.bytes));
    return out;
}

inline uint256 uint256FromEvmC(evmc_uint256be const& _n)
{
    uint256 out;
    out.FromBigEndian(&(_n.bytes[0]), sizeof(_n.bytes));
    return out;
}

inline CDestination destFromAddress(Address const& _address)
{
    return CDestination(_address.data(), _address.size);
}

inline Address addressFromDest(CDestination const& _address)
{
    return Address(bytesConstRef(_address.begin(), _address.size()));
}

inline Address addressFromEvmC(evmc::address const& _addr)
{
    return reinterpret_cast<Address const&>(_addr);
}

/////////////////////////////////
Address createContractAddress(const Address& _sender, const u256& _nonce);
Address createContractAddress(const Address& _sender, const bytes& _code, const u256& _salt);

bool FetchContractCreateCode(const bytes& btSrcCreateCode, const bytes& btRunCode, bytes& dstCreateCode);

std::string GetStatusInfo(const int64 status_code);

} // namespace mvm
} // namespace metabasenet

#endif // MVM_MUTIL_H
