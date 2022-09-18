#ifndef lint
static char SccsId[] = "  @(#)caparse.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: caparse.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,
 *	1986, 1987, 1988, 1989.  ALL RIGHTS RESERVED.
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
 *  28-JAN-1988 14:33  mhw
 *      Initial Version of combination of duparse and caparse
 *
 *   3-FEB-1988 08:22  mhw
 *      Add pr_text, taken from dumpu.c
 *
 *  16-FEB-1988 09:35 mhw
 *      Add camac.lib_hc for non-dump oprintf
 *
 *   3-MAR-1988 14:52 mhs
 *      Fix arithmetic typecasting for expressions, equations, etc.
 *
 *  18-MAR-1988 16:05 mhs
 *      Add actual call to enter graphics mode in DEC_GR.
 *
 *  23-MAR-1988 11:17 mhs
 *      Restore code that does above and below baseline.
 *
 *  13-APR-1988 12:55 mhs
 *      Move dec_gr to its rightful place in castate module
 *      as new "ac_dec_gr" routine.
 *
 *  11-MAY-1988 16:38 mhs
 *	Remove "pr_nul" as it is not supported by the VAX.  We now
 *	do an "ac_nop" instead, when we receive a NULL.
 *
 *  14-MAY-1988 17:48 mhs
 *	Remove pr_xon and pr_xoff as they are not used now that real
 *	control rendition code is in place.
 *
 *  19-MAY-1988 14:33 mhs
 *	Add pr_char_crm as special handler for printing control
 *	rendition characters (normal text and BOLD acronyms).
 *
 *  24-MAY-1988 16:53 mhs
 *	Fix pr_sub so that it calls pr_text of SP for now, vs.
 *	reprocessing through the state tables (which didn't
 *	work).  Eventually, once pr_text is rewritten, call
 *	pr_text of VIR_CHAR_BLOB instead.
 *
 *  28-JUL-1988 14:04  mhw
 *	Rewrite pr_text
 *
 *  29-NOV-1988 12:45 ejs
 *	Modified pr_text to use a table lookup instead of get_width.  Also
 *	turned test_ssf into M$TEST_SSF so it would be included as inline 
 *	code.
 *
 *  30-NOV-1988 15:00 mhw
 *	Change pr_space to call process_char instead of process_vchar
 *  end edit_history
 *
 *   2-DEC-1988 16:17 araj
 *	Install cfont_caching, modified test_ssf (M$TESTSSF), to check validity
 *	of g_table before using
 *
 *   8-DEC-1988 11:54 ejs
 *	Modified pr_text to use gset info for check_ap.  To use structure copy
 *	for font metrics. Eliminated temp1,temp2.
 *
 *  16-DEC-1988 16:04 ejs
 *	More changes that affect pr_text and space.  Created fast path routines.
 *
 *  18-DEC-1988 15:19 ejs
 *		Major changes to data structutes.  GLYPH has been modified
 *		to allow struct copies more easily.  The PS output is not
 *		changed (barring bugs of course).
 *
 *-----------------------------------------------------------
 */



/*
 *-----------------------------------------------------------
 *
 *   begin description
 *
 *   Filename:  caparse.c
 *
 *   Dump Utility and real version
 *
 *   This module includes
 *
 *   pr_can()
 *   pr_sub()
 *   pr_sxl_sub()
 *   pr_space()
 *   pr_del()
 *   pr_xa0()
 *   pr_xff()
 *   pr_st()
 *   pr_text()
 *   pr_text_crm()
 *   pr_char_crm(bold_flag)
 *   pr_ctrl_crm(index)
 *   pr_c0_crm()
 *   pr_c1_crm()
 *   pr_crnr_crm()
 *
 *   pr_esc()
 *   pr_dcs()
 *   pr_csi()
 *   pr_osc()
 *   pr_pm()
 *   pr_apc()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */


