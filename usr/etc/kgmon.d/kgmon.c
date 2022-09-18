#ifndef lint
static char sccsid[] = "@(#)kgmon.c	4.1	(ULTRIX)	7/2/90";
#endif lint
/*
 * Based on:
 * static char sccsid[] = "@(#)kgmon.c	4.9 (Berkeley) 83/08/11";
 */

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
 *20-Jun-88	jaw  
 *	kgmon now dumps out kernel profiling data for multiple 
 *	processors.  Format dumped is not compatible with gprof.  Must
 *	run the kgdump.out though the kgconv program.  Manual page 
 *	must be update for smp release.
 * 
 */
#include <sys/param.h>
#include <machine/pte.h>
#include <sys/vm.h>
#include <stdio.h>
#include <nlist.h>
#include <ctype.h>
#include <sys/gprof.h>

/*
 * froms is actually a bunch of unsigned shorts indexing tos
 */
u_short			*froms;
struct	tostruct	*tos;
char			*s_lowpc;
u_long			s_textsize;
int			ssiz;
off_t			sbuf;

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
	{ "_tostruct_size" },
	0,
};

struct	pte *Sysmap;

char	*progname;
char	*dumpfile =	"kgdump.out";
char	*sysfile =	"/vmunix";
char	*kmemf =	"/dev/kmem";
int	kmem;
int	bflag, hflag, kflag, rflag, pflag;
char	*malloc();
void	exit(), perror(), nlist();
long	klseek();
long	lseek();

main(argc, argv)
	int argc;
	char *argv[];
{
	int mode, disp, openmode = 0;

	progname = argv[0];
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {
		case 'b':
			bflag++;
			openmode = 2;
			break;
		case 'h':
			hflag++;
			openmode = 2;
			break;
		case 'r':
			rflag++;
			openmode = 2;
			break;
		case 'p':
			pflag++;
			openmode = 2;
			break;
		default:
			(void) fprintf(stderr,
				"Usage: %s [ -b -h -r -p sysfile memory ]\n",
				progname);
			exit(1);
		}
		argc--, argv++;
	}
	if (argc > 0) {
		sysfile = *argv;
#ifdef DEBUG
		(void) fprintf(stderr,"Using sysfile = %s\n",sysfile);
#endif DEBUG
		argv++, argc--;
	}
	nlist(sysfile, nl);
	if (nl[0].n_type == 0) {
		(void) fprintf(stderr, "%s: %s has no namelist\n",
			progname, sysfile);
		exit(2);
	}
	if (argc > 0) {
		kmemf = *argv;
#ifdef DEBUG
		(void) fprintf(stderr,"Using kmemfile = %s\n",kmemf);
#endif DEBUG
		kflag++;
	}
	kmem = open(kmemf, openmode);
	if (kmem < 0) {
		openmode = 0;
		kmem = open(kmemf, openmode);
		if (kmem < 0) {
			(void) fprintf(stderr, "%s: cannot open ", progname);
			perror(kmemf);
			exit(3);
		}
		(void) fprintf(stderr, "%s opened read-only\n", kmemf);
		if (rflag)
			(void) fprintf(stderr, "-r supressed\n");
		if (bflag)
			(void) fprintf(stderr, "-b supressed\n");
		if (hflag)
			(void) fprintf(stderr, "-h supressed\n");
		rflag = 0;
		bflag = 0;
		hflag = 0;
	}
	if (kflag) {
		off_t off;

		off = nl[N_SYSMAP].n_value & 0x7fffffff;
		if (lseek(kmem, (long)off, 0) == -1) {
			(void) fprintf(stderr,"%s: ",progname);
			perror("lseek");
		}
		nl[N_SYSSIZE].n_value *= 4;
		Sysmap = (struct pte *)malloc((unsigned)nl[N_SYSSIZE].n_value);
		if (Sysmap == 0) {
			(void) fprintf(stderr,"%s: ",progname);
			perror("Sysmap");
			exit(4);
		}
		if (read(kmem, (char *)Sysmap, (int) nl[N_SYSSIZE].n_value) !=
			(int) nl[N_SYSSIZE].n_value) {
			(void) fprintf(stderr,"%s: ",progname);
			perror("read");
			exit(5);
		}
	}
	mode = kfetch(N_PROFILING);
	if (hflag)
		disp = PROFILING_OFF;
	else if (bflag)
		disp = PROFILING_ON;
	else
		disp = mode;
	if (pflag) {
		if (openmode == 0 && mode == PROFILING_ON)
			(void) fprintf(stderr, "%s: data may be inconsistent\n",
				progname);
		dumpstate();
	}
	if (rflag)
		resetstate();
	turnonoff(disp);
	(void) printf("Kernel profiling is %s.\n",disp?"off":"running");
}

