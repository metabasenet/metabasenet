console.log("ETH test!!!!!!!!!");

// var Personal = require('web3-eth-personal');
// var personal = new Personal(Personal.givenProvider || 'http://192.168.192.159:9049');

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');
//var web3 = new Web3('http://192.168.192.159:9049');

// web3.eth.sign("Hello world", "0xcc2695eed79372bb7e7c69cafb9f98a6b4846da2")
// .then(console.log);


web3.eth.sign("Hello world", "0x5962974eeb0b17b43edabfc9b747839317aa852f")
.then(console.log);
