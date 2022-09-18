

/* 	@(#)paths.h	4.1	(ULTRIX)	7/17/90 	*/

/*
 *
 *	PATHS.H -- This header file contains the path information required
 *		   by sccs.c (and a few others) to find the appropriate 
 *		   commands.
 *
 */

/*
 *
 *  The following defines are used primarily in SCCS.C
 *
 */
# ifndef SCCSPATH
# define SCCSPATH	"SCCS"		/* pathname in which to find s-files */
# endif NOT SCCSPATH

# ifndef MYNAME
# define MYNAME		"sccs"		/* name used for printing errors */
# endif NOT MYNAME

# ifndef PROGPATH
# define PROGPATH(name)	"/usr/bin/name"	/* place to find binaries */
# endif PROGPATH

/*
 *
 *  The following defines are used in HELP.C
 *
 */
# define HELPLOC 	"/usr/lib/sccs.help/helploc"
# define DFTFILE	"/usr/lib/sccs.help/default"
# define HELPDIR	"/usr/lib/sccs.help/"
