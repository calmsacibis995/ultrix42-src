#ifndef lint
static	char	*sccsid = "@(#)subr_rmap.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1988 by			*
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
/*
 *
 *   Modification History:
 *
 * 11 Dec 89 jaa
 *	removed check for swap device in rmalloc()/rmfree()
 *	we make sure we have a swap device in main()
 *
 *  12-Jun-89 -- gg
 *	Replaced variable dmmax with swapfrag. Also added a check in
 *	rmalloc() and rmfree() for the presence of swap device before
 *	doing swap allocation/freeing.
 *	
 *  14-Mar-88 -- depp 
 *      Added comment to rminit about resource sizing.  No code change.
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/kernel.h"
#include "../h/smp_lock.h"

extern int swapfrag;	/* swap fragment size for text, data,stack */

/*
 * locks for manipulating resource maps
 */
struct lock_t lk_rmap;

/*
 * Resource map handling routines.
 *
 * A resource map is an array of structures each
 * of which describes a segment of the address space of an available
 * resource.  The segments are described by their base address and
 * length, and sorted in address order.  Each resource map has a fixed
 * maximum number of segments allowed.  Resources are allocated
 * by taking part or all of one of the segments of the map.
 *
 * Returning of resources will require another segment if
 * the returned resources are not adjacent in the address
 * space to an existing segment.  If the return of a segment
 * would require a slot which is not available, then one of
 * the resource map segments is discarded after a warning is printed.
 * Returning of resources may also cause the map to collapse
 * by coalescing two existing segments and the returned space
 * into a single segment.  In this case the resource map is
 * made smaller by copying together to fill the resultant gap.
 *
 * N.B.: the current implementation uses a dense array and does
 * not admit the value ``0'' as a legal address, since that is used
 * as a delimiter.
 *
 *             IMPORTANT NOTE:
 *
 * Since address == 0 is illegal and since all invocations of this routine 
 * start with either 1 or 2, the routines using rminit, rmalloc, rmfree must
 * account for the fact that this routine DOES NOT deduct the starting address
 * from the map.  This means that if the calling routines do not account for 
 * this, it's possible that rmalloc() could allocate past the end of a given
 * resource.
 *
 * The calling routines have basically two options, either:
 *
 *      1. To insure upon calling rminit, that the parameter "size" has already
 *         been reduced by the parameter "addr" (as in kernelmap) or
 *
 *      2. Decrementing the address returned by rmalloc() by the rminit "addr"
 *         parameter, prior to use and incrementing the "addr" parameter by the
 *         rminit "addr" parameter prior to calling rmfree() (as in msg queues)
 */

/*
 * rm_lock_init initializes smp locking for resource maps.
 */
rm_lock_init()
{
	lockinit(&lk_rmap, &lock_rmap_d);
}


/*
 * Initialize map mp to have (mapsize-2) segments
 * and to be called ``name'', which we print if
 * the slots become so fragmented that we lose space.
 * The map itself is initialized with size elements free
 * starting at addr.
 */
rminit(mp, size, addr, name, mapsize)
	register struct map *mp;
	long size, addr;
	char *name;
	int mapsize;
{
	register struct mapent *ep;
	int s = splclock();

	smp_lock(&lk_rmap, LK_RETRY);
	ep = (struct mapent *)(mp+1);
	mp->m_name = name;
/* N.B.: WE ASSUME HERE THAT sizeof (struct map) == sizeof (struct mapent) */
	/*
	 * One of the mapsize slots is taken by the map structure,
	 * segments has size 0 and addr 0, and acts as a delimiter.
	 * We insure that we never use segments past the end of
	 * the array which is given by mp->m_limit.
	 * Instead, when excess segments occur we discard some resources.
	 */
	mp->m_limit = (struct mapent *)&mp[mapsize];
	/*
	 * Simulate a rmfree(), but with the option to
	 * call with size 0 and addr 0 when we just want
	 * to initialize without freeing.
	 */
	ep->m_size = size;
	ep->m_addr = addr;
	smp_unlock(&lk_rmap);
	splx(s);
}

