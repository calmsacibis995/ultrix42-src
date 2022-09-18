/*	@(#)ftoi.c	4.1	ULTRIX	7/3/90	*/
#include	"VAX_to_IEEE.h"

int		ftoi	(value)

	single_precision	*value;

{	single_precision	result;

	/* We ignore true zeros.					*/

	if	(value->longs [0] == 0)
	{	return	(0);
	}

	/* Look for reserved operands, translating them into NaNs.	*/

	if	((value->VF.exponent__7__0 == 0) &&
		 (value->VF.sign != 0)
		)
	{	value->longs [0] = 0XFFFFFFFF;
		return	(0);
	}

	/* Check the exponent for range problems, signalling underflow	*/
	/* if too small.						*/

	if	((value->VF.exponent__7__0 - VF_IS_E_dif) < IS_E_biased_min)
	{	return (-1);
	}

	/* Make the conversion, by:					*/
	/*								*/
	/*	(a) swaping the words, and				*/
	/*	(b) biasing the exponent (-2).				*/

	result.words [0] = value->words [1];
	result.words [1] = value->words [0];

	result.ISL.exponent__7__0 -= VF_IS_E_dif;

	/* Return the result.						*/

	value->longs [0] = result.longs [0];
	return	(0);
}
