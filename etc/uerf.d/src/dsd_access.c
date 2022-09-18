#ifndef lint
static char sccsid[]  =  "@(#)dsd_access.c	4.2   (ULTRIX)   9/11/90";
#endif  lint
/*
**	.title DSD Table Access Functions
**	.ident / 1.16 /
**
**
**	  File:	dsd_access.c
** Description:	DSD Table Access Functions
**	Author:	Paul Baker
**	  Date:	7-Oct-1986
**
**
**	Copyright 1986, Digital Equipment Corporation
**
**
**++
**	The DSD table access functions interface either directly to the
**	application, or indirectly through the DSD services functions.
**	The DSD table access functions can not read or manipulate the
**	data-segment.
**
**	These functions do not provide simultaneous access to the STD
**	and O/S DSD tables.
**--
**
**
**
*/

#include <stdio.h>
#include "dsd_switch.h"		/* Compilation control switches */
#include "generic_dsd.h"	/* DSD table structure definitions */
#include "std_dsd.h"		/* Standard event Data Structure Definitions */
#include "os_dsd.h"		/* O/S event Data Structure Definitions */


/*
*++
*=
*=
*=MODULE FLOW - dsd_access.c
*=
*=  a - dsd_init(bin_file)                          Reads bin file and creates
*=                                                  arrays for each DSD.
*=          open(bin_file)
*=          while(!EOF)
*=              malloc(DSD_array_size)
*=              read(DSD_arrays)
*=          return(DD$SUCCESS)
*=
*=  b - get_bin_file_ver()
*=          return(bin_file_ver)
*=
*=  c - get_item_index(item_id)
*=          return(std_item_dsd_index)
*=
*=  d - get_item_size(data_type,length)
*=          return(item_size)
*=
*=  e - fld_align(data_ptr,data_type)
*=          return(aligned_data_ptr)
*=
*=  f - get_std_segment_dsd(*ctx)                   Initializes ctx for 1st
*=                                                  item in segment.
*=          find_std_segment_dsd(seg_type,seg_subtype)      (* o)
*=          fld_align(data_ptr,data_type)                   (* e)
*=          get_validity_code(ctx)                          (* aa)
*=          return(DD$SUCCESS)
*=
*=  g - get_next_item_dsd(*ctx)                     Updates ctx for next item.
*=          get_item_size(item_type,length)                 (* d)
*=          fld_align(data_ptr,data_type)                   (* e)
*=          get_validity_code(ctx)                          (* aa)
*=          return(DD$SUCCESS)
*=
*=  h - get_next_field_dsd(*ctx)                    Updates ctx for next reg
*=                                                  field.
*=          return(DD$SUCCESS)
*=
*=  i - decode_std_item(*ctx,item_code)             Finds text string to coded
*=                                                  item in ctx.
*=          dsd_get_label(index)                            (* n)
*=          return(string_ptr)
*=
*=  j - decode_register_field(*ctx,field_code)      Finds text string to coded
*=                                                  field in ctx.
*=          dsd_get_label(index)                            (* n)
*=          return(string_ptr)
*=
*=  k - get_reg_fld_code(item_id,fld_ix,fld_code)   Finds text string to coded
*=                                                  field in param. Used for
*=                                                  out of order processing.
*=          find_std_item_dsd(item_id)                      (* u)
*=          get_reg_dsd(index)                              (* s)
*=          get_code_dsd(index)                             (* r)
*=          dsd_get_label(index)                            (* n)
*=          return(string_ptr)
*=   
*=  l - get_code_std_item(item_id,code_string)      finds item_code for the
*=                                                  given text string within
*=                                                  the given item_id.
*=          find_std_item_dsd(item_id)                      (* u)
*=          tolower(code_string)
*=          find_std_code_dsd(item_dsd, index)              (* v)
*=          dsd_get_label(index)                            (* n)
*=          return(code)
*=
*=  m - decode_os_item(item_id,item_code)           Finds STD code for item
*=                                                  from OS code
*=          find_os_item_dsd(item_id)                       (* x)
*=          return(STD_code)
*=
*=  n - dsd_get_label(label_index)                  Finds pointer to the text
*=                                                  string.
*=          return(string_ptr)
*=
*=  o - find_std_segment_dsd(seg_type,seg_subtype)  Finds requested segment.
*=          return(segment_dsd_ptr)
*=
*=  p - get_item_offset(seg_ptr,item_id)            Finds item offset in seg
*=          return(item_offset)
*=
*=  q - get_seg_item_dsd(index)                     Finds seg_item_dsd @ index.
*=          return(seg_item_dsd_ptr)
*=
*=  r - get_code_dsd(index)                         Finds std_code_dsd @ index.
*=          return(std_codes_dsd_ptr)
*=
*=  s - get_reg_dsd(index)                          Finds reg_dsd @ index.
*=          return(std_regs_dsd_ptr)
*=
*=  t - get_item_dsd(index)                         Finds item_dsd @ index.
*=          return(std_items_dsd_ptr)
*=
*=  u - find_std_item_dsd(item_id)                  Finds item_dsd for item_id.
*=          return(std_items_dsd_ptr)
*=
*=  v - find_std_code_dsd(item_dsd_ptr,code_index)  Finds code_dsd for
*=                                                  item/code.
*=          return(std_codes_dsd_ptr)
*=
*=  w - find_reg_code_dsd(reg_dsd_ptr,code_index)   Finds code_dsd for
*=                                                  item/code.
*=          return(std_codes_dsd_ptr)
*=
*=  x - find_os_item_dsd(os_item_id)                Finds os_item_dsd for
*=                                                  os_item.
*=          return(os_items_dsd_ptr)
*=
*=  y - find_label_dsd(index)                       Finds dsp_labels @ index.
*=          return(dsp_labels_ptr)
*=
*=  z - set_validity_code(*seg,item_seq,val_code)   Sets valid bits in segment.
*=          return()
*=
*=  aa- get_validity_code(*ctx)                     Gets validity value for
*=                                                  current item.
*=
*=
*--
*/

