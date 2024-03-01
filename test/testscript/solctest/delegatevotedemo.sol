// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.8.2 <0.9.0;

contract delegatevotedemo {
    struct inValue{
        uint blockNumber;
        address sender;
        uint value;
    }

    address creater;
    uint32 public number;
    uint public valueTotal;
    mapping(uint32 => inValue) public mapInValue;

    constructor() payable {
        creater = msg.sender;
    }
    
    receive() external payable {
        mapInValue[number] = inValue(block.number, msg.sender, msg.value);
        number += 1;
        valueTotal += msg.value;
    }

    function getCreater() external view returns (address) {
        return creater;
    }

    function delegateVote(address mintAddress, uint16 rewardRatio, uint vote_amount) external payable {
        assert(msg.sender == creater);
        (bool success,) = address(0x00000000000000000000000000000000000000A1).call(abi.encodeWithSelector(bytes4(keccak256("delegateVote(address,uint16,uint256)")), mintAddress, rewardRatio, vote_amount));
        assert(success == true);
    }

    function delegateRedeem(address mintAddress, uint16 rewardRatio, uint redeem_amount) external {
        assert(msg.sender == creater);
        (bool success,) = address(0x00000000000000000000000000000000000000A1).call(abi.encodeWithSelector(bytes4(keccak256("delegateRedeem(address,uint16,uint256)")), mintAddress, rewardRatio, redeem_amount));
        assert(success == true);
    }

    function transferRedeem(address recvAddress, uint amount) external payable {
        assert(msg.sender == creater);
        payable(recvAddress).transfer(amount);
    }
    
    function getSelfbalance() external view returns (uint){
        uint balanceSelf;
        assembly {
            balanceSelf := selfbalance()
        }
        return balanceSelf;
    }
}
