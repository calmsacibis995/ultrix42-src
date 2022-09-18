#ifndef lint
static	char	*sccsid = "@(#)sc.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/


/*
 * File: 	sc.c
 *
 * Pascal to C languge translator - scanner
 */

/*	Modification History: sc.c
 *
 * 21-July-87 afd
 *	If line longer than 132 chars, exit.
 *	We used to try to skip over rest of line (after 132) but we were
 *	   accidently reading stdin not file pointer!  When reading from file
 *	   you don't get ';' at end of line and soon get a syntax error.
 *
 */

#include <stdio.h>
#include "ptoc.h"

#ifdef SCDEBUG
int scdebug = 1;
#define printd if (prdebug) fprintf
#define printd10 if (prdebug >= 10) fprintf
#endif


extern enum token
    restoken[NRESERVES+1],		/* reserved words */
    chartoken[127+1],			/* legal one char tokens */
    nexttoken;				/* next Pascal token obtained as input */

extern char tokenahead;			/* 'nexttoken' ahead flag */
extern int linecounter;
extern struct scaninfo scandata;
extern int linesize;				/* # of chars in inputline */
extern char ahead;				/* got 'nextchar' ahead flag */
extern char endofinput;
extern int charcounter;
extern int savecmt;			/* True when comment saved */
extern FILE *fp;			/* file to scan from */
extern int doincl;			/* > 0 when processing include file */
struct cmtinfo *getcmtinfo();
char *getname();
int curley;				/* true if '{' found */

/* scanner data structures */

int resindex[SYMBOLMAX+1];		/* index into resword for [n]
					   char Pascal reserved words */
symbol resword[NRESERVES+1];		/* Pascal reserved words */
int resnext[NRESERVES+1];		/* link to next reserved word of [n] */

/* local variables */

char nextchar;				/* next char from inputline */
line inputline;				/* next whole input line */


/***************************************************************************/


/*
 * Get a new input line from the source file.  It is read into the
 * global variable "inputline".
 */

getline()
{
    int i;
    int c;		/* input char */

    for (i=0; i < LINELENGTH-1 && (c = getc(fp)) != EOF && c != '\n'; i++)
	inputline[i] = c;
    inputline[i] = ' ';
    linesize = i;
    if (c == EOF)
	endofinput = 1;
    else
	if (c != '\n')
	    myexit(6,"");
    linecounter++;
}


/*
 * Gets the next char from the current input line and puts it in the
 * global var nextchar.
 */

getnext()
{
    if (endofinput)
	nextchar = ' ';
    else
	{
	if (charcounter >= linesize)
	    {
	    getline();
	    if (endofinput)
		nextchar = ' ';
	    else
		{
		charcounter = 0;
		nextchar = inputline[charcounter];
		}
	    }
	else
	    {
	    charcounter++;
	    nextchar = inputline[charcounter];
	    }
	}
}


/*
 * Build up the next token.  It calls getnext when it needs another char.
 */

scanner(noreal)
    char noreal;	/* Real numbers not allowed when getting array or
			 * subrange declaration.  This is to avoid
			 * confussion with "[1." and the start of a real number
			 */
{
    char found;
    if (tokenahead)
	{
	tokenahead = 0;
	return;
	}
    curley = 0;
    do
	{
	if (ahead)
	    ahead = 0;
	else
	    getnext();
	found = 1;
	if (endofinput)
	    nexttoken = ILLEGALTOKE;
	else
	    switch (nextchar) {
	    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
	    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
	    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
	    case 'y': case 'z':
	    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
	    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
	    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
	    case 'Y': case 'Z':
	    case '$':				/* for VMS/Pascal */
		word();
#		ifdef SCDEBUG
		printd(stderr,"scanner: got word %s\n", scandata.si_name);
#		endif
		break;

	    case '0': case '1': case '2': case '3': case '4': case '5':
	    case '6': case '7': case '8': case '9':
		number(noreal);
#		ifdef SCDEBUG
		printd(stderr,"scanner: got word %s\n", scandata.st_cval);
#		endif
		break;
/* FIX - make this more efficient, by looping here to skip over blanks &
tabs */
	    case ' ':
	    case '\t':
		found = 0;
		getnext();
		ahead = 1;
		break;
	    case '{':
		curley = 1;
		getcomment();    
		break;
	    case '(':
		getnext();
		if (nextchar == '*')
		    getcomment();    
		else
		    {
		    nexttoken = LEFTPAREN;
		    ahead = 1;
		    }
		break;
	    case '\'':
		charconst();
		break;
	    case '"':
		strconst();
		break;
	    case '.':
		dot();
		break;
	    case ':':
		colon();
		break;
	    case '<': case '>':  case '=':
		relation();
		break;
	    default:
		nexttoken = chartoken[nextchar];
#		ifdef SCDEBUG
		printd(stderr,"scanner: got char token %c\n", nextchar);
#		endif
	    }   /* switch */
	}
    while (!found);
}

		
/*
 * Comment scan.
 * If "savecmt" is set then store the comment in a
 *   linked list of structures.  Each structure contains one line of
 *   the comment.
 * Else, scan & ignore the comment.
 */

