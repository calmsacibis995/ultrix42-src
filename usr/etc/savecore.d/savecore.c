#ifndef lint
static char *sccsid = "@(#)savecore.c	4.2    ULTRIX  10/9/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985,86 by			*
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
/************************************************************************
 *
 *			Modification History
 *
 *	JAW 17-Sep-90
 *	Sync out a crash dump after it has been saved but before the 
 *	dump partition has been cleared.
 *
 * 	Alan Frechette 09-Nov-89
 *	Check for bad or failed dumps by making sure all the dump 
 *	descriptors are valid by examining the PFN's and by checking 
 *	the last dump descriptor for magic numbers. This sanity checking
 *	is needed to determine whether a dump succeeded or failed. Also 
 *	added logging of all dumps that succeeded or failed in the file 
 *	"/usr/adm/shutdownlog".
 *
 * 	Alan Frechette 22-Sep-89
 *	Check for EOF in save_core() when saving the core image. If
 *	we reach EOF on the dump device then break out of the loop 
 *	otherwise we will be stuck in an infinite loop. Needed for
 *	dumps that failed.
 *
 * 	Alan Frechette 15-Aug-89
 *	Fixed bug in savecore to read the partial dump magic number
 *	and the full dump magic number from /dev/kmem instead of from
 *	the dump device. Save the error logger buffer before saving
 *	the core image.
 *
 * 	Alan Frechette 04-Aug-89
 *	Fixed a minor bug so savecore would work as it did before
 *	when the system image that crashed was not "/vmunix". When
 *	savecore is run on a different system image it assumes the
 *	running system image is "/vmunix".
 *
 * 	Alan Frechette 24-July-89
 *	New savecore utility which handles the new crash dump strategy.
 *	System now does selective dumping of physical memory. Now only 
 *	one savecore utility for both VAX and MIPS architectures. Removed
 *	lots of code that was no longer needed. Changed entire utility to
 *	read and write in DEV_BSIZE byte increments.
 *
 * 	Alan Frechette 27-June-89
 *	Added a new option "-d" to specify the dump device and the
 *	dump device offset when running savecore on a system image
 *	other then the currently running system image. This is needed
 *	because the dump device and the dump device offset of the
 *	running system image may be different from the that of the
 *	system image that crashed. Changed several routines for this
 *	to work correctly.
 *
 *	Paul Shaughnessy, 03-Feb-89
 *	Changed clear_dump() to read and write in DEV_BSIZE byte
 *	increments. Some disk drivers do not support operations on
 *	non sector boundaries.
 *
 *	Mark Parenti, 27-Feb-89
 *	Modified include of ufs/fs.h to be <ufs/fs.h> instead of
 *	dot relative through the sys link.
 *
 *	Joe Amato, 12-May-87
 *	Modified to use statfs(path, buf) instead of getmnt().
 *
 * 	Paul Shaughnessy, 08-April-87
 *	Added logic to handle saving of the core file on a NFS
 *	mounted file system.
 *
 *	Paul Shaughnessy, 19-Mar-87
 *	Added -f flag for a network crash dump. The core image for
 *	network crash client will reside in a file on the server,
 *	not a disk partition. Also fixed clearing of the dump logic.
 *
 *	pete keilty 12-Feb-87
 *	Added printf stating checking for dump. Bar
 *
 *	Paul Shaughnessy, 22-May-86
 *	Added saving of the u_area support to save_core. This
 *	required writing a new procedure save_uarea, which
 *	users raw i/o for accesses to the dump device.
 *
 *	Barb Glover, 05-May-86
 *	Changed savecore to use raw i/o to solve "I/O error" problem,
 *	because of file system read past the end of the partition.
 *	Also, append to end of elbuffer file.
 *
 *	Paul Shaughnessy, 09-Apr-86
 *	Changed initialization of namelist values because of the
 *	addition on the include file a.out.h. Added partial
 *	crash dump support, including modifying the namelist
 *	entry for elbuf. Also added a print statement to print
 *	out when saving core.
 *
 *	Barbara Glover, 05-Mar-86
 *	Store elbuf base addr at beg. of elbuffer file
 *	Added include of errlog.h for DUMPSIZE
 *
 *	Barbara Glover, 19-Feb-86
 *	Added save of error log bufffer; added -e switch to
 *	save error log buffer(only).
 *
 *	Stephen Reilly, 22-Apr-85
 * 001- Added the -c switch to enable the user to clear the dump flag
 *	on the dump device.
 *
 ***********************************************************************/

/*
 * savecore
 */
#include <stdio.h>
#ifdef vax
#include <a.out.h>
#endif vax
#ifdef mips
#include <nlist.h>
#endif mips
#include <stab.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/dump.h>
#include <ufs/fs.h>
#include <sys/errlog.h>
#include <sys/types.h>
#include <sys/un.h>
#include <elcsd.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <sys/mount.h>
#include <sys/fs_types.h>

#define	DAY	(60L*60L*24L)
#define	LEEWAY	(3*DAY)

#define BLOCK 0
#define RAW 1

#ifdef vax
#define ok(number) ((number)&0x7fffffff)
#endif vax
#ifdef mips
#define ok(number) ((number)&0x1fffffff)
#endif mips

#define SHUTDOWNLOG "/usr/adm/shutdownlog"

#define X_DUMPDEV	0
#define X_DUMPLO	1
#define X_TIME		2
#define	X_DUMPSIZE	3
#define	X_DUMPSIZE2	4
#define X_VERSION	5
#define X_PANICSTR	6
#define	X_DUMPMAG	7
#define X_FULLDUMPMAG	8
#define X_PARTDUMPMAG	9
#define	X_NUMDUMPDESC	10
#define	X_ELBUF		11
#define X_DUMPSOFTPGSZ	12
#define X_DUMPHARDPGSZ	13
#define X_SBR		14
#define X_CPU		15
#define X_PHYSMEM	16
#define NUM_IN_TAB	17
#define RAWBUFSZ	2048

