/*        @(#)caglobal.hc	4.1      7/2/90      */

/*  Version 2.00   4/ 3/89 13:32:00.0000  */
/* makehf /hc /cp /setup=portab.h,cpsys.def caglobal.def caglobal.hc  */

#define 	NUL	 (0)
#define 	SOH	 (1)
#define 	STX	 (2)
#define 	ETX	 (3)
#define 	EOT	 (4)
#define 	ENQ	 (5)
#define 	ACK	 (6)
#define 	BEL	 (7)
#define 	BS	 (8)
#define 	HT	 (9)
#define 	LF	 (10)
#define 	VT	 (11)
#define 	FF	 (12)
#define 	CR	 (13)
#define 	SO	 (14)
#define 	SI_C	 (15)
#define 	DLE	 (16)
#define 	DC1	 (17)
#define 	DC2	 (18)
#define 	DC3	 (19)
#define 	DC4	 (20)
#define 	NAK	 (21)
#define 	SYN	 (22)
#define 	ETB	 (23)
#define 	CAN	 (24)
#define 	EM	 (25)
#define 	SUB_C	 (26)
#define 	ESC_C	 (27)
#define 	FS	 (28)
#define 	GS	 (29)
#define 	RS	 (30)
#define 	US	 (31)
#define 	X80	 (128)
#define 	X81	 (129)
#define 	X82	 (130)
#define 	X83	 (131)
#define 	IND	 (132)
#define 	NEL	 (133)
#define 	SSA	 (134)
#define 	ESA	 (135)
#define 	HTS	 (136)
#define 	HTJ	 (137)
#define 	VTS	 (138)
#define 	PLD	 (139)
#define 	PLU	 (140)
#define 	RI	 (141)
#define 	SS2	 (142)
#define 	SS3	 (143)
#define 	DCS	 (144)
#define 	PU1	 (145)
#define 	PU2	 (146)
#define 	STS	 (147)
#define 	CCH	 (148)
#define 	MW	 (149)
#define 	SPA	 (150)
#define 	EPA	 (151)
#define 	X98	 (152)
#define 	X99	 (153)
#define 	X9A	 (154)
#define 	ESC_Z	 (154)
#define 	CSI	 (155)
#define 	ST_C	 (156)
#define 	OSC	 (157)
#define 	PM	 (158)
#define 	APC	 (159)
#define 	SP_C	 (32)
#define 	DEL	 (127)
#define 	XA0	 (160)
#define 	XFF	 (255)
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

/*  ** WARNING File capdl.def Line 457 -
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

/*  ** WARNING File capdl.def Line 660 -
 *  Expression error - can't evaluate [FONT_NUMBER]
 */
#define 	END_OF_DICTIONARY	 ((FONT_NUMBER)0xFFFF)
typedef FONT_DICT_PARAM * PTR_FDP;
typedef WORD SGR_NUMBER;

