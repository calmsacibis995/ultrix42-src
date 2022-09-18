
# ifndef lint
static char *sccsid = "@(#)dumprmt.c	4.1    (ULTRIX)        7/2/90";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

/* ------------------------------------------------------------------------
 * Modification History: /usr/src/etc/dump/dumprmt.c
 *
 *  1 Jun 89 -- Sam
 *	Make rmtgenioctl smarter about dealing with remote rmt.
 *
 * 29 Jun 88 -- Sam
 *	Modified code in rmtgenioctl() to read the number of bytes returned
 *	from the remote node instead of reading the "sizeof" the devget struct.
 *
 *  8 Sep 87 -- fries
 *	Added 2nd message.(see below).
 *	"Lost connection to remote host."
 *	"Try using the -o option."
 *
 * 13 Feb 86 -- fries
 *	Modified code to output disk messages if output device is a
 *	disk device.
 *
 * ------------------------------------------------------------------------
 */

#include "dump.h"

#include <netinet/in.h>

#include <netdb.h>

#define	TS_CLOSED	0
#define	TS_OPEN		1

static	int rmtstate = TS_CLOSED;
extern errno;
extern  int devtyp;	/* device type of output device */
int	rmtape;
int	rmtconnaborted();
char	*rmtpeer;

/* Set host name & establish connection with Remote System */
rmthost(host)
	char *host;
{

	rmtpeer = host;
	signal(SIGPIPE, rmtconnaborted);
	rmtgetconn();
	if (rmtape < 0)
		exit(1);
}

/* Remote Connection aborted handling */
rmtconnaborted()
{

	fprintf(stderr, "Lost connection to remote host.\n");
	fprintf(stderr, "Try using the -o option.\n");
	exit(1);
}

/* Make remote connection */
rmtgetconn()
{
	static struct servent *sp = 0;

	if (sp == 0) {
		sp = getservbyname("shell", "tcp");
		if (sp == 0) {
			fprintf(stderr, "rdump: shell/tcp: unknown service\n");
			exit(1);
		}
	}
	rmtape = rcmd(&rmtpeer, sp->s_port, "root", "root", "/etc/rmt", 0);
}

/* Open Remote device */
rmtopen(tape, mode)
	char *tape;
	int mode;
{
	int n;
	char buf[256];

	sprintf(buf, "O%s\n%d\n", tape, mode);
	n = rmtcall(tape, buf);
	if (n < 0) {
		return (-1);
	}
	rmtstate = TS_OPEN;
}

/* Close Remote Device */
rmtclose()
{
	register int n;

	if (rmtstate != TS_OPEN)
		return;
	n = rmtcall("close", "C\n");
	if (n < 0) {
		return (-1);
	}
	rmtstate = TS_CLOSED;
}

/* Read Remote Device */
rmtread(buf, count)
	char *buf;
	int count;
{
	char line[30];
	int n, i, cc;

	sprintf(line, "R%d\n", count);
	n = rmtcall("read", line);
	if (n < 0) {
		return (-1);
	}
	for (i = 0; i < n; i += cc) {
		cc = read(rmtape, buf+i, n - i);
		if (cc <= 0) {
			rmtconnaborted();
		}
	}
	return (n);
}

/* Write Remote Device */
rmtwrite(buf, count)
	char *buf;
	int count;
{
	char line[30];

	sprintf(line, "W%d\n", count);
	write(rmtape, line, strlen(line));
	write(rmtape, buf, count);
	return (rmtreply("write"));
}

/* Sets up # of bytes to Write to Remote Device */
/* Informs remote device that next operation is */
/* a write of "count" many bytes		*/
/* The next routine `rmtwrite1'(below)		*/
/* actually sends the "count" number of bytes   */
/* to be written to the device over the network */
rmtwrite0(count)
	int count;
{
	char line[30];

	sprintf(line, "W%d\n", count);
	write(rmtape, line, strlen(line));
}

/* Send characters across the Network */
/* to be written on the Remote Device */
rmtwrite1(buf, count)
	char *buf;
	int count;
{

	write(rmtape, buf, count);
}

/* This routine finds out the actual number */
/* of bytes that were written to the Remote */
/* Device.				    */
rmtwrite2()
{
	int i;

	return (rmtreply("write"));
}

/* Seek Remote Device */
rmtseek(offset, pos)
	int offset, pos;
{
	char line[80];

	sprintf(line, "L%d\n%d\n", offset, pos);
	return (rmtcall("seek", line));
}

/* Perform DEVIOCGET ioctl to get Remote Device Status */
struct	devget gen_info;

