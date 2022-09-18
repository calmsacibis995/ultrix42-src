/*	@(#)fixpoint.h	4.1	(ULTRIX)	7/2/90	*/
#ifndef _FIXPT_
#define _FIXPT_

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/*
 * Fix-point arithmetic package
 */

/*
 * Basic fix-point types
 */
/*
 * TODO: should probably move this over to types.h so that avenrun is
 * not defined in vm_sched.c
 */
typedef	int 		fix;
typedef	unsigned int	ufix;

/*
 * Number of fraction bits.
 */
#define FBITS		8

/*
 * Conversion to fix-point representation
 * works with int, float, double, char, ....
 */
#define	TO_FIX(x)	((fix)((x)*(1<<FBITS)))

/*
 * Conversion from fix-point to various integer datatypes
 */
#define	FIX_TO_SHORT(x)		((short)((x)>>FBITS))
#define	FIX_TO_INT(x)		((int)((x)>>FBITS))

/*
 * Conversion from fix-point to double
 */
#define	FIX_TO_DBL(x)	(((double)(x))/(1<<FBITS))

/*
 * Multiplication/division of 2 fix-point values
 */
#define	MUL_2FIX(x, y)	(((x)*(y))>>FBITS)
#define	DIV_2FIX(x, y)	(((x)<<FBITS)/(y))
#endif /* _FIXPT_ */
