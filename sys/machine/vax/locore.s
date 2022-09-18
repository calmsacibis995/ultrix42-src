/*    @(#)locore.s	4.10     (ULTRIX)        4/11/91     */

/************************************************************************
 *									*
 *			Copyright (c) 1983 - 1989 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 * Modification history: /sys/vax/locore.s 
 *
 * 11-Apr-91	dlh
 *	add call to vp_reset() during slavestart; call will be made if this CPU
 *	is in vpmask.
 *
 * 30-Jan-91    jaw
 *	fix branch in stop cpu to properly save off stack pointer...
 *
 * 21-Jan-91	dlh
 *	In the case of the VAX9000, the routine cpuident() is accessing the
 *	wrong register to find out which cpu it is.  It was accessing the
 *	CPUCNF.  It now accesses CPUID.
 *
 * 10-Oct-90	paradis
 *	Modify Xprotflt handler to parse all the bits in the
 *	trap type word (including the new vector memory fault
 *	bits)
 *
 * 10-Oct-90	dlh
 *	add vector support for the Mariah (VAX6000-5xx)
 *
 * 4-Sep-90	dlh
 *	added support for vector processor support
 *		- entry point for vector disabled fault
 *		- Rigel - correct the way ACCS is dealt with - want to 
 *		  deal with FPA and vector present bit separately
 *		- a process which is a vector process needs to call
 *		  vp_contextlimbo()
 *		- added routine is_legal_vector_instruction()
 *
 * 31-Aug-90	paradis
 *	Removed debug scaffolding from call to ka9000_rxfct_isr().
 *	Removed call to ka9000_txfct_isr() entirely since we don't
 *	use it anymore.
 *	Set modify bit in PTEs for all kernel-writeable pages.  This
 *	(a) prevents crashes due to taking modify faults inside
 *	fault handlers, and (b) keeps us from taking superfluous 
 *	kernel modify faults (we don't do anything with them anyway).
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added support for VAX9000.
 *
 * 19-Jun-90 -- jmartin
 *	Fixes for "panic: vrelvm rssize"
 *
 * 07-Jun-90 jas
 *	Added code to Slavestart.  If VAX is 6400/6500, load ACCS with
 *	0.  This will put 6500 in 30-bit mode, and have no affect on 6400.
 *
 * 06-Jun-90 pmk
 *	Remove ci0int routine, ci vector interrupt routine now made by
 *	config.
 *
 * 01-May-90 jas
 *	added 6500 support.
 *
 * 30-Mar-90 jaw
 *	allow 6200 to recover from hard error.
 *
 * 03-Mar-90 jaw
 *	primitive change to optimize mips.
 *
 * 17-Nov-89    Paul Grist
 *      modified so that the pointer to the pc/psl will be passed for
 *      an nmifault for some 8800 error handling inmprovements.
 *
 * 09-Nov-89 	bp
 *	Checked that physical memory doesn't exceed 511.5 Mb and
 *	when it does set it to 511.5 Mb
 *	optimized distcl-jmartin request
 *	have __km_alloc return null when index < MINBUCKET
 *
 * 20-Jul-89	rafiey (Ali Rafieymehr)
 *	Deleted the stub routine xmisst(). The real routine is now
 *	defined in xmiinit.c.
 *
 * 20-Jun-89 -- jaw
 *	fix old 780 bug where it falsely claims to have a protection
 *	fault.  fix stolen from VMS.
 *
 * 13-Jun-89 -- jmartin
 *	Change calculation of P0LR for physical-to-virtual transition
 *	to accomodate SVS kernel.  Before each LDPCTX instruction
 *	write 255 and 0 respectively into PCB_PRVCPU and PCB_ASN.
 *	This is minimal support for ECO 119.
 *
 * 30-May-89	darrell
 *	Added include of ../../machine/common/cpuconf.h -- cpu types
 *	were moved there.  Changed the values for processor types,
 *	and other things for the new cpusw.
 *
 * 22-May-89 -- darrell
 *	Changed tocons to cons_putc -- as part of the new cpusw.
 *
 * 18-May-89	Jaw
 *	fix MSI/CI interrupt vectors to count interrupts in percpu structure.
 *	fix baddaddr for 6400.
 *
 * 12-May-89	Todd M. Katz		TMK0002
 *	1. Increment counter on all CI interrupts.  Also, change the name of
 *	   the SCB vector from Xcia0int -> Xci0int.  Note: The second CI SCB
 *	   vector, Xci1int() has been deleted because only a single CI port
 *	   is currently supported.
 *	2. Completely change MSI interrupt code.
 *	3. Include header file "msi.h".
 *
 * 12-May-89 -- darrell
 *	Merged V3.1 changes.
 *
 * 5-May-89 --	Adrian Thoms
 *	Moved VAX_6200 to cpu #14 for VVAX
 *	Added VVAX dump routines v_qio and v_qioinit
 *
 * 07-March-89 -- gmm
 *	release_uarea_noreturn uses u_procp, instead of an argument to get
 *	the current proc pointer. 
 *
 * 15-Feb-89 -- jaw
 *	Add check to copyin/copyout to make sure not spin locks are held.
 *
 * 09-Feb-89 -- jaw
 *	Move CPU_TBI state flag to seperate longword.  This was done
 *	because the state flag field can only be written by the cpu
 *	who owns it.
 *
 * 1-Feb-89	Tom Kong
 *	Added workaround for Rigel 2nd pass bug.  When probing NXM
 *	that causes machine checks, clear Rigel error registers.
 *
 * 10 Jan 89 -- kong
 *	Added Rigel (VAX6400) support.  Added comments about CPU/system
 *	identification.
 *
 *  02-Feb-89	jaw
 *	Make longjmp mpsafe.  move newpc and newfp global to per-cpudata base.
 *
 * 17-Nov-88	darrell
 *	Added ka60memerr SCBVEC entry for Firefox bus error handling
 *
 *  26-Jan-89	jaw
 *	fix up start/stop cpu.
 *
 * 23-Jan-89 -- jmartin
 *	Introduce a new trap type NOACCESS for protection fault on read.
 *
 * 12-Jan-89 - jaw
 *	merge Xe changes for FFox drivers
 *
 * 03 Jan 89 -- jaw
 *	fix branch in stop_secondary_cpu
 *
 * 29 Dec 88 -- jmartin
 *	Zero physical memory from _edata to (vmbinfo).
 *
 * 08 Dec 88 -- jaw 
 *	When panicing the system put the cpu's into tight loop
 * 	after flushing out context.
 *
 * 22 Nov 88 -- jaw 
 *	fix case where we were sending signal to our parent before we
 *	were REALLY stopped....  more magic.
 *
 * 10 Oct 88 -- jaw
 *	test for lock held after swtch.
 *
 * 31 Aug 88 -- jmartin
 *	Use the lock lk_text in Fastreclaim.  Rationalize Fastreclaim
 *	to use longword operands and to avoid branching on the primary
 *	code path.
 *
 * 19 Aug 88 -- miche
 *	cleaned up context switch code for SMP
 *
 * 25 Jul 88 -- jmartin
 *	Use the lock lk_cmap in Fastreclaim.  Also don't write the bit
 *	SPTECHG to (struct proc *)foo->p_vm, because VAXen don't care.
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 *
 * 22 Apr 88 -- mbh
 *	Slave checks for work to do on all queues.  This section
 *	should be consolidated with that of the primary processor.
 *
 * 24 Mar 88 -- prs
 *	Changed copy*str routines to return ENAMETOOLONG, instead
 *	of ENOENT for POSIX conformance.
 *
 * 21 Jan 88 -- jmartin
 *	Replace calls to the (inline) functions clearseg and copyseg
 *	respectively with blkclr (or bzero) and blkcpy (or bcopy).
 *	Establish a window in process memory through which a parent
 *	can write to (and read from) the memory of the child.  This
 *	window is UPAGES*NBPG bytes located between the u-area and the
 *	user stack.  Remove the following entities: CMAP1, CADDR1,
 *	CMAP2, CADDR2, Vfmap, vfutl, clearseg, copyseg.  Redefine
 *	Forkmap and forkutl.  Change the computation for the location
 *	of USRSTACK and the size of the process page table.
 *
 ***********************SMP rev history above
 * 17-Oct-88	jaw
 *	Prefetch istream in bisst routine so that 8200 won't machinecheck.
 *
 * 01-Sep-88	darrell
 *	Added a call to ka60initslave to turn on the slave processors
 *	internal cache.
 *
 * 30-Aug-88	jaw
 *	add proc field p_master to fix ASMP bug.
 *
 * 18-Aug-88	darrell
 *	Added multiprocessor support for VAX60, and a call to chrqueue
 *	in the fcpdma code to get around the fact that the VAX60 console
 *	DZ interrupts come in at IPL 16.
 *
 * 08-Aug-88	Fred Canter
 *	Temporary fix for PVAX SCSI/SCSI controller hardware hack.
 *	Do an I/O space access each time thru the idle loop to
 *	generate address strobe needed to allow the SCSI/SCSI
 *	controller temporary hardware fix to work.
 *
 * 15-Jul-88	Todd M. Katz		TMK0001
 *	Add stub xmisst() routine for resetting XMI nodes.
 *
 * 15-July-88	robin
 *	Added cpu_sub_subtype because cpu_systype was being misused.  This
 *	allows sizer and uerf to both work for the KA640.
 *
 * 26-Apr-88    jaw
 *	Add VAX8820 support.
 *
 * 24-Mar-88 -- robin
 *	added code to set up the interupt vec. for ka640 lance chips (NI)
 *	
 * 07-Mar-88 -- prs
 *	Changed copy*str routines to return ENAMETOOLONG, instead
 *	of ENOENT for POSIX conformance.
 *
 * 12-Feb-88 -- fred (Fred Canter)
 *	Changes for VAX420 (CVAXstar/PVAX) support.
 *	Added case 11 to all casel statements.
 *	Modified MVAX class cpu ident code.
 *	Save arch_ident bits in SID EXT register.
 *
 * 25-Jan-88 -- rsp (Ricky Palmer)
 *      Added interrupt code for MSI.
 *
 * 19-Jan-88 -- jaw
 *	added cpu support for Calypso.  Also moved intrcpu into machdep 
 *	because the routine shouldn't be written in assembly code.
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass/kmalloc support to the system.
 *
 * 2-Oct-87	robin
 *	fixed a problem which stoped the AIO init process from working.
 *	The arg to qioinit is indirect and the code was direct.
 *
 * 20-Apr-87 -- afd
 *	Removed work-around for bugs in the P1 CVAX chips.
 *
 * 19-Mar-87 -- fred (Fred Canter for Brian Stevens)
 *	Schedule X from the idle loop.
 *
 * 06-Mar-87 -- afd
 *	Added CPU specific support for Mayfair/CVAX.
 * 
 * 03 Feb 87 -- depp and woodward
 *	Reinstated the "yech" code in Swtch()/Resume().  It was removed
 *	during the multiprocessing work.  It will only be invoked on the
 *	master CPU.
 *
 * 21 Jan 87 -- jaw
 *	performance fixes to syscall.
 *
 *  16-Dec-86 -- pmk
 *	Changed 750 console tu58 intr. branch to turintr instead of tudma.
 *	750 now uses MRSP protocol
 *
 *  27-Aug-86 -- fred (Fred Canter)
 *	Removed kludge used to identify pass 1 standard cell VAXstar CPUs.
 *
 *  5-Aug-86 -- fred (Fred Canter)
 *	Minor changes to VAXstar SLU pseudo DMA code (sspdma).
 *
 * 18-Jun-86 -- jaw
 *	bisst fix to set NOARB before hitting a node over the head.
 *
 * 18-Jun-86 -- fred (Fred Canter)
 *	Changes for VAXstar kernel support.
 *
 * 22-May-86 -- prs
 *	Added time saving measure to the qioinit routine for
 *	the generic dump driver.
 *
 * 18-Apr-86 -- jaw	hooks for nmi faults and fixes to bierrors.
 *
 * 16-Apr-86 -- darrell
 *	changed badaddr to bbadaddr -- to allow the routine 
 *	badaddr for backward comaptibility.
 *
 * 15-Apr-86 -- jf
 *	Add support for system processes.
 *
 * 14-Apr-86 -- afd
 *	Re-did mutually exclusive MicroVAX areas.  Mainly this is
 *	putting MicroVAX into the case stmts with other cpus.
 *
 * 09-Apr-86 -- prs
 *	Added two qio calls to use the vmb boot driver. These routines
 *	are called from the generic dump driver.
 *
 * 09-Apr-86 -- jaw  add dispatchs for multiple bi error routines.
 *
 * 02-Apr-86 -- jrs
 *	Rewrite scheduler to eliminate special master queue.
 *	Add code in inter-processor interrupts for panic handling.
 *
 * 18-Mar-86 -- jrs
 *	Change intrslave to intrcpu to do one at a time.  Changed
 *	entry conditions for cpuindex and merged w/ cpuident.
 *	Added Jim's change to keep ipl at 1f until machine specific
 *	autoconfig called.
 *
 * 18-mar-86  -- jaw     br/cvec changed to NOT use registers.
 *
 * 10-Mar-86 -- pmk
 *	change panic() entry mask to 0x0fff, save r0-r11 for errlogging.
 *
 * 06-Mar-86 -- tresvik
 *	clear RPB rp_flag so dump will work after copying in VMB's RPB
 *
 * 03-Mar-86 -- jrs
 *	Added code to initialize secondary processor start address in rpb
 *
 * 25-Feb-86 -- jrs
 *	Once again removed cold start check for rxcd routine that was
 *	accidentally added back in previous edit
 *
 * 19-Feb-86 -- bjg
 *	Added sbi error logging, uba error logging
 *	Changed doadump to do tbia before accessing rbp to prevent 
 *	protection fault on 8600
 *
 * 18-Feb-86 -- jrs
 *	Change slavestart to _slavestart so we can pick it up from c.
 *	Also get slave into virtual mode a little sooner.
 *	Cold start check moves up to rxcd() routine.
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 * 12-Feb-86 -- pmk
 *	Added save pc,psl to bierror
 *
 * 12-Feb-86 -- jrs
 *	Changed scheduling loop to only do TBIA on demand
 *
 * 05-Feb-86 -- jrs
 *	Added save of pcbb address for adb backtrace
 *
 * 04-Feb-86 -- tresvik
 *	added VMB boot path support
 *
 * 03-Feb-86 -- jaw  Set ingnorebi error bit so to ignore errors VMB cause.
 *
 * 20 Jan 86 -- bjg
 *	Added stray interrupt error logging (logstray)
 *
 * 17-Jul-85 -- jrs
 *	Add run queue locking
 *
 * 26-Oct-85 -- jaw
 * 	Fix to bisst() to reset BDA properly once system is up.
 *
 * 25-Sep-85 -- jaw
 *	Fix to bisst to mask problem with BIIC SST bit.
 *	After a dump...halt a 8200 so memory is preserved.
 *
 * 09 Sep 85 -- Reilly
 *	Modified to handle the new 4.3BSD namei code.
 * 03-Sep-85 -- jaw
 *	Added BI error interrupt code.
 *
 * 09-AUG-85 -- darrell dunnuck
 *	Unibus zero vectors are now counted here in locore.  We set a
 *	timer in ubaerror on the first zero vector that comes in,
 *	and count them here in locore until the timer has expired.
 *	We never do a unibus reset no matter how many zero vectors we get.
 *
 * 26-Jul-85 -- jaw
 *	fixed up Violation to SRM...TBIA data MBZ.
 *
 * 18-Jul-85 -- tresvik
 *	Change the way the end of doadump causes a reboot.  Call tocons
 *	instead of duplicating C code here in assembly.  VAX 8600 requires
 *	special handling already defined in tocons.
 *
 * 11-Jul-85 -- tresvik
 *	Fix bug in handling write timeout.  Branching around the PANIC
 *	macro requires local label other than 1, since 1 is already used
 *	within the macro.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 06-Jun-85 -- Jaw
 *	got rid of badwrite and replaced it with bisst(). 
 *
 * 05-May-85 -- Larry Cohen
 * 	network interrupt vectors are now configurable
 *
 * 16-Apr-85 -- lp
 *      Added cpu serial line support for VAX8200. Changed tu58 
 *      interface for non-MRSP machines. 
 *
 *  9 Apr 85 -- depp
 *	Added check for Allocated shared memory page in Fastreclaim
 *
 * 22-Mar-85 -- reilly
 *	Added support for floating point emulation for non microvaxes
 *
 * 13-Mar-85 -jaw
 *	Changes for support of the VAX8200 were merged in.
 *
 * 3-MAR-85 -- darrell
 *    handle sbi interupts.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 * 17 Dec 84 -- jrs
 *	Change setjmp, resume to use new regs to avoid swap problems.
 *	Also add savectx for same reason.  Update code in copyin/copyout
 *	so it can be used with new inline program.
 *
 * 06 Nov 84 -- jrs
 *	Reroll code in swtch/resume to avoid double context switch
 *	and handle most likely case linearly.
 *
 * 31 Oct 84 -- jrs
 *	Remove auto calls to dh and dz timer routines in softclock.
 *	These are now done on a demand basis at a higher level
 *
 * 20 Aug 84 -- larry
 *	removed system page table and made it a seperate file - spt.s
 *	so that SYSPTSIZE could be configurable on a binary kernel.
 *
 *  5 May 84 -- rjl
 *	Added QVSS support. The bit map looks like regular memory so some
 *	of the normal autoconfiguration stuff has to be done here.
 *
 * 12 Apr 84 -- rjl
 *	Added float type check for MicroVAX
 *
 * 23 Mar 84 -- slr
 *	Added emulation support for MicroVAX
 *
 * 15 Feb 84 -- rjl
 *	Added support for MicroVAX intelligent boot. Now saving
 *	r10 and r11 as bootdevice and boothowto for later use by
 *	autoconfig.
 *
 *  2 Jan 84 --jmcg
 *	Added support for MicroVAX 1.
 *
 *************************************************************************/

#include "../h/param.h"
#include "../machine/psl.h"
#include "../machine/pte.h"

#include "../h/errno.h"
#include "../h/cmap.h"
#include "../h/cpudata.h"
#include "../h/kmalloc.h"