/* Translator edit history of this file 
 *
 * file:	xlc_pars.c - Translator parser routines
 * created:	laf	 13-FEB-1986 15:18:21 
 * edit:	gh	 3-APR-1986 11:05:14 Removed check_parm() call, replaced
 *			 with set/reset_mode_ansi() calls, added pr_ssu() call
 *		gh	 9-APR-1986 09:34:08 Added test for rmf in pr_text()
 *		gh	 11-APR-1986 15:11:52 Added call to ris()
 *		laf	 17-APR-1986 15:45:54 Added routines for DCS 
 *			 processing: dcs_chkcc(), dcs_ignore_seq(),
 *			 dcs_char_seq(), dcs_sixel_seq()
 *			 Added C1 control chars to  
 *			 Simplified "chkcc" routine 
 *			 Created new "pr_dcsseq" routine
 *			 Simplified "gr_chkcc" routine
 *		gh	 18-APR-1986 11:18:19 Changing the names of the sixel
 *			 routines called. Adding set_c1rcv_mode() & 
 *			 reset_c1rcv_mode()
 *		laf	 23-APR-1986 12:26:33 Changed call from pr_dcs() to
 *			 pr_dcsseq().
 *		laf	 24-APR-1986 13:21:16 Added call to decvec ()
 *		gh	 30-APR-1986 16:54:12 Changed chkcc() to include the
 *			 DEL character.  Added to pr_sub() output of a 'blob'
 *		lf/araj  30-APR-1986 19:09:59 Modified dcs_chkcc() to
 *			 exit current mode on c1 controls and CAN, SUB, ESC
 *			 Removed exit_current_mode from pr_can, pr_sub,
 *			 and pr_esc; modified range check for c1 controls
 *			 in dcs_chkcc and gr_chkcc; modified gr_chkcc () to
 *			 exit_current_mode on C1, CAN and ESC (but NOT on 
 *			 SUB!) 
 *		gh	 2-MAY-1986 07:52:44 Adding Code to pr_text() for
 *			 font dictionary support. Added decoding for:
 *			 ls1r, ls1, ls2, ls2r, ls3, ls3r, ss2, ss3
 *		laf	 8-MAY-1986 11:29:49 Modifying pr_text to include
 *			 use of gset data; eliminate dependence on calls to 
 *			"add_to_jfy" and "dispose_of_char" to return
 *			 character width and do it explicitly instead
 *		gh	 8-MAY-1986 18:38:29 Changing nrcs_table references
 *			 to nrcs_tables
 *		gh	 4-JUN-1986 13:33:41 Changing in pr_text() the call
 *			 dispose_of_char(&xl_st); to (&xl_st.curchar);
 *		nv	 13-JUN-1986 11:18:26 modified pr_text to properly 
 *			 handle incoming 7 bit character codes of 0x20 and 0x7f.
 *		nv	 27-JUN-1986 14:29:01 deleted code to assign xl_st
 *			.g_table [g_set] .glyph_fontdata .algorithmic_attributes
 *			to xl_st .curchar .requested_attributes.
 *			Removed obsolete code from pr_space.
 *		laf     15-JUL-1986 11:48:46 In gr_chkcc(), changed case SUB:
 *			to move 0x3f into "c", call gr_sixel (c), and reset
 *			action=A_CONTINUE.  These things were making the
 *			SUB char not repeat when it followed the '!' cmd.
 *		laf     29-JUL-1986 14:43:19  Modified cseq to not accept
 *			any private chars except '?'.  Modified pr_cseq and
 *			pr_escseq to check the "syntax" flag (until now, 
 *			lots of code sets that flag, but nobody every 
 *			used it).  Reorganized param() for clarity.  Took
 *			all references to "pdef" out of this module because
 *			it is no longer used by anyone.
 *		nv	 30-JUL-1986 13:26:33 Delete function calls to get 
 *			G0-G3 assigned to GL, GR and 'xl_st.ssf' and put 
 *			software to do these things into pr_escseq and
 *			pr_cont_char. 
 *			Make changes to pr_text to reflect the declaration
 *			changes of .gl & .gr to .gl_ptr & .gr_ptr.
 *		laf	1-AUG-1986 15:55:37 Added the line "pstate = S_TEXT"
 *			to "exit_current_mode" in the case of S_GRAPHICS,
 *		        so that state would revert back to text mode when
 *			graphics is terminated by receipt of
 *			a C1 control code.
 *		nv	 15-AUG-1986 09:50:39 Added comments to pr_text 
 *			referencing appropriate sections of the VSRM for
 *			handling A/0 and F/F when a character set of 94
 *			characters is mapped through GR. 
 *		laf	20-AUG-1986 10:38:06 Added 0xff to the control
 *			chars checked for by chkcc() (relative to QAR 524)
 *			so that an 0xff in the protocol selector
 *			of a graphics escape sequence would be ignored.
 *		laf/araj 25-AUG-1986 17:56:03 Rearranged sixel_seq for
 *			efficiency; took second parameter out of call to
 *			gr_repeat; fixed repeat problem ("-" repeating as
 *			a sixel code instead of a control char).
 *		laf/araj 25-AUG-1986 19:14:05 Put gr_repeat and pr_graphics
 *			inline.
 *		laf 	 28-AUG-1986 14:33:41 Changed all occurrences of 
 *			the variable "c" to "c7" in gr_param().
 *		laf	 29-AUG-1986 10:09:17 Changed "pvalue" in param()
 *			to a ULONG
 *		laf 	 29-AUG-1986 10:13:51 Added 3a to the list of 
 *			invalid characters in a control sequence (cseq()).
 *		nv	  29-AUG-1986 15:16:23 Changed the definition of
 *			'max_buff_size' from 'double' to 'LONG'.
 *		nv	 16-SEP-1986 16:04:28 When an <ESC>c is parsed, call a 
 *			non 'power_up' kind of ris.
 *		mgb	 23-SEP-1986 10:58:41 Fixed sixel repeat so that a CR
 *			in the middle of the repeat command does not cause an
 *			error by resetting the repeat count.
 *		nv	 1-OCT-1986 12:12:23 In 'pr_text' add logic to ensure 
 *			that space character codes are printed as blobs for
 *			nonexistent character sets; and add substantial 
 *			comment explaining the logic.
 *			Remove the variable 'graphics_left_table' and its 
 *			determination, and instead calculate its value only
 *			when and where it is needed.
 *			Re-optimized the code in 'pr_text' considering the 
 *			above changes.
 *		nv	 10-OCT-1986 10:10:38 put calls to 'compute_font' in
 *			'exit_current_mode' in two places: one for when a
 *			'decdtff' is exitted and one for when a 'declff' is
 *			exitted. 
 *		nv	 16-OCT-1986 16:53:12 Moved "pstate = S_STORECHAR;"
 *			statement from the end of 'case ~' and 'case }' in
 *			'pr_dcsseq' to the end of 'case ~' and 'case }' in
 *			'enter_dcs_store'. 
 *			Moved "pstate = S_STORESIXEL;" statement from the end
 *			of 'case y' in 'pr_dcsseq' to the end of 'case y' in
 *			'enter_dcs_store'. 
 *
 *		araj	 21-OCT-1986 16:06:36 added call to 
 *			empty_jfy when entering graphics mode
 *
 *		araj	 22-OCT-1986 19:41:35 added a special 
 *			test to invalidate control sequences
 *			if we receive several "private" in a row
 *			There was a test for private after
 *			numeric, private after intermediate
 *			but no test for private after private.
 *
 *		araj	 24-OCT-1986 19:12:07 Modified pr_sub
 *			to also set C7, as now both C and C7 
 *			are expected by pr_text
 *		mgb	 10-DEC-1986 13:06:17 Added two parameters to the call
 *			to check_ap, above and below baseline.
 *              kws	 17-MAR-1987 16:59:29 Add DCS to assign user
 *			preference character set.
 *              kws      18-MAR-1987 22:51:13 Free DCS string memory on an
 *                      invalid assign or delete font DCS in dcs_store_char().
 *
 *		mgb	 2-JUL-1987 10:08:38 In sixel_seq () corrected test
 *			for digit and ';' to exclude ':', '<', '=' and '>'.
 *			Those char should be handled as control chars not
 *			as parameters or parameter delimeters.
 *
 *	
 *		araj	 16-JUL-1987 23:03:31 Modified processing of
 *			7f, a0 ff, to llow for 96 char sets to be mapped 
 *			in GL, in compliance with a memo from TIM LASKO, 
 *			superseding the SRM, and allowing 96 char sets 
 *			in GL
 *
 *		ejs	13-APR-1989 09:25 pr_sxl_sub had to  be taught to
 *			check for expanded tables since it wants to go 
 *			back upstream and reference the state tables
 */



