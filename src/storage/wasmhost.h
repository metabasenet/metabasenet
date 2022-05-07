// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_WASMHOST_H
#define STORAGE_WASMHOST_H

#include <evmc/evmc.hpp>
#include <map>

#include "destination.h"
#include "hcode.h"
#include "transaction.h"
#include "uint256.h"
#include "wasmbase.h"

namespace metabasenet
{
namespace storage
{

class CWasmHost : public evmc::Host
{
private:
    evmc_tx_context tx_context;
    CHostBaseDB& dbHost;

public:
    std::map<uint256, evmc::bytes32> cacheKv;

    evmc::address logAddr;
    std::vector<evmc::bytes32> vLogTopics;
    std::vector<uint8_t> vLogData;

public:
    CWasmHost(const evmc_tx_context& _tx_context, CHostBaseDB& dbHostIn);

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

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_WASMHOST_H
