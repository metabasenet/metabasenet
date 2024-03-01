// SPDX-License-Identifier: MIT
pragma solidity ^0.8.18;
contract bt {
    uint public p;
    uint public b;

    constructor() {
        p = block.prevrandao;
        b = block.basefee;
    }
    
    function test() public view returns (uint256) {
        return block.prevrandao;
    }
}

// ./solc --evm-version cancun  ./block.sol --bin
