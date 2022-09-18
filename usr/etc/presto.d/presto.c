#ifndef lint
static	char	*sccsid = "@(#)presto.c	4.2	(ULTRIX)	10/8/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by			*
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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1990 Legato Systems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 *
 *   Modification history:
 *
 *  25 Aug 90 -- chet
 *	Prestoserve V2.1 stuff.
 *
 *  25 May 90 -- chet
 *	Added this file; it was derived from Legato sources.
 *
 */

/*
 * Presto control program
 * usage
 *  local:  presto [-pRLlFv] [-s size] [-u|d [filesystem ...]]
 *  remote: presto -h hostname [-pLlv] [-s size] [-u|d]
 *
 *	l - will display "mount" like list of accelerated filesystems.
 *	L - like above, but more information.
 *	p - print presto statistics
 *	u - set state to up. If blkdev (directory) specifed, also enable that
 *	    particular blkdev or filesystem list, else enable
 *	    all mounted filesystems.
 *	d - set state to down. If blkdev (directory) specified, disable
 *	    that particular blkdev or filesystem list after flushing their
 *	    blocks from the cache, else disable
 *	    all filesystems and blkdevs after flushing cache.
 *	R - reset presto
 *	F - flush all dirty presto buffers
 *	h - do it to a remote machine
 *	s - set the cache size
 *	v - verbose (semi-documented)
 *	f - override presto's default device name (undocumented)
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <fcntl.h>
#include <rpc/rpc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <dirent.h>
#include <sys/fs_types.h>
#include <strings.h>
#include <sys/mount.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#ifdef DEBUG
#include "prestoioctl.h"
#else
#include <sys/prestoioctl.h>
#endif
#include "prestoctl.h"

extern int errno;

#define REMOTE
#ifndef REMOTE
#define CLIENT int
#endif

struct timeval TIMEOUT = { 25, 0 };

struct vallist {
	char *value;
	struct vallist *next;
} Vallist;

char *prioctl_to_str[]  = {
	"NOOP",
	"PRGETSTATUS",
	"PRSETSTATE",
	"PRSETMEMSZ",
	"PRRESET",
	"PRENABLE",
	"PRDISABLE",
	"PRNEXTOPRTAB",		/* PRNEXTOPRTAB - old style prtab */
	"PRGETOPRTAB",		/* PRGETOPRTAB  - old style prtab */
	"PRNEXTUPRTAB",		/* PRNEXTUPRTAB - new style, user prtab */
	"PRGETUPRTAB",		/* PRGETUPRTAB  - new style, user prtab */
	"PRFLUSH"
};

void handle_machine();
void list();
void usage();
void enabledisable();
void print_status();
void print_interval();
void show_local_service();
void show_remote_service();
void printio();
void printstats();
CLIENT *openclnt();

char	*Myname;		/* name progam invoked under */
char	*rhost;			/* remote host name */
int	vflag;			/* verbose output */
int	pflag;			/* print information */
int	lflag;			/* list all the accelerated filesystems */
int	Lflag;			/* like above, but more information format */
int	dflag;			/* set state to down */
int	Fflag;			/* flush dirty presto buffers */
int	Rflag;			/* reset presto */
int	sflag;			/* size presto memory */
int	uflag;			/* set state to up */
int	hflag;			/* do it all to a remote host */
int	fflag;			/* default file over-ridden */
char	*prdev = PRDEV;		/* name of generic character presto device */
int	memsize;		/* memory size given with -s option */

struct	fs_data *mountbuffer;
#define MSIZE (NMOUNT*sizeof(struct fs_data))

