#!/bin/bash 

echo assembling asmtest.s
if ! as -g asmtest.s -o asmtest.o
then
   exit 1
fi
echo compiling vars.c
if ! gcc -g -c -o vars.o vars.c
then
   exit 1
fi

source ./gen-asm-include.sh

RUNTEST()
{
   echo Starting test for $1 $2
   echo generating test for $1 $2...

   INIT
   count=0
   WRITE $1 $2
   FINISH

   echo assembling source opcodes to test.
   if ! as -g opcodes-asm.s -o opcodes-asm.o
   then
      exit 1
   fi

   echo compiling C opcode test initializer
   if ! gcc -g -c opcodes.c -o opcodes.o
   then
      exit 1
   fi

   echo Compiling/Linking $1 $2
   if ! gcc -g -I . 68000-tester.c vars.o asmtest.o opcodes-asm.o opcodes.o -o mc68000-tester
   then
      exit 1
   fi
   
   echo Running $1 $2 test...
   ./mc68000-tester
   echo done $1 $2 test.
   echo 
}

source ./gen-asm-list.sh
