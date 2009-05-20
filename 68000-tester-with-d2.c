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



// behavior of test results.
// #define SKIP_CCR_TESTS 1
// #define SKIP_PASSED_TESTS 1
// #define LIMIT_FAILURES 1



#define IN_LISAEM_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>

#include <signal.h>

#include <generator.h>
#include <cpu68k.h>
#include <mem68k.h>

#define UHAVELISAEMTYPESH

// #include <vars.h> already got it from cpu68k.h

#include <registers.h>
#include <reg68k.h>
#include "../cpu68k/def68k-proto.h"


//typedef unsigned char  uint8;
//typedef unsigned short uint16;
//typedef unsigned int   uint32;


//typedef signed char  sint8;
//typedef signed short sint16;
//typedef signed int   sint32;


//typedef signed char  int8;
//typedef signed short int16;
//typedef signed int   int32;

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

void my_cpu68k_ipc(uint32 addr68k, uint8 *opcodebuffer, t_iib * iib, t_ipc * ipc);
uint8 opcode_to_test[4096];             // execution space for generator.

/* Strip white space at the end of a string.  No need to size
   check the string since strlen will return its length. */


// bullshit fn's to satisfy compilation - not needed here, but needed by supporting fn's.
void mmuflush(void) {return;}
void dump_scc(void) {return;}
char *printslr(char *x, long size, uint16 slr) {return NULL;}
char *mspace(lisa_mem_t fn) {return NULL;}
void checkcontext(uint8 c, char *text) {;}
lisa_mem_t rmmuslr2fn(uint16 slr, uint32 a9) {return 0;}
void get_slr_page_range(int cx,int seg, int16 *pagestart, int16 *pageend, lisa_mem_t *rfn, lisa_mem_t *wfn)
{;}

// memory debug functions - not needed here.
void  *dmem68k_memptr(    char *file, char *function, int line,uint32 a) {return NULL;}
uint8  dmem68k_fetch_byte(char *file, char *function, int line,uint32 a ) {return 0;}
uint16 dmem68k_fetch_word(char *file, char *function, int line,uint32 a ) {return 0;}
uint32 dmem68k_fetch_long(char *file, char *function, int line,uint32 a ) {return 0;}
void   dmem68k_store_byte(char *file, char *function, int line,uint32 a, uint8  d) {;}
void   dmem68k_store_word(char *file, char *function, int line,uint32 a, uint16 d) {;}
void   dmem68k_store_long(char *file, char *function, int line,uint32 a, uint32 d) {;}



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


// stolen from generator
RETSIGTYPE gen_sighandler(int signum)
{
	signal(signum, gen_sighandler); // reinstate signal handler to prevent race condition.

    dumpram();

    if (gen_debugmode) {
		if (signum == SIGINT) {
			if (gen_quit)
			{
				ui_log_critical(("Bye!"));
				//uninit_gui();
			}
			else
            {
				ui_log_request("Ping - current PC = 0x%X",regs.pc);
			}
			exit(1);
		}
	}
	else
	{
		//uninit_gui();
		exit(0);
	}

}


