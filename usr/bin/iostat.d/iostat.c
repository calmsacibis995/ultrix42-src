#ifndef lint
static	char	*sccsid = "@(#)iostat.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986-1989 by			*
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

/*-----------------------------------------------------------------------
 *
 *	Modification History
 *
 * 24-Jan-90 -- Janet Schank
 *      Added mips support for "read_names".
 *
 * 22-Sep-89 -- Tim Burke
 *	Stripped out useless calculations in the stat routine.
 *	Removed all references to dk_busy since it was only read but not
 *	used.  Changed handling of input arguments.
 *
 * 15-Jun-88 -- jaw  --- changed to new cpudata format 
 *
 * 11-Dec-86 -- jaw
 *	header for ra disks is being truncated to 1 digit.
 *
 * 07-Apr-86 -- jrs
 *	Add code to track usage of multiple cpus.  Also remove msps
 *	column from disk data as meaningless with most disks.
 *
 *	Derived from 4.2bsd labelled:
 *		@(#)iostat.c	4.9 (Berkeley) 83/09/25
 *
 *-----------------------------------------------------------------------
 */

#include <stdio.h>
#include <nlist.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/buf.h>
#include <sys/smp_lock.h>
#include <sys/cpudata.h>
#include <sys/dk.h>
#include <io/uba/ubavar.h>
#ifdef vax
#include <vaxmba/mbavar.h>
#endif
#ifdef sun
#include <sundev/mbvar.h>
#endif

struct nlist nl[] = {
	{ "_dk_time" },
#define	X_DK_TIME	0
	{ "_dk_xfer" },
#define	X_DK_XFER	1
	{ "_dk_wds" },
#define	X_DK_WDS	2
	{ "_dk_seek" },
#define	X_DK_SEEK	3
	{ "_dk_mspw" },
#define	X_DK_MSPW	4
	{ "_hz" },
#define	X_HZ		5
	{ "_cpudata" },
#define	X_CPUDATA	6
	{ "_boot_cpu_num"},
#define X_BOOT_CPU_NUM  7

#ifndef sun
	{ "_ubdinit" },
#define X_UBDINIT	8
#ifdef vax
	{ "_mbdinit" },
#define X_MBDINIT	9
#endif vax
#else sun
	{ "_mbdinit" },
#define X_MBDINIT	8
#endif sun
	{ 0 },
};

char dr_name[DK_NDRIVE][11];
int	activecpu;
#ifdef mips
int dr_active[DK_NDRIVE];
#endif mips

#define	MAXREPORTCPU	32

struct
{
	long	cp_time[CPUSTATES];
	long	dk_printit[DK_NDRIVE];
	long	dk_time[DK_NDRIVE];
	long	dk_wds[DK_NDRIVE];
	long	dk_seek[DK_NDRIVE];
	long	dk_xfer[DK_NDRIVE];
	float	dk_mspw[DK_NDRIVE];
	long	tk_nin;
	long	tk_nout;
	struct	cpudata cpudata[MAXREPORTCPU];
} s, s1;

struct cpudata *pcpudata;
int cpudata_size;

int	mf;
int	hz;
double	etime;
char 	*disktest[DK_NDRIVE];
int	diskptr;
int 	showtty;
int	showallcpu;

