

#ifdef lint
static char *sccsid = "@(#)2780e.c	4.1	ULTRIX	7/17/90";
#endif


/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 * Modification History:
 *
 * 09-Jun-88	Mark Parenti
 *	Changed signal handlers to void.
 *
 */



#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <ustat.h>
#include <pwd.h>
#include <signal.h>
#include <ctype.h>
#include "rje.local.h"
#include <errno.h>

#define EOL 10

char    *tfname;		/* tmp copy of cf before linking */
char    *cfname;		/* daemon control files, linked from tf's */
char    *dfname;		/* data files */
char    *outfile;		/* optional output file name */

int	nact=0;			/* number of jobs to act on */
int	tfd;			/* control file descriptor */
int     mailflg;		/* send mail */
int	qflag;			/* q job, but don't exec daemon */
int 	transpflg=0;		/* transparent mode             */
#ifdef d3780
int 	spacecomprs=1;		/* Space Compression for 3780e is on */
#endif
int	rflag;			/* remove files upon completion */	
int	sflag=0;		/* symbolic link flag */
int 	ebcflg=0;		/* ebcdic conversion needed   lg  */
int 	prioflg=0;		/* priority job               */
int 	multibuf=0;		/* multibuffering is off      */
char	format = 'f';		/* format char for printing files */
int	inchar;			/* location to increment char in file names */
int     ncopies = 1;		/* # of copies to make */
int	hdr = 1;		/* print header or not (default is yes) */
int     userid;			/* user id */
char	*person;		/* user name */
char	host[32];		/* host name */
char    *jobname;		/* job name on header page */
char	*name;			/* program name */
/*char	*phone_num;		/* phone_number to dial    lg    */
char	buf[BUFSIZ];

int	MX;			/* maximum number of blocks to copy */
int	MC;			/* maximum number of copies allowed */
int	DU;			/* daemon user-id */
char	*SD;			/* spool directory */
char	*LO;			/* lock file name */
short	SC;			/* suppress multiple copies */

char	*getenv();
char	*rindex();
char	*linked();
void	cleanup();
/*char	*phone_num;		/* phone_number to dial    lg    */

/*ARGSUSED*/
main(argc, argv)
	int argc;
	char *argv[];
{
	extern struct passwd *getpwuid();
	struct passwd *pw;
	extern char *itoa(),*malloc();
	register char *arg, *cp;
	char **slist,**olist,*uid,*dfault;
	int no=0,ns=0,ndefault=0;
	int nsends=0,nrecvs=0;
	int i, f,n;
	int cc,rfd,pidno;
	FILE *sfp;		/* fp for signon card */
	struct stat stb;
	struct ustat usb;
	char sdir[1024];
	char *signon=NULL;	/* path of sign on card */
	char buff[80],c;		/* signon */

	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, cleanup);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, cleanup);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, cleanup);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, cleanup);

	name = argv[0];
	gethostname(host, sizeof (host));

	slist=(char **)calloc(argc,MAXPATHLEN);
	olist=(char **)calloc(argc,MAXPATHLEN);
	
	if ((slist==NULL) || (olist==NULL))
		fatal("out of memory");

	for(i=1; i<argc; i++) {
		if (*argv[i]=='-'){
			arg=argv[i];  
			switch(argv[i][1]){


/*		case 'P':		/* specify phone number */
/*			if (arg[2])
				phone_num = &arg[2];
			else if (argc > i) {
				++i;
				phone_num = argv[i];
			}
			continue;

*/
		case 'S':		/* sign on card */
			if (arg[2])
				signon = &arg[2];
			else if (argc > i) {
				++i;
				signon = argv[i];
			}
			continue;

		case 'o':		/* output files    */
			while (++i < argc) olist[no++]=argv[i];
			continue;

/*		case 'J':		/* job name */
/*			hdr++;
			if (arg[2])
				jobname = &arg[2];
			else if (argc > 1) {
				argc--;
				jobname = *++argv;
			}
			continue;


*/

#ifndef d3780
		case 'b':		/* multiple record transmission for 2780 */
			multibuf=1;
			continue;
#endif

		case 'm':		/* send mail when done */
			mailflg++;
			continue;

#ifdef d3780				/* -t or -tb */
		case 't':		/* transparent mode       */
			transpflg++;
			spacecomprs=0;	/* no space comprs in transp mode */
			if (arg[2])
				if (arg[2]=='b') /* multiple 80 col card
records in Transparent Mode */
					/* otherwise only 1 rec goes out */
					multibuf=1;
				/* else garbage */
			continue;

		case 'C':		/* turn off space compression */
			spacecomprs=0;
			continue;

#else
		case 't':		/* transparent mode       */
			transpflg++;
			continue;
#endif
/*
		case 'e':		/* EBCDIC conversion    lg  */
/*			ebcflg++;
			continue;
*/

		case 'a':		/* priority job          */
			prioflg++;
			continue;


/*		case 's':		/* try to link files */
/*			sflag++;
			continue;
*/
		case 'q':		/* just q job */
			qflag++;
			continue;


		case '#':		/* n defaults */
			if (isdigit(arg[2])) {
				n = atoi(&arg[2]);
				if (n > 0)
					ndefault = n;

			}
			continue;
		default:
			printf("option -%c ignored\n",argv[i][1]);
			continue;

		}  /* switch */
		}  /* if */
		
		slist[ns++]=argv[i];		/* sendfiles */
	} /* for */

	if (ns == 0)
