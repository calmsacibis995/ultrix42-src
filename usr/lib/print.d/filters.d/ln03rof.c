#ifndef lint
static char *sccsid = "@(#)ln03rof.c	4.2      ULTRIX 	10/16/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
/*
 *    ln03rof.c	ASCII / PostScript filter
 *    can be used as the "of" and the "if" filter
 *************************************************************************
 *
 *    Modification History
 *
 * 04-Oct-90 - Adrian Thoms (thoms@wessex)
 *	Removed all redundant prologue strings
 *	Cleaned up prologue selection code
 *	Pass allowed argv[0] names via cpp from Makelocal.mk for build
 *	robustness.
 *	Fix landscape to print the same way up as ANSI translator so that
 *	N up prints sensibly on PrintServer
 *
 * 07-Mar-90 - Adrian Thoms (thoms)
 *	Fixed code which examines argv[0] to determine character set.
 *
 * 13 nov 89	Adrian Thoms
 *	Added PostScript preamble to enable ISOLATIN1 or DECMCS encoding
 *	vectors to be used.  This is switched on the value of argv[0].
 *	This switching method is so it can work without the PrintServer
 #	lpd which has parameterised argument passing.
 *
 * 23-Nov-88 - Dave Gray (gray)
 *
 *      Fixed bad pointer reference in strncmp
 *
 *    11-Sep-88  David J. Gray (gray)
 *
 *    Creating a new ln03rof filter. This filter has a very terse prolog
 *    that takes one second to send at 9600. The filter treats
 *    everything as one job. Each job has an initial prolog, the job
 *    itself followed by a ^D. The banner page counts as a seperate job,
 *    and if there is a trailer page, that is also a seperate job.
 *    Accounting is only performed if the file is actually translated
 *    by this filter (acting as the "if" filter). It does not 
 *    communicate or sync with the printer.
 *
 *    It prints the prolog first and then calculates prolog variables
 *    like:     orientation : portrait or landscape
 *
 *                            if the width specified is greater than 80
 *                            either through user specification or the
 *                            :pw: parameter in the printcap file the mode
 *                            is specified to be landscape, otherwise it
 *                            is portrait.
 *
 *              fontname    : The default font is Courier. This can be
 *                            changed by setting fontname to one of the
 *                            recognized values in the prolog.
 *
 *              topmargin   : Is set to 0.5 inches
 *
 *              bottommargin: Is set to 0.5 inches
 *
 *              leftmargin  : Is set to 0.25 inches
 *
 *                           This allows a 10 inch page in portrait
 *                           and a 7.5 inch page in landscape.
 *
 *              cpi         : Characters per is set to:
 *                               10 -> Portrait mode with page length <= 66             
 *                               12 -> Portrait mode with page length > 66 &  <= 88             
 *                               17 -> Portrait mode with page length >  88             
 *                               13 -> Landscape mode with page length <= 88             
 *                               19 -> Landscape mode with page length >  88             
 *
 *              lpi         : Lines per inch is set to:
 *                               pagelength / 10.0 in Portrait Mode
 *                               pagelength /  7.5 in Landscape Mode
 *
 ************************************************************************
 */
#include <stdio.h>			/* standard i/o definitions */
#include <strings.h>
#include <sys/signal.h>
#include <ctype.h>
#include <locale.h>

#define MAXWIDTH		132		/* maximum char. width	*/
#define DEFHEIGHT		66		/* length char. count	*/
#define DEFWIDTH                80              /* default width        */
#define MAXCOPIES		1		/* default # of copies	*/
#define BUFSIZE                 133             /* size of input buffer */
#define ESC			'\033'		/* the escape character */
#define MSIZE                   0.6             /* the unit size of 'M' */
#define PSBUFSIZE 25

/* These are the alternative names by which the filter may be called */

#ifndef LN03ROF_ISOLATIN1
#define LN03ROF_ISOLATIN1
#endif

#ifndef LN03ROF_DECMCS
#define LN03ROF_DECMCS
#endif

#define DEFAULT_FONT_FAMILY	"Courier"

/*******************************************************/
/* DECMCS Postscript Prolog inserted at beginning of document */
/*******************************************************/

