library SafeMath {
    function add(uint x, uint y) internal pure returns (uint) {
        uint z = x + y;
        require(z >= x, 'ds-math-add-overflow');
        return z;
    }

    function sub(uint x, uint y) internal pure returns (uint) {
        uint z = x - y;
        require(z <= x, 'ds-math-sub-underflow');
        return z;
    }

    function mul(uint x, uint y) internal pure returns (uint) {
        uint z = x * y;
        require(y == 0 || z / y == x, 'ds-math-mul-overflow');
        return z;
    }
}

contract UniswapV2ERC20 {
    using SafeMath for uint;

    string public constant name = 'Uniswap V2';
    string public constant symbol = 'UNI-V2';
    uint8 public constant decimals = 18;
    uint  public totalSupply;
    mapping(address => uint) public balanceOf;
    mapping(address => mapping(address => uint)) public allowance;

    bytes32 public DOMAIN_SEPARATOR = 0x12345678;
    // keccak256("Permit(address owner,address spender,uint256 value,uint256 nonce,uint256 deadline)");
    bytes32 public constant PERMIT_TYPEHASH = 0x6e71edae12b1b97f4d1f60370fef10105fa2faae0126114a169c64845d6126c9;
    mapping(address => uint) public nonces;

    event Approval(address indexed owner, address indexed spender, uint value);
    event Transfer(address indexed from, address indexed to, uint value);

    constructor() public {
        //uint chainId;
        //assembly {
        //    chainId := chainid
        //}
        //DOMAIN_SEPARATOR = keccak256(
        //    abi.encode(
        //        keccak256('EIP712Domain(string name,string version,uint256 chainId,address verifyingContract)'),
        //        keccak256(bytes(name)),
        //        keccak256(bytes('1')),
        //        chainId,
        //        address(this)
        //    )
        //);
    }

    function _mint(address to, uint value) internal {
        totalSupply = totalSupply.add(value);
        uint v = balanceOf[to];
        balanceOf[to] = v.sub(value);
        emit Transfer(address(0), to, value);
    }

    function _burn(address from, uint value) internal {
        uint v = balanceOf[from];
        balanceOf[from] = v.sub(value);
        totalSupply = totalSupply.sub(value);
        emit Transfer(from, address(0), value);
    }

    function _approve(address owner, address spender, uint value) private {
        allowance[owner][spender] = value;
        emit Approval(owner, spender, value);
    }

    function _transfer(address from, address to, uint value) private {
        uint v = balanceOf[from];
        balanceOf[from] = v.sub(value);
        v = balanceOf[to];
        balanceOf[to] = v.add(value);
        emit Transfer(from, to, value);
    }

    function approve(address spender, uint value) external returns (bool) {
        _approve(msg.sender, spender, value);
        return true;
    }

    function transfer(address to, uint value) external returns (bool) {
        _transfer(msg.sender, to, value);
        return true;
    }

    function transferFrom(address from, address to, uint value) external returns (bool) {
        if (allowance[from][msg.sender] != uint(-1)) {
            uint v = allowance[from][msg.sender];
            allowance[from][msg.sender] = v.sub(value);
        }
        _transfer(from, to, value);
        return true;
    }

    function permit(address owner, address spender, uint value, uint deadline, uint8 v, bytes32 r, bytes32 s) external {
        /*
        require(deadline >= block.timestamp, 'UniswapV2: EXPIRED');
        bytes32 digest = keccak256(
            abi.encodePacked(
                '\x19\x01',
                DOMAIN_SEPARATOR,
                keccak256(abi.encode(PERMIT_TYPEHASH, owner, spender, value, nonces[owner]++, deadline))
            )
        );
        address recoveredAddress = ecrecover(digest, v, r, s);
        require(recoveredAddress != address(0) && recoveredAddress == owner, 'UniswapV2: INVALID_SIGNATURE');
        _approve(owner, spender, value);
        */
    }
}


library Math {
    function min(uint x, uint y) internal pure returns (uint) {
        if (x < y) {
            return x;
        } else {
            return y;
        }
    }

    // babylonian method (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Babylonian_method)
    function sqrt(uint y) internal pure returns (uint) {
        if (y > 3) {
            uint z = y;
            uint x = y / 2 + 1;
            while (x < z) {
                z = x;
                x = (y / x + x) / 2;
            }
            return z;
        } else if (y != 0) {
            return 1;
        }
    }
}

