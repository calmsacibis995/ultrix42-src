#ifndef lint
static char *sccsid = "@(#)decuniversal_of.c	4.1      ULTRIX 	10/16/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1987, 1988 and 1990 by		*
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
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
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
 * File:	ln03_lg31_lg02_la75.c
 * Author:	Adrian Thoms (thoms@wessex)
 * Description:
 *	This is the print filter for the following printers:
 *	LN03, LG31, LG02 and LA75
 *
 *	This file was created by merging the code in lg31of.c, lg02of.c
 *	and la75.c back into ln03of.c from which they appear to have been
 *	derived.
 *
 *	File type guessing has been removed and is now performed by a
 *	library module.
 *
 * Modification history:
 *
 *  26-sep-90 - Adrian Thoms
 *	No longer prints PostScript(TM) files
 *
 *  25-sep-90 - Adrian Thoms
 *	Merged in fix to literal handling of EOF from la75.c
 *
 *  25-sep-90 - Adrian Thoms
 *	Merged in la75 functionality from la75.c
 *
 *  25-sep-90 - Adrian Thoms (thoms@wessex)
 *	Merged in lg02 functionality from lg02of.c
 *
 *  25-sep-90 - Adrian Thoms (thoms@wessex)
 *	Increased MAXWIDTH and BUFWIDTH to answer QAR #04155
 *
 *  25-sep-90 - Adrian Thoms (thoms@wessex)
 *	Merged in lg31 functionality from lg31of.c
 *
 *  24-sep-90 - Adrian Thoms (thoms@wessex)
 *	Removed determinefile() and associated functions and definitions
 *	to use library guesser module
 *
 * 23-Nov-88 - Dave Gray (gray)
 *
 *      Fixed bad pointer reference in strncmp
 *
 *
 *	5-May-88 Pradeep Chetal (chetal)
 *	 - made 8-bit clean
 *
 *  23-Jan-88  David Gray (gray)
 *
 *      Modified the escape handler to embed Nroff escape sequences
 *      in the buffer to allow appropriate actions to take place, ie.,
 *      super and subscripting.
 *
 *  7-Dec-87   Pradeep Chetal (chetal)
 *
 * 	Added code to handle literal option and some optimizations.
 *
 *  9-Sep-87 - Ricky Palmer (rsp)
 *
 *	Added some additional comments.
 *
 * 21-Aug-87 - David Gray (gray)
 *
 *	To determine file type the first 4096 characters of the file
 *	are buffered in "filestorage". When the file is actually printed
 *	an attempt was made to access the 4097 element which resulted in an
 *	extra character being inserted into the final output. This has been
 *	corrected. The correction made was to change the test
 *		  globi <= in	  to	globi < in
 *	in function ln03of().
 *
 *
 * 25-Apr-87 - Ricky Palmer (rsp)
 *
 *	Berkeley ln03 filter code put into source maintenance format.
 *
 *	Letter Quality Printers filter for ln03 looking like lqp
 *
 *	filter which reads the output of nroff and converts lines
 *	with ^H's to overwritten lines.  Thus this works like 'ul'
 *	but is much better: it can handle more than 2 overwrites
 *	and it is written with some style.
 *	modified by kls to use register references instead of arrays
 *	to try to gain a little speed.
 *
 *	Passes through escape and control sequences.
 *
 *	Sends control chars to change to landscape mode for pages wider
 *	than 80 columns.  Also changes font and pitch for this case in
 *	order to get 66 lines per page in landscape mode.
 *
 *	Special logic for nroff ESC9 (partial line feed): this is
 *	converted to ln03 sequence for partial line feed.  Eventually
 *	this kluge should be replaced by an output package for nroff
 *	which knows about ln03 output.
 *
 * 25-Apr-87 - Ricky Palmer (rsp)
 *
 *	Added code to original Berkeley filter based on
 *	LCG01 and LJ250 filters. This filter supports
 *	normal text output as well as sixels for output of
 *	sixel graphics.
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
#include <locale.h>
#include "guesser.h"

struct filter_info;		/* forward declaration */
/*
 * Functions in this file
 */
static struct filter_info *
set_which_filter();		/* Determine which printer we are driving */

static int
filter();			/* Process the data */


