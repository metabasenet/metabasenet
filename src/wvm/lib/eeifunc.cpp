// SPDX-License-Identifier: Apache-2.0
#include "eeifunc.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "Keccak.h"
#include "common/hexstr.h"
#include "crypto.h"
#include "util.h"

bool islog = false;

namespace SSVM
{
namespace Host
{

bool islog = false;
Expect<uint32_t> EEICall::body(Runtime::Instance::MemoryInstance* MemInst,
                               uint64_t Gas, uint32_t AddressOffset,
                               uint32_t ValueOffset, uint32_t DataOffset,
                               uint32_t DataLength)
{
    //if (islog)
    //std::cout << "EEICall" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Prepare call message.
    evmc_address Addr;
    if (auto Res = loadAddress(*MemInst, AddressOffset))
    {
        Addr = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }
    evmc::uint256be Val;
    if (auto Res = loadUInt(*MemInst, ValueOffset, 32))
    {
        Val = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }

    if (DataLength > 0)
    {
        evmc_message CallMsg = {
            .kind = evmc_call_kind::EVMC_CALL,
            .flags = Env.getFlag() & evmc_flags::EVMC_STATIC,
            .depth = static_cast<int32_t>(Env.getDepth() + 1),
            .gas = static_cast<int64_t>(std::min(Gas, getMaxCallGas())),
            .destination = Addr,
            .sender = Env.getCallerEVMC(),
            .input_data = nullptr,
            .input_size = 0,
            .value = Val,
            .create2_salt = evmc::bytes32()
        };
        return callContract(*MemInst, CallMsg, DataOffset, DataLength);
    }
    else
    {
        evmc_message CallMsg = {
            .kind = evmc_call_kind::EVMC_CALL,
            .flags = Env.getFlag() & evmc_flags::EVMC_STATIC,
            .depth = static_cast<int32_t>(Env.getDepth() + 1),
            .gas = static_cast<int64_t>(std::min(Gas, getMaxCallGas())),
            .destination = Addr,
            .sender = Env.getAddressEVMC(),
            .input_data = nullptr,
            .input_size = 0,
            .value = Val,
            .create2_salt = evmc::bytes32()
        };
        return callContract(*MemInst, CallMsg, DataOffset, DataLength);
    }
}

Expect<uint32_t> EEICallCode::body(Runtime::Instance::MemoryInstance* MemInst,
                                   uint64_t Gas, uint32_t AddressOffset,
                                   uint32_t ValueOffset, uint32_t DataOffset,
                                   uint32_t DataLength)
{
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Prepare call message.
    evmc_address Addr;
    if (auto Res = loadAddress(*MemInst, AddressOffset))
    {
        Addr = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }
    evmc::uint256be Val;
    if (auto Res = loadUInt(*MemInst, ValueOffset, 32))
    {
        Val = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }
    evmc_message CallMsg = {
        .kind = evmc_call_kind::EVMC_CALLCODE,
        .flags = Env.getFlag() & evmc_flags::EVMC_STATIC,
        .depth = static_cast<int32_t>(Env.getDepth() + 1),
        .gas = static_cast<int64_t>(std::min(Gas, getMaxCallGas())),
        .destination = Addr,
        .sender = Env.getAddressEVMC(),
        .input_data = nullptr,
        .input_size = 0,
        .value = Val,
        .create2_salt = evmc::bytes32()
    };

    return callContract(*MemInst, CallMsg, DataOffset, DataLength);
}

Expect<void> EEICallDataCopy::body(Runtime::Instance::MemoryInstance* MemInst,
                                   uint32_t ResultOffset, uint32_t DataOffset,
                                   uint32_t Length)
{
    //if (islog)
    //    std::cout << "EEICallDataCopy" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Take additional gas of copy.
    if (auto Res = addCopyCost(Length); !Res)
    {
        return Unexpect(Res);
    }
    return MemInst->setBytes(Env.getCallData(), ResultOffset, DataOffset, Length);
}

Expect<uint32_t>
EEICallDelegate::body(Runtime::Instance::MemoryInstance* MemInst, uint64_t Gas,
                      uint32_t AddressOffset, uint32_t DataOffset,
                      uint32_t DataLength)
{
    //if (islog)
    //std::cout << "EEICallDelegate" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Prepare call message.
    evmc_address Addr;
    if (auto Res = loadAddress(*MemInst, AddressOffset))
    {
        Addr = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }

    evmc_message CallMsg = {
        .kind = evmc_call_kind::EVMC_DELEGATECALL,
        .flags = Env.getFlag() & evmc_flags::EVMC_STATIC,
        .depth = static_cast<int32_t>(Env.getDepth() + 1),
        .gas = static_cast<int64_t>(std::min(Gas, getMaxCallGas())),
        .destination = Addr,
        .sender = Env.getAddressEVMC(),
        .input_data = nullptr,
        .input_size = 0,
        .value = Env.getCallValueEVMC(),
        .create2_salt = evmc::bytes32()
    };

    return callContract(*MemInst, CallMsg, DataOffset, DataLength);
}

Expect<uint32_t> EEICallStatic::body(Runtime::Instance::MemoryInstance* MemInst,
                                     uint64_t Gas, uint32_t AddressOffset,
                                     uint32_t DataOffset, uint32_t DataLength)
{
    //if (islog)
    //    std::cout << "EEICallStatic" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Load address and convert to uint256.
    evmc_address Addr;
    if (auto Res = loadAddress(*MemInst, AddressOffset))
    {
        Addr = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }
    boost::multiprecision::uint256_t AddrNum = 0;
    for (auto& I : Addr.bytes)
    {
        AddrNum <<= 8;
        AddrNum |= I;
    }
    if (AddrNum == 1)
    {
        std::vector<unsigned char> data;
        if (auto Res = MemInst->getBytes(DataOffset, DataLength))
        {
            data = std::vector<uint8_t>((*Res).begin(), (*Res).end());
        }
        else
        {
            return Unexpect(Res);
        }
        uint256 hash;
        uint256 address;
        memcpy(hash.begin(), data.data(), 32);
        memcpy(address.begin(), data.data() + 32, 32);
        std::vector<uint8_t> sig(data.data() + 64, data.data() + 128);

        if (metabasenet::crypto::CryptoVerify(address, hash, sig))
        {
            Env.getReturnData() = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
        }
        else
        {
            Env.getReturnData() = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        }
        return UINT32_C(0);
    }
    else if (AddrNum == 2)
    {
        // sha256
        // Check data copy cost.
        if (auto Res = addCopyCost(DataLength); !Res)
        {
            return Unexpect(Res);
        }
        /// Prepare call data.
        std::vector<unsigned char> Data;
        if (auto Res = MemInst->getBytes(DataOffset, DataLength))
        {
            Data = std::vector<uint8_t>((*Res).begin(), (*Res).end());
        }
        else
        {
            return Unexpect(Res);
        }
        uint256 hash = metabasenet::crypto::CryptoSHA256(Data.data(), Data.size());
        Env.getReturnData().assign(hash.begin(), hash.end());
        return UINT32_C(0);
    }
    else if (AddrNum == 3)
    {
        /// Check data copy cost.
        if (auto Res = addCopyCost(DataLength); !Res)
        {
            return Unexpect(Res);
        }

        /// Prepare call data.
        std::vector<unsigned char> Data;
        if (auto Res = MemInst->getBytes(DataOffset, DataLength))
        {
            Data = std::vector<uint8_t>((*Res).begin(), (*Res).end());
        }
        else
        {
            return Unexpect(Res);
        }

        /// Run Keccak
        Keccak K(256);
        for (auto& I : Data)
        {
            K.addData(I);
        }
        Env.getReturnData() = K.digest();
        return UINT32_C(0);
    }
    else if (AddrNum == 4) // create3
    {
        evmc_address contract_addr;
        if (auto Res = loadAddress(*MemInst, DataOffset))
        {
            contract_addr = std::move(*Res);
        }
        else
        {
            return Unexpect(Res);
        }
        //std::cout << "contract_addr:" << mtbase::ToHexString(contract_addr.bytes, sizeof(contract_addr.bytes)) << std::endl;

        evmc::bytes32 salt;
        if (auto Res = loadBytes32(*MemInst, DataOffset + 32))
        {
            salt = std::move(*Res);
        }
        else
        {
            return Unexpect(Res);
        }

        std::vector<uint8_t> vParam;
        if (DataLength > 64)
        {
            if (auto Res = MemInst->getBytes(DataOffset + 64, DataLength - 64))
            {
                vParam = std::vector<uint8_t>(Res->begin(), Res->end());
            }
            else
            {
                return Unexpect(Res);
            }
        }

        evmc_message CreateMsg = { .kind = evmc_call_kind::EVMC_CREATE3,
                                   .flags = 0,
                                   .depth = static_cast<int32_t>(Env.getDepth() + 1),
                                   .gas = static_cast<int64_t>(std::min(Gas, getMaxCallGas())),
                                   .destination = contract_addr,
                                   .sender = Env.getAddressEVMC(),
                                   .input_data = vParam.data(),
                                   .input_size = vParam.size(),
                                   .value = {},
                                   .create2_salt = salt };

        return callContract(*MemInst, CreateMsg, DataOffset, 0);
    }
    else
    {
        return UINT32_C(1);
        /*
    /// Prepare call message.
    evmc_message CallMsg = {
        .kind = evmc_call_kind::EVMC_CALL,
        .flags = evmc_flags::EVMC_STATIC,
        .depth = static_cast<int32_t>(Env.getDepth() + 1),
        .gas = static_cast<int64_t>(std::min(Gas, getMaxCallGas())),
        .destination = Addr,
        .sender = Env.getAddressEVMC(),
        .input_data = nullptr,
        .input_size = 0,
        .value = {}};

    return callContract(*MemInst, CallMsg, DataOffset, DataLength);
    */
    }
}

Expect<void> EEICodeCopy::body(Runtime::Instance::MemoryInstance* MemInst,
                               uint32_t ResultOffset, uint32_t CodeOffset,
                               uint32_t Length)
{
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Take additional gas of copy.
    if (auto Res = addCopyCost(Length); !Res)
    {
        return Unexpect(Res);
    }
    return MemInst->setBytes(Env.getCode(), ResultOffset, CodeOffset, Length);
}

Expect<uint32_t> EEICreate::body(Runtime::Instance::MemoryInstance* MemInst,
                                 uint32_t ValueOffset, uint32_t DataOffset,
                                 uint32_t DataLength, uint32_t ResultOffset)
{
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Prepare creation message.
    evmc::uint256be Val;
    if (auto Res = loadUInt(*MemInst, ValueOffset, 32))
    {
        Val = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }
    evmc_message CreateMsg = { .kind = evmc_call_kind::EVMC_CREATE,
                               .flags = 0,
                               .depth = static_cast<int32_t>(Env.getDepth() + 1),
                               .gas = static_cast<int64_t>(getMaxCallGas()),
                               .destination = {},
                               .sender = Env.getAddressEVMC(),
                               .input_data = nullptr,
                               .input_size = 0,
                               .value = Val,
                               .create2_salt = evmc::bytes32() };

    /// Return: Result(i32)
    return callContract(*MemInst, CreateMsg, DataOffset, DataLength,
                        ResultOffset);
}

Expect<uint32_t> EEICreate2::body(Runtime::Instance::MemoryInstance* MemInst,
                                  uint32_t ValueOffset, uint32_t DataOffset,
                                  uint32_t DataLength, uint32_t SaltOffset,
                                  uint32_t ResultOffset)
{
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Prepare creation message.
    evmc::uint256be Val;
    if (auto Res = loadUInt(*MemInst, ValueOffset, 32))
    {
        Val = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }

    evmc::uint256be Salt;
    if (auto Res = loadUInt(*MemInst, SaltOffset, 32))
    {
        Salt = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }

    evmc_message CreateMsg = { .kind = evmc_call_kind::EVMC_CREATE2,
                               .flags = 0,
                               .depth = static_cast<int32_t>(Env.getDepth() + 1),
                               .gas = static_cast<int64_t>(getMaxCallGas()),
                               .destination = {},
                               .sender = Env.getAddressEVMC(),
                               .input_data = nullptr,
                               .input_size = 0,
                               .value = Val,
                               .create2_salt = Salt };

    /// Return: Result(i32)
    return callContract(*MemInst, CreateMsg, DataOffset, DataLength,
                        ResultOffset);
}

Expect<void>
EEIExternalCodeCopy::body(Runtime::Instance::MemoryInstance* MemInst,
                          uint32_t AddressOffset, uint32_t ResultOffset,
                          uint32_t CodeOffset, uint32_t Length)
{
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Take additional gas of copy.
    if (auto Res = addCopyCost(Length); !Res)
    {
        return Unexpect(Res);
    }
    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Get address from memory instance.
    evmc_address Addr;
    if (auto Res = loadAddress(*MemInst, AddressOffset))
    {
        Addr = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }

    /// Copy code to vector.
    std::vector<uint8_t> Buffer(Length, 0);
    size_t Copied = Cxt.copy_code(Addr, CodeOffset, Buffer.data(), Length);
    if (Length != Copied)
    {
        return Unexpect(ErrCode::MemoryOutOfBounds);
    }

    /// Store to memory instance.
    return MemInst->setBytes(Buffer, ResultOffset, 0, Length);
}

Expect<void> EEIFinish::body(Runtime::Instance::MemoryInstance* MemInst,
                             uint32_t DataOffset, uint32_t DataLength)
{
    //if (islog)
    //    std::cout << "EEIFinish" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    Env.getReturnData().clear();
    if (auto Res = MemInst->getBytes(DataOffset, DataLength))
    {
        Env.getReturnData() = std::vector<uint8_t>((*Res).begin(), (*Res).end());
    }
    else
    {
        return Unexpect(Res);
    }
    return Unexpect(ErrCode::Terminated);
}

Expect<void> EEIGetAddress::body(Runtime::Instance::MemoryInstance* MemInst,
                                 uint32_t ResultOffset)
{
    if (islog)
        std::cout << "EEIGetAddress" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    return MemInst->setBytes(Env.getAddress(), ResultOffset, 0, 32);
}

Expect<void>
EEIGetBlockCoinbase::body(Runtime::Instance::MemoryInstance* MemInst,
                          uint32_t ResultOffset)
{
    if (islog)
        std::cout << "EEIGetBlockCoinbase" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Get block coinbase and store bytes32.
    return storeAddress(*MemInst, Cxt.get_tx_context().block_coinbase,
                        ResultOffset);
}

Expect<void>
EEIGetBlockDifficulty::body(Runtime::Instance::MemoryInstance* MemInst,
                            uint32_t ResultOffset)
{
    if (islog)
        std::cout << "EEIGetBlockDifficulty" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Get block difficulty and store uint256 little-endian value.
    return storeUInt(*MemInst, Cxt.get_tx_context().block_difficulty,
                     ResultOffset);
}

Expect<uint64_t>
EEIGetBlockGasLimit::body(Runtime::Instance::MemoryInstance* MemInst)
{
    if (islog)
        std::cout << "EEIGetBlockGasLimit" << std::endl;
    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Return: GasLimit(u64)
    return Cxt.get_tx_context().block_gas_limit;
}

Expect<uint32_t>
EEIGetBlockHash::body(Runtime::Instance::MemoryInstance* MemInst,
                      uint64_t Number, uint32_t ResultOffset)
{
    if (islog)
        std::cout << "EEIGetBlockHash" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Get the block hash value.
    const evmc::bytes32 Hash = Cxt.get_block_hash(Number);

    /// Check is zero.
    if (evmc::is_zero(Hash))
    {
        /// Return: Result(u32)
        return UINT32_C(1);
    }
    else
    {
        /// Store bytes32.
        if (auto Res = storeBytes32(*MemInst, Hash, ResultOffset); !Res)
        {
            return Unexpect(Res);
        }
        /// Return: Result(u32)
        return UINT32_C(0);
    }
}

Expect<uint64_t>
EEIGetBlockNumber::body(Runtime::Instance::MemoryInstance* MemInst)
{
    if (islog)
        std::cout << "EEIGetBlockNumber" << std::endl;
    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Return: BlockNumber(u64)
    return Cxt.get_tx_context().block_number;
}

Expect<uint64_t>
EEIGetBlockTimestamp::body(Runtime::Instance::MemoryInstance* MemInst)
{
    if (islog)
        std::cout << "EEIGetBlockTimestamp" << std::endl;
    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Return: BlockNumber(u64)
    return Cxt.get_tx_context().block_timestamp;
}

Expect<void>
EEIGetChainId::body(Runtime::Instance::MemoryInstance* MemInst, uint32_t ResultOffset)
{
    if (islog)
        std::cout << "EEIGetChainId" << std::endl;
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    evmc::HostContext& Cxt = Env.getEVMCContext();

    evmc::uint256be ChainId = Cxt.get_tx_context().chain_id;

    /// Store uint256 little-endian value.
    return storeUInt(*MemInst, ChainId, ResultOffset, 32);
}

Expect<uint32_t>
EEIGetCallDataSize::body(Runtime::Instance::MemoryInstance* MemInst)
{
    /// Return: Length(u32)
    if (islog)
        std::cout << "EEIGetCallDataSize" << std::endl;
    return Env.getCallData().size();
}

Expect<void> EEIGetCaller::body(Runtime::Instance::MemoryInstance* MemInst,
                                uint32_t ResultOffset)
{

    if (islog)
        std::cout << "EEIGetCaller" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    return MemInst->setBytes(Env.getCaller(), ResultOffset, 0, 32);
}

Expect<void> EEIGetCallValue::body(Runtime::Instance::MemoryInstance* MemInst,
                                   uint32_t ResultOffset)
{

    if (islog)
        std::cout << "EEIGetCallValue" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    return MemInst->setBytes(Env.getCallValue(), ResultOffset, 0, 32);
}

Expect<uint32_t>
EEIGetCodeSize::body(Runtime::Instance::MemoryInstance* MemInst)
{
    /// Return: CodeSize(u32)
    return Env.getCode().size();
}

Expect<void>
EEIGetExternalBalance::body(Runtime::Instance::MemoryInstance* MemInst,
                            uint32_t AddressOffset, uint32_t ResultOffset)
{
    if (islog)
        std::cout << "EEIGetExternalBalance" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Get address from memory instance.
    evmc_address Addr;
    if (auto Res = loadAddress(*MemInst, AddressOffset))
    {
        Addr = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }

    /// Get balance uint256 big-endian value.
    evmc::uint256be Balance = Cxt.get_balance(Addr);

    /// Store uint256 little-endian value.
    return storeUInt(*MemInst, Balance, ResultOffset, 32);
}

Expect<uint32_t>
EEIGetExternalCodeSize::body(Runtime::Instance::MemoryInstance* MemInst,
                             uint32_t AddressOffset)
{
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Get address from memory instance.
    evmc_address Addr;
    if (auto Res = loadAddress(*MemInst, AddressOffset))
    {
        Addr = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }

    /// Return: ExtCodeSize(u32)
    return Cxt.get_code_size(Addr);
}

Expect<uint64_t>
EEIGetGasLeft::body(Runtime::Instance::MemoryInstance* MemInst)
{
    if (islog)
        std::cout << "EEIGetGasLeft" << std::endl;
    return Env.getGasLeft();
}

Expect<uint32_t>
EEIGetReturnDataSize::body(Runtime::Instance::MemoryInstance* MemInst)
{
    /// Return: DataSize(u32)
    if (islog)
        std::cout << "EEIGetReturnDataSize" << std::endl;
    return Env.getReturnData().size();
}

Expect<void> EEIGetTxGasPrice::body(Runtime::Instance::MemoryInstance* MemInst,
                                    uint32_t ResultOffset)
{

    if (islog)
        std::cout << "EEIGetTxGasPrice" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Get tx gas price uint256 big-endian value.
    evmc::uint256be Price = Cxt.get_tx_context().tx_gas_price;

    /// Store uint256 little-endian value.
    return storeUInt(*MemInst, Price, ResultOffset, 32);
}

Expect<void> EEIGetTxOrigin::body(Runtime::Instance::MemoryInstance* MemInst,
                                  uint32_t ResultOffset)
{
    if (islog)
        std::cout << "EEIGetTxOrigin" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Get block coinbase and store bytes20.
    return storeAddress(*MemInst, Cxt.get_tx_context().tx_origin, ResultOffset);
}

Expect<void> EEILog::body(Runtime::Instance::MemoryInstance* MemInst,
                          uint32_t DataOffset, uint32_t DataLength,
                          uint32_t NumberOfTopics, uint32_t Topic1,
                          uint32_t Topic2, uint32_t Topic3, uint32_t Topic4)
{
    if (islog)
        std::cout << "EEILog" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Check number of topics
    if (NumberOfTopics > 4)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Take additional gas of logs.
    uint64_t TakeGas = 375ULL * NumberOfTopics + 8ULL * DataLength;
    if (!Env.consumeGas(TakeGas))
    {
        return Unexpect(ErrCode::CostLimitExceeded);
    }
    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Copy topics to array.
    std::vector<evmc::bytes32> Topics(4, evmc::bytes32());
    if (NumberOfTopics >= 1)
    {
        if (auto Res = loadBytes32(*MemInst, Topic1))
        {
            Topics[0] = std::move(*Res);
        }
        else
        {
            return Unexpect(Res);
        }
    }
    if (NumberOfTopics >= 2)
    {
        if (auto Res = loadBytes32(*MemInst, Topic2))
        {
            Topics[1] = std::move(*Res);
        }
        else
        {
            return Unexpect(Res);
        }
    }
    if (NumberOfTopics >= 3)
    {
        if (auto Res = loadBytes32(*MemInst, Topic3))
        {
            Topics[2] = std::move(*Res);
        }
        else
        {
            return Unexpect(Res);
        }
    }
    if (NumberOfTopics == 4)
    {
        if (auto Res = loadBytes32(*MemInst, Topic4))
        {
            Topics[3] = std::move(*Res);
        }
        else
        {
            return Unexpect(Res);
        }
    }

    /// Load data.
    std::vector<uint8_t> Data;
    if (auto Res = MemInst->getBytes(DataOffset, DataLength))
    {
        Data = std::vector<uint8_t>((*Res).begin(), (*Res).end());
    }
    else
    {
        return Unexpect(Res);
    }

    /// Get address data.
    evmc_address Addr = Env.getAddressEVMC();

    /// Call emit_log.
    Cxt.emit_log(Addr, &Data[0], DataLength, &Topics[0], NumberOfTopics);

    return {};
}

Expect<void> EEIReturnDataCopy::body(Runtime::Instance::MemoryInstance* MemInst,
                                     uint32_t ResultOffset, uint32_t DataOffset,
                                     uint32_t Length)
{
    if (islog)
        std::cout << "EEIReturnDataCopy" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Take additional gas of copy.
    if (auto Res = addCopyCost(Length); !Res)
    {
        return Unexpect(Res);
    }
    return MemInst->setBytes(Env.getReturnData(), ResultOffset, DataOffset,
                             Length);
}

Expect<void> EEIRevert::body(Runtime::Instance::MemoryInstance* MemInst,
                             uint32_t DataOffset, uint32_t DataLength)
{
    /// Check memory instance from module.
    if (islog)
        std::cout << "EEIRevert" << std::endl;
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    Env.getReturnData().clear();
    if (auto Res = MemInst->getBytes(DataOffset, DataLength))
    {
        Env.getReturnData() = std::vector<uint8_t>((*Res).begin(), (*Res).end());
    }
    else
    {
        return Unexpect(Res);
    }
    Env.getIsRevert() = true;
    return Unexpect(ErrCode::Terminated);
}

Expect<void> EEISelfDestruct::body(Runtime::Instance::MemoryInstance* MemInst,
                                   uint32_t AddressOffset)
{
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Get address data.
    evmc_address Addr;
    if (auto Res = loadAddress(*MemInst, AddressOffset))
    {
        Addr = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }
    evmc_address Self = Env.getAddressEVMC();

    /// Take additional gas if call new account.
    if (!Cxt.account_exists(Addr))
    {
        if (!Env.consumeGas(25000ULL))
        {
            return Unexpect(ErrCode::CostLimitExceeded);
        }
    }

    /// Call selfdestruct.
    Cxt.selfdestruct(Self, Addr);
    return Unexpect(ErrCode::Terminated);
}

Expect<void> EEIStorageLoad::body(Runtime::Instance::MemoryInstance* MemInst,
                                  uint32_t PathOffset, uint32_t ValueOffset)
{
    /// Check memory instance from module.
    if (islog)
        std::cout << "EEIStorageLoad" << std::endl;
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Get destination, path, and value data.
    evmc_address Addr = Env.getAddressEVMC();
    evmc::bytes32 Path;
    if (auto Res = loadBytes32(*MemInst, PathOffset))
    {
        Path = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }

    /// Store bytes32 into memory instance.
    return storeBytes32(*MemInst, Cxt.get_storage(Addr, Path), ValueOffset);
}

Expect<void> EEIStorageStore::body(Runtime::Instance::MemoryInstance* MemInst,
                                   uint32_t PathOffset, uint32_t ValueOffset)
{
    if (islog)
        std::cout << "EEIStorageStore" << std::endl;
    /// Check memory instance from module.
    if (MemInst == nullptr)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    evmc::HostContext& Cxt = Env.getEVMCContext();

    /// Static mode cannot store storage
    if (Env.getFlag() & evmc_flags::EVMC_STATIC)
    {
        return Unexpect(ErrCode::ExecutionFailed);
    }

    /// Get destination, path, value data, and current storage value.
    evmc_address Addr = Env.getAddressEVMC();
    evmc::bytes32 Path, Value;
    if (auto Res = loadBytes32(*MemInst, PathOffset))
    {
        Path = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }
    if (auto Res = loadBytes32(*MemInst, ValueOffset))
    {
        Value = std::move(*Res);
    }
    else
    {
        return Unexpect(Res);
    }
    evmc::bytes32 CurrValue = Cxt.get_storage(Addr, Path);

    /// Take additional gas if create case.
    if (evmc::is_zero(CurrValue) && !evmc::is_zero(Value))
    {
        if (!Env.consumeGas(15000ULL))
        {
            return Unexpect(ErrCode::CostLimitExceeded);
        }
    }

    /// Store value into storage.
    Cxt.set_storage(Addr, Path, Value);
    return {};
}

Expect<void> EEIUseGas::body(Runtime::Instance::MemoryInstance* MemInst,
                             uint64_t Amount)
{
    if (islog)
        std::cout << "EEIUseGas" << std::endl;
    /// Take gas.
    if (!Env.consumeGas(Amount))
    {
        return Unexpect(ErrCode::CostLimitExceeded);
    }
    return {};
}

} // namespace Host
} // namespace SSVM
