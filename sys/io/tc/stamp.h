/*
 *	@(#)stamp.h	4.1	(ULTRIX)	7/2/90
 */
/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
/*
 * 16-Jan-90	Sam Hsu
 *	Remove symbol conflicts with the ucode/server.  Move STIC definitions
 *	to gx.h
 *
 * 00-Xyz-89	Sam Hsu
 *	$Header: stamp.h,v 1.7 90/02/16 22:38:47 fhsu Exp $
 *	Created.
 */
#ifndef _STAMP_H_
#define _STAMP_H_

typedef struct _stamp_cmd {
    unsigned opcode : 4;		/* <3:0> */
#define	STAMP_CMD_POINTS	0
#define	STAMP_CMD_LINES		1
#define	STAMP_CMD_TRIANGLES	2
#define	STAMP_CMD_COPYSPANS	5
#define	STAMP_CMD_READSPANS	6
#define	STAMP_CMD_WRITESPANS	7
#define	STAMP_CMD_VIDEO		8
    unsigned rgb_format : 2;		/* <5:4> */
#define STAMP_RGB_NONE		0
#define STAMP_RGB_CONST		1
#define STAMP_RGB_FLAT		2
#define STAMP_RGB_SMOOTH	3
    unsigned z_format : 2;		/* <7:6> */
#define STAMP_Z_NONE		0
#define STAMP_Z_CONST		1
#define STAMP_Z_FLAT		2
#define STAMP_Z_SMOOTH		3
    unsigned xymask_format : 2;		/* <9:8> */
#define	STAMP_XY_NONE		0
#define	STAMP_XY_PERPACKET	1
#define	STAMP_XY_PERPRIMITIVE	2
    unsigned linewidth_format : 2;	/* <11:10> */
#define	STAMP_LW_NONE		0
#define	STAMP_LW_PERPACKET	1
#define	STAMP_LW_PERPRIMITIVE	2
#define STAMP_LW_MASK		0x3fff	/* 14 bits */
    unsigned : 7;			/* <18:12> */
    unsigned cliprect : 1;		/* <19> */
    unsigned : 1;			/* <20> */
    unsigned mesh : 1;			/* <21> */
    unsigned : 1;			/* <22> */
    unsigned aa_line : 1;		/* <23> */
    unsigned : 7;			/* <30:24> */
    unsigned hs_equals : 1;		/* <31> */
} stampCmd;
#define STAMP_cmd	stampCmd

		/* COMMAND <3:0> */

#define CMD_MASK		(0xf)
#define CMD_POINTS		(0)
#define CMD_LINES		(1)
#define CMD_TRIANGLES		(2)
#define CMD_COPYSPANS		(5)
#define CMD_READSPANS		(6)
#define CMD_WRITESPANS		(7)
#define CMD_VIDEO		(8)

		/* COMMAND <5:4> */

#define RGB_MASK        	(0x30)
#define RGB_NONE  	        (0<<4)
#define RGB_CONST	 	(1<<4)
#define RGB_FLAT		(2<<4)
#define RGB_SMOOTH        	(3<<4)

		/* COMMAND <7:6> */

#define Z_MASK	        	(0xc0)
#define Z_NONE  	        (0<<6)
#define Z_CONST		 	(1<<6)
#define Z_FLAT			(2<<6)
#define Z_SMOOTH        	(3<<6)

		/* COMMAND <9:8> */

#define XY_MASK			(0x300)
#define XY_NONE			(0<<8)
#define XY_PERPKT		(1<<8)
#define XY_PERPRIM		(2<<8)

		/* COMMAND <11:10> */

#define LW_MASK			(0xc00)
#define LW_NONE	   		(0<<10)
#define LW_PERPKT	  	(1<<10)
#define LW_PERPRIM	 	(2<<10)

		/* COMMAND <19> */

#define CLIPRECT_MASK        	(0x80000)
#define CLIPRECT        	(1<<19)

		/* COMMAND <21> */

#define MESH_MASK		(0x200000)
#define MESH         		(1<<21)

		/* COMMAND <23> */

#define AA_LINE_MASK		(0x800000)
#define AA_LINE               	(1<<23)

		/* COMMAND <31> */

