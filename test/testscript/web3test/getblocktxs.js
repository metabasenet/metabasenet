var Web3 = require('web3');
console.log(Web3.version);
//设置web3对象
// var web3 = new Web3('https://rinkeby.infura.io/'); //rinkeby测速网络节点地址，开发测试可以使用测试网络，快
//var web3 = new Web3('https://mainnet.infura.io/');//以太坊正式网络节点地址
var web3 = new Web3('http://localhost:8814');

//获取当前区块高度
function getBlockNumber () {
    web3.eth.getBlockNumber().then(
    function(result){
        console.log("blockNumber:"+result);
        throughBlock(result);
    })
}

//从创世区块0开始遍历
function throughBlock (blockNumber) {
    if (!blockNumber) {console.log('blockNumber is 0');return false;};
    for (var i = 0; i < blockNumber; i++) {
        getBlock(i);
    };
}

//获取当前区块的信息
function getBlock (blockNumber) {
    console.log('getBlock number: '+blockNumber);
    web3.eth.getBlock(blockNumber).then(
        function(result){
            transactions = result.transactions;
            console.log('getBlock tx length: '+transactions.length);
            for (var i = 0; i < transactions.length; i++) {
                getTransactions(transactions[i]);
            }
        });
}

//获取交易信息
function getTransactions (txh) {
    console.log('getTransactions txid: '+txh);
    web3.eth.getTransaction(txh).then(
        function(result){
            from = result.from;
            to = result.to;
            //getCode(from);
            //getCode(to);
            console.log('getTransactions from: '+from);
            console.log('getTransactions to: '+to);

            // if (to != '0x0000000000000000000000000000000000000000')
            // {
            //     getBalance(to);
            // }
    });
}

// 验证地址是否是合约地址
function getCode (address) {
    if (!address) {return false;};
    web3.eth.getCode(address).then(
        function(result){
            if (result == '0x') {
                getBalance(address);                
            };          
    });
}

// 获取地址余额
function getBalance (address) {
    console.log('getBalance address: '+address);
    web3.eth.getBalance(address).then(
        function(result){
            console.log('balance: '+address+":\t0x"+result); //地址 余额
            // if (!addressList.includes(address)) {
            //     addressList.push(address);
            //     console.log(address+"\t"+result); //地址 余额
            // };          
        });
}

getBlockNumber();