
#ifndef lint
static char *sccsid = "@(#)dd.c	4.1    (ULTRIX)        7/2/90";
#endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1988 by			*
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

/* ------------------------------------------------------------------------
 * Modification History: /usr/src/bin/dd.c
 *
 * 03 Nov 89 -- alan frechette
 *	Added a new conversion for dd to create sparse output files.
 *
 * 19 Oct 89 -- lambert
 *	Fixed null pointer problem in strcmp in the "open outfile" section.
 *
 * 22 Aug 89 -- lambert
 *	Modify 'sbrk()' call in main() to use 'getpagesize()' to ensure
 *	alignment of I/O buffers on page boundries.
 *
 * 19 Aug 88 -- lambert
 *	Change return value of EBCDIC macro so that the proper value is
 *	returned to caller.
 *
 * 08 Jun 88 -- map
 *	Changed signal handlers to void.
 *
 * 16 Dec 87 -- fries
 *	Removed no space left on device message from being
 *	displayed. This provides backward compatability.
 *
 * 24 Nov 87 -- fries
 *	Fixed odd byte count allocation bug. sbrk() allocates with
 *	the buffer starting on an odd byte boundary. Corrected buffer
 *	index bug, whereas indexes are not correct if bs not used for
 *	blocking(problem using ibs= or obs=).
 *
 * 28 Jan 87 -- lp & rr
 *	Fixed skip and count cases. Added rr's fixes for various convs.
 *
 * 17 Nov 86 -- J. Fries
 *	Added code to rewind buffer pointers and pending i/o buffers
 *	when a File Mark was detected and still files to read.
 *
 * 16 Oct 86 -- lp
 *	Fixed a bug with eot handling. Remove mtcache.
 *
 * 11 Sep 86 -- lp
 *	BSR done a little different to handle big tape drives.
 *
 * 27 Aug 86 -- lp
 *	Handle how n-buf read is done. Bugfixes.
 *
 * 1  Jul 86 -- fries
 *	Added ioctl call MTCACHE in statchk() to enable cache.
 *
 * 7  May 86 -- lp
 *	Async & mvol now default. conv=nomulti overrides multivolume hooks.	
 * 	wbuf=0, rbuf=0 overrides n-buffered.
 *
 * 21 May 86 -- lp
 * 	Merged multivolume changes. Cleanup.
 *
 * 14 May 86 -- fries
 *	Added code to make use of generic ioctl
 *
 * 14 Apr 86 -- lp
 *	Added n-buffered I/O support. Added multivolume hooks.
 *
 * 19 Mar 86 -- fries
 *	Corrected typo stat() to stats() to print out number of input
 *	and output records. Bug fix.
 *
 * 29 Jan 86 -- fries
 *	Changed exit codes to provide an error exit status of -1
 *	Added comments for clarity.
 *
 *
 * ------------------------------------------------------------------------
 */
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <sys/devio.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>

#define	BIG	2147483647
#define	LWRCASE	01			/* map upper to lower case         */
#define	UCASE	02			/* map lower to upper case         */
#define	SWAB	04			/* swap byte pairs                 */
#define NERR	010			/* do not stop conversion on error */
#define SYNC	020			/* pad every input record to ibs   */
#define MVOL	040			/* Multi-volume indicator          */
#define SPARSE	0100			/* Sparse file indicator           */
#define MAXASYNC 8			/* Maximum # of async. i/o op's    */
#define WRITEIP 2			/* signals buffer used in write i/o*/
#define ERRORIP 4

off_t	outfileoffset = 0; 		/* Current output file offset      */
off_t	lastwriteoffset = 0; 		/* Last write output file offset   */
int	cflag = MVOL;			/* Conversion type indicator       */
int	fflag;				/* Flag (1 = no conversion)        */
int	skip;				/* skip 'n' input rec's before cpy */
int	seekn;				/* seek 'n' rec.'s from output     */
					/* beginning before copying        */
