#ifndef lint
static char *sccsid = "@(#)print_utils.c	4.2      ULTRIX 	10/16/90";
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


#include "lp.h"

/*
 * print_utils.c -- init(), status(), sendmail(), lpd_time()
 */

/*
 * Modification History
 *
 * 04-Oct-90 - Adrian Thoms (thoms@wessex)
 *	Added global flag pc_orientation_found for orientation
 *	/width/length defaulting scheme 
 *
 * 18-jan-90 -- thoms
 *	Made OP default to null string, hence no longer compulsory
 *
 * 04-jan-90 -- thoms
 *	Fixed mysterious non-expansion of datatype arg in printcap
 *	Made OS default to null string, hence no longer compulsory
 */
/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  20/05/88 -- thoms
 * date and time created 88/05/20 16:31:27 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  02/06/88 -- thoms
 * Simplified code to allow any -D<datatype>
 * 
 * 
 * ***************************************************************
 *
 * 1.3  19/07/88 -- thoms
 * Added copyright notice and modification history
 * Changed names to agree with code review
 * Removed check and canonicalisation of data_type and def_data_type
 * Note that pcap_get_and_canon is no longer used
 *
 * ***************************************************************
 *
 * 1.4  29/07/88 -- thoms
 * dofork() routine was moved to filter.c where it belonged
 *
 * ***************************************************************
 *
 * 1.5  01/09/88 -- thoms
 * Fixed Da capability to expand using pcap_get_and_canon as before
 *
 * ****************************************************************
 *
 * 1.6  21/10/88 -- thoms
 * Added lpd_time() function
 *
 * SCCS history end
 */


extern char *pcap_get_and_check(/* char *cap;
				 * char *default_str;
				 * int which_set */);      /* pcap_choices.c */
extern char *pcap_get_and_canon(/* char *cap;
				 * char *default_str;
				 * int which_set */);      /* pcap_choices.c */


/****************************************************************/

/*
 * New printcap capabilities for psv1.0
 *
 * The old ones are in common.c
 *
 * These are new general capabilities:
 */
char	*UV;			/* = Ultrix version */
char	*PS;			/* = is it a PostScript printer? */
char	*DL;			/* = The device control library */
char	*CT;			/* = Connection type */


/*
 * This is an old one and is needed to avoid setting tof here
 */
short	FO;

/*
 * These capabilities set PostScript defaults
 * (comments denote printcap character id pair
 */
char	*pc_datatype;		/* Da */
char	*pc_input_tray;		/* It */
char	*pc_output_tray;	/* Ot */
char	*pc_orientation;	/* Or */
int	pc_orientation_found;	/* Or was found */
char	*pc_pagesize;		/* Ps */
char	*pc_sheetsize;		/* Ss */
char	*pc_upper_pglim;	/* Ul */
char	*pc_number_up;		/* Nu */
char	*pc_message;		/* Ml */
char	*pc_layup_file;		/* Lu */
char	*pc_sides;		/* Si */

char	*pc_sheetstd;		/* Sd: fallback sheetsize if none specified */

char	*pc_layup;		/* Lf */
char	*pc_xlator;		/* Xf */

/****************************************************************/

