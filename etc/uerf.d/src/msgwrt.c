#ifndef lint
static char sccsid[]  =  "@(#)msgwrt.c	4.4   (ULTRIX)   10/16/90";
#endif  lint

/*
*	.TITLE  MSGWRT - output field formatter for BTT
*	.IDENT	/1-001/
*
* COPYRIGHT (C) 1985 DIGITAL EQUIPMENT CORP.,
* CSSE SOFTWARE ENGINEERING
* MARLBOROUGH, MASSACHUSETTS
*
* THIS SOFTWARE IS FURNISHED UNDER A LICENSE FOR USE ONLY ON A 
* SINGLE COMPUTER SYSTEM AND MAY BE COPIED ONLY WITH THE INCLUSION
* OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE,  OR ANY OTHER
* COPIES THEREOF; MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE 
* TO ANY OTHER PERSON EXCEPT FOR USE ON SUCH SYSTEM AND TO ONE WHO 
* AGREES TO THESE LICENSE TERMS.  TITLE TO AND OWNERSHIP OF THE 
* SOFTWARE SHALL AT ALL TIMES REMAIN IN DEC.
*
* THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
* NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL
* EQUIPMENT CORPORATION.
*
* DEC ASSUMES NO RESPONSIBILITY FOR THE USE OR RELIABILITY OF
* ITS SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DEC.
*
*++
*
* FACILITY:		FMA - Event Information Management System
*
* ABSTRACT:
*
*	This module does the final output field processing for BTT
*	process.  It receives a pointer to the specific field in the
*	segment and a pointer to the field descriptor.  It uses this 
*	to transform the field to its final output.
*	
* ENVIRONMENT:	VAX/VMS C,  ULTRIX-32 C
*
* AUTHOR:  Bob Winant,  CREATION DATE:  05-Dec-85
*
* MODIFIED BY:
*
*	Luis Arce	24-Jan-86	complete initial development
*
*
*--
*/

#include <stdio.h>
#include <sys/time.h>
#include "ueliterals.h"
#include "uestruct.h"
#include "uerror.h"
#include "btliterals.h"
#include "generic_dsd.h"
#include "std_dsd.h"

/*
*++
*=
*=
*=MODULE FLOW - msgwrt.c
*=
*=  a - output_fld(ctx)                             Prints designated field.
*=          find_label_dsd(LABEL_IX)                        (dsd_access.c)
*=          dsd_get_label(LABEL_IX)                         (dsd_access.c)
*=          switch(item_type)
*=              {
*=              INDEX:
*=                  decode_std_item(ctx.item_ptr)           (dsd_access.c)
*=                  print_string(string_ptr)                (* e)
*=              INT:
*=                  printf(...)
*=              STRING:
*=                  print_string(item_ptr)                  (* e)
*=              REGISTER:
*=                  bld_reg_string(ctx,reg)                 (* b)
*=              DATE:
*=                  bld_dmy_string(item_ptr)                (* d)
*=              VECTOR:
*=                  hex_dump(item_ptr)                      (* f)
*=              INVALID:
*=                  bld_invalid_string(ctx)                 (* c)
*=              }
*=          return(UE$SUCC)
*=
*=  b - bld_reg_string(ctx,reg)                     Formats register output.
*=          do
*=              {
*=              dsd_get_label(LABEL_IX)                     (dsd_access.c)
*=              find_label_dsd(LABEL_IX)                    (dsd_access.c)
*=              decode_register_field(ctx,bits)             (dsd_access.c)
*=              print_string(string_ptr)                    (* e)
*=              }
*=          while(get_next_field_dsd(ctx))                  (dsd_access.c)
*=          return(UE$SUCC)
*=
*=  c - bld_invalid_string(ctx)                     Builds a string with item
*=                                                  value when unknown type.
*=          printf(INVALID ITEM xxx)
*=          return(UE$SUCC)
*=
*=  d - bld_dmy_string(bin_date)                    Prints ascii date.
*=          gettimeofday(tvp,tzp)
*=          localtime(bin_date)
*=          ctime(bin_date)
*=          timezone(min_west,is_dst)
*=          printf(ascii date)
*=          return()
*=
*=  e - print_string(str_ptr,wrap_col,line_len,1st_len) Prints string with wrap.
*=
*=  f - hex_dump(ptr,len,size,addr)                 Prints a hex dump of data.
*=
*
*--
*/