#define MSIZE (NMOUNT * sizeof (struct fs_data))

struct nlist nlsystem[NUM_IN_TAB + 1];
struct nlist nl[NUM_IN_TAB +1];

char	*system;
char	*dirname;			/* directory to save dumps in */
char	*ddname;			/* name of dump device */
char	*corefile;			/* core file name for network dump */
char	*find_dev();
dev_t	dumpdev;			/* dump device */
time_t	dumptime;			/* time the dump was taken */
int	dumplo;				/* where dump starts on dumpdev */
int	dumpsize;			/* initial size of memory dumped */
int	dumpsize2;			/* additional size of memory dumped */
int	dumpdescriptors;		/* number of dump descriptor blocks */
int	dumpsoftpgsize;			/* system software pagesize */
int	dumphardpgsize;			/* system hardware pagesize */
int	dumpmag;			/* magic number in dump */
int	physmem;			/* physical memory size */
int	full_dumpmag;			/* full dump magic number */
int	partial_dumpmag;		/* partial dump magic number */
int	kerrsize;			/* size of error log buffer in pages */
int	partial_dump;			/* switch for partial or full dump */
int	old_bounds;			/* temp hold for original bounds */
time_t	now;				/* current date */
char	*path();
char 	*malloc();
char	*ctime();
char	vers[80];
char	core_vers[80];
char	panic_mesg[80];
int	panicstr;
off_t	lseek();
off_t	Lseek();
int	tstdebug = 0;
int	cflag = 0;		/* Used for the -c switch */
int	dflag = 0;		/* Used for the -d switch */
int	eflag = 0;		/* Used for the -e switch */
int	fflag = 0;		/* Used for the -f switch */
int	dirname_notlocal = -1;	/* 0 - dirname on local file system
				   1 - dirname on a NFS mounted file system */
char rbuf[RAWBUFSZ];		/* buffer for raw i/o */
struct stat dsb;

main(argc, argv)
	char **argv;
	int argc;
{
	struct fs_data fsdbuf;

	argv++; argc--;

	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {
		case 'c':
			cflag++;
			break;
		case 'd':
			dflag++;
			argc--;
			argv++;
			if(argc < 2) {
				usage();
				exit(1);
			}
			if((strncmp(argv[0], "0x", 2) == 0) ||
					(strncmp(argv[0], "0X", 2) == 0))
				sscanf(&argv[0][2], "%x", &dumpdev);
			else
				sscanf(argv[0], "%d", &dumpdev);
			argc--;
			argv++;
			if((strncmp(argv[0], "0x", 2) == 0) ||
					(strncmp(argv[0], "0X", 2) == 0))
				sscanf(&argv[0][2], "%x", &dumplo);
			else
				sscanf(argv[0], "%d", &dumplo);
			break;
		case 'e':
			eflag++;
			break;
		case 'f':
			fflag++;
			argc--;
			argv++;
			corefile = argv[0];
			if (access(corefile, 2) < 0) {
				perror(corefile);
				exit(1);
			}
			break;
		default:
			usage();
			exit(1);
		}
		argc--; argv++;
	}
	if (!eflag && !cflag && argc != 1 && argc != 2) {
		usage();
		exit(1);
	}
	/*
	 * We don't need the directory path if -e or -c flag was given -afd
	 */
	if (!eflag && !cflag) {
		dirname = argv[0];
		if (argc == 2)
			system = argv[1];
		if (access(dirname, 2) < 0) {
			perror(dirname);
			exit(1);
		}
		/*
		 * stat the dirname
		 */
		if (stat(dirname, &dsb) < 0) {
			perror(dirname);
			exit(1);
		}
		if(statfs(dirname, &fsdbuf) == 0){
			fprintf(stderr, "savecore: statfs %s failed\n", dirname);
			exit(1);
		}
		dirname_notlocal = (fsdbuf.fd_fstype == GT_NFS ? 1 : 0);
	}

	/*
	 * Initialize the name list tables.
	 */
	init_nlist();
	read_kmem();
	printf("savecore: checking for dump...");
	if (dump_exists()) {
		/*
		 * if cflag set then clear the dump flag
		 */
		if (cflag) {
			clear_dump(); 
			printf("dump cleared\n");
			exit(0);
		}

		if (partial_dump)
			printf("partial dump exists\n");
		else
			printf("full dump exists\n");
		(void) time(&now);
		check_kmem();
		log_entry();
		if (get_crashtime() && check_space()) {
			if (eflag) {
				printf("saving elbuf\n");
				save_elbuf();
				sync();
				sync();
				clear_dump();
			}
			else {
				printf("saving elbuf\n");
				save_elbuf();
				printf("saving core\n");
				save_core();
				sync();
				sync();
				clear_dump(); 
			}
		} else {
			printf("crashtime or space problem\n");
			exit(1);
		}
	}
	else
		printf("dump does not exist\n");

	exit(0);
}

usage()
{
	fprintf(stderr, "usage: savecore [-c] [-d dumpdev dumplo] [-e] [-f corename] dirname [ system ]\n");
}