main(argc, argv)
	int argc;
	char *argv[];
{
	int prfd = -1;	/* local presto device */
	struct vallist *hlist;
	char *opts;

	Myname = argv[0];
	argc--;
	argv++;

	while (argc > 0 && **argv == '-') {
		opts = &argv[0][1];
		while (*opts) {
			switch (*opts++) {
			      case 'l':
				lflag++;
				break;

			      case 'L':
				Lflag++;
				lflag++;
				break;

			      case 'p':
				pflag++;
				break;

			      case 'd':
				dflag++;
				break;

			      case 'u':
				uflag++;
				break;

			      case 'F':
				Fflag++;
				break;

			      case 'R':
				Rflag++;
				break;

			      case 'v':
				vflag++;
				break;

			      case 's':
				if (*opts || argc < 2) {
					usage();
					/* NOTREACHED */
				}
				sflag++;
				argv++;
				argc--;
				memsize = getnum(*argv);
				break;

			      case 'f':
				if (*opts || argc < 2) {
					usage();
					/* NOTREACHED */
				}
				fflag++;
				argv++;
				argc--;
				prdev = *argv;
				break;

			      case 'h':
#ifdef REMOTE
				if (*opts || argc < 2) {
					usage();
					/* NOTREACHED */
				}
				hflag++;
				argv++;
				argc--;
				rhost = *argv;
				break;
#else
				(void) fprintf(stderr,
				    "Remote operation not supported\n");
				exit(2);
#endif /* REMOTE */

			      default:
				usage();
				/* NOTREACHED */
			}
		}
		argv++;
		argc--;
	}

	/*
	 * If devices given, must have
	 * exactly one of -u or -d specified.
	 */
	if (argc > 0 && !((dflag != 0) ^ (uflag != 0))) {
		usage();
		/* NOTREACHED */
	}

	if (vflag && !lflag)
		pflag++;

	if (hflag) {
		if (fflag) {
			(void) fprintf(stderr,
"%s: warning: -f ignored; cannot override remote device names.\n", Myname);
		}
		if (argc > 0 && (dflag || uflag)) {
			(void) fprintf(stderr,
"%s: warning: arguments to -d and -u are ignored when -h is specified.\n",
			    Myname);
		}
		hlist = &Vallist;
		hlist->value = rhost;
		hlist->next = (struct vallist *)NULL;
	} else if (pflag || uflag || dflag || Fflag || Rflag || sflag || 
		   !lflag || access(prdev, R_OK) == 0) {
		/*
		 * We either will be doing one of [pudFRs] (which obviously
		 * require the device) or none of [pudFRsl] (which will
		 * require the device for the basic status).  If we have -l
		 * and no [pudFRs], then we want to open the device for local
		 * device presto-ization information if it exists.
		 */
		prfd = openpr(prdev);
		if (prfd == -1) {
			exit(2);
		}
	} else {
		prfd = -1;
	}

	if (hflag) {
		/*
		 * Maybe more than one server with Prestoserve
		 */
		for (; hlist != NULL; hlist = hlist->next) {
			handle_machine(hlist->value, -1, 0, (char **)NULL);
		}
	} else {
		handle_machine((char *)NULL, prfd, argc, argv);
	}
	exit(0);
}

void
handle_machine(hname, prfd, argc, argv)
	char *hname;
	int prfd;
	int argc;
	char *argv[];
{
	struct presto_status status;
	CLIENT *clnt;

#ifdef REMOTE
	if (hname != NULL) {
		if (pflag || !(uflag || dflag || sflag))
			(void) fprintf(stdout, "%s:\n", hname);
		clnt = openclnt(hname, 1);
		if (clnt == NULL)
			return;
	}
#endif /* REMOTE */

	if (getstatus(prfd, &status, clnt) == 0) {
		if (pflag) {
			if (vflag &&
			    (Fflag || Rflag || uflag || dflag || sflag)) {
				/*
				 * Print the ``before'' status.
				 */
				print_status(&status);
			}
			printstats(&status);
		}
	}

	if (dflag) {
		if (argc == 0) {
			/* down everything */
			(void) setupdown(prfd, PRDOWN, clnt, TRUE);
		} else {
			do {
				enabledisable(prfd, *argv++, PRDISABLE);
			} while (--argc > 0);
		}
	}

	if (Fflag) {
		(void) prflush(prfd);
	}

	if (Rflag) {
		(void) reset(prfd);
	}

	if (sflag) {
		if (memsize > status.pr_maxsize)
			memsize = status.pr_maxsize;
		(void) setsize(prfd, (u_int)memsize, clnt);
	}

	if (uflag) {
		if (argc == 0) {
			/* up everything */
			(void) setupdown(prfd, PRUP, clnt, TRUE);
		} else {
			/* up the basic device and then handle each dev */
			(void) setupdown(prfd, PRUP, clnt, FALSE);
			do {
				enabledisable(prfd, *argv++, PRENABLE);
			} while (--argc > 0);
		}
	}

	/*
	 * Print the new current status except when
	 * -u, -d,  -F, -R, -s, or -l is given w/o a -p.
	 */
	if ((pflag || !(uflag || dflag || Fflag || Rflag || sflag || lflag))
	    && getstatus(prfd, &status, clnt) == 0) {
		print_status(&status);
	}

	if (lflag)
		list(prfd, hname);

#ifdef REMOTE
	if (hname != NULL) {
		clnt_destroy(clnt);
	}
#endif /* REMOTE */
}

/*
 * enable or disable presto write caching on the specified filesystem.
 *	prfd	is the previously opened presto file descripter.
 *	specf	is either a block device name or a path naming a filesystem.
 */
