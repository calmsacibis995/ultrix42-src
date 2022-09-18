/*
 * 	@(#)fgreg.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University	of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/***********************************************************************
 *
 * Modification History:
 *
 * 23-Sep-88 -- carito (Allen Carito)
 *      Added fgctsi structure.  This structure describes the extended
 *      driver state area used by the LEGSS Firmware.
 *
 * 22-Jan-88  -- rafiey (Ali Rafieymehr)
 *	Created this header file for the Firefox driver.
 *	Derived from qdreg.h.
 *
 **********************************************************************/

/* Dragon ADDER reg map */
/* ADDER register bit definitions */
/* Y_SCROLL_CONSTANT */

#define SCROLL_ERASE		0x2000
#define ADDER_SCROLL_DOWN	0x1000

/*******************************************************************************/
/* A CHIP Control Status Register bits */

#define	OP_DONE			0x0001

/* FBIC registers + bit definitions */

#define	FGSAVGPR	0x071	/* scratch register for halt code */
#define	FGWHAMI		0x077	/* Unique software ID register */
#define	FBICSR	0x07A		/* FBIC control status register */

#define HALT_ENB	0x00000080  
#define BIT_13		0x00002000
#define	HALTCPU		0x02000000
#define	RESET		0x01000000
#define	IRQC2M		0x000F0000
#define	NORMAL_MODE	0x0000003E   /* Normal mode */

/* Tchip registers + bit definitions */

#define	TCHIP_UNBLANK	0x800		/* unblank */

/* DCHIP constants */

/* Values for the SETUP_* registers */

/* Which color channel */

#define	RED_PLANE	0x0000
#define	GREEN_PLANE	0x0200
#define	BLUE_PLANE	0x0400

/* Color channel nibble */

#define	HIGH_NIBBLE	0x00010100 /* Also respond to dest. register reads */
#define	NO_LOW_NIBBLE	0x4000	/* for 4 or 12 plane system */

/* Nibble of longword during Z-cycles */

#define	Z_NIBBLE_0	0x0000
#define	Z_NIBBLE_1	0x0800
#define	Z_NIBBLE_2	0x1000
#define	Z_NIBBLE_3	0x1800
#define	Z_NIBBLE_4	0x2000
#define	Z_NIBBLE_5	0x2800

/* Nibble of longword during direct bitmap access in "I" mode.
 * Affects the Write_mask as well.  It basicly determines the
 * logical plane number.
*/

#define	I_NIBBLE_0	0x0000
#define	I_NIBBLE_1	0x0001
#define	I_NIBBLE_2	0x0002
#define	I_NIBBLE_3	0x0003
#define	I_NIBBLE_4	0x0004
#define	I_NIBBLE_5	0x0005
#define	I_NIBBLE_6	0x0006
#define	I_NIBBLE_15	0x000F

#define	MASTER_DCHIP	0x00020000	/* respond to source register reads */

/* TCHIP bit definitions */

/* TCHIP CSR bits */

#define UNBLANK         0x0800
#define WC_LOAD         0x0001
#define VDAC_LOAD       0x0002
#define PMAP_LOAD       0x0004
#define ONE_LOAD        0x0008
#define COLLECT_VDAC0   0x0120
#define COLLECT_VDAC1   0x0160
#define COLLECT_VDAC2   0x01A0

/* INTERRUPT bits */

#define PVI_IRQ 1
#define VBS_IRQ 2
#define VBF_IRQ 4

#define       LF_ONE          15
#define       LF_ZERO          0

/*******************************************************************************/
/* ADDER status and interrupt enable registers [1], [2], [3] */

#define DISABLE 		0x0000
#define PAUSE_COMPLETE		0x0001
#define FRAME_SYNC		0x0002
#define INIT_COMPLETE		0x0004
#define RASTEROP_COMPLETE	0x0008

#define ADDRESS_COMPLETE	0x0010
#define RX_READY		0x0020
#define TX_READY		0x0040
#define ID_SCROLL_READY 	0x0080

#define TOP_CLIP		0x0100
#define BOTTOM_CLIP		0x0200
#define LEFT_CLIP		0x0400
#define RIGHT_CLIP		0x0800
#define NO_CLIP 		0x1000
#define VSYNC			0x2000

