/* @(#)atol.c	4.1 (Berkeley) 12/21/80 */
#include <limits.h>
#include <errno.h>
long
atol(p)
register char *p;
{
        long maxover = LONG_MAX/10;  /* check for overflow */
        long maxunder = LONG_MIN/10; /* check for underflow */
        int errflg = 0;
	long n;
	register int f;

	n = 0;
	f = 0;
	for(;;p++) {
		switch(*p) {
		case ' ':
		case '\t':
			continue;
		case '-':
			f++;
		case '+':
			p++;
		}
		break;
	}
	while(*p >= '0' && *p <= '9')
        {
                if(n > maxover || n < maxunder)
                        errflg++;
                n = n*10;
                if(f)
                {
                        if(LONG_MIN + n > -(*p - '0'))
                                errflg++;
                }
                else
                        if(LONG_MAX - n < *p - '0')
                                errflg++;
                n += *p++ - '0';
        }
        if(errflg)
        {
                errno = ERANGE;
                return(f ? LONG_MIN : LONG_MAX);
        }
        else
                return(f? -n: n);

}
