#ifndef lint
static char sccsid[] = "@(#)la75.c	4.1	ULTRIX	7/2/90";
#endif

/*
 *   LA75.c:
 *      23-Nov-88 Dave Gray (gray)
 *      Fixed bad pointer reference in strncmp
 *
 *	19-Dec-87 Pradeep Chetal (chetal)
 *   	1) Made modifications for the width > 80 chars code, and
 * 	   handle "literals" as stated in the lpr command.
 *
 *	2) Handle Xdumps by converting to sixel format.
 *
 *	3) Modified Escape Handler to embed nroff escape sequences
 *	   for underlining, superscripting, & subscripting.
 *
 * 	filter which reads the output of nroff and converts lines
 *	with ^H's to overwritten lines.  Thus this works like 'ul'
 *	but is much better: it can handle more than 2 overwrites
 *	and it is written with some style.
 *	modified by kls to use register references instead of arrays
 *	to try to gain a little speed.
 *
 *	5-May-88 Pradeep Chetal (chetal)
 *	4) made this 8-bit clean
 */

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <a.out.h>
#include <imghdr.h>
#include <locale.h>

#define MAXWIDTH  132				/* maximum char.  width  */
#define BUFWIDTH  182				/* maximum buf.  width  */
#define MAXREP    10				/* buffer depth         */
#define MAXP_PIX_WIDTH          800             /* portrait pixel width */
#define MAXP_PIX_HEIGHT         1040            /* portrait pix. height */
#define HOW_MUCH_TO_CHECK       4096            /* input buffer amount */
#define EMPTY_FILE              0               /* 0-10,77 file types */
#define EXECUTABLE_FILE         1
#define ARCHIVE_FILE            2
#define TEXT_FILE               4
#define XIMAGE_FILE             10
#define ITS                     77
#define SOFF                    '\077'          /* sixel element offset */
#define MAX(a,b)                ((a) < (b) ? (b) : (a)) /* useful macros */
#define MAXIMUM(a,b,c)          (MAX(MAX((a),(b)),(c)))
#define MIN(a,b)                ((a) > (b) ? (b) : (a))
#define MINIMUM(a,b,c)          (MIN(MIN((a),(b)),(c)))
#define ESC       '\033'        /* escape sequence introducer */
#define BSLH      '\134'        /* back slash */
#define UCP       '\120'        /* upper case P */
#define PLD       '\113'        /* upper case K = la75of partial line down */

#define PLU       '\114'        /* upper case L = la75of partial line up */
#define E_NINE    '\071'        /* 9: nroff ESC9 = partial line down */
#define E_EIGHT   '\070'        /* 8: nroff ESC8 = partial line up */
#define escend(x) ((x!='\120')&&(x!='\133')&&(x>='\100')&&(x<='\176'))
/* Added a few macros for 8bit support */
#define is_7bit_cntrl(ch)	((unsigned char)ch < 040 || (unsigned char)ch == 0177)

unsigned	sleep();
void	bcopy(), exit();
char	buf[MAXREP][BUFWIDTH];
int	maxcol[MAXREP] = {-1};
int	lineno;
char    filestorage[HOW_MUCH_TO_CHECK];		/* First chunk of pipe */
int     error;                                  /* error return status */
int     globi = 0, in = 0;                      /* global i and in vars.*/
int     kindofile = EMPTY_FILE;                 /* initial kind of file */
int     tmppagecount = 0, tmplinecount = 0;     /* tmp counters */
char    *imgptr, *chptr;                         /* image data pointers */
short   *cmpptr;                                /* color map pointer */
struct  imghdr im;                              /* image file header */
char    *malloc();                              /* malloc pointer */
unsigned  size;                                 /* the usual      */
int     escflg =  0;           /* escape sequence flag, 1 = in progress */
int     lstchr;                                 /* last character       */
int	width = 132;	/* default line length */
int	length = 66;	/* page length */
int	indent;		/* indentation length */
int	npages = 1;
int	literal;	/* print control characters */
char	*name;		/* user's login name */
char	*host;		/* user's machine name */
char	*acctfile;	/* accounting information file */
FILE *p = stdin, *o = stdout;
int col, ch;
char *cp;
int done, linedone, maxrep;
int     esclen;                                 /* num escape chars     */
char *setlocale();                            /* Intl. function */


extern char *index();

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

