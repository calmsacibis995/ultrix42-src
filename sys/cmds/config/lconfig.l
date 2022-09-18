%{

/*
 *	@(#)lconfig.l	4.2	(ULTRIX)	9/10/90	
 */


/************************************************************************
 *									*
 *			Copyright (c) 1984,86,88 by			*
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
/*
 *
 *   Modification 
 *
 *  30-Sep-90      skc
 *      Added keyword "shadow".
 *
 *  20-Dec-89      Paul Grist
 *      Added support for the VMEbus - vba adapters. Placed "vba" in the
 *      keyword table as INTERCONNECT.
 *
 *  12-Jun-89 	   G Gopal
 *	Added configurable virtual memory keywords "maxssiz" , "maxdsiz"
 *	"vasslop", "maxretry" and "swapfrag".
 *
 *  13-Jun-88      chet
 *      Added configurable buffer cache keyword "bufcache"
 *
 *  1-25-88	Ricky Palmer
 *	Added MSI support.
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass support to the system.
 *
 * 8 Apr 86  -- lp
 *	Added bvp support
 *
 * 02 Apr 86 -- depp
 *	Added shared memory configurable items
 *
 * 05-Mar-86 -- jrs
 *	Added support for configuring bi based devices
 *
 *  4 June 85 -- depp
 *	Added new config specs "release" and "version".
 *
 */

/*	config.l	1.14	83/05/18	*/

#include <ctype.h>
#include "y.tab.h"
#include "config.h"

#define tprintf if (do_trace) printf

/*
 * Key word table
 */

struct kt {
	char *kt_name;
	int kt_val;
} key_words[] = {
	{ "adapter",	CONTROLLER },
	{ "and",	AND },
	{ "args",	ARGS },
	{ "at",		AT },
	{ "bufcache",	BUFCACHE },
	{ "bus",	BUS },
	{ "cinode",	REMOTE_CONTROLLER },
	{ "config",	CONFIG },
	{ "controller",	CONTROLLER },
	{ "cpu",	CPU },
	{ "csr",	CSR },
	{ "device",	DEVICE },
	{ "disk",	DISK },
	{ "dmmin",	DMMIN },
	{ "dmmax",	DMMAX },
	{ "drive",	DRIVE },
	{ "dst",	DST },
	{ "dumps",	DUMPS },
	{ "flags",	FLAGS },
	{ "hz",		HZ },
	{ "ident",	IDENT },
	{ "machine",	MACHINE },
	{ "major",	MAJOR },
	{ "makeoptions", MAKEOPTIONS },
	{ "master",	MASTER },
	{ "maxuprc",	MAXUPRC },
	{ "maxusers",	MAXUSERS },
	{ "mba",	INTERCONNECT },
	{ "minor",	MINOR },
	{ "mscp",	MSCP },
	{ "msinode",	REMOTE_CONTROLLER },
	{ "nexus",	NEXUS },
	{ "node",	NEXUS },
	{ "on",		ON },
	{ "options",	OPTIONS },
	{ "physmem",	PHYSMEM },
	{ "port",	CONTROLLER },
	{ "priority",	PRIORITY },
	{ "processors",	PROCESSORS },
	{ "pseudo-device",PSEUDO_DEVICE },
	{ "release",	RELEASE },
	{ "root",	ROOT },
	{ "scs_sysid",	SCSID },
	{ "size",	SIZE },
	{ "slave",	SLAVE },
	{ "swap",	SWAP },
	{ "shadow",	SHADOW },
	{ "tape",	DEVICE },
	{ "timezone",	TIMEZONE },
	{ "trace",	TRACE },
	{ "uba",	INTERCONNECT },
	{ "vaxbi",	INTERCONNECT },
        { "vba",        INTERCONNECT },
	{ "vector",	VECTOR },
	{ "version",	VERSION },
	{ "smmax",	SMMAX },
	{ "smmin",	SMMIN },
	{ "smbrk",	SMBRK },
	{ "smseg",	SMSEG },
	{ "smsmat",	SMSMAT },
	{ "maxuva",	MAXUVA },
	{ "maxtsiz",	MAXTSEG },
	{ "maxdsiz",	MAXDSIZE },
	{ "maxssiz",	MAXSSIZE },
	{ "vasslop",	VASSLOP },
	{ "maxretry",	MAXRETRY },
	{ "swapfrag",	SWAPFRAG },
	{ 0, 0 },
};
%}
WORD	[A-Za-z_][-A-Za-z_]*
%%
{WORD}		{
			int i;

			yylval.str = yytext;
			if ((i = kw_lookup(yytext)) == -1)
			{
				tprintf("id(%s) ", yytext);
				return ID;
			}
			tprintf("(%s) ", yytext);
			return i;
		}
\"[^"]+\"	{
			yytext[strlen(yytext)-1] = '\0';
			yylval.str = yytext + 1;
			return ID;
		}
0[0-7]*		{
			yylval.val = octal(yytext);
			tprintf("#O:%o ", yylval.val);
			return NUMBER;
		}
0x[0-9a-fA-F]+	{
			yylval.val = hex(yytext);
			tprintf("#X:%x ", yylval.val);
			return NUMBER;
		}
[1-9][0-9]*	{
			yylval.val = atoi(yytext);
			tprintf("#D:%d ", yylval.val);
			return NUMBER;
		}
[0-9]"."[0-9]*	{
			double atof();
			yylval.fval = atof(yytext);
			return FPNUMBER;
		}
"-"		{
			return MINUS;
		}
"?"		{
			yylval.val = -1;
			tprintf("? ");
			return NUMBER;
		}
\n/[ \t]	{
			yyline++;
			tprintf("\n... ");
		}
\n		{
			yyline++;
			tprintf("\n");
			return SEMICOLON;
		}
#.*		{	/* Ignored (comment) */;	}
[ \t]*		{	/* Ignored (white space) */;	}
";"		{	return SEMICOLON;		}
","		{	return COMMA;			}
"="		{	return EQUALS;			}
"@"		{	return AT;			}
.		{	return yytext[0];		}

%%
/*
 * kw_lookup
 *	Look up a string in the keyword table.  Returns a -1 if the
 *	string is not a keyword otherwise it returns the keyword number
 */

kw_lookup(word)
register char *word;
{
	register struct kt *kp;

	for (kp = key_words; kp->kt_name != 0; kp++)
		if (eq(word, kp->kt_name))
			return kp->kt_val;
	return -1;
}

/*
 * Number conversion routines
 */

octal(str)
char *str;
{
	int num;

	(void) sscanf(str, "%o", &num);
	return num;
}

hex(str)
char *str;
{
	int num;

	(void) sscanf(str+2, "%x", &num);
	return num;
}
