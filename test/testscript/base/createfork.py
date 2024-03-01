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

GENERATE_ADDR_MODE = 0
CREATE_NODE_MODE = 1
CHECK_MODE = 2
mode = GENERATE_ADDR_MODE
password = '123'

rpcurl_mainnet = 'http://127.0.0.1:8812'
rpcurl_testnet = 'http://127.0.0.1:8814'

testnet = True
rpcurl = rpcurl_testnet


# RPC HTTP request
def call(body):
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
def makeorigin(forktype, prev, owner, amount, name, symbol, chainid, reward, halvecycle):
    unlockkey(owner)

    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'makeorigin',
        'params': {
            'type': forktype,
            'prev': prev,
            'owner': owner,
            'amount': str(amount),
            'name': name,
            'symbol': symbol,
            'chainid': chainid,
            'reward': str(reward),
            'halvecycle': halvecycle
        }
    })

    if result:
        forkid = result.get('hash')
        data = result.get('hex')
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
        return result
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
def checkfork(name, symbol, chainid):
    forklist = listfork()
    for x in forklist:
        if x['name'] == name or x['symbol'] == symbol or x['chainid'] == chainid:
            print('fork exist, forkid: {}, name: {}, symbol: {}, chainid: {}'.format(x['fork'], x['name'], x['symbol'], x['chainid']))
            return False
    return True

def getmaxchainid():
    forklist = listfork()
    maxchainid = 0
    for x in forklist:
        chainid = x['chainid']
        if chainid > maxchainid:
            maxchainid = chainid
    return maxchainid

def createfork(forktype, prev, owner, amount, name, symbol, chainid, reward, halvecycle):
    if not checkfork(name, symbol, chainid):
        print('fork created!')
        return

    prev_block = prev
    if prev == 'height':
        height = getforkheight()
        if testnet:
            block_list = getblockhash(height-1)
        else:
            if height <= 30:
                print('height not enough, height: {}'.format(height))
                return
            block_list = getblockhash(height-30)
        prev_block = block_list[0]

    print('prev block: {}'.format(prev_block))

    forkid, data = makeorigin(forktype, prev_block, owner, amount, name, symbol, chainid, reward, halvecycle)
    print('makeorigin success, forkid: {}'.format(forkid))
    forkaddr = addforktemplate(owner, forkid)
    unlockkey(owner)
    transamount = 100000
    if forktype == 'clonemap':
        transamount = 0
    txid, err = sendfrom(owner, forkaddr, transamount, data)
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
    forktype = 'user'
    prev = 'height'
    if testnet:
        owner = '0x5962974eeb0b17b43edabfc9b747839317aa852f'
    else:
        owner = '0x885c14725a43a6bbc165b2561617c5da8162f170'
    amount = 3000000000
    name = ''
    symbol = ''
    chainid = 0
    reward = 600
    halvecycle = 0
    
    if len(sys.argv) >= 2 and (sys.argv[1] == '-help' or sys.argv[1] == '-h' or sys.argv[1] == '?'):
        print("usage:")
        print("python createfork.py -type <fork type: clonemap, user> -prev <prev block or height> -owner <owner> -amount <amount> -name <name> -symbol <symbol> -chainid <chainid> -reward <reward> -halvecycle <halvecycle> -testnet -mainnet -rpcurl <rpcurl>")
        print("default:")
        if testnet:
            print("net: testnet")
        else:
            print("net: mainnet")
        print("rpcurl: {}".format(rpcurl))
        print("type: {}".format(forktype))
        print("prev: {}".format(prev))
        print("owner: {}".format(owner))
        print("amount: {}".format(amount))
        print("name: {}".format(name))
        print("symbol: {}".format(symbol))
        print("chainid: {}".format(chainid))
        print("reward: {}".format(reward))
        print("halvecycle: {}".format(halvecycle))
    else:
        seturl = False
        setowner = False
        i = 1
        while i < len(sys.argv):
            if sys.argv[i] == '-rpcurl':
                if i+1 >= len(sys.argv):
                    break
                rpcurl = sys.argv[i+1]
                seturl = True
            elif sys.argv[i] == '-type':
                if i+1 >= len(sys.argv):
                    break
                forktype = sys.argv[i+1]
            elif sys.argv[i] == '-prev':
                if i+1 >= len(sys.argv):
                    break
                prev = sys.argv[i+1]
            elif sys.argv[i] == '-owner':
                if i+1 >= len(sys.argv):
                    break
                owner = sys.argv[i+1]
                setowner = True
            elif sys.argv[i] == '-amount':
                if i+1 >= len(sys.argv):
                    break
                amount = int(sys.argv[i+1])
            elif sys.argv[i] == '-name':
                if i+1 >= len(sys.argv):
                    break
                name = sys.argv[i+1]
            elif sys.argv[i] == '-symbol':
                if i+1 >= len(sys.argv):
                    break
                symbol = sys.argv[i+1]
            elif sys.argv[i] == '-chainid':
                if i+1 >= len(sys.argv):
                    break
                chainid = int(sys.argv[i+1])
            elif sys.argv[i] == '-reward':
                if i+1 >= len(sys.argv):
                    break
                reward = int(sys.argv[i+1])
            elif sys.argv[i] == '-halvecycle':
                if i+1 >= len(sys.argv):
                    break
                halvecycle = int(sys.argv[i+1])
            elif sys.argv[i] == '-testnet':
                testnet = True
                i -= 1
            elif sys.argv[i] == '-mainnet':
                testnet = False
                i -= 1
            elif sys.argv[i] == '-autochainid':
                if i+1 >= len(sys.argv):
                    break
                chainid = int(sys.argv[i+1])
                symbol = 'm'+sys.argv[i+1]
                name = 'fork-'+sys.argv[i+1]

            i += 2

        if not seturl:
            if testnet:
                rpcurl = rpcurl_testnet
            else:
                rpcurl = rpcurl_mainnet

        if not setowner:
            if testnet:
                owner = '0x5962974eeb0b17b43edabfc9b747839317aa852f'
            else:
                owner = '0xa7581ec78f00f8e30da9dd5ead10c2abbf125dcb'

        if chainid == 0:
            maxchainid = getmaxchainid()
            if maxchainid == 0:
                maxchainid = 201
            chainid = maxchainid + 1
        if name == '':
            name = 'fork-'+str(chainid)
        if symbol == '':
            symbol = 'SY'+str(chainid)
        if forktype == 'clonemap':
            amount = 0
            reward = 0
            halvecycle = 0
        
        print('create fork!')
        if testnet:
            print("net: testnet")
        else:
            print("net: mainnet")
        print("rpcurl: {}".format(rpcurl))
        print("type: {}".format(forktype))
        print("prev: {}".format(prev))
        print("owner: {}".format(owner))
        print("amount: {}".format(amount))
        print("name: {}".format(name))
        print("symbol: {}".format(symbol))
        print("chainid: {}".format(chainid))
        print("reward: {}".format(reward))
        print("halvecycle: {}".format(halvecycle))
        
        createfork(forktype, prev, owner, amount, name, symbol, chainid, reward, halvecycle)

'''
example:
python createfork.py -autochainid 202
python createfork.py -mainnet
python createfork.py -mainnet -autochainid 203
python createfork.py -prev 0x000001d5013a391895cc53c474d1b643f298837c37ad3b32e661fa206dcec8c9 -owner 0x5962974eeb0b17b43edabfc9b747839317aa852f -amount 500000000 -name fork-02 -symbol m02 -chainid 206 -reward 600 -halvecycle 200000
'''
