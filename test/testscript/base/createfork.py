#!/usr/bin/env python

import time
import requests
import json
from collections import OrderedDict
import os
import sys
import random
from pprint import pprint

COIN = 1000000000000000000
TX_FEE = 0.01

rpcurl_mainnet = 'http://127.0.0.1:6602'
rpcurl_testnet = 'http://127.0.0.1:6604'

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


# RPC: makeorigin
def makeorigin(prev, owner, amount, name, symbol, reward, halvecycle, forktype=None):
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


####################################################################
def checkfork(name, symbol):
    forklist = listfork()
    for x in forklist:
        if x['name'] == name or x['symbol'] == symbol:
            print('fork exist, forkid: {}, name: {}, symbol: {}'.format(x['fork'], x['name'], x['symbol']))
            return False
    return True

def createfork(prev, owner, amount, name, symbol, reward, halvecycle):
    if not checkfork(name, symbol):
        print('fork created!')
        return

    prev_block = prev
    if prev == 'height':
        height = getforkheight()
        block_list = getblockhash(height-1)
        prev_block = block_list[0]

    print('prev block: {}'.format(prev_block))

    forkid, data = makeorigin(prev_block, owner, amount, name, symbol, reward, halvecycle)
    print('makeorigin success, forkid: {}'.format(forkid))
    forkaddr = addforktemplate(owner, forkid)
    unlockkey(owner)
    txid, err = sendfrom(owner, forkaddr, 10000, data)
    if err != 0:
        return
    print('sendfrom success, forkaddr: {}, txid: {}'.format(forkaddr, txid))

    while True:
        txdata = gettransaction(txid)
        if len(txdata['blockhash']) != 0:
            break
        time.sleep(1)
        print('wait......')

    print('create fork success, forkid: {}'.format(forkid))


####################################################################
if __name__ == "__main__":
    print('create fork......')
    
    if len(sys.argv) < 8:
        raise Exception('argv error! need: prev, owner, amount, name, symbol, reward, halvecycle')

    prev = sys.argv[1]
    owner = sys.argv[2]
    amount = int(sys.argv[3])
    name = sys.argv[4]
    symbol = sys.argv[5]
    reward = int(sys.argv[6])
    halvecycle = int(sys.argv[7])
    
    createfork(prev, owner, amount, name, symbol, reward, halvecycle)

'''
example:
python doc/createfork.py 00000000abeb38f06f8463b9b6d5538e2866157556620270d83266fe955ae5cc 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 3000000000 my-test-fork-01 mtd01 6000 0
python3 doc/createfork.py 00000000abeb38f06f8463b9b6d5538e2866157556620270d83266fe955ae5cc 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 3000000000 my-test-fork-01 mtd01 6000 0
'''