#define HS_EQUALS_MASK		(0x80000000)
#define HS_EQUALS              	(1<<31)


typedef struct _stamp_zcount {
    unsigned mask : 24;
    unsigned count : 8;
} stampZCount;
#define STAMP_zcount	stampZCount

#define STAMP_MAX_CMDS	((1<<8)-1)

typedef struct _stamp_update {
    unsigned enable : 1;		/* <0> */
    unsigned read_buff : 2;		/* <2:1> */
    unsigned write_buff : 2;		/* <4:3> */
    unsigned plane : 1;			/* <5> */
#define STAMP_PLANE_8X3		0
#define STAMP_PLANE_24		1
    unsigned save_sign : 1;		/* <6> */
    unsigned save_alpha : 1;		/* <7> */
    unsigned make_we : 3;		/* <10:8> */
#define STAMP_WE_SIGN		0x4
#define STAMP_WE_XYMASK		0x2
#define STAMP_WE_CLIPRECT	0x1
#define STAMP_WE_NONE		0x0
    unsigned supersample : 1;		/* <11> */
    unsigned umet : 7;			/* <18:12> */
#define STAMP_UMET_CLEAR	0x60
#define STAMP_UMET_AND		0x14
#define STAMP_UMET_ANDREV	0x15
#define STAMP_UMET_COPY		0x20
#define STAMP_UMET_ANDINV	0x16
#define STAMP_UMET_NOOP		0x40
#define STAMP_UMET_XOR		0x11
#define STAMP_UMET_OR		0x0f
#define STAMP_UMET_NOR		0x17
#define STAMP_UMET_EQUIV	0x10
#define STAMP_UMET_INV		0x4e
#define STAMP_UMET_ORREV	0x0e
#define STAMP_UMET_COPYINV	0x2d
#define STAMP_UMET_ORINV	0x0d
#define STAMP_UMET_NAND		0x0c
#define STAMP_UMET_SET		0x6c
#define STAMP_UMET_SUM		0x00
#define STAMP_UMET_DIFF		0x02
#define STAMP_UMET_REVDIFF	0x01
    unsigned span : 1;			/* <19> */
    unsigned aligned : 1;		/* <20> */
    unsigned minmax : 1;		/* <21> */
    unsigned mult : 1;			/* <22> */
    unsigned multacc : 1;		/* <23> */
    unsigned : 3;			/* <26:24> */
    unsigned halfbuff : 1;		/* <27> */
    unsigned dblbuff : 3;		/* <30:28> */
#define STAMP_DB_NONE		0x0
#define STAMP_DB_01		0x1
#define STAMP_DB_12		0x2
#define STAMP_DB_02		0x4
    unsigned init : 1;			/* <31> */
} stampUpdate;
#define STAMP_update	stampUpdate

		/* UPDATE{RGB,Z} <0> */

#define UPD_ENABLE_MASK		(0x1)
#define UPD_ENABLE             	(1)

		/* UPDATE <2:1> */

#define READ_BUFF_MASK    	(0x6)
#define READ_BUFF_0		(0<<1)
#define READ_BUFF_1		(1<<1)
#define READ_BUFF_2		(2<<1)
#define READ_BUFF_3		(3<<1)

		/* UPDATE <4:3> */

#define WRITE_BUFF_MASK    	(0x18)
#define WRITE_BUFF_0		(0<<3)
#define WRITE_BUFF_1		(1<<3)
#define WRITE_BUFF_2		(2<<3)
#define WRITE_BUFF_3		(3<<3)

		/* UPDATE <5> */

#define PLANE_MASK		(0x20)
#define PLANE_8x3		(0<<5)
#define PLANE_24		(1<<5)

		/* UPDATE <6> */

#define SAVE_SIGN_MASK		(0x40)
#define SAVE_SIGN		(1<<6)

		/* UPDATE <7> */

#define SAVE_ALPHA_MASK		(0x80)
#define SAVE_ALPHA		(1<<7)

		/* UPDATE <10:8> (Indicates how to construct write-enables */

#define WE_MASK			(0x700)
#define WE_CLIPRECT		(1<<8)
#define WE_XYMASK		(2<<8)
#define WE_SIGN			(4<<8)

		/* UPDATE <11> */

