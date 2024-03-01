// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_MODE_MODE_TYPE_H
#define METABASENET_MODE_MODE_TYPE_H

namespace metabasenet
{
// mode type
enum class EModeType
{
    MODE_ERROR = 0, // ERROR type
    MODE_SERVER,    // server
    MODE_CONSOLE,   // console
    MODE_MINER,     // miner
};

} // namespace metabasenet

#endif // METABASENET_MODE_MODE_TYPE_H
