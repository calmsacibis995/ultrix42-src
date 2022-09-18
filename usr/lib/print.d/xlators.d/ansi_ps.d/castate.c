#ifndef lint
static char SccsId[] = "  @(#)castate.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: castate.c
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
 *    7-DEC-1987 13:51 mhw
 *      Initial Version
 *
 *   13-APR-1988 11:48 mhs
 *	Add routines for ac_dec_aupss and ac_dec_mac, to set the
 *      state table pointer.
 *      Add routines for ac_cr_sxl and ac_nl_sxl, to call dec_gcr
 *      and dec_gnl and then set the state to graphics SXL.
 *
 *  19-MAY-1988 16:48  mhw
 *	Corrected bug in ac_dec_lff.  Was (cpbuf[0] = 0) and should have been
 *	(cpbuf[0] == 0).  Bug caused lff never to be entered.
 *
 *  21-MAY-1988 13:57 araj
 *	Corrected ac_lff, so that the called routine can force ignore state.
 *	The state should be set before the routine is called, so it can be overwritten.
 *
 *   9-DEC-1988 15:59 mhw
 *	Corrected ac_dec_dtff so that it looks at cp_pbuf[0] for if the
 *	parameters are valid or not, instead of dcs_intro.ps1 which is not
 *	yet initialized at the time this routine is called.
 *
 *	Also fixed this type of problem in ac_dec_atff.
 *  
 *  16-DEC-1988 16:03 ejs
 *	Added routines to move from normal text mode to fast text mode.
 *
 *  18-JAN-1989 18:07 araj
 *	Added call to pr_ris and pr_str to exit CRM when the exit is caused by 
 *	ris or str	
 *
 *   4-APR-1989 10:40 ejs
 *	ca_zap_repeat used to clean up sixel command processing.  New routine to
 *	handle command states, ac_do_store_cmd1.
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
 *   Filename:  castate.c
 *
 *   This file contains the ANSI Change State Routines as
 *   described in the Common Action Routine Design Spec.
 *
 *   This file contains the following routines:
 * 
 *    ac_nop()	        ac_esc2()	       ac_scs()
 *    ac_text()         
 *    ac_esc_invalid()  ac_csi_invalid()
 *    ac_csi_startp()   ac_add_csip()          ac_add_csii()
 *    ac_csi_addp()     
 *    ac_dcs_invalid()  ac_dcs_ignore()	       ac_sxl_ignore()
 *    ac_dcs_startp()   ac_dcs_addp()
 *    ac_dec_lff()      ac_dec_atff()          ac_dec_dtff() 
 *    ac_dec_mac()      ac_dec_aupss()         ac_dec_graph()
 *    ac_dec_dld()      
 *    ac_store_cmd2()   ac_store_cmd1()        ac_add_dcs()      
 *    ac_crm()          ac_crm3()	       ac_crm4()
 *    ac_crm3_startp()  ac_crm3_addp()	       ac_crm_reset()
 *    ac_crm_lf()       ac_crm_ff()	       ac_crm_ris()
 *    ac_crm_c0()	ac_crm2_esc()	       ac_crm_str()
 *    ac_crm_c1()	ac_crm3_csi()	       ac_crm_crnr()
 *    ac_do_cmd2()      ac_start_cmd1()        ac_start_cmd2()
 *    ac_do_cr_sxl()    ac_do_nl_sxl()         ac_do_cmd1()
 *    ac_add_cmd1()     ac_add_cmd2()          ac_do_sxl()
 *    ac_do_store_cmd2()
 *    ac_gr_sxl()       ac_do_gr_sxl()
 *    ac_start_mac_rpt()
 *    ac_start_lff_rpt()
 *    ac_lff_term_ignore()
 *    ac_mac_term_ignore()
 *    ac_reset_start()
 *    ac_reset_font_lff()
 *    ac_reset_font_mac()
 *    ac_reset_lff()
 *    ac_reset_mac()
 *    ac_reset_sxl_lff()
 *    ac_reset_sxl_mac()
 *    ac_font_ignore()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"      /* ANSI State Table Declarations */
#include    "cpglobal.hc"   /* Globals for all Parser files */
#include    "caglobal.hc"   /* Globals for all CAR files */

/*  end   include_file    */



/****************************************************************************
    NOP Routine
*****************************************************************************/

VOID ac_nop()
    {
     /* This routine does nothing */
    }