/* ADDER command register [8], [10] */

#define OCR_zero		0x0000
#define Z_BLOCK0		0x0000
#define OCRA			0x0000
#define OCRB			0x0004
#define RASTEROP		0x02c0
#define PBT			0x03c0
#define BTPZ			0x0bb0
#define PTBZ			0x07a0
#define DTE			0x0400
#define S1E			0x0800
#define S2E			0x1000
#define VIPER_Z_LOAD		0x01A0
#define ID_LOAD 		0x0100
#define CANCEL			0x0000
#define LF_R1			0x0000
#define LF_R2			0x0010
#define LF_R3			0x0020
#define LF_R4			0x0030

/* ADDER rasterop mode register [9] */

#define NORMAL			0x0000
#define LINEAR_PATTERN		0x0002
#define X_FILL			0x0003
#define Y_FILL			0x0007
#define BASELINE		0x0008
#define HOLE_ENABLE		0x0010
#define SRC_1_INDEX_ENABLE	0x0020
#define DST_INDEX_ENABLE	0x0040
#define DST_WRITE_ENABLE	0x0080

/* ADDER source 2 size register */

#define NO_TILE 		0x0080

/* External registers base addresses */

#define CS_UPDATE_MASK		0x0060
#define CS_SCROLL_MASK		0x0040

/* VIPER registers */

#define RESOLUTION_MODE 	0x0080
#define MEMORY_BUS_WIDTH	0x0081
#define PLANE_ADDRESS		0x0083
#define LU_FUNCTION_R1		0x0084
#define LU_FUNCTION_R2		0x0085
#define LU_FUNCTION_R3		0x0086
#define LU_FUNCTION_R4		0x0087
#define MASK_1			0x0088
#define MASK_2			0x0089
#define SOURCE			0x008a
#define SOURCE_Z		0x0000
#define BACKGROUND_COLOR	0x008e
#define BACKGROUND_COLOR_Z	0x000C
#define FOREGROUND_COLOR	0x008f
#define FOREGROUND_COLOR_Z	0x0004
#define SRC1_OCR_A		0x0090
#define SRC2_OCR_A		0x0091
#define DST_OCR_A		0x0092
#define SRC1_OCR_B		0x0094
#define SRC2_OCR_B		0x0095
#define DST_OCR_B		0x0096

/* VIPER scroll registers */

#define SCROLL_CONSTANT 	0x0082
#define SCROLL_FILL		0x008b
#define SCROLL_FILL_Z		0x0008
#define LEFT_SCROLL_MASK	0x008c
#define RIGHT_SCROLL_MASK	0x008d

/* VIPER register bit definitions */

#define EXT_NONE		0x0000
#define EXT_SOURCE		0x0001
#define EXT_M1_M2		0x0002
#define INT_NONE		0x0000
#define INT_SOURCE		0x0004
#define INT_M1_M2		0x0008
#define ID			0x0010
#define NO_ID			0x0000
#define WAIT			0x0020
#define NO_WAIT 		0x0000
#define BAR_SHIFT_DELAY 	WAIT
#define NO_BAR_SHIFT_DELAY	NO_WAIT


/* VIPER logical function unit codes */

#define LF_ZEROS		0x0000
#define LF_D_XOR_S		0x0006
#define LF_SOURCE		0x000A
#define LF_D_OR_S		0x000E
#define LF_ONES 		0x000F
#define INV_M1_M2		0x0030
#define FULL_SRC_RESOLUTION	0X00C0 /* makes second pass like first pass */

/* VIPER scroll register [2] */

#define SCROLL_DISABLE		0x0040
#define SCROLL_ENABLE		0x0020
#define VIPER_LEFT		0x0000
#define VIPER_RIGHT		0x0010
#define VIPER_UP		0x0040
#define VIPER_DOWN		0x0000

/* Adder scroll register */

#define ADDER_UP		0x0000
#define ADDER_DOWN		0x1000

/* Misc scroll definitions */

