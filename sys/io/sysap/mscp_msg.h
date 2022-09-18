/*	@(#)mscp_msg.h	4.1	(ULTRIX)	7/2/90	*/
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1987, 1989 by                     *
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
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *		MSCP-speaking Class Drivers
 *
 *   Abstract:	This module contains all the MSCP message definitions
 *
 *   Author:	David E. Eiche	Creation Date:	December 1, 1984
 *
 *   History:
 *
 *   01-May-1990		Mike Thomas
 *	DECwest ANSI mt 1990 May 01
 *	Replaced MSCP_def.H with MSCP_def_H for ANSI.
 *
 *   31-Oct-1989	Tim Burke
 *	Added MSCP_SC_AVAIL to the table of unit available subcode values.
 *
 *   21-Jul-1989	David E. Eiche		DEE0070
 *	1.  Recode the MSLG structure to make it maintainable.
 *	2.  Change field names in the MSCP and MSLG structures to
 *	    conform to the T/MSCP specification preferred mnemonics
 *	    and provide additional field names for the convenience
 *	    of the error formatter program.
 *	3.  Eliminate defines starting with "tmscp".
 *	4.  Correct the offsets of the mscp_excl_lba, mscp_excl_lbc,
 *	    and mscp_dev_parm fields.
 *	5.  Add structures for the DISPLAY and FMT commands.
 *	6.  Add subcodes for additional media loader errors.
 *	7.  Incorporate other miscellaneous changes to reflect the
 *	    latest round of T/MSCP ECOs.
 *	8.  Reformatted previous audit trail entries so that the
 *	    audit trail editor macros can be used on them.
 *
 *   20-Jul-89		Mark A. Parenti
 *	Add KDM70 definition. Remove HSB50.
 *
 *   20-Jun-89		Tim Burke
 *	Spec updates mainly for loader support (TQK7L) changes include:
 *	Added MSCP_UF_LOADR to table A-5.
 *	In table B-1 change MSCP_ST_LOADR from x15 to x17, added
 *	MSCP_ST_IPARM.  Added MSCP_SC_LOADE to "Unit-Offline" Subcode
 *	values.  Added additional "media loader error" status subcodes
 *	to table B-10 (subcodes 0x5 to 0x12).
 *
 *   08-May-1989	Tim Burke
 *	Brought forward 3.1 changes into pu and isis pools.
 *
 *   28-Dec-1988	Tim Burke
 *	Updates to t/mscp specs for the following tables:
 *	A-9, A-10, B-1, B-10, B-11
 *	These changes mainly involve definitions for loader and
 *	informational error logs.
 *
 *   07-May-1988	Robin
 *	Changed the define value for MSCP_TF_NOR from 0 to the
 *	correct value of 1.
 *
 *   15-Feb-1988	Robin
 *	Changed the define of mslg_position to mslg_tape_position to
 *	resolve a multi-define problem seen in eribld.c
 *
 *   02-Feb-1988	Robin
 *	Added new mscp devices like RF30
 *
 *   02-Jan-1988	Robin
 *	Changed the name for RV80 to RV20, the real name was changed.
 *
 *   16-Dec-1986	Robin
 *	Fixed the error log macro for position and vol_ser
 *	mslg_position changed to Mslg_position.
 */
/**/

/* If included previously don't include it again.
 * Test a known define (MSCP_def_H) and if it's NOT defined, define everything,
 * otherwise don't define anything.
 */
#ifndef MSCP_def_H
#define MSCP_def_H	0

/* mscp.h Mass Storage Control Protocol 84/12/01
 */
#define	MSCP_VERSION	0	/* Current MSCP protocol version	     */


/* MSCP control message opcodes (from MSCP specification Table A-1)
 */
#define MSCP_OP_ABORT	0x01	/* ABORT 				     */
#define MSCP_OP_ACCESS	0x10	/* ACCESS 				     */
#define MSCP_OP_ACCNM	0x05	/* ACCESS NON-VOLATILE MEMORY 		     */
#define MSCP_OP_AVAIL	0x08	/* AVAILABLE 				     */
#define MSCP_OP_CMPCD	0x11	/* COMPARE CONTROLLER DATA 		     */
#define MSCP_OP_COMP	0x20	/* COMPARE HOST DATA 			     */
#define MSCP_OP_DTACP	0x0b	/* DETERMINE ACCESS PATHS 		     */
#define MSCP_OP_ERASE	0x12	/* ERASE 				     */
#define MSCP_OP_ERGAP	0x16	/* ERASE GAP (T)			     */
#define MSCP_OP_FLUSH	0x13	/* FLUSH 				     */
#define MSCP_OP_FMT	0x18	/* FORMAT				     */
#define MSCP_OP_GTCMD	0x02	/* GET COMMAND STATUS 			     */
#define MSCP_OP_GTUNT	0x03	/* GET UNIT STATUS 			     */
#define MSCP_OP_ONLIN	0x09	/* ONLINE 				     */
#define MSCP_OP_RCEDC	0x23	/* READ CONTROLLER ENCRYPTION/DECRYPTION
				    CODE 				     */
#define MSCP_OP_READ	0x21	/* READ 				     */
#define MSCP_OP_REPLC	0x14	/* REPLACE 				     */
#define MSCP_OP_REPOS	0x25	/* REPOSITION (T)			     */
#define MSCP_OP_SPEC1	0x28	/* Manufacturing reserved command 1	     */
#define MSCP_OP_SPEC2	0x29	/* Manufacturing reserved command 2	     */
#define MSCP_OP_SPEC3	0x2a	/* Manufacturing reserved command 3	     */
#define MSCP_OP_SPEC4	0x2b	/* Manufacturing reserved command 4	     */
#define MSCP_OP_SPEC5	0x2c	/* Manufacturing reserved command 5	     */
#define MSCP_OP_SPEC6	0x2d	/* Manufacturing reserved command 6	     */
#define MSCP_OP_SPEC7	0x2e	/* Manufacturing reserved command 7	     */
#define MSCP_OP_SPEC8	0x2f	/* Manufacturing reserved command 8	     */
#define MSCP_OP_STCON	0x04	/* SET CONTROLLER CHARACTERISTICS	     */
#define MSCP_OP_STUNT	0x0a	/* SET UNIT CHARACTERISTICS 		     */
#define MSCP_OP_WRITE	0x22	/* WRITE 				     */
#define MSCP_OP_WRITM	0x24	/* WRITE TAPE MARK (T)			     */
#define MSCP_OP_END	0x80	/* End message flag (endmsg == ctlmsg | END) */
#define MSCP_OP_SEREX	0x07	/* Serious exception end message	     */
#define MSCP_OP_AVATN	0x40	/* AVAILABLE attention message		     */
#define MSCP_OP_DUPUN	0x41	/* DUPLICATE UNIT NUMBER attention message   */
#define MSCP_OP_ACPTH	0x42	/* ACCESS PATH attention message	     */
#define MSCP_OP_RWATN	0x43	/* REWIND attention message (T)		     */


/* MSCP command modifiers (from MSCP specification Table A-2)
 */

/*
 * Generic command modifiers
 */
