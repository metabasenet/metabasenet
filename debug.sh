#!/bin/sh

GMNT=$HOME/Workspace/github/metabasenet/build/bin/gmnt
DATADIR=$HOME/Library/Metabasenet

dlv exec $GMNT -- --testnet --mine --miner.etherbase 0x5962974eeb0b17b43EDaBFc9b747839317AA852F --unlock 0x5962974eeb0b17b43EDaBFc9b747839317AA852F --password $DATADIR/testnet/password.txt
