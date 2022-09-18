/*
 *	@(#)entrypt.h	4.3	(ULTRIX)	10/9/90
 *
 */

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/*
 * entrypt.h -- misc. defines of interest to standalones and kernels
 *
 * Modification History: entrypt.h
 *
 * 01-Jun-89 -- Kong
 *	Added a field in the "save_state" struct to store the value
 *	of "cpu".  This is to allow the ULTRIX installation program
 *	"finder" to determine the type of machine it is running on
 *	without doing a name list on the kernel.  
 *
 * 04-May-89 -- Kong
 *	Added symbol PROM_HALT 
 *
 * 27-Apr-89 -- afd
 *	Change location of save_state area (consistent with 3.1)
 *	Change location of temporary Ultrix startup stack to allow for
 *	console space expansion.
 *
 * 17-Apr-89 -- afd
 *	Created "save_state" struct and defines for its magic number and
 *	its address at the bottom of the ULTRIX startup stack.
 *	This struct contains a magic number and the address of "doadump".
 *	This allows people to force a memory dump from console mode when
 *	a system is hung.
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif


/*
 * memory map assumed by prom and standalone system
 *
 * physical	kseg1			use
 *
 * 0x1fc20000	0xbfc20000
 * to					prom text and read-only data
 * 0x1fc00000	0xbfc00000		(in cpu board "prom space")
 *
 * (Top of RAM - 8K) downward		sash and standalone program stack
 *		|			( - 8K to preserve kernel message bufs)
 *		V			(standalone programs grow their stack
 *					 immediately below sash's stack)
 *
 *		^
 *		|
 * 0x00100000	0xa0100000 upward	sash program text, data, and bss
 *
 *		^
 *		|
 * 0x00030000	0xa0030000 upward	standalone program text, data, and bss
 *					(kernel is loaded here, also)
 *					Nothing "magic" about this location;
 *					  this address can be moved up if
 *					  future consoles need more space.
 * 0x0002ffff	0xa002ffff downward
 *		|
 *		V
 *
 *		^
 *		|
 * 0x00020000	0xa0020000 upward	Additional PROM space (64K)
 * 0x0001ffff	0xa001ffff
 *		^
 *		|			1K
 * 0x0001fc00	0xa001fc00 upward	Netblock (host/client info for net boot)
 *		^
 *		|			1K
 * 0x0001f800	0xa001f800 upward	Ultrix Save STate area (addr of doadump)
 *
 * 0x0001f7ff	0xa001f7ff downward	1K Ultrix temporary startup stack
 *		|
 *		V
 * 0x0001f400	0xa001f400
 * 0x0001f3ff	0xa001f3ff downward	dbgmon stack
 *		|
 *		V
 *
 *		^
 *		|
 * 0x00010000	0xa0010000 upward	dbgmon text, data, and bss
 *
 * 0x0000ffff	0xa000ffff downward	prom monitor stack
 *		|
 *		V
 *
 *		^
 *		|
 * 0x00000500	0xa0000500 upward	prom monitor bss
 *
 * 0x000004ff	0xa00004ff
 * to					restart block
 * 0x00000400	0xa0000400
 *
 * 0x000003ff	0xa00003ff
 * to					general exception code
 * 0x00000080	0xa0000080		(note cpu addresses as 0x80000080!)
 *
 * 0x0000007f	0xa000007f
 * to					utlbmiss exception code
 * 0x00000000	0xa0000000		(note cpu addresses as 0x80000000!)
 */

#define	PROM_STACK	0x80010000
#define STARTUP_STACK	0x8001F800

/*
 * Prom entry points
 */

/*
 * Return control to prom entry points
 *
 * RESET	transferred to on hardware reset, configures MIPS boards,
 *		runs diags, check for appropriate auto boot action in
 *		"bootmode" environment variable and performs that action.
 *
 * EXEC		called to utilize prom to boot new image.  After the booted
 *		program returns control can either be returned to the
 *		original caller of the exec routine or to the prom monitor.
 *		(to return to the original caller, the new program must
 *		not destroy any text, data, or stack of the parent.  the
 *		new programs stack continues on the parents stack.
 *
 * RESTART	re-enter the prom command parser, do not reset prom state
 *
 * REINIT	reinitialize prom state and re-enter the prom command parser
 *
 * REBOOT	check for appropriate bootmode and perform, no configuration
 *		or diags run
 *
 * AUTOBOOT	perform an autoboot sequence, no configuration or diags run
 *
 */
#define	PROM_ENTRY(x)	(R_VEC+((x)*8))

#define	PROM_RESET	PROM_ENTRY(0)	/* run diags, check bootmode, reinit */
#define	PROM_EXEC	PROM_ENTRY(1)	/* load new program image */
#define	PROM_RESTART	PROM_ENTRY(2)	/* re-enter monitor command loop */
#define	PROM_REINIT	PROM_ENTRY(3)	/* re-init monitor, then cmd loop */
#define	PROM_REBOOT	PROM_ENTRY(4)	/* check bootmode, no config */
#define	PROM_AUTOBOOT	PROM_ENTRY(5)	/* autoboot the system */
/*
 * these routines access prom "saio" routines, and may be used
 * by standalone programs that would like to use prom io
 */
