/* This program is used during an Ultrix build to set the Ultrix release
 * date for LMF into login.c.
 * It simply writes the current time plus a fixed offset onto the
 * standard outut, as a decimal number of days from the standard epoch,
 * and a human-readable message containing the same information on the
 * standard error.
 *
 * 15-Jun-1989	Giles Atkinson
 *
 */

#define OFFSET 10*24*60*60		/* Offset is 10 days */

#include <stdio.h>
#include <sys/time.h>

main () {
	struct timeval t;
	long when;

	gettimeofday(&t, 0);
	when = t.tv_sec+OFFSET;
	printf("%d\n", when);
	fprintf(stderr,"Setting LMF release date for ULTRIX to %s\n",
		ctime(&when));
}
