#!/usr/bin/env python

import time
import requests
import json
from collections import OrderedDict
import os
import sys
import random
from pprint import pprint
from multiprocessing import Process

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

genesis_privkey = ''
genesis_address = ''

# RPC HTTP request
def call(body):
    req = requests.post(rpcurl, json=body)

    if mode != GENERATE_ADDR_MODE:
        print('DEBUG: request: {}'.format(body))
        print('DEBUG: response: {}'.format(req.content))

    resp = json.loads(req.content.decode('utf-8'))
    return resp.get('result'), resp.get('error')


# RPC: getnewkey
def getnewkey():
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getnewkey',
        'params': {
            'passphrase': password
        }
    })
    if result:
        return result.get('address')
    else:
        return ""

# RPC: listkey
def listkey(page=0,count=0):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'listkey',
        'params': {
            'page': page,
            'count': count
        }
    })
    return result

# RPC: importprivkey
def importprivkey(privkey):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'importprivkey',
        'params': {
            'privkey': privkey,
            'passphrase': password
        }
    })
    if result:
        return result.get('address')
    else:
        return ""

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

# RPC: removekey
def removekey(address):
    call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'removekey',
        'params': {
            'pubkey': address,
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

# RPC: eth_sendTransaction
def eth_sendTransaction(from_addr, to, amount, gas_in, data_in, nonce_in):
    gas = '0x{:x}'.format(gas_in)
    value = '0x{:x}'.format(amount*COIN)
    nonce = None
    if nonce_in:
        nonce = '0x{:x}'.format(nonce_in)
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'eth_sendTransaction',
        'params': [{
            'from': from_addr,
            'to': to,
            'gas':gas,
            'value': value,
            'data': data_in,
            'nonce' : nonce
        }]
    })

    if result:
        return result
    else:
        print('eth_sendTransaction error: {}'.format(error))
        return ""

# RPC: eth_sendTransaction create contract
def eth_sendTransaction_create_contract(from_addr, amount, gas_in, data_in):
    gas = '0x{:x}'.format(gas_in)
    value = '0x{:x}'.format(amount*COIN)
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'eth_sendTransaction',
        'params': [{
            'from': from_addr,
            'gas': gas,
            'value': value,
            'data': data_in
        }]
    })

    if result:
        return result
    else:
        print('eth_sendTransaction error: {}'.format(error))
        return ""

# RPC: eth_sendTransaction call contract
def eth_sendTransaction_call_contract(from_addr, to_addr, amount, gas_in, data_in):
    gas = '0x{:x}'.format(gas_in)
    value = '0x{:x}'.format(amount*COIN)
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'eth_sendTransaction',
        'params': [{
            'from': from_addr,
            'to': to_addr,
            'gas': gas,
            'value': value,
            'data': data_in
        }]
    })

    if result:
        return result
    else:
        print('eth_sendTransaction error: {}'.format(error))
        return ""

# RPC: eth_estimateGas
def eth_estimateGas(from_addr, to_addr, amount, gas_in, data_in):
    gas = '0x{:x}'.format(gas_in)
    value = '0x{:x}'.format(amount*COIN)
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'eth_estimateGas',
        'params': [{
            'from': from_addr,
            'to': to_addr,
            'gas': gas,
            'value': value,
            'data': data_in
        }]
    })

    if result:
        return result
    else:
        print('eth_estimateGas error: {}'.format(error))
        return ""

# RPC: eth_call
def eth_call(from_addr, to_addr, amount, gas_in, data_in):
    gas = '0x{:x}'.format(gas_in)
    value = '0x{:x}'.format(amount*COIN)
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'eth_call',
        'params': [{
            'from': from_addr,
            'to': to_addr,
            'gas': gas,
            'value': value,
            'data': data_in
        }]
    })

    if result:
        return result
    else:
        print('eth_call error: {}'.format(error))
        return ""

# RPC: gettransactionreceipt
def gettransactionreceipt(txid, forkid=None):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'gettransactionreceipt',
        'params': {
            'txid': txid,
            'fork': forkid
        }
    })

    if result:
        return result, True
    else:
        #print('gettransactionreceipt error: {}'.format(error))
        return "", False


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

# RPC: getforkport
def getforkport(forkid):
    result, error = call({
        'id': 1,
        'jsonrpc': '2.0',
        'method': 'getforkport',
        'params': {
            'forkid': forkid
        }
    })

    if result:
        return result
    else:
        raise Exception('getforkport error: {}'.format(error))


