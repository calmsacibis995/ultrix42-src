/*	@(#)tlb.s	4.1      (ULTRIX)        7/2/90	*/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * Revision History:
 *
 * 19-Oct-89 -- jmartin
 *	For R2000/R3000, shift PTE left 8 just before writing C0_TLBLO.
 */

#include "../machine/param.h"
#include "../machine/cpu.h"
#include "../machine/asm.h"
#include "../machine/reg.h"
#include "../machine/regdef.h"

#if NOMEMCACHE==1
#include "../machine/pte.h"
#endif

#include "assym.h"


/*
 * NOTE: These routines are coded conservatively in regards to nop's
 * and m[tf]c0 operations.  Some of the nop's may be able to be removed
 * after consulting with the chip group.  Also note: the routines are
 * coded noreorder to avoid the reorganizer moving C0 instructions around
 * that it doesn't realize are order dependent.
 */

#ifdef notdef
/*
 * Extracted and unrolled most common case of pagein (hopefully):
 *	resident and not on free list (reclaim of page is purely
 *	for the purpose of simulating a reference bit)
 *
 * Built in constants:
 *	CLSIZE of 2, USRSTACK of 0x7ffff000, any bit fields
 *	in pte's or the core map
 */
	.text
	.globl	Fastreclaim
Fastreclaim:
	PUSHR
	extzv	$9,$23,28(sp),r3	# virtual address
	bicl2	$1,r3			# v = clbase(btop(virtaddr)); 
	movl	_u+U_PROCP,r5		# p = u.u_procp 
					# from vtopte(p, v) ...
	cmpl	r3,P_TSIZE(r5)
	jgequ	2f			# if (isatsv(p, v)) {
	ashl	$2,r3,r4
	addl2	P_P0BR(r5),r4		#	tptopte(p, vtotp(p, v));
	movl	$1,r2			#	type = CTEXT;
	jbr	3f
2:
	subl3	P_SSIZE(r5),$0x3ffff8,r0
	cmpl	r3,r0
	jgequ	2f			# } else if (isadsv(p, v)) {
	ashl	$2,r3,r4
	addl2	P_P0BR(r5),r4		#	dptopte(p, vtodp(p, v));
	clrl	r2			#	type = !CTEXT;
	jbr	3f
2:
	cvtwl	P_SZPT(r5),r4		# } else (isassv(p, v)) {
	ashl	$7,r4,r4
	subl2	$(0x3ffff8+UPAGES),r4
	addl2	r3,r4
	ashl	$2,r4,r4
	addl2	P_P0BR(r5),r4		#	sptopte(p, vtosp(p, v));
	clrl	r2			# 	type = !CTEXT;
3:					# }
	bitb	$0x82,3(r4)
	beql	2f			# if (pte->pg_v || pte->pg_fod)
	POPR; rsb			#	let pagein handle it
2:
	bicl3	$0xffe00000,(r4),r0
	jneq	2f			# if (pte->pg_pfnum == 0)
	POPR; rsb			# 	let pagein handle it 
2:
	subl2	_firstfree,r0
	ashl	$-1,r0,r0	
	incl	r0			# pgtocm(pte->pg_pfnum) 
	mull2	$12,r0
	addl2	_cmap,r0		# &cmap[pgtocm(pte->pg_pfnum)] 
	tstl	r2
	jeql	2f			# if (type == CTEXT &&
	jbc	$29,4(r0),2f		#     c_intrans)
	POPR; rsb			# 	let pagein handle it
2:
	jbc	$30,4(r0),2f		# if (c_free)
	POPR; rsb			# 	let pagein handle it 
2:
	bisb2	$0x80,3(r4)		# pte->pg_v = 1;
	jbc	$26,4(r4),2f		# if (anycl(pte, pg_m) 
	bisb2	$0x04,3(r4)		#	pte->pg_m = 1;
2:
	bicw3	$0x7f,2(r4),r0
	bicw3	$0xff80,6(r4),r1
	bisw3	r0,r1,6(r4)		# distcl(pte);
	ashl	$PGSHIFT,r3,r0
	mtpr	r0,$TBIS
	addl2	$NBPG,r0
	mtpr	r0,$TBIS		# tbiscl(v); 
	tstl	r2
	jeql	2f			# if (type == CTEXT) 
	movl	P_TEXTP(r5),r0
	movl	X_CADDR(r0),r5		# for (p = p->p_textp->x_caddr; p; ) {
	jeql	2f
	ashl	$2,r3,r3
