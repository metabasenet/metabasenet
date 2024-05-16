#!/usr/bin/env python

from binascii import hexlify, unhexlify
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

mainnet_genesis_privkey = '9ae89671cc1a74e9e404a16982ae48d21c56d4ad8278bc9755235a68fc841271'
mainnet_genesis_addr = '1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws'
testnet_genesis_privkey = '141a6728ded4f83f767ea770e3582be497c5088fcc3b9ca248751887534f5197'
testnet_genesis_addr = '1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb'
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


def get_genesis_privkey():
    if testnet:
        return testnet_genesis_privkey
    else:
        return mainnet_genesis_privkey

def get_genesis_addr():
    if testnet:
        return testnet_genesis_addr
    else:
        return mainnet_genesis_addr

# RPC: makekeypair
def makekeypair():
    result, error = call({
        'id': 0,
        'jsonrpc': '2.0',
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
        'jsonrpc': '2.0',
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
        'jsonrpc': '2.0',
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


# RPC: getaddresskey
def getaddresskey(address):
    result, error = call({
        'id': 0,
        'jsonrpc': '2.0',
        'method': 'getaddresskey',
        'params': {
            "address": address
        }
    })
    if result:
        return result
    else:
        raise Exception('getaddresskey error: {}'.format(error))


# RPC: importprivkey
def importprivkey(privkey):
    result, error = call({
        'id': 0,
        'jsonrpc': '2.0',
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
def sendfrom(from_addr, to, amount, fork=None, data=None, contractcode=None, contractparam=None):
    unlockkey(from_addr)

    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'sendfrom',
        'params': {
            'from': from_addr,
            'to': to,
            'amount': str(amount),
            'fork': fork,
            'data': data,
            'contractcode': contractcode,
            'contractparam': contractparam
        }
    })

    if result:
        txid = result
        return txid, 0
    else:
        print('sendfrom error, error: {}'.format(error))
        return "", -1

# RPC: sendfrom_td
def sendfrom_td(from_addr, to, amount, td):
    unlockkey(from_addr)

    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'sendfrom',
        'params': {
            'from': from_addr,
            'to': to,
            'amount': str(amount),
            'todata': td
        }
    })

    if result:
        txid = result
        return txid, 0
    else:
        print('sendfrom error, error: {}'.format(error))
        return "", -1

# RPC: createcontract
def createcontract(from_addr, to_addr, amount, fork, contractcode, contractparam):
    unlockkey(from_addr)

    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'sendfrom',
        'params': {
            'from': from_addr,
            'to': to_addr,
            'amount': str(amount),
            'fork': fork,
            'contractcode': contractcode,
            'contractparam': contractparam
        }
    })

    if result:
        txid = result
        return txid, 0
    else:
        print('createcontract sendfrom error, error: {}'.format(error))
        return "", -1

# RPC: createmuxcontract
def createmuxcontract(from_addr, to_addr, amount, fork, fdata, contractparam):
    unlockkey(from_addr)

    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'sendfrom',
        'params': {
            'from': from_addr,
            'to': to_addr,
            'amount': str(amount),
            'fork': fork,
            'fdata': fdata,
            'contractparam': contractparam
        }
    })

    if result:
        txid = result
        return txid, 0
    else:
        print('createmuxcontract sendfrom error, error: {}'.format(error))
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
            'amount': str(amount),
            'name': name,
            'symbol': symbol,
            'reward': str(reward),
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
        return addr
    else:
        raise Exception('addforktemplate error: {}'.format(error))


# RPC: addnewtemplate delegate
def adddelegatetemplate(delegate, owner, rewardratio):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'addnewtemplate',
        'params': {
            'type': 'delegate',
            'delegate': {
                'delegate': delegate,
                'owner': owner,
                'rewardratio': rewardratio
            }
        }
    })

    if result:
        addr = result
        return addr
    else:
        raise Exception('adddelegatetemplate error: {}'.format(error))


# RPC: addnewtemplate vote
def addvotetemplate(delegate, owner, rewardmode):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'addnewtemplate',
        'params': {
            'type': 'vote',
            'vote': {
                'delegate': delegate,
                'owner': owner,
                'rewardmode': rewardmode
            }
        }
    })

    if result:
        addr = result
        return addr
    else:
        raise Exception('adddelegatetemplate error: {}'.format(error))


# RPC: maketemplate vote
def makevotetemplate(delegate, owner, rewardmode):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'maketemplate',
        'params': {
            'type': 'vote',
            'vote': {
                'delegate': delegate,
                'owner': owner,
                'rewardmode': rewardmode
            }
        }
    })

    if result:
        addr = result.get('address')
        hex = result.get('hex')
        return addr,hex
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


# RPC: funcsign
def funcsign(funcname):
    result, _ = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'funcsign',
        'params': {
            'funcname': funcname,
        }
    })
    if result:
        return result
    else:
        raise Exception('funcsign error: {}'.format(error))

# RPC: getpubkey
def getpubkey(address):
    result, _ = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getpubkey',
        'params': {
            'privkeyaddress': address,
        }
    })
    if result:
        return result
    else:
        raise Exception('getpubkey error: {}'.format(error))

# RPC: reversehex
def reversehex(value):
    result, _ = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'reversehex',
        'params': {
            'hex': value,
        }
    })
    if result:
        return result
    else:
        raise Exception('reversehex error: {}'.format(error))

