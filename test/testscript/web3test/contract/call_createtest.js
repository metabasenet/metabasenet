console.log("ETH test!!!!!!!!!");

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');


////////////// call //////////////////////////

var abi = JSON.parse('[{\"inputs\":[],\"stateMutability\":\"nonpayable\",\"type\":\"constructor\"},{\"inputs\":[],\"name\":\"create1\",\"outputs\":[],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[],\"name\":\"create1_addr\",\"outputs\":[{\"internalType\":\"address\",\"name\":\"\",\"type\":\"address\"}],\"stateMutability\":\"view\",\"type\":\"function\"}]');
var myContract = new web3.eth.Contract(abi, '0xe7bF45E20df9F7d85433ddFf8a376a2427122362')

myContract.methods.create1().send({
    from: '0x5962974eeb0b17b43edabfc9b747839317aa852f',
    gas: 990000,
    gasPrice: '2000000000000'
})
.then(console.log);

