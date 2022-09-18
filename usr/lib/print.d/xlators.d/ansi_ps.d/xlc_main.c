#ifndef lint
static char SccsId[] = "  @(#)xlc_main.c	4.1   LPS_ULT_TRN   7/2/90";
#endif
/* file:	xlc_main - Translator entry point
 * created:	laf	 13-FEB-1986 17:09:27
 * edit: 	gh	 26-MAR-1986 09:10:31 Made portable
 *		gh	 10-APR-1986 15:39:53 Added empty_jfy_buf() to 
 *			 EOF handling
 *		laf      17-APR-1986 16:43:51 Added new parser states
 *			 for DCS processing; changed call to "pr_dcs"
 *			 to "pr_dcsseq"
 *		gh	 25-APR-1986 10:37:33 Changed xlate() to do any
 *			 required power-up initialization itself, then to
 *			 call ris().
 *		gh	 4-JUN-1986 11:19:35 Changed xlate() to save 
 *			 item list ptr
 *		gh	 12-JUN-1986 10:14:37 Adding the following just before
 *			 the call to ris() in xlate():
 *				xl_st.h_lim_bound.max = 0x7ffffffe;
 *				enter_jfy();
 *			 since ris() immediately calls empty_jfy_buf();
 *
 *		araj	 24-JUL-1986 08:38:16 Removed 
 *			indirection in calling, since we only 
 *			have one entry point now
 *
 *		araj	 24-JUL-1986 10:51:35 
 *			added include XLM_IO.H
 *
 *		araj	 25-JUL-1986 09:21:02 
 *			Unwind main loop
 *
 *		araj	 25-JUL-1986 10:54:34 
 *			include fast path for printables
 *
 *		araj	25-JUL-1986 11:16:10 
 *			replaced usage of else in main loop 
 *			with continue
 *
 *		araj	 25-JUL-1986 11:34:00 
 *			included input code in main loop
 *
 *
 *		araj	 25-JUL-1986 11:53:55 
 *			added fast path for CR/LF/FF
 *
 *		araj	 25-JUL-1986 15:50:12 
 *			moved process eof in line at end of 
 *			main.
 *
 *			re-spelled the "fast path if"
 *			with more parentheses
 *
 *		nv	 25-JUL-1986 17:09:30 include fast version of pr_text 
 *			in the fast track.
 *
 *		nv	 25-JUL-1986 18:04:48 convert fast version of pr_text
 *			to use gset pointers.
 *
 *		araj	 25-JUL-1986 18:46:07 
 *			improved on above
 *		araj	 25-JUL-1986 19:18:53 
 *			Modified test for empry buffer
 *			Expanded Check_ap
 *			and GET_WIDTH
 *
 *		araj	 25-JUL-1986 20:54:40 
 *			Expanded dispose of char
 *
 *
 *		araj	 25-JUL-1986 21:42:28 
 *			improved on dispose_of char by 
 *			creating a fast path to only change
 *			the active position
 *
 *		araj	 25-JUL-1986 22:46:34 
 *			Expanded PS_SCHAR
 *
 *		araj	 26-JUL-1986 08:54:17 
 *			Changed reference to OPRINTF and PS_STR
 *			into references to TOPRINTF, TPS_STR
 *
 *		araj	 26-JUL-1986 12:08:45 
 *			moved the PS strings used by fast track 
 *			to XLV_PS.
 *
 *		araj	 26-JUL-1986 15:19:58 
 *			changed TPS_STR back now that this is 
 *			the official guy
 *
 *		araj	 26-JUL-1986 15:45:19 
 *			changed TOPRINTF to oprintf, this is now 
 *			the official guy"
 *
 *		nv	 30-JUL-1986 10:39:50 Changed the way gr and 
 *			gl are accessed on account of the changed
 *			declaration of .gl to .gl_ptr and .gr to .gr_ptr.
 *
 *		araj/nv	 30-JUL-1986 11:59:42 
 *			Moved c7 from local to external, so everyone
 *			can use it without having to recompute it.
 *
 *		gh	 31-JUL-1986 12:30:49
 *			Changed the main loop to calculate .ahp+.wid only once
 *
 *		laf	 21-AUG-1986 13:27:39 Added fast path to gr_sixel ()
 *			to the main loop.
 *
 *		laf	 22-AUG-1986 13:30:38 Put "gr_sixel" inline instead
 *			of having it be a subroutine call in the main loop.
 *		laf 	 29-AUG-1986 10:04:32 Took "pcnt=0" and "icnt=0"
 *			out at end of main loop, right after switching on 
 *			the token type.  This was in response to QAR697;
 *			was causing control chars encountered (validly)
 *			in the middle of escape sequences with multiple
 *			parameters to reset the parameter count to 0, which
 *			is incorrect.  Resetting pcnt and icnt is taken care
 *			of by the call to setseq() in pr_esc, pr_dcs, and
 *			pr_cseq.
 *		nv	 29-AUG-1986 15:14:12 Changed the definition of
 *			'max_buff_size' from 'double' to 'LONG'.
 *		nv	 3-SEP-1986 15:12:29 Added call to clean_up_host_memory 
 *			in 'process_eof' to assure that all allocated host
 *			memory is released at the end of the translation job
 *		nv	 16-SEP-1986 15:52:13 Specify the new power_up_flag of 
 *			ris as TRUE.
 *		mgb	 16-SEP-1986 17:51:46 Fix to qar719 - a LF should
 *			cause a FF when avp is equal or greater than bottom
 *			margin a FF is now issued.
 *			Also, took out support for octal representation of
 *			special char.
 *		nv	 1-OCT-1986 13:32:03 changed the fast path in 'main' 
 *			to reflect today's change in 'pr_text'.
 *		mgb	 2-OCT-1986 17:40:42 added below baseline to test if
 *			past bottom margin.
 *		nv	 14-OCT-1986 11:28:14 Moved the check and condtional
 *			correction of C1 to C0 from near the end of the main 
 *			loop to the point immediately after receiving the next
 *			character but leave unactuated.
 *			Simplified the coding of the conditions for entering 
 *			the fast path.
 *		nv	 23-OCT-1986 18:38:15 Remove superfluous reference to 
 *			'above_baseline_table'.
 *		mgb	 10-DEC-1986 11:46:39 added two parameters to check_ap,
 *			(above baseline, below baseline).
 *
 *		araj	 27-MAR-1987 13:47:38 , modified fast path, so 
 *			that no GR code is printed if in 7 bit or 7 bit GL
 *			output mode, to support the two new output modes.
 *
 *		araj	 30-MAR-1987 12:05:29 
 *			added entry point TRN$ANSI_POSTSCRIPT
 *
 *		araj	 30-MAR-1987 12:43:37 
 *			changed to use official names for gl_only, 
 *			gl_gr_only.
 *
 *		mgb	 2-JUL-1987 10:14:02 
 *			put test for sheet length in fast path test.
 *
 *		mgb	 10-JUL-1987 15:35:18 
 *			Added code for set up in power-up initialization
 *			that would not be done by ris().
 *
 *		mgb	 14-JUL-1987 14:18:01 
 *			Added 8 new entry points for preamble/setup
 *			Added switch to print/noprint of preamble called
 *			print_prologue.
 *
 *		mgb	 30-NOV-1987 11:37:55 
 *			Modified the main entry point trn$ansi_ps to handle
 *			preable breakout, setup and form.
 *
 *		mgb	 19-JAN-1988 16:12:51 
 *			Common Parser update - changed parse_init() to
 *			cp_init(). Changed c and c7 to cp_c and cp_c7.
 *
 *		mgb	 27-JAN-1988 16:33:05 
 *			Spilt xlate.h into xlate.h & capdl.hc for the
 *			common parser. Later capdl.def of the common
 *			parser will have capdl.hc.
 *
 *		mgb	 27-JAN-1988 16:34:49 
 *			Spliting xlc_main into 2 modules; xlc_main.c and
 *			cpparse.c. cpparse.c will contain the code that
 *			gets the data. Later cpparse.c will be part of the 
 *			common parser.
 *
 *              kws	 02-MAR-1988 12:53:11
 *			Remove entry points for Dupont and compatibility.
 *
 *              kws	 21-MAR-1988 18:44:24 
 *                      Add check for end of itemlist when checking for
 *                      TRN$ANSI_SPECIAL.
 *
 *              kws	 17-MAY-1988 17:55:17 
 *                      Put in new resource loading code for small job
 *			performance fixes.
 *
 *		mgb	19-MAY-1988 17:45
 *			Took out vm_wasted_and vm_being_used
 *
 *		tep	26 May 1988 11:31 
 *			added call to init_sixel_font_keys() from 
 *			body_trn$ansi_postscript
 *
 *		kws	 6-JUN-1988 21:39:55 
 *			Change name of prologue file for resource loading
 *			to lps_ansi_prologue.ps for ULTRIX compatibility.
 *
 *		tep	15 June 1988 15:28
 *			moved call to init_sixel_font keys() to happen
 *			in ps_init() (in xlc_codegen)
 *
 *		kws	 27-JUN-1988 16:54:25 
 *			Added code to output the prologue and exit
 *			if prologue_select item is passed to Translator
 *
 *		kws	 21-JUL-1988 16:42:01 
 *			Insure that the output buffer is flushed 
 *			when outputting the prologue only.
 *
 *		kws	 18-AUG-1988 12:51:32 
 *			Up prologue version number to v3.0-01.  Define
 *			decbind to be null.
 *
 *		kws	 24-AUG-1988 16:02:37 
 *			Split ps_init () into ps_init_file (),
 *			dispose_prologue (), and ps_init_subjob ().
 *
 *		kws	 2-NOV-1988 20:46:48 
 *			Up prologue version number to v3.1-00.
 *
 *		ejs	 3-NOV-1988 21:15
 *			Up prologue version number to v3.1-07.
 *
 *		araj	 4-NOV-1988 09:05
 *			Changed V to T in version ID
 *
 *		araj	 7-NOV-1988 13:11
 *			up version id to T3.1-08
 *
 *		araj	 7-NOV-1988 13:11
 *			up version id to T3.1-10
 *		
 *		kws	 19-DEC-1988 17:20:07 
 *			Issue a "save" and "restore" around
 *			each subjob.  Temporary fix to the
 *			invalidrestore setup problem where
 *			device control library modules are
 *			loaded between subjobs.
 *
 *	araj	26-JAN-1989 15:32
 *	    Temporary patch to power up code, so that cache limit, 
 *	    sixel font and defpapertray get defined.
 *	    Although they are the only 3 issues at this time,
 *	    the whole scan item and init job/subjob will have to be
 *	    completely revisited.
 *
 *		cp	 20-MAR-1989
 *			Added conditional code around '#include <ssdef.h>'.
 *			Removed keyword 'extern' before variable
 *			item_special_ptr.  Added necessary cast operators
 *			for Ultrix port.
 *
 *		cp	 24-MAR-1989
 *			Changed constant PAPER_TRAY_UNDEF to
 *			TRAY_DEV_DEF for Alain.
 *
 *		araj	 4-APR-1989 10:51
 *			Changed 1 to TRUE and 0 to FALSE
 *			Removed First Time, as it is redundant with
 *			reset entry. The first time we are called,
 *			reset entry better be TRUE or not there 
 *			(in which case it defaults to TRUE)
 *
 *		araj	10-APR-1989 09:09
 *			changed ps_trace_macro to item_trace_macro
 *
 *		ejs	13-APR-1989 10:43
 *			last_showpage is now processed regardless of
 *			reset on entry.
 */ 


