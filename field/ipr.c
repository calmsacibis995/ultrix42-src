
#ifndef lint
static	char	*sccsid = "@(#)ipr.c	4.1	(ULTRIX)	7/2/90";
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
 *		   IPR.C
 *
 *	SNAPSHOT IPR Registers Display Routine
 *
 *	Modification History
 *
 *	15-Jan-88	Decode SID Register       swc
 */

#include <stdio.h>

#define ZERO 0
#define VALID_RECORD 1
#define TRUE 1
#define FALSE 0

extern bytes_used;
extern rec_length;



void ipr_print()
{
	int	b,i,skip,temp;


	rec_length = ZERO;
	skip = FALSE;

	if (getbyte() != VALID_RECORD)
	   {
	    printf("IPR Registers Record is Invalid..... Skiping \n");
	    skip = TRUE;
	   }

	b = getbyte();
	rec_length = (getbyte() << 8) + b;

	if (skip == FALSE)
	{
	  skip_bytes(6);  /* skip over .CDF file name */

	  printf("\nInternal Processor Registers \n\n");

	  printf("KSP\t");
	  print_longword();

	  printf("ESP\t");
	  print_longword();

	  printf("SSP\t");
	  print_longword();

	  printf("USP\t");
	  print_longword();

	  printf("ISP\t");
	  print_longword();

	  printf("P0BR\t");
	  print_longword();

	  printf("P0LR\t");
	  print_longword();

	  printf("P1BR\t");
	  print_longword();

	  printf("P1LR\t");
	  print_longword();

	  printf("SBR\t");
	  print_longword();

	  printf("SLR\t");
	  print_longword();

	  printf("PCBB\t");
	  print_longword();

	  printf("SCBB\t");
	  print_longword();

	  printf("IPL\t");
	  print_longword();

	  printf("ASTLVL\t");
	  print_longword();

	  printf("SISR\t");
	  print_longword();

	  printf("ICCS\t");
	  print_longword();

	  printf("ICR\t");
	  print_longword();

	  printf("TODR\t");
	  print_longword();

	  printf("RXCS\t");
	  print_longword();

	  printf("RXDB\t");
	  print_longword();

	  printf("TXCS\t");
	  print_longword();

	  printf("ACCS\t");
	  print_longword();

	  printf("MAPEN\t");
	  print_longword();

	  printf("PME\t");
	  print_longword();

	  printf("SID\t");
	  temp = ZERO;			/* make a longword by hand */
	  for (i = 0; i < 4; i++)
	      temp = (temp | (getbyte() << 8*i));
	  printf("%x\n");  
	  printf("\t\tSerial Number: 0x%x\n", (temp & 0x00000fff));
	  printf("\t\tPlant: 0x%x\n", ((temp & 0x0000f000) >> 12));
	  printf("\t\tECO Level: 0x%x\n", ((temp & 0x007f0000) >> 16));
	  if (temp & 0x00800000)
	     printf("\t\tType: VAX 8650\n");
	  else
	     printf("\t\tType: VAX 8600\n");

	  printf("PAMACC\t");
	  print_longword();

	  printf("PAMLOC\t");
	  print_longword();

	  printf("CSWP\t");
	  print_longword();

	  printf("MDECC\t");
	  print_longword();

	  printf("MENA\t");
	  print_longword();

	  printf("MDCTL\t");
	  print_longword();

	  printf("MCCTL\t");
	  print_longword();

	  printf("MERG\t");
	  print_longword();

	  printf("EHSR\t");
	  print_longword();

	  printf("STXCS\t");
	  print_longword();

	  printf("STXDB\t");
	  print_longword();

	  printf("\n\nInternal Registers\n\n");

	  printf("VPCBITS\t");
	  print_longword();

	  printf("EDPSR\t");
	  print_longword();

	  printf("EBCS\t");
	  print_longword();

	  printf("CSLINT\t");
	  print_longword();

	  printf("EDMC\t");
	  print_longword();

	  printf("IBESR\t");
	  print_longword();

	  printf("EMD\t");
	  print_longword();

	  printf("IVASAV\t");
	  print_longword();

	  printf("VIBASAV\t");
	  print_longword();

	  printf("ESASAV\t");
	  print_longword();

	  printf("ISASAV\t");
	  print_longword();

	  printf("CPC\t");
	  print_longword();

	  printf("MSTAT1\t");
	  print_longword();

	  printf("MSTAT2\t");
	  print_longword();

	  printf("MEDR\t");
	  print_longword();

	  printf("MEAR\t");
	  print_longword();

	  printf("CSHCTL\t");
	  print_longword();

	  printf("\n\nMiscellaneous Registers\n\n");

	  printf("CSES\t");
	  print_longword();

	  printf("IBGPR\t");
	  print_longword();

	  printf("PSL\t");
	  print_longword();

	  printf("SPADR\t");
	  print_longword();

	  printf("STATE\t");
	  print_longword();

	  printf("EVMQSAV\t");
	  print_longword();

	}

	skip_to_end_of_rec();

}
