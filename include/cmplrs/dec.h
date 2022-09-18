/* --------------------------------------------------------- */
/* | Copyright (c) 1988, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: dec.h,v 2010.4.1.5 89/11/29 22:38:54 bettina Exp $ */

/*
 *----------------------------------------------------------------------------
 *	dec.h has all of the common definitions used by the fixed/float
 * deciaml runtimes.  All of the routines use these definitions and macros
 * in all cases.  Thus, the underlying representation should be relatively
 * painless to change.  The code assumes an array of characters that is
 * aliased with the access macros.  Float decimal routines pass float
 * decimal numbers as these strings.
 *	The current underlying representation is that used by the 68881
 * floating point processor.  A float decimal number consists of 96
 * bits of information, laid out in the following manner:
 *
 *  Bits     Definition of the field
 * _______   ___________________________________________________________
 *   95    - Sign of the mantissa.
 *   94    - Sign of the exponent.
 * 93 - 92 - 2 bits, used for +/-infinity or NaN only
 * 91 - 88 - Digit #1 of the exponent (most significant digit)
 * 87 - 84 - Digit #2 of the exponent
 * 83 - 80 - Digit #3 of the exponent (least significant digit)
 * 79 - 68 - Unused - set to zeroes
 * 67 - 64 - Digit #1 of the mantissa (most significant digit)
 *           The implicit decimal point is always to the right of this digit.
 * 63 - 60 - Digit #2 of the mantissa
 * 59 - 0  - Remaining 15 digits of the mantissa (4 bits for each)
 *----------------------------------------------------------------------------
 */


#include <ansi_compat.h>
#define FIX_PLUS	0x0C
#define FIX_MINUS	0x0D
#define MF_OVER_SIGN	0x40

typedef unsigned long	ULONG;		/* same as UWORD, only 4 bytes */
typedef unsigned int	UWORD;
typedef unsigned short	UHALF;
typedef unsigned char	UCHAR;

typedef short		Bool;

#define TRUE		-1
#define FALSE		 0
#define PASSED		-1
#define FAILED		 0

#ifndef MAX
#define MAX(A,B)        ((A) > (B) ? (A) : (B))
#define MIN(A,B)        ((A) > (B) ? (B) : (A))
#endif
#ifndef	ABS
#define ABS(A)          ((A) >= 0  ? (A) : -(A))
#endif

#define FD_OVERFLOW     0
#define FD_UNDERFLOW    1
#define FD_ZERODIVIDE   2
#define FD_INVALID      3
#define FD_CONVERSION   4

/*
 * Values returned by the V$FDTY routine:
 */
#define FD_NUMBER       0
#define FD_INFINITY     1
#define FD_NAN          2
#define UNKNOWN_FD_TYPE -137

/*
 *----------------------------------------------------------------------------
 * The structure defined here is the 68881 FPU representation of
 * float decimal numbers.
 *----------------------------------------------------------------------------
 */
#define MINFD_DIGITS    1

#if LPI
#define MAXFD_DIGITS    17      /* Maximum precision of mantissa */
#else
#define MAXFD_DIGITS    14      /* Maximum precision of mantissa - one less
				   than for fixed decimal so as to allow
				   for carry when adding */
#endif /* LPI */

#define MAXFD_XDIGITS   3       /* Maximum precision of exponent */
#define FD_BYTES        12
#define FD_WORDS        3
#define FD_HALFS        6

typedef union FloatDec
{
    ULONG words[FD_WORDS];
    UHALF halfs[FD_HALFS];
    UCHAR bytes[FD_BYTES];
} FLOAT_DEC, *FLT_DEC;

typedef unsigned long	Sign;

#define PLUS_SIGN       0x00
#define MINUS_SIGN      0x80
#define MASK_MANTISSA   0x7F
#define MASK_EXPONENT   0xBF

#define MASK_INFINITY	0x7FFF0000

#define CLEAR_FD(ME) \
	((ME)->words[0] = (ME)->words[1] = (ME)->words[2] = 0)

#define SET_SPECIAL_BITS(ME) \
	(ME)->bytes[0] = ((ME)->bytes[0] | 0x30)	/* NaN, infinities */

#define SET_MANTISSA_SIGN(ME, X) \
	(ME)->bytes[0] = (((ME)->bytes[0] & 0x7F) | (X))

#define GET_MANTISSA_SIGN(ME) \
	((ME)->bytes[0] & 0x80)

#define SET_EXPONENT_SIGN(ME, X) \
	(ME)->bytes[0] = (((ME)->bytes[0] & 0xBF) | (((X) >> 1) & 0x40))

#define GET_EXPONENT_SIGN(ME) \
	(((ME)->bytes[0] & 0x40) << 1)

#define GET_EXPONENT(ME) \
	((ME)->halfs[0])

/*
 * The digits are defined with the most significant being numbered 1.
 * The least significant is numbered MAXFD_XDIGITSth.
 */

#define SET_EDIGIT(ME, DIG, X) \
	(ME)->bytes[((DIG) / 2)] = \
	((DIG) & 0x01 \
	? (((ME)->bytes[((DIG) / 2)] & 0xF0) | (X)) \
	: (((ME)->bytes[((DIG) / 2)] & 0x0F) | ((X) << 4)))

#define GET_EDIGIT(ME, DIG) \
	(((ME)->bytes[(DIG) / 2] >> ((((DIG) - 1) & 0x01) * 4)) & 0x0F)

