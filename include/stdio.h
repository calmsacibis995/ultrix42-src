/*	@(#)stdio.h	4.6	(ULTRIX)	3/1/91	*/
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
/************************************************************************
 *			Modification History
 *
 *	Mitch Condylis 28-Feb-1991
 *	Changed _file member of FILE structure from char to short
 *	as part of work to increase max number of open file descriptors.
 *
 *	Mike Thomas, 7-Sep-1990
 * 016	Back out most of 015. Leave in explicit declarations.
 *
 * 	Mike Thomas, 21-Aug-1990
 * 015	DECwest ANSI - change _filbuf to __filbuf for ANSI compliance.
 *	Explicitly declare __filbuf under __STDC__.
 *	Likewise flsbuf.
 *
 * 014  Mike Thomas, 08-Jun-90
 *	Changed _POSIX_SOURCE reference back to POSIX.
 *
 * 013  Dan Smith, 23-Feb-90
 *      Added const to several prototypes. More namespace protection.
 *      Changed reference of POSIX to _POSIX_SOURCE.
 *   
 *	Jon Reeves, 07-Dec-1989
 * 012	Namespace protection.
 *
 *	Jon Reeves, 09-Nov-1989
 * 011	Fix putc properly: could still sign-extend in some cases before.
 *
 *	Linda Wilson, 06-Oct-1989
 * 010  Declare sprintf as int for std conformance
 *
 *	Jon Reeves, 25-Aug-1989
 * 009	Fix putc[har] for 8-bit mode (unsigned int->char)
 *
 *	Jon Reeves, 18-Jul-1989
 * 008	Add getw, putw for X/Open conformance.
 *
 *	Jon Reeves, 31-May-1989
 * 007	ANSI conformance; clean up rogue comments.  sprintf is still
 *	wrong.
 *
 *	Lu Anne Van de Pas, 02-Jun-1986
 * 006  Added ending "/" to P_tmpdir string.  
 * 
 *	David L Ballenger, 22-Nov-1985
 * 005	Correct definition of sprintf() for System V environment.
 *
 *	David L Ballenger, 01-Aug-1985
 * 004	Add _IOAPPEND flag for files opened with "A" or "A+".
 *
 *	David L Ballenger, 26-Jun-1985
 * 003	Add changes so that FILE structures are allocated dynamically.
 *
 *	Larry Cohen, 23-April-1985
 *      - change NFILE from 20 to 64
 *
 *	David L Ballenger, 13-Mar-1985
 * 0001	Add System V definitions.
 ************************************************************************/

#include <ansi_compat.h>
#ifndef	_SIZE_T_
#define	_SIZE_T_
typedef unsigned int	size_t;		/* type of sizeof */
#endif	/* _SIZE_T_ */

#ifndef _STDIO_H_
#define	_STDIO_H_

#define	BUFSIZ	1024
#define FOPEN_MAX	64	/* equals NOFILE in <sys/param.h> */
#define FILENAME_MAX	1024	/* equals MAXPATHLEN in <sys/param.h> */
#define	TMP_MAX	17576 /* equals value in limits.h: 26*26*26 */
			/* Note: spacing must match, too, to avoid warnings */

#define _N_STATIC_IOBS 3
#define	_NFILE	64   /* should equal NOFILE in <sys/param.h> */

extern	struct	_iobuf {
	int	_cnt;
	char	*_ptr;
	char	*_base;
	int	_bufsiz;
	short	_flag;
	short	_file;
} _iob[_N_STATIC_IOBS];
typedef	struct _iobuf	FILE;

typedef	long	fpos_t;

#define	stdin	(&_iob[0])
#define	stdout	(&_iob[1])
#define	stderr	(&_iob[2])

#define _IOFBF		00000
#define	_IOREAD		00001
#define	_IOWRT		00002
#define	_IONBF		00004
#define	_IOMYBUF	00010
#define	_IOEOF		00020
#define	_IOERR		00040
#define	_IOSTRG		00100
#define	_IOLBF		00200
#define	_IORW		00400
#define _IOAPPEND	01000

/*	fseek() values	*/
#define SEEK_SET 0
#define	SEEK_CUR 1
#define SEEK_END 2

#define	NULL	0
#define	EOF	(-1)
#ifdef __STDC__
/*
 *  prototype
 *
 */
extern int 	getc( FILE *__stream );
extern int	getchar( void );
extern int	putc( int __c, FILE *__stream);
extern int	putchar( int __c);
extern int	feof( FILE *__stream );
extern int	ferror( FILE *__stream );
extern int	fileno( FILE *__stream );
extern int	_filbuf( FILE *p);
extern int	_flsbuf( unsigned char x , FILE *p);
#endif /* __STDC__ */

#define	getc(p)		(--(p)->_cnt>=0? *(p)->_ptr++&0377:_filbuf(p))
#define	getchar()	getc(stdin)
#define putc(x,p) \
	(--(p)->_cnt>=0? \
		((int)(*(unsigned char *)(p)->_ptr++=(x))):\
		_flsbuf((unsigned char)(x),p))
