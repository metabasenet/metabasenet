console.log("ETH test!!!!!!!!!");

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');

////////////// call //////////////////////////

var abi = JSON.parse('[{\"inputs\":[],\"stateMutability\":\"nonpayable\",\"type\":\"constructor\"},{\"inputs\":[],\"name\":\"addr\",\"outputs\":[{\"internalType\":\"address\",\"name\":\"\",\"type\":\"address\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"uint256\",\"name\":\"a\",\"type\":\"uint256\"}],\"name\":\"set\",\"outputs\":[],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[],\"name\":\"value\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"stateMutability\":\"view\",\"type\":\"function\"}]');
var myContract = new web3.eth.Contract(abi, '0x9d458F4497D8945402258Bb7e209321C52f60d53')

myContract.methods.value().call()
.then(console.log);
