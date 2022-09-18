/*	@(#)prom.h	4.2	(ULTRIX)	9/4/90				      */
#include <ansi_compat.h>
#ifdef __mips
/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/*
 * prom.h -- prom monitor definitions
 */

/*
 * catch bogus compiles
 */
#if defined(__MIPSEB) && defined(__MIPSEL)
# include "error -- both MIPSEB and MIPSEL defined"
#endif /* defined(__MIPSEB) && defined(__MIPSEL) */

#if !defined(__MIPSEB) && !defined(__MIPSEL)
# include "error -- neither MIPSEB or MIPSEL defined"
#endif /* !defined(__MIPSEB) && !defined(__MIPSEL) */

/*
 * PROM_STACK is the address of the first word above the prom stack
 * the prom stack grows downward from the first word less than PROM_STACK
 */
#define	PROM_STACK	0xa0010000

/*
 * PROM_SR is the status register value used for normal prom operation
 * currently set to let bus error interrupts occur
 */
#define	PROM_SR		(SR_IMASK7|SR_IEC)

/*
 * width defines for memory operations
 */
#define	SW_BYTE		1
#define	SW_HALFWORD	2
#define	SW_WORD		4

/*
 * non-volatile ram addresses
 * NOTE: everything must fit within 50 bytes for the 146818 TOD chip
 */
#define	NVLEN_MAX	50
#define	NVADDR_BASE	0

/*
 * netaddr is used by network software to determine the internet
 * address, it should be a string containing the appropriate
 * network address in "." format
 */
#define	NVADDR_NETADDR	(NVADDR_BASE)
#define	NVLEN_NETADDR	16

/*
 * lbaud/rbaud are the initial baud rates for the duart
 * (e.g. "9600")
 */
#define	NVADDR_LBAUD	(NVADDR_NETADDR+NVLEN_NETADDR)
#define	NVLEN_LBAUD	5

#define	NVADDR_RBAUD	(NVADDR_LBAUD+NVLEN_LBAUD)
#define	NVLEN_RBAUD	5

/*
 * bootfile is the initial program loaded on an autoboot
 * (e.g. "bfs(0)mipsboot_le")
 */
#define	NVADDR_BOOTFILE	(NVADDR_RBAUD+NVLEN_RBAUD)
#define	NVLEN_BOOTFILE	20

/*
 * bootmode controls autoboots/warm starts/command mode on reset
 * "a" => autoboot on reset
 * "w" => warm start if restart block correct, else autoboot
 * anything else cause entry to command mode
 */
#define	NVADDR_BOOTMODE	(NVADDR_BOOTFILE+NVLEN_BOOTFILE)
#define	NVLEN_BOOTMODE	1

/*
 * console controls what consoles are enabled at power-up
 * 'a' indicates "all" consoles
 * 'r' indicates both local and remote uarts
 * anything else indicates only local uart
 */
#define	NVADDR_CONSOLE	(NVADDR_BOOTMODE+NVLEN_BOOTMODE)
#define	NVLEN_CONSOLE	1

/*
 * state maintains the current validity of the tod clock and
 * non-volatile ram
 * see NVSTATE_* definitions below
 */
#define	NVADDR_STATE	(NVADDR_CONSOLE+NVLEN_CONSOLE)
#define	NVLEN_STATE	1

/*
 * failcode is used by the power-on diagnostics to save a failure
 * code for use by service techs
 */
#define	NVADDR_FAILCODE	(NVADDR_STATE+NVLEN_STATE)
#define	NVLEN_FAILCODE	1

#define	NVLEN_TOTAL	(NVADDR_VALID+NVLEN_VALID)
#if NVLEN_TOTAL > NVLEN_MAX
# include "error -- non-volatile ram overflow"
#endif /* NVLEN_TOTAL > NVLEN_MAX */

#define	NVSTATE_TODVALID	0x1	/* tod can be trusted */
#define	NVSTATE_RAMVALID	0x2	/* nv ram can be trusted */

/*
 * Misc constants
 */
#define	AUTOBOOT_DELAY	20		/* seconds to abort autoboots */
#define	DEFAULT_BOOTFILE "dkip(0,0,8)sash"	/* boot standalone shell */

#define	streq(a,b)	(strcmp(a,b) == 0)

/*
 * RMW_TOGGLE -- cpu board address which when read
 * cause a read/modify/write cycle to occur with next read and
 * write cycles
 */
#define	RMW_TOGGLE	0xbe400003

/*
 * startup led sequence values
 */
#define	MEMCFG_PATTERN		0x21	/* memory configured */
#define	WARMST_PATTERN		0x22	/* warm start attempted */
#define	ZEROBSS_PATTERN		0x23	/* prom bss zero'ed */
#define	CACHE_PATTERN		0x24	/* cache initialized */
#define	SAIO_PATTERN		0x25	/* saio initialized */
#define	CACHE2_PATTERN		0x26	/* cache initialized */
#define	ENV_PATTERN		0x27	/* environment initialized */
#define	SAIO2_PATTERN		0x28	/* saio initialized */
#define	ZEROMEM_PATTERN		0x29	/* memory cleared */
#define	LMEM_PATTERN		0x2a	/* in local memory config */
#define	LMEM_WAIT_PATTERN	0x2b	/* lmem waiting for exception */
#define	LMEM_ACK_PATTERN	0x2c	/* lmem acking vme interrupt */
#define	LMEM_NOMEM_PATTERN	0x2d	/* no local memory found */
#define	LMEM_ERROR_PATTERN	0x2e	/* error in config code */
#define	LMEM_FAULT_PATTERN	0x31	/* unexpected exception in lmem */
#define	LMEM_RESET_PATTERN	0x32	/* resetting local memory */
#define	LMEM_RSTDONE_PATTERN	0x33	/* local memory reset complete */
#define	LMEM_CLRVME_PATTERN	0x34	/* clearing pending vme intrs */
#define	BEV_UTLBMISS_PATTERN	0x35	/* unexpected bev utlbmiss */
#define	BEV_GENERAL_PATTERN	0x36	/* unexpected bev exception */

/*
 * HACK
 * define eprintf to be printf for now, maybe implement as printf
 * to console() in future
 */
#define	eprintf	printf
#endif /* __mips */
