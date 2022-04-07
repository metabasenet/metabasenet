## Repo Guidelines

* Developers work in their own forks, then submit pull requests when they think their feature or bug fix is ready.
* If it is a simple/trivial/non-controversial change, then one of the development team members simply pulls it.
* If it is a more complicated or potentially controversial change, then the change may be discussed in the pull request.
* The patch will be accepted if there is broad consensus that it is a good thing. Developers should expect to rework and resubmit patches if they don't match the project's coding conventions (see [the coding guides](https://github.com/Block-Way/metabasenet/wiki/MetabaseNet-Open-source-Style-Guides)) or are controversial.
* From time to time a pull request will become outdated. If this occurs, and the pull is no longer automatically mergeable; a comment on the pull will be used to issue a warning of closure.  Pull requests closed in this manner will have their corresponding issue labeled 'stagnant'.

## Installation (Ubuntu16.04/18.04)
```
sudo apt install -y g++ libboost-all-dev openssl libreadline-dev pkg-config libncurses5-dev autoconf

# ubuntu16.04
sudo apt install -y libssl-dev

# ubuntu18.04
sudo apt install -y libssl1.0-dev libprotobuf-dev protobuf-compiler

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
git clone https://github.com/Block-Way/MetabaseNet.git
cd metabasenet
./INSTALL.sh
```

## Configuration node
```
$ mkdir ~/.metabasenet
$ cd ~/.metabasenet
$ vim metabasenet.conf
mpvssaddress=1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda
mpvsskey=28efbfda61b473c37549d02784648d89fe21ff082b7a42da9ef97b0b83cdb1a9
rewardratio=500
cryptonightaddress=1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda
cryptonightkey=28efbfda61b473c37549d02784648d89fe21ff082b7a42da9ef97b0b83cdb1a9
listen4
```

## Start / stop node
**Start node：**
```
$ metabasenet -debug -daemon -testnet
```
**Stop node：**
```
$ metabasenet -testnet stop

Note: the above is to start the test network (with parameter: - testnet), which can easily and quickly obtain the test results.
```

## Import initial address
**Start client：**
```
$ metabasenet-cli -testnet
```

**Execute the following command on the client：**
```
importprivkey 141a6728ded4f83f767ea770e3582be497c5088fcc3b9ca248751887534f5197 123
unlockkey 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 123
importprivkey 42b889a2668eda6d78682c23b5651fb76b5aac2b71caba1aa23b6b14d5ce75b7 123
unlockkey 1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda 123

addnewtemplate delegate '{"delegate": "883f29fb7740f8e625159cb6bcee42cf3ac460be8b9fcb839ccbfeda3744b217", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda", "rewardratio": 500}'

sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 20m0c4bzffcmavt6ndfze92qvzeheptm5xq9vxm50xr2xm7s0ymd7g7w0 10000000
```

## Create the template address of currency deposit and interest generation and transfer it to token
**Start client：**
```
$ metabasenet-cli -testnet
```

**Execute the following command on the client：**
```
addnewtemplate vote '{"delegate": "20m0c4bzffcmavt6ndfze92qvzeheptm5xq9vxm50xr2xm7s0ymd7g7w0", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda", "rewardmode": 1}'
sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 20w081db9yjg08bbndanqveb4e8kscv750565d1mm5qbjgbas39j43gk2 3000000
```