/****************************************************************************
    NEL Routine
*****************************************************************************/

VOID ac_nel()
   {
    pr_nel();			/* Call the New Line function */
    cp_exit_cur_mode();		/* Exit the currrent mode */
   }



/****************************************************************************
    Set state to DCS_INVALID
*****************************************************************************/

VOID ac_dcs_invalid()
   {
    cp_setctptr(&ast_dcsinv);	/* Set Table Ptr to DCS_Invalid Table */
   }



/****************************************************************************
    Set state to DCS_IGNORE
*****************************************************************************/

VOID ac_dcs_ignore()
   {
    cp_setctptr(&ast_dcsignore); /* Set Table Ptr to DCS_IGNORE Table */
   }



/****************************************************************************
    Set state to SXL_IGNORE
*****************************************************************************/

VOID ac_sxl_ignore()
   {
    cp_setctptr(&ast_sxlignore); /* Set Table Ptr to SXL_IGNORE Table */
   }



/****************************************************************************
    Enter ECS2 State
*****************************************************************************/

VOID ac_esc2()
   {
    cp_setctptr(&ast_esc2);	/* Set Table Ptr to ESC2 Table */
    cp_add_to_esc_ibuf();	/* Add input to intermediate buffer */
   }



/****************************************************************************
    Enter SCS State
*****************************************************************************/

VOID ac_scs()
   {
    cp_setctptr(&ast_scs);	/* Set Table Ptr to SCS Table */
    cp_add_to_esc_ibuf();	/* Add input to intermediate buffer */
   }



/****************************************************************************
    Set State to Text
*****************************************************************************/

VOID ac_text()
   {
    cp_setctptr(&ast_text);	/* Set Table Ptr to TEXT Table */
   }

VOID ac_fast_text_gl()
   {
    /* Wish it weren't so, but ssf becomes a special case.  It affects the 
    **	next character in the data stream.  The following character is allowed
    **	to revert.  This means a change without an entry.
    **	    Ex.	    <ss2>ab results in a coming from a different gset than b,
    **		    yet the ab sequence would normally cause fast_text, thus
    **		    b would have been processed with a's gset.
    */
    if (!xl_st.ssf)
	{
        cp_setctptr(&ast_fast_text_gl);	/* Set Table Ptr to FAST_TEXT_GL*/
	} ;
    pr_text();
   }

VOID ac_fast_text_gr()
   {
    if (!xl_st.ssf)
	{
	cp_setctptr(&ast_fast_text_gr);	/* Set Table Ptr to FAST_TEXT_GR*/
	} ;
    pr_text();
   }

VOID ac_fast_space()
   {
    if (!xl_st.ssf)
	{
	cp_setctptr(&ast_fast_text_gl);	/* Set Table Ptr to FAST_TEXT_GL*/
	} ;
    pr_space();
   }

VOID ac_abort_fast_text()
   {
    cp_setctptr(&ast_text);	/* Set Table Ptr to TEXT Table */
   
   /* This is a little tricky. Instead of having multiple ac_abort_* to put
   ** each function back to normal text mode, all fast text 'faults' go 
   ** through this routine.  To continue, the text table is used to vector.
   */
   (*cp_tok_tbl[astx_text[cp_c]])();

   }



/****************************************************************************
    Set State to Invalid Escape
*****************************************************************************/

VOID ac_esc_invalid()
   {
    cp_setctptr(&ast_escinv);	/* Set Table Ptr to ESC Invalid Table */
   }



/****************************************************************************
    Set State to Invalid CSI
*****************************************************************************/

VOID ac_csi_invalid()
   {
    cp_setctptr(&ast_csiinv);	/* Set Table Ptr to CSI Invalid Table */
   }



/****************************************************************************
    Start a New Parameter and set the State to CSI_P
*****************************************************************************/

VOID ac_csi_startp()
   {
    cp_setctptr(&ast_csip);	/* Set Table Ptr to CSI_P Table */
    cp_start_new_param();	/* Start a new Parmeter in the param. Buf */
   }



/****************************************************************************
    Add input to Intermediate Buffer and set state to CSI_P
*****************************************************************************/

VOID ac_add_csip()
   {
    cp_setctptr(&ast_csip);	/* Set Table Ptr to CSI_P Table */
    cp_add_to_csi_ibuf();	/* Add input to Intermediate Buffer */
   }



