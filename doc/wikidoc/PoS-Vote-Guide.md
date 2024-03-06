## 1. ready For POS vote
**NOTE:**  

1) MetabaseNet Core use the PoA + PoS consensus model, and there is no pre-excavation and pre-projection, it is necessary to dig out enough Token through the PoW before Token can be used to vote

2) Download address of executable program (please select the latest version):  

```  
https://github.com/metabasenetwork/metabasenet/releases  
```

3) In the windows operating system,when executing the program in your current directory, please change the "./metabasenet" command to "metabasenet". Same below. 

## 2. Run the executable program to generate two groups of standby keys  

**NOTE:**
  
1) Use to create POS template and Wallet  

2) Use the command **"metabasenet makekeypair"** to generate two groups of public-private key pairs, and use the command **"metabasenet getpubkeyaddress $pubkey"** to obtain the public key address  

```
First key pairs (for spending):
{
    "privkey" : "e7382046309d5a623bd87a75b666b088f53b8f86fa87a27d1aaafb5b174ca9f1",
    "pubkey" : "8e0594f219d5ffa30cc938fa9c26c76e0bfa3655fe91b67ee1b1891bee5dbb7f",
    "pubkeyaddress" : "1fyxnvvgvh6ry2znpj7z5adqt1dqce9mwz8wcj353zzahkwmm0p74y0t4"
}

Second key pairs (for mining):
{
    "privkey" : "57bdb88a874facd3c1e5dada6b6ce4d7d271a101236523b9f0fd314576c96b4d",
    "pubkey" : "928aa9ac6e67b094024ca2a59a228eb6fd45a26bc3ee02d53b7689903f6a7941"
}

```

## 3. Modify the configuration file (.metabasenet/metabasenet.conf)

```
# Start P2P listening (IPv4)
listen4   

connect=218.12.12.56:8813(Test Net)
connect=112.126.160.14:16601(Test Net)

# Public key address for POS(spending address/wallet address)
mpvssaddress=1fyxnvvgvh6ry2znpj7z5adqt1dqce9mwz8wcj353zzahkwmm0p74y0t4

# Private key for POS(Private key for signing)
mpvsskey=57bdb88a874facd3c1e5dada6b6ce4d7d271a101236523b9f0fd314576c96b4d

# pos node reward ratio (range: 0~10000)
rewardratio=500

```

**NOTE:**
  
If multiple POS nodes are configured on the same host,the port and rpcport parameters of each node should be different.


## 4. After the configuration is completed, restart the program, and the data starts to be synchronized automatically
```
command: metabasenet -daemon -debug
```

## 5. Import POS mining template into Wallet (Use different escape character on Different OS, see [Tips on Different OS](https://github.com/metabasenetwork/metabasenet/wiki/JSON-RPC#tips-on-different-os))

```
command:
metabasenet addnewtemplate delegate '{"delegate": "Public key for signing", "owner": "spending address/wallet address", "rewardratio": 500}'

example:
metabasenet addnewtemplate delegate '{"delegate": "928aa9ac6e67b094024ca2a59a228eb6fd45a26bc3ee02d53b7689903f6a7941", "owner": "1fyxnvvgvh6ry2znpj7z5adqt1dqce9mwz8wcj353zzahkwmm0p74y0t4", "rewardratio": 500}'
20m04megx747dtd8jpg1sxgwsz3v2mwq4fejgtg47fc7n7z7s02c1ngkq //template address

```

## 6. Vote

**NOTE:** PoS voting consists of voting by oneself and voting by other address

### 6.1 Voting by the PoS template address owner
```
command:
metabasenet sendfrom <PoS spending address>  <POS template address>  <$TOKEN>

example:
metabasenet sendfrom 1fyxnvvgvh6ry2znpj7z5adqt1dqce9mwz8wcj353zzahkwmm0p74y0t4 20m04megx747dtd8jpg1sxgwsz3v2mwq4fejgtg47fc7n7z7s02c1ngkq 1000
```

### 6.2 Voting for others
```
NOTE: You need to generate a vote template address to be voted for based on the other's DPoS template address and your wallet address,and then vote for the newly generated vote address.This voting method is redeemable. Direct voting for POS mining template addresses is non-redeemable.

1) Generate a vote template address (Use different escape character on Different OS, see [Tips on Different OS](https://github.com/metabasenetwork/metabasenet/wiki/JSON-RPC#tips-on-different-os))
command:
metabasenet addnewtemplate vote '{"delegate": "The other's POS template address", "owner": "Your spending address/Your wallet address", "rewardmode": "0: reward to vote address, 1: reward to owner address"}'

example:
metabasenet addnewtemplate vote '{"delegate": "20m04megx747dtd8jpg1sxgwsz3v2mwq4fejgtg47fc7n7z7s02c1ngkq", "owner": "1tefq9nem7mdkr14gx3man2sr1bvvvfhey0evvctp3tdj6r4d279sr9p7", "rewardmode": 1}'
--- 20w03ny60ab1h1s6kbqd1qxja2797s37942tzsxyk9qhw0xtj8nemm36q  --- vote address

2) Voting
command:
metabasenet sendfrom {$Your spending address/Your wallet address} {$vote address} {$amount}

example:
metabasenet sendfrom 1tefq9nem7mdkr14gx3man2sr1bvvvfhey0evvctp3tdj6r4d279sr9p7 20w03ny60ab1h1s6kbqd1qxja2797s37942tzsxyk9qhw0xtj8nemm36q 1000

```

