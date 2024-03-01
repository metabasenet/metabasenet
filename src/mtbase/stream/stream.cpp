// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stream.h"

namespace mtbase
{

///////////////////////////////
// CStream

CStream& CStream::Serialize(std::string& t, ObjectType&, SaveType&)
{
    *this << CVarInt(t.size());
    return Write((const char*)&t[0], t.size());
}

CStream& CStream::Serialize(std::string& t, ObjectType&, LoadType&)
{
    CVarInt var;
    *this >> var;
    t.resize(var.nValue);
    return Read((char*)&t[0], var.nValue);
}

CStream& CStream::Serialize(std::string& t, ObjectType&, std::size_t& serSize)
{
    CVarInt var(t.size());
    serSize += GetSerializeSize(var);
    serSize += t.size();
    return (*this);
}

///////////////////////////////
// CVarInt

std::size_t CVarInt::BytesRequired()
{
    std::size_t n = 0;
    uint64 _i = nValue;
    while (_i != 0)
    {
        ++n;
        _i >>= 8;
    }
    return n;
}

void CVarInt::GetBytes(bytes& out, std::size_t n)
{
    out.resize(n);
    uint8* b = &out.back();
    uint64 _i = nValue;
    for (; _i; _i >>= 8)
    {
        *(b--) = (uint8)(_i & 0xff);
    }
}

uint64 CVarInt::GetInt(const bytes& in)
{
    uint64 ret = 0;
    for (auto i : in)
    {
        ret = (uint64)((ret << 8) | (uint8)i);
    }
    return ret;
}

//----------------------------------------
void CVarInt::Serialize(CStream& s, SaveType&)
{
    if (nValue == 0)
    {
        s.Write((const char*)&nDataImmLenStart, 1);
    }
    else if (nValue < nDataImmLenStart)
    {
        s.Write((const char*)&nValue, 1);
    }
    else
    {
        bytes bt;
        std::size_t n = BytesRequired();
        GetBytes(bt, n);
        uint8 fn = nDataImmLenStart + n;
        s.Write((const char*)&fn, 1);
        s.Write((const char*)bt.data(), bt.size());
    }
}

void CVarInt::Serialize(CStream& s, LoadType&)
{
    uint8 n;
    s.Read((char*)&n, 1);
    if (n == nDataImmLenStart)
    {
        nValue = 0;
    }
    else if (n < nDataImmLenStart)
    {
        nValue = n;
    }
    else
    {
        uint8 fn = n - nDataImmLenStart;
        if (fn == 0 || fn > 8)
        {
            throw std::runtime_error("fn error");
        }
        bytes bt;
        bt.resize(fn);
        s.Read((char*)bt.data(), fn);
        nValue = GetInt(bt);
    }
}

void CVarInt::Serialize(CStream& s, std::size_t& serSize)
{
    if (nValue < nDataImmLenStart)
    {
        serSize += 1;
    }
    else
    {
        serSize += 1;
        serSize += BytesRequired();
    }
}

///////////////////////////////
// CBinary

void CBinary::Serialize(CStream& s, SaveType&)
{
    s.Write(pBuffer, nLength);
}

void CBinary::Serialize(CStream& s, LoadType&)
{
    s.Read(pBuffer, nLength);
}

void CBinary::Serialize(CStream& s, std::size_t& serSize)
{
    serSize += nLength;
}

} // namespace mtbase
