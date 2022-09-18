/* Copyright (C) BULL, Digital Equipment Co., SIEMENS 1986,1987 */
#ifndef lint
static char Sccsid[] = "@(#)mk_lpath.c	4.1 (ULTRIX) 7/3/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
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

/*
 * ini_lpath --  init mk_lpath() routine
 *
 * int ini_lpath(env_var, def_path)
 *	char *env_var;		environ variable to work on
 *	char *def_path;		default path to use if env_var is empty
 *
 * IMPLEMENTATION DETAILS:
 *	reads the given env_var (via getenv()) and prepares module to
 *	work with the contens of this variable.
 *	if variable doesn't exist, def_path is taken to work on
 *	else def_path is remembered to be used, if the value
 *	of path contains no valid path (see mk_lpath()).
 *
 * RETURNS:
 *	0:	env_var contains a valid path
 *     -1:	env_var does not contain a valid path, setting 
 *		path to default path.
 *
 */

/*
 * mk_lpath --  process the varible given by ini_lpath
 * 		for the syntax see X/OPEN Poratbility Guide
 * 		ENVIRON(5) (page 5.2/5.1)
 *
 * int mk_lpath(lang, terr, code, name, archive, ret_nam)
 * 	char *lang;	LAN of "$LANG"
 * 	char *terr;	TER of "$LANG"
 * 	char *code;	COD of "$LANG"
 * 	char *name;	program name
 * 	char *archive;	archive name
 * 	char *ret_nam;	buffer to return found name 
 *			  a length of PATH_MAX + 1 is assumed 
 *
 * IMPLEMENTATION:
 *	each call gives you the next name of the path
 *	until the whole path is recognised.
 *	If called again, the defaultpath is used.
 *	Then you'll get ENDOFPATH.
 *	the name is returned in the buffer 'ret_nam'
 *	you have to be shure that this buffer is
 *	of the size PATH_MAX + 1.
 *
 *	if 'archive' is not a (char *)0 only PATHS
 *	containing an '%A' are recognised.
 *
 *
 * RETURNS:
 *	0:		ret_nam contains a valid name
 *	ENDOFPATH (-1)	whole path is already done
 *	BADCALL   (-2)	no init before
 *	NOCORE    (-3)	ret_nam would exeed the size of the buffer
 *
 * BUGS:
 *	may be NOCORE is returned one char to early
 *
 */

/* 
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * 002	David Lindner Mon Dec 11 13:43:34 EST 1989
 *	- Modified ini_lpath so it would return CORRECT success or failure
 *	  flag.
 *
 * 001	David Lindner Tue Oct 31 10:09:54 EST 1989
 *	- Modified ini_lpath so it would return a success or failure flag.
 *	- Modified comment header.
 *
 */


#include <limits.h>

#ifdef NOVOID
typedef char void;			/* for Siemens MX (M80S30)	*/
#endif

#define GOT_IT		0x01		/* status one msgnam is ready	*/
#define END		0x02		/* status path is empty		*/
#define EOS		'\0'		/* end of string 		*/

/*
 * return values of _mk_langpath
 */
#define ENDOFPATH	-1
#define BADCALL		-2
#define NOCORE		-3

static char *path = (char *)0;	/* variable to hold current pos in path */

int
ini_lpath(env_var, def_path)	/* DJL 001 */
char *env_var;		/* environ variable to work on	*/
char *def_path;		/* default path to use if env_var is empty */
{
    char *getenv();

    path = getenv(env_var);

    if ((path == (char *)0) || (*path == EOS)) {
	path = def_path;
	return -1;
    }
    return 0;
}