#define UP		0
#define DOWN		1
#define LEFT		2
#define RIGHT		3
#define NODIR		4
#define SCROLL_VMAX	31
#define SCROLL_HMAX	15
#define NEW		2
#define OLD		1
#define BUSY		1
#define DRAG		1
#define SCROLL		0

/* VDAC color map entries */

#define VDAC_BLACK	0x0000
#define	VDAC_BLUE	0x00F0
#define	VDAC_GREEN	0x0F00
#define	VDAC_CYAN	0x0FF0
#define	VDAC_RED	0x000F
#define	VDAC_MAGENTA	0x00FF
#define	VDAC_YELLOW	0x0F0F
#define	VDAC_WHITE	0x0FFF
#define	VDAC_GREY_1	0x0000
#define	VDAC_GREY_2	0x0424
#define	VDAC_GREY_3	0x0848
#define	VDAC_GREY_4	0x0C6C
#define	VDAC_GREY_5	0x0090
#define	VDAC_GREY_6	0x04B4
#define	VDAC_GREY_7	0x08D8
#define	VDAC_GREY_8	0x0FFF

/* miscellaneous defines */

#define ALL_PLANES	0xffffffff
#define UNITY		0x1fff		 /* Adder scale factor */
#define MAX_SCREEN_X	1280
#define MAX_SCREEN_Y	1024
#define FONT_HEIGHT	32

	struct achip {

	    /* A chip registers */

	    u_long achip_csr;		/* A chip Control Status Register */
	    u_long achip_counter;	/* A chip counter register */
            u_long achip_alpha_cntl;   	/* A chip alpha control register */
            u_long achip_int_mask1;    	/* A chip interrupt mask reg1 */
            u_long achip_int_mask2;    	/* A chip interrupt mask reg2 */
            u_long achip_int_mask3;    	/* A chip interrupt mask reg3 */
            u_long achip_clr_bits;     	/* A chip clear bits register */
            u_long achip_set_bits;     	/* A chip set bits register */
	    u_long pad1[9];
            u_long achip_x_a;		/* A chip X address register */
            u_long achip_y_a;		/* A chip Y address register */
	    u_long pad2[2];
            u_long achip_x_i;		/* A chip X increment register */
            u_long achip_y_i;		/* A chip Y increment register */
	    u_long pad3[2];
            u_long achip_x_size;       	/* A chip X size register */
            u_long achip_y_size;       	/* A chip Y size register */
	    u_long pad4[38];
            u_long achip_x_offset_src; 	/* A chip X offset SRC register */
            u_long achip_y_offset_src; 	/* A chip Y offset SRC register */
	    u_long pad5;
            u_long achip_x_offset_dst; 	/* A chip X offset DST register */
	    u_long pad6[3];
            u_long achip_y_offset_dst; 	/* A chip Y offset DST register */
	    u_long pad7[8];
            u_long achip_width_src;    	/* A chip SRC width register */
            u_long achip_width_dst;    	/* A chip DST width register */
	    u_long pad8;
            u_long achip_offset_src;   	/* A chip SRC offset register */
	    u_long pad9[3];
            u_long achip_offset_dst;   	/* A chip DST offset register */
	    u_long pad10[8];
            u_long achip_clip_x_1;     	/* A chip Clip registers */
            u_long achip_clip_y_1;     	/* A chip Clip registers */
	    u_long pad11;
            u_long achip_clip_x_2;     	/* A chip Clip registers */
	    u_long pad12[3];
            u_long achip_clip_y_2;     	/* A chip Clip registers */
	};



	struct dchip {

	    /* D chip registers */

	    u_long dchip_setup_0;	/* D chip setup register 0 */
	    u_long dchip_setup_1;	/* D chip setup register 1 */
	    u_long dchip_setup_2;	/* D chip setup register 2 */
	    u_long dchip_setup_3;	/* D chip setup register 3 */
	    u_long dchip_setup_4;	/* D chip setup register 4 */
	    u_long dchip_setup_5;	/* D chip setup register 5 */
	    u_long dchip_setup_6;	/* D chip setup register 6 */
	    u_long dchip_setup_7;	/* D chip setup register 7 */
	    u_long dchip_setup_8;	/* D chip setup register 8 */
	    u_long dchip_setup_9;	/* D chip setup register 9 */
	    u_long dchip_setup_10;	/* D chip setup register 10 */
	    u_long dchip_setup_11;	/* D chip setup register 11 */
	    u_long dchip_setup_12;	/* D chip setup register 12 */
	    u_long dchip_setup_13;	/* D chip setup register 13 */
	    u_long dchip_setup_14;	/* D chip setup register 14 */
	    u_long dchip_setup_15;	/* D chip setup register 15 */
	    u_long pad1[16];
	    u_long dchip_test_0;	/* D chip test register 0 */
	    u_long dchip_test_1;	/* D chip test register 1 */
	    u_long dchip_test_2;	/* D chip test register 2 */
	    u_long dchip_test_3;	/* D chip test register 3 */
	    u_long dchip_test_4;	/* D chip test register 4 */
	    u_long dchip_test_5;	/* D chip test register 5 */
	    u_long dchip_test_6;	/* D chip test register 6 */
	    u_long dchip_test_7;	/* D chip test register 7 */
	    u_long dchip_test_8;	/* D chip test register 8 */
	    u_long dchip_test_9;	/* D chip test register 9 */
	    u_long dchip_test_10;	/* D chip test register 10 */
	    u_long dchip_test_11;	/* D chip test register 11 */
	    u_long dchip_test_12;	/* D chip test register 12 */
	    u_long dchip_test_13;	/* D chip test register 13 */
	    u_long dchip_test_14;	/* D chip test register 14 */
	    u_long dchip_test_15;	/* D chip test register 15 */
	    u_long pad2[16];
	    u_long dchip_write_mask;	/* D chip test write mask */
	    u_long dchip_read_mask;	/* D chip test read mask */
	    u_long pad3[62];
	    u_long dchip_logical_0;	/* D chip test logic register 0 */
	    u_long dchip_logical_4;	/* D chip test logic register 4 */
	    u_long dchip_logical_8;	/* D chip test logic register 8 */
	    u_long dchip_logical_12;	/* D chip test logic register 12 */
	    u_long dchip_logical_16;	/* D chip test logic register 16 */
	    u_long dchip_logical_20;	/* D chip test logic register 20 */
	    u_long dchip_logical_24;	/* D chip test logic register 24 */
	    u_long dchip_logical_28;	/* D chip test logic register 28 */
	    u_long pad4[57];
	    u_long dchip_logical_z_0;	/* D chip test logic Z register 0 */
	    u_long dchip_logical_z_1;	/* D chip test logic Z register 1 */
	    u_long pad5;
	    u_long dchip_logical_z_2;	/* D chip test logic Z register 2 */
	    u_long pad6[3];
	    u_long dchip_logical_z_3;	/* D chip test logic Z register 3 */
	    u_long pad7[56];
	    u_long dchip_red_start;	/* D chip red start register */
	    u_long dchip_green_start;	/* D chip green start register */
	    u_long pad8;
	    u_long dchip_blue_start;	/* D chip blue start register */
	    u_long pad9[11];
	    u_long dchip_src_start_error;  /* D chip src start err. register */
	    u_long dchip_src_start_int;  /* D chip src start int register */
	    u_long pad10[47];
	    u_long dchip_edge_step_red;  /* D chip edge step red register */
	    u_long dchip_edge_step_green;  /* D chip edge step green reg. */
	    u_long pad11;
	    u_long dchip_edge_step_blue;  /* D chip edge step blue register */
	    u_long pad12[11];
	    u_long dchip_src_edge_error;  /* D chip src edge error register */
	    u_long dchip_src_edge_int;  /* D chip src edge int register */
	    u_long pad13[47];
	    u_long dchip_span_step_red;  /* D chip span step red register */
	    u_long dchip_span_step_green;  /* D chip span step green reg. */
	    u_long pad14;
	    u_long dchip_span_step_blue;  /* D chip span step blue register */
	    u_long pad15[11];
	    u_long dchip_src_span_step_error;  /* D chip src span step err. register */
	    u_long dchip_src_span_step_int;  /* D chip src span step int reg.*/
	    u_long pad16[47];
	    u_long dchip_control_red;	  /* D chip control red register */
	    u_long dchip_control_green;  /* D chip control green register */
	    u_long pad17;
	    u_long dchip_control_blue;	  /* D chip control blue register */
	    u_long pad18[11];
	    u_long dchip_control_src;	  /* D chip control src register */
	};




	struct tchip {

	    /* T chip registers */

	    u_long tchip_csr;		  /* T chip control status register */
	    u_long tchip_collectw;	  /* T chip collecting register */
	    u_long tchip_y_start;	  /* T chip display y start register */
	    u_long tchip_table_y_start;   /* T chip table y start register */
	    u_long tchip_x_start;	  /* T chip display x start register */
	    u_long tchip_table_x_start;   /* T chip table x start register */
	    u_long tchip_test_reg;	  /* T chip test register */
	    u_long tchip_table_cntl_reg;  /* T chip table control register */
	    u_long tchip_mon_cntl_reg0;	  /* T chip monitor control register0 */
	    u_long tchip_mon_cntl_reg1;	  /* T chip monitor control register1 */
	    u_long tchip_mon_cntl_reg2;	  /* T chip monitor control register2 */
	    u_long tchip_mon_cntl_reg3;	  /* T chip monitor control register3 */
	    u_long tchip_mon_cntl_reg4;	  /* T chip monitor control register4 */
	    u_long tchip_mon_cntl_reg5;	  /* T chip monitor control register5 */
	    u_long tchip_int_reg;	  /* T chip interrupt registers */
	    u_long tchip_vertical_int;	  /* T chip vertical int. register */
	};



	struct fbic {

	    /* FBIC registers */

	    u_long  fbic_savgpr;	  /* Scratch reg. for halt code */
	    u_long  pad1[2];
            u_long fbic_iadr2;		  /* Interlock 2 address reg. */
            u_long fbic_iadr1;		  /* Interlock 1 address reg. */
            u_long fbic_cpuid;		  /* unique hardware ID reg. */
            u_long fbic_whami;		  /* unique software ID reg. */
            u_long fbic_ipdvint;	  /* interprocess/device int. reg.*/
            u_long fbic_range;		  /* I/O space range decode re. */
            u_long fbic_fbicsr;		  /* FBIC conrol status reg. */
            u_long fbic_busdat;		  /* M-bus error data reg. */
            u_long fbic_busadr;		  /* M-bus error address reg. */
            u_long fbic_busctl;		  /* M-bus error control reg. */
            u_long fbic_buscsr;		  /* M-bus errr status reg. */
            u_long fbic_modtype;	  /* Module type reg. */
	};


