// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet.h"

#include "param.h"
#include "template/delegate.h"
#include "template/fork.h"
#include "template/mint.h"
#include "template/vote.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{

//////////////////////////////
// CDBAddressWalker

class CDBAddrWalker : public storage::CWalletDBAddrWalker
{
public:
    CDBAddrWalker(CWallet* pWalletIn)
      : pWallet(pWalletIn) {}
    bool WalkPubkey(const crypto::CPubKey& pubkey, int version, const crypto::CCryptoCipher& cipher) override
    {
        crypto::CKey key;
        key.Load(pubkey, version, cipher);
        return pWallet->LoadKey(key);
    }
    bool WalkTemplate(const CDestination& dest, const std::vector<unsigned char>& vchData) override
    {
        CTemplatePtr ptr = CTemplate::Import(vchData);
        if (ptr)
        {
            return pWallet->LoadTemplate(ptr);
        }
        return false;
    }

protected:
    CWallet* pWallet;
};

//////////////////////////////
// CWallet

CWallet::CWallet()
{
    pBlockChain = nullptr;
}

CWallet::~CWallet()
{
}

bool CWallet::HandleInitialize()
{
    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain");
        return false;
    }
    return true;
}

void CWallet::HandleDeinitialize()
{
    pBlockChain = nullptr;
}

bool CWallet::HandleInvoke()
{
    if (!dbWallet.Initialize(Config()->pathData / "wallet"))
    {
        Error("Failed to initialize wallet database");
        return false;
    }

    if (!LoadDB())
    {
        Error("Failed to load wallet database");
        return false;
    }

    return true;
}

void CWallet::HandleHalt()
{
    dbWallet.Deinitialize();
    Clear();
}

boost::optional<std::string> CWallet::AddKey(const crypto::CKey& key)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    if (!InsertKey(key))
    {
        Warn("AddKey : invalid or duplicated key");
        return std::string("AddKey : invalid or duplicated key");
    }

    if (!dbWallet.UpdateKey(key.GetPubKey(), key.GetVersion(), key.GetCipher()))
    {
        mapKeyStore.erase(CDestination(key.GetPubKey()));
        Warn("AddKey : failed to save key");
        return std::string("AddKey : failed to save key");
    }
    return boost::optional<std::string>{};
}

boost::optional<std::string> CWallet::RemoveKey(const CDestination& dest)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    auto it = mapKeyStore.find(dest);
    if (it == mapKeyStore.end())
    {
        Warn("RemoveKey: failed to find key");
        return std::string("RemoveKey: failed to find key");
    }

    if (!dbWallet.RemoveKey(dest))
    {
        Warn("RemoveKey: failed to remove key from dbWallet");
        return std::string("RemoveKey: failed to remove key from dbWallet");
    }

    mapKeyStore.erase(it);

    return boost::optional<std::string>{};
}

bool CWallet::GetPubkey(const CDestination& dest, crypto::CPubKey& pubkey) const
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    auto it = mapKeyStore.find(dest);
    if (it == mapKeyStore.end())
    {
        return false;
    }
    pubkey = it->second.key.GetPubKey();
    return true;
}

bool CWallet::LoadKey(const crypto::CKey& key)
{
    if (!InsertKey(key))
    {
        Error("LoadKey : invalid or duplicated key");
        return false;
    }
    return true;
}

size_t CWallet::GetPubKeys(set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);

    uint64 nCurPos = 0;
    for (auto it = mapKeyStore.begin(); it != mapKeyStore.end(); ++it, nCurPos++)
    {
        if (nCount == 0 || nCurPos >= nPos)
        {
            setPubKey.insert(it->second.key.GetPubKey());
            if (nCount > 0 && setPubKey.size() >= nCount)
            {
                break;
            }
        }
    }

    return mapKeyStore.size();
}

