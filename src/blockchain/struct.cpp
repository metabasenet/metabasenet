// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "struct.h"

#include "crypto.h"
#include "mtbase.h"

using namespace mtbase;

namespace metabasenet
{

//////////////////////////////////////////////
// CCacheBlsPubkey

bool CCacheBlsPubkey::AddBlsPubkey(const uint256& hashBlock, const std::vector<uint384>& vPubkey)
{
    mtbase::CBufStream ss;
    ss << vPubkey;
    uint256 hash = crypto::CryptoHash(ss.GetData(), ss.GetSize());
    if (mapPubkey.count(hash) == 0)
    {
        if (nCreateIndex >= 0x7FFFFFFFFFFFFFFFL)
        {
            mapPubkey.clear();
            mapKeyIndex.clear();
            mapBlock.clear();
            nCreateIndex = 0;
        }
        else
        {
            while (mapPubkey.size() >= MAX_PUBKEY_CACHE_COUNT && mapKeyIndex.size() > 0)
            {
                const uint256& hashKey = mapKeyIndex.begin()->second;
                for (auto it = mapBlock.begin(); it != mapBlock.end();)
                {
                    if (it->second == hashKey)
                    {
                        mapBlock.erase(it++);
                    }
                    else
                    {
                        ++it;
                    }
                }
                mapPubkey.erase(hashKey);
                mapKeyIndex.erase(mapKeyIndex.begin());
            }
        }
        mapPubkey.insert(std::make_pair(hash, vPubkey));
        mapKeyIndex[++nCreateIndex] = hash;
    }

    auto nt = mapBlock.find(hashBlock);
    if (nt == mapBlock.end() || nt->second != hash)
    {
        while (mapBlock.size() >= MAX_BLOCK_CACHE_COUNT)
        {
            mapBlock.erase(mapBlock.begin());
        }
        mapBlock[hashBlock] = hash;
    }
    return true;
}

bool CCacheBlsPubkey::GetBlsPubkey(const uint256& hashBlock, std::vector<uint384>& vPubkey) const
{
    auto it = mapBlock.find(hashBlock);
    if (it != mapBlock.end())
    {
        auto mt = mapPubkey.find(it->second);
        if (mt != mapPubkey.end())
        {
            vPubkey = mt->second;
            return true;
        }
    }
    return false;
}

} // namespace metabasenet
