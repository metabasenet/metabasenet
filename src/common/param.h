// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_PARAM_H
#define METABASENET_PARAM_H

#include "uint256.h"

///////////////////////////////////
extern bool TESTNET_FLAG;
extern bool FASTTEST_FLAG;

#define GET_PARAM(MAINNET_PARAM, TESTNET_PARAM) (TESTNET_FLAG ? TESTNET_PARAM : MAINNET_PARAM)
#define GET_FAST_PARAM(MAINNET_PARAM, TESTNET_PARAM, FASTTEST_PARAM) (TESTNET_FLAG ? (FASTTEST_FLAG ? FASTTEST_PARAM : TESTNET_PARAM) : MAINNET_PARAM)

///////////////////////////////////
static const uint32 TOKEN_DECIMAL_DIGIT = 18;
static const uint256 COIN(10, TOKEN_DECIMAL_DIGIT);
static const uint256 MIN_TX_FEE(100000000);
static const uint256 MIN_GAS_PRICE(10000);
static const uint256 TX_BASE_GAS(10000);
static const uint256 DATA_GAS_PER_BYTE(50);
static const uint256 DEF_TX_GAS_LIMIT(990000);
static const uint256 DEF_TX_GAS_PRICE(10000);

static const unsigned int MAX_BLOCK_SIZE = 2000000;
static const unsigned int MAX_TX_SIZE = (MAX_BLOCK_SIZE / 20);
static const unsigned int MAX_SIGNATURE_SIZE = 2048;
static const unsigned int MAX_TX_INPUT_COUNT = (MAX_TX_SIZE - MAX_SIGNATURE_SIZE - 4) / 33;
static const int64 MAX_BLOCK_GAS_LIMIT = 10000 * 10000 * 10;

#define BLOCK_TARGET_SPACING GET_FAST_PARAM(20, 10, 1)
#define PROOF_OF_WORK_BLOCK_SPACING GET_FAST_PARAM(20, 10, 1)
#define EXTENDED_BLOCK_SPACING GET_PARAM(1, 1)

static const unsigned int DAY_HEIGHT_MAINNET = 3600 * 24 / BLOCK_TARGET_SPACING;
static const unsigned int DAY_HEIGHT_TESTNET = 10;

#define WAIT_AGREEMENT_PUBLISH_TIMEOUT GET_FAST_PARAM(10, 5, 1)
#define MAX_SUBMIT_POW_TIMEOUT GET_FAST_PARAM(30, 20, 2)

///////////////////////////////////
// DELEGATE
#define CONSENSUS_DISTRIBUTE_INTERVAL GET_PARAM(5, 3) //15
#define CONSENSUS_ENROLL_INTERVAL GET_PARAM(10, 6)    //30
#define CONSENSUS_INTERVAL (CONSENSUS_DISTRIBUTE_INTERVAL + CONSENSUS_ENROLL_INTERVAL + 1)

#define MAX_DELEGATE_THRESH (21)

///////////////////////////////////
// FORK
const uint256 MORTGAGE_BASE = 10000 * COIN; // initial mortgage. Change from 100000 * COIN to 10000 * COIN on about 565620 height
const uint32 MORTGAGE_DECAY_CYCLE_MAINNET = DAY_HEIGHT_MAINNET * 90;
const uint32 MORTGAGE_DECAY_CYCLE_TESTNET = DAY_HEIGHT_TESTNET * 10;
#define MORTGAGE_DECAY_CYCLE GET_PARAM(MORTGAGE_DECAY_CYCLE_MAINNET, MORTGAGE_DECAY_CYCLE_TESTNET)
const double MORTGAGE_DECAY_QUANTITY = 0.5; // decay quantity

#define MIN_CREATE_FORK_INTERVAL_HEIGHT GET_PARAM(30, 0)
#define MAX_JOINT_FORK_INTERVAL_HEIGHT GET_PARAM(DAY_HEIGHT_MAINNET, 0x7FFFFFFF)

