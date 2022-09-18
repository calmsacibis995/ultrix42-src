/*	@(#)fp_control.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: fp_control.s,v 1.1 87/04/05 15:43:53 dce Exp $ */

/*
 * This file contains routines to get and set the floating point control
 * registers.
 */
#include <mips/regdef.h>
#include <mips/asm.h>
#include <mips/fpu.h>

/*
 * get_fpc_csr returns the fpc_csr.
 */
LEAF(get_fpc_csr)
	cfc1	v0,fpc_csr
	j	ra
	END(get_fpc_csr)

/*
 * set_fpc_csr sets the fpc_csr and returns the old fpc_csr.
 */
LEAF(set_fpc_csr)
	cfc1	v0,fpc_csr
	ctc1	a0,fpc_csr
	j	ra
	END(set_fpc_csr)

/*
 * get_fpc_irr returns the fpc_irr.
 */
LEAF(get_fpc_irr)
	cfc1	v0,fpc_irr
	j	ra
	END(get_fpc_irr)

/*
 * set_fpc_led sets the floating board leds.
 */
LEAF(set_fpc_led)
	ctc1	a0,fpc_led
	j	ra
	END(set_fpc_led)

/*
 * get_fpc_eir returns the fpc_eir.
 */
LEAF(get_fpc_eir)
	cfc1	v0,fpc_eir
	j	ra
	END(get_fpc_eir)
