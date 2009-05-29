/****************************************************************************\
*                          Generator Voltmeter                               *
*                                                                            *
*                Copyright (C) MMIV Ray A. Arachelian                        *
*                          All Rights Reserved                               *
*          Released under the terms of the GNU GPL 2.0 License               *
*                                                                            *
*                                                                            *
*                     MC68000 correctness tester.                            *
*                                                                            *
*                                                                            *
\****************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>

#include <signal.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/mman.h>

#define PAGE_SIZE		(sysconf(_SC_PAGESIZE))
#define PAGE_MASK		(~(PAGE_SIZE-1))

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;

typedef signed char  sint8;
typedef signed short sint16;
typedef signed int   sint32;

typedef signed char  int8;
typedef signed short int16;
typedef signed int   int32;

#define DREG_SHIFT	0
#define AREG_SHIFT	8
#define IMM16_SHIFT	30
#define IMM32_SHIFT	31
#define DREG(_x)	((mask & (1 << ((_x) + DREG_SHIFT))) != 0)
#define AREG(_x)	((mask & (1 << ((_x) + AREG_SHIFT))) != 0)
#define IMM16()		((mask & (1 << IMM16_SHIFT)) != 0)
#define IMM32()		((mask & (1 << IMM32_SHIFT)) != 0)

#include "patterns.h"

#define X_FLAG 16
#define N_FLAG  8
#define Z_FLAG  4
#define V_FLAG  2
#define C_FLAG  1

#include "opcodes.h"

extern uint16 *test_opcodes[NUMOPCODES];
extern char   *text_opcodes[NUMOPCODES];

static uint8 opcode_to_test[4096];             // execution space for generator.

// our assembly functions

extern uint32 getfnptr(void);
extern uint32 getfnsize(void);
extern uint32 getopcodeoffset(void);
extern uint32 getopcodesize(void);
extern void init_opcodes(void);

extern uint16  asmtest(uint16 *ccrin, uint32 *reg1, uint32 *reg2, uint32 *reg3, uint16 *ccrout);

uint16 (*exec68k_opcode)(uint16 *ccrin, uint32 *reg1, uint32 *reg2, uint32 *reg3, uint16 *ccrout);

uint32 *fnstart, codesize, nopoffset, nopsize;

typedef struct cpu_stat {
    uint32 mask;
    uint16 ccr_in;
    uint16 ccr_out;
    uint32 regs_in[32];
    uint32 regs_out[32];
} cpu_stat;

static int verbose = 0;

static void banner(void)
{
    fprintf(stderr, "\n\n");
    fprintf(stderr, 
"-----------------------------------------------------------------------\n"
"     Generator Meter - MC68000 emulation opcode correctness tester.    \n"
"             A part of The Apple Lisa Emulator Project                 \n"
"  -------------------------------------------------------------------  \n"
"      Copyright (C) MMIV by Ray A Arachelian, All Rights Reserved      \n"
"  Released  under  the terms of  the  GNU Public License  Version 2.0  \n"
"  -------------------------------------------------------------------  \n");
}

static char *getccr(uint16 ccr)
{
    static char text[6];

    text[0]=(ccr & X_FLAG) ? 'X':'.';
    text[1]=(ccr & N_FLAG) ? 'N':'.';
    text[2]=(ccr & Z_FLAG) ? 'Z':'.';
    text[3]=(ccr & V_FLAG) ? 'V':'.';
    text[4]=(ccr & C_FLAG) ? 'C':'.';

    text[5]=0; // terminate string

    return text;
}

static int create_asm_fn(uint8 *newopcode,size_t size, uint32 imm, int set_imm)
{
    uint32 i;
    uint8 *memory;

    if (size > nopsize)
        return 1;

    /* allocate memory for it + pad it to prevent problems just incase. */

    memory = mmap(NULL, (codesize + PAGE_SIZE - 1) & PAGE_MASK,
                  PROT_EXEC | PROT_WRITE | PROT_READ,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == (void*)-1)
        return 2;

    /* copy the function to the allocated memory + some padding. */

    memcpy(memory,fnstart,codesize+16); 

    /* now overwrite the NOP's with the actual opcode we want to test. */

    for (i=0; i < size; i++)
        memory[i + nopoffset] = newopcode[i];

    /* we have an immediate for parameter, put it in */

    switch(set_imm) {
    case 1:    /* 16bit immediat */
        memory[nopoffset + 2] = (imm >> 8) & 0xff;
        memory[nopoffset + 3] = (imm     ) & 0xff;
        break;
    case 2: /* 32bit immediat */
        memory[nopoffset + 2] = (imm >> 24) & 0xff;
        memory[nopoffset + 3] = (imm >> 16) & 0xff;
        memory[nopoffset + 4] = (imm >> 8 ) & 0xff;
        memory[nopoffset + 5] = (imm      ) & 0xff;
        break;
    default:
        break;
    }

    /* Free the old fn if we had one, then assign fn pointer to
     * newly allocated memory.
     * this "malloc first, then free" method avoids cache coherency issues.
     */

    exec68k_opcode = (void *)memory;        

    return 0;
}

