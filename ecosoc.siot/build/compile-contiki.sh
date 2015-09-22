#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Descomprimindo contiki-2.6.zip"
unzip contiki-2.6.zip

echo "Criando backup de Makefiles"
cp contiki-2.6/Makefile.include contiki-2.6/Makefile.include.orig
cp contiki-2.6/cpu/native/Makefile.native contiki-2.6/cpu/native/Makefile.native.orig

echo "Substituindo Makefiles"
cp ${DIR}/ctkMakeInc contiki-2.6/Makefile.include
cp ${DIR}/ctkMakeNat contiki-2.6/cpu/native/Makefile.native

#echo "Compilando exemplo hello-world usando gcc com address-sanitizer"
cd contiki-2.6/examples/hello-world
#${DIR}/makegccasan clean
#${DIR}/makegccasan -j2

echo "Compilando exemplo hello-world usando llvm"
cd contiki-2.6/examples/hello-world
#${DIR}/makeclang clean
${DIR}/makeclang -j4

echo "Compilando exemplo udp-ipv6 usando llvm"
cd ${DIR}/contiki-2.6/examples/udp-ipv6
#${DIR}/makeclang clean
${DIR}/makeclang -j4
