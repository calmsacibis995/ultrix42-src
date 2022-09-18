/*
 * vaxstart.c
 */

/*
 * @(#)vaxstart.c	4.2	(ULTRIX)	1/31/91
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
 /*
  * Revision History:
  * 
  * 31-Jan-91 jas
  * 	VAX6000 wouldn't install from tk50 if 512Mb of memory present.
  *	To fix, back of pointer to 511.5Mb if more than 511.5Mb present
  *	(if more, then 512Mb.)
  */
#include "../../machine/vax/mtpr.h"
#define LOCORE
#include "vmb.h"
        .globl  _end
        .globl  _edata
        .globl  _entry
/*
 * Functional Discription:
 *	This is the entry point for the boot and loader paths.  It is
 *	called is called from the bootblock, except for a network boot.
 *
 * STANDARD BOOT PATH,
 *	
 *	The bootblock calls _entry with:
 *
 * Inputs:
 *       4(ap) = mode of boot, VMB_BOOT, ROM_BOOT, TK50_BOOT
 *
 *    The next two args only if VMB mode boot:
 *       8(ap) = Address of VMB's RPB
 *      12(ap) = Address of VMB's argument list
 *
 * NETWORK BOOT PATH, 
 *
 *	VMB jumps directly to _entry with:
 *
 * Inputs:
 *	ap = VMB argument list
 *	R11 = RPB
 *
 *	The inputs are pushed out onto the stack with _mode
 *	and a call frame built before preoceding normally with 
 *	a VMB style boot.  This replaces the normal function 
 *	of the bootblock.
 *
 * Outputs:
 *	none
 *
 */
_entry:
	brw	net_entry		# entry mask for normal boot path
	halt				# filler
	/*
	 * entry point when called from a bootblock
	 * MUST BE POSITIONED 4 BYTES FROM _entry
	 */
entry_bblk:	
	.word	0			# entry mask
        mtpr    $HIGH,$IPL              # just in case
        movl    4(ap),_mode             # save the boot mode
        bitl    $ROM_BOOT,_mode         # were we booted via system ROMs
        bneq    rom_mode		# if so, proceed with ROM mode boot
        brw     vmb_mode		# handle a VMB mode boot

net_entry:
        mtpr    $HIGH,$IPL              # just in case
	/*
	 * The following few instructions emulate the function normally
	 * done by a boot block.  They place the information on the
	 * stack required to make the rest of the startup code operate
	 * consistently.  A stack frame is established as if this
	 * routine were calls'ed.  When booting the network, VMB jumps
	 * in to this module, where as it is normally called from the
	 * bootblock code.
	 */
	pushl	ap			# Push the VMBARG list out
	pushl	r11			# Push the RPB address
	xorl2	$VMB_BOOT,_mode		# Booting using VMB
	xorl2	$NET_BOOT,_mode		# Booting via the network
	pushl	_mode			# Push the mode out, too
	pushl	$3			# set up the arg count
	movl	sp,ap			# plug in the arg count
	/* 
	 * Proceed normally now.
	 */
        movl    4(ap),_mode             # save the boot mode
        brw     vmb_mode		# handle a VMB mode boot

/*
 * Branch here when booted via 750/8200 style boot ROMs.  Save the contents
 * of the GPR's as given to the bootblock.  They will eventually be
 * passed on to VMB.EXE.
 */
rom_mode:
        /*
         * Save original ROM input parameters
         */
	movl	8(ap),_SAVE_fp		# pushed on by bootblk.  This
					# is need by calypso rom.
        movl    r0,_ROM_r0              # Save R0 for VMB
        movl    r1,_ROM_r1              # Save R1 for VMB
        movl    r2,_ROM_r2              # Save R2 for VMB
        movl    r3,_ROM_r3              # Save R3 for VMB
        movl    r4,_ROM_r4              # Save R4 for VMB
        movl    r5,_ROM_r5              # Save R5 for VMB
        movl    r6,_ROM_r6              # Save R6 for VMB
        movl    r7,_ROM_r7              # Save R7 for VMB.  This contains
					# the CCA address for calypso.

        movl    sp,_ROM_sp              # Save SP for VMB
        addl2   $8,_ROM_sp              # Adjust to original for VMB
        /*
         * Because of a bug in the 750 ROM UDA driver. It is necessary
         * to point to a page aligned 512 byte buffer.  The driver can'T
         * do unaligned transfers.  The ROM driver also cannot deal with
	 * relocaion above 256K.  This buffer is alway the transfer
         * address for all I/O by ROM drivers.  Data is then copied back 
	 * to the desired and presumeably unaligned buffer.  The buffer 
	 * is set at location 0 which will be safe for loading VMB a
	 * block at a time.
         */
        clrl    _ROM_buffer             # set ROM read buffer address
        calls   $0,_setup               # now, go do some setup
        /*
         * Go relocate to higher memory
         */
        pushl   $rom_continue           # push the restart address
        calls   $1,_reloc               # relocate to higher memory