static void print_header(uint8 *opcode, int opcode_size)
{
    int i;

    printf("opcode_bin:");
    for (i = 0; i < opcode_size; i+= 2)
        printf("%02x%02x ", opcode[i],opcode[i + 1]);
    printf("\n");
}

static void run_opcodes(uint32 mask)
{
 int status;
 uint16 ccrin=0xff;                      // condition code register  in
 uint16 xccr=0xff;      // condition code registers out
 cpu_stat m68k_stat;

 uint8  *p,*q;                         // execution space for generator.
 int i,j,k0,k1,k2;
 uint32 testsdone=0;
 int set_imm=0;

 for (i=0; i<NUMOPCODES; i++) 
 {

  // setup an opcode to test.
  p=(uint8 *)test_opcodes[i]; q=opcode_to_test;

  if (verbose > 1) {
       fprintf(stderr, "Testing %s (0x%04x)\n",
               text_opcodes[i], test_opcodes[i][0]);
      fflush(stderr);
  }
  
  memcpy(opcode_to_test,test_opcodes[i],16);     // copy the opcode to the generator buffer

  // find the NOP after the opcode so we know
  // many bytes to write over the asmtest fn.

  j=0;
  while (j<16 && // NOP = 0x4E71;
         !(opcode_to_test[j] ==0x4E && opcode_to_test[j+1] ==0x71)) {
    j+=2;
  }

  if (verbose > 1) {
      fprintf(stderr, "Excercising opcode %s %d                    \n\n",
              text_opcodes[i],i);
      fflush(stderr);
  }

  // single operand opcodes, both k0 and k1 need to be commented out.
  // for dual operand opcodes k0 should be commented out.


  k0 = 0;
  do {
   if (test_pattern[k0] == 0xdeadbeef)
     break;

   m68k_stat.regs_in[2] = test_pattern[k0];

  print_header(opcode_to_test, j);

  if (IMM16())
    set_imm = 1;
  else if (IMM32())
    set_imm = 2;
  else
    set_imm = 0;

  if (verbose > 1) {
    fprintf(stderr, "Creating ASM function %s (%d) bytes in length\n",
            text_opcodes[i],j);
    fflush(stderr);
  }

  status=create_asm_fn(opcode_to_test,j,m68k_stat.regs_in[2],set_imm);
  if (status) {fprintf(stderr, "Couldn't create asm function because:%d\n",status); exit(1);}
  //---------------------------------


   k1 = 0;
   do {
    if (test_pattern[k1] == 0xdeadbeef)
     break;
    m68k_stat.regs_in[0] = test_pattern[k1];

    k2 = 0;
    do {
      if (test_pattern[k2] == 0xdeadbeef)
        break;
      m68k_stat.regs_in[1] = test_pattern[k2];

      if (verbose && (testsdone & 0x3ff) == 0) {
        fprintf(stderr,"%s ",text_opcodes[i]);
        if (DREG(0))
            fprintf(stderr, "d0=%08x", m68k_stat.regs_in[0]);
        if (DREG(1))
            fprintf(stderr, ",d1=%08x", m68k_stat.regs_in[1]);
        if (DREG(2))
            fprintf(stderr, ",d2=%08x", m68k_stat.regs_in[2]);
        else if (IMM16())
            fprintf(stderr, ",imm16=%08x", m68k_stat.regs_in[2] & 0xffff);
        else if (IMM32())
            fprintf(stderr, ",imm32=%08x", m68k_stat.regs_in[2]);
        fprintf(stderr," test #%d                 \r", testsdone);
      }

     for (m68k_stat.ccr_in=0; m68k_stat.ccr_in<32; m68k_stat.ccr_in++) // test all combinations of flags.
      {
        if (text_opcodes[i][0]=='d'  && // avoid divide by zero. (DIV[U/S] is word size only hence 0xffff)
            text_opcodes[i][1]=='i'  &&
            text_opcodes[i][2]=='v'  && (m68k_stat.regs_in[0] & 0xffff) == 0 )
            m68k_stat.regs_in[0] = 1;

        // execute the opcodes, first natively under th 68040, then under generator.
        //     execgen_opcode(&ccrin,&gend0,&gend1,&genccrout);

        m68k_stat.regs_out[0] = m68k_stat.regs_in[0];
        m68k_stat.regs_out[1] = m68k_stat.regs_in[1];
        m68k_stat.regs_out[2] = m68k_stat.regs_in[2];

        xccr=exec68k_opcode(&m68k_stat.ccr_in, &m68k_stat.regs_out[0],
                            &m68k_stat.regs_out[1], &m68k_stat.regs_out[2],
                            &m68k_stat.ccr_out);

        testsdone++; 

         // sanity check - might not be needed.
         if (xccr!=m68k_stat.ccr_out) {fprintf(stderr,"xccr!=m68ccrout! %d!=%d\n",xccr,m68k_stat.ccr_out); exit(1);}

	  printf("broken opcode: %s\n",text_opcodes[i]);
          printf("before d0=%08x    d1=%08x",
                 m68k_stat.regs_in[0], m68k_stat.regs_in[1]);
          if (DREG(2))
              printf("    d2=%08x", m68k_stat.regs_in[2]);
          else if (IMM16())
              printf("    imm16=%08x", m68k_stat.regs_in[2]);
          else if (IMM32())
              printf("    imm32=%08x", m68k_stat.regs_in[2]);
          printf("    CCR=%s (%d)\n", getccr(ccrin), ccrin);
          printf("M68K   d0=%08x    d1=%08x",
                 m68k_stat.regs_out[0], m68k_stat.regs_out[1]);
          if (DREG(2))
              printf("    d2=%08x", m68k_stat.regs_out[2]);
          else if (IMM16())
              printf("    imm16=%08x", m68k_stat.regs_out[2]);
          else if (IMM32())
              printf("    imm32=%08x", m68k_stat.regs_out[2]);
          printf("    CCR=%s (%d)\n",
                 getccr(m68k_stat.ccr_out), m68k_stat.ccr_out); 
          printf("GEN    d0=%08x    d1=%08x",
                 m68k_stat.regs_out[0], m68k_stat.regs_out[1]);
          if (DREG(2))
              printf("    d2=%08x", m68k_stat.regs_out[2]);
          else if (IMM16())
              printf("    imm16=%08x", m68k_stat.regs_out[2]);
          else if (IMM32())
              printf("    imm32=%08x", m68k_stat.regs_out[2]);
          printf("    CCR=%s (%d)\n\n",
                 getccr(m68k_stat.ccr_out), m68k_stat.ccr_out);

          fflush(stdout);
         } // end of k2 
         k2++;
       } while (DREG(1));
       k1++;
      } while (DREG(0));

      munmap(exec68k_opcode, (codesize + PAGE_SIZE - 1) & PAGE_MASK);
      k0++;
 } while (DREG(2) || IMM16() || IMM32());
  
  printf("%d errors of %d tests done for %s (%02x%02x)                                  \n",0,testsdone,text_opcodes[i],opcode_to_test[0],opcode_to_test[1]); 

 } // end of opcode loop.

 if (verbose) 
   fprintf(stderr,"                                                                \r"); 
 // add a newline to linefeed since update display above doesn't.
 return;
}