int	count;				/* cpy only 'n' input records      */
int	files	= 1;			/* cpy 'n' inp files before stopng */ 
int	curibuf = 0;			/* current async. i/o buffer to use*/
int	curobuf = 0;			/* current async. i/o buffer to use*/
int	maxasync = MAXASYNC;		/* maximum number of async. i/o's  */
int 	readasync = 1;			/* Flag (1 = read async.)          */
int 	writeasync = 1;			/* Flag (1 = write async.)         */
int	reading=0;			/* Flag (1 = read operation)       */
int 	eomflag = 0;			/* Flag (1 = end of media)         */
char	*string;			/* general usage char. pointer     */
char	*ifile;				/* Input filename pointer          */
char	*ofile;				/* Output filename pointer         */
char	*ibuf[MAXASYNC];		/* ary. of ptr.'s(async. inp. bfrs)*/
char	*obuf[MAXASYNC];		/* ary. of ptr.'s(async. out. bfrs)*/
int 	pending[MAXASYNC];		/* holds info. about pending bfrs  */
int 	pendcnt[MAXASYNC];		/* holds counts ret'd  pending bfrs*/
char	*sbrk();			/* call to add spc. to prog. area  */
int	ibs	= 512;			/* input block size(default to 512)*/
int	obs	= 512;			/* output blk. size(default to 512)*/
int	ibsz, obsz;			/* Allocation buffer sizes         */
int	bs;				/* overrides ibs &obs block sizes  */
int	cbs;				/* conversion buffer size          */
int	ibc;				/* input byte count(read returned) */
int	obc;				/* ouput byte count(used in write) */
int	cbc;				/* converion byte count            */
int	nifr;				/* number of input full records    */
int	nipr;				/* number of input partial rec.'s  */
int	nofr;				/* number of output full rec.'s    */
int	nopr;				/* number of output partial rec.'s */
int	ntrunc;				/* number of truncated records     */
int	ibf;				/* input device file descriptor    */
int	obf;				/* output device file descriptor   */
char	*op;
int	nspace;				/* # of spaces(conversion code)    */
struct 	stat	stat_buf;		/* file stat buffer                */
struct mtop mtcom;

/****************************/
/* CONVERSION TABLES FOLLOW */
/****************************/
/* Conversion table to convert */
/* EBCDIC ---> ASCII           */
char	etoa[] = {
	0000,0001,0002,0003,0234,0011,0206,0177,
	0227,0215,0216,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0235,0205,0010,0207,
	0030,0031,0222,0217,0034,0035,0036,0037,
	0200,0201,0202,0203,0204,0012,0027,0033,
	0210,0211,0212,0213,0214,0005,0006,0007,
	0220,0221,0026,0223,0224,0225,0226,0004,
	0230,0231,0232,0233,0024,0025,0236,0032,
	0040,0240,0241,0242,0243,0244,0245,0246,
	0247,0250,0133,0056,0074,0050,0053,0041,
	0046,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0135,0044,0052,0051,0073,0136,
	0055,0057,0262,0263,0264,0265,0266,0267,
	0270,0271,0174,0054,0045,0137,0076,0077,
	0272,0273,0274,0275,0276,0277,0300,0301,
	0302,0140,0072,0043,0100,0047,0075,0042,
	0303,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0304,0305,0306,0307,0310,0311,
	0312,0152,0153,0154,0155,0156,0157,0160,
	0161,0162,0313,0314,0315,0316,0317,0320,
	0321,0176,0163,0164,0165,0166,0167,0170,
	0171,0172,0322,0323,0324,0325,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0345,0346,0347,
	0173,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0350,0351,0352,0353,0354,0355,
	0175,0112,0113,0114,0115,0116,0117,0120,
	0121,0122,0356,0357,0360,0361,0362,0363,
	0134,0237,0123,0124,0125,0126,0127,0130,
	0131,0132,0364,0365,0366,0367,0370,0371,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0372,0373,0374,0375,0376,0377,
};
/* Conversion table to convert */
/* ASCII ---> EBCDIC           */
char	atoe[] = {
	0000,0001,0002,0003,0067,0055,0056,0057,
	0026,0005,0045,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0074,0075,0062,0046,
	0030,0031,0077,0047,0034,0035,0036,0037,
	0100,0117,0177,0173,0133,0154,0120,0175,
	0115,0135,0134,0116,0153,0140,0113,0141,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0172,0136,0114,0176,0156,0157,
	0174,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0321,0322,0323,0324,0325,0326,
	0327,0330,0331,0342,0343,0344,0345,0346,
	0347,0350,0351,0112,0340,0132,0137,0155,
	0171,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0221,0222,0223,0224,0225,0226,
	0227,0230,0231,0242,0243,0244,0245,0246,
	0247,0250,0251,0300,0152,0320,0241,0007,
	0040,0041,0042,0043,0044,0025,0006,0027,
	0050,0051,0052,0053,0054,0011,0012,0033,
	0060,0061,0032,0063,0064,0065,0066,0010,
	0070,0071,0072,0073,0004,0024,0076,0341,
	0101,0102,0103,0104,0105,0106,0107,0110,
	0111,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0142,0143,0144,0145,0146,0147,
	0150,0151,0160,0161,0162,0163,0164,0165,
	0166,0167,0170,0200,0212,0213,0214,0215,
	0216,0217,0220,0232,0233,0234,0235,0236,
	0237,0240,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0312,0313,0314,0315,0316,0317,0332,0333,
	0334,0335,0336,0337,0352,0353,0354,0355,
	0356,0357,0372,0373,0374,0375,0376,0377,
};
/* Conversion table to convert */
/* ASCII ---> IBM EBCDIC       */
char	atoibm[] =
{
	0000,0001,0002,0003,0067,0055,0056,0057,
	0026,0005,0045,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0074,0075,0062,0046,
	0030,0031,0077,0047,0034,0035,0036,0037,
	0100,0132,0177,0173,0133,0154,0120,0175,
	0115,0135,0134,0116,0153,0140,0113,0141,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0172,0136,0114,0176,0156,0157,
	0174,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0321,0322,0323,0324,0325,0326,
	0327,0330,0331,0342,0343,0344,0345,0346,
	0347,0350,0351,0255,0340,0275,0137,0155,
	0171,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0221,0222,0223,0224,0225,0226,
	0227,0230,0231,0242,0243,0244,0245,0246,
	0247,0250,0251,0300,0117,0320,0241,0007,
	0040,0041,0042,0043,0044,0025,0006,0027,
	0050,0051,0052,0053,0054,0011,0012,0033,
	0060,0061,0032,0063,0064,0065,0066,0010,
	0070,0071,0072,0073,0004,0024,0076,0341,
	0101,0102,0103,0104,0105,0106,0107,0110,
	0111,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0142,0143,0144,0145,0146,0147,
	0150,0151,0160,0161,0162,0163,0164,0165,
	0166,0167,0170,0200,0212,0213,0214,0215,
	0216,0217,0220,0232,0233,0234,0235,0236,
	0237,0240,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0312,0313,0314,0315,0316,0317,0332,0333,
	0334,0335,0336,0337,0352,0353,0354,0355,
	0356,0357,0372,0373,0374,0375,0376,0377,
};
/****************************/
/* END OF CONVERSION TABLES */
/****************************/