#include "../machine/mtpr.h"
#include "../machine/pcb.h"
#include "../machine/trap.h"
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/b_params.h"
#include "../machine/cons.h"
#include "../machine/clock.h"
#include "../io/bi/bireg.h"
/*
#include "../machine/nexus.h"
#include "../io/uba/ubareg.h"
*/
#include "../machine/sas/vmb.h"
#include "../h/smp_lock.h"

#include "dh.h"
#include "dz.h"
#include "ss.h"
#include "fc.h"
#include "uu.h"
#include "ps.h"
#include "mba.h"
#include "ci.h"
#include "msi.h"

#define	is_vect_inst 0
#define	isnt_vect_inst 1

#define	ENTRY(name, regs) \
 	.globl _/**/name; .align 1; _/**/name: .word regs
#define R0 0x01
#define R1 0x02
#define R2 0x04
#define R3 0x08
#define R4 0x10
#define R5 0x20
#define R6 0x40

	.set	INTSTK,1	# handle this interrupt on the interrupt stack
	.set	HALT,3		# halt if this interrupt occurs
	.set	HIGH,0x1f	# mask for total disable
	.set	MCKVEC,4	# offset into scb of machine check vector
	.set	VPN_WIDTH,21
	.set	SYSPROC,10	# system process priority level

/*
 * User structure is UPAGES at top of user space.
 */
	.globl	_u
	.set	_u,0x80000000 - UPAGES*NBPG
	.globl	_forkutl
	.set	_forkutl,_u - FORKPAGES*NBPG

	.globl	_intstack
	.globl  _eintstack
/*
 * Do a dump.
 * Called by auto-restart.
 * May be called manually.
 */
	.align	2
	.globl	_doadump
_doadump:
	nop; nop				# .word 0x0101
#define _rpbmap _Sysmap 			# rpb, scb, UNI*vec, istack*4
	bicl2	$PG_PROT,_rpbmap
	bisl2	$PG_KW,_rpbmap
	mtpr	$0,$TBIA
	bitl	$2,_rpb+RP_FLAG			# dump only once!
	bneq	1f
	bisl2	$2,_rpb+RP_FLAG
	movl	sp,_edumpstack
	movab	_edumpstack,sp
	mtpr 	sp,$ISP
	mfpr	$PCBB,-(sp)
	mfpr	$MAPEN,-(sp)
	mfpr	$IPL,-(sp)
	cmpl    $V_VAX,_cpu             	# Are we a Virtual VAX?
	bneq    2f				# no: go to normal dump

	calls	$0,_vvaxdump			# yes
	brb     1f
2:
	mtpr	$HIGH,$IPL
	mtpr	$0,$MAPEN
	pushr	$0x3fff
	calls	$0,_dumpsys
1:
	pushl	$TXDB_BOOT			# reboot the system
	calls	$1,_cons_putc			# let cons_putc tell the console
						# (it handles VAX 8600)
	halt					# come back and halt

/*
 * Where called	: qioinit is called in the generic dump driver.
 * Function	: Called to initialize the dump device.
 * Parameters	: None.
 * Side effects	: Computes global variable _qioentry, the entry point
 *		  into the qio routine of the new boot driver.
 */

	.globl	_qioinit
_qioinit:
	.word	0xffc				# save r2 - r11
	movl	_vmbinfo,r8			# r8 points to info list
	movl	INFO_RPBBAS(r8),r9		# required bu init routines
	movl	INFO_BTDRBAS(r8),r7		# get address of boot driver
	addl3	r7,BQO$L_QIO(r7),_qioentry	# compute address qio routine
	movl	$1,r0				# in case no init routine.
	tstl	BQO$L_UNIT_INIT(r7)		# does init routine exist ?
	beql	4f				# if not dont bother trying
	addl3	r7,BQO$L_UNIT_INIT(r7),r2	# calculate entry point
/*
 * KLUDGE (for [T]MSCP controller types) to force total controller
 * initialization, otherwise we wait an awful long time for
 * initialization to complete. Also used to set up driver rings to
 * talk to VMB, not UFS.
 */
#define	IP	0
#define	SA	2
	cmpb	$BTD$K_UDA,RPB$B_DEVTYP(r9)	# is this a uda/qda/kda ?
	beql	1f				# yes, go hit it
	cmpb	$BTD$K_TK50,RPB$B_DEVTYP(r9)	# is this a TK50 controller ?
	beql	1f				# yes, go hit it
	brb	2f				# proceed with normal init
1:
	movl	RPB$L_CSRPHY(r9),r7		#get the IP register address
	clrw	IP(r7)				# poke it to make it step
1:
	movw	SA(r7),r0			# get the status register
	bitw	$0xfe00,SA(r7)			# is something active ?
	beql	1b				# if not, wait until there is
2:
	mfpr	$SCBB,r7			# required by BI (BDA driver)
	callg	*INFO_VMBARGBAS(r8),(r2)	# do it
4:
	ret

/*
 * Where called	: qiois called in the generic dump driver.
 * Function	: Called to perform reading from core and writing to
 *		  the dump device.
 * Parameters	: 1) Address mode, physical or virtual.
 *		  2) Function, reading or writing virtual/physical space.
 *		  3) Starting block number, starting disk block number.
 *		  4) Transfer size, in bytes.
 *		  5) Address of buffer, memory location.
 * Side effects	: Performs io function and returns completion code.
 */

	.globl _qio
_qio:
	.word	0xe00				# save r9 - r11
	movl	_vmbinfo,r9			# point to the info list
	movl	INFO_RPBBAS(r9),r9		# get address of the RPB
	pushl	r9				# push address of RPB
	pushl	4(ap)				# push address mode
	pushl	8(ap)				# push I/O function code
	pushl	12(ap)				# push starting block number
	pushl	16(ap)				# push transfer size in bytes
	pushl	20(ap)				# push address of buffer
	calls	$6,*_qioentry			# call qio routine
	ret					# return completion code
#ifdef VVAX
/*
 * These two functions are versions of qio and qioinit which run with
 * Virtual Memory turned on. This is required by VVAX.
 */

/*
 * Where called	: v_qioinit is called in the generic dump driver.
 * Function	: Called to initialize the dump device.
 * Parameters	: None.
 * Side effects	: Computes global variable _qioentry, the entry point
 *		  into the qio routine of the new boot driver.
 */

	.globl	_v_qioinit
_v_qioinit:
	.word	0xffc				# save r2 - r11
	moval	_vmb_info,r8			# r8 points to info list
	moval	_rpb,r9				# required by init routines
	movl	INFO_BTDRBAS(r8),r7		# get address of boot driver

	subl2	_vmbinfo,r7			# now convert to virtual
	addl2	r8,r7				# address

	addl3	r7,BQO$L_QIO(r7),_qioentry	# compute address qio routine
	movl	$1,r0				# in case no init routine.
	tstl	BQO$L_UNIT_INIT(r7)		# does init routine exist ?
	beql	1f				# if not dont bother trying
	addl3	r7,BQO$L_UNIT_INIT(r7),r2	# calculate entry point
	
	movl	INFO_VMBARGBAS(r8),r7		# get address of arg list
	subl2	_vmbinfo,r7			# now convert to virtual
	addl2	r8,r7				# address

	callg	(r7),(r2)			# do it
1:
	ret

/*
 * Where called	: v_qio is called in the generic dump driver.
 * Function	: Called to perform reading from core and writing to
 *		  the dump device.
 * Parameters	: 1) Address mode, physical or virtual.
 *		  2) Function, reading or writing virtual/physical space.
 *		  3) Starting block number, starting disk block number.
 *		  4) Transfer size, in bytes.
 *		  5) Address of buffer, memory location.
 * Side effects	: Performs io function and returns completion code.
 */

	.globl _v_qio
_v_qio:
	.word	0xe00				# save r9 - r11
	pushal	_rpb				# push address of RPB
	pushl	4(ap)				# push address mode
	pushl	8(ap)				# push I/O function code
	pushl	12(ap)				# push starting block number
	pushl	16(ap)				# push transfer size in bytes
	pushl	20(ap)				# push address of buffer
	calls	$6,*_qioentry			# call qio routine
	ret					# return completion code

#endif VVAX

/*
 * {fu,su},{byte,word}, all massaged by asm.sed to jsb's
 */
	.globl	_Fuword
_Fuword:
	prober	$3,$4,(r0)
	beql	fserr
	movl	(r0),r0
	rsb
fserr:
	mnegl	$1,r0
	rsb

	.globl	_Fubyte
_Fubyte:
	prober	$3,$1,(r0)
	beql	fserr
	movzbl	(r0),r0
	rsb

	.globl	_Suword
_Suword:
	probew	$3,$4,(r0)
	beql	fserr
	movl	r1,(r0)
	clrl	r0
	rsb

	.globl	_Subyte
_Subyte:
	probew	$3,$1,(r0)
	beql	fserr
	movb	r1,(r0)
	clrl	r0
	rsb


/*
 * Check address.
 * Given virtual address, byte count, and rw flag
 * returns 0 on no access.
 */
_useracc:	.globl	_useracc
	.word	0x0
	movl	4(ap),r0		# get va
	movl	8(ap),r1		# count
	tstl	12(ap)			# test for read access ?
	bneq	userar			# yes
	cmpl	$NBPG,r1			# can we do it in one probe ?
	bgeq	uaw2			# yes
uaw1:
	probew	$3,$NBPG,(r0)
	beql	uaerr			# no access
	addl2	$NBPG,r0
	acbl	$NBPG+1,$-NBPG,r1,uaw1
uaw2:
	probew	$3,r1,(r0)
	beql	uaerr
	movl	$1,r0
	ret

userar:
	cmpl	$NBPG,r1
	bgeq	uar2
uar1:
	prober	$3,$NBPG,(r0)
	beql	uaerr
	addl2	$NBPG,r0
	acbl	$NBPG+1,$-NBPG,r1,uar1
uar2:
	prober	$3,r1,(r0)
	beql	uaerr
	movl	$1,r0
	ret
uaerr:
	clrl	r0
	ret


_addupc:	.globl	_addupc
	.word	0x0
	movl	8(ap),r2		# &u.u_prof
	subl3	8(r2),4(ap),r0		# corrected pc
	blss	9f
	extzv	$1,$31,r0,r0		# logical right shift
	extzv	$1,$31,12(r2),r1	# ditto for scale
	emul	r1,r0,$0,r0
	ashq	$-14,r0,r0
	tstl	r1
	bneq	9f
	bicl2	$1,r0
	cmpl	r0,4(r2)		# length
	bgequ	9f
	addl2	(r2),r0 		# base
	probew	$3,$2,(r0)
	beql	8f
	addw2	12(ap),(r0)
9:
	ret
8:
	clrl	12(r2)
	ret

 
/*
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 *
 * copyinstr(fromaddr, toaddr, maxlength, &lencopied)
 */
ENTRY(copyinstr, R6)
 	movl	12(ap),r6		# r6 = max length
 	jlss	8f
 	movl	4(ap),r1		# r1 = user address
 	bicl3	$~(NBPG*CLSIZE-1),r1,r2	# r2 = bytes on first page
 	subl3	r2,$NBPG*CLSIZE,r2
 	movl	8(ap),r3		# r3 = kernel address
1:
 	cmpl	r6,r2			# r2 = min(bytes on page, length left);
	jgeq	2f
	movl	r6,r2
2:
	prober	$3,r2,(r1)		# bytes accessible?
	jeql	8f
	subl2	r2,r6			# update bytes left count
	locc	$0,r2,(r1)		# null byte found?
	jneq	3f
	subl2	r2,r1			# back up pointer updated by `locc'
	movc3	r2,(r1),(r3)		# copy in next piece
	movl	$(NBPG*CLSIZE),r2	# check next page
	tstl	r6			# run out of space?
	jneq	1b
	movl	$ENAMETOOLONG,r0	# set error code and return
	jbr	9f
3:
	tstl	16(ap)			# return length?
	beql	4f
	subl3	r6,12(ap),r6		# actual len = maxlen - unused pages
	subl2	r0,r6			#	- unused on this page
	addl3	$1,r6,*16(ap)		#	+ the null byte
4:
 	subl2	r0,r2			# r2 = number of bytes to move
	subl2	r2,r1			# back up pointer updated by `locc'
	incl	r2			# copy null byte as well
	movc3	r2,(r1),(r3)		# copy in last piece
	clrl	r0			# redundant
	ret
8:
	movl	$EFAULT,r0
9:
	tstl	16(ap)
	beql	1f
 	subl3	r6,12(ap),*16(ap)
1:
	ret

/*
 * Copy a null terminated string from the kernel
 * address space to the user address space.
 *
 * copyoutstr(fromaddr, toaddr, maxlength, &lencopied)
 */
ENTRY(copyoutstr, R6)
	movl	12(ap),r6		# r6 = max length
	jlss	8b
	movl	4(ap),r1		# r1 = kernel address
	movl	8(ap),r3		# r3 = user address
	bicl3	$~(NBPG*CLSIZE-1),r3,r2	# r2 = bytes on first page
	subl3	r2,$NBPG*CLSIZE,r2
1:
	cmpl	r6,r2			# r2 = min(bytes on page, length left);
	jgeq	2f
	movl	r6,r2
2:
	probew	$3,r2,(r3)		# bytes accessible?
	jeql	8b
	subl2	r2,r6			# update bytes left count
	locc	$0,r2,(r1)		# null byte found?
	jneq	3b
	subl2	r2,r1			# back up pointer updated by `locc'
	movc3	r2,(r1),(r3)		# copy in next piece
	movl	$(NBPG*CLSIZE),r2	# check next page
	tstl	r6			# run out of space?
	jneq	1b
	movl	$ENAMETOOLONG,r0	# set error code and return
	jbr	9b

/*
 * Copy a null terminated string from one point to another in
 * the kernel address space.
 *
 * copystr(fromaddr, toaddr, maxlength, &lencopied)
 */
ENTRY(copystr, R6)
	movl	12(ap),r6		# r6 = max length
	jlss	8b
	movl	4(ap),r1		# r1 = src address
	movl	8(ap),r3		# r3 = dest address
1:
	movzwl	$65535,r2		# r2 = bytes in first chunk
	cmpl	r6,r2			# r2 = min(bytes in chunk, length left);
	jgeq	2f
	movl	r6,r2
2:
	subl2	r2,r6			# update bytes left count
	locc	$0,r2,(r1)		# null byte found?
	jneq	3b
	subl2	r2,r1			# back up pointer updated by `locc'
	movc3	r2,(r1),(r3)		# copy in next piece
	tstl	r6			# run out of space?
	jneq	1b
	movl	$ENAMETOOLONG,r0	# set error code and return
	jbr	9b

/*
 * Copy specified amount of data from user space into the kernel
 * Copyin(from, to, len)
 *	r1 == from (user source address)
 *	r3 == to (kernel destination address)
 *	r5 == length
 */
	.globl	_Copyin 		# massaged to jsb, args R1 R3 R5
_Copyin:
#ifdef SMP_DEBUG
	tstl	_smp_debug
	beql	1f
	pushr	$0x3f
	calls	$0,_sleep_check
	popr	$0x3f
1:
#endif
	cmpl	r5,$(NBPG*CLSIZE)	# probing one page or less ?
	bgtru	1f			# no
	prober	$3,r5,(r1)		# bytes accessible ?
	beql	ersb			# no
	movc3	r5,(r1),(r3)
/*	clrl	r0			# redundant */
	rsb
1:
	blss	ersb			# negative length?
	pushl	r6			# r6 = length
	movl	r5,r6
	bicl3	$~(NBPG*CLSIZE-1),r1,r0 # r0 = bytes on first page
	subl3	r0,$(NBPG*CLSIZE),r0
	addl2	$(NBPG*CLSIZE),r0	# plus one additional full page
	jbr	2f

ciloop:
	movc3	r0,(r1),(r3)
	movl	$(2*NBPG*CLSIZE),r0	# next amount to move
2:
	cmpl	r0,r6
	bleq	3f
	movl	r6,r0
3:
	prober	$3,r0,(r1)		# bytes accessible ?
	beql	ersb1			# no
	subl2	r0,r6			# last move?
	bneq	ciloop			# no

	movc3	r0,(r1),(r3)
/*	clrl	r0			# redundant */
	movl	(sp)+,r6		# restore r6
	rsb

ersb1:
	movl	(sp)+,r6		# restore r6
ersb:
	movl	$EFAULT,r0
	rsb

/*
 * Copy specified amount of data from kernel to the user space
 * Copyout(from, to, len)
 *	r1 == from (kernel source address)
 *	r3 == to (user destination address)
 *	r5 == length
 */
	.globl	_Copyout		# massaged to jsb.  Args R1 R3 R5
_Copyout:
#ifdef SMP_DEBUG
	tstl	_smp_debug
	beql	1f
	pushr	$0x3f
	calls	$0,_sleep_check
	popr	$0x3f
1:
#endif
	cmpl	r5,$(NBPG*CLSIZE)	# moving one page or less ?
	bgtru	1f			# no
	probew	$3,r5,(r3)		# bytes writeable?
	beql	ersb			# no
	movc3	r5,(r1),(r3)
/*	clrl	r0			# redundant */
	rsb
1:
	blss	ersb			# negative length?
	pushl	r6			# r6 = length
	movl	r5,r6
	bicl3	$~(NBPG*CLSIZE-1),r3,r0 # r0 = bytes on first page
	subl3	r0,$(NBPG*CLSIZE),r0
	addl2	$(NBPG*CLSIZE),r0	# plus one additional full page
	jbr	2f

coloop:
	movc3	r0,(r1),(r3)
	movl	$(2*NBPG*CLSIZE),r0	# next amount to move
2:
	cmpl	r0,r6
	bleq	3f
	movl	r6,r0
3:
	probew	$3,r0,(r3)		# bytes writeable?
	beql	ersb1			# no
	subl2	r0,r6			# last move?
	bneq	coloop			# no

	movc3	r0,(r1),(r3)
/*	clrl	r0			# redundant */
	movl	(sp)+,r6		# restore r6
	rsb

/*
 * Interrupt vector routines
 */
	.globl	_waittime

#define SCBVEC(name)	.align 2; .globl _X/**/name; _X/**/name
#define PANIC(msg)	clrl _waittime; pushab 1f; \
			calls $1,_panic; 1: .asciz msg
