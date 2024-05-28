#!/bin/sh

GETH=$HOME/Workspace/metabasenet/build/bin/geth

dlv exec $GETH -- --metabasenet --mine --miner.etherbase 0x5962974eeb0b17b43EDaBFc9b747839317AA852F --unlock 0x5962974eeb0b17b43EDaBFc9b747839317AA852F --password $HOME/.ethereum/metabasenet/password.txt
