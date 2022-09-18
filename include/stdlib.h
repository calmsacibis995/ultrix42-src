/*	@(#)stdlib.h	4.2	(ULTRIX)	9/4/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *	Jon Reeves, 1989 May 10
 * 001	Created to satisfy ANSI standard
 *
 * 002	jlr, 1989 June 02
 *	Make multi-include protection finer grained for typedefs
 *
 *      Dan Smith, 1990 Feb 21
 * 003  Added const to various prototypes. Change wchar_t to unsigned
 *      int and update MB_CUR_MAX to 4.
 ************************************************************************/

#include <ansi_compat.h>
#ifndef	_STDLIB_H_
#define	_STDLIB_H_

#ifndef _SIZE_T_
#define _SIZE_T_
typedef	unsigned int	size_t;		/* type of sizeof */
#endif

#ifndef _WCHAR_T_
#define _WCHAR_T_
typedef	unsigned int	wchar_t;	/* size of largest character */
#endif

typedef struct {
		int	quot;	/* quotient */
		int	rem;	/* remainder */
	}	div_t;		/* result of div() */

typedef struct {
		long	quot;	/* quotient */
		long	rem;	/* remainder */
	}	ldiv_t;		/* result of ldiv() */

/*
	Null pointer.
 */
#define	NULL 0

/*
	Arguments to exit().
 */
#define	EXIT_FAILURE 1
#define	EXIT_SUCCESS 0

/*
	Range of rand().
 */
#ifdef	__SYSTEM_FIVE
#define	RAND_MAX 32767
#else
#define	RAND_MAX 2147483647
#endif

/*
	Current multibyte-character size.
	Since wchar_t is unsigned int MB_CUR_MAX is 4 octets
 */
#define	MB_CUR_MAX 4

#ifdef __STDC__
/*
 *  prototypes
 *
 */

void		abort( void );	
int		abs( int __i );
int		atexit( void (*__func)() );
double		atof( const char *__nptr );	
int		atoi( const char *__nptr );
long		atol( const char *__nptr );	
void *		bsearch( const void *__key, const void *__base,
			size_t __nel, size_t __size ,
			int (*__compar)(const void *, const void *) );
void *		calloc( size_t __nelem, size_t __size );
div_t		div( int __numer, int __denom );	
void		exit( int __status );	
void		free( void *__ptr );
char *		getenv( const char *__name );	
long		labs( long __j );
ldiv_t		ldiv( long __numer, long __denom );
void *		malloc( size_t __size );	
int		mblen(const char *__s, size_t __n); 
size_t		mbstowcs( wchar_t *__pwcs, const char *__s, size_t __n );
int		mbtowc(  wchar_t *__pwc, const char *__s, size_t __n );
void		qsort( void *__base, size_t __nel, size_t __width,
		      int (*__compar)(const void *, const void *) );
int		rand( void );		
void *		realloc( void *__ptr, size_t __size );
void		srand( unsigned int __seed );	
double		strtod( const char *__nptr, char **__eptr );
long		strtol( const char *__nptr, char **__eptr, int __base );
unsigned long	strtoul( const char *__nptr, char **__eptr, int __base );
int		system( const char *__string );
size_t		wcstombs( char *__s, const wchar_t *__pwcs, size_t __n );
int		wctomb( char *__s, wchar_t __wchar);

#else

/*
	Function declarations.
 */
/*	String conversion.	*/
double	atof();
int	atoi();
long	atol();
double	strtod();
long	strtol();
unsigned long	strtoul();

/*	Pseudo-random sequence generation.	*/
int	rand();
void	srand();

/*	Memory management.	*/
void	*calloc();
void	free();
void	*malloc();
void	*realloc();

/*	Communication with the environment.	*/
void	abort();
int	atexit();
void	exit();
char	*getenv();
int	system();

/*	Searching and sorting.	*/
void	*bsearch();
void	qsort();

/*	Integer arithmetic.	*/
int	abs();
div_t	div();
long	labs();
ldiv_t	ldiv();

/*	Multibyte characters.	*/
int	mblen();
int	mbtowc();
int	wctomb();
size_t	mbstowcs();
size_t	wcstombs();

#endif /* __STDC__ */
#endif