#define PRINTF(n,msg)	pushab 1f; calls $n+1,_printf; M_MSG(msg)
#define M_MSG(msg)	  .data; 1: .asciz msg; .text
#define PUSHR		pushr $0x3f
#define POPR		popr $0x3f
#define SBI0	0
#define SBI1	1
#define SBI_ERROR 0
#define SBI_WTIME 1
#define SBI_ALERT 2
#define SBI_FLT 3
#define SBI_FAIL 4

SCBVEC(machcheck):
	PUSHR; pushab 6*4(sp); calls $1,_machinecheck; POPR;
	addl2 (sp)+,sp; rei
SCBVEC(kspnotval):
	PUSHR; PANIC("KSP not valid");
SCBVEC(powfail):
	halt
SCBVEC(chme): SCBVEC(chms): SCBVEC(chmu):
	PUSHR; PANIC("CHM? in kernel");
SCBVEC(cmrd):
	PUSHR; calls $0,_memerr; POPR; rei
SCBVEC(wtime):
	PUSHR
	pushal 6*4(sp)
	cmpl	$VAX_8600,_cpu
	bneq	2f		# 2f because 1: used in panic macro
	pushl $SBI_ERROR	# 0 - sbierror type
	pushl $SBI0		# sbi0
	calls $3, _logsbi	#log 8600 sbi error 
	POPR
	PANIC("sbia0error")
2:
	cmpl	$VAX_60,_cpu
	beql	3f		# 3f - Firefox memerr interrtup
	pushl $SBI_WTIME		# 1 - wtime error type
	pushl $SBI0		# sbi0
	calls $3, _logsbi	#log non-8600 sbi error
	cmpl	$VAX_6400,_cpu	# if VAX6400, don't panic, we recovered.
	beql	5f
	cmpl	$VAX_6200,_cpu
	beql	5f

	PRINTF(1,"write timeout %x\n"); POPR;
	PANIC("wtimo")
3:
	calls $0,_ka60memerr	# Handle Firefox Mbus memerr interrupts
	addl2	$4,sp		# clean off the pushal above
5:
	POPR
	rei


SCBVEC(sbi0alert):
	cmpl	$VAX_8200,_cpu
	bneq	2f
	PUSHR
	calls	$0,_ka820rxcd
	POPR
	rei
	
2:	
	PUSHR; pushal 6*4(sp); pushl $SBI_ALERT; pushl $SBI0; calls $3, _logsbi; POPR; 
	PANIC("sbi0alert")

SCBVEC(sbi0flt):
	cmpl	$VAX_8820,_cpu
	beql	3f
	cmpl	$VAX_8800,_cpu
	bneq	2f
3:
	PUSHR
        pushab  6*4(sp)                  # make pc/psl an argument
	calls	$1,_ka8800nmifault       # for 8800 mchk changes
	POPR
	rei

2:
	PUSHR; pushal 6*4(sp); pushl $SBI_FLT; pushl $SBI0; calls $3, _logsbi; POPR;
	PANIC("sbi0flt")
SCBVEC(sbi0fail):
	PUSHR; pushal 6*4(sp); pushl $SBI_FAIL; pushl $SBI0; calls $3, _logsbi; POPR;
	PANIC("sbi0fail")

SCBVEC(sbi1fail):
	PUSHR; pushal 6*4(sp); pushl $SBI_FAIL; pushl $SBI1; calls $3, _logsbi; POPR; 
	PANIC("sbi1fail")
SCBVEC(sbi1alert):
	PUSHR; pushal 6*4(sp); pushl $SBI_ALERT; pushl $SBI1; calls $3, _logsbi; POPR; 
	PANIC("sbi1alert")
SCBVEC(sbi1flt):
	PUSHR; pushal 6*4(sp); pushl $SBI_FLT; pushl $SBI1; calls $2, _logsbi; POPR; 
	PANIC("sbi1flt")
SCBVEC(sbi1error):
	PUSHR; pushal 6*4(sp); pushl $SBI_ERROR; pushl $SBI1; calls $3, _logsbi; POPR; 
	PANIC("sbia1error")

#if NMBA > 0
SCBVEC(mba3int):
	PUSHR; pushl $3; brb 1f
SCBVEC(mba2int):
	PUSHR; pushl $2; brb 1f
SCBVEC(mba1int):
	PUSHR; pushl $1; brb 1f
SCBVEC(mba0int):
	PUSHR; pushl $0
1:	calls $1,_mbintr
	mfpr 	$ESP,r0
	incl	CPU_INTR(r0)
	POPR
	rei
#endif

#if defined(VAX780) || defined (VAX8600)

/*
 * Registers for the uba handling code
 */
#define rUBANUM r0
#define rUBAHD	r1
#define rUVEC	r3
#define rUBA	r4
#define rUBAPC  r5
/* r2 are scratch */

SCBVEC(ua6int):
	PUSHR; moval 6*4(sp), rUBAPC; movl $6,rUBANUM; moval _uba_hd+(6*UH_SIZE),rUBAHD; brb 1f
SCBVEC(ua5int):
	PUSHR; moval 6*4(sp), rUBAPC; movl $5,rUBANUM; moval _uba_hd+(5*UH_SIZE),rUBAHD; brb 1f
SCBVEC(ua4int):
	PUSHR; moval 6*4(sp), rUBAPC; movl $4,rUBANUM; moval _uba_hd+(4*UH_SIZE),rUBAHD; brb 1f
SCBVEC(ua3int):
	PUSHR; moval 6*4(sp), rUBAPC; movl $3,rUBANUM; moval _uba_hd+(3*UH_SIZE),rUBAHD; brb 1f
SCBVEC(ua2int):
	PUSHR; moval 6*4(sp), rUBAPC; movl $2,rUBANUM; moval _uba_hd+(2*UH_SIZE),rUBAHD; brb 1f
SCBVEC(ua1int):
	PUSHR; moval 6*4(sp), rUBAPC; movl $1,rUBANUM; moval _uba_hd+(1*UH_SIZE),rUBAHD; brb 1f
SCBVEC(ua0int):
	PUSHR; moval 6*4(sp), rUBAPC; movl $0,rUBANUM; moval _uba_hd+(0*UH_SIZE),rUBAHD;

1:
	mfpr 	$ESP,r2
	incl	CPU_INTR(r2)
	mfpr	$IPL,r2 			/* r2 = mfpr(IPL); */
	movl	UH_UBA(rUBAHD),rUBA		/* uba = uhp->uh_uba; */
	movl	UBA_BRRVR-0x14*4(rUBA)[r2],rUVEC
					/* uvec = uba->uba_brrvr[r2-0x14] */
ubanorm:
	bleq	ubaerror
	addl2	UH_VEC(rUBAHD),rUVEC		/* uvec += uh->uh_vec */
	bicl3	$3,(rUVEC),r1
	jmp	2(r1)				/* 2 skips ``pushr $0x3f'' */
ubaerror:
	tstl	rUVEC				/* set condition codes */
	beql	zvec				/* branch if zero vector */
ubaerr1:
	PUSHR; calls $0,_ubaerror; POPR 	/* ubaerror r/w's r0-r5 */
	tstl rUVEC; jneq ubanorm		/* rUVEC contains result */
	brb	rtrn	
zvec:
	tstl	UH_ZVFLG(rUBAHD)		/* zero vector timer flg set? */
	beql	ubaerr1				/* yes -- inc counter here */
						/* no  -- call ubaerror */
	incl	UH_ZVCNT(rUBAHD)		/* bump zero vector counter */
rtrn:
	POPR
	rei

#endif VAX780 || VAX8600

/* MSI interrupt routine */

#if NMSI > 0
SCBVEC(msi0int):
        movq    r0,-(sp)
        movq    r2,-(sp)
        movq    r4,-(sp)
	pushl	$0
        calls	$1,_msi_isr
	mfpr $ESP,r0
	incl CPU_INTR(r0)
        movq    (sp)+,r4
        movq    (sp)+,r2
        movq    (sp)+,r0
	rei
#endif

/* Console xmit/rcv interrupt routines */

SCBVEC(cnrint):
	PUSHR; pushl $0; calls $1,_cnrint; mfpr $ESP,r0; incl CPU_INTR(r0);
	POPR; rei
SCBVEC(cnxint):
	PUSHR; pushl $0; calls $1,_cnxint; mfpr $ESP,r0; incl CPU_INTR(r0);
	POPR; rei

/* 3 Serial lines on VAX8200 == cpu type #5 */
/* pushl $1,$2,$3 is the index into the struct tty cons array */

SCBVEC(cnrint1):
	PUSHR
	cmpl $VAX_8200,_cpu
	bneq 1f
	pushl $1
	calls $1,_cnrint
1:
	mfpr $ESP,r0; incl CPU_INTR(r0);
	POPR
	rei
SCBVEC(cnxint1):
	PUSHR
	cmpl $VAX_8200,_cpu
	bneq 1f
	pushl $1
	calls $1,_cnxint
1:
	mfpr $ESP,r0; incl CPU_INTR(r0);
	POPR
	rei
SCBVEC(cnrint2):
	PUSHR
	cmpl $VAX_8200,_cpu
	bneq 1f
	pushl $2
	calls $1,_cnrint
1:
	mfpr $ESP,r0; incl CPU_INTR(r0);
	POPR
	rei
SCBVEC(cnxint2):
	PUSHR
	cmpl	_cpu,$VAX_3400
	jneq	1f
	pushl	$0
	calls	$1,_lnintr
1:
	cmpl	_cpu,$VAX_8200
	jneq	1f
	pushl	$2
	calls	$1,_cnxint
1:	
	mfpr $ESP,r0; incl CPU_INTR(r0);
	POPR
	rei
SCBVEC(cnrint3):
	PUSHR
	cmpl $VAX_8200,_cpu
	bneq 1f
	pushl $3
	calls $1,_cnrint
1:
	mfpr $ESP,r0; incl CPU_INTR(r0);
	POPR
	rei
SCBVEC(cnxint3):
	PUSHR
	cmpl $VAX_8200,_cpu
	bneq 1f
	pushl $3
	calls $1,_cnxint
1:
	mfpr $ESP,r0; incl CPU_INTR(r0);
	POPR
	rei

SCBVEC(hardclock):
	PUSHR
	mtpr $ICCS_RUN|ICCS_IE|ICCS_INT|ICCS_ERR,$ICCS
	pushl 4+6*4(sp); pushl 4+6*4(sp);
	calls $2,_hardclock			# hardclock(pc,psl)
#if NPS > 0
	tstb	_nNPS
	beql	1f
	pushl	4+6*4(sp); pushl 4+6*4(sp);
	calls	$2,_psextsync
#endif
1:	POPR;
	rei

SCBVEC(intqueue):			# Entry point for SIRR 7's
	PUSHR				#  @ IPL 7
	calls	$0,_intqueue
	POPR
	rei
SCBVEC(softclock):
	PUSHR
	pushl	4+6*4(sp); pushl 4+6*4(sp)
	calls	$2,_softclock			# softclock(pc,psl)
	POPR
	rei
#include "../net/net/netisr.h"
	.globl	_netisr
SCBVEC(netintr):
	pushr	$0x83f
	moval	_netisr_tab, r11     	# while (netisr_tab[i] >= 0)
startwhile:
	tstl	(r11)
	jlss	endwhile
	bbcc	(r11)+,_netisr,1f;  calls $0,*(r11)+; jbr startwhile; 1:	
	tstl	(r11)+
	jbr 	startwhile
endwhile:
	popr	$0x83f
	rei
SCBVEC(consdin):
	PUSHR;
	casel	_cpu,$1,$CPU_MAX
0:
	.word	1f-0b		# 1 is 780
	.word	3f-0b		# 2 is 750
	.word	3f-0b		# 3 is 730
	.word	4f-0b		# 4 is 8600
	.word	5f-0b		# 5 is 8200
	.word	1f-0b		# 6 is 8800
	.word	1f-0b		# 7 is MVAX_I
	.word	1f-0b		# 8 is MVAX_II
	.word	1f-0b		# 9 is VVAX
	.word	1f-0b		# 10 is VAX_3600 (Mayfair I)
	.word	1f-0b		# 11 is VAX6200 (Calypso CVAX)
	.word	1f-0b		# 12 is VAX_3400 (Mayfair II)
	.word	1f-0b		# 13 is C_VAXSTAR (PVAX)
	.word	1f-0b		# 14 is VAX_60 (Firefox)
	.word	1f-0b		# 15 is VAX_3400 (Mayfair II)
	.word	1f-0b		# 16 RESERVED for MIPS processor
	.word	1f-0b		# 17 is VAX_8820 (Polarstar)
	.word	1f-0b		# 18 RESERVED for MIPS processor
	.word	1f-0b		# 19 RESERVED for MIPS processor
	.word	1f-0b		# 20 RESERVED for MIPS processor
	.word	1f-0b		# 21 RESERVED for MIPS processor
	.word	1f-0b		# 22 is VAX_6400 (Calypso Rigel)
	.word	1f-0b		# 23 is VAXSTAR
	.word	1f-0b		# 24 RESERVED for MIPS processor
	.word	1f-0b		# 25 RESERVED for MIPS processor
	.word	6f-0b		# 26 is VAX_9000

1:	halt
#if defined(VAX750) || defined(VAX730)
2:	jsb	 tudma		 # 750
/*	mfpr	$CSRD,r5         # If using new tudma code
	pushl	$0
	pushl	r5
	pushal	_turintr
	calls	$3,_chrqueue
	brb	1f    */
3:	calls $0,_turintr	# 750/730
#else
2:
3:
#endif
	brb 1f
4:
#if defined(VAX8600)
	calls $0,_crlintr	# 8600
#endif
	brb 1f
5:
#if defined(VAX8200)
	calls $0,_rx5_intr	#8200
#endif
	brb 1f
6:
#if defined(VAX9000)
        calls   $0,_ka9000_rxfct_isr    # 9000
#endif

1:
	mfpr $ESP,r0; incl CPU_INTR(r0);
	POPR
	rei
SCBVEC(consdout):
	PUSHR;
	casel	_cpu,$1,$CPU_MAX
0:
	.word	1f-0b		# 1 is VAX_780
	.word	2f-0b		# 2 is VAX_750
	.word	2f-0b		# 3 is VAX_730
	.word	1f-0b		# 4 is VAX_8600
	.word	1f-0b		# 5 is VAX_8200
	.word	1f-0b		# 6 is VAX_8800
	.word	1f-0b		# 7 is MVAX_I
	.word	1f-0b		# 8 is MVAXII
	.word	1f-0b		# 9 is VVAX
	.word	1f-0b		# 10 is VAX_3600 (Mayfair I)
	.word	1f-0b		# 11 is VAX6200 (Calypso CVAX)
	.word	1f-0b		# 12 is VAX_3400 (Mayfair II)
	.word	1f-0b		# 13 is C_VAXSTAR (PVAX)
	.word	1f-0b		# 14 is VAX_60 (Firefox)
	.word	1f-0b		# 15 is VAX_3400 (Mayfair II)
	.word	1f-0b		# 16 RESERVED for MIPS processor
	.word	1f-0b		# 17 is VAX_8820 (Polarstar)
	.word	1f-0b		# 18 RESERVED for MIPS processor
	.word	1f-0b		# 19 RESERVED for MIPS processor
	.word	1f-0b		# 20 RESERVED for MIPS processor
	.word	1f-0b		# 21 RESERVED for MIPS processor
	.word	1f-0b		# 22 is VAX_6400 (Calypso Rigel)
	.word	1f-0b		# 23 is VAXSTAR
	.word	1f-0b		# 24 RESERVED for MIPS processor
	.word	1f-0b		# 25 RESERVED for MIPS processor
	.word	1f-0b		# 26 is VAX_9000
1:	halt
2:
#if defined(VAX750) || defined(VAX730)
	calls $0,_tuxintr	# 750/730
#endif
        brb     1f

1:
#if defined(VAX750) || defined(VAX730) || defined(VAX9000)
	mfpr $ESP,r0; incl CPU_INTR(r0);
	POPR
	rei
#endif

#if NDZ > 0
/*
 * DZ pseudo dma routine:
 *	r0 - controller number
 */
	.align	1
	.globl	dzdma
dzdma:
	mull2	$8*20,r0
	movab	_dzpdma(r0),r3		# pdma structure base
					# for this controller
dzploop:
	movl	r3,r0
	movl	(r0)+,r1		# device register address
	movzbl	1(r1),r2		# get line number
	bitb	$0x80,r2		# TRDY on?
	beql	dzprei			# no
	bicb2	$0xf8,r2		# clear garbage bits
	mull2	$20,r2
	addl2	r2,r0			# point at line's pdma structure
	movl	(r0)+,r2		# p_mem
	cmpl	r2,(r0)+		# p_mem < p_end ?
	bgequ	dzpcall 		# no, go call dzxint
	movb	(r2)+,6(r1)		# dztbuf = *p_mem++
	movl	r2,-8(r0)
	brb	dzploop 		# check for another line
dzprei:
	POPR
	incl	_cnt+V_PDMA
	rei

dzpcall:
	pushl	r3
	pushl	(r0)+			# push tty address
	calls	$1,*(r0)		# call interrupt rtn
	movl	(sp)+,r3
	brb	dzploop 		# check for another line
#endif

#if NSS > 0
/*
 * VAXstar SLU pseudo dma routine:
 *	r0 - controller number
 * CAUTION:
 *	The VAXstar system unit spec says that byte reads
 *	of the SLU CSR are not allowed. This is not an absolute
 *	restriction. It would only cause problems if there were
 *	bits in the CSR which were modified by reading.
 */
	.align	1
	.globl	ssdma
ssdma:
	mull2	$8*20,r0
	movab	_sspdma(r0),r3		# pdma structure base
					# for this controller
ssploop:
	movl	r3,r0
	movl	(r0)+,r1		# device register address
	movzbl	1(r1),r2		# get line number
	bitb	$0x80,r2		# TRDY on?
	beql	ssprei			# no
	bicb2	$0xfc,r2		# clear garbage bits
	mull2	$20,r2
	addl2	r2,r0			# point at line's pdma structure
	movl	(r0)+,r2		# p_mem
	cmpl	r2,(r0)+		# p_mem < p_end ?
	bgequ	sspcall 		# no, go call ssxint
	movb	(r2)+,12(r1)		# sstbuf = *p_mem++
	movl	r2,-8(r0)
	brb	ssploop 		# check for another line
ssprei:
	POPR
	incl	_cnt+V_PDMA
	rei