#define MSCP_MD_COMP	0x4000	/* Compare				     */
#define MSCP_MD_EXPRS	0x8000	/* Express request			     */
#define MSCP_MD_CLSEX	0x2000	/* Clear serious exception		     */
#define MSCP_MD_CDATL	0x1000	/* Clear cache data lost exception	     */
#define MSCP_MD_ERROR	0x1000	/* Force error				     */
#define MSCP_MD_SCCHH	0x0800	/* Suppress caching (high speed)	     */
#define MSCP_MD_SCCHL	0x0400	/* Suppress caching (low speed)		     */
#define MSCP_MD_SECOR	0x0200	/* Suppress error correction		     */
#define MSCP_MD_SEREC	0x0100	/* Suppress error recovery		     */
#define MSCP_MD_WBKNV	0x0040	/* Write-back non volatile		     */
#define MSCP_MD_WBKVL	0x0020	/* Write-back volatile memory		     */
#define MSCP_MD_UNLOD	0x0010	/* Unload				     */
#define MSCP_MD_REVRS	0x0008	/* Reverse				     */
#define MSCP_MD_IMMED	0x0040	/* Immediate Completion			     */


/* AVAILABLE command modifiers
 */
#define MSCP_MD_DSOLV	0x0004	/* Dissolve shadow set			     */
#define MSCP_MD_ALLCD	0x0002	/* All class drivers			     */
#define MSCP_MD_SPNDW	0x0001	/* Spin-down				     */


/* FLUSH command modifiers
 */
#define MSCP_MD_VOLTL	0x0002	/* Volatile only			     */
#define MSCP_MD_FLENU	0x0001	/* Flush entire unit			     */


/* GET UNIT STATUS command modifiers
 */
#define MSCP_MD_NXUNT	0x0001	/* Next unit				     */


/* ONLINE command modifiers
 */
#define MSCP_MD_IGNMF	0x0002	/* Ignore media format			     */
#define MSCP_MD_RIP	0x0001	/* Allow self destruction		     */


/* ONLINE and SET UNIT CHARACTERISTICS command modifiers
 */
#define MSCP_MD_EXCAC	0x0020	/* Exclusive access			     */
#define MSCP_MD_SHDSP	0x0010	/* Shadow unit specified		     */
#define MSCP_MD_CLWBL	0x0008	/* Clear write-back data lost		     */
#define MSCP_MD_STWRP	0x0004	/* Enable set write protect		     */


/* REPLACE command modifiers
 */
#define MSCP_MD_PRIMR	0x0001	/* Primary replacement block		     */

/* REPOSITION command modifiers
 */
#define MSCP_MD_DLEOT	0x0080	/* Detect LEOT				     */
#define MSCP_MD_OBJCT	0x0004	/* Object count				     */
#define MSCP_MD_REWND	0x0002	/* Rewind				     */

/* SET CONTROLLER CHARACTERISTICS command modifiers
 */
#define MSCP_MD_FKC	0x0001	/* Flush encryption/decryption key cache */

/* WRITE command modifiers
 */
#define MSCP_MD_ENRWR	0x0010	/* Enable Re-Write Error Recovery	     */

/* MSCP End message flags (from MSCP specification Table A-3)
 */
#define MSCP_EF_BBLKR	0x0080	/* Bad block reported (disks only)	     */
#define MSCP_EF_BBLKU	0x0040	/* Bad block unreported (disks only)	     */
#define MSCP_EF_ERLOG	0x0020	/* Error log generated			     */
#define MSCP_EF_SEREX	0x0010	/* Serious Exception (tapes only)	     */
#define MSCP_EF_EOT	0x0008	/* EOT encountered (tapes only)		     */
#define MSCP_EF_PLS	0x0004	/* Position lost (tapes only)		     */
#define MSCP_EF_DLS	0x0002	/* Cache Data Lost(tapes only)		     */


/* MSCP Controller flags (from MSCP specification Table A-4)
 */
#define MSCP_CF_REPLC	0x8000	/* Controller initiated BBR		     */
#define MSCP_CF_EDCRP	0x4000	/* Data encrypt/decrypt			     */
#define MSCP_CF_ATTN	0x0080	/* Enable attention messages		     */
#define MSCP_CF_MISC	0x0040	/* Enable miscellaneous error log msgs	     */
#define MSCP_CF_OTHER	0x0020	/* Enable other host's error log msgs	     */
#define MSCP_CF_THIS	0x0010	/* Enable this host's error log msgs	     */
#define MSCP_CF_MLTHS	0x0004	/* Multi-host support			     */
#define MSCP_CF_SHADW	0x0002	/* Shadowing				     */
#define MSCP_CF_576	0x0001	/* 576 byte sectors			     */

/* MSCP unit flags (from MSCP specification Table A-5).
 */
#define MSCP_UF_REPLC	0x8000	/* Controller initiated BBR		     */
#define MSCP_UF_CACH	0x8000	/* Set if caching is there		     */
#define MSCP_UF_SSMEM	0x4000	/* Shadow set member			     */
#define MSCP_UF_WRTPH	0x2000	/* Write protect (hardware)		     */
#define MSCP_UF_WRTPS	0x1000	/* Write protect (software)		     */
#define MSCP_UF_SCCHH	0x0800	/* Suppress caching 			     */
#define MSCP_UF_EXACC	0x0400	/* Exclusive access			     */
#define MSCP_UF_SSMST	0X0200	/* Shadow set master			     */
#define MSCP_UF_LOADR	0X0200	/* Media loader present (T)		     */
#define MSCP_UF_WRTPD	0x0100	/* Write protect (data safety)		     */
#define MSCP_UF_RMVBL	0x0080	/* Removable Media			     */
#define MSCP_UF_WBKNV	0x0040	/* Write-back (non-volatile)		     */
#define MSCP_UF_VSMSU	0x0020	/* Variable Speed Mode Suppression	     */
#define MSCP_UF_VARSP	0x0010	/* Variable Speed Unit			     */
#define MSCP_UF_EWRER	0x0008	/* Enhanced Write Error Recovery 	     */
#define MSCP_UF_576	0x0004	/* 576 byte sectors			     */
#define MSCP_UF_CACFL	0x0004	/* Write-back cache has been flushed	     */
#define MSCP_UF_CMPWR	0x0002	/* Compare writes			     */
#define MSCP_UF_CMPRD	0x0001	/* Compare reads			     */

/* MSCP command, end, and attention message offsets
 * (from MSCP specification tables A-6 and A-7).
 */
