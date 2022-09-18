#ifndef lint
static char SccsId[] = "  @(#)catmgt.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: catmgt.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,
 *	1986, 1987, 1988, 1989 ALL RIGHTS RESERVED.
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
 *  28-JAN-1988 15:26  mhw
 *      Initial Entry of combination of catmgt and dutgmt
 *
 *  13-FEB-1988 13:48  bf
 *	Changed DEBUG value to compile real code.
 *	changed references to private to use cp_pflag and
 *	to plist to use cp_pbuf.  Added include of cpbuf_mgt.hc
 *	for declarations.
 *
 *  16-FEB-1988 09:40  mhw
 *      Added camac.lib_hc to list of includes for null oprintf 
 *      when not a DUMP
 *
 *  23-FEB-1988 15:07 mhw
 *	Remove cp_pflag from pr_ris, it automatically calls dec_pfs
 *      and does not need the private flag.  Also removed include
 *      of cpbuf_mgt since only needed for private flag.
 *
 *   5-APR-1988 16:50 mhs
 *      Add switch statement to DECSTR to implement parameter.
 *
 *  20-APR-1988 10:45 mhw
 *	Added call to pdli_dispose_reserve_memory to pr_ris to reset the
 *      memory back to factory defaults
 *
 *  13-MAY-1988 14:03 mhs
 *	Implement Control Rendition Mode.
 *
 *  19-MAY-1988 17:46 mhs
 *	Move all Control Rendition routines to caparse.c.
 *
 *   7-JUL-1988 14:28 mhs
 *	Add nvm_gather routine as utility routine to support NVM.
 *	Add ca_set_given_state routine as utility routine for normal init
 *	as well as NVM support when the parameter points to NVM.
 *
 *   7-NOV-1988 12:30	araj
 *	Fix SCL so that is does parameter ranging and checking
 *	so that we don't fetch reset values from scl[1223434556778]...
 *
 *  30-NOV-1988 14:28 araj
 *	Install cfont caching, change calls to compute font into calls to invalidate_font
 *	Make sure references to vai or g_table have a test for the validity of the data
 *	first
 *
 *   8-DEC-1988 11:56 ejs
 *	Added nvm_stall_io and nvm_recall.
 *
 *  18-JAN-1989 18:14 araj
 *	added exit JFY to RIS/STR (in set_given_state)
 *
 *  18-JAN-1989 18:14 araj
 *	added RIS and STR to exit CRM
 *
 *  24-JAN-1989 17:07 araj
 *	Modified pr_ris and decstr so that NVM is default, level 3 is
 *	special
 *
 *  20-FEB-1989 13:06 araj
 *	Moved compute spacing before htabs_init, so xl_st.hai is computed
 *	before being used.
 *
 *  20-MAR-1989 C. Peters
 *	Removed extraneous '&' operator for Ultrix port.
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
 *   Filename:  catmgt.c
 *
 *
 *   This module contains the following routines:
 *
 *   nvm_gather()
 *   pr_dsr()
 *   dec_str()
 *   pr_ris()
 *   pr_cpr()
 *   pr_da()
 *   pr_da2()
 *   pr_bel()
 *   dec_fnvr()
 *   pr_ansb()
 *   pr_ansb_num()
 *   pr_ansb_cap()
 *   pr_ansb_lc()
 *   pr_ansb_store()
 *   dec_bcmm() 
 *   pr_spd()
 *   dec_den()
 *   dec_snc()
 *   dec_sss()
 *   dec_scl()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */


/* Translator edit history for this file 
 *
 * file:	xlc_termmanag.c - Translator terminal management routines
 * created:	gh	 26-MAR-1986 09:10:31
 * edit:	gh	 11-APR-1986 15:13:22 Added code for ris() and decstr()
 *		gh	 28-APR-1986 09:01:20 Adding call to dispose_condshowpg
 *			 in ris()
 *		gh	 2-MAY-1986 08:59:04 Adding call to font_init() in ris()
 *		gh	 5-JUN-1986 16:49:16 Changed ris() to do a more complete
 *			 job of initialization, including calling scan_items()
 *		nv	 10-JUL-1986 18:04:04 Included call to 'reset_pitch ()'
 *			in 'ris ()' and embellished a few comments.
 *		nv	 12-JUL-1986 11:31:39 Removed old commented-out 
 *			conditional initialization of .cur_sgr.
 *		nv	 16-SEP-1986 15:59:06 Make provision within ris
 *			for the distinction between power_up and ordinary ris.
 *			Have decstr call a non 'power_up' kind of ris.
 *		nv	 17-OCT-1986 11:21:34 Remove all reference to 'hsi'.
 *		nv	 22-OCT-1986 13:48:09 Move initialization of 
 *			'xl_st.cur_sgr' to 'scan_items'.
 *	        kws	 13-MAY-1987 16:24:32 Reinitialize current font on
 *			a 'power_up' ris.
 *
 *		araj	 16-JUL-1987 20:27:54 undid previous change.
 *			The fix is not to always set SGR to 1, as done 
 *			above, but to not forget to define a default
 *			in SCAN_ITEMS if no papersize is passed.
 */



