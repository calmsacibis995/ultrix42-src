/*	@(#)rand.c	4.1	ULTRIX	7/3/90	*/
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/

static long randx=1;

void
srand(x)
unsigned x;
{
	randx = x;
}

int
rand()
{
	return(((randx = randx * 1103515245L + 12345)>>16) & 0x7fff);
}
