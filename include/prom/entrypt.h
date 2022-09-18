/*	@(#)entrypt.h	4.2	(ULTRIX)	9/4/90				      */
#include <ansi_compat.h>
#ifdef __mips
/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/*
 * entrypt.h -- misc. defines of interest to standalones and kernels
 */

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
 * 0x00020000	0xa0020000 upward	standalone program text, data, and bss
 *					(kernel is loaded here, also)
 *
 * 0x0001ffff	0xa001ffff downward	dbgmon stack
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
#define	PROM_GETCHAR	PROM_ENTRY(11)	/* getchar from console */
#define	PROM_PUTCHAR	PROM_ENTRY(12)	/* putchar to console */
#define	PROM_SHOWCHAR	PROM_ENTRY(13)	/* show a char visibly */
#define	PROM_GETS	PROM_ENTRY(14)	/* gets with editing */
#define	PROM_PUTS	PROM_ENTRY(15)	/* puts to console */
#define	PROM_PRINTF	PROM_ENTRY(16)	/* kernel style printf to console */
/*
 * prom protocol entry points
 */
#define	PROM_INITPROTO	PROM_ENTRY(17)	/* initialize protocol */
#define	PROM_PROTOENABLE PROM_ENTRY(18)	/* enable protocol mode */
#define	PROM_PROTODISABLE PROM_ENTRY(19)/* disable protocol mode */
#define	PROM_GETPKT	PROM_ENTRY(20)	/* get protocol packet */
#define	PROM_PUTPKT	PROM_ENTRY(21)	/* put protocol packet */
/*
 * read-modify-write routine use special cpu board circuitry to accomplish
 * vme bus r-m-w cycles.  all routines are similar to:
 *	unsigned char
 *	orb_rmw(addr, mask)
 *	unsigned char *addr;
 *	unsigned mask;
 *	{
 *		register unsigned rval;
 *
 *		lockbus();
 *		rval = *addr;
 *		*addr = rval & mask;
 *		unlockbus();
 *		return(rval);
 *	}
 */
#define	PROM_ORW_RMW	PROM_ENTRY(22)	/* r-m-w version of or word */
#define	PROM_ORH_RMW	PROM_ENTRY(23)	/* r-m-w version of or halfword */
#define	PROM_ORB_RMW	PROM_ENTRY(24)	/* r-m-w version of or byte */
#define	PROM_ANDW_RMW	PROM_ENTRY(25)	/* r-m-w version of and word */
#define	PROM_ANDH_RMW	PROM_ENTRY(26)	/* r-m-w version of and halfword */
#define	PROM_ANDB_RMW	PROM_ENTRY(27)	/* r-m-w version of and byte */
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

/*
 * clear existing fault handlers
 * used by clients that link to prom on situations where client has
 * interrupted out of prom code and wish to reenter without being
 * tripped up by any pending prom timers set earlier.
 */
#define	PROM_CLEARNOFAULT PROM_ENTRY(51)/* clear existing fault handlers */

/*
 * PROM_NOTIMPLEMENT is guaranteed to be not implemented so programs
 * which link against various rev's of the prom can config to entry points
 * which aren't implemented in earlier prom rev's.  An entry point is
 * guaranteed to be implemented if the 32 bit word at the prom entrypt
 * is not equal to this entrypt.
 */
#define	PROM_NOTIMPLEMENT PROM_ENTRY(52)/* guaranteed to be not implemented */

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
#endif /* __LANGUAGE_C */

/*
 * promexec flags
 */
#define	EXEC_NOGO	1	/* just load, don't transfer control */
#endif /* __mips */
