// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_BITMAP_H
#define MTBASE_BITMAP_H

#include <stream/stream.h>

#include "type.h"

using namespace std;

namespace mtbase
{

//////////////////////////
// CBitmap

class CBitmap
{
    friend class mtbase::CStream;

public:
    CBitmap()
      : varBits(0) {}
    CBitmap(const uint32 nBitsIn)
    {
        Initialize(nBitsIn);
    }
    CBitmap(const uint32 nBitsIn, const bytes& btBitmapIn)
    {
        Initialize(nBitsIn);
        size_t nMinSize = min(btBitmap.size(), btBitmapIn.size());
        for (size_t i = 0; i < nMinSize; i++)
        {
            btBitmap[i] = btBitmapIn[i];
        }
    }
    CBitmap(const CBitmap& btm)
      : varBits(btm.varBits), btBitmap(btm.btBitmap) {}

    bool Initialize(const uint32 nBitsIn);
    bool ImportBytes(const bytes& btBitmap);
    bool ImportString(const string& strBitmap);
    bool IsNull() const;
    void Clear();

    bool GetBit(const uint32 nBitPos) const;
    void SetBit(const uint32 nBitPos);
    void UnsetBit(const uint32 nBitPos);
    bool HasValidBit() const;

    uint32 GetMaxBits() const;
    uint32 GetValidBits() const;

    bytes GetBytes() const;
    void GetBytes(bytes& btOut) const;

    void GetIndexList(vector<uint32>& vIndexList) const;
    string GetBitmapString() const;

    CBitmap& operator=(const CBitmap& b)
    {
        varBits.SetValue(b.varBits.GetValue());
        btBitmap = b.btBitmap;
        return *this;
    }

    CBitmap& operator&=(const CBitmap& b)
    {
        const size_t nSize = min(btBitmap.size(), b.btBitmap.size());
        uint8* ap = btBitmap.data();
        const uint8* bp = b.btBitmap.data();
        for (size_t i = 0; i < nSize; i++)
        {
            ap[i] &= bp[i];
        }
        return *this;
    }

    CBitmap& operator|=(const CBitmap& b)
    {
        const size_t nSize = min(btBitmap.size(), b.btBitmap.size());
        uint8* ap = btBitmap.data();
        const uint8* bp = b.btBitmap.data();
        for (size_t i = 0; i < nSize; i++)
        {
            ap[i] |= bp[i];
        }
        return *this;
    }

    CBitmap& operator^=(const CBitmap b)
    {
        const size_t nSize = min(btBitmap.size(), b.btBitmap.size());
        uint8* ap = btBitmap.data();
        const uint8* bp = b.btBitmap.data();
        for (size_t i = 0; i < nSize; i++)
        {
            ap[i] ^= bp[i];
        }
        return *this;
    }

    friend inline bool operator==(const CBitmap& a, const CBitmap& b)
    {
        if (a.varBits != b.varBits)
        {
            return false;
        }
        if (a.btBitmap != b.btBitmap)
        {
            return false;
        }
        return true;
    }

    friend inline bool operator!=(const CBitmap& a, const CBitmap& b)
    {
        return (!(a == b));
    }

protected:
    CVarInt varBits;
    bytes btBitmap;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(varBits, opt);
        s.Serialize(btBitmap, opt);
    }
};

} // namespace mtbase

#endif