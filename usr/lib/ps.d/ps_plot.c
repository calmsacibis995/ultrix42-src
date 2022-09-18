#ifndef lint
static char *sccsid = "@(#)ps_plot.c	4.1	ULTRIX	7/2/90";
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
/*	ps_plot - file which outputed using plot print filter		*/
/*                                         for PostScript Printer	*/
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

#include <stdio.h>
#include <math.h>
#include <signal.h>
#define YES 1
#define NO  0
#define PS_EOF  '\004'		/* PostScript End of File */
 
/*#define DEFXSCALE	612.0
#define DEFYSCALE	792.0
#define MARGX		39.0
#define MARGY		20.0
*/
#define FONT        "Times-Roman"
#define PITCH		10
#define LINEWIDTH	.2
 
 
#define FOREVER		1
#define DEGREES		* 180.0 / 3.141592654
#define DOTS		/ scale
#define min(a,b)   ( (a < b) ? a : b )
/*#define MARGSCALE   min(( DEFXSCALE - MARGX )/DEFXSCALE,  \
					    ( DEFYSCALE - MARGY )/DEFYSCALE)
*/
struct {
	int	copies;		/* how many cpies ...                       */
	int	tray;		/* tray 1:Small,2:large,3:Large capacity    */
	int	output;		/* output 1:SideDown,2:SideUp,3:TopDown     */
	int	landscape	/* Landscape ?                              */
	} ps;

char	usage[] = "Valid ps_plot options:\n\t-c[copies]\n\t-t[papertray]\n\t-o[output lacation]\n\t-p (Portrait)\n";

char	*prolog[] = {
#include "plot_pro.h"
	0
	};