#define	PROM_OPEN	PROM_ENTRY(6)
#define	PROM_READ	PROM_ENTRY(7)
#define	PROM_WRITE	PROM_ENTRY(8)
#define	PROM_IOCTL	PROM_ENTRY(9)
#define	PROM_CLOSE	PROM_ENTRY(10)
#define PROM_LSEEK      PROM_ENTRY(11)
#define	PROM_GETCHAR	PROM_ENTRY(12)	/* getchar from console */
#define	PROM_PUTCHAR	PROM_ENTRY(13)	/* putchar to console */
#define	PROM_SHOWCHAR	PROM_ENTRY(14)	/* show a char visibly */
#define	PROM_GETS	PROM_ENTRY(15)	/* gets with editing */
#define	PROM_PUTS	PROM_ENTRY(16)	/* puts to console */
#define	PROM_PRINTF	PROM_ENTRY(17)	/* kernel style printf to console */
/*
 * prom protocol entry points
 */
#define	PROM_INITPROTO	PROM_ENTRY(18)	/* initialize protocol */
#define	PROM_PROTOENABLE PROM_ENTRY(19)	/* enable protocol mode */
#define	PROM_PROTODISABLE PROM_ENTRY(20)/* disable protocol mode */
#define	PROM_GETPKT	PROM_ENTRY(21)	/* get protocol packet */
#define	PROM_PUTPKT	PROM_ENTRY(22)	/* put protocol packet */

/*
 * cache control entry points
 * flushcache is called without arguments and invalidates entire contents
 *	of both i and d caches
 * clearcache is called with a base address and length (where address is
 * 	either K0, K1, or physical) and clears both i and d cache for entries
 * 	that alias to specified address range.
 */
#define	PROM_FLUSHCACHE	PROM_ENTRY(28)	/* flush entire cache */
#define	PROM_CLEARCACHE	PROM_ENTRY(29)	/* clear_cache(addr, len) */
/*
 * The following entry points are sole to reduce the size of the debug
 * monitor and could be removed by including the appropriate code in the
 * debugger
 *
 * Libc-ish entry points
 */
#define	PROM_SETJMP	PROM_ENTRY(30)	/* save stack state */
#define	PROM_LONGJMP	PROM_ENTRY(31)	/* restore stack state */
#define	PROM_BEVUTLB	PROM_ENTRY(32)	/* utlbmiss boot exception vector */
#define	PROM_GETENV	PROM_ENTRY(33)	/* get environment variable */
#define	PROM_SETENV	PROM_ENTRY(34)	/* set environment variable */
#define	PROM_ATOB	PROM_ENTRY(35)	/* convert ascii to binary */
#define	PROM_STRCMP	PROM_ENTRY(36)	/* string compare */
#define	PROM_STRLEN	PROM_ENTRY(37)	/* string length */
#define	PROM_STRCPY	PROM_ENTRY(38)	/* string copy */
#define	PROM_STRCAT	PROM_ENTRY(39)	/* string concat */
/*
 * command parser entry points
 */
#define	PROM_PARSER	PROM_ENTRY(40)	/* command parser */
#define	PROM_RANGE	PROM_ENTRY(41)	/* range parser */
#define	PROM_ARGVIZE	PROM_ENTRY(42)	/* tokenizer */
#define	PROM_HELP	PROM_ENTRY(43)	/* prints help from command table */
/*
 * prom commands
 */
#define	PROM_DUMPCMD	PROM_ENTRY(44)	/* dump memory command */
#define	PROM_SETENVCMD	PROM_ENTRY(45)	/* setenv command */
#define	PROM_UNSETENVCMD PROM_ENTRY(46)	/* unsetenv command */
#define	PROM_PRINTENVCMD PROM_ENTRY(47)	/* printenv command */
#define	PROM_BEVEXCEPT	PROM_ENTRY(48)	/* general boot exception vector */
#define	PROM_ENABLECMD	PROM_ENTRY(49)	/* enable console command */
#define	PROM_DISABLECMD	PROM_ENTRY(50)	/* disable console command */

#define	PROM_HALT	PROM_ENTRY(54)	/* Handler for halt interrupt */

/*
 * The following entry is added in support of VAX Diagnostic Supervisors
 * DECsystem 58xx.
 */
#define	PROM_STARTCVAX	PROM_ENTRY(97)	/* start the onboard CVAX */

/*
 * Restart block -- monitor support for "warm" starts
 *
 * prom will perform "warm start" if restart_blk is properly set-up:
 *	rb_magic == RESTART_MAGIC
 *	rb_occurred == 0
 *	rb_checksum == 2's complement, 32-bit sum of first 32, 32-bit words 
 */
#define	RESTART_MAGIC	0xfeedface
#define	RESTART_CSUMCNT	32		/* chksum 32 words of restart routine */
#define	RESTART_ADDR	0xa0000400	/* prom looks for restart block here */
#define	RB_BPADDR	(RESTART_ADDR+24)/* address of rb_bpaddr */

