#ifndef lint
static char SccsId[] = "  @(#)xlc_codegen.c	4.1   LPS_ULT_TRN   7/2/90";
#endif
/* file:	xlc_codegen.c - Translator code generator routines
 * created:	laf     13-FEB-1986 14:22:08 
 * edit:	gh	18-MAR-1986 16:42:23 - major changes were glyph 
 *			passing to dispose_of_char, and PORTAB.H stuff
 *		mb	18-APR-1986 17:42:07 - major changes to entire 
 *			file, added new routines and modified existing 
 *			routine to meet the design spec.
 *		mb	23-APR-1986 13:41:07 - added init of ps states 
 *			for codegen
 *		gh	14-MAY-1986 12:38:13 Updating oprintf () to output
 *			8-bit chars as octal codes
 *		laf	25-JUN-1986 19:51:13 Added output of FontBBox and
 *			FontMatrix to output_font
 *		laf     26-JUN-1986 10:03:02 Took out dispose_font 
 *			routine and created xlc__dll.c module
 *		laf     2-JUL-1986 10:50:15 Re-created the routine 
 *			"dispose_font" in this module, and made it call
 *			"dll", which is the routine that lives in xlc_dll.
 *			This change was made to avoid conflicts with macros
 *			between the two modules in the future.
 *
 *		araj	 2-JUL-1986 14:47:34 
 *			added a print string to close the 
 *			translator dictionary upon EOF
 *
 *		araj	 3-Jul-1986
 *			move the code that defines DLL fonts into 
 *			XLC_DLL, and replaced  it with a call to
 *			DEFINE_DLL_FONTS();
 *		nv	 18-JUL-1986 15:29:03 Add provisions for dispose_font
 *			passing another parameter to dll.
 *
 *		araj	 26-JUL-1986 15:21:26 
 *			Renamed PS_STR to OLD_PS_STR
 *			Renamed TPS_STR to PS_STR
 *
 *		araj	 26-JUL-1986 15:42:14 
 *			renamed oprintf to old_oprintf
 *			renamed toprintf to oprintf
 *			changed open font to use old_oprintf
 *			as the string it outputs uses a %% to
 *			output a PS comment
 *		gh	 29-JUL-1986 14:36:11 Added the output of a <ff>
 *			char right after the preamble is output
 *
 *		araj	 30-JUL-1986 20:08:13 
 *			Redesigned and streamlined output sixel
 *		gh	 31-JUL-1986 14:28:18 
 *			Changed declaration of obuf_sum to UBYTE pointer
 *
 *		gh	 11-AUG-1986 13:30:55
 *			Adding 'include dbug.h'.  Deleted M$OUTPUT_PREAMBLE
 *			from XLM_CODEGEN.H and placed code in ps_init with
 *			preprocessor condition around the outputting of err hand
 *		mgb	 20-AUG-1986 11:19:15 Adding sheet length to current
 *			ps state.
 *		nv	 3-SEP-1986 10:15:46 Added 'font_table_entry' 
 *			parameter to dispose_font.
 *		mgb	 4-SEP-1986 15:47:51 Added wipe_all and 
 *			dispose_replace_all to handle dll.
 *		nv	 5-SEP-1986 19:00:36 Removed boxnumber from
 *			'dispose_restore_all'.
 *		mgb	 16-SEP-1986 17:55:30 Take out support for
 *			octal representation of special char.
 *		mgb	 26-SEP-1986 18:13:25 move imagepic from preamble
 *			to string and added code to output it.
 *		nv	 7-OCT-1986 14:51:22 Added 'dispose_of_error' function.
 *			Added type_of_error_condition as external variable.
 *		mgb	 4-NOV-1986 13:43:37 Changed test in dispose of eof 
 *			from & to a && and set wpf to true in dispose of 
 *			sixels.
 *		mgb	 13-JAN-1987 15:21:13 added changes to make sixel
 *			output in hex or bin an option during convert time.
 *			Added code in output of the preamble to output the
 *			right PS code for selected options and made two
 *			subroutines to covert sixels, one to hex and the other
 *			to binary. The call to output sixel is now done 
 *			indirectly to one of these routines.
 *
 *		araj	 27-MAR-1987 13:06:15 
 *			Added support for two new output modes
 *			GL only, and true 7 bit
 *			We can now output in
 *				Full 8 bit binary
 *				7 bit C0/GL
 *				7 bit GL
 *				8 bit GL/GR only
 *			Note in current implementation, 7 bit C0/GL
 *			and 7 bit GL are the same.
 *
 *		araj	 27-MAR-1987 13:36:02 
 *			removed tps_char and old_ps_str that 
 *			were unused.
 *
 *		araj	 27-MAR-1987 21:34:10 
 *			added TRN$.H in list of include
 *
 *		araj	 30-MAR-1987 12:42:59 
 *			changed to use official names for gl_only
 *			gl_gr_only
 *
 *		araj	 3-APR-1987 23:55:10 
 *			modified dispose_of_error, to also 
 *			do a printf of the error message
 *			if withoutput is enebled
 *
 *		araj	 3-APR-1987 23:56:00 
 *			modified dispose of set paper tray to reset the VM counters
 *			when doing a wipe all.
 *			This was causing a miscount of the fonts, and the undue rejection
 *			of some, as the counter was not reset when the fonts were cleared, 
 *			but was incremented when they were restored.
 *
 *		mgb	 10-JUL-1987 20:10:51 
 *			Took print string of imagepic hex and bin and put them
 *			in the preamble. After printing preamble the string
 *			TRN$XLATE_DICT begin is printed becuse if in no 
 *			preamble mode it will be needed.
 *
 *		mgb	 14-JUL-1987 14:10:20 
 *			added switch to print/noprint preamble
 *
 *		mgb	 3-DEC-1987 13:56:51 
 *			Added test to dispose_eof() that will end the 
 *			file at bareunit +1 (entry level) for form opposed
 *			going back to base level.
 *
 *		mgb	 7-JAN-1988 16:22:43 
 *			took out all the oprintf and print string stuf and
 *			put them in xlc_iface.
 *
 *		kws	 24-MAY-1988 13:51:02 
 *			Increase ps_st from 6 elements to 8 to mask out
 *			PostScript context stack bug.
 *
 *		mgb	25-MAY-1988 13:51
 *			renamed all symbols with font_table... to 
 *			vax_font_table...
 *
 *		mgb	25-MAY-1988 13:52
 *			changed ps_font... to paired_font...
 *
 *	mgb	25-MAY-1988 13:52
 *		Took out vm_wasted and vm_being_used
 *
 *	mgb	25-MAY-1988 13:52
 *		Took out paired_font_table [] since it is no longer needed
 *
 *	mgb	25-MAY-1988 13:52
 *		Took out vm_wasted and vm_being_used. Replaced them with
 *		vm_available
 *
 *	tep	26 May 1988 10:33 - added routine output_sixel_as_font()
 *
 *	mgb	 6-JUN-1988 14:23 - added spacing to font cache 
 *		( dispose_add_spacing_to_cache () )
 *
 *	mgb	 6-JUN-1988 14:51 - added spacing level to PS machine
 *
 *	tep	15 June 1988 15:35 - finished changes to do sixelfont
 *
 *	kws	 29-JUL-1988 16:36:39 Change references to boxnumber in
 *		dispose_of_char, dispose_add_spacing_to_cache, &
 *		dispose_cache_spacing to spaced_boxnumber for spaced
 *		fonts and unspaced_boxnumber for fonts that do not
 *		need their spacing changed.
 *  
 *	ejs  	 1-SEP-1988 18:54
 *		Modified references to cur_ps_st and vax_font_table.  Added
 *		call to font_init.
 *
 *	kws	 18-AUG-1988 12:54:06 Define decbind before outputting
 *		prologue.
 *
 *	kws	 19-AUG-1988 15:06:28 Output setpacking and setcachelimit
 *		in ps_init.  This was originally part of the prologue.
 *
 *	kws	 23-AUG-1988 14:59:47 Split ps_init () into ps_init_file (),
 *		dispose_prologue (), and ps_init_subjob ().
 *
 *	kws	 24-AUG-1988 17:31:11 Split dispose_eof into dispose_eof (),
 *		and dispose_end_of_subjob ().
 *
 *	kws	 6-SEP-1988 15:24:58 Keep track of VM used for downloaded
 *		and spaced fonts.  Recover VM for spaced fonts if needed.
 *
 *	kws	 23-SEP-1988 16:14:08 Create spaced fonts from built-in
 *		font dictionaries for built-in fonts and from the downloaded
 *		font for DLL fonts.
 *
 *	kws	 23-SEP-1988 17:48:58 Have dispose_tray_select save back
 *		to page level after outputting setpapertray operator.
 *
 *	araj	 9-NOV-1988 16:45 Add processing for default tray is process
 *		tray select
 *
 *	ejs	 8-DEC-1988 12:44 Modified the process_char to detect AP
 *		changes and take a shortcut.
 *
 *	ejs	 8-DEC-1988 22:51 algorithmic attributes are filtered when the
 *		glyph is received in process_char.  This will relieve the
 *		thrashing that currently occurs when an attribute is passed 
 *		that can not be processed (every character faults).
 *
 *		Also compressed the restore and the moveto in the shortcut.
 *
 *	araj	 8-DEC-1988 19:21   remove restore_all and copy_font,
 *		they have been unused for quite a while
 *
 *	araj	 8-DEC-1988 19:22   changed dispose_delete_font_from_cache
 *		to dispose_delete_box_from_cache, as the intent was
 *		to destroy a box, not a font, to destroy the box,
 *		we would find the font in it, then destroy the box containing
 *		the font. Now the code is straigh forward, given a box_number, 
 *		delete the box
 *
 *	araj	 8-DEC-1988 19:24   Changed wipe_all to call invalidate_space_cache
 *		instead of duplicating the code. Same with delete_box_from_cache
 *
 *	araj	 8-DEC-1988 19:25   Added a new function, dispose_delete_font,
 *		to be used by the font dictionary when it deletes a font, so the 
 *		entry in the cache can be re-used
 *	
 *  18-DEC-1988 15:19 ejs
 *		Major changes to data structutes.  GLYPH has been modified
 *		to allow struct copies more easily.  The PS output is not
 *		changed (barring bugs of course).
 *
 *	kws	 22-DEC-1988 18:14:39 
 *		Have a "save" and "restore" around each subjob.  This
 *		is a temporary fix for the invalidrestore errors 
 *		received when device control library modules are sent
 *		between the first and second subjobs.
 *
 *	araj	29-DEC-1988 14:56
 *		Temporarily disabled the optimization in vchar that turns
 *		vchar_space of the peoper width into normal spaces.
 *		The proper width is that of the (possibly) respaced
 *	    PS font, not that of the original dlled font.
 *
 *	araj	26-JAN-1989 15:32
 *	    Temporary patch to power up code, so that cache limit, 
 *	    sixel font and defpapertray get defined.
 *	    Although they are the only 3 issues at this time,
 *	    the whole scan item and init job/subjob will have to be
 *	    completely revisited.
 *
 *	cp	20-MAR-1989
 *	    Changed NULLPTR to NULL.  Removed extraneous '&' operators.
 *	    Removed keyword 'extern' before variables 'paper_tray' and
 *	    'next_element_for_swap' for Ultrix port.
 *
 *	cp	24-MAR-1989
 *	    Integrated Alain's changes.  Function process_tray_select()
 *	    replaces dispose_tray_select().  Variable cg_st.cgst_paper_tray
 *	    replaces paper_tray.
 *
 *	ejs 13-APR-1989 13:13
 *	    Added exec_newsheet to dispose_end_of_subjob.
 *
 *	cp 14-APR-1989 21:38:35 
 *	    Changed ps_st.psst...tray to cur_ps_st->psst...
 *
 *	araj	20-APR-1989 20:38
 *	    Ensured that we would not issue a scale command of 0,
 *	    even when we have very very thin pixels
 *
 *	cp	24-APR-1989 18:48
 *	    Removed extraneous definition of NULLPTR.
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



#include "portab.h"
#include "capdl.hc"
#include "xlate.h"
#include "dbug.h"
#include "trn$.hc"
#include "xlc_font_dictionary.hc"
#include "xlc_font_init.hc"
#include "xlm_codegen.h" 
#include "xlc_codegen.hc" 
#include "xlc_graph.hc"
#include "xlm_io.hc"
#include "xlc_ps.hc"
#include "xlc_iface.hc"

#define SPACE (0x20)
/************************************************************************/
/*
 * This module includes the following routines:
 * -------------------------------------------
 * oprintf ()
 * convert10 ()
 * convert16 ()
 */
