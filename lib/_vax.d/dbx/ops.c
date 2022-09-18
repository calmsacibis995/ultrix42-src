/*@(#)ops.c	4.2	Ultrix	11/9/90*/

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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

/************************************************************************
 *									*
 *			Modification History				*
 *									*
 *  002	- Added support for vectors.							*
 *		  (L Miller, 18JAN90)									*
 *									*
 *	001 - Merged in 4.3 changes.					*
 *	      (Victoria Holt, April 29, 1986)				*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)ops.c	5.1 (Berkeley) 5/31/85";
#endif not lint

static char rcsid[] = "$Header: ops.c,v 1.5 84/12/26 10:41:07 linton Exp $";

/*
 * Machine operators.
 */

#include "defs.h"
#include "ops.h"

#ifndef public
typedef unsigned char VaxOpcode;

#define O_HALT 0x00		/* halt */
#define O_NOP 0x01		/* no operation */
#define O_REI 0x02		/* return from exception or interrupt */
#define O_BPT 0x03		/* break point fault */
#define O_RET 0x04		/* return from called procedure */
#define O_RSB 0x05		/* return from subroutine */
#define O_LDPCTX 0x06		/* load process context */
#define O_SVPCTX 0x07		/* save process context */
#define O_CVTPS 0x08		/* convert packed to leading separate numeric */
#define O_CVTSP 0x09		/* convert leading separate numeric to packed */
#define O_INDEX 0x0A		/* compute index */
#define O_CRC 0x0B		/* calculate cyclic redundancy check */
#define O_PROBER 0x0C		/* probe read access */
#define O_PROBEW 0x0D		/* probe write access */
#define O_INSQUE 0x0E		/* insert into queue */
#define O_REMQUE 0x0F		/* remove from queue */
#define O_BSBB 0x10		/* branch to subroutine with byte disp */
#define O_BRB 0x11		/* branch with byte disp */
#define O_BNEQ 0x12		/* branch on not equal (also BNEQU) */
#define O_BEQL 0x13		/* branch on equal (also BEQLU */
#define O_BGTR 0x14		/* branch on greater */
#define O_BLEQ 0x15		/* branch on less or equal */
#define O_JSB 0x16		/* jump to subroutine */
#define O_JMP 0x17		/* jump */
#define O_BGEQ 0x18		/* branch on greater or equal */
#define O_BLSS 0x19		/* branch on less */
#define O_BGTRU 0x1A		/* branch on greater unsigned */
#define O_BLEQU 0x1B		/* branch on less of equal unsigned */
#define O_BVC 0x1C		/* branch on overflow clear */
#define O_BVS 0x1D		/* branch on overflow set */
#define O_BCC 0x1E		/* branch on carry clear (also BGEQU) */
#define O_BCS 0x1F		/* branch on carry set (also BLSSU) */
#define O_ADDP4 0x20		/* add packed 4 operand */
#define O_ADDP6 0x21		/* add packed 6 operand */
#define O_SUBP4 0x22		/* subtract packed 4 operand */
#define O_SUBP6 0x23		/* subtract packed 6 operand */
#define O_CVTPT 0x24		/* convert packed to trailing numeric */
#define O_MULP 0x25		/* multiply packed */
#define O_CVTTP 0x26		/* convert trailing numeric to packed */
#define O_DIVP 0x27		/* divide packed */
#define O_MOVC3 0x28		/* move character 3 operand */
#define O_CMPC3 0x29		/* compare character 3 operand */
#define O_SCANC 0x2A		/* scan for character */
#define O_SPANC 0x2B		/* span characters */
#define O_MOVC5 0x2C		/* move character 5 operand */
#define O_CMPC5 0x2D		/* compare character 5 operand */
#define O_MOVTC 0x2E		/* move translated characters */
#define O_MOVTUC 0x2F		/* move translated until character */
#define O_BSBW 0x30		/* branch to subroutine with word disp */
#define O_BRW 0x31		/* branch with word disp */
#define O_CVTWL 0x32		/* convert word to long */
#define O_CVTWB 0x33		/* convert word to byte */
#define O_MOVP 0x34		/* move packed */
#define O_CMPP3 0x35		/* compare packed 3 operand */
#define O_CVTPL 0x36		/* convert packed to long */
#define O_CMPP4 0x37		/* compare packed 4 operand */
#define O_EDITPC 0x38		/* edit packed to character */
#define O_MATCHC 0x39		/* match characters */
#define O_LOCC 0x3A		/* locate characters */
#define O_SKPC 0x3B		/* skip character */
#define O_MOVZWL 0x3C		/* move zero-extended word to long */
#define O_ACBW 0x3D		/* add compare and branch word */
#define O_MOVAW 0x3E		/* move address of word */
#define O_PUSHAW 0x3F		/* push address of word */
#define O_ADDF2 0x40		/* add floating 2 operand */
#define O_ADDF3 0x41		/* add floating 3 operand */
#define O_SUBF2 0x42		/* subtract floating 2 operand */
#define O_SUBF3 0x43		/* subtract floating 3 operand */
#define O_MULF2 0x44		/* multiply floating 2 operand */
#define O_MULF3 0x45		/* multiply floating 3 operand */
#define O_DIVF2 0x46		/* divide floating 2 operand */
#define O_DIVF3 0x47		/* divide floating 3 operand */
#define O_CVTFB 0x48		/* convert float to byte */
#define O_CVTFW 0x49		/* convert float to word */
#define O_CVTFL 0x4A		/* convert float to long */
#define O_CVTRFL 0x4B		/* convert rounded float to long */
#define O_CVTBF 0x4C		/* convert byte to float */
#define O_CVTWF 0x4D		/* convert word to float */
#define O_CVTLF 0x4E		/* convert long to float */
#define O_ACBF 0x4F		/* add compare and branch floating */
#define O_MOVF 0x50		/* move float */
#define O_CMPF 0x51		/* compare floating */
#define O_MNEGF 0x52		/* move negated floating */
#define O_TSTF 0x53		/* test float */
#define O_EMODF 0x54		/* extended modulus floating */
#define O_POLYF 0x55		/* evaluate polynomial floating */
#define O_CVTFD 0x56		/* convert float to double */
#define O_DUMMY57 0x57		/* RESERVED to DIGITAL */
#define O_ADAWI 0x58		/* add aligned word interlocked */
#define O_DUMMY59 0x59		/* RESERVED to DIGITAL */
#define O_DUMMY5a 0x5A		/* RESERVED to DIGITAL */
#define O_DUMMY5b 0x5B		/* RESERVED to DIGITAL */
#define O_INSQHI 0x5C		/* insert into queue head, interlocked */
#define O_INSQTI 0x5D		/* insert into queue tail, interlocked */
#define O_REMQHI 0x5E		/* remove from queue head, interlocked */
#define O_REMQTI 0x5F		/* remove from queue tail, interlocked */
#define O_ADDD2 0x60		/* add double 2 operand */
#define O_ADDD3 0x61		/* add double 3 operand */
#define O_SUBD2 0x62		/* subtract double 2 operand */
#define O_SUBD3 0x63		/* subtrace double 3 operand */
#define O_MULD2 0x64		/* multiply double 2 operand */
#define O_MULD3 0x65		/* multiply double 3 operand */
#define O_DIVD2 0x66		/* divide double 2 operand */
#define O_DIVD3 0x67		/* divide double 3 operand */
#define O_CVTDB 0x68		/* convert double to byte */
#define O_CVTDW 0x69		/* convert double to word */
#define O_CVTDL 0x6A		/* convert double to long */
#define O_CVTRDL 0x6B		/* convert rounded double to long */
#define O_CVTBD 0x6C		/* convert byte to double */
#define O_CVTWD 0x6D		/* convert word to double */
#define O_CVTLD 0x6E		/* convert long to double */
#define O_ACBD 0x6F		/* add compare and branch double */
#define O_MOVD 0x70		/* move double */
#define O_CMPD 0x71		/* compare double */
#define O_MNEGD 0x72		/* move negated double */
#define O_TSTD 0x73		/* test double */
#define O_EMODD 0x74		/* extended modulus double */
#define O_POLYD 0x75		/* evaluate polynomial double */
#define O_CVTDF 0x76		/* convert double to float */
#define O_DUMMY77 0x77		/* RESERVED to DIGITAL */
#define O_ASHL 0x78		/* arithmetic shift long */
#define O_ASHQ 0x79		/* arithmetic shift quad */
#define O_EMUL 0x7A		/* extended multiply */
#define O_EDIV 0x7B		/* extended divide */
#define O_CLRQ 0x7C		/* clear quad (also CLRD) */
#define O_MOVQ 0x7D		/* move quad */
#define O_MOVAQ 0x7E		/* move address of quad (also MOVAD) */
#define O_PUSHAQ 0x7F		/* push address of quad (also PUSHAD) */
#define O_ADDB2 0x80		/* add byte 2 operand */
#define O_ADDB3 0x81		/* add byte 3 operand */
#define O_SUBB2 0x82		/* subtract byte 2 operand */
#define O_SUBB3 0x83		/* subtract byte 3 operand */
#define O_MULB2 0x84		/* multiply byte 2 operand */
#define O_MULB3 0x85		/* multiply byte 3 operand */
#define O_DIVB2 0x86		/* divide byte 2 operand */
#define O_DIVB3 0x87		/* divide byte 3 operand */
#define O_BISB2 0x88		/* bit set byte 2 operand */
#define O_BISB3 0x89		/* bit set byte 3 operand */
#define O_BICB2 0x8A		/* bit clear byte 2 operand */
#define O_BICB3 0x8B		/* bit clear byte 3 operand */
#define O_XORB2 0x8C		/* exclusive or byte 2 operand */
#define O_XORB3 0x8D		/* exclusive or byte 3 operand */
#define O_MNEGB 0x8E		/* move negated byte */
#define O_CASEB 0x8F		/* case byte */
#define O_MOVB 0x90		/* move byte */
#define O_CMPB 0x91		/* compare byte */
#define O_MCOMB 0x92		/* move complemented byte */
#define O_BITB 0x93		/* bit test byte */
#define O_CLRB 0x94		/* clear byte */
#define O_TSTB 0x95		/* test byte */
#define O_INCB 0x96		/* increment byte */
#define O_DECB 0x97		/* decrement byte */
#define O_CVTBL 0x98		/* convert byte to long */
#define O_CVTBW 0x99		/* convert byte to word */
#define O_MOVZBL 0x9A		/* move zero-extended byte to long */
#define O_MOVZBW 0x9B		/* move zero-extended byte to word */
#define O_ROTL 0x9C		/* rotate long */
#define O_ACBB 0x9D		/* add compare and branch byte */
#define O_MOVAB 0x9E		/* move address of byte */
#define O_PUSHAB 0x9F		/* push address of byte */
#define O_ADDW2 0xA0		/* add word 2 operand */
#define O_ADDW3 0xA1		/* add word 3 operand */
#define O_SUBW2 0xA2		/* subtract word 2 operand */
#define O_SUBW3 0xA3		/* subtract word 3 operand */
#define O_MULW2 0xA4		/* multiply word 2 operand */
#define O_MULW3 0xA5		/* multiply word 3 operand */
#define O_DIVW2 0xA6		/* divide word 2 operand */
#define O_DIVW3 0xA7		/* divide word 3 operand */
#define O_BISW2 0xA8		/* bit set word 2 operand */
#define O_BISW3 0xA9		/* bit set word 3 operand */
#define O_BICW2 0xAA		/* bit clear word 2 operand */
#define O_BICW3 0xAB		/* bit clear word 3 operand */
#define O_XORW2 0xAC		/* exclusive or word 2 operand */
#define O_XORW3 0xAD		/* exclusive or word 3 operand */
#define O_MNEGW 0xAE		/* move negated word */
#define O_CASEW 0xAF		/* case word */
#define O_MOVW 0xB0		/* move word */
#define O_CMPW 0xB1		/* compare word */
#define O_MCOMW 0xB2		/* move complemented word */
#define O_BITW 0xB3		/* bit test word */
#define O_CLRW 0xB4		/* clear word */
#define O_TSTW 0xB5		/* test word */
#define O_INCW 0xB6		/* increment word */
#define O_DECW 0xB7		/* decrement word */
#define O_BISPSW 0xB8		/* bit set processor status word */
#define O_BICPSW 0xB9		/* bit clear processor status word */
#define O_POPR 0xBA		/* pop register */
#define O_PUSHR 0xBB		/* push register */
#define O_CHMK 0xBC		/* change mode to kernel */
#define O_CHME 0xBD		/* change mode to executive */
#define O_CHMS 0xBE		/* change mode to supervisor */
#define O_CHMU 0xBF		/* change mode to user */
#define O_ADDL2 0xC0		/* add long 2 operand */
#define O_ADDL3 0xC1		/* add long 3 operand */
#define O_SUBL2 0xC2		/* subtract long 2 operand */
#define O_SUBL3 0xC3		/* subtract long 3 operand */
#define O_MULL2 0xC4		/* multiply long 2 operand */
#define O_MULL3 0xC5		/* multiply long 3 operand */
#define O_DIVL2 0xC6		/* divide long 2 operand */
#define O_DIVL3 0xC7		/* divide long 3 operand */
#define O_BISL2 0xC8		/* bit set long 2 operand */
#define O_BISL3 0xC9		/* bit set long 3 operand */
#define O_BICL2 0xCA		/* bit clear long 2 operand */
#define O_BICL3 0xCB		/* bit clear long 3 operand */
#define O_XORL2 0xCC		/* exclusive or long 2 operand */
#define O_XORL3 0xCD		/* exclusive or long 3 operand */
#define O_MNEGL 0xCE		/* move negated long */
#define O_CASEL 0xCF		/* case long */
#define O_MOVL 0xD0		/* move long */
#define O_CMPL 0xD1		/* compare long */
#define O_MCOML 0xD2		/* move complemented long */
#define O_BITL 0xD3		/* bit test long */
#define O_CLRL 0xD4		/* clear long (also CLRF) */
#define O_TSTL 0xD5		/* test long */
#define O_INCL 0xD6		/* increment long */
#define O_DECL 0xD7		/* decrement long */
#define O_ADWC 0xD8		/* add with carry */
#define O_SBWC 0xD9		/* subtrace with carry */
#define O_MTPR 0xDA		/* move to processor register */
#define O_MFPR 0xDB		/* move from processor register */
#define O_MOVPSL 0xDC		/* move processor status longword */
#define O_PUSHL 0xDD		/* push long */
#define O_MOVAL 0xDE		/* move address of long (also MOVAF) */
#define O_PUSHAL 0xDF		/* push address of long (also PUSHAF) */
#define O_BBS 0xE0		/* branch on bit set */
#define O_BBC 0xE1		/* branch on bit clear */
#define O_BBSS 0xE2		/* branch on bit set and set */
#define O_BBCS 0xE3		/* branch on bit clear and set */
#define O_BBSC 0xE4		/* branch on bit set and clear */
#define O_BBCC 0xE5		/* branch on bit clear and clear */
#define O_BBSSI 0xE6		/* branch on bit set and set interlocked */
#define O_BBCCI 0xE7		/* branch on bit clear and clear interlocked */
#define O_BLBS 0xE8		/* branch on low bit set */
#define O_BLBC 0xE9		/* branch on low bit clear */
#define O_FFS 0xEA		/* find first set bit */
#define O_FFC 0xEB		/* find first clear bit */
#define O_CMPV 0xEC		/* compare field */
#define O_CMPZV 0xED		/* compare zero-extended field */
#define O_EXTV 0xEE		/* extract field */
#define O_EXTZV 0xEF		/* extract zero-extended field */
#define O_INSV 0xF0		/* insert field */
#define O_ACBL 0xF1		/* add compare and branch long */
#define O_AOBLSS 0xF2		/* add one and branch on less */
#define O_AOBLEQ 0xF3		/* add one and branch on less or equal */
#define O_SOBGEQ 0xF4		/* subtract one and branch on gtr or equal */
#define O_SOBGTR 0xF5		/* subtract one and branch on greater */
#define O_CVTLB 0xF6		/* convert long to byte */
#define O_CVTLW 0xF7		/* convert long to word */
#define O_ASHP 0xF8		/* arithmetic shift and round packed */
#define O_CVTLP 0xF9		/* convert long to packed */
#define O_CALLG 0xFA		/* call with general argument list */
#define O_CALLS 0xFB		/* call with stack */
#define O_XFC 0xFC		/* extended function call */
#define O_ESCD 0xFD		/* ESCD to DIGITAL */
#define O_ESCE 0xFE		/* ESCE to DIGITAL */
#define O_ESCF 0xFF		/* ESCF to DIGITAL */

