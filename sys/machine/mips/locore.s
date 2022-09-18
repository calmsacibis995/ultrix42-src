/*
 * @(#)locore.s	4.7        (ULTRIX)        3/6/91
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/************************************************************************
 *
 *	Modification History: locore.s
 *
 *   06-Mar-91 -- jaw
 *	3min spl optimization.
 *
 *   06-Sep-90 -- Randall Brown
 *	In exception_exit(), if we are a 3min a call is made to 
 *	kn02ba_exception_exit() to restore the interrupt mask
 *	status register in the exception frame.  This is because the
 *	values stored in the exception frame could be wrong if a
 *	device has enabled or disabled its interrupt.
 *
 *   23-Aug-90 -- Randall Brown
 *	In the VEC_tlbmiss(), VEC_tlbmod(), VEC_nofault() routines,  the
 *	status register is restored to the value it was before
 *	calling routines in trap.c.  This is because the status register
 *	may come back with interrupt enabled and interrupts must be disabled
 *	before calling exception_exit().
 *
 *   09-Aug-90 -- Randall Brown
 *	Changed the size of the exception frame.  Added two new entries,
 *	the variables except_sys1 and except_sys2 contain the 
 *	address of the data to save on the frame.
 *
 *   30-Apr-90 -- Randall Brown
 *	Changed the exeception handler to call (*intrp)() instead of 
 *	a straight call to intr().  This makes the interrupt handler
 *	more system specific.  Also changed all the spl routines to
 *	kn01_spl* so they can be called through the new spl mechanism.
 *
 *   17-Jul-90 -- jaw
 *	Fix for int overflow on wrap of l_won.
 *
 *   19-Jun-90 -- jmartin
 *	Fixes for "panic: vrelvm rssize"
 *
 *   16-Apr-90 -- jaw/gmm
 *	move kstackflg to cpudata structure.
 *
 *   29-Mar-90 gmm/jaw
 *	Do not change affinity to ALLCPU if a signal pending for the process
 *	(in checkfp()).
 *
 *   16-Mar-90 robin
 *	Changed kn220wbflush to use a uncached address to force the flush.
 *
 *   13-Mar-90 jaw
 *	mips lock primitive needs to inc won field.
 *
 *   06-Mar-90 -- gmm
 *	Inform every cpu if the process affinity changes back to ALLCPU, if
 *	smp defined.
 *
 *   03-Mar-90 jaw
 *	primitive change to optimize mips.
 *
 *   29-Dec-89 -- afd
 *	Put a work-around into VEC_ibe and VEC_dbe for DS5000 (3max)
 *	so we can dismiss the pending interrupt when we have a bus error.
 *
 *   26-Dec-1989 Robin
 *	Added kn220 write buffer flush routine.
 *
 *   4-Dec-89 -- jmartin
 *	Fix functions checkfp and VEC_cpfault to use new PTE format.
 *
 *   28-Nov-89  Alan Frechette
 *	Now successfully get dumps on hung MIPS systems. Use the
 *	ultrix startup stack for performing system dumps and remap
 *	the OS exception vector handling code in doadump(). This is
 *	needed when forcing a dump after hitting the RESET button.
 *	Also changed the _coredump() routine to call doadump().
 *
 *   13-Nov-89  Giles Atkinson
 *	Added wait_tick function for use by LMF.
 *
 *   09-Nov-89	bp
 *	Added globals VSysmap and Dbptemap for Vax pte mapping.
 *
 *  19-Oct-89 -- jmartin
 *	For R2000/R3000, shift PTE left 8 just before writing C0_TLBLO.
 *
 *   04-Oct-89  gmm
 *	SMP changes (use cpudata for nofault, get rid of kstackflag, 
 *	process affinity for fpowner, etc)
 *
 * 22-sep-89 burns (ISIS pool merge)
 *
 *   10-July-89	burns
 *	Added cpu check for ISIS to avoid crash due to lack of a SBE register.
 *	Handle halt interrupt early.
 *
 *   17-Apr-1989	Kong
 *	Fixed VEC_ibe exception handler so that it doesn't accidentally
 *	branch to the breakpoint trap handler.
 *
 *   22-Mar-1989  Pete Keilty
 *	Added unixtovms time routine used by SCS
 *
 *   16-Mar-1989  Kong
 *	Added routine "read_nofault".  
 *
 *   20-Feb-1989	Kong
 *	Changed routine "badaddr" to "bbadaddr" as in VAXen.  "badaddr"
 *	is now a routine in machdep.c that does system dependent code.
 *	PMAX does not seem to use badaddr.
 *
 *
 * 06-Feb-1989  Kong
 *	Added entries to "splm" array to include new routines spl6==splhigh,
 *	spl7 == splextreme.
 *	VEC_int saves registers s1-s8 if the interrupt may be a FPU interrupt.
 *
 * 23-Jan-1989	Kong
 *     .Made the splxxx routines work for PMAX and MIPSFAIR.  This is
 *	done by introducing an array "splm" which contains the proper
 *	interrupt masks for the machine we are running on.  The "splm"
 *	array is initialized at system startup time, before any interrupts
 *	are allowed.
 *     .Made the routine "wbflush" system dependent.
 *
 */

#define CNT_TLBMISS_HACK 0

#include "../machine/param.h"
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
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
#include "../machine/entrypt.h"	/* prom entry point definitions */
#include "../machine/debug.h"
#include "assym.h"
#include "../machine/kn02ba.h"

/* create define for original mips architecture */
#if defined(DS3100) || defined(DS5400) || defined(DS5500) || defined(DS5800) || defined(DS5000) || defined(DS5100)
#define MIPS_ARCH_SPL_ORIG
#endif
#ifdef DS5000_100
IMPORT(mips_spl_arch_type,4)
IMPORT(ipllevel,4)
#endif

/* bss area location that kn220 uses to force a write buffer flush
 */
LBSS(kn220wbflush_loc,4)

/*
 * User structure is UPAGES at top of user space.
 * This is here so that u appears in the symbol table for debuggers, etc.
 */
	ABS(u, UADDR)

/*
 * define prom entrypoints of interest to kernel
 *
 * reset reboots as indicated by $bootmode and looks for warm start block
 */
EXPORT(prom_reset)
	li	v0,PROM_RESET
	j	v0

/*
 * halt
 */
EXPORT(prom_halt)
	li	v0,PROM_HALT
	j	v0

/*
 * autoboot always reboots regardless of $bootmode
 */
EXPORT(prom_autoboot)
	li	v0,PROM_AUTOBOOT
	j	v0

/*
 * reboot does action indicated by $bootmode
 */
EXPORT(prom_reboot)
	li	v0,PROM_REBOOT
	j	v0

/*
 * restart always enters prom command mode
 */
EXPORT(prom_restart)
	li	v0,PROM_RESTART
	j	v0

/*
 * prom flow-controlled console io
 */
EXPORT(prom_getchar)
	li	v0,PROM_GETCHAR
	j	v0

EXPORT(prom_putchar)
	li	v0,PROM_PUTCHAR
	j	v0

#ifdef notdef
/*
 * The next 6 are no longer in the PROM
 */
/*
 * prom read-modify-write routines
 */
EXPORT(andb_rmw)
	li	v0,PROM_ANDB_RMW
	j	v0

EXPORT(andh_rmw)
	li	v0,PROM_ANDH_RMW
	j	v0

EXPORT(andw_rmw)
	li	v0,PROM_ANDW_RMW
	j	v0

EXPORT(orb_rmw)
	li	v0,PROM_ORB_RMW
	j	v0

EXPORT(orh_rmw)
	li	v0,PROM_ORH_RMW
	j	v0

EXPORT(orw_rmw)
	li	v0,PROM_ORW_RMW
	j	v0
#endif notdef

EXPORT(prom_exec)
	li	v0,PROM_EXEC
	j	v0

EXPORT(prom_getenv)
	lw	v0,rex_base
	bne	v0,zero,1f
	li	v0,PROM_GETENV
	j	v0
1:	j	rex_getenv

EXPORT(prom_setenv)
	li	v0,PROM_SETENV
	j	v0

EXPORT(prom_open)
	li	v0,PROM_OPEN
	j	v0

EXPORT(prom_close)
	li	v0,PROM_CLOSE
	j	v0

EXPORT(prom_lseek)
	li	v0,PROM_LSEEK
	j	v0

EXPORT(prom_read)
	li	v0,PROM_READ
	j	v0

EXPORT(prom_write)
	li	v0,PROM_WRITE
	j	v0

/*
 * REX global variable
 */
	BSS(rex_base,4)			# REX base address.
	BSS(rex_magicid,4)		# REX base address.

/*
 * ROM Executive Program (REX) callbacks (entry points)
 *
 * These are presently used only with 3MAX.  They are currently TURBOchannel
 * specific.
 */

EXPORT(rex_memcpy)
	lw	v0,rex_base
	lw	v0,REX_MEMCPY(v0)
	j	v0
/*
EXPORT(rex_memset)
	lw	v0,rex_base
	lw	v0,REX_MEMSET(v0)
	j	v0

EXPORT(rex_strcat)
	lw	v0,rex_base
	lw	v0,REX_STRCAT(v0)
	j	v0

EXPORT(rex_strcmp)
	lw	v0,rex_base
	lw	v0,REX_STRCMP(v0)
	j	v0

EXPORT(rex_strlen)
	lw	v0,rex_base
	lw	v0,REX_STRLEN(v0)
	j	v0

EXPORT(rex_strncat)
	lw	v0,rex_base
	lw	v0,REX_STRNCAT(v0)
	j	v0

EXPORT(rex_strncpy)
	lw	v0,rex_base
	lw	v0,REX_STRNCPY(v0)
	j	v0

EXPORT(rex_strncmy)
	lw	v0,rex_base
	lw	v0,REX_STRNCMY(v0)
	j	v0

EXPORT(rex_getchar)
	lw	v0,rex_base
	lw	v0,REX_GETCHAR(v0)
	j	v0
*/
EXPORT(rex_gets)
	lw	v0,rex_base
	lw	v0,REX_GETS(v0)
	j	v0
/*
EXPORT(rex_puts)
	lw	v0,rex_base
	lw	v0,REX_PUTS(v0)
	j	v0
*/
EXPORT(rex_printf)
	lw	v0,rex_base
	lw	v0,REX_PRINTF(v0)
	j	v0
/*
EXPORT(rex_sprintf)
	lw	v0,rex_base
	lw	v0,REX_SPRINTF(v0)
	j	v0

EXPORT(rex_io_poll)
	lw	v0,rex_base
	lw	v0,REX_IO_POLL(v0)
	j	v0

EXPORT(rex_strtol)
	lw	v0,rex_base
	lw	v0,REX_STRTOL(v0)
	j	v0

EXPORT(rex_signal)
	lw	v0,rex_base
	lw	v0,REX_SIGNAL(v0)
	j	v0

EXPORT(rex_raise)
	lw	v0,rex_base
	lw	v0,REX_RAISE(v0)
	j	v0

EXPORT(rex_time)
	lw	v0,rex_base
	lw	v0,REX_TIME(v0)
	j	v0

EXPORT(rex_setjump)
	lw	v0,rex_base
	lw	v0,REX_SETJUMP(v0)
	j	v0

EXPORT(rex_longjump)
	lw	v0,rex_base
	lw	v0,REX_LONGJUMP(v0)
	j	v0
*/
EXPORT(rex_bootinit)
	lw	v0,rex_base
	lw	v0,REX_BOOTINIT(v0)
	j	v0

