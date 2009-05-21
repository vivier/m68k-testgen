CC=gcc

mc68000-tester: 68000-tester.c asmtest.s opcodes-asm.s opcodes.c opcodes.h
	$(CC) -g -o mc68000-tester 68000-tester.c asmtest.s opcodes-asm.s opcodes.c

clean:
	rm -f mc68000-tester asmtest.o opcodes.h opcodes.c opcodes-asm.s opcodes-asm.o opcodes.o