#ifdef d3780
		fatal("usage: 3780e [options]  sendfiles [-o receivefiles]");
#else
		fatal("usage: 2780e [options]  sendfiles [-o receivefiles]");
#endif
	if (prioflg && (getuid()!=0))        /*  not super-user    lg    */
		fatal("not allowed to prioritize your job");
/*	if (phone_num == NULL )
		phone_num = DEFPH;                      /*   lg    */
	chkdefault();                           
	if (SC && ncopies > 1)
		fatal("multiple copies are not allowed");
	if (MC > 0 && ncopies > MC)
		fatal("only %d copies are allowed", MC);

	if ((signon) && test(signon) < 0)
		exit(-1);
	/*
	 * Get the identity of the person doing the lpr using the same
	 * algorithm as lprm. 
	 */
	userid=getuid();
	uid=malloc(5);
	if (uid==NULL) fatal("out of memory");
	sprintf(uid,"%d",userid);
	if ((pw = getpwuid(userid)) == NULL)
		fatal("Who are you?");
	person = pw->pw_name;
	/*
	 * Check to make sure queuing is enabled if userid is not root.
	 */
	(void) sprintf(buf, "%s/%s", SD, LO);
	if (userid && stat(buf, &stb) == 0 && (stb.st_mode & 010))
		fatal("RJE queue is disabled");
	stat(SD,&stb);
	ustat(stb.st_dev,&usb);
	if (usb.f_tfree < 250)
		fatal("spool directory is full");
	/*
	 * Initialize the control file.
	 */
	mktemps();
	tfd = nfile(tfname);
	(void) fchown(tfd, DU, -1);	/* owned by daemon for protection */
	card('H', host);
/* 	card('D',phone_num);		/*   lg  */
	card('P', person);
	card('I', uid);

	jobname = (arg = rindex(slist[0], '/')) ? arg+1 : slist[0];
	card('J', jobname);

	if (mailflg)
		card('M', person);
	if (transpflg)
		card('X',"transp");
	if (prioflg)
		card('A',"special");
	if (ebcflg)
		card('E',"BCDIC");
#ifdef 	d3780
	card('3',"780");
	if (spacecomprs)	card('C',"spacecomp");
#else	card('2',"780");
#endif
	if (multibuf)
		card('B',"multibuf");

	if (signon){
		sfp=fopen(signon,"r");
		n=0;
		while ((c=getc(sfp)) !=EOL)
			buff[n++]=c;
		card('S',buff);
	}
	nrecvs=no;
	while(outfile=olist[nrecvs++ - no])
		card('O',outfile);
	/* create entries for default recv files */
	
	/* I could make it so everyone has at least 1 'O'file 
         *  if (ndefault==0) && (no==0)
         *		ndefault=1;
         */

	dfault=malloc(30);
	if (dfault==NULL) fatal("out of memory");
	for(n=1;n<=ndefault;n++){
		sprintf(dfault,"R%s%d%d",person,getpid(),n);
		card('O',dfault);
	}

	/*
	 * Read the files and spool them.
	 */
