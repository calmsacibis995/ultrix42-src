#ifndef lint
static char *sccsid = "@(#)interlock.c	4.4	(ULTRIX)	11/9/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,85,86,88,89 by		*
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

/************************************************************************
 *
 * Modification history				
 *
 * 09-Nov-90 -- burns
 *	Yet a further bugfix for atomic op, the vslock size arg
 *	was too big, causing locking of multiple (possibly) non-
 *	existant pages.
 *
 * 09-Oct-90 -- burns
 *	Bug fix for user locking via atomic op. This ensures that
 * 	the page containing the lock is locked into memory to avoid
 *	bad translation by uvtophy.
 *
 * 11-Dec-89 -- jas
 *      Modified interlock_read() and unlock_write() to work with
 *      kuseg space addresses.
 *
 * 26-Oct-89 -- gmm
 *	Moved bbcci() and bbssi() to kn5800.c
 *
 * 03-Oct-89 -- Joe Szczypek
 *	Added new routine bbcci() and bbssi()
 *
 ************************************************************************/

#include "../h/types.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../machine/cpu.h"
#include "../machine/pte.h"
#include "../h/vmmac.h"
#include "../machine/kn5800.h"
#include "../io/scs/scamachmac.h"

#define SOFT_LOCK_BIT	0x1

struct _gvpbq {
	u_long flink;
	u_long blink;
};


volatile int	interlock_sync;
volatile int	*interlock_address = IAR_REGISTER;
volatile int	*interlock_data = IDR_REGISTER;
extern int	*kn5800_wbflush_addr;
extern	struct v5800csr *v5800csr;

interlock_read(addr)
int addr;
{
	register int data, address;

	if (IS_KSEG0(addr)) {
		address = K0_TO_PHYS(addr);
	} else if(IS_KSEG1(addr)) {
		address = K1_TO_PHYS(addr);
	} else if(IS_KSEG2(addr)) {
		address = svtophy(addr);
	} else if(IS_KUSEG(addr)) {
	        vslock(addr, sizeof(int));	/* make sure that the page is in and
					   locked */
		address = uvtophy(u.u_procp,addr);
	}
	*interlock_address = address;
	data = *interlock_data;
	interlock_sync = v5800csr->csr1;
	return (data);
}

unlock_write(addr,data)
int addr;
int data;
{
	register int address;

	if (IS_KSEG0(addr)) {
		address = K0_TO_PHYS(addr);
	} else if(IS_KSEG1(addr)) {
		address = K1_TO_PHYS(addr);
	} else if(IS_KSEG2(addr)) {
		address = svtophy(addr);
	} else if(IS_KUSEG(addr)) {
		address = uvtophy(u.u_procp,addr);
	}
	/*
	 * The write buffer flush is necessary as we can't allow
	 * an R3000 chip bug to cause a double write to memory for
	 * an unlock write cycle. To guarantee this does not occur
	 * the write buffer must be flushed.
	 */
	KN5800_WBFLUSH();
	*interlock_address = address;
	*interlock_data = data;
	if(IS_KUSEG(addr))
	        vsunlock(addr, sizeof(int), B_READ); /* Unlock the page now, B_READ
						 forces pte to be marked modified */
	return (0);
}


que_lock(qhdr)
register struct _gvpbq *qhdr;
{
	register u_long data;
	register u_long retval = 1; 	/* failed to get lock */

	/*
	 * Attempt to hardware interlock on the queue address.
	 */
	data = interlock_read(qhdr);
	/*
	 * Check the soft lock bit.
	 */
	if ((data & SOFT_LOCK_BIT) == 0) {
		retval = data;
		data |= SOFT_LOCK_BIT;
		unlock_write(qhdr, data);
	} else {
		unlock_write(qhdr, data);
	}
	return( retval );
}


que_unlock(qhdr, newdata)
register struct _gvpbq *qhdr;
register int newdata;
{
	register int data;

	/*
	 * Attempt to hardware interlock on the queue address.
	 */
	data = interlock_read(qhdr);
	/*
	 * Check the soft lock bit.
	 */
	if ((data & SOFT_LOCK_BIT) == 0) {
		panic("que_unlock: lock not held");
	}
	unlock_write(qhdr, (( newdata ) & ~SOFT_LOCK_BIT));
}

insqti(entry, qhdr, retry)
register struct _gvpbq *entry;
register struct _gvpbq *qhdr;
register u_long retry;
{
    register struct _gvpbq *tail;
    register u_long tmp1;
    register int s, i;
    register u_long rtn = 1; 		/* failure to get lock */

    do {
        s = splextreme();
        if ((tmp1 = que_lock( qhdr )) != 1) {	/* get que lock */
	    if (tmp1 == 0) {			/* is que empty? */
	        rtn = 0;
	    } else {
	        rtn = -1;
	    }
			/* Insert Element on tail of Que */
 	    tail = (struct _gvpbq *)((u_long)&qhdr->flink + (qhdr->blink));
            entry->flink = ((unsigned)&qhdr->flink - (unsigned)&entry->flink);
            entry->blink = ((unsigned)&tail->flink - (unsigned)&entry->flink);
	    if( qhdr->blink ) {
	        tail->flink = ((unsigned)&entry->flink - (unsigned)&tail->flink);
	    }
	    else {
	        tmp1 = ((unsigned)&entry->flink - (unsigned)&tail->flink);
	    }
            qhdr->blink = ((unsigned)&entry->flink - (unsigned)&qhdr->flink);

            que_unlock(qhdr, tmp1);		/* unlock soft lock */
            splx(s);
	    break;
        }
        splx(s);

	for (i = 0; i < 100 && (qhdr->flink & SOFT_LOCK_BIT); i++) 
			;
    } while( retry-- );

    return(rtn);
}

remqhi(qhdr, retry)
register struct _gvpbq *qhdr;
register u_long retry;
{
    register struct _gvpbq *next;
    register u_long tmp1;
    register int s, i;
    register u_long rtn = 1; 		/* failure to get lock */

    do {
        s = splextreme();
        if ((tmp1 = que_lock( qhdr )) != 1) {	/* get que lock */
	    if (tmp1 == 0) {			/* is que empty? */
	        rtn = 0;
	    }
	    else {	
			/* Remove Head Que Element */
                rtn = (u_long)&qhdr->flink + (tmp1 & ~SOFT_LOCK_BIT);
	        next = (struct _gvpbq *)(rtn + (*(u_long *)rtn));
	        next->blink = ((unsigned)&qhdr->flink - (unsigned)&next->flink);
	        tmp1 = ((unsigned)&next->flink - (unsigned)&qhdr->flink);
	    }
	    que_unlock(qhdr, tmp1);		/* unlock soft lock */
            splx(s);
	    break;
        }
        splx(s);

	for (i = 0; i < 100 && (qhdr->flink & SOFT_LOCK_BIT); i++) 
			;
    } while( retry-- );

    return(rtn);
}