/****************************************************************************
    Add input to Intermediate Buffer and set state to CSI_I
*****************************************************************************/

VOID ac_add_csii()
   {
    cp_setctptr(&ast_csii);	/* Set Table Ptr to CSI_I Table */
    cp_add_to_csi_ibuf();	/* Add input to Intermediate Buffer */
   }



/****************************************************************************
    Add input to Parameter Buffer and set state to CSI_P
*****************************************************************************/

VOID ac_csi_addp()
   {
    cp_setctptr(&ast_csip);	/* Set Table Ptr to CSI_P Table */
    cp_add_to_cur_param();	/* Add input to current parameter */
   }



/****************************************************************************
    Start new Parameter and set state to DCS_P
*****************************************************************************/

VOID ac_dcs_startp()
   {
    cp_setctptr(&ast_dcsp);	/* Set Table Ptr to DCS_P Table */
    cp_start_new_param();	/* Start new Parameter */
   }



/****************************************************************************
    Add to current Parameter and set state to DCS_P
*****************************************************************************/

VOID ac_dcs_addp()
   {
    cp_setctptr(&ast_dcsp);	/* Set Table Ptr to DCS_P Table */
    cp_add_to_cur_param();	/* Add to Current Parameter */
   }


/***********************************************************************
    Call dec_gr_enter and set state to GRAPH
***********************************************************************/

VOID ac_dec_graph()
   {
    cp_setctptr(&ast_graph);	/* Set State to Graphics */
    dec_gr_enter();		/* Enter Graphics Mode */
    cp_reset();			/* Reset the param and interm buffers */
   }



/****************************************************************************
    Call dec_lff_enter and set state to LFF
*****************************************************************************/

VOID ac_dec_lff()
   {
    /* First check that all parameters are valid 
     * (check for LN03 CFFF font format and valid delete command)
     */
    if ( (cp_pbuf[0] == 0) && (cp_pbuf[2] <= 1) )
       {
        cp_setctptr(&ast_lff);	/* Set Table Ptr to LFF Table */
	ca_start_dcs();		/* initialise DCS intro and vars */
	dec_lff_enter();	/* Call dec_lff_enter */
       }
    else
       {
	cp_setctptr(&ast_sxlignore); /* Set Table Ptr to SXL_IGNORE Table */
       }		
   }


/****************************************************************************
    Call dec_atff_enter and set state to ATFF
*****************************************************************************/

VOID ac_dec_atff()
   {
    /* First check that all parameters are valid */
    if ( (cp_pbuf[1] >= 10) || 
         (cp_pbuf[1] <= 19) ||
         (cp_pbuf[0] <= 2)
       )
       {
        cp_setctptr(&ast_atff);	/* Set Table Ptr to ATFF Table */
	ca_start_dcs();		/* initialise DCS intro and vars */
	dec_atff_enter();	/* Call dec_atff_enter */
       }
    else
       {
        cp_setctptr(&ast_dcsignore); /* Set Table Ptr to DCS_IGNORE Table */
       }
   }


/****************************************************************************
    Call dec_dtff_enter and set state to DTFF
*****************************************************************************/

VOID ac_dec_dtff()
   {
    /* First check that all parameters are valid */
    if (cp_pbuf[0] <= 1) 
       {
        cp_setctptr(&ast_dtff); /* Set Table Ptr to DTFF Table */
	ca_start_dcs();		/* initialise DCS intro and vars */
	dec_dtff_enter();	/* Call dec_dtff_enter */
       }
    else 
       {
	cp_setctptr(&ast_dcsignore); /* Set Table Ptr to DCS_IGNORE Table */
       }
   }



/****************************************************************************
    Call dec_aupss_enter and set state to AUPSS
*****************************************************************************/

VOID ac_dec_aupss()
   {
    /* First check that all parameters are valid */
    if (cp_pbuf[0] <= 1) 
       {
        cp_setctptr(&ast_aupss); /* Set Table Ptr to AUPSS Table */
	ca_start_dcs();		/* initialise DCS intro and vars */
	dec_aupss_enter();	/* Call dec_aupss_enter */
       }
    else
       {
        cp_setctptr(&ast_dcsignore); /* Set Table Ptr to DCS_IGNORE Table */
       }
   }



/****************************************************************************
    Call dec_mac_enter and set state to MAC
*****************************************************************************/