/* Fast macros */
#define Null(c) {					\
			*op++ = c;			\
			if(++obc >= obs) {		\
				flsh();			\
				if(bs)                  \
				op = obuf[curibuf];	\
				else                    \
				op = obuf[curobuf];	\
			}				\
		}


#define CNull(c) {							\
			register c_n;					\
			c_n = c;					\
			if((cflag & UCASE) && (c_n >= 'a') && (c_n <= 'z'))\
				c_n += 'A' - 'a';			\
			if((cflag & LWRCASE) && (c_n >= 'A') && (c_n <= 'Z'))\
				c_n += 'a' - 'A';			\
			Null(c_n);					\
		  }


#define Ascii(c)	{						\
				register c_a;				\
				c_a = etoa[c] & 0377;			\
				if(cbs == 0) {				\
					CNull(c_a);			\
				}					\
				else {					\
					if(c_a == ' ') {		\
						nspace++;		\
					}				\
					else {				\
						while(nspace > 0) {	\
							Null(' ');	\
							nspace--;	\
						}			\
						CNull(c_a);		\
					}				\
					if(++cbc >= cbs) {		\
						Null('\n');		\
						cbc = 0;		\
						nspace = 0;		\
					}				\
				}					\
			}

#define Ebcdic(c) {							\
			register c_e;					\
			c_e = c;					\
			if(cflag & UCASE && c_e >= 'a' && c_e <= 'z')	\
				c_e += 'A' - 'a';			\
			if(cflag & LWRCASE && c_e >= 'A' && c_e <= 'Z')	\
				c_e += 'a' - 'A';			\
			c_e = atoe[c_e] & 0377;				\
			if(cbs == 0) {					\
				Null(c_e);				\
			}						\
			else {						\
				if(c == '\n') {				\
					while(cbc < cbs) {		\
						Null(atoe[' ']);	\
						cbc++;			\
					}				\
					cbc = 0;			\
				}					\
				else {					\
					if(cbc == cbs)			\
						ntrunc++;		\
					cbc++;				\
					if(cbc <= cbs)			\
						Null(c_e);		\
				}					\
			}						\
		}

#define Ibm(c) {							\
			register c_i;					\
			c_i = c;					\
			if(cflag & UCASE && c_i >= 'a' && c_i <= 'z')	\
				c_i += 'A' - 'a';			\
			if(cflag & LWRCASE && c_i >= 'A' && c_i <= 'Z')	\
				c_i += 'a' - 'A';			\
			c_i = atoibm[c_i] & 0377;			\
			if(cbs == 0) {					\
				Null(c_i);				\
			}						\
			else {						\
				if(c == '\n') {				\
					while(cbc < cbs) {		\
						Null(atoibm[' ']);	\
						cbc++;			\
					}				\
					cbc = 0;			\
				}					\
				else {					\
					if(cbc == cbs)			\
						ntrunc++;		\
					cbc++;				\
					if(cbc <= cbs)			\
						Null(c_i);		\
				}					\
			}						\
		}

