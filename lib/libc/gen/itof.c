/*	@(#)itof.c	4.1	ULTRIX	7/3/90	*/
#include	"VAX_to_IEEE.h"

float		itof	(value)

		single_precision	*value;

{	single_precision	result;

	/* We ignore true zeros.					*/

	if	(value->longs [0] == 0)
	{	return	(0);
	}

	/* Look for NaNs, translating them into reserved operands.	*/

	if	((value->ISL.exponent__7__0 == (IS_E_biased_max + 1)) &&
		 (value->ISL.fraction_22__0 != 0)
		)
	{	value->longs [0] = 0;
		value->VF.sign = 1;
		return	(0);
	}

	/* Check the exponent for range problems, signalling overflow	*/
	/* if too large.						*/

	if	(VF_E_biased_max < (value->ISL.exponent__7__0 + VF_IS_E_dif))
	{	return (-1);
	}

	/* Make the conversion, by:					*/
	/*								*/
	/*	(a) swaping the words, and				*/
	/*	(b) biasing the exponent (+2).				*/

	result.words [0] = value->words [1];
	result.words [1] = value->words [0];

	result.VF.exponent__7__0 += VF_IS_E_dif;

	/* Return the result.						*/

	value->longs [0] = result.longs [0];
	return	(0);
}
