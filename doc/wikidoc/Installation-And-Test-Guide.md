# Installation and test methods

## 1）Source installation：
```
sudo apt install -y g++ libboost-all-dev openssl libreadline-dev pkg-config libncurses5-dev autoconf
```
**ubuntu16.04**
```
sudo apt install -y libssl-dev
```
**ubuntu18.04**
```
sudo apt install -y libssl1.0-dev libprotobuf-dev protobuf-compiler
```

**Compile cmake**
```
sudo apt autoremove cmake
wget https://github.com/Kitware/CMake/releases/download/v3.22.3/cmake-3.22.3.tar.gz
tar -zxvf cmake-3.22.3.tar.gz
cd cmake-3.22.3
./bootstrap 
make
sudo make install
sudo ln -s /usr/local/bin/cmake /usr/bin
cmake --version
```

**Installation gcc10**
```
sudo apt install build-essential
sudo apt install software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt install gcc-10 g++-10
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100 --slave /usr/bin/g++ g++ /usr/bin/g++-10
sudo update-alternatives --config gcc

```

**Compile libsodium (version >= 1.0.18)**
```
wget https://github.com/jedisct1/libsodium/releases/download/1.0.18-RELEASE/libsodium-1.0.18.tar.gz --no-check-certificate
tar -zxvf libsodium-1.0.18.tar.gz
cd libsodium-1.0.18
./configure
make -j8 && make check
sudo make install
```
**Compile protobuf**
```
wget https://github.com/protocolbuffers/protobuf/releases/download/v3.11.3/protobuf-cpp-3.11.3.tar.gz
tar -xzvf protobuf-cpp-3.11.3.tar.gz
cd protobuf-3.11.3
./configure --prefix=/usr
make -j8
sudo make install
```
**Download code and compile**
```
git clone https://github.com/metabasenetwork/metabasenet.git
cd metabasenet
./INSTALL.sh
```

## 2）Configuration node
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

## 3）Start / stop node
**Start node：**
```
$ metabasenet -debug -daemon -testnet
```
**Stop node：**
```
$ metabasenet -testnet stop

Note: the above is to start the test network (with parameter: - testnet), which can easily and quickly obtain the test results.
```

## 4）Import initial address
**Start client：**
```
$ metabasenet-cli -testnet
```

**Execute the following command on the client：**
```
importprivkey 14e1abd0802f7065b55f5076d0d2cfbea159abd540a977e8d3afd4b3061bf47f 123
unlockkey 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 123
importprivkey 42b889a2668eda6d78682c23b5651fb76b5aac2b71caba1aa23b6b14d5ce75b7 123
unlockkey 1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda 123

addnewtemplate delegate '{"delegate": "883f29fb7740f8e625159cb6bcee42cf3ac460be8b9fcb839ccbfeda3744b217", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda", "rewardratio": 500}'

sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 20m0f7zkhbd840hk34zwfeazr8cwkkskxhcdqpz0jw6rx2a160bb38wth 10000000
```

## 5）Create the template address of currency deposit and interest generation and transfer it to token
**Start client：**
```
$ metabasenet-cli -testnet
```

**Execute the following command on the client：**
```
addnewtemplate vote '{"delegate": "20m0f7zkhbd840hk34zwfeazr8cwkkskxhcdqpz0jw6rx2a160bb38wth", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda", "rewardmode": 1}'
sendfrom 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 20w0fxkagqhjty33ahctq072e681tdjfwh16vqxajm3s194y7v4kaj4j2 3000000

```
