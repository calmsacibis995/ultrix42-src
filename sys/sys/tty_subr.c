
 
#ifndef lint
static char *sccsid = "@(#)tty_subr.c	4.2	(ULTRIX)	11/9/90";
#endif lint
 
/************************************************************************
 *									*
 *			Copyright (c) 1985, 1988 by			*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
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
 *			Modification History				*
 *									*
 *	Tim Burke    -  03/11/88					*
 *		In unputc(), change to the previous cblock if we've	*
 *		indexed 12 (not 4) into the present cblock.		*
 *	Andy Gadsby  -  07/24/87					*
 *		Moved delay bit to new field				*
 *		Removed unused cwaiting code				*
 *	Larry Cohen  -	09/16/85					*
 * 		include ioctl.h for winsiz structure definition		*
 *									*
 ************************************************************************/
 
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/clist.h"
#include "../h/smp_lock.h"

 
 
/*
 * Character list get/put
 */
getc(p)
	register struct clist *p;
{
	register struct cblock *bp;
	register int c, s;
	register int charnum;
 
	s = spltty();
	if (p->c_cc <= 0) {
		c = -1;
		p->c_cc = 0;
		p->c_cf = p->c_cl = NULL;
	} else {
		bp = (struct cblock *)((int)p->c_cf & ~CROUND);
		charnum = p->c_cf - (char *)bp - CINFO_OFFSET;
		c = *p->c_cf++ & CHAR_MASK;
		if (bp->c_delay[charnum / NBUNS] & (1 << (charnum % NBUNS)))
		    c |= DELAY_FLAG;
		if (--p->c_cc<=0) {
			p->c_cf = NULL;
			p->c_cl = NULL;
			smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
			bp->c_next = cfreelist.c_head;
			cfreelist.c_head = bp;
			cfreelist.c_count += CBSIZE;
			smp_unlock(&cfreelist.c_lk_free_clist);
		} else if (((int)p->c_cf & CROUND) == 0){
			p->c_cf = bp->c_next->c_info;
			smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
			bp->c_next = cfreelist.c_head;
			cfreelist.c_head = bp;
			cfreelist.c_count += CBSIZE;
			smp_unlock(&cfreelist.c_lk_free_clist);

		}
	}
	splx(s);
	return(c);
}
 
/*
 * copy clist to buffer.
 * return number of bytes moved.
 *
 * It is the callers responsibility to only move upto a delay byte
 * since the delay attribute will not be preserved on this call.
 */
q_to_b(q, cp, cc)
	register struct clist *q;
	register char *cp;
{
	register struct cblock *bp;
	register int s;
	char *acp;
 
	if (cc <= 0)
		return(0);
	s = spltty();
	if (q->c_cc <= 0) {
		q->c_cc = 0;
		q->c_cf = q->c_cl = NULL;
		splx(s);
		return(0);
	}
	acp = cp;
	cc++;
 
	while (--cc) {
		*cp++ = *q->c_cf++;
		if (--q->c_cc <= 0) {
			bp = (struct cblock *)(q->c_cf-1);
			bp = (struct cblock *)((int)bp & ~CROUND);
			q->c_cf = q->c_cl = NULL;
			smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
			bp->c_next = cfreelist.c_head;
			cfreelist.c_head = bp;
			cfreelist.c_count += CBSIZE;
			smp_unlock(&cfreelist.c_lk_free_clist);
			break;
		}
		if (((int)q->c_cf & CROUND) == 0) {
			bp = (struct cblock *)(q->c_cf);
			bp--;
			q->c_cf = bp->c_next->c_info;
			smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
			bp->c_next = cfreelist.c_head;
			cfreelist.c_head = bp;
			cfreelist.c_count += CBSIZE;
			smp_unlock(&cfreelist.c_lk_free_clist);
		}
	}
	splx(s);
	return(cp-acp);
}
 
/*
 * Return count of contiguous characters
 * in clist starting at q->c_cf.
 * If flag is set non-zero then this indicates that we should
 * return number of characters upto the delay byte.
 * NOTE: The bit testing here needs improving before production
 */
ndqb(q, flag)
	register struct clist *q;
{
	register int cc;
	int s;
 
	s = spltty();
	if (q->c_cc <= 0) {
		cc = -q->c_cc;
		goto out;
	}
	cc = ((int)q->c_cf + CBSIZE) & ~CROUND;
	cc -= (int)q->c_cf;
	if (q->c_cc < cc)
		cc = q->c_cc;
	if (flag) {
		register char *p, *end;
		register struct cblock *bp;
		register int charnum;
 
		bp = (struct cblock *)((int)q->c_cf & ~CROUND);
		charnum = q->c_cf - (char *)bp - CINFO_OFFSET;
 
		p = q->c_cf;
		end = p;
		end += cc;
		while (p < end) {
			if (bp->c_delay[charnum / NBUNS] & (1 << (charnum % NBUNS))) {
				cc = (int)p;
				cc -= (int)q->c_cf;
				break;
			}
			charnum++;
			p++;
		}
	}
out:
	splx(s);
	return(cc);
}
 
 
 
