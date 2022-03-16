#!/bin/bash

cd build
make -j7
cd ../resources/shaders/
python3 compile_shadowmap_shaders.py
res=$?
cd ../../
exit $res

