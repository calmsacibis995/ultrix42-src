#ifndef lint
static char sccsid[]  =  "@(#)eixfrm.c	4.3   (ULTRIX)   9/11/90"; 
#endif  lint

/* DEC/CMS REPLACEMENT HISTORY, Element EIXFRM.C*/
/* *1    20-FEB-1986 13:14:41 EMLICH "Raw to standard transformation engine - first base-level"*/
/* DEC/CMS REPLACEMENT HISTORY, Element EIXFRM.C*/
/*
*	.TITLE	EIXFRM - Transformation system for ERIT
*	.IDENT	/1-001/
*
* COPYRIGHT (C) 1986 DIGITAL EQUIPMENT CORP.,
* CSSE SOFTWARE ENGINEERING
* MARLBOROUGH, MASSACHUSETTS
*
* THIS SOFTWARE IS FURNISHED UNDER A LICENSE FOR USE ONLY ON A 
* SINGLE COMPUTER SYSTEM AND MAY BE COPIED ONLY WITH THE INCLUSION
* OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE,  OR ANY OTHER
* COPIES THEREOF, MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE
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
* FACILITY:             ERIT - Event Record Input Transformer
*
* ABSTRACT:
*
*	This module contains the code for making transformations
*       between the raw entry and the standard entry. It is called by
*       the ERIT process to build one segment. This module calls the
*       DSD system to get raw and standard definitions and to set the
*       field validity code.
*	
* ENVIRONMENT:	VAX/VMS C,  ULTRIX-32 C, (and hopefully beyond)
*
* AUTHOR:  Larry Emlich,  CREATION DATE:  22-Nov-85 (templates)
*                                          7-Feb-86 (code)
*
* MODIFIED BY:
*
*
*--
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include "erms.h"
#include "eiliterals.h"   /* For return codes  */
#include "generic_dsd.h"  /* For DSD interface */
#include "std_dsd.h" 	  /* For DSD interface */
#include "os_dsd.h" 	  /* For DSD interface */

/*
*++
*=
*=
*=MODULE FLOW - eixfrm.c
*=
*=  a - os_std(os_ptr,os_id,std_id)                 OS_item moved & transformed
*=                                                  to the std segment
*=          find_os_item_dsd(os_id)                         (dsd_access.c)
*=          decode_os_item(os_id,data)                      (dsd_access.c)
*=          chk_fld(item_ptr)                               (* c)
*=          set_validity_code(seg,seq,val)                  (dsd_access.c)
*=
*=  b - ini_seg(seg_buf)                            Info for seg_items saved
*=          get_std_segment_dsd(ctx)                        (dsd_access.c)
*=          get_next_item_dsd(ctx)                          (dsd_access.c)
*=
*=  c - chk_fld()                                   Null routine (for future
*=                                                  field value checking)
*=
*
*--
*/


/*
*	.SBTTL	INTERNAL_DATA - Internal structures and stuff
*++
* FUNCTIONAL DESCRIPTION:		
*
*     Structures and other things global to this module but not
*     useful outside it.
*--
*/

/********************  STUCTURE FOR SEGMENT INFO ********************/

struct
    {
    DD$STD_HEADER_PTR	beg_seg;	/* Begining of segment		*/
    char 		*end_seg;	/* End of segment		*/
    char 		*str_loc;	/* next string storage area     */
    short   		seg_size;	/* segment size			*/
    }  seg[] =
    {
    {0,0,0,0},
    {0,0,0,ES$EISIZE},		/* ES$EIS is 1 */
    {0,0,0,ES$DISIZE},		/* ES$DIS is 2 */
    {0,0,0,ES$SDSIZE},		/* ES$SDS is 3 */
    {0,0,0,ES$CDSIZE},		/* ES$CDS is 4 */
    {0,0,0,ES$ADSIZE},		/* ES$ADS is 5 */
    {0,0,0,ES$SISIZE},		/* ES$SIS is 6 */
    {0,0,0,ES$CISIZE}		/* ES$CIS is 7 */
    };

