#ifndef lint
static char sccsid[]  =  "@(#)dsd_build.c	4.2   (ULTRIX)   10/16/90";
#endif lint 
/*
**	.title DSD Table Build Functions
**	.ident / 1.16 /
**
**
**	  File:	dsd_build.c
** Description:	DSD Table Build Functions
**	Author:	Luis Arce
**	  Date:	10-Nov-1986
**
**
**	Copyright 1986, Digital Equipment Corporation
**
**
**++
**	The DSD table build functions is a separate module that builds
**	the proper STD and O/S data structures using the data from the 
**	binary tables.
**
**	Several externs are created to allow outside access to the 
**	information in the structures.
**
**	The output structures can be compiled alone and then linked
**	with other onother object module.
**--
*/


#include <sys/file.h>
#include <limits.h>
#include <stdio.h>
#include "generic_dsd.h"	/* DSD table structure definitions */
#include "os_dsd.h"
#include "std_dsd.h"

/**********************************************************************/

#define os_codes_max			 1000
#define os_items_max			 1000
#define std_codes_max			10000
#define std_regs_max			10000
#define std_items_max			 5000
#define std_items_in_seg_max		10000
#define std_segs_max			 1500
#define labels_max			15000
#define strings_area_max	       200000

DD$OS_CODES_DSD      os_codes[os_codes_max+2];
DD$OS_ITEMS_DSD      os_items[os_items_max+2];
DD$STD_CODES_DSD     std_codes[std_codes_max+2];
DD$STD_REGS_DSD      std_regs[std_regs_max+2];
DD$STD_ITEMS_DSD     std_items[std_items_max+2];
DD$STD_SEG_ITEMS_DSD std_seg_items[std_items_in_seg_max+2];
DD$STD_SEGS_DSD      std_segs[std_segs_max+2];
DD$DSP_LABELS        labels[labels_max+2];
char		     strings_area[strings_area_max+2];

long		os_codes_count		= 1;
long		os_items_count		= 1;
long		std_codes_count		= 1;
long		std_regs_count		= 1;
long		std_items_count		= 1;
long		std_seg_items_count	= 1;
long		std_segs_count		= 1;
long		labels_count		= 1;
long		strings_area_count	= 1;

char            label[256];



/********************************************************************/

void  print_stuff ();
short get_item_size ();
long  get_item_index ();
long  save_label();

/********************************************************************/