/************************************************************************/

/******************************************************************
 *
 * PRIMARY ACTION ROUTINES, DISPOSE_XXX  
 * 
 *      ps_init_file () 
 *      ps_init_subjob () 
 *	dispose_prologue ()
 *      dispose_showpage () 
 *      dispose_condshowpg () 
 *      dispose_eof ()  
 *	dispose_end_of_subjob ()
 *      dispose_of_Char (glyph_ptr)  
 *      dispose_vchar (glyph_ptr) 
 *      dispose_decvec (decvec_ptr) 
 *      dispose_sixel (sixel_ptr) 
 *	dispose_font (boxnumber,left_half_is_empty)
 *      dispose_copy_font (destination_buffer,source_buffer) 
 *      process_tray_select ()
 * 
 *******************************************************************/

/******************************************************************
 * 
 * SECONDARY ACTION ROUTINES, DISPOSE_XXX  
 * 
 *      save ()
 *      restore ()
 *      check_state_change (object_type)
 * 
 *      set_font (font)
 *      set_spacing ()
 *      set_open ()
 *      set_ap ()
 *      set_origin ()
 *      set_orient ()
 *      set_scale ()
 *      set_buff_size ()
 *      set_thickness ()
 * 
 *      open_show ()
 *      close_show ()
 *      open_font (boxnumber) *** (TBD) *** 
 *      close_font () *** (TBD) ***  
 *      open_sixel ()
 *      close_sixel ()
 *      open_decvec ()
 *      close_decvec ()
 *      close_all ()
 * 
 *      output_font (source_buffer,lr_half) *** (T.B.D.) *** 
 *      output_sixel_in_bin ()
 *      output_sixel_in_hex ()
 *      output_sixel_as_font ()
 *      output_vchar (attrs,width,drawerase)
 *      output_showpage ()
 * 
 *      update_ap_r (x)
 *      update_ap_a (x,y)
 *      update_showpage ()
 *      update_sixel ()
 *      update_buff_size ()
 *      update_paired_fontdict () *** (TBD) ***  
 * 
 ******************************************************************/


/*
 * External variables referenced:
 */





/******************************************************************
 * The advent of setup modules have changed the concept of jobs
 * and files within the translator.  In the past, the translator
 * would be called once for each file in the job.  With V3 the
 * DPS symbiont will call the translator to process setup modules.
 * Therefore, the translator can be called multiple times to process
 * a single file based on the number of setup modules specified
 * on the command line.  Each invocation of the Translator for a
 * single file is called a *subjob*.  The Translator and PostScript
 * state must be held consistant across subjobs to ensure that the
 * file is printed properly.  The DPS symbiont tells the translator
 * to reset after the last of the subjobs (i.e. the file associated
 * with the setup modules).  Normal end of file processing is done 
 * after the subjobs are done.
 *******************************************************************/

/*****************************************************************
 * The old ps_init has been turned into three routines.  ps_init_file,
 * dispose_of_prologue, and ps_init_subjob.  ps_init_file () initializes
 * the state of the PostScript machine, Translator "PostScript state"
 * variables, and the internal dll and spacing font cache.  It is
 * called at the start of every file.  dispose_of_prologue () loads the
 * prologue into the PS machine in a three step process.  ps_init_subjob ()
 * opens the Translator dictionary (TRN$XLATE_DICT) to be used with
 * each subjob.  It is called at the start of each subjob.  The calling
 * of these three routines is controlled in xlc_main.c
 *********************************************************************/ 

ps_init_file ()
/******************************************************************

 ******************************************************************/

{
WORD boxnumber;
LONG sixel_index;

/*
 * Initialize "postscript state" (ALL VARIABLES ARE STORED IN
 * UNITS OF "CENTIPOINTS", where a centipoint is 1/7200th of a inch.) 
 * ------------------------------------------------------------- */

stack_ptr = BAREUNIT;	/* init stack pointer to bareunit */
cur_ps_st = &ps_st[stack_ptr];

cur_ps_st -> curchar.ap.xval = -1; /* Initialize PS pos to weird value so a moveto */
cur_ps_st -> curchar.ap.yval = -1; /* will get generated before the 1st char output */
cur_ps_st -> curchar.attr_data.attr_baseline_offset = -1; /* will get generated before the 1st char output */
cur_ps_st -> origin.xval = cur_ps_st -> origin.yval = 0;	/* page origin */
cur_ps_st -> scale_factor.xval = DEFAULT_SCALE_X;
cur_ps_st -> scale_factor.yval = DEFAULT_SCALE_Y;
/*    FONT font_dict;	*/
cur_ps_st -> curchar.char_data.char_font = END_OF_DICTIONARY;
cur_ps_st -> curchar.font_data.horizontal_spacing = -1;
cur_ps_st -> orientation = ORIENT_PS;
cur_ps_st -> sheet_len = 0;
cur_ps_st -> line_thickness = desired_st.line_thickness 
			    = DEFAULT_THICKNESS;
cur_ps_st -> sixel_buf_size = 0;
cur_ps_st -> open = ALLCLOSED;
cur_ps_st -> psst_paper_tray = TRAY_DEV_DEF;
cg_st.cgst_wpf = FALSE;

for (sixel_index = 0; sixel_index <= BLK_SIZE; sixel_index++)
{
    sixel_buffer [sixel_index] = 0;
}

cg_st.cgst_rightmost_pixel = 0; 
cg_st.cgst_sixel_buffer_empty = TRUE;
cg_st.cgst_first_row = TRUE;

/* Init VAX font table */

font_init();

/*
 * Initialize variables for tracking the VM used for downloaded and
 * spaced fonts.
 */

cg_st.cgst_vm_downloaded_used = cg_st.cgst_vm_spaced_used = 0;

/* Init dll font table */

cg_st.cgst_next_element_for_swap = 0;
for (boxnumber = 0; boxnumber < DLL_FONT_TABLE_SIZE; boxnumber++)
    {
    dll_font_table [boxnumber] .dll_active = INACTIVE;
    }

/* Init spacing font table */

for (boxnumber = 0; boxnumber < SPACING_FONT_TABLE_SIZE; boxnumber++)
    {
    spacing_font_table [boxnumber] .space_active = INACTIVE;
    }

/* Put ps_st onto stack */ 
 
old_ps_stack_ptr = stack_ptr;   /* Save old stack pointer */ 
stack_ptr++;			/* Increment stack pointer */ 
 
/* Copy last ps_st into top of stack */ 
ps_st[stack_ptr] = ps_st[old_ps_stack_ptr];
}

