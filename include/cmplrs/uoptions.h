/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: uoptions.h,v 2010.2.1.5 89/11/29 22:39:15 bettina Exp $ */

/* Options names for the Uoptn Ucode				*/
#define UCO_SOURCE	0
#define UCO_VARARGS     1	/* function receives variable number of arguments */
/* TVARARGS <value>, where value is the position of va_alist in the actual 
         parameter list. va_alist is defined in /usr/include/varargs.h. */
#define UCO_STACK_LIMIT 2  	/* block number of variable containing stacklimit */
#define UCO_NO_R23      3	/* register 23 reserved to store the stack limit, in Ada only */ 
#define UCO_STACK_REVERSED	4 /* stack is upward growing instead of the
				     default downward growing */
#define UCO_RSTEXT	5 	/* this option specifies that all the text in
				   the current file is to be put in the
				   special section named by .Trstext */

/* Option names for the OPTN UCO_SOURCE that specifies the source language */
#define PASCAL_SOURCE		1
#define FORTRAN_SOURCE		2
#define C_SOURCE		3
#define ADA_SOURCE		4
#define PL1_SOURCE		5
#define COBOL_SOURCE		6
#define RESERVED1_SOURCE	7

/* uopt options */
#define UCO_ZMARK	401
#define UCO_ZVREF	402
#define UCO_ZDBUG	403
#define UCO_ZMOVC	404 
#define UCO_ZCOPY	405
#define UCO_ZCOMO	406
#define UCO_ZSTOR	407
#define UCO_ZSCM 	408
#define UCO_ZALOC	409
