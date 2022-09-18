#ifndef lint
static char SccsId[] = "  @(#)cajfy.c	4.1   LPS_ULT_TRN   7/2/90";
#endif

/* file: cajfy.c
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,
 *	1986, 1987, 1988, 1989 ALL RIGHTS RESERVED.
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
 *   001   28-JAN-1988 13:12  mhw
 *	    Initial entry for common code
 *
 *   002   15-FEB-1988 11:51  bf
 *	    Merged real and debug code.
 *
 *   29-MAR-1988 17:10  mhs
 *   Add support for JFY split tables, plus stubs for new 
 *   routines for limits/nolimits/jfyoff.
 *
 *  22-JUL-1988 10:30 mhw
 *	Call process_vchar, not process_char for space and blob
 *
 *  31-AUG-1988 09:02 mhw
 *	Space in justify should use vir_char_add, not vir_char_space
 *
 *  29-NOV-1988 13:32 mhw
 *	vir_char_space does not = 20h anymore, so it must be tested for
 *	in the justify routine, else process_char would be called
 *
 *  18-DEC-1988 15:19 ejs
 *		Major changes to data structutes.  GLYPH has been modified
 *		to allow struct copies more easily.  The PS output is not
 *		changed (barring bugs of course).
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
 *   Filename:  cajfy.c
 *
 *	Combination of Dump Utility version and real code 
 *
 *    The module contains
 *
 *   dec_jfy()
 * pr_jfy ()		- Handle ANSI justify command
 * enter_jfy()		- Init for a new line to be justified
 * add_to_jfy_buf()	- Add another char to the justify buffer
 * empty_jfy_buf()	- Dispose of contents of justify buffer
 * justify_buffer()	- Calculate a space char adjustment factor
 *
 *   end description
 *
 *-----------------------------------------------------------
 */

/* Translator edit history for this file
 *
 * file:	xlc_jfy.c - Translator justification routines
 * created:	gh	 12-MAR-1986 12:10:41
 * edit:	gh   	 24-MAR-1986 10:49:45 variable names update
 *		gh	 26-MAR-1986 14:43:16 put max on add_to_jfy_buf
 *		gh	 8-APR-1986 15:49:56 Changing justify_buffer to empty
 *			  w/o justification the jfy buf when it overflows, &
 *			  when justify was on then JFY-off sequence received.
 *			  Bumped justify buffer size up to 512.
 *		gh	 7-MAY-1986 17:21:01 Updated call to get_width()
 *		gh	 3-JUN-1986 10:33:15 There were still three places where
 *			  'clib' routines were being called with 'glyph->font'
 *			  rather than 'glyph->font.ps_font_number'. - fixed
 *		gh	 16-JUN-1986 13:41:18 Changing pr_jfy() to allow
 *			  selecting jfty_with_limits once jfy_wo_limits already
 *			  in effect and visa-versa.
 *		gh	 27-JUN-1986 13:52:35 Adding processing of virtual chars
 *			 in empty_jfy_buf()
 *		gh	 14-JUL-1986 15:14:39 Since the right bearing can be
 *			 beyond the next chaacter's starting ahp, empty_jfy_buf
 *			 must therefore output a VIR_ADD char instead of a
 *			 VIR_ERASE on the first and last chars of a justified
 *			 line.  Adding this code.
 *		gh	 21-JUL-1986 14:23:42 Fixed how right bearing was used
 *			 in justification calculations.
 *
 *		araj	 1-OCT-1986 14:48:17 Trying to fix the 
 *			position used for the erasure of the attributes 
 *			that extend past the left and right 
 *			anchors (remember that the characters 
 *			are aligned with the anchors, not their charbox, but that the 
 *			attributes work on the full box, causing a few dashes outside 
 *			the anchors). The problem is that the 
 *			position of the erase commands is not 
 *			computed properly In addition, trailing 
 *			blanks are printed when they should not.
 *			As a irst step, just moving stuff around 
 *			so I can read it.
 *
 *		araj	 3-OCT-1986 12:14:34 Note that in the 
 *			mean time, Nick incorporated two of these
 *			fixes in a temporary version, to drop trailing spaces
 *			and fix ahp = avp + temp
 *
 *		araj	 20-OCT-1986 08:53:27 
 *			Inverted with_limits/Without_limits, as 
 *			these were the opposite of the spec.
 *
 *			Added a test to ensure that even without 
 *			limits, the width of space remains > 0
 *
 *			Improved teh parameter tests, to ensure 
 *			that only valid parameters are 
 *			processed, up to now, any sequence
 *			would toggle the state, unless the 
 *			new parameter was equal to the previous 
 *			one that would be fine if an other test 
 *			was made to allow only for valid 
 *			parameters.
 *
 *			Added a reprint of the first and last 
 *			character to suppress dents when erasing
 *			attributes that exceed the border.
 *
 *		 21-OCT-1986 09:25:10 araj
 *			Modified code to use limits when 
 *			limits are exceeded, rather than 0
 *
 *		 23-OCT-1986 08:47:02 araj
 *			Modified code to use o when limits
 *			are exceeded rather than limits
 *			A previous QAR caused the above
 *			change, because they wanted us to use 
 *			lower limit when the lower limit was 
 *			exceeded, the wording caused the
 *			limits to be used for both Min and 
 *			max. They want us to use 0 if max is 
 *			exceeeded, so we'll do.
 *
 *		araj	 30-OCT-1986 17:21:38 
 *			Modified code so that esc [ ? o sp F
 *			does not get us out of JFY
 *
 *		araj	 3-NOV-1986 11:56:59 
 *			When we re-space a font, for instance
 *			to print a 10cpi font at 5cpi,
 *			the rightmost character should be lined
 *			up on the character box, not the 
 *			respaced box.
 *
 */




