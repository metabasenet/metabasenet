// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitmap.h"

namespace mtbase
{

//////////////////////////
// CBitmap

bool CBitmap::Initialize(const uint32 nBitsIn)
{
    varBits.SetValue(0);
    btBitmap.clear();
    if (nBitsIn == 0)
    {
        return false;
    }
    uint32 nByteCount = nBitsIn / 8;
    if (nBitsIn % 8 > 0)
    {
        nByteCount++;
    }
    btBitmap.clear();
    btBitmap.resize(nByteCount);
    varBits.SetValue(nBitsIn);
    return true;
}

bool CBitmap::ImportBytes(const bytes& btBitmap)
{
    try
    {
        mtbase::CBufStream ss(btBitmap);
        ss >> *this;
    }
    catch (exception& e)
    {
        return false;
    }
    return true;
}

bool CBitmap::ImportString(const string& strBitmap)
{
    try
    {
        mtbase::CBufStream ss(strBitmap);
        ss >> *this;
    }
    catch (exception& e)
    {
        return false;
    }
    return true;
}

bool CBitmap::IsNull() const
{
    if (varBits.GetValue() == 0 || !HasValidBit())
    {
        return true;
    }
    return false;
}

void CBitmap::Clear()
{
    varBits.SetValue(0);
    btBitmap.clear();
}

bool CBitmap::GetBit(const uint32 nBitPos) const
{
    if (nBitPos < varBits.GetValue())
    {
        return ((btBitmap[nBitPos / 8] & (0x01 << (nBitPos % 8))) != 0);
    }
    return false;
}

void CBitmap::SetBit(const uint32 nBitPos)
{
    if (nBitPos < varBits.GetValue())
    {
        btBitmap[nBitPos / 8] |= (0x01 << (nBitPos % 8));
    }
}

void CBitmap::UnsetBit(const uint32 nBitPos)
{
    if (nBitPos < varBits.GetValue())
    {
        btBitmap[nBitPos / 8] &= ~(0x01 << (nBitPos % 8));
    }
}

bool CBitmap::HasValidBit() const
{
    for (auto& b : btBitmap)
    {
        if (b != 0)
        {
            return true;
        }
    }
    return false;
}

uint32 CBitmap::GetMaxBits() const
{
    return varBits.GetValue();
}

uint32 CBitmap::GetValidBits() const
{
    vector<uint32> vIndexList;
    GetIndexList(vIndexList);
    return (uint32)(vIndexList.size());
}

bytes CBitmap::GetBytes() const
{
    mtbase::CBufStream ss;
    ss << *this;

    bytes btOut;
    ss.GetData(btOut);
    return btOut;
}

void CBitmap::GetBytes(bytes& btOut) const
{
    btOut = GetBytes();
}

void CBitmap::GetIndexList(vector<uint32>& vIndexList) const
{
    for (uint32 i = 0; i < btBitmap.size(); i++)
    {
        uint8 b = btBitmap[i];
        if (b != 0)
        {
            for (uint32 j = 0; j < 8; j++)
            {
                if (b & 0x01)
                {
                    vIndexList.push_back(i * 8 + j);
                }
                b >>= 1;
            }
        }
    }
}

string CBitmap::GetBitmapString() const
{
    vector<uint32> vIndexList;
    GetIndexList(vIndexList);

    string strList;
    for (auto& index : vIndexList)
    {
        char sTempBuf[12] = { 0 };
        if (strList.empty())
        {
            sprintf(sTempBuf, "%d", index);
        }
        else
        {
            sprintf(sTempBuf, ",%d", index);
        }
        strList += sTempBuf;
    }

    char sTempBuf[12] = { 0 };
    sprintf(sTempBuf, "[%ld]: ", varBits.GetValue());

    string strOut;
    strOut += sTempBuf;
    strOut += strList;
    return strOut;
}

} // namespace mtbase
