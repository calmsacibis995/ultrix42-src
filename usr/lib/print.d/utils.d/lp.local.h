/* @(#)lp.local.h	4.1      ULTRIX 	7/2/90 */

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
 * lp.local.h -- local parameters to the spooling system
 */

/*
 * Modification History:
 *
 * 12-jan-90 -- thoms
 *	Added constant HOSTNAME_LEN for hostname length
 *
 * 2/08/89 -- Giles Atkinson
 * Added parameters for progress monitoring
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  02/06/88 -- thoms
 * date and time created 88/06/02 17:45:41 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  02/06/88 -- thoms
 * Simplified code to allow any -D<datatype>
 * 
 * 
 * ***************************************************************
 *
 * 1.3  15/07/88 -- thoms
 * Added copyright notice and modification history
 *
 * ***************************************************************
 *
 * 1.4  21/07/88 -- thoms
 * Added MAX_RETRY_WAIT and increased PRINT_NRETRIES
 * which control the retry loop in printjob.c.
 *
 * ***************************************************************
 *
 * 1.5  29/07/88 -- thoms
 * Changed default DL to NULL
 *
 * ***************************************************************
 *
 * 1.6 16/08/88 -- maxwell
 * changed default data_type from ps to postscript
 *
 * ***************************************************************
 *
 * 1.7  01/09/88 -- thoms
 * Tweaked retry parameters
 *
 * ****************************************************************
 *
 * 1.8  10/11/88 -- thoms
 * Removed #include of a.out.h, lpr.c now includes exec.h instead
 *
 * ****************************************************************
 *
 *
 * SCCS history end
 */


/*
 * Magic number mapping for binary files, used by lpr to avoid
 * printing objects files.
 */

#include <ar.h>

#ifndef A_MAGIC1	/* must be a VM/UNIX system */
#	define A_MAGIC1	OMAGIC
#	define A_MAGIC2	NMAGIC
#	define A_MAGIC3	ZMAGIC
#	undef ARMAG
#	define ARMAG	0177545
#endif

/*
 * Defaults for line printer capabilities data base
 */

#define DEFDB		0	/* Debug logging turned off */
#define DEFDL		NULL

/* the choice strings for these capabilities reside in pcap_choices.c */
#define DEFUV		UV_choices[0] /* default*/
#define DEFCT		CT_choices[0] /* default connection type */
#define DEFPS		PS_choices[0] /* default printer type */

#define	DEFLP		"lp"
#define DEFLOCK		"lock"
#define DEFSTAT		"status"
#define DEFINIT		"init"
#define	DEFSPOOL	"/usr/spool/lpd"
#define	DEFDAEMON	"/usr/lib/lpd"
#define	DEFLOGF		"/dev/console"
#define	DEFDEVLP	"/dev/lp"
#define DEFRLPR		"/usr/lib/rlpr"
#define DEFBINDIR	"/usr/ucb"
#define	DEFMX		1000
#define DEFMAXCOPIES	0
#define DEFFF		"\f"
#define DEFWIDTH	132
#define DEFLENGTH	66
#define DEFUID		1

/*
 * Default parameters for PostScript printers
 */
#define DEF_SHEETSIZE	"a"
#define DEF_ORIENTATION	"portrait"
#define DEF_DATATYPE	"postscript"

/*
 * Default PostScript translators
 */
#define DEF_LAYUP	"layup"
#define DEF_XLATOR	"xlator_call"

/*
 * When files are created in the spooling area, they are normally
 *   readable only by their owner and the spooling group.  If you
 *   want otherwise, change this mode.
 */
#define FILMOD		0660

/*
 * Printer is assumed to support LINELEN (for block chars)
 *   and background character (blank) is a space
 */
#define LINELEN		132
#define BACKGND		' '

#define HEIGHT	9		/* height of characters */
#define WIDTH	8		/* width of characters */
#define DROP	3		/* offset to drop characters with descenders */

/*
 * path name of files created by lpd.
 */
#define MASTERLOCK "/usr/spool/lpd.lock"
#define SOCKETNAME "/dev/printer"
#define DQSNAME "/dev/dqsport"

/*
 * Some utilities used by printjob.
 */
#define PR		"/bin/pr"
#define MAIL		"/usr/lib/sendmail"

/*
 * Define PATH string for filter invocation
 *
 * To use inherited PATH replace with
 * #define PATH_STRING	0
 */
#define PATH_STRING	"PATH=/usr/local/lib/lpdfilters:/usr/ucb:/bin:/usr/bin:/usr/lib:/usr/lib/lpdfilters"


/*
 * Define TERMCAP if the terminal capabilites are to be used for lpq.
 */
#define TERMCAP

/*
 * Maximum number of user and job requests for lpq and lprm.
 */
#define MAXUSERS	50
#define MAXREQUESTS	50


/*
 * Maximum number of times to retry a failed job
 */
#define PRINT_NRETRIES	60

/*
 * Maximum and minimum time to wait between retries
 */
#define MIN_RETRY_WAIT	15
#define MAX_RETRY_WAIT	1800	/* 30 minutes */

/*
 * Parameters for time interval between examinations of data file pointer
 * during printing to determine whether progress is being made.
 */
#define MON_INIT 1		/* Just long enough for buffers to fill */
#define MON_RUN  2		/* While job is actually printing */
#define MON_FP   4		/* Time allowed for printing flag page */

/*
 * Length of buffer needed for a fully qualified host name
 */
#define HOSTNAME_LEN	287

#define CFNAME_LEN	HOSTNAME_LEN+6

#define MAX_COML	160		/* Maximum length of command line. */