/*	if we allow sendfile to = stdin then...  watch out for recvfiles
		copy(0, " ");      */
	while(nsends != ns){
		if ((f = test(arg = slist[nsends++])) < 0){
		    printf("file unreasonable\n");
		    if (ns==1) cleanup();
		    continue;
		}

		if (sflag && (cp = linked(arg)) != NULL) {

 			for (i = 0; i < ncopies; i++)
 				card(format, &dfname[inchar-2]);
 
			card('U', &dfname[inchar-2]);
			if (f)
				card('U', cp);
			card('N', arg);
			dfname[inchar]++;  /* A, B .. */
			nact++;
		}

		if (sflag)
			printf("%s: %s: not linked, copying instead\n", name, arg);

		if ((i = open(arg, O_RDONLY)) < 0) {
			printf("%s: cannot open %s\n", name, arg);
		}
		copy(i, arg);
		(void) close(i);
		if (f && unlink(arg) < 0)
			printf("%s: %s: not removed\n", name, arg);

	

} /*while*/
	if (nact) {
		(void) close(tfd);
		tfname[inchar]--;
		/*
		 * Touch the control file to fix position in the queue.
		 */
		if ((tfd = open(tfname, O_RDWR)) >= 0) {
			char c;

			if (read(tfd, &c, 1) == 1 && lseek(tfd, 0L, 0) == 0 &&
			    write(tfd, &c, 1) != 1) {
				printf("%s: cannot touch %s\n", name, tfname);
				tfname[inchar]++;
				cleanup();
			}
			(void) close(tfd);
		}
		if (link(tfname, cfname) < 0) {
			printf("%s: cannot rename %s\n", name, cfname);
			tfname[inchar]++;
			cleanup();
		}
		unlink(tfname);
		if (qflag)		/* just q things up */
			exit(0);
		if (getwd(sdir) == 0) {
			printf(" cannot get current directory");
			return(-1);
		}
		if (chdir(SD) < 0){
			printf("cannot change directory to %s\n",SD);
			exit(1);
		}
		rfd=open(".rjed",O_RDONLY,0644);
		if (rfd >= 0)   /* file exists */
		{
		  cc=read(rfd,buf,BUFSIZ);
		  close(rfd);
		  if (cc<0){  printf("error reading .rjed\n"); exit(1); }	
		  sscanf(buf,"%d",&pidno);
		  if (kill(pidno,0)==0){
			exit(0)
			/* daemon exists */ ;
		  }
		  else if (errno == EPERM) {
			 /*daemon exists */ 
			exit(0);
			}
		}
		if (chdir(sdir) < 0){
			printf("cannot change directory to %s\n",sdir);
			exit(1);
		}
		
		if (fork() == 0)	/* child */
		  execl("/etc/2780d","2780d",0);  
}
}

/*
 * Create the file n and copy from file descriptor f.
 */
copy(f, n)
	int f;
	char n[];
{
	register int fd, i, nr, nc;

	for (i = 0; i < ncopies; i++)
		card(format, &dfname[inchar-2]);

	card('U', &dfname[inchar-2]);
	card('N', n);
	fd = nfile(dfname);
	nr = nc = 0;
	while ((i = read(f, buf, BUFSIZ)) > 0) {
		if (write(fd, buf, i) != i) {
			printf("%s: %s: temp file write error\n", name, n);
			break;
		}
		nc += i;
		if (nc >= BUFSIZ) {
			nc -= BUFSIZ;
			nr++;
			if (MX > 0 && nr > MX) {
				printf("%s: %s: copy file is too large\n", name, n);
				break;
			}
		}
	}
	(void) close(fd);
	if (nc==0 && nr==0) 
		printf("%s: %s: empty input file\n", name, f ? n : "stdin");
	else
		nact++;
}

/*
 * Try and link the file to dfname. Return a pointer to the full
 * path name if successful.
 */
char *
linked(file)
	register char *file;
{
	register char *cp;
	static char buf[BUFSIZ];

	if (*file != '/') {
		if (getwd(buf) == NULL)
			return(NULL);
		while (file[0] == '.') {
			switch (file[1]) {
			case '/':
				file += 2;
				continue;
			case '.':
				if (file[2] == '/') {
					if ((cp = rindex(buf, '/')) != NULL)
						*cp = '\0';
					file += 3;
					continue;
				}
			}
			break;
		}
		strcat(buf, "/");
		strcat(buf, file);
		file = buf;
	}
	return(symlink(file, dfname) ? NULL : file);
}

/*
 * Put a line into the control file.
 */
card(c, p2)
	register char c, *p2;
{
	register char *p1 =buf;
	register int len = 2;

	*p1++ = c;
	while ((c = *p2++) != '\0') {
		*p1++ = c;
		len++;
	}
	*p1++ = '\n';
	write(tfd, buf, len);
}

/*
 * Create a new file in the spool directory.
 */
nfile(n)
	char *n;
{
	register f;
	int oldumask = umask(0);		/* should block signals */

	f = creat(n, FILMOD);
	(void) umask(oldumask);
	if (f < 0) {
		printf("%s: cannot create %s\n", name, n);
		cleanup();
	}
	if (fchown(f, userid, -1) < 0) {
		printf("%s: cannot chown %s\n", name, n);
		cleanup();
	}

	if (++n[inchar] > 'z') {
		if (++n[inchar-2] == 't') {
			printf("too many files - break up the job\n");
			cleanup();
		}
		n[inchar] = 'A';
	} else if (n[inchar] == '[')
		n[inchar] = 'a';
	return(f);
}

/*
 * Cleanup after interrupts and errors.
 */
