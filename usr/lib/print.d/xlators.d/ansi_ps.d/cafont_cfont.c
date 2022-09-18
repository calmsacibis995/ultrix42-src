#ifndef lint
static char SccsId[] = "  @(#)cafont_cfont.c	4.1   LPS_ULT_TRN   7/2/90";
#endif
/* file: cafont_cfont.c
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
 *  araj   14-MAY-1988 11:38
 *	Creation by splitting CAFONT_SEL.C
 *
 *  araj    22-MAY-1988 21:02
 *	Added processing for assign by font_12 and
 *	assign by font 16
 *
 *  araj    24-MAY-1988 12:32
 *	Modified init_sgr_tbl to use a constant cfont_def_sgr.
 *
 *  mhs      7-JUL-1988 13:34
 *	Move init_sgr_tbl to cafont_sgr.
 *
 *  araj    28-NOV-1988 16:50
 *	Created and use cfont_data_best structure, to gather info
 *	about best font.
 *
 *	Modified update_best, horiz_spacing, .. to use it
 *
 *  ejs	    29-NOV-1988 12:43
 *	Created cfont_cache_widths and called it from compute_font_for_g_set.
 *
 *  araj    30-NOV-1988 10:30
 *	Merged Ed's changes and mine, removed cfont_transform_index, that
 *	was used to differentiate between real ISOs, and built in ISOs.
 *	This is now handled by codegen.
 *
 *  18-DEC-1988 15:19 ejs
 *		Major changes to data structutes.  GLYPH has been modified
 *		to allow struct copies more easily.  The PS output is not
 *		changed (barring bugs of course).
 *
 *  20-MAR-1989 C. Peters
 *		Removed extraneous '&' operators for Ultrix port.
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
 *   Filename:  cafont_cfont.c
 *
 *
 *   This module includes
 *
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
 * invalidate_font()
 * invalidate_font_for_g_set90
 * invalidate_vai()
 * cfont_space()
 * cfont_blob()
 * cfont_explode_attributes()
 * cfont_grade_font()
 * compute_font_for_g_set()
 * cfont_horiz_spacing()
 * init_g_table ()
 * cfont_pairing()
 *
 *   end description
 *
 *-----------------------------------------------------------
 */


#define cafont_cfont	(1)

/*  begin include_file    */

#include    "portab.h"	    /* portability constants */
#include    "cpsys.hc"	    /* global defs, typedefs, etc. */
#include    "cpast.hc"	    /* ANSI State Table & Token Table Declarations */
#include    "cpglobal.hc"   /* globals for CP modules */
#include    "camac.lib_hc"  /* non-dump oprintf */
#include    "caglobal.hc"   /* globals defs for the CARs */


/*  end   include_file    */




/*****  ca_font_init()  *************************************************
 * Do any initialization required to set up for font processing.	*
 ************************************************************************/

VOID ca_font_init ()
   {
    if (xl_st.cur_sgr == 5)
       {
	xl_st.last_gss = xl_st.v_size = 670;	/* Typesize in centipoints */
       }
    else
       {
	xl_st.last_gss = xl_st.v_size = 1000;	/* Typesize in centipoints */
       }

    xl_st.character_proportion = 100;		/* % of char width to height */
    xl_st.user_pref_cset = DEC_SUPPLEMENTAL;
    xl_st.user_pref_repertory = C94;
    xl_st.gl_ptr = &xl_st.g_table [0];
    xl_st.gr_ptr = &xl_st.g_table [2];
    xl_st.ssf = 0;
 
    init_g_table ();
    init_sgr_tbl();


}  /* end of font_init */


/*****  compute_vai() ***************************************************
 * Compute Vertical Advance Increment					*
 ************************************************************************/

VOID compute_vai ()
   {
    /* Set the vertical advance increment equal either to the vertical space 
     * increment, if it is a real non-zero value, or otherwise, to the 
     * estimated total vertical size (in centipoint), controlled by GSS and
     * GSM.  The 'total vertical size' is estimated by multiplying the nominal 
     * point size by 1.152 --this ratio is obtained from built-in 'courier' 
     * font metrics and is taken to be representative. 
     */ 

    /*	First, make sure that GL is pointing to a valid GSET, as a change in the
     *	font selection criteria may have invalidate the choice
     */

    if ( xl_st.gl_ptr->gset_valid == FALSE)
	{
	    compute_font_for_g_set ( xl_st.gl_ptr - (&xl_st.g_table[0]));
	}

    xl_st.vai = (xl_st.vsi != 0)
       ? (xl_st.vsi)
       : (FNT_HEIGHT(xl_st.gl_ptr->gset_map[32].char_font));

    xl_st.vai_valid = TRUE;
   }


/*****  b36_to_bin () ***************************************************
 * Convert base 36 to binary						*
 ************************************************************************/
/*  This function converts a base 36 digit into binary, 
**  the digit must have already been capitalized, and 
**  checked to ensure validity
*/

WORD b36_to_bin (font_id_byte)
UBYTE font_id_byte;

{
  if ('A' <= font_id_byte)
    return ((WORD)(font_id_byte - 'A' + 10));
  else
    return ((WORD)(font_id_byte - '0'));
}

/*****  nbin_to_b36 () **************************************************
 * Convert binary to base 36 (n bytes)					*
 ************************************************************************/
/*  This function converts a binary word in a base 36 string n bytes long
*/

VOID nbin_to_b36 (font_id_ptr, value, t_length)
PUB font_id_ptr;
WORD value;
WORD t_length;
{
UBYTE	temp_byte;
WORD	temp_value;
WORD	i;

temp_value = value;

    for ( i = t_length - 1; i >= 0; i--)
       {
        temp_byte = temp_value % 36;
        temp_value = temp_value / 36;
    
        if (temp_byte < 10)
           {
	    font_id_ptr[i] = temp_byte +'0';
	   }
	else
	   {
    	    font_id_ptr[i] = temp_byte + 'A' - 10;
	   }
       }
   }


/*****  bin_to_b36 () ***************************************************
 * Convert binary to base 36 (1 byte)					*
 ************************************************************************/
/*  This function converts a binary word onto 1 base 36 byte, 
**  there is no provision for overflow.
*/

UBYTE bin_to_b36 (value)
WORD value;
{
        if (value < 10)
        {	return ((UBYTE)(value +'0'));
	}
	else
	{	return ((UBYTE)(value + 'A' - 10));
	}
}


/*****  cfont_type_family () ********************************************
 * Type family								*
 ************************************************************************/

/* This function extracts the type family from a font_file_id
*/

VOID cfont_type_family (font_id_ptr,type_family_ptr)
PTR_FONT_ID font_id_ptr;
PB  type_family_ptr;
{
strncpy (type_family_ptr, font_id_ptr->fi_type_family, FIDS_TYPE_FAMILY);
}

/*****  cfont_spacing () ************************************************
 * spacing								*
 ************************************************************************/

/* This function extracts the spacing from a font_file_id
*/

VOID cfont_spacing (font_id_ptr, spacing_criterion, spacing_value)
PTR_FONT_ID font_id_ptr;
PW spacing_criterion;
PW spacing_value;
{
WORD tmp_spacing_value_fraction;
WORD tmp_spacing_value;

tmp_spacing_value = b36_to_bin (font_id_ptr->fi_spacing);			/* Get byte 8 */
tmp_spacing_value_fraction = b36_to_bin (font_id_ptr->fi_res1);			/* Get byte 29 */

    *spacing_criterion = MONO_SPACED_MODE;
  switch (tmp_spacing_value)
    {
    case 0:	tmp_spacing_value = PROPORTIONAL_PITCH;
		*spacing_criterion =  PROPORTIONAL_MODE;
		break;
    case 1: 	tmp_spacing_value = CPI_13_6;
		break;
    case 2: 	tmp_spacing_value = CPI_10_3;
    		break;
    case 3: 	tmp_spacing_value = CPI_6_8;
    		break;
    case 4: 	tmp_spacing_value = CPI_9_34;
    		break;
    case 5: 	tmp_spacing_value = CPI_7_5;
    		break;
    case 6: 	tmp_spacing_value = CPI_18_75;
    		break;
    case 7: 	tmp_spacing_value = CPI_13_3;
    		break;
    case 8: 	tmp_spacing_value = CPI_5_7;
    		break;
    case 9: 	tmp_spacing_value = CPI_5_4;
    		break;
    default:
		tmp_spacing_value = 7200L/((LONG)tmp_spacing_value - 9L + ((LONG)tmp_spacing_value_fraction /20L ));    
    }
    *spacing_value = tmp_spacing_value;
 

}

