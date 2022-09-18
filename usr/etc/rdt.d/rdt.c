#ifndef lint
static	char	*sccsid = "@(#)rdt.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
/****************************************************************
*								*
*	RDT.C	-->	Read Diagnostic Tape			*
*								*
*	Ray Glaser	09-Oct-84				*
*								*
****************************************************************/


/*
	/--------------------------------------------------\

		Read Diagnostic Programs from the

			Diagnostic Tape

		A subset of the labeled tape facility (ltf)

	\--------------------------------------------------/

*/
#include	<stdio.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/ioctl.h>
#include	<sys/mtio.h>
#include	<sys/dir.h>
#include	<sys/time.h>
#include	<sys/stat.h>

/*	Define constants for sizes of various items.
 */

#define	NNML	100	/* Normal (ANSI) file name length.
			   76 is all that seems to be used on the tape, so
			   why this was def'd to 100 isn't known yet. rjg
			*/

#define BUFSIZE	80
#define LABSIZE	80	/* Number of chctrs in an ANSI label */


/*	Define constants for ?
 */

#define CREATE		1
#define WRITE		2
#define EXTRACT		3
#define TABLE		4

#define BINARY		10
#define TEXT		-10
#define COUNTED		-11

#define YES		1
#define NO		0

#define MAXBLKSIZE	20480		 /* maximum tape block length */

#define MAXRECSIZE	512		/* maximum tape record length */
#define MAXLEN		95		/*
					 * maximum length of a file name,
					 * including its pathname
					 */

#define MAXRECFUF	130
#define MAXREC4		126
#define MAXREC6		124
#define RECOFF		6		/* fuf record offset */
#define FIRST		1
#define MIDDLE		0
#define LAST		2
#define ALL		3

#define	FIXED		'F'		/* fixed-length tape records */
#define VARIABLE	'D'		/* variable-length tape records */
#define FUF		01		/* fortran unformatted file */
#define DD		02		/* direct dump */
#define PAD		'^'		/* padding character */

#define SYSNAME		"ULTRIX"	/* system name */


/* field sizes */

#define	LABID	3
#define	SERIAL	6
#define	OWNER	14
#define	FILEID	17
#define	SETID	6
#define	SECNO	4
#define	SEQNO	4
#define	GENNO	4
#define	GENVSNO	2
#define	CRDATE	6
#define	EXDATE	6
#define	BLOCKS	6
#define	SYSTEM	13
#define	BLKMAX	5
#define	RECMAX	5
#define	BYTOFFS	2
#define DBYTES	4

/* pad fields (reserved for future use) */
#define	VRES1	20
#define	VRES2	6
#define	VRES3	28
#define	HRES1	7
#define	H2RES1	35
#define	H2RES2	28

#ifdef oktoc
/* Volume header label */
struct vol {
	char	v_labid[LABID];		/* label identifier "VOL" */
	char	v_labno;		/* label number */
	char	v_serial[SERIAL];	/* volume serial number */
	char	v_access;		/* accessibility */
	char	v_res1[VRES1];		/* reserved for future use */
	char	v_res2[VRES2];		/* reserved for future use */
	char	v_owner[OWNER];		/* owner identifier */
	char	v_res3[VRES3];		/* reserved for future use */
	char	v_stdlabel;		/* standard label flag */
	};




/* file header/eof label */
struct hdr {
	char	h_labid[LABID];		/* label identifier:
					 *  "HDR" or "EOF" */
	char	h_labno;		/* label number */
	char	h_fileid[FILEID];	/* file identifier */
	char	h_setid[SETID];		/* file set identifier */
	char	h_secno[SECNO];		/* file section number */
	char	h_seqno[SEQNO];		/* file sequence number */
	char	h_genno[GENNO];		/* generation number */
	char	h_genvsno[GENVSNO];	/* generation vsn number */
	char	h_crdate[CRDATE];	/* creation date */
	char	h_exdate[EXDATE];	/* expiration date */
	char	h_access;		/* accessibility */
	char	h_blocks[BLOCKS];	/* block count */
	char	h_system[SYSTEM];	/* system code */
	char	h_x1[7];		/* reserved */
	};

/* deblocking hdr2/eof2 label */
struct hd2 {
	char	h2_labid[LABID];	/* label identifier:
					 * "HDR" or "EOF" */
	char	h2_labno;		/* label number "2" */
	char	h2_datatype;		/* type identifier "D" or "F" */
	char	h2_blkmax[BLKMAX];	/* maximum block size - bytes */
	char	h2_recmax[RECMAX];	/* max record size - bytes */
	char	h2_x1[H2RES1];		/* reserved */
	char	h2_bytoffs[BYTOFFS];	/* extra stuff at the 
					 * start of block  ?? */
	char	h2_x2[H2RES2];		/* reserved */
	};



struct vol vol1;
struct hdr hdr1, eof1;
struct hd2 hdr2, eof2;

#endif /* oktoc */

/* --> In struct__filestat (below) the following members are
 *	used as indicated.
 *
 * f_src
 * ------
 * When appending files, f_src is the pathname of the file as it
 * appears on disk.  When extracting files, it contains the
 * "extracted file name" (if one is specified).  It is unused when
 * listing files.
 *
 * f_dest
 * ------
 * f_dest always deals with the file names (only the file identifiers)
 * on tape, whereas f_src only works with file names on disk.
 * f_dest and f_version together identify each file on tape.
 *
 * f_version
 * ------
 * the version number of tape files.  The default value when
 * listing or extracting files is '*' (i.e., everything).
 * When appending files, the default is 1.
 *
 * f_numleft
 * ------
 * If a number between curly braces (denoting the nth occurrence)
 * is appending to a file name, f_numleft is set equal to that
 * number.  In this case, f_numleft is decremented every time
 * the file appears on tape until it is equal to 0.  If no
 * occurrence is specified, f_numleft is set equal to -1.
 * The file will only be processed if f_numleft is equal to
 * -1 or 1.
 *
 * f_flags
 * ------
 * f_flags is used when extracting files to indicate if the file
 * should be directly dumped (dd) from tape, FUF converted, or
 * extracted normally.
 */

struct filestat
	{
	char	f_src[MAXLEN];
	char	f_dest[MAXLEN];
	char	f_version[MAXLEN];
	int	f_numleft;
	int	f_flags;
	struct	filestat *f_next;
	} *f_head;

/*
 * alinkbuf is the link table when appending files.
 * It contains information about all head link files.
 */
struct alinkbuf
	{
	ino_t	a_inum;			/* inode number */
	dev_t	a_dev;			/* device */
	int	a_count;		/*
					 * a_count is the number of
					 * links that have not yet been
					 * appended to tape (if they
					 * ever will).  At first,
					 * a_count is equal to the
					 * number of links; then it is
					 * decremented each time a link
					 * is appended to tape.
					 */
	char	a_pathname[NNML];	/* pathname of head link file */
	int	a_fsecno;		/*
					 * file section number of head
					 * link file
					 */
	int	a_fseqno;		/*
					 * file sequence number of head
					 * link file
					 */
	struct	alinkbuf *a_next;
	} *a_head;

/*
 * xlinkbuf is the link table when extracting files.  It contains
 * information about all potential head link files that have been
 * extracted so far.
 *  xlinkbuf is important because files are only linked
 * if the appropriate head link file has already been extracted.
 * The "appropriate" head link file is determined by matching file
 * section numbers and matching file sequence numbers (rather than by
 * pathname because alternate extracted file names can be specified).
 */
struct xlinkbuf
	{
	int	x_fsecno;		/*
					 * file section no. of a
					 * potential head link file
					 */
	int	x_fseqno;		/*
					 * file sequence no. of a
					 * potential head link file
					 */
	char	x_pathname[NNML];	/* extracted file name */
	struct	xlinkbuf *x_next;
	} *x_head;

struct stat inode;

FILE	*magtfp;			/* file pointer to tape device */
long	block;				/* number of tape blocks extracted */
static	char *progname;			/* program name for error routines */
static	int numrecs = 0;		/* number of records to process */
static	char label[7] = "ULTRIX";	/* tape label (volume identifier) */
static	char labelbuf[(BUFSIZE*2)+2];	/* buffer for reading tape labels */
int	blocksize = 2048;		/* tape blocksize */
int	reclength = MAXRECSIZE;		/* record length */
static	int verbose = NO;		/* verbose flag */
int	func;				/* function (crxt) */
char	inbuf[MAXBLKSIZE+1];		/* buffer */
char	*bb;				/* fuf buffer (block) pointer */
char	*rb;				/* fuf record pointer */
int	nskip = 0;			/* number of files to skip with -S */
int	oflag = NO;			/* -O option */
int	oskip = 0;			/* number of files to skip with -O */
char	posname[MAXLEN];		/* position file name (-P option) */
char	posnum[MAXLEN];			/* version number of position file */
int	posnth = 0;			/* nth occurrence of position file */
int	poscount;			/* number of occurrences left to find */
int	pos = 0;			/*
					 * pos == 0 if the -P option is
					 * not used.  pos == -1 if a
					 * position file has been
					 * specified, but has not yet 
					 * been found.  pos == 1 after
					 * the position file has been
					 * matched on tape.
					 */
int	filetype = 0;			/*
					 * to override normal file type
					 * (FIXED or VARIABLE).
					 */
int	use_versnum = NO;		/*
					 * indicates whether to
					 * interpret version numbers
					 * from input file names (-N flag).
					 */
int	warning_only = YES;		/* For 'rdt', we don't want to ask
					 * if we should overwrite existing
					 * files. ie. AUTO-OVERWRITE is
					 * assumed. Formerly: program
					 * prompts you after certain
					 * errors if the -W flag is set
					 */
int	rep_misslinks = NO;		/*
					 * reports undumped links
					 * if the -M flag is set
					 */
int	toggle = NO;			/* toggles file type specifications */
int	freemem = YES;			/* free memory flag */
int	mstrcmp();			/* compares strings of metacharacters */
int	mstrcmp_dot();			/*
					 * compares strings of metacharacters
					 * while distinguishing between
					 * file names and file types
					 */
int	(*cmp)() = mstrcmp;		/* pointer to string compare routine */
static	char magtdev[MAXLEN] = "/dev/rmt8";	/* default tape device name */

/****************************************
 *
 *	FILE  headr/eof  label ->  HDR1
 *
 ****************************************/

/* (hdr1) 
----------*/

static	char	l_labelid[4];		/*[03] label identifier -
					 *     "HDR" or "EOF" */
static	int	l_labelno;		/*[01] label number */
static	char	l_filename[18];		/*[17] file identifier */
static	char	l_volid[7];		/*[06] volume identifier
					 *     (tape label) 
					 *     (file set identifier) */
static	int	l_fsecno;		/*[04] file section number */
static	int	l_fseqno;		/*[04] file sequence number */
static	int	l_gen;			/*[04] generation number */
static	int	l_genver;		/*[02] genration vrsn number */
static	char	l_credate[6];		/*[06] creation date */
static	char	l_expirdate[6];		/*[06] expiration date */
/*static char	l_access[]=" ";		 *[01] accessibility */
static	long	l_nblocks;		/*[06] block count */
/*static char	l_sys_code[13];		 *[13] system identifier */
/*static char	l_rsvd1_hdr1[7];	 *[07] reserved for future
					 *     standardization */


/****************************************
 *
 *	FILE  headr/eof  label ->  HDR2
 *
 ****************************************/

/* (hdr2) 
----------*/

/*static char	l_labelid[4];		 *[03] label identifier -
					 *     "HDR" or "EOF" */
/*static int	l_labelno;		/*[01] label number */
static	char	l_recformat;		/*[01] record format
					 *	F = fixed length
					 *	D = variable length
					 *	S = spanned  */
static	int	l_blklen;		/*[05] block length -
					 *     no. chctrs/block */
static	int	l_reclen;		/*[05] record length 
					 * If the record format is
					 * FIXED, this contains the
					 * actual record length.  If
					 * the record format if
					 * VARIABLE, this is the
					 * maximum record length
					 * including the count field.
					 */
/*static char	l_rsvd1_hdr2[35];	 *[35] reserved for sys use */


/****************************************
 *
 *	FILE  headr/eof  label ->  HDR3 - HDR9
 *
 ****************************************/

/* (hdr3 - hdr9) 
---------------*/

/*static char	l_labelid[4];		 *[03] label identifier -
					 *     "HDR" or "EOF" */
/*static int	l_labelno;		/*[01] label number */



/* ?? */

static	char	l_systemid[14];		/*
					 * identifies the system
					 * that recorded the file
					 */
static	char spaces[LABSIZE];		/* a string of spaces */
static	int fsecno;			/* fsecno of next file to be appended */
static	int fseqno;			/* fseqno of next file to be appended */

/*---------------------------------------------------------------------
 *
 *  MAIN  -  checks flags and calls the appropriate subroutine to
 *	     begin processing the file arguments.
 *
 *-------------------------------------------------------------------*/
