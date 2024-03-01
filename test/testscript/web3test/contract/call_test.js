console.log("Hello World");

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');
//var web3 = new Web3('http://94.74.83.128:7103');

// var fs= require('fs');
// var path = require('path');

// fs.readFile(path.join(__dirname, 'erc20_sol_erc20.abi'), function (err,bytesRead) {
//     if (err) throw err;
//     //console.log(bytesRead);
//     //var abi = bytesRead;
//     var abi = JSON.parse(bytesRead);
//     var MyContract = new web3.eth.Contract(abi, '0x8d414bfe051431d910cf9f33cb6c1d0aa0eb5583');
//     MyContract.methods.totalSupply().call({from:'0x5962974eeb0b17b43edabfc9b747839317aa852f'})
//     .then(console.log);
// });

// var MyContract = new web3.eth.contract(abi, '0x8d414bfe051431d910cf9f33cb6c1d0aa0eb5583');
// MyContract.methods.totalSupply().call()
// .then(console.log);


var abi = JSON.parse('[{\"inputs\":[{\"internalType\":\"uint256\",\"name\":\"n\",\"type\":\"uint256\"}],\"stateMutability\":\"nonpayable\",\"type\":\"constructor\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"internalType\":\"address\",\"name\":\"_from\",\"type\":\"address\"},{\"indexed\":true,\"internalType\":\"address\",\"name\":\"_to\",\"type\":\"address\"},{\"indexed\":false,\"internalType\":\"uint256\",\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"Transfer\",\"type\":\"event\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"owner\",\"type\":\"address\"},{\"internalType\":\"address\",\"name\":\"spender\",\"type\":\"address\"}],\"name\":\"allowance\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"spender\",\"type\":\"address\"},{\"internalType\":\"uint256\",\"name\":\"value\",\"type\":\"uint256\"}],\"name\":\"approve\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"account\",\"type\":\"address\"}],\"name\":\"balanceOf\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"spender\",\"type\":\"address\"},{\"internalType\":\"uint256\",\"name\":\"subtractedValue\",\"type\":\"uint256\"}],\"name\":\"decreaseAllowance\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"spender\",\"type\":\"address\"},{\"internalType\":\"uint256\",\"name\":\"addedValue\",\"type\":\"uint256\"}],\"name\":\"increaseAllowance\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[],\"name\":\"totalSupply\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"recipient\",\"type\":\"address\"},{\"internalType\":\"uint256\",\"name\":\"amount\",\"type\":\"uint256\"}],\"name\":\"transfer\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"_sender\",\"type\":\"address\"},{\"internalType\":\"address\",\"name\":\"recipient\",\"type\":\"address\"},{\"internalType\":\"uint256\",\"name\":\"amount\",\"type\":\"uint256\"}],\"name\":\"transferFrom\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"stateMutability\":\"nonpayable\",\"type\":\"function\"}]');

// var MyContract = new web3.eth.Contract(abi, '0x8d414bfe051431d910cf9f33cb6c1d0aa0eb5583');
// MyContract.methods.totalSupply().call({from:'0x5962974eeb0b17b43edabfc9b747839317aa852f'})
// .then(console.log);

var MyContract = new web3.eth.Contract(abi, '0x8d414bfe051431d910cf9f33cb6c1d0aa0eb5583');
MyContract.methods.balanceOf('0xe7ee178b1c67114c513126719a798dedf6f69b1b').call({from:'0x5962974eeb0b17b43edabfc9b747839317aa852f'})
.then(console.log);

var MyContract = new web3.eth.Contract(abi, '0x8d414bfe051431d910cf9f33cb6c1d0aa0eb5583');
MyContract.methods.balanceOf('0xef0a0BD37c8D1984A65D9Bede4c6f74f4094fC04').call({from:'0x5962974eeb0b17b43edabfc9b747839317aa852f'})
.then(console.log);