#define catmgt (1)

/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* general defs, typedefs, etc. */
#include    "cpast.hc"	    /* ANSI State Table & Token Table Declarations */
#include    "cpglobal.hc"   /* global defs for cpxxx.c files		*/
#include    "camac.lib_hc"  /* CAR macros                               */
#include    "caglobal.hc"   /* global defs for caxxx.c files		*/

#ifdef DUMP
#include    "dumpu.hc"      /* Dump Utility                             */
#include    "xlc_iface.hc"  /* defines for oprintf			*/
#endif DUMP

/*  end   include_file    */


/*
 * nvm_recall
 */

VOID nvm_recall()
{
    ca_restore_default();
    nvm_recall_flag = FALSE;

    /* Switch the IO ptr so the current unprocessed input will not be lost */
    nvm_hold_ioptr = cp_ioptr ;
    cp_ioptr = nvm_stall_io ;

    /* Restore the table pointer for the next pass through. */
    cp_ctptr = nvm_cp_ctptr ;
}

/*
 * nvm_stall_io
 */

DEF nvm_stall_io()
{
    /* Restore the IO pointer so the next byte of data is gotten normally.
       Return the unprocessed data from the last IO call */
    cp_ioptr = nvm_hold_ioptr ;
    return (test_for_eof) ;
}

/*
 * nvm_gather
 */

VOID nvm_gather()
   {
    WORD i;	    /* loop index */

    strcpy(nvm_st.svst_version_id,VERSION);

    nvm_st.svst_origin.xval = xl_st.origin.xval;
    nvm_st.svst_origin.yval = xl_st.origin.yval;

    nvm_st.svst_lfnl_mode = xl_st.lfnl_mode;
    nvm_st.svst_crnl_mode = xl_st.crnl_mode;
    nvm_st.svst_wrap_mode = xl_st.wrap_mode;
    nvm_st.svst_psp = xl_st.psp;
    nvm_st.svst_c1rcv_mode = xl_st.c1rcv_mode;

    nvm_st.svst_psel_mode = xl_st.psel_mode;
    nvm_st.svst_shorp = xl_st.shorp;

    nvm_st.svst_pum_mode = xl_st.pum_mode;
    nvm_st.svst_sizeunit = xl_st.sizeunit;

    nvm_st.svst_hai = xl_st.hai;
    nvm_st.svst_vsi = xl_st.vsi;

    nvm_st.svst_last_gss = xl_st.last_gss;
    nvm_st.svst_v_size = xl_st.v_size;
    nvm_st.svst_character_proportion = xl_st.character_proportion;
    nvm_st.svst_user_pref_cset = xl_st.user_pref_cset;
    nvm_st.svst_user_pref_repertory = xl_st.user_pref_repertory;

    nvm_st.svst_gl_index = (xl_st.gl_ptr - (&xl_st.g_table[0]) );
    nvm_st.svst_gr_index = (xl_st.gr_ptr - (&xl_st.g_table[0]) );

    for (i = 0; i < G_TABLE_SIZE ; i++)
       {
	nvm_st.svst_repertory[i] = xl_st.g_table[i].repertory;
	nvm_st.svst_char_set_id[i] = xl_st.g_table[i].char_set_id;
       }

    nvm_st.svst_requested_attributes = xl_st.requested_attributes;
    nvm_st.svst_cur_sgr = xl_st.cur_sgr;

    for (i = 0; i < SGR_TABLE_SIZE; i++)
       {
	nvm_st.svst_sgr_tbl[i].font_file = xl_st.sgr_tbl[i].font_file;
	nvm_st.svst_sgr_tbl[i].selection_type = xl_st.sgr_tbl[i].selection_type;
        strcpy (nvm_st.svst_sgr_tbl[i].id_string, xl_st.sgr_tbl[i].id_string);
       }

    /* Left margin */
	nvm_st.svst_pfs.pfs_lrm.min = xl_st.h_lim_bound.min;
    /* Right margin */
	nvm_st.svst_pfs.pfs_lrm.max = xl_st.h_lim_bound.max;
    /* Line home pos */
	nvm_st.svst_pfs.pfs_lhe.min = xl_st.h_fmt_bound.min;
    /* Line end pos */
	nvm_st.svst_pfs.pfs_lhe.max = xl_st.h_fmt_bound.max;
    /* Top margin */
	nvm_st.svst_pfs.pfs_tbm.min = xl_st.v_lim_bound.min;
    /* Bottom margin */
	nvm_st.svst_pfs.pfs_tbm.max = xl_st.v_lim_bound.max;
    /* Page home line */
	nvm_st.svst_pfs.pfs_phe.min = xl_st.v_fmt_bound.min;
    /* Page end line */
	nvm_st.svst_pfs.pfs_phe.max = xl_st.v_fmt_bound.max;
    /* Width of form */
	nvm_st.svst_pfs.pfs_pwid = xl_st.sheet_fwid;
    /* Length of form */
	nvm_st.svst_pfs.pfs_plen = xl_st.sheet_flen;
    /* Set orientation */
	nvm_st.svst_pfs.pfs_or = xl_st.orientation;

    nvm_st.svst_htabct = xl_st.htabct;

    for (i = 0; i < nvm_st.svst_htabct; i++)
       {
	nvm_st.svst_htabs[i] = xl_st.htabs[i];
       }

    nvm_st.svst_vtabct = xl_st.vtabct;

    for (i = 0; i < nvm_st.svst_vtabct; i++)
       {
	nvm_st.svst_vtabs[i] = xl_st.vtabs[i];
       }

    nvm_st.svst_paper_tray = xl_st.paper_tray;
    nvm_st.svst_num_copies = xl_st.num_copies;
   }


