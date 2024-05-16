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


# RPC: makekeypair
def makekeypair():
    result, error = call({
        'id': 0,
        'jsonrpc': '1.0',
        'method': 'makekeypair',
        'params': {}
    })

    if result:
        pubkey = result.get('pubkey')
        privkey = result.get('privkey')
        # print('makekeypair success, pubkey: {}'.format(pubkey))
        return pubkey, privkey
    else:
        raise Exception('makekeypair error: {}'.format(error))


# RPC: getnewkey
def getnewkey():
    result, error = call({
        'id': 0,
        'jsonrpc': '1.0',
        'method': 'getnewkey',
        'params': {
            'passphrase': password
        }
    })

    if result:
        pubkey = result
        # print('getnewkey success, pubkey: {}'.format(pubkey))
        return pubkey
    else:
        raise Exception('getnewkey error: {}'.format(error))


# RPC: getpubkeyaddress
def getpubkeyaddress(pubkey):
    result, error = call({
        'id': 0,
        'jsonrpc': '1.0',
        'method': 'getpubkeyaddress',
        'params': {
            "pubkey": pubkey
        }
    })

    if result:
        address = result
        # print('getpubkeyaddress success, address: {}'.format(address))
        return address
    else:
        raise Exception('getpubkeyaddress error: {}'.format(error))


# RPC: importprivkey
def importprivkey(privkey):
    result, error = call({
        'id': 0,
        'jsonrpc': '1.0',
        'method': 'importprivkey',
        'params': {
            'privkey': privkey,
            'passphrase': password
        }
    })

    if result:
        pubkey = result
        # print('importprivkey success, pubkey: {}'.format(pubkey))
        return pubkey
    else:
        raise Exception('importprivkey error: {}'.format(error))


# RPC: getbalance
def getbalance(addr, forkid=None):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getbalance',
        'params': {
            'address': addr,
            'fork': forkid
        }
    })

    if result:
        if len(result) >= 1:
            avail = result[0].get('avail')
            # print('getbalance success, avail: {}'.format(avail))
            return avail
        else:
            #raise Exception('getbalance result is 0, addr: {}'.format(addr))
            print('getbalance result is 0, addr: {}'.format(addr))
            return -1
    else:
        #raise Exception('getbalance fail, addr: {}'.format(addr))
        print('getbalance fail, error: {}, addr: {}'.format(error, addr))
        return -2

def getbalance_total(addr, forkid=None):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getbalance',
        'params': {
            'address': addr,
            'fork': forkid
        }
    })

    if result:
        if len(result) >= 1:
            avail = result[0].get('avail')
            locked = result[0].get('locked')
            # print('getbalance success, avail: {}'.format(avail))
            return avail + locked
        else:
            #raise Exception('getbalance result is 0, addr: {}'.format(addr))
            print('getbalance result is 0, addr: {}'.format(addr))
            return -1
    else:
        #raise Exception('getbalance fail, addr: {}'.format(addr))
        print('getbalance fail, error: {}, addr: {}'.format(error, addr))
        return -2


def getbalance_locked(addr, forkid=None):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getbalance',
        'params': {
            'address': addr,
            'fork': forkid
        }
    })

    if result:
        if len(result) >= 1:
            avail = result[0].get('avail')
            locked = result[0].get('locked')
            return avail, locked
        else:
            print('getbalance result is 0, addr: {}'.format(addr))
            return -1, 0
    else:
        print('getbalance fail, error: {}, addr: {}'.format(error, addr))
        return -2, 0


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
def sendfrom(from_addr, to, amount, fork=None, type=0, data=None):
    unlockkey(from_addr)

    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'sendfrom',
        'params': {
            'from': from_addr,
            'to': to,
            'amount': amount,
            'fork': fork,
            'type': type,
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
def makeorigin(prev, owner, amount, name, symbol, reward, halvecycle):
    unlockkey(owner)

    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'makeorigin',
        'params': {
            'prev': prev,
            'owner': owner,
            'amount': amount,
            'name': name,
            'symbol': symbol,
            'reward': reward,
            'halvecycle': halvecycle
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


# RPC: addnewtemplate delegate
def adddelegatetemplate(delegate, owner):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'addnewtemplate',
        'params': {
            'type': 'delegate',
            'delegate': {
                'delegate': delegate,
                'owner': owner,
            }
        }
    })

    if result:
        addr = result
        # print('adddelegatetemplate success, template address: {}'.format(addr))
        return addr
    else:
        raise Exception('adddelegatetemplate error: {}'.format(error))


