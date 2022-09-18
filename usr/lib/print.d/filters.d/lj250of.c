#ifndef lint
static char *sccsid = "@(#)lj250of.c	4.2      ULTRIX 	10/16/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1987, 1990 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * lj250of.c
 *
 * Modification history
 *
 * 16-Oct-90 - Adrian Thoms (thoms@wessex)
 *	Fixed the colour sixel generation algorithm:
 *	Reduced amount of data generated for typical file by a factor of 14
 *	Reduced cpu utilisation by factor of over 5
 *	Can now generate all colours, (a few were missing due to IFMAC bug)
 *
 * 01-Oct-90 - Adrian Thoms (thoms@wessex)
 *	Exit with failure status if the data is unprintable
 *	This is to match behaviour of other filters
 *
 * 26-Sep-90 - Adrian Thoms (thoms@wessex)
 *	Fixed to match new file guesser types
 *
 * 24-Sep-90 - Adrian Thoms (thoms@wessex)
 *	Removed determinefile() and associated functions and definitions
 *	to use library guesser module
 *
 * 30-Nov-89 - Daren Seymour
 *
 *	Fixed last 2 lines of file missing when printing
 *	from a VAXstation 2000.
 *
 *  9-Nov-89 - Daren Seymour
 *
 *	Fixed UWS sixel screen dump problem.
 *
 * 23-Nov-88 - Dave Gray (gray)
 *
 *      Fixed bad pointer reference in strncmp
 *
 * LJ250 Colorwriter/Companion Color Printer filter
 *
 *  9-Sep-87 - Ricky Palmer (rsp)
 *
 *	Added some additional comments.
 *
 * 16-Jun-87 - Ricky Palmer (rsp)
 *
 *	Added a couple lines to subtract one from lastcol and tmpcol
 *	variables due to a firmware change in the LJ250 (rev. 6.0).
 *
 *  3-Mar-87 - Ricky Palmer (rsp)
 *
 *	Created original file and filter program contents.
 *	This filter supports all normal text output as well
 *	as color sixels for output of sixel graphics.
 *
 */

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <a.out.h>
#include <imghdr.h>
#include <strings.h>
#include "guesser.h"

#define MAXWIDTH		80		/* width char. count	*/
#define MAXLENGTH		66		/* length char. count	*/
#define MAXP_PIX_WIDTH		720		/* portrait pixel width */
#define MAXP_PIX_HEIGHT		925		/* portrait pix. height */
#define MAXL_PIX_WIDTH		925		/* landscape p. width	*/
#define MAXL_PIX_HEIGHT		720		/* landscape p. length	*/
#define MAXCOPIES		1		/* default # of copies	*/
#define ITS			77
#define SOFF			'\077'		/* sixel element offset */
#define MAX(a,b)		(a < b ? b : a) /* useful macros	*/
#define MAXIMUM(a,b,c)		(MAX(MAX(a,b),c))
#define MIN(a,b)		(a > b ? b : a)
#define MINIMUM(a,b,c)		(MIN(MIN(a,b),c))

/*
 * These macros are used to select a colour number if parameter (a) is
 * in the range (b) to (c).
 * Parameter may be hue which wraps around fom 359 to 0
 * In this case we may have a range which straddles 0 so this
 * case is catered for.
 * Note that this is not a runtime hit because the extra comparisons will
 * be done at compile time.
 * This part of the algorithm is speeded up by factor 2 by using else if
 */
#define IFMAC			if (0) {}
#define ELSEIFMAC(a,b,c,d)	else if(((b)<(c)&&(a)>=(b)&&(a)<=(c)) || \
					((b)>(c)&&((a)>=(b)||(a)<=(c)))) {\
					if (lastcol != d) tmpcol = d; \
				}

