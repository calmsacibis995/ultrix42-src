/*	@(#)itod.c	4.1	ULTRIX	7/3/90	*/
#include	"VAX_to_IEEE.h"

double		itod	(value)

		double_precision	*value;

{	double_precision	argument;
	double_precision	result;
	unsigned short		temp;
	unsigned long		work;

	/* We ignore true zeros.					*/

	if	((value->longs [0] | value->longs [1]) == 0)
	{	return	(0);
	}

	/* Save the input value in the result area for processing.	*/

	result.longs [0] = value->longs [0];
	result.longs [1] = value->longs [1];

	/* Look for NaNs, translating them into reserved operands.	*/
	/* (equivalent to -0.0).					*/

	if	((result.IDL.exponent_10__0 == (ID_E_biased_max + 1)) &&
		 ((result.IDL.fraction_51_32 | result.IDL.fraction_31__0) != 0)
		)
	{	value->longs [0] = 0;
		value->longs [1] = 0;
		value->VD.sign = 1;
		return (0);
	}

	/* Check the exponent for range problems, signalling overflow	*/
 	/* if too large, underflow if too small.			*/

	if	(VD_E_biased_max < (result.IDL.exponent_10__0 + VD_ID_E_dif))
	{	return (-1);
	}
	if	((result.IDL.exponent_10__0 + VD_ID_E_dif) < VD_E_biased_min)
	{	return (-1);
	}

	/* Make the conversion, by:					*/
	/*								*/
	/*	(a) forming a new, smaller, exponent,			*/
	/*	(b) copying the sign, and				*/
	/*	(c) forming a new, larger, mantissia and [implicitly]	*/
	/*	    swaping the words (0 <> 3, 1 <> 2).			*/

	result.VD.exponent__7__0 = value->IDL.exponent_10__0 + VD_ID_E_dif;

	result.VD.sign = value->IDL.sign;

	result.VD.fraction_54_48 =
		(value->IDL.fraction_51_32 >> 13);
	result.VD.fraction_47_32 =
		(value->IDL.fraction_51_32 <<  3) +
		(value->IDL.fraction_31__0 >> 29);
	result.VD.fraction_31_16 =
		(value->IDL.fraction_31__0 >> 13);
	result.VD.fraction_15__0 =
		(value->IDL.fraction_31__0 <<  3);

	/* Return the result.						*/

	value->longs [0] = result.longs [0];
	value->longs [1] = result.longs [1];
	return	(0);
}