EXPORT(rex_bootread)
	lw	v0,rex_base
	lw	v0,REX_BOOTREAD(v0)
	j	v0

EXPORT(rex_bootwrite)
	lw	v0,rex_base
	lw	v0,REX_BOOTWRITE(v0)
	j	v0
/*
EXPORT(rex_setenv)
	lw	v0,rex_base
	lw	v0,REX_SETENV(v0)
	j	v0
*/
EXPORT(rex_getenv)
	lw	v0,rex_base
	lw	v0,REX_GETENV(v0)
	j	v0
/*
EXPORT(rex_unsetenv)
	lw	v0,rex_base
	lw	v0,REX_UNSETENV(v0)
	j	v0

EXPORT(rex_slot_address)
	lw	v0,rex_base
	lw	v0,REX_SLOT_ADDRESS(v0)
	j	v0

EXPORT(rex_wbflush)
	lw	v0,rex_base
	lw	v0,REX_WBFLUSH(v0)
	j	v0

EXPORT(rex_msdelay)
	lw	v0,rex_base
	lw	v0,REX_MSDELAY(v0)
	j	v0

EXPORT(rex_leds)
	lw	v0,rex_base
	lw	v0,REX_LEDS(v0)
	j	v0

EXPORT(rex_clear_cache)
	lw	v0,rex_base
	lw	v0,REX_CLEAR_CACHE(v0)
	j	v0
*/
EXPORT(rex_getsystype)
	lw	v0,rex_base
	lw	v0,REX_GETSYSTYPE(v0)
	j	v0

EXPORT(rex_getbitmap)
	lw	v0,rex_base
	lw	v0,REX_GETBITMAP(v0)
	j	v0
/*
EXPORT(rex_disableintr)
	lw	v0,rex_base
	lw	v0,REX_DISABLEINTR(v0)
	j	v0

EXPORT(rex_enableintr)
	lw	v0,rex_base
	lw	v0,REX_ENABLEINTR(v0)
	j	v0

EXPORT(rex_testintr)
	lw	v0,rex_base
	lw	v0,REX_TESTINTR(v0)
	j	v0

EXPORT(rex_console_init)
	lw	v0,rex_base
	lw	v0,REX_CONSOLE_INIT(v0)
	j	v0
*/
EXPORT(rex_halt)
	lw	v0,rex_base
	lw	v0,REX_HALT(v0)
	j	v0
/*
EXPORT(rex_showfault)
	lw	v0,rex_base
	lw	v0,REX_SHOWFAULT(v0)
	j	v0

EXPORT(rex_gettcinfo)
	lw	v0,rex_base
	lw	v0,REX_GETTCINFO(v0)
	j	v0
*/
EXPORT(rex_execute_cmd)
	lw	v0,rex_base
	lw	v0,REX_EXECUTE_CMD(v0)
	j	v0

EXPORT(rex_rex)
	lw	v0,rex_base
	lw	v0,REX_REX(v0)
	j	v0

/*
 * Misc. kernel entry points
 */
EXPORT(_coredump)
	j	doadump

EXPORT(_xprdump)
	jal	xprdump
	lw	a0,rex_magicid
	li	a1,0x30464354
	bne	a0,a1,1f
	li	a0,0x68
	j	rex_rex
1:	j	prom_restart

EXPORT(_xprtail)
	jal	xprtail
	lw	a0,rex_magicid
	li	a1,0x30464354
	bne	a0,a1,1f
	li	a0,0x68
	j	rex_rex
1:	j	prom_restart

EXPORT(_msgdump)
#ifndef ultrix
	jal	msgdump
#endif not ultrix
	lw	a0,rex_magicid
	li	a1,0x30464354
	bne	a0,a1,1f
	li	a0,0x68
	j	rex_rex
1:	j	prom_restart

/*
 * do a dump and then perform power-up sequence
 * called on warm starts
 */
EXPORT(doadump)
	li	sp,STARTUP_STACK	# setup temporary dumpstack
	la	gp,_gp			# load the gp
	.set noreorder
	nop
	mtc0	zero,C0_TLBHI
	nop
	li	v0,KPTEBASE
	nop
	mtc0	v0,C0_CTXT
	nop
	.set reorder
	jal	dumpsetupvectors	# load exception vector code
	jal	dumpsys			# do the dump

	lw	a0,rex_magicid
	li	a1,0x30464354
	bne	a0,a1,1f
	li	a0,0x62
	j	rex_rex
1:	j	prom_reboot


/*
 * Deal with tlbmisses in KUSEG
 */
NESTED(utlbmiss, 0, k1)			# Copied down to 0x80000000
	.set	noreorder
 	.set	noat
	mfc0	k0,C0_CTXT
	mfc0	k1,C0_EPC
	lw	k0,0(k0)
	nop
#if NOMEMCACHE==1
	or	k0,PG_N
#endif
	sll	k0,8			# abstract format to R2000/R3000
#if CNT_TLBMISS_HACK==1
	mtc0	k0,C0_TLBLO
	lw	k0,CNT_UTLBMISS
	c0	C0_WRITER
	addu	k0,1
	sw	k0,CNT_UTLBMISS
#else
	mtc0	k0,C0_TLBLO
	nop
	c0	C0_WRITER
#endif CNT_TLBMISS_HACK
	j	k1
	c0	C0_RFE
EXPORT(eutlbmiss)
 	.set	at
	.set	reorder
	END(utlbmiss)

/*
 * General exception entry point.
 */
#ifdef ASM_FIXED
#define	M_EXCEPT	+(M_SP|M_GP|M_AT|M_K1|M_A0|M_A1|M_A2|M_A3|M_S0|M_RA)
#define	M_TFISAVE	+(M_V0|M_V1|M_T0|M_T1|M_T2|M_T3|M_T4|M_T5|M_T6|\
			M_T7|M_T8|M_T9)
#else
#define	M_EXCEPT	0xb80100f3
#define	M_TFISAVE	0x4300ff0d
#define	M_EXCSAVE	0xfb01ffff
#define	M_TRAPSAVE	0xfbffffff
#define	M_SYSCALLSAVE	0xf80100ff
#endif

VECTOR(exception, M_EXCEPT)	# Copied down to 0x80000080
#ifdef notdef
	.set	noreorder
 	.set	noat
	/*
	 * Inline implementation of a fast kernel tlbmiss
	 * WARNING: this is half-baked and incomplete
	 */
	mfc0	k0,C0_CAUSE
	sw	AT,K2_ATSAVE(zero)	# put AT in u area and fill load delay
	and	k0,CAUSE_EXCMASK	# isolate exception cause
	beq	k0,EXC_RMISS,1f		# we can only handle TLB r/w misses
	nop
	bne	k0,EXC_WMISS,longway
1:
	mfc0	k0,C0_CTXT		# pick up pte address and fill BDSLOT
	lw	AT,Sysptsize		# should be a literal but for mas
	/*
	 * The expression in the next instruction requires unsigned
	 * arithmetic (I'll bet it won't work!)
	 */
	subu	k0,KPTEBASE+(K2BASE/PGSIZE*4)	# make Sys pte offset (LDSLOT)
	bgeu	k0,AT,longway		# outside Sysmap
	addu	k0,Sysmap		# offset into Sysmap and fill BDSLOT
	lw	k0,0(k0)		# load desired pte
	mfc0	k1,C0_EPC		# get ready for rfe and fill load delay
	sll	k0,8			# abstract format to R2000/R3000
	mtc0	k0,C0_TLBLO		# drop pte in tlb
	and	k0,TLBLO_V		# check valid bit and fill load delay
	c0	C0_WRITER		# random drop in into tlb
	beq	k0,zero,2f		# panic if not valid
	lw	AT,K2_ATSAVE(zero)	# reload AT
	j	k1
	c0	C0_RFE
	nop
2:	PANIC("invalid kernel pte")
longway:
	lw	AT,K2_ATSAVE(zero)	# reload AT
	move	k0,AT
	.set	at
	.set	reorder
#endif

#ifdef ROWEN_LEDS
	li	k0,0xbe0800c0
	sb	k0,-189(k0)
#endif
	lw	k0,u+PCB_CPUPTR		# get cpudata pointer
	lw	k0,CPU_KSTACK(k0)	# get kstack flag
	beq	k0,zero,1f
	sw	sp,EF_SP*4-EF_SIZE(sp)
	subu	sp,EF_SIZE
	b	2f

	/*
	 * Came from user mode or utlbmiss, initialize kernel stack
	 */
1:	sw	sp,KERNELSTACK-EF_SIZE+EF_SP*4
	la	sp,KERNELSTACK-EF_SIZE
	sw	gp,EF_GP*4(sp)
	la	gp,_gp
	lw	k0,u+PCB_CPUPTR		# get cpudata pointer
	sw	gp,CPU_KSTACK(k0)	# now on kernel stack (gp always != 0)
	/*
	 * This instruction stream can be cleaned up somewhat for write stalls,
	 * but for now, left as is so its readable when debugging
	 */
	.set	noat
2:	sw	AT,EF_AT*4(sp)
	.set	at
	sw	k1,EF_K1*4(sp)		# in case we came from utlbmiss

	.set noreorder
	sw	a0,EF_A0*4(sp)
	sw	a1,EF_A1*4(sp)
	sw	a2,EF_A2*4(sp)
	sw	a3,EF_A3*4(sp)

#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	a0,mips_spl_arch_type
	nop
	beq	a0,zero,1f
	nop
#endif

	lw	a1,ipllevel
	nop
	sw	a1,EF_SYS1*4(sp)
#endif
1:
	mfc0	a3,C0_CAUSE
	mfc0	a0,C0_EPC
	sw	a3,EF_CAUSE*4(sp)
	sw	s0,EF_S0*4(sp)
	sw	ra,EF_RA*4(sp)
	mfc0	s0,C0_SR
	sw	a0,EF_EPC*4(sp)
	.set reorder

#ifdef notdef
#ifdef XPRBUG
	lw	a2,xpr_flags
	and	a2,XPR_INTR
	beq	a2,zero,1f
	jal	tfi_save
	subu	sp,10*4
	la	a0,9f
	lw	a1,EF_CAUSE*4(sp)
	la	a2,cause_desc
	move	a3,s0
	la	v0,sr_desc
	sw	v0,4*4(sp)
	jal	xprintf
	MSG("exception cause=%r sr=%r")
	la	a0,9f
	move	a1,sp
	lw	a2,EF_EPC*4(sp)
	jal	xprintf
	MSG("exception sp=0x%x pc=0x%x")
	addu	sp,10*4
	jal	tfi_restore
	lw	a3,EF_CAUSE*4(sp)
