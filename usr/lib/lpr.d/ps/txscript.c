#ifndef lint
static char *sccsid = "@(#)txscript.c	4.1	ULTRIX	11/23/87";
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
 *			Modification History				*
 ************************************************************************/
/************************************************************************/
/*									*/
/*	txscript - image file print filter for PostScript Printer	*/
/*									*/
/*	description: This routine prints the specified Ascii file on 	*/
/*			PostScript printer.             		*/
/*									*/
/*      PostScript is a trademark of Adobe Systems Incorporated.        */
/*	LaserWriter is a trademark of Apple computer, Inc.              */
/*	Apple is a registerd trademark of Apple computer, Inc.          */
/*	Times, Helvetica, Palatino, and Optima registerd trademarks of  */
/*	Allied Corporation. ITC Avant Garde Gothic, ITC Souvenir,       */
/*	ITC Lubalin Graph, and ITC Garamond are registerd trademarks    */
/*	of International Typeface Corporation.                          */
/*				                                        */
/************************************************************************/

 
#include	<stdio.h>
#include	<strings.h>
#include	<sys/signal.h>
 
#define	INCH		72.0		/* no. postscript units / inch */
#define	CM		28.35		/* postscript units / cm */
#define	PAGEOFFSET	(1.0*CM)	/* default page offset for -o arg. */
#define	FONTSIZE	12.0		/* default font size (in points) */
#define	TABSIZE		8		/* default tab size */
#define	ROTATION	0.0		/* default orientation */
#define	FONT		"Times-Roman"	/* default font */
					
#define NEW_PAGE	014		/* ^L forces a new page */
#define PAGEWIDTH	(14.0*CM)	/* Width of one page (in 2 page mode)#*/
#define PAGE_LINE	66		/* default number of lines per page #*/
 
#define	TRUE		1
#define	FALSE		0
 
/* typedef char	int; */
 
char	usage[] = "Valid txscript options:\n\t-O[offset]\n\t-r[rotation]\n\t-s[fontsize]\n\t-T[tabsize]\n\t-p[pitch]\n\t-f[font]\n\t-h[horizon space]\n\t-m\n\t-2\n\t-l[page length]\n\t-t[input tray]\n\t-o[output tray]\n";
int	tabsize;		/* in character positions */
int	linecount,		/* current line number #*/
	twopage,		/* two page mode and odd/even flag #*/
	pageline,		/* number of line per page #*/
	otray,			/* Output tray */
	itray; 			/* Input tray  */
char	*progname;

main(argc, argv)
int	argc;
char	**argv;
{
	int	status = 0;	/* exit status (no. errors occured) */
	int	mflag = 0;	/* Multi font flag #*/
	double	atof();		/* add for ULTRIX-32/32m #*/
	float	pageoffset,
		fontsize,
		linepitch,
		spacing,
		rotation;
	char	*fontname;
	FILE	*istr;
 
	fontsize = FONTSIZE;
	pageoffset = PAGEOFFSET;
	spacing = 0.0;
	tabsize = TABSIZE;
	rotation = ROTATION;
	fontname = FONT;
	otray    = 0;
	itray    = 0;
	linecount = 1;		/*#*/
	twopage = 0;		/*#*/
	pageline = PAGE_LINE;	/*#*/
	progname = *argv;
	argv++;		/* skip program name */
	while(*argv && **argv == '-')
	{
		char	c;
 
		(*argv)++;	/* skip the '-' */
		c = **argv;	/* option letter */
		(*argv)++;	/* skip option letter */
		switch(c)
		{
			case 'O':	/* offset */
				if(**argv == '\0')
					pageoffset = PAGEOFFSET;
				else
					pageoffset = atof(*argv) * CM;
				break;
 
			case 'r':	/* rotation */
				if(**argv == '\0')
					rotation = 90.0;
				else
					rotation = atof(*argv);
				break;

			case 't':	/* input tray */
				itray = atof(*argv);
				break;

			case 'o':	/* input tray */
				otray = atof(*argv);
				break;
 
			case 'p':	/* pitch (line spacing) */
				linepitch = atof(*argv);
				break;
 
			case 's':	/* font size */
				if(**argv == '\0')
					fontsize = 12.0;
				else
					fontsize = atof(*argv);
				break;
 
			case 'T':	/* tab size */
				if(**argv == '\0')
					tabsize = 4;
				else
					tabsize = (int) atof(*argv);
				break;
 
			case 'f':	/* font */
				if(**argv == '\0')
					fontname = "Times-Roman";
				else
					fontname = *argv;
				break;
 
			case 'h':	/* horizontal spacing */
				if(**argv == '\0')
					spacing = 0.25;
				else
					spacing = atof(*argv);
				break;
 
			case 'm':	/* Multi-font #*/
				mflag = 1;
				break;
 
			case '2':	/* two page format mode #*/
				twopage = 1;
				break;
 
			case 'l':	/* number of lines per page */
				if(**argv == '\0')
					pageline = 65;
				else
					pageline = (int) atof(*argv);
				break;
 
			default:
				fprintf(stderr, "%s: illegal option -- %c\n", progname, c);
				status++;
				break;
		}
		argv++;
	}
	if(status) {
		fprintf(stderr, usage);
		status = 0;
	}
	if(linepitch == 0)
		linepitch = fontsize + 2;
	spacing *= fontsize;
	init(fontsize, pageoffset, linepitch, rotation, fontname, spacing, mflag,itray,otray);
	if(! *argv)
		process(stdin);
	else while(*argv)
	{
		if((istr = fopen(*argv, "r")) == NULL)
		{
			perror(*argv);
			status++;
		}
		else
		{
			process(istr);
			fclose(istr);
		}
		argv++;
	}
	exit(status);
}
 