sspcall:
	pushl	r3
	pushl	(r0)+			# push tty address
	calls	$1,*(r0)		# call interrupt rtn
	movl	(sp)+,r3
	brb	ssploop 		# check for another line
#endif

#if NFC > 0
/*
 * Firefox SLU pseudo dma routine:
 *	r0 - controller number
 * CAUTION:
 *	The Firefox system unit spec says that byte reads
 *	of the SLU CSR are not allowed. This is not an absolute
 *	restriction. It would only cause problems if there were
 *	bits in the CSR which were modified by reading.
 */
	.align	1
	.globl fcdma
fcdma:
	pushl	$0
	pushl	r0
	pushal	fcdma2
	calls	$3,_chrqueue
	POPR
	rei


	.align 1
	.globl fcdma2
fcdma2:
	.word	0
	movab	_fcpdma, r3		# base addr of pdma structure
fcploop:
	movl	r3,r0			# get base of pdma
	movl	(r0)+,r1		# device register address
	movw	$0x20,(r1)		# disable RIE, TIE
	movzbl	(r1),r2			# get line number
	bitb	$0x80,r2		# RDONE on?
	bneq	fcprint			# yes, go call fcrint
	movzbl	1(r1),r2		# get line number
	bitb	$0x80,r2		# TRDY on?
	beql	fcprei			# no
	bicb2	$0xfc,r2		# clear garbage bits
	mull2	$20,r2
	addl2	r2,r0			# point at line's pdma structure
	movl	(r0)+,r2		# p_mem
	cmpl	r2,(r0)+		# p_mem < p_end ?
	bgequ	fcpxint 		# no, go call fcxint
	movb	(r2)+,12(r1)		# fctbuf = *p_mem++
	movl	r2,-8(r0)
	brb	fcploop 		# check for another line
fcprei:
	incl	_cnt+V_PDMA
	movw	$0x4060,(r1)		# enable reciever interrupts (RIE)
	ret
fcpxint:
	pushl	r3
	pushl	(r0)+			# push tty address
	calls	$1,*(r0)		# call interrupt rtn
	movl	(sp)+,r3
	brb	fcploop 		# check for another line

fcprint:
	pushl	r3
	pushl	$0			# controller 0
	calls	$1,_fcrint		# call fcrint
	movl	(sp)+,r3
	brb	fcploop 		# check for another line or TRDY
#endif
#if NUU > 0 && defined(UUDMA)
/*
 * Pseudo DMA routine for tu58 (on DL11)
 *	r0 - controller number
 */
	.align	1
	.globl	uudma
uudma:
	movl	_uudinfo[r0],r2
	movl	16(r2),r2		# r2 = uuaddr
	mull3	$48,r0,r3
	movab	_uu_softc(r3),r5	# r5 = uuc

	cvtwl	2(r2),r1		# c = uuaddr->rdb
	bbc	$15,r1,1f		# if (c & UUDB_ERROR)
	movl	$13,16(r5)		#	uuc->tu_state = TUC_RCVERR;
	rsb				#	let uurintr handle it
1:
	tstl	4(r5)			# if (uuc->tu_rcnt) {
	beql	1f
	movb	r1,*0(r5)		#	*uuc->tu_rbptr++ = r1
	incl	(r5)
	decl	4(r5)			#	if (--uuc->tu_rcnt)
	beql	2f			#		done
	tstl	(sp)+
	POPR				#	registers saved in ubglue.s
	rei				# }
2:
	cmpl	16(r5),$8		# if (uuc->tu_state != TUS_GETH)
	beql	2f			#	let uurintr handle it
1:
	rsb
2:
	mull2	$14,r0			# sizeof(uudata[ctlr]) = 14
	movab	_uudata(r0),r4		# data = &uudata[ctlr];
	cmpb	$1,(r4) 		# if (data->pk_flag != TUF_DATA)
	bneq	1b
#ifdef notdef
	/* this is for command packets */
	beql	1f			#	r0 = uuc->tu_rbptr
	movl	(r5),r0
	brb	2f
1:					# else
#endif
	movl	24(r5),r0		#	r0 = uuc->tu_addr
2:
	movzbl	1(r4),r3		# counter to r3 (data->pk_count)
	movzwl	(r4),r1 		# first word of checksum (=header)
	mfpr	$IPL,-(sp)		# s = spl5();
	mtpr	$0x15,$IPL		# to keep disk interrupts out
	clrw	(r2)			# disable receiver interrupts
3:	bbc	$7,(r2),3b		# while ((uuaddr->rcs & UUCS_READY)==0);
	cvtwb	2(r2),(r0)+		# *buffer = uuaddr->rdb & 0xff
	sobgtr	r3,1f			# continue with next byte ...
	addw2	2(r2),r1		# unless this was the last (odd count)
	brb	2f

1:	bbc	$7,(r2),1b		# while ((uuaddr->rcs & UUCS_READY)==0);
	cvtwb	2(r2),(r0)+		# *buffer = uuaddr->rdb & 0xff
	addw2	-2(r0),r1		# add to checksum..
2:
	adwc	$0,r1			# get the carry
	sobgtr	r3,3b			# loop while r3 > 0
/*
 * We're ready to get the checksum
 */
1:	bbc	$7,(r2),1b		# while ((uuaddr->rcs & UUCS_READY)==0);
	cvtwb	2(r2),12(r4)		# get first (lower) byte
1:	bbc	$7,(r2),1b
	cvtwb	2(r2),13(r4)		# ..and second
	cmpw	12(r4),r1		# is checksum ok?
	beql	1f
	movl	$14,16(r5)		# uuc->tu_state = TUS_CHKERR
	brb	2f			# exit
1:
	movl	$11,16(r5)		# uuc->tu_state = TUS_GET (ok)
2:
	movw	$0x40,(r2)		# enable receiver interrupts
	mtpr	(sp)+,$IPL		# splx(s);
	rsb				# continue processing in uurintr
#endif

#if defined(VAX750) || defined(BINARY)
/*
 * Pseudo DMA routine for VAX-11/750 console tu58
 *	    (without MRSP)
 */
	.align	1
	.globl	tudma
tudma:
	movab	_tu,r5			# r5 = tu
	tstl	4(r5)			# if (tu.tu_rcnt) {
	beql	3f
	mfpr	$CSRD,r1		# get data from tu58
	movb	r1,*0(r5)		#	*tu.tu_rbptr++ = r1
	incl	(r5)
	decl	4(r5)			#	if (--tu.tu_rcnt)
	beql	1f			#		done
	tstl	(sp)+
	POPR				#	registers saved in ubglue.s
	rei				#	data handled, done
1:					# }
	cmpl	16(r5),$8		# if (tu.tu_state != TUS_GETH)
	beql	2f			#	let turintr handle it
3:
	rsb
2:
	movab	_tudata,r4		# r4 = tudata
	cmpb	$1,(r4) 		# if (tudata.pk_flag != TUF_DATA)
	bneq	3b			#	let turintr handle it
1:					# else
	movl	24(r5),r1		# get buffer pointer to r1
	movzbl	1(r4),r3		# counter to r3
	movzwl	(r4),r0 		# first word of checksum (=header)
	mtpr	$0,$CSRS		# disable receiver interrupts
3:
	bsbw	5f			# wait for next byte
	mfpr	$CSRD,r5
	movb	r5,(r1)+		# *buffer = rdb
	sobgtr	r3,1f			# continue with next byte ...
	mfpr	$CSRD,r2		# unless this was the last (odd count)
	brb	2f

1:	bsbw	5f			# wait for next byte
	mfpr	$CSRD,r5
	movb	r5,(r1)+		# *buffer = rdb
	movzwl	-2(r1),r2		# get the last word back from memory
2:
	addw2	r2,r0			# add to checksum..
	adwc	$0,r0			# get the carry
	sobgtr	r3,3b			# loop while r3 > 0
/*
 * We're ready to get the checksum.
 */
	bsbw	5f
	movab	_tudata,r4
	mfpr	$CSRD,r5
	movb	r5,12(r4)		# get first (lower) byte
	bsbw	5f
	mfpr	$CSRD,r5
	movb	r5,13(r4)		# ..and second
	movab	_tu,r5
	cmpw	12(r4),r0		# is checksum ok?
	beql	1f
	movl	$14,16(r5)		# tu.tu_state = TUS_CHKERR
	brb	2f			# exit
1:
	movl	$11,16(r5)		# tu.tu_state = TUS_GET
2:
	mtpr	$0x40,$CSRS		# enable receiver interrupts
	rsb				# continue processing in turintr
/*
 * Loop until a new byte is ready from
 * the tu58, make sure we don't loop forever
 */
5:
	movl	$5000,r5		# loop max 5000 times
1:
	mfpr	$CSRS,r2
	bbs	$7,r2,1f
	sobgtr	r5,1b
	movab	_tu,r5
	movl	$13,16(r5)		# return TUS_RCVERR
	tstl	(sp)+			# and let turintr handle it
1:
	rsb
#endif

/*
 * Stray SCB interrupt catcher
 */
.text
SCBVEC(passrel):
	rei

	.data
	.align	2
#define PJ	PUSHR;jsb _Xstray
	.globl	_scb_stray
_scb_stray:
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
#if defined(VAX8600) || defined(VAX9000)
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
#endif VAX8600 || VAX9000
#if defined(VAX9000)
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
#endif VAX9000
#undef PJ
	.text
SCBVEC(stray):
	/*
	 * Calculate and report the vector location of the stray
	 * scb interrupt
	 */
	subl3	$_scb_stray+8,(sp)+,r0
	ashl	$-1,r0,-(sp)
	mfpr	$IPL,-(sp)
	pushl	$1		#ELSI_SCB
	calls $3, _logstray;	#log stray interrupt
	POPR
	rei

/*
 * Stray UNIBUS interrupt catch routines
 */
	.data
	.align	2
#define PJ	PUSHR;jsb _Xustray
	.globl	_catcher
_catcher:
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
#if defined(VAX9000)
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
#endif VAX9000

	.globl	_cold
_cold:	.long	1

	.globl	_br
_br:	.long 	0

	.globl	_cvec
_cvec:	.long	0
	.data

	.text
SCBVEC(ustray):
	blbc	_cold,1f
	mfpr	$IPL,_br
	subl3	$_catcher+8,(sp)+,_cvec
	ashl	$-1,_cvec,_cvec
	POPR
	rei
1:
	subl3	$_catcher+8,(sp)+,r0
	ashl	$-1,r0,-(sp)
	mfpr	$IPL,-(sp)
	pushl	$2		#ELSI_UNI
	calls $3, _logstray;	#log stray interrupt
	POPR
	rei

/*
 * Trap and fault vector routines
 */
#define TRAP(a) pushl $T_/**/a; jbr alltraps

/*
 * Ast delivery (profiling and/or reschedule)
 */
SCBVEC(astflt):
	pushl $0; TRAP(ASTFLT)
SCBVEC(privinflt):
/*
 *	Check to make sure that we either have the floationg emulation code
 *	linked in or the entire emulation code
 */
	tstl	_vaxopcdec		# floating point emulation load ?
	bneq	2f			# br if yes
	tstl	_vaxemulbegin		# Is any emulation loaded in?
	beql	1f			# br if no
	bneq	3f			# br if yes

2:	jmp	*_vaxopcdec		# See if it is one that we can emulate
	.globl	_Xprivinflt1
_Xprivinflt1:				# Special symboll used by vax$opcdec if
					# instruction can not be emulated.
/*
 *	We check for the vax$special because this is the way the emulation
 *	code changes it's current mode to kernel
 */
3:
	cmpl	_vaxspecialhalt,(sp)	# Is this the special address
	bneq	1f			# br if no
	addl2	$(4*2),sp		# Pop all of the priv stuff off and
	jmp	*_vaxspecialcont	# jmp to emulation code to continue
					# it's ways
1:
	pushl $0; TRAP(PRIVINFLT)
SCBVEC(xfcflt):
	pushl $0; TRAP(XFCFLT)
SCBVEC(resopflt):
	pushl $0; TRAP(RESOPFLT)
SCBVEC(resadflt):
	pushl $0; TRAP(RESADFLT)
SCBVEC(bptflt):
	pushl $0; TRAP(BPTFLT)
SCBVEC(compatflt):
	TRAP(COMPATFLT);
SCBVEC(tracep):
	pushl $0; TRAP(TRCTRAP)
SCBVEC(arithtrap):
	TRAP(ARITHTRAP)
SCBVEC(protflt):
/* this is a 780 fix gotton from our friends in VMS land.  It
 * seems that the VAX780 sometimes falsely claims an access violation.  This
 * code does a probe to make sure it is really an access violation.  If it
 * is a false one, we just REI, else it continues to the trap code.
 */
#ifdef VAX780
	cmpl	$VAX_780,_cpu		# only do if 780
	bneq	3f

	bitl	$4,(sp)			# test if modify 
	beql	1f

	probew	$0,$1,4(sp)		# modify...do write check...
	beql	3f
	addl2 $8,sp			# if the probe passed then 
	rei				# this is 780 bug....retry

1:
	prober	$0,$1,4(sp)		# read ... do read check...
	beql	3f	
	addl2 $8,sp			# if the probe passed then
	rei				# this is 780 bug...retry

3:
#endif VAX780

	tstl	_vaxemulbegin		# Emulation loaded in?
	beql	2f			# br if no
/*
 *	Check to see if the pc is in the emulation space, if so go
 *	to the emulation handler for the exception
 */
	cmpl	8(sp),_vaxemulbegin	# If less the vax$emul_being
	blssu	1f			# then br
	cmpl	8(sp),_vaxemulend	# If greater than vax$emul_end
	bgequ	1f			# then br
	jmp	*_exeacviolat		# else jmp to the emulator's
					# exception handler
1:
	.globl	_Xprotflt1
_Xprotflt1:				# Special symbol used to interface
					# the emulation code to ultrix
					# signalling
2:

	movl	r0,-(sp)		# Save r0 (caller was using it...)
	movl	4(sp),r0		# Get the fault type

/*	For now, panic on any vector async fault, since none of the
 *	currently-supported vector processors supports async
 *	memory management.  THIS WILL HAVE TO CHANGE IF WE GET
 *	AN ASYNC VECTOR PROCESSOR TO SUPPORT!!!
 */
	bitl	$VM_FLT_VAS,r0
	beql	3f
	pushab	1f
	calls	$1,_panic
	halt
1:	.asciz	"Async vector memory exception"

3:	bitl	$VM_FLT_P,r0		# Pte reference (table flt)
	bneq	Xtableflt

/*	Length violation or vector I/O reference get reported
 *	as seg faults.
 */

	bitl	$VM_FLT_L|VM_FLT_VIO,r0
	bneq	Xsegflt

/*	Vector alignment errors or write protection errors
 *	get reported as protection faults (which get reported
 *	to the user as SIGBUS)
 */
	bitl	$VM_FLT_VAL|VM_FLT_M,r0
	bneq	4f

/*	We tested all the defined bits in this word... so
 *	either the word is 0 (which indicates a read protection
 *	fault), or else it's got bits we don't know about!
 * 	Panic in the latter case...
 */
	cmpl	r0,$0
	beql	5f

	pushab	1f			# Not zero; panic!
	calls	$1,_panic
1:	.asciz	"Unknown memory management trap"

4:
	movl	(sp)+,r0		# Restore r0
	addl2	$4,sp			# Pop off trap type
	TRAP(PROTFLT)
5:
	movl	(sp)+,r0		# Restore r0
	addl2	$4,sp			# Pop off trap type
	TRAP(NOACCESS)

Xsegflt:
	movl	(sp)+,r0		# Restore r0
	addl2	$4,sp			# Pop off trap type
segflt:
	TRAP(SEGFLT)

SCBVEC(transflt):
	bitl	$2,(sp)+
	bnequ	tableflt
	jsb	Fastreclaim		# try and avoid pagein
	TRAP(PAGEFLT)

Xtableflt:
	movl	(sp)+,r0		# Restore r0
	addl2	$4,sp			# Pop off trap type
tableflt:
	TRAP(TABLEFLT)

alltraps:
	mfpr	$USP,-(sp); calls $0,_trap; mtpr (sp)+,$USP
	addl2	$8,sp			# pop type, code
	mtpr	$HIGH,$IPL		## dont go to a higher IPL (GROT)
	rei

SCBVEC(syscall):
	pushl	$T_SYSCALL
	mfpr	$USP,-(sp); calls $0,_syscall; mtpr (sp)+,$USP
	addl2	$8,sp			# pop type, code
	mtpr	$HIGH,$IPL		## dont go to a higher IPL (GROT)
	rei

SCBVEC(ipintr):
	PUSHR
	calls	$0,_cpu_ip_intr
	POPR
	rei


#ifdef VAX_60
SCBVEC(ka60memerr):
	PUSHR;
	calls	$0,_ka60memerr;
	POPR;
	rei
#endif VAX_60

/*
 * Vector Processor Disabled Fault
 */
SCBVEC(vdisflt):
	pushl $0
	TRAP(VDISFLT)

/*
 * Initialization
 *
 * ipl 0x1f; mapen 0; scbb, pcbb, sbr, slr, isp, ksp not set
 */
	.globl	_slavestart
_slavestart:
	cmpl	$VAX_6400,_cpu		# is this rigel/mariah?
	bneq	1f			# if no, jump over ACCS write.

	cmpl	$MARIAH_VARIANT,_cpu_sub_subtype
	bneq	1f			# skip for rigel 

	pushr	$0x2			# save r2
	mfpr	$ACCS,r2		# get copy of ACCS
	bicl2	$4,r2			# clear bit #2 to put Mariah into
					# 30-bit mode.  (don't want to change
 					# values of FPA and Vector present bits
	mtpr	r2,$ACCS		# write new ACCS
	movl	(sp)+,r2		# restore r2

1:	# mtpr	$0,$TXCS		# shut down transmit interrupts
	mtpr	$0,$ICCS
	mtpr	$_scb-0x80000000,$SCBB
	mtpr	$_Sysmap-0x80000000,$SBR
	mtpr	$_Syssize,$SLR
/* double map the kernel into the virtual user addresses of phys mem */
	mtpr	$_Sysmap,$P0BR
	mtpr	$_Syssize,$P0LR
	mtpr	$0,$TBIA		# clean tb
	mtpr	$1,$MAPEN		# enable mapping
	jmp	*$1f			# run in virt space