#define SET_MDIGIT(ME, DIG, X) \
	(ME)->bytes[((DIG) / 2) + 3] = \
	((DIG) & 0x01 \
	? (((ME)->bytes[((DIG) / 2) + 3] & 0xF0) | (X)) \
	: (((ME)->bytes[((DIG) / 2) + 3] & 0x0F) | ((X) << 4)))

#define GET_MDIGIT(ME, DIG) \
	(((ME)->bytes[((DIG) / 2) + 3] >> ((((DIG) - 1) & 0x01) * 4)) & 0x0F)

/*
 *----------------------------------------------------------------------------
 *	The structure defined here is the MIPS representation of 39-digit
 * fixed decimal numbers. 
 *
 *	7-digit numbers occupy one word, 15-digit numbers two words, and
 * 23-digit numbers three word, and so on.  In any case, the digits occupy
 * nibbles from high-order to low-order portions of memory, with the 
 * lowest-order nibble containing the sign.  That is, it's in a big-endian
 * format.
 *
 *	When dealing with fixed decimals having other precisions, note that
 * those in the range 16..23 are represented as 23 digit numbers with the
 * appropriate number of high-order digits being zero. Those in the range
 * 8..15 are represented as 15-digit numbers with the appropriate number
 * of high-order digits being zero. Those in the range 1..7 are represented
 * as 7-digit numbers with the appropriate number of high-order digits
 * being zero.
 *
 *	The GET_FDIGIT and SET_FDIGIT macros will work for varying precisions
 * provided you number the digits as if the precision were 7, 15, 23, 31, or
 * 39.  Thus, a 5-digit number occupies digits 3..7, and an 11-digit number
 * occupies digits 5..15, and a 18-digit number occupies 6..23.
 *
    5-DIGIT NUMBER ======================================================

	      +-----+-----+-----+-----+
  Word 1      | 0 0 | d d | d d | d s |
	      +-----+-----+-----+-----+
	        1 2   3 4   5 6   7 SGN


   11-DIGIT NUMBER ======================================================

	      +-----+-----+-----+-----+
  Word 1      | 0 0 | 0 0 | d d | d d |
	      +-----+-----+-----+-----+
	        1 2   3 4   5 6   7 8 

	      +-----+-----+-----+-----+
  Word 2      | d d | d d | d d | d s |
	      +-----+-----+-----+-----+
	        9 10 11 12 13 14 15 SGN

   18-DIGIT NUMBER ======================================================

	      +-----+-----+-----+-----+
  Word 1      | 0 0 | 0 0 | 0 d | d d |
	      +-----+-----+-----+-----+
	        1 2   3 4   5 6   7 8 

	      +-----+-----+-----+-----+
  Word 2      | d d | d d | d d | d d |
	      +-----+-----+-----+-----+
	        9 10 11 12 13 14 15 16

	      +-----+-----+-----+-----+
  Word 3      | d d | d d | d d | d s |
	      +-----+-----+-----+-----+
	       17 18 19 20 21 22 23 SGN

 *
 *----------------------------------------------------------------------------
 */

#define DE_ZERO		0x0C
#define DE_ONE		0x1C
#define DE_FIVE		0x50
#define POS_DE_FIVE     0x5C

#define BITS_PER_DIGIT	 4
#define DIGITS_PER_WORD	 8
#define BYTES_PER_WORD	 4
#define BITS_PER_WORD	32

#define DE07_PREC	 7	/* max precision for 1 word fixed decimal */
#define DE15_PREC	15	/* max precision for 2 word fixed decimal */
#define DE23_PREC	23	/* max precision for 3 word fixed decimal */
#define DE31_PREC	31	/* max precision for 4 word fixed decimal */
#define DE39_PREC	39	/* max precision for 5 word fixed decimal */

#define DE07_WORD	 1	/* words needed for precision  7 */
#define DE15_WORD	 2	/* words needed for precision 15 */
#define DE23_WORD	 3	/* words needed for precision 23 */
#define DE31_WORD	 4	/* words needed for precision 31 */
#define DE39_WORD	 5	/* words needed for precision 39 */

#define FIXED_DECIMAL_MAX_P	39
#define FIXED_DECIMAL39_MAX	39

#define MAKEPQ(p, q)	((q) >= 0 ? ((q) * 256 + (p)) : ((q) * 256 - (p)))
#define PRECISION(pq)	((pq) >= 0 ? ((pq) % 256) : (-(pq) % 256))
#define SCALE(pq)	((pq) / 256)
#define NORMP(p,q,r,s) \
	MIN(FIXED_DECIMAL_MAX_P, MAX((p) - (q), (r) - (s)) + MAX((q), (s)) + 1)

#define SIZE_IN_WORDS(p) (((p) / DIGITS_PER_WORD) + 1)
#define SIZE_IN_BYTES(p) (((p) + 1) / 2)

#define MAXI8_DIGITS	18		/* 64-bit integer can only hlod 18 */
#define MAX_FIXDIGITS   15
#define MAXDECQ         127		/* largest allowed */


#if LPI
/*
 * LPI's fixed decimal format.
 */
#define MAX_FIXDIGITS   (((MAXFD_DIGITS) * 2) + 1)        /* MUST BE ODD! */
#define FIX_BYTES       (((MAX_FIXDIGITS) + 1) / 2)
typedef struct FixedDec
{
    unsigned char  Fbytes[(FIX_BYTES)];
} FIX_DEC, *X_DEC;
#endif /* LPI */


/*
 * MIPS implementation of the fixed decimal format.
 */

