#ifndef lint
static char *sccsid = "@(#)connection.c	4.1      ULTRIX 7/2/90";
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
 * connection.c -- Connection object implementation
 *
 * Description:
 *	This code implements all the necessary operations on
 *	the output connection from lpd.
 *	Most of the code is a restructured version of the original
 *	Berkeley code, so if it wasn't broke I tried not to fix it
 *
 *	The output connection object supports 4 calls:
 *	open close start and stop.
 *	The appropriate set of 4 calls is selected via a table
 *	according to connection type which may be explicitly
 *	set in printcap or guessed at according to the other
 *	parameters supplied. (See set_connection_type in printjob.c)
 *
 *	The open_output_filter would live here but for the
 *	fact that it needs static data to supply the filter
 *	arguments so it has to live in printjob.c
 */
/*
 * Modification History
 *
 * 27-jul-89 -- thoms
 *      Fixed use of sprintf
 *
 * 10-may-89 -- thoms
 *      Initialised from PrintServer Client pool
 *      SCCS history below refers to OLD sid's
 *
 * 12-Dec-89 -- thomas
 *	Cleanup lat support a bit.
 */
/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  25/04/88 -- thoms
 * date and time created 88/04/25 18:56:29 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  29/04/88 -- thoms
 * nor_open exits on open fail, EXCEPT on EWOULDBLOCK
 * 
 * 
 * ***************************************************************
 * 
 * 1.3  03/05/88 -- thoms
 * Fixed and stable, minimal PS functionality.
 * 
 * 
 * ***************************************************************
 *
 * 1.4  11/07/88 -- thoms
 * Added copyright notice, modification history, improved comments
 *
 * ***************************************************************
 *
 * 1.5  21/07/88 -- thoms
 * cx_open and cx_close routines now return status.
 * cx_close return value is meaningful and reflects filter exit status,
 * cx_open return value is a place holder for now.
 *
 * ***************************************************************
 * 1.6  01/09/88 -- thoms
 * Added sane_start routine which send SIGCONT to output filters
 * Added con_berkdev and con_berklat to use sane_start
 *
 * ***************************************************************
 *
 * 1.7  07/09/88 -- thoms
 * Made Berkeley style output filters the default
 *
 * ****************************************************************
 *
 * 1.8  16/10/88 -- thoms
 * Added TCP Printserver support
 * Removed old-style filter restart facility. (Code still here for emergencies)
 *
 *
 * ****************************************************************
 *
 * 1.9  21/10/88 -- thoms
 * Tidied up debug log and status code
 *
 * SCCS history end
 */

#include "lp.h"

/*
 * calls to external functions
 */
extern status(/* char *msg; va_alist arglist */);/* print_utils.c */
extern int noresponse(/* int fd */);		/* sendjob.c */
extern void open_output_filter(/* CXP cxp */);	/* printjob.c */

extern int lat_conn();		/* lat_conn.c */
extern int tcp_conn();		/* tcp_conn.c */

/*
 * external data references
 */
extern int sys_nerr;		/* libc.a */
extern char *sys_errlist[];	/* libc.a */

/*
 * static utility functions
 */

static int close_output_filter(/* CXP cxp */);
static char *err_string();
static set_tty_modes(/* int pfd */);

/*
 * actions for each connection type
 */

static int nor_open(/* CXP cxp */);
static int nor_close(/* CXP cxp */);
static int nor_stop(/* CXP cxp */);
static int nor_start(/* CXP cxp */);
static int sane_start(/* CXP cxp */);

static int lat_open(/* CXP cxp */);

static int rem_open(/* CXP cxp */);

static int net_open(/* CXP cxp */);
static int net_close(/* CXP cxp */);

static int tcp_open(/* CXP cxp */);

static int cx_nop(/* CXP cxp */);

/*
 * Function call table for the 4 operations on the
 * connection object for each of the four connection types.
 *
 * Note that start and stop are no-ops for network and
 * Berkeley remote connections
 */

/*      open      close      stop      start      connection_type value */
static struct cx_fns cx_sw_tab[] = {
	nor_open, nor_close, nor_stop, sane_start,/* con_dev */
	lat_open, nor_close, nor_stop, sane_start,/* con_lat */
	rem_open, nor_close, cx_nop, cx_nop,	  /* con_remote */
	net_open, net_close, cx_nop, cx_nop,	  /* con_network */
	tcp_open, nor_close, nor_stop, sane_start,/* con_tcp */
};
/****************************************************************/

