// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CRYPTO_UINT256_H
#define CRYPTO_UINT256_H

#include <boost/multiprecision/cpp_int.hpp>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "stream/stream.h"

/** Base class without constructors for uint256 and uint160.
 * This makes the compiler let u use it in a union.
 */
template <unsigned int BITS>
class base_uint
{
    friend class hnbase::CStream;

protected:
    enum
    {
        WIDTH = BITS / 32
    };
    unsigned int pn[WIDTH];

public:
    bool operator!() const
    {
        for (int i = 0; i < WIDTH; i++)
            if (pn[i] != 0)
                return false;
        return true;
    }

    const base_uint operator~() const
    {
        base_uint ret;
        for (int i = 0; i < WIDTH; i++)
            ret.pn[i] = ~pn[i];
        return ret;
    }

    const base_uint operator-() const
    {
        base_uint ret;
        for (int i = 0; i < WIDTH; i++)
            ret.pn[i] = ~pn[i];
        ++ret;
        return ret;
    }

    base_uint& operator=(int8 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator=(uint8 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator=(int16 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator=(uint16 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator=(int32 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator=(uint32 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator=(int64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator=(uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator^=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] ^= b.pn[i];
        return *this;
    }

    base_uint& operator&=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] &= b.pn[i];
        return *this;
    }

    base_uint& operator|=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] |= b.pn[i];
        return *this;
    }

    base_uint& operator^=(uint64 b)
    {
        pn[0] ^= (unsigned int)b;
        pn[1] ^= (unsigned int)(b >> 32);
        return *this;
    }

    base_uint& operator|=(uint64 b)
    {
        pn[0] |= (unsigned int)b;
        pn[1] |= (unsigned int)(b >> 32);
        return *this;
    }

    base_uint& operator<<=(unsigned int shift)
    {
        base_uint a(*this);
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
        int k = shift / 32;
        shift = shift % 32;
        for (int i = 0; i < WIDTH; i++)
        {
            if (i + k + 1 < WIDTH && shift != 0)
                pn[i + k + 1] |= (a.pn[i] >> (32 - shift));
            if (i + k < WIDTH)
                pn[i + k] |= (a.pn[i] << shift);
        }
        return *this;
    }

    base_uint& operator>>=(unsigned int shift)
    {
        base_uint a(*this);
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
        int k = shift / 32;
        shift = shift % 32;
        for (int i = 0; i < WIDTH; i++)
        {
            if (i - k - 1 >= 0 && shift != 0)
                pn[i - k - 1] |= (a.pn[i] << (32 - shift));
            if (i - k >= 0)
                pn[i - k] |= (a.pn[i] >> shift);
        }
        return *this;
    }

    base_uint& operator+=(const base_uint& b)
    {
        uint64 carry = 0;
        for (int i = 0; i < WIDTH; i++)
        {
            uint64 n = carry + pn[i] + b.pn[i];
            pn[i] = n & 0xffffffff;
            carry = n >> 32;
        }
        return *this;
    }

    base_uint& operator-=(const base_uint& b)
    {
        *this += -b;
        return *this;
    }

    base_uint& operator+=(uint64 b64)
    {
        base_uint b;
        b = b64;
        *this += b;
        return *this;
    }

    base_uint& operator-=(uint64 b64)
    {
        base_uint b;
        b = b64;
        *this += -b;
        return *this;
    }

    base_uint& operator++()
    {
        // prefix operator
        int i = 0;
        while (++pn[i] == 0 && i < WIDTH - 1)
            i++;
        return *this;
    }

    const base_uint operator++(int)
    {
        // postfix operator
        const base_uint ret = *this;
        ++(*this);
        return ret;
    }

    base_uint& operator--()
    {
        // prefix operator
        int i = 0;
        while (--pn[i] == -1 && i < WIDTH - 1)
            i++;
        return *this;
    }

    const base_uint operator--(int)
    {
        // postfix operator
        const base_uint ret = *this;
        --(*this);
        return ret;
    }

    unsigned int& operator[](size_t pos)
    {
        return pn[pos];
    }

    const unsigned int& operator[](size_t pos) const
    {
        return pn[pos];
    }

    friend inline bool operator<(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH - 1; i >= 0; i--)
        {
            if (a.pn[i] < b.pn[i])
                return true;
            else if (a.pn[i] > b.pn[i])
                return false;
        }
        return false;
    }

    friend inline bool operator<=(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH - 1; i >= 0; i--)
        {
            if (a.pn[i] < b.pn[i])
                return true;
            else if (a.pn[i] > b.pn[i])
                return false;
        }
        return true;
    }

