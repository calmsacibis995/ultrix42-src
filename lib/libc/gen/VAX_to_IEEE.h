/*	@(#)VAX_to_IEEE.h	4.1	ULTRIX	7/3/90	*/
/***********************************************************************\
*									*
*	From the "VAX Architecture" manual, and the "mips R2000 RISC	*
*	Architecture" manual, the picture for floating point numbers.	*
*									*
*									*
*   VAX F (single precision) floating point:				*
*									*
*    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1                        *
*    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0	*
*   +-------------------------------+-+---------------+-------------+	*
*   | fraction (15-0)               |S| exponent      | frac (22-16)| 0	*
*   +-------------------------------+-+---------------+-------------+	*
*									*
\***********************************************************************/

typedef	struct
	{	unsigned	fraction_22_16	:  7;
		unsigned	exponent__7__0	:  8;
		unsigned	sign		:  1;
		unsigned	fraction_15__0	: 16;
	}	VF_float;

#define	VF_E_max	127
#define	VF_E_min	-127
#define	VF_E_bias	128
#define	VF_E_biased_max	(VF_E_max + VF_E_bias)
#define	VF_E_biased_min	(VF_E_min + VF_E_bias)

/***********************************************************************\
*									*
*   IEEE single precision floating point (little endian):		*
*									*
*    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1                        *
*    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0	*
*   +-+---------------+---------------------------------------------+	*
*   |S| exponent      | fraction (22-0)                             | 0	*
*   +-+---------------+---------------------------------------------+	*
*									*
\***********************************************************************/

typedef	struct
	{	unsigned	fraction_22__0	: 23;
		unsigned	exponent__7__0	:  8;
		unsigned	sign		:  1;
	}	ISL_float;

/***********************************************************************\
*									*
*   IEEE single precision floating point (big endian):			*
*									*
*    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1                        *
*    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0	*
*   +-----------------+-------------+-+-------------+-+-------------+	*
*   | frac (7-0)      | frac (15-8) |e| frac (22-16)|S| exp (7-1)   | 0	*
*   +-----------------+-------------+-+-------------+-+-------------+	*
*									*
\***********************************************************************/

typedef	struct
	{	unsigned	exponent__7__1	:  7;
		unsigned	sign		:  1;
		unsigned	fraction_22_16	:  7;
		unsigned	exponent__0__0	:  1;
		unsigned	fraction_15__8	:  8;
		unsigned	fraction__7__0	:  8;
	}	ISB_float;

#define	IS_E_max	127
#define	IS_E_min	-126
#define	IS_E_bias	127
#define	IS_E_biased_max	(IS_E_max + IS_E_bias)
#define	IS_E_biased_min	(IS_E_min + IS_E_bias)
#define	IS_NaN		0XFFFFFFFF

/***********************************************************************\
*									*
*   VAX D (double precision) floating point:				*
*									*
*    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1                        *
*    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0	*
*   +-------------------------------+-+---------------+-------------+	*
*   | fraction (47-32)              |S| exponent      | frac (54-48)| 0	*
*   +-------------------------------+-+---------------+-------------+	*
*   | fraction (15-0)               | fraction (31-16)              | 4	*
*   +---------------------------------------------------------------+	*
*    6 6 6 6 5 5 5 5 5 5 5 5 5 5 4 4 4 4 4 4 4 4 4 4 3 3 3 3 3 3 3 3	*
*    3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2	*
*									*
\***********************************************************************/

typedef	struct
	{	unsigned	fraction_54_48	:  7;
		unsigned	exponent__7__0	:  8;
		unsigned	sign		:  1;
		unsigned	fraction_47_32	: 16;
		unsigned	fraction_31_16	: 16;
		unsigned	fraction_15__0	: 16;
	}	VD_float;

#define	VD_E_max	127
#define	VD_E_min	-127
#define	VD_E_bias	128
#define	VD_E_biased_max	(VD_E_max + VD_E_bias)
#define	VD_E_biased_min	(VD_E_min + VD_E_bias)

