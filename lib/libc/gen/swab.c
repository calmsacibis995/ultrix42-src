/*	@(#)swab.c	4.1	ULTRIX	7/3/90	*/
/*	Based on:	*/
/*	swab.c	4.2	83/06/27	*/

/*
 * Swab bytes
 * Jeffrey Mogul, Stanford
 */

swab(from, to, n)
	register char *from, *to;
	register int n;
{
	register unsigned long temp;
	
	if (n<=0) return;
	n >>= 1; n++;
#define	STEP	temp = *from++,*to++ = *from++,*to++ = temp
	/* round to multiple of 8 */
	while ((--n) & 07)
		STEP;
	n >>= 3;
	while (--n >= 0) {
		STEP; STEP; STEP; STEP;
		STEP; STEP; STEP; STEP;
	}
}
