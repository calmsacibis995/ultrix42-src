/*	@(#)restore.h	4.1	(ULTRIX)	7/2/90	*/

#include <stdio.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "dir.h"
#include <sys/file.h>
#include <sys/mtio.h>
#include <dumprestor.h>

/*
 * Flags
 */
extern int	device_open;	/* flag to indicate device is open     */
extern int	cvtflag;	/* convert from old to new tape format */
extern int	dflag;		/* print out debugging info */
extern int	hflag;		/* restore heirarchies */
extern int	mflag;		/* restore by name instead of inode number */
extern int	vflag;		/* print out actions taken */
extern int	yflag;		/* always try to recover from tape errors */
extern int	pipein;		/* input is from standard input */
extern int	eomflag;	/* end of media detected flag   */
/*
 * Global variables
 */
extern int	mt;		/* input device or file's fd */
extern char	*dumpmap; 	/* map of inodes on this dump tape */
extern char	*clrimap; 	/* map of inodes to be deleted */
extern char	*magtape;	/* pointer to input device name*/
extern ino_t	maxino;		/* highest numbered inode in this file system */
extern long	dumpnum;	/* location of the dump on this tape */
extern long	volno;		/* current volume being read */
extern time_t	dumptime;	/* time that this dump begins */
extern time_t	dumpdate;	/* time that this dump was made */
extern char	command;	/* opration being performed */
extern FILE	*terminal;	/* file descriptor for the terminal input */
extern int	devtyp;		/* device type of input device */

#ifdef	RRESTORE
extern struct	stat	*rmtstat();
extern struct	pt	*rmtgetpart();
extern struct	devget	*rmtgenioctl();
extern rmtopen(),rmtclose();
#endif	RRESTORE

/*
 * Each file in the file system is described by one of these entries
 */
struct entry {
	char	*e_name;		/* the current name of this entry */
	u_char	e_namlen;		/* length of this name */
	char	e_type;			/* type of this entry, see below */
	short	e_flags;		/* status flags, see below */
	ino_t	e_ino;			/* inode number in previous file sys */
	long	e_index;		/* unique index (for dumpped table) */
	struct	entry *e_parent;	/* pointer to parent directory (..) */
	struct	entry *e_sibling;	/* next element in this directory (.) */
	struct	entry *e_links;		/* hard links to this inode */
	struct	entry *e_entries;	/* for directories, their entries */
	struct	entry *e_next;		/* hash chain list */
};
/* types */
#define	LEAF 1			/* non-directory entry */
#define NODE 2			/* directory entry */
#define LINK 4			/* synthesized type, stripped by addentry */
/* flags */
#define EXTRACT		0x0001	/* entry is to be replaced from the tape */
#define NEW		0x0002	/* a new entry to be extracted */
#define KEEP		0x0004	/* entry is not to change */
#define REMOVED		0x0010	/* entry has been removed */
#define TMPNAME		0x0020	/* entry has been given a temporary name */

/* Device types for restore input */
#define TAP 		1		/* input type is a Tape */
#define FIL		2		/* input type is a file */
#define PIP		3		/* input is from standard input */
#define DSK		4		/* input is from raw disk device */

/*
 * functions defined on entry structs
 */
extern struct entry *lookupino();
extern struct entry *lookupname();
extern struct entry *lookupparent();
extern struct entry *addentry();
extern char *myname();
extern char *savename();
extern char *gentempname();
extern char *flagvalues();
extern ino_t lowerbnd();
extern ino_t upperbnd();
#define NIL ((struct entry *)(0))
/*
 * Constants associated with entry structs
 */
#define HARDLINK	1
#define SYMLINK		2
#define TMPHDR		"RSTTMP"

/*
 * The entry describes the next file available on the tape
 */
struct context {
	char	*name;		/* name of file */
	ino_t	ino;		/* inumber of file */
	struct	dinode *dip;	/* pointer to inode */
	char	action;		/* action being taken on this file */
} curfile;
/* actions */
#define	USING	1	/* extracting from the tape */
#define	SKIP	2	/* skipping */
#define UNKNOWN 3	/* disposition or starting point is unknown */

/*
 * Other exported routines
 */
extern ino_t psearch();
extern ino_t dirlookup();
extern long listfile();
extern long deletefile();
extern long addfile();
extern long nodeupdates();
extern long verifyfile();
extern char *rindex();
extern char *index();
extern char *strcat();
extern char *strncat();
extern char *strcpy();
extern char *strncpy();
extern char *fgets();
extern char *mktemp();
extern char *malloc();
extern char *calloc();
extern char *realloc();
extern long lseek();

/*
 * Useful macros
 */
#define	MWORD(m,i) (m[(unsigned)(i-1)/NBBY])
#define	MBIT(i)	(1<<((unsigned)(i-1)%NBBY))
#define	BIS(i,w)	(MWORD(w,i) |=  MBIT(i))
#define	BIC(i,w)	(MWORD(w,i) &= ~MBIT(i))
#define	BIT(i,w)	(MWORD(w,i) & MBIT(i))

#define dprintf		if (dflag) fprintf
#define vprintf		if (vflag) fprintf

#define GOOD 1
#define FAIL 0

/* Number of buffers to use in n-buffered I/O operations */
#define NBCNT 4
