// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_WALLET_H
#define METABASENET_WALLET_H

#include <boost/thread/thread.hpp>
#include <set>

#include "base.h"
#include "util.h"
#include "walletdb.h"

namespace metabasenet
{

using namespace mtbase;

class CWalletKeyStore
{
public:
    CWalletKeyStore()
      : nTimerId(0), nAutoLockTime(-1), nAutoDelayTime(0) {}
    CWalletKeyStore(const crypto::CKey& keyIn)
      : key(keyIn), nTimerId(0), nAutoLockTime(-1), nAutoDelayTime(0) {}
    virtual ~CWalletKeyStore() {}

public:
    crypto::CKey key;
    uint32 nTimerId;
    int64 nAutoLockTime;
    int64 nAutoDelayTime;
};

class CWalletFork
{
public:
    CWalletFork(const uint256& hashParentIn = uint64(0), int nOriginHeightIn = -1, bool fIsolatedIn = true)
      : hashParent(hashParentIn), nOriginHeight(nOriginHeightIn), fIsolated(fIsolatedIn)
    {
    }
    void InsertSubline(int nHeight, const uint256& hashSubline)
    {
        mapSubline.insert(std::make_pair(nHeight, hashSubline));
    }

public:
    uint256 hashParent;
    int nOriginHeight;
    bool fIsolated;
    std::multimap<int, uint256> mapSubline;
};

class CWallet : public IWallet
{
public:
    CWallet();
    ~CWallet();
    /* Key store */
    boost::optional<std::string> AddKey(const crypto::CKey& key) override;
    boost::optional<std::string> RemoveKey(const CDestination& dest) override;
    bool GetPubkey(const CDestination& dest, crypto::CPubKey& pubkey) const override;
    bool LoadKey(const crypto::CKey& key);
    std::size_t GetPubKeys(std::set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) const override;
    bool HaveKey(const CDestination& dest, const int32 nVersion) const override;
    bool Export(const CDestination& dest, std::vector<unsigned char>& vchKey) const override;
    bool Import(const std::vector<unsigned char>& vchKey, crypto::CPubKey& pubkey) override;

    bool Encrypt(const CDestination& dest, const crypto::CCryptoString& strPassphrase, const crypto::CCryptoString& strCurrentPassphrase) override;
    bool GetKeyStatus(const CDestination& dest, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic) const override;
    bool IsLocked(const CDestination& dest) const override;
    bool Lock(const CDestination& dest) override;
    bool Unlock(const CDestination& dest, const crypto::CCryptoString& strPassphrase, int64 nTimeout) override;
    void AutoLock(uint32 nTimerId, const CDestination& dest);
    bool Sign(const CDestination& dest, const uint256& hash, std::vector<uint8>& vchSig) override;
    bool SignEthTx(const CDestination& dest, const CEthTxSkeleton& ets, uint256& hashTx, bytes& btEthTxData) override;
    /* Template */
    bool LoadTemplate(CTemplatePtr ptr);
    void GetTemplateIds(std::set<pair<uint8, CTemplateId>>& setTemplateId, const uint64 nPos, const uint64 nCount) const override;
    bool HaveTemplate(const CDestination& dest) const override;
    bool AddTemplate(CTemplatePtr& ptr) override;
    CTemplatePtr GetTemplate(const CDestination& dest) const override;
    bool RemoveTemplate(const CDestination& dest) override;
    /* Destination */
    void GetDestinations(std::set<CDestination>& setDest) override;
    /* Wallet Tx */
    bool SignTransaction(const uint256& hashFork, const uint256& hashSignRefBlock, CTransaction& tx) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    bool LoadDB();
    void Clear();
    bool InsertKey(const crypto::CKey& key);
    bool IsPubkeyAddress(const CDestination& dest);
    bool IsTemplateAddress(const CDestination& dest);

    bool SignPubKey(const CDestination& dest, const uint256& hash, std::vector<uint8>& vchSig, std::set<CDestination>& setSignedAddress);
    //bool SignMultiPubKey(const std::set<crypto::CPubKey>& setPubKey, const uint256& hash, std::vector<uint8>& vchSig, std::set<crypto::CPubKey>& setSignedKey);
    bool SignDestination(const uint256& hashFork, const uint256& hashSignRefBlock, CTransaction& tx, std::set<CDestination>& setSignedAddress);
    void UpdateAutoLock(const std::set<CDestination>& setSignedAddress);

protected:
    storage::CWalletDB dbWallet;
    IBlockChain* pBlockChain;
    mutable boost::shared_mutex rwKeyStore;
    std::map<CDestination, CWalletKeyStore> mapKeyStore;
    std::map<CDestination, CTemplatePtr> mapTemplatePtr;
};

// dummy wallet for on wallet server
class CDummyWallet : public IWallet
{
public:
    CDummyWallet() {}
    ~CDummyWallet() {}
    /* Key store */
    virtual boost::optional<std::string> AddKey(const crypto::CKey& key) override
    {
        return std::string();
    }
    virtual boost::optional<std::string> RemoveKey(const CDestination& dest) override
    {
        return std::string();
    }
    virtual bool GetPubkey(const CDestination& dest, crypto::CPubKey& pubkey) const override
    {
        return true;
    }
    virtual std::size_t GetPubKeys(std::set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) const override
    {
        return 0;
    }
    virtual bool HaveKey(const CDestination& dest, const int32 nVersion) const override
    {
        return false;
    }
    virtual bool Export(const CDestination& dest,
                        std::vector<unsigned char>& vchKey) const override
    {
        return false;
    }
    virtual bool Import(const std::vector<unsigned char>& vchKey,
                        crypto::CPubKey& pubkey) override
    {
        return false;
    }
    virtual bool Encrypt(
        const CDestination& dest, const crypto::CCryptoString& strPassphrase,
        const crypto::CCryptoString& strCurrentPassphrase) override
    {
        return false;
    }
    virtual bool GetKeyStatus(const CDestination& dest, int& nVersion,
                              bool& fLocked, int64& nAutoLockTime, bool& fPublic) const override
    {
        return false;
    }
    virtual bool IsLocked(const CDestination& dest) const override
    {
        return false;
    }
    virtual bool Lock(const CDestination& dest) override
    {
        return false;
    }
    virtual bool Unlock(const CDestination& dest,
                        const crypto::CCryptoString& strPassphrase,
                        int64 nTimeout) override
    {
        return false;
    }
    virtual bool Sign(const CDestination& dest, const uint256& hash, std::vector<uint8>& vchSig) override
    {
        return false;
    }
    virtual bool SignEthTx(const CDestination& dest, const CEthTxSkeleton& ets, uint256& hashTx, bytes& btEthTxData) override
    {
        return false;
    }
    /* Template */
    virtual void GetTemplateIds(std::set<pair<uint8, CTemplateId>>& setTemplateId, const uint64 nPos, const uint64 nCount) const override {}
    virtual bool HaveTemplate(const CDestination& dest) const override
    {
        return false;
    }
    virtual bool AddTemplate(CTemplatePtr& ptr) override
    {
        return false;
    }
    virtual CTemplatePtr GetTemplate(const CDestination& dest) const override
    {
        return nullptr;
    }
    virtual bool RemoveTemplate(const CDestination& dest) override
    {
        return false;
    }
    /* Destination */
    virtual void GetDestinations(std::set<CDestination>& setDest) override
    {
    }
    /* Wallet Tx */
    virtual bool SignTransaction(const uint256& hashFork, const uint256& hashSignRefBlock, CTransaction& tx) override
    {
        return false;
    }
};

} // namespace metabasenet

#endif //METABASENET_WALLET_H