typedef union _mscp {

    /* Generic command message offsets
     */
    struct _cm {
	u_long	cmd_ref;			/* Command reference number  */
	u_short	unit;				/* Unit number		     */
	u_short		:16;			/* Reserved		     */
	u_char	opcode;				/* Opcode		     */
	u_char		:8;			/* Reserved		     */
	u_short	modifier;			/* Command modifiers	     */
	union {

	    /* Overlaid generic command message offsets
	     */
	    struct _gen_cm {
		u_long	byte_cnt;		/* Byte count		     */
		u_long	buffer[ 3 ];		/* Buffer descriptor	     */
		u_long	lbn;			/* Logical block number	     */
	    } gen;

	    /* ABORT and GET COMMAND STATUS command message offsets
	     */
	    struct _abo_cm {			/* Command offsets for ABORT */
		u_long	out_ref;		/* Outstanding reference no. */
	    } abo;

	    /* ACCESS NON-VOLATILE MEMORY command message offsets
	     */
	    struct _anm_cm {
		u_long		:32;		/* Reserved		     */
		u_long	anm_offs;		/* Offset in NVM	     */
		u_short	anm_oper;		/* Operation (see A-12)	     */
		u_short	anm_dlgh;		/* Data length		     */
		u_char	anm_memd[ 4 ];		/* NVM data (variable)	     */
	    } anm;

	    /* ONLINE and SET UNIT CHARACTERISTICS command message offsets
	     */
	    struct _onl_cm {
		u_short		:16;		/* Reserved		     */
		u_short	unt_flgs;		/* Unit flags		     */
		u_long		:32;		/* Reserved		     */
		union {
		    struct {			/* Disk variant		     */
			u_long	excl_lba;	/* Excluded LBN address	     */
			u_long	excl_lbc;	/* Excluded LBN count	     */
			u_long	dev_parm;	/* Device parameters	     */
			u_short	shdw_unt;	/* Shadow unit number	     */
			u_short	copy_mod;	/* Copy mode		     */
		    } d;
		    struct {			/* Tape variant		     */
			u_long		:32;	/* Reserved		     */
			u_long		:32;	/* Reserved		     */
			u_long	dev_parm;	/* Device parameters	     */
			u_short	format;		/* Format		     */
			u_short	speed;		/* Speed		     */
		    } t;
		} u1;
	    } onl;

	    /* REPOSITION command message offsets
	     */
	    struct _rpo_cm {
		u_long	rec_cnt;		/* Record count/object count */
		u_long	tmgp_cnt;		/* Tape mark count	     */
	    } rpo;

	    /* REPLACE command message offsets
	     */
	    struct _rep_cm {
		u_long	rbn;			/* Replacement block number  */
	    } rep;

	    /* set controller characteristics command message offsets
	     */
	    struct _scc_cm {
		u_short	version;		/* MSCP version		     */
		u_short	cnt_flgs;		/* Controller flags	     */
		u_short	hst_tmo;		/* Host timeout		     */
		u_short		:16;		/* Reserved		     */
		u_long	time[ 2 ];		/* Quadword date/time	     */
		u_long	cnt_parm;		/* Cnt-dependent parameters  */
	    } scc;

	    /* FORMAT command message offsets
	     */
	    struct _fmt_cm {
		u_long	byte_cnt;		/* Byte count		     */
		u_long	buffer[ 3 ];		/* Buffer descriptor	     */
		u_long	fmt_func;		/* Format function code	     */
	    } fmt;

	    /* DISPLAY command message offsets
	     */
	    struct _dpy_cm {
		u_short	ditem;			/* Item code		     */
		u_short	dmode;			/* Mode			     */
		u_char	dtext[ 16 ];		/* Display text		     */
	    } dpy;
	} u0;
    } cm; 

    /* Generic end message offsets
     */
    struct _em {
	u_long	cmd_ref;			/* Command reference number  */
	u_short	unit;				/* Unit number		     */
	u_short	seq_num;			/* Last err log sequence num */
	u_char	endcode;			/* Endcode		     */
	u_char	flags;				/* End message flags	     */
	u_short	status;				/* Status		     */
	union {

	    /* Overlaid generic end message offsets
	     */
	    struct _gen_em {
		u_long	byte_cnt;		/* Byte count		     */
		u_long		:32;		/* Reserved		     */
		u_long		:32;		/* Reserved		     */
		u_long		:32;		/* Reserved		     */
		u_long	frst_bad;		/* First bad block	     */
	    } gen;

	    /* ABORT and GET COMMAND STATUS end message offsets
	     */
	    struct _abo_em {
		u_long	out_ref;		/* Outstanding reference no. */
		u_long	cmd_sts;		/* Command status	     */
	    } abo;

	    /* ACCESS NON-VOLATILE MEMORY end message offsets
	     */
	    struct _anm_em {
		u_long	anm_size;		/* NVM size		     */
		u_long	anm_offs;		/* Offset in NVM	     */
		u_short	anm_oper;		/* Operation (see A-12)	     */
		u_short	anm_dlgh;		/* Data length		     */
		u_long	anm_memd;		/* NVM data (variable)	     */
	    } anm;			

	    /* GET UNIT STATUS end message offsets
	     */
	    struct _gus_em {
		u_short	mult_unt;		/* Multi-unit code	     */
		u_short	unt_flgs;		/* Unit flags		     */
		u_long		:32;		/* Reserved		     */
		u_long	unit_id[ 2 ];		/* Unit identifier	     */
		u_long	media_id;		/* Media identifier	     */
		union {
		    struct {
			u_short	shdw_unt;	/* Shadow unit		     */
			u_short	shdw_sts;	/* Shadow status	     */
			u_short	track;		/* Track size		     */
			u_short	group;		/* Group size		     */
			u_short	cylinder;	/* Cylinder size	     */
			u_char	unit_svr;	/* Unit software version */
			u_char	unit_hvr;	/* Unit hardware version */
			u_short	rct_size;	/* RCT table size	     */
			u_char	rbns;		/* RBNs per track	     */
			u_char	rct_cpys;	/* RCT copies		     */
		    } d;
		    struct {
			u_short format;		/* Density in bpi	     */
			u_short speed;		/* Instantaneous data rate   */
			u_short formenu;	/* Format menu		     */
			u_char	freecap;	/* Free capacity	     */
			u_char		:8;	/* Reserved		     */
			u_char	fmtr_svr;	/* Formater software version */
			u_char	fmtr_hvr;	/* Formater hardware version */
			u_char	unit_svr;	/* Unit software version */
			u_char	unit_hvr;	/* Unit hardware version */
		    } t;
		} u1;
	    } gus;

	    /* ONLINE and SET UNIT CHARACTERISTICS end message and
	     * AVAILABLE attention message offsets.
	     */
	    struct _onl_em {
		u_short	mult_unt;		/* Multi-unit code	     */
		u_short	unt_flgs;		/* Unit flags		     */
		u_long		:32;		/* Reserved		     */
		u_long	unit_id[ 2 ];		/* Unit identifier	     */
		u_long	media_id;		/* Media identifier	     */
		union {
		    struct {
			u_short	shdw_unt;	/* Shadow unit		     */
			u_short	shdw_sts;	/* Shadow status	     */
			u_long	unt_size;	/* Unit size		     */
			u_long	vol_ser;	/* Volume serial number	     */
		    } d;
		    struct {
			u_short format;		/* Density in bpi	     */
			u_short speed;		/* Instantaneous data rate   */
			u_long  maxwtrec;	/* Max write byte count      */
			u_short noiserec;	/* Max noise record size     */
			u_short		:16;	/* Reserved		     */
		    } t;
		} u1;
	    } onl;

	    /* SET CONTROLLER CHARACTERISTICS end message offsets.
	     */
	    struct _scc_em {
		u_short	version;		/* MSCP version		     */
		u_short	cnt_flgs;		/* Controller flags	     */
		u_short	cnt_tmo;		/* Controller timeout	     */
		u_char	cnt_svr;		/* Ctrlr software version    */
		u_char	cnt_hvr;		/* Ctrlr hardware version    */
		u_long	cnt_id[ 2 ];		/* Controller ID	     */
		u_long	max_bcnt;		/* Ctrlr maximum byte count  */
	    } scc;

	    /* Tape ACCESS, COMPARE HOST DATA, FLUSH, READ,
	     * WRITE and WRITE TAPE MARK end message offsets.
	     */
	    struct _acc_em {
                u_long  byte_cnt;		/* Host transfer             */
		u_long		:32;		/* Reserved		     */
		u_long		:32;		/* Reserved		     */
		u_long		:32;		/* Reserved		     */
                u_long  position;		/* Object count              */
                u_long  taperec;		/* Tape record byte count    */
	    } acc;

	    /* REPOSITION end message offsets.
	     */
	    struct _rpo_em {
		u_long  rcskiped;		/* Records skipped/undefined */
		u_long  tmskiped;		/* Tape marks skipped/undef  */
		u_long		:32;		/* Reserved		     */
		u_long		:32;		/* Reserved		     */
		u_long	position;		/* Object count 	     */
	    } rpo;
	} u0;
    } em;
} MSCP;

