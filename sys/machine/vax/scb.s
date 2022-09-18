/*
 * @(#)scb.s	4.3  (ULTRIX)        9/6/90    
 */
/************************************************************************
 *									*
 *			Copyright (c) 1984 - 1989 by			*
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
/*
 * Modification History:
 *
 * 4-Sep-90	dlh
 *	changed scb vector 0x64 - vector disabled fault 
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added support for multiple XMIs for VAX9000.
 *
 * 11-May-89	Todd M. Katz		TMK0001
 *	Currently the SCB vector used by the MSI local port for its
 *	interrupts is always initialzed regardless of whether the port is
 *	present or not.  This is incorrect.  It should only be initialized
 *	when a MSI local port is present.  Change the default for this
 *	vector to scb_stray, the routine responsible for processing stray
 *	interrupts.
 *
 * 10-Jan-89 -- kong
 *	Added Rigel (VAX6400) to CPUs needing extra page for BI vectors.
 *
 * 07-Jun-88 -- darrell
 *	Added VAX60 (Firefox) to CPUs needing emulation code.
 *
 * 26-Apr-88    jaw
 *	Add VAX8820 support.
 *
 * 15-Feb-88 -- fred (Fred Canter)
 *	Added VAX420 (CVAXstar/PVAX) to CPUs needing emulation code.
 *
 * 25-Jan-88 -- rsp (Ricky Palmer)
 *	Added msiintr to base interrupt stack for MSI support.
 *	
 * 19-Jan-88 -- jaw
 * 	added changes for calypso to allocate BI vector pages and
 *	for emulation code.
 *
 * 20-Apr-87 -- afd
 *	Changed name CVAXQ to VAX3600 for Mayfair.
 *
 * 06-Mar-87 -- afd
 *	Added CVAXQ to ifdef on dummy definition of "vax$emulate" and
 *	"vax$emulate_fpd".  Only define dummy symbols if neither MVAX nor
 *	CVAXQ is defined.
 *
 * 18-Apr-86 -- jaw	hooks for nmi faults and fixes to bierrors.
 *
 * 17-Apr-86  -- jaw     re-write scbprot so it protects ALL of the scb.  This
 *			 also fixes the "lost" instack page bug.
 *
 * 15-Apr-86 -- afd
 *	Added dummy definition of "vax$emulate" and "vax$emulate_fpd"
 *	if MVAX is not defined.
 *
 * 15-Apr-86 -- jf   add support for system processes. Uses IPL10 interrupt
 *
 * 02-Apr-86 -- jaw  add support for nautilus console and memory adapter
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 16-Apr-85 -- lp
 * 	Added interrupt entry points for vax8200 serial lines.
 *
 * 12-Mar-85 -- darrell
 *	Handle sbi interrupts.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 */

#include "uba.h"
#include "vaxbi.h"

/*
 * System control block
 */
	.set	INTSTK,1	# handle this interrupt on the interrupt stack
	.set	HALT,3		# halt if this interrupt occurs

/*
 * All unused or unexpected vectors in the SCB point to it's relative
 * position in a dummy scb called scb_stray.  The dummy block contains
 * PUSHR and a jsb to Xstray in locore.s.  Xstray calculates the offset
 * in the block and reports it as the vector.
 */

_scb:	.globl	_scb

/*
 * offset in the STRAY def is multiplied by two because the set of
 * instructions used in scb_stray occupys two longwords of storage.
 */

#define STRAY(offset)	.long	_scb_stray+(offset*2)+INTSTK
#define KS(a)	.long	_X/**/a
#define IS(a)	.long	_X/**/a+INTSTK
#define STOP(a) .long	_X/**/a+HALT