main(argc, argv)
int	argc;
char	**argv;
{
double	atof();
int	c;
extern int optind;
extern char *optarg;
int	status = 0;	/* exit status (no. errors occured) */
int x, y, x2, y2, xc, yc;  /* or whatever 16 bits is in your C    */
			   /* if a short int is less than 16 bits */
			   /* this program won't work unless you  */
			   /* change this declaration.            */
float r, ang1, ang2;
float xscale, yscale;
float scale = 1.0;
float newpitch = (float) PITCH;
float newlinew = LINEWIDTH;
char command;
char *dashstyle;
char string [257];
int docore();	/* SEL */
signal(11, docore); 
signal(10, docore);
signal(8, docore);
signal(7, docore);
signal(6, docore);
ps.copies = 1;
ps.tray   = 0;
ps.output = 0;
ps.landscape = YES;
while ((c = getopt (argc, argv, "pc:t:o:")) != EOF)
	switch(c)
	{
		case 'c':	/* how many copies ... */
			ps.copies = atof(optarg);
			break;

		case 't':	/* Papertray by Location... */
			ps.tray = atof(optarg);
			break;

		case 'o':	/* Output location ... */
			ps.output = atof(optarg);
			break;
		case 'p':	/* portrait... */
			ps.landscape = NO;
			break;

		case '?':
			status++;
			break;
	}
if(status)
	fprintf(stderr, usage);
putcomment();
printf ("/@LINEWIDTH %f def\n@LINEWIDTH setlinewidth\n", LINEWIDTH);
printf ("/@FONT (%s) def\n/@PITCH %d def\n",FONT, PITCH);
printf ("@FONT cvn findfont @PITCH scalefont setfont\n");
printf ("/landscape 1 def\n");
printf ("/portrait  2 def\n");
if (ps.landscape)
	fprintf(stdout,"/orientation landscape def\n");
else
	fprintf(stdout,"/orientation portrait def\n");
if (ps.tray > 0)
	fprintf(stdout,"/tray %d def\n",ps.tray);
if (ps.output > 0)
	fprintf(stdout,"/out  %d def\n",ps.output);
ppss(prolog);
initcoords();
 
do {
	command = getchar();
	switch (command)
	{
		default:   /* if invalid command char, skip rest of line */

			/* This is good if you want to put comments into your
			   plot (5) files.  If not, just commenting out the 
			   next line will ignore each invalid character.
			 */

		if (! gets(string)) fin (0);
		break;

		case 'm':   /*   move (x, y)   */
			readswpd (&x, stdin);
			readswpd (&y, stdin);
			printf ("%d %d moveto\n",x, y); 
		break;

		case 'n':   /*   cont (x, y)   */
			readswpd (&x, stdin);
			readswpd (&y, stdin);
			printf ("%d %d lineto\n", x, y);
			printf ("currentpoint   stroke   moveto\n");
		break;
 
		case 'p':   /*   point (x, y)  */
			readswpd (&x, stdin);
			readswpd (&y, stdin);
			printf ("gsave \n");
			printf ("1 setlinecap\n");
			printf ("currentlinewidth 2 mul setlinewidth\n");
			printf ("%d %d moveto\n", x, y);
			printf ("0 0  rlineto\n");
			printf ("stroke\n");
			printf ("grestore\n");
		break;
 
		case 'a':   /*   arc (xc, yc, x, y, x2, y2)   */
			readswpd (&xc, stdin);
			readswpd (&yc, stdin);
			readswpd (&x, stdin);
			readswpd (&y, stdin);
			readswpd (&x2, stdin);
			readswpd (&y2, stdin);
			r = sqrt ((float) ((x-xc) * (x-xc) + (y-yc) * (y-yc)));
			ang1 = atan ( (float) (y-yc) / (float) (x-xc) ) DEGREES;
			ang2 = atan ( (float) (y2-yc) / (float) (x2-xc) ) DEGREES;
			if ((x - xc) < 0) ang1 = 180.0 + ang1;
			if ((x2 - xc) < 0) ang2 = 180.0 + ang2;
			printf ("newpath \n");
			printf ("%d %d %6.3f %6.3f %6.3f arc\n", xc, yc, r, ang1, ang2);
			printf ("currentpoint   stroke   moveto\n");
		break;

		case 'c':   /*   circle (xc, yc, r)   */
			readswpd (&xc, stdin);
			readswpd (&yc, stdin);
			readswpd (&x, stdin);
			printf ("newpath \n");
			printf ("%d %d %d 0 360 arc\n", xc, yc, x);
			printf ("currentpoint   stroke   moveto\n");
		break;

		case 'l':   /*   line (x, y, x2, y2)   */
			readswpd (&x, stdin);
			readswpd (&y, stdin);
			readswpd (&x2, stdin);
			readswpd (&y2, stdin);
			printf ("%d %d moveto\n", x, y); 
			printf ("%d %d lineto\n", x2, y2);
			printf ("currentpoint   stroke   moveto\n");
		break;
 
		case 't':   /*   label ( string )   */
			if (! gets(string) ) fin (0);
			printf ("%6.3f %6.3f rmoveto\n", -newpitch/4, -newpitch/4);
			printf ("\t(%s) show\n", string);
			printf ("%6.3f %6.3f rmoveto\n", newpitch/4, newpitch/4);
		break;
 
		case 'e':    /*  erase ()   */
			printf ("erasepage\n");  /* just erase current page */
		break;

		case 'f':    /*  linemod(dashstyle)   */
			if (! gets(string) ) fin (0);
			dashstyle = string;

			if (strcmp (dashstyle, "dotted") == 0) {
			fprintf(stdout,"[%6.3f %6.3f] 0 setdash\n", 1 DOTS, 4 DOTS);
			}
			else if (strcmp (dashstyle, "solid") == 0) {
			fprintf(stdout,"[] 0 setdash\n");
			}
			else if (strcmp (dashstyle, "longdashed") == 0) {
			fprintf(stdout,"[%6.3f %6.3f] 0 setdash\n", 4 DOTS, 1 DOTS);
			}
			else if (strcmp (dashstyle, "shortdashed") == 0) {
			fprintf(stdout,"[%6.3f %6.3f] 0 setdash\n", 2 DOTS, 3 DOTS);
			}
			else if (strcmp (dashstyle, "dotdashed") == 0) {
			fprintf(stdout,"[%6.3f %6.3f %6.3f %6.3f] 0 setdash\n",5 DOTS, 2 DOTS, 1 DOTS, 2 DOTS);
			}
			else break;   /* ignore any other styles */
		break;

		case 's':   /*   space (x, y, x2, y2)   */
			readswpd (&x, stdin);
			readswpd (&y, stdin);
			readswpd (&x2, stdin);
			readswpd (&y2, stdin);
			fprintf(stdout,"/@x  %d def\n",x);
			fprintf(stdout,"/@y  %d def\n",y);
			fprintf(stdout,"/@x2 %d def\n",x2);
			fprintf(stdout,"/@y2 %d def\n",y2);
			fprintf(stdout,"imheight @x2 @x sub div dup imwidth @y2 @y sub div dup\n");
			fprintf(stdout,"3 -1 roll lt {dup /@scale exdef dup scale pop}\n");
			fprintf(stdout,"             {pop dup /@scale exdef dup scale} ifelse %% min (xsc, ysc)\n");
			fprintf(stdout,"@x neg @y neg translate\n");
			fprintf(stdout,"@LINEWIDTH @scale div setlinewidth\n");
			fprintf(stdout,"@FONT cvn findfont @PITCH @scale div scalefont setfont\n");
		break;
 
	} /* end of switch */
 
} while (FOREVER);
putchar(PS_EOF);
exit(0);
}
 
