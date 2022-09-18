/*        @(#)xlm_codegen.h	4.1      7/2/90      */
/* file:        xlm_codegen.h - Translator code generator routines
 * created:     mgb     29-MAY-1986 15:22:08 
 * edit:        gh       20-JUN-1986 13:09:07 Changed M$SET_FONT to compute
 *                      [1200 0 0 -1200 0 0] makefont in str_buffer, then
 *                      to output that in the middle of str_courier string.
 *              mgb     25-JUN-1986 14:04:00 Put output_vchar in and modified
 *                      set_font.
 *
 *              araj     26-JUL-1986 13:19:11 
 *                      modified set origin to use M$UPDATE_AP_A (-1,-1)
 *                      instead of M$SET_AP (-1,-1)
 *                      The goal is to indicate an invalid AP, 
 *                      not to force one.
 *              gh       11-AUG-1986 14:02:24 Removed M$OUTPUT_PREAMBLE -
 *                      put equiv code in codegen because needed to make
 *                      output of error handler conditional, which macro
 *                      processor did not seem to like
 *              mgb      20-AUG-1986 11:10:13 Adding sheet length to current
 *                      ps state.
 *              laf      27-AUG-1986 15:57:15 Modified M$CLOSE_SHOW to fix
 *                      problem of UL "following" the chars on PLU/PLD.
 *              nv       15-SEP-1986 09:36:17 Removed NUMBER_OF_PS_DICTIONARIES
 *                      and NUMBER_OF_PS_FIXED_FONT_DICTNRS.
 *
 *      mgb     19-APR-1988 18:05
 *              changed ps_font... to paired_font...
 *
 *      mgb     12-MAY-1988 14:38
 *              Adding font cache to M$SET_FONT
 *
 *      kws     29-JUL-1988 16:32:42 
 *              Change boxnumber to unspaced_boxnumber in M$SET_FONT.  
 *              pass unspaced_boxnumber it to dispose_cache_spacing &
 *              dispose_add_spacing_to_cache.  Change boxnumber to
 *              spaced_boxnumber in M$SET_SPACING.
 *
 *
 *      ejs  1-SEP-1988 19:04
 *              Modified a number of macros to use the CPAR approch to font
 *              numbering.  This included:
 *
 *                              M$SAVE
 *                              M$SET_FONT
 *                              M$SET_SPACING
 *                              M$CHECK_STATE_CHANGE_FONT
 *                              M$CHECK_STATE_CHANGE_ALL
 *
 *	araj	29-NOV-1988 15:18
 *	    Removed M$SET_FONT snd M$SET_SPACING, modified M$CHECK_STATE_CHANGE_FONT
 *	    to do test, then call subroutine set_font_and_spacing.
 *
 *  18-DEC-1988 15:19 ejs
 *		Major changes to data structutes.  GLYPH has been modified
 *		to allow struct copies more easily.  The PS output is not
 *		changed (barring bugs of course).
 *
 *  20-MAR-1989 C.Peters
 *		Removed extraneous '&' operator for Ultrix port.
 *
 *  23-MAR-1989 C.Peters
 *		Removed extraneous '&' operator for Ultrix port.
 */



/************************************************************************
 *                                                                      *
 *      COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,            *
 *            1986.   ALL RIGHTS RESERVED.                              *
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

/*** DFEINITIONS ***/

#define SIXEL_BASELINE_OFFSET   700

#define ALLCLOSED       0
#define SIXELOPEN       1
#define SHOWOPEN        2
#define DECVECOPEN      3
#define FONTOPEN        4
#define SPACINGOPEN     5

#define BAREUNIT        0       /* Translator entry level */
#define PREAMBLE        1       /* Preamble level */
#define DLL_LEVEL       2       /* DLL loaded */
#define SPACED_LEVEL    3       /* SPACED loaded */
#define DEFINE_LEVEL    4       /* Define loaded */
#define PAGE            5       /* Page loaded */


/* DEFAULT VALUE */
/* >>>>>>>>>>>>>>>>>>>>>>>*** MAY NEED TO BE REDEFINED!!! */

#define XL_ST_DEFAULT_ORIGIN_X          0
#define XL_ST_DEFAULT_ORIGIN_Y          0
#define XL_ST_DEFAULT_ORIENTATION       ORIENT_PS