VOID ac_dec_mac()
   {
    cp_setctptr(&ast_mac);	/* Set Table Ptr to MAC Table */
    dec_mac_enter();		/* Call dec_mac_enter */
    cp_reset();			/* Reset the param and interm buffers */
   }



/****************************************************************************
    Call pr_ansb and set state to ANSB
*****************************************************************************/

VOID ac_pr_ansb()
   {
    cp_setctptr(&ast_ansb);	/* Set Table Ptr to ANSB Table */
    pr_ansb();			/* Load the Answerback message */
    cp_reset();			/* Reset the param and interm buffers */
   }



/****************************************************************************
    Call dec_uffm and set state to TEXT
*****************************************************************************/

VOID ac_dec_uffm()
   {
    cp_setctptr(&ast_text);	/* Set Table Ptr to TEXT Table */
    dec_uffm();			/* Upload Font File Metrics */
    cp_reset();			/* Reset the param and interm buffers */
   }



/****************************************************************************
    Call dec_dld_enter and set state to DLD (LA75 functionality)
    Not Implemented
*****************************************************************************/

VOID ac_dec_dld()
   {
    /*** TEST CODE ***/
    cp_setctptr(&ast_sxlignore); /* Set Table Ptr to DLD Table, ignore for now */
    /*** END TEST CODE ***/
    /* dec_dld_enter();		/* Call dec_dld_enter when implement this */
    cp_reset();			/* Reset the param and interm buffers */
   }



/****************************************************************************
    Store Command and set the state to GRAPHICS CMD 2    
*****************************************************************************/

VOID ac_store_cmd2()
   {
    cp_setctptr(&ast_graphcmd2); /* Set Table Ptr to Graphics CMD2 Table */
    ca_zap_repeat();		/* Before every command, cancel any repeat */
    cp_store_cmd(cp_c7);	/* Store the Sixel Command */
   }



/****************************************************************************
    Store Command and set the state to GRAPHICS CMD 1
*****************************************************************************/

VOID ac_store_cmd1()
   {
    cp_setctptr(&ast_graphcmd1); /* Set Table Ptr to Graphics CMD1 Table */
    ca_zap_repeat();		/* Before every command, cancel any repeat */
    cp_store_cmd(cp_c7);	/* Store the Sixel Command */
   }



/****************************************************************************
    Add Intermediate to buffer and set state to DCS_I    
*****************************************************************************/

VOID ac_add_dcs()
   {
    cp_setctptr(&ast_dcsi);	/* Set Table Ptr to DCS_I Table */
    cp_add_to_dcs_ibuf();	/* Store the intermediate in buffer */
   }



/****************************************************************************
    Print the input and set the state to Control Rendition     
*****************************************************************************/

VOID ac_crm()
   {
    cp_setctptr(&ast_crm);	/* Set Table Ptr to Control Rendition Table */
    pr_text_crm();		/* Print the input as is */
   }



/****************************************************************************
    Print the input and set the state to Control Rendition     
*****************************************************************************/

VOID ac_crm_c0()
   {
    cp_setctptr(&ast_crm);	/* Set Table Ptr to Control Rendition Table */
    pr_c0_crm();		/* Print the input as is */
   }



/****************************************************************************
    Print the input and set the state to Control Rendition     
*****************************************************************************/

VOID ac_crm_c1()
   {
    cp_setctptr(&ast_crm);	/* Set Table Ptr to Control Rendition Table */
    pr_c1_crm();		/* Print the input as is */
   }



/****************************************************************************
    Print the input and set the state to Control Rendition 2    via <ESC>
*****************************************************************************/

VOID ac_crm2_esc()
   {
    cp_setctptr(&ast_crm2);	/* Set Table Ptr to Ctrl Ren 2 Table */
    pr_c0_crm();		/* Print the input as is */
    cp_reset();			/* Reset the param and intermed buffers */
   }



/****************************************************************************
    Print the input and set the state to Control Rendition 3    via <ESC>[
*****************************************************************************/

VOID ac_crm3()
   {
    cp_setctptr(&ast_crm3);	/* Set Table Ptr to Ctrl Ren 3 Table */
    pr_text_crm();		/* Print the input as is */
    cp_reset();			/* Reset the param and intermed buffers */
   }



/****************************************************************************
    Print the input and set the state to Control Rendition 3    via <CSI>
*****************************************************************************/

VOID ac_crm3_csi()
   {
    cp_setctptr(&ast_crm3);	/* Set Table Ptr to Ctrl Ren 3 Table */
    pr_c1_crm();		/* Print the input as is */
    cp_reset();			/* Reset the param and intermed buffers */
   }



