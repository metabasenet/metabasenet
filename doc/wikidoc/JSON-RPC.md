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
  -nowallet                             Launch server without wallet
  -version                              Get metabasenet version
  -purge                                Purge database and blockfile
  -checkrepair                          Check and repair database
  -onlycheck                            Only check database and blockfile
  -blocknotify                          Execute command when the best block changes (%s in cmd is replaced by block hash)
  -logfilesize=<size>                   Log file size(M) (default: 200M)
  -loghistorysize=<size>                Log history size(M) (default: 2048M)
  -fulldb                               Launch server full db
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
 - [listaddress](#listaddress): List all of the addresses from pub keys and template ids
 - [exportwallet](#exportwallet): Export all of keys and templates from wallet to a specified file in JSON format.
 - [importwallet](#importwallet): Import keys and templates from an archived file to the wallet in JSON format.
 - [makeorigin](#makeorigin): Return hex-encoded block.
### Util
 - [verifymessage](#verifymessage): Verify a signed message
 - [makekeypair](#makekeypair): Make a public/private key pair.
 - [getpubkey](#getpubkey): Return public key by address or private key.
 - [getpubkeyaddress](#getpubkeyaddress): Return encoded address for the given public key.
 - [getaddresskey](#getaddresskey): Return address for key.
 - [gettemplateaddress](#gettemplateaddress): Return encoded address for the given template id.
 - [getcontractaddress](#getcontractaddress): Return encoded address for the given contract id.
 - [maketemplate](#maketemplate): Return encoded address for the given template id.
 - [decodetransaction](#decodetransaction): Return a JSON object representing the serialized, hex-encoded transaction.
 - [gettxfee](#gettxfee): Return TxFee for vchData Hex data
 - [makesha256](#makesha256): Make sha256
 - [aesencrypt](#aesencrypt): aes encrypt
 - [aesdecrypt](#aesdecrypt): aes decrypt
### Mint
 - [getwork](#getwork): Get mint work
 - [submitwork](#submitwork): Submit mint work
 - [querystat](#querystat): Query statistical data
 - [reversehex](#reversehex): Reverse a hex string by byte
 - [callcontract](#callcontract): Call contract api
 - [gettransactionreceipt](#gettransactionreceipt): Get transaction receipt
 - [getcontractmuxcode](#getcontractmuxcode): Get contract mux code, including source code, name and code
 - [listcontractcode](#listcontractcode): List contract code
 - [listcontractaddress](#listcontractaddress): List contract address
 - [getdestcontract](#getdestcontract): Get address contract
 - [getcontractsource](#getcontractsource): Get contract source
 - [getcontractcode](#getcontractcode): Get contract code
 - [funcsign](#funcsign): Function sign
 - [makehash](#makehash): Make hash
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
* {"code":-206,"message":"Failed to remove node."}
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
       "name": "",                      (string, required) fork name
       "symbol": "",                    (string, required) fork symbol
       "amount": "",                    (string, required) amount (big float)
       "reward": "",                    (string, required) mint reward (big float)
       "halvecycle": 0,                 (uint, required) halve cycle: 0: fixed reward, >0: blocks of halve cycle
       "isolated": true|false,          (bool, required) is isolated
       "private": true|false,           (bool, required) is private
       "enclosed": true|false,          (bool, required) is enclosed
       "owner": "",                     (string, required) owner's address
       "createtxid": "",                (string, required) create fork txid
       "createforkheight": 0,           (int, required) create fork height
       "parentfork": "",                (string, required) parent fork
       "forkheight": 0,                 (int, required) fork height
       "lastnumber": 0,                 (uint, required) last block number
       "lastblock": "",                 (string, required) last block hash
       "totaltxcount": 0,               (uint, required) total tx count
       "moneysupply": "",               (string, required) money supply (big float)
       "moneydestroy": ""               (string, required) money destroy (big float)
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listfork
<< [{"fork":"00000023e3f9ba506f2ee2f96869f54eb4f23f3141471d57cad55b78ace699ff","name":"MetabaseNet Network","symbol":"BIG","amount":100000000.000000,"reward":15,"halvecycle":0,"isolated":true,"private":false,"enclosed":false,"owner":"1mjw7aa0s7v9sv7x3thvcexxzjz4tq82j5qc12dy29ktqy84haa0j7dwb","createtxid":"5fcef9b220f234b1b1356d545877e55551728bae8484e4a68d425d9242c6860c","forkheight":34,"lastnumber":34,"parentfork":"0000000006854ebdc236f48dbbe5c87312ea0abd7398888374b5ee9a5eb1d291","forktype":"common","forkheight":98,"lastblock":"000000622f660db7b61aa77912070b4d1dade697f7cf98bcddb6c26036d2cb1d","moneysupply":80000.000000,"moneydestroy":20.000000}]

>> {"id":69,"method":"listfork","jsonrpc":"2.0","params":{}}
<< {"id":69,"jsonrpc":"2.0","result":[{"fork":"00000023e3f9ba506f2ee2f96869f54eb4f23f3141471d57cad55b78ace699ff","name":"MetabaseNet Network","symbol":"BIG","amount":100000000.000000,"reward":15,"halvecycle":0,"isolated":true,"private":false,"enclosed":false,"owner":"1mjw7aa0s7v9sv7x3thvcexxzjz4tq82j5qc12dy29ktqy84haa0j7dwb","createtxid":"5fcef9b220f234b1b1356d545877e55551728bae8484e4a68d425d9242c6860c","forkheight":34,"lastnumber":34,"parentfork":"0000000006854ebdc236f48dbbe5c87312ea0abd7398888374b5ee9a5eb1d291","forktype":"common","forkheight":98,"lastblock":"000000622f660db7b61aa77912070b4d1dade697f7cf98bcddb6c26036d2cb1d","moneysupply":80000.000000,"moneydestroy":20.000000}]}
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
   "fork": "",                          (string, required) fork hash
   "height": 0                          (int, required) block height
 }
```
**Examples:**
```
>> metabasenet-cli getblocklocation 609a797ca28042d562b11355038c516d65ba30b91c7033d83c61b81aa8c538e3
<< {"fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","height":1}

>> curl -d '{"id":6,"method":"getblocklocation","jsonrpc":"2.0","params":{"block":"609a797ca28042d562b11355038c516d65ba30b91c7033d83c61b81aa8c538e3"}}' http://127.0.0.1:8812
<< {"id":6,"jsonrpc":"2.0","result":{"fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","height":1}}
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

>> metabasenet-cli getblockcount -f=a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0
<< 32081

>> curl -d '{"id":5,"method":"getblockcount","jsonrpc":"2.0","params":{"fork":"0"}}' http://127.0.0.1:8812
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
<< ["a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"]

>> curl -d '{"id":37,"method":"getblockhash","jsonrpc":"2.0","params":{"height":0}}' http://127.0.0.1:8812
<< {"id":37,"jsonrpc":"2.0","result":["a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"]}
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
<< ["a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"]

>> curl -d '{"id":37,"method":"getblocknumberhash","jsonrpc":"2.0","params":{"number":0}}' http://127.0.0.1:8812
<< {"id":37,"jsonrpc":"2.0","result":["a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"]}
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
   "hashPrev": "",                      (string, required) block prev hash
   "version": 0,                        (uint, required) version
   "type": "",                          (string, required) block type
   "time": 0,                           (uint, required) block time
   "number": 0,                         (uint, required) block number
   "stateroot": "",                     (string, required) state root
   "receiptsroot": "",                  (string, required) receipts root
   "bloom": "",                         (string, required) block bloom
   "fork": "",                          (string, required) fork hash
   "height": 0,                         (uint, required) block height
   "txmint": "",                        (string, required) transaction mint hash
   "tx":                                (array, required, default=RPCValid) transaction hash list
   [
     "tx": ""                           (string, required) transaction hash
   ]
   "prev": ""                           (string, optional) previous block hash
 }
```
**Examples:**
```
>> metabasenet-cli getblock ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c
<< {"hash":"ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c","version":1,"type":"primary-pow","time":1538138566,"prev":"47b86e794e7ce0546def4fe3603d58d9cc9fc87eeee676bd15ae90e45ab51f8a","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","height":31296,"txmint":"3d4ed629c594b924d72480e29a332ca91915be685c85940a8c501f8248269e29","tx":[]}

>> curl -d '{"id":10,"method":"getblock","jsonrpc":"2.0","params":{"block":"ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c"}}' http://127.0.0.1:8812
<< {"id":10,"jsonrpc":"2.0","result":{"hash":"ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c","version":1,"type":"primary-pow","time":1538138566,"prev":"47b86e794e7ce0546def4fe3603d58d9cc9fc87eeee676bd15ae90e45ab51f8a","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","height":31296,"txmint":"3d4ed629c594b924d72480e29a332ca91915be685c85940a8c501f8248269e29","tx":[]}}
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
   "hashPrev": "",                      (string, required) block prev hash
   "version": 0,                        (uint, required) version
   "type": "",                          (string, required) block type
   "time": 0,                           (uint, required) block time
   "number": 0,                         (uint, required) block number
   "stateroot": "",                     (string, required) state root
   "receiptsroot": "",                  (string, required) receipts root
   "bloom": "",                         (string, required) block bloom
   "bits": 0,                           (uint, required) nBits
   "fork": "",                          (string, required) fork hash
   "height": 0,                         (uint, required) block height
   "txmint":                            (object, required) transaction mint data
   {
     "txid": "",                        (string, required) transaction hash
     "version": 0,                      (uint, required) version
     "type": "",                        (string, required) transaction type
     "time": 0,                         (uint, required) transaction timestamp
     "nonce": 0,                        (uint, required) nonce
     "from": "",                        (string, required) from address
     "to": "",                          (string, required) to address
     "amount": "",                      (string, required) amount (big float)
     "gaslimit": 0,                     (uint, required) gas limit
     "gasprice": "",                    (string, required) gas price (big float)
     "gasused": 0,                      (uint, required) gas used
     "txfee": "",                       (string, required) transaction fee (big float)
     "data": "",                        (string, required) data
     "sig": "",                         (string, required) sign
     "fork": "",                        (string, required) fork hash
     "height": 0,                       (int, required) height
     "blockhash": "",                   (string, required) which block tx located in
     "confirmations": 0                 (int, optional) confirmations
   }
   "tx":                                (array, required, default=RPCValid) transaction hash list
   [
     {
       "txid": "",                      (string, required) transaction hash
       "version": 0,                    (uint, required) version
       "type": "",                      (string, required) transaction type
       "time": 0,                       (uint, required) transaction timestamp
       "nonce": 0,                      (uint, required) nonce
       "from": "",                      (string, required) from address
       "to": "",                        (string, required) to address
       "amount": "",                    (string, required) amount (big float)
       "gaslimit": 0,                   (uint, required) gas limit
       "gasprice": "",                  (string, required) gas price (big float)
       "gasused": 0,                    (uint, required) gas used
       "txfee": "",                     (string, required) transaction fee (big float)
       "data": "",                      (string, required) data
       "sig": "",                       (string, required) sign
       "fork": "",                      (string, required) fork hash
       "height": 0,                     (int, required) height
       "blockhash": "",                 (string, required) which block tx located in
       "confirmations": 0               (int, optional) confirmations
     }
   ]
   "prev": ""                           (string, optional) previous block hash
 }
```
**Examples:**
```
>> metabasenet-cli getblockdetail 0000497da49cf85b3f7faabe13716534b6dfd9e287b109356cb6bcb8c795f0d7
<< {"hash":"0000497da49cf85b3f7faabe13716534b6dfd9e287b109356cb6bcb8c795f0d7","hashPrev":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0","version":1,"type":"primary-pow","time":1576134143,"bits":36,"fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","height":18813,"txmint":{"txid":"5df1e5ffc3bea8bb3e86f822e5072aa2843242f1889f3cdc5559f7201569079a","version":1,"type":"work","time":1576134143,"anchor":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0","vin":[],"from":"000000000000000000000000000000000000000000000000000000000","to":"20g075m4mh5trbkdy8vbh74n9h7t3npe7tewpnvsfb19p57jnyf3kdh45","amount":1153.000800,"txfee":0.000000,"data":"","sig":"","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},"tx":[{"txid":"5df1e5eb943a31136951da3afb491d4d6f4e3aeba5cfca9c4ef5ca00c30920b8","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e2d022709018117f93273ca192308776586916ba21ee7e513fe04fab9b2f","vout":0},{"txid":"5df1dee3e78e4526f90d15cdcc8e96fedfe1103eca7065772f2ac2bc41d4ad8f","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1xjzjhz5w0an635wngt5qyx8xbg9x2vg0wzkz1a9hx5c97w0b9k0wbzjt","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f99612f76c34f85b16bee2dd3c06e7cb876fb1ef7d6f7d2160de2d3fd9beb75c9b19e90ed9c46053ce81106e1e37717ad175489f30045180be1a2cc7ae524f790e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5eb90ce47fceb3ace5e463f88db7ec98769e2a77571f5bc4b7e957aa69f","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb943a31136951da3afb491d4d6f4e3aeba5cfca9c4ef5ca00c30920b8","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1c23w26d19h6sntvktzfycpgwfge4efy3zghdjc93mnxbptdw7yqwbwgy","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f9374f46f5caf72e9488a168a64c21744d3160b67d168376420945f9375dfda5a5967678db5f32e26ea17e990f9d00890ae0cc4b72ed0ed8a12f80c2aa6de40700","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5eb8208741b33a708a72a29a7380843789975e000dcc74eb31f857b691e","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e1ea583124fa957a4c995cdea702bbdea2e832f8b0fe9a2d1d32578b8869","vout":0},{"txid":"5df1e5eb90ce47fceb3ace5e463f88db7ec98769e2a77571f5bc4b7e957aa69f","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"11nf1cnwft7f2yhr3qnbx9qxsc0b75y4gn7v1rd1qza8khzjdv7623gmm","amount":999.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f95b134de0889f32099b50f354d754b34b4cebdfab6eaa150f0de465967c020a3f140713591b1faf03d4112fa0924a796aca54be26fb8091ba472e8e0632e7c600","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5eb17690dbeae0609370f87c30b490eaf123500fdd55f961780415d5d22","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb8208741b33a708a72a29a7380843789975e000dcc74eb31f857b691e","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1eenpdhcyk5y3ma5pjzf3gp2drq344mc7vcdep170zmbafg5317ae044h","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f977497144b8d4fc40250ecfc7e007d03f1d6293decd0bb115a0b5939a92314282edce494dde0b992e2b503a1f9f50ae00f6f8bc850fd4cbd5e32771f54bea470f","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5eb3000e640a2bea70acdc4f994fb05c8a36e54480e5cdc0ccc578f9cc0","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e3e1bd39f2188ce116039c8af172b9a6b4a4ae464bef5734b496540d9db4","vout":0},{"txid":"5df1e5eb17690dbeae0609370f87c30b490eaf123500fdd55f961780415d5d22","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1q6xgeqaa9hzy19qhhfpmg7hhee05bnq6ha7ph2kvb1d2w6b8qe8hffcy","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f969d47d443711312df441072eca89c3cea44197bec0a7b709ad9d533684f051081c94ed71bea935a8f2eb224cf3a4cb6fa0c79e8925e68a6b8ff35a3fe196a80f","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5ebb45d6713d5048b73780a2e5e9e36a10c5b432bb409ff930b116ffef1","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb3000e640a2bea70acdc4f994fb05c8a36e54480e5cdc0ccc578f9cc0","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1mp11457nha6830emx3mkv4r2zvtg6aebsacdd72x8gzkpqrpvcn6ygd9","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f9c517184e60c41bde9d504d48a02f815a6de60889175d0307b5810336d1c0eacc63978ff83338a5c7546c6e16d76336ac9c436f95cf9dc9e06928e3df80a7a00e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5eb169ec81721b49ca9325b80afd950a685067aa003ae83b8d6d0d982b0","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e337d09b7d286885cd69c0da98c7d04cf740221019d70bcbbb313a159d2a","vout":0},{"txid":"5df1e5ebb45d6713d5048b73780a2e5e9e36a10c5b432bb409ff930b116ffef1","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1a2fse94fpjepygypmn7qwg8jv8jt2nybqdp2me317tn3t7qy20m9gqmt","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f98b71bda06c23d650ed07d3a0c743689583df2c7f0c26be9c8a75b26e20d3cdafd2bfc6653ce519bd2164ae5f1301b65a220b1a7a34fb0676850f26fcf567c30e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5ebf4b068ad6775a68cb27a95abea0ee058b1d7c356e46fa04e006b255e","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb169ec81721b49ca9325b80afd950a685067aa003ae83b8d6d0d982b0","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"161pk1rj8qkxmbwfcw4131b9qmxqcby7975vhe81h6k3wczg8r7dw7xg2","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f959d19b18b937ef40516e051d8e36a86d86291f2fb2f4db30ba465eb076154b294b0024ab95267f353f095053bd0160999eaeb902c7add7bf11f54ebce1166202","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15}],"prev":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0"}

>> curl -d '{"id":10,"method":"getblockdetail","jsonrpc":"2.0","params":{"block":"0000497da49cf85b3f7faabe13716534b6dfd9e287b109356cb6bcb8c795f0d7"}}' http://127.0.0.1:8812
<< {"id":10,"jsonrpc":"2.0","result":{"hash":"0000497da49cf85b3f7faabe13716534b6dfd9e287b109356cb6bcb8c795f0d7","hashPrev":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0","version":1,"type":"primary-pow","time":1576134143,"bits":36,"fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","height":18813,"txmint":{"txid":"5df1e5ffc3bea8bb3e86f822e5072aa2843242f1889f3cdc5559f7201569079a","version":1,"type":"work","time":1576134143,"anchor":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0","vin":[],"from":"000000000000000000000000000000000000000000000000000000000","to":"20g075m4mh5trbkdy8vbh74n9h7t3npe7tewpnvsfb19p57jnyf3kdh45","amount":1153.000800,"txfee":0.000000,"data":"","sig":"","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},"tx":[{"txid":"5df1e5eb943a31136951da3afb491d4d6f4e3aeba5cfca9c4ef5ca00c30920b8","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e2d022709018117f93273ca192308776586916ba21ee7e513fe04fab9b2f","vout":0},{"txid":"5df1dee3e78e4526f90d15cdcc8e96fedfe1103eca7065772f2ac2bc41d4ad8f","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1xjzjhz5w0an635wngt5qyx8xbg9x2vg0wzkz1a9hx5c97w0b9k0wbzjt","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f99612f76c34f85b16bee2dd3c06e7cb876fb1ef7d6f7d2160de2d3fd9beb75c9b19e90ed9c46053ce81106e1e37717ad175489f30045180be1a2cc7ae524f790e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5eb90ce47fceb3ace5e463f88db7ec98769e2a77571f5bc4b7e957aa69f","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb943a31136951da3afb491d4d6f4e3aeba5cfca9c4ef5ca00c30920b8","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1c23w26d19h6sntvktzfycpgwfge4efy3zghdjc93mnxbptdw7yqwbwgy","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f9374f46f5caf72e9488a168a64c21744d3160b67d168376420945f9375dfda5a5967678db5f32e26ea17e990f9d00890ae0cc4b72ed0ed8a12f80c2aa6de40700","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5eb8208741b33a708a72a29a7380843789975e000dcc74eb31f857b691e","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e1ea583124fa957a4c995cdea702bbdea2e832f8b0fe9a2d1d32578b8869","vout":0},{"txid":"5df1e5eb90ce47fceb3ace5e463f88db7ec98769e2a77571f5bc4b7e957aa69f","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"11nf1cnwft7f2yhr3qnbx9qxsc0b75y4gn7v1rd1qza8khzjdv7623gmm","amount":999.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f95b134de0889f32099b50f354d754b34b4cebdfab6eaa150f0de465967c020a3f140713591b1faf03d4112fa0924a796aca54be26fb8091ba472e8e0632e7c600","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5eb17690dbeae0609370f87c30b490eaf123500fdd55f961780415d5d22","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb8208741b33a708a72a29a7380843789975e000dcc74eb31f857b691e","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1eenpdhcyk5y3ma5pjzf3gp2drq344mc7vcdep170zmbafg5317ae044h","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f977497144b8d4fc40250ecfc7e007d03f1d6293decd0bb115a0b5939a92314282edce494dde0b992e2b503a1f9f50ae00f6f8bc850fd4cbd5e32771f54bea470f","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5eb3000e640a2bea70acdc4f994fb05c8a36e54480e5cdc0ccc578f9cc0","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e3e1bd39f2188ce116039c8af172b9a6b4a4ae464bef5734b496540d9db4","vout":0},{"txid":"5df1e5eb17690dbeae0609370f87c30b490eaf123500fdd55f961780415d5d22","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1q6xgeqaa9hzy19qhhfpmg7hhee05bnq6ha7ph2kvb1d2w6b8qe8hffcy","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f969d47d443711312df441072eca89c3cea44197bec0a7b709ad9d533684f051081c94ed71bea935a8f2eb224cf3a4cb6fa0c79e8925e68a6b8ff35a3fe196a80f","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5ebb45d6713d5048b73780a2e5e9e36a10c5b432bb409ff930b116ffef1","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb3000e640a2bea70acdc4f994fb05c8a36e54480e5cdc0ccc578f9cc0","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1mp11457nha6830emx3mkv4r2zvtg6aebsacdd72x8gzkpqrpvcn6ygd9","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f9c517184e60c41bde9d504d48a02f815a6de60889175d0307b5810336d1c0eacc63978ff83338a5c7546c6e16d76336ac9c436f95cf9dc9e06928e3df80a7a00e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5eb169ec81721b49ca9325b80afd950a685067aa003ae83b8d6d0d982b0","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e337d09b7d286885cd69c0da98c7d04cf740221019d70bcbbb313a159d2a","vout":0},{"txid":"5df1e5ebb45d6713d5048b73780a2e5e9e36a10c5b432bb409ff930b116ffef1","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"1a2fse94fpjepygypmn7qwg8jv8jt2nybqdp2me317tn3t7qy20m9gqmt","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f98b71bda06c23d650ed07d3a0c743689583df2c7f0c26be9c8a75b26e20d3cdafd2bfc6653ce519bd2164ae5f1301b65a220b1a7a34fb0676850f26fcf567c30e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5ebf4b068ad6775a68cb27a95abea0ee058b1d7c356e46fa04e006b255e","version":1,"type":"token","time":1576134123,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb169ec81721b49ca9325b80afd950a685067aa003ae83b8d6d0d982b0","vout":1}],"from":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","to":"161pk1rj8qkxmbwfcw4131b9qmxqcby7975vhe81h6k3wczg8r7dw7xg2","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f959d19b18b937ef40516e051d8e36a86d86291f2fb2f4db30ba465eb076154b294b0024ab95267f353f095053bd0160999eaeb902c7add7bf11f54ebce1166202","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63}],"prev":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0"}}
```
**Errors:**
```
* {"code":-6,"message":"Unknown block."}
```
##### [Back to top](#commands)
---
### getblockdata
**Usage:**
```
        getblockdata (-b="blockhash") (-h=height) (-n=number) (-f="fork")

Return details of a block with given block hash or height or number.
```
**Arguments:**
```
 -b="blockhash"                         (string, optional) block hash
 -h=height                              (uint, optional) block height
 -n=number                              (uint, optional) block number
 -f="fork"                              (string, optional) fork hash
```
**Request:**
```
 "param" :
 {
   "blockhash": "",                     (string, optional) block hash
   "height": 0,                         (uint, optional) block height
   "number": 0,                         (uint, optional) block number
   "fork": ""                           (string, optional) fork hash
 }
```
**Response:**
```
 "result" :
 {
   "hash": "",                          (string, required) block hash
   "hashPrev": "",                      (string, required) block prev hash
   "version": 0,                        (uint, required) version
   "type": "",                          (string, required) block type
   "time": 0,                           (uint, required) block time
   "number": 0,                         (uint, required) block number
   "stateroot": "",                     (string, required) state root
   "receiptsroot": "",                  (string, required) receipts root
   "bloom": "",                         (string, required) block bloom
   "fork": "",                          (string, required) fork hash
   "height": 0,                         (uint, required) block height
   "txmint": "",                        (string, required) transaction mint hash
   "tx":                                (array, required, default=RPCValid) transaction hash list
   [
     "tx": ""                           (string, required) transaction hash
   ]
   "prev": ""                           (string, optional) previous block hash
 }
```
**Examples:**
```
>> metabasenet-cli getblockdata -b=ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c
<< {"hash":"ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c","version":1,"type":"primary-pow","time":1538138566,"prev":"47b86e794e7ce0546def4fe3603d58d9cc9fc87eeee676bd15ae90e45ab51f8a","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","height":31296,"txmint":"3d4ed629c594b924d72480e29a332ca91915be685c85940a8c501f8248269e29","tx":[]}

>> curl -d '{"id":10,"method":"getblockdata","jsonrpc":"2.0","params":{"blockhash":"ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c"}}' http://127.0.0.1:8812
<< {"id":10,"jsonrpc":"2.0","result":{"hash":"ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c","version":1,"type":"primary-pow","time":1538138566,"prev":"47b86e794e7ce0546def4fe3603d58d9cc9fc87eeee676bd15ae90e45ab51f8a","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","height":31296,"txmint":"3d4ed629c594b924d72480e29a332ca91915be685c85940a8c501f8248269e29","tx":[]}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid block"}
* {"code":-6,"message":"Invalid height"}
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
```
##### [Back to top](#commands)
---
### gettransaction
**Usage:**
```
        gettransaction <"txid"> (-s|-nos*serialized*)

Get transaction information
```
**Arguments:**
```
 "txid"                                 (string, required) transaction hash
 -s|-nos*serialized*                    (bool, optional, default=false) If serialized=0, return an Object with information about <txid>.
                                        If serialized is non-zero, return a string that is
                                        serialized, hex-encoded data for <txid>.
```
**Request:**
```
 "param" :
 {
   "txid": "",                          (string, required) transaction hash
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
     "txid": "",                        (string, required) transaction hash
     "version": 0,                      (uint, required) version
     "type": "",                        (string, required) transaction type
     "time": 0,                         (uint, required) transaction timestamp
     "nonce": 0,                        (uint, required) nonce
     "from": "",                        (string, required) from address
     "to": "",                          (string, required) to address
     "amount": "",                      (string, required) amount (big float)
     "gaslimit": 0,                     (uint, required) gas limit
     "gasprice": "",                    (string, required) gas price (big float)
     "gasused": 0,                      (uint, required) gas used
     "txfee": "",                       (string, required) transaction fee (big float)
     "data": "",                        (string, required) data
     "sig": "",                         (string, required) sign
     "fork": "",                        (string, required) fork hash
     "height": 0,                       (int, required) height
     "blockhash": "",                   (string, required) which block tx located in
     "confirmations": 0                 (int, optional) confirmations
   }
 }
```
**Examples:**
```
>> metabasenet-cli gettransaction 5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03
<< {"transaction":{"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","version":1,"type":"token","time":1576046641,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df088912a5a607904d8c69670a8b704c5739e847d2002c862f6b0cd712b797a","vout":0}],"from":"1n56xmva8131c4q0961anv5wdzfqtdctyg9e53fxp65f1jyhbtkfbxz6q","to":"1mkeeh3zeeejsvknz5d0bm78k81s585jbj5kf0rxjx3ah6ngh33b1erg9","amount":162.260000,"txfee":0.100000,"data":"","sig":"d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":1597}}

>> curl -d '{"id":13,"method":"gettransaction","jsonrpc":"2.0","params":{"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","serialized":false}}' http://127.0.0.1:8812
<< {"id":13,"jsonrpc":"2.0","result":{"transaction":{"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","version":1,"type":"token","time":1576046641,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df088912a5a607904d8c69670a8b704c5739e847d2002c862f6b0cd712b797a","vout":0}],"from":"1n56xmva8131c4q0961anv5wdzfqtdctyg9e53fxp65f1jyhbtkfbxz6q","to":"1mkeeh3zeeejsvknz5d0bm78k81s585jbj5kf0rxjx3ah6ngh33b1erg9","amount":162.260000,"txfee":0.100000,"data":"","sig":"d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":1631}}}

>> metabasenet-cli gettransaction -s 5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03
<< {"serialization":"010000003190f05d00000000701af4705c5e6fcb04efc3ca3c851c1e4d8948e10923025f54bea9b000000000017a792b71cdb0f662c802207d849e73c504b7a87096c6d80479605a2a9188f05d0001a4dce88fee73a59dcebf2b40ba1d13407254164b9166f063b2e8d513561118d620e4ab0900000000a0860100000000000040d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000"}

>> curl -d '{"id":13,"method":"gettransaction","jsonrpc":"2.0","params":{"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","serialized":true}}' http://127.0.0.1:8812
<< {"id":13,"jsonrpc":"2.0","result":{"serialization":"010000003190f05d00000000701af4705c5e6fcb04efc3ca3c851c1e4d8948e10923025f54bea9b000000000017a792b71cdb0f662c802207d849e73c504b7a87096c6d80479605a2a9188f05d0001a4dce88fee73a59dcebf2b40ba1d13407254164b9166f063b2e8d513561118d620e4ab0900000000a0860100000000000040d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000"}}
```
**Errors:**
```
* {"code":-6,"message":"No information available about transaction."}
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
>> metabasenet-cli sendtransaction 01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a0860100000000000212348182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e0182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052ed494d90cd96c252446b4a10459fea8c06186154b2bee2ce2182556e9ba40e7e69ddae2501862e4251bba2abf11c90d6f1fd0dec48a1419e81bb8c7d922cf3e03
<< 0a1b944071970589aa524a6f4e40e0b50bab9a64feefc292867692bbf35442a6

>> curl -d '{"id":9,"method":"sendtransaction","jsonrpc":"2.0","params":{"txdata":"01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a0860100000000000212348182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e0182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052ed494d90cd96c252446b4a10459fea8c06186154b2bee2ce2182556e9ba40e7e69ddae2501862e4251bba2abf11c90d6f1fd0dec48a1419e81bb8c7d922cf3e03"}}' http://127.0.0.1:8812
<< {"id":9,"jsonrpc":"2.0","result":"0a1b944071970589aa524a6f4e40e0b50bab9a64feefc292867692bbf35442a6"}
```
**Errors:**
```
* {"code":-8,"message":"TX decode failed"}
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

>> curl -d '{"id":4,"method":"getforkheight","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":4,"jsonrpc":"2.0","result":32081}

>> metabasenet-cli getforkheight -f=a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0
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
        getvotes <"address">

Get votes
```
**Arguments:**
```
 "address"                              (string, required) delegate template address or vote template address
```
**Request:**
```
 "param" :
 {
   "address": ""                        (string, required) delegate template address or vote template address
 }
```
**Response:**
```
 "result": "votes"                      (string, required) number of votes (big float)
```
**Examples:**
```
>> metabasenet-cli getvotes 20m04f7cbzcgqjtj6arnv65s9ap8f1setyezt34kg2q9vdvd1tgspy5r0
<< 70000000.000000

>> curl -d '{"id":1,"method":"getvotes","jsonrpc":"2.0","params":{"address":"20m04f7cbzcgqjtj6arnv65s9ap8f1setyezt34kg2q9vdvd1tgspy5r0"}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":70000000.000000}
```
**Errors:**
```
* {"code" : -6, "message" : "Invalid address"}
* {"code" : -32603, "message" : "Not a delegate template address"}
* {"code" : -32603, "message" : "Vote template address not imported"}
* {"code" : -32603, "message" : "Query failed"}
```
##### [Back to top](#commands)
---
### listdelegate
**Usage:**
```
        listdelegate (-n=count)

List delegate
```
**Arguments:**
```
 -n=count                               (uint, optional, default=0) list count, default 0 is all
```
**Request:**
```
 "param" :
 {
   "count": 0                           (uint, optional, default=0) list count, default 0 is all
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
<< {"address":"20m01802pgptaswc5b2dq09kmj10ns88bn69q0msrnz64mtypx4xm5sff","votes":100002000.000000}

>> curl -d '{"id":1,"method":"listdelegate","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":"{"address":"20m01802pgptaswc5b2dq09kmj10ns88bn69q0msrnz64mtypx4xm5sff","votes":100002000.000000}"}
```
**Errors:**
```
* {"code" : -32603, "message" : "Query failed"}
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
       "key": "",                       (string, required) public key with hex system
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
<< [{"key":"3d266a564ec85f3385babf615b1d7eeb01b3e4456d35174732bb9ec0fa8c8f4f","version": 1,"locked": true},{"key":"58e148d9e8610a6504c26ed346d15920c4d832cf0f03ecb8a016e0d0ec838b1b","version": 1,"locked": true}]

>> curl -d '{"id":43,"method":"listkey","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":43,"jsonrpc":"2.0","result":[{"key":"3d266a564ec85f3385babf615b1d7eeb01b3e4456d35174732bb9ec0fa8c8f4f","version": 1,"locked": true},{"key":"58e148d9e8610a6504c26ed346d15920c4d832cf0f03ecb8a016e0d0ec838b1b","version": 1,"locked": true}]}
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
 "result": "pubkey"                     (string, required) public key
```
**Examples:**
```
>> metabasenet-cli getnewkey 123
<< f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9

>> curl -d '{"id":7,"method":"getnewkey","jsonrpc":"2.0","params":{"passphrase":"123"}}' http://127.0.0.1:8812
<< {"id":7,"jsonrpc":"2.0","result":"f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9"}
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
>> metabasenet-cli removekey f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9 123
<< f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9

>> curl -d '{"id":7,"method":"removekey","jsonrpc":"2.0","params":{"pubkey":"f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9","passphrase":"123"}}' http://127.0.0.1:8812
<< {"id":7,"jsonrpc":"2.0","result":"f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9"}
```
**Errors:**
```
* {"code":-6,"message":"Passphrase must be nonempty"}
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
 "pubkey"                               (string, required) public key
 -new="passphrase"                      (string, required) passphrase of key
 -old="oldpassphrase"                   (string, required) old passphrase of key
```
**Request:**
```
 "param" :
 {
   "pubkey": "",                        (string, required) public key
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
>> encryptkey f4c3babec11363be80e7b6aa1d803d63206a11f36fc99b874b63a262110a0add -new=456 -old=123
<< Encrypt key successfully: f4c3babec11363be80e7b6aa1d803d63206a11f36fc99b874b63a262110a0add

>> curl -d '{"id":5,"method":"encryptkey","jsonrpc":"2.0","params":{"pubkey":"f4c3babec11363be80e7b6aa1d803d63206a11f36fc99b874b63a262110a0add","passphrase":"456","oldpassphrase":"123"}}' http://127.0.0.1:8812
<< {"id":5,"jsonrpc":"2.0","result":"Encrypt key successfully: f4c3babec11363be80e7b6aa1d803d63206a11f36fc99b874b63a262110a0add"}
```
**Errors:**
```
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
>> metabasenet-cli lockkey 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882
<< Lock key successfully: 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882

>> curl -d '{"id":1,"method":"lockkey","jsonrpc":"2.0","params":{"pubkey":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":"Lock key successfully: 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882"}
```
**Errors:**
```
* {"code":-4,"message":"Unknown key"}
* {"code":-6,"message":"This method only accepts pubkey or pubkey address as parameter rather than template address you supplied."}
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
>> metabasenet-cli unlockkey d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07 1234
<< Unlock key successfully: d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07

>> curl -d '{"id":13,"method":"unlockkey","jsonrpc":"2.0","params":{"pubkey":"d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07","passphrase":"1234"}}' http://127.0.0.1:8812
<< {"id":13,"jsonrpc":"2.0","result":"Unlock key successfully: d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07"}

>> metabasenet-cli unlockkey f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9 123 10
<< Unlock key successfully: f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9

>> curl -d '{"id":15,"method":"unlockkey","jsonrpc":"2.0","params":{"pubkey":"f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9","passphrase":"123","timeout":10}}' http://127.0.0.1:8812
<< {"id":15,"jsonrpc":"2.0","result":"Unlock key successfully: f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9"}
```
**Errors:**
```
* {"code":-6,"message":"Passphrase must be nonempty"}
* {"code":-4,"message":"Unknown key"}
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
 "result": "pubkey"                     (string, required) public key with hex number system
```
**Examples:**
```
>> metabasenet-cli importprivkey feb51e048380c0ade1cdb60b25e9f3e05cd4507553a97faadc8a94771fcb1a5b 123
<< d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07

>> curl -d '{"id":9,"method":"importprivkey","jsonrpc":"2.0","params":{"privkey":"feb51e048380c0ade1cdb60b25e9f3e05cd4507553a97faadc8a94771fcb1a5b","passphrase":"123"}}' http://127.0.0.1:8812
<< {"id":9,"jsonrpc":"2.0","result":"d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07"}
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
 "pubkey"                               (string, required) private key
```
**Request:**
```
 "param" :
 {
   "pubkey": ""                         (string, required) private key
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of public key
```
**Examples:**
```
>> metabasenet-cli importpubkey 73f3b3d8545b60e58deb791c4da33089a40d7c6156a89e76e00ac0be2a9924d5
<< 1tmj9janyr05e0xmyn1b62z0dmj4k18td3hwyq3f5c1dn9p5kydsjpvrm

>> curl -d '{"id":31,"method":"importpubkey","jsonrpc":"2.0","params":{"pubkey":"73f3b3d8545b60e58deb791c4da33089a40d7c6156a89e76e00ac0be2a9924d5"}' http://127.0.0.1:8812
<< {"id":31,"jsonrpc":"2.0","result":"73f3b3d8545b60e58deb791c4da33089a40d7c6156a89e76e00ac0be2a9924d5"}
```
**Errors:**
```
* {"code":-6,"message":"Template id is not allowed"}
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
>> metabasenet-cli importkey 642e19a647f9f2b795b8edf97c849ab1866855c9ac6b59d4cf2d9e63d23639de010000002f63a31bed90496a03bb58269e77b98751aa902be47ecbf9ac3adef221cbdcf6ecfba5a9c86e92323fb5af7a2df3f805caaf5dd80caf630e5eb206f0
<< de3936d2639e2dcfd4596bacc9556886b19a847cf9edb895b7f2f947a6192e64

>> curl -d '{"id":3,"method":"importkey","jsonrpc":"2.0","params":{"pubkey":"642e19a647f9f2b795b8edf97c849ab1866855c9ac6b59d4cf2d9e63d23639de010000002f63a31bed90496a03bb58269e77b98751aa902be47ecbf9ac3adef221cbdcf6ecfba5a9c86e92323fb5af7a2df3f805caaf5dd80caf630e5eb206f0"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"de3936d2639e2dcfd4596bacc9556886b19a847cf9edb895b7f2f947a6192e64"}
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
>> metabasenet-cli exportkey de3936d2639e2dcfd4596bacc9556886b19a847cf9edb895b7f2f947a6192e64
<< 642e19a647f9f2b795b8edf97c849ab1866855c9ac6b59d4cf2d9e63d23639de010000002f63a31bed90496a03bb58269e77b98751aa902be47ecbf9ac3adef221cbdcf6ecfba5a9c86e92323fb5af7a2df3f805caaf5dd80caf630e5eb206f0

>> curl -d '{"id":13,"method":"exportkey","jsonrpc":"2.0","params":{"pubkey":"de3936d2639e2dcfd4596bacc9556886b19a847cf9edb895b7f2f947a6192e64"}}' http://127.0.0.1:8812
<< {"id":13,"jsonrpc":"2.0","result":"642e19a647f9f2b795b8edf97c849ab1866855c9ac6b59d4cf2d9e63d23639de010000002f63a31bed90496a03bb58269e77b98751aa902be47ecbf9ac3adef221cbdcf6ecfba5a9c86e92323fb5af7a2df3f805caaf5dd80caf630e5eb206f0"}
```
**Errors:**
```
* {"code":-4,"message":"Unknown key"}
* {"code":-401,"message":"Failed to export key"}
```
##### [Back to top](#commands)
---
### addnewtemplate
**Usage:**
```
        addnewtemplate <"type"> <{delegate}>|<{vote}>|<{fork}>|<{mint}>|<{activatecode}>

Return encoded address for the given template id.
```
**Arguments:**
```
 "type"                                 (string, required) template type
  (if type=delegate)
 {delegate}                             (object, required) a delegate template
  (if type=vote)
 {vote}                                 (object, required) a vote template
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
     "delegate": "",                    (string, required) delegate public key
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
>> metabasenet-cli addnewtemplate mint '{"mint": "e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9", "spent": "1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}'
<< 20g0b87qxcd52ceh9zmpzx0hy46pjfzdnqbkh8f4tqs4y0r6sxyzyny25

>> curl -d '{"id":1,"method":"addnewtemplate","jsonrpc":"2.0","params":{"type":"mint","mint":{"mint":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9","spent":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":"20g0b87qxcd52ceh9zmpzx0hy46pjfzdnqbkh8f4tqs4y0r6sxyzyny25"}

>> metabasenet-cli addnewtemplate delegate '{"delegate":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882","owner":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","rewardratio":500}'

>> metabasenet-cli addnewtemplate fork '{"redeem":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"}'

>> metabasenet-cli addnewtemplate vote '{"delegate": "20m01802pgptaswc5b2dq09kmj10ns88bn69q0msrnz64mtypx4xm5sff", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda", "rewardmode":0}'
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
>> metabasenet-cli importtemplate 0100010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e01b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f402
<< 21w2040000000000000000000000000000000000000000000000epcek

>> curl -d '{"id":52,"method":"importtemplate","jsonrpc":"2.0","params":{"data":"0100010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e01b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f402"}}' http://127.0.0.1:8812
<< {"id":52,"jsonrpc":"2.0","result":"21w2040000000000000000000000000000000000000000000000epcek"}
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
>> metabasenet-cli exporttemplate 2040fpytdr4k5h8tk0nferr7zb51tkccrkgqf341s6tg05q9xe6hth1m
<< 0100010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e01b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f402

>> curl -d '{"id":25,"method":"exporttemplate","jsonrpc":"2.0","params":{"address":"2040fpytdr4k5h8tk0nferr7zb51tkccrkgqf341s6tg05q9xe6hth1m4"}}' http://127.0.0.1:8812
<< {"id":25,"jsonrpc":"2.0","result":"0100010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e01b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f402"}
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
>> metabasenet-cli removetemplate 20m01gbqzmw3nvndx684nsxp1z3tcy4rg0715tgc9sraxd7m56ydttama
<< Success

>> curl -d '{"id":7,"method":"removetemplate","jsonrpc":"2.0","params":{"address":"20m01gbqzmw3nvndx684nsxp1z3tcy4rg0715tgc9sraxd7m56ydttama"}}' http://127.0.0.1:8812
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
        validateaddress <"address"> (-f="fork")

Return information about <address>.
```
**Arguments:**
```
 "address"                              (string, required) wallet address
 -f="fork"                              (string, optional) fork hash (default genesis fork)
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) wallet address
   "fork": ""                           (string, optional) fork hash (default genesis fork)
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
     "pubkey": "",                      (string, required) public key
     (if type=template)
     "template": "",                    (string, required) template type name
     (if type=template && ismine=true)
     "templatedata":                    (object, required) template data
     {
       "type": "",                      (string, required) template type
       "hex": "",                       (string, required) temtplate data
       (if type=delegate)
       "delegate":                      (object, required) delegate template struct
       {
         "delegate": "",                (string, required) delegate public key
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
>> metabasenet-cli validateaddress 20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26
<< {"isvalid":true,"addressdata":{"address":"20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26","ismine":true,"type":"template","template":"mint"}}

>> curl -d '{"id":2,"method":"validateaddress","jsonrpc":"2.0","params":{"address":"20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26"}}' http://127.0.0.1:8812
<< {"id":2,"jsonrpc":"2.0","result":{"isvalid":true,"addressdata":{"address":"20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26","ismine":true,"type":"template","template":"mint"}}}

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
        getbalance (-f="fork") (-a="address") (-p=page) (-n=count)

Get balance of address.
If (address) is not specified, return the balance for wallet's each address.
If (address) is specified, return the balance in the address.
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash, default is genesis
 -a="address"                           (string, optional) address, default is all
 -p=page                                (uint, optional, default=0) page, default is 0
 -n=count                               (uint, optional, default=30) count, default is 30
```
**Request:**
```
 "param" :
 {
   "fork": "",                          (string, optional) fork hash, default is genesis
   "address": "",                       (string, optional) address, default is all
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
<< [{"address":"20g098nza351f53wppg0kfnsbxqf80h3x8fwp9vdmc98fbrgbv6mtjagy","nonce":7,"avail":30.00000000,"locked":0.00000000,"unconfirmedin":0.00000000,"unconfirmedout":0.00000000}]

>> curl -d '{"id":1,"method":"getbalance","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":[{"address":"20g098nza351f53wppg0kfnsbxqf80h3x8fwp9vdmc98fbrgbv6mtjagy","nonce":7,"avail":30.00000000,"locked":0.00000000,"unconfirmedin":0.00000000,"unconfirmedout":0.00000000}]}

>> metabasenet-cli getbalance -a=20g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth8116bm
<< [{"address":"20g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth8116bm","nonce":7,"avail":58.99990000,"locked":0.00000000,"unconfirmedin":13.99990000,"unconfirmedout":0.00000000}]

>> curl -d '{"id":20,"method":"getbalance","jsonrpc":"2.0","params":{"address":"20g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth8116bm"}}' http://127.0.0.1:8812
<< {"id":20,"jsonrpc":"2.0","result":[{"address":"20g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth8116bm","nonce":7,"avail":58.99990000,"locked":0.00000000,"unconfirmedin":13.99990000,"unconfirmedout":0.00000000}]}
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
        listtransaction <"address"> (-f="fork") (-o=offset) (-n=count) (-r|-nor*reverse*)

return up to (count) most recent transactions skipping the first (offset) transactions.
```
**Arguments:**
```
 "address"                              (string, required) address
 -f="fork"                              (string, optional) fork hash. If not set, default is genesis
 -o=offset                              (uint, optional, default=0) query offset. If reverse is true and offset is 0, from last tx
 -n=count                               (uint, optional, default=20) transaction count
 -r|-nor*reverse*                       (bool, optional, default=false) if reverse order, false is positive order, true is reverse order
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) address
   "fork": "",                          (string, optional) fork hash. If not set, default is genesis
   "offset": 0,                         (uint, optional, default=0) query offset. If reverse is true and offset is 0, from last tx
   "count": 0,                          (uint, optional, default=20) transaction count
   "reverse": true|false                (bool, optional, default=false) if reverse order, false is positive order, true is reverse order
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
       "to": "",                        (string, required) to address
       "amount": "",                    (string, required) transaction amount (big float)
       "fee": "",                       (string, required) transaction fee (big float)
       "from": ""                       (string, optional) from address
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listtransaction 20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer -n=2 -o=11 -r
<< [{"txid":"61a774aea9e125d307ed18e34eec05bcce6875acc2531b94008c9b94c816ea7a","txindex":11,"blocknumber":12,"transfertype":"common","txtype":"certification","time":1638364334,"to":"20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer","amount":"0.0000000001","fee":"0.0000000000","from":"20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer"},{"txid":"61a774b84bfed7a6728ea4c4553e480c4c8e7af7c8046d30ba67209b5b9a49de","txindex":10,"blocknumber":12,"transfertype":"common","txtype":"stake","time":1638364344,"to":"20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer","amount":"5.0000000000","fee":"0.0000000000","from":"000000000000000000000000000000000000000000000000000000000"}]

>> curl -d '{"id":2,"method":"listtransaction","jsonrpc":"2.0","params":{"address":"20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer","offset":11,"count":2,"reverse":true}}' http://127.0.0.1:8812
<< {"id":2,"jsonrpc":"2.0","result":[{"txid":"61a774aea9e125d307ed18e34eec05bcce6875acc2531b94008c9b94c816ea7a","txindex":11,"blocknumber":12,"transfertype":"common","txtype":"certification","time":1638364334,"to":"20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer","amount":"0.0000000001","fee":"0.0000000000","from":"20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer"},{"txid":"61a774b84bfed7a6728ea4c4553e480c4c8e7af7c8046d30ba67209b5b9a49de","txindex":10,"blocknumber":12,"transfertype":"common","txtype":"stake","time":1638364344,"to":"20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer","amount":"5.0000000000","fee":"0.0000000000","from":"000000000000000000000000000000000000000000000000000000000"}]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid to address"}
* {"code":-401,"message":"Failed to list transactions"}
* {"code":-32600,"message":"If you need this function, please set config 'fulldb=true' and restart"}
```
##### [Back to top](#commands)
---
### sendfrom
**Usage:**
```
        sendfrom <"from"> <"to"> <"amount"> (-n=nonce) (-p="gasprice") (-g=gas) (-f="fork") (-d="data") (-fd="fdata") (-td="todata") (-cc="contractcode") (-cp="contractparam")

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
 "result": "transaction"                (string, required) transaction id
```
**Examples:**
```
>> metabasenet-cli sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 1nh9060wk5t4g2828wd8zw9x4aysvhca4b3f7yd6mmnzhkvrfv9ptxp89 10
<< 61addf4764ad69c36aff77647f983b9c9441de5cb2428f9ce969130743dcfee0

>> curl -d '{"id":18,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb","to":"1nh9060wk5t4g2828wd8zw9x4aysvhca4b3f7yd6mmnzhkvrfv9ptxp89","amount":10.0000000000}}' http://127.0.0.1:8812
<< {"id":18,"jsonrpc":"2.0","result":"61addf4764ad69c36aff77647f983b9c9441de5cb2428f9ce969130743dcfee0"}

>> metabasenet-cli sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda 10 -f=00000001a8b04e2d045c5c99aec41f3b9e7393bf9c237c0b2d6cf92997da2f0d
<< 61ade019cedde7580b4a44c2cda6b4a2d6fdbb0d2ff0115e9cd4ce7a93d30fd2

>> curl -d '{"id":53,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb","to":"1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda","amount":10.0000000000,"fork":"00000001a8b04e2d045c5c99aec41f3b9e7393bf9c237c0b2d6cf92997da2f0d"}}' http://127.0.0.1:8812
<< {"id":53,"jsonrpc":"2.0","result":"61ade019cedde7580b4a44c2cda6b4a2d6fdbb0d2ff0115e9cd4ce7a93d30fd2"}

>> metabasenet-cli sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 1nh9060wk5t4g2828wd8zw9x4aysvhca4b3f7yd6mmnzhkvrfv9ptxp89 10 -n=2 -p=0.000002 -g=30000 -d=9a87d21254
<< 61aed5c5bc247e361d96f41f2edcedd93a1e6faaaee4f341e75b82a1fc25f84b

>> curl -d '{"id":18,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb","to":"1nh9060wk5t4g2828wd8zw9x4aysvhca4b3f7yd6mmnzhkvrfv9ptxp89","amount":10.0000000000,"nonce":2,"gasprice":"0.000002","gas":30000,"data":"9a87d21254"}}' http://127.0.0.1:8812
<< {"id":18,"jsonrpc":"2.0","result":"61aed5c5bc247e361d96f41f2edcedd93a1e6faaaee4f341e75b82a1fc25f84b"}

>> metabasenet-cli sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 20m0f7zkhbd840hk34zwfeazr8cwkkskxhcdqpz0jw6rx2a160bb38wth 1000000 -td=050017b24437dafecb9c83cb9f8bbe60c43acf42eebcb69c1525e6f84077fb293f880191ba8db6735d7ae45f92841eaaa588a23f1a3be785076fd737ec1911bbd8e757f4010000
<< 61aed6e51699a60f02424ade7dbcded73d31f788eedef8a64c0e3dd024845848

>> curl -d '{"id":18,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb","to":"20m0f7zkhbd840hk34zwfeazr8cwkkskxhcdqpz0jw6rx2a160bb38wth","amount":1000000.0000000000,"todata":"050017b24437dafecb9c83cb9f8bbe60c43acf42eebcb69c1525e6f84077fb293f880191ba8db6735d7ae45f92841eaaa588a23f1a3be785076fd737ec1911bbd8e757f4010000"}}' http://127.0.0.1:8812
<< {"id":18,"jsonrpc":"2.0","result":"61aed6e51699a60f02424ade7dbcded73d31f788eedef8a64c0e3dd024845848"}

>> metabasenet-cli sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 0 0 -cc=<contract code> -cp=<contract param>
<< 61aed7a28e04217f524d434e8d7ca2b84818c56595b040ec2199fec5f52b62b0

>> curl -d '{"id":18,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb","to":"0","amount":0.0000000000,"contractcode":"<contract code>","contractparam":"<contract param>"}}' http://127.0.0.1:8812
<< {"id":18,"jsonrpc":"2.0","result":"61aed7a28e04217f524d434e8d7ca2b84818c56595b040ec2199fec5f52b62b0"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid from address"}
* {"code":-6,"message":"Invalid to address"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
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
>> metabasenet-cli createtransaction 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda 10
<< 010000003ee1ad61cce55a95fe6632d870026256751566288e53d5b6b963846ff038ebab0000000008000000000000000129136f7de86c7a7c13a404cb2401e112f53b5df6c4ac11916c83312aa4c3aff30191ba8db6735d7ae45f92841eaaa588a23f1a3be785076fd737ec1911bbd8e75700e8764817000000000000000000000000000000000000000000000000000000102700000000000000000000000000000000000000000000000000000000000010270000000000000000000000000000000000000000000000000000000000000000

>> curl -d '{"id":59,"method":"createtransaction","jsonrpc":"2.0","params":{"from":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb","to":"1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda","amount":10.0000000000,}}' http://127.0.0.1:8812
<< {"id":59,"jsonrpc":"2.0","result":"010000003ee1ad61cce55a95fe6632d870026256751566288e53d5b6b963846ff038ebab0000000008000000000000000129136f7de86c7a7c13a404cb2401e112f53b5df6c4ac11916c83312aa4c3aff30191ba8db6735d7ae45f92841eaaa588a23f1a3be785076fd737ec1911bbd8e75700e8764817000000000000000000000000000000000000000000000000000000102700000000000000000000000000000000000000000000000000000000000010270000000000000000000000000000000000000000000000000000000000000000"}
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
>> metabasenet-cli signtransaction 010000003ee1ad61cce55a95fe6632d870026256751566288e53d5b6b963846ff038ebab0000000008000000000000000129136f7de86c7a7c13a404cb2401e112f53b5df6c4ac11916c83312aa4c3aff30191ba8db6735d7ae45f92841eaaa588a23f1a3be785076fd737ec1911bbd8e75700e8764817000000000000000000000000000000000000000000000000000000102700000000000000000000000000000000000000000000000000000000000010270000000000000000000000000000000000000000000000000000000000000000
<< {"hex":"010000003ee1ad61cce55a95fe6632d870026256751566288e53d5b6b963846ff038ebab0000000008000000000000000129136f7de86c7a7c13a404cb2401e112f53b5df6c4ac11916c83312aa4c3aff30191ba8db6735d7ae45f92841eaaa588a23f1a3be785076fd737ec1911bbd8e75700e876481700000000000000000000000000000000000000000000000000000010270000000000000000000000000000000000000000000000000000000000001027000000000000000000000000000000000000000000000000000000000000004031451e46035a856453d8d078e22a785d432bd03e95cbcc2e94cd0b3a1e926f0bcdc5074daf21e316d19817ecc7bc1241c36c3e153d798aa0d74f71adfc969c00","complete":true}

>> curl -d '{"id":62,"method":"signtransaction","jsonrpc":"2.0","params":{"txdata":"010000003ee1ad61cce55a95fe6632d870026256751566288e53d5b6b963846ff038ebab0000000008000000000000000129136f7de86c7a7c13a404cb2401e112f53b5df6c4ac11916c83312aa4c3aff30191ba8db6735d7ae45f92841eaaa588a23f1a3be785076fd737ec1911bbd8e75700e8764817000000000000000000000000000000000000000000000000000000102700000000000000000000000000000000000000000000000000000000000010270000000000000000000000000000000000000000000000000000000000000000"}}' http://127.0.0.1:8812
<< {"id":62,"jsonrpc":"2.0","result":{"hex":"01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a0860100000000000212348182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e0182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052ed494d90010000003ee1ad61cce55a95fe6632d870026256751566288e53d5b6b963846ff038ebab0000000008000000000000000129136f7de86c7a7c13a404cb2401e112f53b5df6c4ac11916c83312aa4c3aff30191ba8db6735d7ae45f92841eaaa588a23f1a3be785076fd737ec1911bbd8e75700e876481700000000000000000000000000000000000000000000000000000010270000000000000000000000000000000000000000000000000000000000001027000000000000000000000000000000000000000000000000000000000000004031451e46035a856453d8d078e22a785d432bd03e95cbcc2e94cd0b3a1e926f0bcdc5074daf21e316d19817ecc7bc1241c36c3e153d798aa0d74f71adfc969c00cd96c252446b4a10459fea8c06186154b2bee2ce2182556e9ba40e7e69ddae2501862e4251bba2abf11c90d6f1fd0dec48a1419e81bb8c7d922cf3e03","complete":true}}
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
 "message"                              (string, required) message to be signed. It will be hash(blake2b) before sign
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
   "message": "",                       (string, required) message to be signed. It will be hash(blake2b) before sign
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
>> metabasenet-cli signmessage 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882 123456
<< 045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707

>> curl -d '{"id":4,"method":"signmessage","jsonrpc":"2.0","params":{"pubkey":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882","message":"123456"}}' http://127.0.0.1:8812
<< {"id":4,"jsonrpc":"2.0","result":"045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707"}

>> metabasenet-cli signmessage dc7547feed1d145c21915a739f1fcc75f7b9f88ebe4dba176628844d6916803e 3bdc5190cd3283c81d6b7a186610ce4ada5e81c4f7fcb153b379afc6154d0014
<< 98235a42178cf985dfcb58c9e3f728d38b9c60a92107084adc4ce40e391c51593e57ee109ee7c65aca24627219da8fc8e632a57a013967213630e33b17b4b209

>> metabasenet-cli signmessage dc7547feed1d145c21915a739f1fcc75f7b9f88ebe4dba176628844d6916803e -pubkey=3bdc5190cd3283c81d6b7a186610ce4ada5e81c4f7fcb153b379afc6154d0014 -nop
<< d73de744d34b4e15dbc790638b65692325adddc4723125139a21e53eee282b79095db38ba27d732d41ad00f5fa2071b3290f8ec77ba69fd8f34537786ae10d07
```
**Errors:**
```
* {"code":-4,"message":"Unknown key"}
* {"code":-405,"message":"Key is locked"}
* {"code":-401,"message":"Failed to sign message"}
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
           "delegate": "",              (string, required) delegate public key
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
<< [{"type":"pubkey","address":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","pubkey":"182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e"},{"type":"template","address":"208043ht3c51qztrdfa0f3349pe2m8ajjw1mdb2py68fbckaa2s24tq55","template":"multisig","templatedata":{"type":"multisig","hex":"0200010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052eb9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f4","multisig":{"sigsrequired":1,"addresses":["1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q"]}}},{"type":"template","address":"20g0b87qxcd52ceh9zmpzx0hy46pjfzdnqbkh8f4tqs4y0r6sxyzyny25","template":"mint","templatedata":{"type":"mint","hex":"0400f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e801f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8","mint":{"mint":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1","spent":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}}}]

>> curl -d '{"id":1,"method":"listaddress","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":0,"jsonrpc":"2.0","result":[{"type":"pubkey","address":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","pubkey":"182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e"},{"type":"template","address":"208043ht3c51qztrdfa0f3349pe2m8ajjw1mdb2py68fbckaa2s24tq55","template":"multisig","templatedata":{"type":"multisig","hex":"0200010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052eb9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f4","multisig":{"sigsrequired":1,"addresses":["1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q"]}}},{"type":"template","address":"20g0b87qxcd52ceh9zmpzx0hy46pjfzdnqbkh8f4tqs4y0r6sxyzyny25","template":"mint","templatedata":{"type":"mint","hex":"0400f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e801f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8","mint":{"mint":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1","spent":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}}}]}
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
        makeorigin <"prev"> <"owner"> <"amount"> <"name"> <"symbol"> <"reward"> <halvecycle> (-i|-noi*isolated*) (-p|-nop*private*) (-e|-noe*enclosed*) (-n=nonce)

Return hex-encoded block.
```
**Arguments:**
```
 "prev"                                 (string, required) prev block hash
 "owner"                                (string, required) owner address
 "amount"                               (string, required) amount (big float)
 "name"                                 (string, required) unique fork name
 "symbol"                               (string, required) fork symbol
 "reward"                               (string, required) mint reward (big float)
 halvecycle                             (uint, required) halve cycle: 0: fixed reward, >0: blocks of halve cycle
 -i|-noi*isolated*                      (bool, optional, default=true) is isolated
 -p|-nop*private*                       (bool, optional, default=false) is private
 -e|-noe*enclosed*                      (bool, optional, default=false) is enclosed
 -n=nonce                               (uint, optional, default=0) fork nonce
```
**Request:**
```
 "param" :
 {
   "prev": "",                          (string, required) prev block hash
   "owner": "",                         (string, required) owner address
   "amount": "",                        (string, required) amount (big float)
   "name": "",                          (string, required) unique fork name
   "symbol": "",                        (string, required) fork symbol
   "reward": "",                        (string, required) mint reward (big float)
   "halvecycle": 0,                     (uint, required) halve cycle: 0: fixed reward, >0: blocks of halve cycle
   "isolated": true|false,              (bool, optional, default=true) is isolated
   "private": true|false,               (bool, optional, default=false) is private
   "enclosed": true|false,              (bool, optional, default=false) is enclosed
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
>> metabasenet-cli makeorigin a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0 1p2e0yjz5y1ga705csrzpsp1wre1vjqepkbwnpchyx2vay86wf01pz3qs 1500 test-fork POW 1.0 0
<< {"hash" : "327ec1ffabeae75295401ec69591f845e6025c24d31ae07d9f6e9dec3462bd7a","hex" : "010000ffc06f585ac0afdc176980a0a1ebe849d09b51e4b4c1dd46fb939579d71bdc55809d6f3da600000000000000000000000000000000000000000000000000000000000000004c8001000000e109746573742d666f726be203504f574301a46400000000000000a540420f0000000000e62101b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803010000010000000000000000000000000000000000000000000000000000000000000000000000000001b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803002f685900000000000000000000000009746573742d666f726b000040f253de866a5357b5a11e493162885f4b8a34e94279e29b8354b237aaec650af420b61d24721b4e5e3a4096e9370c77f7dad6c05ced6eedcb2229958671620c05"}

>> curl -d '{"id":4,"method":"makeorigin","jsonrpc":"2.0","params":{"prev":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","owner":"1p2e0yjz5y1ga705csrzpsp1wre1vjqepkbwnpchyx2vay86wf01pz3qs","amount":1500,"name":"test-fork","symbol":"POW","reward":1,"halvecycle":0,"isolated":true,"private":false,"enclosed":false}}' http://127.0.0.1:8812
<< {"id":4,"jsonrpc":"2.0","result":{"hash":"327ec1ffabeae75295401ec69591f845e6025c24d31ae07d9f6e9dec3462bd7a","hex":"010000ffc06f585ac0afdc176980a0a1ebe849d09b51e4b4c1dd46fb939579d71bdc55809d6f3da600000000000000000000000000000000000000000000000000000000000000004c8001000000e109746573742d666f726be203504f574301a46400000000000000a540420f0000000000e62101b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803010000010000000000000000000000000000000000000000000000000000000000000000000000000001b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803002f685900000000000000000000000009746573742d666f726b000040f253de866a5357b5a11e493162885f4b8a34e94279e29b8354b237aaec650af420b61d24721b4e5e3a4096e9370c77f7dad6c05ced6eedcb2229958671620c05"}}

>> metabasenet-cli makeorigin a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0 1p2e0yjz5y1ga705csrzpsp1wre1vjqepkbwnpchyx2vay86wf01pz3qs 1500 test-fork POW 1.0 0 -i -nop -e
<< {"hash":"ade2a8f61082cc0b8e98581e013c52126989d5c15e5c4c6b882f76c98bc68025","hex":"010000ffc06f585ac0afdc176980a0a1ebe849d09b51e4b4c1dd46fb939579d71bdc55809d6f3da600000000000000000000000000000000000000000000000000000000000000004c8001000000e109746573742d666f726be203504f574305a46400000000000000a540420f0000000000e62101b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803010000010000000000000000000000000000000000000000000000000000000000000000000000000001b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803002f685900000000000000000000000009746573742d666f726b00004078bbc21e237d727a58de54e14a19d0244e9dd27ce7d55dfca891158913068185f3dc5b98d4e11500b7512bc8d0e8aa136b34f22f16c16be45e9650e15253980e"}

>> {"id":2,"method":"makeorigin","jsonrpc":"2.0","params":{"prev":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","owner":"1p2e0yjz5y1ga705csrzpsp1wre1vjqepkbwnpchyx2vay86wf01pz3qs","amount":1500,"name":"test-fork","symbol":"POW","reward":1,"halvecycle":0,"isolated":true,"private":false,"enclosed":true}}
<< {"id":2,"jsonrpc":"2.0","result":{"hash":"ade2a8f61082cc0b8e98581e013c52126989d5c15e5c4c6b882f76c98bc68025","hex":"010000ffc06f585ac0afdc176980a0a1ebe849d09b51e4b4c1dd46fb939579d71bdc55809d6f3da600000000000000000000000000000000000000000000000000000000000000004c8001000000e109746573742d666f726be203504f574305a46400000000000000a540420f0000000000e62101b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803010000010000000000000000000000000000000000000000000000000000000000000000000000000001b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803002f685900000000000000000000000009746573742d666f726b00004078bbc21e237d727a58de54e14a19d0244e9dd27ce7d55dfca891158913068185f3dc5b98d4e11500b7512bc8d0e8aa136b34f22f16c16be45e9650e15253980e"}}
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
>> metabasenet-cli verifymessage 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882 123456 045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707
<< true

>> curl -d '{"id":5,"method":"verifymessage","jsonrpc":"2.0","params":{"pubkey":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882","message":"123456","sig":"045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707"}}' http://127.0.0.1:8812
<< {"id":5,"jsonrpc":"2.0","result":true}

>> metabasenet-cli verifymessage 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882 12345 045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707
<< false

>> curl -d '{"id":6,"method":"verifymessage","jsonrpc":"2.0","params":{"pubkey":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882","message":"12345","sig":"045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707"}}' http://127.0.0.1:8812
<< {"id":6,"jsonrpc":"2.0","result":false}
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
   "pubkey": ""                         (string, required) public key
 }
```
**Examples:**
```
>> metabasenet-cli makekeypair
<< {"privkey":"833a5d51d2db84debc0eb3a40d7d41b2723452d211d7e81ce489a95ef48b2324","pubkey":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}

>> curl -d '{"id":42,"method":"makekeypair","jsonrpc":"2.0","params":{}}' http://127.0.0.1:8812
<< {"id":42,"jsonrpc":"2.0","result":{"privkey":"833a5d51d2db84debc0eb3a40d7d41b2723452d211d7e81ce489a95ef48b2324","pubkey":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}}
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
        getpubkey <"privkeyaddress">

Return public key by address or private key.
```
**Arguments:**
```
 "privkeyaddress"                       (string, required) address or private key
```
**Request:**
```
 "param" :
 {
   "privkeyaddress": ""                 (string, required) address or private key
 }
```
**Response:**
```
 "result": "pubkey"                     (string, required) public key
```
**Examples:**
```
>> metabasenet-cli getpubkey 15c02b5f9eb6e516159c230011a87e57757645b53d3534958f910c08feb5c203
<< 06c4246621002576ec70545f04f2cb75378e3f1a16eca2c596fc1c64f52e122b

>> curl -d '{"id":44,"method":"getpubkey","jsonrpc":"2.0","params":{"privkey":"15c02b5f9eb6e516159c230011a87e57757645b53d3534958f910c08feb5c203"}}' http://127.0.0.1:8812
<< {"id":44,"jsonrpc":"2.0","result":"06c4246621002576ec70545f04f2cb75378e3f1a16eca2c596fc1c64f52e122b"}

>> metabasenet-cli getpubkey 1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1
<< e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9

>> curl -d '{"id":44,"method":"getpubkey","jsonrpc":"2.0","params":{"privkey":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}}' http://127.0.0.1:8812
<< {"id":44,"jsonrpc":"2.0","result":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
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
>> metabasenet-cli getpubkeyaddress e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9
<< 1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1

>> curl -d '{"id":44,"method":"getpubkeyaddress","jsonrpc":"2.0","params":{"pubkey":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}}' http://127.0.0.1:8812
<< {"id":44,"jsonrpc":"2.0","result":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
```
##### [Back to top](#commands)
---
### getaddresskey
**Usage:**
```
        getaddresskey <"address">

Return address for key.
```
**Arguments:**
```
 "address"                              (string, required) address
```
**Request:**
```
 "param" :
 {
   "address": ""                        (string, required) address
 }
```
**Response:**
```
 "result": "key"                        (string, required) address of key
```
**Examples:**
```
>> metabasenet-cli getaddresskey 1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda
<< 57e7d8bb1119ec37d76f0785e73b1a3fa288a5aa1e84925fe47a5d73b68dba91

>> curl -d '{"id":44,"method":"getaddresskey","jsonrpc":"2.0","params":{"address":"1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda"}}' http://127.0.0.1:8812
<< {"id":44,"jsonrpc":"2.0","result":"57e7d8bb1119ec37d76f0785e73b1a3fa288a5aa1e84925fe47a5d73b68dba91"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid address"}
```
##### [Back to top](#commands)
---
### gettemplateaddress
**Usage:**
```
        gettemplateaddress <"tid">

Return encoded address for the given template id.
```
**Arguments:**
```
 "tid"                                  (string, required) template id
```
**Request:**
```
 "param" :
 {
   "tid": ""                            (string, required) template id
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of template
```
**Examples:**
```
>> metabasenet-cli gettemplateaddress 2040fpytdr4k5h8tk0nferr7zb51tkccrkgqf341s6tg05q9xe6hth1m4
<< 21w2040000000000000000000000000000000000000000000000epcek

>> curl -d '{"id":53,"method":"gettemplateaddress","jsonrpc":"2.0","params":{"tid":"2040fpytdr4k5h8tk0nferr7zb51tkccrkgqf341s6tg05q9xe6hth1m4"}}' http://127.0.0.1:8812
<< {"id":53,"jsonrpc":"2.0","result":"21w2040000000000000000000000000000000000000000000000epcek"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid tid"}
```
##### [Back to top](#commands)
---
### getcontractaddress
**Usage:**
```
        getcontractaddress <"cid">

Return encoded address for the given contract id.
```
**Arguments:**
```
 "cid"                                  (string, required) contract id
```
**Request:**
```
 "param" :
 {
   "cid": ""                            (string, required) contract id
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of contract
```
**Examples:**
```
>> metabasenet-cli getcontractaddress 114bdaff36abc82402036f4f9c975f7fccfd5dda33cc851c90a05f49e2692550
<< 3a0jpkrj9byg90745sgsxmqfxshznz5ww9xqg60h4s2nkdzyt9c8tw3mt

>> curl -d '{"id":53,"method":"getcontractaddress","jsonrpc":"2.0","params":{"cid":"114bdaff36abc82402036f4f9c975f7fccfd5dda33cc851c90a05f49e2692550"}}' http://127.0.0.1:8812
<< {"id":53,"jsonrpc":"2.0","result":"3a0jpkrj9byg90745sgsxmqfxshznz5ww9xqg60h4s2nkdzyt9c8tw3mt"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid cid"}
```
##### [Back to top](#commands)
---
### maketemplate
**Usage:**
```
        maketemplate <"type"> <{delegate}>|<{vote}>|<{fork}>|<{mint}>|<{activatecode}>

Return encoded address for the given template id.
```
**Arguments:**
```
 "type"                                 (string, required) template type
  (if type=delegate)
 {delegate}                             (object, required) a delegate template
  (if type=vote)
 {vote}                                 (object, required) a vote template
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
     "delegate": "",                    (string, required) delegate public key
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
>> metabasenet-cli maketemplate mint '{"mint": "e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9", "spent": "1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}'
<< {"address":"20g0b87qxcd52ceh9zmpzx0hy46pjfzdnqbkh8f4tqs4y0r6sxyzyny25","hex":"0400f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e801f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8"}

>> curl -d '{"id":1,"method":"maketemplate","jsonrpc":"2.0","params":{"type":"mint","mint":{"mint":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9","spent":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":"{"address":"20g0b87qxcd52ceh9zmpzx0hy46pjfzdnqbkh8f4tqs4y0r6sxyzyny25","hex":"0400f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e801f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8"}"}

>> metabasenet-cli maketemplate delegate '{"delegate":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882","owner":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","rewardratio":500}'

>> metabasenet-cli maketemplate fork '{"redeem":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"}'

>> metabasenet-cli maketemplate mint '{"mint": "e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9", "spent": "1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}'
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
   "txid": "",                          (string, required) transaction hash
   "version": 0,                        (uint, required) version
   "type": "",                          (string, required) transaction type
   "time": 0,                           (uint, required) transaction timestamp
   "nonce": 0,                          (uint, required) nonce
   "from": "",                          (string, required) from address
   "to": "",                            (string, required) to address
   "amount": "",                        (string, required) amount (big float)
   "gaslimit": 0,                       (uint, required) gas limit
   "gasprice": "",                      (string, required) gas price (big float)
   "gasused": 0,                        (uint, required) gas used
   "txfee": "",                         (string, required) transaction fee (big float)
   "data": "",                          (string, required) data
   "sig": "",                           (string, required) sign
   "fork": "",                          (string, required) fork hash
   "height": 0,                         (int, required) height
   "blockhash": "",                     (string, required) which block tx located in
   "confirmations": 0                   (int, optional) confirmations
 }
```
**Examples:**
```
>> metabasenet-cli decodetransaction 010000003190f05d00000000701af4705c5e6fcb04efc3ca3c851c1e4d8948e10923025f54bea9b000000000017a792b71cdb0f662c802207d849e73c504b7a87096c6d80479605a2a9188f05d0001a4dce88fee73a59dcebf2b40ba1d13407254164b9166f063b2e8d513561118d620e4ab0900000000a0860100000000000040d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000
<< {"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","version":1,"type":"token","time":1576046641,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df088912a5a607904d8c69670a8b704c5739e847d2002c862f6b0cd712b797a","vout":0}],"sendfrom":"1n56xmva8131c4q0961anv5wdzfqtdctyg9e53fxp65f1jyhbtkfbxz6q","sendto":"1mkeeh3zeeejsvknz5d0bm78k81s585jbj5kf0rxjx3ah6ngh33b1erg9","amount":162.260000,"txfee":0.100000,"data":"","sig":"d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70"}

>> curl -d '{"id":1,"method":"decodetransaction","jsonrpc":"2.0","params":{"txdata":"010000003190f05d00000000701af4705c5e6fcb04efc3ca3c851c1e4d8948e10923025f54bea9b000000000017a792b71cdb0f662c802207d849e73c504b7a87096c6d80479605a2a9188f05d0001a4dce88fee73a59dcebf2b40ba1d13407254164b9166f063b2e8d513561118d620e4ab0900000000a0860100000000000040d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":{"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","version":1,"type":"token","time":1576046641,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df088912a5a607904d8c69670a8b704c5739e847d2002c862f6b0cd712b797a","vout":0}],"sendfrom":"1n56xmva8131c4q0961anv5wdzfqtdctyg9e53fxp65f1jyhbtkfbxz6q","sendto":"1mkeeh3zeeejsvknz5d0bm78k81s585jbj5kf0rxjx3ah6ngh33b1erg9","amount":162.260000,"txfee":0.100000,"data":"","sig":"d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70"}}
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
>> metabasenet-cli gettxfee feeda13f124bbc
<< 0.01

>> curl -d '{"id":1,"method":"gettxfee","jsonrpc":"2.0","params":{"hexdata":"feeda13f124bbc"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":"0.01"}
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
<< {"hexdata" : "381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e", "sha256" : "5e2ab85020823529d3ee2d8c4c41af4d10c205aa8cb45b139d0e7054e73c11d4"}

>> curl -d '{"id":1,"method":"makesha256","jsonrpc":"2.0","params":{"hexdata":"54ec6b4a77eadc39207697e816e2d2cc6881263ecab078a4c770228e11cbff99"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":{"hexdata" : "381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e", "sha256" : "5e2ab85020823529d3ee2d8c4c41af4d10c205aa8cb45b139d0e7054e73c11d4"}}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### aesencrypt
**Usage:**
```
        aesencrypt <"localaddress"> <"remoteaddress"> <"message">

aes encrypt
```
**Arguments:**
```
 "localaddress"                         (string, required) local address
 "remoteaddress"                        (string, required) remote address
 "message"                              (string, required) message
```
**Request:**
```
 "param" :
 {
   "localaddress": "",                  (string, required) local address
   "remoteaddress": "",                 (string, required) remote address
   "message": ""                        (string, required) message
 }
```
**Response:**
```
 "result": "result"                     (string, required) ciphertext
```
**Examples:**
```
>> metabasenet-cli aesencrypt 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 1jv78wjv22hmzcwv07bkkphnkj51y0kjc7g9rwdm05erwmr2n8tvh8yjn 381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e
<< 06839cf364696762f980640a618d49981a8ab103306605336881eaa112e3693e99325fdd73b9052253de02107e4a373c

>> curl -d '{"id":1,"method":"aesencrypt","jsonrpc":"2.0","params":{"localaddress":"1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm","remoteaddress":"1jv78wjv22hmzcwv07bkkphnkj51y0kjc7g9rwdm05erwmr2n8tvh8yjn","message":"381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":"06839cf364696762f980640a618d49981a8ab103306605336881eaa112e3693e99325fdd73b9052253de02107e4a373c"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid local address."}
* {"code":-6,"message":"Invalid remote address."}
* {"code":-6,"message":"Invalid message."}
* {"code":-401,"message":"Encrypt fail."}
```
##### [Back to top](#commands)
---
### aesdecrypt
**Usage:**
```
        aesdecrypt <"localaddress"> <"remoteaddress"> <"ciphertext">

aes decrypt
```
**Arguments:**
```
 "localaddress"                         (string, required) local address
 "remoteaddress"                        (string, required) remote address
 "ciphertext"                           (string, required) ciphertext
```
**Request:**
```
 "param" :
 {
   "localaddress": "",                  (string, required) local address
   "remoteaddress": "",                 (string, required) remote address
   "ciphertext": ""                     (string, required) ciphertext
 }
```
**Response:**
```
 "result": "result"                     (string, required) message
```
**Examples:**
```
>> metabasenet-cli aesdecrypt 1jv78wjv22hmzcwv07bkkphnkj51y0kjc7g9rwdm05erwmr2n8tvh8yjn 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 06839cf364696762f980640a618d49981a8ab103306605336881eaa112e3693e99325fdd73b9052253de02107e4a373c
<< 381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e

>> curl -d '{"id":1,"method":"aesdecrypt","jsonrpc":"2.0","params":{"localaddress":"1jv78wjv22hmzcwv07bkkphnkj51y0kjc7g9rwdm05erwmr2n8tvh8yjn","remoteaddress":"1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm","ciphertext":"06839cf364696762f980640a618d49981a8ab103306605336881eaa112e3693e99325fdd73b9052253de02107e4a373c"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":"381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid local address."}
* {"code":-6,"message":"Invalid remote address."}
* {"code":-6,"message":"Invalid ciphertext."}
* {"code":-401,"message":"Decrypt fail."}
```
##### [Back to top](#commands)
---
### getwork
**Usage:**
```
        getwork <"spent"> <"privkey"> ("prev")

If (prev hash) is matched with the current primary chain,return true
If next block is not generated by proof-of-work,return false
Otherwise, return formatted proof-of-work parameters to work on:
"prevblockheight" : prevblock height
"prevblockhash" : prevblock hash
"prevblocktime" : prevblock timestamp
"algo" : proof-of-work algorithm: cryptonight=1,...
"bits" : proof-of-work difficulty nbits
"data" : work data
```
**Arguments:**
```
 "spent"                                (string, required) spent address
 "privkey"                              (string, required) private key
 "prev"                                 (string, optional) prev block hash
```
**Request:**
```
 "param" :
 {
   "spent": "",                         (string, required) spent address
   "privkey": "",                       (string, required) private key
   "prev": ""                           (string, optional) prev block hash
 }
```
**Response:**
```
 "result" :
 {
   (if prev is matched or block is not generated by POW)
   "result": true|false,                (bool, optional) result
   (if next block is generated by POW)
   "work":                              (object, optional) work data
   {
     "prevblockheight": 0,              (int, required) prev block height
     "prevblockhash": "",               (string, required) prev block hash
     "prevblocktime": 0,                (uint, required) prev block time
     "algo": 0,                         (int, required) algo
     "bits": 0,                         (int, required) bits
     "data": ""                         (string, required) work data
   }
 }
```
**Examples:**
```
>> metabasenet-cli getwork 1pdr1knaaa4fzr846v89g3q2tzb8pbvbavbbft8xppkky0mqnmsq8gn5y ceae964a1119f110b0cff3614426dd692f8467a95cc2c276e523efc63c5e5031 7ee748e9a827d476d1b4ddb77dc8f9bad779f7b71593d5c5bf73b535e1cc2446
<< {"work":{"prevblockheight":23,"prevblockhash":"f734bb6bc12ab4058532113cfe6a3412d1036eae25f60a97ee1b17effc6e74de","prevblocktime":1538142032,"algo":1,"bits":25,"data":"01000100822fae5bde746efcef171bee970af625ae6e03d112346afe3c11328505b42ac16bbb34f74300000000000000000000000000000000000000000000000000000000000000000001190000000000000000000000000000000000000000000000000000000000000000"}}

>> curl -d '{"id":1,"method":"getwork","jsonrpc":"2.0","params":{"spent":"1pdr1knaaa4fzr846v89g3q2tzb8pbvbavbbft8xppkky0mqnmsq8gn5y","privkey":"ceae964a1119f110b0cff3614426dd692f8467a95cc2c276e523efc63c5e5031","prev":"7ee748e9a827d476d1b4ddb77dc8f9bad779f7b71593d5c5bf73b535e1cc2446"}}' http://127.0.0.1:8812
<< {"id":1,"jsonrpc":"2.0","result":{"work":{"prevblockheight":23,"prevblockhash":"f734bb6bc12ab4058532113cfe6a3412d1036eae25f60a97ee1b17effc6e74de","prevblocktime":1538142032,"algo":1,"bits":25,"data":"01000100822fae5bde746efcef171bee970af625ae6e03d112346afe3c11328505b42ac16bbb34f74300000000000000000000000000000000000000000000000000000000000000000001190000000000000000000000000000000000000000000000000000000000000000"}}}
```
**Errors:**
```
* {"code" : -32603, "message" : "The primary chain is invalid."}
```
##### [Back to top](#commands)
---
### submitwork
**Usage:**
```
        submitwork <"data"> <"spent"> <"privkey">

Attempt to construct and submit new block to network
Return hash of new block.
```
**Arguments:**
```
 "data"                                 (string, required) work data
 "spent"                                (string, required) spent address
 "privkey"                              (string, required) private key
```
**Request:**
```
 "param" :
 {
   "data": "",                          (string, required) work data
   "spent": "",                         (string, required) spent address
   "privkey": ""                        (string, required) private key
 }
```
**Response:**
```
 "result": "hash"                       (string, required) block hash
```
**Examples:**
```
>> metabasenet-cli submitwork 01000100502fae5b4624cce135b573bfc5d59315b7f779d7baf9c87db7ddb4d176d427a8e948e77e43000000000000000000000000000000000000000000000000000000000000000000011acfff020000000000000000000000000000000000000000000000000000000000 1dj5qcjst7eh4tems36n1m500hhyba3vx436t4a8hgdm7r7jrdbf2yqp9 41a9f94395ced97d5066e2d099df4f1e2bd96057f9c38e8ea3f8a02eccd0a98e
<< f734bb6bc12ab4058532113cfe6a3412d1036eae25f60a97ee1b17effc6e74de

>> curl -d '{"id":2,"method":"submitwork","jsonrpc":"2.0","params":{"data":"01000100502fae5b4624cce135b573bfc5d59315b7f779d7baf9c87db7ddb4d176d427a8e948e77e43000000000000000000000000000000000000000000000000000000000000000000011acfff020000000000000000000000000000000000000000000000000000000000","spent":"1dj5qcjst7eh4tems36n1m500hhyba3vx436t4a8hgdm7r7jrdbf2yqp9","privkey":"41a9f94395ced97d5066e2d099df4f1e2bd96057f9c38e8ea3f8a02eccd0a98e"}}' http://127.0.0.1:8812
<< {"id":2,"jsonrpc":"2.0","result":"f734bb6bc12ab4058532113cfe6a3412d1036eae25f60a97ee1b17effc6e74de"}
```
**Errors:**
```
* {"code" : -4, "message" : "Invalid spent address"}
* {"code" : -4, "message" : "Invalid private key"}
* {"code" : -4, "message" : "Invalid mint template"}
* {"code" : -6, "message" : "Block rejected : operation failed"}
* {"code" : -6, "message" : "Block rejected : block proof-of-work is invalid"}
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
                                        -- powblocks: number of POW blocks produced in one minute
                                        -- dposblocks: number of DPOS blocks produced in one minute
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
>> metabasenet-cli reversehex e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9
<< f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8

>> curl -d '{"id":15,"method":"reversehex","jsonrpc":"2.0","params":{"hex":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}}' http://127.0.0.1:8812
<< {"id":15,"jsonrpc":"2.0","result":"f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8"}
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
>> metabasenet-cli callcontract 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 3zfm64bkxqsfn10vhqgghj6hwcqaqhqcsg03gyf9nwba2brrf1h81egmk 0 -cp=8d4b7932
<< {"status":0,"result":"00000000000000000000000000000000000000000000000029a2241af62c0000"}

>> curl -d '{"id":3,"method":"callcontract","jsonrpc":"2.0","params":{"from":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb","to":"3zfm64bkxqsfn10vhqgghj6hwcqaqhqcsg03gyf9nwba2brrf1h81egmk","amount":0.0000000000,"contractparam":"8d4b7932"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"{"status":0,"result":"00000000000000000000000000000000000000000000000029a2241af62c0000"}"}
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
 -f="fork"                              (string, optional) fork hash (default genesis fork)
```
**Request:**
```
 "param" :
 {
   "txid": "",                          (string, required) transaction id
   "fork": ""                           (string, optional) fork hash (default genesis fork)
 }
```
**Response:**
```
 "result" :
 {
   "txid": "",                          (string, required) txid
   "txindex": 0,                        (int, required) tx index
   "blockhash": "",                     (string, required) block hash
   "blocknumber": 0,                    (uint, required) block number
   "from": "",                          (string, required) from address
   "to": "",                            (string, required) to address
   "blockgasused": 0,                   (uint, required) block gas used
   "txgasused": 0,                      (uint, required) tx gas used
   "contractcodehash": "",              (string, required) contract code hash
   "contractaddress": "",               (string, required) contract address
   "contractstatus": 0,                 (int, required) contract status
   "contractgasleft": 0,                (int, required) contract gas left
   "contractresult": "",                (string, required) contract result
   "logs":                              (object, required) logs
   {
     "address": "",                     (string, required) address
     "data": "",                        (string, required) data
     "topics":                          (array, required, default=RPCValid) topics list
     [
       "tp": ""                         (string, required) topics data
     ]
   }
   "logsbloom": ""                      (string, required) logs bloom
 }
```
**Examples:**
```
>> metabasenet-cli gettransactionreceipt 61ade380031b5677d4afa3fc27affcc91a74ed41d48287c63847d08eed2a383c
<< {"txid":"61ade380031b5677d4afa3fc27affcc91a74ed41d48287c63847d08eed2a383c","txindex":5,"blockhash":"00000385ed7b808f3b169f12a190da3432ee76d65a3c9c971da564b22a471e9f","blocknumber":901,"from":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb","to":"300000000000000000000000000000000000000000000000000000000","blockgasused":371723,"txgasused":371723,"contractcodehash":"579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8","contractaddress":"3zfm64bkxqsfn10vhqgghj6hwcqaqhqcsg03gyf9nwba2brrf1h81egmk","contractstatus":0,"contractgasleft":618277,"contractresult":"","logs":{"address":"","data":"","topics":[]},"logsbloom":"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"}

>> curl -d '{"id":3,"method":"gettransactionreceipt","jsonrpc":"2.0","params":{"txid":"61ade380031b5677d4afa3fc27affcc91a74ed41d48287c63847d08eed2a383c"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"{"txid":"61ade380031b5677d4afa3fc27affcc91a74ed41d48287c63847d08eed2a383c","txindex":5,"blockhash":"00000385ed7b808f3b169f12a190da3432ee76d65a3c9c971da564b22a471e9f","blocknumber":901,"from":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb","to":"300000000000000000000000000000000000000000000000000000000","blockgasused":371723,"txgasused":371723,"contractcodehash":"579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8","contractaddress":"3zfm64bkxqsfn10vhqgghj6hwcqaqhqcsg03gyf9nwba2brrf1h81egmk","contractstatus":0,"contractgasleft":618277,"contractresult":"","logs":{"address":"","data":"","topics":[]},"logsbloom":"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"}"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid txid"}
* {"code":-6,"message":"Invalid fork"}
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
>> metabasenet-cli getcontractmuxcode create myname1 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 'code' -t=type1 -d=describe1 -s='source code'
<< 579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8......

>> curl -d '{"id":3,"method":"getcontractmuxcode","jsonrpc":"2.0","params":{"muxtype":"create","name":"myname1","owner":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb","code":"code1","type":"type1","describe":"describe1","source":"source1"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8......"}
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
        listcontractcode (-c="codehash") (-t="txid") (-f="fork")

List contract code
```
**Arguments:**
```
 -c="codehash"                          (string, optional) code hash
 -t="txid"                              (string, optional) up txid
 -f="fork"                              (string, optional) fork
```
**Request:**
```
 "param" :
 {
   "codehash": "",                      (string, optional) code hash
   "txid": "",                          (string, optional) up txid
   "fork": ""                           (string, optional) fork
 }
```
**Response:**
```
 "result" :
   "codedata":                          (array, required, default=RPCValid) 
   [
     {
       "codehash": "",                  (string, required) contract code hash
       "sourcehash": "",                (string, required) contract source hash
       "type": "",                      (string, required) contract code type
       "owner": "",                     (string, required) contract code owner address
       "name": "",                      (string, required) contract code name
       "describe": "",                  (string, required) contract code describe
       "txid": "",                      (string, required) txid of contract code created
       "status": 0                      (int, required) contract code status: 0: not activate, 1: activated
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listcontractcode
<< {"type":"type1@type11","name":"erc20","describe":"erc20test","txid":"61ade380031b5677d4afa3fc27affcc91a74ed41d48287c63847d08eed2a383c","sourcehash":"9358768717ec4837f96d73dbded55932b8865573bf88a6c5b2b5d8023d7fa2de","codehash":"579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8"}

>> curl -d '{"id":3,"method":"listcontractcode","jsonrpc":"2.0","params":{}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"{"type":"type1@type11","name":"erc20","describe":"erc20test","txid":"61ade380031b5677d4afa3fc27affcc91a74ed41d48287c63847d08eed2a383c","sourcehash":"9358768717ec4837f96d73dbded55932b8865573bf88a6c5b2b5d8023d7fa2de","codehash":"579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8"}"}
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
        listcontractaddress (-f="fork")

List contract address
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork
```
**Request:**
```
 "param" :
 {
   "fork": ""                           (string, optional) fork
 }
```
**Response:**
```
 "result" :
   "addressdata":                       (array, required, default=RPCValid) 
   [
     {
       "address": "",                   (string, required) contract address
       "type": "",                      (string, required) contract type
       "owner": "",                     (string, required) contract owner address
       "name": "",                      (string, required) contract name
       "describe": "",                  (string, required) contract describe
       "txid": "",                      (string, required) txid of contract created
       "sourcehash": "",                (string, required) contract source hash
       "codehash": ""                   (string, required) contract code hash
     }
   ]
```
**Examples:**
```
>> metabasenet-cli listcontractaddress
<< {"type":"type1@type11","name":"erc20","describe":"erc20test","txid":"61ade380031b5677d4afa3fc27affcc91a74ed41d48287c63847d08eed2a383c","sourcehash":"9358768717ec4837f96d73dbded55932b8865573bf88a6c5b2b5d8023d7fa2de","codehash":"579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8"}

>> curl -d '{"id":3,"method":"listcontractaddress","jsonrpc":"2.0","params":{}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"{"type":"type1@type11","name":"erc20","describe":"erc20test","txid":"61ade380031b5677d4afa3fc27affcc91a74ed41d48287c63847d08eed2a383c","sourcehash":"9358768717ec4837f96d73dbded55932b8865573bf88a6c5b2b5d8023d7fa2de","codehash":"579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8"}"}
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
        getdestcontract <"address"> (-f="fork")

Get address contract
```
**Arguments:**
```
 "address"                              (string, required) contract address
 -f="fork"                              (string, optional) fork
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) contract address
   "fork": ""                           (string, optional) fork
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
<< {"type":"type1@type11","name":"erc20","describe":"erc20test","txid":"61ade380031b5677d4afa3fc27affcc91a74ed41d48287c63847d08eed2a383c","sourcehash":"9358768717ec4837f96d73dbded55932b8865573bf88a6c5b2b5d8023d7fa2de","codehash":"579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8"}

>> curl -d '{"id":3,"method":"getdestcontract","jsonrpc":"2.0","params":{"address":"3zfm64bkxqsfn10vhqgghj6hwcqaqhqcsg03gyf9nwba2brrf1h81egmk"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"{"type":"type1@type11","name":"erc20","describe":"erc20test","txid":"61ade380031b5677d4afa3fc27affcc91a74ed41d48287c63847d08eed2a383c","sourcehash":"9358768717ec4837f96d73dbded55932b8865573bf88a6c5b2b5d8023d7fa2de","codehash":"579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8"}"}
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
        getcontractsource <"sourcehash"> (-f="fork")

Get contract source
```
**Arguments:**
```
 "sourcehash"                           (string, required) source hash
 -f="fork"                              (string, optional) fork
```
**Request:**
```
 "param" :
 {
   "sourcehash": "",                    (string, required) source hash
   "fork": ""                           (string, optional) fork
 }
```
**Response:**
```
 "result": "contractsource"             (string, required) contract source
```
**Examples:**
```
>> metabasenet-cli getcontractsource 9358768717ec4837f96d73dbded55932b8865573bf88a6c5b2b5d8023d7fa2de
<< 707261676d6120736f6c6964697479205e302e352e303b0a0a636f6e7472616374206572633230207b0a20202f2f205361......

>> curl -d '{"id":3,"method":"getcontractsource","jsonrpc":"2.0","params":{"sourcehash":"9358768717ec4837f96d73dbded55932b8865573bf88a6c5b2b5d8023d7fa2de"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"707261676d6120736f6c6964697479205e302e352e303b0a0a636f6e7472616374206572633230207b0a20202f2f205361......"}
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
        getcontractcode <"codehash"> (-f="fork")

Get contract code
```
**Arguments:**
```
 "codehash"                             (string, required) code hash
 -f="fork"                              (string, optional) fork
```
**Request:**
```
 "param" :
 {
   "codehash": "",                      (string, required) code hash
   "fork": ""                           (string, optional) fork
 }
```
**Response:**
```
 "result": "contractcode"               (string, required) contract code
```
**Examples:**
```
>> metabasenet-cli getcontractcode 579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8
<< 0061736d01000000014e0b60027f7f0060017f0060037f7f7f006000017e60047e7f7f7f017f60057f7e7e7e7e0060067f7e7e7......

>> curl -d '{"id":3,"method":"getcontractcode","jsonrpc":"2.0","params":{"codehash":"579b261853b14cf9d026f1e0027852c52c4dd65845fbac13baff216ba2b04cf8"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"0061736d01000000014e0b60027f7f0060017f0060037f7f7f006000017e60047e7f7f7f017f60057f7e7e7e7e0060067f7e7e7......"}
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
>> metabasenet-cli funcsign 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 35339avavkr9t9mvcdkmft2nay8efb8zsqyyzfen2be1aja4k5nghk08v 1
<< 5eccd2e09fe34ae041d9a90d896759041b2bc2844f43ed6929889b53e826386b

>> curl -d '{"id":3,"method":"funcsign","jsonrpc":"2.0","params":{"from":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"5eccd565e410131217f89ade75e3f89a5a17627edaa7da7638babc94e331ea49"}
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
>> metabasenet-cli makehash 123456789
<< c764360e1f0eb7db438f602a4190754d5305edd82f902a645b38534a046ee897

>> curl -d '{"id":3,"method":"makehash","jsonrpc":"2.0","params":{"data":"123456789"}}' http://127.0.0.1:8812
<< {"id":3,"jsonrpc":"2.0","result":"c764360e1f0eb7db438f602a4190754d5305edd82f902a645b38534a046ee897"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid data"}
```
##### [Back to top](#commands)
---

