/*@(#)yaccpar.h	1.1	11/7/83*/

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
 *	Stephen Reilly, 06-Nov-83:
 * 000- Created this module for binary kit.  This source came from yaccpar.
 *
 ***********************************************************************/

static char *yacclin[] ={
"#ifndef lint\n",
"static char yaccpar_sccsid[] = \"@(#)yaccpar	4.1	(Berkeley)	2/11/83\";\n",
"#endif\n",
"\n",
"#\n",
"# define YYFLAG -1000\n",
"# define YYERROR goto yyerrlab\n",
"# define YYACCEPT return(0)\n",
"# define YYABORT return(1)\n",
"\n",
"/*	parser for yacc output	*/\n",
"\n",
"#ifdef YYDEBUG\n",
"int yydebug = 0; /* 1 for debugging */\n",
"#endif\n",
"YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */\n",
"int yychar = -1; /* current input token number */\n",
"int yynerrs = 0;  /* number of errors */\n",
"short yyerrflag = 0;  /* error recovery flag */\n",
"\n",
"yyparse() {\n",
"\n",
"	short yys[YYMAXDEPTH];\n",
"	short yyj, yym;\n",
"	register YYSTYPE *yypvt;\n",
"	register short yystate, *yyps, yyn;\n",
"	register YYSTYPE *yypv;\n",
"	register short *yyxi;\n",
"\n",
"	yystate = 0;\n",
"	yychar = -1;\n",
"	yynerrs = 0;\n",
"	yyerrflag = 0;\n",
"	yyps= &yys[-1];\n",
"	yypv= &yyv[-1];\n",
"\n",
" yystack:    /* put a state and value onto the stack */\n",
"\n",
"#ifdef YYDEBUG\n",
"	if( yydebug  ) printf( \"state %d, char 0%o\\n\", yystate, yychar );\n",
"#endif\n",
"		if( ++yyps> &yys[YYMAXDEPTH] ) { yyerror( \"yacc stack overflow\" ); return(1); }\n",
"		*yyps = yystate;\n",
"		++yypv;\n",
"		*yypv = yyval;\n",
"\n",
" yynewstate:\n",
"\n",
"	yyn = yypact[yystate];\n",
"\n",
"	if( yyn<= YYFLAG ) goto yydefault; /* simple state */\n",
"\n",
"	if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;\n",
"	if( (yyn += yychar)<0 || yyn >= YYLAST ) goto yydefault;\n",
"\n",
"	if( yychk[ yyn=yyact[ yyn ] ] == yychar ){ /* valid shift */\n",
"		yychar = -1;\n",
"		yyval = yylval;\n",
"		yystate = yyn;\n",
"		if( yyerrflag > 0 ) --yyerrflag;\n",
"		goto yystack;\n",
"		}\n",
"\n",
" yydefault:\n",
"	/* default state action */\n",
"\n",
"	if( (yyn=yydef[yystate]) == -2 ) {\n",
"		if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;\n",
"		/* look through exception table */\n",
"\n",
"		for( yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 ) ; /* VOID */\n",
"\n",
"		while( *(yyxi+=2) >= 0 ){\n",
"			if( *yyxi == yychar ) break;\n",
"			}\n",
"		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */\n",
"		}\n",
"\n",
"	if( yyn == 0 ){ /* error */\n",
"		/* error ... attempt to resume parsing */\n",
"\n",
"		switch( yyerrflag ){\n",
"\n",
"		case 0:   /* brand new error */\n",
"\n",
"			yyerror( \"syntax error\" );\n",
"		yyerrlab:\n",
"			++yynerrs;\n",
"\n",
"		case 1:\n",
"		case 2: /* incompletely recovered error ... try again */\n",
"\n",
"			yyerrflag = 3;\n",
"\n",
"			/* find a state where \"error\" is a legal shift action */\n",
"\n",
"			while ( yyps >= yys ) {\n",
"			   yyn = yypact[*yyps] + YYERRCODE;\n",
"			   if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE ){\n",
"			      yystate = yyact[yyn];  /* simulate a shift of \"error\" */\n",
"			      goto yystack;\n",
"			      }\n",
"			   yyn = yypact[*yyps];\n",
"\n",
"			   /* the current yyps has no shift onn \"error\", pop stack */\n",
"\n",
"#ifdef YYDEBUG\n",
"			   if( yydebug ) printf( \"error recovery pops state %d, uncovers %d\\n\", *yyps, yyps[-1] );\n",
"#endif\n",
"			   --yyps;\n",
"			   --yypv;\n",
"			   }\n",
"\n",
"			/* there is no state on the stack with an error shift ... abort */\n",
"\n",
"	yyabort:\n",
"			return(1);\n",
"\n",
"\n",
"		case 3:  /* no shift yet; clobber input char */\n",
"\n",
"#ifdef YYDEBUG\n",
"			if( yydebug ) printf( \"error recovery discards char %d\\n\", yychar );\n",
"#endif\n",
"\n",
"			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */\n",
"			yychar = -1;\n",
"			goto yynewstate;   /* try again in the same state */\n",
"\n",
"			}\n",
"\n",
"		}\n",
"\n",
"	/* reduction by production yyn */\n",
"\n",
"#ifdef YYDEBUG\n",
"		if( yydebug ) printf(\"reduce %d\\n\",yyn);\n",
"#endif\n",
"		yyps -= yyr2[yyn];\n",
"		yypvt = yypv;\n",
"		yypv -= yyr2[yyn];\n",
"		yyval = yypv[1];\n",
"		yym=yyn;\n",
"			/* consult goto table to find next state */\n",
"		yyn = yyr1[yyn];\n",
"		yyj = yypgo[yyn] + *yyps + 1;\n",
"		if( yyj>=YYLAST || yychk[ yystate = yyact[yyj] ] != -yyn ) yystate = yyact[yypgo[yyn]];\n",
"		switch(yym){\n",
"			$A \n",
"		}\n",
"		goto yystack;  /* stack new state and value */\n",
"\n",
"	}\n",
0 };
