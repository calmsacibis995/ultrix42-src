#ifndef lint
static  char    *sccsid = "@(#)tee.c	4.2  (ULTRIX)        8/13/90";
#endif
/*
 * tee-- pipe fitting
 *
 * Modification History
 *
 * 001	Bob Fontaine Fri Aug 10 10:45:21 EDT 1990
 *
 *	Make tee check that it can write to the file and if not print
 *	an error message.
 */

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define	BUFSIZ	8192
int openf[20] = { 1 };
int n = 1;
int t = 0;
int aflag;

char in[BUFSIZ];

char out[BUFSIZ];

extern errno;
long	lseek();

main(argc,argv)
char **argv;
{
	register int r,w,p;
	int errflg = 0;
	struct stat buf;
	while(argc>1&&argv[1][0]=='-') {
		switch(argv[1][1]) {
		case 'a':
			aflag++;
			break;
		case 'i':
		case 0:
			signal(SIGINT, SIG_IGN);
		}
		argv++;
		argc--;
	}
	fstat(1,&buf);
	t = (buf.st_mode&S_IFMT)==S_IFCHR;
	if(lseek(1,0L,1)==-1&&errno==ESPIPE)
		t++;
	while(argc-->1) {
		errflg = 0;
		if(aflag) {
			openf[n] = open(argv[1],1);
			if(openf[n] < 0)
				openf[n] = creat(argv[1],0666);
			if(openf[n] < 0)
				errflg++;
			lseek(openf[n++],0L,2);
		} else {
			openf[n++] = creat(argv[1],0666);
			if(openf[n-1] < 0)
				errflg++;
			
		}
		if(stat(argv[1],&buf)>=0 && !errflg) {
			if((buf.st_mode&S_IFMT)==S_IFCHR)
				t++;
		} else {
			puts("tee: cannot open ");
			puts(argv[1]);
			puts("\n");
			n--;
		}
		argv++;
	}
	r = w = 0;
	for(;;) {
		for(p=0;p<BUFSIZ;) {
			if(r>=w) {
				if(t>0&&p>0) break;
				w = read(0,in,BUFSIZ);
				r = 0;
				if(w<=0) {
					stash(p);
					exit(0);
				}
			}
			out[p++] = in[r++];
		}
		stash(p);
	}
}

stash(p)
{
	int k;
	int i;
	int d;
	d = t ? 16 : p;
	for(i=0; i<p; i+=d)
		for(k=0;k<n;k++)
			write(openf[k], out+i, d<p-i?d:p-i);
}

puts(s)
char *s;
{
	while(*s)
		write(2,s++,1);
}
