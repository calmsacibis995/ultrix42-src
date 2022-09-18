/* @(#)timeb.h	4.1  (ULTRIX)        7/2/90     */

/*
 * Structure returned by ftime system call
 */
struct timeb
{
	time_t	time;
	unsigned short millitm;
	short	timezone;
	short	dstflag;
};