/*  begin include_file    */

#define cajfy (1)	    /* define for this file to get static vars. */

#include    "portab.h"	    /* general portability constants */
#include    "cpsys.hc"	    /* system wide constants, typedefs, etc. */
#include    "cpast.hc"	    /* search tables				*/
#include    "caglobal.hc"   /* global defs				*/
#include    "camac.lib_hc"  /* Tempory macros for font functions	*/

#ifdef DUMP  
#include    "dumpu.hc"	    /* Dump Utility                             */
#include    "xlc_iface.hc"  /* defines for oprintf			*/
#endif DUMP

/*  end   include_file    */




/*****  enter_jfy()  ****************************************************
 *									*
 *	enter_jfy() - Reset the justify buffer index.			*
 ************************************************************************/

VOID enter_jfy()

{
#ifdef DUMP
{
  return;
}
#endif

    	/* Set the left anchor to a very high value (greater than any right
    	   limit would be set).  The reason is if justify mode is on, 
    	   characters are in the justify buffer, and empty_jfy_buf() is called
    	   without justification having been perfomed first, then the
    	   test for adjustnment-necessary in empty_jfy_buf() will fail,
    	   allowing the buffer to be emptied as fast as possible and without
    	   alteration. */
    	left_anchor = xl_st.h_lim_bound.max + 1;
    	space_adj = 0;
	xl_st.jfy_buf_index = 0;	/* Zero out buffer-store index */
	left_char = 0;			/* Left most char is 0 */
	right_char = -1;		/* Flags that the line */
					/*has not been justified */
}


/*****  empty_jfy_buf()  ************************************************
 *									*
 *	empty_jfy_buf() - Dispose of all current contents of the 	*
 *	justify buffer.  						*
 ************************************************************************/

