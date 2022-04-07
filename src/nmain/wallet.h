// Copyright (c) 2021-2022 The MetabaseNet developers
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

using namespace hnbase;

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
    boost::optional<std::string> RemoveKey(const crypto::CPubKey& pubkey) override;
    bool LoadKey(const crypto::CKey& key);
    std::size_t GetPubKeys(std::set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) const override;
    bool Have(const crypto::CPubKey& pubkey, const int32 nVersion = -1) const override;
    bool Export(const crypto::CPubKey& pubkey, std::vector<unsigned char>& vchKey) const override;
    bool Import(const std::vector<unsigned char>& vchKey, crypto::CPubKey& pubkey) override;

    bool Encrypt(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase,
                 const crypto::CCryptoString& strCurrentPassphrase) override;
    bool GetKeyStatus(const crypto::CPubKey& pubkey, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic) const override;
    bool IsLocked(const crypto::CPubKey& pubkey) const override;
    bool Lock(const crypto::CPubKey& pubkey) override;
    bool Unlock(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase, int64 nTimeout) override;
    void AutoLock(uint32 nTimerId, const crypto::CPubKey& pubkey);
    bool Sign(const crypto::CPubKey& pubkey, const uint256& hash, std::vector<uint8>& vchSig) override;
    bool AesEncrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vMessage, std::vector<uint8>& vCiphertext) override;
    bool AesDecrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vCiphertext, std::vector<uint8>& vMessage) override;
    /* Template */
    bool LoadTemplate(CTemplatePtr ptr);
    void GetTemplateIds(std::set<CTemplateId>& setTemplateId, const uint64 nPos, const uint64 nCount) const override;
    bool Have(const CTemplateId& tid) const override;
    bool AddTemplate(CTemplatePtr& ptr) override;
    CTemplatePtr GetTemplate(const CTemplateId& tid) const override;
    bool RemoveTemplate(const CTemplateId& tid) override;
    /* Destination */
    void GetDestinations(std::set<CDestination>& setDest) override;
    /* Wallet Tx */
    bool SignTransaction(const CDestination& destIn, CTransaction& tx, const uint256& hashFork, const int32 nForkHeight) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    bool LoadDB();
    void Clear();
    bool InsertKey(const crypto::CKey& key);

    bool SignPubKey(const crypto::CPubKey& pubkey, const uint256& hash, std::vector<uint8>& vchSig,
                    std::set<crypto::CPubKey>& setSignedKey);
    bool SignMultiPubKey(const std::set<crypto::CPubKey>& setPubKey, const uint256& hash,
                         std::vector<uint8>& vchSig, std::set<crypto::CPubKey>& setSignedKey);
    bool SignDestination(const CDestination& destIn, const CTransaction& tx, const uint256& hash,
                         std::vector<uint8>& vchSig, const uint256& hashFork, const int32 nForkHeight,
                         std::set<crypto::CPubKey>& setSignedKey);
    void UpdateAutoLock(const std::set<crypto::CPubKey>& setSignedKey);

protected:
    storage::CWalletDB dbWallet;
    IBlockChain* pBlockChain;
    mutable boost::shared_mutex rwKeyStore;
    std::map<crypto::CPubKey, CWalletKeyStore> mapKeyStore;
    std::map<CTemplateId, CTemplatePtr> mapTemplatePtr;
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
    virtual boost::optional<std::string> RemoveKey(const crypto::CPubKey& pubkey) override
    {
        return std::string();
    }
    virtual std::size_t GetPubKeys(std::set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) const override
    {
        return 0;
    }
    virtual bool Have(const crypto::CPubKey& pubkey, const int32 nVersion = -1) const override
    {
        return false;
    }
    virtual bool Export(const crypto::CPubKey& pubkey,
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
        const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase,
        const crypto::CCryptoString& strCurrentPassphrase) override
    {
        return false;
    }
    virtual bool GetKeyStatus(const crypto::CPubKey& pubkey, int& nVersion,
                              bool& fLocked, int64& nAutoLockTime, bool& fPublic) const override
    {
        return false;
    }
    virtual bool IsLocked(const crypto::CPubKey& pubkey) const override
    {
        return false;
    }
    virtual bool Lock(const crypto::CPubKey& pubkey) override
    {
        return false;
    }
    virtual bool Unlock(const crypto::CPubKey& pubkey,
                        const crypto::CCryptoString& strPassphrase,
                        int64 nTimeout) override
    {
        return false;
    }
    virtual bool Sign(const crypto::CPubKey& pubkey, const uint256& hash,
                      std::vector<uint8>& vchSig) override
    {
        return false;
    }
    virtual bool AesEncrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vMessage, std::vector<uint8>& vCiphertext) override
    {
        return false;
    }
    virtual bool AesDecrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vCiphertext, std::vector<uint8>& vMessage) override
    {
        return false;
    }
    /* Template */
    virtual void GetTemplateIds(std::set<CTemplateId>& setTemplateId, const uint64 nPos, const uint64 nCount) const override {}
    virtual bool Have(const CTemplateId& tid) const override
    {
        return false;
    }
    virtual bool AddTemplate(CTemplatePtr& ptr) override
    {
        return false;
    }
    virtual CTemplatePtr GetTemplate(const CTemplateId& tid) const override
    {
        return nullptr;
    }
    virtual bool RemoveTemplate(const CTemplateId& tid) override
    {
        return false;
    }
    /* Destination */
    virtual void GetDestinations(std::set<CDestination>& setDest) override
    {
    }
    /* Wallet Tx */
    virtual bool SignTransaction(const CDestination& destIn, CTransaction& tx,
                                 const uint256& hashFork, const int32 nForkHeight) override
    {
        return false;
    }
};

} // namespace metabasenet

#endif //METABASENET_WALLET_H
