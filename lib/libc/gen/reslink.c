#ifndef lint
static char *sccsid = "@(#)reslink.c	4.1  (ULTRIX) 7/3/90";
#endif

/*
 *			Copyright (c) 1988 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
 */


#include  <sys/types.h>
#include  <sys/stat.h>
#include  <sys/param.h>
#include  <stdio.h>

typedef struct visit {
	struct visit	*v_next;
	char		v_path[MAXPATHLEN];
	int		v_cnt;
} visit_t;


/**********************************************************************
*
*  Original - 06 November 87,	Teoman Topcubasi,
*				Charley Bennett
*
*  This subroutine will check for symbolic link references and replace
*  them with the file name contained within the link!  It is written 
*  recursively, so that the resolved name is also checked again.
*  
*  This is called from utilities like editors in handling file names 
*  
**********************************************************************/

static visit_t *visits = NULL;


/**********************************************************************
*
*  getdir(f,j) will  extract the directory portion within a pathname
*  
*  input parameter: char *f	contains the pathname
*  output parameters: int *j	contains the index to the last element 
*				of the path which identifies the
*				directory
*  return value:  char *	contains the part of the path specifying
*				the directory.
**********************************************************************/

static  char  *getdir(f,j)
   char	*f;
   int  *j;
	{
	char tmp[MAXPATHLEN], *k;

	k = (char *) rindex(f,'/');
	if (k == 0)
	   return(NULL);
	strcpy (tmp,k);		/* now it contains the remainder after dir. */
	*j = strlen (tmp);
	*j = strlen(f) - *j;
	strncpy (tmp, f, *j);
	tmp[*j] = '\0';
	return(tmp);
	}

/***********************************************************************
*
*  makevisit(v,path) will record a visit to each file name encountered
*  in the link chain.
*
*  input parameter: visit_t *v	a pointer to a linked list or NULL
*  		    char *path	present pathname to be resolved
*  return value:  visit_t *	a pointer to the newest element in the
*				linked list.
************************************************************************/

static visit_t *makevisit(v,path)
visit_t *v;
char *path;
{
	visit_t	*t;

	t = (visit_t *) malloc( sizeof(visit_t) );
	strcpy( t->v_path, path );
	if( v )
		t->v_cnt = v->v_cnt + 1;
	else
		t->v_cnt = 1;
	t->v_next = v;
	return(t);
}

/**********************************************************************
*
*  checkvisit(v,p) will recursively look if the path to be resolved 
*  has already been visited by reslink or not.
*
*  input parameter: visit_t *v	contains a pointer to the linked list of
*				visited file names.
*		    char *path	contains the unresolved pathname
*  return value:  visit_t *	either 0 if list is NULL or
*				-1 if file name already encountered.
***********************************************************************/

static visit_t *checkvisit(v,path)
visit_t *v;
char *path;
{
	if( v == NULL )
		return(0);

	if( !strcmp( v->v_path, path ) )
		return((visit_t *) -1);

	return( checkvisit(v->v_next,path) );
}

/***********************************************************************
*
*  freevisit(v) will free the dynamic linked list of file names which 
*  have been visited by reslink.
*
*  input/output parameters: visit_t *v	contains a pointer to a list
*  return value:  visit_t *	always a NULL.
*
***********************************************************************/

static visit_t *freevisits(v)
visit_t *v;
{
	visit_t	*t;

	if( v == NULL )
		return(NULL);

	t = v->v_next;
	free(v);
	return(freevisits(t));
}

/***********************************************************************
*
*  reslink(pathname) will resolve a file name which could be a soft link
*  to its absolute value.  It is a recursive function and has been
*  written with the intend of fixing old software which don't tolerate 
*  softly linked files.
*
*	Example:	j --> /etc/fstab
*			(here j is a link pointing to /etc/fstab)
*			crontab --> ../../etc/crontab
*			(here crontab is a link pointing to a file
*			relative to its present location in the file 
*			system hierarchy)
*
*  input parameter:  char *pathname	contains any file name
*  return value:  char *		contains the resolved absolute
*					pathname referred by the link.
*
************************************************************************/

