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

for o in bchg bclr bset abcd sbcd
do
   WRITE "${o}" "d0,d1"
done

WRITE "extw" "d1"
WRITE "extl" "d1"

FINISH
