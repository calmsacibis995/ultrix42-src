#ifndef lint
static char SccsId[] = "  @(#)xlc_graph.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

 /* file: xlc_graph.c
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
 *   begin description
 *
 *   Filename :  xlc_graph.def
 *
 *
 *	SIXELS TO HEX conversion program
 *
 *	This program will convert a sixel file into a hex file.  There will be 
 *	no attempts to parse the ANSI command sequence. In fact, only the sixel
 *	data after the q will be dealt with in the program.  Aspect Ratio, Color
 *	and any other parameters will be ignored.
 *
 *	Only one  sixel row at a time will be dealt with and then written to a 
 *	file before going on to the next sixel row.
 *
 *
 *   end description
 *
 *-----------------------------------------------------------------
 */
/*
 *-----------------------------------------------------------------
 *
 *   begin edit_history
 *

	Rev 0		10-FEB-86	Michael Blumenreich
	Original	

	Rev 0.1		25-MAR-86	Michael Blumenreich
	Added transportability to storage buffers and variables

	 15-JUL-1986 13:08:57 		laf
	Added check for a repeat count of zero to "gr_repeat" (QAR 509)
	 28-AUG-1986 21:09:18 		Araj
	Removed "-1" in computation of Byte count in Exit_Graphic.
	This "-1" caused us to loose the last sixel of the last line, if
	the sixel byte count was "1" modulo 8, and there was no 
	overprinting, or the overprinting was bigger than the 
	overprinted area.
	 For instance ESC P q ~ ESC \ would not print
	 nor would    ESC P q !3~ $ !9~ ESC \,

	in the first case byte count was 1, in the seconf the 
	overprinting was 1 modulo 8, and larger than the overprinted 
	area.
	 on the other end, ESC P q ~~ ESC \ would print
	 so would          ESC P q !17~ $ !9~ ESC \,

	in the first case, byte count was 2 modulo 8, in the second, 
	byte count was 1 modulo 8, so was old byte count, but old byte 
	count was > than byte count (overprinting smaller than 
	overprinted).

	 29-AUG-1986 13:30:26		nv
	Changed the evaluation of v_g to be as follows:
		v_g = (6.0 * (hor_grid) * aspect_ratio.xval) / aspect_ratio.yval;
	Changed the definition of 'hor_grid' and 'max_buff_size' from 'double' to
	'LONG'.

	 26-SEP-1986 20:14:31 		araj
					retyped Mikes changes for improved performance
					aspect ratio
	 30-SEP-1986 10:15:07 		mgb
					added default setting to Hor. Grid when input
					parameter is greater than 9.
	 2-OCT-1986 17:48:45 		mgb
					moved check_ap after defining v_g and h_g

	 8-OCT-1986 18:12:59 		araj
					Modified GR_CR to invalidate the 
					raster flag, and modified GR_AR
					to check the raster_flag in 
					conjunction with byte_cnt and
					old_byte_cnt, to ensure that
					no GR_NL, or Sixel, was received 
					before the AR command.
					Ensuring that no GR_NL had been 
					received did not match QA 
					requirements.

	 10-DEC-1986 11:41:35 		mgb
					Adding parameters to call to check_ap

	 29-APR-1987 10:58:21 		mgb
					Changed call to vpos_rel() to 
					vpos_rel_w_wrap() because vpos_rel() uses
					the wrong bottom limit.

	 26 May 1988 11:00		tep - added support for sixelfont:
						1) #include sixel_font_init_list
						2) arrays six_rptr[], sixel_cmprss_mppng[]
						3) routine init_sixel_font_keys()

	 24-JUN-1988 17:04:31 		kws
					Add angle brackets on #include for stdio.h
 *
 *	20-APR-1989 14:59		araj
 *					changed byte_cnt from word to long
 *
 *
 *-----------------------------------------------------------------
 */

/**************************
	DEFINITIONS
**************************/

#include	<stdio.h>
#include	"portab.h"
#include	"xlate.h"
#include	"capdl.hc"
#include	"xlc_ps.hc"
#include	"xlc_codegen.hc"
#include	"xlc_graph.hc"
#include	"xlc_iface.hc"



/***********************************************************************
*   
*   init_sixel_font_keys ()
*
*  this routine translates the source vector init_list[]
*    into the array of data structures sixel_cmprss_mppng[];
*    it simultaneously builds part of a PostScript encoding vector
*    for the variable characters and outputs it to build the sixel font;
*  each element of the data structure consists of a repeat
*    count and a sixel_font character that expands to that count;
*    also part of this initialization is defintion of six_rptr[], 
*    the array of pointers into sixel_cmprss_mppng[];
*  mapping characters are assigned consecutively from 'space' upward 
*    as far as '>';  too many or too few entries in the init_list are 
*    cut off or padded out, as needed;
*    this version of the sixel_font does not use characters in GR at all
************************************************************************/



VOID init_sixel_font_keys()

