#ifndef lint
static char sccsid[]  =  "@(#)summary.c	4.2   (ULTRIX)   9/11/90";
#endif  lint
/*
*	.TITLE	SUMMARY - Program used show errlog summaries.
*	.IDENT	/1-001/
*
* COPYRIGHT (C) 1985 DIGITAL EQUIPMENT CORP.,
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
* ABSTRACT:
*	This program is used to print a summary of the errorlog
*	
* ENVIRONMENT:	ULTRIX-32 C
*	
* AUTHOR:  Luis Arce,  CREATION DATE:  26-APR-88
*
* MODIFIED BY:
*
*--
*/

#include "ueliterals.h"
#include "uestruct.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include "generic_dsd.h"
#include "std_dsd.h"

/*
*++
*=
*=
*=MODULE FLOW - summary.c
*=
*=  a - sum_save(eis,sis)                           Save summary information.
*=          get_std_segment_dsd(ctx) "eis"                  (dsd_access.c)
*=          do
*=              {
*=              "save selected fields"
*=              if(CODED type)
*=                  decode_std_item(ctx,item_ptr)           (dsd_access.c)
*=              if(REG type)
*=                  decode_register_field(ctx,item_ptr)     (dsd_access.c)
*=              }
*=          while(get_next_item_dsd(ctx))                   (dsd_access.c)
*=          find_node(temp_node)                            (* b)
*=          get_std_segment_dsd(ctx) "sis"                  (dsd_access.c)
*=          find_type(type_ptr,line)                        (* c)
*=          do
*=              {
*=              dsd_get_label(LABEL_IX)                     (dsd_access.c)
*=              trans_item(ctx,i)                           (* h)
*=              }
*=          while(get_next_item_dsd(ctx))                   (dsd_access.c)
*=          find_item(item_ptr,line)                        (* c)
*=          return()
*=
*=  b - find_node(node_ptr)                         Checks for duplicate node
*=                                                  and incr cnt or allocates.
*=          strncmp()
*=              return(curr_node)
*=          alloc_node(temp_node)                           (* e)
*=          return(new_node)
*=
*=  c - find_type(type_ptr_ptr,line_ptr)            Checks for duplicate type
*=                                                  and incr cnt or allocates.
*=          strcmp()
*=              return(curr_type)
*=          alloc_type(line_ptr)                            (* f)
*=          return(new_type)
*=
*=  d - find_item(item_ptr_ptr,line_ptr)            Checks for duplicate item
*=                                                  and incr cnt or allocates.
*=          strcmp()
*=              return(curr_item)
*=          alloc_item(line_ptr)                            (* g)
*=          return(new_item)
*=
*=  e - alloc_node(temp_node)                       Allocates and inits node.
*=          malloc(new_node)
*=          "inits new_node"
*=          return(new_node)
*=
*=  f - alloc_type(line_ptr)                        Allocates and inits type.
*=          malloc(new_type)
*=          "inits new_type"
*=          return(new_type)
*=
*=  g - alloc_item(line_ptr)                        Allocates and inits item.
*=          malloc(new_item)
*=          "inits new_item"
*=          return(new_item)
*=
*=  h - trans_item(ctx,len)                         Translates item value to
*=                                                  string.
*=          find_label_dsd(LABEL_IX)                        (dsd_access.c)
*=          if(CODED type)
*=              decode_std_item(ctx,item_ptr)               (dsd_access.c)
*=          else
*=              sprintf(string,...., val)
*=          return(string)
*=
*=  i - sum_print(out_form)                         Prints the summary report.
*=          while(nodes)
*=              printf(node info)
*=              while(types)
*=                  printf(type info)
*=                  while(items)
*=                      printf(type info)
*=          if(UE$OUT_TERSE)
*=              {
*=              free(item_ptr)
*=              free(type_ptr)
*=              free(node_ptr)
*=              }
*=          return()
*=



/*********************  STRUCTURE DEFINITIONS  ******************/

typedef struct item_struc
    {
    struct item_struc *next_ptr;
    long   count;
    char   values[80];
    }ITEM_SUM, *ITEM_SUM_PTR;

