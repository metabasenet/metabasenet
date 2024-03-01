// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.8.2 <0.9.0;

interface VoteInterface {
    function delegateVote(address mintAddress, uint16 rewardRatio, uint tokens) external returns (bool);
    function delegateRedeem(address mintAddress, uint16 rewardRatio, uint tokens) external returns (bool);
    function getDelegateVotes(address mintAddress, address ownerAddress, uint16 rewardRatio) external view returns (uint);

    function userVote(address delegateAddress, uint8 rewardMode, uint tokens) external returns (bool);
    function userRedeem(address delegateAddress, uint8 rewardMode, uint tokens) external returns (bool);
    function getUserVotes(address delegateAddress, address ownerAddress, uint8 rewardMode) external view returns (uint);
}

library baselib {
    function bytesToUint256(bytes memory b) public pure returns (uint256) {
        require(b.length >= 32, "bytes array must be at least 32 bytes long");

        uint256 result;
        assembly {
            result := mload(add(b, 32))
        }
        return result;
    }
}

contract test6{
    uint public voteamount;

    constructor() payable {
    }

    function getdelegateaddress(address delegate, address owner, uint16 rewardratio) external pure returns (address addr,bytes memory e)
    {
        uint8 ttype = 3;
        addr = address(uint160(uint(keccak256(abi.encodePacked(
            ttype,
            delegate,
            owner,
            rewardratio
            )))));
        e = abi.encodePacked(
            ttype,
            delegate,
            owner,
            rewardratio
            );
    }
    function getvoteaddress(address delegate, address owner, uint8 rewardmode) external pure returns (address addr,bytes memory e)
    {
        uint8 ttype = 4;
        addr = address(uint160(uint(keccak256(abi.encodePacked(
            ttype,
            delegate,
            owner,
            rewardmode
            )))));
        e = abi.encodePacked(
            ttype,
            delegate,
            owner,
            rewardmode
            );
    }
    function getvotebalance(address delegate, address owner, uint8 rewardmode) payable external returns (address addr,uint amount)
    {
        uint8 ttype = 4;
        addr = address(uint160(uint(keccak256(abi.encodePacked(
            ttype,
            delegate,
            owner,
            rewardmode
            )))));
        voteamount = addr.balance;
        amount = voteamount;
    }
    function sendvote(address delegate, address owner, uint8 rewardmode) payable external {
        uint8 ttype = 4;
        address vote_addr = address(uint160(uint(keccak256(abi.encodePacked(
            ttype,
            delegate,
            owner,
            rewardmode
            )))));
        payable(vote_addr).transfer(2 ether);
    }
    function getuservotes1(address delegate, address owner, uint8 rewardmode) public view returns (uint) {
        uint n = VoteInterface(address(0xA1)).getUserVotes(delegate, owner, rewardmode);
        return n;
    }
    function getuservotes2() public view returns (uint) {
        uint n = VoteInterface(address(0xA1)).getUserVotes(0xe8b6Fb9ab773cA3A12Cce9F871E4f7b7bFc0084d, 0x5962974eeb0b17b43EDaBFc9b747839317AA852F, 1);
        return n;
    }
    function userVote() public returns (bool) {
       return VoteInterface(address(0xA1)).userVote(0xe8b6Fb9ab773cA3A12Cce9F871E4f7b7bFc0084d, 1, 33333);
   }
}
