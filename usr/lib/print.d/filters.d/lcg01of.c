#ifndef lint
static char *sccsid = "@(#)lcg01of.c	4.2      ULTRIX 	10/16/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1987,1990 by			*
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
 * lcg01of.c
 *
 * Modification history
 *
 *  24-Sep-90 - Adrian Thoms (thoms@wessex)
 *	Removed determinefile() and associated functions and definitions
 *	to use library guesser module
 *
 *  23-Nov-88 - Dave Gray (gray)
 *     Fixed bad pointer reference in strncmp
 *
 * LCG01(formerly LCP01) Color Printing System filter
 *
 *  9-Sep-87 - Ricky Palmer (rsp)
 *
 *	Added some additional comments.
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
#include <sys/ioctl.h>

#include "guesser.h"

#define MAXWIDTH		80		/* width char. count	*/
#define MAXLENGTH		66		/* length char. count	*/
#define MAXLWIDTH		132		/* max. landscape width */
#define MAXP_PIX_WIDTH		585		/* portrait pixel width */
#define MAXP_PIX_HEIGHT		770		/* portrait pix. height */
#define MAXL_PIX_WIDTH		1540		/* landscape p. width	*/
#define MAXL_PIX_HEIGHT		1170		/* landscape p. length	*/
#define MAXCOPIES		1		/* default # of copies	*/
#define ITS			77
#define LCG_LOADFILE		"/usr/lib/lpdfilters/lcg01sw.dat" /* load file	*/
#define SOFF			'\077'		/* sixel element offset */
#define MAX(a,b)		(a < b ? b : a) /* useful macros	*/
#define MAXIMUM(a,b,c)		(MAX(MAX(a,b),c))
#define MIN(a,b)		(a > b ? b : a)
#define MINIMUM(a,b,c)		(MIN(MIN(a,b),c))

FILE	*input = stdin, *output = stdout;	/* input and output	*/
char	lpin[80];				/* lp device tmp input	*/
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
int	tmppagecount = 0, tmplinecount = 0;	/* tmp counters		*/
char	*imgptr; 				/* image data pointers	*/
short	*cmpptr;				/* color map pointer	*/
struct	imghdr im;				/* image file header	*/
char	*malloc();				/* malloc pointer	*/
unsigned  size;					/* the usual		*/

void		free(), exit(), bcopy(), syslog();
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

	openlog("lcg01of",LOG_PID);
	kindofile = determinefile(fileno(input));
	error = lcg01of();
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
   appropriate code for the determined file type stream.  The LCG01
   has to also download its program if necessary and it does so if
   the response code (lpin='c') is correct.  This filter code has the
   peculiarity that it must "communicate" with the printer bi-directionally
   for the above stated reason. The download (if necessary) and bi-directional
   communications path are established before the warm reset.