// stolen from reg68k.c of course. :)
void      execgen_opcode(uint16 *ccrin, uint32 *reg1, uint32 *reg2, uint32 *reg3, uint16 *ccrout)
//void execgen_opcode(uint16 *ccrin, uint32 *reg1, uint32 *reg2, uint16 *ccrout)
{
        //uint16 myopcode; 
        static t_ipc ipc;
        static t_iib *piib;
        jmp_buf jb;

          if (!setjmp(jb)) 
        {
                // setup generator registers for testing 
                abort_opcode=0;
                regs.regs[0]=*reg1; 
                regs.regs[1]=*reg2;
                regs.regs[1]=*reg3;
                regs.pc=1024;
                regs.sr.sr_int=(*ccrin & 0x00ff);


     //          printf("Pointers: ipc:%p  %p reg1, %p reg2, %p reg68k_regs:: %p %p\n",&ipc,reg1,reg2,reg68k_regs, &regs.regs[0],&regs.regs[1]);

                /* move PC and register block into global processor register variables */
		reg68k_regs = regs.regs;
                reg68k_pc =   regs.pc;
                reg68k_sr =   regs.sr;
               
                myopcode=((opcode_to_test[0]<<8)|opcode_to_test[1]); 

                if (!(piib = cpu68k_iibtable[myopcode]))
                    DEBUG_LOG(0,"Invalid instruction @ %08X\n", reg68k_pc); // RA

                my_cpu68k_ipc(reg68k_pc, opcode_to_test, piib, &ipc);


		//fprintf(stderr,"in d0,d1: :%08x,%08x\n",reg68k_regs[0],reg68k_regs[1]);

                cpu68k_functable[myopcode*2 + 1] (&ipc);
		//fprintf(stderr,"%04x d0,d1: :%08x,%08x  ,  %08x,%08x            \n",myopcode,regs.regs[0],regs.regs[1],reg68k_regs[0],reg68k_regs[1]);

                // get return values;
                regs.pc = reg68k_pc;
                regs.sr = reg68k_sr;
                *ccrout = reg68k_sr.sr_int;
                *reg1=reg68k_regs[0]; 
                *reg2=reg68k_regs[1]; 
//		fprintf(stderr,"out %04x d0,d1: :%08x,%08x  ,  %08x,%08x            \n",myopcode,*reg1,*reg2,reg68k_regs[0],reg68k_regs[1]);

                  longjmp(jb, 1);
        }
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
extern uint32 *getfnptr(void);
extern uint32 getfnsize(void);
extern uint32 getopcodeoffset(void);
extern uint32 getopcodesize(void);
extern void init_opcodes(void);


extern uint16  asmtest(uint16 *ccrin, uint32 *reg1, uint32 *reg2, uint32 *reg3, uint16 *ccrout);

// our allocated function to the copy of asmtest.
uint16 (*exec68k_opcode)(uint16 *ccrin, uint32 *reg1, uint32 *reg2, uint32 *reg3, uint16 *ccrout);
void      execgen_opcode(uint16 *ccrin, uint32 *reg1, uint32 *reg2, uint32 *reg3, uint16 *ccrout);


uint32 *fnstart, codesize, nopoffset, nopsize;

int create_asm_fn(uint8 *newopcode,size_t size)
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
    if (size) for (i=0; i<size; i++) memory[i+nopoffset]=newopcode[i];
    // if size==0, that means I'm just testing the copy mechanism. :)

    // Free the old fn if we had one, then assign fn pointer to newly allocated memory.
    // this "malloc first, then free" method avoids cache coherency issues.


    // memory leak to prevent over malloc/free'd.

    if (exec68k_opcode!=NULL) free(exec68k_opcode);
    (void *)exec68k_opcode=(void *)memory;        

    return 0;
}

// get rid of the real one that will be used by the emulator.
#undef RAM_MMU_TRANS

// New one - faking start PC to 1024 to avoid low ram vectors.
#define RAM_MMU_TRANS(x) (&opcode_to_test[(x-1024)])


uint8  lisa_rb_ram(uint32 addr)
{
    return (*(uint8 *)(RAM_MMU_TRANS(addr)));
}

uint16 lisa_rw_ram(uint32 addr)
{
   return LOCENDIAN16(*(uint16 *)(RAM_MMU_TRANS(addr)));
}

uint32 lisa_rl_ram(uint32 addr)
{

#ifdef ALIGNLONGS
   {uint32 addr2=addr+2; // this is here to prevent warning about needing parens around & when +/- used from gcc
    return (LOCENDIAN16(*(uint16 *)(RAM_MMU_TRANS(addr  )) << 16) |
            LOCENDIAN16(*(uint16 *)(RAM_MMU_TRANS(addr2))));
   }
#else
   return  LOCENDIAN32(*(uint32 *)(RAM_MMU_TRANS(addr  )));
#endif
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

 cpu68k_init();				 // initialize generator core.
 //init_ipct_allocator();
 cpu68k_reset();
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
 uint32 long testsdone=0;

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


  fprintf(stderr,"Creating ASM function %s (%d) bytes in length\n",text_opcodes[i],j); fflush(stderr);
  status=create_asm_fn(opcode_to_test,j);
  if (status) {printf("Couldn't create asm function because:%d\n",status); exit(1);}

  fprintf(stderr,"Excercising opcode %s %d                    \n\n",text_opcodes[i],i); fflush(stderr);


  skippy=0; skippy2=0; numdiffs=0;
  for (k0=0;  (orgd2=test_pattern[k0])!=0xdeadbeef && skippy2<65536; k0++)  // only used if you have 3 operands! comment out otherwise.
  {

  if (fd!=NULL) pclose(fd);

  sprintf(pipecmd,"/usr/bin/gzip -1 >/mnt/next/m68040-opcode-%s.d2=%08x.bin.gz",text_opcodes[i],orgd2);
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
  
 } // end of opcode loop.

 fprintf(stderr,"                                                                \n\n"); 
 // add a newline to linefeed since update display above doesn't.
 return;
}



