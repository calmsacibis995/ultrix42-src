#ifndef lint
static char *sccsid = "@(#)printjob.c	4.3      ULTRIX 	11/15/90";
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
 * printjob.c -- print jobs in the queue.
 *
 * Description:
 *	This file contains the main loop of the job print action
 *	and is called from lpd.c
 *
 *	The code is a complete rewrite of the code in the
 *	original Berkeley printjob.c
 *
 *	As much code as possible has been modularised and
 *	moved elsewhere, in particular to filter.* connection.*
 *	dcl.* and to print_utils.c
 *
 *	NOTE: the lock file is used to pass information to lpq and lprm.
 *	it does not need to be removed because file locks are dynamic.
 */

/*
 * Modification History:
 *
 * 13-Nov-90 - Adrian Thoms (thoms@wessex)
 *	Corrected use of LPS_SEPARATE module to separate files of job
 *
 * 04-Oct-90 - Adrian Thoms (thoms@wessex)
 *	Added new defaulting scheme for orientation
 *	Added new %x escape for passing a string of flags to filters
 *	Straightened out recent modification history
 *
 * 11-jan-90 -- thoms
 *	Fixed hostname buffer lengths
 *
 * 16/08/89 -- Giles Atkinson
 * Monitor flag page and Postscript prologue as well as file printing.
 *
 * 2/08/89 -- Giles Atkinson
 * Add "4.0" as option for the `uv' printcap capability (same as `psv1.0')
 * and support for the progress monitor.
 */


/* SCCS history beginning
 * THIS SECTION IS NOW OBSOLETE, add history above
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 *
 * 1.1  01/11/83 -- sccs
 * date and time created 83/11/01 20:58:56 by sccs
 *
 * ***************************************************************
 *
 * 1.2  11/12/84 -- root
 * added Ultrix id keywords - lp
 *
 *
 * ***************************************************************
 *
 * 1.3  11/12/84 -- root
 * merged into library - lp
 * several additions by williams of:
 * add of ditroff filter call
 * fix sticky spooler control file param
 * add PP print replacement filter
 * add -z option for regulating page length
 *
 *
 * ***************************************************************
 *
 * 1.4  28/06/85 -- root
 * Comments taken from: /usr/src/usr.lib/lpr/SCCS/s.printjob.c:
 *                      1.4 85/06/24 08:47:56 williams 4 3	00011/00001/01154
 * added initialization file creation code and option on printcap to invoke
 * the init code.
 *
 *
 *
 * ***************************************************************
 *
 * 1.5  25/11/85 -- root
 * Comments taken from: /usr/src/usr.lib/lpr/SCCS/s.printjob.c:
 *                      1.5 85/11/13 16:20:23 williams 5 4	00001/00001/01164
 * added modes to init file open call
 *
 *
 *
 * ***************************************************************
 *
 * 1.6  17/07/86 -- root
 * Comments taken from: 1.7:
 * *** Working Pool Statistics ***: 1.7 86/06/24 15:05:14 logcher 00000/00006/01357
 *
 * Removed three #ifdefs that were used in the testing process
 *
 *
 *
 *
 * ***************************************************************
 *
 * 1.7  17/07/86 -- root
 * Comments taken from: 1.8:
 * *** Working Pool Statistics ***: 1.8 86/07/01 13:48:39 logcher 00021/00018/01339
 *
 * Added logic to make system calls, safe systems calls by setting IFS and PATH.
 *
 *
 *
 *
 * ***************************************************************
 *
 * 1.8  17/07/86 -- root
 * Comments taken from: 1.9:
 * *** Working Pool Statistics ***: 1.9 86/07/02 17:41:26 hoffman 00013/00000/01360
 *
 * added LAT 5.1 host-initiated connect
 *
 *
 *
 *
 * ***************************************************************
 *
 * 1.9  02/10/86 -- root
 * Comments taken from: 1.10:
 * *** Working Pool Statistics ***: 1.10 86/07/08 17:20:06 hoffman 00006/00000/01373
 *
 * Added pass-thru parameter xf
 *
 *
 *
 *
 *
 *
 * ***************************************************************
 *
 * 1.10  30/10/86 -- pnh
 * ADded pgetstr() initialization for TS, OP, OS attributes for Lat printer supp.
 *
 *
 * ***************************************************************
 *
 * 1.11  10/03/88 -- root
 * Brand new filter argument collection and despatch code.
 *
 *
 * ***************************************************************
 *
 * 1.12  22/03/88 -- thoms
 * DCL support added (using dcl output filter)
 *
 *
 * ***************************************************************
 *
 * 1.13  30/03/88 -- thoms
 * Basic PostScript job building and execution working
 *
 *
 * ***************************************************************
 *
 * 1.14  25/04/88 -- thoms
 * Lpd restructured, based on printer_type and connection_type switches.
 *
 *
 * ***************************************************************
 *
 * 1.15  29/04/88 -- thoms
 * Minor fixes, start and exit debug messages, onquit fix
 *
 *
 * ***************************************************************
 *
 * 1.16  03/05/88 -- thoms
 * Fixed and stable minimal PS functionality.
 *
 *
 * ***************************************************************
 *
 * 1.17  06/05/88 -- thoms
 * Added the fancy PostScript support features we had all been waiting for:
 * 	Q defaults in printcap
 * 	Inserting the appropriate PostScript modules as per command params.
 *
 *
 * ***************************************************************
 *
 * 1.18  12/05/88 -- thoms
 * Added layup file, and datatype code, hooked in ansi xlator
 *
 *
 * ***************************************************************
 *
 * 1.19  16/05/88 -- thoms
 * Fixed set output tray to occur before banner page.
 *
 *
 * ***************************************************************
 *
 * 1.20  17/05/88 -- thoms
 * Passes job id to banner page and makes available to filters.
 *
 *
 * ***************************************************************
 *
 * 1.21  20/05/88 -- thoms
 * Moved some utils to print_utils.c
 * Complete rewrite of printjob() to improved structure and
 * comprehensibility!
 *
 *
 * ***************************************************************
 *
 * 1.22  02/06/88 -- thoms
 * Simplified xlator code to allow any -D<datatype>
 * 3.0 is now the default behaviour
 * 2.6: removed if_pipe_to_of feature
 *
 *
 * ***************************************************************
 *
 * 1.23  19/07/88 -- thoms
 * Added copyright notice and modification history
 * Changed naming to conform to code review
 *
 * ***************************************************************
 *
 * 1.24  21/07/88 -- thoms
 * Check output filter exit status for ct_network connection type
 * so as to enable correct abort/retry behaviour.
 * Added exponential backoff in retry loop.
 *
 * ***************************************************************
 *
 * 1.25 28/07/88 -- thoms
 * Add code to deal with the stderr info from lpscomm
 * (output filter for pt_lps_v3).
 * This involves the parasite filter lpserrof and adds the
 * following functionality:
 *	Supply of pre-loaded resource table to translators
 *	Accounting
 *	Message mail back to user
 *
 * ***************************************************************
 *
 * 1.26 29/07/88 -- thoms
 * Made Dl capability compulsory for PostScript printers
 *
 * ***************************************************************
 *
 * 1.27 29/07/88 -- thoms
 * Added cleanup on job abort
 *
 * ***************************************************************
 *
 * 1.28 01/09/88 -- thoms
 * Added SIGPIPE catcher to cope with output filter problems
 * Pass debug level to lpserrof
 * Fixed open pipe fd to lpserrof bug
 * Correct input_tray/sheetsize interaction bug
 * No longer include errorhandler module
 *
 * ***************************************************************
 *
 * 1.29 07/09/88 -- thoms
 * Made retry on SIGPIPE strict: retry on exit status 1 only
 *
 * ***************************************************************
 *
 * 1.30 09/09/88 -- thoms
 * Fixed argument passing to lpserrof: -J now switches on -Mk mailing
 *
 * ****************************************************************
 *
 * 1.31 16/10/88 -- thoms
 * Hacked out redundant code relating to LPS_v2
 * Added TCP Printserver support
 * Added -p title fix as per 3.0 lpd
 *
 * ****************************************************************
 *
 * 1.32 21/10/88 -- thoms
 * Time stamp key log messages, improve status messages
 * 
 * ****************************************************************
 *
 * 1.33  09/11/88 -- thoms
 * Fixed hanging problem:- switched SIGCHILD to SIG_DEF
 *
 * ****************************************************************
 *
 * 1.34  09/11/88 -- thoms
 * Fixed LPS_SETOUTPUTTRAY bug (was conditional on flagpage)
 * Now supply LPS_JOBJOG module for offset stacking
 *
 * ****************************************************************
 * SCCS history end
 */