typedef union FixedDec
{
    ULONG fdwords[5];
    UCHAR fdbytes[20];
} FIX_DEC, *X_DEC;


/*
 * At MIPS we don't understand the need for this, but probably it will
 * get us in the end
 */

#if LPI
extern  char    _P_SGNMAP[];		/* Sign map array [0-F] */
#define SGNMAP(x) (_P_SGNMAP[x])
#else
#define SGNMAP(x) (x)
#endif /* LPI */


#ifdef __MIPSEL

/* !! Inspect and change all the macros in this file for little-endian!! */
/* Since we don't do COBOL anyway, don't sweat if these aren't right */

#define XHIGH(X)  ((*X).fdwords[4])
#define XMID(X)   ((*X).fdwords[3])
#define XLOW(X)   ((*X).fdwords[2])
#define XLOW31(X) ((*X).fdwords[1])
#define XLOW39(X) ((*X).fdwords[0])

#define XWORD0(X) ((*X).fdwords[0])
#define XWORD1(X) ((*X).fdwords[1])
#define XWORD2(X) ((*X).fdwords[2])
#define XWORD3(X) ((*X).fdwords[3])
#define XWORD4(X) ((*X).fdwords[4])

#define HIGH(X)	  ((X).fdwords[4])
#define MID(X)    ((X).fdwords[3])
#define LOW(X)    ((X).fdwords[2])
#define LOW31(X)  ((X).fdwords[1])
#define LOW39(X)  ((X).fdwords[0])

#define WORD0(X)  ((X).fdwords[0])
#define WORD1(X)  ((X).fdwords[1])
#define WORD2(X)  ((X).fdwords[2])
#define WORD3(X)  ((X).fdwords[3])
#define WORD4(X)  ((X).fdwords[4])

#endif /* __MIPSEL */


#ifdef __MIPSEB

#define XHIGH(X)  ((*X).fdwords[0])
#define XMID(X)   ((*X).fdwords[1])
#define XLOW(X)   ((*X).fdwords[2])
#define XLOW31(X) ((*X).fdwords[3])
#define XLOW39(X) ((*X).fdwords[4])

#define XWORD0(X) ((*X).fdwords[0])
#define XWORD1(X) ((*X).fdwords[1])
#define XWORD2(X) ((*X).fdwords[2])
#define XWORD3(X) ((*X).fdwords[3])
#define XWORD4(X) ((*X).fdwords[4])

#define HIGH(X)	  ((X).fdwords[0])
#define MID(X)    ((X).fdwords[1])
#define LOW(X)    ((X).fdwords[2])
#define LOW31(X)  ((X).fdwords[3])
#define LOW39(X)  ((X).fdwords[4])

#define WORD0(X)  ((X).fdwords[0])
#define WORD1(X)  ((X).fdwords[1])
#define WORD2(X)  ((X).fdwords[2])
#define WORD3(X)  ((X).fdwords[3])
#define WORD4(X)  ((X).fdwords[4])

#endif /* __MIPSEB */


#define CLEAR_DE07(X) ((X).fdwords[0] = 0)
#define CLEAR_DE15(X) ((X).fdwords[0] = (X).fdwords[1] = 0)
#define CLEAR_DE23(X) ((X).fdwords[0] = (X).fdwords[1] = (X).fdwords[2] = 0)
#define CLEAR_DE31(X) ((X).fdwords[0] = (X).fdwords[1] = (X).fdwords[2] = (X).fdwords[3] = 0)
#define CLEAR_DE39(X) ((X).fdwords[0] = (X).fdwords[1] = (X).fdwords[2] = (X).fdwords[3] = (X).fdwords[4] = 0)


/*
 * Sign operations for fixed decimal.
 */

#define GET_DE_SIGN(A)		((A) & 0x0F)
#define CLEAR_DE_SIGN(A)	((A) & ~0x0F)
#define SET_DE_SIGN(A, X)	(A = ((A) & ~0x0F) | ((X) & 0x0F))
#define CHANGE_TO_PLUS(A)	((A) & ~0x01)
#define CHANGE_TO_MINUS(A)	((A) | 0x01)
#define FLIP_DE_SIGN(A)		((A) ^ 0x01)

#define GET_DE07_SIGN(ME) (WORD0(ME) & 0x0F)
#define GET_DE15_SIGN(ME) (WORD1(ME) & 0x0F)
#define GET_DE23_SIGN(ME) (WORD2(ME) & 0x0F)
#define GET_DE31_SIGN(ME) (WORD3(ME) & 0x0F)
#define GET_DE39_SIGN(ME) (WORD4(ME) & 0x0F)

#define CLEAR_DE07_SIGN(ME)  WORD0(ME) = ((WORD0(ME) & ~0x0F))
#define CLEAR_DE15_SIGN(ME)  WORD1(ME) = ((WORD1(ME) & ~0x0F))
#define CLEAR_DE23_SIGN(ME)  WORD2(ME) = ((WORD2(ME) & ~0x0F))
#define CLEAR_DE31_SIGN(ME)  WORD3(ME) = ((WORD3(ME) & ~0x0F))
#define CLEAR_DE39_SIGN(ME)  WORD4(ME) = ((WORD4(ME) & ~0x0F))

