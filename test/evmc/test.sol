pragma solidity ^0.5.0;

contract test {

    function test(uint256 v) public payable pure returns (uint256) {
        //return address(this).balance;
        //return msg.value;
        return v;
    }

    function test0(bytes32 h, bytes32 p, bytes sig) public pure returns (bool) {
        return ecrecover(h, p, sig);
    }

    function test1(address contract_addr,address addr) public returns (bool) {
        bool b;
        bytes bs;
        (b,bs) = contract_addr.delegatecall(abi.encodeWithSignature("balanceOf(address)",addr));
        uint v = abi.decode(bs, (uint));
        (b,bs) = contract_addr.call(abi.encodeWithSignature("balanceOf(address)",addr));
        addr.transfer(100);
        b = addr.send(100);
        //return addr.send(100);
        return b;
    }

    
    function test2(address contract_addr) public pure returns (address) {
        uint token0 = 1;
        uint token1 = 2;
        bytes32 salt = sha256(abi.encodePacked(token0, token1));
        return create3(contract_addr,salt);
    }
}