struct devget *
rmtgenioctl()
{
	register int i,n;
	register char *cp;
	struct  v22_devget old_gen_info;

	if (rmtstate != TS_OPEN)
		return (0);
	n = rmtcall("general status", "D\n");

	switch (n) {
	   case sizeof(gen_info):
	   	/* Easy case; just read normally */
		for (i = 0, cp = (char *)&gen_info; i < n; i++)
			*cp++ = rmtgetb();
	   	break;

	   case sizeof(old_gen_info):
	   	/* The size of devget struct from pre-V3.0 days */
		for (i = 0, cp = (char *)&old_gen_info; i < n; i++)
			*cp++ = rmtgetb();

		/* Map the V2.2 devget struct components into geninfo struct */
		gen_info.category 	= old_gen_info.category;
		gen_info.bus		= old_gen_info.bus;
		gen_info.interface[DEV_SIZE] = old_gen_info.interface[DEV_SIZE];
		gen_info.device[DEV_SIZE] = old_gen_info.device[DEV_SIZE];
		gen_info.adpt_num	= old_gen_info.adpt_num;
		gen_info.nexus_num	= old_gen_info.nexus_num;
		gen_info.bus_num	= old_gen_info.bus_num;
		gen_info.ctlr_num	= old_gen_info.ctlr_num;
		gen_info.rctlr_num	= 0;
		gen_info.slave_num	= old_gen_info.slave_num;
		gen_info.dev_name[DEV_SIZE] = old_gen_info.dev_name[DEV_SIZE];
		gen_info.unit_num	= old_gen_info.unit_num;
		gen_info.soft_count	= old_gen_info.soft_count;
		gen_info.hard_count	= old_gen_info.hard_count;
		gen_info.stat		= old_gen_info.stat;
		gen_info.category_stat	= old_gen_info.category_stat;
		break;
		
	   default:
	   	/* If the size returned is neither the "new" size nor 
		   the "old" size then return as if an error occured */
		return (NULL);
		break;
	}

	/* If we didn't return the null we need to return gen_info */
	return (&gen_info);
}

/* Perform DIOCDGTPT ioctl to get Remote Disk Partition Inormation */
struct	pt pt_info;

struct pt *
rmtgetpart()
{
	register int i,n;
	register char *cp;

	if (rmtstate != TS_OPEN)
		return (0);
	n = rmtcall("get partitions", "P\n");
	if (n < 0) {
		return (NULL);
	}
	for (i = 0, cp = (char *)&pt_info; i < sizeof(pt_info); i++)
		*cp++ = rmtgetb();
	return (&pt_info);
}

/* Perform `stat' on a Remote File System */
struct	stat stat_info;

struct stat *
rmtstat(tape)
	char *tape;
{
	register int i,n;
	register char *cp;
	char buf[256];

	sprintf(buf, "T%s\n", tape);
	n = rmtcall("stat a file",buf);
	if (n < 0) {
		return (NULL);
	}
	for (i = 0, cp = (char *)&stat_info; i < sizeof(stat_info); i++)
		*cp++ = rmtgetb();
	return (&stat_info);
}

/* Perform MTIOCGET ioctl to get Remote Device Status */
struct	mtget mts;

struct mtget *
rmtstatus()
{
	register int i,n;
	register char *cp;

	if (rmtstate != TS_OPEN)
		return (0);
	n = rmtcall("status", "S\n");
	if (n < 0) {
		return (NULL);
	}
	for (i = 0, cp = (char *)&mts; i < sizeof(mts); i++)
		*cp++ = rmtgetb();
	return (&mts);
}

/* Remote Device ioctl */
rmtioctl(cmd, count)
	int cmd, count;
{
	char buf[256];

	if (count < 0)
		return (-1);
	sprintf(buf, "I%d\n%d\n", cmd, count);
	return (rmtcall("ioctl", buf));
}

/* Send a message to Remote System & await response */
rmtcall(cmd, buf)
	char *cmd, *buf;
{

	if (write(rmtape, buf, strlen(buf)) != strlen(buf))
		rmtconnaborted();
	return (rmtreply(cmd));
}

/* Get protocol reply from Remote System */
rmtreply(cmd)
	char *cmd;
{
	register int c;
	char code[30], emsg[BUFSIZ];

	rmtgets(code, sizeof (code));
	if (*code == 'E' || *code == 'F') {
		rmtgets(emsg, sizeof (emsg));
		if (*code == 'F') {
		        msg("%s: %s\n", cmd, emsg, code + 1);
			rmtstate = TS_CLOSED;
			return (-1);
		}
		errno = atoi(code+1);
		return (-1);
	}
	if (*code != 'A') {
		if(devtyp == TAP)
		  msg("Protocol to remote tape server botched (code %s?).\n",
		    code);
		else
		  msg("Protocol to remote disk server botched (code %s?).\n",
		    code);
		rmtconnaborted();
	}
	return (atoi(code + 1));
}

/* Remote get byte */
rmtgetb()
{
	char c;

	if (read(rmtape, &c, 1) != 1)
		rmtconnaborted();
	return (c);
}

/* Remote get string */
rmtgets(cp, len)
	char *cp;
	int len;
{
	/* Get response from Remote System */
	while (len > 1) {
		*cp = rmtgetb();
		if (*cp == '\n') {
			cp[1] = 0;
			return;
		}
		cp++;
		len--;
	}
	if(devtyp == TAP)
	  msg("Protocol to remote tape server botched (in rmtgets).\n");
	else
	  msg("Protocol to remote disk server botched (in rmtgets).\n");

	rmtconnaborted();
}
