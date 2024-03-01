console.log("send raw tx test..............");

var Tx = require('ethereumjs-tx');
var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');

console.log('version: '+web3.version);

var privateKey = new Buffer('d8b8db5ef512bd8e5a04f83ccfcfe6470c83d3f2e31480828ecf93a9ac0a54e2', 'hex')
// address: 0x5962974eeb0b17b43edabfc9b747839317aa852f

var vValue = new web3.utils.BN(web3.utils.toWei('23.123456789012345678', 'ether'),10);
console.log('value: '+vValue.toString());


const Common=require('ethereumjs-common').default
const customCommon = Common.forCustomChain(
  'mainnet',
  {
    name: 'my-network',
    networkId: 201,
    chainId: 201,
  },
  'petersburg',
)

const rawTx = {
  nonce: '0x0E',
  gasPrice: '0xE8D4A51000',
  gasLimit: '0x182B8',
  to: '0x9614bc2411cb0e2cd56f068f022b695d66d5ad43',
  value: '0x12345678',
  data: '0x4cb4ea581d88e0b604039a',
  chainId: '201'
};
var tx = new Tx.Transaction(rawTx, {common:customCommon});

tx.sign(privateKey);

console.log('chain id: '+tx.getChainId());

var serializedTx = tx.serialize();

console.log('serializedTx: 0x'+serializedTx.toString('hex'));

if (tx.verifySignature()) {
    console.log('Signature verify success!')
  }
  else{
    console.log('Signature verify fail!')
  }

console.log('tx hash: 0x'+tx.hash().toString('hex'));
  
 web3.eth.sendSignedTransaction('0x' + serializedTx.toString('hex'))
 .on('transactionHash', function(hash){
  console.log('return tx hash: '+hash.toString('hex'));
})
 .on('error', console.log);
 
