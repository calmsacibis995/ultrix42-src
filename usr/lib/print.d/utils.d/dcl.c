#ifndef lint
static char *sccsid = "@(#)dcl.c	4.2      ULTRIX 	10/16/90";
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
 * dcl.c -- code to describe PostScript job
 * 
 * Description:
 *	This code implements a set of calls on a complex
 *	data structure which describes any print job completely
 *
 *	The data structure consists of a struct dcl_job
 *	together with a list of struct dcl_modules
 *	Each module represents one of the following job
 *	items:
 *
 *	dm_archive:	Device Control Module with arguments
 *	dm_print:	Translate and print via output filter
 *	dm_bypass:	Translate and print bypassing output filter
 *	dm_outstr:	List of strings to be output literally
 *	dm_banner:	banner page to be printed via output filter
 *
 *	The job module data consists of:
 *		A list of string arguments
 *		An opaque pointer to a method structure.
 *
 *	The method is only used by dm_print and dm_bypass
 *	modules where it points to a filter object which
 *	describes the filter program to be used
 *
 *	Three major job building activities are table driven
 *	according to printer type.
 *	The table is built in printjob.c and supplied upon
 *	initialisation of the dcl_job object by dj_init.
 */

/*
 * Modification History:
 *
 * 04-Oct-90 - Adrian Thoms (thoms@wessex)
 *	Initialise dcl_job structure all unused fields to 0
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  30/03/88 -- thoms
 * date and time created 88/03/30 17:59:35 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  25/04/88 -- thoms
 * Knows about connection structure, rationalised job module types.
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
 * 1.4  06/05/88 -- thoms
 * Altered to use linked list of modules instead of array.
 * 
 * 
 * ***************************************************************
 *
 * 1.5  12/07/88 -- thoms
 * Added copyright notice and improved commenting
 *
 * ***************************************************************
 *
 * 1.6  28/07/88 -- thoms
 * Added code to initialise and delete dj_resources field in
 * dcl_job structure.
 *
 * SCCS history end
 */


#include "lp.h"

/* external declarations */
extern char *malloc();

/* Private utility functions */
static void dj_mod_init(/* DJP djp; DMP dmp */);
static void dj_grow(/* DJP djp; DMP dmp */);


/**************** Handler functions for job execution ****************/

/* These functions handle particular job elements: */
static job_element_pfn dj_vec[] = {
    dm_default, dm_archive, dm_print, dm_bypass, dm_outstr, dm_banner,
};

/* These are used if debug level is greater than 9 */
static job_element_pfn dj_debug[] = {
	dm_debug, dm_debug, dm_debug, dm_debug, dm_debug, dm_debug,
};


/****************************************************************/

/* Utility functions */
/*
 * dj_mod_init() -- does job related housekeeping for module
 */
static void
dj_mod_init(djp, dmp)
register DJP djp;
register DMP dmp;
{
	/* initialise job module */
	dmp->dm_next = NULL;
	dmp->dm_argv = NULL;
	dmp->dm_el = je_default;
	dmp->dm_method = 0;

	/* link it in to job structure */
	if (djp->dj_mod == NULL) {
		djp->dj_mod = dmp;
	} else {
		djp->dj_mod_tail->dm_next = dmp;
	}
	djp->dj_mod_tail = dmp;

	/* initialise counters for current job module */
	djp->dj_i = 0;
	djp->dj_maxargs = 0;
	dj_grow(djp, dmp);
}

/*
 * dj_grow(djp, dmp) -- make room for more arguments in job module
 */
static void
dj_grow(djp, dmp)
register DJP djp;
register DMP dmp;
{
	register char **p, **plim;
	register unsigned newsiz = (djp->dj_i + DJ_CLICK);
	newsiz /= DJ_CLICK;
	newsiz *= DJ_CLICK;

	dmp->dm_argv = ((dmp->dm_argv)
			? (char **)realloc((char *)dmp->dm_argv,
					   newsiz*sizeof(char *))
			: (char **)malloc(newsiz*sizeof(char *)));
	assert((char *)dmp->dm_argv != NULL);
	plim = &dmp->dm_argv[newsiz];
	for (p = &dmp->dm_argv[djp->dj_maxargs]; p < plim; p++) {
		*p = 0;
	}
	djp->dj_maxargs = newsiz;
}


/****************************************************************
* functions to add new args to current module description
****************************************************************/

/*
 * dj_add_arg -- add one more argument to job module
 */
void
dj_add_arg(djp, arg)
register DJP djp;
char *arg;
{
	register DMP dmp = djp->dj_mod_tail;

	if (djp->dj_i >= djp->dj_maxargs) {
		dj_grow(djp, dmp);
	}
	dmp->dm_argv[djp->dj_i++] = strsave(arg);
}

/*
 * dj_add_args_l -- add NULL terminated list of arguments to job module
 */