/* 000 */	IS(passrel);	IS(machcheck);	IS(kspnotval);	STOP(powfail);
/* 010 */	KS(privinflt);	KS(xfcflt);	KS(resopflt);	KS(resadflt);
/* 020 */	KS(protflt);	KS(transflt);	KS(tracep);	KS(bptflt);
/* 030 */	KS(compatflt);	KS(arithtrap);	STRAY(0x38);	STRAY(0x3c);
/* 040 */	KS(syscall);	KS(chme);	KS(chms);	KS(chmu);
/* 050 */	STRAY(0x50);	IS(cmrd);	IS(sbi0alert);	IS(sbi0flt);
/* 060 */	IS(wtime);	IS(sbi0fail);	KS(vdisflt);	STRAY(0x6c);
/* 070 */	STRAY(0x70);	STRAY(0x74);	STRAY(0x78);	STRAY(0x7c);
/* 080 */	IS(ipintr);	STRAY(0x84);	KS(astflt);	STRAY(0x8c);
/* 090 */	STRAY(0x90);	STRAY(0x94);	STRAY(0x98);	IS(intqueue);
/* 0a0 */	IS(softclock);  STRAY(0xa4);	STRAY(0xa8);	STRAY(0xac);
/* 0b0 */	IS(netintr);	STRAY(0xb4);	STRAY(0xb8);	STRAY(0xbc);
/* 0c0 */	IS(hardclock);	STRAY(0xc4);	IS(cnrint1);	IS(cnxint1);
/* 0d0 */	IS(cnrint2);	IS(cnxint2);	IS(cnrint3);	IS(cnxint3);
/* 0e0 */	STRAY(0xe0);	STRAY(0xe4);	STRAY(0xe8);	STRAY(0xec);
/* 0f0 */	IS(consdin);	IS(consdout);	IS(cnrint);	IS(cnxint);
/* ipl 0x14, nexus 0-15 */
/* 100 */	STRAY(0x100);	STRAY(0x104);	STRAY(0x108);	STRAY(0x10c);
/* 110 */	STRAY(0x110);	STRAY(0x114);	STRAY(0x118);	STRAY(0x11c);
/* 120 */	STRAY(0x120);	STRAY(0x124);	STRAY(0x128);	STRAY(0x12c);
/* 130 */	STRAY(0x130);	STRAY(0x134);	STRAY(0x138);	STRAY(0x13c);
/* ipl 0x15, nexus 0-15 */
/* 140 */	STRAY(0x140);	STRAY(0x144);	IS(cmrd);	STRAY(0x14c);
/* 150 */	STRAY(0x150);	STRAY(0x154);	STRAY(0x158);	STRAY(0x15c);
/* 160 */	STRAY(0x160);	STRAY(0x164);	STRAY(0x168);	STRAY(0x16c);
/* 170 */	STRAY(0x170);	STRAY(0x174);	STRAY(0x178);	STRAY(0x17c);
/* ipl 0x16, nexus 0-15 */
/* 180 */	STRAY(0x180);	STRAY(0x184);	STRAY(0x188);	STRAY(0x18c);
/* 190 */	STRAY(0x190);	STRAY(0x194);	STRAY(0x198);	STRAY(0x19c);
/* 1a0 */	STRAY(0x1a0);	STRAY(0x1a4);	STRAY(0x1a8);	STRAY(0x1ac);
/* 1b0 */	STRAY(0x1b0);	STRAY(0x1b4);	STRAY(0x1b8);	STRAY(0x1bc);
/* ipl 0x17, nexus 0-15 */
/* 1c0 */	STRAY(0x1c0);	STRAY(0x1c4);	STRAY(0x1c8);	STRAY(0x1cc);
/* 1d0 */	STRAY(0x1d0);	STRAY(0x1d4);	STRAY(0x1d8);	STRAY(0x1dc);
/* 1e0 */	STRAY(0x1e0);	STRAY(0x1e4);	STRAY(0x1e8);	STRAY(0x1ec);
/* 1f0 */	STRAY(0x1f0);	STRAY(0x1f4);	STRAY(0x1f8);	STRAY(0x1fc);

	.globl	_UNIvec
_UNIvec:

#if defined(VAX8600) || defined(VAX9000)
	/*
	 *	extended SCB for second SBI on VENUS
	 */