main(argc, argv)
int	argc;
char	*argv[];
	{
	char	bsize[6];		/* blocksize from the -B option */
	char	*a;			/* pointer to *argv */
	int	i;
	int	iflag = 0;		/*
					 * the -I option.  iflag = 0 if
					 * no arguments othan ther the
					 * command-line arguments are
					 * to be processed.  iflag = -1 
					 * if the standard input is to
					 * be read for more file names.
					 * iflag = 1 if a file
					 * containing additional file
					 * arguments is given.
					 */
	char	inputfile[MAXLEN];	/*
					 * If iflag = 1, inputfile is
					 * the file containing additional
					 * file arguments.  If iflag = -1,
					 * inputfile[0] = '-' to indicate
					 * that the user is to be
					 * prompted for the other file
					 * names; otherwise,  inputfile[0] = 0
					 * for no prompting.
					 */
	FILE	*ifp;			/*
					 * file pointer to inputfile
					 * (if iflag = 1)
					 */

	for(i=0; i < LABSIZE; i++)
		spaces[i] = ' ';

	/* so the error routines know the name of this program */
	progname = argv[0];

	fprintf(stderr, "\n%s: ", progname);
/*	Get the function code...  general format of the command is:
 *
 *		ltf  function  -qualifier  filename(s)
 *
 */
	if(--argc > 0)
		for(a = *++argv; *a != '\0'; a++)
			switch(*a)
				{
				/*
				case 'c':
					func = CREATE;
					break;
				case 'r':
					func = WRITE;
					break;
				*/
				case 'x':
					func = EXTRACT;
					break;
				case 't':
					func = TABLE;
					break;
				case 'v':
					verbose = YES;
					break;
				case '-':
					errorexit("no function specified", 0, 0);
					break;
				default:
					error("%c: unknown function", *a, 0);
					usage();
				}
	else
		usage();

	argv++;

/*
 *	Now process the qualifier(s)....
 *
*/
	for(argc-- ; (a = *argv)[0] == '-' && *(a+1) != 'b' && *(a+1) != 't'
		&& *(a+1) != 'u' && *(a+1) != 'd' && *(a+1) != 0 && argc > 0;
		argc--, argv++)
		switch(*++a)
			{
			char	*s1, *s2;

			case 'D':
				if(*++a == 0)
					errorexit("no tape device specified", 0, 0);
				if( strcmp(a, "800")==0)
					strcpy(magtdev, "/dev/rmt0");
				else if( strcmp(a, "1600")==0)
					strcpy(magtdev, "/dev/rmt8");
				else if( strcmp(a, "6250")==0)
					strcpy(magtdev, "/dev/rmt9");
				else
					{
					/*
					 * The name following '-D' is used
					 * literally as the tape device name.
					 */
					s1 = magtdev;
					s2 = a;
					for(i=0; (*s1++ = *s2++) && i < MAXLEN; i++);
					if(i >= MAXLEN)
						errorexit("%s: file name too long", a, 0);
					}
				break;

			case 'L':
				if(strlen(++a) > 6)
					errorexit("maximum label length is 6", 0, 0);
				strcpy(label, a);
				upper(label);
				break;
			case 'B':
				sscanf(++a, "%5s", bsize);
				blocksize = num(bsize);
				if(blocksize > MAXBLKSIZE || blocksize < 4)
					errorexit("invalid blocksize (max %d).",
					MAXBLKSIZE, 0);
				break;

			case 'I':
				s1 = inputfile;
				s2 = ++a;
				for(i=0; (*s1++ = *s2++) && i < MAXLEN; i++);
				if(i >= MAXLEN)
					{
					error("%s: file name too long", a, 0);
					break;
					}
				if(*a == 0 || *a == '-')
					iflag = -1;
				else
					{
					iflag = 1;
					if((ifp = fopen(inputfile, "r")) == NULL)
						errorexit("cannot open %s", inputfile, 0);
					}
				break;
#if 0
/*
			case 'R':
				sscanf(++a, "%d", &reclength);
				break;
			case 'O':
				if(func == CREATE)
					errorexit("the -O option does not apply to the 'c' function.", 0, 0);
				sscanf(++a, "%d", &oskip);
				if(oskip < 0)
					errorexit("invalid number of skipped files", 0, 0);
				oflag = YES;
				break;
			case 'S':
				sscanf(++a, "%d", &nskip);
				if(nskip < 0)
					errorexit("invalid number of skipped files", 0, 0);
				/*
				 * each LTF file entry consists of
				 * three actual tape files.
				 */
				nskip *= 3;
				break;
			case 'P':
				if(*++a == 0)
					errorexit("no position file specified", 0, 0);
				/* check for an occurrence request */
				s1 = a;
				s1 += strlen(a) - 1;
				if(*s1 == '}')
					{
					while(--s1 != a && *s1 >= '0' && *s1 <= '9');
					if(*s1 == '{')
						{
						*s1 = 0;
						if(*++s1 < '1' || *s1 > '9')
							errorexit("invalid occurrence request in position file.", 0, 0);
						sscanf(s1, "%d", &poscount);
						posnth = poscount;
						}
					else
						poscount = -1;
					}
				else
					poscount = -1;
				if(! make_num(a, posname, posnum))
					errorexit("invalid file name for -P option", 0, 0);
				pos = -1;
				break;

			case 'N':
				use_versnum = YES;
				cmp = mstrcmp_dot;
				break;
			case 'W':
				warning_only = YES;
				break;
			case 'M':
				rep_misslinks = YES;
				break;
			case 'T':
				toggle = YES;
				if(func == TABLE)
					error("the -T option does not apply when listing files.", 0, 0);
				break;
*/
#endif	
			default:
				error("%c: unknown flag", *a, 0);
				usage();
			}

	if(reclength > MAXRECSIZE || reclength < 1)
		errorexit("invalid record length (max %d).", MAXRECSIZE, 0);


	if(func == CREATE || ! oflag)
		rew();

	/*
	 * If LTF is to create a new tape, magtdev will be opened
	 * for writing in createtp().
	 */
	if(func != CREATE && ((magtfp = fopen(magtdev, "r")) == NULL))
		errorexit("cannot open %s for reading", magtdev, 0);


	if(func == TABLE || func == EXTRACT)
		{
		int	dumpflag = 0;

		for( ; argc > 0; argc--, argv++)
			{
			if(! strcmp(*argv,"-d"))
				{
				if(func == EXTRACT)
					{
					/*
					 * dumpflag is not set to 0 if toggle
					 * is NO because we want to check
					 * in scantape() if DD and FUF have
					 * together been specified for one
					 * file.
					 */
					if(toggle)
						dumpflag = 0;
					dumpflag |= DD;
					}
				else
					error("the -d option does not apply when listing files", 0, 0);
				}

			else if(! strcmp(*argv,"-u"))
				{
				if(func == EXTRACT)
					{
					if(toggle)
						dumpflag = 0;
					dumpflag |= FUF;
					}
				else
					error("the -u option does not apply when listing files", 0, 0);
				}

			else if(! strcmp(*argv, "-"))
				dumpflag = 0;
			else if(! strcmp(*argv, "-b") || ! strcmp(*argv, "-t"))
				error("-b and -t flags are not needed to list or extract files", 0, 0);
			else if((*argv)[0] == '-')
				error("%s: unknown flag", *argv, 0);
			else
				{
				rec_args(*argv, dumpflag);
				if(! toggle)
					dumpflag = 0;
				}
			}

		if(iflag == 1)
			rec_file(ifp, '\0', "");
		else if(iflag == -1)
			rec_file(stdin, inputfile[0], "");
		scantape();
		}

	else if(func == WRITE)
		{
		if(iflag == 1)
			fclose(ifp);
		writetp(argc, argv, iflag, inputfile);
		}
	else if(func == CREATE)
		{
		if(iflag == 1)
			fclose(ifp);
		createtp(argc, argv, iflag, inputfile);
		}
	else errorexit("no function specified", 0, 0);
	}

/*---------------------------------------------------------------------
 *
 *  REC_ARGS  -  sorts file arguments into 'filestat' structure.
 *
 *-------------------------------------------------------------------*/
rec_args(filename, dumpflag)
char	*filename;
int	dumpflag;
	{
	struct	filestat *fstat;
	char	*l;

	fstat = (struct filestat *) malloc(sizeof(*fstat));
	if(fstat == NULL)
		errorexit("Too many arguments.  Out of memory.", 0, 0);

	fstat->f_next = f_head;
	f_head = fstat;

	fstat->f_src[0] = 0;

	l = filename;
	l += strlen(filename) - 1;

	/* check for an occurrence request */
	if(*l == '}')
		{
		while(*--l >= '0' && *l <= '9');
		if(*l == '{')
			{
			*l = 0;
			if(*++l < '1' || *l > '9')
				{
				error("invalid occurrence request .  {%s part of %s ignored.", l, filename);
				fstat->f_numleft = -1;
				}
			else
				sscanf(l, "%d", &fstat->f_numleft);
			}
		else
			fstat->f_numleft = -1;
		}
	else
		fstat->f_numleft = -1;

	if(make_num(filename, fstat->f_dest, fstat->f_version))
		{
		numrecs++;
		fstat->f_flags = dumpflag;
		}
	else
		{
		f_head = fstat->f_next;
		free((char *)fstat);
		}


	return;
	}

/*---------------------------------------------------------------------
 *
 *  ERROR  -  prints non-fatal error messages.  Arguments are passed
 *	      to 'error' like this because fprintf does not recognize
 *	      the %r format option.
 *
 *-------------------------------------------------------------------*/
error(s1, s2, s3)
char	*s1, *s2, *s3;
	{
	fprintf(stderr, "%s WARNING: ", progname);
	fprintf(stderr, s1, s2, s3);
	fprintf(stderr, "\n");
	}

/*---------------------------------------------------------------------
 *
 *  ERROREXIT  -  prints fatal error messages.
 *
 *-------------------------------------------------------------------*/
errorexit(s1, s2, s3)
char	*s1, *s2, *s3;
	{
	fprintf(stderr, "%s ERROR: ", progname);
	fprintf(stderr, s1, s2, s3);
	fprintf(stderr, "\n");
	exit(1);
	}

/*---------------------------------------------------------------------
 *
 *  UPPER  -  converts lower-case letters in the string 's' to
 *	      upper-case.
 *
 *-------------------------------------------------------------------*/
upper(s)
char	*s;
	{
	while(*s != 0)
		{
		if(islower(*s))
			*s = toupper(*s);
		s++;
		}
	}

/*---------------------------------------------------------------------
 *
 *  LOWER  -  converts upper-case letters in the string 's' to
 *	      lower-case.
 *
 *-------------------------------------------------------------------*/
lower(s)
char	*s;
	{
	while(*s != 0)
		{
		if(isupper(*s))
			*s = tolower(*s);
		s++;
		}
	}

/*---------------------------------------------------------------------
 *
 *  SCANTAPE  -  scans the tape for the TABLE and EXTRACT functions.
 *
 *-------------------------------------------------------------------*/
