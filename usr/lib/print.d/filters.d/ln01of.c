#ifndef lint
static char *sccsid = "@(#)ln01of.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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

/*  	LN01 NROFF FILTER
 * 	filter which reads the output of nroff and converts it to
 *	LN01 compatible image.  Since nroff only deals with monospaced
 *	fonts this limits the number of fonts that will work affectively.
 *	The fonts and font selection files are in /usr/lib/font/devln01/
 *	This filter will pass through any escape or control sequences.
 *	Underlining has been implemented using the escape sequences so
 *	that if at some time proportional fonts are used (ever so ragged)
 *	they will be properly underlined.  
 */

#include <stdio.h>
#include <signal.h>
#define	MAXWIDTH 132    /* maximum width in landscape mode */
#define psiz 50		/* 50 pixels per line in portrait mode */
#define lsiz 35		/* 35 pixels per line in landscape mode   */
#define fontdir "/usr/lib/font/devln01/" /* font directory */

int	lineno;
int	width = 80;	/* default line length */
int	length = 66;	/* page length */
int	indent = 0;		/* indentation length */
int	npages = 1;
int	literal;	/* print control characters */
/********************************************************/
/*	added for escape sequence ignoring for ln01	*/
/*							*/
#define ESC		'\033'  /* escape sequence introducer */
#define BSLH		'\134'  /* back slash */
#define UCP		'\120'  /* uppercase P */
#define UCF		'\106'  /* uppercase F for internal font loads */
#define escend(x)	((x!='\120')&&(x!='\133')&&(x>='\100')&&(x<='\176'))
int	escflg=0;		/* escape sequence flag = 1 in progress */
char	filnam[20];	/* variables used in loading fonts */
int	lstchr,filcnt,fnmind;
int	proname;
/********************************************************/
char	*name;		/* user's login name */
char	*host;		/* user's machine name */
char	*acctfile;	/* accounting information file */
main(argc, argv) 
	int argc;
	char *argv[];
{
	register int i, col;
	register char *cp;
	FILE *p = stdin, *o = stdout;
	int nlines=0,npages=0;
	int lund,ch,undflg,last;
	if(argv[0]=="of")
		proname=0;
	else	proname=1;
	while (--argc) {
		if (*(cp = *++argv) == '-') {
			switch (cp[1]) {
			case 'n':	/*      collect login name   	*/
				argc--;
				name = *++argv;
				break;

			case 'h':	/*	collect host name	*/
				argc--;
				host = *++argv;
				break;

			case 'w':	/*	collect page width	*/
				if ((i = atoi(&cp[2])) > 0 && i <= MAXWIDTH)
					width = i;
				break;

			case 'l':	/*	collect page length	*/
				length = atoi(&cp[2]);
				break;

			case 'i':	/*	collect indent		*/
				indent = atoi(&cp[2]);
				break;

			case 'c':	/* Print control chars */
				literal++;
				break;
			}
		} else
			acctfile = cp;
	}
/*								      */
/*	attempt load of default fonts if not acting as ditroff filter */
/*								      */
	fprintf(o,"\033\143");	/* reset printer */
	sleep(2);		/*allow some time*/
	lodfnt(o,width); /* go load nroff setup fonts for */   
	setsiz(o,length,width); /* set page size */
	col = indent;
	last=0;	 	/* last character processed	*/
	lund=0;		/* flag was last char underlined (needed for tbl bug)*/
	undflg=0;	/* is underlining in progress   */
	escflg=0;	/* is escape/control sequence in progress */
	while((ch=getc(p)) != EOF)
		{
		/*	Escape sequence pass through code     */
		if (((escflg==0) && (ch == ESC)) || escflg)
			{
			if(last=='_')
				{
				putc('_',o);
				last=0;
				}
			if(undflg)
				{
				undflg=0;
				lund=0;
				fprintf(o,"\033\1330m");
				}
			col=indent; /* assume ditroff in control of chrs/line */
			eschdl(o,ch);
			}
		else
			{
			if((last=='_')&&(ch!='\b'))
				{
				putc('_',o);
				undflg=0;
				col++;
				}
			switch (ch) 
			{
			case '\f':	/* new page on form feed */
				npages++;
			case '\n':	/* new line */
				if(ch=='\f')
					nlines=0;
				else
					nlines++;
				col=0;
				if(undflg)
					{
					undflg=0;
					lund=0;
					fprintf(o,"\033\1330m");
					}
				putc(ch,o);
				break;
			case '\b':	/* backspace */
				if(last=='_')
					{
					undflg=1;
					fprintf(o,"\177\033\1334m");
					}
				else	if(lund)
						lund++;
					else	putc(ch,o);
				break;
			case '\r':	/* carriage return */
				col = indent;
				if(undflg)
					{
					undflg=0;
					lund=0;
					fprintf(o,"\033\1330m");
					}
				putc(ch,o);
				for(i=0;i != indent;i++,putc(' ',o));
				break;
			case '_':
				break;
			case '\t':	/* tab it default is tab/8char*/
				if(undflg)
					{
					undflg=0;
					lund=0;
					fprintf(o,"\033\1330m");
					}
				for(col++;(col % 8) != 0;col++);
				putc(ch,o);
				break;

			case '\031':	/* end media */
				/*
				 * lpd needs to use a different filter to
				 * print data so stop what we are doing and
				 * wait for lpd to restart us.
				 */
				if(undflg)
					{
					undflg=0;
					fprintf(o,"\033\1330m");
					}
				if ((ch = getchar()) == '\1') {
					fflush(o);
					kill(getpid(), SIGSTOP);
					break;
				} else {
					ungetc(ch, stdin);
					ch = '\031';
				}
			default:	/* all else */
				if (col < width)
					col++;
				else
					{
					undflg=0;
					fprintf(o,"\033\1330m");
					col=indent;
					putc('\r',o);
					putc('\n',o);
					for(i=0;i != indent;i++,putc(' ',o));
					nlines++;
					}
				if(lund<=1)
					{
					if(ch=='1' && width <= 80 && proname) /* OCRB better be there */
						fprintf(o,"\033[12m1\033[11m");
					else	putc(ch,o);
					}
				lund=0;
				if(undflg)
					{
					undflg=0;
					fprintf(o,"\033\1330m");
					lund=1;
					}
			}
			last=ch;
			}
		}
	fprintf(o,"\014\033\143");	/* send FF + reset printer */
	sleep(2);			/* allow some time */
	if (name && acctfile && access(acctfile, 02) >= 0 &&
	    freopen(acctfile, "a", o) != NULL) {
		printf("%7.2f\t%s:%s\n", (float)npages, host, name);
	}
	exit(0);
}
/****************************************************************/
/*								*/
/*	eschdl - escape sequence handler			*/
/*								*/
/*      This routine intercepts escape sequences for the purpose*/
/*	of pass through.					*/
/*								*/
/*	This routine also intercepts inyternal font load sequences */
/*	the sequence format is as follows:			*/
/*	ESC UCF fontfile;fontfile;...fontfile BSLH		*/
/*      fonts are assumed to be in loadable form in fontdir	*/
/****************************************************************/
eschdl(o,c)
int c;
FILE  *o;
{
FILE  *fnt;
char	fntfil[50];	/* string for font filename*/
char	filnam[20];
int	lstchr,filcnt,fnmind;
int j,chr;
if(escflg==0)
	{		/* set escflg=1 => ready to receive 2nd seqchar*/
	escflg=1;
	}
else	switch(escflg)
		{
		case 1:		/* second character of escseq 		*/
			switch(c)
				{
  				case UCP:
					escflg=2; /*ctrl str pass thru mode=8 */
					lstchr=c;
					putc(ESC,o);
					putc(c,o);
					break;
				case UCF:
					escflg=4;  /* set load font case */
					fnmind=0;
					filcnt=0;
					break;
				default:
					escflg=3;  /* set seq pass thru mode*/
					putc(ESC,o);
					putc(c,o);
					break;
				}
			break;
		case 2:		/* ctrl string pass through mode       	*/
			if((lstchr==ESC) && (c==BSLH))
				{
				escflg=0;
				lstchr=0;
				}
			else lstchr=c;	/* save it for next pass */
			putc(c,o);
			break;
		case 3:
			if(escend(c))
				escflg=0;/* turn off esc handler if at end  */
			putc(c,o);
			break;
		case 4:
			switch(c)
				{
				case BSLH:
				case ';':
					if(filcnt==0)
						{
						fprintf(o,"\033P1;1y");
						filcnt++;
						}
					filnam[fnmind]='\0';
					fnmind=0;
					sprintf(fntfil,"%s%s",fontdir,filnam);
					if((fnt=fopen(fntfil,"r")) == NULL)
						break;
					while((chr=getc(fnt)) != EOF)
						putc(chr,o);
					fclose(fnt);
					if(c==BSLH)
						{
						escflg=0;
						fprintf(o,"; \033\134");
						}
					break;
				default:
					filnam[fnmind++]=c;
				}
		}
return(0);
}
/************************************************************************/
/*									*/
/*	setsiz - set page size						*/
/*									*/
/*	assume constant width characters				*/
/************************************************************************/
setsiz(o,len,wid)
FILE	*o;
int len,wid;
{
int plen,pstr;
if(wid>80)
	{			/* landscape mode	*/
	plen=(71*lsiz)+1;
	pstr=(5*lsiz)+1;
	}
else	
	{			/* portrait mode	*/
	plen=(len*psiz)+1;
	pstr=1;
	}
fprintf(o,"\033\1333300t");	/* set full size page	*/
fprintf(o,"\033\133%d;%dr",pstr,plen);/* set margins within page*/
fprintf(o,"\033\1331d");
return(0);
}
/************************************************************************/
/*									*/
/*	   FONT LOADING PROGRAM						*/
/*									*/
/************************************************************************/
lodfnt(o,wide)
FILE	*o;
int wide;	/* orientation of the page is dependant on page width */
{		/* 0-80 => portrait mode  81-132 => landscape mode    */
FILE	*list;
FILE	*fnt;
char	*escstr;
char	*font;
char	*ptr;
char	*lisnam;
char *nam[4];
int	j,nflg,c,i,k;
nam[1]="\033P1;12}DETitan10-R         \033\134"; /* R - roman font   */
nam[2]="\033P1;13}DETitan10-R         \033\134"; /* I - italic font  */
nam[3]="\033P1;14}DETitan10-R         \033\134"; /* B - bold font    */
nam[4]="\033P1;15}DETitan10-R         \033\134"; /* S - special font */
font="                                            ";
if(wide<=80)
	lisnam="/usr/lib/font/devln01/nroff.pfnts";
else lisnam="/usr/lib/font/devln01/nroff.lfnts";
if(((list=fopen(lisnam,"r")) == NULL) || proname==0)
	{
	if(wide>80)
		escstr= "\033[10m";	/* invoke landscape selection first */
	else    escstr= "\033[11m";	/* invoke portrait  selection first */
	for(i=0;escstr[i] != '\0';putc(escstr[i++],o));
	if(list!=NULL)
		fclose(list);
	proname=0;	/* set no fonts loaded indicator */
	return(0);
	}
/****************************************/
/*	copy out fonts 			*/
/****************************************/
k=0;
i=0;
nflg=0;
while ((c=getc(list)) != EOF)
	if(k<=4)
	switch(c)
	{
		case '\n':	/* end of font filename = endline */
			if(nflg!=2)
				break;
			font[i++]='\0';
			i=0;
			nflg=0;
			if((fnt=fopen(font,"r")) == NULL)
				{
				if(k==0)
					proname=0;/* assume no fonts loaded */
				break;
				}
			if(k==1)
				{
				escstr="\033P1;1y";
				for(i=0;escstr[i] != '\0';putc(escstr[i++],o));
				}
			while((c=getc(fnt)) != EOF)
				putc(c,o);
			fclose(fnt);
			break;
		case ' ':  /* fontfilename space font internal name */
			if((nflg==1) && (k<4))
				{
				nflg=2;
				k++;
				ptr=nam[k];
				ptr+=7;
				}
			break;
		default:
			if(nflg==2)
				{
				*ptr=c;
				ptr++;
				}
			else 	{
				font[i++]=c;
				nflg=1;
				}
	}
fclose(list);
if(k!=0)
	{
	proname=1;
	escstr="; \033\134";
	for(i=0;escstr[i] != '\0';putc(escstr[i++],o));
	k++;
	while(k--)
		for(i=0;nam[k] [i] != '\0';putc(nam[k] [i++],o));
	escstr= "\033[11m";	/* invoke roman font selection first */
	for(i=0;escstr[i] != '\0';putc(escstr[i++],o));
	}
else
	{
	if(wide>80)
		escstr= "\033[10m";	/* invoke landscape selection first */
	else    escstr= "\033[11m";	/* invoke portrait  selection first */
	for(i=0;escstr[i] != '\0';putc(escstr[i++],o));
	}
return(0);
}