/***********************  external data  **************************/
unsigned long vms_to_unix_time();

static char tran_tbl[260] = {"\
................................\
 !\"#$%&'()*+,-./0123456789:;<=>?\
@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\
`abcdefghijklmnopqrstuvwxyz{|}~.\
................................\
................................\
................................\
..................................."};

extern struct in_struc in_st;

/********************************************************************/

#define LINESIZE  62

/***********  ROUTINE DEFINITIONS TO USE IN MATRIX  **********/

#define shtdec   1
#define shthex	 2
#define shtoct	 3
#define lngdec	 4
#define lnghex	 5
#define lngoct	 6
#define strstr	 7
#define shtinx	 8
#define lnginx	 9
#define srghex	10
#define srgdec	11
#define lrghex	12
#define lrgdec	13
#define dattim	14
#define bytvec	15
#define invmov	16
#define ctsvec	17
#define ctlvec	18
#define ctadvc	19
#define byvcdc  20
#define lngvec  21
#define vmstim	22
#define srgnov	23
#define lrgnov	24

#define display_num	7		/* num of display codes	*/
#define type_num	19		/* number of type codes	*/

/********    TABLE MATRIX OF ROUTINES TO PRINT DATA   ****************/

/* ------------------ display format codes --------------------------

 DEFLT   DEC    HEX    OCT    DATE   ELAP   HEX
			      TIME   TIME   DUMP
---------------------------------------------------------------------*/
short rtn_tbl[type_num][display_num] = {

{shtdec,shtdec,shthex,shtoct,invmov,invmov,invmov},	/* SHORT     */
{lngdec,lngdec,lnghex,lngoct,invmov,invmov,invmov},	/* LONG	     */
{strstr,invmov,invmov,invmov,invmov,invmov,invmov},	/* STRING    */
{shtinx,invmov,invmov,invmov,invmov,invmov,invmov},	/* SHORT INX */
{lnginx,invmov,invmov,invmov,invmov,invmov,invmov},	/* LONG INX  */
{srghex,srgdec,srgnov,invmov,invmov,invmov,invmov},	/* SHORT REG */
{lrghex,lrgdec,lrgnov,invmov,invmov,invmov,invmov},	/* LONG REG  */
{invmov,invmov,invmov,invmov,dattim,invmov,invmov},	/* DATE	     */
{bytvec,byvcdc,bytvec,invmov,invmov,invmov,invmov},	/* BYTE VEC  */
{ctsvec,invmov,ctsvec,invmov,invmov,invmov,ctsvec},	/* CT_SH VEC */
{ctlvec,invmov,ctlvec,invmov,invmov,invmov,ctlvec},	/* CT_LG VEC */
{invmov,invmov,invmov,invmov,invmov,invmov,ctadvc},	/* CT_AD VEC */
{invmov,invmov,invmov,invmov,invmov,invmov,invmov},	/* TINY */
{invmov,invmov,invmov,invmov,invmov,invmov,invmov},	/* TINY_INDEX */
{invmov,invmov,invmov,invmov,invmov,invmov,invmov},	/* ASCIZ */
{invmov,invmov,invmov,invmov,invmov,invmov,invmov},	/* BIT_VECTOR */
{invmov,invmov,invmov,invmov,invmov,invmov,invmov},	/* SHORT_VECTOR */
{lngvec,invmov,lngvec,invmov,invmov,invmov,invmov},	/* LONG_VECTOR */
{invmov,invmov,invmov,invmov,vmstim,invmov,invmov} 	/* VMS_TIME */
};

