#ifndef lint
static	char	*sccsid = "@(#)langpats.c	4.3	(ULTRIX)	4/11/91";
#endif lint

/************************************************************************
 *									*
 *	      Copyright (c) 1984, 1986, 1987, 1988, 1989 by		*
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

/*-----------------------------------------------------------------------
 *
 * Modification History
 *
 * 11-Apr-91	dlh
 *	added the adawi() routine.
 *
 * 4-Sep-90	dlh
 *	added vector processor opcode
 *
 * 03-Mar-90 jaw
 *	primitive change to optimize mips.
 *
 * 10-Dec-89 -- Matt Thomas
 *	Fix typos in dna_add64.
 *
 * 09-Nov-89 -- jaw
 *	replace references to maxcpu to smp.
 *
 * 20-Sep-89 -- Pete Keilty
 *	Added unixtovms inline expansion used by SCA.
 *
 * 20-Jul-89 jaw
 *	change bbssi and bbcci routine to set_bit_atomic and clear_bit_atomic.
 *	this was done to be cpu generic and to not be misleading.
 *
 * 26-Jun-89 -- Matt Thomas
 *	add inline expansions for DECnet 64-bit counters
 *
 * 22 Nov 88 -- jaw 
 *	fix case where we were sending signal to our parent before we
 *	were REALLY stopped....  more magic.
 *
 * 27-Jun-88 -- larry
 *	add getpsl.  used by printf.
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
 * 3-Sep-87 -- Jaw
 *	speed up for hardclock....added diskcheck macro.
 *
 * 19-Mar-87 -- fred (Fred Canter for Brian Stevens)
 *	Added inline expansions needed for X in the kernel.
 *
 * 02-Apr-86 -- jrs
 *	Changed lock pattern for efficiency.  Also only do cpuindex
 *	if number if active cpus greater than one.
 *
 * 18 Mar 86 -- jrs
 *	Changed patterns for cpuindex and cpuident to match new locore
 *
 * 16 Jul 85 -- jrs
 *	Added patterns for lock and unlock primitives
 *
 *	Based on 4.2bsd labeled:
 *		langpats.c	2.6 (Berkeley) 11/19/84
 *
 */

#include "inline.h"

/*
 * Pattern table for kernel specific routines.
 * These patterns are based on the old asm.sed script.
 */
struct pats language_ptab[] = {

	{ "0,_spl0\n",
"	mfpr	$18,r0\n\
	mtpr	$0,$18\n" },

	{ "0,_spl1\n",
"	mfpr	$18,r0\n\
	mtpr	$1,$18\n" },

	{ "0,_spl2\n",
"	mfpr	$18,r0\n\
	mtpr	$2,$18\n" },

	{ "0,_splsoftclock\n",
"	mfpr	$18,r0\n\
	mtpr	$0x8,$18\n" },

	{ "0,_splnet\n",
"	mfpr	$18,r0\n\
	mtpr	$0xc,$18\n" },

	{ "0,_splimp\n",
"	mfpr	$18,r0\n\
	mtpr	$0x15,$18\n" },

	{ "0,_spl4\n",
"	mfpr	$18,r0\n\
	mtpr	$0x14,$18\n" },

	{ "0,_splbio\n",
"	mfpr	$18,r0\n\
	mtpr	$0x15,$18\n" },

	{ "0,_spltty\n",
"	mfpr	$18,r0\n\
	mtpr	$0x15,$18\n" },

	{ "0,_spl5\n",
"	mfpr	$18,r0\n\
	mtpr	$0x15,$18\n" },

	{ "0,_splclock\n",
"	mfpr	$18,r0\n\
	mtpr	$0x18,$18\n" },

	{ "0,_spl6\n",
"	mfpr	$18,r0\n\
	mtpr	$0x18,$18\n" },

	{ "0,_splhigh\n",
"	mfpr	$18,r0\n\
	mtpr	$0x18,$18\n" },

	{ "0,_spl7\n",
"	mfpr	$18,r0\n\
	mtpr	$0x1f,$18\n" },

	{ "0,_splextreme\n",
"	mfpr	$18,r0\n\
	mtpr	$0x1f,$18\n" },

	{ "1,_splx\n",
"	movl	(sp)+,r1\n\
	mfpr	$18,r0\n\
	mtpr	r1,$18\n" },

	{ "1,_mfpr\n",
"	movl	(sp)+,r5\n\
	mfpr	r5,r0\n" },

	{ "2,_mtpr\n",
"	movl	(sp)+,r4\n\
	movl	(sp)+,r5\n\
	mtpr	r5,r4\n" },

	{ "0,_setintqueue\n",
"	mtpr	$0x7,$0x14\n" },

	{ "0,_setsoftclock\n",
"	mtpr	$0x8,$0x14\n" },

