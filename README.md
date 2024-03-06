
## MetabaseNet introduction

* MetabaseNet reinvents the concept of “Smart Contract” by allowing authors and verifiers of contracts to profit together, and proposes a smarter verification method - “Proof of Stake”, making it the most energy efficient solution and the fastest blockchain to date that is ideal for solving the everyday issues in global consumer goods market.
* MetabaseNet opens up a new era and creates infinite possibilities for the existing ecosystem, providing all the elements that make a great blockchain.  MetabaseNet is the first blockchain to achieve this speed without any compromise, which in turn makes it the best-performance real-world platform for real-world use.
* Imagine this - if there is a decentralized blockchain that is able to extend to any fields or industries, process all of the global transactions with a lightning speed and it proves to be absolutely safe. These are what MetabaseNet can bring to the table.
* This is why MetabaseNet will be the platform that truly bridges the gap between crypto asset enthusiasts and the rest of the world, as the technology it uses was created specifically to cope with the challenges that the blockchain has faced so far.
* MetabaseNet uses “proof of stake” as its verification method, which makes its energy consumption far less than verification methods of any other blockchain. The energy it consumes is even less than the traditional fiat transactions. With this verification method, MetabaseNet achieved 99.5% less energy consumption, while it is as secure as any other chain. 

## Version control
```
The version control of this project is based on semantic versioning, please refer to:
https://semver.org/
```

## Installation (Ubuntu16.04/18.04/22.04)
```
sudo apt install -y g++ libboost-all-dev openssl libreadline-dev pkg-config libncurses5-dev autoconf

# ubuntu16.04/22.04
sudo apt install -y libssl-dev

# ubuntu18.04
sudo apt install -y libssl1.0-dev

# Compile cmake
sudo apt autoremove cmake
wget https://github.com/Kitware/CMake/releases/download/v3.22.3/cmake-3.22.3.tar.gz
tar -zxvf cmake-3.22.3.tar.gz
cd cmake-3.22.3
./bootstrap 
make
sudo make install
sudo ln -s /usr/local/bin/cmake /usr/bin
cmake --version
cd ..

# Installation gcc10
sudo apt install build-essential
sudo apt install software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt install gcc-10 g++-10
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100 --slave /usr/bin/g++ g++ /usr/bin/g++-10
sudo update-alternatives --config gcc

# Compile libsodium (version >= 1.0.18)
wget https://github.com/jedisct1/libsodium/releases/download/1.0.18-RELEASE/libsodium-1.0.18.tar.gz --no-check-certificate
tar -zxvf libsodium-1.0.18.tar.gz
cd libsodium-1.0.18
./configure
make -j8 && make check
sudo make install
cd ..

# Compile protobuf
wget https://github.com/protocolbuffers/protobuf/releases/download/v3.11.3/protobuf-cpp-3.11.3.tar.gz
tar -xzvf protobuf-cpp-3.11.3.tar.gz
cd protobuf-3.11.3
./configure --prefix=/usr
make -j8
sudo make install
cd ..

```
## Download code and compile
```
git clone https://github.com/metabasenetwork/MetabaseNet.git
cd metabasenet
./INSTALL.sh
```

## Running metabasenet
Going through all the possible command line flags is out of scope here (please consult our CLI Wiki page), but we've enumerated a few common parameter combos to get you up to speed quickly on how you can run your own metabasenet instance.

### Hardware requirements
Minimum:

* CPU with 2+ cores
* 4GB RAM
* 128GB free storage space
* 8 MBit/sec download Internet service

Recommended:

* Fast CPU with 8+ cores
* 16GB+ RAM
* High-performance SSD with at least 1TB of free space
* 25+ MBit/sec download Internet service

### Configuration node
```
mint key: 0x2c6de2dad84e0a0a66e2dabed8acb31c7a9108fc180bafaff2375e48f36d8565
mint address: 0x03209207263898cbc66f8eb90c7f860578829e51
owner key: 0x565c0ac233ffb2f8a37cbd445344227fc3e22ff62e1da9fcf02a6c588cd1ca1c
owner address: 0x0a7f19bc7d945a7500fc6a86d79e643664533912

$ mkdir ~/.metabasenet
$ cd ~/.metabasenet
$ vim metabasenet.conf
mint=0x2c6de2dad84e0a0a66e2dabed8acb31c7a9108fc180bafaff2375e48f36d8565:0x0a7f19bc7d945a7500fc6a86d79e643664533912:500
listen4
```

### Start / stop node
**Start node：**
```
$ metabasenet -debug -daemon -testnet
```
**Stop node：**
```
$ metabasenet -testnet stop

Note: the above is to start the test network (with parameter: - testnet), which can easily and quickly obtain the test results.
```

### Import initial address
**Start client：**
```
$ metabasenet-cli -testnet
```

**Execute the following command on the client：**
```
importprivkey 0xd8b8db5ef512bd8e5a04f83ccfcfe6470c83d3f2e31480828ecf93a9ac0a54e2 123
unlockkey 0x5962974eeb0b17b43edabfc9b747839317aa852f 123

addnewtemplate delegate '{"mint": "0x03209207263898cbc66f8eb90c7f860578829e51", "owner": "0x0a7f19bc7d945a7500fc6a86d79e643664533912", "rewardratio": 500}'

sendfrom 0x5962974eeb0b17b43edabfc9b747839317aa852f 0x08512815194eb90873dd1d6a167294c329fa9782 10000000
```

### Create the template address of currency deposit and interest generation and transfer it to token
**Start client：**
```
$ metabasenet-cli -testnet
```

**Execute the following command on the client：**
```
addnewtemplate pledge '{"delegate": "0x93b09f6e5a5a3e7122a7ce468d4e5d2b0fc8d41d", "owner": "0x1882823f69b35fbf58833f2892b0cfdabddf0c63", "pledgetype": 5, "cycles": 1, "nonce": 0}'
sendfrom 0x5962974eeb0b17b43edabfc9b747839317aa852f 0x6e4089b464e08441a27804d8e78f7d691e2ef736 3000000
```

## Repo Guidelines

* Developers work in their own forks, then submit pull requests when they think their feature or bug fix is ready.
* If it is a simple/trivial/non-controversial change, then one of the development team members simply pulls it.
* If it is a more complicated or potentially controversial change, then the change may be discussed in the pull request.
* The patch will be accepted if there is broad consensus that it is a good thing. Developers should expect to rework and resubmit patches if they don't match the project's coding conventions or are controversial.
* From time to time a pull request will become outdated. If this occurs, and the pull is no longer automatically mergeable; a comment on the pull will be used to issue a warning of closure.  Pull requests closed in this manner will have their corresponding issue labeled 'stagnant'.
