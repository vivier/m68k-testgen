/**************************************************************************************\\
*                               Generator Voltmeter                                    *
*                                                                                      *
*                     Copyright (C) MMIV Ray A. Arachelian                             *
*                               All Rights Reserved                                    *
*               Released under the terms of the GNU GPL 2.0 License                    *
*                                                                                      *
*                                                                                      *
*   MC68000 Assembly Opcode Tester Routines.  These MC68000 instructions are to        *
*   be executed on a real Motorola 68040 machine (NeXTStep 3.3) at the same time       *
*   time that Generator CPU core code is executed with the same parameters.  The       *
*   condition code register and output registers are then compared in order to         *
*   detect emulation errors.                                                           *
*                                                                                      *
\**************************************************************************************/


/*-----------------------------------------------------------

 CCR Register bits:

	X . is bit 4 of the CCR
	N . is bit 3 of the CCR
	Z . is bit 2 of the CCR
	V . is bit 1 of the CCR
	C . is bit 0 of the CCR

  ----------------------------------------------------------

 C Prototype for this function call is:

   extern void asmtest(uint16 *ccrin, uint32 *reg1, 
                       uint32 *reg2, uint16 *ccrout);

   word: &ccrin =%a6@(8)  - condition code register before (in)
   long: &reg1  =%a6@(12) - %d0 register (in/out)
   long: &reg2  =%a6@(16) - %d1 register (in/out)
   long: &ccrout=%a6@(20) - condition code register after (out)

 Note that on NeXTStep 3.3/m68k - byte ( 8 bits) is char
                                  word (16 bits) is short
                                  long (32 bits) is int

 Do not get confused, use uint8/16/32 and typedefs to be sure.

  ----------------------------------------------------------*/

/* 68000-tester.c:extern uint16  asmtest(uint16 *ccrin, uint32 *reg1, uint32 *reg2, uint32 *reg3, uint16 *ccrout); */


.text
        .align 1
.globl asmtest
asmtest:

	pea %a6@                      /*  Setup stack frame   */
        movel %sp,%a6
        movel %a2,%sp@-

        movel %a6@(8),%a4              /* Get pointer to ccrin */

        movel %a6@(12),%a0             /* Get pointer to reg1  */
        movel %a6@(16),%a1             /* Get pointer to reg2  */
        movel %a6@(20),%a2             /* Get pointer to ccrout*/

        movel %a6@(24),%a3             /* Get pointer to ccrout*/

        movel %a0@,%d0                 /* Get reg1 into %d0     */
        movel %a1@,%d1                 /* Get reg2 into %d1     */
        movel %a2@,%d2                 /* Get reg2 into %d1     */

        movew %a4@,%d3                 /* Get CCRin            */
        movew %d3,%ccr                 /* copy it to CCR       */

        /*---------------------------------------------------*/
MYOPCODE:
                                     /* execute test opcode  */
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
ENDMYOPC:
        /*---------------------------------------------------*/

        movew %ccr,%d3                 /* Get new CCR value    */

        movel %d0,%a0@                 /* Save %d0 into reg1    */
        movel %d1,%a1@                 /* Save %d1 into reg2    */
        movel %d2,%a2@                 /* Save %d1 into reg2    */

        movew %d3,%a3@                 /* Save CCRout          */
        movew %d3,%d0                  /* Save CCR to retval   */
         
        jra L2                       /* Return to C land     */
        .align 1
L2:
        movel %sp@+,%a2
        unlk %a6
        rts
ENDOFFN:


/*---------------------------------------------------------

 Self modifying code recon functions.

 Get the address of the function.
 Get the size of the function.
 Get a offset in the fn to the opcode we're executing.
 Get the size of the space for opcodes (filled with NOP's)

 This returns a pointer to the opcode in the above function
 that I'm executing.  This is useful for two reasons:

 1. I can make sure that I'm passing the same opcode to
    the generator code as is assembled.

 2. It may be possible to have the high level C code change
    the opcode on the fly. Two problems with this:

    a) have to figure out how to force flush the 
       instruction cache for the MC68040 first while in
       user mode.
    b) NeXTStep doesn't allow me to modify code pages.
       Instead, I'm allocating memory, copying this fn
       to that memory and executing it there.  The allocated
       memory is executeable, and writeable. 

 C Prototype for this function call is:

 extern uint32 *getfnptr(void);
 extern uint32 *getfnsize(void);
 extern uint32 *getopcodeoffset(void);
 extern uint32 *getopcodesize(void);

  ---------------------------------------------------------*/

.globl getfnptr
getfnptr:
        pea %a6@
        movel %sp,%a6
        movel #asmtest,%d0
        jra L3
        .align 1
L3:
        unlk %a6
        rts

.globl getfnsize
getfnsize:
        pea %a6@
        movel %sp,%a6
        movel #asmtest,%d1
        movel #ENDOFFN,%d0
        subl  %d1,%d0
        jra L4
        .align 1
L4:
        unlk %a6
        rts

.globl getopcodeoffset
getopcodeoffset:
        pea %a6@
        movel %sp,%a6
        movel #asmtest,%d1
        movel #MYOPCODE,%d0
        subl  %d1,%d0
        jra L5
        .align 1
L5:
        unlk %a6
        rts

.globl getopcodesize
getopcodesize:
        pea %a6@
        movel %sp,%a6
        movel #MYOPCODE,%d1
        movel #ENDMYOPC,%d0
        subl  %d1,%d0
        jra L6
        .align 1
L6:
        unlk %a6
        rts
