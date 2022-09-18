/*        @(#)xlate.h	4.1      7/2/90      */
/* file: xlate.h -- definitions for ansi_to_ps xlator
 * edit:laf  7-NOV-1985 16:59:18 
 *	gh  13-MAR-1986 14:50:56  Added font & attrib subfields to GLYPH &
 *				   XLATOR_STATE, & JUSTIFY mode defines
 *	laf  21-MAR-1986 14:56:55 Added new defines for FONT, SGR, POINT
 *				   DECIPT, PIXEL, etc.
 *	gh   24-MAR-1986 10:18:29 Created PS_ST
 *	gh   27-MAR-1986 08:39:06 Bumped tabs table sizes from WORD to LONG
 *				  XLATOR_STATE WORD *ssf chgd to CSET *ssf
 *	gh   3-APR-1986 12:14:51  Adding fields to XLATOR_STATE for DECSHORP,
 *				  DECPSP, and HSI
 *	gh	 10-APR-1986 11:26:36 Adding fields to PS_ST for codegen
 *	gh	 16-APR-1986 09:11:13 Adding DECVEC structure, virtual char defs
 *      nv       17-APR-1986 18:35:19 redefined CSET and SGR and cur_sgr, 
 *                                    repositioned *ssf, and modified comments
 *                                    pertaining to attributes
 *	laf	 18-APR-1986 10:28:39 Added DCS processing definitions
 *	gh	 18-APR-1986 14:25:26 Added circv_mode into XLATOR_STATE
 *	laf	 22-APR-1986 11:17:14 Added DUMMY define, sgr table size
 *	nv   23-APR-1986 09:59:27  1) In XLATOR_STATE:
 *					a) reposition psp, hsi and vsi
 *					b) add v_size and character_proportion
 *					a) change g0, g1, g2 and g3 to
 *							g_table [G_TABLE_SIZE]
 *				   2) Defined G_TABLE_SIZE as 4
 *				   3) In GLYPH_FONT_DATA
 *					a) change spacing to horizontal_spacing
 *					b) change horizontal_spacing and
 *					   ps_font_number from WORD to LONG
 *					   algorithmic_attributes
 *					c) change algorithmic_attrs to 
 *				   4) In GSET:
 *					a) change *xlate_table_ptr from
 *					   WORD to BYTE,
 *					b) remove font_index;
 *				   5) In PS_ST change font_dict form FONT
 *				      to GLYPH_FONT_DATA
 *				   6) In the definition of ALL_ATTRS, change
 *				      the ORing of the constituents from 
 *				      Logical ORing to Bitwise ORing.
 *				   7) In SGR change:
 *					a) flag to font_file
 *					b) WORD name [STYLE_SIZE] to
 *					   BYTE id_string [FONT_ID_SIZE + 1]
 *				   8) change the definition of STYLE_SIZE to 
 *				      a definition of FONT_ID_SIZE
 *				   9) Add comments
 *		gh	 24-APR-1986 09:10:46 Changing circv_mode to c1rcv_mode,
 *			 MAXFONTSIZE from 0xffff to 0x10000, and deleted all
 *			 dcs_state defines
 *		gh	 29-APR-1986 10:09:26 Adding xl_st.sheet_len initial-
 *			 ization.
 *		gh	 30-APR-1986 10:50:36 Chgd GSET.BYTE *xlate_table_ptr
 *			 to GSET.WORD transform_index
 *			 Chgd XLATOR_STATE.GSET *gl,*gr,*ssf to
 *			 XLATOR_STATE.WORD gl,gr,ssf;    
 *		gh	 30-APR-1986 17:19:57 Added definition of a blob
 *		laf	 5-MAY-1986 14:58:59 Added defines for FONT_FILE_
 *			 ID_SIZE AND TYPE_FAMILY_ID_SIZE
 *		gh	 8-MAY-1986 09:17:36 Adding NUMBER_OF_PS_DICTIONARIES
 *			 and NUMBER_OF_8_BIT_CHARACTERS defines
 *			 Changing GSET user_pref_cset to WORD user_pref_cset
 *			 in XLATOR_STATE
 *		nv	 8-MAY-1986 16:24:00 Changed FONT_TABLE_SIZE to 48.
 *		gh	 13-JUN-1986 08:03:24 Changing the defines for
 *			 different .plf states
 *		nv	 27-JUN-1986 15:10:05 removed 'attrs' structure item of 
 *			GLYPH and created instead a 'requested_attributes'
 *			structure item of XLATOR_STATE.
 *		nv	 10-JUL-1986 19:07:02 Incorporated literal defining
 *			pitch values and proportional spacing modes.
 *		nv	 12-JUL-1986 08:09:40 Removed 'opm' and 'pcf' from
 *			XLATOR_STATE and the #define RESET for initializing opm.
 *		nv	 14-JUL-1986 12:54:29 Removed 'FONT' and 'CSET' 
 *			structures.
 *		nv	 22-JUL-1986 13:06:11 Changed FONT_TABLE_SIZE from 48
 *			to 35 so as to limit the number of downloaded font 
 *			files to 3 (35 - 32 = 3).
 *		nv	 29-JUL-1986 17:36:39 Add CPI_10_3 and CPI_13_6 literals
 *		nv	 30-JUL-1986 10:14:16 Changing the declaration of .gl &
 *			.gr to .gl_ptr & .gr_ptr respectively.
 *
 *		araj	 4-AUG-1986 15:43:03 
 *			Moved MAXROWS, MAXCOLS, MAXBITMAP used 
 *			by MAKETEST and XLC_DCS to here, rather 
 *			than having two independent definitions.
 *		nv	 18-AUG-1986 14:43:28 Added the following literal 
 *			definitions:
 *				MAX_CSET_SIZE
 *				BLOB_BITMAP
 *				NULL_BITMAP
 *	nv	 27-AUG-1986 11:35:30 Changed the definition of FONT_TABLE_SIZE 
 *		to 36, thereby making the number of downloaded font files equal
 *		to 4.
 *
 *		laf	 27-AUG-1986 14:28:05 Added a "ul_avp" field to
 *			 GLYPH definition.
 *	nv	 28-AUG-1986 18:56:44 Re-defined CPI_10_3 as 696.
 *		Changed the definition of MAXPVAL from 16384 to 32767
 *
 *	gh	 2-SEP-1986 10:49:27 Putting typedef of DLL_FONT_CHAR in here
 *		Also moved define of FIRST_DOWNLOADED_FONT into here
 *	nv	 4-SEP-1986 16:52:37 Included FONT_TABLE_STRUCTURE and 
 *		PS_FONT_TABLE_SIZE.
 *	nv	 5-SEP-1986 11:25:56 Added the following structure items to 
 *		the FONT_TABLE_STRUCT:
 *				above_baseline,
 *				total_height,
 *				superscript_height,
 *				subscript_height,
 *				average_width,
 *				min_table,
 *				max_table.
 *		Moved from 'xlc_font.c' the following 'literal' definitions:
 *			UNUSED,
 *			FIRST_DOUBLE_PS_DOWNLOADED_FONT,
 *			NUMBER_OF_PS_FIXED_FONT_DICTNRS.
 *		Added the following 'literal' definitions:
 *			NUMBER_OF_FIXED_FONTS,
 *			NUMBER_OF_DOWNLOADED_FONTS.
 *		Redefined the several literals pertaining to the size and key
 *		places within the font_table and ps_font_table in terms of
 *		NUMBER_OF_FIXED_FONTS and NUMBER_OF_DOWNLOADED_FONTS.
 *		Set the NUMBER_OF_DOWNLOADED_FONTS as 31.
 *	nv	 15-SEP-1986 11:28:13 Put parenthises around all defined 
 *		expressions.
 *	nv	 15-OCT-1986 17:44:12 Add defintion for 
 *		'FONT_FILE_MODE_IS_MEANINGFUL'.
 *	nv	 17-OCT-1986 12:16:18 Remove declaration of 'hsi'.
 *	nv	 5-NOV-1986 12:43:30 Add CPI_6_8 literal.
 *	nv	 21-NOV-1986 17:32:35 Added 'vm_size' to XLATOR_STATE and 
 *		added two literals: ONE_MEGABYTE and PREAMBLE_SIZE.
 *	mgb	 13-JAN-1987 16:22:49 Added 2 new translator states
 *		eight_bits and output_sixel_ptr.
 *      kws      16-MAR-1987 19:18:32 Changed 'thickness' in DECVEC
 *              structure to LONG.  SCOUT QAR 00199
 *      kws      18-MAR-1987 23:21:19 Add MAXCSETID literal.
 *
 *	araj	 27-MAR-1987 12:54:22 Changed eight_bit from BOOLEAN to 
 *		byte, to support new output encoding schemes, renamed it
 *		output_mode
 *
 *	mgb	 6-JAN-1988 14:36:42 Added CPI_9_34, CPI_7_5, CPI_18_75,
 *		CPI_13_3, CPI_5_7 and CPI_5_4 to meet DEC STD 180-0 for
 *		Bar Code fonts.
 *
 *	mgb	 22-JAN-1988 16:44:41 Deleting all non PostScript definitions.
 *		All non PostScript related definitions have been moved to 
 *		CAPDL.DEF for the Common Parser.
 *
 *	mgb	 19-APR-1988 17:30:23 changed ps_font_xxx to paired_font_xxx
 *
 *	mgb	25-MAY-1988 11:46 added structures DLL_FONT_TABLE_STRUCT
 *		and SPACING_FONT_TABLE_STRUCT for font cache.
 *
 *	tep	26 May 1988 10:30 - added SIXEL_MAP_EL for sixel font support
 *
 *	kws	 1-AUG-1988 15:44:19 add active flag to 
 *		SPACING_FONT_TABLE_STRUCT
 *
 *	kws	 17-OCT-1988 16:38:19 Temporarily include PFS tables from
 *		the common parser.  These values need to be shared because
 *		of the ability to pass paper sizes through the itemlist.
 *
 *	cp	 21-MAR-1989
 *		Modified definition of EOF for Ultrix port.
 *
 */
