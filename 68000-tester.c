/**************************************************************************************\
*                               Generator Voltmeter                                    *
*                                                                                      *
*                     Copyright (C) MMIV Ray A. Arachelian                             *
*                               All Rights Reserved                                    *
*               Released under the terms of the GNU GPL 2.0 License                    *
*                                                                                      *
*                                                                                      *
*                          MC68000 correctness tester.                                 *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

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

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;


typedef signed char  sint8;
typedef signed short sint16;
typedef signed int   sint32;


typedef signed char  int8;
typedef signed short int16;
typedef signed int   int32;

#include "patterns.h"

#define X_FLAG 16
#define N_FLAG  8
#define Z_FLAG  4
#define V_FLAG  2
#define C_FLAG  1

#include "opcodes.h"

extern uint16 *test_opcodes[NUMOPCODES];
extern char   *text_opcodes[NUMOPCODES];
extern int    pcount_opcodes[NUMOPCODES];

uint16 myopcode; 
unsigned int gen_quit = 0;
unsigned int gen_debugmode = 1;

void initialize_all_subsystems(void);

uint8 opcode_to_test[4096];             // execution space for generator.

//for RC parsing
char *rstrip(char *s)
{
	char space=' ';
	int32 i;
	i=strlen(s)-1;
	if (i<=0) return s;
	while (s[i]<=space && i>0) {s[i]=0; i--;}
	return s;
}

/* strip white space at the left (start) of a string.
   First we check that there is some, if there isn't,
   we bail out fast, if there is, we start copying the
   string over itself. For safety, we also check the
   size of the string so we prevent buffer overflows,
   incase we accidentally don't have a string terminator. */

// for RC parsing
char *lstrip(char *s, size_t n)
{
	char space=' ';
	uint32 i=0, j=0;

	if (*s>space) return s; // nothing to do, no ws leftmost

	while (s[i]<=space && i<n) i++; // find first nonspace.
	while (s[i] && i<n)	s[j++]=s[i++]; // move the string over

	s[j++]=0; // make sure that we terminate string properly.
	return s;
}

// for RC parsing
int isalphanumeric(char *s)
{
	while (*s)
	{ if (isalnum((int)(*s))) s++; else return 0; }
	return 1;
}

// for RC parsing
char *stringtoupper(char *s)
{
	while ( (*s=toupper(*s)) ) s++; return s;
}

void banner(void)
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

char *getccr(uint16 ccr)
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

int create_asm_fn(uint8 *newopcode,size_t size, uint32 orgd2, int packflag)
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



    if (size) 
       {
        FILE *assembly=fopen("/mnt/next/opcode-bytes.txt","a");
        fprintf(assembly,"   opcode binary: ");

          for (i=0; i<size; i++)  
              { 
               fprintf(assembly,"%02x",newopcode[i]); if (i &1) fprintf(assembly," ");
              }
          fprintf(assembly,"\n");
          fflush(assembly);
          fclose(assembly);
       }



    if (packflag) {memory[nopoffset+2]= (orgd2>>8) & 0xff; memory[nopoffset+3]=(orgd2   ) & 0xff;}

    // if size==0, that means I'm just testing the copy mechanism. :)

    // Free the old fn if we had one, then assign fn pointer to newly allocated memory.
    // this "malloc first, then free" method avoids cache coherency issues.


    // memory leak to prevent over malloc/free'd.

    if (exec68k_opcode!=NULL) free(exec68k_opcode);
    exec68k_opcode=(void *)memory;        

    return 0;
}

void run_opcodes();


int main(int argc, char *argv[])
{


 banner();

 /* Set the stage */
 
 exec68k_opcode=NULL;                    // make sure we don't free illegally.
                                         // get fn start, codesize, nopoffset, nopsizes
 fnstart  = getfnptr();  
 codesize = getfnsize();
 nopoffset= getopcodeoffset();
 nopsize  = getopcodesize();
 init_opcodes();

 run_opcodes(); 
}


void run_opcodes()
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
 int i,j,k0,k1,k2,failed=0,skippy=0,skippy2=0;
 char c;
 char pipecmd[1024];
 uint32 testsdone=0;

typedef struct binary_output
{
 uint32 head;
 uint32 testnum;
 uint32 input_d0;
 uint32 input_d1;
 uint32 input_d2;
 uint32 input_ccr;
 uint32 separator;
 uint32 output_d0;
 uint32 output_d1;
 uint32 output_d2;
 uint32 output_ccr;
 uint32 end;

} bin_output_t;