/*     declaration of other processing routines ***/

long  bld_reg_string();
long  bld_invalid_string();
long  bld_dmy_string();
long  print_string();
long  hex_dump();


/*
*	.SBTTL	OUTPUT_FLD
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine separates output fields by field type
*	as designated by the field descriptor.
*	
* CALLING SEQUENCE:		CALL OUTPUT_FLD (..See Below..)
*
* FORMAL PARAMETERS:		ctx
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* ROUTINE VALUE:		invalid_field (error)
*				good_field (success)
*	
* SIDE EFFECTS:			NONE
*
*--
*/

/*...	ROUTINE OUTPUT_FLD (ctx)				    */

long  output_fld(ctx)

DD$STD_DSD_CTX *ctx;

{
char			*dsd_get_label();
long			get_next_field_dsd();
DD$DSP_LABELS_PTR	find_label_dsd();

DD$DSP_LABELS_PTR	label_dsd_ptr;
short i;
short len;
unsigned short sht;
unsigned long  lng;
long  addr;
long  reg;
char  *string_ptr;
double vdate1, vdate2, vtmp;
long vlong;

if ((ctx->item_VALID_code == DD$N_V$N_A) ||
    (ctx->item_VALID_code == DD$N_A))
    return(UE$SUCC);

label_dsd_ptr = find_label_dsd(ctx->item_DSD_ptr->LABEL_IX);
if (in_st.out_form == UE$OUT_TERSE)
    {
    if (ctx->user_1 == BT$NEW_SEG)
	{
        ctx->user_1 = UE$NULL;
	printf("%-*.*s", BT$F1_LEN, BT$F1_LEN,
			dsd_get_label(ctx->item_DSD_ptr->LABEL_IX));
	}
    else
        printf("\n%-*.*s", BT$F1_LEN, BT$F1_LEN,
			dsd_get_label(ctx->item_DSD_ptr->LABEL_IX));
    }

else 
    {
    if (ctx->user_1 == BT$NEW_SEG)
        {
        ctx->user_1 = UE$NULL;
        if (strlen(dsd_get_label(ctx->segment_DSD_ptr->LABEL_IX)) > 1)
            printf("\n\n----- %s -----\n",
			dsd_get_label(ctx->segment_DSD_ptr->LABEL_IX));
        }
    switch (rtn_tbl[ctx->item_DSD_ptr->TYPE-1] [label_dsd_ptr->TYPE-1])
        {
        case (shtinx):
        case (shthex):
        case (shtdec):
        case (shtoct):
        case (srgdec):
        case (srghex):
        case (srgnov):
            if (*((short *) ctx->item_ptr) == -1)
                return(UE$SUCC);
        break;
        case (lnginx):
        case (lnghex):
        case (lngdec):
        case (lngoct):
        case (lrgdec):
        case (lrghex):
        case (lrgnov):
            if (*((long *) ctx->item_ptr) == -1)
                return(UE$SUCC);
        break;
        }
    printf("\n%-*.*s", BT$F1_LEN, BT$F1_LEN,
			dsd_get_label(ctx->item_DSD_ptr->LABEL_IX));
    }

switch (ctx->item_VALID_code)
    {
    case (DD$VALID):
        switch (rtn_tbl[ctx->item_DSD_ptr->TYPE-1] [label_dsd_ptr->TYPE-1])
	{
		/************************************************/
		/* Following routines are in order of frequency */
		/************************************************/

	case (shtinx):			/*** short indexed 	***/
	    if (*((short *) ctx->item_ptr) == -1)
    		break;
	    if ((int)(string_ptr = (char *) decode_std_item
			(ctx, *((short *) ctx->item_ptr)))
			!= DD$UNKNOWN_CODE)
	        {
		printf("%*s", BT$F2_LEN, BT$SPACE);
		printf("%*s", BT$F3_LEN, BT$SPACE);
	        print_string   (string_ptr,
				BT$F1_LEN + BT$F2_LEN + BT$F3_LEN,
				BT$F4_LEN,
				BT$F4_LEN);
	        }
    	break;

	case (lnghex):			/*** long Hexidecimal	***/
	    if (*((long *) ctx->item_ptr) == -1)
	        break;
	    printf("%*s", BT$F2_LEN-9, BT$SPACE);
	    lng = *((long *) ctx->item_ptr);
	    printf("x%08.8X", lng);
    	break;

	case (strstr):			/*** string		***/
	    if (in_st.out_form != UE$OUT_TERSE)
		{
	        printf("%*s", BT$F2_LEN, BT$SPACE);
	        printf("%*s", BT$F3_LEN, BT$SPACE);
	        print_string(*((long *) ctx->item_ptr),
				BT$F1_LEN + BT$F2_LEN + BT$F3_LEN,
				BT$F4_LEN,
				BT$F4_LEN);
		}
	    else
		{
	        printf("   ");
	        print_string(*((long *) ctx->item_ptr),
				BT$F1_LEN + 3,
				BT$F2_LEN + BT$F3_LEN + BT$F4_LEN - 3,
				BT$F2_LEN + BT$F3_LEN + BT$F4_LEN - 3);
		}
    	break;

	case (lngdec):			/*** long decimal	***/
	    if (*((long *) ctx->item_ptr) == -1)
	        break;
	    printf("%*d.", BT$F2_LEN-1, *((long *) ctx->item_ptr));
    	break;

	case (lrgdec):			/*** long reg decimal	***/
	    printf("%*d.",BT$F2_LEN-1, *((long *) ctx->item_ptr));
	    reg = *((long *) ctx->item_ptr);
	    if ((reg != -1) && (in_st.out_form != UE$OUT_TERSE))
	        bld_reg_string(ctx,reg);
    	break;

	case (lrghex):			/*** long reg hex	***/
	    printf("%*s", BT$F2_LEN-9, BT$SPACE);
	    lng = *((long *) ctx->item_ptr);
	    printf("x%08.8X", lng);
	    reg = *((long *) ctx->item_ptr);
	    if ((reg != -1) && (in_st.out_form != UE$OUT_TERSE))
	        bld_reg_string(ctx,reg);
    	break;

	case (lrgnov):			/*** long reg (no label or value) ***/
	    printf("%*s", BT$F2_LEN, BT$SPACE);
	    reg = *((long *) ctx->item_ptr);
	    if ((reg != -1) && (in_st.out_form != UE$OUT_TERSE))
	        bld_reg_string(ctx,reg);
    	break;

	case (dattim):			/*** date		***/
	    bld_dmy_string((long ) ctx->item_ptr);
    	break;

	case (vmstim):			/*** vms format date/time  */
		vlong = vms_to_unix_time( *(long *)ctx->item_ptr,
										*((long *)(ctx->item_ptr)+1));
	    bld_dmy_string((long ) &vlong);
    	break;

	case (lnginx):			/*** long indexed	***/
	    if (*((long *) ctx->item_ptr) == -1)
	        break;
	    if ((int)(string_ptr = (char *) decode_std_item
			(ctx, *((long *) ctx->item_ptr)))
			!= DD$UNKNOWN_CODE)
	        {
		printf("%*s", BT$F2_LEN, BT$SPACE);
		printf("%*s", BT$F3_LEN, BT$SPACE);
	        print_string   (string_ptr,
				BT$F1_LEN + BT$F2_LEN + BT$F3_LEN,
				BT$F4_LEN,
				BT$F4_LEN);
	        }
    	break;

	case (shthex):			/*** short hexidecimal	***/
	    if (*((short *) ctx->item_ptr) == -1)
    		break;
	    printf("%*s", BT$F2_LEN-5, BT$SPACE);
	    sht = *((short *) ctx->item_ptr);
	    printf("x%04.4X", sht);
    	break;

	case (ctlvec):			/*** count long vector  ***/
	    len = *((long *)ctx->item_ptr);
	    if (len > ctx->item_DSD_ptr->COUNT)
		len = ctx->item_DSD_ptr->COUNT;
	    if (len > 0)
		hex_dump((ctx->item_ptr + 4), (len * 4),4,0);
	break;

	case (lngvec):			/*** long vector  ***/
	    len = ctx->item_DSD_ptr->COUNT;
	    if (len > 0)
		hex_dump((ctx->item_ptr), (len * 4),4,0);
	break;

	case (ctadvc):			/*** count addr vector  ***/
	    len  = *((long *)(ctx->item_ptr+4));
	    addr = *((long *)ctx->item_ptr);
	    if (len > ctx->item_DSD_ptr->COUNT)
		len = ctx->item_DSD_ptr->COUNT;
	    if (len > 0)
		hex_dump((ctx->item_ptr + 8), (len * 4),4,addr);
	break;

	case (bytvec):			/*** byte vector	***/
	    printf("%*s", (BT$F2_LEN - 1 - (2 * ctx->item_DSD_ptr->COUNT)),
				BT$SPACE);
	    printf("x");
	    for (i = ctx->item_DSD_ptr->COUNT -1; i >= 0; i--)
		{
	    	printf("%02.2X", ctx->item_ptr[i]);
		}
    	break;

	case (byvcdc):			/*** byte vector decimal ***/
	    printf("%*s", (BT$F2_LEN - 1 - ctx->item_DSD_ptr->COUNT),
				BT$SPACE);
	    for (i = ctx->item_DSD_ptr->COUNT -1; i >= 0; i--)
		{
	    	printf("%01.1d", ctx->item_ptr[i]);
		}
	    printf(".");
    	break;

	case (shtdec):			/*** short decimal	***/
	    if (*((short *) ctx->item_ptr) == -1)
    		break;
	    printf("%*d.", BT$F2_LEN-1, *((short *) ctx->item_ptr));
    	break;

	case (shtoct):			/*** short octal	***/
	    if (*((short *) ctx->item_ptr) == -1)
	        break;
	    printf("%*s", BT$F2_LEN-5, BT$SPACE);
	    printf("o%04o", *((short *) ctx->item_ptr));
    	break;

	case (lngoct):			/*** long octal		***/
	    if (*((long *) ctx->item_ptr) == -1)
	        break;
	    printf("%*s", BT$F2_LEN-9, BT$SPACE);
	    printf("o%08o", *((long *) ctx->item_ptr));
    	break;

	case (srgdec):			/*** short reg decimal	***/
	    printf("%*d.",BT$F2_LEN-1, *((short *) ctx->item_ptr));
	    reg = *((long *) ctx->item_ptr);
	    if ((reg != -1) && (in_st.out_form != UE$OUT_TERSE))
	        bld_reg_string(ctx,reg);
    	break;

	case (srghex):			/*** short reg hex	***/
	    printf("%*s", BT$F2_LEN-5, BT$SPACE);
	    sht = *((short *) ctx->item_ptr);
	    printf("x%04.4X", sht);
	    reg = *((long *) ctx->item_ptr);
	    if ((reg != -1) && (in_st.out_form != UE$OUT_TERSE))
	        bld_reg_string(ctx,reg);
    	break;

	case (srgnov):			/*** short reg (no label or value)***/
	    printf("%*s", BT$F2_LEN, BT$SPACE);
	    reg = *((long *) ctx->item_ptr);
	    if ((reg != -1) && (in_st.out_form != UE$OUT_TERSE))
	        bld_reg_string(ctx,reg);
    	break;

	case (ctsvec):			/*** count short vector ***/
	    len = *((short *)ctx->item_ptr);
	    if (len > ctx->item_DSD_ptr->COUNT)
		len = ctx->item_DSD_ptr->COUNT;
	    if (len > 0)
		hex_dump((ctx->item_ptr + 2), (len * 2),2,0);
	break;

	case (invmov):			/*** invalid move	***/
	    label_dsd_ptr = find_label_dsd(ctx->item_DSD_ptr->LABEL_IX);
	    printf("Invalid move TYPE = %d, DISPLAY TYPE = %d",
		ctx->item_DSD_ptr->TYPE,label_dsd_ptr->TYPE);
    	break;

	}
    break;
    case (DD$N_V):
	bld_invalid_string(ctx);
    break;
    }
return(UE$SUCC);   
}