ps_init_subjob ()
/**************************************************************************
 *
 * ps_init_subjob is called for every subjob to open the Translator
 * dictionary, setup any debug information, and perform the save and
 * scale operations to prepare for translated data.
 *
 **************************************************************************/
{
/* Open TRN$XLATE_DICT to define cache limit, init sixelfont, 
 * perform setpacking, and get the current input tray.
 */

ps_str ("TRN$XLATE_DICT begin\n");

/****
 **** setpacking and setcachelimit are done here instead of in the prologue
 **** so that non-ANSI jobs will not be adversely effected by the state of 
 **** the PS machine when the prologue is preloaded by the symbiont.
 ****/

ps_str (str_setpacking);
ps_str (str_setcachelimit);

/*
 * Get the current paper tray to print from.  Printing is restored to this
 * tray on a RIS.
 */
 
ps_str (str_finddefpapertray);

/*
 * Set up the sixel font compression mapping.  This needs to be done before 
 * any PostScript 'saves' (and especially 'restores') are done;  It is not
 * included as part of the prologue as constant data, since part of it 
 * has to be built for consistent sixel compression mapping.
 */

init_sixel_font_keys();		

/* Close the Translator dictionary (TRN$XLATE_DICT)		*/

ps_str (str_end);


ps_str ("\f");	/* A <ff> to quickly find the end of the preamble */
ps_str ("\n");

/* Open the Translator dictionary.	*/

ps_str ("TRN$XLATE_DICT begin\n");

/* Start the translated output on a new line.  */

ps_str ("\nvmstatus pop /dllvminit exch def pop\n");


PS_VM_TRACE_MACRO ("(Save level  1) dm0\n",cg_st.cgst_vm_size);

M$SAVE();	/* Close Preamble level. Move stack to DLL level */

PS_VM_TRACE_MACRO ("(Save level  2) dm0\n",cg_st.cgst_vm_size);


M$SAVE();	/* Close DLL level. Move stack to Spaced level */

PS_VM_TRACE_MACRO ("(Save level  3) dm0\n",cg_st.cgst_vm_size);

M$SAVE();	/* Close Spaced level. Move stack to Define level */

PS_VM_TRACE_MACRO ("(Save level  4) dm0\n",cg_st.cgst_vm_size);

M$SAVE();	/* Close Define level. Move stack to Page level */
ps_str (str_scale);
}

dispose_prologue (decbind)
/*************************************************************************
 *
 * dispose_prologue () sends the ANSI prologue to the PS machine.  If
 * the prologue is being preloaded for future use, the Translator does
 * not "bind" to the PostScript operators.
 *
 ************************************************************************/
BOOLEAN decbind;
{
/*
 * Output the initial save if the prologue is going down to the
 * PostScript machine with the file.
 */
/*
 * To fix the invalidrestore problems.  The initial save has been
 * moved to xlc_main ().
 */
/* if (decbind) ps_str ("save\n"); */

/* Output the error handler only if ERROR_HANDLER is defined <>0 in dbug.h */
#if ERROR_HANDLER
    ps_str (str_error_handler);    /* <--- FOR DEBUG ONLY!!! */
#endif

/* Output the time stamp and version number of this prologue  */

ps_str (str_time_date_and_version);

/*
 * If the prologue is going down to the PS machine with the file, then
 * bind to all the PostScript operators defined in the prologue.
 */

if (decbind)
    ps_str (str_decbind);
else
    ps_str (str_decbind_null);

/* Now output the meat of the prologue */

ps_str (str_prologue); 

}

/*
 *
 *  dispose_showpage 
 *
 */

dispose_showpage ()
{ 

    /*	Let's empty the sixel buffer, so we forget them 
     */
    sixel_empty_sixel_buffer ();



if ((cur_ps_st -> curchar.ap.xval	!= cur_ps_st -> curchar.ap.xval)       || 
    (cur_ps_st -> curchar.ap.yval	!= cur_ps_st -> curchar.ap.yval)       || 
    (ALLCLOSED 		!= cur_ps_st -> open)) 
          {
          M$CLOSE_ALL();
          M$CHECK_STATE_CHANGE_AP(cur_ps_st -> curchar.ap.xval,cur_ps_st -> curchar.ap.yval);
          M$CHECK_STATE_CHANGE_OPEN(ALLCLOSED,0);
          }

/* Output the object and                */
/* Update ps_st based on objects impact */

while (stack_ptr > PAGE-1) 
     {
     M$RESTORE();		/* Restore back to page-1 level */
     }
M$OUTPUT_SHOWPAGE();
M$SAVE();
ps_str (str_scale);
M$UPDATE_SHOWPAGE();
}

/*
 *
 *  dispose_condshowpg 
 *
 */
dispose_condshowpg ()
{ 
if ( cg_st.cgst_wpf) dispose_showpage ();
}

dispose_eof ()
/******************************************************************

dispose_eof performs the final "restore" bringing the PS state
back to BAREUNIT

******************************************************************/

{ 
/* ps_str (" restore ");	/* Return to base level */
}

dispose_end_of_subjob ()
/**********************************************************************

dispose_end_of_subjob closes all open PS states, does a final showpage if
neccessary, and closed the Translator dictionary (TRN$XLATE_DICT).

***********************************************************************/
{
if ((ALLCLOSED != cur_ps_st -> open)) 
          {
          M$CLOSE_ALL();
          M$CHECK_STATE_CHANGE_OPEN(ALLCLOSED,0);
          }

/*
 *  Empty the sixel buffer, so we forget them 
 */

sixel_empty_sixel_buffer ();

 /* If the Translator is processing a setup module (i.e. the reset
  * exit bit is off) reset the active position to the top of the 
  * page before doing the showpage.
  */

/* Test to see if showpage command should be sent */

while (stack_ptr > (PAGE - 1)) 
     {
     M$RESTORE(); 		/* Restore back to preamble level */
     }
if (cg_st.cgst_wpf & cg_st.cgst_last_showpage) 
     {
     /* Output the final showpage	*/
     M$OUTPUT_SHOWPAGE();
     cg_st.cgst_wpf = FALSE;
     }

if (cg_st.cgst_last_showpage) 
     {
     /* The last item sent after the last showpage is a newsheet */
     ps_str(str_exec_newsheet);
     }


while (stack_ptr > BAREUNIT+1) 
     {
     M$RESTORE(); 		/* Restore back to entry level +1 */
     }

ps_str (" ");
ps_str (str_end);		/* Close Translator Dictionary */

/*
 * An invalidrestore error message is received when printing jobs
 * with multilple files using setup.  This is because the DPS
 * symbiont sends down device control library modules between 
 * the first and second subjob.  This leaves the PostScript
 * context in a state where a restore will produce the error
 * message.  For the time being this problem will be fixed
 * by placing a save and restore around every subjob.
 */

ps_str (" restore ");	/* Return to base level */

}

/*
 *
 *  process_char 
 *
 */
DEF process_char(glyph_ptr)
PGLYPH glyph_ptr;

{

cg_st.cgst_wpf = TRUE;


if (
    (vax_font_table_box_number[glyph_ptr -> char_data.char_font]
			!= cur_ps_st -> curchar.char_data.char_font) || 
    (glyph_ptr -> font_data.horizontal_spacing 
			!= cur_ps_st -> curchar.font_data.horizontal_spacing) || 
    (1			!= cur_ps_st -> scale_factor.xval) || 
    (1			!= cur_ps_st -> scale_factor.yval) || 
    (cg_st.cgst_orientation	!= cur_ps_st -> orientation)       || 
    (cg_st.cgst_sheet_len	!= cur_ps_st -> sheet_len)         || 
    (cg_st.cgst_origin.xval	!= cur_ps_st -> origin.xval)       || 
    (cg_st.cgst_origin.yval	!= cur_ps_st -> origin.yval)       || 
    (glyph_ptr -> attr_data.attr_baseline_offset	!= cur_ps_st -> curchar.attr_data.attr_baseline_offset)       || 
    (glyph_ptr -> font_data.algorithmic_attributes     
		    != cur_ps_st -> curchar.font_data.algorithmic_attributes)     || 
    (SHOWOPEN 		!= cur_ps_st -> open)) 
          {
          M$CLOSE_ALL();
          M$CHECK_STATE_CHANGE_FONT(glyph_ptr -> char_data.char_font,
					glyph_ptr -> font_data.horizontal_spacing);
          M$CHECK_STATE_CHANGE_SCALE(1,1);
          M$CHECK_STATE_CHANGE_ORIENT(cg_st.cgst_orientation,
					cg_st.cgst_sheet_len);
          M$CHECK_STATE_CHANGE_ORIGIN(cg_st.cgst_origin.xval,
					cg_st.cgst_origin.yval);
          M$CHECK_STATE_CHANGE_AP(glyph_ptr -> ap.xval,glyph_ptr -> ap.yval);
          M$CHECK_STATE_CHANGE_ATTRS(glyph_ptr -> font_data.algorithmic_attributes,
					    glyph_ptr -> attr_data.attr_baseline_offset);
          M$CHECK_STATE_CHANGE_OPEN(SHOWOPEN,
				    glyph_ptr -> font_data.algorithmic_attributes);
          } 
     else 
	if  (	(glyph_ptr -> ap.xval	!= cur_ps_st -> curchar.ap.xval)       || 
		(glyph_ptr -> ap.yval	!= cur_ps_st -> curchar.ap.yval)       )
	    {
	    /* 
	    ** Patch the AP since it is such a common fault.  This will avoid
	    ** the overhead usually created for a AP fault.  This was handled
	    ** in three steps (using J171 as an example).
	    **
	    ** 1) Close the current show
	    ** 2) Update the AP
	    ** 3) Open the show
	    */
    
	    /*
	    ** 1) Closing the current show.  We steal the top of M$CLOSE_SHOW
	    */
	    ps_char ('\)'); 
	    if (cur_ps_st -> curchar.font_data.algorithmic_attributes & 
	       (UL | OVERLINE | DOU_UL))
	       {
		oprintf(" %d",
		       (cur_ps_st->curchar.attr_data.attr_baseline_offset - cur_ps_st->curchar.ap.yval));
	       };
	    ps_str (close_show_str 
		[(cur_ps_st -> curchar.font_data.algorithmic_attributes & (UL | OVERLINE | DOU_UL | ITALIC | BOLD | STRIKE))]);
	    /*
	    ** 2) Update the AP. Taken from M$SET_AP.  The restore is from
	    **	  the M$RESTORE that followed the closing above.  It comes
	    **    as a freebie here (no additional call needed), so we take it.
	    **    We also have to lie to the VAX stack that minics PS stack.
	    */

	    oprintf (str_restore_moveto, glyph_ptr ->ap.xval, glyph_ptr ->ap.yval );

	    ps_st[(stack_ptr -1)].curchar.ap.xval  =
		cur_ps_st->curchar.ap.xval =
		glyph_ptr-> ap.xval ;

	    ps_st[(stack_ptr -1)].curchar.ap.yval  =
		cur_ps_st->curchar.ap.yval =
		glyph_ptr-> ap.yval ;

	    /*
	    ** 3) Open the SHOW.  Sorta from M$OPEN_SHOW (more a matter of 
	    **	  following the lead of J171.
	    */
	    ps_str (str_save_open_show) ;

	    }


     /* Output the object and                */
     /* Update ps_st based on objects impact */
     {
     UBYTE tmp_local_cache = mapping_tables
		[vax_font_table_mapping_index[glyph_ptr->char_data.char_font]]
		[glyph_ptr->char_data.char_code] ;

     M$PS_SCHAR (tmp_local_cache) ;
     }
     M$UPDATE_AP_R(glyph_ptr -> char_data.char_width);
     return(TRUE);
}




