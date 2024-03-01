console.log("Hello World");

var Web3 = require('web3');
//var web3 = new Web3('http://localhost:8814');
var web3 = new Web3('http://94.74.83.128:7103');

var receipt = web3.eth.getTransactionReceipt('0xdab95b23520f06115f8e785d8a1cce7734a51a86ff156a4a2ddc2a17c4258670')
.then(console.log);

// var transaction = web3.eth.getTransactionFromBlock('0x0000014865fe13f90e3bb5f57307d3307210280495672a42886b04330758b220', 2)
// .then(console.log);