void
enabledisable(prfd, specf, op)
	int prfd;	/* local presto device */
	char *specf;
	int op;
{
	struct stat statbuf;
	int ret;

	/*
	 * Try it first as a path to a block device, and if that fails,
	 * try it as a path's parent filesystem.
	 */
	ret = stat(specf, &statbuf);
	if (ret < 0) {
		(void) fprintf(stderr, "%s: cannot stat %s\n",
			       Myname, specf);
		return;
	}

	if ((statbuf.st_mode & S_IFMT) == S_IFBLK) {
		/* simple block device case */
		ret = ioctl(prfd, op, &statbuf.st_rdev);
	} else if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
		if (vflag) {
			(void) fprintf(stdout, "%s: %s is a directory\n",
			    Myname, specf);
		}
		ret = ioctl(prfd, op, &statbuf.st_dev);
	} else {
		(void) fprintf(stderr,
		    "%s: %s is not a block device or directory.\n",
		    Myname, specf);
		return;
	}

	if (ret < 0) {
		(void) fprintf(stderr, "%s: %s failed ",
			       Myname, prioctl_to_str[IOCTL_NUM(op)]);
			perror(specf);
	} else if (vflag) {
		(void) fprintf(stdout, "%s: %s %s ok\n",
		    Myname, prioctl_to_str[IOCTL_NUM(op)], specf);
	}
}

/*
 * Return the number for the given string.
 * String should be either a decimal or a hex number.
 */
int
getnum(str)
	char *str;
{
	int size;

	if (!isdigit(*str)) {
		usage();
		/* NOTREACHED */
	}

	if (str[0] == '0' && str[1] == 'x') {
		/* hex */
		if (sscanf(&str[2], "%x", &size) == 0) {
			usage();
			/* NOTREACHED */
		}
	} else {
		/* decimal */
		size = atoi(str);
	}
	return (size);
}

/*
 * openpr - Open the given pr device and return an fd.
 * Returns 0 if ok, -1 on error.
 */
int
openpr(prname)
	char *prname;
{
	int ret;

	ret = open(prname, O_RDONLY|O_NDELAY, 0);
	if (ret < 0) {
		(void) fprintf(stderr, "%s: can't open `%s': ",
		    Myname, prname);
		perror("");
	} else if (vflag) {
		(void) fprintf(stdout, "%s: open `%s'\n", Myname, prname);
	}
	return (ret);
}

/*
 * get a valid RPC handle to the remote machine
 * Return 0 if not ok.
 */
CLIENT *
openclnt(name, verbose)
	char *name;
	int verbose;
{
#ifdef REMOTE
	struct sockaddr_in sin;
	struct hostent *hp;
	int sock;
	static CLIENT *clnt = NULL;
	static char oldname[MAXHOSTNAMELEN];

	if (clnt != NULL) {
		/* same host as before ? */
		if (strcmp(name, oldname) == 0)
			return (clnt);	/* already created  */
		else
			clnt_destroy(clnt);
	}
	if (hp = gethostbyname(name))
		(void) bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	else {
		fprintf(stderr,"%s: gethostbyname of %s failed\n",
			Myname, name);
		exit(1);
	}
	sin.sin_family = AF_INET;
	sin.sin_port=0;
	sock = RPC_ANYSOCK;
	clnt = clntudp_create(&sin, PRESTOCTLPROG, PRESTOCTLVERS, TIMEOUT,
			&sock);
	if (clnt != NULL)
		(void) strcpy(oldname, name);
	else if (verbose) {
		(void) fprintf(stdout, "%s: cannot reach machine %s\n",
		    Myname, name);
		(void) clnt_pcreateerror(Myname);
	}
	return (clnt);
#else REMOTE
	(void) fprintf(stderr, "Remote operation not supported\n");
	return ((CLIENT *) NULL);
#endif REMOTE
}

/*
 * getstatus
 * Returns 0 if ok, -1 on error.
 */
int
getstatus(prfd, prstatus, clnt)
	int prfd;
	struct presto_status *prstatus;
	CLIENT *clnt;
{

	if (hflag) {
#ifdef REMOTE
		register struct presto_modstat *ps;

		ps = prestoctl_getstate_3((void *)0, clnt);
		if (ps == 0) {
			(void) fprintf(stderr,
				"%s: PRGETSTATUS failed",
				Myname);
			clnt_perror(clnt, "");
			return (-1);
		}
		if (! ps->ps_status) {
			(void) fprintf(stderr,
				"%s: PRGETSTATUS failed %s\n",
				Myname, ps->presto_modstat_u.ps_errmsg);
			return (-1);
		}
		if (vflag) {
			(void) fprintf(stdout, "%s: PRGETSTATUS ok\n",
			Myname);
		}
		*prstatus = ps->presto_modstat_u.ps_new;
#endif /* REMOTE */
		return (0);
	} else {
		int ret;

		ret = ioctl(prfd, PRGETSTATUS, prstatus);
		if (ret < 0) {
			(void) fprintf(stderr, "%s: PRGETSTATUS failed: ",
				Myname);
			perror("");
		} else if (vflag) {
/*			(void) fprintf(stdout, "%s: PRGETSTATUS ok\n",
			Myname); */
		}
		return (ret);
	}
}

