#!/usr/bin/bash 

echo assembling asmtest.s
as -g asmtest.s -o asmtest.o

source ./gen-asm-include.sh

RUNTEST()
{
	INIT
	count=0
	WRITE $1 $2
	FINISH

   echo assembling source opcodes to test.
   as -g opcodes-asm.s -o opcodes-asm.o
   echo compiling C opcode test initializer
   gcc -g -c opcodes.c -o opcodes.o

   echo Compiling/Linking $1 $2
   gcc -g -I ../hdr -I ../cpu68k -I . 68000-tester.c ../cpu68k/cpu68k-0.o ../cpu68k/cpu68k-1.o ../cpu68k/cpu68k-2.o ../cpu68k/cpu68k-3.o ../cpu68k/cpu68k-4.o ../cpu68k/cpu68k-5.o ../cpu68k/cpu68k-6.o ../cpu68k/cpu68k-7.o ../cpu68k/cpu68k-8.o ../cpu68k/cpu68k-9.o ../cpu68k/cpu68k-a.o ../cpu68k/cpu68k-b.o ../cpu68k/cpu68k-c.o ../cpu68k/cpu68k-d.o ../cpu68k/cpu68k-e.o ../cpu68k/cpu68k-f.o ../cpu68k/tab68k.o ../generator/reg68k.o ../generator/cpu68k.o ../generator/diss68k.o ../generator/ui_log.o vars.o asmtest.o opcodes-asm.o opcodes.o -o mc68000-tester
   
   echo Running $1 $2 test...
   ./mc68000-tester
   echo done $1 $2 test.
   echo 
}

source ./gen-asm-list.sh

#while sleep 10; do beep;done