#define DEFAULT_SCALE_X                 1
#define DEFAULT_SCALE_Y                 1
#define DEFAULT_THICKNESS               100



/******************************************************************
M$SAVE()

       5.1.1  Generalities

       Outputs the "save" command in postscript and  pushes  the  current
       psstate table on the stack.


       5.1.2  Input

       1.  ps_state
       2.  stack pointer


       5.1.3  Output

       1.  str_save
       2.  stack_ptr


       5.1.4  Called Routines

       1.  oprintf


       5.1.5  Internal Variables And Storage

       *** (T.B.D.) ***


       5.1.6  Code Comments

       1.  Send "save" command
       2.  Push current ps_state
           1.  Increment current stack pointer
           2.  Copy ps_state to new current location on stack
******************************************************************/


#define M$SAVE() \
 \
/* Output  PS command */ \
 \
ps_str (str_save); \
 \
/* Puts ps_st onto stack */ \
 \
old_ps_stack_ptr = stack_ptr;	    /* Save old stack pointer */ \
stack_ptr++;			    /* Increment stack pointer */ \
cur_ps_st = &ps_st[stack_ptr];	    /* Update cur_ps_st */ \
 \
/* Copy last ps_st into top of stack */ \
ps_st[stack_ptr] = ps_st[old_ps_stack_ptr]

/******************************************************************
M$RESTORE()

       5.2.1  Generalities

       Outputs the "restore" command and decrements the stack pointer  to
       previous states.


       5.2.2  Input

       None


       5.2.3  Output

       1.  str_restore - PS "restore" command
       2.  stack_ptr - stack pointer


       5.2.4  Called Routines

       1.  oprintf


       5.2.5  Internal Variables And Storage

       None


       5.2.6  Code Comments

       1.  prints "restore" command
       2.  Decrement stack pointer

******************************************************************/

#define M$RESTORE() \
 \
/* Output PS command and decrement stack pointer */ \
 \
ps_str (str_restore); \
cur_ps_st = &ps_st[--stack_ptr]; 



/******************************************************************
M$UPDATE_PAIRED_FONTDICT() 
       
       5.35.1  Generalities


       5.35.2  Input


       5.35.3  Output


       5.35.4  Called Routines


       5.35.5  Internal Variables And Storage


       5.35.6  Code Comments
******************************************************************/
#define M$UPDATE_PAIRED_FONTDICT() \
 \
/* STUB */ 



/******************************************************************
M$UPDATE_BUFF_SIZE(m$sixel_buf_size) 

       5.32.1  Generalities

       This will store the old sixel count.


       5.32.2  Input


       5.32.3  Output


       5.32.4  Called Routines

           None


       5.32.5  Internal Variables And Storage


******************************************************************/       
/*
 * #define M$UPDATE_BUFF_SIZE(m$sixel_buf_size) \
 * \
 * cur_ps_st -> sixel_buf_size = m$sixel_buf_size; 
 */



/******************************************************************
M$UPDATE_SIXEL()

       5.31.1  Generalities

       This will store the old sixel count and update ap.


       5.31.2  Input


       5.31.3  Output


       5.31.4  Called Routines

           None


       5.31.5  Internal Variables And Storage


******************************************************************/       
#define M$UPDATE_SIXEL(m$buff_size) \
 \
       cg_st.cgst_wpf = TRUE; \
       cur_ps_st -> scale_factor.xval = scalex; \
       cur_ps_st -> scale_factor.yval = scaley; \
       cur_ps_st -> sixel_buf_size = m$buff_size; 



/******************************************************************
M$UPDATE_SHOWPAGE()

       5.30.1  Generalities

       This will close all open elements and resets all related states to
       postscript defaults.

           i.e.
           Origin goes back to lower left corner of page.
                  ^
                 y|
                  +--->
                    x

           Orientation
                  ^
                 y|
                  +--->
                    x

           Scale goes back to 1/72" per pixel.


       5.30.2  Input

       None


       5.30.3  Output

       None


       5.30.4  Called Routines

           None


       5.30.5  Internal Variables And Storage

******************************************************************/       

#define M$UPDATE_SHOWPAGE() \
 \
cg_st.cgst_wpf = FALSE; \
 \
 \
cur_ps_st -> curchar.ap.xval        = -1; \
cur_ps_st -> curchar.ap.yval        = -1; \
 \