/*
 * Flush cc bytes from q.
 */
ndflush(q, cc)
	register struct clist *q;
	register int cc;
{
	register struct cblock *bp;
	char *end;
	int rem, s;
 
	s = spltty();
	if (q->c_cc <= 0) {
		goto out;
	}
	while (cc>0 && q->c_cc) {
		bp = (struct cblock *)((int)q->c_cf & ~CROUND);
		if ((int)bp == (((int)q->c_cl-1) & ~CROUND)) {
			end = q->c_cl;
		} else {
			end = (char *)((int)bp + sizeof (struct cblock));
		}
		rem = end - q->c_cf;
		if (cc >= rem) {
			cc -= rem;
			q->c_cc -= rem;
			q->c_cf = bp->c_next->c_info;
			smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
			bp->c_next = cfreelist.c_head;
			cfreelist.c_head = bp;
			cfreelist.c_count += CBSIZE;
			smp_unlock(&cfreelist.c_lk_free_clist);
		} else {
			q->c_cc -= cc;
			q->c_cf += cc;
			if (q->c_cc <= 0) {
				smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
				bp->c_next = cfreelist.c_head;
				cfreelist.c_head = bp;
				cfreelist.c_count += CBSIZE;
				smp_unlock(&cfreelist.c_lk_free_clist);
			}
			break;
		}
	}
	if (q->c_cc <= 0) {
		q->c_cf = q->c_cl = NULL;
		q->c_cc = 0;
	}
out:
	splx(s);
}
 
 
putc(c, p)
	register struct clist *p;
{
	register struct cblock *bp;
	register char *cp;
	register int s;
	int charnum;
 
	s = spltty();
	if ((cp = p->c_cl) == NULL || p->c_cc < 0 ) {
		smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
		if ((bp = cfreelist.c_head) == NULL) {
			smp_unlock(&cfreelist.c_lk_free_clist);
			splx(s);
			return(-1);
		}
		cfreelist.c_head = bp->c_next;
		cfreelist.c_count -= CBSIZE;
		smp_unlock(&cfreelist.c_lk_free_clist);
		bp->c_next = NULL;
		p->c_cf = cp = bp->c_info;
	} else if (((int)cp & CROUND) == 0) {
		bp = (struct cblock *)cp - 1;
		smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
		if ((bp->c_next = cfreelist.c_head) == NULL) {
			smp_unlock(&cfreelist.c_lk_free_clist);
			splx(s);
			return(-1);
		}
		bp = bp->c_next;
		cfreelist.c_head = bp->c_next;
		cfreelist.c_count -= CBSIZE;
		smp_unlock(&cfreelist.c_lk_free_clist);
		bp->c_next = NULL;
		cp = bp->c_info;
	}
	bp = (struct cblock *)((int)cp & ~CROUND);
	charnum = cp - (char *)bp - CINFO_OFFSET;
	if (c & DELAY_FLAG)
		bp->c_delay[charnum / NBUNS] |=  1 << (charnum % NBUNS);
	else
		bp->c_delay[charnum / NBUNS] &=  ~(1 << (charnum % NBUNS));
			
	*cp++ = c & CHAR_MASK;
 
	p->c_cc++;
	p->c_cl = cp;
	splx(s);
	return(0);
}
 
 
 
/*
 * copy buffer to clist.
 * return number of bytes not transfered.
 */
b_to_q(cp, cc, q)
	register char *cp;
	struct clist *q;
	register int cc;
{
	register char *cq;
	register struct cblock *bp;
	register int s, acc;
	register int charnum;
 
	if (cc <= 0)
		return(0);
	acc = cc;
 
 
	s = spltty();
	if ((cq = q->c_cl) == NULL || q->c_cc < 0) {
		smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
		if ((bp = cfreelist.c_head) == NULL) {
			smp_unlock(&cfreelist.c_lk_free_clist);
			goto out;
		}
		cfreelist.c_head = bp->c_next;
		cfreelist.c_count -= CBSIZE;
		smp_unlock(&cfreelist.c_lk_free_clist);
		bp->c_next = NULL;
		q->c_cf = cq = bp->c_info;
	} else 
		bp = (struct cblock *)((int)cq & ~CROUND);
	charnum = cq - (char *)bp - CINFO_OFFSET;
 
	while (cc) {
		if (((int)cq & CROUND) == 0) {
			bp = (struct cblock *) cq - 1;
			smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
			if ((bp->c_next = cfreelist.c_head) == NULL) {
				smp_unlock(&cfreelist.c_lk_free_clist);
				goto out;
			}
			bp = bp->c_next;
			cfreelist.c_head = bp->c_next;
			cfreelist.c_count -= CBSIZE;
			smp_unlock(&cfreelist.c_lk_free_clist);
			bp->c_next = NULL;
			cq = bp->c_info;
			charnum = 0;
		}
		bp->c_delay[charnum / NBUNS] &= ~(1 << (charnum % NBUNS));
		charnum++;
		*cq++ = *cp++;
		cc--;
	}
out:
	q->c_cl = cq;
	q->c_cc += acc-cc;
	splx(s);
	return(cc);
}
 