#define SET_DE07_SIGN(ME, X) WORD0(ME) = ((WORD0(ME) & ~0x0F) | ((X) & 0x0F))
#define SET_DE15_SIGN(ME, X) WORD1(ME) = ((WORD1(ME) & ~0x0F) | ((X) & 0x0F))
#define SET_DE23_SIGN(ME, X) WORD2(ME) = ((WORD2(ME) & ~0x0F) | ((X) & 0x0F))
#define SET_DE31_SIGN(ME, X) WORD3(ME) = ((WORD3(ME) & ~0x0F) | ((X) & 0x0F))
#define SET_DE39_SIGN(ME, X) WORD4(ME) = ((WORD4(ME) & ~0x0F) | ((X) & 0x0F))

#define FD2FIX_SIGN(X)	(((X) == (PLUS_SIGN)) ? (FIX_PLUS) : (FIX_MINUS))
#define FIX2FD_SIGN(X)	((SGNMAP(X)==(FIX_MINUS)) ? (MINUS_SIGN) : (PLUS_SIGN))


/*
 * Digit operations for fixed decimal, digits are numbered 1..39
 */

#if LPI
#define GET_FDIGIT(ME, DIG) \
	((DIG) & 0x01 \
	? ((ME).fdbytes[((DIG) - 1) / 2] >> 4) \
	: ((ME).fdbytes[((DIG) - 1) / 2] & 0x0F))
#else
#define GET_FDIGIT(ME, N) \
	(((ME).fdbytes[((N) - 1) / 2] >> (((N) & 0x01) * 4)) & 0x0F)
#endif /* LPI */

#define SET_FDIGIT(ME, DIG, X) \
	(ME).fdbytes[((DIG) - 1) / 2] = \
	(((DIG) & 0x01) \
	? (((ME).fdbytes[((DIG) - 1) / 2] & 0x0F) | ((X) << 4)) \
	: (((ME).fdbytes[((DIG) - 1) / 2] & 0xF0) | ((X) & 0x0F)))

#if LPI
#define XGET_FDIGIT(ME, DIG) \
	((DIG) & 0x01 \
	? ((*ME).fdbytes[((DIG) - 1) / 2] >> 4) \
	: ((*ME).fdbytes[((DIG) - 1) / 2] & 0x0F))
#else
#define XGET_FDIGIT(ME, N) \
	(((*ME).fdbytes[((N) - 1) / 2] >> (((N) & 0x01) * 4)) & 0x0F)
#endif /* LPI */

#define XSET_FDIGIT(ME, DIG, X) \
	(*ME).fdbytes[((DIG) - 1) / 2] = \
	(((DIG) & 0x01) \
	? (((*ME).fdbytes[((DIG) - 1) / 2] & 0x0F) | ((X) << 4)) \
	: (((*ME).fdbytes[((DIG) - 1) / 2] & 0xF0) | ((X) & 0x0F)))

/*
 * Given operand precisions (p,q) and (r,s), return dope vector for the
 * precision at which the intermediate computation should occur (assumes
 * non-negative scale-factor)
 */

#define ADD_INTERMED(P,Q,R,S) \
	(MIN (MAX_FIXDIGITS, MAX ((P) - (Q), (R) - (S)) + MAX ((Q), (S)) + 1) \
	+ 256 * MAX ((Q),(S)))

#define MUL_INTERMED(P,Q,R,S) \
	(MIN (MAX_FIXDIGITS, (P) + (R) + 1) + 256 * ((Q) + (S)))
  

/*
 *---------------------------------------------------------------------
 * fixed decimal functions exported by libpl1.
 *---------------------------------------------------------------------
 */

ULONG
cell_mul (
	ULONG x,	/* 1st operand */
	ULONG y,	/* 2nd operand */
	ULONG *z,	/* low word of the prod */
	ULONG carry	/* in-coming carry, added to the low word */
	);

void
shift_de_right (
	X_DEC src,
	X_DEC dst,
	ULONG precision,
	ULONG shift_factor
	);

void
shift_de_left (
	X_DEC src,
	X_DEC dst,
	ULONG precision,
	ULONG shift_factor
	);

void
P$MAKE_EQ_P (
	X_DEC target,
	long  t_p,	/* target precision */
	X_DEC src,
	long  s_p	/* src precision */
	);

/*
 *----------------------------------------------------------------------------
 * P$ABS_DE  - Assign absolute value of decimal number.
 * V$ABSDE08 - precision < 8
 * V$ABSDE16 - precision < 16
 * V$ABSDE24 - precision < 24
 * V$ABSDE32 - precision >= 32
 *----------------------------------------------------------------------------
 */

void
P$ABS_DE (
	X_DEC target,
	short *target_p,
	X_DEC source,
	short *source_p
	);

ULONG
V$ABSDE08 (
	long  target_pq,
	ULONG src,
	long  src_pq
	);

void
V$ABSDE16 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_lw,
	long  src_pq
	);

void
V$ABSDE24 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_mw,
	ULONG src_lw,
	long  src_pq
	);

void
V$ABSDE32 (
	X_DEC target,
	long  target_pq,
	X_DEC src,
	long  src_pq
	);

void
P$ADD_DE (
    X_DEC target,
    long  target_pq,
    X_DEC src1,
    long  src1_pq,
    X_DEC src2,
    long  src2_pq,
    long  norm_pq
	);

void
P$SUB_DE (
    X_DEC target,
    long  target_pq,
    X_DEC src1,
    long  src1_pq,
    X_DEC src2,
    long  src2_pq,
    long  norm_pq
	);

V$ADDDE08_EQ (
	long  target_p,
	ULONG op1,
	ULONG op2,
	long  norm_pq
	);