1:
	casel	_cpu,$1,$CPU_MAX
0:
	.word	3f-0b		# 1 is VAX_780
	.word	3f-0b		# 2 is VAX_750
	.word	3f-0b		# 3 is VAX_730
	.word	3f-0b		# 4 is VAX_8600
	.word	1f-0b		# 5 is VAX_8200
	.word	2f-0b		# 6 is VAX_8800
	.word	3f-0b		# 7 is MVAX_I
	.word	3f-0b		# 8 is MVAX_II
	.word	3f-0b		# 9 is VVAX
	.word	3f-0b		# 10 is VAX_3600 (Mayfair I)
	.word	4f-0b		# 11 is VAX6200 (Calypso CVAX)
	.word	3f-0b		# 12 is VAX_3400 (Mayfair II)
	.word	3f-0b		# 13 is C_VAXSTAR (PVAX)
	.word	6f-0b		# 14 is CVAX (Firefox)
	.word	3f-0b		# 15 is VAX_3900 (Mayfair III)
	.word	3f-0b		# 16 RESERVED for MIPS processor
	.word	5f-0b		# 17 is VAX_8820 (Polarstar)
	.word	3f-0b		# 18 RESERVED for MIPS processor
	.word	3f-0b		# 19 RESERVED for MIPS processor
	.word	3f-0b		# 20 RESERVED for MIPS processor
	.word	3f-0b		# 21 RESERVED for MIPS processor
	.word	7f-0b		# 22 is Calypso Rigel (VAX6400)
	.word	3f-0b		# 23 is VAXSTAR
	.word	3f-0b		# 24 RESERVED for MIPS processor
	.word	3f-0b		# 25 RESERVED for MIPS processor
	.word	9f-0b		# 26 VAX_9000

7:
#ifdef	VAX6400
	movl	_rssc+0x40,r0		# Read RSSC IPORT register
	bicl2	$0xfffffff0,r0		# Get node ID of processor
#endif  VAX6400
	brb	cpuid_found
6:
#ifdef VAX60
	mfpr	$CPUID,r0		# VAX60 - just need something unique
#endif	VAX60
	brb	cpuid_found

4:
#ifdef VAX6200
	movl	_v6200csr,r0
	bicl2	$0xfffffff0,r0
#endif VAX6200
	brb 	cpuid_found

1:
#ifdef	VAX8200
	mfpr	$BINID,r0		# VAX8200
#endif	VAX8200
	brb 	cpuid_found
5:
	mfpr	$SID,r0			# VAX8820 
	ashl	$-22,r0,r0
	bicl2	$0xfffffffc,r0
	brb	cpuid_found
2:
	mfpr	$SID,r0			# VAX8800 
	ashl	$-23,r0,r0
	bicl2	$0xfffffffe,r0
	brb	cpuid_found
9:
	mfpr	$CPUID, r0		# VAX9000
	brb	cpuid_found

3:
	movl	$1,r0			# no choice for uniprocessors
	brb 	cpuid_found

cpuid_found:
	movl	_cpudata[r0],r2
	movl	$1,CPU_NOPROC(r2)	# no user proc at this time
	movl	$CPU_RUN,CPU_STATE(r2)	# we are now running
	movl	CPU_ISTACK(r2),sp	# load stack pointer
	mtpr 	sp,$ISP			#
	pushl	r2			# save off cpudata pointer.

	calls	$0,_startrtclock	# start interval timer
	movl	(sp)+,r3		# need table index in idle loop
	mtpr	r3,$ESP			#
	PUSHR
	casel	_cpu,$1,$CPU_MAX
0:
	.word	1f-0b		# 1 is VAX_780
	.word	1f-0b		# 2 is VAX_750
	.word	1f-0b		# 3 is VAX_730
	.word	1f-0b		# 4 is VAX_8600
	.word	1f-0b		# 5 is VAX_8200
	.word	1f-0b		# 6 is VAX_8600
	.word	1f-0b		# 7 is MVAX_I
	.word	1f-0b		# 8 is MVAX_II
	.word	1f-0b		# 9 is VVAX
	.word	1f-0b		# 10 is VAX_3600 (Mayfair I)
	.word	2f-0b		# 11 is VAX_6200 (Calypso/CVAX)
	.word	1f-0b		# 12 is VAX_3600 (Mayfair II)
	.word	1f-0b		# 13 is CVAXSTAR
	.word	3f-0b		# 14 is VAX_60 (Firefox)
	.word	1f-0b		# 15 is VAX_3600 (Mayfair III)
	.word	1f-0b		# 16 RESERVED for MIPS processor
	.word	1f-0b		# 17 is VAX_8820 (Polarstar)
	.word	1f-0b		# 18 RESERVED for MIPS processor
	.word	1f-0b		# 19 RESERVED for MIPS processor
	.word	1f-0b		# 20 RESERVED for MIPS processor
	.word	1f-0b		# 21 RESERVED for MIPS processor
	.word	4f-0b		# 22 is VAX6400 (Calypso Rigel)
	.word	1f-0b		# 23 is VAXSTAR
	.word	1f-0b		# 24 RESERVED for MIPS processor
	.word	1f-0b		# 25 RESERVED for MIPS processor
	.word	1f-0b		# 26 is VAX_9000

4:
	# Rigel processor only.  The FPA is switched off on reset,
	# turn it on!
	# The following line turns on FPA and without affecting the
	# state of the vector present bit
	# (may need to examine the VINTSR<vector_unit_present> bit to see if 
	# there is a vector unit.  See Rigel spec for details.
	pushr	$0x2			# save r2
	mfpr	$ACCS,r2		# get copy of ACCS
	bisl2	$2,r2			# mask in bit to turn on FPA.
	mtpr	r2,$ACCS		# write back to ACCS
	movl	(sp)+,r2		# restore r2
	calls	$0,_clear_xrperr	# Clear errors to get by 2ndpass bug.
	calls	$0,_ka6400initslave
	brb	1f
	
3:
	calls	$0,_ka60initslave
	brb	1f

2:
	calls	$0,_ka6200initslave

1:	
	POPR

	bbc	CPU_NUM(r2),_vpmask,no_vp_reset

	pushr	$0x2			# save r2
	pushl	CPU_NUM(r2)		# param for vp_reset: cpu number
	calls	$1,_vp_reset
	movl	(sp)+,r2		# restore r2

no_vp_reset:

	pushl	$0x04010000		# psl = mode(k,k) ipl=1 is=1
	pushl	$idle0
	rei				# rei into idle loop (best we can do)

	.data
	.globl	_cpu
	.globl	_cpu_subtype
	.globl	_cpu_sub_subtype
	.globl	_cpu_systype
	.globl	_cpu_archident
	.globl	_vs_cfgtst
	.globl	_mb_slot
/*
 * The usage of the following variables requires explanation since
 * it is not obvious.  The VAX System ID register (IPR 62) has the
 * high order 8 bits <31:24> containing a CPU type, the low order
 * 24 bits are type dependent.  The CPU types are:
 *	0	Reserved to DIGITAL
 *	1	VAX 780 or VAX 785
 *	2	VAX 750
 *	3	VAX 730
 *	4	VAX 8600 or 8650
 *	5	VAX 8200, 8250, 8300, or 8350
 *	6	VAX 8500, 8530, 8550, 8700, or 8800
 *	7	MicroVAX-I chip
 *	8	MicroVAX-II chip
 *	9	VVAX
 *	10	CVAX chip
 *	11	Rigel chip
 *    Others	Reserved to DIGITAL
 *
 * In addition to SID, the newer processors (namely of the microvax 
 * family such as microVAX-II, CVAX, and RIGEL) have a SYS_TYPE register 
 * in physical address 0x20040004.
 * The high order 8 bits <31:24> of SYS_TYPE contain the system
 * type code, the next 8 bits <23:16> contain the rev level, and
 * the low 16 bits <15:0> contain type dependent information. The
 * system type codes are:
 *	1	MicroVAX-II
 *	4	VAXstation 2000
 *	2	Calypso systems (currently CVAX and RIGEL)
 *   Others	Reserved to DIGITAL
 *	
 * To simplify the job of identifying the many variants, the following
 * variables are used:
 *
 *	_cpu	
 *		ULTRIX's internal code for identifying different
 *		systems.  It is related but not identical to the CPU
 *		type field in the SID register.
 *
 *	_cpu_systype
 *		Contains the value of SYS_TYPE register for systems
 *		that has the SYS_TYPE register.
 *
 *	_cpu_subtype
 *		Contains the high 8 bits <31:24> system type field
 *		of the SYS_TYPE register.
 *
 *	_cpu_sub_subtype
 *		Contains bits<15:8> of the SYS_TYPE register.  It
 *		identifies system variants.
 *	_cpu_archident
 *		Contains the low 8 bits <7:0> of the SYS_TYPE register.
 *		
 */

_cpu:			.long	0
_cpu_subtype:		.long	0
_cpu_sub_subtype:	.long	0
_cpu_systype:		.long	0
_cpu_archident:		.long	0
_vs_cfgtst:		.long	0
_mb_slot:		.long	0
	.text
	.globl	start
start:
	.word	0
/*
 * If we are booted there should be an arg passed in.  If present, then
 * save the arg otherwise, assume the old boot path
 */
	tstl	(ap)			# arg present?
	beql	1f			# if not, skip ahead
	movl	4(ap),_vmbinfo		# otherwise, save the address passed
1:
	mtpr	$0,$TXCS		# shut down transmit interrupts
	mtpr	$0,$ICCS
/* set system control block base */
	mtpr	$_scb-0x80000000,$SCBB
/* set ISP and get cpu type (and sub-type) */
	movl	$_eintstack,sp
	mfpr	$SID,r0
	movab	_cpu,r1
	extzv	$24,$8,r0,(r1)		# _cpu is just the high byte of SID
	cmpl	$MVAX_II,_cpu		# is cpu a uVAX II chip?
	bneq	_is_C_VAX		#   no, so see if its a CVAX chip
	brb	1f			#   yes, so go get cpu subtype
_is_C_VAX:
	cmpl	$CVAX_CPU,_cpu		# is cpu a CVAX chip?
	bneq	2f			#   no, see if it is a VAX9000
	brb	1f			#   yes, go get cpu subtype
2:
	cmpl	$ST_9000,_cpu		# is cpu VAX9000?
	bneq	3f			#   no, see if it is a RIGEL chip
	movl	$VAX_9000,_cpu		# set _cpu to VAX_9000
	brw	_done			#

3:
	cmpl	$RIGEL_CPU,_cpu		# is cpu a RIGEL chip?
	bneq	4f			#   no, see if it is a MARIAH chip
	brb	1f			#   yes, go get cpu.subtype
4:
	cmpl	$MARIAH_CPU,_cpu	# is this a Mariah?
 	jneq	_done			#   no, don't have to get SYS_TYPE

/* When we get here, the CPU is uVAXII, CVAX, RIGEL or MARIAH, get SYS_TYPE */
1:
	movl	*$SID_EXT,r0		# Read in SYS_TYPE register.
	movl	r0,_cpu_systype		# save off copy of systype reg.
	movl	r0,_cpu_archident	# Save STS_TYPE ARCH_IDENT bits
	bicl2	$0xffffff00,_cpu_archident
	movab	_cpu_subtype,r1
	extzv	$24,$8,r0,(r1)		# Save upper 8 bits of SYS_TYPE

	/* uniquely identify Mariah if this is a Mariah CPU */	
	cmpl	$MARIAH_CPU,_cpu	# is this a Mariah?
	bneq	2f			#   no, go get sys_sub_type.
	movl	$MARIAH_VARIANT,_cpu_sub_subtype #load with Mariah code.
	movl	$RIGEL_CPU,_cpu		# piggyback Mariah on Rigel code.
	brb	3f

	/* get sys_sub_type from SID_EXT */
2:	extzv	$8,$8,_cpu_systype,_cpu_sub_subtype 

3:	cmpl	$CVAX_CPU,_cpu		# if CVAX {
	jneq	_is_RIGEL_CPU			#
	cmpl	$ST_CVAXQ,_cpu_subtype	#    if QBUS Mayfair {
	bneq	_is_VAX_60		#
	cmpl	$SB_KA650,_cpu_sub_subtype #   if Mayfair I {
	bneq	_is_VAX_3600		#
	movl	$VAX_3600,_cpu		#       set _cpu to VAX_3600
	brw	_done			#      }
_is_VAX_3600:
	cmpl	$SB_KA640,_cpu_sub_subtype #   else if Mayfair II {
	bneq	_is_VAX_3900		#
	movl	$VAX_3400,_cpu		#	set _cpu to VAX_3400
	brw	_done			#      }
_is_VAX_3900:
	movl	$VAX_3900,_cpu		#      else set _cpu to VAX_3900
	brw	_done			#    }
_is_VAX_60:
	cmpl	$ST_KA60,_cpu_subtype	#    else if Firefox {
	bneq	_is_CVAXSTAR		#
	movl	$VAX_60,_cpu		#	set _cpu to VAX_60
	movl	*$0x2014054a,_mb_slot	#	slot num of IO module from NVR
	brw	_done			#    }
_is_CVAXSTAR:
	cmpl	$ST_VAXSTAR,_cpu_subtype#    else if CVAXSTAR {
	bneq	_is_VAX_6200		#
	movl	$C_VAXSTAR,_cpu		#	set _cpu to C_VAXSTAR
	clrb	*$VS_IORESET		#	init CVAXstar I/O controllers
	movzwl	*$VS_CFGTST,_vs_cfgtst	#	save CVAXstar config/test reg
	brw	_done			#    }
_is_VAX_6200:					#    else {
	movl	$VAX_6200,_cpu		#	must be Calypso CVAX, set _cpu
	brw	_done			#    }
_is_RIGEL_CPU:					# }
	cmpl	$RIGEL_CPU,_cpu		# else if RIGEL {
	bneq	_is_MVAX_II		#
	movl	$VAX_6400,_cpu		#    assume it is Calypso Rigel.
	brw	_done			# }
_is_MVAX_II:
	cmpl	$MVAX_II,_cpu		# else if MVAX II {
	bneq	_done			#
	cmpl	$ST_VAXSTAR,_cpu_subtype#    if VAXSTAR {
	bneq	_done			#
	movl	$VAXSTAR,_cpu		#	set _cpu to VAXSTAR
	clrb	*$VS_IORESET		#	init VAXstar I/O controllers
	movzbl	*$VS_CFGTST,_vs_cfgtst	#	save VAXstar config/test reg
	brw	_done			#    }
					# }

_done:	
/* All done with testing for various system types.  Init RPB */
	movl	_vmbinfo,r7		# point to the info list
	beql	1f			# is there one?
	movc3	INFO_RPBSIZ(r7),*INFO_RPBBAS(r7),_rpb
					# plug in VMB's RPB
	movl	INFO_MEMSIZ(r7),r7	# get the size of memory
1:
	movab	_rpb,r0
	movl	r0,(r0)+			# rp_selfref
	movab	_doadump,r1
	movl	r1,(r0)+			# rp_dumprout
	movl	$0x1f,r2
	clrl	r3
1:	addl2	(r1)+,r3; sobgtr r2,1b
	movl	r3,(r0)+			# rp_chksum
	clrl	(r0)				# make sure rp_flag is 0

/* slave starts executing at rpb+0x100.  Why?  Because VMS does it that way.
   This code fills in a jump instruction and somewhere to go */
	bicl3	$0x80000000,$_slavestart,_rpb+RP_BUGCHK	# slave start routine
	movl	$0xf9bf17,_rpb+RP_WAIT		# indirect jump of .-4

	tstl	r7				# was memsiz passed via VMB?
	bneq	9f				# if so, skip old mechanism
/*
 * THIS SECTION IS SKIPPED WHEN USING VMB TO BOOT ULTRIX - It is left
 * for backwards compatibility with older boot paths.
 * It counts up memory.
 */
/*
 * On microvaxen we've saved the memory size at address 0xf0000;
 * calculated by V1.x boot code.  This size is computed via the pfn bitmap.
 */
#define MEMSIZEADDR	0xf0000
	casel	_cpu,$1,$CPU_MAX
0:
	.word	1f-0b		# 1 is 780
	.word	1f-0b		# 2 is 750
	.word	1f-0b		# 3 is 730
	.word	1f-0b		# 4 is 8600
	.word	1f-0b		# 5 is 8200
	.word	1f-0b		# 6 is 8800
	.word	7f-0b		# 7 is MicroVAX 1
	.word	7f-0b		# 8 is MicroVAX Chip
	.word	7f-0b		# 9 is VVAX
	.word	7f-0b		# 10 is VAX_3600 (Mayfair I)
	.word	7f-0b		# 11 is VAX_6200 (CVAX/Calypso)
	.word	7f-0b		# 12 is VAX_3400 (Mayfair II)
	.word	7f-0b		# 13 is C_VAXSTAR (VAX3100)(PVAX)
	.word	7f-0b		# 14 is VAX_60 (Firefox)
	.word	7f-0b		# 15 is VAX_3900 (Mayfair III)
	.word	1f-0b		# 16 is DS_3100 (DECstation 3100)(PMAX)
	.word	1f-0b		# 17 is VAX_8820 (Polarstar)
	.word	1f-0b		# 18 is DS_5400 (MIPSfair)
	.word	1f-0b		# 19 is DS_5800 (ISIS)
	.word	1f-0b		# 20 is DS_3MAX (need correct product name)
	.word	1f-0b		# 21 is DS_CMAX (need correct product name)
	.word	7f-0b		# 22 is VAX6400 (Calypso Rigel)
	.word	7f-0b		# 23 is VAXSTAR
	.word	1f-0b		# 24 RESERVED for MIPS processor
	.word	1f-0b		# 25 RESERVED for MIPS processor
	.word	1f-0b		# 26 is VAX9000
5:
	movl	$0x00400000,r7		# get the size of memory
	jbr	9f
/* MVAX memsize was determined in the boot code */
7:
	movl	*$MEMSIZEADDR,r7		# get the size of memory
	brb	9f
/* count up memory by testing addresses at every 64K boundary until if fails */
1:	pushl	$4; pushl r7; calls $2,_bbadaddr; tstl r0; bneq 9f
	acbl	$MAX_MEM-1,$64*1024,r7,1b
