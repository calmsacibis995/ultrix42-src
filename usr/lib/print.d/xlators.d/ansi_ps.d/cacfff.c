#ifndef lint
static char SccsId[] = "  @(#)cacfff.c	4.1   LPS_ULT_TRN   7/2/90";
#endif
/* file: cacfff.c
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
 *	10-MAR-1988 14:24 mhw
 *      Initial Version 
 *
 *	24-MAY-1988 10:26  mhw
 *	Add references to dll_font structure defined in capdl.def
 *
 *	31-MAY-1988 14:15  mhw
 *	Add type_size_whole and type_size_fraction for the LN04
 *	Changed substitute_flag to MAX_CSET_SIZE if substitute error char,
 *	    MAX_CSET_SIZE+1 if not substitution,
 *	    and 0-(MAX_CSET_SIZE-1) if character substitution
 *
 *	22-AUG-1988 10:13 mhw
 *	Add returns after abort macros such that analyze will not continue
 *	after an error.  This is the way the Translator use to work.
 *
 *	12-SEP-1988 09:35  mhw
 *	Changed BYTE to UBYTE for ABORT_MACRO test 62 and 67
 *
 *	15-SEP-1988 16:04 mhw
 *	Added character replacement, then commented out until translator
 *	side is ready for this.  It will then need to be re-activated.
 *	This addition adds Warnings (87) and (88) and Abort (89)
 *
 *	Changed Abort (68) to Warning (68) as per CFFF meeting held on
 *	1/12/88 and described in the Parser Notes File, note 19.1
 *
 *	Removed Abort (65) since with character replacement, Bit 31 of
 *	the character locator can be set - it is no longer an error.
 *
 *	14-OCT-1988 14:48 mwh
 *	Added support for overline to dll_font.
 *
 *	 3-NOV-1988 09:44 mhw
 *	Added angle brackets to stdio for ultrix support
 *
 *	 1-DEC-1988 10:46 mhw
 *	If no space character exists in a 94 character set, the substitute
 *	flag must be set to MAX_CSET_SIZE + 1, because we create a space for
 *	it.  It was set to MAX_CSET_SIZE and so the translator substituted
 *	a blob.
 *
 *	14-APR-1989 21:43   araj
 *	change inclde to .lib_hc, not .lib
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
 *   Filename:  cacfff.c
 *
 *
 *   This module includes
 *
 *	stars_and_dots()
 *	substitute_blob_for_char()
 *	analyze_font_file()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */


/* file:  xlc_analyze.c -- analyze the DCS buffer, which contains a CFFF font.
 * edit:  laf  6-JUN-1986 16:12:04 Added conditional compile of fprintf's.
 *	  nv   27-JUN-1986 18:15:23 Fixed dll_char_set_id so that font files 
 *	       which claim to have a USER_PREFERENCE character set are
 *	       re-identified as DEC_SUPPLEMENTAL.
 *	  laf  27-JUN-1986 20:29:53 Changed length of all tables and arrays
 *	       from MAX_CSET_SIZE to MAX_CSET_SIZE+1 so that we can always
 *	       stuff the error char in the last slot of each table so that 
 * 	       Nick's font code can access it easily and it can be shipped 
 *	       to the PS machine just like any other character.
 *	  gh   16-JUL-1986 09:06:04 Right bearing is calculated using 'numcols'
 *		before numcols is set - moving setting of numcols before the
 *		right bearing is calculated.
 *	  nv   18-JUL-1986 18:07:24 Add provisions to force space characters 
 *		under certain circumstances.
 *	  nv   22-JUL-1986 12:54:07 Disabled enunciation of font file errors.
 *
 *	araj	 1-AUG-1986 22:43:08 
 *		Looking for initialization problems.
 *		Included Copyright notice, that had been forgotten.
 *		Added use of conditional compile, to make use
 *		of ERROR macro easier.
 *
 *	araj	 4-AUG-1986 15:47:31 
 *		added test in dump routine to ensure the 
 *		character does not overflow the dump area
 *
 *		Moved MAXCOLS, MAXROWS, MAXBITMAP to XLATE.H
 *
 *	araj	 4-AUG-1986 18:56:10 
 *		Added two additional chack variables, 
 *		number_of_bytes_in_bitamp, and 
 *		number_of_odd_words_in_bitmap to verify that 
 *		some of the files that were rejected were
 *		rejected because the creator was counting
 *		bytes, not rouded up words.
 *
 *		Did not do anything with this yet, 
 *		just printing it in the analyze log file.
 *		we can decide later whether to accept or
 *		reject such files.
 *	nv	 12-AUG-1986 16:05:30 
 *		Put conditional compilation stuff around VM monitoring print 
 *		statement.
 *		Added 'include "dbug.h"' at the beginning of this module.
 *		Part of error 68 was reunited with the rest of it and shuffled 
 *		around; sometime earlier additional code had been inserted
 *		among the code for managing 'error 68' and this created a
 *		nonsensical stretch of code.
 *
 *	laf	Added conditional compile of the code that reads the
 *		following fields out of the CFFF, because nobody uses them
 *		in the current translator:
 *			dll_num_char_defs
 *			dll_monospacing_flag
 *			dll_font_horiz_prop
 *			dll_horiz_prop_numerator
 *			dll_horiz_prop_denominator
 *			dll_below_baseline_dist
 *
 *	gh	 18-AUG-1986 14:34:54 Laurie left nested #ifdefs that did
 *		 not start in column 1 - fixed it.  This goes on her record.
 *	nv	 20-AUG-1986 10:17:02 Standardized the left margins so that 
 *		this module agrees with xlc_cont.c.
 *		Moved conditional compilation controls to dbug.h.
 *		Changed conditional compilation controls from '#ifdef' to 
 *		'#if'.
 *		Add printout of font file id along with error 68. 
 *		Moved definition of MAX_CSET_SIZE to xlate.h 
 *		Removed ERROR_CODE definition -- obsolete. 
 *		Made changes so that for glyphs with no bit-map (i.e., space) 
 *		dll_char [i] .bitmap_ptr = NULL_BITMAP; this necessary so the 
 *		software which removes redundant error characters to work. 
 *	nv	 21-AUG-1986 10:25:41 removed .num_wds_in_bitmap from the 
 *		DLL_FONT_CHAR structure and associated software. This happened
 *		to involve fixing as well as changing the determination of
 *		width for the forced space capability. 
 *		Add printout of font file id along with error 56.
 *		Streamlined calculation of 'number_of_odd_words_in_bitmaps'.
 *		Fixed 'dump2' print statement of number of characters 
 *		replaced by blob references.
 *		Fixed an index in the 
 *	nv	 22-AUG-1986 10:08:18 Added comments.
 *	nv	 27-AUG-1986 15:33:11 Put the checking for complete character 
 *		set (the 'ZZZZ' problem) under the conditional compilation of
 *		COMPLETE_ANALYSIS.
 *		Added check that dll_last_character >= dll_first_character.
 *		In an attempt to ensure that 'divide by zero' do not occur,
 *		added checks that the following parameters are not zero:
 *			dll_total_vertical_size,
 *			dll_horiz_prop_numerator,
 *			dll_horiz_prop_denominator;
 *		Changed 'dll_average_char_width' to 'dll_space_width' in the 
 *		software for forcing space characters under certain
 *		circumstances. 
 *		Changed the definition of dll_spacing from UBYTE to UWORD.
 *	nv	 28-AUG-1986 18:34:06 Included checks for consistent font 
 *		widths and  character widths for monospaced fonts.
 *	gh	 1-SEP-1986 10:17:14 Changed the 32-bit arithmetic flag test &
 *		the test for the four 'Z's in the char set ID to only output
 *		a message in COMPLETE_ANALYSIS mode, since it has been
 *		determined that these really aren't errors.  Also, the setting
 *		of dll_type_size has been changed to fetch UWORDS from WORDS,
 *		and a test is done before the fetch to ensure there won't be
 *		overflow from the caculation.  If there will be, an ERROR(101)
 *		is generated.
 *	nv	 3-SEP-1986 11:13:26 Moved 'number_of_odd_words_in_bitmaps' and
 *		'total_portrait_bytes' from local automatics within 'analyze'
 *		to globals declared at the beginning of this module.
 *	nv	 5-SEP-1986 19:22:57 Defined the following literals and used 
 *		the first two to replace constants where we generate our home
 *		made 'blob':
 *			NUMBER_OF_BLOB_ROWS,
 *			NUMBER_OF_BLOB_COLUMNS,
 *			BLOB_BITMAP_SIZE.
 *		Used BLOB_BITMAP_SIZE augment the total_portrait_bytes count
 *		when the error character is not defined within a downloaded
 *		font file and we have to use our own homemade 'blob'.
 *	nv	 8-SEP-1986 11:40:12 Fixed main for-loop, which analyzes the 
 *		individuals characters, to disregard direct, as well as
 *		indirect, references to 'blobs. 
 *	nv	 7-OCT-1986 13:04:58 Checked that neither 'dll_type_size' nor
 *		'dll_total_vertical_size' exceed 3 inches.
 *		Define MAX_FONT_SIZE to be 216 (point).
 *		Include a check that the dll_num_char_defs equals
 *		(end_idx - start_idx + 1).
 *		Removed definition of ERROR_MACRO.
 *		Replaced each use of ERROR_MACRO with 'ABORT_MACRO' except for
 *		old errors 52 and 56 which were replaced with 'WARNING_MACRO'.
 *		Removed the comment delimiters which used to be around errors
 *		52 and 56.
 *		Also, nearby errors 52 and 56, I found other code which put
 *		information into the dump2.dcs file but which needed to be
 *		couched in conditional compilation to prevent possible
 *		attempted output when the dump file was not opened; I made
 *		that code compiled conditionally according to 'WITHOUTPUT.'
 *		Removed superfluous 'return' statements which followed 
 *		'ABORT_MACRO'.
 *		Changed code compiled conditionally with respect to 
 *		ERROR_REPORT to compiled never.
 *	nv	 22-OCT-1986 10:45:08 Over-write 'dll_average_char_width' with
 *		'dll_space_width'.
 *	nv	 31-OCT-1986 14:19:55 Considering the change, converted
 *		ABORT_MACRO(71) to WARNING_MACRO(71) and placed it under
 *		'COMPLETE_ANALYSIS'.
 *	nv	 5-NOV-1986 12:48:03 Add provisions for a spacing code of '3'
 *		(6.8 cpi).
 *
 *	araj	 15-APR-1987 19:34:56 Add printing of more data for 
 *		ABORT 68 and 73
 *	araj	 1-JUL-1987 16:10:55 Add print statement for blob size 
 *		to help debug fonts
 *
 *	mgb	 21-JUL-1987 12:58:19 When calculating the number of bytes 
 *		in portrait, they (the creator of the fonts) forget to 
 *		include the blob and/or rounding. Made change to be 
 *		more accepting of the fonts and print them even if they are 
 * 		wrong. Now we will accept fonts even if they forget to do
 *		rounding and/or the blob.
 *
 * 	mgb	 9-NOV-1987 15:13:46 
 * 		For the sake of the LN04, compute the landscape bytecount, as 
 *		well as the mixed byte count, and run the font tests again (no 
 *		need to print anything, just the log with the warning/aborts is 
 *		sufficient.
 *
 *		To compute landscape byte count, I modified the code that 
 *		computes portrait byte count, but invert the role of x and y 
 *		(number_rows/number_columns).
 *
 *		To compute mixed byte count, take the max of portrait and 
 *		landscape on each char,
 *
 *		Whereve the code did the following:
 *
 *		     portrait_byte_count (character_i) = f (num_rows, numcols)
 *		     total_portrait_bytes += portrait_byte_count (character_i) 
 *
 *
 *		Was changed to do:
 *
 *		     portrait_byte_count (character_i)  = f (num_rows, num_cols)
 *		     landscape_byte_count (character_i) = f (num_cols, num_rows)
 *		     total_portrait_bytes  += portrait_byte_count (character_i) 
 *		     total_landscape_bytes += landscape_byte_count (character_i)
 *		     total_mixed_bytes     += MAX (total_portrait_bytes, 
 *		                                   total_landscape_bytes)
 *
 *		Then at the end, of analyze, a test was added to check the 
 *		computed values against the stored values, that we currently 
 *		do for portrait.
 *
 *	mgb	 16-NOV-1987 15:06:26 
 *		added the following code for mixed.
 *
 *		dll_even_blob_bitmap_size_mixed = MAX (P_even, L_even);
 *		dll_odd_blob_bitmap_size_mixed  = MAX (P_odd, L_odd);
 *		if (dll_even_blob_bitmap_size_mixed 
 *		 != dll_odd_blob_bitmap_size_mixed) 
 *		  number_of_odd_words_in_bm_mixed += 1;
 *
 *	kws	 14-JAN-1988 19:26:13 
 *		Fixed bug in calculating mixed byte count.
 *            
 *	kws	 14-JAN-1988 19:35:08 
 *              Move where start_idx and end_idx are filled in.
 */