/*
 *
 *  dispose_vchar 
 *
 */

dispose_vchar (glyph_ptr)
GLYPH	*glyph_ptr;
{ 

/* Set all desired states in desired_st table */

/* Check for any state changes                          */
/*     If any changes update ps_st and issue PS command */

if (
    (vax_font_table_box_number [glyph_ptr -> char_data.char_font]
			!= cur_ps_st -> curchar.char_data.char_font) || 
    (glyph_ptr -> font_data.horizontal_spacing 
			!= cur_ps_st -> curchar.font_data.horizontal_spacing) || 
    (1			!= cur_ps_st -> scale_factor.xval) || 
    (1			!= cur_ps_st -> scale_factor.yval) || 
    (cg_st.cgst_orientation	!= cur_ps_st -> orientation)       || 
    (cg_st.cgst_sheet_len	!= cur_ps_st -> sheet_len)         || 
    (cg_st.cgst_origin.xval	!= cur_ps_st -> origin.xval)       || 
    (cg_st.cgst_origin.yval	!= cur_ps_st -> origin.yval)       || 
    (glyph_ptr -> ap.xval	!= cur_ps_st -> curchar.ap.xval)       || 
    (glyph_ptr -> ap.yval	!= cur_ps_st -> curchar.ap.yval)       || 
    (glyph_ptr -> attr_data.attr_baseline_offset	!= cur_ps_st -> curchar.attr_data.attr_baseline_offset)       || 
    (glyph_ptr -> font_data.algorithmic_attributes != cur_ps_st -> curchar.font_data.algorithmic_attributes)     || 
    (ALLCLOSED		!= cur_ps_st -> open)) 
          {
          M$CLOSE_ALL();
          M$CHECK_STATE_CHANGE_FONT(glyph_ptr -> char_data.char_font, cur_ps_st -> curchar.font_data.horizontal_spacing);
          M$CHECK_STATE_CHANGE_SCALE(1, 1);
          M$CHECK_STATE_CHANGE_ORIENT(cg_st.cgst_orientation, cg_st.cgst_sheet_len);
          M$CHECK_STATE_CHANGE_ORIGIN(cg_st.cgst_origin.xval, cg_st.cgst_origin.yval);
          M$CHECK_STATE_CHANGE_AP(glyph_ptr -> ap.xval,glyph_ptr -> ap.yval);
          M$CHECK_STATE_CHANGE_ATTRS(cg_st.cgst_origin.xval, glyph_ptr -> attr_data.attr_baseline_offset);
          M$CHECK_STATE_CHANGE_OPEN(ALLCLOSED,cg_st.cgst_origin.xval);
          }

/* Output the object and                */
/* Update ps_st based on objects impact */
{
   /* Super kludge to avoid a compile error in macro string length.  The
   ** compiler could not handle the text for 'tmp', so a caching is done
   ** with a local var.
   */ 
   LONG    tmp= (glyph_ptr->attr_data.attr_baseline_offset-glyph_ptr->ap.yval);

M$OUTPUT_VCHAR(glyph_ptr->font_data.algorithmic_attributes,
	       glyph_ptr->char_data.char_width,
	       glyph_ptr->char_data.char_code,
	       tmp);
}
M$UPDATE_AP_A(-1,-1);	/*  The routines doing VCHAR loose the active position, (there is no current point left
			 *  so we cannot do an update ap relative as would seem the case, 
			 *  but have to set the active position to undefined (-1, -1) using 
			 *  an update ap absolute
			 */
}


/*
 *
 *  dispose_decvec 
 *
 */

dispose_decvec (decvec_ptr)
DECVEC	*decvec_ptr;

{ 
/* Set all desired states in desired_st table */
 cg_st.cgst_wpf = TRUE;

/* Check for any state changes                          */
/*     If any changes update ps_st and issue PS command */

if ((1 				!= cur_ps_st -> scale_factor.xval) || 
    (1 				!= cur_ps_st -> scale_factor.yval) || 
    (cg_st.cgst_orientation		!= cur_ps_st -> orientation)       || 
    (cg_st.cgst_sheet_len		!= cur_ps_st -> sheet_len)         || 
    (cg_st.cgst_origin.xval		!= cur_ps_st -> origin.xval)       || 
    (cg_st.cgst_origin.yval		!= cur_ps_st -> origin.yval)       || 
    (decvec_ptr -> dv_ap.xval		!= cur_ps_st -> curchar.ap.xval)       || 
    (decvec_ptr -> dv_ap.yval		!= cur_ps_st -> curchar.ap.yval)       || 
    (decvec_ptr -> thickness 	!= cur_ps_st -> line_thickness)    || 
    (DECVECOPEN			!= cur_ps_st -> open)) 
          {
	  decvec_ptr -> dv_ap.yval = decvec_ptr -> dv_ap.yval + 
	    (0.5 * decvec_ptr -> thickness);
	  decvec_ptr -> dvend.yval = decvec_ptr -> dvend.yval + 
	    (0.5 * decvec_ptr -> thickness);
          M$CLOSE_ALL();
          M$CHECK_STATE_CHANGE_SCALE(1,1);
          M$CHECK_STATE_CHANGE_ORIENT(cg_st.cgst_orientation, cg_st.cgst_sheet_len);
          M$CHECK_STATE_CHANGE_ORIGIN(cg_st.cgst_origin.xval, cg_st.cgst_origin.yval);
          M$CHECK_STATE_CHANGE_AP(decvec_ptr -> dv_ap.xval,decvec_ptr -> dv_ap.yval);
          M$CHECK_STATE_CHANGE_THICK(decvec_ptr -> thickness);
          M$CHECK_STATE_CHANGE_OPEN(DECVECOPEN,0);
          }

/* Output the object and                */
/* Update ps_st based on objects impact */

oprintf (str_lineto, decvec_ptr -> dvend.xval, decvec_ptr -> dvend.yval); 

/* update_decvec here */

cur_ps_st -> line_thickness = decvec_ptr -> thickness;
M$UPDATE_AP_A(-1, -1);
}


/*
 *
 *  dispose_sixel 
 *
 */

dispose_sixel ( ahp, avp, im_hor_grid, vert_grid, byte_cnt, 
		sf_hor_grid, sf_byte_cnt, sixel_buff )

UBYTE	sixel_buff [];
LONG	ahp, avp;
LONG	byte_cnt, sf_byte_cnt;
LONG	im_hor_grid, sf_hor_grid, vert_grid;
{ 
/* Set all desired states in desired_st table */
 cg_st.cgst_wpf = TRUE;

/* Check for any state changes                          */
/*     If any changes update ps_st and issue PS command */

/*     M$CHECK_STATE_CHANGE_ALL() */
if ((1			!= cur_ps_st -> scale_factor.xval) || 
    (1			!= cur_ps_st -> scale_factor.yval) || 
    (cg_st.cgst_orientation	!= cur_ps_st -> orientation)       || 
    (cg_st.cgst_sheet_len	!= cur_ps_st -> sheet_len)         || 
    (ahp + cg_st.cgst_origin.xval 			     != cur_ps_st -> origin.xval)       || 
    (avp + cg_st.cgst_origin.yval - SIXEL_BASELINE_OFFSET + vert_grid != cur_ps_st -> origin.yval)       || 
    (0 			!= cur_ps_st -> curchar.ap.xval)       || 
    (0 			!= cur_ps_st -> curchar.ap.yval)       || 
    (byte_cnt		!= cur_ps_st -> sixel_buf_size)    || 
    (SIXELOPEN		!= cur_ps_st -> open)) 
          {
          M$CLOSE_ALL();
          M$CHECK_STATE_CHANGE_SCALE(1,1);
          M$CHECK_STATE_CHANGE_ORIENT(cg_st.cgst_orientation, cg_st.cgst_sheet_len);
          M$CHECK_STATE_CHANGE_ORIGIN(ahp + cg_st.cgst_origin.xval,avp + cg_st.cgst_origin.yval - SIXEL_BASELINE_OFFSET + vert_grid);
          M$CHECK_STATE_CHANGE_AP(0,0);
          if ( cg_st.cgst_image_mode != trn$k_image_sixel_font ) 
              M$CHECK_STATE_CHANGE_BUFF(byte_cnt);
          M$CHECK_STATE_CHANGE_OPEN(SIXELOPEN,0);
          }

/* Output the object and                */
/* Update ps_st based on objects impact */

	/* call output_sixel_ routine selected by pointer in init */
	/* parameter list changed 3 June 1988 to support sixelfont */
(*cg_st.cgst_output_sixel_ptr)( ahp, avp, im_hor_grid, sf_hor_grid, vert_grid, 
			byte_cnt, sf_byte_cnt, sixel_buff );

M$UPDATE_SIXEL(byte_cnt);
}


