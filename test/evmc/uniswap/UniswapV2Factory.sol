contract UniswapV2Factory {
    address public feeTo;
    address public feeToSetter;

    mapping(address => mapping(address => address)) public getPair;

    event PairCreated(address indexed token0, address indexed token1, address pair, uint);
    
    constructor() public {
        feeToSetter = msg.sender;
    }
    
    function createPair(address tokenA, address tokenB,address pair) external {
        require(tokenA != tokenB, 'UniswapV2: IDENTICAL_ADDRESSES');
        address pair = address(0);
        address token0 = address(0);
        address token1 = address(0);
        if (tokenA < tokenB) {
            token0 = tokenA;
            token1 = tokenB;
        } else {
            token0 = tokenB;
            token1 = tokenA;
        }
        require(token0 != address(0), 'UniswapV2: ZERO_ADDRESS');
        require(getPair[token0][token1] == address(0), 'UniswapV2: PAIR_EXISTS');
        
        bytes32 salt = sha256(abi.encodePacked(token0, token1));
        address addr = address(0x1234567890);
        pair = create3(addr,salt);
        // ??
        //IUniswapV2Pair(pair).initialize(token0, token1);
        pair.delegatecall(abi.encodeWithSignature("initialize(address,address)",token0,token1));
        getPair[token0][token1] = pair;
        getPair[token1][token0] = pair; // populate mapping in the reverse direction
        emit PairCreated(token0, token1, pair, 0);
    }

    function setFeeTo(address _feeTo) external {
        require(msg.sender == feeToSetter, 'UniswapV2: FORBIDDEN');
        feeTo = _feeTo;
    }

    function setFeeToSetter(address _feeToSetter) external {
        require(msg.sender == feeToSetter, 'UniswapV2: FORBIDDEN');
        feeToSetter = _feeToSetter;
    }
}