getcomment()
{
    char gotast = 0;		/* set true when '*' scanned */
    int i;			/* loop index */
    int linenumber;		/* last linenumber */
    char first = 1;
    struct cmtinfo *headptr = NULL;
    struct cmtinfo *ptr;

    /*
     * Don't want to save any comments if processing an include file
     */
    if (doincl)
	savecmt = 0;
    ptr = getcmtinfo();
    headptr = ptr;
    linenumber = linecounter;
    for (i=0;; i++)
	{
	getnext();
	if (savecmt && linenumber != linecounter)
	    {
	    ptr->cmt = getname(i);
	    strncpy(ptr->cmt, scandata.si_name,i);
	    strcat(ptr->cmt, "\0");
	    linenumber = linecounter;
	    i = 0;
	    ptr->next = getcmtinfo();
	    ptr = ptr->next;
	    }
	if (curley == 1)
	    {
	    if (nextchar == '}')
		break;
	    }
	else
	    if (gotast == 1 && nextchar == ')')
		break;
	if (savecmt)
	    scandata.si_name[i] = nextchar;
	gotast = 0;
	if (nextchar == '*')
	    gotast = 1;
	}
    if (savecmt)
	{
	ptr->cmt = getname(i);
	if (curley == 0)
	    i--;
	strncpy(ptr->cmt, scandata.si_name,i);
	strcat(ptr->cmt, "\0");
	nexttoken = COMMENT;
	scandata.si_cmtptr = headptr;
	}
    else
	scanner(0);		/* get next token after comment */
}
		

/*
 * Called if the first char of token was a letter.  It scans until the
 *   nextchar is not a letter, digit, or underscore (or $).
 * The '$' is accepted for VMS/Pascal compatibility but it is 
 *   simply discarded.
 */

word()
{
    int index;
    char foundit;
    int j;

    if (nextchar >= 'A' && nextchar <= 'Z')
	nextchar = nextchar + 'a' - 'A';
    scandata.si_name[0] = nextchar;
    j = 1;
    getnext();
    for (; (nextchar >= 'A' && nextchar <= 'Z') ||
	   (nextchar >= 'a' && nextchar <= 'z') ||
	   (nextchar >= '0' && nextchar <= '9') ||
	   (nextchar == '_')||(nextchar == '$') ;)
	{
	if (j < WORDLENGTH-1)
	    if (nextchar != '$')
		{
		if (nextchar >= 'A' && nextchar <= 'Z')
		    nextchar = nextchar + 'a' - 'A';
		scandata.si_name[j++] = nextchar;
		}
	getnext();
	}
    scandata.si_name[j] = '\0';
    ahead = 1;
    scandata.si_idlen = j;

    /* look for word in reserved word table */

    foundit = 0;
    if (scandata.si_idlen > SYMBOLMAX)
	index = 0;
    else
	index = resindex[scandata.si_idlen];
    for (; !foundit && index != 0;)
	{
	if (!strcmp(resword[index], scandata.si_name))
	    {
	    nexttoken = restoken[index];
	    foundit = 1;
	    }
	else
	    index = resnext[index];
	}
    if (!foundit)
	nexttoken = IDENT;
}


/*
 * Called if first char of token was a digit. Scans until a non digit
 * is found.
 * Save the numeric string in 'name' field for expression parsing.
 */