/*  begin include_file    */

#include    "portab.h"	    /* portablity */
#include    "cpsys.hc"	    /* to get global def such as AL */
#include    "camac.lib_hc"    /* Get general Macros such as MAX and MIN */
#include    "caglobal.hc"   /* defs for this file */

#define cacfff	(1)


/* Debug Tools */

#ifdef WITHOUTPUT
#include <stdio.h>
GLOBAL UBYTE dump_number[] = {"dump2@.dcs"};
#endif

#ifdef DEBUG
extern dumpdcs ();  /* This is declared in dum_codegen, only if DEBUG defined */
#endif


 



/*----------*/
VOID stars_and_dots (numrows, numcols, bitmap_ptr, fout)
WORD numrows, numcols;
PUB   bitmap_ptr;
PUB  fout;	 
{
#ifdef WITHOUTPUT
    WORD i, j, k;
    UBYTE curbyte, outbyte;
    UBYTE chardump [MAXROWS][MAXCOLS];
	
	if (numrows > MAXROWS)
	{
	fprintf (fout, "\nMore rows than allowed for dump routine MAX: %d, Actual: % d\n",
		MAXROWS, numrows);
	return;
	}	
	
	if (numcols > MAXCOLS)
	{
	fprintf (fout, "\nMore columns than allowed for dump routine MAX: %d, Actual: %d\n",
		MAXCOLS, numcols);
	return;
	}	

    /* Init the array */
    for (i=0; i<numrows; i++)
	for (j=0; j<numcols; j++)
	    chardump [i][j] = ' ';

    for (i=0; i < numrows; i++) {

	for (j=0; j < (numcols + 7)/8; j++) {

	    curbyte = *bitmap_ptr++;

	    for (k=0; k < 8; k++) {

	    	outbyte = curbyte & 0x01;

	        if (outbyte) {
		    chardump [i][(j*8)+k] = '*';
	        } else {
		    chardump [i][(j*8)+k] = '.';
	        }
	        curbyte = curbyte >> 1;
	   }
	}
    }

    /* Dump out the array now to the file specified */

    fprintf (fout, "\f");		/* start bitmap on new page */

    for (i=0; i<numrows; i++) {
	fprintf (fout, "\n");
	for (j=0; j<numcols; j++)
	    fprintf (fout, "%c", chardump [i][j]);
    }
#endif

}


/*----------*/
VOID substitute_blob_for_char (thischar, idx)
DLL_FONT_CHAR	*thischar;
LONG		idx;
{

thischar->bitmap_ptr = BLOB_BITMAP;

/* The following is a signal for certain code below that this glyph is not
 * represented by a NULL_BITMAP.
 */ 
thischar->numrows = 2;
thischar->numcols = 2;

/* substituted blob for char., flag for device */
thischar->substitute_flag = MAX_CSET_SIZE;	    

dll_font.dll_l_bearing_tbl [idx] = dll_font.blob_l_bearing;
dll_font.dll_r_bearing_tbl [idx] = dll_font.blob_r_bearing;
dll_font.dll_width_tbl [idx] = dll_font.blob_width;	
}


/*----------*/
/* analyze_font_file (bufptr, buflen) -- This routine interprets the font
 * 					 data stored in the DCS storage area
 * 					 and supplies values for all of the
 * 					 following variables:
 *	new_font_file_id
 *  	dll_char_set_id
 *	dll_spacing
 *	dll_first_char
 *	dll_last_char
 *	dll_ul_font_flag
 *	dll_ul_offset
 *	dll_ul_thickness
 * 	dll_strikethru_flag
 *	dll_strikethru_offset
 *	dll_strikethru_thickness
 *	dll_ol_offset
 *	dll_ol_thickness
 *	dll_shadow_flag
 *	dll_shadow_vertical
 *	dll_shadow_horizontal
 *	dll_italic_flag
 *	dll_font_weight
 *  	dll_monospacing_flag
 *      dll_type_size 		
 *	dll_type_size_whole
 *	dll_type_size_fraction
 *	dll_total_vertical_size
 *	dll_average_char_width
 *	dll_font_horiz_prop 
 *	dll_font_horiz_prop_numerator
 *	dll_font_horiz_prop_demoninator
 *	dll_superscript_vert
 *	dll_subscript_vert
 *	dll_space_max
 *      dll_space_min
 *	dll_space_width
 *      dll_above_baseline_dist
 *	dll_below_baseline_dist
 *      dll_scale_factor_code
 *	dll_num_char_defs
 *	dll_num_char_defs
 *	dll_char []
 *	dll_width_tbl []
 *	dll_l_bearing_tbl []
 *	dll_r_bearing_tbl []
 *	dll_even_blob_bitmap_size
 *	dll_odd_blob_bitmap_size
 *	dll_horiz_prop_denominator
 *
 * 
 * Analyze_font_file () returns a value of 0 if the font file is "good"
 * and a non-zero error code if the file is rejected due to an error 
 * condition.
 */ 