typedef struct _cm	MSCP_CMDMSG;
typedef struct _em	MSCP_ENDMSG;


/* Redefine cells inside inner unions and structures to eliminate
 * multiple qualification levels.
 */
#define	mscp_anm_offs	cm.u0.anm.anm_offs	/* Offset in NVM	     */
#define	mscp_anm_oper	cm.u0.anm.anm_oper	/* Operation (see A-12)	     */
#define	mscp_anm_dlgh	cm.u0.anm.anm_dlgh	/* Data length		     */
#define	mscp_anm_memd	cm.u0.anm.anm_memd	/* NVM data		     */
#define mscp_anm_size	em.u0.anm.anm_size	/* NVM size		     */
#define mscp_buffer	cm.u0.gen.buffer	/* Buffer descriptor	     */
#define mscp_byte_cnt	cm.u0.gen.byte_cnt	/* Byte count		     */
#define mscp_cmd_ref	cm.cmd_ref		/* Command reference number  */
#define mscp_cmd_sts	em.u0.abo.cmd_sts	/* Command status	     */
#define mscp_cnt_flgs	cm.u0.scc.cnt_flgs	/* Controller flags	     */
#define mscp_cnt_hvr	em.u0.scc.cnt_hvr	/* Hardware version	     */
#define mscp_cnt_id	em.u0.scc.cnt_id	/* Controller ID	     */
#define mscp_cnt_parm	cm.u0.scc.cnt_parm	/* Ctlr-dependent parms	     */
#define mscp_cnt_svr	em.u0.scc.cnt_svr	/* Software version	     */
#define mscp_cnt_tmo	em.u0.scc.cnt_tmo	/* Controller timeout	     */
#define mscp_copy_mod	cm.u0.onl.u1.d.copy_mod	/* Copy mode		     */
#define mscp_cylinder	em.u0.gus.u1.d.cylinder	/* Cylinder size	     */
#define mscp_dev_parm	cm.u0.onl.u1.d.dev_parm	/* Device parameters	     */
#define mscp_ditem	cm.u0.dpy.ditem		/* Item code		     */
#define mscp_dmode	cm.u0.dpy.dmode		/* Mode			     */
#define mscp_dtext	cm.u0.dpy.dtext		/* Display text		     */
#define mscp_endcode	em.endcode		/* Endcode aka opcode	     */
#define mscp_excl_lba	cm.u0.onl.u1.d.excl_lba	/* Excluded LBN address	     */
#define mscp_excl_lbc	cm.u0.onl.u1.d.excl_lbc	/* Excluded LBN count	     */
#define mscp_flags	em.flags		/* End message flags	     */
#define mscp_fmtr_hvr	em.u0.gus.u1.t.fmtr_hvr	/* Formatter h/w version     */
#define mscp_fmtr_svr	em.u0.gus.u1.t.fmtr_svr	/* Formatter s/w version     */
#define mscp_fmt_func	cm.u0.fmt.fmt_func	/* Format disk function code */
#define mscp_format	cm.u0.onl.u1.t.format	/* Tape format		     */
#define mscp_formenu	em.u0.gus.u1.t.formenu	/* Format menu		     */
#define mscp_freecap	em.u0.gus.u1.t.freecap	/* Tape media size function  */
#define mscp_frst_bad	em.u0.gen.frst_bad	/* First bad block	     */
#define mscp_group	em.u0.gus.u1.d.group	/* Group size		     */
#define mscp_hst_tmo	cm.u0.scc.hst_tmo	/* Host timeout		     */
#define mscp_lbn	cm.u0.gen.lbn		/* Logical block number	     */
#define mscp_max_bcnt	em.u0.scc.max_bcnt	/* Maximum byte count	     */
#define mscp_maxwtrec	em.u0.onl.u1.t.maxwtrec	/* Maximum WRITE record size */
#define mscp_media_id	em.u0.gus.media_id	/* Media identifier	     */
#define mscp_modifier	cm.modifier		/* Command modifier	     */
#define mscp_mult_unt	em.u0.gus.mult_unt	/* Multi-unit code	     */
#define mscp_noiserec	em.u0.onl.u1.t.noiserec	/* Maximum noise record size */
#define mscp_opcode	cm.opcode		/* Command opcode	     */
#define mscp_out_ref	cm.u0.abo.out_ref	/* Outstanding reference no. */
#define mscp_position	em.u0.acc.position	/* Tape position	     */
#define mscp_rbn	cm.u0.rep.rbn		/* Replacement block number  */
#define mscp_rbns	em.u0.gus.u1.d.rbns	/* RBNs per track	     */
#define mscp_rcskiped	em.u0.rpo.rcskiped	/* Records skipped	     */
#define mscp_rct_cpys	em.u0.gus.u1.d.rct_cpys	/* RCT copies		     */
#define mscp_rct_size	em.u0.gus.u1.d.rct_size	/* RCT size		     */
#define mscp_rec_cnt	cm.u0.rpo.rec_cnt	/* Record/object count	     */
#define mscp_seq_num	em.seq_num		/* Last err log sequence num */
#define mscp_status	em.status		/* End message status	     */
#define mscp_shdw_sts	em.u0.gus.u1.d.shdw_sts	/* Shadow status	     */
#define mscp_shdw_unt	cm.u0.onl.u1.d.shdw_unt	/* Shadow unit		     */
#define mscp_speed	cm.u0.onl.u1.t.speed	/* Speed		     */
#define mscp_taperec	em.u0.acc.taperec	/* Tape record byte count    */
#define mscp_time	cm.u0.scc.time		/* Quadword date/time	     */
#define mscp_tmgp_cnt	cm.u0.rpo.tmgp_cnt	/* Tape mark count	     */
#define mscp_tmskiped	em.u0.rpo.tmskiped	/* Tape marks skipped	     */
#define mscp_track	em.u0.gus.u1.d.track	/* Track size		     */
#define mscp_unit	cm.unit			/* Unit number		     */
#define mscp_unit_hvr	em.u0.gus.u1.d.unit_hvr	/* Unit hardware version     */
#define mscp_unit_id	em.u0.gus.unit_id	/* Unit identifier	     */
#define mscp_unit_svr	em.u0.gus.u1.d.unit_svr	/* Unit software version     */
#define mscp_unt_flgs	cm.u0.onl.unt_flgs	/* Unit flags		     */
#define mscp_unt_size	em.u0.onl.u1.d.unt_size	/* Unit size		     */
#define mscp_version	cm.u0.scc.version	/* MSCP version		     */
#define mscp_vol_ser	em.u0.onl.u1.d.vol_ser	/* Volume serial number	     */


