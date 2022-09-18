
#ifndef lint
static	char	*sccsid = "@(#)uvsr.c	4.1	(ULTRIX)	7/2/90";
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
 *
 *				UVSR.C	
 *
 *		ULTRIX-32 VAX 8600/8650 SNAPSHOT FILE REPORTER	
 *
 *	Supported Record Types:	
 *
 *		- Console Registers
 *		- EMM Registers	
 *		- EBox Scratch Pad 
 *		- IPR Register	
 *		- PAMM	
 *		- Interupt Stack
 *		- ABus Registers
 *
 */

#include <stdio.h>

#define ZERO 0

#define MST 0x20
#define FBA 0x21
#define FBM 0x22
#define MCD 0x23
#define IBD 0x24
#define IDP 0x25
#define ICA 0x26
#define ICB 0x27
#define CLK 0x28
#define EDP 0x29
#define EBE 0x2a
#define MCC 0x2b
#define MAP 0x2c
#define EBD 0x2d
#define EBC 0x2e
#define CSB 0x2f
#define CSA 0x30
#define MTM 0x31
#define CSL 0x32
#define EMM 0x33
#define IPR 0x34
#define ESC 0x35
#define PAMM 0x36
#define ISP 0x37
#define SBI0 0x38
#define SBI1 0x39

int	bytes_used;	/* number of bytes used in current record */
int	rec_length;	/* length in bytes of current record */



void print_longword()	/* print out a longword, do a <CR> at the end */
{
	int	b0,b1,b2,b3;

	b0 = getbyte();
	b1 = getbyte();
	b2 = getbyte();
	b3 = getbyte();
	printf("%02x%02x%02x%02x\n",b3,b2,b1,b0);
}




void pnt_longword()	/* print out a longword */
{
	int	b0,b1,b2,b3;

	b0 = getbyte();
	b1 = getbyte();
	b2 = getbyte();
	b3 = getbyte();
	printf("%02x%02x%02x%02x",b3,b2,b1,b0);
}




int getbyte()	/* return one byte from stdin, keeping a running count */
{
	++bytes_used;
	return(getchar());
}


skip_bytes(x)	/* skip over the given number of bytes */

	int	x;     /* number of bytes to skip */
{
	int	i;

	for ( i = 0; i < x; i++)
		getbyte();
}



void skip_to_end_of_rec()    /* Finish reading to the end of the current */
{			     /* record. THIS FUNCTION **MUST** BE CALLED */
	int	i,b;	     /* AT THE END OF PRINTING OR READING EVERY  */
			     /* RECORD					 */

	if (bytes_used > rec_length)
	{
	    printf("fatal error, read past end of a record\n");
	    exit(1);
	}

	b = (rec_length - bytes_used);
	if (b != ZERO)
	{
	    for (i = 0; i < b; i++)
		getbyte();
	    if (rec_length != bytes_used)
		{
		 printf("\nError reading record\n");
		 exit(1);
		}
	}
	rec_length = ZERO; /* clean up for the next record */
	bytes_used = ZERO;
}


void skip_record()	/* Skip an entire record */
{			/* This function should ONLY be called immediatly */
			/* after the records STATUS CODE has been checked */
	int	b;

	b = getbyte();
	rec_length = (getbyte() << 8 ) + b;
	skip_to_end_of_rec();
}


    void not_supported()	/* prints not supported stuff and cleans up */
{
	printf(" reporting not currently supported\n");
	getbyte();	/* trash the status code */
	skip_record();
}



	/*
	 *  The main() routine expects to read the very first
	 *  byte of the snapshot file or the very first byte of each
	 *  individual record of the file. This first byte identifies
	 *  the type of the record. Main() is thus very simple and serves
	 *  only to dispatch to the appropiate routine which does ALL the
	 *  work of parsing through and printing the record. It is the
	 *  responsibility of each print function to insure that when it
	 *  returns it has read/consumed up to and including the last byte 
	 *  of its record.
	 */


main()
{

	int	byte;

	bytes_used = ZERO;

	byte = getbyte();
	while ( byte != EOF)
	{
	switch (byte)
	{

	  case  MST  :	mst_print();
			break;

	  case  FBA  :  printf("FBA");
			not_supported();
			break;

	  case  FBM  :  printf("FBM");
			not_supported();
			break;

	  case  MCD  :  printf("MCD");
			not_supported();
			break;

	  case  IBD  :  printf("IBD");
			not_supported();
			break;

	  case  IDP  :  printf("IDP");
			not_supported();
			break;

	  case  ICA  :  printf("ICA");
			not_supported();
			break;

	  case  ICB  :  printf("ICB");
			not_supported();
			break;

	  case  CLK  :  printf("CLK");
			not_supported();
			break;

	  case  EDP  :  printf("EDP");
			not_supported();
			break;

	  case  EBE  :  printf("EBE");
			not_supported();
			break;

	  case  MCC  :  printf("MCC");
			not_supported();
			break;

	  case  MAP  :  printf("MAP");
			not_supported();
			break;

	  case  EBD  :  printf("EBD");
			not_supported();
			break;

	  case  EBC  :  printf("EBC");
			not_supported();
			break;

	  case  CSB  :  printf("CSB");
			not_supported();
			break;

	  case  CSA  :  printf("CSA");
			not_supported();
			break;

	  case  MTM  :  printf("MTM");
			not_supported();
			break;

	  case  CSL  :  csl_print();
			break;

	  case  EMM  :  emm_print();
			break;

	  case  IPR  :  ipr_print();
			break;

	  case  ESC  :  esc_print();
			break;

	  case  PAMM :  pamm_print();
			break;

	  case  ISP  :  isp_print();
			break;

	  case  SBI0 :  sbi_print(0);
			break;

	  case  SBI1 : sbi_print(1);
		       break;

	  default   :  printf("\n UNKNOWN RECORD TYPE\n");
		       getbyte();  /* consume the status code */
		       printf("skipping the record......");
		       skip_record();


       }
       byte = getbyte();
       }


}

