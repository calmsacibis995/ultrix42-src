#ifndef lint
static	char	*sccsid = "@(#)cklock.c	4.1 (ULTRIX)	7/2/90";
#endif lint

/* cklock.c will create a file /tmp/dmslock so that only one
 *  /etc/dms will be run at one time.  The pid is put into the 
 *  lock file so that it can be checked against dead processes.
 *  To remove the lock file dms calls cklock with DMSLCOFF.
*/	
#include <stdio.h>
#include <errno.h>
extern int errno;

#define DMSLCON  '1'	/* signal /etc/dms as being inuse */
#define DMSLCOFF '0'	/* remove the lock file */

main(argc,argv)
int	argc;
char	*argv[];
{
	FILE *rfp;
	int pidno;
	char *name;

	name=argv[0];
	switch (argv[1][0]) {
	case DMSLCON:

	rfp=fopen("/tmp/dmslock","r");

	if (rfp != NULL) {
		fscanf(rfp,"%d",&pidno);
		fclose(rfp);
		if (kill(pidno,0)== 0)
			exit(-1);
		else if (errno==ESRCH)
			unlink("/tmp/dmslock");
		else exit(-1);	 	/* some other error */
	}

	rfp=fopen("/tmp/dmslock","w");
	if ( !rfp ) {
		printf("%s: Cannot open /tmp/dmslock\n",name);
		exit(-1);
	}
	pidno=getppid();
	fprintf(rfp, "%d", pidno);
	fclose(rfp);
		break;

	case DMSLCOFF:
		unlink("/tmp/dmslock");
		break;
	}
}