#define caparse (1)


/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"	    /* First Level State Tables			*/
#include    "cpglobal.hc"   /* global parser defs			*/
#include    "caglobal.hc"   /* global action routine defs		*/
#include    "camac.lib_hc"  /* CARs macros                              */

#ifdef DUMP
#include    "dumpu.hc"      /* Dump Utility                             */
#include    "xlc_iface.hc"  /* defines for oprintf			*/
#endif DUMP

/*  end   include_file    */

/*  The following macros were once subroutines.  TEST_SSF was only used locally,
**  CHECK_AP was shared with cagraphics.c.  For speed reasons they are now 
**  macros.  The usage of check_ap by cagraphics was a little warped anyway, so
**  it was seperated.
*/

#define M$RIGHT_MARGIN_CHECK()\
    if	(   (!xl_st.justify_mode) \
	&&  ((xl_st.curchar.ap.xval + xl_st.curchar.char_data.char_width) > \
					xl_st.h_lim_bound.max)\
	)  \
	{\
\
	    M$CHECK_AP(	-(xl_st.curchar.font_data.above_baseline_offset+1),xl_st.curchar.font_data.below_baseline_offset);\
\
	} /* end if "not jfy and beyond right margin */


/*****  check_ap(above,below)  ******************************************
 *									*
 *	check_ap(above,below) - Check active position			*
 *				above - above baseline			*
 *				below - below baseline			*
 ************************************************************************/

