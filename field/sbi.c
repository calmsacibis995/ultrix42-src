
#ifndef lint
static	char	*sccsid = "@(#)sbi.c	4.1	(ULTRIX)	7/2/90";
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
 *		   SBI.C
 *
 *	SNAPSHOT SBI Longword Display Routine
 */

#include <stdio.h>

#define ZERO 0
#define VALID_RECORD 1
#define TRUE 1
#define FALSE 0

extern bytes_used;
extern rec_length;


void sbi_print(adptr)

	int adptr;	/* adapter number, SBI0 or SBI1 */
{
	int	b,i,skip;


	rec_length = ZERO;
	skip = FALSE;

	if (getbyte() != VALID_RECORD)
	   {
	    printf("SBI%d Record is Invalid..... Skiping \n",adptr);
	    skip = TRUE;
	   }

	b = getbyte();
	rec_length = (getbyte() << 8) + b;

	if (skip == FALSE)
	{
	  skip_bytes(6);  /* skip over .CDF file name */

	  printf("\n\tSBI%d Longwords\n\n",adptr);

	  printf("Configuration     \t");
	  print_longword();

	  printf("Control Status    \t");
	  print_longword();

	  printf("Error Summary     \t");
	  print_longword();

	  printf("Diagnostic Control\t");
	  print_longword();
	  printf("\n");

	  printf("DMAI Cmd/ADDr and ID\t");
	  pnt_longword();
	  printf(" ");
	  print_longword();

	  printf("DMAA Cmd/ADDr and ID\t");
	  pnt_longword();
	  printf(" ");
	  print_longword();

	  printf("DMAB Cmd/ADDr and ID\t");
	  pnt_longword();
	  printf(" ");
	  print_longword();

	  printf("DMAC Cmd/ADDr and ID\t");
	  pnt_longword();
	  printf(" ");
	  print_longword();

	  printf("\nSBI Silo\n\n");
	  for (b = 0; b < 2; b++)
	  {
	    for (i = 0; i < 8; i++)
	    {
	      pnt_longword();
	      printf(" ");
            }
	    printf("\n\n");
	  }

	  printf("SBI Error           \t");
	  print_longword();

	  printf("SBI Time-out Address\t");
	  print_longword();

	  printf("SBI Fault Status    \t");
	  print_longword();

	  printf("SBI Silo Comparator \t");
	  print_longword();

	  printf("SBI Maintenance     \t");
	  print_longword();

	  printf("\nTR1 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR2 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR3 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR4 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR5 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR6 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR7 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR8 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR9 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR10 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR11 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR12 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR13 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR14 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();

	  printf("\nTR15 NEXUS CSR\t");
	  print_longword();

	  printf("Error Summary  \t");
	  print_longword();

	  printf("Error          \t");
	  print_longword();

	  printf("Fault Status   \t");
	  print_longword();


	}

	skip_to_end_of_rec();

}
