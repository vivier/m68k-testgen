
/**************************************************************************************\
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

.text
        .align 1
        .align 1
.globl _opcode_bftst
_opcode_bftst:
        pack d2,d0,#1
        nop                
        .align 1
	.word 0x0000