/*****  cfont_type_size () *********************************************
 * type size								*
 ************************************************************************/

/*  This function extracts the type size from a font_file_id
**  The actual point size is not the point size, but the point size * scale factor
*/
WORD cfont_type_size (font_id_ptr)
PTR_FONT_ID font_id_ptr;
{
WORD i;
WORD temp;


    for (i=0, temp = 0; i<FIDS_TYPE_SIZE; i++)
    {
    	temp = (temp * 36) + b36_to_bin(font_id_ptr->fi_type_size[i]);
    }
    return (temp*10*cfont_scale_factor(font_id_ptr));
}

/*****  cfont_scale_factor () *******************************************
 * Scale factor								*
 ************************************************************************/

/* This function extracts the scale factor from a font_file_id
*/
WORD cfont_scale_factor (font_id_ptr)
PTR_FONT_ID font_id_ptr;
{
WORD temp;
temp = (b36_to_bin (font_id_ptr->fi_scale_factor));
switch (temp)
    {
/* Test code */
/* we need to modify scale factor to return the floating point values */

	default:    temp = 1;

/*  End test code */

    }
return (temp);
}


/*****  cfont_style() ***************************************************
 * Style								*
 ************************************************************************/

/* This function extracts the style from a font_file_id
*/

WORD cfont_style (font_id_ptr)
PTR_FONT_ID font_id_ptr;
{
WORD i;
WORD temp;

    for (i=0, temp = 0; i<FIDS_STYLE; i++)
    {	
	temp = (temp * 36) + b36_to_bin(font_id_ptr->fi_style[i]);
    }
    if (temp & SLANT)
    {
	temp = ((temp & NO_SLANT) | ITALIC);
    }
    return (temp);
}

/*****  cfont_weight() **************************************************
 * Weight								*
 ************************************************************************/

/* This function extracts the weight from a font_file_id
*/

WORD cfont_weight (font_id_ptr)
PTR_FONT_ID font_id_ptr;
{
return (b36_to_bin (font_id_ptr->fi_weight));
}

/*****  cfont_proportion() **********************************************
 * Proportion								*
 ************************************************************************/

/* This function extracts the proportion from a font_file_id
*/

WORD cfont_proportion (font_id_ptr)
PTR_FONT_ID font_id_ptr;
{
WORD temp_proportion;

    /*	Note, there is no recognized relationship between the codes 
    **	in the font ID, compressed, super compressed, ..
    **	and the actual values received for the GSM
    **	or stored in the font header.
    **
    **	The following code is an attempt (bad) to create such a relationship
    */
temp_proportion = (b36_to_bin (font_id_ptr->fi_proportion));

    if (temp_proportion <= 3) 
    {	
	temp_proportion = 220;
    }
    else
    {
	if (temp_proportion >= 32)
	{
	    temp_proportion = 10;
	}
	else
	{
	    temp_proportion = ( 220-( (temp_proportion-4)*10) );
	}
    }
    return (temp_proportion);

}

/*****  cfont_rotation () ***********************************************
 * Rotation								*
 ************************************************************************/

/* This function extracts the rotation from a font_file_id
*/

WORD cfont_rotation (font_id_ptr)
PTR_FONT_ID font_id_ptr;
{

WORD i;
WORD temp;

    for (i=0, temp = 0; i<FIDS_ROTATION; i++)
    {	
	temp = (temp * 36) + b36_to_bin(font_id_ptr->fi_rotation[i]);
    }
    return (temp);
}

/*****  cfont_cset() ****************************************************
 * Cset									*
 ************************************************************************/

/* This function extracts the character set from a font_file_id
*/

VOID cfont_cset (font_id_ptr, cset)
PTR_FONT_ID font_id_ptr;
PW cset;
{
WORD tmp_cset;
WORD i;

/* Get the 3-byte base-36 character set ID from Bytes 19-21 of the 
 * font file ID and convert it to a 16-bit hex value.
 */
tmp_cset = 0;

for (i=0; i<FIDS_CSET; i++)
  {
      tmp_cset = (tmp_cset*36)+(b36_to_bin (font_id_ptr->fi_cset[i]));
  }

/* If the character set id, in its 16 bit form, of a font file == 0x3c
 * reassign it as 0xab5 (the id for DEC supplemental). [ 0x3c was the old
 * 'user preference' id which was supposedly synonymous with DEC
 * supplemental. Now 'user preference' is an indirect way of specifying a
 * character set; as such there theoretically no way of there being a 0x3c
 * character set.
 */
if (tmp_cset == USER_PREFERENCE)	tmp_cset = DEC_SUPPLEMENTAL;
 
*cset = tmp_cset;

}

/*****  cfont_id_type_family () *****************************************
 * Type family								*
 ************************************************************************/

/* This function inserts the type family into a font_file_id
*/

VOID cfont_id_type_family (font_id_ptr, type_family_ptr)
PTR_FONT_ID font_id_ptr;
PB  type_family_ptr;
{
    strncpy (font_id_ptr->fi_type_family, type_family_ptr, FIDS_TYPE_FAMILY);
}


/*****  cfont_id_spacing () ************************************************
 * spacing								*
 ************************************************************************/

/* This function inserts the spacing into a font_file_id
*/

VOID cfont_id_spacing (font_id_ptr, spacing_criterion, spacing_value)
PTR_FONT_ID font_id_ptr;
WORD spacing_criterion;
WORD spacing_value;
{
WORD temp_fraction;
WORD temp;

    if (spacing_criterion == PROPORTIONAL_MODE)
    {
	font_id_ptr->fi_res1=font_id_ptr->fi_spacing= '0';
    }
    else
    {
	temp_fraction = 0;
	switch (spacing_value)
        {
        case CPI_13_6:	    temp = 1;
			    break;
        case CPI_10_3:	    temp = 2;
			    break;
        case CPI_6_8:	    temp = 3;
			    break;
        case CPI_9_34:	    temp = 4;
			    break;
        case CPI_7_5:	    temp = 5;
			    break;
        case CPI_18_75:	    temp = 6;
			    break;
        case CPI_13_3:	    temp = 7;
			    break;
        case CPI_5_7:	    temp = 8;
			    break;
        case CPI_5_4:	    temp = 9;
			    break;
        default:
			    temp = 7200L / spacing_value;
			    temp_fraction = ((7200L % spacing_value) / 20L);
			    temp += 10L - 1L;

	}
    font_id_ptr->fi_spacing = bin_to_b36(temp); /* 1 CPI is A, 2 is B ... */
    font_id_ptr->fi_res1 = bin_to_b36 (temp_fraction);
    }

}


/*****  cfont_id_type_size () *********************************************
 * type size								*
 ************************************************************************/

/* This function inserts the type size into a font_file_id
*/

VOID cfont_id_type_size (font_id_ptr,type_size)
PTR_FONT_ID font_id_ptr;
WORD type_size;
{
    /* note, internaly, the type size is in centipoints, but for the ID, and the ID 
    ** only, it is decipoints
    */
    nbin_to_b36 (font_id_ptr->fi_type_size, (type_size/10), FIDS_TYPE_SIZE);
}


/*****  cfont_id_scale_factor () *******************************************
 * Scale factor								*
 ************************************************************************/

/* This function inserts the scale factor into a font_file_id
*/

VOID cfont_id_scale_factor (font_id_ptr,scale_factor)
PTR_FONT_ID font_id_ptr;
WORD scale_factor;
{
/* test code */
/* For now scale factor will always be 20, which is 1 */

font_id_ptr->fi_scale_factor = bin_to_b36(20); 


/*    font_id_ptr->fi_scale_factor = bin_to_b36(scale_factor); */

}


