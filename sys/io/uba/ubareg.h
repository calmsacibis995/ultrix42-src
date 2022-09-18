/*
 *    @(#)ubareg.h	4.2 (ULTRIX)    8/3/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1984,86,87,88 by			*
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/vaxuba/ubareg.h
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added a define for VAX9000.
 *
 *  8-May-89  Giles Atkinson
 *    Extend definition of nb4_regs to include console version.
 *
 * 05-Nov-88	Robin Lewis
 *	Changed the defines used to get map register information on
 *	the Q22-Bus.  The format of the "cookie" was changed to allow
 *	more map registers to be allocated on one call.
 *
 * 28-Jun-88	Fred Canter
 *	Modified nb_regs structure to add PVAX system SCSI bus ID.
 *	This allows the SCSI drive to read the system's (initiator)
 *	ID from NVR.
 *
 * 08-Jun-88	darrell for Ali
 *	Added VAX60 (Firefox) support.
 *
 * 07-Jun-88	Fred Canter
 *	Back out last change (hardware changed their mind).
 *
 * 06-Jun-88	Fred Canter
 *	Use different address for SCSI/SCSI controller.
 *
 * 19-May-88    Fred Canter
 *	Modifications for SCSI driver and extended I/O mode
 *	operation of CVAXstarPVAX.
 *
 * 15-Feb-88	Fred Canter
 *	Added support for VAX420 (CVAXstar/PVAX) CPU.
 *	Mostly I/O space register structutes and definitions.
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass support to the system.
 *
 *  3-Aug-87 -- rafiey (Ali Rafieymehr)
 *	Added structure for the VAXstation 2000 system scratch RAM.
 *
 * 20-Apr-87 -- afd
 *	Moved structures for Mayfair local register space to ka650.h
 *
 * 06-Mar-87 -- afd
 *	Defined 5 structures for Mayfair/CVAX local register space;
 *	also defined constants for their addresses and sizes.
 *
 * 07-Jan-87  -- rafiey (Ali Rafieymehr)
 *	Corrected the value for the VAXstar color display option
 *	(VS_CID_COLOR).
 *
 * 13-Dec-86  -- fred (Fred Canter) and rafiey (Ali Rafieymehr)
 *	Change sh and sg maps (address and size).
 * 30-Aug-86  -- fred (Fred Canter)
 *	General cleanup of comments.
 *
 *  5-Aug-86   -- fred (Fred Canter)
 *	Updated VAXstar I/O space register definitions.
 *
 *  2-Jul-86   -- fred (Fred Canter)
 *	Defines for TEAMmate 8 line SLU register mapping.
 *
 * 18-Jun-86   -- fred (Fred Canter)
 *	Changes for VAXstar kernel support.
 *
 * 13-Jun-86   -- jaw 	fix to uba reset and drivers.
 *
 * 14-Apr-86 -- jaw
 *	remove MAXNUBA referances.....use NUBA only!
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 * 04-feb-86 -- jaw  get rid of biic.h.
 *
 * 22-Oct-85 -- tresvik
 *	fixed typo to for VAX8600 conditional
 *
 * 10-sep-85 -- jaw
 *	increased number of UBA's on 8200 from 4 to 10.
 *
 * 31-Jul-85 -- tresvik
 *	Fixed sizing problem for maximum uba's on VAX8600.  Increased 
 *	MAXNUBA and NUBA8600 to 8 to allow proper indexing.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 18-Jun-85 -- JAW
 *	fixed up purge macro so machine specific compiles work.
 *
 * 01-May-85 -tresvik
 *	Fix conditionals on UBA register defs so that /sys/stand could build
 *	a 750 only version of autoconf.c
 *
 * 13-Mar-85 -jaw
 *	Changes for support of the VAX8200 were merged in.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 *  3 Nov 84 -- rjl
 *	MicroVAX-II support
 *	The microVAX support stuff should probably be in seperate qbus files
 *	but at this time is so close that it hardly seems to make much sense.
 *
 * 29 Dec 83 --jmcg
 *	MicroVAX I support.
 *
 * 28 Dec 83 --jmcg
 *	Derived from 4.2BSD, labeled:
 *		ubareg.h	6.2	83/09/29
 *
 * ------------------------------------------------------------------------
 */

#include "../io/bi/buareg.h"

/*
 * VAX UNIBUS adapter registers
 */
