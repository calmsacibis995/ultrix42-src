#ifdef ultrix
#ifdef lint
static char *sccsid = "@(#)lup_errs.h	4.1	(ULTRIX)	7/2/90";
#endif lint
#endif ultrix

/***	
 ***	cps$lup_errs.h --
 ***	
 ***	Error handling definitions for the Page Layup Definition File
 ***	Translator.
 ***	
 ***	N.Batchelder, 2/11/87.
 ***/

/* 
 * The error code type.
 */

typedef enum {
	Success = 1723,
	NoOption,		/* No option on the line */
	UnknownOpt,		/* Don't recognize this option */
	NoNegOpt,		/* Can't negate this option */
	NoValue,		/* No value supplied */
	NegAndValue,		/* Can't give values with negated option */
	NeedValue,		/* Must supply a value for this option */
	NoValAllowed,		/* Can't supply a value for this option */
	UnknownKey,		/* Didn't recognize the keyword */
	BadMargins,		/* Bad form for margin values */
	BadGrid,		/* Bad form for page grid values */
	TooManyPages,		/* Can't have more than this many pages */
	NoNumber,		/* Didn't find a number as a value */
	RangeCheck,		/* Number was out of the proper range */
	ExtraChars,		/* Extra characters after values */
} errcode;

/* end of errs.h */
