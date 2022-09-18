/*	@(#)dump.h	4.3	(ULTRIX)	12/6/90	*/

#ifdef	RDUMP
#define	REMOTE
#endif

#ifdef	RRESTORE
#define	REMOTE
#endif

#include <ctype.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/fs.h>
#include <sys/inode.h>

#include <sys/dir.h>
#include <utmp.h>
#include <sys/time.h>
#include <signal.h>
#include <fstab.h>
#include <sys/ioctl.h>

#include <sys/mtio.h>
#include <sys/file.h>

#include <dumprestor.h>

/* Local Equates */
#define	NI		16
#define MAXINOPB	(MAXBSIZE / sizeof(struct dinode))
#define MAXNINDIR	(MAXBSIZE / sizeof(daddr_t))
#define MAXASYNC	16

/* File Names */
#define	NINCREM	"/etc/dumpdates"	/* new format incremental info */
#define	TEMP	"/etc/dtmp"		/* output temp file */

/* Default Device Names */
#define	TAPE	DEFTAPE_RH		/* default tape device */
#define	DISK	"/dev/rrp1g"		/* default disk */
#define	OPGRENT	"operator"		/* group entry to notify */
#define DIALUP	"ttyd"			/* prefix for dialups */

/* Device types for dump output */
#define TAP 		1		/* output type is a Tape */
#define FIL		2		/* output type is a file */
#define PIP		3		/* output is to standard output */
#define DSK		4		/* output is to raw disk device */

/* Exit status codes */
#define	X_FINOK		1		/* normal exit */
#define	X_REWRITE	2		/* restart writing from check point */
#define	X_ABORT		3		/* abort dump; don't checkpoint */

/* Local Macro Definitions */
#define	ITITERATE(i, ip) for (i = 0,ip = idatev[0]; i < nidates; i++, ip = idatev[i])

#define	HOUR	(60L*60L)
#define	DAY	(24L*HOUR)
#define	YEAR	(365L*DAY)

#define	MBIT(i)		(1<<((unsigned)(i-1)%NBBY))
#define	MWORD(m,i)	(m[(unsigned)(i-1)/NBBY])

#define	BIS(i,w)	(MWORD(w,i) |=  MBIT(i))
#define	BIC(i,w)	(MWORD(w,i) &= ~MBIT(i))
#define	BIT(i,w)	(MWORD(w,i) & MBIT(i))

/*
 *	Local Variables 
 *	Note: All tape calculations are in .1" units!
 */
char	*disk;		/* name of the disk file */
char	*tape;		/* name of the tape file */
char	*increm;	/* name of the file containing incremental information*/
char	*temp;		/* name of the file for doing rewrite of increm */
char	lastincno;	/* increment number of previous dump */
char	incno;		/* increment number */

int	fi;		/* disk device file descriptor */
int	to;		/* tape device file descriptor */

ino_t	ino;		/* current inumber; used globally */

int	newtape;	/* flag - new tape */
int	dadded;		/* flag - directory added */
int	notify;		/* flag - notify operator */
int	oflag;		/* flag - compatability mode rdump */
int	uflag;		/* flag - update */
int	user_tsize;	/* flag - user specified tape length (s option) */
int	nadded;		/* number of added sub directories */
int	nsubdir;

/* Tape Information */
int	density;	/* density in 0.1" units */
long	tsize;		/* tape cap. - # of blocks tape will hold(0.1" units) */
long	esize;		/* estimated tape size, blocks */
long	asize;		/* actual number of blocks written on current tape */
int	etapes;		/* estimated number of tapes */

int	blockswritten;	/* number of blocks written on current tape */
int	tapeno;		/* current tape number */
time_t	tstart_writing;	/* time first tape block was written */
char	*processname;	/* unused by dump */
struct fs *sblock;	/* ptr. to file system super block structure */
struct devget mt_info;	/* returned device information from ioctl */
char	buf[MAXBSIZE];

int	msiz;		/* map size - # of memory char elements */
char	*clrmap;	/* pointer to bitmap of free inodes */
char	*dirmap;	/* pointer to bitmap of directory nodes */
char	*nodmap;	/* pointer to bitmap of inodes to be dumped */


/* Global Variables */
#ifdef	REMOTE
char	*host;
struct	stat	*statbfp; 
struct  devget  mt_info;
struct 	devget 	*devgetp;
struct 	pt	*partp;
struct	stat	*rmtstat();
struct	pt	*rmtgetpart();
struct	devget	*rmtgenioctl();
extern	char	*host;
extern  struct  devget mt_info;
extern 	struct	devget	*devgetp;
extern 	struct	stat	*rmtstat();
extern	struct	pt	*rmtgetpart();
extern	struct	devget	*rmtgenioctl();
#endif REMOTE
extern	int	to;		/* file descriptor for output device */
extern	int	devblocks;      /* number of blocks on outout device */
extern	int	device_open;    /* flag to signal output device open */
extern	int	eom_flag;	/* flag to signal at end of media */
extern	int	size_only;	/* flag to signal sizing only     */
extern	int	usecreate;	/* flag indicating to use 'creat' */
				/* instead of 'open' */

/* Function Returns Declarations */
char	*ctime();
char	*prdate();
long	atol();
int	mark();
int	add();
int	dump();
int	tapsrec();
int	dmpspc();
int	dsrch();
int	nullf();
char	*getsuffix();
char	*rawname();
struct dinode *getino();
int	interrupt();		/* in case operator bangs on console */
struct	fstab	*fstabsearch();	/* search in fs_file and fs_spec */

/* 	Interrupt Handlers  */
int	sighup();
int	sigquit();
int	sigill();
int	sigtrap();
int	sigfpe();
int	sigkill();
int	sigbus();
int	sigsegv();
int	sigsys();
int	sigalrm();
int	sigterm();

/*
 *	The contents of the file NINCREM is maintained both on
 *	a linked list, and then (eventually) arrayified.
 */
struct	idates {
	char	id_name[MAXNAMLEN+3];
	char	id_incno;
	time_t	id_ddate;
};
struct	itime{
	struct	idates	it_value;
	struct	itime	*it_next;
};
struct	itime	*ithead;	/* head of the list version */
int	nidates;		/* number of records (might be zero) */
int	idates_in;		/* we have read the increment file */
struct	idates	**idatev;	/* the arrayfied version */

/* uVAXrx50: Added to minimize the # of concurrent processes */
int 	lparentpid;	/* latest parrent process id (the one we kill) */
int 	mpid;		/* master process id (the one run from the tty) */
int	exitmaster();	/* SIGEMT to mpid gets caught there (normal exit) */
int	abortmaster();	/* SIGQUIT to mpid gets caught there (abort exit) */