/*...	ENDROUTINE OUTPUT_FLD				    */



/*
*	.SBTTL	BLD_REG_STRING - Builds register formated output
*
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine is called to build formatted
*	register output of the given input.
*	
* CALLING SEQUENCE:		CALL BLD_REG_STRING (..See Below..)
*
* FORMAL PARAMETERS:		
*
*	reg			a long with the register contents
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		Final formatted output string
*	
* ROUTINE VALUE:		UE$SUCC  if successful
*				UE$FAIL  if failure
*	
* SIDE EFFECTS:			Creates the final output
*
*--
*/
/*...	ROUTINE BLD_REG_STRING (ctx,reg)	    */
long  bld_reg_string (ctx,reg)

DD$STD_DSD_CTX *ctx;
unsigned long  reg;

{
DD$DSP_LABELS_PTR	find_label_dsd();
DD$DSP_LABELS_PTR	label_dsd_ptr;
long			get_next_field_dsd();

char *string_ptr;
unsigned long bits;
short first;
short len;

first = TRUE;

do
    {
				/* The next three instructions need  */
				/* to be kept separate because VMS   */
				/* will propagate the high order bit */
				/* on the last shift right if they   */
				/* are combined into one instruction */

    bits = reg  >> ctx->field_position;
    bits = bits << (32 - ctx->field_DSD_ptr->SIZE);
    bits = bits >> (32 - ctx->field_DSD_ptr->SIZE);

				/*************************************/

    switch (ctx->field_DSD_ptr->TYPE)
	{
	case (DC_INTEGER):
	    if (!first)
		{
		printf("\n%*s",BT$F1_LEN,BT$SPACE);
		printf("%*s",  BT$F2_LEN,BT$SPACE);
		}
	    printf("%*s",  BT$F3_LEN,BT$SPACE);
	    printf("%s ", dsd_get_label(ctx->field_DSD_ptr->LABEL_IX));
	    label_dsd_ptr = find_label_dsd(ctx->field_DSD_ptr->LABEL_IX);
	    if (label_dsd_ptr->TYPE == DF_DECIMAL)
		printf("%-d.", bits);
	    else if (label_dsd_ptr->TYPE == DF_OCTAL)
		printf("o%-o", bits);
	    else
		printf("x%-X", bits);
	    break;
	case (DC_CODED):
	    if ((int)(string_ptr = (char *)
		    decode_register_field(ctx,bits))
		    != DD$UNKNOWN_CODE)
		{
		if (!first)
		    {
		    printf("\n%*s",BT$F1_LEN,BT$SPACE);
		    printf("%*s",  BT$F2_LEN,BT$SPACE);
		    }
		printf("%*s",  BT$F3_LEN,BT$SPACE);
		len = strlen(dsd_get_label(ctx->field_DSD_ptr->LABEL_IX));
		if (len > 1)
		    printf("%s ", dsd_get_label(ctx->field_DSD_ptr->LABEL_IX));
	        print_string   (string_ptr,
				BT$F1_LEN + BT$F2_LEN + BT$F3_LEN,
				BT$F4_LEN,
				BT$F4_LEN - len);
		}
	    break;
	default:
	    break;
	}
    first = FALSE;
    }
while ((get_next_field_dsd(ctx)) == DD$SUCCESS);
return(UE$SUCC);
}
/*...	ENDROUTINE BLD_REG_STRING					    */



