/*	@(#)itog.c	4.1	ULTRIX	7/3/90	*/
#include	"VAX_to_IEEE.h"

double		itog	(value)

		double_precision	*value;

{	double_precision	result;

	/* We ignore true zeros.					*/

	if	((value->longs [0] | value->longs [1]) == 0)
	{	return	(0);
	}

	/* Look for NaNs, translating them into reserved operands.	*/

	if	((value->IDL.exponent_10__0 == (ID_E_biased_max + 1)) &&
		 ((value->IDL.fraction_51_32 | value->IDL.fraction_31__0) != 0)
		)
	{	value->longs [0] = 0;
		value->longs [1] = 0;
		value->VG.sign = 1;
		return	(0);
	}

	/* Check the exponent for range problems, signalling overflow	*/
	/* if too large.						*/

	if	(VG_E_biased_max < (value->IDL.exponent_10__0 + VG_ID_E_dif))
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

	result.VG.exponent_10__0 += VG_ID_E_dif;

	/* Return the result.						*/

	value->longs [0] = result.longs [0];
	value->longs [1] = result.longs [1];
	return	(0);
}
