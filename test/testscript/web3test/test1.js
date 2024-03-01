console.log("test1...............");

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');

console.log(web3.utils.hexToNumberString('0xea'));
console.log(web3.utils.toBN('0xea').toString());
console.log(web3.utils.fromWei('1', 'ether'));

web3.eth.getProtocolVersion()
.then(console.log);

web3.eth.isSyncing()
.then(console.log);

web3.eth.getCoinbase()
.then(console.log);

web3.eth.isMining()
.then(console.log);

web3.eth.getHashrate()
.then(console.log);

web3.eth.getGasPrice()
.then(
    function(result){
        console.log('gas price: '+web3.utils.fromWei(result, 'ether'));
    });

web3.eth.getAccounts()
.then(console.log);

web3.eth.getBalance("0x5962974eeb0b17b43edabfc9b747839317aa852f")
.then(
    function(result){
        console.log('balance: '+web3.utils.fromWei(result, 'ether'));
    });

web3.eth.getBlockNumber()
.then(
    function(result){
        console.log('block number: '+result);
    });

web3.eth.getBlockTransactionCount("0x000021ab31205f819e83dd5c8ded02d093af0e47ddc3cf671ebbff06e2f3a42f")
.then(
    function(result){
        console.log('block tx count: '+result);
    });

web3.eth.getBlockTransactionCount("genesis")
.then(
    function(result){
        console.log('genesis block tx count: '+result);
    });

web3.eth.getBlockTransactionCount("latest")
.then(
    function(result){
        console.log('latest block tx count: '+result);
    });

web3.eth.getTransaction('0xc000b42fdc28942909e325fa1f2d2b85e72b4e0a735eb2d05d9955c55e6c806b')
.then(console.log);

web3.eth.getTransactionCount("0x5962974eeb0b17b43edabfc9b747839317aa852f")
.then(
    function(result){
        console.log('address tx count: '+result);
    });

web3.eth.getTransactionFromBlock('0x000023b43374245d68082206151b3488db263edf62ffe1ea6dc1dd218b58225e', 1)
.then(console.log);

web3.eth.getTransactionFromBlock('latest', 0)
.then(console.log);

web3.eth.getTransactionFromBlock('0x23B', 0)
.then(console.log);

web3.eth.getTransactionReceipt('0xc000b42fdc28942909e325fa1f2d2b85e72b4e0a735eb2d05d9955c55e6c806b')
.then(console.log);

web3.eth.getBlock('0x000021ab31205f819e83dd5c8ded02d093af0e47ddc3cf671ebbff06e2f3a42f')
.then(console.log);

web3.eth.getBlock(234)
.then(console.log);

web3.eth.getBlock('latest')
.then(console.log);