#ifndef LOCORE
/*
 * UBA hardware registers
 */
struct uba_regs
{
	int	uba_cnfgr;		/* configuration register */
	int	uba_cr;			/* control register */
	int	uba_sr;			/* status register */
	int	uba_dcr;		/* diagnostic control register */
	int	uba_fmer;		/* failed map entry register */
	int	uba_fubar;		/* failed UNIBUS address register */
	int	pad1[2];
	int	uba_brsvr[4];
	int	uba_brrvr[4];		/* receive vector registers */
	int	uba_dpr[16];		/* buffered data path register */
	int	pad2[480];
	struct	pte uba_map[496];	/* unibus map register */
	int	pad3[16];		/* no maps for device address space */
};
/*
 * MicroVAX-II local registers
 */
struct qb_regs
{
	u_short	qb_bdr;			/* boot and diag register	*/
	u_short qb_pad1;
	u_long	qb_mser;		/* memory system error register	*/
	u_long	qb_caer;		/* cpu error address		*/
	u_long	qb_daer;		/* dma error address		*/
	u_long	qb_pad2[7676];
	union {
		struct {
		int qb_pad[512];
		struct pte qb_map[8192];/* q-bus map registers		*/
		} qba; 
		struct uba_regs uba;
	} qb_uba;
	u_long	qb_pad3[40960];
	u_short	qb_toysecs;		/* seconds			*/
	u_short	qb_toysecs_alarm;	/* not used			*/
	u_short	qb_toymins;		/* minutes			*/
	u_short	qb_toymins_alarm;	/* not used			*/
	u_short	qb_toyhours;		/* hours			*/
	u_short	qb_toyhours_alarm;	/* not used			*/
	u_short	qb_toyday_week;		/* not used			*/
	u_short	qb_toyday;		/* day of month			*/
	u_short	qb_toymonth;		/* month			*/
	u_short	qb_toyyear;		/* year				*/
	u_short	qb_toycsra;		/* csr				*/
	u_short	qb_toycsrb;		/* csr				*/
	u_short	qb_toycsrc;		/* csr				*/
	u_short	qb_toycsrd;		/* csr				*/
	u_short	qb_cpmbx;		/* console program mailbox	*/
	u_short qb_toyram[49];		/* toy ram			*/
};
/*
 * VAXstar/CVAXstar local registers & I/O space including SLU controller.
 * The VAXstar is a busless machine.
 * The name nb_regs represents "no bus", i.e.,
 * uba_regs, qb_regs, nb_regs.
 *
 * Mapping is split into three segments:
 *   nb_regs	hltcod -> toyram
 *  nb1_regs	disk cntlr -> video option board ROM
 *  nb2_regs	Monochrome video RAM
 *  A fourth segment will be needed for the color option.
 *  The fifth segment is used for the MicroVAX 2000 serial 
 *  line expander (8 line SLU). Defined in shreg.h, not here.
 */