1:
#endif XPRBUG
#endif notdef

	/*
	 * Dispatch to appropriate exception handler
	 * Register setup:
	 *	s0 -- SR register at time of exception
	 *	a0 -- exception frame pointer
	 *	a1 -- cause code
	 *	a3 -- cause register
	 */
	and	a1,a3,CAUSE_EXCMASK
	lw	a2,causevec(a1)
	move	a0,sp
	.set	noreorder
	j	a2
	sw	s0,EF_SR*4(sp)		# should clear PE in s0 after here
	.set	reorder
EXPORT(eexception)
	END(exception)

/*
 * VEC_int -- interrupt handler
 */
#ifdef ASM_FIXED
VECTOR(VEC_int, M_EXCEPT|M_TFISAVE)
#else
VECTOR(VEC_int, M_EXCSAVE)
#endif
	.set noreorder
	li	k0,SR_IEC|SR_IMASK8	# enable, but mask all interrupts
	mtc0	k0,C0_SR
	.set reorder
	jal	tfi_save
	/* For ISIS halt interrupt we want to go directly to the boot rom. */
	lw	v0, cpu			# get cpu
	bne	v0, DS_5800, 1f		# see if 5800
	andi	a2,a3,CAUSE_IP7 	# check cause for halt intr
	beq	a2,zero,1f		# no halt, skip to check for FP intr
	lw	v0,EF_V0*4(sp)		# restore v0 for isis halt path
	jal	prom_halt		# Call rom halt routine
	.set noreorder
	nop
	b	2f			# Let's get outta here.
	nop
	.set reorder
	/*
	 * If this is a floating-point interrupt then we may need "all" the
	 * user's register values in case we need to emulate an branch
	 * instruction if we are in a branch delay slot.
	 *
	 * For pmax, if IP5 of the cause register is set, then it must
 	 * be a FPU interrupt.  For mipsfair, and isis, if IP4 of the
	 * cause interrupt is set, then it may be a FPU interrupt, it may
	 * also be other hard error interrupts.  We save s1-s8 if IP5 or
	 * IP4 is set just in case that the interrupt is a FPU interrupt.
	 */
1:
	andi	a2,a3,CAUSE_IP8		# check cause for possible fp intr
	beq	a2,zero,1f
	sw	s1,EF_S1*4(sp)
	sw	s2,EF_S2*4(sp)
	sw	s3,EF_S3*4(sp)
	sw	s4,EF_S4*4(sp)
	sw	s5,EF_S5*4(sp)
	sw	s6,EF_S6*4(sp)
	sw	s7,EF_S7*4(sp)
	sw	s8,EF_S8*4(sp)
1:

#if defined(DS5000_100) && defined(MIPS_ARCH_SPL_ORIG)
	lw	a2,mips_spl_arch_type
	beq	a2,zero,1f

	move	a2,s0			# sr is arg3
        jal     kn02ba_intr
	b	2f
1:
	move	a2,s0			# sr is arg3
	jal	kn01_intr
#else
#if defined(DS5000_100)
	move	a2,s0			# sr is arg3
	jal	kn02ba_intr
#else
	move	a2,s0			# sr is arg3
	jal	kn01_intr
#endif
#endif
2:
	jal	tfi_restore
	.set noreorder
	mtc0	s0,C0_SR		# disable interrupts
	.set reorder
	b	exception_exit
	END(VEC_int)

/*
 * TLB mod.
 * Could enable interrupts here if we were so inclined....
 */
#ifdef ASM_FIXED
VECTOR(VEC_tlbmod, M_EXCEPT|M_TFISAVE)
#else
VECTOR(VEC_tlbmod, M_EXCSAVE)
#endif
	.set noreorder
	mfc0	a2,C0_BADVADDR		# arg3 is bad vaddr
	nop
	.set reorder
	sw	a2,EF_BADVADDR*4(sp)	# save in case of trap (ugh!)
	jal	tfi_save
	jal	tlbmod			# tlbmod(ef_ptr, code, vaddr, cause)
#ifdef DS5000_100
	.set noreorder
	mtc0	s0, C0_SR		# restore status register
	.set reorder
#endif
	la	ra,exception_exit	# fake jal with less nops
					# if we had reloc-reloc, 1 cycle
	beq	v0,zero,tfi_restore	# zero if legal to modify
	or	a0,s0,SR_IEC		# enable interrupts
	.set noreorder
	mtc0	a0,C0_SR
	.set reorder
	move	a0,sp			# restore ep since tlbmod can trash
	move	a1,v0			# move software exception code
	lw	a3,EF_CAUSE*4(sp)	# restore cause since tlbmod can trash
	b	soft_trap		# and handle as trap
	END(VEC_tlbmod)

/*
 * TLB miss. 
 * Handles TLBMiss Read and TLBMiss Write
 * Could enable interrupts here if we were so inclined....
 */
#ifdef ASM_FIXED
VECTOR(VEC_tlbmiss, M_EXCEPT|M_TFISAVE)
#else
VECTOR(VEC_tlbmiss, M_EXCSAVE)
#endif
	.set noreorder
	mfc0	a2,C0_BADVADDR		# arg3 is bad vaddr
	nop
	.set reorder
	sw	a2,EF_BADVADDR*4(sp)	# save in case of trap (ugh!)
	jal	tfi_save
	jal	tlbmiss			# tlbmiss(ef_ptr, code, vaddr, cause)
	lw	s0,EF_SR*4(sp)		# tlbmiss can alter return SR
#ifdef DS5000_100
	.set noreorder
	nop
	mtc0	s0, C0_SR		# restore status register
	.set reorder
#endif
	beq	v0,zero,1f		# zero if accessable
	or	a0,s0,SR_IEC		# enable interrupts
	.set noreorder
	mtc0	a0,C0_SR
	.set reorder
	move	a0,sp			# restore ep since tlbmiss can trash
	move	a1,v0			# software exception code
	lw	a3,EF_CAUSE*4(sp)	# restore cause since tlbmiss can trash
	b	soft_trap		# handle as trap

1:	la	ra,exception_exit	# 2 cycles, but 1 fills delay slot
	b	tfi_restore
	END(VEC_tlbmiss)

/*
 * VEC_addrerr
 * Handles AdrErrRead, AdrErrWrite
 */
VECTOR(VEC_addrerr, M_EXCEPT)
	.set noreorder
	nop
	mfc0	a2,C0_BADVADDR
	nop
	.set reorder
	sw	a2,EF_BADVADDR*4(sp)
	b	VEC_trap
	END(VEC_addrerr)

#define KN02ERR_ADDR	0xbfd80000	/* KSEG1 addr of kn02 Error register */
#define KN02CHKSYN_ADDR	0xbfd00000	/* KSEG1 addr of kn02 check/syn reg */

/*
 * VEC_ibe
 * Handles Instruction Bus Errors
 */
VECTOR(VEC_ibe, M_EXCEPT)
	.set noreorder
	nop
	mfc0	a2,C0_EPC
	nop
	mfc0	ra,C0_CAUSE
	nop
	.set reorder
	bgez	ra,1f		# BD bit not set
	addu	a2,4		# point at BD slot
1:	sw	a2,EF_BADVADDR*4(sp) # ibe's occur at pc
	/*
	 * Work around for DS5000 (3max).  We must get a soft copy of
	 * the "erradr" reg, then clear the "erradr" reg to dismiss the
	 * pending interrupt.  We "know" that "a2" and "ra" are safe to
	 * use here as temp registers.
	 */
	lw      a2, cpu                 # get system type
	bne     a2, DS_5000, VEC_trap	# see if 5000
	la	a2, KN02ERR_ADDR	# get addr of hardware erradr reg
	lw      ra, (a2)		# get contents of hardware erradr reg
	sw      ra, kn02erradr		# and save a software copy
	sw      zero, (a2)		# clear hardware erradr to clear intr
	la	a2, KN02CHKSYN_ADDR	# get addr of hardware chksyn reg
	lw      ra, (a2)		# get contents of hardware chksyn reg
	sw      ra, kn02chksyn		# and save a software copy
					# latency absorbed in 3 prior instructs
	jal     kn01wbflush		# wait for erradr write
	b	VEC_trap
	END(VEC_ibe)

/*
 * VEC_dbe
 * Handles Data Bus Errors
 *
 * Trap will calculate appropriate badvaddr
 */
VECTOR(VEC_dbe, M_EXCEPT)
	/*
	 * Work around for DS5000 (3max).  We must get a soft copy of
	 * the "erradr" reg, then clear the "erradr" reg to dismiss the
	 * pending interrupt.  We "know" that "a2" and "ra" are safe to
	 * use here as temp registers.
	 */
	lw      a2, cpu			# get system type
	bne     a2, DS_5000, VEC_trap	# see if 5000 (3max)
	la	a2, KN02ERR_ADDR	# get addr of hardware erradr reg
	lw      ra, (a2)		# get contents of hardware erradr reg
	sw      ra, kn02erradr		# and save a software copy
	sw      zero, (a2)		# clear hardware erradr to clear intr
	la	a2, KN02CHKSYN_ADDR	# get addr of hardware chksyn reg
	lw      ra, (a2)		# get contents of hardware chksyn reg
	sw      ra, kn02chksyn		# and save a software copy
					# latency absorbed in 3 prior instructs
	jal     kn01wbflush		# wait for erradr write
	b	VEC_trap
	END(VEC_dbe)

/*
 * TRAP
 * Illegal Instruction, and Overflow.
 * Also handles software exceptions raised by tlbmod and tlbmiss,
 * NOTE: tlbmod and tlbmiss replace the original exception code with
 * an appropriate software exception code.
 */
#define	M_TRAP		+(M_S1|M_S2|M_S3|M_S4|M_S5|M_S6|M_S7)
#ifdef ASM_FIXED
VECTOR(VEC_trap, M_EXCEPT|M_TFISAVE|M_TRAP)
#else
VECTOR(VEC_trap, M_TRAPSAVE)
#endif
	.set noreorder
	or	a2,s0,SR_IEC		# enable interrupts
	mtc0	a2,C0_SR
	.set reorder
	jal	tfi_save
soft_trap:				# (from tlbmod / tlbmiss)
	/*
	 * Save rest of state for debuggers
	 * ENTRY CONDITIONS: interrupts enabled, a1 contains software
	 * exception code
	 */
	sw	s1,EF_S1*4(sp)
	sw	s2,EF_S2*4(sp)
	sw	s3,EF_S3*4(sp)
	move	a2,s0
	sw	s4,EF_S4*4(sp)
	sw	s5,EF_S5*4(sp)
	sw	s6,EF_S6*4(sp)
	sw	s7,EF_S7*4(sp)
	sw	s8,EF_S8*4(sp)
	jal	trap			# trap(ef_ptr, code, sr, cause)
full_restore:
	lw	s1,EF_S1*4(sp)
	lw	s2,EF_S2*4(sp)
	lw	s3,EF_S3*4(sp)
	lw	s4,EF_S4*4(sp)
	lw	s5,EF_S5*4(sp)
	lw	s6,EF_S6*4(sp)
	lw	s7,EF_S7*4(sp)
	lw	s8,EF_S8*4(sp)
	jal	tfi_restore
	.set noreorder
	mtc0	s0,C0_SR		# disable interrupts
	.set reorder
	b	exception_exit
	END(VEC_trap)