*/
lcg01of()
{
	register int i = 0;
	register FILE *dload;
	register int counter = 0;
	register short *tmpptr;
	int	ch;
	short sr,sg,sb;
	int lpinc = 0, retrycnt = 0;

tryitagain:
	fprintf(output,"\033[c");
	fflush(output);
	sleep(5);
	ioctl(fileno(output),FIONREAD,(char *)&lpinc);
	lpinc = read(fileno(output),lpin,lpinc);
	if(lpinc == 0) {
		retrycnt++;
		if(retrycnt > 5)
			return(1);
		if(retrycnt <= 5) {
			sleep(40);
			goto tryitagain;
		}
	}
/* top: */
	if(lpin[5] == 'c') {
		if((dload=(fopen(LCG_LOADFILE,"r")))==NULL) {
			syslog(LOG_INFO,"Unable to open %s",LCG_LOADFILE);
			return(1);
		}
		while((ch = getc(dload)) != EOF) {
			if(ch == '\033') {
				write(fileno(output),(char *)&ch,1);
				sleep(1);
				while((ch = getc(dload)) != EOF &&
				      ch != '\012' ) {
					write(fileno(output),(char *)&ch,1);
					sleep(1);
				}
				if(ch == '\012') {
					write(fileno(output),(char *)&ch,1);
					sleep(5);
					continue;
				} else {
					break;
				}
			}
			write(fileno(output), (char *)&ch,1);
		}
		fclose(dload);
		syslog(LOG_INFO,"%s downloaded into LCG01",LCG_LOADFILE);
	}

	fprintf(output,"\033[5n");
	fflush(output);
	sleep(5);
	ioctl(fileno(output),FIONREAD, (char *)&lpinc);
	lpinc = read(fileno(output),lpin,lpinc);
	if(lpinc != 0) {
		switch(lpin[3]) {
		case '2':
			switch(lpin[4]) {
			case '0':
				syslog(LOG_INFO,"Printer is functional");
				break;
			case '1':
				syslog(LOG_INFO,"Printer hardware error");
				return(1);
				break;
			case '2':
				syslog(LOG_INFO,"Printer communications I/O error");
				return(1);
				break;
			case '3':
				syslog(LOG_INFO,"Printer input buffer overflow");
				return(1);
				break;
			case '4':
				syslog(LOG_INFO,"Printer deselected");
				return(1);
				break;
			case '7':
				syslog(LOG_INFO,"Printer out of ink or paper");
				return(1);
				break;
			case '8':
				syslog(LOG_INFO,"Printer program load failure");
				return(1);
				break;
			default:
				break;
			}
			break;
		case '4':
			switch(lpin[4]) {
			case '2':
				syslog(LOG_INFO,"Printer font load failure");
				return(1);
				break;
			case '4':
				syslog(LOG_INFO,"Printer font memory exceeded");
				return(1);
				break;
			default:
				break;
			}
			break;
		case '1':
			switch(lpin[4]) {
			case '0':
				switch(lpin[5]) {
				case '4':
					syslog(LOG_INFO,"Printer has too many fonts");
					return(1);
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

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
					rgbtohls((unsigned short)sr, (unsigned short)sg, (unsigned short)sb, tmpptr,(tmpptr+1),(tmpptr+2));
					tmpptr+=3;
				}
			}
			error = readXimgdat();
			if(error) {
				syslog(LOG_INFO,"Failed to use image data");
				return(1);
			}
			if(im.spbxnm > MAXP_PIX_WIDTH || im.spbynm > MAXP_PIX_HEIGHT) {
				fprintf(output,"\033[1 J");
				if(im.spbxnm > MAXP_PIX_HEIGHT || im.spbynm > MAXP_PIX_WIDTH) {
					fprintf(output,"\033P9;0;1q\"1;1;%d;%d",
						MAXL_PIX_WIDTH,MAXL_PIX_HEIGHT);
					if(im.spbynm > MAXL_PIX_HEIGHT) {
						tmppagecount = im.spbynm/MAXL_PIX_HEIGHT;
						tmplinecount = MAXL_PIX_HEIGHT;
					}
				} else {
					fprintf(output,"\033[7 I\033P9;0;2q\"1;1;%d;%d",
						MAXP_PIX_HEIGHT,MAXP_PIX_WIDTH);
				}
			} else {
				fprintf(output,"\033[7 I\033P9;0;2q\"1;1;%d;%d",
					MAXP_PIX_WIDTH,MAXP_PIX_HEIGHT);
			}
			error = docsixel();
			if(error) {
				syslog(LOG_INFO,"Failed to 'sixelize' data");
				fprintf(output, "\033\\");
				fprintf(output, "\033c");
				sleep(5);
				fprintf(output, "\014");
				fflush(output);
				return(1);
			}
			free((char *)cmpptr);
			free((char *)imgptr);
			break;
		case TEXT_FILE:
		case CTEXT_FILE:
		case ATEXT_FILE:
		case RTEXT_FILE:
		case FTEXT_FILE:
		default:
			if(width > MAXLWIDTH) {
				fprintf(output,"\033[1 J");
			}
			for(globi=0;globi<in;globi++) {
				putc(filestorage[globi],output);
				if(filestorage[globi] == '\012') {
					putc('\015',output);
					counter++;
				}
				if((counter%4) == 0)
					fflush(output);
				counter++;
			}
			while((ch = getc(input)) != EOF) {
				putc(ch,output);
				if(ch == '\012') {
					putc('\015',output);
					counter++;
				}
				if((counter%4) == 0)
					fflush(output);
				counter++;
			}
			break;
	}

	fprintf(output, "\014");
	fflush(output);
	retrycnt = 0;
	lpinc = 0;
	fprintf(output,"\033[c");
	fflush(output);
	sleep(5);
	while(!lpinc) {
		ioctl(fileno(output),FIONREAD, (char *)&lpinc);
		lpinc = read(fileno(output),lpin,lpinc);
		if(lpinc == 0) {
			retrycnt++;
			sleep(10);
			if(retrycnt > 20)
				return(1);
		}
	}
	fflush(output);
	return(0);
}

/* If the file stream is of the XIMAGE_FILE type this routine is called
   from lcg01of.  The routine creates a color sixel output stream to send
   the printer based on the input data from an image file. Currently,
   only a frame buffer image of raw red, green, blue points or a
   GPX workstation "savimg"  image are valid data streams.  Color
   sixel output is initialized and then the processing begins.
   The current design of the algorithm does not attempt to dither
   the image data but rather sends color values to the LCG01 which performs
   a hardware dither.  There 256 colors per page available.  The algorithm
   used here employs the Hue, Lightness, and Saturation color model
   in determining the correct sixels to send to the printer.
*/
docsixel()
{
	register int i;
	int xcnt,ycnt,n = 1,ij=0,tmpycnt=0;
	unsigned char	*srcptr;
	unsigned char	*nxtptr;
	short sh,sl,ss;
	unsigned short sr,sg,sb;
	char *base_band,*pb;
	int lastcol = -1;
	int tmpcol = 0;
	char tmpbuf[30];
	int base_band_length = 0;
	int count = 1;
	char lastc = 0;
	register char *cp;
	int didload[256];





	srcptr=(unsigned char *)imgptr;
	ycnt=im.spbynm;
	if(im.format != ITS)
		nxtptr=srcptr+im.spbxnm * n;
	else
		nxtptr=srcptr+(im.spbxnm * n * 3);
	pb = base_band = (char *)malloc((unsigned) 12 * im.spbxnm);
	for(i=0;i<256;i++)
		didload[i] = 0;
	while((ycnt/n) * n) {
		xcnt=im.spbxnm;
		base_band_length = 0;
		base_band = pb;
		while(xcnt>0) {
			if(MAXL_PIX_WIDTH - xcnt < 0) {
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
			tmpcol = -1;
			if(lastcol != (unsigned short)*srcptr) {
				lastcol = (unsigned short)*srcptr;
				tmpcol = lastcol;
			}
			if(tmpcol >= 0) {
				if(!didload[lastcol]) {
					if(sh == -1)
						sh = 0;
					sprintf(&tmpbuf[0],"#%d;1;%d;%d;%d",lastcol,sh,sl,ss);
					strcpy(base_band,tmpbuf);
					base_band += strlen(tmpbuf);
					base_band_length += strlen(tmpbuf);
					didload[lastcol]++;
				}
				*base_band++ = '#';
				sprintf(&tmpbuf[0],"%d",lastcol);
				strcpy(base_band,tmpbuf);
				base_band += strlen(tmpbuf);
				base_band_length += 1 + strlen(tmpbuf);
			}
			*base_band++ = (1<<(tmpycnt%6)) + SOFF;
			ij = tmpycnt%6;
			base_band_length++;
			srcptr+=n;
			xcnt-=n;
		}
		if(ij != 5) {
			cp = pb;
			lastc = *cp++;
			for(i=0;i < base_band_length;i++) {
			    if(*cp != lastc) {
				if(count >= 4)
				    fprintf(output,"!%d%c",count,lastc);
				else
				    while(count--)putc(lastc,output);
				count = 1;
				lastc = *cp;
			    } else count++;
			    cp++;
			}
			fprintf(output,"$");
		} else {
			cp = pb;
			lastc = *cp++;
			for(i=0;i < base_band_length;i++) {
			    if(*cp != lastc) {
				if(count >= 4)
				    fprintf(output,"!%d%c",count,lastc);
				else
				    while(count--)putc(lastc,output);
				count = 1;
				lastc = *cp;
			    } else count++;
			    cp++;
			}
			fprintf(output,"-");
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
		if(tmppagecount) {
			tmplinecount--;
			if(tmplinecount == 0) {
				fprintf(output,"+");
				tmppagecount--;
				tmplinecount = MAXL_PIX_HEIGHT;
			}
		}
	}
	fprintf(output,"+");
	fprintf(output,"\033\\");
	free(base_band);
	return(0);
}

readXimghdr()
{
	register int tmp;

	size=HEDLEN*512;
	bcopy(filestorage+globi,(char *)&im, (int)size);
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
	if((imgptr=(char *) malloc(size))==NULL)
		return(1);
	bcopy(&filestorage[globi],imgptr,in-globi);
	fread(imgptr+(in-globi),(char)size-(in-globi),1,input);
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