3:
	addl3	P_P0BR(r5),r3,r0	#	tpte = tptopte(p, tp);
	bisb2	$1,P_FLAG+3(r5)		#	p->p_flag |= SPTECHG;
	movl	(r4),(r0)+		#	for (i = 0; i < CLSIZE; i++)
	movl	4(r4),(r0)		#		tpte[i] = pte[i];
	movl	P_XLINK(r5),r5		#	p = p->p_xlink;
	jneq	3b			# }
2:					# collect a few statistics...
	incl	_u+U_RU+RU_MINFLT	# u.u_ru.ru_minflt++;
	moval	_cnt,r0
	incl	V_FAULTS(r0)		# cnt.v_faults++; 
	incl	V_PGREC(r0)		# cnt.v_pgrec++;
	incl	V_FASTPGREC(r0)		# cnt.v_fastpgrec++;
	incl	V_TRAP(r0)		# cnt.v_trap++;
	POPR
	addl2	$8,sp			# pop pc, code
	mtpr	$HIGH,$IPL		## dont go to a higher IPL (GROT)
	rei
#endif

/*
 * unmaptlb(rpid, vpage): unmap a single tlb entry in the TLB.
 *
 *	Probe for RPID/VPAGE in TLB.  If it doesn't exist, all done.
 *	If it does, mark the entries as invalid.
 *	Interrupts must be disabled because interrupts can generate
 *	a tlbmiss which will destroy the C0 registers.
 */
LEAF(unmaptlb)
	.set	noreorder
	mfc0	t0,C0_SR		# save SR
	mfc0	t1,C0_TLBHI		# save TLBHI (for TLBPID)
	mtc0	zero,C0_SR		# no interrupts
	sll	a1,TLBHI_VPNSHIFT	# shift vpn
	sll	a0,TLBHI_PIDSHIFT	# shift tlbpid
	or	t2,a1,a0		# tlbhi value to look for
	mtc0	t2,C0_TLBHI		# put args into tlbhi
	nop				# let tlbhi get through pipe
	c0	C0_PROBE		# probe for address
	mfc0	t3,C0_INX		# see what happened
	move	v0,zero			# LDSLOT
	bltz	t3,1f			# probe failed
	nop

#ifdef PROBE_BUG
	move	t4,t3			# start with original index "hint"
					# this should be the lower bounds.
6:
	c0	C0_READI		# read an entry.
	mfc0	t5,C0_TLBHI
	nop
	beq	t5,t2,2f		# test for pid/vpn match
	nop
	and	t5,TLBHI_VPNMASK	# isolate vpn
	bne	t5,a1,3f		# test vpn match for global pages
	nop
	mfc0	t5,C0_TLBLO		# vpn match, check global bit
	nop
	and	t5,TLBLO_G
	bne	t5,zero,2f
	nop
					# did not find it, try another slot
3:
	li	t5,TLBINX_INXMASK<<TLBINX_INXSHIFT
	beq	t4,t5,4f
	nop
	add	t4,1<<TLBINX_INXSHIFT
5:	mtc0	t4,C0_INX
	b	6b
	nop
	/*
	 * Bad news if we get to here. The entire tlb has been searched
	 * without finding the entry that match on the probe instruction.
	 * Return -1 to the caller and let him panic. To dangerous to restore
	 * the sr.
	 */
4:	li	v0,-1			# searched entire tlb, panic time.
	j	ra			# let caller panic
	nop
2:
	sne	v0,t3,t4		# found the match
#endif	PROBE_BUG

	li	t2,K0BASE&TLBHI_VPNMASK	# BDSLOT
	mtc0	t2,C0_TLBHI		# invalidate entry
	mtc0	zero,C0_TLBLO		# cosmetic
	nop
	c0	C0_WRITEI
1:	mtc0	t1,C0_TLBHI		# restore old TLBHI
	nop
	.set	reorder
	.set noreorder
	nop
	mtc0	t0,C0_SR		# restore sr and return
	nop
	.set reorder
	j	ra
	END(unmaptlb)

/*
 * unmodtlb(rpid, vpage): Clear the dirty/writeable bit for the TLB.
 */
LEAF(unmodtlb)
	.set	noreorder
	mfc0	t0,C0_SR		# disable interrupts
	mfc0	t1,C0_TLBHI		# save TLBHI
	mtc0	zero,C0_SR
	sll	a1,TLBHI_VPNSHIFT	# construct new TLBHI
	sll	a0,TLBHI_PIDSHIFT	# shift tlbpid
	or	t2,a0,a1
	mtc0	t2,C0_TLBHI		# move to C0 for probe
	nop
	c0	C0_PROBE		# probe for address
	mfc0	t3,C0_INX
	move	v0,zero			# LDSLOT
	bltz	t3,1f			# probe failed
	nop				# BDSLOT

