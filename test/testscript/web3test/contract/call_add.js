console.log("ETH test!!!!!!!!!");

// var Personal = require('web3-eth-personal');
// var personal = new Personal(Personal.givenProvider || 'http://192.168.192.159:9049');

var Web3 = require('web3');
//var web3 = new Web3('http://192.168.192.159:9049');
var web3 = new Web3('http://localhost:8814');

////////////// contract code //////////////////////////

// pragma solidity ^0.5.0;

// contract add {
//     function adda(uint256 a,uint256 b) public view returns (uint256) {
//         return a+b;
//     }
// }



////////////// call //////////////////////////

var abi = JSON.parse('[{\"constant\":true,\"inputs\":[{\"name\":\"a\",\"type\":\"uint256\"},{\"name\":\"b\",\"type\":\"uint256\"}],\"name\":\"adda\",\"outputs\":[{\"name\":\"\",\"type\":\"uint256\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"}]')
var myContract = new web3.eth.Contract(abi, '0xe7bF45E20df9F7d85433ddFf8a376a2427122362')

myContract.methods.adda(25,3).call()
.then(console.log);

console.log(myContract.methods.adda(25,3).encodeABI())

//0x8a39ec3a00000000000000000000000000000000000000000000000000000000000000190000000000000000000000000000000000000000000000000000000000000003