void my_cpu68k_ipc(uint32 addr68k, uint8 *opcodebuffer, t_iib * iib, t_ipc * ipc)
{
    t_type type;
    uint8  *addr;
    uint32 *p, a9,adr;

    
    addr=opcodebuffer;
    addr68k &= 0x00ffffff;

    if ( !ipc)
    {
      fprintf(stderr,"I was passed a NULL ipc.\nLet the bodies hit the floor... 1, Nothin' wrong with me. 2, Nothing wrong with me, 3. Nothing wrong with me. 4. Nothing wrong with me."); exit(123);
    }

    if ( !iib)
    {
      fprintf(stderr,"I was passed a NULL iib.\nLet the bodies hit the floor... 1, Nothin' wrong with me. 2, Nothing wrong with me, 3. Nothing wrong with me. 4. Nothing wrong with me."); exit(123);
    }

  ipc->opcode = (opcodebuffer[0]<<8)|(opcodebuffer[1]);
  ipc->wordlen = 1;
  if (!iib) {
    /* illegal instruction, no further details (wordlen must be set to 1) */
    return;
  }

  ipc->used = iib->flags.used;
  ipc->set = iib->flags.set;


  if ((iib->mnemonic == i_Bcc) || (iib->mnemonic == i_BSR)) {
    /* special case - we can calculate the offset now */
    /* low 8 bits of current instruction are addr+1 */
    //ipc->src = (sint32)(*(sint8 *)(addr + 1));
    ipc->src = (sint32)((sint8)(lisa_rb_ram(addr68k+1)));
    DEBUG_LOG(205,"i_Bcc @ %08x target:%08x opcode:%04x",addr68k,ipc->src,ipc->opcode);

    if (ipc->src == 0) {
      ipc->src = (sint32)(sint16)(lisa_rw_ram(addr68k + 2));
      DEBUG_LOG(205,"i_Bcc2 @ %08x target:%08x opcode:%04x",addr68k,ipc->src,ipc->opcode);
      ipc->wordlen++;
    }

    ipc->src += addr68k + 2;    /* add PC of next instruction */
    DEBUG_LOG(205,"i_Bcc2 @ %08x target:%08x opcode:%04x",addr68k,ipc->src,ipc->opcode);
    return;
  }
  if (iib->mnemonic == i_DBcc || iib->mnemonic == i_DBRA) {
    /* special case - we can calculate the offset now */

    ipc->src = (sint32)(sint16)lisa_rw_ram(addr68k + 2);
    ipc->src += addr68k + 2;    /* add PC of next instruction */
    DEBUG_LOG(205,"i_DBcc/DBRA @ %08x target:%08x opcode:%04x",addr68k,ipc->src,ipc->opcode);
    ipc->wordlen++;
    return;
  }

  addr += 2;
  addr68k += 2;

  //check_iib();

  for (type = 0; type < 2; type++) {
    if (type == tp_src)
      p = &(ipc->src);
    else
      p = &(ipc->dst);

    switch (type == tp_src ? iib->stype : iib->dtype) {
    case dt_Adis:

      *p = (sint32)(sint16)(lisa_rw_ram(addr68k));

      ipc->wordlen++;
      DEBUG_LOG(205,"dt_Adis @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
      addr += 2;
      addr68k += 2;
      break;
    case dt_Aidx:
      *p = (sint32)(sint8)lisa_rb_ram(addr68k+1);
      *p = (*p & 0xFFFFFF) | (lisa_rb_ram(addr68k) << 24);
      DEBUG_LOG(205,"dt_Adix @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
      ipc->wordlen++;
      addr += 2;
      addr68k += 2;
      break;
    case dt_AbsW:
      *p = (sint32)(sint16)lisa_rw_ram(addr68k);



      ipc->wordlen++;
      DEBUG_LOG(205,"dt_AbsW @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
      addr += 2;
      addr68k += 2;
      break;
    case dt_AbsL:
      //*p = (uint32)((LOCENDIAN16(*(uint16 *)addr) << 16) +
      //              LOCENDIAN16(*(uint16 *)(addr + 2)));
      *p=lisa_rl_ram(addr68k);
      DEBUG_LOG(205,"dt_AbsL @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
      ipc->wordlen += 2;
      addr += 4;
      addr68k += 4;
      break;
    case dt_Pdis:
      *p = (sint32)(sint16)lisa_rw_ram(addr68k);
      *p += addr68k;            /* add PC of extension word (this word) */
      DEBUG_LOG(205,"dt_Pdis @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
      ipc->wordlen++;
      addr += 2;
      addr68k += 2;
      break;
    case dt_Pidx:

      *p = ((sint32)(sint8)(lisa_rb_ram(addr68k+1))  + addr68k);
      *p = (*p & 0x00FFFFFF) | (lisa_rb_ram(addr68k)) << 24;  // <<--- is this correct???



      DEBUG_LOG(205,"dt_Pidx @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
      ipc->wordlen++;
      addr += 2;
      addr68k += 2;
      break;
    case dt_ImmB:
      /* low 8 bits of next 16 bit word is addr+1 */
      *p = (uint32)lisa_rb_ram(addr68k + 1);

      DEBUG_LOG(205,"dt_ImmB @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
      ipc->wordlen++;
      addr += 2;
      addr68k += 2;
      break;
    case dt_ImmW:
      *p = (uint32)lisa_rw_ram(addr68k);
      DEBUG_LOG(205,"dt_ImmW @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
      ipc->wordlen++;
      addr += 2;
      addr68k += 2;
      break;
    case dt_ImmL:
      *p = (uint32)lisa_rl_ram(addr68k); // ((LOCENDIAN16(*(uint16 *)addr) << 16) +  LOCENDIAN16(*(uint16 *)(addr + 2)));


      DEBUG_LOG(205,"dt_ImmL @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
      ipc->wordlen += 2;
      addr += 4;
      addr68k += 4;
      break;
    case dt_Imm3:
      if (type == tp_src)
         {
             *p = (ipc->opcode >> iib->sbitpos) & 7;
             DEBUG_LOG(205,"dt_Imm3 src @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
         }
      else
         {
             *p = (ipc->opcode >> iib->dbitpos) & 7;
             DEBUG_LOG(205,"dt_Imm3 dst @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
         }
      break;
    case dt_Imm4:
      if (type == tp_src)
         {
            *p = (ipc->opcode >> iib->sbitpos) & 15;
            DEBUG_LOG(205,"dt_Imm4 src @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
         }
      else
        {
            *p = (ipc->opcode >> iib->dbitpos) & 15;
            DEBUG_LOG(205,"dt_Imm4 dst @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
        }
      break;
    case dt_Imm8:
      if (type == tp_src)
        {
            *p = (ipc->opcode >> iib->sbitpos) & 255;
            DEBUG_LOG(205,"dt_Imm8 src @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
        }
      else
        {
            *p = (ipc->opcode >> iib->dbitpos) & 255;
            DEBUG_LOG(205,"dt_Imm8 dst @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
        }
      break;
    case dt_Imm8s:
      if (type == tp_src)
        {
            *p = (sint32)(sint8)((ipc->opcode >> iib->sbitpos) & 255);
            DEBUG_LOG(200,"dt_Imm8s src @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
        }
      else
        {
            *p = (sint32)(sint8)((ipc->opcode >> iib->dbitpos) & 255);
            DEBUG_LOG(200,"dt_Imm8s dst @ %08x target:%08x opcode:%04x",addr68k,*p,ipc->opcode);
        }
      break;
    default:
      break;
    }
  }
  /******************* FUN ENDS HERE ***********************************/
}
