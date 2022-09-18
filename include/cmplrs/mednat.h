/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: mednat.h,v 2010.2.1.5 89/11/29 22:39:04 bettina Exp $ */
#ifndef __CMPLRS_MEDNAT_H
#define __CMPLRS_MEDNAT_H

/*#define MEDNAT	1	/* define MEDNAT flag		*/

#ifdef MEDNAT

#define		flush(err);		;
#define		flush(stderr);		;
#define		flush(output);		;
#define		flush(dumpfile);	;

#define		flush (err);		;
#define		flush (stderr);		;
#define		flush (output);		;
#define		flush (dumpfile);	;

#define		cardinal	Card32

#else

#define		stderr			err

#endif

#endif
