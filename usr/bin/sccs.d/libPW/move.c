#ifndef lint
static char *sccsid="@(#)move.c	4.1	(ULTRIX)	7/17/90";
#endif not lint
/*static char Sccsid[] = "@(#)move	3.1";*/
/*
	Copies `n' characters from string `a' to string `b'.

	Guy Harris suggests using memcpy(); sounds good to me.
	I haven't made the change, though, since I'm not trying to
	radically improve UNIX System V software but just provide it.
		- DAG
*/

/* struct { int *ip; }; /* DAG -- found a better way, see below */

void move(a,b,n)	/* DAG -- bug fix (was (char *)) */
char *a,*b;
unsigned n;
{
	register char *x, *y;
	register int m;

	/*
		Test for non-zero number of characters to move
	*/
	if (m=n) {
		x = a;
		y = b;
#ifdef pdp11	/* DAG -- conditionalize */
		/*
			Compare the low order bits of the two pointers
			If both are equal (both even or both odd) then
			move words instead of bytes
		*/
		if (((x^y)&(char *)1) == 0) {
			/*
				If both odd move one byte to make both even
			*/
			if (x&(char *)1) {
				*y++ = *x++;
				n--;
			}
			/*
				Add one to n/2 so that we can use --m instead
				of m--; --m is more efficient
			*/
			for (m=n/2+1; --m; )
				*((int *)y)++ = *((int *)x)++;	/* DAG cleanup */
			m = n&1;
		}
#endif
		/*
			Add one to m so that we can use --m instead of m--;
			--m is more eficient.
		*/
		for (++m; --m; ) *y++ = *x++;
	}
}
