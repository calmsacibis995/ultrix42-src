#ifndef lint
static char *sccsid = "@(#)monitor.c	4.1	ULTRIX	7/2/90";
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
 * monitor.c - Monitor progress of the print job and update queue status.
 *
 * Description:
 * Functions to monitor progress in printing a file.   If no progress is
 * observed then the queue status is changed to reflect this.  When
 * printing resumes it is changed back.
 *
 * A simple fixed timer monitors the printing of the flag page.
 * Monitoring the printing of the file itself is a littel harder.
 * It works by triggering an alarm signal every few minutes and
 * examining the input file pointer in the signal handler to see if any more
 * of the file has been read.    It would seem simpler to count writes
 * to the printer, but doing it this way allows it to be centralised.
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 2.1   3/08/89 -- Giles Atkinson
 * Original version
 * 
 * ***************************************************************
 *
 * 2.2  16/08/89 -- Giles Atkinson
 * Changes to monitor flagpage as well.
 *
 * ****************************************************************
 *
 * SCCS history end
 */


#include "lp.h"

/* Static variables for progress monitoring */
static int mon_stall;			/* Printer believed stalled */
static struct timeval mon_time;		/* Time of last movement */
static int mon_int;			/* Interval between peeks (minutes) */
static int mon_fd;			/* Input file descriptor */
static int mon_pos;			/* Saved input file pointer */
static char *mon_type;			/* String for status message */

static char runmsg[] = "%s is ready and printing via %s";

/* Declaration of local functions */
static void fp_monitor(), start_file_mon(), monitor(), stalled();
void stop_mon();

/* Functions for monitoring the flag page (and other Postscript modules). */

void start_mon(type)
int type;
	{
	/* What type of printer is this?  (get descriptive word) */
	mon_type = CT_choices[type];
	mon_int = MON_FP;
	mon_stall = 0;
        gettimeofday(&mon_time, 0);
        signal(SIGALRM, fp_monitor);
        alarm(60 * mon_int);
}

static void fp_monitor() {
	stalled();
	alarm(mon_int * 60);
}

void stop_mon() {
	if (mon_int) {				/* Monitor active */
		mon_int = 0;
		alarm(0);
		signal(SIGALRM, SIG_DFL);
		if (mon_fd >= 0) {
			close(mon_fd);
			mon_fd = -1;
		}
	}
}

/* The following functions are for monitoring the printing of the file. */

/* Exported function: this sets up the filter_chain structure to call
 * monitor control functions at the right time,
 */

void mon_setup(fcp)
FCP fcp;
	{
	fcp->fc_mon_start = start_file_mon;		/* Enable monitoring */
	fcp->fc_mon_stop  = stop_mon;
}


static void start_file_mon(fcp)
FCP fcp;
	{
	alarm(0);				/* Stop flag-page monitor */
	if (mon_stall) {
		mon_stall = 0;
	        status(runmsg, printer, mon_type);
	}
	if ((mon_fd = dup(fcp->fc_fds[FC_STDIN])) < 0) {
		dlog(0, "start_mon: dup failed (%d)", errno);
		mon_int = 0;
		return;
	}
	if ((mon_pos = lseek(mon_fd, 0, L_INCR)) != 0) {
		/* Not a regular file or something really wierd - forget it */

		dlog(0, "start_mon: lseek returned %d (%d)", mon_pos, errno);
		mon_int = 0;			/* Indicate not active */
		close(mon_fd);
		mon_fd = -1;
		return;
	}

	mon_int = MON_INIT;
	mon_stall = 0;
	gettimeofday(&mon_time, 0);
	signal(SIGALRM, monitor);
	alarm(60 * mon_int);
}

static void monitor() {
	int pos;

	if ((pos = lseek(mon_fd, 0, L_INCR)) <= mon_pos) {
		if (pos < mon_pos) {
			if (pos < 0)
				log("Progress monitor lseek failed: %d",errno);
			stop_mon();
			status("%s is active (progress not monitored)",
			       printer);
			return;
		} else					/* No progress */
			stalled();
	} else {					/* Healthy */
		mon_pos = pos;
		mon_int = MON_RUN;
		if (mon_stall) {
			mon_stall = 0;
		        status(runmsg, printer, mon_type);
		}
		gettimeofday(&mon_time, 0);
	}
	alarm(mon_int * 60);
}

/* Printing is stalled - update status */

static void stalled() {
	struct timeval now;
	int time;
	char tbuf[20];

	gettimeofday(&now, 0);
	time = (now.tv_sec - mon_time.tv_sec)/60;
	if (time) {		/* Ignore spurious alarms */
		++mon_stall;
		mon_int = time/5 + MON_RUN;
		if (time > 120)		/* Two hours */
			if (time > 48*60)	/* Two days */
				sprintf(tbuf, "%d days", time/(60*24));
			else
				sprintf(tbuf, "%d hours", time/60);
		else
			sprintf(tbuf, "%d minutes", time);
		status("%s is stalled (no progress for %s)", printer, tbuf);
	}
}