/****************************************************************************
    Print the input and set the state to Control Rendition  4   
*****************************************************************************/

VOID ac_crm4()
   {
    cp_setctptr(&ast_crm4);	/* Set Table Ptr to Ctrl Ren 4 Table */
    pr_text_crm();		/* Print the input as is */
   }



/****************************************************************************
    Print the input, start a new Parameter, and set the state to Ctrl Ren 3
*****************************************************************************/

VOID ac_crm3_startp()
   {
    cp_setctptr(&ast_crm3);	/* Set Table Ptr to Ctrl Ren 3 Table */
    pr_text_crm();		/* Print the input as is */
    cp_start_new_param();	/* Start a new Param. in the parameter Buf. */
   }



/****************************************************************************
    Print the input, add to current param. and set the state to Ctrl Ren 3
*****************************************************************************/

VOID ac_crm3_addp()
   {
    pr_text_crm();		/* Print the input as is */
    cp_add_to_cur_param();	/* Add input to current parameter */
   }



/****************************************************************************
    Control Rendition  Corner Character
******************************************************************************/

VOID ac_crm_crnr()
   {
    cp_setctptr(&ast_crm);	/* Set Table Ptr to Control Rendition Table */
    pr_crnr_crm();		/* Print the input as is */
   }



/****************************************************************************
    Control Rendition  LF 
******************************************************************************/

VOID ac_crm_lf()
   {
    cp_setctptr(&ast_crm);	/* Set Table Ptr to Control Rendition Table */
    pr_c0_crm();		/* Print the input as is */
    pr_nel();			/* Print the Carriage Return/Line Feed */
   }



/****************************************************************************
    Control Rendition  FF 
******************************************************************************/

VOID ac_crm_ff()
   {
    cp_setctptr(&ast_crm);	/* Set Table Ptr to Control Rendition Table */
    pr_c0_crm();		/* Print the input as is */
    pr_ff();			/* Print the Form Feed */
   }



/****************************************************************************
    Control Rendition  RIS
******************************************************************************/

VOID ac_crm_ris()
   {
    pr_text_crm();		/* print the Final as is */
    cp_cond_exit_crm();
   }



/****************************************************************************
    Control Rendition  STR
******************************************************************************/

VOID ac_crm_str()
   {
    pr_text_crm();		/* print the Final as is */
    cp_cond_exit_crm();
   }



/****************************************************************************
    Control Rendition  Reset Sequence - must check to make sure we got CSI 3l
******************************************************************************/

VOID ac_crm_reset()
   {
    WORD i;

    pr_text_crm();		/* print the Final as is */

    for (i = 0; i <= cp_pcnt; ++i)
       {
	if (cp_pbuf[i] == 3)
           {
	    cp_cond_exit_crm();
	   }
       }
   }



/****************************************************************************
    Do current command and set to Graphic_cmd2
*****************************************************************************/

VOID ac_do_cmd2()
   {
    cp_setctptr(&ast_graphcmd2); /* Set Table Ptr to Graphics CMD2 Table */
    cp_do_cur_cmd();		/* Do the current sixel command */
    ca_zap_repeat();
   }



/****************************************************************************
    Start New Parameter and set to Graphic_cmd1
*****************************************************************************/

VOID ac_start_cmd1()
   {
    cp_setctptr(&ast_graphcmd1); /* Set Table Ptr to Graphics CMD1 Table */
    cp_start_new_param();	/* Start new parameter in buffer */
   }



/****************************************************************************
    Start New Parameter and set to Graphic_cmd2
*****************************************************************************/

VOID ac_start_cmd2()
   {
    cp_setctptr(&ast_graphcmd2); /* Set Table Ptr to Graphics CMD2 Table */
    cp_start_new_param();	/* Start new parameter in buffer */
   }



/****************************************************************************
    Do graphic CR, set state to GRAPHICS_SXL
*****************************************************************************/

VOID ac_cr_sxl()
   {
    cp_setctptr(&ast_graphsxl);	/* Set Table Ptr to Graphics SXL Table */
    ca_zap_repeat();		/* Before every command, cancel any repeat */
    dec_gcr();			/* do a graphic CR */
   }



/****************************************************************************
    Do graphic NL, set state to GRAPHICS_SXL
*****************************************************************************/