main(argc, argv) 
	int argc;
	char *argv[];
{
	register int i;

	/* 
	 * The general strategy here is to reset the printer to initial state,
   	 * sleep for five seconds for stability, process the command line arguments,
   	 * determine the input stream "file type", call the filter code, and then 
	 * optionally process accounting information.
	 */

	setlocale(LC_CTYPE,"ENG_GB.MCS");
	fprintf(o,"\033c");   	/* RIS */
	sleep(5);
	while (--argc) {
		if (*(cp = *++argv) == '-') {
			switch (cp[1]) {
			case 'n':
				argc--;
				name = *++argv;
				break;

			case 'h':
				argc--;
				host = *++argv;
				break;

			case 'w':
				if ((i = atoi(&cp[2])) > 0 && i <= MAXWIDTH)
					width = i;
				if ( width > 80 ) /* change HOR pitch to 16.5 cpi */
					fprintf(o,"\033[4w");
				break;

			case 'l':
				length = atoi(&cp[2]);
				break;

			case 'i':
				indent = atoi(&cp[2]);
				break;

			case 'c':	/* Print control chars */
				literal++;
				break;
			}
		} else
			acctfile = cp;
	}
	kindofile = determinefile();
	error = la75of();
	if ( error ){
	  	fprintf(stderr,"Failed to output data.\n");
		exit(2);
	}
	if (name && acctfile && access(acctfile, 02) >= 0 &&
	    freopen(acctfile, "a", stdout) != NULL) {
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

        in = read(fileno(p), &filestorage[0], HOW_MUCH_TO_CHECK);
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
	return(TEXT_FILE);
	/* At present we restrict ourselves to these types only */
}



la75of()
{
        register int i = 0;
        register short *tmpptr, *tmp;
        unsigned int temp;
        char *limit;


        switch(kindofile) {
                case EMPTY_FILE:
                        break;
                case EXECUTABLE_FILE:
                case ARCHIVE_FILE:
                        fprintf(stderr,"Unprintable data");
                        return(1);
                        break;
                case XIMAGE_FILE:
                        error = readXimghdr();
                        if(error) {
                                fprintf(stderr,"Failed to use image header");
                                return(1);
                        }
                        error = readXimgcmp();
                        if(error) {
                                fprintf(stderr,"Failed to use image colormap");
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
                                fprintf(stderr,"Failed to use image data");
                                return(1);
                        }
			/* Now LA75 has only one mode...so go for that */
			fprintf(o,"\033P9;;5;q\"1;1-");
			if ( im.spbxnm > MAXP_PIX_WIDTH )
				im.spbxnm =  MAXP_PIX_WIDTH;
			if ( im.spbynm > MAXP_PIX_HEIGHT ){
				tmppagecount = im.spbynm/MAXP_PIX_HEIGHT;
                                tmplinecount = MAXP_PIX_HEIGHT;
			}
                        error = dosixel();
                        if(error) {
                                fprintf(stderr,"Failed to 'sixelize' data");
                                fprintf(o, "\033\\");
                                fprintf(o, "\033c");
                                sleep(5);
                                fprintf(o, "\014");
                                fflush(o);
                                return(1);
                        }
                        break;

                case TEXT_FILE:
                default:
			for (cp = buf[0], limit = buf[MAXREP]; cp < limit; *cp++ = ' ');
			done = 0;
                        globi = 0;
			
			while (!done) {
				col = indent;
				esclen = 0;
				maxrep = -1;
				linedone = 0;
				while (!linedone) {
					ch =  globi < in ? filestorage[globi++] : getc(p);
	                                if (((escflg==0)&&(ch==ESC))||escflg)
       		                                 eschdl(o,ch);   /* deal with e
scape character */
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
				else
					switch (ch) {
					case EOF:
						linedone = done = 1;
						ch = '\n';
						break;

					case '\f':
						lineno = length;
					case '\n':
						if (maxrep < 0)
							maxrep = 0;
						linedone = 1;
						break;

					case '\b':
						if (--col < indent)
							col = indent;
						break;

					case '\r':
						col = indent;
						break;

					case '\t':
						col = ((col - indent) | 07) + indent + 1;
						break;

					case '\031':
						/*
						 * lpd needs to use a different filter to
						 * print data so stop what we are doing and
						 * wait for lpd to restart us.
						 */
						ch = globi < in ? filestorage[globi++] : getc(p);
						if ( ch == '\1') {
							fflush(stdout);
							kill(getpid(), SIGSTOP);
							break;
						} else {
							if ( globi <= in )
								globi--;
							else
								ungetc(ch,p);
							ch = '\031';
						}

					default:	/*everything else */
						addtobuf(ch);
						break;
					}
				}

				/* print out lines */
				for (i = 0; i <= maxrep; i++) {
					for (cp = buf[i], limit = cp+maxcol[i]; cp <= limit;) {
						putc(*cp, o);
						*cp++ = ' ';
					}
					if (i < maxrep)
						putc('\r', o);
					else
						putc(ch, o);
					if (++lineno >= length) {
						npages++;
						lineno = 0;
					}
					maxcol[i] = -1;
				}
			}
			if (lineno) {		/* be sure to end on a page boundary */
				putc('\f',o);
				npages++;
			}
			fprintf(o,"\033c");	/* RIS */
			fflush(o);
                        sleep(6);               /* some printers eat lines during
 reset so wait */
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
/*                                                              */
/*      eschdl - escape sequence handler                        */
/*                                                           	*/
/*      This routine intercepts escape sequences for the purpose*/
/*      of pass through.   					*/
/*								*/
/****************************************************************/

eschdl(o,c)
int c;
FILE  *o;
{

if(escflg==0)
        {               /* set escflg=1 => ready to receive 2nd seqchar*/
        escflg=1;
        }
else    switch(escflg)
                {
                case 1:         /* second character of escseq           */
                        switch(c)
                                {
                                case E_NINE: 
                                        addtobuf (ESC);
                                        addtobuf (PLD);
                                        esclen += 2;
                                        escflg = 0;
                                        break;
                                case E_EIGHT: 
                                        addtobuf (ESC);
                                        addtobuf (PLU);
                                        esclen += 2;
                                        escflg = 0;
                                        break;
                                case UCP:
                                        escflg=2; /*ctrl str pass thru mode
=8 */
                                        lstchr=c;
                                        putc(ESC,o);
                                        putc(c,o);
                                        break;
                                default:
                                       escflg=3;  /* set seq pass thru mod
e*/
                                        putc(ESC,o);
                                        putc(c,o);
                                        break;
                                }
                        break;
                case 2:         /* ctrl string pass through mode        */
                        if((lstchr==ESC) && (c==BSLH))
                                {
                                escflg=0;
                                lstchr=0;
                                }
                        else lstchr=c;  /* save it for next pass */
                        putc(c,o);
                        break;
                case 3:
                        if(escend(c))
                                escflg=0;/* turn off esc handler if at end
 */

                        putc(c,o);
                        break;
                }
return(0);
}





/* If the file stream is of the XIMAGE_FILE type this routine is called
   from la75of.  The routine creates a sixel output stream to send
   the printer based on the input data from an image file. Currently,
   only a frame buffer image of raw red, green, blue  points or a
   GPX workstation "savimg"  image are valid data streams.  Sixel
   output is initialized and then the processing begins.
   The current design of the algorithm dithers the data to a 3 x 3 matrix
  of points using the structure sixmap.
*/
dosixel()
{
        register int i;
        int xcnt,ycnt,n = 1,iindex,ij=0;
        unsigned char   *srcptr;
        unsigned char   *nxtptr;
        unsigned short sl;
        unsigned int    temp;
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
        pb = base_band = (char *)malloc((unsigned int) 6  *im.spbxnm);
        Sixmap = base_sixmap;
        Sixmap_offset = offset_sixmap;
        while((ycnt/n) * n) {
                xcnt=im.spbxnm;
                base_band_length = 0;
                base_band = pb;
                while(xcnt>0) {
                        if(MAXP_PIX_WIDTH - xcnt < 0) {
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
				    fprintf(o,"!%d%c",count,lastc);
				else
				    while(count--)putc(lastc,o);
				count = 1;
				lastc = *cp;
			    } else count++;
			    cp++;
			}
			fprintf(o,"$");
			count = 1;
			ij++;
		} else {
			cp = pb;
			lastc = *cp++;
			for(i=0;i < base_band_length;i++) {
				if(*cp != lastc) {
					if(count >= 4)
					    fprintf(o,"!%d%c",count,lastc);
					else
					    while(count--)
						    putc(lastc,o);
					count = 1;
					lastc = *cp;
				} else
					count++;
				cp++;
			}
			count = 1;
			ij = 0;
			fprintf(o,"-");
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
				fprintf(o,"+");
				tmppagecount--;
				if(im.spbynm > MAXP_PIX_HEIGHT)
					tmplinecount = MAXP_PIX_HEIGHT;
			}
		}
	}
	fprintf(o,"+");
	fprintf(o,"\033\\");
	return(0);
}

readXimghdr()
{
	register int tmp;

	size=HEDLEN*512;
	bcopy(&filestorage[globi],(char *)&im, (int)size);
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
	bcopy(&filestorage[globi], (char *)cmpptr, (int)size);
	globi = globi + size;
	return(0);
}

readXimgdat()
{
	size=im.spblen*512;
	if((imgptr=(char *) malloc(size))==NULL)
		return(1);
	bcopy(filestorage+globi,imgptr,in-globi);
	fread(imgptr+(in-globi),(char)size-(in-globi),1,p);
	return(0);
}