/*
*	.SBTTL	BLD_INVALID_STRING - Builds invalid output
*
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine is called to build a string
*	from invalid input.
*	
* CALLING SEQUENCE:		CALL BLD_INVALID_STRING (..See Below..)
*
* FORMAL PARAMETERS:		ctx
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		Final formatted output string
*	
* ROUTINE VALUE:		UE$SUCC  if successful
*				UE$FAIL  if failure
*	
* SIDE EFFECTS:			Creates the final output
*
*--
*/
/*...	ROUTINE BLD_INVALID_STRING (ctx)	    */
long  bld_invalid_string (ctx)

DD$STD_DSD_CTX *ctx;

{
unsigned short sht;
unsigned long  lng;

printf("%*s",BT$F2_LEN, BT$INV_ITEM);
printf("%*s",BT$F3_LEN,BT$SPACE);		/* skip filler		*/
switch (ctx->item_DSD_ptr->TYPE)
    {
    case (DT_LONG):
    case (DT_INDEXED):
    case (DT_REGISTER):
	lng = *((long *) ctx->item_ptr);
	printf("x%-08.8X", lng);
	break;
    case (DT_SHORT):
    case (DT_SHORT_INDEX):
    case (DT_SHORT_REGISTER):
	sht = *((short *) ctx->item_ptr);
	printf("x%-04.4X", sht);
	break;
    }
}
/*...	ENDROUTINE BLD_INVALID_STRING					    */