scantape()
	{
	char	name[MAXLEN];		/*
					 * extracted file name according
					 * to the tape labels.
					 */
	char	xname[MAXLEN];		/*
					 * actual extracted file name
					 * if different from 'name'
					 * above.
					 */
	long	ret;
	long	xtractf();
	long	charcnt;		/*
					 * character count of locally-
					 * appended binary files.
					 * This is just a precaution
					 * in case the last character
					 * of a binary file is the
					 * same as the padding
					 * character.
					 */
	long	modtime;		/*
					 * modification time of
					 * locally-appended files
					 */
	int	verno_exp;		/* expanded version of version number */
	char	verno_exp_a[6];		/* ascii representation of verno_exp */
	char	pathname[77];
	char	cat_misc[15];		/* misc. string used for concatenating */
	int	mode;			/* file mode */
	int	uid;			/* user id */
	int	gid;			/* group id */
	int	linkflag;		/* set if link() is successful */
	int	lnk_fsecno;		/* file section no. of head link file */
	int	lnk_fseqno;		/* file sequence no. of head link file */
	char	lnk_name[MAXLEN];	/* file name of head link file */
	struct	xlinkbuf *lp;
	struct	filestat *lookup();
	struct	filestat *fstat;
	static	char sdate[13];		/* creation-date string */
	int	i;


/* Read the VOLUME LABEL (VOL1) from the input tape.
*/
	if(! oflag)
		{
		i = read(fileno(magtfp), labelbuf, LABSIZE);
		if(i < 0)
			errorexit("cannot read tape.  Incorrect tape density???", 0, 0);
		else if(i == 0)
			errorexit("cannot read	VOL1  from tape.  Empty tape???", 0, 0);
	
		l_labelid[0] = 0;
		sscanf(labelbuf, "%3s%1d%6s", l_labelid, &l_labelno, l_volid);
#ifdef DEBUG
	pr_label();
#endif
		if(strcmp(l_labelid, "VOL"))
			{
			errorexit("%s: illegal label format (VOL)", l_labelid, 0);
			}
		if(strcmp(l_volid, label))
				strcpy(label, l_volid);
				upper(label);
			fprintf(stderr,"\nVolume label is: %s\n\n", l_volid);
		}
	else if(oskip && skip(oskip) != oskip)
		errorexit("there aren't that many files to skip (-O).", 0, 0);

	/*
	 * turn off oflag because the -S option does not skip
	 * over valid end-of-tape marks like -O option does.
	 * Re: skip()
	 */
	oflag = NO;

	if(nskip && (skip(nskip) != nskip))
		errorexit("there aren't that many files to skip (-S).", 0, 0);

	for(;;)
		{
		pathname[0] = 0;
		labelbuf[BUFSIZE] = 0;
		charcnt = 0L;
		modtime = 0L;
		linkflag = NO;
		lnk_fsecno = 0;
		lnk_fseqno = 0;
/*
   Read  HDR1  from the tape.
*/
		if(read(fileno(magtfp), labelbuf, LABSIZE) <= 0)
			{
			if(pos == -1)
				not_on_tape();
			exit(0);
			}

		sscanf(labelbuf, "%3s%1d%17s%6s%4d%4d%4d%2d %5s %5s%*7c%13s",
		l_labelid, &l_labelno, l_filename, l_volid, &l_fsecno,
		&l_fseqno, &l_gen, &l_genver, l_credate, l_expirdate, l_systemid);

		lower(l_filename);

		if( ! strcmp(l_labelid, "EOV"))
			{
			if(pos == -1)
				not_on_tape();
			exit(0);
			}

#ifdef DEBUG
	pr_label();
#endif
		if(strcmp(l_labelid, "HDR") || l_labelno != 1)
			{
			errorexit("%s%d: illegal label format (HDR1)",
			l_labelid, l_labelno);
			}
/*
   Read  HDR2  from the tape.
*/
		if(read(fileno(magtfp), labelbuf, LABSIZE) <= 0)
			errorexit("cannot read HDR2 from tape", 0, 0);

		sscanf(labelbuf, "%3s%1d%c%5d%5d", l_labelid,
		&l_labelno, &l_recformat, &l_blklen, &l_reclen);
#ifdef DEBUG
	pr_label();
#endif

		if(strcmp(l_labelid, "HDR") || l_labelno != 2)
			{
			errorexit("%s%d: illegal label format (HDR2)",
			l_labelid, l_labelno);
			}
/*
   Ask the question:  IS THIS ONE OF OUR TAPES ?
*/
		if(! strcmp(l_systemid, SYSNAME))
			{
/*
   Read  HDR3  from the tape (if one of ours)..
*/
			if(read(fileno(magtfp), labelbuf, LABSIZE) <= 0)
				errorexit("cannot read HDR3 from tape", 0, 0);

			labelbuf[LABSIZE] = 0;
#ifdef DEBUG
	pr_label();
#endif
			sscanf(labelbuf, "%3s%1d%76s", l_labelid,
			&l_labelno, pathname);

			if(strcmp(l_labelid, "HDR") || l_labelno != 3)
				{
				errorexit("%s%d: illegal label format (HDR3)",
				l_labelid, l_labelno);
				}
/*
	WHAT DOES THE FOLLOWING TEST IMPLY ?
*/
			if(labelbuf[4] != ' ')
				lower(pathname);
/*
   Read  HDR4  from the tape. Ultrix native mode tapes always have
   HDR1  thru  HDR4  labels.
*/
			if(read(fileno(magtfp), labelbuf, LABSIZE) < 0)
				errorexit("cannot read HDR4 from tape", 0, 0);
#ifdef DEBUG
	pr_label();
#endif

			sscanf(labelbuf, "%3s%1d%4o%4d%4d%4d%4d%10ld%10ld", l_labelid,
			&l_labelno, &mode, &uid, &gid,
			&lnk_fsecno, &lnk_fseqno, &charcnt, &modtime);

			if(strcmp(l_labelid, "HDR") || l_labelno != 4)
				{
				errorexit("%s%d: illegal label format (HDR4)",
				l_labelid, l_labelno);
				}
/*
   We only try to read	HDR5  "if"  this is a native mode Ultrix tape
   and the current file has (or is) a link.
*/
			if(func == TABLE && lnk_fsecno != 0 && lnk_fseqno != 0)
				{
				if(read(fileno(magtfp), labelbuf, LABSIZE) < 0)
					errorexit("cannot read HDR5 from tape", 0, 0);
#ifdef DEBUG
	pr_label();
#endif

				sscanf(labelbuf, "%3s%1d%76s", l_labelid,
				&l_labelno, lnk_name);
				if(strcmp(l_labelid, "HDR") || l_labelno != 5)
					{
					errorexit("%s%d: illegal label format (HDR5)",
					l_labelid, l_labelno);
					}
				}
			}

		/* check if file is an FUF */
		else if(! strcmp(l_systemid, "DECFILE11A"))
			{
			int	num1 = 0;
			int	num2 = 0;

			if((read(fileno(magtfp), labelbuf, LABSIZE)) <= 0)
				{
				struct mtop	mt;

				mt.mt_count = 1;
				mt.mt_op = MTBSF;

				if(ioctl(fileno(magtfp), MTIOCTOP, &mt) < 0)
					errorexit("cannot backskip tape", 0, 0);

				error("cannot read HDR3 from tape", 0, 0);
				}

			else
				{
#ifdef DEBUG
	fprintf(stdout,"\n_tp: DECFILE11A seen_\n"); pr_label();
#endif
				sscanf(labelbuf, "%3s%1d%4x%4x", l_labelid,
				&l_labelno, &num1, &num2);

				if(strcmp(l_labelid, "HDR") || l_labelno != 3)
					{
					errorexit("%s%d: illegal label format (HDR3)",
					l_labelid, l_labelno);
					}
				if(num1 == MAXREC4 && num2 == 2)
					l_recformat = FUF;
				}
			}

		/* Expanded version number (combines l_gen and l_genver)
		*/
		verno_exp = (l_gen-1) * 100 + l_genver + 1;
		itoa(verno_exp, verno_exp_a);

		sprintf(name, "%s%s", pathname, l_filename);

/*
   The next question says:  We have been told to position the tape
   but have not (as yet) reached the position.
*/
		if(pos == -1)
			{
			/* for additional interpretation */
			char	posname2[MAXLEN];

			posname2[0] = 0;

			if(! use_versnum)
				{
				int	length;
	
				length = strlen(posname);
				/*
				 * skip '.' at end of posname.  second dot
				 * must be 2, 3, or 4 places behind ending dot.
				 * e.g., ansi.exe. , ansi.ex. , ansi.c.
				 */
				for(i = length - 2; i >= length - 5; i--)
					{
					if(posname[i] == '.')
						{
						if(i == length - 2)
							break;
						strcpy(posname2, posname);
						posname2[length-1] = 0;
						break;
						}
					if(posname[i] < 'a' ||
					posname[i] > 'z')
						break;
					}
				}
			if(! (*cmp)(name, posname) ||
			(! use_versnum && ! (*cmp)(name, posname2)))
				{
				/* true if (! use_versnum) or
				 * if (use_versnum && mstrcmp()).
				 */
				if(! use_versnum || ! mstrcmp(verno_exp_a, posnum))
					{
					if(poscount == -1)
						pos = 1;
					else if(poscount == 1)
						{
						poscount--;
						pos = 1;
						}
					else if(poscount)
						{
						poscount--;
						skip(3);
						continue;
						}
					}
				else
					{
					skip(3);
					continue;
					}
				}
			else
				{
				skip(3);
				continue;
				}
			}

		/*
		 * numrecs equals 0 if no file arguments are specified.
		 * Thus the entire tape must be processed.
		 */
		if(! numrecs ||
		((fstat = lookup(name, verno_exp_a)) != NULL))
			/*
			The last comment seems to be saying:  Do this is
			if we are extracting ALL files; -or- The user wants
			the one we are positioned at.
			*/
			{
			if(use_versnum)
				sprintf(name, "%s%s.%d", pathname,
				l_filename, verno_exp);
			else
				/* remove '.' at end of string */
				remove_c(name);

			if(func == TABLE)
				skip(2);

			else /* Does the file have links ? */

			     if(lnk_fsecno != 0 && lnk_fseqno != 0)
				{
				int	found = 0;

				for(lp = x_head; lp != NULL; lp = lp->x_next)
					{

					/* Look thru our links.. */

					if(lp->x_fsecno == lnk_fsecno &&
					lp->x_fseqno == lnk_fseqno)
						{
						found++;
						break;
						}
					}
				if(found)
					{
					char	path[MAXLEN];
					char	*p;

					if(stat(name, &inode) >= 0) /* ck for any error rtn */
						{
						if(warning_only)
							error("%s already exists.  Overwriting and linking.", name, 0);
						else
							{
							fprintf(stderr,
							"%s: %s already exists.  Overwrite and link (y/n)? ", progname, name);
							gets(labelbuf);
							if(labelbuf[0] != 'y')
								{
								skip(3);
								continue;
								}
							}
						}
					else
						{
						strcpy(path, name);
						p = path;
						p += strlen(path);

						while(--p != path)
							{
							if(*p == '/')
								break;
							}
						if(*p == '/')
							{
							*++p = 0;
							checkdir(path);
							}
						}
					unlink(name);
					if(link(lp->x_pathname, name) < 0)
						error("cannot link %s to %s",
						lp->x_pathname, name);

					else
						{
						linkflag = YES;
						skip(2);
						}
					}
				else if(freemem)
					error("cannot link %s.  Head link was not extracted.", name, 0);
				}
			else
				{
				lp = (struct xlinkbuf *) malloc(sizeof(*lp));
				if(lp == NULL)
					{
					if(freemem)
						{
						error("Out of memory.  Link information lost", 0, 0);
						freemem = NO;
						}
					}
				else
					{
					/* Add this into the list of "link"
					   files in case we are one or are
					   pointed to by someone.
					 */
					lp->x_next = x_head;
					x_head = lp;
					lp->x_fsecno = l_fsecno;
					lp->x_fseqno = l_fseqno;
					strcpy(lp->x_pathname, name);
					}
				}
			xname[0] = 0;

	/* The following test basically says:

		Did the user want to override what we think the disk file
		file format should be ?
	*/
			if(func == EXTRACT && numrecs && fstat->f_flags)
				{
				if(fstat->f_flags & FUF && fstat->f_flags & DD)
					error("your -u request has precedence over the -d option.", 0, 0);
				if(fstat->f_flags & FUF)
					{
					if(l_recformat == VARIABLE)
						l_recformat = FUF;
					else
						error("the -u option does not apply when extracting binary files", 0, 0);
					}
				if(fstat->f_flags & DD)
					{
					if(l_recformat == VARIABLE)
						l_recformat = DD;
					else
						error("the -d option does not apply when extracting binary files", 0, 0);
					}
				}

			/* This file is not a link. */

			if(func == EXTRACT && ! linkflag)
				{
				/* Go extract a real file from the tape.
				*/
				ret = xtractf(pathname,
				! numrecs ? "" : fstat->f_src,
				charcnt, xname);
				/*
				  Did the extract go ok and is this an
				  Ultrix tape ?
				*/
				if(ret >= 0L && ! strcmp(l_systemid, SYSNAME))
				 /* /\--> a ret of -1 === an error */

					{
					char	temp[MAXLEN];
					/*
					The following says: Did the user give
					us a name to use for this file on the
					disk, or should we use the name of
					the file as we interpreted it from
					the tape ?
					*/
					strcpy(temp, xname[0] != 0 ? xname : name);

					chmod(temp, mode);
					chown(temp, uid);
					/* no chgrp routine
					chgrp(temp, gid);
					*/
					if(modtime > 0L)
						{
						time_t	timep[2];

						timep[0] = time(NULL);
						timep[1] = modtime;
						utime(temp, timep);
						}
					}
				/*
				Ask the question: If this is "NOT" a
				-link- file ?
				*/
				if((lnk_fsecno == 0 || lnk_fseqno == 0)
				&& lp != NULL)
					{

				  /* If error on extract & we have something
				     in our extract list - then do the
				     following.
				  */
					if(ret < 0L)
						{
						/* Remove this file name from
						 our list of extracted files.
						 (used for links) */

						x_head = lp->x_next;
						free((char*)lp);
						continue;
						}
					else
					     /* Else, if the user specified an
					     extract name, put that name in our
					     list instead of the name it has on tape.
					     */
					     if(xname[0] != 0)	 strcpy(lp->x_pathname, xname);
					}
				/* stop processing any files with
				 * ret < 0L that haven't been caught
				 * before this.  e.g., non-head link
				 * files that were not extracted.
				 */
				if(ret < 0L)
					continue;
				}

			if(read(fileno(magtfp), labelbuf, LABSIZE) < 0)
				errorexit("cannot read EOF1 from tape", 0, 0);

			sscanf(labelbuf, "%3s%1d%*50c%6ld", l_labelid,
			&l_labelno, &l_nblocks);
#ifdef DEBUG
	pr_label();
#endif

			if(strcmp(l_labelid, "EOF") || l_labelno != 1)
				{
				errorexit("%s%d: illegal label format (EOF1)",
				l_labelid, l_labelno);
				}

			/* The following constant of 1 will fail when we implement
			   all of  LTF	functionality. */
			/* skip rest of EOF label set */
			skip(1);

			if(func == TABLE)
				{
				if(verbose)
					{
					sprintf(cat_misc, "(%d,%d)", l_fseqno, l_fsecno);
					printf("%-7s", cat_misc);
					if(strcmp(l_systemid, SYSNAME))
						printf("---------   -/-   ");
					else
						{
						expand_mode(mode);
						printf("%4d/%-4d", uid, gid);
						}
					if(modtime > 0L)
						date_time(sdate, &modtime);
					else
						date_year(sdate, l_credate);
					printf("%12s", sdate);
					sprintf(cat_misc, "%ld(%d)%c", l_nblocks,
					l_blklen, l_recformat == 'D' ? 't' : 'b');
					printf("%11s", cat_misc);
					if(strlen(name) > 30)
						printf("\n       %s", name);
					else
						printf(" %s", name);
					}
				else
					printf("%s", name);

				if(lnk_fsecno != 0 && lnk_fseqno != 0)
					printf("%slinked to %s\n",
					verbose ? "\n       " : " ", lnk_name);
				else
					printf("\n");
				}
			else if(verbose)
				{
				if(linkflag)
					printf("x %s linked to %s\n", name,
					lp->x_pathname);
				else
					{
					printf("x %s,  %ld byte%c, %ld %d-byte tape block%c",
					name, ret, ret == 1 ? 0 : 's',
					l_nblocks, l_blklen,
					l_nblocks == 1L ? 0 : 's');

					if(l_recformat == FUF)
						printf(" (fuf)\n");
					else if(l_recformat == DD)
						printf(" (dd)\n");
					else
						printf("\n");

					if(xname[0] != 0)
						printf("  > %s\n", xname);



if ((func == EXTRACT)) {
	char *sp;
	int wildc;

	wildc = NO;
	sp = fstat->f_dest;

	while (*sp != '\n') {
		if ((*sp == '*') || (*sp == '?')) {
			wildc = YES;
			break;
		}
		sp++;
	}

	if ((!wildc) && (numrecs)) {
		strcpy(fstat->f_src,"1extracted1");
		strcpy(fstat->f_dest,"1extracted1");
		fstat->f_numleft =0;
		numrecs--;
		if (!numrecs) exit(0);
	}
}
					}
				}
			}
		else
			skip(3);
		}/*E for ;;..*/
	}
/*end SCANTAPE*/

/*---------------------------------------------------------------------
 *
 *  LOOKUP  -  looks up the given LTF entry among user-input
 *	       file arguments.
 *
 *-------------------------------------------------------------------*/
struct filestat *
lookup(name, gen)
char	*name;			/* file name (and file type) */
char	*gen;			/* version number */
	{
	struct	filestat *fstat;
	char	*d, *n;
	char	dest2[MAXLEN];		 /* for additional interpretation */

	for(fstat = f_head; fstat != NULL; fstat = fstat->f_next)
		{
		if(! fstat->f_numleft)
			continue;
		dest2[0] = 0;

		if(! use_versnum)
			{
			int	i, length;

			length = strlen(fstat->f_dest);
			/*
			 * skip '.' at end of dest.  second dot
			 * must be 2, 3, or 4 places behind ending dot.
			 * e.g., ansi.exe. , ansi.ex. , ansi.c.
			 */
			for(i = length - 2; i >= length - 5; i--)
				{
				if(fstat->f_dest[i] == '.')
					{
					if(i == length - 2)
						break;
					strcpy(dest2, fstat->f_dest);
					dest2[length-1] = 0;
					break;
					}
				}
			}
		if(! (*cmp)(name, fstat->f_dest) ||
			(! use_versnum && ! (*cmp)(name, dest2)))
			{
			if(! use_versnum || ! mstrcmp(gen, fstat->f_version))
				{
				if(fstat->f_numleft == -1)
					return(fstat);
				if(fstat->f_numleft == 1)
					{
					fstat->f_numleft--;
					return(fstat);
					}
				/* decrement if f_numleft > 1 */
				if(fstat->f_numleft)
					fstat->f_numleft--;
				return(NULL);
				}
			}

		/*
		 * check if the LTF entry is a file
		 * (recursively) under a requested directory.
		 */
		n = name;
		d = fstat->f_dest;

		while(*n == *d)
			{
			n++;
			d++;
			}
		if(*d == '.' && *(d+1) == 0 && (*(d-1) == '/' || *n == '/'))
			return(fstat);
		}
	return(NULL);
	}

