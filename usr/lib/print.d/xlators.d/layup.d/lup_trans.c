#ifdef ultrix
#ifdef lint
static char *sccsid = "@(#)lup_trans.c	4.1	(ULTRIX)	7/2/90";
#endif lint
#endif ultrix

/***	
 ***	trans.c --
 ***	
 ***	Do the actual translation for the Layup Definition File
 ***	Translator.
 ***	
 ***	N.Batchelder, 2/9/87.
 ***/

# include "lup_def.h"
# include "lup_errs.h"

/** 
 ** Keyword processing stuff.
 **/

typedef struct keyent {
	char	*keyword;		/* The keyword */
	char	*psarg;			/* The PS arg to use for this keyword */
} keyent;

 private keyent	flipkeys[] = {
	"left",		"(l)",
	"right",	"(l)",
	"top",		"(t)",
	"bottom",	"(t)",
	NULL
};

 private keyent	pokeys[] = {
	"rightdown",		"(rd)",
	"rightup",		"(ru)",
	"leftdown",		"(ld)",
	"leftup",		"(lu)",
	"downright",		"(dr)",
	"upright",		"(ur)",
	"downleft",		"(dl)",
	"upleft",		"(ul)",
	NULL
};

/* 
 * The option interpreter is table-driven. Each option has an entry in the
 * table with the following fields:
 * 
 *  	name		is a character string containing the name of the
 *  			option. It is in all lower case, but any mixture of
 *  			cases will be recognized.
 * 
 *  	psfunc		is a character string containing the name of the
 *  			PostScript function to call to set the parameter.
 * 
 *  	negatable	is a boolean saying if the user is allowed to negate
 *  			the option. Negation is as with qualifiers, put `No'
 *  			before the name.
 * 
 *  	takesvals	is a boolean saying if the user is allowed to use an 
 *  			equals sign to specify values for the option.
 * 
 *  	yesargs		is a string containing PostScript arguments to use if
 *			the option is named but no values are given. NULL
 *			means that there are no defaults, and values must be
 *			provided.
 * 
 *	noargs		is a string containing PostScript arguments to use if
 *			the option is negated. If negatable is true, this will
 *			be non-NULL.
 * 
 *	process		is a pointer to a function to call to massage the
 *			values into arguments. If no special processing is
 *			needed, NULL is given.
 * 
 * The end of the table is marked by an entry with name == NULL.
 */

typedef struct optent {
	char	*name;		/* The name of the entry */
	char	*psfunc;	/* The PS function to invoke */
	flag	negatable;	/* Can we negate it? */
	flag	takesvals;	/* Can we supply values? */
	char	*yesargs;	/* PS args if just mentioned */
	char	*noargs;	/* PS args if just negated */
	int	(*process)();	/* Function to process args */
	int	procarg;	/* A random argument for the processor */
} optent;

# define CanNegate	true
# define CantNegate	false
# define CanValue	true
# define CantValue	false

private errcode	domargins(),		/* Functions mentioned in the table. */
		dogrid(),
		dokeyword(),
		checkint();

 optent	opttable[] = {
{	"margins",			/* Specify the margins */
	"set-margins",		
	CanNegate,			/* Can negate it */
	CanValue,			/* Can provide values */
	"36.0 36.0 36.0 36.0",		/* If no values, 1/2" all around */
	"0.0 0.0 0.0 0.0",		/* If negated, 0" all around */
	(int (*)())domargins,
	0				/* A place holder */
},
{	"grid",				/* Specify the size of the page grid */
	"set-page-grid",
	CantNegate,
	CanValue,
	NULL,				/* Must specify a value */
	NULL,
	(int (*)())dogrid,
	0
},
{	"borders",			/* Specify lines around pages */
	"set-lines-around-pages",
	CanNegate,
	CantValue,
	"true",
	"false",
	NULL,
	0
},
{	"alternate",			/* Specify margin alternation */
	"set-margin-flip",
	CanNegate,
	CanValue,
	"(l)",			/* Default is to do left and right */
	"()",			/* No means none */
	(int (*)())dokeyword,
	(int)flipkeys,
},
{	"pageorder",		/* Specify page order */
	"set-page-order",
	CantNegate,
	CanValue,
	NULL,			/* No default if just mentioned */
	NULL,			/* Can't negate */
	(int (*)())dokeyword,
	(int)pokeys,
},
{	"firstpage",		/* Specify the first spot on the sheet */
	"set-first-page",
	CantNegate,
	CanValue,
	NULL,			/* Must specify something */
	NULL,			/* Can't negate anyway */
	(int (*)())checkint,		/* Check that it is at least... */
	1			/*	one */
},
{	"pagespersheet",	/* How many pages to really put on a sheet */
	"set-pages-per-sheet",
	CantNegate,
	CanValue,
	NULL,			/* Must specify something */
	NULL,			/* Can't negate anyway */
	(int (*)())checkint,		/* Check that it is at least... */
	1			/*	one */
},
{	NULL			/* Marker for end of list */
}};

/* 
 * transline:
 * 
 * Takes an input buffer containing a line of the input, and an output buffer
 * which we fill with a line of the output. All comments and white space have
 * been stripped out at this point, so we only have substantive characters
 * left (and they're all lower case!). The return value is one of the error
 * codes. 
 */