/*
 * Perform op on all the enabled presto devices.
 */
void
op_on_enabled(prfd, op)
	int prfd;
	int op;
{
	register int i;
	register struct prbits *enabled;
	int ret;
	dev_t dev;
	struct uprtab uprtab;

	for (uprtab.upt_bmajordev = NODEV;;) {
		if (ioctl(prfd, PRNEXTUPRTAB, &uprtab) < 0) {	/* get next */
			perror(prioctl_to_str[IOCTL_NUM(PRNEXTUPRTAB)]);
			return;
		}

		if (uprtab.upt_bmajordev == NODEV)	/* got last one ? */
			return;

		/* perform the op */
		enabled = &uprtab.upt_enabled;
		for (i = 0; i < NBBY * sizeof (struct prbits); i++) {
			if (isset(enabled->bits, i)) {
				dev = makedev(uprtab.upt_bmajordev, i);
				ret = ioctl(prfd, op, &dev);
				if (ret < 0) {
					(void) fprintf(stderr,
					    "%s: %s failed on dev (%d, %d): ",
					    Myname,
					    prioctl_to_str[IOCTL_NUM(op)],
					    uprtab.upt_bmajordev, i);
					perror("");
					return;
				} else if (vflag) {
					(void) fprintf(stdout,
					    "%s: %s on dev (%d, %d)\n",
					    Myname,
					    prioctl_to_str[IOCTL_NUM(op)],
					    uprtab.upt_bmajordev, i);
				}
			}
		}
	}
}

/*
 * setupdown
 * Set the presto state to down or up.
 * If we are to end up down (local case),
 * first disable all enabled filesystems.
 * Returns -1 if error, 0 otherwise.
 */
int
setupdown(prfd, prstate, clnt, doall)
	int prfd;
	prstates prstate;		/* PRUP or PRDOWN */
	CLIENT *clnt;
	bool_t doall;
{

	if (hflag) {
#ifdef REMOTE
		bool_t up = (prstate == PRUP);
		presto_modstat *res;

		res = prestoctl_toggle_3(&up, clnt);
		if (res == 0) {
			(void) fprintf(stderr, "%s: PRSETSTATE failed",
			    Myname);
			clnt_perror(clnt, "");
			return (-1);
		}
		if (! res->ps_status) {
			(void) fprintf(stderr,
			    "%s: PRSETSTATE remote failure: %s\n",
			    Myname, res->presto_modstat_u.ps_errmsg);
			return (-1);
		}
		if (vflag) {
			(void) fprintf(stdout, "%s: PRSETSTATE call ok\n",
			Myname);
		}
#endif /* REMOTE */
		return (0);
	} else {
		int ret;

		if (prstate == PRDOWN)
			op_on_enabled(prfd, PRDISABLE);

		ret = ioctl(prfd, PRSETSTATE, &prstate);
		if (ret < 0) {
			(void) fprintf(stderr, "%s: PRSETSTATE %s failed: ",
			    Myname, prstate == PRUP ? "UP" : "DOWN");
			perror("");
			return (ret);
		} else if (vflag) {
			(void) fprintf(stdout, "%s: PRSETSTATE %s\n",
			    Myname, prstate == PRUP ? "UP" : "DOWN");
		}

		if (prstate == PRUP && doall) {
			if (mnt_presto_op(prfd, PRENABLE) < 0)
				ret = -1;
		}
		return (ret);
	}
}

/*
 * Scan the kernel mount table and perform the specified operation on it
 * if it matches the requirements for a "prestoized" entry.
 */
int
mnt_presto_op(prfd, op)
	int prfd;
	int op;		/* PRENABLE or PRDISABLE */
{
	register struct	fs_data *fd;
	int loc = 0, ret;

	if (!mountbuffer) {
		mountbuffer = (struct fs_data *) malloc(MSIZE);
		if (mountbuffer == NULL) {
			perror("malloc");
			exit(1);
		}
	}
	ret = getmnt(&loc, mountbuffer, MSIZE, NOSTAT_MANY, 0);
	if (ret < 0) {
		perror("getmnt");
		(void) fprintf(stdout, "%s: cannot get mount info\n", Myname);
		exit(1);
	}

	/*
	 * enable/disable all local, r/w filesystems
	 */
	for (fd = mountbuffer; fd < &mountbuffer[ret]; fd++) {
		if (vflag) {
			(void) fprintf(stdout, "%s: scanning %s entry\n",
			    Myname, fd->fd_devname);
		}

		if (fd->fd_fstype != GT_ULTRIX) {
			if (vflag) {
				(void) fprintf(stdout, "%s: type is %s\n",
				    Myname, gt_names[fd->fd_fstype]);
			}
			continue;
		}

		if (fd->fd_flags & M_RONLY) {
			if (vflag) {
		           (void) fprintf(stdout, "%s: mounted read-only\n",
				    Myname);
			}
			continue;
		}

		enabledisable(prfd, fd->fd_devname, op);
	}
	return (0);
}

