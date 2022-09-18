/*
 * 	@(#)pdma.h	4.1	(ULTRIX)	7/2/90
 */

struct pdma {
	struct	dzdevice *p_addr;
	char	*p_mem;
	char	*p_end;
	int	p_arg;
	int	(*p_fcn)();
};
