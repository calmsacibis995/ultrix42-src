/*
 *	Copyright (c) 1982 Regents of the University of California
 */
#ifndef lint
static char sccsid[] = "@(#)aspseudo.c	4.3 ULTRIX 9/4/90";
#endif /* not lint */

/************************************************************************
 *									*
 *			Modification History				*
 *
 * 001  Tanya Klinchina, 20-Nov-1989
 *      Added support for vector instructions.
 *
 ************************************************************************/

#include <stdio.h>
#include "as.h"

#define	OP(name, eopcode, popcode, nargs, arg1, arg2, arg3, arg4, arg5, arg6) \
	{ \
		name, popcode, nargs, arg1, arg2, arg3, arg4, arg5, arg6, \
		(nargs == 0 ? INST0:INSTn), eopcode \
	}
#define	VOP(name, instr, eopcode, popcode, format, ctrlcode, nargs, arg1, arg2, arg3, arg4, arg5, arg6) \
	{ \
		name, popcode, nargs, arg1, arg2, arg3, arg4, arg5, arg6, \
		instr, eopcode, format, ctrlcode \
	}
#define	PSEUDO(name, type, tag) \
	{ \
		name, type, 0,   0, 0, 0, 0, 0, 0, \
		tag, CORE \
	}

readonly struct Instab instab[] = {
#include "instrs.as"
PSEUDO("\0\0\0\0\0\0\0\0\0\0", 0, 0)
};