/*
	struct fbic {


	    u_short pad1[112];
	    u_short fbic_savgpr;	  /* Scratch reg. for halt code **
	    u_short pad2[2];
            u_short fbic_iadr2;		  /* Interlock 2 address reg. **
            u_short fbic_iadr1;		  /* Interlock 1 address reg. **
            u_short fbic_cpuid;		  /* unique hardware ID reg. **
            u_short fbic_whami;		  /* unique software ID reg. **
            u_short fbic_ipdvint;	  /* interprocess/device int. reg. **
            u_short fbic_range;		  /* I/O space range decode re. **
            u_short fbic_fbicsr;	  /* FBIC conrol status reg. **
            u_short fbic_busdat;	  /* M-bus error data reg. **
            u_short fbic_busadr;	  /* M-bus error address reg. **
            u_short fbic_busctl;	  /* M-bus error control reg. **
            u_short fbic_buscsr;	  /* M-bus errr status reg. **
            u_short fbic_modtype;	  /* Module type reg. **
	};
*/




	struct adder {

	    /* adder control registers */

	    u_short register_address;	/* ADDER reg pntr for use by DGA */
	    u_short request_enable;	/* DMA request enables */
	    u_short interrupt_enable;	/* interrupt enables */
	    u_short status;		/* ADDER status bits */
	    u_short reserved1;		/* test function only */
	    u_short spare1;		/* spare address (what else?) */

	    u_short reserved2;		/* test function only */
	    u_short id_data;		/* data path to I/D bus */
	    u_short command;		/* ADDER chip command register */
	    u_short rasterop_mode;	/* sets rasterop execution modes */
	    u_short cmd;		/* duplicate path to above cmd reg */
	    u_short reserved3;		/* test function only */

	    /* scroll registers */

	    u_short ID_scroll_data;	/* I/D bus scroll data */
	    u_short ID_scroll_command;	/* I/D bus scroll command */
	    u_short scroll_x_min;	/* X scroll min - left boundary */
	    u_short scroll_x_max;	/* X scroll max - right boundary */
	    u_short scroll_y_min;	/* Y scroll min - upper boundary */
	    u_short scroll_y_max;	/* Y scroll max - lower boundary */
	    u_short pause;		/* Y coord to set stat when scanned */
	    u_short y_offset_pending;	/* vertical scroll control */
	    u_short y_scroll_constant;

	    /* update control registers */

	    u_short x_index_pending;	/* x pending index */
	    u_short y_index_pending;	/* y pending index */
	    u_short x_index_new;	/* new x index */
	    u_short y_index_new;		/* new y index */
	    u_short x_index_old;		/* old x index */
	    u_short y_index_old;		/* old y index */
	    u_short x_clip_min; 	/* left clipping boundary */
	    u_short x_clip_max; 	/* right clipping boundary */
	    u_short y_clip_min; 	/* upper clipping boundary */
	    u_short y_clip_max; 	/* lower clipping boundary */
	    u_short spare2;		/* spare address (another!) */

	    /* rasterop control registers */

	    u_short source_1_dx;	/* source #1 x vector */
	    u_short source_1_dy;	/* source #1 y vector*/
	    u_short source_1_x; 	/* source #1 x origin */
	    u_short source_1_y; 	/* source #1 y origin */
	    u_short destination_x;	/* destination x origin */
	    u_short destination_y;	/* destination y origin */
	    u_short fast_dest_dx;	/* destination x fast vector */
	    u_short fast_dest_dy;	/* destination y fast vector */
	    u_short slow_dest_dx;	/* destination x slow vector */
	    u_short slow_dest_dy;	/* destination y slow vector */
	    u_short fast_scale; 	/* scale factor for fast vector */
	    u_short slow_scale; 	/* scale factor for slow vector */
	    u_short source_2_x; 	/* source #2 x origin */
	    u_short source_2_y; 	/* source #2 y origin */
	    u_short source_2_size;	/* source #2 height & width */
	    u_short error_1;		/* error regs (?) */
	    u_short error_2;

	    /* screen format control registers */

	    u_short y_scan_count_0;	/* y scan counts for vert timing */
	    u_short y_scan_count_1;
	    u_short y_scan_count_2;
	    u_short y_scan_count_3;
	    u_short x_scan_conf;	/* x scan configuration */
	    u_short x_limit;
	    u_short y_limit;
	    u_short x_scan_count_0;	/* x scan count for horiz timing */
	    u_short x_scan_count_1;
	    u_short x_scan_count_2;
	    u_short x_scan_count_3;
	    u_short x_scan_count_4;
	    u_short x_scan_count_5;
	    u_short x_scan_count_6;
	    u_short sync_phase_adj;	/* sync phase (horiz sync count) */
	};