FILE	*input = stdin, *output = stdout;	/* input and output	*/
int	width = MAXWIDTH;			/* default line length	*/
int	length = MAXLENGTH;			/* page length		*/
int	indent;					/* indentation length	*/
int	npages = MAXCOPIES;			/* number of copies	*/
int	literal;				/* print control chars. */
int	error;					/* error return status	*/
char	*name;					/* user's login name	*/
char	*host;					/* user's machine name	*/
char	*acctfile;				/* accounting info. file*/
int	kindofile = EMPTY_FILE;			/* initial kind of file */
char	*imgptr; 				/* image data pointers	*/
short	*cmpptr;				/* color map pointer	*/
struct	imghdr im;				/* image file header	*/
char	*rotptr;				/* image rotate buffer	*/
char	*malloc();				/* malloc pointer	*/
unsigned  size;					/* the usual		*/
int	rotated = 0;				/* rotated image	*/

void		exit(), bcopy(), free(), syslog();
unsigned	sleep();
/* The general strategy here is to process the command line arguments,
   open the syslog file for log information, determine the input
   stream "file type", call the filter code, and then optionally
   process accounting information upon completion. Informational
   and failure conditions are logged to syslog.
*/
main(argc, argv)
	int argc;
	char *argv[];
{
	register char *cp;
	register int i;

	while (--argc) {
		if (*(cp = *++argv) == '-') {
			switch (cp[1]) {
			case 'n':		/* collect login name	*/
				argc--;
				name = *++argv;
				break;

			case 'h':		/* collect host name	*/
				argc--;
				host = *++argv;
				break;

			case 'w':		/* collect page width	*/
				if ((i = atoi(&cp[2])) > 0 && i <= MAXWIDTH)
					width = i;
				break;

			case 'l':		/* collect page length	*/
				length = atoi(&cp[2]);
				break;

			case 'i':		/* collect indent	*/
				indent = atoi(&cp[2]);
				break;

			case 'c':		/* print control chars	*/
				literal++;
				break;
			}
		} else
			acctfile = cp;
	}

	openlog("lj250of",LOG_PID);
	kindofile = determinefile(fileno(input));
	error = lj250of();
	if(error) {
		syslog(LOG_INFO,"Failed to output data");
		exit(2);
	}
	if (name && acctfile && access(acctfile, 02) >= 0 &&
	    freopen(acctfile, "a", output) != NULL) {
		printf("%7.2f\t%s:%s\n", (float)npages, host, name);
	}
	exit(0);
}

/* Here is where all the real output work begins. The printer is
   issued a "warm reset" to clear any faulty conditions.  We sleep
   for five seconds to allow for stability and then switch to the
   appropriate code for the determined file type stream.
*/
lj250of()
{
	register int i = 0;
	register int ch;
	register int counter = 0;
	register short *tmpptr;
	unsigned short sr,sg,sb;

	fprintf(output, "\033c");
	sleep(5);
	counter = 0;
	switch(kindofile) {
		case EMPTY_FILE:
			break;
		case EXECUTABLE_FILE:
		case ARCHIVE_FILE:
		case DATA_FILE:
		case CAT_FILE:
		case POSTSCRIPT_FILE:
			syslog(LOG_INFO,"Unprintable data");
			return(1);
			break;
		case XIMAGE_FILE:
			error = readXimghdr();
			if(error) {
				syslog(LOG_INFO,"Failed to use image header");
				return(1);
			}
			error = readXimgcmp();
			if(error) {
				syslog(LOG_INFO,"Failed to use image colormap");
				return(1);
			}
			if(im.format != ITS) {
				/* do RGB to HLS conversion */
				tmpptr=cmpptr;
				for(i=0;i!=256;i++) {
					sr = *tmpptr;
					sg = *(tmpptr+1);
					sb = *(tmpptr+2);
					rgbtohls(sr,sg,sb,tmpptr,(tmpptr+1),(tmpptr+2));
					tmpptr+=3;
				}
			}
			error = readXimgdat();
			if(error) {
				syslog(LOG_INFO,"Failed to use image data");
				return(1);
			}
			if(im.spbxnm > MAXP_PIX_WIDTH) {
				error = rotateXimgdat();
				if(error) {
					syslog(LOG_INFO,"Failed to rotate image data");
					return(1);
				}
				rotated++;
			}
			error = docsixel();
			if(error) {
				syslog(LOG_INFO,"Failed to 'sixelize' data");
				fprintf(output, "\033\\");
				fprintf(output, "\033c");
				sleep(5);
				return(1);
			}
			break;
		case TEXT_FILE:
		case CTEXT_FILE:
		case ATEXT_FILE:
		case RTEXT_FILE:
		case FTEXT_FILE:
		default:
			for(globi=0;globi<in;globi++) {
				putc(filestorage[globi],output);
				if(filestorage[globi] == '\012') {
					putc('\015',output);
					counter++;
				}
				if((counter%MAXWIDTH) == 0)
					fflush(output);
				counter++;
			}
			while((ch = getc(input)) != EOF) {
				putc(ch,output);
				if(ch == '\012') {
					putc('\015',output);
					counter++;
				}
				if((counter%MAXWIDTH) == 0)
					fflush(output);
				counter++;
			}
			break;
	}
		fprintf(output,"\014");
		fflush(output);
		sleep(5);

	return(0);
}