main ()
{

FILE	*ifp;				/* file pointer */
long    ofd;				/* output file descriptor */

long    i;
long    j;
long	off;
short   item_id;
long	status;
long    format;
long	count;

long    file_ver;
char    file_name[32];
char    name[256];
float	version;

strcpy(file_name,"uerf.bin");

if ((ifp = fopen("ultrix_dsd.bin", "r")) == NULL)
    {
    printf("\nCan't open input file ultrix_dsd.bin!\n");
    return(DD$BAD_BIN_FILE);
    }
status = fscanf(ifp, "%ld", &file_ver);
if (status != 1)
    {
    printf("\nError reading .bin file - no file version!\n");
    print_stuff(status);
    return(DD$BAD_BIN_FILE);
    }

status = fscanf(ifp, "%[^|]|", name);	/* Bypass header */

status = fscanf(ifp, " Start of %s */ %ld", name, &format);

if (status != 2)
    {
    printf("\nError reading .bin file! a\n");
    print_stuff(status);
    return(DD$BAD_BIN_FILE);
    }
if (format != 3)
    {
    printf("\nWrong format .bin file!\n");
    return(DD$BAD_BIN_FILE);
    }

status = fscanf(ifp, " %f %ld",
			&version,
			&count);
if (status != 2)
    {
    printf("\nError reading .bin file! b\n");
    print_stuff(status);
    return(DD$BAD_BIN_FILE);
    }


/************************* STD-ITEM LOOP *************************/

for (std_items_count = 1; std_items_count <= count; std_items_count++)
    {
    if (std_items_count >= std_items_max)
        {
        printf("\nERROR - Not enough STD_ITEMS space!\n");
        print_stuff(99);
        return(DD$BAD_BIN_FILE);
        }
    std_items[std_items_count].COUNT = 0;
    std_items[std_items_count].INDEX = 0;

    status = fscanf(ifp, " %hd %s %hd",
			&std_items[std_items_count].ID,
			name,
			&std_items[std_items_count].TYPE);
    if (status != 3)
	{
	printf("\nError reading .bin file! c, count = %d\n", count);
        print_stuff(status);
	return(DD$BAD_BIN_FILE);
	}

    switch (std_items[std_items_count].TYPE)
      {
      case DT_BYTE_VECTOR:
      case DT_COUNTED_SHORT_VECTOR:
      case DT_COUNTED_LONG_VECTOR:
      case DT_ADDR_CNT_VECTOR:
	status = fscanf(ifp, " %hd",
			&std_items[std_items_count].COUNT);
	if (status != 1)
	    {
	    printf("\nError reading .bin file! d, count = %d\n", count);
            print_stuff(status);
	    return(DD$BAD_BIN_FILE);
	    }
        status = fscanf(ifp, " %hd |%[^|]|",
			&labels[labels_count].TYPE,
                        label);
        if (status != 2)
	    {
	    printf("\nError reading .bin file! e, count = %d\n", count);
            print_stuff(status);
	    return(DD$BAD_BIN_FILE);
	    }
	std_items[std_items_count].LABEL_IX = save_label();
      break;


			/********* CODED ITEM PROCESSING *********/

      case DT_SHORT_INDEX:
      case DT_INDEXED:
        status = fscanf(ifp, " %hd |%[^|]|",
			&labels[labels_count].TYPE,
                        label);
        if (status != 2)
	    {
	    printf("\nError reading .bin file! fcount = %d\n", count);
            print_stuff(status);
	    return(DD$BAD_BIN_FILE);
	    }
	std_items[std_items_count].LABEL_IX = save_label();

        status = fscanf(ifp, " %hd",
			&std_items[std_items_count].COUNT);
        if (status != 1)
	    {
	    printf("\nError reading .bin file! g, count = %d\n", count);
            print_stuff(status);
	    return(DD$BAD_BIN_FILE);
	    }

	if (std_items[std_items_count].COUNT != 0);
	    {
	    std_items[std_items_count].INDEX = std_codes_count;

				/******** CODE STRUCTURES ******/

	    for (i = 0; i < std_items[std_items_count].COUNT;
                            i++, std_codes_count++)
	        {
                if (std_codes_count >= std_codes_max)
                    {
                    printf("\nERROR - Not enough STD_CODES space!\n");
                    print_stuff(99);
                    return(DD$BAD_BIN_FILE);
                    }
	        status = fscanf (ifp, " %ld %s |%[^|]|", 
			&std_codes[std_codes_count].CODE,
			name,
                        label);
	        if (status != 3)
		    {
		    printf("\nError reading .bin file! h, count = %d, i = %d\n", count, i);
                    print_stuff(status);
		    return(DD$BAD_BIN_FILE);
		    }
		std_codes[std_codes_count].LABEL_IX = save_label();
	        }
	    }
      break;


			/******* REGISTER ITEM PROCESSING *******/

      case DT_SHORT_REGISTER:
      case DT_REGISTER:
        status = fscanf(ifp, " %hd |%[^|]|",
			&labels[labels_count].TYPE,
                        label);
        if (status != 2)
	    {
	    printf("\nError reading .bin file! i, count = %d\n", count);
            print_stuff(status);
	    return(DD$BAD_BIN_FILE);
	    }
	std_items[std_items_count].LABEL_IX = save_label();

        status = fscanf(ifp, " %hd",
			&std_items[std_items_count].COUNT);
        if (status != 1)
	    {
	    printf("\nError reading .bin file! j, count = %d\n", count);
            print_stuff(status);
	    return(DD$BAD_BIN_FILE);
	    }

	if (std_items[std_items_count].COUNT != 0);
	    {
	    std_items[std_items_count].INDEX = std_regs_count;

	    		/********** REGISTER FIELD LOOP **********/

	    for (i = 0; i < std_items[std_items_count].COUNT;
                            i++, std_regs_count++)
	        {
                if (std_regs_count >= std_regs_max)
                    {
                    printf("\nERROR - Not enough STD_REGS space!\n");
                    print_stuff(99);
                    return(DD$BAD_BIN_FILE);
                    }
                status = fscanf (ifp, " %hd %hd %hd |%[^|]|",
                	&std_regs[std_regs_count].SIZE,
                	&std_regs[std_regs_count].TYPE,
                        &labels[labels_count].TYPE,
                        label);
                if (status != 4)
		    {
		    printf("\nError reading .bin file! k, count = %d, i = %d\n", count, i);
                    print_stuff(status);
		    return(DD$BAD_BIN_FILE);
		    }
		std_regs[std_regs_count].LABEL_IX = save_label();
		std_regs[std_regs_count].COUNT = 0;
		std_regs[std_regs_count].CODE_IX = 0;
	        if (std_regs[std_regs_count].TYPE == DC_CODED)
		    {
	            status = fscanf(ifp, " %hd",
				&std_regs[std_regs_count].COUNT);
	            if (status != 1)
		        {
  		        printf("\nError reading .bin file! l, count = %d, i = %d\n", count, i);
                        print_stuff(status);
		        return(DD$BAD_BIN_FILE);
		        }

		    if (std_regs[std_regs_count].COUNT != 0)
			{
                        std_regs[std_regs_count].CODE_IX = std_codes_count;

			/********* CODED FIELD PROCESSING *********/

	                for (j = 0; j < std_regs[std_regs_count].COUNT;
                                    j++, std_codes_count++)
	        	    {
                            if (std_codes_count >= std_codes_max)
                                {
                                printf("\nERROR - Not enough STD_CODES space!\n");
                                print_stuff(99);
                                return(DD$BAD_BIN_FILE);
                                }
		            status = fscanf (ifp, " %ld |%[^|]|",
				&std_codes[std_codes_count].CODE,
                        	label);
		            if (status != 2)
		                {
        		        printf("\nError reading .bin file! m, count = %d, i = %d, j = %d\n", count, i, j);
                                print_stuff(status);
		                return(DD$BAD_BIN_FILE);
		                }
			    std_codes[std_codes_count].LABEL_IX = save_label();
			    }
		        }
		    }
	        }
	    }
      break;
      default:
        status = fscanf(ifp, " %hd |%[^|]|",
			&labels[labels_count].TYPE,
                        label);
        if (status != 2)
	    {
	    printf("\nError reading .bin file! n, count = %d\n", count);
            print_stuff(status);
	    return(DD$BAD_BIN_FILE);
	    }
	std_items[std_items_count].LABEL_IX = save_label();
      break;
      }
    }


/**************************** STD_SEGMENT_STRUCT ***********************/

status = fscanf(ifp, " %f %ld",
	    		&version,
	    		&count);
if (status != 2)
    {
    printf("\nError reading .bin file! o\n");
    print_stuff(status);
    return(DD$BAD_BIN_FILE);
    }

for (std_segs_count = 0; std_segs_count < count; std_segs_count++)
    {
    if (std_segs_count >= std_segs_max)
        {
        printf("\nERROR - Not enough STD_SEGS space!\n");
        print_stuff(99);
        return(DD$BAD_BIN_FILE);
        }
    status = fscanf(ifp, " %hd %hd |%[^|]| %hd",
			&std_segs[std_segs_count].TYPE,
			&std_segs[std_segs_count].SUBTYPE,
                        label,
			&std_segs[std_segs_count].COUNT);
    if (status != 4)
	{
	printf("\nError reading .bin file! p, count = %d\n", count);
        print_stuff(status);
	return(DD$BAD_BIN_FILE);
	}
    std_segs[std_segs_count].LABEL_IX = save_label();
    if (std_segs[std_segs_count].COUNT != 0)
	{
	std_segs[std_segs_count].SEG_ITEM_IX = std_seg_items_count;
	off = (DD$HEADER_BYTES + DD$VALID_BYTES(std_segs[std_segs_count].COUNT));
	off +=3;
	off &= 0xfffffffc;		/* longword alignment needed.  */
	std_segs[std_segs_count].STR_OFFSET = off; 

		/************ ITEMS IN SEGMENT LOOP *****************/

	for (i = 1; i <= std_segs[std_segs_count].COUNT;
		i++, std_seg_items_count++)
	    {
            if (std_seg_items_count >= std_items_in_seg_max)
                {
                printf("\nERROR - Not enough STD_SEG_ITEMS space!\n");
                print_stuff(99);
                return(DD$BAD_BIN_FILE);
                }
	    status = fscanf (ifp," %hd", &item_id);
	    if (status != 1)
	        {
	        printf("\nError reading .bin file! q, count = %d, i = %d\n", count, i);
                print_stuff(status);
	        return(DD$BAD_BIN_FILE);
	        }
	    j = get_item_index(item_id);
	    std_seg_items[std_seg_items_count].ITEM_IX = j;
	    std_seg_items[std_seg_items_count].ITEM_OFFSET = 
	    		std_segs[std_segs_count].STR_OFFSET;
	    off= get_item_size(std_items[j].TYPE, std_items[j].COUNT);
	    off +=3;
	    off &= 0xfffffffc;		/* longword alignment needed.  */
		    std_segs[std_segs_count].STR_OFFSET += off;
	    }
	}
    }

/**************************** OS_CODE_STRUCT ***************************/

status = fscanf(ifp, " %f %ld",
		    		&version,
		    		&count);
if (status != 2)
    {
    printf("\nError reading .bin file! r\n");
    print_stuff(status);
    return(DD$BAD_BIN_FILE);
    }

for (os_items_count = 0; os_items_count < count; os_items_count++)
    {
    if (os_items_count >= os_items_max)
        {
        printf("\nERROR - Not enough OS_ITEMS space!\n");
        print_stuff(99);
        return(DD$BAD_BIN_FILE);
        }
    os_items[os_items_count].COUNT = 0;
    status = fscanf(ifp, " %hd %hd",
			&os_items[os_items_count].ID,
    			&os_items[os_items_count].TYPE);
    if (status != 2)
	{
	printf("\nError reading .bin file! s, count = %d\n", count);
        print_stuff(status);
	return(DD$BAD_BIN_FILE);
	}

    switch (os_items[os_items_count].TYPE)
      {
      case DT_BYTE_VECTOR:
      case DT_COUNTED_SHORT_VECTOR:
      case DT_COUNTED_LONG_VECTOR:
      case DT_ADDR_CNT_VECTOR:
      case DT_ASCIZ:
      case DT_BIT_VECTOR:
      case DT_SHORT_VECTOR:
      case DT_LONG_VECTOR:
	status = fscanf(ifp, " %hd",
			&os_items[os_items_count].COUNT);
	if (status != 1)
	    {
	    printf("\nError reading .bin file! t, count = %d\n", count);
            print_stuff(status);
	    return(DD$BAD_BIN_FILE);
	    }
      break;
      case DT_TINY_INDEX:
      case DT_SHORT_INDEX:
      case DT_INDEXED:
	status = fscanf(ifp, " %hd",
			&os_items[os_items_count].COUNT);
	if (status != 1)
	    {
	    printf("\nError reading .bin file! u, count = %d\n", count);
            print_stuff(status);
	    return(DD$BAD_BIN_FILE);
	    }
	if (os_items[os_items_count].COUNT != 0)
	    {
	    os_items[os_items_count].CODE_IX = os_codes_count;

			/************** OS CODES  ******************/
	    for (i = 0; i < os_items[os_items_count].COUNT;
			i++, os_codes_count++)
	        {
                if (os_codes_count >= os_codes_max)
                    {
                    printf("\nERROR - Not enough OS_CODES space!\n");
                    print_stuff(99);
                    return(DD$BAD_BIN_FILE);
                    }
	        status = fscanf(ifp, " %lu %lu", 
			&os_codes[os_codes_count].OS_CODE,
			&os_codes[os_codes_count].STD_CODE);
	        if (status != 2)
		    {
		    printf("\nError reading .bin file! v, count = %d, i = %d\n", count, i);
                    print_stuff(status);
		    return(DD$BAD_BIN_FILE);
		    }
	        }
	    }
      break;
      }
    }
if ((ofd = creat("uerf.bin", 0444)) == -1)
    {
    printf("\nCan't open output file - uerf.bin!\n");
    return(DD$BAD_BIN_FILE);
    }

i = DD$BIN_FILE_VER_ID;
if(write (ofd, &i, 2) == -1)
    {
    printf("\nUnable to write to uerf.bin file!\n");
    return(DD$BAD_BIN_FILE);
    }
i = 1;
write (ofd, &i, 4);
write (ofd, &file_ver, 4);

i = DD$OS_CODES_ID;
write (ofd, &i, 2);
write (ofd, &os_codes_count, 4);
write (ofd, os_codes, (sizeof(DD$OS_CODES_DSD)*(os_codes_count+1)));

i = DD$OS_ITEMS_ID;
write (ofd, &i, 2);
write (ofd, &os_items_count, 4);
write (ofd, os_items, (sizeof(DD$OS_ITEMS_DSD)*(os_items_count+1)));

i = DD$STD_CODES_ID;
write (ofd, &i, 2);
write (ofd, &std_codes_count, 4);
write (ofd, std_codes, (sizeof(DD$STD_CODES_DSD)*(std_codes_count+1)));

i = DD$STD_REGS_ID;
write (ofd, &i, 2);
write (ofd, &std_regs_count, 4);
write (ofd, std_regs, (sizeof(DD$STD_REGS_DSD)*(std_regs_count+1)));

i = DD$STD_ITEMS_ID;
write (ofd, &i, 2);
write (ofd, &std_items_count, 4);
write (ofd, std_items, (sizeof(DD$STD_ITEMS_DSD)*(std_items_count+1)));

i = DD$STD_SEG_ITEMS_ID;
write (ofd, &i, 2);
write (ofd, &std_seg_items_count, 4);
write (ofd, std_seg_items, (sizeof(DD$STD_SEG_ITEMS_DSD)*(std_seg_items_count+1)));

i = DD$STD_SEGS_ID;
write (ofd, &i, 2);
write (ofd, &std_segs_count, 4);
write (ofd, std_segs, (sizeof(DD$STD_SEGS_DSD)*(std_segs_count+1)));

i = DD$DSP_LABELS_ID;
write (ofd, &i, 2);
write (ofd, &labels_count, 4);
write (ofd, labels, (sizeof(DD$DSP_LABELS)*(labels_count+1)));

i = DD$STRINGS_ID;
write (ofd, &i, 2);
write (ofd, &strings_area_count, 4);
write (ofd, strings_area, (strings_area_count+1));
close (ofd);

printf ("\nSUCCESSFUL\n");
print_stuff(99);
return (0);
}