char	*decmcsprolog[] =
{
       	"%!PS-Adobe",
	"%%Title: ln03rof.c",
	"%%DocumentFonts: Courier",
	"%%Creator: David Gray",
	"%%CreationDate: September 11, 1988",
	"%%EndComments",
	"% START of LPS_DECMCSENCODING",
	"% Copyright, 1986, 1988, Digital Equipment Corp.",
	"save/DECMCSEncoding where exch pop/DECMCSEncoding ISOLatin1Encoding 256",
	"array copy def mark 8#240 8#244 8#246 8#254 8#255 8#256 8#257 8#264 8#270",
	"8#276 8#320 8#336 8#360 8#376 8#377 counttomark{DECMCSEncoding exch/.notdef",
	"put}repeat 8#250/currency 8#327/OE 8#335/Ydieresis 8#367/oe 8#375/ydieresis",
	"counttomark 2 idiv{DECMCSEncoding 3 1 roll put}repeat",
	"cleartomark{restore}if",
	"% Copyright, 1986, 1988, Digital Equipment Corp.",
	"save/LPS$findfontdict where exch pop",
	"% END of LPS_DECMCSENCODING",
	0
};

/************************************************************************
 * ISOlatin1 encoding PostScript prolog
 ************************************************************************/
char	*isolatin1prolog[] =
{
	"%!PS-Adobe",
	"%%Title: ln03rof.c",
	"%%DocumentFonts: Courier",
	"%%Creator: David Gray",
	"%%CreationDate: September 11, 1988",
	"%%EndComments",
	"% START of LPS_ISOLATIN1ENCODING",
	"% Copyright, 1986, 1988, Digital Equipment Corp.",
	"save/ISOLatin1Encoding where exch pop/ISOLatin1Encoding[8#000 1",
	"8#054{StandardEncoding exch get}for/minus 8#056 1 8#217{StandardEncoding",
	"exch get}for/dotlessi 8#301 1 8#317{StandardEncoding exch",
	"get}for/space/exclamdown/cent/sterling/currency/yen/brokenbar/section",
	"/dieresis/copyright/ordfeminine/guillemotleft/logicalnot/hyphen/registered",
	"/macron/degree/plusminus/twosuperior/threesuperior/acute/mu/paragraph",
	"/periodcentered/cedilla/onesuperior/ordmasculine/guillemotright/onequarter",
	"/onehalf/threequarters/questiondown/Agrave/Aacute/Acircumflex/Atilde",
	"/Adieresis/Aring/AE/Ccedilla/Egrave/Eacute/Ecircumflex/Edieresis/Igrave",
	"/Iacute/Icircumflex/Idieresis/Eth/Ntilde/Ograve/Oacute/Ocircumflex/Otilde",
	"/Odieresis/multiply/Oslash/Ugrave/Uacute/Ucircumflex/Udieresis/Yacute/Thorn",
	"/germandbls/agrave/aacute/acircumflex/atilde/adieresis/aring/ae/ccedilla",
	"/egrave/eacute/ecircumflex/edieresis/igrave/iacute/icircumflex/idieresis",
	"/eth/ntilde/ograve/oacute/ocircumflex/otilde/odieresis/divide/oslash/ugrave",
	"/uacute/ucircumflex/udieresis/yacute/thorn/ydieresis]def{restore}if",
	"% Copyright, 1986, 1988, Digital Equipment Corp.",
        "save/LPS$findfontdict where exch pop",
	"% END of LPS_ISOLATIN1ENCODING",
	0
};