cur_ps_st -> origin.xval  = XL_ST_DEFAULT_ORIGIN_X; \
cur_ps_st -> origin.yval  = XL_ST_DEFAULT_ORIGIN_Y; \
 \
cur_ps_st -> orientation  = XL_ST_DEFAULT_ORIENTATION; \
cur_ps_st -> sheet_len    = 0; \
 \
cur_ps_st -> scale_factor.xval  = DEFAULT_SCALE_X; \
cur_ps_st -> scale_factor.yval  = DEFAULT_SCALE_Y; \
 \
cur_ps_st -> line_thickness    = DEFAULT_THICKNESS; \
 \
cur_ps_st -> open    = ALLCLOSED;  /* This is not needed since \
                              everything is already closed */ 



/******************************************************************
M$UPDATE_AP_A(m$x,m$y)

       5.29.1  Generalities

       This updates active position absolute into the ps_state table


       5.29.2  Input

       desired ap


       5.29.3  Output

       ps_state.ap


       5.29.4  Called Routines

           None


       5.29.5  Internal Variables And Storage


******************************************************************/

#define M$UPDATE_AP_A(m$x,m$y) \
 \
       /* add the change in active position */ \
 \
       cur_ps_st -> curchar.ap.xval = (m$x); \
       cur_ps_st -> curchar.ap.yval = (m$y); 



/******************************************************************
M$UPDATE_AP_R(m$x)

       5.28.1  Generalities

       This updates active position relative into the ps_state table


       5.28.2  Input

       desired ap


       5.28.3  Output

       ps_state.ap


       5.28.4  Called Routines

           None


       5.28.5  Internal Variables And Storage


******************************************************************/       

#define M$UPDATE_AP_R(m$x) \
 \
       /* add the change in active position */ \
 \
       cur_ps_st -> curchar.ap.xval += (m$x); 



/******************************************************************
M$OUTPUT_SHOWPAGE()

       5.26.1  Generalities

       The postscript command is outputted here.


       5.26.2  Input


       5.26.3  Output

       showpage


       5.26.4  Called Routines

       1.  restore
       2.  save
       3.  oprintf


       5.26.5  Internal Variables And Storage

******************************************************************/
#define M$OUTPUT_SHOWPAGE() \
 \
       ps_str (str_showpage); \

/* \
 *       M$SAVE() \
 *      ps_str (str_scale); \
 */ 



/******************************************************************
M$OUTPUT_VCHAR(m$attrs,m$width,m$drawerase,m$ulavp)

       5.25.1  Generalities

       This routine  prints  one  virtual  character  to  the  Postscript
       machine.


       5.25.2  Input

       vchar


       5.25.3  Output

       vchar


       5.25.4  Called Routines

       1.  oprintf
       2.  line_ul
       3.  line_st
       4.  line_stul


       5.25.5  Internal Variables And Storage

******************************************************************/

#define M$OUTPUT_VCHAR(m$attrs,m$width,m$drawerase,m$ulavp) \
 \
/*if (m$drawerase == VIR_CHAR_ERASE) oprintf (str_white);*/ /* set white ink*/ \
/*else oprintf (str_black);*/ /*set black ink*/ \
\
oprintf (str_black); /*set black ink*/\
\
switch (m$attrs & (UL | STRIKE | DOU_UL | OVERLINE))\
     {\
     case UL                            : oprintf (str_line_ul, m$width, m$ulavp); break;\
     case OVERLINE                      : oprintf (str_line_ol, m$width, m$ulavp); break;\
     case (DOU_UL | UL)                 :\
     case DOU_UL                        : oprintf (str_line_dul, m$width, m$ulavp); break;\
     case STRIKE                        : oprintf (str_line_st, m$width, m$ulavp); break;\
     case (UL | STRIKE)                 : oprintf (str_line_stul, m$width, m$ulavp); break;\
     case (OVERLINE | STRIKE)           : oprintf (str_line_stol, m$width, m$ulavp); break;\
     case (DOU_UL | STRIKE | UL)        :\
     case (DOU_UL | STRIKE)             : oprintf (str_line_stdul, m$width, m$ulavp); break;\
     case (UL | OVERLINE)               : oprintf (str_line_ulol, m$width, m$ulavp); break;\
     case (DOU_UL | OVERLINE | UL)      :\
     case (DOU_UL | OVERLINE)           : oprintf (str_line_dulol, m$width, m$ulavp); break;\
     case (DOU_UL | OVERLINE | STRIKE | UL):\
     case (UL | OVERLINE | STRIKE)      : oprintf (str_line_stulol, m$width, m$ulavp); break;\
     case (DOU_UL | OVERLINE | STRIKE)  : oprintf (str_line_stdulol, m$width, m$ulavp); break;\
                break;\
     }\