/* Main Code Follows */
main(argc, argv)
int	argc;
char	**argv;
{
	int (*conv)();
	register char *ip;
	register c;
	int ebcdic(), ibm(), ascii(), null(), cnull(), term(), block(), unblock();
	void term2();
	int a, k;
	extern int errno;

	/* Set conversion indicator */
	conv = null;
	for(c=1; c<argc; c++) {
		string = argv[c];

		/* # of async. write buffers */
		if(match("wbuf=")) {
			if(writeasync) {
				readasync=0;
				maxasync = number(BIG) <= 0 ? 1 : number(BIG);
			}
			continue;
		}

		/* # of async. read buffers */
		if(match("rbuf=")) {
			if(readasync) {
				writeasync=0;
				maxasync = number(BIG) <= 0 ? 1 : number(BIG);
			}
			continue;
		}

		/* input block size */
		if(match("ibs=")) {
			ibs = number(BIG);
			continue;
		}

		/* output block size */
		if(match("obs=")) {
			obs = number(BIG);
			continue;
		}

		/* conversion buffer size */
		if(match("cbs=")) {
			cbs = number(BIG);
			continue;
		}

		/* block size to use for both */
		/* ibs and obs block sizes    */
		if (match("bs=")) {
			bs = number(BIG);
			continue;
		}

		/* input filename */
		if(match("if=")) {
			ifile = string;
			continue;
		}

		/* output filename */
		if(match("of=")) {
			ofile = string;
			continue;
		}

		/* skip 'n' input records, then */
		/* begin copying                */
		if(match("skip=")) {
			skip = number(BIG);
			continue;
		}

		/* seek 'n' records from beginning */
		/* of file prior to copying        */
		if(match("seek=")) {
			seekn = number(BIG);
			continue;
		}

		/* copy only 'n' input records */
		if(match("count=")) {
			count = number(BIG);
			continue;
		}

		/* copy 'n' input files prior to stopping */

		if(match("files=")) {
			files = number(BIG);
			continue;
		}

 		/* If conversion... */	
		if(match("conv=")) {
		cloop:
			if(match(","))
				goto cloop;
			if(*string == '\0')
				continue;
			if(match("ebcdic")) {
				conv = ebcdic;
				goto cloop;
			}
			if(match("ibm")) {
				conv = ibm;
				goto cloop;
			}
			if(match("ascii")) {
				conv = ascii;
				goto cloop;
			}
			if(match("block")) {
				conv = block;
				goto cloop;
			}
			if(match("unblock")) {
				conv = unblock;
				goto cloop;
			}
			if(match("lcase")) {
				cflag |= LWRCASE;
				goto cloop;
			}
			if(match("ucase")) {
				cflag |= UCASE;
				goto cloop;
			}
			if(match("swab")) {
				cflag |= SWAB;
				goto cloop;
			}
			if(match("noerror")) {
				cflag |= NERR;
				goto cloop;
			}
			if(match("sync")) {
				cflag |= SYNC;
				goto cloop;
			}
			if(match("nomulti")) {
				cflag &= ~MVOL;
				goto cloop;
			}
			if(match("sparse")) {
				cflag |= SPARSE;
				goto cloop;
			}
		}
		fprintf(stderr,"bad arg: %s\n", string);
		exit(-1);
	}
	if(conv == null && cflag & (LWRCASE|UCASE))
		conv = cnull;

restart:
	errno = 0;
	if(!eomflag || reading) {

		/* Open the input device or file */
		if (ifile)
			if((stat(ifile,&stat_buf) >= 0)
		 	&&(((stat_buf.st_mode & S_IFMT) == S_IFCHR)
		 	||((stat_buf.st_mode & S_IFMT)  == S_IFBLK)))
		   	ibf = statchk(ifile, O_RDONLY);
			else
		 	ibf = open(ifile, O_RDONLY);
		else {
			ibf = dup(0);
		}
		if(ibf < 0) {
			if(errno)perror(ifile);
			exit(-1);
		}
	}
	if(!eomflag || !reading) {

		/* Open the output device or file */
		if (ofile){
			if((stat(ofile,&stat_buf) >= 0)
		 	&&(((stat_buf.st_mode & S_IFMT) == S_IFCHR)
		 	||((stat_buf.st_mode & S_IFMT)  == S_IFBLK))) {
		    	obf = statchk(ofile, O_RDWR);
			if(cflag & SPARSE)
				cflag &= ~SPARSE;
			}
			else {
			if (ifile) 
			    if(strcmp(ifile, ofile) == 0) {
				fprintf(stderr, "%s: ifile cannot be ofile\n", argv[0]); 
				exit(-1);
			}
		 	obf = creat(ofile, 0666);
			}
		}
		else {
			obf = dup(1);
			if((cflag & SPARSE) && (fstat(obf,&stat_buf) >= 0)
		 	&& !(stat_buf.st_mode & S_IFREG))
				cflag &= ~SPARSE;
		}
		if(obf < 0) {
			if(errno)fprintf(stderr,"cannot create: %s\n", ofile);
			exit(-1);
		}
	}

	/* Set input and output block size to 'bs' */
	/* Set no conversion flag 'fflag'          */
	if (bs) {
		ibs = obs = bs;
		if (conv == null)
			fflag++;
	}

	/* Check for blocks size of zero */
	if(ibs == 0 || obs == 0) {
		fprintf(stderr,"counts: cannot be zero\n");
		exit(-1);
	}

	/* If input block size < 1k, then use synchronous i/o */
	if(ibs <= 1024) 
		readasync = 0;

	/* Modify # of async. i/o's to perform(default is 1) */
	if(maxasync > 1) {
		int maxcnt = maxasync;

		/* If requested # async. i/o's > than max. */
		/* then set it to max.                     */
		if(maxasync > MAXASYNC) 
			maxcnt = MAXASYNC;
		maxasync = maxcnt;	

		/* If doing async. writes... */
		if(writeasync)
			if(ioctl(obf, FIONBUF, &maxcnt) < 0)
				writeasync=0;
		maxcnt = maxcnt ? maxcnt : maxasync;

		/* If using sync. writes... */
		if(!writeasync) {
			/* If using async. reads... */
			if(readasync) {
				if(ioctl(ibf, FIONBUF, &maxcnt) < 0)
					readasync=0;
			}
		} else
			readasync=0;

		/* If using either async. reads or writes */
		if(readasync || writeasync)
			maxasync = maxcnt;
		else
			maxasync=1;
	} else {
		writeasync = readasync = 0;
	}

	/* Request memory allocation for buffers        */
	/* placing buffer pointers in array of pointers */
	ibsz = ibs;
	if(ibs%2 != 0)
		ibsz++;

	obsz = obs;
	if(obs%2 != 0)
		obsz++;

	if(!eomflag) {
		for(k=0; k<maxasync; k++) {
			ibuf[k] = sbrk(ibsz+(2*getpagesize()));
			if (fflag) {
				obuf[k] = ibuf[k];
			} else {
				obuf[k] = sbrk(obsz);
			}
			pending[k] = 0;
		}

	/* For good measure */
	sbrk(1024);

	/* Check the last element used in the buffer pointer  */
	/* array for -1. -1 indicates a failure on allocation */
	if(ibuf[maxasync-1] == (char *)-1 || obuf[maxasync-1] == (char *)-1) {
		fprintf(stderr, "not enough memory\n");
		exit(-1);
	}

	/* If Control-C Interrupt... */
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, term2);

	/* IF skipping records... */
	if(skip) {
	while(skip) {
		if(readasync) {
			int cnt = 0;
			(void)ioctl(ibf, FIONBUF, &cnt);
		}
		read(ibf, ibuf[0], ibs);
		skip--;
	}
	if(readasync) {
		int cnt = maxasync;
		(void)ioctl(ibf, FIONBUF, &cnt);
	}
	}

	/* If seeking into a file... */
	while(seekn) {
		lseek(obf, (long)obs, 1);
		seekn--;
	}
	} /* End of if (!eomflag) */

	/* Reflush pending write buffers */
	if(!writeasync && reading && eomflag) {
		for(k=0; k<maxasync; k++)
			pending[k]=0;
		flsh();
	} else {
		for(k=0; k<maxasync; k++)
			if(pending[k] & WRITEIP)
				pending[k] = ERRORIP;
			else
	        		pending[k] = 0;
		for(k=0; k<maxasync; k++)
			if(bs){
			   if(pending[curibuf] & ERRORIP) {
				pending[curibuf]=0;
				obc=pendcnt[curibuf];
				flsh();
			   }
			}
			else{
			   if(pending[curobuf] & ERRORIP) {
				pending[curobuf]=0;
				obc=pendcnt[curobuf];
				flsh();
			   }
			}
	}

	reading = eomflag = 0;
	ibc = 0;
	obc = 0;
	cbc = 0;
	op = obuf[0];