/* MSCP error log message offsets (from MSCP specification Table A-8)
 */
typedef struct	_mslg {
    struct _lg {
	u_long	cmd_ref;			/* Command reference number  */
	u_short	unit;				/* Unit number		     */
	u_short	seq_num;			/* Sequence number	     */
	u_char	format;				/* Format		     */
	u_char	flags;				/* Error log message flags   */
	u_short	event;				/* Event code		     */
	u_char	cnt_id[ 8 ];			/* Controller ID	     */
	u_char	cnt_svr;			/* Controller s/w version    */
	u_char	cnt_hvr;			/* Controller h/w version    */
	union {
	    u_short	mult_unt;		/* Multi-unit code	     */
	    u_char	cnt_err;		/* Cntlr error (1st byte)    */
	} u1;
	union {
	    u_long	unit_id[ 2 ];		/* Unit ID		     */
	    u_long	bus_addr;		/* Bus address		     */
	} u2;
	union {
	    struct _dsk {			/* Disk transfer errors	     */
		u_char	unit_svr;		/* Unit software version     */
		u_char	unit_hvr;		/* Unit hardware version     */
		u_char	level;			/* Level		     */
		u_char	retry;			/* Retry		     */
		u_long	vol_ser;		/* Volume serial number	     */
		u_long	hdr_code;		/* Header code		     */
		u_char	disk_trn;		/* Disk transfer (1st byte)  */
	    } dsk;
	    struct _sdi {			/* SDI errors		     */
		u_char	unit_svr;		/* Unit software version     */
		u_char	unit_hvr;		/* Unit hardware version     */
		u_short			:16;	/* Reserved		     */
		u_long	vol_ser;		/* Volume serial number	     */
		u_long	hdr_code;		/* Header code		     */
		u_char	sdi[ 12 ];		/* SDI Errors		     */
	    } sdi;
	    struct _sde {			/* Small disk errors	     */
		u_char	unit_svr;		/* Unit software version     */
		u_char	unit_hvr;		/* Unit hardware version     */
		u_short	sde_cyl;		/* Cylinder		     */
		u_long	vol_ser;		/* Volume serial number	     */
	    } sde;
	    struct _rpl {			/* BBR errors		     */
		u_char	unit_svr;		/* Unit software version     */
		u_char	unit_hvr;		/* Unit hardware version     */
		u_short	rpl_flgs;		/* Replacement flags 	     */
		u_long	vol_ser;		/* Volume serial number	     */
		u_long	bad_lbn;		/* Bad logical block number  */
		u_long	old_rbn;		/* Old replacement BN	     */
		u_long	new_rbn;		/* New replacement BN	     */
		u_short	cause;			/* Cause (an event code)     */
	    } rpl;
	    struct _ldr {			/* Loader errors	     */
		u_char	unit_svr;		/* Unit software version     */
		u_char	unit_hvr;		/* Unit hardware version     */
		u_short			:16;	/* Reserved		     */
		u_long	ml_id[2];		/* Loader identifier	     */
		u_char	ml_svr;			/* Loader software version   */
		u_char	ml_hvr;			/* Loader hardware version   */
		u_short	ml_unit;		/* Loader unit number	     */
		u_char	ldr_err;		/* Loader error (1st byte)   */
	    } ldr;
	    struct _tpe {			/* Tape transfer errors	     */
		u_char	unit_svr;		/* Unit software version     */
		u_char	unit_hvr;		/* Unit hardware version     */
		u_char	level;			/* Level		     */
		u_char	retry;			/* Retry		     */
		u_long	vol_ser;		/* Volume serial number	     */
		u_long	hdr_code;		/* Header code		     */
		u_char	tape_trn;		/* Tape transfer (1st byte)  */
	    } tpe;
	    struct _sti {			/* STI errors		     */
		u_char	unit_svr;		/* Unit software version     */
		u_char	unit_hvr;		/* Unit hardware version     */
		u_short			:16;	/* Reserved		     */
		u_long 	gap_cnt;		/* Position (object count)   */
		u_char	fmtr_svr;		/* Formatter s/w version     */
		u_char	fmtr_hvr;		/* Formatter h/w version     */
		u_short			:16;	/* Reserved		     */
		u_char	sti[62];		/* STI error information     */
	    } sti;
	} u3;
    } lg;
} MSLG;

/* Redefine cells inside inner unions and structures to eliminate
 * multiple qualification levels.
 */
#define mslg_bad_lbn	lg.u3.rpl.bad_lbn	/* Bad logical block number  */
#define mslg_bus_addr	lg.u2.bus_addr		/* Bus address		     */
#define mslg_cause	lg.u3.rpl.cause		/* Cause (an event code)     */
#define mslg_cmd_ref	lg.cmd_ref		/* Command reference number  */
#define mslg_cnt_err	lg.u1.cnt_err		/* Cntlr-dep data (1st byte) */
#define mslg_cnt_hvr	lg.cnt_hvr		/* Controller h/w version    */
#define mslg_cnt_id	lg.cnt_id		/* Controller ID	     */
#define mslg_cnt_svr	lg.cnt_svr		/* Controller s/w version    */
#define mslg_disk_trn	lg.u3.dsk.disk_trn	/* Disk transfer (1st byte)  */
#define mslg_event	lg.event		/* Event code		     */
#define mslg_flags	lg.flags		/* Error log message flags   */
#define mslg_fmtr_hvr	lg.u3.sti.fmtr_hvr	/* Formatter h/w version     */
#define mslg_fmtr_svr	lg.u3.sti.fmtr_svr	/* Formatter s/w version     */
#define mslg_format	lg.format		/* Format		     */
#define mslg_gap_cnt	lg.u3.sti.gap_cnt	/* Position (object count)   */
#define mslg_hdr_code	lg.u3.dsk.hdr_code	/* Header code		     */
#define mslg_ldr_err	lg.u3.ldr.ldr_err	/* Loader error (1st byte )  */
#define mslg_level	lg.u3.dsk.level		/* Level		     */
#define mslg_ml_hvr	lg.u3.ldr.ml_hvr	/* Loader hardware version   */
#define mslg_ml_id	lg.u3.ldr.ml_id		/* Loader identifier	     */
#define mslg_ml_svr	lg.u3.ldr.ml_svr	/* Loader software version   */
#define mslg_ml_unit	lg.u3.ldr.ml_unit	/* Loader unit number	     */
#define mslg_mult_unt	lg.u1.mult_unt		/* Multi-unit code	     */
#define mslg_new_rbn	lg.u3.rpl.new_rbn	/* New replacement BN	     */
#define mslg_old_rbn	lg.u3.rpl.old_rbn	/* Old replacement BN	     */
#define mslg_retry	lg.u3.dsk.retry		/* Retry		     */
#define mslg_rpl_flgs	lg.u3.rpl.rpl_flgs	/* Replacement flags	     */
#define mslg_sde_cyl	lg.u3.sde.sde_cyl	/* Cylinder		     */
#define mslg_sdi	lg.u3.sdi.sdi		/* SDI Information	     */
#define mslg_seq_num	lg.seq_num		/* Sequence number	     */
#define mslg_sti	lg.u3.sti.sti		/* STI Information	     */
#define mslg_tape_trn	lg.u3.tpe.tape_trn	/* Tape transfer (1st byte ) */
#define mslg_unit	lg.unit			/* Unit number		     */
#define mslg_unit_hvr	lg.u3.dsk.unit_hvr	/* Unit hardware version     */
#define mslg_unit_id	lg.u2.unit_id		/* Unit identifier	     */
#define mslg_unit_svr	lg.u3.dsk.unit_svr	/* Unit software version     */
#define mslg_vol_ser	lg.u3.dsk.vol_ser	/* Volume serial number	     */