bool CWallet::HaveKey(const CDestination& dest, const int32 nVersion) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    auto it = mapKeyStore.find(dest);
    if (nVersion < 0)
    {
        return it != mapKeyStore.end();
    }
    else
    {
        return (it != mapKeyStore.end() && it->second.key.GetVersion() == nVersion);
    }
}

bool CWallet::Export(const CDestination& dest, vector<unsigned char>& vchKey) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    auto it = mapKeyStore.find(dest);
    if (it != mapKeyStore.end())
    {
        it->second.key.Save(vchKey);
        return true;
    }
    return false;
}

bool CWallet::Import(const vector<unsigned char>& vchKey, crypto::CPubKey& pubkey)
{
    crypto::CKey key;
    if (!key.Load(vchKey))
    {
        return false;
    }
    pubkey = key.GetPubKey();
    return AddKey(key) ? false : true;
}

bool CWallet::Encrypt(const CDestination& dest, const crypto::CCryptoString& strPassphrase, const crypto::CCryptoString& strCurrentPassphrase)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    auto it = mapKeyStore.find(dest);
    if (it != mapKeyStore.end())
    {
        crypto::CKey& key = it->second.key;
        crypto::CKey keyTemp(key);
        if (!keyTemp.Encrypt(strPassphrase, strCurrentPassphrase))
        {
            Error("AddKey : Encrypt fail");
            return false;
        }
        if (!dbWallet.UpdateKey(key.GetPubKey(), keyTemp.GetVersion(), keyTemp.GetCipher()))
        {
            Error("AddKey : failed to update key");
            return false;
        }
        key.Encrypt(strPassphrase, strCurrentPassphrase);
        key.Lock();
        return true;
    }
    return false;
}

bool CWallet::GetKeyStatus(const CDestination& dest, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    auto it = mapKeyStore.find(dest);
    if (it != mapKeyStore.end())
    {
        const CWalletKeyStore& keystore = (*it).second;
        nVersion = keystore.key.GetVersion();
        fLocked = keystore.key.IsLocked();
        fPublic = keystore.key.IsPubKey();
        nAutoLockTime = (!fLocked && keystore.nAutoLockTime > 0) ? keystore.nAutoLockTime : 0;
        return true;
    }
    return false;
}

bool CWallet::IsLocked(const CDestination& dest) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    auto it = mapKeyStore.find(dest);
    if (it != mapKeyStore.end())
    {
        return it->second.key.IsLocked();
    }
    return false;
}

bool CWallet::Lock(const CDestination& dest)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    auto it = mapKeyStore.find(dest);
    if (it != mapKeyStore.end() && it->second.key.IsPrivKey())
    {
        CWalletKeyStore& keystore = it->second;
        keystore.key.Lock();
        if (keystore.nTimerId)
        {
            CancelTimer(keystore.nTimerId);
            keystore.nTimerId = 0;
            keystore.nAutoLockTime = -1;
            keystore.nAutoDelayTime = 0;
        }
        return true;
    }
    return false;
}

bool CWallet::Unlock(const CDestination& dest, const crypto::CCryptoString& strPassphrase, int64 nTimeout)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    auto it = mapKeyStore.find(dest);
    if (it != mapKeyStore.end() && it->second.key.IsPrivKey())
    {
        CWalletKeyStore& keystore = it->second;
        if (!keystore.key.IsLocked() || !keystore.key.Unlock(strPassphrase))
        {
            return false;
        }

        if (nTimeout > 0)
        {
            keystore.nAutoDelayTime = nTimeout;
            keystore.nAutoLockTime = GetTime() + keystore.nAutoDelayTime;
            keystore.nTimerId = SetTimer(nTimeout * 1000, boost::bind(&CWallet::AutoLock, this, _1, it->first));
        }
        return true;
    }
    return false;
}

