/*	@(#)ether.h	4.1	Ultrix	7/2/90	*/

/*
 * Sockaddr for raw 3Mb/s interface.
 *
 * The network is needed to locate an interface on output.
 */
struct	sockaddr_en {
	short	sen_family;
	short	sen_zero1;
	u_int	sen_net;
	u_char	sen_host;
	u_char	sen_zero2[5];
};