/*----------*/
BYTE analyze_font_file ( bufptr, buflen )
UBYTE	*bufptr;
LONG	buflen;
{

#ifdef WITHOUTPUT
  FILE  *fptr;
#endif

LONG number_of_bytes_in_bitmap_port, number_of_bytes_in_bitmap_land, 
      number_of_bytes_in_bitmap_mixed;
BYTE cset_id [32];
UBYTE temp_spacing, temp_spacing_fraction;   /* chars so UBYTE */
LONG temp_long;
LONG temp, org_flags, char_dir_idx, current_locator;
LONG cset_idlen, cset_idptr, temploc;
LONG font_attr_flags, i, start_idx, end_idx, fontsegaddr;
WORD no_dll_error_char_defined, size_of_char_params, j;
LONG  default_blob_l_bearing;
LONG  default_blob_r_bearing;
LONG  default_blob_width;
LONG  blob_bitmap_size, dll_even_blob_bitmap_size, dll_odd_blob_bitmap_size;
LONG  dll_even_blob_bitmap_size_land, dll_odd_blob_bitmap_size_land;
LONG  dll_even_blob_bitmap_size_mixed, dll_odd_blob_bitmap_size_mixed;

DLL_FONT_CHAR default_blob, blob;

/* LONG blob_bitmap_size_land, blob_bitmap_size_mixed; */



#ifdef WITHOUTPUT
  printf("\nEntering Analyze\n");
  dumpdcs ();
  printf("DCS string dumped to file DUMP1.DCS");
  dump_number [5]+=1;
  fptr = fopen (dump_number, "w");
        
  fprintf (fptr, "\n bufptr= %xH,  buflen= %xH (%d)", bufptr,buflen,buflen);

  printf("\nAnalyze Log File %s opened\n",dump_number );
#endif 

dll_font.total_portrait_bytes = dll_font.total_landscape_bytes = 0;
dll_font.total_mixed_bytes = 0;
no_dll_error_char_defined = FALSE;
dll_font.number_of_odd_words_in_bitmaps = 0;
dll_font.number_of_odd_words_in_bm_land = 0;
dll_font.number_of_odd_words_in_bm_mixed = 0;


/* Init the tables to zero, as blank chars (No raster) are skipped, as are
 * characters that character replacement is done on    
 */


for (i=0; i< MAX_CSET_SIZE; i++)
  {
  dll_font.dll_char[i].baseline=0;
  dll_font.dll_char[i].no_underlining=0;
  dll_font.dll_char[i].numrows=0;
  dll_font.dll_char[i].numcols=0 ;    
  dll_font.dll_char[i].bitmap_ptr=0;
  dll_font.dll_char[i].substitute_flag= MAX_CSET_SIZE + 1;
  dll_font.dll_width_tbl[i] = 0;
  dll_font.dll_l_bearing_tbl[i] = 0;
  dll_font.dll_r_bearing_tbl[i] = 0;
  
  }

/* Initialize "default blob" (default error character) */
for (i=0; i < MAX_BITMAP_SIZE; i++)
  dll_font.default_blob_bitmap [i] = 0xff;
default_blob.baseline = 0;
default_blob.no_underlining = TRUE;
default_blob.numrows = NUMBER_OF_BLOB_ROWS;
default_blob.numcols = NUMBER_OF_BLOB_COLUMNS;
default_blob.bitmap_ptr = dll_font.default_blob_bitmap;
default_blob_l_bearing = 0;
default_blob_r_bearing = 0;
default_blob_width = 0;



/* Do as much ERROR CHECKING as possible on the font file first.
 *
 * Since the program which "makes" fonts pads font files
 * with nulls to the nearest block (512 bytes), our only
 * check on font file length can be that the length stored
 * in the first longword of the font file must be <= the
 * number of bytes in the DCS buffer (buflen).
 */

temp_long = GET_LONG(bufptr + OFFSET_TOTAL_FONT_FILE_LENGTH);
if (  temp_long > buflen)
  {

#ifdef WITHOUTPUT
  fprintf (fptr, "(         file len stored in font file is %xH.)\n", temp_long);
#endif
  ABORT_MACRO(41); /* Stored font file length does not match buffer len */
  return (TRUE);
  }



/* If the length of the incoming font file is less than the
 * offset of the character directory region, then it is too short 
 * to even contain a full header.
 */

if (buflen < OFFSET_CHAR_DIR_REGION)
  {
  ABORT_MACRO(42); /* font file too short to contain full header */
  return (TRUE);
  }

temp_long = GET_LONG(bufptr + OFFSET_FONT_FILE_ID_DESCRIP+4);
if ( ( *((WORD *)(bufptr + OFFSET_FONT_FILE_ID_DESCRIP))!= FONT_FILE_ID_SIZE )
     ||
     ( temp_long != OFFSET_FONT_FILE_ID_STRING )
   )
  {
  ABORT_MACRO(43); /* Invalid font_file_id ptr in CFF */
  return (TRUE);
  }


/* Pick out the part that gives us the escape sequence required
 * to select the proper cset.
 */

cset_idlen = *((WORD *)(bufptr + OFFSET_CHAR_SET_DES_DESCRIP));
cset_idptr = GET_LONG(bufptr + OFFSET_CHAR_SET_DES_DESCRIP+4);
if (cset_idlen > 32)
  {
  ABORT_MACRO(44);		/* Length of cset ID in CFF too big */
  return (TRUE);
  }

for (i=0; i < cset_idlen; i++) 
  cset_id [i] = *((BYTE *)(bufptr + cset_idptr + i));

if (i < 32)
  cset_id [i] = 0;	/* null terminate the cset ID */

#ifdef WITHOUTPUT
    	    fprintf	(fptr, "\n    Length of charset ID string: %xH.", 
    			cset_idlen);
    	    fprintf 	(fptr, "\n    Ptr to charset ID string: %xH.",
    			cset_idptr);
    	    fprintf 	(fptr, "\n    Charset ID string: %s", 
    			cset_id);

    	    if (cset_id [0] == '0')
    		fprintf (fptr, 
    			"\n    [As intermediate, use '(', ')', '*' or '+'.]");
    	    else if (cset_id [0] == '1')
    		fprintf (fptr, "\n    [As intermediate, use '-', '.' or '/'.]");
#endif


/* Check for multiple font segments (anything > 1 in the
 * font segment table).
 */

if ( ( fontsegaddr = GET_LONG(bufptr + OFFSET_FONTSEGLIST_REG_ADDR))
     &&
     GET_LONG(bufptr + OFFSET_FONTSEGLIST_REG_SIZE)
     &&
     ((GET_LONG(bufptr + fontsegaddr)) > 1 )
   )
  {

#ifdef WITHOUTPUT
  fprintf (fptr, "\nWARNING:  Multiple font segments (%xH) detected ...\n",
 			 GET_LONG(bufptr + fontsegaddr) );
#endif

  ABORT_MACRO(45); 	/* Multiple font segments detected */
  return (TRUE);
  }

if ( GET_LONG(bufptr + OFFSET_STRING_POOL_REGION_SIZE)
     &&
     (GET_LONG(bufptr + OFFSET_STRING_POOL_REGION_ADDR) > buflen )
   )
  {
  ABORT_MACRO(46);   /* STRING_POOL_REGION ptr points out of range */
  return (TRUE);
  }

if ( GET_LONG(bufptr + OFFSET_CHAR_DEF_REGION_SIZE)
     && 
     ( GET_LONG(bufptr + OFFSET_CHAR_DEF_REGION_ADDR) > buflen )
   )
  {
  ABORT_MACRO(47); /* CHAR_DEF_REGION ptr points out of range */
  return (TRUE);
  }
    /* End of integrity check on pointers */


/*
 * DEC Common Font File Format Appendix G, "Imaging Device Specifics",
 * lists several file conditions that cause rejection of the downloaded
 * font file.  These are:
 * 
 *		Multiple font segments
 *		Compressed rasters
 *		Characters not portrait oriented (i.e., rotated)
 * 		Parameters larger than 16 bits
 * 		More/less than 16 bytes of character parameters 
 *		Font design resolution other than 300 DPI
 *		Pixel aspect ratio other than 1:1
 *
 * In addition, the translator handles only binary font encoding
 * and standard character subsets.
 * 
 * Read in the font file ID, and then check for these conditions.
 */

strncpy (dll_font.new_font_file_id, bufptr + OFFSET_FONT_FILE_ID_STRING, 
							FONT_FILE_ID_SIZE);

/* Mark end of font_file_id with a null */
 
dll_font.new_font_file_id [FONT_FILE_ID_SIZE] = 0;

/* Check that all chars in ID string are valid */
 
for (i=0; i < FONT_FILE_ID_SIZE; i++)
  if ( ( (dll_font.new_font_file_id [i] < '0') || (dll_font.new_font_file_id [i] > '9') )
       && 
       ( (dll_font.new_font_file_id [i] < 'A') || (dll_font.new_font_file_id [i] > 'Z') )
     )
    {
    ABORT_MACRO(48); /* Illegal char in font file ID */
    return (TRUE);
    }



/* Get the 3-byte base-36 character set ID from Bytes 19-21 of the 
 * font file ID and convert it to a 16-bit hex value.
 */

dll_font.dll_char_set_id = 0;

for (i=20, j=1; i>=18; i--, j *= 36)
  {
  temp = dll_font.new_font_file_id [i];
  if ( ('A' <= temp) && (temp <= 'Z') )
    temp = (temp - 'A') + 10;
  else
    temp -= '0';
  dll_font.dll_char_set_id += temp * j;
  }



/* If the character set id, in its 16 bit form, of a font file == 0x3c
 * reassign it as 0xab5 (the id for DEC supplemental). [ 0x3c was the old
 * 'user preference' id which was supposedly synonymous with DEC
 * supplemental. Now 'user preference' is an indirect way of specifying a
 * character set; as such there theoretically no way of there being a 0x3c
 * character set.
 */

if (dll_font.dll_char_set_id == USER_PREFERENCE)
  dll_font.dll_char_set_id = DEC_SUPPLEMENTAL;

/* Check for compressed rasters.
 */
org_flags = GET_LONG(bufptr + OFFSET_ORGANIZATION_FLAGS);
if (org_flags & COMPRESSED_RASTERS_FLAG)
  {
  ABORT_MACRO(49);	/* Compressed rasters detected */
  return (TRUE);
  }

/* Check if font is rotated.  The 17th and 18th bytes of the font ID 
 * (indexes 16 & 17) contain rotation data; they must contain "00".
 * Otherwise, return error.
 */

#ifdef WITHOUTPUT
  fprintf (fptr, "\ndll_ff_id: %s\n", dll_font.new_font_file_id);
#endif    

if ( (dll_font.new_font_file_id [16] != '0') || (dll_font.new_font_file_id [17] != '0') )
  {
  ABORT_MACRO(50);	/* Rotated font detected; not supported */
  return (TRUE);
  }

/* Check for parameters > 16 bits (i.e., "large values").  
 * (Also, check length of the parameters region of the font file.)
 */
if ( GET_LONG(bufptr + OFFSET_FONT_PARAMS_REGION_SIZE) != 124 )
  {
  ABORT_MACRO(51);	/* Font parameters region size is invalid */
  return (TRUE);
  }


if (org_flags & LARGE_VALUE_FLAG)
  {

#ifdef WITHOUTPUT
  fprintf(fptr,"32-bit arithmetic flag is set\n");
#endif

  WARNING_MACRO (52);	/* Large value flag detected */
  }


/* Check for other than 16 parameters per character definition.
 * (Save number of parameter bytes for later use.)
 */
if (  (size_of_char_params= GET_LONG(bufptr+OFFSET_SIZE_OF_CHAR_PARAMETERS) != 16 )
      &&
      (size_of_char_params != 8)
   )
  {
  ABORT_MACRO(53);	/* SIZE_OF_CHAR_PARAMETERS is invalid */
  return (TRUE); 
  }

/* The 28th byte of the font ID contains resolution data; an 'F' 
 * indicates 300 dpi.  Otherwise, return error.
 */
if (dll_font.new_font_file_id [27] != 'F')
  {
  ABORT_MACRO(54);	/* Byte 28 of font ID is not 'F' */
  return (TRUE);
  }


/* The 22nd-25th bytes of the font ID contain character set data; 
 * all four of these bytes must contain 'Z'.  Otherwise, 
 * return error.
 */
for (i=21; i <= 24; i++)
  if (dll_font.new_font_file_id [i] != 'Z')
    {

#ifdef WITHOUTPUT
    fprintf (fptr,"ID of font file is: %s\n", dll_font.new_font_file_id );
#endif

    WARNING_MACRO (56);	/* Bytes 22-25 of font ID not all 'Z' */
    }


/* The 26th-27th bytes of the font ID contain encoding data; a '02' 
 * in this two-byte field indicates binary encoding.  Otherwise, 
 * return error.
 */
if ( ( (BYTE)(dll_font.new_font_file_id [25]) != '0' )
     ||
     ( (BYTE)(dll_font.new_font_file_id [26]) != '2' )
   )
  {
  ABORT_MACRO(57); /* Bytes 26-27 aren't '02'; not binary encoding */
  return (TRUE);
  }

temp_spacing = dll_font.new_font_file_id [7];

if ( (temp_spacing >= 'A') && (temp_spacing <= 'Z') )
   {
   temp_spacing_fraction = dll_font.new_font_file_id [28];   /* Get byte 29 */
   if ( (temp_spacing_fraction >= 'A') && (temp_spacing_fraction <= 'J') )
      {
      temp_spacing_fraction = temp_spacing_fraction - 'A' + 10;
      }
   else if ( (temp_spacing_fraction >= '0') && (temp_spacing_fraction <= '9') )
      {
      temp_spacing_fraction = temp_spacing_fraction - '0';
      }
   else
      {
      ABORT_MACRO(58);	/* Spacing not defined */
      return (TRUE);
	  }

/* note: the following statement was originally :
 *
 * dll_spacing = 7200/(temp_spacing - 'A' + 1 + temp_spacing_fraction * .05);
 * 
 * For lattice compiles we can not have the real constant (0.05), or would
 * have calls to it CXxxx routines, and to keep
 * the precision we can not convert this to a divide by 20.  So instead
 * 720000L was used and a multiple by 5.
 */

   dll_font.dll_spacing = 720000L/(temp_spacing - 'A' + 1 + (temp_spacing_fraction * 5));
   }

else if ( (temp_spacing >= '0') && (temp_spacing <= '9') )
  switch (temp_spacing)
    {
    case '0':   dll_font.dll_spacing = PROPORTIONAL_PITCH;
		break;
    case '1': 	dll_font.dll_spacing = CPI_13_6;
		break;
    case '2': 	dll_font.dll_spacing = CPI_10_3;
    		break;
    case '3': 	dll_font.dll_spacing = CPI_6_8;
    		break;
    case '4': 	dll_font.dll_spacing = CPI_9_34;
    		break;
    case '5': 	dll_font.dll_spacing = CPI_7_5;
    		break;
    case '6': 	dll_font.dll_spacing = CPI_18_75;
    		break;
    case '7': 	dll_font.dll_spacing = CPI_13_3;
    		break;
    case '8': 	dll_font.dll_spacing = CPI_5_7;
    		break;
    case '9': 	dll_font.dll_spacing = CPI_5_4;
    		break;
    }


/* Check for pixel aspect ratio other than 1:1.  A value other than 1
 * in either of the words at PIXEL_ASPECT_RATIO will cause the file
 * to be rejected.  (Note that even though the
 * CFFF manual states that one of the fields the LN03 should 
 * ignore is the Pixel Aspect Ratio, it is still necessary 
 * for the translator to CHECK this region to ensure that the
 * the incoming font does not have an unsupported aspect ratio.)
 */
if ( ( *((WORD *)(bufptr + OFFSET_PIXEL_ASPECT_RATIO)) != 1 )
     ||
     ( *((WORD *)(bufptr + (OFFSET_PIXEL_ASPECT_RATIO+2))) != 1 )
   )
  {
  ABORT_MACRO(55);	/* Pixel aspect ratio not 1:1 */
  return (TRUE);
  }


/* Done with basic error checking and retrieval of font-specific
 * data; now start reading character-specific data from the rest
 * of the font file.
 */

/* Get "first char" and "last char" from font file
 */
if ( (dll_font.dll_first_char = GET_LONG(bufptr + OFFSET_FIRST_CHAR)) < 32  )
  {
  ABORT_MACRO(59);	/* Invalid FIRST_CHARACTER in font file */
  return (TRUE);
  }

#ifdef WITHOUTPUT
  fprintf (fptr, "\nfirst char: %xH\n", dll_font.dll_first_char);
#endif
    
if ( (dll_font.dll_last_char = GET_LONG(bufptr + OFFSET_LAST_CHAR)) > 127 )
  {
  ABORT_MACRO(60);	/* Invalid LAST_CHARACTER in font file */
  return (TRUE);
  }

if ( dll_font.dll_last_char < dll_font.dll_first_char )
  {
  ABORT_MACRO(69);  /* LAST_CHARACTER less than FIRST_CHARACTER in font file.*/
  return (TRUE);
  }

#ifdef WITHOUTPUT
    fprintf (fptr, "\nlast char: %xH\n", dll_font.dll_last_char);
#endif


/* Pick up the error character (i.e., "blob")'s locator
 */
dll_font.blob_locator = GET_LONG(bufptr + OFFSET_ERROR_CHAR_LOCATOR);

#ifdef WITHOUTPUT
  fprintf (fptr, "\nACTUAL Error char locator picked up = %xH", dll_font.blob_locator);
#endif

/*
printf ("blob_locator =%ld =%xH; bufptr =%ld buflen=%ld\n",
		dll_font.blob_locator, dll_font.blob_locator, bufptr, buflen);
*/
/* If the locator is 0, set the "no error char defined" flag
 * then put the "default blob"'s data into the blob.
 */
if (dll_font.blob_locator == 0)
  {
  no_dll_error_char_defined = TRUE;
  blob.baseline = default_blob.baseline;
  blob.no_underlining = default_blob.no_underlining;
  blob.numrows = default_blob.numrows;
  blob.numcols = default_blob.numcols;
  blob.bitmap_ptr = default_blob.bitmap_ptr;
  blob_bitmap_size = BLOB_BITMAP_SIZE;
  dll_even_blob_bitmap_size =dll_odd_blob_bitmap_size = 0;
  dll_even_blob_bitmap_size_land = dll_odd_blob_bitmap_size_land = 0;
  dll_even_blob_bitmap_size_mixed = dll_odd_blob_bitmap_size_mixed  = 0;
  dll_font.blob_l_bearing = default_blob_l_bearing;
  dll_font.blob_r_bearing = default_blob_r_bearing;
  dll_font.blob_width = default_blob_width;	
  }

else
  {	/* error char locator was present */
  /* If high bit of locator is on, file is invalid,
   * because that indicates multiple font segments.
   */
  if (dll_font.blob_locator & BIT31_MASK)
    {
    ABORT_MACRO(63);	/* Bit 31 of error char locator was set */
    return (TRUE);
    } 
	
  /* If Bit 31 is 0, mask out unused bits 24-30 of the locator.
   */
  dll_font.blob_locator &= LOCATOR_MASK;

#ifdef WITHOUTPUT
    	fprintf (fptr, "\nBlob_locator after masking = %xH\n", dll_font.blob_locator);
#endif

  no_dll_error_char_defined = FALSE;

  /* Pick up the flags bytes from the blob char parameters
   */
  temp = GET_LONG(bufptr + dll_font.blob_locator);

#ifdef WITHOUTPUT
    	fprintf (fptr, "\nFlags for downloaded blob are: %xH", temp);
#endif

  /* If the blob's flag bit is not set, file is invalid.
   */
  if ( (temp &  FLAG_FLAG) == 0 )
    {
    ABORT_MACRO(61);	/* Error character's flag flag not set */
    return (TRUE);
    }

  /* If blob has a raster ...
   */
  if ( (temp &  NO_RASTER_FLAG) == 0 )
    {
    /* ... make sure it's a Type 81 (extended) raster ...
     */
    if ( *((UBYTE *)(bufptr + dll_font.blob_locator + OFFSET_TYPE1)) != 0x81 )
      {
      ABORT_MACRO(62);	/* Error char's raster not Type 81 */
      return (TRUE);
      }
    }

  /* Get the blob's width ...
   */
  dll_font.blob_width = GET_LONG(bufptr + dll_font.blob_locator + OFFSET_TYPE_FIELD_NOMINAL_WIDTH);
	
  /* ... and number of columns in blob definition ...
   */
  blob.numcols = *((WORD *)(bufptr + dll_font.blob_locator + OFFSET_COLUMNS));

#ifdef WITHOUTPUT
 	fprintf (fptr, "\nBlob numcols:  %xH", blob.numcols);
#endif

	
  /* ... get its left bearing ...
   */
  dll_font.blob_l_bearing = GET_LONG(bufptr + dll_font.blob_locator + OFFSET_LEFT_BEARING);

  /* ... create its right bearing ...
   */
  dll_font.blob_r_bearing = dll_font.blob_width - dll_font.blob_l_bearing - (blob.numcols * 24);

  /* ... get its baseline (centipts) ...
   */
  blob.baseline = GET_LONG(bufptr + dll_font.blob_locator + OFFSET_RASTER_BASELINE);

#ifdef WITHOUTPUT
	fprintf (fptr, "\nBaseline: %xH", blob.baseline);
#endif

  /* ... get its no_underlining flag ...
   */
  blob.no_underlining = GET_LONG(bufptr + dll_font.blob_locator) & 
							NO_UNDERLINING_FLAG;
#ifdef WITHOUTPUT
	fprintf (fptr,"\nBlob's no_underlining flag: %xH", blob.no_underlining);
#endif

  /* ... get number of rows in blob definition ...
   */
  blob.numrows = *((WORD *)((bufptr + dll_font.blob_locator) + OFFSET_ROWS));

#ifdef WITHOUTPUT
	fprintf (fptr, "\nBlob numrows:  %xH", blob.numrows);
#endif


  /* ... calculate number of words in blob's bit map ...
   */
  number_of_bytes_in_bitmap_port =  (((blob.numcols + 7) / 8 ) * blob.numrows);
  number_of_bytes_in_bitmap_land =  (((blob.numrows + 7) / 8 ) * blob.numcols);
  dll_font.number_of_odd_words_in_bitmaps += number_of_bytes_in_bitmap_port % 2;
  dll_font.number_of_odd_words_in_bm_land += number_of_bytes_in_bitmap_land % 2;

  /* Add total number of words in downloaded blob's bitmap to 
   * total calculated portrait bytes for checking at end.  
   */
  /* Note for mixed case mixed even = MAX (P_even, L_even) is the same as
   * mixed even = mixed odd rounded because MAX(ROUND0 = ROUND(MAX).
   */

  blob_bitmap_size = dll_even_blob_bitmap_size = ((number_of_bytes_in_bitmap_port + 1 ) / 2) * 2;
  dll_even_blob_bitmap_size_land  = ((number_of_bytes_in_bitmap_land + 1 ) / 2) * 2;
  dll_even_blob_bitmap_size_mixed = MAX (dll_even_blob_bitmap_size, dll_even_blob_bitmap_size_land);
  dll_odd_blob_bitmap_size = number_of_bytes_in_bitmap_port;
  dll_odd_blob_bitmap_size_land = number_of_bytes_in_bitmap_land;
  dll_odd_blob_bitmap_size_mixed  = MAX (dll_odd_blob_bitmap_size, dll_odd_blob_bitmap_size_land);
  dll_font.total_portrait_bytes  += dll_even_blob_bitmap_size;
  dll_font.total_landscape_bytes += dll_even_blob_bitmap_size_land;
  dll_font.total_mixed_bytes     += MAX(dll_even_blob_bitmap_size, 
                               dll_even_blob_bitmap_size_land);

  if (dll_even_blob_bitmap_size_mixed 
   != dll_odd_blob_bitmap_size_mixed) 
    dll_font.number_of_odd_words_in_bm_mixed += 1;



#ifdef WITHOUTPUT
  fprintf (fptr, "\nADDING #1.1 num portrait words in blob's bitmap: %dd, running total.", 
 			    			    	dll_font.total_portrait_bytes);
  fprintf (fptr, "\nADDING #1.2 num landscape words in blob's bitmap: %dd, running total.", 
 			    			    	dll_font.total_landscape_bytes);
  fprintf (fptr, "\nADDING #1.3 num mixed words in blob's bitmap: %dd, running total.", 
 			    			    	dll_font.total_mixed_bytes);
#endif

  /* ... and save the ptr to the blob's bitmap.
   */
  blob.bitmap_ptr = bufptr + dll_font.blob_locator + OFFSET_CHARACTER_RASTER;

#ifdef WITHOUTPUT
	fprintf (fptr, "\nBlob's bitmap OFFSET IN DCS BUFFER: %xH.", 
    			dll_font.blob_locator + OFFSET_CHARACTER_RASTER);
	fprintf (fptr, "\nActual PTR stored in blob.bitmap_ptr: %xH.", 
    			blob.bitmap_ptr);
#endif

  }

/* Pick up the "font attributes flags".
 */
font_attr_flags = GET_LONG(bufptr + OFFSET_FONT_ATTRIBUTE_FLAGS);

/*
 * Read "underlined_font_flag" from font_attr_flags.  If this is
 * NOT an underlined font, then get the underline offset and thickness
 * from the font parameters in case underlining is requested (in
 * which case algorithmic underlining will have to be performed).
 *
 * NOTE:  If the "underlined_font_flag" is clear and underlining is
 *	      requested, the NO_UNDERLINING_FLAG for each individual
 *	      character must be checked; if THAT flag is set, it
 *	      indicates that the character is NOT INTENDED to be
 *        algorithmically underlined.  (There is a field in each
 *	      character, ".no_underlining".)
 */
dll_font.dll_ul_font_flag = font_attr_flags & UNDERLINED_FONT_FLAG;

#ifdef WITHOUTPUT
	fprintf (fptr, "\nul_font_flag= %xH", 
	dll_font.dll_ul_font_flag);
#endif

if (dll_font.dll_ul_font_flag == 0)
  {
  dll_font.dll_ul_offset = GET_LONG(bufptr + OFFSET_UNDERLINE_OFFSET);
  dll_font.dll_ul_thickness = GET_LONG(bufptr + OFFSET_UNDERLINE_THICKNESS);
  }

#ifdef WITHOUTPUT
  	fprintf (fptr, "\nul_offset= %xH", dll_font.dll_ul_offset);
	fprintf (fptr, "\nul_thickness= %xH", dll_font.dll_ul_thickness);
#endif


/* Read "struckthru_font_flag" from font_attr_flags.  If this is
 * NOT a struckthru font, then get the strikethru offset and thickness
 * from the font parameters in case strikethru is requested (in
 * which case algorithmic strikethru will have to be performed).
 */
dll_font.dll_strikethru_flag = font_attr_flags & STRUCKTHRU_FONT_FLAG;

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_strikethru_flag= %xH", dll_font.dll_strikethru_flag);
#endif

if (dll_font.dll_strikethru_flag == 0)
  {
  dll_font.dll_strikethru_offset = GET_LONG(bufptr + OFFSET_STRIKETHRU_OFFSET);
  dll_font.dll_strikethru_thickness = GET_LONG(bufptr + OFFSET_STRIKETHRU_THICKNESS);
  }

#ifdef WITHOUTPUT
  	fprintf (fptr, "\nstrikethru_offset= %xH", dll_font.dll_strikethru_offset);
	fprintf (fptr, "\nstrikethru_thickness= %xH", dll_font.dll_strikethru_thickness);
#endif


/*
 * Read "overlined_font_flag" from font_attr_flags.  If this is
 * NOT an overlined font, then get the overline offset and thickness
 * from the font parameters in case overlining is requested (in
 * which case algorithmic overlining will have to be performed).
 *
 */
dll_font.dll_ol_font_flag = font_attr_flags & OVERLINED_FONT_FLAG;

#ifdef WITHOUTPUT
	fprintf (fptr, "\nol_font_flag= %xH", 
	dll_font.dll_ol_font_flag);
#endif

if (dll_font.dll_ol_font_flag == 0)
  {
  dll_font.dll_ol_offset = GET_LONG(bufptr + OFFSET_OVERLINE_OFFSET);
  dll_font.dll_ol_thickness = GET_LONG(bufptr + OFFSET_OVERLINE_THICKNESS);
  }

#ifdef WITHOUTPUT
  	fprintf (fptr, "\nol_offset= %xH", dll_font.dll_ol_offset);
	fprintf (fptr, "\nol_thickness= %xH", dll_font.dll_ol_thickness);
#endif




/* Read "shadow_flag" from font attributes
 */
dll_font.dll_shadow_flag = font_attr_flags & SHADOW_FLAG;

dll_font.dll_shadow_vertical = *((FONT_METRIC *)(bufptr + 
					OFFSET_SHADOW_VERTICAL_OFFSET));
dll_font.dll_shadow_horizontal = *((FONT_METRIC *)(bufptr +
					OFFSET_SHADOW_HORIZONTAL_OFFSET));

#ifdef WITHOUTPUT
  fprintf (fptr,
  "dll_font.dll_shadow_flag %ld, dll_font.dll_shadow_vertical %d,dll_font.dll_shadow_horizontal %d\n", dll_font.dll_shadow_flag, 
		dll_font.dll_shadow_vertical, dll_font.dll_shadow_horizontal);
#endif



/* Read "italic_flag" from font attributes
 */
dll_font.dll_italic_flag = font_attr_flags & ITALIC_FLAG;

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_italic_flag= %xH", dll_font.dll_italic_flag);
#endif

/* Get the font weight (i.e., bolding) from the font attributes;
 * if its value is not BOLD or greater, clear the bold flag.  
 */
dll_font.dll_font_weight = GET_LONG(bufptr + OFFSET_FONT_WEIGHT);

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_font_weight= %xH", dll_font.dll_font_weight);
#endif


/* Check if this is a monospaced or proportional font
 */
dll_font.dll_monospacing_flag = font_attr_flags & MONOSPACING_FLAG;

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_monospacing_flag= %xH", dll_font.dll_monospacing_flag);
#endif

/* Get the type size of the font.
 */
/*** IMPORTANT NOTE:  A DECISION NEEDS TO BE MADE HERE AS TO WHICH OF
 *   THE TWO "TYPE SIZES" SUPPLIED BELOW IS THE CORRECT ONE FOR OUR
 *   NEEDS.  TO DECIDE, SEE SECTION 3.4.8 (P.3-30) IN THE CFFF MANUAL
 *   FOR DESCRIPTION OF "TYPE SIZE" AND SECTION 3.5.5.4 (P.3-55) FOR
 *   DESCRIPTION OF "TOTAL VERTICAL SIZE".  WE MAY ULTIMATELY NEED BOTH,
 *   ONE FOR SELECTING THE FONT, THE OTHER FOR PRINTING THE FONT.
 *
 *    7-OCT-1986 12:29:54 'dll_type_size' is used in the font selection 
 *   process by 'compute_font_for_g_set'. 'dll_total_vertical_size' is used to 
 *   normalize the scale of character metrics in 'dll'.
 ***/

/* (This section fetches "Type Size" from the CFFF.)  The low word 
 * of this longword contains the point size in whole numbers.  The 
 * high word indicates additional ten-thousandths of a point.  
 *
 * The WORD returned in dll_type_size is calculated as follows:
 * low word (# of whole pts) multiplied by 100 (to convert
 * to centipoints), PLUS the high word (which represents # of 
 * 10,000th's points) divided by 100 (to convert to centipts).  
 */

/* Check that the font is less than three inches (MAX_FONT_SIZE = 216 point);
 * this also ensures that an overflow does not occur, since type size is only
 * stored in an unsigned word. 
 */
if ( *((WORD *)(bufptr + OFFSET_TYPE_SIZE)) >= MAX_FONT_SIZE )
  {
#ifdef WITHOUTPUT
  printf ("font size %d", *((WORD *)(bufptr + OFFSET_TYPE_SIZE)));
#endif
  ABORT_MACRO(73);	/* Font size is greater than 3 inches */
  return (TRUE);
  }

dll_font.dll_type_size_whole = *((UWORD *)(bufptr + OFFSET_TYPE_SIZE));
dll_font.dll_type_size_fraction = *((UWORD *)(bufptr + OFFSET_TYPE_SIZE+2));

dll_font.dll_type_size = (*((WORD *)(bufptr + OFFSET_TYPE_SIZE)) * 100);
dll_font.dll_type_size += (*((WORD *)(bufptr + OFFSET_TYPE_SIZE+2)) / 100);

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_type_size= %xH", dll_font.dll_type_size);
#endif


/* (This section fetches "Total Vertical Size" from the CFFF.)
 * This parameter is in units of centipoints; it gives the total
 * vertical size of the type field, which I believe is the one
 * we want for printing.  SEE SECTION 3.5.5.4 (P.3-55) OF THE CFFF MANUAL 
 * FOR DESCRIPTION.
 */
dll_font.dll_total_vertical_size = GET_LONG(bufptr + OFFSET_TOTAL_VERTICAL_SIZE);

#ifdef WITHOUTPUT
     fprintf (fptr, "\ndll_total_vertical_size= %xH", dll_font.dll_total_vertical_size);
#endif

if ( dll_font.dll_total_vertical_size >= MAX_FONT_SIZE * 100 )
  {
  ABORT_MACRO(74); /* Font total vertical size is greater than 3 inches */
  return (TRUE);
  }

/* Get average character width for font (in centipoints).
 *
 * THIS WILL BE OVER-WRITTEN; SEE 'dll_space_width' BELOW.
 */
dll_font.dll_average_char_width = GET_LONG(bufptr + OFFSET_AVERAGE_CHAR_WIDTH);

#ifdef WITHOUTPUT
      fprintf (fptr, "\ndll_average_char_width= %lxH", dll_font.dll_average_char_width);
#endif

/* Get the horizontal proportion of the font. (See cfff_offsets.h for
 * possible values of this variable.  It is comprised of two longwords,
 * one containing a whole number and one a fractional value.)
 */
dll_font.dll_font_horiz_prop = GET_LONG(bufptr + OFFSET_FONT_HORIZ_PROPORTION);

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_font_horiz_prop = %xH", dll_font.dll_font_horiz_prop);
#endif

dll_font.dll_horiz_prop_numerator = *((WORD *)(bufptr + OFFSET_FONT_HORIZ_PROP_FRAC));
dll_font.dll_horiz_prop_denominator = *((WORD *)(bufptr +OFFSET_FONT_HORIZ_PROP_FRAC+2));

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_horiz_prop_numerator = %xH",
						dll_font.dll_horiz_prop_numerator);
	fprintf (fptr, "\ndll_horiz_prop_denominator = %xH",
						dll_font.dll_horiz_prop_denominator);
