# include "stdio.h"
# define U(x) x
# define NLSTATE yyprevious=YYNEWLINE
# define BEGIN yybgin = yysvec + 1 +
# define INITIAL 0
# define YYLERR yysvec
# define YYSTATE (yyestate-yysvec-1)
# define YYOPTIM 1
# define YYLMAX 200
# define output(c) putc(c,yyout)
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
# define unput(c) {yytchar= (c);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;}
# define yymore() (yymorfg=1)
# define ECHO fprintf(yyout, "%s",yytext)
# define REJECT { nstr = yyreject(); goto yyfussy;}
int yyleng; extern char yytext[];
int yymorfg;
extern char *yysptr, yysbuf[];
int yytchar;
FILE *yyin ={stdin}, *yyout ={stdout};
extern int yylineno;
struct yysvf { 
	struct yywork *yystoff;
	struct yysvf *yyother;
	int *yystops;};
struct yysvf *yyestate;
extern struct yysvf yysvec[], *yybgin;
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
# define YYNEWLINE 10
yylex(){
int nstr; extern int yyprevious;
	extern long	yyintval;
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:
		{
			comment:  if (yyinput() == '*')
					if (yyinput() != '/')
						goto comment;
					else ;
				else
					goto comment;
			}
break;
case 2:
	{
			sscanf(yytext, "%o", &yyintval);
			return (INTEGER);
			}
break;
case 3:
	{
			sscanf(yytext+2, "%x", &yyintval);
			return(INTEGER);
			}
break;
case 4:
	{
			sscanf(yytext, "%ld", &yyintval);
			return(INTEGER);
			}
break;
case 5:
		return(COLON);
break;
case 6:
		return(SEMICOLON);
break;
case 7:
		return(PERCENTS);
break;
case 8:
		return(COMMA);
break;
case 9:
		return(BANG);
break;
case 10:
		return(LBRACE);
break;
case 11:
		return(RBRACE);
break;
case 12:
	return
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
break;
case 13:
		;
break;
case -1:
break;
default:
fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */
int yyvstop[] ={
0,

13,
0,

9,
0,

12,
0,

4,
0,

8,
0,

4,
0,

5,
0,

6,
0,

10,
0,

11,
0,

7,
0,

1,
0,

2,
4,
0,

3,
0,
0};
# define YYTYPE char
struct yywork { YYTYPE verify, advance; } yycrank[] ={
0,0,	0,0,	1,0,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,3,	0,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	0,0,	1,4,	1,0,	
1,0,	1,5,	1,6,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,7,	1,8,	0,0,	1,0,	
1,9,	1,10,	1,7,	1,7,	
1,7,	1,7,	1,7,	1,7,	
1,7,	1,7,	6,15,	1,11,	
1,12,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,5,	7,7,	
9,16,	7,7,	0,0,	0,0,	
7,7,	7,7,	7,7,	7,7,	
7,7,	7,7,	7,7,	7,7,	
7,7,	7,7,	17,17,	17,17,	
17,17,	17,17,	17,17,	17,17,	
17,17,	17,17,	0,0,	0,0,	
1,0,	1,0,	1,0,	1,0,	
0,0,	1,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
1,13,	1,0,	1,14,	1,0,	
1,0,	2,0,	2,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
2,0,	0,0,	0,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
0,0,	2,4,	2,0,	2,0,	
0,0,	2,6,	2,0,	2,0,	
2,0,	2,0,	2,0,	0,0,	
2,8,	0,0,	2,0,	2,9,	
0,0,	2,7,	2,7,	2,7,	
2,7,	2,7,	2,7,	2,7,	
0,0,	0,0,	2,11,	2,12,	
2,0,	2,0,	2,0,	2,0,	
2,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	10,17,	10,17,	
10,17,	10,17,	10,17,	10,17,	
10,17,	10,17,	0,0,	2,0,	
2,0,	2,0,	2,0,	0,0,	
2,0,	0,0,	0,0,	5,5,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	0,0,	10,18,	2,13,	
2,0,	2,14,	2,0,	2,0,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	10,18,	0,0,	
0,0,	0,0,	5,5,	0,0,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	5,5,	5,5,	
5,5,	5,5,	18,19,	18,19,	
18,19,	18,19,	18,19,	18,19,	
18,19,	18,19,	18,19,	18,19,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	18,19,	
18,19,	18,19,	18,19,	18,19,	
18,19,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	18,19,	
18,19,	18,19,	18,19,	18,19,	
18,19,	0,0,	0,0,	0,0,	
0,0};
struct yysvf yysvec[] ={
0,	0,	0,
yycrank+-1,	0,		0,	
yycrank+-128,	yysvec+1,	0,	
yycrank+0,	0,		yyvstop+1,
yycrank+0,	0,		yyvstop+3,
yycrank+191,	0,		yyvstop+5,
yycrank+21,	0,		0,	
yycrank+24,	0,		yyvstop+7,
yycrank+0,	0,		yyvstop+9,
yycrank+26,	0,		0,	
yycrank+162,	yysvec+7,	yyvstop+11,
yycrank+0,	0,		yyvstop+13,
yycrank+0,	0,		yyvstop+15,
yycrank+0,	0,		yyvstop+17,
yycrank+0,	0,		yyvstop+19,
yycrank+0,	0,		yyvstop+21,
yycrank+0,	0,		yyvstop+23,
yycrank+34,	yysvec+7,	yyvstop+25,
yycrank+266,	0,		0,	
yycrank+0,	yysvec+18,	yyvstop+28,
0,	0,	0};
struct yywork *yytop = yycrank+368;
struct yysvf *yybgin = yysvec+1;
char yymatch[] ={
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,011 ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
011 ,01  ,01  ,01  ,'$' ,01  ,01  ,01  ,
01  ,01  ,01  ,'+' ,01  ,'+' ,01  ,01  ,
'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,
'8' ,'8' ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'$' ,
'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,
'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,
'$' ,'$' ,'$' ,01  ,01  ,01  ,01  ,'$' ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'$' ,
'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,
'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,'$' ,
'$' ,'$' ,'$' ,01  ,01  ,01  ,01  ,01  ,
0};
char yyextra[] ={
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0};
/*	ncform	4.1	83/08/11	*/

int yylineno =1;
# define YYU(x) x
# define NLSTATE yyprevious=YYNEWLINE
char yytext[YYLMAX];
struct yysvf *yylstate [YYLMAX], **yylsp, **yyolsp;
char yysbuf[YYLMAX];
char *yysptr = yysbuf;
int *yyfnd;
extern struct yysvf *yyestate;
int yyprevious = YYNEWLINE;
yylook(){
	register struct yysvf *yystate, **lsp;
	register struct yywork *yyt;
	struct yysvf *yyz;
	int yych;
	struct yywork *yyr;
# ifdef LEXDEBUG
	int debug;
# endif
	char *yylastch;
	/* start off machines */
# ifdef LEXDEBUG
	debug = 0;
# endif
	if (!yymorfg)
		yylastch = yytext;
	else {
		yymorfg=0;
		yylastch = yytext+yyleng;
		}
	for(;;){
		lsp = yylstate;
		yyestate = yystate = yybgin;
		if (yyprevious==YYNEWLINE) yystate++;
		for (;;){
# ifdef LEXDEBUG
			if(debug)fprintf(yyout,"state %d\n",yystate-yysvec-1);
# endif
			yyt = yystate->yystoff;
			if(yyt == yycrank){		/* may not be any transitions */
				yyz = yystate->yyother;
				if(yyz == 0)break;
				if(yyz->yystoff == yycrank)break;
				}
			*yylastch++ = yych = input();
		tryagain:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"char ");
				allprint(yych);
				putchar('\n');
				}
# endif
			yyr = yyt;
			if ( (int)yyt > (int)yycrank){
				yyt = yyr + yych;
				if (yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
# ifdef YYOPTIM
			else if((int)yyt < (int)yycrank) {		/* r < yycrank */
				yyt = yyr = yycrank+(yycrank-yyt);
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"compressed state\n");
# endif
				yyt = yyt + yych;
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				yyt = yyr + YYU(yymatch[yych]);
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"try fall back character ");
					allprint(YYU(yymatch[yych]));
					putchar('\n');
					}
# endif
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transition */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
			if ((yystate = yystate->yyother) && (yyt= yystate->yystoff) != yycrank){
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"fall back to state %d\n",yystate-yysvec-1);
# endif
				goto tryagain;
				}
# endif
			else
				{unput(*--yylastch);break;}
		contin:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"state %d char ",yystate-yysvec-1);
				allprint(yych);
				putchar('\n');
				}
