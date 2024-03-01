// clang-format off

#pragma once

//#include "Executive.h"
//#include "State.h"
#include "ExtVMFace.h"

#include <libethcore/Common.h>
//#include <libethcore/SealEngine.h>
#include <libethcore/TransactionBase.h>

#include <functional>
#include <map>

namespace dev
{
namespace eth
{

class CMemAddressStorage
{
public:
    CMemAddressStorage() : m_nonce(0) {}
    CMemAddressStorage(const u256& nonce_in, const u256& amount_in) 
        : m_nonce(nonce_in), m_balance(amount_in) {}

    u256 balance() const { printf("m_balance: %lu", static_cast<uint64_t>(m_balance)); return m_balance; }
    u256 nonce() const { return m_nonce; }
    u256 storageValue(u256 const& _key) const 
    {
        auto it = mapKvStorage.find(_key);
        if (it != mapKvStorage.end())
        {
            return it->second;
        }
        return 0;
    }
    void setStorage(u256 const& _key, u256 const& _value)
    {
        mapKvStorage[_key] = _value;
    }
    u256 originalStorageValue(u256 const& _key) const
    {
        auto it = mapKvOriginalStorage.find(_key);
        if (it != mapKvOriginalStorage.end())
        {
            return it->second;
        }
        return 0;
    }
    bytes const& code()
    {
        return m_code;
    }
    void addBalance(u256 const& _amount)
    {
        m_balance += _amount;
    }

public:
    u256 m_nonce;
    u256 m_balance;
    bytes m_code;
    std::map<u256, u256> mapKvStorage;
    std::map<u256, u256> mapKvOriginalStorage;
};

class CMemState
{
public:
    CMemState() {}

public:
    CMemAddressStorage const* account(Address const& _addr) const
    {
        auto it = mapAddressState.find(_addr);
        if (it != mapAddressState.end())
        {
            return &(it->second);
        }
        return nullptr;
    }

    CMemAddressStorage* account(Address const& _addr)
    {
        auto it = mapAddressState.find(_addr);
        if (it != mapAddressState.end())
        {
            return &(it->second);
        }
        return nullptr;
    }

    u256 balance(Address const& _id) const
    {
        if (auto a = account(_id))
            return a->balance();
        else
            return 0;
    }

    u256 getNonce(Address const& _addr) const
    {
        if (auto a = account(_addr))
            return a->nonce();
        else
            return 0;
    }

    u256 storage(Address const& _id, u256 const& _key) const
    {
        if (auto a = account(_id))
            return a->storageValue(_key);
        else
            return 0;
    }

    void setStorage(Address const& _contract, u256 const& _key, u256 const& _value)
    {
        if (auto a = account(_contract))
            a->setStorage(_key, _value);
    }

    u256 originalStorageValue(Address const& _id, u256 const& _key) const
    {
        if (auto a = account(_id))
            return a->originalStorageValue(_key);
        else
            return 0;
    }

    bytes const& code(Address _a)
    {
        if (auto a = account(_a))
            return a->code();
        else
            return bytes();
    }

    u256 const& requireAccountStartNonce() const
    {
        // if (m_accountStartNonce == Invalid256)
        //     BOOST_THROW_EXCEPTION(InvalidAccountStartNonceInState());
        // return m_accountStartNonce;
        return u256(0);
    }

    void createAccount(Address const& _address, CMemAddressStorage const&& _account)
    {
        mapAddressState.insert(make_pair(_address, _account));
    }

    void addBalance(Address const& _id, u256 const& _amount)
    {
        if (auto a = account(_id))
        {
            // Log empty account being touched. Empty touched accounts are cleared
            // after the transaction, so this event must be also reverted.
            // We only log the first touch (not dirty yet), and only for empty
            // accounts, as other accounts does not matter.
            // TODO: to save space we can combine this event with Balance by having
            //       Balance and Balance+Touch events.
        //    if (!a->isDirty() && a->isEmpty())
        //        m_changeLog.emplace_back(Change::Touch, _id);

            // Increase the account balance. This also is done for value 0 to mark
            // the account as dirty. Dirty account are not removed from the cache
            // and are cleared if empty at the end of the transaction.
            a->addBalance(_amount);
        }
        else
            createAccount(_id, {requireAccountStartNonce(), _amount});

        //if (_amount)
        //    m_changeLog.emplace_back(Change::Balance, _id, _amount);
    }

    void subBalance(Address const& _addr, u256 const& _value)
    {
        if (_value == 0)
            return;

        auto a = account(_addr);
        // if (!a || a->balance() < _value)
        //     // TODO: I expect this never happens.
        //     BOOST_THROW_EXCEPTION(NotEnoughCash());

        // Fall back to addBalance().
        addBalance(_addr, 0 - _value);
    }

