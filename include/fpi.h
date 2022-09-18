/*	@(#)fpi.h	4.2	(ULTRIX)	9/4/90				      */
#include <ansi_compat.h>
#ifdef __mips
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

#ifdef __LANGUAGE_C
/* The counters */
extern int fpi_counts[];

/* string names of the causes */
extern char *fpi_list[];

extern void fpi();
extern void printfpi_counts();
#endif /* __LANGUAGE_C */

/* causes of fp interrupts in the order they are counted */
#define	FPI_SRCSNAN	0	/* source signaling NaN */
#define	FPI_SRCQNAN	1	/* source quiet NaN */
#define FPI_SRCDENORM	2	/* source denormalized value */
#define	FPI_MOVEZERO	3	/* moving a zero value R2360 only */
#define	FPI_NEGZERO	4	/* negating a zero value R2360 only */
#define	FPI_UNIMP	5	/* implemented in software only (sqrt) */
#define	FPI_INVALID	6	/* invalid operation */
#define	FPI_DIVIDE0	7	/* divide by zero */
#define	FPI_OVERFLOW	8	/* destination overflow */
#define	FPI_UNDERFLOW	9	/* destination underflow */
#define	FPI_SIZE	10
#endif /* __mips */