/* Vector instruction defines */
#define O_MFVP 0x31
#define O_VLDL 0x34
#define O_VGATHL 0x35
#define O_VLDQ 0x36
#define O_VGATHQ 0x37
#define O_VVADDL 0x80
#define O_VSADDL 0x81
#define O_VVADDG 0x82
#define O_VSADDG 0x83
#define O_VVADDF 0x84
#define O_VSADDF 0x85
#define O_VVADDD 0x86
#define O_VSADDD 0x87
#define O_VVSUBL 0x88
#define O_VSSUBL 0x89
#define O_VVSUBG 0x8a
#define O_VSSUBG 0x8b
#define O_VVSUBF 0x8c
#define O_VSSUBF 0x8d
#define O_VVSUBD 0x8e
#define O_VSSUBD 0x8f
#define O_VSTL 0x9c
#define O_VSCATL 0x9d
#define O_VSTQ 0x9e
#define O_VSCATQ 0x9f
#define O_VVMULL 0xa0
#define O_VSMULL 0xa1
#define O_VVMULG 0xa2
#define O_VSMULG 0xa3
#define O_VVMULF 0xa4
#define O_VSMULF 0xa5
#define O_VVMULD 0xa6
#define O_VSMULD 0xa7
#define O_VSYNC 0xa8
#define O_MTVP 0xa9
#define O_VVDIVG 0xaa
#define O_VSDIVG 0xab
#define O_VVDIVF 0xac
#define O_VSDIVF 0xad
#define O_VVDIVD 0xae
#define O_VSDIVD 0xaf
#define O_VVCMP  0xc0
#define O_VVCMPL 0xc0
#define O_VSCMPL 0xc1
#define O_VVCMPG 0xc2
#define O_VSCMPG 0xc3
#define O_VVCMPF 0xc4
#define O_VSCMPF 0xc5
#define O_VVCMPD 0xc6
#define O_VSCMPD 0xc7
#define O_VVBISL 0xc8
#define O_VSBISL 0xc9
#define O_VVBICL 0xcc
#define O_VSBICL 0xcd
#define O_VVSRLL 0xe0
#define O_VSSRLL 0xe1
#define O_VVSLLL 0xe4
#define O_VSSLLL 0xe5
#define O_VVXORL 0xe8
#define O_VSXORL 0xe9
#define O_VVCVT 0xec
#define O_IOTA 0xed
#define O_VVMERGE 0xee
#define O_VSMERGE 0xef
#define FIRST_EOP 0x31