loop:
	/* If input byte count = 0... */
	if(ibc-- == 0) {
		ibc = 0;
		if(count == 0 || nifr + nipr != count) {
			if(cflag & (NERR | SYNC))
			for(ip=ibuf[curibuf]+ibs; ip > ibuf[curibuf];)
				*--ip = 0;

	/* If using async. reads... */
	if(readasync) {
		int found = 0, i, j;
waitfor:
		for(i = 0, j=curibuf; i < maxasync && !found; i++) {
			if(!pending[j]) {
			    ibc = read(ibf, ibuf[j], ibs);
			    pending[j] = ibc;
			} else {
			    curibuf = j;
			    /* Get status of prior reads */
			    ibc = ioctl(ibf, FIONBDONE, &ibuf[curibuf]);
#ifdef notyet
			    if(pending[j] != ibc || bytes <= 0)
REREAD
#endif
			    	pending[curibuf] = 0;
			    found++;
			}
			if(++j >= maxasync)
				j = 0;
		}
		if(!found)
			goto waitfor;
	} else
		ibc = read(ibf, ibuf[curibuf], ibs);
	}

		if(ibc < 0) {
			if(errno != ENOSPC)
				perror("read");
			if((cflag & NERR) == 0) {
				flsh();
				reading = 1;
				term();
				if(eomflag)
					goto restart;
			}
			ibc = 0;
			for(c=0; c<ibs; c++)
				if(ibuf[curibuf][c] != 0)
					ibc = c;
			stats();
		}
		/* If input byte count is zero */
		if(ibc == 0) {
			int i;
			flsh();
			if(cflag & SPARSE)
				sparsewrite(1);

			/* signal read operation */
			reading = 1;

			(void) mvterm();

			/* If at EOT... */
			if(eomflag)
				goto restart;

			/* If no files left to get... */
			if(--files<=0) {
				stats();
				exit(0);
			}
			else{
			   for (i = 0 ; i < maxasync ; i++)
                               pending[i] = 0;
			   curibuf = 0;

			   /* Turn off nbuffered i/o */
			   /* Work-around for driver */
			   /* bug (not clearing      */
		           /* pending response bfr's)*/
			   ioctl(ibf ,FIONBUF,&curibuf);

			   /* And turn it back on again */
			   ioctl(ibf ,FIONBUF,&maxasync);
			   /*****************************/
			   /****  End of Work-around  ***/
			   /*****************************/
			}
		}
		/* If input byte count != to input block size... */
		if(ibc != ibs) {
			nipr++; /* bump partial records ctr. */
			if(cflag & SYNC)
				ibc = ibs;
		} else
			nifr++; /* else bump full records ctr. */
		ip = ibuf[curibuf];

		/* divide input byte count by 2 */
		c = ibc >> 1;

		/* If swap byte pairs... */
		if(cflag & SWAB && c)
		do {
			a = *ip++;
			ip[-1] = *ip;
			*ip++ = a;
		} while(--c);

		ip = ibuf[curibuf];
		if (fflag) {
			obc = ibc;
			flsh();

			/* If next volume... */
			if(eomflag)
				goto restart;
			ibc = 0;
		}
		goto loop;
	}
	c = 0;
	c |= *ip++;
	c &= 0377;
	if (conv == null) {
		Null(c);
	}
	else if (conv == cnull) {
		CNull(c);
	}
	else if (conv == ascii) {
		Ascii(c);
	}
	else if (conv == ebcdic) {
		Ebcdic(c);
	}
	else if (conv == ibm) {
		Ibm(c);
	}
	else (*conv)(c);

	/* If at end of media... */
	if(eomflag)
		goto restart;
	goto loop;
}

