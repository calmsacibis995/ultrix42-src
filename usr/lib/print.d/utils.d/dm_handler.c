#ifndef lint
static char *sccsid = "@(#)dm_handler.c	4.3      ULTRIX 	10/17/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * dm_handler.c -- handle job elements
 *
 * Description:
 *	The job description code in dcl.c initialises two
 *	tables of job module execution function which
 *	are declared in dcl.h and implemented here
 */

/*
 * Modification History:
 *
 * 01-Oct-90 - Adrian Thoms (thoms@wessex)
 *	Make missing archive module a soft error
 */
/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  30/03/88 -- thoms
 * date and time created 88/03/30 18:00:04 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  25/04/88 -- thoms
 * Fixed to know about connections and rationalised job modules.
 * 
 * 
 * ***************************************************************
 *
 * 1.3  15/07/88 -- thoms
 * Added copyright notice, modification history, improved comments
 *
 * ***************************************************************
 *
 * 1.4  28/07/88 -- thoms
 * Filters now executed with fc_plumb_and_run instead of fc_run.
 *
 * ***************************************************************
 *
 * 1.5  01/09/88 -- thoms
 * Check return codes of cx_start and cx_stop
 *
 * ***************************************************************
 *
 * 1.6  20/08/90 -- atkinson
 * Check data files which are symbolic links.
 *
 * SCCS history end
 */


#include "lp.h"

/*
 * dm_default -- default job element handler -- does nothing but successfully
 */
int
dm_default(djp, dmp)
register DJP djp;
register DMP dmp;
{
	return 0;
}

/*
 * dm_debug -- debug job element handler, prints them to stderr
 */
int
dm_debug(djp, dmp)
register DJP djp;
register DMP dmp;
{
	switch (dmp->dm_el) {
	    case je_default:
		fprintf(stderr, "default:  nop value\n");
		break;
	    case je_archive:
		fprintf(stderr, "archive:  used by PostScript jobs\n");
		break;
	    case je_print:
		fprintf(stderr, "print:  xlate and print file\n");
		break;
	    case je_bypass:
		fprintf(stderr, "bypass:  xlate and print, bypass output filter\n");
		break;
	    case je_outstr:
		fprintf(stderr, "outstr:   output strings\n");
		break;
	    case je_banner:
		fprintf(stderr, "banner:  used by lp jobs only\n");
		break;
	}
	printv(dmp->dm_argv);	/* print out the arguments */
	fprintf(stderr, "\n");

	/* Note that we have no way to print the method out yet */

	return 0;
}

static int ar_fd = -1;		/* archive file descriptor */

/*
 * ar_init -- open the library, called from init()
 */
int
ar_init(arname)
char *arname;
{
	if ((ar_fd = ar_get(arname)) >= 0) {
		return 0;
	} else {
		return 1;
	}
}


/*
 * dm_archive -- extract an archive module and download the arguments
 *
 * Since almost all arguments are strings, we put the ()'s on here
 * We borrow the buffer in dcl_xar.c to reduce system call traffic.
 * Hence the limit on total argument size is MAXBSIZE.
 *
 * Numerical strings which don't need ()'s have a leading # which
 * we strip off before sending
 */
int
dm_archive(djp, dmp)
register DJP djp;
register DMP dmp;
{
	int ofd = djp->dj_cxp->cx_out_fd;
	extern char buf[/* MAXBSIZE */];
	int narg;
	register char **argv = dmp->dm_argv;
	char *module;
	register char *bufp, *arg;
	register int extra;

	if (!(module = *argv++)) {
		fprintf(stderr, "ar_handler: no module name\n");
		return 1;
	}
	bufp = buf;		/* initialise buffer pointer */
	for (narg = 0; arg = *argv; narg++, argv++) {
		if (arg[0] == '#') { /* numerical string, no ()'s */
			arg++;
			if (arg[0] == '\0') arg = "0"; /* check for null str */

			sprintf(bufp, "%s\n", arg);
			extra = 1; /* room for newline */
		} else {
			sprintf(bufp, "(%s)\n", arg);
			extra = 3; /* room for newline and ()'s */
		}
		bufp += strlen(arg) + extra;
	}
	if (bufp > &buf[MAXBSIZE]) {
		log("dm_archive, huge args caused overflow");
		exit(1);
	}
	write(ofd, buf, bufp - buf);

	if (ar_x(ar_fd, ofd, module)) {
		return 0;	/* succeeded */
	} else {
		/* don't use buf here, since it shouldn't often happen  */
		fprintf(stderr, "ar_handler: module %s not found\n", module);
		/* remove arguments from stack */
		while ( narg--) {
			write(ofd, "pop ", 4);
		}
		write(ofd, "\n", 1);
		return 0;	/* Hope we didn't need that one! */
	}
}

/*
 * dm_xlate_file -- translate the file and send to file descriptor
 */
static int
dm_xlate_file(ofd, dmp)
int ofd;
register DMP dmp;
{
	struct stat stb;
	union wait filter_status;
	int fi;

	register FCP fcp = (FCP)dmp->dm_method;

	if (lstat(dmp->dm_argv[0], &stb) < 0) {
		logerr("failed to lstat input file %s", dmp->dm_argv[0]);
		return -1;
	}

	if ((fi = open(dmp->dm_argv[0], O_RDONLY)) < 0) {
		logerr("open failure of input file %s", dmp->dm_argv[0]);
		return(-1);
	}

	if ((stb.st_mode & S_IFMT) == S_IFLNK && bad_link(dmp->dm_argv[0], fi))
		return -1;

	fc_plumb_and_run(fcp, DORETURN, fi, ofd, 2);
	
	filter_status = fc_wait(fcp);
	fc_delete(fcp, 1);
	
	(void) close(fi);

	if (!WIFEXITED(filter_status) || filter_status.w_retcode > 1) {
		log("Daemon Filter Malfunction (%d)",
		    filter_status.w_retcode);
		return(-1);
	} else if (filter_status.w_retcode == 1) {
		return(1);
	}
	return 0;
}

/*
 * dm_print -- print the file via the output filter
 */
int
dm_print(djp, dmp)
register DJP djp;
register DMP dmp;
{
	if (cx_start(djp->dj_cxp)) {
		return -1;
	}
	return dm_xlate_file(djp->dj_cxp->cx_out_fd, dmp);
}

/*
 * dm_bypass -- stop the output filter and print the file
 */
int
dm_bypass(djp, dmp)
register DJP djp;
register DMP dmp;
{
	if (cx_stop(djp->dj_cxp)) {
		return -1;
	}
	return dm_xlate_file(djp->dj_cxp->cx_pr_fd, dmp);
}

/*
 * dm_outstr -- concatenate all the args a send to file
 */
int
dm_outstr(djp, dmp)
register DJP djp;
register DMP dmp;
{
	register char **argv;
	register char *bufp = buf;

	for (argv = dmp->dm_argv; *argv; argv++) {
		strcpy(bufp, *argv);
		bufp += strlen(*argv);
	}
	if (bufp > &buf[MAXBSIZE]) {
		log("dm_outstr, huge args caused overflow");
		exit(1);
	}
	write(djp->dj_cxp->cx_out_fd, buf, bufp - buf);
	return 0;
}

/*
 * dm_banner -- print old style ascii banner page
 */
int
dm_banner(djp, dmp)
register DJP djp;
register DMP dmp;
{
	banner(djp->dj_cxp->cx_out_fd,
	       dmp->dm_argv[0],
	       dmp->dm_argv[1],
	       dmp->dm_argv[2]);
	return 0;
}