/* If the file stream is of the XIMAGE_FILE type this routine is called
   from lj250of.  The routine creates a color sixel output stream to send
   the printer based on the input data from an image file. Currently,
   only a frame buffer image of raw red, green, blue points or a
   GPX workstation "savimg"  image are valid data streams.  Color
   sixel output is initialized and then the processing begins.
   The current design of the algorithm does not attempt to dither
   the image data but rather matches indices to valid LJ250 hardware
   map colors.	There are only 256 of these colors available as documented
   in the LJ250 Programmer's Guide.  The algorithm used here employs the
   Hue, Lightness, and Saturation color model in determining the correct
   sixels to send to the printer.
*/
docsixel()
{
	register int i;
	int xcnt,ycnt,n = 1,ij=0,tmpycnt = 0;
	unsigned char *srcptr;
	unsigned char *nxtptr;
	short sh,sl,ss;
	unsigned short sr,sg,sb;
	char *base_band,*pb;
	int lastcol = -1;
	int tmpcol = 0;
	int count = 1;
	char lastc = 0;
	register char *cp;
	struct {
		char colour_number[4];
		short colour_len;
		} didload[256];




	fprintf(output,"\033P0;0;8q\"1;1");
	srcptr=(unsigned char *)imgptr;
	ycnt=im.spbynm;
	if(im.format != ITS)
		nxtptr=srcptr+im.spbxnm * n;
	else
		nxtptr=srcptr+(im.spbxnm * n * 3);
	pb = base_band = (char *)malloc((unsigned) 12 * im.spbxnm);
	for(i=0;i<256;i++)
		didload[i].colour_len = 0;
	while(ycnt/n) {
		xcnt=im.spbxnm;
		base_band = pb;
		while(xcnt > 0) {
			if(MAXP_PIX_WIDTH - xcnt < 0) {
				if(im.format != ITS)
					srcptr+=n;
				else
					srcptr+=n*3;
				xcnt-=n;
				continue;
			}
			if(im.format != ITS) {
				sh = *((cmpptr + 3*(*srcptr))+0);
				sl = *((cmpptr + 3*(*srcptr))+1);
				ss = *((cmpptr + 3*(*srcptr))+2);
			} else {
				sr = (unsigned short)*srcptr++;
				sg = (unsigned short)*srcptr++;
				sb = (unsigned short)*srcptr;
				rgbtohls(sr,sg,sb,&sh,&sl,&ss);
			}
			for(i=0;i<2;i++) {
				*(base_band+i) = SOFF;
			}
			tmpcol = 0;
			if(ss == 0) {
				IFMAC
				ELSEIFMAC(sl,0,19,1)
				ELSEIFMAC(sl,20,39,221)
				ELSEIFMAC(sl,40,59,254)
				ELSEIFMAC(sl,60,79,255)
				ELSEIFMAC(sl,80,100,256)
				goto done;
			}
			if(sl >= 0 && sl <= 14 && lastcol != 1) {
				lastcol = 1;
				tmpcol = 1;
				goto done;
			}
			if(sl >= 86 && sl <= 100 && lastcol != 256) {
				tmpcol = 256;
				goto done;
			}
			if(sl >= 15 && sl <= 28 && ss >= 1 && ss <= 49) {
				IFMAC
				ELSEIFMAC(sh,330,29,239)
				ELSEIFMAC(sh,30,59,24)
				ELSEIFMAC(sh,60,89,50)
				ELSEIFMAC(sh,90,149,97)
				ELSEIFMAC(sh,150,179,146)
				ELSEIFMAC(sh,180,209,127)
				ELSEIFMAC(sh,210,269,159)
				ELSEIFMAC(sh,270,299,208)
				ELSEIFMAC(sh,300,329,196)
				goto done;
			}
			if(sl >= 15 && sl <= 28 && ss >= 50 && ss <= 100) {
				IFMAC
				ELSEIFMAC(sh,345,14,2)
				ELSEIFMAC(sh,15,29,3)
				ELSEIFMAC(sh,30,44,25)
				ELSEIFMAC(sh,45,54,26)
				ELSEIFMAC(sh,55,64,71)
				ELSEIFMAC(sh,65,74,37)
				ELSEIFMAC(sh,75,89,51)
				ELSEIFMAC(sh,90,104,81)
				ELSEIFMAC(sh,105,134,90)
				ELSEIFMAC(sh,135,149,43)
				ELSEIFMAC(sh,150,164,108)
				ELSEIFMAC(sh,165,174,175)
				ELSEIFMAC(sh,175,184,120)
				ELSEIFMAC(sh,185,194,128)
				ELSEIFMAC(sh,195,209,161)
				ELSEIFMAC(sh,210,224,137)
				ELSEIFMAC(sh,225,254,150)
				ELSEIFMAC(sh,255,269,187)
				ELSEIFMAC(sh,270,284,174)
				ELSEIFMAC(sh,285,294,211)
				ELSEIFMAC(sh,295,304,200)
				ELSEIFMAC(sh,305,314,210)
				ELSEIFMAC(sh,315,329,223)
				ELSEIFMAC(sh,330,344,222)
				goto done;
			}
			if(sl >= 29 && sl <= 42 && ss >= 1 && ss <= 49) {
				IFMAC
				ELSEIFMAC(sh,345,359,241)
				ELSEIFMAC(sh,0,14,240)
				ELSEIFMAC(sh,15,29,13)
				ELSEIFMAC(sh,30,44,35)
				ELSEIFMAC(sh,45,59,34)
				ELSEIFMAC(sh,60,74,52)
				ELSEIFMAC(sh,75,89,36)
				ELSEIFMAC(sh,90,104,44)
				ELSEIFMAC(sh,105,119,42)
				ELSEIFMAC(sh,120,134,98)
				ELSEIFMAC(sh,135,149,99)
				ELSEIFMAC(sh,150,164,148)
				ELSEIFMAC(sh,165,179,147)
				ELSEIFMAC(sh,180,194,129)
				ELSEIFMAC(sh,195,209,149)
				ELSEIFMAC(sh,210,224,188)
				ELSEIFMAC(sh,225,239,186)
				ELSEIFMAC(sh,240,254,160)
				ELSEIFMAC(sh,255,269,151)
				ELSEIFMAC(sh,270,284,198)
				ELSEIFMAC(sh,285,299,199)
				ELSEIFMAC(sh,300,314,197)
				ELSEIFMAC(sh,315,329,209)
				ELSEIFMAC(sh,330,349,246)
				goto done;
			}
			if(sl >= 29 && sl <= 42 && ss >= 50 && ss <= 100) {
				IFMAC
				ELSEIFMAC(sh,350,9,5)
				ELSEIFMAC(sh,10,19,6)
				ELSEIFMAC(sh,20,29,14)
				ELSEIFMAC(sh,30,39,28)
				ELSEIFMAC(sh,40,49,39)
				ELSEIFMAC(sh,50,59,57)
				ELSEIFMAC(sh,60,69,65)
				ELSEIFMAC(sh,70,79,56)
				ELSEIFMAC(sh,80,89,75)
				ELSEIFMAC(sh,90,99,64)
				ELSEIFMAC(sh,100,109,82)
				ELSEIFMAC(sh,110,129,91)
				ELSEIFMAC(sh,130,139,74)
				ELSEIFMAC(sh,140,149,100)
				ELSEIFMAC(sh,150,159,253)
				ELSEIFMAC(sh,160,169,111)
				ELSEIFMAC(sh,170,179,135)
				ELSEIFMAC(sh,180,189,121)
				ELSEIFMAC(sh,190,199,152)
				ELSEIFMAC(sh,200,209,130)
				ELSEIFMAC(sh,210,219,179)
				ELSEIFMAC(sh,220,229,155)
				ELSEIFMAC(sh,230,249,163)
				ELSEIFMAC(sh,250,259,191)
				ELSEIFMAC(sh,260,269,180)
				ELSEIFMAC(sh,270,279,203)
				ELSEIFMAC(sh,280,289,202)
				ELSEIFMAC(sh,290,299,216)
				ELSEIFMAC(sh,300,309,215)
				ELSEIFMAC(sh,310,319,226)
				ELSEIFMAC(sh,320,329,224)
				ELSEIFMAC(sh,330,339,233)
				ELSEIFMAC(sh,340,349,242)
				goto done;
			}
			if(sl >= 43 && sl <= 57 && ss >= 1 && ss <= 33) {
				IFMAC
				ELSEIFMAC(sh,330,344,10)
				ELSEIFMAC(sh,345,359,21)
				ELSEIFMAC(sh,0,14,22)
				ELSEIFMAC(sh,15,29,11)
				ELSEIFMAC(sh,30,44,41)
				ELSEIFMAC(sh,45,59,45)
				ELSEIFMAC(sh,60,74,61)
				ELSEIFMAC(sh,75,89,40)
				ELSEIFMAC(sh,90,104,63)
				ELSEIFMAC(sh,105,119,62)
				ELSEIFMAC(sh,120,134,88)
				ELSEIFMAC(sh,135,149,89)
				ELSEIFMAC(sh,150,164,173)
				ELSEIFMAC(sh,165,179,158)
				ELSEIFMAC(sh,180,194,136)
				ELSEIFMAC(sh,195,209,172)
				ELSEIFMAC(sh,210,224,178)
				ELSEIFMAC(sh,225,239,189)
				ELSEIFMAC(sh,240,254,154)
				ELSEIFMAC(sh,255,269,185)
				ELSEIFMAC(sh,270,289,219)
				ELSEIFMAC(sh,290,309,220)
				ELSEIFMAC(sh,310,329,218)
				goto done;
			}
			if(sl >= 43 && sl <= 57 && ss >= 34 && ss <= 66) {
				IFMAC
				ELSEIFMAC(sh,345,351,7)
				ELSEIFMAC(sh,352,359,243)
				ELSEIFMAC(sh,0,6,16)
				ELSEIFMAC(sh,7,14,4)
				ELSEIFMAC(sh,15,21,15)
				ELSEIFMAC(sh,22,29,27)
				ELSEIFMAC(sh,30,36,38)
				ELSEIFMAC(sh,37,44,29)
				ELSEIFMAC(sh,45,51,59)
				ELSEIFMAC(sh,52,59,55)
				ELSEIFMAC(sh,60,66,66)
				ELSEIFMAC(sh,67,74,60)
				ELSEIFMAC(sh,75,81,54)
				ELSEIFMAC(sh,82,89,53)
				ELSEIFMAC(sh,90,96,83)
				ELSEIFMAC(sh,97,104,84)
				ELSEIFMAC(sh,105,111,73)
				ELSEIFMAC(sh,112,119,72)
				ELSEIFMAC(sh,120,126,92)
				ELSEIFMAC(sh,127,134,101)
				ELSEIFMAC(sh,135,141,252)
				ELSEIFMAC(sh,142,149,251)
				ELSEIFMAC(sh,150,156,110)
				ELSEIFMAC(sh,157,164,109)
				ELSEIFMAC(sh,165,171,145)
				ELSEIFMAC(sh,172,179,144)
				ELSEIFMAC(sh,180,186,122)
				ELSEIFMAC(sh,187,194,134)
				ELSEIFMAC(sh,195,201,162)
				ELSEIFMAC(sh,202,209,250)
				ELSEIFMAC(sh,210,216,139)
				ELSEIFMAC(sh,217,224,138)
				ELSEIFMAC(sh,225,231,177)
				ELSEIFMAC(sh,232,239,176)
				ELSEIFMAC(sh,240,246,164)
				ELSEIFMAC(sh,247,254,153)
				ELSEIFMAC(sh,255,261,201)
				ELSEIFMAC(sh,262,269,190)
				ELSEIFMAC(sh,270,276,192)
				ELSEIFMAC(sh,277,284,204)
				ELSEIFMAC(sh,285,291,213)
				ELSEIFMAC(sh,292,299,214)
				ELSEIFMAC(sh,300,306,217)
				ELSEIFMAC(sh,307,314,212)
				ELSEIFMAC(sh,315,321,231)
				ELSEIFMAC(sh,322,329,234)
				ELSEIFMAC(sh,330,336,232)
				ELSEIFMAC(sh,337,344,225)
				goto done;
			}
			if(sl >= 43 && sl <= 57 && ss >= 67 && ss <= 100) {
				IFMAC
				ELSEIFMAC(sh,353,7,9)
				ELSEIFMAC(sh,8,22,17)
				ELSEIFMAC(sh,23,37,30)
				ELSEIFMAC(sh,38,52,58)
				ELSEIFMAC(sh,53,67,67)
				ELSEIFMAC(sh,68,82,76)
				ELSEIFMAC(sh,83,97,85)
				ELSEIFMAC(sh,98,112,86)
				ELSEIFMAC(sh,113,127,87)
				ELSEIFMAC(sh,128,142,102)
				ELSEIFMAC(sh,143,157,114)
				ELSEIFMAC(sh,158,172,118)
				ELSEIFMAC(sh,173,187,123)
				ELSEIFMAC(sh,188,202,131)
				ELSEIFMAC(sh,203,217,141)
				ELSEIFMAC(sh,218,232,156)
				ELSEIFMAC(sh,233,247,181)
				ELSEIFMAC(sh,248,262,184)
				ELSEIFMAC(sh,263,277,193)
				ELSEIFMAC(sh,278,292,205)
				ELSEIFMAC(sh,293,307,227)
				ELSEIFMAC(sh,308,322,235)
				ELSEIFMAC(sh,323,337,244)
				ELSEIFMAC(sh,338,352,247)
				goto done;
			}
			if(sl >= 58 && sl <= 71 && ss >= 1 && ss <= 49) {
				IFMAC
				ELSEIFMAC(sh,300,359,23)
				ELSEIFMAC(sh,0,59,12)
				ELSEIFMAC(sh,60,119,107)
				ELSEIFMAC(sh,120,179,106)
				ELSEIFMAC(sh,180,239,171)
				ELSEIFMAC(sh,240,299,170)
				goto done;
			}
			if(sl >= 58 && sl <= 71 && ss >= 50 && ss <= 100) {
				IFMAC
				ELSEIFMAC(sh,350,359,18)
				ELSEIFMAC(sh,0,9,8)
				ELSEIFMAC(sh,10,19,32)
				ELSEIFMAC(sh,20,29,31)
				ELSEIFMAC(sh,30,39,47)
				ELSEIFMAC(sh,40,49,46)
				ELSEIFMAC(sh,50,69,68)
				ELSEIFMAC(sh,70,79,78)
				ELSEIFMAC(sh,80,89,77)
				ELSEIFMAC(sh,90,99,94)
				ELSEIFMAC(sh,100,109,93)
				ELSEIFMAC(sh,110,119,96)
				ELSEIFMAC(sh,120,129,95)
				ELSEIFMAC(sh,130,139,104)
				ELSEIFMAC(sh,140,149,113)
				ELSEIFMAC(sh,150,159,119)
				ELSEIFMAC(sh,160,169,117)
				ELSEIFMAC(sh,170,189,124)
				ELSEIFMAC(sh,190,199,133)
				ELSEIFMAC(sh,200,209,132)
				ELSEIFMAC(sh,210,219,140)
				ELSEIFMAC(sh,220,229,157)
				ELSEIFMAC(sh,230,239,166)
				ELSEIFMAC(sh,240,249,165)
				ELSEIFMAC(sh,250,259,182)
				ELSEIFMAC(sh,260,269,183)
				ELSEIFMAC(sh,270,279,207)
				ELSEIFMAC(sh,280,289,206)
				ELSEIFMAC(sh,290,309,228)
				ELSEIFMAC(sh,310,319,237)
				ELSEIFMAC(sh,320,329,236)
				ELSEIFMAC(sh,330,339,249)
				ELSEIFMAC(sh,340,349,248)
				goto done;
			}
			if(sl >= 72 && sl <= 85 && ss >= 1 && ss <= 49) {
				IFMAC
				ELSEIFMAC(sh,0,119,70)
				ELSEIFMAC(sh,120,239,126)
				ELSEIFMAC(sh,240,359,230)
				goto done;
			}
			if(sl >= 72 && sl <= 85 && ss >= 50 && ss <= 100) {
				IFMAC
				ELSEIFMAC(sh,345,354,20)
				ELSEIFMAC(sh,355,4,19)
				ELSEIFMAC(sh,5,14,33)
				ELSEIFMAC(sh,15,29,49)
				ELSEIFMAC(sh,30,44,48)
				ELSEIFMAC(sh,45,74,69)
				ELSEIFMAC(sh,75,89,80)
				ELSEIFMAC(sh,90,104,79)
				ELSEIFMAC(sh,105,114,105)
				ELSEIFMAC(sh,115,124,103)
				ELSEIFMAC(sh,125,134,112)
				ELSEIFMAC(sh,135,149,116)
				ELSEIFMAC(sh,150,164,115)
				ELSEIFMAC(sh,165,194,125)
				ELSEIFMAC(sh,195,209,143)
				ELSEIFMAC(sh,210,224,142)
				ELSEIFMAC(sh,225,234,168)
				ELSEIFMAC(sh,235,244,167)
				ELSEIFMAC(sh,245,254,169)
				ELSEIFMAC(sh,255,269,195)
				ELSEIFMAC(sh,270,284,194)
				ELSEIFMAC(sh,285,314,229)
				ELSEIFMAC(sh,315,329,238)
				ELSEIFMAC(sh,330,344,245)
				    /****** This can't be necessary
				     ****** I may nuke it AT
				     ******/
				    /* royal kludge to account for poor skin tone abilities of printer */
				if(tmpcol == 105 || tmpcol == 103 || tmpcol == 112) {
					sh = 200;
					sl = 80;
					ss = 40;
					tmpcol = 126;
				}
				goto done;
			}
done:
			if(tmpcol) {
				register int stringlen;

				lastcol = tmpcol;
				tmpcol = tmpcol - 1;
				if(!didload[tmpcol].colour_len) {
					register char *p;
					if(sh == -1)
						sh = 0;
					sprintf(base_band,"#%d;1;%d;%d;%d",tmpcol,sh,sl,ss);
					p = strchr(base_band, ';');
					stringlen = p - base_band;
					didload[tmpcol].colour_len = stringlen;

					strncpy(didload[tmpcol].colour_number,
						base_band,
						stringlen);
					base_band += strlen(base_band);
				}
				/*
				 * This is done very often so don't use
				 * strncpy
				 */
#if 0
				strncpy(base_band,
					didload[tmpcol].colour_number,
					didload[tmpcol].colour_len);
				base_band += didload[tmpcol].colour_len;
#endif
				for (i=0; i<didload[tmpcol].colour_len; i++) {
					*base_band++ =
					    didload[tmpcol].colour_number[i];
				}
			}
			ij = tmpycnt%6;
			*base_band++ = (1<<ij) + SOFF;

			srcptr+=n;
			xcnt-=n;
		}

		*base_band = '\0';

		for(cp = pb, lastc = *cp++; lastc; cp++) {
			if(*cp != lastc) {
				if(count >= 4)
				    fprintf(output,"!%d%c",count,lastc);
				else
				    while(count--)putc(lastc,output);
				count = 1;
				lastc = *cp;
			} else count++;
		}
		if (ij == 5) {
			putc('-', output);
		} else {
			putc('$', output);
		}
		count=1;
		srcptr=nxtptr;
		if(im.format != ITS)
			nxtptr=srcptr+im.spbxnm * n;
		else
			nxtptr=srcptr+(im.spbxnm * n * 3);
		ycnt-= n;
		tmpycnt++;
		lastcol = -1;
	}
	fprintf(output,"\033\\");
	return(0);
}

