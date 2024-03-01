
var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');

console.log(web3.utils.toWei('1', 'ether'));

//const BN = require('bn.js');

var BN = web3.utils.BN;

console.log(BN('1234567890123456789').toString());

console.log(web3.utils.BN(web3.utils.toWei('1.23', 'ether')).toString());

//console.log(web3.utils.BN(web3.utils.toWei('1.23', 'ether')).Buffer().toString());

console.log(web3.utils.toHex(web3.utils.toWei('1', 'ether')));

var b = new web3.utils.BN('123','hex');
console.log(b.toString(10));
//console.log(web3.utils.toHex(e.toString()));

var c = new web3.utils.BN(web3.utils.toWei('1', 'ether'),10);
console.log(c.toString());

var d = c.toBuffer();
//var d = new Buffer(c, 'hex')
console.log(d.toString());

//var e = web3.utils.toHex(web3.utils.toWei('1', 'ether'));
//console.log(e);

// var nValue = new web3.utils.Buffer('1234', 'hex')
// console.log(nValue.toString());