/*
 *	SMP changes:
 *	The layout of the dump file is:
 *	4 bytes for ssiz
 *	ssiz bytes of sbuf (the time counters)
 *	4 bytes for s_textsize
 *	4 bytes for s_lowpc
 *	4 bytes for fromssize
 *	fromssize bytes of the from structure
 *	4 bytes for number of tostructs
 *	4 bytes for the tossize
 *	tossize bytes of the tostruct (variable size because of NCPU)
 */

dumpstate()
{
	register int i,j;
	int fd;
	off_t kfroms, ktos;
	int fromssize, tossize, num_tostructs, to_struct_size;
	char buf[BUFSIZ];

	turnonoff(PROFILING_OFF);
	fd = creat(dumpfile, 0666);
	if (fd < 0) {
		(void) fprintf(stderr,"%s: ", progname);
		perror(dumpfile);
		return;		/* print the debugging state on return */
	}
	ssiz = kfetch(N_SSIZ);
	/* dump ssiz */
	if (write(fd,(char *)&ssiz,sizeof(ssiz)) != sizeof(ssiz)) {
		(void) fprintf(stderr,"%s: ",progname);
		perror("write");
		return;
	}
	sbuf = kfetch(N_SBUF);
	if (klseek(kmem, (off_t)sbuf, 0) == -1) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("lseek");
		return;
	}
	for (i = ssiz; i > 0; i -= BUFSIZ) {
		j = (i < BUFSIZ ? i : BUFSIZ);
		if (read(kmem, buf, j) != j) {
			(void) fprintf(stderr,"%s: ",progname);
			perror("read");
			return;
		}
		/* dump the time counters */
		if (write(fd, buf, j) != j) {
			(void) fprintf(stderr,"%s: ",progname);
			perror("write");
			return;
		}
	}
	s_textsize = kfetch(N_S_TEXTSIZE);
	/* dump the textsize */
	if (write(fd,(char *)&s_textsize,sizeof(s_textsize)) !=
	sizeof(s_textsize)) {
		(void) fprintf(stderr,"%s: ",progname);
		perror("write");
		return;
	}
	s_lowpc = (char *)kfetch(N_S_LOWPC);
	/* dump the lowest pc */
	if (write(fd,(char *)&s_lowpc,sizeof(s_lowpc)) != sizeof(s_lowpc)) {
		(void) fprintf(stderr,"%s: ",progname);
		perror("write");
		return;
	}
	fromssize = s_textsize / HASHFRACTION;
	/* dump the fromsize */
	if (write(fd,(char *)&fromssize,sizeof(fromssize)) !=
	sizeof(fromssize)){
		(void) fprintf(stderr,"%s: ",progname);
		perror("write");
		return;
	}
	froms = (u_short *)malloc((unsigned)fromssize);
	kfroms = kfetch(N_FROMS);
	if (klseek(kmem, kfroms, 0) == -1) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("lseek");
		return;
	}
	i = read(kmem, ((char *)(froms)), fromssize);
	if (i != fromssize) {
		(void) fprintf(stderr,"%s: ", progname);
		(void) fprintf(stderr, "read froms: request %d, got %d",
			fromssize, i);
		perror("");
		return;
	}
	/* dump the froms structures */
	if (write(fd,(char *)froms,fromssize) != fromssize) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("write");
		return;
	}
	num_tostructs = (s_textsize * ARCDENSITY / 100);
	/* dump the number of to structures */
	if (write(fd,(char *)&num_tostructs,sizeof(num_tostructs)) !=
		sizeof(num_tostructs)) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("write");
		return;
	}
	to_struct_size = kfetch(N_TOSTRUCTSIZE);
	/* dump the size of the tostruct */
	if (write(fd,(char *)&to_struct_size,sizeof(to_struct_size)) !=
		sizeof(to_struct_size)) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("write");
		return;
	}
	/* the last two numbers combine to allow kgconv/kgdump */
	/* to figure out how many cpus there are */
	tossize = num_tostructs * to_struct_size;
	tos = (struct tostruct *)malloc((unsigned)tossize);
	ktos = kfetch(N_TOS);
	if (klseek(kmem, ktos, 0) == -1) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("lseek");
		return;
	}
	i = read(kmem, ((char *)(tos)), tossize);
	if (i != tossize) {
		(void) fprintf(stderr,"%s: ", progname);
		(void) fprintf(stderr, "read tos: request %d, got %d",
			tossize, i);
		perror("");
		return;
	}
	/* dump the to structures */
	if (write(fd,(char *)tos,tossize) != tossize) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("write");
		return;
	}
	if (close(fd) == -1) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("close");
		return;
	}
}