/* Define a union of the MSCP and MSLG structures for use in determining 
 * data structure storage requirements.
 */
typedef union {
    MSCP	mscp_maxsize;	/* MSCP maximum message size		     */
    MSLG	mslg_maxsize;	/* MSLG maximum datagram size		     */
} MSCP_MAXBUF;



/* MSCP format code definitions (from MSCP specification Table A-9)
 */
#define MSLG_FM_CNT_ER		0x00	/* Controller errors		     */
#define MSLG_FM_BUS_ADDR	0x01	/* Host memory access errors	     */
#define MSLG_FM_DISK_TRN	0x02	/* Disk transfer errors	(D)	     */
#define MSLG_FM_SDI		0x03	/* SDI errors			     */
#define MSLG_FM_SML_DSK		0x04	/* Small disk errors		     */
#define MSLG_FM_TAPE_TRN	0x05	/* Tape transfer error (T)	     */
#define	MSLG_FM_STI_ERR		0X06	/* STI command error (T)	     */
#define MSLG_FM_STI_DEL		0x07	/* STI driver error log (T)	     */
#define MSLG_FM_STI_FEL		0x08	/* STI formatter error log (T)	     */
#define MSLG_FM_REPLACE 	0x09	/* Bad block replacement attempt     */
#define MSLG_FM_LDR_ERR 	0x0a	/* Media loader errors		     */
#define MSLG_FM_IBMSENSE	0x0b	/* Sense data error log (T)	     */


/* MSCP error log message flags (from MSCP specification Table A-10)
 */
#define MSLG_LF_SUCC	0x80	/* Operation successful			     */
#define MSLG_LF_CONT	0x40	/* Operation continuing			     */
#define MSLG_LF_BBR	0x20	/* Bad block replacement attempt	     */
#define MSLG_LF_RPLER	0x10	/* Error during replacement		     */
#define MSLG_LF_INFO	0x02	/* Informational			     */
#define MSLG_LF_SQNRS	0x01	/* Sequence number reset		     */

/* Bad block replacement attempt (from MSCP specification table A-11)
 */
#define	MSLG_LFR_RP	0x8000	/* Replacement attempted		     */
#define	MSLG_LFR_FE	0x4000	/* Force error				     */
#define	MSLG_LFR_TE	0x2000	/* Tertiary revector			     */
#define	MSLG_LFR_RF	0x1000	/* Reformat error			     */
#define	MSLG_LFR_RI	0x0800	/* RCT inconsistent			     */
#define	MSLG_LFR_BR	0x0400	/* Bad replacement block number 	     */

/* ACCESS NON-VOLATILE MEMORY command operation codes
 *  (from MSCP specification table A-12)
 */
#define MSCP_ANM_READ	0x00	/* Read non-volatile memory		     */
#define MSCP_ANM_EXCG	0x01	/* Exchange command data with NVM data	     */
#define MSCP_ANM_TSST	0x02	/* Test and set contents of NVM		     */

/* FORMAT function codes (from MSCP specification table A-13)
 */
#define MSCP_FMT_DFLT	0	/* Device's default			     */
#define MSCP_FMT_SING	1	/* Single density			     */
#define MSCP_FMT_DOUB	2	/* Double density			     */
#define MSCP_FMT_RX33	282	/* RX33 - ISO DIS8630-1985		     */

/**/

/* Status and event codes (MSCP specification table B-1)
 */
#define MSCP_ST_MASK	0x1f	/* Status / event code mask		     */
#define MSCP_ST_SBCOD	0x20	/* Sub-code multiplier			     */
#define MSCP_ST_SBBIT	0x05	/* Sub-code starting bit position	     */
#define MSCP_ST_SUCC	0x00	/* Success				     */
#define MSCP_ST_ICMD	0x01	/* Invalid command			     */
#define MSCP_ST_ABRTD	0x02	/* Command aborted			     */
#define MSCP_ST_OFFLN	0x03	/* Unit-offline				     */
#define MSCP_ST_AVLBL	0x04	/* Unit-available			     */
#define MSCP_ST_MFMTE	0x05	/* Media format error			     */
#define MSCP_ST_WRTPR	0x06	/* Write protected			     */
#define MSCP_ST_COMP	0x07	/* Compare error			     */
#define MSCP_ST_DATA	0x08	/* Data error				     */
#define MSCP_ST_HSTBF	0x09	/* Host buffer access error		     */
#define MSCP_ST_CNTLR	0x0a	/* Controller error			     */
#define MSCP_ST_DRIVE	0x0b	/* Drive error				     */
#define MSCP_ST_SHST	0x0c	/* Shadow set has changed (D)		     */
#define MSCP_ST_FMTER	0x0c	/* Formatter error (T)			     */
#define MSCP_ST_BOT	0x0d	/* BOT encountered (T)			     */
#define MSCP_ST_TAPEM	0x0e	/* Tape mark encountered (T)		     */
#define MSCP_ST_RDTRN	0x10	/* Record data truncated		     */
#define MSCP_ST_PLOST	0x11	/* Position lost (T)			     */
#define MSCP_ST_PRESE	0x12	/* Previous serious exception (T)	     */
#define MSCP_ST_SEX	0x12	/* Serious Exception (T)		     */
#define MSCP_ST_LED	0x13	/* LEOT detected (T)			     */
#define MSCP_ST_BBR	0x14	/* Bad block replacement completion	     */
#define MSCP_ST_IPARM	0x15	/* Invalid parameter			     */
#define MSCP_ST_INFO	0x16	/* Informational message, not an error	     */
#define MSCP_ST_LOADR	0x17	/* Media loader error			     */
#define MSCP_ST_DIAG	0x1f	/* Internal diagnostic message		     */


/* Success sub-code values (MSCP specification table B-2)
 */
#define MSCP_SC_NORML	0x0000	/* Normal				     */
#define MSCP_SC_SDIGN	0x0001	/* Spin-down ignored			     */
#define MSCP_SC_STCON	0x0002	/* Still connected			     */
#define MSCP_SC_DUPUN	0x0004	/* Duplicate unit number		     */
#define MSCP_SC_ALONL	0x0008	/* Already online			     */
#define MSCP_SC_STONL	0x0010	/* Still online				     */
#define MSCP_SC_UNIGN	0x0011	/* Still online/Unload ignored (T)	     */
#define MSCP_SC_EOT	0x0020	/* EOT encountered (T)			     */
#define MSCP_SC_INREP	0x0020	/* Incomplete replacement (D)		     */
#define MSCP_SC_IVRCT	0x0040	/* Invalid RCT				     */
#define MSCP_SC_ROVOL	0x0080	/* Read-only volume format		     */