/*
 * VEC_nofault -- handles nofault exceptions early on in system initialization
 * before VEC_trap is usable.
 */
#ifdef ASM_FIXED
VECTOR(VEC_nofault, M_EXCEPT|M_TFISAVE)
#else
VECTOR(VEC_nofault, M_EXCSAVE)
#endif
	jal	tfi_save
	move	a2,s0
	jal	trap_nofault		# trap_nofault(ef_ptr, code, sr, cause)
	jal	tfi_restore

#ifdef DS5000_100
	.set noreorder
	mtc0	s0, C0_SR		# restore status register
	.set reorder
#endif

	b	exception_exit
	END(VEC_nofault)

/*
 * Syscall
 * NOTE: v0, and, v1 must get restored on exit from syscall!!
 */
#define	M_SYSCALL	+(M_V0|M_V1)
#ifdef ASM_FIXED
VECTOR(VEC_syscall, M_EXCEPT|M_SYSCALL)
#else
VECTOR(VEC_syscall, M_SYSCALLSAVE)
#endif
	.set noreorder
	or	a1,s0,SR_IEC		# enable interrupts
	mtc0	a1,C0_SR
	.set reorder
	sw	v0,EF_V0*4(sp)		# u_rval1
	sw	v1,EF_V1*4(sp)		# u_rval2
	move	a1,v0			# arg2 -- syscall number
	move	a2,s0			# arg3 -- sr
	jal	syscall			# syscall(ef_ptr, sysnum, sr, cause)
	bne	v0,zero,full_restore	# doing a sigreturn
	lw	v0,EF_V0*4(sp)		# u_rval1
	lw	v1,EF_V1*4(sp)		# u_rval2
	.set noreorder
	mtc0	s0,C0_SR		# disable interrupts
	.set reorder
	b	exception_exit
	END(VEC_syscall)

/*
 * Breakpoint -- determine if breakpoint is for prom monitor, else
 * call trap.
 */
VECTOR(VEC_breakpoint, M_EXCEPT)
	.set noreorder
	nop
	mfc0	k1,C0_CAUSE
	nop
	.set reorder
	lw	k0,EF_EPC*4(sp)
	and	k1,CAUSE_BD
	beq	k1,zero,1f
	addu	k0,4				# advance pc to bdslot
1:	lw	k0,0(k0)			# read faulting instruction
	and	k1,s0,SR_KUP			# if from use mode
	bne	k1,zero,2f			# kernel break not allowed
	lw	k1,kernelbp			# what a kernel bp looks like
	bne	k0,k1,2f			# not a kernel bp inst
	lw	k0,+RB_BPADDR			# address of breakpoint handler
	bne	k0,zero,4f
2:
	/*
	 * Check to see if there is a branch delay slot emulation taking place
	 * which is indicated by a non-zero value in PCB_BD_RA (left there by
	 * emulate_instr() ).  If this is the case go on to check for the two
	 * possible break instructions that emulate_instr() laid down.  If it
	 * is one of those two break instructions set the resulting pc and
	 * branch back to the caller of emulate_instr().  See emulate_instr()
	 * for the interface of how and where all this happens.
	 */
	lw	a2,u+PCB_BD_RA
	beq	a2,zero,VEC_trap	# handle as a trap
	lw	k1,bd_nottaken_bp	# check for the not taken branch bp
	bne	k0,k1,3f
	or	a3,s0,SR_IEC		# enable interrupts
	.set noreorder
	nop
	mtc0	a3,C0_SR
	nop
	.set reorder
	sw	zero,u+PCB_BD_RA	# clear the branch delay emulation
	lw	a3,u+PCB_BD_EPC		# the resulting pc in this case is just
	addu	a3,8			#  the pc of the next instruction after
					#  delay slot
	j	a2			# return to caller of emulate_instr()

bd_nottaken_bp:
	break	BRK_BD_NOTTAKEN

3:	lw	k1,bd_taken_bp		# check for the taken branch bp
	bne	k0,k1,VEC_trap		# handle as a trap
	or	a3,s0,SR_IEC		# enable interrupts
	.set noreorder
	nop
	mtc0	a3,C0_SR
	nop
	.set reorder
	sw	zero,u+PCB_BD_RA	# clear the branch delay emulation
	lw	a3,u+PCB_BD_EPC		# the resulting pc in this case is the
	lw	a1,u+PCB_BD_INSTR	#  the target of the emulated branch
	sll	a1,16			#  so add the sign extended offset to
	sra	a1,16-2			#  branch's pc for the resulting pc
	addu	a3,a1
	addu	a3,4
	j	a2			# return to caller of emulate_instr()

bd_taken_bp:
	break	BRK_BD_TAKEN

4:	and	k0,s0,SR_KUP
	beq	k0,zero,5f		# breakpoint from kernel mode
	lw	k0,u+PCB_CPUPTR		# get cpudata pointer
	sw	zero,CPU_KSTACK(k0)	
	lw	gp,EF_GP*4(sp)
5:	lw	a0,EF_A0*4(sp)
	lw	a1,EF_A1*4(sp)
	lw	a2,EF_A2*4(sp)
	lw	a3,EF_A3*4(sp)
	lw	s0,EF_S0*4(sp)
	lw	ra,EF_RA*4(sp)
	lw	k1,EF_SR*4(sp)
	.set noreorder
	nop
	mtc0	k1,C0_SR
	nop
	.set reorder
	lw	k1,EF_K1*4(sp)
	lw	k0,EF_AT*4(sp)		# save AT in k0
	lw	sp,EF_SP*4(sp)

	.set	noat
	lw	AT,+RB_BPADDR		# address of breakpoint handler
	j	AT			# enter breakpoint handler
	.set	at

kernelbp:
	break	BRK_KERNELBP
	END(VEC_breakpoint)

EXPORT(sstepbp)
	break	BRK_SSTEPBP

/*
 * Coprocessor unusable fault
 */
VECTOR(VEC_cpfault, M_EXCSAVE)
	.set noreorder
	or	a1,s0,SR_IEC		# enable interrupts
	mtc0	a1,C0_SR
	.set reorder

	and	a1,s0,SR_KUP
	beq	a1,zero,coproc_panic	# kernel tried to use coprocessor

	and	a1,a3,CAUSE_CEMASK
	srl	a1,CAUSE_CESHIFT
	bne	a1,1,coproc_not1	# not coproc 1

#ifdef ASSERTIONS
	and	a1,s0,SR_IBIT6
	bne	a1,zero,1f		# fp interrupts must be enabled!
	PANIC("VEC_cpfault")
1:
#endif ASSERTIONS

	/*
	 * This is the floating-point coprocessor (coprocessor 1) unusable
	 * fault handling code.  During auto configuration fptype_word
	 * is loaded from the floating-point coprocessor revision word or
	 * zeroed if there is no floating-point coprocessor.
	 */
	sw	gp,u+PCB_OWNEDFP	# mark that fp has been touched
	lw	a2,fptype_word		# check for what type of fp coproc
	bne	a2,zero,1f
	j	softfp_unusable		# no fp coproc (goto fp software)
1:
	or	a1,s0,SR_CU1		# enable coproc 1 for the user process
	sw	a1,EF_SR*4(sp)

	lw	a2,u+PCB_CPUPTR		# get cpudata pointer
	lw	a2,CPU_FPOWNER(a2)	# current coproc 1 (fp) owner
	lw	a1,u+U_PROCP		# current process executing
	beq	a2,a1,coproc_done	# owned by the current process

	or	a3,s0,SR_CU1|SR_IEC	# enable fp and interrupts
	.set noreorder
	nop
	mtc0	a3,C0_SR
	nop
	.set reorder
	beq	a2,zero,fp_notowned	# coproc 1 not currently owned

	/*
	 * Owned by someone other than the current process.
	 * Save state (into the fpowner) before taking possession.
	 */
#ifdef ASSERTIONS
	lw	a3,P_SCHED(a2)
	and	a3,SLOAD
	bne	a3,zero,1f
	PANIC("VEC_cpfault swapped out")
1:
#endif ASSERTIONS
	lw	a3,P_ADDR(a2)		# address of u page ptes
	lw	a3,0(a3)		# u page pte
	and	a3,PG_PFNUM		# isolate physical address of u page
	sll	a3,8			# abstract format to R2000/R3000
	or	a3,K0BASE		# change to virtual address

#define SAVECP1REG(reg) \
	swc1	$f/**/reg,PCB_FPREGS+reg*4(a3)

	/*
	 * The floating-point control and status register must be
	 * read first to force all fp operations to complete and insure
	 * that all fp interrupts for this process have been delivered
	 */
	.set	noreorder
	cfc1	a2,fpc_csr
	nop
	sw	a2,PCB_FPC_CSR(a3)
	cfc1	a2,fpc_eir
	nop
	sw	a2,PCB_FPC_EIR(a3)
	SAVECP1REG(31); SAVECP1REG(30); SAVECP1REG(29); SAVECP1REG(28)
	SAVECP1REG(27); SAVECP1REG(26); SAVECP1REG(25); SAVECP1REG(24)
	SAVECP1REG(23); SAVECP1REG(22); SAVECP1REG(21); SAVECP1REG(20)
	SAVECP1REG(19); SAVECP1REG(18); SAVECP1REG(17); SAVECP1REG(16)
	SAVECP1REG(15); SAVECP1REG(14); SAVECP1REG(13); SAVECP1REG(12)
	SAVECP1REG(11); SAVECP1REG(10); SAVECP1REG(9);  SAVECP1REG(8)
	SAVECP1REG(7);  SAVECP1REG(6);  SAVECP1REG(5);  SAVECP1REG(4)
	SAVECP1REG(3);  SAVECP1REG(2);  SAVECP1REG(1);  SAVECP1REG(0)

	.set	reorder
	/* Make the process affinity ALLCPU  */
	lw	a2,u+PCB_CPUPTR		# get cpudata pointer
	lw	a2,CPU_FPOWNER(a2)	# current coproc 1 (fp) owner
	la	a3,ALLCPU		# affinity of ALLCPU
	sw	a3,P_AFFINITY(a2)	# change affinity of currrent fpowner
					# to all cpus. What if there was 
					# some other affinity restriction 
					# before??
	lw	a2,smp			# if smp defined
	beq	a2,zero,fp_notowned	#
	jal	tfi_save
	jal	alert_cpu		# inform every processor
	jal	tfi_restore
	lw	a1,u+U_PROCP		# restore current process in a1
fp_notowned:
	/*
	 * restore coprocessor state (from the current process)
	 */
	.set	noreorder
	li	a3,u

