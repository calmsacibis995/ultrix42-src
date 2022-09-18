#ifndef lint
static char SccsId[] = "  @(#)xlc_font_dictionary.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: xlc_font_dictionary.c
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
 *   8-DEC-1988 16:16 araj
 *	Font_calc_overrides was not computing the max, but was using the last value.
 *	Removed the "first_occurence" code, and initialized the max_xxx to 0 instead
 *
 *   8-DEC-1988 17:20 araj
 *	Modified to invalidate cache entry when a font gets deleted
 *
 *  15-DEC-1988 09:32 ejs
 *	Above baseline is minimized, not maximized.
 *
 *  28-DEC-1988 10:17 mhw
 *	Free buffer of the font passed in font_dispose_of_font.  Also free
 *	if no entry available in the vax_font_table.  Use cfree because the
 *	buffer was allocated using fnt_get_mem which uses calloc.
 *
 *  22-FEB-1989 14:32 araj
 *	Superscript_height is a negative number, need to use MIN to get largest
 *	value
 *
 *  20-MAR-1989 cp
 *	Added cast operators for Ultrix port.
 *
 *  26-APR-1989 18:15	araj
 *	Added font too big test, if we ask for globs of memory, PMAX gives it
 *	to us, but we end up crashing everything
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
 *   Filename:  font_dictionary.c
 *
 *	These routines are informational.  They do not modify anything.
 *
 *   end description
 *
 *-----------------------------------------------------------------
 */




#include    "portab.h"
#include    "capdl.hc"
#include    "dbug.h"
#include    "xlate.h"

#include    "xlc_font_dictionary.hc"

PUB font_get_mem(ptr_amt_allocated)
PL ptr_amt_allocated;

{
    PUB	buffer;

    buffer = (PUB) calloc( 1, *ptr_amt_allocated) ;

    if ( buffer == 0 )
	*ptr_amt_allocated = 0 ;

    return (buffer) ;
}

VOID font_dispose_of_font (buffer_ptr, buffer_size, dll_valid_load, font_state)
PUB buffer_ptr;
LONG buffer_size;
BOOLEAN dll_valid_load;
PTR_DLL_FONT_STATE font_state;
{
UBYTE	*resized_buffer ;
LONG	vm_needs_of_font ;
FONT_NUMBER i ;

/* 
**  Check if something has gone wrong, free memory if it has.
*/
if ( !dll_valid_load )
    {
    cfree(buffer_ptr) ;
    return ;
    } ;

/* 
**  Memory work: Calc the vm_needs and allocate a new buffer.
*/
vm_needs_of_font =
	  font_state->total_portrait_bytes
	- font_state->number_of_odd_words_in_bitmaps
	+ FONT_VM_OVERHEAD;
if (vm_needs_of_font > FONT_VM_MAXIMUM) 
    {
    cfree(buffer_ptr) ;  /* Free the buffer */
    ABORT_MACRO(15) ;	/* Font too big (>128000) */
    return ;
    } ;


if (( resized_buffer = (PUB) malloc(vm_needs_of_font - FONT_VM_OVERHEAD) ) == 0 )
    {
    cfree(buffer_ptr) ;  /* Free the buffer */
    ABORT_MACRO(14) ;	/* Not enough memory in the host */
    return ;
    } ;

/* 
**  Delete a previous downloaded version of the new file
*/
font_delete_font_files( FIDS_FONT_ID,
		        font_state->new_font_file_id ) ;



/*
**  Determine if any entry in the Font Table is available. 
**  If there is no entry available, release the memory and report an error.
*/
for ( i = FIRST_DOWNLOADED_FONT; i < VAX_FONT_TABLE_SIZE; i++ )
    {
    if (vax_font_table[i].vax_active == INACTIVE)
	{
	break;
	} ;
    } ;

if ( i == VAX_FONT_TABLE_SIZE )
    {
    cfree (buffer_ptr);
    free (resized_buffer);
    ABORT_MACRO(12);  
    return ;
    } ;

update_tables(i, font_state, resized_buffer, vm_needs_of_font);

font_calc_overrides(&vax_font_table[i].font_file_id);

/* pair the fonts here */
font_pairs();

/* free the buffer passed to this routine */
cfree(buffer_ptr) ;

return ;
}

VOID font_delete_font_files(delete_mode, font_ptr)
UWORD delete_mode;
AUB font_ptr;