/*
 * ca_nvm_store
 */

VOID ca_nvm_store()
   {
    nvm_gather();			/* fill the local copy of NVM */
    pdli_nvm_store(&nvm_st, S_NVM_ST);	/* attempt to move this data to NVM */
   }


/*
 * ca_set_given_state(restore_st)
 */

VOID ca_set_given_state(restore_st)
PPDL_SAVED_STATE restore_st;
   {
    WORD i;	    /* loop index */
    LONG dummy;	    /* used by compute_spacing */

    /* First, exit the current state in case we were in Graphics Mode */
    cp_exit_cur_mode();

    /* First, empty any contents of the justify buffer */
    empty_jfy_buf();

    /* Get out of JFY mode */
    xl_st.justify_mode = JUSTIFY_OFF;

    /* Close any open states - print page if something on it */
    process_condshowpg();

    xl_st.origin.xval = restore_st->svst_origin.xval;
    xl_st.origin.yval = restore_st->svst_origin.yval;

    xl_st.lfnl_mode = restore_st->svst_lfnl_mode;
    xl_st.crnl_mode = restore_st->svst_crnl_mode;
    xl_st.wrap_mode = restore_st->svst_wrap_mode;
    xl_st.psp = restore_st->svst_psp;
    xl_st.c1rcv_mode = restore_st->svst_c1rcv_mode;

    xl_st.psel_mode = restore_st->svst_psel_mode;
    xl_st.shorp = restore_st->svst_shorp;

    xl_st.pum_mode = restore_st->svst_pum_mode;
    xl_st.sizeunit = restore_st->svst_sizeunit;

    xl_st.hai = restore_st->svst_hai;
    xl_st.vsi = restore_st->svst_vsi;

    xl_st.last_gss = restore_st->svst_last_gss;
    xl_st.v_size = restore_st->svst_v_size;
    xl_st.character_proportion = restore_st->svst_character_proportion;
    xl_st.user_pref_cset = restore_st->svst_user_pref_cset;
    xl_st.user_pref_repertory = restore_st->svst_user_pref_repertory;

    xl_st.gl_ptr = &xl_st.g_table[restore_st->svst_gl_index];
    xl_st.gr_ptr = &xl_st.g_table[restore_st->svst_gr_index];

    for (i = 0; i < G_TABLE_SIZE ; i++)
       {
	xl_st.g_table[i].repertory = restore_st->svst_repertory[i];
	xl_st.g_table[i].char_set_id = restore_st->svst_char_set_id[i];
       }

    xl_st.requested_attributes = restore_st->svst_requested_attributes;
    xl_st.cur_sgr = restore_st->svst_cur_sgr;

    for (i = 0; i < SGR_TABLE_SIZE; i++)
       {
	xl_st.sgr_tbl[i].font_file = restore_st->svst_sgr_tbl[i].font_file;
	xl_st.sgr_tbl[i].selection_type = restore_st->svst_sgr_tbl[i].selection_type;
        strcpy (xl_st.sgr_tbl[i].id_string, restore_st->svst_sgr_tbl[i].id_string);
       }

    ca_init_pfs(&(restore_st->svst_pfs));


    /* Call the internal parser routines to recompute
     * the current font and other variables, so hai is valid
     */
    compute_proportional_mode();
    compute_spacing(&dummy, &dummy);
    invalidate_font();		    /* NOTE: probably redundant */


    /* The tabs are generated algorithmically if the count is set
     * to "-1", by calling htabs_init and vtabs_init.  This allows the 
     * constant table to not have to worry about the default margins 
     * and other similar issues.
     */
    xl_st.htabct = restore_st->svst_htabct;

    if (xl_st.htabct == -1)
       {
	htabs_init();
       }
    else
       {
	for (i = 0; i < restore_st->svst_htabct; i++)
           {
	    xl_st.htabs[i] = restore_st->svst_htabs[i];
           }
       }

    xl_st.vtabct = restore_st->svst_vtabct;

    if (xl_st.vtabct == -1)
       {
	vtabs_init();
       }
    else
       {
	for (i = 0; i < restore_st->svst_vtabct; i++)
           {
	    xl_st.vtabs[i] = restore_st->svst_vtabs[i];
           }
       }

    xl_st.paper_tray = restore_st->svst_paper_tray;
    xl_st.num_copies = restore_st->svst_num_copies;

    /* 
     *	Get to top left corner 
     */
    hpos_abs(xl_st.h_fmt_bound.min);
    vpos_abs(xl_st.v_fmt_bound.min);

    /* Set the memory in the machine to the factory default.
     * The second parameter is ignored for factory default, using 0.
     * Call the process routines to update the PDLI.
     */
    PROCESS_RESERVE_MEM(MEM_FACTORY_DEF, 0);
    process_set_origin(xl_st.origin);
    process_tray_select(xl_st.paper_tray);
    process_orientation(xl_st.orientation);
    process_set_copy(xl_st.num_copies);
   }


