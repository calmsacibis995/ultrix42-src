/*
 * @(#)cpuconf.h	4.8    ULTRIX  3/6/91
 */

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * Modification History: cpuconf.h
 *
 * 06-Mar-91 	jaw
 *	3min spl optimization.
 *
 * 20-Aug-90	Matt Thomas
 *	Added maxdriftrecip (The reciprocal of the maximum clock drift
 *	rate) for both VAX and MIPS.
 *
 * 13-Aug-90	sekhar
 *	added print_consinfo interface for both vax and mips.
 *	added log_errinfo interface for both vax and mips.
 *
 * 03-Aug-90	Randall Brown
 *	Added system specific entries for all spl's and the intr() routine.
 *	Added entries for specific clock variables ( todrzero, rdclk_divider).
 *	Added #define for DS_5000_100 (3MIN).
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added support for VAX9000.
 *
 * 01-May-90    szczypek
 *      Added Mariah ID and Mariah variant.
 *
 * 01-May-90    Paul Grist
 *      Added support for mipsmate - DECsystem 5100
 *
 * 10-July-89	burns
 *	Moved Vax io related items to common section of table for DS5800.
 *	Added memsize and cpuinit for afd.
 *
 * 30-June-89	afd
 *	Add memory sizing routine to cpu switch table.
 *
 * 09-June-89	afd
 *	Added field HZ to cpusw (used to be in param.c).
 *	hz, tick, tickadj are set in processor specific init routines.
 *
 * 02-May-89	afd
 *	Added define VAX_3900 for Mayfair III system.
 *
 * 07-Apr-89	afd
 *	Created this file as a merged version of the old VAX cpuconf.h
 *	with new entries for MIPS based systems.  This file now supports
 *	both VAX and MIPS based systems.
 *
 *	Moved cpu type and system type defines here from cpu.h
 *	Moved macros for getting items from "systype" word here from hwconf.h
 *	Moved defines for R2000a cpu type and PMAX systype here from hwconf.h
 *	Added additional defines for CPU types, systypes and cpusw indexes.
 */

/*
 * Macros for getting the fields out of the PROMs "systype" word on mips systems
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#define GETCPUTYPE(systword) ((systword >> 24) & 0xff)
#define GETSYSTYPE(systword) ((systword >> 16) & 0xff)
#define GETFRMREV(systword) ((systword >> 8) & 0xff)
#define GETHRDREV(systword) (systword & 0xff)

/*
 * DEC cpu type for mips based systems is: 128 + mips PRId implementation level.
 * These values are architected.
 */
#define R2000a_CPU	130
#define R3000_CPU	130

/*
 * DEC cpu types for VAX systems (the contents of the SID field).
 * These values are architected.
 */
#define CVAX_CPU	10 
#define RIGEL_CPU	11
#define MARIAH_CPU      18

/*
 * System variants.
 */
#define MARIAH_VARIANT  1000

/*
 * Defines for bits in the cpu switch flags field
 */
#define CPU_ICR 0x80000000		/* bit set if cpu has ICR & NICR */

/*
 * Systypes (ST_) for mips based systems (as found in the PROM).
 * These values are architected.
 */
#define ST_DS3100	1		/* PMAX		*/
#define ST_DS5000	2		/* 3MAX		*/
#define ST_DS5000_100	3		/* 3MIN */
#define ST_DS5800	5		/* ISIS		*/
#define ST_DS5400	6		/* MIPSfair	*/
#define ST_DS5500	11		/* MIPSFAIR-2	*/
#define ST_DS5100	12		/* MIPSMATE	*/

/*
 * VAX SID extension register and CPU systypes,
 */
