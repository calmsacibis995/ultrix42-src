#ifndef lint
static	char	*sccsid = "@(#)sort5.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *                          Bull, France                                *
 *                         Siemens AG, FR Germany                       *
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * Modification History:
 *
 * 08-Jun-88	Mark Parenti
 *	Changed signal handlers to void.
 */

/*
 * source code for the sort program.
 *
 * For internationalization a (partial) rewrite was necessary to allow for
 *	1) the dynamic memory management used by the internationalization
 *	   routines
 *	2) To take out all the ascii codeset dependent tables/parts of
 *	   code.
 *
 * NAME:
 *	sort -- sort and/or merge files
 *
 * SYNOPSIS:
 *	sort [-cmu] [-o output] [-y kmem] [-z recsz] [-tx] [-T dir] [-bdfinrM]
 *	     [+pos[-pos]] [file...]
 *
 * DESCRIPTION:
 *	The command sort sorts lines of all the named files together
 *	and writes the result on stdout. Stdin is read when '-' or no
 *	file arguments are given.
 *
 *	Comparisons are based on one or more sort keys extracted from
 *	each line of input. By default the entire input line is the sort
 *	key and ordering is lexicographic by default collation sequence
 *	taken from the current International UNIX database.
 *
 *	When the last line of an input file is missing a newline, sort
 *	appends the newline, issues a diagnostic message and continues.
 *
 * OPTIONS:
 *	The following options alter the default behaviour:
 *
 *	-T dir	Use the named directory for temporary files
 *
 *	-c	Check that the input file is sorted according to the
 *		ordering rules. No output is given unless the file is
 *		out of sort.
 *
 *	-m	Do only the merge part. All input files are assumed to be
 *		already sorted.
 *
 *	-u	Supress all but one in each set of lines having equal keys.
 *		This also works with the -c option.
 *
 *	-o output Use the named file for output of the sort instead of stdout.
 *		Output may be the same as one of the input files.
 *
 *	-y kmem	Set the amount of memory used to kmem kilo bytes. If this
 *		option is ommitted, sort begins with a default size and
 *		continues to use more space as needed.  If no kmem argument
 *		is given, sort will use the maximum. A kmem value of 0
 *		will start using minimal memory. The maximum and minimum
 *		values are set at compile time and values outside this range
 *		are mapped onto max or min respectivly.
 *
 *	-z recsz Take the maximum length of an input line to be recsz bytes.
 *		Sort will determine the maximal size automatically when
 *		the sort phase is run through, so this option only has
 *		to be specified when either -c or -m is in effect. Lines
 *		longer than recsz will cause sort to terminate abnormally.
 *
 *	The following options override the default ordering rules:
 *
 *	-d	Only letters, digits and white space is significant in
 *		comparisons.
 *
 *	-f	Fold lower case letters to upper case.
 *
 *	-i	Ignore control characters in comparisons.
 *
 *	-n	Sort numerically on an initial numeric string, consisting
 *		of optional leading white space, optional minus sign and
 *		zero or more digits with optional radix character.
 *
 *	-r	reverse the sense of comparisons.
 *
 *	-M	Use numerical values of monthnames for comparisons
 *
 *	When given before a restricted sort key, the ordering rules apply
 *	globally to all sort keys. All of these options may also be
 *	attached to a specific sort key. In this case the attached options
 *	take precedence.
 *
 *	The notation +pos1 -pos2 restricts a sort key to the field beginning
 *	at pos1 and ending at pos2 inclusive. A missing pos2 means the end
 *	of the line.
 *
 *	Pos1 and pos2 have the form m[.n][bdfinrM].
 *		where m designates the m+1th field,
 *		      n designates the n+1th character in this field and
 *		      [bdfinrM] is the ordering rule to apply to the field.
 *
 *	A missing .n in pos1 denotes the first character of the field.
 *	A missing .n in pos2 denotes the last character of the field,
 *	values for n larger than zero specify characters after the end of
 *	the field.
 *
 *	A field is starts with optional white space and ends with the first
 *	white space character after a non white space sequence.
 *
 *	The following options influence the interpretation of fields:
 *
 *	-b Ignore leading blanks when determining the starting and ending
 *	   positions of a restricted sort key.
 *
 *	-tx Use x as the field separation character. x is not considered
 *	   to be part of a field. Each occurence of x is significant and
 *	   starts a new field.
 *
 *	When there are multiple sort keys, later keys are compared only
 *	after all earlier keys compare equal. Lines that otherwise compare
 *	equal are ordered with all codes significant.
 *
 * STATUS:
 *	!= 0 for various trouble conditions and for disorder under the -c
 *	option,
 *	0 otherwise.
 */

/*
 * include files
 */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <values.h>
#include <locale.h>
#include <langinfo.h>
#include <nl_types.h>

/*
 * message handling
 */
#ifdef INTLM		/************** X/OPEN message handling ************/
#define NL_SETN 1	/* set number */
nl_catd _m_catd;
nl_catd catopen();
char *catgets();
#else
#define catgets(a,b,c,d)	d
#endif

/*
 * The following constant _NFILE comes from stdio.h and is the number
 * of file pointers available to a program.
 */
#define NFILES	_NFILE
#define N	NFILES - 5	/* stdin, stdout, stderr, output, database */
#define NF	10		/* max number of field specifications */
#define MTHRESH	 8		/* threshhold for median of 3 qksort */
#if (NFILES > 32)
#define TREEZ	64		/* no less than N and best pow of 2 */
#else
#define TREEZ	32		/* no less than N and best pow of 2 */
#endif

/*
 * Memory administration:
 *
 * Using a lot of memory is great when sorting a lot of data.
 * Using a megabyte to sort the output of `who' loses big.
 * MAXMEM, MINMEM and DEFMEM define the absolute maximum,
 * minimum and default memory requirements.  Administrators
 * can override any or all of these via defines at compile time.
 * Users can override the amount allocated (within the limits
 * of MAXMEM and MINMEM) on the command line.
 *
 * For PDP-11s, memory is limited by the maximum unsigned number, 2^16-1.
 * Administrators can override this too.
 * Arguments to core getting routines must be unsigned.
 * Unsigned long not supported on 11s.
 */

#ifndef	MAXMEM
#ifdef pdp11
#define	MAXMEM		((1L << 16)-1)		/* 64 kB maximum */
#else
#define	MAXMEM		1048576		/* 1 MB maximum */
#endif
#endif

#ifndef	MINMEM
#define	MINMEM		16384		/* 16 kB minimum */
#endif

#ifndef	DEFMEM
#define	DEFMEM		32768		/* 32 kB default */
#endif

/*
 * First parameter passed to diag function determines the action taken
 * after the diagnostic message has been output.
 */
#define WARN	0			/* warning msg only: return to caller */
#define TERM	1			/* fatal error: call term() */
#define EXIT	2			/* fatal error: call exit() */

/*
 * comparison routines available
 */
int asciicmp();				/* use ascii comparison */
int numcmp();				/* use numeric comparison */
int monthcmp();				/* use monthname comparison */
int intlcmp();				/* external comparison from database */
int tagcmp();				/* compare tag fields */
void disorder();			/* disorder message and exit */

#define	USE	catgets(_m_catd, NL_SETN, 1, "invalid use of command line options")

#define	blank(c) (c == ' ' || c == '\t')

/*
 * globals for file handling
 */
