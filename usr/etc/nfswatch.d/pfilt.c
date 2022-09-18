#ifndef lint
static char *sccsid = "@(#)pfilt.c	4.2	(ULTRIX)	1/25/91";
#endif
/* Based on:
 * RCSid = "$Header: /sparky/a/davy/system/nfswatch/RCS/pfilt.c,v 3.0 91/01/23 08:23:15 davy Exp $";
 */

#ifdef ultrix
/*
 * pfilt.c - routines for messing with the packet filter
 *
 * Jeffrey Mogul
 * DECWRL
 *
 * $Log:	pfilt.c,v $
 * Revision 3.0  91/01/23  08:23:15  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.2  90/12/04  08:02:43  davy
 * Changes from Jeff Mogul for Ultrix 4.1 and higher.
 * 
 * Revision 1.1  90/08/17  15:47:34  davy
 * Initial revision
 * 
 * Revision 1.1  90/04/20  13:59:36  mogul
 * Initial revision
 * 
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/file.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>

#include <net/pfilt.h>

#include "nfswatch.h"
#include "externs.h"

static struct ifreq ifr;			/* holds interface name	*/

/*
 * setup_pfilt_dev - set up the packet filter
 */
int
setup_pfilt_dev(device)
char **device;
{
	int fd;
	struct timeval timeout;
	short enmode;
	short backlog = -1;	/* request the most */
	struct enfilter Filter;

	/*
	 * Open the packetfilter.  If it fails, we're out of
	 * devices.
	 */
	if ((fd = pfopen(*device, 0)) < 0) {
		return(-1);
	}

	/*
	 * We want the ethernet in promiscuous mode
	 */
	enmode = ENBATCH|ENTSTAMP|ENNONEXCL|ENPROMISC;
	if (ioctl(fd, EIOCMBIS, &enmode) < 0) {
		error("ioctl: EIOCMBIS");
		finish(-1);
	}

#ifdef ENCOPYALL
	/*
	 * Attempt to set "copyall" mode (see our own packets).
	 * Okay if this fails.
	 */
	enmode = ENCOPYALL;
	(void) ioctl(fd, EIOCMBIS, &enmode);
#endif /* ENCOPYALL */

	/*
	 * Set the read timeout.
	 */
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if (ioctl(fd, EIOCSRTIMEOUT, &timeout) < 0) {
		error("ioctl: EIOCSRTIMEOUT");
		finish(-1);
	}

	/* set the backlog */
	if (ioctl(fd, EIOCSETW, &backlog) < 0) {
		error("ioctl: EIOCSETW");
		finish(-1);
	}

	/* set the truncation */
	if (ioctl(fd, EIOCTRUNCATE, &truncation) < 0) {
		error("ioctl: EIOCTRUNCATE");
		finish(-1);
	}

	/* find out the actual device name */
	if (*device == NULL) {
		if (ioctl(fd, EIOCIFNAME, &ifr) >= 0) {
			*device = ifr.ifr_name;
		}
		else {
			*device = "pf0";
		}
	}

	/* accept all packets */
	Filter.enf_Priority = 37;	/* anything > 2 */
	Filter.enf_FilterLen = 0;	/* means "always true" */
	if (ioctl(fd, EIOCSETF, &Filter) < 0) {
		error("ioctl: EIOCSETF");
		finish(-1);
	}

	return(fd);
}

/*
 * flush_pfilt - flush data from the packet filter
 */
void
flush_pfilt()
{
	if (ioctl(if_fd, EIOCFLUSH) < 0) {
		error("ioctl: EIOCFLUSH");
		finish(-1);
	}
}
#endif /* ultrix */