9:
/* clear physical memory starting at _edata, ending at (vmbinfo) */
	movab	_edata,r3		# start clearing here
	movl	_vmbinfo,r6		# stop here
2:	movzwl	$0xfffc,r0		# clear longwords
3:	movc5	$0,(r3),$0,r0,(r3)
	subl3	r3,r6,r0		# how much left to clear?
	beqlu	4f			# nothing
	cmpl	r0,$0xffff
	bgtru	2b			# more than than movc5 max
	brb	3b			# less than or equal to movc5 max
4:
/* trap() and syscall() save r0-r11 in the entry mask (per ../h/reg.h) */
/* panic() save r0-r11 in the entry mask also, for panic errlog. */
	bisw2	$0x0fff,_trap
	bisw2	$0x0fff,_syscall
	bisw2	$0x0fff,_panic
/* initialize system page table: scb and int stack writeable.  Set
 * modify bit in all writeable pages (kernel data & stack) so we
 * don't take unnecessary modify faults on those architectures that
 * support them.
 */
	clrl	r2
	movab	_doadump,r1; ashl $-PGSHIFT,r1,r1
1:	bisl3	$PG_M|PG_V|PG_KW,r2,_Sysmap[r2]; aoblss r1,r2,1b
/* make rpb read-only as red zone for interrupt stack */
	bicl2	$PG_PROT,_rpbmap
	bisl2	$PG_KR,_rpbmap
/* make kernel text space read-only */
	movab	_etext+NBPG-1,r1; ashl $-PGSHIFT,r1,r1
1:	bisl3	$PG_V|PG_KR,r2,_Sysmap[r2]; aoblss r1,r2,1b
/* make kernel data, bss, read-write */
	movab	_end+NBPG-1,r1; ashl $-PGSHIFT,r1,r1
1:	bisl3	$PG_M|PG_V|PG_KW,r2,_Sysmap[r2]; aoblss r1,r2,1b

/*
 * Allow the user access to the kernel space containing
 * the emulation code.
 */
	extzv	$PGSHIFT,$VPN_WIDTH,_vaxemulbegin,r2	# first page number
	beql	2f			# br if no?
	addl3	_vaxemulend,$0x80000000+NBPG-1,r1; ashl $-PGSHIFT,r1,r1
1:	bisl3	$PG_V|PG_URKR,r2,_Sysmap[r2]; aoblss r1,r2,1b
2:
/*
 * If cpu type is uVAX I or uVAX II or VAX3600 (Mayfair) or
 * VAX420 (CVAXstar) or VAX6200 (Calypso) or VAX60 (Firefox),
 * test the processor to see if it has dfloat support. If not it could
 * either be a gfloat uVAX-I or a uVAX-II without the fpu.
 * VAXstar and CVAXstar and Firefox always have the fpu.
 *
 * If the cpu is a Rigel (which has a FPA that can be turned
 * on or off), try to turn on the FPA and see if a reserved instruction
 * fault occurs.  If it does, the FPA is not in place.
 */
	cmpl	$MVAX_II,_cpu		# is cpu a uVAX II?
	beql	2f			#   yes, do float test
	cmpl	$VAXSTAR,_cpu		# is cpu a VAXSTAR?
	beql	2f			#   yes, do float test
 	cmpl	$VAX_60,_cpu		# is this firefox Firefox?
	beql	2f			#   yes, do float test.
 	cmpl	$VAX_6200,_cpu		# is this Calypso CVAX?
	beql	2f			#   yes, do float test.
 	cmpl	$VAX_6400,_cpu		# is this Rigel?
	beql	3f			#   yes, do float test.
	cmpl	$VAX_3600,_cpu		# is cpu a Mayfair I?
	beql	2f			#   yes, so go do the float test
	cmpl	$VAX_3400,_cpu		# is cpu a Mayfair II?
	beql	2f			#   yes, so go do the float test
	cmpl	$VAX_3900,_cpu		# is cpu a Mayfair III?
	beql	2f			#   yes, so go do the float test
	cmpl	$C_VAXSTAR,_cpu		# is cpu a CVAXstar?
	beql	2f			#   yes, so go do the float test
	cmpl	$MVAX_I,_cpu		# is cpu a uVAX I?
	beql	2f			#   yes, do float test
	cmpl	$VAX_9000,_cpu		# is cpu a VAX9000
	beql	2f			#   yes, do float test
	brb	5f			# no, skip the float test

3:	# Rigel processor only.  The FPA is switched off on reset,
	# turn it on to do the float test!
	# The following line turns on FPA and does not affect the state of
	# the vector present bit.
	# Bit<0> of ACCS turns on vector unit, Bit<1> turns on FPA.
	# May need to look at the VINTSR<vector_unit_present> bit to
	# determine if there is a vector processor present.  See Rigel spec 
	# for details.
	# (use r1 without saving it because existing code uses it a few lines
	# from here, so it must free
	mfpr	$ACCS,r1		# get copy of ACCS
	bisl2	$2,r1			# mask in bit to turn on FPA.
	mtpr	r1,$ACCS		# write back to ACCS
2:
	mfpr	$SCBB,r1
	movl	0x10(r1),r2		# Save the vector
	movab	0f,0x10(r1)		# Plug in our vector
	tstd	r0
/*
 * If we reach here then no trap occured.
 */
	incl	_fl_ok			# If no exception then set okay
	brb	1f
	.align	2			# must align to word !!!!
0:
	movab	1f,(sp) 		# Return to vector restoration
	rei
1:
	movl	r2,0x10(r1)		# Put the vector back
/*
 * Setup the scb vectors to point to the emulation code.
 */
4:
	mfpr	$SCBB,r1
	movl	$vax$emulate,0xc8(r1)
	movl	$vax$emulate_fpd,0xcc(r1)
5:
/* Set up the system map. */
	mtpr	$_Sysmap-0x80000000,$SBR
	mtpr	$_Syssize,$SLR
/* Double map the kernel into user virtual addresses identical to physical. */
	mtpr	$_Sysmap,$P0BR
	movab	0f,r6		# physical address of jmp to S0 below
	ashl	$-PGSHIFT,r6,r6 # index of its PTE in Sysmap
	incl	r6		# number of pages we need in P0 map
	mtpr	r6,$P0LR	# For SVS make the page table just big enough.

/* now go to mapped mode */
	mtpr	$0,$TBIA; mtpr $1,$MAPEN; jmp *$0f; 0:
/* init mem sizes */
	ashl	$-PGSHIFT,r7,r6
	movl	r6,_maxmem
	cmpl	r6,$MAXMEM_PGS		# max memory 511.5 Mb
	bleq	0f			# ok
	movl	$MAXMEM_PGS,r6		# set the maximum
0:	movl	r6,_physmem
	movl	r6,_freemem
/* setup context for proc[0] == Scheduler */
	movab	_end+NBPG-1,r6
	bicl2	$NBPG-1,r6		# make page boundary
/* setup page table for proc[0] */
	bbcc	$31,r6,0f; 0:
	ashl	$-PGSHIFT,r6,r3 		# r3 = btoc(r6)
	bisl3	$PG_V|PG_KW,r3,_Usrptmap	# init first upt entry
	movab	_usrpt,r0
	mtpr	r0,$TBIS
/* init p0br, p0lr */
	mtpr	r0,$P0BR
	mtpr	$0,$P0LR
/* init p1br, p1lr */
	movab	NBPG(r0),r0
	movl	$0x200000-HIGHPAGES,r1
	mtpr	r1,$P1LR
	mnegl	r1,r1
	moval	-4*HIGHPAGES(r0)[r1],r2
	mtpr	r2,$P1BR
/* setup mapping for UPAGES of _u */
	movl	$UPAGES,r2; movab _u+NBPG*UPAGES,r1; addl2 $UPAGES+1,r3; jbr 2f
1:	decl	r3
	moval	-NBPG(r1),r1;
	bisl3	$PG_M|PG_V|PG_URKW,r3,-(r0)
	mtpr	r1,$TBIS
2:	sobgeq	r2,1b
/* initialize (slightly) the pcb */
	movab	UPAGES*NBPG(r1),PCB_KSP(r1)
	mnegl	$1,PCB_ESP(r1)
	mnegl	$1,PCB_SSP(r1)
	movab	-FORKPAGES*NBPG(r1),PCB_USP(r1)
	mfpr	$P0BR,PCB_P0BR(r1)
	mfpr	$P0LR,PCB_P0LR(r1)
	movb	$4,PCB_P0LR+3(r1)		# disable ast
	mfpr	$P1BR,PCB_P1BR(r1)
	mfpr	$P1LR,PCB_P1LR(r1)
	movl	$CLSIZE,PCB_SZPT(r1)		# init u.u_pcb.pcb_szpt
	movl	r11,PCB_R11(r1)
	movl	r10,PCB_R10(r1)
	movab	1f,PCB_PC(r1)			# initial pc
	movzbl	$NO_PRVCPU,PCB_PRVCPU(r1)	# PCB_ASN = 0 for now
	movl	$0x1f0000,PCB_PSL(r1)		# mode(k,k), ipl=1f
	ashl	$PGSHIFT,r3,r3
	cmpl	$VAX_9000,_cpu
	beql 	start_vax9000
	mtpr	r3,$PCBB			# first pcbb
	brb 	start_cont

start_vax9000:
	mtpr	r1,$PCBB

start_cont:
/* set regs, p0br, p0lr, p1br, p1lr, astlvl, ksp and change to kernel mode */
	ldpctx
	rei
/* put signal trampoline code in u. area */
1:	movab	_u,r0
	movc3	$16,sigcode,PCB_SIGC(r0)
/* save reboot flags in global _boothowto */
	movl	r11,_boothowto
	movl	r10,_bootdevice
#ifdef defined(BINARY)
/*
 * compute login limit and save with boot flag
 * in high order word.
 * LOGLIMxxx are defined in cpu.h.
 * TODO:  add MicroVAX, and Superstar as well as other VAXen.
 */
	pushl	r11		# now, use r11 as place to do work

	casel	_cpu,$1,$CPU_MAX
0:
	.word	1f-0b		# 1 is VAX_780
	.word	2f-0b		# 2 is VAX_750
	.word	3f-0b		# 3 is VAX_730
	.word	4f-0b		# 4 is VAX_8600 (treated as 780)
	.word	9f-0b		# 5 is VAX_8200
	.word	7f-0b		# 6 is VAX_8800
	.word	5f-0b		# 7 is MVAX_I
	.word	5f-0b		# 8 is MVAX_II
	.word	3f-0b		# 9 is VVAX
	.word	5f-0b		# 10 is VAX_3600 (Mayfair I)
	.word	3f-0b		# 11 is VAX_6200 (Calypso CVAX)
	.word	5f-0b		# 12 is VAX_3400 (Mayfair II)
	.word	5f-0b		# 13 is C_VAXSTAR (VAX3100)(PVAX)
	.word	5f-0b		# 14 is VAX_60 (Firefox)
	.word	5f-0b		# 15 is VAX_3900 (Mayfair III)
	.word	1f-0b		# 16 is RESERVED for MIPS processor
	.word	7f-0b		# 17 is VAX_8820 (Polarstar)
	.word	1f-0b		# 18 is RESERVED for MIPS processor
	.word	1f-0b		# 19 is RESERVED for MIPS processor
	.word	1f-0b		# 20 is RESERVED for MIPS processor
	.word	1f-0b		# 21 is RESERVED for MIPS processor
	.word	5f-0b		# 22 is VAX6400 (Calypso Rigel)
	.word	5f-0b		# 23 is VAXSTAR
	.word	1f-0b		# 24 is RESERVED for MIPS processor
	.word	1f-0b		# 25 is RESERVED for MIPS processor
	.word	10f-0b		# 26 is VAX_9000

	# fall here if cputype doesn't jive.
	brb 3f					# CPU not known. Should we do anything?
1:	# 780
	ashl	$16, $LOGLIM780, r11		# move login limit to high word
	bisl2	(sp)+, r11			# OR bootflags back in
	brb 6f
2:	# 750
	ashl	$16, $LOGLIM750, r11		# move login limit to high word
	bisl2	(sp)+, r11			# OR bootflags back in
	brb 6f
3:	# 730 and unknown's
	ashl	$16, $LOGLIM730, r11		# move login limit to high word
	bisl2	(sp)+, r11			# OR bootflags back in
	brb 6f
4:	# 8600
	ashl	$16, $LOGLIM8600, r11		# move login limit to high word
	bisl2	(sp)+, r11			# OR bootflags back in
	brb 6f
9:	# 8200
	ashl	$16, $LOGLIM8200, r11		# move login limit to high word
	bisl2	(sp)+, r11			# OR bootflags back in
	brb 6f
5:
	ashl	$16, $LOGLIMMVAX, r11		# move login limit to high word
	bisl2	(sp)+, r11			# OR bootflags back in
	brb 6f
7: #8800
	ashl	$16, $LOGLIM8800, r11		# move login limit to high word
	bisl2	(sp)+, r11			# OR bootflags back in
	brb 6f
10: #9000
	ashl	$16, $LOGLIM9000, r11		# move login limit to high word
	bisl2	(sp)+, r11			# OR bootflags back in
	brb 6f
6:
#endif BINARY
/* calculate firstaddr, and call main() */
	movab	_end+NBPG-1,r0; bbcc $31,r0,0f; 0:; ashl $-PGSHIFT,r0,-(sp)
	addl2	$UPAGES+1,(sp); calls $1,_main
/* proc[1] == /etc/init now running here; run icode */
	pushl	$PSL_CURMOD|PSL_PRVMOD; pushl $0; rei

/* signal trampoline code: it is known that this code takes exactly 16 bytes */
/* in ../vax/pcb.h and in the movc3 above */
sigcode:
	calls	$4,5(pc)			# params pushed by sendsig
	chmk	$139				# cleanup mask and onsigstack
	rei
	.word	0x7f				# registers 0-6 (6==sp/compat)
	callg	(ap),*16(ap)
	ret

/*
 * Primitives
 */

	.data
	.globl	_ignorebi
_ignorebi: .long 1
	.text
	.globl	_bisst
_bisst:
	.word	0
	movl 	_ignorebi,r1
	movl	$1,_ignorebi
	movl	4(ap),r3
	bisl2	$BICTRL_NOARB,(r3)
	movl	$100,r0
	bisl2	$BICTRL_STS+BICTRL_SST,(r3)
1:
	sobgtr	r0,1b

2:
	movl	$100,r0

1:
	
	pushr	$0x1f
	pushl	$100000
	calls 	$1,_microdelay
	popr	$0x1f

	bitl	$BICTRL_BROKE,(r3)
	beql	7f

	decl	r0
	bneq	1b
	
	incl 	r0		
	movl	r1,_ignorebi
	ret

7:
	clrl	r0
	movl	r1,_ignorebi
	ret


/*
 * bbadaddr(addr, len)
 *	see if access addr with a len type instruction causes a machine check
 *	len is length of access (1=byte, 2=short, 4=long)
 */
	.globl	_bbadaddr
_bbadaddr:
	.word	0
	# Workaround for 2nd pass XRP (VAX6400) bug.  We have to clear
	# the processor's status register and the XMI bus register's
	# error bits before probing.
	cmpl	_cpu,$VAX_6400		#if Rigel then
	bneq	3f			#
	calls	$0,_clear_xrperr	#	clear error bits.
	brb	3f			#
3:	movl	$1,r0
	mfpr	$IPL,r1
	mtpr	$HIGH,$IPL
	movl	_scb+MCKVEC,r2
	movl	4(ap),r3
	movl	8(ap),r4
	movab	9f+INTSTK,_scb+MCKVEC
	bbc	$0,r4,1f; tstb	(r3)
1:	bbc	$1,r4,1f; tstw	(r3)
1:	bbc	$2,r4,1f; tstl	(r3)

	movl	$100,r4
1:
	sobgtr r4,1b

	clrl	r0			# made it w/o machine checks
2:	movl	r2,_scb+MCKVEC
	mtpr	r1,$IPL
	ret
	.align	2
9:
	casel	_cpu,$1,$CPU_MAX
0:
	.word	8f-0b		# 1 is VAX_780
	.word	5f-0b		# 2 is VAX_750
	.word	5f-0b		# 3 is VAX_730
	.word	4f-0b		# 4 is VAX_8600
	.word	5f-0b		# 5 is VAX_8200
	.word	6f-0b		# 6 is VAX_8800
	.word	5f-0b		# 7 is MVAX_I
	.word	5f-0b		# 8 is MVAX_II
	.word	1f-0b		# 9 is VVAX
	.word	1f-0b		# 10 is VAX_3600 (Mayfair I)
	.word	1f-0b		# 11 is VAX_6200 (Calypso/CVAX)
	.word	1f-0b		# 12 is VAX_3400 (Mayfair II)
	.word	1f-0b		# 13 is C_VAXSTAR (PVAX)
	.word	7f-0b		# 12 is VAX_60 (Firefox)
	.word	1f-0b		# 15 is VAX_3900 (Mayfair III)
	.word	1f-0b		# 16 RESERVED for MIPS processor
	.word	6f-0b		# 17 is VAX_8820 (Polarstar)
	.word	1f-0b		# 18 RESERVED for MIPS processor
	.word	1f-0b		# 19 RESERVED for MIPS processor
	.word	1f-0b		# 20 RESERVED for MIPS processor
	.word	1f-0b		# 21 RESERVED for MIPS processor
	.word	9f-0b		# 22 is VAX6400 (Calypso Rigel)
	.word	5f-0b		# 23 is VAXSTAR
	.word	1f-0b		# 24 RESERVED for MIPS processor
	.word	1f-0b		# 25 RESERVED for MIPS processor
	.word	1f-0b		# 26 is VAX_9000
7:
	calls	$0,_enafbiclog	# go enable fbic error logging
	brb	1f
6:
	calls	$0,_nmifaultclear
5:
#if defined(VAX750) || defined(VAX730) || defined (MVAX) || defined(VAX8200)
	mtpr	$0xf,$MCESR
#endif
	brb	1f
8:
#if VAX780
	mtpr	$0xc0000,$SBIFS /* set sbi interrupts and clear fault latch */
	brb	1f
#endif VAX780
4:
#if defined(VAX8600)
	mtpr	$0,$EHSR
	brb	1f
#endif VAX8600
9:	
	mtpr	$0,$MCESR		# Ack Rigel mcheck before discarding
	calls	$0,_clear_xrperr	# Clear Rigel error registers.
	brb	1f