#define MAXPLEN		100		/* max length of a temp file pathname */
#define NAMEOHD		12		/* length of tmp suffix "/stm00000aa" */
FILE	*os;				/* output file descriptor */
char	*dirtry[] = {"/usr/tmp", "/tmp", NULL};		/* temp file area */
char	*file;				/* pointer to tempfile name */
int	nfiles;				/* number of files to sort/merge */

int	maxrec;				/* size of one record (-z option) */
int	cmp();				/* compare field routine */
int	cmpa();				/* compare whole line routines */
int	(*compare)() = cmpa;		/* compare routine to use */
char	*eol();				/* skip to eol in a string */
char	*skip();			/* skip according to field specs */
void	term();				/* terminate sort */

/*
 * globals for option handling
 */
int 	mflg;				/* merge only flag */
int	nway;				/* ??? */
int	cflg;				/* check only flag */
int	uflg;				/* unique flag */
char	*outfil = "-";			/* output file name, if any */
int	unsafeout;			/* kludge to assure -m -o works */
char	tabchar;			/* field separator character given */
int     tag = 0;			/* TRUE if doing a tag sort */

/*
 * globals for memory management
 */
unsigned tryfor;			/* memory we want to have */
unsigned getmem();			/* memory allocator during sort */
char *malloc();				/* memory allocator */
char *realloc();			/* memory allocator */
long memmoved = 0;			/* amount moved during realloc */
					/* state of last getline */
#define OKAY	0
#define INHEAD  1
#define INREAD	2
#define INTAG   3
int  getstate = OKAY;			/* used in sort/getline */
char *recstart = (char *)0;		/* used if getline fails */

/*
 * Below earg[cv] replace arg[cv] in place while command arguments are
 * scanned.
 */
char	**eargv;			/* pointer to file argument vector */
int 	eargc;				/* argument count for file arguments */

/*
 * The following is used to implement tree sort/insert
 */
struct btree {				/* binary tree construct */
    char *rp;				/* contents of line */
    int  rn;				/* diverse uses ?? */
};
struct btree tree[TREEZ];
struct btree *treep[TREEZ];

int	blkcnt[TREEZ];
char	**blkcur[TREEZ];

long	wasfirst = 0;
long	notfirst = 0;
int	bonus;

/*
 * tables used in field specification.
 * see specification of struct field below
 * The interesting point is that always table[128] is used as starting
 * point (sign extension for characters is assumed!).
 *
 * NAME		TYPE		FUNCTION
 * fold:	code		used to fold upper to lower case
 * nofold:	code		used to differ between upper and lower case
 * zero:	ignore		used to ignore no characters
 * nonprint:	ignore		used to ignore non printable characters
 * dict:	ignore		used to ignore characters not in the dictionary
 */
char	fold[256] = {
	0200,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0212,0213,0214,0215,0216,0217,
	0220,0221,0222,0223,0224,0225,0226,0227,
	0230,0231,0232,0233,0234,0235,0236,0237,
	0240,0241,0242,0243,0244,0245,0246,0247,
	0250,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0300,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0312,0313,0314,0315,0316,0317,
	0320,0321,0322,0323,0324,0325,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0345,0346,0347,
	0350,0351,0352,0353,0354,0355,0356,0357,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0372,0373,0374,0375,0376,0377,

	0000,0001,0002,0003,0004,0005,0006,0007,
	0010,0011,0012,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0024,0025,0026,0027,
	0030,0031,0032,0033,0034,0035,0036,0037,
	0040,0041,0042,0043,0044,0045,0046,0047,
	0050,0051,0052,0053,0054,0055,0056,0057,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0072,0073,0074,0075,0076,0077,
	0100,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0112,0113,0114,0115,0116,0117,
	0120,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0132,0133,0134,0135,0136,0137,
	0140,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0112,0113,0114,0115,0116,0117,
	0120,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0132,0173,0174,0175,0176,0177
};

char nofold[256] = {
	0200,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0212,0213,0214,0215,0216,0217,
	0220,0221,0222,0223,0224,0225,0226,0227,
	0230,0231,0232,0233,0234,0235,0236,0237,
	0240,0241,0242,0243,0244,0245,0246,0247,
	0250,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0300,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0312,0313,0314,0315,0316,0317,
	0320,0321,0322,0323,0324,0325,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0345,0346,0347,
	0350,0351,0352,0353,0354,0355,0356,0357,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0372,0373,0374,0375,0376,0377,

	0000,0001,0002,0003,0004,0005,0006,0007,
	0010,0011,0012,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0024,0025,0026,0027,
	0030,0031,0032,0033,0034,0035,0036,0037,
	0040,0041,0042,0043,0044,0045,0046,0047,
	0050,0051,0052,0053,0054,0055,0056,0057,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0072,0073,0074,0075,0076,0077,
	0100,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0112,0113,0114,0115,0116,0117,
	0120,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0132,0133,0134,0135,0136,0137,
	0140,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0152,0153,0154,0155,0156,0157,
	0160,0161,0162,0163,0164,0165,0166,0167,
	0170,0171,0172,0173,0174,0175,0176,0177
};

char zero[256];

char	nonprint[256] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,

	1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1
};

char	dict[256] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,

	1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1
};

/*
 * structure used to hold a field specification
 * The array used all have two elements.
 * Element 1 (index 0) holds the information for +pos
 * Element 2 (index 1) holds the information for -pos
 */
struct field {
	char *code;		/* pointer to table fold/nofold */
	char *ignore;		/* pointer to table zero/nonprint/dict */
	int (*fcmp)();		/* pointer to comparison routine */
	int rflg;		/* sign of comparison 1 or -1 */
	int bflg[2];		/* ignore leading/trailing blanks */
	int m[2];		/* field number for + and - pos */
	int n[2];		/* byte number for + and - pos */
};

/*
 * specification of fields.
 * fields[0] is used if no fields are specified and for the
 * global bnifrM options in case of additional field specifications
 */
struct field fields[NF];	/* The array of field specifications */

/*
 * prototype field specification. This is also the default.
 */
struct field proto = {
	nofold+128,		/* upper and lower case are distinct */
	zero+128,		/* do not ignore any characters */
	asciicmp,		/* use ASCII native comparison */
	1,			/* ordinary sense of comparison */
	0,0,			/* no not ignore blanks */
	0,-1,			/* take whole line */
	0,0			/* byte positions are ignored */
};

int	nfields;		/* number of fields to sort */
char	*setfil();		/* generate temp file names */
FILE	*output();		/* open an output file */
FILE	*input();		/* open an input file */
void	doclose();		/* close a file used by sort */

/*
 * main -- sort/merge files
 */