#include "lp.h"

/**************** Table for new features enable ****************/

enum enable_flag_code {
	e_IF_pipe_OF, e_printcap_escapes, e_CT_capability,
};


/* Flags to selectively enable new daemon features
 * these are turned on according to the features_enable_tab
 */
static short enable_IF_pipe_OF;	      /* if 1: IF pipes into OF */
static short enable_printcap_escapes; /* if 1; allow printcap escapes */
static short enable_CT_capability; /* if 1 expect CT capability */

static short features_enable_tab[][ULTRIX_upb] = {

/* Ultrix V	3.0	psv1.0	4.0	*/
/* -------------------------------- */
	{	0,	0,	0 },	/* IF_pipe_OF enabled */
	{	0,	1,	1 },	/* printcap_escapes enabled */
	{	0,	1,	1 },	/* CT_capability expected */
};
/****************************************************************/

/*
 * enumeration for state of queue as set in lock file mode bits
 */
enum q_state_e {
	q_enabled, q_disabled, q_rebuild,
};

/**************** related extern functions (not in headers) ****************/

extern enum job_status_e startdqs(/* char *printer */);
extern enum job_status_e sendit(/* char *command_file */); /* sendjob.c */
extern void sendmail(/* char *user, *fromhost, *jobname;
		      * enum job_status_e job_status */);  /* print_utils.c */
extern char *lpd_time();				   /* print_utils.c */
extern int strlookup(/* char **strtab; char *str */);	   /* pcap_choices.c */
extern void strtabprint(/* FILE *fp; char **strtab */);	   /* pcap_choices.c */
extern void mon_setup(/* FCP */);			   /* monitor.c */
extern void start_mon(/* int */), stop_mon();		   /* monitor.c */

/**************** Declarations of local functions ****************/

static int onintr();
static int onquit();
static int onsigpipe();
static void final_cleanup(/* CXP cxp; int sig_num */);
static void init_features();
static void set_connection_type();
static void set_printer_type();
static void process_the_queue(/* CXP cxp */);
static int process_a_job(/* CXP cxp; char *command_file */);
static enum job_status_e try_the_job(/* CXP cxp; char *command_file */);

static enum job_status_e printit(/* char *command_file */);

static void add_ps_module(/* DJP djp; va_dcl */ );
static void cleanup_job(/* char *command_file */);
static void job_init();

/**************** Miscellaneous variables (ugh!!) ****************/

static int	pid;		/* pid of lpd process */
long		pidoff;		/* offset past pid in lock file */
static int	lfd;		/* lock file descriptor */

static int	tof;		/* true if at top of form */

char	ininame[32];		/* initialization file name */
static int	inid;		/* initialization file descriptor */

static char	format_ncpy[] = "%.*s";

static short cf_W_or_Z_found;	/* user specified width or length */
static short cf_O_found;		/* user specified orientation */

/**************** Behaviour switch variables ****************/

static enum printer_type_e printer_type; /* type of printer */
static enum connection_type_e connection_type;/* type of output connection */

/**************** Job building functions ****************/
static void get_resources(/* register FCP xlator, err_fcp; */);

static void non_PS_prolog(/* DJP djp */);
static void LPS_prolog(/* DJP djp */);

static void non_PS_epilog(/* DJP djp */);
static void LN03R_epilog(/* DJP djp */);
static void LPS_epilog(/* DJP djp */);

static enum job_status_e LPS_add_file(/* DJP djp; int format char *file */);
static enum job_status_e non_PS_add_file(/* DJP djp; int format char *file */);

/* check enum printer_type_e against order of this table */

/****************************************************************
*	Switch tables to select behaviour for different printers
****************************************************************/

static job_build_sw job_build_sw_per_printer[] = { 	   /* Printer type */
							   /*--------------*/
	{ non_PS_prolog, non_PS_epilog, non_PS_add_file	}, /* :- pt_non_PS */
	{ LPS_prolog, LN03R_epilog, LPS_add_file	}, /* :- pt_LN03R */
	{ LPS_prolog, LPS_epilog, LPS_add_file		}, /* :- pt_LPS */
};

/*
 * In order to handle the standard error output from the lpscomm
 * network filter we have to do run an extra filter which
 * reads standard error from the output filter
 * The following table is used by open_output_filter() to select
 * the appropriate function according to enum printer_type_e
 */
static void open_of();
static void open_of_and_errorof();

/*
 * RESOURCE TABLE CODE
 */
static void (*open_output_filter_sw[])() = {	/* Printer type */
						/*--------------*/
	open_of,	       			/* :- pt_non_PS */
	open_of,				/* :- pt_LN03R */
	open_of_and_errorof,			/* :- pt_LPS */
};

/****************************************************************
*	Parameters set per job
****************************************************************/
#define TITLE_LEN	79
#define PARAM_LEN	31
#define CLASS_LEN	31
#define DATATYPE_LEN	63
#define LAYUP_FILE_LEN	127

static char	title[TITLE_LEN+1];	/* ``pr'' title */
static char	fromhost[HOSTNAME_LEN+1];	/* user's host machine */
static char	logname[PARAM_LEN+1];	/* user's login name */
static char	jobname[PARAM_LEN+1];	/* job or file name */
static char	job_id[PARAM_LEN+1];	/* job id number */
static char	class[CLASS_LEN+1]; 	/* job class */
static char	width[10];		/* page width in characters */
static char	length[10];		/* page length in lines */
static char	pxwidth[10];		/* page width in pixels */
static char	pxlength[10];		/* page length in pixels */
static char	indent[10];		/* indentation size in characters */

/* These are new for psv1.0 */

static char	username[PARAM_LEN+1];

static char	datatype[DATATYPE_LEN+1];	/* D */
static char	input_tray[PARAM_LEN+1];	/* < */
static char	output_tray[PARAM_LEN+1];	/* > */
static char	orientation[PARAM_LEN+1];	/* O */
static char	pagesize[PARAM_LEN+1];		/* F */
static char	sheetsize[PARAM_LEN+1];		/* S */
static char	message[PARAM_LEN+1];		/* E */
static char	sheetcount[PARAM_LEN+1];	/* X */
static char	lower_pglim[PARAM_LEN+1];	/* A */
static char	upper_pglim[PARAM_LEN+1];	/* B */
static char	number_up[PARAM_LEN+1];		/* G */
static char	layup_file[LAYUP_FILE_LEN+1];	/* z */
static char	sides[PARAM_LEN+1];		/* K */

