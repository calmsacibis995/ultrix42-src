#ifndef lint
static	char	*sccsid = "@(#)dosys.c	4.1	(ULTRIX)	7/17/90";
#endif lint

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
/*static	char *sccsid = "@(#)dosys.c	4.7 (Berkeley) 83/06/22";*/
#include "defs"
#include <signal.h>

dosys(comstring,nohalt)
register char *comstring;
int nohalt;
{
register int status;

if(metas(comstring))
	status = doshell(comstring,nohalt);
else	status = doexec(comstring);

return(status);
}



metas(s)   /* Are there are any  Shell meta-characters? */
register char *s;
{
register char c;

while( (funny[c = *s++] & META) == 0 )
	;
return( c );
}

doshell(comstring,nohalt)
char *comstring;
int nohalt;
{
#ifdef SHELLENV
char *getenv(), *rindex();
char *shellcom = getenv("SHELL");
char *shellstr;
#endif
if((waitpid = vfork()) == 0)
	{
	enbint(SIG_DFL);
	doclose();

#ifdef SHELLENV
	if (shellcom == 0) shellcom = SHELLCOM;
	shellstr = rindex(shellcom, '/') + 1;
	execl(shellcom, shellstr, (nohalt ? "-c" : "-ce"), comstring, 0);
#else
	execl(SHELLCOM, "sh", (nohalt ? "-c" : "-ce"), comstring, 0);
#endif
	fatal("Couldn't load Shell");
	}

return( await() );
}




int intrupt();

await()
{
int status;
register int pid;

enbint(SIG_IGN);
while( (pid = wait(&status)) != waitpid)
	if(pid == -1)
		fatal("bad wait code");
waitpid = 0;
enbint(intrupt);
return(status);
}






doclose()	/* Close open directory files before exec'ing */
{
register struct dirhdr *od;

for (od = firstod; od; od = od->nxtopendir)
	if (od->dirfc != NULL) {
		/* This was messing up the parents tables */
		/* closedir(od->dirfc);
		od->dirfc = NULL; */
		(void) close(od->dirfc->dd_fd);
	}
}


int maxargv = 400;
char **argv = NULL;

doexec(str)
register char *str;
{
register char *t;
register char **p;

if(argv == NULL){
    if((argv = (char **) malloc(maxargv * sizeof(char *))) == NULL){
	perror("make");
	fatal1("doexec: malloc failed");
    }
}
while( *str==' ' || *str=='\t' )
	++str;
if( *str == '\0' )
	return(-1);	/* no command */

p = argv;
for(t = str ; *t ; )
	{
	if (p >= argv + maxargv){
	    	if((argv = (char **) realloc(argv, 2*maxargv*sizeof(char *))) == NULL){
		    perror("make");
		    fatal1("doexec: realloc failed");
		}
		p = argv + maxargv;
		maxargv *= 2;
	}
	*p++ = t;
	while(*t!=' ' && *t!='\t' && *t!='\0')
		++t;
	if(*t)
		for( *t++ = '\0' ; *t==' ' || *t=='\t'  ; ++t)
			;
	}

*p = NULL;

if((waitpid = vfork()) == 0)
	{
	enbint(SIG_DFL);
	doclose();
	enbint(intrupt);
	execvp(str, argv);
	fatal1("Cannot load %s",str);
	}

return( await() );
}

#include <errno.h>

#include <sys/stat.h>



touch(force, name)
int force;
char *name;
{
struct stat stbuff;
char junk[1];
int fd;

if( stat(name,&stbuff) < 0)
	if(force)
		goto create;
	else
		{
		fprintf(stderr, "touch: file %s does not exist.\n", name);
		return;
		}

if(stbuff.st_size == 0)
	goto create;

if( (fd = open(name, 2)) < 0)
	goto bad;

if( read(fd, junk, 1) < 1)
	{
	close(fd);
	goto bad;
	}
lseek(fd, 0L, 0);
if( write(fd, junk, 1) < 1 )
	{
	close(fd);
	goto bad;
	}
close(fd);
return;

bad:
	fprintf(stderr, "Cannot touch %s\n", name);
	return;

create:
	if( (fd = creat(name, 0666)) < 0)
		goto bad;
	close(fd);
}
