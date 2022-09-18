
#ifndef lint
static	char	*sccsid = "@(#)emm.c	4.1	(ULTRIX)	7/2/90";
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
 *		EMM.C
 *
 *	SNAPSHOT EMM Display Routine
 */

#include <stdio.h>

#define ZERO 0
#define VALID_RECORD 1
#define TRUE 1
#define FALSE 0

extern bytes_used;
extern rec_length;


value_a()	/* convert and print 'value' for voltage regulators A - H */
{
	int	polarity;
	int	value;
	float	volts;

	value = getbyte();
	polarity = getbyte();  
	volts = (float)value * 0.0276;
	if ((polarity & 0x80) != 0)
	    volts = (volts * -1.0);
	printf("%5.3f\n",volts);
}


value_b()	/* convert and print ground current 'value' */
{
	int	polarity;
	int	value;
	float	amps;

	value = getbyte();
	polarity = getbyte();
	amps = (70.0 + (2.8 * (float)value));
	if ((polarity & 0x80) != 0 )
	   amps = (amps * -1.0);
	printf("%6.3f\n",amps);
}


value_c()	/* convert and print "negative" regulators L & K */
{
	int	polarity;
	int	value;
	float	volts;

	value = getbyte();
	polarity = getbyte();
	volts = (0.0691 * (float)value);
	if ((polarity & 0x80) != 0 )
	   volts = (volts * -1.0);
	printf("%5.3f\n",volts);
}




value_d()	/* convert and print "positive" regulators L & K */
{
	int	polarity;
	int	value;
	float	volts;

	value = getbyte();
	polarity = getbyte();
	volts = (0.0762 * (float)value);
	if ((polarity & 0x80) != 0 )
	   volts = (volts * -1.0);
	printf("%5.3f\n",volts);
}




value_e()	/* convert and print temperatures */
{
	int	polarity;
	int	value;
	float	temp;

	polarity = getbyte(); /* no polarity, just need to use up the byte */
	value = getbyte();
	temp = 16.0 + (0.0762 * (float)value);
	printf("%6.3f\n",temp);
}


void emm_print()
{
	int	b,skip;


	rec_length = ZERO;
	skip = FALSE;

	if (getbyte() != VALID_RECORD)
	   {
	    printf("EMM Registers Record is Invalid..... Skiping \n");
	    skip = TRUE;
	   }

	b = getbyte();
	rec_length = (getbyte() << 8) + b;

	if (skip == FALSE)
	{
	  skip_bytes(6);  /* skip over .CDF file name */

	  printf("\n\tEMM Registers \n\n");

	  printf("POWREG\t%02x\n",getbyte());

	  printf("MRGEN\t%02x\n",getbyte());

	  printf("MARHILO\t%02x\n",getbyte());

	  b = getbyte();
	  printf("MODOK\t%02x%02x\n",getbyte(),b);

	  printf("MISREG\t%02x\n",getbyte());

	  printf("SWREG\t%02x\n",getbyte());

	  printf("PROM Revision\t%02x\n",getbyte());


	  /* How to decode/convert the voltages, temps, etc
	   *
	   * +----+------------------+-----------------+
	   * | 15 | 14 - ignore - 08 | 07 - value - 00 |  <-- EMM word
	   * +----+------------------+-----------------+
	   *
	   * 1. Convert 'value' to floating point number
	   *
	   * 2. If bit 15 is set and this is not a "temperature",
	   *    negate the value obtained in step 1.
	   *
	   * 3. The rest depends on which thing:
	   *
	   *    a. If voltage regulators (A - H), multiply 'value' by .0276
	   *
	   *    b. If ground current; 70 + (2.8 * value)
	   *
	   *    c. For "negative" regulators (L & K ); .0691 * value
	   *
	   *    d. For "positive" regulators (L & K);   .0762 *value
	   *
	   *    e. For temperatures;   16.0 + (.333 * value)
	   */

	  printf("REGULATOR_A VOLTAGE\t");
	  value_a();

	  printf("REGULATOR_B VOLTAGE\t");
	  value_a();

	  printf("REGULATOR_C VOLTAGE\t");
	  value_a();

	  printf("REGULATOR_D VOLTAGE\t");
	  value_a();

	  printf("REGULATOR_E VOLTAGE\t");
	  value_a();

	  printf("REGULATOR_F VOLTAGE\t");
	  value_a();

	  printf("REGULATOR_H VOLTAGE\t");
	  value_a();

	  printf("GND CURRENT VALUE\t");
	  value_b();

	  printf("REGULATOR_L + VOLTAGE\t");
	  value_d();

	  printf("REGULATOR_L - VOLTAGE\t");
	  value_c();

	  printf("REGULATOR_K + VOLTAGE\t");
	  value_d();

	  printf("REGULATOR_K - VOLTAGE\t");
	  value_c();

	  printf("T1 TEMPERATURE VOLTAGE\t");
	  value_e();

	  printf("T2 TEMPERATURE VOLTAGE\t");
	  value_e();

	  printf("T3 TEMPERATURE VOLTAGE\t");
	  value_e();

	  printf("T4 TEMPERATURE VOLTAGE\t");
	  value_e();
	}

	skip_to_end_of_rec();

}
