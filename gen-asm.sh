#!/usr/bin/bash

#############################################################
# gen-asm.sh - opcode test assembly generator
#
# Copyright (C) MMIV Ray A. Arachelian, All Rights Reserved
#         Released under the GNU GPL 2.0 License.
#
#############################################################


cat >opcodes-asm.s <<BANNEREND 

/**************************************************************************************\\
*                               Generator Voltmeter                                    *
*                                                                                      *
*                     Copyright (C) MMIV Ray A. Arachelian                             *
*                               All Rights Reserved                                    *
*               Released under the terms of the GNU GPL 2.0 License                    *
*                                                                                      *
*                          MC68000 correctness test suite                              *
*                        Assembled List of Opcodes to Test                             *
*                                                                                      *
*                                                                                      *
* This uses the GNU as to assemble the opcodes, then shadows them in opcodes.c and .h  *
* array to get pointers to them in 16 bit chunks.  We follow the pointer to get to the *
* assembled opcode until we hit a NULL.                                                *
*                                                                                      *
\**************************************************************************************/

BANNEREND

cat opcodes-asm.s >opcodes.c
cat opcodes-asm.s >opcodes.h

echo '#include <stdio.h>'>>opcodes.c;echo "">>opcodes.c
echo '#include <stdlib.h>'>>opcodes.c;echo "">>opcodes.c
echo "">>opcodes.c
echo 'typedef unsigned short uint16;' >>opcodes.c
echo "">>opcodes.c
echo '#include "opcodes.h"'>>opcodes.c;echo "">>opcodes.c
echo "">>opcodes.c

echo ".text" >>opcodes-asm.s
echo "        .align 1" >>opcodes-asm.s

echo "void (*test_opcodes[NUMOPCODES])(void);" >>opcodes.c
#echo "uint16 *test_opcodes[NUMOPCODES];" >>opcodes.c

echo "char   *text_opcodes[NUMOPCODES];" >>opcodes.c
echo ""                           >>opcodes.c
echo "void init_opcodes(void)"    >>opcodes.c
echo "{"                          >>opcodes.c

WRITE() 
{
 OPCODE=$1;
 PARAMS=$2;
   echo "	test_opcodes[${count}]=opcode_${OPCODE};	text_opcodes[${count}]=\"${OPCODE}\";" >>opcodes.c
   #echo "	extern uint16 *opcode_${OPCODE};" >>opcodes.h
   echo "	extern void *opcode_${OPCODE}(void);" >>opcodes.h

   echo "        .align 1" >>opcodes-asm.s
   echo ".globl _opcode_${OPCODE}" >>opcodes-asm.s
   echo "_opcode_${OPCODE}:" >>opcodes-asm.s
   echo "        ${OPCODE} ${PARAMS}" >>opcodes-asm.s
   echo "        nop     " >>opcodes-asm.s
   echo "        .align 1" >>opcodes-asm.s
   echo "	.word 0x0000">>opcodes-asm.s
   echo "">>opcodes-asm.s
   count=$(($count+1));   
}


count=0;

# word only!
for o in divs divu muls mulu
do
 WRITE "${o}" "d0,d1"
done

#byte,long only
for o in bchg bclr bset abcd sbcd
do
   WRITE "${o}" "d0,d1"
done

WRITE "extw" "d1"
WRITE "extl" "d1"


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


echo "">>opcodes-asm.s;echo "">>opcodes.c
echo "/* Rotate Left/Right, Long/Word/Byte */" >>opcodes-asm.s
echo "	/* Rotate Left/Right, Long/Word/Byte */" >>opcodes.c
echo "">>opcodes-asm.s;echo "">>opcodes.c
 for d in r l;
 do
  for s in l w b;
  do
   WRITE "ro${d}${s}" "d0,d1"
  done
 done


echo "">>opcodes-asm.s;echo "">>opcodes.c
echo "/* Rotate with carry in x-flag Left/Right, Long/Word/Byte */" >>opcodes-asm.s
echo "	/* Rotate with carry in x-flag Left/Right, Long/Word/Byte */" >>opcodes.c
echo "">>opcodes-asm.s;echo "">>opcodes.c
 for d in r l;
 do
  for s in l w b;
  do
   WRITE "rox${d}${s}" "d0,d1"
  done
 done

#byte only - single parameter
for o in nbcd scc scs seq sge sgt shi sle sls slt smi sne spl svc svs tas
do
   WRITE "${o}" "d1"
done


echo "	test_opcodes[${count}]=NULL;" >>opcodes.c

count=$(($count+1));   
echo "#define NUMOPCODES $count" >>opcodes.h

echo "}" >>opcodes.c