/*
 * Allocate 'size' units from the given
 * map. Return the base of the allocated space.
 * In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 *
 * Algorithm is first-fit.
 *
 * This routine knows about the interleaving of the swapmap
 * and handles that.
 */
long
rmalloc(mp, size)
	register struct map *mp;
	long size;
{
	register int addr;
	register struct mapent *bp;
	swblk_t first, rest;
	int s ; 

	if (size <= 0 || mp == swapmap && size > swapfrag)
		panic("rmalloc");
	/*
	 * Search for a piece of the resource map which has enough
	 * free space to accomodate the request.
	 */
	s = splclock();
	smp_lock(&lk_rmap, LK_RETRY);
	for (bp = (struct mapent *)(mp+1); bp->m_size; bp++) {
		if (bp->m_size >= size) {
			/*
			 * If allocating from swapmap,
			 * then have to respect interleaving
			 * boundaries.
			 */
			if (mp == swapmap && nswdev > 1 &&
			    (first = swapfrag - bp->m_addr%swapfrag) < bp->m_size) {
				if (bp->m_size - first < size)
					continue;
				addr = bp->m_addr + first;
				rest = bp->m_size - first - size;
				bp->m_size = first;
				smp_unlock(&lk_rmap);
				splx(s);
				if (rest)
					rmfree(swapmap, rest, addr+size);
				return (addr);
			}
			/*
			 * Allocate from the map.
			 * If there is no space left of the piece
			 * we allocated from, move the rest of
			 * the pieces to the left.
			 */
			addr = bp->m_addr;
			bp->m_addr += size;
			if ((bp->m_size -= size) == 0) {
				do {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
				} while ((bp-1)->m_size = bp->m_size);
			}
			if (mp == swapmap && addr % CLSIZE)
				panic("rmalloc swapmap");
			smp_unlock(&lk_rmap);
			splx(s);
			return (addr);
		}
	}
	smp_unlock(&lk_rmap);
	splx(s);
	return (0);
}

/*
 * Free the previously allocated space at addr
 * of size units into the specified map.
 * Sort addr into map and combine on
 * one or both ends if possible.
 */
rmfree(mp, size, addr)
	struct map *mp;
	long size, addr;
{
	struct mapent *firstbp;
	register struct mapent *bp;
	register int t;
	int s;

	/*
	 * Both address and size must be
	 * positive, or the protocol has broken down.
	 */
	if (addr <= 0 || size <= 0)
		goto badrmfree;
	/*
	 * Locate the piece of the map which starts after the
	 * returned space (or the end of the map).
	 */
	s = splclock();
	smp_lock(&lk_rmap, LK_RETRY);
	firstbp = bp = (struct mapent *)(mp + 1);
	for (; bp->m_addr <= addr && bp->m_size != 0; bp++)
		continue;
	/*
	 * If the piece on the left abuts us,
	 * then we should combine with it.
	 */
	if (bp > firstbp && (bp-1)->m_addr+(bp-1)->m_size >= addr) {
		/*
		 * Check no overlap (internal error).
		 */
		if ((bp-1)->m_addr+(bp-1)->m_size > addr)
			goto badrmfree;
		/*
		 * Add into piece on the left by increasing its size.
		 */
		(bp-1)->m_size += size;
		/*
		 * If the combined piece abuts the piece on
		 * the right now, compress it in also,
		 * by shifting the remaining pieces of the map over.
		 */
		if (bp->m_addr && addr+size >= bp->m_addr) {
			if (addr+size > bp->m_addr)
				goto badrmfree;
			(bp-1)->m_size += bp->m_size;
			while (bp->m_size) {
				bp++;
				(bp-1)->m_addr = bp->m_addr;
				(bp-1)->m_size = bp->m_size;
			}
		}
		goto done;
	}
	/*
	 * Don't abut on the left, check for abutting on
	 * the right.
	 */
	if (addr+size >= bp->m_addr && bp->m_size) {
		if (addr+size > bp->m_addr)
			goto badrmfree;
		bp->m_addr -= size;
		bp->m_size += size;
		goto done;
	}
	/*
	 * Don't abut at all.  Make a new entry
	 * and check for map overflow.
	 */
	do {
		t = bp->m_addr;
		bp->m_addr = addr;
		addr = t;
		t = bp->m_size;
		bp->m_size = size;
		bp++;
	} while (size = t);
	/*
	 * Segment at bp is to be the delimiter;
	 * if there is not room for it 
	 * then the table is too full
	 * and we must discard something.
	 */
	if (bp+1 > mp->m_limit) {
		/*
		 * Back bp up to last available segment.
		 * which contains a segment already and must
		 * be made into the delimiter.
		 * Discard second to last entry,
		 * since it is presumably smaller than the last
		 * and move the last entry back one.
		 */
		bp--;
		printf("%s: rmap ovflo, lost [%d,%d)\n", mp->m_name,
		    (bp-1)->m_addr, (bp-1)->m_addr+(bp-1)->m_size);
		bp[-1] = bp[0];
		bp[0].m_size = bp[0].m_addr = 0;
	}
done:
	/*
	 * THIS IS RIDICULOUS... IT DOESN'T BELONG HERE!
	 */
	smp_unlock(&lk_rmap);
	splx(s);
	if ((mp == kernelmap) && kmapwnt) {
		kmapwnt = 0;
		wakeup((caddr_t)kernelmap);
	}
	return;
badrmfree:
	panic("bad rmfree");
}