V$SUBDE08_EQ (
	long  target_p,
	ULONG op1,
	ULONG op2,
	long  norm_pq
	);

V$ADDDE16_EQ (
	X_DEC target,
	long  target_p,
	ULONG src1_hw,
	ULONG src1_lw,
	ULONG src2_hw,
	ULONG src2_lw,
	long  norm_pq
	);

V$SUBDE16_EQ (
	X_DEC target,
	long  target_p,
	ULONG src1_hw,
	ULONG src1_lw,
	ULONG src2_hw,
	ULONG src2_lw,
	long  norm_pq
	);

void
V$ADDDE08 (
	X_DEC target,
	long  target_pq,
	ULONG src1,
	long  src1_pq,
	ULONG src2,
	long  src2_pq,
	long  norm_pq
	);

void
V$SUBDE08 (
	X_DEC target,
	long  target_pq,
	ULONG src1,
	long  src1_pq,
	ULONG src2,
	long  src2_pq,
	long  norm_pq
	);

void
V$ADDDE16 (
	X_DEC target,
	long  target_pq,
	ULONG src1_hw,
	ULONG src1_lw,
	long  src1_pq,
	ULONG src2_hw,
	ULONG src2_lw,
	ULONG src2_pq,
	long  norm_pq
	);

void
V$SUBDE16 (
	X_DEC target,
	long  target_pq,
	ULONG src1_hw,
	ULONG src1_lw,
	long  src1_pq,
	ULONG src2_hw,
	ULONG src2_lw,
	ULONG src2_pq,
	long  norm_pq
	);

void
V$ADDDE24 (
	ULONG *target,
	long  target_pq,
	ULONG *src1,
	ULONG *src2,
	long  src1_pq,
	long  src2_pq,
	long  norm_pq
	);

void
V$SUBDE24 (
	ULONG *target,
	long  target_pq,
	ULONG *src1,
	ULONG *src2,
	long  src1_pq,
	long  src2_pq,
	long  norm_pq
	);

void
V$ADDDE32 (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	X_DEC src2,
	long  src1_pq,
	long  src2_pq,
	long  norm_pq
	);

void
V$SUBDE32 (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	X_DEC src2,
	long  src1_pq,
	long  src2_pq,
	long  norm_pq
	);

/*
 *----------------------------------------------------------------------------
 * P$ASS_DE - Assign decimal number.  It is pl1 callable, used by P$ANY2.
 * V$ASSDE08 - precision < 8
 * V$ASSDE16 - precision < 16
 * V$ASSDE24 - precision < 24
 * V$ASSDE32 - precision >= 32
 *----------------------------------------------------------------------------
 */

void
P$ASS_DE (
	X_DEC target,
	short *target_p,
	X_DEC source,
	short *source_p
	);

ULONG
V$ASSDE08 (
	long target_pq,
	ULONG src,
	long src_pq
	);

void
V$ASSDE16 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_lw,
	long  src_pq
	);

void
V$ASSDE24 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_mw,
	ULONG src_lw,
	long  src_pq
	);

void
V$ASSDE32 (
	X_DEC target,
	long  target_pq,
	X_DEC src,
	long  src_pq 
	);

/*
 *----------------------------------------------------------------------------
 *----------------------------------------------------------------------------
 */

void
V$CCHDE (
	char  *src,
	long  src_dopev,
	X_DEC target,
	long  target_pq
	);

void
V$CDECH (
	X_DEC src,
	long  src_pq,
	char  *target,
	long  target_dopev
	);

float
V$CDEF2 (
	X_DEC src,
	long  src_pq
	);

double
V$CDEF4 (
	X_DEC src,
	long  src_pq
	);

long
V$CDEI4 (
	X_DEC src,
	long  src_pq,
	long  target_pq
	);

/*
 *----------------------------------------------------------------------------
 * P$CEI_DE  - decimal ceiling function.  It is a pl1 callable version.
 *		It is provided for P$ANY2's use.
 * V$CEIDE08 - decimal ceiling function, precision < 8
 * V$CEIDE16 - decimal ceiling function, precision < 16
 * V$CEIDE24 - decimal ceiling function, precision < 24
 * V$CEIDE32 - decimal ceiling function, precision < 32
 * V$CEIDE40 - decimal ceiling function, precision >= 32
 *----------------------------------------------------------------------------
 */

void
P$CEI_DE (
	X_DEC target,
	long  target_pq,
	X_DEC source,
	long  source_pq
	);

ULONG
V$CEIDE08 (
	long  target_pq,
	ULONG src,
	long  src_pq
	);

void
V$CEIDE16 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_lw,
	long  src_pq
	);

void
V$CEIDE24 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_mw,
	ULONG src_lw,
	long  src_pq
	);

void
V$CEIDE32 (
	X_DEC target,
	long  target_pq,
	X_DEC src,
	long  src_pq
	);

void
V$CEIDE40 (
	X_DEC target,
	long  target_pq,
	X_DEC src,
	long  src_pq
	);

/*
 *----------------------------------------------------------------------------
 *----------------------------------------------------------------------------
 */

void
V$CF4DE (
	double source,
	X_DEC  target,
	long   target_pq
	);

void
V$CI4DE (
	long  source,
	X_DEC target,
	long  target_pq
	);

/*
 *----------------------------------------------------------------------------
 * P$CMP_DE - Compare decimal number, returns 1, 0, or -1 depends on if
 *		arg1 > arg2, arg1 == arg2, or arg1 < arg2.
 *		It is pl1 callable, used by P$ANY2.
 * V$CMPDE08 - precision < 8
 * V$CMPDE16 - precision < 16
 * V$CMPDE24 - precision < 24
 * V$CMPDE32 - precision >= 32
 *----------------------------------------------------------------------------
 */