/* vdac registers */

	struct	vdac {
	    u_short a_color_map[16];	/* active region color map */
	    u_short b_color_map[16];	/* background region color map */
	    u_short resv1;		/* reserved */
	    u_short b_cur_colorA;	/* background cursor color A */
	    u_short b_cur_colorB;	/* background cursor color B */
	    u_short b_cur_colorC;	/* background cursor color C */
	    u_short resv2;		/* reserved */
	    u_short a_cur_colorA;	/* active cursor color A */
	    u_short a_cur_colorB;	/* active cursor color B */
	    u_short a_cur_colorC;	/* active cursor color C */
	    u_short resv3[8];		/* reserved */
	    u_short mode;		/* mode register */
	    u_short dadj_sync;		/* delay adjust sync */
	    u_short dadj_blank;		/* delay adjust blank */
	    u_short dadj_active;		/* delay adjust active */
	    u_short mem_read_reg;	/* memory read register */
	    u_short pad[75];
	};

/*---------------------
* DUART definitions */

	/* command definitions */

#define EN_RCV		0x01
#define DIS_RCV 	0x02
#define EN_XMT		0x04
#define DIS_XMT 	0x08
#define RESET_M 	0x10
#define RESET_RCV	0x20
#define RESET_XMT	0x30
#define RESET_ERR	0x40
#define RESET_BD	0x50
#define START_BREAK	0x60
#define STOP_BREAK	0x70

	/* interupt bit definitions */

