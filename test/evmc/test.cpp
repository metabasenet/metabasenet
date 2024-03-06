// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/loader/ethereumTest.cpp - Ethereum related wasm tests ---===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading WASM about Ethereum.
///
//===----------------------------------------------------------------------===//

#include <cstring>
#include <fstream>
#include <iostream>

#include "common/hexstr.h"
#include "crypto.h"
#include "erc20.h"
#include "evmc/evmc.hpp"
#include "evmc/loader.h"
#include "example_host.h"
#include "keccak/Keccak.h"
#include "key.h"
#include "util.h"

#ifndef BOOST_TEST_MODULE
#define BOOST_TEST_MODULE Unit Tests for Core Wallet
#include <boost/test/unit_test.hpp>
#endif

// cd build/test
// soll ../../test/evmc/erc20.sol
// mv ../../test/evmc/erc20.wasm ./
// ./test_big --run_test=WASM --log_level=test_suite

evmc_host_context* context = example_host_create_context(evmc_tx_context{});

const evmc_host_interface* host_interface = example_host_get_interface();

class VM
{
};

static std::size_t erc20_deploy_wasm_len = 0;
static char* erc20_deploy_wasm_ = nullptr;

static std::size_t erc20_wasm_len = 0;
static char* erc20_wasm_ = nullptr;

long getFileSize(const char* strFileName)
{
    FILE* fp = fopen(strFileName, "r");
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    return size;
}

void loaderc20()
{
    using namespace std;
    const std::string wasmf = "/home/shang/mam/metabasenet/test/evmc/test.wasm";
    ifstream inFile(wasmf, ios::in | ios::binary);
    erc20_deploy_wasm_len = getFileSize(wasmf.c_str());
    erc20_deploy_wasm_ = new char[erc20_deploy_wasm_len];
    inFile.read(erc20_deploy_wasm_, erc20_deploy_wasm_len);
    inFile.close();
}

void saveerc20()
{
    using namespace std;
    const std::string wasmf = "/home/shang/mam/metabasenet/test/evmc/test_.wasm";
    ofstream outFile(wasmf, ios::out | ios::binary);
    outFile.write(erc20_wasm_, erc20_wasm_len);
    outFile.close();
}

evmc::address string_to_address(std::string SenderStr)
{
    evmc::address address;
    std::vector<uint8_t> Sender;
    SSVM::convertHexStrToBytes(SenderStr, Sender);
    for (int i = 0; i < 20; i++)
    {
        address.bytes[i] = Sender[i];
    }
    return address;
}

evmc_result evmc_vm_execute(evmc_vm* vm, std::string SenderStr,
                            std::string CallDataStr)
{
    std::vector<uint8_t> CallData;
    SSVM::convertHexStrToBytes(CallDataStr, CallData);
    evmc::address sender = string_to_address(SenderStr);
    evmc::address destination = {};
    int64_t gas = 999999;
    evmc::uint256be value = {};

    //uint256 nTxAmount = 5000000000000;
    //memcpy(value.bytes, nTxAmount.begin(), std::min(sizeof(value.bytes), (size_t)(nTxAmount.size())));
    //std::reverse(std::begin(value.bytes), std::end(value.bytes));

    evmc_message msg{ EVMC_CALL,
                      0,
                      0,
                      gas,
                      destination,
                      sender,
                      CallData.data(),
                      CallData.size(),
                      value,
                      {} };
    evmc_result result = vm->execute(vm, host_interface, context, EVMC_MAX_REVISION, &msg,
                                     (uint8_t*)erc20_wasm_, erc20_wasm_len);
    return result;
}

BOOST_FIXTURE_TEST_SUITE(WASM, VM)

BOOST_AUTO_TEST_CASE(Run__1_deploy)
{
    loaderc20();
    struct evmc_vm* vm = evmc_load_and_create();
    evmc::address sender({ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x7f, 0xff, 0xff, 0xff });
    evmc::address destination = {};
    int64_t gas = 999999;
    evmc::uint256be value = {};
    evmc_message msg{ EVMC_CALL,
                      0,
                      0,
                      gas,
                      destination,
                      sender,
                      (uint8_t*)erc20_deploy_wasm_,
                      erc20_deploy_wasm_len,
                      value,
                      {} };
    evmc_result result = vm->execute(vm, host_interface, context, EVMC_MAX_REVISION, &msg,
                                     (uint8_t*)erc20_deploy_wasm_, erc20_deploy_wasm_len);

    BOOST_CHECK(result.status_code == EVMC_SUCCESS);

    erc20_wasm_len = result.output_size;
    erc20_wasm_ = new char[erc20_wasm_len];
    memcpy(erc20_wasm_, result.output_data, erc20_wasm_len);
    saveerc20();
    if (result.release)
        result.release(&result);
}

std::string getSignatureHash(std::string fun)
{
    Keccak h(256);
    h.addData((uint8_t*)fun.c_str(), 0, fun.size());
    std::vector<uint8_t> ret = h.digest();
    return mtbase::ToHexString(std::vector<uint8_t>(ret.begin() + 28, ret.end()));
}

BOOST_AUTO_TEST_CASE(Run_test5)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string SenderStr = "000000000000000000000000000000007fffffff";
    std::string fun_sign = getSignatureHash("test(uint256)");
    //1ad7be82: test5()
    metabasenet::crypto::CPubKey pub;
    std::string CallDataStr = fun_sign + "0000000000000000000000000000000000000000000000000000000000000020";
    //+ u.GetHex() + "00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000020410612e0c437831394a9ca949aae073ca671d5dd041cb7d26bd3a0827ea196e2";

    evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    std::cout << result.output_size << std::endl;
    std::cout << mtbase::ToHexString(result.output_data, result.output_size) << std::endl;
    /*
    */
}

BOOST_AUTO_TEST_SUITE_END()