{
UWORD	i ;

    for (i=FIRST_DOWNLOADED_FONT; i< VAX_FONT_TABLE_SIZE; i++)
	{
	if (  (vax_font_table[i].vax_active==ACTIVE)
	    &&(!strncmp(font_ptr,&vax_font_table[i].font_file_id,delete_mode)))
	    {

	    dispose_delete_font_from_cache(i);

	    vax_font_table[i].vax_active= INACTIVE ;

	    font_calc_overrides(&vax_font_table[i].font_file_id);

	    if (i >= FIRST_DOWNLOADED_FONT )
		{
		free (vax_font_table[i].bitmaps_ptr) ;
		} ;
	    } ;
	} ;
font_pairs() ;
}


VOID update_tables(new_entry, font_state, resized_buffer, vm_needs_of_font)
FONT_NUMBER new_entry ;
PTR_DLL_FONT_STATE font_state;
UBYTE	*resized_buffer ;
LONG	vm_needs_of_font ;

{
WORD	i, j ;
WORD	font_char_entry ;
LONG	number_of_bytes ;


/*
**  Fill all the vax_font_table fields.
*/

vax_font_table [new_entry] .vax_active = ACTIVE;

strcpy (&vax_font_table [new_entry] .font_file_id,
        font_state->new_font_file_id);

vax_font_table [new_entry] .character_weight =
				    font_state->dll_font_weight;
vax_font_table [new_entry] .proportion_numerator = 
				    font_state->dll_horiz_prop_numerator;
vax_font_table [new_entry] .proportion_denominator = 
				    font_state->dll_horiz_prop_denominator ;


vax_font_table_box_number  [new_entry] = END_OF_DICTIONARY;

vax_font_table_mapping_index [new_entry] = MAP_LEFT ;

for (i = 0; i < PAIRING_OPTIONS_COUNT; i++)
    {
    if (!strncmp(vax_font_table[new_entry].font_file_id.fi_cset,
	pairing_table[i].source_cset,
	FIDS_CSET))
        {
        vax_font_table_mapping_index [new_entry] = MAP_RIGHT ;
	};
    };

vax_font_table [new_entry] .opposite_entry = 
				    UNUSED;


vax_font_table [new_entry] .space_width =
				    font_state->dll_width_tbl[MAX_CSET_SIZE];
vax_font_table [new_entry] .ul_thickness =
				    font_state->dll_ul_thickness;
vax_font_table [new_entry] .ul_offset =
				    font_state->dll_ul_offset;
vax_font_table [new_entry] .ol_thickness =
				    font_state->dll_ol_thickness;
vax_font_table [new_entry] .ol_offset =
				    font_state->dll_ol_offset;
vax_font_table [new_entry] .strikethru_thickness =
				    font_state->dll_strikethru_thickness;
vax_font_table [new_entry] .strikethru_offset =
				    font_state->dll_strikethru_offset;

vax_font_table [new_entry] .above_baseline =
				    font_state->dll_above_baseline_dist;
vax_font_table [new_entry] .total_height =
				    font_state->dll_total_vertical_size;
vax_font_table [new_entry] .superscript_height =
				    font_state->dll_superscript_vert;
vax_font_table [new_entry] .subscript_height =
				    font_state->dll_subscript_vert;
vax_font_table [new_entry] .average_width =
				    font_state->dll_average_char_width;
vax_font_table [new_entry] .vmin_table =
				    font_state->dll_space_min;
vax_font_table [new_entry] .vmax_table =
				    font_state->dll_space_max;

vax_font_table [new_entry] .virtual_memory_needs =
				    vm_needs_of_font;
vax_font_table [new_entry] .bitmaps_ptr = 
				    (BYTE *) resized_buffer ;


/*
**  Fill all the override metric tables.
*/

above_baseline_table [new_entry] =
				font_state->dll_above_baseline_dist;
height_table [new_entry] =
				font_state->dll_total_vertical_size;
below_baseline_table [new_entry] =
				height_table [new_entry] +
				above_baseline_table [new_entry];
superscript_height_table [new_entry] =
				font_state->dll_superscript_vert;
subscript_height_table [new_entry] =
				font_state->dll_subscript_vert;
average_width_table [new_entry] =
				font_state->dll_average_char_width;
min_table [new_entry] =
				font_state->dll_space_min;
max_table [new_entry] =
				font_state->dll_space_max;



/* 
**  Update the character metrics.  The tables are offset as the fixed fonts
**  do not have metrics.  'font_char_entry' is used to adjust for the offset.
**
**  The blob character is passed as the last character but it's metrics are 
**  remapped to the SUB character.
**
**  If a character has a subtitute flag less than MAX_CSET_SIZE then it is 
**  remapped.
*/

font_char_entry = new_entry - FIRST_DOWNLOADED_FONT;

for (i = 0; i < MAX_CSET_SIZE - 1; i++)
    {
    int ii = i ;

    if ( font_state->dll_char [i] .substitute_flag < MAX_CSET_SIZE)
	{
	ii = font_state->dll_char [i] .substitute_flag ;
	} ;

    width_table [new_entry] [i+32] = 
			font_state->dll_width_tbl [ii];
    left_bearing_table [new_entry] [i+32] = 
			font_state->dll_l_bearing_tbl [ii];
    right_bearing_table [new_entry] [i+32] = 
			font_state->dll_r_bearing_tbl [ii];
    }

width_table [new_entry] [VIR_CHAR_BLOB] =
			font_state->dll_width_tbl [MAX_CSET_SIZE - 1];
left_bearing_table [new_entry] [VIR_CHAR_BLOB] =
			font_state->dll_l_bearing_tbl [MAX_CSET_SIZE - 1];
right_bearing_table [new_entry] [VIR_CHAR_BLOB] =
			font_state->dll_r_bearing_tbl [MAX_CSET_SIZE - 1];



/*
**  Copy the glyph metrics and the glyph data.
**  Note the substitution is not done, as the font_bitmap info will never
**  be referenced.
*/

for (i = 0; i < MAX_CSET_SIZE; i++)
    {

    font_bitmap [font_char_entry] [i] .baseline = 
			font_state->dll_char [i] .baseline;
    font_bitmap [font_char_entry] [i] .no_underlining =
    			font_state->dll_char [i] .no_underlining;
    font_bitmap [font_char_entry] [i] .numrows = 
			font_state->dll_char [i] .numrows;
    font_bitmap [font_char_entry] [i] .numcols = 
			font_state->dll_char [i] .numcols;

    font_bitmap [font_char_entry] [i] .substitute_flag = 
			font_state->dll_char [i] .substitute_flag ;
    
    if (font_state->dll_char [i] .bitmap_ptr > 0)
        {
	number_of_bytes = 
		font_state->dll_char [i] .numrows * 
		((font_state->dll_char [i] .numcols+7)/8);

        font_bitmap [font_char_entry] [i] .bitmap_ptr = resized_buffer;

        for (j = 0; j < number_of_bytes; j++)
	    {
	    *resized_buffer++ = *font_state->dll_char[i].bitmap_ptr++;
            }
        }
     else
        {
	font_bitmap [font_char_entry] [i] .bitmap_ptr = 
				font_state->dll_char [i] .bitmap_ptr;
	} ;
    } ;

return ;
}