oprintf (str_black); /*set black ink*/ 



/******************************************************************
M$OUTPUT_FONT(m$source_buffer,m$lrhalf) 


       5.21.1  Input


       5.21.2  Output

       None


       5.21.3  Called Routines

           None


       5.21.4  Internal Variables And Storage


       5.21.5  Error Handling

       None


******************************************************************/

/* WORD
 *      source_buffer,
 *      lrhalf;   
 */

#define M$OUTPUT_FONT(m$source_buffer,m$lrhalf) \
 \
/* LONG m$i;  */ \
 \
 \
/*       for (m$i = BIGIN;  m$i =< END;  m$i++) */ \
            { \
            /* Get data from Nick's stuff */ \
 \
            /* Put data to Mark's stuff   */ \
            } 


/******************************************************************
M$CLOSE_DECVEC()

       5.19.1  Generalities

       This will clear the decvec flag and restores the psstate.


       5.19.2  Input

       1.  ps_state.decvec
       2.  ap


       5.19.3  Output

       1.  ps_state.decvec


       5.19.4  Called Routines

       1.  restore


       5.19.5  Internal Variables And Storage

******************************************************************/

#define M$CLOSE_DECVEC() \
 \
cur_ps_st -> open = ALLCLOSED; \
/* ps_str (str_closepath);  */ \
ps_str (str_stroke); \
 \
M$RESTORE(); \


/******************************************************************
M$OPEN_DECVEC()

       5.18.1  Generalities

       Sets the decvec flag and saves ps_state.


       5.18.2  Input

       1.  ps_state.decvec
       2.  ap


       5.18.3  Output

       1.  ps_state.decvec


       5.18.4  Called Routines

       1.  save


       5.18.5  Internal Variables And Storage

******************************************************************/

#define M$OPEN_DECVEC() \
 \
M$SAVE(); \
/* ps_str (str_newpath);  */ \
cur_ps_st -> open = DECVECOPEN;  /* not needed in the code */ 


/******************************************************************
M$CLOSE_SIXEL()

       5.17.1  Generalities

       This clears the sixel flag and restores ps_state


       5.17.2  Input

       ps_state.sixel


       5.17.3  Output

       ps_state.sixel


       5.17.4  Called Routines

       1.  restore


       5.17.5  Internal Variables And Storage

******************************************************************/

#define M$CLOSE_SIXEL() \
 \
cur_ps_st -> open = ALLCLOSED;  /* not needed in code */ \
M$RESTORE(); \


/******************************************************************
M$OPEN_SIXEL()

       5.16.1  Generalities

       Set sixel flag to open and saves ps_state.


       5.16.2  Input

       1.  psstate.sixel
       2.  count
       3.  ap


       5.16.3  Output

       1.  ps_state.sixel
       2.  prints (str_sixel_preamble)


       5.16.4  Called Routines

       1.  save
       2.  output_sixel


       5.16.5  Internal Variables And Storage

******************************************************************/

/*       print (str_sixel_preamble) */

#define M$OPEN_SIXEL() \
 \
M$SAVE(); \
cg_st.cgst_first_row = TRUE; \
cur_ps_st -> open = SIXELOPEN;  /* set sixel flag to open */ 


/******************************************************************
M$REPLACE(m$box_number)

******************************************************************/

/* WORD box_number;  */

#define M$REPLACE(m$box_number) \
 \
/*** stub ***/ 


/******************************************************************
M$OPEN_SPACING()
******************************************************************/
#define M$OPEN_SPACING() \
 \
cur_ps_st -> open = SPACINGOPEN;  /* The state that was restored \
                                      better be all closed or else \
                                      MERDDE!!! */ \
while (stack_ptr > SPACED_LEVEL) { M$RESTORE(); } \


/******************************************************************
M$CLOSE_SPACING()
******************************************************************/
#define M$CLOSE_SPACING() \
 \
M$SAVE(); \
cur_ps_st -> open = ALLCLOSED; \

