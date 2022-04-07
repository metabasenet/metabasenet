// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_PROOF_H
#define COMMON_PROOF_H

#include "destination.h"
#include "key.h"
#include "uint256.h"
#include "util.h"

class CProofOfSecretShare
{
public:
    unsigned char nWeight;
    uint256 nAgreement;

public:
    void Save(std::vector<unsigned char>& vchProof)
    {
        hnbase::CBufStream os;
        ToStream(os);
        os.GetData(vchProof);
    }
    bool Load(const std::vector<unsigned char>& vchProof)
    {
        hnbase::CBufStream is(vchProof);
        try
        {
            FromStream(is);
        }
        catch (const std::exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
        return true;
    }
    size_t size()
    {
        std::vector<unsigned char> vchProof;
        Save(vchProof);
        return vchProof.size();
    }

protected:
    virtual void ToStream(hnbase::CBufStream& os)
    {
        os << nWeight << nAgreement;
    }
    virtual void FromStream(hnbase::CBufStream& is)
    {
        is >> nWeight >> nAgreement;
    }
};

class CProofOfPiggyback : public CProofOfSecretShare
{
public:
    uint256 hashRefBlock;

protected:
    virtual void ToStream(hnbase::CBufStream& os) override
    {
        CProofOfSecretShare::ToStream(os);
        os << hashRefBlock;
    }
    virtual void FromStream(hnbase::CBufStream& is) override
    {
        CProofOfSecretShare::FromStream(is);
        is >> hashRefBlock;
    }
};

class CProofOfHashWork : public CProofOfSecretShare
{
public:
    unsigned char nAlgo;
    unsigned char nBits;
    CDestination destMint;
    uint64_t nNonce;

protected:
    virtual void ToStream(hnbase::CBufStream& os) override
    {
        CProofOfSecretShare::ToStream(os);
        os << nAlgo << nBits << destMint << nNonce;
    }
    virtual void FromStream(hnbase::CBufStream& is) override
    {
        CProofOfSecretShare::FromStream(is);
        is >> nAlgo >> nBits >> destMint >> nNonce;
    }
};

class CConsensusParam
{
public:
    CConsensusParam()
      : nPrevTime(0), nPrevHeight(0), nPrevNumber(0), nPrevMintType(0), nWaitTime(0), fPow(false), ret(false) {}

    uint256 hashPrev;
    int64 nPrevTime;
    int nPrevHeight;
    uint32 nPrevNumber;
    uint16 nPrevMintType;
    int64 nWaitTime;
    bool fPow;
    bool ret;
};

#endif //COMMON_PROOF_H