/*********************  STRUCTURES FOR STD ITEM INFO  *************/

ITEM_INFO       item_info[300];
short           next_item;

/*******************  MODULE WIDE FUNCTIONS ****************/

long    chk_fld();

long    get_next_item_dsd();	/* external functions		*/
long    set_validity_code();
long    get_std_segment_dsd();
DD$OS_ITEMS_DSD_PTR find_os_item_dsd();



/********************   MACRO  ************************/

/******************** DECODE_RTN ***********************/

#define DECODE_RTN(src_type, dst_type) \
    if ((*((dst_type *) item_info[i].item_ptr) = \
	  decode_os_item (raw_id, gen_item.gen_long)) \
	  == DD$UNKNOWN_CODE) \
	    { \
	    s = DD$N_V; \
	    *((dst_type *) item_info[i].item_ptr) = \
	    (src_type) gen_item.gen_long;\
	    }\
        break;


/********************** END OF MACROS ******************/



/*
*	.SBTTL	OS_TO_STD - Moves data from the OS record to the 
*                           STD segment.
*++
* FUNCTIONAL DESCRIPTION:		
*
*    Controls the transformation of raw data to a standard segment.
*	
* CALLING SEQUENCE:		CALL OS_TO_STD (..See Below..)
*                                   Called by EI$BLD once per item.
*
* FORMAL PARAMETERS:		raw item pointer
*                               raw item type
*                               std item id
*
* IMPLICIT INPUTS:              type-move matrix
*
* IMPLICIT OUTPUTS:		NONE
*                  
* COMPLETION CODES:             Transformation successful (success)
*                                   (may have invalid fields)  
*                               Fatal Program error
*
* SIDE EFFECTS:                 Lower routines fill the segment buffer
*              
*--
*/
/*...	ROUTINE OS_TO_STD ()		    */
long os_std (raw_item, raw_id, std_id)

DD$BYTE *raw_item;
short   raw_id;
short   std_id;

