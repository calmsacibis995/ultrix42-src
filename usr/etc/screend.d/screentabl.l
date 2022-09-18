%{
/*
 * SCCSID: @(#)screentabl.l	4.1	(ULTRIX)	9/11/90
 *
 * screentabl.l
 *	scanner for screentab database
 *
 * Modification history:
 *
 * 19 December 1988	Jeffrey Mogul/DECWRL
 *	Created.
 */
/*
 *               Copyright 1989, 1990 Digital Equipment Corporation
 *                          All Rights Reserved
 * 
 * 
 * Permission to use, copy, and modify this software and its documentation
 * is hereby granted only under the following terms and conditions.  Both
 * the above copyright notice and this permission notice must appear in
 * all copies of the software, derivative works or modified versions, and
 * any portions threof, and both notices must appear in supporting
 * documentation.
 * 
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-free
 * right and license under any changes, enhancements or extensions made to the
 * core functions of the software, including but not limited to those affording
 * compatibility with other hardware or software environments, but excluding
 * applications which incorporate this software.  Users further agree to use
 * their best efforts to return to Digital any such changes, enhancements or
 * extensions that they make and inform Digital of noteworthy uses of this
 * software.  Correspondence should be provided to Digital at:
 * 
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301  
 * 
 * Comments and bug reports may also be sent using electronic mail to:
 * 			screend-reports@decwrl.dec.com
 * 
 * 	>> This software may NOT be distributed to third parties. <<
 *   
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL
 * EQUIPMENT CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include "screentab.h"
#include "y.tab.h"

char *malloc();

extern	int	yylval;
char linebuf[255];

/*
 * Reserved word table
 */
struct rw {
	char *name;
	int token;
} reserved_words[] = {
	{"accept",	ACCEPT},
	{"and",		AND},
	{"any",		ANY},
	{"between",	BETWEEN},
	{"default",	DEFAULT},
	{"for",		FOR},
	{"from",	FROM},
	{"host",	HOST},
	{"host-not",	HOSTNOT},
	{"icmp",	ICMP},
	{"infotype",	INFOTYPE},
	{"is",		IS},
	{"log",		LOG},
	{"multicast",	MULTICAST},
	{"net",		NET},
	{"net-not",	NETNOT},
	{"netmask",	NETMASK},
	{"notify",	NOTIFY},
	{"port",	PORT},
	{"port-not",	PORTNOT},
	{"proto",	PROTO},
	{"proto-not",	PROTONOT},
	{"reject",	REJECTIT},
	{"reserved",	RESERVED},
	{"subnet",	SUBNET},
	{"subnet-not",	SUBNETNOT},
	{"tcp",		TCP},
	{"to",		TO},
	{"type",	TYPE},
	{"type-not",	TYPENOT},
	{"udp",		UDP},
	{"xserver",	XSERVER},
	{0, 0}
};

%}

Word	[a-zA-Z_][0-9a-zA-Z_.-]*	/* maybe should include "$"? */
String	\"([^\"]|(\\\"))*\"	/* quotes are escaped within strings */

%%	/* beginning of rules section */

^.*$		{ 
		 strcpy(linebuf, yytext);
		 REJECT;
		} /* for error messages */


"/*".*"*/"	;	/* comment */
"#".*$		;	/* comment, too */
\n		;
[ \t]+		;	/* white space (not in a string) */

{Word}		{
		    int i;
		    
		    if ((i = rw_lookup(yytext)) < 0) {
		    	yylval = makestring(yytext);
			return(NAME);
		    }
		    return(reserved_words[i].token);
		}

{String}	{yytext[strlen(yytext)-1] = 0;		/* strip leading " */
		 yylval = makestring(&(yytext[1]));	/* strip trailing " */
		 return(LITERAL);
		}

0x[0-9a-fA-F]*	{	/* hex */
			yylval = makestring(yytext);
			return(HEXNUMBER);
		}

[0-9][0-9]*		{	/* decimal */
			yylval = makestring(yytext);
			return(DECNUMBER);
		}




"."		return(DOT);

";"		return(SEMICOLON);


%%	/* user subroutines section */

makestring(s)
char *s;
{
	char *snew = malloc(strlen(s)+1);
	
	strcpy(snew, s);
	
	return((int)snew);
}

/*
 * look up a reserved word; return the table index or -1 if not found.
 */
rw_lookup(word)
register char *word;
{
	register int i = 0;
	register struct rw *rp;
	
	rp = &(reserved_words[i]);
	
	while (rp->name) {
	    if (strcmp(word, rp->name) == 0)
	    	return(i);
	    rp++;
	    i++;
	}
	return(-1);
}
