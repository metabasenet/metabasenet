console.log("ETH test!!!!!!!!!");

// var Personal = require('web3-eth-personal');
// var personal = new Personal(Personal.givenProvider || 'http://192.168.192.159:9049');

var Web3 = require('web3');
//var web3 = new Web3('http://192.168.192.159:9049');
var web3 = new Web3('http://localhost:8814');


////////////// call //////////////////////////

var abi = JSON.parse('[{\"constant\":true,\"inputs\":[{\"name\":\"account\",\"type\":\"address\"}],\"name\":\"getamount\",\"outputs\":[{\"name\":\"\",\"type\":\"uint256\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"account\",\"type\":\"address\"},{\"name\":\"value\",\"type\":\"uint256\"}],\"name\":\"setamount\",\"outputs\":[{\"name\":\"\",\"type\":\"bool\"}],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"constructor\"}]')
var myContract = new web3.eth.Contract(abi, '0xe7bF45E20df9F7d85433ddFf8a376a2427122362')

myContract.methods.getamount('0x8fa079a96ce08f6e8a53c1c00566c434b248bfa4').call()
.then(console.log);
