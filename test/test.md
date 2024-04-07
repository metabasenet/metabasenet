
## add config
```conf
#           0x03209207263898cbc66f8eb90c7f860578829e51 privkey
mint=0x2c6de2dad84e0a0a66e2dabed8acb31c7a9108fc180bafaff2375e48f36d8565:0x0a7f19bc7d945a7500fc6a86d79e643664533912:500
listen4
fulldb=true
addfork=any
chainid=101
chainidrpcport=102:6002:6012
```

## run metabasenet
```bash
metabasenet -testnet -daemon -debug
```

## import privkey
```bash
metabasenet -testnet importprivkey 0xd8b8db5ef512bd8e5a04f83ccfcfe6470c83d3f2e31480828ecf93a9ac0a54e2 123
metabasenet -testnet unlockkey 0x5962974eeb0b17b43edabfc9b747839317aa852f 123
```
## create fork
```bash
cd {project dir}/test/testscript/base
python createfork.py -type clonemap
```

## add dpos node
```bash
#                                                                    0x565c0ac233ffb2f8a37cbd445344227fc3e22ff62e1da9fcf02a6c588cd1ca1c
addnewtemplate delegate '{"mint": "0x03209207263898cbc66f8eb90c7f860578829e51", "owner": "0x0a7f19bc7d945a7500fc6a86d79e643664533912", "rewardratio": 500}'
sendfrom 0x5962974eeb0b17b43edabfc9b747839317aa852f 0x08512815194eb90873dd1d6a167294c329fa9782 10000000
importprivkey 0x565c0ac233ffb2f8a37cbd445344227fc3e22ff62e1da9fcf02a6c588cd1ca1c 123
```

## ~~vote~~
```bash
importprivkey 0x24a20a15b3e4b08a6117dc1926705df9173fb81d9198dded894c0e584c6ae2e3 123
addnewtemplate pledge '{"delegate": "0x08512815194eb90873dd1d6a167294c329fa9782", "owner": "0x60050d723fe92803fc0a8f4eb778db0b9a331a70", "pledgetype": 5, "cycles": 1, "nonce": 0}'
unlockkey 0x5962974eeb0b17b43edabfc9b747839317aa852f 123
sendfrom 0x5962974eeb0b17b43edabfc9b747839317aa852f 0x90354237ac4c59015c1115321c0474357f910b33 3000000
```

## cross transfer
```bash
# from 101 frok to 102 frok 100MNT
sendcrosstransfertx 0x60050d723fe92803fc0a8f4eb778db0b9a331a70 102 100
getbalance -a=0x60050d723fe92803fc0a8f4eb778db0b9a331a70 -f=101
getbalance -a=0x60050d723fe92803fc0a8f4eb778db0b9a331a70 -f=102
```