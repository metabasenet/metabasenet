console.log("test2...............");

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');

// web3.eth.getTransactionFromBlock('0x000023b43374245d68082206151b3488db263edf62ffe1ea6dc1dd218b58225e', 1)
// .then(console.log);

// web3.eth.getTransactionFromBlock('latest', 0)
// .then(console.log);

// web3.eth.getTransactionFromBlock('0x23B', 0)
// .then(console.log);

// web3.eth.getTransactionFromBlock(12, 0)
// .then(console.log);

// web3.eth.estimateGas({
//     to: "0x5962974eeb0b17b43edabfc9b747839317aa852f",
//     data: "0xc6888fa10000000000000000000000000000000000000000000000000000000000000003"
// })
// .then(console.log);

// web3.eth.signTransaction({
//     from: "0x5962974eeb0b17b43edabfc9b747839317aa852f",
//     gasPrice: "1000000000000",
//     gas: "99000",
//     to: '0x9614bc2411cb0e2cd56f068f022b695d66d5ad43',
//     value: web3.utils.toWei('23.123456789012345678', 'ether'),//"1000000000000000000",
//     data: "0x4cb4ea581d88e0b604039a"
// }).then(console.log);

// web3.eth.sendTransaction({
//     from: "0x5962974eeb0b17b43edabfc9b747839317aa852f",
//     gasPrice: "1000000000000",
//     gas: "99000",
//     to: '0x9614bc2411cb0e2cd56f068f022b695d66d5ad43',
//     value: web3.utils.toWei('23.123456789012345678', 'ether'),//"1000000000000000000",
//     data: "0x4cb4ea581d88e0b604039a"
// })
// .on('transactionHash', function(hash){
//     console.log('return tx hash: 0x'+hash.toString('hex'));
// })
// .on('error', console.log);

// web3.eth.sendTransaction({
//     from: "0x5962974eeb0b17b43edabfc9b747839317aa852f",
//     gasPrice: "1000000000000",
//     gas: "99000",
//     to: '0x9614bc2411cb0e2cd56f068f022b695d66d5ad43',
//     value: web3.utils.toWei('23.123456789012345678', 'ether'),//"1000000000000000000",
//     data: "0x4cb4ea581d88e0b604039a"
// })
// .once('transactionHash', function(hash){ console.log('return tx hash: 0x'+hash); })
// .once('receipt', function(receipt){ console.log('return receipt: '+receipt); })
// .on('confirmation', function(confNumber, receipt){ console.log('return confirmation: '+confNumber); })
// .on('error', function(error){ console.log('return error: '+error); })
// .then(function(receipt){
//     // will be fired once the receipt is mined
//     console.log('return receipt: '+receipt);
// });

web3.eth.estimateGas({
    to: "0x8d414bfe051431d910cf9f33cb6c1d0aa0eb5583",
    data: "0xc6888fa10000000000000000000000000000000000000000000000000000000000000003"
})
.then(console.log);

// web3.eth.estimateGas({
//     from: "0xbbf0a87f870372c18445c7e9eb0337caffd43106",
//     to: "0xbbf0a87f870372c18445c7e9eb0337caffd43106",
//     data: "0xbbf0a87f870372c18445c7e43106"
// })
// .then(console.log);

web3.eth.call({
    to: "0x8d414bfe051431d910cf9f33cb6c1d0aa0eb5583", // contract address
    data: "0xf2a360be000000000000000000000000e7ee178b1c67114c513126719a798dedf6f69b1b"
})
.then(console.log);