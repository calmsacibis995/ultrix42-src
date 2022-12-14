/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * SCCSID: @(#)md.c	4.1	ULTRIX	1/25/91
 * Based on:
static char rcsid[] =
    "@(#) $Header: md-vax.c,v 1.3 90/10/03 14:14:33 mccanne Locked $ (LBL)";
static char rcsid[] =
    "@(#) $Header: md-mips.c,v 1.2 90/09/21 02:20:11 mccanne Exp $ (LBL)";
 */

#ifdef vax
/* Vaxen appear to have clocks accurate to 1 us,
   but packetfilter is timestamping to 10 ms. */

int
clock_sigfigs()
{
	return 2;
}
#endif


#ifdef mips
/*
 * On DEC RISC machines, timestamps to about 4 msec so we pretend
 * it is 1 msec.  Future kernel hacking could improve it.
 */

int
clock_sigfigs()
{
	return 3;
}
#endif

