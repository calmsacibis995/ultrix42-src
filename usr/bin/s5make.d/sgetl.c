#ifndef lint
static	char	*sccsid = "@(#)sgetl.c	4.1	(ULTRIX)	8/17/88";
#endif lint
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: sgetl.c,v 1.1 86/07/02 11:08:41 dce Exp $ */
/* This is taken from System 5 source from /src/cmd/sgs/libld/common/sgetl.c */
/*
 * The intent here is to provide a means to make the value of
 * bytes in an io-buffer correspond to the value of a long
 * in the memory while doing the io a `long' at a time.
 * Files written and read in this way are machine-independent.
 *
 */
#include "values.h"

long
sgetl(buffer)
register char *buffer;
{
	register long w = 0;
	register int i = BITSPERBYTE * sizeof(long);

	while ((i -= BITSPERBYTE) >= 0)
		w |= (long) ((unsigned char) *buffer++) << i;
	return (w);
}