void CWallet::AutoLock(uint32 nTimerId, const CDestination& dest)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    auto it = mapKeyStore.find(dest);
    if (it != mapKeyStore.end() && it->second.key.IsPrivKey())
    {
        CWalletKeyStore& keystore = it->second;
        if (keystore.nTimerId == nTimerId)
        {
            keystore.key.Lock();
            keystore.nTimerId = 0;
            keystore.nAutoLockTime = -1;
            keystore.nAutoDelayTime = 0;
        }
    }
}

bool CWallet::Sign(const CDestination& dest, const uint256& hash, vector<uint8>& vchSig)
{
    set<CDestination> setSignedAddress;
    {
        boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);

        auto it = mapKeyStore.find(dest);
        if (it == mapKeyStore.end())
        {
            StdError("CWallet", "Sign: find privkey fail, dest: %s", dest.ToString().c_str());
            return false;
        }
        auto& key = it->second.key;
        if (!key.IsPrivKey())
        {
            StdError("CWallet", "Sign: can't sign tx by non-privkey, dest: %s", dest.ToString().c_str());
            return false;
        }
        if (!key.Sign(hash, vchSig))
        {
            StdError("CWallet", "Sign: sign fail, dest: %s", dest.ToString().c_str());
            return false;
        }

        setSignedAddress.insert(dest);
    }
    UpdateAutoLock(setSignedAddress);
    return true;
}

bool CWallet::SignEthTx(const CDestination& dest, const CEthTxSkeleton& ets, uint256& hashTx, bytes& btEthTxData)
{
    set<CDestination> setSignedAddress;
    {
        boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);

        auto it = mapKeyStore.find(dest);
        if (it == mapKeyStore.end())
        {
            StdError("CWallet", "Get eth tx: find privkey fail, dest: %s", dest.ToString().c_str());
            return false;
        }
        auto& key = it->second.key;
        if (!key.IsPrivKey())
        {
            StdError("CWallet", "Get eth tx: can't sign tx by non-privkey, dest: %s", dest.ToString().c_str());
            return false;
        }
        if (!key.SignEthTx(ets, hashTx, btEthTxData))
        {
            StdError("CWallet", "Get eth tx: get fail, dest: %s", dest.ToString().c_str());
            return false;
        }

        setSignedAddress.insert(dest);
    }
    UpdateAutoLock(setSignedAddress);
    return true;
}

bool CWallet::LoadTemplate(CTemplatePtr ptr)
{
    if (ptr != nullptr)
    {
        return mapTemplatePtr.insert(make_pair(CDestination(ptr->GetTemplateId()), ptr)).second;
    }
    return false;
}

void CWallet::GetTemplateIds(set<pair<uint8, CTemplateId>>& setTemplateId, const uint64 nPos, const uint64 nCount) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);

    uint64 nCurPos = 0;
    for (auto it = mapTemplatePtr.begin(); it != mapTemplatePtr.end(); ++it, nCurPos++)
    {
        if (it->second && (nCount == 0 || nCurPos >= nPos))
        {
            setTemplateId.insert(make_pair(it->second->GetTemplateType(), it->second->GetTemplateId()));
            if (nCount > 0 && setTemplateId.size() >= nCount)
            {
                break;
            }
        }
    }
}

bool CWallet::HaveTemplate(const CDestination& dest) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    return (!!mapTemplatePtr.count(dest));
}

bool CWallet::AddTemplate(CTemplatePtr& ptr)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    if (ptr != nullptr)
    {
        CDestination dest(ptr->GetTemplateId());
        if (mapTemplatePtr.insert(make_pair(dest, ptr)).second)
        {
            return dbWallet.UpdateTemplate(dest, ptr->Export());
        }
    }
    return false;
}

CTemplatePtr CWallet::GetTemplate(const CDestination& dest) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    auto it = mapTemplatePtr.find(dest);
    if (it != mapTemplatePtr.end())
    {
        return it->second;
    }
    return nullptr;
}

bool CWallet::RemoveTemplate(const CDestination& dest)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    auto it = mapTemplatePtr.find(dest);
    if (it != mapTemplatePtr.end())
    {
        if (!dbWallet.RemoveTemplate(dest))
        {
            return false;
        }
        mapTemplatePtr.erase(it);
        return true;
    }
    return false;
}

