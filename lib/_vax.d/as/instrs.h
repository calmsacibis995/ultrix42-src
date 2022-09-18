/*
 *	Copyright (c) 1982 Regents of the University of California
 *	@(#)instrs.h 4.5 6/9/83
 */

/************************************************************************
 *									*
 *			Modification History				*
 *
 * 001  Tanya Klinchina, 20-Nov-1989
 *      Added support for vector instructions.
 *
 ************************************************************************/

/*
 *	Argument data types
 *
 *	If you change these definitions, you must also change the tables
 *	in assizetab.c
 */
#define	TYPB		000	/* byte integer */
#define	TYPW		001	/* word integer */
#define	TYPL		002	/* long integer */
#define	TYPQ		003	/* quad integer */
#define	TYPO		004	/* octa integer */
#define	TYPF		005	/* F float */
#define	TYPD		006	/* D float */
#define	TYPG		007	/* G float */
#define	TYPH		010	/* H float */
#define TYPF4           011     /* F float + 4 for VSMERGEF instruction */
#define	TYPUNPACKED	012	/* when unpacked into mantissa & exponent */
#define	TYPNONE		013	/* when nothing */
#define	TYPLG		4	/* number of bits the above take up */

#define	TYPMASK	((1<<TYPLG)-1)	/* the mask (assumes 2's comp arith) */
/*
 *	Constructors and extractors for argument access kinds and types
 */
#define A_CONS(access, type)	((access) | (type))
#define	A_ACCEXT(consed)	((consed) & (TYPMASK << TYPLG))
#define	A_TYPEXT(consed)	((consed) & TYPMASK)

/*
 * Argument access types used to test validity of operands to operators
 */
#define	ACCR	(1<<TYPLG)			/* read */
#define	ACCW	(2<<TYPLG)			/* write */
#define	ACCB	(4<<TYPLG)			/* branch displacement */
#define	ACCA	(8<<TYPLG)			/* address only */
#define	ACCV	(8<<TYPLG)			/* address only */
#define	ACCM	(ACCR | ACCW)			/* modify */
#define	ACCI	(ACCB | ACCR)			/* XFC code */

#define ACCESSMASK	(ACCA | ACCR | ACCW | ACCB)	/* the mask */

/*
 *	Construction of TYPX and ACCX, to make the instrs table
 *	easy to use and read.
 */
/*
 *	For real memory address
 */
#define	A_AB	A_CONS(ACCA, TYPB)
#define	A_AW	A_CONS(ACCA, TYPW)
#define	A_AL	A_CONS(ACCA, TYPL)
#define	A_AQ	A_CONS(ACCA, TYPQ)
#define	A_AO	A_CONS(ACCA, TYPO)
#define	A_AF	A_CONS(ACCA, TYPF)
#define	A_AD	A_CONS(ACCA, TYPD)
#define	A_AG	A_CONS(ACCA, TYPG)
#define	A_AH	A_CONS(ACCA, TYPH)
/*
 *	For real memory addresses, or register addresses [sic]
 *
 *	CHEAT! we just call these read access, since
 *	registers are allowed. All field instruction, except insv,
 *	are are read access fields.
 */
#define	A_VB	A_CONS(ACCR, TYPB)
#define	A_VW	A_CONS(ACCR, TYPW)
#define	A_VL	A_CONS(ACCR, TYPL)
#define	A_VQ	A_CONS(ACCR, TYPQ)
#define	A_VO	A_CONS(ACCR, TYPO)
#define	A_VF	A_CONS(ACCR, TYPF)
#define	A_VD	A_CONS(ACCR, TYPD)
#define	A_VG	A_CONS(ACCR, TYPG)
#define	A_VH	A_CONS(ACCR, TYPH)
/*
 *	For branch displacement
 */
#define	A_BB	A_CONS(ACCB, TYPB)
#define	A_BW	A_CONS(ACCB, TYPW)
/*
 *	For modification
 */
