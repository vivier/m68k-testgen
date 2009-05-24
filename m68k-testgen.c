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

#define GZIP "/bin/gzip"

// behavior of test results.
// #define SKIP_CCR_TESTS 1
// #define SKIP_PASSED_TESTS 1
// #define LIMIT_FAILURES 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>

#include <signal.h>
#include <getopt.h>
#include <libgen.h>

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
#define IMM_SHIFT	31
#define DREG(_x)	((mask & (1 << ((_x) + DREG_SHIFT))) != 0)
#define AREG(_x)	((mask & (1 << ((_x) + AREG_SHIFT))) != 0)
#define IMM()		((mask & (1 << IMM_SHIFT)) != 0)

#include "patterns.h"

#define X_FLAG 16
#define N_FLAG  8
#define Z_FLAG  4
#define V_FLAG  2
#define C_FLAG  1

#include "opcodes.h"

#define DEFAULT_DIR "."

extern uint16 *test_opcodes[NUMOPCODES];
extern char   *text_opcodes[NUMOPCODES];

static uint8 opcode_to_test[4096];             // execution space for generator.

static void banner(void)
{

 //         .........1.........2.........3.........4.........5.........6.........7
 //         123456789012345678901234567890123456789012345678901234567890123456789012345678
    printf("\n\n");
    printf("-----------------------------------------------------------------------\n");
    printf("     Generator Meter - MC68000 emulation opcode correctness tester.    \n");
    printf("             A part of The Apple Lisa Emulator Project                 \n");
    printf("  -------------------------------------------------------------------  \n");
    printf("      Copyright (C) MMIV by Ray A Arachelian, All Rights Reserved      \n");
    printf("  Released  under  the terms of  the  GNU Public License  Version 2.0  \n");
    printf("  -------------------------------------------------------------------  \n");
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

// our assembly functions

extern uint32 getfnptr(void);
extern uint32 getfnsize(void);
extern uint32 getopcodeoffset(void);
extern uint32 getopcodesize(void);
extern void init_opcodes(void);

extern uint16  asmtest(uint16 *ccrin, uint32 *reg1, uint32 *reg2, uint32 *reg3, uint16 *ccrout);

uint16 (*exec68k_opcode)(uint16 *ccrin, uint32 *reg1, uint32 *reg2, uint32 *reg3, uint16 *ccrout);

uint32 *fnstart, codesize, nopoffset, nopsize;

static int create_asm_fn(uint8 *newopcode,size_t size, uint32 orgd2, int packflag)
{
    uint32 i;
    uint8 *memory,*fnmem;

    if (size>nopsize) return 1;

    // allocate memory for it + pad it to prevent problems just incase.
    memory=(uint8 *)malloc(codesize<65536 ? 65536:codesize+32768);
    if (!memory) return 2;

    // copy the function to the allocated memory + some padding.
    // (should change these to do long/read+writes, but it doesn't much matter.)

    memcpy(memory,fnstart,codesize+16); 
    //for (fnmem=(char *)fnstart, i=0; i<codesize+16; i++) memory[i]=fnmem[i];

    // now overwrite the NOP's with the actual opcode we want to test.
    if (size) for (i=0; i<size; i++)  memory[i+nopoffset]=newopcode[i];

    if (packflag) {memory[nopoffset+2]= (orgd2>>8) & 0xff; memory[nopoffset+3]=(orgd2   ) & 0xff;}

    // if size==0, that means I'm just testing the copy mechanism. :)

    // Free the old fn if we had one, then assign fn pointer to newly allocated memory.
    // this "malloc first, then free" method avoids cache coherency issues.


    // memory leak to prevent over malloc/free'd.

    if (exec68k_opcode!=NULL) free(exec68k_opcode);
    exec68k_opcode=(void *)memory;        

    return 0;
}

static void run_opcodes(const char *directory, const char *compress,
			uint32 mask)
{
 FILE *fd=NULL;
 int status;
 uint16 ccrin=0xff;                      // condition code register  in
 uint16 m68ccrout=0xff, genccrout=0xff;  // condition code registers out
 uint16 diffccrout=0xff, xccr=0xff;      // condition code registers out
 uint32 orgd0=0,    orgd1=0,  orgd2;     // original  d0/d1 registers
 uint32 m68d0=0,    m68d1=0,  m68d2;     // m68k      d0/d1 registers
 uint32 gend0=0,    gend1=0,  gend2;     // generator d0/d1 registers
 uint32 diffd0=0,   diffd1=0, diffd2=0;  // generator d0/d1 registers

 long numdiffs=0;

 size_t opc_size=0;                      // size of the opcode to test.
 uint8  *p,*q,x;                         // execution space for generator.
 int i,j,k,k0,k1,k2,failed=0,skippy=0,skippy2=0;
 char c;
 char pipecmd[1024];
 uint32 testsdone=0;

 for (i=0; i<NUMOPCODES; i++) 
 {

  // setup an opcode to test.
  p=(uint8 *)test_opcodes[i]; q=opcode_to_test;
  j=0;

  fprintf(stderr,"Testing %s\n",text_opcodes[i]); fflush(stderr);
  
  if (!p) { exit(1);}
  if (!q) { exit(1);}

  fprintf(stderr,"Copying opcode %04x to buffer\n",test_opcodes[i][0]); fflush(stderr);

  memcpy(opcode_to_test,test_opcodes[i],16);     // copy the opcode to the generator buffer

  // find the NOP after the opcode so we know
  // many bytes to write over the asmtest fn.

  j=0;
  while (j<16 && // NOP = 0x4E71;
         !(opcode_to_test[j] ==0x4E && opcode_to_test[j+1] ==0x71)) {
    j+=2;
  }

  fprintf(stderr,"Excercising opcode %s %d                    \n\n",text_opcodes[i],i); fflush(stderr);

  skippy=0; skippy2=0; numdiffs=0;

  // single operand opcodes, both k0 and k1 need to be commented out.
  // for dual operand opcodes k0 should be commented out.


  if (DREG(2) || IMM())
    orgd2 = test_pattern[0];
  else
    orgd2 = 0;
  k0 = 0;
  do {
   int packflag=0;

  if (fd!=NULL) pclose(fd);

  if (compress) {
    char *name = basename((char*)compress);

    if (DREG(2) || IMM())
      sprintf(pipecmd, "%s > %s/m68040-opcode-%s.d2=%08x.txt.%c%c",
	      compress, directory, text_opcodes[i], orgd2, name[0], name[1]);
    else
      sprintf(pipecmd, "%s > %s/m68040-opcode-%s.txt.%c%c",
	      compress, directory, text_opcodes[i], name[0], name[1]);

    fprintf(stderr,"\nOpening pipe to %s\n\n",pipecmd);

    fd = popen(pipecmd,"w");
    if (!fd) {
      perror("Could not open gzip pipe.");
      exit(1);
    }
  } else {

    if (DREG(2) || IMM())
      sprintf(pipecmd, "%s/m68040-opcode-%s.d2=%08x.txt",
	      directory, text_opcodes[i], orgd2);
    else
      sprintf(pipecmd, "%s/m68040-opcode-%s.txt",
	      directory, text_opcodes[i]);

    fprintf(stderr,"\nOpening file %s\n\n",pipecmd);

    fd = fopen(pipecmd, "w");
    if (!fd) {
      perror("Could not open file.");
      exit(1);
    }
  }

  fprintf(fd,"opcode_bin:");
  for (k = 0; k < j; k+= 2)
    fprintf(fd,"%02x%02x",opcode_to_test[k],opcode_to_test[k+1]);
  fprintf(fd,"\n");
 
  //---------------------------------
  if ((text_opcodes[i][0]=='p'  && // avoid divide by zero. (DIV[U/S] is word size only hence 0xffff)
       text_opcodes[i][1]=='a'  &&
       text_opcodes[i][2]=='c'  &&
       text_opcodes[i][3]=='k'    )
                                    ||
      (text_opcodes[i][0]=='u'  && 
       text_opcodes[i][1]=='n'  &&
       text_opcodes[i][2]=='p'  &&
       text_opcodes[i][3]=='k'    )  )    packflag=1;

  fprintf(stderr,"Creating ASM function %s (%d) bytes in length\n",text_opcodes[i],j); fflush(stderr);

  status=create_asm_fn(opcode_to_test,j,orgd2,packflag);
  if (status) {printf("Couldn't create asm function because:%d\n",status); exit(1);}
  //---------------------------------


   if (DREG(0))
       orgd0 = test_pattern[0];
   else
       orgd0 = 0;
   k1 = 0;
   do {

    for (k2=0; (orgd1=test_pattern[k2])!=0xdeadbeef; k2++)
    {
      if ((testsdone & 0x3ff) == 0) {
        fprintf(stderr,"%s ",text_opcodes[i]);
        if (DREG(0))
            fprintf(stderr, "d0=%08x", orgd0);
        if (DREG(1))
            fprintf(stderr, ",d1=%08x", orgd1);
        if (DREG(2) || IMM())
            fprintf(stderr, ",d2=%08x", orgd2);
        fprintf(stderr," test #%ld                 \r", testsdone);
      }

#ifndef LIMIT_FAILURES
     skippy=0; skippy2=0;
#endif

#ifdef SKIP_CCR_TESTS
     #define CCRINC 31
#else
     #define CCRINC  1
#endif

     for (ccrin=0,skippy=0; ccrin<32; ccrin+=CCRINC) // test all combinations of flags.
      {
        if (text_opcodes[i][0]=='d'  && // avoid divide by zero. (DIV[U/S] is word size only hence 0xffff)
            text_opcodes[i][1]=='i'  &&
            text_opcodes[i][2]=='v'    )
            {orgd0=(orgd0 & 0xffff)>0 ? orgd0 : 1;}

        gend0=orgd0; gend1=orgd1;  gend2=orgd2;
        m68d0=orgd0; m68d1=orgd1;  m68d2=orgd2; 

        // execute the opcodes, first natively under th 68040, then under generator.
        //     execgen_opcode(&ccrin,&gend0,&gend1,&genccrout);
        xccr=exec68k_opcode(&ccrin,&m68d0,&m68d1,&m68d2,&m68ccrout);

        testsdone++; 


         // sanity check - might not be needed.
         if (xccr!=m68ccrout) {fprintf(stderr,"xccr!=m68ccrout! %d!=%d\n",xccr,m68ccrout); exit(1);}

	  fprintf(fd, "broken opcode: %s\n",text_opcodes[i]);
          fprintf(fd, "before d0=%08lx    d1=%08lx    CCR=%s (%d)\n"  ,orgd0,orgd1,getccr(ccrin),ccrin);
          fprintf(fd, "M68K   d0=%08lx    d1=%08lx    CCR=%s (%d)\n"  ,m68d0,m68d1,getccr(m68ccrout),m68ccrout); 
          fprintf(fd, "GEN    d0=%08lx    d1=%08lx    CCR=%s (%d)\n\n",
                gend0, gend1,
                getccr(genccrout), genccrout);

          if (failed) numdiffs++;
          // if we did one whole round of flags, suppress the rest of the output for other rounds.
          // since most of our output is because of flags...

#ifdef LIMIT_FAILURES
 	  if (ccrin==31 && k1>0) skippy=30; 
	  else skippy2++;
#endif
          fflush(stdout);
         } // end of k2 
       } // end of CCRin loop
       k1++;
      } while (DREG(0) &&
               (orgd0 = test_pattern[k1] )!=0xdeadbeef &&
               skippy2 < 65536);

      k0++;
 } while ((DREG(2) || IMM()) &&
          (orgd2 = test_pattern[k0]) != 0xdeadbeef &&
          skippy2 < 65536);
  
  fprintf(fd,"\n%ld errors of %ld tests done for %s (%02x%02x)                                  \n\n",numdiffs,testsdone,text_opcodes[i],opcode_to_test[0],opcode_to_test[1]); 

  if (fd!=NULL) pclose(fd);
  
 } // end of opcode loop.

 fprintf(stderr,"                                                                \n\n"); 
 // add a newline to linefeed since update display above doesn't.
 return;
}

static void Usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [-d|--directory <directory>][-c|--compress <tool>][-r|--registers=<registers>]\n", argv[0]);
	fprintf(stderr,
		"    -d|--directory    define directory where to save data (Default \""DEFAULT_DIR"\")\n");
	fprintf(stderr,
		"    -c|--compress     define the command used to compress data\n");
	fprintf(stderr, "                      (Default no compression)\n");
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
				if (*s != 'p')	/* A6 */
					return -1;
				*mask |= 1 << (6 + AREG_SHIFT);
				break;
				
			/* stack pointer */

			case 'S':
			case 's':
				s++;
				if (*s != 'p')	/* A7 */
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
			*mask |= 1 << IMM_SHIFT;
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
	char *directory = DEFAULT_DIR;
	char *compress = NULL;
	static struct option long_options[] = {
		{ "directory", 1, NULL, 'd' },
		{ "compress", 1, NULL, 'c' },
		{ "registers", 1, NULL, 'r' },
		{ "help", 0, NULL, 'h' },
		{0, 0, 0, 0}
	};
	int c;
	uint32 mask = 0x3;	/* %d0,%d1 */

	while (1) {
		c = getopt_long(argc, argv, "hd:c:r:",
				long_options, &option_index);
        	if (c == -1)
			break;

		switch(c) {
		case 'd':
			directory = optarg;
			break;
		case 'c':
			compress = optarg;
			break;
		case 'r':
			if (registers_mask(optarg, &mask) || !mask) {
				fprintf(stderr, "Error: Invalid registers\n");
				Usage(argc, argv);
				exit(1);
			}
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

	banner();
	
	/* Set the stage */
	 
	exec68k_opcode=NULL;                    // make sure we don't free illegally.

	// get fn start, codesize, nopoffset, nopsizes

	fnstart  = (uint32*)getfnptr();  
	codesize = getfnsize();
	nopoffset= getopcodeoffset();
	nopsize  = getopcodesize();

	init_opcodes();

	run_opcodes(directory, compress, mask); 
}
