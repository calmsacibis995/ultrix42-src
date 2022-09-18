#ifndef lint
static char SccsId[] = "  @(#)dumpu.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: dumpu.c
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
 *   001   25-NOV-1987 13:06 mhw
 *      Initial version
 *   002   28-JAN-1988 12:04 bf
 *	Filled out pr_text to call composer routines.
 *   003    3-FEB-1988 08:20 mhw
 *      Moved pr_text to caparse.c
 *   004   10-FEB-1988 13:22  mhw
 *      Add dummy routine for lattice use of pprintf
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
 *   Filename:  dumpu.c
 *
 *   This is a Dump Utility file to include routines used by the dump utility
 *   that do not coorespond the any real routines in the Common Code and other
 *   general routines needed to test the parser.
 *
 *   end description
 *
 *-----------------------------------------------------------
 */





/*  begin include_file    */

#include    "portab.h"	    /* For portable definitions of types, etc.	*/
#include    "cpsys.hc"	    /* For global var. - cp_pcnt & cp_pbuf	*/
#include    "dumpu.hc"	    /* defs for this file			*/
#include    "xlc_iface.hc"  /* defines for oprintf			*/
#include    "caglobal.hc"    /* defines for xl_st			*/
#include    "camac.lib_hc"  /* for oprintf				*/


/*  end   include_file    */


#ifdef VAXC



/*******************************************************************
    This routine will print the parameter list when called
*******************************************************************/

VOID pprint()
   {
    WORD i;

    for (i = 0; i <= cp_pcnt; ++i)
       {
        oprintf("%d ", cp_pbuf[i]);
       }
   }


#else  /* Lattice */



/*******************************************************************
    This routine will print the parameter list when called
*******************************************************************/

VOID pprint()
   {
   }

#endif  /* Lattice */