#define RESTCP1REG(reg) \
	lwc1	$f/**/reg,PCB_FPREGS+reg*4(a3)

	or	a2,s0,SR_CU1
	mtc0	a2,C0_SR		# disable interrupts, fp enabled
	nop				# before we can really use cp1
	nop				# before we can really use cp1
	RESTCP1REG(0);  RESTCP1REG(1);  RESTCP1REG(2);  RESTCP1REG(3)
	RESTCP1REG(4);  RESTCP1REG(5);  RESTCP1REG(6);  RESTCP1REG(7)
	RESTCP1REG(8);  RESTCP1REG(9);  RESTCP1REG(10); RESTCP1REG(11)
	RESTCP1REG(12); RESTCP1REG(13); RESTCP1REG(14); RESTCP1REG(15) 
	RESTCP1REG(16); RESTCP1REG(17); RESTCP1REG(18); RESTCP1REG(19)
	RESTCP1REG(20); RESTCP1REG(21); RESTCP1REG(22); RESTCP1REG(23)
	RESTCP1REG(24); RESTCP1REG(25); RESTCP1REG(26); RESTCP1REG(27)
	RESTCP1REG(28); RESTCP1REG(29); RESTCP1REG(30); RESTCP1REG(31)
	ctc1	zero,fpc_csr
	lw	a2,PCB_FPC_EIR(a3)
	nop
	ctc1	a2,fpc_eir
	lw	a2,PCB_FPC_CSR(a3)
	nop
	ctc1	a2,fpc_csr
	nop
	lw	a2,u+PCB_CPUPTR		# get cpudata pointer
	nop
	sw	a1,CPU_FPOWNER(a2)	# we now own fp
	lw	a2,CPU_MASK(a2)		# cpu mask for this cpu 
	nop
	sw	a2,P_AFFINITY(a1)	# change the process's affinity to 
					# the current cpu
	mtc0	s0,C0_SR		# disable interrupt and clear SR_CU1
	.set	reorder
	b	exception_exit

coproc_done:
	.set noreorder
	mtc0	s0,C0_SR		# disable interrupts
	.set reorder
	b	exception_exit

coproc_not1:
	li	a1,SEXC_CPU		# handle as software trap
	b	VEC_trap		# not soft_trap, must save regs yet

coproc_panic:
	PANIC("kernel used coprocessor")
	END(VEC_cpfault)

/*
 * checkfp(procp, exiting)
 *	procp = proc pointer of process exiting or being swapped out.
 *	exiting = 1 if exiting.
 *	Called from exit and swapout to release FP ownership.
 */
LEAF(checkfp)
	lw	v0,u+PCB_CPUPTR		# get cpudata pointer
	lw	v0,CPU_FPOWNER(v0)	# current coproc 1 (fp) owner
	bne	a0,v0,2f		# not owned by us, just return
	bne	a1,zero,1f		# exiting, don't save state
	lw	a3,fptype_word
	beq	a3,zero,1f		# no fp coprocessor
	lw	a3,P_ADDR(a0)		# address of u page ptes
	lw	a3,0(a3)		# u page pte
	and	a3,PG_PFNUM		# isolate physical address of u page
	sll	a3,8			# abstract format to R2000/R3000
	or	a3,K0BASE		# change to virtual address

	/*
	 * The floating-point control and status register must be
	 * read first so to stop the floating-point coprocessor.
	 */
	.set	noreorder
	mfc0	v1,C0_SR		# enable coproc 1 for the kernel
	nop
	or	v0,v1,SR_CU1		
	mtc0	v0,C0_SR		# PE BIT
	nop				# before we can really use cp1
	nop				# before we can really use cp1
	cfc1	v0,fpc_csr
	nop
	sw	v0,PCB_FPC_CSR(a3)
	cfc1	v0,fpc_eir
	nop
	sw	v0,PCB_FPC_EIR(a3)
	SAVECP1REG(31); SAVECP1REG(30); SAVECP1REG(29); SAVECP1REG(28)
	SAVECP1REG(27); SAVECP1REG(26); SAVECP1REG(25); SAVECP1REG(24)
	SAVECP1REG(23); SAVECP1REG(22); SAVECP1REG(21); SAVECP1REG(20)
	SAVECP1REG(19); SAVECP1REG(18); SAVECP1REG(17); SAVECP1REG(16)
	SAVECP1REG(15); SAVECP1REG(14); SAVECP1REG(13); SAVECP1REG(12)
	SAVECP1REG(11); SAVECP1REG(10); SAVECP1REG(9);  SAVECP1REG(8)
	SAVECP1REG(7);  SAVECP1REG(6);  SAVECP1REG(5);  SAVECP1REG(4)
	SAVECP1REG(3);  SAVECP1REG(2);  SAVECP1REG(1);  SAVECP1REG(0)
	ctc1	zero,fpc_csr		# clear any pending interrupts
	mtc0	v1,C0_SR		# disable kernel fp access
	nop
	.set	reorder

1:	lw	v0,u+PCB_CPUPTR		# get cpudata pointer
	sw	zero,CPU_FPOWNER(v0)	# Mark FP as unowned

	lw	a1,CPU_FPE_EVENT(v0)	# if psiganl call pending
	bne	a1,zero,3f		# then don't change affinity.  this
					# will be done after psignal is called
					# in softnet.

	la	a1,ALLCPU		# change affinity to ALLCPU
	sw	a1,P_AFFINITY(a0)
3:
	lw	a1,u+U_PROCP
	bne	a1,a0,2f		# not current process
	lw	a1,KERNELSTACK-EF_SIZE+(4*EF_SR) # current user's sr
	and	a1,~SR_CU1		# clear fp coprocessor usable bit
	sw	a1,KERNELSTACK-EF_SIZE+(4*EF_SR)

2:	j	ra
	END(checkfp)

/*
 * tfi_save -- save enough state so that C routines can be called
 */
LEAF(tfi_save)
	sw	v0,EF_V0*4(sp)
	sw	v1,EF_V1*4(sp)
	sw	t0,EF_T0*4(sp)
	mflo	t0
	sw	t1,EF_T1*4(sp)
	mfhi	t1
	sw	t2,EF_T2*4(sp)
	sw	t3,EF_T3*4(sp)
	sw	t4,EF_T4*4(sp)
	sw	t5,EF_T5*4(sp)
	sw	t6,EF_T6*4(sp)
	sw	t7,EF_T7*4(sp)
	sw	t8,EF_T8*4(sp)
	sw	t9,EF_T9*4(sp)
	sw	t0,EF_MDLO*4(sp)
	sw	t1,EF_MDHI*4(sp)
	j	ra
	END(tfi_save)

/*
 * tfi_restore -- restore state saved by tfi_save
 */
LEAF(tfi_restore)
	lw	v0,EF_MDLO*4(sp)
	lw	v1,EF_MDHI*4(sp)
	mtlo	v0
	mthi	v1
	lw	v0,EF_V0*4(sp)
	lw	v1,EF_V1*4(sp)
	lw	t0,EF_T0*4(sp)
	lw	t1,EF_T1*4(sp)
	lw	t2,EF_T2*4(sp)
	lw	t3,EF_T3*4(sp)
	lw	t4,EF_T4*4(sp)
	lw	t5,EF_T5*4(sp)
	lw	t6,EF_T6*4(sp)
	lw	t7,EF_T7*4(sp)
	lw	t8,EF_T8*4(sp)
	lw	t9,EF_T9*4(sp)
	j	ra
	END(tfi_restore)

/*
 * End of exception processing.  Interrupts should be disabled.
 */
VECTOR(exception_exit, M_EXCEPT)
	/*
	 * ENTRY CONDITIONS:
	 *	Interrupts Disabled
	 *	s0 contains sr at time of exception
	 *
	 * If we are returning to user mode, check to see if a resched is
	 * desired.  If so, fake a RESCHED cause bit and let trap save/restore
	 * our state for us.
	 */
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	a0,mips_spl_arch_type
	beq	a0,zero,1f
#endif
	.set noreorder
	lw	a0, EF_SYS1*4(sp)
	lw	a3, EF_SR*4(sp)		# get SR from exception frame
	sw	a0, ipllevel
	sll	a0, a0, 2		# multiply by 4
	lw	a1, kn02ba_sim(a0)	# get system interrupt mask value
	lw	a2, splm(a0)		# get status register mask value
	sw	a1, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a1, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	andi	a2, a2, 0xff00		# get interrupt mask bits only
	li	k0, 0xffff00ff
	and	a3, a3, k0		# turn off all mask bits
	or	a2, a2, a3		# or in new mask bits
	sw	a2, EF_SR*4(sp)		# restore SR to exception frame
	.set reorder

1:	
#endif
	and	k0,s0,SR_KUP
	beq	k0,zero,2f			# returning to kernel mode
	lw	k0,u+PCB_RESCHED
	beq	k0,zero,1f			# no resched requested
	move	a0,sp
	li	a1,SEXC_RESCHED			# software exception
	lw	a3,EF_CAUSE*4(sp)
	b	VEC_trap

1:	lw	k0,u+PCB_CPUPTR		# get cpudata pointer
	sw	zero,CPU_KSTACK(k0)
	lw	gp,EF_GP*4(sp)
2:	lw	a0,EF_A0*4(sp)
	lw	a1,EF_A1*4(sp)
	lw	a2,EF_A2*4(sp)
	lw	a3,EF_A3*4(sp)
	lw	s0,EF_S0*4(sp)
	lw	ra,EF_RA*4(sp)

#ifdef ROWEN_LEDS
	li	k0,0xbe0800c1
	sb	k0,-190(k0)
#endif

	lw	k0,EF_EPC*4(sp)
	lw	k1,EF_SR*4(sp)
	.set noreorder
 	.set	noat
	lw	AT,EF_AT*4(sp)
	mtc0	k1,C0_SR			# PE BIT
	lw	sp,EF_SP*4(sp)
	j	k0
	c0	C0_RFE
 	.set	at
	.set	reorder
	END(exception_exit)

VECTOR(VEC_unexp, M_EXCEPT)
	PANIC("unexpected exception")
	END(VEC_unexp)

/*
 * Primitives
 */ 

LEAF(clearcpe)
	.set	noreorder
	mfc0	t0,C0_SR		# get the Status Reg
	nop
	or	t1,t0,SR_PE		# OR in the Parity Err Bit to clear it
	mtc0	t1,C0_SR		# write back the status reg
	j	ra
	 nop
	.set	reorder
	END(clearcpe)

#ifdef oldmips
/*
 * Interrupts: (8, 1,2 soft ints, 3-8 hard ints)
 * 8	Bus error/timeout/sec/ded
 * 7	profiling clock.
 * 6	fp interrupt
 * 5	sched clock.
 * 4	uart.
 * 3	vectored devices
 * 2	softnet
 * 1	softclock
 */
#else ultrix
/* ALERT- REMOVE splcons() and spl0() if we can so we have ONE convention */
/*
 * The following is true for PMAX:
 * Interrupts: (8, 1,2 soft ints, 3-8 hard ints)
 * 8	fpu interrupt		splfpu()|splhigh()
 * 7	memory/video		splmem()
 * 6	scheduling clock	splclock()
 * 5	dz			spltty()|splcons()
 * 4	lance			splimp()
 * 3	sii			splbio()
 * 2	softnet			splnet()
 * 1	softclock		splsoftclock()
 * 0	none			splnone()|spl0()
 *
 * In order to change the order you just have to change the defines 
 * in cpu.h and these LEAF routines (and hope the rest of the code works).
 *
 * MIPSFAIR uses a different interrupt scheme.  To allow these splxxx
 * routine to do the right thing on different systems, the proper interrupt
 * masks are initialized at system startup time.  Here we just fetch the
 * proper mask.
 */
#endif ultrix

IMPORT(splm,SPLMSIZE*4)
IMPORT(cpu,4)


