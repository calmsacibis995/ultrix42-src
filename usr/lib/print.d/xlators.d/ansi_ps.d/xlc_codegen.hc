/*        @(#)xlc_codegen.hc	4.1      7/2/90      */

/*  Version 2.00   4/ 3/89 13:54:00.0000  */
/* makehf /hc /cp /setup=portab.h,capdl.def xlc_codegen.def xlc_codegen.hc  */





struct xlc_codegen_DSTR_23
{
	POINT cgst_origin;
	LONG cgst_sheet_len;
	ORIENTATION cgst_orientation;
	BYTE cgst_output_mode;
	BYTE cgst_image_mode;
	BYTE (* cgst_output_sixel_ptr) ();
	LONG cgst_vm_size;
	LONG cgst_vm_available;
	LONG cgst_vm_downloaded_used;
	LONG cgst_vm_spaced_used;
	BOOLEAN cgst_last_showpage;
	WORD cgst_paper_tray;
	POINT cgst_sixel_last_ap;
	POINT cgst_sixel_size_cpt;
	WORD cgst_rightmost_pixel;
	BOOLEAN cgst_sixel_buffer_empty;
	WORD cgst_pg_copies;
	BOOLEAN cgst_wpf;
	BOOLEAN cgst_first_row;
	ITEM * cgst_ilistptr;
	BYTE reset_entry;
	BYTE reset_exit;
	BYTE print_prologue;
	WORD cgst_next_element_for_swap;
	WORD cgst_item_list_sgr;
	CPINIT_STATE cpinit_vals;
};
typedef struct xlc_codegen_DSTR_23 CODEGEN_STATE;
NOSHARE extern CODEGEN_STATE cg_st;
#define 	PAPER_TRAY_UNDEF	 (-1)
#define 	DECBIND	 (1)
#define 	DECBIND_NULL	 (0)
struct xlc_codegen_DSTR_24
{
	GLYPH curchar;
	POINT origin;
	POINT scale_factor;
	GLYPH_FONT_DATA font_dict;
	ORIENTATION orientation;
	LONG sheet_len;
	LONG line_thickness;
	LONG sixel_buf_size;
	LONG open;
	WORD psst_paper_tray;
};
typedef struct xlc_codegen_DSTR_24 PS_ST;
NOSHARE extern PS_ST ps_st[8];
NOSHARE extern PS_ST desired_st;
NOSHARE extern PS_ST * cur_ps_st;
NOSHARE extern float scalex;
NOSHARE extern float scaley;
NOSHARE extern LONG old_bitwidth;
NOSHARE extern LONG old_stp;
NOSHARE extern LONG stack_ptr;
NOSHARE extern LONG old_ps_stack_ptr;
NOSHARE extern BYTE str_buffer[128];
#define 	BLK_SIZE	 (122400)
NOSHARE extern UBYTE sixel_buffer[122400];
CONST extern LONG sxl_mac_tbl[10][3];

