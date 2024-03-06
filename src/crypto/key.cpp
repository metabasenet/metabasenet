// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key.h"

#include "util.h"

using namespace mtbase;

namespace metabasenet
{
namespace crypto
{

//////////////////////////////
// CPubKey
CPubKey::CPubKey()
{
}

CPubKey::CPubKey(const uint512& pubkey)
  : uint512(pubkey)
{
}

uint160 CPubKey::ToAddress() const
{
    return CryptoGetPubkeyAddress(static_cast<uint512>(*this));
}

bool CPubKey::Verify(const uint256& hash, const std::vector<uint8>& vchSig) const
{
    return CryptoVerify(*this, hash, vchSig);
}

//////////////////////////////
// CKey

CKey::CKey()
{
    nVersion = INIT;
    pCryptoKey = NormalAlloc<CCryptoKey>();
    if (!pCryptoKey)
    {
        throw CCryptoError("CKey : Failed to alloc memory");
    }
    pCryptoKey->secret = 0;
    pCryptoKey->pubkey = 0;
}

CKey::CKey(const CKey& key)
{
    pCryptoKey = key.IsLocked() ? NormalAlloc<CCryptoKey>() : CryptoAlloc<CCryptoKey>();
    if (!pCryptoKey)
    {
        throw CCryptoError("CKey : Failed to alloc memory");
    }

    nVersion = key.nVersion;
    *pCryptoKey = *key.pCryptoKey;
    cipher = key.cipher;
}

CKey& CKey::operator=(const CKey& key)
{
    nVersion = key.nVersion;
    cipher = key.cipher;

    if (IsLocked())
    {
        if (!key.IsLocked())
        {
            NormalFree(pCryptoKey);
            pCryptoKey = CryptoAlloc<CCryptoKey>();
        }
    }
    else
    {
        if (key.IsLocked())
        {
            CryptoFree(pCryptoKey);
            pCryptoKey = NormalAlloc<CCryptoKey>();
        }
    }

    *pCryptoKey = *key.pCryptoKey;

    return *this;
}

CKey::~CKey()
{
    if (IsLocked())
    {
        NormalFree(pCryptoKey);
    }
    else
    {
        CryptoFree(pCryptoKey);
    }
}

uint32 CKey::GetVersion() const
{
    return nVersion;
}

bool CKey::IsNull() const
{
    return pCryptoKey->pubkey.IsNull();
}

bool CKey::IsLocked() const
{
    return (pCryptoKey->secret == 0);
}

bool CKey::IsPrivKey() const
{
    return nVersion == PRIVATE_KEY;
}

bool CKey::IsPubKey() const
{
    return nVersion == PUBLIC_KEY;
}

bool CKey::Renew()
{
    if (IsLocked())
    {
        NormalFree(pCryptoKey);
        pCryptoKey = CryptoAlloc<CCryptoKey>();
    }

    return (!CryptoMakeNewKey(*pCryptoKey).IsNull() && UpdateCipher());
}

void CKey::Load(const CPubKey& pubkeyIn, const uint32 nVersionIn, const CCryptoCipher& cipherIn)
{
    pCryptoKey->pubkey = pubkeyIn;
    pCryptoKey->secret = 0;
    nVersion = nVersionIn;
    cipher = cipherIn;
}

bool CKey::Load(const std::vector<unsigned char>& vchKey)
{
    CPubKey pubkey;
    uint32 version;
    CCryptoCipher cipherNew;
    uint256 check;

    mtbase::CBufStream is(vchKey);
    try
    {
        bytes btEncrypted;
        is >> pubkey >> version >> btEncrypted >> cipherNew.nonce >> check;
        if (btEncrypted.size() != 48)
        {
            return false;
        }
        std::copy(btEncrypted.begin(), btEncrypted.end(), cipherNew.encrypted);
    }
    catch (const std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    if (CryptoHash(&vchKey[0], vchKey.size() - check.size()) != check)
    {
        return false;
    }
    Load(pubkey, version, cipherNew);
    return true;
}

void CKey::Save(CPubKey& pubkeyRet, uint32& nVersionRet, CCryptoCipher& cipherRet) const
{
    pubkeyRet = pCryptoKey->pubkey;
    nVersionRet = nVersion;
    cipherRet = cipher;
}

void CKey::Save(std::vector<unsigned char>& vchKey) const
{
    bytes btEncrypted;
    btEncrypted.assign(GetCipher().encrypted, GetCipher().encrypted + 48);

    mtbase::CBufStream os;
    os << GetPubKey() << GetVersion() << btEncrypted << GetCipher().nonce;
    vchKey.clear();
    os.GetData(vchKey);

    uint256 check = CryptoHash(&vchKey[0], vchKey.size());
    os << check;
    vchKey.clear();
    os.GetData(vchKey);
}

bool CKey::SetSecret(const CCryptoKeyData& vchSecret)
{
    if (vchSecret.size() != 32)
    {
        return false;
    }

    if (IsLocked())
    {
        NormalFree(pCryptoKey);
        pCryptoKey = CryptoAlloc<CCryptoKey>();
    }

    return (!CryptoImportKey(*pCryptoKey, *((uint256*)&vchSecret[0])).IsNull() && UpdateCipher());
}

bool CKey::GetSecret(CCryptoKeyData& vchSecret) const
{
    if (!IsNull() && !IsLocked())
    {
        vchSecret.assign(pCryptoKey->secret.begin(), pCryptoKey->secret.end());
        return true;
    }
    return false;
}

const CPubKey CKey::GetPubKey() const
{
    return CPubKey(pCryptoKey->pubkey);
}

const CCryptoCipher& CKey::GetCipher() const
{
    return cipher;
}

bool CKey::Sign(const uint256& hash, std::vector<uint8>& vchSig) const
{
    if (IsNull())
    {
        StdError("CKey", "Sign: pubkey is null");
        return false;
    }
    if (IsLocked())
    {
        StdError("CKey", "Sign: pubkey is locked");
        return false;
    }
    CryptoSign(*pCryptoKey, hash, vchSig);
    return true;
}

bool CKey::Encrypt(const CCryptoString& strPassphrase,
                   const CCryptoString& strCurrentPassphrase)
{
    if (!IsLocked())
    {
        Lock();
    }
    if (Unlock(strCurrentPassphrase))
    {
        return UpdateCipher(1, strPassphrase);
    }
    return false;
}

void CKey::Lock()
{
    if (!IsLocked())
    {
        CryptoToNormalAlloc();
    }
    pCryptoKey->secret = 0;
}

bool CKey::Unlock(const CCryptoString& strPassphrase)
{
    if (IsLocked())
    {
        NormalToCryptoAlloc();
    }

    try
    {
        bool isSuccess = CryptoDecryptSecret(nVersion, strPassphrase, cipher, *pCryptoKey);
        if (!isSuccess)
        {
            // Restore origin state
            if (IsLocked())
            {
                CryptoToNormalAlloc();
            }
        }
        return isSuccess;
    }
    catch (std::exception& e)
    {
        if (IsLocked())
        {
            CryptoToNormalAlloc();
        }
        StdError(__PRETTY_FUNCTION__, e.what());
    }
    return false;
}

bool CKey::UpdateCipher(uint32 nVersionIn, const CCryptoString& strPassphrase)
{
    try
    {
        CCryptoCipher cipherNew;
        if (CryptoEncryptSecret(nVersionIn, strPassphrase, *pCryptoKey, cipherNew))
        {
            nVersion = nVersionIn;
            cipher = cipherNew;
            return true;
        }
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
    }
    return false;
}

void CKey::NormalToCryptoAlloc()
{
    auto pTempCryptoKey = CryptoAlloc<CCryptoKey>();
    *pTempCryptoKey = *pCryptoKey;
    NormalFree(pCryptoKey);
    pCryptoKey = pTempCryptoKey;
}

void CKey::CryptoToNormalAlloc()
{
    auto pTempCryptoKey = NormalAlloc<CCryptoKey>();
    *pTempCryptoKey = *pCryptoKey;
    CryptoFree(pCryptoKey);
    pCryptoKey = pTempCryptoKey;
}

bool CKey::SignEthTx(const CEthTxSkeleton& ets, uint256& hashTx, bytes& btEthTxData)
{
    if (IsNull())
    {
        StdError("CKey", "Get eth tx: pubkey is null");
        return false;
    }
    if (IsLocked())
    {
        StdError("CKey", "Get eth tx: pubkey is locked");
        return false;
    }
    return GetEthTxData(pCryptoKey->secret, ets, hashTx, btEthTxData);
}

bool CKey::GetBlsKey(CCryptoBlsKey& keyBls)
{
    if (IsNull())
    {
        StdError("CKey", "Get bls key: pubkey is null");
        return false;
    }
    if (IsLocked())
    {
        StdError("CKey", "Get bls key: pubkey is locked");
        return false;
    }
    return CryptoBlsMakeNewKey(keyBls, pCryptoKey->secret);
}

} // namespace crypto
} // namespace metabasenet