#endif

/* Get the vertical distance for superscripts and subscripts (in 
 * centipoints).
 */
dll_font.dll_superscript_vert = GET_LONG(bufptr + OFFSET_SUPERSCRIPT_VERTICAL);

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_superscript_vert = %lxH", dll_font.dll_superscript_vert);
#endif

dll_font.dll_subscript_vert = GET_LONG(bufptr + OFFSET_SUBSCRIPT_VERTICAL);

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_subscript_vert = %xH", dll_font.dll_subscript_vert);
#endif

dll_font.dll_subscript_vert = GET_LONG(bufptr + OFFSET_SUBSCRIPT_VERTICAL);

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_subscript_vert = %xH", dll_font.dll_subscript_vert);
#endif

/* Get max and min space sizes for font.  NOTE that these are actual
 * interword space lengths in centipoints, NOT percentages! 
 */
dll_font.dll_space_max = GET_LONG(bufptr + OFFSET_MAX_SPACE_WIDTH);

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_space_max = %xH", dll_font.dll_space_max);
#endif



dll_font.dll_space_min = GET_LONG(bufptr + OFFSET_MIN_SPACE_WIDTH);

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_space_min = %xH", dll_font.dll_space_min);
#endif

/* Get width of the space character in this font (in centipts).
 */
