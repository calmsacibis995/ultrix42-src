/*        @(#)capdl.hc	4.1      7/2/90      */

/*  Version 2.00   4/ 3/89 13:54:00.0000  */
/* makehf /hc /cp /setup=portab.h /w=0 capdl.def capdl.hc  */

#define 	MAX_SXL_PARM	 (9)
#define 	CENTIPTS_PER_PIXEL	 (24)
#define 	QUARTER_INCH_PXL	 (75)
#define 	HALF_INCH_PXL	 (150)
#define 	QUARTER_INCH_CTPT	 (1800)
#define 	HALF_INCH_CTPT	 (3600)
#define 	EIGHT_INCHES	 (57600)
#define 	ELEVEN_INCHES	 (79200)
#define 	NO_ATTR	 (0)
#define 	ITALIC	 (1)
#define 	SLANT	 (2)
#define 	BOLD	 (2)
#define 	UL	 (4)
#define 	OVERLINE	 (8)
#define 	STRIKE	 (16)
#define 	DOU_UL	 (32)
#define 	SUPERSCR	 (64)
#define 	SUBSCR	 (128)
#define 	NO_BOLD	 (-3)
#define 	NO_SLANT	 (-3)
#define 	NO_ITALIC	 (-2)
#define 	NO_UL	 (-5)
#define 	NO_STRIKE	 (-17)
#define 	NO_DOU_UL	 (-33)
#define 	NO_OVERLINE	 (-9)
#define 	NO_SUPERSCR	 (-65)
#define 	NO_SUBSCR	 (-129)
#define 	SUP_OR_SUB	 (192)
#define 	ALL_ATTRS	 (55)
#define 	ALL_PR_ATTRS	 (200)
#define 	SHOW_OPEN	 (240)
#define 	NO_SHOW_OPEN	 (-241)
#define 	PROPORTIONAL_PITCH	 (0)
#define 	CPI_5	 (1440)
#define 	CPI_5_4	 (1333)
#define 	CPI_5_7	 (1263)
#define 	CPI_6	 (1200)
#define 	CPI_6_6	 (1090)
#define 	CPI_6_8	 (1056)
#define 	CPI_7_5	 (960)
#define 	CPI_8_25	 (872)
#define 	CPI_8_55	 (840)
#define 	CPI_9	 (800)
#define 	CPI_9_34	 (771)
#define 	CPI_10	 (720)
#define 	CPI_10_3	 (696)
#define 	CPI_12	 (600)
#define 	CPI_12_77	 (563)
#define 	CPI_13_2	 (545)
#define 	CPI_13_3	 (541)
#define 	CPI_13_6	 (528)
#define 	CPI_15	 (480)
#define 	CPI_16_5	 (436)
#define 	CPI_17_1	 (421)
#define 	CPI_18	 (400)
#define 	CPI_18_75	 (384)
#define 	MONO_SPACED_MODE	 (0)
#define 	SEMI_PROPORTIONAL_MODE	 (-1)
#define 	PROPORTIONAL_MODE	 (1)
#define 	VIR_CHAR_BLOB	 (29)
#define 	VIR_CHAR_ADD	 (31)
#define 	VIR_CHAR_SPACE	 (30)
#define 	MAX_CSET_SIZE	 (97)
#define 	MAX_BITMAP_SIZE	 (375)
#define 	FONT_FILE_ID_SIZE	 (31)

/*  ** WARNING File capdl.def Line 458 -
 *  Expression error - can't evaluate [PUB]
 */
#define 	NULL_BITMAP	 ((PUB)0)