/*---------------------------------------------------------------------
 *
 *  SKIP  -  skips 'nwanted' number of tape files.  It returns the
 *	     number of files actually skipped.
 *
 *-------------------------------------------------------------------*/
skip(nwanted)
int	nwanted;
	{
	int	ndone;
	struct	mtop mt;
	int	ret;
	char	labelid[5];
	int	eofok = NO;		/*
					 * determines when an eof
					 * indicates an eot.  A
					 * zero-length LTF file
					 * will cause a premature eot
					 * unless we use 'eofok'.
					 */

	mt.mt_count = 1;
	mt.mt_op = MTFSF;

	for(ndone=1; ndone != nwanted; ndone++)
		{
		if(ioctl(fileno(magtfp), MTIOCTOP, &mt) < 0)
			errorexit("cannot skip tape", 0, 0);

		inbuf[0] = 0;
		ret = read(fileno(magtfp), inbuf, sizeof(inbuf));
		if(ret < 0)
			{
			error("cannot read tape.", 0, 0);
			if(oflag)
				error("Incorrect tape density???", 0, 0);
			return(ndone);
			}
		else
			inbuf[ret] = 0;

		/* eofok is always equal to NO with the -O option */
		if(! oflag && ret)
			{
			labelid[0] = 0;
			sscanf(inbuf, "%4s", labelid);
	
			if(! strcmp(labelid, "EOF1"))
				eofok = YES;
			else if(! strcmp(labelid, "HDR1"))
				eofok = NO;
			}

		if(! ret && ! eofok)
			{
			if(ndone + 1 == nwanted)
				return(++ndone);
			else
				{
				/*
				 * set eofok equal to YES because EOF1
				 * label will be skipped over by ioctl.
				 * i.e., assume next label is EOF.
				 */
				if(! oflag)
					eofok = YES;
				ndone++;
				}
			}
		else if(! ret && eofok)
			{
			/*
			 * if a second eof is found (eot)
			 * back up two file and stop.
			 */
			mt.mt_count = 2;
			mt.mt_op = MTBSF;
			if(ioctl(fileno(magtfp), MTIOCTOP, &mt) < 0)
				errorexit("cannot backskip tape", 0, 0);
			break;
			}
		}
	mt.mt_count = 1;
	mt.mt_op = MTFSF;
	if(ioctl(fileno(magtfp), MTIOCTOP, &mt) < 0)
		errorexit("cannot skip tape", 0, 0);
	return(ndone);
	}

/*---------------------------------------------------------------------
 *
 *  XTRACTF  -  extracts next LTF file to disk.  It returns the
 *		the character count of the extracted file or -1 if
 *		error.  Xtractf must return -1 upon error because a
 *		zero-length file will have a return value of 0.
 *
 *-------------------------------------------------------------------*/
long
xtractf(path, s, charcnt, name)
char	*path;				/* pathname only */
char	*s;				/* file name */
long	charcnt;			/* character count (if applicable) */
char	*name;				/* extracted filename (if different) */
	{
	FILE	*xfp;
	char	xname[MAXLEN];
	int	nbytes, count;
	long	num = 0L;
	char	*p;

	if(*s == 0)
		{
/*
	We are not sure if versions of a file come off the tape correctly
	as of yet...rjg
*/
		if(use_versnum)
			sprintf(xname, "%s%s.%d", path, l_filename,
			(l_gen-1) * 100 + l_genver+1);
		else
			{
			sprintf(xname, "%s%s", path, l_filename);

			/* remove '.' at end of string */
			remove_c(xname);
			}
		}
	else
		{
		strcpy(name, s);
		strcpy(xname, s);
		}

	for(p = xname, count = 0; xname[count]; count++)
		{
		if(xname[count] == '/')
			p = &xname[count];
		}

	if(p != xname)
		p++;

	if(stat(xname, &inode) >= 0)
		{
		if(warning_only)
			;
#if 0
	Ray Glaser and Tom Tresvik decided that since no other unix 
	utility warns you (which may be alarming to field service)
	about overwriting files, then  'rdt' should not either.
	It may be a wrong descision, but it's ours to make and
	this is our choice.

			error("%s already exists.  Overwriting.", xname, 0);
#endif 0
		else
			{
			fprintf(stderr,
			"%s: %s already exists.  Overwrite (y/n)? ", progname, xname);
			gets(labelbuf);
			if(labelbuf[0] != 'y')
				{
				char	dummy[256];

				fprintf(stderr,
				"Alternative pathname ('return' to quit processing this file)? ");
				gets(dummy);
				if(dummy[0] == 0)
					{
					skip(3);
					return((long)(-1));
					}
				else
					{
					if(strlen(dummy) > MAXLEN - 1)
						{
						error("%s: file name too long", dummy, 0);
						skip(3);
						return((long)(-1));
						}
					else
						{
						strcpy(xname, dummy);
						strcpy(name, xname);
						}
					}
				}
			}
		}

	/* Open the destination file on the disk.
	*/
	if((xfp = fopen(xname, "w")) == NULL)
		{
		checkdir(path);
		if((xfp = fopen(xname, "w")) == NULL)
			{
			error("cannot create %s", xname, 0);
			skip(3);
			return((long)(-1));
			}
		}
	/* Skip over the tape mark between the last  HDRn & the start of
	   real file data.
	*/
	skip(1);
	while((nbytes = read(fileno(magtfp), p=inbuf, l_blklen)) > 0)
		{
		if(l_recformat == FIXED)
			{
			/*
			 * test if charcnt == 0L because some old tapes
			 * with SYSNAME won't have a character count.
			 * Foreign tapes won't have a charcnt either.
			 * CHARCNT comes from  HDR4 in orginal LTF.
			 */
			if(charcnt == 0L)
				{
				char	*pp;

				pp = p;
				pp += nbytes;
				while(*--pp == PAD);
				pp++;
				num += (long)(pp - p);
				if(write(fileno(xfp), p, pp - p) < 0)
					errorexit("cannot write on filesystem", 0, 0);
				}
			else
				/* If charcnt > nbytes, we read less than we should
				   have according to HDR4 (in org LTF).
				*/
				 if(charcnt >= (long)nbytes)
				{
				/* We get here if more bytes were read in than
				   HDR4 says we should have (in org LTF).
				*/
				if(write(fileno(xfp), inbuf, nbytes) < 0)
					errorexit("cannot write on filesystem", 0, 0);
				charcnt -= (long)nbytes;
				num += (long)nbytes;
				}
			else if(charcnt > 0L)
				{
				if(write(fileno(xfp), inbuf, (int)charcnt) < 0)
					errorexit("cannot write on filesystem", 0, 0);
				num += charcnt;
				/* Set charcnt equal to -1
				   so that we don't try to get any more of
				   this apparent garbage from the tape. */
				charcnt = (long)(-1);
				}
			}
		else
			{
			while(p < &inbuf[nbytes] &&
			(count = getlen(p)) >= 0)
				{
				if(l_recformat == VARIABLE)
					{
					p += 4; /* What are we skipping ? */
					if(count == 0)
						{
						putc('\n', xfp); /* Why are we
								 tacking this on ?*/

						fflush(xfp);
						num++;
						continue;
						}
					}
				else if(l_recformat == FUF)
					{
					p += 4;
					if(! fufcnv(p, count, xfp))
						{
						error("fuf record too long.  %s not dumped", xname, 0);
						skip(3);
						return((long)(-1));
						}
					p += count;
					continue;
					}
				else
					/* l_recformat == DD */
					count += 4;
				if(write(fileno(xfp), p, count) < 0)
					errorexit("cannot write on filesystem", 0, 0);
				p += count;
				num += (long)count;
				if(l_recformat == VARIABLE)
					{
					putc('\n', xfp);
					fflush(xfp);
					num++;
					}
				}
			}
		}
	fclose(xfp);

	return(num); /* Go do next, if any, file.*/
	}
/*end XTRACTF*/

/*---------------------------------------------------------------------
 *
 *  GETLEN  -  returns the length of the next variable-length record
 *		on an extract from tape.
 *
 *-------------------------------------------------------------------*/
getlen(s)
char	*s;
	{
	int val, i;
	if(*s == PAD)
		return(-1);
	val = 0;
	for(i=0; i < 4; i++)
		val = 10 * val + (*s++ - '0');
	return(val-4);
	}

