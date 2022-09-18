/*
 * static char *sccsid = "@(#)cca.h	4.1      (ULTRIX)  7/2/90";
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88,89 by			*
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

/***********************************************************************
 *
 * Modification History:	cca.h
 *
 *	03-Feb-89 -- Bill Burns
 *		Console communications area defines and structures.
 *
 **********************************************************************/

#define CCA_V_BOOTIP 0x1
#define CCA_V_WARMIP 0x2
#define CCA_V_REBOOT 0x10
#define CCA_V_NO_2ND_TEST  0x4

struct cca {
	v_char		*cca_base;
	v_short		cca_size;
	v_char		cca_indent0;
	v_char		cca_indent1;
	v_char		cca_nproc;
	v_char		cca_chksum;
	v_char		cca_hflag;
	v_char		cca_wstarter;
	vu_long		cca_ready;
	vu_long		cca_ready1;
	vu_long		cca_console;
	vu_long		cca_console1;
	vu_long 	cca_enabled;
	vu_long 	cca_enabled1;
	vu_long		cca_bitmap_sz;
	vu_long		cca_bitmap;
	vu_long 	cca_bitmap_cksum;
	vu_long 	cca_serialnum;	
	v_char		pad[460];
	struct {
		v_char	flags;
		v_char	zdest;
		v_char	zsrc;
		v_char	spare;
		v_char 	txlen;
		v_char	rxlen;
		v_short	zrxcd;
		v_char	tx[80];
		v_char	rx[80];
	} cca_buf[64];

};

struct cca *ccabase;

#define RXRDY 0x1

struct ctsi_chan_blk {
	vu_char	ctcb_dvatr;		/* Device attributes byte	    */
	vu_char	ctcb_chatr;		/* Channel attributes byte	    */
	vu_short	ctcb_statesize;		/* Driver state size	    */
	v_char	*ctcb_entry_p;		/* Driver entry point physical addr */
	v_char	*ctcb_entry_v;		/* Driver entry point virtual addr  */
	v_char	*ctcb_iosegtbl_p;	/* I/O Segment table physical addr  */
	v_char	*ctcb_iosegtbl_v;	/* I/O Segment table virtual addr   */
	v_char	*ctcb_extdvrstate_p;	/* Extended Driver State physical addr*/
	v_char	*ctcb_extdvrstate_v;	/* Extended Driver State virtual addr*/
};

struct ctsi_mode_desc {
	vu_short	ctmd_pgcnt;	/* Module Descriptor page count	    */
	vu_short 	:16;		/* unused			    */
	v_char		*ctmd_baseaddr;	/* Base Address of Firmware Module  */
};

struct ctsi_getchar_state {
	vu_char		ctgs_flags;	/* Flags		      */
	vu_char		ctgs_kbd;	/* Keyboard		      */
	vu_short	:16;		/* unused		      */
	vu_long		pad[3];		/* Reserved		      */
};

struct ctsi_putchar_state {
	vu_char		ctps_flags;	/* Flags		      */
	vu_char		:8;		/* unused		      */
	vu_short	:16;		/* unused		      */
	vu_long 	pad[3];		/* unused		      */
};

struct ctsi {
	vu_long		ct_base;		/* phys addr of base of CTSIA	      */
	vu_short	ct_size;		/* size in bytes of the CTSIAZZ	      */
	vu_short	ct_ident;		/* The ASCII characters "CT"	      */
	vu_char		:8;			/* unused			      */
	vu_char		ct_cksum;		/* Checksum			      */
	vu_char		ct_flags;		/* Flags used by CTSIA routines	      */
	vu_char		ct_revisn;		/* Rev. number  for the CTSIA format  */
	struct ctsi_mode_desc	ct_cons0;	/* Console Mod Descriptor 0   */
	struct ctsi_mode_desc	ct_cons1;	/* Console Mod Descriptor 1   */
	struct ctsi_mode_desc	ct_rcons0;	/* Remote Console Desc. 0     */
	struct ctsi_mode_desc	ct_rcons1;	/* Remote Console Desc. 1     */
	struct ctsi_mode_desc	ct_fcons;	/* Fallback Conssole Desc.    */
	struct ctsi_mode_desc	ct_ucons;	/* Utility Console Descriptor */
	struct ctsi_chan_blk	ct_std_in;	/* Standard Input Channel     */
	struct ctsi_chan_blk	ct_std_out;	/* Standard Output Channel    */
	struct ctsi_chan_blk	ct_fb_in;	/* Fallback Input Channel     */
	struct ctsi_chan_blk	ct_fb_out;	/* Fallback Output Channel    */
	struct ctsi_chan_blk	ct_rmot_in;	/* Remote Input Channel	      */
	struct ctsi_chan_blk	ct_rmot_out;	/* Remote Output Channel      */
	v_char	*ct_save_p;		/* SAVE physical address	      */
	v_char	*ct_save_v;		/* SAVE virutal address		      */
	v_char	*ct_restore_p;		/* RESTORE physical address	      */
	v_char	*ct_restore_v;		/* RESTORE virtual address	      */
	v_char	*ct_translate_p;	/* TRANSLATE physical address	      */
	v_char	*ct_translate_v;	/* TRANSLATE virtual address	      */
	v_char	*ct_getchar_p;		/* GET_CHARACTER physical address     */
	v_char	*ct_getchar_v;		/* GET_CHARACTER virtual address      */
	struct ctsi_getchar_state ct_getchar_state; /* GET_CHARACTER state   */
	v_char	*ct_putchar_p;		/* PUT_CHARACTER physical address     */
	v_char	*ct_putchar_v;		/* PUT_CHARACTER virtual address      */
	struct ctsi_putchar_state ct_putchar_state; /* PUT_CHARACTER state   */
	v_char	*ct_msgout_p;		/* MESSAGE_OUT physical address	      */
	v_char	*ct_msgout_v;		/* MESSAGE_OUT virtual address	      */
	v_char	*ct_rwp_p;		/* READ_WITH_PROMPT physical address  */
	v_char	*ct_rwp_v;		/* READ_WITH_PROMPT virtual address   */
};
