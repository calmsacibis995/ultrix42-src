
#ifndef lint
static	char	*sccsid = "@(#)lg31of.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * lg31of.c	
 *
 * Modification history
 *
 * LG31 LINE PRINTER FILTER
 *
 * 23-Nov-88 - Dave Gray (gray)
 *
 *      Fixed bad pointer reference in strncmp
 *
 * The LG31 filter is a modified version of the ln03of filter. They are
 * basically the same.
 *
 *	5-May-88 Pradeep Chetal (chetal)
 *	 - made 8-bit clean
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

#define MAXWIDTH		132		/* maximum char. width	*/
#define BUFWIDTH		182		/* maximum buf.  width	*/
#define MAXREP			10		/* buffer depth		*/
#define DEFWIDTH		80		/* width char. count	*/
#define DEFHEIGHT		66		/* length char. count	*/
#define MAXP_PIX_WIDTH		800		/* portrait pixel width */
#define MAXP_PIX_HEIGHT		1040		/* portrait pix. height */
#define MAXL_PIX_WIDTH		1040		/* landscape p. width	*/
#define MAXL_PIX_HEIGHT		800		/* landscape p. length	*/
#define MAXCOPIES		1		/* default # of copies	*/
#define HOW_MUCH_TO_CHECK	4096		/* input buffer amount	*/
#define EMPTY_FILE		0		/* 0-10,77 file types	*/
#define EXECUTABLE_FILE		1
#define ARCHIVE_FILE		2
#define DATA_FILE		3
#define TEXT_FILE		4
#define CTEXT_FILE		5
#define ATEXT_FILE		6
#define RTEXT_FILE		7
#define FTEXT_FILE		8
#define CAT_FILE		9
#define XIMAGE_FILE		10
#define ITS			77
#define SOFF			'\077'		/* sixel element offset */
#define ESC	  '\033'	/* escape sequence introducer */
#define BSLH	  '\134'	/* back slash */
#define UCP	  '\120'	/* upper case P */
#define PLD	  '\113'	/* upper case K = lg31of partial line down */
#define PLU	  '\114'	/* upper case L = lg31of partial line up */
#define FLU	  '\115'	/* upper case M = lg31of full line up */
#define E_NINE	  '\071'	/* 9: nroff ESC9 = partial line down */
#define E_EIGHT   '\070'	/* 8: nroff ESC8 = partial line up */
#define E_SEVEN   '\067'	/* 7: nroff ESC7 = full line up */
#define escend(x) ((x!='\120')&&(x!='\133')&&(x>='\100')&&(x<='\176'))
#define MAX(a,b)		((a) < (b) ? (b) : (a)) /* useful macros */
#define MAXIMUM(a,b,c)		(MAX(MAX((a),(b)),(c)))
#define MIN(a,b)		((a) > (b) ? (b) : (a))
#define MINIMUM(a,b,c)		(MIN(MIN((a),(b)),(c)))
/* Added a few macros for 8bit support */
#define is_7bit_cntrl(ch)	((unsigned char)ch < 040 || (unsigned char)ch == 0177)

FILE	*input = stdin, *output = stdout;	/* input and output	*/
char	filestorage[HOW_MUCH_TO_CHECK];		/* first chunk of pipe	*/
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
int	globi = 0, in = 0;			/* global i and in vars.*/
int	kindofile = EMPTY_FILE;			/* initial kind of file */
int	tmppagecount = 0, tmplinecount = 0;	/* tmp counters		*/
char	*imgptr; 				/* image data pointers	*/
short	*cmpptr;				/* color map pointer	*/
struct	imghdr im;				/* image file header	*/
char	*malloc();				/* malloc pointer	*/
unsigned  size;					/* the usual		*/
int	escflg =  0;	       /* escape sequence flag, 1 = in progress */
int	lstchr;					/* last character	*/
int	rotated = 0;				/* image rotated	*/
char *setlocale();                              /* Intl. function 	*/
char	*fort[] =  { "function","subroutine","common",
		  "dimension","block","integer",
		  "real","data","double",0 };
char	*asc[] =   { "chmk","mov","tst","clr","jmp",0 };
char	*c[] =	   { "int","char","float","double",
		  "struct","extern",0 };
char	*as[] =    { "globl","byte","align","text",
		  "data","comm",0 };

