#!/bin/bash

# COREUTILS=$(realpath $(dirname $0)/../coreutils/)
# cat $1 | clang -DHAVE_STROPTS_H=0 -I $COREUTILS/src/ -I $COREUTILS/lib/ -emit-llvm -x c -c - -o - | opt-11 --O1 | opt-11 --dot-cfg-only -o /dev/null
cat $1 | clang -emit-llvm -x c -c - -o - | opt-11 --O1 | opt-11 --dot-cfg-only -o /dev/null
