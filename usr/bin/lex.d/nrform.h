/*	@(#)nrform.h	1.2		11/7/83	*/

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
 *	Stephen Reilly, 07-Nov-83:
 * 001-	Because of the binary kit restrictions nrform can no longer be
 *	read in.  It is now a data structure tat will be used to create
 *	the Ratfor version of the lexical analy.
 *
 ***********************************************************************/

static char *nrformlin[] = {
"block data\n",
"integer cshift, csize, yynlin\n",
"common /yyllib/ cshift, csize, yynlin\n",
"data yynlin/YYNEWLINE/\n",
"end\n",
"block data\n",
"common /yyldat/ yyfnd, ymorf, yyprev, yybgin, yytop\n",
"integer yyfnd, yymorf, yyprev, yybgin, yytop\n",
"data yybgin/1/\n",
"data yyprev/YYNEWLINE/\n",
"data yytop/YYTOPVAL/\n",
"end\n",
"integer function yylook(dummy)\n",
"common /Lverif/ verif\n",
"common /Ladvan/ advan\n",
"common /Lstoff/ stoff\n",
"common /Lsfall/ sfall\n",
"common /Latable/ atable\n",
"common /Lextra/ extra\n",
"common /Lvstop/ vstop\n",
"integer verif(Sverif), advan(Sadvan),stoff(Sstoff),match(Smatch)\n",
"integer sfall(Ssfall),atable(Satable),extra(Sextra), vstop(Svstop)\n",
"integer state, lsp, r\n",
"integer  ch, n\n",
"common /yyldat/ yyfnd, yymorf, yyprev, yybgin, yytop, yylsp, yylsta(YYLMAX)\n",
"common /yyxel/ yyleng, yytext\n",
"integer yyfnd, yymorf, yylsta, yylsp, yytext, yyprev, yyleng, yytop\n",
"integer  lexshf, yytext(YYLMAX), yyback, yybgin\n",
"integer z, t\n",
"if (yymorf .eq. 0)\n",
"	yyleng = 0\n",
"else\n",
"	yymorf=0\n",
"1776\n",
"	lsp = 1\n",
"	state = yybgin\n",
"	if (yyprev .eq. YYNEWLINE)\n",
"		state = state + 1\n",
"	for (;;){\n",
"		r = stoff(state)\n",
"		if (r .eq. 0){\n",
"			z = sfall(state)\n",
"			if (z .eq. 0)\n",
"				break\n",
"			if(stoff(z) == 0) break\n",
"			}\n",
"		ch = input(dummy)\n",
"		ich = lexshf(ch)\n",
"		yyleng = yyleng+1\n",
"		yytext(yyleng) = ch\n",
"		1984\n",
"		if(r .gt. 0){\n",
"			t = r + ich\n",
"			if (t<= yytop){\n",
"			  if (verif(t) .eq. state){\n",
"				if(advan(t) == YYERROR){\n",
"					call unput(yytext(yyleng))\n",
"					yyleng = yyleng - 1\n",
"					break\n",
"					}\n",
"				state = advan(t)\n",
"				yylsta(lsp) = state\n",
"				lsp = lsp +1\n",
"				goto 2001\n",
"				}\n",
"			  }\n",
"			}\n",
"		if(r < 0){\n",
"			t = r + ich\n",
"			if (t <= yytop .and. verif(t) .eq. state){\n",
"				if(advan(t) == YYERROR){\n",
"					call unput(yytext(yyleng))\n",
"					yyleng = yyleng - 1\n",
"					break\n",
"					}\n",
"				state = advan(t)\n",
"				yylsta(lsp) = state\n",
"				lsp = lsp +1\n",
"				goto 2001\n",
"				}\n",
"			t = r + match(ich)\n",
"			if(t <= yytop && state == verif(t)){\n",
"				if(advan(t) == YYERROR){\n",
"					call unput(yytext(yyleng))\n",
"					yyleng = yyleng - 1\n",
"					break\n",
"					}\n",
"			state = advan(t)\n",
"			yylsta(lsp) = state\n",
"			lsp = lsp + 1\n",
"			goto 2001\n",
"			}\n",
"		}\n",
"		else {\n",
"			if (state > 0) state = sfall(state)\n",
"			if (state .gt. 0) r = stoff(state)\n",
"			if (state .gt. 0 .and. r .ne. 0)\n",
"				goto 1984\n",
"			call unput(yytext(yyleng))\n",
"			yyleng = yyleng -1\n",
"			break\n",
"			}\n",
"	2001\n",
"		continue\n",
"		}\n",
"	while (lsp .gt. 1){\n",
"		lsp = lsp -1\n",
"		ilsp = yylsta(lsp)\n",
"		yyfnd = atable(ilsp)\n",
"		if (yyfnd .gt. 0)\n",
"			if (vstop(yyfnd) .gt. 0){\n",
"				r = vstop(yyfnd)\n",
"				if (extra(r) .ne. 0){\n",
"					for(;;){\n",
"					ilsp = yylsta(lsp)\n",
"					if (yyback(atable(ilsp), -r) .eq. 1)\n",
"						break\n",
"					lsp= lsp -1\n",
"					call unput(yytext(yyleng))\n",
"					yyleng = yyleng -1\n",
"					}\n",
"					}\n",
"				yyprev = lexshf(yytext(yyleng))\n",
"				yylsp = lsp\n",
"				yyfnd = yyfnd + 1\n",
"				yylook = r\n",
"				yytext(yyleng+1) = 0\n",
"				return\n",
"				}\n",
"		call unput(yytext(yyleng))\n",
"		}\n",
"	if (yytext(1) .eq. 0){\n",
"		yylook=0\n",
"		return\n",
"		}\n",
"	yyprev = input(dummy)\n",
"	call output(yyprev)\n",
"	yyprev = lexshf(yyprev)\n",
"	yyleng = 0\n",
"	goto 1776\n",
"end\n",
"integer function yyback (isub, n)\n",
"common /Lvstop/ vstop\n",
"integer vstop(Svstop)\n",
"if (isub .ne. 0)\n",
"while (vstop(isub) .ne. 0){\n",
"	if (vstop(isub) .eq. m){\n",
"		yyback = 1\n",
"		return\n",
"		}\n",
"	isub = isub + 1\n",
"	}\n",
"yyback = 0\n",
"return \n",
"end\n",
0 };