/*  ** WARNING File capdl.def Line 677 -
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

/*  ** WARNING File capdl.def Line 718 -
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
#define 	S_VERSION_ID	 (9)
#define 	MAX_HTABS	 (150)
#define 	MAX_VTABS	 (150)
#define 	DEFAULT_COLS_PER_TAB	 (8)
#define 	DEFAULT_LINES_PER_TAB	 (1)
#define 	PUM_CHAR	 (0)
#define 	PUM_SIZEUNIT	 (1)
#define 	CENTIPT	 (1)
#define 	DECIPT	 (10)
#define 	PIXEL	 (24)
#define 	CENTIPTS_PER_DECIPT	 (10)
#define 	TYPE_FAMILY_ID_SIZE	 (7)
#define 	FONT_ID_SIZE	 (16)
#define 	DUMMY	 (0)
#define 	NUMBER_OF_8_BIT_CHARACTERS	 (256)
#define 	CSET_TOKSZ	 (3)
#define 	NUM_TOKTYPS	 (8)
#define 	BLOB_BITMAP	 (0)
#define 	SGR_TABLE_SIZE	 (10)
#define 	G_TABLE_SIZE	 (4)
#define 	JBUF_SIZE	 (512)
#define 	FONT_FILE_MODE_IS_MEANINGFUL	 (0)
#define 	ONE_MEGABYTE	 (1048575)
#define 	PREAMBLE_SIZE	 (160000)


struct pdl_st_DSTR_23
{
	WORD font_file;
	WORD selection_type;
	UBYTE id_string[32];
};
typedef struct pdl_st_DSTR_23 SGR;
#define 	AF	 (1)
#define 	ATF	 (2)
#define 	AG	 (3)
#define 	C94	 (0)
#define 	C96	 (1)
struct gset
{
	BOOL gset_valid;
	WORD repertory;
	WORD char_set_id;
	GLYPH_CHAR_DATA gset_map[128];
	GLYPH_FONT_DATA gset_fontdata;
};
typedef struct gset GSET;
#define 	S_GSET	 (1037)
struct pdl_saved_state
{
	UBYTE svst_version_id[9];
	POINT svst_origin;
	BOOL svst_lfnl_mode;
	BOOL svst_crnl_mode;
	BOOL svst_wrap_mode;
	BOOL svst_psp;
	WORD svst_c1rcv_mode;
	BOOL svst_psel_mode;
	WORD svst_shorp;
	WORD svst_pum_mode;
	WORD svst_sizeunit;
	LONG svst_hai;
	LONG svst_vsi;
	WORD svst_last_gss;
	WORD svst_v_size;
	WORD svst_character_proportion;
	WORD svst_user_pref_cset;
	WORD svst_user_pref_repertory;
	WORD svst_gl_index;
	WORD svst_gr_index;
	WORD svst_repertory[4];
	WORD svst_char_set_id[4];
	WORD svst_requested_attributes;
	WORD svst_cur_sgr;
	SGR svst_sgr_tbl[10];
	PFSBOUNDS svst_pfs;
	WORD svst_htabct;
	LONG svst_htabs[150];
	WORD svst_vtabct;
	LONG svst_vtabs[150];
	WORD svst_paper_tray;
	WORD svst_num_copies;
};
typedef struct pdl_saved_state PDL_SAVED_STATE;
typedef PDL_SAVED_STATE * PPDL_SAVED_STATE;
#define 	S_NVM_ST	 (1681)
struct pdl_st_DSTR_24
{
	POINT origin;
	GLYPH curchar;
	BOOL lfnl_mode;
	BOOL crnl_mode;
	BOOL wrap_mode;
	BOOL psp;
	WORD c1rcv_mode;
	BOOL psel_mode;
	WORD shorp;
	WORD pum_mode;
	WORD sizeunit;
	WORD justify_mode;
	BOOL limits_enabled_flg;
	WORD jfy_buf_index;
	GLYPH justify_buf[512];
	WORD plf;
	BOOL limit_flag;
	BOOL rmf;
	BOOL fcf;
	LONG hai;
	BOOL vai_valid;
	LONG vai;
	LONG vsi;
	WORD last_gss;
	WORD v_size;
	WORD character_proportion;
	WORD user_pref_cset;
	WORD user_pref_repertory;
	WORD propl_mode;
	GSET * gl_ptr;
	GSET * gr_ptr;
	WORD ssf;
	GSET g_table[4];
	WORD requested_attributes;
	WORD cur_sgr;
	SGR sgr_tbl[10];
	BOUND h_lim_bound;
	BOUND h_fmt_bound;
	BOUND v_lim_bound;
	BOUND v_fmt_bound;
	LONG sheet_fwid;
	LONG sheet_flen;
	LONG flen;
	ORIENTATION orientation;
	WORD htabct;
	LONG htabs[150];
	WORD vtabct;
	LONG vtabs[150];
	SIXEL cur_sxl;
	LONG hor_grid;
	POINT aspect_ratio;
	WORD paper_tray;
	WORD num_copies;
};
typedef struct pdl_st_DSTR_24 PDL_STATE;
#define 	NOPLUPLD	 (0)
#define 	PLUTM	 (32767)
#define 	PLDBM	 (-32767)
#define 	LIM_NOT_EXC	 (0)
#define 	MIN_LIM_EXC	 (1)
#define 	MAX_LIM_EXC	 (2)
#define 	JUSTIFY_OFF	 (0)
#define 	JUSTIFY_ON	 (2)
#define 	MAXROWS	 (100)
#define 	MAXCOLS	 (72)
#define 	TERMINAL_ID_SIZE	 (29)
NOSHARE extern BOOLEAN font_dict_change_flag;
NOSHARE extern BOOLEAN nvm_recall_flag;
NOSHARE extern PDL_SAVED_STATE nvm_st;
NOSHARE extern PDL_STATE xl_st;
NOSHARE extern CPINIT_STATE pdli_init_st;


extern DEFAULT test_against_limits();
extern void check_ap();
extern void copy_glyph();
extern LONG get_left_bearing();
extern LONG get_right_bearing();
extern LONG get_width();
extern void get_font_limits();
extern LONG get_font_height();
#define 	BIT0_MASK	 (1)
#define 	BIT1_MASK	 (2)
#define 	BIT2_MASK	 (4)
#define 	BIT3_MASK	 (8)
#define 	BIT4_MASK	 (16)
#define 	BIT5_MASK	 (32)
#define 	BIT6_MASK	 (64)
#define 	BIT7_MASK	 (128)
#define 	BIT8_MASK	 (256)
#define 	BIT9_MASK	 (512)
#define 	BIT10_MASK	 (1024)
#define 	BIT11_MASK	 (2048)
#define 	BIT12_MASK	 (4096)
#define 	BIT13_MASK	 (8192)
#define 	BIT14_MASK	 (16384)
#define 	BIT15_MASK	 (32768)
#define 	BIT16_MASK	 (65536)
#define 	BIT17_MASK	 (131072)
#define 	BIT18_MASK	 (262144)
#define 	BIT19_MASK	 (524288)
#define 	BIT20_MASK	 (1048576)
#define 	BIT21_MASK	 (2097152)
#define 	BIT22_MASK	 (4194304)
#define 	BIT23_MASK	 (8388608)
#define 	BIT24_MASK	 (16777216)
#define 	BIT25_MASK	 (33554432)
#define 	BIT26_MASK	 (67108864)
#define 	BIT27_MASK	 (134217728)
#define 	BIT28_MASK	 (268435456)
#define 	BIT29_MASK	 (536870912)
#define 	BIT30_MASK	 (1073741824)
#define 	BIT31_MASK	 (-2147483648)
#define 	OFFSET_FONT_FILE_HEADER_REGION	 (0)
#define 	OFFSET_TOTAL_FONT_FILE_LENGTH	 (0)
#define 	OFFSET_THE_WORD_FONT	 (4)
#define 	OFFSET_FONT_FILE_FORMAT_VERS_NO	 (8)
#define 	OFFSET_FONT_FILE_ID_DESCRIP	 (12)
#define 	OFFSET_FONT_FILE_ID_STRING	 (20)
#define 	OFFSET_FONT_REVISION_NUM	 (84)
#define 	OFFSET_DATE_AND_TIME_RECORD	 (88)
#define 	OFFSET_FONT_ATTRS_REGION_SIZE	 (100)
#define 	OFFSET_FONT_ATTRS_REGION_ADDR	 (104)
#define 	OFFSET_FONT_PARAMS_REGION_SIZE	 (108)
#define 	OFFSET_FONT_PARAMS_REGION_ADDR	 (112)
#define 	OFFSET_CHAR_DIR_REGION_SIZE	 (116)
#define 	OFFSET_CHAR_DIR_REGION_ADDR	 (120)
#define 	OFFSET_FONTSEGLIST_REG_SIZE	 (124)
#define 	OFFSET_FONTSEGLIST_REG_ADDR	 (128)
#define 	OFFSET_FUTURE_INFO_REGION_SIZE	 (132)
#define 	OFFSET_FUTURE_INFO_REGION_ADDR	 (136)
#define 	OFFSET_STRING_POOL_REGION_SIZE	 (140)
#define 	OFFSET_STRING_POOL_REGION_ADDR	 (144)
#define 	OFFSET_KERNING_INFO_REGION_SIZE	 (148)
#define 	OFFSET_KERNING_INFO_REGION_ADDR	 (152)
#define 	OFFSET_CHAR_DEF_REGION_SIZE	 (156)
#define 	OFFSET_CHAR_DEF_REGION_ADDR	 (160)
#define 	OFFSET_CHAR_COUNT_INFO	 (164)
#define 	OFFSET_FIRST_CHAR	 (164)
#define 	OFFSET_LAST_CHAR	 (168)
#define 	OFFSET_LESS_THAN_LOCATOR	 (172)
#define 	OFFSET_GREATER_THAN_LOCATOR	 (176)
#define 	OFFSET_ERROR_CHAR_LOCATOR	 (180)
#define 	OFFSET_EXTENSION_CT_LOCATOR	 (184)
#define 	OFFSET_EXTENSION_USE	 (188)
#define 	OFFSET_SPACE_CHAR_CODE	 (192)
#define 	OFFSET_ORGANIZATION_FLAGS	 (196)
#define 	DIRECTORY_ONLY_FLAG	 (1)
#define 	SELF_CONTAINED_FLAG	 (2)
#define 	DIMENSIONS_ONLY_FLAG	 (4)
#define 	EXPANDED_RASTERS_FLAG	 (8)
#define 	COMPRESSED_RASTERS_FLAG	 (16)
#define 	IDENTICAL_ORIENTATION_FLAG	 (32)
#define 	WYSIWYG_FILE_FLAG	 (64)
#define 	LARGE_VALUE_FLAG	 (128)
#define 	NRC_GLYPHS_FLAG	 (256)
#define 	OFFSET_SIZE_OF_CHAR_PARAMETERS	 (200)
#define 	OFFSET_RASTER_EXPANSION_INFO	 (204)
#define 	OFFSET_INFILE_LOCATOR_COUNT	 (204)
#define 	OFFSET_NULL_LOCATOR_COUNT	 (208)
#define 	OFFSET_NUM_CHAR_DEFS	 (212)
#define 	OFFSET_NUM_ALT_CHAR_PARAM_BLKS	 (216)
#define 	OFFSET_NUM_RASTERS	 (220)
#define 	OFFSET_NUM_COMPRESSED_RASTERS	 (224)
#define 	OFFSET_PORTRAIT_BYTE_COUNT	 (228)
#define 	OFFSET_LANDSCAPE_BYTE_COUNT	 (232)
#define 	OFFSET_TOTAL_MIXED_BYTE_COUNT	 (236)
#define 	OFFSET_PORTRAIT_COMPRESSED	 (240)
#define 	OFFSET_LANDSCAPE_COMPRESSED	 (244)
#define 	OFFSET_TOTAL_MIXED_COMPRESSED	 (248)
#define 	OFFSET_FONT_ATTRS_REGION	 (252)
#define 	OFFSET_FONT_ATTRIBUTE_FLAGS	 (252)
#define 	GENERIC_STYLE_FLAG	 (1)
#define 	ROMAN_FLAG	 (2)
#define 	GOTHIC_FLAG	 (4)
#define 	ITALIC_FLAG	 (8)
#define 	OTHER_SLANT_FLAG	 (16)
#define 	MONOSPACING_FLAG	 (32)
#define 	SHADOW_FLAG	 (64)
#define 	OUTLINE_FLAG	 (128)
#define 	INLINE_FLAG	 (256)
#define 	CONTOUR_FLAG	 (512)
#define 	CONNECTING_SCRIPT_FLAG	 (1024)
#define 	NONCONNECTING_SCRIPT_FLAG	 (2048)
#define 	REVERSE_FONT_FLAG	 (4096)
#define 	SERIF_FLAG	 (8192)
#define 	UNDERLINED_FONT_FLAG	 (16384)
#define 	OVERLINED_FONT_FLAG	 (32768)
#define 	STRUCKTHRU_FONT_FLAG	 (65536)
#define 	OFFSET_CHAR_SET_DES_DESCRIP	 (256)
#define 	OFFSET_TYPE_FAMILY_ID_DESCRIP	 (264)
#define 	OFFSET_TYPE_FAMILY_NAME_DESCRIP	 (272)
#define 	OFFSET_FONT_ID_DESCRIP	 (280)
#define 	OFFSET_TYPE_CATEGORY_DESCRIP	 (288)
#define 	OFFSET_FONT_DESCRIPTION_DESCRIP	 (296)
#define 	OFFSET_TYPE_SIZE	 (304)
#define 	OFFSET_AVERAGE_CHAR_WIDTH	 (308)
#define 	OFFSET_RESOLUTION	 (312)
#define 	OFFSET_FONT_WEIGHT	 (316)
#define 	OFFSET_FONT_HORIZ_PROPORTION	 (320)
#define 	ULTRA_EXPANDED	 (4)
#define 	EXTRA_EXPANDED	 (7)
#define 	EXPANDED	 (10)
#define 	SEMI_EXPANDED	 (13)
#define 	SEMI_CONDENSED	 (22)
#define 	CONDENSED	 (25)
#define 	EXTRA_CONDENSED	 (28)
#define 	ULTRA_CONDENSED	 (31)
#define 	OFFSET_FONT_HORIZ_PROP_FRAC	 (324)
#define 	OFFSET_PIXEL_ASPECT_RATIO	 (328)
#define 	OFFSET_CHARACTER_UP_VECTOR	 (332)
#define 	OFFSET_DEVICE_CHARACTERISTICS	 (336)
#define 	OFFSET_FOUNDRY_DESCRIP	 (340)
#define 	OFFSET_FONT_DESIGNER_DESCRIP	 (348)
#define 	OFFSET_FONT_PARAMETERS_REGION	 (356)
#define 	OFFSET_FONT_PARAMETERS_FLAGS	 (356)
#define 	INSIDE_TYPE_FIELD_FLAG	 (1)
#define 	CELL_EQUALS_RASTER_FLAG	 (2)
#define 	OFFSET_LINING_AND_ELEC_FEATURES	 (360)
#define 	OFFSET_UNDERLINE_OFFSET	 (360)
#define 	OFFSET_UNDERLINE_THICKNESS	 (364)
#define 	OFFSET_STRIKETHRU_OFFSET	 (368)
#define 	OFFSET_STRIKETHRU_THICKNESS	 (372)
#define 	OFFSET_OVERLINE_OFFSET	 (376)
#define 	OFFSET_OVERLINE_THICKNESS	 (380)
#define 	OFFSET_SLANT	 (384)
#define 	OFFSET_SHADOW_VERTICAL_OFFSET	 (388)
#define 	OFFSET_SHADOW_HORIZONTAL_OFFSET	 (390)
#define 	OFFSET_SUPER_AND_SUBSCRIPT_INFO	 (392)
#define 	OFFSET_SUPERSCRIPT_VERTICAL	 (392)
#define 	OFFSET_SUPERSCRIPT_HORIZONTAL	 (396)
#define 	OFFSET_SUBSCRIPT_VERTICAL	 (400)
#define 	OFFSET_SUBSCRIPT_HORIZONTAL	 (404)
#define 	OFFSET_HORIZ_SP_PARAMS	 (408)
#define 	OFFSET_CENTERLINE_OFFSET	 (408)
#define 	OFFSET_MIN_SPACE_WIDTH	 (412)
#define 	OFFSET_MAX_SPACE_WIDTH	 (416)
#define 	OFFSET_WIDTH_OF_SPACE	 (420)
#define 	OFFSET_WIDTH_OF_M_SPACE	 (424)
#define 	OFFSET_WIDTH_OF_N_SPACE	 (428)
#define 	OFFSET_WIDTH_OF_THIN_SPACE	 (432)
#define 	OFFSET_WIDTH_OF_DIGIT_SPACE	 (436)
#define 	OFFSET_VERT_SP_PARAMS	 (440)
#define 	OFFSET_TOP_LINE_OFFSET	 (440)
#define 	OFFSET_FLOAT_ACCENT_OFFSET	 (444)
#define 	OFFSET_HALF_LINE_OFFSET	 (448)
#define 	OFFSET_TOTAL_VERTICAL_SIZE	 (452)
#define 	OFFSET_ABOVE_BASELINE_OFFSET	 (456)
#define 	OFFSET_BELOW_BASELINE_OFFSET	 (460)
#define 	OFFSET_CAPITAL_H_HEIGHT	 (464)
#define 	OFFSET_SMALL_X_HEIGHT	 (468)
#define 	OFFSET_WHITE_SP_ABOVE_TALLEST	 (472)
#define 	OFFSET_WHITE_SP_BELOW_DEEPEST	 (476)
#define 	OFFSET_CHAR_DIR_REGION	 (480)
#define 	OFFSET_NUM_FONT_SEGMENTS	 (0)
#define 	OFFSET_BEG_FONTSEG_DESCRIP	 (4)
#define 	OFFSET_FUTURE_INFO_DATA_LENGTH	 (0)
#define 	OFFSET_FUTURE_INFO_DATA_TYPE	 (4)
#define 	OFFSET_FUTURE_INFO_DATA	 (8)
#define 	OFFSET_STR_POOL_CSET_DES_STR	 (0)
#define 	OFFSET_KERNING_FORMAT	 (0)
#define 	OFFSET_KERNING_DATA	 (4)
#define 	OFFSET_NUMBER_OF_DATA_SETS	 (0)
#define 	OFFSET_NUMBER_OF_SECTORS	 (2)
#define 	OFFSET_KERN_DATA	 (4)
#define 	OFFSET_K_INDEX	 (0)
#define 	OFFSET_FLAGS	 (0)
#define 	NO_RASTER_FLAG	 (256)
#define 	RT_MARGIN_ALIGN_FLAG	 (512)
#define 	LEFT_MARGIN_ALIGN_FLAG	 (1024)
#define 	NO_UNDERLINING_FLAG	 (2048)
#define 	NO_OVERLINING_FLAG	 (4096)
#define 	NO_STRIKETHRU_FLAG	 (8192)
#define 	PUNCTUATION_FLAG	 (16384)
#define 	CHAR_IN_TYPE_FIELD_FLAG	 (32768)
#define 	COMPLETE_RASTER_FLAG	 (65536)
#define 	ALT_PARAM_BLK_FLAG	 (268435456)
#define 	EXTENSION_INDEX_FLAG	 (536870912)
#define 	ALTERNATE_EXISTS_FLAG	 (1073741824)
#define 	FLAG_FLAG	 (-2147483648)
#define 	OFFSET_TYPE_FIELD_NOMINAL_WIDTH	 (4)
#define 	OFFSET_LEFT_BEARING	 (8)
#define 	OFFSET_RASTER_BASELINE	 (12)
#define 	OFFSET_ENCODED_RASTER	 (16)
#define 	OFFSET_ORIENTATION	 (16)
#define 	OFFSET_TYPE1	 (17)
#define 	OFFSET_TYPE2	 (18)
#define 	OFFSET_ROWS	 (20)
#define 	OFFSET_COLUMNS	 (22)
#define 	OFFSET_CHARACTER_RASTER	 (24)
#define 	LONGWORD	 (4)
#define 	LOCATOR_MASK	 (16777215)
#define 	CHAR_REPLACEMENT_MASK	 (65535)
#define 	MAX_FONT_SIZE	 (216)
#define 	NUMBER_OF_BLOB_ROWS	 (42)
#define 	NUMBER_OF_BLOB_COLUMNS	 (30)
#define 	BLOB_BITMAP_SIZE	 (168)


NOSHARE extern DLL_FONT_STATE dll_font;


extern void stars_and_dots();
extern void substitute_blob_for_char();
extern BYTE analyze_font_file();
#define 	FIRST_PFS	 (0)
#define 	LAST_PFS	 (9)
#define 	FIRST_PRIVATE_PFS	 (20)
#define 	LAST_PRIVATE_PFS	 (27)
#define 	VPFS_NUM_PARAMS	 (11)
extern void pr_pfs();
extern void dec_pfs();
extern void dec_vpfs();
extern void dec_slpp();
extern void dec_slrm();
extern void dec_stbm();
extern void dec_hpwa();
extern void dec_asfc();
extern void setorigin();
extern void ca_init_pfs();
extern void do_pfs();
extern void dec_vec();
extern void dec_rvec();
extern void ac_exit();
extern void ac_ex_graphics();
extern void ac_ex_ansb();
extern void ac_ex_atff();
extern void ac_ex_dtff();
extern void ac_ex_lff();
extern void ac_ex_aupss();
extern void ac_ex_mac();
extern void ac_ex_crm();
extern void dec_mac_term();
extern void dec_mac_enter();
extern void dec_mac_font();
extern void dec_mac_sxl();
extern void dec_spp();
#define 	FIRST_SGR_TABLE_ENTRY	 (10)


NOSHARE extern BOOL valid_load;


extern void dec_rfs();
extern void dec_rfnm();
extern void dec_rbmm();
extern void dec_uffm();
extern void save_dcs_introducer();
extern void init_cmd_ptr();
extern void dec_lff_font();
extern void dec_lff_sxl();
extern void dec_lff_load();
extern void dec_lff_enter();
extern void dec_lff_term();
extern void dec_lff_sxl_rpt();
extern void dec_dld();
extern void sixel_cvt_and_store();
#define 	CHAR_SET_DEF_TABLE_SIZE	 (22)
#define 	TRANSFORM_TABLE_SIZE	 (128)
#define 	ALL_BLOBS	 (2)


NOSHARE extern WORD preferred_index;
struct cafontcfont_DSTR_25
{
	UBYTE cfa_type_family[7];
	WORD cfa_spacing_criterion;
	WORD cfa_spacing_value;
	WORD cfa_type_size;
	WORD cfa_scale_factor;
	WORD cfa_style;
	WORD cfa_weight;
	WORD cfa_proportion;
	WORD cfa_rotation;
	WORD cfa_cset;
};
typedef struct cafontcfont_DSTR_25 CFONT_FNT_ATTR;
typedef CFONT_FNT_ATTR * PTR_CFA;
struct cafontcfont_DSTR_26
{
	FONT_NUMBER cfb_index;
	FONT_NUMBER cfb_pair_index;
	UBYTE cfb_pair_type;
	POINT cfb_scale;
	WORD cfb_nrc_type;
	WORD cfb_spacing_crit;
	WORD cfb_spacing_value;
	WORD cfb_attr;
};
typedef struct cafontcfont_DSTR_26 CFONT_BEST_STRUCT;
typedef CFONT_BEST_STRUCT * PTR_CFB;
struct cafontcfont_DSTR_27
{
	UWORD cfg_fault_flags;
	UWORD cfg_type_family;
	UWORD cfg_cset;
	UWORD cfg_type_size;
	UWORD cfg_spacing_criterion;
	UWORD cfg_spacing_value;
	UWORD cfg_scale_factor;
	UWORD cfg_style;
	UWORD cfg_weight;
	UWORD cfg_proportion;
	UWORD cfg_rotation;
};
typedef struct cafontcfont_DSTR_27 CFONT_GRADE_STRUCT;
typedef CFONT_GRADE_STRUCT * PTR_CFG;
#define 	FLT_NONE	 (0)
#define 	FLT_TYPE_FAMILY	 (16384)
#define 	FLT_CSET	 (32768)
#define 	FLT_TYPE_SIZE	 (8192)
#define 	FLT_SPACING_CRITERION	 (4096)
#define 	FLT_SPACING_VALUE	 (2048)
#define 	FLT_PROPORTION	 (1024)
#define 	FLT_STYLE	 (512)
#define 	FLT_WEIGHT	 (256)
#define 	FLT_SCALE_FACTOR	 (128)
#define 	FLT_ROTATION	 (64)
#define 	EXACT_MATCH	 (0)
#define 	NO_STYLE_FALL_BACK	 (1)
#define 	BUILT_IN_FALL_BACK	 (2)
#define 	NO_BUILT_IN_FALL_BACK	 (3)
#define 	MONO_PROP_FALL_BACK	 (1)
#define 	PAIRING_FALL_BACK	 (1)
#define 	BLOB_FALL_BACK	 (2)
#define 	NO_BLOB_FALL_BACK	 (3)
#define 	ALG_ATT_FALL_BACK	 (1)
#define 	ULTRA_LIGHT_WEIGHT_CODE	 (4)
#define 	EXTRA_LIGHT_WEIGHT_CODE	 (7)
#define 	LIGHT_WEIGHT_CODE	 (10)
#define 	SEMI_LIGHT_WEIGHT_CODE	 (13)
#define 	REGULAR_WEIGHT_CODE	 (16)
#define 	MEDIUM_WEIGHT_CODE	 (19)
#define 	SEMI_BOLD_WEIGHT_CODE	 (22)
#define 	BOLD_WEIGHT_CODE	 (25)
#define 	HEAVY_AND_EXTRA_BOLD	 (28)
#define 	BLACK_ULTRA_BO_ULTRA_HVY	 (31)
#define 	MAX_WEIGHT_VALUE	 (63)
#define 	BIN_CFFF_ENCODING	 (2)
#define 	SIX_CFFF_ENCODING	 (0)
#define 	RESOLUTION_300	 (15)
#define 	SCALE_FACTOR_1	 (20)
#define 	ROTATION_0	 (0)
#define 	SET_96_BIT	 (8192)
#define 	SGR_TYPE_FAMILY	 (0)
#define 	SGR_FONT_12	 (1)
#define 	SGR_FONT_16	 (2)
#define 	ASCII	 ('B')
#define 	USER_PREFERENCE	 ('<')
#define 	DEC_SUPPLEMENTAL	 (2741)
#define 	DEC_VT100	 ('0')
#define 	DEC_PORTUGESE	 (2742)
#define 	DEC_TECHNICAL	 ('>')
#define 	UNITED_KINGDOM	 ('A')
#define 	DEC_DUTCH	 ('4')
#define 	DEC_FINNISH	 ('5')
#define 	ISO_FINNISH	 ('C')
#define 	FRENCH	 ('R')
#define 	DEC_FRENCH_CANADIAN	 ('9')
#define 	GERMAN	 ('K')
#define 	ITALIAN	 ('Y')
#define 	JIS_ROMAN	 ('J')
#define 	DEC_NORWEGIAN_DANISH	 ('6')
#define 	NORWEGIAN_DANISH	 ('`')
#define 	ISO_NORWEGIAN_DANISH	 ('E')
#define 	SPANISH	 ('Z')
#define 	DEC_SWEDISH	 ('7')
#define 	ISO_SWEDISH	 ('H')
#define 	DEC_SWISS	 ('=')
#define 	ISO_LATIN1	 (8257)
CONST extern SGR cfont_def_sgr[10];
CONST extern UBYTE dpi_font_string[8];
CONST extern UBYTE dbuiltin_string[8];
CONST extern UBYTE subset_zzzz[5];
CONST extern UBYTE nrcs_tables[22][128];


extern void ca_font_init();
extern void compute_vai();
extern WORD b36_to_bin();
extern void nbin_to_b36();
extern UBYTE bin_to_b36();
extern void cfont_type_family();
extern void cfont_spacing();
extern WORD cfont_type_size();
extern WORD cfont_scale_factor();
extern WORD cfont_style();
extern WORD cfont_weight();
extern WORD cfont_proportion();
extern WORD cfont_rotation();
extern void cfont_cset();
extern void cfont_id_type_family();
extern void cfont_id_spacing();
extern void cfont_id_type_size();
extern void cfont_id_scale_factor();
extern void cfont_id_attributes();
extern void cfont_id_weight();
extern void cfont_id_proportion();
extern void cfont_id_rotation();
extern void cfont_id_cset();
extern void cfont_id_csubset();
extern void cfont_id_encoding();
extern void cfont_id_resolution();
extern void cfont_id_reserved();
extern void compute_font();
extern void invalidate_font();
extern void invalidate_font_for_g_set();
extern void invalidate_vai();
extern void cfont_blob();
extern void cfont_space();
extern void cfont_explode_attributes();
extern void cfont_grade_font();
extern BOOL cfont_comp_grade();
extern void cfont_update_best();
extern void compute_font_for_g_set();
extern void cfont_cache_widths();
extern LONG cfont_horiz_spacing();
extern void init_g_table();
extern void cfont_pairing();
extern void init_nrcs_tables();
#define 	DECDTFF_DELETE_TYPE_FAMILY	 (7)
#define 	DECDTFF_DELETE_FONT_FILE	 (31)
#define 	DECLFF_DELETE_FONT_FILE	 (31)
#define 	DECLFF_DELETE_ALL_DNLODED_FONTS	 (0)
#define 	FIFTY_PERCENT	 (50)
#define 	HUNDRED_PERCENT	 (100)
#define 	MAXCSETID	 (2)
#define 	MAXFONTSIZE	 (65536)


CONST extern UBYTE font_id_blank[32];


extern void pr_gss();
extern void pr_gsm();
extern void do_gsm();
extern void ca_restart_dcs();
extern void ca_start_dcs();
extern void dec_atff_enter();
extern void dec_atff_term();
extern void dec_dtff_enter();
extern void dec_dtff_term();
extern void pr_si();
extern void pr_so();
extern void pr_ss2();
extern void pr_ss3();
extern void pr_ls2();
extern void pr_ls3();
extern void pr_ls1r();
extern void pr_ls2r();
extern void pr_ls3r();
extern void pr_scs();
extern void dec_rqupss();
extern void pr_ascef1();
extern void pr_ascef2();
extern void pr_ascef3();
extern void dec_aupss_enter();
extern void dec_aupss_term();
extern void init_sgr_specific();
extern void init_sgr_tbl();
extern void pr_sgr();
extern void pr_sgr_off();
extern void pr_bold();
extern void pr_faint();
extern void pr_italics();
extern void pr_under();
extern void pr_strike();
extern void pr_font();
extern void pr_dou_under();
extern void pr_bold_off();
extern void pr_italics_off();
extern void pr_under_off();
extern void pr_strike_off();
extern void pr_black();
extern void pr_red();
extern void pr_green();
extern void pr_yellow();
extern void pr_blue();
extern void pr_magenta();
extern void pr_cyan();
extern void pr_no();
extern void pr_default();
extern void dec_sgr();
extern void dec_sgr_off();
extern void dec_super();
extern void dec_sub();
extern void dec_over();
extern void dec_trans();
extern void dec_super_sub_off();
extern void dec_over_off();
extern void dec_trans_off();
#define 	MAX_RPT_CNT	 (32767)
#define 	FIRST_SIXEL_CODE	 ('?')


NOSHARE extern BOOL sxl_raster_flag;
NOSHARE extern LONG gr_left_bound;


extern void dec_gri();
extern void ca_zap_repeat();
extern void dec_gra();
extern void dec_gci();
extern void dec_gcr();
extern void dec_gnl();
extern void dec_gr_enter();
extern void dec_gr_sxl();
extern void dec_gr_term();
extern void ca_graphics_init();
extern void ca_state_init();
extern void ca_restore_default();


#ifdef	cajfy
NOSHARE extern LONG left_anchor;
#endif
#ifdef	cajfy
NOSHARE extern LONG space_adj;
#endif
#ifdef	cajfy
NOSHARE extern WORD left_char;
#endif
#ifdef	cajfy
NOSHARE extern WORD right_char;
#endif


extern void pr_jfy();
extern void dec_jfy();
extern void pr_jfy_off();
extern void pr_jfy_limits();
extern void dec_jfy_nolimits();
extern void justify_buffer();
extern void empty_jfy_buf();
extern void enter_jfy();
extern LONG add_to_jfy_buf();
#define 	CM_UNSET	 (0)
#define 	CM_SET	 (1)
#define 	CM_RESET	 (2)


NOSHARE extern WORD cm_mode;


extern void pr_set();
extern void pr_reset();
extern void dec_set();
extern void dec_reset();
extern void pr_lnm();
extern void pr_pum();
extern void dec_awm();
extern void dec_psp();
extern void dec_psm();
extern void dec_crnlm();
extern void dec_opm();
extern void pr_crm();
extern void pr_ssu();
extern void dec_ssu();
extern void pr_decipoints();
extern void pr_pixels();
extern void dec_centipoints();
extern void dec_tc1();
extern void dec_ac1();
extern void pr_s8c1t();
extern void pr_s7c1t();
extern void pr_bcmm();


NOSHARE extern GSET * g_set_pointer;


extern void pr_text();
extern void pr_fast_text();
extern void pr_can();
extern void pr_sub();
extern void pr_sxl_sub();
extern void pr_space();
extern void pr_fast_space();
extern void pr_del();
extern void pr_xa0();
extern void pr_xff();
extern void pr_st();
extern void pr_esc();
extern void pr_dcs();
extern void pr_csi();
extern void pr_osc();
extern void pr_pm();
extern void pr_apc();
extern void pr_char_crm();
extern void pr_text_crm();
extern void pr_ctrl_crm();
extern void pr_c0_crm();
extern void pr_c1_crm();
extern void pr_crnr_crm();
#define 	PAGE_INFINITY	 (2500000)


CONST extern BOUND max_bound;


extern LONG hdist();
extern LONG vdist();
extern void hpos_abs();
extern void hpos_rel();
extern void vpos_abs();
extern void vpos_rel();
extern void update_ahp();
extern void update_avp();
extern void vpos_rel_w_wrap();
extern void vert_rel_w_wrap();
extern void pr_cond_ff();
extern void pr_nel();
extern void pr_bs();
extern void pr_lf();
extern void pr_ff();
extern void pr_cr();
extern void pr_hpa();
extern void pr_hpr();
extern void pr_hpb();
extern void pr_vpa();
extern void pr_vpr();
extern void pr_vpb();
extern void pr_ind();
extern void pr_pld();
extern void pr_plu();
extern void pr_ri();
extern void pr_cuu();
#define 	NUM_PFS	 (10)
#define 	NUM_DECPFS	 (8)
#define 	PFS0_LM	 (0)
#define 	PFS0_RM	 (55440)
#define 	PFS0_LH	 (3600)
#define 	PFS0_LE	 (55440)
#define 	PFS0_TM	 (0)
#define 	PFS0_BM	 (75600)
#define 	PFS0_PH	 (3600)
#define 	PFS0_PE	 (69600)
#define 	PFS0_PWID	 (59400)
#define 	PFS0_PLEN	 (79200)
#define 	PFS1_LM	 (0)
#define 	PFS1_RM	 (75600)
#define 	PFS1_LH	 (3600)
#define 	PFS1_LE	 (75600)
#define 	PFS1_TM	 (0)
#define 	PFS1_BM	 (55200)
#define 	PFS1_PH	 (3600)
#define 	PFS1_PE	 (49200)
#define 	PFS1_PWID	 (79200)
#define 	PFS1_PLEN	 (59400)
#define 	PFS2_LM	 (0)
#define 	PFS2_RM	 (55440)
#define 	PFS2_LH	 (3600)
#define 	PFS2_LE	 (55440)
#define 	PFS2_TM	 (0)
#define 	PFS2_BM	 (80400)
#define 	PFS2_PH	 (3600)
#define 	PFS2_PE	 (74400)
#define 	PFS2_PWID	 (59400)
#define 	PFS2_PLEN	 (84168)
#define 	PFS3_LM	 (0)
#define 	PFS3_RM	 (79200)
#define 	PFS3_LH	 (3600)
#define 	PFS3_LE	 (79200)
#define 	PFS3_TM	 (0)
#define 	PFS3_BM	 (55200)
#define 	PFS3_PH	 (3600)
#define 	PFS3_PE	 (49200)
#define 	PFS3_PWID	 (84168)
#define 	PFS3_PLEN	 (59400)
#define 	PFS4_LM	 (0)
#define 	PFS4_RM	 (57600)
#define 	PFS4_LH	 (3600)
#define 	PFS4_LE	 (57600)
#define 	PFS4_TM	 (0)
#define 	PFS4_BM	 (75600)
#define 	PFS4_PH	 (3600)
#define 	PFS4_PE	 (69600)
#define 	PFS4_PWID	 (59400)
#define 	PFS4_PLEN	 (79200)
#define 	PFS5_LM	 (0)
#define 	PFS5_RM	 (75600)
#define 	PFS5_LH	 (3600)
#define 	PFS5_LE	 (75600)
#define 	PFS5_TM	 (0)
#define 	PFS5_BM	 (56400)
#define 	PFS5_PH	 (3600)
#define 	PFS5_PE	 (50400)
#define 	PFS5_PWID	 (79200)
#define 	PFS5_PLEN	 (59400)
#define 	PFS6_LM	 (0)
#define 	PFS6_RM	 (55440)
#define 	PFS6_LH	 (3600)
#define 	PFS6_LE	 (55440)
#define 	PFS6_TM	 (0)
#define 	PFS6_BM	 (79200)
#define 	PFS6_PH	 (0)
#define 	PFS6_PE	 (79200)
#define 	PFS6_PWID	 (59400)
#define 	PFS6_PLEN	 (84168)
#define 	PFS7_LM	 (0)
#define 	PFS7_RM	 (79200)
#define 	PFS7_LH	 (3600)
#define 	PFS7_LE	 (79200)
#define 	PFS7_TM	 (0)
#define 	PFS7_BM	 (52800)
#define 	PFS7_PH	 (0)
#define 	PFS7_PE	 (52800)
#define 	PFS7_PWID	 (84168)
#define 	PFS7_PLEN	 (59400)
#define 	PFS8_LM	 (0)
#define 	PFS8_RM	 (57600)
#define 	PFS8_LH	 (3600)
#define 	PFS8_LE	 (57600)
#define 	PFS8_TM	 (0)
#define 	PFS8_BM	 (97200)
#define 	PFS8_PH	 (3600)
#define 	PFS8_PE	 (91224)
#define 	PFS8_PWID	 (59400)
#define 	PFS8_PLEN	 (100800)
#define 	PFS9_LM	 (0)
#define 	PFS9_RM	 (97200)
#define 	PFS9_LH	 (3600)
#define 	PFS9_LE	 (97200)
#define 	PFS9_TM	 (0)
#define 	PFS9_BM	 (56376)
#define 	PFS9_PH	 (3600)
#define 	PFS9_PE	 (50400)
#define 	PFS9_PWID	 (100800)
#define 	PFS9_PLEN	 (59400)
#define 	DEC_PFS20_LM	 (0)
#define 	DEC_PFS20_RM	 (57600)
#define 	DEC_PFS20_LH	 (0)
#define 	DEC_PFS20_LE	 (57600)
#define 	DEC_PFS20_TM	 (0)
#define 	DEC_PFS20_BM	 (76032)
#define 	DEC_PFS20_PH	 (0)
#define 	DEC_PFS20_PE	 (76032)
#define 	DEC_PFS20_PWID	 (59400)
#define 	DEC_PFS20_PLEN	 (79200)
#define 	DEC_PFS21_LM	 (3168)
#define 	DEC_PFS21_RM	 (72864)
#define 	DEC_PFS21_LH	 (3168)
#define 	DEC_PFS21_LE	 (72864)
#define 	DEC_PFS21_TM	 (0)
#define 	DEC_PFS21_BM	 (57024)
#define 	DEC_PFS21_PH	 (0)
#define 	DEC_PFS21_PE	 (57024)
#define 	DEC_PFS21_PWID	 (79200)
#define 	DEC_PFS21_PLEN	 (59400)
#define 	DEC_PFS22_LM	 (0)
#define 	DEC_PFS22_RM	 (55680)
#define 	DEC_PFS22_LH	 (0)
#define 	DEC_PFS22_LE	 (55680)
#define 	DEC_PFS22_TM	 (0)
#define 	DEC_PFS22_BM	 (78336)
#define 	DEC_PFS22_PH	 (0)
#define 	DEC_PFS22_PE	 (78336)
#define 	DEC_PFS22_PWID	 (59400)
#define 	DEC_PFS22_PLEN	 (84168)
#define 	DEC_PFS23_LM	 (5280)
#define 	DEC_PFS23_RM	 (74976)
#define 	DEC_PFS23_LH	 (5280)
#define 	DEC_PFS23_LE	 (74976)
#define 	DEC_PFS23_TM	 (0)
#define 	DEC_PFS23_BM	 (57024)
#define 	DEC_PFS23_PH	 (0)
#define 	DEC_PFS23_PE	 (57024)
#define 	DEC_PFS23_PWID	 (84168)
#define 	DEC_PFS23_PLEN	 (59400)
#define 	DEC_PFS24_LM	 (0)
#define 	DEC_PFS24_RM	 (57600)
#define 	DEC_PFS24_LH	 (0)
#define 	DEC_PFS24_LE	 (57600)
#define 	DEC_PFS24_TM	 (0)
#define 	DEC_PFS24_BM	 (97632)
#define 	DEC_PFS24_PH	 (0)
#define 	DEC_PFS24_PE	 (97632)
#define 	DEC_PFS24_PWID	 (59400)
#define 	DEC_PFS24_PLEN	 (100800)
#define 	DEC_PFS25_LM	 (3168)
#define 	DEC_PFS25_RM	 (94464)
#define 	DEC_PFS25_LH	 (3168)
#define 	DEC_PFS25_LE	 (94464)
#define 	DEC_PFS25_TM	 (0)
#define 	DEC_PFS25_BM	 (57024)
#define 	DEC_PFS25_PH	 (0)
#define 	DEC_PFS25_PE	 (57024)
#define 	DEC_PFS25_PWID	 (100800)
#define 	DEC_PFS25_PLEN	 (59400)
#define 	DEC_PFS26_LM	 (0)
#define 	DEC_PFS26_RM	 (76032)
#define 	DEC_PFS26_LH	 (0)
#define 	DEC_PFS26_LE	 (76032)
#define 	DEC_PFS26_TM	 (0)
#define 	DEC_PFS26_BM	 (118800)
#define 	DEC_PFS26_PH	 (0)
#define 	DEC_PFS26_PE	 (118800)
#define 	DEC_PFS26_PWID	 (79200)
#define 	DEC_PFS26_PLEN	 (122400)
#define 	DEC_PFS27_LM	 (0)
#define 	DEC_PFS27_RM	 (118800)
#define 	DEC_PFS27_LH	 (0)
#define 	DEC_PFS27_LE	 (118800)
#define 	DEC_PFS27_TM	 (0)
#define 	DEC_PFS27_BM	 (76032)
#define 	DEC_PFS27_PH	 (0)
#define 	DEC_PFS27_PE	 (76032)
#define 	DEC_PFS27_PWID	 (122400)
#define 	DEC_PFS27_PLEN	 (79200)


CONST extern PFSBOUNDS pfstbl[10];
CONST extern PFSBOUNDS decpfstbl[8];
CONST extern UBYTE VERSION[9];
#define 	LPI_11_66	 (1200)
#define 	LPI_11_88	 (900)
#define 	LPI_11_132	 (600)
#define 	LPI_11_22	 (3600)
#define 	LPI_11_33	 (2400)
#define 	LPI_11_44	 (1800)
#define 	LPI_10_5_66	 (1152)
#define 	LPI_10_5_88	 (864)
#define 	LPI_10_5_132	 (576)
#define 	LPI_10_5_22	 (3456)
#define 	LPI_10_5_33	 (2304)
#define 	LPI_10_5_44	 (1728)


extern void dec_shorp();
extern void dec_verp();
extern void pr_spi();
extern void pr_svs();
extern void pr_shs();
extern void compute_proportional_mode();
extern void compute_spacing();
extern void ac_nop();
extern void ac_nel();
extern void ac_esc2();
extern void ac_scs();
extern void ac_text();
extern void ac_fast_text_gl();
extern void ac_fast_text_gr();
extern void ac_fast_space();
extern void ac_abort_fast_text();
extern void ac_esc_invalid();
extern void ac_csi_invalid();
extern void ac_csi_startp();
extern void ac_add_csip();
extern void ac_add_csii();
extern void ac_csi_addp();
extern void ac_dcs_invalid();
extern void ac_dcs_ignore();
extern void ac_sxl_ignore();
extern void ac_dcs_startp();
extern void ac_dcs_addp();
extern void ac_dec_graph();
extern void ac_pr_ansb();
extern void ac_dec_lff();
extern void ac_dec_atff();
extern void ac_dec_dtff();
extern void ac_dec_aupss();
extern void ac_dec_mac();
extern void ac_dec_uffm();
extern void ac_dec_dld();
extern void ac_store_cmd2();
extern void ac_store_cmd1();
extern void ac_add_dcs();
extern void ac_crm();
extern void ac_crm3();
extern void ac_crm4();
extern void ac_crm3_startp();
extern void ac_crm3_addp();
extern void ac_crm_c0();
extern void ac_crm2_esc();
extern void ac_crm_c1();
extern void ac_crm3_csi();
extern void ac_crm_lf();
extern void ac_crm_ff();
extern void ac_crm_crnr();
extern void ac_crm_ris();
extern void ac_crm_str();
extern void ac_crm_reset();
extern void ac_do_cmd2();
extern void ac_start_cmd1();
extern void ac_start_cmd2();
extern void ac_cr_sxl();
extern void ac_nl_sxl();
extern void ac_do_cr_sxl();
extern void ac_do_nl_sxl();
extern void ac_do_cmd1();
extern void ac_add_cmd1();
extern void ac_add_cmd2();
extern void ac_do_sxl();
extern void ac_do_store_cmd1();
extern void ac_do_store_cmd2();
extern void ac_gr_sxl();
extern void ac_do_gr_sxl();
extern void ac_start_mac_rpt();
extern void ac_start_lff_rpt();
extern void ac_lff_term_ignore();
extern void ac_mac_term_ignore();
extern void ac_reset_start();
extern void ac_reset_font_lff();
extern void ac_reset_font_mac();
extern void ac_reset_lff();
extern void ac_reset_mac();
extern void ac_reset_sxl_lff();
extern void ac_reset_sxl_mac();
extern void ac_font_ignore();
extern void pr_ht();
extern void pr_vt();
extern void pr_hts();
extern void pr_vts();
extern void dec_hts();
extern void dec_shts();
extern void dec_caht();
extern void pr_tbc();
extern void dec_vts();
extern void dec_svts();
extern void dec_cavt();
extern void pr_htac();
extern void pr_vtac();
extern void pr_htcl();
extern void pr_htc();
extern void pr_vtc();
extern void htabs_init();
extern void vtabs_init();
extern void tab_tbl_insert();
extern void scale_htabs();
extern void scale_vtabs();
#define 	MAX_COPIES	 (127)
#define 	DEFAULT_COPIES	 (1)
#define 	NUM_C0_CODES	 (32)
#define 	NUM_C1_CODES	 (32)
#define 	NUM_CTRL_CODES	 (64)
#define 	NUM_CRNRS	 (4)
#define 	ACRONYM_SIZE	 (3)


CONST extern UBYTE acronyms[68][4];
CONST extern UBYTE terminal_id[29];
CONST extern PDL_SAVED_STATE pdl_scl_st[3];
NOSHARE extern PFD nvm_hold_ioptr;
NOSHARE extern PFIRST nvm_cp_ctptr;


extern void pr_dsr();
extern void dec_str();
extern void pr_ris();
extern void pr_cpr();
extern void pr_da();
extern void pr_da2();
extern void pr_bel();
extern void dec_fnvr();
extern void pr_ansb();
extern void pr_ansb_num();
extern void pr_ansb_cap();
extern void pr_ansb_lc();
extern void dec_bcmm();
extern void pr_spd();
extern void dec_den();
extern void dec_snc();
extern void dec_sss();
extern void dec_scl();
extern void nvm_gather();
extern void nvm_recall();
extern DEF nvm_stall_io();
extern void ca_set_given_state();
extern void pr_ansb_store();
#define 	TOK_pr_pfs	 (0)
#define 	TOK_dec_pfs	 (1)
#define 	TOK_dec_vpfs	 (2)
#define 	TOK_dec_slpp	 (3)
#define 	TOK_dec_slrm	 (4)
#define 	TOK_dec_stbm	 (5)
#define 	TOK_dec_hpwa	 (6)
#define 	TOK_dec_asfc	 (7)
#define 	TOK_dec_vec	 (8)
#define 	TOK_dec_rvec	 (9)
#define 	TOK_dec_mac_enter	 (10)
#define 	TOK_dec_mac_font	 (11)
#define 	TOK_dec_mac_sxl	 (12)
#define 	TOK_dec_spp	 (13)
#define 	TOK_dec_rfs	 (14)
#define 	TOK_dec_rfnm	 (15)
#define 	TOK_dec_rbmm	 (16)
#define 	TOK_dec_uffm	 (17)
#define 	TOK_dec_lff_font	 (18)
#define 	TOK_dec_lff_sxl	 (19)
#define 	TOK_pr_gss	 (20)
#define 	TOK_pr_gsm	 (21)
#define 	TOK_pr_si	 (22)
#define 	TOK_pr_so	 (23)
#define 	TOK_pr_ss2	 (24)
#define 	TOK_pr_ss3	 (25)
#define 	TOK_pr_ls2	 (26)
#define 	TOK_pr_ls3	 (27)
#define 	TOK_pr_ls1r	 (28)
#define 	TOK_pr_ls2r	 (29)
#define 	TOK_pr_ls3r	 (30)
#define 	TOK_pr_scs	 (31)
#define 	TOK_dec_rqupss	 (32)
#define 	TOK_pr_ascef1	 (33)
#define 	TOK_pr_ascef2	 (34)
#define 	TOK_pr_ascef3	 (35)
#define 	TOK_dec_aupss_enter	 (36)
#define 	TOK_pr_sgr	 (37)
#define 	TOK_pr_sgr_off	 (38)
#define 	TOK_pr_bold	 (39)
#define 	TOK_pr_faint	 (40)
#define 	TOK_pr_italics	 (41)
#define 	TOK_pr_under	 (42)
#define 	TOK_pr_strike	 (43)
#define 	TOK_pr_font	 (44)
#define 	TOK_pr_dou_under	 (45)
#define 	TOK_pr_bold_off	 (46)
#define 	TOK_pr_italics_off	 (47)
#define 	TOK_pr_under_off	 (48)
#define 	TOK_pr_strike_off	 (49)
#define 	TOK_pr_black	 (50)
#define 	TOK_pr_red	 (51)
#define 	TOK_pr_green	 (52)
#define 	TOK_pr_yellow	 (53)
#define 	TOK_pr_blue	 (54)
#define 	TOK_pr_magenta	 (55)
#define 	TOK_pr_cyan	 (56)
#define 	TOK_pr_no	 (57)
#define 	TOK_pr_default	 (58)
#define 	TOK_dec_sgr	 (59)
#define 	TOK_dec_sgr_off	 (60)
#define 	TOK_dec_super	 (61)
#define 	TOK_dec_sub	 (62)
#define 	TOK_dec_over	 (63)
#define 	TOK_dec_trans	 (64)
#define 	TOK_dec_super_sub_off	 (65)
#define 	TOK_dec_over_off	 (66)
#define 	TOK_dec_trans_off	 (67)
#define 	TOK_dec_gri	 (68)
#define 	TOK_ca_zap_repeat	 (69)
#define 	TOK_dec_gra	 (70)
#define 	TOK_dec_gci	 (71)
#define 	TOK_dec_gcr	 (72)
#define 	TOK_dec_gnl	 (73)
#define 	TOK_pr_jfy	 (74)
#define 	TOK_dec_jfy	 (75)
#define 	TOK_pr_jfy_off	 (76)
#define 	TOK_pr_jfy_limits	 (77)
#define 	TOK_dec_jfy_nolimits	 (78)
#define 	TOK_pr_text	 (79)
#define 	TOK_pr_fast_text	 (80)
#define 	TOK_pr_can	 (81)
#define 	TOK_pr_sub	 (82)
#define 	TOK_pr_sxl_sub	 (83)
#define 	TOK_pr_space	 (84)
#define 	TOK_pr_fast_space	 (85)
#define 	TOK_pr_del	 (86)
#define 	TOK_pr_xa0	 (87)
#define 	TOK_pr_xff	 (88)
#define 	TOK_pr_st	 (89)
#define 	TOK_pr_esc	 (90)
#define 	TOK_pr_dcs	 (91)
#define 	TOK_pr_csi	 (92)
#define 	TOK_pr_osc	 (93)
#define 	TOK_pr_pm	 (94)
#define 	TOK_pr_apc	 (95)
#define 	TOK_pr_bs	 (96)
#define 	TOK_pr_lf	 (97)
#define 	TOK_pr_ff	 (98)
#define 	TOK_pr_cr	 (99)
#define 	TOK_pr_hpa	 (100)
#define 	TOK_pr_hpr	 (101)
#define 	TOK_pr_hpb	 (102)
#define 	TOK_pr_vpa	 (103)
#define 	TOK_pr_vpr	 (104)
#define 	TOK_pr_vpb	 (105)
#define 	TOK_pr_ind	 (106)
#define 	TOK_pr_pld	 (107)
#define 	TOK_pr_plu	 (108)
#define 	TOK_pr_ri	 (109)
#define 	TOK_pr_cuu	 (110)
#define 	TOK_pr_set	 (111)
#define 	TOK_pr_reset	 (112)
#define 	TOK_dec_set	 (113)
#define 	TOK_dec_reset	 (114)
#define 	TOK_pr_lnm	 (115)
#define 	TOK_pr_pum	 (116)
#define 	TOK_dec_awm	 (117)
#define 	TOK_dec_psp	 (118)
#define 	TOK_dec_psm	 (119)
#define 	TOK_dec_crnlm	 (120)
#define 	TOK_dec_opm	 (121)
#define 	TOK_pr_crm	 (122)
#define 	TOK_pr_ssu	 (123)
#define 	TOK_dec_ssu	 (124)
#define 	TOK_pr_decipoints	 (125)
#define 	TOK_pr_pixels	 (126)
#define 	TOK_dec_centipoints	 (127)
#define 	TOK_dec_tc1	 (128)
#define 	TOK_dec_ac1	 (129)
#define 	TOK_pr_s8c1t	 (130)
#define 	TOK_pr_s7c1t	 (131)
#define 	TOK_pr_bcmm	 (132)
#define 	TOK_dec_shorp	 (133)
#define 	TOK_dec_verp	 (134)
#define 	TOK_pr_spi	 (135)
#define 	TOK_pr_svs	 (136)
#define 	TOK_pr_shs	 (137)
#define 	TOK_ac_nop	 (138)
#define 	TOK_ac_nel	 (139)
#define 	TOK_ac_esc2	 (140)
#define 	TOK_ac_scs	 (141)
#define 	TOK_ac_text	 (142)
#define 	TOK_ac_fast_text_gl	 (143)
#define 	TOK_ac_fast_text_gr	 (144)
#define 	TOK_ac_fast_space	 (145)
#define 	TOK_ac_abort_fast_text	 (146)
#define 	TOK_ac_esc_invalid	 (147)
#define 	TOK_ac_csi_invalid	 (148)
#define 	TOK_ac_csi_startp	 (149)
#define 	TOK_ac_add_csip	 (150)
#define 	TOK_ac_add_csii	 (151)
#define 	TOK_ac_csi_addp	 (152)
#define 	TOK_ac_dcs_invalid	 (153)
#define 	TOK_ac_dcs_ignore	 (154)
#define 	TOK_ac_sxl_ignore	 (155)
#define 	TOK_ac_dcs_startp	 (156)
#define 	TOK_ac_dcs_addp	 (157)
#define 	TOK_ac_dec_graph	 (158)
#define 	TOK_ac_pr_ansb	 (159)
#define 	TOK_ac_dec_lff	 (160)
#define 	TOK_ac_dec_atff	 (161)
#define 	TOK_ac_dec_dtff	 (162)
#define 	TOK_ac_dec_aupss	 (163)
#define 	TOK_ac_dec_mac	 (164)
#define 	TOK_ac_dec_uffm	 (165)
#define 	TOK_ac_dec_dld	 (166)
#define 	TOK_ac_store_cmd2	 (167)
#define 	TOK_ac_store_cmd1	 (168)
#define 	TOK_ac_add_dcs	 (169)
#define 	TOK_ac_crm	 (170)
#define 	TOK_ac_crm3	 (171)
#define 	TOK_ac_crm4	 (172)
#define 	TOK_ac_crm3_startp	 (173)
#define 	TOK_ac_crm3_addp	 (174)
#define 	TOK_ac_crm_c0	 (175)
#define 	TOK_ac_crm2_esc	 (176)
#define 	TOK_ac_crm_c1	 (177)
#define 	TOK_ac_crm3_csi	 (178)
#define 	TOK_ac_crm_lf	 (179)
#define 	TOK_ac_crm_ff	 (180)
#define 	TOK_ac_crm_crnr	 (181)
#define 	TOK_ac_crm_ris	 (182)
#define 	TOK_ac_crm_str	 (183)
#define 	TOK_ac_crm_reset	 (184)
#define 	TOK_ac_do_cmd2	 (185)
#define 	TOK_ac_start_cmd1	 (186)
#define 	TOK_ac_start_cmd2	 (187)
#define 	TOK_ac_cr_sxl	 (188)
#define 	TOK_ac_nl_sxl	 (189)
#define 	TOK_ac_do_cr_sxl	 (190)
#define 	TOK_ac_do_nl_sxl	 (191)
#define 	TOK_ac_do_cmd1	 (192)
#define 	TOK_ac_add_cmd1	 (193)
#define 	TOK_ac_add_cmd2	 (194)
#define 	TOK_ac_do_sxl	 (195)
#define 	TOK_ac_do_store_cmd1	 (196)
#define 	TOK_ac_do_store_cmd2	 (197)
#define 	TOK_ac_gr_sxl	 (198)
#define 	TOK_ac_do_gr_sxl	 (199)
#define 	TOK_ac_start_mac_rpt	 (200)
#define 	TOK_ac_start_lff_rpt	 (201)
#define 	TOK_ac_lff_term_ignore	 (202)
#define 	TOK_ac_mac_term_ignore	 (203)
#define 	TOK_ac_reset_start	 (204)
#define 	TOK_ac_reset_font_lff	 (205)
#define 	TOK_ac_reset_font_mac	 (206)
#define 	TOK_ac_reset_lff	 (207)
#define 	TOK_ac_reset_mac	 (208)
#define 	TOK_ac_reset_sxl_lff	 (209)
#define 	TOK_ac_reset_sxl_mac	 (210)
#define 	TOK_ac_font_ignore	 (211)
#define 	TOK_pr_ht	 (212)
#define 	TOK_pr_vt	 (213)
#define 	TOK_pr_hts	 (214)
#define 	TOK_pr_vts	 (215)
#define 	TOK_dec_hts	 (216)
#define 	TOK_dec_shts	 (217)
#define 	TOK_dec_caht	 (218)
#define 	TOK_pr_tbc	 (219)
#define 	TOK_dec_vts	 (220)
#define 	TOK_dec_svts	 (221)
#define 	TOK_dec_cavt	 (222)
#define 	TOK_pr_htac	 (223)
#define 	TOK_pr_vtac	 (224)
#define 	TOK_pr_htcl	 (225)
#define 	TOK_pr_htc	 (226)
#define 	TOK_pr_vtc	 (227)
#define 	TOK_pr_dsr	 (228)
#define 	TOK_dec_str	 (229)
#define 	TOK_pr_ris	 (230)
#define 	TOK_pr_cpr	 (231)
#define 	TOK_pr_da	 (232)
#define 	TOK_pr_da2	 (233)
#define 	TOK_pr_bel	 (234)
#define 	TOK_dec_fnvr	 (235)
#define 	TOK_pr_ansb	 (236)
#define 	TOK_pr_ansb_num	 (237)
#define 	TOK_pr_ansb_cap	 (238)
#define 	TOK_pr_ansb_lc	 (239)
#define 	TOK_dec_bcmm	 (240)
#define 	TOK_pr_spd	 (241)
#define 	TOK_dec_den	 (242)
#define 	TOK_dec_snc	 (243)
#define 	TOK_dec_sss	 (244)
#define 	TOK_dec_scl	 (245)
#define 	TOK_nvm_recall	 (246)
#define 	TOK_cp_add_to_cur_param	 (247)
#define 	TOK_cp_add_to_esc_ibuf	 (248)
#define 	TOK_cp_add_to_csi_ibuf	 (249)
#define 	TOK_cp_add_to_dcs_ibuf	 (250)
#define 	TOK_cp_set_private	 (251)
#define 	TOK_cp_exit_cur_mode	 (252)
#define 	TOK_cp_text_search	 (253)
#define 	TOK_cp_cmd_search	 (254)
#define 	TOK_cp_search_cur_tab	 (255)
#define 	TOK_cp_font_name	 (256)
typedef void (* TOK_fn_ptr) ();


CONST extern TOK_fn_ptr cp_tok_tbl[257];