/*
*	.SBTTL	BLD_DMY_STRING - Builds date  and time formated output
*
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine is called to build formatted
*	date and time output of the given input.
*	
* CALLING SEQUENCE:		CALL BLD_DMY_STRING (..See Below..)
*
* FORMAL PARAMETERS:		
*
*	std_item		pointer to the input field
*	dsd_item		pointer to the field descriptor
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		Final formatted output string
*	
* ROUTINE VALUE:		UE$SUCC  if successful
*				UE$FAIL  if failure
*	
* SIDE EFFECTS:			Creates the final output
*
*--
*/
/*...	ROUTINE BLD_DMY_STRING (bindate)	    */
long  bld_dmy_string (bindate)

long  *bindate;

{
struct timeval  tv, *tvp;
struct timezone tz, *tzp;
struct tm       lt, *ltp;

printf("%*s",BT$F2_LEN,BT$SPACE);		/* skip numeric field	*/
printf("%*s",BT$F3_LEN,BT$SPACE);		/* skip filler		*/
if (*bindate == UE$NULL)
    printf("NULL Date & Time");
else
    {
    tvp = &tv;
    tzp = &tz;
    ltp = &lt;
    gettimeofday(tvp,tzp);
    ltp = localtime(bindate);
    printf("%-24.24s %s", ctime(bindate),	/* print string */
			  timezone(tzp->tz_minuteswest,ltp->tm_isdst));
    }
}
/*...	ENDROUTINE BLD_DMY_STRING					    */