#define MAXWIDTH		220		/* maximum char. width	*/
#define BUFWIDTH		300		/* maximum buf.  width	*/
#define MAXREP			10		/* buffer depth		*/
#define DEFWIDTH		80		/* width char. count	*/
#define DEFHEIGHT		66		/* length char. count	*/
#define MAXP_PIX_WIDTH		800		/* portrait pixel width */
#define MAXP_PIX_HEIGHT		1040		/* portrait pix. height */
#define MAXL_PIX_WIDTH		1040		/* landscape p. width	*/
#define MAXL_PIX_HEIGHT		800		/* landscape p. length	*/
#define MAXCOPIES		1		/* default # of copies	*/
#define ITS			77
#define SOFF			'\077'		/* sixel element offset */

#define MAX(a,b)		(a < b ? b : a) /* useful macros	*/
#define MAXIMUM(a,b,c)		(MAX(MAX(a,b),c))
#define MIN(a,b)		(a > b ? b : a)
#define MINIMUM(a,b,c)		(MIN(MIN(a,b),c))

#define ESC	  '\033'	/* escape sequence introducer */
#define BSLH	  '\134'	/* back slash */
#define UCP	  '\120'	/* upper case P */

#define PLD	  '\113'	/* upper case K = ANSI partial line down */
#define PLU	  '\114'	/* upper case L = ANSI partial line up */
#define FLU	  '\115'	/* upper case M = ANSI full line up */

#define E_NINE	  '\071'	/* 9: nroff ESC9 = partial line down */
#define E_EIGHT   '\070'	/* 8: nroff ESC8 = partial line up */
#define E_SEVEN   '\067'	/* 7: nroff ESC7 = full line up */

#define escend(x) ((x!='\120')&&(x!='\133')&&(x>='\100')&&(x<='\176'))


/* Added a few macros for 8bit support */
#define is_7bit_cntrl(ch)	((unsigned char)ch < 040 || (unsigned char)ch == 0177)

FILE	*input = stdin, *output = stdout;	/* input and output	*/
char	buf[MAXREP][BUFWIDTH];			/* buffer for output	*/
int	maxcol[MAXREP] = {-1};			/* maximum columns	*/
int	lineno;					/* line number		*/
int	width = DEFWIDTH;			/* default line length	*/
int	length = DEFHEIGHT;			/* page length		*/
int	indent;					/* indentation length	*/
int	npages = MAXCOPIES;			/* number of copies	*/
int	literal;				/* print control chars. */
int	error;					/* error return status	*/
int     esclen;                                 /* num escape chars     */
int     maxrep;                                 /* current depth in buf */
int     col;                                    /* column position      */
char	*name;					/* user's login name	*/
char	*host;					/* user's machine name	*/
char	*acctfile;				/* accounting info. file*/
int	kindofile = EMPTY_FILE;			/* initial kind of file */
int	tmppagecount = 0, tmplinecount = 0;	/* tmp counters		*/

char	*imgptr;				/* image data pointers	*/
short	*cmpptr;				/* color map pointer	*/
struct	imghdr im;				/* image file header	*/
char	*malloc();				/* malloc pointer	*/

char *setlocale();				/* Intl. function       */

unsigned  size;					/* the usual		*/
int	escflg =  0;	       /* escape sequence flag, 1 = in progress */
int	lstchr;					/* last character	*/
int	rotated = 0;				/* image rotated	*/

void		exit(), bcopy(), syslog();
unsigned	sleep();

/* sixmap is the dither pattern used to create sixel output bytes */
struct sixmap {
	char s0, s1, s2;
} base_sixmap[] = {
	{ 0+SOFF, 0+SOFF, 0+SOFF },
	{ 4+SOFF, 0+SOFF, 0+SOFF },
	{ 4+SOFF, 0+SOFF, 1+SOFF },
	{ 4+SOFF, 4+SOFF, 1+SOFF },
	{ 6+SOFF, 4+SOFF, 1+SOFF },
	{ 6+SOFF, 6+SOFF, 1+SOFF },
	{ 6+SOFF, 6+SOFF, 6+SOFF },
	{ 7+SOFF, 6+SOFF, 6+SOFF },
	{ 7+SOFF, 6+SOFF, 7+SOFF },
	{ 7+SOFF, 7+SOFF, 7+SOFF }
};

struct sixmap offset_sixmap[] = {
	{  0+SOFF,  0+SOFF,  0+SOFF },
	{ 32+SOFF,  0+SOFF,  0+SOFF },
	{ 32+SOFF,  0+SOFF,  8+SOFF },
	{ 32+SOFF, 32+SOFF,  8+SOFF },
	{ 48+SOFF, 32+SOFF,  8+SOFF },
	{ 48+SOFF, 48+SOFF,  8+SOFF },
	{ 48+SOFF, 48+SOFF, 48+SOFF },
	{ 56+SOFF, 48+SOFF, 48+SOFF },
	{ 56+SOFF, 48+SOFF, 56+SOFF },
	{ 56+SOFF, 56+SOFF, 56+SOFF }
};

