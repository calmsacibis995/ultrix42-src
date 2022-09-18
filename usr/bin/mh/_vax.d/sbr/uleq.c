/* uleq.c - "unsigned" lexical compare */

#define TO_LOWER 040
#define NO_MASK  000
#include <ctype.h>

uleq (c1, c2)
register char  *c1,
               *c2;
{
    register int    c,
		    mask;

    if (!c1)
	c1 = "";
    if (!c2)
	c2 = "";

    while (c = *c1++)
    {
	mask = (isalpha(c) && isalpha(*c2)) ?  TO_LOWER : NO_MASK;
	if ((c | mask) != (*c2 | mask))
	    return 0;
	else
	    c2++;
    }
    return (*c2 == 0);
}