resetstate()
{
	int i;
	off_t kfroms, ktos;
	int fromssize, tossize;
	char buf[BUFSIZ];

	turnonoff(PROFILING_OFF);
	bzero(buf, BUFSIZ);
	ssiz = kfetch(N_SSIZ);
	sbuf = kfetch(N_SBUF);
	ssiz -= sizeof(struct phdr);
	sbuf += sizeof(struct phdr);
	if (klseek(kmem, (off_t)sbuf, 0) == -1) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("lseek");
		exit(6);
	}
	for (i = ssiz; i > 0; i -= BUFSIZ)
		if (write(kmem, buf, i < BUFSIZ ? i : BUFSIZ) < 0) {
			(void) fprintf(stderr,"%s: ", progname);
			perror("sbuf write");
			exit(7);
		}
	s_textsize = kfetch(N_S_TEXTSIZE);
	fromssize = s_textsize / HASHFRACTION;
	kfroms = kfetch(N_FROMS);
	if (klseek(kmem, kfroms, 0) == -1) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("lseek");
		exit(8);
	}
	for (i = fromssize; i > 0; i -= BUFSIZ)
		if (write(kmem, buf, i < BUFSIZ ? i : BUFSIZ) < 0) {
			(void) fprintf(stderr,"%s: ", progname);
			perror("kfroms write");
			exit(9);
		}
	tossize = (s_textsize * ARCDENSITY / 100) * (kfetch(N_TOSTRUCTSIZE));
	ktos = kfetch(N_TOS);
	if (klseek(kmem, ktos, 0) == -1) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("lseek");
		exit(10);
	}
	for (i = tossize; i > 0; i -= BUFSIZ)
		if (write(kmem, buf, i < BUFSIZ ? i : BUFSIZ) < 0) {
			(void) fprintf(stderr,"%s: ", progname);
			perror("ktos write");
			exit(11);
		}
}

turnonoff(onoff)
	int onoff;
{
	off_t off;

	if ((off = nl[N_PROFILING].n_value) == 0) {
		(void) fprintf(stderr,"%s: profiling not defined in kernel\n",
			progname);
		exit(12);
	}
#ifdef DEBUG
	(void)fprintf(stderr,"turnonoff: seeking on kmem to offset 0x%x\n",off);
#endif DEBUG
	if (klseek(kmem, off, 0) == -1) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("lseek");
		exit(13);
	}
#ifdef DEBUG
	(void) fprintf(stderr,"turnonoff: Writing kmem at offset 0x%x\n",off);
#endif DEBUG
	if (write(kmem, (char *)&onoff, sizeof (onoff)) != sizeof(onoff)) {
		(void) fprintf(stderr,"%s: ",progname);
		perror("write");
		exit(14);
	}
}

kfetch(index)
	int index;
{
	off_t off;
	int value;

	if ((off = nl[index].n_value) == 0) {
		(void) fprintf(stderr,"%s: %s not defined in kernel\n",
			progname, nl[index].n_name);
		exit(15);
	}
#ifdef DEBUG
	(void) fprintf(stderr,"kfetch: for %s ",nl[index].n_name);
#endif DEBUG
	if (klseek(kmem, off, 0) == -1) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("lseek");
		exit(16);
	}
	if (read(kmem, (char *)&value, sizeof (value)) != sizeof (value)) {
		(void) fprintf(stderr,"%s: ", progname);
		perror("read");
		exit(17);
	}
#ifdef DEBUG
	if (value < 0) {
		(void) fprintf(stderr,"got 0x%x\n",value);
	}
	else {
		(void) fprintf(stderr,"got %d\n",value);
	}
#endif DEBUG
	return (value);
}

long
klseek(fd, base, off)
	int fd, base, off;
{

	if (kflag) {
		/* get kernel pte */
#if vax
		base &= 0x7fffffff;
#endif
		base = ((int)ptob(Sysmap[btop(base)].pg_pfnum))+(base&(NBPG-1));
	}
	return (lseek(fd, (long) base, off));
}