/* 200 */	STRAY(0x200);	STRAY(0x204);	STRAY(0x208);	STRAY(0x20c);
/* 210 */	STRAY(0x210);	STRAY(0x214);	STRAY(0x218);	STRAY(0x21c);
/* 220 */	STRAY(0x220);	STRAY(0x224);	STRAY(0x228);	STRAY(0x22c);
/* 230 */	STRAY(0x230);	STRAY(0x234);	STRAY(0x238);	STRAY(0x23c);
/* 240 */	STRAY(0x240);	STRAY(0x244);	STRAY(0x248);	STRAY(0x24c);
/* 250 */	STRAY(0x250);	IS(sbi1fail);	IS(sbi1alert);	IS(sbi1flt);
/* 260 */	IS(sbi1error);	STRAY(0x264);	STRAY(0x268);	STRAY(0x26c);
/* 270 */	STRAY(0x270);	STRAY(0x274);	STRAY(0x278);	STRAY(0x27c);
/* 280 */	STRAY(0x280);	STRAY(0x284);	STRAY(0x288);	STRAY(0x28c);
/* 290 */	STRAY(0x290);	STRAY(0x294);	STRAY(0x298);	STRAY(0x29c);
/* 2a0 */	STRAY(0x2a0);	STRAY(0x2a4);	STRAY(0x2a8);	STRAY(0x2ac);
/* 2b0 */	STRAY(0x2b0);	STRAY(0x2b4);	STRAY(0x2b8);	STRAY(0x2bc);
/* 2c0 */	STRAY(0x2c0);	STRAY(0x2c4);	STRAY(0x2c8);	STRAY(0x2cc);
/* 2d0 */	STRAY(0x2d0);	STRAY(0x2d4);	STRAY(0x2d8);	STRAY(0x2dc);
/* 2e0 */	STRAY(0x2e0);	STRAY(0x2e4);	STRAY(0x2e8);	STRAY(0x2ec);
/* 2f0 */	STRAY(0x2f0);	STRAY(0x2f4);	STRAY(0x2f8);	STRAY(0x2fc);
/* ipl 0x14, SBIA1 nexus 0-15 */
/* 300 */	STRAY(0x300);	STRAY(0x304);	STRAY(0x308);	STRAY(0x30c);
/* 310 */	STRAY(0x310);	STRAY(0x314);	STRAY(0x318);	STRAY(0x31c);
/* 320 */	STRAY(0x320);	STRAY(0x324);	STRAY(0x328);	STRAY(0x32c);
/* 330 */	STRAY(0x330);	STRAY(0x334);	STRAY(0x338);	STRAY(0x33c);
/* ipl 0x15, SBIA1 nexus 0-15 */
/* 340 */	STRAY(0x340);	STRAY(0x344);	STRAY(0x348);	STRAY(0x34c);
/* 350 */	STRAY(0x350);	STRAY(0x354);	STRAY(0x358);	STRAY(0x35c);
/* 360 */	STRAY(0x360);	STRAY(0x364);	STRAY(0x368);	STRAY(0x36c);
/* 370 */	STRAY(0x370);	STRAY(0x374);	STRAY(0x378);	STRAY(0x37c);
/* ipl 0x16, SBIA1 nexus 0-15 */
/* 380 */	STRAY(0x380);	STRAY(0x384);	STRAY(0x388);	STRAY(0x38c);
/* 390 */	STRAY(0x390);	STRAY(0x394);	STRAY(0x398);	STRAY(0x39c);
/* 3a0 */	STRAY(0x3a0);	STRAY(0x3a4);	STRAY(0x3a8);	STRAY(0x3ac);
/* 3b0 */	STRAY(0x3b0);	STRAY(0x3b4);	STRAY(0x3b8);	STRAY(0x3bc);
/* ipl 0x17, SBIA1 nexus 0-15 */
/* 3c0 */	STRAY(0x3c0);	STRAY(0x3c4);	STRAY(0x3c8);	STRAY(0x3cc);
/* 3d0 */	STRAY(0x3d0);	STRAY(0x3d4);	STRAY(0x3d8);	STRAY(0x3dc);
/* 3e0 */	STRAY(0x3e0);	STRAY(0x3e4);	STRAY(0x3e8);	STRAY(0x3ec);
/* 3f0 */	STRAY(0x3f0);	STRAY(0x3f4);	STRAY(0x3f8);	STRAY(0x3fc);
#endif VAX8600 || VAX9000
#if defined(VAX9000)
/*
 * SCB page for third XMI (xmi2)
 */