    friend inline bool operator>(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH - 1; i >= 0; i--)
        {
            if (a.pn[i] > b.pn[i])
                return true;
            else if (a.pn[i] < b.pn[i])
                return false;
        }
        return false;
    }

    friend inline bool operator>=(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH - 1; i >= 0; i--)
        {
            if (a.pn[i] > b.pn[i])
                return true;
            else if (a.pn[i] < b.pn[i])
                return false;
        }
        return true;
    }

    friend inline bool operator==(const base_uint& a, const base_uint& b)
    {
        for (int i = 0; i < base_uint::WIDTH; i++)
            if (a.pn[i] != b.pn[i])
                return false;
        return true;
    }

    friend inline bool operator==(const base_uint& a, uint64 b)
    {
        if (a.pn[0] != (unsigned int)b)
            return false;
        if (a.pn[1] != (unsigned int)(b >> 32))
            return false;
        for (int i = 2; i < base_uint::WIDTH; i++)
            if (a.pn[i] != 0)
                return false;
        return true;
    }

    friend inline bool operator!=(const base_uint& a, const base_uint& b)
    {
        return (!(a == b));
    }

    friend inline bool operator!=(const base_uint& a, uint64 b)
    {
        return (!(a == b));
    }

    std::string GetHex() const
    {
        char psz[sizeof(pn) * 2 + 1];
        for (unsigned int i = 0; i < sizeof(pn); i++)
            sprintf(psz + i * 2, "%02x", ((unsigned char*)pn)[sizeof(pn) - i - 1]);
        return std::string(psz, psz + sizeof(pn) * 2);
    }

    size_t SetHex(const char* psz)
    {
        size_t len = 0;
        const char* in = psz;

        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;

        // skip leading spaces
        while (isspace(*psz))
            psz++;

        // skip 0x
        if (psz[0] == '0' && tolower(psz[1]) == 'x')
            psz += 2;

        // hex string to uint
        static unsigned char phexdigit[256] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        const char* pbegin = psz;
        while (phexdigit[(unsigned char)*psz] || *psz == '0')
            psz++;
        len = psz - in;
        psz--;
        unsigned char* p1 = (unsigned char*)pn;
        unsigned char* pend = p1 + WIDTH * 4;
        while (psz >= pbegin && p1 < pend)
        {
            *p1 = phexdigit[(unsigned char)*psz--];
            if (psz >= pbegin)
            {
                *p1 |= (phexdigit[(unsigned char)*psz--] << 4);
                p1++;
            }
        }
        return len;
    }

    size_t SetHex(const std::string& str)
    {
        return SetHex(str.c_str());
    }

    std::string ToString() const
    {
        return (GetHex());
    }

    unsigned char* begin()
    {
        return (unsigned char*)&pn[0];
    }

    unsigned char* end()
    {
        return (unsigned char*)&pn[WIDTH];
    }

    const unsigned char* begin() const
    {
        return (unsigned char*)&pn[0];
    }

    const unsigned char* end() const
    {
        return (unsigned char*)&pn[WIDTH];
    }

    static unsigned int size()
    {
        return sizeof(pn);
    }

    uint64 Get64(int n = 0) const
    {
        return pn[2 * n] | (uint64)pn[2 * n + 1] << 32;
    }

    uint32 Get32(int n = 0) const
    {
        return pn[n];
    }

    const base_uint& reverse()
    {
        unsigned char* front = (unsigned char*)pn;
        unsigned char* after = front + sizeof(pn) - 1;
        while (front < after)
        {
            unsigned char temp = *front;
            *front = *after;
            *after = temp;
            front++;
            after--;
        }
        return *this;
    }