#define SID_EXT         0x20040004      /* I/O space phys addr of SID ext reg */
#define ST_MVAXII       0x1             /* Micro/VAX-II sub-type */
#define ST_VAXSTAR      0x4             /* VAXstar/CVAXstar/PVAX sub-type */
#define SB_TMII         0x2             /* SYS_DEPEND field in SYS_TYPE for TMII */
#define ST_CVAXQ        0x1             /* Single CPU Qbus systype of CVAX */
#define SB_KA650        0x1             /* KA650 subtype of CVAXQ */
#define SB_KA640        0x2             /* KA640 subtype of CVAXQ */
#define SB_KA655        0x3             /* KA655 subtype of CVAXQ */
#define ST_KA60         0x3             /* Firefox (KA60) subtype of CVAX */
#define ST_8200         0x5             /* Single CPU Scorpio */
#define ST_8300         0x6             /* Dual CPU Scorpio */
#define ST_8400         0x7             /* 3-CPU Scorpio (non-supp.) */
#define ST_8500         0x8             /* Single CPU Naut. (slow) */
#define ST_8550         0x9             /* Single CPU Naut.(fast/non-expand) */
#define ST_8700         0xa             /* Single CPU Naut. (fast/expand) */
#define ST_8800         0xb             /* Dual CPU Naut. (fast) */
#define	ST_9000		0xe		/* VAX9000 */
#define ST_6210         0x1             /* 1 CPU calypso  */
#define ST_6220         0x1             /* 2 CPU calypso  */
#define ST_6230         0x1             /* 3 CPU calypso  */
#define ST_6240         0x1             /* 4 CPU calypso  */
#define ST_6250         0x1             /* 5 CPU calypso  */
#define ST_6260         0x1             /* 6 CPU calypso  */
#define ST_6270         0x1             /* 7 CPU calypso  */
#define ST_6280         0x1             /* 8 CPU calypso  */

/*
 * Value to be stored in the global variable "cpu".
 * The contents of "cpu" is no longer used to index into the "cpusw".
 * These values are arbitrarily assigned by ULTRIX, but must each be unique.
 * The variable "cpu" really contains a unique "system" identifier.
 *
 * Early VAX values (up through MVAX_II) correspond exactly to the VAX SID
 * register value.  New VAX systems must have a define above such as:
 *     #define xxxx_CPU SID-value
 *
 * note: the vector cpu_types[] within ../../io/scs/scsvar.c must be updated when
 *       new cpu values are defined.
 */

#define UNKN_SYSTEM	0
#define VAX_780         1
#define VAX_750         2
#define VAX_730         3
#define VAX_8600        4
#define VAX_8200        5
#define VAX_8800        6
#define MVAX_I          7
#define MVAX_II         8
#define V_VAX           9	/* Virtual VAX 			*/
#define VAX_3600        10	/* Mayfair I 			*/
#define VAX_6200        11	/* CVAX/Calypso 		*/
#define VAX_3400        12	/* Mayfair II 			*/
#define C_VAXSTAR       13	/* VAX3100 (PVAX) 		*/
#define VAX_60          14	/* Firefox 			*/
#define VAX_3900        15	/* Mayfair III 			*/
#define	DS_3100		16	/* DECstation 3100 (PMAX) 	*/
#define VAX_8820        17	/* This is the SID for Polarstar*/
#define	DS_5400 	18	/* MIPSfair 			*/
#define	DS_5800		19	/* ISIS 			*/
#define	DS_5000		20	/* afdfix: put in product name 	*/
#define	DS_CMAX		21	/* afdfix: put in product name 	*/
#define VAX_6400	22	/* RIGEL/Calypso 		*/
#define VAXSTAR		23	/* VAXSTAR			*/
#define DS_5500		24	/* MIPSFAIR-2			*/
#define DS_5100		25	/* MIPSMATE			*/
#define	VAX_9000	26	/* VAX9000			*/
#define DS_5000_100	27	/* 3MIN 			*/
#define CPU_MAX         27	/* Same # as last real entry	*/

#ifdef __mips
/*
 * Defines for possible values of the flags field for mips
 */
#define SCS_START_SYSAPS	0x00000001
#define MSCP_POLL_WAIT		0x00000002
#endif 

#ifndef LOCORE
/*
 * The system switch is defined here.  Each entry is the only
 * line between the main unix code and the cpu dependent
 * code.  The initialization of the system switch is in the
 * file cpuconf.c.  The index values used in the switch are
 * defined in cpu.h.
 */