/* ps_str (str_scale); */

/******************************************************************
M$OPEN_FONT() 

       5.15.1  Generalities


       5.15.2  Input


       5.15.3  Output


       5.15.4  Called Routines

       1.  restore
       2.  replace


       5.15.5  Internal Variables And Storage

******************************************************************/

#define M$OPEN_FONT() \
 \
cur_ps_st -> open = FONTOPEN;  /* The state that was restored \
                                      better be all closed or else \
                                      MERDDE!!! */ \
while (stack_ptr > DLL_LEVEL) { M$RESTORE(); } \


/******************************************************************
M$CLOSE_FONT() 

       5.14.1  Generalities


       5.14.2  Input


       5.14.3  Output


       5.14.4  Called Routines

       1.  save
       2.  get_box
       3.  output_box


       5.14.5  Internal Variables And Storage

******************************************************************/

/* WORD box_number;  */

#define M$CLOSE_FONT() \
 \
M$SAVE(); \
M$SAVE(); \
/* get_box (); */ \
cur_ps_st -> open = ALLCLOSED; 




/******************************************************************
M$CLOSE_SHOW()
******************************************************************/

#define M$CLOSE_SHOW() \
\
ps_char ('\)'); \
if (cur_ps_st -> curchar.font_data.algorithmic_attributes & (UL | OVERLINE | DOU_UL)) \
oprintf (" %d", (cur_ps_st -> curchar.attr_data.attr_baseline_offset - \
	cur_ps_st -> curchar.ap.yval)); \
\
ps_str (close_show_str [(cur_ps_st -> curchar.font_data.algorithmic_attributes & (UL | OVERLINE | DOU_UL | ITALIC | BOLD | STRIKE))]); \
M$RESTORE();    /*    ps_str (str_restore);  */ \
/* update_show ();  */ \
cur_ps_st -> open = ALLCLOSED; 


/******************************************************************
M$OPEN_SHOW()

       5.12.1  Generalities

       This routine is used by ALL shows.


       5.12.2  Input


       5.12.3  Output


       5.12.4  Called Routines

       1.  close_all
       2.  output_show
       3.  update_show


       5.12.5  Internal Variables And Storage


       *** (Don't forget to add to xlv__state module) ***

       str_open_show [] = {"("}; 

******************************************************************/

#define M$OPEN_SHOW(m$attrs) \
 \
M$SAVE(); \
ps_str (str_open_show); \
/* cur_ps_st -> curchar.font_data.algorithmic_attributes = (SHOWOPEN | (m$attrs & (UL | OVERLINE | DOU_UL | ITALIC | BOLD | STRIKE))); */ \
cur_ps_st -> curchar.font_data.algorithmic_attributes = (m$attrs ); \
cur_ps_st -> open = SHOWOPEN; 


/******************************************************************
M$CLOSE_ALL()

       5.20.1  Generalities


       5.20.2  Input


       5.20.3  Output


       5.20.4  Called Routines

           None


       5.20.5  Internal Variables And Storage

******************************************************************/

#define M$CLOSE_ALL() \
 \
if (cur_ps_st -> open != ALLCLOSED) \
 \
     switch (cur_ps_st -> open) \
          { \
          case SHOWOPEN   : M$CLOSE_SHOW(); break; \
          case FONTOPEN   : M$CLOSE_FONT(); break; \
          case SPACINGOPEN : M$CLOSE_SPACING(); break; \
          case DECVECOPEN : M$CLOSE_DECVEC(); break; \
          case SIXELOPEN  : M$CLOSE_SIXEL(); break; \
                            break; \
          } 


/******************************************************************
M$SET_OPEN(m$open,m$x) 

       5.6.1  Generalities


       5.6.2  Input

       ap


       5.6.3  Output


       5.6.4  Called Routines

       1.  close_all
       2.  open_decvec
       3.  open_show
       4.  open_sixel
       5.  open_font


       5.6.5  Internal Variables And Storage

******************************************************************/

#define M$SET_OPEN(m$open,m$x) \
 \
desired_st.open = m$open; \
switch (desired_st.open) \
     { \
     case ALLCLOSED:  break; \
     case DECVECOPEN: M$OPEN_DECVEC();  break; \
     case SHOWOPEN:   M$OPEN_SHOW(m$x);  break; \
     case SIXELOPEN:  M$OPEN_SIXEL();  break; \
     case FONTOPEN:   M$OPEN_FONT();  break; \
     case SPACINGOPEN: M$OPEN_SPACING();  break; \
                      break; \
     } 


