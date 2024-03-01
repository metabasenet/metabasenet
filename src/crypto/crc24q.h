// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CRYPTO_CRC24Q_H
#define CRYPTO_CRC24Q_H

namespace metabasenet
{
namespace crypto
{

// CRC-24Q cyclic redundancy checksum
// polynomial : x^24+ x^23+ x^18+ x^17+ x^14+ x^11+ x^10+ x^7+ x^6+ x^5+ x^4+ x^3+ x+1

unsigned int crc24q(const unsigned char* data, int size);

} // namespace crypto
} // namespace metabasenet

#endif // CRYPTO_CRC24Q_H
