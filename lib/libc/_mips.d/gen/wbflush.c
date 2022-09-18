/*	@(#)wbflush.c	4.1	(ULTRIX)	7/3/90	*/
/*
 *    This routine does a series of writes to an array.  This should cause
 *    the write buffer to be flushed.  At some point in time we may want to
 *    make this a system call, but the problem is that by the time you
 *    do the context switch and get to call the wbflush() kernel routine
 *    the write buffer will already be flushed (because of other writes).
 */

#define   WBSIZE   16
int
wbflush()
{
        register int wbuf[WBSIZE];
	register int i;

	for( i = 0; i < WBSIZE; i++)
	  wbuf[i] = 0;
	return;
}
