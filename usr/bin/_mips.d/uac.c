#ifdef mips

# ifndef lint
static char *sccsid = "@(#)uac.c	4.1	(ULTRIX)	7/17/90";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1987, 1988 by		*
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
 * Modification History: /usr/src/bin/uac.c
 *
 * 14-Nov-1988	jaa	creation date
 *			prints current state or change state of
 *                      flag that controls printing of unaligned 
 *                      access messages on a system wide basis,
 *			or for the parent process
 *
 *			usage: uac: [sp] [01]
 *
 */

#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/exec.h>

main(argc, argv)
int argc;
char *argv[];
{
	int err = 0, sflg = 0, val;
	unsigned arg;
	char *ap = (char *)0;

	if(--argc < 1 || argc > 2)
		usage();

	ap = *++argv;
	switch(*ap) {
	      case 's':
		arg = GSI_UACSYS;
		if(--argc) {
			arg = SSIN_UACSYS;
			sscanf( *++argv, "%x", &val);
			++sflg;
		}
		break;
	      case 'p':
		arg = GSI_UACPARNT;
		if(--argc) {
			arg = SSIN_UACPARNT;
			sscanf( *++argv, "%x", &val);
			++sflg;
		}
		break;
	      default:
		usage();
	}
	
	if(sflg) {
		if(val == 0)
			val = UAC_MSGOFF;
		else if(val == 1)
			val = UAC_MSGON;
		else
			usage();
		err = setinfo(arg, val);
	} else
		err = getinfo(arg);

	if(err < 0)
		perror("uac");
	exit(err);
}

getinfo(arg)
unsigned arg;
{
	int start = 0, buf = 0, err = 0;
	char *str = (arg == GSI_UACSYS) ? "sys" : "parent";

	err = getsysinfo(arg, (char *)&buf, (unsigned)sizeof(buf), (int *)&start, (char *)0);
	if(err == 0)
		printf(" %s unavailable\n", str);
	else if(err == 1)
		printf("%s is %s\n", str, buf ? "on" : "off");
	return(err);
}


setinfo(arg, val)
unsigned arg;
int val;
{
	int buf[2];

	buf[0] = arg;
	buf[1] = val;
	return(setsysinfo((unsigned)SSI_NVPAIRS, (char *)buf, (unsigned)1, (unsigned)0, (unsigned)0));
}

usage()
{
	printf("usage: uac [sp] [01]\n");
	exit(1);
}

#endif mips
