#!/usr/bin/env python
# -*- coding: utf-8 -*-

from binascii import hexlify, unhexlify
import struct
from ethereum.abi import encode_abi
from ethereum.utils import sha3
import sys
import subprocess
import json

#sudo -H pip3 install --user --upgrade pip -i https://mirrors.aliyun.com/pypi/simple/
#sudo -H pip3 install ethereum -i https://mirrors.aliyun.com/pypi/simple/

def run_cmd(cmd):
	cmdline = "metabasenet-cli -testnet " + cmd
	print(cmdline)
	info = subprocess.run(cmdline, shell=True,stdout=subprocess.PIPE,universal_newlines=True)
	#info = subprocess.run(cmd, shell=True,stdout=subprocess.PIPE,universal_newlines=True)
	return info.stdout

def create(account_addr,file):
	f = open(file,'rb')
	unlockkeycmd = "unlockkey %s 123" % (account_addr)
	run_cmd(unlockkeycmd)
	cmd = "sendfrom %s 0 0 -d=%s" % (account_addr,f.read().hex())
	print(run_cmd(cmd))

def totalSupply(account_addr,contract_addr):
	unlockkeycmd = "unlockkey %s 123" % (account_addr)
	run_cmd(unlockkeycmd)
	FunSig = sha3("totalSupply()")[28:].hex()
	cmd = "callcontract %s %s 0 -d=%s" % (account_addr,contract_addr,FunSig)
	print(cmd)
	print(run_cmd(cmd))

def transfer(cmd_addr, contract_addr,account_addr,amount):
	unlockkeycmd = "unlockkey %s 123" % (account_addr)
	run_cmd(unlockkeycmd)
	fun_sig = sha3("transfer(address,uint256)")[28:].hex()
	cmd = "validateaddress " + account_addr
	ret = run_cmd(cmd)
	account_pub = json.loads(ret)["addressdata"]["pubkey"]
	account_pub = hexlify(unhexlify(account_pub)[::-1]).decode()
	call_data = fun_sig + encode_abi(["bytes32","uint256"],[unhexlify(account_pub),int(amount)]).hex()
	cmd = "sendfrom %s %s 0 -d=%s" % (cmd_addr,contract_addr,call_data)
	ret = run_cmd(cmd)
	print(ret)

def balanceOf(cmd_addr,contract_addr,account_addr):
	unlockkeycmd = "unlockkey %s 123" % (account_addr)
	run_cmd(unlockkeycmd)
	cmd = "validateaddress " + account_addr
	ret = run_cmd(cmd)
	account_pub = json.loads(ret)["addressdata"]["pubkey"]
	account_pub = hexlify(unhexlify(account_pub)[::-1]).decode()
	call_data = sha3("balanceOf(address)")[28:].hex() + account_pub
	cmd = "callcontract %s %s 0 -d=%s" % (cmd_addr,contract_addr,call_data)
	ret = run_cmd(cmd)
	print(ret)

if __name__ == '__main__':
	#print(encode_abi(["uint256"],[1]).hex())
	#exit()
	if len(sys.argv) == 1:
		print("create contract cmd: ./erc20.py create 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb erc20.wasm")
		print("see totalSupply cmd: ./erc20.py totalSupply 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 3t0txqw8rnhy1gbh6j4m6v6jp1qgc4rs5ns7zef1a6hhmk7ns3hgx5wva")
		print("transfer cmd: ./erc20.py transfer 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 3t0txqw8rnhy1gbh6j4m6v6jp1qgc4rs5ns7zef1a6hhmk7ns3hgx5wva 1xpkvx48v2ze2q6gea24y4bdm6ernaenzgf4kekw422wzt8txg3sjt4tp 100")
		print("see balanceOf cmd: ./erc20.py balanceOf 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 3t0txqw8rnhy1gbh6j4m6v6jp1qgc4rs5ns7zef1a6hhmk7ns3hgx5wva 1xpkvx48v2ze2q6gea24y4bdm6ernaenzgf4kekw422wzt8txg3sjt4tp")
	else:
		if sys.argv[1] == "create":
			if len(sys.argv) == 4:
				create(sys.argv[2],sys.argv[3])
			else:
				print("Wrong number of parameters")
		if sys.argv[1] == "totalSupply":
			if len(sys.argv) == 4:
				totalSupply(sys.argv[2],sys.argv[3])
			else:
				print("Wrong number of parameters")
		if sys.argv[1] == "transfer":
			if len(sys.argv) == 6:
				transfer(sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5])
			else:
				print("Wrong number of parameters")
		if sys.argv[1] == "balanceOf":
			if len(sys.argv) == 5:
				balanceOf(sys.argv[2],sys.argv[3],sys.argv[4])
			else:
				print("Wrong number of parameters")

#18160ddd: totalSupply()
#70a08231: balanceOf(address)
#a9059cbb: transfer(address,uint256)
#dd62ed3e: allowance(address,address)
#095ea7b3: approve(address,uint256)
#23b872dd: transferFrom(address,address,uint256)
#39509351: increaseAllowance(address,uint256)
#a457c2d7: decreaseAllowance(address,uint256)