1:
	addl2	(sp)+,sp		# discard mchchk trash
	movab	2b,(sp)
	rei

/*
 * save off context of processes and stop cpu....
 */
 	.globl _stop_secondary_cpu

_stop_secondary_cpu:
	.word	0x0fff
	mfpr 	$ESP,r0
	tstb	CPU_NOPROC(r0)
	bneq	stop_noproc
	pushl	CPU_NUM(r0)	# this literal will end up
	pushal	stop_noproc	# in the PCB of the dead 
				# process as the PC and PSL.
	svpctx			# switch to interrupt stack 

stop_noproc:
	tstl	CPU_STACK(r0)
	bneq	stop_stack_nonzero

	movl 	fp,CPU_STACK(r0) #save off frame pointer.

stop_stack_nonzero:
	bicl2	$CPU_RUN,CPU_STATE(r0)
        bisl2   $CPU_STOP,CPU_STATE(r0)
	adawi	$0,_cpu		#flush out write buffers....


	cmpl	$VAX_8200,_cpu	# halt 8200 because of console reboot
	bneq	stop_donot_halt	# problems.
	halt			
stop_donot_halt:
	nop
	nop
	brb	stop_donot_halt	#stay here until rebooted.


/*
 * non-local goto's
 */
#ifdef notdef		/* this is now expanded completely inline */
	.globl	_Setjmp
_Setjmp:
	movl	fp,(r0)+	# current stack frame
	movl	(sp),(r0)	# resuming pc
	clrl	r0
	rsb
#endif

#define PCLOC 16	/* location of pc in calls frame */
#define APLOC 8 	/* location of ap,fp in calls frame */

	.globl	_Longjmp
_Longjmp:
	mfpr	$ESP,r1
	movl	(r0)+,CPU_NEWFP(r1) # must save parameters in memory as all
	movl	(r0),CPU_NEWPC(r1)	# registers may be clobbered.
1:
	mfpr	$ESP,r0
	cmpl	fp,CPU_NEWFP(r0) # are we there yet?
	bgequ	2f		# yes
	moval	1b,PCLOC(fp)	# redirect return pc to us!
	ret			# pop next frame
2:
	beql	3f		# did we miss our frame?
	pushab	4f		# yep ?!?
	calls	$1,_panic
3:
	movl	CPU_NEWPC(r0),r0	# all done, just return to the `setjmp'
	jmp	(r0)		# ``rsb''

	.data
4:	.asciz	"longjmp"
	.text
/*
 * setjmp that saves all registers as the call frame may not
 * be available to recover them in the usual mannor by longjmp.
 * Called before swapping out the u. area, restored by resume()
 * below.
 */

	.globl	_savectx
	.align	1
_savectx:
	.word	0
	movl	4(ap),r0
	movq	r6,(r0)+
	movq	r8,(r0)+
	movq	r10,(r0)+
	movq	APLOC(fp),(r0)+ # save ap, fp
	addl3	$8,ap,(r0)+	# save sp
	movl	PCLOC(fp),(r0)	# save pc
	clrl	r0
	ret

	.data
	.globl	_whichqs
_whichqs:	.space	8
	.text
	.globl	_lk_rq
	.globl	_qs
	.globl	_cnt
/*
 * The following primitives use the fancy VAX instructions
 * much like VMS does.	_whichqs tells which of the 32 queues _qs
 * have processes in them.  Setrq puts processes into queues, Remrq
 * removes them from queues.  The running process is on no queue,
 * other processes are on a queue related to p->p_pri, divided by 4
 * actually to shrink the 0-127 range of priorities into the 32 available
 * queues.
 */

	.data
	.globl	_work_to_do
_work_to_do: .long 1
	.text
/*
 * Setrq(p), using fancy VAX instructions.
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
	.globl	_Setrq		# <<<massaged to jsb by "asm.sed">>>
_Setrq:
#ifdef SMP_DEBUG
	tstl	_smp_debug
	beql	setfast
	pushr	$0x3f
	pushab	set2
	pushal	_lk_rq
	calls	$2,_lsert
	popr	$0x3f
setfast:
#endif SMP_DEBUG
	tstl	P_RLINK(r0)		## firewall: p->p_rlink must be 0
	bneq	set1			##
	movzbl	P_PRI(r0),r1		# put on queue which is p->p_pri / 4
	ashl	$-2,r1,r1
	movaq	_qs[r1],r2
	insque	(r0),*4(r2)		# at end of queue
	bbss	r1,_whichqs,set0	# mark queue non-empty
set0:
	movl	$-1,_work_to_do		# set all bits: cleared in switch
	rsb
set1:
	pushab	set2			##
	calls	$1,_panic		##

set2:	.asciz	"setrq"

/*
 * Remrq(p), using fancy VAX instructions
 *
 * Call should be made at spl6().
 */
	.globl	_Remrq		# <<<massaged to jsb by "asm.sed">>>
_Remrq:
#ifdef SMP_DEBUG
	tstl	_smp_debug
	beql	remfast
	pushr	$0x3f
	pushab	rem2
	pushal	_lk_rq
	calls	$2,_lsert
	popr	$0x3f
remfast:
#endif SMP_DEBUG
	movzbl	P_PRI(r0),r1
	ashl	$-2,r1,r1
	# bbcc	r1,_whichqs,rem1
	remque	(r0),r2
	bneq	rem0
	bbcc	r1,_whichqs,rem0
rem0:
	bvs	noentry
	clrl	P_RLINK(r0)		## for firewall checking
	rsb

rem1:
	pushab	rem2			# it wasn't recorded to be on its q
	calls	$1,_panic
noentry:
	pushab	rem3
	calls	$1,_panic

rem2:	.asciz	"remrq"
rem3:	.asciz  "remrq: no entry"

sw0:	.asciz	"swtch"
hold1:	.asciz	"hold lock after swtch"


	.globl	_Sig_parent_swtch
_Sig_parent_swtch: 			# <<<massaged to jsb by "asm.sed">>>
#ifdef SMP_DEBUG
	tstl	_smp_debug
	beql	sig_fast

	pushr	$0x3f
	pushab	sw0
	pushal	_lk_rq
	calls	$2,_lsert
	calls 	$0,_swtch_check
	popr	$0x3f
sig_fast:
#endif SMP_DEBUG
	mfpr	$ESP,r3
	movl	$1,CPU_NOPROC(r3)	# no longer running anything
	mtpr	$0x18,$IPL		# must be non-zero when on intr stack
	svpctx

	pushl	CPU_PROC(r3)
	calls	$1,_sig_parent

	mfpr	$ESP,r3
	brw	idle0


/* this routine switches to the interupt stack and then
   releases the uarea for the proc (u.u_procp).
   Then we drop into the idle loop */

	.globl	_release_uarea_noreturn

_release_uarea_noreturn:
	.word	0
	pushl	$0xdead1	# this literal will end up
	pushal	1f		# in the PCB of the dead 
				# process as the PC and PSL.
	svpctx			# switch to interrupt stack 
1:
	movl	_u+U_PROCP,r7	# process pointer (u.u_procp)
	pushl	$0		# push flag
	pushl	r7		# push proc pointer
	calls	$2,_vrelu	# release u_area for proc
	pushl	r7		# push proc pointer
	calls	$1,_vrelpt	# release pte's u_area for proc
	pushl	r7		# push proc pointer
	calls	$1,_proc_exit	# coordinate with parent for wait
	mfpr	$ESP,r3
	brw	idle0
/*
 * When no processes are on the runq, Swtch branches to idle
 * to wait for something to come ready.
 */
idle2:
	mtpr	$0x1,$IPL	# reenable in case in lock wait
	mtpr	$0x18,$IPL	# no clock while locked

	pushl	r3
	moval	_lk_rq,r0
	jsb	_Smp_lock_once
	movl	(sp)+,r3

	tstl	r0		# did we get the lock?
	beql	idle0		# no - wait a moment.
	brw	sw1		# yes, process queue

clearbit:
	cmpl	$32,r0		# if first time through, no work at all
	bneq	clearself	# otherwise, just clear self
	clrl	_work_to_do	# clear everybody
clearself:
	bicl2	CPU_MASK(r3),_work_to_do# Keep from looping on rq
				# fall through
	.globl	Idle
Idle: idle:
	PUSHR;
	moval	_lk_rq,r0
	jsb	_Smp_unlock_short		
	POPR;

	.globl	idle0
idle0:
	mtpr	$1,$IPL 		# must allow interrupts here

idlenoint:
	mtpr	$0x18,$IPL		# no clock while locked
	tstl	CPU_STOPS(r3)		# global stop cpu check
	beql 	idlecont
	

	bitl	$CPU_STOP,CPU_STATE(r3)
	bneq  	idlestopped
	PUSHR;

	
	bitl	$IPIMSK_STOP,CPU_STOPS(r3)
	beql	idlenoaffinity

	calls	$0,_timeout_affinity_fix

idlenoaffinity:	
	moval	_lk_rq,r0
	jsb	_Smp_lock_retry

	mfpr 	$ESP,r3
	bicl2	$CPU_RUN,CPU_STATE(r3)	# clear run flag
	bisl2	$CPU_STOP,CPU_STATE(r3) # set stopped flag

	moval	_lk_rq,r0
	jsb	_Smp_unlock_short		


	POPR;

idlestopped:
	tstl	CPU_INT_REQ(r3)
	beql	idlenoint

	PUSHR
	calls	$0,_cpu_ip_intr
	POPR
	brb	idlenoint
idlecont:
	bitl	CPU_MASK(r3),_work_to_do# anything added to runq?
	beql	idle0			# if so, fall through
		# we don't panic and do drastic things here if
		# work_to_do is set and whichqs is not, since we
		# are checking work_to_do without a lock, and so
		# anything strage could happen:  just look again.
	tstl	_whichqs		# see if anything in run queues
	beql	idle0			# something to do
	cmpl    $V_VAX,_cpu		# if not a Virtual VAX
	bneq    1f			#   nothing in idle loop
					# else if a Virtual VAX wait
	.word   0x02FD			#   wait
1:
	brw	idle2			# nothing to do, wait for change

/*
 * Swtch(), using fancy VAX instructions
 */
	.globl	_Swtch
_Swtch: 			# <<<massaged to jsb by "asm.sed">>>
#ifdef SMP_DEBUG
	tstl	_smp_debug
	beql	swfast

	pushr	$0x3f
	pushab	sw0
	pushal	_lk_rq
	calls	$2,_lsert
	calls 	$0,_swtch_check
	popr	$0x3f
swfast:
#endif SMP_DEBUG
	mfpr	$ESP,r3			# put cpudata struct pointer into r3

	# if this processor is vector capable then it's vpdata pointer will
	# be non-zero and we must test to see if this is a vector process.
	PUSHR				# standard reg save before calls
	movl	CPU_VPDATA(r3),r0	# get ptr to cpu_vpdata
	tstl	r0			# if ptr is 0, then this processor is
					# not vector capable
	beql	not_vector

	# test the vpd_proc pointer in the vpdata structure to determine 
	# if this is a vector process.
	tstl	VPD_PROC(r0)
	beql	not_vector

	# this is a vector process, so the vector context should be put
	# into a state of limbo.  call vp_contextlimbo().  the 
	# arguement to vp_contextlimbo() is the cpudata struct pointer.  
	pushr	$0x8			# cpudata struct ptr is in r3
	calls	$1,_vp_contextlimbo
not_vector:
	POPR				# standard reg restore after calls

	movl	$1,CPU_NOPROC(r3)	# no longer running anything
	mtpr	$0x18,$IPL		# must be non-zero when on intr stack
	svpctx
sw1:
	tstl	CPU_STOPS(r3)		# this check MUST BE IN SWTCH
	jneq 	idle
	movl	$0,r0			# prime for ffs instruction
nextbit:
	ffs	r0,$32,_whichqs,r0	# find next queue with work
	jeql	clearbit		# none: continue to idle
	movaq	_qs[r0],r1		# get queue header
	movl	(r1),r2			# get addr of first queue element
checkp:					# see if this proc can run here
	bitl	P_AFFINITY(r2),CPU_MASK(r3)	# all cpus check affinity
	bneq	gotp			# yes, our search is over
	movl	(r2),r2			# try next queue element
	cmpl	r1,r2			# if at end of queue, try next
	bneq	checkp
	incl	r0			# try next queue
	brb	nextbit			# no, we keep going

gotp:
	remque	(r2),r2			# Dequeue the lucky winner
	bvc	sw5			# if successfull, back to common code
badsw:
	PUSHR;
	moval	_lk_rq,r0
	jsb	_Smp_unlock_short		
	POPR;
	
	pushab	sw0
	calls	$1,_panic
	/*NOTREACHED*/

sw5:	bneq	sw6
	bbcc	r0,_whichqs,sw6		# still more procs in this queue
sw6:
	clrl	CPU_NOPROC(r3)		
	movl	$10,CPU_ROUNDROBIN(r3)	
	clrl	CPU_RUNRUN(r3)		#
	incl	CPU_SWITCH(r3)		# monitor switches
	movl	r2,CPU_PROC(r3)
	tstl	P_WCHAN(r2)		## firewalls
	bneq	badsw			##
	cmpb	P_STAT(r2),$SRUN	##
	bneq	badsw			##
	clrl	P_RLINK(r2)		##
	tstl	CPU_TBI_FLAG(r3)	# see if TBI is needed
	beql	sw7			# nope
	mtpr	$0,$TBIA		# clear TB
	clrl	CPU_TBI_FLAG(r3)	# clear TBI request
sw7:
	movl	P_PCB(r2),r0		# get pcb address
	mtpr    r0,$TBIS                # make sure translation valid.
	movzbl	$NO_PRVCPU,PCB_PRVCPU(r0)	# PCB_ASN = 0 for now
	movl	r3,U_ESP(r0)		# save cpudata address

	movl	*P_ADDR(r2),r0
	movl	r0,CPU_PADDR(r3)	# save for adb backtrace

	PUSHR;	
	moval	_lk_rq,r0
	jsb	_Smp_unlock_short		
	POPR;

	tstl	CPU_HLOCK(r3)		# test for lock held after swtch
	bneq 	holdlock

	ashl	$PGSHIFT,r0,r0		# r0 = pcbb(p)
	brb	res0

holdlock:
	pushab	hold1
	calls	$1,_panic
	/* no return */

/*
 * Resume(pf)
 */
	.globl	_Resume 	# <<<massaged to jsb by "asm.sed">>>
_Resume:
	mtpr	$0x18,$IPL		# vax rei requires new ipl <= current
	svpctx
res0:
	cmpl	$VAX_9000,_cpu
	beql 	res_vax9000
	mtpr	r0,$PCBB
	brb	res_cont

res_vax9000:
	mfpr	$ESP,r3
	movl	CPU_PROC(r3),r2	
	mtpr	P_PCB(r2),$PCBB

res_cont:
	ldpctx
	tstl	_u+PCB_SSWAP
	bneq	res1
	rei

res1:
	movl	_u+PCB_SSWAP,r0 		# longjmp to saved context
	clrl	_u+PCB_SSWAP
	movq	(r0)+,r6
	movq	(r0)+,r8
	movq	(r0)+,r10
	movq	(r0)+,r12
	movl	(r0)+,r1
	cmpl	r1,sp				# must be a pop
	bgequ	1f
	pushab	2f
	calls	$1,_panic
	/* NOTREACHED */
1:
	movl	r1,sp
	movl	(r0),(sp)			# address to return to
	movl	$PSL_PRVMOD,4(sp)		# ``cheating'' (jfr)
	rei

2:	.asciz	"ldctx"

/*
 * Determine identity of cpu we are running on and possibly its table index
 *	Note: Due to dependencies in _start, we can not touch r1 + r3 in
 *	the cpuident case !!!
 */

	.globl	cpuident
cpuident:
	casel	_cpu,$1,$CPU_MAX
0:
	.word	3f-0b		# 1 is VAX_780
	.word	3f-0b		# 2 is VAX_750
	.word	3f-0b		# 3 is VAX_730
	.word	3f-0b		# 4 is VAX_8600
	.word	1f-0b		# 5 is VAX_8200
	.word	2f-0b		# 6 is VAX_8800
	.word	3f-0b		# 7 is MVAX_I
	.word	3f-0b		# 8 is MVAX_II
	.word	3f-0b		# 9 is VVAX
	.word	3f-0b		# 10 is VAX_3600 (Mayfair I)
	.word	4f-0b		# 11 is VAX_6200 (Calypso/CVAX)
	.word	3f-0b		# 12 is VAX_3400 (Mayfair II)
	.word	3f-0b		# 13 is C_VAXSTAR (PVAX)
	.word	6f-0b		# 14 is VAX_60 (Firefox)
	.word	3f-0b		# 15 is VAX_3900 (Mayfair III)
	.word	1f-0b		# 16 RESERVED for MIPS processor
	.word	5f-0b		# 17 is VAX_8820 (Polarstar)
	.word	1f-0b		# 18 RESERVED for MIPS processor
	.word	1f-0b		# 19 RESERVED for MIPS processor
	.word	1f-0b		# 20 RESERVED for MIPS processor
	.word	1f-0b		# 21 RESERVED for MIPS processor
	.word	7f-0b		# 22 is VAX6400 (Calypso Rigel)
	.word	3f-0b		# 23 is VAXSTAR
	.word	1f-0b		# 24 RESERVED for MIPS processor
	.word	1f-0b		# 25 RESERVED for MIPS processor
	.word	9f-0b		# 26 is VAX_9000

7:
#ifdef VAX6400
	movl	_rssc+0x40,r0		#Read RSSC IPORT register
	bicl2	$0xfffffff0,r0		#Get node ID of processor
#endif VAX6400
	rsb
6:
#ifdef VAX60
	mfpr	$CPUID,r0		# VAX60 - just need something unique
#endif	VAX60
	rsb

4:
#ifdef VAX6200
	movl	_v6200csr,r0
	bicl2	$0xfffffff0,r0
#endif VAX6200
	rsb

1:
#ifdef	VAX8200
	mfpr	$BINID,r0		# VAX8200
#endif	VAX8200
	rsb

5:
	mfpr	$SID,r0			# VAX8820 
	ashl	$-22,r0,r0
	bicl2	$0xfffffffc,r0
	rsb