/*
 * Set presto memory size.
 * Returns -1 if error, 0 otherwise.
 */
int
setsize(prfd, size, clnt)
	int prfd;
	u_int size;
	CLIENT *clnt;
{

	if (hflag) {
#ifdef REMOTE
		presto_modstat *res;

		res = prestoctl_setbytes_3(&size, clnt);
		if (res == 0) {
			(void) fprintf(stderr, "%s: PRSETMEMSZ failed",
			    Myname);
			clnt_perror(clnt, "");
			return (-1);
		}
		if (! res->ps_status) {
			(void) fprintf(stderr,
			    "%s: PRSETMEMSZ remote failure: %s\n",
			    Myname, res->presto_modstat_u.ps_errmsg);
			return (-1);
		}
		if (vflag) {
			(void) fprintf(stdout, "%s: PRSETMEMSZ to 0x%x bytes\n",
			    Myname, size);
		}
#endif /* REMOTE */

		return (0);
	} else {
		int ret;

		ret = ioctl(prfd, PRSETMEMSZ, &size);
		if (ret < 0) {
			(void) fprintf(stderr, "%s: PRSETMEMSZ 0x%x failed: ",
			    Myname, size);
			perror("");
		} else if (vflag) {
			(void) fprintf(stdout,
			    "%s: PRSETMEMSIZ to 0x%x bytes\n",
			    Myname, size);
		}
		return (ret);
	}
}

/*
 * Flush the presto device.
 * Returns -1 if error, 0 otherwise.
 */
int
prflush(prfd)
	int prfd;
{
	int ret;

	if (hflag)  {
		(void) fprintf(stderr,
		    "%s: warning: -F ignored; cannot flush remote machines.\n",
		    Myname);
		return (-1);
	}
	ret = ioctl(prfd, PRFLUSH);
	if (ret < 0) {
		(void) fprintf(stderr, "%s: PRFLUSH failed: ", Myname);
		perror("");
	} else if (vflag) {
		(void) fprintf(stdout, "%s: PRFLUSH\n", Myname);
	}
	return (ret);
}

/*
 * Reset the presto status.
 * Returns -1 if error, 0 otherwise.
 */
int
reset(prfd)
	int prfd;
{
	int ret;

	if (hflag)  {
		(void) fprintf(stderr,
		    "%s: warning: -R ignored; cannot reset remote machines.\n",
		    Myname);
		return (-1);
	}
	ret = ioctl(prfd, PRRESET);
	if (ret < 0) {
		(void) fprintf(stderr, "%s: PRRESET failed: ", Myname);
		perror("");
	} else if (vflag) {
		(void) fprintf(stdout, "%s: PRRESET\n", Myname);
	}
	return (ret);
}

/*
 * Print out the status information for the given presto structure.
 */
void
print_status(prstatus)
	struct presto_status *prstatus;
{
	char *pr_state;
	int sdwhp;		/* synchronous dirty write hit percentage */

	switch (prstatus->pr_state) {
	case PRUP:
		pr_state = "UP";
		break;
	case PRDOWN:
		pr_state = "DOWN";
		break;
	case PRERROR:
		pr_state = "ERROR";
		break;
	default:
		pr_state = "???";
		break;
	}

	(void) fprintf(stdout, "state = %s, size = 0x%x ",
	    pr_state, prstatus->pr_cursize);
	if (prstatus->pr_cursize == prstatus->pr_maxsize) {
		(void) fprintf(stdout, "bytes\n");
	} else {
		(void) fprintf(stdout, "/ 0x%x bytes\n", prstatus->pr_maxsize);
	}
	print_interval(prstatus->pr_seconds);
	sdwhp = prstatus->pr_wrstats.total - prstatus->pr_wrstats.pass;
	if (sdwhp > 0) {
		sdwhp = (prstatus->pr_wrstats.hitdirty * 1005) / (sdwhp * 10);
	} else {
		sdwhp = 0;
	}
	(void) printf("write cache efficiency: %u%%\n", sdwhp);