rom_continue:
        calls   $0,_main                # call the main program


/*
 * Branch here when booted via VMB.EXE.  Much initialization occurs here.
 * Information is save at the top of memory on an information stack.
 * A vmbinfo list is build which is passed through to main and
 * eventually to the kernel.
 */
vmb_mode:
        calls   $0,_setup               # first, go do some setup
        movl    8(ap),r11               # get the address of the RPB
        clrb    RPB$T_FILE(r11)         # clear the file name byte count
                                        # so the DIAG SUPER knows it's Ultrix
	moval	tmpinfo,r8		# initialize pointer
        /*
         * Size memory
         */
        movl    12(ap),r9		# get VMB's arg list
        cmpw    $VMB$Q_PFNMAP/4,VMB$L_ARGBYTCNT(r9)
                                        # is the extended map arg present?
        bgtr    1f                      # Branch if not
        movq    VMB$Q_PFNMAP(r9),RPB$Q_PFNMAP(r11)
                                        # otherwise, update the RPB
1:
        movl    RPB$Q_PFNMAP+4(r11),r6  # get the address of the PFNMAP
        movl    RPB$Q_PFNMAP(r11),r7    # get its size 
        clrl    r9                      # init good page counter
3:      cmpb    (r6)+,$0xff             # is it 8 good pages ?
        bneq    1f                      # if not we're done
        addl2   $8,r9                   # 8 more good pages
        sobgtr  r7,3b                   # is this the end?
1:      
        ashl    $9,r9,INFO_MEMSIZ(r8)	# convert from pages and save
#define MEMSIZEADDR     0xf0000         /* Backward compatability on MVAX */
#define MAXMEMSIZE	1024*1024*512
#define BISPACE		(1024*1024)/2
        ashl    $9,r9,*$MEMSIZEADDR     # convert from pages

	/* 
	 * check if >511.5Mb of memory present.  If yes, back off to 511.5Mb to
	 * prevent trashing of common data. 
	 */
	cmpl	INFO_MEMSIZ(r8),$MAXMEMSIZE-BISPACE
	blssu	1f			# is mem size less than or eq to 511.5?
	
	movl	$MAXMEMSIZE-BISPACE,INFO_MEMSIZ(r8)	# reload total memory
	movl	$MAXMEMSIZE-BISPACE,*$MEMSIZEADDR	# ditto.

        /*
         * set the info stack pointer (isp) to the top of memory.
         */
1:
        movl    INFO_MEMSIZ(r8),isp 	# set isp to the top of memory
        /*
         * save the BOOT DRIVER
         */
        pushl   RPB$L_IOVEC(r11)        # push the address of the boot driver
        pushl   RPB$L_IOVECSZ(r11)      # push its size
        calls   $2,move_info            # move it
        movl    r0,INFO_BTDRBAS(r8)	# save its address
        movl    RPB$L_IOVECSZ(r11),INFO_BTDRSIZ(r8)
                                        # save its size
        /*
         * save the RPB
         */
        pushl   r11                     # push the address of the RPB
        pushl   $RPBSIZ                 # push its size
        calls   $2,move_info            # move it
        movl    r0,INFO_RPBBAS(r8)	# save its address
        movl    $RPBSIZ,INFO_RPBSIZ(r8)	# save its size
        movl    INFO_RPBBAS(r8),r11	# point to the moved RPB
        movl    INFO_BTDRBAS(r8),RPB$L_IOVEC(r11)
                                        # reset new RPB$L_IOVEC
        /*
         * Save the Arg list
         */
        movl    12(ap),r9              	# get the address passed
        mull3   $4,(r9),r10             # make arg count a bytecount
        addl3   $4,r10,r10              # Add 4 to include arg count itself
        pushl   r9                      # push address of VMB ARGLIST
        pushl   r10                     # push its size
        calls   $2,move_info            # move it
        movl    r0,INFO_VMBARGBAS(r8)	# save its address
	movl	r0,r9			# update r9 as a pointer to the
					# new location
        movl    r10,INFO_VMBARGSIZ(r8)	# save its size

	/*
	 * save VMB's version number
	 */
        movl    RPB$L_IOVEC(r11),r10    # point to the boot driver
	movzwl	BQO$W_VERSION(r10),INFO_VMBVERS(r8)
					# save VMB's version number
        /*
         * When booting a CI, a system page table is built after the 
         * end of the ucode, so space must be reserved.
         */
        cmpb    $BTD$K_HSCCI,RPB$B_DEVTYP(r11) # are we booting a CI/HSC?
        beql    1f                      # if so, allow for SPT
        cmpb    $BTD$K_BVPSSP,RPB$B_DEVTYP(r11) # are we booting an AIO?
        beql    1f                      # if so, allow for SPT
        cmpb    $BTD$K_AIE_TK50,RPB$B_DEVTYP(r11) # are we booting an AIE/TK50?
        beql    1f                      # if so, allow for SPT
	brb	2f			# if neither, then skip ahead
