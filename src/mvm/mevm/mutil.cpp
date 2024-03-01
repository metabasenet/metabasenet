// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mutil.h"

#include "../meth/libdevcore/RLP.h"
#include "../meth/libdevcore/SHA3.h"

using namespace std;
using namespace dev;

namespace metabasenet
{
namespace mvm
{

/////////////////////////////////
Address createContractAddress(const Address& _sender, const u256& _nonce)
{
    return right160(sha3(rlpList(_sender, _nonce)));
}

Address createContractAddress(const Address& _sender, const bytes& _code, const u256& _salt)
{
    //keccak256( 0xff ++ address ++ salt ++ keccak256(init_code))[12:]
    return right160(sha3(bytes{ 0xFF } + _sender.asBytes() + toBigEndian(_salt) + sha3(_code)));
}

bool FetchContractCreateCode(const bytes& btSrcCreateCode, const bytes& btRunCode, bytes& dstCreateCode)
{
    const size_t nAuxDataSize = 43;
    if (btSrcCreateCode.size() >= nAuxDataSize)
    {
        auto p1 = btRunCode.data() + btRunCode.size() - nAuxDataSize;
        auto p2 = btSrcCreateCode.data() + btSrcCreateCode.size() - nAuxDataSize;
        while (p2 >= btSrcCreateCode.data())
        {
            if (memcmp(p1, p2, nAuxDataSize) == 0)
            {
                if (p2 > btSrcCreateCode.data())
                {
                    dstCreateCode.assign(btSrcCreateCode.data(), p2 + nAuxDataSize);
                }
                return true;
            }
            p2 -= 32;
        }
    }
    return false;
}

std::string GetStatusInfo(const int64 status_code)
{
    string strRet;
    switch (status_code)
    {
    case EVMC_SUCCESS:
        strRet = "Success";
        break;
    case EVMC_FAILURE:
        strRet = "Generic execution failure";
        break;
    case EVMC_REVERT:
        strRet = "Execution terminated with REVERT opcode";
        break;
    case EVMC_OUT_OF_GAS:
        strRet = "The execution has run out of gas";
        break;
    case EVMC_INVALID_INSTRUCTION:
        strRet = "The designated INVALID instruction has been hit during execution";
        break;
    case EVMC_UNDEFINED_INSTRUCTION:
        strRet = "An undefined instruction has been encountered";
        break;
    case EVMC_STACK_OVERFLOW:
        strRet = "The execution has attempted to put more items on the EVM stack than the specified limit";
        break;
    case EVMC_STACK_UNDERFLOW:
        strRet = "Execution of an opcode has required more items on the EVM stack";
        break;
    case EVMC_BAD_JUMP_DESTINATION:
        strRet = "Execution has violated the jump destination restrictions";
        break;
    case EVMC_INVALID_MEMORY_ACCESS:
        strRet = "Tried to read outside memory bounds";
        break;
    case EVMC_CALL_DEPTH_EXCEEDED:
        strRet = "Call depth has exceeded the limit (if any)";
        break;
    case EVMC_STATIC_MODE_VIOLATION:
        strRet = "Tried to execute an operation which is restricted in static mode";
        break;
    case EVMC_PRECOMPILE_FAILURE:
        strRet = "A call to a precompiled or system contract has ended with a failure";
        break;
    case EVMC_CONTRACT_VALIDATION_FAILURE:
        strRet = "Contract validation has failed (e.g. due to EVM 1.5 jump validity";
        break;
    case EVMC_ARGUMENT_OUT_OF_RANGE:
        strRet = "An argument to a state accessing method has a value outside of the accepted range of values";
        break;
    case EVMC_WASM_UNREACHABLE_INSTRUCTION:
        strRet = "A WebAssembly unreachable instruction has been hit during execution";
        break;
    case EVMC_WASM_TRAP:
        strRet = "A WebAssembly trap has been hit during execution. This can be for many reasons, including division by zero, validation errors";
        break;
    case EVMC_INTERNAL_ERROR:
        strRet = "EVM implementation generic internal error";
        break;
    case EVMC_REJECTED:
        strRet = "The execution of the given code and/or message has been rejected by the EVM implementation";
        break;
    case EVMC_OUT_OF_MEMORY:
        strRet = "The VM failed to allocate the amount of memory needed for execution";
        break;
    default:
        strRet = "Other error";
        break;
    }
    return strRet;
}

} // namespace mvm
} // namespace metabasenet
