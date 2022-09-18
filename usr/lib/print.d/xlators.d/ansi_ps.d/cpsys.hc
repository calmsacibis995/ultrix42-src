/*        @(#)cpsys.hc	4.1      7/2/90      */

/*  Version 2.00   4/ 3/89 13:52:00.0000  */
/* makehf /hc /cp /setup=portab.h cpsys.def cpsys.hc  */

#define 	CP_MAXPCNT	 (16)
#define 	CP_MAXPARM	 (151200)
#define 	CP_MAXICNT	 (3)
#define 	CP_MAX_FONT_CHAR	 (31)
#define 	CP_DIGIT_MASK	 (15)
#define 	CP_7BIT_MASK	 (127)
#define 	CP_8BIT_MASK	 (255)
#define 	CP_SRCH_DEFAULT	 (65535)
#define 	BIT0	 (1)
#define 	BIT1	 (2)
#define 	BIT2	 (4)
#define 	BIT3	 (8)
#define 	BIT4	 (16)
#define 	BIT5	 (32)
#define 	BIT6	 (64)
#define 	BIT7	 (128)
#define 	SET_PRIVATE_FLAG	 (1)
#define 	RESET_PRIVATE_FLAG	 (0)
#define 	C0	 (31)
#define 	GL	 (127)
#define 	C1	 (159)
#define 	GR	 (255)
#define 	COMPRESSED_TABLE	 (1)
#define 	EXPANDED_TABLE	 (0)




struct search
{
	UWORD pif;
	WORD token;
};
typedef struct search SEARCH;
typedef SEARCH AS[1];
typedef AS * PAS;
struct second
{
	PAW tab2_2;
	BYTE equ_cnt;
	PAB equ_ptr;
};
typedef struct second SECOND;
typedef SECOND * PSECOND;
struct first
{
	PFV exit;
	PAS search_tab;
	BYTE encoding;
	SECOND * ch_array[4];
};
typedef struct first FIRST;
typedef FIRST * PFIRST;
struct introducer
{
	WORD ps1;
	WORD ps2;
	WORD ps3;
};
typedef struct introducer INTRODUCER;
NOSHARE extern WORD modulo;
NOSHARE extern LONG amt_allocated;
NOSHARE extern UBYTE * dcs_store_ptr;
NOSHARE extern LONG cmd_str_len;
NOSHARE extern INTRODUCER dcs_intro;
NOSHARE extern PFD cp_ioptr;
NOSHARE extern PFIRST cp_ctptr;
NOSHARE extern WORD cp_pcnt;
NOSHARE extern WORD cp_icnt;
NOSHARE extern UBYTE cp_font_name_cnt;
NOSHARE extern UBYTE cp_pflag;
NOSHARE extern UBYTE cp_c;
NOSHARE extern UBYTE cp_c7;
NOSHARE extern BYTE cp_sxlcmd;
NOSHARE extern WORD cp_currpar;
NOSHARE extern WORD cp_token;
NOSHARE extern PFD cp_host;
NOSHARE extern PFD cp_keyboard;
NOSHARE extern PFD cp_macro;
NOSHARE extern DEF test_for_eof;
NOSHARE extern LONG cp_pbuf[17];
NOSHARE extern UBYTE cp_ibuf[3];
NOSHARE extern UBYTE cp_font_name_buf[32];