main(argc, argv)
char *argv[];
{
	extern char *ctime();
	register  i, cpindex;
	int count, interval;
	double f1, f2;
	long t;
	int tohdr = 1;
	struct cpudata *pcpu;
	int boot_cpu_num;

	nlist("/vmunix", nl);
	if(nl[X_DK_TIME].n_type == 0) {
		printf("dk_time not found in /vmunix namelist\n");
		exit(1);
	}
	mf = open("/dev/kmem", 0);
	if(mf < 0) {
		printf("cannot open /dev/kmem\n");
		exit(1);
	}
	diskptr = 0;
	count = 0;
	interval=0;
	showtty=0;
	showallcpu=0;

	/*
	 * Parse out option switches.
	 */
	while (argc>1&&argv[1][0]=='-') {
		for(i=1; argv[1][i] !=0 ; i++) {
			if (argv[1][i]=='t') showtty=1;
			if (argv[1][i]=='c') showallcpu=1;
		}
		argc--;
		argv++;
	}
	/*
	 * Obtain a list of disks to print out information on.
	 * Assume that a disk name is being specified if the first
	 * character in this argument is an alpha character.
	 */
	while(argc >1) {
		if (isalpha(argv[1][0])) {
			disktest[diskptr] = argv[1];
			diskptr++;
			argc--;
			argv++;
		} else break;	
	}
	/*
 	 * Get the optional "interval" parameter.  This specifies how long
	 * to sleep between each iteration.
	 */
	if(argc > 1) {
		if (isdigit(argv[1][0])) {
			interval = atoi(argv[1]);
			argc--;
			argv++;
		}
	}
	/*
	 * Get the optional "count" parameter.  This specifies how many times
	 * to print results.
	 */
	if(argc > 1) {
		if (isdigit(argv[1][0])) {
			count = atoi(argv[1]);
			argc--;
			argv++;
		}
	}
#ifdef mips
loop:
#endif mips

	lseek(mf, (long)nl[X_DK_MSPW].n_value, 0);
	read(mf, s.dk_mspw, sizeof s.dk_mspw);
#ifdef mips
	lseek(mf, (long)nl[X_DK_WDS].n_value, 0);
	read(mf, s.dk_wds, sizeof s.dk_wds);
#endif mips
	for (i = 0; i < DK_NDRIVE; i++)
#ifdef mips
		if(s.dk_wds[i]) { /* Something xferred so drive is alive */
			sprintf(dr_name[i], "dk%d", i);
			dr_active[i] = 1;
			
		}
#endif mips
#ifdef vax
		sprintf(dr_name[i], "dk%d", i);
#endif vax
	read_names();
	for(i = 0; i< MAXREPORTCPU; i++) {
		lseek(mf, (long)nl[X_CPUDATA].n_value + 4*i, 0);
		read(mf, &pcpu, sizeof (pcpu));
		if (pcpu) {
			activecpu = activecpu | (1<<i);
		}
	}
	findwhichdisks();
#ifdef vax
loop:
#endif vax
	if (--tohdr == 0) {
		if (showtty) printf("   tty    ");
		for (i = 0; i < DK_NDRIVE; i++)
#ifdef mips
			if (dr_active[i])
#endif mips
#ifdef vax
			if ((s.dk_mspw[i] != 0.0) && s.dk_printit[i])
#endif vax
				printf("   %3.5s ", dr_name[i]);
		if (showallcpu) {
			for (cpindex = 0; cpindex < MAXREPORTCPU; cpindex++) {
				if (activecpu & (1<<cpindex)) 
					printf("      cpu %d  ", cpindex);
			}
		}else	printf("     cpu   ");
		printf("\n");
		if (showtty) printf(" tin tout ");
		for (i = 0; i < DK_NDRIVE; i++)
#ifdef vax
			if ((s.dk_mspw[i] != 0.0) && s.dk_printit[i]) 
#endif vax
#ifdef mips
			if (dr_active[i])
#endif mips
				printf(" bps tps ");
		if (showallcpu) {
			for (cpindex = 0; cpindex < MAXREPORTCPU; cpindex++) {
				if (activecpu & (1<<cpindex)) 
					printf("  us ni sy id");
			}
		}else printf(" us ni sy id");
		printf("\n");
		tohdr = 19;
	}
 	lseek(mf, (long)nl[X_DK_TIME].n_value, 0);
 	read(mf, s.dk_time, sizeof s.dk_time);
 	lseek(mf, (long)nl[X_DK_XFER].n_value, 0);
 	read(mf, s.dk_xfer, sizeof s.dk_xfer);
 	lseek(mf, (long)nl[X_DK_WDS].n_value, 0);
 	read(mf, s.dk_wds, sizeof s.dk_wds);
	lseek(mf, (long)nl[X_DK_SEEK].n_value, 0);
	read(mf, s.dk_seek, sizeof s.dk_seek);
	lseek(mf, (long)nl[X_DK_MSPW].n_value, 0);
	read(mf, s.dk_mspw, sizeof s.dk_mspw);
	lseek(mf, (long)nl[X_HZ].n_value, 0);
	read(mf, &hz, sizeof hz);
	lseek(mf, (long)nl[X_BOOT_CPU_NUM].n_value, 0);
	read(mf, &boot_cpu_num, sizeof boot_cpu_num);

	for(i=0; i< MAXREPORTCPU; i++) {
		if (activecpu & (1<<i)) {
			lseek(mf, (long)nl[X_CPUDATA].n_value + (4*i), 0);
			read(mf, &pcpu, 4);
			lseek(mf, (long)pcpu, 0);
			read(mf, &s.cpudata[i], sizeof(struct cpudata));
		}
	}
	for (i = 0; i < DK_NDRIVE; i++) {
#define X(fld)	t = s.fld[i]; s.fld[i] -= s1.fld[i]; s1.fld[i] = t
		X(dk_xfer); X(dk_seek); X(dk_wds); X(dk_time);
	}
	etime = 0;
	for(i=0; i<CPUSTATES; i++) {
		s.cp_time[i] = 0;
		for (cpindex = 0; cpindex < MAXREPORTCPU; cpindex++) {
			if (activecpu & (1<< cpindex)) {
				X(cpudata[cpindex].cpu_cptime);
				s.cp_time[i] +=s.cpudata[cpindex].cpu_cptime[i];
			}
		}
		etime += s.cpudata[boot_cpu_num].cpu_cptime[i];
	}
	s.tk_nin = 0;	s.tk_nout = 0;
	for (cpindex = 0; cpindex < MAXREPORTCPU; cpindex++) {
		if (activecpu & (1<<cpindex)) {
				t = s.cpudata[cpindex].cpu_ttyin; 
			s.cpudata[cpindex].cpu_ttyin -= 
					s1.cpudata[cpindex].cpu_ttyin; 
			s1.cpudata[cpindex].cpu_ttyin = t;
			s.tk_nin+=s.cpudata[cpindex].cpu_ttyin;

			t = s.cpudata[cpindex].cpu_ttyout; 
			s.cpudata[cpindex].cpu_ttyout -= 
					s1.cpudata[cpindex].cpu_ttyout; 
			s1.cpudata[cpindex].cpu_ttyout = t;
			s.tk_nout+=s.cpudata[cpindex].cpu_ttyout;
		}
	}

	if (etime == 0.0)
		etime = 1.0;
	etime /= (float) hz;
	if (showtty) printf("%4.0f%5.0f ", s.tk_nin/etime, s.tk_nout/etime);
	for (i=0; i<DK_NDRIVE; i++)
#ifdef vax
		if ((s.dk_mspw[i] != 0.0) && s.dk_printit[i])
#endif vax
#ifdef mips
		if (dr_active[i])
#endif mips
			stats(i);
	if ((activecpu > 1)&& showallcpu) {
		for (cpindex = 0; cpindex < MAXREPORTCPU; cpindex++) {
			if (activecpu & (1<<cpindex)) {
				printf(" ");
				stat1(s.cpudata[cpindex].cpu_cptime);
			}
		}
	}else stat1(s.cp_time);
	printf("\n");
	fflush(stdout);

contin:
	--count;
	if(count)
	if (interval) {
		sleep(interval);
		goto loop;
	}
}