/* Flush output */
flsh()
{
	register c;

	if(obc) {
		if (cflag & SPARSE)
			c = sparsewrite(0);
		else {
			if (bs)
				c = write(obf, obuf[curibuf], obc);
			else
				c = write(obf, obuf[curobuf], obc);
		}
		if(writeasync){
		if (bs) {
			pendcnt[curibuf] = obc;
			pending[curibuf] |= WRITEIP;
			if(++curibuf == maxasync)
				curibuf = 0;

			/* Wait till this buffers done */
			if(pending[curibuf] & WRITEIP) {
			        pending[curibuf] &= ~WRITEIP;
			   	if((c=ioctl(obf, FIONBDONE, &obuf[curibuf]))
					!= pendcnt[curibuf]) {
					pending[curibuf] |= WRITEIP;
				} else
					obc = pendcnt[curibuf];
				if(c > 0)
			        	c == obs ? nofr++ : nopr++;
			    }
		}
		else {
			pendcnt[curobuf] = obc;
			pending[curobuf] |= WRITEIP;
			if(++curobuf == maxasync)
				curobuf = 0;

			/* Wait till this buffers done */
			if(pending[curobuf] & WRITEIP) {
			        pending[curobuf] &= ~WRITEIP;
			   	if((c=ioctl(obf, FIONBDONE, &obuf[curobuf]))
					!= pendcnt[curobuf]) {
					pending[curobuf] |= WRITEIP;
				} else
					obc = pendcnt[curobuf];
				if(c > 0)
			        	c == obs ? nofr++ : nopr++;
			    }
		}
		}
		if(c != obc) {
			if(errno != ENOSPC)
				perror("write");
			reading = 0;
			term();
			return;
		}
		if(!writeasync)
			obc == obs ? nofr++ : nopr++;
		obc = 0;
	} 
}

/* Create a sparse output file */
sparsewrite(done)
int done;
{
	int i, notzero=0, dummy=0;
	char *buf;

	/* Point to correct output buffer */
	if(bs)
		buf = obuf[curibuf];
	else
		buf = obuf[curobuf];

	/* If we are done then check if a last write is needed */
	if(done && (lastwriteoffset != outfileoffset)) {
	    	if(lseek(obf, (off_t)outfileoffset-1, 0) == -1)
			return(-1);
	    	if(write(obf, (char *)&dummy, 1) != 1)
			return(-1);
	}

	/* Check if output buffer contains all zeros */
	for(i=0; i<obc; i++) {
		if(buf[i] != 0) {
			notzero = 1;
			break;
		}
	}

	/* If output buffer doesn't contain all zeros then write it out */
	if(notzero) {
	    	if(lseek(obf, (off_t)outfileoffset, 0) == -1)
			return(-1);
	    	if(write(obf, buf, obc) != obc)
			return(-1);
		lastwriteoffset = outfileoffset + obc;
	}
	outfileoffset += obc;
	return(obc);
}