main(argc, argv)
int argc;
char **argv;
{
	register a;
	char *arg;		/* pointer into argument */
	struct field *p;
	struct field *q;
	int i;			/* temporary uses */

	/*
	 * close any file descriptors that may have been left open
	 * sort/merge may need them all.
	 */
	for (i = 3; i < NFILES; i++)
		(void) close(i);

	/*
 	 * open corresponding catalogue file
	 */
#ifdef INTLM
	 _m_catd = catopen("sort", 0);
#endif

	if (setlocale(LC_ALL, "")) {
		proto.fcmp = intlcmp;
		compare = cmp;
	}

	fields[nfields] = proto;
	initree();

	eargv = argv;
	tryfor = DEFMEM;

	/*
	 * scan arguments of sort command
	 */
	while (--argc > 0)
	{
		if (**++argv == '-')
		{
			/*
			 * option or field specification
			 */
			for (arg = *argv; ; )
			{
				switch (*++arg)
				{
				case '\0':	/* end of option list or lone - */
					if (arg[-1] == '-')
						eargv[eargc++] = "-";
					break;

				case 'o':	/* output file specified */
					if (*(arg + 1) != '\0')
						outfil = arg + 1;
					else if(--argc > 0)
						outfil = *++argv;
					else
						diag(EXIT, catgets(_m_catd, NL_SETN, 2, "can't identify output file"),"");
					break;

				case 'T':	/* location of temp files */
					if (--argc > 0)
					{
						if ((strlen(*++argv) + NAMEOHD) > MAXPLEN)
							diag(EXIT, catgets(_m_catd, NL_SETN, 3, "path name too long:"), *argv);
						else dirtry[0] = *argv;
					}
					break;

				case 'c':	/* only check whether sorted */
					cflg = 1;
					continue;

				case 'm':	/* merge only */
					mflg = 1;
					continue;

				case 'y':	/* memory to use for sort */
					if (*++arg)
					{
						if (isdigit(*arg))
							tryfor = number(&arg) * 1024;
						else
							diag(EXIT, USE,"");
					}
					else
					{
						--arg;
						tryfor = MAXMEM;
					}
					continue;

				case 'z':	/* max linelength for merge */
					if (*++arg && isdigit(*arg))
						maxrec = number(&arg);
					else
						diag(EXIT, USE,"");
					continue;

				case 'X':		/* do a TAG sort */
					tag = 1;
					compare = tagcmp;
					continue;

				default:	/* field specification */
					field(++*argv, nfields > 0);
					break;
				}
				break;
			}
		}
		else if (**argv == '+')
		{
			/*
			 * field specification
			 */
			if (++nfields >= NF)
				diag(EXIT, catgets(_m_catd, NL_SETN, 4, "too many keys"), "");
			fields[nfields] = proto;
			field(++*argv, 0);
		}
		else
		{
			/*
			 * file name to sort
			 */
			eargv[eargc++] = *argv;
		}
	}

	/*
	 * propagate the default field specification into all field
	 * variables where no fieldspecific bifndrM was given
	 */
	q = &fields[0];
	for (a = 1; a <= nfields; a++)
	{
		p = &fields[a];
		if (p->code != proto.code)
			continue;
		if (p->ignore != proto.ignore)
			continue;
		if (p->fcmp != proto.fcmp)
			continue;
		if (p->rflg != proto.rflg)
			continue;
		if (p->bflg[0] != proto.bflg[0])
			continue;
		if (p->bflg[1] != proto.bflg[1])
			continue;
		p->code = q->code;
		p->ignore = q->ignore;
		p->fcmp = q->fcmp;
		p->rflg = q->rflg;
		p->bflg[0] = p->bflg[1] = q->bflg[0];
	}

	/*
	 * if no args given sort stdin
	 */
	if (eargc == 0)
		eargv[eargc++] = "-";

	/*
	 * consistency check for check option
	 */
	if (cflg && eargc > 1)
		diag(EXIT, catgets(_m_catd, NL_SETN, 5, "can check only 1 file"), "");

	/*
	 * create a safe place to put our output
	 */
	safeoutfil();

	/*
	 * find a directory for temporary files
	 */
	gettmpdir();

	/*
	 * set up signal handling
	 */
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		(void) signal(SIGHUP, term);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, term);
	(void) signal(SIGPIPE, term);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		(void) signal(SIGTERM, term);

	nfiles = eargc;

	/*
	 * only check sort
	 */
	if (cflg)
		checksort();

	/*
	 * sort and/or merge
	 */
	if (!mflg)
	{
		sort();
		doclose(stdin, (char *)0);	/* ATT: may close stdin twice */
		a = eargc;
	}
	else
	{
		if (maxrec == 0)
			maxrec = 512;
		a = 0;
	}

	wasfirst = notfirst = 0;

	nway = safemem(maxrec);

	if ((i = nfiles - a) > nway)
	{	/* Do leftovers early */
		if ((i %= (nway - 1)) == 0)
			i = nway - 1;
		if (i != 1)
		{
			os = output((char *)0);
			merge(a, a+i, 0);
			a += i;
		}
	}

	for (; a + nway < nfiles || unsafeout && a < eargc; a = i)
	{
		i = a + nway;
		if (i >= nfiles)
			i = nfiles;
		os = output((char *)0);
		merge(a, i, 0);
	}

	if (a != nfiles)
	{
		os = output(outfil);
		merge(a, nfiles, 1);
	}

	/*
	 * That's all folks
	 */
	term(0);
	/*NOTREACHED*/
}

/*
 * checksort -- check whether file is sorted ok
 */
checksort()
{
	char *lines[2];		/* two lines to compare */
	FILE *fp;		/* file beeing read */
	register int i;		/* used to alternate line indices */
	register int j;		/* used to alternate line indices */
	register int r;		/* used to alternate line indices */

	/*
	 * open the input file to check
	 */
	fp = input(0, (char *)0);

	if (maxrec == 0)
		maxrec = 512;

	/*
	 * get two buffers that will be used in turns
	 */
	lines[0] = malloc(maxrec);
	lines[1] = malloc(maxrec);

	/*
	 * read first line
	 */
	if (getline(fp, lines[0], maxrec, 1, 0) == 0)
	{
		doclose(fp, setfil(0));
		exit(0);
	}

	/*
	 * read lines into alternating buffers
	 */
	for (i = 0, j = 1; getline(fp, lines[j], maxrec, 1, 0); r=i, i=j, j=r)
	{
		/*
		 * check whether they are sorted ok
		 */
		if ((r = (*compare)(lines[i], lines[j])) < 0)
			disorder(catgets(_m_catd, NL_SETN, 7, "disorder: "), lines[j]);
		if (r == 0 && uflg)
			disorder(catgets(_m_catd, NL_SETN, 8, "non-unique: "), lines[j]);
	}

	/*
	 * free used storage
	 */
	free(lines[0]);
	free(lines[1]);

	doclose(fp, setfil(0));
	exit(0);
}

/*
 * sort -- read the input files and sort them a coreload at a time into
 *	   temporary files that will be merged during the second phase
 *
 *	   Records are read in from the front of the buffer area.
 *	   Pointers to the records are allocated from the back of the buffer.
 *	   If a partially read record exhausts the buffer, it is saved and
 *	   then copied to the start of the buffer for processing with the
 *	   next coreload.
 *
 *	   +------------+-------------+--------+
 *	   | strings    |             |  ptr   |
 *	   +------------+-------------+--------+
 *	   ^         -> ^             ^ <-     ^
 *	   lspace       |             |        ep
 *	                cp            lp
 *
 *	   CAUTION:
 *	   --------
 *	   In the code segment marked below malloc may not be used, and this
 *	   includes all functions called!  Functions that use malloc for
 *	   temporary storage and return the memory before they return are ok.
 */
