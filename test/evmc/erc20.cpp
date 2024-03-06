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

#include "erc20.h"

#include <cstring>
#include <fstream>
#include <iostream>

#include "common/hexstr.h"
#include "common/log.h"
#include "crypto.h"
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
metabasenet::crypto::CPubKey pub;

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
    const std::string wasmf = "/home/shang/mam/metabasenet/doc/erc20p.wasm";
    ifstream inFile(wasmf, ios::in | ios::binary);
    erc20_deploy_wasm_len = getFileSize(wasmf.c_str());
    erc20_deploy_wasm_ = new char[erc20_deploy_wasm_len];
    inFile.read(erc20_deploy_wasm_, erc20_deploy_wasm_len);
    inFile.close();
}

void saveerc20()
{
    using namespace std;
    const std::string wasmf = "/home/shang/mam/metabasenet/build/test/erc20p_.wasm";
    ofstream outFile(wasmf, ios::out | ios::binary);
    outFile.write(erc20_wasm_, erc20_wasm_len);
    outFile.close();
}

static std::string getSignatureHash(std::string fun)
{
    Keccak h(256);
    h.addData((uint8_t*)fun.c_str(), 0, fun.size());
    std::vector<uint8_t> ret = h.digest();
    return mtbase::ToHexString(std::vector<uint8_t>(ret.begin() + 28, ret.end()));
}

evmc_result evmc_vm_execute(evmc_vm* vm, evmc::address sender,
                            std::string CallDataStr)
{
    std::vector<uint8_t> CallData;
    SSVM::convertHexStrToBytes(CallDataStr, CallData);
    evmc::address destination = {};
    int64_t gas = 999999;
    evmc::uint256be value; // = {};

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

static evmc::address sender(1234567890);
static evmc::address sender_b(222222222);
static int transfer_amount = 12;
BOOST_FIXTURE_TEST_SUITE(WASM, VM)

BOOST_AUTO_TEST_CASE(Run__1_deploy)
{
    loaderc20();
    struct evmc_vm* vm = evmc_load_and_create();
    evmc::address destination = {};
    int64_t gas = 999999;
    evmc::uint256be value(1); // = {};
    evmc_message msg{ EVMC_CALL,
                      0,
                      0,
                      gas,
                      destination,
                      sender,
                      nullptr,
                      0,
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

BOOST_AUTO_TEST_CASE(Run__2_check_balance_of_sender)
{
    struct evmc_vm* vm = evmc_load_and_create();
    //70a08231: balanceOf(address)
    std::string fun_sign = getSignatureHash("balanceOf(address)");
    std::string CallDataStr = fun_sign + mtbase::ToHexString(sender.bytes, 32);
    evmc_result result = evmc_vm_execute(vm, sender, CallDataStr);
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == 32);
    std::vector<uint8_t> vu(result.output_data, result.output_data + 32);
    std::reverse(vu.begin(), vu.end());
    uint256 ret(vu);
    BOOST_CHECK(ret == 1000);
    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__3_check_total_supply)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string CallDataStr = getSignatureHash("totalSupply()");
    evmc_result result = evmc_vm_execute(vm, sender, CallDataStr);
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == 32);
    std::vector<uint8_t> vu(result.output_data, result.output_data + 32);
    std::reverse(vu.begin(), vu.end());
    uint256 ret(vu);
    BOOST_CHECK(ret == 1000);
    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__4_transfer_20)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string CallDataStr = getSignatureHash("transfer(address,uint256)")
                              + mtbase::ToHexString(sender_b.bytes, 32) + uint256(transfer_amount).GetHex();
    evmc_result result = evmc_vm_execute(vm, sender, CallDataStr);

    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == 32);
    std::vector<uint8_t> vu(result.output_data, result.output_data + 32);
    std::reverse(vu.begin(), vu.end());
    uint256 ret(vu);
    BOOST_CHECK(ret == 1);
    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__5_check_balance_of_sender_b)
{

    struct evmc_vm* vm = evmc_load_and_create();
    std::string fun_sign = getSignatureHash("balanceOf(address)");
    std::string CallDataStr = fun_sign + mtbase::ToHexString(sender_b.bytes, 32);
    evmc_result result = evmc_vm_execute(vm, sender, CallDataStr);
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == 32);
    std::vector<uint8_t> vu(result.output_data, result.output_data + 32);
    std::reverse(vu.begin(), vu.end());
    uint256 ret(vu);
    BOOST_CHECK(ret == transfer_amount);
    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__6_approve_10_from_sender_for_sender_b)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string fun_sign = getSignatureHash("approve(address,uint256)");

    std::string CallDataStr = fun_sign + mtbase::ToHexString(sender_b.bytes, 32) + uint256(transfer_amount).GetHex();
    evmc_result result = evmc_vm_execute(vm, sender, CallDataStr);

    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == 32);
    std::vector<uint8_t> vu(result.output_data, result.output_data + 32);
    std::reverse(vu.begin(), vu.end());
    uint256 ret(vu);
    BOOST_CHECK(ret == 1);
    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__7_check_allowance_from_0x7FFFFFFF_by_0x01)
{

    struct evmc_vm* vm = evmc_load_and_create();
    std::string fun_sign = getSignatureHash("allowance(address,address)");
    std::string CallDataStr = fun_sign + mtbase::ToHexString(sender.bytes, 32) + mtbase::ToHexString(sender_b.bytes, 32);
    evmc_result result = evmc_vm_execute(vm, sender, CallDataStr);

    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == 32);
    std::vector<uint8_t> vu(result.output_data, result.output_data + 32);
    std::reverse(vu.begin(), vu.end());
    uint256 ret(vu);
    BOOST_CHECK(ret == transfer_amount);
    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__8_transfer_3_from)
{
    struct evmc_vm* vm = evmc_load_and_create();
    //23b872dd: transferFrom(address,address,uint256)
    std::string fun_sign = getSignatureHash("transferFrom(address,address,uint256)");
    std::string CallDataStr = fun_sign + mtbase::ToHexString(sender.bytes, 32) + mtbase::ToHexString(sender_b.bytes, 32) + uint256(transfer_amount).GetHex();

    evmc_result result = evmc_vm_execute(vm, sender_b, CallDataStr);
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == 32);
    std::vector<uint8_t> vu(result.output_data, result.output_data + 32);
    std::reverse(vu.begin(), vu.end());
    uint256 ret(vu);
    BOOST_CHECK(ret == 1);
    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__9_check_balance_of_0x7FFFFFFF)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string fun_sign = getSignatureHash("balanceOf(address)");
    std::string CallDataStr = fun_sign + mtbase::ToHexString(sender.bytes, 32);
    evmc_result result = evmc_vm_execute(vm, sender, CallDataStr);
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == 32);
    std::vector<uint8_t> vu(result.output_data, result.output_data + 32);
    std::reverse(vu.begin(), vu.end());
    uint256 ret(vu);
    BOOST_CHECK(ret == 1000 - transfer_amount * 2);
    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_SUITE_END()