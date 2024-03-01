// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_BLOOMFILTER_H
#define MTBASE_BLOOMFILTER_H

#include <bitset>
#include <string.h>
#include <vector>

#include "MurmurHash3.h"

namespace mtbase
{

#define BF_M 5

class CNhBloomFilter
{
public:
    CNhBloomFilter(const std::size_t _Nb = 32)
    {
        nNb = _Nb;
        if (nNb < 32)
        {
            nNb = 32;
        }
        if (nNb % 8 != 0)
        {
            nNb = (nNb / 8 + 1) * 8;
        }
        btBit.resize(nNb / 8);
        nAddBits = 0;
    }
    CNhBloomFilter(const std::vector<unsigned char>& vBfData)
    {
        SetData(vBfData);
    }
    ~CNhBloomFilter()
    {
    }

    void Add(const unsigned char* pData, const std::size_t nSize)
    {
        if (pData && nSize > 0)
        {
            uint32_t seed = 0;
            for (std::size_t i = 0; i < BF_M; i++)
            {
                MurmurHash3_x86_32(pData, nSize, seed, &seed);
                AddBit(seed);
            }
        }
    }
    void Add(const std::vector<unsigned char>& vBfData)
    {
        Add(vBfData.data(), vBfData.size());
    }
    bool Test(const unsigned char* pData, const size_t nSize)
    {
        if (pData && nSize > 0)
        {
            uint32_t seed = 0;
            for (std::size_t i = 0; i < BF_M; i++)
            {
                MurmurHash3_x86_32(pData, nSize, seed, &seed);
                if (!TestBit(seed))
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
    bool Test(const std::vector<unsigned char>& vBfData)
    {
        return Test(vBfData.data(), vBfData.size());
    }
    void Reset()
    {
        btBit.clear();
        nNb = 0;
        nAddBits = 0;
    }
    bool IsNull() const
    {
        return (nAddBits == 0);
    }
    std::size_t GetBitSize() const
    {
        return nNb;
    }
    std::size_t GetSize() const
    {
        return btBit.size();
    }
    bool GetData(const unsigned char* pBfData, std::size_t& nBfSize) const
    {
        if (pBfData == nullptr || nBfSize < btBit.size())
        {
            return false;
        }
        memcpy((void*)pBfData, (void*)btBit.data(), btBit.size());
        nBfSize = btBit.size();
        return true;
    }
    void GetData(std::vector<unsigned char>& vBfData) const
    {
        vBfData.resize(btBit.size());
        memcpy((unsigned char*)&(vBfData[0]), (void*)btBit.data(), btBit.size());
    }
    std::vector<unsigned char> GetData() const
    {
        std::vector<unsigned char> vBfData;
        GetData(vBfData);
        return vBfData;
    }
    void SetData(const unsigned char* pBfData, const std::size_t nBfSize)
    {
        btBit.clear();
        nNb = 0;
        nAddBits = 0;
        if (pBfData && nBfSize > 0)
        {
            btBit.resize(nBfSize);
            memcpy((unsigned char*)btBit.data(), pBfData, nBfSize);
            nNb = nBfSize * 8;
        }
    }
    void SetData(const std::vector<unsigned char>& vBfData)
    {
        SetData(vBfData.data(), vBfData.size());
    }

private:
    void AddBit(const std::size_t pos)
    {
        std::size_t sp = pos % nNb;
        btBit[sp / 8] |= (0x01 << (sp % 8));
        nAddBits++;
    }
    bool TestBit(const std::size_t pos)
    {
        std::size_t sp = pos % nNb;
        return (btBit[sp / 8] & (0x01 << (sp % 8)));
    }

private:
    std::vector<unsigned char> btBit;
    std::size_t nNb;
    std::size_t nAddBits;
};

} // namespace mtbase

#endif // MTBASE_BLOOMFILTER_H