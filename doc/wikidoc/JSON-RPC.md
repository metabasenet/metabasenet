### command line format:
```
Usage:
        metabasenet-cli (OPTIONS) COMMAND

Run metabasenet RPC client

Options:
  -help                                 Get more information
  -daemon                               Run server in background
  -debug                                Run in debug mode
  -datadir=<path>                       Root directory of resources
  -conf=<file>                          Configuration file name
  -testnet                              Use the test network
  -fasttest                             Fast test
  -testmainnet                          Mainnet test
  -nowallet                             Launch server without wallet
  -version                              Get metabasenet version
  -purge                                Purge database and blockfile
  -logfilesize=<size>                   Log file size(M) (default: 200M)
  -loghistorysize=<size>                Log history size(M) (default: 2048M)
  -fulldb                               Launch server full db
  -chainid=<chainid>                    chain id (default is 0, mainnet is 100, testnet is 101)
  -netid=<netid>                        net id (default is chainid)
  -modrpcthreads                        rpc thread count (default is 1)
  -rewardcheck                          Check reward tx (default is false)
  -rpcport=port                         Listen for JSON-RPC connections on <port> (default: 8812 or testnet: 8814))
  -rpclisten                            Accept RPC IPv4 and IPv6 connections (default: 0)
  -rpclisten4                           Accept RPC IPv4 connections (default: 0)
  -rpclisten6                           Accept RPC IPv6 connections (default: 0)
  -rpcuser=<user>                       <user> name for JSON-RPC connections
  -rpcpassword=<password>               <password> for JSON-RPC connections
  -rpcssl                               Use OpenSSL (https) for JSON-RPC connections or not (default false)
  -norpcsslverify                       Verify SSL or not (default yes)
  -rpccafile=<file.crt>                 SSL CA file name (default ca.crt)
  -rpccertfile=<file.crt>               Server certificate file (default: server.crt)
  -rpcpkfile=<file.pem>                 Server private key (default: server.pem)
  -rpcciphers=<ciphers>                 Acceptable ciphers (default: TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH)
  -statdata                             Enable statistical data or not (default false)
  -rpclog                               Enable write RPC log (default true)
  -rpchost=<ip>                         Send commands to node running on <ip> (default: 127.0.0.1)
  -rpctimeout=<time>                    Connection timeout <time> seconds (default: 120)
```