	/* inform the user of the battery states */
	{
		register int low_batt = 0;
		register int i;

		for (i = 0; i < prstatus->pr_battcnt; i++) {
			if (prstatus->pr_batt[i] == BAT_LOW) {
				(void) fprintf(stdout, "battery %d is low!\n",
				    i + 1);
				low_batt = 1;
			} else if (prstatus->pr_batt[i] == BAT_DISABLED) {
				(void) fprintf(stdout, "battery %d is disabled!\n",
				    i + 1);
				low_batt = 1;
			}
		}
		if (!low_batt)
			(void) fprintf(stdout, "All batteries are ok\n");
	}
}

void
print_interval(secs)
	u_int secs;
{
	u_int ndays, nhours, nmin, nsecs;
#define	MINSEC	(60)
#define	HOURSEC	(60*MINSEC)
#define	DAYSEC	(24*HOURSEC)

	(void) printf("statistics interval: ");
	nsecs = secs;

	ndays = nsecs / DAYSEC;
	nsecs = nsecs % DAYSEC;
	if (ndays > 0) {
		(void) printf("%d day", ndays);
		if (ndays != 1)
			(void) printf("s, ");
		else
			(void) printf(", ");
	}

	nhours = nsecs / HOURSEC;
	nsecs = nsecs % HOURSEC;

	nmin = nsecs / MINSEC;
	nsecs = nsecs % MINSEC;

	(void) printf("%02d:%02d:%02d  (%u seconds)\n", nhours, nmin, nsecs,
	    secs);
}

/*
 * Data structure organized to keep track of per device bits.
 * This structure is used as a linked list of local major devices
 * using Prestoserve.  We keep one bit per minor so that we will
 * later be able to tell if there are any unmounted block devices that
 * we didn't report on when trying to list all the Presto-ized filesystems
 * and devices.
 */
struct devdisp {
	struct devdisp *dd_next;
	u_int		dd_bmajordev;
	struct prbits	dd_disp;
} *Devdisp;

/*
 * Mark the given dev as displayed in global state.
 * Use haddev() routine (below) to test.
 */
void
markdev(dev)
	dev_t dev;
{
	register struct devdisp **ddpp, *ddp;
	u_int maj_d, min_d;

	maj_d = major(dev);
	min_d = minor(dev);

	/* look for an already existing devdisp structure for this major */
	for (ddpp = &Devdisp; *ddpp != NULL; ddpp = &(*ddpp)->dd_next) {
		ddp = *ddpp;
		if (ddp->dd_bmajordev == maj_d) {
			break;
		}
	}

	if (*ddpp == NULL) {
		/* allocate new structure for this major dev */
		*ddpp = ddp = (struct devdisp *)calloc(1, sizeof (*ddp));
		if (ddp == NULL) {
			(void) fprintf(stderr, "%s: out of memory!\n", Myname);
			return;
		}
		ddp->dd_bmajordev = maj_d;
	}

	/* mark the given minor bit for this device */
	setbit(ddp->dd_disp.bits, min_d);
}

/*
 * Return a TRUE/FALSE indicator of whether markdev() was called with this dev.
 */
bool_t
haddev(dev)
	dev_t dev;
{
	register struct devdisp **ddpp, *ddp;
	u_int maj_d, min_d;

	maj_d = major(dev);
	min_d = minor(dev);

	/* look for an already existing devdisp structure for this major */
	for (ddpp = &Devdisp; *ddpp != NULL; ddpp = &(*ddpp)->dd_next) {
		ddp = *ddpp;
		if (ddp->dd_bmajordev == maj_d) {
			/* got it - see if the minor bit for dev was set */
			return (isset(ddp->dd_disp.bits, min_d));
		}
	}
	return (FALSE);
}