number(noreal)
    char noreal;	/* Real numbers not allowed when getting array or
			 * subrange declaration.  This is to avoid
			 * confussion with "[1." and the start of a real number
			 */
{
    int nextdigit;
    float realvalue;
    int j = 0;
    float mult;
    int power;		/* for Pascal exponent notation */
    int expon;		/* for Pascal exponent notation */
    int i;
    char neg;		/* for negative number */

    scandata.si_cvalue = 0;
    scandata.si_dflag = 1;
    nexttoken = NUMCONST;
    do
	{
	nextdigit = nextchar - '0';
	scandata.si_cvalue = scandata.si_cvalue * 10 + nextdigit;
	scandata.si_name[j++] = nextchar;
	getnext();
	}
    while (nextchar >= '0' && nextchar <= '9');

    if (nextchar == '.' && (!noreal))
	{
	scandata.si_name[j++] = nextchar;
	getnext();
	scandata.si_dflag = 2;
	mult = 10;
	while (nextchar >= '0' && nextchar <= '9')
	    {
	    realvalue = (nextchar - '0') * (1/mult);
	    scandata.si_cvalue += realvalue;
	    scandata.si_name[j++] = nextchar;
	    mult = mult * 10;
	    getnext();
	    }
	}
    neg = 0;
    if (nextchar == 'E')
	{
	getnext();
	if (nextchar == '-')
	    {
	    neg = 1;
	    getnext();
	    scandata.si_name[j++] = '/';
	    }
	else
	    {
	    scandata.si_name[j++] = '*';
	    if (nextchar == '+')
		getnext();
	    }
	scandata.si_name[j++] = '(';
	scandata.si_name[j++] = '1';
	expon = 0;
	while (nextchar >= '0' && nextchar <= '9')
	    {
	    nextdigit = nextchar - '0';
	    expon = expon * 10 + nextdigit;
	    getnext();
	    }
	power = 1;
	for (i = 0; i < expon; i++)
	    {
	    scandata.si_name[j++] = '0';
	    power = power * 10;
	    }
	if (neg)
	    scandata.si_cvalue = scandata.si_cvalue / power;
	else
	    scandata.si_cvalue = scandata.si_cvalue * power;
	scandata.si_name[j++] = ')';
	}
    scandata.si_name[j] = '\0';
    ahead = 1;
}


/*
 * Called if first char of token was a single quot.
 * Token is constant.
 */

charconst()
{
    int i;

    nexttoken = CHARCONST;
    scandata.si_dflag = 0;
    getnext();
    for (i = 0; nextchar != '\''; i++)
	{
	scandata.si_name[i] = nextchar;
	getnext();
	}
    scandata.si_name[i] = '\0';
    scandata.si_idlen = i;
}


/*
 * Called if first char of token was a quot (").
 * Token is QUOTE.  Only needed for include syntax.
 */

strconst()
{
    int i;

    nexttoken = QUOTE;
    scandata.si_dflag = 0;
    getnext();
    for (i = 0; nextchar != '"'; i++)
	{
	scandata.si_name[i] = nextchar;
	getnext();
	}
    scandata.si_name[i] = '\0';
    scandata.si_idlen = i;
}


/*
 * Called if first char of token was a period.
 * Get 2nd period.
 */

dot()
{
    getnext();
    if (nextchar == '.')
	nexttoken = DOTDOT;
    else
	{
	ahead = 1;
	nexttoken = DOT;
	}
}


/*
 * Called if first char of token was a colon.
 * Determines if its a variable declaration or an assignment stmt.
 */

colon()
{
    getnext();
    if (nextchar == '=')
	nexttoken = ASSIGNOP;
    else
	{
	ahead = 1;
	nexttoken = COLON;
	}
}


/*
 * Called if first char of token was <, > or =.
 * Determines which of the 6 relational ops it is.
 */

relation()
{

    nexttoken = RELATIONAL;
    switch (nextchar) {
    case '<':
	getnext();
	if (nextchar == '>')
	    scandata.si_rel = nerel;
	else
	    if (nextchar == '=')
		scandata.si_rel = lerel;
	    else
		{
		scandata.si_rel = ltrel;
		ahead = 1;
		}
	break;
    case '>':
	getnext();
	if (nextchar == '=')
	    scandata.si_rel = gerel;
	else
	    {
	    scandata.si_rel = gtrel;
	    ahead = 1;
	    }
	break;
    case '=':
	scandata.si_rel = eqrel;
	break;
    }
}


/*
 * Initialize the:
 *   Pascal Reserved Word data structures
 *   Single-character Token data structures
 */