#define std_segment	1
#define os_record	2

#define MAX_STRING	256	/* Maximum size of a NAME or LABEL string */

/**********************************************************************/

long		os_codes_count;
long		os_items_count;
long		std_codes_count;
long		std_regs_count;
long		std_items_count;
long		std_seg_items_count;
long		std_segs_count;
long		labels_count;
long		strings_area_count;

long		bin_file_ver;

DD$OS_CODES_DSD_PTR		os_codes;
DD$OS_ITEMS_DSD_PTR		os_items;
DD$STD_CODES_DSD_PTR		std_codes;
DD$STD_REGS_DSD_PTR		std_regs;
DD$STD_ITEMS_DSD_PTR		std_items;
DD$STD_SEG_ITEMS_DSD_PTR	std_seg_items;
DD$STD_SEGS_DSD_PTR		std_segs;
DD$DSP_LABELS_PTR		labels;
char				*strings_area;

/********************************************************************/
/****************  FUNCTIONS USED IN THIS MODULE ********************/

short			get_item_size ();
char			*dsd_get_label ();
DD$BYTE			*fld_align();
DD$STD_SEGS_DSD_PTR	find_std_segment_dsd ();
DD$STD_ITEMS_DSD_PTR	find_std_item_dsd ();
DD$STD_CODES_DSD_PTR	find_std_code_dsd ();
DD$OS_ITEMS_DSD_PTR	find_os_item_dsd ();
DD$STD_CODES_DSD_PTR    get_code_dsd ();
DD$STD_REGS_DSD_PTR     get_reg_dsd ();

char   			*malloc();


/**********************  BIN FILE LOADING  ***************************
 *
 *	INPUT		bin_file full name
 *
 *	OUTPUT		arrays of all data items
 *
 *********************************************************************/

dsd_init (bin_file)
char	*bin_file;
{

short	ifd;			/* Input file descriptor */
short   i;
long	count;

if ((ifd = open(bin_file, 0)) == -1)
    {
    printf("\nCan't open input file %s!\n",bin_file);
    return(DD$BAD_BIN_FILE);
    }

while (read(ifd, &i, 2) == 2)		/* read array type	*/
    {
    if (read(ifd, &count, 4) != 4)	/* read array inexes	*/
	return (DD$BAD_BIN_FILE);
    switch (i)
	{
	case DD$BIN_FILE_VER_ID:
	  if(read(ifd, &bin_file_ver, 4) != 4)
	      return(DD$BAD_BIN_FILE);
	break;
	case DD$OS_CODES_ID:
	  os_codes_count = count;
	  count = ((count + 1) * sizeof(DD$OS_CODES_DSD));
	  os_codes = (DD$OS_CODES_DSD_PTR) malloc(count);
	  if (read(ifd, os_codes, count) != count)
	      return(DD$BAD_BIN_FILE);
	break;
	case DD$OS_ITEMS_ID:
	  os_items_count = count;
	  count = ((count + 1) * sizeof(DD$OS_ITEMS_DSD));
	  os_items = (DD$OS_ITEMS_DSD_PTR) malloc(count);
	  if (read(ifd, os_items, count) != count)
	      return(DD$BAD_BIN_FILE);
	break;
	case DD$STD_CODES_ID:
	  std_codes_count = count;
	  count = ((count + 1) * sizeof(DD$STD_CODES_DSD));
	  std_codes = (DD$STD_CODES_DSD_PTR) malloc(count);
	  if (read(ifd, std_codes, count) != count)
	      return(DD$BAD_BIN_FILE);
	break;
	case DD$STD_REGS_ID:
	  std_regs_count = count;
	  count = ((count + 1) * sizeof(DD$STD_REGS_DSD));
	  std_regs = (DD$STD_REGS_DSD_PTR) malloc(count);
	  if (read(ifd, std_regs, count) != count)
	      return(DD$BAD_BIN_FILE);
	break;
	case DD$STD_ITEMS_ID:
	  std_items_count = count;
	  count = ((count + 1) * sizeof(DD$STD_ITEMS_DSD));
	  std_items = (DD$STD_ITEMS_DSD_PTR) malloc(count);
	  if (read(ifd, std_items, count) != count)
	      return(DD$BAD_BIN_FILE);
	break;
	case DD$STD_SEG_ITEMS_ID:
	  std_seg_items_count = count;
	  count = ((count + 1) * sizeof(DD$STD_SEG_ITEMS_DSD));
	  std_seg_items = (DD$STD_SEG_ITEMS_DSD_PTR) malloc(count);
	  if (read(ifd, std_seg_items, count) != count)
	      return(DD$BAD_BIN_FILE);
	break;
	case DD$STD_SEGS_ID:
	  std_segs_count = count;
	  count = ((count + 1) * sizeof(DD$STD_SEGS_DSD));
	  std_segs = (DD$STD_SEGS_DSD_PTR) malloc(count);
	  if (read(ifd, std_segs, count) != count)
	      return(DD$BAD_BIN_FILE);
	break;
	case DD$DSP_LABELS_ID:
	  labels_count = count;
	  count = ((count + 1) * sizeof(DD$DSP_LABELS));
	  labels = (DD$DSP_LABELS_PTR) malloc(count);
	  if (read(ifd, labels, count) != count)
	      return(DD$BAD_BIN_FILE);
	break;
	case DD$STRINGS_ID:
	  strings_area_count = count++;
	  strings_area = malloc(count);
	  if (read(ifd, strings_area, count) != count)
	      return(DD$BAD_BIN_FILE);
	break;
	default:
	  return(DD$BAD_BIN_FILE);
	break;
        }
    }
return (DD$SUCCESS);
}


