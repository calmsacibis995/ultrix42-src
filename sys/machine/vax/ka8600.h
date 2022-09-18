/*	7/2/90 (ULTRIX-32) @(#)ka8600.h	4.1	*/	
/************************************************************************
 *									*
 *			Copyright (c) 1985,86 by			*
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


/* **********************************************************************
 * Modification History: /sys/vax/ka8600.h
 *
 * 09-Apr-86 -- darrell
 *	Created this file, and added structures and constants for 8600 
 *	machine check recovery.
 *
 ************************************************************************/

/*
 * 8600 Machine Check bits used to detect the
 *	type of machine check.
 */
#define M_MBOX_FE	0x8000		/* Mbox Fatal Error */
#define M_FBOX		0x10000000	/* Fbox service */
#define M_IBOX_ERR	0x2000		/* Ibox error */

/*
 * 8600 EBCS Register bits
 */
#define EBOX_ERR_MASK	0x1e00		/* OR of M_EDP_PE, M_USTK_PE,
					   M_ECS_PE, M_EMCR_PE */
#define M_ABORT_MASK	0x1f		/* VAX state abort bits */
#define M_EDP_PE	0x0200		/* Ebox Data Path Parity Error */
#define M_USTK_PE	0x0400		/* Microstack Parity Error ? */
#define M_ECS_PE	0x0800		/* Ebox Control Store Parity Error ? */
#define M_EMCR_PE	0x1000		/* ? */
#define M_MBOX_1D	0x81d0000	/* MBOX 1D interrupt */
#define M_MUL_ERR	0x80		/* multiple error */
#define M_ABUS_CYC_MASK 0x3c000000
#define M_ABUS_CYC	0x8		/* error during an ABUS cycle */
#define M_CSH_ERR	0x60		/* one of several cache errors */
#define M_SELECT	0x4		/* which half of cache */
#define M_CPR_AB	0xc00000	/* cycle parameter ram s parity error */
#define M_CSH_DAT_PE	0x8		/* cache data parity error */
#define M_FATAL_MEM_ERR 0x580000	/* Mbox fatal memory error condition */
/*
 * 8600 Machine Check type codes
 */
#define C_FBOX		1	/* Fbox error */
#define C_EBOX		2	/* Ebox error */
#define C_IBOX		3	/* Ibox error */
#define C_MBOX		4	/* Mbox error */
#define C_TB_ERR	5	/* translation buffer error */
#define C_MBOX_1D	6	/* Mbox 1D interrupt */
#define C_DBL_ERR	0xff	/* More than one type of machine check */
#define C_MCHK_MASK	0xff	/* Machine check type mask */

/* This is a table that maps one bit foreach opcode in the VAX
 * instruction set.  If the corresponding bit is set, that opcode
 * does only one read and may be safely restarted if a CP_IO BUF error occurs.
 * The table does not take into account reads done for address calcualtions.
 * These are assumed to not be relevant in deciding if I/O space was referenced.
 * Queue, decimal, and string instructions are not assumed to be save.
 */
struct {
	u_short op_bit_mask;
}
	sbi_inst [] = {
		0x343f,		/* halt, nop, rei, ret, prober/w */
		0xffff,		/* branches */
		0x0000,
		0xd00f,		/* brw,converts, moves */
		0x7f55,		/* floating ops */
		0x014d,
		0x7f55,
		0xc14f,
		0xd555,		/* arithmitic byte */
		0xdff5,		/* cmpb, tstb */
		0xd555,		/* arithmetic word */
		0xfff5,		/* cmpw, tstw, bispsw, bicpsw */
		0xd555,		/* arithmetic long */
		0xfff5,		/* cmpl,tstl */
		0xcfff,		/* bbs, bbc blbs, blbc, cmpv, cmpzv */
		0x0cfd		/* aob, cvt */
	};

#define IBOX_THRESH	10	/* minimum time allowed for 3
					 * ibox errors (10 ms units) */
#define FBOX_THRESH	10	/* minimum time allowed for 3
					 * fbox errors (10 ms units) */
#define EBOX_THRESH	10	/* minimum time allowed for 3
					 * ebox errors (10 ms units) */
