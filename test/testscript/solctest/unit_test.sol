// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;
 
contract Contract {
    uint public value;
    address public addr;

    constructor() {
        value = 100;
    }

    function set(uint a) external {
        value = a;
        addr = msg.sender;
    }
}

contract Test {             
    uint public value;
    address public addr;
    uint public a_;
    uint public c_;

    uint public basefee;
    address public coinbase;
    uint public prevrandao;
    uint public gaslimit;
    uint public number;
    uint public timestamp;
    bytes public data;
    uint public gas_left;
    address public sender;
    bytes4 public sig;
    uint public value_;
    uint public gasprice;
    address public origin;
    bytes32 public blockhash_;
    
    address public self;
    uint public balance1;
    uint public balance2;
    bytes32 public keccak256_;
    uint public ext_code_size1;
    uint public ext_code_size2;
    bytes32 public codeHash_;
    uint public chainId_;

    address public create1_addr;
    address public create2_addr;

    constructor(uint a,uint b) payable {
        a_ = a;
        c_ = a + b;
    }

    function w(uint a,uint b) payable external {
        a_ = a;
        c_ = a + b; 

        basefee = block.basefee;
        coinbase = block.coinbase; 
        prevrandao = block.prevrandao;
        gaslimit = block.gaslimit;
        number = block.number;
        timestamp = block.timestamp;

        data = msg.data;
        gas_left = gasleft();
        sender = msg.sender;
        sig = msg.sig;
        value_ = msg.value; 
        
        gasprice = tx.gasprice; 
        origin = tx.origin;     

        blockhash_ = blockhash(1);
        //
        self = address(this);
        balance1 = msg.sender.balance;
    }

    function w2() payable external {
        address test_addr = address(0x2eA6C9264049fC0730acd5F375B62BC32768C4c1);
        payable(test_addr).transfer(1 ether);
        assert(payable(test_addr).send(100 gwei)); 
        
        keccak256_ = keccak256(bytes('1'));

        uint chainId;
        assembly {
            chainId := chainid()
        }
        chainId_ = chainId;

        ext_code_size1 = self.code.length;
        ext_code_size2 = msg.sender.code.length;
        assert(ext_code_size2 == 0);
        bytes32 codeHash;
        assembly {
            let size := codesize()
            codecopy(mload(0x40), 0, size)
            codeHash := keccak256(mload(0x40), size)
        }
        codeHash_ = codeHash;
        uint sb;
        assembly {
            sb := selfbalance()
        }
        balance2 = sb;
    }

    //function exit(address addr_) external {
    //    selfdestruct(payable(addr_));
    //}

    function create1() external {
        Contract obj = new Contract();
        create1_addr = address(obj);
    }
        
    function create2() external {
        bytes memory bytecode = type(Contract).creationCode;
        bytes32 salt = keccak256(bytes('1'));
        address pair;
        assembly {
            // value	offset	length	salt 	
            pair := create2(0, add(bytecode, 32), mload(bytecode), salt)
        }
        create2_addr = pair;
        bytes32 bs32 = keccak256(abi.encodePacked(bytes1(0xff),address(this),salt,keccak256(bytecode)));
        address create2_addr_ = address(bytes20(bs32 << 96));
        assert(create2_addr_ == create2_addr);
    }
    
    function call1() external {
        Contract(create2_addr).set(123);
        (bool success,) = create2_addr.call{value:0}(abi.encodeWithSelector(bytes4(keccak256("set(uint256)")),123));
        assert(success == true);
        assert(Contract(create2_addr).value() == 123);
        assert(Contract(create2_addr).addr() == address(this));
    }

    /* 
    function call2() external {
        //value = 123;
        //create2_addr.callcode(abi.encodeWithSelector(bytes4(keccak256("set(uint256)")),456));
        //assert(value == 456);
        //assert(addr == address(this));
    }*/
        
    function call3() external  {
        value = 123;
        (bool success, bytes memory data_) = create2_addr.delegatecall(abi.encodeWithSelector(bytes4(keccak256("set(uint256)")),789));
        assert(data_.length == 0 && success);
        assert(value == 789);
        assert(addr == msg.sender);
    }

    function call4() pure external returns(address,bytes20,bytes32,bytes32) {
        uint8 v = 0x1c;
        bytes32 r = 0xf604a8cae61ffcff451682804bd32b2e8b70bcc4822f9827cbbbda668e31d09b;
        bytes32 s = 0x04a4e349e6ec60a144ce600518698bc97fae2bbe595b01da740ff7ab1597b412;
        address addr_ = ecrecover(keccak256(abi.encodePacked(0x75F59364e68049Ae146cc22e5fFDAfe2DAc80a3c)), v, r, s);
        assert(addr_ == 0x74F8379b34a9Ac50E33A0095BCe40cd7DCC2dA5a);
        assert(ripemd160(bytes('1')) == hex'c47907abd2a80492ca9388b05c0e382518ff3960');
        assert(sha256(bytes('1')) == 0x6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b);
        assert(keccak256(bytes('1')) == 0xc89efdaa54c0f20c7adf612882df0950f5a951637e0307cdcb4c672f298b8bc6);
        return (addr_,ripemd160(bytes('1')),sha256(bytes('1')),keccak256(bytes('1')));
    }
}