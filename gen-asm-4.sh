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


# Arithmetic and logical shifts, left/right, long/word/byte.
echo "">>opcodes-asm.s;echo "">>opcodes.c
echo "/* Arithmetic/Logical shifts, Left/Right, Long/Word/Byte */" >>opcodes-asm.s
echo "	/* Arithmetic/Logical shifts, Left/Right, Long/Word/Byte */" >>opcodes.c
echo "">>opcodes-asm.s;echo "">>opcodes.c; echo "">>opcodes.h



for a in l a;
do
 for d in r l;
 do
  for s in l w b;
  do
   WRITE "${a}s${d}${s}" "d0,d1"
  done
 done
done

FINISH