1:

        mull3   $4,VMB$L_HI_PFN(r9),r0 # How many bytes
        subl2   r0,isp                  # adjust the info stack pointer
	movl	isp,VMB$L_BVP_PGTBL(r9)	# BVP init routines look here
2:
        /*
         * Save the ucode - check the VMBARG List
         */
        tstl    VMB$Q_UCODE+4(r9)       # is there any ucode?
        beql    3f                      # if not, skip ahead
        pushl   VMB$Q_UCODE+4(r9)       # push its address
        pushl   VMB$Q_UCODE(r9)         # push its size
        calls   $2,move_info            # move it
        movl    r0,INFO_CIUCODBAS(r8)	# save its address
        movl    VMB$Q_UCODE(r9),INFO_CIUCODSIZ(r8)
                                        # save its size
        movl    r0,BQO$L_UCODE(r10)     # update the driver ucode pointer
        movl    INFO_VMBARGBAS(r8),r9	# get the new address of the arg list
        movl    r0,VMB$Q_UCODE+4(r9)    # point the vmbarglst at the ucode
3:
        /*
         * The vmb information list is built, now move it in front of
         * all the stuff saved.
         */
        clrl    -(sp)                   # request 1k align (by passing 3 args)
        pushal  tmpinfo                 # push the address
        pushl   $INFO_SIZE              # push its size
        calls   $3,move_info            # move it
        movl    r0,_vmbinfo             # save address in the global location 
        pushl   $vmb_continue           # push the restart address
        calls   $1,_reloc               # relocate to higher memory
vmb_continue:
        /*
         * Re-INIT the boot driver after moving it
         */
	cmpb	$BTD$K_KA640_NI,RPB$B_DEVTYP(r11) # is this is a VAXstar NI?
	bneq	1f			# if not, continue, otherwise
	calls   $0,_disconnect		# disconnect the boot driver
	blbc	r0,2f			# I would hope not.
1:
        calls   $0,_drvinit             # reinit the boot driver
        blbc    r0,2f                   # go halt on error
        calls   $0,_main                # startup is done, call the
                                        # main routine
2:
        halt                            # We should never return

        .data
tmpinfo:                                # used to build vmb info list
        .space INFO_SIZE    		# a tmp work area needted to
					# build the vmbinfo list before
					# saving it too.

/*
 * Functional Discription:
 *	This routine builds a stack of information to be passed 
 *	through to the kernel.  The stack is built at the top of
 *	the contiguous portion of main memory.
 *
 *      The final call to move_info must force alignment on a 1k 
 *      boundary for consistency with kernel cmap structuring.
 *      Intermediate calls need only force page alignment.
 *
 * Inputs:
 *	4(ap) = size to be saved
 *	8(ap) = starting address of info to be saved
 *	[12(ap)] optional, forces 1k boundary alignment
 *
 * Outputs:
 *	address where information was stored
 *
 */
        .text
move_info:
        .word   0x1e                    # save r1 - r5
        subl2   4(ap),isp               # move the info stack down as needed
        movzwl  $0x1ff,r0               # assume page alignment
        cmpb    $3,(ap)                 # third arg present, 1k align requested
        bneq    1f                      # if not, skip ahead
        movzwl  $0x3ff,r0               # 1k alignment requested
1:
        bicl2   r0,isp                  # align accordingly
        movc3   4(ap),*8(ap),*isp       # move information
        movl    isp,r0                  # address it was moved to
        ret                             # return

        .data
isp:                                    # information stack pointer
        .long 0

/*
 * Functional Discription:
 *	This routine is called when ROM_BOOT is active and after
 *	VMB.EXE has beed loaded.  It restores the registers at boot
 *	time.  Bit 3 and Bit 16 get set R5 to force bootblock mode boot by
 *	VMB and to tell VMB not to drop out pages with CRD's.
 *	Assumes good memory was found.
 *
 * Inputs:
 *	none
 *
 * Outputs:
 *	none
 *
 */
        .text
        .globl  _start_vmb
_start_vmb:
        .word   0                       # don't care at this point
        movl    _ROM_r0,r0              # restore R0 for VMB
        movl    _ROM_r1,r1              # restore R1 for VMB
        movl    _ROM_r2,r2              # restore R2 for VMB
        bicl2   $0xfffc0000,r2          # Reduce 32-bit CSR to 18-bit
                                        # CSR that VMB expects

        movl    _ROM_r3,r3              # restore R3 for VMB
        movl    _ROM_r7,r7              # needed for calypso. this
					# restore  the CCA addr into r7.

	clrl	r4			# force bootblock 0
        movl    _ROM_r5,r5              # restore R5 for VMB
        bisl2   $0x10008,r5             # set bootblock mode bit 3 for VMB
                                        # and don't discard CRD pages
        movl    $0x200,sp               # restore sp for VMB
        jmp     *$0x200                 # startup VMB
        halt                            # can't get here