struct capdl_DSTR_1
{
	LONG min;
	LONG max;
};
typedef struct capdl_DSTR_1 BOUND;
typedef BOUND * PBOUND;
struct capdl_DSTR_2
{
	LONG xval;
	LONG yval;
};
typedef struct capdl_DSTR_2 POINT;
typedef POINT * PTR_POINT;
typedef POINT * PTR_RATIO;
struct capdl_DSTR_3
{
	WORD above_baseline_offset;
	WORD below_baseline_offset;
	WORD algorithmic_attributes;
	WORD horizontal_spacing;
};
typedef struct capdl_DSTR_3 GLYPH_FONT_DATA;
struct capdl_DSTR_4
{
	WORD char_code;
	WORD char_font;
	LONG char_width;
};
typedef struct capdl_DSTR_4 GLYPH_CHAR_DATA;
struct capdl_DSTR_5
{
	LONG attr_baseline_offset;
	WORD attr_font;
};
typedef struct capdl_DSTR_5 GLYPH_ATTR_DATA;
struct capdl_DSTR_6
{
	GLYPH_CHAR_DATA char_data;
	GLYPH_FONT_DATA font_data;
	GLYPH_ATTR_DATA attr_data;
	POINT ap;
};
typedef struct capdl_DSTR_6 GLYPH;
typedef GLYPH * PGLYPH;
struct capdl_DSTR_7
{
	UWORD itlength;
	UWORD code;
	ULONG address;
};
typedef struct capdl_DSTR_7 ITEM;
typedef ITEM * PITEM;
typedef WORD GSET_NUMBER;
typedef WORD FONT_METRIC;
typedef WORD FONT_NUMBER;
typedef FONT_NUMBER * PTR_FN;
typedef WORD CHAR_INDEX;
#define 	FIDS_FONT_ID	 (31)
#define 	FIDS_FONT_12	 (12)
#define 	FIDS_FONT_16	 (16)
#define 	FIDS_TYPE_FAMILY	 (7)
#define 	FIDS_TYPE_SIZE	 (3)
#define 	FIDS_STYLE	 (2)
#define 	FIDS_ROTATION	 (2)
#define 	FIDS_CSET	 (3)
#define 	FIDS_CSUBSET	 (4)
#define 	FIDS_ENCODING	 (2)
typedef UBYTE FID_TYPE_FAMILY[7];
typedef UBYTE FID_TYPE_SIZE[3];
typedef UBYTE FID_STYLE[2];
typedef UBYTE FID_ROTATION[2];
typedef UBYTE FID_CSET[3];
typedef UBYTE FID_CSUBSET[4];
typedef UBYTE FID_ENCODING[2];
struct capdl_DSTR_8
{
	UBYTE fi_type_family[7];
	UBYTE fi_spacing;
	UBYTE fi_type_size[3];
	UBYTE fi_scale_factor;
	UBYTE fi_style[2];
	UBYTE fi_weight;
	UBYTE fi_proportion;
	UBYTE fi_rotation[2];
	UBYTE fi_cset[3];
	UBYTE fi_csubset[4];
	UBYTE fi_encoding[2];
	UBYTE fi_resolution;
	UBYTE fi_res1;
	UBYTE fi_res2;
	UBYTE fi_res3;
};
typedef struct capdl_DSTR_8 FONT_ID;
typedef FONT_ID * PTR_FONT_ID;
#define 	NO_DESIRED_PAIR	 (0)
#define 	DEC_MCS_PAIR	 (1)
#define 	ISO_LAT_PAIR	 (2)
#define 	DEC_TEC_PAIR	 (4)
#define 	DEC_PUB_PAIR	 (8)
struct capdl_DSTR_9
{
	FONT_NUMBER fdp_font_index;
	FONT_ID fdp_font_id;
	LONG fdp_horiz_prop_numerator;
	LONG fdp_horiz_prop_denominator;
	LONG fdp_font_weight;
	UBYTE fdp_scaleable_flag;
	UBYTE fdp_valid_blob_flag;
	UBYTE fdp_paired_font_flag;
};
typedef struct capdl_DSTR_9 FONT_DICT_PARAM;

/*  ** WARNING File capdl.def Line 661 -
 *  Expression error - can't evaluate [FONT_NUMBER]
 */
#define 	END_OF_DICTIONARY	 ((FONT_NUMBER)0xFFFF)
typedef FONT_DICT_PARAM * PTR_FDP;
typedef WORD SGR_NUMBER;

/*  ** WARNING File capdl.def Line 678 -
 *  Expression error - can't evaluate [SGR_NUMBER]
 */
#define 	SGR_END_OF_DICTIONARY	 ((SGR_NUMBER)0xFFFF)
typedef WORD SGR_TYPE;
struct capdl_DSTR_10
{
	SGR_NUMBER sgr_number;
	SGR_TYPE sgr_type;
	FONT_ID sgr_font_id;
};
typedef struct capdl_DSTR_10 SGR_ENTRY;
typedef SGR_ENTRY * PTR_SGR_ENTRY;
typedef WORD ERROR_NUMBER;
typedef WORD SEVERITY_LEVEL;
typedef UBYTE CHAR_CODE_MAP[128];
typedef FONT_NUMBER FONT_NUMBER_MAP[128];
typedef WORD NRCS_NUMBER;

