#ifndef lint
static char SccsId[] = "  @(#)cpvars.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cpvars.c
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
 *   001   17-DEC-1987 17:09 mhs
 *      Initial version
 *
 *   002    8-FEB-1988 16:59 mhw
 *      added cpexit to list
 *
 *   003   11-FEB-1988 12:52 bf 
 *	Added cpast.vc.  Maybe only temporary.
 *
 *   004  25-FEB-1988 17:21 mhs
 *      Simplify bookkeeping by directly calling cpglobal.vc
 *      vs. individual .vc files.
 *
 *   4-AUG-1988 14:10 ejs
 *  Changed the extention on cptok to 'thc'. Added 'hc' needed for 'vc' 
 *  counterparts.
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
 *   Filename:  cpvars.c
 *
 *   This file contains all the variable declarations for 
 *   the common parser, so that all variables can be placed 
 *   in one area of RAM.  It is a series of includes on the 
 *   .vc files produced by running the makehf tool on .def
 *   files.  There are currently only three files that 
 *   generate variables (until the actual action routines 
 *   are integrated into the code):  cpparse.vc, cpsixel.vc
 *   and cpsys.vc.
 *  
 *   end description
 *
 *-----------------------------------------------------------
 */





/*  begin include_file    */

#include    "portab.h"	    /* For portable definitions of types, etc. */
#include    "cptok.thc"	    /* For token names */

#include    "cpsys.hc"
#include    "cpsys.vc"      /* parser system wide defines */

#include    "caexit.hc"

#include    "cpglobal.hc"
#include    "cpglobal.vc"   /* global defs */

#include    "cpast.vc"      /* ANSI state tables */

/*  end   include_file    */

