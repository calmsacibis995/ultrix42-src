#ifndef lint
static        char    *sccsid = "@(#)hostname.c	4.2  (ULTRIX)        10/12/90";
#endif
/*
 * hostname -- get (or set hostname)
 */
#include <stdio.h>

char hostname[32];
extern int errno;

main(argc,argv)
	char *argv[];
{
	int	myerrno;

	argc--;
	argv++;
	if (argc) {
		if (sethostname(*argv,strlen(*argv)))
			perror("sethostname");
		myerrno = errno;
	} else {
		gethostname(hostname,sizeof(hostname));
		myerrno = errno;
		/* use write instead of printf (keep image small) */
		write(1, hostname, strlen(hostname));
		write(1, "\n", strlen("\n"));
	}
	exit(myerrno);
}