/*****  cfont_id_attributes () ******************************************
 * Style								*
 ************************************************************************/

/* This function inserts the style into a font_file_id
*/

VOID cfont_id_attributes (font_id_ptr, requested_attributes)
PTR_FONT_ID font_id_ptr;
WORD requested_attributes;
{
    /* First drop the bold attribute, which goes to weight, not style */
    /* Also drop sub/superscript, that are not font attributes, but   */
    /* just used to select a different font
    requested_attributes = (requested_attributes & (NO_BOLD & NO_SUPERSCR & NO_SUBSCR);


    /* Now convert the result to an ID like style specification */

    nbin_to_b36 (font_id_ptr->fi_style, requested_attributes, FIDS_STYLE);
}


/*****  cfont_id_weight() **************************************************
 * Weight								*
 ************************************************************************/

/* This function inserts the weight into a font_file_id
*/

VOID cfont_id_weight (font_id_ptr,weight)
PTR_FONT_ID font_id_ptr;
WORD weight;
{
    font_id_ptr->fi_weight = bin_to_b36(weight);
}


/*****  cfont_id_proportion() **********************************************
 * Proportion								*
 ************************************************************************/

/* This function inserts the proportion into a font_file_id
*/

VOID cfont_id_proportion (font_id_ptr, proportion)
PTR_FONT_ID font_id_ptr;
WORD proportion;
{
WORD temp_proportion;

    /*	Note, there is no recognized relationship between the codes 
    **	in the font ID, compressed, super compressed, ..
    **	and the actual values received for the GSM
    **	or stored in the font header.
    **
    **	The following code is an attempt (bad) to create such a relationship
    */

    if (proportion > 220) 
    {	
	temp_proportion = 3;
    }
    else
    {
	if (proportion < 10)
	{
	    temp_proportion = 32;
	}
	else
	{
	    temp_proportion = ((220-proportion)/10)+4;
	}
    }
    font_id_ptr->fi_proportion = bin_to_b36(temp_proportion);
}


/*****  cfont_id_rotation () ********************************************
 * Rotation								*
 ************************************************************************/

/* This function inserts the rotation into a font_file_id
*/

VOID cfont_id_rotation (font_id_ptr, rotation)
PTR_FONT_ID font_id_ptr;
WORD rotation;
{
    nbin_to_b36 (font_id_ptr->fi_rotation, rotation, FIDS_ROTATION);
}


/*****  cfont_id_cset() *************************************************
 * Cset									*
 ************************************************************************/

/* This function inserts the character set into a font_file_id
*/

VOID cfont_id_cset (font_id_ptr, cset)
PTR_FONT_ID font_id_ptr;
WORD cset;
{
    nbin_to_b36 (font_id_ptr->fi_cset, cset, FIDS_CSET);
    /*	Until we start supporting the additional bit for CSET ID, byte 30
    **	(was res2) is set to 0.
    */
    font_id_ptr->fi_res2 = bin_to_b36 (0);
}


/*****  cfont_id_csubset() **********************************************
 * Cset									*
 ************************************************************************/

/* This function inserts the character sub set into a font_file_id
*/

VOID cfont_id_csubset (font_id_ptr, csubset_ptr)
PTR_FONT_ID font_id_ptr;
PB  csubset_ptr;
{
    strncpy (font_id_ptr->fi_csubset, csubset_ptr, FIDS_CSUBSET);
}


/*****  cfont_id_encoding () ********************************************
 * Cset									*
 ************************************************************************/

/* This function inserts the encoding into a font_file_id
*/

VOID cfont_id_encoding (font_id_ptr)
PTR_FONT_ID font_id_ptr;
{
    nbin_to_b36 (font_id_ptr->fi_encoding, BIN_CFFF_ENCODING, FIDS_ENCODING);
}


/*****  cfont_id_resolution () ******************************************
 * Resolution								*
 ************************************************************************/

/* This function inserts the resolution into a font_file_id
*/

VOID cfont_id_resolution (font_id_ptr, resolution)
PTR_FONT_ID font_id_ptr;
WORD resolution;
{
    font_id_ptr->fi_resolution = bin_to_b36 (RESOLUTION_300);
}


/*****  cfont_id_reserved () ********************************************
 * Reserved 							*
 ************************************************************************/

/* This function inserts the reserved fields into a font_file_id
** at this time, the only one left is byte 31, reserved for customer, 
** all other reserved bits have been assigned
*/

VOID cfont_id_reserved (font_id_ptr)
PTR_FONT_ID font_id_ptr;
{
    font_id_ptr->fi_res3 = '0';
}


/*****  compute_font() **************************************************
 * Compute Font								*
 ************************************************************************/

VOID compute_font()
{
WORD	i;	/* for-loop index */

  compute_proportional_mode ();

/* Re-compute the font for all four G sets. */
for (i = 0; i < G_TABLE_SIZE; i++)
  compute_font_for_g_set (i);

compute_vai ();
}  /* end of compute_font */


/*****  invalidate_font() ***********************************************
 * Invalidate Font							*
 ************************************************************************/

VOID invalidate_font()
{
WORD	i;	/* for-loop index */

/* Invalidate the font for all four G sets. */
for (i = 0; i < G_TABLE_SIZE; i++)
  xl_st.g_table[i].gset_valid = FALSE;

/* Invalidate the value of VAI as it depends on the font used for GL*/
xl_st.vai_valid = FALSE;

/* Compute font used to have teh side effect of calling 
 * compute proportional mode
 *
 * To avoid breaking too many pieces, this side effect
 * is not transferred to invalidate_font
 *
 * At some future point, it might be worth looking at 
 * all the calls to invalidate_font, and decide which ones
 * really need to call compute_proportional_mode
 */
 
 compute_proportional_mode ();

}  /* end of invalidate_font */



/*****  invalidate_font_for_g_set() *************************************
 * Invalidate Font for one G set					*
 ************************************************************************/

VOID invalidate_font_for_g_set(g_table_entry)
WORD g_table_entry;
{

  xl_st.g_table[g_table_entry].gset_valid = FALSE;

/* Invalidate the value of VAI as it depends on the font used for GL*/
xl_st.vai_valid = FALSE;

}  /* end of invalidate_font_for_g_set */


/*****  invalidate_vai **************************************************
 * Invalidate VAI							*
 ************************************************************************/

VOID invalidate_vai ()
{

/* Invalidate the value of VAI as it depends on the font used for GL*/
xl_st.vai_valid = FALSE;

}  /* end of invalidate_vai */



/*****  cfont_blob() ****************************************************
 * Cfont Blob 								*
 ************************************************************************/
VOID cfont_blob (g_table_entry,cfont_index_best)
WORD	    g_table_entry;
FONT_NUMBER cfont_index_best;
{

/* Prepare the specified G set to print the error character for all 
   character codes. */
WORD	    i;

    for (i = 0; i <= 127; i++)
    {

	xl_st.g_table[g_table_entry].gset_map[i].char_code  = VIR_CHAR_BLOB;
	xl_st.g_table[g_table_entry].gset_map[i].char_font  = cfont_index_best;
    }

}  /* end of cfont_blob */


/*****  cfont_space() ***************************************************
 * Cfont space we don't even have a blob in the best font, use space 	*
 ************************************************************************/
VOID cfont_space (g_table_entry,cfont_index_best)
WORD	    g_table_entry;
FONT_NUMBER cfont_index_best;
{

/* Prepare the specified G set to print the error character for all 
   character codes. */
WORD	    i;

    for (i = 0; i <= 127; i++)
    {

	xl_st.g_table[g_table_entry].gset_map[i].char_code  = VIR_CHAR_SPACE;
	xl_st.g_table[g_table_entry].gset_map[i].char_font  = cfont_index_best;
    }

}  /* end of space */

/*****  cfont_explode_attributes () *************************************
 * cfont explode attributes 						*
 ************************************************************************/
