#ifndef lint
static char SccsId[] = "  @(#)caexit.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: caexit.c
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
 *  begin edit_history
 *
 *   7-DEC-1987 16:07 mhw
 *      Initial Entry 
 *
 *  19-MAY-1988 14:17 mhs
 *	Add ac_ex_crm for exit control rendition mode.
 *
 *  end edit_history
 *
 *-----------------------------------------------------------
 */



/*
 *-----------------------------------------------------------
 *
 *   begin description
 *
 *   Filename:  caexit.c
 *
 *   This file contains the ANSI Exit State routines
 *
 *   This module contains:
 *
 *   ac_exit()
 *   ac_ex_graphics()
 *   ac_ex_ansb()
 *   ac_ex_atff()
 *   ac_ex_dtff()
 *   ac_ex_lff()
 *   ac_ex_crm()
 *   ac_ex_mac()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"      /* First level table declarations  */
#include    "cpglobal.hc"   /* defs for cpxxx.c files */
#include    "caglobal.hc"   /* defs for caxxx.c files */

/*  end   include_file    */


/*****************************************************************************
    Exit Routine for most states of ANSI.  It will set the state to TEXT

    Used for TEXT, IGNORE, ESC, ESC2, ESC_INVALID, SCS, CSI, CSI_I, CSI_P,
	     CSI_INVALID, DCS, DCS_I, DCS_P, and DCS_INVALID.

*******************************************************************************/

VOID ac_exit()
   {
    cp_setctptr(&ast_text);	/* Set state to TEXT */
   }


/*****************************************************************************
    Exit Routine for all Graphics states.
*******************************************************************************/

VOID ac_ex_graphics()
   {
    dec_gr_term();		/* restore active position to left bound/etc */
    cp_setctptr(&ast_text);	/* Set state to TEXT */
   }


/*****************************************************************************
    Exit Routine for Answerback.  Store the answerback and copy the 
    buffer before setting the state to TEXT
*******************************************************************************/

VOID ac_ex_ansb()
   {
    pr_ansb_store();		/* Store the ansb and copy the buffer */
    cp_setctptr(&ast_text);	/* Set state to TEXT */
   }


/*****************************************************************************
    Exit Routine for ATFF.  
*******************************************************************************/

VOID ac_ex_atff()
   {
    dec_atff_term();		/* Terminate ATFF */
    cp_setctptr(&ast_text);	/* Set state to TEXT */
   }


/*****************************************************************************
    Exit Routine for DTFF.  
*******************************************************************************/

VOID ac_ex_dtff()
   {
    dec_dtff_term();		/* Terminate DTFF */
    cp_setctptr(&ast_text);	/* Set state to TEXT */
   }


/*****************************************************************************
    Exit Routine for LFF.  
*******************************************************************************/

VOID ac_ex_lff()
   {
    dec_lff_term();		/* Terminate LFF */
    cp_setctptr(&ast_text);	/* Set state to TEXT */
   }


/*****************************************************************************
    Exit Routine for AUPSS
*******************************************************************************/

VOID ac_ex_aupss()
   {
    dec_aupss_term();		/* terminate the AUPSS */
    cp_setctptr(&ast_text);	/* Set state to TEXT */
   }


/*****************************************************************************
    Exit Routine for Macro
*******************************************************************************/

VOID ac_ex_mac()
   {
    dec_mac_term();		/* Terminate Macro */
    cp_setctptr(&ast_text);	/* Set state to TEXT */
   }


/*****************************************************************************
    Exit Routine for Control Rendition Mode
*******************************************************************************/

VOID ac_ex_crm()
   {
    cp_setctptr(&ast_text);	/* Set state to TEXT */
   }