/*
 * Determine which disks to print stats on.
 */
findwhichdisks() {
	int i=0;
	int j=0;
	int pos;

	/*
 	 * If no disks are specified in the parameters to the iostat
	 * command then enable printing on the first 3 disks by default.
	 */
	if (diskptr == 0) {
		s.dk_printit[0]=1;
		s.dk_printit[1]=1;
		s.dk_printit[2]=1;
	}
	/*
	 * Some disks have been specified in the arguments.  If a match for
	 * this disk can be found in the table of configured disks then
	 * enable printing of stats for the specified disk.
	 */
	else {
		for (i=0; i<diskptr; i++){
			for(j=0; j < DK_NDRIVE; j++) {
				pos=0;
				while(dr_name[j][pos]  == disktest[i][pos]) {
					if (disktest[i][pos] == '\0') break;
					if (dr_name[j][pos] == '\0') break;
					pos++;
				}		
				if ((disktest[i][pos] == '\0') &&
				    (dr_name[j][pos] == ' '))
						s.dk_printit[j] = 1;
			
		 	}
		}
	}

}

/*
 * Print out the number of blocks and transfers in this time duration.
 */
stats(dn)
{
	double words = 0.0;
	double xfer_rate = 0.0;

#ifdef vax
	if (s.dk_mspw[dn] != 0.0) {
#endif vax
#ifdef mips
	if(dr_active[dn] != 0) {
#endif mips
                words = (s.dk_wds[dn]*32.0)/512/etime;    
		xfer_rate = s.dk_xfer[dn]/etime;
	}
	printf("%4.0f%4.0f ",words,xfer_rate);
}

stat1(timebuf)
long timebuf[];
{
	register i;
	double time;

	time = 0;
	for(i = 0; i < CPUSTATES; i++)
		time += timebuf[i];
	if (time == 0.0)
		time = 1.0;
	for(i = 0; i < CPUSTATES; i++) {
		printf("%3.0f", (100.0 * timebuf[i]) / time);
	}
}

#define steal(where, var) lseek(mf, where, 0); read(mf, &var, sizeof var);

#ifndef sun
read_names()
{
#ifdef vax
	struct mba_device mdev;
	register struct mba_device *mp;
	struct mba_driver mdrv;
#endif vax
	short two_char;
	char *cp = (char *) &two_char;
	struct uba_device udev, *up;
	struct uba_driver udrv;

#ifdef vax
	mp = (struct mba_device *) nl[X_MBDINIT].n_value;
#endif vax
	up = (struct uba_device *) nl[X_UBDINIT].n_value;
	if (up == 0)
	{
		fprintf(stderr, "iostat: Disk init info not in namelist\n");
		exit(1);
	}
#ifdef vax
	if (mp) for (;;) {
		steal(mp++, mdev);
		if (mdev.mi_driver == 0)
			break;
		if (mdev.mi_dk < 0 || mdev.mi_alive == 0)
			continue;
		steal(mdev.mi_driver, mdrv);
		steal(mdrv.md_dname, two_char);
		sprintf(dr_name[mdev.mi_dk], "%c%c%d", cp[0], cp[1], mdev.mi_unit);
	}
#endif vax
	if (up) for (;;) {
		steal(up++, udev);
		if (udev.ui_driver == 0)
			break;
		if (udev.ui_dk < 0 || udev.ui_alive == 0)
			continue;
		steal(udev.ui_driver, udrv);
		steal(udev.ui_devname, two_char);
		sprintf(dr_name[udev.ui_dk], "%c%c%d  ", cp[0], cp[1], udev.ui_unit);
	}
}
#endif

#ifdef sun
/*
 * Obain a list of disks that are configured into the system.
 * This list of names will be matched with the input args to determine
 * which disks to display statistics on.
 *
 * Restriction: The controller name must be 2 characters in length
 */
read_names()
{
	struct mb_device mdev;
	register struct mb_device *mp;
	struct mb_driver mdrv;
	short two_char;
	char *cp = (char *) &two_char;

	mp = (struct mb_device *) nl[X_MBDINIT].n_value;
	if (mp == 0) {
		fprintf(stderr, "iostat: Disk init info not in namelist\n");
		exit(1);
	}
	for (;;) {
		steal(mp++, mdev);
		if (mdev.md_driver == 0)
			break;
		if (mdev.md_dk < 0 || mdev.md_alive == 0)
			continue;
		steal(mdev.md_driver, mdrv);
		steal(mdrv.mdr_dname, two_char);
		sprintf(dr_name[mdev.md_dk], "%c%c%d", cp[0], cp[1], mdev.md_unit);
	}
}
#endif