/* 400 */	STRAY(0x400);	STRAY(0x404);	STRAY(0x408);	STRAY(0x40c);
/* 410 */	STRAY(0x410);	STRAY(0x414);	STRAY(0x418);	STRAY(0x41c);
/* 420 */	STRAY(0x420);	STRAY(0x424);	STRAY(0x428);	STRAY(0x42c);
/* 430 */	STRAY(0x430);	STRAY(0x434);	STRAY(0x438);	STRAY(0x43c);
/* 440 */	STRAY(0x440);	STRAY(0x444);	STRAY(0x448);	STRAY(0x44c);
/* 450 */	STRAY(0x450);	STRAY(0x454);	STRAY(0x458); 	STRAY(0x45c);
/* 460 */	STRAY(0x460); 	STRAY(0x464);	STRAY(0x468);	STRAY(0x46c);
/* 470 */	STRAY(0x470);	STRAY(0x474);	STRAY(0x478);	STRAY(0x47c);
/* 480 */	STRAY(0x480);	STRAY(0x484);	STRAY(0x488);	STRAY(0x48c);
/* 490 */	STRAY(0x490);	STRAY(0x494);	STRAY(0x498);	STRAY(0x49c);
/* 4a0 */	STRAY(0x4a0);	STRAY(0x4a4);	STRAY(0x4a8);	STRAY(0x4ac);
/* 4b0 */	STRAY(0x4b0);	STRAY(0x4b4);	STRAY(0x4b8);	STRAY(0x4bc);
/* 4c0 */	STRAY(0x4c0);	STRAY(0x4c4);	STRAY(0x4c8);	STRAY(0x4cc);
/* 4d0 */	STRAY(0x4d0);	STRAY(0x4d4);	STRAY(0x4d8);	STRAY(0x4dc);
/* 4e0 */	STRAY(0x4e0);	STRAY(0x4e4);	STRAY(0x4e8);	STRAY(0x4ec);
/* 4f0 */	STRAY(0x4f0);	STRAY(0x4f4);	STRAY(0x4f8);	STRAY(0x4fc);
/* 500 */	STRAY(0x500);	STRAY(0x504);	STRAY(0x508);	STRAY(0x50c);
/* 510 */	STRAY(0x510);	STRAY(0x514);	STRAY(0x518);	STRAY(0x51c);
/* 520 */	STRAY(0x520);	STRAY(0x524);	STRAY(0x528);	STRAY(0x52c);
/* 530 */	STRAY(0x530);	STRAY(0x534);	STRAY(0x538);	STRAY(0x53c);
/* 540 */	STRAY(0x540);	STRAY(0x544);	STRAY(0x548);	STRAY(0x54c);
/* 550 */	STRAY(0x550);	STRAY(0x554);	STRAY(0x558);	STRAY(0x55c);
/* 560 */	STRAY(0x560);	STRAY(0x564);	STRAY(0x568);	STRAY(0x56c);
/* 570 */	STRAY(0x570);	STRAY(0x574);	STRAY(0x578);	STRAY(0x57c);
/* 580 */	STRAY(0x580);	STRAY(0x584);	STRAY(0x588);	STRAY(0x58c);
/* 590 */	STRAY(0x590);	STRAY(0x594);	STRAY(0x598);	STRAY(0x59c);
/* 5a0 */	STRAY(0x5a0);	STRAY(0x5a4);	STRAY(0x5a8);	STRAY(0x5ac);
/* 5b0 */	STRAY(0x5b0);	STRAY(0x5b4);	STRAY(0x5b8);	STRAY(0x5bc);
/* 5c0 */	STRAY(0x5c0);	STRAY(0x5c4);	STRAY(0x5c8);	STRAY(0x5cc);
/* 5d0 */	STRAY(0x5d0);	STRAY(0x5d4);	STRAY(0x5d8);	STRAY(0x5dc);
/* 5e0 */	STRAY(0x5e0);	STRAY(0x5e4);	STRAY(0x5e8);	STRAY(0x5ec);
/* 5f0 */	STRAY(0x5f0);	STRAY(0x5f4);	STRAY(0x5f8);	STRAY(0x5fc);
/*
 * SCB page for fourth XMI (xmi3)
 */
