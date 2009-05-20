#!/bin/bash

#############################################################
# gen-asm-include.sh - opcode test assembly generator fn's
#
# Copyright (C) MMIV Ray A. Arachelian, All Rights Reserved
#         Released under the GNU GPL 2.0 License.
#
#############################################################


WRITE() 
{
 OPCODE=$1
 PARAMS=$2
 NUMPARAMS=2

   if [ "$PARAMS" = "d1" ]; then NUMPARAMS=1; fi

   echo generating assembly for $OPCODE $PARAMS 
   ##echo WRITE \"$OPCODE\" \"$PARAMS\"  >>gen-asm-list.sh

   echo "	test_opcodes[${count}]=opcode_${OPCODE}; text_opcodes[${count}]=\"${OPCODE}\"; pcount_opcodes[${count}]=$NUMPARAMS;" >>opcodes.c
   #echo "	extern uint16 *opcode_${OPCODE};" >>opcodes.h
   echo "	extern void *opcode_${OPCODE}(void);" >>opcodes.h

   echo "        .align 1" >>opcodes-asm.s
   echo ".globl _opcode_${OPCODE}" >>opcodes-asm.s
   echo "_opcode_${OPCODE}:" >>opcodes-asm.s
   echo "        ${OPCODE} ${PARAMS}" >>opcodes-asm.s
   echo "        nop                " >>opcodes-asm.s
   echo "        .align 1" >>opcodes-asm.s
   echo "	.word 0x0000">>opcodes-asm.s
   echo "">>opcodes-asm.s
   count=$(($count+1));   
}

INIT()
{
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
echo "int   pcount_opcodes[NUMOPCODES];" >>opcodes.c
echo ""                           >>opcodes.c
echo "void init_opcodes(void)"    >>opcodes.c
echo "{"                          >>opcodes.c

count=0;
}


FINISH()
{
 echo "	test_opcodes[${count}]=NULL;" >>opcodes.c

 count=$(($count+1));   
 echo "#define NUMOPCODES $count" >>opcodes.h

 echo "}" >>opcodes.c
}
