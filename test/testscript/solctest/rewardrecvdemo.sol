// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.8.2 <0.9.0;

contract rewardRecvDemo {

    struct inValue{
        uint blockNumber;
        address sender;
        uint value;
    }

    address creater;
    uint32 public number;
    uint public total;
    mapping(uint32 => inValue) public mapInValue;

    constructor() payable {
        creater = msg.sender;
        total += msg.value;
    }
    
    receive() external payable {
        mapInValue[number] = inValue(block.number, msg.sender, msg.value);
        number += 1;
        total += msg.value;
    }

    function transOut(address recv_addr, uint amount) external payable {
        assert(msg.sender == creater);
        uint balanceSelf;
        assembly {
            balanceSelf := selfbalance()
        }
        assert(amount <= balanceSelf);
        payable(recv_addr).transfer(amount);
        total -= amount;
    }

    function getSelfbalance() external view returns (uint){
        uint balanceSelf;
        assembly {
            balanceSelf := selfbalance()
        }
        return balanceSelf;
    }
}