dll_font.dll_space_width = GET_LONG(bufptr + OFFSET_WIDTH_OF_SPACE);

#ifdef WITHOUTPUT
	fprintf (fptr, "\ndll_space_width = %xH", dll_font.dll_space_width);
#endif

if ( dll_font.dll_spacing && (dll_font.dll_average_char_width != dll_font.dll_space_width) )
  {
  WARNING_MACRO(71);	/* Inconsistent font widths */
  }

/* Due to an inconsistancy in the PSRM on whether to use space_width or 
 * average_width for mono_spacing of proportionally spaced font, the LN03 uses
 * space_width while this translator uses average_width. To restore
 * compatibility we over-write average_width as follows: 
 */
dll_font.dll_average_char_width = dll_font.dll_space_width;

/* Determine baseline placement by finding the distances above baseline
 * and below baseline.  (NOTE THAT DISTANCE ABOVE BASELINE IS GOING
 * TO BE A *NEGATIVE* VALUE.)
 * (QUESTION:  What are these values used for?  Each character 
 *             definition contains an explicit "baseline" field.)
 */
dll_font.dll_above_baseline_dist = GET_LONG(bufptr + OFFSET_ABOVE_BASELINE_OFFSET);

   dll_font.dll_below_baseline_dist = GET_LONG(bufptr + OFFSET_BELOW_BASELINE_OFFSET);