int
mk_lpath(lang, terr, code, name, archive, ret_nam)
char *lang;	/* LAN of "$LANG"	*/
char *terr;	/* TER of "$LANG"	*/
char *code;	/* COD of "$LANG"	*/
char *name;	/* program name		*/
char *archive;	/* archive name		*/
char *ret_nam;	/* buffer to return found name 
		   a length of PATH_MAX + 1 is assumed */
{

    /*
     * locally useful variables:
     */
    register char *cp;
    register int len;
    register int n;
    int arch_stat;
    int status;
    int fall_throug;
    char *strcpy();

    /*
     * point to restart routine when unsuccesful because of archives
     */
restart:

    /*
     * initialisation of the local variables 
     */
    cp = ret_nam;
    len = 0;
    arch_stat = 0;
    status = 0;
    fall_throug = 0;

    /*
     * check for valid path
     */
    if (path == (char *)0)
	return BADCALL;
    
    /*
     * check if already at end
     */
    if (*path == EOS)
	    return ENDOFPATH;

    /*
     * loop until valid path is found
     *
     * for the syntax see X/OPEN Poratbility Guide
     * ENVIRON(5) (page 5.2/5.1) + extensions
     */

    for (;;) {
	switch(*path) {
	case '\0':
	case ':':
	    status = GOT_IT;
	    if (cp == ret_nam &&  name != (char *)0) {
		/*
		 * added to get the name for one ':'
		 */
		n = strlen(name);
		if ((len += n) > PATH_MAX)
		    return NOCORE;
		(void)strcpy(cp, name);
		cp += n;
	    }
	    *cp = '\0';
	    break;

	case '%':
	    switch(*++path) {
	    case '\0':
	    case ':':
		status = GOT_IT;
		*cp++ = '%';
		*cp = '\0';
		len++;
		break;

	    case 'A': /* archive */
		arch_stat = 1;
		if (archive != (char *)0) {
		    n = strlen(archive);
		    if ((len += n) > PATH_MAX)
			return NOCORE;
		    (void)strcpy(cp, archive);
		    cp += n;
		}
		break;

	    case 'N':
		if (name != (char *)0) {
		    n = strlen(name);
		    if ((len += n) > PATH_MAX)
			return NOCORE;
		    (void)strcpy(cp, name);
		    cp += n;
		    break;
		}

	    /*
	     * ATTENTION:
	     *
	     * don't change the order of this cases.
	     *
	     * 'L' is build fo 'l'_'t'.'c'
	     *  see cases below
	     */
	    case 'L':
		fall_throug = 1;

	    case 'l':
		if (lang != (char *)0) {
		    n = strlen(lang);
		    if ((len += n) > PATH_MAX) {
			return NOCORE;
		    }
		    (void)strcpy(cp, lang);
		    cp += n;
		}
		if (!fall_throug)
		    break;
		
	    case 't':
		if (terr != (char *)0) {
		    if (fall_throug && *terr) {
			*cp++ = '_';
			len++;
		    }

		    n = strlen(terr);
		    if ((len += n) > PATH_MAX)
			return NOCORE;
		    (void)strcpy(cp, terr);
		    cp += n;
		}
		if (!fall_throug)
		    break;

	    case 'c':
		if (code != (char *)0) {
		    if (fall_throug && *code) {
			*cp++ = '.';
			len++;
		    }

		    n = strlen(code);
		    if ((len += n) > PATH_MAX)
			return NOCORE;
		    (void)strcpy(cp, code);
		    cp += n;
		}
		fall_throug = 0;
		break;

	    default:
		*cp++ = '%';
		n++;
		break;
	    }
	    break;

	default:
	    *cp++ = *path;
	    len++;
	    break;
	}

	/*
	 * continue creating ret_nam but don't trample on nuls
	 */
	if (*path)
		path++;

	if (status == GOT_IT) {
	    if (archive == (char *)0 && arch_stat) {
		/*
		 * path contained a %A but archive was not valid
		 * skip to next element of path
		 */
		goto restart;
	    }
	    else {
		return 0;
	    }
	}
	else if (len >= PATH_MAX)
	    return NOCORE;
	

    } /* end of for loop */
}
