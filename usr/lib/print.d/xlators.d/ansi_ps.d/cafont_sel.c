#ifndef lint
static char SccsId[] = "  @(#)cafont_sel.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cafont_sel.c
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
 *   begin edit_history
 *
 *  araj   9-MAR-1988 14:22
 *	Creation by splitting CAFONT.C
 *
 *  araj  10-MAR-1988 14:37
 *	Added CFONT (compute font)
 *
 *  mhs   15-MAR-1988 16:02
 *      Typecast all arithmetic expressions
 *
 *  mhw	    21-APR-1988 09:33
 *	Added code to super and sub script routines
 *	changed pr_gsm to call do_gsm
 *
 *  mhs	    24-MAY-1988 07:58
 *	Add default to switch statements in ATFF/DTFF, and
 *	implement the new parameter value of Ps = 3.
 *	Fix numerous bugs in ATFF and DTFF, including change
 *	to SGR structure that has changed the compute_font
 *	interface.
 *	Pad the font id string with zeroes for all unused
 *	fields, different for each ATFF parameter value.
 *	This is especially important because byte 29 is
 *	"part" of byte 8, and can't be left null.
 *
 *  mhw	     8-JUL-1988 09:02
 *	Changes for conversion of cp_pbuf from WORD to LONG
 *
 *  30-NOV-1988 14:28 araj
 *	Install cfont caching, change calls to compute font into calls to invalidate_font
 *	Make sure references to vai or g_table have a test for the validity of the data
 *	first
 *
 *   1-DEC-1988 11:02 ejs
 *	Eliminated use of cp_final_char.  Using cp_c.
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
 *   Filename:  cafont_sel.c
 *
 *
 *   This module includes
 *
 *  pr_gss()
 *  pr_gsm()
 *  do_gsm()
 *  dec_atff_enter() dec_atff_term()
 *  dec_dtff_enter() dec_dtff_term()
 *  pr_si()    pr_so()
 *  pr_ss2()   pr_ss3()
 *  pr_ls2()   pr_ls3()
 *  pr_ls1r()  pr_ls2r()  pr_ls3r()
 *  pr_scs()
 *  dec_aupss_enter() dec_aupss_term()
 *  dec_rqupss()
 *  pr_ascef1()  pr_ascef2() pr_ascef3()
 * ca_font_init ()
 * b36_to_bin()
 * nbin_to_b36()
 * bin_to_b36()
 * cfont_type_family()
 * cfont_spacing()
 * cfont_type_size()
 * cfont_scale_factor()
 * cfont_style()
 * cfont_weight()
 * cfont_proportion()
 * cfont_rotation()
 * cfont_cset()
 * cfont_id_type_family()
 * cfont_id_spacing()
 * cfont_id_type_size()
 * cfont_id_scale_factor()
 * cfont_id_attributes()
 * cfont_id_weight()
 * cfont_id_proportion()
 * cfont_id_rotation()
 * cfont_id_cset()
 * cfont_id_csubset()
 * cfont_id_encoding()
 * cfont_id_resolution()
 * cfont_id_reserved()
 * compute_font()
 * blob()
 * cfont_explode_attributes()
 * cfont_grade_font()
 * compute_font_for_g_set()
 * cfont_transform_index()
 * cfont_horiz_spacing()
 * init_g_table ()
 * 
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/* Translator edit history for these routines.
 *
 * file:	xlc_font.c - Translator font routines
 * created:	gh	 26-MAR-1986 09:10:31
 * edit:
 *	araj	 9-MAR-1988 14:25   edit history removed, makes little
 *		sense after split
 */



#define cafont_sel	(1)

/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"	    /* ANSI State Table & Token Table Declarations */
#include    "cpglobal.hc"   /* globals for CP modules */
#include    "caglobal.hc"   /* global defs for the CARs */
#include    "camac.lib_hc"  /* non-dump oprintf */