typedef struct type_struc
    {
    struct type_struc *next_ptr;
    struct item_struc *item_ptr;
    long   count;
    char   err_type[40];
    char   labels[80];
    }TYPE_SUM, *TYPE_SUM_PTR;

typedef struct node_struc
    {
    struct node_struc *next_ptr;
    struct type_struc *type_ptr;
    long   count;
    unsigned long   sys_id;
    char   *sys_type;
    char   node[12];
    }NODE_SUM, *NODE_SUM_PTR;

/************************* COMMON DATA  *************************/

extern  struct  in_struc	in_st;
NODE_SUM_PTR    first_node_ptr;

NODE_SUM_PTR            alloc_node();
NODE_SUM_PTR            find_node();
TYPE_SUM_PTR            alloc_type();
TYPE_SUM_PTR            find_type();
ITEM_SUM_PTR            alloc_item();
ITEM_SUM_PTR            find_item();
char                    *trans_item();

char                    *malloc();
long                    get_std_segment_dsd();
long	                get_next_item_dsd();
char	                *dsd_get_label();
char                    *decode_std_item();
char                    *decode_register_field();
char                    *get_reg_fld_code();
DD$DSP_LABELS_PTR       find_label_dsd();

/**********************************************************************/
/*******************************  SUM_SAVE  ***************************/
/**********************************************************************/

long sum_save (ueis, usis)

DD$STD_HEADER *ueis;
DD$STD_HEADER *usis;
{

short           i;
char            tchar[80];
char            line[80];
char            label[80];
char            *line_ptr;
char            *str_ptr;

ITEM_SUM        temp_item;
NODE_SUM        temp_node;

ITEM_SUM_PTR    curr_item_ptr;
TYPE_SUM_PTR    curr_type_ptr;
NODE_SUM_PTR    curr_node_ptr;

DD$STD_DSD_CTX  my_ctx;

/********************* GET ITEMS FROM THE EIS *************************/

my_ctx.segment_ptr = ueis;
get_std_segment_dsd(&my_ctx);
do  {
    switch (my_ctx.item_DSD_ptr->ID)
	{
	case DD$sysid:
	    if (*(long *) my_ctx.item_ptr != 0)
	        temp_node.sys_id = *(long *) my_ctx.item_ptr;
        break;
	case DD$sysid2:
	    if (*(long *) my_ctx.item_ptr != 0)
	        temp_node.sys_id = *(long *) my_ctx.item_ptr;
        break;
	case DD$pmax_prid:
	    if (*(long *) my_ctx.item_ptr != 0)
	        temp_node.sys_id = *(long *) my_ctx.item_ptr;
	break;
	case DD$hostname:
	    strncpy(temp_node.node, *((long *) my_ctx.item_ptr), 12);
	break;
	case DD$sysktype:
	    temp_node.sys_type = decode_std_item(
                                        &my_ctx,
                                        *(long *)my_ctx.item_ptr);
	break;
	case DD$eventtype:
            sprintf(line,"%3d - ", *(long *)my_ctx.item_ptr);
            if ((str_ptr = decode_register_field(
                                        &my_ctx,
                                        *(long *)my_ctx.item_ptr)) !=
                        DD$NO_TRANSLATION)
                {
                strcat(line, str_ptr);
                }
	break;
	}
    }
while (get_next_item_dsd(&my_ctx) != DD$END_OF_SEGMENT);

curr_node_ptr = find_node(&temp_node);
curr_type_ptr = find_type(&curr_node_ptr->type_ptr, line);

my_ctx.segment_ptr = usis;
if (get_std_segment_dsd(&my_ctx) == DD$SUCCESS)
    {
    line[0]  = '\0';
    label[0] = '\0';

    do  {
        line_ptr = dsd_get_label(my_ctx.item_DSD_ptr->LABEL_IX);
        i = strlen(line_ptr);
        if (i <= 1)
            continue;
        strcat (label, line_ptr);
        strcat (label, "  ");
        strcat (line,  trans_item(&my_ctx, i));
        strcat (line, "  ");
        }
    while (get_next_item_dsd(&my_ctx) != DD$END_OF_SEGMENT);

    strcpy(curr_type_ptr->labels, label); 
    curr_item_ptr = find_item(&curr_type_ptr->item_ptr, line);
    }
}


