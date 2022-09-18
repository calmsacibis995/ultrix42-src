#ifdef ultrix
#ifdef lint
static char *sccsid = "@(#)lup_error.c	4.1	(ULTRIX)	7/2/90";
#endif lint
#endif ultrix

/***	
 ***	lup_error.c --
 ***	
 ***	Do something with error messages for the Layup Definition File
 ***	Translator.
 ***	
 ***	N.Batchelder, 2/11/87.
 ***/

# include "lup_def.h"
# include "lup_errs.h"

extern  int	inputline;

/* 
 * handleerror:
 * 
 * Takes an error code and does something with it. We're kind of stuck as to
 * what we can do because of our calling environment. Our solution is to
 * print them as comments in the output, because then at least the developer
 * can see what's going on.
 */

handleerror(stat, arg)
errcode	stat;
any	arg;
{
	char	oline[100];
	char	msgtext[100];
	char	*msg;

	switch (stat) {

	case Success:
		msg = "Unexpected success in handleerror()!";
		break;

	case NoOption:			/* Arg is empty */
		msg = "No option present";
		break;

	case UnknownOpt:		/* Arg is option */
		msg = "Unrecognized option `%s'";
		break;

	case NoNegOpt:			/* Arg is option */
		msg = "Cannot negate option `%s'";
		break;

	case NoValue:			/* Arg is option */
		msg = "No values specified for option `%s'";
		break;

	case NegAndValue:		/* Arg is option */
		msg = "Cannot give values with negated option `%s'";
		break;

	case NeedValue:			/* Arg is option */
		msg = "Must supply a value for option `%s'";
		break;

	case NoValAllowed:		/* Arg is option */
		msg = "Cannot supply a value for option `%s'";
		break;

	case UnknownKey:		/* Arg is keyword */
		msg = "Unrecognized keyword `%s'";
		break;

	case BadMargins:		/* Arg is margin values */
		msg = "Bad form for margin values";
		break;

	case BadGrid:			/* Arg is page grid values */
		msg = "Bad form for page grid values";
		break;
		
	case TooManyPages:		/* Arg is how the page limit */
		msg = "Page count must be less than %d";
		break;
		
	case NoNumber:			/* Arg is bad string */
		msg = "Could not find a number as a value";
		break;

	case RangeCheck:			/* Arg is lowest int */
		msg = "Number must be greater than %d";
		break;

	case ExtraChars:			/* Arg is empty */
		msg = "Extra characters present after values";
		break;

	default:
		msg = "Unknown error type in handleerror()!";
		break;
	}

	sprintf(msgtext, msg, arg);
	sprintf(oline, "mark (%s) (%d) 144 returnstatus", msgtext, inputline); 
/*	sprintf(oline, "(%s on line %d in layup definition.\\n) print flush",
							msgtext, inputline); */
	putline(oline);
}

/* end of error.c */