# RPC: addnewtemplate vote
def addvotetemplate(delegate, owner):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'addnewtemplate',
        'params': {
            'type': 'vote',
            'vote': {
                'delegate': delegate,
                'owner': owner
            }
        }
    })

    if result:
        addr = result
        # print('adddelegatetemplate success, template address: {}'.format(addr))
        return addr
    else:
        raise Exception('adddelegatetemplate error: {}'.format(error))


# RPC: maketemplate vote
def makevotetemplate(delegate, owner):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'maketemplate',
        'params': {
            'type': 'vote',
            'vote': {
                'delegate': delegate,
                'owner': owner
            }
        }
    })

    if result:
        addr = result.get('address')
        # print('maketemplate success, template address: {}'.format(addr))
        return addr
    else:
        raise Exception('maketemplate error: {}'.format(error))


# RPC: removetemplate
def removetemplate(address):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'removetemplate',
        'params': {
            'address': address
        }
    })

    if result:
        return result
    else:
        #raise Exception('removetemplate fail, address: {}, error: {}'.format(address, error))
        return "fail"


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
        # print('getforkheight success, height: {}'.format(height))
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
        # print('getblockhash success, block hash: {}'.format(block_hash))
        return block_hash
    else:
        return None


# RPC: getblock
def getblock(blockid):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getblock',
        'params': {
            'block': blockid,
        }
    })

    if result:
        block = result
        # print('getblock success, block: {}'.format(block))
        return block
    else:
        raise Exception('getblock error: {}'.format(error))


# RPC: getblockdetail
def getblockdetail(blockid):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getblockdetail',
        'params': {
            'block': blockid,
        }
    })

    if result:
        block = result
        # print('getblockdetail success, block: {}'.format(block))
        return block
    else:
        raise Exception('getblockdetail error: {}'.format(error))


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


# RPC: getgenealogy
def getgenealogy(forkid):
    result, _ = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getgenealogy',
        'params': {
            'fork': forkid,
        }
    })

    if result:
        return True
    else:
        return False