/**********************************************************************
 ****************************** FIND_NODE *****************************
 **********************************************************************/

NODE_SUM_PTR find_node (temp_node_ptr)

NODE_SUM_PTR temp_node_ptr;

{
NODE_SUM_PTR prev_node_ptr;
NODE_SUM_PTR curr_node_ptr;
NODE_SUM_PTR new_node_ptr;

prev_node_ptr = first_node_ptr;
curr_node_ptr = first_node_ptr;

while (curr_node_ptr != NULL)
    {
    if (strncmp(temp_node_ptr->node, curr_node_ptr->node, 12) > 0)
        {
        prev_node_ptr = curr_node_ptr;
        curr_node_ptr = curr_node_ptr->next_ptr;
        continue;
        }

    if (strncmp(temp_node_ptr->node, curr_node_ptr->node, 12) < 0)
        break;

/*************************** EQUAL NODE ******************************/

    while (curr_node_ptr != NULL)
        {
        if (temp_node_ptr->sys_id > curr_node_ptr->sys_id)
            {
            prev_node_ptr = curr_node_ptr;
            curr_node_ptr = curr_node_ptr->next_ptr;
            continue;
            }
        if (temp_node_ptr->sys_id < curr_node_ptr->sys_id)
            break;

/*************************** EQUAL SYS_ID ****************************/

        while (curr_node_ptr != NULL)
            {
            if (strcmp(temp_node_ptr->sys_type, curr_node_ptr->sys_type) > 0)
                {
                prev_node_ptr = curr_node_ptr;
                curr_node_ptr = curr_node_ptr->next_ptr;
                continue;
                }
            if (strcmp(temp_node_ptr->sys_type, curr_node_ptr->sys_type) < 0)
                break;
            curr_node_ptr->count++;
            return (curr_node_ptr);
            }
        break;
        }
    break;
    }    
new_node_ptr = alloc_node(temp_node_ptr);

if (first_node_ptr == NULL)
    {
    first_node_ptr = new_node_ptr;
    }

else
if (curr_node_ptr == NULL)
    {
    prev_node_ptr->next_ptr = new_node_ptr;
    }

else
if (prev_node_ptr == curr_node_ptr)
    {
    new_node_ptr->next_ptr = first_node_ptr;
    first_node_ptr = new_node_ptr;
    }

else
    {
    new_node_ptr->next_ptr  = curr_node_ptr;
    prev_node_ptr->next_ptr = new_node_ptr;
    }
return (new_node_ptr);

}

/**********************************************************************
 ***************************** FIND_TYPE ******************************
 **********************************************************************/

TYPE_SUM_PTR find_type(type_ptr_ptr, line_ptr)

TYPE_SUM_PTR *type_ptr_ptr;
char         *line_ptr;

{
TYPE_SUM_PTR prev_type_ptr;
TYPE_SUM_PTR curr_type_ptr;
TYPE_SUM_PTR new_type_ptr;

prev_type_ptr = *type_ptr_ptr;
curr_type_ptr = *type_ptr_ptr;

while (curr_type_ptr != NULL)
    {
    if (strcmp (line_ptr, curr_type_ptr->err_type) > 0)
        {
        prev_type_ptr = curr_type_ptr;
        curr_type_ptr = curr_type_ptr->next_ptr;
        continue;
        }

    if (strcmp (line_ptr, curr_type_ptr->err_type) < 0)
        break;

    curr_type_ptr->count++;
    return (curr_type_ptr);
    }

new_type_ptr = alloc_type(line_ptr);

if (*type_ptr_ptr == NULL)
    *type_ptr_ptr = new_type_ptr;

else
if (curr_type_ptr == NULL)
    prev_type_ptr->next_ptr = new_type_ptr;

else
if (prev_type_ptr == curr_type_ptr)
    {
    new_type_ptr->next_ptr = *type_ptr_ptr;
    *type_ptr_ptr = new_type_ptr;
    }

else
    {
    new_type_ptr->next_ptr  = curr_type_ptr;
    prev_type_ptr->next_ptr = new_type_ptr;
    }
return (new_type_ptr);
}


