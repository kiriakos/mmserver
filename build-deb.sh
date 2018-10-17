#!/bin/sh

[ ! -f get-build-deps.sh ] || ./get-build-deps.sh

rm -rf build
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr .. && make -j4 && cpack .. && cd .. && cp build/*.deb .