/********************** GET_BIN_FILE_VERSION ********************
 *
 *		INPUT		
 *
 *		OUTPUT		bin_file_ver
 *
 ****************************************************************/

long  get_bin_file_ver()
{

return bin_file_ver;
}



/********************** GET_ITEM_INDEX FUNCTION *****************
 *
 *		INPUT		item_id
 *
 *		OUTPUT		item_index
 *
 ****************************************************************/

long  get_item_index (item_id)
short item_id;
{

short	i;

for (i = 0; i < std_items_count; i++)
    if (item_id == std_items[i].ID)
	return i;

printf("\nUnknown data-id %d in \"get_item_index\"!\n", item_id); 
return 0;
}


/**********************  GET ITEM SIZE FUNCTION *****************
 *
 *		INPUT		data_type
 *				length (if vector)
 *
 *		OUTPUT		storage size
 *
 ****************************************************************/

short get_item_size (data_type, length)
short data_type;
short length;
{
short size;
switch (data_type)
    {
    case DT_TINY :
    case DT_TINY_INDEX :
        size = 1;
        break;
    case DT_SHORT :
    case DT_SHORT_INDEX :
    case DT_SHORT_REGISTER :
        size = 2;
        break;
    case DT_LONG :
    case DT_INDEXED :
    case DT_REGISTER :
    case DT_DATE :
        size = 4;
        break;
    case DT_VMS_TIME :
        size = 8;
        break;
    case DT_BYTE_VECTOR :
    case DT_ASCIZ :
    case DT_BIT_VECTOR :
        size = length;
        break;
    case DT_COUNTED_SHORT_VECTOR :
        size = (length + 1) * 2;		/* + 1 is for the length word */
        break;
    case DT_COUNTED_LONG_VECTOR :
        size = (length + 1) * 4;		/* + 1 is for the length word */
        break;
    case DT_ADDR_CNT_VECTOR :
        size = (length + 2) * 4;		/* +2 for addr & length words */
        break;
    case DT_STRING :
        size = 4;
        break;
    case DT_SHORT_VECTOR :
        size = length * 2;
        break;
    case DT_LONG_VECTOR :
        size = length * 4;
        break;
    default :
        printf("\nUnknown data-type %d in \"get_item_size\"!\n", data_type); 
        size = 0;
        break;
    }
return size;
}


/**********************  ALIGN STD OUTPUT ***********************
 *
 *		INPUT		pointer
 *				data_type
 *
 *		OUTPUT		aligned pointer
 *
 ****************************************************************/

DD$BYTE *fld_align(pointer, data_type)

DD$BYTE *pointer;
short data_type;
{
switch (data_type)
    {
    case DT_TINY:
    case DT_TINY_INDEX:
    case DT_ASCIZ:
    case DT_BIT_VECTOR:
				/* no allignment needed */
/*
	return pointer;
    break;
*/
    case DT_SHORT:
    case DT_SHORT_INDEX:
    case DT_SHORT_REGISTER:
    case DT_SHORT_VECTOR:
    case DT_COUNTED_SHORT_VECTOR:
		/* short allignment needed */
/*
        return (DD$BYTE *)(((long)pointer + 1) & 0xfffffffe);
    break;
*/
    case DT_LONG:
    case DT_INDEXED:
    case DT_REGISTER:
    case DT_DATE:
    case DT_BYTE_VECTOR:
    case DT_COUNTED_LONG_VECTOR:
    case DT_ADDR_CNT_VECTOR:
    case DT_LONG_VECTOR:
    case DT_VMS_TIME:
    case DT_STRING:
		/* long allignment needed */
        return (DD$BYTE *)(((long)pointer + 3) & 0xfffffffc);
    break;

    default:
		/* long allignment needed */
        return (DD$BYTE *)(((long)pointer + 3) & 0xfffffffc);
    break;
    }
}


/******************************************************************
**++
**  get_std_segment_dsd
**
**  o Description
**
**      The application fills in the pointer to the data-segemnt in the CTX
**      before the call and the function fills in the information for the
**      segment and its first element.
**
**  o Return values
**
**	DD$SUCCESS		- Function completed successfully.
**	DD$UNKNOWN_SEGMENT	- The "segment_type" / "segment_subtype"
**				  combination doesn't exist in the DSD tables.
**	DD$UNKNOWN_ITEM		- Can not find the definition for an item!
**				  Indicates that the DSD tables are corrupted!
**
**  o Remarks
**
**	A call must have been made to "dsd_init" to initialize the DSD
**	tables before using this function.
**
**--
*/

