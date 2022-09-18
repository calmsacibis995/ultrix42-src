#ifndef lint
static char SccsId[] = "  @(#)cainit.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cainit.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *	1988, 1989 ALL RIGHTS RESERVED.
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
 *  15-FEB-1988 11:37 mhw
 *	Initial version
 *
 *  25-FEB-1988 16:03 mhs
 *	Typecast arithmetic expression for proper compile.
 *
 *   6-APR-1988 16:10 mhs
 *	Move htabs_init and vtabs_init to catabs.c.
 *
 *   9-NOV-1988 16:29 araj
 *	Changed default for hai, vsi, tray and num_copies to use
 *	SCL level 3 table values instead of hradcoded numbers.
 *	Did not dare go further because of the time pressure
 *	forfear of introducing bugs, but this needs to be completed
 *	when the atmosphere is more relaxed.
 *
 *  18-DEC-1988 15:19 ejs
 *		Major changes to data structutes.  GLYPH has been modified
 *		to allow struct copies more easily.  The PS output is not
 *		changed (barring bugs of course).
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
 *   Filename:  cainit.c
 *
 *
 *   This module includes
 *
 *   parse_init()
 *   ca_state_init()
 *   reset_pitch ()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */


/* file:	xlc_init - Translator initialization routines
 * created:	gh	 26-MAR-1986 09:10:31
 * edit:	gh	 27-MAR-1986 08:41:50 Fixing default horiz. tab stops
 *		gh	 7-APR-1986 13:52:30 Changing default itemlist PFS vals
 *		gh	 10-APR-1986 13:47:44 Fixed bug in (),
 *			 added ps state initialization in state_init.
 *			 Added xl_st.orientation initialization in state_init()
 *		gh	 23-APR-1986 08:19:59 Changing state_init() because
 *			 XLATE.H structures changed.
 *		gh	 24-APR-1986 09:08:14 Changed SGR structure name
 *			 Changing state_init to not init PS variables, but to
 *			 call ps_init(). Adding last_showpage init.
 *		gh	 29-APR-1986 10:12:10 Adding xl_st.sheet_len itlzation
 *		gh	 1-MAY-1986 10:43:07 Changing default states of
 *			 lfnl and ssu modes to FALSE and DECIPT.
 *		gh	 3-JUN-1986 15:01:50 Item list selecting landscape
 *			 mode only sets orientation - scan_items should return
 *			 value of 21 - changed that.
 *		gh	 13-JUN-1986 08:01:58 Changing the initialization of
 *			 .plf
 *		nv	 27-JUN-1986 15:03:30 Renamed 'xl_st.curchar.attrs' to
 *			'xl_st .requested_attributes.'
 *		nv	 10-JUL-1986 17:47:49 Incorporated new function called
 *			reset_pitch ().
 *			Revised the way xl_st .propl_mode is initialized.
 *		nv	 12-JUL-1986 08:13:22 Removed the initialization of 
 *			the obsolute variables 'opm' and 'pcf'.
 *			Initialized 'psel_mode'.
 *			Removed redundant initialization in reset_pitch
 *			of .character_proportion and .cur_sgr.
 *			Added comments.
 *		laf	 30-JUL-1986 10:36:02 Removed reference to unused
 *			variable "pdef"
 *		laf	 27-AUG-1986 14:29:07 Added initz'n of xl_st.
 *			curchar.ul_avp to state_init().
 *
 *		araj	 7-OCT-1986 17:22:00 added 
 *			initialization for max_bound
 *		nv	 17-OCT-1986 11:23:24 Remove all reference to 'hsi'.
 *		nv	 21-OCT-1986 17:02:49 Remove initialization of
 *			'xl_st .shorp' from 'reset_pitch'; it is initialized
 *			in 'state_init'.
 *			Remove initialization of 'xl_st .vai' from
 *			'reset_pitch'; it is calculated as a result of the 
 *			call to 'compute_font'.
 *			Changed the initialization of 'xl_st .vai' and
 *			'xl_st .hsi' to be semi-proportional.
 *		nv	 22-OCT-1986 09:59:29 Redesigned 'htabs_init' and 
 *			'vtabs_init'.
 *		nv	 22-OCT-1986 13:40:06 Added current sgr initialization 
 *			with pfs initialization in 'scan_items' to make the
 *			initial sgr paper size and orientation dependent. 
 *		nv	 21-NOV-1986 17:58:25 In 'scan_items' initialized and 
 *			conditionally set 'xl_st .vm_size'.
 *		mgb	 13-JAN-1987 16:01:09 Adding option to have sixels
 *			converted into hex or binary output. Routine that was
 *			modified was scan_items().
 *		mgb	 13-JAN-1987 16:04:24 removed parameted list from
 *			state_init() since it is not used.
 *		kws      13-FEB-1987 15:16:19 Added B size and legal paper
 *			support.
 *		kws	 13-MAR-1987 15:17:08 Select SGR 10 by default with
 *                      B size paper.
 *
 *		araj	 27-MAR-1987 20:41:29 added support for 2 new 
 *			output modes. 
 *
 *		araj	 30-MAR-1987 12:40:37 
 *			changed to use official names for the 2 new 
 *			output modes
 *
 *		araj	 1-APR-1987 13:39:38 
 *			Modified to swap height and width when landscape 
 *			is selected.
 *
 *              kws	 13-MAY-1987 16:14:26 
 *			SPR ICA-04651.  Format page for A4 landscape 
 *			when selected from print command.  Page is
 *                      currently formatted for A landscape.
 *
 *		mgb	 7-JUL-1987 13:31:43 
 *			Put break in case statement that sets VM size.
 *			If you send VM size you would set 7 bit also which
 *			is not good.
 *
 *			Also added #if SYMBIONT_DEBUG prints.
 *
 *		araj	 16-JUL-1987 20:23:44 
 *			Attempt to fix  LPS40 QAR 144, by undoing Keith's
 *			13-may change to termmanag, and moving the 
 *			default initialization of SGR to here. Added 
 *			defaulting to SGR of 1 if no paper size/orientation
 *			is passed to us.
 */