/*****  pr_ris()  *******************************************************
 *									*
 *	pr_ris() - Reset to initial state.  Resets all state variables	*
 *	to the intial values.						*
 ************************************************************************/

VOID pr_ris()
   {
#ifdef DUMP
    oprintf("RIS \n");    
#endif
    ca_restore_default();		/* Return to NVM */
   }


/*****  decstr()  *******************************************************
 *									*
 *	decstr() - Soft Terminal Reset.  Resets all state variables to	*
 *	the intial values.  If Pn = 0, this is the same as RIS.  If     *
 *      Pn = 1, reset from NVM.	 Ignore all other parameter values.	*
 ************************************************************************/

VOID dec_str()
   {
#ifdef DUMP
    oprintf("DECSTR \n");    
#endif
    switch (cp_pbuf[0])
       {
        case 0:
	default:
		ca_restore_default();			/* Invalid parameters are processed as 0 */
    							/* Return to NVM */
		break;
	case 1:	
		ca_set_given_state(&pdl_scl_st[2]);	/* set to conformance level 3 */
		break;
       }
   }


/***************************************************************************
    Request Device Status Report
**************************************************************************/

VOID pr_dsr()
   {
#ifdef DUMP
    oprintf("DSR \n");    
    pprint();		    /* Print the parameter list */
#endif
   }


/***************************************************************************
    Cursor Position Request
**************************************************************************/

VOID pr_cpr()
   {
#ifdef DUMP
    oprintf("CPR \n");    
#endif
   }


/***************************************************************************
    Device Attributes
***************************************************************************/

VOID pr_da()
   {
    WORD i;	    /* loop counter */

#ifdef DUMP
    oprintf("DA \n");    
#endif
    pdli_com_put_string((PUB)&terminal_id[0], FALSE);
    pdli_com_start();

    for (i = 0; i < TERMINAL_ID_SIZE; i++)
       {
	pdli_com_put_byte(terminal_id[i]);
       }

    pdli_com_stop();
   }


/******************************************************************************
    Device Attributes (secondary)
*****************************************************************************/

VOID pr_da2()
   {
#ifdef DUMP
    oprintf("DA2 \n");    
#endif
   }


/*****************************************************************************
    Bell Action Routine
***************************************************************************/

VOID pr_bel()
   {
#ifdef DUMP
    oprintf("BELL \n");    
#endif
   }


