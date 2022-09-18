/*	@(#)gtoi.c	4.1	ULTRIX	7/3/90	*/
#include	"VAX_to_IEEE.h"

double		gtoi	(value)

		double_precision	*value;

{	double_precision	result;

	/* We ignore true zeros.					*/

	if	((value->longs [0] | value->longs [1]) == 0)
	{	return	(0);
	}

	/* Look for reserved operands, translating them into NaNs.	*/

	if	((value->VG.exponent_10__0 == 0) &&
		 (value->VG.sign != 0)
		)
	{	value->longs [0] = 0XFFFFFFFF;
		value->longs [1] = 0XFFFFFFFF;
		return	(0);
	}

	/* Check the exponent for range problems, signalling underflow	*/
	/* if too small.						*/

	if	((value->VG.exponent_10__0 - VG_ID_E_dif) < ID_E_biased_min)
	{	return (-1);
	}

	/* Make the conversion, by:					*/
	/*								*/
	/*	(a) swaping the words (0 <> 3, 1 <> 2), and		*/
	/*	(b) biasing the exponent (-2).				*/

	result.words [0] = value->words [3];
	result.words [3] = value->words [0];

	result.words [1] = value->words [2];
	result.words [2] = value->words [1];

	result.IDL.exponent_10__0 -= VG_ID_E_dif;

	/* Return the result.						*/

	value->longs [0] = result.longs [0];
	value->longs [1] = result.longs [1];
	return	(0);
}