/***********************************************************************\
*									*
*   VAX G (double precision) floating point:				*
*									*
*    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1                        *
*    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0	*
*   +-------------------------------+-+---------------------+-------+	*
*   | fraction (47-32)              |S| exponent            | frac  | 0	*
*   +-------------------------------+-+---------------------+-------+	*
*   | fraction (15-0)               | fraction (31-16)              | 4	*
*   +-------------------------------+-------------------------------+	*
*    6 6 6 6 5 5 5 5 5 5 5 5 5 5 4 4 4 4 4 4 4 4 4 4 3 3 3 3 3 3 3 3	*
*    3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2	*
*									*
\***********************************************************************/

typedef	struct
	{	unsigned	fraction_51_48	:  4;
		unsigned	exponent_10__0	: 11;
		unsigned	sign		:  1;
		unsigned	fraction_47_32	: 16;
		unsigned	fraction_31_16	: 16;
		unsigned	fraction_15__0	: 16;
	}	VG_float;

#define	VG_E_max	1023
#define	VG_E_min	-1023
#define	VG_E_bias	1024
#define	VG_E_biased_max	(VG_E_max + VG_E_bias)
#define	VG_E_biased_min	(VG_E_min + VG_E_bias)

/***********************************************************************\
*									*
*   IEEE double precision floating point (little endian):		*
*									*
*    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1                        *
*    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0	*
*   +---------------------------------------------------------------+	*
*   | fraction (31-0)                                               | 0	*
*   +-+---------------------+---------------------------------------+	*
*   |S| exponent            | fraction (51-32)                      | 4	*
*   +-+---------------------+---------------------------------------+	*
*    6 6 6 6 5 5 5 5 5 5 5 5 5 5 4 4 4 4 4 4 4 4 4 4 3 3 3 3 3 3 3 3	*
*    3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2	*
*									*
\***********************************************************************/

typedef	struct
	{	unsigned	fraction_31__0	: 32;
		unsigned	fraction_51_32	: 20;
		unsigned	exponent_10__0	: 11;
		unsigned	sign		:  1;
	}	IDL_float;

/***********************************************************************\
*									*
*   IEEE double precision floating point (big endian):			*
*									*
*    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1                        *
*    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0	*
*   +----------------+--------------+-------+-------+-+-------------+	*
*   | frac (39-32)   | frac (47-40) | exp   | frac  |S| exp (10-4)  | 0	*
*   +----------------+--------------+-------+-------+-+-------------+	*
*   | frac (7-0)     | frac (15-8)  | frac (23-16)  | frac (31-24)  | 4	*
*   +----------------+--------------+---------------+---------------+	*
*    6 6 6 6 5 5 5 5 5 5 5 5 5 5 4 4 4 4 4 4 4 4 4 4 3 3 3 3 3 3 3 3	*
*    3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2	*
*									*
\***********************************************************************/

typedef	struct
	{	unsigned	exponent_10__4	:  7;
		unsigned	sign		:  1;
		unsigned	fraction_51_48	:  4;
		unsigned	exponent__3__0	:  4;
		unsigned	fraction_47_40	:  8;
		unsigned	fraction_39_32	:  8;
		unsigned	fraction_31_24	:  8;
		unsigned	fraction_23_16	:  8;
		unsigned	fraction_15__8	:  8;
		unsigned	fraction__7__0	:  8;
	}	IDB_float;

#define	ID_E_max	1023
#define	ID_E_min	-1022
#define	ID_E_bias	1023
#define	ID_E_biased_max	(ID_E_max + ID_E_bias)
#define	ID_E_biased_min	(ID_E_min + ID_E_bias)

/***********************************************************************\
*									*
\***********************************************************************/

typedef	union
	{	VF_float	VF;
		ISL_float	ISL;
		ISB_float	ISB;
		unsigned char	bytes [4];
		unsigned short	words [2];
		unsigned long	longs [1];
		float		bits;
	}	single_precision;

#define	VF_IS_E_dif	((VF_E_bias - IS_E_bias) + 1)

typedef	union
	{	VD_float	VD;
		VG_float	VG;
		IDL_float	IDL;
		IDB_float	IDB;
		unsigned char	bytes [8];
		unsigned short	words [4];
		unsigned long	longs [2];
		double		bits;
	}	double_precision;

#define	VD_ID_E_dif	((VD_E_bias - ID_E_bias) + 1)
#define	VG_ID_E_dif	((VG_E_bias - ID_E_bias) + 1)

