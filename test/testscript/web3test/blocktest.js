console.log("block test..............");

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');
// or
//var web3 = new Web3(new Web3.providers.HttpProvider('http://localhost:8545'));

web3.eth.getBlockNumber()
.then(console.log);

// web3.eth.getBlock('latest', true)
// .then(console.log);

// web3.eth.getBlock('0x000012018f9fd9e6f68e2012fafcde82b5f7780f6c503e1b0db955ae27c087a4')
// .then(console.log);

// web3.eth.getBlockTransactionCount("latest")
// .then(console.log);

// web3.eth.getTransaction('0xfa8863903f6021106ed621346f4f5b7cfed3d031794efd95b8d0e24a13199d25')
// .then(console.log);

// web3.eth.getTransactionFromBlock('0x00001231031237cd6a212402ecc60a67048c41a4478f624b8d3646ec274f68b1', 1)
// .then(console.log);

// web3.eth.getTransactionCount("0x2338e108BcB4BD56eD1b4584848A1Ef2Fcf2FA75")
// .then(console.log);

web3.eth.sendTransaction({
    from: '0x108429003ce0116c737e4f83267faacffcf8048d',
    to: '0x101dd52254910510e86cccaab3acd35bb5d06236',
    value: '1000000000000000'
})
.then(console.log);