long
P$CMP_DE (
	X_DEC src1,
	long  src1_pq,
	X_DEC src2,
	long  src2_pq
	);

long
V$CMPDE08 (
	ULONG arg1,
	long  arg1_pq,
	ULONG arg2,
	long  arg2_pq
	);

long
V$CMPDE16 (
	ULONG arg1_hw,
	ULONG arg1_lw,
	long  arg1_pq,
	ULONG arg2_hw,
	ULONG arg2_lw,
	long  arg2_pq
	);

long
V$CMPDE24 (
	ULONG arg1_hw,
	ULONG arg1_mw,
	ULONG arg1_lw,
	long  arg1_pq,
	ULONG arg2_hw,
	ULONG arg2_mw,
	ULONG arg2_lw,
	long  arg2_pq
	);

long
V$CMPDE32 (
	X_DEC arg1,
	long  arg1_pq,
	X_DEC arg2,
	long  arg2_pq
	);

/*
 *----------------------------------------------------------------------------
 * P$DIV_DE  - Divide decimal numbers.  It is pl1 callable, used by P$ANY2.
 * V$DIVDE08 - precision < 8
 * V$DIVDE16 - precision < 16
 * V$DIVDE24 - precision < 24
 * V$DIVDE32 - precision < 32
 * V$DIVDE40 - precision >= 32
 *----------------------------------------------------------------------------
 */

void
P$DIV_DE (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	long  src1_pq,
	X_DEC src2,
	long  src2_pq,
	long  norm_pq
	);

ULONG
V$DIVDE08 (
	long  target_pq,
	ULONG src1,
	long  src1_pq,
	ULONG src2,
	long  src2_pq,
	long  norm_pq
	);

void
V$DIVDE16 (
	X_DEC target,
	long  target_pq,
	ULONG src1_hw,
	ULONG src1_lw,
	long  src1_pq,
	ULONG src2_hw,
	ULONG src2_lw,
	long  src2_pq,
	long  norm_pq
	);

void
V$DIVDE24 (
	X_DEC target,
	long  target_pq,
	ULONG src1_hw,
	ULONG src1_mw,
	ULONG src1_lw,
	long  src1_pq,
	ULONG src2_hw,
	ULONG src2_mw,
	ULONG src2_lw,
	long  src2_pq,
	long  norm_pq
	);

void
V$DIVDE32 (
	X_DEC target,
	long  target_pq,
	ULONG s1[4],
	long  src1_pq,
	ULONG s2[4],
	long  src2_pq,
	long  norm_pq
	);

void
V$DIVDE40 (
	X_DEC target,
	long  target_pq,
	ULONG s1[5],
	long  src1_pq,
	ULONG s2[5],
	long  src2_pq,
	long  norm_pq
	);

/*
 *----------------------------------------------------------------------------
 * P$FLO_DE  - decimal floor function.  It is a pl1 callable version.
 *		It is provided for P$ANY2's use.
 * V$FLODE08 - decimal floor function, precision < 8
 * V$FLODE16 - decimal floor function, precision < 16
 * V$FLODE24 - decimal floor function, precision < 24
 * V$FLODE32 - decimal floor function, precision < 32
 * V$FLODE40 - decimal floor function, precision >= 32
 *----------------------------------------------------------------------------
 */

void
P$FLO_DE (
	X_DEC target,
	long  target_pq,
	X_DEC source,
	long  source_pq
	);

ULONG
V$FLODE08 (
	long  target_pq,
	ULONG src,
	long  src_pq
	);

void
V$FLODE16 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_lw,
	long  src_pq
	);

void
V$FLODE24 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_mw,
	ULONG src_lw,
	long  src_pq
 	);

void
V$FLODE32 (
	X_DEC target,
	long  target_pq,
	X_DEC src,
	long  src_pq
   	);

void
V$FLODE40 (
	X_DEC target,
	long  target_pq,
	X_DEC src,
	long  src_pq
	);

/*
 *----------------------------------------------------------------------------
 * P$MAX_DE  - decimal max function.  It is a pl1 callable version.
 *		It is provided for P$ANY2's use.
 * V$MAXDE08 - decimal max function, precision < 8
 * V$MAXDE16 - decimal max function, precision < 16
 * V$MAXDE24 - decimal max function, precision < 24
 * V$MAXDE32 - decimal max function, precision >= 32
 *----------------------------------------------------------------------------
 */

void
P$MAX_DE (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	long  src1_pq,
	X_DEC src2,
	long  src2_pq
	);

ULONG
V$MAXDE08 (
	long  target_pq,
	ULONG src1,
	long  src1_pq,
	ULONG src2,
	long  src2_pq
	);

void
V$MAXDE16 (
	X_DEC target,
	long  target_pq,
	ULONG src1_hw,
	ULONG src1_lw,
	long  src1_pq,
	ULONG src2_hw,
	ULONG src2_lw,
	long  src2_pq
	);

void
V$MAXDE24 (
	X_DEC target,
	long  target_pq,
	ULONG src1_hw,
	ULONG src1_mw,
	ULONG src1_lw,
	long  src1_pq,
	ULONG src2_hw,
	ULONG src2_mw,
	ULONG src2_lw,
	long  src2_pq
	);