/*
 * process_font_status_sheet
 */
VOID process_font_status_sheet ()
{
	return;
}

/*
 * font_pairs
 */
VOID	font_pairs()
{
FONT_NUMBER source_font;
FONT_NUMBER target_font;
UBYTE	target_pairing ;
FONT_ID	target_id;

WORD	i; 

    for (source_font=0; source_font < VAX_FONT_TABLE_SIZE; source_font++ )
	{
	vax_font_table[source_font].pairing_code = NO_DESIRED_PAIR;
	vax_font_table[source_font].opposite_entry = UNUSED;

	target_pairing = NO_DESIRED_PAIR ;

	for (i = 0; i < PAIRING_OPTIONS_COUNT; i++)
	    {
	    if (!strncmp(vax_font_table[source_font].font_file_id.fi_cset,
			pairing_table[i].source_cset,
			FIDS_CSET))
		    {
		    target_pairing= pairing_table[i].source_pairing_code;
		    strncpy(&target_id,
			    &vax_font_table[source_font].font_file_id,
			    FIDS_FONT_ID);

		    strncpy(target_id.fi_cset,
			    pairing_table[i].search_cset,
			    FIDS_CSET);

		    break ;
		    };
	    }


	if (target_pairing != NO_DESIRED_PAIR)
	    {
	    for (   target_font = VAX_FONT_TABLE_SIZE; 
		    target_font >=0; 
		    target_font--	)
		{

		if (!strncmp(&target_id,
			     &vax_font_table[target_font].font_file_id,
			     FIDS_FONT_ID))
		    {
		    vax_font_table[source_font].pairing_code=target_pairing;
		    vax_font_table[source_font].pairing_font= target_font ;
		    break;
		    };
		};
	    };
	};

    return ;
}