/*
*	.SBTTL	PRINT_STRING - Prints out a string on the right side of
*				the screen.
*
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine is called to print out a char string within
*	th boundries of the field on the right hand side of the screen.
*	
* CALLING SEQUENCE:		CALL PRINT_STRING (..See Below..)
*
* FORMAL PARAMETERS:		
*
*	str_ptr 		pointer to the input field
*	wrap_col		starting column for wrap
*	buflen			size of buffer for lines
*	this_buf		size of buffer for line 1
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		Final formatted output string
*	
* ROUTINE VALUE:		UE$SUCC  if successful
*				UE$FAIL  if failure
*	
* SIDE EFFECTS:			Creates the final output
*
*--
*/
/*...	ROUTINE PRINT_STRING (str_ptr,wrap_col,buflen,this_buf)	    */

long  print_string (str_ptr,wrap_col,buflen,this_buf)
DD$BYTE  *str_ptr;
short wrap_col;
short buflen;
short this_buf;

{
short i;
short len;
short offset;
short num_put;
char   line[LINESIZE];

len = strlen(str_ptr);
num_put = 0;

tran_tbl['\0'] = '\0';
tran_tbl['\t'] = '\t';
tran_tbl['\n'] = '\n';

for (i = 0; i < len ; i++)
    {
    str_ptr[i] = tran_tbl[str_ptr[i]];
    }

while (len > 0)
    {
    offset = strcspn(str_ptr," \t\n\0");
    if ((offset + num_put) > this_buf)
	{
	printf("\n%*s _", wrap_col, BT$SPACE);
	num_put = 2;
	this_buf = buflen;
	}
    printf("%.*s ",(offset), str_ptr);
    len     -= (offset + 1);
    num_put += (offset + 1);
    if ((len != 0) && (str_ptr[offset] == '\n'))
	{
	printf("\n%*s", wrap_col, BT$SPACE);
	num_put = 0;
	}
    str_ptr += (offset + 1);
    }
return(UE$SUCC);
}
/*...	ENDROUTINE PRINT_STRING					    */



