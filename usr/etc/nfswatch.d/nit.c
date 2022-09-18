#ifndef lint
static char *sccsid = "@(#)nit.c	4.2	(ULTRIX)	1/25/91";
#endif
/* Based on:
 * RCSid = "$Header: /sparky/a/davy/system/nfswatch/RCS/nit.c,v 3.0 91/01/23 08:23:14 davy Exp $";
 */

#ifdef sun
/*
 * nit.c - routines for messing with the network interface tap.
 *
 * David A. Curry
 * SRI International
 * 333 Ravenswood Avenue
 * Menlo Park, CA 94025
 * davy@erg.sri.com
 *
 * $Log:	nit.c,v $
 * Revision 3.0  91/01/23  08:23:14  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.4  90/12/04  08:25:22  davy
 * Fixed to automatically define SUNOS40.
 * 
 * Revision 1.3  90/12/04  08:11:40  davy
 * Changed ifdef for SunOS 4.0.x.
 * 
 * Revision 1.2  90/08/17  15:47:32  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:20:47  davy
 * NFSWATCH Release 1.0
 * 
 */
#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/file.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>

#include <net/nit_if.h>
#include <net/nit_buf.h>

#include "nfswatch.h"
#include "externs.h"

#if NOFILE > 64
#define SUNOS40 1
#endif /* NOFILE */

/*
 * setup_nit_dev - set up the network interface tap.
 */
int
setup_nit_dev(device)
char **device;
{
	int s, fd;
	u_int chunksz;
	char *strdup();
	u_long if_flags;
	char buf[BUFSIZ];
	struct ifreq ifr;
	struct ifconf ifc;
	struct strioctl si;
	struct timeval timeout;

	/*
	 * If the interface device was not specified,
	 * get the default one.
	 */
	if (*device == NULL) {
		/*
		 * Grab a socket.
		 */
		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			error("socket");
			finish(-1);
		}

		ifc.ifc_buf = buf;
		ifc.ifc_len = sizeof(buf);

		/*
		 * See what device it's attached to.
		 */
		if (ioctl(s, SIOCGIFCONF, (char *) &ifc) < 0) {
			error("ioctl: SIOCGIFCONF");
			finish(-1);
		}

		*device = strdup(ifc.ifc_req->ifr_name);
		(void) close(s);
	}

	/*
	 * We want the ethernet in promiscuous mode, and
	 * we want to know about dropped packets.
	 */
	if_flags = NI_DROPS | NI_PROMISC;

	/*
	 * Open the network interface tap.
	 */
	if ((fd = open(NIT_DEV, O_RDONLY)) < 0) {
		error("nit: open");
		finish(-1);
	}

	/*
	 * Arrange to get discrete messages.
	 */
	if (ioctl(fd, I_SRDOPT, (char *) RMSGD) < 0) {
		error("ioctl: I_SRDOPT");
		finish(-1);
	}

	/*
	 * Push and configure the nit buffering module.
	 */
	if (ioctl(fd, I_PUSH, NIT_BUF) < 0) {
		error("ioctl: I_PUSH NIT_BUF");
		finish(-1);
	}

	/*
	 * Set the read timeout.
	 */
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	si.ic_cmd = NIOCSTIME;
	si.ic_timout = INFTIM;
	si.ic_len = sizeof(timeout);
	si.ic_dp = (char *) &timeout;

	if (ioctl(fd, I_STR, (char *) &si) < 0) {
		error("ioctl: I_STR NIOCSTIME");
		finish(-1);
	}

	/*
	 * Set the chunk size.
	 */
	chunksz = NIT_CHUNKSIZE;

	si.ic_cmd = NIOCSCHUNK;
	si.ic_len = sizeof(chunksz);
	si.ic_dp = (char *) &chunksz;

	if (ioctl(fd, I_STR, (char *) &si) < 0) {
		error("ioctl: I_STR NIOCSCHUNK");
		finish(-1);
	}

	/*
	 * Configure the network interface tap by binding it
	 * to the underlying interface, setting the snapshot
	 * length, and setting the flags.
	 */
	(void) strncpy(ifr.ifr_name, *device, sizeof(ifr.ifr_name));
	ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';

	si.ic_cmd = NIOCBIND;
	si.ic_len = sizeof(ifr);
	si.ic_dp = (char *) &ifr;

	/*
	 * If the bind fails, there's no such device.
	 */
	if (ioctl(fd, I_STR, (char *) &si) < 0) {
		close(fd);
		return(-1);
	}

	/*
	 * SNAP is buggy on SunOS 4.0.x
	 */
#ifndef SUNOS40
	si.ic_cmd = NIOCSSNAP;
	si.ic_len = sizeof(truncation);
	si.ic_dp = (char *) &truncation;

	if (ioctl(fd, I_STR, (char *) &si) < 0) {
		error("ioctl: I_STR NIOCSSNAP");
		finish(-1);
	}
#endif /* SUNOS40 */

	si.ic_cmd = NIOCSFLAGS;
	si.ic_len = sizeof(if_flags);
	si.ic_dp = (char *) &if_flags;

	if (ioctl(fd, I_STR, (char *) &si) < 0) {
		error("ioctl: I_STR NIOCSFLAGS");
		finish(-1);
	}

	return(fd);
}

/*
 * flush_nit - flush data from the nit.
 */
void
flush_nit(fd)
int fd;
{
	if (ioctl(fd, I_FLUSH, (char *) FLUSHR) < 0) {
		error("ioctl: I_FLUSH");
		finish(-1);
	}
}
#endif /* sun */