/*
 * In order to merge the functionality of the ln03, lg31 and lg02 filters
 * we need a table of structure to contain the difference information.
 * we index into this with an enum.
 *
 * We pass the known names by which the filter can be called from the
 * via pre-processor definitions
 */

static enum which_filter_e {		/* Enumerate different printers supported */
	ln03,
	lg31,
	lg02,
	la75
    } which_filter;

static struct filter_info {		/* Printer specific info */
	char *fi_name;
	char *fi_sixel_intro;
} filter_info[] = {
	{ LN03OF, "\033P9;0;1q\"1;1-"},
	{ LG31OF, "\033P9;0;10q\"1;1-"},
	{ LG02OF, "\033P9;0;0q\"1;1-"},
	{ LA75OF, "\033P9;;5;q\"1;1-"},
	{ NULL }
};

static char *name;		/* Name program called with */

static struct filter_info *
set_which_filter(arg0)
	char *arg0;
{
	register struct filter_info *fip;

	if ((name=strrchr(arg0, '/')) == NULL) {
		name = arg0;
	} else {	
		name++;
	}
	for (fip=filter_info; fip->fi_name != NULL; fip++) {
		if (!strcmp(fip->fi_name, name)) {
			which_filter =
			    (enum which_filter_e) (fip - filter_info);
			return(fip);
		}
	}
	fprintf(stderr, "%s: Must be called one of following:\n\t", name);
	for (fip=filter_info; fip->fi_name != NULL; fip++) {
		fprintf(stderr, " %s", fip->fi_name);
	}
	putc('\n', stderr);
	exit(2);
	/* not reached */
	return NULL;
}