#define MBOX_THRESH	2	/* minimum time allowed for 2
					 * mbox errors (10 ms units) */
#define TB_THRESH	10	/* minimum time allowed for 3
					 * tb errors (10 ms units) */
#define CACHE_THRESH	10	/* minimum time allowed for 3
					 * cache errors (10 ms units) */
#define MBOX_1D_THRESH	10	/* minimum time allowed for 3
					 * mbox 1d errors (10 ms units) */
#define GENERIC_THRESH	10	/* minimum time allowed for 3
					 * mbox 1d errors (10 ms units) */
struct ibox_errcnt {
	int	ibox_last;	/* time of most recent ibox error */
	int	ibox_prev;	/* time of previous ibox error */
	int	ibox_total;	/* total ibox errors */
};

struct fbox_errcnt {
	int	fbox_last;	/* time of most recent fbox error */
	int	fbox_prev;	/* time of previous fbox error */
	int	fbox_total;	/* total fbox errors */
};

struct ebox_errcnt {
	int	ebox_last;	/* time of most recent ebox error */
	int	ebox_prev;	/* time of previous fbox error */
	int	ebox_total;	/* total ebox errors */
};

struct mbox_errcnt {
	int	mbox_last;	/* time of most recent mbox error */
	int	mbox_pc;	/* PC of most recent mbox fatal error */
	int	mbox_total;	/* total mbox errors */
};

struct tb_errcnt {
	int	tb_last;	/* time of most recent tb error */
	int	tb_prev;	/* time of previous tb error */
	int	tb_total;	/* total tb errors */
};

struct csh_errcnt {
	int	csh_a_last;	/* time of most recent csh_a error */
	int	csh_a_prev;	/* time of previous csh_a error */
	int	csh_b_last;	/* time of most recent csh_b error */
	int	csh_b_prev;	/* time of previous csh_b error */
	int	csh_total;	/* total ebox errors */
};

struct mbox_1d_errcnt {
	int	mbox_1d_last;	/* time of most recent mbox_1d error */
	int	mbox_1d_prev;	/* time of previous mbox_1d error */
	int	mbox_1d_total;	/* total mbox_1d errors */
};

struct generic_errcnt {
	int	gen_last;	/* time of most recent generic error */
	int	gen_prev;	/* time of previous generic error */
	int	gen_total;	/* total generic errors */
};

char   *mc8600[] = {
	"Unknown machine check type",		/* 0 */
	"fbox error",				/* 1 */
	"ebox error",				/* 2 */
	"ibox error",				/* 3 */
	"mbox error",				/* 4 */
	"translation buffer error",		/* 5 */
	"mbox 1d error" 			/* 6 */
};

struct mc8600frame {
	int	mc8600_bytcnt;		/* machine check stack frame byte cnt */
	int	mc8600_ehm_sts; 	/* ehm.sts */
	int	mc8600_evmqsav; 	/* ebox vmq sav */
	int	mc8600_ebcs;		/* ebox control status register */
	int	mc8600_edpsr;		/* ebox data path status register */
	int	mc8600_cslint;		/* ebox console/interrupt register */
	int	mc8600_ibesr;		/* ibox error and status register */
	int	mc8600_ebxwd1;		/* ebox write data 1 */
	int	mc8600_ebxwd2;		/* ebox write data 1 */
	int	mc8600_ivasav;		/* ibox va sav */
	int	mc8600_vibasav; 	/* ibox viba */
	int	mc8600_esasav;		/* ibox esa */
	int	mc8600_isasav;		/* ibox isa */
	int	mc8600_cpc;		/* ibox cpc */
	int	mc8600_mstat1;		/* mbox status reg#1 */
	int	mc8600_mstat2;		/* mbox status reg#2 */
	int	mc8600_mdecc;		/* mbox data ecc register */
	int	mc8600_merg;		/* mbox error generator register */
	int	mc8600_cshctl;		/* mbox cache control register */
	int	mc8600_mear;		/* mbox error address register */
	int	mc8600_medr;		/* mbox error data register */
	int	mc8600_accs;		/* accelerator status register */
	int	mc8600_cses;		/* control store error status reg */
	int	mc8600_pc;		/* pc */
	int	mc8600_psl;		/* psl */
};