####################################################################
def getdefforkid():
    main_chainid = 0
    main_forkid = ''
    forklist = listfork()
    for x in forklist:
        if x['parentfork'] == '0x0000000000000000000000000000000000000000000000000000000000000000':
            main_chainid = x['chainid']
            main_forkid = x['fork']
        else:
            chainid = x['chainid']
            forkid = x['fork']
            print('sub fork, chainid: {}, forkid: {}'.format(chainid, forkid))
            return True,chainid,forkid
    if main_chainid != 0:
        return True,main_chainid,main_forkid
    return False,0,''

def getforkidbychainid(chainid):
    forklist = listfork()
    for x in forklist:
        if x['chainid'] == chainid:
            forkid = x['fork']
            print('get forkid, chainid: {}, forkid: {}'.format(chainid, forkid))
            return True,forkid
    return False,''

def getchainidbyforkid(forkid):
    forklist = listfork()
    for x in forklist:
        if x['fork'] == forkid:
            chainid = x['chainid']
            print('get chainid, chainid: {}, forkid: {}'.format(chainid, forkid))
            return True,chainid
    return False,''

def sendtx(forkid, from_addr, to_addr, amount, count, fCommonTx, fShowTxid, sleeptime, data_in, sscount_in):
    prevtime = int(time.time())
    sendcount = 0
    totalcount = 0
    while totalcount < count:
        if fCommonTx:
            txid, err = sendfrom_fork(from_addr, to_addr, amount, forkid)
            if err != 0:
                print('sendfrom fail, err: {}'.format(err))
                return
            if fShowTxid:
                print('sendfrom success, txid: {}'.format(txid))
        else:
            txid = eth_sendTransaction(from_addr, to_addr, amount, 0, data_in, sscount_in) #22000
            if txid == '' or txid == '0x0000000000000000000000000000000000000000000000000000000000000000':
                print('eth_sendTransaction fail')
                return
            if fShowTxid:
                print('eth_sendTransaction success, txid: {}'.format(txid))

        totalcount = totalcount + 1
        sendcount = sendcount + 1

        if sleeptime > 0:
            time.sleep(float(sleeptime)/1000)

        curtime = int(time.time())
        if curtime > prevtime:
            prevtime = curtime
            print('send count: {}, total: {}'.format(sendcount, totalcount))
            sendcount = 0

            while True:
                txpools = gettxpool(forkid)
                if txpools == -1:
                    print('gettxpool fail, forkid: {}'.format(forkid))
                    return
                if txpools < 50000:
                    break
                print('pool tx count: {}'.format(txpools))
                time.sleep(200/1000)

    #print('Process exit!')

def create_contract(from_in, value_in, code_in):
    gas_ret = eth_estimateGas(from_in, None, value_in, 990000, code_in)
    gas_n = int(gas_ret, 16)
    print('need gas: {}, gas_n: {}'.format(gas_ret,gas_n))

    txid = eth_sendTransaction_create_contract(from_in, value_in, gas_n, code_in)
    print('txid: {}'.format(txid))

    for i in range(0,120):
        receipt, ret = gettransactionreceipt(txid)
        if ret:
            addressContract = receipt['contractAddress']
            return addressContract
        else:
            time.sleep(1)
            print('wait...')
    return ''

def sendcall_contract(from_in, to_in, value_in, data_in):
    txid = eth_sendTransaction_call_contract(from_in, to_in, value_in, 990000, data_in)
    print('txid: {}'.format(txid))

def call_contract(from_in, to_in, value_in, data_in):
    ret = eth_call(from_in, to_in, value_in, 990000, data_in)
    print('ret: {}, value: {}'.format(ret, int(ret, 16)))

def more_sendcall_contract(from_in, to_in, value_in, data_in):
    prevtime = int(time.time())
    sendcount = 0
    totalcount = 0
    while totalcount < 6000000:
        txid = eth_sendTransaction_call_contract(from_in, to_in, value_in, 990000, data_in)
        totalcount += 1
        sendcount += 1

        curtime = int(time.time())
        if curtime > prevtime:
            prevtime = curtime
            print('send count: {}, total: {}'.format(sendcount, totalcount))
            sendcount = 0


####################################################################
def import_genesis_privkey():
    address = importprivkey(genesis_privkey)
    unlockkey(address)

