/**************************************************************************************\
*                             Apple Lisa 2 Emulator                                    *
*                                                                                      *
*                  Copyright (C) 1998 Ray Arsen Arachelian                             *
*                            All Rights Reserved                                       *
*                                                                                       *
*                           Global Variables .c file                                   *
*                    This file doesn't contain any code, just                          *
*                     global variable definitions for other                            *
*                       code files.  This gets turned into                             *
*                      an include file and gets included.                              *
\**************************************************************************************/

#include <stdio.h>
///#include <sys/types.h>
///#include <types.h>



typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;


typedef signed char  int8;
typedef signed short int16;
typedef signed int   int32;

typedef signed char  sint8;
typedef signed short sint16;
typedef signed int   sint32;


/********************* Global Variables for entire emulator ***********************************************/

/*-----------------6/10/98 11:47AM---------------------------------------------
 * fcc001-fcc7ff floppy controller ram. fcc161-fcc17d protected area....
 *----------------------------------------------------------------------------
0000|                       ;               $FCC161     : Error code
0000|                       ;               $FCC163-165 : Contents of memory error address latch if parity error
0000|                       ;               $FCC167     : Memory board slot # if memory error
0000|                       ;               $FCC169-173 : Last value read from clock
0000|                       ;               $FCC175-17B : Reserved
0000|                       ;               $FCC17D-17F : Checksum
 *
0x00b0-00bf floppy_ram protected area

*/

int debug_log_enabled=0;

uint8 floppy_FDIR;
uint8 floppy_ram[2048];
uint8 floppy_6504_wait = 0;
uint8 floppy_irq_top=1, floppy_irq_bottom=1;  // interrupt settings (are floppies allowd to interrupt)


/// import the next line from vars.h
#define MAX_LISA_MFN     25      /* The last Lisa Memory function type we have */

// for mmu.c
uint32 mmudirty=0, mmudirty_all[5];
uint8 *(*mem68k_memptr[MAX_LISA_MFN]) (uint32 addr);
uint8  (*mem68k_fetch_byte[MAX_LISA_MFN]) (uint32 addr);
uint16 (*mem68k_fetch_word[MAX_LISA_MFN]) (uint32 addr);
uint32 (*mem68k_fetch_long[MAX_LISA_MFN]) (uint32 addr);
void   (*mem68k_store_byte[MAX_LISA_MFN]) (uint32 addr, uint8 data);
void   (*mem68k_store_word[MAX_LISA_MFN]) (uint32 addr, uint16 data);
void   (*mem68k_store_long[MAX_LISA_MFN]) (uint32 addr, uint32 data);

// Cache MC68000 Supervisor flag here to detect changes as they affect MMU context.
unsigned int lastsflag;

uint32 maxlisaram=2*1024*1024;       // Maximum Lisa RAM defaults to 2mb
uint32 minlisaram=0x4000;            // Minmum  Lisa RAM address.
uint8 *lisaram, lisarom[0x4000];     // pointer to Lisa RAM, space for the MC68000 ROM
uint32 segment1=0, segment2=0, context=0, lastcontext=0, address32, address, mmuseg, mmucontext, transaddress;
// special memory latches toggled by i/o space addresses in the lisa.
uint32 diag1, diag2, start=1, softmem, vertical, hardmem, videolatch=0x2f, lastvideolatch=0x2f,
videolatchaddress=(0x2f*32768), lastvideolatchaddress=0x2f*32768, statusregister,
videoramdirty=0,videoximgdirty=0;

uint16 memerror;

uint8 contrast=0xff; // 0xff=black 0x80=visible 0x00=all white
uint8 volume=4; // 0x0e is the mask for this.

long *dtc_rom_fseeks=NULL;
FILE *rom_source_file=NULL;



/****************** interrupt.c definitions/vars visible to outside world... ******************************/

int32 triggerevent;                 // code of the event that occured.
char *triggermessage;               // pointer to trigger pop ups, could point to trigger buffer
char triggerbuffer[256];            // scratch space for trigger pop ups
volatile uint32 IRQHIT;             // 0 if no IRQ, IRQ source otherwise
#define IRQFLAG IRQHIT              // synonym.