long get_std_segment_dsd (ctx)
DD$STD_DSD_CTX_PTR ctx;
{					/* Fill the Context Structure */

ctx->CTX_type = std_segment;		/* CTX for std segment */
ctx->curr_item = 1;			/* Set current seg item*/
ctx->curr_field = 1;
ctx->field_DSD_ptr = 0;
ctx->field_position = 0;
ctx->code_DSD_ptr = 0;

ctx->segment_DSD_ptr =			/* Get segment DSD pointer */
	find_std_segment_dsd(ctx->segment_ptr->type,
			     ctx->segment_ptr->subtype);
if (ctx->segment_DSD_ptr == DD$UNKNOWN_SEGMENT)
    return DD$UNKNOWN_SEGMENT;

ctx->segment_VALID_code = DD$VALID;		/* Get segment validity code */

ctx->item_DSD_ptr =
	&std_items[std_seg_items[ctx->segment_DSD_ptr->SEG_ITEM_IX].ITEM_IX];

ctx->item_ptr = (DD$BYTE *)ctx->segment_ptr +	/* data_item ptr     */
 		DD$HEADER_BYTES +
		DD$VALID_BYTES(DD$ELEMENT_COUNT);
ctx->item_ptr = fld_align(ctx->item_ptr, ctx->item_DSD_ptr->TYPE);

ctx->item_VALID_code = get_validity_code(ctx);	/* data-item validity code */

				/* Initialize the register information */

if (ctx->item_DSD_ptr->TYPE == DT_SHORT_REGISTER ||
    ctx->item_DSD_ptr->TYPE == DT_REGISTER)
    {
    if (ctx->item_DSD_ptr->COUNT != 0)
	{ 
	ctx->curr_field = 1;		/* Initialize the current field */
        ctx->field_position = 0;
        ctx->field_DSD_ptr = &std_regs[ctx->item_DSD_ptr->INDEX];
        if (ctx->field_DSD_ptr->TYPE == DC_CODED &&
	    ctx->field_DSD_ptr->COUNT != 0)
            ctx->code_DSD_ptr = &std_codes[ctx->field_DSD_ptr->CODE_IX];
	}
    }
				/* Initialize the coded  information */

if (ctx->item_DSD_ptr->TYPE == DT_TINY_INDEX ||
    ctx->item_DSD_ptr->TYPE == DT_SHORT_INDEX ||
    ctx->item_DSD_ptr->TYPE == DT_INDEXED)
    if (ctx->item_DSD_ptr->COUNT != 0)
        ctx->code_DSD_ptr = &std_codes[ctx->item_DSD_ptr->INDEX];
return DD$SUCCESS;
}


/*********************************************************************
**++
**  get_next_item_dsd
**
**  o Description
**
**	This function updates the data-item fields of the CTX with the
**	definition of the next element in the data-segment.
**
**  o Return values
**
**	DD$SUCCESS		- Function completed successfully.
**	DD$END_OF_SEGMENT	- No more elements in the data-segment.
**	DD$UNKNOWN_ITEM		- Can not find the definition for an item!
**				  Indicates that the DSD tables are corrupted!
**
**  o Synopses
**
**	#include "generic_dsd.h"
**      #include "std_dsd.h"
**
**	long get_next_item_dsd (ctx)
**	DD$STD_DSD_CTX *ctx;
**
**  o Remarks
**
**	A call must have been made to "get_std_segment_dsd" to initialize
**	CTX before using this function.
**
**  o Example
**
**--
*/

long get_next_item_dsd (ctx)
DD$STD_DSD_CTX *ctx;
{
					/* Update the data-item pointer */
					/* depending on old data-type   */

ctx->item_ptr += get_item_size (ctx->item_DSD_ptr->TYPE,
				ctx->item_DSD_ptr->COUNT);

					/* Check for end of segment */

if (ctx->curr_item >= ctx->segment_DSD_ptr->COUNT)
    return DD$END_OF_SEGMENT;
					/* Update the CTX for next item */
					/* Get the new item_DSD_ptr   */
ctx->item_DSD_ptr = 
	&std_items[std_seg_items[ctx->segment_DSD_ptr->SEG_ITEM_IX +
				 ctx->curr_item++].ITEM_IX];

ctx->item_ptr = fld_align(ctx->item_ptr, ctx->item_DSD_ptr->TYPE);

					/* Get data-item validity code */
ctx->item_VALID_code = get_validity_code(ctx);

				/* Initialize the register information */
if (ctx->item_DSD_ptr->TYPE == DT_SHORT_REGISTER ||
    ctx->item_DSD_ptr->TYPE == DT_REGISTER)
    {
    if (ctx->item_DSD_ptr->COUNT != 0)
	{ 
	ctx->curr_field = 1;		/* Initialize the current field */
        ctx->field_position = 0;
        ctx->field_DSD_ptr = &std_regs[ctx->item_DSD_ptr->INDEX];
        if (ctx->field_DSD_ptr->TYPE == DC_CODED &&
	    ctx->field_DSD_ptr->COUNT != 0)
            ctx->code_DSD_ptr = &std_codes[ctx->field_DSD_ptr->CODE_IX];
	}
    }
				/* Initialize the coded  information */

if (ctx->item_DSD_ptr->TYPE == DT_TINY_INDEX ||
    ctx->item_DSD_ptr->TYPE == DT_SHORT_INDEX ||
    ctx->item_DSD_ptr->TYPE == DT_INDEXED)
    if (ctx->item_DSD_ptr->COUNT != 0)
        ctx->code_DSD_ptr = &std_codes[ctx->item_DSD_ptr->INDEX];

return DD$SUCCESS;
}