char *findfont_prolog[] = {
	"% LPS$FINDFONT_ISOLATIN1_DECMCS_V40PLUS device control module",
	"% Copyright (C) 1986, 1988, Digital Equipment Corporation",
	"% All Rights Reserved",
	"/setpacking where{pop}{/setpacking{pop}def/currentpacking{false}def}ifelse",
	"currentpacking true setpacking/findfont{LPS$findfontdict begin dup",
	"FontDirectory exch known{FontDirectory exch get}{mark 1",
	"index(123456789012345678901234567890123456789)cvs/LPS$encodings",
	"where{/LPS$encodings get{3 1 roll(1234567890)cvs search{exit}{exch",
	"pop}ifelse}forall}if counttomark 1 ne{exch pop exch length 0 eq{3 -1 roll",
	"pop true}{pop pop pop false}ifelse}{pop pop false}ifelse{dup findfont dup",
	"length dict 3 -1 roll pop exch{1 index/FID ne 2 index/UniqueID ne and{2",
	"index 3 1 roll put}{pop pop}ifelse}forall/LPS$fallbacks where{pop",
	"dup/Encoding 4 -1 roll exec 256 array copy put dup/CharStrings known{0 1",
	"255{{1 index/Encoding get 1 index get 2 index/CharStrings get exch known",
	"not{1 index/Encoding get 1 index get 2 index/Encoding get 2 index",
	"LPS$fallbacks/others get put LPS$fallbacks{exch 2 index eq{3 index/Encoding",
	"get 3 index 3 -1 roll put exit}{pop}ifelse}forall pop}{exit}ifelse}loop",
	"pop}for}if}{dup/Encoding 4 -1 roll exec put}ifelse",
	"definefont}{findfont}ifelse}ifelse end}bind def/LPS$findfontdict 32 dict",
	"def LPS$findfontdict begin/findfont dup load def/LPS$encodings 6 dict def",
	"LPS$encodings",
	"begin/-ISOLatin1{ISOLatin1Encoding}def/-DECMCS{DECMCSEncoding}def/-DECTech{",
	"DECTechEncoding}def/-DECPub{DECPubEncoding}def end end",
	"setpacking{restore}if",
	"% end of LPS_FINDFONT_ISOLATIN1_DECMCS_V40",
	0
};

char *adobe_prolog[] = {0};

char *prolog[] =
{
	"%!PS-Adobe",
	"%%Title: ln03rof.c",
	"%%DocumentFonts: Courier",
	"%%Creator: David Gray",
	"%%CreationDate: September 11, 1988",
	"%%EndComments",
	"% Page Setup",
	"/p-s {",
	"  initmatrix rotate translate",
	"  initclip clippath pathbbox newpath",
	"% Upper Y       Right X       Lower Y       Left X", 
	"  /uy exch def  /rx exch def  /ly exch def  /lx  exch def",
	"} def",
	"",
	"% Portrait Mode      Top Y @ 11 inches",
	"/portrait {0 0 0 p-s /ty 792 def} def",
	"",
	"% Landscape Mode           Top Y @ 8.5 inches",
	"/landscape {0 -612 90 p-s /ty 612 def} def",
	"",
	"% Simple defs and binds ",
	"/in {72 mul} def",
	"/m {moveto} bind def",
	"/s {show} bind def",
	"/sp {showpage} bind def",
	"/cpt {currentpoint} bind def",
	"",
	"% Get larger value based on current value of op",
	"/glrgr { dup 2 index lt {pop} {exch pop} ifelse} def",
	"",
	"% Define Top Margin",
	"/top-m {/tmar exch ty uy sub glrgr def} def",
	"",
	"% Define Bottom Margin",
	"/bot-m {/bmar exch ly glrgr def} def",
	"",
	"% Define Left Margin",
	"/left-m {/lmar exch lx glrgr def} def",
	"",
	"% Initial values of X and Y",
	"/ix { lmar } def",
	"/iy { ty lfsize .8 mul sub tmar sub } def",
	"",
	"% Check to see it at initial position",
	"/notinit {cpt cvi exch cvi ix cvi lt exch iy cvi lt or} def",
	"",
	"% Start of Job - move to initial position",
	"/soj {ix iy m save} def",
	"",
	"/eoj {notinit {sp} if restore} def",
	"",
	"% Form Feed",
	"/ff {sp restore save ix iy m} def",
	"",
	"% Check current position, if not at init do form feed",
	"/doff {notinit {ff} if} def",
	"",
	"% Carraige Return",
	"/cr {cpt exch pop lmar exch m} def",
	"",
	"% BackSpace",
	"/bsp {cpt exch lmar exch spsize sub glrgr exch m} def",
	"",
	"% New Line",
	"/nl {cr cpt lfsize sub m cpt exch pop bmar le {ff} if} def",
	"",
	"% Partial Line Down (nroff subscripting)",
	"/pld {cpt lfsize 2 div sub m} def",
	"",
	"% Partial Line Up (nroff superscripting)",
	"/plu {cpt lfsize 2 div add m} def",
	"",
	"% Full Line Up (nroff superscripting)",
	"/flu {cpt lfsize add m} def",
	"",
	"% Tab",
	"/tab {cpt pop lmar sub spsize div round cvi",
	"      8 mod neg 8 add { ( ) s} repeat} def",
	"",
	0
};
/*******************************************************/
/* Default Prolog End of Postscript Prolog                */
/*******************************************************/