init_nlist()
{
#ifdef vax
	nl[X_DUMPDEV].n_un.n_name = nlsystem[X_DUMPDEV].n_un.n_name =
		"_dumpdev";
	nl[X_DUMPLO].n_un.n_name = nlsystem[X_DUMPLO].n_un.n_name =
		"_dumplo";
	nl[X_TIME].n_un.n_name = nlsystem[X_TIME].n_un.n_name =
		"_time";
	nl[X_DUMPSIZE].n_un.n_name = nlsystem[X_DUMPSIZE].n_un.n_name =
		"_dumpsize";
	nl[X_DUMPSIZE2].n_un.n_name = nlsystem[X_DUMPSIZE2].n_un.n_name =
		"_dumpsize2";
	nl[X_VERSION].n_un.n_name = nlsystem[X_VERSION].n_un.n_name =
		"_version";
	nl[X_PANICSTR].n_un.n_name = nlsystem[X_PANICSTR].n_un.n_name =
		"_panicstr";
	nl[X_DUMPMAG].n_un.n_name = nlsystem[X_DUMPMAG].n_un.n_name =
		"_dumpmag";
	nl[X_FULLDUMPMAG].n_un.n_name = nlsystem[X_FULLDUMPMAG].n_un.n_name = 
		"_full_dumpmag";
	nl[X_PARTDUMPMAG].n_un.n_name = nlsystem[X_PARTDUMPMAG].n_un.n_name = 
		"_partial_dumpmag";
	nl[X_NUMDUMPDESC].n_un.n_name = nlsystem[X_NUMDUMPDESC].n_un.n_name =
		"_dumpdescriptors";
	nl[X_ELBUF].n_un.n_name = nlsystem[X_ELBUF].n_un.n_name =
		"_elbuf";
	nl[X_DUMPSOFTPGSZ].n_un.n_name = nlsystem[X_DUMPSOFTPGSZ].n_un.n_name =
		"_dumpsoftpgsize";
	nl[X_DUMPHARDPGSZ].n_un.n_name = nlsystem[X_DUMPHARDPGSZ].n_un.n_name =
		"_dumphardpgsize";
	nl[X_SBR].n_un.n_name = nlsystem[X_SBR].n_un.n_name =
		"_Sysmap";
	nl[X_CPU].n_un.n_name = nlsystem[X_CPU].n_un.n_name =
		"_cpudata";
	nl[X_PHYSMEM].n_un.n_name = nlsystem[X_PHYSMEM].n_un.n_name =
		"_physmem";
	nl[NUM_IN_TAB].n_un.n_name = nlsystem[NUM_IN_TAB].n_un.n_name =
		"" ;
#endif vax
#ifdef mips
	nl[X_DUMPDEV].n_name = nlsystem[X_DUMPDEV].n_name =
		"_dumpdev";
	nl[X_DUMPLO].n_name = nlsystem[X_DUMPLO].n_name =
		"_dumplo";
	nl[X_TIME].n_name = nlsystem[X_TIME].n_name =
		"_time";
	nl[X_DUMPSIZE].n_name = nlsystem[X_DUMPSIZE].n_name =
		"_dumpsize";
	nl[X_DUMPSIZE2].n_name = nlsystem[X_DUMPSIZE2].n_name =
		"_dumpsize2";
	nl[X_VERSION].n_name = nlsystem[X_VERSION].n_name =
		"_version";
	nl[X_PANICSTR].n_name = nlsystem[X_PANICSTR].n_name =
		"_panicstr";
	nl[X_DUMPMAG].n_name = nlsystem[X_DUMPMAG].n_name =
		"_dumpmag";
	nl[X_FULLDUMPMAG].n_name = nlsystem[X_FULLDUMPMAG].n_name =
		"_full_dumpmag";
	nl[X_PARTDUMPMAG].n_name = nlsystem[X_PARTDUMPMAG].n_name =
		"_partial_dumpmag";
	nl[X_NUMDUMPDESC].n_name = nlsystem[X_NUMDUMPDESC].n_name =
		"_dumpdescriptors";
	nl[X_ELBUF].n_name = nlsystem[X_ELBUF].n_name =
		"_elbuf";
	nl[X_DUMPSOFTPGSZ].n_name = nlsystem[X_DUMPSOFTPGSZ].n_name =
		"_dumpsoftpgsize";
	nl[X_DUMPHARDPGSZ].n_name = nlsystem[X_DUMPHARDPGSZ].n_name =
		"_dumphardpgsize";
	nl[X_SBR].n_name = nlsystem[X_SBR].n_name =
		"_Sysmap";
	nl[X_CPU].n_name = nlsystem[X_CPU].n_name =
		"_cpudata";
	nl[X_PHYSMEM].n_name = nlsystem[X_PHYSMEM].n_name =
		"_physmem";
	nl[NUM_IN_TAB].n_name = nlsystem[NUM_IN_TAB].n_name =
		"" ;
#endif mips
}

int
dump_exists()
{
	register int dumpfd;
	int word;
	int seektot,seekval,seekrem;
	char *rptr;

	dumpfd = Open(ddname, 0);
	seektot = (off_t) (dumplo + ok(nlsystem[X_DUMPMAG].n_value));
	seekval = (seektot / DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t) seekval, 0);
	Read(dumpfd, rbuf, round(seekrem + 4));
	close(dumpfd);
	
	rptr = rbuf + seekrem;
	word = *(int *)rptr;
	if (word == full_dumpmag)  {
		partial_dump =  0;
		return(1);
	}
	else if (word == partial_dumpmag) {
		partial_dump =  1;
		return(1);
	}
	else
		return(0);
}

clear_dump()
{
	register int dumpfd;
	int seektot,seekval,seekrem;
	char junkbuf[DEV_BSIZE], *rptr;

	/* Get the initial "dumpsize" from the dump device */
	dumpfd = Open(ddname, 2);
	bzero(junkbuf, DEV_BSIZE);
	seektot = (off_t)(dumplo + ok(nlsystem[X_DUMPSIZE].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t) seekval, 0);
	Read(dumpfd, rbuf, round(seekrem + sizeof(dumpsize)));
	rptr = rbuf + seekrem;
	dumpsize = *(int *)rptr;

	/* Get the additional "dumpsize2" from the dump device */
	seektot = (off_t)(dumplo + ok(nlsystem[X_DUMPSIZE2].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t) seekval, 0);
	Read(dumpfd, rbuf, round(seekrem + sizeof(dumpsize2)));
	rptr = rbuf + seekrem;
	dumpsize2 = *(int *)rptr;

	/* Get the number of "dumpdescriptors" from the dump device */
	seektot = (off_t)(dumplo + ok(nlsystem[X_NUMDUMPDESC].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t) seekval, 0);
	Read(dumpfd, rbuf, round(seekrem + sizeof(dumpdescriptors)));
	rptr = rbuf + seekrem;
	dumpdescriptors = *(int *)rptr;

	/* Clear the dump itself */
	seektot = (off_t) (dumplo + ok(nlsystem[X_DUMPMAG].n_value));
	seekval = (seektot / DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t) seekval, 0);
	Write(dumpfd, junkbuf, DEV_BSIZE);

	/* Clear the dump descriptor at the end of the dump */
	seekval = dumplo + (dumpsize+dumpsize2+dumpdescriptors) * DEV_BSIZE;
	Lseek(dumpfd, (off_t) seekval, 0);
	Write(dumpfd, junkbuf, DEV_BSIZE);
	close(dumpfd);
}