    void setBalance(Address const& _addr, u256 const& _value)
    {
        auto a = account(_addr);
        u256 original = a ? a->balance() : 0;

        // Fall back to addBalance().
        addBalance(_addr, _value - original);
    }

public:
    std::map<Address, CMemAddressStorage> mapAddressState;
};

//class SealEngineFace;

/// Externality interface for the Virtual Machine providing access to world state.
class ExtMemVM : public ExtVMFace
{
public:
    /// Full constructor.
    // ExtMemVM(State& _s, EnvInfo const& _envInfo, SealEngineFace const& _sealEngine, Address _myAddress,
    //     Address _caller, Address _origin, u256 _value, u256 _gasPrice, bytesConstRef _data,
    //     bytesConstRef _code, h256 const& _codeHash, u256 const& _version, unsigned _depth,
    //     bool _isCreate, bool _staticCall)
    //   : ExtVMFace(_envInfo, _myAddress, _caller, _origin, _value, _gasPrice, _data, _code.toBytes(),
    //         _codeHash, _version, _depth, _isCreate, _staticCall),
    //     m_s(_s),
    //     m_sealEngine(_sealEngine),
    //     m_evmSchedule(initEvmSchedule(envInfo().number(), _version))
    // {
    //     // Contract: processing account must exist. In case of CALL, the ExtVM
    //     // is created only if an account has code (so exist). In case of CREATE
    //     // the account must be created first.
    //     assert(m_s.addressInUse(_myAddress));
    // }
    
    ExtMemVM(EnvInfo const& _envInfo, Address _myAddress,
        Address _caller, Address _origin, u256 _value, u256 _gasPrice, bytesConstRef _data,
        bytesConstRef _code, h256 const& _codeHash, u256 const& _version, unsigned _depth,
        bool _isCreate, bool _staticCall)
      : ExtVMFace(_envInfo, _myAddress, _caller, _origin, _value, _gasPrice, _data, _code.toBytes(),
            _codeHash, _version, _depth, _isCreate, _staticCall),
        m_evmSchedule(initEvmSchedule(envInfo().number(), _version))
    {
    }

    /// Read storage location.
    u256 store(u256 _n) final { printf("ExtMemVM::store\n"); return m_s.storage(myAddress, _n); }

    /// Write a value in storage.
    void setStore(u256 _n, u256 _v) final { printf("ExtMemVM::setStore\n"); m_s.setStorage(myAddress, _n, _v); }

    /// Read original storage value (before modifications in the current transaction).
    u256 originalStorageValue(u256 const& _key) final
    {
        printf("ExtMemVM::originalStorageValue\n"); 
        return m_s.originalStorageValue(myAddress, _key);
    }

    /// Read address's code.
    bytes const& codeAt(Address _a) final { printf("ExtMemVM::codeAt\n"); return m_s.code(_a); }

    /// @returns the size of the code in  bytes at the given address.
    size_t codeSizeAt(Address _a) final { printf("ExtMemVM::codeSizeAt\n"); return 0; }

    /// @returns the hash of the code at the given address.
    h256 codeHashAt(Address _a) final { printf("ExtMemVM::codeHashAt\n"); return h256(); }

    /// Create a new contract.
    CreateResult create(u256 _endowment, u256& io_gas, bytesConstRef _code, Instruction _op, u256 _salt, OnOpFunc const& _onOp = {}) final
    {
        printf("ExtMemVM::create\n");
        return CreateResult(EVMC_SUCCESS, owning_bytes_ref(), h160());
    }

    /// Create a new message call.
    CallResult call(CallParameters& _params) final
    {
        printf("ExtMemVM::call\n");
        return CallResult(EVMC_SUCCESS, owning_bytes_ref());
    }

    /// Read address's balance.
    u256 balance(Address _a) final { printf("ExtMemVM::balance, address: 0x%s\n", _a.hex().c_str()); return m_s.balance(_a); }

    /// Does the account exist?
    bool exists(Address _a) final
    {
        printf("ExtMemVM::exists\n");
        return !!m_s.account(_a);
    }

    /// Selfdestruct the associated contract to the given address.
    void selfdestruct(Address _a) final
    {
        printf("ExtMemVM::selfdestruct\n");
        // Why transfer is not used here? That caused a consensus issue before (see Quirk #2 in
        // http://martin.swende.se/blog/Ethereum_quirks_and_vulns.html). There is one test case
        // witnessing the current consensus
        // 'GeneralStateTests/stSystemOperationsTest/suicideSendEtherPostDeath.json'.
        m_s.addBalance(_a, m_s.balance(myAddress));
        m_s.setBalance(myAddress, 0);
        ExtVMFace::selfdestruct(_a);
    }

    /// Return the EVM gas-price schedule for this execution context.
    EVMSchedule const& evmSchedule() const final { return m_evmSchedule; }

    //State const& state() const { return m_s; }

    /// Hash of a block if within the last 256 blocks, or h256() otherwise.
    h256 blockHash(u256 _number) final
    {
        printf("ExtMemVM::blockHash\n");
        return h256(32);
    }

private:
    EVMSchedule const& initEvmSchedule(int64_t _blockNumber, u256 const& _version) const
    {
        // If _version is latest for the block, select corresponding latest schedule.
        // Otherwise run with the latest schedule known to correspond to the _version.
        // EVMSchedule const& currentBlockSchedule = m_sealEngine.evmSchedule(_blockNumber);
        // if (currentBlockSchedule.accountVersion == _version)
        //     return currentBlockSchedule;
        // else
            return latestScheduleForAccountVersion(_version);
    }

private:
    CMemState m_s; 
    //SealEngineFace const& m_sealEngine;
    EVMSchedule const& m_evmSchedule;
};

}
}

// clang-format on