struct nb_regs
{
	u_long	nb_hltcod;		/* VAXstar halt code register */
	u_long	nb_mser;		/* memory system error register	*/
					/* (not used on CVAXSTAR) */
	u_long	nb_mear;		/* memory error address register */
					/* (not used on CVAXSTAR) */
	u_char	nb_int_msk;		/* interrupt mask register */
	u_char	nb_vdc_org;		/* monochrome display origin */
	u_char	nb_vdc_sel;		/* video interrupt select register */
	u_char	nb_int_reqclr;		/* intr req (r/o), intr clr (w/o) */
	u_short	nb_diagdisp;		/* diagnostic display (w/o) */
					/* (CVAXSTAR only) */
	u_short	nb_pad60;
	u_long	nb_par_ctl;		/* parity control */
					/* (CVAXSTAR only) */
	u_long	nb_pad61;
	u_short	nb_pad62;
	u_short	nb_diagtime;		/* diagnostic timer */
					/* (CVAXSTAR only) */
	u_long	nb_pad1[4088];
	u_long	nb_cacr;		/* 2nd-level cache control */
					/* (CVAXSTAR only) */
	u_long	nb_pad63[12287];
	u_char	nb_narom[128];		/* network address rom ?????? */
	u_long	nb_pad2[16352];
	u_short	nb_sercsr;		/* serial line controller CSR */
	u_short	nb_pad3;
	u_short	nb_serrbuf_lpr;		/* SLU read bufffer/line parameter */
	u_short	nb_pad4;
	union {
		u_char	c[2];
		u_short	w;
	} nb_sertcr;
	u_short	nb_pad5;
	union	{
		u_char	c[2];
		u_short	w;
	} nb_sermsr_tdr;
	u_short	nb_pad6;
	u_long	nb_pad7[16380];
	u_short	nb_toysecs;		/* seconds			*/
	u_short	nb_pad8;
	u_short	nb_toysecs_alarm;	/* not used			*/
	u_short	nb_pad9;
	u_short	nb_toymins;		/* minutes			*/
	u_short	nb_pad10;
	u_short	nb_toymins_alarm;	/* not used			*/
	u_short	nb_pad11;
	u_short	nb_toyhours;		/* hours			*/
	u_short	nb_pad12;
	u_short	nb_toyhours_alarm;	/* not used			*/
	u_short	nb_pad13;
	u_short	nb_toyday_week;		/* not used			*/
	u_short	nb_pad14;
	u_short	nb_toyday;		/* day of month			*/
	u_short	nb_pad15;
	u_short	nb_toymonth;		/* month			*/
	u_short	nb_pad16;
	u_short	nb_toyyear;		/* year				*/
	u_short	nb_pad17;
	u_short	nb_toycsra;		/* csr				*/
	u_short	nb_pad18;
	u_short	nb_toycsrb;		/* csr				*/
	u_short	nb_pad19;
	u_short	nb_toycsrc;		/* csr				*/
	u_short	nb_pad20;
	u_short	nb_toycsrd;		/* csr				*/
	u_short	nb_pad21;
					/* NON-VOLATILE RAM		*/
	u_long	nb_cpmbx;		/* Console program mailbox	*/
	u_long	nb_cpflg;		/* Console program flags	*/
	u_long	nb_lk201_id;		/* LK201 keyboard variation	*/
	u_long	nb_console_id;		/* Console device type		*/
	u_long	nb_scr[4];		/* Scratch RAM physical address	*/
	u_long	nb_temp[12];		/* Used by System Firmware	*/
	u_long	nb_bat_chk[4];		/* Battery check data		*/
	u_long	nb_boot_dev[4];		/* Default boot device		*/
	u_long	nb_boot_flg[4];		/* Default boot flags		*/
	u_long	nb_scr_length;		/* # of pages of scratch RAM	*/
	u_long	nb_cpu_scsi_id;		/* System's SCSI bus 0 & 1 IDs  */
	u_long	nb_reserved[16];	/* reserved			*/
	u_long	nb_pad22[16320];
};


/*
 * TODO: for CVAXSTAR
 * Size of 2nd Level Cache: for cache flush operation
 */
#define CVS_CACHE_SIZE	(8*1024)	/* 8K longword (32K byte) cache */

struct	nb6_regs {
	u_long	nb_cvscache[CVS_CACHE_SIZE];
};

/* TODO: ext mode - temp */
struct nb7_regs {
	u_char	nb_eddb[128*1024];
};

/*
 * VAXstar NVR bit definitions for
 * only those fields which are used.
 * Each byte of NVR occupies bits 2 thru 9 of a longword.
 */

/*
 * Console program flags,
 * actual bit positions (not shifted by the code).
 */
#define	VS_CPF_CRT	0x8		/* Console is CRT (not hardcopy) */
#define	VS_CPF_VIDEO	0x80		/* video display (not a terminal) */

/*
 * Console type ID value,
 * after shifted right two bits by the code.
 */
#define	VS_CID_UNKNOWN	0x0		/* Console type is unknown */
#define	VS_CID_DIAG3	0x1		/* Diagnostic console (SLU line 3) */
#define	VS_CID_TERM0	0x2		/* Attached terminal (SLU line 0) */
#define	VS_CID_BITMAP	0xb0		/* Base monochrome bitmapped display */
#define	VS_CID_COLOR	0xd0		/* Optional dragon color display */

