#!/usr/bin/env python

import time
import requests
import json
from collections import OrderedDict
import os
import sys
import random
from pprint import pprint
import threading
import signal

COIN = 1000000000000000000
TX_FEE = 0.01

rpcurl_mainnet = 'http://127.0.0.1:8812'
rpcurl_testnet = 'http://127.0.0.1:8814'

genesis_privkey = '9ae89671cc1a74e9e404a16982ae48d21c56d4ad8278bc9755235a68fc841271'
genesis_addr = '1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws'
password = '123'

GENERATE_ADDR_MODE = 0
CREATE_NODE_MODE = 1
CHECK_MODE = 2
mode = GENERATE_ADDR_MODE

testnet = True


# RPC HTTP request
def call(body):
    rpcurl = rpcurl_mainnet
    if testnet:
        rpcurl = rpcurl_testnet
    
    req = requests.post(rpcurl, json=body)

    if mode != GENERATE_ADDR_MODE:
        print('DEBUG: request: {}'.format(body))
        print('DEBUG: response: {}'.format(req.content))

    resp = json.loads(req.content.decode('utf-8'))
    return resp.get('result'), resp.get('error')


# RPC: unlockkey
def unlockkey(key):
    call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'unlockkey',
        'params': {
            'pubkey': key,
            'passphrase': password
        }
    })


# RPC: sendfrom
def sendfrom(from_addr, to, amount, data=None):
    unlockkey(from_addr)

    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'sendfrom',
        'params': {
            'from': from_addr,
            'to': to,
            'amount': str(amount),
            'data': data
        }
    })

    if result:
        txid = result
        # print('sendfrom success, txid: {}'.format(txid))
        return txid, 0
    else:
        # raise Exception('sendfrom error: {}, from: {}, to: {}'.format(error, from_addr, to))
        print('sendfrom error, error: {}'.format(error))
        return "", -1

# RPC: sendfrom
def sendfrom_fork(from_addr, to, amount, fork=None):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'sendfrom',
        'params': {
            'from': from_addr,
            'to': to,
            'amount': str(amount),
            'fork': fork
        }
    })

    if result:
        txid = result
        # print('sendfrom success, txid: {}'.format(txid))
        return txid, 0
    else:
        # raise Exception('sendfrom error: {}, from: {}, to: {}'.format(error, from_addr, to))
        print('sendfrom error, error: {}'.format(error))
        return "", -1

# RPC: gettxpool
def gettxpool(fork):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'gettxpool',
        'params': {
            'fork': fork
        }
    })

    if result:
        return result.get('count')
    else:
        print('gettxpool error, error: {}'.format(error))
        return -1
        

# RPC: makeorigin
def makeorigin(prev, owner, amount, name, symbol, chainid, reward, halvecycle, forktype=None):
    unlockkey(owner)

    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'makeorigin',
        'params': {
            'prev': prev,
            'owner': owner,
            'amount': str(amount),
            'name': name,
            'symbol': symbol,
            'chainid': chainid,
            'reward': str(reward),
            'halvecycle': halvecycle,
            'forktype': forktype,
        }
    })

    if result:
        forkid = result.get('hash')
        data = result.get('hex')
        # print('makeorigin success, forkid: {}, data: {}'.format(forkid, data))
        return forkid, data
    else:
        print(error)
        raise Exception('makeorgin error: {}'.format(error))


# RPC: addnewtemplate fork
def addforktemplate(redeem, forkid):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'addnewtemplate',
        'params': {
            'type': 'fork',
            'fork': {
                'redeem': redeem,
                'fork': forkid,
            }
        }
    })

    if result:
        addr = result
        # print('addforktemplate success, template address: {}'.format(addr))
        return addr
    else:
        raise Exception('addforktemplate error: {}'.format(error))


# RPC: getforkheight
def getforkheight(forkid=None):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getforkheight',
        'params': {
            'fork': forkid,
        }
    })

    if result:
        height = result
        return height
    else:
        return None

# RPC: getblockhash
def getblockhash(height, forkid=None):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getblockhash',
        'params': {
            'height': height,
            'fork': forkid,
        }
    })

    if result:
        block_hash = result
        return block_hash
    else:
        return None


# RPC: gettransaction
def gettransaction(txid):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'gettransaction',
        'params': {
            'txid': txid,
        }
    })

    if result:
        tx = result['transaction']
        # print('gettransaction success, tx: {}'.format(tx))
        return tx
    else:
        raise Exception('gettransaction error: {}'.format(error))


# RPC: listfork
def listfork():
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'listfork',
        'params': {}
    })

    if result:
        return result
    else:
        raise Exception('listfork error: {}'.format(error))

# RPC: makekeypair
def makekeypair():
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'makekeypair',
        'params': {}
    })

    if result:
        return result["privkey"],result["address"]
    else:
        raise Exception('makekeypair error: {}'.format(error))

# RPC: maketemplate
def maketemplate_delegate(mint,owner):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'maketemplate',
        'params': {
            'type': 'delegate',
            'delegate': {
                'mint': mint,
                'owner': owner,
                'rewardratio':500
            }
        }
    })

    if result:
        return result.get('address')
    else:
        raise Exception('maketemplate error: {}'.format(error))

####################################################################
def makenode(nodecount,voteamount):

    maker = []
    cfgp = []
    addt = []
    addresslist = []

    for i in range(0,nodecount):
        privkey_mint,address_mint = makekeypair()
        privkey_owner,address_owner = makekeypair()
        address_delegate = maketemplate_delegate(address_mint,address_owner)
        
        pa="maker mint=%s:%s,owner=%s:%s" % (privkey_mint,address_mint,privkey_owner,address_owner)
        cfgmint="mint=%s:%s:500" % (privkey_mint,address_owner)
        addtemplate = "addnewtemplate delegate \'{\"mint\":\"%s\",\"owner\":\"%s\",\"rewardratio\":500}\'" % (address_mint,address_owner)
        addrstr = "sendfrom 0x5962974eeb0b17b43edabfc9b747839317aa852f %s %s" % (address_delegate, voteamount)

        maker.append(pa)
        cfgp.append(cfgmint)
        addt.append(addtemplate)
        addresslist.append(addrstr)

    for i in maker:
        print(i)

    print('')
    for i in cfgp:
        print(i)

    print('')
    for i in addt:
        print(i)

    print('')
    print('unlockkey 0x5962974eeb0b17b43edabfc9b747839317aa852f 123')
    for i in addresslist:
        print(i)

####################################################################
if __name__ == "__main__":
    nodecount=1
    voteamount=2000000

    if len(sys.argv) >= 2:
        nodecount=int(sys.argv[1])

    if len(sys.argv) >= 3:
        voteamount=int(sys.argv[2])

    makenode(nodecount, voteamount)
    