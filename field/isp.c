
#ifndef lint
static	char	*sccsid = "@(#)isp.c	4.1	(ULTRIX)	7/2/90";
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
 *		     ISP.C
 *
 *	SNAPSHOT Interupt Stack Display Routine
 *
 *	Modification History
 *
 *	15-Jan-88  Report the machinecheck stack frame if valid
 *                 Report first 64 longwords on interupt stack
 *							     swc
 */

#include <stdio.h>

#define ZERO 0
#define VALID_RECORD 1
#define TRUE 1
#define FALSE 0

extern bytes_used;
extern rec_length;


void isp_print()
{
	int	b,i,skip,count;


	rec_length = ZERO;
	skip = FALSE;

	if (getbyte() != VALID_RECORD)
	   {
	    printf("Interrupt Stack Record is Invalid..... Skiping \n");
	    skip = TRUE;
	   }

	b = getbyte();
	rec_length = (getbyte() << 8) + b;

	if (skip == FALSE)
	{
	  skip_bytes(6);  /* skip over .CDF file name */

	  printf("\nInterupt Stack, Top 64 Longwords\n\n");

	  for (i = 0; i < 64; i++)
	  {
	     printf("Longword\t%x\t",i);
	     print_longword();
	  }

	  printf("\n");

	  printf("\n Machinecheck Stack Frame");
	  count = getbyte(); /* machck stack frame byte count*/
	  getbyte();  /* read the rest of the longword, trash it */
	  getbyte();
	  getbyte();
	  if (count == 0x58)
	     printf(" is VALID\n\n");
	  else
	     printf(" is INVALID\n\n");
	  
	  printf("Byte Cnt\t%x\n", count);

	  printf("EHM.STS\t\t");
	  print_longword();

	  printf("EVMQSAV\t\t");
	  print_longword();

	  printf("EBCS\t\t");
	  print_longword();

	  printf("EDPSR\t\t");
	  print_longword();

	  printf("CSLINT\t\t");
	  print_longword();

	  printf("IBESR\t\t");
	  print_longword();

	  printf("EBXWD1\t\t");
	  print_longword();

	  printf("EBXWD2\t\t");
	  print_longword();

	  printf("IVASAV\t\t");
	  print_longword();

	  printf("VIBASAV\t\t");
	  print_longword();

	  printf("ESASAV\t\t");
	  print_longword();

	  printf("ISASAV\t\t");
	  print_longword();

	  printf("CPC\t\t");
	  print_longword();

	  printf("MSTAT1\t\t");
	  print_longword();

	  printf("MSTAT2\t\t");
	  print_longword();

	  printf("MDECC\t\t");
	  print_longword();

	  printf("MERG\t\t");
	  print_longword();

	  printf("CSHCTL\t\t");
	  print_longword();

	  printf("MEAR\t\t");
	  print_longword();

	  printf("MEDR\t\t");
	  print_longword();

	  printf("FBXERR\t\t");
	  print_longword();

	  printf("CSES\t\t");
	  print_longword();

	  printf("PC\t\t");
	  print_longword();

	  printf("PSL\t\t");
	  print_longword();

	  printf("\n");

	  printf("\n Interupt Stack, First 64 Longwords\n\n");
	  for (i = 0; i < 64; i++)
	  {
	     printf("Longword\t%x\t",i);
	     print_longword();
	  }

	  printf("\n");

	}

	skip_to_end_of_rec();

}
