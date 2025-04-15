#!/bin/bash

rm -rf cmake-build-release

rpcgen rpc_api.x

cmake -S ./ -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release
make -C cmake-build-release