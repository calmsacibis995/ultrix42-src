#ifndef lint
static char *sccsid = "@(#)kgconv.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 * 12-Apr-89     gmm
 *	use MAXCPU definition from cpudata.h
 *
 * 20-Jun-88	jaw  
 *	new utility to convert dumps from kgmon to gprof format.
 * 
 */

#include <sys/param.h>
#include <machine/pte.h>
#include <sys/vm.h>
#include <stdio.h>
#include <nlist.h>
#include <ctype.h>
#include <sys/gprof.h>
#include <sys/cpudata.h>

/*
 * froms is actually a bunch of unsigned shorts indexing tos
 */
u_short	*froms;
struct	tostruct *tos;
char	*s_lowpc;
u_long	s_textsize;

struct nlist nl[] = {
#define	N_SYSMAP	0
	{ "_Sysmap" },
#define	N_SYSSIZE	1
	{ "_Syssize" },
#define N_FROMS		2
	{ "_froms" },
#define	N_PROFILING	3
	{ "_profiling" },
#define	N_S_LOWPC	4
	{ "_s_lowpc" },
#define	N_S_TEXTSIZE	5
	{ "_s_textsize" },
#define	N_SBUF		6
	{ "_sbuf" },
#define N_SSIZ		7
	{ "_ssiz" },
#define	N_TOS		8
	{ "_tos" },
#define	N_TOSTRUCTSIZE	9
	{ "_to_struct_size" },
	0,
};

char	*dumpfile =	"kgdump.out";
char	*sysfile  =	"/vmunix";
char	*gmonfile =	"gmon.out";
int	dflag = 0;
char	*progname;
#define ALL -1

int	num_to_dump = ALL;	/* dump all for default */
int	dfd;
char *malloc();
void perror(), exit(), nlist();

main(argc, argv)
	int argc;
	char *argv[];
{

	progname = argv[0];
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {
		case 'd':
			dflag++;
			break;
		case 'n':
			num_to_dump = atoi(&argv[0][2]);
			if (num_to_dump < ALL || num_to_dump >= MAXCPU) {
				/* allow 0, 1, ..., MAXCPU-1
				/* MAXCPU for now for error checking */
				(void) fprintf(stderr,"%s: bad cpu number %d\n",
					progname,num_to_dump);
				exit(1);
			}
			break;
		default:
			(void) fprintf(stderr,
				"Usage: %s [ -d kgdump.out sysfile ]\n",
				progname);
			exit(1);
		}
		argc--, argv++;
	}
	if (argc > 0) {
		dumpfile = *argv;
		argv++, argc--;
	}
	if (argc > 0) {
		sysfile = *argv;
		argv++, argc--;
	}
	if (dflag) {
		(void) fprintf(stderr,"%s: %s %s\n",progname,dumpfile,sysfile);
	}
	nlist(sysfile, nl);
	if (nl[0].n_type == 0) {
		(void) fprintf(stderr, "%s: no namelist\n", sysfile);
		exit(2);
	}
	if ((dfd = open(dumpfile,0)) == -1) {
		(void) fprintf(stderr,"%s: ",progname);
		perror(dumpfile);
		exit(2);
	}
	dumpstate();
	(void) close(dfd);
	exit(0);
}

