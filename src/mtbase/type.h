// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_TYPE_H
#define MTBASE_TYPE_H

#include <boost/cstdint.hpp>
#include <boost/thread/thread.hpp>
#include <map>
#include <tuple>
#include <variant>
#include <vector>

typedef boost::int8_t int8;
typedef boost::uint8_t uint8;
typedef boost::int16_t int16;
typedef boost::uint16_t uint16;
typedef boost::int32_t int32;
typedef boost::uint32_t uint32;
typedef boost::int64_t int64;
typedef boost::uint64_t uint64;

using bytes = std::vector<uint8>;
using bytesmap = std::map<bytes, bytes>;

using CChainId = uint32;
using ReadWriteLock = boost::shared_mutex;
using writeSharedLock = boost::unique_lock<boost::shared_mutex>;
using readSharedLock = boost::shared_lock<boost::shared_mutex>;

#ifdef _MSC_VER

#define __PRETTY_FUNCTION__ __FUNCSIG__
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#endif //MTBASE_TYPE_H
