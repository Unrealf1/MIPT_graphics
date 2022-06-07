#!/bin/bash

initial=$(pwd)
cd build
make -j8 && cd ../resources/shaders && python3 compile_landscape_shaders.py && cd ${initial}/bin && ./simple_forward
cd $initial