process(istr)
FILE	*istr;
{
	register char	ch;
	register int	x;	/* used for tab calculations */
 
	x = 0;
	putc('(', stdout);
	while((ch=getc(istr)) != EOF)
	{
		if (ch == '\031')
			/*
			 * lpd needs to use a different filter to
			 * print data so stop what we are doing and
			 * wait for lpd to restart us.
			 */
			if ((ch = getc(istr)) == '\1') {
				fflush(stdout);
				fprintf(stderr, "%s: Killing output filter\n", progname);
				kill(getpid(), SIGKILL);
				break;
			} else {
				ungetc(ch, stdin);
				ch = '\031';
			}
		if(ch < ' ' && ch != '\t' && ch != '\n' && ch != '\r' && ch != NEW_PAGE)
		{
			ch = '?';
		}
		if(ch == '\t')
		{
			int	n = x + tabsize - (x % tabsize);
 
			while(x++ < n)
				pch(' ');
			x--;
		}
		else if(ch == '\n')
		{
			x = 0;
			if(twopage) {			/*#*/
			    if(linecount++ == pageline)
				nextpage();
			    else
				fprintf(stdout, ") m\n");
			}
			else				/*#*/
			    fprintf(stdout, ") n\n");
			putc('(', stdout);
		}
		else if(ch == '\r')
		{
			x = 0;
			fprintf(stdout, ") r\n");
			putc('(', stdout);
		}
		else if(ch == NEW_PAGE)
		{
			x = 0;
			if(twopage)			/*#*/
			    nextpage();
			else				/*#*/
			    fprintf(stdout, ") r p\n");
			putc('(', stdout);
		}
		else
		{
			pch(ch);
			x++;
		}
	}
	if(twopage) {				/*#*/
	    if(twopage == 1) {
		nextpage();
		putc('(', stdout);
	    }
	    nextpage();
	    linecount = twopage = 1;
	}
	else					/*#*/
	    fprintf(stdout, ") r p\n");
}
 
char	*inittab[] = {
	/* print a page and start a new one */
	"/p { gsave showpage grestore newpath 0 pgtop moveto } def",
	"/n",
	/* show the string given as an arg */
	"{ spacing 0 3 -1 roll ashow",
	/* now move down a line; linepitch is -'ve */
	"  0 linepitch rmoveto",
	/* save the new y posn */
	"  /y currentpoint exch pop def",
	/* test if the next line would be below the bottom margin */
	"  y 0.25 inch lt",
	/* if so, print the page, and move to the top of a new page */
	"  { p }",
	/* else go to where the next line is due to start */
	"  { 0 y moveto } ifelse",
	"} def",
	"/r",
	/* show the string given as an arg */
	"{ spacing 0 3 -1 roll ashow",
	/* save y */
	"  /y currentpoint exch pop def",
	/* and then move to the beginning of the current line */
	"  0 y moveto",
	"} def",
	(char *)0 };
 
char	*inittab2[] = {						/*#*/
	/* print a page and start a new one */
	"/p { gsave showpage grestore newpath 0 pgtop moveto } def",
	"/r",
	/* show the string given as an arg */
	"{ spacing 0 3 -1 roll ashow",
	/* save y */
	"  /y currentpoint exch pop def",
	/* and then move to the beginning of the current line */
	"  0 y moveto",
	"} def",
	"/h",
	/* show the string given as an arg */
	"{ spacing 0 3 -1 roll ashow",
	/* save y */
	"  /y currentpoint exch pop def",
	/* and then move to the beginning of the home position */
	"  0 pgtop moveto",
	"} def",
	"/m",
	/* show the string given as an arg */
	"{ spacing 0 3 -1 roll ashow",
	/* now move down a line; linepitch is -'ve */
	"  0 linepitch rmoveto",
	/* save the new y posn */
	"  /y currentpoint exch pop def",
	/* and then move to the beginning of the current line */
	"  0 y moveto",
	"} def",
	(char *)0 };						/*#*/
 
init(fontsize, pageoffset, linepitch, rotation, fontname, spacing, mflag, itray, otray)
int	mflag,
	itray,
	otray;
float	fontsize,
	pageoffset,
	linepitch,
	spacing,
	rotation;