/*
 * err_string() -- generate the descriptive string for E-number error
 *		remember: E numbers harm your health :-)
 */
static char *
err_string()
{
	static char unknown[20];
	return ((errno < sys_nerr)
		? sys_errlist[errno]
		: sprintf(unknown, "unknown error[%d]", errno), unknown);
}

/****************************************************************/
/*
 * stuff to setup tty lines
 */

static struct bauds {
	int	baud;
	int	speed;
} bauds[] = {
	50,	B50,
	75,	B75,
	110,	B110,
	134,	B134,
	150,	B150,
	200,	B200,
	300,	B300,
	600,	B600,
	1200,	B1200,
	1800,	B1800,
	2400,	B2400,
	4800,	B4800,
	9600,	B9600,
	19200,	EXTA,
	38400,	EXTB,
	0,	0
};


/****************************************************************/

/*
 * set_tty_modes() -- do the relevant ioctls on a tty line
 */
static
set_tty_modes(pfd)
int pfd;
{
	struct sgttyb ttybuf;
	register struct bauds *bp;

	if (ioctl(pfd, TIOCGETP, (char *)&ttybuf) < 0) {
		log("cannot get tty parameters");
		exit(1);
	}
	if (BR > 0) {
		for (bp = bauds; bp->baud; bp++)
		    if (BR == bp->baud)
			break;
		if (!bp->baud) {
			log("illegal baud rate %d", BR);
			exit(1);
		}
		ttybuf.sg_ispeed = ttybuf.sg_ospeed = bp->speed;
	}
	ttybuf.sg_flags &= ~FC;
	ttybuf.sg_flags |= FS;
	if (ioctl(pfd, TIOCSETP, (char *)&ttybuf) < 0) {
		log("cannot set tty parameters");
		exit(1);
	}
	if (XC) {
		if (ioctl(pfd, TIOCLBIC, &XC) < 0) {
			log("cannot set local tty parameters");
			exit(1);
		}
	}
	if (XS) {
		if (ioctl(pfd, TIOCLBIS, &XS) < 0) {
			log("cannot set local tty parameters");
			exit(1);
		}
	}
}

/*
 * close_output_filter(cxp) -- close the output filter, if present
 *
 *	The output filter is accessed via the connection object ptr cxp.
 * The filter shutdown is actually performed by code in filter.c
 */

static int
close_output_filter(cxp)
CXP cxp;
{
	union wait filter_status;

	if (cxp->cx_output_filter == NULL) {
		return 0;
	}
	fc_pout_kill(cxp->cx_output_filter);
	filter_status = fc_wait(cxp->cx_output_filter);
	fc_delete(cxp->cx_output_filter, 1);
	cxp->cx_output_filter = 0;

	if (!WIFEXITED(filter_status) || filter_status.w_retcode > 1) {
		log("Output filter to %s malfunctioned (%d)",
		    CT_choices[(int)cxp->cx_type],
		    filter_status.w_retcode);
		return(-1);
	} else if (filter_status.w_retcode == 1) {
		return(1);
	} else
	    return 0;
}

/*
 * nor_open() -- perform open on a special device
 *
 * Note that tty ioctls are only performed on tty lines
 */
static int
nor_open(cxp)
CXP cxp;
{
	int i;
	if (!*LP) {
		log("%s connection requires non-null lp in printcap",
		    CT_choices[(int)cxp->cx_type]);
		exit(1);
	}
	/* This loop doubles the delay each time round
	 * up to a maximum of 32 seconds
	 */
	for (i = 1; ; i = i < 32 ? i << 1 : i) {
		cxp->cx_pr_fd =
		    open(LP, ((RW ? O_RDWR : O_WRONLY)
			      | O_CREAT | O_NDELAY), 0644);
		if (cxp->cx_pr_fd >= 0)
		    break;
		switch (errno) {
		    default:
			log("cannot open %s (%s)", LP, err_string());
			exit(1);
		    case ENXIO:
			log("No hardware present for %s", LP);
			exit(1);
		    case EWOULDBLOCK:
			break;
		}
		if (i == 1) {
			status("waiting for %s to become ready (offline ?)",
			       printer);
		}
		sleep(i);
	}
	if ((i = flock(cxp->cx_pr_fd, LOCK_EX | LOCK_NB)) < 0) {
 		if (errno == EWOULDBLOCK) {
 			status("waiting for lock on %s", LP);
 			i = flock(cxp->cx_pr_fd, LOCK_EX);
 		}
 		if (i < 0) {
 			log("broken lock on %s", LP);
 			exit(1);
 		}
 	}
	if (isatty(cxp->cx_pr_fd)) {
		set_tty_modes(cxp->cx_pr_fd);
	}
	open_output_filter(cxp);
	return 0;
}