static int days[2][13] =
	{
	{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	};
static char *months[] =
	{ 0, "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec" };

/*---------------------------------------------------------------------
 *
 *  DATE_YEAR  -  fills the string 'sdate' with the appropriate
 *		  month, day, and year.
 *
 *-------------------------------------------------------------------*/
date_year(sdate, date)
char	*sdate, *date;
	{
	int	leap, i, year, yearday;

	sscanf(date, "%2d%3d", &year, &yearday);
	i = 1900 + year;
	leap = i%4 == 0 && i%100 != 0 || i%400 == 0;

	for(i= 0; yearday > days[leap][i]; i++)
		yearday -= days[leap][i];

	sprintf(sdate, "%s %2d  %4d", months[i], yearday, year + 1900);
	}

/*---------------------------------------------------------------------
 *
 *  DATE_TIME  -  fills the string 'sdate' with the appropriate
 *		  month, day, and modification time.
 *
 *-------------------------------------------------------------------*/
date_time(sdate, sec)
char	*sdate;
long	*sec;
	{
	int	thisyear;
	long	rettime;
	struct	tm *localtime(), *ltime;

	rettime = time(NULL);
	ltime = localtime(&rettime);
	thisyear = ltime->tm_year;

	ltime = localtime(sec);

	if(ltime->tm_year == thisyear)
		sprintf(sdate, "%s %2d %02d:%02d", months[ltime->tm_mon + 1],
		ltime->tm_mday, ltime->tm_hour, ltime->tm_min);
	else
		sprintf(sdate, "%s %2d  %4d", months[ltime->tm_mon + 1],
		ltime->tm_mday, ltime->tm_year + 1900);
	}

/*---------------------------------------------------------------------
 *
 *  CREATETP  -  initializes the tape and calls 'proc_args' to
 *		 process the file arguments.
 *
 *-------------------------------------------------------------------*/
createtp(num, args, iflag, inputfile)
int	num;			/* number of arguments to process */
char	*args[];		/* array of pointers to arguments */
int	iflag;			/* -I option */
char	*inputfile;
	{
	char	line[512];		/* line from passwd file */
	char	owner[14];		/* login name of owner */
	int	j;

	/*
	 * NOTE:  createtp does not check whether you are overwriting
	 * something on tape.
	 */
	if((magtfp = fopen(magtdev, "w")) == NULL)
		errorexit("cannot write on tape.  Offline or needs write ring???", 0, 0);

	/* Attempt to get the user's login name from the password file.
	   NOTE: this fails... You always end up with the last name in
	  the /etc/passwd file.
	*/
	getpw(getuid() & 0377, line);
	for(j=0; j < 14 && line[j] && line[j] != ':'; j++)
		owner[j] = line[j];
	if(line[j] == ':')
		owner[j] = 0;
	upper(owner);

/*	Write the VOLUME label  (VOL1) .. See  VMS note 2-6 of
	VMS  MTAACP  documentation for important information.
*/
	sprintf(labelbuf, "VOL1%-6.6s %26.26s%-14.14s%28.28s3",
	label, spaces, owner, spaces);
	write(fileno(magtfp), labelbuf, LABSIZE);

	fsecno = fseqno = 1;
	proc_args(num, args, iflag, inputfile);
	}
/*end CREATETP*/

/*---------------------------------------------------------------------
 *
 *  PROCESS  -  processes the given file and its label sets.
 *		?? Which means ?
 *-------------------------------------------------------------------*/

process(longname, shortname, type)
char	*longname;		/* contains the pathname and file name */
char	*shortname;		/* file name only */
int	type;			/* type of file */
	{
	/* dummy is a buffer for sprintf and is also a copy of longname */
	static	char dummy[MAXLEN];
	static	char ldummy[MAXLEN*2];
	FILE	*fp;
	struct	filestat fstat;
	struct	alinkbuf *lp;
	int	linkflag = NO;		/*
					 * YES if the file is linked to
					 * a file that has already been
					 * appended.
					 */
	int	found = 0;		/* a head link is not on tape yet */
	int	version = 1;		/* version number (1 is default) */
	int	length;			/* length of a line of text file */
	long	append();
	long	charcnt = 0L;		/*
					 * character count.  This is
					 * just a precaution in case
					 * the last character of a
					 * binary file is the same as
					 * the padding character.
					 */
	int	max;			/* maximum line length */
	char	pathname[77];
	char	*l, *l2;
	char	line[512];		/* buffer for reading text files */
	struct	tm *localtime();
	struct	tm *ltime;
	register int j;

	pathname[0] = 0;

	l = dummy;
	l2 = longname;
	for(j=0; (*l++ = *l2++) && j < MAXLEN; j++); /* Copy longname into dummy */

	if(j >= MAXLEN)
		{
		error("%s: file name too long", longname, 0);
		return;
		}
/*	Put a good string copy of the longname in filestat buffer.
*/
	strcpy(fstat.f_src, dummy);

/*	Now, separate the base file name from the pathname+filename.
*/
	l = dummy;
	l += strlen(dummy);

	while(--l != dummy)
		if(*l == '/')
			break;
	if(*l == '/')
		{
		char	*s1, *s2;
		s1 = pathname;
		s2 = dummy;
		*l = 0;

		/* The following will eventually hit the trailing zero and
		   stop...
		*/
		for(j=0; (*s1++ = *s2++) && j < 76; j++);

		if(j >= 76)
			{
			error("%s: pathname too long", longname, 0);
			return;
			}
		strcat(pathname, "/");
		if(! make_num(++l, fstat.f_dest, fstat.f_version))
			return;
		}
	else
		/* We come here when there is apparantly only a filename.
			i.e. Not /path/name  form..
		*/
		{
		if(! make_num(l, fstat.f_dest, fstat.f_version))
			return;
		}

	if(strlen(fstat.f_dest) > 17)
		{
		error("%s: destination file name too long", longname, 0);
		return;
		}
	/*
	 * Must open shortname (rather than longname)
	 * because we might be in a subdirectory.      WHY ???
	 */
	if((fp = fopen(shortname, "r")) == NULL)
		{
		error("cannot open %s", fstat.f_src, 0);
		return;
		}
	if(stat(shortname, &inode) < 0)
		{
		error("cannot stat %s", fstat.f_src, 0);
		return;
		}

	ltime = localtime(&inode.st_mtime);

	block = 0L;
	max = 0;

	if(type == TEXT)
		{
		/* find length of longest line */
		while(fgets(line, 512, fp) != NULL)
			{
			length = strlen(line);
			if(line[length-1] == '\n')
				length--;
			if(length > max)
				max = length;
			if(max + 4 > reclength)
				break;
			}
		if(max + 4 > reclength)
			{
			error("line too long", 0, 0);
			if(warning_only)
				{
				if(reclength == MAXRECSIZE)
					{
					/* Would be nice to say why */
					error("cannot append as a text file.  %s not dumped.",
					fstat.f_src, 0);
					error("Try appending as a BINARY file (with the -b flag).", 0, 0);
					}
				else
					error("record length too small.  %s not dumped.",
					fstat.f_src, 0);
				return;
				}
			if(type == TEXT && reclength == MAXRECSIZE)
				{
				error("cannot append as a text file.  %s not dumped.",
				fstat.f_src, 0);
				error("Try appending as a BINARY file (with the -b flag).", 0, 0);
				return;
				}

			/*
			 * Give the user a chance to change some
			 * parameters before appending the file.  BUT WHY ???
			 */
			fprintf(stderr, "%s: Is %s a TEXT or a BINARY file (t/b)? ", progname, fstat.f_src);
			gets(line);
			if(line[0] == 't')
				type = TEXT;
			else if(line[0] == 'b')
				type = BINARY;
			else if(line[0] == 'd')
				type = COUNTED;
			else
				{
				error("input must be 't', 'b' or 'd'.  %s not dumped.",
				fstat.f_src, 0);
				return;
				}
			if(type == TEXT)
				{
					/* WHY ? TO WHAT ?  Is there no default
					size we can suggest ?  If we know to
					what size, why ASK USER ?
					*/
				error("You MUST now increase the record length.", 0, 0);
				fprintf(stderr, "%s: New record length (current value is %d)? ", progname, reclength);
				gets(line);
				sscanf(line, "%d", &reclength);
				if(max + 4 > reclength)
					{
					if(reclength > MAXRECSIZE ||
					line[0] <= '0' ||
					line[0] > '9')
						error("invalid record length (max 512).", 0, 0);
					else if(reclength == MAXRECSIZE)
						{
						error("cannot append as a text file.  %s not dumped.", fstat.f_src, 0);
						error("Try appending as a BINARY file (with the -b flag).", 0, 0);
						}
					else
						error("record length is still too small.  %s not dumped.", fstat.f_src, 0);
					return;
					}
				}
			}
		}
	else
		charcnt = inode.st_size;

	fseek(fp, 0L, 0);

	sscanf(fstat.f_version, "%d", &version);

/*
	 PUT A FILE ON TAPE HERE....
*/
	/* HDR1 - Identify the file and the system that created the tape.
	 */
sprintf(dummy,
	"HDR1%-17.17s%-6.6s%04d%04d%04d%02d %02d%03d %02d%03d %06d%-13.13s%7.7s",
	fstat.f_dest, label, fsecno, fseqno,
	(version-1) / 100 + 1, (version-1) % 100,
	ltime->tm_year, ltime->tm_yday+1, 99, 366, 0,
	type == FUF ? "DECFILE11A" : SYSNAME, spaces);
	write(fileno(magtfp), dummy, LABSIZE);

	if(type == FUF)
		max = MAXREC4;

/*	HDR2 - describe the record format, maximum record size, & maximum
	block length of the file..
*/
	if(type != TEXT) /* Tell VMS NOT to give carriage return attributes */
		sprintf(dummy,
			 "HDR2%c%05d%05df%20.20sM%13.13s00%28.28s",
		(type == TEXT || type == FUF || type == COUNTED) ? VARIABLE : FIXED, blocksize,
		(type == TEXT || type == FUF) ? max + 4 : reclength, spaces, spaces,spaces);

	else /* Tell VMS it is ok to give carriage return attributes */

			sprintf(dummy,
				 "HDR2%c%05d%05df%34.34s00%28.28s",
		(type == TEXT || type == FUF || type == COUNTED) ? VARIABLE : FIXED, blocksize,
		(type == TEXT || type == FUF) ? max + 4 : reclength, spaces, spaces,spaces);

	write(fileno(magtfp), dummy, LABSIZE);

/*	HDR3 - Stores the VAX/VMS  RMS file attributes.
		For Ultrix, we put out path component here (org vsn).
*/
	if(type == FUF)
		{
		sprintf(ldummy,
			 "HDR3%04x0002%010d01%044d%12.12s",
		MAXREC4, 0, 0, spaces);
/*		sprintf(&ldummy[81],"_-_prototype test_-_'\n'");*/
		write(fileno(magtfp), ldummy, LABSIZE);
		}

	else
		{
		upper(pathname);
		sprintf(ldummy,
			 "HDR3%-76.76s", pathname);
/*		sprintf(&ldummy[81],"_-_prototype test_-_'\n'");*/
		write(fileno(magtfp), ldummy, LABSIZE);
	
	
		if(inode.st_nlink > 1)
			{
			for(lp = a_head; lp != NULL; lp = lp->a_next)
				if(lp->a_inum == inode.st_ino &&
				lp->a_dev == inode.st_dev)
					{
					found++;
					break;
					}
			if(found)
				{
				/* HDR4 - A hard link was found,
				head seen,  point this file  to it.
				*/
	sprintf(ldummy,
			 "HDR4%04o%04d%04d%04d%04d%010ld%010ld%39.39s",
				inode.st_mode & 07777, inode.st_uid, inode.st_gid,
				lp->a_fsecno, lp->a_fseqno, charcnt,
				inode.st_mtime, spaces);
/*			sprintf(&ldummy[81],"_-_prototype test_-_'\n'");*/
				write(fileno(magtfp), ldummy, LABSIZE);


				/* HDR5 - the head file name
				*/
				sprintf(ldummy,
					 "HDR5%-76.76s", lp->a_pathname);
/*			sprintf(&ldummy[81],"_-_prototype test_-_'\n'");*/
				write(fileno(magtfp), ldummy, LABSIZE);
	
				linkflag = YES;
				lp->a_count--;
				}
			else
				{
				/* Hard link, head not seen, this must be
				considered the head link ?
				*/
				sprintf(ldummy,
					 "HDR4%04o%04d%04d%04d%04d%010ld%010ld%39.39s",
				inode.st_mode & 07777, inode.st_uid, inode.st_gid,
				0, 0, charcnt, inode.st_mtime, spaces);
/*			sprintf(&ldummy[81],"_-_prototype test_-_'\n'");*/
				write(fileno(magtfp), ldummy, LABSIZE);
	
				lp = (struct alinkbuf *) malloc(sizeof(*lp));
				if(lp == NULL)
					{
					if(freemem)
						{
						error("Out of memory.  Link information lost.", 0, 0);
						freemem = NO;
						}
					}
				else
					/* Not a hard link seen.
					*/
					{
					lp->a_next = a_head;
					a_head = lp;
					lp->a_inum = inode.st_ino;
					lp->a_dev = inode.st_dev;
					lp->a_count = inode.st_nlink - 1;
					lp->a_fsecno = fsecno;
					lp->a_fseqno = fseqno;
					strcpy(lp->a_pathname, longname);
					}
				}
			}
		else
			{
			sprintf(ldummy, "HDR4%04o%04d%04d%04d%04d%010ld%010ld%39.39s",
			inode.st_mode & 07777, inode.st_uid, inode.st_gid,
			0, 0, charcnt, inode.st_mtime, spaces);
/*			sprintf(&ldummy[81],"_-_prototype test_-_'\n'");*/
			write(fileno(magtfp), ldummy, LABSIZE);
			}
		}

	/* write an end-of-file mark on tape */
	weof();


/*
	The label headers have been done, now write the file itself
	on to the tape.
*/

	if((block = append(fp, type, max)) < 0L)
		{
		if(block == (long)(-1))
			error("%s not dumped.", fstat.f_src, 0);
		else
			{
			error("content type in question. %s not dumped.", fstat.f_src, 0);
			error("Try appending as a BINARY file (with the -b flag).", 0, 0);
			}

		fclose(fp);

		if(fseqno == 1 && fsecno == 1)
			{
			struct mtop	mt;

			fclose(magtfp);
			rew();
			if((magtfp = fopen(magtdev, "w")) == NULL)
				errorexit("cannot open %s for writing", magtdev, 0);
			mt.mt_count = 1;
			mt.mt_op = MTFSR;

			if(ioctl(fileno(magtfp), MTIOCTOP, &mt) < 0)
				errorexit("cannot skip 1 tape record", 0, 0);

			}
		else
			back(1);

		if(! found && lp != NULL)
			{
			a_head = lp->a_next;
			free((char *) lp);
			}

		return;
		}

	fclose(fp);

/*
	Headers and file data done, now put the trailer lables on the
	tape and move on to the next file.
*/
	weof();

	sprintf(dummy,
	"EOF1%-17.17s%-6.6s%04d%04d%04d%02d %02d%03d %02d%03d %06ld%-13.13s%7.7s",
	fstat.f_dest, label, fsecno, fseqno,
	(version-1) / 100 + 1, (version-1) % 100,
	ltime->tm_year, ltime->tm_yday+1, 99, 365, block,
	type == FUF ? "DECFILE11A" : SYSNAME, spaces);
	write(fileno(magtfp), dummy, LABSIZE);

	sprintf(dummy, "EOF2%c%05d%05d%35.35s00%28.28s",
	(type == TEXT || type == FUF) ? VARIABLE : FIXED, blocksize,
	(type == TEXT || type == FUF) ? max + 4 : reclength,
	spaces, spaces);
	write(fileno(magtfp), dummy, LABSIZE);

	if(type == FUF)
		{
		sprintf(dummy, "EOF3%04x0002%010d01%044d%12.12s",
		MAXREC4, 0, 0, spaces);
		write(fileno(magtfp), dummy, LABSIZE);
		}

	else
		{
		sprintf(dummy, "EOF3%-76.76s", pathname);
		write(fileno(magtfp), dummy, LABSIZE);
	
		if(found)
			{
			sprintf(dummy, "EOF4%04o%04d%04d%04d%04d%010ld%010ld%39.39s",
			inode.st_mode & 07777, inode.st_uid, inode.st_gid,
			lp->a_fsecno, lp->a_fseqno, charcnt,
			inode.st_mtime, spaces);
			write(fileno(magtfp), dummy, LABSIZE);

			sprintf(dummy, "EOF5%-76.76s", lp->a_pathname);
			write(fileno(magtfp), dummy, LABSIZE);
			}
		else
			{
			sprintf(dummy, "EOF4%04o%04d%04d%04d%04d%010ld%010ld%39.39s",
			inode.st_mode & 07777, inode.st_uid, inode.st_gid,
			0, 0, charcnt, inode.st_mtime, spaces);
			write(fileno(magtfp), dummy, LABSIZE);
			}
		}

	weof();

	if(verbose)
		{
		lower(fstat.f_dest);
		lower(pathname);
		if(use_versnum)
			printf("a %s%s.%d", pathname, fstat.f_dest, version);
		else
			{
			remove_c(fstat.f_dest);
			printf("a %s%s", pathname, fstat.f_dest);
			}
		if(linkflag)
			printf(" linked to %s", lp->a_pathname);
		else
			printf(",  %ld tape block%c", block,
			block == 1L ? 0 : 's');
		printf("\n");
		}

	fseqno++;
	}

/*---------------------------------------------------------------------
 *
 *  APPEND  -  appends the given file onto tape in the appropriate
 *	       record and block format.  It returns the number of
 * 	       blocks appended.
 *
 *-------------------------------------------------------------------*/
long
append(fp, type, max)
FILE	*fp;
int	type, max;
	{
	char	line[512];
	int	length;
	char	*p;

	block = 0L;
	p = inbuf;

	if(type == TEXT)
		{
		while(fgets(line, reclength, fp) != NULL)
			{
			length = strlen(line);
			if(line[length-1] == '\n')
				line[--length] = 0;
			else
				return((long)(-2));
/*
			if(length == 0)
				{
				length = 2;
				line[0] = line[1] = ' ';
				}
*/

			if(length > max)
				{
				error("file changed size.", 0, 0);
				return((long)(-1));
				}

			if(&p[length+4] > &inbuf[blocksize])
				{
				while(p < &inbuf[blocksize])
					*p++ = PAD;
				write(fileno(magtfp), inbuf, blocksize);
				p = inbuf;
				block++;
				}
			sprintf(p, "%04d%s", length+4, line);
			p = &p[length+4];
			}
		if(p != inbuf)
			{
			while(p < &inbuf[blocksize])
				*p++ = PAD;
			write(fileno(magtfp), inbuf, blocksize);
			block++;
			}
		}
	else if(type == BINARY)
		while((length = read(fileno(fp), p=inbuf, blocksize)) > 0)
			{
			if(length < blocksize)
				{
				p = &p[length];
				while(p < &inbuf[blocksize])
					*p++ = PAD;
				}
			write(fileno(magtfp), inbuf, blocksize);
			p = inbuf;
			block++;
			}
	else if (type == COUNTED) /* Variable length records */
		{
		while( fread( &length, sizeof (short), 1, fp))
			{
			if( length == -1 || p+length+4 > &inbuf[blocksize])
				{
				while( p < &inbuf[blocksize])
					*p++ = PAD;
				write( fileno( magtfp), inbuf, blocksize);
				p = inbuf;
				block++;
				}
			sprintf( p, "%04d", length+4);
			fread( p+4, 2, (length+1)/2, fp);
				/* fudge for COUNTED records always beginning
				   on a word boundary in file */
			p += length+4;
			}
		
		if(p != inbuf)
			{
			while(p < &inbuf[blocksize])
				*p++ = PAD;
			write(fileno(magtfp), inbuf, blocksize);
			block++;
			}
		}
	else	/* type == FUF */
		{
		char	rectype;
		char	*rp;
		long	nbytes, nbytesl, bytcnt;
		int	nrec, res, irec, lastbit, resl;

		/* initialize buffer pointers */
		bb = inbuf;
		rb = bb;
		rp = rb + RECOFF;

		/*
		 * Loop over all records in the file.  The unformatted
		 * format for f77 is as follows: each record is
		 * followed and preceded by a long int which contains
		 * the byte count of the record excluding the 2 long
		 * integers.
		 */
		while((res = read(fileno(fp), &nbytes, sizeof(long))) > 0)
			{
			if(nbytes > 0L)
				{
				bytcnt = nbytes;
				nrec = ((int)nbytes-1) / MAXREC6;
				irec = nrec;

				/*
				 * if record is greater than MAXREC6,
				 * then output it in chunks of MAXREC6
				 * first.
				 */
				 while(irec--)
					{
					if(rb - bb + MAXRECFUF > blocksize)
						{
						bflush();
						rb = bb;
						rp = rb + RECOFF;
						}
					/* read record in chunks of MAXREC6 */
					res = read(fileno(fp), rp, MAXREC6);
					if(res < 0)
						{
						error("eof in middle of file", 0, 0);
						return((long)(-1));
						}
					if(res != MAXREC6)
						{
						error("wrong record length in middle of file", 0, 0);
						return((long)(-1));
						}

					/*
					 * Set up the record type.  Note:
					 * to get here the record must
					 * be > MAXREC6 and it must be
					 * split.  Thus there must be a
					 * FIRST record and all other
					 * records must be MIDDLE records.
					 * There could be a LAST record
					 * if the record was an exact
					 * multiple of MAXREC6.
					 */
					 if(irec == nrec - 1)
						rectype = FIRST;
					else if(bytcnt > MAXREC6)
						rectype = MIDDLE;
					else
						rectype = LAST;

					/*
					 * now output chunk of record
					 * on tape as VARIABLE format
					 */
					resl = res + RECOFF;

					sprintf(rb, "%04d", resl);
					addrtyp(rb, &rectype);

					bytcnt -= res;
					rb += resl;
					rp = rb + RECOFF;
					}
				/*
				 * output final part (unless exact
				 * multiple of MAXREC6) of record or
				 * all of it if record is < MAXREC6.
				 */
				 if(bytcnt > 0)
					{
					lastbit = bytcnt;

					if(rb - bb + lastbit + RECOFF > blocksize)
						{
						bflush();
						rb = bb;
						rp = rb + RECOFF;
						}

					res = read(fileno(fp), rp, lastbit);
					if(res < 0)
						{
						error("eof in middle of file", 0, 0);
						return((long)(-1));
						}
					if(res != lastbit)
						{
						error("wrong record length in middle of file", 0, 0);
						return((long)(-1));
						}

					if(lastbit == nbytes)
						rectype = ALL;
					else
						rectype = LAST;


					resl = res + RECOFF;

					sprintf(rb, "%04d", resl);
					addrtyp(rb, &rectype);

					rb += resl;
					rp = rb + RECOFF;
					}
				}
			else
				{
				if(rb - bb + RECOFF > blocksize)
					{
					bflush();
					rb = bb;
					rp = rb + RECOFF;
					}

				sprintf(rb, "%04d", RECOFF);
				rectype = ALL;
				addrtyp(rb, &rectype);

				rb += RECOFF;
				rp = rb + RECOFF;
				}
			res = read(fileno(fp), &nbytesl, sizeof(long));
			if(res < 0)
				{
				error("eof in middle of file", 0, 0);
				return((long)(-1));
				}
			if(nbytesl != nbytes)
				{
				error("beginning and final fuf record count are not equal", 0, 0);
				return((long)(-1));
				}
			}
		bflush();
		}


	return(block);
	}

/*---------------------------------------------------------------------
 *
 *  ADDRTYP  -  processes FUF record types.
 *
 *-------------------------------------------------------------------*/
addrtyp(inch, typ)
char	*inch;
char	*typ;
	{
	inch[4] = typ[0];
	inch[5] = typ[1];
	}


/*---------------------------------------------------------------------
 *
 *  BFLUSH  -  flushes FUF buffer.
 *
 *-------------------------------------------------------------------*/
bflush()
	{
	if(rb - bb > 0)
		{
		while(rb < bb + blocksize)
			*rb++ = PAD;
		write(fileno(magtfp), inbuf, blocksize);
		block++;
		}
	}

/*---------------------------------------------------------------------
 *
 *  WRITETP  -  positions the tape after the last LTF file
 *		(if !oflag) and gets the next available file section
 *		number and file sequence number.
 *
 *-------------------------------------------------------------------*/
writetp(num, args, iflag, inputfile)
int	num, iflag;
char	*args[];
char	*inputfile;
	{
	int	ret;
	int	nskipped = 0;

	if(! oflag)
		{
		struct	mtop mt;

		ret = read(fileno(magtfp), labelbuf, LABSIZE);
		if(ret < 0)
			errorexit("cannot read tape.  Incorrect tape density???", 0, 0);
		else if(ret == 0)
			errorexit("cannot read VOL from tape.  Empty tape???", 0, 0);
	
		sscanf(labelbuf, "%3s%1d%6s", l_labelid, &l_labelno, l_volid);
#ifdef DEBUG
	pr_label();
#endif
	
		if(strcmp(l_labelid, "VOL"))
			{
			errorexit("%s: illegal label format (VOL)", l_labelid, 0);
			}
		if(strcmp(l_volid, label))
			errorexit("correct label is %s", l_volid, 0);

		/* for tapes with no files (only initialized) */
		ret = read(fileno(magtfp), inbuf, sizeof(inbuf));
		if(! ret)
			{
			ret = read(fileno(magtfp), inbuf, sizeof(inbuf));
			if(! ret)
				nskipped = 1;
			if(ret > 0)
				{
				error("there is an eof mark between the VOL and HDR labels.", 0, 0);
				mt.mt_count = 1;
				mt.mt_op = MTBSF;
				if(ioctl(fileno(magtfp), MTIOCTOP, &mt) < 0)
					errorexit("cannot backskip tape", 0, 0);
				}
			}
		if(ret < 0)
			errorexit("cannot read tape.", 0, 0);
		/*
		 * we don't have to backskip the tape if ret > 0 because
		 * the ioctl in skip(0) would have skipped over it anyway.
		 */

		if(! nskipped)
			nskipped = skip(0);
#ifdef DEBUG
		printf("writetp: after skip(0), nskipped = %d\n", nskipped);
#endif
		}
	else if(oskip && (nskipped=skip(oskip)) != oskip)
		errorexit("there aren't that many files to skip (-O).", 0, 0);

	if(nskipped > 3)
		back(3);
	else
		{
		fclose(magtfp);
		rew();
		if((magtfp = fopen(magtdev, "r")) == NULL)
			errorexit("cannot open %s for reading\n", magtdev, 0);
		}

	if((ret = read(fileno(magtfp), labelbuf, LABSIZE)) <= 0)
		errorexit("cannot read HDR1 from tape", 0, 0);

	sscanf(labelbuf, "%3s%1d%17s%6s%4d%4d%4d%2d %5s %5s%*c%6ld",
	l_labelid, &l_labelno, l_filename, l_volid, &l_fsecno,
	&l_fseqno, &l_gen, &l_genver, l_credate, l_expirdate,
	&l_nblocks);

	/* for tapes containing no files or only one file */
	if(! strcmp(l_labelid, "VOL"))
		{
		if((ret = read(fileno(magtfp), labelbuf, LABSIZE)) < 0)
			errorexit("cannot read tape after VOL (not EOF)", 0, 0);
		sscanf(labelbuf, "%3s%1d%17s%6s%4d%4d%4d%2d %5s %5s%*c%6ld",
		l_labelid, &l_labelno, l_filename, l_volid, &l_fsecno,
		&l_fseqno, &l_gen, &l_genver, l_credate, l_expirdate,
		&l_nblocks);
		}
#ifdef DEBUG
	pr_label();
#endif

	if(ret != 0 && skip(3) && (strcmp(l_labelid, "HDR") || l_labelno != 1))
		{
		if(oflag)
			errorexit("You must append files after the last label in an EOF label set", 0, 0);
		else
			errorexit("%s%d: illegal label format (HDR1)",
		l_labelid, l_labelno);
		}

	if(freopen(magtdev, "w", magtfp) == NULL)
		errorexit("cannot reopen %s for writing", magtfp, 0);

	if(ret == 0)
		{
		/* back up over EOF */
		struct mtop	mt;

		mt.mt_count = 1;
		mt.mt_op = MTBSF;
		if(ioctl(fileno(magtfp), MTIOCTOP, &mt) < 0)
			errorexit("cannot backskip tape", 0, 0);

		fsecno = fseqno = 1;
		}


	else
		{
		fsecno = l_fsecno;
		fseqno = l_fseqno + 1;
		}
	proc_args(num, args, iflag, inputfile);
	}


/*---------------------------------------------------------------------
 *
 *  NUM  -  expands bsize into multiples of 512 and 1024.
 *
 *-------------------------------------------------------------------*/
num(bsize)
char	*bsize;
	{
	int	val;
	char	*p;

	p = bsize;

	for(val = 0; *p >= '0' && *p <= '9'; p++)
		val = 10 * val + *p - '0';

	if(p == bsize)
		return(0);
	switch(*p)
		{
		case '\0':
			return(val);
		case 'b':
			return(val * 512);
		case 'k':
			return(val * 1024);
		default:
			return(0);
		}
	}

/*---------------------------------------------------------------------
 *
 *  USAGE  -  prints a synopsis of all functions, flags, and options.
 *
 *-------------------------------------------------------------------*/
usage()
	{

	fprintf(stderr, "\nUSAGE: rdt tvx [-Bblksize] [-Ddens] [-Llabel] [-Ilist]  [filenames(s)] ...\n\n");

	exit(1);
	}

/*---------------------------------------------------------------------
 *
 *  BACK  -  backskips 'num' number of tape files.  Because the mt
 *	     driver looks only at eof marks, back backskips num+1 and
 *	     then forward skips 1 file in order to position the tape
 *	     AFTER the appropriate eof mark.
 *
 *-------------------------------------------------------------------*/
back(num)
int	num;
	{
	struct mtop	mt;

	mt.mt_count = num + 1;
	mt.mt_op = MTBSF;

	if(ioctl(fileno(magtfp), MTIOCTOP, &mt) < 0)
		errorexit("cannot backskip tape", 0, 0);

	mt.mt_count = 1;
	mt.mt_op = MTFSF;

	if(ioctl(fileno(magtfp), MTIOCTOP, &mt) < 0)
		errorexit("cannot skip tape", 0, 0);
	}

/*---------------------------------------------------------------------
 *
 *  MAKE_NUM  -  separates 'file' into a destination file name
 *		 ('dest') and a version number ('version').
 *
 *-------------------------------------------------------------------*/
make_num(file, dest, version)
char	*file, *dest, *version;
	{
	char	filename[256];
	char	*p, *s1, *s2;
	int	i, length;

	/* temp array for modifications, to preserve original state of argv */
	strcpy(filename, file);
	if(! metachk(filename) && (func == TABLE || func == EXTRACT))
		return(0);

	if(use_versnum)
		{
		char	*pp;

		p = filename;
		length = strlen(filename);

	/* Search  BACK  thru filename until a non * or a digit chctr is found
	*/
		for(p += length-1; p != filename; p--)
			{
			if(*p == '*' || *p == '?')
				continue;
			else if(*p == ']')
				{
				while(*--p != ']');
				}
			else if(*p < '0' || *p > '9')
				break;
			}

		for(pp=p, i=0; pp != filename && i < 4; i++)
			{
			pp--;
			if(func == TABLE || func == EXTRACT)
				{
				/*
				 * an asterisk may represent NO characters
				 * in which case it does not affect the length
				 * of the filetype.
				 */
				if(*pp == '*')
					i--;
				else if(*pp == ']')
					while(*--pp != '[');
				}
			if(*pp == '.')
				break;

			}

	/* Why does  *p appear twice in the following test ? 
	*/
		if(*p == '.' && filename[length-1] != '.' &&
		&filename[length] - p < MAXLEN &&
		pp != p && *p == '.')
			{
			if(strlen(p+1) > 5)
				{
				error("%s: version number too large", filename, 0);
				return(0);
				}
			strcpy(version, p+1);
			*p = 0;
			s1 = dest;
			s2 = filename;
			for(i=0; (*s1++ = *s2++) && i < MAXLEN; i++);
			if(i >= MAXLEN)
				{
				error("%s: file name too long", filename, 0);
				return(0);
				}
			}
		else
			{
			p = filename;
			p += length - 1;
			if(*p == '.')
				{
				for(i=0; i < 4 && p != filename; i++)
					{
					p--;
					if(func == TABLE || func == EXTRACT)
						{
						if(*p == '*')
							i--;
						else if(*p == ']')
							while(*--p != '[');
						}
					if(*p == '.')
						break;
					}
				if(*p == '.')
					filename[length-1] = 0;
				}
			else
				{
				for(i=0; i < 3 && p != filename; i++)
					{
					p--;
					if(func == TABLE || func == EXTRACT)
						{
						if(*p == '*')
							i--;
						else if(*p == ']')
							while(*--p != '[');
						}
					if(*p == '.')
						break;
					}
				if(*p != '.')
					{
					filename[length] = '.';
					filename[length+1] = 0;
					}
				}
			s1 = dest;
			s2 = filename;
			for(i=0; (*s1++ = *s2++) && i < MAXLEN; i++);
			if(i >= MAXLEN)
				{
				error("%s: file name too long", filename, 0);
				return(0);
				}
			if(func == EXTRACT || func == TABLE)
				strcpy(version, "*");
			else
				strcpy(version, "1");
			}
		}
	else
		{
		if(strlen(filename) > MAXLEN - 2)
			{
			error("%s: file name too long", filename, 0);
			return(0);
			}
		sprintf(dest, "%s", filename);
		if(func == EXTRACT || func == TABLE)
			strcpy(version, "*");
		else
			strcpy(version, "1");
		}
	if(func == EXTRACT || func == TABLE)
		lower(dest);
	else
		upper(dest);
	return(1);
	}

/*---------------------------------------------------------------------
 *
 *  REC_FILE  -  Gets arguments from the standard input or from
 *		 the given file.
 *
 *-------------------------------------------------------------------*/
rec_file(fp, prompt, crname)
FILE	*fp;			/* pointer to file or stdin */
char	prompt;			/* prompt flag */
char	*crname;		/* for 'c' and 'r' functions */
	{
	char	s1[MAXLEN], s2[MAXLEN];
	char	*p1, *p2;
	struct	filestat *fstat;
	char	line[256], *l;
	int	dumpflag = 0;
	register int i;

	if(func == CREATE || func == WRITE)
		{
		/* filetype has already been set equal to 0 in proc_args() */

		/* test if prompt is a '-' */
		if(prompt)
			{
			fprintf(stderr, "FILE ('return' to quit)? ");
			if(fgets(line, sizeof(line), fp) == NULL ||
			line[0] == '\n')
				return(EOF);

			/* remove newline */
			remove_c(line);
			chop(line);
			p1 = crname;
			p2 = line;
			for(i=0; (*p1++ = *p2++) && i < 100; i++);
			if(i >= 100)
				{
				error("%s: file name too long", line, 0);
				return(0);
				}
			return(1);
			}
		else if(fgets((l=line), sizeof(line), fp) != NULL)
			{
			remove_c(line);

			if((*l == 'b' || *l == 't' || *l == 'u' || *l == 'd'
			   || *l == '-') && isspace(*(l+1)))
				{
				if(*l == 't')
					filetype = TEXT;
				else if(*l == 'b')
					filetype = BINARY;
				else if(*l == 'u')
					filetype = FUF;
				else if(*l == 'd')
					filetype = COUNTED;
				else
					filetype = 0;
				while(isspace(*++l));
				}

			chop(l);
			p1 = crname;
			p2 = l;
			for(i=0; (*p1++ = *p2++) && i < 100; i++);
			if(i >= 100)
				{
				error("%s: file name too long", l, 0);
				return(0);
				}
			if(*crname == 0)
				return(EOF);
			return(1);
			}
		else
			return(EOF);
		}

	/* func equals either TABLE or EXTRACT */
	fstat = (struct filestat *) malloc(sizeof(*fstat));
	if(fstat == NULL)
		{
		if(prompt)
			{
			error("Out of memory.  No more file arguments can be processed.", 0, 0);
			return(1);
			}
		else
			errorexit("Too many command-line arguments.  Out of memory.", 0, 0);
		}
	fstat->f_next = f_head;
	f_head = fstat;

	for(;;)
		{
		if(prompt)
			{
			fprintf(stderr, "%cFILE ('return' to quit)? ", func == TABLE ? 0 : '\n');
			if(fgets(line, sizeof(line), fp) == NULL ||
			line[0] == '\n')
				{
				f_head = fstat->f_next;
				free((char *)fstat);
				return(1);
				}
			remove_c(line);
			chop(line);

			l = line;
			l += strlen(line) - 1;
			if(*l == '}')
				{
				while(*--l >= '0' && *l <= '9');
				if(*l == '{')
					{
					*l = 0;
					if(*++l < '1' || *l > '9')
						{
						error("invalid occurrence request.  {%s part of %s ignored.", l, line);
						fstat->f_numleft = -1;
						}
					else
						sscanf(l, "%d", &fstat->f_numleft);
					}
				else
					fstat->f_numleft = -1;
				}
			else
				fstat->f_numleft = -1;

			if(! make_num(line, fstat->f_dest, fstat->f_version))
				continue;
			if(func == EXTRACT)
				{
				fprintf(stderr, "EXTRACTED file name ('return' for default)? ");
				if(fgets(line, sizeof(line), fp) == NULL)
					continue;
				remove_c(line);
				chop(line);
				p1 = fstat->f_src;
				p2 = line;
				for(i=0; (*p1++ = *p2++) && i < MAXLEN; i++);
				if(i >= MAXLEN)
					{
					error("%s: file name too long", line, 0);
					continue;
					}
				}
			else
				fstat->f_src[0] = 0;
			numrecs++;
			fstat++;
			}
		else if(fgets(line, sizeof(line), fp) != NULL)
			{
			if(! toggle)
				dumpflag = 0;
			remove_c(line);
			p1 = s1;
			p2 = line;

			/* look for 'd' or 'u' or '-' at beginning of line */
			if(func == EXTRACT &&
			(*p2 == 'u' || *p2 == 'd' || *p2 == '-')
			&& isspace(*(p2+1)))
				{
				if(*p2 == 'u')
					{
					/*
					 * dumpflag is not set to 0 if toggle
					 * is NO because we want to check
					 * in scantape() if DD and FUF have
					 * together been specified for one
					 * file.
					 */
					if(toggle)
						dumpflag = 0;
					dumpflag |= FUF;
					}
				else if(*p2 == 'd')
					{
					if(toggle)
						dumpflag = 0;
					dumpflag |= DD;
					}
				else
					dumpflag = 0;
				while(isspace(*++p2));
				}
			for(i=0; (*p1++ = *p2++) && ! isspace(*(p2-1)) && i < MAXLEN; i++);
			if(i >= MAXLEN)
				{
				*(p1-2) = 0;
				error("%s: file name too long", s1, 0);
				continue;
				}
			*(p1-1) = 0;

			while(isspace(*p2))
				p2++;
			chop(p2);
			p1 = s2;
			for(i=0; (*p1++ = *p2++) && i < MAXLEN; i++);
			if(i >= MAXLEN)
				{
				*(p1-2) = 0;
				error("%s: file name too long", s2, 0);
				continue;
				}

			if(s1[0] == 0)
				{
				f_head = fstat->f_next;
				free((char *)fstat);
				return(1);
				}

			if(func == EXTRACT)
				strcpy(fstat->f_src, s2);
			else
				fstat->f_src[0] = 0;
			l = s1;
			l += strlen(s1) - 1;
			if(*l == '}')
				{
				while(*--l >= '0' && *l <= '9');
				if(*l == '{')
					{
					*l = 0;
					if(*++l < '1' || *l > '9')
						{
						error("invalid occurrence request.  {%s part of %s ignored.", l, s1);
						fstat->f_numleft = -1;
						}
					else
						sscanf(l, "%d", &fstat->f_numleft);
					}
				else
					fstat->f_numleft = -1;
				}
			else
				fstat->f_numleft = -1;

			if(! make_num(s1, fstat->f_dest, fstat->f_version))
				continue;

			fstat->f_flags = dumpflag;

			numrecs++;
			fstat++;
			}
		else
			{
			f_head = fstat->f_next;
			free((char *)fstat);
			return(1);
			}

		fstat = (struct filestat *) malloc(sizeof(*fstat));
		if(fstat == NULL)
			break;
		fstat->f_next = f_head;
		f_head = fstat;
		}
	if(prompt)
		error("Out of memory.  No more file arguments can be processed.", 0, 0);
	else errorexit("Too many arguments.  Out of memory.", 0, 0);
	}

/*---------------------------------------------------------------------
 *
 *  REMOVE_C  -  replaces the next-to-the-last character with a null
 *		 if it is a dot or a newline.
 *
 *-------------------------------------------------------------------*/
remove_c(line)
char	*line;
	{
	int	i;

	i = strlen(line) - 1;
	if(line[i] == '.' || line[i] == '\n')
		line[i] = 0;
	}

/*---------------------------------------------------------------------
 *
 *  REW  -  rewinds the tape.
 *
 *-------------------------------------------------------------------*/
rew()
	{
	FILE *fp;
	struct mtop	mtop;

	if((fp = fopen(magtdev, "r")) == NULL)
		errorexit("cannot rewind %s. Tape drive busy or not online???.", magtdev, 0);
	mtop.mt_op = MTREW;
	mtop.mt_count = 1;
	ioctl( fileno( fp), MTIOCTOP, &mtop);
	fclose(fp);
	}

/*---------------------------------------------------------------------
 *
 *  PROC_ARGS  -  processes arguments for the 'c' and 'r' functions.
 *
 *-------------------------------------------------------------------*/
proc_args(num, args, iflag, inputfile)
int	num;			/* number of arguments to process */
char	*args[];		/* array of pointers to arguments */
int	iflag;			/* -I option */
char	*inputfile;
	{
	FILE	*ifp;
	char	crname[NNML];
	int	ret;

	/*
	 * filetype has already been set equal to 0.
	 */

	if(nskip || pos)
		error("-S and -P flags do not apply when appending files", 0, 0);

	for( ; num > 0; num--, args++)
		{
		if(! strcmp(*args, "-u"))/* Fortran Unformatted */
			filetype = FUF;
		else if(! strcmp(*args, "-d"))/* Counted */
			filetype = COUNTED;
		else if(! strcmp(*args, "-b"))/* Binary */
			filetype = BINARY;
		else if(! strcmp(*args, "-t"))/* Text */ /* FILETYPE overrides
							  the default type.*/
			filetype = TEXT;
		else if(! strcmp(*args, "-")) /* Program decides */
			filetype = 0;
		else if((*args)[0] == '-')
			error("%s: unknown flag", *args, 0);
		else
			/* If none of the above apply, then do this...
			*/
			{
			if(strlen(*args) > 99)
				error("%s: file name too long", *args, 0);
			else
				tree(*args); /* TREE eveuntually gets file on tape. */
			if(! toggle)
				filetype = 0;
			}
		}

	filetype = 0;

	if(iflag == 1) /* Do we want to read more args from a file ?
			*/
		{
		if((ifp = fopen(inputfile, "r")) == NULL)
			errorexit("cannot open %s", inputfile, 0);
		while((ret = rec_file(ifp, '\0', crname)) != EOF)
			{
			if(ret)
				tree(crname); /* TREE eventually gets the file
						written out on the tape.
						*/
			}
		fclose(ifp);
		}
	else if(iflag == -1) /* Do we want to read more args from the STDIN ?
				*/
		while((ret = rec_file(stdin, inputfile[0], crname)) != EOF)
			{
			if(ret)
				tree(crname);
			}
	if(rep_misslinks)
		for( ; a_head != NULL; a_head = a_head->a_next)
			{
			if(a_head->a_count != 0)
				error("Missing links to %s.", a_head->a_pathname, 0);
			}
	}

/*---------------------------------------------------------------------
 *
 *  TREE  -  changes the present working directory before appending
 *	     a file in case a directory is to be recursively appended.
 *
 *-------------------------------------------------------------------*/
tree(pathname)
char	*pathname;
	{
	char	 wdir[1024];
	register char *cp, *cp2;

	if( getwd(wdir) == 0)
		{
		error( "getwd failed: %s", wdir, 0);
		strcpy( wdir, "."); /* Save orginal working directory */

		}
	cp2 = pathname;
	for(cp = pathname; *cp; cp++)
		if(*cp == '/')
			cp2 = cp; /* Remember address of last /  in pathname */
	/* Strip leading path information ( ie. chop  /foo from /foo/name ) */
	if(cp2 != pathname)
		{
		*cp2 = 0;
		if(chdir(pathname) < 0)
			{
			error("cannot chdir to %s", pathname, 0);
			return;
			}
		*cp2 = '/'; /* Put one of these back on. */
		cp2++;
		}
	/* If pathname was  / ,  then pass a null string for longname. 
	*/
	putfile(! strcmp(pathname, "/") ? "" : pathname, cp2);
	if(chdir(wdir) < 0)
		error("cannot chdir back to %s", wdir, 0);
	}

/*---------------------------------------------------------------------
 *
 *  PUTFILE  -  checks the type of file before calling 'process' to
 *		append it.
 *
 *-------------------------------------------------------------------*/
putfile(longname, shortname)
char *longname, *shortname;
	{
	FILE	*infile;
	DIR	*dirfile;
	char	buf[MAXNAMLEN+1];
	struct direct	*dirp;
	short	magic;
	int	i, j;
	char	*cp, *cp2;
	char c;

	if(stat(shortname, &inode) < 0)
		{
		if(! toggle)
			filetype = 0;
		error("cannot stat %s", longname, 0);
		return;
		}
	if((inode.st_mode & S_IFMT) == S_IFDIR)
		{
		if(toggle && filetype != 0)
			error("%s is a directory.  File type toggle reset to default.", longname, 0);
		filetype = 0;
		strcpy( buf, longname);
		strcat( buf, "/");
		
		if(chdir(shortname) < 0)
			{
			error("cannot chdir to %s", longname, 0);
			return;
			}

		/* read through directory */
		if( ( dirfile = opendir( ".")) == NULL)
			{
			error( "cannot open directory %s", longname, 0);
			return;
			}
		while(dirp = readdir( dirfile))
			{
			char sbuf[MAXNAMLEN+1];
			if(strcmp(".", dirp->d_name) == 0 ||
			strcmp("..", dirp->d_name) == 0)
				continue;
			strcpy(sbuf,buf);
			strcat( sbuf, dirp->d_name);
/*RECURSE*/
			putfile(sbuf, dirp->d_name); /* RECURSIVELY  call self !!!! */

			}
		closedir( dirfile);
		if(chdir("..") < 0)
			error("cannot chdir to .. from %s", longname, 0);
		return;
		}
	if((inode.st_mode & S_IFMT) == S_IFCHR)
		{
		if(! toggle)
			filetype = 0;
		error("%s is a character device.  Not dumped.", longname, 0);
		return;
		}
	if((inode.st_mode & S_IFMT) == S_IFBLK)
		{
		if(! toggle)
			filetype = 0;
		error("%s is a block device.  Not dumped.", longname, 0);
		return;
		}
	if((infile = fopen(shortname, "r")) == NULL)
		{
		if(! toggle)
			filetype = 0;
		/* call metachk for a possible warning message */
		metachk(shortname);
		error("cannot open %s", longname, 0);
		return;
		}
	if(! filetype)
		{
		/* fileno  is defined in  stdio as  ->  ((p)->file) 
		*/
		i = read(fileno(infile), (char *)&magic, sizeof(magic));
	
		/* zero-length file */
		if(! i)
			j = TEXT;
	
		else if(i < 0)
			{
			char	line[256];

			/* cannot determine the type of file -- ask the user. */
			fprintf(stderr, "%s: Is %s a TEXT or a BINARY file (t/b)? ", progname, longname);
			gets(line);
			if(line[0] == 't')
				j = TEXT;
			else if(line[0] == 'b')
				j = BINARY;
			else if(line[0] == 'd')
				j = COUNTED;
			else
				{
				j = TEXT;
				error("input must be 't', 'd' or 'b'", 0, 0);
				error("%s assumed to be a TEXT file", longname, 0);
				}
			}

		else
			/* Determine the type of file based only on the
			magic numbers.
			*/
			switch(magic)
				{
				/* executables */

				case 0407: /* old impure format */
				case 0410: /* read only text(code) */
				case 0411: /* jfr or  pdp-11  unix 411 executable */
				case 0405:
				case 0413: /* demand load format */
				case 0430:
				case 0431:
					j = BINARY;
					break;
	
				/* fortran unformatted file */
				case 0177332:
					j = FUF;
					break;

				/* archives */
				case 0177545:
				case 0177555:
					j = BINARY;
					break;
	
				default:
					if(magic & 0100200)
						j = BINARY;
					else
						j = TEXT;
					break;
				}
		}
	fclose(infile);
	process(longname, shortname, filetype == 0 ? j : filetype);
	}

/*---------------------------------------------------------------------
 *
 *  CHECKDIR  - checks if all directories needed to append a file exist.
 *	 	If not, checkdir creates those missing directories.
 *
 *-------------------------------------------------------------------*/
checkdir(path)
char	*path;
	{
	int	i;
	register char *cp;

	for(cp = path; *cp; cp++)
		{
		if(*cp == '/')
			{
			*cp = 0;
			if(stat(path, &inode) < 0)
				{
				register int pid, rp;

				if((pid = fork()) == 0)
					{
					execl("/bin/mkdir", "mkdir", path, 0);
					execl("/usr/bin/mkdir", "mkdir", path, 0);
					errorexit("cannot find mkdir!", 0, 0);
					}
				while((rp = wait(&i)) >= 0 && rp != pid);

				}
			*cp = '/';
			}
		}
	}

#define SUID	04000
#define SGID	02000
#define STXT	01000
#define ROWN	0400
#define WOWN	0200
#define XOWN	0100
#define RGRP	040
#define WGRP	020
#define XGRP	010
#define ROTH	04
#define WOTH	02
#define XOTH	01
int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9 };


