/*
 * @(#)entry.s	4.3	(ULTRIX)	9/7/90
 */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/*
 *		Modification History
 *
 * 06-Sep-90 -- Randall Brown
 *	Cleanup of some code.  Removed code no longer being used. 
 *
 * 09-Aug-90 -- Randall Brown
 *	Added the variable sr_usermask, so that the mask that get put
 *	into the status register can vary from machine to machine .
 *
 * 16-Apr-90 -- jaw/gmm
 *	move kstackflg to cpudata structure.
 *
 * 01-Dec-89 -- bp
 *	fixed the SAS bss alignment problem for MDMAPSIZE
 *
 * 09-Nov-89 -- bp
 *	put mips page table entrys in bss
 *	be carefull adding entries because of an .lcomm problem
 *	which is commented on below
 *
 * 03-Oct-89 -- gmm
 *	SMP changes (remove kstackflag, secondary startup etc)
 *
 * 10-July-89 -- burns
 *	Support for DECsystem 58xx.
 *
 * 20 Feb 89 -- Kong
 *	Added to Sysmap 1026 PTEs to map the 4Mb memory space and 8Kb
 *	I/O space of the Qbus of the Mipsfair.
 *
 * 07 Dec 88 -- depp
 *	Moved the startup stack off the PROM stack (as MIPSco did it), to
 *	prevent PROM area corruption.  The PROM stack now exists elsewhere
 */
#include "../machine/param.h"
#include "../machine/cpu.h"
#include "../machine/cpu_board.h"
#include "../machine/fpu.h"
#include "../machine/asm.h"
#include "../machine/reg.h"
#include "../machine/regdef.h"
#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/fpu.h"
#include "../h/proc.h"
#include "../h/mbuf.h"
#include "../h/signal.h"
#include "../h/syscall.h"
#include "../machine/entrypt.h"		/* prom entry point definitions */
#include "../machine/debug.h"
#include "klesib.h"
#include "assym.h"
#ifdef SAS
#include "md.h"				/* get the size of the miniroot */
#endif SAS


IMPORT(sr_usermask, 4)

/*
 * Kernel entry point table
 */
STARTFRM=	(4*4)+4+4		/* 4 argsaves, old fp, and old sp */
EXPORT(eprol)				/* for benefit of profiling */
NESTED(start, STARTFRM, zero)
	j	_realstart		/* kernel entry point */
	j	_coredump		/* dump core to config'ed dump dev */
	j	_xprdump		/* dump trace buffer to console */
	j	_xprtail		/* dump tail of trace buffer */
	j	_msgdump		/* dump msg buffer to console */

/*
 * Kernel initialization
 */
_realstart:
	/*
	 * Now on prom stack; a0, a1, and a2 contain argc, argv, and environ
	 * from boot.
	 */
	/* jal	_dz_setup						*/
	/* above strickly for porting ... careful about trashing a0-a3	*/
	li	sp,STARTUP_STACK-STARTFRM # Temp startup stack
	la	gp,_gp			# load the gp
	sw	a0,_argc		# save the argc
	sw	a1,_argv		# save the argv
	sw	a2,_environ		# save the envp
#ifdef DBGMON
	la	a3,dbgstart
	jal	_check_dbg		# see if debugging requested
	lw	a0,_argc		# restore args
	lw	a1,_argv		# restore args
	lw	a2,_environ		# restore args
dbgstart:
#endif DBGMON
	sw	zero,STARTFRM-4(sp)	# zero old ra for debuggers
	sw	zero,STARTFRM-8(sp)	# zero old fp for debuggers (???)
	.set noreorder
	nop
	mtc0	zero,C0_TLBHI
	nop
	li	v0,KPTEBASE
	nop
	mtc0	v0,C0_CTXT
	nop
	.set reorder
	lw	v0,start_stack_held	# checking if a secondary processor
	bne	v0,zero,nonboot_start	# if secondary, go to start it
	jal	startup			# called startup(argc, argv, environ)
	la	sp,KERNELSTACK-STARTFRM	# leave room for arg saves, etc
	jal	main
	/*
	 * On return from main, should be set up and running as tlbpid 1.  The
	 * icode should already be copied into the user data space starting
	 * at USRDATA.  Initial register values don't matter, the GP won't
	 * be referenced.  The following should enter user mode for the first
	 * time.
	 */
	lw	v0,sr_usermask		# load sr with mask to get to user
	.set noreorder
	nop
	mtc0	v0,C0_SR		# PE BIT clear here
	nop
	.set reorder
	li	k1,USRDATA		# jump to first word of text
	lw	k0,u+PCB_CPUPTR		# get cpudata pointer
	sw	zero,CPU_KSTACK(k0)	# switching to user stack
	.set	noreorder
	j	k1			# geronimo!  Off to /etc/init!
	c0	C0_RFE			# BDSLOT
	.set	reorder
nonboot_start:
	jal	secondary_startup	# calls secondary startup. No return
	END(start)

	/*
	 * temp place to save boot params
	 */
	LBSS(_argc, 4)
	LBSS(_argv, 4)
	LBSS(_environ, 4)

/*
 * System page table
 */ 
#if !defined(PROFILING)
#define	CAMAPSIZE	4		/* 16K for dynamic space */
#else PROFILING
#define	CAMAPSIZE	256		/* 1M for dynamic space */
#endif PROFILING