/*********************************************************************
**++
**  get_next_field_dsd
**
**  o Description
**
**	This function updates the register field information in the CTX
**	with the definition of the next field in the register currently
**	pointed to by the CTX.
**
** o Return_values
**
**	DD$SUCCESS		- Function completed successfully.
**	DD$END_OF_REGISTER	- No more fields in the register.
**	DD$NOT_A_REGISTER	- The CTX doesn't currently point to a
**				  data-item having a REGISTER or
**				  SHORT_REGISTER data_type.
**
**  o Synopses
**
**	#include "generic_dsd.h"
**      #include "std_dsd.h"
**
**	long get_next_field_dsd (ctx)
**	DD$STD_DSD_CTX *ctx;
**
**  o Remarks
**
**  o Example
**
**--
*/

long get_next_field_dsd (ctx)
DD$STD_DSD_CTX_PTR ctx;
{
					/* Check that item is a register */
if (ctx->item_DSD_ptr->TYPE != DT_SHORT_REGISTER &&
    ctx->item_DSD_ptr->TYPE != DT_REGISTER)
    {
    printf("\n\"get_next_field_dsd\" called with non-register item!\n"); 
    return DD$NOT_A_REGISTER;
    };

if (ctx->curr_field >= ctx->item_DSD_ptr->COUNT)
    return DD$END_OF_REGISTER;
					/* Update the CTX for next field */

ctx->field_position += ctx->field_DSD_ptr->SIZE;

					/* Get the new field DSD */
ctx->field_DSD_ptr = &std_regs[ctx->item_DSD_ptr->INDEX + ctx->curr_field++];

				/* Initialize the coded  information */

if (ctx->field_DSD_ptr->TYPE == DC_CODED)
    ctx->code_DSD_ptr = &std_codes[ctx->field_DSD_ptr->CODE_IX];
return DD$SUCCESS;
}


/*********************************************************************
**++
**  decode_std_item
**  
**  o Description
**  
**      Returns a pointer to the text string corresponding to the
**	standard code for the data-item currently pointed at by the
**	context block.
**
**	Returns DD$UNKNOWN_CODE if the code isn't valid for the item.
**  
**  o Synopses
**  
**      #include "generic_dsd.h"
**      #include "std_dsd.h"
**  
**      char *decode_std_item (ctx, item_code)
**      DD$STD_DSD_CTX *ctx;
**      short item_code;
**  
**  o Remarks
**  
**  o Example
**
**--
*/


char *decode_std_item (ctx, item_code)
DD$STD_DSD_CTX_PTR ctx;
long  item_code;
{
long i;

for (i=0; i < ctx->item_DSD_ptr->COUNT; i++)
    {
    if (ctx->code_DSD_ptr[i].CODE == item_code)
	return dsd_get_label(ctx->code_DSD_ptr[i].LABEL_IX);
    }
return DD$UNKNOWN_CODE;
}


/*********************************************************************
**++
**  decode_register_field
**  
**  o Description
**  
**      Returns a pointer to the text string corresponding to the
**	code for the register field currently pointed at by the
**	context block.
**
**	Returns DD$NO_TRANSLATION if the code isn't to be translated.
**  
**  o Synopses
**  
**      #include "generic_dsd.h"
**      #include "std_dsd.h"
**  
**      char *decode_register_field (ctx, field_code)
**      DD$STD_DSD_CTX *ctx;
**      long field_code;
**  
**  o Remarks
**  
**  o Example
**
**--
*/


char *decode_register_field (ctx, field_code)
DD$STD_DSD_CTX *ctx;
long field_code;
{

long i;

for (i=0; i < ctx->field_DSD_ptr->COUNT; i++)
    {
    if (ctx->code_DSD_ptr[i].CODE == field_code)
	return dsd_get_label(ctx->code_DSD_ptr[i].LABEL_IX);
    };
return DD$NO_TRANSLATION;
}


/*********************************************************************
**++
**  get_reg_fld_code
**  
**  o Description
**  
**      Returns a pointer to the text string corresponding to the
**	input code for the requested register field of teh requested
**      item.
**
**	Returns DD$NO_TRANSLATION if the code isn't to be translated.
**  
**  o Synopses
**  
**      #include "generic_dsd.h"
**      #include "std_dsd.h"
**  
**      char *get_reg_fld_code (item_id, fld_ix, fld_code)
**      long item_id;
**      long fld_ix
**      long fld_code;
**  
**  o Remarks
**  
**  o Example
**
**--
*/


char *get_reg_fld_code(item_id, fld_ix, fld_code)
long item_id;
long fld_ix;
long fld_code;
{

DD$STD_CODES_DSD_PTR    code_dsd;
DD$STD_REGS_DSD_PTR     reg_dsd;
DD$STD_ITEMS_DSD_PTR    item_dsd;

long i;

if ((item_dsd = find_std_item_dsd(item_id)) == DD$UNKNOWN_ITEM)
    return DD$NO_TRANSLATION;

if (item_dsd->COUNT < fld_ix)
    return DD$NO_TRANSLATION;

reg_dsd = get_reg_dsd(item_dsd->INDEX + fld_ix);

if (reg_dsd->TYPE != DC_CODED)
    return DD$NO_TRANSLATION;

for (i = 0; i < reg_dsd->COUNT; i++)
    {
    code_dsd = get_code_dsd(reg_dsd->CODE_IX + i);
    if (code_dsd->CODE == fld_code)
	return dsd_get_label(code_dsd->LABEL_IX);
    };
return DD$NO_TRANSLATION;
}


