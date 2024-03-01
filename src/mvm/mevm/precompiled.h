// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MVM_PRECOMPLILED_H
#define MVM_PRECOMPLILED_H

#include <map>

#include "destination.h"
#include "libdevcore/Address.h"
#include "uint256.h"

using namespace dev;

namespace metabasenet
{
namespace mvm
{

///////////////////////////
// CPrecompliled

typedef bool (*fn_precompliled_exec)(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedga);

class CPrecompliled
{
public:
    static bool execute(const Address& _addr, const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas, bool& _result);

protected:
    static bool exec_ecrecover(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas);
    static bool exec_sha256(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas);
    static bool exec_ripemd160(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas);
    static bool exec_identity(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas);
    static bool exec_modexp(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas);
    static bool exec_alt_bn128_G1_add(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas);
    static bool exec_alt_bn128_G1_mul(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas);
    static bool exec_alt_bn128_pairing_product(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas);
    static bool exec_blake2_compression(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas);

protected:
    static inline fn_precompliled_exec get_prec_func(const Address& _address)
    {
        static const std::map<Address, fn_precompliled_exec> mapPrecFunc = {
            { { Address{ 0x1 }, CPrecompliled::exec_ecrecover },
              { Address{ 0x2 }, CPrecompliled::exec_sha256 },
              { Address{ 0x3 }, CPrecompliled::exec_ripemd160 },
              { Address{ 0x4 }, CPrecompliled::exec_identity },
              { Address{ 0x5 }, CPrecompliled::exec_modexp },
              { Address{ 0x6 }, CPrecompliled::exec_alt_bn128_G1_add },
              { Address{ 0x7 }, CPrecompliled::exec_alt_bn128_G1_mul },
              { Address{ 0x8 }, CPrecompliled::exec_alt_bn128_pairing_product },
              { Address{ 0x9 }, CPrecompliled::exec_blake2_compression } }
        };
        auto it = mapPrecFunc.find(_address);
        if (it != mapPrecFunc.end())
        {
            return it->second;
        }
        return nullptr;
    }
};

} // namespace mvm
} // namespace metabasenet

#endif // MVM_PRECOMPLILED_H
