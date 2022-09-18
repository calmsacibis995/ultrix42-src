/*
 * common.c
 */
/*
 *	@(#)common.c	4.1	(ULTRIX)        7/2/90
 */
/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
#define MVAX
#include "../../machine/vax/mtpr.h"
#define LOCORE
#include "../../machine/vax/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "vmb.h"

/*
 * Functional Discription:
 *	Grabs the cpu type.
 *	Builds an SCB with which to trap serious exceptions.
 *	Builds it on the next page aligned page after the _end of
 *	the image relocated.
 *
 * Inputs:
 *	none
 *
 * Outputs:
 *	none
 *
 * Modification History:
 *
 * 06-Jun-90 -- rafiey (Ali Rafieymehr)
 *	For VAXes, flush "rei" has to be performed for instruction streams.
 *	We have not done them before and we have been fortunate. VAX9000 
 *	wouldn't boot without fixing the problem.
 *
 * 20-Apr-87 -- afd
 *	Removed work-around for bugs in the P1 CVAX chips.
 *
 * 09-Mar-87 -- afd
 *	Added CVAX to CPUs for which we must get the subtype.
 *	Added CVAX to case statement in "badloc" routine.
 */
#define SYSTYPREG 0x20040004
	.text
        .globl  _setup
_setup:
        .word   2                       # save R1
        mfpr    $SID,_cpu              	# get the system id register
        ashl    $-24,_cpu,_cpu          # move the cpu type down and 
                                        # throw the rest away
	cmpb	$MVAX_II,_cpu		# Is this a MVAX II chip
	bneq	1f			#   no, so see if its a CVAX chip
	brb	2f			#   yes, so go get cpu subtype
1:
	cmpl	$CVAX_CPU,_cpu		# is cpu a CVAX chip?
	bneq	3f			#   no, so skip cpu subtype
					#   yes, fall into subtype code
2:
	movl	*$SYSTYPREG,_cpuext	# get the System type register
        ashl    $-24,_cpuext,_cpuext    # move the system type down and 
                                        # throw the rest away
3:
        movl    $_end,r0                # calculate the address of the SCB
        addl2   $0x200,r0               # bump up to a page boundary
        bicl2   $0x1ff,r0               # align it
4:      
        /*
         * Build an SCB to trap exceptions
         */
        mtpr    r0,$SCBB                # plug it in
#ifdef SECONDARY
	clrl	r1	
1:
        addl3    $_scb_catcher+1,r1,(r0)+ # point each vector loc to handler
                                        # +1 to handle on the interrupt stack
	addl2	$8,r1			#
	cmpw	$1024,r1
	bneq	1b
	mfpr	$SCBB,r0
	movl	$vax$emulate,0xc8(r0)
#else SECONDARY
	movl	$128,r1
1:
	movl	$_scb_catcher+1,(r0)+
	sobgtr	r1,1b
#endif SECONDARY
        ret

/*
 * Functional Discription:
 *	This routine relocates the running image's text and data segments
 *	and clears the bss segment.
 *
 * Inputs:
 *	address at which we want to restart execution (relocated)
 * 	NOTE: We JMP back to the specifed restart address after relocating
 *
 * Outputs:
 *	none
 *
 */
        .text
        .globl  _reloc
_reloc:
        .word   0                       # don't care
        movl    4(ap),restradr          # save the restart address
        movl    $_entry,sp              # set stack to start of relocated code
        pushr   $0xfff                  # save GPR's
        movl    $_entry,r2              # get the relocation address again
        /*
         * Move to higher memory locations to make room for
         * for programs to be loaded at memory location 0.
         */
        movab   _edata,r0               # Compute the size
        movab   _entry,r1               # Start of program 
        subl2   r1,r0                   # figure out text+data size
        movc3   r0,_entry,(r2)          # Move it up
        /*
         * Clear relocated bss segment separately from text and data
         * since movc3 can't move more than 64K bytes
         */
1:                                      # r3 remembers where we left off
        clrl    (r3)+                   # clear a longword
        cmpl    r3,$_end                # are we at the end?
        jlss    1b                      # if not, do more
        popr    $0xfff                  # restore GPR's
	calls	$0,_flush_istream	# flush the cache
        jmp     *restradr		# continue at the restart address

        .data
restradr:
        .long 0                         # store the restart address here

