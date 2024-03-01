console.log("Hello World");

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');
//var web3 = new Web3('http://94.74.83.128:7103');

web3.eth.getGasPrice()
.then(console.log);