#define SUPERSAMPLE_MASK	(0x800)
#define SUPERSAMPLE		(1<<11)

		/* UPDATE <18:12> (Update method)

        /* logical functions */
#define UMET_MASK		(0x7f000)
#define UMET_CLEAR		(0x60<<12)
#define UMET_AND		(0x14<<12)
#define UMET_ANDREVERSE		(0x15<<12)
#define UMET_COPY		(0x20<<12)
#define UMET_ANDINVERTED	(0x16<<12)
#define UMET_NOOP		(0x40<<12)
#define UMET_XOR		(0x11<<12)
#define UMET_OR			(0x0F<<12)
#define UMET_NOR		(0x17<<12)
#define UMET_EQUIV		(0x10<<12)
#define UMET_INVERT		(0x4E<<12)
#define UMET_ORREVERSE		(0x0E<<12)
#define UMET_COPYINVERTED	(0x2D<<12)
#define UMET_ORINVERTED		(0x0D<<12)
#define UMET_NAND		(0x0C<<12)
#define UMET_SET		(0x6C<<12)
        /* arithmetic functions */
#define UMET_SUM		(0x00<<12)
#define UMET_DIFF		(0x02<<12)
#define UMET_REVDIFF		(0x01<<12)


		/* UPDATE <19> */

#define SPAN_MASK		(0x80000)
#define SPAN			(1<<19)

		/* UPDATE <20> */

#define COPYSPAN_ALIGNED_MASK	(0x100000)
#define COPYSPAN_ALIGNED	(1<<20)

		/* UPDATE <21> */

#define MINMAX_MASK		(0x200000)
#define MINMAX			(1<<21)

		/* UPDATE <22> */

#define MULT_MASK		(0x400000)
#define MULT			(1<<22)

		/* UPDATE <23> */

#define MULTACC_MASK		(0x800000)
#define MULTACC			(1<<23)

		/* UPDATE <27> */

#define HALF_BUFF_MASK		(0x8000000)
#define HALF_BUFF		(1<27)

		/* UPDATE <30:28> */

#define DB_MASK			(0x70000000)
		      /* for {4,5}x2 only */
#define DB_BUFF02_13		(1<<28)
		      /* for {4,5}x1 only */
#define DB_BUFF0_1		(1<<28)
#define DB_BUFF1_2		(2<<28)
#define DB_BUFF0_2		(4<<28)

		/* UPDATE <31> */

#define INITIALIZE_MASK		(0x80000000)
#define INITIALIZE		(1<<31)


/*
 * per-packet context for xymask 16x16 (text - 8x15)
 * nb: also need per-primitive xymask_addr context.
 */

#ifdef KERNEL
#define STAMP_WIDTH	(gxp->stamp_width)
#define STAMP_HEIGHT	(gxp->stamp_height)
#endif

typedef short stampXYMask[16];
#define STAMP_xymask	stampXYMask

#define XMASKADDR(startX, A)	(((A)-((startX)%STAMP_WIDTH))&0xF)
#define YMASKADDR(startY, B)	(((B)-((startY)%STAMP_HEIGHT))&0xF)

#define XYMASKADDR(X,Y, A,B) \
        ( XMASKADDR(X,A) << 16 \
         |YMASKADDR(Y,B) )

#define CONSXYADDR(X,Y)		XYMASKADDR(X,Y, 0,0)

/* optional per-packet context:
       line width
       xymask
       cliprect min & max
       rgb constant
       z constant
 * optional per-primitive context:
       xymask
       xymask addr
       prim data (vertices, spans info, video)
       line width
       halfspace equals conditions
       rgb flat, or rgb{1,2,3} smooth
       z flat, or z{1,2,3} smooth
 
  The 3 operations needed for the console are:
 	clear screen (wide line, umet=copy)
 	write text (wide line, umet=copy, xymask)
 	scroll (copy spans, umet=copy)
 */

#define STAMP_GOOD	(0)
#define STAMP_BUSY	(1)
#define STAMP_RETRIES	(5000)		/* = 50 msec */
#define STAMP_DELAY	(10)		/* = 10 usec */

#endif /* _STAMP_H_ */
