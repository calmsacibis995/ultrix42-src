#ifndef lint
static	char	*sccsid = "@(#)pseudo_data.c	4.2		(ULTRIX)		2/1/91";
#endif lint
/*
 * This routine is called during system initialization immediately after
 * loattach() is called (in init_main.c).  It in turn calls attach routines
 * for the configured pseudo network interfaces.
 *
 * To add a new pseudo interface add an include line for the header file
 * which defines the number of such interfaces (for example, sl.h for if_sl.c)
 * and add conditionaly compiled code to call the new interface's attach
 * routine.
 */

void
pseudo_attach()

{

#include "sl.h"

#if NSL > 0

slattach();

#endif /* NSL > 0 */

return;
}