/*
**++
**  get_code_std_item
**  
**  o Description
**  
**      Returns the code that corresponds to the text string passed
**	as an argument. This function is called when the text
**	string of a coded field is know and the acutual code value
**	is desired.
**
**	Returns DD$UNKNOWN_ITEM if the data-item id is not found in
**	the DSD table.
**  
**	Returns DD$UNKNOWN_CODE if the code isn't valid for the item.
**
**  o Synopses
**  
**      #include "generic_dsd.h"
**      #include "std_dsd.h"
**  
**      short get_code_std_item (item_id, code_str)
**      short item_id;
**      char *code_str;
**  
**  o Remarks
**  
**	This call does not use the data-item currently pointed to by
**	the context block.  The item-id must be supplied.
**
**  o Example
**
**--
*/

long  get_code_std_item (item_id, code_str)
short item_id;
char *code_str;
{
long	i;
long	j;
long	match;
long	length_1;
char	string_1[MAX_STRING];
char	*string_2;

DD$STD_ITEMS_DSD_PTR item_dsd_ptr;
DD$STD_CODES_DSD_PTR code_dsd_ptr;


item_dsd_ptr = find_std_item_dsd(item_id);
if (item_dsd_ptr == DD$UNKNOWN_ITEM)
    return(DD$UNKNOWN_ITEM);

length_1 = strlen(code_str);
for (j=0; j <= length_1; j++)
    string_1[j] = tolower(code_str[j]);

for (i=0; i < item_dsd_ptr->COUNT; i++)
    {
    if ((code_dsd_ptr =
		find_std_code_dsd(item_dsd_ptr, i)) ==
		    DD$UNKNOWN_CODE)
        return(DD$UNKNOWN_ITEM);
    string_2 = dsd_get_label(code_dsd_ptr->LABEL_IX);
    if (length_1 != strlen(string_2))
	continue;				/* Try the next code */

    match = TRUE;				/* Assume match */
    for (j=0; j < length_1; j++)
	{
	if (string_1[j] != tolower(string_2[j]))
	    {
	    match = FALSE;
	    break;				/* Stop on mismatch */
	    }
	}

    if (match == TRUE)
	return (code_dsd_ptr->CODE);
    }
return DD$UNKNOWN_CODE;
}

/*********************************************************************
**++
**  decode_os_item
**  
**  o Description
**  
**      Returns the standard code corresponding to an O/S code for the
**	os_data_item id passed to the functionid passed to the function
**
**  
**  o Synopses
**  
**      #include "generic_dsd.h"
**  
**      short decode_os_item (item_id, item_code)
**      long item_id;
**      long item_code;
**  
**  o Remarks
**  
**  o Example
**
**--
*/

long  decode_os_item (item_id, item_code)
long item_id;
long item_code;
{
long i;
DD$OS_ITEMS_DSD_PTR  os_item_ptr;

if ((os_item_ptr = find_os_item_dsd(item_id)) ==DD$UNKNOWN_ITEM)
    return DD$UNKNOWN_CODE;

for (i=0; i < os_item_ptr->COUNT; i++)
    {
    if (os_codes[os_item_ptr->CODE_IX + i].OS_CODE == item_code)
        return os_codes[os_item_ptr->CODE_IX + i].STD_CODE;
    };
return DD$UNKNOWN_CODE;
}


/*********************************************************************
**++
**  dsd_get_label
**  
**  o Description
**  
**      Returns a pointer to the label string for the  LABEL_IX
**
**  
**  o Synopses
**  
**  
**      char *dsd_get_label (label_index)
**      short label_index;
**  
**  o Remarks
**  
**  o Example
**
**--
*/

char *dsd_get_label (label_index)
short label_index;
{

return &strings_area[labels[label_index].STRINGS_IX];
}


/*
**++
**  find_std_segment_dsd
**
**  o Description
**
**      Searches the STD-SEGMENT-LIST for the requested segment DSD.
**
**	Returns a pointer to the segment DSD, or DD$UNKNOWN_SEGMENT if
**	the segment_type and segment_subtype are not in the DSD tables.
**
**  o Synopses
**
**      #include "generic_dsd.h"
**      #include "std_dsd.h"
**
**	DD$STD_SEGMENT_DSD_PTR
**      find_std_segment_dsd (segment_type, segment_subtype)
**	DD$STD_HEADER.type segment_type;
**	DD$STD_HEADER.subtype segment_subtype;
**
**  o Remarks
**
**
**  o Example
**
**--
*/

DD$STD_SEGS_DSD_PTR find_std_segment_dsd(seg_type, seg_subtype)
short seg_type;
short seg_subtype;
{
long i;

for (i=0; i < std_segs_count; i++)
    {
    if (std_segs[i].TYPE    == seg_type &&
	std_segs[i].SUBTYPE == seg_subtype)
	return &std_segs[i];
    };
return DD$UNKNOWN_SEGMENT;
}

