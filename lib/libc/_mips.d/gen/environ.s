/*	@(#)environ.s	4.1	ULTRIX	7/3/90	*/
/*
 *	Define environ globally for those who need it, while letting
 *	ANSI purists pretend it doesn't exist (because all the internal
 *	usage in the library uses __environ instead).
 *
 *	It's in sdata for maximum flexibility; this way, it can be
 *	referenced as either large or small.
 */

	.globl	environ
	.globl	__environ
	.sdata
environ:
__environ:
	.space	4