#ifdef DS5000_100
IMPORT(splm,SPLMSIZE*4)
IMPORT(kn02ba_sim,SPLMSIZE*4)
#endif
/*
 * spl0: Don't block against anything.
 * This should work on all machines.
 */
LEAF(spl0)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,spl0_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLNONE
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLNONE*4# get system interrupt mask value
	lw	v1, splm+SPLNONE*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
spl0_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	
	.set	noreorder
	mfc0	v0,C0_SR
	li	v1,SR_IEC|SR_IMASK0
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(spl0)

/*
 * splnone: Don't block against anything.
 * This should work on all machines.
 */
LEAF(splnone)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splnone_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLNONE
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLNONE*4# get system interrupt mask value
	lw	v1, splm+SPLNONE*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
splnone_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	mfc0	v0,C0_SR
	li	v1,SR_IEC|SR_IMASK0
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splnone)

/*
 * splsoftclock: block against clock software interrupts (level 1 softint).
 */
LEAF(splsoftclock)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splsoftclock_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLSOFTC
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLSOFTC*4 # get system interrupt mask value
	lw	v1, splm+SPLSOFTC*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
splsoftclock_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPLSOFTC*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splsoftclock)

/*
 * splnet: block against network software interrupts (level 2 softint).
 */
LEAF(splnet)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splnet_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLNET
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLNET*4	# get system interrupt mask value
	lw	v1, splm+SPLNET*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
splnet_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPLNET*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splnet)

/*
 * splbio: block against all I/O device interrupts. all are vme
 */
LEAF(splbio)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splbio_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLIO
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLIO*4	# get system interrupt mask value
	lw	v1, splm+SPLIO*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
splbio_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPLBIO*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splbio)

/*
 * splimp: block against network device interrupts
 * NOTE: the vax version of this routine blocks hardclocks.
 */
LEAF(splimp)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splimp_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLIO
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLIO*4	# get system interrupt mask value
	lw	v1, splm+SPLIO*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
splimp_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPLIMP*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splimp)


/*
 * spltty: block against tty device interrupts. console uart and vme
 */
LEAF(spltty)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,spltty_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLIO
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLIO*4	# get system interrupt mask value
	lw	v1, splm+SPLIO*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
spltty_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPLTTY*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(spltty)

/*
 * splcons: console output (same as tty for ULTRIX)
 */
LEAF(splcons)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splcons_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLIO
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLIO*4	# get system interrupt mask value
	lw	v1, splm+SPLIO*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
splcons_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPLCONS*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splcons)

/*
 * splclock: block against sched clock interrupts.
 */
LEAF(splclock)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splclock_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLCLOCK
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLCLOCK*4 # get system interrupt mask value
	lw	v1, splm+SPLCLOCK*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
splclock_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPLCLOCK*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splclock)

/*
 * splmem: block against memory error interrupts
 */
LEAF(splmem)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splmem_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLMEM
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLMEM*4	# get system interrupt mask value
	lw	v1, splm+SPLMEM*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
splmem_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPLMEM*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splmem)

/*
 * splfpu: block against fpu interrupts
 */
LEAF(splfpu)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splfpu_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLEXTREME
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLEXTREME*4# get system interrupt mask value
	lw	v1, splm+SPLEXTREME*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
splfpu_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPLFPU*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splfpu)

/*
 * splhigh: block against all device interrupts and clock interrupts.
 */
LEAF(splhigh)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splhigh_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLCLOCK
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLCLOCK*4 # get system interrupt mask value
	lw	v1, splm+SPLCLOCK*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
splhigh_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPLHIGH*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splhigh)

/*
 * spl6: block against all device interrupts and clock interrupts.
 */
LEAF(spl6)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,spl6_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLCLOCK
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLCLOCK*4 # get system interrupt mask value
	lw	v1, splm+SPLCLOCK*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
spl6_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPL6*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(spl6)

/*
 * spl7: block against all interrupts except HALT interrupts, which is
 * never blocked.
 */
LEAF(spl7)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,spl7_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLEXTREME
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLEXTREME*4# get system interrupt mask value
	lw	v1, splm+SPLEXTREME*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
spl7_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPL7*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(spl7)


/*
 * splextreme: block against all interrupts except HALT interrupts, which is
 * never blocked.
 */
LEAF(splextreme)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splextreme_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK	# disable interrupts
	mtc0	v1, C0_SR
	nop
	lw	v0, ipllevel		# load return value with current ipl
	li	v1, SPLEXTREME
	sw	v1, ipllevel		# store new ipl into ipllevel
	lw	a0, kn02ba_sim+SPLEXTREME*4# get system interrupt mask value
	lw	v1, splm+SPLEXTREME*4	# get status register mask value
	sw	a0, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	a0, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	v1, C0_SR		# load status register with value
	j	ra
	nop
	.set	reorder
splextreme_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	lw	v1,splm+SPLEXTREME*4
	mfc0	v0,C0_SR
	mtc0	v1,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splextreme)

/*
 * splx(ipl) -- restore previously saved ipl
 */
LEAF(splx)
#ifdef DS5000_100
#ifdef MIPS_ARCH_SPL_ORIG
	lw	v0,mips_spl_arch_type
	beq	v0,zero,splx_kn01
#endif
	.set	noreorder
	li	v1, KN02BA_SPL_MASK
	mtc0	v1, C0_SR		# disable interrupts
	nop
	lw	v0, ipllevel		# load return value with current ipl
	sll	a1, a0, 2		# multiply by 4
	lw	v1, kn02ba_sim(a1)	# get system interrupt mask value
	sw	a0, ipllevel		# store new ipl into ipllevel
	lw	a2, splm(a1)		# get status register mask value
	sw	v1, KN02BA_SIRM_K1ADDR	# load mask register with value
	lw	v1, KN02BA_SIRM_K1ADDR	# reread address to flush write buffer
	mtc0	a2, C0_SR		# load status register with new mask
	j	ra
	nop
	.set	reorder
splx_kn01:
#endif
#ifdef MIPS_ARCH_SPL_ORIG
	.set	noreorder
	mfc0	v0,C0_SR
/* Temporary fix to avoid caller setting BEV bit in status register */
	li	t0,0xffbfffff	# Get the BEV bit
	and	a0,t0		# Clear BEV bit
	nop
	mtc0	a0,C0_SR
	j	ra
	nop
	.set	reorder
#endif
	END(splx)




/*
 * get_cause: get current value of cause register
 */
LEAF(get_cause)
	.set noreorder
	nop
	mfc0	v0,C0_CAUSE
	nop
	.set reorder
	j	ra
	END(get_cause)

/*
 * get_status_reg: get current value of cause register
 */
LEAF(get_status_reg)
	.set noreorder
	nop
	mfc0	v0,C0_SR
	nop
	.set reorder
	j	ra
	END(get_status_reg)

LEAF(kn01_getspl)
	.set noreorder
	nop
	mfc0	v0,C0_SR
	nop
	.set reorder
	j	ra
	END(kn01_getspl)

/*
 * clear_bev -- Change the control of TLB and general exception vectors to be
 * handled by the kernel and not the console.  This needs to be done after
 * the exception handling bcopy of the vector code in the processor specific.
 */
LEAF(clear_bev)
	.set noreorder
	mfc0	v0,C0_SR		# get contents of SR
	li	t0,(~SR_BEV)		# set up not of BEV bit
	nop
	and	v0,t0			# clear the BEV bit
	nop
	mtc0	v0,C0_SR
	j	ra
	nop
	.set reorder
	END(clear_bev)

/*
 * setsoftnet() - make software network interrupt request
 */
EXPORT(setsoftnet)
	li	a0, CAUSE_SW2
	j	siron

/*
 * acksoftnet() - acknowledge software network interrupt
 */
EXPORT(acksoftnet)
	li	a0, CAUSE_SW2
	j	siroff

/*
 * setsoftclock() - make software clock interrupt request
 */
EXPORT(setsoftclock)
	li	a0, CAUSE_SW1
	j	siron

/*
 * acksoftclock() - acknowledge software clock interrupt
 */
EXPORT(acksoftclock)
	li	a0, CAUSE_SW1
	j	siroff

/*
 * siron(level) -- make software interrupt request
 */
LEAF(siron)
	.set	noreorder
	mfc0	v0,C0_SR
	mtc0	zero,C0_SR		# disable all interrupts
	mfc0	v1,C0_CAUSE
	nop
	or	v1,a0
	mtc0	v1,C0_CAUSE
	mtc0	v0,C0_SR		# PE BIT
	j	ra	
	nop
	.set	reorder
	END(siron)

/*
 * siroff(level) -- acknowledge software interrupt request
 */
LEAF(siroff)
	.set	noreorder
	mfc0	v0,C0_SR
	mtc0	zero,C0_SR		# disable all interrupts
	mfc0	v1,C0_CAUSE
	not	a0
	and	v1,a0
	mtc0	v1,C0_CAUSE
	mtc0	v0,C0_SR		# PE BIT
	j	ra
	nop
	.set	reorder
	END(siroff)

/*
 * bbadaddr(addr, len)
 *	check for bus error on read access to addr
 *	len is length of access (1=byte, 2=short, 4=long)
 */
BADADDRFRM=	(4*4)+4		# 4 arg saves plus a ra
NESTED(bbadaddr, BADADDRFRM, zero)
	.set noreorder
	nop
	mfc0	t0,C0_SR
	nop
	mtc0	zero,C0_SR
	nop
	.set reorder
	subu	sp,BADADDRFRM
	sw	ra,BADADDRFRM-4(sp)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	lw	t1,u+PCB_CPUPTR
	li	v0,NF_BADADDR
	bne	a1,1,1f
	sw	v0,CPU_NOFAULT(t1)

	lb	v0,0(a0)
	b	4f
	nop

1:	bne	a1,2,2f
	nop
	lh	v0,0(a0)
	b	4f
	nop

2:	bne	a1,4,3f
	nop
	lw	v0,0(a0)
	b	4f
	nop

	.set	reorder
3:	PANIC("bbaddaddr")
	.set	noreorder

4:	lw	t1,u+PCB_CPUPTR
	nop
	sw	zero,CPU_NOFAULT(t1)
	lw	ra,BADADDRFRM-4(sp)
	addu	sp,BADADDRFRM
	mtc0	t0,C0_SR		# PE BIT
	move	v0,zero
	j	ra
	nop
	.set	reorder
	END(bbadaddr)

/*
 * wbadaddr(addr, len)
 *	check for bus error on write access to addr
 *	len is length of access (1=byte, 2=short, 4=long)
 */
NESTED(wbadaddr, BADADDRFRM, zero)
	subu	sp,BADADDRFRM
	sw	ra,BADADDRFRM-4(sp)
	.set noreorder
	nop
	mfc0	t0,C0_SR
	nop
	mtc0	zero,C0_SR
	nop
	.set reorder
#ifndef SABLE
	lw	zero,SBE_ADDR|K1BASE
#endif
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	bne	a1,1,1f
	lw	t1,u+PCB_CPUPTR
	li	v0,NF_BADADDR		#LDSLOT
	sw	v0,CPU_NOFAULT(t1)

	sb	zero,0(a0)
	b	4f
	nop