VOID cfont_explode_attributes (cfont_cur,src_font_id)
PTR_CFA cfont_cur;
PTR_FONT_ID src_font_id;
{

cfont_type_family (src_font_id, cfont_cur->cfa_type_family);
cfont_spacing (src_font_id, & cfont_cur->cfa_spacing_criterion, & cfont_cur->cfa_spacing_value);
cfont_cset (src_font_id, & cfont_cur->cfa_cset);

cfont_cur->cfa_type_size	= cfont_type_size (src_font_id);
cfont_cur->cfa_scale_factor	= cfont_scale_factor (src_font_id);
cfont_cur->cfa_style		= cfont_style (src_font_id);
cfont_cur->cfa_weight		= cfont_weight (src_font_id);
cfont_cur->cfa_proportion	= cfont_proportion (src_font_id);
cfont_cur->cfa_rotation		= cfont_rotation (src_font_id);
}

/*****  cfont_grade_font () *********************************************
 * cfont grade font							*
 ************************************************************************/
VOID     cfont_grade_font (cfont_cur, cfont_des, cfont_g, des_pair, fdp_ptr)
PTR_CFA	cfont_cur;			/* exploded attributes of the font to be graded */
PTR_CFA	cfont_des;			/* exploded attributes of the desired font */
PTR_CFG	cfont_g;			/*	structure contining the grade of the font currently 
						under considereration
					*/
UBYTE	des_pair;			/*  What pairing mechanisms can be used to make the CSET */
PTR_FDP	fdp_ptr;			/*  Additional data passed by ditionary, 
					**  What pairing mechanisms does the current font offer 
					**  does it have a blob...
	    				*/
{

    /*	First, clear all fault flags */

    cfont_g->cfg_fault_flags =  FLT_NONE;	 	
    


    /*  Lets start with Type Family */
   
    /*	If there is an exact match, then no fault, grade = exact match 
    **	If the font under consideration is no style (D000000), then no fault, grade = No style fall back
    **	If the font under consideration is builtin  (DBILTIN), then fault, grade = built in fall back
    */	

    /*	Little Trivia, STRNCMP returns false if the strings are equal */

    if	(!(strncmp (cfont_cur->cfa_type_family, cfont_des->cfa_type_family, FIDS_TYPE_FAMILY)))
    {
	cfont_g->cfg_type_family =	EXACT_MATCH;
    }
    else    if	(!(strncmp (cfont_cur->cfa_type_family, dpi_font_string, FIDS_TYPE_FAMILY)))
	    /*	Little Trivia, STRNCMP returns false if the strings are equal */

	    {
		cfont_g->cfg_type_family =   NO_STYLE_FALL_BACK;  
	    }
	    else    if	(!(strncmp (cfont_cur->cfa_type_family, dbuiltin_string, FIDS_TYPE_FAMILY)))
		    /*	Little Trivia, STRNCMP returns false if the strings are equal */

		    {
	    	    	cfont_g->cfg_type_family =   BUILT_IN_FALL_BACK;
			cfont_g->cfg_fault_flags |=  FLT_TYPE_FAMILY;	 	
		    }
		    else
		    {
	    	    	cfont_g->cfg_type_family =   NO_BUILT_IN_FALL_BACK;
			cfont_g->cfg_fault_flags |=  FLT_TYPE_FAMILY;	 	
		    }


    /*	Now do spacing	*/
    
    /*	If the spacing criterion are the same, then spacing criterion = EXACT_MATCH
    **	else MONO_PROP_FALL_BACK
    */

   cfont_g->cfg_spacing_criterion =  (cfont_cur->cfa_spacing_criterion == cfont_des->cfa_spacing_criterion) ?
				    EXACT_MATCH : MONO_PROP_FALL_BACK;

    /*	If the desired and current spacing value are the same, then exact match
    **	If the desired is larger than current, then value grading
    **	If the desired is smaller, then spacing fault and value grading
    */

   cfont_g->cfg_spacing_value = ABS(cfont_cur->cfa_spacing_value - cfont_des->cfa_spacing_value);
   if (cfont_cur->cfa_spacing_value > cfont_des->cfa_spacing_value) 
    {
	cfont_g->cfg_fault_flags |=  FLT_SPACING_VALUE;	 	
    }


    /*	Now do character set */

    /*	If exact match then exact match
    **	if the desired can be obtained by pairing the current with some other font
    **	then PAIRING_FALL_BACK 
    **	If cannot be obtained, but there is a valid BLOB, then CSET fault and BLOB_FALL_BACK
    **	If cannot be obtained, and ther is no valid blob, then CSET fault and NO_BLOB_FALL_BACK
    */

    if (cfont_cur->cfa_cset == cfont_des->cfa_cset) 
    {
        cfont_g->cfg_cset = EXACT_MATCH;
    }
    else
    {	    if (des_pair & fdp_ptr->fdp_paired_font_flag)
	    {
		cfont_g->cfg_cset = PAIRING_FALL_BACK;
	    }
	    else
	    {
		cfont_g->cfg_fault_flags |=  FLT_CSET;	 	
		cfont_g->cfg_cset =  (fdp_ptr->fdp_valid_blob_flag) ?
				    BLOB_FALL_BACK : NO_BLOB_FALL_BACK;
	    }
    }


    /*	Now do type size */
    
    /*	If the desired and current type size are the same, then exact match
    **	If the desired is larger than current, then value grading
    **	If the desired is smaller, then spacing fault and value grading
    */

   cfont_g->cfg_type_size = ABS(cfont_cur->cfa_type_size - cfont_des->cfa_type_size);
   if (cfont_cur->cfa_type_size> cfont_des->cfa_type_size) 
    {
	cfont_g->cfg_fault_flags |=  FLT_TYPE_SIZE;	 	
    }

    /*	Now do scale factor */

    
    /*	At this point we ignore the scale factor, for lack of guidelines
    **	on how to use it
    */

   cfont_g->cfg_scale_factor =	EXACT_MATCH;



    /*	Now do style */

    
    /*	If the desired and current styles are the same, exact match
    **	If a bit is set in current, but not in desired, then STYLE_FAULT
    **	If a bit is set in desired but not in current, ALG_ATT_FALL_BACK
    */

    /*  Preload the grade, with no need for algorithmic attributes
    */

    cfont_g->cfg_style    = EXACT_MATCH;

    
    if (cfont_cur->cfa_style != cfont_des->cfa_style)
    {

	/*  little explanation here.
	**  cfont_cur->cfa_style XOR cfont_des->cfa_style is the list of differences
	**  cfont_cur->cfa_style AND list of differences, is the list of bits set in 
	**  current, but not in desired, therefore the attributes that should be erased
	**  if there is any, this is a fault as we cannot erase.
	**
	*/
	if  (cfont_cur->cfa_style & (cfont_cur->cfa_style ^ cfont_des->cfa_style))

	{
	    cfont_g->cfg_fault_flags |= FLT_STYLE;

	}    
	if  (cfont_des->cfa_style & (cfont_cur->cfa_style ^ cfont_des->cfa_style))
	{
	    cfont_g->cfg_style = ALG_ATT_FALL_BACK;
	}    
    }

    /*	Now do weight*/

    


  /* Determine how closely the font file comes to the desired weight attribute.
   *
   *		   commanded bolding
   *
   *		   16		   25
   *   font         weight	    temp
   *  weight	   value           value
   *	 4	A-   26		A-   42
   *	 5	A-   24		A-   40
   *	 6	A-   22		A-   38
   *	 7	A-   20		A-   36
   *	 8	A-   18		A-   34
   *	 9	A-   16		A-   32
   *	10	A-   14		A-   30
   *	11	A-   12		A-   28
   *	12	A-   10		A-   26
   *	13	A-    8		A-   24
   *	14	A-    6		A-   22
   *	15	A-    4		A-   20
   *	16    best    0		A-   18
   *	17	A+    1		A-   16
   *	18	A+    3		A-   14
   *	19	A+    5		A-   12
   *	20	A+    7		A-   10
   *	21	A+    9		A-    8
   *	22	A+   11		A-    6
   *	23	A+   13		A-    4
   *	24	A+   15		A-    2
   *	25	A+   17		best  0
   *	26	A+   19		A+    1
   *	27	A+   21		A+    3
   *	28	A+   23		A+    5
   *	29	A+   25		A+    7
   *	30	A+   27		A+    9
   *	31	A+   29		A+   11
   *
   * A = accept; +/- indicates, for two weights equi-distant from a desired
   *				  weight, which should be taken as preferable.
   *
   * NOTE: If, when bolding is commanded, the selected font has a weight of
   *       less than 22 it will be algorithmically bolded.
   *
   * The value of weight is subtracted from MAX_WEIGHT_VALUE (63) so that the 
   * smallest weight value becomes the largest [i.e., most desireable] value and
   * accordingly aids in the over-all scoring of each font. 
   */
  if (cfont_cur->cfa_weight == cfont_des->cfa_weight)
    {
    	cfont_g->cfg_weight = EXACT_MATCH;
    }
    else
    {  
        if (cfont_des->cfa_style & BOLD)
	{   /* Bold desired */

            /* Select the font the weight of which is closest to '25',
             * according to the SPRM; but in the case where two fonts are equi-distant
             * from '25' it favors the more bold font. (The PSRM does not consider ties
             * with respect to '25'.) 
             */
	    if (cfont_cur->cfa_weight > BOLD_WEIGHT_CODE)
	    {   
		cfont_g->cfg_weight = ( (cfont_cur->cfa_weight - BOLD_WEIGHT_CODE) * 2 ) - 1;
	    }
            else
	    {
		cfont_g->cfg_weight = (BOLD_WEIGHT_CODE - cfont_cur->cfa_weight) * 2;
            }
	}
	else
	{   /* bolding is NOT requested */

	    /* Select the font the weight of which is closest to '17',
	    * according to the SPRM; it seems to me (nv) that this is a typo or a
	    * mistake (and should be '16') because it tends to favor bold fonts when
	    * the command is for non-bolded fonts. The following code, however, does
	    * what the PSRM calls for; but in the case where two fonts are equi-distant
	    * from '17' it favors the less bold font. (The PSRM does not consider ties
	    * with respect to '17'.) 
	    */
	    if (cfont_cur->cfa_weight > REGULAR_WEIGHT_CODE)
	    {
		cfont_g->cfg_weight = ( (cfont_cur->cfa_weight - REGULAR_WEIGHT_CODE) * 2 ) - 1;
	    }
	    else
	    {
		cfont_g->cfg_weight = (REGULAR_WEIGHT_CODE + 1 - cfont_cur->cfa_weight) * 2;
	    }

        } /* End else for if bold required */

    } /*End else for if weight are the same */


    /*	Now do proportion */

    /*	If the desired and current proportion are the same, then exact match
    **	If the desired is larger than current, then value grading
    **	If the desired is smaller, then proportion fault and value grading
    */

   cfont_g->cfg_proportion = ABS(cfont_cur->cfa_proportion - cfont_des->cfa_proportion);
   if (cfont_cur->cfa_proportion > cfont_des->cfa_proportion) 
    {
	cfont_g->cfg_fault_flags |=  FLT_PROPORTION;	 	
    }


    /*	Now do rotation */

    
    /*	At this point we ignore the rotation, fonts that are rotated are 
    **	thrown out 
    */

   cfont_g->cfg_rotation =   EXACT_MATCH;

} /* End grade_font subroutine */

