
#ifndef lint
static	char	*sccsid = "@(#)mstr.c	4.1	(ULTRIX)	7/2/90";
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
 *			MSTR.C
 *
 *	SNAPSHOT Master Header Format Display Routine
 *
 *	Does not print the value of the TOY clock
 *
 *  Modification History:
 *
 *	15-Jan-88   Update CSM & KAF Error Messages
 *		    Fix Console Version number printing backwards
 *		    Decode clock status word.
 *                                             swc
 */

#include <stdio.h>

#define VALID_RECORD 0x1		/* SNAPSHOT file valid */
#define ZERO 0


extern bytes_used;
extern rec_length;


void mst_print()
{
	int	b,clock;
	int	major,minor;  /* Revision levels major.minor*/


	rec_length = ZERO;

	if (getbyte() != VALID_RECORD)
		{
		 printf("Invalid SNAPSHOT File \n");
		 exit(1);
		}

	b = getbyte();
	rec_length = (getbyte() << 8) + b;

	skip_bytes(8);	/* skip over CPU program name and SNAPSHOT file length*/
	printf("\n\tSNAPSHOT File Master Header Record\n\n");

	printf("CSM Status Code: ");
	b = getbyte();
	switch (b)
	{
	     case 0   : printf("0 = CSM can't run after KAF\n");
		        break;
	     case 0x4 : printf("4 = Interupt Stack not valid\n");
		        break;
	     case 0x5 : printf("5 = Non-EBox double error\n");
			break;
	     case 0x6 : printf("6 = Kernel mode Halt\n");
			break;
	     case 0x7 : printf("7 = SCB vector with <1:0> = 3 \n");
			break;
	     case 0x8 : printf("8 = SCB vector with <1:0> = 2 but no WCS\n");
			break;
	     case 0x9 : printf("9 = Pending error on HALT\n");
			break;
	     case 0xa : printf("a = CHMX with IS = 1 \n");
			break;
	     case 0xb : printf("b = CHMX vector <1:0> not 0 \n");
			break;
	     default  : printf("%x = Unknown Status Code \n",b);
	}

	printf("KAF Reason Code: ");
	b = getbyte();
	switch (b)
	{
	     case 0x18  : printf("18 = ECS A & B Parity Error\n");
			  break;
	     case 0x19  : printf("19 = Error while EHM handling an error\n");
			  break;
	     case 0x1a  : printf("1a = WBUS parity error detected by EBox\n");
			  break;
	     case 0x1b  : printf("1b = CPU ERROR HALT\n");
			  break;
	     case 0x1c  : printf("1c = Non-correctable parity error\n");
			  break;
	     case 0x1d  : printf("1d = Power system failure, DC LO without AC LO\n");
			  break;
	     case 0x1e  : printf("1e = Unidentified KAF occured\n");
			  break;
	     case 0x1f  : printf("1f = MBOX/SBIA DMA Command Error or NXM\n");
			  break;
	     default    : printf("%x = Unknown reason code\n");
	}

	skip_bytes(4);	/* skip over TOY value */
	printf("\n");

	printf("Console PROM Revision\tV%d\n",getbyte());

	printf("EMM PROM Revision\tV%d\n",getbyte());

	printf("Minimun\tHW Revision\tV%d.%d\n",getbyte(),getbyte());

	printf("EBox\tUcode Revision\tV%d.%d\n",getbyte(),getbyte());

	printf("IBox\tUcode Revision\tV%d.%d\n",getbyte(),getbyte());

	printf("Mbox\tUcode Revision\tV%d.%d\n",getbyte(),getbyte());

	printf("FBoxA\tUcode Revision\tV%d.%d\n",getbyte(),getbyte());

	printf("FBoxM\tUcode Revision\tV%d.%d\n",getbyte(),getbyte());

	printf("ACCESS\tUcode Revision\tV%d.%d\n",getbyte(),getbyte());

	printf("CPR\tUcode Revision\tV%d.%d\n",getbyte(),getbyte());

	printf("MCF\tUcode Revision\tV%d.%d\n",getbyte(),getbyte());

	printf("CONTEXT\tUcode Revision\tV%d.%d\n",getbyte(),getbyte());

	printf("Ucode Release Revision\tV%d.%d\n",getbyte(),getbyte());

	printf("\n");

	printf("EBox\tUcode Verification\t%d\n",getbyte());

	printf("MCF\tUcode Verification\t%d\n",getbyte());

	printf("CONTEXT\tUcode Verification\t%d\n",getbyte());

	printf("FBoxA\tUcode Verification\t%d\n",getbyte());

	printf("FBoxM\tUcode Verification\t%d\n",getbyte());

	printf("FDRAM\tUcode Verification\t%d\n",getbyte());

	printf("IBox\tUcode Verification\t%d\n",getbyte());

	printf("IDRAM\tUcode Verification\t%d\n",getbyte());

	printf("MBox\tUcode Verification\t%d\n",getbyte());

	printf("CPR\tUcode Verification\t%d\n",getbyte());

	printf("ACCESS\tUcode Verification\t%d\n",getbyte());

	printf("PAMM\t      Verification\t%d\n",getbyte());

	printf("MCC Shift Channel\t\t");
	for (b = 0; b < 11; b++)
		printf("%02x ",getbyte());
	printf("\n");



	/* Newer versions of the Console software added new things to the 
	 * end of the master header record. Thus depending on the version 
	 * of the Console software that is running some Snapshot files will 
	 * not have the following information in them. Check the record length 
	 * and print things if they are there.
	 *
	 * This will be an ongoing maintenance problem for this program if
	 * things keep getting added.
	 */

	if (rec_length > 0x42)
	{
	  getbyte();	/* skip over a spare byte */

	  major = getbyte();  
	  minor = getbyte();  
	  printf("Console Software Version\tV%d.%d\n",major,minor);
	}

	if (rec_length > 0x44)
	{
	  clock = ((getbyte() << 8) | getbyte());
	  printf("CPU Clock Status\t\t0x%x\n", clock);
	  if (clock & 0x7fff)
	     printf("\t\t\t\t\tClock Rate Full\n");
          else
	     printf("\t\t\t\t\tClock Rate 1/5\n");
	  
	  if (clock & 0x80ff)
	     printf("\t\t\t\t\t%dMhz\n", ((clock & 0x7f00) >> 8));
	  else
	     printf("\t\t\t\t\tExternal Clock\n");
	  if (clock & 0x0010)
	     printf("\t\t\t\t\tClock Running\n");
          else
	     printf("\t\t\t\t\tClock Stopped, Phase = %d\n", (clock & 0x000f));
	}

	if (rec_length > 0x46)
	{
	  printf("Calculated CSPE Syndrome\t");
	  print_longword();
	}


	printf("\n");
	skip_to_end_of_rec();
}