#ifdef  DUMP
#include    "dumpu.hc"	    /* Dump Utility                             */
#include    "xlc_iface.hc"  /* defines for oprintf			*/
#endif DUMP

/*  end   include_file    */


/******************************************************************************
    Do the Graphics Size Modification
    Called from pr_gsm and dec_super, dec_sub and dec_super_sub_off
*****************************************************************************/

VOID do_gsm(char_height, char_width)
UWORD char_height,char_width;
   {
    xl_st.v_size = ((LONG)xl_st.last_gss * (LONG)char_height) / 100L;
    xl_st.character_proportion = char_width;
    invalidate_font();
   }


/******************************************************************************
    Graphic Size Selection
*****************************************************************************/

VOID pr_gss()

   {
#ifdef DUMP
   {
    oprintf("GSS \n");
    pprint();
   }
#endif
    xl_st.last_gss = xl_st.v_size = cp_pbuf[0] * (LONG)xl_st.sizeunit;
    xl_st.character_proportion = 100;	/* !!! Is this correct ?? !!! */
    invalidate_font();
   }


/******************************************************************************
    Graphics Size Modification
*****************************************************************************/

VOID pr_gsm()

   {
#ifdef DUMP
   {
    oprintf("GSM \n");
    pprint();
   }
#endif

    if (cp_pbuf[0] == 0) cp_pbuf[0] = 100;
    if (cp_pbuf[1] == 0) cp_pbuf[1] = 100;


    /* send the height and width to do_gsm that will
     * do the work 
     */
    do_gsm((UWORD)cp_pbuf[0], (UWORD)cp_pbuf[1]);
   }


/******************************************************************************
    Reset the DCS counters/vars
*****************************************************************************/

VOID ca_restart_dcs()
   {
    /* init dcs command string length, repeat count, font name buffer
     * count and modulo-three counter 
     */
    cp_font_name_cnt = 0;   /* used by ATFF/DTFF/AUPSS but not LFF */
    cmd_str_len = 0;	    /* used by LFF only */
    modulo = 0;		    /* used by LFF only */
   }


/******************************************************************************
    Initialise the DCS introducer and vars; called by DTFF, ATFF, LFF and AUPSS
*****************************************************************************/

VOID ca_start_dcs()
   {
    /* save the string introducer 
     */
    save_dcs_introducer ();

    /* init dcs command string length, repeat count, font name buffer
     * count and modulo-three counter 
     */
    ca_restart_dcs();
   }


/******************************************************************************
    DECATFF identified, about to begin loading of ID STRING - Dump Utility
*****************************************************************************/

VOID dec_atff_enter()

   {
#ifdef DUMP
   {
    oprintf("DECATFF_ENTER \n");
   }
#endif
    /* Set up maximum allowed length of font name,
     * for use by cp_font_name routine 
     */
    amt_allocated = CP_MAX_FONT_CHAR;
   }


/******************************************************************************
    DECATFF ST found - Dump Utility
*****************************************************************************/