/* The general strategy here is to reset the printer to initial state,
   sleep for five seconds for stability, process the command line arguments,
   open the syslog file for log information, determine the input
   stream "file type", call the filter code, and then optionally
   process accounting information upon completion. Informational
   and failure conditions are logged to syslog.
*/
main(argc, argv)
	int argc;
	char *argv[];
{
	register int i;
	register char *cp;
	register struct filter_info *fip;

	fip = set_which_filter(argv[0]);
	/* 
	 * The general strategy here is to reset the printer to initial state,
   	 * sleep for five seconds for stability, process the command line arguments,
   	 * determine the input stream "file type", call the filter code, and then 
	 * optionally process accounting information.
	 */

	setlocale(LC_CTYPE, "ENG_GB.MCS");
	fprintf(output, "\033c");		/* reset to initial state */
	sleep(5);
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
	openlog(name, LOG_PID);

	switch(which_filter) {
	case ln03:
		if (width > 80) { /* switch to landscape mode */
			fprintf(output,"\033[15m"); /* change font */
			fprintf(output,"\033[7 J"); /* A4 page format */
			fprintf(output,"\033[66t"); /* 66 lines/page */
			fprintf(output,"\033[8 L"); /* vp = 12 lines/30mm */
		}
		break;

	case lg31:
	case lg02:
		break;

	case la75:
		if ( width > 80 ) /* change HOR pitch to 16.5 cpi */
		    fprintf(output,"\033[4w");
		break;

	default:
		break;
	}
	kindofile = determinefile(fileno(input));

	error = filter();

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

/* Here is where all the real output work begins. We switch
   to the appropriate code for the determined file type stream.
*/
static int
filter()
{
	register int i = 0;
	register char *cp;
	register int ch;
	register short *tmpptr, *tmp;
	unsigned int temp;
	int done, linedone;
	char *limit;
	register struct filter_info *fip= &filter_info[(int)which_filter];

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

	case ANSI_FILE:
		print_ansi_file();
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
			/* do RGB to YIQ conversion */
			tmpptr=cmpptr;
			for(i=0;i!=256;i++) {
				tmp=tmpptr;
				temp= *tmpptr * .30;
				tmpptr++;
				temp+= *tmpptr * .59;
				tmpptr++;
				temp+= *tmpptr * .11;
				tmpptr++;
				*tmp=(255-temp);
			}
		}
		error = readXimgdat();
		if(error) {
			syslog(LOG_INFO,"Failed to use image data");
			return(1);
		}
		switch(which_filter) {
		case ln03:
		case lg31:
		case lg02:
		default:
			if(im.spbxnm > MAXP_PIX_WIDTH) {
				fprintf(output,"\033[11h\033[7 I");
				fprintf(output,"\033[?21 J");
				fprintf(output,"\033[?52l");
				fprintf(output,"\033[1;3150s");
				rotated++;
				if(im.spbxnm > MAXL_PIX_WIDTH) {
					im.spbxnm = MAXL_PIX_WIDTH;
				}
				if(im.spbynm > MAXL_PIX_WIDTH) {
					tmppagecount = im.spbynm/MAXP_PIX_WIDTH;
					tmplinecount = MAXP_PIX_WIDTH;
				}
			} else { /* im.spbxnm <= MAXP_PIX_WIDTH */
				fprintf(output,"\033[11h\033[7 I");
				fprintf(output,"\033[?20 J");
				fprintf(output,"\033[?52l");
				fprintf(output,"\033[1;2400s");
				if(im.spbynm > MAXP_PIX_HEIGHT) {
					tmppagecount = im.spbynm/MAXP_PIX_HEIGHT;
					tmplinecount = MAXP_PIX_HEIGHT;
				}
			}
			break;
		case la75:
			/* Now LA75 has only one mode...so go for that */
			if ( im.spbxnm > MAXP_PIX_WIDTH )
			    im.spbxnm =  MAXP_PIX_WIDTH;
			if ( im.spbynm > MAXP_PIX_HEIGHT ){
				tmppagecount = im.spbynm/MAXP_PIX_HEIGHT;
				tmplinecount = MAXP_PIX_HEIGHT;
			}
			break;
		}
		fprintf(output, fip->fi_sixel_intro);

		error = dosixel();
		if(error) {
			syslog(LOG_INFO,"Failed to 'sixelize' data");
			fprintf(output, "\033\\");
			fprintf(output, "\033c");
			sleep(5);
			fprintf(output, "\014");
			fflush(output);
			return(1);
		}
		break;
	case TEXT_FILE:
	case CTEXT_FILE:
	case ATEXT_FILE:
	case RTEXT_FILE:
	case FTEXT_FILE:
	default:
		memset(buf, ' ', (size_t) sizeof(buf));
		done = 0;

		escflg = 0;	/* is escape/control sequence in progress? */
		globi = 0;
		while (!done) {
			col = indent;
			esclen = 0;
			maxrep = -1;
			linedone = 0;
			while (!linedone) {
				ch = globi < in ? filestorage[globi++] : getc(input);
				if (((escflg==0)&&(ch==ESC))||escflg)
				    eschdl(output,ch); /* deal with escape character */
				else 
				    if ( literal && iscntrl(ch) &&
					(ch != '\n') && (ch != EOF)) {
					    cp = &buf[0][col]; /* Since literal mode..everything is the
								  first row itself.*/
					    maxrep = 0;
					    if (is_7bit_cntrl(ch)){
						    if (ch == 0177){
							    *cp++ = '^';
							    *cp   = '?';
							    col ++;
						    } else{ /* It is < 040 */
							    *cp++ = '^';
							    *cp   = ch + '@';
							    col ++;
						    }
					    } else{ /* It is 8bit cntrl */
						    if ((unsigned char)ch < 0240){
							    *cp++ = 'M';
							    *cp++ = '-';
							    ch   &= 0177;
							    *cp   = ch + '@';
							    col  += 2;
						    } else {
							    *cp++ = 'M';
							    *cp++ = '-';
							    *cp   = '?';
							    col  += 2;
						    }
					    }
					    if ( col > maxcol[0] )
						maxcol[0] = col;
					    col++; /* col points to next blank entry in buf */
				    }
				    else { /* regular characters */
					    switch (ch) {
					    case EOF:
						    linedone = done = 1;
						    ch = '\n';
						    break;
  
					    case '\f': /* new page on form feed */
						    lineno = length;
					    case '\n': /* new line */
						    if (maxrep < 0)
							maxrep = 0;
						    linedone = 1;
						    break;
  
					    case '\b': /* backspace */
						    if (--col < indent)
							col = indent;
						    break;
  
					    case '\r': /* carriage return */
						    col = indent;
						    break;
  
					    case '\t': /* tab */
						    col = ((col - indent) | 07) + indent + 1;
						    break;
  
					    case '\031': /* end media */
						    /*
						     * lpd needs to use a different filter to
						     * print data so stop what we are doing and
						     * wait for lpd to restart us.
						     */
						    ch = globi < in ? filestorage[globi++] : getc(input);
						    if (ch == '\1') {
							    fflush(output);
							    kill(getpid(), SIGSTOP);
							    break;
						    } else {
							    if(globi <= in) {
								    globi--;
							    } else {
								    ungetc(ch, input);
							    }
							    ch = '\031';
						    }
  
					    default: /* everything else */
						    addtobuf(ch);
						    break;
					    } /* end switch */
				    } /* end else */

			}	/* end while not linedone */

			/* print out lines */
			for (i = 0; i <= maxrep; i++) {
				switch(which_filter) {
				case la75:
					if (i == 0) {
						break;
					} else {
						/* drop through */
					}
				case ln03:
				case lg31:
				case lg02:
				default:
					putc('\r', output);
				}
				for (cp = buf[i], limit = cp+maxcol[i]; cp <= limit;) {
					putc(*cp, output);
					*cp++ = ' ';
				}
				if (i == maxrep)
				    putc(ch, output);
				if (++lineno >= length) {
					npages++;
					lineno = 0;
					if (length < 66)
					    putc('\f',output); /* FF for length < 66 */
				}
				maxcol[i] = -1;
			}
		}
		if (lineno) {	/* be sure to end on a page boundary */
			putc('\f',output);
			npages++;
		}  
		fprintf(output,"\033c"); /* reset printer defaults */
		fflush(output);	/* make sure reset goes out */
		sleep(6);	/* some printers eat lines during reset so wait */
		break;
	}
	return(0);
}


