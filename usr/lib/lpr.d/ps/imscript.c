#ifndef lint
static char *sccsid = "@(#)imscript.c	4.1	ULTRIX	11/23/87";
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
/*	imscript - image file print filter for PostScript Printer	*/
/*									*/
/*	description: This routine prints the specified image file on 	*/
/*			PostScript printer using image function		*/
/*		NOTE: only QVSS and QDSS image formats are supported    */
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
#include <imghdr.h>
#define YES 1
#define NO  0

struct {
	int	landscape;	/* Page format (Landscape or portlate)      */
	int	hdrinf;		/* print header information                 */
	int	hex;		/* Output format (hex->RS232C,bin->ps)      */
	int	copies;		/* how many cpies ...                       */
	int	tray;		/* tray 1:Small,2:large,3:Large capacity    */
	int	output;		/* output 1:SideDown,2:SideUp,3:TopDown     */
	int	hspec;		/* Flag for -h option                       */
	int	wspec;		/* Flag for -w option                       */
	int	xspec;		/* Flag for -x option                       */
	int	yspec;		/* Flag for -y option                       */
	float	height;		/* height of image square in inch           */
	float	width;		/* width of image square in inch            */
	float	left;		/* left position (Inch from edge of paper)  */
	float	top;		/* top position (Inch from edge of paper)   */
	} ps;
struct imghdr im;
char	*imgptr;	/* image buffer pointer*/
short	*cmpptr;	/* colormap buffer pointer*/
unsigned  size;
int k,i,c;

char	usage[] = "Valid imscript options:\n\t-H Output Hex format (default: Binary)\n\t-l Landscape (default: portrait)\n\t-w[width]\n\t-h[height]\n\t-c[copies]\n\t-x[left position of image(inch)]\n\t-y[top position of image(inch)]\n\t-t[papertray]\n\t-o[output lacation]\n\t-i (Print header information)\n";

char	*prolog[] = {
#include "improlog.h"
	0
	};

char	hextbl[] = "0123456789abcdef";