VOID dec_atff_term()

   {
    UBYTE string_length;
    WORD sel_type;
    WORD sgr_modified;

#ifdef DUMP
   {
    oprintf("DECATFF_TERM \n");
   }
#endif
    sgr_modified = dcs_intro.ps2 - FIRST_SGR_TABLE_ENTRY;

    /* If we didn't receive any font data, we have a null 
     * decatff command, which calls for reinitializing
     * the designated sgr table entry.
     */
    if (cp_font_name_cnt == 0)
       {
	xl_st.sgr_tbl [sgr_modified].selection_type = 
           cfont_def_sgr [sgr_modified].selection_type;
        strcpy (xl_st.sgr_tbl [sgr_modified].id_string, 
	   cfont_def_sgr [sgr_modified].id_string);
        invalidate_font ();
        return;		/* done processing current ATFF */
       }

    /* Get the correct string length depending on if incoming
     * name is a font or type family ID, then check if ID 
     * string is too short.
     */
    switch (dcs_intro.ps1) 
       {
	case 0:
	case 1:  string_length = FIDS_FONT_12;
		 sel_type = SGR_FONT_12;
		 break;
	case 2:  string_length = FIDS_TYPE_FAMILY;
		 sel_type = SGR_TYPE_FAMILY;
		 break;
	case 3:  string_length = FIDS_FONT_16;
		 sel_type = SGR_FONT_16;
		 break;
	default: ABORT_MACRO(94);   /* Invalid DCS string */
		 cp_setctptr(&ast_dcsignore); /* set state to DCS IGNORE */
		 return;	    /* do not process the ID string */
		 break;
       }

    if ( (cp_font_name_cnt < string_length) && (cp_font_name_cnt != 0) )
       {
	ABORT_MACRO(91); /* Invalid ID for DECATFF */
	cp_setctptr(&ast_dcsignore); /* set state to DCS IGNORE */
	return;	/* do not process the ID string */
       }

    /* Once we know string is not too short, pad the remaining
     * bytes with ANSI zeroes so that compute_font will not
     * get garbage for byte 29 or other currently unused
     * fields that are valid at the dispose level.
     */
    strcpy (&cp_font_name_buf[string_length],&font_id_blank[string_length]);

    /* Now that error checking on string is complete and
     * string has been null-terminated, proceed ...
     */
    strcpy (xl_st.sgr_tbl [sgr_modified].id_string, &cp_font_name_buf[0]);
    xl_st.sgr_tbl [sgr_modified].selection_type = sel_type;
    invalidate_font ();
   }



/******************************************************************************
    DECDTFF identified, about to begin loading of ID String- Dump Utility
*****************************************************************************/

VOID dec_dtff_enter()

   {
#ifdef DUMP
   {
    oprintf("DECDTFF_ENTER \n");
   }
#endif
    /* Set up maximum allowed length of font name,
     * for use by cp_font_name routine 
     */
    amt_allocated = CP_MAX_FONT_CHAR;
   }



/******************************************************************************
    DECDTFF ST found - Dump Utility
*****************************************************************************/

VOID dec_dtff_term()

   {
    UBYTE string_length;

#ifdef DUMP
   {
    oprintf("DECDTFF_TERM \n");
   }
#endif

    /* Get the correct string length based on whether
     * string is type family, font file or font ID, then 
     * check that the ID string is not too short before 
     * passing it on to compute_font.
     */
    switch (dcs_intro.ps1) 
       {
	case 0:	 string_length = TYPE_FAMILY_ID_SIZE;
		 break;
	case 1:	 string_length = FONT_FILE_ID_SIZE;
		 break;
	case 3:  string_length = FONT_ID_SIZE;
		 break;
	default: ABORT_MACRO(94);   /* Invalid DCS string */
		 cp_setctptr(&ast_dcsignore); /* set state to DCS IGNORE */
		 return;	    /* make sure we ignore rest of sequence */
		 break;
       }

    if (cp_font_name_cnt < string_length) 
       {
	ABORT_MACRO(92);    /* Invalid ID for DECDTFF */
	cp_setctptr(&ast_dcsignore); /* set state to DCS IGNORE */
	return;	    /* Do not process the ID string */
       }

    /* Now that error checking on string is complete,
     * proceed ...
     */
    empty_jfy_buf ();

    /* It is not necessary to ensure that the string is terminated as
     * font_delete_font_files works with specific string lengths only.
     */
    FNT_DELETE_FONT_FILES (string_length, &cp_font_name_buf[0]);
    invalidate_font();
   }


/****************************************************************************
    Shift In/ Locking Shift 0 function
******************************************************************************/

VOID pr_si()

   {
#ifdef DUMP
   {
    oprintf("SI \n");
   }
#endif
    xl_st.gl_ptr = &xl_st.g_table [0];	
   }



/****************************************************************************
    Shift Out/ Locking Shift 1 function
******************************************************************************/

VOID pr_so()

   {
#ifdef DUMP
   {
    oprintf("SO \n");
   }
#endif
    xl_st.gl_ptr = &xl_st.g_table [1];	
   }