{
long size;
long len;
long s;				/* for status			*/
union {
      long   gen_long;
      char   *gen_string;
      short  gen_short;
      char   gen_char[4];
      } gen_item;
long item;
short i, j;
short s_type;
short raw_type;
short std_type;

DD$OS_ITEMS_DSD_PTR os_item_dsd;

for (i = next_item - 1; i >= 0; i--)
    {
    if (std_id == item_info[i].id)	/* find std item in item_info table */
	break;
    }
if (std_id != item_info[i].id)
    {
    return EI$FAIL;
    }

s_type = item_info[i].seg_type;
os_item_dsd = find_os_item_dsd(raw_id);
if (os_item_dsd == DD$UNKNOWN_ITEM)
    return EI$FAIL;
s = DD$VALID;		/* preset to valid */

raw_type = os_item_dsd->TYPE;
std_type = item_info[i].type;
/**********************************************************************
 ************ the following is moved by chars to guarantee  ***********
 ************ alignment for cpus such as PMAX               ***********
 **********************************************************************/

switch ((100 * raw_type) + std_type)
    {
    case (100 * DT_LONG                 + DT_LONG):
    case (100 * DT_LONG                 + DT_INDEXED):
    case (100 * DT_LONG                 + DT_REGISTER):
    case (100 * DT_REGISTER             + DT_REGISTER):
    case (100 * DT_DATE                 + DT_DATE):
    case (100 * DT_LONG                 + DT_DATE):
	((char *) item_info[i].item_ptr)[0] = *(raw_item+0);
	((char *) item_info[i].item_ptr)[1] = *(raw_item+1);
	((char *) item_info[i].item_ptr)[2] = *(raw_item+2);
	((char *) item_info[i].item_ptr)[3] = *(raw_item+3);
    break;

    case (100 * DT_LONG                 + DT_VMS_TIME):
	((char *) item_info[i].item_ptr)[0] = *(raw_item+0);
	((char *) item_info[i].item_ptr)[1] = *(raw_item+1);
	((char *) item_info[i].item_ptr)[2] = *(raw_item+2);
	((char *) item_info[i].item_ptr)[3] = *(raw_item+3);
	((char *) item_info[i].item_ptr)[4] = *(raw_item+4);
	((char *) item_info[i].item_ptr)[5] = *(raw_item+5);
	((char *) item_info[i].item_ptr)[6] = *(raw_item+6);
	((char *) item_info[i].item_ptr)[7] = *(raw_item+7);
    break;

    case (100 * DT_LONG                 + DT_SHORT):
    case (100 * DT_LONG                 + DT_SHORT_INDEX):
	((char *) item_info[i].item_ptr)[0] = *(raw_item+0);
	((char *) item_info[i].item_ptr)[1] = *(raw_item+1);
    break;

    case (100 * DT_SHORT                + DT_SHORT):
    case (100 * DT_SHORT                + DT_SHORT_INDEX):
    case (100 * DT_SHORT                + DT_SHORT_REGISTER):
    case (100 * DT_SHORT_REGISTER       + DT_SHORT_REGISTER):
	((char *) item_info[i].item_ptr)[0] = *(raw_item+0);
	((char *) item_info[i].item_ptr)[1] = *(raw_item+1);
    break;

    case (100 * DT_SHORT                + DT_LONG):
    case (100 * DT_SHORT                + DT_INDEXED):
    case (100 * DT_SHORT                + DT_REGISTER):
    case (100 * DT_SHORT_REGISTER       + DT_REGISTER):
	((char *) item_info[i].item_ptr)[0] = *(raw_item+0);
	((char *) item_info[i].item_ptr)[1] = *(raw_item+1);
    break;

    case (100 * DT_TINY                 + DT_LONG):
    case (100 * DT_TINY                 + DT_REGISTER):
	((char *) item_info[i].item_ptr)[0] = *(raw_item+0);
    break;

    case (100 * DT_TINY                 + DT_SHORT):
    case (100 * DT_TINY                 + DT_SHORT_REGISTER):
	((char *) item_info[i].item_ptr)[0] = *(raw_item+0);
    break;

    case (100 * DT_INDEXED              + DT_INDEXED):
	gen_item.gen_long = 0;
	gen_item.gen_char[0] = *(raw_item+0);
	gen_item.gen_char[1] = *(raw_item+1);
	gen_item.gen_char[2] = *(raw_item+2);
	gen_item.gen_char[3] = *(raw_item+3);
	DECODE_RTN(long, long );

    case (100 * DT_INDEXED              + DT_SHORT_INDEX):
	gen_item.gen_long = 0;
	gen_item.gen_char[0] = *(raw_item+0);
	gen_item.gen_char[1] = *(raw_item+1);
	DECODE_RTN(long, short);

    case (100 * DT_SHORT_INDEX          + DT_SHORT_INDEX):
	gen_item.gen_long = 0;
	gen_item.gen_char[0] = *(raw_item+0);
	gen_item.gen_char[1] = *(raw_item+1);
	DECODE_RTN(short, short);

    case (100 * DT_SHORT_INDEX          + DT_INDEXED):
	gen_item.gen_long = 0;
	gen_item.gen_char[0] = *(raw_item+0);
	gen_item.gen_char[1] = *(raw_item+1);
	DECODE_RTN(short, long );

    case (100 * DT_TINY_INDEX           + DT_SHORT_INDEX):
	gen_item.gen_long = 0;
	gen_item.gen_char[0] = *(raw_item+0);
	DECODE_RTN(DD$BYTE, short);

    case (100 * DT_TINY_INDEX           + DT_INDEXED):
	gen_item.gen_long = 0;
	gen_item.gen_char[0] = *(raw_item+0);
	DECODE_RTN(DD$BYTE, long );

    case (100 * DT_ASCIZ                + DT_STRING):
        len = strlen(raw_item);
        if (os_item_dsd->COUNT != 0 && os_item_dsd->COUNT < len)
            len = os_item_dsd->COUNT;
        if (strncmp(raw_item,trailer,4) == 0)
	    len = 0;
        if (len < (seg[s_type].end_seg - seg[s_type].str_loc))
	    {
            (void)strncpy(seg[s_type].str_loc, raw_item, len);
            seg[s_type].str_loc[len] = '\0';
            *((long *)item_info[i].item_ptr) = (long)seg[s_type].str_loc;
            seg[s_type].str_loc = seg[s_type].str_loc + (len + 1);
            }
        break;

    case (100 * DT_BYTE_VECTOR          + DT_BYTE_VECTOR):
	for (j = 0; j < item_info[i].size; j++)
	    ((char *) item_info[i].item_ptr)[j] = *(raw_item+j);
        break;

    case (100 * DT_COUNTED_LONG_VECTOR  + DT_COUNTED_LONG_VECTOR):
	gen_item.gen_long = 0;
	gen_item.gen_char[0] = *(raw_item+0);
	gen_item.gen_char[1] = *(raw_item+1);
	gen_item.gen_char[2] = *(raw_item+2);
	gen_item.gen_char[3] = *(raw_item+3);
	len  = gen_item.gen_long / sizeof(long);
	if ((len > item_info[i].size) || (len < 0))
	    len = item_info[i].size;
	*((long *)item_info[i].item_ptr) = len;
	item_info[i].item_ptr += 4;
        len *= sizeof(long);
	for (j = 0; j < len; j++)
	    ((char *)item_info[i].item_ptr)[j] = *(raw_item+4+j);
        break;

    case (100 * DT_LONG_VECTOR          + DT_COUNTED_LONG_VECTOR):
	len  = item_info[i].size;
	*((long *)item_info[i].item_ptr) = len;
	item_info[i].item_ptr += 4;
        len *= sizeof(long);
	for (j = 0; j < len; j++)
	    ((char *)item_info[i].item_ptr)[j] = *(raw_item+j);
        break;

    case (100 * DT_ADDR_CNT_VECTOR      + DT_ADDR_CNT_VECTOR):
	((char *) item_info[i].item_ptr)[0] = *(raw_item+0);
	((char *) item_info[i].item_ptr)[1] = *(raw_item+1);
	((char *) item_info[i].item_ptr)[2] = *(raw_item+2);
	((char *) item_info[i].item_ptr)[3] = *(raw_item+3);
	item_info[i].item_ptr += 4;

	gen_item.gen_char[0] = *(raw_item+4);
	gen_item.gen_char[1] = *(raw_item+5);
	gen_item.gen_char[2] = *(raw_item+6);
	gen_item.gen_char[3] = *(raw_item+7);
	len  = gen_item.gen_long / sizeof(long);

	if ((len > item_info[i].size) || (len < 0))
	    len = item_info[i].size;
	*((long *)item_info[i].item_ptr) = len;
	item_info[i].item_ptr += 4;
        len *= sizeof(long);
	for (j = 0; j < len; j++)
	    ((char *)item_info[i].item_ptr)[j] = *(raw_item+8+j);
        break;
		    
    default:
	s = DD$N_V$N_A;
    break;
    }		/* end of switch construct	*/
/********************************************************************/

    if (s == DD$VALID)
	{
        s = chk_fld(item_info[i].item_ptr /* , seg_id.valid */);
	}
set_validity_code(seg[s_type].beg_seg, item_info[i].seq_num, s);
return EI$SUCC;
}