#define	M$CHECK_AP(above,below)\
\
{\
\
    /* If the current ahp is less than the left margin, reset it */\
    if (xl_st.curchar.ap.xval < xl_st.h_lim_bound.min)\
    	xl_st.curchar.ap.xval = xl_st.h_lim_bound.min;\
\
    /* IF JUSTIFY MODE NOT ON, Check if printing the next character will cause \
     * ahp to exceed right margin.  If so, wrap or truncate depending on \
     * setting of wrap mode.  \
     */\
    if	(   (!xl_st.justify_mode) \
	&&  ((xl_st.curchar.ap.xval + xl_st.curchar.char_data.char_width) > \
					xl_st.h_lim_bound.max)\
	)  \
	{\
	    if  (xl_st.wrap_mode)	\
		{\
		    /* is VAI valid, or did a font change invalidate it */\
		    if  (xl_st.vai_valid == FALSE)\
			{\
			    compute_vai();\
			}\
			\
		    /* Update .avp to value of new line, do <FF> if necessary */    \
\
		    vert_rel_w_wrap(xl_st.vai);\
		    hpos_abs(xl_st.h_fmt_bound.min);\
		    xl_st.plf = NOPLUPLD;	/* Clear PLU/PLD flag */\
\
		} /* end if wrap mode */\
    		else	\
		{\
		    xl_st.rmf = TRUE;\
		} /* end else for if wrap mode */\
\
	} /* end if "not jfy and beyond right margin */\
\
    /* If the current avp is less than the top margin, set it to the top */\
\
    if ((xl_st.curchar.ap.yval < xl_st.v_lim_bound.min) && \
        (xl_st.plf <= NOPLUPLD))\
    	xl_st.curchar.attr_data.attr_baseline_offset = xl_st.curchar.ap.yval = xl_st.v_lim_bound.min;\
\
    if (xl_st.fcf)\
       {\
	xl_st.curchar.ap.yval += above;\
	xl_st.fcf = FALSE;\
	xl_st.curchar.attr_data.attr_baseline_offset = xl_st.curchar.ap.yval;\
       }\
\
    /* possible wrap condition?   */\
    /* no sub/superscript         */\
    /* are the margins too close? */\
    if ( ((xl_st.curchar.ap.yval + below) > xl_st.v_lim_bound.max) \
      && (xl_st.plf >= NOPLUPLD)\
      && (xl_st.curchar.ap.yval - xl_st.v_lim_bound.min > above) )\
	{\
		pr_ff ();	\
	    	xl_st.curchar.ap.yval += above;\
		xl_st.curchar.attr_data.attr_baseline_offset = \
						xl_st.curchar.ap.yval;\
		xl_st.fcf = FALSE;\
	}\
}

/****************************************************************************
    test_ssf - Test Single Shift Flag

******************************************************************************/


#define	M$TEST_SSF()  \
\
    /*\
     * If the single shift flag has a non-zero value, use that value to\
     * designate the G set and reset the single shift flag; otherwise,\
     * depending on whether the incoming character code is greater than\
     * 127 or not, select GR or GL respectively.\
     */\
\
    if (xl_st.ssf)\
       {\
	g_set_pointer = &xl_st.g_table[xl_st.ssf];\
	xl_st.ssf = 0;\
       }\
    else\
       {\
	g_set_pointer = (cp_c & BIT7) \
	   ? (xl_st.gr_ptr)\
	   : (xl_st.gl_ptr);\
       }\
    if	(g_set_pointer->gset_valid == FALSE)\
	{\
	    compute_font_for_g_set ( g_set_pointer - (&xl_st.g_table[0]));\
	}

VOID test_ssf()
{
M$TEST_SSF()
}


/********************************************************************
pr_text

    This will move the character code to the curchar GLYPH in
    XLATOR_STATE and call the composer function to print the
    input character.  

    NOTE: it is assumed the character is printable, and is not of
	    special significance, e.g., a form feed, del,sub....

*********************************************************************/

VOID pr_text()
   {

#ifdef DUMP
   {
    oprintf("%c", cp_c);	    /* print the input character */
   }
#endif
    M$TEST_SSF(); /* Test for single shift flag */


    /* 
     * Fill in the character code of the current character 
     */
    xl_st.curchar.char_data = g_set_pointer->gset_map[cp_c7] ;

    /* 
     * Fill in the rest of the fields of the current character 
     */
    xl_st.curchar.font_data = g_set_pointer->gset_fontdata ;


    /* Check the active position of the outgoing character */
    M$CHECK_AP
       (
	-(xl_st.curchar.font_data.above_baseline_offset+1),
         xl_st.curchar.font_data.below_baseline_offset
       );

    /* If justifying, add current char to justify buffer */
    if (xl_st.justify_mode)
       {
	add_to_jfy_buf (&xl_st.curchar);
       }

    /* If not justifying, AND the right margin flag is set,
     * do not output the character; otherwise, dispose of it.
     */
    else  
       {
	if (xl_st.rmf)
	   {
	    return;
	   }
    	process_char (&xl_st.curchar);
       }
    /*
     * Adjust the translator's active horizontal position to reflect 
     * the width of the char just printed 
     */
    xl_st.curchar.ap.xval += xl_st.curchar.char_data.char_width;
   }



/********************************************************************
pr_fast_text
*********************************************************************/

