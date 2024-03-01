
#ifndef ETH_COMMON_UTIL_H
#define ETH_COMMON_UTIL_H

#include <libdevcore/Address.h>
#include <libdevcore/Common.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/SHA3.h>

namespace dev
{

inline std::string reverse_hex_string(const std::string& strIn)
{
    //0x9
    //0x1234
    //0x123

    std::string strOut;
    strOut.resize(strIn.size() + (strIn.size() % 2));
    std::size_t b = 2;
    std::size_t e = strIn.size() - 1;
    strOut[0] = '0';
    strOut[1] = 'x';
    while (e >= 2)
    {
        strOut[b + 1] = strIn[e];
        if (e - 1 < 2)
        {
            strOut[b] = '0';
            break;
        }
        strOut[b] = strIn[e - 1];
        e -= 2;
        b += 2;
    }
    return strOut;
}

inline std::string u256_to_hex(const u256& v)
{
    std::string str = v.str(0, std::ios::hex | std::ios::showbase);
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

inline std::string h256_to_hex(const h256& h)
{
    return (std::string("0x") + h.hex());
}

inline std::string address_to_hex(const Address& a)
{
    return (std::string("0x") + a.hex());
}

inline std::string reverse_u256_string(const u256& v)
{
    return reverse_hex_string(u256_to_hex(v));
}

inline u256 reverse_string_u256(const std::string& strIn)
{
    return u256(reverse_hex_string(strIn));
}

inline bytes sha3_bytes(const bytes& btData)
{
    h256 h = sha3(btData);
    return bytes(h.begin(), h.end());
}

} // namespace dev

#endif // ETH_COMMON_UTIL_H
