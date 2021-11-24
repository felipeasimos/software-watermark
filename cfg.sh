#!/bin/bash

COREUTILS=$(realpath $(dirname $0)/../coreutils/)
cat $1 | clang -DHAVE_STROPTS_H=0 -I $COREUTILS/src/ -I $COREUTILS/lib/ -emit-llvm -x c -c - -o - | llvm-opt --O1 | llvm-opt --dot-cfg-only -o /dev/null