char  *reslink (pathname)
char  *pathname;
{

	struct stat Lc;
	static visit_t  *visit = NULL;
	int rc;
	int i, j=0;
	char  *wd,
	      buf[MAXPATHLEN],
	      fname[MAXPATHLEN],
	      *cwd, *dirname, *temp;

	lstat(pathname, &Lc);	/* check if it is a link */
	if ((Lc.st_mode & S_IFMT) != S_IFLNK)
	{
		if (visits->v_cnt > 1)
		    chdir(cwd);
		visits = (visit_t *)freevisits(visits);
		return(pathname);
	}
	else			/* yes it is! */
	{			/* read the link */
	   if ((rc = readlink(pathname, buf, MAXPATHLEN)) < 0)
		{
		if (visits->v_cnt > 1)
		    chdir(cwd);
			visits = (visit_t *) freevisits(visits);
			return(NULL);
		}
	   if (!strcmp(pathname,buf))	/* check if link points to */
		{			/* itself */
		if (visits->v_cnt > 1)
		    chdir(cwd);
			visits = (visit_t *) freevisits(visits);
			return(NULL);
		}


	 /* save cwd */
           if (!visits)
		if ((cwd =  (char *)getcwd((char *)NULL, MAXPATHLEN)) == NULL)
		{
			visits = (visit_t *) freevisits(visits);
			return(NULL);
		}

 	 /* check for directories in input path */
		
	   if (dirname = getdir(pathname,&i))   /* save dir name */
	   {
	   	if ((rc = chdir(dirname)) < 0)
	   	{
		   if (visits->v_cnt > 1)
		       chdir(cwd);
			   visits = (visit_t *) freevisits(visits);
			   return(NULL);
	   	}
	   	while (pathname[i++] != '\0')   /* save file name */
	   	{
			fname[j++] = pathname[i];
	   	}
	   	fname[j] = pathname[i];
		strcpy (pathname, fname);
	   }

	   if (buf[0] == '/')	/* link name absolute */
	   {
		strcpy(pathname, buf);
		if( checkvisit( visits, pathname ) )
		{
		if (visits->v_cnt > 1)
		    chdir(cwd);
			visits = (visit_t *) freevisits(visits);
			return(NULL);
		}
		if (visit)
		    if (visit->v_cnt > MAXSYMLINKS)
		        return (NULL);
		visits = (visit_t *) makevisit( visit, pathname );
		reslink(pathname);
	   }
	   else 		/* link name relative */
	   {
		if (pathname[0] != '/')	/* file is relative */
		{
		   if ((wd = (char *) getcwd((char *)NULL, MAXPATHLEN)) == NULL)
		   {
		     if (visits->v_cnt > 1)
		        chdir(cwd);
		     visits = (visit_t *)freevisits(visits);
		     return(NULL);
		   }
		   else
		   {
			strncat(wd, "/", 1);
			strcat(wd, buf);
			strcpy(pathname, wd);
		   }
		}
		/* file name is full path */
		
		dirname = getdir(pathname,&i);    /* save dir name */
		if ((rc = chdir(dirname)) < 0)
		{
		if (visits->v_cnt > 1)
		    chdir(cwd);
			visits = (visit_t *) freevisits(visits);
			return(NULL);
		}
		j = 0;
		while (pathname[i++] != '\0')   /* save file name */
		{
			fname[j++] = pathname[i];
		}
		fname[j] = pathname[i];
		if (0 == getwd(wd))
		{
		if (visits->v_cnt > 1)
		    chdir(cwd);
			visits = (visit_t *)freevisits(visits);
			return(NULL);
		}
		else
		{
			strncat(wd, "/", 1);
			strcat(wd, fname);
			strcpy(pathname, wd);
		}
		if( checkvisit (visits,pathname) )
		{
		if (visits->v_cnt > 1)
		    chdir(cwd);
			visits = (visit_t *) freevisits(visits);
			return(NULL);
		}
		if (visit)
		    if (visit->v_cnt > MAXSYMLINKS)
		    {
			chdir(cwd);
		        return (NULL);
		    }
		visits = (visit_t *) makevisit(visit, pathname);
		reslink(pathname);
	   }
    }
}