char *
find_dev(dev, type, raw)
	register dev_t dev;
	register int type;
	int raw;
{
	register DIR *dfd = opendir("/dev");
	struct direct *dir;
	struct stat statb;
	static char devname[MAXPATHLEN + 1];
	char *dp;

	strcpy(devname, "/dev/");
	while ((dir = readdir(dfd))) {
		strcpy(devname + 5, dir->d_name);
		if (stat(devname, &statb)) {
			perror(devname);
			continue;
		}
		if ((statb.st_mode&S_IFMT) != type)
			continue;
		if (dev == statb.st_rdev) {
			closedir(dfd);
			dp = (char *)malloc(strlen(devname)+2);

			if (raw) {
				strcpy(dp, "/dev/r");
				strcat(dp, &devname[5]);
			}
			else {
				strcpy(dp, devname);
			}
			return dp;
		}
	}
	closedir(dfd);
	fprintf(stderr, "savecore: Can't find device %d,%d\n",
		major(dev), minor(dev));
	exit(1);
	/*NOTREACHED*/
}

read_kmem()
{
	int kmem;
	FILE *fp;
	register char *cp;
	int seektot, seekval, seekrem;
	char *rptr;
	int dumpfd;

	/*
	 * Get the namelist lookups for the system that is running.
 	 * Savecore assumes the running system image is "/vmunix".
	 */
	nlist("/vmunix", nl);
	if (nl[X_DUMPDEV].n_value == 0) {
		fprintf(stderr, "savecore: /vmunix: dumpdev not in namelist\n");
		exit(1);
	}
	if (nl[X_DUMPLO].n_value == 0) {
		fprintf(stderr, "savecore: /vmunix: dumplo not in namelist\n");
		exit(1);
	}
	if (nl[X_FULLDUMPMAG].n_value == 0) {
		fprintf(stderr, "savecore: /vmunix: full_dumpmag not in namelist\n");
		exit(1);
	}
	if (nl[X_PARTDUMPMAG].n_value == 0) {
		fprintf(stderr, "savecore: /vmunix: partial_dumpmag not in namelist\n");
		exit(1);
	}
	if (nl[X_VERSION].n_value == 0) {
		fprintf(stderr, "savecore: /vmunix: version not in namelist\n");
		exit(1);
	}
	if (nl[X_PHYSMEM].n_value == 0) {
		fprintf(stderr, "savecore: /vmunix: physmem not in namelist\n");
		exit(1);
	}
	/*
	 * Read the dump device, the dump device offset, the full
	 * dump magic number, the partial dump magic number, the
	 * physical memory size, and the version string from the 
	 * running system image "/vmunix".
	 */
	kmem = Open("/dev/kmem", 0);
	if(!dflag) {
		Lseek(kmem, (long)nl[X_DUMPDEV].n_value, 0);
		Read(kmem, (char *)&dumpdev, sizeof (dumpdev));
		Lseek(kmem, (long)nl[X_DUMPLO].n_value, 0);
		Read(kmem, (char *)&dumplo, sizeof (dumplo));
		Lseek(kmem, (long)nl[X_VERSION].n_value, 0);
		Read(kmem, vers, sizeof (vers));
		if(tstdebug) {
	    	    printf("dumpdev = DEC(%d) HEX(%x)\n", dumpdev, dumpdev);
	    	    printf("dumplo  = DEC(%d) HEX(%x)\n", dumplo, dumplo);
		}
	}
	Lseek(kmem, (long)nl[X_FULLDUMPMAG].n_value, 0);
	Read(kmem, (char *)&full_dumpmag, sizeof (full_dumpmag));
	Lseek(kmem, (long)nl[X_PARTDUMPMAG].n_value, 0);
	Read(kmem, (char *)&partial_dumpmag, sizeof (partial_dumpmag));
	Lseek(kmem, (long)nl[X_PHYSMEM].n_value, 0);
	Read(kmem, (char *)&physmem, sizeof (physmem));
	close(kmem);
	/*
	 * Figure out the name of the dump device.
	 */
	if (fflag) {
		ddname = corefile;
		dumplo = 0;
	}
	else {
		ddname = find_dev(dumpdev, S_IFBLK, RAW); 
		dumplo *= DEV_BSIZE;
	}

	/*
	 * Get the namelist lookups for the system that crashed.
	 * Read all the namelist elements from the dump device.
	 */
	if (system) 
		nlist(system, nlsystem);
	else
		nlist("/vmunix", nlsystem);

	if (nlsystem[X_DUMPDEV].n_value == 0) {
		fprintf(stderr, "savecore: %s: dumpdev not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_DUMPLO].n_value == 0) {
		fprintf(stderr, "savecore: %s: dumplo not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_TIME].n_value == 0) {
		fprintf(stderr, "savecore: %s: time not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_DUMPSIZE].n_value == 0) {
		fprintf(stderr, "savecore: %s: dumpsize not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_DUMPSIZE2].n_value == 0) {
		fprintf(stderr, "savecore: %s: dumpsize2 not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_VERSION].n_value == 0) {
		fprintf(stderr, "savecore: %s: version not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_PANICSTR].n_value == 0) {
		fprintf(stderr, "savecore: %s: panicstr not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_DUMPMAG].n_value == 0) {
		fprintf(stderr, "savecore: %s: dumpmag not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_FULLDUMPMAG].n_value == 0) {
		fprintf(stderr, "savecore: %s: full_dumpmag not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_PARTDUMPMAG].n_value == 0) {
		fprintf(stderr, "savecore: %s: partial_dumpmag not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_NUMDUMPDESC].n_value == 0) {
		fprintf(stderr, "savecore: %s: dumpdescriptors not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_ELBUF].n_value == 0) {
		fprintf(stderr, "savecore: %s: elbuf not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_DUMPSOFTPGSZ].n_value == 0) {
		fprintf(stderr, "savecore: %s: dumpsoftpgsize not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_DUMPHARDPGSZ].n_value == 0) {
		fprintf(stderr, "savecore: %s: dumphardpgsize not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}
	if (nlsystem[X_PHYSMEM].n_value == 0) {
		fprintf(stderr, "savecore: %s: physmem not in namelist\n",
			system ? system : "/vmunix");
		exit(1);
	}

	/* Get the "dumpmag" value */
	dumpfd = Open(ddname, 0);
	seektot = (off_t)(dumplo + ok(nlsystem[X_DUMPMAG].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t) seekval, 0);
	Read(dumpfd, rbuf, round(seekrem + sizeof(dumpmag)));
	rptr = rbuf + seekrem;
	dumpmag = *(int *)rptr;

	/* Get the "dumpsoftpgsize" value */
	seektot = (off_t)(dumplo + ok(nlsystem[X_DUMPSOFTPGSZ].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t) seekval, 0);
	Read(dumpfd, rbuf, round(seekrem + sizeof(dumpsoftpgsize)));
	rptr = rbuf + seekrem;
	dumpsoftpgsize = *(int *)rptr;

	/* Get the "dumphardpgsize" value */
	seektot = (off_t)(dumplo + ok(nlsystem[X_DUMPHARDPGSZ].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t) seekval, 0);
	Read(dumpfd, rbuf, round(seekrem + sizeof(dumphardpgsize)));
	rptr = rbuf + seekrem;
	dumphardpgsize = *(int *)rptr;

	/* Get the "physmem" value */
	seektot = (off_t)(dumplo + ok(nlsystem[X_PHYSMEM].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t) seekval, 0);
	Read(dumpfd, rbuf, round(seekrem + sizeof(physmem)));
	rptr = rbuf + seekrem;
	physmem = *(int *)rptr;

	/* Get the "vers" string */
	if(dflag) {
		seektot = (off_t)(dumplo + ok(nlsystem[X_VERSION].n_value));
		seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
		seekrem = seektot % DEV_BSIZE;
		Lseek(dumpfd, (off_t) seekval, 0);
		Read(dumpfd, rbuf, round(seekrem + sizeof(vers)));
		rptr = rbuf + seekrem;
		strcpy(vers, rptr, sizeof(vers));
	}
	close(dumpfd);
}

check_kmem()
{
	int seektot, seekval, seekrem;
	char *rptr;
	int dumpfd;

	dumpfd = Open(ddname, 0);
	seektot = (off_t)(dumplo+ok(nlsystem[X_VERSION].n_value));
	seekval = (seektot / DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t)seekval, 0);
	Read(dumpfd, rbuf, round(sizeof(core_vers) + seekrem)); 
	rptr = rbuf + seekrem;
	strcpy(core_vers, rptr, sizeof(core_vers));
	close(dumpfd);

	if (strncmp(vers, core_vers, sizeof(vers)))
		fprintf(stderr,
		   "savecore: Warning: vmunix version mismatch:\n\t%sand\n\t%s",
		   vers, core_vers);

	dumpfd = Open(ddname, 0);
	seektot = (off_t)(dumplo + ok(nlsystem[X_PANICSTR].n_value));
	seekval = (seektot / DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t)seekval, 0);
	Read(dumpfd, rbuf, round(sizeof(panicstr) + seekrem));
	rptr = rbuf + seekrem;
	panicstr = *(int *)rptr;

	if (panicstr) {
	    close(dumpfd);
	    dumpfd = Open(ddname, 0);
	    seektot = (off_t)(dumplo + ok(panicstr));
	    seekval = (seektot / DEV_BSIZE) * DEV_BSIZE;
	    seekrem = seektot % DEV_BSIZE;
	    Lseek(dumpfd, (off_t)seekval, 0);
	    Read(dumpfd, rbuf, round(sizeof(panic_mesg) + seekrem));
	    rptr = rbuf + seekrem;
	    strcpy(panic_mesg, rptr, sizeof(panic_mesg));
	}
	close(dumpfd);
}

get_crashtime()
{
	int dumpfd;
	int seektot, seekval, seekrem;
	char *rptr;
	time_t clobber = (time_t)0;

	if (system)
		return (1);
	dumpfd = Open(ddname, 0);
	seektot = (off_t)(dumplo + ok(nlsystem[X_TIME].n_value));
	seekval = (seektot / DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dumpfd, (off_t) seekval, 0);
	Read(dumpfd, rbuf, round(seekrem + sizeof(dumptime)));
	rptr = rbuf + seekrem;
	dumptime = *(int *)rptr;
	close(dumpfd);
	if (dumptime == 0)
		return (0);
	printf("System went down at %s", ctime(&dumptime));
	if (dumptime < now - LEEWAY || dumptime > now + LEEWAY) {
		printf("Dump time is unreasonable\n");
		return (0);
	}
	return (1);
}

char *
path(file)
	char *file;
{
	register char *cp = (char *)malloc(strlen(file) + strlen(dirname) + 2);

	(void) strcpy(cp, dirname);
	(void) strcat(cp, "/");
	(void) strcat(cp, file);
	return (cp);
}

check_space()
{
	register char *ddev;
	int dfd, spacefree;
	struct fs fs;
	struct fs *fsptr;
	int seektot, seekval, seekrem;
	char *rptr;

	if (dirname_notlocal)
		return (1);
	ddev = find_dev(dsb.st_dev, S_IFBLK, BLOCK);
	dfd = Open(ddev, 0); 
	seektot = (long)(SBLOCK * DEV_BSIZE);
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(dfd, (long) seekval, 0);
	Read(dfd, rbuf, round(sizeof(fs) + seekrem));
	rptr = rbuf + seekrem;
	fsptr = (struct fs *)rbuf;
	close(dfd);
	spacefree = fsptr->fs_cstotal.cs_nbfree * fsptr->fs_bsize / 1024;
	if (read_number("minfree") > spacefree) {
		fprintf(stderr,
		   "savecore: Dump omitted, not enough space on device\n");
		return (0);
	}
	if (fsptr->fs_cstotal.cs_nbfree * fsptr->fs_frag + fsptr->fs_cstotal.cs_nffree <
	    fsptr->fs_dsize * fsptr->fs_minfree / 100)
		fprintf(stderr,
			"Dump performed, but free space threshold crossed\n");
	return (1);
}

read_number(fn)
	char *fn;
{
	char lin[80];
	register FILE *fp;

	if ((fp = fopen(path(fn), "r")) == NULL)
		return (0);
	if (fgets(lin, 80, fp) == NULL) {
		fclose(fp);
		return (0);
	}
	fclose(fp);
	return (atoi(lin));
}

save_core()
{
	register int n, blk, bc, i, j, myblks, totaldumpsize;
	char buffer[32*DEV_BSIZE];
	register char *cp = buffer;
	register int ifd, ofd, bounds;
	register FILE *fp;
	register blks_out;			/* count of blocks written */
	int num_to_print;			/* After this many pages are
						   written, print a message */
	int seektot, seekval, seekrem;
	char *rptr;
	short fstread = 1;
	int successflag = 1;

	old_bounds = bounds = read_number("bounds");
	ifd = Open(system?system:"/vmunix", 0);
	sprintf(cp, "vmunix.%d", bounds);
	ofd = Create(path(cp), 0644);
	while((n = Read(ifd, cp, BUFSIZ)) > 0)
		Write(ofd, cp, n);
	close(ifd);
	close(ofd);

	/* Get the initial "dumpsize" from the dump device */
	ifd = Open(ddname, 0);
	seektot = (off_t)(dumplo + ok(nlsystem[X_DUMPSIZE].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(ifd, (off_t) seekval, 0);
	Read(ifd, rbuf, round(seekrem + sizeof(dumpsize)));
	rptr = rbuf + seekrem;
	dumpsize = *(int *)rptr;

	/* Get the additional "dumpsize2" from the dump device */
	seektot = (off_t)(dumplo + ok(nlsystem[X_DUMPSIZE2].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(ifd, (off_t) seekval, 0);
	Read(ifd, rbuf, round(seekrem + sizeof(dumpsize2)));
	rptr = rbuf + seekrem;
	dumpsize2 = *(int *)rptr;

	/* Get the number of "dumpdescriptors" from the dump device */
	seektot = (off_t)(dumplo + ok(nlsystem[X_NUMDUMPDESC].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(ifd, (off_t) seekval, 0);
	Read(ifd, rbuf, round(seekrem + sizeof(dumpdescriptors)));
	rptr = rbuf + seekrem;
	dumpdescriptors = *(int *)rptr;

  	/* Set the total dump size of the dump */
	totaldumpsize = dumpsize + dumpsize2;

	/* Calculate "num_to_print" to print a message every 20% */
	num_to_print = (totaldumpsize / 32) * 0.2;
	blks_out = myblks = 0;

	sprintf(cp, "vmcore.%d", bounds);
	ofd = Create(path(cp), 0644);

	seektot = (off_t)dumplo;
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(ifd, (off_t)seekval, 0);
	printf("Saving %d bytes of image in vmcore.%d\n", 
		DEV_BSIZE*totaldumpsize, bounds);

	/*
	 * Write out all the KERNEL PAGES or all of PHYSICAL MEMORY 
	 * to the "vmcore" file depending upon the initial size of 
	 * the dump specified in "dumpsize". For a full dump "dumpsize"
	 * will be set to the size of physical memory in blocks and for 
	 * a partial selective dump "dumpsize" will be set to the size 
	 * of the kernel image in blocks.
	 */
	if(tstdebug) {
	    if(partial_dump)
	        printf("KERNEL PAGES: size in blocks = %d\n",dumpsize);
	    else
	        printf("PHYSICAL MEMORY: size in blocks = %d\n",dumpsize);
	}
	while (dumpsize > 0) {
	    blk = (dumpsize > 32 ? 32 : dumpsize);
	    bc = blk * DEV_BSIZE;
	    n = Read(ifd, cp, bc);
	    if (n == 0) break;
    	    if (fstread) {
	 	cp += seekrem;
		Write(ofd, cp, n-seekrem);
		dumpsize -= (n-seekrem)/DEV_BSIZE;
		totaldumpsize -= (n-seekrem)/DEV_BSIZE;
		fstread = 0;
	    }
	    else {
		if (n == (32 * DEV_BSIZE)) {
		    Write(ofd, cp, n);
		}
		else {	/* last read */
		    if (seekrem) {
			Write(ofd, cp, (n - DEV_BSIZE) + seekrem);
		    }
		    else {
			Write(ofd, cp, n);
		    }
		}
		dumpsize -= n/DEV_BSIZE;
		totaldumpsize -= n/DEV_BSIZE;
	    }
    	    blks_out++;
	    if ((blks_out % num_to_print == 0) && (totaldumpsize > 32)) {
		    printf("saving core, %d bytes remaining\n",
				totaldumpsize*DEV_BSIZE);
		    blks_out = 0;
	    }
	}

	/*
	 * Write out the SELECTED PAGES to the "vmcore" file. This is
	 * only executed if a partial selective dump was performed. The
	 * number of dump descriptors is stored in "dumpdescriptors".
	 * Each dump descriptor block contains the page frame numbers 
	 * of the next set of cluster pages of physical memory to dump.
	 */
	if(tstdebug && partial_dump) {
	    printf("SELECTED PAGES: size in blocks = %d\n",dumpsize2);
	    printf("dumpdescriptors = %d hardpgsize = %d softpgsize = %d\n",
			dumpdescriptors,dumphardpgsize,dumpsoftpgsize);
	}
	for (i=0; i<dumpdescriptors && partial_dump; i++) {
	    n = Read (ifd, (char *)&dumpdesc[0], DEV_BSIZE);
	    if (n == 0 || !valid_dumpdesc(1)) {
		successflag = 0;
		break;
	    }
	    if(tstdebug) {
		printf("\nDUMP OF DUMP DESCRIPTOR #%d\n",i);
		for(j=0; j<NPFNDESC; j += 8) {
		    printf("PFN (%d-%d): %d %d %d %d %d %d %d %d\n", j, j+7,
			dumpdesc[j], dumpdesc[j+1], dumpdesc[j+2],
			dumpdesc[j+3], dumpdesc[j+4], dumpdesc[j+5],
			dumpdesc[j+6], dumpdesc[j+7]); 
		}
	    }
	    for (j=0; j<NPFNDESC; j++) {
		if (dumpdesc[j] == 0)
		    break;
		Lseek (ofd, (off_t)(dumpdesc[j] * dumphardpgsize), 0);
		n = Read (ifd, cp, dumpsoftpgsize);
	        if (n == 0) break;
		Write (ofd, cp, dumpsoftpgsize);
		myblks += (dumpsoftpgsize / DEV_BSIZE);
		dumpsize2 -= (dumpsoftpgsize / DEV_BSIZE);
		totaldumpsize -= (dumpsoftpgsize / DEV_BSIZE);
		if ((myblks % 32) && (myblks <= 32))
		    continue;
		blks_out++;
		myblks = 0;
	        if ((blks_out % num_to_print == 0) && (totaldumpsize > 32)) {
		    printf("saving core, %d bytes remaining\n",
				totaldumpsize*DEV_BSIZE);
		    blks_out = 0;
	        }
	    }
	}
	/*
	 * Check for a final dump descriptor block of magic numbers
	 * which signifies a successful dump occurred.
	 */
	n = Read (ifd, (char *)&dumpdesc[0], DEV_BSIZE);
	if (n == 0 || !successflag || !valid_dumpdesc(2)) {
	    printf("savecore: bad dump due to a dump failure\n");
	    successflag = 0;
	}
	log_crash_entry(successflag);
	close(ifd);
	close(ofd);
	fp = fopen(path("bounds"), "w");
	fprintf(fp, "%d\n", bounds+1);
	fclose(fp);
}

char *days[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec"
};

valid_dumpdesc(which)
int which;
{
	int i;

	for(i=0; i<NPFNDESC; i++) {
	    switch(which) {
	    case 1:
	    	if((dumpdesc[i] < 0) || (dumpdesc[i] >= physmem))
		    return(0);
		break;
	    case 2:
	        if(dumpdesc[i] != dumpmag)
		    return(0);
		break;
	    default:
		break;
	    }
	}
	return(1);
}

log_crash_entry(success)
int success;
{
	register FILE *fp;
	struct tm *tm, *localtime();
	int bounds;

	tm = localtime(&now);
	fp = fopen("/usr/adm/shutdownlog", "a");
	if (fp == 0)
		return;
	fseek(fp, 0L, 2);
	fprintf(fp, "%02d:%02d  %s %s %2d, %4d.  ", tm->tm_hour,
		tm->tm_min, days[tm->tm_wday], months[tm->tm_mon],
		tm->tm_mday, tm->tm_year + 1900);
	bounds = read_number("bounds");
	if(partial_dump)
		fprintf(fp, "Partial Dump (vmunix.%d vmcore.%d) %s\n",
		bounds, bounds, (success) ? "succeeded" : "failed");
	else
		fprintf(fp, "Full Dump (vmunix.%d vmcore.%d) %s\n",
		bounds, bounds, (success) ? "succeeded" : "failed");
	fclose(fp);
}

log_entry()
{
	FILE *fp;
	struct tm *tm, *localtime();

	tm = localtime(&now);
	fp = fopen("/usr/adm/shutdownlog", "a");
	if (fp == 0)
		return;
	fseek(fp, 0L, 2);
	fprintf(fp, "%02d:%02d  %s %s %2d, %4d.  Reboot", tm->tm_hour,
		tm->tm_min, days[tm->tm_wday], months[tm->tm_mon],
		tm->tm_mday, tm->tm_year + 1900);
	if (panicstr)
		fprintf(fp, " after panic: %s\n", panic_mesg);
	else
		putc('\n', fp);
	fclose(fp);
}

/*
 * Versions of std routines that exit on error.
 */

Open(name, rw)
	char *name;
	int rw;
{
	int fd;

	if ((fd = open(name, rw)) < 0) {
		perror(name);
		exit(1);
	}
	return fd;
}

Read(fd, buff, size)
	int fd, size;
	char *buff;
{
	int ret;

	if ((ret = read(fd, buff, size)) < 0) {
		perror("read");
		exit(1);
	}
	return ret;
}

off_t
Lseek(fd, off, flag)
	int fd, flag;
	long off;
{
	long ret;

	if ((ret = lseek(fd, off, flag)) == -1L) {
		perror("lseek");
		exit(1);
	}
	return ret;
}

Create(file, mode)
	char *file;
	int mode;
{
	register int fd;

	if ((fd = creat(file, mode)) < 0) {
		perror(file);
		exit(1);
	}
	return fd;
}

Write(fd, buf, size)
	int fd, size;
	char *buf;
{

	if (write(fd, buf, size) < size) {
		perror("write");
		exit(1);
	}
}

struct elcfg elcfg;

save_elbuf()
{
	int ifd, ofd;
	int kerrsize_old;
	register int n;
	char buffer[32*DEV_BSIZE];
	register char *cp = buffer;
	unsigned long elbufaddr;
	int new_loc;		      /* New location of elbuf (partial dump) */
	char svelbuf[MAX_PATH + 10];  /* store elbuffer pathname */
	extern int errno;
	int seektot, seekval, seekrem;
	char *rptr;
	short fstread = 1;

	/* Now dump error log buffer into a seperate file */

	kerrsize = EL_DUMPSIZE / DEV_BSIZE;
	if ((EL_DUMPSIZE % DEV_BSIZE) != 0)
		kerrsize++;

	kerrsize_old = kerrsize;

	ifd = Open(ddname, 0);

	seektot = (off_t)(dumplo + ok(nlsystem[X_DUMPSIZE].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(ifd, (off_t) seekval, 0);
	Read(ifd, rbuf, round(seekrem + sizeof(dumpsize)));
	rptr = rbuf + seekrem;
	dumpsize = *(int *)rptr;

	seektot = (off_t)(dumplo + ok(nlsystem[X_ELBUF].n_value));
	seekval = (seektot/DEV_BSIZE) * DEV_BSIZE;
	seekrem = seektot % DEV_BSIZE;
	Lseek(ifd, (off_t) seekval, 0);

	/*
	 * Dump the kernel error message buffer into the file
	 * elbuffer. The pathname of the file is determined by
	 * calling the subroutine rdcfg, which reads in the
	 * error logger config file.
	 */

	rdcfg();

	sprintf(svelbuf, "%s%s", elcfg.elpath, "/elbuffer");
	ofd = open(svelbuf, O_WRONLY|O_CREAT|O_APPEND, 0644);
	if (ofd < 0) {
		if (errno != ENOENT) {
			fprintf(stderr, "savecore: error opening %s\n",svelbuf);
	    		fprintf(stderr,"savecore: elbuf not saved\n");
			return(-1);
		}
		else {	/* try single user directory */
			sprintf(svelbuf, "%s%s", elcfg.supath, "/elbuffer");
			ofd = open(svelbuf, O_WRONLY|O_CREAT, 0644);
			if (ofd < 0) {
				fprintf(stderr, "savecore: fatal error opening %s\n", svelbuf);
	    			fprintf(stderr,"savecore: elbuf not saved\n");
				return(-1);
			}
		}
	}
			
	printf("Saving %d bytes of image in %s\n", DEV_BSIZE*kerrsize_old, svelbuf);

	/* First tack on address of elbufaddr */
	elbufaddr = nlsystem[X_ELBUF].n_value; /* don't use ok macro */

	Write(ofd, &elbufaddr, sizeof(elbufaddr));
	while (kerrsize_old > 0) {
            n = Read(ifd, cp, (kerrsize_old > 32 ? 32 : kerrsize_old)*DEV_BSIZE);
	    if (fstread) {	/* first read */
		cp += seekrem;
		Write(ofd, cp, n-seekrem);
		kerrsize_old -= (n-seekrem)/DEV_BSIZE;
		fstread = 0;
	    }
	    else {
		if (n == (32 * DEV_BSIZE)) {
		    Write(ofd, cp, n);
		}
		else {	/* last read */
		    if (seekrem) {
			Write(ofd, cp, (n - DEV_BSIZE) + seekrem);
		    }
		    else {
			Write(ofd, cp, n);
		    }
		}
		kerrsize_old -= n/DEV_BSIZE;
	    }
	}
	close(ifd);
	close(ofd);
}

int rdcfg()
{
	int i, j;
	FILE *cfp;
	char *cp, *cp2;
	char line[256];
	int dataflg = 0;

	cfp = fopen("/etc/elcsd.conf","r");
	if (cfp == NULL) {
	    fprintf(stderr,"savecore: error opening elcsd.conf\n");
	    fprintf(stderr,"savecore: elbuf not saved\n");
	    return(-1);
	}
	i = 0;
	while (fgets(line,sizeof(line),cfp) != NULL) {
	    cp = line;
	    if (*cp == '#' && dataflg == 0)
		continue;
	    if (*cp == '}')
	        break;
	    if (*cp == '{') {
		dataflg++;
		continue;
	    }
	    if (dataflg > 0) {
	        while (*cp == ' ' || *cp == '\t')
		    cp++;
	        if (*cp == '#') {
	            if (i < 1) {
		        fprintf(stderr,"savecore: error reading elcsd.conf\n");
	    		fprintf(stderr,"savecore: elbuf not saved\n");
			(void)fclose(cfp);
	                return(-1);
		    }
	            else {
		        line[0] = '\0';
		        fct(i,line);
		        i++;
		        continue;
		    }
	        }
	        cp2 = cp;
	        while (*cp2 != ' ' && *cp2 != '\t' &&
		       *cp2 != '\n' && *cp2 != '#') 
		    cp2++;
	        *cp2 = '\0';
	        fct(i,cp);
	        i++;
	    }
	}
	(void)fclose(cfp);
}

fct(i,cp)
int i;
char *cp;
{
	int num;

	switch (i) {
	case 0:
	    num = atoi(cp);
	    elcfg.status = num;
	    break;
	case 1:
	    num = atoi(cp);
	    num *= DEV_BSIZE;
	    elcfg.limfsize = num;
	    break;
	case 2:
	    (void)strcpy(elcfg.elpath,cp);
	    break;
	case 3:
	    (void)strcpy(elcfg.bupath,cp);
	    break;
	case 4:
	    (void)strcpy(elcfg.supath,cp);
	    break;
	case 5:
	    (void)strcpy(elcfg.rhpath,cp);
	    break;
	case 6:
	    (void)strcpy(elcfg.rhostn,cp);
	    break;
	}
}
round(num)
int num;
{
	int val, rem;
	val = num/DEV_BSIZE;
	rem = num % DEV_BSIZE;
	if (rem > 0) {
		val++;
	}
	val *= DEV_BSIZE;
	if (val > RAWBUFSZ) {
		fprintf(stderr, "Internal error: rbuf too small\n");
		exit(-1);
	}
	return(val);
}