# endif
			;
			}
# ifdef LEXDEBUG
		if(debug){
			fprintf(yyout,"stopped at %d with ",*(lsp-1)-yysvec-1);
			allprint(yych);
			putchar('\n');
			}
# endif
		while (lsp-- > yylstate){
			*yylastch-- = 0;
			if (*lsp != 0 && (yyfnd= (*lsp)->yystops) && *yyfnd > 0){
				yyolsp = lsp;
				if(yyextra[*yyfnd]){		/* must backup */
					while(yyback((*lsp)->yystops,-*yyfnd) != 1 && lsp > yylstate){
						lsp--;
						unput(*yylastch--);
						}
					}
				yyprevious = YYU(*yylastch);
				yylsp = lsp;
				yyleng = yylastch-yytext+1;
				yytext[yyleng] = 0;
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"\nmatch ");
					sprint(yytext);
					fprintf(yyout," action %d\n",*yyfnd);
					}
# endif
				return(*yyfnd++);
				}
			unput(*yylastch);
			}
		if (yytext[0] == 0  /* && feof(yyin) */)
			{
			yysptr=yysbuf;
			return(0);
			}
		yyprevious = yytext[0] = input();
		if (yyprevious>0)
			output(yyprevious);
		yylastch=yytext;
# ifdef LEXDEBUG
		if(debug)putchar('\n');
# endif
		}
	}
yyback(p, m)
	int *p;
{
if (p==0) return(0);
while (*p)
	{
	if (*p++ == m)
		return(1);
	}
return(0);
}
	/* the following are only used in the lex library */
yyinput(){
	return(input());
	}
yyoutput(c)
  int c; {
	output(c);
	}
yyunput(c)
   int c; {
	unput(c);
	}