struct nb1_regs {
	u_char	nb_dkc_reg;	/* Disk - register data access */
	u_char	nb_pad23[3];
	u_char	nb_dkc_cmd_stat; /* Disk - cntlr command & interupt status */
	u_char	nb_pad24[3];
	u_long	nb_pad25[30];
					/* SCSI controller chip registers */
	u_char	nb_scs_out_data;	/* (wo) Output data register */
#define	nb_scs_cur_data	nb_scs_out_data	/* (ro) Current data register */
	u_char	nb_pad26[3];
	u_char	nb_scs_ini_cmd;		/* (r/w) Initiator command register */
	u_char	nb_pad27[3];
	u_char	nb_scs_mode;		/* (r/w) Mode register */
	u_char	nb_pad28[3];
	u_char	nb_scs_tar_cmd;		/* (r/w) Target command register */
	u_char	nb_pad29[3];
	u_char	nb_cur_stat;		/* (ro) Current bus status register */
#define	nb_scs_sel_ena	nb_scs_cur_stat	/* (wo) Select enable register */
	u_char	nb_pad30[3];
	u_char	nb_scs_status;		/* (ro) Bus and status register */
#define	nb_scs_dma_send	nb_scs_status	/* (wo) Start DMA send action */
	u_char	nb_pad31[3];
	u_char	nb_scs_in_data;		/* (ro) Input data regsiter */
#define	nb_scs_data_trcv nb_scs_in_data	/* (wo) Start DMA target receive action */
	u_char	nb_pad32[3];
	u_char	nb_scs_dma_ircv;	/* (wo) Start DMA initiator rcv action */
#define	nb_scs_reset nb_scs_dma_ircv	/* (ro) Reset interrupt/error action */
	u_char	nb_pad33[3];
	union {				/* DMA address register */
		u_char	c[4];		/* 8-bit (wo) compatability mode */
		u_long	l;		/* 32-bit (rw) extended mode */
	} nb_scd_adr;
	u_long	nb_pad35[7];
	union {				/* DMA byte count register */
		u_short	w[2];		/* 16-bit (r/w) compatability mode */
		u_long	l;		/* 32-bit (r/w) extended mode */
	} nb_scd_cnt;
	u_char	nb_scd_dir;		/* (wo) DMA transfer direction */
	u_char	nb_pad37[3];
	u_long	nb_pad64[6];
	u_char	nb_stc_mode;		/* Storage controller mode */
					/* (CVAXSTAR only) */
	u_char	nb_pad65[3];
	u_long	nb_pad38[16327];
	u_char	nb_ddb[16384];		/* Disk cntlr - data buffer RAM */
	u_long	nb_pad39[12288];
	u_short	nb_ni_rdp;		/* Network cntlr register data port */
	u_short	nb_pad40;
	u_short	nb_ni_rap;		/* Network cntlr register address port */
	u_short	nb_pad41;
	u_long	nb_pad42[16382];
	u_short	nb_cur_cmd;		/* Cursor command register */
	u_short	nb_pad43;
	u_short	nb_cur_xpos;		/* Cursor X position */
	u_short	nb_pad44;
	u_short	nb_cur_ypos;		/* Cursor Y position */
	u_short	nb_pad45;
	u_short	nb_cur_xmin_1;		/* Region 1 left edge */
	u_short	nb_pad46;
	u_short	nb_cur_xmax_1;		/* Region 1 right edge */
	u_short	nb_pad47;
	u_short	nb_cur_ymin_1;		/* Region 1 top edge */
	u_short	nb_pad48;
	u_short	nb_cur_ymax_1;		/* Region 1 bottom edge */
	u_short	nb_pad49;
	u_long	nb_pad50[4];
	u_short	nb_cur_xmin_2;		/* Region 2 left edge */
	u_short	nb_pad51;
	u_short	nb_cur_xmax_2;		/* Region 2 right edge */
	u_short	nb_pad52;
	u_short	nb_cur_ymin_2;		/* Region 2 top edge */
	u_short	nb_pad53;
	u_short	nb_cur_ymax_2;		/* Region 2 bottom edge */
	u_short	nb_pad54;
	u_short	nb_load;		/* Cursor sprite pattern load */
	u_short	nb_pad55;
	u_long	nb_pad56[16368];
	u_long	nb_ni_rom[32768];		/* Network option board ROM */
	u_long	nb_pad57[32768];
	u_long	nb_vo_rom[65536];		/* Video option board ROM */
};

struct	nb2_regs {
	u_long	nb_bitmap[65536];		/* Monochrome video RAM */
						/* VS410=128kb   VS420=256kb */
};

/* TODO: need to map 128KB disk buffer */
/*
struct	nb3_regs {
	u_long	nb_co_map[0x8000000];		/* Video option board (color) */
