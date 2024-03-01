console.log("send raw tx test..............");

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');

var Tx = require('ethereumjs-tx');

function sendtx (gasprice,nonce) {
    console.log('sendtx............');
    var privateKey = Buffer.from('d8b8db5ef512bd8e5a04f83ccfcfe6470c83d3f2e31480828ecf93a9ac0a54e2', 'hex')
    // address: 0x5962974eeb0b17b43edabfc9b747839317aa852f

    // var vGasPrice = new web3.utils.BN(gasprice,10);
    // console.log('gasprice: '+vGasPrice.toString());

    //var vValue = new web3.utils.BN(web3.utils.toWei('23.123456789012345678', 'ether'),10);
    //console.log('value: '+vValue.toString());

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
    
    var tx = new Tx.Transaction({
        to: '0x9614bc2411cb0e2cd56f068f022b695d66d5ad43',
        nonce: nonce,
        gasPrice: web3.utils.BN(gasprice,10),//vGasPrice,//1000000000000,
        gasLimit: 99000,
        value: web3.utils.BN(web3.utils.toWei('23.123456789012345678', 'ether'),10),//vValue,
        data: '0x4cb4ea581d88e0b604039a',
        chainId: '201'
    }, {common:customCommon})

    tx.sign(privateKey);

    console.log('chain id: '+tx.getChainId());

    if (tx.verifySignature()) {
        console.log('Signature verify success!')

        var serializedTx = tx.serialize();
        console.log('serializedTx: 0x'+serializedTx.toString('hex'));
        console.log('tx hash: 0x'+tx.hash().toString('hex'));
        
        web3.eth.sendSignedTransaction('0x' + serializedTx.toString('hex'))
            .on('transactionHash', function(hash){
                console.log('return tx hash: 0x'+hash.toString('hex'));
            })
            .on('error', console.log);
    }
    else{
        console.log('Signature verify fail!')
    }
}

function getaddresstxcount (gasprice) {
    console.log('getaddresstxcount............');
    web3.eth.getTransactionCount("0x5962974eeb0b17b43edabfc9b747839317aa852f")
    .then(
        function(result){
            console.log('address tx count: '+result);
            sendtx(gasprice,result);
        });
}

function getgasprice () {
    console.log('getgasprice............');
    web3.eth.getGasPrice()
    .then(
        function(result){
            console.log('gas price: '+result);
            console.log('gas price: '+web3.utils.fromWei(result, 'ether'));
            getaddresstxcount(result);
        });
}

getgasprice()
