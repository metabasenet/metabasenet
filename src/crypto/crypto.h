// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CRYPTO_CRYPTO_H
#define CRYPTO_CRYPTO_H

#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "uint256.h"

namespace metabasenet
{
namespace crypto
{

// Runtime except
class CCryptoError : public std::runtime_error
{
public:
    explicit CCryptoError(const std::string& str)
      : std::runtime_error(str)
    {
    }
};

// Secure memory
void* CryptoAlloc(const std::size_t size);
void CryptoFree(void* ptr);

void* NormalAlloc(const std::size_t size);
void NormalFree(void* ptr);

template <typename T>
T* CryptoAlloc()
{
    return static_cast<T*>(CryptoAlloc(sizeof(T)));
}

template <typename T>
T* NormalAlloc()
{
    return static_cast<T*>(NormalAlloc(sizeof(T)));
}

template <typename T>
class CCryptoAllocator : public std::allocator<T>
{
public:
    CCryptoAllocator() throw() {}
    CCryptoAllocator(const CCryptoAllocator& a) throw()
      : std::allocator<T>(a)
    {
    }
    template <typename U>
    CCryptoAllocator(const CCryptoAllocator<U>& a) throw()
      : std::allocator<T>(a)
    {
    }
    ~CCryptoAllocator() throw() {}
    template <typename _Other>
    struct rebind
    {
        typedef CCryptoAllocator<_Other> other;
    };

    T* allocate(std::size_t n, const void* hint = 0)
    {
        (void)hint;
        return static_cast<T*>(CryptoAlloc(sizeof(T) * n));
    }
    void deallocate(T* p, std::size_t n)
    {
        (void)n;
        CryptoFree(p);
    }
};

typedef std::basic_string<char, std::char_traits<char>, CCryptoAllocator<char>> CCryptoString;
typedef std::vector<unsigned char, CCryptoAllocator<unsigned char>> CCryptoKeyData;

// Heap memory lock
void CryptoMLock(void* const addr, const std::size_t len);
void CryptoMUnlock(void* const addr, const std::size_t len);

// Rand
uint32 CryptoGetRand32();
uint64 CryptoGetRand64();
void CryptoGetRand256(uint256& u);

// Hash
uint256 CryptoHash(const void* msg, std::size_t len);
uint256 CryptoHash(const uint256& h1, const uint256& h2);
uint256 CryptoPowHash(const void* msg, size_t len);

// SHA256
uint256 CryptoSHA256(const void* msg, size_t len);

// Keccak
bytes CryptoKeccak(const void* msg, size_t len);
uint256 CryptoKeccakHash(const void* msg, size_t len);
uint256 CryptoKeccakHash(const bytes& data);
bytes CryptoKeccakSign(const void* msg, size_t len);
bytes CryptoKeccakSign(const std::string& strMsg);

// Sign & verify
struct CCryptoKey
{
    uint256 secret;
    uint512 pubkey;
};

const size_t SIGN_DATA_SIZE = 65;

uint512 CryptoMakeNewKey(CCryptoKey& key);
uint512 CryptoImportKey(CCryptoKey& key, const uint256& secret);
void CryptoSign(const CCryptoKey& key, const void* md, const std::size_t len, std::vector<uint8>& vchSig);
void CryptoSign(const CCryptoKey& key, const uint256& hash, std::vector<uint8>& vchSig);
bool CryptoVerify(const uint512& pubkey, const void* md, const std::size_t len, const std::vector<uint8>& vchSig);
bool CryptoVerify(const uint512& pubkey, const uint256& hash, const std::vector<uint8>& vchSig);
bool CryptoVerify(const uint160& address, const uint256& hash, const std::vector<uint8>& vchSig);
bool CryptoVerify(const uint256& address, const uint256& hash, const std::vector<uint8>& vchSig);
uint512 CrytoGetSignPubkey(const uint256& hash, const std::vector<uint8>& vchSig);

uint160 CryptoGetEcAddress(const CCryptoKey& key);
uint160 CryptoGetPubkeyAddress(const uint512& pubkey);
uint160 CryptoAddressBySign(const uint256& hash, const std::vector<uint8>& vchSig);

// ETH TX SIGN

class CEthTxSkeleton
{
public:
    bool creation = false;
    uint64 chainId = 0;
    uint160 from;
    uint160 to;
    uint256 value;
    bytes data;
    uint256 nonce;
    uint256 gas;
    uint256 gasPrice;
};

bool GetEthTxData(const uint256& secret, const CEthTxSkeleton& ets, uint256& hashTx, bytes& btEthTxData);
bytes MakeEthTxCallData(const std::string& strFunction, const std::vector<bytes>& vParamList);
bool GetEthSignRsv(const bytes& btSigData, uint256& r, uint256& s, uint8& v);

// assume:
//   1. 1 <= i <= j <= n
//   2. Pi is the i-th public key
//   3. ki is the i-th private key
//   4. Pi = si * B, si = Clamp(sha512(ki)[0, 32))
//      void Clamp(unsigned char k[32])
//      {
//          k[0] &= 248;
//          k[31] &= 127;
//          k[31] |= 64;
//      }
//   5. the sequence of (P1, ..., Pn) is according to the order of uint256
//
// signature:
//   1. vchSig = (index,Ri,Si...,Rj,Sj)
//   2. index is a bitmap of participation keys in (P1, ..., Pn) order.
//      For example: if n = 10, and cosigner participants are (P1, P3, P10), then index is [00000010,00000101]
//   3. (Ri,Si) is the same as standard ed25519 signature algorithm (reference libsodium crypto_sign_ed25519_detached)
//   4. Ri = ri * B, ri = sha512(sha512(ki)[32,64),M)
//   5. Si = ri + hash(Ri,Pi,M) * si,
//
// proof:
//   each (Ri,Si) proof is the same as standard ed25519 verify signature algorithm (reference libsodium crypto_sign_ed25519_verify_detached)
//   Si = Ri + hash(Ri,Pi,M) * Pi
// bool CryptoMultiSign(const std::set<uint256>& setPubKey, const CCryptoKey& privkey, const uint8* pM, const std::size_t lenM, std::vector<uint8>& vchSig);
// bool CryptoMultiVerify(const std::set<uint256>& setPubKey, const uint8* pM, const std::size_t lenM, const std::vector<uint8>& vchSig, std::set<uint256>& setPartKey);
// // defect old version multi-sign algorithm, only for backward compatiblility.
// bool CryptoMultiSignDefect(const std::set<uint256>& setPubKey, const CCryptoKey& privkey, const uint8* pX, const size_t lenX,
//                            const uint8* pM, const size_t lenM, std::vector<uint8>& vchSig);
// bool CryptoMultiVerifyDefect(const std::set<uint256>& setPubKey, const uint8* pX, const size_t lenX,
//                              const uint8* pM, const size_t lenM, const std::vector<uint8>& vchSig, std::set<uint256>& setPartKey);

// Encrypt
struct CCryptoCipher
{
    CCryptoCipher()
      : nonce(0)
    {
        memset(encrypted, 0, 48);
    }

