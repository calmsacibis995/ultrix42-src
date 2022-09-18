#ifndef lint
static char SccsId[] = "  @(#)xlc_dll.c	4.1   LPS_ULT_TRN   7/2/90";
#endif
/* file:	xlc_dll.c - Translator routine (called by codegen) specif-
 *		ically to handle downloaded fonts.
 * created:	laf     26-JUN-1986 10:48:17 
 *
 * edit:	nv 26-JUN-1986 Added calculations for, and use of, FontBBox
 *		boundaries.
 *
 *		nv 26-JUN-1986 21:52:13 Changed the declaration of *cur_ps_st
 *		and  *old_ps_st GLOBAL to EXTERNAL.  Removed a typo bug from 
 *		the FontBBox code generation line.  Repositioned the FontBBox 
 *		code generation line between the two final 'end' statements.
 *		
 *		laf 27-JUN-1986 14:20:06 Fixed some spelling errors.
 *
 *		laf 2-JUL-1986 11:13:18  Took all references to macros out
 *		of this module and renamed this routine from "dispose_font"
 *		to "dll".  Codegen now contains a routine called "dispose_
 *		font", which makes necessary macro calls, then calls "dll"
 *		(the contents of this module), then makes final macro calls.
 *
 *		laf 2-JUL-1986 12:15:58 Changed length of dll_char tables
 *		from MAX_CSET_SIZE to MAX_CSET_SIZE+1 to match table sizes
 *		created in xlc_analyze. 
 *		
 *		laf 3-JUL-1986 Added creation of "define_dll_fonts" rtn
 *		to handle changes in the way dll fonts are defined.
 *
 *		nv  3-JUL-1986 16:45:38 Added check [on every d**n bit-map]
 *		to compensate for PostScript's inability to handle a null 
 *		bit-map except in a highly prescribed manner.
 *
 *		laf  9-JUL-1986 14:09:33 Adding ulthickness/position,
 *		strikethruthickness/position.
 *
 *		nv  18-JUL-1986 15:36:29 Add provisions, when downloading a 
 *		right half character set, for dll specially downloading a
 *		spacing if the left half of the PS font is empty.
 *
 *	nv	 31-JUL-1986 15:31:09 Changed the position of a parenthesis
 *		which then fix the way ll_y_component and bbox_ll_y_boundary
 *		are calculated.
 *	nv	 11-AUG-1986 14:44:59 Changed the character descriptive 
 *		string for error characters to be '/a128' regardless of whether
 *		it is being stored in the right or left portion of a PS font. 
 *	nv	 18-AUG-1986 13:55:40 Added provisions for forcing the 
 *		error character to be the first character sent to the PS 
 *		machine.
 *		Added provisions for undefined character glyph's to use the 
 *		glypy for the error character.
 *	nv	 20-AUG-1986 10:51:59 Moved definitions of MAX_CSET_SIZE and
 *		NUMBER_OF_PS_DICTIONARIES to xlate.h.
 *	gh	 25-AUG-1986 15:23:18 Put conditionals around dm commands
 *	nv	 2-SEP-1986 17:46:35 Removed references to:
 *			dll_width_tbl [MAX_CSET_SIZE+1],
 *			dll_l_bearing_tbl [MAX_CSET_SIZE],
 *			dll_r_bearing_tbl [MAX_CSET_SIZE],
 *			dll_total_vertical_size;
 *		and replaced them with references to
 *			left_bearing_table [] [256],
 *			width_table [] [256]'
 *			height_table [];
 *		Created and evaluated local variables 'ix' and 'ps_index'.
 *	nv	 3-SEP-1986 10:26:37 Added the following reference
 *			NOSHARE EXTERNAL FONT_TABLE_STRUCT
 *			  font_table [FONT_TABLE_SIZE];
 *		Added font_table entry as third parameter 'dll'.
 *		Removed references to:
 *			dll_ul_offset, dll_strikethru_offset,
 *			dll_ul_thickness, and dll_strikethru_thickness;
 *		and replaced them with references to appropriate font_table
 *		structure items.
 *		Made 'font_bitmap' accessable an external declaration.
 *		Removed references to structure items of 'dll_char' except 
 *		'bitmap_ptr' and replaced them with corresponding references to 
 *		'font_bitmap' structure items. Created local variable
 *		'font_char_entry' to assist in this process.
 *		Removed references to the 'bitmap_ptr' structure items of 
 *		'dll_char' and replaced them with references to 'font_bitmap
 *		.dll_char' structure items.
 *		Created local variable 'bitmap_pointer' to assist in
 *		processing bitmaps.
 *	nv	 4-SEP-1986 15:54:37 Included PS template for defining font
 *		structure at the beginning of 'dll'.
 *		Incremented 'vm_being_used' at the end of 'dll'.
 *	nv	 5-SEP-1986 16:10:52 Modified 'define_dll_font' to define font
 *		based upon the the storage cues in 'ps_font_table' (e.i., the 
 *		right way) as opposed to the contents of the height table (the
 *		hokey way). 
 *		Made 'ps_font_table' accessable an external declaration.
 *      mhs      11-SEP-1986 13:56:42 Improved DLL VM tracing.
 *	nv	  15-SEP-1986 15:51:32 Removed definition of
 *		NUMBER_OF_PS_FIXED_FONT_DICTNRS (it was already defined in
 *		xlate.h). 
 *		Removed "xlm_codegen.h" as an include file.
 *	nv	 19-SEP-1986 13:21:05 Moved the PostScript code string for 
 *		defining a font to 'xlv_ps.c' and referenced it as
 *		'str_define_font'.
 *	nv	 24-SEP-1986 16:11:56 In each 'oprint' call (except the those
 *		coded conditionally with VM_DBUG) replaced the literal string
 *		with a reference to the same string, which has been removed to
 *		xlv_ps. 
 *	nv	 7-OCT-1986 12:05:51 Increased the resolution of floating 
 *		point values in character metrics.
 *
 *      kws	 22-FEB-1988 14:13:24 Changed how the downloaded 
 *		fonts TRN$ANSI_FONT8-TRN$ANSI_FONT32 are created
 *		and accessed.  UniqueID side effect.
 *      
 *      mgb	 12-APR-1988 10:58:21 changing all symbols that start
 *      	with font_table... to vax_font_table...
 *      
 *      mgb	19-APR-1988 17:36 changed ps_font... to paired_font...
 *
 *	mgb	22-APR-1988 17:07
 *		Took out paired_font_table [] since it is no longer needed
 *
 *	kws	 28-JUL-1988 14:46:00 Change the outputing of downloaded
 *		font font metrics to use vax_font_table_entry instead
 * 		of boxnumber.
 *      
 *	kws	 1-AUG-1988 14:52:30 Change define_spaced_fonts to not
 *		look in vax_font_table when redefining spaced fonts.
 *
 *	ejs	 1-SEP-1988 18:56
 *		Modifed references to the vax_font_table.
 *
 *	araj	14-OCT-1988 13:27
 *		Add overline thickness and position to downloading
 *
 *	araj	10-APR-1989 09:04
 *		Re-instate conditional code for PS VM traces
 */

