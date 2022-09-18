#ifndef lint
static char SccsId[] = "  @(#)cafont_forms.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cafont_forms.c
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
 *   9-MAR-1988 15:30 mhw
 *	Initial Version, split from cafont.c
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
 *   Filename:  cafont_forms.c
 *
 *
 *   This module includes
 *
 * dec_mac_enter()  dec_mac_font()  dec_mac_sxl()  dec_mac_term()
 * dec_spp()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */



/*
 *-----------------------------------------------------------
 *
 *   begin edit_history from cafont.c
 *
 *   001    22-JAN-1988 17:24 mhw
 *      Added dump file of dufont.c and ducrend.c to translator routines.
 *      #ifdefs separate the dump routines from the real routines.
 *      ducrend routines moved here since were here for the translator.
 *
 *   002    25-JAN-1988 14:01 bf
 *	Started filling out stubbed action routines.  Extract from
 *	switch in original action routine.
 *
 *   003    15-FEB-1988 11:05  mhw
 *      Add include of camac.lib for non-dump oprintf
 *
 *   004   15-FEB-1988 17:11 mhs
 *      Move enter_dcs_store code into specific routines for ATFF,
 *      DTFF, LFF and AUPSS.
 *
 *   end edit_history
 *
 *-----------------------------------------------------------
 */