/* Get encoded scale factor for font (indicates magnification of the
 * font independent of its specified character size).
 *
 * The scale factor will be a 1-byte base 36 value, encoded as 
 * follows:
 *                   code       value         comments
 *
 *                   0        .25             16-up
 *                   1        .254765 
 *                   2        .279081 
 *                   3        .305718 
 *                   4        .333333         9-up
 *                   5        .334898 
 *                   6        .366862 
 *                   7        .401877 
 *                   8        .440235 
 *                   9        .482253
 *                   A        .5              4-up
 *                   B        .528282 
 *                   C        .577350         3-up
 *                   D        .578704 
 *                   E        .633938 
 *                   F        .694444 
 *                   G        .707107         2-up
 *                   H        .760726 
 *                   I        .833333 
 *                   J        .912871 
 *                   K       1                Normal
 *                   L       1.09545          TeX's \magstephalf
 *                   M       1.2              TeX's \magstep1
 *                   N       1.31453  
 *                   O       1.44             TeX's \magstep2
 *                   P       1.57744 
 *                   Q       1.728            TeX's \magstep3
 *                   R       1.89293 
 *                   S       2.0              TeX's "at 20pt"
 *                   T       2.0736           TeX's \magstep4
 *                   U       2.27152 
 *                   V       2.48832          TeX's \magstep5
 *                   W       2.72582 
 *                   X       2.98598 
 *                   Y       3.0              TeX's "at 30pt"
 *                   Z       3.27098 
 *
 * These are mostly factors of sqrt(1.2), with  0.25,  0.333333,  0.5,
 * 0.5735, 0.707107, 2.0 and 3.0 added.
 *
 * NOTE that it is up to the caller of analyze_font_file () to decode
 * dll_scale_factor.
 */
dll_font.dll_scale_factor_code = dll_font.new_font_file_id [12];

/* Get number of character definitions included in this font file
 */

dll_font.dll_num_char_defs = GET_LONG(bufptr + OFFSET_NUM_CHAR_DEFS);

#ifdef WITHOUTPUT
	fprintf	(fptr, "\nNumber of char definitions in this file: %d.",
				    			dll_font.dll_num_char_defs);
#endif

/* Set up first and last character index.
 */

start_idx = dll_font.dll_first_char - 32;
end_idx = dll_font.dll_last_char - 32;


if ( dll_font.dll_num_char_defs != end_idx - start_idx + 1 )
  {
  WARNING_MACRO (75); /* Character number does not agree with last-first range*/
#ifdef WITHOUTPUT
  fprintf (fptr, 
          "\nNumber of char definitions in this file (%d) does not match number of first char (%d) and number of lsst char (%d)",
	    dll_font.dll_num_char_defs, start_idx, end_idx);
#endif
  }


/* Now cycle through each character defined, check that its type=81H, 
 * and if so, "pick out" its width, left bearing, baseline, and raster.
 */

if (start_idx)
  {
  /* here need to put "blobs" in slots[0-(start_idx-1)] of the
   * dll_char table, and the corresponding "blob" info in the
   * same slots in the width, l_bearing, and r_bearing tables.
   */
  for (i=0; i<start_idx; i++)
    substitute_blob_for_char (&dll_font.dll_char [i], i);	    

#ifdef WITHOUTPUT
   fprintf (fptr, "\n\nDll_char [0] through [%d] were replaced by blob.\n",i-1);
#endif
  }


/* Actual character loop, the stuff above was just initialization
 */