static char xlator_call_flags[8];

/****************************************************************/

/* This is the set of char pointers used by printcap_escapes,
 * note that the escapes are initialised by reference, so these
 * pointers may be changed in the course of execution,
 * This applies particularly to printer and AF, which have
 * #defines to keep the naming tidy and the information localised.
 */
static char *es_percent =	"%";
static char *es_null =		"";
static char *es_space =		" ";
static char *es_width =		width+2;
static char *es_length =	length+2;
static char *es_title =		title;
static char *es_indent =	indent+2;
static char *es_pxwidth =	pxwidth+2;
static char *es_pxlength =	pxlength+2;
#define      es_printer		printer
static char *es_fromhost =	fromhost;
static char *es_logname =	logname;
static char *es_jobname =	jobname;
static char *es_job_id =	job_id;
#define      es_account_file	AF
static char *es_datatype =	datatype;
static char *es_orientation =	orientation;
static char *es_pagesize =	pagesize;
static char *es_xflags =	&xlator_call_flags[0];

struct es_pair printcap_escapes[] = {
	{ '%', &es_percent },	/* defines the escape character */
	{ '0', &es_null },	/* the null string (for null arguments) */
	{ '_', &es_space },
	{ 'W', &es_width },
	{ 'L', &es_length },
	{ 'T', &es_title },
	{ 'I', &es_indent },
	{ 'X', &es_pxwidth },
	{ 'Y', &es_pxlength },
	{ 'P', &es_printer },
	{ 'H', &es_fromhost },
	{ 'U', &es_logname },
	{ 'j', &es_jobname },
	{ 'J', &es_job_id },
	{ 'A', &es_account_file },
	{ 'D', &es_datatype },
	{ 'O', &es_orientation },
	{ 'F', &es_pagesize },
	{ 'x', &es_xflags },
	{ '\0', 0 }
};

/*
 * Environment buffers for signal catching code
 */
jmp_buf env, sigpipe_env;

printjob(begin)
int	begin;
{
	struct connection out_plug; /* output connection */

	init();			/* set up capabilities */

	(void) write(1, "", 1);	/* ack that daemon is started */
	(void) close(1);	/* set up log file */
	(void) close(2);

	if((begin==1) && (RM==NULL))
		{
		inid = open(ininame,O_RDWR|O_CREAT,0660);	/* create a init file */
		write(inid, "init file\n", 10);
		(void) close(inid);
		}
	if (open(LF, O_WRONLY|O_APPEND) < 0)
		(void) open("/dev/null", O_WRONLY);
	dup(1);
	pid = getpid();				/* for use with lprm */
	setpgrp(0, pid);

	dlog(0, "%s: daemon %d started", lpd_time(), pid);

	init_features();	/* set flags for this printer version */

	set_connection_type();	/* decide connection type */
	cx_init(&out_plug, connection_type); /* initialise connection */

	{
		int signal_number;

		if ((signal_number = setjmp(env)) != 0) {
			final_cleanup(&out_plug, signal_number);
			exit(1);
		} else {
			signal(SIGHUP, onintr);
			signal(SIGINT, onintr);
			signal(SIGQUIT, onquit);
			signal(SIGTERM, onintr);
			signal(SIGCHLD, SIG_DFL);
		}
	}
	set_printer_type();	/* decide printer type (PS or not) */

	/* If we're type PostScript, open Device Control Module archive */

	switch (printer_type) {
	    case pt_non_PS:
		break;
	    case pt_LN03R:
	    case pt_LPS:
		if (!DL) {
			fatal("Dl must be specified for %s printer_type",
			      PS_choices[(int)printer_type]);
			exit(1);
		}
		if (ar_init(DL) != 0) {
			fatal("archive %s not found\n", DL);
		}
		break;
	}
	/*
	 * uses short form file names
	 */
	if (chdir(SD) < 0) {
		log("cannot chdir to %s", SD);
		exit(1);
	}
	lfd = open(LO, O_WRONLY|O_CREAT, 0644);
	if (lfd < 0) {
		log("cannot create %s", LO);
		exit(1);
	}
	if (flock(lfd, LOCK_EX|LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK)	/* active deamon present */
			exit(0);
		log("cannot lock %s", LO);
		exit(1);
	}
	ftruncate(lfd, 0);
	/*
	 * write process id for others to know
	 */
	sprintf(line, "%u\n", pid);
	pidoff = strlen(line);
	if (write(lfd, line, pidoff) != pidoff) {
		log("cannot write daemon pid");
		exit(1);
	}
	tof = !FO;

	process_the_queue(&out_plug);
	final_cleanup(&out_plug, 0);

	exit(0);
}

/*
 * get_q_state -- find state of queue by checking lock file mode
 */
static enum q_state_e
get_q_state()
{
	struct stat stb;

	if (fstat(lfd, &stb) == 0) {
		if (stb.st_mode & 0100) {
			return q_disabled;
		} else if (stb.st_mode & 0001) {
			if (fchmod(lfd, stb.st_mode & 0776) < 0)
			    log("cannot chmod %s", LO);
			return q_rebuild;
		} else {
			return q_enabled;
		}
	} else {
		return q_disabled;
	}
}

static void
process_the_queue(cxp)
register CXP cxp;
{
	int count = 0;
	int nitems = 0;
	register struct queue *q, **qp;
	struct queue **queue;
	enum q_state_e q_state = get_q_state();

	while (1) {
		dlog(1, "process_the_queue: while");
		if (q_state == q_disabled || (nitems = getq(&queue)) <= 0) {
			dlog(1, "process_the_queue: getq -  nitems %d",nitems);
			break;
		}
		q_state = q_enabled;
		for (qp = queue; nitems--; ) {
			dlog(1, "process_the_queue: for nitems %d",nitems);
			q = *qp++;
			switch(q_state) {
			    case q_enabled:
				dlog(1, "process_the_queue: q_enabled");
				count += process_a_job(cxp, q->q_name);
				q_state = get_q_state();
				break;
			    case q_disabled:
			    case q_rebuild:
				dlog(1, "process_the_queue: q_disabled");
				break;
			}
			free((char *) q);
		}
		dlog(1, "process_the_queue:free");
		free((char *) queue);
	}
	if (nitems < 0) {
		log("can't scan spool directory %s", SD);
	}
	if (printer_type == pt_non_PS && count > 0) {
		dlog(1, "process_the_queue: pt_non_ps");
		switch (connection_type) {
		    case con_dev:
		    case con_lat:
			if (!SF && !tof)
			    (void) write(cxp->cx_out_fd,
					 FF, strlen(FF));
			if (TR != NULL)	/* output trailer */
			    (void) write(cxp->cx_out_fd, TR, strlen(TR));
			break;
		    default:
			dlog(1, "process_the_queue: default");
			/* do nothing */
			break;
		}
	}
}