enum font_encoding_e {
	fe_Adobe,
	fe_ISOlatin1,
	fe_DECMCS
    };

static char *font_suffix[] = {
	"",
	"-ISOLatin1",
	"-DECMCS"
    };

static char **font_encoding_prolog[] = {
	adobe_prolog,
	isolatin1prolog,
	decmcsprolog
    };

int	lineno;					/* line number		*/
int	width = DEFWIDTH;			/* default line length	*/
int	length = DEFHEIGHT;			/* page length		*/
int	indent = 0;				/* indentation length	*/
int	npages = MAXCOPIES;			/* number of copies	*/
int	literal = 0;				/* print control chars. */
char	*name;					/* user's login name	*/
char	*host;					/* user's machine name	*/
char	*acctfile;				/* accounting info. file*/
char    orientation[10];                        /* portrait or landscape*/
char    fontname[40];                           /* name of default font */
double  lpi;                                    /* lines per inch - vert*/
int     cpi;                                    /* characters per inch  */
double  topmargin;			/* margins, top, bottom and left*/
double  bottommargin;
double  leftmargin;
char spbuf[PSBUFSIZE], *sp=spbuf;   /* input buffer - file type testing */

void	exit();
char	*strcpy();


int cc;
char buf[BUFSIZE];


/*
***	Determine if file has Postscript magic number
 */

#define POSTSCRIPT_MN "%!"    /* PostScript Magic Number*/
#define PS_EOF '\004'	      /* PostScript End of File */
#define PS 1

/* MACRO DEFINITIONS */
#define spgetchar() ((*sp) ? (*sp++) : (getchar()))
#define is_7bit_cntrl(ch) ((unsigned char)ch < 040 || (unsigned char)ch == 0177)
#define is_7or8_bit(ch) ((unsigned)ch < '\040' || (unsigned)ch >= '\177')

init()
/* Initial values for variables in prolog */
{
	topmargin = 0.5;
	bottommargin = 0.5;
	leftmargin = 0.25;
        if (strcmp (orientation, "portrait") == 0){
                if (length <= 66) {
                        cpi = 10;
                        lpi = length / 10.0;
                }
                else if ((length > 66) && (length <= 88)) {
			cpi = 12;
			lpi = length / 10.0;
		}
		else {
			cpi = 17;
			lpi = length / 10.0;
		}
	}
	else { /* orientation is landscape */
		if (length <= 88) {
			cpi = 13;
			lpi = length / 7.5;
		}
		else {
			cpi = 19;
			lpi = length / 7.5;
		}
	}

}

is_ps()
/* Is this a PostScript Document ? */
{
	register char  *ps=POSTSCRIPT_MN;
	register int	c;

	while (*ps)
	{
		if ((c=getchar()) == EOF)
			exit(0);
		*sp++ = c;	/* put in buffer */
		if (c != *ps++)
		{
			*sp = NULL ;	/* null terminate buffer */
			sp = spbuf;
			return NULL ;
		}
	}
	*sp = NULL ;	/* null terminate buffer */
	sp=spbuf;
	return PS;
}


ps_end(stat)
int stat;
{
        putchar ('\n');
	putchar(PS_EOF);
	exit(stat);
}