///////////////////////////////////
// CORE
static const int64 MAX_CLOCK_DRIFT = 80;

static const uint256 BBCP_TOKEN_INIT = 20000 * 10000 * COIN;
static const uint256 BBCP_REWARD_INIT = 130 * COIN;
static const uint32 BBCP_REWARD_HALVE_CYCLE_MAINNET = DAY_HEIGHT_MAINNET * 365 * 2;
static const uint32 BBCP_REWARD_HALVE_CYCLE_TESTNET = DAY_HEIGHT_TESTNET * 365 * 2;
#define BBCP_REWARD_HALVE_CYCLE GET_PARAM(BBCP_REWARD_HALVE_CYCLE_MAINNET, BBCP_REWARD_HALVE_CYCLE_TESTNET)

static const uint256 DELEGATE_PROOF_OF_STAKE_ENROLL_TRUST = 1000 * 10000 * COIN;
static const uint256 DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT = 100 * 10000 * COIN;
static const uint256 DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT_MAINNET = 3000 * 10000 * COIN;
#define DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT GET_PARAM(DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT_MAINNET, BBCP_TOKEN_INIT)
static const uint256 DELEGATE_PROOF_OF_STAKE_UNIT_AMOUNT = 1000 * COIN;
static const int64 DELEGATE_PROOF_OF_STAKE_MAXIMUM_TIMES = 100000 * 1000000L;

#define VOTE_REWARD_DISTRIBUTE_HEIGHT GET_PARAM(DAY_HEIGHT_MAINNET, DAY_HEIGHT_TESTNET)

static const uint32 VOTE_REDEEM_HEIGHT_MAINNET = DAY_HEIGHT_MAINNET * 3;
static const uint32 VOTE_REDEEM_HEIGHT_TESTNET = DAY_HEIGHT_TESTNET * 3;
#define VOTE_REDEEM_HEIGHT GET_PARAM(VOTE_REDEEM_HEIGHT_MAINNET, VOTE_REDEEM_HEIGHT_TESTNET)

#define VOTE_REWARD_LOCK_DAY_HEIGHT GET_PARAM(DAY_HEIGHT_MAINNET, DAY_HEIGHT_TESTNET)
#define VOTE_REWARD_LOCK_DAYS GET_PARAM(180, 3)

static const int64 MINT_REWARD_PER = 10000;
static const uint256 CODE_REWARD_USED(50);
static const uint256 CODE_REWARD_PER(100);

///////////////////////////////////
enum ConsensusMethod
{
    CM_MPVSS = 0,
    CM_CRYPTONIGHT = 1,
    CM_MAX
};

///////////////////////////////////
// FUNCTOIN
static const std::map<uint256, std::map<int, uint256>> mapCheckPointsList_Mainnet = {
    { uint256("00000000312731792b03593ba4a4fa476f8db31b8a02ee45a2c5a7acf9112bbf"), //Genesis
      { { 0, uint256("00000000312731792b03593ba4a4fa476f8db31b8a02ee45a2c5a7acf9112bbf") } } }
};
static const std::map<uint256, std::map<int, uint256>> mapCheckPointsList_Testnet = {
    { uint256("00000000a8b00ba26899af9bab3c44898976102504972b71bd7298d99420719b"), //Genesis
      { { 0, uint256("00000000a8b00ba26899af9bab3c44898976102504972b71bd7298d99420719b") } } }
};
#define mapCheckPointsList GET_PARAM(mapCheckPointsList_Mainnet, mapCheckPointsList_Testnet)

static const std::map<std::string, int> mapCodeGrantAddress = {
    { { "1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb", 0 } }
};

///////////////////////////////////
std::string CoinToTokenBigFloat(const uint256& nCoin);
bool TokenBigFloatToCoin(const std::string& strToken, uint256& nCoin);

#endif //METABASENET_PARAM_H