/************************************************************************
 *                                                                      *
 *      COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,            *
 *            1986, 1987, 1988, 1989 ALL RIGHTS RESERVED.               *
 *                                                                      *
 *      THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE           *
 *      USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF           *
 *      SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE           *
 *      COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES           *
 *      THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE           *
 *      AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND           *
 *      OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.           *
 *                                                                      *
 *      THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE           *
 *      WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A            *
 *      COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.                  *
 *                                                                      *
 *      DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR           *
 *      RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT           *
 *      SUPPLIED BY DIGITAL.                                            *
 ************************************************************************/


#include "dbug.h"
#include "portab.h"
#include "trn$.hc"
#include "xlate.h"
#include "xlm_io.hc"
#include "capdl.hc"
#include "pdl_pdli.hc"
#include "xlm_codegen.h"
#include "xlc_codegen.hc"
#include "xlc_iface.hc"
#include "xlc_ps.hc"
#include "xlc_main.hc"

#ifdef CVAX
#include <ssdef.h>
#else
#define SS$_NORMAL 0
#endif


/* These variable definitions are here because of a flaw in the maketool
 *	it does not allow "$" in the names
 */


NOSHARE GLOBAL TRN$K_ANSI_SPECIAL *item_special_ptr;		/* Item special pointer */