def remove_all_key():
    removeallcount = 0
    fContinue = True
    while fContinue:
        removecount = 0
        ks = listkey(0,100)
        for x in ks:
            address = x['address']
            print('address: {}'.format(address))
            removekey(address)
            removecount += 1
        removeallcount += removecount
        if removecount < 100:
            fContinue = False
    print('remove key count: {}'.format(removeallcount))

def stat_key_count():
    allkeycount = 0
    fContinue = True
    page = 0
    while fContinue:
        count = 0
        ks = listkey(page,100)
        for x in ks:
            count += 1
        allkeycount += count
        page += 1
        if count < 100:
            fContinue = False
    print('key count: {}'.format(allkeycount))

####################################################################
if __name__ == "__main__":
    print('send tx......')

    genesis_privkey_mainnet = '0x41e7f692746a5265e170b4cff34c2452729ad7d69aa4c3d5b655f6c7d40613cf'
    genesis_address_mainnet = '0x885c14725a43a6bbc165b2561617c5da8162f170'

    genesis_privkey_testnet = '0xd8b8db5ef512bd8e5a04f83ccfcfe6470c83d3f2e31480828ecf93a9ac0a54e2'
    genesis_address_testnet = '0x5962974eeb0b17b43edabfc9b747839317aa852f'

    process_count = 1
    chainid = 0
    forkid = ''
    from_addr = '0x5962974eeb0b17b43edabfc9b747839317aa852f'
    to_addr = '0x9835c12d95f059eb4eaecb4b00f2ea2c99b2a0d4'
    amount = 1
    data = ''
    count = 2000000
    sscount = None
    fCommonTx = False
    fShowTxid = False
    sleeptime = 0
    codeContract = ''
    fCreateContract = False
    callParam = ''
    fSendCallContract = False
    fMoreSendCallContract = False
    fCallContract = False
    fSetFromAddress = False
    fSendNewAddress = False
    fRemoveAllKey = False
    fStatKeyCount = False

    seturl = False
    setforkid = False

    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '-rpcurl':
            if i+1 >= len(sys.argv):
                break
            rpcurl = sys.argv[i+1]
            seturl = True
        elif sys.argv[i] == '-rpcport':
            if i+1 >= len(sys.argv):
                break
            rpcurl = 'http://127.0.0.1:{}'.format(sys.argv[i+1])
            seturl = True
        elif sys.argv[i] == '-process':
            if i+1 >= len(sys.argv):
                break
            process_count = int(sys.argv[i+1])
        elif sys.argv[i] == '-forkid':
            if i+1 >= len(sys.argv):
                break
            if not setforkid:
                forkid = sys.argv[i+1]
                ret, chainid = getchainidbyforkid(forkid)
                if not ret:
                    chainid = 0
                    forkid = ''
                else:
                    setforkid = True
        elif sys.argv[i] == '-chainid':
            if i+1 >= len(sys.argv):
                break
            if not setforkid:
                chainid = int(sys.argv[i+1])
                ret, forkid = getforkidbychainid(chainid)
                if not ret:
                    chainid = 0
                    forkid = ''
                else:
                    setforkid = True
        elif sys.argv[i] == '-from':
            if i+1 >= len(sys.argv):
                break
            from_addr = sys.argv[i+1]
            fSetFromAddress = True
        elif sys.argv[i] == '-to':
            if i+1 >= len(sys.argv):
                break
            to_addr = sys.argv[i+1]
        elif sys.argv[i] == '-amount':
            if i+1 >= len(sys.argv):
                break
            amount = int(sys.argv[i+1])
        elif sys.argv[i] == '-data':
            if i+1 >= len(sys.argv):
                break
            data = sys.argv[i+1]
        elif sys.argv[i] == '-count':
            if i+1 >= len(sys.argv):
                break
            count = int(sys.argv[i+1])
        elif sys.argv[i] == '-sleeptime':
            if i+1 >= len(sys.argv):
                break
            sleeptime = int(sys.argv[i+1])
        elif sys.argv[i] == '-sscount':
            if i+1 >= len(sys.argv):
                break
            sscount = int(sys.argv[i+1])
        elif sys.argv[i] == '-commontx':
            fCommonTx = True
            i -= 1
        elif sys.argv[i] == '-ethtx':
            fCommonTx = False
            i -= 1
        elif sys.argv[i] == '-showtxid':
            fShowTxid = True
            i -= 1
        elif sys.argv[i] == '-testnet':
            testnet = True
            i -= 1
        elif sys.argv[i] == '-mainnet':
            testnet = False
            rpcurl = rpcurl_mainnet
            i -= 1
        elif sys.argv[i] == '-sendnewaddress':
            fSendNewAddress = True
            i -= 1
        elif sys.argv[i] == '-removeallkey':
            fRemoveAllKey = True
            i -= 1
        elif sys.argv[i] == '-statkeycount':
            fStatKeyCount = True
            i -= 1
        elif sys.argv[i] == '-create':
            if i+1 >= len(sys.argv):
                break
            codeContract = sys.argv[i+1]
            fCreateContract = True
        elif sys.argv[i] == '-sendcall':
            if i+1 >= len(sys.argv):
                break
            callParam = sys.argv[i+1]
            fSendCallContract = True
        elif sys.argv[i] == '-call':
            if i+1 >= len(sys.argv):
                break
            callParam = sys.argv[i+1]
            fCallContract = True
        elif sys.argv[i] == '-moresendcall':
            if i+1 >= len(sys.argv):
                break
            callParam = sys.argv[i+1]
            fMoreSendCallContract = True

        i += 2

    if not setforkid:
        ret, chainid, forkid = getdefforkid()
        if not ret:
            raise Exception('get default forkid fail')
        setforkid = True

    if not seturl:
        if setforkid:
            rpcport = getforkport(forkid)
            rpcurl = 'http://127.0.0.1:{}'.format(rpcport)
        else:
            if testnet:
                rpcurl = rpcurl_testnet
            else:
                rpcurl = rpcurl_mainnet

    if testnet:
        genesis_privkey = genesis_privkey_testnet
        genesis_address = genesis_address_testnet
    else:
        genesis_privkey = genesis_privkey_mainnet
        genesis_address = genesis_address_mainnet

    if not fSetFromAddress:
        from_addr = genesis_address
    
    print("process: {}".format(process_count))
    print("chainid: {}".format(chainid))
    print("forkid: {}".format(forkid))
    print("from: {}".format(from_addr))
    print("to: {}".format(to_addr))
    print("amount: {}".format(amount))
    print("data: {}".format(data))
    print("count: {}".format(count))
    print("sleeptime: {}".format(sleeptime))
    print("fCommonTx: {}".format(fCommonTx))
    print("fShowTxid: {}".format(fShowTxid))
    print("rpcurl: {}".format(rpcurl))
    if testnet:
        print("net: testnet")
    else:
        print("net: mainnet")

    import_genesis_privkey()
    unlockkey(from_addr)

    if fCreateContract:
        unlockkey(from_addr)
        addr = create_contract(from_addr, 0, codeContract)
        print('contract address: {}'.format(addr))
        sys.exit()
    elif fSendCallContract:
        unlockkey(from_addr)
        sendcall_contract(from_addr, to_addr, 0, callParam)
        sys.exit()
    elif fMoreSendCallContract:
        unlockkey(from_addr)
        more_sendcall_contract(from_addr, to_addr, 0, callParam)
        sys.exit()
    elif fCallContract:
        unlockkey(from_addr)
        call_contract(from_addr, to_addr, 0, callParam)
        sys.exit()
    elif fRemoveAllKey:
        remove_all_key()
        sys.exit()
    elif fStatKeyCount:
        stat_key_count()
        sys.exit()
    elif fSendNewAddress:
        prevaddress = from_addr
        for i in range(0,count):
            to_addr = getnewkey()
            unlockkey(to_addr)
            print('{}: new address: {}'.format(i, to_addr))
            sendtx(forkid, from_addr, to_addr, amount*100, 2, fCommonTx, fShowTxid, sleeptime, data, sscount)
            sendtx(forkid, prevaddress, to_addr, amount, 50, fCommonTx, fShowTxid, sleeptime, data, sscount)
            if prevaddress != from_addr:
                removekey(prevaddress)
            prevaddress = to_addr
        sys.exit()
            
    if process_count == 1:
        sendtx(forkid, from_addr, to_addr, amount, count, fCommonTx, fShowTxid, sleeptime, data, sscount)
    else:
        Procs = []

        for i in range(0,process_count):
            proc = Process(target=sendtx,args=(forkid, from_addr, to_addr, amount, count, fCommonTx, fShowTxid, sleeptime, data, sscount))
            Procs.append(proc)

        for p in Procs:
            p.start()

        #while True:
        #    time.sleep(1)

        for p in Procs:
            p.join()

    print('Exit!')