main(argc, argv)
	int argc;
	char *argv[];
{
	register int i, c, ch;
	register char *cp;
        FILE     *af;
	register enum font_encoding_e font_encoding=fe_Adobe;
	char	*arg0;      /* pointer to name filter was called by */ 
	int	prolog_sent=0;
	char *font_family=DEFAULT_FONT_FAMILY;

        lineno = 0;
	indent = 0;
	arg0 = strrchr(argv[0], '/');
	if (!arg0) {
		arg0 = argv[0];
	} else {
		arg0 += 1;	/* Step past the / */
	}
	if (strcmp(arg0, LN03ROF_DECMCS) == 0) {
		font_encoding = fe_DECMCS;
		setlocale(LC_CTYPE,"ENG_GB.MCS");
	}
	else if (strcmp(arg0, LN03ROF_ISOLATIN1) == 0) {		
		font_encoding = fe_ISOlatin1;
		setlocale(LC_CTYPE,"ENG_GB.8859");
	}

	strcpy (orientation, "portrait");
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
				if ((i = atoi(&cp[2])) > 0 && i <= MAXWIDTH){
					width = i;
				}

				if (width > 80) { /* switch to landscape mode */
					strcpy (orientation, "landscape");
				}
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

	if (is_ps()==PS){
		send_ps ();  /* send postscript file and then exit */
	}
	/*========================================================================*/
	/*
	 * load the PostScript function definitions
	 * Since its not a PostScript document
	 */

	init();  /* initialize prolog variables */

	cc = indent;
	for (i = 0; i <= cc; buf[i++] = ' ');  /* Initialize buf to blanks */

	while ((c = spgetchar()) != EOF)
	{
		if (!prolog_sent) {
			send_encoding_and_prolog(font_family, font_encoding);
			puts("soj");
			prolog_sent++;
		}
           /* Print literal characters otherwise ignore them */

		 if ( literal && is_7or8_bit(c) && (c != '\n')) {
			if (is_7bit_cntrl (c)) {
				if (c == '\177') {
					buf[cc++] = '^';
					buf[cc++] = '?';
				}
				else {
					buf[cc++] = '^';
					ch = c + '@';
					if (ch == '\\' || ch == '[' || ch == ']')
						buf[cc++] = '\\';
					buf[cc++] = ch;
				}
			}
			else {  /* its 8 bit */
				if ((unsigned)c < (unsigned)'\240') {
					buf[cc++] = 'M';
					buf[cc++] = '-';
					ch = (c & 0177) + '@';
					if (ch == '\\' || ch == '[' || ch == ']')
						buf[cc++] = '\\';
					buf[cc++] = ch;
				}
				else {
					buf[cc++] = '\\';
					buf[cc++] = 064 + ( (unsigned)c >> 6);  /* WHY? */
					buf[cc++] = 060 + ( ((unsigned)c >> 3) & 07);
					buf[cc++] = 060 + ( (unsigned)c  & 07);
				}
			}
		}
	   else {  /* Supress control characters */
		
		switch (c)
		{
			case ('\n'):
				flush_buf(1);
				puts("nl");
				break;
			case ('\f'):
				flush_buf(1);
				puts("ff");
				npages++;
				break;
			case ('\r'):
				flush_buf(1);
				puts("cr");
				break;
			case ('\b'):
				flush_buf(0);
				puts("bsp");
				break;
			case ('\t'):
				flush_buf(0);
				puts("tab");
				break;
			case (ESC):
				flush_buf(0);
				c = spgetchar ();
				if (c == '9')       /* move partial line down */
					puts("pld");
				else if (c == '8')  /* move partial line up   */
					puts("plu");
				else if (c == '7')  /* move a full line up    */
					puts("flu");
				else
					printf ("(%c) show", c);
				break;
			case ('\031'):
				/*
			 	 * lpd needs to use a different filter 
			 	 * to print data so stop what we are 
			 	 * doing and lpd will restart us later.
                                 * This is executed when this filter is
                                 * used as the "of" as specified in
                                 * /etc/printcap
			 	 */
				if ((c = spgetchar()) == '\1') {
					puts("eoj"); 
					putchar(PS_EOF);
					fflush(stdout);
					kill(getpid(), SIGSTOP); /* sleeeeep */
					prolog_sent = 0;
					break;
				} else {
					ungetc(c, stdin);
					c = '\031';
				}
				break;
			case ('\\'):
			case ('('):
			case (')'):
				if (cc >= (sizeof(buf) - 2))
					flush_buf(1);
				buf[cc++] = '\\';
				/* fall through into default code */
			default:
				if(isprint(c)) {
					buf[cc++] = c;
					if (cc >= (sizeof(buf) - 1))
						flush_buf(1);
				}
				break;
		} /* end switch */

	   } /* end else supress control characters */

	} /* end while not done */

	flush_buf(1);
	/*
	 * execute "end-of-job" function
	 */
	if (prolog_sent) {
		printf("eoj\n");
	}
	if (name && acctfile && access(acctfile, 02) >= 0 &&
	    (af = fopen(acctfile, "a")) != NULL) {
		fprintf(af, "%7.2f\t%s:%s\n", (float)npages, host, name);
	}
	ps_end(0);
}