readXimghdr()
{
	register int tmp;

	size=HEDLEN*512;
	bcopy(filestorage+globi, (char *)&im, (int)size);
	globi = globi + size;
	if(im.imgflg != IMGFLG)
		return(1);
	if(im.format != QDSS || im.spbgnm != 1) {
		if(im.format != ITS)
			return(1);
	}
	if(im.format == ITS) {
		tmp = im.spbxnm;
		im.spbxnm = im.spbynm;
		im.spbynm = tmp;
		im.spblen = im.spblen * 3;
	}
	return(0);
}

readXimgcmp()
{
	if(im.format == ITS)
		return(0);
	size=im.cmplen*512;
	if(size==0)
		return(1);
	if((cmpptr=(short *) malloc(size))==NULL)
		return(1);
	bcopy(filestorage+globi, (char *)cmpptr, (int)size);
	globi = globi + size;
	return(0);
}

readXimgdat()
{
	size=im.spblen*512;
	if((imgptr=(char *) malloc(size))==NULL) {
		return(1);
	}
	bcopy(&filestorage[globi],imgptr,in-globi);
	fread(imgptr+(in-globi), (char)size-(in-globi), 1, input);
	return(0);
}

rgbtohls(sr,sg,sb,sh,sl,ss)
	unsigned short sr,sg,sb;
	short *sh,*sl,*ss;
{
	double r,g,b,h,l,s;
	double rc,gc,bc;
	double max,min;

	r = ((double)sr)/255.0;
	g = ((double)sg)/255.0;
	b = ((double)sb)/255.0;
	max = MAXIMUM(r,g,b);
	min = MINIMUM(r,g,b);
	l = (max+min)/2.0;

	if(max == min) {
		s = 0.0;
		h = -1.0;
	} else {
		if(l <= 0.5) {
			s = (max - min)/(max + min);
		} else {
			s = (max - min)/(2.0 - max - min);
		}
		rc = 1.0-(max - r)/(max - min);
		gc = 1.0-(max - g)/(max - min);
		bc = 1.0-(max - b)/(max - min);

		if(b == max) {
			h =  rc - gc;
			goto done;
		}
		if(g == max) {
			h = 4.0 + bc - rc;
			goto done;
		}
		if(r == max) {
			h = 2.0 + gc - bc;
			goto done;
		}
done:
		h = h*60.0;
		if(h < 0.0) {
			h = h + 360.0;
		}
	}
	*sh = (short)h;
	*sl = (short)(l * 100.0);
	*ss = (short)(s * 100.0);
}

rotateXimgdat()
{
	register char *rotimgptr;
	register char *nxtptr;
	register int xcnt = 0;
	register int ycnt = 0;
	register int size;
	char *startrotptr;

	rotimgptr = imgptr;
	size=im.spblen*512;
	if((rotptr = (char *)malloc((unsigned)size)) == NULL)
		return(1);
	startrotptr = rotptr;
	rotimgptr += (im.spbynm-1)*im.spbxnm;
	nxtptr = rotimgptr+1;
	while(ycnt < im.spbxnm) {
		while(xcnt < im.spbynm) {
			*rotptr = *rotimgptr;
			rotimgptr -= im.spbxnm;
			rotptr++;
			xcnt++;
		}
		xcnt = 0;
		ycnt++;
		rotimgptr = nxtptr;
		nxtptr++;
	}
	xcnt = im.spbxnm;
	im.spbxnm = im.spbynm;
	im.spbynm = xcnt;
	free(imgptr);
	imgptr = startrotptr;
	return(0);
}
