#ifndef lint
static char sccsid[] = "@(#)uupoll.c	4.1 (decvax!larry) 7/2/90";
#endif

/*
 * Poll named system(s).
 *
 * The poll occurs even if recent attempts have failed,
 * but not if L.sys prohibits the call (e.g. wrong time of day).
 *
 * AUTHOR
 *	Tom Truscott (rti!trt)
 */

#include "uucp.h"

main(argc, argv)
int argc;
char **argv;
{
	chdir(Spool);
	strcpy(Progname, "uupoll");
	uucpname(Myname);
	if (argc < 2) {
		fprintf(stderr, "usage: uupoll system ...\n");
		exit(1);
	}

	for (--argc, ++argv; argc > 0; --argc, ++argv) {
		if (strcmp(argv[0], Myname) == SAME) {
			fprintf(stderr, "This *is* %s!\n", Myname);
			continue;
		}
		if (versys(argv[0])) {
			fprintf(stderr, "%s: unknown system.\n", argv[0]);
			continue;
		}
		/* Attempt the call, -f forces a connect attempt */
		xuucico(argv[0], "-f");
	}
	exit(0);
}

cleanup(code)
int code;
{
	exit(code);
}