static int
process_a_job(cxp, command_file)
register CXP cxp;
char *command_file;
{
	struct stat stb;
	register int i, n_retries;
	register unsigned how_long;
	enum job_status_e job_status;

	if (stat(command_file, &stb) < 0)
	    return 0;

	(void) lseek(lfd, pidoff, 0);
	(void) sprintf(line, "%s\n", command_file);
	i = strlen(line);
	if (write(lfd, line, i) != i) {
		log("can't write (%d) control file name %s", errno, LO);
		return 0;
	}
	how_long = MIN_RETRY_WAIT;
	for (n_retries = 0; n_retries < PRINT_NRETRIES; n_retries++) {

		dlog(1, "process_the_job: command file %s",command_file);
		job_status = try_the_job(cxp, command_file);

		switch (job_status) {
		    case js_ok:
			return 1; /* job ok and printed */

		    case js_failed:
		    default:
			log("%s: Job %d aborted",
			    lpd_time(), atoi(command_file+3));
			(void)cx_close(cxp); /* close printer connection */
			cleanup_job(command_file, js_failed);
			return 0; /* job failed */

		    case js_retry:
			/* try reprinting the job after a sleep */
			(void)cx_close(cxp); /* close printer connection */
			log("%s: Job %d, Retry #%d in %d secs",
			    lpd_time(), atoi(command_file+3), n_retries + 1,
			    how_long);
			status("Sleeping for %d seconds before retrying",
			       how_long);
			sleep(how_long);
			break;
		}
		how_long = (((how_long * 2) > MAX_RETRY_WAIT)
			    ? MAX_RETRY_WAIT : (how_long * 2));
	}
	log("Job %d abandonned after %d retries",
	    atoi(command_file+3), n_retries);
	cleanup_job(command_file, js_too_many_retries);

	return 0;
}

static enum job_status_e
try_the_job(cxp, command_file)
register CXP cxp;
char *command_file;
{
	enum job_status_e job_status = js_failed;

	if (setjmp(sigpipe_env) == 0) {
		signal(SIGPIPE, onsigpipe);

		job_init();
		switch(connection_type) {
		    case con_dev:
		    case con_lat:
		    case con_tcp:
			/*
			 * actions associated with cx_open and cx_start
			 * only happen if needed here since out_plug
			 * maintains state and knows what to do
			 */
			(void)cx_open(cxp);
			cx_start(cxp);
			job_status = printit(cxp, command_file);
			break;

		    case con_remote:
			/* open output if necessary */
			(void)cx_open(cxp);
			job_status = sendit(cxp, command_file);
			break;

		    case con_network:
			job_status = printit(cxp, command_file);
			break;
		    case con_dqs:  /* for  dqs - print */
			dlog(1, "try_the_job: connection type dqs");
			job_status = startdqs(printer,cxp,command_file);
			if(job_status == js_ok)
			dlog(1, "try_the_job: job_status is OK exit child");
			else
			dlog(1, "try_the_job: job_status FAILED exit child");
			exit(0);
		    default:
			log("Unknown connection type");
			break;
		}
	} else {
		int cx_ret_code;

		stop_mon();
		dlog(0, "caught SIGPIPE writing to output filter");
		cx_ret_code = cx_close(cxp);
		job_status = ((cx_ret_code == 1) ? js_retry : js_failed);
	}
	return job_status;
}

char	fonts[4][50];	/* fonts for troff */

static char ifonts[4][18] = {
	"/usr/lib/vfont/R",
	"/usr/lib/vfont/I",
	"/usr/lib/vfont/B",
	"/usr/lib/vfont/S"
};

static void
cleanup_job(command_file, why)
char *command_file;
enum job_status_e why;
{
	FILE	*cfp;			/* control file */
	/*
	 * open control file
	 */
	if ((cfp = fopen(command_file, "r")) == NULL) {
		log("open failure (%d) of control file %s",
		    errno, command_file);
		/* shouldn't happen, must have opened ok before */
		return;
	}
	cf_pass2(cfp, why);
	/*
	 * clean-up incase another control file exists
	 */
	(void) fclose(cfp);
	(void) unlink(command_file);
}


/*
 * cf_pass2 --
 *
 * Second pass through command file called by printit and cleanup_job
 */
int
cf_pass2(cfp, job_status)
FILE *cfp;			/* control file */
enum job_status_e job_status;

{
	fseek(cfp, 0L, 0);
	while (getline(cfp)) {
		switch (line[0]) {
		case 'M':
			sendmail(line+1, fromhost, jobname, job_status);
			continue;

		case 'U':
			(void) unlink(line+1);
			continue;
		    default:
			continue;
		}
	}
}

/* build filter
 * Parse the string prog into filter elements, replace escapes if found,
 * If no escapes are found then add in default arguments
 * for backward compatibility (NB remember to terminate with 0)
 * The filter(s) are appended to the filter_chain *fcp.
 * The escapes are described by the escapes pointed to by esp.
 */
/*VARARGS3*/
void
build_filter(fcp, esp, prog, va_alist)
FCP fcp;			/* filter chain we're building */
ESP esp;			/* The escapes */
char *prog;			/* Program string */
va_dcl				/* Default arguments to filter 0 terminated */
{
	struct filter_chain filter_tmp;
	int escapes_found = 0; /* return flag from do_escapes */
	register char *p;

	fc_init(&filter_tmp);	/* tmp: parsed first program put here */

	/* parse the command in prog */
	for (p = parse_prog(&filter_tmp, prog); p; ) {
		p = parse_prog(&filter_tmp, p);
	}
	if (enable_printcap_escapes) {
		escapes_found = do_escapes(&filter_tmp, esp);
	}
	/* append first command to filter chain */
	fc_add_args_v(fcp, filter_tmp.fc_argv[0]);
	if (!escapes_found) {
		va_list args;

		/* Add in default args to first command in pipeline
		 */
		va_start(args);
		fc_add_args_va(fcp, args);
		va_end(args);
	}
	fc_end_filter(fcp);			/* terminate 1st command */
	{
		int i;		/* loop index */
		for (i = 1; i < filter_tmp.fc_nf; i++) {
			/* append each command to filter chain */
			fc_add_args_v(fcp, filter_tmp.fc_argv[i]);
			fc_end_filter(fcp);
		}
	}
	fc_delete(&filter_tmp, 0);
}

/* build_filter_chain
 * Set up filter chain
 *    [ {PR, PP} | ] [ IF | ] OF
 * or
 *    { RF, TF, NF, DF, CF, VF }.
 *
 * Instructions whether OF is needed is passed by return code.
 *
 * Changes:
 * (PP added to above description, AT 26-jan-88)
 * (semantics of IF changed, AT 28-jan-88)
 * Returns:
 */
#define BFC_ERR			(-1) /* foul up */
#define BFC_USE_OUTPUT_FILTER	1 /* use the output filter */
#define BFC_BYPASS		0 /* don't use the output filter */

