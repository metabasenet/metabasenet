#!/usr/bin/env python

import sys
sys.path.append('../hrpc')
import rpci

def invite(parentaddress, subaddress, amount=0):
    rpci.unlockkey(parentaddress)
    rpci.unlockkey(subaddress)
    signstr = rpci.makedefirelsign(parentaddress, subaddress)
    txid, ret = rpci.sendfrom_type(parentaddress, subaddress, amount, 1, signstr)
    if ret == 0:
        print('invite success, parent: {}, sub: {}, txid: {}'.format(parentaddress, subaddress, txid))
    else:
        print('invite fail')
    return ret

def addinvite(parentaddress, amount=0):
    pubkey = rpci.getnewkey()
    subaddress = rpci.getpubkeyaddress(pubkey)
    ret = invite(parentaddress, subaddress, amount)
    #rpci.removekey(subaddress)
    return ret
    
def invitetest(parentaddress, amount=0):
    addinvite(parentaddress, amount)

#############################################################################
if __name__ == "__main__":
    func = "non"

    if len(sys.argv) >= 2:
        func = sys.argv[1]

    if func == 'invite':
        if len(sys.argv) < 4:
            raise Exception('param error, need 4, argv count: {}'.format(len(sys.argv)))
        parentaddress = sys.argv[2]
        subaddress = sys.argv[3]
        amount = 1
        if len(sys.argv) >= 5:
            amount = float(sys.argv[4])
        invite(parentaddress, subaddress, amount)
    elif func == 'newinvite':
        if len(sys.argv) < 3:
            raise Exception('param error, need 3, argv count: {}'.format(len(sys.argv)))
        parentaddress = sys.argv[2]
        amount = 1
        if len(sys.argv) >= 4:
            amount = float(sys.argv[3])
        invitetest(parentaddress, amount)
    else:
        print('error func: {}'.format(func))
        
# python invite.py invite <parentaddress> <subaddress> <amount>
# python invite.py invite 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda 10

# python invite.py newinvite 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 10

