Complete all settings for POA mining with the help of [POA operation guide](https://github.com/metabasenetwork/metabasenet/wiki/POA-Guide)
--------------------------------------------------------------------------------------------------------------

**NOTE:**  

1) Since the POS generate block needs to start after the block height reaches a certain level, and before that, the POA generate block is required before the POS

2) Download address of executable program (please select the latest version):  

```  
https://github.com/metabasenetwork/metabasenet/releases  
```

3) In the windows operating system,when executing the program in your current directory, please change the "./metabasenet" command to "metabasenet". Same below. 

Run the executable program to generate two groups of standby keys  
--------------------------------------------------------------------------------------------------------------

**NOTE:**

1) Use to create POS template and Wallet  

2) Use the command **"./metabasenet makekeypair"** to generate two groups of public-private key pairs, and use the command **"./metabasenet getpubkeyaddress $pubkey"** to obtain the public key address  

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

Modify the configuration file used in POA
--------------------------------------------------------------------------------------------------------------
In the previous [POA Guide](https://github.com/metabasenetwork/metabasenet/wiki/poa-guide),we have described the configuration file **"metabasenet.conf"**, which we will modify it as follows:

```
# use test net,do not set this parameter in real environment
#testnet

# Start P2P listening (IPv4)
listen4   

# Start statistics
statdata

# Fill in the dnseed address.If there is no dnseed,you need to fill in the addnode parameter 
dnseed=15.188.165.143

# By default, the IP address of the gateway can not be provided. At this time, only data can be synchronized, and external services cannot be provided
# gateway=

# The configuration of the primary node, but we don't need to set it when we don't know the primary node. Through dnseed, the primary node will be found automatically
# addnode=

# Public key address for POA(spending address)
cryptonightaddress=15wzfexznydjrsr0qwfma2ppjqs3y1krkpbdvhkme81y7wemytcaf8k28

# Private key for POA(Private key for signing)
cryptonightkey=2f0d0cad44197616195558935407f9febc6028938022ce4dfc912e759c7b45f0

# Public key address for POS(spending address)
mpvssaddress=1fyxnvvgvh6ry2znpj7z5adqt1dqce9mwz8wcj353zzahkwmm0p74y0t4

# Private key for POS(Private key for signing)
mpvsskey=57bdb88a874facd3c1e5dada6b6ce4d7d271a101236523b9f0fd314576c96b4d

# pos node reward ratio (range: 0~10000)
rewardratio=500

```

**NOTE:**
  
If multiple POS nodes are configured on the same host,the port and rpcport parameters of each node should be different.

After the configuration is completed, restart the program, and the data starts to be synchronized automatically
--------------------------------------------------------------------------------------------------------------

Import POS mining template into Wallet
--------------------------------------------------------------------------------------------------------------

```
metabasenet addnewtemplate delegate '{"delegate": "928aa9ac6e67b094024ca2a59a228eb6fd45a26bc3ee02d53b7689903f6a7941", "owner": "1fyxnvvgvh6ry2znpj7z5adqt1dqce9mwz8wcj353zzahkwmm0p74y0t4", "rewardratio": 500}'
20m04megx747dtd8jpg1sxgwsz3v2mwq4fejgtg47fc7n7z7s02c1ngkq //template address

```

Vote
--------------------------------------------------------------------------------------------------------------
**NOTE:**  

When users transfer token to pos mining template address,they will vote.The source address {$from} transferred in must be the corresponding value of owner parameter in pos mining template address.  

```
Command: ./metabasenet sendfrom <$Owner> <$Template Address>  <$Tokens>  

eg: ./metabasenet sendfrom 1fyxnvvgvh6ry2znpj7z5adqt1dqce9mwz8wcj353zzahkwmm0p74y0t4 20m04megx747dtd8jpg1sxgwsz3v2mwq4fejgtg47fc7n7z7s02c1ngkq 1000
```

**NOTE:** 

```
1) The node needs to configure pos mining parameters,**"mpvssaddress"** and **"mpvsskey"**. For security, these two parameters should come from different key pairs.  

2) It is necessary to transfer the token to the pos template address corresponding to the pos mining parameter. The token transferor must be the value of the mpvssaddress parameter, and the receiver is the pos mining template address.  

3) The number of tokens transferred in determines the weight of the address. The ratio of the number of tokens owned by the template address to the total number of tokens is the weight. If the weight is larger, the probability of the address generates block is higher. If the weight is small, the probability of the address generates block is lower.  

4) The total number of tokens issued by MetabaseNet is called **“Total Tokens"**, which increases with the increase of block height. We can calculate the total number of tokens at a certain block height according to the release rules of token.
  
5) The first stage is POA out block. After a certain height(public test time is 7 days, i.e. 7 days (10080 height)) is passed, pos can be used to generate block(determine whether to out POS block according to the address weight)  
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