static int
build_filter_chain(format, fcp)
int format;
register FCP fcp;	/* filter constructed in here */
{
	int retcode = BFC_BYPASS;
	struct escapes escapes;	/* escapes */

	es_init(&escapes, printcap_escapes);

	switch(format) {
		int fd;
		int i;
		static char *vfpre = "/usr/lib/vfont/";

	    case 'p':		/* print using pr-like filter */
		build_filter(fcp, &escapes, PP ? PP : PR,
			     width, length, "-h", title, 0);
		if (IF) {
			build_filter(fcp, &escapes, IF,
				     width, length, indent,
				     "-n", logname, "-h", fromhost, AF, 0);
			if (enable_IF_pipe_OF) retcode = BFC_USE_OUTPUT_FILTER;
		} else {
			retcode = BFC_USE_OUTPUT_FILTER;
		}
		break;
	    case 'f':		/* print plain text using IF */
		if (IF) {
			build_filter(fcp, &escapes, IF,
				     width, length, indent,
				     "-n", logname, "-h", fromhost, AF, 0);
			if (enable_IF_pipe_OF) {
				retcode = BFC_USE_OUTPUT_FILTER;
			}
		} else {
			retcode = BFC_USE_OUTPUT_FILTER;
		}
		break;
	    case 'l':		/* like 'f', pass Ctrl chars */
		if (IF) {
			build_filter(fcp, &escapes, IF,
				     "-c", width, length, indent,
				     "-n", logname, "-h", fromhost, AF, 0);
			if (enable_IF_pipe_OF) {
				retcode = BFC_USE_OUTPUT_FILTER;
			}
		} else {
			retcode = BFC_USE_OUTPUT_FILTER;
		}
		break;
	    case 'r':		/* print fortran text file */
		build_filter(fcp, &escapes, RF,
			     width, length,
			     "-n", logname, "-h", fromhost, AF, 0);
		break;
	    case 'n':		/* print ditroff output */
		build_filter(fcp, &escapes, NF,
			     "-c", width, length, indent,
			     "-n", logname, "-h", fromhost, AF, 0);
		break;
	    case 't':		/* print troff output */
	    case 'd':		/* print tex output */
		(void) unlink(".railmag");
		if ((fd = creat(".railmag", FILMOD)) < 0) {
			log("cannot create .railmag");
			(void) unlink(".railmag");
		} else {
			for (i=0; i < 4; i++) {
				if (fonts[i][0] != '/')
				    (void) write(fd, vfpre, strlen(vfpre));
				(void) write(fd, fonts[i], strlen(fonts[i]));
				(void) write(fd, "\n", 1);
			}
			(void) close(fd);
		}
		build_filter(fcp, &escapes, (format == 't') ? TF : DF,
			     pxwidth, pxlength,
			     "-n", logname, "-h", fromhost, AF, 0);
		break;
	    case 'c':		/* print cifplot output */
		build_filter(fcp, &escapes, CF,
			     pxwidth, pxlength,
			     "-n", logname, "-h", fromhost, AF, 0);
		break;
	    case 'g':		/* print plot(1G) output */
		build_filter(fcp, &escapes, GF,
			     pxwidth, pxlength,
			     "-n", logname, "-h", fromhost, AF, 0);
		break;
	    case 'v':		/* print raster output */
		build_filter(fcp, &escapes, VF,
			     pxwidth, pxlength,
			     "-n", logname, "-h", fromhost, AF, 0);
		break;
	    case 'x':		/* print via pass-thru filter */
		build_filter(fcp, &escapes, XF, 0);
		break;
	    case 'z':
		build_filter(fcp, &escapes, pc_layup, 0);
		break;
	    default:
		log("illegal format character '%c'", format);
		retcode = BFC_ERR;
		break;
	}
	es_delete(&escapes);
	return retcode;
}

/*
 * open_output_filter -- if OF is specified in printcap open it
 */
void
open_output_filter(cxp)
register CXP cxp;
{
	(*open_output_filter_sw[(int)printer_type])(cxp);
}

/*
 * open_of -- if OF is specified in printcap open it
 */
static void
open_of(cxp)
register CXP cxp;
{
	if (OF) {
		struct escapes escapes;	/* escapes */

		cxp->cx_output_filter = new_fc();

		es_init(&escapes, printcap_escapes);

		build_filter(cxp->cx_output_filter, &escapes, OF,
			     width, length, 0);

		es_delete(&escapes);

		fc_plumb_and_run(cxp->cx_output_filter, DOABORT,
				 FC_MAKEPIPE, cxp->cx_pr_fd, 2);

		cxp->cx_out_fd = cxp->cx_output_filter->fc_fds[FC_STDIN];
	} else {
		cxp->cx_out_fd = cxp->cx_pr_fd;
	}
}

/*
 * open_of_and_errorof -- if OF is specified in printcap open it
 *
 * Description:
 *	Also opens up the lpserrof filter.
 *	This filter processes the stderr output of the lpscomm
 *	filter and processes it accordingly.
 *	The plumbing of lpscomm and lpserrof are as follows:
 *
 *	----------------------------------------------------------------
 *	lpscomm:
 *	fds:	0	...	pipe A from lpd
 *		1	...	closed
 *		2	...	pipe B to lpserrof
 *
 *	----------------------------------------------------------------
 *	lpserrof:
 *	fds:	0	...	pipe B from lpscomm
 *		1	...	pipe C to lpd
 *		2	...	copy of lpd fd 2 (to log file)
 *
 *	----------------------------------------------------------------
 */
static void
open_of_and_errorof(cxp)
register CXP cxp;
{
	register FCP err_fcp;
	struct escapes escapes;	/* escapes */
	char debug_level[8];

	cxp->cx_output_filter = new_fc();
	err_fcp = cxp->cx_output_filter->fc_next = new_fc();

	es_init(&escapes, printcap_escapes);

	build_filter(cxp->cx_output_filter, &escapes, OF,
		     width, length, 0);
	es_delete(&escapes);

	sprintf(debug_level, "%d", DB);

	fc_add_args_l(err_fcp, "lpserrof",
		      "-d", debug_level,
		      "-U", logname,
		      "-H", fromhost,
		      "-P", printer,
		      0);

	if (!strcmp(message, "keep")) {
		fc_add_args_l(err_fcp,
			      "-J", job_id,
			      0);
	}
	if (AF) fc_add_args_l(err_fcp,
			      "-A", AF,
			      0);

	fc_end_filter(err_fcp);

	fc_plumb_and_run(err_fcp, DOABORT,
			 FC_MAKEPIPE, FC_MAKEPIPE, 2);
	
	fc_plumb_and_run(cxp->cx_output_filter, DOABORT,
			 FC_MAKEPIPE,
			 cxp->cx_pr_fd,
			 err_fcp->fc_fds[FC_STDIN]);

	/*
	 * lpd doesn't want to write to lpserrof so we
	 * must close spare pipe end
	 */
	(void) close(err_fcp->fc_fds[FC_STDIN]);
	cxp->cx_out_fd = cxp->cx_output_filter->fc_fds[FC_STDIN];
}


