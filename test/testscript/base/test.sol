pragma solidity ^0.5.0;

contract test {

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

    
    function test2(address contract_addr, uint nonce) public pure returns (address) {
        uint token0 = 1;
        //uint token1 = 2;
        bytes32 salt = sha256(abi.encodePacked(token0, nonce));
        //return create3(contract_addr,salt);
        uint amount = 2000000000000000;
        address addr = create3(contract_addr,salt,abi.encodePacked(amount));
        
        bool b;
        bytes bs;
        (b,bs) = addr.delegatecall(abi.encodeWithSignature("transfer(address,uint256)",contract_addr,10));
        require(b, 'test2: transfer fail');
        return addr;
    }

    function test3(address contract_addr,address addr) public returns (bool) {
        bool b;
        bytes bs;
        // 1 调用delegatecall,调用者环境发生改变，调用者调用余额等相关数据修改为当前合约
        (b,bs) = contract_addr.delegatecall(abi.encodeWithSignature("balanceOf(address)",addr));
        return b;
    }

    function test4(address contract_addr,address addr) public payable returns (address) {
        return addr;
    }

    function test5(address addr, uint256 amount) public returns (bool) {
        // 调用call,失败是发生异常
        addr.transfer(amount);
        return true;
    }

    function test6(address addr, uint256 amount) public returns (bool) {
        bool b;
        // 调用call,失败时候返回false
        b = addr.send(amount);
        return b;
    }

    function test7(address contract_addr,address addr) public returns (bool) {
        bool b;
        bytes bs;
        // 2 调用call，调用者环境发生改变，调用者调用余额等相关数据不放生改变，始终为当前调用者
        (b,bs) = contract_addr.call(abi.encodeWithSignature("balanceOf(address)",addr));
        return b;
    }

    function returnsender() public returns (address) {
        return msg.sender;
    }

    function delegatecalltest(address contract_addr) public view returns (address) {
        bool b;
        bytes bs;
        // 1 调用delegatecall,调用者环境发生改变，调用者调用余额等相关数据修改为当前合约
        (b,bs) = contract_addr.delegatecall(abi.encodeWithSignature("returnsender()"));
        require(b, 'delegatecall: FAILED');
        address ret = abi.decode(bs, (address));
        return ret;
    }

    function calltest(address contract_addr) public view returns (uint) {
        bool b;
        bytes bs;
        // 2 调用call，调用者环境发生改变，调用者调用余额等相关数据不放生改变，始终为当前调用者
        (b,bs) = contract_addr.call(abi.encodeWithSignature("returnsender()"));
        require(b, 'call: FAILED');
        address ret = abi.decode(bs, (address));
        return ret;
    }
}
