CC=gcc

m68k-testgen: m68k-testgen.c asmtest.s opcodes-asm.s opcodes.c opcodes.h
	$(CC) -g -o m68k-testgen m68k-testgen.c asmtest.s \
		    opcodes-asm.s opcodes.c

clean:
	rm -f m68k-testgen asmtest.o opcodes.h opcodes.c opcodes-asm.s \
	      opcodes-asm.o opcodes.o
