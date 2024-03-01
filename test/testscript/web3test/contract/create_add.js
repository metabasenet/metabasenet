console.log("ETH test!!!!!!!!!");

// var Personal = require('web3-eth-personal');
// var personal = new Personal(Personal.givenProvider || 'http://192.168.192.159:9049');

var Web3 = require('web3');
//var web3 = new Web3('http://192.168.192.159:9049');
var web3 = new Web3('http://localhost:8814');


////////////// create //////////////////////////

var bytecode = "608060405234801561001057600080fd5b5060d28061001f6000396000f3fe608060405260043610603f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a39ec3a146044575b600080fd5b348015604f57600080fd5b50608360048036036040811015606457600080fd5b8101908080359060200190929190803590602001909291905050506099565b6040518082815260200191505060405180910390f35b600081830190509291505056fea165627a7a72305820a28c5a1aa380b1998c2e97560956d46e687dc70624e969e3e13f027c223d07310029"
web3.eth.estimateGas({code: bytecode})
.then(console.log);

var abi = JSON.parse('[{\"constant\":true,\"inputs\":[{\"name\":\"a\",\"type\":\"uint256\"},{\"name\":\"b\",\"type\":\"uint256\"}],\"name\":\"adda\",\"outputs\":[{\"name\":\"\",\"type\":\"uint256\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"}]')
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


//0x521575dB27a84D2c7651F6F6821232C5cABc0519



