// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "precompiled.h"

#include "../meth/libdevcore/SHA3.h"
#include "../meth/libdevcrypto/Blake2.h"
#include "../meth/libdevcrypto/Common.h"
#include "../meth/libdevcrypto/Hash.h"
#include "../meth/libdevcrypto/LibSnark.h"

using namespace std;
using namespace mtbase;
using namespace dev;

namespace metabasenet
{
namespace mvm
{

bigint linearPricer(unsigned _base, unsigned _word, bytesConstRef _in)
{
    bigint const s = _in.size();
    bigint const b = _base;
    bigint const w = _word;
    return b + (s + 31) / 32 * w;
}

// Parse _count bytes of _in starting with _begin offset as big endian int.
// If there's not enough bytes in _in, consider it infinitely right-padded with zeroes.
bigint parseBigEndianRightPadded(bytesConstRef _in, bigint const& _begin, bigint const& _count)
{
    if (_begin > _in.count())
        return 0;
    assert(_count <= numeric_limits<size_t>::max() / 8); // Otherwise, the return value would not fit in the memory.

    size_t const begin{ _begin };
    size_t const count{ _count };

    // crop _in, not going beyond its size
    bytesConstRef cropped = _in.cropped(begin, min(count, _in.count() - begin));

    bigint ret = fromBigEndian<bigint>(cropped);
    // shift as if we had right-padding zeroes
    assert(count - cropped.count() <= numeric_limits<size_t>::max() / 8);
    ret <<= 8 * (count - cropped.count());

    return ret;
}

namespace
{
bigint expLengthAdjust(bigint const& _expOffset, bigint const& _expLength, bytesConstRef _in)
{
    if (_expLength <= 32)
    {
        bigint const exp(parseBigEndianRightPadded(_in, _expOffset, _expLength));
        return exp ? msb(exp) : 0;
    }
    else
    {
        bigint const expFirstWord(parseBigEndianRightPadded(_in, _expOffset, 32));
        size_t const highestBit(expFirstWord ? msb(expFirstWord) : 0);
        return 8 * (_expLength - 32) + highestBit;
    }
}

bigint multComplexity(bigint const& _x)
{
    if (_x <= 64)
        return _x * _x;
    if (_x <= 1024)
        return (_x * _x) / 4 + 96 * _x - 3072;
    else
        return (_x * _x) / 16 + 480 * _x - 199680;
}
} // namespace

bytes precompliled_modexp(bytesConstRef _in)
{
    bigint const baseLength(parseBigEndianRightPadded(_in, 0, 32));
    bigint const expLength(parseBigEndianRightPadded(_in, 32, 32));
    bigint const modLength(parseBigEndianRightPadded(_in, 64, 32));
    assert(modLength <= numeric_limits<size_t>::max() / 8);  // Otherwise gas should be too expensive.
    assert(baseLength <= numeric_limits<size_t>::max() / 8); // Otherwise, gas should be too expensive.
    if (modLength == 0 && baseLength == 0)
        return bytes{}; // This is a special case where expLength can be very big.
    assert(expLength <= numeric_limits<size_t>::max() / 8);

    bigint const base(parseBigEndianRightPadded(_in, 96, baseLength));
    bigint const exp(parseBigEndianRightPadded(_in, 96 + baseLength, expLength));
    bigint const mod(parseBigEndianRightPadded(_in, 96 + baseLength + expLength, modLength));

    bigint const result = mod != 0 ? boost::multiprecision::powm(base, exp, mod) : bigint{ 0 };

    size_t const retLength(modLength);
    bytes ret(retLength);
    toBigEndian(result, ret);

    return ret;
}

uint64_t pricer_modexp(bytesConstRef _in)
{
    bigint const baseLength(parseBigEndianRightPadded(_in, 0, 32));
    bigint const expLength(parseBigEndianRightPadded(_in, 32, 32));
    bigint const modLength(parseBigEndianRightPadded(_in, 64, 32));

    bigint const maxLength(max(modLength, baseLength));
    bigint const adjustedExpLength(expLengthAdjust(baseLength + 96, expLength, _in));

    return static_cast<uint64_t>(multComplexity(maxLength) * max<bigint>(adjustedExpLength, 1) / 20);
}

///////////////////////////
// CPrecompliled

bool CPrecompliled::execute(const Address& _addr, const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas, bool& _result)
{
    auto funcPrecExec = get_prec_func(_addr);
    if (funcPrecExec)
    {
        _result = funcPrecExec(_in, _outdata, _usedgas);
        return true;
    }
    return false;
}

bool CPrecompliled::exec_ecrecover(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas)
{
    struct
    {
        h256 hash;
        h256 v;
        h256 r;
        h256 s;
    } in;

    _usedgas = 3000;

    memcpy(&in, _in.data(), min(_in.size(), sizeof(in)));

    h256 ret;
    u256 v = (u256)in.v;
    if (v >= 27 && v <= 28)
    {
        SignatureStruct sig(in.r, in.s, (uint8_t)((int)v - 27));
        if (sig.isValid())
        {
            try
            {
                if (Public rec = recover(sig, in.hash))
                {
                    ret = dev::sha3(rec);
                    memset(ret.data(), 0, 12);
                    _outdata = ret.asBytes();
                    return true;
                }
            }
            catch (...)
            {
            }
        }
    }
    return false;
}

bool CPrecompliled::exec_sha256(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas)
{
    _usedgas = static_cast<uint64_t>(linearPricer(60, 12, _in));
    _outdata = dev::sha256(_in).asBytes();
    return true;
}

bool CPrecompliled::exec_ripemd160(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas)
{
    _usedgas = static_cast<uint64_t>(linearPricer(600, 120, _in));
    _outdata = h256(dev::ripemd160(_in), h256::AlignRight).asBytes();
    return true;
}

bool CPrecompliled::exec_identity(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas)
{
    _usedgas = static_cast<uint64_t>(linearPricer(15, 3, _in));
    _outdata = _in.toBytes();
    return true;
}

bool CPrecompliled::exec_modexp(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas)
{
    _usedgas = pricer_modexp(_in);
    _outdata = precompliled_modexp(_in);
    return true;
}

bool CPrecompliled::exec_alt_bn128_G1_add(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas)
{
    //_usedgas = (_blockNumber < _chainParams.istanbulForkBlock ? 500 : 150);
    _usedgas = 150;
    auto r = dev::crypto::alt_bn128_G1_add(_in);
    if (!r.first)
    {
        return false;
    }
    _outdata = r.second;
    return true;
}

bool CPrecompliled::exec_alt_bn128_G1_mul(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas)
{
    //_usedgas = _blockNumber < _chainParams.istanbulForkBlock ? 40000 : 6000;
    _usedgas = 6000;
    auto r = dev::crypto::alt_bn128_G1_mul(_in);
    if (!r.first)
    {
        return false;
    }
    _outdata = r.second;
    return true;
}

bool CPrecompliled::exec_alt_bn128_pairing_product(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas)
{
    auto const k = _in.size() / 192;
    //_usedgas = _blockNumber < _chainParams.istanbulForkBlock ? 100000 + k * 80000 : 45000 + k * 34000;
    _usedgas = 45000 + k * 34000;
    auto r = dev::crypto::alt_bn128_pairing_product(_in);
    if (!r.first)
    {
        return false;
    }
    _outdata = r.second;
    return true;
}

bool CPrecompliled::exec_blake2_compression(const bytesConstRef _in, bytes& _outdata, uint64_t& _usedgas)
{
    _usedgas = fromBigEndian<uint32_t>(_in.cropped(0, 4));

    static constexpr size_t roundsSize = 4;
    static constexpr size_t stateVectorSize = 8 * 8;
    static constexpr size_t messageBlockSize = 16 * 8;
    static constexpr size_t offsetCounterSize = 8;
    static constexpr size_t finalBlockIndicatorSize = 1;
    static constexpr size_t totalInputSize = roundsSize + stateVectorSize + messageBlockSize + 2 * offsetCounterSize + finalBlockIndicatorSize;

    if (_in.size() != totalInputSize)
    {
        return false;
    }

    auto const rounds = fromBigEndian<uint32_t>(_in.cropped(0, roundsSize));
    auto const stateVector = _in.cropped(roundsSize, stateVectorSize);
    auto const messageBlockVector = _in.cropped(roundsSize + stateVectorSize, messageBlockSize);
    auto const offsetCounter0 = _in.cropped(roundsSize + stateVectorSize + messageBlockSize, offsetCounterSize);
    auto const offsetCounter1 = _in.cropped(roundsSize + stateVectorSize + messageBlockSize + offsetCounterSize, offsetCounterSize);
    uint8_t const finalBlockIndicator = _in[roundsSize + stateVectorSize + messageBlockSize + 2 * offsetCounterSize];

    if (finalBlockIndicator != 0 && finalBlockIndicator != 1)
    {
        return false;
    }

    _outdata = dev::crypto::blake2FCompression(rounds, stateVector, offsetCounter0, offsetCounter1, finalBlockIndicator, messageBlockVector);
    return true;
}

} // namespace mvm
} // namespace metabasenet