for (i=start_idx;i <= end_idx; i++) 
  {	/*Start For All characters loop*/

#ifdef WITHOUTPUT
	fprintf (fptr, "\n\n i= %d", i);
#endif

  /* Create an index into the character directory to pick up
   * each succeeding character's locator.
   */
  char_dir_idx = (i - start_idx) * LONGWORD;

#ifdef WITHOUTPUT
  	fprintf (fptr, "\nchar_dir_idx = %xH", char_dir_idx);
#endif    	

  if ( ( temploc = GET_LONG((bufptr + OFFSET_CHAR_DIR_REGION) + char_dir_idx)) > buflen )
    {
    ABORT_MACRO(64);	/* Character locator pts out of range */
    return (TRUE);
    }
  else 
    current_locator = temploc;

#ifdef WITHOUTPUT
      fprintf (fptr, "\nACTUAL Current locator [%d] = %xH", i, current_locator);
#endif



  /* If a character's locator is 0, then use a reference to the blob.
   *
   * The CFFF, in § 3.3.16.1-2, clearly indicates that the 'FIRST CHARACTER'
   * and 'LAST CHARACTER', which control the range of this for-loop, pertain
   * to the printable characters of the character set and should exclude
   * non-printable character (7/f in particular was cited as an example).
   * Nevertheless, font file creators have, as usual, been ignoring the CFFF
   * guidelines and, with the elan of hackers, have been exploiting the fact
   * that the error character is usual placed after the last printable
   * character (e.g., in ASCII 7/e = '~') In particular, they frequently
   * specify the 'LAST CHARACTER' as '7/f'. In this way they ambiguously
   * show that '7/f' is an error character regardless of whether the font
   * file is a character set of 94 or 96 characters. (To date, all character
   * sets have been the 94 type.) Actually, how '7/f' is handled, unless it
   * IS printable, should be outside the perview of this loop and would be
   * if the CFFF were enforced. Since the CFFF is not enforced we have to
   * make the following check to ensure that the error character bit-map
   * size is not counted more than once. 
   *
   * Not only do we have to contend with this irregularity but also there is
   * tendency for font file designers to reference the extra last character
   * (i.e. '7/f') as the error character directly (i.e., by its locator in the
   * CFFF) rather than indirectly (i.e., with a '0' locator, indicating a
   * non-defined character) as is done with all other undefined characters. In
   * other words, there is complete disregard of the proper meaning, or at
   * least use, of 'LAST CHARACTER'. 
   */ 

  if ( (current_locator) 
       &&
       (current_locator != dll_font.blob_locator) 
     )
    {
    /* (Here can proceed with "normal" way of getting character)
     *
     *	current_locator is either the locator for a real character
     *	or the blob_locator
     *
     * If high bit of locator is on, character replacement has been done
     */


/* Test if the current character's locator is increasing in value, as a proper
 * CFFF style font should.  If not, see if it is the same as a previous one.
 * If it is, character replacement is used so that only one bitmap is stored.
 * Else, the downloading is aborted.
 */

/* CHARACTER REPLACEMENT ******************************
 *
 *    if i > start_idx	    
 *    {
 *	last_char_dir_idx = (i - start_idx -1) * LONGWORD;
 *	last_locator = *((LONG*)((bufptr+OFFSET_CHAR_DIR_REGION) +
 *			last_char_dir_idx));
 *
 *	if current_locator > last_locator; /* Just continue, is correct 
 *
 *	else	    /* locators are not increasing in value 
 *	{
 *	  WARNING_MACRO(87); /* Incorrect CFFF style, locator should increase 
 *    
 *	   /* Test if this locator was used previously 
 *
 *	  for (j=start_idx; j=i-1; j++)
 *	  {
 *	    search_char_dir_idx = (j - start_idx) * LONGWORD;
 *          if current_locator = *((LONG*)((bufptr + OFFSET_CHAR_DIR_REGION
 *					+ search_char_dir_idx))
 *	    {
 *	      WARNING(88);  /* Will do char. replacement to correct 
 *	      sub_flag = TRUE;
 *	      dll_font.dll_char[i].substitute_flag = 0 - (MAX_CSET_SIZE -1)
 *	      current_locator &= CHAR_REPLACEMENT_MASK; 
 *            current_locator |= BIT31_MASK;
 *	      curent_locator &= i;
 * 	    }
 *	  }	/* end of for 
 *	  if sub_flag = TRUE
 *	  {
 *	    sub_flag = FALSE;
 *	    ABORT_MACRO(89);    /* locator is invalid and can not do a
 *					character replacement 
 *	  }
 *	}   /* end of replacement check else
 *    }	/* end of if not the first locator if 
 *
 */



/********* CHARACTER REPLACEMENT****************
 *
 * Remove this when character replacement is added
 *
 */

    if (current_locator & BIT31_MASK) 
      {
      ABORT_MACRO(65); /* Bit 31 of a character locator was set */
      return (TRUE);
      } 



/* CHARACTER REPLACEMENT *****************************
 *
 *
 *    if (current_locator & BIT31_MASK) ;
 *    else 
 *    {
 *     }  /* This bracket goes at the end of the character loop
 *
 */

	
    /* But if Bit 31 was off, mask out unused bits of the locator.
     */

    current_locator &= LOCATOR_MASK;

#ifdef WITHOUTPUT
    	fprintf (fptr,"\ncurrent_locator after masking unused bits = %xH\n",
    							current_locator);
#endif


    /* Get character width (in centipts) for width table
     */
    dll_font.dll_width_tbl [i] = GET_LONG(bufptr + current_locator + OFFSET_TYPE_FIELD_NOMINAL_WIDTH);

    if ( dll_font.dll_spacing && (dll_font.dll_average_char_width != dll_font.dll_width_tbl [i]) )
      {
      ABORT_MACRO(72);	/* Inconsistent character widths */
      return (TRUE);
      }

    /* Pick up the flags bytes from the current char's parameters
     */
   temp = GET_LONG(bufptr + current_locator);

#ifdef WITHOUTPUT
	fprintf (fptr, "\nFlags for current char are: %xH", temp);
#endif
	
    /* If flag flag bit is not set, file is invalid
     */
    if ( (temp &  FLAG_FLAG) == 0 )
      {
      ABORT_MACRO(66);	/* Character's flag flag was not set */
      return (TRUE);
      }

    /* If this character has a raster ...
     */
    if ( (temp &  NO_RASTER_FLAG) == 0 ) 
      {

      /* ... make sure it's a Type 81 (extended) raster ...
       */
      if (*((UBYTE *)(bufptr + current_locator + OFFSET_TYPE1)) != 0x81) 
        {
	ABORT_MACRO(67); /* Character not a Type 81 raster */
        return (TRUE);
        }
	
      /* ... get number of columns in this character ...
       */
      dll_font.dll_char [i] .numcols = *((WORD *)((bufptr + current_locator) +
							OFFSET_COLUMNS));

#ifdef WITHOUTPUT
	fprintf (fptr, "\nNumcols:  %xH", dll_font.dll_char [i].numcols);
#endif
	
      /* ... get its left bearing for l_bearing_tbl ...
       */
      dll_font.dll_l_bearing_tbl [i] = GET_LONG(bufptr + current_locator + OFFSET_LEFT_BEARING);

      /* ... create right bearing for r_bearing_tbl ...
       */
      dll_font.dll_r_bearing_tbl [i] = dll_font.dll_width_tbl [i] - dll_font.dll_l_bearing_tbl [i] -
						    (dll_font.dll_char [i].numcols * 24);

      /* ... get its baseline (centipts) ...
       */
      dll_font.dll_char [i].baseline = GET_LONG(bufptr + current_locator + OFFSET_RASTER_BASELINE);

#ifdef WITHOUTPUT
	fprintf (fptr, "\nBaseline: %xH", dll_font.dll_char [i].baseline);
#endif

      /* ... get number of rows in this character ...
       */
      dll_font.dll_char [i].numrows = *((WORD *)((bufptr + current_locator) +
								OFFSET_ROWS));

#ifdef WITHOUTPUT
	fprintf (fptr, "\nNumrows:  %xH", dll_font.dll_char [i].numrows);
#endif

      /* ... get its no_underlining flag ...
       */
      dll_font.dll_char [i].no_underlining = (GET_LONG(bufptr + current_locator) & NO_UNDERLINING_FLAG);

#ifdef WITHOUTPUT
      fprintf (fptr, "\nNo_underlining flag: %xH", dll_font.dll_char [i].no_underlining);
#endif

      /* ... calculate number of words in its bit map ...
       */
      number_of_bytes_in_bitmap_port = ((dll_font.dll_char[i].numcols + 7)/ 8)*
							dll_font.dll_char[i].numrows;
      number_of_bytes_in_bitmap_land = ((dll_font.dll_char[i].numrows + 7)/ 8)*
							dll_font.dll_char[i].numcols;
      number_of_bytes_in_bitmap_mixed = MAX (number_of_bytes_in_bitmap_port,
                                             number_of_bytes_in_bitmap_land);

#ifdef WITHOUTPUT
    fprintf (fptr, "\nCALCULATED num bytes in this char's bitmap: %dd, (%xH).", 
		number_of_bytes_in_bitmap_port, number_of_bytes_in_bitmap_port );
#endif

      /* ... and save a ptr to where the bitmap for this character
       * begins IN THE DOWNLOAD BUFFER (i.e., its offset from 
       * bufptr). 
       */
      dll_font.dll_char [i].bitmap_ptr = bufptr + current_locator +
							OFFSET_CHARACTER_RASTER;

#ifdef WITHOUTPUT
	fprintf (fptr, "\nBitmap offset in buffer: %xH.", 
        			(current_locator + OFFSET_CHARACTER_RASTER));
        fprintf (fptr, "\nActual bitmap ptr: %xH.", dll_font.dll_char [i].bitmap_ptr);
#endif

      if ( !dll_font.dll_char[i].numcols || !dll_font.dll_char[i].numrows )
        dll_font.dll_char [i].bitmap_ptr = NULL_BITMAP;

      } /* end of if raster */
    else
      { /* no raster */
      dll_font.dll_char [i].bitmap_ptr = NULL_BITMAP;
      number_of_bytes_in_bitmap_port  = 0;
      number_of_bytes_in_bitmap_land  = 0;
      dll_font.number_of_odd_words_in_bm_mixed = 0;
      }



    /* Track TOTAL number of words in all STORED bitmaps for
     * checking at end.
     */
    dll_font.total_portrait_bytes  += ( (number_of_bytes_in_bitmap_port + 1 ) / 2) * 2;
    dll_font.total_landscape_bytes += ( (number_of_bytes_in_bitmap_land + 1 ) / 2) * 2;
    dll_font.total_mixed_bytes     += ( (number_of_bytes_in_bitmap_mixed + 1 ) / 2) * 2;
    dll_font.number_of_odd_words_in_bitmaps += number_of_bytes_in_bitmap_port % 2;
    dll_font.number_of_odd_words_in_bm_land += number_of_bytes_in_bitmap_land % 2;
    dll_font.number_of_odd_words_in_bm_mixed += number_of_bytes_in_bitmap_mixed % 2;

    
    
#ifdef WITHOUTPUT
  fprintf (fptr, "\nADDING #2.1 num portrait words in char bitmap: %dd, running total.", 
 			    			    	dll_font.total_portrait_bytes);
  fprintf (fptr, "\nADDING #2.2 num landscape words in char bitmap: %dd, running total.", 
 			    			    	dll_font.total_landscape_bytes);
  fprintf (fptr, "\nADDING #2.3 num mixed words in char bitmap: %dd, running total.", 
 			    			    	dll_font.total_mixed_bytes);

    /* Finally, for each character, dump its bitmap in "stars and dots"
     * form.
     */
    if (number_of_bytes_in_bitmap_port)
      {
      /* printf ("Going to stars and dots "); */
      stars_and_dots (dll_font.dll_char [i].numrows, 
			dll_font.dll_char [i].numcols, 
			dll_font.dll_char [i].bitmap_ptr, fptr);
      /* printf ("Back from stars and dots\n"); */
      }
#endif

    }
  else
    {
    /* character locator was undefined or referenced the blob, so ...
     *
     * Copy the blob data into the current char
     * directly, without referencing any locators.
     */
    substitute_blob_for_char (&dll_font.dll_char [i], i);	
    } 

  } /* end of character loop */

/* Now, fill any remaining slots in the character definition
 * table, width table, and left and right bearing tables with 
 * the error character's definition.
 */
for (i = end_idx + 1; i < (MAX_CSET_SIZE-1); i++)
  substitute_blob_for_char (&dll_font.dll_char [i], i);

/* Fill the last entry (97th char) with the blob character.
 */

i = MAX_CSET_SIZE - 1;
dll_font.dll_char [i] .baseline = blob.baseline;
dll_font.dll_char [i] .no_underlining = blob.no_underlining;
dll_font.dll_char [i] .numrows = blob.numrows;
dll_font.dll_char [i] .numcols = blob.numcols;
dll_font.dll_char [i] .bitmap_ptr = blob.bitmap_ptr;

dll_font.dll_l_bearing_tbl [i] = dll_font.blob_l_bearing;
dll_font.dll_r_bearing_tbl [i] = dll_font.blob_r_bearing;
dll_font.dll_width_tbl [i] = dll_font.blob_width;	
dll_font.dll_char[i].substitute_flag= MAX_CSET_SIZE + 1;

#ifdef WITHOUTPUT
  if ( (end_idx + 1) == (i-1) )
    fprintf (fptr, "\n\nDll_char [%d] was replaced by blob.\n", end_idx + 1);
  else 
    fprintf (fptr, "\n\nDll_chars [%d] through [%d] were replaced by blob.\n", 
    							end_idx + 1, i-1);

  fprintf (fptr, "\n\nSTORED num portrait bytes in font file: %dd (%xH).", 
		  GET_LONG(bufptr + OFFSET_PORTRAIT_BYTE_COUNT),
		  GET_LONG(bufptr + OFFSET_PORTRAIT_BYTE_COUNT));
  fprintf (fptr, "\nCALCULATED num portrait bytes: %dd (%xH).", 
    			dll_font.total_portrait_bytes, dll_font.total_portrait_bytes);

  fprintf (fptr, "\nNumber of odd words in bit maps: %dd (%xH).", 
	     dll_font.number_of_odd_words_in_bitmaps, dll_font.number_of_odd_words_in_bitmaps );
  fprintf (fptr, "\nNumber of bytes in dll blob bitmap: %dd (%xH).", 
	     dll_even_blob_bitmap_size, dll_even_blob_bitmap_size );
#endif

/* 
 * When calculating the number of bytes in portrait, they (the creator 
 * of the fonts) forget to include the blob and/or rounding. Here we will
 * try to be more accepting of the fonts and print them even if they are 
 * wrong. The following "if" can be summarized by the following:
 * 
 * 1) calculated = stored			(They did it right)
 * 2) calculated = stored + rounding		(They forgot rounding)
 * 3) calculated = stored + blob_even		(They forgot blob)
 * 4) calculated = stored + blob_odd + rounding	(They forgot rounding and blob)
 */
if ( dll_font.total_portrait_bytes != (temp = GET_LONG(bufptr+OFFSET_PORTRAIT_BYTE_COUNT)))
  {

#ifdef WITHOUTPUT
    printf ("ID of bad font file is: %s\n", dll_font.new_font_file_id );

    printf ("STORED num portrait bytes in font file: %dd (%xH).\n", 
		GET_LONG(bufptr + OFFSET_PORTRAIT_BYTE_COUNT),
		GET_LONG(bufptr + OFFSET_PORTRAIT_BYTE_COUNT));
    printf ("CALCULATED num portrait bytes: %dd (%xH).\n", 
    			dll_font.total_portrait_bytes, dll_font.total_portrait_bytes);
    printf ("CALCULATED num portrait bytes not rounding odd bytes: %dd (%xH).\n", 
    			dll_font.total_portrait_bytes - dll_font.number_of_odd_words_in_bitmaps ,
			dll_font.total_portrait_bytes - dll_font.number_of_odd_words_in_bitmaps );
    printf ("CALCULATED num portrait bytes, not counting blob: %dd (%xH).\n", 
    			dll_font.total_portrait_bytes - dll_even_blob_bitmap_size,
			dll_font.total_portrait_bytes - dll_even_blob_bitmap_size );
    printf ("CALCULATED num portrait bytes not rounding odd bytes, not counting blob: %dd (%xH).\n", 
    			dll_font.total_portrait_bytes - dll_font.number_of_odd_words_in_bitmaps - dll_odd_blob_bitmap_size,
			dll_font.total_portrait_bytes - dll_font.number_of_odd_words_in_bitmaps - dll_odd_blob_bitmap_size );

#endif

#ifdef WITHOUTPUT
    fprintf (fptr, "\nWARNING:  Stored/calculated byte counts don't match.");
#endif
}

