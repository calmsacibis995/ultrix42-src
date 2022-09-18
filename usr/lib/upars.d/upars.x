#	@(#)upars.x	4.1				7/2/90
#
 /*
 * Program upars.x, lex.yy.c,  Module 
 *
 *									
 *			Copyright (c) 1985 by			
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.		
 *						
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Program upars.x, lex.yy.c,  Module 
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 *
 * 1.00 10-Jul-1985
 *     DECnet Ultrix V1.0
 *
 */

#
#	upars.x
#
#	lex definition of upars compiler lexer
#
#
#


anything	[\40-\176\n]
digit 		[-+0-9]
hexdigit	[0-9A-Fa-f]
octdigit	[0-7]
alpha		[A-Za-z_$]
alphanum	[A-Za-z0-9_$]
%%

		extern long	yyintval;

"/*"			{
			comment:  if (yyinput() == '*')
					if (yyinput() != '/')
						goto comment;
					else ;
				else
					goto comment;
			}


0{octdigit}+		{
			sscanf(yytext, "%o", &yyintval);
			return (INTEGER);
			}

0(X|x){hexdigit}+ 	{
			sscanf(yytext+2, "%x", &yyintval);
			return(INTEGER);
			}

{digit}+		{
			sscanf(yytext, "%ld", &yyintval);
			return(INTEGER);
			}

":"			return(COLON);
";"			return(SEMICOLON);
"%%"			return(PERCENTS);
","			return(COMMA);
"!"			return(BANG);
"{"			return(LBRACE);
"}"			return(RBRACE);


{alpha}{alphanum}* 	return
			(!strncmp(yytext, "init_state", 10) ? INIT_STATE :
			 !strncmp(yytext, "state",       5) ? STATE      :
			 !strncmp(yytext, "$DEBUG",      6) ? DEBUG      :
			 !strncmp(yytext, "binary",      6) ? BINARY     :
			 !strncmp(yytext, "text",        4) ? TEXT       :
			 !strncmp(yytext, "parse",       5) ? PARSE      :
			 !strncmp(yytext, "keyword",     7) ? KEYWORD    :
			 !strncmp(yytext, "$EXIT",	 5) ? EXIT	 :
			 !strncmp(yytext, "$RETURN",	 7) ? RETURN	 :
			 !strncmp(yytext, "$FAIL",	 5) ? FAIL	 :
			 !strncmp(yytext, "$ERROR",      6) ? ERROR      :
			 				      IDENTIFIER);


[ \t\n]			;