/*
 *
 *  dispose_cache_font 
 *
 */

dispose_cache_font (boxnumber, left_half_is_empty, vax_font_table_entry)
WORD boxnumber,
     left_half_is_empty,
     vax_font_table_entry;
{
    WORD i;

    desired_st.open = FONTOPEN;

    /* Check for any state changes; if any, update ps_st and issue
     * PS command 
     */
     M$CHECK_STATE_CHANGE_ALL()
          {
          M$CLOSE_ALL();
          M$CHECK_STATE_CHANGE_OPEN(FONTOPEN,boxnumber);
			/* boxnumber is just a dummy value for this macro */
          }

    /* Download the font into the "box" */

    PS_VM_TRACE_MACRO("%d dm1\n",(cg_st.cgst_vm_size-cg_st.cgst_vm_available));	/* Send command to dump VM */
    dll (boxnumber, left_half_is_empty, vax_font_table_entry);
    PS_VM_TRACE_MACRO("%d dm2\n",(cg_st.cgst_vm_size-cg_st.cgst_vm_available));	/* Send command to dump VM */

    /* Now see if this is a true paired font */

    if (vax_font_table[vax_font_table_entry] .opposite_entry >  vax_font_table_entry)    /* Then download the other half */
	{
	        PS_VM_TRACE_MACRO("%d dm1\n",(cg_st.cgst_vm_size-cg_st.cgst_vm_available));	/* Send command to dump VM */
		dll (boxnumber, 0, vax_font_table[vax_font_table_entry] .opposite_entry);
	        PS_VM_TRACE_MACRO("%d dm2\n",(cg_st.cgst_vm_size-cg_st.cgst_vm_available));	/* Send command to dump VM */
	}

    M$SAVE();	/* Close DLL_LEVEL */
    M$SAVE();	/* Close SPACED_LEVEL */

    /* Wipe out spacing_font_table because all spaced fonts have been 
     * wiped out of PS machine after downloading font.
     */

     invalidate_spaced_font_cache ();
    
        /* Redefine all downloaded fonts including the newest one */

	PS_VM_TRACE_MACRO("%d dm3\n",(cg_st.cgst_vm_size-cg_st.cgst_vm_available));	/* Send command to dump VM */
        define_dll_fonts();
	PS_VM_TRACE_MACRO("%d dm4\n",(cg_st.cgst_vm_size-cg_st.cgst_vm_available));	/* Send command to dump VM */



        M$SAVE();	/* Close DEFINE_LEVEL */
        cur_ps_st -> open = ALLCLOSED;
        ps_str (str_scale);
}


/*
 *
 *  dispose_cache_spacing 
 *
 */

dispose_cache_spacing (spaced_boxnumber, unspaced_boxnumber, spacing)
WORD spaced_boxnumber,
     unspaced_boxnumber,
     spacing;
{
    desired_st.open = SPACINGOPEN;

    /* Check for any state changes; if any, update ps_st and issue
     * PS command 
     */

     M$CHECK_STATE_CHANGE_ALL()
          {
          M$CLOSE_ALL();
          M$CHECK_STATE_CHANGE_OPEN(SPACINGOPEN,spaced_boxnumber);
			/* spaced_boxnumber is just a dummy value 
                           for M$CHECK_STATE_CHANGE_OPEN */
          }


    /* set spacing */
    oprintf (str_spacing, spaced_boxnumber + SPACING_OFFSET, unspaced_boxnumber, spacing); 
    M$SAVE();	/* Close SPACED_LEVEL */

    /* Redefine all downloaded and spaced fonts including the newest one */

    PS_VM_TRACE_MACRO("%d dm3\n",(cg_st.cgst_vm_size-cg_st.cgst_vm_available));	/* Send command to dump VM */
    define_dll_fonts();
    PS_VM_TRACE_MACRO("%d dm4\n",(cg_st.cgst_vm_size-cg_st.cgst_vm_available));	/* Send command to dump VM */

    PS_VM_TRACE_MACRO("%d dm5\n",(cg_st.cgst_vm_size-cg_st.cgst_vm_available));	/* Send command to dump VM */
    define_spaced_fonts();
    PS_VM_TRACE_MACRO("%d dm6\n",(cg_st.cgst_vm_size-cg_st.cgst_vm_available));	/* Send command to dump VM */

    M$SAVE();	/* Close DEFINE_LEVEL */
    cur_ps_st -> open = ALLCLOSED;
    ps_str (str_scale);
}


/*
 *
 *  dispose_add_font_to_cache 
 *
 */

dispose_add_font_to_cache (paired_font_number) 
{
WORD boxnumber,
     left_half_is_empty,
     vax_font_table_entry;
BYTE opposite_entry;
LONG paired_virtual_memory_needs;


opposite_entry = vax_font_table[paired_font_number] .opposite_entry;
paired_virtual_memory_needs = 
    vax_font_table [paired_font_number] .virtual_memory_needs;

if (opposite_entry != UNUSED)
    paired_virtual_memory_needs += 
       vax_font_table[opposite_entry] .virtual_memory_needs;

/* Find out how much room in VM is left */ 

/* If garbage is contributing to a nearly full virtual memory... 
*/ 
if (cg_st.cgst_vm_available - VIRTUAL_MEMORY_SAFETY_MARGIN - 
    paired_virtual_memory_needs < 0) 

    { /* then it is time to collect garbage */ 

    dispose_cleanup (); 
    } 

/*
 * Account for VM available in the printer and VM used
 * by all downloaded fonts for this font.
 */

cg_st.cgst_vm_available -= paired_virtual_memory_needs; 
cg_st.cgst_vm_downloaded_used += paired_virtual_memory_needs;

/* Find next available openning in the Font Cache */ 

boxnumber = 0; 
while (	(dll_font_table [boxnumber] .dll_active) && 
	(boxnumber < DLL_FONT_TABLE_SIZE) ) 
	boxnumber++;   /* find next available slot */ 

/* Is Font Cache full? */ 

if (boxnumber == DLL_FONT_TABLE_SIZE) 
    { /* Here if yes */ 
    boxnumber = cg_st.cgst_next_element_for_swap ++; 
    if (cg_st.cgst_next_element_for_swap  == DLL_FONT_TABLE_SIZE) 
	cg_st.cgst_next_element_for_swap  = 0; 
    dispose_delete_box_from_cache (boxnumber); 
    } 

/* Set font Downloaded in dll_font_table */ 

dll_font_table [boxnumber] .dll_font = paired_font_number; 
dll_font_table [boxnumber] .dll_active = ACTIVE; 

/* Download the font into the "box" */ 

left_half_is_empty = 
    (vax_font_table_mapping_index [paired_font_number] != MAP_LEFT) && 
    (vax_font_table [paired_font_number].opposite_entry == UNUSED); 
dispose_cache_font (boxnumber + NUMBER_OF_FIXED_FONT_BOXES, 
		    left_half_is_empty, paired_font_number); 
return (boxnumber);
/* Done with down loading the font */  
}



/*
 *
 *  dispose_add_spacing_to_cache 
 *
 */

dispose_add_spacing_to_cache (paired_font_number, unspaced_boxnumber, spacing) 

WORD paired_font_number, 
     unspaced_boxnumber,
     spacing;
{
WORD spaced_boxnumber=0;

/*
 * Find out if there is enough VM for the spaced font.  
 */

if (cg_st.cgst_vm_available - VIRTUAL_MEMORY_SAFETY_MARGIN - 
    SPACED_FONT_VM_NEEDS < 0) 
    {

    /* Clean up spaced fonts  */

    wipe_spacing ();
    cg_st.cgst_vm_available += cg_st.cgst_vm_spaced_used;
    cg_st.cgst_vm_spaced_used = 0;
    
    /* If there still is not enough VM for the spaced font then
     * cleanup all VM.
     */

    if (cg_st.cgst_vm_available - VIRTUAL_MEMORY_SAFETY_MARGIN - 
	SPACED_FONT_VM_NEEDS < 0) 
    	{
    	dispose_cleanup ();
        }
    } 

/*
 * Account for VM available in the printer and VM used
 * by all spaced fonts for this spaced font.
 */

cg_st.cgst_vm_available -= SPACED_FONT_VM_NEEDS; 
cg_st.cgst_vm_spaced_used += SPACED_FONT_VM_NEEDS;

/* Find next available openning in the Spacing Cache */ 

while (	(spacing_font_table [spaced_boxnumber] .space_active != INACTIVE) && 
	(spaced_boxnumber < SPACING_FONT_TABLE_SIZE) ) 
	spaced_boxnumber++;   /* find next available slot */ 

/* Is Spacing Cache full? */ 

if (spaced_boxnumber == SPACING_FONT_TABLE_SIZE) 
    { /* Here if yes */ 
    wipe_spacing ();
    cg_st.cgst_vm_available += cg_st.cgst_vm_spaced_used;
    cg_st.cgst_vm_spaced_used = 0; spaced_boxnumber = 0; 
    } 

/* Set font Downloaded in dll_font_table */ 

spacing_font_table [spaced_boxnumber] .space_font = 
			vax_font_table_box_number[paired_font_number]; 
spacing_font_table [spaced_boxnumber] .spacing = spacing;
spacing_font_table [spaced_boxnumber] .space_active = ACTIVE;
dispose_cache_spacing (spaced_boxnumber, unspaced_boxnumber, spacing);
return (spaced_boxnumber);
}


/*
 *
 *  dispose_delete_box_from_cache 
 *
 *  There are no more entries available in the cache, the entry "box_number"
 *  has been selected as the one to be thrown away. Find out what font(s) were
 *  in that box, (There could be pairing), mark them as not loaded in vax_font_table
 */



/*
 *  WARNING !!! DANGER !!! CAREFUL !!!
 * 
 *  At this time, there is no pairing on the PS side
 *  fonts are downloaded individually.
 *  This code does not deal with potential pairing.
 *  When doing pairing on the PS side, make sure to complete this code
 */ 

