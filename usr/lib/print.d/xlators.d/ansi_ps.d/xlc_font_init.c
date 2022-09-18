#ifndef lint
static char SccsId[] = "  @(#)xlc_font_init.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/************************************************************************
 *                                                                      *
 *      COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,            *
 *      1986, 1989.   ALL RIGHTS RESERVED.                              *
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
#include "xlc_font_dictionary.hc"
#include "xlc_font_init.hc"

/*****  font_init()  ****************************************************
 * Do any initialization required to set up for font processing.	*
 ************************************************************************/
VOID font_init ()
{
WORD i,j;

for (i = 0; i < FIRST_DOWNLOADED_FONT; i++)
  {
  vax_font_table [i] .vax_active = ACTIVE;
  strcpy (&vax_font_table [i] .font_file_id, 
          &builtin_metrics[i].bms_font_file_id [0]);

  vax_font_table [i] .opposite_entry = UNUSED;

  vax_font_table_box_number  [i] = builtin_metrics[i].bms_box;

  vax_font_table_mapping_index [i] = builtin_metrics[i].bms_mapping_index;

  vax_font_table [i] .character_weight = 16; /*  ??????? */

  vax_font_table [i] .proportion_numerator= 
		builtin_metrics[i].bms_prop_numerator;

  vax_font_table [i] .proportion_denominator = 100;

  switch (builtin_metrics[i].bms_font_size)
    {
    case (PT_10) :
      vax_font_table [i] .above_baseline = -840 ;
      vax_font_table [i] .total_height = 1152;
      vax_font_table [i] .superscript_height = -576;
      vax_font_table [i] .subscript_height = 576;
      break ;    

    case (PT_6_7) :
      vax_font_table [i] .above_baseline = -648 ;
      vax_font_table [i] .total_height = 864;
      vax_font_table [i] .superscript_height = -432;
      vax_font_table [i] .subscript_height = 432;
      break ;    
    } ;

      vax_font_table [i] .average_width = builtin_metrics[i].bms_font_spacing;
      vax_font_table [i] .vmin_table = builtin_metrics[i].bms_min;
      vax_font_table [i] .vmax_table = builtin_metrics[i].bms_max;

    superscript_height_table [i] = vax_font_table [i] .superscript_height;
    subscript_height_table [i] = vax_font_table [i] .subscript_height;
    height_table [i] = vax_font_table [i] .total_height;
    above_baseline_table [i] = vax_font_table [i] .above_baseline;
    below_baseline_table [i] = height_table [i] + above_baseline_table [i];
    average_width_table [i] = vax_font_table [i] .average_width;
    min_table [i] = vax_font_table [i] .vmin_table;
    max_table [i] = vax_font_table [i] .vmax_table;

    for (j=0; j<32 ; j++ )
	{
	width_table [i] [j] = 0 ;
	left_bearing_table [i] [j] = 0 ;
	right_bearing_table [i] [j] = 0 ;
	} ;
   
    width_table [i] [VIR_CHAR_BLOB] = builtin_metrics[i].bms_font_spacing ;

    for (j=32; j< 127 ; j++ )
	{
	width_table [i] [j] = builtin_metrics[i].bms_font_spacing ;
	left_bearing_table [i] [j] = 0 ;
	right_bearing_table [i] [j] = 0 ;
	} ;

    width_table [i] [127] = 0 ;
    left_bearing_table [i] [127] = 0 ;
    right_bearing_table [i] [127] = 0 ;

  }


for (i = FIRST_DOWNLOADED_FONT; i < VAX_FONT_TABLE_SIZE; i++)
  {
  vax_font_table [i] .opposite_entry = UNUSED;
  vax_font_table [i] .font_file_id.fi_type_family[0] = 0;
  vax_font_table [i] .vax_active = INACTIVE;
  vax_font_table_box_number  [i] = END_OF_DICTIONARY;
  vax_font_table [i] .character_weight = 0;
  vax_font_table [i] .proportion_numerator= 0;
  vax_font_table [i] .proportion_denominator= 0;
  vax_font_table [i] .above_baseline = 0;
  vax_font_table [i] .total_height = 0;
  vax_font_table [i] .superscript_height = 0;
  vax_font_table [i] .subscript_height = 0;
  vax_font_table [i] .average_width = 0;
  vax_font_table [i] .vmin_table = 0;
  vax_font_table [i] .vmax_table = 0;

    superscript_height_table [i] = 0 ;
    subscript_height_table [i] = 0 ;
    height_table [i] = 0 ;
    above_baseline_table [i] = 0 ;
    below_baseline_table [i] = 0 ;
    average_width_table [i] = 0 ;
    min_table [i] = 0 ;
    max_table [i] = 0 ;

    for (j=0; j< NUMBER_OF_7_BIT_CHARACTERS; j++ )
	{
	width_table [i] [j] = 0 ;
	left_bearing_table [i] [j] = 0 ;
	right_bearing_table [i] [j] = 0 ;
	} ;
  }


for (i = 0; i < VAX_FONT_TABLE_SIZE; i++)
  {
  vax_font_table [i] .space_width = 0;
  vax_font_table [i] .ul_thickness = 0;
  vax_font_table [i] .ul_offset = 0;
  vax_font_table [i] .ol_thickness = 0;
  vax_font_table [i] .ol_offset = 0;
  vax_font_table [i] .strikethru_thickness = 0;
  vax_font_table [i] .strikethru_offset = 0;
  vax_font_table [i] .virtual_memory_needs = 0;
  vax_font_table [i] .bitmaps_ptr = 0;

  }

init_paired_char_set_table ();

font_pairs();

}  /* end of font_init */

/*****  init_paired_char_set_table() ************************************
 * Init Paired Char Set Table						*
 ************************************************************************/
init_paired_char_set_table ()
{
WORD	i;	/* for-loop index */
WORD	j;	/* for-loop index */
WORD	paired_charset;
for ( i = 0; i < FIRST_DOWNLOADED_FONT; )
  {
  vax_font_table [i++] .opposite_entry = j + 2;

  vax_font_table [i++] .opposite_entry = j + 3;

  vax_font_table [i++] .opposite_entry = j;

  vax_font_table [i++] .opposite_entry = j + 1;

if (i < 24)
    {
      vax_font_table [i++] .opposite_entry = j;

      vax_font_table [i++] .opposite_entry = j + 1;
    } ;


  }
for (i = FIRST_DOWNLOADED_FONT; i < VAX_FONT_TABLE_SIZE; )
  {
  vax_font_table [i] .vax_active = INACTIVE;
  vax_font_table [i++] .opposite_entry = UNUSED;
  }
}  /* end of init_paired_char_set_table */