/*****************************************************************
* Prints a file already formatted with ansi control sequences.   *
*  The lf and cr options on the lg02 should be disabled. Fixes   *
*  a problem in lg02 when printing block characters in landscape *
*  mode. This procedure sends file directly to printer, however, *
*  it adds a carraige return character for linefeed character    *
*  seen.                                                         *
******************************************************************/
print_ansi_file ()
{
	int     done;
	register char ch;
	
	globi = 0;
	done = 0;
	while (!done) {
		ch = globi < in ? filestorage[globi++] : getc(input);
		switch (ch) {
			case EOF: 
				done = 1;
				break;
			case '\n':                   /* new line */
				putc('\r',output);   /* insert CR character */
				putc(ch, output);
				break;
			default:
				putc(ch, output);
				break;
		}
	}
	fprintf(output,"\033\143");  /* reset printer defaults - "escape c" */
	fflush(output);		     /* make sure reset goes out            */
	sleep(2);		     /* make sure the reset sets            */
}
					
/****************************************************************
  Adds the character specified to buffer - if it will fit on line
*****************************************************************/

addtobuf (ch)
register char      ch;
{
	register char  *cp;
        int  i;

        if ((col >= (width + esclen) || iscntrl(ch)) && ch != ESC){
		col++;
		return(0);
        }
        cp = &buf[0][col];
        for (i = 0; i < MAXREP; i++) {
		if (i > maxrep)
		    maxrep = i;
		if (*cp == ' ') {
			*cp = ch;
			if (col > maxcol[i])
			    maxcol[i] = col;
			break;
		}
		cp += BUFWIDTH;
        }
        col++;
        return(1);
}

/****************************************************************/
/*								*/
/*	eschdl - escape sequence handler			*/
/*								*/
/*	This routine intercepts escape sequences for the purpose*/
/*	of pass through.					*/
/*								*/
/****************************************************************/
eschdl(o,c)
int c;
FILE  *o;
{
	if(escflg==0)
	    {			/* set escflg=1 => ready to receive 2nd seqchar*/
		    escflg=1;
	    }
	else	switch(escflg)
	    {
	    case 1:		/* second character of escseq		*/
		    switch(c)
			{
			case E_NINE: /* added to buffer - hopefully in the correct place */
				addtobuf (ESC);
				addtobuf (PLD);
				esclen += 2;
				escflg = 0;
				break;
			case E_EIGHT: /* added to buffer - hopefully in the correct place */
				addtobuf (ESC);
				addtobuf (PLU);
				esclen += 2;
				escflg = 0;
				break;
			case E_SEVEN: /* added to buffer - hopefully in the correct place */
				addtobuf (ESC);
				addtobuf (FLU);
				esclen += 2;
				escflg = 0;
				break;
			case UCP:
				escflg=2; /*ctrl str pass thru mode=8 */
				lstchr=c;
				putc(ESC,o);
				putc(c,o);
				break;
			default:
				escflg=3; /* set seq pass thru mode*/
				putc(ESC,o);
				putc(c,o);
				break;
			}
		    break;
	    case 2:		/* ctrl string pass through mode	*/
		    if((lstchr==ESC) && (c==BSLH))
			{
				escflg=0;
				lstchr=0;
			}
		    else lstchr=c; /* save it for next pass */
		    putc(c,o);
		    break;
	    case 3:
		    if(escend(c))
			escflg=0; /* turn off esc handler if at end  */
		    putc(c,o);
		    break;
	    }
	return(0);
}

