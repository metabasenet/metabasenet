console.log("ETH test!!!!!!!!!");

var Web3 = require('web3');
var web3 = new Web3('http://localhost:8814');


////////////// call //////////////////////////

var abi = JSON.parse('[{\"inputs\":[{\"internalType\":\"address\",\"name\":\"mintAddress\",\"type\":\"address\"},{\"internalType\":\"uint16\",\"name\":\"rewardRatio\",\"type\":\"uint16\"},{\"internalType\":\"uint256\",\"name\":\"tokens\",\"type\":\"uint256\"}],\"name\":\"delegateRedeem\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"mintAddress\",\"type\":\"address\"},{\"internalType\":\"uint16\",\"name\":\"rewardRatio\",\"type\":\"uint16\"},{\"internalType\":\"uint256\",\"name\":\"tokens\",\"type\":\"uint256\"}],\"name\":\"delegateVote\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"uint32\",\"name\":\"page\",\"type\":\"uint32\"}],\"name\":\"getDelegateAddress\",\"outputs\":[{\"internalType\":\"address[]\",\"name\":\"\",\"type\":\"address[]\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[],\"name\":\"getDelegateCount\",\"outputs\":[{\"internalType\":\"uint32\",\"name\":\"\",\"type\":\"uint32\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"delegateAddress\",\"type\":\"address\"}],\"name\":\"getDelegateTotalVotes\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"mintAddress\",\"type\":\"address\"},{\"internalType\":\"address\",\"name\":\"ownerAddress\",\"type\":\"address\"},{\"internalType\":\"uint16\",\"name\":\"rewardRatio\",\"type\":\"uint16\"}],\"name\":\"getDelegateVotes\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[],\"name\":\"getPageSize\",\"outputs\":[{\"internalType\":\"uint32\",\"name\":\"\",\"type\":\"uint32\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"delegateAddress\",\"type\":\"address\"},{\"internalType\":\"address\",\"name\":\"ownerAddress\",\"type\":\"address\"},{\"internalType\":\"uint8\",\"name\":\"rewardMode\",\"type\":\"uint8\"}],\"name\":\"getUserVotes\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"stateMutability\":\"view\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"delegateAddress\",\"type\":\"address\"},{\"internalType\":\"uint8\",\"name\":\"rewardMode\",\"type\":\"uint8\"},{\"internalType\":\"uint256\",\"name\":\"tokens\",\"type\":\"uint256\"}],\"name\":\"userRedeem\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"inputs\":[{\"internalType\":\"address\",\"name\":\"delegateAddress\",\"type\":\"address\"},{\"internalType\":\"uint8\",\"name\":\"rewardMode\",\"type\":\"uint8\"},{\"internalType\":\"uint256\",\"name\":\"tokens\",\"type\":\"uint256\"}],\"name\":\"userVote\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"stateMutability\":\"nonpayable\",\"type\":\"function\"}]');
var myContract = new web3.eth.Contract(abi, '0x00000000000000000000000000000000000000A1');

var arguments = process.argv; 
var cmd = 'getUserVotes';
if (arguments.length >= 3)
{
    cmd = arguments[2];
}
console.log('cmd: '+cmd);