## commands
### System
 - [help](#help): List commands, or get help for a command.
 - [stop](#stop): Stop metabasenet server.
 - [version](#version): Get metabasenet server version.
### Network
 - [getpeercount](#getpeercount): Return the number of connections to other nodes.
 - [listpeer](#listpeer): Return data about each connected network node.
 - [addnode](#addnode): Attempt to add a node into the addnode list.
 - [removenode](#removenode): Attempt to remove a node from the addnode list.
 - [getforkport](#getforkport): Get rpc listen port of fork.
### Blockchain & TxPool
 - [getforkcount](#getforkcount): Return the number of forks.
 - [listfork](#listfork): If true, list of all forks, or subscribed forks
 - [getgenealogy](#getgenealogy): Return the list of ancestry and subline.
 - [getblocklocation](#getblocklocation): Return the location with given block.
 - [getblockcount](#getblockcount): Return the number of blocks in the given fork, includes extended block and vacant block
 - [getblockhash](#getblockhash): Return a list of block hash in the fork at a specific height.
 - [getblocknumberhash](#getblocknumberhash): Return a block hash in the fork at a specific number.
 - [getblock](#getblock): Return details of a block with given block-hash.
 - [getblockdetail](#getblockdetail): Return details of a block with given block-hash.
 - [getblockdata](#getblockdata): Return details of a block with given block hash or height or number.
 - [gettxpool](#gettxpool): Get transaction pool information
 - [gettransaction](#gettransaction): Get transaction information
 - [sendtransaction](#sendtransaction): Submit raw transaction (serialized, hex-encoded) to local node and network.
 - [getforkheight](#getforkheight): Return the number of height in the given fork.
 - [getvotes](#getvotes): Get votes
 - [listdelegate](#listdelegate): List delegate
 - [getdelegatevotes](#getdelegatevotes): Get delegate total votes
 - [getuservotes](#getuservotes): Get user votes
 - [getaddresscount](#getaddresscount): Get address count
### Wallet
 - [listkey](#listkey): Return Object that has pubkey as keys, associated status as values.
 - [getnewkey](#getnewkey): Return a new pubkey for receiving payments.
 - [removekey](#removekey): Remove Key in Wallet.
 - [encryptkey](#encryptkey): Changes the passphrase for <oldpassphrase> to <passphrase>
 - [lockkey](#lockkey): Removes the encryption key from memory, locking the key.
 - [unlockkey](#unlockkey): Unlock the key.
 - [importprivkey](#importprivkey): Add a private key to your wallet.
 - [importpubkey](#importpubkey): Add a public key to your wallet.
 - [importkey](#importkey): Reveal the serialized key corresponding to <pubkey>.
 - [exportkey](#exportkey): Reveal the serialized key corresponding to <pubkey>.
 - [addnewtemplate](#addnewtemplate): Return encoded address for the given template id.
 - [importtemplate](#importtemplate): Return encoded address for the given template.
 - [exporttemplate](#exporttemplate): Return encoded address for the given template.
 - [removetemplate](#removetemplate): Remove template in Wallet.
 - [validateaddress](#validateaddress): Return information about <address>.
 - [getbalance](#getbalance): Get balance of an address.
 - [listtransaction](#listtransaction): Return transactions list.
 - [sendfrom](#sendfrom): Send a transaction.
 - [createtransaction](#createtransaction): Create a transaction.
 - [signtransaction](#signtransaction): Sign a transaction.
 - [signmessage](#signmessage): Sign a message with the private key of an pubkey
 - [addusercoin](#addusercoin): Add user coin
 - [addcontractcoin](#addcontractcoin): Add contract coin
 - [getcoininfo](#getcoininfo): Get coin information by symbol
 - [listcoininfo](#listcoininfo): List coin information
 - [getdexcoinpair](#getdexcoinpair): Get dex coin pair by symbol pair
 - [listdexcoinpair](#listdexcoinpair): List dex coin pair
 - [senddexordertx](#senddexordertx): Send dexorder transaction
 - [listdexorder](#listdexorder): List dex order
 - [getdexsymboltype](#getdexsymboltype): Get dex symbol type, sell symbol and buy symbol
 - [listrealtimedexorder](#listrealtimedexorder): List realtime dex order
 - [sendcrosstransfertx](#sendcrosstransfertx): Send crosschain transfer transaction
 - [getcrosstransferamount](#getcrosstransferamount): Get crosschain transfer amount
 - [listaddress](#listaddress): List all of the addresses from pub keys and template ids
 - [exportwallet](#exportwallet): Export all of keys and templates from wallet to a specified file in JSON format.
 - [importwallet](#importwallet): Import keys and templates from an archived file to the wallet in JSON format.
 - [makeorigin](#makeorigin): Return hex-encoded block.
### Util
 - [verifymessage](#verifymessage): Verify a signed message
 - [makekeypair](#makekeypair): Make a public/private key pair.
 - [getpubkey](#getpubkey): Return public key by address or private key.
 - [getpubkeyaddress](#getpubkeyaddress): Return encoded address for the given public key.
 - [maketemplate](#maketemplate): Return encoded address for the given template id.
 - [decodetransaction](#decodetransaction): Return a JSON object representing the serialized, hex-encoded transaction.
 - [gettxfee](#gettxfee): Return TxFee for vchData Hex data
 - [makesha256](#makesha256): Make sha256
 - [querystat](#querystat): Query statistical data
 - [reversehex](#reversehex): Reverse a hex string by byte
 - [callcontract](#callcontract): Call contract api
 - [gettransactionreceipt](#gettransactionreceipt): Get transaction receipt
 - [getcontractmuxcode](#getcontractmuxcode): Get contract mux code, including source code, name and code
 - [gettemplatemuxcode](#gettemplatemuxcode): Get template mux code
 - [listcontractcode](#listcontractcode): List contract code
 - [listcontractaddress](#listcontractaddress): List contract address
 - [getdestcontract](#getdestcontract): Get address contract
 - [getcontractsource](#getcontractsource): Get contract source
 - [getcontractcode](#getcontractcode): Get contract code
 - [funcsign](#funcsign): Function sign
 - [makehash](#makehash): Make hash
 - [addblacklistaddress](#addblacklistaddress): Add blacklist address
 - [removeblacklistaddress](#removeblacklistaddress): Remove blacklist address
 - [listblacklistaddress](#listblacklistaddress): List blacklist address
 - [setfunctionaddress](#setfunctionaddress): Set function address
 - [listfunctionaddress](#listfunctionaddress): List function address
 - [setmintmingasprice](#setmintmingasprice): Set mint min gas price
 - [getmintmingasprice](#getmintmingasprice): Get mint min gas price
 - [listmintmingasprice](#listmintmingasprice): List mint min gas price
 - [checkatbloomfilter](#checkatbloomfilter): Check if the data is in bloomfilter
 - [web3_clientVersion](#web3_clientVersion): [ETH] Get web3 client version
 - [web3_sha3](#web3_sha3): [ETH] Keccak-256
 - [net_version](#net_version): [ETH] Get net version
 - [net_listening](#net_listening): [ETH] Get net listen status
 - [net_peerCount](#net_peerCount): [ETH] Get connect peer node count
 - [eth_chainId](#eth_chainId): [ETH] Get chain id
 - [eth_protocolVersion](#eth_protocolVersion): [ETH] Get protocol version
 - [eth_syncing](#eth_syncing): [ETH] If syncing
 - [eth_coinbase](#eth_coinbase): [ETH] Get coinbase
 - [eth_mining](#eth_mining): [ETH] If mining
 - [eth_hashrate](#eth_hashrate): [ETH] Get hash rate
 - [eth_gasPrice](#eth_gasPrice): [ETH] Get gas price
 - [eth_accounts](#eth_accounts): [ETH] Get wallet accounts
 - [eth_getBalance](#eth_getBalance): [ETH] Get balance
 - [eth_blockNumber](#eth_blockNumber): [ETH] Get latest block numer
 - [eth_getStorageAt](#eth_getStorageAt): [ETH] Get storage value
 - [eth_getStorageRoot](#eth_getStorageRoot): [ETH] Get state root
 - [eth_getTransactionCount](#eth_getTransactionCount): [ETH] Get transaction count of address
 - [eth_pendingTransactions](#eth_pendingTransactions): [ETH] Get list of transaction that being sent by local accounts
 - [eth_getBlockTransactionCountByHash](#eth_getBlockTransactionCountByHash): [ETH] Get transaction quantity in the specified block by block hash
 - [eth_getBlockTransactionCountByNumber](#eth_getBlockTransactionCountByNumber): [ETH] Get transaction quantity in the specified block by block number
 - [eth_getUncleCountByBlockHash](#eth_getUncleCountByBlockHash): [ETH] Get number of uncles in the specified blockby block hash
 - [eth_getUncleCountByBlockNumber](#eth_getUncleCountByBlockNumber): [ETH] Get number of uncles in the specified blockby block number
 - [eth_getCode](#eth_getCode): [ETH] Get code
 - [eth_sign](#eth_sign): [ETH] Sign message
 - [eth_signTransaction](#eth_signTransaction): [ETH] Sign transaction
 - [eth_sendTransaction](#eth_sendTransaction): [ETH] Send transaction
 - [eth_sendRawTransaction](#eth_sendRawTransaction): [ETH] Send raw transaction
 - [eth_call](#eth_call): [ETH] Call contract
 - [eth_estimateGas](#eth_estimateGas): [ETH] Estimate gas
 - [eth_getBlockByHash](#eth_getBlockByHash): [ETH] Get block by hash
 - [eth_getBlockByNumber](#eth_getBlockByNumber): [ETH] Get block by number
 - [eth_getTransactionByHash](#eth_getTransactionByHash): [ETH] Get transaction by hash
 - [eth_getTransactionByBlockHashAndIndex](#eth_getTransactionByBlockHashAndIndex): [ETH] Get transaction by block hash and tx index
 - [eth_getTransactionByBlockNumberAndIndex](#eth_getTransactionByBlockNumberAndIndex): [ETH] Get transaction by block number and tx index
 - [eth_getTransactionReceipt](#eth_getTransactionReceipt): [ETH] Get transaction receipt
 - [eth_getUncleByBlockHashAndIndex](#eth_getUncleByBlockHashAndIndex): [ETH] Get Uncle by block hash and index
 - [eth_getUncleByBlockNumberAndIndex](#eth_getUncleByBlockNumberAndIndex): [ETH] Get Uncle by block number and index
 - [eth_newFilter](#eth_newFilter): [ETH] Create new filter
 - [eth_newBlockFilter](#eth_newBlockFilter): [ETH] Create new block filter
 - [eth_newPendingTransactionFilter](#eth_newPendingTransactionFilter): [ETH] Create new pending transaction filter
 - [eth_uninstallFilter](#eth_uninstallFilter): [ETH] Uninstall filter
 - [eth_getFilterChanges](#eth_getFilterChanges): [ETH] Get filter changes
 - [eth_getFilterLogs](#eth_getFilterLogs): [ETH] Get logs
 - [eth_getFilterBlockTx](#eth_getFilterBlockTx): [ETH] Get filter block tx
 - [eth_getLogs](#eth_getLogs): [ETH] Get logs
---

### help
**Usage:**
```
        help ("command")

List commands, or get help for a command.
```
**Arguments:**
```
 "command"                              (string, optional) command name
```
**Request:**
```
 "param" :
 {
   "command": ""                        (string, optional) command name
 }
```
**Response:**
```
 "result": "help"                       (string, required) help info
```
**Examples:**
```
>> metabasenet-cli help

>> metabasenet-cli help getpeercount
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### stop
**Usage:**
```
        stop

Stop metabasenet server.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result": "result"                     (string, required) stop result
```
**Examples:**
```
>> metabasenet-cli stop
<< metabasenet server stopping

>> curl -d '{"id":1,"method":"stop","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":"metabasenet server stopping"}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### version
**Usage:**
```
        version

Get metabasenet server version.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result": "version"                    (string, required) metabasenet version
```
**Examples:**
```
>> metabasenet-cli version
<< MetabaseNet server version is v0.1.0

>> curl -d '{"id":0,"method":"version","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":"MetabaseNet server version is v0.1.0"}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### getpeercount
**Usage:**
```
        getpeercount

Return the number of connections to other nodes.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result": count                        (int, required) peer count
```
**Examples:**
```
>> metabasenet-cli getpeercount
<< 0

>> curl -d '{"id":3,"method":"getpeercount","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":0}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### listpeer
**Usage:**
```
        listpeer

Return data about each connected network node.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result" :
   "peer":                              (array, required, default=RPCValid) 
   [
     {
       "address": "",                   (string, required) peer address
       "services": "",                  (string, required) service
       "lastsend": "",                  (string, required) last send time(utc)
       "lastrecv": "",                  (string, required) last receive time(utc)
       "conntime": "",                  (string, required) active time(utc)
       "pingtime": 0,                   (int, required) ping pong time
       "version": "",                   (string, required) version
       "subver": "",                    (string, required) sub version
       "inbound": true|false,           (bool, required) accept multiple connection or not
       "height": 0,                     (int, required) starting height
       "banscore": 0                    (int, required) ban score
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listpeer
<< [{"address":"113.105.146.22","services":"NODE_NETWORK,NODE_DELEGATED","lastsend":"2019-10-25 03:11:09","lastrecv":"2019-10-25 03:11:09","conntime":"2019-10-25 03:05:23","version":"0.1.0","subver":"/MetabaseNet:0.9.2/Protocol:0.1.0/9be9a865898c5cea90c716c17603cf3f0f185a5b","inbound":false,"height":31028,"banscore":true}]

>> curl -d '{"id":40,"method":"listpeer","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":40,"jsonrpc":"2.0","result":[{"address":"113.105.146.22","services":"NODE_NETWORK,NODE_DELEGATED","lastsend":"2019-10-25 03:11:09","lastrecv":"2019-10-25 03:11:09","conntime":"2019-10-25 03:05:23","version":"0.1.0","subver":"/MetabaseNet:0.9.2/Protocol:0.1.0/9be9a865898c5cea90c716c17603cf3f0f185a5b","inbound":false,"height":31028,"banscore":true}]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### addnode
**Usage:**
```
        addnode <"node">

Attempt to add a node into the addnode list.
```
**Arguments:**
```
 "node"                                 (string, required) node host:port
```
**Request:**
```
 "param" :
 {
   "node": ""                           (string, required) node host:port
 }
```
**Response:**
```
 "result": "result"                     (string, required) add node result
```
**Examples:**
```
>> metabasenet-cli addnode 113.105.146.22
<< Add node successfully: 113.105.146.22

>> curl -d '{"id":3,"method":"addnode","jsonrpc":"2.0","params":{"node":"113.105.146.22:8811"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"Add node successfully: 113.105.146.22:8811"}
```
**Errors:**
```
* {"code":-206,"message":"Failed to add node."}
```
##### [Back to top](#commands)
---
### removenode
**Usage:**
```
        removenode <"node">

Attempt to remove a node from the addnode list.
```
**Arguments:**
```
 "node"                                 (string, required) node host:port
```
**Request:**
```
 "param" :
 {
   "node": ""                           (string, required) node host:port
 }
```
**Response:**
```
 "result": "result"                     (string, required) remove node result
```
**Examples:**
```
>> metabasenet-cli removenode 113.105.146.22
<< Remove node successfully: 113.105.146.22

>> curl -d '{"id":67,"method":"removenode","jsonrpc":"2.0","params":{"node":"113.105.146.22:8811"}}' http://127.0.0.1:8812
<< {"id":67,"jsonrpc":"2.0","result":"Remove node successfully: 113.105.146.22:8811"}
```
**Errors:**
```
* {"code":-6,"message":"Failed to remove node."}
```
##### [Back to top](#commands)
---
### getforkport
**Usage:**
```
        getforkport <"forkid">

Get rpc listen port of fork.
```
**Arguments:**
```
 "forkid"                               (string, required) fork id or chain id
```
**Request:**
```
 "param" :
 {
   "forkid": ""                         (string, required) fork id or chain id
 }
```
**Response:**
```
 "result": port                         (uint, required) rpc listen port
```
**Examples:**
```
>> metabasenet-cli getforkport 0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55
<< 6002

>> metabasenet-cli getforkport 202
<< 6002

>> curl -d '{"id":67,"method":"getforkport","jsonrpc":"2.0","params":{"forkid":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55"}}' http://127.0.0.1:8812
<< {"id":67,"jsonrpc":"2.0","result":6002}

>> curl -d '{"id":67,"method":"getforkport","jsonrpc":"2.0","params":{"forkid":"202"}}' http://127.0.0.1:8812
<< {"id":67,"jsonrpc":"2.0","result":6002}
```
**Errors:**
```
* {"code":-2,"message":"Permission denied"}
* {"code":-206,"message":"Invalid forkid"}
```
##### [Back to top](#commands)
---
### getforkcount
**Usage:**
```
        getforkcount

Return the number of forks.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result": count                        (int, required) fork count
```
**Examples:**
```
>> metabasenet-cli getforkcount
<< 1

>> curl -d '{"id":69,"method":"getforkcount","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":69,"jsonrpc":"2.0","result":1}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### listfork
**Usage:**
```
        listfork (-a|-noa*all*)

If true, list of all forks, or subscribed forks
```
**Arguments:**
```
 -a|-noa*all*                           (bool, optional, default=false) list all forks or not
```
**Request:**
```
 "param" :
 {
   "all": true|false                    (bool, optional, default=false) list all forks or not
 }
```
**Response:**
```
 "result" :
   "profile":                           (array, required, default=RPCValid) fork profile list
   [
     {
       "fork": "",                      (string, required) fork id with hex system
       "type": "",                      (string, required) fork type
       "chainid": 0,                    (uint, required) chain id
       "name": "",                      (string, required) fork name
       "symbol": "",                    (string, required) fork symbol
       "amount": "",                    (string, required) amount (big float)
       "reward": "",                    (string, required) mint reward (big float)
       "halvecycle": 0,                 (uint, required) halve cycle: 0: fixed reward, >0: blocks of halve cycle
       "owner": "",                     (string, required) owner's address
       "createtxid": "",                (string, required) create fork txid
       "createforkheight": 0,           (int, required) create fork height
       "parentfork": "",                (string, required) parent fork
       "forkheight": 0,                 (int, required) fork height
       "lasttype": "",                  (string, required) last block type
       "lastnumber": 0,                 (uint, required) last block number
       "lastslot": 0,                   (uint, required) last block slot
       "lastblock": "",                 (string, required) last block hash
       "lastconfirm": true|false,       (bool, required) last block confirm
       "totaltxcount": 0,               (uint, required) total tx count
       "rewardtxcount": 0,              (uint, required) reward tx count
       "usertxcount": 0,                (uint, required) user tx count
       "addresscount": 0,               (uint, required) address count
       "moneysupply": "",               (string, required) money supply (big float)
       "moneydestroy": ""               (string, required) money destroy (big float)
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listfork
<< [{"fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","type":"main","chainid":201,"name":"MNTCoretest","symbol":"MNTT","amount":"200000000.0","reward":"130.0","halvecycle":0,"owner":"0x5962974eeb0b17b43edabfc9b747839317aa852f","createtxid":"0xad988352e0e560a9766444c8fb0b7277332835da0b4903c0e65dd272a617fce4","createforkheight":0,"parentfork":"0x0000000000000000000000000000000000000000000000000000000000000000","forkheight":1387,"lastnumber":1387,"lastblock":"0x0000056bcc305fb7ae6691f0fb96055ad6afa6ec8e6b19d8e8e0dee60a168d7c","totaltxcount":14117,"usertxcount":22,"moneysupply":"200164580.0","moneydestroy":"0.0"}]

>> {"id":69,"method":"listfork","jsonrpc":"2.0","params":{}}
<< {"id":69,"jsonrpc":"2.0","result":[{"fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","type":"main","chainid":201,"name":"MNTCoretest","symbol":"MNTT","amount":"200000000.0","reward":"130.0","halvecycle":0,"owner":"0x5962974eeb0b17b43edabfc9b747839317aa852f","createtxid":"0xad988352e0e560a9766444c8fb0b7277332835da0b4903c0e65dd272a617fce4","createforkheight":0,"parentfork":"0x0000000000000000000000000000000000000000000000000000000000000000","forkheight":1387,"lastnumber":1387,"lastblock":"0x0000056bcc305fb7ae6691f0fb96055ad6afa6ec8e6b19d8e8e0dee60a168d7c","totaltxcount":14117,"usertxcount":22,"moneysupply":"200164580.0","moneydestroy":"0.0"}]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### getgenealogy
**Usage:**
```
        getgenealogy (-f="fork")

Return the list of ancestry and subline.
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash
```
**Request:**
```
 "param" :
 {
   "fork": ""                           (string, optional) fork hash
 }
```
**Response:**
```
 "result" :
 {
   "ancestry":                          (array, required, default=RPCValid) ancestry info
   [
     {
       "parent": "",                    (string, required) parent fork hash
       "height": 0                      (int, required) parent origin height
     }
   ]
   "subline":                           (array, required, default=RPCValid) subline info
   [
     {
       "sub": "",                       (string, required) sub fork hash
       "height": 0                      (int, required) sub origin height
     }
   ]
 }
```
**Examples:**
```
>> metabasenet-cli getgenealogy
<< {"ancestry":[],"subline":[]}

>> curl -d '{"id":75,"method":"getgenealogy","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":75,"jsonrpc":"2.0","result":{"ancestry":[],"subline":[]}}

>> metabasenet-cli getgenealogy 1
<< {"code":-6,"message":"Unknown fork"}

>> curl -d '{"id":1,"method":"getgenealogy","jsonrpc":"2.0","params":{"fork":"1"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","error":{"code":-6,"message":"Unknown fork"}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### getblocklocation
**Usage:**
```
        getblocklocation <"block">

Return the location with given block.
```
**Arguments:**
```
 "block"                                (string, required) block hash
```
**Request:**
```
 "param" :
 {
   "block": ""                          (string, required) block hash
 }
```
**Response:**
```
 "result" :
 {
   "chainid": 0,                        (uint, required) chain id
   "fork": "",                          (string, required) fork hash
   "height": 0                          (int, required) block height
 }
```
**Examples:**
```
>> metabasenet-cli getblocklocation 0x00000001c156ff6e4f83d7a39cb234d0003c2ae96be0dbef658700d1edf6e83c
<< {"chainid":201,"fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","height":1}

>> curl -d '{"id":6,"method":"getblocklocation","jsonrpc":"2.0","params":{"block":"0x00000001c156ff6e4f83d7a39cb234d0003c2ae96be0dbef658700d1edf6e83c"}}' http://127.0.0.1:8812
<< {"id":6,"jsonrpc":"2.0","result":{"chainid":201,"fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","height":1}}
```
**Errors:**
```
* {"code":-6,"message":"Unknown block."}
```
##### [Back to top](#commands)
---
### getblockcount
**Usage:**
```
        getblockcount (-f="fork")

Return the number of blocks in the given fork, includes extended block and vacant block
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash
```
**Request:**
```
 "param" :
 {
   "fork": ""                           (string, optional) fork hash
 }
```
**Response:**
```
 "result": count                        (int, required) block count
```
**Examples:**
```
>> metabasenet-cli getblockcount
<< 32081

>> curl -d '{"id":4,"method":"getblockcount","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":4,"jsonrpc":"2.0","result":32081}

>> metabasenet-cli getblockcount -f=0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55
<< 32081

>> curl -d '{"id":5,"method":"getblockcount","jsonrpc":"2.0","params":{"fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55"}}' http://127.0.0.1:8812
<< {"id":5,"jsonrpc":"2.0","result":32081}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### getblockhash
**Usage:**
```
        getblockhash <height> (-f="fork")

Return a list of block hash in fork at specific height.
First of the list is the main block hash, others are extended blocks hash in order.
```
**Arguments:**
```
 height                                 (int, required) block height
 -f="fork"                              (string, optional) fork hash
```
**Request:**
```
 "param" :
 {
   "height": 0,                         (int, required) block height
   "fork": ""                           (string, optional) fork hash
 }
```
**Response:**
```
 "result" :
   "hash":                              (array, required, default=RPCValid) 
   [
     "hash": ""                         (string, required) one block hash
   ]
```
**Examples:**
```
>> metabasenet-cli getblockhash 0
<< ["0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55"]

>> curl -d '{"id":37,"method":"getblockhash","jsonrpc":"2.0","params":{"height":0}}' http://127.0.0.1:8812
<< {"id":37,"jsonrpc":"2.0","result":["0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55"]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Block number out of range."}
```
##### [Back to top](#commands)
---
### getblocknumberhash
**Usage:**
```
        getblocknumberhash <number> (-f="fork")

Return a block hash in fork at specific number.
```
**Arguments:**
```
 number                                 (uint, required) block number
 -f="fork"                              (string, optional) fork hash
```
**Request:**
```
 "param" :
 {
   "number": 0,                         (uint, required) block number
   "fork": ""                           (string, optional) fork hash
 }
```
**Response:**
```
 "result": "blockhash"                  (string, required) block hash
```
**Examples:**
```
>> metabasenet-cli getblocknumberhash 0
<< ["0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55"]

>> curl -d '{"id":37,"method":"getblocknumberhash","jsonrpc":"2.0","params":{"number":0}}' http://127.0.0.1:8812
<< {"id":37,"jsonrpc":"2.0","result":["0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55"]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Block number out of range."}
```
##### [Back to top](#commands)
---
### getblock
**Usage:**
```
        getblock <"block">

Return details of a block with given block-hash.
```
**Arguments:**
```
 "block"                                (string, required) block hash
```
**Request:**
```
 "param" :
 {
   "block": ""                          (string, required) block hash
 }
```
**Response:**
```
 "result" :
 {
   "hash": "",                          (string, required) block hash
   "prev": "",                          (string, required) previous block hash
   "chainid": 0,                        (uint, required) chain id
   "fork": "",                          (string, required) fork hash
   "version": 0,                        (uint, required) version
   "type": "",                          (string, required) block type
   "time": 0,                           (uint, required) block time
   "number": 0,                         (uint, required) block number
   "height": 0,                         (uint, required) block height
   "slot": 0,                           (uint, required) block slot
   "confirm": true|false,               (bool, required) if confirm block
   "prevconfirmblock": "",              (string, required) prev confirm block
   "reward": "",                        (string, required) block reward
   "stateroot": "",                     (string, required) state root
   "receiptsroot": "",                  (string, required) receipts root
   "bloom": "",                         (string, required) block bloom
   "txmint": "",                        (string, required) transaction mint hash
   "tx":                                (array, required, default=RPCValid) transaction hash list
   [
     "tx": ""                           (string, required) transaction hash
   ]
 }
```
**Examples:**
```
>> metabasenet-cli getblock 0xc938f722d704ce2585f114c0005743e69d25b0f83a99d79705fb303c95e3c21b
<< {"hash":"0xc938f722d704ce2585f114c0005743e69d25b0f83a99d79705fb303c95e3c21b","prev":"0xc937f77ee553b2b7b7fd50cf303a9077a1e71fc4d6c1e0336b2dd93cf1c5cbc3","chainid":201,"fork":"0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d","version":1,"type":"primary-stake","time":1691292094,"number":56,"height":56,"slot":0,"reward":"130.0","stateroot":"0xbedbcf8bd065c04d040a1873e310b5a576c4a1ba97ef36ad73b5d86aa06a0da3","receiptsroot":"0xf780fed5098460ecc4333c4deaa5abada70aa14479301462432f9567c7b43607","bloom":"0xfbe36bbadd36f7ce77ffe15befeff8f3db7edfef6f","txmint":"0xe5f3246178169493a351b29cb76064baf25980b12ea3746d5d78f73f14fa21fb","tx":["0x5a666daaa87477d923b890efd15952aa23004878c10aa63370245b72ecfc2bb2","0x1b777e66ea46754a49ebb2a606caab38672f965feb38e3a802d7356096ee19d2"]}

>> curl -d '{"id":10,"method":"getblock","jsonrpc":"2.0","params":{"block":"0xc938f722d704ce2585f114c0005743e69d25b0f83a99d79705fb303c95e3c21b"}}' http://127.0.0.1:8812
<< {"id":10,"jsonrpc":"2.0","result":{"hash":"0xc938f722d704ce2585f114c0005743e69d25b0f83a99d79705fb303c95e3c21b","prev":"0xc937f77ee553b2b7b7fd50cf303a9077a1e71fc4d6c1e0336b2dd93cf1c5cbc3","chainid":201,"fork":"0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d","version":1,"type":"primary-stake","time":1691292094,"number":56,"height":56,"slot":0,"reward":"130.0","stateroot":"0xbedbcf8bd065c04d040a1873e310b5a576c4a1ba97ef36ad73b5d86aa06a0da3","receiptsroot":"0xf780fed5098460ecc4333c4deaa5abada70aa14479301462432f9567c7b43607","bloom":"0xfbe36bbadd36f7ce77ffe15befeff8f3db7edfef6f","txmint":"0xe5f3246178169493a351b29cb76064baf25980b12ea3746d5d78f73f14fa21fb","tx":["0x5a666daaa87477d923b890efd15952aa23004878c10aa63370245b72ecfc2bb2","0x1b777e66ea46754a49ebb2a606caab38672f965feb38e3a802d7356096ee19d2"]}}
```
**Errors:**
```
* {"code":-6,"message":"Unknown block."}
```
##### [Back to top](#commands)
---
### getblockdetail
**Usage:**
```
        getblockdetail <"block">

Return details of a block with given block-hash.
```
**Arguments:**
```
 "block"                                (string, required) block hash
```
**Request:**
```
 "param" :
 {
   "block": ""                          (string, required) block hash
 }
```
**Response:**
```
 "result" :
 {
   "hash": "",                          (string, required) block hash
   "prev": "",                          (string, required) previous block hash
   "chainid": 0,                        (uint, required) chain id
   "fork": "",                          (string, required) fork hash
   "version": 0,                        (uint, required) version
   "type": "",                          (string, required) block type
   "time": 0,                           (uint, required) block time
   "number": 0,                         (uint, required) block number
   "height": 0,                         (uint, required) block height
   "slot": 0,                           (uint, required) block slot
   "confirm": true|false,               (bool, required) if confirm block
   "reward": "",                        (string, required) block reward
   "stateroot": "",                     (string, required) state root
   "receiptsroot": "",                  (string, required) receipts root
   "bloom": "",                         (string, required) block bloom
   "txmint":                            (object, required) transaction mint data
   {
     "txid": "",                        (string, optional) transaction hash
     "type": "",                        (string, optional) transaction type
     "nonce": 0,                        (uint, optional) nonce
     "from": "",                        (string, optional) from address
     "to": "",                          (string, optional) to address
     "amount": "",                      (string, optional) amount (big float)
     "gaslimit": 0,                     (uint, optional) gas limit
     "gasprice": "",                    (string, optional) gas price (big float)
     "txfee": "",                       (string, optional) transaction limit fee (big float)
     "data": "",                        (string, optional) data
     "signhash": "",                    (string, optional) sign hash
     "sig": "",                         (string, optional) sign
     "fork": "",                        (string, optional) fork hash
     "height": 0,                       (int, optional) height
     "blockhash": "",                   (string, optional) which block tx located in
     "confirmations": 0                 (int, optional) confirmations
   }
   "tx":                                (array, required, default=RPCValid) transaction hash list
   [
     {
       "txid": "",                      (string, optional) transaction hash
       "type": "",                      (string, optional) transaction type
       "nonce": 0,                      (uint, optional) nonce
       "from": "",                      (string, optional) from address
       "to": "",                        (string, optional) to address
       "amount": "",                    (string, optional) amount (big float)
       "gaslimit": 0,                   (uint, optional) gas limit
       "gasprice": "",                  (string, optional) gas price (big float)
       "txfee": "",                     (string, optional) transaction limit fee (big float)
       "data": "",                      (string, optional) data
       "signhash": "",                  (string, optional) sign hash
       "sig": "",                       (string, optional) sign
       "fork": "",                      (string, optional) fork hash
       "height": 0,                     (int, optional) height
       "blockhash": "",                 (string, optional) which block tx located in
       "confirmations": 0               (int, optional) confirmations
     }
   ]
 }
```
**Examples:**
```
>> metabasenet-cli getblockdetail 0xc938f722d704ce2585f114c0005743e69d25b0f83a99d79705fb303c95e3c21b
<< {"hash":"0xc938f722d704ce2585f114c0005743e69d25b0f83a99d79705fb303c95e3c21b","version":1,"type":"genesis","time":1690251993,"number":0,"stateroot":"0x81d35840a6e36f5ed343a59ebb0f2be312e2e4d9b08df0c769522039d9c3a083","receiptsroot":"0x","bloom":"0x","bits":0,"prev":"0x0000000000000000000000000000000000000000000000000000000000000000","chainid":201,"fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","height":0,"txmint":{"txid":"0xad988352e0e560a9766444c8fb0b7277332835da0b4903c0e65dd272a617fce4","type":"genesis","nonce":0,"from":"0x0000000000000000000000000000000000000000","to":"0x5962974eeb0b17b43edabfc9b747839317aa852f","amount":"200000000.0","gaslimit":0,"gasprice":"0.0","txfee":"0.0","data":"0x02110220001520000000000000000000000000000000000000000000a56fa5b99019a5c8000000","signhash":"0x74a1abd80618b0dfc476071687c6980673d01fca729748918ffaf703bc25a9b8","sig":"","fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","height":0,"blockhash":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","confirmations":1811},"tx":[]}

>> curl -d '{"id":10,"method":"getblockdetail","jsonrpc":"2.0","params":{"block":"0xc938f722d704ce2585f114c0005743e69d25b0f83a99d79705fb303c95e3c21b"}}' http://127.0.0.1:8812
<< {"id":10,"jsonrpc":"2.0","result":{"hash":"0xc938f722d704ce2585f114c0005743e69d25b0f83a99d79705fb303c95e3c21b","version":1,"type":"genesis","time":1690251993,"number":0,"stateroot":"0x81d35840a6e36f5ed343a59ebb0f2be312e2e4d9b08df0c769522039d9c3a083","receiptsroot":"0x","bloom":"0x","bits":0,"prev":"0x0000000000000000000000000000000000000000000000000000000000000000","chainid":201,"fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","height":0,"txmint":{"txid":"0xad988352e0e560a9766444c8fb0b7277332835da0b4903c0e65dd272a617fce4","type":"genesis","nonce":0,"from":"0x0000000000000000000000000000000000000000","to":"0x5962974eeb0b17b43edabfc9b747839317aa852f","amount":"200000000.0","gaslimit":0,"gasprice":"0.0","txfee":"0.0","data":"0x02110220001520000000000000000000000000000000000000000000a56fa5b99019a5c8000000","signhash":"0x74a1abd80618b0dfc476071687c6980673d01fca729748918ffaf703bc25a9b8","sig":"","fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","height":0,"blockhash":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","confirmations":1811},"tx":[]}}
```
**Errors:**
```
* {"code":-6,"message":"Unknown block."}
* {"code":-32700,"message":"Load proof fail."}
```
##### [Back to top](#commands)
---
### getblockdata
**Usage:**
```
        getblockdata (-b="blockhash") (-h=height) (-s=slot) (-n=number) (-f="fork")

Return details of a block with given block hash or height or number.
```
**Arguments:**
```
 -b="blockhash"                         (string, optional) block hash
 -h=height                              (uint, optional) block height
 -s=slot                                (uint, optional) block slot
 -n=number                              (uint, optional) block number
 -f="fork"                              (string, optional) fork hash
```
**Request:**
```
 "param" :
 {
   "blockhash": "",                     (string, optional) block hash
   "height": 0,                         (uint, optional) block height
   "slot": 0,                           (uint, optional) block slot
   "number": 0,                         (uint, optional) block number
   "fork": ""                           (string, optional) fork hash
 }
```
**Response:**
```
 "result" :
 {
   "hash": "",                          (string, required) block hash
   "prev": "",                          (string, required) previous block hash
   "chainid": 0,                        (uint, required) chain id
   "fork": "",                          (string, required) fork hash
   "version": 0,                        (uint, required) version
   "type": "",                          (string, required) block type
   "time": 0,                           (uint, required) block time
   "number": 0,                         (uint, required) block number
   "height": 0,                         (uint, required) block height
   "slot": 0,                           (uint, required) block slot
   "confirm": true|false,               (bool, required) if confirm block
   "prevconfirmblock": "",              (string, required) prev confirm block
   "reward": "",                        (string, required) block reward
   "stateroot": "",                     (string, required) state root
   "receiptsroot": "",                  (string, required) receipts root
   "bloom": "",                         (string, required) block bloom
   "txmint": "",                        (string, required) transaction mint hash
   "tx":                                (array, required, default=RPCValid) transaction hash list
   [
     "tx": ""                           (string, required) transaction hash
   ]
 }
```
**Examples:**
```
>> metabasenet-cli getblockdata -b=0xc903f7c5e9e1c5ec990b02358e7f60f958e6479d5042d6be27a88456c83924bc
<< {"hash":"0xc903f7c5e9e1c5ec990b02358e7f60f958e6479d5042d6be27a88456c83924bc","prev":"0xc902f73972f2aab9541f844b55a421bdd34167f7f66906dc16be76c5149874ac","chainid":201,"fork":"0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d","version":1,"type":"primary-poa","time":1691291829,"number":3,"height":3,"slot":0,"reward":"0.0","stateroot":"0x1c3ffbbf503d90059c5714e9a5870afc13c6da6b459f9dab4a2aaf3907dd5b45","receiptsroot":"0x","bloom":"0x04029200","txmint":{"txid":"0xc0b87f4b71d493d95d64d954a90801ec6f58c34e6506795d461ccd29294bd7bb","type":"poa","nonce":3,"from":"0x0000000000000000000000000000000000000000","to":"0xb35400ae1477971c22e51311c12edc87ce3cf79d","amount":"0.0","gaslimit":0,"gasprice":"0.0","txfee":"0.0","data":"","signhash":"","sig":"","fork":"0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d","height":3,"blockhash":"0xc903f7c5e9e1c5ec990b02358e7f60f958e6479d5042d6be27a88456c83924bc","confirmations":362},"tx":[]}

>> curl -d '{"id":10,"method":"getblockdata","jsonrpc":"2.0","params":{"blockhash":"0xc903f7c5e9e1c5ec990b02358e7f60f958e6479d5042d6be27a88456c83924bc"}}' http://127.0.0.1:8812
<< {"id":10,"jsonrpc":"2.0","result":{"hash":"0xc903f7c5e9e1c5ec990b02358e7f60f958e6479d5042d6be27a88456c83924bc","prev":"0xc902f73972f2aab9541f844b55a421bdd34167f7f66906dc16be76c5149874ac","chainid":201,"fork":"0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d","version":1,"type":"primary-poa","time":1691291829,"number":3,"height":3,"slot":0,"reward":"0.0","stateroot":"0x1c3ffbbf503d90059c5714e9a5870afc13c6da6b459f9dab4a2aaf3907dd5b45","receiptsroot":"0x","bloom":"0x04029200","txmint":{"txid":"0xc0b87f4b71d493d95d64d954a90801ec6f58c34e6506795d461ccd29294bd7bb","type":"poa","nonce":3,"from":"0x0000000000000000000000000000000000000000","to":"0xb35400ae1477971c22e51311c12edc87ce3cf79d","amount":"0.0","gaslimit":0,"gasprice":"0.0","txfee":"0.0","data":"","signhash":"","sig":"","fork":"0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d","height":3,"blockhash":"0xc903f7c5e9e1c5ec990b02358e7f60f958e6479d5042d6be27a88456c83924bc","confirmations":362},"tx":[]}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid block"}
* {"code":-6,"message":"Invalid height or slot"}
* {"code":-6,"message":"Invalid number"}
* {"code":-6,"message":"Unknown block"}
```
##### [Back to top](#commands)
---
### gettxpool
**Usage:**
```
        gettxpool (-f="fork") (-a="address") (-d|-nod*detail*) (-o=getoffset) (-n=getcount)

If detail==0, return the count and total size of txs for given fork.
Otherwise,return all transaction ids and sizes in memory pool for given fork.
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash
 -a="address"                           (string, optional) address
 -d|-nod*detail*                        (bool, optional, default=false) get detail or not
 -o=getoffset                           (int, optional, default=0) get offset, If not set, from 0
 -n=getcount                            (int, optional, default=20) get count, 0 is all
```
**Request:**
```
 "param" :
 {
   "fork": "",                          (string, optional) fork hash
   "address": "",                       (string, optional) address
   "detail": true|false,                (bool, optional, default=false) get detail or not
   "getoffset": 0,                      (int, optional, default=0) get offset, If not set, from 0
   "getcount": 0                        (int, optional, default=20) get count, 0 is all
 }
```
**Response:**
```
 "result" :
 {
   (if detail=false)
   "count": 0,                          (uint, optional) transaction pool count
   (if detail=false)
   "size": 0,                           (uint, optional) transaction total size
   (if detail=true)
   "list":                              (array, optional) transaction pool list
   [
     {
       "txid": "",                      (string, required) txid
       "txtype": "",                    (string, required) tx type
       "nonce": 0,                      (uint, required) nonce
       "from": "",                      (string, required) from address
       "to": "",                        (string, required) to address
       "amount": "",                    (string, required) amount (big float)
       "gasprice": "",                  (string, required) gas price (big float)
       "gas": 0,                        (uint, required) gas
       "txfee": "",                     (string, required) tx fee (big float)
       "size": 0                        (uint, required) tx pool size
     }
   ]
 }
```
**Examples:**
```
>> metabasenet-cli gettxpool
<< {"count":0,"size":0}

>> curl -d '{"id":11,"method":"gettxpool","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":11,"jsonrpc":"2.0","result":{"count":0,"size":0}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid address"}
```
##### [Back to top](#commands)
---
### gettransaction
**Usage:**
```
        gettransaction <"txid"> (-f="fork") (-s|-nos*serialized*)

Get transaction information
```
**Arguments:**
```
 "txid"                                 (string, required) transaction hash
 -f="fork"                              (string, optional) fork hash or chainid (default local fork)
 -s|-nos*serialized*                    (bool, optional, default=false) If serialized=0, return an Object with information about <txid>.
                                        If serialized is non-zero, return a string that is
                                        serialized, hex-encoded data for <txid>.
```
**Request:**
```
 "param" :
 {
   "txid": "",                          (string, required) transaction hash
   "fork": "",                          (string, optional) fork hash or chainid (default local fork)
   "serialized": true|false             (bool, optional, default=false) If serialized=0, return an Object with information about <txid>.
                                        If serialized is non-zero, return a string that is
                                        serialized, hex-encoded data for <txid>.
 }
```
**Response:**
```
 "result" :
 {
   (if serialized=true)
   "serialization": "",                 (string, optional) transaction hex data
   (if serialized=false)
   "transaction":                       (object, optional) transaction data
   {
     "txid": "",                        (string, optional) transaction hash
     "type": "",                        (string, optional) transaction type
     "nonce": 0,                        (uint, optional) nonce
     "from": "",                        (string, optional) from address
     "to": "",                          (string, optional) to address
     "amount": "",                      (string, optional) amount (big float)
     "gaslimit": 0,                     (uint, optional) gas limit
     "gasprice": "",                    (string, optional) gas price (big float)
     "txfee": "",                       (string, optional) transaction limit fee (big float)
     "data": "",                        (string, optional) data
     "signhash": "",                    (string, optional) sign hash
     "sig": "",                         (string, optional) sign
     "fork": "",                        (string, optional) fork hash
     "height": 0,                       (int, optional) height
     "blockhash": "",                   (string, optional) which block tx located in
     "confirmations": 0                 (int, optional) confirmations
   }
 }
```
**Examples:**
```
>> metabasenet-cli gettransaction 0x8198d41372a78d304eea88ce526a4f3584631204f43871716c7fc591b50304bb
<< {"transaction":{"txid":"0x8198d41372a78d304eea88ce526a4f3584631204f43871716c7fc591b50304bb","type":"token","nonce":21,"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0xeba52feecb5dc5b97a259193d1e9013f22f5aa65","amount":"2000000.0","gaslimit":23600,"gasprice":"0.000001","txfee":"0.0236","data":"0x011131432f0000032b03cea94079fd11c296b819d52c7a7efab7c42c0922e85adab334651e31291973cde1ca25dfd6d0236201f4","signhash":"0x6327fef5b92c703b0187d0282bb589b58b43cce5b5b6732d10f332e8a2e3d515","sig":"0xb8d4ad2c3f45d70a0b5040fc31cc967974a2347e23055555a9a46997296011bc64a1830954bf5dd185744f7e90cca3a08bd65561115da564bed36ee7fd25b11c00","fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","height":102,"blockhash":"0x000000668a3966460f71292dbb262d55fa086ed98b02461ed8eca55853609ceb","confirmations":1818}}

>> curl -d '{"id":13,"method":"gettransaction","jsonrpc":"2.0","params":{"txid":"0x8198d41372a78d304eea88ce526a4f3584631204f43871716c7fc591b50304bb","serialized":false}}' http://127.0.0.1:8812
<< {"id":13,"jsonrpc":"2.0","result":{"transaction":{"txid":"0x8198d41372a78d304eea88ce526a4f3584631204f43871716c7fc591b50304bb","type":"token","nonce":21,"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0xeba52feecb5dc5b97a259193d1e9013f22f5aa65","amount":"2000000.0","gaslimit":23600,"gasprice":"0.000001","txfee":"0.0236","data":"0x011131432f0000032b03cea94079fd11c296b819d52c7a7efab7c42c0922e85adab334651e31291973cde1ca25dfd6d0236201f4","signhash":"0x6327fef5b92c703b0187d0282bb589b58b43cce5b5b6732d10f332e8a2e3d515","sig":"0xb8d4ad2c3f45d70a0b5040fc31cc967974a2347e23055555a9a46997296011bc64a1830954bf5dd185744f7e90cca3a08bd65561115da564bed36ee7fd25b11c00","fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","height":102,"blockhash":"0x000000668a3966460f71292dbb262d55fa086ed98b02461ed8eca55853609ceb","confirmations":1818}}}

>> metabasenet-cli gettransaction 0x8198d41372a78d304eea88ce526a4f3584631204f43871716c7fc591b50304bb -s
<< {"serialization":"0x02c915142f85aa17938347b7c9bfda3eb4170beb4e9762591465aaf5223f01e9d19391257ab9c55dcbee2fa5eb0b01a784379d99db4200000005e8d4a51000025c30011131432f0000032b03cea94079fd11c296b819d52c7a7efab7c42c0922e85adab334651e31291973cde1ca25dfd6d0236201f441b8d4ad2c3f45d70a0b5040fc31cc967974a2347e23055555a9a46997296011bc64a1830954bf5dd185744f7e90cca3a08bd65561115da564bed36ee7fd25b11c00","transaction":{}}

>> curl -d '{"id":13,"method":"gettransaction","jsonrpc":"2.0","params":{"txid":"0x8198d41372a78d304eea88ce526a4f3584631204f43871716c7fc591b50304bb","serialized":true}}' http://127.0.0.1:8812
<< {"id":13,"jsonrpc":"2.0","result":{"serialization":"0x02c915142f85aa17938347b7c9bfda3eb4170beb4e9762591465aaf5223f01e9d19391257ab9c55dcbee2fa5eb0b01a784379d99db4200000005e8d4a51000025c30011131432f0000032b03cea94079fd11c296b819d52c7a7efab7c42c0922e85adab334651e31291973cde1ca25dfd6d0236201f441b8d4ad2c3f45d70a0b5040fc31cc967974a2347e23055555a9a46997296011bc64a1830954bf5dd185744f7e90cca3a08bd65561115da564bed36ee7fd25b11c00","transaction":{}}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid txid."}
* {"code":-6,"message":"Invalid fork."}
* {"code":-6,"message":"Unknown fork."}
* {"code":-32600,"message":"No information available about transaction."}
```
##### [Back to top](#commands)
---
### sendtransaction
**Usage:**
```
        sendtransaction <"txdata">

Submit raw transaction (serialized, hex-encoded) to local node and network.
```
**Arguments:**
```
 "txdata"                               (string, required) transaction binary data
```
**Request:**
```
 "param" :
 {
   "txdata": ""                         (string, required) transaction binary data
 }
```
**Response:**
```
 "result": "data"                       (string, required) transaction raw data
```
**Examples:**
```
>> metabasenet-cli sendtransaction 0x02c918142f85aa17938347b7c9bfda3eb4170beb4e976259142bab3b545e9a35fbf7df91474aadee4d95fc3c7d088ac7230489e8000005e8d4a51000025208004150b186ec91f439696626447f0f379f146665099f2269e84fde9f96802c3410d110716aaf722b8009b8b3585be64163cc21a8f22f404f3c1ccf2af3fec03bf2ea01
<< 0x01fd16a69bc8607d283e505225013f085d438621d5bd62b3c2ac4b2b37907403

>> curl -d '{"id":9,"method":"sendtransaction","jsonrpc":"2.0","params":{"txdata":"0x02c918142f85aa17938347b7c9bfda3eb4170beb4e976259142bab3b545e9a35fbf7df91474aadee4d95fc3c7d088ac7230489e8000005e8d4a51000025208004150b186ec91f439696626447f0f379f146665099f2269e84fde9f96802c3410d110716aaf722b8009b8b3585be64163cc21a8f22f404f3c1ccf2af3fec03bf2ea01"}}' http://127.0.0.1:8812
<< {"id":9,"jsonrpc":"2.0","result":"0x01fd16a69bc8607d283e505225013f085d438621d5bd62b3c2ac4b2b37907403"}
```
**Errors:**
```
* {"code":-8,"message":"TX decode failed"}
* {"code":-9,"message":"Get fork hash failed"}
* {"code":-10,"message":"Tx rejected : xxx"}
```
##### [Back to top](#commands)
---
### getforkheight
**Usage:**
```
        getforkheight (-f="fork")

Return the number of height in the given fork.
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash
```
**Request:**
```
 "param" :
 {
   "fork": ""                           (string, optional) fork hash
 }
```
**Response:**
```
 "result": height                       (int, required) fork height
```
**Examples:**
```
>> metabasenet-cli getforkheight
<< 32081

>> metabasenet-cli getforkheight -f=0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d
<< 32081

>> curl -d '{"id":4,"method":"getforkheight","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":4,"jsonrpc":"2.0","result":32081}

>> curl -d '{"id":4,"method":"getforkheight","jsonrpc":"2.0","params":{"fork":"0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d"}}' http://127.0.0.1:8812
<< {"id":4,"jsonrpc":"2.0","result":32081}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### getvotes
**Usage:**
```
        getvotes <"address"> (-b="block")

Get votes
```
**Arguments:**
```
 "address"                              (string, required) delegate template address or vote template address
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) delegate template address or vote template address
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result": "votes"                      (string, required) number of votes (big float)
```
**Examples:**
```
>> metabasenet-cli getvotes 0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d
<< 70000000.000000

>> curl -d '{"id":1,"method":"getvotes","jsonrpc":"2.0","params":{"address":"0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":70000000.000000}
```
**Errors:**
```
* {"code" : -6, "message" : "Invalid address"}
* {"code" : -6, "message" : "Invalid block"}
```
##### [Back to top](#commands)
---
### listdelegate
**Usage:**
```
        listdelegate (-n=count) (-b="block")

List delegate
```
**Arguments:**
```
 -n=count                               (uint, optional, default=0) list count, default 0 is all
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "count": 0,                          (uint, optional, default=0) list count, default 0 is all
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result" :
   "delegate":                          (array, required, default=RPCValid) 
   [
     {
       "address": "",                   (string, required) delegate address
       "votes": ""                      (string, required) number of votes (big float)
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listdelegate
<< {"address":"0xd2b670ea0bb5bac8cdaac13772dcfec6fadeb0ee","votes":100002000.000000}

>> curl -d '{"id":1,"method":"listdelegate","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":"{"address":"0xd2b670ea0bb5bac8cdaac13772dcfec6fadeb0ee","votes":100002000.000000}"}
```
**Errors:**
```
* {"code" : -6, "message" : "Invalid block"}
* {"code" : -32603, "message" : "Query failed"}
```
##### [Back to top](#commands)
---
### getdelegatevotes
**Usage:**
```
        getdelegatevotes <"address"> (-b="block")

Get delegate total votes
```
**Arguments:**
```
 "address"                              (string, required) delegate template address
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) delegate template address
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result": "votes"                      (string, required) number of votes (big float)
```
**Examples:**
```
>> metabasenet-cli getdelegatevotes 0xd2b670ea0bb5bac8cdaac13772dcfec6fadeb0ee
<< 70000000.000000

>> curl -d '{"id":1,"method":"getdelegatevotes","jsonrpc":"2.0","params":{"address":"0xd2b670ea0bb5bac8cdaac13772dcfec6fadeb0ee"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":70000000.000000}
```
**Errors:**
```
* {"code" : -6, "message" : "Invalid address"}
* {"code" : -6, "message" : "Invalid block"}
* {"code" : -32603, "message" : "Query failed"}
```
##### [Back to top](#commands)
---
### getuservotes
**Usage:**
```
        getuservotes <"address"> (-b="block")

Get user votes
```
**Arguments:**
```
 "address"                              (string, required) delegate or vote or pledge template address
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) delegate or vote or pledge template address
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result" :
 {
   "type": "",                          (string, required) vote type
   "votes": "",                         (string, required) number of votes (big float)
   "unlockheight": 0                    (uint, required) unlock height
 }
```
**Examples:**
```
>> metabasenet-cli getuservotes 0x6e4089b464e08441a27804d8e78f7d691e2ef736
<< 70000000.000000

>> curl -d '{"id":1,"method":"getuservotes","jsonrpc":"2.0","params":{"address":"0x6e4089b464e08441a27804d8e78f7d691e2ef736"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":70000000.000000}
```
**Errors:**
```
* {"code" : -6, "message" : "Invalid address"}
* {"code" : -6, "message" : "Invalid block"}
* {"code" : -32603, "message" : "Query failed"}
```
##### [Back to top](#commands)
---
### getaddresscount
**Usage:**
```
        getaddresscount (-f="fork") (-b="block")

Get address count
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "fork": "",                          (string, optional) fork hash
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result" :
 {
   "block": "",                         (string, required) reference block hash
   "addresscount": 0,                   (uint, required) address count
   "newaddresscount": 0                 (uint, required) new address count
 }
```
**Examples:**
```
>> metabasenet-cli getaddresscount -b=0xc921f7f807e072f0acc0e2e76d5413b64f4840e839758a93274941fe11144344
<< {"block" : "0xc921f7f807e072f0acc0e2e76d5413b64f4840e839758a93274941fe11144344", "addresscount" : 12, "newaddresscount" : 2}

>> curl -d '{"id":1,"method":"getaddresscount","jsonrpc":"2.0","params":{"block":"0xc921f7f807e072f0acc0e2e76d5413b64f4840e839758a93274941fe11144344"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":{"block" : "0xc921f7f807e072f0acc0e2e76d5413b64f4840e839758a93274941fe11144344", "addresscount" : 12, "newaddresscount" : 2}}
```
**Errors:**
```
* {"code" : -6, "message" : "Invalid fork"}
* {"code" : -6, "message" : "Unknown fork"}
* {"code" : -6, "message" : "Invalid block"}
* {"code" : -6, "message" : "Block status error"}
* {"code" : -4, "message" : "Get address count fail"}
```
##### [Back to top](#commands)
---
### listkey
**Usage:**
```
        listkey (-p=page) (-n=count)

Return Object that has pubkey as keys, associated status as values.
```
**Arguments:**
```
 -p=page                                (uint, optional, default=0) page, default is 0
 -n=count                               (uint, optional, default=30) count, default is 30
```
**Request:**
```
 "param" :
 {
   "page": 0,                           (uint, optional, default=0) page, default is 0
   "count": 0                           (uint, optional, default=30) count, default is 30
 }
```
**Response:**
```
 "result" :
   "pubkey":                            (array, required, default=RPCValid) public key list
   [
     {
       "pubkey": "",                    (string, required) public key with hex system
       "address": "",                   (string, required) address
       "version": 0,                    (int, required) public key version
       "public": true|false,            (bool, required) is only public key or not in wallet
       "locked": true|false,            (bool, required) public key locked
       "timeout": 0                     (int, optional) public key timeout locked
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listkey
<< [{"pubkey":"0x63a4f21f10fbabfd07076ea67d84bf0785d906bad245b2d9a0d8fc0512ee3dbcb9b395d7a56589356bf1ce6a0fab53f476493471a22fd7649363a192117ab01d","address":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","version":1,"public":false,"locked":true},{"pubkey":"0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765","address":"0x5962974eeb0b17b43edabfc9b747839317aa852f","version":1,"public":false,"locked":false}]

>> curl -d '{"id":43,"method":"listkey","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":43,"jsonrpc":"2.0","result":[{"pubkey":"0x63a4f21f10fbabfd07076ea67d84bf0785d906bad245b2d9a0d8fc0512ee3dbcb9b395d7a56589356bf1ce6a0fab53f476493471a22fd7649363a192117ab01d","address":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","version":1,"public":false,"locked":true},{"pubkey":"0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765","address":"0x5962974eeb0b17b43edabfc9b747839317aa852f","version":1,"public":false,"locked":false}]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### getnewkey
**Usage:**
```
        getnewkey <"passphrase">

Return a new pubkey for receiving payments.
```
**Arguments:**
```
 "passphrase"                           (string, required) passphrase
```
**Request:**
```
 "param" :
 {
   "passphrase": ""                     (string, required) passphrase
 }
```
**Response:**
```
 "result" :
 {
   "pubkey": "",                        (string, required) public key
   "address": ""                        (string, required) address
 }
```
**Examples:**
```
>> metabasenet-cli getnewkey 123
<< {"pubkey":"0xeed002394906cad1a438415c8d7ef0f3799fec195b25fdaaf79859c08ccf36c9885e3ae94658b3f109542fd79472ee0415c919f3b6f5ecf029636640a56dbbc2","address":"0xf90354b9b4937cca9ac0f675a8c228100f935971"}

>> curl -d '{"id":7,"method":"getnewkey","jsonrpc":"2.0","params":{"passphrase":"123"}}' http://127.0.0.1:8812
<< {"id":7,"jsonrpc":"2.0","result":{"pubkey":"0xeed002394906cad1a438415c8d7ef0f3799fec195b25fdaaf79859c08ccf36c9885e3ae94658b3f109542fd79472ee0415c919f3b6f5ecf029636640a56dbbc2","address":"0xf90354b9b4937cca9ac0f675a8c228100f935971"}}
```
**Errors:**
```
* {"code":-6,"message":"Passphrase must be nonempty"}
* {"code":-401,"message":"Failed add new key."}
```
##### [Back to top](#commands)
---
### removekey
**Usage:**
```
        removekey <"pubkey"> ("passphrase")

Remove Key in Wallet.
```
**Arguments:**
```
 "pubkey"                               (string, required) public key or pubkey address
 "passphrase"                           (string, optional) passphrase
```
**Request:**
```
 "param" :
 {
   "pubkey": "",                        (string, required) public key or pubkey address
   "passphrase": ""                     (string, optional) passphrase
 }
```
**Response:**
```
 "result": "pubkey"                     (string, required) public key
```
**Examples:**
```
>> metabasenet-cli removekey 0xf90354b9b4937cca9ac0f675a8c228100f935971 123
<< Remove key successfully: 0xf90354b9b4937cca9ac0f675a8c228100f935971

>> curl -d '{"id":7,"method":"removekey","jsonrpc":"2.0","params":{"pubkey":"0xf90354b9b4937cca9ac0f675a8c228100f935971","passphrase":"123"}}' http://127.0.0.1:8812
<< {"id":7,"jsonrpc":"2.0","result":"Remove key successfully: 0xf90354b9b4937cca9ac0f675a8c228100f935971"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
* {"code":-4,"message":"Unknown key"}
* {"code":-406,"message":"Can't remove key with incorrect passphrase."}
* {"code":-401,"message":"Failed remove key."}
```
##### [Back to top](#commands)
---
### encryptkey
**Usage:**
```
        encryptkey <"pubkey"> <-new="passphrase"> <-old="oldpassphrase">

Changes the passphrase for <oldpassphrase> to <passphrase>
```
**Arguments:**
```
 "pubkey"                               (string, required) public key or pubkey address
 -new="passphrase"                      (string, required) passphrase of key
 -old="oldpassphrase"                   (string, required) old passphrase of key
```
**Request:**
```
 "param" :
 {
   "pubkey": "",                        (string, required) public key or pubkey address
   "passphrase": "",                    (string, required) passphrase of key
   "oldpassphrase": ""                  (string, required) old passphrase of key
 }
```
**Response:**
```
 "result": "result"                     (string, required) encrypt key result
```
**Examples:**
```
>> encryptkey 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b 456 123
<< Encrypt key successfully: 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b

>> curl -d '{"id":5,"method":"encryptkey","jsonrpc":"2.0","params":{"pubkey":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","passphrase":"456","oldpassphrase":"123"}}' http://127.0.0.1:8812
<< {"id":5,"jsonrpc":"2.0","result":"Encrypt key successfully: 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
* {"code":-6,"message":"Passphrase must be nonempty"}
* {"code":-6,"message":"Old passphrase must be nonempty"}
* {"code":-4,"message":"Unknown key"}
* {"code":-406,"message":"The passphrase entered was incorrect."}
```
##### [Back to top](#commands)
---
### lockkey
**Usage:**
```
        lockkey <"pubkey">

Removes the encryption key from memory, locking the key.
After calling this method, you will need to call unlockkey again.
before being able to call any methods which require the key to be unlocked.
```
**Arguments:**
```
 "pubkey"                               (string, required) pubkey or pubkey address
```
**Request:**
```
 "param" :
 {
   "pubkey": ""                         (string, required) pubkey or pubkey address
 }
```
**Response:**
```
 "result": "result"                     (string, required) lock key result
```
**Examples:**
```
>> metabasenet-cli lockkey 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b
<< Lock key successfully: 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b

>> curl -d '{"id":1,"method":"lockkey","jsonrpc":"2.0","params":{"pubkey":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":"Lock key successfully: 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
* {"code":-4,"message":"Unknown key"}
* {"code":-4,"message":"Can't lock public key."}
* {"code":-401,"message":"Failed to lock key"}
```
##### [Back to top](#commands)
---
### unlockkey
**Usage:**
```
        unlockkey <"pubkey"> <"passphrase"> (-t=timeout)

If (timeout) > 0,stores the wallet decryption key in memory for (timeout) seconds.
before being able to call any methods which require the key to be locked.
```
**Arguments:**
```
 "pubkey"                               (string, required) pubkey or pubkey address
 "passphrase"                           (string, required) passphrase
 -t=timeout                             (int, optional) auto unlock timeout
```
**Request:**
```
 "param" :
 {
   "pubkey": "",                        (string, required) pubkey or pubkey address
   "passphrase": "",                    (string, required) passphrase
   "timeout": 0                         (int, optional) auto unlock timeout
 }
```
**Response:**
```
 "result": "result"                     (string, required) unlock key result
```
**Examples:**
```
>> metabasenet-cli unlockkey 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b 123
<< Unlock key successfully: 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b

>> curl -d '{"id":13,"method":"unlockkey","jsonrpc":"2.0","params":{"pubkey":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","passphrase":"123"}}' http://127.0.0.1:8812
<< {"id":13,"jsonrpc":"2.0","result":"Unlock key successfully: 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b"}

>> metabasenet-cli unlockkey 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b 123 10
<< Unlock key successfully: 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b

>> curl -d '{"id":15,"method":"unlockkey","jsonrpc":"2.0","params":{"pubkey":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","passphrase":"123","timeout":10}}' http://127.0.0.1:8812
<< {"id":15,"jsonrpc":"2.0","result":"Unlock key successfully: 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
* {"code":-6,"message":"Passphrase must be nonempty"}
* {"code":-4,"message":"Unknown key"}
* {"code":-4,"message":"Can't unlock public key"}
* {"code":-409,"message":"Key is already unlocked"}
* {"code":-406,"message":"The passphrase entered was incorrect."}
```
##### [Back to top](#commands)
---
### importprivkey
**Usage:**
```
        importprivkey <"privkey"> <"passphrase">

Add a private key to your wallet.
```
**Arguments:**
```
 "privkey"                              (string, required) private key
 "passphrase"                           (string, required) passphrase
```
**Request:**
```
 "param" :
 {
   "privkey": "",                       (string, required) private key
   "passphrase": ""                     (string, required) passphrase
 }
```
**Response:**
```
 "result" :
 {
   "pubkey": "",                        (string, required) public key
   "address": ""                        (string, required) address
 }
```
**Examples:**
```
>> metabasenet-cli importprivkey 0xf4d5a6d5aa6ca2e2e03f1b6d8d333a2476c7768338c8efa90f90ad4d7c8182bd 123
<< {"pubkey":"0x1d72e78cb2a523947fb66084c27d907bfe7782a403b18cf190e92b7c38d9a8dd5b9f5accdf135935ff8911e3a1b01f6b813fd71906de763930a0c51016aecfb6","address":"0x0af60d2139ada7a9a4377f6d95cbf3a047066219"}

>> curl -d '{"id":9,"method":"importprivkey","jsonrpc":"2.0","params":{"privkey":"0xf4d5a6d5aa6ca2e2e03f1b6d8d333a2476c7768338c8efa90f90ad4d7c8182bd","passphrase":"123"}}' http://127.0.0.1:8812
<< {"id":9,"jsonrpc":"2.0","result":{"pubkey":"0x1d72e78cb2a523947fb66084c27d907bfe7782a403b18cf190e92b7c38d9a8dd5b9f5accdf135935ff8911e3a1b01f6b813fd71906de763930a0c51016aecfb6","address":"0x0af60d2139ada7a9a4377f6d95cbf3a047066219"}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid private key"}
* {"code":-6,"message":"Passphrase must be nonempty"}
* {"code":-401,"message":"Failed to add key"}
```
##### [Back to top](#commands)
---
### importpubkey
**Usage:**
```
        importpubkey <"pubkey">

Add a public key to your wallet.
```
**Arguments:**
```
 "pubkey"                               (string, required) pubkey key
```
**Request:**
```
 "param" :
 {
   "pubkey": ""                         (string, required) pubkey key
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of public key
```
**Examples:**
```
>> metabasenet-cli importpubkey 0x0c5a8805c62f6fc44d1a3bf81107e8c22372c823b875c5a31af55b4951dcc3ac27a6c2b5e867077cd235439cbf8cea28d475401560d0c6e38013b72b307b0941
<< 0xcc963a4ca2a9032e41668756cc4ccb7c59f112be

>> curl -d '{"id":31,"method":"importpubkey","jsonrpc":"2.0","params":{"pubkey":"0x0c5a8805c62f6fc44d1a3bf81107e8c22372c823b875c5a31af55b4951dcc3ac27a6c2b5e867077cd235439cbf8cea28d475401560d0c6e38013b72b307b0941"}' http://127.0.0.1:8812
<< {"id":31,"jsonrpc":"2.0","result":"0xcc963a4ca2a9032e41668756cc4ccb7c59f112be"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
* {"code":-401,"message":"Failed to add key"}
```
##### [Back to top](#commands)
---
### importkey
**Usage:**
```
        importkey <"pubkey">

Reveal the serialized key corresponding to <pubkey>.
```
**Arguments:**
```
 "pubkey"                               (string, required) public key data
```
**Request:**
```
 "param" :
 {
   "pubkey": ""                         (string, required) public key data
 }
```
**Response:**
```
 "result": "pubkey"                     (string, required) public key with hex number system
```
**Examples:**
```
>> metabasenet-cli importkey 0x65371a7a4dce111aa90a6555b94d2a7225af1325d2ecb593a3c418cc694d2ea6a17733bd1badb0d557188287c33e3566c60bca433e57c3561cd4d2b76968cda9010000003057b6a1f2869353d731724a1c2f025cb96d102656e33d07ea595a56473f7d4a153f258766472b01f2f287fba11cbc008cdee177bf0c99ba063cb587d401438ab13b153cafc4edc193062a37f54c37550c6430ff4e733cf7d1
<< 0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765

>> curl -d '{"id":3,"method":"importkey","jsonrpc":"2.0","params":{"pubkey":"0x65371a7a4dce111aa90a6555b94d2a7225af1325d2ecb593a3c418cc694d2ea6a17733bd1badb0d557188287c33e3566c60bca433e57c3561cd4d2b76968cda9010000003057b6a1f2869353d731724a1c2f025cb96d102656e33d07ea595a56473f7d4a153f258766472b01f2f287fba11cbc008cdee177bf0c99ba063cb587d401438ab13b153cafc4edc193062a37f54c37550c6430ff4e733cf7d1"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765"}
```
**Errors:**
```
* {"code":-32602,"message":"Failed to verify serialized key"}
* {"code":-32602,"message":"Can't import the key with empty passphrase"}
* {"code":-401,"message":"Failed to add key"}
```
##### [Back to top](#commands)
---
### exportkey
**Usage:**
```
        exportkey <"pubkey">

Reveal the serialized key corresponding to <pubkey>.
```
**Arguments:**
```
 "pubkey"                               (string, required) public key
```
**Request:**
```
 "param" :
 {
   "pubkey": ""                         (string, required) public key
 }
```
**Response:**
```
 "result": "pubkey"                     (string, required) public key with binary system
```
**Examples:**
```
>> metabasenet-cli exportkey 0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765
<< 0x65371a7a4dce111aa90a6555b94d2a7225af1325d2ecb593a3c418cc694d2ea6a17733bd1badb0d557188287c33e3566c60bca433e57c3561cd4d2b76968cda9010000003057b6a1f2869353d731724a1c2f025cb96d102656e33d07ea595a56473f7d4a153f258766472b01f2f287fba11cbc008cdee177bf0c99ba063cb587d401438ab13b153cafc4edc193062a37f54c37550c6430ff4e733cf7d1

>> curl -d '{"id":13,"method":"exportkey","jsonrpc":"2.0","params":{"pubkey":"0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765"}}' http://127.0.0.1:8812
<< {"id":13,"jsonrpc":"2.0","result":"0x65371a7a4dce111aa90a6555b94d2a7225af1325d2ecb593a3c418cc694d2ea6a17733bd1badb0d557188287c33e3566c60bca433e57c3561cd4d2b76968cda9010000003057b6a1f2869353d731724a1c2f025cb96d102656e33d07ea595a56473f7d4a153f258766472b01f2f287fba11cbc008cdee177bf0c99ba063cb587d401438ab13b153cafc4edc193062a37f54c37550c6430ff4e733cf7d1"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
* {"code":-4,"message":"Unknown key"}
* {"code":-401,"message":"Failed to export key"}
```
##### [Back to top](#commands)
---
### addnewtemplate
**Usage:**
```
        addnewtemplate <"type"> <{delegate}>|<{vote}>|<{pledge}>|<{fork}>|<{mint}>|<{activatecode}>

Return encoded address for the given template id.
```
**Arguments:**
```
 "type"                                 (string, required) template type
  (if type=delegate)
 {delegate}                             (object, required) a delegate template
  (if type=vote)
 {vote}                                 (object, required) a vote template
  (if type=pledge)
 {pledge}                               (object, required) a pledge template
  (if type=fork)
 {fork}                                 (object, required) a new fork template
  (if type=mint)
 {mint}                                 (object, required) a mint template
  (if type=activatecode)
 {activatecode}                         (object, required) a activatecode template
```
**Request:**
```
 "param" :
 {
   "type": "",                          (string, required) template type
   (if type=delegate)
   "delegate":                          (object, required) a delegate template
   {
     "mint": "",                        (string, required) mint address
     "owner": "",                       (string, required) owner address
     "rewardratio": 0                   (uint, required) reward ratio, range: 0~10000
   }
   (if type=vote)
   "vote":                              (object, required) a vote template
   {
     "delegate": "",                    (string, required) delegate template address
     "owner": "",                       (string, required) owner address
     "rewardmode": 0                    (int, required) reward mode, 0: reward vote address, 1: reward owner address
   }
   (if type=pledge)
   "pledge":                            (object, required) a pledge template
   {
     "delegate": "",                    (string, required) delegate template address
     "owner": "",                       (string, required) owner address
     "pledgetype": 0,                   (int, required) pledge type
     "cycles": 0,                       (int, required) cycles
     "nonce": 0                         (int, required) nonce
   }
   (if type=fork)
   "fork":                              (object, required) a new fork template
   {
     "redeem": "",                      (string, required) redeem address
     "fork": ""                         (string, required) fork hash
   }
   (if type=mint)
   "mint":                              (object, required) a mint template
   {
     "mint": "",                        (string, required) mint public key
     "spent": ""                        (string, required) spent address
   }
   (if type=activatecode)
   "activatecode":                      (object, required) a activatecode template
   {
     "grant": ""                        (string, required) grant address
   }
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of template
```
**Examples:**
```
>> metabasenet-cli addnewtemplate delegate '{"mint": "0x316a4d04650b0190377fc40c2b183992046f72c9", "owner": "0x5bc5c1726286ff0a8006b19312ca307210e0e658","rewardratio":500}'
<< 0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d

>> curl -d '{"id":1,"method":"addnewtemplate","jsonrpc":"2.0","params":{"type":"delegate","delegate":{"mint":"0x316a4d04650b0190377fc40c2b183992046f72c9","owner":"0x5bc5c1726286ff0a8006b19312ca307210e0e658","rewardratio":500}}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":"0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d"}

>> metabasenet-cli addnewtemplate fork '{"redeem":"0x5bc5c1726286ff0a8006b19312ca307210e0e658","fork":"0x00000079cae069a3dc43deb4c41600fdf1e457d6a807ee770921d1f88690f918"}'

>> metabasenet-cli addnewtemplate vote '{"delegate": "0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d", "owner": "0x0a9f6b9e0de14c2c9d02883904a69c7bee82c2a5", "rewardmode":0}'

>> metabasenet-cli addnewtemplate pledge '{"delegate": "0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d", "owner": "0x0a9f6b9e0de14c2c9d02883904a69c7bee82c2a5", "pledgetype":1, "cycles":1, "nonce":0}'
```
**Errors:**
```
* {"code":-6,"message":"Invalid parameters,failed to make template"}
* {"code":-401,"message":"Failed to add template"}
* {"code":-6,"message":"Invalid parameter, missing redeem address"}
* {"code":-6,"message":"Invalid parameter, missing spent address"}
* {"code":-6,"message":"Invalid parameter, missing owner address"}
* {"code":-6,"message":"template type error. type: xxx"}
```
##### [Back to top](#commands)
---
### importtemplate
**Usage:**
```
        importtemplate <"data">

Return encoded address for the given template.
```
**Arguments:**
```
 "data"                                 (string, required) template data
```
**Request:**
```
 "param" :
 {
   "data": ""                           (string, required) template data
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of template
```
**Examples:**
```
>> metabasenet-cli importtemplate 0x03316a4d04650b0190377fc40c2b183992046f72c95bc5c1726286ff0a8006b19312ca307210e0e65801f4
<< 0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d

>> curl -d '{"id":52,"method":"importtemplate","jsonrpc":"2.0","params":{"data":"0x03316a4d04650b0190377fc40c2b183992046f72c95bc5c1726286ff0a8006b19312ca307210e0e65801f4"}}' http://127.0.0.1:8812
<< {"id":52,"jsonrpc":"2.0","result":"0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid parameters,failed to make template"}
* {"code":-401,"message":"Failed to add template"}
```
##### [Back to top](#commands)
---
### exporttemplate
**Usage:**
```
        exporttemplate <"address">

Return encoded address for the given template.
```
**Arguments:**
```
 "address"                              (string, required) template address
```
**Request:**
```
 "param" :
 {
   "address": ""                        (string, required) template address
 }
```
**Response:**
```
 "result": "data"                       (string, required) data of template
```
**Examples:**
```
>> metabasenet-cli exporttemplate 0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d
<< 0x03316a4d04650b0190377fc40c2b183992046f72c95bc5c1726286ff0a8006b19312ca307210e0e65801f4

>> curl -d '{"id":25,"method":"exporttemplate","jsonrpc":"2.0","params":{"address":"0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d"}}' http://127.0.0.1:8812
<< {"id":25,"jsonrpc":"2.0","result":"0x03316a4d04650b0190377fc40c2b183992046f72c95bc5c1726286ff0a8006b19312ca307210e0e65801f4"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid address"}
* {"code":-401,"message":"Unkown template"}
```
##### [Back to top](#commands)
---
### removetemplate
**Usage:**
```
        removetemplate <"address">

Remove template in Wallet.
```
**Arguments:**
```
 "address"                              (string, required) template address
```
**Request:**
```
 "param" :
 {
   "address": ""                        (string, required) template address
 }
```
**Response:**
```
 "result": "result"                     (string, required) remove result
```
**Examples:**
```
>> metabasenet-cli removetemplate 0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d
<< Success

>> curl -d '{"id":7,"method":"removetemplate","jsonrpc":"2.0","params":{"address":"0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d"}}' http://127.0.0.1:8812
<< {"id":7,"jsonrpc":"2.0","result":"Success"}
```
**Errors:**
```
* {"code":-401,"message":"Failed remove template."}
```
##### [Back to top](#commands)
---
### validateaddress
**Usage:**
```
        validateaddress <"address"> (-f="fork") (-b="block")

Return information about <address>.
```
**Arguments:**
```
 "address"                              (string, required) wallet address
 -f="fork"                              (string, optional) fork hash (default genesis fork)
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) wallet address
   "fork": "",                          (string, optional) fork hash (default genesis fork)
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result" :
 {
   "isvalid": true|false,               (bool, required) is valid
   (if isvalid=true)
   "addressdata":                       (object, required) address data
   {
     "address": "",                     (string, required) wallet address
     "ismine": true|false,              (bool, required) is mine
     "type": "",                        (string, required) type, pubkey or template
     (if type=pubkey)
     "pubkey": "",                      (string, optional) public key
     (if type=template)
     "template": "",                    (string, optional) template type name
     (if type=template)
     "templatedata":                    (object, optional) template data
     {
       "type": "",                      (string, required) template type
       "hex": "",                       (string, required) temtplate data
       (if type=delegate)
       "delegate":                      (object, required) delegate template struct
       {
         "mint": "",                    (string, required) mint address
         "owner": "",                   (string, required) owner address
         "rewardratio": 0               (uint, required) reward ratio, range: 0~10000
       }
       (if type=vote)
       "vote":                          (object, required) vote template struct
       {
         "delegate": "",                (string, required) delegate template address
         "owner": "",                   (string, required) owner address
         "rewardmode": 0                (int, required) reward mode, 0: reward vote address, 1: reward owner address
       }
       (if type=pledge)
       "pledge":                        (object, required) a pledge template
       {
         "delegate": "",                (string, required) delegate template address
         "owner": "",                   (string, required) owner address
         "pledgetype": 0,               (int, required) pledge type
         "cycles": 0,                   (int, required) cycles
         "nonce": 0                     (int, required) nonce
       }
       (if type=fork)
       "fork":                          (object, required) fork template struct
       {
         "redeem": "",                  (string, required) redeem address
         "fork": ""                     (string, required) fork hash
       }
       (if type=mint)
       "mint":                          (object, required) mint template struct
       {
         "mint": "",                    (string, required) mint public key
         "spent": ""                    (string, required) spent address
       }
       (if type=activatecode)
       "activatecode":                  (object, required) a activatecode template
       {
         "grant": ""                    (string, required) grant address
       }
     }
   }
 }
```
**Examples:**
```
>> metabasenet-cli validateaddress 0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d
<< {"isvalid":true,"addressdata":{"address":"0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d","ismine":true,"type":"template","template":"mint"}}

>> curl -d '{"id":2,"method":"validateaddress","jsonrpc":"2.0","params":{"address":"0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d"}}' http://127.0.0.1:8812
<< {"id":2,"jsonrpc":"2.0","result":{"isvalid":true,"addressdata":{"address":"0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d","ismine":true,"type":"template","template":"mint"}}}

>> metabasenet-cli validateaddress 123
<< {"isvalid":false}

>> curl -d '{"id":3,"method":"validateaddress","jsonrpc":"2.0","params":{"address":"123"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":{"isvalid":false}}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### getbalance
**Usage:**
```
        getbalance (-f="fork") (-c="coinsymbol") (-a="address") (-b="block") (-p=page) (-n=count)

Get balance of address.
If (address) is not specified, return the balance for wallet's each address.
If (address) is specified, return the balance in the address.
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash, default is genesis
 -c="coinsymbol"                        (string, optional) coin symbol
 -a="address"                           (string, optional) address, default is all
 -b="block"                             (string, optional) block hash or number or latest, default is latest block
 -p=page                                (uint, optional, default=0) page, default is 0
 -n=count                               (uint, optional, default=30) count, default is 30
```
**Request:**
```
 "param" :
 {
   "fork": "",                          (string, optional) fork hash, default is genesis
   "coinsymbol": "",                    (string, optional) coin symbol
   "address": "",                       (string, optional) address, default is all
   "block": "",                         (string, optional) block hash or number or latest, default is latest block
   "page": 0,                           (uint, optional, default=0) page, default is 0
   "count": 0                           (uint, optional, default=30) count, default is 30
 }
```
**Response:**
```
 "result" :
   "balance":                           (array, required, default=RPCValid) 
   [
     {
       "address": "",                   (string, required) address
       "type": "",                      (string, required) address type
       "coinsymbol": "",                (string, required) coin symbol
       "nonce": 0,                      (uint, required) last tx nonce
       "avail": "",                     (string, required) balance available amount (big float)
       "locked": "",                    (string, required) locked amount (big float)
       "unconfirmedin": "",             (string, required) unconfirmed in amount (big float)
       "unconfirmedout": ""             (string, required) unconfirmed out amount (big float)
     }
   ]
```
**Examples:**
```
>> metabasenet-cli getbalance
<< [{"address":"0x5962974eeb0b17b43edabfc9b747839317aa852f","type":"pubkey","nonce":24,"avail":"157989979.4212","locked":"0.0","unconfirmedin":"0.0","unconfirmedout":"0.0"}]

>> curl -d '{"id":1,"method":"getbalance","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":[{"address":"0x5962974eeb0b17b43edabfc9b747839317aa852f","type":"pubkey","nonce":24,"avail":"157989979.4212","locked":"0.0","unconfirmedin":"0.0","unconfirmedout":"0.0"}}

>> metabasenet-cli getbalance -a=0x5962974eeb0b17b43edabfc9b747839317aa852f
<< [{"address":"0x5962974eeb0b17b43edabfc9b747839317aa852f","type":"pubkey","nonce":24,"avail":"157989979.4212","locked":"0.0","unconfirmedin":"0.0","unconfirmedout":"0.0"}]

>> curl -d '{"id":20,"method":"getbalance","jsonrpc":"2.0","params":{"address":"0x5962974eeb0b17b43edabfc9b747839317aa852f"}}' http://127.0.0.1:8812
<< {"id":20,"jsonrpc":"2.0","result":[{"address":"0x5962974eeb0b17b43edabfc9b747839317aa852f","type":"pubkey","nonce":24,"avail":"157989979.4212","locked":"0.0","unconfirmedin":"0.0","unconfirmedout":"0.0"}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid address"}
```
##### [Back to top](#commands)
---
### listtransaction
**Usage:**
```
        listtransaction <"address"> (-f="fork") (-o=offset) (-n=count) (-r|-nor*reverse*) (-b="block")

return up to (count) most recent transactions skipping the first (offset) transactions, need set config 'fulldb=true'.
```
**Arguments:**
```
 "address"                              (string, required) address
 -f="fork"                              (string, optional) fork hash. If not set, default is genesis
 -o=offset                              (uint, optional, default=0) query offset. If reverse is true and offset is 0, from last tx
 -n=count                               (uint, optional, default=30) transaction count, if 0 then 1000, max is 1000, default is 30
 -r|-nor*reverse*                       (bool, optional, default=false) if reverse order, false is positive order, true is reverse order
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) address
   "fork": "",                          (string, optional) fork hash. If not set, default is genesis
   "offset": 0,                         (uint, optional, default=0) query offset. If reverse is true and offset is 0, from last tx
   "count": 0,                          (uint, optional, default=30) transaction count, if 0 then 1000, max is 1000, default is 30
   "reverse": true|false,               (bool, optional, default=false) if reverse order, false is positive order, true is reverse order
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result" :
   "transaction":                       (array, required, default=RPCValid) 
   [
     {
       "txid": "",                      (string, required) transaction hash
       "txindex": 0,                    (int, required) tx index
       "blocknumber": 0,                (uint, required) block number
       "transfertype": "",              (string, required) transfer type
       "txtype": "",                    (string, required) transaction type
       "time": 0,                       (uint, required) transaction timestamp
       "from": "",                      (string, required) from address
       "to": "",                        (string, required) to address
       "amount": "",                    (string, required) transaction amount (big float)
       "fee": ""                        (string, required) transaction fee (big float)
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listtransaction 0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f
<< [{"txid":"0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f","txindex":0,"blocknumber":2081,"transfertype":"common","txtype":"token","time":1690338133,"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","amount":"10.0","fee":"0.0"}]

>> curl -d '{"id":2,"method":"listtransaction","jsonrpc":"2.0","params":{"address":"0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f"}}' http://127.0.0.1:8812
<< {"id":2,"jsonrpc":"2.0","result":[{"txid":"0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f","txindex":0,"blocknumber":2081,"transfertype":"common","txtype":"token","time":1690338133,"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","amount":"10.0","fee":"0.0"}]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid address"}
* {"code":-401,"message":"Failed list transactions"}
* {"code":-32600,"message":"If you need this function, please set config 'fulldb=true' and restart"}
```
##### [Back to top](#commands)
---
### sendfrom
**Usage:**
```
        sendfrom <"from"> <"to"> <"amount"> (-n=nonce) (-p="gasprice") (-g=gas) (-f="fork") (-d="data") (-fd="fdata") (-td="todata") (-cc="contractcode") (-cp="contractparam") (-cs="coinsymbol")

<amount> and <txfee> are real and rounded to the nearest 0.000001
Return transaction id
```
**Arguments:**
```
 "from"                                 (string, required) from address
 "to"                                   (string, required) to address
 "amount"                               (string, required) amount (big float)
 -n=nonce                               (uint, optional) nonce
 -p="gasprice"                          (string, optional) gas price (big float)
 -g=gas                                 (uint, optional) gas
 -f="fork"                              (string, optional) fork hash
 -d="data"                              (string, optional) output data
 -fd="fdata"                            (string, optional) format data
 -td="todata"                           (string, optional) If the 'to' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
 -cc="contractcode"                     (string, optional) contract code, code or code hash or contract address
 -cp="contractparam"                    (string, optional) contract param
 -cs="coinsymbol"                       (string, optional) coin symbol
```
**Request:**
```
 "param" :
 {
   "from": "",                          (string, required) from address
   "to": "",                            (string, required) to address
   "amount": "",                        (string, required) amount (big float)
   "nonce": 0,                          (uint, optional) nonce
   "gasprice": "",                      (string, optional) gas price (big float)
   "gas": 0,                            (uint, optional) gas
   "fork": "",                          (string, optional) fork hash
   "data": "",                          (string, optional) output data
   "fdata": "",                         (string, optional) format data
   "todata": "",                        (string, optional) If the 'to' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
   "contractcode": "",                  (string, optional) contract code, code or code hash or contract address
   "contractparam": "",                 (string, optional) contract param
   "coinsymbol": ""                     (string, optional) coin symbol
 }
```
**Response:**
```
 "result": "transaction"                (string, required) transaction id
```
**Examples:**
```
>> metabasenet-cli sendfrom 0x5962974eeb0b17b43edabfc9b747839317aa852f 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b 10
<< 0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f

>> curl -d '{"id":18,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","amount":"10"}}' http://127.0.0.1:8812
<< {"id":18,"jsonrpc":"2.0","result":"0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f"}

>> metabasenet-cli sendfrom 0x5962974eeb0b17b43edabfc9b747839317aa852f 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b 10 -f=0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55
<< 61ade019cedde7580b4a44c2cda6b4a2d6fdbb0d2ff0115e9cd4ce7a93d30fd2

>> curl -d '{"id":53,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","amount":"10","fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55"}}' http://127.0.0.1:8812
<< {"id":53,"jsonrpc":"2.0","result":"61ade019cedde7580b4a44c2cda6b4a2d6fdbb0d2ff0115e9cd4ce7a93d30fd2"}

>> metabasenet-cli sendfrom 0x5962974eeb0b17b43edabfc9b747839317aa852f 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b 10 -n=2 -p=0.000002 -g=30000 -d=0x9a87d21254
<< 0x01fd16a69bc8607d283e505225013f085d438621d5bd62b3c2ac4b2b37907403

>> curl -d '{"id":18,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","amount":"10","nonce":2,"gasprice":"0.000002","gas":30000,"data":"0x9a87d21254"}}' http://127.0.0.1:8812
<< {"id":18,"jsonrpc":"2.0","result":"0x01fd16a69bc8607d283e505225013f085d438621d5bd62b3c2ac4b2b37907403"}

>> metabasenet-cli sendfrom 0x5962974eeb0b17b43edabfc9b747839317aa852f 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b 1000000 -td=0x050017b24437dafecb9c83cb9f8bbe60c43acf42eebcb69c1525e6f84077fb293f880191ba8db6735d7ae45f92841eaaa588a23f1a3be785076fd737ec1911bbd8e757f4010000
<< 0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f

>> curl -d '{"id":18,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","amount":"1000000","todata":"0x050017b24437dafecb9c83cb9f8bbe60c43acf42eebcb69c1525e6f84077fb293f880191ba8db6735d7ae45f92841eaaa588a23f1a3be785076fd737ec1911bbd8e757f4010000"}}' http://127.0.0.1:8812
<< {"id":18,"jsonrpc":"2.0","result":"0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f"}

>> metabasenet-cli sendfrom 0x5962974eeb0b17b43edabfc9b747839317aa852f 0 0 -cc=<contract code> -cp=<contract param>
<< 0x56695bf2ad39aaff472bcf8ed291bbbfca9fffc3b2e0494108d6a5d4a31e75f5

>> curl -d '{"id":18,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0","amount":"0","contractcode":"<contract code>","contractparam":"<contract param>"}}' http://127.0.0.1:8812
<< {"id":18,"jsonrpc":"2.0","result":"0x56695bf2ad39aaff472bcf8ed291bbbfca9fffc3b2e0494108d6a5d4a31e75f5"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid from address"}
* {"code":-6,"message":"Invalid to address"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid amount"}
* {"code":-6,"message":"Invalid gasprice"}
* {"code":-401,"message":"Failed to create transaction"}
* {"code":-401,"message":"Failed to sign transaction"}
* {"code":-10,"message":"Tx rejected : xxx"}
```
##### [Back to top](#commands)
---
### createtransaction
**Usage:**
```
        createtransaction <"from"> <"to"> <"amount"> (-n=nonce) (-p="gasprice") (-g=gas) (-f="fork") (-d="data") (-fd="fdata") (-td="todata") (-cc="contractcode") (-cp="contractparam")

<amount> and <txfee> are real and rounded to the nearest 0.000001.
Return serialized tx.
```
**Arguments:**
```
 "from"                                 (string, required) from address
 "to"                                   (string, required) to address
 "amount"                               (string, required) amount (big float)
 -n=nonce                               (uint, optional) nonce
 -p="gasprice"                          (string, optional) gas price (big float)
 -g=gas                                 (uint, optional) gas
 -f="fork"                              (string, optional) fork hash
 -d="data"                              (string, optional) output data
 -fd="fdata"                            (string, optional) format data
 -td="todata"                           (string, optional) If the 'to' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
 -cc="contractcode"                     (string, optional) contract code, code or code hash or contract address
 -cp="contractparam"                    (string, optional) contract param
```
**Request:**
```
 "param" :
 {
   "from": "",                          (string, required) from address
   "to": "",                            (string, required) to address
   "amount": "",                        (string, required) amount (big float)
   "nonce": 0,                          (uint, optional) nonce
   "gasprice": "",                      (string, optional) gas price (big float)
   "gas": 0,                            (uint, optional) gas
   "fork": "",                          (string, optional) fork hash
   "data": "",                          (string, optional) output data
   "fdata": "",                         (string, optional) format data
   "todata": "",                        (string, optional) If the 'to' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
   "contractcode": "",                  (string, optional) contract code, code or code hash or contract address
   "contractparam": ""                  (string, optional) contract param
 }
```
**Response:**
```
 "result": "transaction"                (string, required) transaction data
```
**Examples:**
```
>> metabasenet-cli createtransaction 0x5962974eeb0b17b43edabfc9b747839317aa852f 0x7d3cfc954deead4a4791dff7fb359a5e543bab2b 10
<< 0x02c918142f85aa17938347b7c9bfda3eb4170beb4e976259142bab3b545e9a35fbf7df91474aadee4d95fc3c7d088ac7230489e8000005e8d4a510000252080000

>> curl -d '{"id":59,"method":"createtransaction","jsonrpc":"2.0","params":{"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0x7d3cfc954deead4a4791dff7fb359a5e543bab2b","amount":"10",}}' http://127.0.0.1:8812
<< {"id":59,"jsonrpc":"2.0","result":"0x02c918142f85aa17938347b7c9bfda3eb4170beb4e976259142bab3b545e9a35fbf7df91474aadee4d95fc3c7d088ac7230489e8000005e8d4a510000252080000"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid from address"}
* {"code":-6,"message":"Invalid to address"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid gasprice"}
* {"code":-401,"message":"Failed to create transaction"}
```
##### [Back to top](#commands)
---
### signtransaction
**Usage:**
```
        signtransaction <"txdata">

Return json object with keys:
hex : raw transaction with signature(s) (hex-encoded string)
completed : true if transaction has a completed set of signature (false if not)
```
**Arguments:**
```
 "txdata"                               (string, required) transaction data(hex string)
```
**Request:**
```
 "param" :
 {
   "txdata": ""                         (string, required) transaction data(hex string)
 }
```
**Response:**
```
 "result": "signature"                  (string, required) hex of transaction data
```
**Examples:**
```
>> metabasenet-cli signtransaction 0x02c918142f85aa17938347b7c9bfda3eb4170beb4e976259142bab3b545e9a35fbf7df91474aadee4d95fc3c7d088ac7230489e8000005e8d4a510000252080000
<< {"hex":"0x02c918142f85aa17938347b7c9bfda3eb4170beb4e976259142bab3b545e9a35fbf7df91474aadee4d95fc3c7d088ac7230489e8000005e8d4a51000025208004150b186ec91f439696626447f0f379f146665099f2269e84fde9f96802c3410d110716aaf722b8009b8b3585be64163cc21a8f22f404f3c1ccf2af3fec03bf2ea01","complete":true}

>> curl -d '{"id":62,"method":"signtransaction","jsonrpc":"2.0","params":{"txdata":"0x02c918142f85aa17938347b7c9bfda3eb4170beb4e976259142bab3b545e9a35fbf7df91474aadee4d95fc3c7d088ac7230489e8000005e8d4a510000252080000"}}' http://127.0.0.1:8812
<< {"id":62,"jsonrpc":"2.0","result":{"hex":"0x02c918142f85aa17938347b7c9bfda3eb4170beb4e976259142bab3b545e9a35fbf7df91474aadee4d95fc3c7d088ac7230489e8000005e8d4a51000025208004150b186ec91f439696626447f0f379f146665099f2269e84fde9f96802c3410d110716aaf722b8009b8b3585be64163cc21a8f22f404f3c1ccf2af3fec03bf2ea01","complete":true}}
```
**Errors:**
```
* {"code":-8,"message":"TX decode failed"}
* {"code":-401,"message":"Failed to sign transaction"}
```
##### [Back to top](#commands)
---
### signmessage
**Usage:**
```
        signmessage <"message"> (-pubkey="pubkey") (-privkey="privkey") (-a="addr") (-p|-nop*hasprefix*)

Sign a message with the private key of an pubkey
```
**Arguments:**
```
 "message"                              (string, required) message to be signed
 -pubkey="pubkey"                       (string, optional) public key
 -privkey="privkey"                     (string, optional) private key
 -a="addr"                              (string, optional) address signed
 -p|-nop*hasprefix*                     (bool, optional, default=true) If add magic string(MetabaseNet Signed Message:
) before message or not
```
**Request:**
```
 "param" :
 {
   "message": "",                       (string, required) message to be signed
   "pubkey": "",                        (string, optional) public key
   "privkey": "",                       (string, optional) private key
   "addr": "",                          (string, optional) address signed
   "hasprefix": true|false              (bool, optional, default=true) If add magic string(MetabaseNet Signed Message:
) before message or not
 }
```
**Response:**
```
 "result": "signature"                  (string, required) signature of message
```
**Examples:**
```
>> metabasenet-cli signmessage 123456 -pubkey=0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765
<< 0xadefe8db8e6f5f845366a520ed6f295e02f2e7e2440096c95f4db37cd40423f53b390213bce4a23a884b4868294222bf462edaf9e455d9fd1e65f231b832425901

>> curl -d '{"id":4,"method":"signmessage","jsonrpc":"2.0","params":{"pubkey":"0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765","message":"123456"}}' http://127.0.0.1:8812
<< {"id":4,"jsonrpc":"2.0","result":"0xadefe8db8e6f5f845366a520ed6f295e02f2e7e2440096c95f4db37cd40423f53b390213bce4a23a884b4868294222bf462edaf9e455d9fd1e65f231b832425901"}
```
**Errors:**
```
* {"code":-4,"message":"Unknown key"}
* {"code":-405,"message":"Key is locked"}
* {"code":-401,"message":"Failed to sign message"}
```
##### [Back to top](#commands)
---
### addusercoin
**Usage:**
```
        addusercoin <"from"> <"symbol"> <chainid>

Add user coin
```
**Arguments:**
```
 "from"                                 (string, required) from address
 "symbol"                               (string, required) coin symbol
 chainid                                (uint, required) at chainid
```
**Request:**
```
 "param" :
 {
   "from": "",                          (string, required) from address
   "symbol": "",                        (string, required) coin symbol
   "chainid": 0                         (uint, required) at chainid
 }
```
**Response:**
```
 "result": "txid"                       (string, required) txid
```
**Examples:**
```
>> metabasenet-cli addusercoin 0xb955034fcefb66112bab47483c8d243b86cb2c1d ABC 202
<< 0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f

>> curl -d '{"id":1,"method":"addusercoin","jsonrpc":"2.0","params":{"from":"0xb955034fcefb66112bab47483c8d243b86cb2c1d","symbol":"ABC","chainid":202}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":"0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid from"}
* {"code":-6,"message":"Invalid symbol"}
* {"code":-6,"message":"Invalid chainid"}
* {"code":-4,"message":"Unknown chainid"}
```
##### [Back to top](#commands)
---
### addcontractcoin
**Usage:**
```
        addcontractcoin <"from"> <"symbol"> <chainid> <"contractaddress">

Add contract coin
```
**Arguments:**
```
 "from"                                 (string, required) from address
 "symbol"                               (string, required) coin symbol
 chainid                                (uint, required) at chainid
 "contractaddress"                      (string, required) contract address
```
**Request:**
```
 "param" :
 {
   "from": "",                          (string, required) from address
   "symbol": "",                        (string, required) coin symbol
   "chainid": 0,                        (uint, required) at chainid
   "contractaddress": ""                (string, required) contract address
 }
```
**Response:**
```
 "result": "txid"                       (string, required) txid
```
**Examples:**
```
>> metabasenet-cli addcontractcoin 0xb955034fcefb66112bab47483c8d243b86cb2c1d ABC 202 0x3e80fb38ae4e71beaccb053b716921eb6b03519f
<< 0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f

>> curl -d '{"id":1,"method":"addcontractcoin","jsonrpc":"2.0","params":{"from":"0xb955034fcefb66112bab47483c8d243b86cb2c1d","symbol":"ABC","chainid":202,"contractaddress":"0x3e80fb38ae4e71beaccb053b716921eb6b03519f"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":"0x4646c445d31e313764db47c894b3372d72e23b9d58ada120b7229cf4ea6b1d6f"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid from"}
* {"code":-6,"message":"Invalid symbol"}
* {"code":-6,"message":"Invalid contractaddress"}
* {"code":-6,"message":"Invalid chainid"}
* {"code":-4,"message":"Unknown chainid"}
```
##### [Back to top](#commands)
---
### getcoininfo
**Usage:**
```
        getcoininfo <"symbol"> (-b="block")

Get coin information by symbol
```
**Arguments:**
```
 "symbol"                               (string, required) coin symbol
 -b="block"                             (string, optional) block hash
```
**Request:**
```
 "param" :
 {
   "symbol": "",                        (string, required) coin symbol
   "block": ""                          (string, optional) block hash
 }
```
**Response:**
```
 "result" :
 {
   "coinsymbol": "",                    (string, required) coin symbol
   "cointype": "",                      (string, required) coin type
   "fork": "",                          (string, required) at fork hash
   "chainid": 0                         (uint, required) at fork chainid
 }
```
**Examples:**
```
>> metabasenet-cli getcoininfo MNT
<< {"coinsymbol":"MNT","cointype":"forkcoin","fork":"0xc9f7f76ed1f265880a269080b5a115470fc36b06717346c44c72caca03dddcc0","chainid":201}

>> curl -d '{"id":1,"method":"getcoininfo","jsonrpc":"2.0","params":{"symbol":"MNT"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":{"coinsymbol":"MNT","cointype":"forkcoin","fork":"0xc9f7f76ed1f265880a269080b5a115470fc36b06717346c44c72caca03dddcc0","chainid":201}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid symbol"}
* {"code":-6,"message":"Invalid block"}
* {"code":-4,"message":"Unknown symbol"}
```
##### [Back to top](#commands)
---
### listcoininfo
**Usage:**
```
        listcoininfo (-b="block")

List coin information
```
**Arguments:**
```
 -b="block"                             (string, optional) block hash
```
**Request:**
```
 "param" :
 {
   "block": ""                          (string, optional) block hash
 }
```
**Response:**
```
 "result" :
   "coindata":                          (array, required, default=RPCValid) 
   [
     {
       "coinsymbol": "",                (string, required) coin symbol
       "cointype": "",                  (string, required) coin type
       "fork": "",                      (string, required) at fork hash
       "chainid": 0,                    (uint, required) at fork chainid
       "contractaddress": ""            (string, required) at contract address
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listcoininfo
<< [{"coinsymbol":"MNT","cointype":"forkcoin","fork":"0xc9f7f76ed1f265880a269080b5a115470fc36b06717346c44c72caca03dddcc0","chainid":201,"contractaddress":""}]

>> curl -d '{"id":1,"method":"listcoininfo","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":[{"coinsymbol":"MNT","cointype":"forkcoin","fork":"0xc9f7f76ed1f265880a269080b5a115470fc36b06717346c44c72caca03dddcc0","chainid":201,"contractaddress":""}]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid block"}
* {"code":-7,"message":"Get coin fail"}
```
##### [Back to top](#commands)
---
### getdexcoinpair
**Usage:**
```
        getdexcoinpair <"symbol1"> <"symbol2"> (-b="block")

Get dex coin pair by symbol pair
```
**Arguments:**
```
 "symbol1"                              (string, required) coin symbol1
 "symbol2"                              (string, required) coin symbol2
 -b="block"                             (string, optional) block hash
```
**Request:**
```
 "param" :
 {
   "symbol1": "",                       (string, required) coin symbol1
   "symbol2": "",                       (string, required) coin symbol2
   "block": ""                          (string, optional) block hash
 }
```
**Response:**
```
 "result": coinpair                     (uint, required) coin pair
```
**Examples:**
```
>> metabasenet-cli getdexcoinpair MNT YUS
<< 1

>> curl -d '{"id":1,"method":"getdexcoinpair","jsonrpc":"2.0","params":{"symbol1":"MNT","symbol1":"YUS"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":1}
```
**Errors:**
```
* {"code":-6,"message":"Invalid symbol1"}
* {"code":-6,"message":"Invalid symbol2"}
* {"code":-6,"message":"Invalid block"}
* {"code":-4,"message":"Unknown symbol"}
```
##### [Back to top](#commands)
---
### listdexcoinpair
**Usage:**
```
        listdexcoinpair (-c=coinpair) (-s="coinsymbol") (-b="block")

List dex coin pair
```
**Arguments:**
```
 -c=coinpair                            (uint, optional) coin pair
 -s="coinsymbol"                        (string, optional) coin symbol
 -b="block"                             (string, optional) block hash
```
**Request:**
```
 "param" :
 {
   "coinpair": 0,                       (uint, optional) coin pair
   "coinsymbol": "",                    (string, optional) coin symbol
   "block": ""                          (string, optional) block hash
 }
```
**Response:**
```
 "result" :
   "coinpairdata":                      (array, required, default=RPCValid) 
   [
     {
       "coinpair": 0,                   (uint, required) coin pair
       "coin1":                         (object, required) coin1
       {
         "coinsymbol": "",              (string, required) coin symbol
         "cointype": "",                (string, required) coin type
         "fork": "",                    (string, required) at fork hash
         "chainid": 0                   (uint, required) at fork chainid
       }
       "coin2":                         (object, required) coin2
       {
         "coinsymbol": "",              (string, required) coin symbol
         "cointype": "",                (string, required) coin type
         "fork": "",                    (string, required) at fork hash
         "chainid": 0                   (uint, required) at fork chainid
       }
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listdexcoinpair
<< [{"coinpair":1,"coin1":{"coinsymbol":"MNTT","cointype":"forkcoin","fork":"0xc9f7f76ed1f265880a269080b5a115470fc36b06717346c44c72caca03dddcc0","chainid":201},"coin2":{"coinsymbol":"SY202","cointype":"forkcoin","fork":"0xcab0f744f00351c05d9035b22f737325596a3b94337542dbecf623f054a6172a","chainid":202}}]

>> curl -d '{"id":1,"method":"listdexcoinpair","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":[{"coinpair":1,"coin1":{"coinsymbol":"MNTT","cointype":"forkcoin","fork":"0xc9f7f76ed1f265880a269080b5a115470fc36b06717346c44c72caca03dddcc0","chainid":201},"coin2":{"coinsymbol":"SY202","cointype":"forkcoin","fork":"0xcab0f744f00351c05d9035b22f737325596a3b94337542dbecf623f054a6172a","chainid":202}}]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid block"}
* {"code":-7,"message":"Get coin pair fail"}
* {"code":-7,"message":"Get coin context fail"}
```
##### [Back to top](#commands)
---
### senddexordertx
**Usage:**
```
        senddexordertx <"from"> <"coinsymbolowner"> <"coinsymbolpeer"> <"amount"> <"price"> (-n=ordernumber) (-f="fork")

Send dexorder transaction
```
**Arguments:**
```
 "from"                                 (string, required) from address
 "coinsymbolowner"                      (string, required) coin symbol owner
 "coinsymbolpeer"                       (string, required) coin symbol peer
 "amount"                               (string, required) order amount (big float)
 "price"                                (string, required) order amount (big float)
 -n=ordernumber                         (uint, optional) order number, default is 0
 -f="fork"                              (string, optional) fork hash or chainid, default is local fork
```
**Request:**
```
 "param" :
 {
   "from": "",                          (string, required) from address
   "coinsymbolowner": "",               (string, required) coin symbol owner
   "coinsymbolpeer": "",                (string, required) coin symbol peer
   "amount": "",                        (string, required) order amount (big float)
   "price": "",                         (string, required) order amount (big float)
   "ordernumber": 0,                    (uint, optional) order number, default is 0
   "fork": ""                           (string, optional) fork hash or chainid, default is local fork
 }
```
**Response:**
```
 "result": "result"                     (string, required) transaction hash
```
**Examples:**
```
>> metabasenet-cli senddexordertx 0xc0f0ddde81c1dff508e038d20295d8001493d8bc AAA BBB 11 0 234.98 1.23
<< 0x45b154d2ab82eac6e79b8babd97abaebe2b4ae7194f3afd946dd1419f012dd3f

>> curl -d '{"id":4,"method":"senddexordertx","jsonrpc":"2.0","params":{"from":"0xc0f0ddde81c1dff508e038d20295d8001493d8bc","coinsymbolowner":"AAA","coinsymbolpeer":"BBB","ordernumber":0,"amount":"234.98","price":"1.23"}}' http://127.0.0.1:8812
<< {"id":4,"jsonrpc":"2.0","result":"0x45b154d2ab82eac6e79b8babd97abaebe2b4ae7194f3afd946dd1419f012dd3f"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid from"}
* {"code":-6,"message":"Invalid coinsymbolowner"}
* {"code":-6,"message":"Invalid coinsymbolpeer"}
* {"code":-6,"message":"Invalid ordernumber"}
* {"code":-6,"message":"Invalid amount"}
* {"code":-6,"message":"Invalid price"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-4,"message":"Get tx nonce fail"}
* {"code":-401,"message":"Failed to sign transaction"}
* {"code":-10,"message":"Tx rejected : xxx"}
```
##### [Back to top](#commands)
---
### listdexorder
**Usage:**
```
        listdexorder <"address"> (-co="coinsymbolowner") (-cp="coinsymbolpeer") (-b=beginnumber) (-n=count) (-f="fork") (-h="block")

List dex order
```
**Arguments:**
```
 "address"                              (string, required) order address
 -co="coinsymbolowner"                  (string, optional) coin symbol owner
 -cp="coinsymbolpeer"                   (string, optional) coin symbol owner
 -b=beginnumber                         (uint, optional, default=0) begin order number
 -n=count                               (uint, optional, default=1000) count, default is 1000
 -f="fork"                              (string, optional) fork hash or chainid
 -h="block"                             (string, optional) block hash
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) order address
   "coinsymbolowner": "",               (string, optional) coin symbol owner
   "coinsymbolpeer": "",                (string, optional) coin symbol owner
   "beginnumber": 0,                    (uint, optional, default=0) begin order number
   "count": 0,                          (uint, optional, default=1000) count, default is 1000
   "fork": "",                          (string, optional) fork hash or chainid
   "block": ""                          (string, optional) block hash
 }
```
**Response:**
```
 "result" :
   "dexorderdata":                      (array, required, default=RPCValid) 
   [
     {
       "ordernumber": 0,                (uint, required) order number
       "coinsymbolowner": "",           (string, required) coin symbol owner
       "coinsymbolpeer": "",            (string, required) coin symbol peer
       "amount": "",                    (string, required) order amount (big float)
       "price": "",                     (string, required) order amount (big float)
       "surplusamount": "",             (string, required) surplus amount (big float)
       "completeamount": "",            (string, required) complete amount (big float)
       "completecount": 0,              (uint, required) complete count
       "height": 0,                     (uint, required) at height
       "slot": 0                        (uint, required) at slot
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listdexorder 0xb955034fcefb66112bab47483c8d243b86cb2c1d
<< [{"ordernumber":1,"coinsymbolowner":"AAA","coinsymbolpeer":"BBB","amount":"100.45","price":"0.5","surplusamount":"30.0","completeamount":"70.5","completecount":2,"height":398,"slot":0}]

>> curl -d '{"id":1,"method":"listdexorder","jsonrpc":"2.0","params":{"address":"0xb955034fcefb66112bab47483c8d243b86cb2c1d"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":[{"ordernumber":1,"coinsymbolowner":"AAA","coinsymbolpeer":"BBB","amount":"100.45","price":"0.5","surplusamount":"30.0","completeamount":"70.5","completecount":2,"height":398,"slot":0}]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid address"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-7,"message":"Query fail"}
```
##### [Back to top](#commands)
---
### getdexsymboltype
**Usage:**
```
        getdexsymboltype <"coinsymbol1"> <"coinsymbol2">

Get dex symbol type, sell symbol and buy symbol
```
**Arguments:**
```
 "coinsymbol1"                          (string, required) coin symbol1
 "coinsymbol2"                          (string, required) coin symbol2
```
**Request:**
```
 "param" :
 {
   "coinsymbol1": "",                   (string, required) coin symbol1
   "coinsymbol2": ""                    (string, required) coin symbol2
 }
```
**Response:**
```
 "result" :
 {
   "coinsymbol1": "",                   (string, required) coin symbol1
   "type1": "",                         (string, required) type1: SELL or BUY
   "coinsymbol2": "",                   (string, required) coin symbol2
   "type2": ""                          (string, required) type2: SELL or BUY
 }
```
**Examples:**
```
>> metabasenet-cli getdexsymboltype AAA BBB
<< {"coinsymbol1":"AAA","type1":"SELL","coinsymbol2":"BBB","type2":"BUY"}

>> curl -d '{"id":1,"method":"getdexsymboltype","jsonrpc":"2.0","params":{"coinsymbol1":"AAA","coinsymbol2":"BBB"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":{"coinsymbol1":"AAA","type1":"SELL","coinsymbol2":"BBB","type2":"BUY"}}]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid coinsymbol1"}
* {"code":-6,"message":"Invalid coinsymbol2"}
```
##### [Back to top](#commands)
---
### listrealtimedexorder
**Usage:**
```
        listrealtimedexorder <"coinsymbolsell"> <"coinsymbolbuy"> (-n=count) (-d|-nod*detail*) (-f="fork") (-h="block")

List realtime dex order
```
**Arguments:**
```
 "coinsymbolsell"                       (string, required) coin symbol sell
 "coinsymbolbuy"                        (string, required) coin symbol buy
 -n=count                               (uint, optional, default=1000) count, default is 1000
 -d|-nod*detail*                        (bool, optional, default=false) detail order, default is false
 -f="fork"                              (string, optional) fork hash or chainid
 -h="block"                             (string, optional) block hash
```
**Request:**
```
 "param" :
 {
   "coinsymbolsell": "",                (string, required) coin symbol sell
   "coinsymbolbuy": "",                 (string, required) coin symbol buy
   "count": 0,                          (uint, optional, default=1000) count, default is 1000
   "detail": true|false,                (bool, optional, default=false) detail order, default is false
   "fork": "",                          (string, optional) fork hash or chainid
   "block": ""                          (string, optional) block hash
 }
```
**Response:**
```
 "result" :
 {
   "coinsymbolsell": "",                (string, required) coin symbol sell
   "coinsymbolbuy": "",                 (string, required) coin symbol buy
   "prevcompleteprice": "",             (string, required) previous complete price
   "sellchainid": 0,                    (uint, required) sell chain id
   "buychainid": 0,                     (uint, required) buy chain id
   "maxmatchheight": 0,                 (uint, required) max match height
   "maxmatchslot": 0,                   (uint, required) max match slot
   "sellorder":                         (array, required, default=RPCValid) sell order
   [
     {
       "price": "",                     (string, required) price
       "orderamount": "",               (string, required) order amount
       "dealamount": "",                (string, required) deal amount
       "orderaddress": "",              (string, optional) order address
       "ordernumber": 0,                (uint, optional) order number
       "oriorderamount": "",            (string, optional) original order amount
       "height": 0,                     (uint, optional) at height
       "slot": 0                        (uint, optional) at slot
     }
   ]
   "buyorder":                          (array, required, default=RPCValid) buy order
   [
     {
       "price": "",                     (string, required) price
       "orderamount": "",               (string, required) order amount
       "dealamount": "",                (string, required) deal amount
       "orderaddress": "",              (string, optional) order address
       "ordernumber": 0,                (uint, optional) order number
       "oriorderamount": "",            (string, optional) original order amount
       "height": 0,                     (uint, optional) at height
       "slot": 0                        (uint, optional) at slot
     }
   ]
 }
```
**Examples:**
```
>> metabasenet-cli listrealtimedexorder AAA BBB
<< {"coinsymbolsell":"AAA","coinsymbolbuy":"BBB","prevcompleteprice":"1.2","sellorder":[{"price":"1.2","orderamount":"66.666666666666666667","dealamount":"80.0"}],"buyorder":[]}

>> curl -d '{"id":1,"method":"listrealtimedexorder","jsonrpc":"2.0","params":{"coinsymbolsell":"AAA","coinsymbolbuy":"BBB"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":{"coinsymbolsell":"AAA","coinsymbolbuy":"BBB","prevcompleteprice":"1.2","sellorder":[{"price":"1.2","orderamount":"66.666666666666666667","dealamount":"80.0"}],"buyorder":[]}}]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid coinsymbolsell"}
* {"code":-6,"message":"Invalid coinsymbolbuy"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-7,"message":"Query fail"}
```
##### [Back to top](#commands)
---
### sendcrosstransfertx
**Usage:**
```
        sendcrosstransfertx <"address"> <peerchainid> <"amount"> (-f="fork")

Send crosschain transfer transaction
```
**Arguments:**
```
 "address"                              (string, required) transfer address
 peerchainid                            (uint, required) peer chainid
 "amount"                               (string, required) transfer amount (big float)
 -f="fork"                              (string, optional) fork hash or chainid
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) transfer address
   "peerchainid": 0,                    (uint, required) peer chainid
   "amount": "",                        (string, required) transfer amount (big float)
   "fork": ""                           (string, optional) fork hash or chainid
 }
```
**Response:**
```
 "result": "result"                     (string, required) transaction hash
```
**Examples:**
```
>> metabasenet-cli sendcrosstransfertx 0xcc963a4ca2a9032e41668756cc4ccb7c59f112be 201 33.67
<< 0x45b154d2ab82eac6e79b8babd97abaebe2b4ae7194f3afd946dd1419f012dd3f

>> curl -d '{"id":1,"method":"sendcrosstransfertx","jsonrpc":"2.0","params":{"address":"0xcc963a4ca2a9032e41668756cc4ccb7c59f112be","peerchainid":201,"amount":"33.67"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":"0x45b154d2ab82eac6e79b8babd97abaebe2b4ae7194f3afd946dd1419f012dd3f"}]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid address"}
* {"code":-6,"message":"Invalid peerchainid"}
* {"code":-6,"message":"Invalid amount"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### getcrosstransferamount
**Usage:**
```
        getcrosstransferamount (-f="fork") (-h="block")

Get crosschain transfer amount
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash or chainid
 -h="block"                             (string, optional) block hash
```
**Request:**
```
 "param" :
 {
   "fork": "",                          (string, optional) fork hash or chainid
   "block": ""                          (string, optional) block hash
 }
```
**Response:**
```
 "result": "result"                     (string, required) transfer amount (big float)
```
**Examples:**
```
>> metabasenet-cli getcrosstransferamount
<< 8845.987

>> curl -d '{"id":1,"method":"sendcrosstransfertx","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":"8845.987"}]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### listaddress
**Usage:**
```
        listaddress (-p=page) (-n=count)

List all of the addresses from pub keys and template ids
```
**Arguments:**
```
 -p=page                                (uint, optional, default=0) page, default is 0
 -n=count                               (uint, optional, default=30) count, default is 30
```
**Request:**
```
 "param" :
 {
   "page": 0,                           (uint, optional, default=0) page, default is 0
   "count": 0                           (uint, optional, default=30) count, default is 30
 }
```
**Response:**
```
 "result" :
   "addressdata":                       (array, required, default=RPCValid) 
   [
     {
       "type": "",                      (string, required) type, pubkey or template
       "address": "",                   (string, required) public key or template address
       (if type=pubkey)
       "pubkey": "",                    (string, required) public key
       (if type=template)
       "template": "",                  (string, required) template type name
       (if type=template)
       "templatedata":                  (object, required) template data
       {
         "type": "",                    (string, required) template type
         "hex": "",                     (string, required) temtplate data
         (if type=delegate)
         "delegate":                    (object, required) delegate template struct
         {
           "mint": "",                  (string, required) mint address
           "owner": "",                 (string, required) owner address
           "rewardratio": 0             (uint, required) reward ratio, range: 0~10000
         }
         (if type=vote)
         "vote":                        (object, required) vote template struct
         {
           "delegate": "",              (string, required) delegate template address
           "owner": "",                 (string, required) owner address
           "rewardmode": 0              (int, required) reward mode, 0: reward vote address, 1: reward owner address
         }
         (if type=pledge)
         "pledge":                      (object, required) a pledge template
         {
           "delegate": "",              (string, required) delegate template address
           "owner": "",                 (string, required) owner address
           "pledgetype": 0,             (int, required) pledge type
           "cycles": 0,                 (int, required) cycles
           "nonce": 0                   (int, required) nonce
         }
         (if type=fork)
         "fork":                        (object, required) fork template struct
         {
           "redeem": "",                (string, required) redeem address
           "fork": ""                   (string, required) fork hash
         }
         (if type=mint)
         "mint":                        (object, required) mint template struct
         {
           "mint": "",                  (string, required) mint public key
           "spent": ""                  (string, required) spent address
         }
         (if type=activatecode)
         "activatecode":                (object, required) a activatecode template
         {
           "grant": ""                  (string, required) grant address
         }
       }
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listaddress
<< [{"type":"pubkey","address":"0xcc963a4ca2a9032e41668756cc4ccb7c59f112be","pubkey":"0x0c5a8805c62f6fc44d1a3bf81107e8c22372c823b875c5a31af55b4951dcc3ac27a6c2b5e867077cd235439cbf8cea28d475401560d0c6e38013b72b307b0941"},{"type":"pubkey","address":"0x5962974eeb0b17b43edabfc9b747839317aa852f","pubkey":"0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765"}]

>> curl -d '{"id":1,"method":"listaddress","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":[{"type":"pubkey","address":"0xcc963a4ca2a9032e41668756cc4ccb7c59f112be","pubkey":"0x0c5a8805c62f6fc44d1a3bf81107e8c22372c823b875c5a31af55b4951dcc3ac27a6c2b5e867077cd235439cbf8cea28d475401560d0c6e38013b72b307b0941"},{"type":"pubkey","address":"0x5962974eeb0b17b43edabfc9b747839317aa852f","pubkey":"0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765"}]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### exportwallet
**Usage:**
```
        exportwallet <"path">

Export all of keys and templates from wallet to a specified file in JSON format.
```
**Arguments:**
```
 "path"                                 (string, required) save file path
```
**Request:**
```
 "param" :
 {
   "path": ""                           (string, required) save file path
 }
```
**Response:**
```
 "result": "result"                     (string, required) export result
```
**Examples:**
```
>> metabasenet-cli exportwallet /Users/Loading/a.txt
<< Wallet file has been saved at: /Users/Loading/a.txt

>> {"id":4,"method":"exportwallet","jsonrpc":"2.0","params":{"path":"/Users/Loading/a.txt"}}
<< {"id":4,"jsonrpc":"2.0","result":"Wallet file has been saved at: /Users/Loading/a.txt"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid template address"}
* {"code":-401,"message":"Must be an absolute path."}
* {"code":-401,"message":"Cannot export to a folder."}
* {"code":-401,"message":"File has been existed."}
* {"code":-401,"message":"Failed to create directories."}
* {"code":-401,"message":"Failed to export key"}
* {"code":-401,"message":"Unkown template"}
* {"code":-401,"message":"filesystem_error"}
```
##### [Back to top](#commands)
---
### importwallet
**Usage:**
```
        importwallet <"path">

Import keys and templates from an archived file to the wallet in JSON format.
```
**Arguments:**
```
 "path"                                 (string, required) save file path
```
**Request:**
```
 "param" :
 {
   "path": ""                           (string, required) save file path
 }
```
**Response:**
```
 "result": "result"                     (string, required) export result
```
**Examples:**
```
>> metabasenet-cli importwallet /Users/Loading/a.txt
<< Imported 0 keys and 0 templates.

>> {"id":5,"method":"importwallet","jsonrpc":"2.0","params":{"path":"/Users/Loading/a.txt"}}
<< {"id":5,"jsonrpc":"2.0","result":"Imported 0 keys and 0 templates."}
```
**Errors:**
```
* {"code":-6,"message":"Invalid parameters,failed to make template"}
* {"code":-401,"message":"Must be an absolute path."}
* {"code":-401,"message":"File has been existed."}
* {"code":-401,"message":"Filesystem_error - failed to read."}
* {"code":-401,"message":"Wallet file exported is invalid, check it and try again."}
* {"code":-401,"message":"Data format is not correct, check it and try again."}
* {"code":-401,"message":"Failed to add key"}
* {"code":-401,"message":"Failed to sync wallet tx"}
* {"code":-401,"message":"Failed to add template"}
* {"code":-32602,"message":"Failed to verify serialized key"}
* {"code":-32602,"message":"Can't import the key with empty passphrase"}
```
##### [Back to top](#commands)
---
### makeorigin
**Usage:**
```
        makeorigin <"type"> <"prev"> <"owner"> <"amount"> <"name"> <"symbol"> <chainid> <"reward"> <halvecycle> (-n=nonce)

Return hex-encoded block.
```
**Arguments:**
```
 "type"                                 (string, required) fork type: clonemap, user
 "prev"                                 (string, required) prev block hash
 "owner"                                (string, required) owner address
 "amount"                               (string, required) amount (big float)
 "name"                                 (string, required) unique fork name
 "symbol"                               (string, required) fork symbol
 chainid                                (uint, required) chain id
 "reward"                               (string, required) mint reward (big float)
 halvecycle                             (uint, required) halve cycle: 0: fixed reward, >0: blocks of halve cycle
 -n=nonce                               (uint, optional, default=0) fork nonce
```
**Request:**
```
 "param" :
 {
   "type": "",                          (string, required) fork type: clonemap, user
   "prev": "",                          (string, required) prev block hash
   "owner": "",                         (string, required) owner address
   "amount": "",                        (string, required) amount (big float)
   "name": "",                          (string, required) unique fork name
   "symbol": "",                        (string, required) fork symbol
   "chainid": 0,                        (uint, required) chain id
   "reward": "",                        (string, required) mint reward (big float)
   "halvecycle": 0,                     (uint, required) halve cycle: 0: fixed reward, >0: blocks of halve cycle
   "nonce": 0                           (uint, optional, default=0) fork nonce
 }
```
**Response:**
```
 "result" :
 {
   "hash": "",                          (string, required) block hash
   "hex": ""                            (string, required) block data hex
 }
```
**Examples:**
```
>> metabasenet-cli makeorigin user 0xc907f7b8e50ea811dce40844ef9435e60d59edd4a9cb0b209289e3c623b8cb68 0x5962974eeb0b17b43edabfc9b747839317aa852f 300000000 test-fork tfs 208 200 0
<< {"hash":"0xca95f7efe0d568dd59d98e0423f80ce292d29590e5b98fb4e059d1e69aaeb288","hex":"0x0111fe5e44bf640038093d54d5c5b2ba6120e3e76f3c647c7c5d2497d85120837bd8345d05000000001e06865d7b088885d75f64c9d73d11d3dea6f6145016c9b5b63773929a0eb33800000000010168010000000309746573742d666f726b03746673d00bf8277896582678ac000000090ad78ebc5ac6200000074a9b6384488000002f85aa17938347b7c9bfda3eb4170beb4e976259553c2e6b595cee0477b24a7228d9ef0679911d438523404487f6746f000000000501d00000142f85aa17938347b7c9bfda3eb4170beb4e9762590bf8277896582678ac000000000002110220001520000000000000000000000000000000000000000000f8277896582678ac0000000000412e01171ce99e96e235fd29c49c916b8dd1f7b81ca8959623f95ab16b7b3935042ca42698d4068710767cbca3d254407a9514b419dc90dc86184b145d846da0aa01"}

>> curl -d '{"id":4,"method":"makeorigin","jsonrpc":"2.0","params":{"type":"user","prev":"0xc907f7b8e50ea811dce40844ef9435e60d59edd4a9cb0b209289e3c623b8cb68","owner":"0x5962974eeb0b17b43edabfc9b747839317aa852f","amount":"300000000","name":"test-fork","symbol":"tfs","reward":"200","chainid":208,"halvecycle":0}}' http://127.0.0.1:8812
<< {"id":4,"jsonrpc":"2.0","result":{"hash":"0xca95f7efe0d568dd59d98e0423f80ce292d29590e5b98fb4e059d1e69aaeb288","hex":"0x0111fe5e44bf640038093d54d5c5b2ba6120e3e76f3c647c7c5d2497d85120837bd8345d05000000001e06865d7b088885d75f64c9d73d11d3dea6f6145016c9b5b63773929a0eb33800000000010168010000000309746573742d666f726b03746673d00bf8277896582678ac000000090ad78ebc5ac6200000074a9b6384488000002f85aa17938347b7c9bfda3eb4170beb4e976259553c2e6b595cee0477b24a7228d9ef0679911d438523404487f6746f000000000501d00000142f85aa17938347b7c9bfda3eb4170beb4e9762590bf8277896582678ac000000000002110220001520000000000000000000000000000000000000000000f8277896582678ac0000000000412e01171ce99e96e235fd29c49c916b8dd1f7b81ca8959623f95ab16b7b3935042ca42698d4068710767cbca3d254407a9514b419dc90dc86184b145d846da0aa01"}}
```
**Errors:**
```
* {"code":-4,"message":"Owner' address should be pubkey address"}
* {"code":-4,"message":"Unknown key"}
* {"code":-6,"message":"Unknown prev block"}
* {"code":-6,"message":"Invalid owner"}
* {"code":-6,"message":"Invalid amount"}
* {"code":-6,"message":"Invalid name or symbol"}
* {"code":-6,"message":"Prev block should not be extended/vacant block"}
* {"code":-405,"message":"Key is locked"}
* {"code":-401,"message":"Failed to sign message"}
```
##### [Back to top](#commands)
---
### verifymessage
**Usage:**
```
        verifymessage <"pubkey"> <"message"> <"sig"> (-a="addr") (-p|-nop*hasprefix*)

Verify a signed message
```
**Arguments:**
```
 "pubkey"                               (string, required) public key
 "message"                              (string, required) message to be verified
 "sig"                                  (string, required) sign
 -a="addr"                              (string, optional) address signed
 -p|-nop*hasprefix*                     (bool, optional, default=true) If add magic string(MetabaseNet Signed Message:
) before message or not
```
**Request:**
```
 "param" :
 {
   "pubkey": "",                        (string, required) public key
   "message": "",                       (string, required) message to be verified
   "sig": "",                           (string, required) sign
   "addr": "",                          (string, optional) address signed
   "hasprefix": true|false              (bool, optional, default=true) If add magic string(MetabaseNet Signed Message:
) before message or not
 }
```
**Response:**
```
 "result": result                       (bool, required) message verified result.
```
**Examples:**
```
>> metabasenet-cli verifymessage 0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765 123456 0xadefe8db8e6f5f845366a520ed6f295e02f2e7e2440096c95f4db37cd40423f53b390213bce4a23a884b4868294222bf462edaf9e455d9fd1e65f231b832425901
<< true

>> curl -d '{"id":5,"method":"verifymessage","jsonrpc":"2.0","params":{"pubkey":"0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765","message":"123456","sig":"0xadefe8db8e6f5f845366a520ed6f295e02f2e7e2440096c95f4db37cd40423f53b390213bce4a23a884b4868294222bf462edaf9e455d9fd1e65f231b832425901"}}' http://127.0.0.1:8812
<< {"id":5,"jsonrpc":"2.0","result":true}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
* {"code":-6,"message":"Invalid message"}
* {"code":-6,"message":"Invalid sig"}
```
##### [Back to top](#commands)
---
### makekeypair
**Usage:**
```
        makekeypair

Make a public/private key pair.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result" :
 {
   "privkey": "",                       (string, required) private key
   "pubkey": "",                        (string, required) public key
   "address": ""                        (string, required) address
 }
```
**Examples:**
```
>> metabasenet-cli makekeypair
<< {"privkey":"0x131795caa7820310fe8fd06b514ec8a42a42c2512d978c8d1d992ff0ffe67bdf","pubkey":"0x0a2cd33b769ca5f06675ddb1d7707d323da5b270ce2c42ea6f09379cd43f2d03a156e217609e512718b322539d8f8e88fec085f54643488e6a7864fca3976fa1","address":"0x08df6d1917566dc74580086303351462091284fd"}

>> curl -d '{"id":42,"method":"makekeypair","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":42,"jsonrpc":"2.0","result":{"privkey":"0x131795caa7820310fe8fd06b514ec8a42a42c2512d978c8d1d992ff0ffe67bdf","pubkey":"0x0a2cd33b769ca5f06675ddb1d7707d323da5b270ce2c42ea6f09379cd43f2d03a156e217609e512718b322539d8f8e88fec085f54643488e6a7864fca3976fa1","address":"0x08df6d1917566dc74580086303351462091284fd"}}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### getpubkey
**Usage:**
```
        getpubkey <"privkey">

Return public key by address or private key.
```
**Arguments:**
```
 "privkey"                              (string, required) private key
```
**Request:**
```
 "param" :
 {
   "privkey": ""                        (string, required) private key
 }
```
**Response:**
```
 "result": "pubkey"                     (string, required) public key
```
**Examples:**
```
>> metabasenet-cli getpubkey 0xd8b8db5ef512bd8e5a04f83ccfcfe6470c83d3f2e31480828ecf93a9ac0a54e2
<< 06c4246621002576ec70545f04f2cb75378e3f1a16eca2c596fc1c64f52e122b

>> curl -d '{"id":44,"method":"getpubkey","jsonrpc":"2.0","params":{"privkey":"0xd8b8db5ef512bd8e5a04f83ccfcfe6470c83d3f2e31480828ecf93a9ac0a54e2"}}' http://127.0.0.1:8812
<< {"id":44,"jsonrpc":"2.0","result":"06c4246621002576ec70545f04f2cb75378e3f1a16eca2c596fc1c64f52e122b"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid privkey"}
```
##### [Back to top](#commands)
---
### getpubkeyaddress
**Usage:**
```
        getpubkeyaddress <"pubkey">

Return encoded address for the given public key.
```
**Arguments:**
```
 "pubkey"                               (string, required) public key
```
**Request:**
```
 "param" :
 {
   "pubkey": ""                         (string, required) public key
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of public key
```
**Examples:**
```
>> metabasenet-cli getpubkeyaddress 0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765
<< 0x5962974eeb0b17b43edabfc9b747839317aa852f

>> curl -d '{"id":44,"method":"getpubkeyaddress","jsonrpc":"2.0","params":{"pubkey":"0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765"}}' http://127.0.0.1:8812
<< {"id":44,"jsonrpc":"2.0","result":"0x5962974eeb0b17b43edabfc9b747839317aa852f"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
```
##### [Back to top](#commands)
---
### maketemplate
**Usage:**
```
        maketemplate <"type"> <{delegate}>|<{vote}>|<{pledge}>|<{fork}>|<{mint}>|<{activatecode}>

Return encoded address for the given template id.
```
**Arguments:**
```
 "type"                                 (string, required) template type
  (if type=delegate)
 {delegate}                             (object, required) a delegate template
  (if type=vote)
 {vote}                                 (object, required) a vote template
  (if type=pledge)
 {pledge}                               (object, required) a pledge template
  (if type=fork)
 {fork}                                 (object, required) a new fork template
  (if type=mint)
 {mint}                                 (object, required) a mint template
  (if type=activatecode)
 {activatecode}                         (object, required) a activatecode template
```
**Request:**
```
 "param" :
 {
   "type": "",                          (string, required) template type
   (if type=delegate)
   "delegate":                          (object, required) a delegate template
   {
     "mint": "",                        (string, required) mint address
     "owner": "",                       (string, required) owner address
     "rewardratio": 0                   (uint, required) reward ratio, range: 0~10000
   }
   (if type=vote)
   "vote":                              (object, required) a vote template
   {
     "delegate": "",                    (string, required) delegate template address
     "owner": "",                       (string, required) owner address
     "rewardmode": 0                    (int, required) reward mode, 0: reward vote address, 1: reward owner address
   }
   (if type=pledge)
   "pledge":                            (object, required) a pledge template
   {
     "delegate": "",                    (string, required) delegate template address
     "owner": "",                       (string, required) owner address
     "pledgetype": 0,                   (int, required) pledge type
     "cycles": 0,                       (int, required) cycles
     "nonce": 0                         (int, required) nonce
   }
   (if type=fork)
   "fork":                              (object, required) a new fork template
   {
     "redeem": "",                      (string, required) redeem address
     "fork": ""                         (string, required) fork hash
   }
   (if type=mint)
   "mint":                              (object, required) a mint template
   {
     "mint": "",                        (string, required) mint public key
     "spent": ""                        (string, required) spent address
   }
   (if type=activatecode)
   "activatecode":                      (object, required) a activatecode template
   {
     "grant": ""                        (string, required) grant address
   }
 }
```
**Response:**
```
 "result" :
 {
   "address": "",                       (string, required) address of template
   "hex": ""                            (string, required) template data hex string
 }
```
**Examples:**
```
>> metabasenet-cli maketemplate delegate '{"mint": "0x316a4d04650b0190377fc40c2b183992046f72c9", "owner": "0x5bc5c1726286ff0a8006b19312ca307210e0e658","rewardratio":500}'
<< 0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d

>> curl -d '{"id":1,"method":"maketemplate","jsonrpc":"2.0","params":{"type":"delegate","delegate":{"mint":"0x316a4d04650b0190377fc40c2b183992046f72c9","owner":"0x5bc5c1726286ff0a8006b19312ca307210e0e658","rewardratio":500}}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":"0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d"}

>> metabasenet-cli maketemplate fork '{"redeem":"0x5bc5c1726286ff0a8006b19312ca307210e0e658","fork":"0x00000079cae069a3dc43deb4c41600fdf1e457d6a807ee770921d1f88690f918"}'

>> metabasenet-cli maketemplate vote '{"delegate": "0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d", "owner": "0x0a9f6b9e0de14c2c9d02883904a69c7bee82c2a5", "rewardmode":0}'

>> metabasenet-cli maketemplate pledge '{"delegate": "0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d", "owner": "0x0a9f6b9e0de14c2c9d02883904a69c7bee82c2a5", "pledgetype":1, "cycles":1, "nonce":0}'
```
**Errors:**
```
* {"code":-6,"message":"Invalid parameters, failed to make template"}
* {"code":-6,"message":"Invalid parameter, missing redeem address"}
* {"code":-6,"message":"Invalid parameter, missing spent address"}
* {"code":-6,"message":"Invalid parameter, missing owner address"}
* {"code":-6,"message":"template type error. type: xxx"}
```
##### [Back to top](#commands)
---
### decodetransaction
**Usage:**
```
        decodetransaction <"txdata">

Return a JSON object representing the serialized, hex-encoded transaction.
```
**Arguments:**
```
 "txdata"                               (string, required) transaction raw data
```
**Request:**
```
 "param" :
 {
   "txdata": ""                         (string, required) transaction raw data
 }
```
**Response:**
```
 "result" :
 {
   "txid": "",                          (string, optional) transaction hash
   "type": "",                          (string, optional) transaction type
   "nonce": 0,                          (uint, optional) nonce
   "from": "",                          (string, optional) from address
   "to": "",                            (string, optional) to address
   "amount": "",                        (string, optional) amount (big float)
   "gaslimit": 0,                       (uint, optional) gas limit
   "gasprice": "",                      (string, optional) gas price (big float)
   "txfee": "",                         (string, optional) transaction limit fee (big float)
   "data": "",                          (string, optional) data
   "signhash": "",                      (string, optional) sign hash
   "sig": "",                           (string, optional) sign
   "fork": "",                          (string, optional) fork hash
   "height": 0,                         (int, optional) height
   "blockhash": "",                     (string, optional) which block tx located in
   "confirmations": 0                   (int, optional) confirmations
 }
```
**Examples:**
```
>> metabasenet-cli decodetransaction 0x120172f8701985e8d4a5100082530294b955034fcefb66112bab47483c8d243b86cb2c1d8a012d231c7c593eec0000808201b5a0a91f9091a641ad5ee6c6754e372dac948eb2e12db7143df3a63c5d8631019c1ca03630cf0aced70b5f75f62c607566bdc9ff257170c55a953b1417cb2178f1f93f
<< {"txid":"0x69350f7d89081c0ceb3ebb3f354840ab3f662a418140e386e7eb9e7b093987b4","type":"eth-message-call","nonce":25,"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0xb955034fcefb66112bab47483c8d243b86cb2c1d","amount":"5555.0","gaslimit":21250,"gasprice":"0.000001","txfee":"0.02125","data":"","signhash":"0xac577ca2e57688eed1de36c982136d7eab509b401dbc507ec7a70c5a0de099a6","sig":"0xa91f9091a641ad5ee6c6754e372dac948eb2e12db7143df3a63c5d8631019c1c3630cf0aced70b5f75f62c607566bdc9ff257170c55a953b1417cb2178f1f93f00","fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","height":0,"blockhash":""}

>> curl -d '{"id":1,"method":"decodetransaction","jsonrpc":"2.0","params":{"txdata":"0x120172f8701985e8d4a5100082530294b955034fcefb66112bab47483c8d243b86cb2c1d8a012d231c7c593eec0000808201b5a0a91f9091a641ad5ee6c6754e372dac948eb2e12db7143df3a63c5d8631019c1ca03630cf0aced70b5f75f62c607566bdc9ff257170c55a953b1417cb2178f1f93f"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":{"txid":"0x69350f7d89081c0ceb3ebb3f354840ab3f662a418140e386e7eb9e7b093987b4","type":"eth-message-call","nonce":25,"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0xb955034fcefb66112bab47483c8d243b86cb2c1d","amount":"5555.0","gaslimit":21250,"gasprice":"0.000001","txfee":"0.02125","data":"","signhash":"0xac577ca2e57688eed1de36c982136d7eab509b401dbc507ec7a70c5a0de099a6","sig":"0xa91f9091a641ad5ee6c6754e372dac948eb2e12db7143df3a63c5d8631019c1c3630cf0aced70b5f75f62c607566bdc9ff257170c55a953b1417cb2178f1f93f00","fork":"0x000000006f74f68744402385431d917906efd928724ab27704ee5c596b2e3c55","height":0,"blockhash":""}}
```
**Errors:**
```
* {"code":-8,"message":"TX decode failed"}
* {"code":-6,"message":"Unknown anchor block"}
```
##### [Back to top](#commands)
---
### gettxfee
**Usage:**
```
        gettxfee <"hexdata">

Return TxFee for vchData Hex data
```
**Arguments:**
```
 "hexdata"                              (string, required) Hex encoded vchData in transaction
```
**Request:**
```
 "param" :
 {
   "hexdata": ""                        (string, required) Hex encoded vchData in transaction
 }
```
**Response:**
```
 "result": "txfee"                      (string, required) transaction Fee (big float)
```
**Examples:**
```
>> metabasenet-cli gettxfee 0xfeeda13f124bbc
<< 0.0215

>> curl -d '{"id":1,"method":"gettxfee","jsonrpc":"2.0","params":{"hexdata":"0xfeeda13f124bbc"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":"0.0215"}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### makesha256
**Usage:**
```
        makesha256 (-h="hexdata")

Make sha256
```
**Arguments:**
```
 -h="hexdata"                           (string, optional) Hex encoded data
```
**Request:**
```
 "param" :
 {
   "hexdata": ""                        (string, optional) Hex encoded data
 }
```
**Response:**
```
 "result" :
 {
   "hexdata": "",                       (string, required) Hex encoded data
   "sha256": ""                         (string, required) Sha256 string
 }
```
**Examples:**
```
>> metabasenet-cli makesha256
<< {"hexdata" : "0x639100de22308c20cd9ae04b4047e8b2cda49f56e5018364d701f9102d1e438a", "sha256" : "0x2f6479763179938cddb1e6abd24e131792a1afa4e00c0201c55928b4e5302f73"}

>> curl -d '{"id":1,"method":"makesha256","jsonrpc":"2.0","params":{"hexdata":"54ec6b4a77eadc39207697e816e2d2cc6881263ecab078a4c770228e11cbff99"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":{"hexdata" : "0x639100de22308c20cd9ae04b4047e8b2cda49f56e5018364d701f9102d1e438a", "sha256" : "0x2f6479763179938cddb1e6abd24e131792a1afa4e00c0201c55928b4e5302f73"}}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### querystat
**Usage:**
```
        querystat <"type"> (-f="fork") (-b="begin") (-n=count)

Query statistical data
```
**Arguments:**
```
 "type"                                 (string, required) statistical type: maker: block maker, p2psyn: p2p synchronization
 -f="fork"                              (string, optional) fork hash (default all fork)
 -b="begin"                             (string, optional) begin time(HH:MM:SS) (default last count records)
 -n=count                               (uint, optional) get record count (default 20)
```
**Request:**
```
 "param" :
 {
   "type": "",                          (string, required) statistical type: maker: block maker, p2psyn: p2p synchronization
   "fork": "",                          (string, optional) fork hash (default all fork)
   "begin": "",                         (string, optional) begin time(HH:MM:SS) (default last count records)
   "count": 0                           (uint, optional) get record count (default 20)
 }
```
**Response:**
```
 "result": "stattable"                  (string, required) statistical data table
                                        1) maker: block maker
                                        -- time: statistical time, format: hh:mm:ss
                                        -- powblocks: number of POA blocks produced in one minute
                                        -- dposblocks: number of POS blocks produced in one minute
                                        -- tps: number of TX produced in one second
                                        2) p2psyn: p2p synchronization
                                        -- time: statistical time, format: hh:mm:ss
                                        -- recvblocks: number of synchronized receiving blocks in one minute
                                        -- recvtps: number of synchronized receiving TX in one second
                                        -- sendblocks: number of synchronized sending blocks in one minute
                                        -- sendtps: number of synchronized sending TX in one second
```
**Examples:**
```
>> metabasenet-cli querystat maker -b=09:11:00 -n=2
<< time      blocktps  tps  
09:11:59  1       12     
09:12:59  1       22     

>> curl -d '{"id":1,"method":"querystat","jsonrpc":"2.0","params":{"type":"maker","begin":"09:11:00","count":2}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":"time      blocktps  tps  
09:11:59  1       12     
09:12:59  1       22     "}
```
**Errors:**
```
* {"code" : -6, "message" : "Invalid type: is empty"}
* {"code" : -6, "message" : "Invalid type"}
* {"code" : -6, "message" : "Invalid fork"}
* {"code" : -6, "message" : "Invalid count"}
* {"code" : -32603, "message" : "query error"}
```
##### [Back to top](#commands)
---
### reversehex
**Usage:**
```
        reversehex <"hex">

Reverse a hex string by byte
```
**Arguments:**
```
 "hex"                                  (string, required) hex string
```
**Request:**
```
 "param" :
 {
   "hex": ""                            (string, required) hex string
 }
```
**Response:**
```
 "result": "hex"                        (string, required) reversed hex string
```
**Examples:**
```
>> metabasenet-cli reversehex 0xe8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9
<< 0xf9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8

>> curl -d '{"id":15,"method":"reversehex","jsonrpc":"2.0","params":{"hex":"0xe8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}}' http://127.0.0.1:8812
<< {"id":15,"jsonrpc":"2.0","result":"0xf9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8"}
```
**Errors:**
```
* {"code":-32602,"message":"hex string size is not even"}
* {"code":-32602,"message":"invalid hex string: g"}
```
##### [Back to top](#commands)
---
### callcontract
**Usage:**
```
        callcontract <"from"> <"to"> <"amount"> (-p="gasprice") (-g=gas) (-cp="contractparam") (-f="fork")

Call contract api
```
**Arguments:**
```
 "from"                                 (string, required) from address
 "to"                                   (string, required) to is contract address
 "amount"                               (string, required) amount (big float)
 -p="gasprice"                          (string, optional) gas price (big float)
 -g=gas                                 (uint, optional) gas
 -cp="contractparam"                    (string, optional) contract param
 -f="fork"                              (string, optional) fork hash (default genesis fork)
```
**Request:**
```
 "param" :
 {
   "from": "",                          (string, required) from address
   "to": "",                            (string, required) to is contract address
   "amount": "",                        (string, required) amount (big float)
   "gasprice": "",                      (string, optional) gas price (big float)
   "gas": 0,                            (uint, optional) gas
   "contractparam": "",                 (string, optional) contract param
   "fork": ""                           (string, optional) fork hash (default genesis fork)
 }
```
**Response:**
```
 "result" :
 {
   "status": 0,                         (int, required) call status, 0 is success, other is fail
   "result": ""                         (string, required) result data
 }
```
**Examples:**
```
>> metabasenet-cli callcontract 0x5962974eeb0b17b43edabfc9b747839317aa852f 0x06963e6599ae8a485491a9008bf01c778bdf80c2 0 -cp=0x8d4b7932
<< {"status":0,"result":"0x00000000000000000000000000000000000000000000000029a2241af62c0000"}

>> curl -d '{"id":3,"method":"callcontract","jsonrpc":"2.0","params":{"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0x06963e6599ae8a485491a9008bf01c778bdf80c2","amount":"0.0000000000","contractparam":"0x8d4b7932"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"{"status":0,"result":"0x00000000000000000000000000000000000000000000000029a2241af62c0000"}"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid from address"}
* {"code":-6,"message":"Invalid to address"}
* {"code":-6,"message":"Invalid amount"}
* {"code":-6,"message":"Invalid gasprice"}
* {"code":-32603,"message":"Call fail"}
```
##### [Back to top](#commands)
---
### gettransactionreceipt
**Usage:**
```
        gettransactionreceipt <"txid"> (-f="fork")

Get transaction receipt
```
**Arguments:**
```
 "txid"                                 (string, required) transaction id
 -f="fork"                              (string, optional) fork hash or chainid (default local fork)
```
**Request:**
```
 "param" :
 {
   "txid": "",                          (string, required) transaction id
   "fork": ""                           (string, optional) fork hash or chainid (default local fork)
 }
```
**Response:**
```
 "result" :
 {
   "transactionHash": "",               (string, optional) transaction hash
   "transactionIndex": 0,               (uint, optional) transaction index
   "blockHash": "",                     (string, optional) block hash
   "blockNumber": 0,                    (uint, optional) block number
   "from": "",                          (string, optional) tx from
   "to": "",                            (string, optional) tx to
   "cumulativeGasUsed": 0,              (uint, optional) cumulative gas used
   "effectiveGasPrice": "",             (string, optional) effective gas price (big float)
   "effectiveGasFee": "",               (string, optional) effective gas fee (big float)
   "gasUsed": 0,                        (uint, optional) gas used
   "contractAddress": "",               (string, optional) contract address
   "root": "",                          (string, optional) root
   "status": "",                        (string, optional) status, 0x1: ok, 0x0: fail
   "logsBloom": "",                     (string, optional) logs bloom
   "logs":                              (array, optional) 
   [
     {
       "removed": true|false,           (bool, optional) removed
       "logIndex": 0,                   (uint, optional) log index
       "transactionIndex": 0,           (uint, optional) transaction index
       "transactionHash": "",           (string, optional) transaction hash
       "blockHash": "",                 (string, optional) block hash
       "blockNumber": 0,                (uint, optional) block number
       "address": "",                   (string, optional) address
       "data": "",                      (string, optional) data
       "topics":                        (array, optional) topic list
       [
         "topic": ""                    (string, required) topic
       ]
       "type": "",                      (string, optional) type
       "id": ""                         (string, optional) id
     }
   ]
   "internaltx":                        (array, optional) internal transfer list
   [
     {
       "type": "",                      (string, optional) transfer type
       "from": "",                      (string, optional) from address
       "to": "",                        (string, optional) to address
       "amount": ""                     (string, optional) transfer amount (big float)
     }
   ]
 }
```
**Examples:**
```
>> metabasenet-cli gettransactionreceipt 0x69350f7d89081c0ceb3ebb3f354840ab3f662a418140e386e7eb9e7b093987b4
<< {"transactionHash":"0x69350f7d89081c0ceb3ebb3f354840ab3f662a418140e386e7eb9e7b093987b4","transactionIndex":11,"blockHash":"0x000016c4eae1e0730e4a428aba13b90f9229f5d0c317f0dc44630ef7e465e781","blockNumber":5828,"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0xb955034fcefb66112bab47483c8d243b86cb2c1d","cumulativeGasUsed":21000,"effectiveGasPrice":"0.000001","effectiveGasFee":"0.021","gasUsed":21000,"status":"0x1","logsBloom":"","logs":[{}],"internaltx":[{}]}

>> curl -d '{"id":3,"method":"gettransactionreceipt","jsonrpc":"2.0","params":{"txid":"0x69350f7d89081c0ceb3ebb3f354840ab3f662a418140e386e7eb9e7b093987b4"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"{"transactionHash":"0x69350f7d89081c0ceb3ebb3f354840ab3f662a418140e386e7eb9e7b093987b4","transactionIndex":11,"blockHash":"0x000016c4eae1e0730e4a428aba13b90f9229f5d0c317f0dc44630ef7e465e781","blockNumber":5828,"from":"0x5962974eeb0b17b43edabfc9b747839317aa852f","to":"0xb955034fcefb66112bab47483c8d243b86cb2c1d","cumulativeGasUsed":21000,"effectiveGasPrice":"0.000001","effectiveGasFee":"0.021","gasUsed":21000,"status":"0x1","logsBloom":"","logs":[{}],"internaltx":[{}]}"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid txid"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-7,"message":"Get transaction receipt fail"}
```
##### [Back to top](#commands)
---
### getcontractmuxcode
**Usage:**
```
        getcontractmuxcode <"muxtype"> <"name"> <"owner"> <"code"> (-t="type") (-d="describe") (-s="source")

Get contract mux code, including source code, name and code
```
**Arguments:**
```
 "muxtype"                              (string, required) dux type: create, setup, upcode
 "name"                                 (string, required) contract name, non empty
 "owner"                                (string, required) contract owner address, non empty
 "code"                                 (string, required) contract code, non empty
 -t="type"                              (string, optional) contract type, separated by '@' symbol, example: type1@type11@type111
 -d="describe"                          (string, optional) contract describe
 -s="source"                            (string, optional) contract source
```
**Request:**
```
 "param" :
 {
   "muxtype": "",                       (string, required) dux type: create, setup, upcode
   "name": "",                          (string, required) contract name, non empty
   "owner": "",                         (string, required) contract owner address, non empty
   "code": "",                          (string, required) contract code, non empty
   "type": "",                          (string, optional) contract type, separated by '@' symbol, example: type1@type11@type111
   "describe": "",                      (string, optional) contract describe
   "source": ""                         (string, optional) contract source
 }
```
**Response:**
```
 "result": "contractmuxhex"             (string, required) contract mux hex
```
**Examples:**
```
>> metabasenet-cli getcontractmuxcode create myname1 0x5962974eeb0b17b43edabfc9b747839317aa852f 'code' -t=type1 -d=describe1 -s='source code'
<< 0x579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8......

>> curl -d '{"id":3,"method":"getcontractmuxcode","jsonrpc":"2.0","params":{"muxtype":"create","name":"myname1","owner":"0x5962974eeb0b17b43edabfc9b747839317aa852f","code":"code1","type":"type1","describe":"describe1","source":"source1"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8......"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid muxtype"}
* {"code":-6,"message":"Invalid name"}
* {"code":-6,"message":"Invalid code"}
* {"code":-8,"message":"Packet fail"}
```
##### [Back to top](#commands)
---
### gettemplatemuxcode
**Usage:**
```
        gettemplatemuxcode <"name"> <"code">

Get template mux code
```
**Arguments:**
```
 "name"                                 (string, required) template name, non empty
 "code"                                 (string, required) template code, non empty
```
**Request:**
```
 "param" :
 {
   "name": "",                          (string, required) template name, non empty
   "code": ""                           (string, required) template code, non empty
 }
```
**Response:**
```
 "result": "templatemuxhex"             (string, required) template mux hex
```
**Examples:**
```
>> metabasenet-cli gettemplatemuxcode pos-xx 'code'
<< 0x579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8......

>> curl -d '{"id":3,"method":"gettemplatemuxcode","jsonrpc":"2.0","params":{"muxtype":"create","name":"pos-xx","code":"code1"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8......"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid muxtype"}
* {"code":-6,"message":"Invalid name"}
* {"code":-6,"message":"Invalid code"}
* {"code":-8,"message":"Packet fail"}
```
##### [Back to top](#commands)
---
### listcontractcode
**Usage:**
```
        listcontractcode (-c="codehash") (-t="txid") (-f="fork") (-b="block")

List contract code
```
**Arguments:**
```
 -c="codehash"                          (string, optional) code hash
 -t="txid"                              (string, optional) up txid
 -f="fork"                              (string, optional) fork
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "codehash": "",                      (string, optional) code hash
   "txid": "",                          (string, optional) up txid
   "fork": "",                          (string, optional) fork
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result" :
   "codedata":                          (array, required, default=RPCValid) 
   [
     {
       "codehash": "",                  (string, required) contract code hash
       "owner": "",                     (string, required) contract code owner address
       "txid": "",                      (string, required) txid of contract code created
       "sourcehash": "",                (string, optional) contract source hash
       "type": "",                      (string, optional) contract code type
       "name": "",                      (string, optional) contract code name
       "describe": ""                   (string, optional) contract code describe
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listcontractcode
<< [{"codehash":"0xaa3fa7bbad961c25b1083ee0bb15c1d8f5cf41bfa0a6fc9be56eb19e2057a65b","owner":"0xb955034fcefb66112bab47483c8d243b86cb2c1d","txid":"0x9d50297ade3ef3dc138ac505f7eb3db8334ac5db9ca33fa218554097c26b0f27"},{"codehash":"0xb2aa186a283a1ad3b250649eb5568ce3a65ca04109c33f53081ff4caff4970c7","owner":"0x00000000000000000000000000000000000000a1","txid":"0x0000000000000000000000000000000000000000000000000000000000000000"}]

>> curl -d '{"id":3,"method":"listcontractcode","jsonrpc":"2.0","params":{}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"[{"codehash":"0xaa3fa7bbad961c25b1083ee0bb15c1d8f5cf41bfa0a6fc9be56eb19e2057a65b","owner":"0xb955034fcefb66112bab47483c8d243b86cb2c1d","txid":"0x9d50297ade3ef3dc138ac505f7eb3db8334ac5db9ca33fa218554097c26b0f27"},{"codehash":"0xb2aa186a283a1ad3b250649eb5568ce3a65ca04109c33f53081ff4caff4970c7","owner":"0x00000000000000000000000000000000000000a1","txid":"0x0000000000000000000000000000000000000000000000000000000000000000"}]"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### listcontractaddress
**Usage:**
```
        listcontractaddress (-f="fork") (-b="block")

List contract address
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "fork": "",                          (string, optional) fork
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result" :
   "addressdata":                       (array, required, default=RPCValid) 
   [
     {
       "address": "",                   (string, required) contract address
       "owner": "",                     (string, required) contract owner address
       "txid": "",                      (string, required) txid of contract created
       "codehash": "",                  (string, required) contract code hash
       "type": "",                      (string, optional) contract type
       "name": "",                      (string, optional) contract name
       "describe": "",                  (string, optional) contract describe
       "sourcehash": ""                 (string, optional) contract source hash
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listcontractaddress
<< [{"address":"0x00000000000000000000000000000000000000a1","owner":"0x00000000000000000000000000000000000000a1","txid":"0x0000000000000000000000000000000000000000000000000000000000000000","codehash":"0xb2aa186a283a1ad3b250649eb5568ce3a65ca04109c33f53081ff4caff4970c7"},{"address":"0x06963e6599ae8a485491a9008bf01c778bdf80c2","owner":"0xb955034fcefb66112bab47483c8d243b86cb2c1d","txid":"0x9d50297ade3ef3dc138ac505f7eb3db8334ac5db9ca33fa218554097c26b0f27","codehash":"0xaa3fa7bbad961c25b1083ee0bb15c1d8f5cf41bfa0a6fc9be56eb19e2057a65b"}]

>> curl -d '{"id":3,"method":"listcontractaddress","jsonrpc":"2.0","params":{}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"[{"address":"0x00000000000000000000000000000000000000a1","owner":"0x00000000000000000000000000000000000000a1","txid":"0x0000000000000000000000000000000000000000000000000000000000000000","codehash":"0xb2aa186a283a1ad3b250649eb5568ce3a65ca04109c33f53081ff4caff4970c7"},{"address":"0x06963e6599ae8a485491a9008bf01c778bdf80c2","owner":"0xb955034fcefb66112bab47483c8d243b86cb2c1d","txid":"0x9d50297ade3ef3dc138ac505f7eb3db8334ac5db9ca33fa218554097c26b0f27","codehash":"0xaa3fa7bbad961c25b1083ee0bb15c1d8f5cf41bfa0a6fc9be56eb19e2057a65b"}]"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### getdestcontract
**Usage:**
```
        getdestcontract <"address"> (-f="fork") (-b="block")

Get address contract
```
**Arguments:**
```
 "address"                              (string, required) contract address
 -f="fork"                              (string, optional) fork
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) contract address
   "fork": "",                          (string, optional) fork
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result" :
 {
   "type": "",                          (string, required) contract type
   "name": "",                          (string, required) contract name
   "describe": "",                      (string, required) contract describe
   "txid": "",                          (string, required) txid of contract created
   "sourcehash": "",                    (string, required) contract source hash
   "codehash": ""                       (string, required) contract code hash
 }
```
**Examples:**
```
>> metabasenet-cli getdestcontract 3zfm64bkxqsfn10vhqgghj6hwcqaqhqcsg03gyf9nwba2brrf1h81egmk
<< {"type":"","name":"","describe":"","txid":"0x9d50297ade3ef3dc138ac505f7eb3db8334ac5db9ca33fa218554097c26b0f27","sourcehash":"0x0000000000000000000000000000000000000000000000000000000000000000","codehash":"0xaa3fa7bbad961c25b1083ee0bb15c1d8f5cf41bfa0a6fc9be56eb19e2057a65b"}

>> curl -d '{"id":3,"method":"getdestcontract","jsonrpc":"2.0","params":{"address":"3zfm64bkxqsfn10vhqgghj6hwcqaqhqcsg03gyf9nwba2brrf1h81egmk"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"{"type":"","name":"","describe":"","txid":"0x9d50297ade3ef3dc138ac505f7eb3db8334ac5db9ca33fa218554097c26b0f27","sourcehash":"0x0000000000000000000000000000000000000000000000000000000000000000","codehash":"0xaa3fa7bbad961c25b1083ee0bb15c1d8f5cf41bfa0a6fc9be56eb19e2057a65b"}"}
```
**Errors:**
```
* {"code":-4,"message":"Address error"}
* {"code":-6,"message":"Invalid address"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### getcontractsource
**Usage:**
```
        getcontractsource <"sourcehash"> (-f="fork") (-b="block")

Get contract source
```
**Arguments:**
```
 "sourcehash"                           (string, required) source hash
 -f="fork"                              (string, optional) fork
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "sourcehash": "",                    (string, required) source hash
   "fork": "",                          (string, optional) fork
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result": "contractsource"             (string, required) contract source
```
**Examples:**
```
>> metabasenet-cli getcontractsource 0x9358768717ec4837f96d73dbded55932b8865573bf88a6c5b2b5d8023d7fa2de
<< 0x707261676d6120736f6c6964697479205e302e352e303b0a0a636f6e7472616374206572633230207b0a20202f2f205361......

>> curl -d '{"id":3,"method":"getcontractsource","jsonrpc":"2.0","params":{"sourcehash":"0x9358768717ec4837f96d73dbded55932b8865573bf88a6c5b2b5d8023d7fa2de"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x707261676d6120736f6c6964697479205e302e352e303b0a0a636f6e7472616374206572633230207b0a20202f2f205361......"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid source"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### getcontractcode
**Usage:**
```
        getcontractcode <"codehash"> (-f="fork") (-b="block")

Get contract code
```
**Arguments:**
```
 "codehash"                             (string, required) code hash
 -f="fork"                              (string, optional) fork
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "codehash": "",                      (string, required) code hash
   "fork": "",                          (string, optional) fork
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result": "contractcode"               (string, required) contract code
```
**Examples:**
```
>> metabasenet-cli getcontractcode 0x579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8
<< 0x0061736d01000000014e0b60027f7f0060017f0060037f7f7f006000017e60047e7f7f7f017f60057f7e7e7e7e0060067f7e7e7......

>> curl -d '{"id":3,"method":"getcontractcode","jsonrpc":"2.0","params":{"codehash":"0x579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x0061736d01000000014e0b60027f7f0060017f0060037f7f7f006000017e60047e7f7f7f017f60057f7e7e7e7e0060067f7e7e7......"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid code"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### funcsign
**Usage:**
```
        funcsign <"funcname">

Function sign
```
**Arguments:**
```
 "funcname"                             (string, required) Function name
```
**Request:**
```
 "param" :
 {
   "funcname": ""                       (string, required) Function name
 }
```
**Response:**
```
 "result": "sign"                       (string, required) sing result
```
**Examples:**
```
>> metabasenet-cli funcsign add
<< 0x4f5a8bb8

>> curl -d '{"id":3,"method":"funcsign","jsonrpc":"2.0","params":{"funcname":"add"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x4f5a8bb8"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid funcname"}
```
##### [Back to top](#commands)
---
### makehash
**Usage:**
```
        makehash <"data">

Make hash
```
**Arguments:**
```
 "data"                                 (string, required) data(hex)
```
**Request:**
```
 "param" :
 {
   "data": ""                           (string, required) data(hex)
 }
```
**Response:**
```
 "result": "hash"                       (string, required) hash
```
**Examples:**
```
>> metabasenet-cli makehash 0x12345678
<< 0x97e86e044a53385b642a902fd8ed05534d7590412a608f43dbb70e1f0e3664c7

>> curl -d '{"id":3,"method":"makehash","jsonrpc":"2.0","params":{"data":"0x12345678"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x97e86e044a53385b642a902fd8ed05534d7590412a608f43dbb70e1f0e3664c7"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid data"}
```
##### [Back to top](#commands)
---
### addblacklistaddress
**Usage:**
```
        addblacklistaddress <"address">

Add blacklist address
```
**Arguments:**
```
 "address"                              (string, required) blacklist address
```
**Request:**
```
 "param" :
 {
   "address": ""                        (string, required) blacklist address
 }
```
**Response:**
```
 "result": result                       (bool, required) add result
```
**Examples:**
```
>> metabasenet-cli addblacklistaddress 0xea811cbcfa6c892340261072d9ecdc7008d28a2e
<< true

>> curl -d '{"id":3,"method":"addblacklistaddress","jsonrpc":"2.0","params":{"address":"0xea811cbcfa6c892340261072d9ecdc7008d28a2e"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":true}
```
**Errors:**
```
* {"code":-6,"message":"Invalid address"}
```
##### [Back to top](#commands)
---
### removeblacklistaddress
**Usage:**
```
        removeblacklistaddress <"address">

Remove blacklist address
```
**Arguments:**
```
 "address"                              (string, required) blacklist address
```
**Request:**
```
 "param" :
 {
   "address": ""                        (string, required) blacklist address
 }
```
**Response:**
```
 "result": result                       (bool, required) remove result
```
**Examples:**
```
>> metabasenet-cli removeblacklistaddress 0xea811cbcfa6c892340261072d9ecdc7008d28a2e
<< true

>> curl -d '{"id":3,"method":"removeblacklistaddress","jsonrpc":"2.0","params":{"address":"0xea811cbcfa6c892340261072d9ecdc7008d28a2e"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":true}
```
**Errors:**
```
* {"code":-6,"message":"Invalid address"}
```
##### [Back to top](#commands)
---
### listblacklistaddress
**Usage:**
```
        listblacklistaddress

List blacklist address
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result" :
   "address":                           (array, required, default=RPCValid) 
   [
     "address": ""                      (string, required) blacklist address
   ]
```
**Examples:**
```
>> metabasenet-cli listblacklistaddress
<< ["0xea811cbcfa6c892340261072d9ecdc7008d28a2e"]

>> curl -d '{"id":3,"method":"listblacklistaddress","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":["0xea811cbcfa6c892340261072d9ecdc7008d28a2e"]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### setfunctionaddress
**Usage:**
```
        setfunctionaddress <funcid> <"newaddress"> (-d|-nod*disablemodify*)

Set function address
```
**Arguments:**
```
 funcid                                 (uint, required) function id
 "newaddress"                           (string, required) new address
 -d|-nod*disablemodify*                 (bool, optional, default=false) if disable modify
```
**Request:**
```
 "param" :
 {
   "funcid": 0,                         (uint, required) function id
   "newaddress": "",                    (string, required) new address
   "disablemodify": true|false          (bool, optional, default=false) if disable modify
 }
```
**Response:**
```
 "result": "txid"                       (string, required) transaction id
```
**Examples:**
```
>> metabasenet-cli setfunctionaddress 1 0xf7d47e4e74084441d5bf3b6d8a9dd6e59d151115
<< 0xc175947a6265c969165efa03170b35a1fca3d4c9d424c012648bf9250b4ce5ed

>> curl -d '{"id":3,"method":"setfunctionaddress","jsonrpc":"2.0","params":{"funcid":1,"newaddress":"0xf7d47e4e74084441d5bf3b6d8a9dd6e59d151115"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0xc175947a6265c969165efa03170b35a1fca3d4c9d424c012648bf9250b4ce5ed"}
```
**Errors:**
```
* {"code":-32600,"message":"Only suitable for the main chain"}
* {"code":-6,"message":"Invalid from"}
* {"code":-6,"message":"Invalid funcid"}
* {"code":-6,"message":"Invalid newaddress"}
* {"code":-4,"message":"Default address not import"}
* {"code":-401,"message":"Default address is locked"}
* {"code":-9"message":"Verify tx fail"}
* {"code":-10"message":"Rejected modify"}
```
##### [Back to top](#commands)
---
### listfunctionaddress
**Usage:**
```
        listfunctionaddress (-b="block")

List function address
```
**Arguments:**
```
 -b="block"                             (string, optional) block hash or number or latest (default latest block)
```
**Request:**
```
 "param" :
 {
   "block": ""                          (string, optional) block hash or number or latest (default latest block)
 }
```
**Response:**
```
 "result" :
   "functionaddresslist":               (array, required, default=RPCValid) 
   [
     {
       "funcid": 0,                     (uint, required) function id
       "funcname": "",                  (string, required) function name
       "defaultaddress": "",            (string, required) default function address
       "funcaddress": "",               (string, required) function address
       "disablemodify": true|false      (bool, required) if disable modify
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listfunctionaddress
<< [{"funcid":1,"funcname":"Pledge surplus reward address","defaultaddress":"0x420757b308d1273215ddd5e8dfea1802e2983245","funcaddress":"0xf7d47e4e74084441d5bf3b6d8a9dd6e59d151115","disablemodify":false}]

>> curl -d '{"id":3,"method":"listfunctionaddress","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":[{"funcid":1,"funcname":"Pledge surplus reward address","defaultaddress":"0x420757b308d1273215ddd5e8dfea1802e2983245","funcaddress":"0xf7d47e4e74084441d5bf3b6d8a9dd6e59d151115","disablemodify":false}]}
```
**Errors:**
```
* {"code":-32600,"message":"Only suitable for the main chain"}
```
##### [Back to top](#commands)
---
### setmintmingasprice
**Usage:**
```
        setmintmingasprice <"fork"> <"mingasprice">

Set mint min gas price
```
**Arguments:**
```
 "fork"                                 (string, required) fork id
 "mingasprice"                          (string, required) min gas price
```
**Request:**
```
 "param" :
 {
   "fork": "",                          (string, required) fork id
   "mingasprice": ""                    (string, required) min gas price
 }
```
**Response:**
```
 "result": result                       (bool, required) set result
```
**Examples:**
```
>> metabasenet-cli setmintmingasprice 0x000000152e03037902d6bdc1d10b29b4a940a2972dfe06976ebcc8c4353cc7d0 0.0000001
<< true

>> curl -d '{"id":3,"method":"setmintmingasprice","jsonrpc":"2.0","params":{"fork":"0x000000152e03037902d6bdc1d10b29b4a940a2972dfe06976ebcc8c4353cc7d0","mingasprice":"0.0000001"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":true}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid mingasprice"}
```
##### [Back to top](#commands)
---
### getmintmingasprice
**Usage:**
```
        getmintmingasprice <"fork">

Get mint min gas price
```
**Arguments:**
```
 "fork"                                 (string, required) fork id
```
**Request:**
```
 "param" :
 {
   "fork": ""                           (string, required) fork id
 }
```
**Response:**
```
 "result": "MinGasPrice"                (string, required) mint min gas price
```
**Examples:**
```
>> metabasenet-cli getmintmingasprice 0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d
<< 0.00001

>> curl -d '{"id":3,"method":"getmintmingasprice","jsonrpc":"2.0","params":{"fork":"0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0.00001"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### listmintmingasprice
**Usage:**
```
        listmintmingasprice

List mint min gas price
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result" :
   "forkprice":                         (array, required, default=RPCValid) 
   [
     {
       "fork": "",                      (string, required) fork id
       "mingasprice": ""                (string, required) min gas price
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listmintmingasprice
<< [{"fork":"0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d","mingasprice":"0.0000001"}]

>> curl -d '{"id":3,"method":"listmintmingasprice","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":[{"fork":"0xc9f7f70be6841102645505f96cdc28f00570d22c64d50dbfa3e44632e6ae3d5d","mingasprice":"0.0000001"}]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### checkatbloomfilter
**Usage:**
```
        checkatbloomfilter <"bloomfilter"> <"checkdata">

Check if the data is in bloomfilter
```
**Arguments:**
```
 "bloomfilter"                          (string, required) bloomfilter data (HEX)
 "checkdata"                            (string, required) check data (HEX)
```
**Request:**
```
 "param" :
 {
   "bloomfilter": "",                   (string, required) bloomfilter data (HEX)
   "checkdata": ""                      (string, required) check data (HEX)
 }
```
**Response:**
```
 "result": result                       (bool, required) check result
```
**Examples:**
```
>> metabasenet-cli checkatbloomfilter 0x7959ff45dffab7bdfbebfb8f9573a1f2dd7fbec3ff57 0x633d2efebbef31e5853b7121bc3698f1239e931e9cd64e9cd37cdf5bd2048474
<< true

>> curl -d '{"id":3,"method":"checkatbloomfilter","jsonrpc":"2.0","params":{"bloomfilter":"0x7959ff45dffab7bdfbebfb8f9573a1f2dd7fbec3ff57","checkdata":"0x633d2efebbef31e5853b7121bc3698f1239e931e9cd64e9cd37cdf5bd2048474"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":true}
```
**Errors:**
```
* {"code":-6,"message":"Invalid bloomfilter"}
* {"code":-6,"message":"Invalid checkdata"}
```
##### [Back to top](#commands)
---
### web3_clientVersion
**Usage:**
```
        web3_clientVersion <[paramlist]>

[ETH] Get web3 client version
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "result"                     (string, required) sha3 result
```
**Examples:**
```
>> metabasenet-cli web3_clientVersion []
<< ["v0.2.1"]

>> curl -d '{"id":3,"method":"web3_clientVersion","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":["v0.2.1"]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### web3_sha3
**Usage:**
```
        web3_sha3 <[paramlist]>

[ETH] Keccak-256
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "result"                     (string, required) sha3 result
```
**Examples:**
```
>> metabasenet-cli web3_sha3 ["0x68656c6c6f20776f726c64"]
<< ["0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad"]

>> curl -d '{"id":3,"method":"web3_sha3","jsonrpc":"2.0","params":["0x68656c6c6f20776f726c64"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":["0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad"]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### net_version
**Usage:**
```
        net_version <[paramlist]>

[ETH] Get net version
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "result"                     (string, required) net version
```
**Examples:**
```
>> metabasenet-cli net_version []
<< ["10"]

>> curl -d '{"id":3,"method":"net_version","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":["10"]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### net_listening
**Usage:**
```
        net_listening <[paramlist]>

[ETH] Get net listen status
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": result                       (bool, required) net listen status
```
**Examples:**
```
>> metabasenet-cli net_listening []
<< [true]

>> curl -d '{"id":3,"method":"net_listening","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":[true]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### net_peerCount
**Usage:**
```
        net_peerCount <[paramlist]>

[ETH] Get connect peer node count
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "result"                     (string, required) peer node count
```
**Examples:**
```
>> metabasenet-cli net_peerCount []
<< ["2"]

>> curl -d '{"id":3,"method":"net_peerCount","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":["2"]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### eth_chainId
**Usage:**
```
        eth_chainId <[paramlist]>

[ETH] Get chain id
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "result"                     (string, required) chain id
```
**Examples:**
```
>> metabasenet-cli eth_chainId []
<< ["0x1"]

>> curl -d '{"id":3,"method":"eth_chainId","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":["0x1"]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### eth_protocolVersion
**Usage:**
```
        eth_protocolVersion <[paramlist]>

[ETH] Get protocol version
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "result"                     (string, required) protocol version
```
**Examples:**
```
>> metabasenet-cli eth_protocolVersion []
<< ["10"]

>> curl -d '{"id":3,"method":"eth_protocolVersion","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":["10"]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### eth_syncing
**Usage:**
```
        eth_syncing <[paramlist]>

[ETH] If syncing
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result" :
 {
   "startingBlock": "",                 (string, required) Start block number
   "currentBlock": "",                  (string, required) Current block number
   "highestBlock": ""                   (string, required) Highest block number
 }
```
**Examples:**
```
>> metabasenet-cli eth_syncing []
<< ["10"]

>> curl -d '{"id":3,"method":"eth_syncing","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":["10"]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### eth_coinbase
**Usage:**
```
        eth_coinbase <[paramlist]>

[ETH] Get coinbase
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "result"                     (string, required) coinbase address
```
**Examples:**
```
>> metabasenet-cli eth_coinbase []
<< ["10"]

>> curl -d '{"id":3,"method":"eth_coinbase","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":["10"]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### eth_mining
**Usage:**
```
        eth_mining <[paramlist]>

[ETH] If mining
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": result                       (bool, required) if mining
```
**Examples:**
```
>> metabasenet-cli eth_mining []
<< true

>> curl -d '{"id":3,"method":"eth_mining","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":true}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### eth_hashrate
**Usage:**
```
        eth_hashrate <[paramlist]>

[ETH] Get hash rate
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "result"                     (string, required) hash rate
```
**Examples:**
```
>> metabasenet-cli eth_hashrate []
<< 0x0

>> curl -d '{"id":3,"method":"eth_hashrate","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x0"}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### eth_gasPrice
**Usage:**
```
        eth_gasPrice <[paramlist]>

[ETH] Get gas price
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "result"                     (string, required) gas price
```
**Examples:**
```
>> metabasenet-cli eth_gasPrice []
<< 0x09184e72a000

>> curl -d '{"id":3,"method":"eth_gasPrice","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x09184e72a000"}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### eth_accounts
**Usage:**
```
        eth_accounts <[paramlist]>

[ETH] Get wallet accounts
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result" :
   "result":                            (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) account address
   ]
```
**Examples:**
```
>> metabasenet-cli eth_accounts []
<< ["0x108429003ce0116c737e4f83267faacffcf8048d"]

>> curl -d '{"id":3,"method":"eth_accounts","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":["0x108429003ce0116c737e4f83267faacffcf8048d"]}
```
**Errors:**
```
* {"code":-32603,"message":"Get wallet accounts fail"}
```
##### [Back to top](#commands)
---
### eth_getBalance
**Usage:**
```
        eth_getBalance <[paramlist]>

[ETH] Get balance
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <address>,(block hash or number,default:latest)
   ]
```
**Response:**
```
 "result": "result"                     (string, required) address balance(hex)
```
**Examples:**
```
>> metabasenet-cli eth_getBalance ["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]
<< 0xa3889a79420a8b8b1d8000

>> curl -d '{"id":3,"method":"eth_getBalance","jsonrpc":"2.0","params":["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0xa3889a79420a8b8b1d8000"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_blockNumber
**Usage:**
```
        eth_blockNumber <[paramlist]>

[ETH] Get latest block numer
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "result"                     (string, required) block number
```
**Examples:**
```
>> metabasenet-cli eth_blockNumber []
<< 0xe2

>> curl -d '{"id":3,"method":"eth_blockNumber","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0xe2"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_getStorageAt
**Usage:**
```
        eth_getStorageAt <[paramlist]>

[ETH] Get storage value
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <address>,<position>,<blocknumber>
   ]
```
**Response:**
```
 "result": "result"                     (string, required) storage value(hex)
```
**Examples:**
```
>> metabasenet-cli eth_getStorageAt ["0x108429003ce0116c737e4f83267faacffcf8048d","0x0","latest"]
<< 0xa3889a79420a8b8b1d8000

>> curl -d '{"id":3,"method":"eth_getStorageAt","jsonrpc":"2.0","params":["0x108429003ce0116c737e4f83267faacffcf8048d","0x0","latest"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0xa3889a79420a8b8b1d8000"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_getStorageRoot
**Usage:**
```
        eth_getStorageRoot <[paramlist]>

[ETH] Get state root
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <address>,<blocknumber>
   ]
```
**Response:**
```
 "result": "result"                     (string, required) state root value(hex)
```
**Examples:**
```
>> metabasenet-cli eth_getStorageRoot ["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]
<< 0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238

>> curl -d '{"id":3,"method":"eth_getStorageRoot","jsonrpc":"2.0","params":["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_getTransactionCount
**Usage:**
```
        eth_getTransactionCount <[paramlist]>

[ETH] Get transaction count of address
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <address>,<blocknumber>
   ]
```
**Response:**
```
 "result": "result"                     (string, required) transaction count(hex)
```
**Examples:**
```
>> metabasenet-cli eth_getTransactionCount ["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]
<< 0x2c98

>> curl -d '{"id":3,"method":"eth_getTransactionCount","jsonrpc":"2.0","params":["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x2c98"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_pendingTransactions
**Usage:**
```
        eth_pendingTransactions <[result]>

[ETH] Get list of transaction that being sent by local accounts
```
**Arguments:**
```
 [result]                               (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "result":                            (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result" :
   "transaction":                       (array, required, default=RPCValid) 
   [
     {
       "hash": "",                      (string, required) transaction hash
       "nonce": "",                     (string, required) tx nonce(hex)
       "from": "",                      (string, required) from address
       "value": "",                     (string, required) transaction amount(hex), unit: wei
       "gasPrice": "",                  (string, required) gas price(hex), unit: wei
       "gas": "",                       (string, required) gas(hex)
       "input": "",                     (string, required) input data
       "blockHash": "",                 (string, optional) block hash
       "blockNumber": "",               (string, optional) block number
       "transactionIndex": "",          (string, optional) transaction index(hex)
       "to": ""                         (string, optional) to address
     }
   ]
```
**Examples:**
```
>> metabasenet-cli eth_pendingTransactions []
<< wait perfect

>> curl -d '{"id":3,"method":"eth_pendingTransactions","jsonrpc":"2.0","params":[]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"wait perfect"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_getBlockTransactionCountByHash
**Usage:**
```
        eth_getBlockTransactionCountByHash <[paramlist]>

[ETH] Get transaction quantity in the specified block by block hash
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <block hash>
   ]
```
**Response:**
```
 "result": "result"                     (string, required) transaction count(hex)
```
**Examples:**
```
>> metabasenet-cli eth_getBlockTransactionCountByHash ["0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"]
<< 0x02

>> curl -d '{"id":3,"method":"eth_getBlockTransactionCountByHash","jsonrpc":"2.0","params":["0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x02"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_getBlockTransactionCountByNumber
**Usage:**
```
        eth_getBlockTransactionCountByNumber <[paramlist]>

[ETH] Get transaction quantity in the specified block by block number
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <block number>
   ]
```
**Response:**
```
 "result": "result"                     (string, required) transaction count(hex)
```
**Examples:**
```
>> metabasenet-cli eth_getBlockTransactionCountByNumber ["0xe203"]
<< 0x02

>> curl -d '{"id":3,"method":"eth_getBlockTransactionCountByNumber","jsonrpc":"2.0","params":["0xe203"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x02"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_getUncleCountByBlockHash
**Usage:**
```
        eth_getUncleCountByBlockHash <[paramlist]>

[ETH] Get number of uncles in the specified blockby block hash
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <block hash>
   ]
```
**Response:**
```
 "result": "result"                     (string, required) transaction count(hex)
```
**Examples:**
```
>> metabasenet-cli eth_getUncleCountByBlockHash ["0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"]
<< 0x02

>> curl -d '{"id":3,"method":"eth_getUncleCountByBlockHash","jsonrpc":"2.0","params":["0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x02"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_getUncleCountByBlockNumber
**Usage:**
```
        eth_getUncleCountByBlockNumber <[paramlist]>

[ETH] Get number of uncles in the specified blockby block number
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <block number>
   ]
```
**Response:**
```
 "result": "result"                     (string, required) transaction count(hex)
```
**Examples:**
```
>> metabasenet-cli eth_getUncleCountByBlockNumber ["0x8543"]
<< 0x02

>> curl -d '{"id":3,"method":"eth_getUncleCountByBlockNumber","jsonrpc":"2.0","params":["0x8543"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x02"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_getCode
**Usage:**
```
        eth_getCode <[paramlist]>

[ETH] Get code
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <address>,(block number)
   ]
```
**Response:**
```
 "result": "result"                     (string, required) code data(hex)
```
**Examples:**
```
>> metabasenet-cli eth_getCode ["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]
<< 0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056

>> curl -d '{"id":3,"method":"eth_getCode","jsonrpc":"2.0","params":["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_sign
**Usage:**
```
        eth_sign <[paramlist]>

[ETH] Sign message
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <address>,<message>
   ]
```
**Response:**
```
 "result": "result"                     (string, required) sign data(hex)
```
**Examples:**
```
>> metabasenet-cli eth_sign ["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]
<< 0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056

>> curl -d '{"id":3,"method":"eth_sign","jsonrpc":"2.0","params":["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_signTransaction
**Usage:**
```
        eth_signTransaction <[paramlist]>

[ETH] Sign transaction
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     {
       "from": "",                      (string, optional) from address
       "to": "",                        (string, optional) to address
       "nonce": "",                     (string, optional) nonce
       "value": "",                     (string, optional) transaction amount(hex), unit: wei
       "gasPrice": "",                  (string, optional) gas price(hex), unit: wei
       "gas": "",                       (string, optional) gas(hex)
       "data": ""                       (string, optional) input data
     }
   ]
```
**Response:**
```
 "result" :
 {
   "tx": "",                            (string, required) transaction hash
   "raw": ""                            (string, required) transaction raw data
 }
```
**Examples:**
```
>> metabasenet-cli eth_signTransaction ["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]
<< 0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056

>> curl -d '{"id":3,"method":"eth_signTransaction","jsonrpc":"2.0","params":["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_sendTransaction
**Usage:**
```
        eth_sendTransaction <[paramlist]>

[ETH] Send transaction
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     {
       "from": "",                      (string, optional) from address
       "to": "",                        (string, optional) to address
       "nonce": "",                     (string, optional) nonce
       "value": "",                     (string, optional) transaction amount(hex), unit: wei
       "gasPrice": "",                  (string, optional) gas price(hex), unit: wei
       "gas": "",                       (string, optional) gas(hex)
       "data": ""                       (string, optional) input data
     }
   ]
```
**Response:**
```
 "result": "result"                     (string, required) transaction hash(hex)
```
**Examples:**
```
>> metabasenet-cli eth_sendTransaction ["0x108429003ce0116c737e4f83267faacffcf8048d","0x108429003ce0116c737e4f83267faacffcf8048d"]
<< 0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056

>> curl -d '{"id":3,"method":"eth_sendTransaction","jsonrpc":"2.0","params":["0x108429003ce0116c737e4f83267faacffcf8048d","latest"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_sendRawTransaction
**Usage:**
```
        eth_sendRawTransaction <[paramlist]>

[ETH] Send raw transaction
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "rawtx": ""                        (string, required) send raw tx data
   ]
```
**Response:**
```
 "result": "result"                     (string, required) transaction hash(hex)
```
**Examples:**
```
>> metabasenet-cli eth_sendRawTransaction ["0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"]
<< 0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056

>> curl -d '{"id":3,"method":"eth_sendRawTransaction","jsonrpc":"2.0","params":["0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_call
**Usage:**
```
        eth_call <[paramlist]>

[ETH] Call contract
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     {
       "from": "",                      (string, optional) from address
       "to": "",                        (string, optional) to address
       "nonce": "",                     (string, optional) nonce
       "value": "",                     (string, optional) transaction amount(hex), unit: wei
       "gasPrice": "",                  (string, optional) gas price(hex), unit: wei
       "gas": "",                       (string, optional) gas(hex)
       "data": "",                      (string, optional) input data
       "code": "",                      (string, optional) code data
       "block": ""                      (string, optional) block hash or number or latest, earliest, pending
     }
   ]
```
**Response:**
```
 "result": "result"                     (string, required) call result
```
**Examples:**
```
>> metabasenet-cli eth_call ["0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"]
<< 0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056

>> curl -d '{"id":3,"method":"eth_call","jsonrpc":"2.0","params":["0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_estimateGas
**Usage:**
```
        eth_estimateGas <[paramlist]>

[ETH] Estimate gas
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     {
       "from": "",                      (string, optional) from address
       "to": "",                        (string, optional) to address
       "nonce": "",                     (string, optional) nonce
       "value": "",                     (string, optional) transaction amount(hex), unit: wei
       "gasPrice": "",                  (string, optional) gas price(hex), unit: wei
       "gas": "",                       (string, optional) gas(hex)
       "data": "",                      (string, optional) input data
       "code": "",                      (string, optional) code data
       "block": ""                      (string, optional) block hash or number or latest, earliest, pending
     }
   ]
```
**Response:**
```
 "result": "result"                     (string, required) gas estimate
```
**Examples:**
```
>> metabasenet-cli eth_estimateGas ["0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"]
<< 0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056

>> curl -d '{"id":3,"method":"eth_estimateGas","jsonrpc":"2.0","params":["0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid address"}
* {"code":-4,"message":"Unknown address"}
* {"code":-32603,"message":"Get last block error"}
```
##### [Back to top](#commands)
---
### eth_getBlockByHash
**Usage:**
```
        eth_getBlockByHash <[paramlist]>

[ETH] Get block by hash
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <block hash>,<if full tx>
   ]
```
**Response:**
```
 "result" :
 {
   "number": "",                        (string, required) block number
   "hash": "",                          (string, required) block hash
   "parentHash": "",                    (string, required) parent block hash
   "nonce": "",                         (string, required) block nonce
   "sha3Uncles": "",                    (string, required) block sha3Uncles
   "logsBloom": "",                     (string, required) block logsBloom
   "transactionsRoot": "",              (string, required) block transactionsRoot
   "stateRoot": "",                     (string, required) block stateRoot
   "receiptsRoot": "",                  (string, required) block receiptsRoot
   "miner": "",                         (string, required) block miner
   "mixHash": "",                       (string, required) block mix hash
   "difficulty": "",                    (string, required) block difficulty
   "totalDifficulty": "",               (string, required) block totalDifficulty
   "extraData": "",                     (string, required) block extraData
   "size": "",                          (string, required) block size
   "gasLimit": "",                      (string, required) block gasLimit
   "gasUsed": "",                       (string, required) block gasUsed
   "timestamp": "",                     (string, required) block timestamp
   "uncles":                            (array, required, default=RPCValid) block uncle list
   [
     "uncle": ""                        (string, required) block uncle hash
   ]
   "transactions":                      (array, optional) transaction hash list
   [
     "tx": ""                           (string, required) transaction hash
   ]
   "transactiondetails":                (array, optional) transaction detail list
   [
     {
       "hash": "",                      (string, optional) tx hash
       "nonce": "",                     (string, optional) tx nonce
       "blockHash": "",                 (string, optional) block hash
       "blockNumber": "",               (string, optional) block number
       "transactionIndex": "",          (string, optional) tx index
       "from": "",                      (string, optional) tx from
       "to": "",                        (string, optional) tx to
       "value": "",                     (string, optional) tx value
       "gasPrice": "",                  (string, optional) tx gas price
       "gas": "",                       (string, optional) tx gas
       "input": "",                     (string, optional) tx input data
       "type": "",                      (string, optional) tx type
       "chainId": "",                   (string, optional) chain id
       "v": "",                         (string, optional) v
       "r": "",                         (string, optional) r
       "s": ""                          (string, optional) s
     }
   ]
 }
```
**Examples:**
```
>> metabasenet-cli eth_getBlockByHash ["0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d", "false"]
<< {"number":"0xa90","hash":"0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","parentHash":"0x00000a8f817569c88f1530ee111e2ffed0d57cbcab7718b7aef4afff09277581","logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000400000000000000000000000000000000000000000000000002008000000000000000000000000000000000000000000020000000000000000000000000000000400000000000000014000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000800000000000008000000001000000000000000000000000000000000000000000000800000000000000000000000080000000000000000000000000000000000000000000000000000000000800","transactionsRoot":"0xf76661bda9c40b072b2c1ee9d613e462eca26bd1a85b705853ce9e4efc1a2fa2","stateRoot":"0x10cdb06582f3c0804527d6f083eab88f555d2b573ef4488c0a6a99158b0c78ad","receiptsRoot":"0x0000000000000000000000000000000000000000000000000000000000000000","miner":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","size":"0x938","gasLimit":"0x000000000000000000000000000000000000000000000000000000003b9aca00","gasUsed":"0x0000000000000000000000000000000000000000000000000000000000000000","timestamp":"0x63899e68","transactions":["0xfa8863899e6837958b02a10aa6c7adddafed038489d6ea092ce3ee12c1de42c5","0xfa8863899e5e8f2d2d729d9694d7f39a0688a1c7e1059287219edfd6b67e80f4","0xfa8863899e18830d93de2a476db4c078d8ba284af3cf4c69ae6d2f66a2bc7cfe"]}

>> curl -d '{"id":3,"method":"eth_getBlockByHash","jsonrpc":"2.0","params":["0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d", "false"]}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":{"number":"0xa90","hash":"0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","parentHash":"0x00000a8f817569c88f1530ee111e2ffed0d57cbcab7718b7aef4afff09277581","logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000400000000000000000000000000000000000000000000000002008000000000000000000000000000000000000000000020000000000000000000000000000000400000000000000014000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000800000000000008000000001000000000000000000000000000000000000000000000800000000000000000000000080000000000000000000000000000000000000000000000000000000000800","transactionsRoot":"0xf76661bda9c40b072b2c1ee9d613e462eca26bd1a85b705853ce9e4efc1a2fa2","stateRoot":"0x10cdb06582f3c0804527d6f083eab88f555d2b573ef4488c0a6a99158b0c78ad","receiptsRoot":"0x0000000000000000000000000000000000000000000000000000000000000000","miner":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","size":"0x938","gasLimit":"0x000000000000000000000000000000000000000000000000000000003b9aca00","gasUsed":"0x0000000000000000000000000000000000000000000000000000000000000000","timestamp":"0x63899e68","transactions":["0xfa8863899e6837958b02a10aa6c7adddafed038489d6ea092ce3ee12c1de42c5","0xfa8863899e5e8f2d2d729d9694d7f39a0688a1c7e1059287219edfd6b67e80f4","0xfa8863899e18830d93de2a476db4c078d8ba284af3cf4c69ae6d2f66a2bc7cfe"]}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-4,"message":"Unknown block"}
```
##### [Back to top](#commands)
---
### eth_getBlockByNumber
**Usage:**
```
        eth_getBlockByNumber <[paramlist]>

[ETH] Get block by number
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <block number>,<if full tx>
   ]
```
**Response:**
```
 "result" :
 {
   "number": "",                        (string, required) block number
   "hash": "",                          (string, required) block hash
   "parentHash": "",                    (string, required) parent block hash
   "nonce": "",                         (string, required) block nonce
   "sha3Uncles": "",                    (string, required) block sha3Uncles
   "logsBloom": "",                     (string, required) block logsBloom
   "transactionsRoot": "",              (string, required) block transactionsRoot
   "stateRoot": "",                     (string, required) block stateRoot
   "receiptsRoot": "",                  (string, required) block receiptsRoot
   "miner": "",                         (string, required) block miner
   "mixHash": "",                       (string, required) block mix hash
   "difficulty": "",                    (string, required) block difficulty
   "totalDifficulty": "",               (string, required) block totalDifficulty
   "extraData": "",                     (string, required) block extraData
   "size": "",                          (string, required) block size
   "gasLimit": "",                      (string, required) block gasLimit
   "gasUsed": "",                       (string, required) block gasUsed
   "timestamp": "",                     (string, required) block timestamp
   "uncles":                            (array, required, default=RPCValid) block uncle list
   [
     "uncle": ""                        (string, required) block uncle hash
   ]
   "transactions":                      (array, optional) transaction hash list
   [
     "tx": ""                           (string, required) transaction hash
   ]
   "transactiondetails":                (array, optional) transaction detail list
   [
     {
       "hash": "",                      (string, optional) tx hash
       "nonce": "",                     (string, optional) tx nonce
       "blockHash": "",                 (string, optional) block hash
       "blockNumber": "",               (string, optional) block number
       "transactionIndex": "",          (string, optional) tx index
       "from": "",                      (string, optional) tx from
       "to": "",                        (string, optional) tx to
       "value": "",                     (string, optional) tx value
       "gasPrice": "",                  (string, optional) tx gas price
       "gas": "",                       (string, optional) tx gas
       "input": "",                     (string, optional) tx input data
       "type": "",                      (string, optional) tx type
       "chainId": "",                   (string, optional) chain id
       "v": "",                         (string, optional) v
       "r": "",                         (string, optional) r
       "s": ""                          (string, optional) s
     }
   ]
 }
```
**Examples:**
```
>> metabasenet-cli eth_getBlockByNumber ["0xa90", "false"]
<< {"number":"0xa90","hash":"0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","parentHash":"0x00000a8f817569c88f1530ee111e2ffed0d57cbcab7718b7aef4afff09277581","logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000400000000000000000000000000000000000000000000000002008000000000000000000000000000000000000000000020000000000000000000000000000000400000000000000014000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000800000000000008000000001000000000000000000000000000000000000000000000800000000000000000000000080000000000000000000000000000000000000000000000000000000000800","transactionsRoot":"0xf76661bda9c40b072b2c1ee9d613e462eca26bd1a85b705853ce9e4efc1a2fa2","stateRoot":"0x10cdb06582f3c0804527d6f083eab88f555d2b573ef4488c0a6a99158b0c78ad","receiptsRoot":"0x0000000000000000000000000000000000000000000000000000000000000000","miner":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","size":"0x938","gasLimit":"0x000000000000000000000000000000000000000000000000000000003b9aca00","gasUsed":"0x0000000000000000000000000000000000000000000000000000000000000000","timestamp":"0x63899e68","transactions":["0xfa8863899e6837958b02a10aa6c7adddafed038489d6ea092ce3ee12c1de42c5","0xfa8863899e5e8f2d2d729d9694d7f39a0688a1c7e1059287219edfd6b67e80f4","0xfa8863899e18830d93de2a476db4c078d8ba284af3cf4c69ae6d2f66a2bc7cfe"]}

>> curl -d '{"id":3,"method":"eth_getBlockByNumber","jsonrpc":"2.0","params":["0xa90", "false"]}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":{"number":"0xa90","hash":"0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","parentHash":"0x00000a8f817569c88f1530ee111e2ffed0d57cbcab7718b7aef4afff09277581","logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000400000000000000000000000000000000000000000000000002008000000000000000000000000000000000000000000020000000000000000000000000000000400000000000000014000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000800000000000008000000001000000000000000000000000000000000000000000000800000000000000000000000080000000000000000000000000000000000000000000000000000000000800","transactionsRoot":"0xf76661bda9c40b072b2c1ee9d613e462eca26bd1a85b705853ce9e4efc1a2fa2","stateRoot":"0x10cdb06582f3c0804527d6f083eab88f555d2b573ef4488c0a6a99158b0c78ad","receiptsRoot":"0x0000000000000000000000000000000000000000000000000000000000000000","miner":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","size":"0x938","gasLimit":"0x000000000000000000000000000000000000000000000000000000003b9aca00","gasUsed":"0x0000000000000000000000000000000000000000000000000000000000000000","timestamp":"0x63899e68","transactions":["0xfa8863899e6837958b02a10aa6c7adddafed038489d6ea092ce3ee12c1de42c5","0xfa8863899e5e8f2d2d729d9694d7f39a0688a1c7e1059287219edfd6b67e80f4","0xfa8863899e18830d93de2a476db4c078d8ba284af3cf4c69ae6d2f66a2bc7cfe"]}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid block number"}
* {"code":-4,"message":"Unknown block"}
```
##### [Back to top](#commands)
---
### eth_getTransactionByHash
**Usage:**
```
        eth_getTransactionByHash <[paramlist]>

[ETH] Get transaction by hash
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <tx hash>
   ]
```
**Response:**
```
 "result" :
 {
   "hash": "",                          (string, optional) tx hash
   "nonce": "",                         (string, optional) tx nonce
   "blockHash": "",                     (string, optional) block hash
   "blockNumber": "",                   (string, optional) block number
   "transactionIndex": "",              (string, optional) tx index
   "from": "",                          (string, optional) tx from
   "to": "",                            (string, optional) tx to
   "value": "",                         (string, optional) tx value
   "gasPrice": "",                      (string, optional) tx gas price
   "gas": "",                           (string, optional) tx gas
   "input": "",                         (string, optional) tx input data
   "type": "",                          (string, optional) tx type
   "chainId": "",                       (string, optional) chain id
   "v": "",                             (string, optional) v
   "r": "",                             (string, optional) r
   "s": ""                              (string, optional) s
 }
```
**Examples:**
```
>> metabasenet-cli eth_getTransactionByHash ["0xfa8863899e18830d93de2a476db4c078d8ba284af3cf4c69ae6d2f66a2bc7cfe"]
<< {"hash":"0xfa8863899e5e8f2d2d729d9694d7f39a0688a1c7e1059287219edfd6b67e80f4","nonce":"0xa77","blockHash":"0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","blockNumber":"0xa90","transactionIndex":"0x1","from":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","to":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","value":"0x01","gasPrice":"0x0","gas":"0x0","input":"0x010801fd08038f0a0000fd010310708b87083d64b53e18be6d9752cee52005d5e0fcdb326946bbe858efe1662a151b1c836747009b16d2f6a9e71c0e66c113c40058b28802da555a20de3aa973de6e525bc0d4c7ce9bd687644f93e268bb4c5d31d1bbc632c89e91a1dbc77be01ab2bdceb3ff685ce56cb8394808f71bf458d749abd50df3780a66c7d5a33d42019b124cb22bd310f989deb862eb552c749b056fef7b3bb646287ea25a514f4e0a3d677cd2738ea4b8eadc4ce02de91e93d5f7b9b4f978562fe56ff3e3b82a840692f4652db81bf6c6446da4b9bd6f75220c766301bb0673adf25b4434a0553cace2f15872711fb20e71c64b68876a80451238628f683a1d64d69aa14ffd0584cf08b21f3bd1da2c842b2ba7cfde089640420eaa94d59499e540fb11c29de8aaf0b2c165d6d6cf8267ef8f21a89d00bb8477d35708a5a12f31b75e9edec88fe06828438f089a44a3f58aa8816c37d67a601fe69d005b0ba1fa8cd4f40641a9a91a382f613e048b44147283601b6d940d8c5ed28c294fc21897f28016ddea3f111f5bf2a3dd6a1492266b73f5e9503ce7b81f05c376d8ed3dead3b92c2f40ebecefeea08480630e3367928e9eb8f633e83793d854b3ce92bbb1e43910156b297eabe820659b43b5466ab62799e4c6aa3a3eef6859ccd2872712931c5f587875f4b56f964591361be1b5a6e787baaa97507fead6be1d0748f365699505f8879cff32c4b36c27e7396fbe17683edf564bc9995cd77f9703fe351914e4aac15f5e24c77b0b7868ba4074c48e4ac59eb952e6475583851e3e4436c40e590517ae26db869cb05356feedda4ba623026795e9198d7fea94ad8b429debb86c959da6e5d11acd626fc12f48054c6183da31dc837187528d2652281a8a45a43c6096be0981087852a8350588b7277be922f369f23ef9b505c158f66f16a53a72b53ae971958c60bfebed89134ff856493df5b2bd2b166816caca792515a7130a894c1e2bbb6c11219f14cbf42c4ff81d14bb676a50d89328c4d98ea61b609c5b4a306e3be3ee0edbcf551f5b839c6ffcde85b07dc4a04732c7298feba8e33471f5509a271146"}

>> curl -d '{"id":3,"method":"eth_getTransactionByHash","jsonrpc":"2.0","params":["0xfa8863899e18830d93de2a476db4c078d8ba284af3cf4c69ae6d2f66a2bc7cfe"]}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":{"hash":"0xfa8863899e5e8f2d2d729d9694d7f39a0688a1c7e1059287219edfd6b67e80f4","nonce":"0xa77","blockHash":"0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","blockNumber":"0xa90","transactionIndex":"0x1","from":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","to":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","value":"0x01","gasPrice":"0x0","gas":"0x0","input":"0x010801fd08038f0a0000fd010310708b87083d64b53e18be6d9752cee52005d5e0fcdb326946bbe858efe1662a151b1c836747009b16d2f6a9e71c0e66c113c40058b28802da555a20de3aa973de6e525bc0d4c7ce9bd687644f93e268bb4c5d31d1bbc632c89e91a1dbc77be01ab2bdceb3ff685ce56cb8394808f71bf458d749abd50df3780a66c7d5a33d42019b124cb22bd310f989deb862eb552c749b056fef7b3bb646287ea25a514f4e0a3d677cd2738ea4b8eadc4ce02de91e93d5f7b9b4f978562fe56ff3e3b82a840692f4652db81bf6c6446da4b9bd6f75220c766301bb0673adf25b4434a0553cace2f15872711fb20e71c64b68876a80451238628f683a1d64d69aa14ffd0584cf08b21f3bd1da2c842b2ba7cfde089640420eaa94d59499e540fb11c29de8aaf0b2c165d6d6cf8267ef8f21a89d00bb8477d35708a5a12f31b75e9edec88fe06828438f089a44a3f58aa8816c37d67a601fe69d005b0ba1fa8cd4f40641a9a91a382f613e048b44147283601b6d940d8c5ed28c294fc21897f28016ddea3f111f5bf2a3dd6a1492266b73f5e9503ce7b81f05c376d8ed3dead3b92c2f40ebecefeea08480630e3367928e9eb8f633e83793d854b3ce92bbb1e43910156b297eabe820659b43b5466ab62799e4c6aa3a3eef6859ccd2872712931c5f587875f4b56f964591361be1b5a6e787baaa97507fead6be1d0748f365699505f8879cff32c4b36c27e7396fbe17683edf564bc9995cd77f9703fe351914e4aac15f5e24c77b0b7868ba4074c48e4ac59eb952e6475583851e3e4436c40e590517ae26db869cb05356feedda4ba623026795e9198d7fea94ad8b429debb86c959da6e5d11acd626fc12f48054c6183da31dc837187528d2652281a8a45a43c6096be0981087852a8350588b7277be922f369f23ef9b505c158f66f16a53a72b53ae971958c60bfebed89134ff856493df5b2bd2b166816caca792515a7130a894c1e2bbb6c11219f14cbf42c4ff81d14bb676a50d89328c4d98ea61b609c5b4a306e3be3ee0edbcf551f5b839c6ffcde85b07dc4a04732c7298feba8e33471f5509a271146"}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-32600,"message":"No information available about transaction"}
```
##### [Back to top](#commands)
---
### eth_getTransactionByBlockHashAndIndex
**Usage:**
```
        eth_getTransactionByBlockHashAndIndex <[paramlist]>

[ETH] Get transaction by block hash and tx index
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <block hash>,<tx index>
   ]
```
**Response:**
```
 "result" :
 {
   "hash": "",                          (string, optional) tx hash
   "nonce": "",                         (string, optional) tx nonce
   "blockHash": "",                     (string, optional) block hash
   "blockNumber": "",                   (string, optional) block number
   "transactionIndex": "",              (string, optional) tx index
   "from": "",                          (string, optional) tx from
   "to": "",                            (string, optional) tx to
   "value": "",                         (string, optional) tx value
   "gasPrice": "",                      (string, optional) tx gas price
   "gas": "",                           (string, optional) tx gas
   "input": "",                         (string, optional) tx input data
   "type": "",                          (string, optional) tx type
   "chainId": "",                       (string, optional) chain id
   "v": "",                             (string, optional) v
   "r": "",                             (string, optional) r
   "s": ""                              (string, optional) s
 }
```
**Examples:**
```
>> metabasenet-cli eth_getTransactionByBlockHashAndIndex ["0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","0x1"]
<< {"hash":"0xfa8863899e5e8f2d2d729d9694d7f39a0688a1c7e1059287219edfd6b67e80f4","nonce":"0xa77","blockHash":"0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","blockNumber":"0xa90","transactionIndex":"0x1","from":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","to":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","value":"0x01","gasPrice":"0x0","gas":"0x0","input":"0x010801fd08038f0a0000fd010310708b87083d64b53e18be6d9752cee52005d5e0fcdb326946bbe858efe1662a151b1c836747009b16d2f6a9e71c0e66c113c40058b28802da555a20de3aa973de6e525bc0d4c7ce9bd687644f93e268bb4c5d31d1bbc632c89e91a1dbc77be01ab2bdceb3ff685ce56cb8394808f71bf458d749abd50df3780a66c7d5a33d42019b124cb22bd310f989deb862eb552c749b056fef7b3bb646287ea25a514f4e0a3d677cd2738ea4b8eadc4ce02de91e93d5f7b9b4f978562fe56ff3e3b82a840692f4652db81bf6c6446da4b9bd6f75220c766301bb0673adf25b4434a0553cace2f15872711fb20e71c64b68876a80451238628f683a1d64d69aa14ffd0584cf08b21f3bd1da2c842b2ba7cfde089640420eaa94d59499e540fb11c29de8aaf0b2c165d6d6cf8267ef8f21a89d00bb8477d35708a5a12f31b75e9edec88fe06828438f089a44a3f58aa8816c37d67a601fe69d005b0ba1fa8cd4f40641a9a91a382f613e048b44147283601b6d940d8c5ed28c294fc21897f28016ddea3f111f5bf2a3dd6a1492266b73f5e9503ce7b81f05c376d8ed3dead3b92c2f40ebecefeea08480630e3367928e9eb8f633e83793d854b3ce92bbb1e43910156b297eabe820659b43b5466ab62799e4c6aa3a3eef6859ccd2872712931c5f587875f4b56f964591361be1b5a6e787baaa97507fead6be1d0748f365699505f8879cff32c4b36c27e7396fbe17683edf564bc9995cd77f9703fe351914e4aac15f5e24c77b0b7868ba4074c48e4ac59eb952e6475583851e3e4436c40e590517ae26db869cb05356feedda4ba623026795e9198d7fea94ad8b429debb86c959da6e5d11acd626fc12f48054c6183da31dc837187528d2652281a8a45a43c6096be0981087852a8350588b7277be922f369f23ef9b505c158f66f16a53a72b53ae971958c60bfebed89134ff856493df5b2bd2b166816caca792515a7130a894c1e2bbb6c11219f14cbf42c4ff81d14bb676a50d89328c4d98ea61b609c5b4a306e3be3ee0edbcf551f5b839c6ffcde85b07dc4a04732c7298feba8e33471f5509a271146"}

>> curl -d '{"id":3,"method":"eth_getTransactionByBlockHashAndIndex","jsonrpc":"2.0","params":["0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","0x1"]}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":{"hash":"0xfa8863899e5e8f2d2d729d9694d7f39a0688a1c7e1059287219edfd6b67e80f4","nonce":"0xa77","blockHash":"0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","blockNumber":"0xa90","transactionIndex":"0x1","from":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","to":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","value":"0x01","gasPrice":"0x0","gas":"0x0","input":"0x010801fd08038f0a0000fd010310708b87083d64b53e18be6d9752cee52005d5e0fcdb326946bbe858efe1662a151b1c836747009b16d2f6a9e71c0e66c113c40058b28802da555a20de3aa973de6e525bc0d4c7ce9bd687644f93e268bb4c5d31d1bbc632c89e91a1dbc77be01ab2bdceb3ff685ce56cb8394808f71bf458d749abd50df3780a66c7d5a33d42019b124cb22bd310f989deb862eb552c749b056fef7b3bb646287ea25a514f4e0a3d677cd2738ea4b8eadc4ce02de91e93d5f7b9b4f978562fe56ff3e3b82a840692f4652db81bf6c6446da4b9bd6f75220c766301bb0673adf25b4434a0553cace2f15872711fb20e71c64b68876a80451238628f683a1d64d69aa14ffd0584cf08b21f3bd1da2c842b2ba7cfde089640420eaa94d59499e540fb11c29de8aaf0b2c165d6d6cf8267ef8f21a89d00bb8477d35708a5a12f31b75e9edec88fe06828438f089a44a3f58aa8816c37d67a601fe69d005b0ba1fa8cd4f40641a9a91a382f613e048b44147283601b6d940d8c5ed28c294fc21897f28016ddea3f111f5bf2a3dd6a1492266b73f5e9503ce7b81f05c376d8ed3dead3b92c2f40ebecefeea08480630e3367928e9eb8f633e83793d854b3ce92bbb1e43910156b297eabe820659b43b5466ab62799e4c6aa3a3eef6859ccd2872712931c5f587875f4b56f964591361be1b5a6e787baaa97507fead6be1d0748f365699505f8879cff32c4b36c27e7396fbe17683edf564bc9995cd77f9703fe351914e4aac15f5e24c77b0b7868ba4074c48e4ac59eb952e6475583851e3e4436c40e590517ae26db869cb05356feedda4ba623026795e9198d7fea94ad8b429debb86c959da6e5d11acd626fc12f48054c6183da31dc837187528d2652281a8a45a43c6096be0981087852a8350588b7277be922f369f23ef9b505c158f66f16a53a72b53ae971958c60bfebed89134ff856493df5b2bd2b166816caca792515a7130a894c1e2bbb6c11219f14cbf42c4ff81d14bb676a50d89328c4d98ea61b609c5b4a306e3be3ee0edbcf551f5b839c6ffcde85b07dc4a04732c7298feba8e33471f5509a271146"}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid block hash"}
* {"code":-32600,"message":"No information available about transaction"}
```
##### [Back to top](#commands)
---
### eth_getTransactionByBlockNumberAndIndex
**Usage:**
```
        eth_getTransactionByBlockNumberAndIndex <[paramlist]>

[ETH] Get transaction by block number and tx index
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <block number>,<tx index>
   ]
```
**Response:**
```
 "result" :
 {
   "hash": "",                          (string, optional) tx hash
   "nonce": "",                         (string, optional) tx nonce
   "blockHash": "",                     (string, optional) block hash
   "blockNumber": "",                   (string, optional) block number
   "transactionIndex": "",              (string, optional) tx index
   "from": "",                          (string, optional) tx from
   "to": "",                            (string, optional) tx to
   "value": "",                         (string, optional) tx value
   "gasPrice": "",                      (string, optional) tx gas price
   "gas": "",                           (string, optional) tx gas
   "input": "",                         (string, optional) tx input data
   "type": "",                          (string, optional) tx type
   "chainId": "",                       (string, optional) chain id
   "v": "",                             (string, optional) v
   "r": "",                             (string, optional) r
   "s": ""                              (string, optional) s
 }
```
**Examples:**
```
>> metabasenet-cli eth_getTransactionByBlockNumberAndIndex ["0xa90","0x1"]
<< {"hash":"0xfa8863899e5e8f2d2d729d9694d7f39a0688a1c7e1059287219edfd6b67e80f4","nonce":"0xa77","blockHash":"0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","blockNumber":"0xa90","transactionIndex":"0x1","from":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","to":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","value":"0x01","gasPrice":"0x0","gas":"0x0","input":"0x010801fd08038f0a0000fd010310708b87083d64b53e18be6d9752cee52005d5e0fcdb326946bbe858efe1662a151b1c836747009b16d2f6a9e71c0e66c113c40058b28802da555a20de3aa973de6e525bc0d4c7ce9bd687644f93e268bb4c5d31d1bbc632c89e91a1dbc77be01ab2bdceb3ff685ce56cb8394808f71bf458d749abd50df3780a66c7d5a33d42019b124cb22bd310f989deb862eb552c749b056fef7b3bb646287ea25a514f4e0a3d677cd2738ea4b8eadc4ce02de91e93d5f7b9b4f978562fe56ff3e3b82a840692f4652db81bf6c6446da4b9bd6f75220c766301bb0673adf25b4434a0553cace2f15872711fb20e71c64b68876a80451238628f683a1d64d69aa14ffd0584cf08b21f3bd1da2c842b2ba7cfde089640420eaa94d59499e540fb11c29de8aaf0b2c165d6d6cf8267ef8f21a89d00bb8477d35708a5a12f31b75e9edec88fe06828438f089a44a3f58aa8816c37d67a601fe69d005b0ba1fa8cd4f40641a9a91a382f613e048b44147283601b6d940d8c5ed28c294fc21897f28016ddea3f111f5bf2a3dd6a1492266b73f5e9503ce7b81f05c376d8ed3dead3b92c2f40ebecefeea08480630e3367928e9eb8f633e83793d854b3ce92bbb1e43910156b297eabe820659b43b5466ab62799e4c6aa3a3eef6859ccd2872712931c5f587875f4b56f964591361be1b5a6e787baaa97507fead6be1d0748f365699505f8879cff32c4b36c27e7396fbe17683edf564bc9995cd77f9703fe351914e4aac15f5e24c77b0b7868ba4074c48e4ac59eb952e6475583851e3e4436c40e590517ae26db869cb05356feedda4ba623026795e9198d7fea94ad8b429debb86c959da6e5d11acd626fc12f48054c6183da31dc837187528d2652281a8a45a43c6096be0981087852a8350588b7277be922f369f23ef9b505c158f66f16a53a72b53ae971958c60bfebed89134ff856493df5b2bd2b166816caca792515a7130a894c1e2bbb6c11219f14cbf42c4ff81d14bb676a50d89328c4d98ea61b609c5b4a306e3be3ee0edbcf551f5b839c6ffcde85b07dc4a04732c7298feba8e33471f5509a271146"}

>> curl -d '{"id":3,"method":"eth_getTransactionByBlockNumberAndIndex","jsonrpc":"2.0","params":["0xa90","0x1"]}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":{"hash":"0xfa8863899e5e8f2d2d729d9694d7f39a0688a1c7e1059287219edfd6b67e80f4","nonce":"0xa77","blockHash":"0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","blockNumber":"0xa90","transactionIndex":"0x1","from":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","to":"0x2338e108bcb4bd56ed1b4584848a1ef2fcf2fa75","value":"0x01","gasPrice":"0x0","gas":"0x0","input":"0x010801fd08038f0a0000fd010310708b87083d64b53e18be6d9752cee52005d5e0fcdb326946bbe858efe1662a151b1c836747009b16d2f6a9e71c0e66c113c40058b28802da555a20de3aa973de6e525bc0d4c7ce9bd687644f93e268bb4c5d31d1bbc632c89e91a1dbc77be01ab2bdceb3ff685ce56cb8394808f71bf458d749abd50df3780a66c7d5a33d42019b124cb22bd310f989deb862eb552c749b056fef7b3bb646287ea25a514f4e0a3d677cd2738ea4b8eadc4ce02de91e93d5f7b9b4f978562fe56ff3e3b82a840692f4652db81bf6c6446da4b9bd6f75220c766301bb0673adf25b4434a0553cace2f15872711fb20e71c64b68876a80451238628f683a1d64d69aa14ffd0584cf08b21f3bd1da2c842b2ba7cfde089640420eaa94d59499e540fb11c29de8aaf0b2c165d6d6cf8267ef8f21a89d00bb8477d35708a5a12f31b75e9edec88fe06828438f089a44a3f58aa8816c37d67a601fe69d005b0ba1fa8cd4f40641a9a91a382f613e048b44147283601b6d940d8c5ed28c294fc21897f28016ddea3f111f5bf2a3dd6a1492266b73f5e9503ce7b81f05c376d8ed3dead3b92c2f40ebecefeea08480630e3367928e9eb8f633e83793d854b3ce92bbb1e43910156b297eabe820659b43b5466ab62799e4c6aa3a3eef6859ccd2872712931c5f587875f4b56f964591361be1b5a6e787baaa97507fead6be1d0748f365699505f8879cff32c4b36c27e7396fbe17683edf564bc9995cd77f9703fe351914e4aac15f5e24c77b0b7868ba4074c48e4ac59eb952e6475583851e3e4436c40e590517ae26db869cb05356feedda4ba623026795e9198d7fea94ad8b429debb86c959da6e5d11acd626fc12f48054c6183da31dc837187528d2652281a8a45a43c6096be0981087852a8350588b7277be922f369f23ef9b505c158f66f16a53a72b53ae971958c60bfebed89134ff856493df5b2bd2b166816caca792515a7130a894c1e2bbb6c11219f14cbf42c4ff81d14bb676a50d89328c4d98ea61b609c5b4a306e3be3ee0edbcf551f5b839c6ffcde85b07dc4a04732c7298feba8e33471f5509a271146"}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid block number"}
* {"code":-32600,"message":"No information available about transaction"}
```
##### [Back to top](#commands)
---
### eth_getTransactionReceipt
**Usage:**
```
        eth_getTransactionReceipt <[paramlist]>

[ETH] Get transaction receipt
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <tx hash>
   ]
```
**Response:**
```
 "result" :
 {
   "transactionHash": "",               (string, required) transaction hash
   "transactionIndex": 0,               (uint, required) transaction index
   "blockHash": "",                     (string, required) block hash
   "blockNumber": 0,                    (uint, required) block number
   "from": "",                          (string, required) tx from
   "to": "",                            (string, required) tx to
   "cumulativeGasUsed": "",             (string, required) cumulative gas used
   "gasUsed": "",                       (string, required) gas used
   "logs":                              (array, required, default=RPCValid) 
   [
     {
       "removed": true|false,           (bool, optional) removed
       "logIndex": 0,                   (uint, optional) log index
       "transactionIndex": 0,           (uint, optional) transaction index
       "transactionHash": "",           (string, optional) transaction hash
       "blockHash": "",                 (string, optional) block hash
       "blockNumber": 0,                (uint, optional) block number
       "address": "",                   (string, optional) address
       "data": "",                      (string, optional) data
       "topics":                        (array, optional) topic list
       [
         "topic": ""                    (string, required) topic
       ]
       "type": "",                      (string, optional) type
       "id": ""                         (string, optional) id
     }
   ]
   "logsBloom": "",                     (string, required) logs bloom
   "status": "",                        (string, required) status, 0x1: ok, 0x0: fail
   "effectiveGasPrice": "",             (string, required) effective gas price
   "contractAddress": "",               (string, optional) contract address
   "root": ""                           (string, optional) root
 }
```
**Examples:**
```
>> metabasenet-cli eth_getTransactionReceipt ["0xfa8863899e18830d93de2a476db4c078d8ba284af3cf4c69ae6d2f66a2bc7cfe"]
<< 0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056

>> curl -d '{"id":3,"method":"eth_getTransactionReceipt","jsonrpc":"2.0","params":["0xfa8863899e18830d93de2a476db4c078d8ba284af3cf4c69ae6d2f66a2bc7cfe"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0x600160008035811a818181146012578301005b601b6001356025565b8060005260206000f25b600060078202905091905056"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid paramlist"}
* {"code":-6,"message":"Invalid tx hash"}
* {"code":-7,"message":"Get transaction receipt fail"}
```
##### [Back to top](#commands)
---
### eth_getUncleByBlockHashAndIndex
**Usage:**
```
        eth_getUncleByBlockHashAndIndex <[paramlist]>

[ETH] Get Uncle by block hash and index
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <block hash>,<uncle index>
   ]
```
**Response:**
```
 "result" :
 {
   "number": "",                        (string, required) block number
   "hash": "",                          (string, required) block hash
   "parentHash": "",                    (string, required) parent block hash
   "nonce": "",                         (string, required) block nonce
   "sha3Uncles": "",                    (string, required) block sha3Uncles
   "logsBloom": "",                     (string, required) block logsBloom
   "transactionsRoot": "",              (string, required) block transactionsRoot
   "stateRoot": "",                     (string, required) block stateRoot
   "receiptsRoot": "",                  (string, required) block receiptsRoot
   "miner": "",                         (string, required) block miner
   "mixHash": "",                       (string, required) block mix hash
   "difficulty": "",                    (string, required) block difficulty
   "totalDifficulty": "",               (string, required) block totalDifficulty
   "extraData": "",                     (string, required) block extraData
   "size": "",                          (string, required) block size
   "gasLimit": "",                      (string, required) block gasLimit
   "gasUsed": "",                       (string, required) block gasUsed
   "timestamp": "",                     (string, required) block timestamp
   "uncles":                            (array, required, default=RPCValid) block uncle list
   [
     "uncle": ""                        (string, required) block uncle hash
   ]
   "transactions":                      (array, optional) transaction hash list
   [
     "tx": ""                           (string, required) transaction hash
   ]
   "transactiondetails":                (array, optional) transaction detail list
   [
     {
       "hash": "",                      (string, optional) tx hash
       "nonce": "",                     (string, optional) tx nonce
       "blockHash": "",                 (string, optional) block hash
       "blockNumber": "",               (string, optional) block number
       "transactionIndex": "",          (string, optional) tx index
       "from": "",                      (string, optional) tx from
       "to": "",                        (string, optional) tx to
       "value": "",                     (string, optional) tx value
       "gasPrice": "",                  (string, optional) tx gas price
       "gas": "",                       (string, optional) tx gas
       "input": "",                     (string, optional) tx input data
       "type": "",                      (string, optional) tx type
       "chainId": "",                   (string, optional) chain id
       "v": "",                         (string, optional) v
       "r": "",                         (string, optional) r
       "s": ""                          (string, optional) s
     }
   ]
 }
```
**Examples:**
```
>> metabasenet-cli eth_getUncleByBlockHashAndIndex ["0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","0x1"]
<< {}

>> curl -d '{"id":3,"method":"eth_getUncleByBlockHashAndIndex","jsonrpc":"2.0","params":["0x00000a9021b0790da106a1de3c397d50ac3d8fd049b9f09df3f2cbf2df93414d","0x1"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":{}}
```
**Errors:**
```
* {"code":-15,"message":"Requested function is obsolete"}
```
##### [Back to top](#commands)
---
### eth_getUncleByBlockNumberAndIndex
**Usage:**
```
        eth_getUncleByBlockNumberAndIndex <[paramlist]>

[ETH] Get Uncle by block number and index
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param: <block number>,<uncle index>
   ]
```
**Response:**
```
 "result" :
 {
   "number": "",                        (string, required) block number
   "hash": "",                          (string, required) block hash
   "parentHash": "",                    (string, required) parent block hash
   "nonce": "",                         (string, required) block nonce
   "sha3Uncles": "",                    (string, required) block sha3Uncles
   "logsBloom": "",                     (string, required) block logsBloom
   "transactionsRoot": "",              (string, required) block transactionsRoot
   "stateRoot": "",                     (string, required) block stateRoot
   "receiptsRoot": "",                  (string, required) block receiptsRoot
   "miner": "",                         (string, required) block miner
   "mixHash": "",                       (string, required) block mix hash
   "difficulty": "",                    (string, required) block difficulty
   "totalDifficulty": "",               (string, required) block totalDifficulty
   "extraData": "",                     (string, required) block extraData
   "size": "",                          (string, required) block size
   "gasLimit": "",                      (string, required) block gasLimit
   "gasUsed": "",                       (string, required) block gasUsed
   "timestamp": "",                     (string, required) block timestamp
   "uncles":                            (array, required, default=RPCValid) block uncle list
   [
     "uncle": ""                        (string, required) block uncle hash
   ]
   "transactions":                      (array, optional) transaction hash list
   [
     "tx": ""                           (string, required) transaction hash
   ]
   "transactiondetails":                (array, optional) transaction detail list
   [
     {
       "hash": "",                      (string, optional) tx hash
       "nonce": "",                     (string, optional) tx nonce
       "blockHash": "",                 (string, optional) block hash
       "blockNumber": "",               (string, optional) block number
       "transactionIndex": "",          (string, optional) tx index
       "from": "",                      (string, optional) tx from
       "to": "",                        (string, optional) tx to
       "value": "",                     (string, optional) tx value
       "gasPrice": "",                  (string, optional) tx gas price
       "gas": "",                       (string, optional) tx gas
       "input": "",                     (string, optional) tx input data
       "type": "",                      (string, optional) tx type
       "chainId": "",                   (string, optional) chain id
       "v": "",                         (string, optional) v
       "r": "",                         (string, optional) r
       "s": ""                          (string, optional) s
     }
   ]
 }
```
**Examples:**
```
>> metabasenet-cli eth_getUncleByBlockNumberAndIndex ["0xa90","0x1"]
<< {}

>> curl -d '{"id":3,"method":"eth_getUncleByBlockNumberAndIndex","jsonrpc":"2.0","params":["0xa90","0x1"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":{}}
```
**Errors:**
```
* {"code":-15,"message":"Requested function is obsolete"}
```
##### [Back to top](#commands)
---
### eth_newFilter
**Usage:**
```
        eth_newFilter <[paramlist]>

[ETH] Create new filter
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     {
       "fromBlock": "",                 (string, optional) fromBlock: block hash or number or latesr or pending or earliest
       "toBlock": "",                   (string, optional) toBlock: block hash or number or latesr or pending or earliest
       "address": "",                   (string, optional) address
       "topics":                        (array, optional) topics list
       [
         "tp": ""                       (string, required) topics data
       ]
     }
   ]
```
**Response:**
```
 "result": "filterid"                   (string, required) filter id
```
**Examples:**
```
>> metabasenet-cli eth_newFilter ["0xa90","0x1"]
<< {}

>> curl -d '{"id":3,"method":"eth_newFilter","jsonrpc":"2.0","params":["0xa90","0x1"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":{}}
```
**Errors:**
```
* {"code":-15,"message":"Requested function is obsolete"}
```
##### [Back to top](#commands)
---
### eth_newBlockFilter
**Usage:**
```
        eth_newBlockFilter <[paramlist]>

[ETH] Create new block filter
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "filterid"                   (string, required) filter id
```
**Examples:**
```
>> metabasenet-cli eth_newBlockFilter ["0xa90","0x1"]
<< {}

>> curl -d '{"id":3,"method":"eth_newBlockFilter","jsonrpc":"2.0","params":["0xa90","0x1"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":{}}
```
**Errors:**
```
* {"code":-15,"message":"Requested function is obsolete"}
```
##### [Back to top](#commands)
---
### eth_newPendingTransactionFilter
**Usage:**
```
        eth_newPendingTransactionFilter <[paramlist]>

[ETH] Create new pending transaction filter
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": "filterid"                   (string, required) filter id
```
**Examples:**
```
>> metabasenet-cli eth_newPendingTransactionFilter ["0xa90","0x1"]
<< {}

>> curl -d '{"id":3,"method":"eth_newPendingTransactionFilter","jsonrpc":"2.0","params":["0xa90","0x1"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":{}}
```
**Errors:**
```
* {"code":-15,"message":"Requested function is obsolete"}
```
##### [Back to top](#commands)
---
### eth_uninstallFilter
**Usage:**
```
        eth_uninstallFilter <[paramlist]>

[ETH] Uninstall filter
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result": result                       (bool, required) result
```
**Examples:**
```
>> metabasenet-cli eth_uninstallFilter ["0xa90","0x1"]
<< {}

>> curl -d '{"id":3,"method":"eth_uninstallFilter","jsonrpc":"2.0","params":["0xa90","0x1"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":{}}
```
**Errors:**
```
* {"code":-15,"message":"Requested function is obsolete"}
```
##### [Back to top](#commands)
---
### eth_getFilterChanges
**Usage:**
```
        eth_getFilterChanges <[paramlist]>

[ETH] Get filter changes
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) param
   ]
```
**Response:**
```
 "result" :
   "result":                            (array, required, default=RPCValid) 
   [
     {
       "removed": true|false,           (bool, optional) removed
       "logIndex": "",                  (string, optional) log index
       "transactionIndex": "",          (string, optional) transaction index
       "transactionHash": "",           (string, optional) transaction hash
       "blockNumber": "",               (string, optional) block number
       "blockHash": "",                 (string, optional) block hash
       "address": "",                   (string, optional) address
       "data": "",                      (string, optional) data
       "topics":                        (array, optional) topics list
       [
         "tp": ""                       (string, required) topics data
       ]
     }
   ]
```
**Examples:**
```
>> metabasenet-cli eth_getFilterChanges ["0xa90","0x1"]
<< {}

>> curl -d '{"id":3,"method":"eth_getFilterChanges","jsonrpc":"2.0","params":["0xa90","0x1"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":{}}
```
**Errors:**
```
* {"code":-15,"message":"Requested function is obsolete"}
```
##### [Back to top](#commands)
---
### eth_getFilterLogs
**Usage:**
```
        eth_getFilterLogs <[paramlist]>

[ETH] Get logs
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) filter id
   ]
```
**Response:**
```
 "result" :
   "result":                            (array, required, default=RPCValid) 
   [
     {
       "removed": true|false,           (bool, optional) removed
       "logIndex": "",                  (string, optional) log index
       "transactionIndex": "",          (string, optional) transaction index
       "transactionHash": "",           (string, optional) transaction hash
       "blockNumber": "",               (string, optional) block number
       "blockHash": "",                 (string, optional) block hash
       "address": "",                   (string, optional) address
       "data": "",                      (string, optional) data
       "topics":                        (array, optional) topics list
       [
         "tp": ""                       (string, required) topics data
       ]
     }
   ]
```
**Examples:**
```
>> metabasenet-cli eth_getFilterLogs ["0xa90","0x1"]
<< {}

>> curl -d '{"id":3,"method":"eth_getFilterLogs","jsonrpc":"2.0","params":["0xa90","0x1"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":{}}
```
**Errors:**
```
* {"code":-15,"message":"Requested function is obsolete"}
```
##### [Back to top](#commands)
---
### eth_getFilterBlockTx
**Usage:**
```
        eth_getFilterBlockTx <[paramlist]>

[ETH] Get filter block tx
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) filter id
   ]
```
**Response:**
```
 "result" :
   "result":                            (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) block or tx hash
   ]
```
**Examples:**
```
>> metabasenet-cli eth_getFilterBlockTx ["0xa90","0x1"]
<< {}

>> curl -d '{"id":3,"method":"eth_getFilterBlockTx","jsonrpc":"2.0","params":["0xa90","0x1"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":{}}
```
**Errors:**
```
* {"code":-15,"message":"Requested function is obsolete"}
```
##### [Back to top](#commands)
---
### eth_getLogs
**Usage:**
```
        eth_getLogs <[paramlist]>

[ETH] Get logs
```
**Arguments:**
```
 [paramlist]                            (array, required, default=RPCValid) 
```
**Request:**
```
 "param" :
   "paramlist":                         (array, required, default=RPCValid) 
   [
     "param": ""                        (string, required) new filter
   ]
```
**Response:**
```
 "result" :
   "result":                            (array, required, default=RPCValid) 
   [
     {
       "removed": true|false,           (bool, optional) removed
       "logIndex": "",                  (string, optional) log index
       "transactionIndex": "",          (string, optional) transaction index
       "transactionHash": "",           (string, optional) transaction hash
       "blockNumber": "",               (string, optional) block number
       "blockHash": "",                 (string, optional) block hash
       "address": "",                   (string, optional) address
       "data": "",                      (string, optional) data
       "topics":                        (array, optional) topics list
       [
         "tp": ""                       (string, required) topics data
       ]
     }
   ]
```
**Examples:**
```
>> metabasenet-cli eth_getLogs ["0xa90","0x1"]
<< {}

>> curl -d '{"id":3,"method":"eth_getLogs","jsonrpc":"2.0","params":["0xa90","0x1"]}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":{}}
```
**Errors:**
```
* {"code":-15,"message":"Requested function is obsolete"}
```
##### [Back to top](#commands)
---

