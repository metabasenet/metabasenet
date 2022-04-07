//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef _XENGINE_MURMURHASH3_H_
#define _XENGINE_MURMURHASH3_H_

namespace hnbase
{

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

// Microsoft Visual Studio

#if defined(_MSC_VER)

typedef unsigned char uint8_t;
typedef unsigned long uint32_t;
typedef unsigned __int64 uint64_t;

// Other compilers

#else // defined(_MSC_VER)

#include <stdint.h>

#endif // !defined(_MSC_VER)

//-----------------------------------------------------------------------------

void MurmurHash3_x86_32(const void* key, int len, uint32_t seed, void* out);

void MurmurHash3_x86_128(const void* key, int len, uint32_t seed, void* out);

void MurmurHash3_x64_128(const void* key, int len, uint32_t seed, void* out);

//-----------------------------------------------------------------------------

} // namespace hnbase

#endif // _XENGINE_MURMURHASH3_H_
