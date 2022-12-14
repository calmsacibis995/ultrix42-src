%{
#ifndef lint
static char *sccsid = "@(#)tcplex.l	4.2	(ULTRIX)	1/25/91";
#endif
/*
 * Copyright (c) 1988-1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header: tcplex.l,v 1.19 90/10/25 13:15:01 mccanne Exp $ (LBL)";
#endif

#include <sys/types.h>
#include <ctype.h>
#include "interface.h"
#include "nametoaddr.h"

#include <sys/time.h>
#include <net/bpf.h>
#include "tokdefs.h"

#ifdef FLEX_SCANNER
#undef YY_INPUT
#define YY_INPUT(buf, result, max)\
 {\
	char *src = in_buffer;\
	int i;\
\
	if (*src == 0)\
		result = YY_NULL;\
	else {\
		for (i = 0; *src && i < max; ++i)\
			buf[i] = *src++;\
		in_buffer += i;\
		result = i;\
	}\
 }
#else
#undef getc
#define getc(fp)  (*in_buffer == 0 ? EOF : *in_buffer++)
#endif

extern YYSTYPE yylval;
static char *in_buffer;

%}

N		([0-9]+|(0X|0x)[0-9A-Fa-f]+)
B		([0-9A-Fa-f][0-9A-Fa-f]?)

%a 3000

%%
dst		return DST;
src		return SRC;

ether		return ETHER;
arp		return ARP;
rarp		return RARP;
ip		return IP;
tcp		return TCP;
udp		return UDP;

host		return HOST;
net		return NET;
port		return PORT;
proto		return PROTO;

gateway		return GATEWAY;

less		return LESS;
greater		return GREATER;
byte		return BYTE;
broadcast	return BROADCAST;

and		return AND;
or		return OR;
not		return '!';

len		return LEN;

[ \n\t]			;
[+\-*/:\[\]!<>()&|=]	return yytext[0];
">="			return GEQ;
"<="			return LEQ;
"!="			return NEQ;
"<<"			return LSH;
">>"			return RSH;
{N}			{ yylval.i = stoi(yytext); return NUM; }
({N}\.{N})|({N}\.{N}\.{N})|({N}\.{N}\.{N}\.{N})	{ 
			yylval.h = atoin(yytext); return HID;
}
{B}:{B}:{B}:{B}:{B}:{B} { yylval.e = ETHER_aton(yytext); return EID; }
{B}:+({B}:+)+		{ error("bogus ethernet address %s", yytext); }
[A-Za-z][-_.A-Za-z0-9]*	{ yylval.s = yytext; return ID; }
"\\"[^ !()\n\t]+	{ yylval.s = yytext + 1; return ID; }
[^ \[\]\t\n\-_.A-Za-z0-9!<>()&|=]+    { error("illegal token: %s\n", yytext); }
.			{ error("illegal char '%c'", *yytext); }
%%
void
lex_init(buf)
	char *buf;
{
	in_buffer = buf;;
}
#ifndef FLEX_SCANNER
int 
yywrap()
/* so we don't need -ll */
{
	return 1;
}				
#endif
