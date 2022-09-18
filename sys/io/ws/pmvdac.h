/*
 * @(#)pmvdac.h	4.1	(ULTRIX)	8/13/90
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
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
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/

/* created by Joel Gringorten - based on pmreg.h */

#include "../../machine/mips/cpu.h"

/********************************************************
 *							*
 *  These are the physical registers in the memory map	*
 *							*
 ********************************************************/
#define CURSOR_REG	0x11000000
#define FRAME_BUF	0x0fc00000
#define VDAC_REG	0x12000000
#define PLANE_REG	0x10000000

/********************************************************
 * Following allow pmputc to function in		*
 * in physical mode (during a crash dump).		*
 * One way transition, can't go back to virtual.	*
 ********************************************************/
#define PCC_BASE (*(struct pcc_regs*)PHYS_TO_K1(CURSOR_REG))
#define PCC_ADDR &PCC_BASE
#define BITMAP_BASE (*(u_int *)PHYS_TO_K1(FRAME_BUF))
#define BITMAP_ADDR &BITMAP_BASE
#define VDAC_BASE (*(vdac_regs *)PHYS_TO_K1(VDAC_REG))
#define VDAC_ADDR &VDAC_BASE
#define PLANE (*(short *)PHYS_TO_K1(PLANE_REG))
#define PLANE_ADDR &PLANE

/* cursor control chip */

struct pcc_regs {
        u_short 	cmdr;   /* 0 */
	u_short		pad1;
	u_short 	xpos;   /* 1 */
	u_short		pad2;
	u_short 	ypos;   /* 2 */
	u_short		pad3;
	u_short 	xmin1;  /* 3 */
	u_short		pad4;
	u_short 	xmax1;  /* 4 */
	u_short		pad5;
	u_short 	ymin1;  /* 5 */
	u_short		pad6;
	u_short 	ymax1;  /* 6 */
	u_short		pad7;
	u_short         unused7; /* 7 */
	u_short         padunused7; 
	u_short         unused8; /* 8 */
	u_short         padunused8; 
	u_short         unused9; /* 9 */
	u_short         padunused9; 
	u_short         unusedA; /* A */
	u_short         padunusedA; 
	u_short  	xmin2;   /* B */
	u_short		pad8;
	u_short		xmax2;   /* C */
	u_short		pad9;
	u_short		ymin2;   /* D */
	u_short		pada;
	u_short		ymax2;   /* E */
	u_short		padb;
	u_short 	memory;  /* F */
	u_short		padc;
};

/*  structure declaration for VDAC, regs aligned on word boundries */

typedef volatile struct {
    u_char   map_wr;
    u_char   pad0;
    u_short  pad1;

    u_char   map_ram;
    u_char   pad2;
    u_short  pad3;

    u_char   mask;
    u_char   pad4;
    u_short  pad5;

    u_char   map_rd;
    u_char   pad6;
    u_short  pad7;

    u_char   over_wr;
    u_char   pad8;
    u_short  pad9;

    u_char   over_regs;
    u_char   pad10;
    u_short  pad11;

    u_char   reserved;
    u_char   pad12;
    u_short  pad13;

    u_char   over_rd;
    u_char   pad14;
    u_short  pad15;
}vdac_regs;


struct pmvdacinfo {
        vdac_regs *pmvdac_addr;
	struct pcc_regs *pccregs_addr;
	int fb_xoffset;				/* offset to video */
	int fb_yoffset;
	int x_hot;				/* hot spot of current cursor*/
	int y_hot;
	ws_color_cell saved_entry;
	ws_color_cell cursor_fg;
	ws_color_cell cursor_bg;
        unsigned short bits[32];
};


/*
 * Cursor Command Register bits
 *
 */

#define	ENPA	0000001
#define	FOPA	0000002
#define	ENPB	0000004
#define	FOPB	0000010
#define XHAIR	0000020
#define	XHCLP	0000040
#define	XHCL1	0000100
#define	XHWID	0000200
#define	ENRG1	0000400
#define	FORG1	0001000
#define	ENRG2	0002000
#define	FORG2	0004000
#define	LODSA	0010000
#define	VBHI	0020000
#define	HSHI	0040000
#define	TEST	0100000


caddr_t pmax_init_closure();
int pmax_load_cursor();
int pmax_init_color_map();
int pmax_load_color_map_entry();
int pmax_load_cursor();
int pmax_recolor_cursor();
int pmax_set_cursor_position();
int pmax_cursor_on_off();
int pmax_video_on();
int pmax_video_off();