/************************************************************************
 *                                                                      *
 *      COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,            *
 *            1986, 1987, 1988, 1989.   ALL RIGHTS RESERVED.            *
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
#include "xlate.h"
#include "capdl.hc"
#include "xlm_io.hc" 
#include "dbug.h"
#include "xlc_font_dictionary.hc"
#include "xlc_codegen.hc"
#include "xlc_ps.hc"
#include "xlc_dll.hc"

/*----------*/
/* dll (boxnumber, left_half_is_empty, vax_font_table_entry) -- Downloads a new font into the given boxnumber.
 */
dll (boxnumber, left_half_is_empty, vax_font_table_entry)
WORD boxnumber,
     left_half_is_empty,
     vax_font_table_entry;
{
    WORD	temp, i, ii, j, k, k1, num_bytes, bytesout, graphic_set_offset;
    WORD	ix, font_char_entry;
    LONG	longnumrows, longnumcols;
    UBYTE	*bitmap_pointer;

    float	bbox_ll_x_boundary, bbox_ll_y_boundary, bbox_ur_x_boundary;
    float 	bbox_ur_y_boundary, ll_x_component, ll_y_component;
    float	ur_x_component, ur_y_component, real_total_vertical_size;
    float	x_translation_value, y_translation_value;


    /* If boxnumber is odd the graphics set is GR and characters downloaded are
     * /a160 through /a255 (thus the graphics set offset = 160); otherwise, the
     * graphics set is GL and characters downloaded are /a32 through /a127 (and
     * the graphics set offset = 32).
     */
    graphic_set_offset = 
       (vax_font_table_mapping_index [vax_font_table_entry] != MAP_LEFT) 
				? 160 : 32;



    /* Define the font structure */
    oprintf (str_font_struct, boxnumber, boxnumber);


    /* output the PostScript object
	Character Metrics, Character bit maps
	Font Metrics.

	First open the stucture to fill it
    */

    oprintf (str_start_font, boxnumber);

    /* Convert the total vertical size of the font's type field from
     * centipts to pixels for later use:
     */
    temp = height_table [vax_font_table_entry] /24;
    real_total_vertical_size = 
	(float)height_table [vax_font_table_entry];

    oprintf (str_fontmatrix, 
             height_table [vax_font_table_entry], 
             height_table[vax_font_table_entry]);

    oprintf (str_imagemaskmatrix, temp, -temp);
    oprintf (str_start_bitmaps);

    /* Initialize the bounding box parameters (in centipoints) to ludicrous 
     * values guaranteed to be adjusted in the first pass of the following 
     * for loop.
     */
    bbox_ll_x_boundary = 1000000.;
    bbox_ll_y_boundary = 1000000.;
    bbox_ur_x_boundary = -1000000.;
    bbox_ur_y_boundary = -1000000.;

    /* Now loop through all characters in the font and download all their
     * necessary "components" (i.e., bounding boxes, widths, bearings,
     * bitmaps, etc.) to the PS machine.
     */

    font_char_entry = vax_font_table_entry - FIRST_DOWNLOADED_FONT;
    for (ii = 0; ii<MAX_CSET_SIZE; ii++)
      {
      /* The following 'if statement' effects the sending of the error character
       * to the ps machine ahead of all of the other characters.
       */ 
      if (!ii)
        {
        i = MAX_CSET_SIZE - 1;
        ix = VIR_CHAR_BLOB;
        }
      else
        {
        i = ii - 1;
        ix = i + 32;
        }

      /* If this character's bitmap pointer indicates [by being zero] that the 
       * glyph for this character is not defined, then the error character's 
       * glyph will be used for this character.
       */
      if ( font_bitmap[font_char_entry][i].substitute_flag == MAX_CSET_SIZE)
        {
        oprintf (str_blob_ref, i + graphic_set_offset );
        continue;
        } ;


	/*
	** If the sustitute flag is less than MAX_CSET_SIZE, the character is
	** remapped to another character.
	*/
      if ( font_bitmap[font_char_entry][i].substitute_flag < MAX_CSET_SIZE)
        {
        oprintf (str_char_replace,
		i + graphic_set_offset,
		font_bitmap[font_char_entry][i].substitute_flag  );
        continue;
        }




    	/* Calculate the first nine elements of the array which defines each 
	 * character, in order, as follows:
	 *
	 * 1st:  Character width	
	 * 	   o  width table / total vertical size (from CFFF)
	 * 2nd:  x component, lower left corner of character BBox
	 *	   o  left bearing (from CFFF) / total vertical size (from CFFF)
	 * 3rd:  y component, lower left corner of character BBox
	 * 	   o  -((numrows * 24) + character baseline) / total vert. size
	 * 4th:  x component, upper right corner of character BBox
	 *	   o  (left bearing + (numcols * 24)) / total vert. size
	 * 5th:  y component, upper right corner of character BBox
	 * 	   o  -(baseline) / total vertical size
	 * 6th:  width of bitmap 
	 *	   o  numcols
	 * 7th:  height of bitmap
	 *	   o  numrows
	 * 8th:  x translation component
	 *	   o  -((left bearing + 12) / 24)
	 * 9th:  y translation component
	 *	   o  num pixel rows above baseline - .5 (NOTE: The number of
	 *		pixel rows above baseline is calculated as follows:
	 *		-(baseline + 12) / 24.)
	 *
	 * As the 2nd thru the 5th are calculated, determine if they cause 
  	 * any impact to the bounding box for the total font.
	 */
	if ( bbox_ll_x_boundary >
               (ll_x_component = (float)(left_bearing_table [vax_font_table_entry]  [ix] )/ 
						    real_total_vertical_size) )
	    bbox_ll_x_boundary = ll_x_component;

	longnumrows = font_bitmap [font_char_entry] [i] .numrows;

	/* printf("\ni: %d, \n  longnumrows: %ld, \n  baseline: %ld, \nvert size: %ld,\
	 *          \n  float of numerator: %.3f, \n  float of vert size: %.3f, \
	 * 	 \n  numrows *24: %d, \n float(numrows): %.3f, \
	 * 	 \n  float(-numrows*24): %.3f",
	 * 	i, longnumrows, dll_char[i].baseline, height_table [vax_font_table_entry],
	 * 	-(float)(longnumrows*24 + dll_char [i] .baseline), 
	 * 	real_total_vertical_size, longnumrows*24, 
	 * 	(float)longnumrows, (float)(-longnumrows*24));
	 */

	if ( bbox_ll_y_boundary >
	     (ll_y_component = -( (float)
                                   (font_bitmap [font_char_entry] [i].baseline +
						 (longnumrows*24) ) /
                                                      real_total_vertical_size)
             )
           )
	    bbox_ll_y_boundary = ll_y_component;

	longnumcols = font_bitmap [font_char_entry] [i] .numcols;
	if ( bbox_ur_x_boundary <
                 (ur_x_component = (float)(left_bearing_table [vax_font_table_entry] [ix] +
				(longnumcols * 24))/real_total_vertical_size) )
	    bbox_ur_x_boundary = ur_x_component;

	if ( bbox_ur_y_boundary <
   	   (ur_y_component = -(float)font_bitmap [font_char_entry][i].baseline /
						     real_total_vertical_size) )
	    bbox_ur_y_boundary = ur_y_component;

	x_translation_value =
		       -(float)(left_bearing_table [vax_font_table_entry] [ix] + 12) / 24.;
	y_translation_value =
	       -(float)(font_bitmap [font_char_entry] [i] .baseline + 12) / 24.;

    	num_bytes = ((longnumcols + 7) >> 3)*longnumrows;

	/* The following code compensates for a PostScript bug
         */
        if (!longnumcols || !longnumrows)
          { /* if this is a null bitmap, the following values must as shown */
          ll_x_component = 0.;	    /* x component, lower left corner of BBox */
          ll_y_component = 0.;	    /* y component, lower left corner of BBox */
          ur_x_component = 0.;	    /* x component, upper right corner of BBox*/
          ur_y_component = 0.;	    /* y component, upper right corner of BBox*/
          longnumcols = 1;	    /* width of bitmap */
          longnumrows = 1;	    /* height of bitmap */
          x_translation_value = 0.;
          y_translation_value = 0.;
          num_bytes = 0;
          }


        sprintf (str_buffer,"\n/a%d [%.4f %.4f %.4f %.4f %.4f %d %d %.2f %.2f\n<",
    	    (i + (i==96? 32 : graphic_set_offset) ),
     	    (float)(width_table [vax_font_table_entry] [ix]) / real_total_vertical_size,
				    /* from width table */
	    ll_x_component,	    /* x component, lower left corner 
				    of BBox */
	    ll_y_component,	    /* y component, lower left corner of BBox */
	    ur_x_component,	    /* x component, upper right corner 
				       of BBox */
	    ur_y_component,	    /* y component, upper right corner 
				       of BBox */
    	    longnumcols,	    /* width of bitmap */
    	    longnumrows,	    /* height of bitmap */
	    x_translation_value,
	    y_translation_value);
        ps_str (str_buffer);

	bytesout = 0;
	bitmap_pointer = font_bitmap [font_char_entry] [i] .bitmap_ptr;
	for (j=0; j<num_bytes; j++)  
	  {  /*for all the bytes in the char*/
	  k = *bitmap_pointer;
	  k1 = k >> 4;
	  k & = 0x0f;

	  ps_char (inverse_nibble [k]);
	  ps_char (inverse_nibble [k1]);
	
	  bytesout++;
	  if (bytesout >= (longnumcols+7) >> 3)
	    {
	    oprintf (str_next_line);
	    bytesout = 0;
	    }
	  bitmap_pointer++;
	  } /* end of a character */

	oprintf (str_finish_bitmap);

    } /* end for all characters */

    /* If we are downloading into the right half of a PS font (i.e.,
     * graphic_set_offset == 160) and the left half is empty, then we jam
     * a valid space into char_code #32. 
     */
    if ( left_half_is_empty && (graphic_set_offset == 160) )
      {
      sprintf (str_buffer,"\n/a32 [%.3f 0 0 0 0 1 1 0 0 < > ] def \n",
		(float)(vax_font_table [vax_font_table_entry] .space_width) /
						real_total_vertical_size ); 
      ps_str (str_buffer);
      }

    oprintf (str_end);

    sprintf (str_buffer,"/FontBBox [%.3f %.3f %.3f %.3f ] def\n",
	bbox_ll_x_boundary,
	bbox_ll_y_boundary,
	bbox_ur_x_boundary,
	bbox_ur_y_boundary );
    ps_str (str_buffer);

    oprintf (str_ul_thickness, vax_font_table [vax_font_table_entry] .ul_thickness);
    oprintf (str_ul_position, vax_font_table [vax_font_table_entry] .ul_offset);
    oprintf (str_ol_thickness, vax_font_table [vax_font_table_entry] .ol_thickness);
    oprintf (str_ol_position, vax_font_table [vax_font_table_entry] .ol_offset);
    oprintf (str_strikethru_thickness,
			vax_font_table [vax_font_table_entry] .strikethru_thickness);
    oprintf (str_strikethru_position,
			vax_font_table [vax_font_table_entry] .strikethru_offset);

    oprintf (str_end);


}

/*----------*/
define_dll_fonts()
{
WORD i,j,k;


    /* Redefine all downloaded fonts including the newest one */

for (i = 0; i < DLL_FONT_TABLE_SIZE; i++)
  {
  if (dll_font_table [i] .dll_active != INACTIVE) 
    {
    oprintf (str_define_dll_font, 
             i + NUMBER_OF_FIXED_FONT_BOXES, 
             i + NUMBER_OF_FIXED_FONT_BOXES);
    oprintf (str_set_box, 
             i + NUMBER_OF_FIXED_FONT_BOXES, 
             i + NUMBER_OF_FIXED_FONT_BOXES);
    }
  }



}

/*----------*/
define_spaced_fonts()
{
WORD i;


    /* Redefine all spaced fonts including the newest one */

for (i = 0; i < SPACING_FONT_TABLE_SIZE; i++)
  {
  if (spacing_font_table [i].space_active != INACTIVE)
    {
    oprintf (str_define_spaced_font, i + SPACING_OFFSET, i + SPACING_OFFSET);
    oprintf (str_set_box, i + SPACING_OFFSET, i + SPACING_OFFSET);
    }
  }


}