/*
};
*/
struct	nb4_regs {
	u_long	mct_base;	/* Phys. addr. of main configuration. table */
	u_long	option1;	/* Phys. addr. of first option's firmware */
	u_long	option2;	/* Phys. addr. of second option's firmware */
	u_long	good64kb;	/* Phys. addr. of good main memory */
	u_long	bitmap;		/* Phys. addr. of memory bitmap */
	u_long	bitmap_length;	/* Length of memory bitmap in bytes */
	u_long	kb_good;	/* Status of LK201 keyboard */
	u_long	mouse_good;	/* Status of pointer device (mouse or tablet) */
	u_long	init_output;	/* Phys. addr. of init routine for console out*/
	u_long	reset_output;	/* Phys. addr.of reset routine for console out*/
	u_long	init_input;	/* Phys. addr. of init routine for console in */
	u_long	reset_input;	/* Phys. addr.of reset routine for console in */
	u_long	get_char;	/* Phys. addr.of "get_char" routine */
	u_long	put_char;	/* Phys. addr.of "put_char" routine */
	u_long	fbeep;		/* Phys. addr.of "beep" routine */
	u_long	cursor_max_row;	/* Max. char. row of console window rect. */
	u_long	cursor_column;	/* Current horizontal pos. */
	u_long	cursor_row;	/* Current vertical pos. */
	u_long	save_console;	/* Phys. addr. of save routine */
	u_long	restore_console;/* Phys. addr. of restore routine */
	u_long	st_flags;	/* System firmware selftest control word */
	u_long	st_loop_addr;	/* System firmware selftest loop address */
	u_long  pad1[94];       /* Undefined */
	u_long  vmb_version;    /* VMB version number */
	u_long  self_test_ver;  /* Self-test version */
	u_long  console_ver;    /* Console version */
};

/*
 * FireFox registers
 */
struct  nb5_regs {
        u_long  nb_legss[0x2000000];  /* Firefox LEGSS */
};

#endif /* LOCORE */

/* qb_mser */
#define	QBM_CD		0x300		/* memory error code	*/
#define QBM_NXM		0x80		/* nonexistant memory	*/
#define QBM_LPE		0x40		/* local memory parity	*/
#define QBM_QPE		0x20		/* q-bus parity error	*/
#define QBM_DMAQPE	0x10		/* dma q-bus parity	*/
#define QBM_LEB		0x8		/* lost error bit	*/
#define QBM_WRW		0x2		/* write wrong parity	*/
#define QBM_PENB	0x1		/* parity enable	*/
#define QBM_EMASK	0xf8		/* mask to isolate cause*/

/* toy csr */
#define QBT_UIP		0x80		/* update in progress	*/
#define QBT_SETA	0x20		/* set up divider	*/
#define	QBT_SETUP	0x80		/* stop			*/
#define QBT_SETB	0x6		/* binary and 24 hour	*/

/* uba_cnfgr */
#define	UBACNFGR_UBINIT	0x00040000	/* unibus init asserted */
#define	UBACNFGR_UBPDN	0x00020000	/* unibus power down */
#define	UBACNFGR_UBIC	0x00010000	/* unibus init complete */

/* uba_cr */
#define	UBACR_MRD16	0x40000000	/* map reg disable bit 4 */
#define	UBACR_MRD8	0x20000000	/* map reg disable bit 3 */
#define	UBACR_MRD4	0x10000000	/* map reg disable bit 2 */
#define	UBACR_MRD2	0x08000000	/* map reg disable bit 1 */
#define	UBACR_MRD1	0x04000000	/* map reg disable bit 0 */
#define	UBACR_IFS	0x00000040	/* interrupt field switch */
#define	UBACR_BRIE	0x00000020	/* BR interrupt enable */
#define	UBACR_USEFIE	0x00000010	/* UNIBUS to SBI error field IE */
#define	UBACR_SUEFIE	0x00000008	/* SBI to UNIBUS error field IE */
#define	UBACR_CNFIE	0x00000004	/* configuration IE */
#define	UBACR_UPF	0x00000002	/* UNIBUS power fail */