protected:
    void Serialize(hnbase::CStream& s, hnbase::SaveType&) const
    {
        //s.Write((char*)pn, sizeof(pn));
        unsigned char us[sizeof(pn)];
        for (unsigned int i = 0; i < sizeof(pn); i++)
            us[i] = ((unsigned char*)pn)[sizeof(pn) - i - 1];
        s.Write((char*)us, sizeof(pn));
    }
    void Serialize(hnbase::CStream& s, hnbase::LoadType&)
    {
        //s.Read((char*)pn, sizeof(pn));
        unsigned char us[sizeof(pn)];
        s.Read((char*)us, sizeof(pn));
        for (unsigned int i = 0; i < sizeof(pn); i++)
            ((unsigned char*)pn)[sizeof(pn) - i - 1] = us[i];
    }
    void Serialize(hnbase::CStream& s, std::size_t& serSize) const
    {
        (void)s;
        serSize += sizeof(pn);
    }

    friend class uint160;
    friend class uint256;
    friend class uint224;
    friend class uint2048;
    friend inline int Testuint256AdHoc(std::vector<std::string> vArg);
};

typedef base_uint<160> base_uint160;
typedef base_uint<256> base_uint256;
typedef base_uint<224> base_uint224;
typedef base_uint<2048> base_uint2048;

//
// uint160 and uint256 could be implemented as templates, but to keep
// compile errors and debugging cleaner, they're copy and pasted.
//

//////////////////////////////////////////////////////////////////////////////
//
// uint160
//

/** 160-bit unsigned integer */
class uint160 : public base_uint160
{
public:
    typedef base_uint160 basetype;