#define	putchar(x)	putc(x,stdout)
#define	feof(p)		(((p)->_flag&_IOEOF)!=0)
#define	ferror(p)	(((p)->_flag&_IOERR)!=0)
#define	fileno(p)	((int)((p)->_file))

#ifdef __STDC__
#ifndef _VA_LIST_
#define _VA_LIST_
typedef char *va_list;
#endif /* _VA_LIST_ */
/*
 *  prototypes 
 *
 */
extern void	clearerr( FILE *__stream); 
extern int	fclose( FILE *__stream );
extern FILE *	fdopen( int __filedes, char *__type );
extern int	fflush( FILE *__stream );
extern int	fgetc( FILE *__stream );
extern int	fgetpos( FILE *__stream, fpos_t *__pos );
extern char *	fgets( char *__s, int __n, FILE *__stream );
extern FILE *	fopen( const char *__filename, const char *__type );
extern int	fprintf( FILE *__stream, const char *__format, ... );
extern int	fputc( int __c, FILE *__stream );
extern int	fputs( const char *__s, FILE *__stream );
extern size_t	fread( void *__ptr, size_t __size,
			size_t __nitems, FILE *__stream ); 
extern FILE *	freopen( const char *__filename, const char *__type,
			FILE *__stream );
extern int	fscanf( FILE *__stream, const char *__format, ... );
extern int	fseek( FILE *__stream, long __offset, int __ptrname );
extern int	fsetpos( FILE *__stream, const fpos_t *__pos );
extern long	ftell( FILE *__stream );
extern size_t	fwrite( const void *__ptr, size_t __size,
			size_t __nitems, FILE *__stream );
extern char *	gets( char *__s );	
extern void	perror( const char *__s );
extern FILE  *	popen(const char *__command, const char *__type );
extern int	printf( const char *__format, ... );	
extern int	puts( const char *__s );	
extern int	remove( const char *__filename );
extern int	rename( const char *__from, const char *__to );
extern void	rewind( FILE *__stream );
extern int	scanf( const char *__format, ... );	
extern void	setbuf( FILE *__stream, char *__buf );
extern int	setvbuf( FILE *__stream, char *__buf,
			int __type, size_t __size );
extern int	sscanf( const char *__s, const char *__format, ... );
extern FILE *	tmpfile( void );	
extern char *	tmpnam( char *__s );
extern int	ungetc( int __c, FILE *__stream );
extern int	vfprintf( FILE *__stream, const char *__format, va_list __ap );
extern int	vprintf( const char *__format, va_list __ap );
extern int	vsprintf( char *__s, const char *__format, va_list __ap);

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
extern char *	tempnam( const char *__dir, const char *__pfx);
extern int	putw( int __w, FILE *__stream );
extern int	getw(FILE *__stream);
extern int	pclose( FILE *__stream );
#endif

#else

extern FILE
	*fopen(),
	*fdopen(),
	*freopen(),
	*popen(),
	*tmpfile();
extern char
	*ctermid(),
	*cuserid(),
	*fgets(),
	*gets(),
	*tmpnam();
extern int
	fclose(),
	fflush(),
	fgetc(),
	fgetpos(),
	fprintf(),
	fputc(),
	fputs(),
	fscanf(),
	fseek(),
	fsetpos(),
	printf(),
	puts(),
	remove(),
	rename(),
	scanf(),
	setvbuf(),
	sscanf(),
	ungetc(),
	vfprintf(),
	vprintf(),
	vsprintf();
extern long
	ftell();
extern size_t
	fread(),
	fwrite();
extern void
	clearerr(),
	perror(),
	rewind(),
	setbuf(),
	setbuffer();

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
extern char
	*tempnam();
extern int
	getw(),
	pclose(),
	putw();
#endif /* _POSIX_SOURCE */

#if !defined(_POSIX_SOURCE)
/*
	Setlinebuf is really void, but it's used in libp (at least) in
	an expression in such a way that it has to be declared int.  Sigh.
 */
#ifdef __vax
extern void
	setlinebuf();
#else
extern int
	setlinebuf();
#endif /* __vax */

#endif /* _POSIX_SOURCE */

#endif /* __STDC__ */

#ifdef __STDC__
/* function prototype */
extern int	sprintf( char *__s, const char *__format, ... );
#else
#if defined(__SYSTEM_FIVE) || defined(__POSIX)
extern int sprintf();
#else
extern char	*sprintf();
#endif /* __SYSTEM_FIVE || __POSIX */
#endif /* __STDC__ */

#define L_ctermid	9
#define L_cuserid	9

#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#define P_tmpdir	"/usr/tmp/"
#define L_tmpnam	(sizeof(P_tmpdir)+15)
#else
#define L_tmpnam	24
#endif

#endif	/* _STDIO_H_ */