initcoords ()  /* set up coordinate system default  (before scaling) */
{
fprintf(stdout,"initmatrix\n");
fprintf(stdout,"orientation landscape eq\n");
fprintf(stdout,"{90 rotate 0 paperheight neg translate} if\n");
fprintf(stdout,"hm vm translate \n");
}
 
readswpd (location, stream) 
int *location;
FILE *stream;
{
	int nextch;
 
	nextch = getc(stream);
	*location = getc(stream);
	*location = ((0377 & (*location)) << 8) + (0377 & nextch);
}
 
 
fin (error)  /* end of input file */
int error;
{
	if (! error)  /* flush output at end of file */ {
		printf ("showpage\n");
		printf ("plcntx restore\n");
	}
	else    /* fatal error (none provided in original source) */
		printf ("%% Error in input file: no page produced.\n");
	putchar(PS_EOF);
	exit (error);
}
/************************************************************************/
/*	Put PostScript Sfuff						*/
/************************************************************************/
ppss(stuff)
	char	**stuff;
{
	char	*p;

	while(p = *stuff++)
		fprintf(stdout,"%s\n", p);
}

/************************************************************************/
/*	put PostScript Comment + alpha					*/
/************************************************************************/
putcomment()
{
long time();
char *ctime();
long tmp;
tmp = time(0);
fprintf(stdout,"%%!PS-Adobe-1.0\n");
fprintf(stdout,"%%%%Title: Plot(5)\n");
fprintf(stdout,"%%%%DocumentFonts: %s\n",FONT);
fprintf(stdout,"%%%%Creator: @(#)ps_plot.c	1.1\n");
fprintf(stdout,"%%%%CreationDate: %s",ctime(&tmp));
fprintf(stdout,"%%%%Pages: ?\n");
fprintf(stdout,"%%%%EndComments\n");
fprintf(stdout,"/plcntx save def\n");
fprintf(stdout,"/#copies %d def\n",ps.copies);
}
docore(x)
int x;
{
/*chdir("/usr/staff1/logcher/core_stuff");*/
/*system("echo core dumping | mail logcher");*/
signal(SIGIOT, SIG_DFL);
abort();
}