/***************************************************************************
    Loading Factory NVR Memory
****************************************************************************/

VOID dec_fnvr()
   {
#ifdef DUMP
    oprintf("DECFNVR \n");    
#endif
   }


/****************************************************************************
    Load Answerback 
******************************************************************************/

VOID pr_ansb()
   {
#ifdef DUMP
    oprintf("ANSB \n");    
    pprint();
#endif
   }


/****************************************************************************
    Load Answerback Numeric
******************************************************************************/

VOID pr_ansb_num()
   {
#ifdef DUMP
    oprintf("ANSB-NUM \n");    
    pprint();
#endif
   }


/****************************************************************************
    Load Answerback Capitals 
******************************************************************************/

VOID pr_ansb_cap()
   {
#ifdef DUMP
    oprintf("ANSB-CAP \n");    
    pprint();
#endif
   }


/****************************************************************************
    Load Answerback Lower Case 
******************************************************************************/

VOID pr_ansb_lc()
   {
#ifdef DUMP
    oprintf("ANSB-LC \n");    
    pprint();
#endif
   }


/*****************************************************************************
    Store the ANSB and Copy the Buffer
*******************************************************************************/

VOID pr_ansb_store()
   {
#ifdef DUMP
    oprintf("ANSB STORE \n");    
#endif
   }


/****************************************************************************
    Business Color Matching Mode
******************************************************************************/

VOID dec_bcmm()
   {
#ifdef DUMP
    oprintf("DECBCMM \n");    
    pprint();
#endif
   }


/****************************************************************************
    Set Presentation Direction
******************************************************************************/

VOID  pr_spd()
   {
#ifdef DUMP
    oprintf("SPD \n");    
    pprint();
#endif
   }


/****************************************************************************
    Printing Density Selection 
******************************************************************************/

VOID dec_den()
   {
#ifdef DUMP
    oprintf("DECDEN \n");    
    pprint();
#endif
   }


/****************************************************************************
    Set Number of Copies 
******************************************************************************/

VOID dec_snc()
   {
    UWORD num_copies;

#ifdef DUMP
    oprintf("DECSNC \n");    
    pprint();
#endif
    num_copies = cp_pbuf[0];	    /* get parameter from parameter list */
    
    if (num_copies == 0)	    /* If zero set to Default of one */
       {
	num_copies = DEFAULT_COPIES;
       }
    else if (num_copies > MAX_COPIES) /* If greater than max, set to max */
       {
	num_copies = MAX_COPIES;
       }

    process_set_copy(num_copies);
   }


/****************************************************************************
    Set Sheet Size
******************************************************************************/

VOID dec_sss()
   {
    SHEET_SIZE decsss;
    LONG length, width;

#ifdef DUMP
    oprintf("DECSSS \n");    
    pprint();
#endif
    process_condshowpg();	    /* dispose of page if has been printed on */

    decsss.tray_num = cp_pbuf[0];   /* get tray num from parameter buf */
    decsss.slot_num = cp_pbuf[1];   /* get slot num from parameter buf */
    width = cp_pbuf[2];		    /* get length of sheet selected */
    length = cp_pbuf[3];	    /* get width of sheet selected */

    /* Get orientation of sheet, landscape or portrait; currently not
     * implemented on the LN04
     */
    decsss.sheet_orient = (UBYTE)(cp_pbuf[4]);

    /*
     * Convert the length and width to centipoints 
     */
    decsss.sheet_width = width * (LONG)xl_st.sizeunit;
    decsss.sheet_length = length * (LONG)xl_st.sizeunit;

    PROCESS_SET_SSIZE(&decsss);
   }


/****************************************************************************
    Set Conformance Level 
	The supported parameter values are 
		    0	reset to default (level 3)
		    71	reset to level 1
		    72	reset to level 2
		    73	reset to level 3
	Any other value is ignored
	

******************************************************************************/	

VOID dec_scl()
   {
#ifdef DUMP
    oprintf("DECSCL \n");    
    pprint();
#endif
    if (cp_pbuf[0] == 0)
    /* If cp_pbuf[0] = 0 then use default of SCL 3 */
       {
	ca_set_given_state(&pdl_scl_st[2]);
       }
    if ((cp_pbuf[0] >=71) && (cp_pbuf[0] <=73))
    /* accept only values between 71 and 73 */
    /* and range them so we can use an array starting at 0 */
       {
	ca_set_given_state(&pdl_scl_st[(cp_pbuf[0] - 71)]);
       }
   }

