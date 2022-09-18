/*
 *	@(#)mem.c	4.1	(ULTRIX)	7/2/90
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 * Revision History:
 *
 *	25-May-1989	Kong
 *	Added driver for /dev/kUmem (minor device number == 3).
 */

/*
 * Memory special file
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/uio.h"

#include "../machine/cpu.h"
#include "../machine/common/cpuconf.h"

mmread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (mmrw(dev, uio, UIO_READ));
}

mmwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (mmrw(dev, uio, UIO_WRITE));
}

mmrw(dev, uio, rw)
	dev_t dev;
	struct uio *uio;
	enum uio_rw rw;
{
	register int o;
	register u_int c, v;
	register struct iovec *iov;
	int error = 0;

	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {

		/*
		 * minor device 0 is physical memory
		 */
		case 0:
			c = (u_int)iov->iov_len;

			v = (u_int)uio->uio_offset + c;
			/*
			 * check for wrap around
			 */
			if (v < (u_int)uio->uio_offset)
				goto fault;
			/*
			 * make sure endpoint is legal
			 */
			if (btop(v) >= physmem || PHYS_TO_K0(v) >= K1BASE)
				goto fault;

			error = uiomove(PHYS_TO_K0(uio->uio_offset),c,rw,uio);
			continue;

		/*
		 * minor device 1 is kernel memory
		 */
		case 1:
			c = iov->iov_len;
			if (!kernacc((caddr_t)uio->uio_offset, c,
			    rw == UIO_READ ? B_READ : B_WRITE))
				goto fault;
			error = uiomove(uio->uio_offset, c, rw, uio);
			continue;

		/*
		 * minor device 2 is EOF/RATHOLE
		 */
		case 2:
			if (rw == UIO_READ)
				return (0);
			c = iov->iov_len;
			break;
		/*
		 * minfor device 3 is /dev/kUmem (Unibus/Qbus memory,
		 * addressed in 16-bit words).
		 */
		case 3:
			c = iov->iov_len;
			if (!kernacc((caddr_t)uio->uio_offset, c, 
				rw == UIO_READ ? B_READ : B_WRITE)){
				goto fault;
				}
			if (!useracc(iov->iov_base, c, 
				rw == UIO_READ ? B_READ : B_WRITE)){
				goto fault;
				}
			error = UNIcpy((caddr_t)uio->uio_offset, 
					iov->iov_base,(int)c, rw);
			break;
		/*
		 * unknown minor device
		 */
		default:
			error = ENXIO;
			break;
		}
		if (error)
			break;
		iov->iov_base += c;
		iov->iov_len -= c;
		uio->uio_offset += c;
		uio->uio_resid -= c;
	}
	return (error);
fault:
	return (EFAULT);
}

/*
 * kernacc -- Check for kernel access
 * NOTE: currently returns failure for KPTE window
 */
kernacc(base, len, rw)
u_int base, len;
{
	u_int end;
	u_int kvpn;
	struct pte pte;
	extern unsigned Syssize;

	end = base + len - 1;

	/*
	 * check for wrap around
	 */
	if (end < base)
		return(0);

	if (IS_KSEG0(base)) {
		if (IS_KSEG0(end) && btop(K0_TO_PHYS(end)) < physmem)
			return(1);
	} else if (IS_KSEG1(base)) {
		if (IS_KSEG1(end) && btop(K1_TO_PHYS(end)) < physmem)
			return(1);
	} else if (IS_KSEG2(base)) {
		if (!IS_KSEG2(end) || btop(end - K2BASE) >= Syssize)
			return(0);
		for (kvpn = btop(base - K2BASE); kvpn <= btop(end - K2BASE); 
		    kvpn++) {
			pte = Sysmap[kvpn];
			/*
			 * Don't allow access to nocachable pages, since
			 * they are likely device register pages which
			 * may bus error.  May have to change this later.
			 *
			 * Kong - 25-May-1989.
			 *   Need to allow access to I/O space so that
			 *   "sizer" works.  Also, /dev/kUmem is mapped
			 *   to unibus/qbus space and the PTEs are set up
			 *   with no-cache bit set.
			 */
			if (!pte.pg_v)
				return(0);
			if (rw == B_WRITE
			    && pte.pg_prot != PROT_KW && pte.pg_prot != PROT_UW)
				return(0);
		}
		return(1);
	}
	return(0);
}

/*
 * UNIBUS/QBUS Address Space <--> User Space transfer
 */
UNIcpy(uniadd, usradd, n, rw)
	caddr_t uniadd, usradd;
	register int n;
	enum uio_rw rw;
{
	register short *from, *to;
	extern char Sysbase[];
 
	if (rw == UIO_READ) {
		from = (short *)uniadd;
		to = (short *)usradd;
	} else {
		from = (short *)usradd;
		to = (short *)uniadd;
	}
	for (n >>= 1; n > 0; n--) {
		int bad;
		extern	int cpu;

 		bad = BADADDR(uniadd, 2);
		if (bad) {
			return (EFAULT);

		}
		*to++ = *from++;
	}
	return (0);
}