struct cpusw
{
    /* Common Routines */
    int system_type;		/* 1 Value for "cpu" (VAX_nnnn or DS_nnnn) */
    int (*machcheck)();		/* 2 Hrdwre trap (Vax: mcheck; Mips: trap err) */
    int (*harderr_intr)();	/* 3 Hard err (Vax:SCB 60; Mips: memerr) */
    int (*softerr_intr)();	/* 4 Soft err (Vax:SCB 54 (CRD); Mips:not-yet) */
    int (*timer_action)();	/* 5 VAX: Enable CRD intr; MIPS: check CPEs */
    int (*cons_putc)();		/* 6 Write char to console		*/
    int (*cons_getc)();		/* 7 Read char from console		*/
    int (*config)();		/* 8 System configuration			*/
    int (*cachenbl)();		/* 9 Turn on cache (I or D cache)		*/
    int (*cachdisbl)();		/* 10 Turn off cache (I or D cache)	*/
    int (*flush_cache)();	/* 11 Flush caches (I or D)		*/
    int (*badaddr)();		/* 12 Probe addresses			*/
    int (*readtodr)();		/* 13 Read time of day			*/
    int (*writetodr)();		/* 14 Write time of day			*/
    int (*microdelay)();	/* 15 Microdelay				*/
    int (*clear_err)();		/* 16 Clear hardware error (from I/O probe)*/
    int (*mapspace)();		/* 17 Make IO, CSR, ..etc accessible	*/
    int (*reboot)();		/* 18 Reboot Ultrix			*/
    int (*halt)();		/* 19 Do a cpu halt (simulate on mips)	*/
    int (*startcpu)();		/* 20 Start a non-boot cpu			*/
    int (*stopcpu)();		/* 21 Stop a cpu				*/
    int (*nexaddr)();		/* 22 Get address of next I/O adapter	*/
    int (*umaddr)();		/* 23					*/
    int (*udevaddr)();		/* 24					*/
    int (*print_consinfo)();    /* 25 print information to console      */
    int (*log_errinfo)();    	/* 26 log information to error log buf  */

#ifdef __mips
    /* MIPS Specific Routines */
    int (*invalid)();		/* Cache Invalidate			*/
    int (*wbflush)();		/* Flush the write buffers		*/
    int (*startclocks)();	/* Start the system clock		*/
    int (*stopclocks)();	/* Stop the system clock		*/
    int (*ackclock)();		/* Acknowledge clock interrupt		*/
    int (*init)();		/* Initialize tables (splm, intr_vecs)	   */
    int (*memsize)();		/* Size and clear memory at system startup */
    int	(*clean_icache)();	/* Clean a portion of the icache	   */
    int	(*clean_dcache)();	/* Clean a portion of the dcache	   */
    int	(*page_iflush)();	/* Flush an icache page			   */
    int	(*page_dflush)();	/* Flush an dcache page			   */
    int (*getspl)();		/* Return current SPL level		   */
    int (*whatspl)();		/* Evaluate current SPL level 		   */
#endif

#ifdef __vax
    /* VAX Specific Routines */
    int (*setcache)();		/* Set up cach enable parameters	*/
#endif

    /* Common Data Values */
    int pc_umsize;		/* unibus memory size */
    short pc_haveubasr;		/* have uba status register */
    unsigned long maxdriftrecip; /* maximum drift rate for this processor */

#ifdef __mips
    /* MIPS Specific Data Values */
    int HZ;			/* how many times a sec the clock interrupts */
    unsigned int todrzero;	/* what the TODR should contain when the */
                                /* 'year' begins.			 */
    int	rdclk_divider;		/* value to divide read_todclk() with to */
                                /* get into seconds */
    int	flags;			/* system specific flags */
#endif

#ifdef __vax
    /* VAX Specific Data Values */
    short pc_nnexus;		/* number of nexus slots */
    int pc_nexsize;		/* size of a nexus */
    short *pc_nextype;		/* adapter types if no conf. reg. */
    short pc_nioa;		/* number of IO adapters (8600) */
    short **pc_ioabase;		/* physical base of IO adapters space */
    int pc_ioasize;		/* size of an IO adapter */
    int flags;			/* cpusw flags */
#endif
};

extern struct cpusw cpusw[];

#endif LOCORE

#define BADADDR(addr,len) (badaddr(addr,len))