/******************************************************************
M$SET_BUFF_SIZE(m$sixel_buf_size) 

       5.11.1  Generalities


       5.11.2  Input


       5.11.3  Output


       5.11.4  Called Routines

       1.  close_all
       2.  output_buff_size
       3.  update_buff_size


       5.11.5  Internal Variables And Storage

       None

******************************************************************/

#define M$SET_BUFF_SIZE(m$sixel_buf_size) \
 \
oprintf (str_sixel_data_info_change, m$sixel_buf_size, m$sixel_buf_size << 3); \
cur_ps_st -> sixel_buf_size = m$sixel_buf_size; 



/******************************************************************
M$SET_THICKNESS(m$thichness) 
 
       5.10.1  Generalities

       This sends the command to change thickness of line drawing.


       5.10.2  Input

       ps_state.line_thickness


       5.10.3  Output


       5.10.4  Called Routines

       1.  close_all
       2.  output_thickness


       5.10.5  Internal Variables And Storage

       None

******************************************************************/
#define M$SET_THICKNESS(m$thickness) \
 \
oprintf (str_setlinewidth, m$thickness); \
cur_ps_st -> line_thickness = m$thickness; 



/******************************************************************
M$SET_SCALE(m$xval,m$yval) 
 
       5.10.1  Generalities

       This sends the scale command.


       5.10.2  Input

       ps_state.scale


       5.10.3  Output

       prints (desired_st.scale.x/ps_state.scale.x, 
               desired_st.scale.y/ps_state.scale.y, " scale")


       5.10.4  Called Routines

       1.  close_all
       2.  output_scale
       3.  update_scale


       5.10.5  Internal Variables And Storage

       None

******************************************************************/
#define M$SET_SCALE(m$xval,m$yval) \
 \
 \
/* output_scale */ \
 \
        scalex = (((float)(m$xval)) / (float)cur_ps_st -> scale_factor.xval); \
        scaley = (((float)(m$yval)) / (float)cur_ps_st -> scale_factor.yval); \
 \
sprintf (str_buffer,"%.3f %.3f%s",scalex,scaley,str_float_scale); \
ps_str (str_buffer); \
 \
/* update_scale */ \
cur_ps_st -> scale_factor.xval = m$xval; \
cur_ps_st -> scale_factor.yval = m$yval; 



/******************************************************************
M$SET_ORIENT(m$orientation) 

       5.9.1  Generalities

       This set either portrait or landscape mode

       Portrait:     x     Landscape:   ^       PS:   ^
                   +--->               x|            y|
                  y|                    +--->         +--->
                   v                      y             x

       5.9.2  Input

       1.  orientation type
       2.  ps_state.orient


       5.9.3  Output

       1.  print ("90 rotate")
       2.  print ("-90 rotate")
       3.  print ("+1 -1 scale)

       5.9.4  Called Routines

       1.  close_all
       2.  set_scale
       3.  oprintf


       5.9.5  Internal Variables And Storage
******************************************************************/
#define M$SET_ORIENT(m$orientation) \
 \
       /* SET THE ORIENTATION */ \
  \
            /* POSTSCRIPT TO ??? */ \
            if (cur_ps_st -> orientation == ORIENT_PS) \
                 { \
                 /* IF PORTRAIT ELSE LANDSCAPE */ \
                 if (m$orientation == ORIENT_PORT) \
                    oprintf (str_ps_port,cg_st.cgst_sheet_len); \
                 else ps_str (str_ps_land); \
                 } \
            else \
                 { \
                 /* PORTRAIT TO LANDSCAPE? */ \
                 if ((cur_ps_st -> orientation == ORIENT_PORT) && \
                     (m$orientation == ORIENT_LAND)) \
                     oprintf (str_port_land, -cur_ps_st -> origin.xval, \
                          cur_ps_st -> sheet_len - cur_ps_st -> origin.yval); \
                 else \
                 /* LANDSCAPE TO PORTRAIT? */ \
                 if ((cur_ps_st -> orientation == ORIENT_LAND) && \
                     (m$orientation == ORIENT_PORT)) \
                     { \
                     oprintf (str_translate, \
                          -cur_ps_st -> origin.xval, -cur_ps_st -> origin.yval); \
                     oprintf (str_land_port, cg_st.cgst_sheet_len); \
                     } \
                 } \
            /* WHATEVER TO POSTSCRIPT - should never happen */ \
 \
      /* update oreientation and sheet length */ \
      cur_ps_st -> sheet_len = cg_st.cgst_sheet_len; \
      cur_ps_st -> orientation = m$orientation; \
      cur_ps_st -> origin.xval = 0; \
      cur_ps_st -> origin.yval = 0; 