void
V$MAXDE32 (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	long  src1_pq,
	X_DEC src2,
	long  src2_pq
	);

/*
 *----------------------------------------------------------------------------
 * P$MIN_DE -  decimal MIN function.  It is a pl1 callable version.
 *		It is provided for P$ANY2's use.
 * V$MINDE08 - decimal MIN function, precision < 8
 * V$MINDE16 - decimal MIN function, precision < 16
 * V$MINDE24 - decimal MIN function, precision < 24
 * V$MINDE32 - decimal MIN function, precision >= 32
 *----------------------------------------------------------------------------
 */

void
P$MIN_DE (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	long  src1_pq,
	X_DEC src2,
	long  src2_pq
	);

ULONG
V$MINDE08 (
	long  target_pq,
	ULONG src1,
	long  src1_pq,
	ULONG src2,
	long  src2_pq
	);

void
V$MINDE16 (
	X_DEC target,
	long  target_pq,
	ULONG src1_hw,
	ULONG src1_lw,
	long  src1_pq,
	ULONG src2_hw,
	ULONG src2_lw,
	long  src2_pq
	);

void
V$MINDE24 (
	X_DEC target,
	long  target_pq,
	ULONG src1_hw,
	ULONG src1_mw,
	ULONG src1_lw,
	long  src1_pq,
	ULONG src2_hw,
	ULONG src2_mw,
	ULONG src2_lw,
	long  src2_pq
	);

void
V$MINDE32 (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	long  src1_pq,
	X_DEC src2,
	long  src2_pq
	);

/*
 *----------------------------------------------------------------------------
 * P$MOD_DE  - Return src1 mod src2.  It is pl1 callable, used by P$ANY2.
 * V$MODDE08 - precision < 8
 * V$MODDE16 - precision < 16
 * V$MODDE24 - precision < 24
 * V$MODDE32 - precision < 32
 * V$MODDE40 - precision >= 32
 *----------------------------------------------------------------------------
 */

void
P$MOD_DE (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	long  src1_pq,
	X_DEC src2,
	long  src2_pq
	);

ULONG
V$MODDE08 (
	long  target_pq,
	ULONG src1,
	long  src1_pq,
	ULONG src2,
	long  src2_pq
	);

void
V$MODDE16 (
	X_DEC target,
	long  target_pq,
	ULONG src1_hw,
	ULONG src1_lw,
	long  src1_pq,
	ULONG src2_hw,
	ULONG src2_lw,
	long  src2_pq
	);

void
V$MODDE24 (
	X_DEC target,
	long  target_pq,
	ULONG src1_hw,
	ULONG src1_mw, 
	ULONG src1_lw,
	long  src1_pq,
	ULONG src2_hw,
	ULONG src2_mw,
	ULONG src2_lw,
	long  src2_pq
  	);

void
V$MODDE32 (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	long  src1_pq,
	X_DEC src2,
	long  src2_pq
 	);

void
V$MODDE40 (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	long  src1_pq,
	X_DEC src2,
	long  src2_pq
	);

/*
 *----------------------------------------------------------------------------
 * P$MUL_DE  - Multiply decimal numbers.  It is pl1 callable, used by P$ANY2.
 * V$MULDE08 - Multiply decimal numbers, precision < 8.
 * V$MULDE40 - Multiply normalized decimal numbers, that is, both src1 and
 *	       src2 have the same precision (>= 8).
 *----------------------------------------------------------------------------
 */

void
P$MUL_DE (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	long  src1_pq,
	X_DEC src2,
	long  src2_pq,
	long  norm_pq
	);

ULONG
V$MULDE08 (
	long  target_pq,
	ULONG src1,
	long  src1_pq,
	ULONG src2,
	long  src2_pq,
	long  norm_pq
	);

void
V$MULDE40 (
	X_DEC target,
	long  target_pq,
	X_DEC src1,
	long  src1_pq,
	X_DEC src2,
	long  src2_pq
	);

/*
 *----------------------------------------------------------------------------
 * P$NEG_DE  - Assign negated value of decimal number, flip sign when not zero.
 * V$NEGDE08 - precision < 8
 * V$NEGDE16 - precision < 16
 * V$NEGDE24 - precision < 24
 * V$NEGDE32 - precision >= 32
 *----------------------------------------------------------------------------
 */

void
P$NEG_DE (
	X_DEC target,
	short *target_pq,
	X_DEC source,
	short *source_pq
	);

ULONG
V$NEGDE08 (
	long  target_pq,
	ULONG src,
	long  src_pq
	);

void
V$NEGDE16 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_lw,
	long  src_pq
	);

void
V$NEGDE24 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_mw,
	ULONG src_lw,
	long  src_pq
	);

void
V$NEGDE32 (
	X_DEC target,
	long  target_pq,
	X_DEC src,
	long  src_pq
	);

/*
 *----------------------------------------------------------------------------
 * P$ROU_DE  - decimal rounding function.  It is a pl1 callable version.
 *		It is provided for P$ANY2's use.
 *		Return the value "src" rounded away from 0 to "places"
 * V$ROUDE08 - decimal rounding function, precision < 8
 * V$ROUDE16 - decimal rounding function, precision < 16
 * V$ROUDE24 - decimal rounding function, precision < 24
 * V$ROUDE32 - decimal rounding function, precision < 32
 * V$ROUDE40 - decimal rounding function, precision >= 32
 *----------------------------------------------------------------------------
 */

void
P$ROU_DE (
	X_DEC target,
	long  target_pq,
	X_DEC source,
	long  source_pq,
	long  places
	);