errcode
transline(in, out)
char	in[], out[];
{
	extern char *strchr();
	flag	negated = false;	/* Is this line a negated option? */
	char	*values;
	optent	*tptr;

	if (in[0] == 'n' && in[1] == 'o') {
		negated = true;
		in += 2;
	}

	/* 
	 * Look for the equals sign.
	 */


	values = strchr(in, '=');
	if (values != NULL) {
		*values++ = '\0';	/* Terminate option and point to vals */
	}

	if (strlen(in) == 0) {
		handleerror(NoOption, "");
		return NoOption;
	}

	/* 
	 * Scan the table until we find an entry with the option's name in it.
	 */

	for (tptr = opttable; tptr->name != NULL; tptr++) {
		if (strequal(tptr->name, in)) {
			break;
		}
	}

	/* 
	 * Make sure we actually found an option.
	 */
	
	if (tptr->name == NULL) {
		handleerror(UnknownOpt, in);		/* No such option */
		return UnknownOpt;
	}

	/* 
	 * We have the option now, so interpret the table's fields.
	 */

	if (negated) {
		if (tptr->negatable == CantNegate) {
			handleerror(NoNegOpt, in);
			return NoNegOpt;	/* Can't negate, but he did */
		}
		if (values != NULL) {
			handleerror(NegAndValue, in);
			return NegAndValue;	/* Negated and values! */
		}

		/* 
		 * Everything is ok, output the negated arguments.
		 */

		strcpy(out, tptr->noargs);
	} else {
		if (values == NULL) {
			/* 
			 * No values, not negated, use default arguments.
			 */

			if (tptr->yesargs == NULL) {
				handleerror(NeedValue, in);
				return NeedValue;	/* Must supply vals */
			}

			strcpy(out, tptr->yesargs);
		} else {
			if (tptr->takesvals == CantValue) {
				handleerror(NoValAllowed, in);
				return NoValAllowed;	/* Can't supply vals */
			}
			if (strlen(values) == 0) {
				handleerror(NoValue, in);
				return NoValue;
			}

			if (tptr->process != NULL) {
				errcode	stat;

				/* 
				 * If there is a processing function, call it.
				 */

				stat = (errcode)(*tptr->process)
						(tptr->procarg, values, out);

				if (stat != Success) {
					return stat;
				}
			} else {
				/* 
				 * No processing, just copy the values.
				 */

				strcpy(out, values);
			} /* if process */
		} /* if values */
	} /* if negated */

	/* 
	 * Now write out the PostScript function name (with a space), and
	 * return.
	 */

	strcat(out, " ");
	strcat(out, tptr->psfunc);

	return Success;
}

/** 
 ** Table-specified processing filters.
 ** 
 ** All are called the same way: The first value is whatever value was
 ** specified in the option table. The second is an input character string
 ** containing the values to be translated (ie, whatever portion of the line
 ** followed the '='). The third is a pointer to a buffer where the PostScript
 ** output should be written. The output consists only of the arguments to the
 ** function. The function name itself will be tacked on by the caller.
 ** 
 ** The return value is an error code, which may be Success. The function is
 ** also responsible for calling handleerror() to actually produce an error
 ** message as well.
 **/

# define Fail(err,val)	{ handleerror(err,val); return err;}
# define Succeed()	return Success

/* 
 * dokeyword:
 * 
 * Takes a pointer to a keyword table, an input buffer and an output buffer,
 * and translates the keyword it finds in the input to the output. A flag is
 * returned indicating if the keyword was successfully translated or not.
 */

private errcode
dokeyword(ktab, in, out)
keyent	ktab[];
char	*in, *out;
{
	while (ktab->keyword != NULL) {
		if (strequal(ktab->keyword, in)) {
			strcpy(out, ktab->psarg);
			Succeed();
		}
		ktab++;
	}

	Fail(UnknownKey, in);		/* Keyword not found */
}

/* 
 * domargins:
 * 
 * Process the Margins= values. 
 */

private errcode
domargins(dummy, in, out)
int	dummy;
char	*in, *out;
{
	float	ml, mr, mt, mb;
	int	numvals;
	char	slop[100];

	numvals = sscanf(in, "%f,%f,%f,%f%s", &ml, &mr, &mt, &mb, slop);

	if (numvals < 4) {
		Fail(BadMargins, in);
	}
	if (numvals > 4) {
		Fail(ExtraChars, "");
	}

	sprintf(out, "%.1f %.1f %.1f %.1f", ml, mr, mt, mb);
	Succeed();
}

/* 
 * dogrid:
 * 
 * Read the two values for the page grid specification.
 */

# define MaxPageCount	100		/* No more than this in a grid */

private errcode
dogrid(dummy, in, out)
int	dummy;
char	*in, *out;
{
	int	gridx, gridy;
	int	numvals;
	char	slop[100];

	numvals = sscanf(in, "%d,%d%s", &gridx, &gridy, slop);

	if (numvals < 2) {
		Fail(BadGrid, in);
	}
	if (numvals > 2) {
		Fail(ExtraChars, "");
	}
	if (gridx <= 0 || gridy <= 0) {
		Fail(RangeCheck, 0);
	}
	if (gridx * gridy > MaxPageCount) {
		Fail(TooManyPages, MaxPageCount);
	}

	sprintf(out, "%d %d", gridx, gridy);
	Succeed();
}

/* 
 * checkint:
 * 
 * Check that the argument is an integer greater than the number supplied.
 */

private errcode
checkint(lowest, in, out)
int	lowest;
char	*in, *out;
{
	int	val;
	int	numvals;
	char	slop[100];

	numvals = sscanf(in, "%d%s", &val, slop);

	if (numvals < 1) {
		Fail(NoNumber, in);
	}
	if (numvals > 1) {
		Fail(ExtraChars, "");
	}

	if (val < lowest) {
		Fail(RangeCheck, lowest);
	}

	sprintf(out, "%d", val);
	Succeed();
}

/* end of trans.c */