/*
 * Opcode definitions.
 *
 * Much of this was taken from the assembler.
 */

/*
 * Argument access types.
 */

#define ACCR    0x10	/* read */
#define ACCW    0x20	/* write */
#define ACCM    0x30	/* modify */
#define ACCB    0x40	/* branch displacement */
#define ACCI    0x50	/* variable field */
#define ACCA    0x80    /* address only */

/*
 * Argument lengths.
 */

#define TYPB    0	/* byte */
#define TYPW    01	/* word */
#define TYPL    02	/* long */
#define TYPQ    03	/* quad */
#define TYPO    04      /* octa */
#define TYPF    05	/* floating */
#define TYPD    06	/* D floating */
#define TYPG    07	/* G floating */
#define TYPH    010	/* H floating */

/*
 * Addressing modes.
 */

#define LITSHORT    0x0	/* short literals */
#define LITUPTO31   0x1
#define LITUPTO47   0x2
#define LITUPTO63   0x3
#define INDEX       0x4 /* i[r] */
#define REG	    0x5 /* r */
#define REGDEF      0x6 /* (r) */
#define AUTODEC     0x7 /* -(r) */
#define AUTOINC     0x8 /* (r)+ */
#define AUTOINCDEF  0x9 /* *(r)+ */
#define BYTEDISP    0xA /* BD(r) */
#define BYTEDISPDEF 0xB /* *BD(r) */
#define WORDDISP    0xC /* WD(r) */
#define WORDDISPDEF 0xD /* *WD(r) */
#define LONGDISP    0xE /* LD(r) */
#define LONGDISPDEF 0xF /* *LD(r) */

/*
 * Vector instruction formats.
 */
#define FMT1        1
#define FMT2        2
#define FMT3	    3
#define FMT4        4
#define FMT5	    5
#define FMT6	    6
#define FMT7	    7
#define FMT8	    8
#define FMT9	    9
#define FMT10	    10
#define FMT11	    11
#define FMT12	    12
#define FMT13	    13
#define FMT14	    14
#define FMT15	    15
#define FMT16	    16

#define mi          (1)
#define exc         (1)
#define mtf         (1<<1)
#define moe         (1<<2)

#define vreg        1
#define cmp         2
#define cvt         3

#define sync        0x04
#define msync       0x05

#define is_branch_disp(arg) ((arg & ACCB) != 0)
#define typelen(arg)        (arg & 0xF)
#define regnm(mode)	    (mode & 0xF)
#define addrmode(mode)      (mode >> 4)

/*
 * Operator information structure.
 */

typedef struct {
    char *iname;
    char val;
    char fmt;
    char numargs;
    char argtype[6];
} Optab;

/*
 * Vector instruction control word.
 */

typedef struct {
  unsigned vc:4;             /* bits 0-3 */
  unsigned vb:4;             /* bits 4-7 */
  unsigned va:4;             /* bits 8-11 */
  unsigned unused:1;         /* bit  12 */
  unsigned modifiers:3  ;    /* bit  13-15 */
} Ctrl_word;

#endif

