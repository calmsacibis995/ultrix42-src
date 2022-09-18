/* @(#)dcl.h	4.2      ULTRIX 	10/16/90 */

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
 * dcl.h -- data structure to describe PostScript job
 * 
 * Two main data structures are:-
 *
 * struct dcl_job -- describes whole job and hides
 *	the vector of functions used for building it
 *
 * struct dcl_module -- used for creating a linked list
 *	of job modules.
 */

/*
 * Modification History:
 *
 * 02-Oct-90 - Adrian Thoms (thoms@wessex)
 *	Added flags field to dcl_job structure
 */

/* SCCS history beginning
 * This is now OBSOLETE, add modification history above
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  02/06/88 -- thoms
 * date and time created 88/06/02 17:45:10 by thoms
 * 
 * ***************************************************************
 *
 * 1.2  15/07/88 -- thoms
 * Added copyright notice, modification history, improved comments
 *
 * ***************************************************************
 *
 * 1.3  28/07/88 -- thoms
 * Added dj_resources field to dcl_job structure to store LPS
 * preloaded resources
 *
 * SCCS history end
 */


/*
 * The vector of arguments to a job module is grown by
 * this much as necessary
 */
#define DJ_CLICK	16


/****************************************************************
*	enum job_element_e -- constants to identify job modules
****************************************************************/
/*
 * Each job module has an associated execution function
 * which is accessed via the table dj_vec in dcl.c which isindexed
 * by constants of this enumeration type
 */
enum job_element_e {
	je_default,		/* nop value */
	je_archive,		/* used by PostScript jobs */
	je_print,		/* xlate and print file */
	je_bypass,		/* xlate and print, bypass output filter */
	je_outstr,		/* output strings */
	je_banner,		/* used by lp jobs only */
};


/****************************************************************
*		handlers for job modules -- in dm_handler.c
****************************************************************/

/*
 * These functions perform the actions required to send a
 * particular job module to the printer
 *
 * They are implemented in dm_handler.c and
 * are accessed indirectly via dj_vec (see above)
 */
extern int dm_default(/* DJP djp; DMP dmp */);	/* does nothing */
extern int dm_archive(/* DJP djp; DMP dmp */);	/* extract archive */
extern int dm_print(/* DJP djp; DMP dmp */);	/* xlate and print */
extern int dm_bypass(/* DJP djp; DMP dmp */);	/* xlate and print (no OF) */
extern int dm_outstr(/* DJP djp; DMP dmp */);	/* output strings */
extern int dm_banner(/* DJP djp; DMP dmp */);	/* output ascii banner */

extern int dm_debug(/* DJP djp; DMP dmp */);	/* describe the module */

/****************************************************************/

typedef enum job_element_e job_el_t; /* job type, index into handler fns vec */
typedef int (*job_element_pfn)(/* DJP djp; DMP dmp */);

/*
 * typedef job_build_sw -- type for table of job building functions
 *
 *	These 3 major job building activities may be different
 *	for different types of printer hence are called indirectly
 */
typedef struct job_build_sw_s {
	void (*jb_prolog)(/* DJP djp */);
	void (*jb_epilog)(/* DJP djp */);
	enum job_status_e (*jb_add_file)(/* DJP djp;
					  * int format;
					  * char *file */);
} job_build_sw;

/*
 * Type of the method object in a job_module
 * used to hold pointer to filter description structure
 */
typedef char *opaque_method;	/* type of object used by element handler */

/*
 * Shorthand names for job and job module pointers
 */
typedef struct dcl_module *DMP;
typedef struct dcl_job *DJP;

/****************************************************************
*	dcl_module -- Structure for job module object
****************************************************************/

struct dcl_module {
	DMP dm_next;		/* linked list pointer */
	char **dm_argv;		/* Module description vector */
	job_el_t dm_el;		/* job element type */
	opaque_method dm_method;/* arg to job element handler */
};

/****************************************************************
*	dcl_job -- Structure for job description object
****************************************************************/

struct dcl_job {
	job_build_sw *dj_build; /* build functions */
	CXP dj_cxp;		/* output connection */
	FCP dj_resources;	/* preloaded resources in LPS */
	unsigned dj_flags;	/* Miscellaneous flags */
	unsigned short dj_i;	/* index into args of current module */
	unsigned short dj_maxargs; /* max args for current module */
	struct dcl_module *dj_mod; /* Module descriptions head */
	struct dcl_module *dj_mod_tail; /* " descriptions tail */
};

/* Flag definitions for dj_flags field */
#define DJ_HAVE_SETNUMBERUP	(1<<0) /* Have loaded number up module */
#define DJ_AFTER_FIRST_FILE	(1<<1) /* Have put first file in job */
#define DJ_WIDTH_WINS		(1<<2) /* old sizes beat orientation */


/****************************************************************
*	dcl_job operations -- export declarations
****************************************************************/

extern void dj_init(/* DJP djp; CXP cxp; job_build_sw *build_sw; */);	
extern DJP new_dj(/* CXP cxp; job_build_sw *dj_build_sw */);
extern void dj_delete(/* DJP djp; int onheap */);

/*
 * functions to add new args to current filter
 */
extern void dj_add_arg(/* DJP djp; char *arg */);
extern void dj_add_args_l(/* DJP djp; char *arg; ... */);
extern void dj_add_args_v(/* DJP djp; char **argv */);
extern void dj_add_args_va(/* DJP djp; va_list ap */);

/*
 * Bracketing functions for setting up module description
 */
extern void dj_start_module(/* DJP djp;
			     * enum job_el_e el;
			     * opaque_method method; */);
extern void dj_end_module(/* DJP djp */);

/*
 * high level job module operations
 *
 * These make indirect calls via table stored in dj_build field
 */
extern void dj_prolog(/* DJP djp */); /* add job prolog to job description */
extern void dj_epilog(/* DJP djp */); /* add job epilog to job description */
extern enum job_status_e dj_add_file(/* DJP djp, int format, char *file */);

/*
 * The "do it" call: sends job to printer
 */
extern enum job_status_e dj_exec(/* DJP djp */); /* execute the job */


/****************************************************************
*         ar module extraction routines -- in dcl_xar.c
****************************************************************/

extern int ar_init(/* char *arname */);	/* 0 for success called from init()*/
extern int ar_x(/* int ar_fd, out_fd; char * module */);
	/* returns 1 for success else 0 */
extern int ar_get(/* char *arnam */);	/* return archive file desc. */
