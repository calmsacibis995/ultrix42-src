#ifndef lint
static char SccsId[] = "  @(#)cafont_load.c	4.1   LPS_ULT_TRN   7/2/90";
#endif
/* file: cafont_load.c
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
 *   001   9-MAR-1988 14:24  mhw
 *	Initial Version, created by splitting cafont.c
 *
 *   002  13-MAR-1988 19:06 mhs
 *      Finish merging routines from CADCS and finish arithmetic
 *      typecasting.
 *
 *  20-APR-1988 10:28 mhw
 *	Added real code for the 2 reserve memory routines
 *
 *  19-MAY-1988 16:52 mhw
 *	Cleaned sixel_cvt_and_store routine such that "ch" variable was
 *	eliminated.  A shift does not cause the value in "sixel" to change, 
 *	so the original value did not need to be stored in "ch".
 *	
 *	Fixed bug in calls to sixel_cvt_and_store.  The position to store the
 *	sixel was not incremented by cmd_str_len, so always stored it at the
 *	beginning of the memory reserved.  This was lost when moved from the
 *	Translator.
 *
 *	In dec_lff_load, removed decrement of dcs_store_ptr by cmd_str_len.
 *	dcs_store_ptr is never increment, it always points to the begining.
 *
 *   3-JUN-1988 15:30  mhw
 *	Change call to font_dispose_font to not use box_number
 *	Change call to font_get_memory to pass PL not LONG
 *	Added calls to font_dispose_of_font if font goes beyond memory
 *	    available, if syntax error, or if bad result from analyze
 *
 *   7-JUL-1988 15:49  mhw
 *	Make changes for conversion of cp_pbuf from WORD to LONG
 *   end edit_history
 *
 *  30-NOV-1988 14:28 araj
 *	Install cfont caching, change calls to compute font into calls to invalidate_font
 *	Make sure references to vai or g_table have a test for the validity of the data
 *	first
 *
 *
 *-----------------------------------------------------------
 */


/*
 *-----------------------------------------------------------
 *
 *   begin description
 *
 *   Filename:  cafont_load.c
 *
 *
 *   This module includes
 *
 * dec_rfs()
 * dec_rfnm()
 * dec_rbmm()
 * dec_uffm()
 * save_dcs_introducer()
 * init_cmd_ptr()
 * dec_lff_enter()
 * dec_lff_term()
 * dec_lff_font()
 * dec_lff_load()
 * dec_lff_sxl()
 * dec_lff_sxl_rpt()
 * dec_dld()
 * sixel_cvt_and_store()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */


/*
 *-----------------------------------------------------------
 *
 *   begin edit_history of cafont.c
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



/* Translator edit history for secondary (store sixel) routines
 *
 * file:	xlc_dcs.c - DCS processing routines for translator
 * created:	laf  17-APR-1986 15:05:39 
 * edit:	laf  17-APR-1986 16:04:04  Added static variables
 *					   for DCS processing
 *		laf	 22-APR-1986 11:19:20 Changing enter_dcs_store(),
 *			 complete_store_char(), complete_store_sixel()
 *		laf	 24-APR-1986 10:03:36 Added check for too big 
 *			 repeat count to dcs_store_repeat; added call to
 *			 compute_font to complete_store_sixel (and 
 *			 COMMENTED IT OUT, all per Nick's request); 
 *			 changed complete_store_char to pass FALSE/TRUE
 *			 depending on value of ps1.
 *		gh	 30-APR-1986 10:25:33 Changing some font routine calls
 *		laf	 5-MAY-1986 14:32:24 Add error checking of params
 *			 to decatff & decdtff; add 0 at end of stored DCS
 *			 string for a decatff so that don't have to pass 
 *			 string len to decatff as a parameter (at Nick's 
 *			 request); also defined FIRST_SGR_TABLE_ENTRY and
 *			 ERROR_CODE
 *		gh	 8-MAY-1986 12:18:56 Deleting delete_font_files,
 *			 declff,decatff,decdtff stubs.
 *		laf	 13-MAY-1986 16:50:44 Was passing dcs_intro.ps2
 *			 to decatff instead of the "adjusted" SGR table
 *			 index "temp", so I fixed it.
 *		laf	 28-MAY-1986 21:33:36  Changed dumpdcs () so it
 *			 writes output to a file called "dump1.dcs"
 *			 instead of to the screen.
 *		nv	 21-JUL-1986 16:06:01 Changed the defined value of
 *			DECLFF_DEL_ALL_DWNLD_FONTS from '0' to '3'.
 *
 *		araj	 2-AUG-1986 02:59:57 
 *			modified DUMPDCS, as the pointer had a tendancy to
 *			go overboard, if an odd number of bytes were
 *			received.
 *
 *		araj	 2-AUG-1986 03:12:07 
 *			CMD_STR_LEN must be 64K, not 32K
 *			so should k for instance
 *		nv	 16-OCT-1986 16:40:46 Moved "pstate = S_STORECHAR;"
 *			statement from the end of 'case ~' and 'case }' in
 *			'pr_dcsseq' to the end of 'case ~' and 'case }' in
 *			'enter_dcs_store'. 
 *			Moved "pstate = S_STORESIXEL;" statement from the end
 *			of 'case y' in 'pr_dcsseq' to the end of 'case y' in
 *			'enter_dcs_store'. 
 *		nv	 17-OCT-1986 11:40:28 In 'complete_store_char', made
 *			provisions for a null decatff commands to reinitialize
 *			a designated sgr table entry. 
 *              kws      18-MAR-1987 22:49:43 Add DCS sequence to select 
 *                      user preference character set.
 */


