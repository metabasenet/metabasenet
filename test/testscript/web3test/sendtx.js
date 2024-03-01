console.log("test3...............");

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');


console.log(web3.version)

web3.eth.sendTransaction({
    from: '0x5962974eeb0b17b43edabfc9b747839317aa852f',
    to: '0xbbf0a87f870372c18445c7e9eb0337caffd43106',
    gas: '21000',
    value: '10000000'
  })
  .on('transactionHash', function(hash){
    console.log('return tx hash: '+hash.toString('hex'));
  })
  .on('receipt', function(receipt){
    console.log(receipt);
  })
  .on('confirmation', function(confirmationNumber, receipt){ })
  .on('error', console.error); 