/* Translator edit history for these routines.
 *
 * file:	xlc_font.c - Translator font routines
 * created:	gh	 26-MAR-1986 09:10:31
 * edit:
 *	gh	 2-MAY-1986 08:10:53
 *		Adding font_init() stub
 *	nv	 5-MAY-1986 10:31:00
 *		Adding the following routines:
 *			declff ()
 *			delete_font_files ()
 *			tabulate_deleted_font_file ()
 *			tabulate_new_font_file ()
 *			update_font_table_and_sgr_flags ()
 *			create_new_paired_font ()
 *			new_ps_font_number ()
 *			decdtff ()
 *			decatff ()
 *			update_sgr_font_tbl_entry_flag ()
 *			blob ()
 *			compute_font ()
 *			compute_font_for_g_set ()
 *		and the following tables:
 *			paired_char_set_table
 *			ps_font_table
 *		and about 50 literals
 *	nv	 7-MAY-1986 19:15:00
 *		Adding the following routines:
 *			init_font_dictionaries ()
 *			init_font_table ()
 *			init_width_table ()
 *			init_left_bearing_table ()
 *			init_right_bearing_table ()
 *			init_paired_char_set_table ()
 *			compute_vai ()
 *		the following literals:
 *			NUMBER_OF_PS_FIXED_FONT_DICTNRS
 *			NUMBER_OF_PS_FONT_DICTIONARIES
 *			NUMBER_OF_8_BIT_CHARACTERS
 *			FIRST_DOWNLOADED_FONT
 *		and the following external declarations:
 *			width_table [ NUMBER_OF_PS_FONT_DICTIONARIES ]
 *                                            [ NUMBER_OF_8_BIT_CHARACTERS ]
 *			left_bearing_table [ NUMBER_OF_PS_FONT_DICTIONARIES ]
 *                                            [ NUMBER_OF_8_BIT_CHARACTERS ]
 *			right_bearing_table [ NUMBER_OF_PS_FONT_DICTIONARIES ]
 *                                            [ NUMBER_OF_8_BIT_CHARACTERS ]
 *	nv	 8-MAY-1986 11:30:00
 *		Adding the following routines:
 *			init_height_table
 *			init_min_table
 *			init_max_table
 *			init_ps_font_table
 *			init_sgr_tbl
 *			init_g_table
 *		and the following external declarations:
 *			height_table [ NUMBER_OF_PS_FONT_DICTIONARIES ]
 *			min_table [ NUMBER_OF_PS_FONT_DICTIONARIES ]
 *			max_table [ NUMBER_OF_PS_FONT_DICTIONARIES ]
 *		and to font_table adding the following substructure items:
 *			min
 *			max
 *	nv	 30-MAY-1986 11:30:00
 *		Removed the init_font_dictionary routine by merging its contents
 *		into the font_init routine.
 *		Defined ERROR_MACRO.
 *		Replaced [undeveloped] references to ERROR_CODE with 
 *		ERROR_MACRO.
 *		Significantly rewrote compute_font_for_g_set to refine the
 *		selection of font file for type families.
 *	gh	 8-JUN-1986 10:51:51 In compute_font_for_g_set() added
 *		'if (font_value1 [i] .whole == DEAD)  continue;' after the FOR
 *		loop that followed the comment:
 *		"Eliminate all font files except those with equal or smaller
 *		spacing or character proportion. Also Enhance all font files 
 *		with the correct kind of spacing, i.e., proportionally spaced
 *		or monospaced.  Note that no font files are eliminated based 
 *		on this property."
 *	nv	 18-JUN-1986 09:42:33 Replaced download_font_to_ps_machine
 *		with dispose_font and restructured so that setup_metric_tables
 *		immediately preceeds download_font_to_ps_machine. Incorporated
 *		preliminary ISO software changes, i.e., mainly shuffled nrcs
 *		transformation indices.
 *	nv	 26-JUN-1986 21:16:48 Redesigned compute_font_for_g_set and
 *		dump_font_value to employ a new three part font_value which
 *		provides, inherently in its design, an automatic fall-back
 *		capability. 
 *		Deactivated ERROR_MACRO and all printf's and dump's.
 *	nv	 27-JUN-1986 14:57:04 Renamed requested_attributes
 *		xl_st .requested_attributes. everywhere used.
 *		Removed local declarations of 'requested_attributes.'
 *		Removed superfluous initialization of 'requested_attributes.'
 *	nv	 27-JUN-1986 21:47:06 Included compute_proportional_mode
 *		in sgr and decatff.
 *		Incorporate blob metrics into tables.
 *	nv	 1-JUL-1986 11:30:47 Make initialization of right_bearing_table
 *		to '0' for all [monospaced] fixed fonts.
 *		Make initialization of superscript_height_table and
 *		subscript_height_table to be half of the height_table entry
 *		[total vertical size] for the fixed fonts.
 *		So that multiple gms (Graphics Size Modification) commands do
 *		not have a cummulative effect, make and maintain a copy of the
 *		last gss (Graphic Size Selection) parameter [called
 *		last_gss_size]. All gms commands are with respect to the last
 *		gss. 
 *	nv	 9-JUL-1986 17:48:53 Enabled a bug in sgr to allow algorithmic 
 *		altering of attributes when fonts are selected by font id. 
 *	nv	 10-JUL-1986 14:33:44 Deleted 'copy_font' stub.
 *		Changed 'copy_font' to 'dispose_copy_font'.
 *	nv	 10-JUL-1986 17:52:10 Redesigned 'horiz_spacing' function.
 *		Removed redundant initialization of xl_st .propl_mode.
 *		In compute_font_for_g_set() revised the way xl_st .propl_mode
 *		is used and added comments.
 *	nv	 22-JUL-1986 12:58:02 Added provisions for deleted font not
 *		being garbaged collected by the PS machine. Basically, once a
 *		font table entry is used it may deactiveated (when the font
 *		file is deleted but it cannot be reused. 
 *		Defined new literal: DELETED_FONT_FILES_ARE_REMOVED FALSE
 *		Added and cleaned up some comments.
 *	nv	 22-JUL-1986 16:07:47 Added provisions for disabling the 
 *		pairing of font files where the coping of a fixed font files
 *		is required (until this functionality is available in the PS
 *		machine). 
 *		Defined new literal: FIXED_FONT_FILES_CAN_BE_COPIED FALSE
 *		Added and cleaned up some comments.
 *	nv	 29-JUL-1986 16:00:07 deleted references to VIRTUAL_CHAR_BLOB;
 *		Replaced them with references to VIR_CHAR_BLOB.
 *	nv	 29-JUL-1986 17:39:00 Changed init_font_table and 
 *		init_width_table to use 'CPI' literals.
 *	nv	 30-JUL-1986 10:52:06 Deleted the assignments of G0-G3 to 
 *		GL and GR.
 *	gh	 1-AUG-1986 15:57:05 Modified init_font_table to initialize the
 *		.font_file_id's to '\0' of all table entries above FIRST_
 *		DOWNLAODED_FONT up to FONT_TABLE_SIZE
 *
 *	araj/gh	 1-AUG-1986 17:46:22 
 *		Added initialization of height_table for DLL fonts
 *		to prevent invalid font being defined.
 *
 *	araj	 1-AUG-1986 19:11:14 
 *		Changed #if 0 into #if dump.....
 *		to make it easier to enable/disable dumps
 *	nv	 6-AUG-1986 12:33:31 
 *		Modified the following initialization procedures so that ALL 
 *		tabular info is initialized instead of that just pertinent to 
 *		fixed fonts:
 *			init_font_table,
 *			init_width_table,
 *			init_paired_char_set_table,
 *			init_max_table,
 *			init_min_table,
 *			init_total_height_table,
 *			init_above_baseline_table, 
 *			init_superscript_height_table, 
 *			init_subscript_height_table, and 
 *			init_average_width_table.
 *		Updated print statement which describes the bit structure of
 *		the font values.
 *	nv	 15-AUG-1986 15:43:49 Fixed sgr for handling the absence of
 *		arguments.
 *	nv	 27-AUG-1986 09:38:52 Fixed bug in 'tabulate_new_font_file' to
 *		allow downloaded font files of character set which are
 *		identical to the user preference character set to be
 *		associated with g_sets (G0-G3) which are user preference.
 *		Disabled 9-JUL-1986 bug in 'compute_font_for_g_set' and 'sgr' 
 *		to allow algorithmic altering of attributes when fonts are
 *		selected by font id. 
 *		Removed definition of ERROR_REPORT to dbug.h
 *		Removed definition of NUMBER_OF_PS_FONT_DICTIONARIES and where
 *		used changed the spelling to NUMBER_OF_PS_DICTIONARIES to as 
 *		to use the definition in xlate.h.
 *		Removed definition of MAX_CSET_SIZE to xlate.h.
 *	laf	 27-AUG-1986 14:44:11 Added xl_st.curchar.ul_avp = 
 *		xl_st.curchar.avp to sgr() case 4 (for UL/PLU/PLD fix)
 *	nv	 28-AUG-1986 15:33:53 Made the evaluation of font_table [i] 
 *		.spacing conditionally dependent upon dll_spacing in 
 *		'update_font_table_and_sgr_flags'.
 *		Changed the definition of dll_spacing from UBYTE to UWORD.
 *		Added comments to highlight the fact that font_table [i] 
 *		.spacing is used as a boolean as well as a value.
 *	gh	 1-SEP-1986 09:57:49 Changed declaration of font_table[].
 *		point_size to UWORD from WORD.
 *	nv	 2-SEP-1986 17:16:11 Within setup_metric_tables changed all
 *		'MAX_CSET_SIZE' references to 'MAX_CSET_SIZE - 1'.
 *		Adjusted the declared size of dll_width_tbl 
 *		made font_table_struct a proper 'typedef'.
 *		Added the following structure items to FONT_TABLE_STRUCT
 *			LONG	space_width,
 *				ul_thickness,
 *				ul_offset,
 *				strikethru_thickness,
 *				strikethru_offset,
 *				virtual_memory_needs;
 *			BYTE	*bitmaps_ptr;
 *		Provided for the initialization for all of the new structure
 *		items.
 *		Implemented the evaluations for all but the last two of the
 *		new structure items.
 *		Removed the LOCAL restriction on font_table.
 *	nv	 3-SEP-1986 10:17:27 Added font_table entry as third parameter 
 *		in all calls to 'dispose_font'.
 *		Made 'total_portrait_bytes' and 'number_of_odd_words_in_bitmaps'
 *		accessable an external declaration.
 *		Implemented the evaluation for
 *		font_table [i] .virtual_memory_needs based on
 *		'total_portrait_bytes' and 'number_of_odd_words_in_bitmaps'.
 *		Made 'font_bitmap' and 'dll_char' accessable external
 *		declaration.
 *		Implemented the evaluations for all but the last of the
 *		'font_bitmap' structure items.
 *		Included within 'declff' memory allocation for the bitmaps of
 *		each downloaded bitmap.
 *		Made a new procedure, called 'clean_up_host_memory', which 
 *		frees all allocated memory.
 *		Implemented the evaluation for the last of the 'font_bitmap'
 *		structure items, i.e., 'bitmap_ptr'.
 *	nv	 4-SEP-1986 08:50:10 streamlined main for-loop, (i.e., with 
 *		respect to 'i') within 'setup_metric_tables'.
 *		Replaced 'declff' and 'tabulated_deleted_font_file' with 
 *		redesigned versions to implement caching of fonts.
 *		Created 'vm_wasted' and 'vm_being_used' as global variables
 *		to support caching of fonts.
 *		Initialize 'vm_wasted' and 'vm_being_used' in 'font_init'.
 *		Defined the following literals to support caching of fonts:
 *			FONT_VM_OVERHEAD
 *			TOTAL_VIRTUAL_MEMORY
 *			VIRTUAL_MEMORY_SAFETY_MARGIN
 *		Moved define of 'FONT_TABLE_STRUCTURE' to xlate.h.
 *		Made 'ps_font_table' global.
 *		Removed definition of FIRST_DOWNLOADED_FONT and
 *		PS_FONT_TABLE_SIZE; they are now defined in 'xlate.h'.
 *	nv	 5-SEP-1986 12:06:19 Removed the following literal definitions
 *		(they are now defined in 'xlate.h'.):
 *			NUMBER_OF_8_BIT_CHARACTERS,
 *			FONT_FILE_ID_SIZE,
 *			FIRST_DOUBLE_PS_DOWNLOADED_FONT'
 *			NUMBER_OF_PS_FIXED_FONT_DICTNRS,
 *			UNUSED.
 *		Added in 'update_font_table_and_sgr_flags' software for 
 *		storing additional font metrics in the font_table's new
 *		structure items:
 *			above_baseline,
 *			total_height,
 *			superscript_height,
 *			subscript_height,
 *			average_width,
 *			min_table,
 *			max_table.
 *		Added provisions in 'init_font_table' for initializing the 
 *		the same above new structure items in the font_table; and,
 *		based upon the new initializations in 'init_font_table',
 *		redesigned the following procedures:
 *			init_min_table,
 *			init_max_table,
 *			init_total_height_table,
 *			init_above_baseline_table,
 *			init_superscript_height_table,
 *			init_subscript_height_table.
 *		Added and changed software within 'setup_metric_tables' to 
 *		average the font metrics of a newly paired font with its mate's
 *		the font metrics.
 *		Replaced 'ERROR_MACRO' with 'ABORT_MACRO' and 'WARNING_MACRO'
 *		as appropriate; removed 'ERROR_MACRO' definition.
 *		Re-defined the following literals:
 *			TOTAL_VIRTUAL_MEMORY         100000
 *			VIRTUAL_MEMORY_SAFETY_MARGIN 50000
 *	nv	 8-SEP-1986 12:09:42 All wipe_all functionality when all 
 *		downloaded font file are deleted.
 *		Re-defined the following literals:
 *			TOTAL_VIRTUAL_MEMORY         1048575 - 160000
 *			VIRTUAL_MEMORY_SAFETY_MARGIN 100000
 *		
 *	nv	 16-SEP-1986 15:53:40 Make provision within font_init for the 
 *		distinction between power_up and ordinary ris
 *	nv	 25-SEP-1986 18:04:31 Changed the determination of font
 *		metrics which are common for paired font files, but may be
 *		different between those files (due to shoddy work by font
 *		foundries), so that the MAXIMUM of a pair of values are used
 *		rather than the average. 
 *	nv	 7-OCT-1986 16:57:23 In 'new_ps_font_number', changed abort 
 *		code from 21 to 29.
 *		In 'setup_metric_tables', added 'WARNING_MACRO' for seven
 *		possible incompatabilities of metrics for paired fonts. 
 *	nv	 10-OCT-1986 10:22:15 Moved call to 'compute_font' from
 *		'decdtff' to 'exit_current_mode' in xlc_pars.c.
 *	nv	 15-OCT-1986 16:43:19 Re-enabled, by conditionally excluding
 *		compilation unless FONT_FILE_MODE_IS_MEANINGFUL, 'feature' in
 *		'sgr' and 'compute_font_for_g_set' to allow algorithmic
 *		altering of attributes when fonts are selected by font id. (Cf
 *		9-JUL and 27-AUG.) 
 *		Excluded, by conditional compilation with respect to
 *		FONT_FILE_MODE_IS_MEANINGFUL, most of the code in 'compute_vai'.
 *	nv	 17-OCT-1986 12:00:19 Redesigned the initialization of
 *		sgr_table so as to support the ability of null decatff
 *		commands to reinitialize the sgr_table. 
 *	nv	 21-OCT-1986 12:57:28 Added external reference to
 *		'below_baseline_table' and calculated its value in 
 *		'setup_metrics_tables'.
 *	nv	 21-OCT-1986 17:31:34 Included initialization of 
 *		'below_baseline_table'.
 *	nv	 22-OCT-1986 10:17:39 Changed the sign of 'above_baseline_table'
 *		in the calculation of 'below_baseline_table'.
 *	nv	 22-OCT-1986 13:09:49 Modified 'compute_vai' to use the height
 *		of the font used by GL.
 *	nv	 31-OCT-1986 14:12:56 Changed the character_proportion is 
 *		initialized for built-in fonts so that Courir 10.3cpi and
 *		Elite can be initialized 97% and 83% respectively. 
 *	nv	 3-NOV-1986 16:04:05 Remove the literal 'EXACT_WEIGHT'.
 *		Define the following literals:
 *			REGULAR_WEIGHT_CODE,
 *			SEMI_BOLD_WEIGHT_CODE,
 *			BOLD_WEIGHT_CODE, and 
 *			MAX_WEIGHT_VALUE.
 *		Re-defined EXACT_UL_AND_STRIKETHRU  as 0x1.
 *		Change the weight threshold for considering a font bold from 
 *		'25' to '22' in 'update_font_table_and_sgr_flags'.
 *		In 'compute_font_for_g_set', completely re-designed how fonts
 *		are scored according to their weight and changed the selection
 *		process so that no font is 'killed' (i.e., disqualified) on
 *		account of its weight. 
 *		Improved and added comments.
 *	nv	 4-NOV-1986 19:00:16 fixed bug: in order to change "the
 *		selection process so that no font is 'killed' (i.e.,
 *		disqualified) on account of its weight", '& NO_BOLD' had to be 
 *		changed to '| BOLD'.
 *	nv	 5-NOV-1986 11:00:32 Worked on refining the software which
 *		effects the reconciling of mismatched font metrics for paired
 *		fonts.
 *		Removed 'wipe_all & restore_all' code at  the end of 'declff'.
 *		(This tended to expanded grossly PS code and translator
 *		processing for each downloaded font.)
 *		Created a 'mismatched_paired_font_metrics' flag; initialized 
 *		it to FALSE in 'create_new_paired_font' and set it in 
 *		'setup_metric_tables' if a mismatch was found.
 *		Changed 'create_new_paired_font', if a mis-match occured, to
 *		overwrite the old version of the new downloaded font's mate
 *		(so that revised font metrics are used), appropriately increment
 *		'vm_wasted' and, only if necessary, do a 'wipe_all &
 *		restore_all'.
 *	nv	 24-NOV-1986 10:07:16 Replaced all references to
 *		'TOTAL_VIRTUAL_MEMORY' wiht 'xl_st .vm_size'.
 *		Deleted the definition for TOTAL_VIRTUAL_MEMORY.
 *      kws	 17-MAR-1987 16:40:58 Add support for ISO LATIN1 as
 *		an NRCS.
 *      kws	 18-MAR-1987 22:53:05 Allow ISO LATIN1 to be selectable
 *              as a user preference character set.
 *      kws      18-MAR-1987 22:53:51 Fix optimization problems in scs().
 *      kws      24-MAR-1987 10:58:25 QAR 1033. Select NRCS table 1 for
 *              DEC Supplemental downloaded fonts.
 *
 *	araj	 4-APR-1987 00:14:19 Made vm_wasted, vm_being used 
 *		Globals. Should be moved to XLATE.H, were Globals belong
 *      kws	 19-JUN-1987 13:16:35 Fix font attributes being ignored
 *              when fonts are both BOLD and italicized.
 */