/* uba_sr */
#define	UBASR_BR7FULL	0x08000000	/* BR7 receive vector reg full */
#define	UBASR_BR6FULL	0x04000000	/* BR6 receive vector reg full */
#define	UBASR_BR5FULL	0x02000000	/* BR5 receive vector reg full */
#define	UBASR_BR4FULL	0x01000000	/* BR4 receive vector reg full */
#define	UBASR_RDTO	0x00000400	/* UNIBUS to SBI read data timeout */
#define	UBASR_RDS	0x00000200	/* read data substitute */
#define	UBASR_CRD	0x00000100	/* corrected read data */
#define	UBASR_CXTER	0x00000080	/* command transmit error */
#define	UBASR_CXTMO	0x00000040	/* command transmit timeout */
#define	UBASR_DPPE	0x00000020	/* data path parity error */
#define	UBASR_IVMR	0x00000010	/* invalid map register */
#define	UBASR_MRPF	0x00000008	/* map register parity failure */
#define	UBASR_LEB	0x00000004	/* lost error */
#define	UBASR_UBSTO	0x00000002	/* UNIBUS select timeout */
#define	UBASR_UBSSYNTO	0x00000001	/* UNIBUS slave sync timeout */

#define	UBASR_BITS \
"\20\13RDTO\12RDS\11CRD\10CXTER\7CXTMO\6DPPE\5IVMR\4MRPF\3LEB\2UBSTO\1UBSSYNTO"

/* uba_brrvr[] */
#define	UBABRRVR_AIRI	0x80000000	/* adapter interrupt request */
#define	UBABRRVR_DIV	0x0000ffff	/* device interrupt vector field */
 
/* uba_dpr */
#define	UBADPR_BNE	0x80000000	/* buffer not empty - purge */
#define	UBADPR_BTE	0x40000000	/* buffer transfer error */
#define	UBADPR_DPF	0x20000000	/* DP function (RO) */
#define	UBADPR_BS	0x007f0000	/* buffer state field */
#define	UBADPR_BUBA	0x0000ffff	/* buffered UNIBUS address */
#define	UBA_PURGE780(uba, bdp){ \
	((uba)->uba_dpr[bdp] |= UBADPR_BNE);\
}


#define	UBACR_ADINIT	0x00000001	/* adapter init */

#define	UBADPR_ERROR	0x80000000	/* error occurred */
#define	UBADPR_NXM	0x40000000	/* nxm from memory */
#define	UBADPR_UCE	0x20000000	/* uncorrectable error */
#define	UBADPR_PURGE	0x00000001	/* purge bdp */
/* the DELAY is for a hardware problem */
#define	UBA_PURGE750(uba, bdp) { \
    ((uba)->uba_dpr[bdp] |= (UBADPR_PURGE|UBADPR_NXM|UBADPR_UCE)); \
    DELAY(8); \
}

#define	BUA_PURGE8200(uba, bdp) { \
    (((struct bua_regs *) uba)->bua_dpr[bdp] |= (BUADPR_PURGE)); \
}

/*
 * Macros for fast buffered data path purging in time-critical routines.
 *
 * Too bad C pre-processor doesn't have the power of LISP in macro
 * expansion...
 */

#define	UBAPURGE(uba, bdp, ubanum) { \
	int ubatype = uba_hd[ubanum].uba_type; \
	if(ubatype&UBABUA) BUA_PURGE8200((uba), (bdp)) \
	if(ubatype&UBA780) UBA_PURGE780((uba), (bdp)) \
	if(ubatype&UBA750) UBA_PURGE750((uba), (bdp)) \
}


/* uba_mr[] */
#define	UBAMR_MRV	0x80000000	/* map register valid */
#define	UBAMR_BO	0x02000000	/* byte offset bit */
#define	UBAMR_DPDB	0x01e00000	/* data path designator field */
#define	UBAMR_SBIPFN	0x000fffff	/* SBI page address field */

#define	UBAMR_DPSHIFT	21		/* shift to data path designator */

/*
 * Number of UNIBUS map registers.  We can't use the last 8k of UNIBUS
 * address space for i/o transfers since it is used by the devices,
 * hence have slightly less than 256K of UNIBUS address space.
 */
#define	NUBMREG	496
/*
 * Number of Q-BUS mapping registers. We actually use just the first 496
 * because the device drivers know about the format of ubinfo which is
 * based on an 18-bit unibus address. To change this would require changes
 * to most of the device drivers.
 */
#define QBMREG 8192
/*
 * All systems now have an 8k csr space. If this changes it should be put
 * into cpusw or some other structure.
 */
#define DEVSPACESIZE 8192

/*
 * Number of unibus buffered data paths and possible uba's per cpu type.
 */

