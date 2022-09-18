/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: aouthdr.h,v 2010.6.1.5 89/11/29 22:41:03 bettina Exp $ */
#include <ansi_compat.h>
#ifndef __AOUTHDR_H
#define __AOUTHDR_H

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if __mips
/*
 * Values for the magic field in aouthdr
 */
#ifdef __LANGUAGE_C
#define	OMAGIC	0407
#define	NMAGIC	0410
#define	ZMAGIC	0413
#define	LIBMAGIC	0443
#define	N_BADMAG(x) \
    (((x).magic)!=OMAGIC && ((x).magic)!=NMAGIC && ((x).magic)!=ZMAGIC && \
     ((x).magic)!=LIBMAGIC)

#endif /* __LANGUAGE_C */
#ifdef LANGUAGE_PASCAL
#define	OMAGIC	8#407
#define	NMAGIC	8#410
#define	ZMAGIC	8#413
#define	LIBMAGIC	8#443
#endif /* LANGUAGE_PASCAL */

#ifdef __LANGUAGE_C
typedef	struct aouthdr {
	short	magic;		/* see above				*/
	short	vstamp;		/* version stamp			*/
	long	tsize;		/* text size in bytes, padded to DW bdry*/
	long	dsize;		/* initialized data "  "		*/
	long	bsize;		/* uninitialized data "   "		*/
#if __u3b
	long	dum1;
	long	dum2;		/* pad to entry point	*/
#endif
	long	entry;		/* entry pt.				*/
	long	text_start;	/* base of text used for this file	*/
	long	data_start;	/* base of data used for this file	*/
#if __mips
	long	bss_start;	/* base of bss used for this file	*/
	long	gprmask;	/* general purpose register mask	*/
	long	cprmask[4];	/* co-processor register masks		*/
	long	gp_value;	/* the gp value used for this object    */
#endif /* __mips */
} AOUTHDR;
#define AOUTHSZ sizeof(AOUTHDR)
#endif /* __LANGUAGE_C */

#ifdef LANGUAGE_PASCAL
type
  aouthdr = packed record
      magic : short;			/* see magic.h			     */
      vstamp : short;			/* version stamp		     */
      tsize : long;			/* text size in bytes, padded to FW  */
					/* bdry 			     */
      dsize : long;			/* initialized data " " 	     */
      bsize : long;			/* uninitialized data " "	     */
#if __u3b
      dum1 : long;
      dum2 : long;			/* pad to entry point		     */
#endif
      entry : long;			/* entry pt.			     */
      text_start : long;		/* base of text used for this file   */
      data_start : long;		/* base of data used for this file   */
      bss_start : long;			/* base of bss used for this file    */
      gprmask : long;			/* general purpose register mask     */
      cprmask : array[0..3] of long;	/* co-processor register masks	     */
      gp_value : long;			/* the gp value used for this object */
    end {record};
#endif /* LANGUAGE_PASCAL */
#endif /* __mips */

/* $Log:	aouthdr.h,v $
 * Revision 2010.6.1.5  89/11/29  22:41:03  bettina
 * 2.10 BETA2
 * 
 * Revision 2010.2  89/09/26  23:32:06  lai
 * added wrapper
 * 
*/
#endif