/*
**++
**  get_item_offset
**
**  o Description
**
**      Searches the STD-SEG_ITEMS for the requested item id.
**
**	Returns a pointer to the seg_items_dsd, or DD$UNKNOWN_ITEM if
**	the item_id is not in the DSD tables.
**
**  o Synopses
**
**      #include "generic_dsd.h"
**      #include "std_dsd.h"
**
**      short get_item_offset (segment_ptr, item_id)
**	DD$STD_SEGMENT_DSD_PTR segment_ptr;
**	short item_id;
**
**  o Remarks
**
**
**  o Example
**
**--
*/

short get_item_offset (segment_ptr, item_id)
DD$STD_SEGS_DSD_PTR segment_ptr;
short item_id;
{
long i;

for (i=0; i < segment_ptr->COUNT; i++)
    {
    if (std_items[std_seg_items
		  [segment_ptr->SEG_ITEM_IX + i
		  ].ITEM_IX
		 ].ID == item_id)
	return std_seg_items[segment_ptr->SEG_ITEM_IX + i ].ITEM_OFFSET;
    };
return DD$UNKNOWN_ITEM;
}

/*
**++
**  get_seg_item_dsd
**
**  o Description
**
**      Offsets into the STD-SEG_ITEMS for the given index
**
**	Returns a pointer to the seg_items_dsd
**
**  o Synopses
**
**      #include "generic_dsd.h"
**      #include "std_dsd.h"
**
**	DD$STD_SEG_ITEMS_DSD_PTR get_seg_item_dsd (index)
**	long  index;
**
**  o Remarks
**
**
**  o Example
**
**--
*/

DD$STD_SEG_ITEMS_DSD_PTR get_seg_item_dsd (index)
long  index;
{
return &std_seg_items[index];
}

/*
**++
**  get_code_dsd
**
**  o Description
**
**      Offsets into the STD-CODES for the given index
**
**	Returns a pointer to the codes_dsd
**
**  o Synopses
**
**      #include "generic_dsd.h"
**      #include "std_dsd.h"
**
**	DD$STD_CODES_DSD_PTR get_code_dsd(index)
**	long  index;
**
**  o Remarks
**
**
**  o Example
**
**--
*/

DD$STD_CODES_DSD_PTR get_code_dsd(index)
long  index;
{
return &std_codes[index];
}

/*
**++
**  get_reg_dsd
**
**  o Description
**
**      Offsets into the STD-REGS for the given index
**
**	Returns a pointer to the regs_dsd
**
**  o Synopses
**
**      #include "generic_dsd.h"
**      #include "std_dsd.h"
**
**	DD$STD_REGS_DSD_PTR get_reg_dsd (index)
**	long  index;
**
**  o Remarks
**
**
**  o Example
**
**--
*/

DD$STD_REGS_DSD_PTR get_reg_dsd (index)
long  index;
{
return &std_regs[index];
}


/*
**++
**  get_item_dsd
**
**  o Description
**
**      offsets into the STD-ITEMS for the given index
**
**	Returns a pointer to the seg_items_dsd
**
**  o Synopses
**
**      #include "generic_dsd.h"
**      #include "std_dsd.h"
**
**      DD$STD_ITEMS_DSD_PTR get_item_dsd(index)
**	long index;
**
**  o Remarks
**
**
**  o Example
**
**--
*/

DD$STD_ITEMS_DSD_PTR get_item_dsd(index)
long index;
{
return &std_items[index];
}


/*********************************************************************
**++
**  find_std_item_dsd
**
**  o Description
**
**      Searches the STD-ITEM-LIST for the requested data-item id.
**
**	Returns a pointer to the data-item DSD, or DD$UNKNOWN_ITEM if
**	the data-item id is not found in the DSD table.
**
**  o Synopses
**
**      #include "generic_dsd.h"
**      #include "std_dsd.h"
**
**	DD$STD_ITEM_DSD_PTR
**      find_std_item_dsd (item_id)
**	short item_id;
**
**  o Remarks
**
**	Should only be used when information about a data-item is required
**	out of the normal data-segment context.
**
**  o Example
**
**--
*/

DD$STD_ITEMS_DSD_PTR find_std_item_dsd (item_id)
short item_id;
{
long i;

for (i=0; i < std_items_count; i++)
    {
    if (std_items[i].ID == item_id)
	return &std_items[i];
    };
return DD$UNKNOWN_ITEM;
}


/*********************************************************************
**++
**  find_std_code_dsd
**  
**  o Description
**  
**      Returns the corresponding STD_CODES_DSD structure for the
**      specified input ( item_dsd_ptr and code index)
**
**  
**  o Synopses
**  
**      #include "generic_dsd.h"
**  
**      DD$STD_CODES_DSD_PTR find_std_code_dsd (item_dsd_ptr, code_index)
**      short item_dsd_ptr;
**      short code_index;
**  
**  o Remarks
**  
**  o Example
**
**--
*/

DD$STD_CODES_DSD_PTR find_std_code_dsd (item_dsd_ptr, code_index)
DD$STD_ITEMS_DSD_PTR item_dsd_ptr;
short code_index;
{

if (item_dsd_ptr->TYPE == DT_TINY_INDEX ||
    item_dsd_ptr->TYPE == DT_SHORT_INDEX ||
    item_dsd_ptr->TYPE == DT_INDEXED)
    {
    if (item_dsd_ptr->COUNT < code_index)
	return DD$UNKNOWN_CODE;
    return &std_codes[item_dsd_ptr->INDEX + code_index];
    };
return DD$UNKNOWN_CODE;
}