/*
 * Allocate 'size' units from the given map, starting at address 'addr'.
 * Return 'addr' if successful, 0 if not.
 * This may cause the creation or destruction of a resource map segment.
 *
 * This routine will return failure status if there is not enough room
 * for a required additional map segment.
 *
 * An attempt to use this on 'swapmap' will result in
 * a failure return.  This is due mainly to laziness and could be fixed
 * to do the right thing, although it probably will never be used.
 */
rmget(mp, size, addr)
	register struct map *mp;
{
	register struct mapent *bp, *bp2;
	int s;

	if (size <= 0)
		panic("rmget");
	if (mp == swapmap)
		return (0);

	/*
	 * Look for a map segment containing the requested address.
	 * If none found, return failure.
	 */
	s = splclock();
	smp_lock(&lk_rmap, LK_RETRY);
	for (bp = (struct mapent *)(mp+1); bp->m_size; bp++)
		if (bp->m_addr <= addr && bp->m_addr + bp->m_size > addr)
			break;
	if (bp->m_size == 0) {
		smp_unlock(&lk_rmap);
		splx(s);
		return (0);
	      }

	/*
	 * If segment is too small, return failure.
	 * If big enough, allocate the block, compressing or expanding
	 * the map as necessary.
	 */
	if (bp->m_addr + bp->m_size < addr + size) {
		smp_unlock(&lk_rmap);
		splx(s);
		return (0);
	      }
	if (bp->m_addr == addr)
		if (bp->m_addr + bp->m_size == addr + size) {
			/*
			 * Allocate entire segment and compress map
			 */
			bp2 = bp;
			while (bp2->m_size) {
				bp2++;
				(bp2-1)->m_addr = bp2->m_addr;
				(bp2-1)->m_size = bp2->m_size;
			}
		} else {
			/*
			 * Allocate first part of segment
			 */
			bp->m_addr += size;
			bp->m_size -= size;
		}
	else
		if (bp->m_addr + bp->m_size == addr + size) {
			/*
			 * Allocate last part of segment
			 */
			bp->m_size -= size;
		} else {
			/*
			 * Allocate from middle of segment, but only
			 * if table can be expanded.
			 */
			for (bp2=bp; bp2->m_size; bp2++)
				;
			if (bp2 == mp->m_limit) {
				smp_unlock(&lk_rmap);
				splx(s);
				return (0);
			      }
			while (bp2 > bp) {
				(bp2+1)->m_addr = bp2->m_addr;
				(bp2+1)->m_size = bp2->m_size;
				bp2--;
			}
			(bp+1)->m_addr = addr + size;
			(bp+1)->m_size =
			    bp->m_addr + bp->m_size - (addr + size);
			bp->m_size = addr - bp->m_addr;
		}
	smp_unlock(&lk_rmap);
	splx(s);
	return (addr);
}