#ifdef PROBE_BUG
	move	t4,t3			# start with original index "hint"
6:
	c0	C0_READI		# read an entry.
	mfc0	t5,C0_TLBHI
	nop
	beq	t5,t2,2f		# test for pid/vpn match
	nop
	and	t5,TLBHI_VPNMASK	# isolate vpn
	bne	t5,a1,3f		# test vpn match for global pages
	nop
	mfc0	t5,C0_TLBLO		# vpn match, check global bit
	nop
	and	t5,TLBLO_G
	bne	t5,zero,2f
	nop
					# did not find it, try another slot
3:
	li	t5,TLBINX_INXMASK<<TLBINX_INXSHIFT
	beq	t4,t5,4f
	nop
	add	t4,1<<TLBINX_INXSHIFT
5:	mtc0	t4,C0_INX
	b	6b
	nop
	/*
	 * Bad news if we get to here. The entire tlb has been searched
	 * without finding the entry that match on the probe instruction.
	 * Return -1 to the caller and let him panic. To dangerous to restore
	 * the sr.
	 */
4:	li	v0,-1			# searched entire tlb, panic time.
	j	ra			# let caller panic
	nop
2:
	sne	v0,t3,t4		# found the match
#endif	PROBE_BUG
	c0	C0_READI		# load entry in TLBLO/TLBHI
	mfc0	t2,C0_TLBLO
	nop
	and	t2,~TLBLO_D		# reset modified bit
	mtc0	t2,C0_TLBLO		# unmod entry
	nop
	c0	C0_WRITEI
1:	mtc0	t1,C0_TLBHI		# restore old TLBHI
	nop
	.set	reorder
	.set noreorder
	nop
	mtc0	t0,C0_SR		# restore sr
	nop
	.set reorder
	j	ra
	END(unmodtlb)

/*
 * invaltlb(i): Invalidate the ith TLB entry.
 * called whenever a specific TLB entry needs to be invalidated.
 */
LEAF(invaltlb)
	.set	noreorder
	li	t2,K0BASE&TLBHI_VPNMASK
	mfc0	t0,C0_TLBHI		# save current TLBHI
	mfc0	v0,C0_SR		# save SR and disable interrupts
	mtc0	zero,C0_SR
	mtc0	t2,C0_TLBHI		# invalidate entry
	mtc0	zero,C0_TLBLO
	sll	a0,TLBINX_INXSHIFT
	mtc0	a0,C0_INX
	nop
	c0	C0_WRITEI
	mtc0	t0,C0_TLBHI
	nop
	.set	reorder
	.set noreorder
	nop
	mtc0	v0,C0_SR
	nop
	.set reorder
	j	ra
	END(invaltlb)

/*
 * tlbwired(indx, tlbpid, vaddr, pte) -- setup wired TLB entry
 * a0 -- indx -- tlb entry index
 * a1 -- tlbpid -- context number to use (0-63)
 * a2 -- vaddr -- virtual address (could have offset bits)
 * a3 -- pte -- contents of pte
 */
LEAF(tlbwired)
	.set	noreorder
	sll	a0,TLBINX_INXSHIFT
	mfc0	t1,C0_TLBHI		# save current TLBPID
	mfc0	v0,C0_SR		# save SR and disable interrupts
	mtc0	zero,C0_SR
	sll	a1,TLBHI_PIDSHIFT	# line up pid bits
	and	a2,TLBHI_VPNMASK	# chop offset bits
	or	a1,a2			# formatted tlbhi entry
	mtc0	a1,C0_TLBHI		# set VPN and TLBPID
#if NOMEMCACHE==1
	or	a3,PG_N
#endif
	sll	a3,8			# abstract format to R2000/R3000
	mtc0	a3,C0_TLBLO		# set PPN and access bits
	mtc0	a0,C0_INX		# set INDEX to wired entry
	nop
	c0	C0_WRITEI		# drop it in
	mtc0	t1,C0_TLBHI		# restore TLBPID
	nop
	.set	reorder
	.set noreorder
	nop
	mtc0	v0,C0_SR		# restore SR
	nop
	.set reorder
	j	ra
	END(tlbwired)

/*
 * tlbdropin(tlbpid, vaddr, pte) -- random tlb drop-in
 * a0 -- tlbpid -- tlbcontext number to use (0-63)
 * a1 -- vaddr -- virtual address to map. Can contain offset bits
 * a2 -- pte -- contents of pte
 *
 * Probes first to ensure that no other tlb entry exists with this pid
 * and vpn.
 */