#define NBDP8200 5
#define NBDP8800 5
#define	NBDP8600	15
#define	NBDP780	15
#define	NBDP750	3
#define	NBDP730	0
#define NBDPUVI 0
#define	MAXNBDP	15

#define NUBA8600 8
#define NUBA8200 10
#define NUBA8800 10
#define NUBA780 4
#define	NUBA750	1
#define	NUBA730	1
#define NUBAUVI 1

/*
 * Symbolic BUS addresses for UBAs and QBUSes.
 *
 */

#define	UMEM730		((u_short *)(0xfc0000))
#define UMEMSIZE730	(512*496)
#define UDEVADDR730	((u_short *)(0xfc0000+0760000))

#define	UMEM750(i)	((u_short *)(0xfc0000-(i)*0x40000))
#define UMEMSIZE750	(512*496)
#define UDEVADDR750(i)	((u_short *)(0xfc0000-(i)*0x40000+0760000))

#define	UMEM780(i)	((u_short *)(0x20100000+(i)*0x40000))
#define UMEMSIZE780	(512*496)
#define UDEVADDR780(i)	((u_short *)(0x20100000+(i)*0x40000+0760000))

#define	UMEM8200(i)	((u_short *)(0x20400000+(i)*0x40000))
#define UMEMSIZE8200	(512*496)
#define UDEVADDR8200(i)	((u_short *)(0x20400000+(i)*0x40000+0760000))

#define	UMEM8800(io,i)	((u_short *)(0x20400000+(i)*0x40000+(io<<25)))
#define UMEMSIZE8800	(512*496)
#define UDEVADDR8800(io,i) ((u_short *)(0x20400000+(i)*0x40000+0760000+(io<<25)))

#define	UMEM8600(io,i)	((u_short *)(0x20100000+(i)*0x40000+(io<<25)))
#define UMEMSIZE8600	(512*496)
#define UDEVADDR8600(io,i) ((u_short *)(0x20100000+(i)*0x40000+0760000+(io<<25)))

#define UMEMSIZE9000	(512*496)

#define QMEMUVI		((char *)(0))
#define QMEMUVII	((char *)(0x30000000))
/*
 * Following allows VAXstar to use the QMEMmap
 * to access the second chunk if its I/O space.
 */
#define	QMEMVAXSTAR	((char *)(0x200c0000))
#define	QMEMSIZEVS	0xc0000
/*
 * Maps third chunk of VAXstar I/O space.
 */
#define	NMEMVAXSTAR	((char *)(0x30000000))
#define	NMEMSIZEVS	0x20000
#define	NMEMSIZECVS	0x40000
/*
 * Maps fourth chunk of VAXstar I/O space,
 * size may grow for color option.
 */
#define	SGMEMVAXSTAR	((char *)(0x3c000000))
#define	SGMEMSIZEVS	0x18000
/*
 * Maps fifth chunk of VAXstar I/O space,
 * for MicroVAX 2000 serial line expander.
 */
#define	SHMEMVAXSTAR	((char *)(0x38000000))
#define	SHMEMSIZEVS	0x200
/*
 * Map for FireFox I/O space
 */
#define NMEMFG          ((char *)(0x3e000000))
#define NMEMSIZEFG      0x2000000
/*
 * TODO:
 * CVAXstar 2nd level cache data storage mapping
 */
#define	CVSCACHEADDR	((char *)0x10000000)
#define	CVSCACHESIZE	0x8000
/*
 * TODO: ext mode temp, disk data buffer
 */
#define	CVSEDDBADDR	((char *)0x202d0000)
#define	CVSEDDBSIZE	0x20000
/*
 * Maps CVAXstar (VAX420) SCSI registers.
 */
#define	SZMEMCVAXSTAR	((char *)(0x200c0000))
#define	SZMEMSIZECVS	0x400
/*
 * The q-bus memory size is 4 meg plus the space for the csr's
 */
#define QMEMSIZEUVI	(512*8192)
#define QDEVADDRUVI	((u_short *)(0x20000000))

/*
 * Macro to offset a UNIBUS device address, often expressed as
 * something like 0172520 by forcing it into the last 8K of UNIBUS memory
 * space.
 */
#define	ubdevreg(addr)	((addr)&017777)

/* set aside 512 map registers (the first ones) for backward compat. with
 * ubasetup() calls.
 */
#define QBNOTUB 512
#define QBREGMASK	0x1fff
#define NQBREGMASK 	0x3ff

