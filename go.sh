#!/usr/bin/bash

as -g asmtest.s -o asmtest.o
./gen-asm.sh 
as -g opcodes-asm.s -o opcodes-asm.o
gcc -g -c opcodes.c -o opcodes.o

gcc -g -I ../hdr -I ../cpu68k -I . 68000-tester.c ../cpu68k/cpu68k-0.o ../cpu68k/cpu68k-1.o ../cpu68k/cpu68k-2.o ../cpu68k/cpu68k-3.o ../cpu68k/cpu68k-4.o ../cpu68k/cpu68k-5.o ../cpu68k/cpu68k-6.o ../cpu68k/cpu68k-7.o ../cpu68k/cpu68k-8.o ../cpu68k/cpu68k-9.o ../cpu68k/cpu68k-a.o ../cpu68k/cpu68k-b.o ../cpu68k/cpu68k-c.o ../cpu68k/cpu68k-d.o ../cpu68k/cpu68k-e.o ../cpu68k/cpu68k-f.o ../cpu68k/tab68k.o ../generator/reg68k.o ../generator/cpu68k.o ../generator/diss68k.o ../generator/ui_log.o vars.o asmtest.o opcodes-asm.o opcodes.o -o mc68000-tester
