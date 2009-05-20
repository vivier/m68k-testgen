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

#byte,word,long
for o in add addx and cmp eor or sub subx
do
  for s in l w b;
  do
   WRITE "${o}${s}" "d0,d1"
  done
done

#byte long word, but single operand
for o in clr neg negx not
do
 for s in l w b;
  do
   WRITE "${o}${s}" "d1"
  done
done

FINISH
