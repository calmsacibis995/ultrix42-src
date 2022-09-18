/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * cachectl.h -- defines for MIPS cache control system calls
 */

/*
 * Options for cacheflush system call
 */
#define	ICACHE	0x1		/* flush i cache */
#define	DCACHE	0x2		/* flush d cache */
#define	BCACHE	(ICACHE|DCACHE)	/* flush both caches */

/*
 * Options for cachectl system call
 */
#define	CACHEABLE	0	/* make page(s) cacheable */
#define	UNCACHEABLE	1	/* make page(s) uncacheable */