dispose_delete_box_from_cache (box_number)

WORD box_number;
{

WORD spaced_box_number; /* Search loop index */

/*
 *  The dll_font_table[box].dll_font tells us what font is downloaded in this box
 *  The vax_font_table[font].box_number tells us in what box a font is downloaded.
 *
 *  If we are destroying "box", need to update the vax_font_table entry for the font in that box
 *
 *  NOTE, more will be needded when doing font_pairing on the PS side.
 */ 

    vax_font_table_box_number [dll_font_table[box_number].dll_font]= END_OF_DICTIONARY;

/*
 * If there is a spaced copy of the box being deleted, invalidate spaced font table.  
 *
 *  Note, the following code only checks the box_number field, and does not check the active field.
 *  This means that we de-activate inactive entries, seems cheaper than testing the active flag
 *
 */
 
for (spaced_box_number = 0; spaced_box_number  < SPACING_FONT_TABLE_SIZE; spaced_box_number ++)
    {
    if (spacing_font_table[spaced_box_number].space_font == box_number)
         spacing_font_table[spaced_box_number].space_active = INACTIVE; 
    }


    /* Mark the cache entry as inactive */

    dll_font_table [box_number].dll_active = INACTIVE;

}


/*
 *
 *  dispose_cleanup 
 *
 */

dispose_cleanup ()
{ 
   cg_st.cgst_vm_available = cg_st.cgst_vm_size;
   cg_st.cgst_vm_downloaded_used = cg_st.cgst_vm_spaced_used = 0;
   wipe_all ();
}



/*
 *
 *  wipe_all 
 *
 */

wipe_all ()
{
    WORD i;

	M$CLOSE_ALL();
	ps_str (str_next_line);
	while (stack_ptr > (SPACED_LEVEL - 1))
		{
		M$RESTORE();
		}

	invalidate_spaced_font_cache ();

	while (stack_ptr > (DLL_LEVEL - 1))
		{
		M$RESTORE();
		}

	/* Invalidate DLL font table */
	cg_st.cgst_next_element_for_swap  = 0;
	for (i = 0; i < DLL_FONT_TABLE_SIZE; i++)
		{
		    if (dll_font_table [i] .dll_active == ACTIVE)
			{
			    dispose_delete_box_from_cache (i);
			}
	        }

	while (stack_ptr < PAGE)
		{
		M$SAVE();
		}
	ps_str (str_scale);
	ps_str (str_next_line);
}



/*
 *
 *  wipe_spacing 
 *
 */

wipe_spacing ()
{
    WORD i;

	M$CLOSE_ALL();
	ps_str (str_next_line);
	while (stack_ptr > (SPACED_LEVEL - 1))
		{
		M$RESTORE();
		}

    	invalidate_spaced_font_cache ();

	while (stack_ptr < PAGE)
		{
		M$SAVE();
		}
	ps_str (str_scale);
	ps_str (str_next_line);
}


/*
 *
 *  output_sixel_in_hex 
 *
 */

output_sixel_in_hex ( ahp, avp, im_hor_grid, sf_hor_grid, vert_grid, 
			byte_cnt, sf_byte_cnt, sixel_buff )
UBYTE	sixel_buff [];
LONG
	ahp, avp,
	sf_byte_cnt, byte_cnt,
	sf_hor_grid, im_hor_grid, vert_grid;
{

register UBYTE
	 *sbuff_ptr,         /* Sixel buffer pointer           */
         hex_buff,           /* One hex byte of converted data */
         vert_mask;          /* Bit mask for sixel bites       */

register LONG
         bitwidth,           /* Total number of bits in a      */
                             /*   row = byte_cnt * 8           */
         alpha,i,j,k;        /* Misc. variables                */
         
if (byte_cnt > 0) 
	{
	bitwidth = byte_cnt << 3;


	/* scale the unit square, so that PS can draw the picture */
	/* remember that we work in an inverted scale in y, so we need */
	/* to invert in y by multiplying the y scale by -1 */
	/* As we are always using this invertedd axis, (Both in Portrait & */
	/* Landscape) there is no need to ask whether we should, as by the time */
	/* we output the sixels (here), orientation has already been set. */

	scalex = ((float)im_hor_grid / (float)cur_ps_st -> scale_factor.xval);
	if (ABS(scalex) < 1) scalex = 1;
	scaley = ((float)vert_grid / (float)cur_ps_st -> scale_factor.yval);
	if (ABS(scaley) < 1) scaley = 1;
	sprintf (str_buffer,"%f %f%s",scalex,scaley,str_float_scale);
	ps_str (str_buffer);

/*	if (! cg_st.cgst_first_row) ps_str (str_sixel_new_line); */
	ps_str (str_sixel_data_hex);




/*	In hex output mode, use HEX output, and short records
	so VMS does not mess up our output file
*/

	vert_mask = 1;
	for (i = 0; i < 6; i ++)
		{
		sbuff_ptr = &sixel_buff[0];
		for (k = 0; k <= bitwidth ; k += 512)
		/* Note that the above number (512) is arbitrary
		** it is just there to include a CR/LF every now 
		** and then so we don't end up with "record too long"
		** 512 seems like a good number, it is 64 bytes, 
		** or 128 HEX chars. Fits neatly on the screen, 
		** and in a record
		*/

			{
			alpha = MIN (512, bitwidth - k);
			for (j = 0; j < alpha; j += 4)
				{
				hex_buff = (*(sbuff_ptr++) & vert_mask) ? 0 : 8;
				if (!(*(sbuff_ptr++) & vert_mask)) hex_buff |= 4;
				if (!(*(sbuff_ptr++) & vert_mask)) hex_buff |= 2;
				if (!(*(sbuff_ptr++)   & vert_mask)) hex_buff |= 1;


	/* Output the object */
	
		/* ps_char (hexbuff) */
		/* expand ps_schar slightly here */
		/* but leave the processing of funnies to */
		/* dispose of char by including them in the */
		/* exclusion above */


		/*
		 * Stuff the character into the output buffer
		 */

		*obuf_ptr++ = (hex_buff < 10 ) ? (hex_buff + '0') : (hex_buff + 'A' - 10);


		/*
		 * Check for a buffer overflow or buffer full
		 */
		
		M$CHECK_BUFF_OF() ;

				}
			ps_str (str_next_line);
			}
		vert_mask <<= 1;
		}

	byte_cnt = 0;
	old_bitwidth = bitwidth;
	 cg_st.cgst_first_row = FALSE;
	}
}



/*
 *
 *  output_sixel_in_bin 
 *
 */

output_sixel_in_bin ( ahp, avp, im_hor_grid, sf_hor_grid, vert_grid, 
			byte_cnt, sf_byte_cnt, sixel_buff )
UBYTE	sixel_buff [];
LONG
	ahp, avp,
	sf_byte_cnt, byte_cnt,
	sf_hor_grid, im_hor_grid, vert_grid;
 
{

register UBYTE
	 *sbuff_ptr,         /* Sixel buffer pointer           */
         hex_buff,           /* One hex byte of converted data */
         vert_mask;          /* Bit mask for sixel bites       */

register LONG
         bitwidth,           /* Total number of bits in a      */
                             /*   row = byte_cnt * 8           */
         alpha,i,j,k;        /* Misc. variables                */
         
if (byte_cnt > 0) 
	{
	bitwidth = byte_cnt << 3;


	/* scale the unit square, so that PS can draw the picture */
	/* remember that we work in an inverted scale in y, so we need */
	/* to invert in y by multiplying the y scale by -1 */
	/* As we are always using this invertedd axis, (Both in Portrait & */
	/* Landscape) there is no need to ask whether we should, as by the time */
	/* we output the sixels (here), orientation has already been set. */

	scalex = ((float)im_hor_grid / (float)cur_ps_st -> scale_factor.xval);
	if (ABS(scalex) < 1) scalex = 1;
	scaley = ((float)vert_grid / (float)cur_ps_st -> scale_factor.yval);
	if (ABS(scaley) < 1) scaley = 1;
	sprintf (str_buffer,"%f %f%s",scalex,scaley,str_float_scale);
	ps_str (str_buffer);

/*	if (! cg_st.cgst_first_row) ps_str (str_sixel_new_line); */
	ps_str (str_sixel_data_bin);




/*
	In binary mode, so we go faster
*/

	vert_mask = 1;
	for (i = 0; i < 6; i ++)
		{
		sbuff_ptr = &sixel_buff[0];
		for (k = 0; k <= bitwidth ; k += 512)
		/* Note that the above number (512) is arbitrary
		** it is just there to include a CR/LF every now 
		** and then so we don't end up with "record too long"
		** 512 seems like a good number, it is 64 bytes, 
		** or 128 HEX chars. Fits neatly on the screen, 
		** and in a record
		*/

			{
			alpha = MIN (512, bitwidth - k);
			for (j = 0; j < alpha; j += 8)
				{
				hex_buff = (*(sbuff_ptr++) & vert_mask) ? 0 : 128;
				if (!(*(sbuff_ptr++) & vert_mask)) hex_buff |= 64;
				if (!(*(sbuff_ptr++) & vert_mask)) hex_buff |= 32;
				if (!(*(sbuff_ptr++) & vert_mask)) hex_buff |= 16;
				if (!(*(sbuff_ptr++) & vert_mask)) hex_buff |= 8;
				if (!(*(sbuff_ptr++) & vert_mask)) hex_buff |= 4;
				if (!(*(sbuff_ptr++) & vert_mask)) hex_buff |= 2;
				if (!(*(sbuff_ptr++) & vert_mask)) hex_buff |= 1;

	/* Output the object */
	
		/* ps_char (hexbuff) */
		/* expand ps_schar slightly here */
		/* but leave the processing of funnies to */
		/* dispose of char by including them in the */
		/* exclusion above */


		/*
		 * Stuff the character into the output buffer
		 */
		*obuf_ptr++ = (hex_buff );


		/*
		 * Check for a buffer overflow or buffer full
		 */
		
		M$CHECK_BUFF_OF() ;

				}
			}
		vert_mask <<= 1;
		}

	byte_cnt = 0;
	old_bitwidth = bitwidth;
	 cg_st.cgst_first_row = FALSE;
	}
}


