/*        @(#)xlc_ps.hc	4.1      7/2/90      */
/* file:    xlc_ps.h- include file for external references to 
 *	    Variable declarations needed by codegen 
 *
 *	    This is a temporary file, until tools can handle long strings
 *
 * edit:	araj	18-AUG-1988 14:57
 *			creation 
 *
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



CONST EXTERNAL BYTE str_font_struct [];
CONST EXTERNAL  BYTE str_next_line [];
CONST EXTERNAL  BYTE str_save [];
CONST EXTERNAL  BYTE str_show [];
CONST EXTERNAL  BYTE str_end [];
CONST EXTERNAL  BYTE str_float_scale [];
CONST EXTERNAL  BYTE str_new_scale [];
CONST EXTERNAL  BYTE str_scale [];
CONST EXTERNAL  BYTE str_trns_on [];
CONST EXTERNAL  BYTE str_courierfind [];
CONST EXTERNAL  BYTE str_sixelfontfind [];
CONST EXTERNAL  BYTE str_showpage [];
CONST EXTERNAL  BYTE str_restore [];
CONST EXTERNAL  BYTE str_spacing [];
CONST EXTERNAL  BYTE str_lineto [];
CONST EXTERNAL  BYTE str_newpath [];
CONST EXTERNAL  BYTE str_closepath [];
CONST EXTERNAL  BYTE str_stroke [];
CONST EXTERNAL  BYTE str_copy_font [];
CONST EXTERNAL  BYTE str_restore_moveto [];
CONST EXTERNAL  BYTE str_moveto [];
CONST EXTERNAL  BYTE str_translate [];
CONST EXTERNAL  BYTE str_rotate [];
CONST EXTERNAL  BYTE str_setlinewidth [];
CONST EXTERNAL  BYTE str_setpapertray [];
CONST EXTERNAL  BYTE str_exec_newsheet [];
CONST EXTERNAL  BYTE str_finddefpapertray [];
CONST EXTERNAL  BYTE str_resdefpapertray [];
CONST EXTERNAL  BYTE str_open_show [];
CONST EXTERNAL  BYTE str_open_proc [];
CONST EXTERNAL  BYTE str_close_proc [];
CONST EXTERNAL  BYTE str_decbind [];
CONST EXTERNAL  BYTE str_decbind_null [];
CONST EXTERNAL  BYTE str_setpacking [];
CONST EXTERNAL  BYTE str_setcachelimit [];
CONST EXTERNAL  BYTE str_time_date_and_version [];
CONST EXTERNAL  BYTE *close_show_str[64];
CONST EXTERNAL  BYTE str_save_open_show [];
CONST EXTERNAL  BYTE str_preamble_show [];
CONST EXTERNAL  BYTE str_preamble_vchar [];
CONST EXTERNAL  BYTE str_preamble_fonts [];
CONST EXTERNAL  BYTE str_define_spaced_font [];
CONST EXTERNAL  BYTE str_define_dll_font [];
CONST EXTERNAL 	BYTE str_set_box [];
CONST EXTERNAL  BYTE str_start_font [];
CONST EXTERNAL  BYTE str_fontmatrix [];
CONST EXTERNAL  BYTE str_imagemaskmatrix [];
CONST EXTERNAL  BYTE str_start_bitmaps [];
CONST EXTERNAL  BYTE str_blob_ref [];
CONST EXTERNAL  BYTE str_char_replace [];
CONST EXTERNAL  BYTE str_finish_bitmap [];
CONST EXTERNAL  BYTE str_ul_thickness [];
CONST EXTERNAL  BYTE str_ul_position [];
CONST EXTERNAL  BYTE str_ol_thickness [];
CONST EXTERNAL  BYTE str_ol_position [];
CONST EXTERNAL  BYTE str_strikethru_thickness [];
CONST EXTERNAL  BYTE str_strikethru_position [];
CONST EXTERNAL  BYTE str_white [];
CONST EXTERNAL  BYTE str_black [];
CONST EXTERNAL  BYTE str_line_ul [];
CONST EXTERNAL  BYTE str_line_ol [];
CONST EXTERNAL  BYTE str_line_dul [];
CONST EXTERNAL  BYTE str_line_st [];
CONST EXTERNAL  BYTE str_line_stul [];
CONST EXTERNAL  BYTE str_line_stol [];
CONST EXTERNAL  BYTE str_line_stdul [];
CONST EXTERNAL  BYTE str_line_ulol [];
CONST EXTERNAL  BYTE str_line_dulol [];
CONST EXTERNAL  BYTE str_line_stulol [];
CONST EXTERNAL  BYTE str_line_stdulol [];
CONST EXTERNAL  BYTE str_ps_port [];
CONST EXTERNAL  BYTE str_ps_land [];
CONST EXTERNAL  BYTE str_port_land [];
CONST EXTERNAL  BYTE str_land_port [];
CONST EXTERNAL  BYTE str_sixel_new_line [];
CONST EXTERNAL  BYTE str_sixel_data_bin [];
CONST EXTERNAL  BYTE str_sixel_data_hex [];
CONST EXTERNAL  BYTE str_sixel_data_info_change [];
CONST EXTERNAL  BYTE str_prologue [];