void CWallet::GetDestinations(set<CDestination>& setDest)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);

    for (auto it = mapKeyStore.begin(); it != mapKeyStore.end(); ++it)
    {
        setDest.insert(it->first);
    }

    for (auto it = mapTemplatePtr.begin(); it != mapTemplatePtr.end(); ++it)
    {
        setDest.insert(it->first);
    }
}

bool CWallet::SignTransaction(const uint256& hashFork, const uint256& hashSignRefBlock, CTransaction& tx)
{
    set<CDestination> setSignedAddress;
    {
        boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
        if (!SignDestination(hashFork, hashSignRefBlock, tx, setSignedAddress))
        {
            Error("Sign transaction: Sign destination fail, from: %s, txid: %s",
                  tx.GetFromAddress().ToString().c_str(), tx.GetHash().GetHex().c_str());
            return false;
        }
    }
    UpdateAutoLock(setSignedAddress);
    return true;
}

bool CWallet::LoadDB()
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);

    CDBAddrWalker walker(this);
    if (!dbWallet.WalkThroughAddress(walker))
    {
        StdLog("CWallet", "LoadDB: WalkThroughAddress fail.");
        return false;
    }
    return true;
}

void CWallet::Clear()
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    mapKeyStore.clear();
    mapTemplatePtr.clear();
}

bool CWallet::InsertKey(const crypto::CKey& key)
{
    if (!key.IsNull())
    {
        CDestination dest(key.GetPubKey());
        auto it = mapKeyStore.find(dest);
        if (it != mapKeyStore.end())
        {
            // privkey update pubkey
            if (it->second.key.IsPubKey() && key.IsPrivKey())
            {
                it->second = CWalletKeyStore(key);
                it->second.key.Lock();
                return true;
            }
            return false;
        }
        else
        {
            auto ret = mapKeyStore.insert(make_pair(dest, CWalletKeyStore(key)));
            ret.first->second.key.Lock();
            return true;
        }
    }
    return false;
}

bool CWallet::IsPubkeyAddress(const CDestination& dest)
{
    auto it = mapKeyStore.find(dest);
    if (it == mapKeyStore.end())
    {
        return false;
    }
    return true;
}

bool CWallet::IsTemplateAddress(const CDestination& dest)
{
    auto it = mapTemplatePtr.find(dest);
    if (it == mapTemplatePtr.end())
    {
        return false;
    }
    return true;
}

