/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"

/*
 * Checksum routine for Internet Protocol family headers (mips version).
 *
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 */

int nocksum=0;

in_cksum(m, len)
	register struct mbuf *m;
	register int len;
{
	register int ck;
	unsigned short in_checksum();
	register int mlen;
	register char *addr;
	register int rlen;

	if (nocksum)
		return(0);

	ck = rlen = 0;
	while(m) {
		mlen = (m->m_len > len) ? len : m->m_len;
		addr = mtod(m, char *);

		if ((rlen^(int)addr)&1)
			ck = nuxi_s(in_checksum(addr, mlen, nuxi_s(ck)));
		else
			ck = in_checksum(addr, mlen, ck);

		rlen += mlen;
		len -= mlen;
		if (len == 0)
			break;
		while (m = m->m_next)
			if (m->m_len)
				break;
	}
	if (len)
		printf("in_cksum, ran out of data, %d bytes left\n", len);

	return(~ck & 0xFFFF);
}
