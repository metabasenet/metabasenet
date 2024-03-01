// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.8.2 <0.9.0;

contract pledgevotedemo {
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

    function pledgeVote(address delegateAddress, uint32 nPledgeType, uint32 nCycles, uint32 nNonce, uint vote_amount) external payable {
        assert(msg.sender == creater);
        (bool success,) = address(0x00000000000000000000000000000000000000A1).call(abi.encodeWithSelector(bytes4(keccak256("pledgeVote(address,uint32,uint32,uint32,uint256)")),delegateAddress,nPledgeType,nCycles,nNonce,vote_amount));
        assert(success == true);
    }

    function pledgeReqRedeem(address delegateAddress, uint32 nPledgeType, uint32 nCycles, uint32 nNonce) external {
        assert(msg.sender == creater);
        (bool success,) = address(0x00000000000000000000000000000000000000A1).call(abi.encodeWithSelector(bytes4(keccak256("pledgeReqRedeem(address,uint32,uint32,uint32)")),delegateAddress,nPledgeType,nCycles,nNonce));
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
