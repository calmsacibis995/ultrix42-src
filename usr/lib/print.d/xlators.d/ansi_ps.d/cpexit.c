#ifndef lint
static char SccsId[] = "  @(#)cpexit.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cpexit.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *	1988, 1989.  ALL RIGHTS RESERVED.
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
 *  17-NOV-1987 16:47 mhs
 *      Original Version
 *
 *  19-MAY-1988 14:15 mhs
 *	Fix recursive bug in cp_cond_exit_crm and directly call
 *	ac_ex_crm instead.
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
 *   Filename:  cpexit.c
 *
 *   This file contains the exit routines.
 *
 *   This file contains:
 *
 *   cp_exit_cur_mode()
 *   cp_cond_exit_crm()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/*  begin include_file    */

#include    "portab.h"	    /* For portable definitions of types, etc.	*/
#include    "cpsys.hc"      /* Parser global variables and structures, etc. */
#include    "cpglobal.hc"   /* defs for cpxxx files */
#include    "camac.lib_hc"  /* macros for caxxx files */
#include    "caglobal.hc"   /* defs for caxxx files */

/*  end   include_file    */


/*
 *
 *   cp_exit_cur_mode()
 *
 *   Called to exit the current state of the parser, using 
 *   the exit routine pointer in the current first level 
 *   state table. 
 *
 */

VOID cp_exit_cur_mode()
   {
    (*(cp_ctptr -> exit))();
   }


/*
 *   cp_cond_exit_crm()
 *
 *   Tests the dip switches, and exits CRM mode only 
 *   if the switch is not set for Control Rendition.
 *
 */

VOID cp_cond_exit_crm()
   {
    if (!(PDLI_CRM_GET))    /* get value of CRM switch from machine */
       {
	ac_ex_crm();	    /* call the control rendition exit routine */
       }
   }

/*
 *   cp_cond_exit_crm_ris()
 *
 *   Tests the dip switches, and exits CRM mode only 
 *   if the switch is not set for Control Rendition.
 *   Then do RIS
 */

VOID cp_cond_exit_crm_ris()
   {
    if (!(PDLI_CRM_GET))    /* get value of CRM switch from machine */
       {
	ac_ex_crm();	    /* call the control rendition exit routine */
        pr_ris();
       }
   }

/*
 *   cp_cond_exit_crm_str()
 *
 *   Tests the dip switches, and exits CRM mode only 
 *   if the switch is not set for Control Rendition.
 *   Then do DECSTR
 */

VOID cp_cond_exit_crm_str()
   {
    if (!(PDLI_CRM_GET))    /* get value of CRM switch from machine */
       {
	ac_ex_crm();	    /* call the control rendition exit routine */
        dec_str();
       }
   }