#define IRQFLOPPYCYCLES 10          // how many cycles to interrupt the 68000 after the floppy is done.


#define IRQ_FLOPPY     1
#define IRQ_FDIR_ON    401
#define IRQ_FDIR_OFF   400
#define IRQ_SCC        6
#define IRQ_VIA1       2
#define IRQ_VIA2       10          // this will map out is IRQ 1 to the lisa,  done to avoid conflict with floppy.
#define IRQ_MMU_SEG    9
#define IRQ_MMU_BUS    8
#define IRQ_SLOT0      5
#define IRQ_SLOT1      4
#define IRQ_SLOT2      3

/*
*   7   NMI - Highest Priority                                                         *
*   6   RS-232 Ports                                                                   *
*   5   Expansion Slot 0                                                               *
*   4   Expansion Slot 1                                                               *
*   3   Expansion Slot 2                                                               *
*   2   Keyboard                                                                       *
*   1   All other internal interrupts -- lowest priority                               *
*                                                                                      *
*   Reset: Initial SSP               $000000                                           *
*   Reset: Initial PC                $000004                                           *
*   Bus Error:                       $000008                                           *
*   Address Error:                   $00000C                                           *
*   Illegal Instruction:             $000010                                           *
*   Zero Divide:                     $000014                                           *
*   CHK Instruction:                 $000018                                           *
*   TRAPV Instruction:               $00001C                                           *
*   Privilidge Violation:            $000020                                           *
*   Trace:                           $000024                                           *
*   Unimplemented Instruction 1010   $000028                                           *
*   Unimplemented Instruction 1111   $00002C                                           *
*   Reserved/Unassigned:             $000030 - $00005F                                 *
*   Spurious Interrupt:              $000060                                           *
*   Other Internal Interrupt:        $000064                                           *
*   Keyboard Interrupt:              $000068                                           *
*   Slot 2 Autovector:               $00006C                                           *
*   Slot 1 Autovector:               $000070                                           *
*   Slot 0 Autovector:               $000074                                           *
*   RS-232 Interrupt                 $000078                                           *
*   Non-Maskable Interrupt:          $00007C                                           *
*   Trap Instruction Vectors:        $000080 - $000BF                                  *
*   Reserved, Unassigned:            $0000C0 - $000Cf                                  *
*   User Interrupt Vectors:          $000100 - $003FF                                  *

*/

// this must be a power of 2 under 65536 and huge!  This is the size of the Interrupt Queue Ring buffer.
// since the lisa can shut off interrupts,  we need this to be huge.
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MAXIRQQUEUE 2048

typedef struct
{
	uint16 cycles;  // how many cycles before this interrupt happens.
	uint16 irql;    // what interrupt is supposed to happen?
	uint32 address; // some irq's are memory faults, what's the address of the fault
} IRQRingBufferType;

IRQRingBufferType irqRB[MAXIRQQUEUE];
uint16 IRQIdx=0;
uint16 IRQSize=0;
/****************************************************************************************
   This macro is used by the 68000 emulator module to see if an interrupt has
   occured.  The reason we are using a ring buffer here is so that we can allow
   emulated devices to interrupt the CPU after a certain number of cycles.  For
   example, our 6504 floppy emulator actually reads/writes sectors in what the
   68000 would see as a single cycle.  But since the 68000 expects to be interrupted
   once the 6504 is done, this isn't very cool.  So this mechanism takes care of
   that.  The minimal number of cycles MUST be 1!  if you set it to zero, the CPU
   module will wait 65535 cycles!
*****************************************************************************************/
#define IRQCycleRingBuffer() {if (IRQSize) {if (!(irqRB[IRQIdx].cycles--)) IRQHIT |=1; }

void IRQRingBufferDelete(void);
int8 IRQRingBufferAdd(uint16 cycles, uint16 irql, uint32 address);



// hexidecimal conversion table table.
char *hex="0123456789abcdef";
//uint32 TWOMEGMLIM=0x001fffff;

// triggers are events that stop the 68k emulator and go to an event handler...

