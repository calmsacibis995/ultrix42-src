#ifndef lint
static char SccsId[] = "  @(#)xlc_font_metrics.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: xlc_font_metrics.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT  CORPORATION 1987,
 *	1988, 1989.  ALL RIGHTS RESERVED.
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
 *-----------------------------------------------------------------
 *
 *   begin edit_history
 *
 *  12-AUG-1988 10:26 ejs
 *	Created.
 *
 *  20-MAR-1989 cp
 *	Removed '(' and ')' around 'm$font_metric' and 'm$character_metric'
 *	for Ultrix port.
 *
 *   end edit_history
 *
 *-----------------------------------------------------------------
 */

/*
 *-----------------------------------------------------------------
 *
 *   begin description
 *
 *   Filename:  font_metrics.c
 *
 *	These routines are informational.  They do not modify anything.
 *
 *   end description
 *
 *-----------------------------------------------------------------
 */

#include    "portab.h"		
#include    "capdl.hc"

#include    "xlc_font_dictionary.hc"
#include    "xlc_font_metrics.hc"	

/* Since all the metrics are similar, a code macro for the functions is used. */

#define	M$FONT_METRIC(m$font_metric,m$font_metric_table)\
\
FONT_METRIC m$font_metric(f)\
FONT_NUMBER f;\
{\
    return ((m$font_metric_table)[f]);\
}\

#define	M$CHARACTER_METRIC(m$character_metric,m$character_metric_table)\
\
FONT_METRIC m$character_metric(f,c)\
FONT_NUMBER f;\
CHAR_INDEX c;\
{\
    return ((m$character_metric_table)[f][c]);\
}\


M$FONT_METRIC(font_get_above_baseline,above_baseline_table)

M$FONT_METRIC(font_get_below_baseline,below_baseline_table)

M$FONT_METRIC(font_get_superscript_height,superscript_height_table)

M$FONT_METRIC(font_get_subscript_height,subscript_height_table)

M$FONT_METRIC(font_get_height,height_table)

M$FONT_METRIC(font_get_average_width,average_width_table)

M$FONT_METRIC(font_get_min,min_table)

M$FONT_METRIC(font_get_max,max_table)

M$CHARACTER_METRIC(font_get_width,width_table)

M$CHARACTER_METRIC(font_get_width_vchar,width_table)

M$CHARACTER_METRIC(font_get_left_bearing,left_bearing_table)

M$CHARACTER_METRIC(font_get_left_bearing_vchar,left_bearing_table)

M$CHARACTER_METRIC(font_get_right_bearing,right_bearing_table)

M$CHARACTER_METRIC(font_get_right_bearing_vchar,right_bearing_table)

/*  VOID font_get_default (orientation, desired_font, returned_font)
**
**  This routine is used at the beginning of compute font to reset the interface
**  with the font dictionnary.
**  It takes the desired orientation as a parameter, so that the dictionary 
**  will only return fonts that can be provided in this orientation.
**  It also takes (for the future) a PTR to a FONT_ID, so that if a true font 
**  server is implemented someday, it can go and fetch the proper font.
**  For the translator, the orientation and desired font are ignored.
**  The get_default is broken into a reset of the scan_index and a get_next 
**  call.
*/

VOID font_get_default (orientation, desired_font, returned_font)
ORIENTATION orientation;
PTR_FONT_ID desired_font;
PTR_FDP returned_font;

{
    /*
     * The index is started off the back of the table and will be decremented.
     * This insures that downloaded fonts are seen first, effectively hiding 
     * the (built-in) fonts.
     */
    font_scan_index = VAX_FONT_TABLE_SIZE;


    /* 
     * font_get_default only sets returned_font, and does not pass a 
     * return value; we only have to call font_get_next and return.
     */

    font_get_next(returned_font);

    return ;
}

/*  VOID font_get_next (returned_font)
**
**  This routine is used to get the next font available for selection
**  It takes a pointer to a FONT_DICT_PARAM structure as a parameter
**  and fills it with relevant information.
**
*/

VOID font_get_next (returned_font)
PTR_FDP returned_font;
{
    while ( (--font_scan_index) >= 0 )
	{
	if (vax_font_table[font_scan_index].vax_active ==ACTIVE)
	    {
	    returned_font->fdp_font_index	= font_scan_index;
	    strncpy (& returned_font->fdp_font_id, 
			 & vax_font_table[font_scan_index].font_file_id,
			 FIDS_FONT_ID) ;
	    returned_font->fdp_horiz_prop_numerator = 
			vax_font_table[font_scan_index].proportion_numerator ;
	    returned_font->fdp_horiz_prop_denominator = 
			vax_font_table[font_scan_index].proportion_denominator ;
	    returned_font->fdp_font_weight = 
			vax_font_table[font_scan_index].character_weight;
	    returned_font->fdp_scaleable_flag	= FALSE;
	    returned_font->fdp_valid_blob_flag  = TRUE;
	    returned_font->fdp_paired_font_flag = 
			vax_font_table[font_scan_index].pairing_code;

	    return ;
	    } ;
	} ;
    
    returned_font->fdp_font_index   = END_OF_DICTIONARY;

}

/*  FONT_NUMBER font_select (selected_font, scale, pair_flag, gset)
**
**  This routine informs the font dictionary that FONT x has been selected as
**  the best match, and should now be brought in and the bitmap cached.
**  It also indicates that the font should be scaled and to what scale for
**  devices that support scaling, and whether it should be paired  and what pair
**  for devices that support pairing.
**  Since the translator will do this on a cache miss during process_char, 
**  nothing needs be done now except generated the paired font index.
*/

FONT_NUMBER font_select (selected_font, scale, pair_flag, gset)
FONT_NUMBER selected_font;
PTR_POINT scale;
UBYTE pair_flag;
GSET_NUMBER gset;
{
if (pair_flag != NO_DESIRED_PAIR)
    {
    return (vax_font_table[selected_font].pairing_font) ;
    }
else
    {
    return ((FONT_NUMBER)END_OF_DICTIONARY);
    } ;
}



/*  VOID font_get_first_sgr (sgr_entry)
*/
VOID font_get_first_sgr (sgr_entry)
PTR_SGR_ENTRY sgr_entry;
{
	sgr_entry->sgr_number = NRCS_END_OF_DICTIONARY;
	return;
}

/*  VOID font_get_next_sgr (sgr_entry)
*/
VOID font_get_next_sgr (sgr_entry)
PTR_SGR_ENTRY sgr_entry;
{
	sgr_entry->sgr_number = NRCS_END_OF_DICTIONARY;
	return;
}


/*  VOID font_get_first_nrcs (nrcs_entry)
*/
VOID font_get_first_nrcs (nrcs_entry)
PTR_NRCS_ENTRY nrcs_entry;
{
	nrcs_entry->nrcs_number = NRCS_END_OF_DICTIONARY;
	return;
}

/*  VOID font_get_next_nrcs (nrcs_entry)
*/
VOID font_get_next_nrcs (nrcs_entry)
PTR_NRCS_ENTRY nrcs_entry;
{
	nrcs_entry->nrcs_number = NRCS_END_OF_DICTIONARY;
	return;
}