/*********************************************************************
**++
**  find_reg_code_dsd
**  
**  o Description
**  
**      Returns the corresponding STD_CODES_DSD structure for the
**      specified input ( reg_dsd_ptr and code index)
**
**  
**  o Synopses
**  
**      #include "generic_dsd.h"
**  
**      DD$STD_CODES_DSD_PTR find_reg_code_dsd (reg_dsd_ptr, code_index)
**      short reg_dsd_ptr;
**      short code_index;
**  
**  o Remarks
**  
**  o Example
**
**--
*/

DD$STD_CODES_DSD_PTR find_reg_code_dsd (reg_dsd_ptr, code_index)
DD$STD_REGS_DSD_PTR reg_dsd_ptr;
short code_index;
{

if (reg_dsd_ptr->TYPE == DC_CODED)
    {
    if (reg_dsd_ptr->COUNT < code_index)
	return DD$UNKNOWN_CODE;
    return &std_codes[reg_dsd_ptr->CODE_IX + code_index];
    };
return DD$UNKNOWN_CODE;
}


/*********************************************************************
**++
**  find_os_item_dsd
**
**  o Description
**
**      Searches the OS-ITEM-LIST for the requested data-item id.
**
**	Returns a pointer to the data-item DSD, or DD$UNKNOWN_ITEM if
**	the data-item id is not found in the DSD table.
**
**  o Synopses
**
**      #include "generic_dsd.h"
**
**	DD$OS_ITEM_DSD_PTR
**      find_os_item_dsd (item_id)
**	DD$OS_ITEM_DSD.ID item_id;
**
**  o Remarks
**
**  o Example
**
**--
**
**/

DD$OS_ITEMS_DSD_PTR find_os_item_dsd (item_id)
short item_id;
{
long i;

for (i=0; i <= os_items_count; i++)
    {
    if (os_items[i].ID == item_id)
        return &os_items[i];
    }
return DD$UNKNOWN_ITEM;
}


/*********************************************************************
**++
**  find_label_dsd  
**
**  o Description
**
**      returns the indicated DSP_LABEL_PRT as requested by the LABEL_INDEX
**
**
**  o Synopses
**
**      #include "generic_dsd.h"
**
**	DD$DSP_LABELS_PTR find_label_dsd(label_index)
**	short label_index;
**
**  o Remarks
**
**  o Example
**
**--
**
**/

DD$DSP_LABELS_PTR find_label_dsd(label_index)
short label_index;
{
return &labels[label_index];
}


/*********************************************************************
**++
**  Set Validity codes
**  
**  o Description
**  
**      This function is only available to ERIT.  It sets the item validity
**      code in the standard data-segment for the current item.
**  
**  o Synopses
**  
**      #include "generic_dsd.h"
**  
**      int set_validity_code (seg_ptr, item_seq, item_validity_code)
**      DD$STD_HEADER_PTR seg_ptr;
**      short item_seq;
**      short item_validity_code;
**  
**  o Remarks
**  
**      The "item_validity_code" must be one of the following:
**  
**  	DD$VALID	The data is valid and available.
**  
**  	DD$N_A		The data does not exist in this context.
**  
**  	DD$N_V		The data is invalid, and the bad value is in
**  			the item. (i.e. The data does not meet the
**  			RANGE or LIST validity checks in the item's
**  			DSD)
**  
**  	DD$N_V$N_A	The data is invalid and not available. (i.e.
**  			The data does not meet the RANGE or LIST
**  			validity checks in the item's DSD, and the
**  			bad value could not be stored in the item.)
**  			(e.g. ASCII number ==> short  -- and the string
**  			recieved contains non-numeric characters.)
**  			
**  o Example
**
**--
*/

static unsigned int LBIT[4] = { 1,4,16,64 };
static unsigned int HBIT[4] = { 2,8,32,128};
static unsigned int ORMASK[4] = {0,1,2,3};
static unsigned int LEFTSHIFT[4] = {0,2,4,6};

/************************** SETNURDLE () *****************************/

setnurdle(nur,nn,nv)
unsigned char nur;
short nn;
short nv;
{
					/* first turn off both bits */
nur &= ~(LBIT[nn-1] | HBIT[nn-1]);
nur = nur | (ORMASK[nv] << LEFTSHIFT[nn-1]);
return(nur);
}

/************************** GETBITS ()  ******************************/

getbits(x,p,n)
unsigned x,p,n;
{

return((x >> (p+1-n)) & ~(~0 << n));
}

/***********************  SET_VALIDITY_CODE () ***********************/

int set_validity_code (seg_ptr, item_seq, item_validity_code)
DD$STD_HEADER_PTR seg_ptr;
short item_seq;
short item_validity_code;
{

short wbyte,wnurdle;
unsigned char thebyte,newbyte;

wbyte = (item_seq / 4) ;
wnurdle = (item_seq % 4) + 1;
thebyte = seg_ptr->VALID_byte[wbyte];
newbyte = setnurdle(thebyte,wnurdle,item_validity_code);    
seg_ptr->VALID_byte[wbyte] = newbyte;
}

/************************  GET_VALIDITY_CODE ()  *********************/

int
get_validity_code (ctx)
DD$STD_DSD_CTX *ctx;
{

short wbyte,wnurdle;
unsigned char thebyte;

wbyte = (ctx->curr_item / 4) ;
wnurdle = (ctx->curr_item % 4) + 1;
thebyte = ctx->segment_ptr->VALID_byte[wbyte];
return(getbits(thebyte,(wnurdle*2)-1,2));
}

