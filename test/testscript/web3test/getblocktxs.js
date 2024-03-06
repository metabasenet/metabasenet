var Web3 = require('web3');
console.log(Web3.version);
// var web3 = new Web3('https://rinkeby.infura.io/');
//var web3 = new Web3('https://mainnet.infura.io/');
var web3 = new Web3('http://localhost:8814');

function getBlockNumber () {
    web3.eth.getBlockNumber().then(
    function(result){
        console.log("blockNumber:"+result);
        throughBlock(result);
    })
}

function throughBlock (blockNumber) {
    if (!blockNumber) {console.log('blockNumber is 0');return false;};
    for (var i = 0; i < blockNumber; i++) {
        getBlock(i);
    };
}

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

function getCode (address) {
    if (!address) {return false;};
    web3.eth.getCode(address).then(
        function(result){
            if (result == '0x') {
                getBalance(address);                
            };          
    });
}

function getBalance (address) {
    console.log('getBalance address: '+address);
    web3.eth.getBalance(address).then(
        function(result){
            console.log('balance: '+address+":\t0x"+result);
            // if (!addressList.includes(address)) {
            //     addressList.push(address);
            //     console.log(address+"\t"+result);
            // };          
        });
}

getBlockNumber();