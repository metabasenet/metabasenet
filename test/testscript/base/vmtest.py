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

# RPC: sendfrom
def sendfromfull(from_addr, to_addr, amount, nonce=None, gasprice=None, gas=None, fork=None, data=None, fdata=None, contractcode=None, contractparam=None):
    unlockkey(from_addr)

    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'sendfrom',
        'params': {
            'from': from_addr,
            'to': to_addr,
            'amount': str(amount),
            'nonce': nonce,
            'gasprice': gasprice,
            'gas': gas,
            'fork': fork,
            'data': data,
            'fdata': fdata,
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
        return tx,0
    else:
        return "",1


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
def getcontractmuxcode(muxtype, name, owner, code, typein, describe, source):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getcontractmuxcode',
        'params': {
            'muxtype': muxtype,
            'name': name,
            'owner': owner,
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

# RPC: listcontractcode
def listcontractcode(txid=None,fork=None):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'listcontractcode',
        'params': {
            'txid': txid,
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

#############################################################################
def funcdata(funcname, param):
    return funcsign(funcname)+param

def getreverseaddresskey(address):
    return getaddresskey(address)[2:66]

def waittransactionreceipt(txid, fork=None):
    while True:
        time.sleep(1)
        tr,err = gettransactionreceipt(txid, fork)
        if err == 0:
            print('status: {}, contract address: {}'.format(tr["contractstatus"],tr["contractaddress"]))
            break
        print('wait...')

def waitcontractcode(txid,fork=None):
    for i in range(0, 60):
        time.sleep(1)
        tx,err=gettransaction(txid)
        if err == 0:
            if tx['blockhash'] != "":
                codelist,err=listcontractcode(txid,fork)
                codehash=""
                if err == 0:
                    for c in codelist:
                        if c['txid'] == txid:
                            codehash = c['codehash']
                            #print('codehash: {}'.format(codehash))
                            break
                return codehash
        print('[{}]  wait...'.format(i))

def waitcontractcodeactivate():
    for i in range(0, 60):
        codelist,err=listcontractcode(None,None)
        if err == 0:
            fNoActivate = False
            for c in codelist:
                if c['status'] == 0:
                    fNoActivate = True
                    break
            if not fNoActivate:
                print('All activate!')
                break
        print('[{}]  wait...'.format(i))
        time.sleep(1)
        
def getgenesisfork():
    forklist = listfork()
    for x in forklist:
        if x['parentfork'] == '0x0000000000000000000000000000000000000000000000000000000000000000' and x['createforkheight'] == 0:
            return x['fork']
    return ''

def createContract(fromaddress, file, fork, cparam):
    f = open(file,'rb')
    wasmcode = f.read().hex()
    f.close()

    unlockkey(fromaddress)
    txid,err = createcontract(fromaddress, '0x0', 0, fork, wasmcode, cparam)
    print('createcontract ret: {}, txid: {}'.format(err,txid))

    waittransactionreceipt(txid, fork)

    return txid

###========================================
# erc20.sol
def balanceOf(from_addr, contract_addr, account_addr, fork=None):
    account_key = getreverseaddresskey(account_addr)
    fdata = funcdata('balanceOf(address)', account_key)
    unlockkey(from_addr)
    ret = callcontract(from_addr, contract_addr, 0, fork, fdata)
    print(ret)

###========================================
# test.sol
def test2(fromaddress, toaddress, contract_addr, fork=None, nonce=0):
    if nonce == 0:
        nonce = int(time.time())
    contract_key = getreverseaddresskey(contract_addr)
    nonce_str = '{:0>64x}'.format(nonce)
    fdata = funcdata('test2(address,uint256)', contract_key+nonce_str)
    print('contract_addr: {}'.format(contract_addr))
    print('contract_key: {}'.format(contract_key))

    unlockkey(fromaddress)
    txid,ret = sendfrom(fromaddress, toaddress, 0, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret,txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)
    return txid

def test3(fromaddress, toaddress, contract_addr, address, fork=None):
    contract_key = getreverseaddresskey(contract_addr)
    address_key = getreverseaddresskey(address)
    fdata = funcdata('test3(address,address)', contract_key+address_key)

    unlockkey(fromaddress)
    txid,ret = sendfrom(fromaddress, toaddress, 0, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret, txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)
    return txid
    
def test4(fromaddress, toaddress, contract_addr, address, amount, fork=None):
    contract_key = getreverseaddresskey(contract_addr)
    address_key = getreverseaddresskey(address)
    fdata = funcdata('test4(address,address)', contract_key+address_key)

    unlockkey(fromaddress)
    txid,ret = sendfrom(fromaddress, toaddress, amount, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret, txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)
    return txid
    
def test5(fromaddress, toaddress, recvaddress, amount, fork=None):
    recvaddress_key = getreverseaddresskey(recvaddress)
    amount_str = '{:0>64x}'.format(amount*COIN)
    fdata = funcdata('test5(address,uint256)', recvaddress_key+amount_str)

    unlockkey(fromaddress)
    txid, ret = sendfrom(fromaddress, toaddress, 0, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret, txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)
    return txid
    
def test6(fromaddress, toaddress, recvaddress, amount, fork=None):
    recvaddress_key = getreverseaddresskey(recvaddress)
    amount_str = '{:0>64x}'.format(amount*COIN)
    fdata = funcdata('test6(address,uint256)', recvaddress_key+amount_str)

    unlockkey(fromaddress)
    txid,ret = sendfrom(fromaddress, toaddress, 0, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret,txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)
    return txid

def test7(fromaddress, toaddress, contract_addr, address, fork=None):
    contract_key = getreverseaddresskey(contract_addr)
    address_key = getreverseaddresskey(address)
    fdata = funcdata('test7(address,address)', contract_key+address_key)

    unlockkey(fromaddress)
    txid,ret = sendfrom(fromaddress, toaddress, 0, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret,txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)
    return txid
    
def transfer(cmd_addr, contract_addr, account_addr, amount, fork=None):
    account_key = getreverseaddresskey(account_addr)
    amount_str = '{:0>64x}'.format(amount*COIN)
    fdata = funcdata('transfer(address,uint256)', account_key+amount_str)
    unlockkey(cmd_addr)
    txid,ret = sendfrom(cmd_addr, contract_addr, 0, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret,txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)

def delegatecalltest(fromaddress, toaddress, contract_addr, fork=None):
    contract_key = getreverseaddresskey(contract_addr)
    fdata = funcdata('delegatecalltest(address)', contract_key)
    unlockkey(fromaddress)
    ret = callcontract(fromaddress, contract_addr, 0, fork, fdata)
    print(ret)

def calltest(fromaddress, toaddress, contract_addr, fork=None):
    contract_key = getreverseaddresskey(contract_addr)
    fdata = funcdata('calltest(address)', contract_key)
    unlockkey(fromaddress)
    ret = callcontract(fromaddress, contract_addr, 0, fork, fdata)
    print(ret)

##============================
# WETH9.sol
def deposit(from_addr, contract_addr, amount, fork=None):
    fdata = funcdata('deposit()', '')
    unlockkey(from_addr)
    txid,ret = sendfrom(from_addr, contract_addr, amount, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret,txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)

def withdraw(from_addr, contract_addr, amoun, fork=None):
    amount_str = '{:0>64x}'.format(amount*COIN)
    fdata = funcdata('withdraw(uint256)', amount_str)
    unlockkey(from_addr)
    txid,ret = sendfrom(from_addr, contract_addr, 0, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret,txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)

def transfer_weth9(from_addr, contract_addr, account_addr, amount, fork=None):
    account_key = getreverseaddresskey(account_addr)
    amount_str = '{:0>64x}'.format(amount*COIN)
    fdata = funcdata('transfer(address,uint256)', account_key+amount_str)
    unlockkey(from_addr)
    txid,ret = sendfrom(from_addr, contract_addr, 0, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret,txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)

def totalSupply(from_addr, contract_addr, fork=None):
    fdata = funcdata('totalSupply()', '')
    unlockkey(from_addr)
    ret = callcontract(from_addr, contract_addr, 0, fork, fdata)
    print(ret)

def balanceOfAddress(from_addr, contract_addr, account_addr, fork=None):
    account_key = getreverseaddresskey(account_addr)
    fdata = funcdata('balanceOfAddress(address)', account_key)
    unlockkey(from_addr)
    ret = callcontract(from_addr, contract_addr, 0, fork, fdata)
    print(ret)

def balance(from_addr, contract_addr, fork=None):
    fdata = funcdata('balance()', '')
    unlockkey(from_addr)
    ret = callcontract(from_addr, contract_addr, 0, fork, fdata)
    print(ret)

def approve_weth9(from_addr, contract_addr, guy_addr, wad_amount, fork=None):
    guy_key = getreverseaddresskey(guy_addr)
    amount_str = '{:0>64x}'.format(wad_amount*COIN)
    fdata = funcdata('approve(address,uint256)', guy_key+amount_str)
    unlockkey(from_addr)
    txid,ret = sendfrom(from_addr, contract_addr, 0, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret,txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)

def transferFrom(from_addr, contract_addr, src_addr, dst_addr, wad_amount, fork=None):
    src_key = getreverseaddresskey(src_addr)
    dst_key = getreverseaddresskey(dst_addr)
    amount_str = '{:0>64x}'.format(wad_amount*COIN)
    fdata = funcdata('transferFrom(address,address,uint256)', src_key+dst_key+amount_str)
    unlockkey(from_addr)
    txid,ret = sendfrom(from_addr, contract_addr, 0, fork, None, None, fdata)
    print('sendfrom ret: {}, txid: {}'.format(ret,txid))
    if ret == 0:
        waittransactionreceipt(txid, fork)

def muxcontracttest(fromaddress, fork, cparam, muxtype, name, owner, codefile, typein, describe, sourcefile):
    code = "None"
    source = "None"
    if muxtype == 'setup':
        code = codefile
    else:
        fcode = open(codefile,'rb')
        code = fcode.read().hex()
        fcode.close()

        fsource = open(sourcefile,'rb')
        source = fsource.read().hex()
        fsource.close()

    muxcode, ret = getcontractmuxcode(muxtype, name, owner, code, typein, describe, source)
    if ret != 0:
        print('getcontractmuxcode fail')
        return ""

    unlockkey(fromaddress)
    txid,err = createmuxcontract(fromaddress, '0x0', 0, fork, muxcode, cparam)
    print('createmuxcontract ret: {}, txid: {}'.format(err,txid))

    if err == 0:
        if muxtype == 'upcode':
            codehash=waitcontractcode(txid, fork)
            if codehash == "":
                print('upcode fail, txid: {}'.format(txid))
            else:
                print('upcode success, code hash: {}, txid: {}'.format(codehash, txid))
        else:
            waittransactionreceipt(txid, fork)

    return txid

def getsource(address, outfile, fork):
    cinfo, err = getdestcontract(fork, address)
    if err != 0:
        print('getdestcontract fail, address: {}'.format(address))
        return 
    sourcehash = cinfo["sourcehash"]
    srccode, err = getcontractsource(fork, sourcehash)
    if err != 0:
        print('getcontractsource fail, sourcehash: {}'.format(sourcehash))
        return 
    srccode_bytes = bytes.fromhex(srccode)
    f = open(outfile, "wb")
    f.write(srccode_bytes)
    f.close()

def getcode(address, outfile, fork):
    cinfo, err = getdestcontract(fork, address)
    if err != 0:
        print('getdestcontract fail, address: {}'.format(address))
        return 
    codehash = cinfo["codehash"]
    code, err = getcontractcode(fork, codehash)
    if err != 0:
        print('getcontractcode fail, codehash: {}'.format(codehash))
        return 
    code_bytes = bytes.fromhex(code)
    f = open(outfile, "wb")
    f.write(code_bytes)
    f.close()

def filemakehash(filename):
    f = open(filename,'rb')
    datahex = f.read().hex()
    f.close()
    hash, err = makehash(datahex)
    if err != 0:
        print('makehash fail')
        return 
    print('hash: {}'.format(hash))


#############################################################################
if __name__ == "__main__":
    func = "non"

    if len(sys.argv) >= 2:
        func = sys.argv[1]

    if func == 'create':
        if len(sys.argv) < 4:
            raise Exception('param error, need 4, argv count: {}'.format(len(sys.argv)))
        fork = None
        cparam = None
        if len(sys.argv) >= 5:
            fork = sys.argv[4]
        if len(sys.argv) >= 6:
            #cparam = sys.argv[5]
            amount = int(sys.argv[5])
            cparam = '{:0>64x}'.format(amount*COIN)
        createContract(sys.argv[2], sys.argv[3], fork, cparam)

    elif func == "muxcontracttest":
        if len(sys.argv) < 12:
            raise Exception('param error, need 12, argv count: {}'.format(len(sys.argv)))

        fromaddress = sys.argv[2]
        fork = sys.argv[3]
        if fork == 'genesisfork':
            fork = getgenesisfork()
        cparam = '{:0>64x}'.format(int(sys.argv[4])*COIN)
        muxtype = sys.argv[5]
        name = sys.argv[6]
        owner = sys.argv[7]
        codefile = sys.argv[8]
        typein = sys.argv[9]
        describe = sys.argv[10]
        sourcefile = sys.argv[11]

        muxcontracttest(fromaddress, fork, cparam, muxtype, name, owner, codefile, typein, describe, sourcefile)
    
    elif func == "waitcontractcodeactivate":
        waitcontractcodeactivate()

    # erc20.sol
    elif func == 'balanceOf':
        if len(sys.argv) < 5:
            raise Exception('param error, need 6, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 6:
            fork = sys.argv[5]
        balanceOf(sys.argv[2], sys.argv[3], sys.argv[4], fork)

    # test.sol
    elif func == 'test2':
        if len(sys.argv) < 5:
            raise Exception('param error, need 5, argv count: {}'.format(len(sys.argv)))
        nonce = 0
        fork = None
        if len(sys.argv) >= 6:
            fork = sys.argv[5]
        if len(sys.argv) >= 7:
            nonce = int(sys.argv[6])
        test2(sys.argv[2], sys.argv[3], sys.argv[4], fork, nonce)
    elif func == 'test3':
        if len(sys.argv) < 6:
            raise Exception('param error, need 6, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 7:
            fork = sys.argv[6]
        test3(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], fork)
    elif func == 'test4':
        if len(sys.argv) < 7:
            raise Exception('param error, need 7, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 8:
            fork = sys.argv[7]
        test4(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], int(sys.argv[6]), fork)
    elif func == 'test5':
        if len(sys.argv) < 6:
            raise Exception('param error, need 6, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 7:
            fork = sys.argv[6]
        test5(sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]), fork)
    elif func == 'test6':
        if len(sys.argv) < 6:
            raise Exception('param error, need 6, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 7:
            fork = sys.argv[6]
        test6(sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]), fork)
    elif func == 'test7':
        if len(sys.argv) < 6:
            raise Exception('param error, need 6, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 7:
            fork = sys.argv[6]
        test7(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], fork)
    elif func == 'transfer':
        if len(sys.argv) < 6:
            raise Exception('param error, need 6, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 7:
            fork = sys.argv[6]
        transfer(sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]), fork)
    elif func == 'delegatecalltest':
        if len(sys.argv) < 5:
            raise Exception('param error, need 5, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 6:
            fork = sys.argv[5]
        delegatecalltest(sys.argv[2], sys.argv[3], sys.argv[4], fork)
    elif func == 'calltest':
        if len(sys.argv) < 5:
            raise Exception('param error, need 5, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 6:
            fork = sys.argv[5]
        calltest(sys.argv[2], sys.argv[3], sys.argv[4], fork)

    elif func == 'deposit':
        if len(sys.argv) < 5:
            raise Exception('param error, need 5, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 6:
            fork = sys.argv[5]
        deposit(sys.argv[2], sys.argv[3], int(sys.argv[4]), fork)
    elif func == 'withdraw':
        if len(sys.argv) < 5:
            raise Exception('param error, need 5, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 6:
            fork = sys.argv[5]
        withdraw(sys.argv[2], sys.argv[3], int(sys.argv[4]), fork)
    elif func == 'transfer_weth9':
        if len(sys.argv) < 6:
            raise Exception('param error, need 6, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 7:
            fork = sys.argv[6]
        transfer_weth9(sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]), fork)
    elif func == 'totalSupply':
        if len(sys.argv) < 4:
            raise Exception('param error, need 4, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 5:
            fork = sys.argv[4]
        totalSupply(sys.argv[2], sys.argv[3], fork)
    elif func == 'balanceOfAddress':
        if len(sys.argv) < 5:
            raise Exception('param error, need 5, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 6:
            fork = sys.argv[5]
        balanceOfAddress(sys.argv[2], sys.argv[3], sys.argv[4], fork)
    elif func == 'balance':
        if len(sys.argv) < 4:
            raise Exception('param error, need 4, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 5:
            fork = sys.argv[4]
        balance(sys.argv[2], sys.argv[3], fork)
    elif func == 'approve_weth9':
        if len(sys.argv) < 6:
            raise Exception('param error, need 6, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 7:
            fork = sys.argv[6]
        approve_weth9(sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]), fork)
    elif func == 'transferFrom':
        if len(sys.argv) < 7:
            raise Exception('param error, need 7, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 8:
            fork = sys.argv[7]
        transferFrom(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], int(sys.argv[6]), fork)

    elif func == 'getsource':
        if len(sys.argv) < 4:
            raise Exception('param error, need 4, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 5:
            fork = sys.argv[4]
        getsource(sys.argv[2], sys.argv[3], fork)
    elif func == 'getcode':
        if len(sys.argv) < 4:
            raise Exception('param error, need 4, argv count: {}'.format(len(sys.argv)))
        fork = None
        if len(sys.argv) >= 5:
            fork = sys.argv[4]
        getcode(sys.argv[2], sys.argv[3], fork)
    elif func == 'filemakehash':
        if len(sys.argv) < 3:
            raise Exception('param error, need 3, argv count: {}'.format(len(sys.argv)))
        filemakehash(sys.argv[2])

    elif func == 'getparam_balanceOf':
        account_key = getreverseaddresskey(sys.argv[2])
        fdata = funcdata('balanceOf(address)', account_key)
        print('param: {}'.format(fdata))

    else:
        print('func error, func: {}'.format(func))


# nohup python imitate-script.py >test-py.log & 