#define EI_XMT_A	0x01
#define EI_RCV_A	0x02
#define EI_XMT_B	0x10
#define EI_RCV_B	0x20

#define XMT_RDY_A	0x01
#define RCV_RDY_A	0x02
#define XMT_RDY_B	0x10
#define RCV_RDY_B	0x20

	/* status register bit defintions */

#define RCV_RDY 	0x01
#define FIFO_FULL	0x02
#define XMT_RDY 	0x04
#define XMT_EMT 	0x08
#define OVER_ERR	0x10
#define ERR_PARITY	0x20
#define FRAME_ERR	0x40
#define RCVD_BREAK	0x80


	struct duart {

	    /* channel A - LK201 */

	    short modeA;		/* ch.A mode reg (read/write) */
	    short statusA;		/* ch.A status reg (read) */
#define clkselA statusA 		/* ch.A clock slect reg (write) */
	    short cmdA; 		/* ch.A command reg (write) */
	    short dataA;		/* rcv/xmt data ch.A (read/write) */
	    short inchng;		/* input change state reg (read) */
#define auxctl inchng			/* auxiliary control reg (write) */
	    short istatus;		/* interrupt status reg (read) */
#define imask istatus			/* interrupt mask reg (write) */
	    short CThi; 		/* counter/timer hi byte (read) */
#define CTRhi CThi			/* counter/timer hi reg (write) */
	    short CTlo; 		/* counter/timer lo byte (read) */
#define CTRlo CTlo			/* counter/timer lo reg (write) */

	    /* channel B - pointing device */

	    short modeB;		/* ch.B mode reg (read/write) */
	    short statusB;		/* ch.B status reg (read) */
#define clkselB statusB 		/* ch.B clock select reg (write) */
	    short cmdB; 		/* ch.B command reg (write) */
	    short dataB;		/* ch.B rcv/xmt data (read/write) */
	    short rsrvd;
	    short inport;		/* input port (read) */
#define outconf inport			/* output port config reg (write) */
	    short strctr;		/* start counter command (read) */
#define setbits setctr			/* output bits set command (write) */
	    short stpctr;		/* stop counter command (read) */
#define resetbits stpctr		/* output bits reset cmd (write) */

	};