/*---------------------------------------------------------------------
 *
 *  EXPAND_MODE  -  calls 'select' for each of the 9 mode fields.
 *
 *-------------------------------------------------------------------*/
expand_mode(mode)
int	mode;
	{
	register int **mp;

	for(mp = &m[0]; mp < &m[9]; )
		select(*mp++, mode);
	}

/*---------------------------------------------------------------------
 *
 *  SELECT  -  determines the status of the next mode field.
 *
 *-------------------------------------------------------------------*/
select(choices, mode)
int	*choices;			/* number of choices for this field */
int	mode;
	{
	register int num, *ch;

	ch = choices;
	num = *ch++;
	while(--num >= 0 && (mode & *ch++) == 0)
		ch++;
	printf("%c", *ch);
	}

/*---------------------------------------------------------------------
 *
 *  CHOP  -  truncates 'string' after the first non-white space
 *	     character.  White space characters include the space, tab,
 *	     carriage return, newline, and the formfeed.
 *
 *-------------------------------------------------------------------*/
chop(string)
char *string;
	{
	while(*string != 0 && ! isspace(*string))
		string++;
	if(isspace(*string))
		*string = 0;
	}

/*---------------------------------------------------------------------
 *
 *  NOT_ON_TAPE  -  formats the error statement when a position file
 *		    is not on the tape.
 *
 *-------------------------------------------------------------------*/