/*
 * Given a non-NULL pointter into the list (like c_cf which
 * always points to a real character if non-NULL) return the pointer
 * to the next character in the list or return NULL if no more chars.
 *
 * Callers must not allow getc's to happen between nextc's so that the
 * pointer becomes invalid.  Note that interrupts are NOT masked.
 *
 * Returns the character pointed to via char_ptr, provided there are
 * charaters in the list
 */
char *
getnextc(p, cp, char_ptr)
	register struct clist *p;
	register char *cp;
	register int *char_ptr;
{	
	struct cblock *bp;
	int c;
	int charnum;
 
	if (p->c_cc && ++cp != p->c_cl) {
		if (((int)cp & CROUND) == 0)
			cp = ((struct cblock *)cp)[-1].c_next->c_info;
 
		bp = (struct cblock *)((int)cp & ~CROUND);
		charnum = cp - (char *)bp - CINFO_OFFSET;
		c = *cp & CHAR_MASK;
		if (bp->c_delay[charnum / NBUNS] & (1 << (charnum % NBUNS)))
		    c |= DELAY_FLAG;
		*char_ptr = c;
		return (cp);
	}
	return (0);
}
 
/*
 * Remove the last character in the list and return it.
 */
unputc(p)
	register struct clist *p;
{
	register struct cblock *bp;
	register int c, s;
	struct cblock *obp;
	int charnum;
 
	s = spltty();
	if (p->c_cc <= 0)
		c = -1;
	else {
		c = *--p->c_cl & CHAR_MASK;
		bp = (struct cblock *)((int)p->c_cl & ~CROUND);
		charnum = p->c_cl - (char *)bp - CINFO_OFFSET;
 
		if (bp->c_delay[charnum / NBUNS] & (1 << (charnum % NBUNS)))
		    c |= DELAY_FLAG;
 
		if (--p->c_cc <= 0) {
			p->c_cl = p->c_cf = NULL;
			smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
			bp->c_next = cfreelist.c_head;
			cfreelist.c_head = bp;
			cfreelist.c_count += CBSIZE;
			smp_unlock(&cfreelist.c_lk_free_clist);
		} else if (((int)p->c_cl & CROUND) == CINFO_OFFSET) {
			p->c_cl = (char *)((int)p->c_cl & ~CROUND);
			bp = (struct cblock *)p->c_cf;
			bp = (struct cblock *)((int)bp & ~CROUND);
			while (bp->c_next != (struct cblock *)p->c_cl)
				bp = bp->c_next;
			obp = bp;
			p->c_cl = (char *)(bp + 1);
			bp = bp->c_next;
			smp_lock(&cfreelist.c_lk_free_clist, LK_RETRY);
			bp->c_next = cfreelist.c_head;
			cfreelist.c_head = bp;
			cfreelist.c_count += CBSIZE;
			smp_unlock(&cfreelist.c_lk_free_clist);
			obp->c_next = NULL;
		}
	}
	splx(s);
	return (c);
} 
 
/*
 * Put the chars in the from que
 * on the end of the to que.
 *
 * should just use q_to_b and then b_to_q here but we must preserve delays.
 */
catq(from, to)
	struct clist *from, *to;
{
	register int c;
 
	while ((c = getc(from)) >= 0)
		(void) putc(c, to);
}
 
/*
 * using clists
 * Integer (short) get/put
 * using clists
 */
typedef	short word_t;
union chword {
	word_t	word;
	struct {
		char	Ch[sizeof (word_t)];
	} Cha;
#define	ch	Cha.Ch
};
 
getw(p)
	register struct clist *p;
{
	register int i;
	union chword x;
 
	if (p->c_cc < sizeof (word_t))
		return (-1);
	for (i = 0; i < sizeof (word_t); i++)
		x.ch[i] = getc(p);
	return (x.word);
}
 
putw(c, p)
	register struct clist *p;
{
	register int s;
	register int i;
	union chword x;
 
	s = spltty();
	if (cfreelist.c_head==NULL) {
		splx(s);
		return(-1);
	}
	x.word = c;
	for (i = 0; i < sizeof (word_t); i++)
		(void) putc(x.ch[i], p);
	splx(s);
	return (0);
}