LEAF(tlbdropin)
	.set	noreorder
	mfc0	t0,C0_SR		# save SR and disable interrupts
	mfc0	t1,C0_TLBHI		# save current pid
	mtc0	zero,C0_SR
	sll	a0,TLBHI_PIDSHIFT	# align pid bits for entryhi
	and	a1,TLBHI_VPNMASK	# chop any offset bits from vaddr
	or	t2,a0,a1		# vpn/pid ready for entryhi
	mtc0	t2,C0_TLBHI		# vpn and pid of new entry
#if NOMEMCACHE==1
	or	a2,PG_N
#endif
	sll	a2,8			# abstract format to R2000/R3000
	mtc0	a2,C0_TLBLO		# pte for new entry
	nop
	c0	C0_PROBE		# probe for stale value
	mfc0	t3,C0_INX	
	move	v0,zero			# LDSLOT
	bltz	t3,1f			# not found
	nop				# BDSLOT

#ifdef PROBE_BUG
	move	t4,t3			# start with original index "hint"
6:
	c0	C0_READI		# read an entry.
	mfc0	t5,C0_TLBHI
	nop
	beq	t5,t2,2f		# test for pid/vpn match
	nop
	and	t5,TLBHI_VPNMASK	# isolate vpn
	bne	t5,a1,3f		# test vpn match for global pages
	nop
	mfc0	t5,C0_TLBLO		# vpn match, check global bit
	nop
	and	t5,TLBLO_G
	bne	t5,zero,2f
	nop
					# did not find it, try another slot
3:
	li	t5,TLBINX_INXMASK<<TLBINX_INXSHIFT
	beq	t4,t5,4f
	nop
	add	t4,1<<TLBINX_INXSHIFT
5:	mtc0	t4,C0_INX
	b	6b
	nop
	/*
	 * Bad news if we get to here. The entire tlb has been searched
	 * without finding the entry that match on the probe instruction.
	 * Return -1 to the caller and let him panic. To dangerous to restore
	 * the sr.
	 */
4:	li	v0,-1			# searched entire tlb, panic time.
	j	ra			# let caller panic
	nop
2:
	sne	v0,t3,t4		# found the match
	mtc0	t2,C0_TLBHI
	mtc0	a2,C0_TLBLO
	nop
#endif	PROBE_BUG
	c0	C0_WRITEI		# re-use slot
	b	2f
	nop				# BDSLOT

1:	c0	C0_WRITER		# use random slot
2:	mtc0	t1,C0_TLBHI		# restore TLBPID
	nop
	.set	reorder
	.set noreorder
	nop
	mtc0	t0,C0_SR		# restore SR
	nop
	.set reorder
	j	ra
	END(tlbdropin)

/*
 * flush entire non-wired tlb
 */
LEAF(flush_tlb)
	.set	noreorder
	li	t0,K0BASE&TLBHI_VPNMASK	# set up to invalidate entries
	li	v0,TLBRANDOMBASE	# first entry to invalidate
	li	v1,TLBRANDOMBASE+NRANDOMENTRIES	# last entry plus one
	mfc0	t1,C0_TLBHI		# save pid
	mfc0	t2,C0_SR
	mtc0	zero,C0_SR		# interrupts off
	mtc0	zero,C0_TLBLO
	mtc0	t0,C0_TLBHI
	sll	t0,v0,TLBINX_INXSHIFT
1:
	mtc0	t0,C0_INX		# set index
	addu	v0,1			# bump to next entry
	c0	C0_WRITEI		# invalidate
	bne	v0,v1,1b		# more to do
	sll	t0,v0,TLBINX_INXSHIFT	# BDSLOT

	mtc0	t1,C0_TLBHI
	nop
	.set	reorder

	.set noreorder
	nop
	mtc0	t2,C0_SR
	nop
	.set reorder
	j	ra
	END(flush_tlb)

/*
 * set_tlbpid -- set current tlbpid and change all the pid fields
 * in the wired tlb entries to the new pid.
 * a0 -- tlbpid -- tlbcontext number to use (0-63)
 */
LEAF(set_tlbpid)
	.set	noreorder
	mfc0	t0,C0_SR
	mtc0	zero,C0_SR		# interrupts off
	sll	a0,TLBHI_PIDSHIFT	# line up pid bits
	li	t1,NWIREDENTRIES-1
