%{
/*
 * SCCSID: @(#)screentab.y	4.2	(ULTRIX)	4/30/91
 *
 * screentab.y
 *
 * yacc grammar for screentab
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
#include <stdio.h>
#include "screentab.h"

extern int yylineno;
extern char linebuf[];

extern int semantic_errors;

extern char *sourcefile;
int	defaction = ASACTION_REJECT;

struct NetmaskData curnetmask;
struct ActionSpec  curaction;
struct ActionSpec  curactionreverse;
struct AddrSpecX *asxp;
struct PortSpecX *psxp;
struct PortValue pval;
struct in_addr ia;
int hexnum;
int osp_flags;

struct ObjectSpec *MakeOSpec();
%}

%start	ScreenTabFile

/*
 * Reserved words
 */
%token	ACCEPT AND ANY BETWEEN DEFAULT FOR FROM
%token	HOST HOSTNOT ICMP INFOTYPE IS
%token	LOG MULTICAST NET NETNOT NETMASK NOTIFY
%token	PORT PORTNOT PROTO PROTONOT
%token	REJECTIT RESERVED SUBNET SUBNETNOT
%token	TCP TO TYPE TYPENOT UDP XSERVER

/*
 * string-valued tokens
 */
%token	NAME	LITERAL

/*
 * numeric-valued tokens (but they are passed as strings)
 */
%token	DECNUMBER HEXNUMBER

/*
 * punctuation
 */
%token	DOT
%token	SEMICOLON

%%	/* rules section */

ScreenTabFile	:	RuleList
		;

RuleList	:	SubnetSpec
		|	DefaultSpec
		|	PairSpec
		|	RuleList SubnetSpec
		|	RuleList PairSpec
		|	RuleList DefaultSpec
		;

SubnetSpec	:	FOR Netval NETMASK IS Maskval SEMICOLON
			{
			    curnetmask.network.s_addr = $2;
			    curnetmask.mask.s_addr = $5;
			    StoreNetmask(&curnetmask);
			}
		;

DefaultSpec	:	DEFAULT Decision  SEMICOLON
			{
			    defaction = $2;
			}
		;

PairSpec	:	FROM Object TO Object Decision SEMICOLON
			{
			    curaction.from = *(struct ObjectSpec *)$2;
			    free($2);
			    curaction.to = *(struct ObjectSpec *)$4;
			    free($4);
			    curaction.action = $5;
			    StoreAction(&curaction);
			}
		|	BETWEEN Object AND Object Decision SEMICOLON
			{
			    curaction.from = *(struct ObjectSpec *)$2;
			    free($2);
			    curaction.to = *(struct ObjectSpec *)$4;
			    free($4);
			    curaction.action = $5;
			    StoreAction(&curaction);
			    curactionreverse.from = curaction.to;
			    curactionreverse.to = curaction.from;
			    curactionreverse.action = curaction.action;
			    StoreAction(&curactionreverse);
			}
		;

Decision	:	ACCEPT
			{ $$ = ASACTION_ACCEPT; }
		|	ACCEPT LOG
			{ $$ = ASACTION_ACCEPT|ASACTION_LOG; }
		|	REJECTIT
			{ $$ = ASACTION_REJECT; }
		|	REJECTIT LOG
			{ $$ = ASACTION_REJECT|ASACTION_LOG; }
		|	REJECTIT NOTIFY
			{ $$ = ASACTION_REJECT|ASACTION_NOTIFY; }
		|	REJECTIT NOTIFY LOG
			{ $$ = ASACTION_REJECT|ASACTION_NOTIFY|ASACTION_LOG; }
		|	REJECTIT LOG NOTIFY
			{ $$ = ASACTION_REJECT|ASACTION_NOTIFY|ASACTION_LOG; }
		;

Object		:	ObjAddress
			{
			    $$ = (YYSTYPE)MakeOSpec(
						(struct AddrSpecX *)$1,
						NULL);
			}
		|	ObjPort
			{
			    $$ = (YYSTYPE)MakeOSpec(
						NULL,
						(struct PortSpecX *)$1);
			}
		|	ObjAddress ObjPort
			{
			    $$ = (YYSTYPE)MakeOSpec(
						(struct AddrSpecX *)$1,
						(struct PortSpecX *)$2);
			}
		;

HostOrNot	:	HOSTNOT
			{ $$ = OSF_NOTADDR; }
		|	HOST
			{ $$ = OSF_DEFAULT; }
		;

NetOrNot	:	NETNOT
			{ $$ = OSF_NOTADDR; }
		|	NET
			{ $$ = OSF_DEFAULT; }
		;

SubnetOrNot	:	SUBNETNOT
			{ $$ = OSF_NOTADDR; }
		|	SUBNET
			{ $$ = OSF_DEFAULT; }
		;

PortOrNot	:	PORTNOT
			{ $$ = OSF_NOTPORT; }
		|	PORT
			{ $$ = OSF_DEFAULT; }
		;

TypeOrNot	:	TYPENOT
			{ $$ = OSF_NOTPORT; }
		|	TYPE
			{ $$ = OSF_DEFAULT; }
		;

ProtoOrNot	:	PROTONOT
			{ $$ = OSF_NOTPROTO; }
		|	PROTO
			{ $$ = OSF_DEFAULT; }
		;

ObjAddress	:	NetOrNot Netval
			{
			    asxp = (struct AddrSpecX *)malloc(sizeof(*asxp));
			    
			    asxp->aspec.addrtype = ASAT_NET;
			    VerifyNetVal($2);
			    asxp->aspec.aval.network.s_addr = $2;
			    asxp->flags = $1;
			    $$ = (YYSTYPE)asxp;
			}
		|	SubnetOrNot Subnetval
			{
			    asxp = (struct AddrSpecX *)malloc(sizeof(*asxp));
			    
			    asxp->aspec.addrtype = ASAT_SUBNET;
			    asxp->aspec.aval.subnet.s_addr = $2;
			    asxp->flags = $1;
			    $$ = (YYSTYPE)asxp;
			}
		|	HostOrNot Hostval
			{
			    asxp = (struct AddrSpecX *)malloc(sizeof(*asxp));
			    
			    asxp->aspec.addrtype = ASAT_HOST;
			    asxp->aspec.aval.host.s_addr = $2;
			    asxp->flags = $1;
			    $$ = (YYSTYPE)asxp;
			}
		|	ANY
			{
			    asxp = (struct AddrSpecX *)malloc(sizeof(*asxp));
			    
			    asxp->aspec.addrtype = ASAT_ANY;
			    asxp->aspec.aval.host.s_addr = 0;
			    asxp->flags = OSF_DEFAULT;
			    $$ = (YYSTYPE)asxp;
			}
		;

ObjPort		:	ICMP TypeOrNot Icmptype
			{
			    psxp = (struct PortSpecX *)malloc(sizeof(*psxp));

			    psxp->pspec.proto = IPPROTO_ICMP;
			    psxp->pspec.pval.code = $3;
			    psxp->pspec.pval.port.discrim = PORTV_ANY;
			    psxp->flags = $2;
			    $$ = (YYSTYPE)psxp;
			}
		|	TCP PortOrNot Portval
			{
			    psxp = (struct PortSpecX *)malloc(sizeof(*psxp));

			    psxp->pspec.proto = IPPROTO_TCP;
			    psxp->pspec.pval.port =
						*((struct PortValue *)($3));
			    psxp->flags = $2;
			    $$ = (YYSTYPE)psxp;
			}
		|	UDP PortOrNot Portval
			{
			    psxp = (struct PortSpecX *)malloc(sizeof(*psxp));

			    psxp->pspec.proto = IPPROTO_UDP;
			    psxp->pspec.pval.port =
			    			*((struct PortValue *)($3));
			    psxp->flags = $2;
			    $$ = (YYSTYPE)psxp;
			}
		|	ProtoOrNot Protoval
			{
			    psxp = (struct PortSpecX *)malloc(sizeof(*psxp));

			    psxp->pspec.proto = $2;
			    psxp->pspec.pval.port.discrim = PORTV_ANY;
			    psxp->pspec.pval.port.value = 0;
			    psxp->flags = $1;
			    $$ = (YYSTYPE)psxp;
			}
		;

/* Every rule after here returns an integer value */

Netval		:	NAME
			{
			    $$ = NetNameToNumber($1);
			 }
		|	ANY
			{ $$ = INADDR_ANY; }
		|	DottedQuad
			{ $$ = $1; }
		;

Maskval		:	NAME
			{ $$ = MaskNameToNumber($1); }
		|	DottedQuad
			{ $$ = $1; }
		;

Subnetval	:	NAME
			{ $$ = SubnetNameToNumber($1); }
		|	ANY
			{ $$ = INADDR_ANY; }
		|	DottedQuad
			{ $$ = $1; }
		;

Hostval		:	NAME
			{ $$ = HostNameToNumber($1); }
		|	ANY
			{ $$ = INADDR_ANY; }
		|	DottedQuad
			{ $$ = $1; }
		;

DottedQuad	:	Number DOT Number DOT Number DOT Number
			{
			    ia.S_un.S_un_b.s_b1 = $1;
			    ia.S_un.S_un_b.s_b2 = $3;
			    ia.S_un.S_un_b.s_b3 = $5;
			    ia.S_un.S_un_b.s_b4 = $7;
			    $$ = ia.s_addr;	/* network order */
			}
		;

Icmptype	:	NAME
			{ $$ = ICMPNameToNumber($1); }
		|	ANY
			{ $$ = 0; }
		|	INFOTYPE
			{ $$ = -1; }
		|	Number
			{ $$ = $1; }
		;

Portval		:	NAME
			{
			    pval.discrim = PORTV_EXACT;
			    pval.value = PortNameToNumber($1);
			    $$ = (YYSTYPE)(&pval);
			}
		|	ANY
			{
			    pval.discrim = PORTV_ANY;
			    pval.value = 0;
			    $$ = (YYSTYPE)(&pval);
			}
		|	RESERVED
			{
			    pval.discrim = PORTV_RESERVED;
			    pval.value = 0;
			    $$ = (YYSTYPE)(&pval);
			}
		|	XSERVER
			{
			    pval.discrim = PORTV_XSERVER;
			    pval.value = 0;
			    $$ = (YYSTYPE)(&pval);
			}
		|	Number
			{
			    pval.discrim = PORTV_EXACT;
			    pval.value = htons($1);
			    $$ = (YYSTYPE)(&pval);
			}
		;

Protoval	:	NAME
			{ $$ = ProtoNameToNumber($1); }
		|	Number
			{ $$ = $1; }
		|	TCP
			{ $$ = IPPROTO_TCP; }
		|	UDP
			{ $$ = IPPROTO_UDP; }
		|	ICMP
			{ $$ = IPPROTO_ICMP; }
		;


/* Numeric conversions */
Number		:	HEXNUMBER
			{
			    sscanf($1, "%x", &hexnum);
			    $$ = hexnum;
			}
		|	DECNUMBER
			{
			    $$ = atoi($1);
			}
		;

%%	/* subroutines */

yyerror(s)
char *s;
{
	fflush(stdout);
	fprintf(stderr, "\"%s\", line %d: %s\n", sourcefile, yylineno, s);
	fprintf(stderr,"erroneous line: %s\n", linebuf);
}

yywarn(s)
char *s;
{
	fflush(stdout);
	fprintf(stderr, "\"%s\", line %d: %s\n", sourcefile, yylineno, s);
	fprintf(stderr,"questionable line: %s\n", linebuf);
}

struct ObjectSpec *
MakeOSpec(asxp, psxp)
struct AddrSpecX *asxp;	/* WE FREE THIS if not NULL */
struct PortSpecX *psxp;	/* WE FREE THIS if not NULL */
{
	struct ObjectSpec *osp = (struct ObjectSpec *)malloc(sizeof(*osp));
	
	osp->flags = OSF_DEFAULT;

	if (asxp) {
	    osp->aspec = asxp->aspec;
	    osp->flags |= asxp->flags;
	    free(asxp);
	}
	else {
	    osp->aspec.addrtype = ASAT_ANY;
	    osp->aspec.aval.host.s_addr = 0;
	}

	if (psxp) {
	    osp->pspec = psxp->pspec;
	    osp->flags |= psxp->flags;
	    free(psxp);
	}
	else {
	    osp->pspec.proto = 0;
	    osp->pspec.pval.port.discrim = PORTV_ANY;
	    osp->pspec.pval.port.value = 0;
	    osp->pspec.pval.code = 0;
	}

	return(osp);
}

VerifyNetVal(nv)
register u_long nv;		/* in network order */
{
	register u_long i = ntohl(nv);
	register u_long netnum;
	
	if (IN_CLASSA(i))
	    netnum = htonl(i&IN_CLASSA_NET);
	else if (IN_CLASSB(i))
	    netnum = htonl(i&IN_CLASSB_NET);
	else if (IN_CLASSC(i))
	    netnum = htonl(i&IN_CLASSC_NET);
	else
	    netnum = i;	/* XXX wrong for Multicast? XXX */

	if (nv != netnum) {
	    yyerror("malformed network number (might be a subnet?)");
	    semantic_errors++;
	}
}