/**********************************************************************
 ***************************** FIND_ITEM ******************************
 **********************************************************************/

ITEM_SUM_PTR find_item(item_ptr_ptr, line_ptr)

ITEM_SUM_PTR *item_ptr_ptr;
char         *line_ptr;

{
ITEM_SUM_PTR prev_item_ptr;
ITEM_SUM_PTR curr_item_ptr;
ITEM_SUM_PTR new_item_ptr;
char         *val_ptr;

prev_item_ptr = *item_ptr_ptr;
curr_item_ptr = *item_ptr_ptr;

while (curr_item_ptr != NULL)
    {
    val_ptr = curr_item_ptr->values;
    if (strcmp (line_ptr, val_ptr) > 0)
        {
        prev_item_ptr = curr_item_ptr;
        curr_item_ptr = curr_item_ptr->next_ptr;
        continue;
        }

    if (strcmp (line_ptr, val_ptr) < 0)
        break;

    curr_item_ptr->count++;
    return (curr_item_ptr);
    }

new_item_ptr = alloc_item(line_ptr);

if (*item_ptr_ptr == NULL)
    *item_ptr_ptr = new_item_ptr;

else
if (curr_item_ptr == NULL)
    prev_item_ptr->next_ptr = new_item_ptr;

else
if (prev_item_ptr == curr_item_ptr)
    {
    new_item_ptr->next_ptr = *item_ptr_ptr;
    *item_ptr_ptr = new_item_ptr;
    }

else
    {
    new_item_ptr->next_ptr  = curr_item_ptr;
    prev_item_ptr->next_ptr = new_item_ptr;
    }
return (new_item_ptr);
}



/**********************************************************************
 **************************** ALLOCATE_NODE ***************************
 **********************************************************************/

NODE_SUM_PTR alloc_node (temp_node_ptr)

NODE_SUM_PTR temp_node_ptr;
{
NODE_SUM_PTR new_node_ptr;

new_node_ptr = (struct node_struc *)malloc(sizeof(NODE_SUM));

new_node_ptr->next_ptr    = NULL;
new_node_ptr->type_ptr    = NULL;
new_node_ptr->count       = 1;
new_node_ptr->sys_id      = temp_node_ptr->sys_id;
new_node_ptr->sys_type    = temp_node_ptr->sys_type;
strncpy(new_node_ptr->node, temp_node_ptr->node, 12);
return (new_node_ptr);
}

/**********************************************************************
 ************************** ALLOCATE_TYPE *****************************
 **********************************************************************/

TYPE_SUM_PTR alloc_type(line_ptr)

char         *line_ptr;
{
TYPE_SUM_PTR new_type_ptr;

new_type_ptr = (struct type_struc *) malloc(sizeof(TYPE_SUM));

new_type_ptr->next_ptr    = NULL;
new_type_ptr->item_ptr    = NULL;
new_type_ptr->count       = 1;
strcpy (new_type_ptr->err_type, line_ptr);
return (new_type_ptr);
}

/**********************************************************************
 ************************** ALLOCATE_ITEM *****************************
 **********************************************************************/

ITEM_SUM_PTR alloc_item(line_ptr)

char    *line_ptr;
{
ITEM_SUM_PTR new_item_ptr;

new_item_ptr = (struct item_struc *) malloc(sizeof(ITEM_SUM));

new_item_ptr->next_ptr    = NULL;
new_item_ptr->count       = 1;
strcpy (new_item_ptr->values, line_ptr);
return (new_item_ptr);
}



/**********************************************************************
 **************************** TRANS_ITEM ******************************
 **********************************************************************/

char *trans_item(ctx_ptr, len)

DD$STD_DSD_CTX_PTR ctx_ptr;
short   len;