static enum job_status_e
printit(cxp, command_file)
register CXP cxp;
register char *command_file;		/* name of command file */
{
	FILE *cfp;		/* control file */
	register int command;
	register char *line_param = &line[1];
	struct dcl_job job;	/* build job description here */
	enum job_status_e job_status = js_ok;
	int first_file_found=0;	/* flag for first file to print */

	/*
	 * tof usage:
	 *	While we are building the job tof is diddled to
	 *	indicate the state the printer WILL be in when
	 *	the job is actually printed.
	 *	When we return from printit(), reality has caught up
	 */
	sprintf(job_id, "%d", atoi(command_file+3)); /* used by PS flagpage */

	if ((cfp = fopen(command_file, "r")) == NULL) {
		log("open failure (%d) of control file %s", errno, command_file);
		return(js_failed); /* used to return ok */
	}

	/*  # format of command file produced by lpr #
	 *
	 *  H	<host> host name on banner page
	 *  P	<user> user's login name
	 *  J	<job> job name on banner page
	 *  C	<class> class name on banner page
	 *  L	<user> literal user name on banner page
	 *  I	<num> amount to indent output for f and l filters
	 *  M	<user> mail user on completion
	 *  1	<font> R font file for troff, ditroff and TeX
	 *  2	<font> I font file for troff, ditroff and TeX
	 *  3	<font> B font file for troff, ditroff and TeX
	 *  4	<font> S font file for troff, ditroff and TeX
	 *  W	<num> width used by pr filter
	 *  Z	<num> length used by pr filter
	 * *D	<data_type> (ansi | regis | tek4014 | postscript)
	 * *<	<input_tray> (top | middle | bottom)
	 * *>	<output_tray> (top | side | face-up)
	 * *O	<orientation> (portrait | landscape)
	 * *F	<page_size> (a | a3 | a4 | a5 | b | b4 | b5 | ex | leg
	 *		| c4 | c5 | dl | 10x13 | 9x12 | bus)
	 * *S	<sheet_size> (a | a3 | a4 | a5 | b | b4 | b5 | ex | leg
	 *		| c4 | c5 | dl | 10x13 | 9x12 | bus)
	 * *E	<message> (print | keep | p+k | ign)
	 * *X	<num> [1,10000] sheet count
	 * *A	<num> [1,10000] lower page limit
	 * *B	<num> [1,10000] upper page limit
	 * *G	<num> [1,100] number up
	 * *z	<filename> layup definition file
	 * *K	<sides> (1 | 2 | tumble | one_sided_duplex
	 *		| one_sided_tumble | two_sided_simplex)
	 *  T	<title> header for pr filter
	 *  f	<file name> text file
	 *  p	<file name> text file for pr filter
	 *  l	<file name> text file with control chars
	 *  t	<file name> troff file
	 *  n	<file name> ditroff file
	 *  d	<file name> TeX file
	 *  g	<file name> plot file
	 *  v	<file name> raster file
	 *  c	<file name> cifplot file
	 *  r	<file name> FORTRAN file
	 *  x	<file name> file for transparent filter
	 *  U	<file name> unlink file after printing/spooling
	 *  N	<name> name of first file (used by lpq)
	 * 	 */

	dj_init(&job, cxp,
		&job_build_sw_per_printer[(int)printer_type]);

	while (getline(cfp))
	    switch(command = line[0]) {
		case 'H':
		    sprintf(fromhost, format_ncpy, PARAM_LEN, line_param);
		    if (class[0] == '\0')
			sprintf(class, format_ncpy, CLASS_LEN, line_param);
		    continue;

		case 'P':
		    sprintf(logname, format_ncpy, PARAM_LEN, line_param);
		    if (RS) {	/* restricted */
			    if (getpwnam(logname) == (struct passwd *)0) {
				    job_status = js_restricted;
				    goto xpass2;
			    }
		    }
		    continue;

		case 'J':
		    /* at least 1 char */
		    sprintf(jobname, "%1.*s", PARAM_LEN, line_param);
		    continue;

		case 'C':
		    if (*line_param != '\0')
			sprintf(class, format_ncpy, CLASS_LEN, line_param);
		    else if (class[0] == '\0')
			gethostname(class, sizeof (class));
		    continue;

		case 'T':	/* header title for pr */
		    /* set to " " if title is null string */
		    sprintf(title, "%1.*s", TITLE_LEN, line_param);
		    continue;

		case '1':	/* troff fonts */
		case '2':
		case '3':
		case '4':
		    if (*line_param != '\0')
			strcpy(fonts[command-'1'], line_param);
		    continue;
		case 'Z':	/* page length */
		    strcpy(length+2, line_param);
		    cf_W_or_Z_found = 1;
		    continue;
		case 'W':	/* page width */
		    strcpy(width+2, line_param);
		    cf_W_or_Z_found = 1;
		    continue;

		case 'I':	/* indent amount */
		    strcpy(indent+2, line_param);
		    continue;

		    /* Here are the new ones for PostScript printers */
		case 'L':
		    if (!SH)
			sprintf(username, format_ncpy, PARAM_LEN, line_param);
		    continue;

		case 'D':
		    sprintf(datatype, format_ncpy, DATATYPE_LEN, line_param);
		    continue;

		case '<':
		    sprintf(input_tray, format_ncpy, PARAM_LEN, line_param);
		    continue;

		case '>':
		    sprintf(output_tray, format_ncpy, PARAM_LEN, line_param);
		    continue;

		case 'O':
		    sprintf(orientation, format_ncpy, PARAM_LEN, line_param);
		    cf_O_found = 1;
		    continue;

		case 'F':
		    sprintf(pagesize, format_ncpy, PARAM_LEN, line_param);
		    continue;

		case 'S':
		    sprintf(sheetsize, format_ncpy, PARAM_LEN, line_param);
		    continue;

		case 'E':
		    sprintf(message, format_ncpy, PARAM_LEN, line_param);
		    continue;

		case 'X':
		    sprintf(sheetcount, format_ncpy, PARAM_LEN, line_param);
		    continue;

		case 'A':
		    sprintf(lower_pglim, format_ncpy, PARAM_LEN, line_param);
		    continue;

		case 'B':
		    sprintf(upper_pglim, format_ncpy, PARAM_LEN, line_param);
		    continue;

		case 'G':
		    sprintf(number_up, format_ncpy, PARAM_LEN, line_param);
		    continue;

		case 'z':
		    sprintf(layup_file, format_ncpy,
			    LAYUP_FILE_LEN, line_param);
		    continue;

		case 'K':
		    sprintf(sides, format_ncpy, PARAM_LEN, line_param);
		    continue;

		default:	/* [a-z] => some file to print */
		    if (command >= 'a' && command <= 'z') {
			    if (first_file_found == 0) {
				    first_file_found++;

				    /*
				     * This cx_open is needed for
				     * network filters
				     * Done here so that
				     * LPS prolog can access
				     * Resource table.
				     * Start monitoring here because the
				     * prologue can hang the job.
				     */
				    (void)cx_open(cxp);
				    if (connection_type == con_network)
				    	start_mon((int)con_network);
				    dj_prolog(&job);
			    }
			    job_status = dj_add_file(&job, command, line_param);
			    job.dj_flags |= DJ_AFTER_FIRST_FILE;

			    if (job_status != js_ok) {
				    goto xpass2;
			    }
		    } else {
			    log("Undefined command letter %c", command);
		    }
		    continue;

		case 'N':	/* used by lpq only */
		case 'U':	/* handled on pass 2 */
		case 'M':	/* handled on pass 2 */
		    continue;
	    }
	dj_epilog(&job);
	/*
         * Start monitoring job progress if not network connection.
	 * Start the job!
	 * If we are a network printer, then we
	 * close the output connection here for each job.
	 */
	if (connection_type == con_network) {
		enum job_status_e xlator_status;
		int cx_ret_code;

		xlator_status = dj_exec(&job);
		cx_ret_code = cx_close(cxp);

		job_status = ((cx_ret_code == 0) ? js_ok :
			      (cx_ret_code > 0) ? js_retry : js_failed);

		/*
		 * If the output filter aborts, we are more
		 * interested in its status than in the
		 * status of the xlator which may have failed
		 * due to an uncaught SIGPIPE
		 * as a consequence of writing on its output pipe
		 * after the output filter has closed it
		 */
		if (job_status == js_ok) job_status = xlator_status;
	} else {
		start_mon((int)cxp->cx_type);
		job_status = dj_exec(&job);
	}
	stop_mon();		/* Probably already stopped! */
	dj_delete(&job, 0);
    xpass2:
	switch(job_status) {
	    default:
		cf_pass2(cfp, job_status);
		(void) unlink(command_file);
		(void) fclose(cfp);
		break;

	    case js_retry:
		tof = 0;
		(void) fclose(cfp);
		break;
	}
	return(job_status);
}


/*VARARGS1*/
static void
add_ps_module(djp, va_alist)
     register DJP djp;
     va_dcl
{
	va_list args;
	va_start(args);

  	dj_start_module(djp, je_archive, 0);
  	dj_add_args_va(djp, args);
  	dj_end_module(djp);

	va_end(args);
}