/****************************************************************************
    Single Shift 2 Dump Utility function
******************************************************************************/

VOID pr_ss2()

   {
#ifdef DUMP
   {
    cp_exit_cur_mode();
    oprintf("SS2 \n");
   }
#endif
    xl_st.ssf = 2;	
    cp_exit_cur_mode();
   }


/****************************************************************************
    Single Shift 3 Dump Utility function
******************************************************************************/

VOID pr_ss3()

   {
#ifdef DUMP
   {
    cp_exit_cur_mode();
    oprintf("SS3 \n");
   }
#endif
    xl_st.ssf = 3;	
    cp_exit_cur_mode();
   }


/****************************************************************************
    Locking Shift 2 Dump Utility function
******************************************************************************/

VOID pr_ls2()

   {
#ifdef DUMP
   {
    oprintf("LS2 \n");
   }
#endif
    xl_st.gl_ptr = &xl_st.g_table [2];	
   }


/****************************************************************************
    Locking Shift 3 Dump Utility function
******************************************************************************/

VOID pr_ls3()

   {
#ifdef DUMP
   {
    oprintf("LS3 \n");
   }
#endif
    xl_st.gl_ptr = &xl_st.g_table [3];
   }


/****************************************************************************
    Locking Shift 1 Right Dump Utility function
******************************************************************************/

VOID pr_ls1r()

   {
#ifdef DUMP
   {
    oprintf("LS1R \n");
   }
#endif
    xl_st.gr_ptr = &xl_st.g_table [1];
   }


/****************************************************************************
    Locking Shift 2 Right Dump Utility function
******************************************************************************/

VOID pr_ls2r()

   {
#ifdef DUMP
   {
    oprintf("LS2R \n");
   }
#endif
    xl_st.gr_ptr = &xl_st.g_table [2];
   }


/****************************************************************************
    Locking Shift 3 Right Dump Utility function
******************************************************************************/

VOID pr_ls3r()

   {
#ifdef DUMP
   {
    oprintf("LS3R \n");
   }
#endif
    xl_st.gr_ptr = &xl_st.g_table [3];	
   }


/****************************************************************************
    Select Charact Set Dump Utility function
******************************************************************************/

VOID pr_scs()

{
    WORD	j;			/* g_table index */
    WORD	new_character_set_id;	/* generated here in 16 bit
                                                      integer form */
#ifdef DUMP
    {
    cp_exit_cur_mode();
    oprintf("SCS \n");
    }
#endif
/*
dump_g_table ();
*/
    /* Parse out which g_table [j] is being addressed and the
    new_character_set_size (94 or 96 glyphs) from the parameter sequence. 
    Compute new_character_set_id */ 
    j = cp_ibuf[0] & 3;
    
    if (cp_ibuf[0] & 4)
      {
      new_character_set_id = 1<<13;
      xl_st .g_table[j] .repertory = C96;
      }
    else
      {
      new_character_set_id = 0;
      xl_st .g_table[j] .repertory = C94;
      }
    
    if (cp_icnt == 2)
      new_character_set_id += (1<<11) + ( (cp_ibuf[1] & 15) <<7 );
    
    new_character_set_id += cp_c;   /*  the final character */
    
      xl_st .g_table[j] .char_set_id = new_character_set_id;

      if (new_character_set_id == USER_PREFERENCE)
        {
        new_character_set_id = xl_st .user_pref_cset;
        xl_st .g_table [j] .repertory = xl_st .user_pref_repertory;
        }

    invalidate_font_for_g_set(j); /* this also does invalidate_vai(); */
    cp_exit_cur_mode();
} /* end of pr_scs */


/****************************************************************************
    Assign User Preference Set functions        
******************************************************************************/

VOID dec_aupss_enter()

   {
#ifdef DUMP
   {
    oprintf("DECAUPSS-ENTER \n");
   }
#endif
    /* Set up maximum allowed length of font name,
     * for use by cp_font_name routine 
     */
    amt_allocated = MAXCSETID;
   }


