console.log("Hello World");

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');
// or
//var web3 = new Web3(new Web3.providers.HttpProvider('http://localhost:8545'));
var Accounts = require('web3-eth-accounts');
var accounts = new Accounts('http://localhost:8814');

console.log(web3.eth.accounts.create());

console.log(web3.eth.accounts.hashMessage("Hello World"));

var a = web3.eth.accounts.privateKeyToAccount('0xecfb1f332d5bbf7333370557dc825334f4ffc5f634e98d7f5cf5e5593f215b33');
console.log(a);

web3.eth.getAccounts(console.log);

web3.eth.getBalance("0x108429003ce0116c737e4f83267faacffcf8048d")
.then(console.log);

web3.eth.getBlockTransactionCount("0x00000c9bcc7dcb44336337c395e4d5563694f003c0e64815f78015589daf4f49")
.then(console.log);

web3.eth.getTransaction('0xfa88638ff72d75d08f83436dcbf97df5e1c8ef17a2eac94b41a9832452cc9247')
.then(console.log);
//web3.eth.getTransaction('0xfa8863899e18830d93de2a476db4c078d8ba284af3cf4c69ae6d2f66a2bc7cfe')
//.then(console.log);

//web3.eth.sign("Hello world", "0x108429003ce0116c737e4f83267faacffcf8048d")
//.then(console.log);

web3.eth.getBlockNumber()
.then(console.log);

// web3.eth.getBlock('latest')
// .then(console.log);

// var blockinfo = web3.eth.getBlock('latest');
// console.log(blockinfo);
//console.log('price: '+web3.eth.getGasPrice());
web3.eth.getGasPrice()
.then(console.log);

// var vPrice = new web3.utils.BN(web3.eth.getGasPrice(),10);
// console.log('prive: '+vPrice.toString());