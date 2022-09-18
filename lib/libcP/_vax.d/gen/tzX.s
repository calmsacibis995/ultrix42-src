/*	"@(#)tzX.s	4.1  (ULTRIX)        7/3/90 */
/*
 *	Define daylight and timezone globally for those who need it,
 *	while letting ANSI and POSIX purists pretend it doesn't exist
 *	(because all the internal usage in the library use __timezone
 *	and __daylight instead).
 */
	.globl	_timezone
	.globl	___timezone
	.data
_timezone:
___timezone:
	.space	4

	.globl	_daylight
	.globl	___daylight
	.data
_daylight:
___daylight:
	.space	4