/*  ** WARNING File capdl.def Line 719 -
 *  Expression error - can't evaluate [NRCS_NUMBER]
 */
#define 	NRCS_END_OF_DICTIONARY	 ((NRCS_NUMBER)0xFFFF)
struct capdl_DSTR_11
{
	CHAR_CODE_MAP xxchar_code;
	FONT_NUMBER_MAP xxchar_font;
};
typedef struct capdl_DSTR_11 PAIR_MAP_DATA;
struct capdl_DSTR_12
{
	UBYTE cs_cset[3];
	UBYTE cs_subset[4];
	UBYTE cs_sup;
};
typedef struct capdl_DSTR_12 CSET_ID;
struct capdl_DSTR_13
{
	NRCS_NUMBER nrcs_number;
	CSET_ID nrcs_nrcs_id;
	CSET_ID nrcs_cset1_id;
	CSET_ID nrcs_cset2_id;
	PAIR_MAP_DATA nrcs_pair_map;
};
typedef struct capdl_DSTR_13 NRCS_ENTRY;
typedef NRCS_ENTRY * PTR_NRCS_ENTRY;
typedef UBYTE COLOR_NUMBER;
struct capdl_DSTR_14
{
	UBYTE coordsys;
	WORD hue_angle;
	WORD lightness;
	WORD saturation;
};
typedef struct capdl_DSTR_14 COLOR_DEFN;
struct capdl_DSTR_15
{
	COLOR_NUMBER color_map_entry;
	COLOR_DEFN color_defn;
};
typedef struct capdl_DSTR_15 COLOR;
typedef COLOR * PCOLOR;
struct capdl_DSTR_16
{
	UBYTE sixel_code;
	COLOR_NUMBER sixel_color;
	WORD sixel_repeat;
	POINT sixel_ap;
	POINT sixel_size;
};
typedef struct capdl_DSTR_16 SIXEL;
typedef SIXEL * PSIXEL;
typedef UBYTE ORIENTATION;
#define 	ORIENT_PS	 (0)
#define 	ORIENT_PORT	 (1)
#define 	ORIENT_LAND	 (2)
#define 	MEM_FACTORY_DEF	 (0)
#define 	MEM_FONT	 (1)
#define 	MEM_BITMAP	 (2)
struct capdl_DSTR_17
{
	POINT dv_ap;
	POINT dvend;
	LONG thickness;
};
typedef struct capdl_DSTR_17 DECVEC;
typedef DECVEC * PDECVEC;
struct capdl_DSTR_18
{
	UWORD tray_num;
	UWORD slot_num;
	LONG sheet_width;
	LONG sheet_length;
	ORIENTATION sheet_orient;
};
typedef struct capdl_DSTR_18 SHEET_SIZE;
typedef SHEET_SIZE * PSHEET_SIZE;
#define 	TRAY_DEV_DEF	 (0)
struct capdl_DSTR_19
{
	WORD baseline;
	UBYTE no_underlining;
	WORD numrows;
	WORD numcols;
	UBYTE * bitmap_ptr;
	UBYTE substitute_flag;
};
typedef struct capdl_DSTR_19 DLL_FONT_CHAR;
typedef DLL_FONT_CHAR * PTR_DLL_FONT_CHAR;
struct capdl_DSTR_20
{
	DLL_FONT_CHAR dll_char[97];
	FONT_METRIC dll_width_tbl[97];
	FONT_METRIC dll_l_bearing_tbl[97];
	FONT_METRIC dll_r_bearing_tbl[97];
	LONG blob_locator;
	FONT_METRIC blob_l_bearing;
	FONT_METRIC blob_r_bearing;
	FONT_METRIC blob_width;
	UBYTE default_blob_bitmap[375];
	UBYTE dll_scale_factor_code;
	WORD dll_spacing;
	UWORD dll_char_set_id;
	WORD dll_num_char_defs;
	FONT_METRIC dll_type_size;
	UWORD dll_type_size_whole;
	UWORD dll_type_size_fraction;
	LONG dll_first_char;
	LONG dll_last_char;
	ULONG dll_ul_font_flag;
	ULONG dll_strikethru_flag;
	ULONG dll_ol_font_flag;
	LONG dll_ul_offset;
	LONG dll_strikethru_offset;
	LONG dll_ol_offset;
	LONG dll_ul_thickness;
	LONG dll_strikethru_thickness;
	LONG dll_ol_thickness;
	ULONG dll_italic_flag;
	LONG dll_font_weight;
	ULONG dll_monospacing_flag;
	LONG dll_total_vertical_size;
	LONG dll_space_max;
	LONG dll_space_min;
	LONG dll_space_width;
	LONG dll_font_horiz_prop;
	WORD dll_horiz_prop_numerator;
	WORD dll_horiz_prop_denominator;
	LONG dll_above_baseline_dist;
	LONG dll_below_baseline_dist;
	LONG dll_superscript_vert;
	LONG dll_subscript_vert;
	LONG dll_average_char_width;
	LONG total_portrait_bytes;
	LONG total_landscape_bytes;
	LONG total_mixed_bytes;
	ORIENTATION font_orientation;
	LONG number_of_odd_words_in_bitmaps;
	LONG number_of_odd_words_in_bm_mixed;
	LONG number_of_odd_words_in_bm_land;
	ULONG dll_shadow_flag;
	FONT_METRIC dll_shadow_vertical;
	FONT_METRIC dll_shadow_horizontal;
	UBYTE new_font_file_id[32];
};
typedef struct capdl_DSTR_20 DLL_FONT_STATE;
typedef DLL_FONT_STATE * PTR_DLL_FONT_STATE;
struct capdl_DSTR_21
{
	BOUND pfs_lrm;
	BOUND pfs_lhe;
	BOUND pfs_tbm;
	BOUND pfs_phe;
	LONG pfs_pwid;
	LONG pfs_plen;
	ORIENTATION pfs_or;
};
typedef struct capdl_DSTR_21 PFSBOUNDS;
typedef PFSBOUNDS * PPFSBOUNDS;
struct capdl_DSTR_22
{
	PFD host_ptr;
	PFD fp_ptr;
	PFD mac_ptr;
	WORD initial_sgr;
	PFSBOUNDS initial_pfs;
};
typedef struct capdl_DSTR_22 CPINIT_STATE;
typedef CPINIT_STATE * PTR_CPINIT_STATE;