/*  begin include_file    */

#define cafont_load	(1)

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"	    /* ANSI State & Token Table Declarations	*/
#include    "caglobal.hc"   /* globals defs for the CARs		*/
#include    "camac.lib_hc"  /* non-dump oprintf                         */

#ifdef  DUMP

#include    "dumpu.hc"	    /* Dump Utility                             */
#include    "xlc_iface.hc"  /* defines for oprintf			*/

#endif

/*  end   include_file    */




/***************************************************************************
    Request Font Status 
*************************************************************************/
VOID dec_rfs()

{
#ifdef DUMP
    {
    oprintf("DECRFS \n");
    pprint();
    }
#endif
}



/***************************************************************************
    Reserve Memory for Font 
*************************************************************************/

VOID dec_rfnm()

{
    UBYTE mem_type;
    UWORD mem_value;

#ifdef DUMP
    {
    oprintf("DECrfnm \n");
    pprint();
    }
#endif
    mem_type = MEM_FONT;	/* set type of memory to font */
    mem_value = (UWORD)cp_pbuf[0];/* get parameter from buf for size of mem */


    /* call the composer/ code_generator to reserve memory */

    process_reserve_mem(mem_type, mem_value);
}


/***************************************************************************
    Reserve Memory for Bitmap 
*************************************************************************/

VOID dec_rbmm()

{
    UBYTE mem_type;
    UWORD mem_value;

#ifdef DUMP
    {
    oprintf("DECRBMM \n");
    pprint();
    }
#endif
    mem_type = MEM_BITMAP;	/* set type of memory to bitmap */
    mem_value = (UWORD)cp_pbuf[0];/* get parameter from buf for size of mem */


    /* call the composer/ code_generator to reserve memory */

    process_reserve_mem(mem_type, mem_value);
}



/******************************************************************************
    Upload Font File Metrics
*****************************************************************************/

VOID dec_uffm()

{
#ifdef DUMP
    {
    oprintf("DECUFFM \n");
    pprint();
    }
#endif
}



/******************************************************************************
    Save the DCS Introducer
*****************************************************************************/

VOID save_dcs_introducer ()
{
    dcs_intro.ps1 = (WORD)cp_pbuf [0];
    dcs_intro.ps2 = (WORD)cp_pbuf [1];
    dcs_intro.ps3 = (WORD)cp_pbuf [2];
}



/******************************************************************************
 * initialise the command string pointer; called by LFF 
*****************************************************************************/

VOID init_cmd_ptr()
 {

    amt_allocated = MAXFONTSIZE;

    /* Set True until a problem found, then set to false
     * Is used to tell font_dispose_of_font to add to dictionary or to
     * release the memory because there is a problem
    */

    valid_load = TRUE;
    
    /* init the ptr to this memory, amt_allocated will contain amount of
	memory actually available
    */
    dcs_store_ptr = FNT_GET_MEM(&amt_allocated);

    /* If no memory allocated, characters will merely be ignored as
     * they come in
     */

    if (dcs_store_ptr == 0)
	{
	    ABORT_MACRO(93); 
	    ac_dcs_ignore();	/* Set Table Ptr to DCS_IGNORE Table */

	}
 }


/*
 * convert character to a binary byte, for use in font names 
 */