/* function to test if strings are equal */
/* returns(1) if equal, else returns 0   */
match(s)
char *s;
{
	register char *cs;

	cs = string;
	while(*cs++ == *s)
		if(*s++ == '\0')
			goto true;
	if(*s != '\0')
		return(0);
true:
	cs--;
	string = cs;
	return(1);
}

/* Perform number calculations for sizes */
number(big)
{
	register char *cs;
	long n;

	cs = string;
	n = 0;
	while(*cs >= '0' && *cs <= '9')
		n = n*10 + *cs++ - '0';
	for(;;)
	switch(*cs++) {

	/* calculate 'n' for kilobytes */
	case 'k':
		n *= 1024;
		continue;

	/* calculate 'n' for words */
	case 'w':
		n *= sizeof(int);
		continue;

	/* calculate 'n' for blocks */
	case 'b':
		n *= 512;
		continue;

	/* calculate product */
	case '*':
	case 'x':
		string = cs;
		n *= number(BIG);

	case '\0':
		if (n>=big || n<0) {
			fprintf(stderr, "dd: argument %D out of range\n", n);
			exit(-1);
		}
		/* return number */
		return(n);
	}
/* NEVER GETS HERE */
}


/* Places character into buffer and flushes */
/* buffer if output byte count equals the   */
/* output block size.                       */
null(c)
{

	*op = c;
	op++;
	if(++obc >= obs) {
		flsh();
		if(bs)
			op = obuf[curibuf];
		else
			op = obuf[curobuf];
	}
}

/* function to convert a lower case character to upper case */
/* or to convert an upper case character to lower case      */
cnull(cc)
{
	register c;

	c = cc;
	if(cflag & UCASE && c >= 'a' && c <= 'z')
		c += 'A' - 'a';
	if(cflag & LWRCASE && c >= 'A' && c <= 'Z')
		c += 'a' - 'A';
	Null(c);
}


ascii(cc)
{
	register c;

	c = etoa[cc] & 0377;
	if(cbs == 0) {
		CNull(c);
		return;
	}
	if(c == ' ') {
		nspace++;
		goto out;
	}
	while(nspace > 0) {
		Null(' ');
		nspace--;
	}
	CNull(c);

out:
	if(++cbc >= cbs) {
		Null('\n');
		cbc = 0;
		nspace = 0;
	}
}




/* function to convert fixed length */
/* records to variable length rec's */
unblock(cc)
{
	register c;

	c = cc & 0377;
	if(cbs == 0) {
		CNull(c);
		return;
	}
	if(c == ' ') {
		nspace++;
		goto out;
	}
	while(nspace > 0) {
		Null(' ');
		nspace--;
	}
	CNull(c);

out:
	if(++cbc >= cbs) {
		Null('\n');
		cbc = 0;
		nspace = 0;
	}
}

/* function to convert ASCII to EBCDIC */
ebcdic(cc)
{
	register c;

	c = cc;
	if(cflag & UCASE && c >= 'a' && c <= 'z')
		c += 'A' - 'a';
	if(cflag & LWRCASE && c >= 'A' && c <= 'Z')
		c += 'a' - 'A';
	c = atoe[c] & 0377;
	if(cbs == 0) {
		Null(c);
		return;
	}
	if(cc == '\n') {
		while(cbc < cbs) {
			Null(atoe[' ']);
			cbc++;
		}
		cbc = 0;
		return;
	}
	if(cbc == cbs)
		ntrunc++;
	cbc++;
	if(cbc <= cbs)
		Null(c);
}

/* function to convert ASCII to IBM EBCDIC */
ibm(cc)
{
	register c;

	c = cc;
	if(cflag & UCASE && c >= 'a' && c <= 'z')
		c += 'A' - 'a';
	if(cflag & LWRCASE && c >= 'A' && c <= 'Z')
		c += 'a' - 'A';
	c = atoibm[c] & 0377;
	if(cbs == 0) {
		Null(c);
		return;
	}
	if(cc == '\n') {
		while(cbc < cbs) {
			Null(atoibm[' ']);
			cbc++;
		}
		cbc = 0;
		return;
	}
	if(cbc == cbs)
		ntrunc++;
	cbc++;
	if(cbc <= cbs)
		Null(c);
}

/* function to convert variable length */
/* records to fixed length records     */
block(cc)
{
	register c;

	c = cc;
	if(cflag & UCASE && c >= 'a' && c <= 'z')
		c += 'A' - 'a';
	if(cflag & LWRCASE && c >= 'A' && c <= 'Z')
		c += 'a' - 'A';
	c &= 0377;
	if(cbs == 0) {
		Null(c);
		return;
	}
	if(cc == '\n') {
		while(cbc < cbs) {
			Null(' ');
			cbc++;
		}
		cbc = 0;
		return;
	}
	if(cbc == cbs)
		ntrunc++;
	cbc++;
	if(cbc <= cbs)
		Null(c);
}
/*
 Terminate showing number of full and partial records read(written)
 on error condition occurance 
*/
term()
{
	if(mvterm())
		return;
	stats();
	exit(-1);
}