VOID empty_jfy_buf()
{
    WORD	cur_index;
    LONG	cur_char_offset, max_ahp;
    GLYPH	*gptr, vchar;

/* If empty, forget it */

if (xl_st.jfy_buf_index == 0)		return;

/* Buffer is not empty, let's print it */

/* Initialize some variables */	

gptr = &xl_st.justify_buf[0];		/* Pointer to current character */
cur_char_offset = 0;		/* Displacement in character 
				   position caused by justification */
max_ahp = left_anchor;		/* */


/*
   If the line was not justified (right_char == -1), 
   no adjustment is required, print things fast 
*/
/*
   Note that a space_adj of 0 is not sufficient, as it could be 
   that the line was justified, but the width of spaces need no 
   modification. In such a case, we still drop trailing spaces 
*/

if (right_char == -1 ) 
	{	
	/* print all characters as is */
	for (cur_index = 0;  cur_index < xl_st.jfy_buf_index  ; cur_index++, gptr++)  
		{
		process_char(gptr);
		}
	/* reset things */
	enter_jfy();		/* Indicate justify buffer is empty */
	return;
	}	



/* First let's print the leading spaces without any modification */

for (cur_index = 0;  cur_index < left_char ; cur_index++, gptr++)  
	{
	process_char(gptr);
	}

/* Now let's print the left anchor */
	/* First move it left by an amount equal to it's left 
	/* bearing so the character is aligned with the limit, 
	/* not it's cell */

	cur_char_offset = - get_left_bearing (gptr->char_data.char_code, 
            gptr->char_data.char_font);
	gptr->ap.xval += cur_char_offset;

	/*  Now let's remove all the line attributes of this
	/*  character, as they may have to be shortened */

	/*  Let's print the stripped character */

	copy_glyph (gptr, &vchar);
	vchar.font_data.algorithmic_attributes &= (~(UL | STRIKE | DOU_UL | OVERLINE));    
	process_char(&vchar);

/* Now let's print the attributes of the character */

if ( gptr->font_data.algorithmic_attributes & (UL | STRIKE | DOU_UL | OVERLINE ) 
	/* attributes on */
   )
	{
	copy_glyph(gptr,&vchar);
	vchar.char_data.char_code = VIR_CHAR_ADD;
	vchar.char_data.char_width  -= cur_char_offset;
	process_vchar(&vchar);
	}

gptr++;
cur_index++;



/* Output the contents of the justify buffer til right most 
character excluded */

for ( ; cur_index < right_char; cur_index++, gptr++)  
{

	/* if the character is not a space */
	if ((gptr->char_data.char_code != SP_C) && (gptr->char_data.char_code != VIR_CHAR_SPACE))
	{

		/* If this character was beyond the 
		   highest ahp yet, record it. */

		if (gptr->ap.xval > max_ahp)  max_ahp = gptr->ap.xval;

		/* adjust the position of the character */
		gptr->ap.xval += cur_char_offset;

		/* Print the character */
		process_char(gptr);

	}

	else
	{	/* This is a space  or a virtual space */
	
		/* If it is beyond the right anchor, ignore it */
		if (gptr->ap.xval > xl_st.justify_buf[right_char].ap.xval) continue;

		/* if it increases the max_ahp, 
		   record the new ahp and include it
		   in the correction factor and
		   modify it's width  */
		if ((gptr->ap.xval > max_ahp) && (gptr->char_data.char_code == SP_C))
		{
			max_ahp = gptr->ap.xval;
		
			/* adjust the position of the character */
			gptr->ap.xval += cur_char_offset;

			/* It is within bounds, print it */
			copy_glyph (gptr, &vchar);
			vchar.char_data.char_code = VIR_CHAR_ADD;
			vchar.char_data.char_width += space_adj;
		    	process_vchar(&vchar);
	
			/*Update the adjustment factor */
			cur_char_offset += space_adj;
	

		} /* End this space is a justifiable space */
		else
		{ /* This space does not increase ahp 
		     print it, but do not adjust it */

			/* adjust the position of the character */
			gptr->ap.xval += cur_char_offset;
			copy_glyph(gptr,&vchar);
			vchar.char_data.char_code = VIR_CHAR_SPACE;
			process_vchar(&vchar);

		
		} /* End this space is not a justifiable space */
		  /* End Space */	

	} /*End of characters (if_else space) */


}	/*End of for loop */


/* Let's print the right most character, but first, erase any attribute 
   that could overflow the right anchor */
/* By definition, the right most character cannot be a space */

    /*	If this character was beyond the 
	highest ahp yet, record it. */

    if (gptr->ap.xval > max_ahp)  max_ahp = gptr->ap.xval;

    /* adjust the position of the character */
    gptr->ap.xval += cur_char_offset;

    /* Print the character  without its attributes */


    copy_glyph(gptr,&vchar);
    vchar.font_data.algorithmic_attributes  &= (~(UL | STRIKE | DOU_UL | OVERLINE ));    
    vchar.char_data.char_width = FNT_WIDTH(gptr->char_data.char_font,
					   gptr->char_data.char_code);
    process_char(&vchar);


    if (gptr->font_data.algorithmic_attributes & (UL | STRIKE | DOU_UL | OVERLINE ))  
    {
    	    copy_glyph(gptr,&vchar);
    	    vchar.char_data.char_width =
		  FNT_WIDTH(gptr->char_data.char_font,
			    gptr->char_data.char_code)
		- get_right_bearing(vchar.char_data.char_code, 
				    vchar.char_data.char_font);
	    vchar.char_data.char_code = VIR_CHAR_ADD;
	    process_vchar(&vchar);
    }

/* Point to next char again */

cur_index++;
gptr++;



/* Now print all the characters that were entered after the 
   right most one
*/


for ( ; cur_index < xl_st.jfy_buf_index  ; cur_index++, gptr++)  
{
	/* adjust the position of the character */
	gptr->ap.xval += cur_char_offset;

	/* if the character is not a space */
	if ((gptr->char_data.char_code != SP_C) && (gptr->char_data.char_code != VIR_CHAR_SPACE))
	{
		/* Print the character */
		process_char(gptr);

	}

	else
	{	/* This is a space  or a virtual space */

		/* If it is beyond the right anchor, ignore it */
		if (gptr->ap.xval < xl_st.justify_buf[right_char].ap.xval);


		/* It is within bounds, print it */

		copy_glyph(gptr,&vchar);
		vchar.char_data.char_code = VIR_CHAR_SPACE;
		process_vchar(&vchar);
	

	} /*End of characters (if_else space) */

}	/*End of for loop */


/*
	All done, reset things 

*/


	enter_jfy();		/* Indicate justify buffer is empty */
}