#define	A_MB	A_CONS(ACCM, TYPB)
#define	A_MW	A_CONS(ACCM, TYPW)
#define	A_ML	A_CONS(ACCM, TYPL)
#define	A_MF	A_CONS(ACCM, TYPF)
#define	A_MD	A_CONS(ACCM, TYPD)
#define	A_MG	A_CONS(ACCM, TYPG)
#define	A_MH	A_CONS(ACCM, TYPH)
/*
 *	For reading
 */
#define	A_RB	A_CONS(ACCR, TYPB)
#define	A_RW	A_CONS(ACCR, TYPW)
#define	A_RL	A_CONS(ACCR, TYPL)
#define	A_RQ	A_CONS(ACCR, TYPQ)
#define	A_RO	A_CONS(ACCR, TYPO)
#define	A_RF	A_CONS(ACCR, TYPF)
#define	A_RD	A_CONS(ACCR, TYPD)
#define	A_RG	A_CONS(ACCR, TYPG)
#define	A_RH	A_CONS(ACCR, TYPH)
#define	A_RF4	A_CONS(ACCR, TYPF4)
/*
 *	For writing
 */
#define	A_WB	A_CONS(ACCW, TYPB)
#define	A_WW	A_CONS(ACCW, TYPW)
#define	A_WL	A_CONS(ACCW, TYPL)
#define	A_WQ	A_CONS(ACCW, TYPQ)
#define	A_WO	A_CONS(ACCW, TYPO)
#define	A_WF	A_CONS(ACCW, TYPF)
#define	A_WD	A_CONS(ACCW, TYPD)
#define	A_WG	A_CONS(ACCW, TYPG)
#define	A_WH	A_CONS(ACCW, TYPH)

#ifndef INSTTAB
/*
 *	Define what the entries in the table look like.
 *	This is only used for adb and sdb; not for as.
 */
#define	INSTTAB
struct insttab{
	char	*iname;
	u_char	eopcode;
	u_char	popcode;
    char 	fmt;
	char	nargs;
	u_char	argtype[6];
} insttab[];

#define OP(name,eopcode,popdcode,fmt,nargs,a1,a2,a3,a4,a5,a6) {name,eopcode,popdcode,fmt,nargs,a1,a2,a3,a4,a5,a6}


#endif /* INSTTAB */

/*
 *	Definitions for the escape bytes
 */
#define	CORE	0
#define	NEW	1
#define	ESCD	0xfd
#define	ESCF	0xff

/*
 * Definitions for vector instruction processing.
 */
#define FMT1      1
#define FMT2      (FMT1 + 1)
#define FMT3      (FMT2 + 1)
#define FMT4      (FMT3 + 1)
#define FMT5      (FMT4 + 1)
#define FMT6      (FMT5 + 1)
#define FMT7      (FMT6 + 1)
#define FMT8      (FMT7 + 1)
#define FMT9      (FMT8 + 1)
#define FMT10     (FMT9 + 1)
#define FMT11     (FMT10 + 1)
#define FMT12     (FMT11 + 1)
#define FMT13     (FMT12 + 1)
#define FMT14     (FMT13 + 1)
#define FMT15     (FMT14 + 1)
#define FMT16     (FMT15 + 1)
#define FMTMAX    (FMT16 + 1)

/* VVCMP/VSCMP instruction COMPARE codes */
#define NONE      0
#define GTR       0
#define EQL       1
#define LSS       2
#define LEQ       4
#define NEQ       5
#define GEQ       6

/* VVCVT instruction CONVERT codes */
#define LF        0x1
#define LD        0x2
#define LG        0x3
#define FL        0x4
#define RFL       0x5
#define FD        0x6
#define FG        0x7
#define DL        0x8
#define DF        0x9
#define RDL       0xA
#define GL        0xC
#define GF        0xD
#define RGL       0xF

/* MTVP/MFVP instruction register codes */
#define VCR       0
#define VLR       1
#define VMRLO     2
#define VMRHI     3
#define SYNC      4
#define MSYNC     5

#define VSYNC     0