/*
 * We have to flush for instruction stream for VAXes.  Not doing it has not created problem
 * for us until now (VAX9000) so, lets fix it
 */
	.text
	.align	2
	.globl	_flush_istream
_flush_istream:
	.word 0
	movpsl	-(sp)			# save the psl
	movab	1f,-(sp)		# save the return address
	rei				# pop
1:
	ret

/*
 * Functional Discription:
 *	A place to catch unexpected interrupts/execeptions.  If we can
 *	print a message we do.  I any case we simply halt.  The routine
 *	reports the SCB vector address to help identify the exception.
 *
 * Inputs:
 *	none
 *
 * Outputs:
 *	none
 *
 */
	.text
	.align	2
	.globl	_scb_catcher
_scb_catcher:
#ifndef SECONDARY
	halt
#else SECONDARY
#define PJ pushr $0x3f;jsb Xstray
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
	PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ;PJ
#undef PJ
Xstray:
	/*
	 * Calculate and report the vector location of the stray
	 * scb interrupt
	 */
	subl3	$_scb_catcher+8,(sp)+,r0
	ashl	$-1,r0,-(sp)
        pushal  exception               # push the address of the text
        calls   $2,_printf              # print it
        halt                            # no recovery

        .data
exception:
        .asciz  "Unexpected exception at SCB vector 0x%x\n"
#endif SECONDARY


/*
 * Functional Discription:
 *	stop is called from C routines to halt execution after 
 *	unrecoverable errors.
 *
 * Inputs:
 *	none
 *
 * Outputs:
 *	none
 *
 */
        .text
        .globl  _stop
_stop:
        .word   0x0
        halt                            # anything fatal dies here


/*
 * Functional Discription:
 *	This routine is called to initialize the selected boot device.
 *	If no arg is passed, the current drive is initialized.
 *
 * Inputs:
 *	OPTIONAL:
 *	4(ap) = new unit number
 *
 * Outputs:
 *	status returned from the init routine
 *		or
 *	1 for devices which do not have an init routine
 *
 */
	.text
        .globl  _drvinit
_drvinit:
        .word   0x3c0                   # save R6 - R9
        movl    _vmbinfo,r8     	# get the info structure
        movl    INFO_RPBBAS(r8),r9      # required by init routines
        tstb    (ap)                    # any args passed (new unit)
        beql    1f                      # no, skip ahead
        movw    4(ap),RPB$W_UNIT(r9)    # plug in a new unit number
        movzwl  4(ap),RPB$L_BOOTR3(r9)  # plug in a new unit number
1:
        movl    RPB$L_IOVEC(r9),r7      # move RPB$L_IOVEC to r0
        addl3   r7,BQO$L_QIO(r7),_qioentry # Compute address of QIO routine
        movzbl  $1,r0                   # assume success
        tstl    BQO$L_UNIT_INIT(r7)     # is there an init routine?
        beql    4f                      # if not go on
        addl3   r7,BQO$L_UNIT_INIT(r7),r6 # figure its entry point
/*
 * KLUDGE (for [T]MSCP controller types) to force total controller 
 * initialization, otherwise we wait an awful long time for 
 * initialization to complete.
 */
#define IP      0
#define SA      2
        tstb    kludge                  # have we done this once?
        bneq    2f                      # if so, skip it to save time
        cmpb    $BTD$K_UDA,RPB$B_DEVTYP(r9) # is this a uda/qda/kda?
        beql    1f                      # yes, go hit it
        cmpb    $BTD$K_TK50,RPB$B_DEVTYP(r9) # is this a TK50 controller?
        beql    1f                      # yes, go hit it
        brb     2f                      # proceed with normal init
1:
        movl    RPB$L_CSRPHY(r9),r7     # get the IP register address
        clrw    IP(r7)                  # poke it to make it step
1:
        movw    SA(r7),r0               # get the status registe
        bitw    $0xfe00,SA(r7)          # is something active?
        beql    1b                      # if not, wait until there is
        incb    kludge                  # mark it done
/*
 * END OF KLUDGE
 */
2:
        mfpr    $SCBB,r7                # required by BDA driver
        callg   *INFO_VMBARGBAS(r8),(r6) # do it
4:
        ret                             # r0 = status

        .data
kludge:
        .long 0                         # mark the fact that funny init is done

