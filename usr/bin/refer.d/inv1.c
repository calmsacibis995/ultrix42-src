#ifndef lint
static	char	*sccsid = "@(#)inv1.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * Modification history
 *
 *  28-Sep-88  D. Long
 *	Wrong number of args were being passed to recopy.  PMAX vararg fix.
 */

/*
static char *sccsid = "@(#)inv1.c	4.1 (Berkeley) 5/6/83";
*/

#include <stdio.h>
#include <assert.h>

main(argc, argv)
char *argv[];
{
	/* Make inverted file indexes.  Reads a stream from mkey which
	 * gives record pointer items and keys.  Generates set of files
	 *	a. NHASH pointers to file b.
	 *	b. lists of record numbers.
	 *	c. record pointer items.
	 *
	 *  these files are named xxx.ia, xxx.ib, xxx.ic;
	 *  where xxx is taken from arg1.
	 *  If the files exist they are updated.
	 */

	FILE *fa, *fb, *fc, *fta, *ftb, *ftc, *fd;
	int nhash = 256;
	int appflg = 1;
	int keepkey = 0, pipein = 0;
	char nma[100], nmb[100], nmc[100], com[100], nmd[100];
	char tmpa[20], tmpb[20], tmpc[20];
	char *remove = NULL;
	int chatty = 0, docs, hashes, fp[2], fr, fw, pfork, pwait, status;
	int i,j,k;
	long keys;
	int iflong =0;
	char *sortdir;

	sortdir = (access("/crp/tmp", 06)==0) ? "/crp/tmp" : "/usr/tmp";
	while (argv[1][0] == '-')
	{
		switch(argv[1][1])
		{
		case 'h': /* size of hash table */
			nhash = atoi (argv[1]+2); 
			break;
		case 'n': /* new, don't append */
			appflg=0; 
			break;
		case 'a': /* append to old file */
			appflg=1; 
			break;
		case 'v': /* verbose output */
			chatty=1; 
			break;
		case 'd': /* keep keys on file .id for check on searching */
			keepkey=1; 
			break;
		case 'p': /* pipe into sort (saves space, costs time)*/
			pipein = 1; 
			break;
		case 'i': /* input is on file, not stdin */
			close(0);
			if (open(argv[2], 0) != 0)
				err("Can't read input %s", argv[2]);
			if (argv[1][2]=='u') /* unlink */
				remove = argv[2];
			argc--; 
			argv++;
			break;
		}
		argc--;
		argv++;
	}
	strcpy (nma, argc >= 2 ? argv[1] : "Index");
	strcpy (nmb, nma);
	strcpy (nmc, nma);
	strcpy (nmd, nma);
	strcat (nma, ".ia");
	strcat (nmb, ".ib");
	strcat (nmc, ".ic");
	strcat (nmd, ".id");

	sprintf(tmpa, "junk%di", getpid());
	if (pipein)
	{
		pipe(fp); 
		fr=fp[0]; 
		fw=fp[1];
		if ( (pfork=fork()) == 0)
		{
			close(fw);
			close(0);
			assert(dup(fr)==0);
			close(fr);
			execl("/bin/sort", "sort", "-T", sortdir, "-o", tmpa, 0);
			execl("/usr/bin/sort", "sort", "-T", sortdir, "-o", tmpa, 0);
			assert(0);
		}
		assert(pfork!= -1);
		close(fr);
		fta = fopen("/dev/null", "w");
		close(fta->_file);
		fta->_file = fw;
	}
	else /* use tmp file */
	{
		fta = fopen(tmpa, "w");
		assert (fta != NULL);
	}
	fb = 0;
	if (appflg )
	{
		if (fb = fopen(nmb, "r"))
		{
			sprintf(tmpb, "junk%dj", getpid());
			ftb = fopen(tmpb, "w");
			if (ftb==NULL)
				err("Can't get scratch file %s",tmpb);
			nhash = recopy(ftb, fb, fopen(nma, "r"), nhash);
			fclose(ftb);
		}
		else
			appflg=0;
	}
	fc = fopen(nmc,  appflg ? "a" : "w");
	if (keepkey)
		fd = keepkey ? fopen(nmd, "w") : 0;
	docs = newkeys(fta, stdin, fc, nhash, fd, &iflong);
	fclose(stdin);
	if (remove != NULL)
		unlink(remove);
	fclose(fta);
	if (pipein)
	{
		pwait = wait(&status);
		printf("pfork %o pwait %o status %d\n",pfork,pwait,status);
		assert(pwait==pfork);
		assert(status==0);
	}
	else
	{
		sprintf(com, "sort -T %s %s -o %s", sortdir, tmpa, tmpa);
		system(com);
	}
	if (appflg)
	{
		sprintf(tmpc, "junk%dk", getpid());
		sprintf(com, "mv %s %s", tmpa, tmpc);
		system(com);
		sprintf(com, "sort -T %s  -m %s %s -o %s", sortdir,
		tmpb, tmpc, tmpa);
		system(com);
	}
	fta = fopen(tmpa, "r");
	fa = fopen(nma, "w");
	fb = fopen(nmb, "w");
	whash(fta, fa, fb, nhash, iflong, &keys, &hashes);
	fclose(fta);
# ifndef D1
	unlink(tmpa);
# endif
	if (appflg)
	{
		unlink(tmpb);
		unlink(tmpc);
	}
	if (chatty)

		printf ("%ld key occurrences,  %d hashes, %d docs\n",
		keys, hashes, docs);
}
