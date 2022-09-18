/* sccsid: @(#)screentab.h	4.1	(ULTRIX)	9/11/90 */
/*
 * screentab.h
 *
 * Internal definitions for screen table functions
 *
 * Modification history:
 *
 * 19 December 1988	Jeffrey Mogul/DECWRL
 *	Created.
 */
/*
 *               Copyright 1989, 1990 Digital Equipment Corporation
 *                          All Rights Reserved
 * 
 * 
 * Permission to use, copy, and modify this software and its documentation
 * is hereby granted only under the following terms and conditions.  Both
 * the above copyright notice and this permission notice must appear in
 * all copies of the software, derivative works or modified versions, and
 * any portions threof, and both notices must appear in supporting
 * documentation.
 * 
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-free
 * right and license under any changes, enhancements or extensions made to the
 * core functions of the software, including but not limited to those affording
 * compatibility with other hardware or software environments, but excluding
 * applications which incorporate this software.  Users further agree to use
 * their best efforts to return to Digital any such changes, enhancements or
 * extensions that they make and inform Digital of noteworthy uses of this
 * software.  Correspondence should be provided to Digital at:
 * 
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301  
 * 
 * Comments and bug reports may also be sent using electronic mail to:
 * 			screend-reports@decwrl.dec.com
 * 
 * 	>> This software may NOT be distributed to third parties. <<
 *   
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL
 * EQUIPMENT CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Maps from network number to appropriate subnet mask
 */
struct NetmaskData {
	struct in_addr	network;
	struct in_addr	mask;
};

/*
 * Address specifier
 */
struct AddrSpec {
	int	addrtype;
	union {
		struct in_addr	network;	/* INADDRANY == ANY */
		struct in_addr	subnet;
		struct in_addr	host;
	} aval;
};

/* values for addrtype */
#define	ASAT_ANY	0
#define	ASAT_NET	1
#define	ASAT_SUBNET	2
#define	ASAT_HOST	3

/*
 * Port value
 *	This is a composite value because it can either be
 *	an actual number, or a code representing a range.
 */
struct PortValue {
	short discrim;		/* see codes below */
	short value;		/* actual port number */
};

/*
 *	Some day, the ranges might be configured dynamically
 */
#define	PORTV_EXACT	0		/* value must match */
#define	PORTV_ANY	-1		/* Any port */
#define	PORTV_RESERVED	-2		/* between 0 and IPPORT_RESERVED */
#define	PORTV_XSERVER	-3		/* between XSERVERPORT_MIN and
						XSERVERPORT_MAX */
#define	XSERVERPORT_MIN	6000		/* shouldn't be here, but ... */
#define	XSERVERPORT_MAX	6100


/*
 * Port specifier
 */
struct PortSpec {
	int	proto;				/* 0 == ANY */
	struct {
		struct PortValue port;
		short	code;			/* 0 == ANY */
	} pval;
};

/*
 * Complete object specifier
 */
struct ObjectSpec {
	struct AddrSpec aspec;
	struct PortSpec pspec;
	int		flags;	/* defs below */
};

/* values for ObjectSpec.flag */
#define	OSF_DEFAULT		0x0	/* no flags set */
#define	OSF_NOTADDR		0x1	/* looking for non-matching addr */
#define	OSF_NOTPROTO		0x2	/* looking for non-matching proto */
#define	OSF_NOTPORT		0x4	/* looking for non-matching port */
		/* OSF_NOTPROTO|OSF_NOTPORT makes no sense! */

/* Complete action specifier */
struct ActionSpec {
	struct ObjectSpec from;
	struct ObjectSpec to;
	int		  action;
};

/* values for action (bit fields) */
#define	ASACTION_REJECT		0x0
#define	ASACTION_ACCEPT		0x1
#define	ASACTION_NOTIFY		0x2
#define	ASACTION_NONOTIFY	0x0
#define	ASACTION_LOG		0x4
#define	ASACTION_NOLOG		0x0

/* AddrSpec marked with flags */
struct AddrSpecX {
	struct AddrSpec aspec;
	int		flags;
};

/* PortSpec marked with flags */
struct PortSpecX {
	struct PortSpec pspec;
	int		flags;
};


/*
 * Unpacked abstraction of an IP datagram header stack
 */

struct unpacked_hdr {
	struct in_addr	addr;
	short		port;	/* could be ICMP type code if "src"*/
};

struct unpacked_hdrs {
	struct unpacked_hdr src;
	struct unpacked_hdr dst;
	int		proto;
};

struct annotation {
	struct in_addr	net;
	struct in_addr	subnet;
};

/*
 * Annotated unpacked header
 */
struct annotated_hdrs {
	struct unpacked_hdrs	hdrs;
	/* The following are INADDR_ANY (0) if not initialized yet */
	struct annotation	srcnote;
	struct annotation	dstnote;
};