1:
	sll	t2,t1,TLBINX_INXSHIFT
	mtc0	t2,C0_INX
	nop
	c0	C0_READI		# read current tlb entry
	mfc0	t2,C0_TLBHI
	and	t2,~TLBHI_PIDMASK	# take down current pid bits
	or	t2,a0			# assert new pid
	mtc0	t2,C0_TLBHI		# write back new value
	nop
	c0	C0_WRITEI
	bne	t1,zero,1b
	sub	t1,1			# BDSLOT
	.set	reorder

	.set noreorder
	nop
	mtc0	t0,C0_SR
	nop
	.set reorder
	j	ra
	END(set_tlbpid)

/*
 * set_tlbwired(p)
 * drop in process' wired tlb mappings
 * return 1
 * NOTE: pcb must be first in u area!
 */
LEAF(set_tlbwired)
	/*
	 * noreorder here, because I'm not sure that the assembler
	 * will realize that changing the TLBPID is dependent on
	 * setting the SR to disable interrupts.
	 */
	.set	noreorder
	/*
	 * Disable interrupts and fetch the new pid value.
	 */
	mfc0	v0,C0_SR		# save status register
	lw	t0,P_TLBPID(a0)		# load new tlbpid
	mtc0	zero,C0_SR		# disable interrupts
	sll	t0,TLBHI_PIDSHIFT	# line up pid bits

	/*
	 * Read the upage mapping, change the pid, then write it back
	 */
	li	t2,(TLBWIREDBASE<<TLBINX_INXSHIFT) # wired entry index
	mtc0	t2,C0_INX
	nop
	c0	C0_READI
	mfc0	t3,C0_TLBHI
	nop
	and	t3,~TLBHI_PIDMASK
	or	t3,t0
	mtc0	t3,C0_TLBHI
	nop
	c0	C0_WRITEI

	/*
	 * Read the kstack mapping, change the pid, then write it back
	 */
	li	t2,((TLBWIREDBASE+1)<<TLBINX_INXSHIFT) # wired entry index
	mtc0	t2,C0_INX
	nop
	c0	C0_READI
	mfc0	t3,C0_TLBHI
	nop
	and	t3,~TLBHI_PIDMASK
	or	t3,t0
	mtc0	t3,C0_TLBHI
	nop
	c0	C0_WRITEI

	/*
	 * Setup a loop for the 2nd level map entries
	 */
	li	t2,((TLBWIREDBASE+2)<<TLBINX_INXSHIFT) # wired entry index
	lw	t1,P_TLBINFO(a0)	# load phys address of tlb mappings
	move	t7,zero			# zero loop counter

	/*
	 * Loop, dropping all per process mappings into tlb wired entries
	 */
1:
	lw	t3,0(t1)		# load pte
	lw	t4,4(t1)		# load vpn
#if NOMEMCACHE==1
	or	t3,PG_N			# set nocache bit
#endif NOMEMCACHE
	sll	t3,8			# abstract format to R2000/R3000
	mtc0	t2,C0_INX		# set wired entry index
	or	t4,t0			# or in tlbpid with vpn
	mtc0	t3,C0_TLBLO		# set pfn and access bits
	mtc0	t4,C0_TLBHI		# set pid and vpn
	addu	t7,1			# bump loop counter
	c0	C0_WRITEI		# perform mapping
	addu	t2,(1<<TLBINX_INXSHIFT)	# bump wired entry index
	bne	t7,NPAGEMAP,1b		# loop test
	addu	t1,8			# bump per process mapping offset
	.set	reorder

	.set noreorder
	nop
	mtc0	v0,C0_SR
	nop
	.set reorder
	li	v0,1			# return non-zero
	j	ra
	END(set_tlbwired)

/*
 * save_tlb(save_array) -- snapshots the tlb
 */
LEAF(save_tlb)
	.set	noreorder
	li	v0,TLBWIREDBASE		# first entry to save
	li	v1,NTLBENTRIES		# last entry plus one
	mfc0	t1,C0_TLBHI		# save pid
	mfc0	t2,C0_SR
	mtc0	zero,C0_SR		# interrupts off
1:
	sll	t0,v0,TLBINX_INXSHIFT	# set tlb index and read
	mtc0	t0,C0_INX
	nop
	c0	C0_READI
	mfc0	t5,C0_TLBHI		# Copy entry
	nop
	sw	t5,0(a0)
	addu	a0,4
	mfc0	t5,C0_TLBLO
	nop
	sw	t5,0(a0)
	addu	a0,4
	addu	v0,1			# bump to next tlb slot
	bne	v0,v1,1b		# done?
	sll	t0,v0,TLBINX_INXSHIFT	# BDSLOT
	mtc0	t1,C0_TLBHI
	nop
	mtc0	t2,C0_SR
	.set	reorder
	j	ra
	END(save_tlb)