/*
*	.SBTTL	HEX_DUMP     - function used to print a hex dump
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  this function receives the pointer to an area in memory
*		and a length and it prints a HEX dump of it.
*
* FORMAL PARAMETERS:		Pointer to area in memory.
*				Length of area to dump.
*
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* COMPLETION STATUS:		NONE
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	FUNCTION HEX_DUMP         			*/

long  hex_dump(ptr,len,size,addr)
DD$BYTE *ptr;
short   len;
short   size;
long    addr;

{

short  offset = 0;
short  i;
short  j;
char   line[LINESIZE];
static char hex_tbl[17] = "0123456789ABCDEF";

tran_tbl['\0'] = '.';
tran_tbl['\t'] = '.';
tran_tbl['\n'] = '.';

if (size != 2)
    size =  4;

for (;;)
    {
    for (i = 0; i < LINESIZE; i++)
	line[i] = ' ';
    line[44] = '*';
    line[61] = '*';
  
    printf("\n%04.4X: ", (addr+offset));
    for (j = 0; j < (16/size); j++)
	{
	for (i = size; i > 0; i--, offset++)
	    {
	    line[(j*(size*2+2))+(i*2)]   = hex_tbl[(ptr[offset]>>4)&(0x0f)];
	    line[(j*(size*2+2))+(i*2)+1] = hex_tbl[(ptr[offset]   )&(0x0f)];
	    }
        for (i = 0; i < size; i++)
	    {
            line[45 + i + (j * size)] = tran_tbl[ptr[offset - size + i]];
	    }
	if (len <= offset)
	    {
            for (i = 0; i < (len - offset); i++)
	        {
                line[45 + i] = tran_tbl[ptr[offset - 16 + i]];
	        }
	    printf("%*.*s", LINESIZE, LINESIZE, line);
	    return;
	    }
	}
    printf("%*.*s", LINESIZE, LINESIZE, line);
    }
}

/*...   ENDFUNCTION HEX_DUMP			*/
char *ctime();

unsigned long
vms_to_unix_time(date_low, date_high)
unsigned long date_low, date_high;
{
  unsigned long divisor, dividend_low, dividend_high, quotient, remainder;
  unsigned long subtrahend_low, subtrahend_high, dif_low, dif_high;
  int i;

  dif_low = date_low;
  dif_high = date_high;
  subtrahend_low = 0x4beb4000; /* difference between 00:00 jan 1 1970 and */
  subtrahend_high = 0x7c9567;  /* 00:00 nov 17 1858 in 100nanoseconds */

	/* returns 1 if result is negative, else returns 0 */
  i=subq(subtrahend_low, subtrahend_high, &dif_low, &dif_high);
  if(i){
	return((long)0);
  }
  dividend_low = dif_low;
  dividend_high = dif_high;
  divisor = 0x989680;
  quotient = 0;
  remainder = 0;
  divq(divisor, dividend_low, dividend_high, &quotient, &remainder);
  return(quotient);
}
