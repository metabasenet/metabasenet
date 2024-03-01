// SPDX-License-Identifier: MIT
pragma solidity ^0.8.18;
 
    
contract Test {             
    constructor() {     
    }

    uint public basefee;
    address public coinbase;
    uint public prevrandao;
    uint public gaslimit;
    uint public number;
    uint public timestamp;
    bytes public data;
    uint public gas_left;
    address public sender;
    bytes4 public sig;
    uint public value;
    uint public gasprice;
    address public origin;
    bytes32 public blockhash_;

    function w() payable external {
        
        basefee = block.basefee;
        coinbase = block.coinbase;
        prevrandao = block.prevrandao;
        gaslimit = block.gaslimit;
        number = block.number;
        timestamp = block.timestamp;
        

        data = msg.data;
        gas_left = gasleft();
        sender = msg.sender;
        sig = msg.sig;
        value = msg.value;
        
        gasprice = tx.gasprice;
        origin = tx.origin;
    }

    function blockhashtest() external {
        blockhash_ = blockhash(1);
    }
  }