def createvote(start, count):
    height = getforkheight()
    print('height: {}'.format(height))

    generate_addr = "1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws"
    if testnet:
        generate_addr = "1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb"

    unlockkey(generate_addr)

    for nonce in range(start, start+count):
        vote_addr = addvotetemplate("20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer", "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
        #print('vote_addr: {}'.format(vote_addr))

        while True:
            total_amount = getbalance_total(vote_addr)
            if total_amount >= 0:
                break

        if total_amount > 0:
            print('nonce: {}, addr: {}, added'.format(nonce, vote_addr))
        else:
            while True:
                txid, err = sendfrom(generate_addr, vote_addr, 2000)
                if err < 0:
                    #raise Exception('sendfrom fail, to: {}'.format(vote_addr))
                    print('sendfrom fail, nonce: {}, addr: {}'.format(nonce, vote_addr))
                else:
                    print('vote success, nonce: {}, txid: {}, addr: {}'.format(nonce, txid, vote_addr))
                    break

def createvote_imp(start, count):
    height = getforkheight()
    print('height: {}'.format(height))

    generate_addr = "1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws"
    if testnet:
        generate_addr = "1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb"

    unlockkey(generate_addr)

    nonce = start
    addcount = 0

    while True:
        vote_addr = addvotetemplate("20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer", "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
        #print('vote_addr: {}'.format(vote_addr))

        while True:
            total_amount = getbalance_total(vote_addr)
            if total_amount >= 0:
                break

        if total_amount > 0:
            print('nonce: {}, amount: {}, addr: {}, added'.format(nonce, total_amount, vote_addr))
        else:
            while True:
                txid, err = sendfrom(generate_addr, vote_addr, 2000)
                if err < 0:
                    #raise Exception('sendfrom fail, to: {}'.format(vote_addr))
                    print('sendfrom fail, nonce: {}, addr: {}'.format(nonce, vote_addr))
                else:
                    print('vote success, nonce: {}, txid: {}, addr: {}'.format(nonce, txid, vote_addr))
                    addcount += 1
                    if addcount >= count:
                        return nonce + 1
                    break
        nonce += 1
        if nonce - start >= 1000:
            return nonce

def transvote(start, count):
    generate_addr = "1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws"
    if testnet:
        generate_addr = "1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb"

    unlockkey(generate_addr)
    unlockkey("1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
    for nonce in range(start, start+count):
        vote_addr = makevotetemplate("20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer", "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")

        avail = 0
        while True:
            avail, locked = getbalance_locked(vote_addr)
            if avail >= 0 and locked == 0:
                break
            print('nonce: {}, avail: {}, locked: {}, addr: {}, fail'.format(nonce, avail, locked, vote_addr))
            time.sleep(1)

        if avail <= 0:
            print('nonce: {}, avail: {}, addr: {}, balance is 0'.format(nonce, avail, vote_addr))
        else:
            vote_addr = addvotetemplate("20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer", "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
            while True:
                txid, err = sendfrom(vote_addr, generate_addr, avail-0.01)
                if err < 0:
                    #raise Exception('transvote sendfrom fail, from: {}'.format(vote_addr))
                    print('sendfrom fail, nonce: {}, addr: {}'.format(nonce, vote_addr))
                else:
                    print('sendfrom success, nonce: {}, txid: {}'.format(nonce, txid))
                    break

        removetemplate(vote_addr)

def transvote_imp(start, count):
    generate_addr = "1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws"
    if testnet:
        generate_addr = "1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb"

    unlockkey(generate_addr)
    unlockkey("1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
    for nonce in range(start, start+count):
        vote_addr = makevotetemplate("20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer", "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
        avail, locked = getbalance_locked(vote_addr)
        if locked > 0:
            print('trans locked: nonce: {}, avail: {}, locked: {}, addr: {}'.format(nonce, avail, locked, vote_addr))
            return nonce

        if avail <= 0:
            print('nonce: {}, avail: {}, addr: {}, balance is 0'.format(nonce, avail, vote_addr))
        else:
            vote_addr = addvotetemplate("20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer", "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
            while True:
                txid, err = sendfrom(vote_addr, generate_addr, avail-0.01)
                if err < 0:
                    #raise Exception('transvote sendfrom fail, from: {}'.format(vote_addr))
                    print('trans fail, nonce: {}, addr: {}'.format(nonce, vote_addr))
                else:
                    print('trans success, nonce: {}, txid: {}'.format(nonce, txid))
                    break

        removetemplate(vote_addr)

    return start+count


def transvote_scan(start, count, times):
    generate_addr = "1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws"
    if testnet:
        generate_addr = "1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb"

    unlockkey(generate_addr)
    unlockkey("1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
    for nonce in range(start, start+count):
        vote_addr = makevotetemplate("20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer", "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
        avail, locked = getbalance_locked(vote_addr)
        if locked > 0:
            print('trans locked: nonce: {}, avail: {}, locked: {}, addr: {}'.format(nonce, avail, locked, vote_addr))
            if times >= 120:
                return 0
            else:
                return nonce

        if avail <= 0:
            print('nonce: {}, avail: {}, addr: {}, balance is 0'.format(nonce, avail, vote_addr))
            removetemplate(vote_addr)
        else:
            vote_addr = addvotetemplate("20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer", "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
            while True:
                txid, err = sendfrom(vote_addr, generate_addr, avail-0.01)
                if err < 0:
                    #raise Exception('transvote sendfrom fail, from: {}'.format(vote_addr))
                    print('trans fail, nonce: {}, addr: {}'.format(nonce, vote_addr))
                else:
                    print('trans success, nonce: {}, txid: {}'.format(nonce, txid))
                    break
            removetemplate(vote_addr)

    return start+count

def removevote(start, count):
    for nonce in range(start, start+count):
        vote_addr = makevotetemplate("20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer", "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
        #print('vote_addr: {}'.format(vote_addr))

        while True:
            amount = getbalance_total(vote_addr)
            if amount >= 0:
                break
                
        if amount == 0:
            removetemplate(vote_addr)
            print('remove success, nonce: {}, vote_addr: {}'.format(nonce, vote_addr))
        else:
            print('amount not is 0, amount: {}, nonce: {}, vote_addr: {}'.format(amount, nonce, vote_addr))


def getblocktxcount(height):
    hash = getblockhash(height)[0]
    block = getblock(hash)
    txcount = len(block['tx'])
    print('txcount: {}'.format(txcount))

def getamount(count):
    total = 0
    for nonce in range(0, count):
        vote_addr = makevotetemplate("20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer", "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda")
        #print('vote_addr: {}'.format(vote_addr))

        amount = getbalance(vote_addr)
        if amount != 2000:
            print('amount error, amount: {}, nonce: {}, address: {}'.format(amount, nonce, vote_addr))
        total += amount

    print('total: {}'.format(total))

def auto_vote(start):
    nonce_create = start
    nonce_trans = 0
    locked_count = 0
    while True:
        nonce_create = createvote_imp(nonce_create, 10)
        next_trans = transvote_imp(nonce_trans, 500)
        if next_trans == nonce_trans:
            locked_count += 1
            if locked_count >= 1200:
                locked_count = 0
                next_trans = 0
        if next_trans > nonce_create:
            nonce_trans = 0
        else:
            nonce_trans = next_trans
        if nonce_create > 200000 and nonce_trans > 100000:
            nonce_create = 0
        if nonce_trans > 200100:
            nonce_trans = 0
        time.sleep(1)

def auto_vote_only(start, single):
    nonce_create = start
    while True:
        nonce_create = createvote_imp(nonce_create, single)
        time.sleep(1)

def auto_trans_only(start, single):
    nonce_trans = start
    times = 0
    while True:
        nonce_trans = transvote_scan(nonce_trans, single, times)
        if nonce_trans == 0:
            times = 0
        else:
            times += 1
        time.sleep(1)


if __name__ == "__main__":
    work = "create"

    if len(sys.argv) >= 2:
        work = sys.argv[1]

    if work == "create":
        start=0
        count = 1
        if len(sys.argv) >= 3:
            count = int(sys.argv[2])
        if len(sys.argv) >= 4:
            start = int(sys.argv[3])
        createvote(start, count)
    elif work == "trans":
        start=0
        count = 1
        if len(sys.argv) >= 3:
            count = int(sys.argv[2])
        if len(sys.argv) >= 4:
            start = int(sys.argv[3])
        transvote(start, count)
    elif work == "remove":
        start=0
        count = 1
        if len(sys.argv) >= 3:
            count = int(sys.argv[2])
        if len(sys.argv) >= 4:
            start = int(sys.argv[3])
        removevote(start, count)
    elif work == "getamount":
        if len(sys.argv) >= 3:
            count = int(sys.argv[2])
        getamount(count)
    elif work == "block":
        if len(sys.argv) < 3:
            raise Exception('argv error')
        height = int(sys.argv[2])
        getblocktxcount(height)
    elif work == "autovote":
        start=0
        if len(sys.argv) >= 3:
            start = int(sys.argv[2])
        auto_vote(start)
    elif work == "autovoteonly":
        start = 0
        single = 10
        if len(sys.argv) >= 3:
            start = int(sys.argv[2])
        if len(sys.argv) >= 4:
            single = int(sys.argv[3])
        auto_vote_only(start, single)
    elif work == "autotransonly":
        start = 0
        single = 200
        if len(sys.argv) >= 3:
            start = int(sys.argv[2])
        if len(sys.argv) >= 4:
            single = int(sys.argv[3])
        auto_trans_only(start, single)
    else:
        print('work param error, work: {}'.format(work))


''''
metabasenet.conf:
node1:
mpvssaddress=1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda
mpvsskey=28efbfda61b473c37549d02784648d89fe21ff082b7a42da9ef97b0b83cdb1a9
cryptonightaddress=1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda
cryptonightkey=28efbfda61b473c37549d02784648d89fe21ff082b7a42da9ef97b0b83cdb1a9
listen4

node2:
mpvssaddress=1nh9060wk5t4g2828wd8zw9x4aysvhca4b3f7yd6mmnzhkvrfv9ptxp89
mpvsskey=28efbfda61b473c37549d02784648d89fe21ff082b7a42da9ef97b0b83cdb1a9
cryptonightaddress=1nh9060wk5t4g2828wd8zw9x4aysvhca4b3f7yd6mmnzhkvrfv9ptxp89
cryptonightkey=28efbfda61b473c37549d02784648d89fe21ff082b7a42da9ef97b0b83cdb1a9
listen4
port=7721
rpcport=7722
addnode=127.0.0.1:8811

node3:
mpvssaddress=181x15vfy56wrhaxqspz8mgaex618t6zk7fchemyaxt7qx5txggt9hprc
mpvsskey=28efbfda61b473c37549d02784648d89fe21ff082b7a42da9ef97b0b83cdb1a9
cryptonightaddress=181x15vfy56wrhaxqspz8mgaex618t6zk7fchemyaxt7qx5txggt9hprc
cryptonightkey=28efbfda61b473c37549d02784648d89fe21ff082b7a42da9ef97b0b83cdb1a9
listen4
port=7733
rpcport=7734
addnode=127.0.0.1:8811

# MAINNET
importprivkey 9ae89671cc1a74e9e404a16982ae48d21c56d4ad8278bc9755235a68fc841271 123
unlockkey 1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws 123
importprivkey 42b889a2668eda6d78682c23b5651fb76b5aac2b71caba1aa23b6b14d5ce75b7 123
unlockkey 1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda 123

addnewtemplate delegate '{"delegate": "883f29fb7740f8e625159cb6bcee42cf3ac460be8b9fcb839ccbfeda3744b217", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda"}'
addnewtemplate delegate '{"delegate": "883f29fb7740f8e625159cb6bcee42cf3ac460be8b9fcb839ccbfeda3744b217", "owner": "1nh9060wk5t4g2828wd8zw9x4aysvhca4b3f7yd6mmnzhkvrfv9ptxp89"}'
addnewtemplate delegate '{"delegate": "883f29fb7740f8e625159cb6bcee42cf3ac460be8b9fcb839ccbfeda3744b217", "owner": "181x15vfy56wrhaxqspz8mgaex618t6zk7fchemyaxt7qx5txggt9hprc"}'

sendfrom 1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws 20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer 10000000
sendfrom 1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws 20m00c7gng4w63kjt8r9n6bpfmea7crw7pmkv9rshxf6d6vwhq9bbmfr4 28000000
sendfrom 1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws 20m0997062yn5vzk5ahjmfkyt2bjq8s6dt5xqynjq71k0rxcrbfc94w9x 20000000

# TESTNET
importprivkey 141a6728ded4f83f767ea770e3582be497c5088fcc3b9ca248751887534f5197 123
unlockkey 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 123
importprivkey 42b889a2668eda6d78682c23b5651fb76b5aac2b71caba1aa23b6b14d5ce75b7 123
unlockkey 1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda 123

addnewtemplate delegate '{"delegate": "883f29fb7740f8e625159cb6bcee42cf3ac460be8b9fcb839ccbfeda3744b217", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda"}'
addnewtemplate delegate '{"delegate": "883f29fb7740f8e625159cb6bcee42cf3ac460be8b9fcb839ccbfeda3744b217", "owner": "1nh9060wk5t4g2828wd8zw9x4aysvhca4b3f7yd6mmnzhkvrfv9ptxp89"}'
addnewtemplate delegate '{"delegate": "883f29fb7740f8e625159cb6bcee42cf3ac460be8b9fcb839ccbfeda3744b217", "owner": "181x15vfy56wrhaxqspz8mgaex618t6zk7fchemyaxt7qx5txggt9hprc"}'

sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer 10000000
sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 20m00c7gng4w63kjt8r9n6bpfmea7crw7pmkv9rshxf6d6vwhq9bbmfr4 28000000
sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 20m0997062yn5vzk5ahjmfkyt2bjq8s6dt5xqynjq71k0rxcrbfc94w9x 20000000

'''
