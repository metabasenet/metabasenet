// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "param.h"

#include "util.h"

using namespace mtbase;

namespace metabasenet
{

//////////////////////////////
bool TESTNET_FLAG = false;
bool FASTTEST_FLAG = false;
bool TESTMAINNET_FLAG = false;
CChainId GENESIS_CHAINID = 0;
uint32 NETWORK_NETID = 0;

//////////////////////////////
bytes getFunctionContractCreateCode()
{
    return ParseHexString("6080604052348015600f57600080fd5b50603f80601d6000396000f3fe6080604052600080fdfea26469706673582212202ea6f39d821dc6f0cd0f6a99ca7ce9f154c082712f949eaa74b785e16926cb5c64736f6c63430008120033");
}

bytes getFunctionContractRuntimeCode()
{
    return ParseHexString("6080604052600080fdfea26469706673582212202ea6f39d821dc6f0cd0f6a99ca7ce9f154c082712f949eaa74b785e16926cb5c64736f6c63430008120033");
}

//////////////////////////////
std::string CoinToTokenBigFloat(const uint256& nCoin)
{
    try
    {
        if (nCoin == 0)
        {
            return std::string("0.0");
        }
        boost::multiprecision::uint256_t mc((uint64)std::pow(10, TOKEN_DECIMAL_DIGIT));
        boost::multiprecision::uint256_t n = nCoin.GetMuint256();
        boost::multiprecision::uint256_t nInteger = n / mc;
        boost::multiprecision::uint256_t nDecimal = n % mc;
        std::string strInt = nInteger.str();
        std::string strDec = nDecimal.str();
        int nDecCount = (int)strDec.size();
        for (int i = nDecCount - 1; i >= 0; --i)
        {
            if (strDec[i] != '0')
            {
                break;
            }
            nDecCount--;
        }
        if (nDecCount == 0)
        {
            return strInt + std::string(".0");
        }
        std::string strDecimalZero;
        if (strDec.size() < TOKEN_DECIMAL_DIGIT)
        {
            strDecimalZero.resize(TOKEN_DECIMAL_DIGIT - strDec.size(), '0');
        }
        if (nDecCount == (int)strDec.size())
        {
            strDecimalZero += strDec;
        }
        else
        {
            strDecimalZero += strDec.substr(0, nDecCount);
        }
        return (strInt + std::string(".") + strDecimalZero);
    }
    catch (std::exception& e)
    {
        return std::string("0.00");
    }
}

bool TokenBigFloatToCoin(const std::string& strToken, uint256& nCoin)
{
    std::string strInteger;
    std::string strDecimal;
    if (!SplitNumber(strToken, strInteger, strDecimal))
    {
        return false;
    }

    int64 nZeroCount = 0;
    for (std::size_t i = 0; i < strInteger.size(); i++)
    {
        if (strInteger[i] != '0')
        {
            break;
        }
        nZeroCount++;
    }
    if (nZeroCount > 0)
    {
        if (strInteger.size() > nZeroCount)
        {
            strInteger = strInteger.substr(nZeroCount, strInteger.size() - nZeroCount);
        }
        else
        {
            strInteger = "";
        }
    }

    if (strDecimal.size() > TOKEN_DECIMAL_DIGIT)
    {
        strDecimal = strDecimal.substr(0, TOKEN_DECIMAL_DIGIT);
    }
    else if (strDecimal.size() < TOKEN_DECIMAL_DIGIT)
    {
        strDecimal.append(TOKEN_DECIMAL_DIGIT - strDecimal.size(), '0');
    }
    nZeroCount = 0;
    for (std::size_t i = 0; i < strDecimal.size(); i++)
    {
        if (strDecimal[i] != '0')
        {
            break;
        }
        nZeroCount++;
    }
    if (nZeroCount > 0)
    {
        if (strDecimal.size() > nZeroCount)
        {
            strDecimal = strDecimal.substr(nZeroCount, strDecimal.size() - nZeroCount);
        }
        else
        {
            strDecimal = "";
        }
    }

    boost::multiprecision::uint256_t mc((uint64)std::pow(10, TOKEN_DECIMAL_DIGIT));
    boost::multiprecision::uint256_t nInt(strInteger);
    boost::multiprecision::uint256_t nDec(strDecimal);

    nInt *= mc;
    nInt += nDec;

    nCoin.SetMuint256(nInt);
    return true;
}

uint256 TokenBigFloatToCoin(const std::string& strToken)
{
    uint256 out;
    if (TokenBigFloatToCoin(strToken, out))
    {
        return out;
    }
    return 0;
}

} // namespace metabasenet
