// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_PEERNET_PEERINFO_H
#define MTBASE_PEERNET_PEERINFO_H

#include <string>

namespace mtbase
{

class CPeerInfo
{
public:
    virtual ~CPeerInfo() {}

public:
    std::string strAddress;
    bool fInBound;
    int64 nActive;
    int64 nLastRecv;
    int64 nLastSend;
    int nScore;
};

} // namespace mtbase

#endif //MTBASE_PEERNET_PEERINFO_H
