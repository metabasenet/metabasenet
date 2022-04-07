#!/usr/bin/env bash
rm -rf ./build
mkdir build
cp -R ../../build/* ./build
docker build -t metabasenet:latest .
rm -rf ./build