1:	bne	a1,2,2f
	nop
	b	4f
	sh	zero,0(a0)

2:	bne	a1,4,3f
	nop
	b	4f
	sw	zero,0(a0)

	.set	reorder
3:	PANIC("wbaddaddr")
	.set	noreorder

4:	bc0f	4b			# wait for write buffer empty
	nop
	lw	v0,K1BASE
	mfc0	t1,C0_CAUSE
	nop
	and	t1,CAUSE_IP7		# Memory line error
	bne	t1,zero,baerror
	nop
	lw	t1,u+PCB_CPUPTR
	nop
	sw	zero,CPU_NOFAULT(t1)
	mtc0	t0,C0_SR		# PE BIT
	lw	ra,BADADDRFRM-4(sp)
	addu	sp,BADADDRFRM
	j	ra
	move	v0,zero
	.set	reorder
	END(wbadaddr)

/*
 * wbadmemaddr(addr)
 *	check for address error on word access to addr
 *	Assumes addr points to RAM since trap is generated by read-back
 */
 NESTED(wbadmemaddr, BADADDRFRM, zero)
	.set noreorder
	nop
	mfc0	t0,C0_SR
	nop
	mtc0	zero,C0_SR
	nop
	.set reorder
 	subu	sp,BADADDRFRM
 	sw	ra,BADADDRFRM-4(sp)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
 	.set	noreorder
	lw	t1,u+PCB_CPUPTR
 	li	v0,NF_BADADDR		# LDSLOT
	sw	v0,CPU_NOFAULT(t1)
 	sw	zero,0(a0)		# store first to generate ECC
 	lw	v0,0(a0)		# load can cause sync DBE
	sw	zero,CPU_NOFAULT(t1)
 	lw	ra,BADADDRFRM-4(sp)
 	addu	sp,BADADDRFRM
	mtc0	t0,C0_SR		# PE BIT
 	j	ra
 	move	v0,zero
 	.set	reorder
	END(wbadmemaddr)

/*
 * trap() nofault code comes here on bus errors when nofault == NF_BADADDR
 */
NESTED(baerror, BADADDRFRM, zero)
#ifndef SABLE
	lw	v0, cpu			# get cpu
	bne	v0, DS_3100, 1f		# only PMAX has SBE
	lw	zero,SBE_ADDR|K1BASE
1:
#endif
	.set noreorder
	nop
	mtc0	t0,C0_SR		# PE BIT
	nop
	.set reorder
	li	v0,1
	lw	ra,BADADDRFRM-4(sp)
	addu	sp,BADADDRFRM
	j	ra
	END(baerror)

/*
 * ffs(word)
 * BEWARE: that C version of this routine that is distributed with 4.2
 * is incorrect!
 *
 * find first bit set in word (a la VAX instruction)
 * looks at low order bits first, lowest order bit is 1, highest bit is 32
 * no bits returns 0
 */
LEAF(ffs)
	.set	noreorder
	move	v0,zero
	beq	a0,zero,2f		# no bits set, return zero
1:	and	v1,a0,1
	addu	v0,1
	beq	v1,zero,1b
	srl	a0,1			# BDSLOT: shift right to next bit
2:	j	ra
	nop
	.set	reorder
	END(ffs)

#ifdef notdef
LEAF(ffs)
	move	v1,zero			# initial table offset
	and	v0,a0,0xffff		# check lower halfword
	bne	v0,zero,1f		# bits in lower halfword
	addu	v1,64			# table offset for halfword
	srl	a0,16			# check upper halfword
1:	and	v0,a0,0xff		# check lower byte of halfword
	bne	v0,zero,2f		# bits in lower byte
	addu	v1,32			# table offset for byte
	srl	a0,8			# check upper byte of halfword
2:	and	v0,a0,0xf		# check lower nibble
	bne	v0,zero,3f		# bits in lower nibble
	addu	v1,16			# table offset for nibble
	srl	v0,a0,4			# check upper nibble
	and	v0,0xf			# isolate lower nibble
3:	addu	v1,v0			# total table offset
	lbu	v0,ffstbl(v1)		# load bit number from table
	j	ra
	END(ffs)

	.data
#define NIBBLE(x) \
	.byte	0,       1+(x)*4, 2+(x)*4, 1+(x)*4; \
	.byte	3+(x)*4, 1+(x)*4, 2+(x)*4, 1+(x)*4; \
	.byte	4+(x)*4, 1+(x)*4, 2+(x)*4, 1+(x)*4; \
	.byte	3+(x)*4, 1+(x)*4, 2+(x)*4, 1+(x)*4
ffstbl:
	NIBBLE(0)
	NIBBLE(1)
	NIBBLE(2)
	NIBBLE(3)
	NIBBLE(4)
	NIBBLE(5)
	NIBBLE(6)
	NIBBLE(7)

	.text
#endif notdef

/*
 * ffintr(cause_register) -- find first bit set in interrupt pending byte
 * bits are numbered as 8 most significant to 1 least significant,
 * search starts from most significant end, returns 0 in no bits set
 */
LEAF(ffintr)
	and	v0,a0,CAUSE_IPMASK
	srl	a0,v0,CAUSE_IPSHIFT+4	# shift to high nibble of IPEND bits
	bne	a0,zero,1f		# bits set in high nibble
	srl	a0,v0,CAUSE_IPSHIFT	# get 2nd nibble right
	add	a0,16			# to get to 2nd half of table
1:	lbu	v0,ffitbl(a0)		# get value from table
	j	ra
	END(ffintr)

	.data
ffitbl:
	.byte 0,5,6,6,7,7,7,7,8,8,8,8,8,8,8,8
	.byte 0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4
	.text


/*
 * scanc(count, cp, table, mask)
 * Like VAX instruction
 */
LEAF(scanc)
	move	v0,a0
	b	2f

1:	subu	v0,1		# decr count
2:	beq	v0,zero,3f	# count exhausted
	lbu	v1,0(a1)	# get char at cp
	addu	a1,1		# incr cp
	addu	t8,a2,v1	# offset into table
	lbu	t9,0(t8)	# load table entry
	and	t9,a3		# mask table entry
	beq	t9,zero,1b	# masked bit set
3:	j	ra
	END(scanc)

/*
 * in_checksum(addr, len, prevcksum)
 *
 * Calculates a 16 bit ones-complement checksum.
 * Note that for a big-endian machine, this routine always adds even
 * address bytes to the high order 8 bits of the 16 bit checksum and
 * odd address bytes are added to the low order 8 bits of the 16 bit checksum.
 * For little-endian machines, this routine always adds even address bytes
 * to the low order 8 bits of the 16 bit checksum and the odd address bytes
 * to the high order 8 bits of the 16 bit checksum.
 */
LEAF(in_checksum)
	move	v0,a2		# copy previous checksum
	beq	a1,zero,4f	# count exhausted
	and	v1,a0,1
	beq	v1,zero,2f	# already on a halfword boundry
	lbu	t8,0(a0)
	addu	a0,1
#ifdef MIPSEL
	sll	t8,8
#endif MIPSEL
	addu	v0,t8
	subu	a1,1
	b	2f

1:	lhu	t8,0(a0)
	addu	a0,2
	addu	v0,t8
	subu	a1,2
2:	bge	a1,2,1b
	beq	a1,zero,3f	# no trailing byte
	lbu	t8,0(a0)
#ifdef MIPSEB
	sll	t8,8
#endif MIPSEB
	addu	v0,t8
3:	srl	v1,v0,16	# add in all previous wrap around carries
	and	v0,0xffff
	addu	v0,v1
	srl	v1,v0,16	# wrap-arounds could cause carry, also
	addu	v0,v1
	and	v0,0xffff
4:	j	ra
	END(in_checksum)

/*
 *	The XNS checksummer does an add-and-cycle checksum.  Odd byte
 *	lengths are dealt with by postpending a garbage byte, which is
 *	carried with the packet forever, and is not assumed to be
 *	zero.  Thus, the algorithm is load a half-word, and add it to
 *	the current checksum.  Then, shift the whole mess left one bit,
 *	and iterate.  All math is ones-complement on  16 bits, so when
 *	we are done, we must  fold back all the carry bits that are in
 *	the high 16 bits of our register.  The caller is required to
 *	half-word align the packet, since we can't easily, and to
 *	postpend the garbage byte if necessary.
 */
LEAF(ns_checksum)
	move	v0,a2		# copy previous checksum
	move	t0, a1		# save count
	beq	a1,zero,3f	# count exhausted
	and	v1,a0,1
	beq	v1,zero,2f	# already on a halfword boundry
	li	v0, 0177777	# error code
	b	3f

1:	lhu	t8,0(a0)
	addu	a0,2
	addu	v0,t8
	subu	a1,2

2:	bge	a1,2,1b
	srl	v1,v0,16	# add in all previous wrap around carries
	and	v0,0xffff
	addu	v0,v1
	srl	v1,v0,16	# wrap-arounds could cause carry, also
	addu	v0,v1
	and	v0,0xffff
	sll	t0, 1		# divide count by two it - is round
	rol	v0, t0		# now do 32 bit rotate...
	and	v1, v0, 0xffff	# and fold it back down to 16 bits
	and	v0, 0xffff0000
	or	v0, v1		# done, now have 16 bit checksum

3:	j	ra
	END(ns_checksum)

/*
 * nuxi_s and nuxi_l -- byte swap short and long
 */
LEAF(nuxi_s)			# a0 = ??ab
	srl	v0,a0,8		# v0 = 0??a
	and	v0,0xff		# v0 = 000a
	sll	v1,a0,8		# v1 = ?ab0
	or	v0,v1		# v0 = ?aba
	and	v0,0xffff	# v0 = 00ba
	j	ra
	END(nuxi_s)

LEAF(nuxi_l)			# a0 = abcd
	sll	v0,a0,24	# v0 = d000
	srl	v1,a0,24	# v1 = 000a
	or	v0,v0,v1	# v0 = d00a
	and	v1,a0,0xff00	# v1 = 00c0
	sll	v1,v1,8		# v1 = 0c00
	or	v0,v0,v1	# v0 = dc0a
	srl	v1,a0,8		# v1 = 0abc
	and	v1,v1,0xff00	# v1 = 00b0
	or	v0,v0,v1	# v0 = dcba
	j	ra
	END(nuxi_l)

/*
 * Write buffer flush routine.  Routine waits until
 * the write buffer is empty before returning.
 */

LEAF(kn01wbflush)		/* WB flush routine for PMAX (R2000a) */
1:	
 	bc0f	1b
 	j	ra
	END(kn01wbflush)

LEAF(kn210wbflush)		/* WB flush routine for R3000	*/
	.set noreorder
	mfc0	v0,C0_SR	/* v0 = status register */
	li	t0,0x80000000	/* set CU3 bit */
	or	v1,v0,t0	/* v1 = v0 | 0x80000000 */
	nop
	mtc0	v1,C0_SR	/* status register = v1 */
	nop			/* both these nops are needed */
	nop			/* both these nops are needed */
1:	
 	bc3f	1b		/* wait till write buffer empty */
	nop			/* this no op too		*/
	mtc0	v0,C0_SR	/* restore old status register */
	nop
 	j	ra
	nop
	.set reorder
	END(kn210wbflush)