/*****  dec_aupss_term()  **********************************************
 * Process a new Use Preference Character Set				*
 ************************************************************************/

VOID dec_aupss_term()

{
WORD	i;	/* for-table indices */
WORD	new_user_preference_id;	/* generated here in 16 bit integer form */
WORD	new_user_preference_repertory;

#ifdef DUMP
    {
    oprintf("DECAUPSS-TERM \n");
    pprint();
    }
#endif
/* Initialize the 16 bit integer user preference id to ensure that
   no unnecessary bits are set.	*/

new_user_preference_id = 0;

/* Fill in the DECAUPSS control sequence data intermediate and 
   final characters. */

/*    Compute new_user_preference_id */

if (cp_font_name_cnt == 1) 
    {new_user_preference_id = cp_font_name_buf[0];
	    /* Only one char means no intermed; just use the final */
    }
else if (cp_font_name_cnt == 2)
    {
     new_user_preference_id = (1 << 11) /* Intermed in used flag */
       + 
	((cp_font_name_buf[0] & 0x0f) << 7) + (cp_font_name_buf[1]);
	    /* buf[0] is Intermed, buf[1] is Final */
    }

   if (dcs_intro.ps1 != 0)		/* If 96-char vs. 94-char set */
   {
    new_user_preference_id += 1<<13;
    new_user_preference_repertory = C96;
   } 
   else 
   {					/* else 94-char vs. 96-char set */
    new_user_preference_id += 0<<13;
    new_user_preference_repertory = C94;
   }

   xl_st .user_pref_cset = new_user_preference_id;
   xl_st .user_pref_repertory = new_user_preference_repertory;

/* For each G set which is designated as 'user preference' determine anew
   the setting of its font table entry flags and then re-compute its font.
*/
for (i = 0; i < G_TABLE_SIZE; i++)
  {
   if (xl_st .g_table [i] .char_set_id == USER_PREFERENCE)
      {
       xl_st .g_table [i] .repertory = xl_st .user_pref_repertory;
      }
  }
  invalidate_font();
}    /* end of dec_aupss_term */


/*************************************************************************
    Request User Preference Supplemental Character Set
************************************************************************/

VOID dec_rqupss()

{
#ifdef DUMP
    {
    oprintf("DECRQUPSS \n");
    }
#endif
}


/*************************************************************************
    Announce Subset Code Extension Format  - Level 1 Dump Utility
************************************************************************/

VOID pr_ascef1()

{
#ifdef DUMP
    {
    oprintf("ASCEF1 \n");
    }
#endif
	/* Load ASCII into G0. */
	cp_ibuf[0] = '(';
	cp_c = 'B';
	pr_scs ();

	/* Load G0 into GL. */
	xl_st .gl_ptr = &xl_st .g_table [0];

	/* Load ISO Latin 1 into G1. */

	cp_ibuf[0] = '-';
	cp_c = 'A';
	pr_scs  ();

	/* Load G1 into GR. */
	xl_st .gr_ptr = &xl_st .g_table [1];

}



/*************************************************************************
    Announce Subset Code Extension Format  - Level 2 Dump Utility
************************************************************************/

VOID pr_ascef2()

{
#ifdef DUMP
    {
    oprintf("ASCEF2 \n");
    }
#endif
    pr_ascef1();	/* as far as LN04/etc. are concerned, ascef1 is the
		           the same as ascef2 */
}



/*************************************************************************
    Announce Subset Code Extension Format  - Level 3 Dump Utility
************************************************************************/

VOID pr_ascef3()

{
#ifdef DUMP
    {
    oprintf("ASCEF3 \n");
    }
#endif
	/* Load ASCII into G0. */
	cp_ibuf[0] = '(';
	cp_c = 'B';
	pr_scs  ();

	/* Load G0 into GL. */
	xl_st .gl_ptr = &xl_st .g_table [0];

}