/****************************************************************/

static void
non_PS_prolog(djp)
     register DJP djp;
{
	/* This is for old style banner page*/
	if (!SF && !tof) {
		dj_start_module(djp, je_outstr, 0);
		dj_add_arg(djp, FF); /* Output Form Feed */
		dj_end_module(djp);
	}
	if (*username) {
		dj_start_module(djp, je_banner, 0);
		dj_add_args_l(djp, username, jobname, class, 0);
		dj_end_module(djp);

		if (!SF) {
			dj_start_module(djp, je_outstr, 0);
			dj_add_arg(djp, FF); /* Output Form Feed */
			dj_end_module(djp);
		}
	}
	tof = 1;
}

/*
 * LPS_prolog -- Generate prolog for V2.0 or V3.0 Postscript job
 */
static void
LPS_prolog(djp)
register DJP djp;
{
	int sheet_size_defaulted = 0;
	char name_tmp[(2 * PARAM_LEN) + 2];

	extern int pc_orientation_found; /* print_utils.c */

	get_resources(djp);

	djp->dj_flags |=
	    ((cf_W_or_Z_found || cf_O_found) ?
	     (!cf_O_found ? DJ_WIDTH_WINS : 0) :
	     (!pc_orientation_found) ? DJ_WIDTH_WINS : 0);
		
	if (*output_tray)
	    add_ps_module(djp, "LPS_SETOUTPUTTRAY", output_tray, 0);

	add_ps_module(djp, "LPS_JOBJOG", 0);

	if (*username) {
		sprintf(name_tmp, "%s@%s", logname, fromhost);

		add_ps_module
		    (djp, "LPS_FLAGPAGE",
		     "",	/* VMesS version number*/
		     lpd_time(),
		     "",	/* node name */
		     printer,	/* queue name */
		     "","","","","","","","","",
		     jobname,	/* job name */
		     job_id,	/* job id number */
		     name_tmp,	/* source of job (user@host) */
		     "",	/* note: */
		     "JOB",	/* type of page */
		     "",	/* is it a burst page? */
		     0);
	}
	add_ps_module(djp, "LPS_LOADDICT", 0);
	/*
	 * Parameter related stuff gets put in between here
	 * and LPS_SETCONTEXT.
	 */
	if (!(*number_up || *layup_file)) {
		if (pc_number_up)
		    sprintf(number_up, format_ncpy, PARAM_LEN, pc_number_up);
		if (pc_layup_file)
		    sprintf(layup_file, format_ncpy, PARAM_LEN, pc_layup_file);
	}
	if (!(*number_up) && *layup_file)
	    strcpy(number_up, "1");
	if (*number_up && atoi(number_up) != 0) {
		add_ps_module(djp, "LPS_SETNUMBERUP", number_up, 0);
		djp->dj_flags |= DJ_HAVE_SETNUMBERUP;
	}

	if (*sides)
	    add_ps_module(djp, "LPS_SETSIDES", sides, 0);
	if (*output_tray)
	    add_ps_module(djp, "LPS_SETOUTPUTTRAY", output_tray, 0);
	if (*orientation)
	    add_ps_module(djp, "LPS_SETPAGEORIENTATION", orientation, 0);

	if (!(*sheetsize || *pagesize || *input_tray)) {
		if (pc_sheetsize)
		    sprintf(sheetsize, format_ncpy, PARAM_LEN, pc_sheetsize);
		if (pc_pagesize)
		    sprintf(pagesize, format_ncpy, PARAM_LEN, pc_pagesize);
		if (pc_input_tray)
		    sprintf(input_tray, format_ncpy, PARAM_LEN, pc_input_tray);
	}
	/* If neither sheet size or page size is set
	 * then use the fall-back sheet size, pc_sheetstd
	 */
	if (!(*pagesize || *sheetsize)) {
		sprintf(sheetsize, format_ncpy, PARAM_LEN, pc_sheetstd);
		sheet_size_defaulted = 1;
	}
	/*
	 * Either pagesize or sheetsize must be set by this point.
	 * If either is not set it defaults to the other.
	 */
	if (!*pagesize) {
		strcpy(pagesize, sheetsize);
	} else if (!*sheetsize) {
		strcpy(sheetsize, pagesize);
	}
	if (*input_tray)
	    add_ps_module(djp, "LPS_SETINPUTTRAY", input_tray, 0);
 	add_ps_module(djp, "LPS_SETPAGESIZE", pagesize, 0);
	add_ps_module(djp, "LPS_SETSHEETSIZE",
		      ((sheet_size_defaulted)
		       ? "defaulted" : "not-defaulted"),
		      sheetsize, 0);
	if (*sheetcount)
	    add_ps_module(djp, "LPS_SETSHEETCOUNT", sheetcount, 0);
	if (*lower_pglim || *upper_pglim)
	    add_ps_module(djp, "LPS_SETPAGELIMIT",
			  (*lower_pglim) ? lower_pglim : "1",
			  (*upper_pglim) ? upper_pglim : "1000000", 0);

	if (*layup_file && LPS_add_file(djp, 'z', layup_file) != js_ok) {
		log("Sorry don't know about layup yet");
	}
	add_ps_module(djp, "LPS_SETCONTEXT", 0);

	/* now we're ready for first file */
}

static enum job_status_e
non_PS_add_file(djp, format, file)
     register DJP djp;
     int format;
     char *file;
{
	FCP xlator = new_fc();
 	int bfc_code = build_filter_chain(format, xlator);

	if (bfc_code == BFC_ERR) {
		return js_failed;
	}

	mon_setup(xlator);		/* This enables the progess monitor */

	if (!SF && !tof){
		dj_start_module(djp, je_outstr, 0);
		dj_add_arg(djp, FF); /* Output Form Feed */
		dj_end_module(djp);
		tof = 1;
	}
	if (xlator->fc_nf == 0) tof = 0; /* no filter, not at tof */

	dj_start_module(djp, ((bfc_code == BFC_BYPASS) ? je_bypass : je_print),
			(opaque_method)xlator);
	dj_add_arg(djp, file);
	dj_end_module(djp);

	return js_ok;
}

static void
get_resources(djp)
register DJP djp;
{
	int fd;
	FILE *res_fp = NULL;
	char buf[322];		/* Guaranteed maximum res. size */
	FCP err_fcp, res_fcp;

	if (djp->dj_cxp->cx_output_filter &&
	    (err_fcp = djp->dj_cxp->cx_output_filter->fc_next)) {
		fd = err_fcp->fc_fds[1];
	} else {
		return;
	}
	res_fcp = djp->dj_resources = new_fc();

	if (fd < 0 || (res_fp = fdopen(fd, "r")) == NULL) {
		dlog(0, "Can't get resource info");
		return;
	}
	while (fgets(buf, 322, res_fp)) {
		register char *newline;

		dlog(0, "Resource report: %s", buf);
		if (!(newline = strchr(buf, '\n'))) break;
		*newline = '\0';
		fc_add_args_l(res_fcp, "-R", buf, 0);
	}
	fclose(res_fp);
	err_fcp->fc_fds[1] = FC_FD_CLOSED;
	fc_end_filter(res_fcp);
}

/*
 * LPS_add_file -- add a file translation module to job description
 *
 * Description:
 *	If the filter to be used is a translator according to
 *	the datatype parameter then xlator_call script is invoked.
 *	Information about preloaded resources is appended to the
 *	argument list.
 */