/* Control-C Interrupt exit code */
void
term2()
{
	stats();
	exit(2);
}

/* Next volume code */
mvterm() {
	extern int errno;

	eomflag = 0;

	if(reading) { /* check tape on read to see if at eot */
		struct devget mt_info;
		if (ioctl(ibf,DEVIOCGET,(char *)&mt_info) >= 0) {
			if(mt_info.stat&DEV_EOM && mt_info.category == DEV_TAPE)
				errno = ENOSPC;
			else {
				if(maxasync) {
				int i, rcnt=0, bytes;
				for(i=0; i<maxasync; i++) {
					if((bytes = ioctl(ibf, FIONBDONE, 
						&ibuf[i])) > 0)
					        if(bytes <= pending[i])
							rcnt++;
				}
				mtcom.mt_op = MTCSE;
				mtcom.mt_count = 1;
				(void) ioctl(ibf, MTIOCTOP, &mtcom);
				mtcom.mt_op = MTBSR;
				mtcom.mt_count = rcnt;
				(void) ioctl(ibf, MTIOCTOP, &mtcom);
				}
				errno = EIO;
			}
		 } else {
			return(0);
		}
	} else {
		struct devget mt_info;
		if (ioctl(obf,DEVIOCGET,(char *)&mt_info) >= 0) {
			if(mt_info.stat&DEV_EOM && mt_info.category == DEV_TAPE)
				errno = ENOSPC;
			else 
				errno = EIO;
		 } else
			return(0);
	}
	
	/* If at End of Volume and in multi-volume mode */
	if((cflag & MVOL) && errno == ENOSPC && !eomflag) {
		struct mtop mt_op;
		int f;
		fprintf(stderr, "dd: end of media, load another volume\n");
		
		/* Set up an Off-line ioctl */
		mt_op.mt_op = MTOFFL;
		mt_op.mt_count = 1;

		/* If the tape is at EOT, place it off-line */
		/* and wait...				    */	
		if(reading) {
			eomflag=1;
			close(ibf);
			while((f = open(ifile, 0)) < 0)
				sleep(3);
			(void)ioctl(f, MTIOCTOP, &mt_op);
			close(f);
			while((f = open(ifile, 0)) < 0)
				sleep(10);
			close(f);
		} else {
			eomflag=1;
			close(obf);
			while((f = open(ofile, 0)) < 0)
				sleep(3);
			(void)ioctl(f, MTIOCTOP, &mt_op);
			close(f);
			while((f = open(ofile, 0)) < 0)
				sleep(10);
			close(f);
		}
		errno = 0;
	}
done:
	return(eomflag == 1);

}

/* Report number of full and partial records read(written) */
stats()
{
	register int i, tobc;
	/* Check pending write buffers */
	for(i=0; i<maxasync && writeasync; i++) 
		if(pending[i] & WRITEIP) {
		if((tobc = ioctl(obf, FIONBDONE, &obuf[i])) > 0)
			tobc == obs ? nofr++ : nopr++;
		pending[i] &= ~WRITEIP;
		}

	fprintf(stderr,"%u+%u records in\n", nifr, nipr);
	fprintf(stderr,"%u+%u records out\n", nofr, nopr);
	if(ntrunc)
		fprintf(stderr,"%u truncated records\n", ntrunc);
}

/* Routine to obtain generic device status */
/* used to determine if device is on-line  */
/* and write-enabled for write operations  */
statchk(tape,mode)
	char	*tape;
	int	mode;
{
	int to;
	struct devget mt_info;
	struct mtop tops;
	int error = 0;

	/* Force device open to obtain status */
	to = open(tape,mode|O_NDELAY);

	/* If open error, then error must be no such device and address */
	if (to < 0)goto reopen;
	
#ifdef notdef
	/* Issue ioctl to enable cache */
	tops.mt_op = MTCACHE;
	tops.mt_count = 1;
	ioctl(to,MTIOCTOP,&tops);
#endif

	/* Get generic device status */
	if (ioctl(to,DEVIOCGET,(char *)&mt_info) < 0) goto closeit;

	/* Check for device on line */
	if(mt_info.stat & DEV_OFFLINE){
	  fprintf(stderr,"\7\nError on device named %s - Place %s device unit #%u ONLINE\n",tape,mt_info.device,mt_info.unit_num);
	  error = 1;
	}

	/* Check for device write locked when in write mode */
	else
	 if((mt_info.stat & DEV_WRTLCK) && (mode != O_RDONLY)){
           fprintf(stderr,"\7\nError on device named %s - WRITE ENABLE %s device unit #%u\n",tape,mt_info.device,mt_info.unit_num);
	   error = 1;
	 }
	   
closeit:

reopen:
	/* Re-Open as user requested */
	if(error == 0){
	   if(to < 0)
	   	to = open(tape,mode);
	   return(to);
        }
	else 
	   return(-1);
}
