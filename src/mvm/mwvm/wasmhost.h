// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MVM_WASMHOST_H
#define MVM_WASMHOST_H

#include <map>

#include "destination.h"
#include "evmc/evmc.hpp"
#include "mtbase.h"
#include "transaction.h"
#include "uint256.h"
#include "vmhostface.h"

namespace metabasenet
{

namespace mvm
{

class CWasmHost : public evmc::Host
{
private:
    evmc_tx_context tx_context;
    CVmHostFaceDB& dbHost;

public:
    std::map<uint256, evmc::bytes32> cacheKv;

    // evmc::address logAddr;
    // std::vector<evmc::bytes32> vLogTopics;
    // std::vector<uint8_t> vLogData;

    std::vector<CTransactionLogs> vLogs;

public:
    static CDestination AddressToDestination(const evmc::address& addr);
    static evmc::address DestinationToAddress(const CDestination& dest);

public:
    CWasmHost(const evmc_tx_context& _tx_context, CVmHostFaceDB& dbHostIn);

    bool account_exists(const evmc::address& addr) const noexcept final;

    evmc::bytes32 get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept final;

    evmc_storage_status set_storage(const evmc::address& addr,
                                    const evmc::bytes32& key,
                                    const evmc::bytes32& value) noexcept final;

    evmc::uint256be get_balance(const evmc::address& addr) const noexcept final;

    size_t get_code_size(const evmc::address& addr) const noexcept;

    evmc::bytes32 get_code_hash(const evmc::address& addr) const noexcept;

    size_t copy_code(const evmc::address& addr,
                     size_t code_offset,
                     uint8_t* buffer_data,
                     size_t buffer_size) const noexcept;

    void selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept;

    evmc::result call(const evmc_message& msg) noexcept final;

    evmc_tx_context get_tx_context() const noexcept final;

    evmc::bytes32 get_block_hash(int64_t number) const noexcept final;

    void emit_log(const evmc::address& addr,
                  const uint8_t* data,
                  size_t data_size,
                  const evmc::bytes32 topics[],
                  size_t topics_count) noexcept final;

protected:
    evmc::result CreateContract3(const evmc_message& msg);
    evmc::result CallContract(const evmc_message& msg);
};

} // namespace mvm
} // namespace metabasenet

#endif // MVM_WASMHOST_H