/************************************************************************
 *
 * This module contains the following routines:
 * -------------------------------------------
 * trn$ansi_ps ()
 * process_eof ()
 *
 ************************************************************************/

/* 
 * The main body of the translator is an ANSI parser.  (See the "xlpars"
 * module for description of the parser.)  Outgoing characters are sent
 * to the output as they are received.  PostScript commands are
 * embedded in the output stream by the translator where necessary.
 *
 */




VOID process_eof()
{
	dispose_eof();
	pdli_clean_up_host_memory ();
}



/***************************************************************************
 * In order to print a given file, the Translator may be called multiple
 * times.  Once for each of the setup modules that may be used with the
 * file, and once for the file itself.  Each invocation of the Translator
 * is called a subjob.  The Translator state is kept static across subjobs.
 * The symbiont tells the Translator whether there are multiple subjobs for
 * a particular file through use of the reset_entry and reset_exit bits.
 * The normal file initializations are done at the beginning of the first
 * subjob and at the end of the last subjob.  The entire process is called
 * the file.
 ***************************************************************************/
/*----------*/
trn$ansi_ps (getr, user_arg_g, putr, user_arg_p, itemlist)
/* 
 * ANSI Translator entry point.  This routine consists of the main processing
 * loop of the Translator.  The main loop continues until an EOF 
 * for the subjob is received.
 */
