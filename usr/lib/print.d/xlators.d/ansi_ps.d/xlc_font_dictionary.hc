/*        @(#)xlc_font_dictionary.hc	4.1      7/2/90      */

/*  Version 2.00   4/ 3/89 13:55:00.0000  */
/* makehf /hc /cp /setup=portab.h,capdl.def xlc_font_dictionary.def xlc_font_d
  ...  ictionary.hc*/

#define 	ACTIVE	 (1)
#define 	INACTIVE	 (!1)
#define 	NUMBER_OF_7_BIT_CHARACTERS	 (128)
#define 	NUMBER_OF_8_BIT_CHARACTERS	 (256)
#define 	FONT_VM_OVERHEAD	 (18500)
#define 	VIRTUAL_MEMORY_SAFETY_MARGIN	 (80000)
#define 	FONT_VM_MAXIMUM	 (131072)
#define 	BLOB_BITMAP	 (0)

/*  ** WARNING File xlc_font_dictionary.def Line 38 -
 *  Expression error - can't find [NUMBER_OF_FIXED_FONTS]
 */
#define 	VAX_FONT_TABLE_SIZE	 (NUMBER_OF_FIXED_FONTS+32)
#define 	NUMBER_OF_FIXED_FONTS	 (40)
#define 	FIRST_DOWNLOADED_FONT	 (40)
#define 	LEFT	 (0)
#define 	RIGHT	 (128)
#define 	DLL_FONT_TABLE_SIZE	 (32)
#define 	NUMBER_OF_FIXED_FONT_BOXES	 (8)
#define 	SPACING_FONT_TABLE_SIZE	 (10)
#define 	SPACING_OFFSET	 (40)
#define 	SPACED_FONT_VM_NEEDS	 (1000)
#define 	MAPPING_TABLE_SIZE	 (4)
#define 	MAP_LEFT	 (0)
#define 	MAP_RIGHT	 (1)
#define 	MAP_RIGHT_ISO	 (2)
#define 	MAP_RIGHT_SUPP	 (3)
#define 	SIZE_OF_CHAR_SET	 (128)




CONST extern UBYTE mapping_tables[4][128];
struct font_DSTR_23
{
	UBYTE source_cset[4];
	UBYTE search_cset[4];
	UBYTE source_pairing_code;
};
typedef struct font_DSTR_23 PAIRING_TABLE_STRUCT;
#define 	PAIRING_OPTIONS_COUNT	 (3)
CONST extern PAIRING_TABLE_STRUCT pairing_table[3];
#define 	PT_10	 (1000)
#define 	PT_6_7	 (670)
struct font_DSTR_24
{
	BYTE bms_font_file_id[32];
	WORD bms_box;
	WORD bms_mapping_index;
	WORD bms_font_spacing;
	WORD bms_font_size;
	WORD bms_prop_numerator;
	LONG bms_min;
	LONG bms_max;
};
typedef struct font_DSTR_24 BUILTIN_METRICS_STRUCT;
CONST extern BUILTIN_METRICS_STRUCT builtin_metrics[40];
struct font_DSTR_25
{
	WORD vax_active;
	FONT_ID font_file_id;
	WORD character_weight;
	WORD proportion_numerator;
	WORD proportion_denominator;
	FONT_NUMBER opposite_entry;
	UBYTE pairing_code;
	FONT_NUMBER pairing_font;
	LONG space_width;
	LONG ul_thickness;
	LONG ul_offset;
	LONG ol_thickness;
	LONG ol_offset;
	LONG strikethru_thickness;
	LONG strikethru_offset;
	LONG above_baseline;
	LONG total_height;
	LONG superscript_height;
	LONG subscript_height;
	LONG average_width;
	LONG vmin_table;
	LONG vmax_table;
	LONG virtual_memory_needs;
	BYTE * bitmaps_ptr;
};
typedef struct font_DSTR_25 VAX_FONT_TABLE_STRUCT;
struct font_DSTR_26
{
	WORD dll_font;
	BOOLEAN dll_active;
};
typedef struct font_DSTR_26 DLL_FONT_TABLE_STRUCT;
struct font_DSTR_27
{
	WORD space_font;
	WORD spacing;
	WORD space_active;
};
typedef struct font_DSTR_27 SPACING_FONT_TABLE_STRUCT;
CONST extern UBYTE fixed_font_ids[32][32];
NOSHARE extern WORD width_table[72][128];
NOSHARE extern WORD left_bearing_table[72][128];
NOSHARE extern WORD right_bearing_table[72][128];
NOSHARE extern WORD above_baseline_table[72];
NOSHARE extern WORD height_table[72];
NOSHARE extern WORD below_baseline_table[72];
NOSHARE extern WORD superscript_height_table[72];
NOSHARE extern WORD subscript_height_table[72];
NOSHARE extern WORD average_width_table[72];
NOSHARE extern WORD min_table[72];
NOSHARE extern WORD max_table[72];
NOSHARE extern VAX_FONT_TABLE_STRUCT vax_font_table[72];
NOSHARE extern WORD vax_font_table_mapping_index[72];
NOSHARE extern WORD vax_font_table_box_number[72];
NOSHARE extern DLL_FONT_CHAR font_bitmap[32][97];
NOSHARE extern DLL_FONT_TABLE_STRUCT dll_font_table[32];
NOSHARE extern SPACING_FONT_TABLE_STRUCT spacing_font_table[10];


extern void update_tables();
extern void font_pairs();
extern void font_calc_overrides();