	{ "1,_resume\n",
"	movl	(sp)+,r5\n\
	ashl	$9,r5,r0\n\
	movpsl	-(sp)\n\
	jsb	_Resume\n" },

	{ "3,_strncmp\n",
"	movl	(sp)+,r1\n\
	movl	(sp)+,r3\n\
	movl	(sp)+,r5\n\
	cmpc3	r5,(r1),(r3)\n" },

	{ "3,_copyin\n",
"	movl	(sp)+,r1\n\
	movl	(sp)+,r3\n\
	movl	(sp)+,r5\n\
	jsb	_Copyin\n" },

	{ "3,_copyout\n",
"	movl	(sp)+,r1\n\
	movl	(sp)+,r3\n\
	movl	(sp)+,r5\n\
	jsb	_Copyout\n" },

	{ "1,_smp_lock_once\n",
"	movl	(sp)+,r0\n\
	jsb	_Smp_lock_once\n" },

	{ "1,_smp_lock_retry\n",
"	movl	(sp)+,r0\n\
	jsb	_Smp_lock_retry\n" },

	{ "1,_smp_unlock_short\n",
"	movl	(sp)+,r0\n\
	jsb	_Smp_unlock_short\n" },

	{ "1,_fubyte\n",
"	movl	(sp)+,r0\n\
	jsb	_Fubyte\n" },

	{ "1,_fuibyte\n",
"	movl	(sp)+,r0\n\
	jsb	_Fubyte\n" },

	{ "1,_fuword\n",
"	movl	(sp)+,r0\n\
	jsb	_Fuword\n" },

	{ "1,_fuiword\n",
"	movl	(sp)+,r0\n\
	jsb	_Fuword\n" },

	{ "2,_subyte\n",
"	movl	(sp)+,r0\n\
	movl	(sp)+,r1\n\
	jsb	_Subyte\n" },

	{ "2,_suibyte\n",
"	movl	(sp)+,r0\n\
	movl	(sp)+,r1\n\
	jsb	_Subyte\n" },

	{ "2,_suword\n",
"	movl	(sp)+,r0\n\
	movl	(sp)+,r1\n\
	jsb	_Suword\n" },

	{ "2,_suiword\n",
"	movl	(sp)+,r0\n\
	movl	(sp)+,r1\n\
	jsb	_Suword\n" },

	{ "0,_movpsl\n",
"	movpsl r0" },

	{ "0,_diskcheck\n",
"	clrl r0\n\
	brb	2f\n\
	1:\n\
	incl _dk_time[r0]\n\
	incl	r0\n\
	2:\n\
	subl3	r0,$32,r1\n\
	ffs	r0,r1,_dk_busy,r0\n\
	bneq	1b\n"},

	{ "0,_mov_return_pc\n",
"	movl 16(fp),r0" },


/*
 * move to/from vector processor registers:
 *	mfvcr/mtvcr	VP reg VCR (Vector Count Register)
 *	mfvlr/mtvlr	VP reg VLR (Vector Length Register)
 *	mfvmrlo/mtvmrlo	VP reg VMRLO (bits <0..31> of Vector Mask Register)
 *	mfvmrhi/mtvmrhi	VP reg VMRHI (bits <32..63> of Vector Mask Register)
 *	vsty		store all 16 vector registers into memory
 *	vldq		load all 16 vector registers from memory
 */

	{ "0,_mfvcr\n",
"	mfvcr	r0\n" },

	{ "1,_mtvcr\n",
"	movl	(sp)+,r5\n\
	mtvcr	r5\n" },

	{ "0,_mfvlr\n",
"	mfvlr	r0\n" },

	{ "1,_mtvlr\n",
"	movl	(sp)+,r5\n\
	mtvlr	r5\n" },

	{ "0,_mfvmrlo\n",
"	mfvmrlo	r0\n" },

	{ "1,_mtvmrlo\n",
"	movl	(sp)+,r5\n\
	mtvmrlo	r5\n" },

	{ "0,_mfvmrhi\n",
"	mfvmrhi	r0\n" },

	{ "1,_mtvmrhi\n",
"	movl	(sp)+,r5\n\
	mtvmrhi	r5\n" },

	{ "1,_vstq\n",
"	movl	(sp)+,r5\n\
	vstq	v0,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v1,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v2,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v3,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v4,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v5,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v6,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v7,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v8,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v9,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v10,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v11,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v12,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v13,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v14,(r5),$8\n\
	addl2	$0x200,r5\n\
	vstq	v15,(r5),$8\n" },

