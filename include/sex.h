#include <ansi_compat.h>
#ifdef __mips
/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: sex.h,v 2010.5.1.5 89/11/29 22:41:08 bettina Exp $ */

/*
 * This file contains macro constant names for byte sex flags, the macros for
 * byte swapping words and half words, and the external declarations for the
 * routines in libsex.a which change the sex of structures that appear
 * in object files.
 */

/*
 * Byte sex constants
 */
#define BIGENDIAN	0
#define LITTLEENDIAN	1
#define UNKNOWNENDIAN	2

/*
 * Byte swaps for word and half words.
 */
#define swap_word(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned long)(a) >>24) )

#define swap_half(a) ( ((a & 0xff) << 8) | ((unsigned short)(a) >> 8) )

extern
int
gethostsex();

extern
void
swap_filehdr();

extern
void
swap_aouthdr();

extern
void
swap_scnhdr();

extern
void
swap_hdr();

extern
void
swap_fd();

extern
void
swap_fi();

extern
void
swap_sym();

extern
void
swap_ext();

extern
void
swap_pd();

extern
void
swap_opt();

extern
void
swap_aux();

extern
void
swap_line();

extern
void
swap_reloc();

extern
void
swap_ranlib();

extern
void
swap_gpt();
#endif