/*
 * The bss lcomm has a bug in it. When specifying a size value of zero
 * it allocates eight bytes of bss.
 * So beware of hand crafting done to this table or at
 * run time you'll have some very bizarre results.
 * Any entry that can possibly be zero because of a configurable device (etc)
 * must be conditionalized correctly to the minimum value below.
 */

#define	BSS_PTE		(2)		/* Always one */

#ifndef ultrix
#define	MBMAPSIZE	(NMBCLUSTERS*CLSIZE)
#endif !ulitrx

#define	SYSMAP_PG	(K2BASE>>PGSHIFT)
#define	USRPTMAP_PG	(SYSMAP_PG+SYSPTSIZE)
#ifdef ultrix
#define	CAMAP_PG	(USRPTMAP_PG+USRPTSIZE)
#define	ECAMAP_PG	(CAMAP_PG+CAMAPSIZE)
#define	KMEM_PG		(ECAMAP_PG+BSS_PTE)
#define	EKMEM_PG	(KMEM_PG+KMEMSIZE)
#ifdef SAS
#define	MDMAP_PG	(EKMEM_PG+BSS_PTE)
#define MDMAPSIZE	((NMD/8)+BSS_PTE)	/* allow for the miniroot */
#define	SWAPMAP_PG	(MDMAP_PG+MDMAPSIZE)
#else SAS
#define	SWAPMAP_PG	(EKMEM_PG+BSS_PTE)
#endif SAS
#define SCS_PTE_PG	(SWAPMAP_PG+UPAGES)
#define SCS_PTES	((16+1)*5*17*2)/* sites*max_per_site*xfer_size*2 */
#define	KN5800_VEC_PG	(SCS_PTE_PG+SCS_PTES)
#define KN5800_VEC_SIZE (BSS_PTE)
#define NEXMAP_PG     (KN5800_VEC_PG+KN5800_VEC_SIZE)
#define NEXMAP_SIZE   (16*16*1)       /* 1 should be CVAXBI form vaxbi.h */
#define	KBMEM_PG	(NEXMAP_PG + NEXMAP_SIZE)
#if	NKLESIB > 0
#define	KBMEM_SIZE	(512 * NKLESIB)
#else
#define	KBMEM_SIZE	BSS_PTE
#endif	NKLESIB	> 0
#define	QBMAP_PG	(KBMEM_PG + KBMEM_SIZE)
#if	defined(DS5400) || defined(DS5500)
#define QBMAPSIZE	(1026)
#else
#define QBMAPSIZE	(BSS_PTE)
#endif
#else ultrix
#define	CAMAP_PG	(USRPTMAP_PG+USRPTSIZE)
#define	ECAMAP_PG	(CAMAP_PG+CAMAPSIZE)
#define	MBMAP_PG	(ECAMAP_PG)
#define	SWAPMAP_PG	(MBMAP_PG+MBMAPSIZE)
#endif ultrix

	.data
	.align	2
	SYSMAP(Sysmap,		Sysbase,	SYSMAP_PG,	SYSPTSIZE)
	SYSMAP(Usrptmap,	usrpt,		USRPTMAP_PG,	USRPTSIZE)
#ifdef ultrix
	SYSMAP(camap,		cabase,		CAMAP_PG,	CAMAPSIZE)
	SYSMAP(ecamap,		calimit,	ECAMAP_PG,	BSS_PTE)
	SYSMAP(kmempt,		kmembase,	KMEM_PG,	KMEMSIZE)
	SYSMAP(ekmempt,		kmemlimit,	EKMEM_PG,	BSS_PTE)
#ifdef SAS
	SYSMAP(mdbufmap,	MD_bufmap,	MDMAP_PG,	MDMAPSIZE)
#endif SAS
#else
	SYSMAP(camap,		cabase,		CAMAP_PG,	CAMAPSIZE)
	SYSMAP(ecamap,		calimit,	ECAMAP_PG,	BSS_PTE)
	SYSMAP(Mbmap,		mbutl,		MBMAP_PG,	MBMAPSIZE)
#endif ultrix
	SYSMAP(Swapmap,		VA_swaputl,	SWAPMAP_PG,	UPAGES)
        SYSMAP(scsmemptmap,	scsmempt,	SCS_PTE_PG,	SCS_PTES)
	SYSMAP(Kn5800_vec,      kn5800_vectors, KN5800_VEC_PG, KN5800_VEC_SIZE)
	SYSMAP(Nexmap,          nexus,          NEXMAP_PG,      NEXMAP_SIZE)
	SYSMAP(KBMEMmap,	kbmem,		KBMEM_PG,	KBMEM_SIZE)
	/*
	 * The Q bus consists of 4Mb (1024 mips pages) of memory space
	 * and 8Kb (2 mips pages) of I/O space.  Here we create an array
	 * of 1026 mips pages. (or 1 if not building for a mipsfair).
	 * These PTEs will eventually be initialised so that the first 1024
	 * pages are mapped to the Q bus memory space, the next 2 pages are
	 * mapped to the Q bus I/O space.
	 */
	SYSMAP(QMEMmap,		qmem,		QBMAP_PG,	QBMAPSIZE)
	.globl	eSysmap
	.lcomm	eSysmap,4
	.text