/*****  pr_jfy_off()  ***************************************************
 *									*
 *	pr_jfy_off - Turn off justification				*
 ************************************************************************/

VOID pr_jfy_off()
{

#ifdef DUMP
    {
    oprintf("JUSTIFY OFF \n");    
    pprint();		    /* Print the parameter list */
    }
#endif

    /*	If Justify mode was on, and a command was received to turn it off.
	Empty the justify buffer. */

    if (xl_st.justify_mode == JUSTIFY_ON)  
    {	
	empty_jfy_buf();
	xl_st.justify_mode = JUSTIFY_OFF;
    }

}




/*****  pr_jfy_limits()  ************************************************
 *									*
 *	pr_jfy_limits - Turn on justification with limits		*
 ************************************************************************/

VOID pr_jfy_limits()
{
#ifdef DUMP
    {
    oprintf("JUSTIFY LIMITS \n");    
    pprint();		    /* Print the parameter list */
    }
#endif
    /* Set limits enabled */

    xl_st.limits_enabled_flg  = TRUE;


    /*	If justify mode was off, and it was just turned on,
	call 'enter_jfy()'. */

    if (xl_st.justify_mode == JUSTIFY_OFF)  
	{
    		enter_jfy();
		xl_st.justify_mode = JUSTIFY_ON;
	}

}




/*****  pr_jfy()  *******************************************************
 *									*
 *	pr_jfy - Turn on/off justification				*
 ************************************************************************/

VOID pr_jfy()
{
#ifdef DUMP
    {
    oprintf("JUSTIFY \n");    
    pprint();		    /* Print the parameter list */
    }
#endif
    cp_split(ast_jfy_srch);
    return;
}





/****************************************************************************
    DEC Private Justify Text
****************************************************************************/

