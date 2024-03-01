console.log("ETH test!!!!!!!!!");

// var Personal = require('web3-eth-personal');
// var personal = new Personal(Personal.givenProvider || 'http://192.168.192.159:9049');

var Web3 = require('web3');
//var web3 = new Web3('http://localhost:8545');
var web3 = new Web3('http://192.168.192.159:9049');

// var receipt = web3.eth.getTransactionReceipt('0x1c1e868546a77f4ec997a0491ee83a28c3e944234eaa05dc5b17720e00b47aa4')
// .then(console.log);


////////////// contract code //////////////////////////

// pragma solidity ^0.5.0;

// contract add {
//     function adda(uint256 a,uint256 b) public view returns (uint256) {
//         return a+b;
//     }
// }


////////////// create //////////////////////////

// var bytecode = "608060405234801561001057600080fd5b5060d28061001f6000396000f3fe608060405260043610603f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a39ec3a146044575b600080fd5b348015604f57600080fd5b50608360048036036040811015606457600080fd5b8101908080359060200190929190803590602001909291905050506099565b6040518082815260200191505060405180910390f35b600081830190509291505056fea165627a7a72305820a28c5a1aa380b1998c2e97560956d46e687dc70624e969e3e13f027c223d07310029"
// web3.eth.estimateGas({data: bytecode})
// .then(console.log);

// var abi = JSON.parse('[{\"constant\":true,\"inputs\":[{\"name\":\"a\",\"type\":\"uint256\"},{\"name\":\"b\",\"type\":\"uint256\"}],\"name\":\"adda\",\"outputs\":[{\"name\":\"\",\"type\":\"uint256\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"}]')
// var myContract = new web3.eth.Contract(abi)


// myContract.deploy({
//     data: bytecode
// })
// .send({
//     from: '0xcc2695eed79372bb7e7c69cafb9f98a6b4846da2',
//     gas: 150000,
//     gasPrice: '3000000000000'
// })
// .then(function(newContractInstance){
//     console.log(newContractInstance.options.address) // instance with the new contract address
// });


//0xbD4347C0884cE42B988396d5c009A3f5D61B68a1


////////////// call //////////////////////////

var abi = JSON.parse('[{\"constant\":true,\"inputs\":[{\"name\":\"a\",\"type\":\"uint256\"},{\"name\":\"b\",\"type\":\"uint256\"}],\"name\":\"adda\",\"outputs\":[{\"name\":\"\",\"type\":\"uint256\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"}]')
var myContract = new web3.eth.Contract(abi, '0xbD4347C0884cE42B988396d5c009A3f5D61B68a1')

myContract.methods.adda(25,3).call()
.then(console.log);

//{from:'0xcc2695eed79372bb7e7c69cafb9f98a6b4846da2'}

////////////// call //////////////////////////

// var abi = JSON.parse('[{\"constant\":true,\"inputs\":[{\"name\":\"a\",\"type\":\"uint256\"},{\"name\":\"b\",\"type\":\"uint256\"}],\"name\":\"adda\",\"outputs\":[{\"name\":\"\",\"type\":\"uint256\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"}]')
// var myContract = new web3.eth.Contract(abi)

// myContract.methods.adda(25,3).encodeABI()
// .then(console.log);