/*
 * nor_close(cxp) -- close a device or lat connection
 */

static int
nor_close(cxp)
register CXP cxp;
{
	register int retval;

	retval = close_output_filter(cxp);
	(void) close(cxp->cx_pr_fd);	/* close printer */
	cxp->cx_pr_fd = cxp->cx_out_fd = -1;

	return retval;
}

/*
 * nor_stop(cxp) -- if there is an output filter stop it
 *
 * This involves sending it a C-Y C-A pair of control
 * characters which it is supposed to recognise a send
 * itself a stop signal
 * We wait for it to stop, so lets hope it does!
 */
static int
nor_stop(cxp)
register CXP cxp;
{
	if (cxp->cx_output_filter) {	/* stop output filter */
		if (fc_stop(cxp->cx_output_filter) < 0) {
			union wait o_status;
			o_status = fc_wait(cxp->cx_output_filter);
			log("output filter died (%d)",
			    o_status.w_retcode);

			fc_delete(cxp->cx_output_filter, 1);
			cxp->cx_output_filter = 0;
			return(1);
		}
	}
	return 0;
}

/*
 * nor_start(cxp) -- if we're stopped, restart the output filter
 *
 * This is the old algorithm which kills the old filter
 * and starts a new invocation.
 * It seems a monstrously inefficient way of doing it.
 * The whole mechanism should be swept away in my view (thoms)
 */
static int
nor_start(cxp)
register CXP cxp;
{
	if (cxp->cx_output_filter
	    && cxp->cx_output_filter->fc_state == fc_stopped) {
		cx_close(cxp);	/* close output and kill filter */

		cx_open(cxp);	/* open output and restart filter */
	}
	return 0;		/* always succeeds for now */
}

/*
 * sane_start(cxp) -- if we're stopped, restart the output filter
 *
 * This should restart the filter in the proper
 * Berkeley fashion as requested by Brian Reid in his QAR
 */
static int
sane_start(cxp)
register CXP cxp;
{
	if (cxp->cx_output_filter)
	    return fc_start(cxp->cx_output_filter);

	return 0;
}

/*
 * lat_open(cxp) -- open a lat connection
 */
static int
lat_open(cxp)
register CXP cxp;
{
	if (!(*LP && TS && (OP || OS))) {
		log("%s %s\n%s", CT_choices[(int)cxp->cx_type],
		    "connection requires TS and either OP or OS",
		    "and non-null LP in printcap");
		exit(1);
	}
	status("waiting for %s to connect", printer);
	cxp->cx_pr_fd = lat_conn(LP,TS,OP,OS);
		
	if (cxp->cx_pr_fd < 0)
	    {
		    log("cannot open %s: %s",LP,strerror(errno));
		    exit(1);
	    }
	set_tty_modes(cxp->cx_pr_fd);

	open_output_filter(cxp);

	status("%s is ready and printing via %s",
	       printer, CT_choices[(int)cxp->cx_type]);

	return 0;
}

/*
 * rem_open(cxp) -- open socket connection to remote Berkeley lpd
 */
static int
rem_open(cxp)
register CXP cxp;
{
	int i, n;
	if (!(RM && RP)) {
		log("%s connection requires RM and RP in printcap",
		    CT_choices[(int)cxp->cx_type]);
		exit(1);
	}
	for (i = 1; ; i = i < 32 ? i << 1 : i) {
		cxp->cx_pr_fd = getport(RM);
		if (cxp->cx_pr_fd >= 0) {
			(void) sprintf(line, "\2%s\n", RP);
			n = strlen(line);
			if (write(cxp->cx_pr_fd, line, n) != n)
			    break;
			if (noresponse(cxp->cx_pr_fd)) {
				if (flock(cxp->cx_pr_fd, LOCK_UN) < 0) {
					log("cannot unlock %s", LP);
					exit(1);
				}
				(void) close(cxp->cx_pr_fd);
			}
			else
			    break;
		} else {
			log("getport(%s) failed: %s", RM, err_string());
		}
		if (i == 1)
		    status("waiting for %s to come up", RM);
		sleep(i);
	}
	status("sending to %s", RM);

	return 0;
}