	{ "1,_vldq\n",
"	movl	(sp)+,r5\n\
	vldq	(r5),$8,v0\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v1\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v2\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v3\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v4\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v5\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v6\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v7\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v8\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v9\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v10\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v11\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v12\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v13\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v14\n\
	addl2	$0x200,r5\n\
	vldq	(r5),$8,v15\n" },



/*
 * vector processor related sync instructions
 *	sync	scalar/vector instruction SYNChronization
 *	msync	Memory instruction SYNChronization
 *	vsync	memory SYNChronization within the Vector processor
 */

	{ "0,_sync\n",
"	sync\n" },

	{ "0,_msync\n",
"	msync\n" },

	{ "0,_vsync\n",
"	vsync\n" },



/* adawi(val,base)
 * 	
 *	This routine uses interlocks ONLY if smp
 *	is not equal to zero.
 *
 *	val: value to be added to the shortword addressed by base
 *	base: address of a shortword
 */
	{ "2,_adawi\n",
"		movl	(sp)+,r1\n\
		movl	(sp)+,r2\n\
		adawi	r1,(r2)\n\
	\n"},



/* set_bit_atomic(bit,base)
 * 	
 *	This routine uses interlocks ONLY if smp
 *	is not equal to zero.
 *
 *	bit: bit to be set interlocked (0 <= bit <=31)
 *	base: address of a longword
 *	return values:
 *		1 : if the bit could be successfully set interlocked
 *		0 : if the bit was already set
 */
	{ "2,_set_bit_atomic\n",
"		movl	(sp)+,r1\n\
		movl	(sp)+,r2\n\
		clrl	r0\n\
		tstl	_smp\n\
		bneq	2f\n\
		bbss	r1,(r2),1f\n\
		incl	r0\n\
		brb	1f\n\
	2:\n\
		bbssi	r1,(r2),1f\n\
		incl	r0\n\
	1:\n"},
		
/* clear_bit_atomic(bit,base)
 * 	
 *	This routine uses interlocks ONLY if smp
 *	is not equal to zero.
 *
 *	bit: bit to be cleared interlocked (0 <= bit <= 31)
 *	base: address of a longword
 *	return values:
 *		1: if the bit was cleared interlocked
 *		0: if the bit was already clear
 */
	{ "2,_clear_bit_atomic\n",
"		movl	(sp)+,r1\n\
		movl	(sp)+,r2\n\
		clrl	r0\n\
		tstl	_smp\n\
		bneq	2f\n\
		bbcc	r1,(r2),1f\n\
		incl	r0\n\
		brb	1f\n\
2:\n\
		bbcci	r1,(r2),1f\n\
		incl	r0\n\
	1:\n"},


	{ "1,_setlock\n",
"	movl	(sp)+,r0\n\
	tstl	_smp\n\
	bneq	3f\n\
	bbss	$31,(r0),1f\n\
	brb	2f\n\
1:\n\
	clrl	r0\n\
	brb	2f\n\
3:\n\
	bbssi	$31,(r0),1b\n\
2:\n"},

	{ "1,_clearlock\n",
"	movl	(sp)+,r0\n\
	tstl	_smp\n\
	bneq	3f\n\
	bbcc	$31,(r0),1f\n\
	brb	1f\n\
3:\n\
	bbcci	$31,(r0),1f\n\
1:\n" },

	{ "0,_getisp\n",
"	mfpr	$4,r0\n" },

	{ "0,_getesp\n",
"	mfpr	$1,r0\n" },

	{ "1,_setrq\n",
"	movl	(sp)+,r0\n\
	jsb	_Setrq\n" },

	{ "0,_getpsl\n",
"	movpsl	r0\n" },

	{ "1,_remrq\n",
"	movl	(sp)+,r0\n\
	jsb	_Remrq\n" },

	{ "0,_swtch\n",
"	movpsl	-(sp)\n\
	jsb	_Swtch\n" },

	{ "0,_sig_parent_swtch\n",
"	movpsl	-(sp)\n\
	jsb	_Sig_parent_swtch\n" },

	{ "1,_setjmp\n",
"	movl	(sp)+,r1\n\
	clrl	r0\n\
	movl	fp,(r1)+\n\
	moval	1(pc),(r1)\n" },

	{ "1,_longjmp\n",
"	movl	(sp)+,r0\n\
	jsb	_Longjmp\n" },

	{ "0,_cpuident\n",
"	jsb	cpuident\n" },

	{ "1,_ffs\n",
"	movl	(sp)+,r1\n\
	ffs	$0,$32,r1,r0\n\
	bneq	1f\n\
	mnegl	$1,r0\n\
1:\n\
	incl	r0\n" },

	{ "1,_htons\n",
"	movl	(sp)+,r5\n\
	rotl	$8,r5,r0\n\
	rotl	$-8,r5,r1\n\
	movb	r1,r0\n\
	movzwl	r0,r0\n" },