if ( ( dll_font.total_portrait_bytes != temp )
  && ( dll_font.total_portrait_bytes != temp + dll_font.number_of_odd_words_in_bitmaps )
  && ( dll_font.total_portrait_bytes != temp + dll_even_blob_bitmap_size )
  && ( dll_font.total_portrait_bytes != temp + dll_font.number_of_odd_words_in_bitmaps + dll_odd_blob_bitmap_size )
   )
  {
  WARNING_MACRO(68); /* Stored/calc'd portrait byte counts don't match */
  }


if ( dll_font.total_portrait_bytes == temp + dll_font.number_of_odd_words_in_bitmaps) 
   WARNING_MACRO(76);  /* Stored/clac'd portrait byte count did'nt include rounding */
if ( dll_font.total_portrait_bytes == temp + dll_even_blob_bitmap_size)
   WARNING_MACRO(77);  /* Stored/clac'd portrait byte count did'nt include blob */
if ( dll_font.total_portrait_bytes == temp + dll_font.number_of_odd_words_in_bitmaps + dll_odd_blob_bitmap_size)
   WARNING_MACRO(78);  /* Stored/clac'd portrait byte count did'nt include rounding and blob */

/* Now do the same for LANDSCAPE */
if ( dll_font.total_landscape_bytes != (temp = GET_LONG(bufptr+OFFSET_LANDSCAPE_BYTE_COUNT)))
  {
#ifdef WITHOUTPUT
    printf ("ID of bad font file is: %s\n", dll_font.new_font_file_id );

    printf ("STORED num landscape bytes in font file: %dd (%xH).\n", 
		GET_LONG(bufptr + OFFSET_LANDSCAPE_BYTE_COUNT),
		GET_LONG(bufptr + OFFSET_LANDSCAPE_BYTE_COUNT));
    printf ("CALCULATED num landscape bytes: %dd (%xH).\n", 
    			dll_font.total_landscape_bytes, dll_font.total_landscape_bytes);
    printf ("CALCULATED num landscape bytes not rounding odd bytes: %dd (%xH).\n", 
    			dll_font.total_landscape_bytes - dll_font.number_of_odd_words_in_bm_land ,
			dll_font.total_landscape_bytes - dll_font.number_of_odd_words_in_bm_land );
    printf ("CALCULATED num landscape bytes, not counting blob: %dd (%xH).\n", 
    			dll_font.total_landscape_bytes - dll_even_blob_bitmap_size_land,
			dll_font.total_landscape_bytes - dll_even_blob_bitmap_size_land );
    printf ("CALCULATED num landscape bytes not rounding odd bytes, not counting blob: %dd (%xH).\n", 
    			dll_font.total_landscape_bytes - dll_font.number_of_odd_words_in_bm_land - dll_odd_blob_bitmap_size_land,
			dll_font.total_landscape_bytes - dll_font.number_of_odd_words_in_bm_land - dll_odd_blob_bitmap_size_land );

#endif

#ifdef WITHOUTPUT
    fprintf (fptr, "\nWARNING:  Stored/calculated byte counts don't match.");
#endif
  }

if ( ( dll_font.total_landscape_bytes != temp )
  && ( dll_font.total_landscape_bytes != temp + dll_font.number_of_odd_words_in_bm_land ) /* + ( (blob.numrows * blob.numcols) %2 ) */
  && ( dll_font.total_landscape_bytes != temp + dll_even_blob_bitmap_size_land )
  && ( dll_font.total_landscape_bytes != temp + dll_font.number_of_odd_words_in_bm_land + dll_odd_blob_bitmap_size_land )
   )
  {
  WARNING_MACRO(79); /* Stored/calc'd landscape byte counts don't match */
  }
if ( dll_font.total_landscape_bytes == temp + dll_font.number_of_odd_words_in_bm_land) 
   WARNING_MACRO(80);  /* Stored/clac'd landscape byte count did'nt include rounding */
if ( dll_font.total_landscape_bytes == temp + dll_even_blob_bitmap_size_land)
   WARNING_MACRO(81);  /* Stored/clac'd landscape byte count did'nt include blob */
if ( dll_font.total_landscape_bytes == temp + dll_font.number_of_odd_words_in_bm_land + dll_odd_blob_bitmap_size_land)
   WARNING_MACRO(82);  /* Stored/clac'd landscape byte count did'nt include rounding and blob */

/* Now do the same for MIXED */
if ( dll_font.total_mixed_bytes != (temp = GET_LONG(bufptr+OFFSET_TOTAL_MIXED_BYTE_COUNT)))
  {
#ifdef WITHOUTPUT
    printf ("ID of bad font file is: %s\n", dll_font.new_font_file_id );

    printf ("STORED num mixed bytes in font file: %dd (%xH).\n", 
		GET_LONG(bufptr + OFFSET_TOTAL_MIXED_BYTE_COUNT),
		GET_LONG(bufptr + OFFSET_TOTAL_MIXED_BYTE_COUNT));
    printf ("CALCULATED num mixed bytes: %dd (%xH).\n", 
    			dll_font.total_mixed_bytes, dll_font.total_mixed_bytes);
    printf ("CALCULATED num mixed bytes not rounding odd bytes: %dd (%xH).\n", 
    			dll_font.total_mixed_bytes - dll_font.number_of_odd_words_in_bm_mixed ,
			dll_font.total_mixed_bytes - dll_font.number_of_odd_words_in_bm_mixed );
    printf ("CALCULATED num mixed bytes, not counting blob: %dd (%xH).\n", 
    			dll_font.total_mixed_bytes - dll_even_blob_bitmap_size_mixed,
			dll_font.total_mixed_bytes - dll_even_blob_bitmap_size_mixed );
    printf ("CALCULATED num mixed bytes not rounding odd bytes, not counting blob: %dd (%xH).\n", 
    			dll_font.total_mixed_bytes - dll_font.number_of_odd_words_in_bm_mixed - dll_odd_blob_bitmap_size_mixed,
			dll_font.total_mixed_bytes - dll_font.number_of_odd_words_in_bm_mixed - dll_odd_blob_bitmap_size_mixed );

#endif

#ifdef WITHOUTPUT
    fprintf (fptr, "\nWARNING:  Stored/calculated byte counts don't match.");
#endif
  }
if ( ( dll_font.total_mixed_bytes != temp )
  && ( dll_font.total_mixed_bytes != temp + dll_font.number_of_odd_words_in_bm_mixed )/* + ( (blob.numrows * blob.numcols) %2 ) */
  && ( dll_font.total_mixed_bytes != temp + dll_even_blob_bitmap_size_mixed )
  && ( dll_font.total_mixed_bytes != temp + dll_font.number_of_odd_words_in_bm_mixed + dll_odd_blob_bitmap_size_mixed )
   )
  {
  WARNING_MACRO(83); /* Stored/calc'd mixed byte counts don't match */
  }
if ( dll_font.total_mixed_bytes == temp + dll_font.number_of_odd_words_in_bm_mixed) /* + ( (blob.numrows * blob.numcols) %2 ) */
   WARNING_MACRO(84);  /* Stored/clac'd mixed byte count did'nt include rounding */
if ( dll_font.total_mixed_bytes == temp + dll_even_blob_bitmap_size_mixed)
   WARNING_MACRO(85);  /* Stored/clac'd mixed byte count did'nt include blob */
if ( dll_font.total_mixed_bytes == temp + dll_font.number_of_odd_words_in_bm_mixed + dll_odd_blob_bitmap_size_mixed)
   WARNING_MACRO(86);  /* Stored/clac'd mixed byte count did'nt include rounding and blob */

  /* Output command to dump # of bytes */
/*
*#ifdef VM_DEBUG
*      printf("\n This font is  %d source bytes long\n",dll_font.total_portrait_bytes);
*#endif
*/

/* Set up a width for 'forced spacing'.
 */
dll_font.dll_width_tbl [MAX_CSET_SIZE] = (dll_font.dll_char [0].numcols && dll_font.dll_char [0].numrows) ?
					dll_font.dll_space_width : dll_font.dll_width_tbl [0];

/* If character set is the 94 character variety and the space glyph is not 
 * a proper space, then force it to be a space.
 */
if ( !(dll_font.dll_char_set_id & 0x2000)
     &&
     dll_font.dll_char [0] .numcols && dll_font.dll_char [0] .numrows )
  {
  dll_font.dll_width_tbl [0] = dll_font.dll_space_width; 
  dll_font.dll_char [0] .numcols =
    dll_font.dll_char [0] .numrows = 0;
  dll_font.dll_char [0] .bitmap_ptr = NULL_BITMAP;
  dll_font.dll_char [0] .substitute_flag = MAX_CSET_SIZE + 1;
  }

if ( !dll_font.dll_total_vertical_size || !dll_font.dll_horiz_prop_numerator
   || !dll_font.dll_horiz_prop_denominator )
  {
  ABORT_MACRO(70); /* Critical font file parameter cannot be zero, but is.*/
  return (TRUE);
  }

#ifdef WITHOUTPUT
  	fprintf (fptr, "\n");


  /* Dump out contents of left bearing table ... */
  for (i=0; i < MAX_CSET_SIZE; i++) 
    fprintf (fptr, "\nLeft bearing [%d]: %xH (%dD)", i, dll_font.dll_l_bearing_tbl [i],
							dll_font.dll_l_bearing_tbl [i]);

  fprintf (fptr, "\n");

  /* ... and dump contents of right bearing table ...
   */
  for (i=0; i < MAX_CSET_SIZE; i++) 
    fprintf (fptr, "\nRight bearing [%d]: %xH (%dD)", i, dll_font.dll_r_bearing_tbl [i],
    							dll_font.dll_r_bearing_tbl [i]);
  fprintf (fptr, "\n");

  /* ... and dump contents of width table
   */
  for (i=0; i < MAX_CSET_SIZE; i++) 
    fprintf (fptr, "\nWidth [%d]: %xH (%dD)", i, dll_font.dll_width_tbl [i],
							dll_font.dll_width_tbl [i]);

  fclose (fptr);
#endif

#ifdef WITHOUTPUT
	printf ("\nSuccessful analyze\n");
#endif

/* If the error character is not defined and we have to use our own homemade 
 * 'blob', then be sure to add its bitmap size to the total_portrait_bytes
 * count. 
 */
if (no_dll_error_char_defined)
  {
  dll_font.total_portrait_bytes += blob_bitmap_size;
  }


dll_font.font_orientation = ORIENT_PORT; /* only portrait supported for now */

return (0);  	/* Nothing wrong was found with the font file. */

}  /* end of analyze_font_file */
