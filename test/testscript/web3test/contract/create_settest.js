console.log("ETH test!!!!!!!!!");

// var Personal = require('web3-eth-personal');
// var personal = new Personal(Personal.givenProvider || 'http://192.168.192.159:9049');

var Web3 = require('web3');
//var web3 = new Web3('http://192.168.192.159:9049');
var web3 = new Web3('http://localhost:8814');


////////////// create //////////////////////////

var bytecode = "608060405234801561001057600080fd5b506175306000803373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055506101ec806100656000396000f3fe60806040526004361061004c576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063b4cef28d14610051578063f38255f9146100b6575b600080fd5b34801561005d57600080fd5b506100a06004803603602081101561007457600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190505050610129565b6040518082815260200191505060405180910390f35b3480156100c257600080fd5b5061010f600480360360408110156100d957600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff16906020019092919080359060200190929190505050610171565b604051808215151515815260200191505060405180910390f35b60008060008373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020549050919050565b6000816000808573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002081905550600190509291505056fea165627a7a72305820cd3f834c0cad45a0e0cbe343010ab43df5f2ec56fd297f515ba55c3cc6057a520029"
web3.eth.estimateGas({code: bytecode})
.then(console.log);

var abi = JSON.parse('[{\"constant\":true,\"inputs\":[{\"name\":\"account\",\"type\":\"address\"}],\"name\":\"getamount\",\"outputs\":[{\"name\":\"\",\"type\":\"uint256\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"account\",\"type\":\"address\"},{\"name\":\"value\",\"type\":\"uint256\"}],\"name\":\"setamount\",\"outputs\":[{\"name\":\"\",\"type\":\"bool\"}],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"constructor\"}]')
var myContract = new web3.eth.Contract(abi)


myContract.deploy({
    data: bytecode
})
.send({
    from: '0x5962974eeb0b17b43edabfc9b747839317aa852f',
    gas: 990000,
    gasPrice: '3000000000000'
})
.then(function(newContractInstance){
    console.log(newContractInstance.options.address) // instance with the new contract address
});