/* 
 * If the file stream is of the XIMAGE_FILE type this routine is called
 * from filter().  The routine creates a sixel output stream to send
 * the printer based on the input data from an image file. Currently,
 * only a frame buffer image of raw red, green, blue points or a
 * GPX workstation "savimg"  image are valid data streams.  Sixel
 * output is initialized and then the processing begins.
 * The current design of the algorithm dithers the data to a 3 x 3 matrix
 * of points using the structure sixmap.
 */
dosixel()
{
	register int i;
	int xcnt,ycnt,n = 1,iindex,ij=0;
	unsigned char	*srcptr;
	unsigned char	*nxtptr;
	unsigned short sl;
	unsigned int	temp;
	struct sixmap *Sixmap;
	struct sixmap *Sixmap_offset;
	char *base_band,*pb;
	int base_band_length = 0;
	int count = 1;
	char lastc = 0;
	register char *cp;

	srcptr=(unsigned char *)imgptr;
	ycnt=im.spbynm;
	if(im.format != ITS)
	    nxtptr=srcptr+im.spbxnm * n;
	else
	    nxtptr=srcptr+(im.spbxnm * n * 3);
	pb = base_band = (char *)malloc((unsigned) 6 * im.spbxnm);
	Sixmap = base_sixmap;
	Sixmap_offset = offset_sixmap;
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
				sl = *(cmpptr + 3*(*srcptr));
			} else {
				/* need to convert to YIQ */
				temp= *srcptr++ * .30;
				temp+= *srcptr++ * .59;
				temp+= *srcptr * .11;
				temp=(255-temp);
				sl = (unsigned short)temp;
			}
			iindex = (sl * 9) / 255;
			for(i=0;i<3;i++) {
				*(base_band+i) = SOFF;
			}
			if(!ij) {
				*base_band++ = Sixmap[iindex].s0;
				*base_band++ = Sixmap[iindex].s1;
				*base_band++ = Sixmap[iindex].s2;
			} else {
				*base_band++ = Sixmap_offset[iindex].s0;
				*base_band++ = Sixmap_offset[iindex].s1;
				*base_band++ = Sixmap_offset[iindex].s2;
			}
			base_band_length+=3;
			srcptr+=n;
			xcnt-=n;
		}
		if(!ij) {
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
			count = 1;
			ij++;
		} else {
			cp = pb;
			lastc = *cp++;
			for(i=0;i < base_band_length;i++) {
				if(*cp != lastc) {
					if(count >= 4)
					    fprintf(output,"!%d%c",count,lastc);
					else
					    while(count--)
						putc(lastc,output);
					count = 1;
					lastc = *cp;
				} else
				    count++;
				cp++;
			}
			count = 1;
			ij = 0;
			fprintf(output,"-");
		}
		srcptr=nxtptr;
		if(im.format != ITS)
		    nxtptr=srcptr+im.spbxnm * n;
		else
		    nxtptr=srcptr+(im.spbxnm * n * 3);
		ycnt-= n;
	
		if(tmppagecount) {
			tmplinecount--;
			if(tmplinecount == 0) {
				fprintf(output,"+");
				tmppagecount--;
				if(im.spbxnm >= MAXL_PIX_WIDTH) {
					if(im.spbynm > MAXL_PIX_WIDTH) {
						tmplinecount = MAXP_PIX_WIDTH;
					}
				}
				if(im.spbxnm <= MAXP_PIX_WIDTH) {
					if(im.spbynm > MAXP_PIX_HEIGHT) {
						tmplinecount = MAXP_PIX_HEIGHT;
					}
				}
			}
		}
	}
	fprintf(output,"+");
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
	if((imgptr=(char *) malloc(size))==NULL)
		return(1);
	bcopy(&filestorage[globi],imgptr,in-globi);
	fread(imgptr+(in-globi), (char)size-(in-globi), 1, input);
	return(0);
}