sort()
{
	register char *cp;
	register char **lp;	/* pointer to pointer to data read */
	FILE *iop;		/* pointer to file beeing read */
	char *keep = (char *)0;	/* pointer to start of buffer overflow area */
	char *ekeep = (char *)0;/* pointer to end of buffer overflow area */
	char *lspace;		/* low end of arena */
	unsigned alloc;		/* memory known to be available */
	char **ep;		/* pointer to current end of buffer */
	int n;
	int done = 0;		/* set when all input files have been read */
	int i = 0;		/* index of input file */
	int first = 1;		/* reading first file and memory still avail */
	char inbuf[BUFSIZ];	/* buffer for input file to not destroy arena */

	/*
	 * get memory
	 */
	if ((alloc = getmem(tryfor, (unsigned)0, &lspace)) == 0)
		diag(EXIT, catgets(_m_catd, NL_SETN, 9, "allocation error before sort"), "");

	/* !! START OF CODE SEGMENT WHERE MALLOC MAY NOT BE USED !! */

	ep = (char **)(lspace + alloc);

	iop = input(i++, inbuf);

	do
	{
		lp = ep - 1;
		cp = lspace;
		*lp-- = cp;

		/*
		 * move record from previous coreload
		 */
		if (keep != (char *)0) {
			recstart = cp;
			for(; keep < ekeep; *cp++ = *keep++);
		}

		/*
		 * test for negative n below only necessary for small machines
		 */
#ifdef SMALL
		while ((n  = (char *)(lp - 1) - cp) > 1 || n < 0)
		{
			if (n < 0)
				n = MAXINT;
#else
		while ((n  = (char *)(lp - 1) - cp) > 1)
		{
#endif
			if ((n = getline(iop, cp, n, 0, 0)) == 0)
			{
				/*
				 * EOF on input file
				 */
				doclose(iop, setfil(i - 1));

				if (keep != (char *)0)
					;
				else if (i < eargc)
				{
					/*
					 * open the next file
					 */
					iop = input(i++, inbuf);
					continue;
				}
				else
				{
					/*
					 * have seen all files
					 */
					done++;
					break;
				}
			}

			/*
			 * advance buffer pointer
			 */
			cp += n - 1;

			/*
			 * check whether complete line read
			 */
			if (getstate == OKAY)
			{
				cp += 2;
				if ((n = cp - *(lp + 1)) > maxrec)
					maxrec = n;
				/*
				 * if tagging force alignment on short boundary
				 */
				if (tag && ((int)cp & 0x1))
					cp++;
				*lp-- = cp;
				keep = (char *)0;
			}
			else
			{
				/*
				 * the buffer is full
				 */
				keep = *(lp + 1);
				ekeep = ++cp;
			}

			if (first && (n = (char *)lp - cp) <= (2 + sizeof(lp))
								      && n >= 0)
			{
				/*
				 * full buffer, try to get more memory
				 */
				tryfor = getmem(tryfor, alloc, &lspace);

				if (tryfor == 0)	/* out of mem */
					first = 0;
				else
				{
					register char **lmp;
					register char **mp;

					/*
					 * calculate where pointers are now
					 */
					lspace += memmoved;
					cp     += memmoved;
					lp     += memmoved/sizeof(char **);
					ep     += memmoved/sizeof(char **);
					keep   += memmoved;
					ekeep  += memmoved;
					/*
					 * move pointers from end of old buffer
					 * to end of new buffer
					 */
					lmp = ep + (tryfor/sizeof(char **) - 1);
					for (mp = ep - 1; mp > lp;)
						*lmp-- = *mp-- + memmoved;
					ep += tryfor/sizeof(char **);
					lp += tryfor/sizeof(char **);
					alloc += tryfor;
				}
			}
		}

		if (keep != (char *)0 && *(lp + 1) == lspace)
			diag(TERM, catgets(_m_catd, NL_SETN, 10, "fatal: record too large"),"");

		first = 0;
		lp += 2;

		/*
		 * open output file: if not seen all input yet divert to
		 * another temporary file else write to final destination.
		 */
		os = output((done == 0 || nfiles != eargc) ? (char *)0 : outfil);

		/*
		 * sort coreload and write it to file os
		 */
		msort(lp, ep, done && (nfiles == eargc));
		doclose(os, catgets(_m_catd, NL_SETN, 11,"sorting"));

	} while (done == 0);

	/* !! END OF CODE SEGMENT WHERE MALLOC MAY NOT BE USED !! */

	/*
	 * free meory used by sort
	 */
	free(lspace);
}

/*
 * msort -- sort a coreload of lines
 */
msort(a, b, done)
char **a;	/* address of pointer to first line in core */
char **b;	/* address of pointer to last line in core  */
int  done;	/* true if we are doing final output	    */
{
	register struct btree **tp;
	register int i, j, n;
	char *save;

	i = (b - a);

	if (i < 1)
		return;
	else if (i == 1)
	{
		/*
		 * one line sort - trivial
		 */
		(void)putline(*a, os, done);
		return;
	}
	else if (i >= TREEZ)
		n = TREEZ; /* number of blocks of records */
	else n = i;

	/*
	 * break into n sorted subgroups of approximately equal size
	 */
	tp = &(treep[0]);
	j = 0;
	do
	{
		(*tp++)->rn = j;
		b = a + (blkcnt[j] = i / n);
		qksort(a, b);
		blkcur[j] = a = b;
		i -= blkcnt[j++];
	} while (--n > 0);
	n = j;

	/*
	 * make a sorted binary tree using the first record in each group
	 */
	for (i = 0; i < n;)
	{
		(*--tp)->rp = *(--blkcur[--j]);
		insert(tp, ++i);
	}

	wasfirst = notfirst = 0;
	bonus = cmpsave(n);

	j = uflg;
	tp = &(treep[0]);
	while (n > 0)
	{
		(void)putline((*tp)->rp, os, done);

		if (j)
			save = (*tp)->rp;

		/*
		 * Get another record and insert. Bypass repeats if uflg
		 */
		do
		{
			i = (*tp)->rn;

			if (j)
				while((blkcnt[i] > 1) && (**(blkcur[i]-1) == '\0'))
				{
					--blkcnt[i];
					--blkcur[i];
				}

			if (--blkcnt[i] > 0)
			{
				(*tp)->rp = *(--blkcur[i]);
				insert(tp, n);
			}
			else
			{
				if (--n <= 0)
					break;
				bonus = cmpsave(n);
				tp++;
			}
		} while (j && (*compare)((*tp)->rp, save) == 0);
	}
}


/*
 * insert -- insert an element into sorted tree
 *
 * Insert the element at tp[0] into its proper place in the array of size n
 * Pretty much Algorith B from 6.2.1 of Knuth, Sorting and Searching
 * Special case for data that appears to be in correct order
 */
insert(tp, n)
struct btree **tp;
int n;
{
	register struct btree **lop;
	register struct btree **hip;
	register struct btree **midp;
	register int c;
	struct btree *hold;

	midp = lop = tp;
	hip = lop++ + (n - 1);

	if ((wasfirst > notfirst) && (n > 2) &&
	    ((*compare)((*tp)->rp, (*lop)->rp) >= 0))
	{
		wasfirst += bonus;
		return;
	}

	while ((c = hip - lop) >= 0)
	{
		/*
		 * leave midp at the one tp is in front of
		 */
		midp = lop + c / 2;
		if ((c = (*compare)((*tp)->rp, (*midp)->rp)) == 0)
			break;		/* match */
		if (c < 0)
			lop = ++midp;   /* c < 0 => tp > midp */
		else
			hip = midp - 1; /* c > 0 => tp < midp */
	}

	c = midp - tp;

	if (--c > 0)
	{
		/*
		 * number of moves to get tp just before midp
		 */
		hip = tp;
		lop = hip++;
		hold = *lop;
		do
			*lop++ = *hip++;
		while (--c > 0);
		*lop = hold;
		notfirst++;
	}
	else
		wasfirst += bonus;
}

/*
 * merge -- merge two already sorted files
 */
merge(a, b, final)
int a;
int b;
int final;		/* TRUE if writing final output */
{
	FILE *tfile[N];
	register int nf;		/* number of merge files */
	register struct btree **tp;
	register int i;
	register int j;
	char *save = malloc(maxrec);		/* buffer for one saved line */
	char *buffer = malloc(nway * maxrec);	/* buffer for files to merge */
	char *fbuffer = malloc(nway * BUFSIZ);	/* buffer or input */

	if (buffer == (char *)0 || save == (char *)0 || fbuffer == (char *)0)
		diag(TERM, catgets(_m_catd, NL_SETN, 12, "BUG: reallocation error in merge"), "");

	tp = &(treep[0]);
	for (nf = 0, i = a; i < b; i++)
	{
		tfile[nf] = input(i, &fbuffer[nf * BUFSIZ]);

		(*tp)->rp = &buffer[nf * maxrec];
		(*tp)->rn = nf;

		if (getline(tfile[nf], (*tp)->rp, maxrec, 1, tag))
		{
			nf++;
			tp++;
		}
		else
			doclose(tfile[nf], setfil(i));
	}

	/*
	 * make a sorted btree from the first record of each file
	 */
	for (--tp, i = 1; i++ < nf;)
		insert(--tp, i);

	bonus = cmpsave(nf);
	tp = &(treep[0]);
	j = uflg;
	while (nf > 0)
	{
		(void)putline((*tp)->rp, os, final);

		if (j)
			strcpy(save, (*tp)->rp);

		/*
		 * Get another record and insert. Bypass repeats if uflg
		 */
		do
		{
			i = (*tp)->rn;
			if (getline(tfile[i], (*tp)->rp, maxrec, 1, tag) == 0)
			{
				doclose(tfile[i], setfil(i + a));

				if (--nf <= 0)
					break;
				++tp;
				bonus = cmpsave(nf);
			}
			else
				insert(tp, nf);
		} while (j && (*compare)((*tp)->rp, save) == 0);
	}

	for (i = a; i < b; i++)
	{
		if (i >= eargc)
			(void) unlink(setfil(i));
	}

	doclose(os, catgets(_m_catd, NL_SETN, 13, "merging"));

	free(fbuffer);
	free(buffer);
	free(save);
}

/*======================== comparison routines ==============================*/

/*
 * cmpa -- compare two whole lines
 */
cmpa(pa, pb)
register unsigned char *pa;
register unsigned char *pb;
{
	while(*pa == *pb++)
		if(*pa++ == '\n')
			return(0);
	return(
		*pa == '\n' ? fields[0].rflg:
		*--pb == '\n' ?-fields[0].rflg:
		*pb > *pa   ? fields[0].rflg:
		-fields[0].rflg
	);
}

/*
 * cmp -- compare two lines using fields
 */
int
cmp(line1, line2)
char *line1;	/* pointer to a line */
char *line2;	/* pointer to the other line */
{
	int result;		/* to retain result of comparison */
	int fieldno;		/* index of field under consideration */
	char *sfield1;		/* start of field in line1 */
	char *efield1;		/* end of field in line1 */
	char *sfield2;		/* start of field in line2 */
	char *efield2;		/* end of field in line2 */

	for (fieldno = (nfields > 0); fieldno <= nfields; fieldno++)
	{
		register struct field *fp; /* pointer to field description */

		fp = &fields[fieldno];

		/*
		 * position to start and end of fields
		 */
		if (fieldno != 0)
		{
			efield1 = skip(line1, fp, 1);
			sfield1 = skip(line1, fp, 0);
			efield2 = skip(line2, fp, 1);
			sfield2 = skip(line2, fp, 0);
		}
		else
		{
			efield1 = eol(line1);
			sfield1 = line1;
			efield2 = eol(line2);
			sfield2 = line2;
		}

		/*
		 * Do the comparison for the field that starts including
		 * sfield[12] and ends including efield[12] in lines[12].
		 */
		if (result=(*fp->fcmp)(fp, sfield1, efield1, sfield2, efield2))
			return fp->rflg * result;
	}

	/*
	 * up to now all fields have compared equal, try to compare the
	 * whole line to break the tie if not discarding duplicates anyway
	 */
	if (uflg)
		return(0);

	return(cmpa(line1, line2));
}

/*
 * tagcmp -- compare two tagged records.
 */

tagcmp(r1, r2)
register char *r1, *r2;
{	int result;
	register struct field *fp;
	int fieldno;
	int offset;		/* offset of record pointer */

	for (fieldno = (nfields > 0);  fieldno <= nfields; fieldno++) {
		fp = &fields[fieldno];
		offset = fieldno + 1 - (nfields > 0);
		
		if (result = strcmp(r1 + ((short *)r1)[offset], r2 + ((short *)r2)[offset]))
			return fp->rflg * -result;
	}
	return 0;
}

#define qsexc(p,q)	t = *p; *p = *q; *q = t
#define qstexc(p,q,r)	t = *p; *p = *r; *r = *q; *q = t

qksort(a, l)
char **a;
char **l;
{
	register char **i;
	register char **j;
	register char **lp;
	register char **hp;
	char **k;
	int c;
	int delta;
	char *t;
	unsigned n;


start:
	if ((n = l - a) <= 1)
		return;

	n /= 2;

	if (n >= MTHRESH)
	{
		lp = a + n;
		i = lp - 1;
		j = lp + 1;
		delta = 0;

		c = (*compare)(*lp, *i);
		if (c < 0)
			--delta;
		else if (c > 0)
			++delta;

		c = (*compare)(*lp, *j);
		if (c < 0)
			--delta;
		else if (c > 0)
			++delta;

		if ((delta /= 2) && (c = (*compare)(*i, *j)))
			if (c > 0)
				n -= delta;
			else
				n += delta;
	}

	hp = lp = a + n;
	i = a;
	j = l - 1;

	for (;;)
	{
		if (i < lp)
		{
			if ((c = (*compare)(*i, *lp)) == 0)
			{
				--lp;
				qsexc(i, lp);
				continue;
			}

			if (c < 0)
			{
				++i;
				continue;
			}
		}

loop:
		if (j > hp)
		{
			if ((c = (*compare)(*hp, *j)) == 0)
			{
				++hp;
				qsexc(hp, j);
				goto loop;
			}

			if (c > 0)
			{
				if (i == lp)
				{
					++hp;
					qstexc(i, hp, j);
					i = ++lp;
					goto loop;
				}
				qsexc(i, j);
				--j;
				++i;
				continue;
			}
			--j;
			goto loop;
		}

		if (i == lp)
		{
			if (uflg)
				for (k = lp; k < hp;)
					**k++ = '\0';
			if (lp - a >= l - hp)
			{
				qksort(hp + 1, l);
				l = lp;
			}
			else
			{
				qksort(a, lp);
				a = hp + 1;
			}
			goto start;
		}

		--lp;
		qstexc(j, lp, i);
		j = --hp;
	}
}

/*===================== help functions for comparisons ======================*/

/*
 * skip -- skip in a record according to field specifications
 */
char *
skip(p, fp, j)
register char *p;	/* line to work on */
struct field *fp;	/* field to isolate */
int j;			/* 1 means get the end of the field, 0 means start */
{
	register i;
	register char tbc;

	if ((i = fp->m[j]) < 0)		/* can only happen for j = 1! */
		return(eol(p));

	/*
	 * get to correct field, handling tabchar or spaces
	 */
	if ((tbc = tabchar) != 0)
		while (--i >= 0)
		{
			while (*p != tbc)
				if (*p != '\n')
					p++;
				else
					goto ret;

			if (i > 0 || j == 0)
				p++;
		}
	else
		while (--i >= 0)
		{
			while (blank(*p))
				p++;
			while (!blank(*p))
				if (*p != '\n')
					p++;
				else
					goto ret;
		}

	/*
	 * skip blanks
	 */
	if (fp->bflg[j])
	{
		if (j == 1 && fp->m[j] > 0)
			p++;

		while (blank(*p))
			p++;
	}

	/*
	 * get to byte position within field
	 */
	i = fp->n[j];

	while((i-- > 0) && (*p != '\n'))
		p++;

ret:
	return(p);
}

/* 
 * settag -- setup the tag fields for a record
 */

char *
settag(tp, start, end, amount)
short *tp;		/* pointer to tag area */
char  *start;		/* pointer to record area */
char  *end;		
int    amount;	
{	char *save = (char *)tp;
	int fieldno;
	char charsave;
	char *ep;
	struct field *fp;
	int len;

	for (fieldno = (nfields > 0) ; fieldno <= nfields; fieldno++) {
		fp = &fields[fieldno];
		/*
		 * set offset to start of field in pointer area
		 */
		*end++ = '\0';
		amount--;
		*++tp  = (short)(end - save);

		ep     = skip(start, fp, 1);
		/*
		 * nul terminate for strxfrm
		 */
		charsave = *ep;		
		*ep      = '\0';

		if ((len = strxfrm(end, skip(start, fp, 0), amount)) > amount) {
			*ep = charsave;
			return (char *)0;
		}

		/*
		 * restore character and bump pointers
		 */
		*ep     = charsave;
		end    += len;
		amount -= len;
	}
	if (amount > 0)
		*end++ = '\0';
	else
		return (char *)0;
	/*
	 * first field is the length of the entire record 
	 */
	*(short *)save = end - save;	

	return end;
}

/*
 * cmpsave -- save result of comparison for later use
 */
cmpsave(n)
register int n;
{
	register int award;

	if (n < 2)
		return (0);

	for (n++, award = 0; (n >>= 1) > 0; award++)
	;

	return (award);
}

/*
 * asciicmp -- compare ascii strings
 */
int
asciicmp(fp, s1, e1, s2, e2)
struct field *fp;		/* pointer to field description */
register char *s1;		/* beginning of first field */
char *e1;			/* end of first field */
register char *s2;		/* beginning of second field */
char *e2;			/* end of second field */
{
	register char *ignore;	/* ptr to ignore table */
	register char *code;	/* ptr to code conversion */
	int a;

	code = fp->code;
	ignore = fp->ignore;

	for (;;)
	{
		/*
		 * skip ignore characters
		 */
		while (ignore[*s1])
			s1++;
		while (ignore[*s2])
			s2++;

		if (s1 >= e1 || *s1 == '\n')
			if (s2 < e2 && *s2 != '\n')
				return 1;
			else
				break;

		if (s2 >= e2 || *s2 == '\n')
			return -1;

		if (a = ((int)code[*s2++] & 0xFF) - ((int)code[*s1++] & 0xFF))
			return a;
	}

	/*
	 * the two fields compare equal
	 */
	return 0;
}

/*
 * intlcmp -- international collation
 */
int
intlcmp(fp, s1, e1, s2, e2)
struct field *fp;		/* pointer to field description */
char *s1;			/* beginning of first field */
char *e1;			/* end of first field */
char *s2;			/* beginning of second field */
char *e2;			/* end of second field */
{
	int result;		/* result of collation */
	char save1, save2;	/* save characters to allow nul terminarion */

	/*
	 * "ignore" is specified in the collation itself
	 * NOT_YET: "fold" is done by converting codes to upper case
	 */

	/*
	 * as e1/e2 are to be compared too:
	 */
	save1 = *++e1;
	save2 = *++e2;
	*e1 = *e2 = '\0';

	result = strcoll(s2, s1);

	*e1 = save1;
	*e2 = save2;

	return result;
}

/*
 * numcmp -- numerical comparison of two fields
 *	bytewise comparison of two numbers of the
 *	form [-][0-9]*\.[0-9]*
 */
int
numcmp(fp, s1, e1, s2, e2)
struct field *fp;		/* pointer to field description */
char *s1;			/* beginning of first field */
char *e1;			/* end of first field */
char *s2;			/* beginning of second field */
char *e2;			/* end of second field */
{
	static char radix;	/* radix char to use, initially zero */
	int sa;			/* sign of number in first line */
	int sb;			/* sign of number in second line */
	register char *ipa;	/* tmp ptr into first number */
	register char *ipb;	/* tmp ptr into second number */
	char *jpa;		/* save tmp ptr into first number */
	char *jpb;		/* save tmp ptr into second number */
	int result = 0;		/* result of comparison */

	/*
	 * set radix character
	 */
	if (radix == '\0')
	{
		if ((radix = *nl_langinfo(RADIXCHAR)) == '\0')
			radix = '.';
	}

	/*
	 * evaluate sign
	 */
	sa = sb = 1;
	if (*s1 == '-')
	{
		s1++;
		sa = -sa;
	}

	if (*s2 == '-')
	{
		s2++;
		sb = -sb;
	}

	/*
	 * skip to end of number before the decimal point
	 */
	for (ipa = s1; ipa < e1 && isdigit(*ipa); ipa++)
	;

	for (ipb = s2; ipb < e2 && isdigit(*ipb); ipb++)
	;

	/*
	 * save pointers for scan after decimal point
	 */
	jpa = ipa;
	jpb = ipb;

	if (sa == sb)
		while (ipa > s1 && ipb > s2)
			if (*--ipb - *--ipa != 0)
				result = *ipb - *ipa;

	while (ipa > s1)
		if (*--ipa != '0')
			return(-sa);
	while (ipb > s2)
		if (*--ipb != '0')
			return(sb);

	if (result)
		return (result * sa);

	if (*(s1 = jpa) == radix)
		s1++;

	if (*(s2 = jpb) == radix)
		s2++;

	if (sa == sb)
		while (s1 < e1 && isdigit(*s1)
		   && s2 < e2 && isdigit(*s2))
			if ((result = *s2++ - *s1++) != 0)
				return(result*sa);

	while (s1 < e1 && isdigit(*s1))
		if (*s1++ != '0')
			return(-sa);

	while (s2 < e2 && isdigit(*s2))
		if (*s2++ != '0')
			return(sb);

	return 0;
}

int
monthcmp(fp, s1, e1, s2, e2)
struct field *fp;		/* pointer to field description */
char *s1;			/* beginning of first field */
char *e1;			/* end of first field */
char *s2;			/* beginning of second field */
char *e2;			/* end of second field */
{
	return (month(s1) - month(s2));
}

/*
 * month -- return numerical match value for a month
 */
month(s)
char *s;
{
	static char *months[] = { "jan", "feb", "mar", "apr", "may", "jun",
				  "jul", "aug", "sep", "oct", "nov", "dec" };
	register char *t;
	register char *u;
	register i;
	register char *f = fold + 128;

	for (i = 0; i < sizeof(months) / sizeof(*months); i++)
	{
		for (t = s, u = months[i]; f[*t++] == f[*u++]; )
			if (*u == 0)
				return(i);
	}
	return(-1);
}

/*====================== file handling routines =============================*/

/*
 * setfil -- return pointer to the name of next input/output file
 *
 * ATTENTION: The algorithm used here will cause problems for NFILES > 26!
 */
char *
setfil(i)
register int i;
{
	static char *filep;

	if (filep == (char *)0)
		filep = &file[strlen(file) - 2];

	if (i < eargc)
		if (eargv[i][0] == '-' && eargv[i][1] == '\0')
			return (char *)0;
		else
			return eargv[i];
	i -= eargc;

	filep[0] = i/26 + 'a';
	filep[1] = i%26 + 'a';

	return file;
}

/*
 * output -- open an output file. If the argument passed is not the null
 *	     pointer, it is assumed that a temporary file is meant.
 */
FILE *
output(filename)
char *filename;
{
	static char outbuf[BUFSIZ];	/* buffer to use for stdio */
	FILE *fp;

	if (filename == (char *)0)
	{
		filename = setfil(nfiles);
		if ((fp = fopen(filename, "w")) == (FILE *)0)
			diag(TERM, catgets(_m_catd, NL_SETN, 15, "can't create"), filename);
		nfiles++;
	}
	else if (!strcmp(filename, "-"))
		fp = stdout;
	else if ((fp = fopen(filename, "w")) == (FILE *)0)
		diag(TERM, catgets(_m_catd, NL_SETN, 16, "can't create"), outfil);

	setbuf(fp, outbuf);
	return fp;
}

/*
 * input -- open a file for input
 */
FILE *
input(i, bp)
int i;
char *bp;
{
	FILE *fp;
	char *name;

	if ((name = setfil(i)) == (char *)0)
		fp = stdin;
	else if ((fp = fopen(name, "r")) == NULL)
		diag(TERM, catgets(_m_catd, NL_SETN, 17, "can't open input:"), name);

	if (bp != (char *)0)
		setbuf(fp, bp);
	return fp;
}

/*
 * doclose -- close an input/output file
 */
void
doclose(fp, msg)
FILE *fp;
char *msg;
{
	if (ferror(fp))
	{
		if (fp == os)
			diag(TERM, catgets(_m_catd, NL_SETN, 18, "write error while"), msg);
		else
			diag(TERM, catgets(_m_catd, NL_SETN, 19, "read error on"), msg ? msg : catgets(_m_catd, NL_SETN, 20, "stdin"));
	}
	(void) fclose(fp);
}

/*
 * gettmpdir -- find a directory for the temp files
 */
gettmpdir()
{
	int a = -1;
	char **dirs;
	static char file1[MAXPLEN];

	for (dirs = dirtry; *dirs; dirs++)
	{
		(void)sprintf(file = file1, "%s/stm%.5uaa", *dirs, getpid());

		/*
		 * try to create a temporary temporary file
		 */
		if ((a = creat(file, 0600)) >= 0)
			break;
	}

	if (a < 0)
		diag(EXIT, catgets(_m_catd, NL_SETN, 21, "can't locate temp"), "");

	/*
	 * remove temporary temporary file
	 */
	(void) close(a);
	(void) unlink(file);
}

/*
 * getline -- get a line from a file and return amount read
 */
int
getline(iop, s, amount, complete, tagged)
FILE *iop;			/* file to read from */
register char *s;		/* buffer to read into */
int amount;			/* max amount to read */
int complete;			/* TRUE if complete line has to be read */
int tagged;			/* TRUE if file is tagged */
{
	char *fgets();
	char *saves = s;
	int offset = 0;
	int bias   = 0;
	short len;

	if (tag) {
	    	if (tagged) {
			/* 
			 * we are reading in a line which was written with tags
			 * simply pull it back in
			 */
			if (fread((char *)&len, sizeof(short), 1, iop) == 0)
				return 0;
			if (amount < len)
				diag(TERM, catgets(_m_catd, NL_SETN, 22, "getline tag read"), catgets(_m_catd, NL_SETN, 23, "too short"));
			*(short *)s = len;
			s += sizeof(short);
			if (fread(s, 1, (unsigned)len - sizeof(short), iop) != (int)len - sizeof(short))
				diag(TERM, catgets(_m_catd, NL_SETN, 24, "getline tag read"), catgets(_m_catd, NL_SETN, 25, "short record"));
			return (int)len;
		}
		offset = (nfields + 2) * sizeof(short);
		/*
		 * if previous getline failed restart 
		 */
		if (getstate != OKAY) {		
			saves = recstart + memmoved;
			bias =  s - saves;
			memmoved = 0L;

			switch (getstate) {
			case INHEAD:
				s = saves;
				break;
			case INREAD:
				if (fgets(s, amount, iop) == (char *)0)
					*s = '\0';
				goto do_tag;
				/* NOT REACHED */
			case INTAG:
				s = saves + offset;
				goto do_tag;
				/* NOT REACHED */
			}
		}
		/* 
		 * save start of record for possible restart 
		 */
		recstart = s;
		if (amount - offset + bias < 1) {
			getstate = INHEAD;
			return amount - 1;
		}
	}
	
	if (amount < 2)
		diag(TERM, catgets(_m_catd, NL_SETN, 26, "BUG: getline"), catgets(_m_catd, NL_SETN, 27, "amount too small"));

	s += offset;
	if (fgets(s, amount - offset, iop) == (char *)0) {
		getstate = OKAY;
		return 0;
	}
do_tag:
	while (*s)
		s++;

	if (s[-1] != '\n')
	{
		if (s - saves < amount - 1)
		{
			diag(WARN, catgets(_m_catd, NL_SETN, 28, "warning: missing NEWLINE added:"), saves);
			*s++ = '\n';
			*s = '\0';
		}
		else if (complete)
			diag(TERM, catgets(_m_catd, NL_SETN, 29, "fatal: line too long:"), saves);
		else {
		     	getstate = INREAD;
			return amount - 1;
		}
	}
	if (tag)
		if ((s = settag(saves, saves + offset, s, amount - (s - saves))) == (char *)0) {
			getstate = INTAG;
			return amount - 1;
		}
	getstate = OKAY;
	return s - saves - bias;
}

/*
 * putline -- output a line, ignoring tags if final output file
 */

putline(buf, iop, cleartag)
char *buf;
FILE *iop;
int cleartag;
{ 	short len;

	if (tag) {
		if (cleartag) 
			fputs(buf + (nfields + 2) * sizeof(short), iop);
		else {
			len = *(short *)buf;
			fwrite(buf, 1, (unsigned)len, iop);
		}
	} else
		fputs(buf, iop);
}

/*====================== memory management funcs ============================*/

#define BLOCK 256
#define round(what) (((what + BLOCK - 1) / BLOCK) * BLOCK)
#define MAXMALLOC ((1L << (BITSPERBYTE * sizeof(unsigned))) - BLOCK)
/*
 * getmem -- try to get memory and return amount allocated
 *
 *	This function is quite critical. It may be changed to care for
 *	different implementations of malloc/realloc (BE CAREFUL!)
 *	At least one implementation of malloc/realloc has problems when
 *	the amount to allocate is close to MAXUNSIGNED. Special care is
 *	taken to avoid overflow problems!
 */
unsigned
getmem(amount, got, whereto)
register unsigned amount;	/* how much memory we want  */
unsigned got;			/* memory we already have */
char **whereto;			/* where we want the memory */
{
	static int nomoremem = 0;	/* 1 when no more mem available */
	unsigned saveamount;		/* what we really want to have */
	register char *cp;		/* where we got it */

	/*
	 * if no more mem no use to try getting some
	 */
	if (nomoremem && got != 0)
		return (unsigned)0;

	amount = round(amount);

	if (got != 0)
	{
#ifdef M80S30
		long newsize = (long)got + (long)amount;
#else
		unsigned long newsize = (long)got + (long)amount;
#endif

		if (newsize > MAXMALLOC)
		{
			amount = MAXMALLOC - got;
			newsize = (long)got + (long)amount;
		}

		if (newsize > MAXMEM)
			amount = (unsigned)(MAXMEM - got);

		saveamount = amount;

		/*
		 * The following code assumes that, even when realloc returns
		 * (char *)0, *whereto still is valid!
		 */
		while (amount != 0 && (cp = realloc(*whereto, got + amount)) == (char *)0)
			amount -= BLOCK;

		if (amount == 0)
			cp = realloc(*whereto, got);

		memmoved = cp - *whereto;
	}
	else
	{
		nomoremem = 0;

		if (amount > MAXMALLOC)
			amount = (unsigned)MAXMALLOC;

		if (amount < MINMEM)
			amount = (unsigned)MINMEM;
		else if (amount > MAXMEM)
			amount = (unsigned)MAXMEM;

		saveamount = amount;

		while (amount != 0 && (cp = malloc(amount)) == (char *)0)
			amount -= BLOCK;

		*whereto = cp;
	}

	if (amount != saveamount)
		nomoremem = 1;

	return amount;
}

/*
 * safemem -- determine how many files can be merged at a time
 *	and make sure we have enough mem available.
 *	ATTENTION: Could be more cautious: no overflowcheck for malloc
 */
int
safemem(reclen)
{
	int nway;
	char *save;		/* memory we need for save area in merge */
	char *bufs;		/* memory we need for line buffers */
	char *fbufs;		/* memory we need for filebuffers */

	save = malloc(reclen);

	for (nway = N; nway >= 2; nway--)
	{
		bufs = malloc(nway * reclen);
		fbufs = malloc(nway * BUFSIZ);
		if (fbufs == (char *)0)
			if (bufs == (char *)0)
				continue;
			else
				free(bufs);
		else if (bufs != (char *)0)
			break;
		else
			free(fbufs);
	}

	if (nway < 2 || save == (char *)0)
		diag(TERM, catgets(_m_catd, NL_SETN, 30, "allocation error before merge"), "");

	free(fbufs);
	free(bufs);
	free(save);

	return nway;
}

/*====================== general support funcs ==============================*/

/*
 * number -- convert number in string to int, advance string pointer
 */
int
number(ppa)
char **ppa;
{
	register int n = 0;	/* result accumulator */
	register char *cp;	/* temporary pointer to scan string */

	for (cp = *ppa; isdigit(*cp); cp++) {
		n = n * 10 + *cp - '0';
		*ppa = cp;
	}
	return n;
}

/*
 * field -- analyse a field specification
 *
 * ATTENTION: Some options require the treatment of a line as one field
 *	(use of cmp instead of cmpa). These options are handled here too.
 */
field(s, k)
char *s;	/* pointer to string to analyse */
int k;		/* 0 for +pos or global option, 1 for -pos */
{
	register struct field *p;	/* ptr to current field */
	register d;			/* index add value */

	p = &fields[nfields];
	d = 0;

	for(; *s != 0; s++)
	{
		switch (*s)
		{
		case '\0':		/* end of field spec */
			return;

		case 'b':		/* ignore blanks in field */
			p->bflg[k]++;
			break;

		case 'd':		/* dictionary order for field */
			p->ignore = dict+128;
			break;

		case 'f':		/* fold cases in field */
			p->code = fold+128;
			break;

		case 'i':		/* ignore nonprinting chars in field */
			p->ignore = nonprint+128;
			break;

		case 'M':		/* use month comparison in field */
			p->fcmp = monthcmp;
			p->bflg[0]++;	/* implies skip of leading blanks! */
			break;

		case 'n':		/* numeric comparison in field */
			p->fcmp = numcmp;
			p->bflg[0]++;	/* implies skip of leading blanks! */
			break;

		case 'r':		/* reverse sense of comparison */
			p->rflg = -1;
			continue;

		case '.':		/* handle field spec */
			if (p->m[k] == -1)	/* -m.n with m missing */
				p->m[k] = 0;

			d = &fields[0].n[0]-&fields[0].m[0];

			if (*++s == '\0')
			{
				--s;
				p->m[k+d] = 0;
				continue;
			}

		default:
			if (isdigit(*s))
				p->m[k+d] = number(&s);
			else
				diag(EXIT, USE, "");
			break;

		/*
		 * The following two here because they require us
		 * to treat the line as one field (cmp instead of cmpa)
		 */
		case 't':		/* tabchar specified */
			tabchar = *++s;
			if (tabchar == 0)
				s--;
			continue;

		case 'u':		/* eliminate double keys */
			uflg = 1;
			continue;
		}

		/*
		 * for all of these we need to use the cmp function instead
		 * of cmpa, but only if not using tagcmp!
		 */
		if (compare != tagcmp)
			compare = cmp;

		return;
	}
}

/*
 * initree -- initialize tree
 */
initree()
{
	register struct btree **tpp;
	register struct btree *tp;
	register int i;

	for (tp = &(tree[0]), tpp = &(treep[0]), i = TREEZ; --i >= 0;)
	    *tpp++ = tp++;
}

/*
 * eol -- advance pointer to newline in string
 */
char *
eol(p)
register char *p;
{
	while (*p != '\n')
		p++;
	return(p);
}

/*
 * safeoutfil -- assure a safe output file
 */
safeoutfil()
{
	register int i;
	struct stat ostat;
	struct stat istat;

	if (!mflg || outfil == 0)
		return;
	if (stat(outfil, &ostat) < 0)
		return;
	if ((i = eargc - N) < 0)
		i = 0;	/* -N is sufficient, not necessary */
	for (; i < eargc; i++)
	{
		if (stat(eargv[i], &istat) < 0)
			continue;
		if (ostat.st_dev == istat.st_dev && ostat.st_ino == istat.st_ino)
			unsafeout++;
	}
}

/*
 * diag -- output a diagnostic message to stderr
 */
diag(type, s, t)
int type;
char *s;
char *t;
{
	fprintf(stderr, "sort: %s %s\n", s, t);

	if (type == TERM)
		term(1);
	else if (type == EXIT)
		exit(1);
}

/*
 * term -- terminate sort, either because of signal or normally
 */
void
term(exitcode)
int exitcode;
{
	register i;

	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);

	if (nfiles == eargc)
		nfiles++;

	/*
	 * remove all temporary files (in case of signal termination)
	 */
	for (i = eargc; i <= nfiles; i++)
		(void) unlink(setfil(i));

	/*
	 * all exitcodes are mapped to the exit codes 0 and 1. This is done
	 * because we want to remain compatible with SYS V sort even when
	 * terminated because of signal.
	 */
	exit(exitcode != 0);
}

/*
 * disorder -- issue disordered message and exit
 */
void
disorder(s, t)
char *s;
char *t;
{
	register char *u;

	/*
	 * make t a null terminated string
	 */
	for (u = t; *u != '\n'; u++)
	;
	*u = 0;

	diag(TERM, s, t);
}