/*****  cfont_comp_grade () *********************************************
 * Compare two grades							*
 ************************************************************************/
/* Returns TRUE if font 2 is better than font 1, FALSE othewise
*/
BOOL cfont_comp_grade (cfont_g_best, cfont_g)
PTR_CFG	cfont_g;
PTR_CFG	cfont_g_best;
{

    /*  First compare fault flags */
	/* If it rates worse in fault flags, forget it */
	/* if it rates better in fault flag, then it is better */
	/* if it rates equal in font flags, then the other items get counted */
   
	
    if	(cfont_g->cfg_fault_flags	>   cfont_g_best->cfg_fault_flags)
	{   return (FALSE); /*Worse, forget it */
	}
    if	(cfont_g->cfg_fault_flags	<   cfont_g_best->cfg_fault_flags)
	{   return (TRUE); /*Better, let's say it */
	}

   if	(cfont_g->cfg_type_family	>   cfont_g_best->cfg_type_family) 
	{   return (FALSE); /*Worse, forget it */
	}
   if	(cfont_g->cfg_type_family	<   cfont_g_best->cfg_type_family) 
	{   return (TRUE); /*Better, let's say it */
	}


    if	(cfont_g->cfg_cset		>   cfont_g_best->cfg_cset) 
	{   return (FALSE); /*Worse, forget it */
	}
    if	(cfont_g->cfg_cset		<   cfont_g_best->cfg_cset) 
	{   return (TRUE); /*Better, let's say it */
	}

    if	(cfont_g->cfg_type_size		>   cfont_g_best->cfg_type_size) 
	{   return (FALSE); /*Worse, forget it */
	}
    if	(cfont_g->cfg_type_size		<   cfont_g_best->cfg_type_size) 
	{   return (TRUE); /*Better, let's say it */
	}

    if	(cfont_g->cfg_spacing_criterion	>   cfont_g_best->cfg_spacing_criterion) 
	{   return (FALSE); /*Worse, forget it */
	}
    if	(cfont_g->cfg_spacing_criterion	<   cfont_g_best->cfg_spacing_criterion) 
	{   return (TRUE); /*Better, let's say it */
	}

    if	(cfont_g->cfg_spacing_value	>   cfont_g_best->cfg_spacing_value) 
	{   return (FALSE); /*Worse, forget it */
	}
    if	(cfont_g->cfg_spacing_value	<   cfont_g_best->cfg_spacing_value) 
	{   return (TRUE); /*Better, let's say it */
	}

    if	(cfont_g->cfg_scale_factor	>   cfont_g_best->cfg_scale_factor) 
	{   return (FALSE); /*Worse, forget it */
	}
    if	(cfont_g->cfg_scale_factor	<   cfont_g_best->cfg_scale_factor) 
	{   return (TRUE); /*Better, let's say it */
	}


    if	(cfont_g->cfg_style		>   cfont_g_best->cfg_style) 
	{   return (FALSE); /*Worse, forget it */
	}
    if	(cfont_g->cfg_style		<   cfont_g_best->cfg_style) 
	{   return (TRUE); /*Better, let's say it */
	}


    if	(cfont_g->cfg_weight		>   cfont_g_best->cfg_weight) 
	{   return (FALSE); /*Worse, forget it */
	}
    if	(cfont_g->cfg_weight		<   cfont_g_best->cfg_weight) 
	{   return (TRUE); /*Better, let's say it */
	}


    if	(cfont_g->cfg_proportion	>   cfont_g_best->cfg_proportion) 
	{   return (FALSE); /*Worse, forget it */
	}
    if	(cfont_g->cfg_proportion	<   cfont_g_best->cfg_proportion) 
	{   return (TRUE); /*Better, let's say it */
	}


    if	(cfont_g->cfg_rotation		>   cfont_g_best->cfg_rotation) 
	{   return (FALSE); /*Worse, forget it */
	}
    if	(cfont_g->cfg_rotation		<   cfont_g_best->cfg_rotation) 
	{   return (TRUE); /*Better, let's say it */
	}

    return (FALSE);	    

}

/*****  cfont_update_best () ********************************************
 * Update best font so far						*
 ************************************************************************/
