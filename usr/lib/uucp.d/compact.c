#ifndef lint
static char sccsid[] = "@(#)compact.c	4.1 (decvax!larry) 7/2/90";
#endif

/*
 * compact.c
 *
 *	Program to compact the directories
 *	   - creates temp directory, links old files to temp directory,
 *		unlinks old files, renames temp directory to old directory
 *
 *					
 */



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



#include	<stdio.h>
#include	<sys/types.h>
#ifdef NDIR
#include "ndir.h"
#else
#include <sys/dir.h>
#endif
#include	<sys/time.h>
#include	<pwd.h>
#include	<errno.h>
#include	<sys/stat.h>

extern int errno;
char temp[250];

main(argc,argv)
char **argv; int argc;
{
struct direct *dirp, *Cdirp;
	struct passwd *pwd;
	DIR *ufd, *Cfd;
	struct stat statbuff;
	char newname[250];
	char oldname[250];
	char *directory;
	FILE *dirlist;
	char Ddir[250];
	char thissys[20];


	while(argc>1) {
		directory = *++argv;
		argc--; 
		printf("directory to compact is: %s\n", directory);
		compact(directory);
		printf("compaction of: %s  is complete\n", directory);
	}
}

pexit(msg)
char *msg;
{
	fprintf(stderr,"%s, errno=%d\n",msg, errno);
	exit(errno);
}

newdir(ndir)
char *ndir;
{
	if (rmdir(ndir))
		pexit("can not remove directory");
	if (rename(temp, ndir))
		pexit("rename");
}

compact(directory)
char *directory;
{
	char oldname[100];
	char newname[100];
	DIR *ufd;
	struct direct *dirp;
	sprintf(temp,"temp.%s",directory);
	if( (ufd=opendir(directory,"r")) == NULL )
		pexit(directory);
	if( mkdir(temp, 0755) )
		pexit("Can not make temp");
	while( (dirp=readdir(ufd))!=NULL)
	{
		if( dirp->d_ino==(ino_t)0 || strncmp(".", dirp->d_name,1)==0)
			continue;
		sprintf(oldname,"%s/%s",directory,dirp->d_name);
		sprintf(newname,"%s/%s",temp, dirp->d_name);
		if (link(oldname, newname))
			pexit(newname);
		if (unlink(oldname))
			pexit(oldname);
	}
	closedir(ufd);
	newdir(directory);
}