#define NMI_INTERRUPT 0x00000001
#define COPSOVERFLOW 10001
#define GENERICTRIGGER -1
#define IRQOVERFLOW 20002


/****************** COPS.C definitions/vars visible to outside world... ************************************/

int16  copsqueuelen=0;
#define MAXCOPSQUEUE 512
char    copsqueue[MAXCOPSQUEUE];
uint8   NMIKEY=0;
uint8   cops_powerset, cops_clocksetmode, cops_timermode;
uint8   mouse_pending=0, mouse_pending_x=0, mouse_pending_y=0;
int16   last_mouse_x=0,    last_mouse_y=0,    last_mouse_button=0;

typedef struct
      {
       int16 x;
       int16 y;
       int8  button;                    // 0=no click change, 1=down, -1=up.
      } mousequeue_t;

#define MAXMOUSEQUEUE 16
int16  mousequeuelen=0;
mousequeue_t mousequeue[MAXMOUSEQUEUE];



/****** Image Writer Stuff - Needs to be before ADPM in viatype ******/

//#include <imagewriter.h>

//ImageWriterType *IWConnected;


/********************* Profile code defines and protos ***************************************************/

#define PROFILE_IMG_HEADER_SIZE 2048

typedef struct
{
	int8   Command;                // what command is the profile doing:
                                    // -1=disabled, -2=idle, 1=read, 2=write, 3=write verify
                                    //
	uint8   StateMachineStep;       // what step of Command is the state machine of this profile in?

	uint8   DataBlock[4+8+532+8+2]; // 4 status bytes, command block, data block, 2 for good luck

	uint16  indexread;              // indeces into the buffer for the Lisa to Read or Write
	uint16  indexwrite;
	uint32  blocktowrite;           // used by the write command to store the block number to write

	uint32  numblocks;              // (24 bit value!) size of this profile in blocks.
                                    //   (9728=5mb, 532 bytes/sector)
                                    // We shouldn't make this more than 10Mb until we've tried it.

                                    // Control Lines from 6522 (except OCDLine which is Command=-1)
	uint8   CMDLine;                // set by Lisa
	uint8   BSYLine;                // set by ProFile
	uint8   DENLine;                // set by Lisa (drive enabled)
	uint8   RRWLine;                // set by Lisa (read or write)

	uint8   VIA_PA;                 // data to from VIA PortA (copied to from V->via[0])

	FILE  *ProFileHandle;          // file handle to disk image
	char  ProFileFileName[256];     // the file name for this Profile disk image to open;

}  ProFileType;


/********************* VIA types, defines, and protos ****************************************************/


