/*	@(#)reboot.h	4.2	(ULTRIX)	9/4/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************
 *  Modification History
 *
 *  20 Apr 8y -- afd
 *	Moved restart/boot/halt codes for Mayfair/CVAX to ka650.h.
 *
 *  06 Mar 8y -- afd
 *	Added restart/boot/halt codes for Mayfair/CVAX.
 *
 *  05 Aug 86 -- Fred Canter
 *	Added VAXstar restart/boot/halt codes.
 *
 * 12 Nov 84 -- rjl
 *	Added constants used to communicate boot options to the MicroVAX-II
 *	console program
 *
 *
 * Arguments to reboot system call.
 * These are passed to boot program in r11,
 * and on to init.
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#define	RB_AUTOBOOT	0	/* flags for system auto-booting itself */

#define	RB_ASKNAME	0x01	/* ask for file name to reboot from */
#define	RB_SINGLE	0x02	/* reboot to single user only */
#define RB_SYNC_DUMP	0x02	/* sync before reboot */
#define	RB_NOSYNC	0x04	/* dont sync before reboot */
#define	RB_HALT		0x08	/* don't reboot, just halt */
#define	RB_INITNAME	0x10	/* name given for /etc/init */
#ifdef __mips
#define	RB_DFLTROOT	0x20	/* use compiled-in rootdev */
#define	RB_NOBOOTRC	0x40	/* don't run /etc/rc.boot */
#endif /* __mips */

#define	RB_PANIC	0	/* reboot due to panic */
#define	RB_BOOT		1	/* reboot due to boot() */

/*
#ifdef __vax
 * Flags for MicroVAX-II console program communication
#endif __vax
#ifdef __mips
 * Constants for converting boot-style device number to type,
 * adaptor (uba, mba, etc), unit number and partition number.
 * Type (== major device number) is in the low byte
 * for backward compatibility.  Except for that of the "magic
 * number", each mask applies to the shifted value.
#endif __mips
 */
#ifdef __vax
#define RB_RESTART	0x21	/* Restart, english	*/
#define RB_REBOOT	0x22	/* Reboot, english	*/
#define RB_HALTMD	0x23	/* Halt, english	*/

/*
 * Flags for VAXstar console program communication.
 * NOTE: must be left shifted two bits.
 */
#define	RB_VS_RESTART	(0x1<<2)	/* restart, boot, halt */
#define	RB_VS_REBOOT	(0x2<<2)	/* boot, halt */
#define	RB_VS_HALTMD	(0x3<<2)	/* halt */
#endif /* __vax */
#ifdef __mips
#define	B_ADAPTORSHIFT	24
#define	B_ADAPTORMASK	0x0f
#define B_UNITSHIFT	16
#define B_UNITMASK	0xff
#define B_PARTITIONSHIFT 8
#define B_PARTITIONMASK	0xff
#define	B_TYPESHIFT	0
#define	B_TYPEMASK	0xff
#define	B_MAGICMASK	0xf0000000
#define	B_DEVMAGIC	0xa0000000
#endif /* __mips */
