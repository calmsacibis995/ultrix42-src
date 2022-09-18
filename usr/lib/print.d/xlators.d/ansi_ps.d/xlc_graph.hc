/*        @(#)xlc_graph.hc	4.1      7/2/90      */

/*  Version 2.00   4/ 3/89 13:57:00.0000  */
/* makehf /hc /cp /setup=portab.h,capdl.def xlc_graph.def xlc_graph.hc  */

#define 	MAXIMUM_PS_SIXEL_STRING	 (32767)




struct xlc_graph_DSTR_23
{
	WORD rpt_val;
	UBYTE char_mapped;
};
typedef struct xlc_graph_DSTR_23 SIXEL_MAP_EL;
typedef SIXEL_MAP_EL * PTR_SIXEL_MAP_EL;
CONST extern WORD init_list[95];
NOSHARE extern PTR_SIXEL_MAP_EL six_rptr[64];
NOSHARE extern SIXEL_MAP_EL sixel_cmprss_mppng[95];
CONST extern SIXEL_MAP_EL sixel_cmprss_mppng_null;


extern void sixel_empty_sixel_buffer();
extern void init_sixel_font_keys();