extern void pdli_clean_up_host_memory();
extern void process_eof();
extern void process_reserve_mem();
extern void process_showpage();
extern void process_condshowpg();
extern DEF process_char();
extern DEF process_vchar();
extern void process_decvec();
extern void process_orientation();
extern void process_set_copy();
extern void process_set_ssize();
extern void process_tray_select();
extern void process_set_origin();
extern void process_logical_page();
extern void pdli_get_physical_grid();
extern void pdli_get_sixel_macro_grid();
extern void process_sixel();
extern void process_color();
extern FONT_METRIC font_get_above_baseline();
extern FONT_METRIC font_get_below_baseline();
extern FONT_METRIC font_get_superscript_height();
extern FONT_METRIC font_get_subscript_height();
extern FONT_METRIC font_get_height();
extern FONT_METRIC font_get_average_width();
extern FONT_METRIC font_get_width();
extern FONT_METRIC font_get_width_vchar();
extern FONT_METRIC font_get_left_bearing();
extern FONT_METRIC font_get_left_bearing_vchar();
extern FONT_METRIC font_get_right_bearing();
extern FONT_METRIC font_get_right_bearing_vchar();
extern FONT_METRIC font_get_min();
extern FONT_METRIC font_get_max();
extern void font_get_default();
extern void font_get_next();
extern FONT_NUMBER font_select();
extern void font_delete_font_files();
extern PUB font_get_mem();
extern void font_dispose_of_font();
extern void font_get_first_sgr();
extern void font_get_next_sgr();
extern void font_get_first_nrcs();
extern void font_get_next_nrcs();
extern void process_error();
extern void pdli_flag_nvm_recall();
extern void pdli_flag_nvm_store();
extern BOOLEAN pdli_nvm_get();
extern BOOLEAN pdli_nvm_store();
extern void pdli_flag_font_dict_change();
extern ORIENTATION pdli_get_sheet_orientation();
extern BOOLEAN pdli_crm_get();
extern void pdli_com_start();
extern void pdli_com_stop();
extern void pdli_com_put_string();
extern void pdli_com_put_byte();