    uint8 encrypted[48];
    uint64 nonce;
};

void CryptoKeyFromPassphrase(int version, const CCryptoString& passphrase, const uint256& salt, CCryptoKeyData& key);
bool CryptoEncryptSecret(int version, const CCryptoString& passphrase, const CCryptoKey& key, CCryptoCipher& cipher);
bool CryptoDecryptSecret(int version, const CCryptoString& passphrase, const CCryptoCipher& cipher, CCryptoKey& key);

// bls sign & verify
struct CCryptoBlsKey
{
    uint256 secret;
    uint384 pubkey;
};

bool CryptoBlsMakeNewKey(CCryptoBlsKey& key);
bool CryptoBlsMakeNewKey(CCryptoBlsKey& key, const uint256& random);
bool CryptoBlsGetPubkey(const uint256& secret, uint384& pubkey);
bool CryptoBlsSign(const uint256& secret, const bytes& btData, bytes& btSig);
bool CryptoBlsVerify(const uint384& pubkey, const bytes& btData, const bytes& btSig);
bool CryptoBlsAggregateSig(const std::vector<bytes>& vSigs, bytes& btAggSig);
bool CryptoBlsAggregateVerify(const std::vector<uint384>& vPubkeys, const std::vector<bytes>& vDatas, const bytes& btAggSig);
bool CryptoBlsFastAggregateVerify(const std::vector<uint384>& vPubkeys, const bytes& btData, const bytes& btAggSig);

} // namespace crypto
} // namespace metabasenet

#endif // CRYPTO_CRYPTO_H