VOID pr_fast_text()
   {
    /* 
     * Fill in the character code of the current character 
     */
    xl_st.curchar.char_data = g_set_pointer->gset_map[cp_c7] ;

    M$RIGHT_MARGIN_CHECK() ;

    /* If justifying, add current char to justify buffer */
    if (xl_st.justify_mode)
       {
	add_to_jfy_buf (&xl_st.curchar);
       }

    /* If not justifying, AND the right margin flag is set,
     * do not output the character; otherwise, dispose of it.
     */
    else  
       {
	if (xl_st.rmf)
	   {
	    return;
	   }
    	process_char (&xl_st.curchar);
       }
    /*
     * Adjust the translator's active horizontal position to reflect 
     * the width of the char just printed 
     */
    xl_st.curchar.ap.xval += xl_st.curchar.char_data.char_width;
   }


/***********************************************************************
    Cancel function 
*********************************************************************/

VOID pr_can()
   {
#ifdef DUMP
   {
    cp_exit_cur_mode();
    oprintf("CAN \n");
   }
#endif
    cp_exit_cur_mode();
   }




/***********************************************************************
    Substitute Dump Utility function 
*********************************************************************/

VOID pr_sub()
   {
    WORD temp1, temp2;

#ifdef DUMP
   {
    cp_exit_cur_mode();
    oprintf("SUB \n");
   }
#endif
    cp_exit_cur_mode();
    cp_c = VIR_CHAR_BLOB;
    cp_c7 = cp_c & CP_7BIT_MASK;

    M$TEST_SSF(); /* Test for single shift flag */

    /* 
     * Fill in the character code of the current character 
     */
    xl_st.curchar.char_data = g_set_pointer->gset_map[cp_c7] ;

    /* 
     * Fill in the rest of the fields of the current character 
     */
    xl_st.curchar.font_data = g_set_pointer->gset_fontdata ;

    /* Check the active position of the outgoing character */
        /* Check the active position of the outgoing character */
    M$CHECK_AP
       (
	-(xl_st.curchar.font_data.above_baseline_offset+1),
         xl_st.curchar.font_data.below_baseline_offset
       );

    /* If justifying, add current char to justify buffer */
    if (xl_st.justify_mode)
       {
	add_to_jfy_buf (&xl_st.curchar);
       }

    /* If not justifying, AND the right margin flag is set,
     * do not output the character; otherwise, dispose of it.
     */
    else  
       {
	if (xl_st.rmf)
	   {
	    return;
	   }
    	process_vchar (&xl_st.curchar);
       }

    /*
     * Adjust the translator's active horizontal position to reflect 
     * the width of the char just printed 
     */
    xl_st.curchar.ap.xval += xl_st.curchar.char_data.char_width;
   }



/***********************************************************************
    SXL Substitute 
*********************************************************************/

VOID pr_sxl_sub()
   {
#ifdef DUMP
   {
    oprintf("SXL SUB \n");
   }
#endif
    cp_c = FIRST_SIXEL_CODE;	/* Select the blank sixel char */
    cp_c7 = cp_c & CP_7BIT_MASK;


	if (cp_ctptr->encoding == EXPANDED_TABLE)
	    {
	    (*cp_tok_tbl[((WORD*)cp_ctptr->ch_array[0])[cp_c]])();
	    }
	else
	    {
	    cp_token = cp_search_2tab(cp_ctptr->ch_array[2]); /* srch 2nd level gl */
	    (*cp_tok_tbl[cp_token])();	/* call action requested */
	    }
   }




/***********************************************************************
    Print a Space
*********************************************************************/

VOID pr_space ()
   {
    WORD temp1, temp2;

#ifdef DUMP
   {
    oprintf("SP \n");
   }
#endif
    M$TEST_SSF(); /* Test for single shift flag */


    /* If c is a space code x020, then it should be printed as a space.
     *
     * But the translation tables could have made temp1 be:
     *
     *		x020 if the GL set is stored in the left half of the
     *		PS font
     *
     *		x0a0 if the GL set is stored in the right half of the
     *		PS font
     *
     *		x01d if the GL set does not exist in the proper style
     *
     * The first and last case are no problems, the second case is an issue
     * as the bitmap for the space is stored at x020 in the PS font.
     * We must therefore undo the conversion.
     *
     * If c is an x0a0, then it should be printed:
     *
     *		as is x0a0, if a 96 character set is stored in GR
     *
     *		as a blob x01d if a 94/95 character set is stored in GR.
     *
     * The conversion table could have made temp1 be an x020, 
     * if the GR set is stored in the left half of the PS font.
     * In this case we must force temp1 to be a blob
     *
     * This is in compliance with the VSRM (referenced by the PSRM) section
     * 3.6.3, for the case when a character set of 94 characters is mapped
     * through GR. 
     *
     * Modified 07/16/87 by ARAJ, to comply with memo from Tim LASKO 
     * superseding the SRM. Now mapping a 96 char set in GL is valid
     */

    /* 
     * Fill in the character code of the current character 
     */
    xl_st.curchar.char_data = g_set_pointer->gset_map[cp_c7] ;

    /* 
     * Fill in the rest of the fields of the current character 
     */
    xl_st.curchar.font_data = g_set_pointer->gset_fontdata ;

    /* Check the active position of the outgoing character */
    M$CHECK_AP
       (
	-(xl_st.curchar.font_data.above_baseline_offset+1),
         xl_st.curchar.font_data.below_baseline_offset
       );

    /* If justifying, add current char to justify buffer */
    if (xl_st.justify_mode)
       {
	add_to_jfy_buf (&xl_st.curchar);
       }

    /* If not justifying, AND the right margin flag is set,
     * do not output the character; otherwise, dispose of it.
     */
    else  
       {
	if (xl_st.rmf)
	   {
	    return;
	   }
    	process_char (&xl_st.curchar);
       }

    /*
     * Adjust the translator's active horizontal position to reflect 
     * the width of the char just printed 
     */
    xl_st.curchar.ap.xval += xl_st.curchar.char_data.char_width;
   }



