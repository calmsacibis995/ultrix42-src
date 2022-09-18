
#ifndef lint
static	char	*sccsid = "@(#)csl.c	4.1	(ULTRIX)	7/2/90";
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
 *			CSL.C
 *
 *	SNAPSHOT Console Registers Display Routine
 */

#include <stdio.h>

#define ZERO 0
#define VALID_RECORD 1
#define TRUE 1
#define FALSE 0

extern bytes_used;
extern rec_length;


void csl_print()
{
	int	b,skip;


	rec_length = ZERO;
	skip = FALSE;

	if (getbyte() != VALID_RECORD)
	   {
	    printf("Console Registers Record is Invalid..... Skiping \n");
	    skip = TRUE;
	   }

	b = getbyte();
	rec_length = (getbyte() << 8) + b;

	if (skip == FALSE)
	{
	  skip_bytes(6);  /* skip over .CDF file name */

	  printf("\n\tConsole Registers \n\n");

	  printf("MSCR0\t%02x\n",getbyte());

	  printf("MCSR1\t%02x\n",getbyte());

	  printf("MSCR2\t%02x\n",getbyte());

	  printf("MCSR3\t%02x\n",getbyte());

	  printf("ERSR\t%02x\n",getbyte());

	  printf("LRSR\t%02x\n",getbyte());

	  printf("RRSR\t%02x\n",getbyte());

	  getbyte();	/* skip over a spare */

	  printf("QCSR0\t%02x\n",getbyte());

	  printf("QCSR1\t%02x\n",getbyte());

	  printf("QCSR2\t%02x\n",getbyte());

	  printf("QCSR3\t%02x\n",getbyte());

	  printf("SID0\t%02x\n",getbyte());

	  printf("SID1\t%02x\n",getbyte());

	  printf("SID2\t%02x\n",getbyte());

	  printf("SID3\t%02x\n",getbyte());

	  printf("RL CTRL Status\t%02x%02x\n",getbyte(),getbyte());

	  printf("RL Drive Status\t%02x%02x\n",getbyte(),getbyte());

	  printf("\n");
	}

	skip_to_end_of_rec();

}