bool CWallet::SignPubKey(const CDestination& dest, const uint256& hash, vector<uint8>& vchSig, std::set<CDestination>& setSignedAddress)
{
    auto it = mapKeyStore.find(dest);
    if (it == mapKeyStore.end())
    {
        StdError("CWallet", "Sign PubKey: find privkey fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    auto& key = it->second.key;
    if (!key.IsPrivKey())
    {
        StdError("CWallet", "Sign PubKey: can't sign tx by non-privkey, dest: %s", dest.ToString().c_str());
        return false;
    }

    if (!key.Sign(hash, vchSig))
    {
        StdError("CWallet", "Sign PubKey: sign fail, dest: %s", dest.ToString().c_str());
        return false;
    }

    setSignedAddress.insert(dest);
    return true;
}

// bool CWallet::SignMultiPubKey(const set<crypto::CPubKey>& setPubKey, const uint256& hash, vector<uint8>& vchSig, std::set<crypto::CPubKey>& setSignedKey)
// {
//     bool fSigned = false;
//     for (auto& pubkey : setPubKey)
//     {
//         auto it = mapKeyStore.find(pubkey);
//         if (it != mapKeyStore.end() && it->second.key.IsPrivKey())
//         {
//             fSigned |= it->second.key.MultiSign(setPubKey, hash, vchSig);
//             setSignedKey.insert(pubkey);
//         }
//     }
//     return fSigned;
// }

bool CWallet::SignDestination(const uint256& hashFork, const uint256& hashSignRefBlock, CTransaction& tx, std::set<CDestination>& setSignedAddress)
{
    const CDestination& destIn = tx.GetFromAddress();
    const uint256 hash = tx.GetSignatureHash();

    if (IsPubkeyAddress(destIn))
    {
        bytes btSig;
        if (!SignPubKey(destIn, hash, btSig, setSignedAddress))
        {
            StdError("CWallet", "Sign destination: Pubkey address sign fail, txid: %s, destIn: %s",
                     tx.GetHash().GetHex().c_str(), destIn.ToString().c_str());
            return false;
        }
        tx.SetSignData(btSig);
        return true;
    }
    //if (IsTemplateAddress(destIn))
    //{
    CTemplatePtr ptr = GetTemplate(destIn);
    if (!ptr)
    {
        CAddressContext ctxAddress;
        if (!pBlockChain->RetrieveAddressContext(hashFork, hashSignRefBlock, destIn, ctxAddress))
        {
            StdError("CWallet", "Sign destination: Retrieve Address Context fail, txid: %s, destIn: %s", tx.GetHash().GetHex().c_str(), destIn.ToString().c_str());
            return false;
        }
        if (!ctxAddress.IsTemplate())
        {
            StdError("CWallet", "Sign destination: Address not is template, txid: %s, destIn: %s", tx.GetHash().GetHex().c_str(), destIn.ToString().c_str());
            return false;
        }
        CTemplateAddressContext ctxtTemplate;
        if (!ctxAddress.GetTemplateAddressContext(ctxtTemplate))
        {
            StdError("CWallet", "Sign destination: Get address context fail, txid: %s, destIn: %s", tx.GetHash().GetHex().c_str(), destIn.ToString().c_str());
            return false;
        }
        ptr = CTemplate::Import(ctxtTemplate.btData);
        if (!ptr)
        {
            StdError("CWallet", "Sign destination: Create Template Ptr fail, txid: %s, destIn: %s", tx.GetHash().GetHex().c_str(), destIn.ToString().c_str());
            return false;
        }
    }
    CDestination destSign;
    if (!ptr->GetSignDestination(tx, destSign))
    {
        StdError("CWallet", "Sign destination: Get sign destination fail, txid: %s, destIn: %s",
                 tx.GetHash().GetHex().c_str(), destIn.ToString().c_str());
        return false;
    }
    bytes btSig;
    if (!SignPubKey(destSign, hash, btSig, setSignedAddress))
    {
        StdError("CWallet", "Sign destination: Template address sign fail, destSign: %s, txid: %s, destIn: %s",
                 destSign.ToString().c_str(), tx.GetHash().GetHex().c_str(), destIn.ToString().c_str());
        return false;
    }
    tx.SetSignData(btSig);
    return true;
    // }

    // StdError("CWallet", "Sign destination: destIn type error, txid: %s, destIn: %s",
    //          tx.GetHash().GetHex().c_str(), destIn.ToString().c_str());
    //return false;
}

void CWallet::UpdateAutoLock(const std::set<CDestination>& setSignedAddress)
{
    if (setSignedAddress.empty())
    {
        return;
    }

    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    for (auto& dest : setSignedAddress)
    {
        auto it = mapKeyStore.find(dest);
        if (it != mapKeyStore.end() && it->second.key.IsPrivKey())
        {
            CWalletKeyStore& keystore = (*it).second;
            if (keystore.nAutoDelayTime > 0)
            {
                CancelTimer(keystore.nTimerId);
                keystore.nAutoLockTime = GetTime() + keystore.nAutoDelayTime;
                keystore.nTimerId = SetTimer(keystore.nAutoDelayTime * 1000, boost::bind(&CWallet::AutoLock, this, _1, dest));
            }
        }
    }
}

} // namespace metabasenet
