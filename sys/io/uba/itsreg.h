
/*
 *	@(#)itsreg.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * itsreg.h
 *
 * Modification history
 *
 * ITEX/FG101 Series 100 frame buffer registers/data structures and definitions
 *
 *  2-Dec-86 - lp/rsp (Larry Palmer/Ricky Palmer)
 *
 *	Created prototype frame buffer driver.
 */

/*
 *	This  driver  provides user level programs with the ability to
 *	read  and  write  any  of  the	frame  buffer registers.  This
 *	capability  is	provided  via  the  ITSMAP  ioctl  request and
 *	subsequent  user  direct  access  to  the  registers and frame
 *	memory.   The  driver  initializes  the  scanner hardware in a
 *	default  state.   The same initialization is possible by using
 *	the  registers	directly.  Note that DMA is NOT possible since
 *	the board does not support local DMA transfers to another QBUS
 *	device.   The  documentation  provided	by  Imaging Technology
 *	should	be  consulted  for further details involving the frame
 *	buffer	hardware/software.   This  driver  is  provided  as  a
 *	service  to  Digital  Ultrix  customers  who  would  like  the
 *	capability to combine the frame buffer hardware with their GPX
 *	system	in  order  to  create  digital	images from a standard
 *	television  camera hookup.  The driver is provided "as-is" and
 *	is  NOT  supported  under any Digital agreements or contracts.
 *	Electronic   mail   concerning	the  driver  may  be  sent  to
 *	decvax!rsp  or	decvax!lp  but	there  is  no guarantee of any
 *	response or support.
 *
 *	This driver assumes the following configuration:
 *
 *		FG101 at 173000
 *		Memory Map @ 0x30200000
 *
 *	The availble ioctl requests for this driver are:
 *
 *	(1) ITSMAP - maps the frame buffer into user space.
 *	(2) ITSGRABIT - starts the frame buffer grabbing frames (30/sec).
 *	(3) ITSFREEZE - freezes a frame.
 *	(4) ITSZOOM - initiates a zoom;subsequent calls cycle through.
 *
 */

struct itsdevice {
		unsigned short	mac; /* Memory access control reg */
#define 	MAC_PSL0 0x0001
#define 	MAC_PSL1 0x0002
#define 	MAC_PSL2 0x0004
#define 	MAC_TMODEL 0x0010
#define 	MAC_PBENL 0x0040
#define 	MAC_PSH0  0x0100
#define 	MAC_PSH1  0x0200
#define 	MAC_PSH2  0x0400
#define 	MAC_TMODEH 0x1000
#define 	MAC_PBENH  0x4000

		unsigned short hmask; /* Host mask reg */
#define 	PROTECT_ALL	0x0fff

		unsigned short vam; /* Video acquistion mask reg */

		unsigned short pbr; /* Pixel Buffer Register */

		unsigned short xptr; /* X pointer register/counter */
#define 	PTRMASK 0x03ff

		unsigned short yptr; /* Y pointer register/counter */

		unsigned short pc; /* Pointer control register */
#define 	YPEN	0x0001
#define 	YPDIR	0x0002
#define 	XPEN	0x0010
#define 	XPDIR	0x0020
#define 	YCNT	0x0700 /* 3 bits */
#define 	XCNT	0x7000 /* 3 bits */

		unsigned short ac; /* CPU address control register */
#define 	ITSMAPEN	0x0001
#define 	MAP	0x0006	/* 512x512 */
#define 	MAP_BYTE 0x0200 /* Scale = 0 */
#define 	MAP_WORD 0x0400 /* Scale = 1 */
#define 	SCALE	0x0600	/* Scale = 0 */

		unsigned short xspin; /* X spin constant register */
#define 	SPINMASK 0x0fff

		unsigned short yspin; /* Y spin constant register */

		unsigned short pana; /* pan a register */
#define 	PANMASK 0x0fff
		unsigned short panb; /* Pan b OR LUT control register */
#define 	lutcsr	panb
/* PANMASK OR these lut bits */
#define 	ILUT	0x000f
#define 	OLUT	0x00f0
#define 	FBKMD	0x0700
#define 	ITSSSEL 0x0800
#define 	LUTSEL	0x1000
#define 	BDSEL	0x2000
#define 	DUALSCAN	0x8000

		unsigned short scrolla; /* scroll */
#define 	SCROLLMASK 0x0fff

		unsigned short scrollb; /* scroll b OR csr */
#define 	csr	scrollb
/* SCROLLMASK OR the following bits */
#define 	POE	0x0003	/* Controls LUT access OR pointer */
#define 	SYNC	0x0004
#define 	VIDSEL	0x0018
#define 	IRQS	0x0020
#define 	CLKSEL	0x0040
#define 	INTENA	0x0080
#define 	ODDEVEN 0x0100
#define 	LSTFLD	0x0200
#define 	VBLANK	0x0400
#define 	HBLANK	0x0800
#define 	ACQMODE 0x3000
#define 	ITSCLEAR	0x1000
#define 	ITSSNAP 	0x2000
#define 	ITSGRAB 	0x3000
#define 	SZMODE	0x4000

		unsigned short zoom; /* Zoom control register */
#define 	ZMASK	0xefe8	/* Sets mask reserved bits */
#define 	ZFACT	0x0003
#define 	REGMUX	0x1000

		unsigned short framedata; /* Non memory map to frame buffer */

		};
#define ITSSCSR 0773000
#define ITSMEM	0x200000 /* Where frame memory starts */
#define ITSREG	0x3f600 /* Where reg memory starts */
#define CAMERA0 0x0000
#define CAMERA1 0x0080
#define CAMERA2 0x0100
#define CAMERA3 0x0180
#define CRYSTAL 0x0000
#define PLLOOP	0x0040
#define ITS_GRABBING 0x0001
#define ITS_FROZEN   0x0002
#define ITSUNIT(x) (minor(x))
#define LUTRED 0x0000
#define LUTGRN 0x2000
#define LUTBLU 0x4000
#define LUTINP 0x6000