if (cmd == 'getUserVotes')
{
    var delegateAddress = '0xe8b6fb9ab773ca3a12cce9f871e4f7b7bfc0084d';
    var ownerAddress = '0x5962974eeb0b17b43edabfc9b747839317aa852f';
    var rewardMode = 1;
    if (arguments.length >= 4)
    {
        delegateAddress = arguments[3];
    }
    if (arguments.length >= 5)
    {
        ownerAddress = arguments[4];
    }
    if (arguments.length >= 6)
    {
        rewardMode = parseInt(arguments[5]);
    }
    myContract.methods.getUserVotes(delegateAddress, ownerAddress, rewardMode).call()
    .then(console.log);
}
else if (cmd == 'userVote')
{
    var delegateAddress = '0xe8b6fb9ab773ca3a12cce9f871e4f7b7bfc0084d';
    var ownerAddress = '0x5962974eeb0b17b43edabfc9b747839317aa852f';
    var rewardMode = 1;
    var amount = '23.123456789012345678';
    if (arguments.length >= 4)
    {
        delegateAddress = arguments[3];
    }
    if (arguments.length >= 5)
    {
        ownerAddress = arguments[4];
    }
    if (arguments.length >= 6)
    {
        rewardMode = parseInt(arguments[5]);
    }
    if (arguments.length >= 7)
    {
        amount = arguments[6];
    }
    var vValue = new web3.utils.BN(web3.utils.toWei(amount, 'ether'),10);
    myContract.methods.userVote(delegateAddress, rewardMode, vValue).send({
        from: ownerAddress,
        gas: 990000,
        gasPrice: '2000000000000'
    })
    .then(console.log);
}
else if (cmd == 'userRedeem')
{
    var delegateAddress = '0xe8b6fb9ab773ca3a12cce9f871e4f7b7bfc0084d';
    var ownerAddress = '0x5962974eeb0b17b43edabfc9b747839317aa852f';
    var rewardMode = 1;
    var amount = '23.123456789012345678';
    if (arguments.length >= 4)
    {
        delegateAddress = arguments[3];
    }
    if (arguments.length >= 5)
    {
        ownerAddress = arguments[4];
    }
    if (arguments.length >= 6)
    {
        rewardMode = parseInt(arguments[5]);
    }
    if (arguments.length >= 7)
    {
        amount = arguments[6];
    }
    var vValue = new web3.utils.BN(web3.utils.toWei(amount, 'ether'),10);
    myContract.methods.userRedeem(delegateAddress, rewardMode, vValue).send({
        from: ownerAddress,
        gas: 990000,
        gasPrice: '2000000000000'
    })
    .then(console.log);
}
//-------------------------------------
else if (cmd == 'getDelegateVotes')
{
    var mintAddress = '0xa50ede92cb064b695316d75c78abc9f84e18a0cc';
    var ownerAddress = '0x9835c12d95f059eb4eaecb4b00f2ea2c99b2a0d4';
    var rewardRatio = 500;
    if (arguments.length >= 4)
    {
        mintAddress = arguments[3];
    }
    if (arguments.length >= 5)
    {
        ownerAddress = arguments[4];
    }
    if (arguments.length >= 6)
    {
        rewardRatio = parseInt(arguments[5]);
    }
    myContract.methods.getDelegateVotes(mintAddress, ownerAddress, rewardRatio).call()
    .then(console.log);
}
else if (cmd == 'delegateVote')
{
    var mintAddress = '0xa50ede92cb064b695316d75c78abc9f84e18a0cc';
    var ownerAddress = '0x9835c12d95f059eb4eaecb4b00f2ea2c99b2a0d4';
    var rewardRatio = 500;
    var amount = '23.123456789012345678';
    if (arguments.length >= 4)
    {
        mintAddress = arguments[3];
    }
    if (arguments.length >= 5)
    {
        ownerAddress = arguments[4];
    }
    if (arguments.length >= 6)
    {
        rewardRatio = parseInt(arguments[5]);
    }
    if (arguments.length >= 7)
    {
        amount = arguments[6];
    }
    var vValue = new web3.utils.BN(web3.utils.toWei(amount, 'ether'),10);
    myContract.methods.delegateVote(mintAddress, rewardRatio, vValue).send({
        from: ownerAddress,
        gas: 990000,
        gasPrice: '2000000000000'
    })
    .then(console.log);
}
else if (cmd == 'delegateRedeem')
{
    var mintAddress = '0xa50ede92cb064b695316d75c78abc9f84e18a0cc';
    var ownerAddress = '0x9835c12d95f059eb4eaecb4b00f2ea2c99b2a0d4';
    var rewardRatio = 500;
    var amount = '23.123456789012345678';
    if (arguments.length >= 4)
    {
        mintAddress = arguments[3];
    }
    if (arguments.length >= 5)
    {
        ownerAddress = arguments[4];
    }
    if (arguments.length >= 6)
    {
        rewardRatio = parseInt(arguments[5]);
    }
    if (arguments.length >= 7)
    {
        amount = arguments[6];
    }
    var vValue = new web3.utils.BN(web3.utils.toWei(amount, 'ether'),10);
    myContract.methods.delegateRedeem(mintAddress, rewardRatio, vValue).send({
        from: ownerAddress,
        gas: 990000,
        gasPrice: '2000000000000'
    })
    .then(console.log);
}
else if (cmd == 'getPageSize')
{
    myContract.methods.getPageSize().call()
    .then(console.log);
}
else if (cmd == 'getDelegateCount')
{
    myContract.methods.getDelegateCount().call()
    .then(console.log);
}
else if (cmd == 'getDelegateAddress')
{
    myContract.methods.getDelegateAddress(0).call()
    .then(console.log);
}
else if (cmd == 'getDelegateTotalVotes')
{
    myContract.methods.getDelegateTotalVotes('0xe8b6fb9ab773ca3a12cce9f871e4f7b7bfc0084d').call()
    .then(console.log);
}

