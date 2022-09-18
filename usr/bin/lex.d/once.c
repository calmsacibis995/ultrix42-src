/*	@(#)once.c	1.2		11/7/83 */
	/* because of external definitions, this code should occur only once */

/************************************************************************
 *									*
 *			Copyright (c) 1983 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
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

/************************************************************************
 *
 *			Modification History
 *
 *	Stephen Reilly,
 *
 ***********************************************************************/

# if NCH == 128			/* 7 bit ACSII */
int ctable[2*NCH] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
100,101,102,103,104,105,106,107,108,109,
110,111,112,113,114,115,116,117,118,119,
120,121,122,123,124,125,126,127};
# endif
# ifdef NCH == 256		/* 8 bit */
int ctable[2*NCH] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
100,101,102,103,104,105,106,107,108,109,
110,111,112,113,114,115,116,117,118,119,
120,121,122,123,124,125,126,127,128,129,
130,131,132,133,134,135,136,137,138,139,
140,141,142,143,144,145,146,147,148,149,
150,151,152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,168,169,
170,171,172,173,174,175,176,177,178,179,
180,181,182,183,184,185,186,187,188,189,
190,191,192,193,194,195,196,197,198,199,
200,201,202,203,204,205,206,207,208,209,
210,211,212,213,214,215,216,217,218,219,
220,221,222,223,224,225,226,227,228,229,
230,231,232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,247,248,249,
250,251,252,253,254,255};
# endif
int ZCH = NCH;
int ZCHHALF = NCH/2;  /* need to increase output table size for 8 bit clean */
FILE *fout = NULL, *errorf = {stdout};
int sect = DEFSECTION;
int prev = '\n';	/* previous input character */
int pres = '\n';	/* present input character */
int peek = '\n';	/* next input character */
unsigned char *pushptr = pushc;
unsigned char *slptr = slist;

# if (unix || ibm)

/*
 *				slr001
 *	No longer need these data structures 
 *
 * char *cname = "/usr/lib/lex/ncform";
 * char *ratname = "/usr/lib/lex/nrform";
 */

# endif

# ifdef gcos
unsigned char *cname = "pounce/lexcform";
unsigned char *ratname = "pounce/lexrform";
# endif
int ccount = 1;
int casecount = 1;
int aptr = 1;
int nstates = NSTATES, maxpos = MAXPOS;
int treesize = TREESIZE, ntrans = NTRANS;
int yytop;
int outsize = NOUTPUT;
int sptr = 1;
int optim = TRUE;
int report = 2;
int debug;		/* 1 = on */
int charc;
int sargc;
char **sargv;
unsigned char buf[520];
int ratfor;		/* 1 = ratfor, 0 = C */
int yyline;		/* line number of file */
int eof;
int lgatflg;
int divflg;
int funcflag;
int pflag;
int chset;	/* 1 = char set modified */
FILE *fin, *fother;
int fptr;
int *name;
int *left;
int *right;
int *parent;
unsigned char *nullstr;
int tptr;
unsigned char pushc[TOKENSIZE];
unsigned char slist[STARTSIZE];
unsigned char **def, **subs, *dchar;
unsigned char **sname, *schar;
unsigned char *ccl;
unsigned char *ccptr;
unsigned char *dp, *sp;
int dptr;
unsigned char *bptr;		/* store input position */
unsigned char *tmpstat;
int count;
int **foll;
int *nxtpos;
int *positions;
int *gotof;
int *nexts;
unsigned char *nchar;
int **state;
int *sfall;		/* fallback state num */
unsigned char *cpackflg;		/* true if state has been character packed */
int *atable;
int nptr;
unsigned char symbol[NCH];
unsigned char cindex[NCH];
int xstate;
int stnum;
unsigned char match[NCH];	/* 8 bit */
unsigned char extra[NACTIONS];
unsigned char *pchar, *pcptr;
int pchlen = TOKENSIZE;
 long rcount;
int *verify, *advance, *stoff;
int scon;
unsigned char *psave;
int buserr(), segviol();