	{ "1,_ntohs\n",
"	movl	(sp)+,r5\n\
	rotl	$8,r5,r0\n\
	rotl	$-8,r5,r1\n\
	movb	r1,r0\n\
	movzwl	r0,r0\n" },

	{ "1,_htonl\n",
"	movl	(sp)+,r5\n\
	rotl	$-8,r5,r0\n\
	insv	r0,$16,$8,r0\n\
	rotl	$8,r5,r1\n\
	movb	r1,r0\n" },

	{ "1,_ntohl\n",
"	movl	(sp)+,r5\n\
	rotl	$-8,r5,r0\n\
	insv	r0,$16,$8,r0\n\
	rotl	$8,r5,r1\n\
	movb	r1,r0\n" },

	{ "2,__insque\n",
"	movl	(sp)+,r4\n\
	movl	(sp)+,r5\n\
	insque	(r4),(r5)\n" },

	{ "1,__remque\n",
"	movl	(sp)+,r5\n\
	remque	(r5),r0\n" },

	{ "2,__queue\n",
"	movl	(sp)+,r0\n\
	movl	(sp)+,r1\n\
	insque	(r1),*4(r0)\n" },

	{ "1,__dequeue\n",
"	movl	(sp)+,r0\n\
	remque	*(r0),r0\n" },

	{ "2,_imin\n",
"	movl	(sp)+,r0\n\
	movl	(sp)+,r5\n\
	cmpl	r0,r5\n\
	bleq	1f\n\
	movl	r5,r0\n\
1:\n" },

	{ "2,_imax\n",
"	movl	(sp)+,r0\n\
	movl	(sp)+,r5\n\
	cmpl	r0,r5\n\
	bgeq	1f\n\
	movl	r5,r0\n\
1:\n" },

	{ "2,_min\n",
"	movl	(sp)+,r0\n\
	movl	(sp)+,r5\n\
	cmpl	r0,r5\n\
	blequ	1f\n\
	movl	r5,r0\n\
1:\n" },

	{ "2,_max\n",
"	movl	(sp)+,r0\n\
	movl	(sp)+,r5\n\
	cmpl	r0,r5\n\
	bgequ	1f\n\
	movl	r5,r0\n\
1:\n" },

	{ "1,_xsetjmp\n",
"	movl	(sp)+,r1\n\
	clrl	r0\n\
	movl	r2,(r1)+\n\
	movl	r3,(r1)+\n\
	movl	r4,(r1)+\n\
	movl	r5,(r1)+\n\
	movl	r6,(r1)+\n\
	movl	r7,(r1)+\n\
	movl	r8,(r1)+\n\
	movl	r9,(r1)+\n\
	movl	r10,(r1)+\n\
	movl	r11,(r1)+\n\
	movl	ap,(r1)+\n\
	movl	fp,(r1)+\n\
	movl	sp,(r1)+\n\
	moval	1(pc),(r1)\n" },

	{ "1,_xlongjmp\n",
"	movl	(sp)+,r0\n\
	movl	(r0)+,r2\n\
	movl	(r0)+,r3\n\
	movl	(r0)+,r4\n\
	movl	(r0)+,r5\n\
	movl	(r0)+,r6\n\
	movl	(r0)+,r7\n\
	movl	(r0)+,r8\n\
	movl	(r0)+,r9\n\
	movl	(r0)+,r10\n\
	movl	(r0)+,r11\n\
	movl	(r0)+,ap\n\
	movl	(r0)+,fp\n\
	movl	(r0)+,sp\n\
	jmp	*(r0)\n" },

	{ "1,_dna_inc64\n",
"	movl	(sp)+,r0\n\
	incl	(r0)+\n\
	adwc	$0,(r0)\n" },

	{ "2,_dna_add64\n",
"	movl	(sp)+,r1\n\
	movl	(sp)+,r0\n\
	addl2	r1,(r0)+\n\
	adwc	$0,(r0)\n" },

/*
 * unixtovms(srcaddr,dstaddr)
 *
 * src address
 * dst address
 * constants - $0x989680   convert sec. to 100 nanosec. units.
 *           - $0x4beb4000 difference between 00:00 Jan. 1 1970
 *           - $0x7c9567   and 00:00 Nov. 17 1858 to form the VMS time.
 */
	{"2,_unixtovms\n",
"	movl	(sp)+,r4\n\
 	movl	(sp)+,r5\n\
 	emul	(r4),$0x989680,4(r4),0(r5)\n\
	addl2	$0x4beb4000,(r5)\n\
	adwc	$0x007c9567,4(r5)\n" },

	{ "3,__km_alloc\n",
"	jsb	__km_alloc\n"},

	{ "2,__km_free\n",
"	jsb	__km_free\n"},

	{ "1,__km_memdup\n",
"	jsb	__km_memdup\n"},

	{ "", "" }
};