VOID ac_nl_sxl()
   {
    cp_setctptr(&ast_graphsxl);	/* Set Table Ptr to Graphics SXL Table */
    ca_zap_repeat();		/* Before every command, cancel any repeat */
    dec_gnl();			/* do a graphic NL */
   }



/****************************************************************************
    Do current sixel command, do graphic CR, set state to GRAPHICS_SXL
*****************************************************************************/

VOID ac_do_cr_sxl()
   {
    cp_setctptr(&ast_graphsxl);	/* Set Table Ptr to Graphics SXL Table */
    cp_do_cur_cmd();		/* Do the current sixel command */
    ca_zap_repeat();		/* Before every command, cancel any repeat */
    dec_gcr();			/* do a graphic CR */
   }



/****************************************************************************
    Do current sixel command, do graphic NL, set state to GRAPHICS_SXL
*****************************************************************************/

VOID ac_do_nl_sxl()
   {
    cp_setctptr(&ast_graphsxl);	/* Set Table Ptr to Graphics SXL Table */
    cp_do_cur_cmd();		/* Do the current sixel command */
    ca_zap_repeat();		/* Before every command, cancel any repeat */
    dec_gnl();			/* do a graphic NL */
   }



/****************************************************************************
    Do current sixel command, set state to GRAPHICS_CMD1
*****************************************************************************/

VOID ac_do_cmd1()
   {
    cp_setctptr(&ast_graphcmd1); /* Set Table Ptr to Graphics CMD1 Table */
    cp_do_cur_cmd();		/* Do the current sixel command */
   }



/****************************************************************************
    Add to current parameter, set state to GRAPHICS_CMD1
*****************************************************************************/

VOID ac_add_cmd1()
   {
    cp_setctptr(&ast_graphcmd1); /* Set Table Ptr to Graphics CMD1 Table */
    cp_add_to_cur_param();	/* Add to current parameter */
   }



/****************************************************************************
    Add to current parameter, set state to GRAPHICS_CMD2
*****************************************************************************/

VOID ac_add_cmd2()
   {
    cp_setctptr(&ast_graphcmd2); /* Set Table Ptr to Graphics CMD2 Table */
    cp_add_to_cur_param();	/* Add to current parameter */
   }



/****************************************************************************
    Do the current sixel command and set state to GRAPHICS_SXL
*****************************************************************************/

VOID ac_do_sxl()
   {
    cp_setctptr(&ast_graphsxl);	/* Set Table Ptr to Graphics SXL Table */
    cp_do_cur_cmd();		/* Do current sixel command */
    ca_zap_repeat();		/* Before every command, cancel any repeat */
				/* In this case we are leaving the command
				   state due to a disabled command DECGRA,
				   and want to zap any pending repeat */
   }



/****************************************************************************
    Do current sixel command & store new command, set state to GRAPHICS_CMD1
*****************************************************************************/

VOID ac_do_store_cmd1()
   {
    cp_setctptr(&ast_graphcmd1); /* Set Table Ptr to Graphics CMD1 Table */
    cp_do_store_cmd();		/* Do the current sixel command and store the
				   new sixel command */
    ca_zap_repeat();		/* Before every command, cancel any repeat */
				/* Since do_store was combined, and there is
				   no problem with the zap after the store,
				   the order is slightly different than the
				   other store commands */
   }


/****************************************************************************
    Do current sixel command & store new command, set state to GRAPHICS_CMD2
*****************************************************************************/

VOID ac_do_store_cmd2()
   {
    cp_setctptr(&ast_graphcmd2); /* Set Table Ptr to Graphics CMD2 Table */
    cp_do_store_cmd();		/* Do the current sixel command and store the
				   new sixel command */
    ca_zap_repeat();		/* Before every command, cancel any repeat */
				/* Since do_store was combined, and there is
				   no problem with the zap after the store,
				   the order is slightly different than the
				   other store commands */
   }



/****************************************************************************
    Call dec_gr_sxl and change the state to GRAPHIC_SXL    
*****************************************************************************/

VOID ac_gr_sxl()
   {
    cp_setctptr(&ast_graphsxl);	/* Set Table Ptr to Graphics SXL Table */
    dec_gr_sxl();		/* call dec_gr_sxl */
   }



/****************************************************************************
    Do the current sixel command, set state to GRAPHICS_SXL
*****************************************************************************/