/********************************************************************
pr_fast_space
*********************************************************************/

VOID pr_fast_space()
   {

    /* 
     * Fill in the character code of the current character 
     */
    xl_st.curchar.char_data = g_set_pointer->gset_map[cp_c7] ;


    M$RIGHT_MARGIN_CHECK() ;

    /* If justifying, add current char to justify buffer */
    if (xl_st.justify_mode)
       {
	add_to_jfy_buf (&xl_st.curchar);
       }

    /* If not justifying, AND the right margin flag is set,
     * do not output the character; otherwise, dispose of it.
     */
    else  
       {
	if (xl_st.rmf)
	   {
	    return;
	   }
    	process_char (&xl_st.curchar);
       }
    /*
     * Adjust the translator's active horizontal position to reflect 
     * the width of the char just printed 
     */
    xl_st.curchar.ap.xval += xl_st.curchar.char_data.char_width;
   }


/***********************************************************************
    Print Delete
*********************************************************************/

VOID pr_del()
   {
    WORD temp1, temp2;

#ifdef DUMP
   {
    oprintf("DELETE \n");
   }
#endif
    M$TEST_SSF(); /* Test for single shift flag */

    /* If the first 7 bits of the incoming character are 0x7f (i.e., the DEL 
     * code), return (i.e., do nothing) unless it is mapped through the 
     * graphics right table and that table implements a character set of 96 
     * characters. This is in compliance with the VSRM (referenced by the     
     * PSRM) section 3.6.4, for the case when a character set of 94 characters
     * is mapped through GR.
     *
     * Modified 07/16/87 by ARAJ, to comply with memo from Tim LASKO, 
     * superseding the SRM, 7f is valid if a 96 char set is mapped in GL
     */
    if ( (cp_c7 == DEL) && (!g_set_pointer->repertory) )
       /* -> 94 character set */
       {
        return;  /* i.e., NOP */
       }

    /* 
     * Fill in the character code of the current character 
     */
    xl_st.curchar.char_data = g_set_pointer->gset_map[cp_c7] ;

    /* 
     * Fill in the rest of the fields of the current character 
     */
    xl_st.curchar.font_data = g_set_pointer->gset_fontdata ;

    /* Check the active position of the outgoing character */
    M$CHECK_AP
       (
	-(xl_st.curchar.font_data.above_baseline_offset+1),
         xl_st.curchar.font_data.below_baseline_offset
       );

    /* If justifying, add current char to justify buffer */
    if (xl_st.justify_mode)
       {
	add_to_jfy_buf (&xl_st.curchar);
       }

    /* If not justifying, AND the right margin flag is set,
     * do not output the character; otherwise, dispose of it.
     */
    else  
       {
	if (xl_st.rmf)
	   {
	    return;
	   }
    	process_char (&xl_st.curchar);
       }

    /*
     * Adjust the translator's active horizontal position to reflect 
     * the width of the char just printed 
     */
    xl_st.curchar.ap.xval += xl_st.curchar.char_data.char_width;
   }



/***********************************************************************
    a0h function 
*********************************************************************/