/*
 * font_calc_overrides
 */
VOID	font_calc_overrides(target_id)

PTR_FONT_ID target_id;
{
FONT_NUMBER vax_index ;

LONG min_above_baseline = 0;
LONG max_height = 0;
LONG max_below_baseline = 0;
LONG max_superscript_height = 0;
LONG max_subscript_height = 0;
LONG max_average_width = 0;
LONG max_space_max = 0;
LONG max_space_min = 0;


for (vax_index = 0 ; vax_index < VAX_FONT_TABLE_SIZE ; vax_index++)
    {
    if (vax_font_table[vax_index].vax_active == INACTIVE)
	{
	continue;
	} ;

    if ( (!strncmp(&vax_font_table[vax_index].font_file_id,
	           target_id,
		   FIDS_TYPE_FAMILY+
		   1+		/* FIDS_SPACING */
		   FIDS_TYPE_SIZE+
		   1+		/* FIDS_SCALE_FACTOR */
		   FIDS_STYLE+
		   1+		/* FIDS_WEIGHT */
		   1+		/* FIDS_PROPORTION */
		   FIDS_ROTATION )) &&
	 (!strncmp(vax_font_table[vax_index].font_file_id.fi_csubset,
	           target_id->fi_csubset,
		   FIDS_CSUBSET+
		   FIDS_ENCODING+
		   1+		/* FIDS_RESOLUTION */
		   3 )))		/* FIDS_RESERVED */
	{
	    min_above_baseline =
		MIN(min_above_baseline,
		vax_font_table [vax_index] .above_baseline );
		/* We are dealing with a negative number */

	    max_height=
	    	MAX(max_height,
		vax_font_table [vax_index] .total_height );

	    max_below_baseline =
		MAX(max_below_baseline,
		vax_font_table [vax_index] .above_baseline +
		vax_font_table [vax_index] .total_height );

	    max_superscript_height =
		MIN(max_superscript_height,
		vax_font_table [vax_index] .superscript_height );
		/* We are dealing with a negative number */

	    max_subscript_height =
		MAX(max_subscript_height,
		vax_font_table [vax_index] .subscript_height );

	    max_average_width =
		MAX(max_average_width,
		vax_font_table [vax_index] .average_width );

	    max_space_min =
		MAX(max_space_min,
		vax_font_table [vax_index] .vmin_table );

	    max_space_max =
		MAX(max_space_max,
		vax_font_table [vax_index] .vmax_table );
	} ;	
    } ;

for (vax_index = 0 ; vax_index < VAX_FONT_TABLE_SIZE ; vax_index++)
    {
    if (vax_font_table[vax_index].vax_active == INACTIVE)
	{
	continue;
	} ;

    if ( (!strncmp(&vax_font_table[vax_index].font_file_id,
	           target_id,
		   FIDS_TYPE_FAMILY+
		   1+		/* FIDS_SPACING */
		   FIDS_TYPE_SIZE+
		   1+		/* FIDS_SCALE_FACTOR */
		   FIDS_STYLE+
		   1+		/* FIDS_WEIGHT */
		   1+		/* FIDS_PROPORTION */
		   FIDS_ROTATION )) &&
	 (!strncmp(vax_font_table[vax_index].font_file_id.fi_csubset,
	           target_id->fi_csubset,
		   FIDS_CSUBSET+
		   FIDS_ENCODING+
		   1+		/* FIDS_RESOLUTION */
		   3 )))		/* FIDS_RESERVED */
	    {
	    above_baseline_table [vax_index] = min_above_baseline ;

	    height_table [vax_index] = max_height ;

	    below_baseline_table [vax_index] = max_below_baseline ;

	    superscript_height_table [vax_index] = max_superscript_height ;

	    subscript_height_table [vax_index] = max_subscript_height ;

	    average_width_table [vax_index] = max_average_width ;

	    min_table [vax_index] = max_space_min ;

	    max_table [vax_index] = max_space_max ;
	    } ;
    } ;

    return ;
}