    uint160()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint160(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint160& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint160(uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint160& operator=(uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    explicit uint160(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint160(const std::vector<unsigned char>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }
};

inline bool operator==(const uint160& a, uint64 b)
{
    return (base_uint160)a == b;
}
inline bool operator!=(const uint160& a, uint64 b)
{
    return (base_uint160)a != b;
}
inline const uint160 operator<<(const base_uint160& a, unsigned int shift)
{
    return uint160(a) <<= shift;
}
inline const uint160 operator>>(const base_uint160& a, unsigned int shift)
{
    return uint160(a) >>= shift;
}
inline const uint160 operator<<(const uint160& a, unsigned int shift)
{
    return uint160(a) <<= shift;
}
inline const uint160 operator>>(const uint160& a, unsigned int shift)
{
    return uint160(a) >>= shift;
}

inline const uint160 operator^(const base_uint160& a, const base_uint160& b)
{
    return uint160(a) ^= b;
}
inline const uint160 operator&(const base_uint160& a, const base_uint160& b)
{
    return uint160(a) &= b;
}
inline const uint160 operator|(const base_uint160& a, const base_uint160& b)
{
    return uint160(a) |= b;
}
inline const uint160 operator+(const base_uint160& a, const base_uint160& b)
{
    return uint160(a) += b;
}
inline const uint160 operator-(const base_uint160& a, const base_uint160& b)
{
    return uint160(a) -= b;
}

inline bool operator<(const base_uint160& a, const uint160& b)
{
    return (base_uint160)a < (base_uint160)b;
}
inline bool operator<=(const base_uint160& a, const uint160& b)
{
    return (base_uint160)a <= (base_uint160)b;
}
inline bool operator>(const base_uint160& a, const uint160& b)
{
    return (base_uint160)a > (base_uint160)b;
}
inline bool operator>=(const base_uint160& a, const uint160& b)
{
    return (base_uint160)a >= (base_uint160)b;
}
inline bool operator==(const base_uint160& a, const uint160& b)
{
    return (base_uint160)a == (base_uint160)b;
}
inline bool operator!=(const base_uint160& a, const uint160& b)
{
    return (base_uint160)a != (base_uint160)b;
}
inline const uint160 operator^(const base_uint160& a, const uint160& b)
{
    return (base_uint160)a ^ (base_uint160)b;
}
inline const uint160 operator&(const base_uint160& a, const uint160& b)
{
    return (base_uint160)a & (base_uint160)b;
}
inline const uint160 operator|(const base_uint160& a, const uint160& b)
{
    return (base_uint160)a | (base_uint160)b;
}
inline const uint160 operator+(const base_uint160& a, const uint160& b)
{
    return (base_uint160)a + (base_uint160)b;
}
inline const uint160 operator-(const base_uint160& a, const uint160& b)
{
    return (base_uint160)a - (base_uint160)b;
}

inline bool operator<(const uint160& a, const base_uint160& b)
{
    return (base_uint160)a < (base_uint160)b;
}
inline bool operator<=(const uint160& a, const base_uint160& b)
{
    return (base_uint160)a <= (base_uint160)b;
}
inline bool operator>(const uint160& a, const base_uint160& b)
{
    return (base_uint160)a > (base_uint160)b;
}
inline bool operator>=(const uint160& a, const base_uint160& b)
{
    return (base_uint160)a >= (base_uint160)b;
}
inline bool operator==(const uint160& a, const base_uint160& b)
{
    return (base_uint160)a == (base_uint160)b;
}
inline bool operator!=(const uint160& a, const base_uint160& b)
{
    return (base_uint160)a != (base_uint160)b;
}
inline const uint160 operator^(const uint160& a, const base_uint160& b)
{
    return (base_uint160)a ^ (base_uint160)b;
}
inline const uint160 operator&(const uint160& a, const base_uint160& b)
{
    return (base_uint160)a & (base_uint160)b;
}
inline const uint160 operator|(const uint160& a, const base_uint160& b)
{
    return (base_uint160)a | (base_uint160)b;
}
inline const uint160 operator+(const uint160& a, const base_uint160& b)
{
    return (base_uint160)a + (base_uint160)b;
}
inline const uint160 operator-(const uint160& a, const base_uint160& b)
{
    return (base_uint160)a - (base_uint160)b;
}

inline bool operator<(const uint160& a, const uint160& b)
{
    return (base_uint160)a < (base_uint160)b;
}
inline bool operator<=(const uint160& a, const uint160& b)
{
    return (base_uint160)a <= (base_uint160)b;
}
inline bool operator>(const uint160& a, const uint160& b)
{
    return (base_uint160)a > (base_uint160)b;
}
inline bool operator>=(const uint160& a, const uint160& b)
{
    return (base_uint160)a >= (base_uint160)b;
}
inline bool operator==(const uint160& a, const uint160& b)
{
    return (base_uint160)a == (base_uint160)b;
}
inline bool operator!=(const uint160& a, const uint160& b)
{
    return (base_uint160)a != (base_uint160)b;
}
inline const uint160 operator^(const uint160& a, const uint160& b)
{
    return (base_uint160)a ^ (base_uint160)b;
}
inline const uint160 operator&(const uint160& a, const uint160& b)
{
    return (base_uint160)a & (base_uint160)b;
}
inline const uint160 operator|(const uint160& a, const uint160& b)
{
    return (base_uint160)a | (base_uint160)b;
}
inline const uint160 operator+(const uint160& a, const uint160& b)
{
    return (base_uint160)a + (base_uint160)b;
}
inline const uint160 operator-(const uint160& a, const uint160& b)
{
    return (base_uint160)a - (base_uint160)b;
}

//////////////////////////////////////////////////////////////////////////////
//
// uint256
//

/** 256-bit unsigned integer */
class uint256 : public base_uint256
{
public:
    typedef base_uint256 basetype;

    uint256()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint256(const uint32 h, const base_uint224& l)
    {
        for (int i = 0; i < WIDTH - 1; i++)
            pn[i] = l.pn[i];
        pn[WIDTH - 1] = h;
    }

    uint256(const int8 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const uint8 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const int16 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const uint16 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const int32 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const uint32 b)
    {
        pn[0] = b;
        for (int i = 1; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const int64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const uint64 b[4])
    {
        for (int i = 0; i < 4; i++)
        {
            pn[2 * i] = (unsigned int)b[i];
            pn[2 * i + 1] = (unsigned int)(b[i] >> 32);
        }
    }

    uint256(const uint32 nBase, const uint32 nPower)
    {
        uint64 b = (uint64)std::pow(nBase, nPower);
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const uint16 a, const uint32 b, const uint256& n)
    {
        for (int i = 0; i < WIDTH - 1; i++)
            pn[i] = n.pn[i];
        pn[WIDTH - 1] = (((uint32)a << 16) | (b >> 16));
        pn[WIDTH - 2] = ((b << 16) | (pn[WIDTH - 2] >> 16));
    }

    explicit uint256(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint256(const std::vector<unsigned char>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }

    explicit uint256(const unsigned char* data, const std::size_t len)
    {
        if (data && len == sizeof(pn))
            memcpy(pn, data, len);
        else
            *this = 0;
    }

    uint256& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint256& operator=(uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    boost::multiprecision::uint256_t GetMuint256() const
    {
        boost::multiprecision::uint256_t m;
        for (int i = WIDTH - 1; i >= 0; i--)
        {
            m += Get32(i);
            if (i > 0)
            {
                m <<= 32;
            }
        }
        return m;
    }

    void SetMuint256(const boost::multiprecision::uint256_t& m)
    {
        boost::multiprecision::uint256_t t(m);
        for (int i = 0; i < WIDTH; i++)
        {
            pn[i] = (uint32_t)(t & std::numeric_limits<uint32_t>::max());
            t >>= 32;
        }
    }
};

inline bool operator==(const uint256& a, uint64 b)
{
    return (base_uint256)a == b;
}
inline bool operator!=(const uint256& a, uint64 b)
{
    return (base_uint256)a != b;
}
inline const uint256 operator<<(const base_uint256& a, unsigned int shift)
{
    return uint256(a) <<= shift;
}
inline const uint256 operator>>(const base_uint256& a, unsigned int shift)
{
    return uint256(a) >>= shift;
}
inline const uint256 operator<<(const uint256& a, unsigned int shift)
{
    return uint256(a) <<= shift;
}
inline const uint256 operator>>(const uint256& a, unsigned int shift)
{
    return uint256(a) >>= shift;
}

inline const uint256 operator^(const base_uint256& a, const base_uint256& b)
{
    return uint256(a) ^= b;
}
inline const uint256 operator&(const base_uint256& a, const base_uint256& b)
{
    return uint256(a) &= b;
}
inline const uint256 operator|(const base_uint256& a, const base_uint256& b)
{
    return uint256(a) |= b;
}
inline const uint256 operator+(const base_uint256& a, const base_uint256& b)
{
    return uint256(a) += b;
}
inline const uint256 operator-(const base_uint256& a, const base_uint256& b)
{
    return uint256(a) -= b;
}

inline bool operator<(const base_uint256& a, const uint256& b)
{
    return (base_uint256)a < (base_uint256)b;
}
inline bool operator<=(const base_uint256& a, const uint256& b)
{
    return (base_uint256)a <= (base_uint256)b;
}
inline bool operator>(const base_uint256& a, const uint256& b)
{
    return (base_uint256)a > (base_uint256)b;
}
inline bool operator>=(const base_uint256& a, const uint256& b)
{
    return (base_uint256)a >= (base_uint256)b;
}
inline bool operator==(const base_uint256& a, const uint256& b)
{
    return (base_uint256)a == (base_uint256)b;
}
inline bool operator!=(const base_uint256& a, const uint256& b)
{
    return (base_uint256)a != (base_uint256)b;
}
inline const uint256 operator^(const base_uint256& a, const uint256& b)
{
    return (base_uint256)a ^ (base_uint256)b;
}
inline const uint256 operator&(const base_uint256& a, const uint256& b)
{
    return (base_uint256)a & (base_uint256)b;
}
inline const uint256 operator|(const base_uint256& a, const uint256& b)
{
    return (base_uint256)a | (base_uint256)b;
}
inline const uint256 operator+(const base_uint256& a, const uint256& b)
{
    return (base_uint256)a + (base_uint256)b;
}
inline const uint256 operator-(const base_uint256& a, const uint256& b)
{
    return (base_uint256)a - (base_uint256)b;
}

inline bool operator<(const uint256& a, const base_uint256& b)
{
    return (base_uint256)a < (base_uint256)b;
}
inline bool operator<=(const uint256& a, const base_uint256& b)
{
    return (base_uint256)a <= (base_uint256)b;
}
inline bool operator>(const uint256& a, const base_uint256& b)
{
    return (base_uint256)a > (base_uint256)b;
}
inline bool operator>=(const uint256& a, const base_uint256& b)
{
    return (base_uint256)a >= (base_uint256)b;
}
inline bool operator==(const uint256& a, const base_uint256& b)
{
    return (base_uint256)a == (base_uint256)b;
}
inline bool operator!=(const uint256& a, const base_uint256& b)
{
    return (base_uint256)a != (base_uint256)b;
}
inline const uint256 operator^(const uint256& a, const base_uint256& b)
{
    return (base_uint256)a ^ (base_uint256)b;
}
inline const uint256 operator&(const uint256& a, const base_uint256& b)
{
    return (base_uint256)a & (base_uint256)b;
}
inline const uint256 operator|(const uint256& a, const base_uint256& b)
{
    return (base_uint256)a | (base_uint256)b;
}
inline const uint256 operator+(const uint256& a, const base_uint256& b)
{
    return (base_uint256)a + (base_uint256)b;
}
inline const uint256 operator-(const uint256& a, const base_uint256& b)
{
    return (base_uint256)a - (base_uint256)b;
}

inline bool operator<(const uint256& a, const uint256& b)
{
    return (base_uint256)a < (base_uint256)b;
}
inline bool operator<=(const uint256& a, const uint256& b)
{
    return (base_uint256)a <= (base_uint256)b;
}
inline bool operator>(const uint256& a, const uint256& b)
{
    return (base_uint256)a > (base_uint256)b;
}
inline bool operator>=(const uint256& a, const uint256& b)
{
    return (base_uint256)a >= (base_uint256)b;
}
inline bool operator==(const uint256& a, const uint256& b)
{
    return (base_uint256)a == (base_uint256)b;
}
inline bool operator!=(const uint256& a, const uint256& b)
{
    return (base_uint256)a != (base_uint256)b;
}
inline const uint256 operator^(const uint256& a, const uint256& b)
{
    return (base_uint256)a ^ (base_uint256)b;
}
inline const uint256 operator&(const uint256& a, const uint256& b)
{
    return (base_uint256)a & (base_uint256)b;
}
inline const uint256 operator|(const uint256& a, const uint256& b)
{
    return (base_uint256)a | (base_uint256)b;
}
inline const uint256 operator+(const uint256& a, const uint256& b)
{
    return (base_uint256)a + (base_uint256)b;
}
inline const uint256 operator-(const uint256& a, const uint256& b)
{
    return (base_uint256)a - (base_uint256)b;
}
inline const uint256 operator*(const uint256& a, const uint256& b)
{
    boost::multiprecision::uint256_t ma;
    for (int i = 7; i >= 0; i--)
    {
        ma += a.Get32(i);
        if (i > 0)
        {
            ma <<= 32;
        }
    }
    boost::multiprecision::uint256_t mb;
    for (int i = 7; i >= 0; i--)
    {
        mb += b.Get32(i);
        if (i > 0)
        {
            mb <<= 32;
        }
    }
    ma *= mb;

    uint256 c;
    for (int i = 0; i < 8; i++)
    {
        c[i] = (uint32_t)(ma & std::numeric_limits<uint32_t>::max());
        ma >>= 32;
    }
    return c;
}
inline const uint256 operator/(const uint256& a, const uint256& b)
{
    boost::multiprecision::uint256_t ma;
    for (int i = 7; i >= 0; i--)
    {
        ma += a.Get32(i);
        if (i > 0)
        {
            ma <<= 32;
        }
    }
    boost::multiprecision::uint256_t mb;
    for (int i = 7; i >= 0; i--)
    {
        mb += b.Get32(i);
        if (i > 0)
        {
            mb <<= 32;
        }
    }
    ma /= mb;

    uint256 c;
    for (int i = 0; i < 8; i++)
    {
        c[i] = (uint32_t)(ma & std::numeric_limits<uint32_t>::max());
        ma >>= 32;
    }
    return c;
}

//////////////////////////////////////////////////////////////////////////////
//
// uint224
//

/** 224-bit unsigned integer */
class uint224 : public base_uint224
{
public:
    typedef base_uint224 basetype;

    uint224()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint224(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint224(const uint256& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint224& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint224(const uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint224& operator=(uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    explicit uint224(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint224(const std::vector<unsigned char>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }
};

inline bool operator<(const base_uint224& a, const uint224& b)
{
    return (base_uint224)a < (base_uint224)b;
}
inline bool operator<=(const base_uint224& a, const uint224& b)
{
    return (base_uint224)a <= (base_uint224)b;
}
inline bool operator>(const base_uint224& a, const uint224& b)
{
    return (base_uint224)a > (base_uint224)b;
}
inline bool operator>=(const base_uint224& a, const uint224& b)
{
    return (base_uint224)a >= (base_uint224)b;
}
inline bool operator==(const base_uint224& a, const uint224& b)
{
    return (base_uint224)a == (base_uint224)b;
}
inline bool operator!=(const base_uint224& a, const uint224& b)
{
    return (base_uint224)a != (base_uint224)b;
}
inline bool operator<(const uint224& a, const base_uint224& b)
{
    return (base_uint224)a < (base_uint224)b;
}
inline bool operator<=(const uint224& a, const base_uint224& b)
{
    return (base_uint224)a <= (base_uint224)b;
}
inline bool operator>(const uint224& a, const base_uint224& b)
{
    return (base_uint224)a > (base_uint224)b;
}
inline bool operator>=(const uint224& a, const base_uint224& b)
{
    return (base_uint224)a >= (base_uint224)b;
}
inline bool operator==(const uint224& a, const base_uint224& b)
{
    return (base_uint224)a == (base_uint224)b;
}
inline bool operator!=(const uint224& a, const base_uint224& b)
{
    return (base_uint224)a != (base_uint224)b;
}
inline bool operator<(const uint224& a, const uint224& b)
{
    return (base_uint224)a < (base_uint224)b;
}
inline bool operator<=(const uint224& a, const uint224& b)
{
    return (base_uint224)a <= (base_uint224)b;
}
inline bool operator>(const uint224& a, const uint224& b)
{
    return (base_uint224)a > (base_uint224)b;
}
inline bool operator>=(const uint224& a, const uint224& b)
{
    return (base_uint224)a >= (base_uint224)b;
}
inline bool operator==(const uint224& a, const uint224& b)
{
    return (base_uint224)a == (base_uint224)b;
}
inline bool operator!=(const uint224& a, const uint224& b)
{
    return (base_uint224)a != (base_uint224)b;
}

//////////////////////////////////////////////////////////////////////////////
//
// uint2048
//

/** 2048-bit unsigned integer */
class uint2048 : public base_uint2048
{
public:
    typedef base_uint2048 basetype;

    uint2048()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint2048(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint2048(const uint256& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint2048& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint2048(const uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint2048& operator=(uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    explicit uint2048(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint2048(const std::vector<unsigned char>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }
};

inline bool operator<(const base_uint2048& a, const uint2048& b)
{
    return (base_uint2048)a < (base_uint2048)b;
}
inline bool operator<=(const base_uint2048& a, const uint2048& b)
{
    return (base_uint2048)a <= (base_uint2048)b;
}
inline bool operator>(const base_uint2048& a, const uint2048& b)
{
    return (base_uint2048)a > (base_uint2048)b;
}
inline bool operator>=(const base_uint2048& a, const uint2048& b)
{
    return (base_uint2048)a >= (base_uint2048)b;
}
inline bool operator==(const base_uint2048& a, const uint2048& b)
{
    return (base_uint2048)a == (base_uint2048)b;
}
inline bool operator!=(const base_uint2048& a, const uint2048& b)
{
    return (base_uint2048)a != (base_uint2048)b;
}
inline bool operator<(const uint2048& a, const base_uint2048& b)
{
    return (base_uint2048)a < (base_uint2048)b;
}
inline bool operator<=(const uint2048& a, const base_uint2048& b)
{
    return (base_uint2048)a <= (base_uint2048)b;
}
inline bool operator>(const uint2048& a, const base_uint2048& b)
{
    return (base_uint2048)a > (base_uint2048)b;
}
inline bool operator>=(const uint2048& a, const base_uint2048& b)
{
    return (base_uint2048)a >= (base_uint2048)b;
}
inline bool operator==(const uint2048& a, const base_uint2048& b)
{
    return (base_uint2048)a == (base_uint2048)b;
}
inline bool operator!=(const uint2048& a, const base_uint2048& b)
{
    return (base_uint2048)a != (base_uint2048)b;
}
inline bool operator<(const uint2048& a, const uint2048& b)
{
    return (base_uint2048)a < (base_uint2048)b;
}
inline bool operator<=(const uint2048& a, const uint2048& b)
{
    return (base_uint2048)a <= (base_uint2048)b;
}
inline bool operator>(const uint2048& a, const uint2048& b)
{
    return (base_uint2048)a > (base_uint2048)b;
}
inline bool operator>=(const uint2048& a, const uint2048& b)
{
    return (base_uint2048)a >= (base_uint2048)b;
}
inline bool operator==(const uint2048& a, const uint2048& b)
{
    return (base_uint2048)a == (base_uint2048)b;
}
inline bool operator!=(const uint2048& a, const uint2048& b)
{
    return (base_uint2048)a != (base_uint2048)b;
}

#endif // CRYPTO_UINT256_H