char	*fontname;
{
	register char	**p;
	long time();
	char *ctime();
	long tmp;
	tmp = time(0);
	fprintf(stdout,"%%!PS-Adobe-1.0\n");
	fprintf(stdout,"%%%%Title: Simple Text\n");
	fprintf(stdout,"%%%%DocumentFonts: %s\n",fontname);
	fprintf(stdout,"%%%%Creator: @(#)txscript.c	1.4\n");
	fprintf(stdout,"%%%%CreationDate: %s",ctime(&tmp));
	fprintf(stdout,"%%%%Pages: \n");
	fprintf(stdout,"%%%%EndComments\n");
	fprintf(stdout,"/inch {72 mul} def\n");
	fprintf(stdout,"%%\n");
	fprintf(stdout,"%% Input output tray\n");
	fprintf(stdout,"%%\n");
	fprintf(stdout,"/tray-control\n");
	fprintf(stdout,"{\n");
	fprintf(stdout,"currentdict /tray known\n");
	fprintf(stdout,"	{\n");
	fprintf(stdout,"	statusdict /setpapertray known\n");
	fprintf(stdout,"		{\n");
	fprintf(stdout,"		tray statusdict begin setpapertray end\n");
	fprintf(stdout,"		} if\n");
	fprintf(stdout,"	} if\n");
	fprintf(stdout,"currentdict /out known\n");
	fprintf(stdout,"	{\n");
	fprintf(stdout,"	statusdict /setoutputtray known\n");
	fprintf(stdout,"		{\n");
	fprintf(stdout,"		out statusdict begin setoutputtray end\n");
	fprintf(stdout,"		} if\n");
	fprintf(stdout,"	} if\n");
	fprintf(stdout,"} def\n");
 
	if(itray > 0)
	    fprintf(stdout,"/tray %d def\n",itray);
	if(otray > 0)
	    fprintf(stdout,"/out %d def\n",otray);
	if(twopage)					/*#*/
	    p = inittab2;
	else						/*#*/
	    p = inittab;
	while(*p)
		fprintf(stdout, "%s\n", *p++);
	if (! mflag)						/*#*/
	    fprintf(stdout, "/%s findfont %.1f scalefont setfont\n",
			fontname, fontsize);
	else {
	    fprintf(stdout, "/cfont1\n");
	    fprintf(stdout, "  /Courier-Oblique findfont %.1f scalefont def\n",fontsize);
	    fprintf(stdout, "/cfont2\n");
	    fprintf(stdout, "  /Courier-BoldOblique findfont %.1f scalefont def\n",fontsize);
	    fprintf(stdout, "/cfont3\n");
	    fprintf(stdout, "  /Courier-Bold findfont %.1f scalefont def\n",fontsize);
	    fprintf(stdout, "/cfont4\n");
	    fprintf(stdout, "  /Courier findfont %.1f scalefont def\n",fontsize);
	    fprintf(stdout, "/cfont5\n");
	    fprintf(stdout, "  /Courier-BoldOblique findfont %.1f scalefont def\n",fontsize*1.5);
	}							/*#*/
	fprintf(stdout, "/linepitch %.1f def\n", -linepitch);
	fprintf(stdout, "/spacing %.1f def\n", spacing);
	fprintf(stdout,"tray-control\n");
	/* apply rotation transformation, if any */
	if(rotation != 0.0)
		fprintf(stdout, "%.1f rotate\n", rotation);
	/* get current imageable area */
	fprintf(stdout, "clippath pathbbox\n");
	/* save the upper right y coordinate */
	fprintf(stdout, "0.25 inch sub /pgtop exch def\n");
	/* save lower left y; translate origin to lower left */
	fprintf(stdout, "pop /y exch def y translate\n");
	/* subtract old lower left from upper right to get top of page */
	/* then subtract linespacing (add -'ve) to get top text row */
	fprintf(stdout, "/pgtop pgtop y sub linepitch add def\n");
	/* apply horizontal offset, if any */
	/* unfortunately, a slight fudge factor is required here */
	fprintf(stdout, "%.1f 0 translate\n", pageoffset + 4);
	/* move to top of page, ready to start printing */
	fprintf(stdout, "newpath 0 pgtop moveto\n");
	if (mflag)
		fprintf(stdout, "cfont1 setfont\n");
}
 
pch(ch)
int	ch;
{
	if(ch < ' ' || ch > '~')
		fprintf(stdout, "\\%3.3o", ch);
	else
	{
		if(ch == '(' || ch == ')' || ch == '\\')
			putc('\\', stdout);
		putc(ch, stdout);
	}
}

nextpage()							/*#*/
{
	if(twopage == 1) {
	    fprintf(stdout, ") h\n");
	    fprintf(stdout, "%.1f 0 translate\n() r\n", PAGEWIDTH);
	}
	else {
	    fprintf(stdout, ") r p\n");
	    fprintf(stdout, "-%.1f 0 translate\n() r\n", PAGEWIDTH);
	}
	linecount = 1;
	twopage ^= 3;
}								/*#*/