static void Usage(int argc, char **argv)
{
    fprintf(stderr, "Usage: %s [-v|--verbose][-r|--registers=<registers>]\n", argv[0]);
    fprintf(stderr,
        "    -v|--verbose      verbose mode (on stderr)\n");
    fprintf(stderr,
        "    -r|--registers    comma separated list of registers to use\n");
    fprintf(stderr, "                      (Default %%d0,%%d1)\n");
    fprintf(stderr, "\nExample:\n");
    fprintf(stderr, "    %s --directory=/mnt/next --compress=\"gzip -1\" --registers=\"%%d0,%%d1\"\n", argv[0]);
    fprintf(stderr, "\n");
}

static int registers_mask(const char *s, uint32 *mask)
{
    enum { UNKNOWN, IMMEDIAT, DATA, ADDRESS } type = UNKNOWN;
    int reg, shift;

    *mask = 0;
    while(*s) {
        switch(type) {
        case UNKNOWN:
            switch(*s) {

            /* data registers */

            case 'd':
            case 'D':
                type = DATA;
                break;

            /* address registers */

            case 'A':
            case 'a':
                type = ADDRESS;
                break;

            /* immediat */

            case '#':
                type = IMMEDIAT;
                break;

            /* frame pointer */

            case 'F':
            case 'f':
                s++;
                if (*s != 'p')    /* A6 */
                    return -1;
                *mask |= 1 << (6 + AREG_SHIFT);
                break;
                
            /* stack pointer */

            case 'S':
            case 's':
                s++;
                if (*s != 'p')    /* A7 */
                    return -1;
                *mask |= 1 << (7 + AREG_SHIFT);
                break;
                
            /* allowed separators */

            case ',':
            case '%':
            case '$':
            case ':':
            case '{':
            case '}':
            case ' ':
                break;
            default:
                return -1;
            }
            break;
        case DATA:
            shift = DREG_SHIFT;
            goto set_bit;
        case ADDRESS:
            shift = AREG_SHIFT;
set_bit:
            reg = (*s) - '0';
            if (reg < 0 || reg > 7)
                return -1;
            *mask |= 1 << (reg + shift);
            type = UNKNOWN;
            break;
        case IMMEDIAT:
            if (*s != '0')
                return -1;
            s++;
            if (*s != '.') {
                *mask |= 1 << IMM16_SHIFT;
                s--;
            } else {
                s++;
                switch(*s) {
                    case 'w':
                        *mask |= 1 << IMM16_SHIFT;
                        break;
                    case 'l':
                        *mask |= 1 << IMM32_SHIFT;
                        break;
                }
            }
            type = UNKNOWN;
            break;
        }
        s++;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int option_index = 0;
    static struct option long_options[] = {
        { "registers", 1, NULL, 'r' },
        { "verbose", 1, NULL, 'v' },
        { "help", 0, NULL, 'h' },
        {0, 0, 0, 0}
    };
    int c;
    uint32 mask = 0x3;    /* %d0,%d1 */

    while (1) {
        c = getopt_long(argc, argv, "vhr:",
                long_options, &option_index);
            if (c == -1)
            break;

        switch(c) {
        case 'r':
            if (registers_mask(optarg, &mask) || !mask) {
                fprintf(stderr, "Error: Invalid registers\n");
                Usage(argc, argv);
                exit(1);
            }
            break;
        case 'v':
            verbose++;
            break;
        case 'h':
        case '?':
            Usage(argc, argv);
            exit(1);
        default:
            fprintf(stderr, "Error: unknown parameter %s\n", optarg);
            break;
        }
    }
    if (argc != optind) {
        fprintf(stderr, "Invalid number of argument\n");
        Usage(argc, argv);
        exit(1);
    }

    if (verbose > 1)
        banner();
    
    // get fn start, codesize, nopoffset, nopsizes

    fnstart  = (uint32*)getfnptr();  
    codesize = getfnsize();
    nopoffset= getopcodeoffset();
    nopsize  = getopcodesize();

    init_opcodes();

    run_opcodes(mask); 

    return 0;
}
