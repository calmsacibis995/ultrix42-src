/*
 *		@(#)strings.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1989 by			*
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
 *			Modification History				*
 *									*
 *	David L Ballenger, 28-Mar-1985					*
 * 0001	Add definitions for System V string routines			*
 *									*
 *	Jon Reeves, 1989 May 30						*
 * 002	Add ANSI, POSIX, X/Open mandated changes (alphabetized		*
 *	existing declarations, too)					*
 *									*
 ************************************************************************/


#ifndef	_SIZE_T_
#define	_SIZE_T_
typedef	unsigned int	size_t;		/* type of sizeof */
#endif	/* _SIZE_T_ */

/*
	Null pointer.
 */
#define	NULL 0

#ifdef __STDC__
/*
 *  prototypes
 *
*/

extern char
	*strcat( char *__s1, const char *__s2 ),	
	*strchr( const char *__s, int __c ),
	*strcpy( char *__s1, const char *__s2 ),
	*strerror( int __n ),		
	*strncat( char *__s1, const char *__s2, size_t __n ),
	*strncpy( char *__s1, const char *__s2, size_t __n ),
	*strpbrk(const char *__s1, const char *__s2 ),	
	*strrchr( const char *__s, int __c ),
	*strstr( const char *__s, const char *__t ),
	*strtok( char *__s1, const char *__s2 );	

extern int
	memcmp( const void *__s1, const void *__s2, size_t __n ),
	strcmp( const char *__s1, const char *__s2 ),
	strcoll( const char *__s1, const char *__s2 ),
	strncmp( const char *__s1, const char *__s2, size_t __n );

extern void
	*memchr( const void *__s, int __c, size_t __n ),	
	*memcpy( void *__s1, const void *__s2, size_t __n ),
	*memmove( void *__s1, const void *__s2, size_t __n ),
	*memset( void *__s, int __c, size_t __n );

extern size_t
	strcspn( const char *__s1, const char *__s2 ),
	strlen( const char *__s ),
	strspn( const char *__s1, const char *__s2 ),
	strxfrm( char *__to, const char *__from, size_t __maxsize  );

#else
/*
	Function declarations.
 */
extern char
	*index(),
	*rindex(),
	*strcpy(),
	*strcat(),
	*strchr(),
	*strerror(),
	*strncat(),
	*strncpy(),
	*strpbrk(),
	*strrchr(),
	*strstr(),
	*strtok();
extern int
	memcmp(),
	strcmp(),
	strcoll(),
	strncmp();
extern size_t
	strcspn(),
	strlen(),
	strspn(),
	strxfrm();
extern void
	*memccpy(),	/* X/Open */
	*memchr(),
	*memcpy(),
	*memmove(),
	*memset();
#endif /* __STDC__ */