void
show_local_status(prfd, devname, dirname)
	int prfd;
	char *devname;
	char *dirname;
{
	struct uprtab uprtab;
	struct stat stats;
	presto_status ps;

	/*
	 * map this device name to a block device number
	 */
	if (stat(devname, &stats) < 0) {
		if (vflag)
			perror(devname);
		return;
	}

	if ((stats.st_mode & S_IFMT) != S_IFBLK) {
		if (vflag)
			(void) fprintf(stdout, "%s: %s is not a block device\n",
			    Myname, devname);
		return;
	}

	/*
	 * query enabled status for this device from presto driver
	 */
	uprtab.upt_bmajordev = major(stats.st_rdev);

	if (vflag) {
		(void) fprintf(stdout,
			       "%s: PRGETUPRTAB for block device 0x%x\n",
			       Myname, uprtab.upt_bmajordev);
	}

	if (ioctl(prfd, PRGETSTATUS, &ps) < 0) {
		perror(prioctl_to_str[IOCTL_NUM(PRGETSTATUS)]);
		return;
	}
	if (ioctl(prfd, PRGETUPRTAB, &uprtab) < 0) {
		perror(prioctl_to_str[IOCTL_NUM(PRGETUPRTAB)]);
		return;
	}
	if (uprtab.upt_bmajordev == NODEV)   /* got the requested device? */
		return;

	if (vflag || Lflag) {
		int open = 0;
		(void) fprintf(stdout, "%s on %s", devname, dirname);
		if (isset(uprtab.upt_bounceio.bits, minor(stats.st_rdev))) {
			(void) fprintf(stdout, " (bounceio");
			++open;
		}
		if (isclr(uprtab.upt_enabled.bits, minor(stats.st_rdev))) {
			if (open)
				(void) fprintf(stdout, " disabled");
			else
				(void) fprintf(stdout, " (disabled");
			++open;
		}
		if (open)
			(void) fprintf(stdout, ")\n");
		else
			(void) fprintf(stdout, "\n");
	} else if (ps.pr_state == PRUP &&
	    isset(uprtab.upt_enabled.bits, minor(stats.st_rdev))) {
		(void) fprintf(stdout, "%s on %s\n", devname, dirname);
	}
	markdev(stats.st_rdev);
}

void
show_remote_status(fsname, dirname, hname)
	char *fsname;
	char *dirname;
	char *hname;
{
#ifdef REMOTE

	presto_get_fs_status *status;
	CLIENT *clnt;
	char *fs, *rhost;

	/*
	 * Parse the NFS fsname entry into host/fs name pairs.
	 */
	if ((fs = index(fsname, ':')) == 0) {
		(void) fprintf(stderr,
		       "%s: missing ':' in NFS mount fsname field of %s\n",
		    Myname, fsname);
		return;
	}
	rhost = fsname;
	*fs++ = '\0';

	if (hname != NULL && strcmp(rhost, hname) != 0) {
		if (vflag)
			(void) fprintf(stdout,
			"%s: %s does not match %s\n", Myname, rhost, hname);
		return;
	}

	if (*fs == '(') {
		if (vflag)
			(void) fprintf(stdout,
			"%s: %s:%s looks like an automounter entry, so punt\n",
				       Myname, rhost, fs);
		return;
	}
	if (vflag) {
		(void) fprintf(stdout, "%s: requesting status of %s:%s\n",
		    Myname, rhost, fs);
	}

	/* establish a connection with the remote server */
	if ((clnt = openclnt(rhost, vflag)) == NULL)
		return;

	status = prestoctl_get_fs_status_3(&fs, clnt);

#define	PFS (status->presto_get_fs_status_u.status)
	if (status == NULL) {
		(void) fprintf(stderr, "%s: get_fs_status failed",
		    Myname);
		clnt_perror(clnt, "");
	} else if (!status->succeeded) {
		(void) fprintf(stderr, "%s: get_fs_status failed : %s\n",
		    Myname, status->presto_get_fs_status_u.errmsg);
	} else if (PFS.pfs_prestoized) {
		if (vflag || Lflag) {
			int open = 0;
			(void) fprintf(stdout,
				       "%s:%s on %s", rhost, fs, dirname);
			if (PFS.pfs_bounceio) {
				(void) fprintf(stdout, " (bounceio");
				++open;
			}
			if (PFS.pfs_state != PRUP || !PFS.pfs_enabled) {
				if (open)
					(void) fprintf(stdout, " disabled");
				else
					(void) fprintf(stdout, " (disabled");
				++open;
			}
			if (open)
				(void) fprintf(stdout, ")\n");
			else
				(void) fprintf(stdout, "\n");
		} else if (PFS.pfs_state == PRUP && PFS.pfs_enabled) {
			(void) fprintf(stdout, "%s:%s on %s\n", rhost, fs,
				       dirname);
		}
	}
#undef PFS
#endif /* REMOTE */
}