/******************************************************************
M$SET_ORIGIN(m$xval,m$yval) 

       5.8.1  Generalities

       Do a translate command.  For sixel origin the origin is  ap.   For
       non  sixel  origin  the  origin  is  xlstate.origin.  But, in both
       cases, origin will be placed in desired origin.


       5.8.2  Input

       1.  ps_state.origin
       2.  desired origin


       5.8.3  Output

       1.  (desired.origin.x - ps_state.origin.x)
       2.  (desired.origin.y - ps_state.origin.y)
       3.  (str_translate)


       5.8.4  Called Routines

       1.  close_all
       2.  output_origin
       3.  update_origin


       5.8.5  Internal Variables And Storage


******************************************************************/


#define M$SET_ORIGIN(m$xval,m$yval) \
 \
oprintf (str_translate, (m$xval) - cur_ps_st -> origin.xval, \
        (m$yval) - cur_ps_st -> origin.yval); \
cur_ps_st -> origin.xval = m$xval; \
cur_ps_st -> origin.yval = m$yval; 



/******************************************************************
M$SET_AP(m$ahp,m$avp)
******************************************************************/

#define M$SET_AP(m$ahp,m$avp) \
 \
oprintf (str_moveto,m$ahp, m$avp); \
M$UPDATE_AP_A(m$ahp, m$avp); 




/******************************************************************
M$CHECK_STATE_CHANGE()

       5.3.1  Generalities

           "Do the Job" in check_state_change.   Affects  the  use  of  a
           global  "possible state change" flag which could be set by any
           escape sequence action  routines  which  know  that  they  are
           changing the state.


       5.3.2  Input

       1.  ps_state


       5.3.3  Output


       5.3.4  Called Routines

       1.  set_origin
       2.  set_orient
       3.  set_ap
       4.  set_font
       5.  set_spacing
       6.  set_buff_size
       7.  set_thickness
       8.  set_scale
       9.  set_open


       5.3.5  Internal Variables And Storage
******************************************************************/

/*--------------------------------------------------------------------*/
/* Note - The order in which the tests for state change is important! */
/*        The following list shows the sequence that each test must   */
/*        done in:                                                    */
/*              PAIRED_FONT_NUMBER      1st <-+- These 2 must occur   */
/*              HORIZONTAL_SPACING      2nd <-!  in togther!          */
/*              SCALE                   3rd                           */
/*              ORIENTATION & SHEET LEN 4th                           */
/*              ORIGIN                  5th                           */
/*              AP                      6th                           */
/*              ATTRS                   7th                           */
/*              SIXEL_BUF_SIZE          anywhere                      */
/*              LINE_THICKNESS          anywhere after the SCALE      */
/*              OPEN                    last at all times             */
/*--------------------------------------------------------------------*/
#define M$CHECK_STATE_CHANGE_ALL() \
 \
if (\
    (vax_font_table_box_number [desired_st.curchar.char_data.char_font]\
                        != cur_ps_st -> curchar.char_data.char_font) || \
    (desired_st.curchar.font_data.horizontal_spacing \
                        != cur_ps_st -> curchar.font_data.horizontal_spacing) || \
    (desired_st.scale_factor.xval != cur_ps_st -> scale_factor.xval) || \
    (desired_st.scale_factor.yval != cur_ps_st -> scale_factor.yval) || \
    (desired_st.orientation       != cur_ps_st -> orientation)       || \
    (desired_st.sheet_len         != cur_ps_st -> sheet_len)         || \
    (desired_st.origin.xval       != cur_ps_st -> origin.xval)       || \
    (desired_st.origin.yval       != cur_ps_st -> origin.yval)       || \
    (desired_st.curchar.ap.xval       != cur_ps_st -> curchar.ap.xval)       || \
    (desired_st.curchar.ap.yval       != cur_ps_st -> curchar.ap.yval)       || \
    (desired_st.curchar.attr_data.attr_baseline_offset       != cur_ps_st -> curchar.attr_data.attr_baseline_offset)       || \
    (desired_st.curchar.font_data.algorithmic_attributes     != cur_ps_st -> curchar.font_data.algorithmic_attributes)     || \
    (desired_st.sixel_buf_size    != cur_ps_st -> sixel_buf_size)    || \
    (desired_st.line_thickness    != cur_ps_st -> line_thickness)    || \
    (desired_st.open              != cur_ps_st -> open)) 