/*
 *
 *  output_sixel_as_font 
 *
 */

output_sixel_as_font ( ahp, avp, im_hor_grid, sf_hor_grid, vert_grid, 
			byte_cnt, sf_byte_cnt, sixel_buff )
/******************************************************************

This routine was derived in structure from previously existing
routines output_sixel_in_hex() and output_sixel_in_bin().
General structure and interface to the rest of the translator 
have been maintained, but the fundamental approach of sixel_font
has caused some internal changes.

This routine maps from a sixel character and its count to a set of
characters that, in the sixel_font, are expressed as a single
character in a PostScript string and 'show' command.
See routine init_sixel_font_keys() for the use of six_rptr[]
and sixel_cmprss_mppng[].

       5.23.1  Generalities

       This prints one row of sixel data to the postscript  machine.   In
       the  process  it will convert the sixels into a text string
       to be output mapped through a special dot-matrix font for output.

       5.23.2  Input

       1.  sixel_buffer
       2.  sixel_count
       3.  margins


       5.23.3  Output

       1.  data as a PostScript string and 'show' command


       5.23.4  Called Routines

       1.  oprintf


       5.23.5  Internal Variables And Storage


       5.23.6  Code Comments

******************************************************************/

UBYTE	sixel_buff [];
LONG
	ahp, avp,
	sf_byte_cnt, byte_cnt,
	sf_hor_grid, im_hor_grid, vert_grid;
 
{

register UBYTE
	 sx_ent,             /* temporary storage for sixel entry */
	 *sbuff_ptr,         /* Sixel buffer pointer           */
         hex_buff,           /* One hex byte of converted data */
         vert_mask;          /* Bit mask for sixel bites       */

register LONG
         bitwidth,           /* old replacement for byte_cnt */
         countdown,          /* byte_cnt countdown */
         rpt_cnt,            /* character repeat count */
         wrap_cnt,           /* counter for character wrap */
         i,j,k;              /* Misc. variables */


         
if (sf_byte_cnt > 0) 
	{
	countdown = sf_byte_cnt;	/* save initial byte_cnt for counting down row */

	/* remember that we work in an inverted scale in y, so we need */
	/* to invert in y by multiplying the y scale by -1 */
	/* As we are always using this invertedd axis, (Both in Portrait & */
	/* Landscape) there is no need to ask whether we should, as by the time */
	/* we output the sixels (here), orientation has already been set. */

	scalex = ( 6.0 * (float)sf_hor_grid ) / (float)cur_ps_st -> scale_factor.xval;
	if (ABS(scalex) < 1) scalex = 1;
		/* vert_grid is 6*hor_grid in gr_nl; same here to maintain aspect ratio */
	scaley = -((float)vert_grid / (float)cur_ps_st -> scale_factor.yval);
	if (ABS(scaley) < 1) scaley = -1;
		/* negated to invert sixel writing direction (was upside down);
			probably will require a new translate value as well */
	if ( ( (float)((int)scalex) == scalex )
	  && ( (float)((int)scaley) == scaley ) ) /* this reduces excess trailing zeros */
	       sprintf (str_buffer, "%d %d %s", (int)scalex, (int)scaley, str_float_scale);
	  else sprintf (str_buffer, "%f %f %s", scalex, scaley, str_float_scale);
	ps_str (str_buffer);
	ps_str ( str_save );		/* do save/restore at this (the sixel band) level */
	ps_str ( str_sixelfontfind );	/* select the sixel font */
	ps_str ( str_open_show );	/* open paren for string */
	wrap_cnt = 1;			/* start wrap counter */
	sbuff_ptr = &sixel_buff[0];

	while ( countdown > 0 )		/* go through the whole buffer */
	  {
	    rpt_cnt = 0;		/* repeat count is for current character */
	    sx_ent = *sbuff_ptr;	/* copy the current character */
		/* while successive characters are the same 
			and there are more characters to follow... */
	    while ( ( sx_ent == *sbuff_ptr ) && ( countdown > 0 ) )
	      {
	        *sbuff_ptr++;	/* ...up the pointer to next character */
	        rpt_cnt++;	/* up the repeat count for this character */
	        countdown--;	/* decrement the saved byte count */
	      }
		/* after the loop, rpt_cnt has repeat count for current character */

		/* is this character compressible? (has equiv and needs it?) */
	    if ( ( six_rptr[sx_ent]->rpt_val != 0 ) && ( rpt_cnt > 1 ) )
		/* yes, process through equivalence list, be ready to fall
		   out into non-repeat handler to take care of residual count */
	      {
	        i = 0;
		/* scan through sixel repeater table for compression mappings */
	        while ( ( six_rptr[sx_ent][i].rpt_val != 0 ) && ( rpt_cnt > 1 ) )
	          {
			/* while the current mapping character is useful... */
	            while ( six_rptr[sx_ent][i].rpt_val <= rpt_cnt )
	              {
			/* ...use it... */
	                ps_schar( six_rptr[sx_ent][i].char_mapped );
			/* ...and account for it by reducing the repeat count */
	                rpt_cnt -= six_rptr[sx_ent][i].rpt_val;
	                wrap_cnt++;		/* increment the wrap counter */
	                if ( ( six_rptr[sx_ent][i].char_mapped == '\\' ) ||
	                     ( six_rptr[sx_ent][i].char_mapped == '('  ) ||
	                     ( six_rptr[sx_ent][i].char_mapped == ')'  ) )
	                        wrap_cnt++;	/* if a backslash was used, up counter again */
	                if ( wrap_cnt > 72 )  /* do a line break if needed */
	                  { ps_char( '\n' ); wrap_cnt = 0; }
	              }		/* end while (application of current compression count) */
	            i++;	/* increment ptr to next repeat equivalence key */
	          } 		/* end while (search for applicable compression counts */
	      }			/* end if (compressible character) then... */

		/* this loop outputs the current sixel-equivalent character its 
			repeat count times, and also outputs the remainder 
			of the characters needed to fulfill the repeat count 
			for compressibles;  most often this will be the last 
			value needed to fill out an odd repeat count, as the 
			smallest repeat mapping is for a count of two */

	    for ( i=0 ; i<rpt_cnt ; i++ ) 
	      {
	        ps_schar( sx_ent+63 );	/* output the sixel character */
	        if ( sx_ent+63 == '\\' ) wrap_cnt++;	/* the only special sixel character */
	        if ( wrap_cnt++ > 72 )  /* do a line break if needed */
	          { ps_char( '\n' ); wrap_cnt = 0; }
	      }

	  }	/* end while (countdown>0) */

		/* sixel_buff has been fully scanned, 
			translated into a sixelfont string, 
			and output;
		   now close the string and clean up */

	ps_str( str_show );	/* close paren, 'show' command */
	ps_str( str_restore );	/* 'restore' ('save' was just before findsixelfont) */
	}		/* end of if ( sf_byte_cnt > 0 ) */
}   /* end of routine output_sixel_as_font */



/*
 *
 *  invalidate_spaced_font_cache 
 *
 */

invalidate_spaced_font_cache ()
/*
 * This routine removes all spaced fonts from the spaced
 * font cache
 */
{
WORD i;

for (i = 0; i < SPACING_FONT_TABLE_SIZE; i++)
    {
    spacing_font_table [i] .space_active = INACTIVE;
    }
}



/*
 *   PROCESS_SHOWPAGE()
 */

VOID process_showpage()
   {
   dispose_showpage ();
   }



/*
 *   PROCESS_CONDSHOWPG()
 */

VOID process_condshowpg()
   {
    dispose_condshowpg ();
   }


/*
 *   PROCESS_ORIENTATION()
 *
 *   Set the orientation for the page.
 */

VOID process_orientation(new_orient)
ORIENTATION new_orient;
   {
    cg_st.cgst_orientation = new_orient;
   }



/*
 *   PROCESS_SET_ORIGIN()
 */

VOID process_set_origin(new_origin)
POINT new_origin;
   {
    cg_st.cgst_origin.xval = new_origin.xval;
    cg_st.cgst_origin.yval = new_origin.yval;

   }

/*
 * pdli_nvm_get
 */

BOOLEAN pdli_nvm_get (target_buffer, length)
PUB target_buffer;
WORD length;
{
	return (TRUE);
}




/*
 *   PROCESS_VCHAR()
 *
 *   This function sets the required virtual character on the page.
 *   A virtual character is one used to carry out some attributing
 *   such as underlining.
 */

DEF process_vchar(glyph_ptr)
PGLYPH glyph_ptr;
   {
    if	(glyph_ptr -> char_data.char_code == VIR_CHAR_BLOB)
	{
	        process_char(glyph_ptr);
		return (TRUE);
	}

#if 0

The width to be used here is not that of the vax font table, or that of the character returned by GET_WIDTH
(which by the way is a lowercase fucntion on the cpar side), but the true width in the PS device, including 
any spacing that may have been done.

So the optimization is disabled for the time being, 

    if	(   (glyph_ptr -> char_data.char_code == VIR_CHAR_SPACE)
	&&  (glyph_ptr -> char_data.char_width == GET_WIDTH(SPACE, 
					glyph_ptr -> char_data.char_font))
	)
	{
		glyph_ptr->char_data.char_code = SPACE;
	        process_char(glyph_ptr);
		glyph_ptr->char_data.char_code = VIR_CHAR_SPACE;
		return (TRUE);
	}
#endif
    dispose_vchar (glyph_ptr);
    return (TRUE);
   }



/*
 * process_error
 */

VOID process_error (severity_level, error_number)
SEVERITY_LEVEL severity_level;
ERROR_NUMBER error_number;