/*...	ENDROUTINE OS_TO_STD		    */

/*
*	.SBTTL	INI_SEG
*++
* FUNCTIONAL DESCRIPTION:		
*
*    Initialize item offsets per segment passed. It also initializes
*    string storage areas.
*	
* CALLING SEQUENCE:		CALL INI_SEG (..See Below..)
*                                    Usually called by EI$BLD  
*
* FORMAL PARAMETERS:		Pointer to segment buffer
*				type, subtype, and version is filled
*				 by ei$bld and/or get_subtypes.
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		Item information filled.
*				string tracking block filled
*
* COMPLETION CODES:             Success or program error (from lower level)
*
* SIDE EFFECTS:			Segment buffer is cleared by lower routine
*--
*/
/*...	ROUTINE INI_SEG ()				    */

long ini_seg (seg_buf)
DD$STD_HEADER_PTR seg_buf;

{
static DD$STD_DSD_CTX seg_ctx;
short i;
short s_type;			/* segment type */

if(seg_buf == 0)		/* clear item table	*/
    {
    next_item = 0;		/* New record start again */
    return EI$SUCC;
    }
s_type = seg_buf->type;

if(s_type == ES$EIS)
    next_item = 0;		/* New record start again */
seg_ctx.segment_ptr = seg_buf;

if (get_std_segment_dsd(&seg_ctx))
    {                             /* fill in item info for each */
				  /* item in segment		*/

    for(i = 1; i <= seg_ctx.segment_DSD_ptr->COUNT; i++)
	{
	item_info[next_item].id       = seg_ctx.item_DSD_ptr->ID;
	item_info[next_item].type     = seg_ctx.item_DSD_ptr->TYPE;
	item_info[next_item].size     = seg_ctx.item_DSD_ptr->COUNT;
	item_info[next_item].seq_num  = i;
	item_info[next_item].item_ptr = (char *)seg_ctx.item_ptr;
	item_info[next_item].seg_type = s_type;
	next_item++;
	get_next_item_dsd(&seg_ctx);
	}
                                   /* clear up next entry */

    item_info[next_item].id       = 0;
    item_info[next_item].type     = 0;
    item_info[next_item].size     = 0;
    item_info[next_item].seq_num  = 0;
    item_info[next_item].item_ptr = NULL;
    item_info[next_item].seg_type = 0;

				/* initialize information for  */
				/* string moving in this segment */

    seg[s_type].beg_seg =		/* start of segment */
		seg_ctx.segment_ptr;
    seg[s_type].str_loc =		/* start of free area */
	        (char *)seg_ctx.item_ptr;
    seg[s_type].end_seg = ((char *)seg_buf + seg[s_type].seg_size);
    return EI$SUCC;
    }
return EI$FAIL;  /* Someone failed & should print error message */

} 
/*...	ENDROUTINE INI_SEG 				    */

/*
*	.SBTTL	CHK_FLD
*++
* FUNCTIONAL DESCRIPTION:		
*
*    Called when a field in the raw or segment buffer must be validity-checked.
*    This routine dispatches to another routine which specializes in the
*    particular kind of validity (valid set, within range, etc).
*	
* CALLING SEQUENCE:		CALL CHK_FLD (..See Below..)
*                                   Called before transformation when
*                                   checking raw data and after to validate
*                                   standard.
*
* FORMAL PARAMETERS:		Field value
*                               Validity parameters
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* COMPLETION CODES:		Valid field (success)
*                               Invalid field (error)
*                               Bad validity parameters (fatal)
*
* SIDE EFFECTS:                 none
*--
*/
/*...	ROUTINE CHK_FLD ()					    */

long chk_fld (val_loc,valid_param)
char *val_loc;
int *valid_param;

{

/* If one of list, call routine to do that kind of validity */

/* If "within range", call routine to do that kind of validity */

/* If something else, do that too */

/* Return what we get from specific validity check routine */

    return DD$VALID;

} /*...	ENDROUTINE CHK_FLD					    */

