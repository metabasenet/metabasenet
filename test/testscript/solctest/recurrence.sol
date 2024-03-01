// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

interface IRecurrence {
    function recurrence(uint) external returns (uint);
}

contract A {
    uint public depth = 0;
    function recurrence(uint a) external returns(uint) {
        depth++;
        if (a == 0) {
            return 0;
        }
        return IRecurrence(msg.sender).recurrence(a - 1) + 1;
    }
}

contract B {
    address public a_addr;

    constructor() {
       A a = new A();
       a_addr = address(a);
    }

    uint public depth = 0;
    function recurrence(uint a) external returns(uint) {
        depth++;
        if (a == 0) {
            return 0;
        }
        return IRecurrence(msg.sender).recurrence(a - 1) + 1;
    }

    uint public ret;
    function test_recurrence() external returns(uint) {
        ret = IRecurrence(a_addr).recurrence(32);
        assert(depth == 16);
        return ret;
    }
}