/* Invalid command sub-code values
 */
#define MSCP_SC_INVML	0x0000	/* Invalid message length		     */


/* Command aborted sub-code values
 *	Subcode values are not used for this status.
 */

/* Data error sub-code values
 */
#define MSCP_SC_LGAP   0x0000  /* Long Gap Encountered			     */
#define MSCP_SC_UREAD  0x0007  /* Unrecoverable read error		     */


/* Unit-offline sub-code values
 */
#define MSCP_SC_UNKNO	0x0000	/* Unit unknown or online to another ctlr    */
#define MSCP_SC_NOVOL	0x0001	/* No volume mounted/drive switched off	     */
#define MSCP_SC_INOPR	0x0002	/* Unit is inoperative			     */
#define MSCP_SC_UDSBL	0x0008	/* Unit is disabled by field service	     */
#define MSCP_SC_EXUSE	0x0010	/* Unit is seized by another host	     */
#define MSCP_SC_LOADE	0x0020	/* Loader cycle error			     */

/* Unit-available sub-code values
 */
#define MSCP_SC_AVAIL	0x0000	/* Implies success to set exclusive access   */
#define MSCP_SC_NOMEMB	0x0001	/* No members				     */
#define MSCP_SC_ALUSE	0x0020	/* Can't seize unit, online to other host    */


/* Write-protected sub-code values (MSCP specification table B-2)
 */
#define MSCP_SC_DATL	0x0008	/* Unit is data loss write protected	     */
#define MSCP_SC_SOFTW	0x0080	/* Unit is software write protected	     */
#define MSCP_SC_HARDW	0x0100	/* Unit is hardware write protected	     */


/* Invalid parameter sub-code values (MSCP specification table B-2)
 */
#define MSCP_SC_IVKLN	0x0001	/* Invalid key length			     */
#define MSCP_SC_IVKTP	0x0002	/* Invalid key type			     */
#define MSCP_SC_IVKVL	0x0003	/* Invalid key value			     */


/* Compare error sub-code values (MSCP specification table B-2)
 *	Sub-codes are not used for this status.
 */ 

/* Message from an internal diagnostic sub-code values (MSCP specification
 * table B-2).  Sub-codes are not used for this status.
 */

/* Record data truncated sub-code values (MSCP specification table B-2)
 *	Sub-codes are not used for this status.
 */

/* Access denied sub-code values (MSCP specification table B-2)
 *	Sub-codes are not used for this status.
 */


/* Media format error sub-code values (MSCP specification table B-3)
 */


/* In addition to the following codes, some data error sub-codes (those
 * marked with "(*)") may also appear as media format error sub-codes.
 */
#define MSCP_SC_NO512	0x0005	/* 576 byte sectors on a 512 byte drive	     */
#define	MSCP_SC_UNFMT	0x0006	/* Disk unformatted or FCT corrupted	     */
#define MSCP_SC_RCTBD	0x0008	/* RCT corrupted			     */
#define MSCP_SC_NORBL	0x0009	/* No replacement block available	     */
#define MSCP_SC_MULT	0x000A	/* Multi-copy protection warning	     */


/* Data error sub-code values (MSCP specification table B-5)
 */

/* Those sub-codes marked (*) may also appear in media format errors
 */
#define MSCP_SC_FRCER	0x0000	/* Forced error (*)			     */
#define MSCP_SC_IVHDR	0x0002	/* Invalid header (*)			     */
#define MSCP_SC_SYNTO	0x0003	/* Data synch timeout (*)		     */
#define MSCP_SC_ECCFL	0x0004	/* Correctable error in ECC field	     */
#define MSCP_SC_UNECC	0x0007	/* Uncorrectable ECC error (*)		     */
#define MSCP_SC_1SECC	0x0008	/* 1 symbol correctable ECC error	     */
#define MSCP_SC_2SECC	0x0009	/* 2 symbol correctable ECC error	     */
#define MSCP_SC_3SECC	0x000a	/* 3 symbol correctable ECC error	     */
#define MSCP_SC_4SECC	0x000b	/* 4 symbol correctable ECC error	     */
#define MSCP_SC_5SECC	0x000c	/* 5 symbol correctable ECC error	     */
#define MSCP_SC_6SECC	0x000d	/* 6 symbol correctable ECC error	     */
#define MSCP_SC_7SECC	0x000e	/* 7 symbol correctable ECC error	     */
#define MSCP_SC_8SECC	0x000f	/* 8 symbol correctable ECC error	     */


/* Host buffer access error sub-code values (MSCP specification table B-6)
 */
#define MSCP_SC_ODDTA	0x0001	/* Odd transfer address			     */
#define MSCP_SC_ODDBC	0x0002	/* Odd byte count			     */
#define MSCP_SC_NXM	0x0003	/* Non-existent memory			     */
#define MSCP_SC_MPAR	0x0004	/* Host memory parity error		     */
#define MSCP_SC_IVPTE	0x0005	/* Invalid Page Table Entry (UQSSP)	     */
#define MSCP_SC_IVBFN	0x0006	/* Invalid buffer name			     */
#define MSCP_SC_BLENV	0x0007	/* Buffer length violation		     */
#define MSCP_SC_ACVIO	0x0008	/* Access violation			     */


/* Controller error sub-code values (MSCP specification table B-7)
 */
#define	MSCP_SC_HDETO	0x0000	/* Host detected controller timeout	     */
#define MSCP_SC_DLATE	0x0001	/* Data late (SERDES) error		     */
#define MSCP_SC_EDCER	0x0002	/* EDC error				     */
#define MSCP_SC_DTSTR	0x0003	/* Data structure error			     */
#define MSCP_SC_IEDC	0x0004	/* Internal EDC error			     */
#define MSCP_SC_LACIN	0x0005	/* LESI adaptor card input error	     */
#define MSCP_SC_LACOU	0x0006	/* LESI adaptor card output error	     */
#define MSCP_SC_LACCB	0x0007	/* LESI adaptor card cable not in place	     */
#define MSCP_SC_OVRUN	0x0008	/* Controller overrun or underrun	     */
#define MSCP_SC_MEMER	0x0009	/* Controller memory error		     */


/* Drive error sub-code values (MSCP specification table B-8)
 */
#define MSCP_SC_CMDTO	0x0001	/* SDI command timed out		     */
#define MSCP_SC_XMSER	0x0002	/* Controller-detected transmission error    */
#define MSCP_SC_MISSK	0x0003	/* Positioner error (mis-seek)		     */
#define MSCP_SC_RWRDY	0x0004	/* Lost read/write ready between transfers   */
#define MSCP_SC_CLKDO	0x0005	/* Drive clock dropout			     */
#define MSCP_SC_RXRDY	0x0006	/* Lost receiver ready between sectors	     */
#define MSCP_SC_DRDET	0x0007	/* Drive-detected error			     */
#define	MSCP_SC_PULSE	0x0008	/* Ctlr-detected pulse/state parity error    */
#define MSCP_SC_PRTCL	0x000a	/* Controller detected protocol error	     */
#define	MSCP_SC_FLINI	0x000b	/* Drive failed initialization		     */
#define	MSCP_SC_IGINI	0x000c	/* Drive ignored initialization		     */
#define	MSCP_SC_RRCOL	0x000d	/* Receiver ready collision		     */


