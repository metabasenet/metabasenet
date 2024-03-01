// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.8.2 <0.9.0;

contract uservotedemo {
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

    function userVote(address delegateAddress, uint8 rewardMode, uint vote_amount) external payable {
        assert(msg.sender == creater);
        (bool success,) = address(0x00000000000000000000000000000000000000A1).call(abi.encodeWithSelector(bytes4(keccak256("userVote(address,uint8,uint256)")), delegateAddress, rewardMode, vote_amount));
        assert(success == true);
    }

    function userRedeem(address delegateAddress, uint8 rewardMode, uint redeem_amount) external {
        assert(msg.sender == creater);
        (bool success,) = address(0x00000000000000000000000000000000000000A1).call(abi.encodeWithSelector(bytes4(keccak256("userRedeem(address,uint8,uint256)")), delegateAddress, rewardMode, redeem_amount));
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
