/*
 * ultrixstart.c
 */

/*
 * @(#)ultrixstart.c	4.1	(ULTRIX)	7/2/90
 */
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1985 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   This software is  derived  from  software  received  from  the     *
 *   University    of   California,   Berkeley,   and   from   Bell     *
 *   Laboratories.  Use, duplication, or disclosure is  subject  to     *
 *   restrictions  under  license  agreements  with  University  of     *
 *   California and with AT&T.                                          *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
#define MVAX
#include "../../machine/vax/mtpr.h"
#define LOCORE
#include "vmb.h"
        .globl  _entry
        .globl  _edata
        .globl  _end
/*
 * Functional Discription:
 *	This is the startup code for the ultrixboot image and is 
 *	called from the vaxboot image loaded from blocks 1-15
 *
 * Inputs:
 *      4(ap) = Physical address of vmb information list to be passed on
 *              to the kernel.
 *
 * Outputs:
 *	none
 *
 */
_entry:
        .word   0                       # entry mask
        mtpr    $HIGH,$IPL              # just in case
        movl    4(ap),_vmbinfo          # save addr of the vmb info structure
        calls   $0,_setup               # go do some initial setup
        movl    _vmbinfo,r11            # point to vmb info again
        movl    INFO_RPBBAS(r11),r11    # point the the RPB
        /*
         * compute the address of the boot driver's qio routine
         */
        movl    RPB$L_IOVEC(r11),r7     # Move RPB$L_IOVEC to r7
        addl3   r7,BQO$L_QIO(r7),_qioentry # Compute address of Bootdriver
        /*
         * pass the boot flags
         */
        movl    RPB$L_BOOTR5(r11),r11   # restore software boot flags
        /*
         * relocate to higher memory
         */
        pushl   $continue               # push the restart address
        calls   $1,_reloc               # go relocate to higher memory
continue:                               # come back here
        calls   $0,_main                # startup is done, call the
                                        # main routine
        halt                            # We should never return

/*
 * Functional Discription:
 *	This routine is called to startup the diagnostic supervisor.
 *	It assumes the DS has been loaded at address 0xfe00 and that it
 *	has a starting address of 0x10000.  As required by the DS, r0 is
 *	set to the SID register and R11 is set to the address of the RPB.
 *
 * Inputs:
 *	none
 *
 * Outputs:
 *	none
 *
 */
	.text
        .globl  _start_ds
_start_ds:
        .word   0                       # don't care at this point
        mfpr    $SID,r0                 # Put the SID in r0
        movl    _vmbinfo,r11            # Get the vmb info struct addr
        movl    INFO_RPBBAS(r11),r11    # Pass the address of RPB in R11
        jmp     *$0x10000               # Startup the DS
        halt                            # can't get here