{
DD$DSP_LABELS_PTR  label_dsd_ptr;

#define str_max 20

char    *str_ptr;
char    str_val[str_max];
unsigned long    value;
short   i;

strncpy(str_val, "                   ", len);
str_val[len] = '\0';

if ((ctx_ptr->item_VALID_code == DD$N_V$N_A) ||
    (ctx_ptr->item_VALID_code == DD$N_A))
    return(str_val);

label_dsd_ptr = find_label_dsd(ctx_ptr->item_DSD_ptr->LABEL_IX);

switch (ctx_ptr->item_DSD_ptr->TYPE)
    {
    case (DT_SHORT):
    case (DT_SHORT_INDEX):
        value = *(short *)ctx_ptr->item_ptr;
    break;
    
    case (DT_LONG):
    case (DT_INDEXED):
        value = *(long *)ctx_ptr->item_ptr;
    break;
    }

if (value < 0)
    return(str_val);

switch (ctx_ptr->item_DSD_ptr->TYPE)
    {
    case (DT_SHORT_INDEX):
    case (DT_INDEXED):
        if ((int)(str_ptr = decode_std_item(ctx_ptr, value))
                                        != DD$UNKNOWN_CODE)      
            {
            i = strlen(str_ptr);
            i = len - i;
            strcpy(str_val+i, str_ptr);
            str_val[len] = '\0';
            } 
    break;

    case (DT_SHORT):
    case (DT_LONG):
        switch(label_dsd_ptr->TYPE)
            {
            case (DF_DEFAULT):
            case (DF_DECIMAL):
                sprintf(str_val, "%*d", len, value);
            break;

            case (DF_HEX):
                sprintf(str_val, "%*x", len, value);
            break;

            case (DF_OCTAL):
                sprintf(str_val, "%*o", len, value);
            break;
            }
    break;
    }

str_val[len] = '\0';

return (str_val);
}




/**********************************************************************/
/***************************  OUTPUT REPORT ***************************/
/**********************************************************************/

long sum_print(out_form)
short   out_form;
{
char                    *label_ptr;

ITEM_SUM_PTR            curr_item_ptr;
ITEM_SUM_PTR            next_item_ptr;
TYPE_SUM_PTR            curr_type_ptr;
NODE_SUM_PTR            curr_node_ptr;

DD$STD_ITEMS_DSD_PTR    item_dsd_ptr;
DD$STD_REGS_DSD_PTR     reg_dsd_ptr;
DD$STD_CODES_DSD_PTR    codes_dsd_ptr;

curr_node_ptr = first_node_ptr;

while (curr_node_ptr != NULL)
    {
    if (out_form != UE$OUT_TERSE)
        {
        printf ("\n\n");
        printf ("***************************************");
        printf ("***************************************");
        printf ("\n\n%4d RECORDS SUMMARIZED FOR \"%s\", %s, SYSID = x%8.8X\n",
                                curr_node_ptr->count,
                                curr_node_ptr->node,
                                curr_node_ptr->sys_type,
                                curr_node_ptr->sys_id);
        }
    curr_type_ptr = curr_node_ptr->type_ptr;

    while (curr_type_ptr != NULL)
        {
        if (out_form != UE$OUT_TERSE)
            {
            printf ("\n%4d  %s\n", curr_type_ptr->count, 
                                   curr_type_ptr->err_type);
            printf ("      %s\n",  curr_type_ptr->labels);
            }
        curr_item_ptr = curr_type_ptr->item_ptr;

        while (curr_item_ptr != NULL)
            {
            if (out_form != UE$OUT_TERSE)
                printf ("%4d  ", curr_item_ptr->count);
            else
                printf ("%4.4s  ", curr_type_ptr->err_type);
            printf ("%s\n", curr_item_ptr->values);
            curr_item_ptr = curr_item_ptr->next_ptr;
            }
        curr_type_ptr = curr_type_ptr->next_ptr;
        }
    curr_node_ptr = curr_node_ptr->next_ptr;
    }
if (out_form == UE$OUT_TERSE)
    {
    if ((curr_node_ptr = first_node_ptr) == NULL)
        return;
    if ((curr_type_ptr = curr_node_ptr->type_ptr) == NULL)
        {
        free (curr_node_ptr);
        first_node_ptr = NULL;
        return;
        }
    curr_item_ptr = curr_type_ptr->item_ptr;
    while (curr_item_ptr != NULL)
        {
        next_item_ptr = curr_item_ptr->next_ptr;
        free (curr_item_ptr);
        curr_item_ptr = next_item_ptr;
        }
    free (curr_type_ptr);
    free (curr_node_ptr);
    first_node_ptr = NULL;
    }
}