VOID pr_xa0()
{
    WORD temp1, temp2;

#ifdef DUMP
   {
    cp_exit_cur_mode();
    oprintf("XA0 \n");
   }
#endif
    M$TEST_SSF(); /* Test for single shift flag */


    /* 
     * Fill in the character code of the current character 
     */
    xl_st.curchar.char_data = g_set_pointer->gset_map[cp_c7] ;

    /* 
     * Fill in the rest of the fields of the current character 
     */
    xl_st.curchar.font_data = g_set_pointer->gset_fontdata ;


    /* If c is a space code x020, then it should be printed as a space.
     *
     * But the translation tables could have made temp1 be:
     *
     *		x020 if the GL set is stored in the left half of the
     *		PS font
     *
     *		x0a0 if the GL set is stored in the right half of the
     *		PS font
     *
     *		x01d if the GL set does not exist in the proper style
     *
     * The first and last case are no problems, the second case is an issue
     * as the bitmap for the space is stored at x020 in the PS font.
     * We must therefore undo the conversion.
     *
     * If c is an x0a0, then it should be printed:
     *
     *		as is x0a0, if a 96 character set is stored in GR
     *
     *		as a blob x01d if a 94/95 character set is stored in GR.
     *
     * The conversion table could have made temp1 be an x020, 
     * if the GR set is stored in the left half of the PS font.
     * In this case we must force temp1 to be a blob
     *
     * This is in compliance with the VSRM (referenced by the PSRM) section
     * 3.6.3, for the case when a character set of 94 characters is mapped
     * through GR. 
     *
     * Modified 07/16/87 by ARAJ, to comply with memo from Tim LASKO 
     * superseding the SRM. Now mapping a 96 char set in GL is valid
     */
    if (cp_c == XA0)
       {
        if (!g_set_pointer->repertory )
           {
	    xl_st.curchar.char_data.char_code = VIR_CHAR_BLOB;
	   }
       }


    /* Check the active position of the outgoing character */
    M$CHECK_AP
       (
	-(xl_st.curchar.font_data.above_baseline_offset+1),
         xl_st.curchar.font_data.below_baseline_offset
       );

    /* If justifying, add current char to justify buffer */
    if (xl_st.justify_mode)
       {
	add_to_jfy_buf (&xl_st.curchar);
       }

    /* If not justifying, AND the right margin flag is set,
     * do not output the character; otherwise, dispose of it.
     */
    else  
       {
	if (xl_st.rmf)
	   {
	    return;
	   }
    	process_char (&xl_st.curchar);
       }

    /*
     * Adjust the translator's active horizontal position to reflect 
     * the width of the char just printed 
     */
    xl_st.curchar.ap.xval += xl_st.curchar.char_data.char_width;

    cp_exit_cur_mode();
   }



/***********************************************************************
    FFh function 
*********************************************************************/

VOID pr_xff()
{
    WORD temp1, temp2;

#ifdef DUMP
   {
    cp_exit_cur_mode();
    oprintf("XFF \n");
   }
#endif
    M$TEST_SSF(); /* Test for single shift flag */



    if ( (cp_c7 == DEL) && (!g_set_pointer->repertory) )
       /* -> 94 character set */
       {
        return;  /* i.e., NOP */
       }


    /* 
     * Fill in the character code of the current character 
     */
    xl_st.curchar.char_data = g_set_pointer->gset_map[cp_c7] ;

    /* 
     * Fill in the rest of the fields of the current character 
     */
    xl_st.curchar.font_data = g_set_pointer->gset_fontdata ;

    /* Check the active position of the outgoing character */
    M$CHECK_AP
       (
	-(xl_st.curchar.font_data.above_baseline_offset+1),
         xl_st.curchar.font_data.below_baseline_offset
       );

    /* If justifying, add current char to justify buffer */
    if (xl_st.justify_mode)
       {
	add_to_jfy_buf (&xl_st.curchar);
       }

    /* If not justifying, AND the right margin flag is set,
     * do not output the character; otherwise, dispose of it.
     */
    else  
       {
	if (xl_st.rmf)
	   {
	    return;
	   }
    	process_char (&xl_st.curchar);
       }

    /*
     * Adjust the translator's active horizontal position to reflect 
     * the width of the char just printed 
     */
    xl_st.curchar.ap.xval += xl_st.curchar.char_data.char_width;

    cp_exit_cur_mode();
   }



/***********************************************************************
    String Terminator Dump Utility function 
*********************************************************************/

VOID pr_st()
   {
#ifdef DUMP
   {
    cp_exit_cur_mode();
    oprintf("ST \n");
   }
#endif
    cp_exit_cur_mode();
   }




/****************************************************************************
    Control Rendition - Print Character (Modified from Print_Text)
******************************************************************************/