/*VARARGS1*/
void
dj_add_args_l(djp, va_alist)
register DJP djp;
va_dcl
{
	va_list ap;
	register char *str;
	register DMP dmp = djp->dj_mod_tail;

	va_start(ap);
	while (str = va_arg(ap, char *)) {
		if (djp->dj_i >= djp->dj_maxargs) {
			dj_grow(djp, dmp);
		}
		dmp->dm_argv[djp->dj_i++] = strsave(str);
	}
	va_end(ap);
}

/*
 * dj_add_args_v -- add vector of arguments to job module
 */
void
dj_add_args_v(djp, argv)
register DJP djp;
char **argv;
{
	register DMP dmp = djp->dj_mod_tail;

	while (*argv) {
		if (djp->dj_i >= djp->dj_maxargs) {
			dj_grow(djp, dmp);
		}
		dmp->dm_argv[djp->dj_i++] = strsave(*argv++);
	}
}

/*
 * dj_add_args_va -- add va_list of arguments to job module
 */
void
dj_add_args_va(djp, ap)
register DJP djp;
va_list ap;
{
	register char *str;
	register DMP dmp = djp->dj_mod_tail;

	while (str = va_arg(ap, char *)) {
		if (djp->dj_i >= djp->dj_maxargs) {
			dj_grow(djp, dmp);
		}
		dmp->dm_argv[djp->dj_i++] = strsave(str);
	}
}

/****************************************************************/

/*
 * dj_start_module() -- does job related housekeeping for module
 *	installs module in job and saves module related stuff
 */
void
dj_start_module(djp, el, method)
register DJP djp;
job_el_t el;
opaque_method method;
{
	register DMP dmp = (DMP)malloc(sizeof(*dmp));

 	dj_mod_init(djp, dmp);
	dmp->dm_el = el;
	dmp->dm_method = method;
}

/*
 * dj_end_module -- terminate current module description
 */
void
dj_end_module(djp)
register DJP djp;
{
	register DMP dmp = djp->dj_mod_tail;

	if (djp->dj_i >= djp->dj_maxargs) {
		dj_grow(djp, dmp);
	}
	dmp->dm_argv[djp->dj_i] = 0;
}

/****************************************************************
* dcl_job initialisation and execution routines
****************************************************************/

/*
 * dj_init -- initialise dcl_job structure
 */
void
dj_init(djp, cxp, build_sw)
register DJP djp;
CXP cxp;
job_build_sw *build_sw;
{
	memset(djp, '\0', sizeof(*djp));
	djp->dj_cxp = cxp;
	djp->dj_build = build_sw;
}

/*
 * new_dj -- malloc a dcl_job structure and initialise it
 */
DJP
new_dj(cxp, build_sw)
CXP cxp;	
job_build_sw *build_sw;
{
	register DJP new_djp;
	new_djp = (DJP)malloc(sizeof(*new_djp));
	dj_init(new_djp, cxp, build_sw);
	return new_djp;
}

/*
 * dj_delete -- de-initialise dcl_job structure, free if malloced
 */
void
dj_delete(djp, onheap)
DJP djp;
int onheap;		/* flag: nonzero === was malloced */
{
	register int i;
	register DMP dmp, old_dmp;
	register char **p, **pv;

	for (dmp = djp->dj_mod; dmp;
	     old_dmp = dmp, dmp = old_dmp->dm_next, free((char *)old_dmp)) {
		if (!(pv = dmp->dm_argv)) continue;
		for (p = pv; *p; p++) free(*p);
		free((char *)pv);
	}
	if (djp->dj_resources) fc_delete(djp->dj_resources, 1);
	if (onheap) free((char *)djp);
}

/****************************************************************
* Generic job building functions
*	see printjob.c for actual implementations
*	for each printer_type
****************************************************************/

/*
 * dj_prolog -- add job prolog to job description
 */
void
dj_prolog(djp)
register DJP djp;
{
	(*djp->dj_build->jb_prolog)(djp);
}

/*
 * dj_epilog -- add job epilog to job description
 */
void
dj_epilog(djp)
register DJP djp;
{
	(*djp->dj_build->jb_epilog)(djp);
}

/*
 * dj_add_file -- add a file job description
 */
enum job_status_e
dj_add_file(djp, format, file)
register DJP djp;
int format;
char *file;
{
	return (*djp->dj_build->jb_add_file)(djp, format, file);
}

/*
 * dj_exec -- Execute the job by calling handlers for each module
 */
enum
job_status_e dj_exec(djp)
DJP djp;
{
	register DMP dmp;
	register int err_code=0;
	register job_element_pfn *module_exec_fns;

	/*
	 * If the debug level is greater than 9 use
	 * a set of dummy execution functions which
	 * print description of what would be done to
	 * the log file instead of actually doing it.
	 */
	module_exec_fns = (DB > 9) ? dj_debug : dj_vec;

	/*
	 * Execute the module handler function for each module
	 * Give up on finding the any error
	 */
	for (dmp=djp->dj_mod; dmp; dmp = dmp->dm_next) {
		if (err_code = (*module_exec_fns[(int)dmp->dm_el])(djp, dmp)) {
			break;
		}
	}

	return ((err_code == 0) ? js_ok :
			      (err_code > 0) ? js_retry : js_failed);
}
