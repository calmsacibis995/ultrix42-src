/*
 * @(#)ga_data.c	4.1	(ULTRIX)	7/2/90
*/

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
#define _GA_DATA_C_

#include "../h/param.h"
#include "../io/tc/gx.h"
#include "../io/tc/ga.h"

#include "ga.h"				/* for NGA */

#ifdef BINARY

#ifdef GA_ALLOC_TX
int ga_dummy();
#endif

#ifdef GA_ALLOC_DT
extern int Ring2da[];
#endif

/* XXX temp debugging garf ??? XXX */
int gaVintr_count[8] = {0, 0, 0, 0, 0 ,0 ,0, 0};
int gaSintr_count[8] = {0, 0, 0, 0, 0 ,0 ,0, 0};
int gaPintr_count[8] = {0, 0, 0, 0, 0 ,0 ,0, 0};
int gaEintr_count[8] = {0, 0, 0, 0, 0 ,0 ,0, 0};

ga_ComAreaPtr gaComArea;

#else binary

#ifdef GA_ALLOC_DT
/* size MUST match gx_priv_size in ga_cons_init, please... */
int	Ring2da[ (sizeof(gxPriv) + _128K + NBPG) / sizeof(int) ];
#endif

#ifdef GA_ALLOC_TX

#define _16W	i++;i++;i++;i++;i++;i++;i++;i++;i++;i++;i++;i++;i++;i++;i++;i++
#define _128W	_16W;_16W;_16W;_16W;_16W;_16W;_16W;_16W
#define _4KB	_128W;_128W;_128W;_128W;_128W;_128W;_128W;_128W
#define _20KB	_4KB;_4KB;_4KB;_4KB;_4KB
#define _32KB	_4KB;_4KB;_4KB;_4KB;_4KB;_4KB;_4KB;_4KB
#define _128KB	_32KB;_32KB;_32KB;_32KB

ga_dummy()
{
    register int i;

#   ifdef HWDEBUG
    _20KB;				/* sizeof(gxPriv) */
#   else
    _4KB;
#   endif

    _128KB;				/* + 128KB */
    _4KB;				/* + NBPG */
    _4KB;				/* + shm alignment */

#if NGA > 1
    _128KB;				/* + 128KB */
    _4KB;				/* + NBPG */
    _4KB;				/* + shm alignment */
#endif

#if NGA > 2
    _128KB;				/* + 128KB */
    _4KB;				/* + NBPG */
    _4KB;				/* + shm alignment */
#endif
}

#endif

#endif binary