/*--------------------------------------------------------------------------*/

/************************************************************************
 *									*
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,		*
 *		1986, 1987, 1988, 1989.   ALL RIGHTS RESERVED.		*
 *									*
 *	THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE		*
 *	USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF		*
 *	SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE		*
 *	COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES		*
 *	THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE		*
 *	AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND		*
 *	OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.		*
 *									*
 *	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE		*
 *	WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A		*
 *	COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.			*
 *									*
 *	DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR		*
 *	RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT		*
 *	SUPPLIED BY DIGITAL.						*
 ************************************************************************/

#ifndef EOF
#define EOF (-1)		/* EOF for character input */
#endif

#define PS_STACK_SIZE 128	/* # of entries in PS stack */
#define UNUSED	(-1)	/* If this value appears in a paired_font_table entry,
			it means that there is no valid font stored in the
			corresponding part of the PS machine. */

					/* number of entries in the font table*/



/* NOTE on .plf: plf=0 means no PLU or PLD is in effect.  .plf > 0 indicates
  nesting level of PLU currently in effect.  .plf = 7fff means top margin was
  exceeded by PLU command.  .plf < 0 indicate -(nesting level) of PLD currently
  in effect.  .plf = -7fff means bottom margin exceeded by PLD command. */


#define 	ONE_MEGABYTE	 (1048575)
#define 	PREAMBLE_SIZE	 (80000)



/*
 *Temporarily, put some useful Macros here, need to extract them from CAMAC.LIB
 *and make them common
 */


/*  Non device-specific stuff */

#define MIN(x,y) ( ( (x) < (y) ) ? (x) : (y) )

#define MAX(x,y) ( ( (x) > (y) ) ? (x) : (y) )

#define CVT_ZERO_TO_ONE(x) ( ( (x) != 0 ) ? (x) : (1) )

#define ABS(x) ( ( (x) > (0) ) ? (x) : ( -(x) ) )

