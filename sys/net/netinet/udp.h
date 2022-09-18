/* SCCSID: @(#)udp.h	4.1		(Ultrix)	7/2/90	*/

/*
 * Udp protocol header.
 * Per RFC 768, September, 1981.
 */
struct udphdr {
	u_short	uh_sport;		/* source port */
	u_short	uh_dport;		/* destination port */
	short	uh_ulen;		/* udp length */
	u_short	uh_sum;			/* udp checksum */
};
extern struct lock_t lk_udb;	/* SMP: udb lock */
