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

#include <boost/test/unit_test.hpp>
#include <cstring>
#include <fstream>
#include <iostream>

#include "common/hexstr.h"
#include "erc20.h"
#include "evmc/evmc.hpp"
#include "evmc/loader.h"
#include "example_host.h"

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
    const std::string wasmf = "erc20.wasm";
    ifstream inFile(wasmf, ios::in | ios::binary);
    erc20_deploy_wasm_len = getFileSize(wasmf.c_str());
    erc20_deploy_wasm_ = new char[erc20_deploy_wasm_len];
    inFile.read(erc20_deploy_wasm_, erc20_deploy_wasm_len);
    inFile.close();
}

void saveerc20()
{
    using namespace std;
    const std::string wasmf = "erc20_.wasm";
    ofstream outFile(wasmf, ios::out | ios::binary);
    outFile.write(erc20_wasm_, erc20_wasm_len);
    outFile.close();
}

evmc::address string_to_address(std::string SenderStr)
{
    evmc::address address;
    std::vector<uint8_t> Sender;
    SSVM::convertHexStrToBytes(SenderStr, Sender);
    for (int i = 0; i < 32; i++)
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

BOOST_AUTO_TEST_CASE(Run__2_check_balance_of_0x7FFFFFFF)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string SenderStr = "000000000000000000000000000000007fffffff";
    std::string CallDataStr = "70a08231"
                              "000000000000000000000000000000000000000000000000000000007fffffff";
    evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

    std::array<uint8_t, 32> expected_result = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe8 }
    };
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == expected_result.size());
    BOOST_CHECK(0 == memcmp(result.output_data, expected_result.data(), result.output_size));

    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__3_check_total_supply)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string SenderStr = "000000000000000000000000000000007fffffff";
    std::string CallDataStr = "18160ddd";
    evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

    std::array<uint8_t, 32> expected_result = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe8 }
    };
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == expected_result.size());
    BOOST_CHECK(0 == memcmp(result.output_data, expected_result.data(), result.output_size));

    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__4_transfer_20_from_0x7FFFFFFF_to_0x01)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string SenderStr = "000000000000000000000000000000007fffffff";
    std::string CallDataStr = "a9059cbb"
                              "0000000000000000000000000000000000000000000000000000000000000001"
                              "0000000000000000000000000000000000000000000000000000000000000014";
    evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

    std::array<uint8_t, 32> expected_result = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
    };
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == expected_result.size());
    BOOST_CHECK(0 == memcmp(result.output_data, expected_result.data(), result.output_size));

    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__5_check_balance_of_0x01)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string SenderStr = "000000000000000000000000000000007fffffff";
    std::string CallDataStr = "70a08231"
                              "0000000000000000000000000000000000000000000000000000000000000001";
    evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

    std::array<uint8_t, 32> expected_result = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14 }
    };
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == expected_result.size());
    BOOST_CHECK(0 == memcmp(result.output_data, expected_result.data(), result.output_size));

    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__6_approve_10_from_0x7FFFFFFF_for_0x01_to_spend)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string SenderStr = "000000000000000000000000000000007fffffff";
    std::string CallDataStr = "095ea7b3"
                              "0000000000000000000000000000000000000000000000000000000000000001"
                              "000000000000000000000000000000000000000000000000000000000000000a";
    evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

    std::array<uint8_t, 32> expected_result = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
    };
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == expected_result.size());
    BOOST_CHECK(0 == memcmp(result.output_data, expected_result.data(), result.output_size));

    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__7_check_allowance_from_0x7FFFFFFF_by_0x01)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string SenderStr = "000000000000000000000000000000007fffffff";
    std::string CallDataStr = "dd62ed3e"
                              "000000000000000000000000000000000000000000000000000000007fffffff"
                              "0000000000000000000000000000000000000000000000000000000000000001";
    evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

    std::array<uint8_t, 32> expected_result = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a }
    };
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == expected_result.size());
    BOOST_CHECK(0 == memcmp(result.output_data, expected_result.data(), result.output_size));

    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__8_transfer_3_from_0x7FFFFFFF_by_0x01_to_0x02)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string SenderStr = "0000000000000000000000000000000000000001";
    std::string CallDataStr = "23b872dd"
                              "000000000000000000000000000000000000000000000000000000007fffffff"
                              "0000000000000000000000000000000000000000000000000000000000000002"
                              "0000000000000000000000000000000000000000000000000000000000000003";
    evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

    std::array<uint8_t, 32> expected_result = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
    };
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == expected_result.size());
    BOOST_CHECK(0 == memcmp(result.output_data, expected_result.data(), result.output_size));

    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_CASE(Run__9_check_balance_of_0x7FFFFFFF)
{
    struct evmc_vm* vm = evmc_load_and_create();
    std::string SenderStr = "0000000000000000000000000000000000000001";
    std::string CallDataStr = "70a08231"
                              "000000000000000000000000000000000000000000000000000000007fffffff";
    evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

    std::array<uint8_t, 32> expected_result = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xd1 }
    };
    BOOST_CHECK(result.status_code == EVMC_SUCCESS);
    BOOST_CHECK(result.output_size == expected_result.size());
    BOOST_CHECK(0 == memcmp(result.output_data, expected_result.data(), result.output_size));

    if (result.release)
        result.release(&result);
}

BOOST_AUTO_TEST_SUITE_END()