/******************** PRINT STUFF ON ERRORS *********************
 *
 ****************************************************************/

void print_stuff(stat)
long  stat;
{
if (stat != 99)
    {
    printf("\n STATUS / items read    = %6.6d", stat);
    printf("\n");
    printf("\n ITEM_ID    = %4d", std_items[std_items_count].ID);
    printf("\n ITEM_TYPE  = %4d", std_items[std_items_count].TYPE);
    printf("\n ITEM_COUNT = %4d", std_items[std_items_count].COUNT);
    printf("\n");
    printf("\n REG_SIZE   = %4d", std_regs[std_regs_count].SIZE);
    printf("\n REG_TYPE   = %4d", std_regs[std_regs_count].TYPE);
    printf("\n REG_COUNT  = %4d", std_regs[std_regs_count].COUNT);
    printf("\n");
    printf("\n ITEM_CODE  = %4d", std_codes[std_codes_count].CODE);
    printf("\n");
    printf("\n SEG_TYPE   = %4d", std_segs[std_segs_count].TYPE);
    printf("\n SEG_SUBTYPE= %4d", std_segs[std_segs_count].SUBTYPE);
    printf("\n SEG_COUNT  = %4d", std_segs[std_segs_count].COUNT);
    printf("\n");
    printf("\n OS_ID      = %4d", os_items[os_items_count].ID);
    printf("\n OS_TYPE    = %4d", os_items[os_items_count].TYPE);
    printf("\n OS_COUNT   = %4d", os_items[os_items_count].COUNT);
    printf("\n");
    printf("\n OS_CODE    = %4d", os_codes[os_codes_count].OS_CODE);
    printf("\n OS_STD_CODE= %4d", os_codes[os_codes_count].STD_CODE);
    printf("\n");
    }

printf("\n os_codes_count         = %6.6d, max = %6.6d, free = %6.6d",
			os_codes_count,
			os_codes_max,
			os_codes_max - os_codes_count);
if ((100 * os_codes_count / os_codes_max) > 90)
    printf("  *** LOW ***");
printf("\n os_items_count         = %6.6d, max = %6.6d, free = %6.6d",
			os_items_count,
			os_items_max,
			os_items_max - os_items_count);
if ((100 * os_items_count / os_items_max) > 90)
    printf("  *** LOW ***");
printf("\n std_codes_count        = %6.6d, max = %6.6d, free = %6.6d",
			std_codes_count,
			std_codes_max,
			std_codes_max - std_codes_count);
if ((100 * std_codes_count / std_codes_max) > 90)
    printf("  *** LOW ***");
printf("\n std_regs_count         = %6.6d, max = %6.6d, free = %6.6d",
			std_regs_count,
			std_regs_max,
			std_regs_max - std_regs_count);
if ((100 * std_regs_count / std_regs_max) > 90)
    printf("  *** LOW ***");
printf("\n std_items_count        = %6.6d, max = %6.6d, free = %6.6d",
			std_items_count,
			std_items_max,
			std_items_max - std_items_count);
if ((100 * std_items_count / std_items_max) > 90)
    printf("  *** LOW ***");
printf("\n std_seg_items_count    = %6.6d, max = %6.6d, free = %6.6d",
			std_seg_items_count,
			std_items_in_seg_max,
			std_items_in_seg_max - std_seg_items_count);
if ((100 * std_seg_items_count / std_items_in_seg_max) < 10)
    printf("  *** LOW ***");
printf("\n std_segs_count         = %6.6d, max = %6.6d, free = %6.6d",
			std_segs_count,
			std_segs_max,
			std_segs_max - std_segs_count);
if ((100 * std_segs_count / std_segs_max) > 90)
    printf("  *** LOW ***");
printf("\n labels_count           = %6.6d, max = %6.6d, free = %6.6d",
			labels_count,
			labels_max,
			labels_max - labels_count);
if ((100 * labels_count / labels_max) > 90)
    printf("  *** LOW ***");
printf("\n strings_area_count     = %6.6d, max = %6.6d, free = %6.6d",
			strings_area_count,
			strings_area_max,
			strings_area_max - strings_area_count);
if ((100 * strings_area_count / strings_area_max) > 90)
    printf("  *** LOW ***");
printf("\n");
}


/********************** SAVE LABEL FUNCTION **********************
 *
 *		INPUT	        label string is global	
 *
 *		OUTPUT		label_index
 *
 ****************************************************************/

long  save_label()
{
long    i;
short   len;

len = strlen(label);

for (i = 0; i < len; i++)
    toupper(label[i]);
    
for (i = 1; i <= labels_count; i++)
    {
    if ((labels[i].COUNT == len) &&
        (strcmp (&strings_area[labels[i].STRINGS_IX], label) == 0))
        {
        labels[labels_count].STRINGS_IX = labels[i].STRINGS_IX;
        break;
        }
    }
if (i > labels_count)           /* could not find match */
    {
    strcpy (&strings_area[strings_area_count],label);            
    labels[labels_count].STRINGS_IX = strings_area_count;
    strings_area_count += (len + 1);
    }
labels[labels_count].COUNT = len;
if (labels_count >= labels_max)
    {
    printf("\nERROR - Not enough LABELS space!\n");
    print_stuff(99);
    return(DD$BAD_BIN_FILE);
    }
return (labels_count++);
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