/* Driver and data specific structure */
struct	qd_softc {
	long	sc_flags;		/* Flags			*/
	long	sc_category_flags;	/* Category flags		*/
	u_long	sc_softcnt;		/* Soft error count total	*/
	u_long	sc_hardcnt;		/* Hard error count total	*/
	char	sc_device[DEV_SIZE];	/* Device type string		*/
};

struct fgctsi {
  unsigned short flag;		/* flags */
  unsigned char chrw;		/* Width of character font */
  unsigned char chrh;		/* Height of character font */
  unsigned short curx;		/* X address of cursor */
  unsigned short cury;		/* Y address of cursor */
  unsigned short dsplyx;	/* X address of display  */
  unsigned short dsplyy;	/* Y address of display */
  unsigned short dsplyw;	/* Display width (in pixels) */
  unsigned short dsplyh;	/* Display height (in pixels) */
  unsigned char dsplyp;	        /* Display plane value, NOT MASK */
  unsigned char pad1[3];
  unsigned short vocx;		/* Y address of VOC load tables */
  unsigned short vocy;		/* Y address of VOC load tables */
  unsigned short vocw;		/* VOC load tables width (in pixels) */
  unsigned short voch;		/* VOC load tables height (in pixels) */
  unsigned short svtchip[16];	/* Saved Tchip registers */
  unsigned long svdchip[20];	/* Saved Dchip registers */
  unsigned long svachip[20];	/* Saved Achip registers */
  unsigned long svipat[2048];	/* Saved I_Pattern Buffer */
};			 

