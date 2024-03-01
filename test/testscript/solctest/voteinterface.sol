// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.8.2 <0.9.0;

interface VoteInterface {
    function delegateVote(address mintAddress, uint16 rewardRatio, uint tokens) external returns (bool);
    function delegateRedeem(address mintAddress, uint16 rewardRatio, uint tokens) external returns (bool);
    function getDelegateVotes(address mintAddress, address ownerAddress, uint16 rewardRatio) external view returns (uint);

    function userVote(address delegateAddress, uint8 rewardMode, uint tokens) external returns (bool);
    function userRedeem(address delegateAddress, uint8 rewardMode, uint tokens) external returns (bool);
    function getUserVotes(address delegateAddress, address ownerAddress, uint8 rewardMode) external view returns (uint);

    function pledgeVote(address delegateAddress, uint32 nPledgeType, uint32 nCycles, uint32 nNonce, uint tokens) external returns (bool);
    function pledgeReqRedeem(address delegateAddress, uint32 nPledgeType, uint32 nCycles, uint32 nNonce) external returns (bool);
    function getPledgeVotes(address delegateAddress, address ownerAddress, uint32 nPledgeType, uint32 nCycles, uint32 nNonce) external view returns (uint);
    function getPledgeUnlockHeight(address delegateAddress, address ownerAddress, uint32 nPledgeType, uint32 nCycles, uint32 nNonce) external view returns (uint32);

    function getPageSize() external view returns (uint32);
    function getDelegateCount() external view returns (uint32);
    function getDelegateAddress(uint32 page) external view returns (address[] memory);
    function getDelegateTotalVotes(address delegateAddress) external view returns (uint);
    function getVoteUnlockHeight(address addressVote) external view returns (uint32);
}

interface FunctionInterface {
    function setFunctionAddress(uint32 nFuncId, address newAddress, bool fDisableModify) external returns (bool);
    function getFunctionAddress(uint32 nFuncId) external view returns (address);
}

contract Functions is FunctionInterface{
    mapping(uint32 => address) funcAddressList;

    function setFunctionAddress(uint32 nFuncId, address newAddress, bool fDisableModify) external returns (bool)
    {
        (fDisableModify);
        funcAddressList[nFuncId] = newAddress;
        return true;
    }
    function getFunctionAddress(uint32 nFuncId) external view returns (address)
    {
        return funcAddressList[nFuncId];
    }
}