VOID dec_jfy()
{
#ifdef DUMP
    {
    oprintf("DEC JUSTIFY \n");    
    pprint();		    /* Print the parameter list */
    }
#endif
    cp_split(ast_decjfy_srch);
    return;
}




/****************************************************************************
    DEC Private Justify Text No Limits
****************************************************************************/

VOID dec_jfy_nolimits()
{
#ifdef DUMP
    {
    oprintf("DEC JUSTIFY NOLIMITS \n");    
    pprint();		    /* Print the parameter list */
    }
#endif
    /* Set limits disabled */

    xl_st.limits_enabled_flg  = FALSE;


    /*	If justify mode was off, and it was just turned on,
	call 'enter_jfy()'. */

    if (xl_st.justify_mode == JUSTIFY_OFF)  
	{
    		enter_jfy();
		xl_st.justify_mode = JUSTIFY_ON;
	}
}


/*****  justify_buffer()  ***********************************************
 *									*
 * Justify the line in the justify buffer by:				*
 *  - Scanning off leading and trailing spaces				*
 *  - Count the number of spaces between the first and last characters	*
 *	that cause the effective ahp to be increased			*
 *  - Calculate the space_adj necessary to expand or contract the line	*
 *	to have the line justified between the left anchor and right	*
 *	margin								*
 *  - If limits are enabled, and the effective size of a space char	*
 *	will be too small or too big, set space_adj = 0.		*
 ************************************************************************/

