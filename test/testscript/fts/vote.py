#!/usr/bin/env python

import sys
sys.path.append('../hrpc')
from rpci import *

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

#############################################################################
if __name__ == "__main__":
    func = "non"

    if len(sys.argv) >= 2:
        func = sys.argv[1]

    if func == 'votetest':
        delegateaddr = "20m0c4bzffcmavt6ndfze92qvzeheptm5xq9vxm50xr2xm7s0ymd7g7w0"
        sleeptime = 2
        voteamount = 200
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
    else:
        print('error func: {}'.format(func))