# RPC: callcontract
def callcontract(from_addr, to, amount, fork=None, contractparam=None):
    unlockkey(from_addr)

    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'callcontract',
        'params': {
            'from': from_addr,
            'to': to,
            'amount': str(amount),
            'fork': fork,
            'contractparam': contractparam
        }
    })

    if result:
        return result
    else:
        print('callcontract error, error: {}'.format(error))
        return ""

# RPC: gettransactionreceipt
def gettransactionreceipt(txid, fork=None):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'gettransactionreceipt',
        'params': {
            'txid': txid,
            'fork': fork
        }
    })
    if result:
        return result,0
    else:
        return "",1

# RPC: getcontractmuxcode
def getcontractmuxcode(muxtype, name, code, typein, describe, source):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getcontractmuxcode',
        'params': {
            'muxtype': muxtype,
            'name': name,
            'code': code,
            'type': typein,
            'describe': describe,
            'source': source
        }
    })
    if result:
        return result,0
    else:
        return "",1

# RPC: getdestcontract
def getdestcontract(fork, address):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getdestcontract',
        'params': {
            'address': address,
            'fork': fork
        }
    })
    if result:
        return result,0
    else:
        return "",1

# RPC: getcontractsource
def getcontractsource(fork, sourcehash):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getcontractsource',
        'params': {
            'sourcehash': sourcehash,
            'fork': fork
        }
    })
    if result:
        return result,0
    else:
        return "",1

# RPC: getcontractcode
def getcontractcode(fork, codehash):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getcontractcode',
        'params': {
            'codehash': codehash,
            'fork': fork
        }
    })
    if result:
        return result,0
    else:
        return "",1

# RPC: makehash
def makehash(data):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'makehash',
        'params': {
            'data': data
        }
    })
    if result:
        return result,0
    else:
        return "",1


#############################################################################
def createvote(delegateaddr, voteamount):
    pubkey, privkey = makekeypair()
    owner = getpubkeyaddress(pubkey)
    voteaddr, votehex = makevotetemplate(delegateaddr, owner, 0)
    genesis_addr = get_genesis_addr()
    if voteamount < 100:
        voteamount = random.randint(100, 10000)
    txid, ret = sendfrom_td(genesis_addr, voteaddr, voteamount, votehex)
    if ret != 0:
        print('sendfrom_td fail, voteaddr: {}'.format(voteaddr))
        return "", "", 0, 1
    return voteaddr, owner, voteamount, 0

def votetest(delegateaddr, sleeptime, voteamount, maxcount):
    gen_privkey = get_genesis_privkey()
    gen_pubkey = importprivkey(gen_privkey)
    unlockkey(gen_pubkey)
    count = 0
    while True:
        voteaddr, owner, sendamount, ret = createvote(delegateaddr, voteamount)
        count += 1
        if ret == 0:
            print('{}: create vote success, voteaddr: {}, votes: {}'.format(count, voteaddr, sendamount))
        else:
            print('{}: create vote fail'.format(count))
        if maxcount > 0 and count >= maxcount:
            print('complete {}'.format(count))
            break
        if sleeptime > 0:
            time.sleep(sleeptime)

def txtest(fromaddr, toaddr, sleeptime, singletxcount, amount, forkid, maxcount):
    unlockkey(fromaddr)
    count = 0
    stxcount = singletxcount
    while True:
        count += 1
        sendamount = amount
        if amount < 0.000001:
            sendamount = random.randint(1, 1000) / 1000000
        txid, ret = sendfrom(fromaddr, toaddr, sendamount, forkid)
        if ret != 0:
            print('{}: send tx fail, amount: {}'.format(count, sendamount))
        else:
            print('{}: send tx success, txid: {}, amount: {}'.format(count, txid, sendamount))
        if maxcount > 0 and count >= maxcount:
            print('complete {}'.format(count))
            break
        stxcount -= 1
        if stxcount <= 0 and sleeptime > 0:
            time.sleep(sleeptime)
            stxcount = singletxcount

#############################################################################
if __name__ == "__main__":
    func = "non"

    if len(sys.argv) >= 2:
        func = sys.argv[1]

    if func == 'votetest':
        delegateaddr = "20m0f7zkhbd840hk34zwfeazr8cwkkskxhcdqpz0jw6rx2a160bb38wth"
        sleeptime = 2
        voteamount = 2000
        maxcount = 0
        if len(sys.argv) >= 3:
            delegateaddr = sys.argv[2]
        if len(sys.argv) >= 4:
            sleeptime = int(sys.argv[3])
        if len(sys.argv) >= 5:
            voteamount = int(sys.argv[4])
        if len(sys.argv) >= 6:
            maxcount = int(sys.argv[5])
        votetest(delegateaddr, sleeptime, voteamount, maxcount)
    if func == 'txtest':
        if len(sys.argv) < 5:
            raise Exception('param error, need 5, argv count: {}'.format(len(sys.argv)))
        fromaddr = sys.argv[2]
        toaddr = sys.argv[3]
        amount = float(sys.argv[4])
        sleeptime = 2
        singletxcount = 1
        maxcount = 0
        forkid = None
        if len(sys.argv) >= 6:
            sleeptime = int(sys.argv[5])
        if len(sys.argv) >= 7:
            singletxcount = int(sys.argv[6])
        if len(sys.argv) >= 8:
            maxcount = int(sys.argv[7])
        if len(sys.argv) >= 9:
            forkid = sys.argv[8]
        txtest(fromaddr, toaddr, sleeptime, singletxcount, amount, forkid, maxcount)
    else:
        print('func error, func: {}'.format(func))


#############################################################################
# nohup python vote-test.py votetest > vote-test-py.log & 
