/* @(#)atoi.c	4.3 (Berkeley) 81/02/28 */
#include <limits.h>
#include <errno.h>
atoi(p)
register char *p;
{
	int maxover = INT_MAX/10; 	/* overflow check */
	int maxunder = INT_MIN/10;	/* underflow check*/
	int errflg = 0;
 	register int n;
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
                        if(INT_MIN + n > -(*p - '0'))
                                errflg++;
                }
                else
                        if(INT_MAX - n < *p - '0')
                                errflg++;

                n += *p++ - '0';
        }
        if(errflg)
        {
                errno = ERANGE;
                return(f ? INT_MIN : INT_MAX);
        }
        else
                return(f? -n: n);
}