public Optab optab[] = {
    {"halt", 0x00, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {"nop", 0x01, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {"rei", 0x02, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {"bpt", 0x03, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {"ret", 0x04, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {"rsb", 0x05, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {"ldpctx", 0x06, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {"svpctx", 0x07, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {"cvtps", 0x08, 0, 4, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, 0, 0}, 
    {"cvtsp", 0x09, 0, 4, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, 0, 0}, 
    {"index", 0x0a, 0, 6, ACCR+TYPL, ACCR+TYPL, ACCR+TYPL, ACCR+TYPL, ACCR+TYPL, ACCW+TYPL}, 
    {"crc", 0x0b, 0, 4, ACCA+TYPB, ACCR+TYPL, ACCR+TYPW, ACCA+TYPB, 0, 0}, 
    {"prober", 0x0c, 0, 3, ACCR+TYPB, ACCR+TYPW, ACCA+TYPB, 0, 0, 0}, 
    {"probew", 0x0d, 0, 3, ACCR+TYPB, ACCR+TYPW, ACCA+TYPB, 0, 0, 0}, 
    {"insque", 0x0e, 0, 2, ACCA+TYPB, ACCA+TYPB, 0, 0, 0, 0}, 
    {"remque", 0x0f, 0, 2, ACCA+TYPB, ACCW+TYPL, 0, 0, 0, 0}, 
    {"bsbb", 0x10, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"brb", 0x11, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"bneq", 0x12, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"beql", 0x13, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"bgtr", 0x14, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"bleq", 0x15, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"jsb", 0x16, 0, 1, ACCA+TYPB, 0, 0, 0, 0, 0}, 
    {"jmp", 0x17, 0, 1, ACCA+TYPB, 0, 0, 0, 0, 0}, 
    {"bgeq", 0x18, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"blss", 0x19, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"bgtru", 0x1a, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"blequ", 0x1b, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"bvc", 0x1c, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"bvs", 0x1d, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"bcc", 0x1e, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"bcs", 0x1f, 0, 1, ACCB+TYPB, 0, 0, 0, 0, 0}, 
    {"addp4", 0x20, 0, 4, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, 0, 0}, 
    {"addp6", 0x21, 0, 6, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB}, 
    {"subp4", 0x22, 0, 4, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, 0, 0}, 
    {"subp6", 0x23, 0, 6, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB}, 
    {"cvtpt", 0x24, 0, 5, ACCR+TYPW, ACCA+TYPB, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, 0}, 
    {"mulp", 0x25, 0, 6, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB}, 
    {"cvttp", 0x26, 0, 5, ACCR+TYPW, ACCA+TYPB, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, 0}, 
    {"divp", 0x27, 0, 6, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB}, 
    {"movc3", 0x28, 0, 3, ACCR+TYPW, ACCA+TYPB, ACCA+TYPB, 0, 0, 0}, 
    {"cmpc3", 0x29, 0, 3, ACCR+TYPW, ACCA+TYPB, ACCA+TYPB, 0, 0, 0}, 
    {"scanc", 0x2a, 0, 4, ACCR+TYPW, ACCA+TYPB, ACCA+TYPB, ACCR+TYPB, 0, 0}, 
    {"spanc", 0x2b, 0, 4, ACCR+TYPW, ACCA+TYPB, ACCA+TYPB, ACCR+TYPB, 0, 0}, 
    {"movc5", 0x2c, 0, 5, ACCR+TYPW, ACCA+TYPB, ACCR+TYPB, ACCR+TYPW, ACCA+TYPB, 0}, 
    {"cmpc5", 0x2d, 0, 5, ACCR+TYPW, ACCA+TYPB, ACCR+TYPB, ACCR+TYPW, ACCA+TYPB, 0}, 
    {"movtc", 0x2e, 0, 6, ACCR+TYPW, ACCA+TYPB, ACCR+TYPB, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB}, 
    {"movtuc", 0x2f, 0, 6, ACCR+TYPW, ACCA+TYPB, ACCR+TYPB, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB}, 
    {"bsbw", 0x30, 0, 1, ACCB+TYPW, 0, 0, 0, 0, 0}, 
    {"brw", 0x31, 0, 1, ACCB+TYPW, 0, 0, 0, 0, 0}, 
    {"cvtwl", 0x32, 0, 2, ACCR+TYPW, ACCW+TYPL, 0, 0, 0, 0}, 
    {"cvtwb", 0x33, 0, 2, ACCR+TYPW, ACCW+TYPB, 0, 0, 0, 0}, 
    {"movp", 0x34, 0, 3, ACCR+TYPW, ACCA+TYPB, ACCA+TYPB, 0, 0, 0}, 
    {"cmpp3", 0x35, 0, 3, ACCR+TYPW, ACCA+TYPB, ACCA+TYPB, 0, 0, 0}, 
    {"cvtpl", 0x36, 0, 3, ACCR+TYPW, ACCA+TYPB, ACCW+TYPL, 0, 0, 0}, 
    {"cmpp4", 0x37, 0, 4, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, 0, 0}, 
    {"editpc", 0x38, 0, 4, ACCR+TYPW, ACCA+TYPB, ACCA+TYPB, ACCA+TYPB, 0, 0}, 
    {"matchc", 0x39, 0, 4, ACCR+TYPW, ACCA+TYPB, ACCR+TYPW, ACCA+TYPB, 0, 0}, 
    {"locc", 0x3a, 0, 3, ACCR+TYPB, ACCR+TYPW, ACCA+TYPB, 0, 0, 0}, 
    {"skpc", 0x3b, 0, 3, ACCR+TYPB, ACCR+TYPW, ACCA+TYPB, 0, 0, 0}, 
    {"movzwl", 0x3c, 0, 2, ACCR+TYPW, ACCW+TYPL, 0, 0, 0, 0}, 
    {"acbw", 0x3d, 0, 4, ACCR+TYPW, ACCR+TYPW, ACCM+TYPW, ACCB+TYPW, 0, 0}, 
    {"movaw", 0x3e, 0, 2, ACCA+TYPW, ACCW+TYPL, 0, 0, 0, 0}, 
    {"pushaw", 0x3f, 0, 1, ACCA+TYPW, 0, 0, 0, 0, 0}, 
    {"addf2", 0x40, 0, 2, ACCR+TYPF, ACCM+TYPF, 0, 0, 0, 0}, 
    {"addf3", 0x41, 0, 3, ACCR+TYPF, ACCR+TYPF, ACCW+TYPF, 0, 0, 0}, 
    {"subf2", 0x42, 0, 2, ACCR+TYPF, ACCM+TYPF, 0, 0, 0, 0}, 
    {"subf3", 0x43, 0, 3, ACCR+TYPF, ACCR+TYPF, ACCW+TYPF, 0, 0, 0}, 
    {"mulf2", 0x44, 0, 2, ACCR+TYPF, ACCM+TYPF, 0, 0, 0, 0}, 
    {"mulf3", 0x45, 0, 3, ACCR+TYPF, ACCR+TYPF, ACCW+TYPF, 0, 0, 0}, 
    {"divf2", 0x46, 0, 2, ACCR+TYPF, ACCM+TYPF, 0, 0, 0, 0}, 
    {"divf3", 0x47, 0, 3, ACCR+TYPF, ACCR+TYPF, ACCW+TYPF, 0, 0, 0}, 
    {"cvtfb", 0x48, 0, 2, ACCR+TYPF, ACCW+TYPB, 0, 0, 0, 0}, 
    {"cvtfw", 0x49, 0, 2, ACCR+TYPF, ACCW+TYPW, 0, 0, 0, 0}, 
    {"cvtfl", 0x4a, 0, 2, ACCR+TYPF, ACCW+TYPL, 0, 0, 0, 0}, 
    {"cvtrfl", 0x4b, 0, 2, ACCR+TYPF, ACCW+TYPL, 0, 0, 0, 0}, 
    {"cvtbf", 0x4c, 0, 2, ACCR+TYPB, ACCW+TYPB, 0, 0, 0, 0}, 
    {"cvtwf", 0x4d, 0, 2, ACCR+TYPW, ACCW+TYPF, 0, 0, 0, 0}, 
    {"cvtlf", 0x4e, 0, 2, ACCR+TYPL, ACCW+TYPF, 0, 0, 0, 0}, 
    {"acbf", 0x4f, 0, 4, ACCR+TYPF, ACCR+TYPF, ACCM+TYPF, ACCB+TYPW, 0, 0}, 
    {"movf", 0x50, 0, 2, ACCR+TYPF, ACCW+TYPF, 0, 0, 0, 0}, 
    {"cmpf", 0x51, 0, 2, ACCR+TYPF, ACCR+TYPF, 0, 0, 0, 0}, 
    {"mnegf", 0x52, 0, 2, ACCR+TYPF, ACCW+TYPF, 0, 0, 0, 0}, 
    {"tstf", 0x53, 0, 1, ACCR+TYPF, 0, 0, 0, 0, 0}, 
    {"emodf", 0x54, 0, 5, ACCR+TYPF, ACCR+TYPB, ACCR+TYPF, ACCW+TYPL, ACCW+TYPF, 0}, 
    {"polyf", 0x55, 0, 3, ACCR+TYPF, ACCR+TYPW, ACCA+TYPB, 0, 0, 0}, 
    {"cvtfd", 0x56, 0, 2, ACCR+TYPF, ACCW+TYPD, 0, 0, 0, 0}, 
    {"dummy", 0x57, 0, 0, 0, 0, 0, 0, 0, 0},
    {"adawi", 0x58, 0, 2, ACCR+TYPW, ACCM+TYPW, 0, 0, 0, 0}, 
    {"dummy", 0x59, 0, 0, 0, 0, 0, 0, 0, 0},
    {"dummy", 0x5a, 0, 0, 0, 0, 0, 0, 0, 0},
    {"dummy", 0x5b, 0, 0, 0, 0, 0, 0, 0, 0},
    {"dummy", 0x5c, 0, 0, 0, 0, 0, 0, 0, 0},
    {"dummy", 0x5d, 0, 0, 0, 0, 0, 0, 0, 0},
    {"dummy", 0x5e, 0, 0, 0, 0, 0, 0, 0, 0},
    {"dummy", 0x5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"addd2", 0x60, 0, 2, ACCR+TYPD, ACCM+TYPD, 0, 0, 0, 0}, 
    {"addd3", 0x61, 0, 3, ACCR+TYPD, ACCR+TYPD, ACCW+TYPD, 0, 0, 0}, 
    {"subd2", 0x62, 0, 2, ACCR+TYPD, ACCM+TYPD, 0, 0, 0, 0}, 
    {"subd3", 0x63, 0, 3, ACCR+TYPD, ACCR+TYPD, ACCW+TYPD, 0, 0, 0}, 
    {"muld2", 0x64, 0, 2, ACCR+TYPD, ACCM+TYPD, 0, 0, 0, 0}, 
    {"muld3", 0x65, 0, 3, ACCR+TYPD, ACCR+TYPD, ACCW+TYPD, 0, 0, 0}, 
    {"divd2", 0x66, 0, 2, ACCR+TYPD, ACCM+TYPD, 0, 0, 0, 0}, 
    {"divd3", 0x67, 0, 3, ACCR+TYPD, ACCR+TYPD, ACCR+TYPD, 0, 0, 0}, 
    {"cvtdb", 0x68, 0, 2, ACCR+TYPD, ACCW+TYPB, 0, 0, 0, 0}, 
    {"cvtdw", 0x69, 0, 2, ACCR+TYPD, ACCW+TYPW, 0, 0, 0, 0}, 
    {"cvtdl", 0x6a, 0, 2, ACCR+TYPD, ACCW+TYPL, 0, 0, 0, 0}, 
    {"cvtrdl", 0x6b, 0, 2, ACCR+TYPD, ACCW+TYPL, 0, 0, 0, 0}, 
    {"cvtbd", 0x6c, 0, 2, ACCR+TYPB, ACCW+TYPD, 0, 0, 0, 0}, 
    {"cvtwd", 0x6d, 0, 2, ACCR+TYPW, ACCW+TYPD, 0, 0, 0, 0}, 
    {"cvtld", 0x6e, 0, 2, ACCR+TYPL, ACCW+TYPD, 0, 0, 0, 0}, 
    {"acbd", 0x6f, 0, 4, ACCR+TYPD, ACCR+TYPD, ACCM+TYPD, ACCB+TYPW, 0, 0}, 
    {"movd", 0x70, 0, 2, ACCR+TYPD, ACCW+TYPD, 0, 0, 0, 0}, 
    {"cmpd", 0x71, 0, 2, ACCR+TYPD, ACCR+TYPD, 0, 0, 0, 0}, 
    {"mnegd", 0x72, 0, 2, ACCR+TYPD, ACCW+TYPD, 0, 0, 0, 0}, 
    {"tstd", 0x73, 0, 1, ACCR+TYPD, 0, 0, 0, 0, 0}, 
    {"emodd", 0x74, 0, 5, ACCR+TYPD, ACCR+TYPB, ACCR+TYPD, ACCW+TYPL, ACCW+TYPD, 0}, 
    {"polyd", 0x75, 0, 3, ACCR+TYPD, ACCR+TYPW, ACCA+TYPB, 0, 0, 0}, 
    {"cvtdf", 0x76, 0, 2, ACCR+TYPD, ACCW+TYPF, 0, 0, 0, 0}, 
    {"dummy0x77", 0x77, 0, 0, 0, 0, 0, 0, 0, 0},
    {"ashl", 0x78, 0, 3, ACCR+TYPB, ACCR+TYPL, ACCW+TYPL, 0, 0, 0}, 
    {"ashq", 0x79, 0, 3, ACCR+TYPB, ACCR+TYPQ, ACCW+TYPQ, 0, 0, 0}, 
    {"emul", 0x7a, 0, 4, ACCR+TYPL, ACCR+TYPL, ACCR+TYPL, ACCW+TYPQ, 0, 0}, 
    {"ediv", 0x7b, 0, 4, ACCR+TYPL, ACCR+TYPQ, ACCW+TYPL, ACCW+TYPL, 0, 0}, 
    {"clrq", 0x7c, 0, 1, ACCW+TYPD, 0, 0, 0, 0, 0}, 
    {"movq", 0x7d, 0, 2, ACCR+TYPQ, ACCW+TYPQ, 0, 0, 0, 0}, 
    {"movaq", 0x7e, 0, 2, ACCA+TYPQ, ACCW+TYPL, 0, 0, 0, 0}, 
    {"pushaq", 0x7f, 0, 1, ACCA+TYPQ, 0, 0, 0, 0, 0}, 
    {"addb2", 0x80, 0, 2, ACCR+TYPB, ACCM+TYPB, 0, 0, 0, 0}, 
    {"addb3", 0x81, 0, 3, ACCR+TYPB, ACCR+TYPB, ACCW+TYPB, 0, 0, 0}, 
    {"subb2", 0x82, 0, 2, ACCR+TYPB, ACCM+TYPB, 0, 0, 0, 0}, 
    {"subb3", 0x83, 0, 3, ACCR+TYPB, ACCR+TYPB, ACCW+TYPB, 0, 0, 0}, 
    {"mulb2", 0x84, 0, 2, ACCR+TYPB, ACCM+TYPB, 0, 0, 0, 0}, 
    {"mulb3", 0x85, 0, 3, ACCR+TYPB, ACCR+TYPB, ACCW+TYPB, 0, 0, 0}, 
    {"divb2", 0x86, 0, 2, ACCR+TYPB, ACCM+TYPB, 0, 0, 0, 0}, 
    {"divb3", 0x87, 0, 3, ACCR+TYPB, ACCR+TYPB, ACCW+TYPB, 0, 0, 0}, 
    {"bisb2", 0x88, 0, 2, ACCR+TYPB, ACCM+TYPB, 0, 0, 0, 0}, 
    {"bisb3", 0x89, 0, 3, ACCR+TYPB, ACCR+TYPB, ACCW+TYPB, 0, 0, 0}, 
    {"bicb2", 0x8a, 0, 2, ACCR+TYPB, ACCM+TYPB, 0, 0, 0, 0}, 
    {"bicb3", 0x8b, 0, 3, ACCR+TYPB, ACCR+TYPB, ACCW+TYPB, 0, 0, 0}, 
    {"xorb2", 0x8c, 0, 2, ACCR+TYPB, ACCM+TYPB, 0, 0, 0, 0}, 
    {"xorb3", 0x8d, 0, 3, ACCR+TYPB, ACCR+TYPB, ACCW+TYPB, 0, 0, 0}, 
    {"mnegb", 0x8e, 0, 2, ACCR+TYPB, ACCW+TYPB, 0, 0, 0, 0}, 
    {"caseb", 0x8f, 0, 3, ACCR+TYPB, ACCR+TYPB, ACCR+TYPB, 0, 0, 0}, 
    {"movb", 0x90, 0, 2, ACCR+TYPB, ACCW+TYPB, 0, 0, 0, 0}, 
    {"cmpb", 0x91, 0, 2, ACCR+TYPB, ACCR+TYPB, 0, 0, 0, 0}, 
    {"mcomb", 0x92, 0, 2, ACCR+TYPB, ACCW+TYPB, 0, 0, 0, 0}, 
    {"bitb", 0x93, 0, 2, ACCR+TYPB, ACCR+TYPB, 0, 0, 0, 0}, 
    {"clrb", 0x94, 0, 1, ACCW+TYPB, 0, 0, 0, 0, 0}, 
    {"tstb", 0x95, 0, 1, ACCR+TYPB, 0, 0, 0, 0, 0}, 
    {"incb", 0x96, 0, 1, ACCM+TYPB, 0, 0, 0, 0, 0}, 
    {"decb", 0x97, 0, 1, ACCM+TYPB, 0, 0, 0, 0, 0}, 
    {"cvtbl", 0x98, 0, 2, ACCR+TYPB, ACCW+TYPL, 0, 0, 0, 0}, 
    {"cvtbw", 0x99, 0, 2, ACCR+TYPB, ACCW+TYPW, 0, 0, 0, 0}, 
    {"movzbl", 0x9a, 0, 2, ACCR+TYPB, ACCW+TYPL, 0, 0, 0, 0}, 
    {"movzbw", 0x9b, 0, 2, ACCR+TYPB, ACCW+TYPW, 0, 0, 0, 0}, 
    {"rotl", 0x9c, 0, 3, ACCR+TYPB, ACCR+TYPL, ACCW+TYPL, 0, 0, 0}, 
    {"acbb", 0x9d, 0, 4, ACCR+TYPB, ACCR+TYPB, ACCM+TYPB, ACCB+TYPW, 0, 0}, 
    {"movab", 0x9e, 0, 2, ACCA+TYPB, ACCW+TYPL, 0, 0, 0, 0}, 
    {"pushab", 0x9f, 0, 1, ACCA+TYPB, 0, 0, 0, 0, 0}, 
    {"addw2", 0xa0, 0, 2, ACCR+TYPW, ACCM+TYPW, 0, 0, 0, 0}, 
    {"addw3", 0xa1, 0, 3, ACCR+TYPW, ACCR+TYPW, ACCW+TYPW, 0, 0, 0}, 
    {"subw2", 0xa2, 0, 2, ACCR+TYPW, ACCM+TYPW, 0, 0, 0, 0}, 
    {"subw3", 0xa3, 0, 3, ACCR+TYPW, ACCR+TYPW, ACCW+TYPW, 0, 0, 0}, 
    {"mulw2", 0xa4, 0, 2, ACCR+TYPW, ACCM+TYPW, 0, 0, 0, 0}, 
    {"mulw3", 0xa5, 0, 3, ACCR+TYPW, ACCR+TYPW, ACCW+TYPW, 0, 0, 0}, 
    {"divw2", 0xa6, 0, 2, ACCR+TYPW, ACCM+TYPW, 0, 0, 0, 0}, 
    {"divw3", 0xa7, 0, 3, ACCR+TYPW, ACCR+TYPW, ACCW+TYPW, 0, 0, 0}, 
    {"bisw2", 0xa8, 0, 2, ACCR+TYPW, ACCM+TYPW, 0, 0, 0, 0}, 
    {"bisw3", 0xa9, 0, 3, ACCR+TYPW, ACCR+TYPW, ACCW+TYPW, 0, 0, 0}, 
    {"bicw2", 0xaa, 0, 2, ACCR+TYPW, ACCM+TYPW, 0, 0, 0, 0}, 
    {"bicw3", 0xab, 0, 3, ACCR+TYPW, ACCR+TYPW, ACCW+TYPW, 0, 0, 0}, 
    {"xorw2", 0xac, 0, 2, ACCR+TYPW, ACCM+TYPW, 0, 0, 0, 0}, 
    {"xorw3", 0xad, 0, 3, ACCR+TYPW, ACCR+TYPW, ACCW+TYPW, 0, 0, 0}, 
    {"mnegw", 0xae, 0, 2, ACCR+TYPW, ACCW+TYPL, 0, 0, 0, 0}, 
    {"casew", 0xaf, 0, 3, ACCR+TYPW, ACCR+TYPW, ACCR+TYPW, 0, 0, 0}, 
    {"movw", 0xb0, 0, 2, ACCR+TYPW, ACCW+TYPW, 0, 0, 0, 0}, 
    {"cmpw", 0xb1, 0, 2, ACCR+TYPW, ACCR+TYPW, 0, 0, 0, 0}, 
    {"mcomw", 0xb2, 0, 2, ACCR+TYPW, ACCW+TYPW, 0, 0, 0, 0}, 
    {"bitw", 0xb3, 0, 2, ACCR+TYPW, ACCR+TYPW, 0, 0, 0, 0}, 
    {"clrw", 0xb4, 0, 1, ACCW+TYPW, 0, 0, 0, 0, 0}, 
    {"tstw", 0xb5, 0, 1, ACCR+TYPW, 0, 0, 0, 0, 0}, 
    {"incw", 0xb6, 0, 1, ACCM+TYPW, 0, 0, 0, 0, 0}, 
    {"decw", 0xb7, 0, 1, ACCM+TYPW, 0, 0, 0, 0, 0}, 
    {"bispsw", 0xb8, 0, 1, ACCR+TYPW, 0, 0, 0, 0, 0}, 
    {"bicpsw", 0xb9, 0, 1, ACCR+TYPW, 0, 0, 0, 0, 0}, 
    {"popr", 0xba, 0, 1, ACCR+TYPW, 0, 0, 0, 0, 0}, 
    {"pushr", 0xbb, 0, 1, ACCR+TYPW, 0, 0, 0, 0, 0}, 
    {"chmk", 0xbc, 0, 1, ACCR+TYPW, 0, 0, 0, 0, 0}, 
    {"chme", 0xbd, 0, 1, ACCR+TYPW, 0, 0, 0, 0, 0}, 
    {"chms", 0xbe, 0, 1, ACCR+TYPW, 0, 0, 0, 0, 0}, 
    {"chmu", 0xbf, 0, 1, ACCR+TYPW, 0, 0, 0, 0, 0}, 
    {"addl2", 0xc0, 0, 2, ACCR+TYPL, ACCM+TYPL, 0, 0, 0, 0}, 
    {"addl3", 0xc1, 0, 3, ACCR+TYPL, ACCR+TYPL, ACCW+TYPL, 0, 0, 0}, 
    {"subl2", 0xc2, 0, 2, ACCR+TYPL, ACCM+TYPL, 0, 0, 0, 0}, 
    {"subl3", 0xc3, 0, 3, ACCR+TYPL, ACCR+TYPL, ACCW+TYPL, 0, 0, 0}, 
    {"mull2", 0xc4, 0, 2, ACCR+TYPL, ACCM+TYPL, 0, 0, 0, 0}, 
    {"mull3", 0xc5, 0, 3, ACCR+TYPL, ACCR+TYPL, ACCW+TYPL, 0, 0, 0}, 
    {"divl2", 0xc6, 0, 2, ACCR+TYPL, ACCM+TYPL, 0, 0, 0, 0}, 
    {"divl3", 0xc7, 0, 3, ACCR+TYPL, ACCR+TYPL, ACCW+TYPL, 0, 0, 0}, 
    {"bisl2", 0xc8, 0, 2, ACCR+TYPL, ACCM+TYPL, 0, 0, 0, 0}, 
    {"bisl3", 0xc9, 0, 3, ACCR+TYPL, ACCR+TYPL, ACCW+TYPL, 0, 0, 0}, 
    {"bicl2", 0xca, 0, 2, ACCR+TYPL, ACCM+TYPL, 0, 0, 0, 0}, 
    {"bicl3", 0xcb, 0, 3, ACCR+TYPL, ACCR+TYPL, ACCW+TYPL, 0, 0, 0}, 
    {"xorl2", 0xcc, 0, 2, ACCR+TYPL, ACCM+TYPL, 0, 0, 0, 0}, 
    {"xorl3", 0xcd, 0, 3, ACCR+TYPL, ACCR+TYPL, ACCW+TYPL, 0, 0, 0}, 
    {"mnegl", 0xce, 0, 2, ACCR+TYPL, ACCW+TYPL, 0, 0, 0, 0}, 
    {"casel", 0xcf, 0, 3, ACCR+TYPL, ACCR+TYPL, ACCR+TYPL, 0, 0, 0}, 
    {"movl", 0xd0, 0, 2, ACCR+TYPL, ACCW+TYPL, 0, 0, 0, 0}, 
    {"cmpl", 0xd1, 0, 2, ACCR+TYPL, ACCR+TYPL, 0, 0, 0, 0}, 
    {"mcoml", 0xd2, 0, 2, ACCR+TYPL, ACCW+TYPL, 0, 0, 0, 0}, 
    {"bitl", 0xd3, 0, 2, ACCR+TYPL, ACCR+TYPL, 0, 0, 0, 0}, 
    {"clrl", 0xd4, 0, 1, ACCW+TYPL, 0, 0, 0, 0, 0}, 
    {"tstl", 0xd5, 0, 1, ACCR+TYPL, 0, 0, 0, 0, 0}, 
    {"incl", 0xd6, 0, 1, ACCM+TYPL, 0, 0, 0, 0, 0}, 
    {"decl", 0xd7, 0, 1, ACCM+TYPL, 0, 0, 0, 0, 0}, 
    {"adwc", 0xd8, 0, 2, ACCR+TYPL, ACCM+TYPL, 0, 0, 0, 0}, 
    {"sbwc", 0xd9, 0, 2, ACCR+TYPL, ACCM+TYPL, 0, 0, 0, 0}, 
    {"mtpr", 0xda, 0, 2, ACCR+TYPL, ACCR+TYPL, 0, 0, 0, 0}, 
    {"mfpr", 0xdb, 0, 2, ACCR+TYPL, ACCW+TYPL, 0, 0, 0, 0}, 
    {"movpsl", 0xdc, 0, 1, ACCW+TYPL, 0, 0, 0, 0, 0}, 
    {"pushl", 0xdd, 0, 1, ACCR+TYPL, 0, 0, 0, 0, 0}, 
    {"moval", 0xde, 0, 2, ACCA+TYPL, ACCW+TYPL, 0, 0, 0, 0}, 
    {"pushal", 0xdf, 0, 1, ACCA+TYPL, 0, 0, 0, 0, 0}, 
    {"bbs", 0xe0, 0, 3, ACCR+TYPL, ACCR+TYPB, ACCB+TYPB, 0, 0, 0}, 
    {"bbc", 0xe1, 0, 3, ACCR+TYPL, ACCR+TYPB, ACCB+TYPB, 0, 0, 0}, 
    {"bbss", 0xe2, 0, 3, ACCR+TYPL, ACCR+TYPB, ACCB+TYPB, 0, 0, 0}, 
    {"bbcs", 0xe3, 0, 3, ACCR+TYPL, ACCR+TYPB, ACCB+TYPB, 0, 0, 0}, 
    {"bbsc", 0xe4, 0, 3, ACCR+TYPL, ACCR+TYPB, ACCB+TYPB, 0, 0, 0}, 
    {"bbcc", 0xe5, 0, 3, ACCR+TYPL, ACCR+TYPB, ACCB+TYPB, 0, 0, 0}, 
    {"bbssi", 0xe6, 0, 3, ACCR+TYPL, ACCR+TYPB, ACCB+TYPB, 0, 0, 0}, 
    {"bbcci", 0xe7, 0, 3, ACCR+TYPL, ACCR+TYPB, ACCB+TYPB, 0, 0, 0}, 
    {"blbs", 0xe8, 0, 2, ACCR+TYPL, ACCB+TYPB, 0, 0, 0, 0}, 
    {"blbc", 0xe9, 0, 2, ACCR+TYPL, ACCB+TYPB, 0, 0, 0, 0}, 
    {"ffs", 0xea, 0, 4, ACCR+TYPL, ACCR+TYPB, ACCR+TYPB, ACCW+TYPL, 0, 0}, 
    {"ffc", 0xeb, 0, 4, ACCR+TYPL, ACCR+TYPB, ACCR+TYPB, ACCW+TYPL, 0, 0}, 
    {"cmpv", 0xec, 0, 4, ACCR+TYPL, ACCR+TYPB, ACCR+TYPB, ACCR+TYPL, 0, 0}, 
    {"cmpzv", 0xed, 0, 4, ACCR+TYPL, ACCR+TYPB, ACCR+TYPB, ACCR+TYPL, 0, 0}, 
    {"extv", 0xee, 0, 4, ACCR+TYPL, ACCR+TYPB, ACCR+TYPB, ACCW+TYPL, 0, 0}, 
    {"extzv", 0xef, 0, 4, ACCR+TYPL, ACCR+TYPB, ACCR+TYPB, ACCW+TYPL, 0, 0}, 
    {"insv", 0xf0, 0, 4, ACCR+TYPL, ACCR+TYPL, ACCR+TYPB, ACCW+TYPB, 0, 0}, 
    {"acbl", 0xf1, 0, 4, ACCR+TYPL, ACCR+TYPL, ACCM+TYPL, ACCB+TYPW, 0, 0}, 
    {"aoblss", 0xf2, 0, 3, ACCR+TYPL, ACCM+TYPL, ACCB+TYPB, 0, 0, 0}, 
    {"aobleq", 0xf3, 0, 3, ACCR+TYPL, ACCM+TYPL, ACCB+TYPB, 0, 0, 0}, 
    {"sobgeq", 0xf4, 0, 2, ACCM+TYPL, ACCB+TYPB, 0, 0, 0, 0}, 
    {"sobgtr", 0xf5, 0, 2, ACCM+TYPL, ACCB+TYPB, 0, 0, 0, 0}, 
    {"cvtlb", 0xf6, 0, 2, ACCR+TYPL, ACCW+TYPB, 0, 0, 0, 0}, 
    {"cvtlw", 0xf7, 0, 2, ACCR+TYPL, ACCW+TYPW, 0, 0, 0, 0}, 
    {"ashp", 0xf8, 0, 6, ACCR+TYPB, ACCR+TYPW, ACCA+TYPB, ACCR+TYPB, ACCR+TYPW, ACCA+TYPB}, 
    {"cvtlp", 0xf9, 0, 3, ACCR+TYPL, ACCR+TYPW, ACCA+TYPB, 0, 0, 0}, 
    {"callg", 0xfa, 0, 2, ACCA+TYPB, ACCA+TYPB, 0, 0, 0, 0}, 
    {"calls", 0xfb, 0, 2, ACCR+TYPL, ACCA+TYPB, 0, 0, 0, 0}, 
    {"xfc", 0xfc, 0, 1, ACCI+TYPB, 0, 0, 0, 0, 0}, 
    {"escd", 0xfd, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {"esce", 0xfe, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {"escf", 0xff, 0, 0, 0, 0, 0, 0, 0, 0},
};

public Optab eoptab[] = {
    {"mfvp", 0x31, 0, 2, ACCR+TYPW, ACCW+TYPL, 0, 0, 0, 0},
	/* vcr, vlr, vmrlo, vmrhi, sync, msync */
    {"cvtdh", 0x32, 0, 2, ACCR+TYPD, ACCW+TYPH, 0, 0, 0, 0},
    {"cvtgf", 0x33, 0, 2, ACCR+TYPG, ACCW+TYPF, 0, 0, 0, 0},
    {"vldl", 0x34, FMT1, 3, ACCR+TYPW, ACCA+TYPB, ACCR+TYPL, 0, 0, 0},
    {"vgathl", 0x35, FMT3, 2, ACCR+TYPW, ACCA+TYPB, 0, 0, 0, 0},
    {"vldq", 0x36, FMT1, 3, ACCR+TYPW, ACCA+TYPB, ACCR+TYPL, 0, 0, 0},
    {"vgathq", 0x37, FMT3, 2, ACCR+TYPW, ACCA+TYPB, 0, 0, 0, 0},
    {"dummy", 0x38, 0},
    {"dummy", 0x39, 0},
    {"dummy", 0x3a, 0},
    {"dummy", 0x3b, 0},
    {"dummy", 0x3c, 0},
    {"dummy", 0x3d, 0},
    {"dummy", 0x3e, 0},
    {"dummy", 0x3f, 0},
    {"addg2", 0x40, 0, 2, ACCR+TYPG, ACCM+TYPG, 0, 0, 0, 0},
    {"addg3", 0x41, 0, 3, ACCR+TYPG, ACCR+TYPG, ACCW+TYPG, 0, 0, 0},
    {"subg2", 0x42, 0, 2, ACCR+TYPG, ACCM+TYPG, 0, 0, 0, 0},
    {"subg3", 0x43, 0, 3, ACCR+TYPG, ACCR+TYPG, ACCW+TYPG, 0, 0, 0},
    {"mulg2", 0x44, 0, 2, ACCR+TYPG, ACCM+TYPG, 0, 0, 0, 0},
    {"mulg3", 0x45, 0, 3, ACCR+TYPG, ACCR+TYPG, ACCW+TYPG, 0, 0, 0},
    {"divg2", 0x46, 0, 2, ACCR+TYPG, ACCM+TYPG, 0, 0, 0, 0},
    {"divg3", 0x47, 0, 3, ACCR+TYPG, ACCR+TYPG, ACCW+TYPG, 0, 0, 0},
    {"cvtgb", 0x48, 0, 2, ACCR+TYPG, ACCW+TYPB, 0, 0, 0, 0},
    {"cvtgw", 0x49, 0, 2, ACCR+TYPG, ACCW+TYPW, 0, 0, 0, 0},
    {"cvtgl", 0x4a, 0, 2, ACCR+TYPG, ACCW+TYPL, 0, 0, 0, 0},
    {"cvtrgl", 0x4b, 0, 2, ACCR+TYPG, ACCW+TYPL, 0, 0, 0, 0},
    {"cvtbg", 0x4c, 0, 2, ACCR+TYPB, ACCW+TYPG, 0, 0, 0, 0},
    {"cvtwg", 0x4d, 0, 2, ACCR+TYPW, ACCW+TYPG, 0, 0, 0, 0},
    {"cvtlg", 0x4e, 0, 2, ACCR+TYPL, ACCW+TYPG, 0, 0, 0, 0},
    {"acbg", 0x4f, 0, 4, ACCR+TYPG, ACCR+TYPG, ACCM+TYPG, ACCB+TYPW, 0, 0},
    {"movg", 0x50, 0, 2, ACCR+TYPG, ACCW+TYPG, 0, 0, 0, 0},
    {"cmpg", 0x51, 0, 2, ACCR+TYPG, ACCR+TYPG, 0, 0, 0, 0},
    {"mnegg", 0x52, 0, 2, ACCR+TYPG, ACCW+TYPG, 0, 0, 0, 0},
    {"tstg", 0x53, 0, 1, ACCR+TYPG, 0, 0, 0, 0, 0},
    {"emodg", 0x54, 0, 5, ACCR+TYPG, ACCR+TYPW, ACCR+TYPG, ACCW+TYPL, ACCW+TYPG, 0},
    {"polyg", 0x55, 0, 3, ACCR+TYPG, ACCR+TYPW, ACCA+TYPB, 0, 0, 0},
    {"cvtgh", 0x56, 0, 2, ACCR+TYPG, ACCW+TYPH, 0, 0, 0, 0},
    {"dummy", 0x57, 0},
    {"dummy", 0x58, 0},
    {"dummy", 0x59, 0},
    {"dummy", 0x5a, 0},
    {"dummy", 0x5b, 0},
    {"dummy", 0x5c, 0},
    {"dummy", 0x5d, 0},
    {"dummy", 0x5e, 0},
    {"dummy", 0x5f, 0},
    {"addh2", 0x60, 0, 2, ACCR+TYPH, ACCM+TYPH, 0, 0, 0, 0},
    {"addh3", 0x61, 0, 3, ACCR+TYPH, ACCR+TYPH, ACCW+TYPH, 0, 0, 0},
    {"subh2", 0x62, 0, 2, ACCR+TYPH, ACCM+TYPH, 0, 0, 0, 0},
    {"subh3", 0x63, 0, 3, ACCR+TYPH, ACCR+TYPH, ACCW+TYPH, 0, 0, 0},
    {"mulh2", 0x64, 0, 2, ACCR+TYPH, ACCM+TYPH, 0, 0, 0, 0},
    {"mulh3", 0x65, 0, 3, ACCR+TYPH, ACCR+TYPH, ACCW+TYPH, 0, 0, 0},
    {"divh2", 0x66, 0, 2, ACCR+TYPH, ACCM+TYPH, 0, 0, 0, 0},
    {"divh3", 0x67, 0, 3, ACCR+TYPH, ACCR+TYPH, ACCR+TYPH, 0, 0, 0},
    {"cvthb", 0x68, 0, 2, ACCR+TYPH, ACCW+TYPB, 0, 0, 0, 0},
    {"cvthw", 0x69, 0, 2, ACCR+TYPH, ACCW+TYPW, 0, 0, 0, 0},
    {"cvthl", 0x6a, 0, 2, ACCR+TYPH, ACCW+TYPL, 0, 0, 0, 0},
    {"cvtrhl", 0x6b, 0, 2, ACCR+TYPH, ACCW+TYPL, 0, 0, 0, 0},
    {"cvtbh", 0x6c, 0, 2, ACCR+TYPB, ACCW+TYPH, 0, 0, 0, 0},
    {"cvtwh", 0x6d, 0, 2, ACCR+TYPW, ACCW+TYPH, 0, 0, 0, 0},
    {"cvtlh", 0x6e, 0, 2, ACCR+TYPL, ACCW+TYPH, 0, 0, 0, 0},
    {"acbh", 0x6f, 0, 4, ACCR+TYPH, ACCR+TYPH, ACCM+TYPH, ACCB+TYPW, 0, 0},
    {"movh", 0x70, 0, 2, ACCR+TYPH, ACCW+TYPH, 0, 0, 0, 0},
    {"cmph", 0x71, 0, 2, ACCR+TYPH, ACCR+TYPH, 0, 0, 0, 0},
    {"mnegh", 0x72, 0, 2, ACCR+TYPH, ACCW+TYPH, 0, 0, 0, 0},
    {"tsth", 0x73, 0, 1, ACCR+TYPH, 0, 0, 0, 0, 0},
    {"emodh", 0x74, 0, 5, ACCR+TYPH, ACCR+TYPW, ACCR+TYPH, ACCW+TYPL, ACCW+TYPH, 0},
    {"polyh", 0x75, 0, 3, ACCR+TYPH, ACCR+TYPW, ACCA+TYPB, 0, 0, 0},
    {"cvthg", 0x76, 0, 2, ACCR+TYPH, ACCW+TYPG, 0, 0, 0, 0},
    {"dummy", 0x77, 0},
    {"dummy", 0x78, 0},
    {"dummy", 0x79, 0},
    {"dummy", 0x7a, 0},
    {"dummy", 0x7b, 0},
    {"clrh", 0x7c, 0, 1, ACCW+TYPH, 0, 0, 0, 0, 0},
    {"movo", 0x7d, 0, 2,  ACCR+TYPO, ACCW+TYPO, 0, 0, 0, 0},
    {"movah", 0x7e, 0, 2, ACCA+TYPH, ACCW+TYPL, 0, 0, 0, 0},
    {"pushah", 0x7f, 0, 1, ACCA+TYPH, 0, 0, 0, 0, 0},
    {"vvaddl", 0x80, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsaddl", 0x81, FMT10, 2, ACCR+TYPW, ACCR+TYPL, 0, 0, 0, 0},
    {"vvaddg", 0x82, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsaddg", 0x83, FMT10, 2, ACCR+TYPW, ACCR+TYPG, 0, 0, 0, 0},
    {"vvaddf", 0x84, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsaddf", 0x85, FMT10, 2, ACCR+TYPW, ACCR+TYPF, 0, 0, 0, 0},
    {"vvaddd", 0x86, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsaddd", 0x87, FMT10, 2, ACCR+TYPW, ACCR+TYPD, 0, 0, 0, 0},
    {"vvsubl", 0x88, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vssubl", 0x89, FMT10, 2, ACCR+TYPW, ACCR+TYPL, 0, 0, 0, 0},
    {"vvsubg", 0x8a, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vssubg", 0x8b, FMT10, 2, ACCR+TYPW, ACCR+TYPG, 0, 0, 0, 0},
    {"vvsubf", 0x8c, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vssubf", 0x8d, FMT10, 2, ACCR+TYPW, ACCR+TYPF, 0, 0, 0, 0},
    {"vvsubd", 0x8e, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vssubd", 0x8f, FMT10, 2, ACCR+TYPW, ACCR+TYPD, 0, 0, 0, 0},
    {"dummy", 0x90, 0},
    {"dummy", 0x91, 0},
    {"dummy", 0x92, 0},
    {"dummy", 0x93, 0},
    {"dummy", 0x94, 0},
    {"dummy", 0x95, 0},
    {"dummy", 0x96, 0},
    {"dummy", 0x97, 0},
    {"cvtfh", 0x98, 0, 2, ACCR+TYPF, ACCW+TYPH, 0, 0, 0, 0},
    {"cvtfg", 0x99, 0, 2, ACCR+TYPF, ACCW+TYPG, 0, 0, 0, 0},
    {"dummy", 0x9a, 0},
    {"dummy", 0x9b, 0},
    {"vstl", 0x9c, FMT2, 3, ACCR+TYPW, ACCA+TYPB, ACCR+TYPL, 0, 0, 0},
    {"vscatl", 0x9d, FMT4, 2, ACCR+TYPW, ACCA+TYPB, 0, 0, 0, 0},
    {"vstq", 0x9e, FMT2, 3, ACCR+TYPW, ACCA+TYPB, ACCR+TYPL, 0, 0, 0},
    {"vscatq", 0x9f, FMT4, 2, ACCR+TYPW, ACCA+TYPB, 0, 0, 0, 0},
    {"vvmull", 0xa0, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsmull", 0xa1, FMT10, 2, ACCR+TYPW, ACCR+TYPL, 0, 0, 0, 0},
    {"vvmulg", 0xa2, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsmulg", 0xa3, FMT10, 2, ACCR+TYPW, ACCR+TYPG, 0, 0, 0, 0},
    {"vvmulf", 0xa4, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsmulf", 0xa5, FMT10, 2, ACCR+TYPW, ACCR+TYPF, 0, 0, 0, 0},
    {"vvmuld", 0xa6, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsmuld", 0xa7, FMT10, 2, ACCR+TYPW, ACCR+TYPD, 0, 0, 0, 0},
    {"vsync", 0xa8, 0, 1, ACCR+TYPW},
    {"mtvp", 0xa9, 0, 2, ACCR+TYPW, ACCW+TYPL, 0, 0, 0, 0},
	/* vcr, vlr, vmrlo, vmrhi */
    {"vvdivg", 0xaa, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsdivg", 0xab, FMT10, 2, ACCR+TYPW, ACCR+TYPG, 0, 0, 0, 0},
    {"vvdivf", 0xac, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsdivf", 0xad, FMT10, 2, ACCR+TYPW, ACCR+TYPF, 0, 0, 0, 0},
    {"vvdivd", 0xae, FMT5, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsdivd", 0xaf, FMT10, 2, ACCR+TYPW, ACCR+TYPD, 0, 0, 0, 0},
    {"dummy", 0xb0, 0},
    {"dummy", 0xb1, 0},
    {"dummy", 0xb2, 0},
    {"dummy", 0xb3, 0},
    {"dummy", 0xb4, 0},
    {"dummy", 0xb5, 0},
    {"dummy", 0xb6, 0},
    {"dummy", 0xb7, 0},
    {"dummy", 0xb8, 0},
    {"dummy", 0xb9, 0},
    {"dummy", 0xba, 0},
    {"dummy", 0xbb, 0},
    {"dummy", 0xbc, 0},
    {"dummy", 0xbd, 0},
    {"dummy", 0xbe, 0},
    {"dummy", 0xbf, 0},
    {"vvcmpl", 0xc0, FMT7, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
	/* gtr, eql, lss, leq, neq, geq */
    {"vscmpl", 0xc1, FMT12, 2, ACCR+TYPW, ACCR+TYPL, 0, 0, 0, 0},
	/* gtr, eql, lss, leq, neq, geq */
    {"vvcmpg", 0xc2, FMT7, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
	/* gtr, eql, lss, leq, neq, geq */
    {"vscmpg", 0xc3, FMT12, 2, ACCR+TYPW, ACCR+TYPG, 0, 0, 0, 0},
	/* gtr, eql, lss, leq, neq, geq */
    {"vvcmpf", 0xc4, FMT7, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
	/* gtr, eql, lss, leq, neq, geq */
    {"vscmpf", 0xc5, FMT12, 2, ACCR+TYPW, ACCR+TYPF, 0, 0, 0, 0},
	/* gtr, eql, lss, leq, neq, geq */
    {"vvcmpd", 0xc6, FMT7, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
	/* gtr, eql, lss, leq, neq, geq */
    {"vscmpd", 0xc7, FMT12, 2, ACCR+TYPW, ACCR+TYPD, 0, 0, 0, 0},
	/* gtr, eql, lss, leq, neq, geq */
    {"vvbisl", 0xc8, FMT6, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsbisl", 0xc9, FMT11, 2, ACCR+TYPW, ACCR+TYPL, 0, 0, 0, 0},
    {"illegal vector opcode", 0xca, 0},
    {"illegal vector opcode", 0xcb, 0},
    {"vvbicl", 0xcc, FMT6, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsbicl", 0xcd, FMT11, 2, ACCR+TYPW, ACCR+TYPL, 0, 0, 0, 0},
    {"illegal vector opcode", 0xce, 0},
    {"illegal vector opcode", 0xcf, 0},
    {"dummy", 0xd0, 0},
    {"dummy", 0xd1, 0},
    {"dummy", 0xd2, 0},
    {"dummy", 0xd3, 0},
    {"dummy", 0xd4, 0},
    {"dummy", 0xd5, 0},
    {"dummy", 0xd6, 0},
    {"dummy", 0xd7, 0},
    {"dummy", 0xd8, 0},
    {"dummy", 0xd9, 0},
    {"dummy", 0xda, 0},
    {"dummy", 0xdb, 0},
    {"dummy", 0xdc, 0},
    {"dummy", 0xdd, 0},
    {"dummy", 0xde, 0},
    {"dummy", 0xdf, 0},
    {"vvsrll", 0xe0, FMT6, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vssrll", 0xe1, FMT11, 2, ACCR+TYPW, ACCR+TYPL, 0, 0, 0, 0},
    {"illegal vector opcode", 0xe2, 0},
    {"illegal vector opcode", 0xe3, 0},
    {"vvslll", 0xe4, FMT6, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsslll", 0xe5, FMT11, 2, ACCR+TYPW, ACCR+TYPL, 0, 0, 0, 0},
    {"illegal vector opcode", 0xe6, 0},
    {"illegal vector opcode", 0xe7, 0},
    {"vvxorl", 0xe8, FMT6, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsxorl", 0xe9, FMT11, 2, ACCR+TYPW, ACCR+TYPL, 0, 0, 0, 0},
    {"illegal vector opcode", 0xea, 0},
    {"illegal vector opcode", 0xeb, 0},
    {"vvcvt", 0xec, FMT8, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
	/* fd, fg, gf, df, lf, ld, lg, fl, dl, gl, rfl, rdl, rgl */
    {"iota", 0xed, FMT14, 2, ACCR+TYPW, ACCR+TYPL, 0, 0, 0, 0},
    {"vvmerge", 0xee, FMT9, 1, ACCR+TYPW, 0, 0, 0, 0, 0},
    {"vsmerge", 0xef, FMT13, 2, ACCR+TYPW, ACCR+TYPQ, 0, 0, 0, 0},
	/* f, d, g */
    {"dummy", 0xf0, 0},
    {"dummy", 0xf1, 0},
    {"dummy", 0xf2, 0},
    {"dummy", 0xf3, 0},
    {"dummy", 0xf4, 0},
    {"dummy", 0xf5, 0},
    {"cvthf", 0xf6, 0, 2, ACCR+TYPH, ACCW+TYPF, 0, 0, 0, 0},
    {"cvthd", 0xf7, 0, 2, ACCR+TYPH, ACCW+TYPD, 0, 0, 0, 0},
};
/*
 * Register names.
 */

public String regname[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10","r11","ap", "fp", "sp", "pc"
};

/*
 * Floating point immediate operands.
 */

public String fltimm[] = {
    "0.5", "0.5625", "0.625", "0.6875", "0.75", "0.8125", "0.875", "0.9375",
    "1.0", "1.125", "1.25", "1.375", "1.5", "1.625", "1.75", "1.875",
    "2.0", "2.25", "2.5", "2.75", "3.0", "3.25", "3.5", "3.75",
    "4.0", "4.5", "5.0", "5.5", "6.0", "6.5", "7.0", "7.5",
    "8.0", "9.0", "10.0", "11.0", "12.0", "13.0", "14.0", "15.0",
    "16.0", "18.0", "20.0", "22.0", "24.0", "26.0", "28.0", "30.0",
    "32.0", "36.0", "40.0", "44.0", "48.0", "52.0", "56.0", "60.0",
    "64.0", "72.0", "80.0", "88.0", "96.0", "104.0", "112.0", "120.0"
};

/*
 * Vector instruction compare codes.
 */
public String vcmpname[8][8] = {
     "vvgtrl", "vveqll", "vvlssl", "", "vvleql", "vvneql", "vvgeql", "",
     "vsgtrl", "vseqll", "vslssl", "", "vsleql", "vsneql", "vsgeql", "",
     "vvgtrg", "vveqlg", "vvlssg", "", "vvleqg", "vvneqg", "vvgeqg", "",
     "vsgtrg", "vseqlg", "vslssg", "", "vsleqg", "vsneqg", "vsgeqg", "",
     "vvgtrf", "vveqlf", "vvlssf", "", "vvleqf", "vvneqf", "vvgeqf", "",
     "vsgtrf", "vseqlf", "vslssf", "", "vsleqf", "vsneqf", "vsgeqf", "",
     "vsgtrd", "vseqld", "vslssd", "", "vsleqd", "vsneqd", "vsgeqd", "",
     "vvgtrd", "vveqld", "vvlssd", "", "vvleqd", "vvneqd", "vvgeqd", "",
};
/*
 * Vector instruction convert codes.
 */
public String vcvtname[16] = {
    "", "vvcvtlf", "vvcvtld", "vvcvtlg", 
    "vvcvtfl", "vvcvtrfl", "vvcvtfd", "vvcvtfg", 
    "vvcvtdl", "vvcvtdf", "vvcvtrdl", "", 
    "vvcvtgl", "vvcvtgf", "", "vvcvtrgl"
};
/*
 * Vector instrustion register codes.
 */
public String mtvpname[] = {
   "mtvcr", "mtvlr", "mtvmrlo", "mtvmrhi"
};
public String mfvpname[] = {
   "mfvcr", "mfvlr", "mfvmrlo", "mfvmrhi", "sync", "msync"
};
public String vsyncname[] = {
   "vsync"
};

public Ctrl_word vinst_fmt[] = {
/*             vc    vb    va  x  modifiers */
/* dummy*/  { 0},
/* FMT1 */  {vreg,    0,    0, 0, moe | mtf | mi},
/* FMT2 */  {vreg,    0,    0, 0, moe | mtf},
/* FMT3 */  {vreg, vreg,    0, 0, moe | mtf | mi},
/* FMT4 */  {vreg, vreg,    0, 0, moe | mtf},
/* FMT5 */  {vreg, vreg, vreg, 0, moe | mtf | exc},
/* FMT6 */  {vreg, vreg, vreg, 0, moe | mtf},
/* FMT7 */  {cmp,  vreg, vreg, 0, moe | mtf},
/* FMT8 */  {vreg, vreg,  cvt, 0, moe | mtf | exc},
/* FMT9 */  {vreg, vreg, vreg, 0, mtf},
/* FMT10 */ {vreg, vreg,    0, 0, moe | mtf | exc},
/* FMT11 */ {vreg, vreg,    0, 0, moe | mtf},
/* FMT12 */ {cmp,  vreg,    0, 0, moe | mtf},
/* FMT13 */ {vreg, vreg,    0, 0, mtf},
/* FMT14 */ {vreg,    0,    0, 0, mtf},
};