int     (*getr)();
int	user_arg_g;
int     (*putr)();
int	user_arg_p;
ITEM	*itemlist;

{
WORD	i;

    /* Set default state.  Reset on entry and send preamble
     * with the job.                      
     */

    cg_st.reset_entry = TRUE;
    cg_st.reset_exit = TRUE;
    cg_st.print_prologue = TRUE;
    cg_st.cgst_last_showpage = TRUE;

    /* Initialize I/O		*/

    iface_init (getr, user_arg_g, putr, user_arg_p);

    /* Save the item list pointer */

    cg_st.cgst_ilistptr = itemlist;			

    /* Check the itemlist for the ANSI Special, resource loading, and
     * emit prologue items to determine whether we are processing or have 
     * processed a setup module, whether a prologue should be sent with the
     * job or whether just the prologue should be output.
     */                                 

    while (cg_st.cgst_ilistptr->code != trn$_end_of_list)  {
	switch (cg_st.cgst_ilistptr->code)  {

	    case trn$_page_fragment:
    		switch (cg_st.cgst_ilistptr->address) 
		   {
    		    case trn$k_multi_page:
    			cg_st.cgst_last_showpage = TRUE;	break;
    		    case trn$k_page_fragment:
    			cg_st.cgst_last_showpage = FALSE;	break;
    		   }
		ITEM_TRACE_MACRO("(Got a page fragment item value of %d) print flush\n",cg_st.cgst_ilistptr->address);
		break;

	    case trn$_ansi_special:

		/* Save address of ANSI special item.              */

		item_special_ptr = (TRN$K_ANSI_SPECIAL *) cg_st.cgst_ilistptr -> address;

		/* Check to see if this is a setup module.  If so, don't clean
		 * up Postscript state at the end of a job.  If a setup module
		 * has already come down don't reset Translator state at the
		 * beginning of the job.
		 */

		if (item_special_ptr -> reset_entry)
		    cg_st.reset_entry = TRUE;
		else
		    cg_st.reset_entry = FALSE;

		ITEM_TRACE_MACRO(" (Got a reset entry item value of %d) print flush\n",item_special_ptr->reset_entry);

		if (item_special_ptr -> reset_exit)
		    cg_st.reset_exit = TRUE;
		else 
		    cg_st.reset_exit = FALSE;

		ITEM_TRACE_MACRO("(Got a reset exit item value of %d) print flush\n",item_special_ptr->reset_exit);

    		break;

    	    case trn$_prologue_select:	
    		
    		if (cg_st.cgst_ilistptr ->address == trn$k_emit_prologue_only) {

		ITEM_TRACE_MACRO("(Got a emit prologue only item value of %d) print flush\n",cg_st.cgst_ilistptr ->address);

    		    /* Set output mode to seven bit gl.	*/

		    cg_st.cgst_output_mode = trn$k_gl_only;

    		    /* Output the prologue.	*/

		    dispose_prologue (DECBIND_NULL); 

		    /*
		    * Flush any remaining data from the output buffer
		    */
	
		    ps_flush ();

    		    /* Exit the Translator.   */
		    return(SS$_NORMAL);
    		   }
	    case trn$_resource_table:

    		/* Check the resource table for the version number of
                 * the prologue that has been previously loaded into
                 * the PS machine.  The resource table consists of a
                 * number of entries.  A byte at the beginning of the
                 * table specifies the number of entries in the table.
    		 * Each entry contains:
                 *
                 *    1. Length field for resource type (1 byte)
                 *    2. Resource type (max 32 bytes)
                 *    3. Length of resource name (1 byte)
                 *    4. Resource name (max 255 bytes)
                 *    5. Length of resource version number (1 byte)
                 *    6. Resource version number (max 32 bytes)
                 */
   
		/* Save address of resource table.              */

		resource_table_ptr = (BYTE *) cg_st.cgst_ilistptr -> address;

    		/* Get the number of entries in the resource table */

    		resource_table_entries = *resource_table_ptr++;
		ITEM_TRACE_MACRO("(Got a resource table size of %d) print flush\n",resource_table_entries);



    		/* Move pointer to resource type and put it
                 * into a null terminated string. 
                 */

    		resource_type_length = *resource_table_ptr++;
		ITEM_TRACE_MACRO("(Got a resource type length %d) print flush\n",resource_type_length);

    		for (i=0; i < resource_type_length; i++) 
                    {
    		    resource_type [i] = *resource_table_ptr++;
    		    }
    		resource_type [i] = '\0';

		ITEM_TRACE_MACRO("(Got a resource type: %s) print flush\n",resource_type);


    		/* See if resource type is a prologue.  If so continue
                 * through entry to check for resource name.  NOTE:
                 * since strcmp returns a zero if there is a match,
                 * the conditionals use the logical negation.
                 */

                if (!strcmp(resource_type, ansi_resource_type)) 
    		   {
		    /* Put resource name into a null terminated string. */

		    resource_name_length = *resource_table_ptr++;
		    ITEM_TRACE_MACRO("(Got a resource name length %d) print flush\n",resource_name_length);


		    for (i=0; i < resource_name_length; i++) 
			{
			resource_name [i] = *resource_table_ptr++;
			}
		    resource_name [i] = '\0';

		    /* See if resource name is for an ANSI prologue.  
		     * If so get version number of prologue.
		     */
		    ITEM_TRACE_MACRO("(Got a resource name: %s) print flush\n",resource_name);


		    if (!strcmp(resource_name, ansi_resource_name)) 
		       {
			/* Put resource version number into a null 
                         * terminated string. 
                         */

			resource_vers_num_length = *resource_table_ptr++;
			ITEM_TRACE_MACRO("(Got a resource version length %d) print flush\n",resource_vers_num_length);


			for (i=0; i < resource_vers_num_length; i++) 
			    {
			    resource_vers_num [i] = *resource_table_ptr++;
			    }
			resource_vers_num [i] = '\0';


			ITEM_TRACE_MACRO("(Got a resource version mumber: %s) print flush\n",resource_vers_num);


    			/* Check to see if version number of the preloaded
                         * prologue matches the version number of the 
                         * prologue associated with this version of the
                         * Translator.  If they match, set a bit informing
                         * xlc_codegen to not send a prologue down with
                         * this job.
                         */

			if (!strcmp(resource_vers_num, ansi_resource_vers_num)) 
    			    cg_st.print_prologue = FALSE;
    			}
    		   }
    	   }
	cg_st.cgst_ilistptr++;
	}
    
    /* Reset the item list pointer to again point to the top of the 
     * item list 
     */

    cg_st.cgst_ilistptr = itemlist;			

    /*********************************************************
     *
     * To fix the invalidrestore error with setup modules a
     * "save" and "restore" is done around each subjob.  In
     * addition the prologue must be sent with each subjob.
     * The Translator must return to BAREUNIT at the end of
     * subjob because device control libraries loaded between
     * the first and second subjob create a state that causes
     * an invalidrestore.  This is a temporary fix.  Eventually
     * the Translator should only restore back to BAREUNIT on
     * the final subjob.
     *
     * Code that was commented out below has been moved here.
     * When the final complete fix is implememted the code
     * will be deleted from here and returned to below.
     *
     *********************************************************/
       
    ps_str ("save\n"); /* Output initial "save"	*/

    /*
     * Send down the prologue if it has not been previously
     * loaded.
     */

    if (cg_st.print_prologue) 
	{
	dispose_prologue (DECBIND);
	}

    /*
     * Initialize the Translator and PS machine, and internal Translator
     * PS state for the file.  Send the prologue down if neccessary.
     */

    if (cg_st.reset_entry)
        {

    	/* Initialize PS state for the entire file.	*/

    	ps_init_file ();

    	/* Read the itemlist for default values.	*/

	scan_items(cg_st.cgst_ilistptr);

    	/* Set input routine to ANSI			*/

	cg_st.cpinit_vals.host_ptr = ansi_input; 

    	/* do powerup reset */

	pdl_init(&cg_st.cpinit_vals); 

        }

    /* Initialize this subjob.	*/

    /* Set VM available in PS machine.		*/

    cg_st.cgst_vm_available = cg_st.cgst_vm_size;

    /* Initialize paper tray values.		*/

    cg_st.cgst_paper_tray = TRAY_DEV_DEF;

    ps_init_subjob ();

    /*
     * Reset the output paper tray.
     */

    process_tray_select(cg_st.cgst_paper_tray);

/*
 * We are now ready to accept data.  Call the parser to read the data stream
 * and begin translation.
 */

 	cp_parse ();
 


    /* Finish processing the subjob.	*/

    cp_eof(cg_st.cgst_last_showpage);	    /* terminate parsing of file */
					    /* if needed, reset ap */

    dispose_end_of_subjob ();

    /* Finish processing the file if neccessary. */

    if  (cg_st.reset_exit)		    /* if needed, terminate parsing of job */
	{		    
	    cp_eoj();
	    process_eof ();
	}
	
    /*
    * Flush any remaining data from the output buffer
    */

    ps_flush ();

    /* Exit the Translator.   */

    return(SS$_NORMAL);


}	/*End of translator*/

