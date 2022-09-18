#ifndef lint
static char *sccsid = "@(#)sh.char.c	4.1  (ULTRIX)        7/17/90";
#endif
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: sh.char.c,v 1.3 86/07/11 10:25:14 dce Exp $ */
/*
 * Modification History
 */

#include "sh.char.h"

unsigned short _cmap[256] = {
/*	nul		soh		stx		etx	*/
	0,		0,		0,		0,

/*	eot		enq		ack		bel	*/
	0,		0,		0,		0,

/*	bs		ht		nl		vt	*/
	0,		_SP|_META,	_NL|_META,	0,

/*	np		cr		so		si	*/
	0,		0,		0,		0,

/*	dle		dc1		dc2		dc3	*/
	0,		0,		0,		0,

/*	dc4		nak		syn		etb	*/
	0,		0,		0,		0,

/*	can		em		sub		esc	*/
	0,		0,		0,		0,

/*	fs		gs		rs		us	*/
	0,		0,		0,		0,

/*	sp		!		"		#	*/
	_SP|_META,	0,		_Q,		_META,

/*	$		%		&		'	*/
	_DOL,		0,		_META,		_Q,

/*	(		)		*		+	*/
	_META,		_META,		_GLOB,		0,

/*	,		-		.		/	*/
	0,		0,		0,		0,

/*	0		1		2		3	*/
	_DIG,		_DIG,		_DIG,		_DIG,

/*	4		5		6		7	*/
	_DIG,		_DIG,		_DIG,		_DIG,

/*	8		9		:		;	*/
	_DIG,		_DIG,		0,		_META,

/*	<		=		>		?	*/
	_META,		0,		_META,		_GLOB,

/*	@		A		B		C	*/
	0,		_LET,		_LET,		_LET,

/*	D		E		F		G	*/
	_LET,		_LET,		_LET,		_LET,

/*	H		I		J		K	*/
	_LET,		_LET,		_LET,		_LET,

/*	L		M		N		O	*/
	_LET,		_LET,		_LET,		_LET,

/*	P		Q		R		S	*/
	_LET,		_LET,		_LET,		_LET,

/*	T		U		V		W	*/
	_LET,		_LET,		_LET,		_LET,

/*	X		Y		Z		[	*/
	_LET,		_LET,		_LET,		_GLOB,

/*	\		]		^		_	*/
	_ESC,		0,		0,		_LET,

/*	`		a		b		c	*/
	_Q1|_GLOB,	_LET,		_LET,		_LET,

/*	d		e		f		g	*/
	_LET,		_LET,		_LET,		_LET,

/*	h		i		j		k	*/
	_LET,		_LET,		_LET,		_LET,

/*	l		m		n		o	*/
	_LET,		_LET,		_LET,		_LET,

/*	p		q		r		s	*/
	_LET,		_LET,		_LET,		_LET,

/*	t		u		v		w	*/
	_LET,		_LET,		_LET,		_LET,

/*	x		y		z		{	*/
	_LET,		_LET,		_LET,		_GLOB,

/*	|		}		~		del	*/
	_META,		0,		0,		0,
};