static enum job_status_e
LPS_add_file(djp, format, file)
register DJP djp;
int format;
char *file;
{
	FCP xl_tmp = new_fc();
	FCP xlator = new_fc();
 	int bfc_code;

	sprintf(xlator_call_flags, "-%s%s",
		(djp->dj_flags & DJ_AFTER_FIRST_FILE) ? "h" : "",
		(djp->dj_flags & DJ_WIDTH_WINS) ? "w" : "");


	bfc_code = build_filter_chain(format, xlator);

	mon_setup(xlator);		/* This enables the progess monitor */

	if (bfc_code == BFC_ERR) {
		return js_failed;
	} else if (bfc_code == BFC_USE_OUTPUT_FILTER) {
		/*
		 * For PostScript jobs: BFC_USE_OUTPUT_FILTER is interpreted
		 * to mean use the data_type translator via xlator_call
		 */
		struct escapes escapes;

		if (djp->dj_flags & DJ_AFTER_FIRST_FILE) {
			add_ps_module(djp, "LPS_SEPARATE", 0);
		}

		es_init(&escapes, printcap_escapes);
		/*
		 * This builds xlator_call calling
		 * sequence.
		 * Main difference from v2 is that we
		 * have to append the Resource info
		 * hence the extra copy of the arguments
		 */
		build_filter(xl_tmp, &escapes, pc_xlator,
			     es_xflags,
			     es_datatype,
			     es_pagesize,
			     es_orientation,
			     es_width,
			     es_length,
			     es_indent,
			     0);
		fc_add_args_v(xlator, xl_tmp->fc_argv[0]);
		fc_delete(xl_tmp, 1);

		if (djp->dj_resources)
		    fc_add_args_v(xlator, djp->dj_resources->fc_argv[0]);

		fc_end_filter(xlator);
		es_delete(&escapes);
	}
	dj_start_module(djp, je_print, (opaque_method)xlator);
	dj_add_arg(djp, file);
	dj_end_module(djp);

	return js_ok;
}

static void
non_PS_epilog(djp)
register DJP djp;
{
	return;			/* Thats it until we do trailers */
}

static void
LN03R_epilog(djp)
register DJP djp;
{
	if (djp->dj_flags & DJ_HAVE_SETNUMBERUP)
	    add_ps_module(djp, "LPS_FLUSHPAGES", 0);
	add_ps_module(djp, "LPS_EOJ", 0);

	dj_start_module(djp, je_outstr, 0);
	dj_add_arg(djp, "\004"); /* Output ^D */
	dj_end_module(djp);
}

static void
LPS_epilog(djp)
register DJP djp;
{
	if (djp->dj_flags & DJ_HAVE_SETNUMBERUP)
	    add_ps_module(djp, "LPS_FLUSHPAGES", 0);
	add_ps_module(djp, "LPS_EOJ", 0);
}

/****************************************************************/

/*
 * Cleanup child processes when a signal is caught.
 */
static int
onintr()
{
	longjmp(env, SIGINT);
}

static int
onquit()
{
	longjmp(env, SIGQUIT);
}

static int
onsigpipe()
{
	longjmp(sigpipe_env, SIGPIPE);
}

/****************************************************************/

static void
final_cleanup(cxp, sig_num)
register CXP cxp;
int sig_num;
{
	/* SIG_IGN needed else next line is suicide under Posix (clever huh?)*/
	if (sig_num != 0) {
		signal(SIGINT, SIG_IGN);
		kill(0, SIGINT);
	}
	cx_close(cxp);
	cx_delete(cxp, 0);

	switch (sig_num) {
	    case 0:
		dlog(0, "%s: daemon %d exit", lpd_time(), pid);
		break;
	    case SIGQUIT:
		dlog(0, "%s: daemon %d dumped core due to SIGQUIT",
		     lpd_time(), pid);
		kill(pid, SIGIOT);
		break;
	    default:
		dlog(0, "%s: daemon %d killed by signal %d",
		     lpd_time(), pid, sig_num);
		break;
	}
}

/****************************************************************/

static void
init_features()
{

	int uv_code;

	uv_code = strlookup(UV_choices, UV);

	if (uv_code < 0) {
		uv_code = (int)ULTRIX_base;
		log("Ultrix version %s unrecognised, use one of:");
		strtabprint(stderr, UV_choices);
		exit(1);
	} else {
		dlog(0, "Ultrix version for daemon enhancements: %s",
		    UV_choices[uv_code]);
	}
	enable_IF_pipe_OF =
	    features_enable_tab[(int)e_IF_pipe_OF][uv_code];
	enable_printcap_escapes =
	    features_enable_tab[(int)e_printcap_escapes][uv_code];
	enable_CT_capability =
	    features_enable_tab[(int)e_CT_capability][uv_code];
}

static void set_connection_type()
{
	int ct_code;

	/* Find out what sort of beast we are */

	if (enable_CT_capability) {
		/* Good, we know what we are connected to */
		ct_code = strlookup(CT_choices, CT);
		if (ct_code < 0) {
			log("Unknown connection type, use one of\n");
			strtabprint(stderr, CT_choices);
			exit(1);
		}
		connection_type = (enum connection_type_e)ct_code;
	} else {
		/* Never mind, have to guess instead! */
		if (TS && *TS) connection_type = con_lat;
		else if (*LP && !RM) {
			if (LP[0] == '@')
			    connection_type = con_tcp;
			else
			    connection_type = con_dev;
		} else if (RM && !*LP)
		    connection_type = con_remote;
		else {
			log("impossible lp and rm combination in printcap");
			exit(1);
		}
	}
}

static void set_printer_type()
{
	int ps_code;

	ps_code = strlookup(PS_choices, PS);
	if (ps_code < 0) {
		log("Unknown printer type, use one of\n");
		strtabprint(stderr, PS_choices);
		exit(1);
	}
	printer_type = (enum printer_type_e)ps_code;
}

static char *reset_to_null_list[] = {
	fromhost,
	logname,
	jobname,
	class,
	username,
	datatype,
	input_tray,
	output_tray,
	orientation,
	pagesize,
	sheetsize,
	message,
	sheetcount,
	lower_pglim,
	upper_pglim,
	number_up,
	layup_file,
	sides,
	0
};

/*
 * job_init -- initialise all strings with scope of current job only
 */
static void job_init()
{
	register char **p;

	for (p = reset_to_null_list; *p; p++) **p = '\0';

	sprintf(width, "-w%d", PW);
	sprintf(length, "-l%d", PL);
	sprintf(pxwidth, "-x%d", PX);
	sprintf(pxlength, "-y%d", PY);
	strcpy(indent,	"-i0");	/* indentation size in characters */
	strcpy(title,	" ");	/* title string */

	if (pc_datatype)
	    sprintf(datatype, format_ncpy, DATATYPE_LEN, pc_datatype);

	if (pc_output_tray)
	    sprintf(output_tray, format_ncpy, PARAM_LEN, pc_output_tray);
	if (pc_orientation)
	    sprintf(orientation, format_ncpy, PARAM_LEN, pc_orientation);
	/*
	 * input_tray, sheetsize and pagesize have a complex default
	 * mechanism and are not set up here
	 */
	if (pc_message)
	    sprintf(message, format_ncpy, PARAM_LEN, pc_message);
	if (pc_upper_pglim)
	    sprintf(upper_pglim, format_ncpy, PARAM_LEN, pc_upper_pglim);
	/*
	 * number_up and layup_file interact and hence the
	 * defaults are not set up here
	 */
	if (!*sides && pc_sides) {
		sprintf(sides, format_ncpy, PARAM_LEN, pc_sides);
	}
}