#ifdef DS5000_100
LEAF(kn02ba_wbflush)			/* WB flush routine for 5000_100 */
	lw	v0, KN02BA_SIRM_K1ADDR	/* a read will flush all writes	 */
	j	ra
	END(kn02ba_wbflush)
#endif

LEAF(kn220wbflush)		/* WB flush routine for R3000	*/
	.set noreorder
	la	t0,kn220wbflush_loc
	li	t1, 0xa0000000  /* Make kseg1 address   */
	or      t0,t1
	sw	t1,0(t0)
	lw	t1,0(t0)
	nop
 	j	ra
	nop
	.set reorder
	END(kn210wbflush)

NESTED(smp_lock_retry,24,zero)
	.set	noreorder

#ifndef SMP
	lw	t5,(a0)	
	lw	t1,u+PCB_CPUPTR	 /* get cpudata pointer */
	bltz	t5,3f
	lw	t2,CPU_HLOCK(t1) /* save off lock list head */
	sw	a0,CPU_HLOCK(t1) /* put new lock at head */
	sw	ra,L_PC(a0)	/* store off PC lock asserted at */	
	lui	t3,0x8000
	lw	t4,L_WON(a0)	/* get lock won */
	or	t6,t3,t5	/* set lock bit */
	addiu	t4,t4,1		/* increment lock won */
	sw	t6,(a0)
	sw	t4,L_WON(a0)
	j	ra
	sw	t2,L_PLOCK(a0)  /* chain old list off new lock */


3:
	addiu	sp,sp,-24	/* save stack space */
	b 	1f
	sw	ra,20(sp)	/* save off return */
#else			
2:	
	lw	t6,smp_debug	/* read up smp_debug*/
	addiu	sp,sp,-24	/* save stack space */
	bne	t6,zero,1f	/* branch if debug enabled */
	sw	ra,20(sp)	/* save off return */

	jal	setlock
	sw	a0,24(sp)	/* save off lock pointer */

	beq 	v0,zero,1f	/* branch if we failed to get lock */
	lw	a0,24(sp)	/* restore lock pointer */
	lw	t1,u+PCB_CPUPTR	 /* get cpudata pointer */
	lw	ra,20(sp)		/* restore return address */
	lw	t2,CPU_HLOCK(t1) /* save off lock list head */
	sw	a0,CPU_HLOCK(t1)	/* put new lock at head */
	lw	t4,L_WON(a0)	/* get lock won */
	sw	ra,L_PC(a0)		
	addiu	t4,t4,1		/* increment lock won */
	addiu	sp,sp,24		/* give back stack space */
	sw	t4,L_WON(a0)
	j	ra
	sw	t2,L_PLOCK(a0);		/* chain old list off new lock */
#endif

1:	li	a1,LK_RETRY
	jal	smp_lock_long		/*  smp_lock_long(lk,LK_RETRY,pc) */
	lw	a2,20(sp)

	lw	ra,20(sp)		/* restore return address */
	addiu	sp,sp,24		/* give back stack space */
	j 	ra
	li	v0,0			/* failed */

	.set reorder
        END(smp_lock_retry)

NESTED(smp_lock_once,24,zero)
	.set	noreorder
#ifndef SMP
	lw	t5,(a0)
	lw	t1,u+PCB_CPUPTR	 /* get cpudata pointer */
	bltz	t5,3f
	lw	t2,CPU_HLOCK(t1) /* save off lock list head */
	sw	a0,CPU_HLOCK(t1) /* put new lock at head */
	sw	ra,L_PC(a0)	/* store off PC lock asserted at */	
	lui	t3,0x8000
	lw	t4,L_WON(a0)	/* get lock won */
	or	t6,t3,t5	/* set lock bit */
	addiu	t4,t4,1		/* increment lock won */
	sw	t6,(a0)
	sw	t4,L_WON(a0)
	li	v0,1			/* success */
	j	ra
	sw	t2,L_PLOCK(a0)  /* chain old list off new lock */

3:				/* failed */
	j 	ra
	li	v0,0
#else			
2:	

	lw	t6,smp_debug	/* read up smp_debug*/
	addiu	sp,sp,-24	/* save stack space */
	bne	t6,zero,1f	/* branch if debug enabled */
	sw	ra,20(sp)	/* save off return */

	jal	setlock
	sw	a0,24(sp)	/* save off lock pointer */

	beq 	v0,zero,2f	/* branch if we failed to get lock */
	lw	a0,24(sp)	/* restore lock pointer */
	lw	t1,u+PCB_CPUPTR	 /* get cpudata pointer */
	lw	ra,20(sp)		/* restore return address */
	lw	t2,CPU_HLOCK(t1) /* save off lock list head */
	sw	a0,CPU_HLOCK(t1)	/* put new lock at head */


	lw	t4,L_WON(a0)	/* get lock won */
	sw	ra,L_PC(a0)		
	addiu	t4,t4,1		/* increment lock won */
	addiu	sp,sp,24		/* give back stack space */
	sw	t4,L_WON(a0)
	j	ra
	sw	t2,L_PLOCK(a0);		/* chain old list off new lock */

2:					/* failed */
	lw 	ra,20(sp)
	addiu	sp,sp,24		/* give back stack space */
	j 	ra
	li	v0,0
#endif

1:
	li	a1,LK_ONCE
	jal	smp_lock_long		/*  smp_lock_long(lk,LK_ONCE,pc) */
	lw	a2,20(sp)

	lw	ra,20(sp)		/* restore return address */
	addiu	sp,sp,24		/* give back stack space */
	j 	ra
	nop
	.set reorder
        END(smp_lock_once)



/*
 * This is a routine that reads a 32-bit word from an address
 * that may not exist, or may cause a bus timeout.
 *
 * NOTE:  This routine is used by the interrupt handler code for
 *	  kn210 to read the interrupt vector from the vector registers.
 *	  If there is a passive release from the interrupting devices,
 *	  the reading of the vector register will cause a bus timeout
 *	  (DBE exception).  Don't generalise this routine to do more
 *	  because speed is important here.
 *
 * synopsis:
 *	int read_nofault(nofault_src)
 *	int *nofault_src;
 *
 *	nofault_src is the address to read from.  If the read fails,
 *	a zero will be returned.  If the read is successful, the value
 *	from nofault_src will be returned.
 */
LEAF(read_nofault)
	.set noreorder
	mfc0	t0,C0_SR		# save status register
	lw	v0,u+PCB_CPUPTR
	nop
	lw	t1,CPU_NOFAULT(v0)	# save old value of nofault
	mtc0	zero,C0_SR
	.set reorder
	li	v1,NF_INTR		#
	sw	v1,CPU_NOFAULT(v0)	# nofault = NF_BADADDR
	lw	v0,0(a0)		# v0 = *nofault_src;
	/* if we get here the read was successful */
	lw	v1,u+PCB_CPUPTR
	sw	t1,CPU_NOFAULT(v1)		# restore old value of nofault
	.set noreorder
	mtc0	t0,C0_SR		# restore status register
	nop
	.set reorder
	j	ra
	END(read_nofault)

LEAF(rdnf_error)
	lw	v1,u+PCB_CPUPTR
	sw	t1,CPU_NOFAULT(v1)	# restore old value of nofault
	.set noreorder
	mtc0	t0,C0_SR		# PE BIT
	.set reorder
	move	v0,zero
	j	ra
	END(rdnf_error)

/*
 * unixtovms(srcaddr,dstaddr)
 *
 * a0 src address
 * a1 dst address
 * t0 tmp
 * t1 tmp
 * t2 tmp value ->0(a1)
 * t3 tmp value ->4(a1)
 * t4 carry
 * t5 <- constants - 0x989680   convert sec. to 100 nanosec. units.
 *	           - 0x4beb4000 difference between 00:00 Jan. 1 1970
 *	           - 0x7c9567   and 00:00 Nov. 17 1858 to form the
 *				 VMS time.
 */
LEAF(unixtovms)
	lw 	t0, 0(a0)
	lw 	t1, 4(a0)
	li 	t5, 0x989680
	multu 	t0, t5
	mflo	t2
	mfhi	t3
	addu	t0, t2, t1
	sltu	t4, t0, t2
	addu 	t3, t4
	li 	t5, 0x4beb4000
	addu 	t2, t0, t5
	sltu	t4, t2, t0
	addu 	t3, t4
	li 	t5, 0x7c9567
	addu	t3, t5
	sw	t2, 0(a1)
	sw	t3, 4(a1)
	j	ra
	END(unixtovms)

/* Function used by LMF code to measure the speed of a PMAX type processor.
 * It returns the number of loop iterations before the clock
 * next ticks.   The arguments are the previous value of the clock
 * for comparison and a maximum for safety.
 *
 * This is in assembler to protect against compiler changes.
 * ***** If you change this code you must also change the line:
 * ***** #define THRESH xxx
 * ***** in kern_lmf.c after experimenting to find the new value.
 */

LEAF(wait_tick)
	.set	noreorder
	li	v0, 0		# Initialise counter
	la	t1, timepick
1:	lw	t0, (t1)	# Load timepick
	blez	a1, 2f		# Check for safety counter exhausted
	lw	t0, 4(t0)	# Load timepick->tv_usecs
	addi	a1, -1		# Decrement safety counter
	beq	t0, a0, 1b	# Branch if time has changed (clock ticked)
	addiu	v0, 1		# Count loop iterations in delay slot
2:	j	ra
	nop
	.set	reorder
	END(wait_change)

/*
 * This statement measures the length of the mfc0 code sequence to
 * see if the mfc0 assembler option has been invoked. See autoconf
 * for exact usage. (This is a hack).
 */
EXPORT(mfc0_start)
	.set noreorder
	nop
	mfc0	v0,C0_SR
	nop
	.set reorder

EXPORT(mfc0_end)

/*
 * Bootstrap program executed in user mode
 * to bring up the system.
 */
EXPORT(icode)
	la	a0,icode_file
	la	v0,icode		# relocate for user space
	subu	a0,v0
	addu	a0,USRDATA
	la	a1,icode_argv
	la	v0,icode		# relocate for user space
	subu	a1,v0
	addu	a1,USRDATA
	sw	a0,0(a1)		# relocate vector in d space
	li	a2,0			# no environment please
	li	v0,SYS_execve
	syscall
	li	v0,SYS_exit
	syscall
1:	b	1b

	.align	4
EXPORT(icode_argv)
	.word	icode_file
	.space	10*4			# leave space for boot args

EXPORT(icode_file)
	.asciiz	"/etc/init"
	.space	32
argp:					# leave space for boot args
	.space	64

	.align	4

EXPORT(icode_args)
	.word	argp

EXPORT(icode_argc)
	.word	1

EXPORT(eicode)

	.sdata
EXPORT(Vaxmap)
	.word	1
EXPORT(Vaxmap_size)
	.word	1
EXPORT(VSysmap)
	.word	1
EXPORT(Dbptemap)
	.word	1
EXPORT(Vaxdbpte)
	.word	1
	.text

#ifndef ASM_FIXED
	.word	0
#endif !ASM_FIXED
