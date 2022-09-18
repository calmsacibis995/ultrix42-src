#include <ansi_compat.h>
#ifdef __vax
/*	ar.h	4.1	83/05/03	*/

#define	ARMAG	"!<arch>\n"
#define	SARMAG	8

#define	ARFMAG	"`\n"

struct ar_hdr {
	char	ar_name[16];
	char	ar_date[12];
	char	ar_uid[6];
	char	ar_gid[6];
	char	ar_mode[8];
	char	ar_size[10];
	char	ar_fmag[2];
};
#endif /* __vax */
#ifdef __mips
/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ar.h,v 2010.5.1.5 89/11/29 22:41:13 bettina Exp $ */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if __vax || __u3b || __M32 || __u3b15 || __u3b5 || __u3b2 || __mc68000 || __mips
/*		COMMON ARCHIVE FORMAT
*
*	ARCHIVE File Organization:
*	_______________________________________________
*	|__________ARCHIVE_MAGIC_STRING_______________|
*	|__________ARCHIVE_FILE_MEMBER_1______________|
*	|					      |
*	|	Archive File Header "ar_hdr"          |
*	|.............................................|
*	|	Member Contents			      |
*	|		1. External symbol directory  |
*	|		2. Text file		      |
*	|_____________________________________________|
*	|________ARCHIVE_FILE_MEMBER_2________________|
*	|		"ar_hdr"		      |
*	|.............................................|
*	|	Member Contents (.o or text file)     |
*	|_____________________________________________|
*	|	.		.		.     |
*	|	.		.		.     |
*	|	.		.		.     |
*	|_____________________________________________|
*	|________ARCHIVE_FILE_MEMBER_n________________|
*	|		"ar_hdr"		      |
*	|.............................................|
*	|		Member Contents 	      |
*	|_____________________________________________|
*
*/

#define ARMAG	"!<arch>\n"
#define SARMAG	8
#define ARFMAG	"`\n"

struct ar_hdr		/* archive file member header - printable ascii */
{
	char	ar_name[16];	/* file member name - `/' terminated */
	char	ar_date[12];	/* file member date - decimal */
	char	ar_uid[6];	/* file member user id - decimal */
	char	ar_gid[6];	/* file member group id - decimal */
	char	ar_mode[8];	/* file member mode - octal */
	char	ar_size[10];	/* file member size - decimal */
	char	ar_fmag[2];	/* ARFMAG - string to end header */
};
typedef struct ar_hdr ARHDR;

#else /* u370 || pdp11 */

#define ARMAG	0177545
struct ar_hdr
{
	char	ar_name[14];
	long	ar_date;
	char	ar_uid;
	char	ar_gid;
	int	ar_mode;
	long	ar_size;
};

#endif

#ifdef __mips
/*
 * The rest of this file deals the symdef file. This file contains a hash table
 *	and a string table. The hash table contains ranlib structures; if
 *	the ran_off field is non-zero, then the element refers to a defined
 *	external in one of the member files.
 *
 * The symdef file name is always prefixed by SYMPREF. The IHASHSEXth
 *	character in the name determines the sex of the hash table and
 *	the ITARGETSEXth character determines the target sex of the
 *	members -- no  symdef file will be created if there are member 
 *	with conflicting target sexes (see the -s flag in ar.c).
 *
 * In addition, if an archive is modified with ar, the IOUTOFDATEth character
 *	in the name of the symdef file will be set to OUTOFDATE. The other times
 *	it will be set to '_'
 *
 * Notes:
 *	1) Mips archives use the "PORTAR" or portable archive format.
 *	2) You must include sex.h before ar.h if you use the symdef macros.
 */

#define AR_SYMPREF 	"__________E"		/* common unique prefix */
#define AR_SYMPREFLEN	11			/* length of prefix */
#define AR_IHASHSEX	11			/* index for hash sex char */
#define AR_IUCODE	12			/* index of ucode char */
#define AR_ITARGETSEX	13			/* index for target sex char */
#define AR_IOUTOFDATE	14			/* index for out of date char */
#define AR_OUTOFDATE	'X'			/* out of date char */
#define AR_EL		'L'			/* EL char */
#define AR_EB		'B'			/* EB char */
#define AR_UCODE	'U'			/* UCODE char */
#define AR_CODE		'E'			/* regulart code char */

/* the following macros provide a higher level interface for above constants */
#define AR_HASHSEX(x) (((x)[AR_IHASHSEX] == AR_EB) ? BIGENDIAN : LITTLEENDIAN)
#define AR_TARGETSEX(x) (((x)[AR_ITARGETSEX] == AR_EB) ? BIGENDIAN : LITTLEENDIAN)
#define AR_ISSYMDEF(x) (!strncmp(x, AR_SYMPREF, AR_SYMPREFLEN))
#define AR_ISOUTOFDATE(x) ((x)[AR_IOUTOFDATE] == AR_OUTOFDATE)
#define AR_ISUCODE(x) ((x)[AR_IUCODE] == AR_UCODE)


/*
 * The symdef file begins with a word giving the number of ranlib structures
 *	which immediately follow, and then continues with a string
 *	table consisting of a word giving the number of bytes of strings
 *	which follow and then the strings themselves.
 *
 * The ran_strx fields index the string table whose first byte is numbered 0.
 *
 * Since this is a hash table, only those entry's with non-zero ran_off
 *	fields really represent a defined external. See libld (ldhash) for the
 *	hash function -- if ran_off is negative it contains the size of
 *	the symbol which must be common. No file is included to define a
 *	common.
 */

struct	ranlib {
	union {
		long	ran_strx;	/* string table index of */
		char	*ran_name;	/* symbol defined by */
	} ran_un;
	long	ran_off;		/* library member at this offset */
};
#endif /* __mips */
#endif /* __mips */
