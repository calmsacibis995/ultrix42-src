/*	@(#)environ.s	4.1	ULTRIX	7/3/90	*/
/*
 *	Define environ globally for those who need it, while letting
 *	ANSI purists pretend it doesn't exist (because all the internal
 *	usage in the library uses __environ instead).
 */
	.globl	_environ
	.globl	___environ
	.data
_environ:
___environ:
	.space	4