/* 600 */	STRAY(0x600);	STRAY(0x604);	STRAY(0x608);	STRAY(0x60c);
/* 610 */	STRAY(0x610);	STRAY(0x614);	STRAY(0x618);	STRAY(0x61c);
/* 620 */	STRAY(0x620);	STRAY(0x624);	STRAY(0x628);	STRAY(0x62c);
/* 630 */	STRAY(0x630);	STRAY(0x634);	STRAY(0x638);	STRAY(0x63c);
/* 640 */	STRAY(0x640);	STRAY(0x644);	STRAY(0x648);	STRAY(0x64c);
/* 650 */	STRAY(0x650);	STRAY(0x654);	STRAY(0x658); 	STRAY(0x65c);
/* 660 */	STRAY(0x660); 	STRAY(0x664);	STRAY(0x668);	STRAY(0x66c);
/* 670 */	STRAY(0x670);	STRAY(0x674);	STRAY(0x678);	STRAY(0x67c);
/* 680 */	STRAY(0x680);	STRAY(0x684);	STRAY(0x688);	STRAY(0x68c);
/* 690 */	STRAY(0x690);	STRAY(0x694);	STRAY(0x698);	STRAY(0x69c);
/* 6a0 */	STRAY(0x6a0);	STRAY(0x6a4);	STRAY(0x6a8);	STRAY(0x6ac);
/* 6b0 */	STRAY(0x6b0);	STRAY(0x6b4);	STRAY(0x6b8);	STRAY(0x6bc);
/* 6c0 */	STRAY(0x6c0);	STRAY(0x6c4);	STRAY(0x6c8);	STRAY(0x6cc);
/* 6d0 */	STRAY(0x6d0);	STRAY(0x6d4);	STRAY(0x6d8);	STRAY(0x6dc);
/* 6e0 */	STRAY(0x6e0);	STRAY(0x6e4);	STRAY(0x6e8);	STRAY(0x6ec);
/* 6f0 */	STRAY(0x6f0);	STRAY(0x6f4);	STRAY(0x6f8);	STRAY(0x6fc);
/* 700 */	STRAY(0x700);	STRAY(0x704);	STRAY(0x708);	STRAY(0x70c);
/* 710 */	STRAY(0x710);	STRAY(0x714);	STRAY(0x718);	STRAY(0x71c);
/* 720 */	STRAY(0x720);	STRAY(0x724);	STRAY(0x728);	STRAY(0x72c);
/* 730 */	STRAY(0x730);	STRAY(0x734);	STRAY(0x738);	STRAY(0x73c);
/* 740 */	STRAY(0x740);	STRAY(0x744);	STRAY(0x748);	STRAY(0x74c);
/* 750 */	STRAY(0x750);	STRAY(0x754);	STRAY(0x758);	STRAY(0x75c);
/* 760 */	STRAY(0x760);	STRAY(0x764);	STRAY(0x768);	STRAY(0x76c);
/* 770 */	STRAY(0x770);	STRAY(0x774);	STRAY(0x778);	STRAY(0x77c);
/* 780 */	STRAY(0x780);	STRAY(0x784);	STRAY(0x788);	STRAY(0x78c);
/* 790 */	STRAY(0x790);	STRAY(0x794);	STRAY(0x798);	STRAY(0x79c);
/* 7a0 */	STRAY(0x7a0);	STRAY(0x7a4);	STRAY(0x7a8);	STRAY(0x7ac);
/* 7b0 */	STRAY(0x7b0);	STRAY(0x7b4);	STRAY(0x7b8);	STRAY(0x7bc);
/* 7c0 */	STRAY(0x7c0);	STRAY(0x7c4);	STRAY(0x7c8);	STRAY(0x7cc);
/* 7d0 */	STRAY(0x7d0);	STRAY(0x7d4);	STRAY(0x7d8);	STRAY(0x7dc);
/* 7e0 */	STRAY(0x7e0);	STRAY(0x7e4);	STRAY(0x7e8);	STRAY(0x7ec);
/* 7f0 */	STRAY(0x7f0);	STRAY(0x7f4);	STRAY(0x7f8);	STRAY(0x7fc);
#endif VAX9000

	.space (512*NUBA)


/* note that scorpio uses first page for BI vectors. */
/* VAX8800 needs extra BI page to align BI scb  pages. */

#if defined(VAX8800) || defined(VAX6200) || defined(VAX6400) || defined(VAX9000)
	.globl _vax8800bivec
_vax8800bivec:
#if CVAXBI > 7
	.space ((CVAXBI) * 512)
#else
#if defined(VAX8800)
	.space (7 * 512)
#else
	.space (CVAXBI*512)
#endif
#endif


#endif
	.globl _scbend

_scbend:

	.globl  _eintstack
        .globl  _intstack
_intstack:
        .space  NISP*512
_eintstack:

/*
 * In order to merge MicroVAX support back in with other vaxen,
 * these 2 symbols need to be defined for non-MicroVAX vaxen.
 * If MVAX or VAX3600 or VAX420 or VAX6200 or VAX6400 is defined,
 * then we get the emulation code and hence
 * get the real definition of these.
 */
#if defined (MVAX) || defined (VAX3600) || defined (VAX420) || defined(VAX6200) || defined (VAX60) || defined (VAX6400) || defined (VAX9000)
#define GOTEMUL
#endif

#ifndef GOTEMUL
	.data
	.globl vax$emulate
	.globl vax$emulate_fpd
vax$emulate:
	.long 0
vax$emulate_fpd:
	.long 0
#endif
#undef GOTEMUL
