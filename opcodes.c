
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

#include <stdio.h>

#include <stdlib.h>


typedef unsigned short uint16;

#include "opcodes.h"


void (*test_opcodes[NUMOPCODES])(void);
char   *text_opcodes[NUMOPCODES];
int   pcount_opcodes[NUMOPCODES];

void init_opcodes(void)
{
	test_opcodes[0]=opcode_roxlb; text_opcodes[0]="roxlb"; pcount_opcodes[0]=2;
	test_opcodes[1]=NULL;
}
