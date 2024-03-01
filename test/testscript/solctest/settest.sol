pragma solidity ^0.5.0;

contract settest {

    mapping (address => uint256) private _balances;

    constructor() public {
        _balances[msg.sender] = 30000;
    }

    function setamount(address account,uint256 value) public returns (bool) {
        _balances[account] = value;
        return true;
    }
    function getamount(address account) public view returns (uint256) {
        return _balances[account];
    }
}
