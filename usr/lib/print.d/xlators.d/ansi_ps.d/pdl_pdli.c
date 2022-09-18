#ifndef lint
static char SccsId[] = "  @(#)pdl_pdli.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: pdl_pdli.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *	1988 ALL RIGHTS RESERVED.
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
 *  15-FEB-1988 15:08	mhw
 *      Initial version
 *
 *  19-FEB-1988 16:56	mhw
 *      add initialization of jfy_buf_index 
 *
 *  15-MAY-1988 11:58	araj
 *	add initialization of constants here for the time being
 *
 *  23-NOV-1988 12:59 mhw
 *	Temporary fix for computing tabs added to pdl_init
 *	This will be removed when NVM is done correctly
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
 *   Filename:  pdl_pdli.c
 *
 *   This file contains:
 *
 *    pdl_init()
 *    pdli_flag_nvm_store()
 *    pdli_flag_nvm_recall()
 *    pdli_flag_font_dict_change()
 * 
 *   end description
 *
 *-----------------------------------------------------------
 */



/*  begin include_file    */
#include    "portab.h"		/* portability file for global def's */
#include    "cpsys.hc"		/* Common Parser variables */
#include    "cpglobal.hc"	/* Globals for cpxxx files */
#include    "camac.lib_hc"	/* Macros for caxxx files */
#include    "caglobal.hc"	/* Globals for caxxx files */
#include    "cpast.hc"		/* Allows override of current state */
/*  end   include_file    */


/*
 * pdl_init()
 *
 * This routine initialises the parser to a state that allows it to
 * communicate effectively and correctly through the PDL Interface.
 * This routine is called on powerup.
 */
VOID pdl_init(init_vals)
PTR_CPINIT_STATE init_vals;
   {
    /* Initialise the local copy of the PDL Interface values */
    pdli_init_st = *init_vals;

    /*
     * Initialise the Common Parser variables.
     * For now, set cp_host from the interface
     * initialiser structure, until it is decided
     * whether pdli_init_st.host_ptr is a global 
     * variable to be referenced by the parser 
     * routines.
     */
    cp_host = pdli_init_st.host_ptr;
    cp_init();

    /* Set the flags for configuration change */
    pdli_flag_font_dict_change();
    pdli_flag_nvm_recall();

    /* Initialise the xl_st variables */
    ca_state_init();

    /* Initialise the local copy of NVM (nvm_st)
     * so that NVM Recall can be emulated using
     * the initial values of xl_st when there is
     * no NVM in the machine or when NVM fails.
     */
    nvm_gather();

/******************************************************************************
 ***************************   TEMPORARY FIX *********************************
 *
 * This is a tempory fix until NVM is done correctly
 *
 * The horizontal and vertical tab count will be set to -1 such that in 
 * ca_set_given_state the init routines will be called to calculate where
 * the tabs should be.  If NVM were correct, this would be passed to us by
 * the Translator.  This problem was seen when we tried to print Sue
 * Gleeson's file that contained tabs, in landscape mode.
 *
 */

    nvm_st.svst_htabct = -1;
    nvm_st.svst_vtabct = -1;

/*********************************************************************/

   }


/*
 * pdli_flag_nvm_store
 */

VOID pdli_flag_nvm_store()
   {
    /*
     * If we have been asked to do an NVM Recall, and have not yet
     * completed it, then ignore this NVM Store request for now so 
     * that we don't get a mixed state.  Otherwise, the printer is
     * assumed to have guaranteed that the parser is not processing
     * an action routine that could be trashing xl_st values during
     * the NVM Store.
     */
    if (!nvm_recall_flag)
       {
	ca_nvm_store();
       }
   }


/*
 * pdli_flag_nvm_recall
 */

VOID pdli_flag_nvm_recall()
   {
    /*
     * Set the NVM Recall flag for the cp_parse loop,
     * so that an NVM Recall doesn't trash xl_st while
     * it is being modified by an action routine.
     */
    nvm_recall_flag = TRUE;
    nvm_cp_ctptr = cp_ctptr ;
    cp_ctptr = &ast_nvm ;
   }


/* 
 * pdli_flag_font_dict_change;
 */

VOID pdli_flag_font_dict_change()
   {
    font_dict_change_flag = TRUE;
   }