not_on_tape()
	{
	char	temp[MAXLEN+7];

	if(! strcmp(posnum, "*"))
		{
		if(! use_versnum)
			remove_c(posname);
		strcpy(temp, posname);
		}
	else
		sprintf(temp, "%s.%s", posname, posnum);
	if(posnth)
		{
		sprintf(posname, "{%d}", posnth);
		strcat(temp, posname);
		}
	errorexit("%s is not on the tape.", temp, 0);
	}

/*----------------------------------------------------------------------
 *
 *  FUFCNV  -  converts an LTF FUF file to a Ultrix disk file.
 *
 *-------------------------------------------------------------------*/
fufcnv(p, count, fp)
char	*p;
int	count;
FILE	*fp;
	{
	static	int bincnt = 0;
	int	flag1, flag2;

	flag1 = *p++;
	flag2 = *p++;

	if(flag2 != 0)
		error("second control byte is %d", flag2, 0);

	count -= 2;

	switch(flag1)
		{
		case ALL:
			bincnt = count;
			write(fileno(fp), &bincnt, sizeof(bincnt));
			write(fileno(fp), p, bincnt);
			write(fileno(fp), &bincnt, sizeof(bincnt));
			break;

		case FIRST:
			bincnt = 0;
			write(fileno(fp), spaces, sizeof(bincnt));
			write(fileno(fp), p, count);
			bincnt += count;
			break;

		case MIDDLE:
			write(fileno(fp), p, count);
			bincnt += count;
			break;

		case LAST:
			write(fileno(fp), p, count);
			bincnt += count;
			fseek(fp, (long)(-1 * (bincnt+sizeof(bincnt))), 1);
			write(fileno(fp), &bincnt, sizeof(bincnt));
			fseek(fp, (long)bincnt, 1);
			write(fileno(fp), &bincnt, sizeof(bincnt));
			break;

		default:
			error("first byte is %d", flag1, 0);
			break;
		}
	return(1);
	}

