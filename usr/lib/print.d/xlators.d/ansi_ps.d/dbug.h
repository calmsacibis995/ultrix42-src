/*        @(#)dbug.h	4.1      7/2/90      */
/*
 * dbug.h - Include file enable/disable the inclusion of the Adobe Postscript
 *	    error handler and the Postscript output of VM commands
 * 
 * Created:	gh	 11-AUG-1986 13:25:18
 *
 * Edit:
 *		nv	 20-AUG-1986 10:09:47 Include analyze conditional 
 *			compilation ocntrols:
 *					WITHOUTPUT
 *					COMPLETE_ANALYSIS
 *					ERROR_REPORT 
 *		araj	 5-SEP-1986 10:20:01 
 *				added warning and abort macros
 *		nv	 7-OCT-1986 14:56:12 Added definitions for WARNING_ERR 
 *			and ABORT_ERR.
 *			Redefined WARNING_MACRO and ABORT_MACRO.
 *		mbg	 15-OCT-1986 13:25:44 
 *			Making no error handler as the default.
 *		mgb	 20-OCT-1986 10:08:58 
 *			Cosmetic change - split into two pages; flags for
 *			conditional compiling and routines for help in 
 *			debugging.
 *
 *		mgb	 6-JUL-1987 16:32:36 
 *			Added item to item list for symbiont debug mode
 *
 *		ejs  1-SEP-1988 19:03
 *			Eliminated the return statement in ABORT_MACRO.
 *
 *		araj	29-DEC-1988 15:03
 *		    change dispsoe_of_error into process_error
 *
 *		araj	28-MAR-1989 11:04
 *		    added ps_trace macro
 *
 *		araj	10-APR-1989 09:06
 *		    added ps_vm_trace macro
 */




/************************************************************************
 *									*
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,		*
 *		1986, 1987, 1988, 1989.                                 *
 *                  ALL RIGHTS RESERVED.				*
 *									*
 *	THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE		*
 *	USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF		*
 *	SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE		*
 *	COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES		*
 *	THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE		*
 *	AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND		*
 *	OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.		*
 *									*
 *	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE		*
 *	WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A		*
 *	COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.			*
 *									*
 *	DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR		*
 *	RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT		*
 *	SUPPLIED BY DIGITAL.						*
 ************************************************************************/


/************************************************************************
 *                                                                      *
 * These are part of the conditional compile of the translator          *
 *                                                                      *
 ************************************************************************/

/* Define the following symbol <> 0 to enable inclusion of the error handler */
#define ERROR_HANDLER 0

/* Define the following symbol <> 0 to enable VM command output */
#define VM_DEBUG 0

/* Define the following symbol <> 0 to enable 'DUMP2x' font analysis files */
#define WITHOUTPUT 0

/* Define the following symbol <> 0 to enable extra font analysis */
#define COMPLETE_ANALYSIS 0

/* Define the following symbol <> 0 to enable translator error reporting */
#define ERROR_REPORT 0

/* Define the following symbol <> 0 to enable translator symbiont debug mode */
#define SYMBIONT_DEBUG 0

/* Define the following symbol <> 0 to enable sixel debug state. Sixels are
   converted to 8 bits and since 0 - 31 are control char the xlator will not 
   work in stand alone mode (without the symbiont). Default is 0 for the 
   symbiont */
#define STAND_ALONE 0




/************************************************************************
 *                                                                      *
 * This page is for routine that log errors to help debug               *
 *                                                                      *
 ************************************************************************/

/* The following literals serve as indices [into 'type_of_error_condition' in 
   xlv_ps.c] for specifying the appropriate error condition enunciation 
   sting. */
#define WARNING_ERR 0
#define ABORT_ERR 1


/* Define the error reporting macro "warning".
 */
#define WARNING_MACRO(xyz) \
process_error (WARNING_ERR, (xyz) )


/* Define the error reporting macro "abort".
 */
#define ABORT_MACRO(xyz) \
process_error (ABORT_ERR, (xyz) );\


/* Define the symbiont debug reporting macro ps_trace */

#if SYMBIONT_DEBUG

#define ITEM_TRACE_MACRO(xyz,zyx) \
oprintf (xyz,zyx)

#else

#define ITEM_TRACE_MACRO(xyz,zyx) 


#endif



/* Define the symbiont debug reporting macro ps_trace */

#if VM_DEBUG

#define PS_VM_TRACE_MACRO(xyz,zyx) \
oprintf (xyz,zyx)

#else

#define PS_VM_TRACE_MACRO(xyz,zyx) 


#endif

