/* uprf.c - "unsigned" lexical prefix  */

#define TO_LOWER 040
#define NO_MASK  000
#include <ctype.h>

uprf (c1, c2)
register char  *c1,
               *c2;
{
    register int    c,
		    mask;

    if (c1 == 0 || c2 == 0)
	return(0);         /* XXX */

    while (c = *c2++)
    {
	mask = (isalpha(c) && isalpha(*c1)) ?  TO_LOWER : NO_MASK;
	if ((c | mask) != (*c1 | mask))
	    return 0;
	else
	    c1++;
    }
    return 1;
}
