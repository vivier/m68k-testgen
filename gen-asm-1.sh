#!/usr/bin/bash

#############################################################
# gen-asm.sh - opcode test assembly generator
#
# Copyright (C) MMIV Ray A. Arachelian, All Rights Reserved
#         Released under the GNU GPL 2.0 License.
#
#############################################################


source ./gen-asm-include.sh

INIT

count=0;

# word only!
#for o in mulu muls divs divu
for o in divs divu
do
 WRITE "${o}" "d0,d1"
done

FINISH