VOID cfont_update_best (cfont_d_best, cfont_g_best, cfont_g, cur_font, cfont_att)
PTR_CFB	    cfont_d_best;
PTR_CFG	    cfont_g;
PTR_CFG	    cfont_g_best;
PTR_FDP	    cur_font;
PTR_CFA	    cfont_att;
{

    
    /* Better, update best so far */

    cfont_d_best->cfb_index		=   cur_font->fdp_font_index;
	
	
    cfont_d_best->cfb_scale.xval	=   1;
    cfont_d_best->cfb_scale.yval	=   1;

    cfont_d_best->cfb_pair_type		=   cur_font->fdp_paired_font_flag;
    cfont_d_best->cfb_spacing_crit	=   cfont_att->cfa_spacing_criterion;
    cfont_d_best->cfb_spacing_value	=   cfont_att->cfa_spacing_value;
    cfont_d_best->cfb_attr		=   cfont_att->cfa_style;
    if (cfont_att->cfa_weight > REGULAR_WEIGHT_CODE)
	{ cfont_d_best->cfb_attr |= BOLD; 
        }

    cfont_g_best->cfg_fault_flags	=   cfont_g->cfg_fault_flags;
    cfont_g_best->cfg_type_family	=   cfont_g->cfg_type_family; 
    cfont_g_best->cfg_cset		=   cfont_g->cfg_cset; 
    cfont_g_best->cfg_type_size		=   cfont_g->cfg_type_size; 
    cfont_g_best->cfg_spacing_criterion =   cfont_g->cfg_spacing_criterion;
    cfont_g_best->cfg_spacing_value	=   cfont_g->cfg_spacing_value; 
    cfont_g_best->cfg_scale_factor	=   cfont_g->cfg_scale_factor;
    cfont_g_best->cfg_style		=   cfont_g->cfg_style; 
    cfont_g_best->cfg_weight		=   cfont_g->cfg_weight; 
    cfont_g_best->cfg_proportion	=   cfont_g->cfg_proportion; 
    cfont_g_best->cfg_rotation		=   cfont_g->cfg_rotation; 

}

/*****  cfont_pairing () ************************************************
 * Setup pairing data							*
 ************************************************************************/

VOID cfont_pairing (g_table_entry, cfont_data_best_ptr)
WORD	    g_table_entry;
PTR_CFB	    cfont_data_best_ptr;
   {
    PUB	    byte_source;
    PTR_FN  font_number_source;
    WORD    i;
    FONT_NUMBER cfont_pair_left, cfont_pair_right;

    /*	Assume for now that we have pairing, ans set upp left and right accordingly */
    
    cfont_pair_right = cfont_data_best_ptr->cfb_index;
    cfont_pair_left =  cfont_data_best_ptr->cfb_pair_index;

    switch (cfont_data_best_ptr->cfb_pair_type)
       {
	case DEC_MCS_PAIR:  byte_source = nrcs_tables [0+cfont_data_best_ptr->cfb_nrc_type];
			    break;
	case ISO_LAT_PAIR:  byte_source = nrcs_tables [16+cfont_data_best_ptr->cfb_nrc_type];
			    break;
	case DEC_TEC_PAIR:  byte_source = nrcs_tables [32+cfont_data_best_ptr->cfb_nrc_type];
			    break;
	case DEC_PUB_PAIR:  byte_source = nrcs_tables [48+cfont_data_best_ptr->cfb_nrc_type];
			    break;
	case NO_DESIRED_PAIR:
	default:	    byte_source = nrcs_tables  [0];
    			    cfont_pair_left = cfont_data_best_ptr->cfb_index;
			    cfont_pair_right = cfont_data_best_ptr->cfb_index;
       }
		
    for (i = 0; i <= 127; i++)
       {
	xl_st.g_table[g_table_entry].gset_map[i].char_code = 
	   ((byte_source[i]) & CP_7BIT_MASK);
	xl_st.g_table[g_table_entry].gset_map[i].char_font = 
	   (((byte_source[i]) & BIT7) == 0)
	      ?	(cfont_pair_left)
	      :	(cfont_pair_right);
       }
   }


/*****  compute_font_for_g_set() ****************************************
 * Compute Font For G Set						*
 ************************************************************************/