#define cafont_forms	(1)

/*  begin include_file    */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"	    /* ANSI State Table & Token Table Declarations */
#include    "cpglobal.hc"   /* globals defs for the parser */
#include    "caglobal.hc"   /* globals defs for the CARs */
#include    "camac.lib_hc"  /* non-dump oprintf */

#ifdef  DUMP
#include    "dumpu.hc"	    /* Dump Utility                             */
#include    "xlc_iface.hc"  /* defines for oprintf			*/
#endif DUMP

/*  end   include_file    */



/******************************************************************************
    Enter Macro Mode 
*****************************************************************************/

VOID dec_mac_enter()

   {
#ifdef DUMP
   {
    oprintf("MACRO-ENTER\n");
   }
#endif
   }


/******************************************************************************
    Begin New Macro 
*****************************************************************************/

VOID dec_mac_font()

   {
#ifdef DUMP
   {
    oprintf("MACRO-FONT\n");
   }
#endif
   }


/******************************************************************************
    Sixel Data in Macro 
*****************************************************************************/

VOID dec_mac_sxl()

   {
#ifdef DUMP
   {
    oprintf("MACRO-SXL \n");
   }
#endif
   }


/******************************************************************************
    Terminate Macro 
*****************************************************************************/

VOID dec_mac_term()

   {
#ifdef DUMP
   {
    oprintf("MACRO-TERM \n");
   }
#endif
   }


/******************************************************************************
    Macro Automatic Page Overlay 
*****************************************************************************/

VOID dec_spp()

   {
#ifdef DUMP
   {
    oprintf("DECSPP \n");
   }
#endif
   }

