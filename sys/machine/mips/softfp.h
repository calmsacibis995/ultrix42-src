/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * softfp.h -- constants for software floating point emulation
 */

/*
 * The _MASK's are used to get a the specified field after it has been
 * shifted by _SHIFT and then bit patterns (like _COPN) can be used to test
 * the field.
 */
/* constants for the OPCODE field for some general instructions */
#define	OPCODE_SHIFT	26
#define	OPCODE_MASK	0x3f
#define	OPCODE_SPECIAL	0x00
#define	OPCODE_BCOND	0x01
#define	OPCODE_J	0x02
#define	OPCODE_JAL	0x03
#define	OPCODE_BEQ	0x04
#define	OPCODE_C1	0x11

/* constants for the emulating jump or jump and link instructions */
#define	TARGET_MASK	0x03ffffff
#define	PC_JMP_MASK	0xf0000000

/* constants for the FUNC field for some general instructions */
#define	FUNC_MASK	0x3f
#define	FUNC_JR		0x08
#define	FUNC_JALR	0x09

/*
 * constants for the OPCODE field for detecting all general branch
 * (beq,bne,blez,bgtz) instructions and all coprocessor instructions.
 */
#define	BRANCH_MASK	0x3c
#define	OPCODE_BRANCHES	0x04
#define	COPN_MASK	0x3c
#define	OPCODE_COPN	0x10

/* constants for load/store COPN instructions */
#define	OP_LSWCOPNMASK	0x37
#define	OP_LSWCOPN	0x31
#define OP_LSBITMASK	0x8
#define OP_LBIT		0x0

/* constants for branch on COPN condition instructions */
#define	COPN_BCSHIFT	24
#define	COPN_BCMASK	0x3
#define	COPN_BC		0x1
#define	BC_TFBITSHIFT	16
#define	BC_TFBITMASK	0x1
#define BC_FBIT		0x0

/* constants for move to/from COPN instructions */
#define	COPN_MTFSHIFT	25
#define	COPN_MTFMASK	0x1
#define	COPN_MTF	0x0
#define	COPN_MTFBITSHIFT	23
#define	COPN_MTFBITMASK	0x1
#define COPN_MFBIT	0x0

/* constants for move control registers to/from CP1 instructions */
#define M_CONBITSHIFT	22
#define	M_CONBITMASK	0x1

#define FPR_REV		0
#define FPR_EIR		30
#define FPR_CSR		31
#define	SOFTFP_REVWORD	0x0

/*
 * These constants refer to the fields of coprocessor instructions not
 * cpu instructions (ie the RS and RD fields are different).
 */
#define BASE_SHIFT	21
#define BASE_MASK	0x1f
#define RT_SHIFT	16
#define	RT_MASK		0x1f
#define	RT_FPRMASK	0x1e
#define RS_SHIFT	11
#define	RS_MASK		0x1f
#define	RS_FPRMASK	0x1e
#define RD_SHIFT	6
#define	RD_MASK		0x1f
#define	RD_FPRMASK	0x1e

#define IMMED_SHIFT	16

#define C1_FMT_SHIFT	21
#define	C1_FMT_MASK	0xf
#define C1_FMT_SINGLE	0
#define C1_FMT_DOUBLE	1
#define C1_FMT_EXTENDED	2
#define C1_FMT_QUAD	3
#define C1_FMT_WORD	4
#define C1_FMT_MAX	4

#define C1_FUNC_MASK	0x3f
#define C1_FUNC_DIV	3
#define C1_FUNC_NEG	7
#define C1_FUNC_CVTS	32
#define C1_FUNC_CVTW	36
#define C1_FUNC_1stCMP	48

#define COND_UN_MASK	0x1
#define COND_EQ_MASK	0x2
#define COND_LT_MASK	0x4
#define COND_IN_MASK	0x8

/*
 * These constants refer to fields in the floating-point status and control
 * register.
 */
#define	CSR_CBITSHIFT	23
#define	CSR_CBITMASK	0x1
#define	CSR_CBITSET	0x00800000
#define	CSR_CBITCLEAR	0xff7fffff

#define	CSR_EXCEPT	0x0003f000
#define	UNIMP_EXC	0x00020000
#define	INVALID_EXC	0x00010040
#define	DIVIDE0_EXC	0x00008020
#define	OVERFLOW_EXC	0x00004010
#define	UNDERFLOW_EXC	0x00002008
#define	INEXACT_EXC	0x00001004

#define CSR_ENABLE		0x00000f80
#define	INVALID_ENABLE		0x00000800
#define	DIVIDE0_ENABLE		0x00000400
#define	OVERFLOW_ENABLE		0x00000200
#define	UNDERFLOW_ENABLE	0x00000100
#define	INEXACT_ENABLE		0x00000080

#define	CSR_RM_MASK	0x3
#define	CSR_RM_RN	0
#define	CSR_RM_RZ	1
#define	CSR_RM_RPI	2
#define	CSR_RM_RMI	3

/*
 * These constants refer to floating-point values for all formats
 */
#define	SIGNBIT		0x80000000

#define	GUARDBIT	0x80000000
#define	STKBIT		0x20000000

/*
 * These constants refer to word values
 */
#define	WORD_MIN	0x80000000
#define	WORD_MAX	0x7fffffff
#define	WEXP_MIN	-1
#define	WEXP_MAX	30
#define	WQUIETNAN_LEAST	0x7fffffff

/*
 * These constants refer to single format floating-point values
 */
#define	SEXP_SHIFT	23
#define	SEXP_MASK	0xff
#define	SEXP_NAN	0xff
#define	SEXP_INF	0xff
#define	SEXP_BIAS	127
#define	SEXP_MAX	127
#define	SEXP_MIN	-126
#define	SEXP_OU_ADJ	192
#define	SIMP_1BIT	0x00800000
#define	SFRAC_LEAD0S	8
#define	SFRAC_BITS	23
#define	SFRAC_MASK	0x007fffff
#define	SFRAC_LEAST_MAX	0x007fffff

#define	SSNANBIT_MASK	0x00400000
#define	SQUIETNAN_LEAST	0x7fbfffff

/*
 * These constants refer to double format floating-point values
 */
#define	DEXP_SHIFT	20
#define	DEXP_MASK	0x7ff
#define	DEXP_NAN	0x7ff
#define	DEXP_INF	0x7ff
#define	DEXP_BIAS	1023
#define	DEXP_MAX	1023
#define	DEXP_MIN	-1022
#define	DEXP_OU_ADJ	1536
#define	DIMP_1BIT	0x00100000
#define	DFRAC_LEAD0S	11
#define	DFRAC_BITS	52
#define	DFRAC_MASK	0x000fffff
#define	DFRAC_LESS_MAX	0x000fffff
#define	DFRAC_LEAST_MAX	0xffffffff

#define	DSNANBIT_MASK	0x00080000
#define	DQUIETNAN_LESS	0x7ff7ffff
#define	DQUIETNAN_LEAST	0xffffffff
