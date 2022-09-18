	.data
_sccsid:
	.asciz	"@(#)call_stubs.s	4.1      ULTRIX 7/2/90"

#************************************************************************
#									*
#			Copyright (c) 1988 by				*
#		Digital Equipment Corporation, Maynard, MA		*
#			All rights reserved.				*
#									*
#   This software is furnished under a license and may be used and	*
#   copied  only  in accordance with the terms of such license and	*
#   with the  inclusion  of  the  above  copyright  notice.   This	*
#   software  or  any  other copies thereof may not be provided or	*
#   otherwise made available to any other person.  No title to and	*
#   ownership of the software is hereby transferred.			*
#									*
#   The information in this software is subject to change  without	*
#   notice  and should not be construed as a commitment by Digital	*
#   Equipment Corporation.						*
#									*
#   Digital assumes no responsibility for the use  or  reliability	*
#   of its software on equipment which is not supplied by Digital.	*
#									*
#************************************************************************


# call_stubs.s -- call Ultrix routines from VMS BLISS code
#    preserving the appropriate registers

#  SCCS history beginning
#  ***************************************************************
#                 -- Revision History --
#  ***************************************************************
#  
#  1.1  06/07/88 -- thoms
#  date and time created 88/07/06 13:19:16 by thoms
#  
#  ***************************************************************
#
#  1.2  19/07/88 -- thoms
#  Added copyright notice and modification history
#
#  SCCS history end
#


	.text

#
# GET_XLBUF -- is passed to BLISS to enable call of
#	get_xlbuf routine (ultrix_io.c)
#
#	more registers are saved than strictly necessary
#
	.align	1
	.globl	_GET_XLBUF
	.globl	_get_xlbuf
_GET_XLBUF:
	.word	0x3e
	callg	(ap), _get_xlbuf
	ret

#
# PUT_XLBUF -- is passed to BLISS to enable call of
#	put_xlbuf routine (ultrix_io.c)
#
#	more registers are saved than strictly necessary
#
	.align	1
	.globl	_PUT_XLBUF
	.globl	_put_xlbuf
_PUT_XLBUF:
	.word	0x3e
	callg	(ap), _put_xlbuf
	ret

# TRANSLATOR -- this routine simply maps the call of TRANSLATOR
#	in ultrix_main.c onto the particular entry point.
#	No extra registers are saved.
#
# NOTE:
#	TRN$REGIS_PS and TRN$TEK4014_PS have the same
#	value so this routine is called for BOTH translators
#
	.align	1
	.globl	TRN$REGIS_PS
	.globl	_TRANSLATOR
_TRANSLATOR:
	.word	0x00
	callg	(ap), TRN$REGIS_PS
	ret
