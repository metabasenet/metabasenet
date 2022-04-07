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
        // 1 调用delegatecall,调用者环境发生改变，调用者调用余额等相关数据修改为当前合约
        (b,bs) = contract_addr.delegatecall(abi.encodeWithSignature("balanceOf(address)",addr));
        // 得到余额
        uint v = abi.decode(bs, (uint));
        // 2 调用call，调用者环境发生改变，调用者调用余额等相关数据不放生改变，始终为当前调用者
        (b,bs) = contract_addr.call(abi.encodeWithSignature("balanceOf(address)",addr));
        // 3 调用call,失败是发生异常
        addr.transfer(100);
        // 3 调用call,失败时候返回false
        b = addr.send(100);
        // 4 调用call,但数据长度等于零，金额等于100,如果转账失败返回false
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