int init()
{
	int status;
	extern char ininame[];	/* in printjob.c */

	if ((status = pgetent(line, printer)) < 0)
		fatal("can't open printer description file");
	else if (status == 0)
		fatal("unknown printer");

	init_args();		/* Set up data for recognising argstrings */

	UV = pcap_get_and_check("uv", DEFUV, as_string);
	CT = pcap_get_and_check("ct", DEFCT, as_string);
	PS = pcap_get_and_check("ps", DEFPS, as_string);
	if ((DB = pgetnum("db")) < 0)
		DB = DEFDB;
	DL = pcap_get_and_check("Dl", DEFDL, as_string);
	LP = pcap_get_and_check("lp", DEFDEVLP, as_string);
	RP = pcap_get_and_check("rp", DEFLP, as_string);
	LO = pcap_get_and_check("lo", DEFLOCK, as_string);
	ST = pcap_get_and_check("st", DEFSTAT, as_string);
	LF = pcap_get_and_check("lf", DEFLOGF, as_string);
	SD = pcap_get_and_check("sd", DEFSPOOL, as_string);
	if ((DU = pgetnum("du")) < 0)
		DU = DEFUID;
	FF = pcap_get_and_check("ff", DEFFF, as_string);
	if ((PW = pgetnum("pw")) < 0)
		PW = DEFWIDTH;
	if ((PL = pgetnum("pl")) < 0)
		PL = DEFLENGTH;
	if ((PX = pgetnum("px")) < 0)
		PX = 0;
	if ((PY = pgetnum("py")) < 0)
		PY = 0;
	RM = pgetstr("rm", &bp);
	TS = pgetstr("ts", &bp);
	OP = pcap_get_and_check("op", "", as_string);
	OS = pcap_get_and_check("os", "", as_string);
	AF = pgetstr("af", &bp);
	OF = pgetstr("of", &bp);
	IF = pgetstr("if", &bp);
	PP = pgetstr("pp", &bp); /* substitute print filter to replace pr */
	RF = pgetstr("rf", &bp);
	TF = pgetstr("tf", &bp);
	NF = pgetstr("nf", &bp);
	DF = pgetstr("df", &bp);
	GF = pgetstr("gf", &bp);
	VF = pgetstr("vf", &bp);
	CF = pgetstr("cf", &bp);
	TR = pgetstr("tr", &bp);
	XF = pgetstr("xf", &bp);
	RS = pgetflag("rs");
	SF = pgetflag("sf");
	SH = pgetflag("sh");
	SB = pgetflag("sb");
	RW = pgetflag("rw");
	BR = pgetnum("br");
	if ((FC = pgetnum("fc")) < 0)
		FC = 0;
	if ((FS = pgetnum("fs")) < 0)
		FS = 0;
	if ((XC = pgetnum("xc")) < 0)
		XC = 0;
	if ((XS = pgetnum("xs")) < 0)
		XS = 0;
	FO = pgetflag("fo");

	pc_datatype = pcap_get_and_canon("Da", DEF_DATATYPE, as_data_types);

	pc_input_tray = pcap_get_and_check("It", 0, as_input_trays);
	pc_output_tray = pcap_get_and_check("Ot", 0, as_output_trays);
	{
		extern char *bp;
		char *old_bp = bp;

		pc_orientation =
		    pcap_get_and_check("Or", DEF_ORIENTATION, as_orientations);
		if (bp != old_bp) pc_orientation_found = 1;
	}
	pc_pagesize = pcap_get_and_check("Ps", 0, as_page_sizes);
	pc_sheetsize = pcap_get_and_check("Ss", 0, as_page_sizes);
	pc_sheetstd = pcap_get_and_check("Sd", DEF_SHEETSIZE, as_page_sizes);
	pc_upper_pglim = pcap_get_and_check("Ul", 0, as_numerical);
	pc_number_up = pcap_get_and_check("Nu", 0, as_numerical);
	pc_message = pcap_get_and_check("Ml", 0, as_messages);
	pc_layup_file = pgetstr("Lu", &bp);
	pc_sides = pcap_get_and_check("Si", 0, as_sides);

	pc_layup = pcap_get_and_check("Lf", DEF_LAYUP, as_string);
	pc_xlator = pcap_get_and_check("Xf", DEF_XLATOR, as_string);

	sprintf(ininame, "%s/%s", SD, DEFINIT);	/* setup init file name */
}


/*VARARGS1*/
status(msg, va_alist)
     char *msg;
     va_dcl
{
	register int fd;
	register FILE *status_fp;
	va_list args;

	umask(0);
	fd = open(ST, O_WRONLY|O_CREAT, 0664);
	if (fd < 0 || flock(fd, LOCK_EX) < 0)
	    fatal("cannot create status file");

	ftruncate(fd, 0);
	status_fp = fdopen(fd, "w");

	va_start(args);
	vfprintf(status_fp, msg, args);
	va_end(args);

	fprintf(status_fp, "\n");

	(void) fclose(status_fp);
}

void sendmail(user, fromhost, jobname, job_status)
     char *user, *fromhost, *jobname;
     enum job_status_e job_status;
{
	struct filter_chain mail_prog;
	register FILE *mfp;
	register int mfd;
	char buf[100];
	sprintf(buf, "%s@%s", user, fromhost);

	fc_init(&mail_prog);
	fc_add_args_l(&mail_prog, MAIL, buf, 0);
	fc_end_filter(&mail_prog);

	fc_plumb_and_run(&mail_prog, DORETURN, FC_MAKEPIPE, 2, 2);
	mfd = mail_prog.fc_fds[FC_STDIN];
	if ((mfp = fdopen(mfd, "w")) ==NULL) return;

	fprintf(mfp, "To: %s@%s\n", user, fromhost);
	fprintf(mfp, "Subject: Printer job\n\n");
	fprintf(mfp, "Your printer job ");
	if (*jobname)
	    fprintf(mfp, "(%s) ", jobname);
	fprintf(mfp, "on %s ", printer);
	switch (job_status) {
	    case js_ok:
		fprintf(mfp, "\ncompleted successfully.\n");
		break;
	    default:
	    case js_failed:
		fprintf(mfp, "\ncould not be printed.\n");
		break;
	    case js_too_many_retries:
		fprintf(mfp, "\nreached retry limit: not printed.\n");
		break;
	    case js_restricted:
		fprintf(mfp, "\n%s %s.\n",
			"could not be printed without an account on",
			host);
		break;
	}
	fclose(mfp);
	fc_pout_kill(&mail_prog);
	(void)fc_wait(&mail_prog);
	fc_delete(&mail_prog, 0);
}

char *
lpd_time()
{
	register char *time_string;
	time_t clock;
	char *ctime();

	clock = time((long *)0);
	time_string = ctime(&clock);
	/* kill off unwanted newline */
	time_string[24] = '\0';

	return time_string;
}
