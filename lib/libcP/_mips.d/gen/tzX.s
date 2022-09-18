/*	"@(#)tzX.s	4.1  (ULTRIX)        7/3/90 */
/*
 *	Define daylight and timezone globally for those who need it,
 *	while letting ANSI and POSIX purists pretend it doesn't exist
 *	(because all the internal usage in the library use __timezone
 *	and __daylight instead).
 */
	.globl	timezone
	.globl	__timezone
	.sdata
timezone:
__timezone:
	.space	4

	.globl	daylight
	.globl	__daylight
	.sdata
daylight:
__daylight:
	.space	4
