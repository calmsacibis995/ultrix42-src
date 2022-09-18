/*	@(#)fp_class.h	4.2	(ULTRIX)	9/4/90				      */
#include <ansi_compat.h>
#ifdef __mips
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * Constants returned by the floating point fp_class_[fd]() functions.
 */
#define	FP_SNAN		0
#define	FP_QNAN		1
#define	FP_POS_INF	2
#define	FP_NEG_INF	3
#define	FP_POS_NORM	4
#define	FP_NEG_NORM	5
#define	FP_POS_DENORM	6
#define	FP_NEG_DENORM	7
#define	FP_POS_ZERO	8
#define	FP_NEG_ZERO	9
#endif /* __mips */