bin_output_t output;
 


 for (i=0; i<NUMOPCODES; i++) 
 {


  // setup an opcode to test.
  p=(uint8 *)test_opcodes[i]; q=opcode_to_test;
  j=0;

  fprintf(stderr,"Testing %s\n",text_opcodes[i]); fflush(stderr);
  
  if (!p) {fprintf(stderr,"p==NULL\n"); exit(1);}
  if (!q) {fprintf(stderr,"q==NULL\n"); exit(1);}

  fprintf(stderr,"Copying opcode %04x to buffer\n",test_opcodes[i][0]); fflush(stderr);

  //fprintf(stderr,"reading...:%d pointer is %p %p %p\n",j,opcode_divs,p,test_opcodes[i]); fflush(stderr);
  //fprintf(fd,"opcode_bin:");


  memcpy(opcode_to_test,test_opcodes[i],16);     // copy the opcode to the generator buffer

  j=0; while (j<16 && opcode_to_test[j]!=0x4E && // find the NOP after the opcode so we know
              opcode_to_test[j+1]!=0x71) {       // find the NOP after the opcode so we know
                                                 // many bytes to write over the asmtest fn.
                                                 // NOP = 0x4E71;
              
              //fprintf(fd,"%02x%02x",opcode_to_test[j],opcode_to_test[j+1]); 
              j+=2; }

  //fprintf(fd,"\n");

//  fprintf(stderr,"Creating ASM function %s (%d) bytes in length\n",text_opcodes[i],j); fflush(stderr);
//  status=create_asm_fn(opcode_to_test,j,0,0);
//  if (status) {printf("Couldn't create asm function because:%d\n",status); exit(1);}


  fprintf(stderr,"Excercising opcode %s %d                    \n\n",text_opcodes[i],i); fflush(stderr);


  skippy=0; skippy2=0; numdiffs=0;

  // single operand opcodes, both k0 and k1 need to be commented out.
  // for dual operand opcodes k0 should be commented out.


#ifdef ASSEMBLE_ONLY
  {
  int packflag=0;
  FILE *assembly=fopen("/mnt/next/opcode-bytes.txt","a");
  fprintf(assembly,"Testing %s :",text_opcodes[i]); fflush(assembly);
  fprintf(stderr,"Testing %s :",text_opcodes[i]); fflush(stderr);
  fclose(assembly);

  status=create_asm_fn(opcode_to_test,j,orgd2,packflag);
  if (status) {printf("Couldn't create asm function because:%d\n",status); exit(1);}
  }

#else


 orgd2=0;
 //for (k0=0;  (orgd2=test_pattern[k0])!=0xdeadbeef && skippy2<65536; k0++)  // only used if you have 3 operands! comment out otherwise.
  {
   int packflag=0;
 
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

  {
  FILE *assembly=fopen("/mnt/next/opcode-bytes.txt","a");
  fprintf(assembly,"Testing %s :",text_opcodes[i]); fflush(assembly);
  fprintf(stderr,"Testing %s :",text_opcodes[i]); fflush(stderr);
  fclose(assembly);
  }

  status=create_asm_fn(opcode_to_test,j,orgd2,packflag);
  if (status) {printf("Couldn't create asm function because:%d\n",status); exit(1);}
  //---------------------------------

  if (fd!=NULL) pclose(fd);

  sprintf(pipecmd,GZIP" -1 >/mnt/next/m68040-opcode-%s.d2=%08x.bin.gz",text_opcodes[i],orgd2);
  // sprintf(pipecmd,GZIP" -1 >/mnt/next/m68040-opcode-%s.bin.gz",text_opcodes[i]); //no-d2
  fprintf(stderr,"\nOpening pipe to %s\n\n",pipecmd);
  fd=popen(pipecmd,"w");
  if (!fd) {perror("Could not open gzip pipe."); exit(1);}

  for (k1=0;  (orgd0=test_pattern[k1])!=0xdeadbeef && skippy2<65536; k1++)
   {
    // update display extra spaces are so we can wipe previous text. note \r not \n
    fprintf(stderr,"%s d0=%08x,d1=%08x,d2=%08x test #%ld                 \r",text_opcodes[i],orgd0,orgd1,orgd2,testsdone);
    for (k2=0; (orgd1=test_pattern[k2])!=0xdeadbeef; k2++)
    {


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

         // report differences and flag with a * where it's different for easy viewing.
         // deltad0, deltad1, and deltaflags are written as text for easy searching of output.



        // diffd0=gend0^m68d0;
        // diffd1=gend1^m68d1;
        // diffd2=gend1^m68d2;
        // diffccrout=(genccrout^m68ccrout) & 31;
        // failed=( diffd0 || diffd1 || diffccrout);


          output.head     =0x00068040;
          output.separator=0x000c0ffe;
          output.end      =0xfff68040;
          output.testnum  =testsdone;
  
          output.input_d0 =orgd0;
          output.input_d1 =orgd1;
          output.input_d2 =orgd2;
          output.input_ccr=(uint32)ccrin;
          output.output_d0=m68d0;
          output.output_d1=m68d1;
          output.output_d2=m68d2;
          output.output_ccr=(uint32)m68ccrout;

	  fwrite(&output,48,1,fd);

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
      } // end of k1 loop
      if (pcount_opcodes[i]==1) break;  // if there's only one param for this opcode, don't loop on k1
 } // end of k0 loop

  if (fd!=NULL) pclose(fd);
  
  fprintf(stderr,"\n%ld errors of %ld tests done for %s (%02x%02x)                                  \n\n",numdiffs,testsdone,text_opcodes[i],opcode_to_test[0],opcode_to_test[1]); 
  
#endif
 } // end of opcode loop.

 fprintf(stderr,"                                                                \n\n"); 
 // add a newline to linefeed since update display above doesn't.
 return;



}