void
list(prfd, hname)
	int prfd;
	char *hname;	/* host name for remote list */
{
	register struct	fs_data *fd;
	register struct prbits *enabled;
	register int min_d;
	struct uprtab uprtab;
	int loc = 0, nmounted;

	if (!mountbuffer) {
		mountbuffer = (struct fs_data *) malloc(MSIZE);
		if (mountbuffer == NULL) {
			perror("malloc");
			exit(1);
		}
	}
	nmounted = getmnt(&loc, mountbuffer, MSIZE, NOSTAT_MANY, 0);
	if (nmounted < 0) {
		perror("getmnt");
		(void) fprintf(stdout, "%s: cannot get mount info\n", Myname);
		exit(1);
	}

	/*
	 * Show presto status for each mounted filesystem,
	 * whether it is local or remotely mounted via NFS.
	 * For each local dev found, mark a bit.
	 */
	for (fd = mountbuffer; fd < &mountbuffer[nmounted]; fd++) {
		if (vflag) {
			(void) fprintf(stdout, "%s: scanning mount entry %s\n",
				       Myname, fd->fd_devname);
		}
		if (fd->fd_fstype == GT_NFS) {
			show_remote_status(fd->fd_devname, fd->fd_path, hname);
		} else if (prfd != -1 && fd->fd_fstype == GT_ULTRIX) {
			show_local_status(prfd, fd->fd_devname,
					  fd->fd_path);
		}
		/* else it is some unknown file system type, ignore it */
	}

	if (prfd == -1)
		return;			/* no local presto device, all done */

	/*
	 * Now use presto ioctl's to get all the presto-ized devices.
	 * List any currently enabled devices that weren't already printed.
	 */
	for (uprtab.upt_bmajordev = NODEV;;) {
		if (ioctl(prfd, PRNEXTUPRTAB, &uprtab) < 0) {
			perror(prioctl_to_str[IOCTL_NUM(PRNEXTUPRTAB)]);
			return;
		}

		if (uprtab.upt_bmajordev == NODEV)
			return;				/* got the last one */

		enabled = &uprtab.upt_enabled;
		for (min_d = 0; min_d < NBBY * sizeof (*enabled); min_d++) {
			if (isclr(enabled->bits, min_d)) {
				/* not enabled, just skip */
				continue;
			}

			/* we got a presto-ized block device */

			if (haddev(makedev(uprtab.upt_bmajordev, min_d))) {
				/* already printed */
				continue;
			}

			/*
			 * Maybe we should go through the work of
			 * mapping the device to a name in /dev?
			 */
			if (vflag || Lflag) {
				(void) fprintf(stdout,
				       "*unmounted* block device (%d, %d)",
					       uprtab.upt_bmajordev, min_d);
				if (isset(uprtab.upt_bounceio.bits, min_d))
					(void) fprintf(stdout, " (bounceio)");
				(void) fprintf(stdout, "\n");
			} else {
				(void) fprintf(stdout,
				       "*unmounted* block device (%d, %d)\n",
					       uprtab.upt_bmajordev, min_d);
			}
		}
	}
}

/*
 * Print out the given struct io stats with the given header.
 */
void
printio(s, iop)
	char *s;
	struct io *iop;
{
	double hits;
	double hitpct;

	hits = iop->hitclean + iop->hitdirty;
	if (iop->total != (u_int)0) {
		hitpct = (hits * 1005) / (iop->total * 10);	/* round */
	} else {
		hitpct = 100;
	}
	(void) fprintf(stdout,
	    "%6s %12d%11d%%%12d%12d%12d%12d\n",
	    s, iop->total, (u_int)hitpct, iop->hitclean, iop->hitdirty,
	    iop->alloc, iop->pass);
}

/*
 * Print out the statistics from the given presto structure.
 */
void
printstats(prstatus)
	struct presto_status *prstatus;
{
	struct io total;

	total.total = prstatus->pr_wrstats.total + prstatus->pr_rdstats.total;
	total.hitclean =
	    prstatus->pr_wrstats.hitclean + prstatus->pr_rdstats.hitclean;
	total.hitdirty =
	    prstatus->pr_wrstats.hitdirty + prstatus->pr_rdstats.hitdirty;
	total.alloc = prstatus->pr_wrstats.alloc + prstatus->pr_rdstats.alloc;
	total.pass = prstatus->pr_wrstats.pass + prstatus->pr_rdstats.pass;
	(void) fprintf(stdout,
	    "dirty = %d, clean = %d, inval = %d, active = %d\n",
	    prstatus->pr_ndirty, prstatus->pr_nclean,
	    prstatus->pr_ninval, prstatus->pr_nactive);
	(void) fprintf(stdout,
	    "%6s %12s%12s%12s%12s%12s%12s\n",
	    "", "count", "hit rate", "clean hits", "dirty hits",
	    "allocations", "passes");
	printio("write:", &prstatus->pr_wrstats);
	printio("read:", &prstatus->pr_rdstats);
	printio("total:", &total);
}

/*
 * Print usage message and exit with error condition.
 */
void
usage()
{

	(void) fprintf(stderr,
"usage:\n  %s [-plLFRv] [-s size] [-u|d [filesystem ...]]\n",
	    Myname);
	(void) fprintf(stderr,
"  %s -h hostname [-plLv] [-s size] [-u|d]\n",
	    Myname);
	exit(1);
	/* NOTREACHED */
}