/* sixmap is the dither pattern used to create sixel output bytes */
struct sixmap {
	char s0,s1,s2;
} base_sixmap[] = {
	{ 0+SOFF, 0+SOFF, 0+SOFF } ,
	{ 4+SOFF, 0+SOFF, 0+SOFF } ,
	{ 4+SOFF, 0+SOFF, 1+SOFF } ,
	{ 4+SOFF, 4+SOFF, 1+SOFF } ,
	{ 6+SOFF, 4+SOFF, 1+SOFF } ,
	{ 6+SOFF, 6+SOFF, 1+SOFF } ,
	{ 6+SOFF, 6+SOFF, 6+SOFF } ,
	{ 7+SOFF, 6+SOFF, 6+SOFF } ,
	{ 7+SOFF, 6+SOFF, 7+SOFF } ,
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

void		exit(), bcopy(), syslog();
unsigned	sleep();

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

	setlocale(LC_CTYPE,"ENG_GB.MCS");
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

	/***************	if (width > 80) {  switch to landscape mode 
					fprintf(output,"\033[15m");  change font 
					fprintf(output,"\033[7 J");  A4 page format 
					fprintf(output,"\033[66t");  66 lines/page 
					fprintf(output,"\033[8 L");  vp = 12 lines/30mm 
				} *************/
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

	openlog("lg31of",LOG_PID);
	kindofile = determinefile();
	error = lg31of();
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

/* The general strategy here is to "sniff" the incoming data to
   determine how to process output characters to the printer. Not
   all possible data streams are reflected here but a wide enough
   variety is covered to handle printing of just about all kinds
   of data streams.  This code is derived originally from the "file"
   command code and modified to reflect its current usage.  Note
   that currently only the first HOW_MUCH_TO_CHECK bytes are
   used so that if multiple files are concatenated together and
   then sent through the filter the first file will determine
   how the filter will act.
*/
determinefile()
{
	register int j = 0, nl = 0;
	register char ch, *chptr;

	in = read(fileno(input), &filestorage[0], HOW_MUCH_TO_CHECK);
	if((chptr = index(&filestorage[0],'\014')) != NULL)
		globi = chptr + 1 - &filestorage[0];
	if(in == 0 && globi == 0){
		return(EMPTY_FILE);
	}
	switch(*(int *)&filestorage[globi]) {

	case 0413:
	case 0410:
	case 0411:
	case 0407:
		return(EXECUTABLE_FILE);
		break;
	case 0177555:
	case 0177545:
	case 070707:
	case 027456:
		return(ARCHIVE_FILE);
		break;
	case 04553207:
		return(XIMAGE_FILE);
		break;
	default:
		break;
	}
	if((strncmp(filestorage+globi, "!<arch>\n__.SYMDEF", 17) == 0) ||
	   (strncmp(filestorage+globi, "!<arch>\n", 8) == 0)) {
		return(ARCHIVE_FILE);
	}
	if(ccom() == 0)
		goto notc;
	while(filestorage[globi] == '#'){
		j = globi;
		while(filestorage[globi++] != '\n'){
			if(globi - j > 255){
				return(DATA_FILE);
			}
			if(globi >= in)
				goto notc;
		}
		if(ccom() == 0)
			goto notc;
	}
check:
	if(lookup(c) == 1){
		while((ch = filestorage[globi++]) != ';' &&
		      ch != '{')
			if(globi >= in)
				goto notc;
		return(CTEXT_FILE);
	}
	nl = 0;
	while(filestorage[globi] != '('){
		if(filestorage[globi] <= 0)
			goto notas;
		if(filestorage[globi] == ';'){
			globi++;
			goto check;
		}
		if(filestorage[globi++] == '\n')
			if(nl++ > 6)
				goto notc;
		if(globi >= in)
			goto notc;
	}
	while(filestorage[globi] != ')'){
		if(filestorage[globi++] == '\n')
			if(nl++ > 6)
				goto notc;
		if(globi >= in)
			goto notc;
	}
	while(filestorage[globi] != '{'){
		if(filestorage[globi++] == '\n')
			if(nl++ > 6)
				goto notc;
		if(globi >= in)
			goto notc;
	}
	return(CTEXT_FILE);
notc:
	globi = 0;
	while(filestorage[globi] == 'c' || filestorage[globi] == '#'){
		while(filestorage[globi++] != '\n')
			if(globi >= in)
				goto notfort;
	}
	if(lookup(fort) == 1){
		return(FTEXT_FILE);
	}
notfort:
	globi=0;
	if(ascom() == 0)
		goto notas;
	j = globi-1;
	if(filestorage[globi] == '.'){
		globi++;
		if(lookup(as) == 1){
			return(ATEXT_FILE);
		}
		else if(filestorage[j] == '\n' &&
			isalpha(filestorage[j+2])){
			return(RTEXT_FILE);
		}
	}
	while(lookup(asc) == 0){
		if(ascom() == 0)
			goto notas;
		while(filestorage[globi] != '\n' && filestorage[globi++] != ':')
			if(globi >= in)
				goto notas;
		while(filestorage[globi] == '\n' || filestorage[globi] == ' ' ||
		      filestorage[globi] == '\t')
			if(globi++ >= in)
				goto notas;
		j = globi-1;
		if(filestorage[globi] == '.'){
			globi++;
			if(lookup(as) == 1){
				return(ATEXT_FILE);
			}
			else if(filestorage[j] == '\n' &&
				isalpha(filestorage[j+2])){
				return(RTEXT_FILE);
			}
		}
	}
	return(ATEXT_FILE);
notas:
	for(globi=0; globi < in; globi++)if(filestorage[globi]&0200){
		if ((unsigned)filestorage[0]==(unsigned)'\100' && 
		    (unsigned)filestorage[1]==(unsigned)'\357') {
			return(CAT_FILE);
		}
	}
	return(TEXT_FILE);
}
lookup(tab)
register char *tab[];
{
	register char r;
	register int k,j,l;

	while(filestorage[globi] == ' ' || filestorage[globi] == '\t' ||
	      filestorage[globi] == '\n')globi++;
	for(j=0; tab[j] != 0; j++){
		l=0;
		for(k=globi; ((r=tab[j][l++]) == filestorage[k] &&
		    r != '\0');k++);
		if(r == '\0')
			if(filestorage[k] == ' ' || filestorage[k] == '\n' ||
			   filestorage[k] == '\t' || filestorage[k] == '{' ||
			   filestorage[k] == '/'){
				globi=k;
				return(1);
			}
	}
	return(0);
}

ccom()
{
	register char cc;

	while((cc = filestorage[globi]) == ' ' || cc == '\t' ||
	      cc == '\n')
		if(globi++ >= in)
			return(0);
	if(filestorage[globi] == '/' && filestorage[globi+1] == '*'){
		globi += 2;
		while(filestorage[globi] != '*' || filestorage[globi+1] != '/'){
			if(filestorage[globi] == '\\')
				globi += 2;
			else
				globi++;
			if(globi >= in)
				return(0);
		}
		if((globi += 2) >= in)
			return(0);
	}
	if(filestorage[globi] == '\n')
		 if(ccom() == 0)
			return(0);
	return(1);
}

ascom()
{
	while(filestorage[globi] == '/') {
		globi++;
		while(filestorage[globi++] != '\n')
			if(globi >= in)
				return(0);
		while(filestorage[globi] == '\n')
			if(globi++ >= in)
				return(0);
	}
	return(1);
}

/* Here is where all the real output work begins. We switch
   to the appropriate code for the determined file type stream.
*/
lg31of()
{
	register int i = 0;
	register char  *cp;
	register int ch;
	register short *tmpptr, *tmp;
	unsigned int temp;
	int done, linedone;
	char *limit;

	switch(kindofile) {
		case EMPTY_FILE:
			break;
		case EXECUTABLE_FILE:
		case ARCHIVE_FILE:
		case DATA_FILE:
		case CAT_FILE:
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
			if(im.spbxnm > MAXP_PIX_WIDTH) {
				fprintf(output,"\033[11h\033[7 I");
				fprintf(output,"\033[?21 J");
				fprintf(output,"\033[?52l");
				fprintf(output,"\033[1;3150s");
				fprintf(output,"\033P9;0;10q\"1;1-");
				rotated++;
				if(im.spbxnm > MAXL_PIX_WIDTH) {
					im.spbxnm = MAXL_PIX_WIDTH;
				}
				if(im.spbynm > MAXL_PIX_WIDTH) {
					tmppagecount = im.spbynm/MAXP_PIX_WIDTH;
					tmplinecount = MAXP_PIX_WIDTH;
				}
			}
			if(im.spbxnm <= MAXP_PIX_WIDTH) {
				fprintf(output,"\033[11h\033[7 I");
				fprintf(output,"\033[?20 J");
				fprintf(output,"\033[?52l");
				fprintf(output,"\033[1;2400s");
				fprintf(output,"\033P9;0;10q\"1;1-");
				if(im.spbynm > MAXP_PIX_HEIGHT) {
					tmppagecount = im.spbynm/MAXP_PIX_HEIGHT;
					tmplinecount = MAXP_PIX_HEIGHT;
				}
			}
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
			for (cp = buf[0], limit = buf[MAXREP]; cp < limit; *cp++ = ' ');
			done = 0;

			escflg = 0;		/* is escape/control sequence in progress? */
			globi = 0;
                        while (!done) {
                            col = indent;
                            esclen = 0;
                            maxrep = -1;
                            linedone = 0;
                            while (!linedone) {
                                ch = globi < in ? filestorage[globi++] : getc(input);
                                if (((escflg==0)&&(ch==ESC))||escflg)
                                        eschdl(output,ch);   /* deal with escape character */
                                else 
                                      /* chetal: literal code added follows */
                                      if ( literal && iscntrl(ch) && ( (ch != '\n')
                                            && (ch != EOF) ) ){
                                          cp = &buf[0][col];    /* Since literal mode..everything is the
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
  
                                          case '\f':                /* new page on form feed */
                                                  lineno = length;
                                          case '\n':                /* new line */
                                                  if (maxrep < 0)
                                                          maxrep = 0;
                                                  putc('\r',output);   /* LITOUT eight bit fix */
                                                  linedone = 1;
                                                  break;
  
                                          case '\b':                /* backspace */
                                                  if (--col < indent)
                                                          col = indent;
                                                  break;
  
                                          case '\r':                /* carriage return */
                                                  col = indent;
                                                  break;
  
                                          case '\t':                /* tab */
                                                  col = ((col - indent) | 07) + indent + 1;
                                                  break;
  
                                          case '\031':                /* end media */
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
  
                                                  default:   /* everything else */
                                                          addtobuf(ch);
                                                          break;
                                                  } /* end switch */
                                    } /* end else */

                            } /* end while not linedone */

                            /* print out lines */
                            for (i = 0; i <= maxrep; i++) {
                                    for (cp = buf[i], limit = cp+maxcol[i]; cp <= limit;) {
                                            putc(*cp, output);
                                            *cp++ = ' ';
                                    }
                                    if (i < maxrep)
                                            putc('\r', output);
                                    else
                                            putc(ch, output);
                                    if (++lineno >= length) {
                                            npages++;
                                            lineno = 0;
                                            if (length < 66)
                                                    putc('\f',output);  /* FF for length < 66 */
                                    }
                                    maxcol[i] = -1;
                            }
                        }
			if (lineno) {		/* be sure to end on a page boundary */
				putc('\f',output);
				npages++;
			}  
			fprintf(output,"\033\143");  /* reset printer defaults */
			fflush(output);		     /* make sure reset goes out */
			sleep(6);		/* some printers eat lines during reset so wait */
			break;
		}
	return(0);
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
	{		/* set escflg=1 => ready to receive 2nd seqchar*/
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
					escflg=3;  /* set seq pass thru mode*/
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
			else lstchr=c;	/* save it for next pass */
			putc(c,o);
			break;
		case 3:
			if(escend(c))
				escflg=0;/* turn off esc handler if at end  */
			putc(c,o);
			break;
		}
return(0);
}

/* If the file stream is of the XIMAGE_FILE type this routine is called
   from lg31of.  The routine creates a sixel output stream to send
   the printer based on the input data from an image file. Currently,
   only a frame buffer image of raw red, green, blue points or a
   GPX workstation "savimg"  image are valid data streams.  Sixel
   output is initialized and then the processing begins.
   The current design of the algorithm dithers the data to a 3 x 3 matrix
   of points using the structure sixmap.
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