dumpstate()
{
	int fromindex, endfrom, fromssize, tossize;
	u_long frompc;
	int toindex;
	struct rawarc rawarc;
	u_short *sbuf;
	register int i,sum,*top;
	int tostructs,to_struct_size;
	int ssiz;
	int ncpu;
	int fd;
	int *cnt;

	fd = creat(gmonfile, 0666);
	if (fd < 0) {
		perror(gmonfile);
		return;
	}
	if (dflag) {
		(void) fprintf(stderr,"opened %s\n",gmonfile);
	}
	if (read(dfd,(char *)&ssiz,sizeof(ssiz)) != sizeof(ssiz)) {
		(void) fprintf(stderr,"short read of ssiz\n");
		exit(1);
	}
	sbuf = (u_short *) malloc((unsigned)ssiz);
	if (read(dfd,(char *)sbuf,ssiz) != ssiz) {
		(void) fprintf(stderr,"short read of sbuf\n");
		exit(1);
	}
	if (write(fd,(char *)sbuf,ssiz) != ssiz) {
		(void) fprintf(stderr,"short write of sbuf\n");
		exit(3);
	}
	if (read(dfd,(char *)&s_textsize,sizeof(s_textsize))
	!= sizeof(s_textsize)) {
		(void) fprintf(stderr,"short read of s_textsize\n");
		exit(1);
	}
	if (read(dfd,(char *)&s_lowpc,sizeof(s_lowpc)) != sizeof(s_lowpc)) {
		(void) fprintf(stderr,"short read of s_lowpc\n");
		exit(1);
	}
	if (read(dfd,(char *)&fromssize,sizeof(fromssize))
	!= sizeof(fromssize)) {
		(void) fprintf(stderr,"short read of s_fromssize\n");
		exit(1);
	}
	froms = (u_short *)malloc((unsigned)fromssize);
	i = read(dfd, ((char *)(froms)), fromssize);
	if (i != fromssize) {
		(void) fprintf(stderr,
			"read froms: request %d, got %d", fromssize, i);
		perror("");
		exit(5);
	}
	if (read(dfd,(char *)&tostructs,sizeof(tostructs))
	!= sizeof(tostructs)) {
		(void) fprintf(stderr,"short read of tostructs\n");
		exit(1);
	}
	if (read(dfd,(char *)&to_struct_size,sizeof(to_struct_size))
	!= sizeof(to_struct_size)) {
		(void) fprintf(stderr,"short read of to_struct_size\n");
		exit(1);
	}
	tossize = tostructs * to_struct_size;
	/* #define NCPU 1 must be true */
	ncpu = 1+((to_struct_size - sizeof(struct tostruct))/sizeof(int));
	if (dflag) {
		(void) fprintf(stderr,"Ncpus = %d\n",ncpu);
		if (num_to_dump != ALL && ncpu-1 < num_to_dump) {
			(void) fprintf(stderr,
				"error asked to dump %d\n",num_to_dump);
			exit(2);
		}
	}
	tos = (struct tostruct *)malloc((unsigned)tossize);
	i = read(dfd, ((char *)(tos)), tossize);
	if (i != tossize) {
		(void) fprintf(stderr,
			"read tos: request %d, got %d", tossize, i);
		perror("");
		exit(6);
	}
	endfrom = fromssize / sizeof(*froms);
/* TOP(i) gives pointer to tostruct */
#define TOP(i)	((int *)(((char *)tos) + (i) * to_struct_size))
/* CNT(i) gives pointer to first count integer */
#define CNT(i)	((int *)(((char *)TOP(i)) + 4))
/* LNK(i) gives pointer to link field */
#define LNK(i)	((int *)(((char *)TOP(i+1)) - 4))
	for (fromindex = 0; fromindex < endfrom; fromindex++) {
		if (froms[fromindex] == 0)
			continue;
		frompc = (u_long)s_lowpc +
		    (fromindex * HASHFRACTION * sizeof(*froms));
		for (toindex = froms[fromindex]; toindex != 0;
		   toindex = *LNK(toindex)) {
			top = TOP(toindex);
			cnt = CNT(toindex);
			if (dflag) {
				(void) fprintf(stderr,"frompc 0x%x selfpc 0x%x",
					frompc, *top);
				for(i=0;i<ncpu;i++)
					(void) fprintf(stderr,"%d ",cnt[i]);
				(void) fprintf(stderr,"\n");
			}
			rawarc.raw_frompc = frompc;
			rawarc.raw_selfpc = (u_long)(top[0]); /* selfpc */
			sum = 0;
			if (num_to_dump == ALL) {
				for(i=0;i<ncpu;i++) sum += cnt[i];
			}
			else sum = cnt[num_to_dump];
			rawarc.raw_count = sum;
			if (write(fd, (char *)&rawarc, sizeof (rawarc)) !=
				sizeof(rawarc)) {
				(void)fprintf(stderr,"short write of rawarc\n");
				exit(2);
			}
		}
	}
	(void) close(fd);
}
