// SPDX-License-Identifier: MIT
pragma solidity ^0.8.18;

contract Contract {
    uint public value;
    address public addr;

    constructor() {
        value = 100;
    }

    function set(uint a) external {
        value = a;
        addr = msg.sender;
    }
}

contract Create1Test {
    address public create1_addr;   

    constructor() {     
    }

    function create1() external {
        Contract obj = new Contract();
        create1_addr = address(obj);
    }
}
