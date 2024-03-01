

// Passing in the eth or web3 package is necessary to allow retrieving chainId, gasPrice and nonce automatically
// for accounts.signTransaction().
var Accounts = require('web3-eth-accounts');
var accounts = new Accounts('http://localhost:8814')

var a = accounts.create()
console.log(a);

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');

console.log(web3.eth.accounts.create());

// web3.eth.accounts.signTransaction({
//     to: '0x67CBC4A5E5aCFf4803a9Ed2D71EC5797d85B076f',
//     value: '1000000000',
//     gas: 2000000
// }, '0x4c0883a69102937d6231471b5dbb6204fe5129617082792ae468d01a3f362318')
// .then(console.log);


// web3.eth.personal.unlockAccount("0x101dd52254910510e86cccaab3acd35bb5d06236", "123", 600)
// .then(console.log('Account unlocked!'));

console.log(web3.utils.randomHex(32));

console.log(web3.utils.sha3('234'));

web3.eth.getAccounts()
.then(console.log);