#define M$CHECK_STATE_CHANGE_FONT(m$paired_font_number, m$horizontal_spacing) \
 \
    if  ((vax_font_table_box_number [m$paired_font_number]\
                        != cur_ps_st -> curchar.char_data.char_font) ||\
         (m$horizontal_spacing!=cur_ps_st->curchar.font_data.horizontal_spacing))\
         { \
         set_font_and_spacing(m$paired_font_number, m$horizontal_spacing); \
         } \


#define M$CHECK_STATE_CHANGE_SCALE(m$xval,m$yval) \
 \
/* 3 - SCALE */ \
 \
if ((m$xval != cur_ps_st -> scale_factor.xval) || \
    (m$yval != cur_ps_st -> scale_factor.yval)) \
     { \
     M$SET_SCALE(m$xval,m$yval); \
     } 


#define M$CHECK_STATE_CHANGE_ORIENT(m$orientation,m$sheet_len) \
 \
/* 4 - ORIENTATION */ \
 \
if (m$orientation != cur_ps_st -> orientation) \
     { \
     M$SET_ORIENT(m$orientation); \
     } \
else if ((m$sheet_len   != cur_ps_st -> sheet_len) && \
         (ORIENT_LAND   != cur_ps_st -> orientation)) \
     { \
     oprintf (str_translate, 0, cur_ps_st -> sheet_len - m$sheet_len); \
     cur_ps_st -> sheet_len = m$sheet_len; \
     }

#define M$CHECK_STATE_CHANGE_ORIGIN(m$xval,m$yval) \
 \
/* 5 - ORIGIN */ \
 \
if ((m$xval != cur_ps_st -> origin.xval) || \
    (m$yval != cur_ps_st -> origin.yval)) \
     { \
     M$SET_ORIGIN(m$xval,m$yval); \
     M$UPDATE_AP_A(-1,-1); \
     } 


#define M$CHECK_STATE_CHANGE_AP(m$ahp,m$avp) \
 \
/* 6 - AP */ \
 \
if ((m$ahp != cur_ps_st -> curchar.ap.xval) || \
    (m$avp != cur_ps_st -> curchar.ap.yval)) \
     { \
     M$SET_AP(m$ahp,m$avp); \
     } 


#define M$CHECK_STATE_CHANGE_ATTRS(m$attrs,m$ulavp) \
 \
/* 7 - ATTRS */ \
 \
if ((m$attrs != cur_ps_st -> curchar.font_data.algorithmic_attributes) || \
    (m$ulavp != cur_ps_st -> curchar.attr_data.attr_baseline_offset)) \
   { \
     cur_ps_st -> curchar.font_data.algorithmic_attributes = m$attrs; \
     cur_ps_st -> curchar.attr_data.attr_baseline_offset = m$ulavp; \
   }


#define M$CHECK_STATE_CHANGE_BUFF(m$sixel_buf_size) \
 \
/* 8 - SIXEL_BUF_SIZE (This can go anywhere, order is not important here) */ \
 \
if (m$sixel_buf_size != cur_ps_st -> sixel_buf_size) \
     { \
     M$SET_BUFF_SIZE(m$sixel_buf_size); \
     } 


#define M$CHECK_STATE_CHANGE_THICK(m$line_thickness) \
 \
/* 9 - LINE_THICKNESS (This can go anywhere after scale) */ \
 \
if (m$line_thickness != cur_ps_st -> line_thickness) \
     { \
     M$SET_THICKNESS(m$line_thickness); \
     } 


#define M$CHECK_STATE_CHANGE_OPEN(m$open,m$x) \
 \
/* 10 - OPEN (Must be done last!) */ \
 \
if (m$open != cur_ps_st -> open) \
     { \
     M$SET_OPEN(m$open,m$x); \
     } 


