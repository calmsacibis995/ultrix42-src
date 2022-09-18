#ifndef lint
static char *sccsid = "@(#)accton.c	4.1	(ULTRIX)	7/2/90";
#endif lint
/*
 * Based on:
 * static char *sccsid = "@(#)accton.c	4.1 (Berkeley) 10/1/80";
 */
main(argc, argv)
char **argv;
{
	extern errno;
	if (argc > 1)
		acct(argv[1]);
	else
		acct((char *)0);
	if (errno) {
		perror("accton");
		exit(1);
	}
	exit(0);
}