/* Bad block replacement sub-code values (MSCP specification table B-9)
 */
#define MSCP_SC_BBROK	0x0000	/* Bad block successfully replaced	     */
#define MSCP_SC_NOTRP	0x0001	/* Block verified OK, not replaced	     */
#define MSCP_SC_RPLFL	0x0002	/* REPLACE command failure		     */
#define MSCP_SC_ICRCT	0x0003	/* Inconsistent RCT			     */
#define	MSCP_SC_RPLACC	0x0004	/* Drive access failure			     */
#define MSCP_SC_RCTFL	0x0005	/* RCT full				     */
#define MSCP_SC_RCTRC	0x0006	/* RCT recursion failure		     */

/* Media loader error sub-code values (MSCP specification table B-10)
 */
#define MSCP_SC_LTIMO	0x0001	/* Loader command time out		     */
#define MSCP_SC_LTERR	0x0002	/* Loader controller transmission error	     */
#define MSCP_SC_LPERR	0x0003	/* Loader controller protocol error	     */
#define MSCP_SC_LDERR	0x0004	/* Loader error				     */
#define MSCP_SC_LDONL	0x0005	/* Drive online				     */
#define MSCP_SC_INVSRC	0x0006	/* Invalid SOURCE slot ID		     */
#define MSCP_SC_INVDST	0x0007	/* Invalid DESTINATION slot ID		     */
#define MSCP_SC_SRCEMP	0x0008	/* SOURCE slot is empty			     */
#define MSCP_SC_DSTFUL	0x0009	/* DESTINATION slot is full		     */
#define MSCP_SC_LDMOT	0x000a	/* Loader motion error			     */
#define MSCP_SC_LDDRIE	0x000b	/* Loader/drive interface error		     */
#define MSCP_SC_LDSLIE	0x000c	/* Loader/slot interface error		     */
#define MSCP_SC_LDMECH	0x000d	/* Loader mechanical error		     */
#define MSCP_SC_LDHARD	0x000e	/* Loader hardware error		     */
#define MSCP_SC_LDCTLR	0x000f	/* Loader controller error		     */
#define MSCP_SC_LDUNRE	0x0010	/* Unrecognized loader subcommand	     */
#define MSCP_SC_LDREXC	0x0011	/* Exception - recoverable via RESET	     */
#define MSCP_SC_LDUEXC	0x0012	/* Exception - unrecoverable		     */

/* Informational event only subcode values (MSCP specification table B-11)
 */
#define MSCP_SC_IQUAL	0x0001	/* Media Quality Log			     */
#define MSCP_SC_ISTAT	0x0002	/* Unload, spin down statistics		     */

/* TMSCP tape format bitflags (from TMSCP specification Table C-1)
 */
#define MSCP_TF_MASK	0x00ff	/* Bitfield mask			     */
#define MSCP_TF_CODE	0x0100	/* Format Code multiplier		     */
#define MSCP_TC_OLD	0x0000	/* Backwards compatible			     */
#define MSCP_TC_9TRACK	0x0100	/* New 9 track devices			     */
#define MSCP_TC_CTP	0x0200	/* TK50 compatible cartridges		     */
#define MSCP_TC_3480	0x0300	/* IBM 3480 compatible cartridge	     */
#define MSCP_TC_W1	0x0400	/* RV20 compatible cartridges		     */


/* TMSCP tape format flags (from TMSCP specification Table C-2)
 */
#define MSCP_TF_NORML	0x0001	/* Normal (low) Density			     */
#define	MSCP_TF_800	0x0001	/* NRZI 800 bpi				     */
#define	MSCP_TF_PE	0x0002	/* Phase Encoded 1600 bpi		     */
#define MSCP_TF_BLKHD	0x0002	/* High Density				     */
#define MSCP_TF_ENHD	0x0002	/* Enhanced Density			     */
#define	MSCP_TF_GCR	0x0004	/* Group Code Recording 6250 bpi	     */
#define	MSCP_TF_NDCP	0x0004	/* Normal density compacted		     */
#define	MSCP_TF_BLOCK	0x0008	/* Cartridge Block Mode			     */
#define	MSCP_TF_EDCP	0x0008	/* Enhanced density compacted		     */

/**/

/* Controller and Unit Identifier "Class" Values
 *     (from MSCP specification Table C-1)
 */
#define MSCP_CL_CNTRL		1	/* Mass storage controller	     */
#define MSCP_CL_DISK		2	/* DEC STD 166 disk class device     */
#define MSCP_CL_TAPE		3	/* Tape class device		     */
#define MSCP_CL_D144		4	/* DEC STD 144 disk class device     */
#define MSCP_CL_LOADER		5	/* Loader Class Devices		     */

/* Controller "Model" values (from MSCP specification Table C-2)
 *
 * (TODO Put a few comments back in here.)
 */
#define MSCP_CM_HSC50		1
#define MSCP_CM_UDA50		2
#define MSCP_CM_RC25		3
#define MSCP_CM_VMS		4
#define MSCP_CM_TU81		5
#define MSCP_CM_UDA50A		6
#define MSCP_CM_RDRX		7
#define MSCP_CM_TOPS		8
#define MSCP_CM_TK50		9
#define MSCP_CM_TQK50		9
#define MSCP_CM_RUX50		10
#define MSCP_CM_AIO		12
#define MSCP_CM_KFBTA		12
#define MSCP_CM_KDA50		13
#define MSCP_CM_TK70		14
#define MSCP_CM_TQK70		14
#define MSCP_CM_RV20		15
#define MSCP_CM_RRD50		16
#define MSCP_CM_KRQ50		16
#define MSCP_CM_KDB50		18
#define MSCP_CM_RQDX3		19
#define MSCP_CM_RQDX4		20
#define MSCP_CM_SII_DISK	21
#define MSCP_CM_SII_TAPE	22
#define MSCP_CM_SII_DISK_TAPE	23
#define MSCP_CM_SII_OTHER	24
#define MSCP_CM_TUK50		25
#define MSCP_CM_KRU50		26
#define MSCP_CM_KDM70		27
#define MSCP_CM_TQL70		28
#define MSCP_CM_TM32		29
#define MSCP_CM_HSC70		32
#define MSCP_CM_HSC40		33
#define MSCP_CM_HSC60		34
#define MSCP_CM_HSC90		35
#define MSCP_CM_DEBNT		65
#define MSCP_CM_TBK70		66
#define MSCP_CM_TBK7L		68
#define MSCP_CM_RF30		96
#define MSCP_CM_RF71		97
#define MSCP_CM_TF85		98
#define MSCP_CM_TF70		99
#define MSCP_CM_RF31		100
#define MSCP_CM_RF72		101
#define MSCP_CM_RF73		102
#define MSCP_CM_TF70L		103
#define MSCP_CM_ULTRIX32	248
#define MSCP_CM_SVS		249

#endif