/*
 * rem_close(cxp) -- close socket connection
 */
static int
rem_close(cxp)
register CXP cxp;
{
	register int retval;

	retval = close(cxp->cx_pr_fd);	/* close socket connection */
	cxp->cx_pr_fd = -1;

	return retval;
}

/*
 * net_open(cxp) -- open up network filter
 *
 * This is just opening the output filter but without opening
 * up a device
 */
static int
net_open(cxp)
register CXP cxp;
{
	if (!(OF && *OF)) {
		log("%s connection requires OF in printcap",
		    CT_choices[(int)cxp->cx_type]);
		exit(1);
	}
	open_output_filter(cxp);
	cxp->cx_pr_fd = cxp->cx_out_fd;

	status("%s is ready and printing via %s",
	       printer, CT_choices[(int)cxp->cx_type]);

	return 0;
}

/*
 * net_close(cxp) -- close network filter
 */
static int
net_close(cxp)
register CXP cxp;
{
	register int retval;
	retval = close_output_filter(cxp);
	cxp->cx_pr_fd = cxp->cx_out_fd = -1;

	return retval;
}

/*
 * tcp_open(cxp) -- open tcp connection
 */
static int
tcp_open(cxp)
register CXP cxp;
{
	if (LP[0] != '@') {
		log("%s connection requires lp in printcap starts with @",
		    CT_choices[(int)cxp->cx_type]);
		exit(1);
	}
	if ((cxp->cx_pr_fd = tcp_conn(&LP[1])) < 0) {
		log("Cannot open tcp connection %s", LP);
		exit(1);
	}
	open_output_filter(cxp);
	return 0;
}

/*
 * cx_nop(cxp) -- no-op function for insertion in switch table
 */
/*ARGSUSED*/
static int
cx_nop(cxp)
register CXP cxp;
{
	return 0;
}

/****************************************************************
*	Main entry points for connection object
****************************************************************/

/*
 * cx_init -- initialise the object structure
 */
void
cx_init(cxp, connection_type)
register CXP cxp;
enum connection_type_e connection_type;
{
	cxp->cx_state = cxs_closed;
	cxp->cx_out_fd = cxp->cx_pr_fd = -1;
	cxp->cx_output_filter = NULL;
	cxp->cx_type = connection_type;
}

/*
 * cx_delete -- de-initialise the connection object
 */
void
cx_delete(cxp, on_heap)
CXP cxp;
int on_heap;
{
	if (cxp && on_heap) free(cxp);
}

/*
 * cx_open -- open call on connection object
 */
int
cx_open(cxp)
register CXP cxp;
{
	register int retval = 0;

	if (cxp->cx_state == cxs_closed) {
		cxp->cx_state = cxs_open;
		dlog(0, "Opening output connection for %s",
		     CT_choices[(int)cxp->cx_type]);

		retval = (*cx_sw_tab[(int)cxp->cx_type].cxf_open)(cxp);
		if (retval == 0) {
			status("%s is ready and printing via %s",
			       printer, CT_choices[(int)cxp->cx_type]);
		}
	}
	return retval;
}

/*
 * cx_close -- close call on connection object
 */
int
cx_close(cxp)
register CXP cxp;
{
	if (cxp->cx_state == cxs_open) {
		cxp->cx_state = cxs_closed;
		dlog(0, "Closing output connection for %s",
		     CT_choices[(int)cxp->cx_type]);
		return (*cx_sw_tab[(int)cxp->cx_type].cxf_close)(cxp);
	} else
	    return 0;
}

/*
 * cx_stop -- stop call on connection object
 *
 * See nor_stop above for non_null implementation
 */
int
cx_stop(cxp)
register CXP cxp;
{
	if (cxp->cx_state == cxs_open) {
		return((*cx_sw_tab[(int)cxp->cx_type].cxf_stop)(cxp));
	} else
	    return 0;
}

/*
 * cx_start -- start call on connection object
 *
 * See nor_start for non_null implementation
 */
int
cx_start(cxp)
register CXP cxp;
{
	if (cxp->cx_state == cxs_open) {
		return ((*cx_sw_tab[(int)cxp->cx_type].cxf_start)(cxp));
	} else
	    return 0;
}