/*
 * Functional Discription:
 *	This routine is called to disconnect (shutdown) the selected 
 *	boot device.  Here, it is used to support network boot.
 *
 * Inputs:
 *	none
 *
 * Outputs:
 *	status returned from the disconnect routine
 *		or
 *	1 for devices which do not have a disconnect routine
 *
 */
	.text
        .globl  _disconnect
_disconnect:
        .word   0x3c0                   # save R6 - R9
        movl    _vmbinfo,r8     	# get the info structure
        movl    INFO_RPBBAS(r8),r9      # required by disconnect 
					# routines
        movl    RPB$L_IOVEC(r9),r7      # move RPB$L_IOVEC to r7
        movzbl  $1,r0                   # assume success
        tstl    BQO$L_UNIT_DISC(r7)     # is there an init routine?
        beql    1f                      # if not go on
        addl3   r7,BQO$L_UNIT_INIT(r7),r6 # figure its entry point
        callg   *INFO_VMBARGBAS(r8),(r6) # do it
1:
        ret                             # r0 = status

#ifdef SECONDARY

/*
 * Functional Discription:
 *	This routine is used to find video consoles.  It can catch 
 *	machine checks when sizing of these devices.
 *	SET UP TO HANDLE MVAX I, MVAX II, AND CVAX ONLY.
 *
 * Inputs:
 *	4(ap) = address to be probed
 *	8(ap) = size to probe (byte, word, longword)
 *
 * Outputs:
 *	0 = address no response
 *	1 = address responded
 *
 */
        .text
        .globl  _badloc
_badloc:
        .word   0x1c                    # save r2, r3, r4
        movl    $1,r0                   # assume success
        movl    4(ap),r3                # get the address to be probed
        mfpr    $SCBB,r2                # get the SCBB
        moval   4(r2),r2                # get the address of the mchkvec
        movl    (r2),r4                 # save it contents for later
        movab   9f,(r2)                 # shove in the trap address
        bbc     $0,8(ap),1f; tstb (r3)  # probe a bytes worth
1:      bbc     $1,8(ap),1f; tstw (r3)  # probe a words worth
1:      bbc     $2,8(ap),1f; tstl (r3)  # probe a longs worth
1:      clrl    r0                      # made it w/o machine checks
8:      movl    r4,(r2)                 # restore good mchkvec
        ret                             # all done
        .align  2
9:
        casel   _cpu,$1,$CPU_MAX        # check cpu type to clean up
0:
        .word   1f-0b                   # 1 is 780
        .word   1f-0b                   # 2 is 750
        .word   1f-0b                   # 3 is 730
        .word   1f-0b                   # 4 is 8600
        .word   1f-0b                   # 5 is 8200 
        .word   1f-0b                   # 6 is undefined
        .word   2f-0b                   # 7 is MicroVAX I
        .word   3f-0b                   # 8 (MicroVAX chip)
        .word   1f-0b                   # 9 is undefined
        .word   3f-0b                   # 10 (CVAX chip)
1:
        halt
2:                                      # MicroVAX I
        mtpr    $0xf,$MCESR
3:                                      # MicroVAX II & CVAX
        addl2   (sp)+,sp                # discard mchchk frame
        movab   8b,(sp)                 # setup point at which to continue
        rei                             # return from the mchk exception


/*
 * Functional Discription:
 *	provides mtpr() in C modules
 *
 * Inputs:
 *	4(ap) = value
 *	8(ap) = processor register
 *
 * Outputs:
 *	none
 *
 */
	 .text
        .globl  _mtpr
_mtpr:
        .word   0                       # entry mask
        mtpr    8(ap),4(ap)             # do the mtpr
        ret                             # return

/*
 * Functional Discription:
 *	provides mfpr() in C modules
 *
 * Inputs:
 *	4(ap) = processor register
 *
 * Outputs:
 *	none
 *
 */
	.text
        .globl  _mfpr
_mfpr:
        .word   0                       # entry mask
        mfpr    4(ap),r0                # do the mfpr
        ret                             # return with r0 = reg contents

#endif SECONDARY

        .data
        .globl  _mode
_mode:                                  # hold boot mode
        .long 0
        .globl  _vmbinfo
_vmbinfo:                               # hold address of vmb info list
        .long 0
        .globl  _qioentry
_qioentry:                              # hold address of qio entry in bootdrvr
        .long 0
        .globl  _cpu
_cpu:                                   # hold CPU type
        .long 0
        .globl  _cpuext
_cpuext:                                # hold System type
        .long 0
