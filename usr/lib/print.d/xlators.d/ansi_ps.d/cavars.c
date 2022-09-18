#ifndef lint
static char SccsId[] = "  @(#)cavars.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cavars.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *      1988, 1989.  ALL RIGHTS RESERVED.
 *
 *	THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE
 *	USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF
 *	SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE
 *	COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES
 *	THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE
 *	AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND
 *	OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.
 *
 *	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE
 *	WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A
 *	COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.
 *
 *	DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR
 *	RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT
 *	SUPPLIED BY DIGITAL.
 *
 */




/*
 *-----------------------------------------------------------
 *
 *   begin edit_history
 *
 *   22-JAN-1988 16:21 mhs
 *      Original version
 * 
 *   8-FEB-1988 16:53  mhw
 *      add camode
 *
 *  22-MAY-1988 14:06 araj
 *	remove definition of pointers to constants, now that Bernard 
 *	generates them
 *
 *   4-AUG-1988 14:10 ejs
 *  Added the 'hc' files to provide the prototypes for the cp/ca funtions
 *
 *  14-APR-1989 21:23	araj
 *  Added xlc_iface.vc
 *
 *   end edit_history
 *
 *-----------------------------------------------------------
 */



/*
 *-----------------------------------------------------------
 *
 *   begin description
 *
 *   Filename:  cavars.c
 *
 *   This file contains all the variable declarations for 
 *   the common action routines, so that all variables can be 
 *   placed in one area of RAM.  It is a series of includes on 
 *   the .vc files produced by running the makehf tool on .def
 *   files.
 *  
 *   end description
 *
 *-----------------------------------------------------------
 */





/*  begin include_file    */

#include    "portab.h"	    /* For portable definitions of types, etc. */
#include    "cpsys.hc"	/* For typedef of AL */
#include    "cpparse.hc"    /* For function prototypes */
#include    "cpexit.hc"    /* For function prototypes */
#include    "cpbuf_mgt.hc"    /* For function prototypes */
#include    "cpsixel.hc"    /* For function prototypes */
#include    "caglobal.hc"
#include    "caglobal.vc"	    /* variable for capdl.c */
#include    "xlc_iface.hc"
#include    "xlc_iface.vc"	    /* variable for xlc_iface.c */
/*  end   include_file    */


