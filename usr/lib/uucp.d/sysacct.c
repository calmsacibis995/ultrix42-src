#ifndef lint
static char sccsid[] = "@(#)sysacct.c	4.1 (decvax!larry) 7/2/90";
#endif

#include <sys/types.h>


/*******
 *	sysacct(bytes, time)	output accounting info
 *	time_t time;
 *	long bytes;
 */

sysacct(bytes, time)
time_t time;
long bytes;
{
	return;
}