void
cleanup()
{
	register i;

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	i = inchar;
	if (tfname)
		do
			unlink(tfname);
		while (tfname[i]-- != 'A');
	if (cfname)
		do
			unlink(cfname);
		while (cfname[i]-- != 'A');
	if (dfname)
		do {
			do
				unlink(dfname);
			while (dfname[i]-- != 'A');
			dfname[i] = 'z';
		} while (dfname[i-2]-- != 'd');
	exit(1);
}

/*
 * Test to see if this is an OK file.
 * Return -1 if it is not, 0 if it is , and 1 if
 * we should remove it after printing.
 */
test(file)
	char *file;
{
	struct exec execb;
	struct stat statb;
	register int fd;
	register char *cp;

	if (access(file, 4) < 0) {
		printf("%s: cannot access %s\n", name, file);
		return(-1);
	}
	if (stat(file, &statb) < 0) {
		printf("%s: cannot stat %s\n", name, file);
		return(-1);
	}
	if ((statb.st_mode & S_IFMT) == S_IFDIR) {
		printf("%s: %s is a directory\n", name, file);
		return(-1);
	}
	if (statb.st_size == 0) {
		printf("%s: %s is an empty file\n", name, file);
		return(-1);
 	}
	if ((fd = open(file, O_RDONLY)) < 0) {
		printf("%s: cannot open %s\n", name, file);
		return(-1);
	}
	if (read(fd, &execb, sizeof(execb)) == sizeof(execb))
		switch(execb.a_magic) {
		case A_MAGIC1:
		case A_MAGIC2:
		case A_MAGIC3:
#ifdef A_MAGIC4
		case A_MAGIC4:
#endif
			printf("%s: %s is an executable program", name, file);
			goto error1;

		case ARMAG:
			printf("%s: %s is an archive file", name, file);
			goto error1;
		}
	(void) close(fd);
	if (rflag) {
		if ((cp = rindex(file, '/')) == NULL) {
			if (access(".", 2) == 0)
				return(1);
		} else {
			*cp = '\0';
			fd = access(file, 2);
			*cp = '/';
			if (fd == 0)
				return(1);
		}
		printf("%s: %s: is not removable by you\n", name, file);
	}
	return(0);

error1:
	printf(" and is unprintable\n");
	(void) close(fd);
	return(-1);
}

/*
 * itoa - integer to string conversion
 */
char *
itoa(i)
	register int i;
{
	static char b[10] = "########";
	register char *p;

	p = &b[8];
	do
		*p-- = i%10 + '0';
	while (i /= 10);
	return(++p);
}

/*
 * Perform lookup for all default string names  --
 */
chkdefault()
{

		SD = DEFSPOOL;
		LO = DEFLOCK;
		MX = DEFMX;
		MC = DEFMAXCOPIES;
		DU = DEFUID;
		SC=1;
}

/*
 * Make the temp files.
 */
mktemps()
{
	register int c, len, fd, n;
	register char *cp;
	char *mktemp();

	(void) sprintf(buf, "%s/.seq", SD);
	if ((fd = open(buf, O_RDWR|O_CREAT, 0661)) < 0) {
		printf("%s: cannot create %s\n", name, buf);
		exit(1);
	}
	if (flock(fd, LOCK_EX)) {
		printf("%s: cannot lock %s\n", name, buf);
		exit(1);
	}
	n = 0;
	if ((len = read(fd, buf, sizeof(buf))) > 0) {
		for (cp = buf; len--; ) {
			if (*cp < '0' || *cp > '9')
				break;
			n = n * 10 + (*cp++ - '0');
		}
	}
	len = strlen(SD) + strlen(host) + 8;
	tfname = mktemp("tf", n, len);
	cfname = prioflg  ? (mktemp("af",n,len)): (mktemp("cf", n, len));
	dfname = mktemp("df", n, len);
	inchar = strlen(SD) + 3;
	n = (n + 1) % 1000;
	(void) lseek(fd, 0L, 0);
	sprintf(buf, "%03d\n", n);
	(void) write(fd, buf, strlen(buf));
	(void) close(fd);	/* unlocks as well */
}

/*
 * Make a temp file name.
 */
char *
mktemp(id, num, len)
	char	*id;
	int	num, len;
{
	register char *s;
	extern char *malloc();

	if ((s = malloc(len)) == NULL)
		fatal("out of memory");
	(void) sprintf(s, "%s/%sA%03d%s", SD, id, num, host);
	return(s);
}

/*VARARGS1*/
fatal(msg, a1, a2, a3)
	char *msg;
{
	printf("%s: ", name);
	printf(msg, a1, a2, a3);
	putchar('\n');
	exit(1);
}