{
  WORD	i, j, k, longest_run;
  UBYTE	mppng_char;
  UBYTE	null_mppng_char = (UBYTE)0; /* null mapping char is unused value */

			/* set all repeat counts to zero to start */
  for ( j=0 ; j < ( sizeof(init_list)/sizeof(WORD) ) ; j++ )
    sixel_cmprss_mppng[j].rpt_val = 0;
			/* all pointers point to an empty structure 
				in case there aren't enough real ones */
  for ( i=0 ; i < 64 ; i++ ) six_rptr[i] = &sixel_cmprss_mppng_null; 

			/* build a PostScript procedure to fill the part
				of the encoding vector based on init_list */
  ps_str( "/sixel_encoding_low31 " );
  ps_str( str_open_proc );

	/* scan the init list, load sixel equivalence structure
		and output the encoded names to the open PS proc;
		stop scanning at the end of init_list or when
		the mapping character exceeds the non-sixel range */
  k=0; j=0; i=0; mppng_char = ' ';	/* init indexes, mapping character */
  longest_run = 0;			/* keep a value of the longest run */

  while ( ( j < ( sizeof(init_list)/sizeof(WORD) ) ) && ( mppng_char < '?' ) )
    {
      six_rptr[i++] = &sixel_cmprss_mppng[j];
      do 
        {
          if ( ( sixel_cmprss_mppng[j].rpt_val = init_list[j] ) != 0 )
            /*then*/
              {		/* put mapping character into structure */
                sixel_cmprss_mppng[j].char_mapped = mppng_char++;
			/* put the encoded name into the partial encoding vector */
                sprintf( str_buffer, "/%dx%d ", init_list[j], i-1 );
                ps_str( str_buffer );
                if ( ++k%8 == 0 ) ps_str( "\n" );	/* wrap lines */
                if ( init_list[j] > longest_run ) longest_run = init_list[j];
              }
            else
                sixel_cmprss_mppng[j].char_mapped = null_mppng_char;

        } while ( ( init_list[j++] != 0 ) && ( mppng_char < '?' ) );
    }	/* end while */
	/* fill out PS encoding vector if not all mapping characters were used */
  while ( mppng_char++ < '?' ) ps_str( "/.notdef " );

  ps_str( str_close_proc );		/* close the list of names */
				/* define the longest run value for the font */
  sprintf( str_buffer, " /longest_sf_run %d def\n", longest_run );
  ps_str( str_buffer );
  sprintf( str_buffer, " /sixel_font_unid %d def\n", 2337119 ); /* *** random number *** */
  ps_str( str_buffer );
  ps_str( " generate_sixelfont " );	/* execute the sixelfont proc, invoking  the names */
} /* end init_sixel_font_keys */



/*
 *   PROCESS_SIXEL
 */

VOID process_sixel(sixel_ptr)
PSIXEL sixel_ptr;
   {
    LONG    sixel_left_index;	/*  left most position in buffer where to store sixel */
    LONG    sixel_right_index;	/*  left most position in buffer where to store sixel */
    LONG    sixel_index;	/*  loop index */

				
    /*  Optimization, if sixel is blank, just go home */
    if (sixel_ptr->sixel_code == 0)
    {
	return;
    }


    /*	Should we empty the sixel buffer ? */
    if	(    (sixel_ptr->sixel_ap.yval != cg_st.cgst_sixel_last_ap.yval)
	||  (sixel_ptr->sixel_ap.xval < cg_st.cgst_sixel_last_ap.xval)
	)
    {
	sixel_empty_sixel_buffer();


	/*  save the active position of the firts sixel in the buffer
	 *  from then on, all sixels must have the same grid, the same
	 *  avp, and a higher ahp, else, we empty the buffer and start again.
         */

	cg_st.cgst_sixel_last_ap.yval= sixel_ptr->sixel_ap.yval ;
	cg_st.cgst_sixel_last_ap.xval= sixel_ptr->sixel_ap.xval ;
    }


    /*	Compute starting x in buffer, (ahp- (starting ahp of buffer))/ pixel_width 
     *	This cannot be a negative number, as we ensured above that ahp was increasing
     */
   sixel_left_index =
	    (sixel_ptr->sixel_ap.xval- cg_st.cgst_sixel_last_ap.xval)/sixel_ptr->sixel_size.xval;

   sixel_right_index =
	    (sixel_left_index + sixel_ptr->sixel_repeat - 1);

    /*
     *	PS only allows 4K in a string, so we can't put more than 32K in the buffer at once
     *	For this time, we'll also cheat, as we know that CPAR won't send us more 
     *	than 32K at once, so we only have to do this once. In the future, we need to 
     *	use some iterative code
     */

   if	(sixel_right_index >= MAXIMUM_PS_SIXEL_STRING)
	{
	    sixel_empty_sixel_buffer();
	    cg_st.cgst_sixel_last_ap.yval= sixel_ptr->sixel_ap.yval ;
	    cg_st.cgst_sixel_last_ap.xval= sixel_ptr->sixel_ap.xval ;
	    sixel_left_index = (sixel_ptr->sixel_ap.xval- cg_st.cgst_sixel_last_ap.xval)/sixel_ptr->sixel_size.xval;
	    sixel_right_index = (sixel_left_index + sixel_ptr->sixel_repeat - 1);
	}



    /*	Now store this in the buffer, ORing it with what is already there */

    cg_st.cgst_sixel_buffer_empty = FALSE;	/* Let's keep track of our achievements	    */
    cg_st.cgst_wpf = TRUE;

    for (sixel_index = sixel_left_index; sixel_index <= sixel_right_index;
	sixel_index++)
    {
	sixel_buffer [sixel_index] |=   sixel_ptr->sixel_code;
    }
    if (sixel_index > cg_st.cgst_rightmost_pixel)
    {
	cg_st.cgst_rightmost_pixel = sixel_index;

    }
}


