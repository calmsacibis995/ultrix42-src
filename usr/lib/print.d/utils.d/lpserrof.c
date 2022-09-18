#ifndef lint
static char *sccsid = "@(#)lpserrof.c	4.1      ULTRIX 7/2/90";
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

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  29/07/88 -- thoms
 * date and time created 88/07/29 11:05:31 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  29/07/88 -- thoms
 * Added missing break in getopt switch loop.
 * 
 * ***************************************************************
 *
 * 1.3  01/09/88 -- thoms
 * Minor fixes and cleaning up
 * Tell user how many pages were printed
 * Pick up debug level from lpd
 *
 * ***************************************************************
 *
 * 1.4  09/09/88 -- thoms
 * Open account file for append with no overwrite, log if open fails
 *
 *
 * ****************************************************************
 *
 * 1.5  21/10/88 -- thoms
 * Removed unnecessary debug message
 *
 * ****************************************************************
 *
 * 1.6  01/03/89 -- thoms
 * Ignore interrupt and quits so as not to cause SIGPIPE to lpscomm
 * 
 * SCCS history end
 */

#include "lp.h"
#include <signal.h>

#define LINE_BUFSIZ 1024

static void lpserr_process_loop();
static void lpserr_handler();
static FCP open_mail_prog();
static void close_mail_prog();
static void do_accounting();

int DU;

static char *account_file = NULL;
static char *logname = NULL;
static char *fromhost = NULL;
static char *job_id = NULL;

int npages= -1;

char *printer = "<unknown printer>";
char *name;

main(argc, argv)
int argc;
char **argv;
{
	char *options = "U:H:J:P:A:d:";
	int opt;
	extern int optind;
	extern char *optarg;

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	DU = getuid();		/* needed by dofork() */

	name = argv[0];		/* used by log() and dlog() */

	while ((opt = getopt(argc, argv, options)) != EOF) {
		switch (opt) {
		    case 'd':
			DB = atoi(optarg);
			break;
		    case 'U':
			logname = optarg;
			break;
		    case 'H':
			fromhost = optarg;
			break;
		    case 'J':
			job_id = optarg;
			break;
		    case 'P':
			printer = optarg;
			break;
		    case 'A':
			account_file = optarg;
			break;
		    default:
		    case '?':
			exit(2);
		}
	}
	lpserr_handler();
}

static FCP
open_mail_prog()
{
	register FCP fcp = new_fc();
	char name_tmp[32];

	sprintf(name_tmp, "%s@%s", logname, fromhost);

	fc_add_args_l(fcp, MAIL, name_tmp, 0);
	fc_end_filter(fcp);

	fc_plumb_and_run(fcp, DORETURN, FC_MAKEPIPE, 2, 2);
	return fcp;
}

static void
close_mail_prog(fcp)
register FCP fcp;
{

	fc_pout_kill(fcp);
	(void)fc_wait(fcp);
	fc_delete(fcp, 1);
}

static void
do_accounting(buf)
char buf[];
{
	register FILE *afp = NULL;
	double f_npages;

	sscanf(buf, "Asheets printed %d", &npages);

	if (account_file && fromhost && logname) {
		if (!(afp = fopen(account_file, "A")))
		    log("Couldn't open accounting file %s", account_file);
	}
	if (afp) {
		f_npages = npages;
		fprintf(afp, "%7.2f\t%s:%s\n", f_npages, fromhost, logname);
		fclose(afp);
	}
}

static void
lpserr_process_loop(mail_fp)
register FILE *mail_fp;
{
	int done_resources = 0;
	int found_U = 0;
	char buf[LINE_BUFSIZ];

	while (fgets(buf, LINE_BUFSIZ, stdin) != 0) {
		switch (buf[0]) {
		    case 'R':
			if (done_resources) break;
			if (strlen(buf) <= 2) {
				done_resources = 1;
				fclose(stdout);
				break;
			}
			fprintf(stdout, "%s", buf+1);
			break;
		    case 'O':
			if (strlen(buf) > 2)
			    fprintf(stderr, "%s", buf+1);
			break;
		    case 'U':
			if (mail_fp) {
				fprintf(mail_fp, "%s", buf+1);
				found_U = 1;
			}
			break;
		    case 'A':
			do_accounting(buf);
			break;
		    default:
			if (strlen(buf) > 1)
			    fprintf(stderr, "%s", buf);
			break;
		}
	}
	if (mail_fp) {
		fprintf(mail_fp, ((npages >= 0)
				  ? "(%d pages printed)\n"
				  : "(Job submission failed)"), npages);
	}
	return;
}

static void
lpserr_handler()
{
	register FCP mail_progp;
	register FILE *mfp = NULL;
	register int mfd;

	if (logname && fromhost && printer && job_id) {
		mail_progp = open_mail_prog();
		mfd = mail_progp->fc_fds[FC_STDIN];

		if ((mfp = fdopen(mfd, "w")) ==NULL) return;
		fprintf(mfp, "To: %s@%s\n", logname, fromhost);
		fprintf(mfp, "Subject: Printer messages for job %s on %s\n\n",
			job_id, printer);

	}
	lpserr_process_loop(mfp);

	if (mfp) {
		fclose(mfp);
		close_mail_prog(mail_progp);
	}
}
