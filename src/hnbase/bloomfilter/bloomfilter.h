// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XENGINE_BLOOMFILTER_BLOOMFILTER_H
#define XENGINE_BLOOMFILTER_BLOOMFILTER_H

#include <bitset>
#include <string.h>
#include <vector>

#include "MurmurHash3.h"

namespace hnbase
{

#define BF_M 5

template <std::size_t _Nb>
class CBloomFilter
{
public:
    CBloomFilter()
    {
        pbv = new std::bitset<_Nb>();
        pbv->reset();
    }
    ~CBloomFilter()
    {
        delete pbv;
        pbv = nullptr;
    }

    void Add(const unsigned char* pData, const std::size_t nSize)
    {
        if (pData && nSize > 0)
        {
            uint32_t seed = 0;
            for (std::size_t i = 0; i < BF_M; i++)
            {
                MurmurHash3_x86_32(pData, nSize, seed, &seed);
                pbv->set(seed % _Nb);
            }
        }
    }
    void Add(const std::vector<unsigned char>& vBfData)
    {
        if (!vBfData.empty())
        {
            uint32_t seed = 0;
            for (std::size_t i = 0; i < BF_M; i++)
            {
                MurmurHash3_x86_32((unsigned char*)&(vBfData[0]), vBfData.size(), seed, &seed);
                pbv->set(seed % _Nb);
            }
        }
    }
    bool Test(const unsigned char* pData, const size_t nSize)
    {
        if (pData && nSize > 0)
        {
            uint32_t seed = 0;
            for (std::size_t i = 0; i < BF_M; i++)
            {
                MurmurHash3_x86_32(pData, nSize, seed, &seed);
                if (!pbv->test(seed % _Nb))
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
        if (!vBfData.empty())
        {
            uint32_t seed = 0;
            for (std::size_t i = 0; i < BF_M; i++)
            {
                MurmurHash3_x86_32((unsigned char*)&(vBfData[0]), vBfData.size(), seed, &seed);
                if (!pbv->test(seed % _Nb))
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
    void Reset()
    {
        pbv->reset();
    }
    std::size_t GetBitSize()
    {
        return _Nb;
    }
    std::size_t GetSize()
    {
        return sizeof(*pbv);
    }
    bool GetData(const unsigned char* pBfData, std::size_t& nBfSize)
    {
        if (pBfData == nullptr || nBfSize < sizeof(*pbv))
        {
            return false;
        }
        memcpy((void*)pBfData, (void*)pbv, sizeof(*pbv));
        nBfSize = sizeof(*pbv);
        return true;
    }
    void GetData(std::vector<unsigned char>& vBfData)
    {
        vBfData.resize(sizeof(*pbv));
        memcpy((unsigned char*)&(vBfData[0]), (unsigned char*)pbv, sizeof(*pbv));
    }
    void SetData(const unsigned char* pBfData, const std::size_t nBfSize)
    {
        pbv->reset();
        if (pBfData && nBfSize > 0)
        {
            std::size_t nCpSize = nBfSize;
            if (nCpSize > sizeof(*pbv))
            {
                nCpSize = sizeof(*pbv);
            }
            memcpy((unsigned char*)pbv, pBfData, nCpSize);
        }
    }
    void SetData(const std::vector<unsigned char>& vBfData)
    {
        pbv->reset();
        if (!vBfData.empty())
        {
            std::size_t nCpSize = vBfData.size();
            if (nCpSize > sizeof(*pbv))
            {
                nCpSize = sizeof(*pbv);
            }
            memcpy((unsigned char*)pbv, (unsigned char*)&(vBfData[0]), nCpSize);
        }
    }

private:
    std::bitset<_Nb>* pbv;
};

} // namespace hnbase

#endif //XENGINE_BLOOMFILTER_BLOOMFILTER_H