put_text(text_block)
char **text_block;
{
	register char *p;
	while(p = *text_block++) {
		if (*p != '%')  /* do not send comments */
                	fprintf(stdout,"%s\n", p);
	}
}


set_and_scale_font()
/*
 * Set font and scaling
 */
{
	double  spsize,               /* The size of one blank space         */
		lfsize,               /* The size of a line feed             */
		fontsize;             /* The size of the font                */

	spsize = (1.0 / cpi) * 72.0;
	lfsize = (1.0 / lpi) * 72.0;
	fontsize = spsize / MSIZE;

	/* re-initialize prolog variables */
	fprintf (stdout, "\n");
	fprintf (stdout, "%s\n", orientation);
	fprintf (stdout, "%4.2f in top-m\n", topmargin);
	fprintf (stdout, "%4.2f in bot-m\n", bottommargin);
	fprintf (stdout, "%4.2f in left-m\n", leftmargin);
	fprintf (stdout, "/lfsize %4.2f def\n", lfsize);
	fprintf (stdout, "/spsize %4.2f def\n", spsize);
	fprintf (stdout, "/%s findfont %4.2f scalefont setfont\n", 
			  fontname, fontsize);
	fprintf (stdout, "\n");
}

can_print (c)
int	c;
/*
 * Returns true if it is a printable 7 or 8 bit character 
 * A newline, carriage return or a tab
 */
{
	return (((unsigned)c >= (unsigned)'\040' && (unsigned)c <= (unsigned)'\176') || 
                ((unsigned)c >= (unsigned)'\240' && (unsigned)c <= (unsigned)'\375') || 
		((unsigned)c == (unsigned)'\012') || ((unsigned)c == (unsigned)'\011') || 
		((unsigned)c == (unsigned)'\015'));
}

send_ps ()
/*
 * The document is already formatted in PostScript, so we'll pass it 
 * straight through, but remove most control characters.
 *
 */
{
	register	int	c;

	while ((c = spgetchar()) != EOF) {
		if (can_print(c)) {
			putchar (c);
		}
	}
	ps_end(0);
}

flush_buf(entire_line)
int	entire_line;
/*
 * Flushes the output buffer to the printer (if it is not empty).
 * If entire line is flushed then cc is reset to indent, otherwise
 * only part of the line is being flushed so cc is reset to 0.
 *
 */
{
int  i;
	if (cc != 0)
	{
		buf[cc] = NULL;
		printf("(%s) s ", buf);
		if (++lineno == length) {
			lineno = 0;
			npages ++;
		}
		for (i = 0; i <= cc; buf[i++] = ' ');
		if (entire_line)
                	cc = indent;
		else
			cc = 0;
	}
}

/* Sends the appropriate PS prolog based on value of font_encoding */
send_encoding_and_prolog(font_family, font_encoding)
char *font_family;
enum font_encoding_e font_encoding;
{
	sprintf(fontname, "%s%s",
		font_family, font_suffix[(int)font_encoding]);

	/*
	 * We need to output the magic %! here because
	 * put_text() strips all comments
	 */
        fprintf (stdout, "%%!\n");
	put_text(font_encoding_prolog[(int)font_encoding]);		
	if (font_encoding != fe_Adobe)
	    put_text(findfont_prolog);
	put_text(prolog);
	set_and_scale_font();
}