### 6.3 Voting for others (No need to import the voice template address)
```
NOTE: Because the whole chain of transactions needs to be loaded when importing the voice template address, it takes time. If you do not need to import the voice template address, you can use the following methods:

1) Generate a vote template address (Use different escape character on Different OS, see [Tips on Different OS](https://github.com/metabasenetwork/metabasenet/wiki/JSON-RPC#tips-on-different-os))
command:
metabasenet maketemplate vote '{"delegate": "The other's POS template address", "owner": "Your spending address/Your wallet address", "cycle": "cycle: 7,14,30,90", "nonce": "nonce"}}'

example:
metabasenet maketemplate vote '{"delegate": "20m0bk8tjwt73gtjb4jv8r6sa04fe22r3ns2era8zbnn20qfv8sz38vg6", "owner": "1tefq9nem7mdkr14gx3man2sr1bvvvfhey0evvctp3tdj6r4d279sr9p7", "cycle":7, "nonce":1}'
--- "address" : "20w0629r20911rwy0mfkqmcp8qqsghp1m4v8e3998s04xmcd6qb1dj9er", --- This is the address of the vote template. Transfer to this address to vote;
---"hex" : "0700020500b9a352e68e386a4b24b68c1b2a011ee10b03ae44ec291f5d6a205dfb467e01d39f74d5d43d1b3c0490e8e8aa8b380af7bdbe2ef01dbdb3561e9b23608d11d3"    ---This is the data of the address of the voice template, which is used for the - TD parameter in the sendfromcommand;

2) Voting
command:
metabasenet sendfrom {$Your spending address/Your wallet address} {$vote address} {$amount} -td={template address data}

NOTE: -td=This parameter is the hex data generated by the maketemplate command, that is, the template address data;

example:
metabasenet sendfrom 1tefq9nem7mdkr14gx3man2sr1bvvvfhey0evvctp3tdj6r4d279sr9p7  20w0629r20911rwy0mfkqmcp8qqsghp1m4v8e3998s04xmcd6qb1dj9er 1000 -td=0700020500b9a352e68e386a4b24b68c1b2a011ee10b03ae44ec291f5d6a205dfb467e01d39f74d5d43d1b3c0490e8e8aa8b380af7bdbe2ef01dbdb3561e9b23608d11d3

```

## 7.  Clients who voted cancel the vote and retrieve token
```
NOTE: Redemption can only be done by the voting address and the corresponding wallet needs to be unlocked

command: 
metabasenet sendfrom {$vote address} {$Your spending address/Your wallet address} {$amount}

example：
metabasenet sendfrom 20w03ny60ab1h1s6kbqd1qxja2797s37942tzsxyk9qhw0xtj8nemm36q 1tefq9nem7mdkr14gx3man2sr1bvvvfhey0evvctp3tdj6r4d279sr9p7 1000

```

**NOTE:** 

```
1) The node needs to configure pos mining parameters,**"mpvssaddress"** and **"mpvsskey"**. For security, these two parameters should come from different key pairs.  

2) There are two ways to vote, vote for oneself and vote for others. The template for accepting the vote is two different templates.  

3) The number of tokens transferred in determines the weight of the address. The ratio of the number of tokens owned by the template address to the total number of tokens is the weight. If the weight is larger, the probability of the address generates block is higher. If the weight is small, the probability of the address generates block is lower.  

4) The total number of tokens issued by MetabaseNet is called **“Total Tokens"**, which increases with the increase of block height. We can calculate the total number of tokens at a certain block height according to the release rules of token.
  
```

Q&A  
--------------------------------------------------------------------------------------------------------------

```
1) What is the height of the poa generates block reached can the POS start to generate block? What principle is this based on?    
A:In the initial stage, blocks are generated in POA mode. After that, if the POS miner gets the vote, POS will have the opportunity to generate the block. However, whether POS blocks are generated depends on POA, voting, and the random number of the whole network. In the first phase, the height is hard coded into the program. When the current height is lower than this height, POA blocks will be generated. When the current height is higher than this height, whether POA blocks or POS blocks are determined by voting and random number.

2) How many tickets do POS node needs to get to get the right of block generate?  
A: POS nodes need to obtain at least 2% of the total number of token to have the chance to participate in POS out block node competition. Finally, according to the voting situation, select the top 23 nodes to participate in the random number selection. As for how many votes need to be obtained in order to get the block right, it needs to be determined according to the voting situation of the whole network. The more votes a node gets, the higher the chance to get the block right.

3) What is the relationship between permissions and weights for POS generates block?  
A:The weight of POS node is calculated by taking the number of token and the total number of token obtained by the node as parameters. The more votes the node gets, the higher the weight and the higher the chance to get the block weight.

4) Why do we need two groups of keys when adding mining templates?  
A: when adding a mining template, two parameters need to be added. One is the private key used for mining, which is used for the signature of blocks generate. The other is the token spending address. Only when you have the private key of the address, you can spend the token owned by the mining template address. For security reasons, the two groups of keys cannot be the same. The mining private key needs to be saved in the mining node and may be disclosed, so this private key is not secure. The other parameter is the spend address, which does not need to give the private key. As long as the private key is saved, the token in the template address is secure. Therefore, in our MetabaseNet system, using two groups of key pairs can improve the security.
```