#ifdef __LANGUAGE_C
struct restart_blk {
	int	rb_magic;		/* magic pattern */
	int	(*rb_restart)();	/* restart routine */
	int	rb_occurred;		/* to avoid loops on restart failure */
	int	rb_checksum;		/* checksum of 1st 32 wrds of restrt */
	char	*rb_fbss;		/* start of prom bss and stack area */
	char	*rb_ebss;		/* end of prom bss and stack area */
	/*
	 * These entries are for communication between the debug monitor
	 * and the client process being debugged
	 * NOTE: a return value of -1 from (*rb_vtop)() is distinguished
	 * to indicate that a translation could not be made.
	 */
	int	(*rb_bpaddr)();		/* breakpoint handler */
	int	(*rb_vtop)();		/* virtual to physical conversion rtn */
	/*
	 * config table goes here
	 */
};

/*
 * args to promexec -- monitor support for loading new programs
 *
 * bootfiles should be specified as DEV(UNIT)FILE
 * (e.g. bfs(0)bootmips_le)
 */
struct promexec_args {
	char	*pa_bootfile;		/* file to boot (only some devices) */
	int	pa_argc;		/* arg count */
	char	**pa_argv;		/* arg vector */
	char	**pa_environ;		/* environment vector */
	int	pa_flags;		/* flags, (see below) */
};

/*
 * Save STate ADDRess: at the bottom end of the startup stack.
 * We stuff the address of doadump here to get manual dumps from console mode.
 */

#define SST_ADDR 0x8001f800		/* Save STate Addr (just below netblk)*/
#define SST_MAGIC 0x15551212		/* Magic Number (get info) */

struct save_state {
	int sst_magic;			/* magic number to recognize this */
	int (*sst_dump)();		/* address of the dump routine */
	int cpu;			/* The value of "cpu".	*/
};
#endif /* __LANGUAGE_C */

/*
 * promexec flags
 */
#define	EXEC_NOGO	1	/* just load, don't transfer control */

/*
 * prom non-volatile ram conventions
 */
#define	NVSTATE_ADDR	48	/* byte in nv ram that indicates if nv valid */
#define	NVTOD_VALID	1	/* flag bit that indicates clock ok if set */

/*
 * ROM Executive Program (REX) callback offsets.  These are initially used
 * only by 3MAX and 3MIN (TURBOchannel systems).
 */
#define REX_MEMCPY		0x00	/* Copy memory			      */
#define REX_MEMSET		0x04
#define REX_STRCAT		0x08	
#define REX_STRCMP		0x0c
#define REX_STRCPY		0x10
#define REX_STRLEN		0x14
#define REX_STRNCAT		0x18
#define REX_STRNCPY		0x1c
#define REX_STRNCMY		0x20
#define REX_GETCHAR		0x24
#define REX_GETS		0x28
#define REX_PUTS		0x2c
#define REX_PRINTF		0x30
#define REX_SPRINTF		0x34
#define REX_IO_POLL		0x38
#define REX_STRTOL		0x3c
#define REX_SIGNAL		0x40
#define REX_RAISE		0x44
#define REX_TIME		0x48
#define REX_SETJUMP		0x4c
#define REX_LONGJUMP		0x50
#define REX_BOOTINIT		0x54
#define REX_BOOTREAD		0x58
#define REX_BOOTWRITE		0x5c
#define REX_SETENV		0x60
#define REX_GETENV		0x64
#define REX_UNSETENV		0x68
#define REX_SLOT_ADDRESS	0x6c
#define REX_WBFLUSH		0x70
#define REX_MSDELAY		0x74
#define REX_LEDS		0x78
#define REX_CLEAR_CACHE		0x7c
#define REX_GETSYSTYPE		0x80
#define REX_GETBITMAP		0x84
#define REX_DISABLEINTR		0x88
#define REX_ENABLEINTR		0x8c
#define REX_TESTINTR		0x90
#define REX_PRIVATE		0x94	/* Private for consle -Not for OS use */
#define REX_CONSOLE_INIT	0x98
#define REX_HALT		0x9c
#define REX_SHOWFAULT		0xa0
#define REX_GETTCINFO		0xa4
#define REX_EXECUTE_CMD		0xa8
#define REX_REX			0xac
#define REX_RESERVED1		0xb0
#define REX_RESERVED2		0xb4
#define REX_RESERVED3		0xb8
#define REX_RESERVED4		0xbc
#define REX_RESERVED5		0xc0
#define REX_RESERVED6		0xc4
#define REX_RESERVED7		0xc8
#define REX_RESERVED8		0xcc
#define REX_RESERVED9		0xd0
#define REX_RESERVED10		0xd4

/*
 * REX constants
 */
#define REX_MAGIC	0x30464354	/* REX Magic number */

/*
 * REX bitmap structure
 */
#ifdef LANGUAGE_C
struct mem_bitmap {
	int	mbmap_pagesize;	/* pagesize the bitmap represents */
	int	mbmap_bitmap[15360];	/* pointer to the memory bitmap */
};
#endif LANGUAGE_C
