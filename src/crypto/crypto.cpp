// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "crypto.h"

#include <iostream>
#include <sodium.h>

#include "bls.hpp"
#include "curve25519/curve25519.h"
#include "devcommon/util.h"
#include "keccak/Keccak.h"
#include "libdevcrypto/Common.h"
#include "libethcore/TransactionBase.h"
#include "util.h"

using namespace std;
using namespace bls;
using namespace mtbase;

namespace metabasenet
{
namespace crypto
{

//////////////////////////////
// CCryptoSodiumInitializer()
class CCryptoSodiumInitializer
{
public:
    CCryptoSodiumInitializer()
    {
        if (sodium_init() < 0)
        {
            throw CCryptoError("CCryptoSodiumInitializer : Failed to initialize libsodium");
        }
    }
    ~CCryptoSodiumInitializer()
    {
    }
};

static CCryptoSodiumInitializer _CCryptoSodiumInitializer;

//////////////////////////////
// Secure memory
void* CryptoAlloc(const size_t size)
{
    return sodium_malloc(size);
}

void CryptoFree(void* ptr)
{
    sodium_free(ptr);
}

void* NormalAlloc(const std::size_t size)
{
    return malloc(size);
}

void NormalFree(void* ptr)
{
    free(ptr);
}

//////////////////////////////
// Heap memory lock

void CryptoMLock(void* const addr, const size_t len)
{
    if (sodium_mlock(addr, len) < 0)
    {
        throw CCryptoError("CryptoMLock : Failed to mlock");
    }
}

void CryptoMUnlock(void* const addr, const size_t len)
{
    if (sodium_munlock(addr, len) < 0)
    {
        throw CCryptoError("CryptoMUnlock : Failed to munlock");
    }
}

//////////////////////////////
// Rand

uint32 CryptoGetRand32()
{
    return randombytes_random();
}

uint64 CryptoGetRand64()
{
    return (randombytes_random() | (((uint64)randombytes_random()) << 32));
}

void CryptoGetRand256(uint256& u)
{
    randombytes_buf(&u, 32);
}

//////////////////////////////
// Hash

uint256 CryptoHash(const void* msg, size_t len)
{
    uint256 hash;
    crypto_generichash_blake2b((uint8*)&hash, sizeof(hash), (const uint8*)msg, len, nullptr, 0);
    return hash;
}

uint256 CryptoHash(const uint256& h1, const uint256& h2)
{
    uint256 hash;
    crypto_generichash_blake2b_state state;
    crypto_generichash_blake2b_init(&state, nullptr, 0, sizeof(hash));
    crypto_generichash_blake2b_update(&state, h1.begin(), sizeof(h1));
    crypto_generichash_blake2b_update(&state, h2.begin(), sizeof(h2));
    crypto_generichash_blake2b_final(&state, hash.begin(), sizeof(hash));
    return hash;
}

uint256 CryptoPowHash(const void* msg, size_t len)
{
    return CryptoHash(msg, len);
}

//////////////////////////////
// SHA256

uint256 CryptoSHA256(const void* msg, size_t len)
{
    uint256 hash;
    crypto_hash_sha256((uint8*)&hash, (const uint8*)msg, len);
    return hash;
}

//////////////////////////////
// Keccak

bytes CryptoKeccak(const void* msg, size_t len)
{
    if (msg == nullptr || len == 0)
    {
        return {};
    }
    Keccak k(256);
    k.addData((const uint8*)msg, 0, len);
    return k.digest();
}

uint256 CryptoKeccakHash(const void* msg, size_t len)
{
    bytes data = CryptoKeccak(msg, len);
    uint256 hash;
    if (!data.empty())
    {
        hash.SetBytes(data);
    }
    return hash;
}

uint256 CryptoKeccakHash(const bytes& data)
{
    return CryptoKeccakHash(data.data(), data.size());
}

bytes CryptoKeccakSign(const void* msg, size_t len)
{
    auto cak = CryptoKeccak(msg, len);
    if (cak.size() < 4)
    {
        return {};
    }
    return bytes(cak.begin(), cak.begin() + 4);
}

bytes CryptoKeccakSign(const std::string& strMsg)
{
    return CryptoKeccakSign(strMsg.data(), strMsg.size());
}

//////////////////////////////
// Sign & verify

uint512 CryptoMakeNewKey(CCryptoKey& key)
{
    uint8 pkbuf[32];
    uint8 keybuf[64];
    while (crypto_sign_ed25519_keypair(&(pkbuf[0]), &(keybuf[0])) == 0)
    {
        int count = 0;
        const uint8* p = &(keybuf[0]);
        for (int i = 1; i < 31; ++i)
        {
            if (p[i] != 0x00 && p[i] != 0xFF)
            {
                if (++count >= 4)
                {
                    dev::Secret s(bytes(&(keybuf[0]), &(keybuf[0]) + 32));
                    dev::Public p = dev::toPublic(s);
                    dev::Address a = dev::toAddress(p);
                    if (!a)
                    {
                        break;
                    }

                    memcpy(key.secret.begin(), &(keybuf[0]), std::min((uint32)32, key.secret.size()));
                    memcpy(key.pubkey.begin(), p.data(), std::min((uint32)(p.size), key.pubkey.size()));

                    return key.pubkey;
                }
            }
        }
    }

    return uint64(0);
}

uint512 CryptoImportKey(CCryptoKey& key, const uint256& secret)
{
    uint8 pkbuf[32];
    uint8 keybuf[64];
    if (crypto_sign_ed25519_seed_keypair(&(pkbuf[0]), &(keybuf[0]), secret.begin()) != 0)
    {
        return uint64(0);
    }

    dev::Secret s(bytes(&(keybuf[0]), &(keybuf[0]) + 32));
    dev::Public p = dev::toPublic(s);
    dev::Address a = dev::toAddress(p);
    if (!a)
    {
        return uint64(0);
    }

    memcpy(key.secret.begin(), &(keybuf[0]), std::min((uint32)32, key.secret.size()));
    memcpy(key.pubkey.begin(), p.data(), std::min((uint32)(p.size), key.pubkey.size()));

    return key.pubkey;
}

void CryptoSign(const CCryptoKey& key, const void* md, const size_t len, vector<uint8>& vchSig)
{
    // vchSig.resize(64);
    // uint8 keybuf[64] = { 0 };
    // memcpy(&(keybuf[0]), key.secret.begin(), std::min((uint32)32, key.secret.size()));
    // memcpy(&(keybuf[32]), key.pubkey.begin(), std::min((uint32)32, key.pubkey.size()));
    // crypto_sign_ed25519_detached(&vchSig[0], nullptr, (const uint8*)md, len, &(keybuf[0]));

    CryptoSign(key, CryptoHash(md, len), vchSig);
}

void CryptoSign(const CCryptoKey& key, const uint256& hash, std::vector<uint8>& vchSig)
{
    dev::Secret secret(bytes(key.secret.begin(), key.secret.end()));
    dev::h256 h(bytes(hash.begin(), hash.end()));
    vchSig = dev::sign(secret, h).asBytes();
}

bool CryptoVerify(const uint512& pubkey, const void* md, const size_t len, const vector<uint8>& vchSig)
{
    // return (vchSig.size() == 64
    //         && !crypto_sign_ed25519_verify_detached(&vchSig[0], (const uint8*)md, len, pubkey.begin()));
    //bool dev::verify(Public const& _p, Signature const& _s, h256 const& _hash)

    return CryptoVerify(pubkey, CryptoHash(md, len), vchSig);
}

bool CryptoVerify(const uint512& pubkey, const uint256& hash, const std::vector<uint8>& vchSig)
{
    dev::Signature sig(vchSig);
    dev::h256 h(bytes(hash.begin(), hash.end()));
    dev::Public pub = dev::recover(sig, h);
    return (pubkey == uint512(pub.data(), pub.size));
}

bool CryptoVerify(const uint160& address, const uint256& hash, const std::vector<uint8>& vchSig)
{
    return (address == CryptoAddressBySign(hash, vchSig));
}

bool CryptoVerify(const uint256& address, const uint256& hash, const std::vector<uint8>& vchSig)
{
    return (uint160(address.begin() + 12, uint160::size()) == CryptoAddressBySign(hash, vchSig));
}

uint512 CrytoGetSignPubkey(const uint256& hash, const std::vector<uint8>& vchSig)
{
    dev::Signature sig(vchSig);
    dev::h256 h(bytes(hash.begin(), hash.end()));
    dev::Public pub = dev::recover(sig, h);
    return uint512(pub.data(), pub.size);
}

uint160 CryptoGetEcAddress(const CCryptoKey& key)
{
    bytes btData;
    btData.assign(key.secret.begin(), key.secret.end());
    dev::Secret secret(btData);
    dev::Address a = dev::toAddress(secret);
    return uint160(a.data(), a.size);
}

uint160 CryptoGetPubkeyAddress(const uint512& pubkey)
{
    dev::Public p(bytes(pubkey.begin(), pubkey.end()));
    dev::Address a = dev::toAddress(p);
    return uint160(a.data(), a.size);
}

uint160 CryptoAddressBySign(const uint256& hash, const std::vector<uint8>& vchSig)
{
    dev::Signature sig(vchSig);
    dev::h256 h(bytes(hash.begin(), hash.end()));
    dev::Public p = dev::recover(sig, h);
    dev::Address a = dev::toAddress(p);
    return uint160(a.data(), a.size);
}

///////////////////////////////////////////////////////////////
// ETH TX SIGN

bool GetEthTxData(const uint256& secret, const CEthTxSkeleton& ets, uint256& hashTx, bytes& btEthTxData)
{
    auto pSecret = CryptoAlloc<dev::Secret>();
    if (pSecret == nullptr)
    {
        return false;
    }
    memcpy((void*)(pSecret->data()), secret.begin(), secret.size());

    try
    {
        dev::eth::TransactionSkeleton ts;

        ts.chainId = ets.chainId;
        ts.creation = ets.creation;
        ts.from = dev::Address(bytes(ets.from.begin(), ets.from.end()));
        ts.to = dev::Address(bytes(ets.to.begin(), ets.to.end()));
        ts.value = dev::u256(ets.value.GetValueHex());
        ts.data = ets.data;
        ts.nonce = dev::u256(ets.nonce.GetValueHex());
        ts.gas = dev::u256(ets.gas.GetValueHex());
        ts.gasPrice = dev::u256(ets.gasPrice.GetValueHex());

        dev::eth::TransactionBase tx(ts, *pSecret);

        hashTx = uint256(h256_to_hex(tx.sha3()));
        btEthTxData = tx.rlp();
    }
    catch (exception& e)
    {
        CryptoFree(pSecret);
        return false;
    }
    CryptoFree(pSecret);
    return true;
}

bytes MakeEthTxCallData(const std::string& strFunction, const std::vector<bytes>& vParamList)
{
    bytes btData;
    bytes btFuncSign = CryptoKeccakSign(strFunction);
    btData.insert(btData.end(), btFuncSign.begin(), btFuncSign.end());
    for (auto& p : vParamList)
    {
        btData.insert(btData.end(), p.begin(), p.end());
    }
    return btData;
}

bool GetEthSignRsv(const bytes& btSigData, uint256& r, uint256& s, uint8& v)
{
    if (btSigData.size() != SIGN_DATA_SIZE)
    {
        return false;
    }
    dev::h256 rOut;
    dev::h256 sOut;
    dev::byte vOut;
    dev::toSignatureRsv(btSigData, rOut, sOut, vOut);
    r = uint256(rOut.data(), rOut.size);
    s = uint256(sOut.data(), sOut.size);
    v = vOut;
    return true;
}

///////////////////////////////////////////////////////////////
// return the nIndex key is signed in multiple signature
// static bool IsSigned(const uint8* pIndex, const size_t nLen, const size_t nIndex)
// {
//     return (nLen * 8 > nIndex) && pIndex[nLen - 1 - nIndex / 8] & (1 << (nIndex % 8));
// }

// // set the nIndex key signed in multiple signature
// static bool SetSigned(uint8* pIndex, const size_t nLen, const size_t nIndex)
// {
//     if (nLen * 8 <= nIndex)
//     {
//         return false;
//     }
//     pIndex[nLen - 1 - nIndex / 8] |= (1 << (nIndex % 8));
//     return true;
// }

// bool CryptoMultiSign(const set<uint256>& setPubKey, const CCryptoKey& privkey, const uint8* pM, const size_t lenM, vector<uint8>& vchSig)
// {
//     if (setPubKey.empty())
//     {
//         mtbase::StdTrace("multisign", "key set is empty");
//         return false;
//     }

//     // index
//     set<uint256>::const_iterator itPub = setPubKey.find(privkey.pubkey);
//     if (itPub == setPubKey.end())
//     {
//         mtbase::StdTrace("multisign", "no key %s in set", privkey.pubkey.ToString().c_str());
//         return false;
//     }
//     size_t nIndex = distance(setPubKey.begin(), itPub);

//     // unpack index of  (index,S,Ri,...,Rj)
//     size_t nIndexLen = (setPubKey.size() - 1) / 8 + 1;
//     if (vchSig.empty())
//     {
//         vchSig.resize(nIndexLen);
//     }
//     else if (vchSig.size() < nIndexLen + 64)
//     {
//         mtbase::StdTrace("multisign", "vchSig size %lu is too short, need %lu minimum", vchSig.size(), nIndexLen + 64);
//         return false;
//     }
//     uint8* pIndex = &vchSig[0];

//     // already signed
//     if (IsSigned(pIndex, nIndexLen, nIndex))
//     {
//         mtbase::StdTrace("multisign", "key %s is already signed", privkey.pubkey.ToString().c_str());
//         return true;
//     }

//     // position of RS, (index,Ri,Si,...,R,S,...,Rj,Sj)
//     size_t nPosRS = nIndexLen;
//     for (size_t i = 0; i < nIndex; ++i)
//     {
//         if (IsSigned(pIndex, nIndexLen, i))
//         {
//             nPosRS += 64;
//         }
//     }
//     if (nPosRS > vchSig.size())
//     {
//         mtbase::StdTrace("multisign", "index %lu key is signed, but not exist R", nIndex);
//         return false;
//     }

//     // sign
//     vector<uint8> vchRS;
//     CryptoSign(privkey, pM, lenM, vchRS);

//     // record
//     if (!SetSigned(pIndex, nIndexLen, nIndex))
//     {
//         mtbase::StdTrace("multisign", "set %lu index signed error", nIndex);
//         return false;
//     }
//     vchSig.insert(vchSig.begin() + nPosRS, vchRS.begin(), vchRS.end());

//     return true;
// }

// bool CryptoMultiVerify(const set<uint256>& setPubKey, const uint8* pM, const size_t lenM, const vector<uint8>& vchSig, set<uint256>& setPartKey)
// {
//     if (setPubKey.empty())
//     {
//         mtbase::StdTrace("multiverify", "key set is empty");
//         return false;
//     }

//     // unpack (index,Ri,Si,...,Rj,Sj)
//     int nIndexLen = (setPubKey.size() - 1) / 8 + 1;
//     if (vchSig.size() < (nIndexLen + 64))
//     {
//         mtbase::StdTrace("multiverify", "vchSig size %lu is too short, need %lu minimum", vchSig.size(), nIndexLen + 64);
//         return false;
//     }
//     const uint8* pIndex = &vchSig[0];

//     size_t nPosRS = nIndexLen;
//     size_t i = 0;
//     for (auto itPub = setPubKey.begin(); itPub != setPubKey.end(); ++itPub, ++i)
//     {
//         if (IsSigned(pIndex, nIndexLen, i))
//         {
//             const uint256& pk = *itPub;
//             if (nPosRS + 64 > vchSig.size())
//             {
//                 mtbase::StdTrace("multiverify", "index %lu key is signed, but not exist R", i);
//                 return false;
//             }

//             vector<uint8> vchRS(vchSig.begin() + nPosRS, vchSig.begin() + nPosRS + 64);
//             if (!CryptoVerify(pk, pM, lenM, vchRS))
//             {
//                 mtbase::StdTrace("multiverify", "verify index %lu key sign failed", i);
//                 return false;
//             }

//             setPartKey.insert(pk);
//             nPosRS += 64;
//         }
//     }

//     if (nPosRS != vchSig.size())
//     {
//         mtbase::StdTrace("multiverify", "vchSig size %lu is too long, need %lu", vchSig.size(), nPosRS);
//         return false;
//     }

//     return true;
// }

/******** defect old version multi-sign begin *********/
//     0        key1          keyn
// |________|________| ... |_______|
// static vector<uint8> MultiSignPreApkDefect(const set<uint256>& setPubKey)
// {
//     vector<uint8> vecHash;
//     vecHash.resize((setPubKey.size() + 1) * uint256::size());

//     int i = uint256::size();
//     for (const uint256& key : setPubKey)
//     {
//         memcpy(&vecHash[i], key.begin(), key.size());
//         i += key.size();
//     }

//     return vecHash;
// }

// H(Ai,A1,...,An)*Ai + ... + H(Aj,A1,...,An)*Aj
// setPubKey = [A1 ... An], setPartKey = [Ai ... Aj], 1 <= i <= j <= n
// static bool MultiSignApkDefect(const set<uint256>& setPubKey, const set<uint256>& setPartKey, uint8* md32)
// {
//     vector<uint8> vecHash = MultiSignPreApkDefect(setPubKey);

//     CEdwards25519 apk;
//     for (const uint256& key : setPartKey)
//     {
//         memcpy(&vecHash[0], key.begin(), key.size());
//         CSC25519 hash = CSC25519(CryptoHash(&vecHash[0], vecHash.size()).begin());

//         CEdwards25519 point;
//         if (!point.Unpack(key.begin()))
//         {
//             return false;
//         }
//         apk += point.ScalarMult(hash);
//     }

//     apk.Pack(md32);
//     return true;
// }

// // hash = H(X,apk,M)
// static CSC25519 MultiSignHashDefect(const uint8* pX, const size_t lenX, const uint8* pApk, const size_t lenApk, const uint8* pM, const size_t lenM)
// {
//     uint8 hash[32];

//     crypto_generichash_blake2b_state state;
//     crypto_generichash_blake2b_init(&state, nullptr, 0, 32);
//     crypto_generichash_blake2b_update(&state, pX, lenX);
//     crypto_generichash_blake2b_update(&state, pApk, lenApk);
//     crypto_generichash_blake2b_update(&state, pM, lenM);
//     crypto_generichash_blake2b_final(&state, hash, 32);

//     return CSC25519(hash);
// }

// // r = H(H(sk,pk),M)
// static CSC25519 MultiSignRDefect(const CCryptoKey& key, const uint8* pM, const size_t lenM)
// {
//     uint8 hash[32];

//     crypto_generichash_blake2b_state state;
//     crypto_generichash_blake2b_init(&state, NULL, 0, 32);
//     uint256 keyHash = CryptoHash((uint8*)&key, 64);
//     crypto_generichash_blake2b_update(&state, (uint8*)&keyHash, 32);
//     crypto_generichash_blake2b_update(&state, pM, lenM);
//     crypto_generichash_blake2b_final(&state, hash, 32);

//     return CSC25519(hash);
// }

// static CSC25519 ClampPrivKeyDefect(const uint256& privkey)
// {
//     uint8 hash[64];
//     crypto_hash_sha512(hash, privkey.begin(), privkey.size());
//     hash[0] &= 248;
//     hash[31] &= 127;
//     hash[31] |= 64;

//     return CSC25519(hash);
// }

// bool CryptoMultiSignDefect(const set<uint256>& setPubKey, const CCryptoKey& privkey, const uint8* pX, const size_t lenX,
//                            const uint8* pM, const size_t lenM, vector<uint8>& vchSig)
// {
//     if (setPubKey.empty())
//     {
//         return false;
//     }

//     // unpack (index,R,S)
//     CSC25519 S;
//     CEdwards25519 R;
//     int nIndexLen = (setPubKey.size() - 1) / 8 + 1;
//     if (vchSig.empty())
//     {
//         vchSig.resize(nIndexLen + 64);
//     }
//     else
//     {
//         if (vchSig.size() != nIndexLen + 64)
//         {
//             return false;
//         }
//         if (!R.Unpack(&vchSig[nIndexLen]))
//         {
//             return false;
//         }
//         S.Unpack(&vchSig[nIndexLen + 32]);
//     }
//     uint8* pIndex = &vchSig[0];

//     // apk
//     uint256 apk;
//     if (!MultiSignApkDefect(setPubKey, setPubKey, apk.begin()))
//     {
//         return false;
//     }
//     // H(X,apk,M)
//     CSC25519 hash = MultiSignHashDefect(pX, lenX, apk.begin(), apk.size(), pM, lenM);

//     // sign
//     set<uint256>::const_iterator itPub = setPubKey.find(privkey.pubkey);
//     if (itPub == setPubKey.end())
//     {
//         return false;
//     }
//     size_t index = distance(setPubKey.begin(), itPub);

//     if (!(pIndex[index / 8] & (1 << (index % 8))))
//     {
//         // ri = H(H(si,pi),M)
//         CSC25519 ri = MultiSignRDefect(privkey, pM, lenM);
//         // hi = H(Ai,A1,...,An)
//         vector<uint8> vecHash = MultiSignPreApkDefect(setPubKey);
//         memcpy(&vecHash[0], itPub->begin(), itPub->size());
//         CSC25519 hi = CSC25519(CryptoHash(&vecHash[0], vecHash.size()).begin());
//         // si = H(privkey)
//         CSC25519 si = ClampPrivKeyDefect(privkey.secret);
//         // Si = ri + H(X,apk,M) * hi * si
//         CSC25519 Si = ri + hash * hi * si;
//         // S += Si
//         S += Si;
//         // R += Ri
//         CEdwards25519 Ri;
//         Ri.Generate(ri);
//         R += Ri;

//         pIndex[index / 8] |= (1 << (index % 8));
//     }

//     R.Pack(&vchSig[nIndexLen]);
//     S.Pack(&vchSig[nIndexLen + 32]);

//     return true;
// }

// bool CryptoMultiVerifyDefect(const set<uint256>& setPubKey, const uint8* pX, const size_t lenX,
//                              const uint8* pM, const size_t lenM, const vector<uint8>& vchSig, set<uint256>& setPartKey)
// {
//     if (setPubKey.empty())
//     {
//         return false;
//     }

//     if (vchSig.empty())
//     {
//         return true;
//     }

//     // unpack (index,R,S)
//     CSC25519 S;
//     CEdwards25519 R;
//     int nIndexLen = (setPubKey.size() - 1) / 8 + 1;
//     if (vchSig.size() != (nIndexLen + 64))
//     {
//         return false;
//     }
//     if (!R.Unpack(&vchSig[nIndexLen]))
//     {
//         return false;
//     }
//     S.Unpack(&vchSig[nIndexLen + 32]);
//     const uint8* pIndex = &vchSig[0];

//     // apk
//     uint256 apk;
//     if (!MultiSignApkDefect(setPubKey, setPubKey, apk.begin()))
//     {
//         return false;
//     }
//     // H(X,apk,M)
//     CSC25519 hash = MultiSignHashDefect(pX, lenX, apk.begin(), apk.size(), pM, lenM);

//     // A = hi*Ai + ... + aj*Aj
//     CEdwards25519 A;
//     vector<uint8> vecHash = MultiSignPreApkDefect(setPubKey);
//     setPartKey.clear();
//     int i = 0;

//     for (auto itPub = setPubKey.begin(); itPub != setPubKey.end(); ++itPub, ++i)
//     {
//         if (pIndex[i / 8] & (1 << (i % 8)))
//         {
//             // hi = H(Ai,A1,...,An)
//             memcpy(&vecHash[0], itPub->begin(), itPub->size());
//             CSC25519 hi = CSC25519(CryptoHash(&vecHash[0], vecHash.size()).begin());
//             // hi * Ai
//             CEdwards25519 Ai;
//             if (!Ai.Unpack(itPub->begin()))
//             {
//                 return false;
//             }
//             A += Ai.ScalarMult(hi);

//             setPartKey.insert(*itPub);
//         }
//     }

//     // SB = R + H(X,apk,M) * (hi*Ai + ... + aj*Aj)
//     CEdwards25519 SB;
//     SB.Generate(S);

//     return SB == R + A.ScalarMult(hash);
// }

/******** defect old version multi-sign end *********/

// Encrypt
void CryptoKeyFromPassphrase(int version, const CCryptoString& passphrase, const uint256& salt, CCryptoKeyData& key)
{
    key.resize(32);
    if (version == 0)
    {
        key.assign(salt.begin(), salt.end());
    }
    else if (version == 1)
    {
        if (crypto_pwhash_scryptsalsa208sha256(&key[0], 32, passphrase.c_str(), passphrase.size(), (const uint8*)&salt,
                                               crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE,
                                               crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE)
            != 0)
        {
            throw CCryptoError("CryptoKeyFromPassphrase : Failed to create key, key version = 1");
        }
    }
    else
    {
        throw CCryptoError("CryptoKeyFromPassphrase : Failed to create key, unknown key version");
    }
}

bool CryptoEncryptSecret(int version, const CCryptoString& passphrase, const CCryptoKey& key, CCryptoCipher& cipher)
{
    uint256 pubkey;
    memcpy(pubkey.begin(), key.pubkey.begin(), pubkey.size());
    CCryptoKeyData ek;
    CryptoKeyFromPassphrase(version, passphrase, pubkey, ek);
    randombytes_buf(&cipher.nonce, sizeof(cipher.nonce));

    return (crypto_aead_chacha20poly1305_encrypt(cipher.encrypted, nullptr,
                                                 (const uint8*)&key.secret, 32,
                                                 (const uint8*)&pubkey, 32,
                                                 nullptr, (uint8*)&cipher.nonce, &ek[0])
            == 0);
}

bool CryptoDecryptSecret(int version, const CCryptoString& passphrase, const CCryptoCipher& cipher, CCryptoKey& key)
{
    uint256 pubkey;
    memcpy(pubkey.begin(), key.pubkey.begin(), pubkey.size());
    CCryptoKeyData ek;
    CryptoKeyFromPassphrase(version, passphrase, pubkey, ek);
    return (crypto_aead_chacha20poly1305_decrypt((uint8*)&key.secret, nullptr, nullptr,
                                                 cipher.encrypted, 48,
                                                 (const uint8*)&pubkey, 32,
                                                 (const uint8*)&cipher.nonce, &ek[0])
            == 0);
}

/////////////////////////////
// bls sign & verify

bool CryptoBlsMakeNewKey(CCryptoBlsKey& key)
{
    CCryptoKey skey;
    if (CryptoMakeNewKey(skey).IsNull())
    {
        return false;
    }
    return CryptoBlsMakeNewKey(key, skey.secret);
}

bool CryptoBlsMakeNewKey(CCryptoBlsKey& key, const uint256& random)
{
    try
    {
        PrivateKey sk = PopSchemeMPL().KeyGen(random.GetBytes());
        G1Element pk = sk.GetG1Element();
        key.secret.SetBytes(sk.Serialize());
        key.pubkey.SetBytes(pk.Serialize());
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CryptoBlsGetPubkey(const uint256& secret, uint384& pubkey)
{
    try
    {
        PrivateKey sk = PrivateKey::FromByteVector(secret.GetBytes());
        pubkey.SetBytes(sk.GetG1Element().Serialize());
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CryptoBlsSign(const uint256& secret, const bytes& btData, bytes& btSig)
{
    try
    {
        btSig = PopSchemeMPL().Sign(PrivateKey::FromByteVector(secret.GetBytes()), btData).Serialize();
        if (btSig.size() != 96)
        {
            StdError(__PRETTY_FUNCTION__, "Sign fail, sig size: %lu", btSig.size());
            return false;
        }
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CryptoBlsVerify(const uint384& pubkey, const bytes& btData, const bytes& btSig)
{
    try
    {
        return PopSchemeMPL().Verify(G1Element::FromByteVector(pubkey.GetBytes()), btData, G2Element::FromByteVector(btSig));
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CryptoBlsAggregateSig(const std::vector<bytes>& vSigs, bytes& btAggSig)
{
    if (vSigs.size() == 0)
    {
        StdError(__PRETTY_FUNCTION__, "param error");
        return false;
    }
    try
    {
        vector<G2Element> sigs;
        sigs.reserve(vSigs.size());
        for (auto const& sig : vSigs)
        {
            sigs.emplace_back(G2Element::FromByteVector(sig));
        }
        btAggSig = PopSchemeMPL().Aggregate(sigs).Serialize();
        if (btAggSig.size() != 96)
        {
            StdError(__PRETTY_FUNCTION__, "Aggregate fail, agg sig size: %lu", btAggSig.size());
            return false;
        }
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CryptoBlsAggregateVerify(const std::vector<uint384>& vPubkeys, const std::vector<bytes>& vDatas, const bytes& btAggSig)
{
    if (vPubkeys.size() == 0 || vDatas.size() != vPubkeys.size() || btAggSig.size() != 96)
    {
        StdError(__PRETTY_FUNCTION__, "param error");
        return false;
    }
    try
    {
        vector<G1Element> pks;
        pks.reserve(vPubkeys.size());
        for (auto const& pk : vPubkeys)
        {
            pks.emplace_back(G1Element::FromByteVector(pk.GetBytes()));
        }
        return PopSchemeMPL().AggregateVerify(pks, vDatas, G2Element::FromByteVector(btAggSig));
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CryptoBlsFastAggregateVerify(const std::vector<uint384>& vPubkeys, const bytes& btData, const bytes& btAggSig)
{
    if (vPubkeys.size() == 0 || btData.size() == 0 || btAggSig.size() != 96)
    {
        StdError(__PRETTY_FUNCTION__, "param error");
        return false;
    }
    try
    {
        vector<G1Element> pks;
        pks.reserve(vPubkeys.size());
        for (auto const& pk : vPubkeys)
        {
            pks.emplace_back(G1Element::FromByteVector(pk.GetBytes()));
        }
        return PopSchemeMPL().FastAggregateVerify(pks, btData, G2Element::FromByteVector(btAggSig));
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

} // namespace crypto
} // namespace metabasenet