main(argc, argv)
int	argc;
char	**argv;
{
double	atof();
int	c;
extern int optind;
extern char *optarg;
int	status = 0;	/* exit status (no. errors occured) */

init_ps();	/* init paramater    */
while ((c = getopt (argc, argv, "Hilw:h:x:y:c:t:o:")) != EOF)
	switch(c)
	{
		case 'l':	/* landscape */
			ps.landscape = YES;
			break;

		case 'H':	/* output in Hex format */
			ps.hex = YES;
			break;

		case 'w':	/* width in inch */
			ps.wspec = YES;
			ps.width = atof(optarg);
			break;

		case 'h':	/* height in inch */
			ps.hspec = YES;
			ps.height = atof(optarg);
			break;

		case 'x':	/* left position of image in inch */
			ps.xspec = YES;
			ps.left = atof(optarg);
			break;

		case 'y':	/* top position of image in inch */
			ps.yspec = YES;
			ps.top  = atof(optarg);
			break;

		case 'c':	/* how many copies ... */
			ps.copies = atof(optarg);
			break;

		case 't':	/* Papertray by Location... */
			ps.tray = atof(optarg);
			break;

		case 'o':	/* Output location ... */
			ps.output = atof(optarg);
			break;

		case 'i':
			ps.hdrinf = YES;
			break;

		case '?':
			status++;
			break;
	}
if(status)
	fprintf(stderr, usage);

readhdr();	        /* read in header              */
if (ps.hdrinf)
	dumphdr();	/* display header information  */
readcmp();		/* read colormap               */
readimg();		/* read image                  */
putprolog();		/* output prolog of PostScript */
ppss(prolog);		/* Copy fixed part of prolog   */
putdata();		/* output image data           */
fprintf(stdout,"imcntx restore %% The End\n");
exit(0);
}
/************************************************************************/
/*	init paramater							*/
/************************************************************************/
init_ps()
{
ps.hex       = NO;     /* output binary (for lps40 or ..) */
ps.landscape = NO;     /* portlate                        */
ps.height    = 0.0;    /* height not specified            */
ps.hspec     = NO;     /*                                 */
ps.width     = 0.0;    /* width not specified             */
ps.wspec     = NO;     /*                                 */
ps.left      = 0.0;    /* position not                    */
ps.xspec     = NO;     /*                                 */
ps.top       = 0.0;    /*          specified              */
ps.yspec     = NO;     /*                                 */
ps.tray      = 0;      /* Not specified                   */
ps.output    = 0;      /* Not specified                   */
ps.copies    = 1;      /* make 1 copy                     */
ps.hdrinf    = NO;     /* not print header information    */
return(0);
}
/************************************************************************/
/*	dump header							*/
/************************************************************************/
dumphdr()
{
fprintf(stderr," image filetype flag= IMGFLG		%d\n",im.imgflg);
fprintf(stderr," image format (fax, workstation, RGB)	%d\n",im.format);
fprintf(stderr," grid type				%d\n",im.grdtyp);
fprintf(stderr," image encoding scheme 			%d\n",im.coding);
fprintf(stderr," header length in sectors		%d\n",im.hedlen);
fprintf(stderr," colormap length in sectors		%d\n",im.cmplen);
fprintf(stderr," number of color map entries		%d\n",im.cmpenm);
fprintf(stderr," number of color map sections/entry	%d\n",im.cmpsnm);
fprintf(stderr," size of each colormap section (bits)	%d\n",im.cmscsz);
fprintf(stderr," length of spectral band groups		%d\n",im.spbgnm);
fprintf(stderr," length of spectral band data		%d\n",im.spblen);
fprintf(stderr," number spectral bands			%d\n",im.spbnum);
fprintf(stderr," number of scan lines/band in pixels	%d\n",im.spbynm);
fprintf(stderr," width of each band in pixels		%d\n",im.spbxnm);
fprintf(stderr," number of bits/pixel			%d\n",im.spbznm);
fprintf(stderr," image pixel ordering			%d\n",im.orient);
fprintf(stderr," separation of adjacent pixels		%d\n",im.imxres);
fprintf(stderr," separation of pixel scan lines		%d\n",im.imyres);
fprintf(stderr," relative xposition in larger grid	%d\n",im.imxpos);
fprintf(stderr," relative yposition in larger grid	%d\n",im.imypos);
fprintf(stderr," interscene time duration		%d\n",im.timgap);
fprintf(stderr," image information string (ASCII)	%s\n",im.inform);
}
/************************************************************************/
/*	read in header							*/
/************************************************************************/
readhdr()
{
size=HEDLEN*512;	/* default headerlength */
fread(&im,size,1,stdin);	/* read in header */
if(im.imgflg!=IMGFLG)
	{
	fprintf(stderr,"image file format error\n");
	exit(0);
	}
if((im.format!=QDSS && im.format != QVSS) ||im.spbgnm!=1)
	{
	fprintf(stderr,"image file is not local format \n");
	exit(0);
	}
return(0);
}
/************************************************************************/
/*	read in color map						*/
/************************************************************************/
readcmp()
{
short	*tmpptr;
short	*tmp;
unsigned int	temp;
size=im.cmplen*512;
if(size==0)
	return(0);
if((cmpptr=(short *) malloc(size))==NULL)
	{
	fprintf(stderr,"colormap allocation failure\n");
	exit(0);
	}
fread(cmpptr,size,1,stdin);	/* read in colormap */
if(im.format==QVSS)
	return(0);
/* do RGB to YIQ conversion */
/* or color to black and white */
/* only the Y component is used */
tmpptr=cmpptr;
for(i=0;i!=256;i++)
	{
	tmp=tmpptr;
	temp= *tmpptr * .30;
	tmpptr++;
	temp+= *tmpptr * .59;
	tmpptr++;
	temp+= *tmpptr * .11;
	tmpptr++;
	*tmp = temp;
	}
return(0);
}
/************************************************************************/
/*	read in image data						*/
/************************************************************************/
readimg() 
{
unsigned	temp;
char	*tmpptr;
size=im.spblen*512;
if((imgptr=(char *) malloc(size))==NULL)
	{
	fprintf(stderr,"image map allocation failure\n");
	exit(0);
	}
fread(imgptr,size,1,stdin);	/* read in header */
return(0);
}
/************************************************************************/
/*	put PostScript prolog						*/
/************************************************************************/
putprolog()
{
long time();
char *ctime();
long tmp;
tmp = time(0);
fprintf(stdout,"%%!PS-Adobe-1.0\n");
fprintf(stdout,"%%%%Title: %s\n",im.inform);
fprintf(stdout,"%%%%DocumentFonts: (none)\n");
fprintf(stdout,"%%%%Creator: @(#)imscript.c	1.1\n");
fprintf(stdout,"%%%%CreationDate: %s",ctime(&tmp));
fprintf(stdout,"%%%%Pages: %d\n",ps.copies);
fprintf(stdout,"%%%%EndComments\n");
fprintf(stdout,"/imcntx save def\n");
fprintf(stdout,"/#copies %d def\n",ps.copies);
fprintf(stdout,"/pixperline %d def %%define to be # pixels per line in the image\n",im.spbxnm);
fprintf(stdout,"/nscans %d def %% define to be # scan lines in the image\n",im.spbynm);
fprintf(stdout,"/negscans nscans neg def  %%%% need -nscans for matrix used by IMAGE\n");
if(im.format==QVSS)
	fprintf(stdout,"/numbits    1     def\n");
else	fprintf(stdout,"/numbits    8     def\n");
fprintf(stdout,"/inch  {72 mul}   def\n");
fprintf(stdout,"/exdef {exch def} def\n");
fprintf(stdout,"/portrait   0     def\n");
fprintf(stdout,"/landscape  1     def\n");
if (ps.hex)
	fprintf(stdout,"/readfunc {readhexstring} def %% for hooked via RS232C\n");
else
	fprintf(stdout,"/readfunc {readstring pop currentfile crlf readstring pop} def %% for print server\n");
fprintf(stdout,"/pic 64 string def\n");
if (!ps.hex)
	fprintf(stdout,"/crlf 1 string def %%%% must do readstring to get rid of CR/LF combo\n");
fprintf(stdout,"%%\n");
fprintf(stdout,"%% User Parameter\n");
fprintf(stdout,"%%\n");
if (ps.hspec)
	fprintf(stdout,"/height %f inch def\n",ps.height);
if (ps.wspec)
	fprintf(stdout,"/width  %f inch def\n",ps.width);
if (ps.xspec)
	fprintf(stdout,"/x-pos  %f inch def\n",ps.left);
if (ps.yspec)
	fprintf(stdout,"/y-pos  %f inch def\n",ps.top);
if (ps.landscape)
	fprintf(stdout,"/orientation landscape def\n");
else
	fprintf(stdout,"/orientation portrait def\n");
if (ps.tray != 0)
	fprintf(stdout,"/tray %d def\n",ps.tray);
if (ps.output != 0)
	fprintf(stdout,"/out  %d def\n",ps.output);
return(0);
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
/*	put image data							*/
/************************************************************************/
putdata()
{
unsigned  c;
int temp,xcnt,ycnt,bytcnt;
char   *srcptr;
short  *pixptr;
char   *nxtptr;

srcptr=imgptr;
pixptr= (short *) imgptr;
bytcnt = 0;
if(ps.hex)
	{
	if(im.format==QVSS)	/* check if no colormap */
		{
		ycnt=im.spbynm;
		while(ycnt)
			{
			xcnt=im.spbxnm/8;
			while(xcnt)
				{
				c = *pixptr & 0377;
				putc(hextbl[(c >> 4)],stdout);
				putc(hextbl[(c & 0017)],stdout);
				bytcnt++; pixptr++;
				if(bytcnt>=32) { bytcnt=0; putc('\n',stdout); }
				}
			ycnt--;
			}
		}
	else	{
		nxtptr=srcptr+im.spbxnm;
		ycnt=im.spbynm;
		while(ycnt)
			{
			xcnt=im.spbxnm;
			while(xcnt)
				{
				temp= 3*(*srcptr & 0377);
				c= (cmpptr[temp] & 0377);
				putc(hextbl[(c >> 4)],stdout);
				putc(hextbl[(c & 0017)],stdout);/* put one grayscale */
				bytcnt++;
				if(bytcnt>=32) { bytcnt=0; putc('\n',stdout); }
				srcptr++;
				xcnt--;
				}
			srcptr=nxtptr;
			nxtptr=srcptr+im.spbxnm;
			ycnt--;
			}
		}
	}
else	{
	if(im.format==QVSS)	/* check if no colormap */
		{
		ycnt=im.spbynm;
		while(ycnt)
			{
			xcnt=im.spbxnm/8;
			while(xcnt)
				{
				putc(*pixptr,stdout); /* put 8 dot */
				bytcnt++; pixptr++;
				if(bytcnt>=64) { bytcnt=0; putc('\n',stdout); }
				}
			ycnt--;
			}
		}
	else	{
		nxtptr=srcptr+im.spbxnm;
		ycnt=im.spbynm;
		while(ycnt)
			{
			xcnt=im.spbxnm;
			while(xcnt)
				{
				temp= 3*(*srcptr & 0377);
				c= (cmpptr[temp] & 0377);
				putc(c,stdout); /* put one grayscale */
				bytcnt++;
				if(bytcnt>=64) { bytcnt=0; putc('\n',stdout); }
				srcptr++;
				xcnt--;
				}
			srcptr=nxtptr;
			nxtptr=srcptr+im.spbxnm;
			ycnt--;
			}
		}
	}
return(0);
}