void
P$ROUNDE (
	X_DEC target,
	short *target_pq,
	X_DEC source,
	short *source_pq,
	short *places
	);

ULONG
V$ROUDE08 (
	long  target_pq,
	ULONG src,
	long  src_pq,
	long  places
	);

void
V$ROUDE16 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_lw,
	long  src_pq,
	long  places
	);

void
V$ROUDE24 (
	X_DEC target,
	long  target_pq,
	ULONG src_hw,
	ULONG src_mw,
	ULONG src_lw,
	long  src_pq,
	long  places
	);

void
V$ROUDE32 (
	X_DEC target,
	long  target_pq,
	X_DEC src,
	long  src_pq,
	long  places
	);

void
V$ROUDE40 (
	X_DEC target,
	long  target_pq,
	X_DEC src,
	long  src_pq,
	long  places
	);

/*
 *----------------------------------------------------------------------------
 * P$SIGN_DE  - Sign function for decimal numbers, returns 1, 0, -1.
 * V$SIGNDE08 - precision < 8
 * V$SIGNDE16 - precision < 16
 * V$SIGNDE24 - precision < 24
 * V$SIGNDE32 - precision >= 32
 *----------------------------------------------------------------------------
 */

long
P$SIGN_DE (
	X_DEC src,
	long  src_pq
	);

long
V$SIGNDE08 (
	ULONG src,
	long  src_pq
	);

long
V$SIGNDE16 (
	ULONG src_hw,
	ULONG src_lw,
	long  src_pq
	);

long
V$SIGNDE24 (
	ULONG src_hw,
	ULONG src_mw,
	ULONG src_lw,
	long  src_pq
	);

long
V$SIGNDE32 (
	X_DEC src,
	long  src_pq
	);


/*
 *----------------------------------------------------------------------
 * libc functions imported by libpl1.
 *----------------------------------------------------------------------
 */

double
atof (
	char *
	);

/*
 * The values which ecvt returns for "Infinity" and "Nan".
 */
#define ECVT_INFINITY	'I'
#define ECVT_NAN	'N'
char *
ecvt (
	double,
	int,
	int*,
	int*
	);


/*
 *----------------------------------------------------------------------
 * float decimal functions exported by libpl1.
 *----------------------------------------------------------------------
 */

void
V$ADDFD (
	FLT_DEC,
	long,
	FLT_DEC,
	long,
	FLT_DEC,
	long
	);

void
V$FDNN (
	FLT_DEC,
	Sign,
	long
	);

void
V$FDIN (
	FLT_DEC,
	Sign
	);

long
V$FDIZ (
	FLT_DEC
	);

void
V$SUBFD (
	FLT_DEC target,
	long    target_p,
	FLT_DEC op1,
	long    op1_p,
	FLT_DEC op2,
	long    op2_p
	);

void
V$NEGFD (
	FLT_DEC target,
	long    target_p,
	FLT_DEC source,
	long    source_p
	);

void
V$DIVFD (
	FLT_DEC target,
	long   target_p,
	FLT_DEC op1,
	long   op1_p,
	FLT_DEC op2,
	long   op2_p
	);

void
V$MULFD (
	FLT_DEC target,
	long    target_p,
	FLT_DEC op1,
	long    op1_p,
	FLT_DEC op2,
	long    op2_p
	);

int
M$FDTY (
	FLT_DEC me
	);

void
V$ASSFD (
	FLT_DEC target,
	long    target_p,
	FLT_DEC source,
	long    source_p
	);

void
V$ABSFD (
	FLT_DEC target,
	long    target_p,
	FLT_DEC source,
	long    source_p
	);

void
V$FDZR (
	FLT_DEC target
	);

long
V$CMPFD (
	FLT_DEC op1,
	long    op1_p,
	FLT_DEC op2,
	long    op2_p
	);

void
V$MAXFD (
	FLT_DEC target,
	long    target_p,
	FLT_DEC op1,
	long    op1_p,
	FLT_DEC op2,
	long    op2_p
	);

long
V$SGNFD (
	FLT_DEC me
	);

void
V$MINFD (
	FLT_DEC target,
	long    target_p,
	FLT_DEC op1,
	long    op1_p,
	FLT_DEC op2,
	long    op2_p
	);

void
V$CF4FD (
	double  source,
	FLT_DEC target,
	long    target_p
	);

void
V$CI4FD (
	long    source,
	FLT_DEC target,
	long    target_p
	);

void
V$CDEFD (
	X_DEC   source,
	long    source_pq,
	FLT_DEC target,
	long	  target_p
	);

long
V$CFDCH (
	FLT_DEC source,
	long    source_p,
	char    *target,
	long    target_dopev
	);

long
V$CCHFD (
	char    *source,
	long    source_dopev,
	FLT_DEC target,
	long    target_p
	);

float
V$CFDF2 (
	FLT_DEC source,
	long    source_p
	);

double
V$CFDF4 (
	FLT_DEC source,
	long    source_p
	);

void
V$CFDDE (
	FLT_DEC source,
	long    source_p,
	X_DEC   target,
	long    target_pq
	);

void
P$CFDDE (
	FLT_DEC source,
	short   *source_p,
	X_DEC   target,
	short   *target_pq
	);

long
V$CFDI4 (
	FLT_DEC source,
	long    source_p
	);

/*
 *----------------------------------------------------------------------------
 * end of exported floating decimal routines.
 *----------------------------------------------------------------------------
 */
