#ifndef lint
static CHTYPE *sccsid = "@(#)pwd.c	4.2 (ULTRIX) 8/13/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 *
 *   Modification History:
 *
 * 003 - David Lindner, Thu Aug  9 14:06:37 EDT 1990
 *		Fixed pwd so it always returns the correct directory.
 *
 * 002 - Gary A. Gaudet, Wed Nov  9 10:24:49 EST 1988
 *	 	MIPS portability and bug fixes
 *		MAXPWD is PATH_MAX from limit.h
 *
 * 001 - Gary Gaudet for Andy Gadsby 09-mar-88
 *		i18n version of csh
 *
 *
 *
 */

/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#include	"mac.h"
#include	<limits.h>	/* GAG - for PATH_MAX */

#define	DOT		'.'
#define	NULL	0
#define	SLASH	'/'
#define MAXPWD	PATH_MAX	/* GAG */

extern CHTYPE	longpwd[];

static CHTYPE   cwdname[MAXPWD];
static int 	didpwd = FALSE;

cwd(dir)
	register CHTYPE *dir;
{
	register CHTYPE *pcwd;
	register CHTYPE *pdir;

	/* First remove extra /'s */

	rmslash(dir);

	/* Now remove any .'s */

	pdir = dir;
	while(*pdir) 			/* remove /./ by itself */
	{
		if((*pdir==DOT) && (*(pdir+1)==SLASH))
		{
			movstr(pdir+2, pdir);
			continue;
		}
		pdir++;
		while ((*pdir) && (*pdir != SLASH)) 
			pdir++;
		if (*pdir) 
			pdir++;
	}
	if(pdir>dir && *(--pdir)==DOT && pdir>dir && *(--pdir)==SLASH)	/* DAG -- bug fix (added first test) */
		*pdir = '\0';	/* DAG -- not NULL */
	

	/* Remove extra /'s */

	rmslash(dir);

	/* Now that the dir is canonicalized, process it */

	if(*dir==DOT && *(dir+1)==NULL)
	{
		return;
	}

	if(*dir==SLASH)
	{
		/* Absolute path */

		pcwd = cwdname;
		didpwd = TRUE;
	}
	else
	{
		/* Relative path */

		if (didpwd == FALSE) 
			return;
			
		pcwd = cwdname + length(cwdname) - 1;
		if(pcwd != cwdname+1)
		{
			*pcwd++ = SLASH;
		}
	}
	while(*dir)
	{
		if(*dir==DOT && 
		   *(dir+1)==DOT &&
		   (*(dir+2)==SLASH || *(dir+2)==NULL))
		{
			/* Parent directory, so backup one */

			if( pcwd > cwdname+2 )
				--pcwd;
			while(*(--pcwd) != SLASH)
				;
			pcwd++;
			dir += 2;
			if(*dir==SLASH)
			{
				dir++;
			}
			continue;
		}
		*pcwd++ = *dir++;
		while((*dir) && (*dir != SLASH))
			*pcwd++ = *dir++;
		if (*dir) 
			*pcwd++ = *dir++;
	}
	*pcwd = '\0';	/* DAG -- not NULL */

	--pcwd;
	if(pcwd>cwdname && *pcwd==SLASH)
	{
		/* Remove trailing / */

		*pcwd = '\0';	/* DAG -- not NULL */
	}
	return;
}

/*
 *	Print the current working directory.
 *	DJL 003
 */

cwdprint()
{
	pwd();
	prs_buff(cwdname);
	prc_buff(NL);
	return;
}

/*
 *	This routine will remove repeated slashes from string.
 */

static
rmslash(string)
	CHTYPE *string;
{
	register CHTYPE *pstring;

	pstring = string;
	while(*pstring)
	{
		if(*pstring==SLASH && *(pstring+1)==SLASH)
		{
			/* Remove repeated SLASH's */

			movstr(pstring+1, pstring);
			continue;
		}
		pstring++;
	}

	--pstring;
	if(pstring>string && *pstring==SLASH)
	{
		/* Remove trailing / */

		*pstring = NULL;
	}
	return;
}

/*
 *	Find the current directory the hard way.
 */

#include	<sys/types.h>
#include	<sys/dir.h>
#include	<sys/stat.h>


static CHTYPE *dotdots =
"../../../../../../../../../../../../../../../../../../../../../../../..";

extern struct direct	*readdir();
extern CHTYPE		*movstrn();

static
pwd()
{
	struct stat		cdir;	/* current directory status */
	struct stat		tdir;
	struct stat		pdir;	/* parent directory status */
	DIR			*pddd;	/* parent directory dir descriptor */

	struct direct	*dir;
			/* 72 is (sizeof(dotdots) / sizeof(CHTYPE)) */
	CHTYPE 		*dot = dotdots + 72 - 3;
	int		index = 72 - 2;
	int		cwdindex = MAXPWD - 1;
	int 		i;
	
	cwdname[cwdindex] = 0;
	dotdots[index] = 0;

	if(stat(wtoc(dot), &pdir) < 0)
	{
		error("pwd: cannot stat .");
	}

	dotdots[index] = '.';

	for(;;)
	{
		cdir = pdir;

		if ((pddd = opendir(wtoc(dot))) == NULL)     /* test */
		{
			error("pwd: cannot open ..");
		}

		if(fstat(pddd->dd_fd, &pdir) < 0)
		{
			closedir(pddd);
			error("pwd: cannot stat ..");
		}

		if(cdir.st_dev == pdir.st_dev)
		{
			if(cdir.st_ino == pdir.st_ino)
			{
				didpwd = TRUE;
				closedir(pddd);
				if (cwdindex == (MAXPWD - 1))
					cwdname[--cwdindex] = SLASH;

				movstr(&cwdname[cwdindex], cwdname);
				return;
			}

			do
			{
				if ((dir = readdir(pddd)) == 0)
				{
					closedir(pddd); 
					reset_dir();
					error("pwd: read error in ..");
				}
			}
			while (dir->d_ino != cdir.st_ino);
		}
		else
		{
			CHTYPE name[256+MAXPWD];	/* DAG -- (was 512) */
			
			movstr(dot, name);
			i = length(name) - 1;

			name[i++] = '/';

			tdir.st_dev = pdir.st_dev;	/* DAG -- (safety) */
			do
			{
				if ((dir = readdir(pddd)) == 0)
				{
					closedir(pddd); 
					reset_dir();
					error("pwd: read error in ..");
				}
				*(movstrn(ctow(dir->d_name), &name[i],MAXNAMLEN)) = 0;
				stat(wtoc(name), &tdir);
			}		
			while(tdir.st_ino != cdir.st_ino || tdir.st_dev != cdir.st_dev);
		}
		closedir(pddd);  
		reset_dir();

		for (i = 0; i < MAXNAMLEN; i++)
			if (dir->d_name[i] == 0)
				break;

		if (i > cwdindex - 1)
				error(longpwd);
		else
		{
			cwdindex -= i;
			movstrn(ctow(dir->d_name), &cwdname[cwdindex], i);
			cwdname[--cwdindex] = SLASH;
		}

		dot -= 3;
		if (dot<dotdots) 
			error(longpwd);
	}
}
