/*	@(#)dtoi.c	4.1	ULTRIX	7/3/90	*/
#include	"VAX_to_IEEE.h"

double		dtoi	(value)

		double_precision	*value;

{	double_precision	result;

	/* We ignore true zeros.					*/

	if	((value->longs [0] | value->longs [1]) == 0)
	{	return	(0);
	}

	/* Look for reserved operands, translating them into NaNs.	*/

	if	((result.VD.exponent__7__0 == 0) &&
		 (result.VD.sign != 0)
		)
	{	value->longs [0] = 0XFFFFFFFF;
		value->longs [1] = 0XFFFFFFFF;
		return	(0);
	}

	/* Make the conversion, by:					*/
	/*								*/
	/*	(a) forming a new, larger, exponent,			*/
	/*	(b) copying the sign, and				*/
	/*	(c) forming a new, smaller, mantissia and [implicitly]	*/
	/*	    swaping the words (0 <> 3, 1 <> 2).			*/

	result.IDL.exponent_10__0 = value->VD.exponent__7__0 - VD_ID_E_dif;

	result.IDL.sign = value->VD.sign;

	result.IDL.fraction_51_32 =
		(value->VD.fraction_54_48 << 13) +
		(value->VD.fraction_47_32 >>  3);
	result.IDL.fraction_31__0 =
		(value->VD.fraction_47_32 << 29) +
		(value->VD.fraction_31_16 << 13) +
		(value->VD.fraction_15__0 >>  3);

	/* Return the result.						*/

	value->longs [0] = result.longs [0];
	value->longs [1] = result.longs [1];
	return	(0);
}
