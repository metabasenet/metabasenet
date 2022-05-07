// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HCODE_VERSION_H
#define HCODE_VERSION_H

namespace hcode
{
#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#define _TOSTR(s) #s
#define _VERSTR(major, minor) \
    _TOSTR(major)             \
    "." _TOSTR(minor)
#define VERSION_STRING _VERSTR(VERSION_MAJOR, VERSION_MINOR)

} // namespace hcode

#endif //HCODE_VERSION_H