VOID ac_do_gr_sxl()
   {
    cp_setctptr(&ast_graphsxl);	/* Set Table Ptr to Graphics SXL Table */
    cp_do_cur_cmd();		/* Do the current sixel command */
    dec_gr_sxl();		/* call dec_gr_sxl */
   }



/****************************************************************************
    Start a new parameter and set the state to Macro Repeat
*****************************************************************************/

VOID ac_start_mac_rpt()
   {
    cp_setctptr(&ast_macrpt);	/* Set Table Ptr to MACRO REPEAT Table */
    cp_start_new_param();	/* Start a new parameter in buffer */
   }



/****************************************************************************
    Start a new parameter and set the state to LFF Repeat
*****************************************************************************/

VOID ac_start_lff_rpt()
   {
    cp_setctptr(&ast_lffrpt);	/* Set Table Ptr to LFF REPEAT Table */
    cp_start_new_param();	/* Start a new parameter in buffer */
   }



/****************************************************************************
    Terminate LFF and enter SXL_IGNORE state    
*****************************************************************************/

VOID ac_lff_term_ignore()
   {
    cp_setctptr(&ast_sxlignore); /* Set Table Ptr to SXL_IGNORE Table */
    dec_lff_term();		/* Terminate LFF */
   }



/****************************************************************************
    Terminate MACRO and enter SXL_IGNORE state    
*****************************************************************************/

VOID ac_mac_term_ignore()
   {
    cp_setctptr(&ast_sxlignore); /* Set Table Ptr to SXL_IGNORE Table */
    dec_mac_term();		/* Terminate MACRO */
   }



/****************************************************************************
    Reset the Parameter Buffer and Start a new parameter 
*****************************************************************************/

VOID ac_reset_start()
   {
    cp_reset();			/* Reset the state of the parser */
    cp_start_new_param();	/* Start a new parameter */
   }



/****************************************************************************
    Reset the Buffers, call dec_lff_font and set the state to LFF
*****************************************************************************/

VOID ac_reset_font_lff()
   {
    cp_setctptr(&ast_lff);	/* Set Table Ptr to LFF Table */
    dec_lff_font();		/* Load one font and begin the next */
    cp_reset();			/* Reset the Parser */
    ca_restart_dcs();		/* reset the DCS counters */
   }



/****************************************************************************
    Reset the Buffers, call dec_font_mac and set the state to MACRO
*****************************************************************************/

VOID ac_reset_font_mac()
   {
    cp_setctptr(&ast_mac);	/* Set Table Ptr to MAC Table */
    dec_mac_font();		/* Load one macro and begin the next */
    cp_reset();			/* Reset the Parser */
    ca_restart_dcs();		/* reset the DCS counters */
   }



/****************************************************************************
    Reset the Buffers, and set the state to LFF
*****************************************************************************/

VOID ac_reset_lff()
   {
    cp_setctptr(&ast_lff);	/* Set Table Ptr to LFF Table */
    cp_reset();			/* Reset the Parser */
   }



/****************************************************************************
    Reset the Buffers, and set the state to MAC
*****************************************************************************/

VOID ac_reset_mac()
   {
    cp_setctptr(&ast_mac);	/* Set Table Ptr to MAC Table */
    cp_reset();			/* Reset the Parser */
   }



/****************************************************************************
    Reset the Buffers, call dec_lff_sxl, and set the state to LFF
*****************************************************************************/

VOID ac_reset_sxl_lff()
   {
    cp_setctptr(&ast_lff);	/* Set Table Ptr to LFF Table */
    dec_lff_sxl_rpt();		/* Byte of sixel data in Font Record */
    cp_reset();			/* Reset the Parser */
   }



/****************************************************************************
    Reset the Buffers, call dec_mac_sxl and set the state to MAC
*****************************************************************************/

VOID ac_reset_sxl_mac()
   {
    cp_setctptr(&ast_mac);	/* Set Table Ptr to MAC Table */
    dec_mac_sxl();		/* Byte of sixel data found in Macro record */
    cp_reset();			/* Reset the Parser */
   }



/****************************************************************************
    Call cp_font_name and set the state to DCS_IGNORE 
*****************************************************************************/

VOID ac_font_ignore()
   {
    cp_setctptr(&ast_dcsignore); /* Set Table Ptr to DCS_IGNORE Table */
    cp_font_name();		/* process the final font name character */
    dec_aupss_term();		/* select the font */
   }