#define cainit (1)

/* INCLUDE files */

#include "portab.h"		/* system-wide global defs */
#include "cpsys.hc"		/* common parser global defs */
#include "camac.lib_hc"		/* macros for caxxx files */
#include "caglobal.hc"		/* globals for caxx files */


/*----------*/

VOID ca_state_init()
   {
    /*
     * Initialise "translator state" (ALL VARIABLES ARE STORED IN
     * UNITS OF "CENTIPOINTS", where a centipoint is 1/7200th of an inch) 
     * The xl_st structure is initialised in the order in which it is
     * declared.  Any special processing is handled by reinitialising
     * after exiting this routine.
     */

    setorigin(FALSE);

    xl_st.curchar.char_data.char_code = NUL;  /* default to NUL character */
    xl_st.curchar.char_data.char_font= 0;
    xl_st.curchar.char_data.char_width = 0;
    xl_st.curchar.ap.xval = 0;	/* default to (0,0) */
    xl_st.curchar.ap.yval = 0;	/* default to (0,0) */
    xl_st.curchar.attr_data.attr_baseline_offset = 0;
    xl_st.curchar.font_data.above_baseline_offset = 0;
    xl_st.curchar.font_data.below_baseline_offset = 0;
    xl_st.curchar.font_data.algorithmic_attributes = 0;
    xl_st.curchar.font_data.horizontal_spacing = 0;

    xl_st.hai = 720;		/* used to force proper spacing in call to
				   compute_font_for_g_set */
    xl_st.vai = xl_st.vsi = 0;
    xl_st.vai_valid = TRUE;
    xl_st.lfnl_mode = FALSE; 	/* (ANSI escape sequence sets this) */
    xl_st.crnl_mode = FALSE;	/* (DEC Private Sequence sets this) */
    xl_st.wrap_mode = TRUE;
    xl_st.psp = FALSE;
    xl_st.c1rcv_mode = CP_8BIT_MASK;

    xl_st.psel_mode = FALSE;
    xl_st.shorp = 0;

    xl_st.pum_mode = PUM_CHAR;	/* position unit is "character"	*/
    xl_st.sizeunit = DECIPT;	

    /* Clear # of entries in justify buf - note that the actual justify buffer
     * doesn't need to be cleared if the counter is zero, as the counter points
     * to the NEXT position and there's nothing to the left of zero to be read
     * accidentally by a routine 
     */
    xl_st.justify_mode = JUSTIFY_OFF;
    xl_st.limits_enabled_flg = FALSE;
    xl_st.jfy_buf_index = 0;	/* next empty space in justify buffer */

    xl_st.plf = NOPLUPLD;
    xl_st.limit_flag = FALSE;
    xl_st.rmf = FALSE;		/* flag that right margin is not exceeded */
    xl_st.fcf = TRUE;		/* flag the first character on the page */

    xl_st.propl_mode = MONO_SPACED_MODE;

    /*
     * Initialise the SGR variables.
     *
     * NOTE: We probably should do range-checking on the
     *       initial SGR index.
     */
    xl_st.requested_attributes = 0;
    xl_st.cur_sgr = pdli_init_st.initial_sgr; /* Default SGR is Courier */

    /* Initialise the G tables, SGR tables, NRCS tables */
    ca_font_init();

    /* Set hai, vai and vsi to semi-proportional mode and select a 
     * Portrait font (use values directly out of level 3 default table
     */
    xl_st.hai = pdl_scl_st[2].svst_hai;	
				/* used to force proper spacing in call to
				   compute_font_for_g_set */
    xl_st.vai = xl_st.vsi = pdl_scl_st[2].svst_vsi;
    xl_st.vai_valid = TRUE;

    /*
     * Effect a pfs() to set the margin, origin, orientation, and 
     * active position (Default PFS is Private A-size Portrait).
     */

     do_pfs(&pdli_init_st.initial_pfs);

    /* Init tab tables (hai and vai must already be set at this point)
     */
    htabs_init();
    vtabs_init();

    /* Initialise the graphics vars */
    ca_graphics_init();

    /* use level 3 defaults, directly out of the default table */

    xl_st.paper_tray = pdl_scl_st[2].svst_paper_tray;
    xl_st.num_copies = pdl_scl_st[2].svst_num_copies;
   }


/* Restore to default routine */

VOID ca_restore_default ()
   {
    /*
     * Check to see if NVM Recall will work.
     * If it will, set to NVM, otherwise set
     * to Conformance Level 3.
     */
    if (   (pdli_nvm_get(&nvm_st, S_NVM_ST))
	&& (! (strncmp(&VERSION[0], &nvm_st.svst_version_id[0],S_VERSION_ID)))
       )
       {
	ca_set_given_state(&nvm_st);	    /* set xl_st to NVM */
       }
    else
       {
        /*
	 * Set to conformance level 3 -
	 * NVM Recall didn't work 
	 */
	ca_set_given_state(&pdl_scl_st[2]); /* set xl_st to Level 3 */
	ca_nvm_store();			    /* clean up the NVM */
       }
   }