init()
{
    /*
     * Where keywords of each respective character length [n]
     * start in the resword array.
     */
    resindex[1] = 0;
    resindex[2] = 8;
    resindex[3] = 1;
    resindex[4] = 5;
    resindex[5] = 2;
    resindex[6] = 28;
    resindex[7] = 3;
    resindex[8] = 15;
    resindex[9] = 29;
    resindex[10] = 0;
    resindex[11] = 0;
    resindex[12] = 0;

    strcpy (resword[1] , "and");
    restoken[1] = ANDT;
    resnext[1] = 24;

    strcpy (resword[2], "array");
    restoken[2] = ARRAYT;
    resnext[2] = 7;

    strcpy (resword[3], "boolean");
    restoken[3] = BOOLEANT;
    resnext[3] = 18;

    strcpy (resword[5], "char");
    restoken[5] = CHART;
    resnext[5] = 9;

    strcpy (resword[6], "in");
    restoken[6] = IN;
    resnext[6] = 26;

    strcpy (resword[7], "const");
    restoken[7] = CONSTT;
    resnext[7] = 17;

    strcpy (resword[8], "do");
    restoken[8] = LOOPDO;
    resnext[8] = 20;

    strcpy (resword[9], "else");
    restoken[9] = ELSET;
    resnext[9] = 31;

    strcpy (resword[15], "external");
    restoken[15] = EXTERNALT;
    resnext[15] = 19;

    strcpy (resword[16], "include");
    restoken[16] = INCLUDET;
    resnext[16] = 21;

    strcpy (resword[17], "false");
    restoken[17] = FALSET;
    resnext[17] = 38;

    strcpy (resword[18], "forward");
    restoken[18] = FORWARDT;
    resnext[18] = 16;

    strcpy (resword[19], "function");
    restoken[19] = FUNCTIONT;
    resnext[19] = 66;

    strcpy (resword[20], "if");
    restoken[20] = IFT;
    resnext[20] = 6;

    strcpy (resword[21], "integer");
    restoken[21] = INTEGERT;
    resnext[21] = 30;

    strcpy (resword[24], "mod");
    restoken[24] = MODT;
    resnext[24] = 25;

    strcpy (resword[25], "not");
    restoken[25] = NOTT;
    resnext[25] = 37;

    strcpy (resword[26], "of");
    restoken[26] = OFT;
    resnext[26] = 27;

    strcpy (resword[27], "or");
    restoken[27] = ORT;
    resnext[27] = 52;

    strcpy (resword[28], "packed");
    restoken[28] = PACKED;
    resnext[28] = 32;

    strcpy (resword[29], "procedure");
    restoken[29] = PROCEDURET;
    resnext[29] = 60;

    strcpy (resword[30], "program");
    restoken[30] = PROGRAMT;
    resnext[30] = 40;

    strcpy (resword[31], "read");
    restoken[31] = READT;
    resnext[31] = 34;

    strcpy (resword[32], "readln");
    restoken[32] = READLNT;
    resnext[32] = 44;

    strcpy (resword[34], "then");
    restoken[34] = THENT;
    resnext[34] = 35;

    strcpy (resword[35], "true");
    restoken[35] = TRUET;
    resnext[35] = 36;

    strcpy (resword[36], "type");
    restoken[36] = TYPET;
    resnext[36] = 41;

    strcpy (resword[37], "var");
    restoken[37] = VART;
    resnext[37] = 43;

    strcpy (resword[38], "while");
    restoken[38] = WHILET;
    resnext[38] = 39;

    strcpy (resword[39], "write");
    restoken[39] = WRITET;
    resnext[39] = 46;

    strcpy (resword[40], "writeln");
    restoken[40] = WRITELNT;
    resnext[40] = 57;

    strcpy (resword[41], "file");
    restoken[41] = FILET;
    resnext[41] = 42;

    strcpy (resword[42], "real");
    restoken[42] = REALT;
    resnext[42] = 47;

    strcpy (resword[43], "set");
    restoken[43] = SETT;
    resnext[43] = 45;

    strcpy (resword[44], "record");
    restoken[44] = RECORDT;
    resnext[44] = 50;

    strcpy (resword[45], "end");
    restoken[45] = ENDT;
    resnext[45] = 49;

    strcpy (resword[46], "begin");
    restoken[46] = BEGINT;
    resnext[46] = 55;

    strcpy (resword[47], "case");
    restoken[47] = CASET;
    resnext[47] = 48;

    strcpy (resword[48], "with");
    restoken[48] = WITHT;
    resnext[48] = 54;

    strcpy (resword[49], "for");
    restoken[49] = FORT;
    resnext[49] = 51;

    strcpy (resword[50], "repeat");
    restoken[50] = REPEATT;
    resnext[50] = 53;

    strcpy (resword[51], "div");
    restoken[51] = INTDIVT;
    resnext[51] = 61;

    strcpy (resword[52], "to");
    restoken[52] = TOT;
    resnext[52] = 0;

    strcpy (resword[53], "downto");
    restoken[53] = DOWNTOT;
    resnext[53] = 59;

    strcpy (resword[54], "goto");
    restoken[54] = GOTOT;
    resnext[54] = 0;

    strcpy (resword[55], "until");
    restoken[55] = UNTILT;
    resnext[55] = 56;

    strcpy (resword[56], "label");
    restoken[56] = LABELT;
    resnext[56] = 62;

    strcpy (resword[57], "varying");
    restoken[57] = VARYING;
    resnext[57] = 58;

    strcpy (resword[58], "fortran");
    restoken[58] = EXTERNALT;
    resnext[58] = 64;

    strcpy (resword[59], "extern");
    restoken[59] = EXTERNALT;
    resnext[59] = 67;

    strcpy (resword[60], "otherwise");
    restoken[60] = OTHERWISE;
    resnext[60] = 69;

    strcpy (resword[61], "rem");
    restoken[61] = MODT;
    resnext[61] = 65;

    strcpy (resword[62], "descr");
    restoken[62] = MECHT;
    resnext[62] = 63;

    strcpy (resword[63], "immed");
    restoken[63] = MECHT;
    resnext[63] = 0;

    strcpy (resword[64], "stdescr");
    restoken[64] = MECHT;
    resnext[64] = 0;

    strcpy (resword[65], "ref");
    restoken[65] = MECHT;
    resnext[65] = 0;

    strcpy (resword[66], "unsigned");
    restoken[66] = UNSIGNT;
    resnext[66] = 0;

    strcpy (resword[67], "single");
    restoken[67] = REALT;
    resnext[67] = 68;

    strcpy (resword[68], "double");
    restoken[68] = DOUBLE;
    resnext[68] = 70;

    strcpy (resword[69], "quadruple");
    restoken[69] = DOUBLE;
    resnext[69] = 0;

    strcpy (resword[70], "module");
    restoken[70] = MODULET;
    resnext[70] = 0;

    chartoken[0] = ILLEGALTOKE;
    chartoken[1] = ILLEGALTOKE;
    chartoken[2] = ILLEGALTOKE;
    chartoken[3] = ILLEGALTOKE;
    chartoken[4] = ILLEGALTOKE;
    chartoken[5] = ILLEGALTOKE;
    chartoken[6] = ILLEGALTOKE;
    chartoken[7] = ILLEGALTOKE;
    chartoken[8] = ILLEGALTOKE;
    chartoken[9] = ILLEGALTOKE;
    chartoken[10] = ILLEGALTOKE;
    chartoken[11] = ILLEGALTOKE;
    chartoken[12] = ILLEGALTOKE;
    chartoken[13] = ILLEGALTOKE;
    chartoken[14] = ILLEGALTOKE;
    chartoken[15] = ILLEGALTOKE;
    chartoken[16] = ILLEGALTOKE;
    chartoken[17] = ILLEGALTOKE;
    chartoken[18] = ILLEGALTOKE;
    chartoken[19] = ILLEGALTOKE;
    chartoken[20] = ILLEGALTOKE;
    chartoken[21] = ILLEGALTOKE;
    chartoken[22] = ILLEGALTOKE;
    chartoken[23] = ILLEGALTOKE;
    chartoken[24] = ILLEGALTOKE;
    chartoken[25] = ILLEGALTOKE;
    chartoken[26] = ILLEGALTOKE;
    chartoken[27] = ILLEGALTOKE;
    chartoken[28] = ILLEGALTOKE;
    chartoken[29] = ILLEGALTOKE;
    chartoken[30] = ILLEGALTOKE;
    chartoken[31] = ILLEGALTOKE;
    chartoken[32] = ILLEGALTOKE;
    chartoken[33] = ILLEGALTOKE;
    chartoken[34] = QUOTE;
    chartoken[35] = POUND;
    chartoken[36] = ILLEGALTOKE;
    chartoken[37] = PERCENT;
    chartoken[38] = ILLEGALTOKE;
    chartoken[39] = ILLEGALTOKE;
    chartoken[40] = LEFTPAREN;
    chartoken[41] = RIGHTPAREN;
    chartoken[42] = MULT;
    chartoken[43] = PLUS;
    chartoken[44] = COMMA;
    chartoken[45] = MINUS;
    chartoken[46] = ILLEGALTOKE;
    chartoken[47] = DIVT;
    chartoken[48] = ILLEGALTOKE;
    chartoken[49] = ILLEGALTOKE;
    chartoken[50] = ILLEGALTOKE;
    chartoken[51] = ILLEGALTOKE;
    chartoken[52] = ILLEGALTOKE;
    chartoken[53] = ILLEGALTOKE;
    chartoken[54] = ILLEGALTOKE;
    chartoken[55] = ILLEGALTOKE;
    chartoken[56] = ILLEGALTOKE;
    chartoken[57] = ILLEGALTOKE;
    chartoken[58] = COLON;
    chartoken[59] = SEMICOLON;
    chartoken[60] = ILLEGALTOKE;
    chartoken[61] = ILLEGALTOKE;
    chartoken[62] = ILLEGALTOKE;
    chartoken[63] = ILLEGALTOKE;
    chartoken[64] = ILLEGALTOKE;
    chartoken[65] = ILLEGALTOKE;
    chartoken[66] = ILLEGALTOKE;
    chartoken[67] = ILLEGALTOKE;
    chartoken[68] = ILLEGALTOKE;
    chartoken[69] = ILLEGALTOKE;
    chartoken[70] = ILLEGALTOKE;
    chartoken[71] = ILLEGALTOKE;
    chartoken[72] = ILLEGALTOKE;
    chartoken[73] = ILLEGALTOKE;
    chartoken[74] = ILLEGALTOKE;
    chartoken[75] = ILLEGALTOKE;
    chartoken[76] = ILLEGALTOKE;
    chartoken[77] = ILLEGALTOKE;
    chartoken[78] = ILLEGALTOKE;
    chartoken[79] = ILLEGALTOKE;
    chartoken[80] = ILLEGALTOKE;
    chartoken[81] = ILLEGALTOKE;
    chartoken[82] = ILLEGALTOKE;
    chartoken[83] = ILLEGALTOKE;
    chartoken[84] = ILLEGALTOKE;
    chartoken[85] = ILLEGALTOKE;
    chartoken[86] = ILLEGALTOKE;
    chartoken[87] = ILLEGALTOKE;
    chartoken[88] = ILLEGALTOKE;
    chartoken[89] = ILLEGALTOKE;
    chartoken[90] = ILLEGALTOKE;
    chartoken[91] = LEFTBRACKET;
    chartoken[92] = ILLEGALTOKE;
    chartoken[93] = RIGHTBRACKET;
    chartoken[94] = UPARROW;
    chartoken[95] = ILLEGALTOKE;
    chartoken[96] = ILLEGALTOKE;
    chartoken[97] = ILLEGALTOKE;
    chartoken[98] = ILLEGALTOKE;
    chartoken[99] = ILLEGALTOKE;
    chartoken[100] = ILLEGALTOKE;
    chartoken[101] = ILLEGALTOKE;
    chartoken[102] = ILLEGALTOKE;
    chartoken[103] = ILLEGALTOKE;
    chartoken[104] = ILLEGALTOKE;
    chartoken[105] = ILLEGALTOKE;
    chartoken[106] = ILLEGALTOKE;
    chartoken[107] = ILLEGALTOKE;
    chartoken[108] = ILLEGALTOKE;
    chartoken[109] = ILLEGALTOKE;
    chartoken[110] = ILLEGALTOKE;
    chartoken[111] = ILLEGALTOKE;
    chartoken[112] = ILLEGALTOKE;
    chartoken[113] = ILLEGALTOKE;
    chartoken[114] = ILLEGALTOKE;
    chartoken[115] = ILLEGALTOKE;
    chartoken[116] = ILLEGALTOKE;
    chartoken[117] = ILLEGALTOKE;
    chartoken[118] = ILLEGALTOKE;
    chartoken[119] = ILLEGALTOKE;
    chartoken[120] = ILLEGALTOKE;
    chartoken[121] = ILLEGALTOKE;
    chartoken[122] = ILLEGALTOKE;
    chartoken[123] = ILLEGALTOKE;
    chartoken[124] = ILLEGALTOKE;
    chartoken[125] = ILLEGALTOKE;
    chartoken[126] = ILLEGALTOKE;
    chartoken[127] = ILLEGALTOKE;
}