VOID sixel_cvt_and_store (dest)
UBYTE *dest;
{
    UBYTE  sixel;

    sixel = cp_c;

    /* convert ch to a binary byte */
    sixel -= 0x3f;

    switch (modulo)
       {
	case 0:	dest [0] |= sixel << 2;
		break;
	case 1: dest [0] |= ((sixel >> 4) & 0x03);
		dest [1] |= ((sixel << 4) & 0xf0);
		break;
	case 2: dest [1] |= ((sixel >> 2) & 0x0f);
		dest [2] |= ((sixel << 6) & 0xc0);
		break;
	case 3: dest [2] |= sixel;
		break;
       } /* end switch */

    modulo++;
    modulo &= 0x03;
  
    if (modulo == 0) 
       {
	cmd_str_len += 3L;
       } 
}


/******************************************************************************
    dec_lff_load - load the font just received
*****************************************************************************/

VOID dec_lff_load()
{

    /* If there were any partial sixel quads received, bump up the byte
     * count.
     */
    cmd_str_len += (LONG)modulo;


    /* Check that the new font file has something wrong with it
     * Analyze returns a zero if the font is ok
     */

    if ( ANALYZE_FONT_FILE ( dcs_store_ptr, cmd_str_len) )
       {
        ABORT_MACRO(11);  /* analysis of font file turned up problem(s) */
	valid_load = FALSE; /* Release the memory if analyze problems */
       }
    
	FNT_DISPOSE_FONT(dcs_store_ptr, cmd_str_len, valid_load, &dll_font);
}


/******************************************************************************
    dec_lff_enter identified, about to begin loading of CFFF 
*****************************************************************************/

VOID dec_lff_enter()

{
#ifdef DUMP
    {
    oprintf("DECLFF_ENTER \n");
    }
#endif

    /* scale the summary sheet parameter */
    if (dcs_intro.ps2 != 1) 
       {
	dcs_intro.ps2 = 0;
       }

    /* check if need to delete all fonts */
    if (dcs_intro.ps3 == 0)
       {
        FNT_DELETE_FONT_FILES (DECLFF_DELETE_ALL_DNLODED_FONTS, NULLPTR);
       }

    /* allocate memory for command string and init ptr to it  */
    init_cmd_ptr();
}




/******************************************************************************
    dec_lff_term end of font file transfer 
*****************************************************************************/

VOID dec_lff_term()

{
#ifdef DUMP
    {
    oprintf("DECLFF_TERM \n");
    }
#endif

    dec_lff_load();

    /* font repertory has changed, so determine which font to use */
    invalidate_font();

    if (dcs_intro.ps2 == 0)
       {
        FNT_PRNT_STATUS_SHEET;
       }

}  /* end of dec_lff_term */



/******************************************************************************
    dec_lff_font - comma detected, begin next font file 
*****************************************************************************/

VOID dec_lff_font()

{
#ifdef DUMP
    {
    oprintf("DECLFF_FONT \n");
    }
#endif
    /* load the font file just specified */
    dec_lff_load();

    /* allocate memory for command string and init ptr to it  */
    init_cmd_ptr();
}



/******************************************************************************
    dec_lff_sxl identified- sixel data in font record 
*****************************************************************************/

VOID dec_lff_sxl()

{
#ifdef DUMP
    {
    oprintf("DECLFF_SXL \n");
    }
#endif

   /* If no param supplied, no repeat so only perform the loop
    * once.  Otherwise, convert the current sixel byte to binary 
    * and store "cp_pbuf[0]" copies of it in the designated 
    * memory location .
    */

    if (cmd_str_len < amt_allocated)
	 sixel_cvt_and_store (dcs_store_ptr + cmd_str_len);
    else
	  /* Not enough memory was allocated, so set flag to release memory */
	 valid_load= FALSE;
}



/******************************************************************************
    dec_lff_sxl_rpt identified - sixel Repeat count 
*****************************************************************************/

VOID dec_lff_sxl_rpt()

{
#ifdef DUMP
    {
    oprintf("DECLFF_SXL_RPT \n");
    }
#endif
   /* If no param supplied, no repeat so only perform the loop
    * once.  Otherwise, convert the current sixel byte to binary 
    * and store "cp_pbuf[0]" copies of it in the designated 
    * memory location .
    */

    if (cp_pbuf[0] == 0) 
       {
	cp_pbuf[0] = 1;
       }

    for (; ((cp_pbuf[0] >= 1) && (cmd_str_len < amt_allocated)) ; cp_pbuf[0]--) 
       {
	sixel_cvt_and_store (dcs_store_ptr + cmd_str_len);
       }
    if (cmd_str_len < amt_allocated)
	valid_load = FALSE; /* not enough memory */
}



/******************************************************************************
    Downline Load of Font files (LA75) 
*****************************************************************************/

VOID dec_dld()

{
#ifdef DUMP
    {
    oprintf("DECDLD \n");
    }
#endif
}

