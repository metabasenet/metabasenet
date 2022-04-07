// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XENGINE_PEERNET_PEERINFO_H
#define XENGINE_PEERNET_PEERINFO_H

#include <string>

namespace hnbase
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

} // namespace hnbase

#endif //XENGINE_PEERNET_PEERINFO_H
