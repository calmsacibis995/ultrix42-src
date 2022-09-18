
#ifndef lint
static	char	*sccsid = "@(#)esc.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 *			ESC.C
 *
 *	SNAPSHOT EBox Scratch Pad Display Routine
 */

#include <stdio.h>

#define ZERO 0
#define VALID_RECORD 1
#define TRUE 1
#define FALSE 0

extern bytes_used;
extern rec_length;



void esc_print()
{
	int	i,b,skip;


	rec_length = ZERO;
	skip = FALSE;

	if (getbyte() != VALID_RECORD)
	   {
	    printf("EBox Scratch Pad Record is Invalid..... Skiping \n");
	    skip = TRUE;
	   }

	b = getbyte();
	rec_length = (getbyte() << 8) + b;

	if (skip == FALSE)
	{
	  skip_bytes(6);  /* skip over .CDF file name */

	  printf("\n\tEBox Scratch Pad Contents\n\n");
	  printf("General Purpose Registers\n\n");

	  printf("R0:  ");
	  pnt_longword();
	  printf("\tR1:  ");
	  pnt_longword();
	  printf("\tR2:  ");
	  pnt_longword();
	  printf("\tR3:  ");
	  print_longword();

	  printf("R4:  ");
	  pnt_longword();
	  printf("\tR5:  ");
	  pnt_longword();
	  printf("\tR6:  ");
	  pnt_longword();
	  printf("\tR7:  ");
	  print_longword();

	  printf("R8:  ");
	  pnt_longword();
	  printf("\tR9:  ");
	  pnt_longword();
	  printf("\tR10: ");
	  pnt_longword();
	  printf("\tR11: ");
	  print_longword();

	  printf("AP:  ");
	  pnt_longword();
	  printf("\tFP:  ");
	  pnt_longword();
	  printf("\tSP:  ");
	  pnt_longword();
	  printf("\tPC:  ");
	  print_longword();

	  printf("\n\nTemporaries:\n\n");
	  for (i = 0x10; i < 0x17; i++)
		{
		  printf("%x:\t",i);
		  print_longword();
		}

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tSFBYCT\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tEHMSTS\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tEVMQSAV\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tEBCS\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tEDPSR\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tCSLINT\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tIBESR\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tEBXWD1\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tEBXWD2\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tIVASAV\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tVIBASAV\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tESASAV\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tISASAV\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tCPC\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tMSTAT1\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tMSTAT2\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tMDECC\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tMERG\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tCSHCTL\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tMEAR\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tMEDR\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tFBXERR\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tCSES\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tPC\n");

	  printf("%x:\t",i++);
	  pnt_longword();
	  printf("\tPSL\n");

	  for (i = 0x30; i < 0x100; i++)
		{
		  printf("%x:\t",i);
		  print_longword();
		}




	}

	skip_to_end_of_rec();

}