VOID pr_char_crm(attr)
WORD attr;
   {
    WORD temp1, temp2;
    GSET *g_set_pointer;

    /*
     * Depending on whether the incoming character code is greater than
     * 127 or not, select GR or GL respectively.
     */
    g_set_pointer = (cp_c & BIT7) 
       ? (xl_st.gr_ptr)
       : (xl_st.gl_ptr);

    if (g_set_pointer->gset_valid == FALSE)
	{
	    compute_font_for_g_set ( g_set_pointer - (&xl_st.g_table[0]));
	}




    /* 
     * Fill in the character code of the current character 
     */
    xl_st.curchar.char_data = g_set_pointer->gset_map[cp_c7] ;

    /* 
     * Fill in the rest of the fields of the current character 
     */
    xl_st.curchar.font_data = g_set_pointer->gset_fontdata ;

    /* 
     * Fill in the rest of the fields of the current character 
     * (Note that if the bold flag is true, the attributes are
     * BOLD; otherwise they come from the font).
     */
    xl_st.curchar.font_data.algorithmic_attributes = attr;

    /* Check the active position of the outgoing character */
    M$CHECK_AP
       (
	-(xl_st.curchar.font_data.above_baseline_offset+1),
         xl_st.curchar.font_data.below_baseline_offset
       );

    /* If the right margin flag is set, print New Line and dispose of it;
     * otherwise, just dispose of it.
     */
    if (xl_st.rmf)
       {
        pr_nel();
       }
    process_char (&xl_st.curchar);

    /*
     * Adjust the translator's active horizontal position to reflect 
     * the width of the char just printed 
     */
    xl_st.curchar.ap.xval += xl_st.curchar.char_data.char_width;
   }



/****************************************************************************
    Control Rendition - Print Text
******************************************************************************/

VOID pr_text_crm()
   {
#ifdef DUMP
   {
    oprintf("%c", cp_c);	/* print the input character */
   }
#endif
    pr_char_crm(NO_ATTR);	/* print using no attributes */
   }



/****************************************************************************
    Control Rendition - Print C0/C1 Acronym Utility Routine
******************************************************************************/

VOID pr_ctrl_crm(index)
UBYTE index;
   {
    WORD i;	/* index into acronym string */

    i = 0;		    /* initialise the string array index */
    cp_c = cp_c7 = '<';	    /* get the first character */

    while (cp_c7 != STRING_TERMINATOR)
       {
	pr_char_crm(BOLD);  /* print the current character BOLD */
        cp_c = cp_c7 = acronyms[index][i++]; /* get the next character */
       }

    cp_c = cp_c7 = '>';	    /* get the last character */
    pr_char_crm(BOLD);	    /* print the last character BOLD */
   }



/****************************************************************************
    Control Rendition - Print C0 Acronym
******************************************************************************/

VOID pr_c0_crm()
   {
    pr_ctrl_crm(cp_c7);
   }



/****************************************************************************
    Control Rendition - Print C1 Acronym
******************************************************************************/

VOID pr_c1_crm()
   {
    pr_ctrl_crm(cp_c7 + NUM_C0_CODES);
   }



/****************************************************************************
    Control Rendition - Print Corner Characters Acronym (SP, DEL, XA0, XFF)
******************************************************************************/

VOID pr_crnr_crm()
   {
    switch (cp_c)
       {
        case SP_C: pr_ctrl_crm(NUM_CTRL_CODES);
                   break;
        case DEL:  pr_ctrl_crm(NUM_CTRL_CODES + 1);
                   break;
	case XA0:  pr_ctrl_crm(NUM_CTRL_CODES + 2);
		   break;
	case XFF:  pr_ctrl_crm(NUM_CTRL_CODES + 3);
		   break;
        default:   break;
       }
   }



/***********************************************************************
    Escape Introducer
***********************************************************************/

VOID pr_esc()
   {
    cp_exit_cur_mode();		/* Exit the current state */
    cp_reset();			/* Reset the param and inter buffers */    
    cp_setctptr(&ast_esc);	/* Set State to ESC */
   }



/***********************************************************************
    Device Control String Introducer
***********************************************************************/

VOID pr_dcs()
   {
    cp_exit_cur_mode();		/* Exit the current state */
    cp_reset();			/* Reset the param and inter buffers */    
    cp_setctptr(&ast_dcs);	/* Set State to DCS */
   }



/***********************************************************************
    Control Sequence Introducer
***********************************************************************/

VOID pr_csi()
   {
    cp_exit_cur_mode();		/* Exit the current state */
    cp_reset();			/* Reset the param and inter buffers */    
    cp_setctptr(&ast_csi);	/* Set State to CSI */
   }



/***********************************************************************
    Operating System Command 
***********************************************************************/

VOID pr_osc()
   {
    cp_exit_cur_mode();		/* Exit the current state */
    cp_reset();			/* Reset the param and inter buffers */    
    cp_setctptr(&ast_dcsignore); /* Set State to DCS Ignore */
   }



/***********************************************************************
    Privacy Message
***********************************************************************/

VOID pr_pm()
   {
    cp_exit_cur_mode();		/* Exit the current state */
    cp_reset();			/* Reset the param and inter buffers */    
    cp_setctptr(&ast_dcsignore); /* Set State to DCS Ignore */
   }



/***********************************************************************
    Application Program Command 
***********************************************************************/

VOID pr_apc()
   {
    cp_exit_cur_mode();		/* Exit the current state */
    cp_reset();			/* Reset the param and inter buffers */    
    cp_setctptr(&ast_dcsignore); /* Set State to DCS Ignore */
   }