VOID compute_font_for_g_set (g_table_entry)
WORD	g_table_entry;
{

/* These local variables are used to store the desired attributes of the font
*/

ORIENTATION	    cfont_des_orientation;	/*	Desired orientation */
CFONT_FNT_ATTR	    cfont_des;			/*	Structure containing the attributes
							of the desired font
						*/
WORD		    cfont_selection_type;	/*	selection_type, (by Type Family, Font_12, Font_16) */
PTR_FONT_ID	    cfont_requested_id;		/*	This points to the selection string of the current SGR
							in case of a selection by type family, it points to the 
							same 7 characters as cfont_type_family
							in case of a selection by font, it points to 16 characters
							(same 7 plus 9 additional selection characters)  
							in case of a selection by xxx, it points to 12 characters
							(same 7 plus 5 additional selection characters)
					        */

FONT_ID		    cfont_perfect_id;		/*	What would the ID of the perfect font be */
UBYTE		    desired_pair;		/*	What pairs can we used to make CSET */
WORD		    MCS_pairing_table_index;	/*	What is the index in the DEC MCS pairing table */
WORD		    ISO_pairing_table_index;	/*	What is the index in the ISO MCS pairing table */
WORD		    TEC_pairing_table_index;	/*	What is the index in the DEC TEChnical pairing table */
WORD		    PUB_pairing_table_index;	/*	What is the index in the DEC PUBlishing pairing table */

/* These local variables are used to store the attributes of the font under
** consideration
*/



CFONT_FNT_ATTR cfont_cur;		/*	Structure contining the attributes of the font under consideration
					*/


/* These local variables are used to store the grades of the font under
** consideration.
*/

CFONT_GRADE_STRUCT cfont_grade;			/*	structure containing the grade of the font currently 
							under considereration
						*/


/*  These local variables are used to dialogue with the font dictionary
*/

FONT_DICT_PARAM	current_font;			/*	Structure used by Dictionary to return
							parameters
						*/

/*  These variables are used to memorize the best font
*/

CFONT_GRADE_STRUCT cfont_grade_best;		/*	structure contining the grade of the best font 
							so far 
						*/
CFONT_BEST_STRUCT   cfont_data_best;		/*	This structure memorizes the necessary information,
							fallbacks and adjustments required
							to use the best font 
						 */


/*  The first step is to gather the selection criteria, to define which font we
**  want to use
**
**	    orientation
**	    type_family
**	    spacing 
**	    type_size 
**	    scale_factor 
**	    style
**	    weight
**	    proportion
**	    rotation 
**	    cset
**
*/


/*  First, point to the currently selected FONT SGR.
*/

cfont_requested_id = (PTR_FONT_ID)(xl_st.sgr_tbl[xl_st.cur_sgr].id_string);
cfont_selection_type = xl_st.sgr_tbl[xl_st.cur_sgr].selection_type;

/*  Find the current orientation
*/

cfont_des_orientation = xl_st.orientation;



/*  Gather the Type family
**  by making cfont_des.cfa_type_family point to the id_string of the selected sgr
*/



strncpy (cfont_des.cfa_type_family, cfont_requested_id, FIDS_TYPE_FAMILY);


/*  Find the proper kind of spacing
*/
/* If assign by type family, then get it from xl_st, else get it from SGR */


/* If the translator is in proportional mode as opposed to monospaced
 * mode, we do not have a critrion for evaluating font files based upon their
 * spacing attributes. Such a critrion is synthesized below basically by
 * multiplying commanded character proportion (~=type_width / type_height),
 * from GSS/GSM, by commanded type size (=type_height). Because a normal
 * character_proportion, expressed as 100%, (as we define it within this
 * module) refers to a 
 *     width : size (=height) ratio = 720 centipoint / 1000 centipoint
 * xl_st .character_proportion needs to be multiplied by 
 *     (720 centipoint / 1000 centipoint) / 100[%]
 * to yield a true width : size (=height) ratio. Therefore the proposed
 * spacing critrion may be synthesized as follows: 
 */

if  (cfont_selection_type == SGR_TYPE_FAMILY)
    {
	cfont_des.cfa_spacing_criterion = xl_st.propl_mode;

	cfont_des.cfa_spacing_value =   (cfont_des.cfa_spacing_criterion == MONO_SPACED_MODE) ?
	                                xl_st .hai :
	 				(   ( ((LONG)xl_st .v_size)
					    * ((LONG) xl_st .character_proportion)
					    * 72L
					    )/ 10000L 
					);
    }
    else
    {

	cfont_spacing	(cfont_requested_id, 
			 &cfont_des.cfa_spacing_criterion, 
			 &cfont_des.cfa_spacing_value
			);
    }


/*  Find the desired type_size 
*/

/* If assign by type family, then get it from xl_st_v_size, else get it from SGR */

if  (cfont_selection_type == SGR_TYPE_FAMILY)
    {

	cfont_des.cfa_type_size = xl_st.v_size;

    }
    else
    {
	cfont_des.cfa_type_size = cfont_type_size (cfont_requested_id);	
    }

/* Find desired Scale factor
*/
/* If assign by type family, then use 1, else get it from SGR */

if  (cfont_selection_type == SGR_TYPE_FAMILY)
    {

	/*TEST CODE */
	/* For now, scale factor is 1, need to deal with float values */

	cfont_des.cfa_scale_factor = 1;

	/*  cfont_des.cfa_scale_factor = SCALE_FACTOR_1;*/

	/* END TEST CODE */

    }
    else
    {
	cfont_des.cfa_scale_factor = cfont_scale_factor (cfont_requested_id);
    }


/* Find desired Style
*/

/*  Gather the desired attributes
**
**  If the SGR is a select by type family (7 chars), or by xxx (12 chars)
**  get the attributes from the attribute selection (xl_st.requested_attributes)
**
**  Else, this is a select by font, get the attribute selection from the font_id
*/
if  (cfont_selection_type == SGR_FONT_16)
    {
	cfont_des.cfa_style = cfont_style (cfont_requested_id);
	if (cfont_weight (cfont_requested_id) == BOLD_WEIGHT_CODE)
	{
	    cfont_des.cfa_style |= BOLD;
	}
    }    
    else
    {
	cfont_des.cfa_style = xl_st.requested_attributes;
    }

/*
 * Find desired Weight
 * 
 * If the selection is by FONT_16, then get the weight from the SGR
 * else get it from XL_ST
 */

if  (cfont_selection_type == SGR_FONT_16)
    {
	cfont_des.cfa_weight = cfont_weight (cfont_requested_id);
    }
    else
    {
	cfont_des.cfa_weight = (xl_st.requested_attributes & BOLD) ?
		    BOLD_WEIGHT_CODE : REGULAR_WEIGHT_CODE; 
    }

/* Find desired Proportion
 * 
 * If the selection is by FONT_16, then get the proportion from the SGR
 * else get it from XL_ST
 */

if  (cfont_selection_type == SGR_FONT_16)
    {
	cfont_des.cfa_proportion = cfont_proportion(cfont_requested_id);
    }
    else
    {
	cfont_des.cfa_proportion = xl_st.character_proportion;
    }

/*Find desired rotation 
*/

cfont_des.cfa_rotation = ROTATION_0;


/*  Gather the selection criteria for the character set 
**  This entails:
**
**	The character set ID
**	Whether or not the character set is an NRCS that  can be obtained from a paired font
**
**  The character set id may be obtained directly, or if we are dealing with User Preference CSET, 
**  must be de-referenced.
*/




/*
 * Get a local copy of the character set desired for this g_table_entry.
 */

	/* Point to selected CSET */

cfont_des.cfa_cset	= xl_st .g_table [g_table_entry].char_set_id ;

	/*  If this is the User Preference set, go through 
	    an additional level of indirection
	*/	

if (cfont_des.cfa_cset  == USER_PREFERENCE)
  {
  cfont_des.cfa_cset = xl_st .user_pref_cset;
  };




/* Decide whether the character set can be obtained from a paired font
*/
    /* First is this set out of the 94 repertory, if not this is not an NRC */
    
    if (!(cfont_des.cfa_cset & SET_96_BIT))
    {
	/* If so, is it in the NRC list */
	/*  At this time, this code assumes that there is only 
	    one supported pairing mechanism for each NRC,
	    i.e. French can only be obtained from DEC_MCS,
	    but not ISO_LATIN_#1, if this were to change, 
	    this code, as well as the grading of fonts on CSET
	    as well as the final setup performed by compute_font_for_gset
	    need to be modified
	*/
    desired_pair = DEC_MCS_PAIR;
    MCS_pairing_table_index = 3;
    switch (cfont_des.cfa_cset)
      {
      case FRENCH                   : MCS_pairing_table_index++; /*16 */
      case DEC_DUTCH                : MCS_pairing_table_index++; /*15 */
    /*
     * nrcs above this comment are supported differently by DEC and ISO SUPPLEMENTAL
     */
      case ISO_NORWEGIAN_DANISH     :
      case DEC_NORWEGIAN_DANISH     : MCS_pairing_table_index++; /*14 */
      case NORWEGIAN_DANISH         : MCS_pairing_table_index++; /*13 */
      case ISO_SWEDISH              :
      case DEC_SWEDISH              : MCS_pairing_table_index++; /*12 */
      case DEC_PORTUGESE            : MCS_pairing_table_index++; /*11 */
      case SPANISH                  : MCS_pairing_table_index++; /*10 */
      case DEC_FRENCH_CANADIAN      : MCS_pairing_table_index++; /* 9 */
      case GERMAN                   : MCS_pairing_table_index++; /* 8 */
      case ITALIAN                  : MCS_pairing_table_index++; /* 7 */
      case JIS_ROMAN                : MCS_pairing_table_index++; /* 6 */
      case DEC_SWISS                : MCS_pairing_table_index++; /* 5 */
      case ISO_FINNISH              :
      case DEC_FINNISH              : MCS_pairing_table_index++; /* 4 */
      case UNITED_KINGDOM           : break;  /* 3 */
    
      case DEC_SUPPLEMENTAL         :
      case DEC_TECHNICAL            :
      case ASCII                    :
      case DEC_VT100                :
      default                       : MCS_pairing_table_index = 0;
				      desired_pair = NO_DESIRED_PAIR;
                                      break;
  }
} /* end if C94 repertory */


/* We now have gathered the selection Criteria */

/* Let's build the ID of the perfect font */

cfont_id_type_family (& cfont_perfect_id, cfont_des.cfa_type_family);
cfont_id_spacing (& cfont_perfect_id, cfont_des.cfa_spacing_criterion, cfont_des.cfa_spacing_value);
cfont_id_type_size (& cfont_perfect_id, cfont_des.cfa_type_size);
cfont_id_scale_factor (& cfont_perfect_id, cfont_des.cfa_scale_factor);
cfont_id_attributes (& cfont_perfect_id, cfont_des.cfa_style);
cfont_id_weight (& cfont_perfect_id, cfont_des.cfa_weight);
cfont_id_proportion (& cfont_perfect_id, cfont_des.cfa_proportion);
cfont_id_rotation (& cfont_perfect_id, cfont_des.cfa_rotation);
cfont_id_cset (& cfont_perfect_id, cfont_des.cfa_cset);
cfont_id_csubset (& cfont_perfect_id, subset_zzzz);
cfont_id_encoding (& cfont_perfect_id);
cfont_id_resolution (& cfont_perfect_id, RESOLUTION_300);
cfont_id_reserved (& cfont_perfect_id);

/* Let's initialize the dialogue with the Font Dictionary */

font_get_default (cfont_des_orientation, & cfont_perfect_id, & current_font);

/* Let's grade the default font returned */

cfont_explode_attributes (&cfont_cur, &current_font.fdp_font_id);
    
cfont_grade_font (&cfont_cur, &cfont_des, &cfont_grade, desired_pair, &current_font);

/* Initialize best so far to the default returned */

cfont_update_best ( &cfont_data_best, &cfont_grade_best, &cfont_grade, &current_font, &cfont_cur);





while (font_get_next(& current_font), current_font.fdp_font_index != END_OF_DICTIONARY)

{
    
    
    /* if by magic we got the perfect one, quit */

    if (!(strncmp (& current_font.fdp_font_id, & cfont_perfect_id, FIDS_FONT_ID)))
    /*	Little Trivia, STRNCMP returns false if the strings are equal */

    {

    /*	The following lines are there to ensure that cfont_data_best gets filled
     *	properly.
     *	In a future optimization pass, these calls could be cut down to the minimum
     *  necessary to fill the structure, knowing that everything is perfect.
     */

    cfont_explode_attributes (& cfont_cur,& current_font.fdp_font_id);
    
    cfont_grade_font (& cfont_cur,& cfont_des,& cfont_grade, desired_pair, & current_font);

    cfont_update_best ( &cfont_data_best, &cfont_grade_best, &cfont_grade, &current_font, &cfont_cur);

    break;
    }        

    cfont_explode_attributes (& cfont_cur,& current_font.fdp_font_id);
    
    cfont_grade_font (& cfont_cur,& cfont_des,& cfont_grade, desired_pair, & current_font);


    /* Compare best grade so far with current font */
    if (cfont_comp_grade (&cfont_grade_best, &cfont_grade))
    {
	cfont_update_best ( &cfont_data_best, &cfont_grade_best, &cfont_grade, &current_font, &cfont_cur);
    }

}



/*  Got best font for the job 
 */

/*  Tell font dictionary to cache font 
 *  and get the font_number for the paired font
 *  if any
 */

/* First find out if we decided to use pairing or not
 */

    if	(cfont_grade_best.cfg_cset != PAIRING_FALL_BACK)
	{
	    cfont_data_best.cfb_pair_type = NO_DESIRED_PAIR;
	}



    cfont_data_best.cfb_pair_index = font_select (cfont_data_best.cfb_index, 
                                                  &cfont_data_best.cfb_scale, 
                                                  cfont_data_best.cfb_pair_type, 
                                                  g_table_entry);


/*  Cache Gtable */
/*  Algorithmic attributes */
/*  NRC tables */
/*  Algorithmic spacing */
/*  Width table	*/
/*  Above and below baseline values */
/*
 * The g_table is updated according to the font table information for the
 * best font file found ( = preferred_index).
 *
 * The algorithmic_attributes = the requested_attributes which are not 
 * already part of the best font file.
 */



xl_st .g_table [g_table_entry] .gset_fontdata .algorithmic_attributes = 
    (~ (cfont_data_best.cfb_attr )) &  (cfont_des.cfa_style);
    

/*
 * The ps_font_number = the best font file's ps_font_number 
 */

/*xl_st .g_table [g_table_entry] .gset_map .ps_font_number = 
**			cfont_data_best.cfb_index;
*/
/*
 * The horizontal_spacing is determined by the horiz_spacing function below.
 */


  xl_st .g_table [g_table_entry] .gset_fontdata .horizontal_spacing =
    cfont_horiz_spacing (&cfont_data_best);

 xl_st .g_table [g_table_entry] .gset_fontdata . above_baseline_offset = 
    FNT_ABOVE_BASELINE(cfont_data_best.cfb_index);

 xl_st .g_table [g_table_entry] .gset_fontdata . below_baseline_offset = 
    FNT_BELOW_BASELINE(cfont_data_best.cfb_index);


    /* Now deal with NRC stuff */

    switch (cfont_data_best.cfb_pair_type)
    {
	case DEC_MCS_PAIR:  cfont_data_best.cfb_nrc_type = MCS_pairing_table_index;
			    break;
	case ISO_LAT_PAIR:  cfont_data_best.cfb_nrc_type = ISO_pairing_table_index;
			    break;
	case DEC_TEC_PAIR:  cfont_data_best.cfb_nrc_type = TEC_pairing_table_index;
			    break;
	case DEC_PUB_PAIR:  cfont_data_best.cfb_nrc_type = PUB_pairing_table_index;
			    break;
	case NO_DESIRED_PAIR:
	default:	    cfont_data_best.cfb_nrc_type = 0;
    }
		


    cfont_pairing (g_table_entry, &cfont_data_best);

    /* if we got a CSET fault, (can't find cset), then use blob.
     * Note, in a future optimization pass, this should be 
     * mutually exclusive with the above pairing code, 
     * provided we can ensure that Blob performs all the required functions
     */

    if 	(cfont_grade_best.cfg_fault_flags &  FLT_CSET)	 	
	{
	    if	(cfont_grade_best.cfg_cset == BLOB_FALL_BACK)
		{
		    cfont_blob (g_table_entry, cfont_data_best.cfb_index);
		}
		else
		{
		    cfont_space (g_table_entry, cfont_data_best.cfb_index);
		}
	}


    cfont_cache_widths(g_table_entry);

    xl_st.g_table[g_table_entry].gset_valid = TRUE;

}  /* end of compute_font_for_g_set */


