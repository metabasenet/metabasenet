// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.8.2 <0.9.0;

interface voteInterface {
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

interface functionInterface {
    function setFunctionAddress(uint32 nFuncId, address newAddress, bool fDisableModify) external returns (bool);
    function getFunctionAddress(uint32 nFuncId) external view returns (address);
}

interface coinInterface {
    function addUserCoin(string memory strCoinSymbol, uint32 nChainId) external returns (bool);
    function addContractCoin(string memory strCoinSymbol, uint32 nChainId, address contractAddress) external returns (bool);
    function getCoin(string memory strCoinSymbol) external view returns (bool fResult, uint32 nChainId, uint32 nCoinType, address contractAddress);
}

interface coinCallInterface {
    function balanceOf(address tokenOwner) external view returns (uint);
    function transfer(address to, uint tokens) external returns (bool);
}

interface dexInterface {
    function addDexOrder(string memory strCoinSymbolOwner, string memory strCoinSymbolPeer, uint64 nNumber, uint nAmount, uint nPrice) external returns (bool);
    function getDexOrder(string memory strCoinSymbolOwner, string memory strCoinSymbolPeer, uint64 nNumber) external view returns (bool fResult, uint nAmount, uint nPrice);
    function getMaxDexOrderNumber(string memory strCoinSymbolOwner, string memory strCoinSymbolPeer) external view returns (uint64);
}

interface crossTransferInterface {
    function transferCoin(uint32 nPeerChainId, uint nTransferAmount) external returns (bool);

    function getCrosschainTransferAmount() external view returns (uint);
}
