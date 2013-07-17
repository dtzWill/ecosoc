#! /usr/bin/env bash

if [[ "$1" != "" ]]; then
  TMP=`mktemp`
  clang -S -emit-llvm -g $1 -o $TMP
  opt -load ArAnot.so -aranot-metadata -aranot -aranot-output "aranot.$1" $TMP &> /dev/null
fi