/*********************************************************************************************************/
const char reversebit[]={
	0x0, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x8, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x4, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0xc, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x2, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0xa, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x6, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0xe, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x1, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x9, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x5, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0xd, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x3, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0xb, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x7, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0xf, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

/*********************************************************************************************************/
extern uint16 floppy_sony_table[2][80][13];
extern uint16 floppy_twiggy_table[2][46][24];
uint8 floppy_fdir;

/*********************************************************************************************************/


/****** Video ***********/
uint8 bitdepth;


uint8 softmemerror=0, harderror=0, videoirq=0, bustimeout=0, videobit=0;
uint8 serialnumshiftcount=0, serialnumshift=0;
uint8 serialnum[8], serialnum240[32];

/****** Sound **********/

#define MAXSOUNDS 10
int  SoundLastOne=5;
// don't need this // char *SoundFile[10]={"twiggy.au","iw.out","beep.au","beep2.au","beep3.au",NULL,NULL,NULL,NULL,NULL};
char *SoundBuffer[10];
uint32 SoundBufSize[10];

// Serial port connection

#define SCC_NOTHING         0
#define SCC_LOCALPORT       1
#define SCC_IMAGEWRITER_PS  2
#define SCC_IMAGEWRITER_PCL 3
#define SCC_FILE            4  // same as localport???
#define SCC_PIPE            5


char *SCC_A_port, SCC_A_OPTIONS[1024];
char *SCC_B_port, SCC_B_OPTIONS[1024];

uint8 SERIAL_A;
uint8 SERIAL_B;

FILE *SCC_A_PORT_F;
FILE *SCC_B_PORT_F;

/// main program parameters
struct
{
	int argc;
	char **argv;

} XLisaEm_Args;


// USed by memory diag tests
uint8 *mem_parity_bits1=NULL;
uint8 *mem_parity_bits2=NULL;
uint32 last_bad_parity_adr=0;


int scc_running=0;

/****************** Memory/MMU protos, defines, and vars ***********************************/
/* these are a bit too conservative perhaps, but they will prevent address overflows. */
#define RAM_MMU_TRANS (lisaram+(((addr & 0x00ffffff)+mmu_trans[(addr & 0x00ffffff)>>9].address) & 0x001fffff))
#define RFN_MMU_TRANS (mmu_trans[(addr & 0x00ffffff)>>9].readfn)
#define WFN_MMU_TRANS (mmu_trans[(addr & 0x00ffffff)>>9].writefn)

/* These macro "functions" are used by generator.  I use the above macro's in here just to avoid
   macro soup insanity.  Yeah, so they're probably not super duper optimized, but there's time
   for that once the emulator is debugged and working. -- RA */
#define fetchaddr(addr) mem68k_memptr[RFN_MMU_TRANS](addr)
#define fetchbyte(addr) mem68k_fetch_byte[RFN_MMU_TRANS](addr)
#define fetchword(addr) mem68k_fetch_word[RFN_MMU_TRANS](addr)
#define fetchlong(addr) mem68k_fetch_long[RFN_MMU_TRANS](addr)
#define storebyte(addr,data) mem68k_store_byte[WFN_MMU_TRANS ](addr,data)
#define storeword(addr,data) mem68k_store_word[WFN_MMU_TRANS ](addr,data)
#define storelong(addr,data) mem68k_store_long[WFN_MMU_TRANS ](addr,data)



// Lisa I/O Space types and other types of accesses.

// These used to be enums, but enums in gcc are 32 bits long, and unless you want to pass
// the -fshort-enums which may break other things, I'd rather just set them up as #DEFINEs and
// be done with it.  Could have left them as enums, but that would bloat the mmu structures I use
// as mmu_trans_all would grow huge.  consts can't be used to set arrays, so #DEFINES they'll have
// to be.


// Note, these are the letter Ox not zero x, so they're perfectly legal symbol names
// It makes for easier reading of addresses when reading the source. :)

// Regular (non-special) I/O address space.  Subject to MMU mapping.

#define OxERROR           0     /* This should never be used - it indicates a bug in our code */

#define OxUnused	 	  1     /* unused I/O space address (only used in I/O space map) */
#define Ox0000_slot1l	  2
#define Ox2000_slot1h	  3
#define Ox4000_slot2l	  4
#define Ox6000_slot2h	  5
#define Ox8000_slot3l	  6
#define Oxa000_slot3h	  7
#define Oxc000_flopmem	  8
#define Oxd000_contrast	  1     /* Same as unused - was tricked by a define in the rom listings. */
#define Oxd200_sccz8530	 10
#define Oxd800_par_via2	 11
#define Oxdc00_cops_via1 12
#define Oxe000_latches	 13
#define Oxe800_videlatch 14
#define Oxf000_memerror  15
#define Oxf800_statreg	 16

// Real Lisa memory
#define ram		         17     /* Plain old RAM, or stack access.                                   */
#define vidram           18     /* same as ram, but flag on write that screen needs refreshing       */
#define ro_violn         19     /* Read only violation - what trap should I call? See schematic      */
#define bad_page         20     /* Bad page or unallocated segment - what trap here?                 */

// Special I/O space
#define sio_rom          21     /* access to ROM via sio mode                                        */
#define sio_mrg          22     /* mmu register being accessed.  Which depends on bit 3 of addr      */

#define sio_mmu          23     /* access ram or other spaces via the mmu (bit14=1 in address)       */
#define io               24      /* This is a dispatcher for I/O space when we don't know the address */

#define MAX_LISA_MFN     25      /* The last Lisa Memory function type we have */


char *memspaces[]=
{
	"00-OxERROR",
	"01-OxUnused",
	"02-Ox0000_slot1l",
	"03-Ox2000_slot1h",
	"04-Ox4000_slot2l",
	"05-Ox6000_slot2h",
	"06-Ox8000_slot3l",
	"07-Oxa000_slot3h",
	"08-Oxc000_flopmem",
	"unused",
	"10-Oxd200_sccz8530",
	"11-Oxd800_par_via2",
	"12-Oxdc00_cops_via1",
	"13-Oxe000_latches",
	"14-Oxe800_videlatch",
	"15-Oxf000_memerror",
	"16-Oxf800_statreg",
	"17-ram",
	"18-vidram",
	"19-ro_violn",
	"20-bad_page",
	"21-sio_rom",
	"22-sio_mrg",
	"23-sio_mmu",
	"24-io"
};


// Disparcher to I/O space (dispatcher to the Ox????_ fn's list above)

typedef uint8 lisa_mem_t;         /* type indicator for the above #defines for use with fn's           */


// I/O Address Maps.  These are used to initialize the fn pointers on the OUTPUT side of the mmu.

lisa_mem_t io_map[]=
{
/*                             000                 200                 400                  600 */
/* 0xfc0000 : */   Ox0000_slot1l,    Ox0000_slot1l,    Ox0000_slot1l,    Ox0000_slot1l,
/* 0xfc0800 : */   Ox0000_slot1l,    Ox0000_slot1l,    Ox0000_slot1l,    Ox0000_slot1l,
/* 0xfc1000 : */   Ox0000_slot1l,    Ox0000_slot1l,    Ox0000_slot1l,    Ox0000_slot1l,
/* 0xfc1800 : */   Ox0000_slot1l,    Ox0000_slot1l,    Ox0000_slot1l,    Ox0000_slot1l,
/* 0xfc2000 : */   Ox2000_slot1h,    Ox2000_slot1h,    Ox2000_slot1h,    Ox2000_slot1h,
/* 0xfc2800 : */   Ox2000_slot1h,    Ox2000_slot1h,    Ox2000_slot1h,    Ox2000_slot1h,
/* 0xfc3000 : */   Ox2000_slot1h,    Ox2000_slot1h,    Ox2000_slot1h,    Ox2000_slot1h,
/* 0xfc3800 : */   Ox2000_slot1h,    Ox2000_slot1h,    Ox2000_slot1h,    Ox2000_slot1h,
/* 0xfc4000 : */   Ox4000_slot2l,    Ox4000_slot2l,    Ox4000_slot2l,    Ox4000_slot2l,
/* 0xfc4800 : */   Ox4000_slot2l,    Ox4000_slot2l,    Ox4000_slot2l,    Ox4000_slot2l,
/* 0xfc5000 : */   Ox4000_slot2l,    Ox4000_slot2l,    Ox4000_slot2l,    Ox4000_slot2l,
/* 0xfc5800 : */   Ox4000_slot2l,    Ox4000_slot2l,    Ox4000_slot2l,    Ox4000_slot2l,
/* 0xfc6000 : */   Ox6000_slot2h,    Ox6000_slot2h,    Ox6000_slot2h,    Ox6000_slot2h,
/* 0xfc6800 : */   Ox6000_slot2h,    Ox6000_slot2h,    Ox6000_slot2h,    Ox6000_slot2h,
/* 0xfc7000 : */   Ox6000_slot2h,    Ox6000_slot2h,    Ox6000_slot2h,    Ox6000_slot2h,
/* 0xfc7800 : */   Ox6000_slot2h,    Ox6000_slot2h,    Ox6000_slot2h,    Ox6000_slot2h,
/* 0xfc8000 : */   Ox8000_slot3l,    Ox8000_slot3l,    Ox8000_slot3l,    Ox8000_slot3l,
/* 0xfc8800 : */   Ox8000_slot3l,    Ox8000_slot3l,    Ox8000_slot3l,    Ox8000_slot3l,
/* 0xfc9000 : */   Ox8000_slot3l,    Ox8000_slot3l,    Ox8000_slot3l,    Ox8000_slot3l,
/* 0xfc9800 : */   Ox8000_slot3l,    Ox8000_slot3l,    Ox8000_slot3l,    Ox8000_slot3l,
/* 0xfca000 : */   Oxa000_slot3h,    Oxa000_slot3h,    Oxa000_slot3h,    Oxa000_slot3h,
/* 0xfca800 : */   Oxa000_slot3h,    Oxa000_slot3h,    Oxa000_slot3h,    Oxa000_slot3h,
/* 0xfcb000 : */   Oxa000_slot3h,    Oxa000_slot3h,    Oxa000_slot3h,    Oxa000_slot3h,
/* 0xfcb800 : */   Oxa000_slot3h,    Oxa000_slot3h,    Oxa000_slot3h,    Oxa000_slot3h,
/* 0xfcc000 : */   Oxc000_flopmem,   Oxc000_flopmem,   Oxc000_flopmem,   Oxc000_flopmem,
/* 0xfcc800 : */   Oxc000_flopmem,   Oxc000_flopmem,   Oxc000_flopmem,   Oxc000_flopmem,
/* 0xfcd000 : */   Oxd000_contrast,  Oxd200_sccz8530,  Oxd200_sccz8530,  Oxd200_sccz8530,
/* 0xfcd800 : */   Oxd800_par_via2,  Oxd800_par_via2,  Oxdc00_cops_via1, Oxdc00_cops_via1,
/* 0xfce000 : */   Oxe000_latches,   OxUnused,         OxUnused,         OxUnused,
/* 0xfce800 : */   Oxe800_videlatch, Oxe800_videlatch, Oxe800_videlatch, Oxe800_videlatch,
/* 0xfcf000 : */   Oxf000_memerror,  Oxf000_memerror,  Oxf000_memerror,  Oxf000_memerror,
/* 0xfcf800 : */   Oxf800_statreg,   Oxf800_statreg,   Oxf800_statreg,   Oxf800_statreg
};



lisa_mem_t sio_map[]= // danger! this is for use on pre-MMU addresses - lop off the top 17 bits of the address
                      // as well as the low 9 bits ( address & 01fe00)
{
/*               000       200       400       600       800       a00        c00       e00 */
/*000000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, // 8/line
/*001000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
/*002000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
/*003000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
/*004000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*005000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*006000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*007000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*008000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
/*009000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
/*00a000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
/*00b000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
/*00c000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*00d000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*00e000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*00f000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*010000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
/*011000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
/*012000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
/*013000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
/*014000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*015000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*016000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*017000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*018000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
/*019000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
/*01a000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
/*01b000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
/*01c000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*01d000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*01e000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
/*01f000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu
};


/* MMU Translation type.  This is unsed in an array to do a quick address translation
   and to hold an index to the function to call depending on whether access is a read
   or a write.  There are 3 subtypes for byte,word,long, so a total of 6fn's of each
   of the types in the #defines are needed to make this work. (rb,rw,rl,wb,ww,wl)
   These are one per page (512 bytes!) after the MMU translation (segment selection)
*/
typedef struct _t_ipc {
	void (*function)(struct _t_ipc *ipc);
	uint8 used;               /* bitmap of XNZVC flags inspected */
	uint8 set;                /* bitmap of XNZVC flags altered */     //20/24 bytes
	uint16 opcode;  		// absolutely necessary

	uint16 wordlen; 		// might be able to delete this if we also add nextpc, but diss68k will need work as will
							// lots of mods of cpu68.k  To get it call iib->wordlen  and the illegal instruction in
							// there needs checking.

							// might have to add the PC for this current IPC, though not likely needed or useful.
	//unsigned int :0;  		// what's this here for?  Can it be deleted?
	uint32 src;
	uint32 dst;
	struct _t_ipc *next;			// next ipc in chain. we could get rid of this, but then things would run slower
							// and we'd need to do a whole lot of more work to get things to work.
} t_ipc;


typedef struct _t_ipc_table
{
	// Pointers to all the IPC's in this page.  Since the min 68k opcode is 2 bytes in size
	// the most you can have are 256 instructions per page.  We thus no longer need a hash table
	// nor any linked list of IPC's as this is a direct pointer to the IPC.  Ain't life grand?
	t_ipc ipc[256];

	// These are merged together so that on machines with 32 bit architectures we can
	// save four bytes.  It will still save 2 bytes on 64 bit machines.
	union t
	{ 	uint32 clocks;
		struct _t_ipc_table *next;
	} t;


#ifdef PROCESSOR_ARM
	void (*compiled)(struct _t_ipc *ipc);
	//uint8 norepeat;	// what's this do? this only gets written to, but not read.  Maybe Arm needs it?
#endif
} t_ipc_table;


// How many percent of the current IPCT's should we allocate
#define IPCT_ALLOC_PERCENT 20

// What's the maximum time's we'll call malloc to get ipc tables?
#define MAX_IPCT_MALLOCS 8

// we should never need to even do more than the initial malloc if initial_ipcts=4128.
t_ipc_table *ipct_mallocs[MAX_IPCT_MALLOCS];
// size of each of the above mallocs - not really needed, but could be useful for debugging.
uint32 sipct_mallocs[MAX_IPCT_MALLOCS];
uint32 iipct_mallocs;
uint32 ipcts_allocated;
uint32 ipcts_used;
uint32 ipcts_free;


/* (2MB RAM max+ 16KROM)=2113536 bytes of potentially executable code divided by 512(bytes/mmu page) = 4128
*  ipc's page = maximum= 8256 ipct's is maximum - should not have to go above this ever.
*  this value is only for testing, until we test the LisaEM under heavy load to find the actual used number of ipct's
*  and lessen it to free things up.  remember, each ipct holds 256 ipcs plus extra info.  */

uint32 initial_ipcts=4128;

t_ipc_table  *ipct_free_head, *ipct_free_tail;




typedef struct _mmu_trans_t
{	int32 address;	     /* quick ea translation. Just add to lower bits  - needs to be signed 	*/
    //uint32 sor9;
	lisa_mem_t readfn;   /* index to read and write fn's for that segment, that way I			*/
	lisa_mem_t writefn;  /* can have read only segments without doing special checking.		*/
	t_ipc_table *table;  /* Pointer to a table of IPC's or NULL if one hasn't been assigned.	*/
} mmu_trans_t;




typedef struct
{
	uint16 sor, slr;        	  // real sor, slr
    //uint16 newsor, newslr;    // used when updating - won't change to this until both are written to.
	uint8  changed;   		  // this is a flag to let us know that an mmu segment has changed.
                              // come back later to correct it. bit 0=newslr set, bit 1=newsor set,  2=newsor, 3=newslr+newsor
} mmu_t;

//  5 sets of mmu registers, each of 128 segments.
//  (4 are real lisa mmu contexts, added an extra for easier START mode translation)
mmu_t mmu_all[5][128];

// *mmu is in current context. i.e. mmu = mmu_all[4] sets context[4] (START mode).  I do this so that I

// don't have to dereference the array every single memory access as that would add an unnecessary expense.
// i.e. mmu_all[SEGMENT1+SEGMENT2|SETUP][(address>>17)&0x7f] is very expensive, but
// mmu[(address>>17)&0x7f] is far less expensive.
mmu_t *mmu;

mmu_trans_t mmu_trans_all[5][32768];
/* sadly if I used enums for lisa_mem_t.readfn/.writefn gcc would allocate 4 bytes for each so the total
   or this would be 12 bytes x 32768 pages/context x 5 = 1.85MB of ram for this table.  Instead I used
   defines and uint8's for lisa_mem_t, so this should be 1.25M depending on how they're packed    */

mmu_trans_t *mmu_trans=mmu_trans_all[0]; // ptr to current context, so I don't have to do expensive double array deref.
// i.e. mmu_trans=mmu_trans_all[0] is context 0.

// Refs for Generator's mem68k.c
// These are initialized by init_lisa_mmu.

// temp vars to play with, don't include these in vars.c
mmu_t m; // temp variable to play with
mmu_trans_t mt; // mmu translation - temporary var
mmu_trans_t *lastvideo_mt;

// Stuff for Generator

//unsigned long cpu68k_frames;  -- already in cpu68k.c!
//unsigned long cpu68k_frozen;