VOID	justify_buffer()
{
	GLYPH	*gptr;
	LONG	right_maxahp, max_ahp;
	WORD	i, num_spaces;
	LONG	print_area, text_area, amount_to_spread, size_of_space;
	LONG    left_bearing;	/* Hold centerlines a character for purpose */
	LONG    right_anchor;	/*   for purpose of justification */
	BOUND	space_lim;	/* With limits enabled, hold the minimum and */
				/*   maximum allowable width for a space char */

	/* Set default for space character adjustment factor */
	space_adj = 0;

    	/* If justify buffer is empty, don't waste time trying to justify! */
    	if (xl_st.jfy_buf_index  == 0)		return;


	/* Remove trailing spaces in the justify buffer */
	gptr = &xl_st.justify_buf[xl_st.jfy_buf_index -1];
	while (0 < xl_st.jfy_buf_index )  
	{
		if (gptr->char_data.char_code != SP_C)	break;
		gptr--;
		xl_st.jfy_buf_index --;
	}


	/* Scan off the leading spaces in the justify buffer */
	gptr = &xl_st.justify_buf[0];
	left_char = 0;
	while (left_char < xl_st.jfy_buf_index )  
	{
		if (gptr->char_data.char_code != SP_C)	break;
		gptr++;
		left_char++;
	}

	/* Return if no non-spaces were found */
	if (left_char == xl_st.jfy_buf_index )  	return;

	/* Fetch the left bearing of the character that forms the left anchor,
	   and calculate a left_anchor */
	left_bearing = get_left_bearing(gptr->char_data.char_code,
					gptr->char_data.char_font);
	left_anchor = gptr->ap.xval;

	/* Continue scanning the justify buffer and record as right_char the
	   character that logs the highest ahp value. */

	i = right_char = left_char;
	right_maxahp = left_anchor;
	while (i < xl_st.jfy_buf_index )  
	{

		if (	(gptr->ap.xval > right_maxahp)
		     &&	(gptr->char_data.char_code != SP_C)  
		   )
		{
			right_maxahp = gptr->ap.xval;
			right_char = i;
		}
		gptr++;  i++;
	}



	/* Fetch the right bearing of the character that logged the highest
	   ahp and add to its ahp */

	gptr = &xl_st.justify_buf[right_char];
	
	/* Was using modified width of character (wid) should 
	** use "true" width
	** right_maxahp = gptr->ap.xval + 
	**		  gptr->char_data.char_width -
	**		  get_right_bearing(gptr->char_data.char_code,
	**				    gptr->char_data.char_font);	
	*/

	/* Fetch the real width of the rightmost character, in 
	**   case, we are using spacing 
	*/
	right_maxahp =
		   gptr->ap.xval
		 + FNT_WIDTH(gptr->char_data.char_font,
			     gptr->char_data.char_code)
		 - get_right_bearing(gptr->char_data.char_code,
				     gptr->char_data.char_font);

	/* Now lets count the number of spaces between the character on the 
	   left anchor point and the character that logged the highest ahp to
	   the right (only the spaces that cause the effective ahp to be
	   incremented count provided they are not beyond the 
	   right most printable) */

	i = left_char;
	gptr = &xl_st.justify_buf[left_char];
	max_ahp = gptr->ap.xval;
	num_spaces = 0;

	while (i <= right_char)  
	{
		if (gptr->ap.xval > max_ahp)  
		{
			if (gptr->char_data.char_code != SP_C) max_ahp = gptr->ap.xval;
			else
			{	if (gptr->ap.xval <= xl_st.justify_buf[right_char]
								    .ap.xval)
				{
					max_ahp = gptr->ap.xval;
					num_spaces++;
				}
			}
		}
		gptr++;  i++;
	}
	/* If num_spaces =0, return with space_adj = 0 so we don't get
	   any divide errors in the code that follows */
	if (num_spaces == 0)		return;

	/* Line end pos is the right anchor point for justification.
	   Calculate print area as right anchor - left anchor */
	right_anchor = xl_st.h_fmt_bound.max;
	print_area = right_anchor - left_anchor;

	/* text area (including SP's) = right max ahp - (left anchor + left bearing of first char) */
	text_area = right_maxahp - (left_anchor + left_bearing);

	/* amount to spread = print area - text area */
	amount_to_spread = print_area - text_area;

	/* Calculate the amount to add to each final space that is output */

	/* Note, if the adjustment is positive, the divide will round down which
	   ok, but id it is negative, it will be rounded up, which is bad.
	   So a little magic is used to garantee a rounding down
        */

	if (amount_to_spread >= 0)
	{
	    space_adj = amount_to_spread / (LONG) num_spaces;
	}
	else
	{
	    space_adj = (amount_to_spread - (num_spaces -1) )/ (LONG) num_spaces;
	}


	/* Get width of a space character of the font type found in the
	first character in the justify buffer */
	size_of_space = get_width((WORD)SP_C, xl_st.justify_buf[0]
						    .char_data.char_font);

	/* If the adjustement would make the space < 0, limit it to zero */
	if ( ( space_adj + size_of_space) < 0 )
	{
		space_adj = (-size_of_space);
	}

	/* If limits are enabled, then if the width of a space character with
	   the newly-calculated adjustment factor is less than the minimum
	   space width, or greater than the maximum space width allowed,
	   then set the space adjustment factor = 0.  */
	if (xl_st.limits_enabled_flg )  
	{


		/* Get limits */
		get_font_limits((WORD)SP_C, xl_st.justify_buf[0]
					    .char_data.char_font,&space_lim);

		/* Check */

		if ( (size_of_space + space_adj) > space_lim.max)
		{
			space_adj = 0;
		}
		if ( (size_of_space + space_adj) < space_lim.min)
		{
			space_adj = space_lim.min - size_of_space;
		}
	}


} /* End justify_buffer */


/*****  add_to_jfy_buf()  ***********************************************
 *									*
 *	add_to_jfy_buf(glyph) - Add the character to the justify	*
 *	buffer.  Return to the caller the width of the character.	*
 ************************************************************************/
LONG	add_to_jfy_buf(cglyph)
GLYPH	*cglyph;
{
    	if (xl_st.jfy_buf_index  >= JBUF_SIZE) empty_jfy_buf();

	/* Capture the current translator state and save it with the character
	   in the justify buffer */
    	copy_glyph(cglyph,&xl_st.justify_buf[xl_st.jfy_buf_index ++]);
	return(cglyph->char_data.char_width);		/* Return the width of the character */
}