/*---------------------------------------------------------------------
 *
 *  PR_LABEL  -  prints the contents of 'labelbuf'.
 *
 *-------------------------------------------------------------------*/
pr_label()
	{
	labelbuf[LABSIZE] = 0;
	fprintf(stdout,"\n%s\n", labelbuf);
	}

/*---------------------------------------------------------------------
 *
 *  MSTRCMP_DOT  -  compares 's' and 't' while distinguishing between
 *		    file names and file types.  It expands any
 *		    metacharacters in 't'.  However, metacharacters
 *		    cannot represent the delimiting dot '.'.
 *		    's' is the file name from tape and 't' is the
 *		    user input.  Mstrcmp_dot returns 0 if the two
 *		    strings are equivalent and -1 if not.
 *		    I chose 0 as a return value because 'strcmp'
 *		    also returns 0 in this case.  The number 1, in
 *		    my mind, represents a "good" return value
 *		    relative to -1; thus -1 is returned if the
 *		    two strings are different.
 *
 *-------------------------------------------------------------------*/
mstrcmp_dot(s, t)
char *s, *t;
	{
	while(1)
		{
		if(*s == *t)
			{
			if(*s == 0)
				return(0);
			s++;
			t++;
			continue;
			}
		else if(*t == '.' || (*s == '.' && *t != '*'))
			return(-1);
		else
			switch(*t)
				{
				case '?':
					t++;
					s++;
					break;
				case '[':
					while(*++t != ']')
						{
						if(*s == *t || *t == '-' &&
						*s >= *(t-1) && *s <= *(t+1))
							{
							s++;
							while(*t++ != ']');
							break;
							}
						}
					if(*t == ']')
						return(-1);
					break;
	
				case '*':
					while(*++t == '*');
					if(*t == '.')
						{
						while(*s++ != '.');
						t++;
						}
					else
						{
						char *ss;
						for(;;)
							{
							while(*s != *t)
								{
								if(*s == '.')
									return(-1);
								s++;
								}
							ss = s;
							while(*s == *t)
								{
								if(*s == '.')
									break;
								s++;
								t++;
								}
							if((*s == *t && *s == '.') ||
							*t == '*' || *t == '?' ||
							*t == '[')
								break;
							s = ss+1;
							}
						}
					break;
				default:
					return(-1);
				}
		}
	}

/*---------------------------------------------------------------------
 *
 *  METACHK  -  checks 's' for the proper use of metacharacters.  It
 *		returns 1 if OK, 0 if error.
 *
 *-------------------------------------------------------------------*/
metachk(s)
char *s;
	{
	while(*s != 0)
		{
		if((*s == '*' || *s == ']' || *s == '?') &&
		(func == CREATE || func == WRITE))
			{
				/* The term append here implies a create. */
			error("metacharacters have no special meaning when appending files.", 0, 0);
			return(1);
			}
		switch(*s)
			{
			case '*':
			case '?':		/* fall through */
				break;
	
			case '[':
				while(*++s != ']')
					if(*s == 0)
						{
						error("unmatched [", 0, 0);
						return(0);
						}
				break;
			}
		s++;
		}
	return(1);
	}

/*---------------------------------------------------------------------
 *
 *  MSTRCMP  -  compares 's' and 't' while expanding metacharacters in
 *		't'.  's' is the file name from tape and 't' is the
 *		user input.  Mstrcmp returns 0 if the two strings
 *		are equivalent and -1 if not.
 *
 *-------------------------------------------------------------------*/
mstrcmp(s, t)
char *s, *t;
	{
	while(1) {
		if(*s == *t) {
			if(*s == 0)
				return(0);
			s++;
			t++;
			continue;
		}/*E if *s == *t ..*/
		/*
		 * Character *s != character *t.
		 * The following says  "IF" we are not at the end
		 * of user input string (t) return "no-match".
		 * -or- We are at the end of the tape name (s) and
		 * user input is "NOT" a wild *, also say "no-match".
		 */
		else if(*t == 0 || (*s == 0 && *t != '*'))
			return(-1);
		else
			/*
			 * *s character != *t  character  _and_
			 * there are more characters in at
			 * least one of the two strings.
			 *
			 * Key off of the character in the user
			 * input string.
			 */
			switch(*t) {
				case '?':
					t++;
					s++;
					break;
				case '[':
					while(*++t != ']')
						{
						if(*s == *t || *t == '-' &&
						*s >= *(t-1) && *s <= *(t+1))
							{
							s++;
							while(*t++ != ']');
							break;
							}
						}
					if(*t == ']')
						return(-1);
					break;
	
				/* Case '*' says match any string of
				 * chctrs up to the position of '*'.
				 */
				case '*':
					/* The 'while' filters out
					 * multiple consecutive
					 * of the '*' character.
					 */
					while(*++t == '*')
						;
					if(*t == 0)
						/* If we have exhausted
						 * the user input string
						 * return a "match".
						 */
						return(0);
					else {
						char *ss;
						for(;;)
							{
							/* We have seen
							 * an '*' in usr
							 * input, skiped
							 * over it..now
							 * begin direct
							 * compare again
							 * We are trying
							 * to dicard
							 * leading
							 * chctrs in 's'
							 * due to the
							 * '*' seen in
							 * 't' & find
							 * the next 
							 * match of
							 * real chctrs
							 * in the two
							 * strings to
							 * resume real
							 * comparison.
							 */
							while(*s != *t){
							    if(*s == 0)
							    /* Ran out
							     * of tape
							     * file
							     * name, say
							     * strings
							     * .ne. ..
							     */
							        return(-1);
							    s++;
							}/*E while *s */
							/*
							 * Save pointer
							 * to first re-
							 * match chctrs.
							 */
							ss = s;
							while(*s == *t){
							    if(*s == 0)
							    /* Ran out
							     * tape name
							     * say names
							     * are ===.?
							     */
								return(0);
							    s++;
							    t++;
							}/*E while *s */

							if(*t == '*' || *t == '?' ||
							*t == '[')
							    /* Break out
							     * of the
							     * for ;;
							     * loop.
							     */
							    break;
							/* Reset 's' to
							 * point to the
							 * chctr after
							 * the last
							 * match chctr
							 * in the 2
							 * strings &
							 * go back to
							 * top-level of
							 * compare logic
							 * now that we
							 * have seen a
							 * non-match.
							 */
					/* The previous note and the
					 * next 'break' serve to hilite
					 * that given the user input of
					 *  *.dat, the code was pulling
					 * file names off the tape that
					 * had any occurance of the
					 * string 'dat' in the suffix.
					 * ie. elle.update,
					 *     file.dat
					 *     file.withdatin
					 * would all get dragged in
					 * when all we wanted was
					 * 'file.dat' ! (rjg).
					 */
							s = ss+1;
							/* Break out of
							 * for ;; loop.
							 */
							break;
							}/*E for ;; ..*/
						}/*E to else for
						  * if *t == 0 ..*/

					/* Break out of 'switch' */
					break;

				default:
					/* No match ... */
					return(-1);

				}/*E switch (*t) ..*/
		}/*E while (1) ..*/
	}/*E mstrcmp() ..*/

/*---------------------------------------------------------------------
 *
 *  WEOF  -  writes an end-of-file mark on tape.
 *
 *-------------------------------------------------------------------*/
weof()
	{
	struct mtop	mt;

	mt.mt_count = 1;
	mt.mt_op = MTWEOF;

	if(ioctl(fileno(magtfp), MTIOCTOP, &mt) < 0)
		errorexit("cannot write eof on tape.", 0, 0);
	}

/*---------------------------------------------------------------------
 *
 *  ITOA  -  fills 'str' with the ascii representation of 'num'.
 *
 *-------------------------------------------------------------------*/
itoa(num, str)
int	num;
char	*str;
	{
	char *p;
	p = str;
	do
		{
		*p++ = num % 10 + '0';
		} while ((num /= 10) > 0);
	*p = 0;
	reverse(str);
	}

/*---------------------------------------------------------------------
 *
 *  REVERSE  -  reverses the characters of string 's' in place.
 *
 *-------------------------------------------------------------------*/
reverse(s)
char *s;
	{
	char c;
	int i, j;
	for(i=0, j=strlen(s)-1; i < j; i++, j--)
		{
		c = s[i];
		s[i] = s[j];
		s[j] = c;
		}
	}

