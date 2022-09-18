
#ifndef lint
static	char	*sccsid = "@(#)mknod.c	4.2	(ULTRIX)	10/12/90";
#endif lint

#include <sys/types.h>						/* 3 */
#include <sys/stat.h>						/* 3 */
#define FILEMODE	0666					/* 3 */

/* History -							*/
/*	1) - added code to handle the 'p' flag for creating	*/
/*		ports (named pipes).				*/
/*		LeRoy Fundingsland    9/26/84    DEC		*/
/*	2) - minor bug fix					*/
/*		LeRoy Fundingsland    3/18/85    DEC		*/
/*	3) - minor code readability change			*/
/*		LeRoy Fundingsland    4/11/85    DEC		*/

#define ARG_MESS "arg count\n"
#define USAGE_MESS "usage: mknod name b/c major minor\n               or\n       mknod name p\n"

main(argc, argv)
    int argc; char **argv;{
	int m, a, b;

	if(argc != 5  &&  argc != 3){
		/* use write instead of printf (keep it small) */
		/* printf("arg count\n"); */
		write(1, ARG_MESS, strlen(ARG_MESS));
		goto usage;
	}
	if(argc == 3  &&  *argv[2] != 'p')
		goto usage;

	if(*argv[2] == 'p'){		/* port special file	*/
		if(argv[2][1] != 0)				/* 2 */
			goto usage;				/* 2 */
		m = S_IFPORT | FILEMODE;			/* 3 */
		a = 0;
		b = 0;
	}
	else{
		if(*argv[2] == 'b')	/* block special file	*/
			m = S_IFBLK | FILEMODE;			/* 3 */
		else if(*argv[2]=='c')	/* char special file	*/
			m = S_IFCHR | FILEMODE;			/* 3 */
		else
			goto usage;
		a = number(argv[3]);
		if(a < 0)
			goto usage;
		b = number(argv[4]);
		if(b < 0)
			goto usage;
	}

	if(mknod(argv[1], m, (a<<8)|b) < 0)
		perror("mknod");
	exit(0);

usage:
	/* use write instead of printf (keep it small) */
	/***
	printf("usage: mknod name b/c major minor\n");
	printf("              or\n");
	printf("       mknod name p\n");
	***/
	write(1, USAGE_MESS, strlen(USAGE_MESS));
}

number(s)
char *s;
{
	int n, c;

	n = 0;
	while(c = *s++) {
		if(c<'0' || c>'9')
			return(-1);
		n = n*10 + c-'0';
	}
	return(n);
}