contract Votes is VoteInterface{
    struct delegateData{
        uint32 index; // >= 1
        uint votes;
    }

    mapping(address => delegateData) delegates;
    mapping(address => uint) voters;
    mapping(uint32 => address) indexs; // key starts from 1
    uint32 delegateCount;
    uint32 pageSize;

    constructor() {
        delegateCount = 0;
        pageSize = 32;

        inDelegateVote(0xa0b6C7A9b03dC458CF2d1f604D460DA2a3002fe5, 0xef0a0BD37c8D1984A65D9Bede4c6f74f4094fC04, 500, 123456);
    }

    // internal
    function inGetDelegateAddress(address mintAddress, address owner, uint16 rewardRatio) internal pure returns (address delegateAddress)
    {
        uint8 ttype = 3;
        delegateAddress = address(uint160(uint(keccak256(abi.encodePacked(
            ttype,
            mintAddress,
            owner,
            rewardRatio
            )))));
    }
    function inGetVoteAddress(address delegateAddress, address owner, uint8 rewardMode) internal pure returns (address voteAddress)
    {
        uint8 ttype = 4;
        voteAddress = address(uint160(uint(keccak256(abi.encodePacked(
            ttype,
            delegateAddress,
            owner,
            rewardMode
            )))));
    }
    function inGetPledgeAddress(address delegateAddress, address owner, uint32 nPledgeType, uint32 nCycles, uint32 nNonce) internal pure returns (address pledgeAddress)
    {
        uint8 ttype = 5;
        pledgeAddress = address(uint160(uint(keccak256(abi.encodePacked(
            ttype,
            delegateAddress,
            owner,
            nPledgeType,
            nCycles,
            nNonce
            )))));
    }
    function inAddDelegateAddress(address delegateAddress) internal
    {
        if (delegates[delegateAddress].index == 0)
        {
            delegateCount++;
            delegates[delegateAddress].index = delegateCount;
            indexs[delegateCount] = delegateAddress;
        }
    }
    function inDelegateVote(address mintAddress, address owner, uint16 rewardRatio, uint tokens) internal
    {
        address delegateAddress = inGetDelegateAddress(mintAddress, owner, rewardRatio);

        delegates[delegateAddress].votes += tokens;
        voters[delegateAddress] += tokens;

        inAddDelegateAddress(delegateAddress);
    }
    function inUserVote(address delegateAddress, address owner, uint8 rewardMode, uint tokens) internal
    {
        address voteAddress = inGetVoteAddress(delegateAddress, owner, rewardMode);

        delegates[delegateAddress].votes += tokens;
        voters[voteAddress] += tokens;

        inAddDelegateAddress(delegateAddress);
    }
    function inUserPledge(address delegateAddress, address owner, uint32 nPledgeType, uint32 nCycles, uint32 nNonce, uint tokens) internal
    {
        address pledgeAddress = inGetPledgeAddress(delegateAddress, owner, nPledgeType, nCycles, nNonce);

        delegates[delegateAddress].votes += tokens;
        voters[pledgeAddress] += tokens;

        inAddDelegateAddress(delegateAddress);
    }

    // external
    function delegateVote(address mintAddress, uint16 rewardRatio, uint tokens) external returns (bool)
    {
        inDelegateVote(mintAddress, msg.sender, rewardRatio, tokens);
        return true;
    }
    function delegateRedeem(address mintAddress, uint16 rewardRatio, uint tokens) external returns (bool)
    {
        address delegateAddress = inGetDelegateAddress(mintAddress, msg.sender, rewardRatio);
        require(delegates[delegateAddress].votes >= tokens);
        require(voters[delegateAddress] >= tokens);
        delegates[delegateAddress].votes -= tokens;
        voters[delegateAddress] -= tokens;
        return true;
    }
    function getDelegateVotes(address mintAddress, address ownerAddress, uint16 rewardRatio) external view returns (uint)
    {
        return voters[inGetDelegateAddress(mintAddress, ownerAddress, rewardRatio)];
    }
    
    function userVote(address delegateAddress, uint8 rewardMode, uint tokens) external returns (bool)
    {
        inUserVote(delegateAddress, msg.sender, rewardMode, tokens);
        return true;
    }
    function userRedeem(address delegateAddress, uint8 rewardMode, uint tokens) external returns (bool)
    {
        address voteAddress = inGetVoteAddress(delegateAddress, msg.sender, rewardMode);
        require(delegates[delegateAddress].votes >= tokens);
        require(voters[voteAddress] >= tokens);
        delegates[delegateAddress].votes -= tokens;
        voters[voteAddress] -= tokens;
        return true;
    }
    function getUserVotes(address delegateAddress, address ownerAddress, uint8 rewardMode) external view returns (uint)
    {
        return voters[inGetVoteAddress(delegateAddress, ownerAddress, rewardMode)];
    }

    function pledgeVote(address delegateAddress, uint32 nPledgeType, uint32 nCycles, uint32 nNonce, uint tokens) external returns (bool)
    {
        inUserPledge(delegateAddress, msg.sender, nPledgeType, nCycles, nNonce, tokens);
        return true;
    }
    function pledgeReqRedeem(address delegateAddress, uint32 nPledgeType, uint32 nCycles, uint32 nNonce) external returns (bool)
    {
        return true;
    }
    function getPledgeVotes(address delegateAddress, address ownerAddress, uint32 nPledgeType, uint32 nCycles, uint32 nNonce) external view returns (uint)
    {
        return voters[inGetPledgeAddress(delegateAddress, ownerAddress, nPledgeType, nCycles, nNonce)];
    }
    function getPledgeUnlockHeight(address delegateAddress, address ownerAddress, uint32 nPledgeType, uint32 nCycles, uint32 nNonce) external view returns (uint32)
    {
        return 0;
    }

    function getPageSize() external view returns (uint32)
    {
        return pageSize;
    }
    function getDelegateCount() external view returns (uint32)
    {
        return delegateCount;
    }
    function getDelegateAddress(uint32 page) external view returns (address[] memory)
    {
        uint32 outSize = pageSize;
        if (pageSize * page >= delegateCount)
        {
            outSize = 1;
        }
        else if (pageSize * (page + 1) >= delegateCount)
        {
            outSize = delegateCount - (pageSize * page);
        }
        address[] memory out = new address[](outSize);
        if (pageSize * page < delegateCount)
        {
            for (uint32 i = 0; i < outSize; i++)
            {
                out[i] = indexs[pageSize * page + i + 1];
            }
        }
        return out;
    }
    function getDelegateTotalVotes(address delegateAddress) external view returns (uint)
    {
        return delegates[delegateAddress].votes;
    }
    function getVoteUnlockHeight(address addressVote) external view returns (uint32)
    {
        return 0;
    }
}