/*
 * pdli_get_sixel_macro_grid
 */

VOID pdli_get_sixel_macro_grid (macro_number, aspect_ratio, physical_grid_ptr)
WORD macro_number;
PTR_RATIO aspect_ratio;
PTR_POINT physical_grid_ptr;
{
    physical_grid_ptr->xval = sxl_mac_tbl [macro_number][0];
    physical_grid_ptr->yval =
	(sxl_mac_tbl [macro_number][0] * sxl_mac_tbl [macro_number][1] * 6L)
	/ sxl_mac_tbl [macro_number][2];
    aspect_ratio->xval = sxl_mac_tbl [macro_number][1];
    aspect_ratio->yval = sxl_mac_tbl [macro_number][2];

}


/*
 *  Get the physical sixels grid
 */

VOID pdli_get_physical_grid(horizontal_grid, aspect_ratio, physical_grid_ptr)
LONG horizontal_grid;
PTR_RATIO aspect_ratio;
PTR_POINT physical_grid_ptr;
{


    /*	Let's empty the sixel buffer, so we don't mix sixels
     *	of a different kind in a single buffer.
     */
    sixel_empty_sixel_buffer ();

 
    /* Let's not forget to return the desired values back to the caller	    */
    /* And keep a copy for ourselves					    */

    cg_st.cgst_sixel_size_cpt.xval =   horizontal_grid;
    cg_st.cgst_sixel_size_cpt.yval =
	(6 * (horizontal_grid * aspect_ratio->xval))/aspect_ratio->yval;
    
    physical_grid_ptr->xval = horizontal_grid;
    physical_grid_ptr->yval =
	(horizontal_grid * aspect_ratio->xval * 6)/aspect_ratio->yval;

}



VOID process_color(color_struct_ptr)
PCOLOR color_struct_ptr;
   {
   }


VOID comp_init()
{
    DEF  sixel_index;


    /*  Now clear the sixel buffer */
    for (sixel_index = 0; sixel_index < BLK_SIZE ; sixel_index++)
    {
	sixel_buffer [sixel_index] = 0;
    }

    cg_st.cgst_sixel_buffer_empty = TRUE;

}




/*									    */
/*  Sixel_empty_sixel_buffer						    */
/*									    */

VOID sixel_empty_sixel_buffer ()
{

    LONG    sixel_index;		/*  Loop counter */
    LONG    byte_cnt;
    LONG    horizontal_image_size;
    LONG    vertical_image_size;


/*  Considering that we only call this routine once per sixel line, there   */
/*  is now less need to be clever, by caching value, or pre-converting ...  */
/*  as opposed to the first approach, where sixels were converted to tiles  */
/*  on an individual basis, and converting from centipoints to dots was an  */
/*  enormous burden							    */


/*  The parameters for this routine are global, they are		    */
/*	the sixel_buffer, filled by process_sixel			    */
/*	the sixel_size_cpt, filled by get_physical_grid			    */
/*	the page_limits in dots, currently filled by get_physical_grid,	    */
/*	but that will be filled by comp_determine_page_size in the future   */
/*									    */
/*  The sixel buffer is an hybrid, in the x axis, it is in dots, in the y   */
/*  axis, it is in pixels, the x and y axis are user coordinates, they have */
/*  not yet been rotated to account for page orientation		    */
/*									    */


    /*	If the buffer is empty, forget it				    */
    if (cg_st.cgst_sixel_buffer_empty)
	return;


    /*	Send the buffer to the PS_Machine   */

    byte_cnt = ((cg_st.cgst_rightmost_pixel +7)>>3); 
    horizontal_image_size = byte_cnt * 8 * cg_st.cgst_sixel_size_cpt.xval;
    vertical_image_size =   cg_st.cgst_sixel_size_cpt.yval;

    dispose_sixel ( cg_st.cgst_sixel_last_ap.xval,
		    cg_st.cgst_sixel_last_ap.yval,
		    horizontal_image_size,   
		    vertical_image_size,
		    byte_cnt,
		    cg_st.cgst_sixel_size_cpt.xval,
		    cg_st.cgst_rightmost_pixel, 
		    sixel_buffer);

    /*  Now clear the buffer */

    for (sixel_index = 0; sixel_index <= cg_st.cgst_rightmost_pixel; sixel_index++)
    {
	sixel_buffer [sixel_index] = 0;
    }
    cg_st.cgst_rightmost_pixel = 0; 
    cg_st.cgst_sixel_buffer_empty = TRUE;
    
}