2:
	mfpr	$SID,r0			# VAX8800 
	ashl	$-23,r0,r0
	bicl2	$0xfffffffe,r0
	rsb
9:
	mfpr	$CPUID,r0		# VAX9000
	rsb

3:
	movl	$1,r0			# no choice for uniprocessors
	rsb


/*
 * kernacc - check for kernel access privileges
 *
 * We can't use the probe instruction directly because
 * it ors together current and previous mode.
 */
	.globl	_kernacc
_kernacc:
	.word	0x0
	movl	4(ap),r0	# virtual address
	bbcc	$31,r0,kacc1
	bbs	$30,r0,kacerr
	mfpr	$SBR,r2 	# address and length of page table (system)
	bbss	$31,r2,0f; 0:
	mfpr	$SLR,r3
	brb	kacc2
kacc1:
	bbsc	$30,r0,kacc3
	mfpr	$P0BR,r2	# user P0
	mfpr	$P0LR,r3
	brb	kacc2
kacc3:
	mfpr	$P1BR,r2	# user P1 (stack)
	mfpr	$P1LR,r3
kacc2:
	addl3	8(ap),r0,r1	# ending virtual address
	addl2	$NBPG-1,r1
	ashl	$-PGSHIFT,r0,r0
	ashl	$-PGSHIFT,r1,r1
	bbs	$31,4(ap),kacc6
	bbc	$30,4(ap),kacc6
	cmpl	r0,r3		# user stack
	blss	kacerr		# address too low
	brb	kacc4
kacc6:
	cmpl	r1,r3		# compare last page to P0LR or SLR
	bgtr	kacerr		# address too high
kacc4:
	movl	(r2)[r0],r3
	bbc	$31,4(ap),kacc4a
	bbc	$31,r3,kacerr	# valid bit is off
kacc4a:
	cmpzv	$27,$4,r3,$1	# check protection code
	bleq	kacerr		# no access allowed
	tstb	12(ap)
	bneq	kacc5		# only check read access
	cmpzv	$27,$2,r3,$3	# check low 2 bits of prot code
	beql	kacerr		# no write access
kacc5:
	aoblss	r1,r0,kacc4	# next page
	movl	$1,r0		# no errors
	ret
kacerr:
	clrl	r0		# error
	ret
/*
 * Extracted and unrolled most common case of pagein (hopefully):
 *	resident and not on free list (reclaim of page is purely
 *	for the purpose of simulating a reference bit)
 *
 * Built in constants:
 *	CLSIZE == 2
 *	0x400000 pages in user space (P0+P1)
 *
 * Register usage:
 *	r0	scratch
 *	r1	cpudata structure pointer
 *	r2	First, text(==1)/not-text(==0); later, value of CPU_TBI_FLAG
 *	r3	VAX virtual page number (rounded down to cluster boundary)
 *	r4	address of PTE
 *	r5	proc structure pointer
 */
	.text
	.globl	Fastreclaim
Fastreclaim:
	PUSHR
	ashl	$-PGSHIFT,28(sp),r3	# (user) virtual address
	movl	_u+U_PROCP,r5		# p = u.u_procp
	clrl	r2			#	type = !CTEXT;
	bicl2	$1,r3			# v = clbase(btop(virtaddr));
					# from vtopte(p, v) ...
	subl3	P_SSIZE(r5),$(0x400000-HIGHPAGES),r0
	cmpl	r3,r0
	bgequ	2f			# stack
	ashl	$2,r3,r4		#	{t|d}ptopte(p, vtodp(p, v));
	cmpl	r3,P_TSIZE(r5)
	bgequ	3f			# data
	incl	r2			#	type = CTEXT;
	brb	3f
2:
	cvtwl	P_SZPT(r5),r4		# } else (isassv(p, v)) {
	ashl	$7,r4,r4
	subl2	$0x400000,r4
	addl2	r3,r4
	ashl	$2,r4,r4		#	sptopte(p, vtosp(p, v));
3:					# }
	addl2	P_P0BR(r5),r4
	bitl	$(PG_V | PG_FOD | PG_ALLOC),(r4)
	bneq	8f	# if (pte->pg_v || pte->pg_fod || pte->pg_alloc)

	mtpr	$0x16,$IPL
	pushr	$0x3c			# need to preserve r2, r3, r4, r5
	moval	_lk_cmap,r0
	jsb	_Smp_lock_once		# may do a CALLS
	tstl	r0
	popr	$0x3c
	beql	8f			# didn't get the lock
	bicl3	$(~PG_PFNUM),(r4),r0
	beql	7f			# if (pte->pg_pfnum == 0)
	subl2	_firstfree,r0
	ashl	$-1,r0,r0
	incl	r0			# pgtocm(pte->pg_pfnum)
	mull2	$CMAPSZ,r0
	addl2	_cmap,r0		# &cmap[pgtocm(pte->pg_pfnum)]
	tstl	r2
	beql	9f			# if (type == CTEXT &&
	bbc	$CMAP_INTRANS,(r0),9f	#     c_intrans)
7:
	moval	_lk_cmap,r0
	jsb	_Smp_unlock_short
8:
	POPR; rsb			#	let pagein handle it
9:
	bbs	$CMAP_FREE,(r0),7b	# if (c_free)
	bisl2	$PG_V,(r4)		# pte->pg_v = 1;
	bicl3	$~PG_M,4(r4),r0		# if (anycl(pte, pg_m)
	bisl2	r0,(r4)			#	pte->pg_m = 1;
	addl3	$1,(r4),4(r4)		# distcl(pte);
	ashl	$PGSHIFT,r3,r0
	mtpr	r0,$TBIS
	addl2	$NBPG,r0
	mtpr	r0,$TBIS		# tbiscl(v);
	tstl	r2
	beql	6f			# if (type == CTEXT)
	pushr	$0x38			# need to preserve r3, r4, r5
	moval	_lk_text,r0
	jsb	_Smp_lock_retry		# may do a CALLS
	popr	$0x38
	movl	P_TEXTP(r5),r0
	movl	X_CADDR(r0),r5		# for (p = p->p_textp->x_caddr; p; ) {
	beql	5f
	mfpr	$ESP,r1				# see which CPU we are
	movl	CPU_TBI_FLAG(r1),r2	# extract TBI state
	ashl	$2,r3,r3
3:
	addl3	P_P0BR(r5),r3,r0	#	tpte = tptopte(p, tp);
/*	bisb2	$1,P_VM+3(r5)	 	#	p->p_vm |= SPTECHG;	*/
	tstl	r2			# TBI pending?
	beql	4f			# No.
	mtpr	r0,$TBIS		# Yes, but just do 1 page table page.
4:
	movq	(r4),(r0)		#	for (i = 0; i < CLSIZE; i++)
					#		tpte[i] = pte[i];
	movl	P_XLINK(r5),r5		#	p = p->p_xlink;
	bneq	3b			# }
5:
	moval	_lk_text,r0
	jsb	_Smp_unlock_short
6:					# collect a few statistics...
	incl	_u+U_RU+RU_MINFLT	# u.u_ru.ru_minflt++;
	moval	_cnt,r0
	incl	V_FAULTS(r0)		# cnt.v_faults++;
	incl	V_PGREC(r0)		# cnt.v_pgrec++;
	incl	V_FASTPGREC(r0) 	# cnt.v_fastpgrec++;
	mfpr	$1,r0			# get percpu structure.
	incl	CPU_TRAP(r0)		# 
	moval	_lk_cmap,r0
	jsb	_Smp_unlock_short
	POPR
	addl2	$8,sp			# pop pc, code
	mtpr	$HIGH,$IPL		## dont go to a higher IPL (GROT)
	rei

	.data
_qioentry:
	.long 0

	.data
	.globl	_fl_ok
_fl_ok: .long	0
	.text
	

	.set	SAVE,8
	.globl	__km_alloc
__km_alloc:
#ifdef	KMEM_DEBUG
	bisl3	_kmdebug,_smp_debug,r0	# any debug enabled
	bneq	0f			# enabled
#endif	KMEM_DEBUG
	cmpl	12(sp),$MAXBUCKETSAVE	# max bucket
	bleq	1f			# saved buckets
0:	movl	(sp),r0			# return pc
	movl	12(sp),(sp)		# index 
	movl	r0,12(sp)		# return pc
	calls	$3,_km_alloc		# call general allocator
	rsb				# return
1:	cmpl	12(sp),$MINBUCKET	# minimum bucket
	jlss	9f			# bogus
	movq	r6,-(sp)		# save r6 and r7
	mfpr	$IPL,r6			# current spl
	mtpr	$SPLDEVHIGH,$IPL	# splimp
	movab	_lk_buckets,r0		# address of lock_t
	jsb	_Smp_lock_retry		# smp_lock(&lk_buckets,LK_RETRY)
	movl	12+SAVE(sp),r0		# bucket index
	bitl	$KM_WIRED,8+SAVE(sp)	# wired bucket
	bneq	2f			# yes
	movab	_bucket,r1		# address of unwired
	brb	3f			# continue
2:	movab	_wbucket,r1		# wired buckets
3:	mull2	$KB_SIZE,r0		# index into bucket array
	movab	(r1)[r0],r1		# bucket address
	remque	*KB_EFL(r1),r7		# remove element
	bvc	4f			# not empty
	movab	_lk_buckets,r0		# bucket address
	jsb	_Smp_unlock_short	# smp_unlock(&lk_buckets)
	mtpr	r6,$IPL			# splx(s)
	movq	(sp)+,r6		# pop stack
	brb	0b			# long call
4:	subl3	$_kmembase,r7,r1	# kup space
	ashl	$-CLSHIFT,r1,r1		# index
	mull2	$KU_SIZE,r1		# offset
	addl2	_kmemusage,r1		# address of kup
	cmpl	r7,$_kmembase		# base address
	jlssu	km_badva		# bad va
	cmpl	r7,$_kmemlimit		# maximum va
	jgequ	km_badva		# bad va
	incw	KU_REFCNT(r1)		# kup->ku_refcnt++
	cmpl	KU_HELE(r1),KU_TELE(r1) # last element on usage
	bneq	5f			# no
	clrl	KU_HELE(r1)		# clear head
	brb	6f			# continue
5:	movl	KE_FL(r7),KU_HELE(r1)	# reset head
6:	movab	_lk_buckets,r0		# address of lock_t
	jsb	_Smp_unlock_short	# unlock
	mtpr	r6,$IPL			# restore spl
	movl	4+SAVE(sp),r0		# type	
	incl	_kmemu[r0]		# kmemu[type]++
	bitl	$KM_CLEAR,8+SAVE(sp)	# KM_CLEAR
	beql	7f			# not set
	ashl	12+SAVE(sp),$1,r6	# size of area
	movc5	$0,(r7),$0,r6,(r7)	# clear the buffer
7:	movl	r7,r0			# km_alloc pointer
	movq	(sp)+,r6		# restore r6 and r7
8:	movl	(sp),r1			# return pc
	movab	16(sp),sp		# pop stack
	jmp	(r1)			# return
9:	clrl	r0			# bad index supplied nul buffer
	brb	8b			# go exit
	
	.globl	__km_free
__km_free:
#ifdef	KMEM_DEBUG
	bisl3	_kmdebug,_smp_debug,r0	# any debug on
	bneq	1f			# yes
#endif	KMEM_DEBUG
	movq	r6,-(sp)		# save r6 and r7
	movl	8+SAVE(sp),r6		# address
	subl3	$_kmembase,r6,r7	# memory base
	ashl	$-CLSHIFT,r7,r7		# index
	mull2	$KU_SIZE,r7		# offset
	addl2	_kmemusage,r7		# address of kup
	cmpl	r6,$_kmembase		# base address
	jlssu	km_badva		# bad va
	cmpl	r6,$_kmemlimit		# maximum va
	jgequ	km_badva		# bad va
	cmpw	KU_INDEX(r7),$MAXBUCKETSAVE
	bleq	2f			# continue	
	movq	(sp)+,r6		# restore r6 and r7
1:	movl	(sp),r0			# return pc
	movl	8(sp),(sp)		# addr to pc
	movl	r0,8(sp)		# return pc
	calls	$2,_km_free		# call free
	rsb				# back to caller
2:	mfpr	$IPL,r0			# current spl
	movl	r0,8+SAVE(sp)		# save the spl
	mtpr	$SPLDEVHIGH,$IPL	# splimp
	movab	_lk_buckets,r0		# lock_t addr
	jsb	_Smp_lock_retry		# get the lock
	decw	KU_REFCNT(r7)		# kup->ku_refcnt--
	jlss	km_badfree		# bad count
	beql	3f			# count zero
	cmpw	KU_INDEX(r7),$KMBUCKET	# small bucket
	bgeq	8f			# done
3:	tstl	KU_HELE(r7)		# kup->ku_hele
	bneq	6f			# usage not empty
	cmpl	r7,_uwkmemusage		# unwired
	blequ	4f			# yes
	movab	_wbucket,r0		# wired bucket
	brb	5f			# continue
4:	movab	_bucket,r0		# address of bucket
5:	movzwl	KU_INDEX(r7),r1		# index
	mull2	$KB_SIZE,r1		# offset
	movab	(r0)[r1],r0		# bucket address
	movl	r6,KU_HELE(r7)		# update head
	insque	(r6),(r0)		# insert element
	brb	7f			# continue
6:	insque	(r6),*KU_TELE(r7)	# insert
7:	movl	r6,KU_TELE(r7)		# update tail
	movl	4+SAVE(sp),r0		# type
	decl	_kmemu[r0]		# kmemu[type]--
8:	movab	_lk_buckets,r0		# &lk_buckets
	jsb	_Smp_unlock_short	# unlock
	movl	8+SAVE(sp),r0		# spl
	mtpr	r0,$IPL			# restore spill
	movq	(sp)+,r6		# restore register
	movl	(sp),r0			# return pc
	movab	12(sp),sp		# pop stack
	jmp	(r0)			# return

	.globl	__km_memdup
__km_memdup:
#ifdef	KMEM_DEBUG
	bisl3	_smp_debug,_kmdebug,r0	# debug on
	beql	0f			# none
	movl	(sp),r0			# return pc
	movl	4(sp),(sp)		# address
	movl	r0,4(sp)		# stash pc
	calls	$1,_km_memdup		# call dup code
	rsb				# back to caller
0:
#endif	KMEM_DEBUG
	movl	r6,-(sp)		# save r6
	mfpr	$IPL,r6			# current spl
	mtpr	$SPLDEVHIGH,$IPL	# splimp
	movab	_lk_buckets,r0		# lock address
	jsb	_Smp_lock_retry		# grab lock
	subl3	$_kmembase,8(sp),r0	# memory base
	ashl	$-CLSHIFT,r0,r0		# index
	mull2	$KU_SIZE,r0		# offset
	addl2	_kmemusage,r0		# address of kup
	incw	KU_REFCNT(r0)		# bump reference count
	movab	_lk_buckets,r0		# lock address
	jsb	_Smp_unlock_short	# unlock
	mtpr	r6,$IPL			# restore spl
	movl	(sp)+,r6		# restore r6
	movl	(sp),r0			# return pc
	movab	8(sp),sp		# pop stack
	jmp	(r0)			# back to caller

	.data
bad0:
	.asciz "KM_ALLOC: bucket corruption"
bad1:
	.asciz	"KM_FREE: multiple frees"
	.text
km_badva:
	pushab	bad0			# panic string
	calls	$1,_panic		# done
	halt

km_badfree:
	pushab	bad1			# panic string
	calls	$1,_panic		# done
	halt

/*
 *
 * test to see if a two-byte opcode is a legal vector instruction
 *
 * this code is a blatent copy of the vaxfloat.s code
 *
 * for clarity: if this were a C routine, it would have the following header:
 *
 * int is_legal_vector_instruction (ptr)
 * short	*opcode;
 */

	.globl	_is_legal_vector_instruction
_is_legal_vector_instruction:
	.word	0x0

	movl	4(ap),r0		# get the first byte (use a move zero
	movzbl	(r0),r1			# extended byte to long instruction
					# so that bits 8-31 are clear)

	cmpl	r1,$0xfd		# first byte of a vector instruction
	bneq	not_vect_inst		# is always 0xfd

					# first byte is legal, 
	incl	r0			# get second byte of opcode
	movzbl	(r0), r1

	# use the contents of r1 as an index into the bit array at address 
	# vect_op_2byte.  if the bit is set, then the whole opcode is a 
	# legal vector opcode.

	bbc	r1,vect_op_2byte,not_vect_inst

	movl	$is_vect_inst,r0
	ret
not_vect_inst:
	movl	$isnt_vect_inst,r0
	ret


	# vector legal opcodes:
	#	0x80fd          0xa0fd  0xc0fd  0xe0fd
	#	0x31fd  0x81fd          0xa1fd  0xc1fd  0xe1fd
	#	        0x82fd          0xa2fd  0xc2fd       
	#	        0x83fd          0xa3fd  0xc3fd       
	#	0x34fd  0x84fd          0xa4fd  0xc4fd  0xe4fd
	#	0x35fd  0x85fd          0xa5fd  0xc5fd  0xe5fd
	#	0x36fd  0x86fd          0xa6fd  0xc6fd       
	#	0x37fd  0x87fd          0xa7fd  0xc7fd       
	#	        0x88fd          0xa8fd  0xc8fd  0xe8fd
	#	        0x89fd          0xa9fd  0xc9fd  0xe9fd
	#	        0x8afd          0xaafd              
	#	        0x8bfd          0xabfd              
	#	        0x8cfd  0x9cfd  0xacfd  0xccfd  0xecfd
	#	        0x8dfd  0x9dfd  0xadfd  0xcdfd  0xedfd
	#	        0x8efd  0x9efd  0xaefd          0xeefd
	#	        0x8ffd  0x9ffd  0xaffd          0xeffd

vect_op_2byte:
	.long	0x00000000	# first byte 0x00 - 0x1f
	.long	0x00f20000	# first byte 0x20 - 0x3f
	.long	0x00000000	# first byte 0x40 - 0x5f
	.long	0x00000000	# first byte 0x60 - 0x7f
	.long	0xf000ffff	# first byte 0x80 - 0x9f
	.long	0x0000ffff	# first byte 0xa0 - 0xbf
	.long	0x000033ff	# first byte 0xc0 - 0xdf
	.long	0x0000f333	# first byte 0xe0 - 0xff

	.text