VOID cfont_cache_widths(g_table_entry)
WORD	g_table_entry;
{
WORD i;

for (i=0;i<=127;i++)
    {
    xl_st.g_table[g_table_entry].gset_map[i].char_width=
	get_width(
	    (xl_st.g_table[g_table_entry].gset_map[i].char_code),
	    (xl_st.g_table[g_table_entry].gset_map[i].char_font)
		 );
    };
}





LONG cfont_horiz_spacing (cfont_data_best_ptr)
PTR_CFB cfont_data_best_ptr;	/* ptr to structure containing among others :		*/
				/* font_table index  of selected font			*/
				/* spacing value of selected font (if MONO)		*/
				/* spacing criterion of selected font   (MONO vs PROP)  */
  

/* This function returns the horizontal spacing which is to be associated with 
 * a G set's glyph's. A zero value means that the font's width table satisfies
 * the commanded spacing requirements; if the font's width table does not
 * satisfy the commanded spacing requirements, then a non-zero value is
 * returned which indicates the corrected value to use for spacing. 
 */

{
/* If what is commanded is a mono-spaced mode with a spacing = xl_st .hai but 
 * the font, indicated by the font table index i, is either proportionally
 * spaced (font_table [i] .spacing = 0) or mono-spaced (font_table [i] .spacing
 * = non-zero spacing value) but not equal to xl_st .hai, then use (i.e.,
 * return) xl_st .hai. 
 * If what is commanded is a semi-proportional mode but the font, indicated by
 * the font table index i, is proporttionally spaced then use (i.e., return)
 * the font's average width. 
 * In all other situations return a value of zero signifying that the font's
 * width table satisfies the commanded spacing requirements.
 *
 *
 *                      |        commanded proportional spacing:
 *                      |
 *                      |   MONO_SPACED_                   SEMI_
 *                      |   MODE           PROPORTIONAL_   PROPORTIONAL_
 *      font file's     |   [xl_st .hai]   MODE            MODE
 *        spacing:      |
 *                      |------------------------------------------------
 *      == xl_st .hai   |        0             0              0  
 *                      |
 *      != xl_st .hai   |    xl_st .hai        0              0
 *                      |
 *      == proportional |    xl_st .hai        0         average_width
 *                      |
 */


if (  (xl_st .propl_mode == MONO_SPACED_MODE)
      &&
      (xl_st .hai != cfont_data_best_ptr->cfb_spacing_value)
    )
  return (xl_st .hai);

else if ( (xl_st .propl_mode == SEMI_PROPORTIONAL_MODE)
          &&
	  (cfont_data_best_ptr->cfb_spacing_crit == PROPORTIONAL_MODE)
	)
  return ( FNT_AVERAGE_WIDTH(cfont_data_best_ptr->cfb_index));

else
  return (0);

}


/*****  init_g_table() **************************************************
 * Init G Table								*
 ************************************************************************/

VOID init_g_table ()
   {
    WORD i;

    /* 
     * All gsets have the same repertory, C94
     */
    for (i=0; i<G_TABLE_SIZE; i++)
	xl_st .g_table [i].repertory = C94;

    /*
     * Init the char_set_id fields individually
     */
    xl_st .g_table [0].char_set_id =
	xl_st .g_table [1].char_set_id = ASCII;
    xl_st .g_table [2].char_set_id =
	xl_st .g_table [3].char_set_id = USER_PREFERENCE;
   }