library UQ112x112 {
    uint224 constant Q112 = 2**112;

    // encode a uint112 as a UQ112x112
    function encode(uint112 y) internal pure returns (uint224) {
        return uint224(y) * Q112; // never overflows
    }

    // divide a UQ112x112 by a uint112, returning a UQ112x112
    function uqdiv(uint224 x, uint112 y) internal pure returns (uint224) {
        return x / uint224(y);
    }
}

contract UniswapV2Pair is UniswapV2ERC20 {
    using SafeMath  for uint;
    using UQ112x112 for uint224;

    uint public constant MINIMUM_LIQUIDITY = 10**3;
    bytes4 private constant SELECTOR = 0x12345678;// bytes4(keccak256(bytes('transfer(address,uint256)')));

    address public factory;
    address public token0;
    address public token1;

    uint112 private reserve0;           // uses single storage slot, accessible via getReserves
    uint112 private reserve1;           // uses single storage slot, accessible via getReserves
    uint32  private blockTimestampLast; // uses single storage slot, accessible via getReserves

    uint public price0CumulativeLast;
    uint public price1CumulativeLast;
    uint public kLast; // reserve0 * reserve1, as of immediately after the most recent liquidity event

    //uint private unlocked = 1;
    //modifier lock() {
    //    require(unlocked == 1, 'UniswapV2: LOCKED');
    //    unlocked = 0;
    //    _;
    //    unlocked = 1;
    //}
    
    function getReserves() public view returns (uint112, uint112, uint32) {
        return (reserve0,reserve1,blockTimestampLast);
    }

    function _safeTransfer(address token, address to, uint value) private {
        bool success;
        bytes data;
        (success, data) = token.call(abi.encodeWithSignature('transfer(address,uint256)', to, value));
        require(success, 'UniswapV2: TRANSFER_FAILED');
    }

    function _balanceOf(address token, address addr) private returns(uint) {
        bool success;
        bytes data;
        (success, data) = token.delegatecall(abi.encodeWithSignature('balanceOf(address)', addr));
        require(success, 'UniswapV2: TRANSFER_FAILED');
        return abi.decode(data, (uint));
    }

    function _feeTo() private returns(address) {
        // IUniswapV2Factory(factory).feeTo();
        bool success;
        bytes data;
        (success, data) = factory.delegatecall(abi.encodeWithSignature('feeTo'));
        return abi.decode(data, (address));
    }

    event Mint(address indexed sender, uint amount0, uint amount1);
    event Burn(address indexed sender, uint amount0, uint amount1, address indexed to);
    event SwapIn(
        address indexed sender,
        uint amount0In,
        uint amount1In,
        address indexed to
    );
    event SwapOut(
        address indexed sender,
        uint amount0Out,
        uint amount1Out,
        address indexed to
    );
    event Sync(uint112 reserve0, uint112 reserve1);

    constructor() public {
        factory = msg.sender;
    }
    
    // called once by the factory at time of deployment
    function initialize(address _token0, address _token1) external {
        require(msg.sender == factory, 'UniswapV2: FORBIDDEN'); // sufficient check
        token0 = _token0;
        token1 = _token1;
    }
    
    // update reserves and, on the first call per block, price accumulators
    function _update(uint balance0, uint balance1, uint112 _reserve0, uint112 _reserve1) private {
        require(balance0 <= uint112(-1) && balance1 <= uint112(-1), 'UniswapV2: OVERFLOW');
        uint32 blockTimestamp = uint32(block.timestamp % 2**32);
        uint32 timeElapsed = blockTimestamp - blockTimestampLast; // overflow is desired
        if (timeElapsed > 0 && _reserve0 != 0 && _reserve1 != 0) {
            // * never overflows, and + overflow is desired
            uint224 v = UQ112x112.encode(_reserve1);
            v = v.uqdiv(_reserve0);
            price0CumulativeLast += uint(v) * timeElapsed;

            v = UQ112x112.encode(_reserve0);
            v = v.uqdiv(_reserve1);
            price1CumulativeLast += uint(v) * timeElapsed;
        }
        reserve0 = uint112(balance0);
        reserve1 = uint112(balance1);
        blockTimestampLast = blockTimestamp;
        emit Sync(reserve0, reserve1);
    }
    
    // if fee is on, mint liquidity equivalent to 1/6th of the growth in sqrt(k)
    function _mintFee(uint112 _reserve0, uint112 _reserve1) private returns (bool) {
        address feeTo = _feeTo();// IUniswapV2Factory(factory).feeTo();
        bool feeOn = feeTo != address(0);
        uint _kLast = kLast; // gas savings
        if (feeOn) {
            if (_kLast != 0) {
                uint rootK = Math.sqrt(uint(_reserve0).mul(_reserve1));
                uint rootKLast = Math.sqrt(_kLast);
                if (rootK > rootKLast) {
                    uint v = rootK.sub(rootKLast);
                    uint numerator = totalSupply.mul(v);
                    v = rootK.mul(5);
                    uint denominator = v.add(rootKLast);
                    uint liquidity = numerator / denominator;
                    if (liquidity > 0) _mint(feeTo, liquidity);
                }
            }
        } else if (_kLast != 0) {
            kLast = 0;
        }
        return feeOn;
    }
    
    // this low-level function should be called from a contract which performs important safety checks
    function mint(address to) external returns (uint) {
        (uint112 _reserve0, uint112 _reserve1, uint32 _blockTimestampLast) = getReserves(); // gas savings
        uint balance0 = _balanceOf(token0,address(this));// IERC20(token0).balanceOf(address(this));
        uint balance1 = _balanceOf(token1,address(this));// IERC20(token1).balanceOf(address(this));
        uint amount0 = balance0.sub(_reserve0);
        uint amount1 = balance1.sub(_reserve1);
        uint liquidity = 0;
        bool feeOn = _mintFee(_reserve0, _reserve1);
        uint _totalSupply = totalSupply; // gas savings, must be defined here since totalSupply can update in _mintFee
        if (_totalSupply == 0) {
            uint v = amount0.mul(amount1);
            v = Math.sqrt(v);
            liquidity = v.sub(MINIMUM_LIQUIDITY);
           _mint(address(0), MINIMUM_LIQUIDITY); // permanently lock the first MINIMUM_LIQUIDITY tokens
        } else {
            liquidity = Math.min(amount0.mul(_totalSupply) / _reserve0, amount1.mul(_totalSupply) / _reserve1);
        }
        require(liquidity > 0, 'UniswapV2: INSUFFICIENT_LIQUIDITY_MINTED');
        _mint(to, liquidity);

        _update(balance0, balance1, _reserve0, _reserve1);
        if (feeOn) kLast = uint(reserve0).mul(reserve1); // reserve0 and reserve1 are up-to-date
        emit Mint(msg.sender, amount0, amount1);
        return liquidity;
    }
    
    // this low-level function should be called from a contract which performs important safety checks
    function burn(address to) external returns (uint, uint) {
        (uint112 _reserve0, uint112 _reserve1,uint32 _blockTimestampLast) = getReserves(); // gas savings
        address _token0 = token0;                                // gas savings
        address _token1 = token1;                                // gas savings
        uint balance0 = _balanceOf(_token0,address(this));// IERC20(_token0).balanceOf(address(this));
        uint balance1 = _balanceOf(_token1,address(this));// IERC20(_token1).balanceOf(address(this));
        uint liquidity = balanceOf[address(this)];

        bool feeOn = _mintFee(_reserve0, _reserve1);
        uint _totalSupply = totalSupply; // gas savings, must be defined here since totalSupply can update in _mintFee
        uint amount0 = liquidity.mul(balance0) / _totalSupply; // using balances ensures pro-rata distribution
        uint amount1 = liquidity.mul(balance1) / _totalSupply; // using balances ensures pro-rata distribution
        require(amount0 > 0 && amount1 > 0, 'UniswapV2: INSUFFICIENT_LIQUIDITY_BURNED');
        _burn(address(this), liquidity);
        _safeTransfer(_token0, to, amount0);
        _safeTransfer(_token1, to, amount1);
        balance0 = _balanceOf(_token0,address(this));// IERC20(_token0).balanceOf(address(this));
        balance1 = _balanceOf(_token1,address(this));// IERC20(_token1).balanceOf(address(this));

        _update(balance0, balance1, _reserve0, _reserve1);
        if (feeOn) kLast = uint(reserve0).mul(reserve1); // reserve0 and reserve1 are up-to-date
        emit Burn(msg.sender, amount0, amount1, to);
        return (amount0,amount1);
    }
    
    // this low-level function should be called from a contract which performs important safety checks
    function swap(uint amount0Out, uint amount1Out, address to, bytes calldata data) external {
        require(amount0Out > 0 || amount1Out > 0, 'UniswapV2: INSUFFICIENT_OUTPUT_AMOUNT');
        (uint112 _reserve0, uint112 _reserve1,uint32 _blockTimestampLast) = getReserves(); // gas savings
        require(amount0Out < _reserve0 && amount1Out < _reserve1, 'UniswapV2: INSUFFICIENT_LIQUIDITY');
        
        uint balance0;
        uint balance1;
        { // scope for _token{0,1}, avoids stack too deep errors
            address _token0 = token0;
            address _token1 = token1;
            require(to != _token0 && to != _token1, 'UniswapV2: INVALID_TO');
            if (amount0Out > 0) _safeTransfer(_token0, to, amount0Out); // optimistically transfer tokens
            if (amount1Out > 0) _safeTransfer(_token1, to, amount1Out); // optimistically transfer tokens
            balance0 = _balanceOf(_token0,address(this));// IERC20(_token0).balanceOf(address(this));
            balance1 = _balanceOf(_token1,address(this));//IERC20(_token1).balanceOf(address(this));
        }
        uint amount0In = 0; // balance0 > _reserve0 - amount0Out ? balance0 - (_reserve0 - amount0Out) : 0;
        if (balance0 > _reserve0 - amount0Out) {
            amount0In = balance0 - (_reserve0 - amount0Out);
        }
        uint amount1In = 0;// balance1 > _reserve1 - amount1Out ? balance1 - (_reserve1 - amount1Out) : 0;
        if (balance1 > _reserve1 - amount1Out) {
            amount1In = balance1 - (_reserve1 - amount1Out);
        }
        require(amount0In > 0 || amount1In > 0, 'UniswapV2: INSUFFICIENT_INPUT_AMOUNT');
        { 
            // scope for reserve{0,1}Adjusted, avoids stack too deep errors
            uint v1 = balance0.mul(1000);
            uint v2 = amount0In.mul(3);
            uint balance0Adjusted = v1.sub(v2);
            v1 = balance1.mul(1000);
            v2 = amount1In.mul(3);
            uint balance1Adjusted = v1.sub(v2);

            uint v3 = uint(_reserve0).mul(_reserve1);
            require(balance0Adjusted.mul(balance1Adjusted) >= v3.mul(1000**2), 'UniswapV2: K');
        }
        _update(balance0, balance1, _reserve0, _reserve1);
        //amount0In, amount1In
        emit SwapIn(msg.sender, amount0In, amount1In, to);
        emit SwapOut(msg.sender, amount0Out, amount1Out, to);
    }
    
    // force balances to match reserves
    function skim(address to) external {
        address _token0 = token0; // gas savings
        address _token1 = token1; // gas savings
        uint u = _balanceOf(token0,address(this));
        u = u.sub(reserve0);
        _safeTransfer(_token0, to, u);

        u = _balanceOf(token1,address(this));
        u = u.sub(reserve1);
        _safeTransfer(_token1, to, u);
        //_safeTransfer(_token0, to, IERC20(_token0).balanceOf(address(this)).sub(reserve0));
        //_safeTransfer(_token1, to, IERC20(_token1).balanceOf(address(this)).sub(reserve1));
    }
    
    // force reserves to match balances
    function sync() external {
        _update(_balanceOf(token0,address(this)), _balanceOf(token1,address(this)), reserve0, reserve1);
        //_update(IERC20(token0).balanceOf(address(this)), IERC20(token1).balanceOf(address(this)), reserve0, reserve1);
    }
    
}