{
#if WITHOUTPUT
printf (type_of_error_condition [severity_level], error_number );
#endif

#if ERROR_REPORT
/* set desired state,
    check for changes 
    set state
*/
if (ALLCLOSED != cur_ps_st -> open)
  {
  M$CLOSE_ALL();
  }

/* Output the object */

M$SAVE();

    oprintf("%%process_error, severity: %d, err_num: %d\n",
	    severity_level,
	    error_number);
    oprintf (type_of_error_condition [severity_level], error_number);

M$RESTORE();

/* Update State based on Object */

#endif
}



/*
 *   PDLI_crm_get()
 */

BOOLEAN pdli_crm_get()
#ifdef DEBUG
   {
    oprintf ("%%pdli_crm_get\n");
    return(FALSE);
   }
#else
   {
    return(FALSE);
   }
#endif




/*
 *   PROCESS_SET_COPY()
 */

VOID process_set_copy(no_copies)
UWORD no_copies;
#ifdef DEBUG
   {
    oprintf ("%%process_set_copy, num_copies: %d\n", no_copies);
   }
#else
   {
   }	/* Translator should consider implementing this someday */
#endif




/*
 *   PROCESS_DECVEC()
 */

VOID process_decvec(vec_ptr)
PDECVEC vec_ptr;
#ifdef DEBUG
   {
    oprintf("%%dispose_decvec, vec_start_x: %d, vec_start_y: %d, vec_end_x: %d, vec_end_y: %d, vec_thickness: %d\n", 
	    vec_ptr->dv_ap.xval,
	    vec_ptr->dv_ap.yval,
	    vec_ptr->dvend.xval,
	    vec_ptr->dvend.yval,
	    vec_ptr->thickness); 
   }
#else
   {
    dispose_decvec(vec_ptr);
   }
#endif



/*
 *   PROCESS_SET_SSIZE()
 */

VOID process_set_ssize(sss_ptr)
PSHEET_SIZE sss_ptr;
#ifdef DEBUG
   {
    oprintf ("%%process_set_ssize, tray_num: %d, slot_num: %d\n",
	     sss_ptr->tray_num,
	     sss_ptr->slot_num
	    );
    oprintf ("%%sheet_width: %d, sheet_length: %d, sheet_orient: %d\n",
	     sss_ptr->sheet_width,
	     sss_ptr->sheet_length,
	     sss_ptr->sheet_orient
	    );
   }
#else
   {
   }
#endif



/*
 *   PROCESS_SET_SSIZE()
 */

VOID process_logical_page(length,width)
LONG	length,width ;

   {
    /*
    ** For the ANSI Xlator, only the length is needed to adjust the system.
    */
    cg_st.cgst_sheet_len = length ;
    
   }



/*
 *   PROCESS_RESERVE_MEM()
 */

VOID process_reserve_mem(mem_type, mem_value)
UBYTE mem_type;
UWORD mem_value;
#ifdef DEBUG
   {
    oprintf("%%process_reserve_mem, mem_type: %d, mem_value: %d\n",
	    mem_type,
	    mem_value
           );
   }
#else
   {
   }
#endif

    

/*
 *   PROCESS_TRAY_SELECT
 */

VOID process_tray_select(tray_number)
WORD tray_number;
#ifdef DEBUG
   {
    oprintf ("%%process_tray_select, tray_number: %d\n", tray_number);
   }
#else
   {
    /* Normal Code, what process_tray_select should do */ 
    /* set current tray to tray number passed */
    cg_st.cgst_paper_tray = tray_number;


    /*	Optimization, instead of always checking whether
     *	the desired tray (cg_st.cgst_paper_tray) is the
     *	same as the current tray (ps_st[stack_ptr].psst_paper_tray)
     *	Ensure that they are equal upon transitions
     */

     if (cur_ps_st->psst_paper_tray != cg_st.cgst_paper_tray)
       {

         /* Close all current states. */
         M$CLOSE_ALL();

         /* Wipe out the current downloaded fonts. */
         dispose_cleanup();

         /* Restore to PREAMBLE level to output the setpapertray command. */
         while (stack_ptr > PREAMBLE )
           {
              M$RESTORE();
           }
         if (cg_st.cgst_paper_tray == TRAY_DEV_DEF)
           {
              oprintf (str_resdefpapertray);
           }
         else
           {
              oprintf (str_setpapertray, cg_st.cgst_paper_tray);
           }

         /* Save to page level. */
         while (stack_ptr < PAGE)
           {
             M$SAVE();
           }
         ps_str (str_scale);
	 cur_ps_st->psst_paper_tray = cg_st.cgst_paper_tray;
	 }
   }
#endif



/*
 * pdli_nvm_store
 */

BOOLEAN pdli_nvm_store (target_buffer, length)
PUB target_buffer;
WORD length;

#ifdef DEBUG
   {
    oprintf ("%%pdli_nvm_store, Buffer Pointer: %d, length: %d\n",
	     target_buffer,
	     length
	    );
    return (TRUE);
   }
#else
   {
    return (TRUE);
   }
#endif


/*
 * pdli_get_sheet_orientation
 */

ORIENTATION pdli_get_sheet_orientation ()
#ifdef DEBUG
   {
    return (ORIENT_PORT);
   }
#else
   {
    return (ORIENT_PORT);
   }
#endif


/*
 * pdli_com_start
 */

VOID pdli_com_start()

#ifdef DEBUG
   {
    oprintf ("%%pdli_com_start\n");
   }
#else
   {
   }
#endif


/*
 * pdli_com_stop
 */

VOID pdli_com_stop()

#ifdef DEBUG
   {
    oprintf ("%%pdli_com_stop\n");
   }
#else
   {
   }
#endif


/*
 * pdli_com_put_string
 */

VOID pdli_com_put_string(string_ptr, C1_disabled)
PUB string_ptr;
BOOLEAN C1_disabled;

#ifdef DEBUG
   {
    oprintf ("%%pdli_com_put_string, String Pointer: %d, C1 Disabled Flag: %d\n",
	     string_ptr,
	     C1_disabled
	    );
   }
#else
   {
   }
#endif


/*
 * pdli_com_put_byte
 */

VOID pdli_com_put_byte(id_char)
UBYTE id_char;

#ifdef DEBUG
   {
    oprintf ("%%pdli_com_put_byte, ID Char: %d\n", id_char);
   }
#else
   {
   }
#endif



/*
 *   PDLI_clean_up_host_memory
 */

VOID pdli_clean_up_host_memory()
   {
    /*
    **  The required match for deleting a file is 0 characters.
    **  Thus all the files will be deleted.
    */
    font_delete_font_files(0, NULL) ;
   }

/************************************************************************
 *  Set_font_and_spacing
 *
 *  This routine is called when the current font is not the desired
 *  font, either wrong font or wrong spacing.
 *
 *  First it ensures that the font is downloaded in the font cache, 
 *  then if necessary ensures that a properly spaced copy of the 
 *  font exists in teh spaced font cache, 
 *
 *  Finally, it selects the desired font (or spaced font)
 *  to be the current font)
 *
 ***********************************************************************/

set_font_and_spacing (font_number,spacing)

FONT_NUMBER font_number;
WORD spacing;
{

FONT_NUMBER final_box;	    /* Number of the "box" containing
			     * the desired font in its final form, 
			     * either spaced or unspaced
			     */

    /* if the font is not currently downloaded, then do it */
    if ( vax_font_table_box_number [font_number]== END_OF_DICTIONARY )
        {
        vax_font_table_box_number [font_number]=
	    dispose_add_font_to_cache(font_number) + NUMBER_OF_FIXED_FONT_BOXES;
        } ;


    if (spacing == 0) 
	{
	    /* no need to respace font, use it as is */ 
	    final_box = vax_font_table_box_number[font_number];
	}
	else
	{   /* need to respace font, search thrue spaced cache for a match */ 
	    for (final_box = 0; final_box < SPACING_FONT_TABLE_SIZE; final_box++)
	        {
	            if (    (spacing_font_table [final_box] .space_font == 
					 vax_font_table_box_number[font_number])
			&&  (spacing_font_table [final_box] .spacing == spacing)
			&&  (spacing_font_table [final_box] .space_active == ACTIVE)
		       )
			break;
	        } /* end for loop, searching for match */

	    /* If not found, create a spaced copy */	
	    if  (final_box == SPACING_FONT_TABLE_SIZE)
		{   /* no, create a spaced copy */
		    final_box =	dispose_add_spacing_to_cache (font_number,
			    vax_font_table_box_number[font_number],spacing);
		}
	    final_box += SPACING_OFFSET;
	} /* end else (spacing != 0) */

    /*	Whether or not we had to download the font, 
     *	Whether or not we had to respace the font, 
     *	by now final_box contains the proper value
     */

     oprintf (str_courierfind, final_box);		    /* Activate the font */
							    /* Update current state */
     cur_ps_st -> curchar.char_data.char_font = 
				    vax_font_table_box_number[font_number];  
     /* Note, the number stored in cu_ps_st must be the above.
      * It cannot be the VAX font number (font_number), as several vax fonts may be stored
      * in a box, to avoid changing box when switching font.
      * It cannot  be the spaced box number (final_box), as there is no way to relate
      * this to the data coming with a character (font_number)
      */
     cur_ps_st -> curchar.font_data.horizontal_spacing = spacing;


}


/*
 *
 *  dispose_delete_font_from_cache
 *
 *  Note this code does not deal with pairing on the PS side.
 *  when pairing on the PS side is implemented, this code will 
 *  have to be revisited
 */

dispose_delete_font_from_cache (font_number)
FONT_NUMBER font_number;
{
    if	(vax_font_table_box_number [font_number]!= END_OF_DICTIONARY)   
	{
	    dispose_delete_box_from_cache (vax_font_table_box_number [font_number]- NUMBER_OF_FIXED_FONT_BOXES);
	}


}

