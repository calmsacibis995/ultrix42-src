/*
 * Copyright (c) 1990, by John Robert LoVerso.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by John Robert LoVerso.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * This implementaion has been influenced by the CMU SNMP release,
 * by Steve Waldbusser.  However, this shares no code with that system.
 * Additional ASN.1 insight gained from Marshall T. Rose's _The_Open_Book_.
 * Earlier forms of this implemention were derived and/or inspired by an
 * awk script originally written by C. Philip Wood of LANL (but later
 * heavily modified by John Robert LoVerso).  The copyright notice for
 * that work is preserved below, even though it may not rightly apply
 * to this file.
 *
 #			Los Alamos National Laboratory
 #
 #	Copyright, 1990.  The Regents of the University of California.
 #	This software was produced under a U.S. Government contract
 #	(W-7405-ENG-36) by Los Alamos National Laboratory, which is
 #	operated by the	University of California for the U.S. Department
 #	of Energy.  The U.S. Government is licensed to use, reproduce,
 #	and distribute this software.  Permission is granted to the
 #	public to copy and use this software without charge, provided
 #	that this Notice and any statement of authorship are reproduced
 #	on all copies.  Neither the Government nor the University makes
 #	any warranty, express or implied, or assumes any liability or
 #	responsibility for the use of this software.
 #	@(#)snmp.awk.x	1.1 (LANL) 1/15/90
 *
 * SCCSID: @(#)print-snmp.c	4.1	ULTRIX	1/25/91
 * Based on:
static char rcsid[] =
    "@(#) $Id: snmp_print.c,v 3.4 90/07/26 23:19:46 loverso Rel $ (jlv)";
 */

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>

#include "interface.h"
#include "addrtoname.h"

/*
 * Universal ASN.1 types
 * (we only care about the tag values for those allowed in the SMI)
 */
char *Universal[] = {
	"U-0",
	"Boolean",
	"Integer",
#define INTEGER 2
	"Bitstring",
	"String",
#define STRING 4
	"Null",
#define ASN_NULL 5
	"ObjID",
#define OBJECTID 6
	"ObjectDes",
	"U-8","U-9","U-10","U-11",	/* 8-11 */
	"U-12","U-13","U-14","U-15",	/* 12-15 */
	"Sequence",
#define SEQUENCE 16
	"Set"
};

/*
 * Application-wide types from the SMI and their tags
 */
char *Application[] = {
	"IpAddress",
#define IPADDR 0
	"Counter",
#define COUNTER 1
	"Gauge",
#define GAUGE 2
	"TimeTicks",
#define TIMETICKS 3
	"Opaque"
};

/*
 * Context-specific types for the SNMP PDUs and their tags
 */
char *Context[] = {
	"GetRequest",
#define GETREQ 0
	"GetNextRequest",
#define GETNEXTREQ 1
	"GetResponse",
#define GETRESP 2
	"SetRequest",
#define SETREQ 3
	"Trap"
#define TRAP 4
};

/*
 * Private types - there are none in this case
 */
char *Private[] = {
	"P-0"
};

/*
 * error-status in an PDU for SNMP
 */
char *ErrorStatus[] = {
	"noError",
	"tooBig",
	"noSuchName",
	"badValue",
	"readOnly",
	"genErr"
};
#define DECODE_ErrorStatus(e) \
	( e >= 0 && e <= sizeof(ErrorStatus)/sizeof(ErrorStatus[0]) \
	? ErrorStatus[e] : (sprintf(errbuf, "err=%d", e), errbuf))

/*
 * generic-trap values in the SNMP Trap-PDU
 */
char *GenericTrap[] = {
	"coldStart",
	"warmStart",
	"linkDown",
	"linkUp",
	"authenticationFailure",
	"egpNeighborLoss",
	"enterpriseSpecific"
#define GT_ENTERPRISE 7
};
#define DECODE_GenericTrap(t) \
	( t >= 0 && t <= sizeof(GenericTrap)/sizeof(GenericTrap[0]) \
	? GenericTrap[t] : (sprintf(buf, "gt=%d", t), buf))

/*
 * Type Class table
 */
#define defineCLASS(x) { "x", x, sizeof(x)/sizeof(x[0]) } /* not ANSI-C */
struct {
	char	*name;
	char	**Id;
	int	numIDs;
} Class[] = {
	defineCLASS(Universal),
#define	UNIVERSAL	0
	defineCLASS(Application),
#define	APPLICATION	1
	defineCLASS(Context),
#define	CONTEXT		2
	defineCLASS(Private),
#define	PRIVATE		3
};

/*
 * Type Forms
 */
char *Form[] = {
	"Primitive",
#define PRIMITIVE	0
	"Constructed",
#define CONSTRUCTED	1
};

/*
 * A structure for the OID tree for the known (concatenated) MIB
 * This is a general-order tree.
 */
struct obj {
	char	*desc;			/* name of object */
	u_char	oid;			/* sub-id following parent */
	u_char	type;			/* object type, as yet unused */
	struct obj *child, *next;	/* child and next sibling pointers */
} *objp = NULL;

/* this is gross, as mib.h defines an initialized struct */
#include "mib.h"
/* mib.h MUST define at least `mibroot' */

/*
 * This defines a table of allowable OID abreviations
 */
struct obj_abrev {
	char *prefix;			/* prefix for this abrev */
	struct obj *node;		/* pointer into object table */
	char *oid;			/* ASN.1 encoded OID */
} obj_abrev_list[] = {
#ifndef NO_ABREV_MIB
	/* .iso.org.dod.internet.mgmt.mib */
	{ "",	&_mib_obj,	 	"\53\6\1\2\1" },
#endif
#ifndef NO_ABREV_ENTER
	/* .iso.org.dod.internet.private.enterprises */
	{ "E:",	&_enterprises_obj,	"\53\6\1\4\1" },
#endif
#ifndef NO_ABREV_EXPERI
	/* .iso.org.dod.internet.experimental */
	{ "X:",	&_experimental_obj,	"\53\6\1\3" },
#endif
	{ 0,0,0 }
};

/* Walk down object tree */
#define OBJ_PRINT(o, suppressdot) \
{ \
	if (objp) { \
		do { \
			if ((o) == objp->oid) \
				break; \
		} while (objp = objp->next); \
	} \
	if (objp) { \
		printf(suppressdot?"%s":".%s", objp->desc); \
		objp = objp->child; \
	} else \
		printf(suppressdot?"%u":".%u", (o)); \
}

/*
 * Any-Data-Type storage used for internal representation while decoding
 * ASN.1 data.
 */
struct be {
	unsigned long asnlen;
	union {
		caddr_t raw;
		long integer;
		unsigned long uns;
		unsigned char *str;
	} data;
	unsigned char form, class, id;		/* tag info */
	u_char type;
#define BE_ANY		255
#define BE_NONE		0
#define BE_NULL		1
#define BE_OCTET	2
#define BE_OID		3
#define BE_INT		4
#define BE_UNS		5
#define BE_STR		6
#define BE_SEQ		7
#define BE_INETADDR	8
#define BE_PDU		9
};

/*
 * Defaults to not display in SNMP messages
 */
#define DEF_COMMUNITY "public"
#define DEF_VERSION 0

#define OIDMUX 40
#define ASNLEN_INETADDR 4
#define ASN_SHIFT7 7
#define ASN_SHIFT8 8
#define ASN_BIT8 0x80
#define ASN_LONGLEN 0x80

#define ASN_ID_BITS 0x1f
#define ASN_FORM_BITS 0x20
#define ASN_FORM_SHIFT 5
#define ASN_CLASS_BITS 0xc0
#define ASN_CLASS_SHIFT 6

#define ASN_ID_EXT 0x1f		/* extension ID in tag field */

int
asn1_parse(p, len, elem)
	register u_char *p;
	int len;
	struct be *elem;
{
	unsigned char form, class, id;
	int indent=0, i, hdr;
	char *classstr;

	elem->asnlen = 0;
	if (len < 1) {
		printf("[nothing?]");
		return -1;
	}

	/*
	 * it would be nice to use a bit field, but you can't depend on them.
	 *  +---+---+---+---+---+---+---+---+
	 *  + class |frm|        id         |
	 *  +---+---+---+---+---+---+---+---+
	 *    7   6   5   4   3   2   1   0
	 */
	id = *p & ASN_ID_BITS;		/* lower 5 bits, range 00-1f */
#ifdef notdef
	form = (*p & 0xe0) >> 5;	/* move upper 3 bits to lower 3 */
	class = form >> 1;		/* bits 7&6 -> bits 1&0, range 0-3 */
	form &= 0x1;			/* bit 5 -> bit 0, range 0-1 */
#else
	form = (*p & ASN_FORM_BITS) >> ASN_FORM_SHIFT;
	class = (*p & ASN_CLASS_BITS) >> ASN_CLASS_SHIFT;
#endif
	elem->form = form;
	elem->class = class;
	elem->id = id;
	if (vflag)
		printf("|%.2x", *p);
	p++; len--; hdr = 1;
	/* extended tag field */
	if (id == ASN_ID_EXT) {
		for (id = 0; *p & ASN_BIT8 && len > 0; len--, hdr++, p++) {
			if (vflag)
				printf("|%.2x", *p);
			id += *p & ~ASN_BIT8;
		}
		if (len == 0 && *p & ASN_BIT8) {
			printf("[Xtagfield?]");
			return -1;
		}
	}
	if (len < 1) {
		printf("[asnlen?]");
		return -1;
	}
	elem->asnlen = *p;
	if (vflag)
		printf("|%.2x", *p);
	p++; len--; hdr++;
	if (elem->asnlen & ASN_BIT8) {
		int noct = elem->asnlen % ASN_BIT8;
		elem->asnlen = 0;
		if (len < noct) {
			printf("[asnlen? %d<%d]", len, noct);
			return -1;
		}
		for (; noct-- > 0; len--, hdr++) {
			if (vflag)
				printf("|%.2x", *p);
			elem->asnlen = (elem->asnlen << ASN_SHIFT8) | *p++;
		}
	}
	if (len < elem->asnlen) {
		printf("[len%d<asnlen%u]", len, elem->asnlen);
		return -1;
	}
	if (form >= sizeof(Form)/sizeof(Form[0])) {
		printf("[form?%d]", form);
		return -1;
	}
	if (class >= sizeof(Class)/sizeof(Class[0])) {
		printf("[class?%c/%d]", *Form[form], class);
		return -1;
	}
	if (id >= Class[class].numIDs) {
		printf("[id?%c/%s/%d]", *Form[form],
			Class[class].name, id);
	}

	switch (form) {
	case PRIMITIVE:
		switch (class) {
		case UNIVERSAL:
			switch (id) {
			case STRING:
				elem->type = BE_STR;
				elem->data.str = p;
				break;

			case INTEGER: {
				register long data;
				elem->type = BE_INT;
				data = 0;

				if (*p & ASN_BIT8)	/* negative */
					data = -1;
				for (i = elem->asnlen; i-- > 0; p++)
					data = (data << ASN_SHIFT8) | *p;
				elem->data.integer = data;
				break;
			}

			case OBJECTID:
				elem->type = BE_OID;
				elem->data.raw = (caddr_t)p;
				break;

			case ASN_NULL:
				elem->type = BE_NULL;
				elem->data.raw = NULL;
				break;

			default:
				elem->type = BE_OCTET;
				elem->data.raw = (caddr_t)p;
				printf("[P/U/%s]",
					Class[class].Id[id]);
				break;
			}
			break;

		case APPLICATION:
			switch (id) {
			case IPADDR:
				elem->type = BE_INETADDR;
				elem->data.raw = (caddr_t)p;
				break;

			case COUNTER:
			case GAUGE:
			case TIMETICKS: {
				register unsigned long data;
				elem->type = BE_UNS;
				data = 0;
				for (i = elem->asnlen; i-- > 0; p++)
					data = (data << 8) + *p;
				elem->data.uns = data;
				break;
			}

			default:
				elem->type = BE_OCTET;
				elem->data.raw = (caddr_t)p;
				printf("[P/A/%s]",
					Class[class].Id[id]);
				break;
			}
			break;

		default:
			elem->type = BE_OCTET;
			elem->data.raw = (caddr_t)p;
			printf("[P/%s/%s]",
				Class[class].name, Class[class].Id[id]);
			break;
		}
		break;

	case CONSTRUCTED:
		switch (class) {
		case UNIVERSAL:
			switch (id) {
			case SEQUENCE:
				elem->type = BE_SEQ;
				elem->data.raw = (caddr_t)p;
				break;

			default:
				elem->type = BE_OCTET;
				elem->data.raw = (caddr_t)p;
				printf("C/U/%s", Class[class].Id[id]);
				break;
			}
			break;

		case CONTEXT:
			elem->type = BE_PDU;
			elem->data.raw = (caddr_t)p;
			break;

		default:
			elem->type = BE_OCTET;
			elem->data.raw = (caddr_t)p;
			printf("C/%s/%s",
				Class[class].name, Class[class].Id[id]);
			break;
		}
		break;
	}
	p += elem->asnlen;
	len -= elem->asnlen;
	return elem->asnlen + hdr;
}

void
asn1_print(elem)
	struct be *elem;
{
	u_char *p = (u_char *)elem->data.raw;
	u_long asnlen = elem->asnlen;
	int i;

	switch (elem->type) {

	case BE_OCTET:
		for (i = asnlen; i-- > 0; p++);
			printf("_%.2x", *p);
		break;

	case BE_NULL:
		break;

	case BE_OID: {
		int o = 0, first = -1, i = asnlen;

		if (!nflag && asnlen > 2) {
			struct obj_abrev *a = &obj_abrev_list[0];
			for (; a->node; a++) {
				if (!memcmp(a->oid, p, strlen(a->oid))) {
					objp = a->node->child;
					i -= strlen(a->oid);
					p += strlen(a->oid);
					fputs(a->prefix, stdout);
					first = 1;
					break;
				}
			}
		}
		for (; i-- > 0; p++) {
			o = (o << ASN_SHIFT7) + (*p & ~ASN_BIT8);
			if (*p & ASN_LONGLEN)
				continue;

			/*
			 * first subitem encodes two items with 1st*OIDMUX+2nd
			 */
			if (first < 0) {
				if (!nflag)
					objp = mibroot;
				first = 0;
				OBJ_PRINT(o/OIDMUX, first);
				o %= OIDMUX;
			}
			OBJ_PRINT(o, first);
			if (--first < 0)
				first = 0;
			o = 0;
		}
		break;
	}

	case BE_INT:
		printf("%ld", elem->data.integer);
		break;

	case BE_UNS:
		printf("%ld", elem->data.uns);
		break;

	case BE_STR: {
		int printable = 1;
		u_char *p = elem->data.str;
		for (i = asnlen; printable && i-- > 0; p++)
			printable = isprint(*p);
		if (printable)
			putchar('"');
		for (p = elem->data.str, i = asnlen; i-- > 0; p++)
			if (printable)
				putchar(*p);
			else
				printf("_%.2x", *p);
		if (printable)
			putchar('"');
		break;
	}

	case BE_SEQ:
		printf("Seq(%d)", elem->asnlen);
		break;

	case BE_INETADDR: {
		char sep;
		if (asnlen != ASNLEN_INETADDR)
			fprintf(stdout, "[inetaddr len!=%d]", ASNLEN_INETADDR);
		sep='[';
		for (i = asnlen; i-- > 0; p++) {
			printf("%c%u", sep, *p);
			sep='.';
		}
		putchar(']');
		break;
	}

	case BE_PDU:
		printf("%s(%d)",
			Class[CONTEXT].Id[elem->id], elem->asnlen);
		break;

	default:
		printf("[be!?]");
		break;
	}
}

/*
 * Brute force ASN.1 printer: recurses to dump an entire structure
 */
void
asn1_decode(p, length)
	u_char *p;
	int length;
{
	struct be elem;
	int i = 0;

	while (i >= 0 && length > 0) {
		i = asn1_parse(p, length, &elem);
		if (i >= 0) {
			printf(" ");
			asn1_print(&elem);
			if (elem.type == BE_SEQ || elem.type == BE_PDU) {
				printf(" {");
				asn1_decode(elem.data.raw, elem.asnlen);
				printf(" }");
			}
			length -= i;
			p += i;
		}
	}
}

/*
 * General SNMP header
 *	SEQUENCE {
 *		version INTEGER {version-1(0)},
 *		community OCTET STRING,
 *		data ANY 	-- PDUs
 *	}
 * PDUs for all but Trap: (see rfc1157 from page 15 on)
 *	SEQUENCE {
 *		request-id INTEGER,
 *		error-status INTEGER,
 *		error-index INTEGER,
 *		varbindlist SEQUENCE OF
 *			SEQUENCE {
 *				name ObjectName,
 *				value ObjectValue
 *			}
 *	}
 * PDU for Trap:
 *	SEQUENCE {
 *		enterprise OBJECT IDENTIFIER,
 *		agent-addr NetworkAddress,
 *		generic-trap INTEGER,
 *		specific-trap INTEGER,
 *		time-stamp TimeTicks,
 *		varbindlist SEQUENCE OF
 *			SEQUENCE {
 *				name ObjectName,
 *				value ObjectValue
 *			}
 *	}
 */

/*
 * Decode SNMP varBind
 */
void
varbind_print (pduid, np, length, error)
	u_char pduid, *np;
	int length, error;
{
	struct be elem;
	int count = 0, index;

	/* Sequence of varBind */
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_SEQ) {
		printf("[!SEQ of varbind]");
		asn1_print(&elem);
		return;
	}
	if (count < length)
		printf("[%d extra after SEQ of varbind]", length - count);
	/* descend */
	np = (u_char *)elem.data.raw;
	length = snapend - np;

	for (index = 1; length > 0; index++) {
		u_char *vbend;
		int vblength;

		if (!error || index == error)
			fputs(" ", stdout);

		/* Sequence */
		count = asn1_parse(np, length, &elem);
		if (elem.type != BE_SEQ) {
			printf("[!varbind]");
			asn1_print(&elem);
			return;
		}
		vbend = np + count;
		vblength = length - count;
		/* descend */
		np = (u_char *)elem.data.raw;
		length = snapend - np;

		/* objName (OID) */
		count = asn1_parse(np, length, &elem);
		if (elem.type != BE_OID) {
			printf("[objName!=OID]");
			asn1_print(&elem);
			return;
		}
		if (!error || index == error)
			asn1_print(&elem);
		length -= count;
		np += count;

		if (pduid != GETREQ && pduid != GETNEXTREQ && !error)
				fputs("=", stdout);

		/* objVal (ANY) */
		count = asn1_parse(np, length, &elem);
		if (pduid == GETREQ || pduid == GETNEXTREQ) {
			if (elem.type != BE_NULL) {
				printf("[objVal!=NULL]");
				asn1_print(&elem);
			}
		} else
			if (error && index == error && elem.type != BE_NULL)
				printf("[err objVal!=NULL]");
			if (!error || index == error)
				asn1_print(&elem);

		length = vblength;
		np = vbend;
	}
}

/*
 * Decode SNMP PDUs: GetRequest, GetNextRequest, GetResponse, and SetRequest
 */
void
snmppdu_print (pduid, np, length)
	u_char pduid, *np;
	int length;
{
	struct be elem;
	int count = 0, error;

	/* reqId (Integer) */
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_INT) {
		printf("[reqId!=INT]");
		asn1_print(&elem);
		return;
	}
	/* ignore */
	length -= count;
	np += count;

	/* errorStatus (Integer) */
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_INT) {
		printf("[errorStatus!=INT]");
		asn1_print(&elem);
		return;
	}
	error = 0;
	if ((pduid == GETREQ || pduid == GETNEXTREQ)
	    && elem.data.integer != 0) {
		char errbuf[10];
		printf("[errorStatus(%s)!=0]", 
			DECODE_ErrorStatus(elem.data.integer));
	} else if (elem.data.integer != 0) {
		char errbuf[10];
		printf(" %s", DECODE_ErrorStatus(elem.data.integer));
		error = elem.data.integer;
	}
	length -= count;
	np += count;

	/* errorIndex (Integer) */
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_INT) {
		printf("[errorIndex!=INT]");
		asn1_print(&elem);
		return;
	}
	if ((pduid == GETREQ || pduid == GETNEXTREQ)
	    && elem.data.integer != 0)
		printf("[errorIndex(%d)!=0]", elem.data.integer);
	else if (elem.data.integer != 0) {
		if (!error)
			printf("[errorIndex(%d) w/o errorStatus]",
				elem.data.integer);
		else {
			printf("@%d", elem.data.integer);
			error = elem.data.integer;
		}
	} else if (error) {
		printf("[errorIndex==0]");
		error = 0;
	}
	length -= count;
	np += count;

	varbind_print(pduid, np, length, error);
	return;
}

/*
 * Decode SNMP Trap PDU
 */
void
trap_print (np, length)
	u_char *np;
	int length;
{
	struct be elem;
	int count = 0, generic;

	putchar(' ');

	/* enterprise (oid) */
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_OID) {
		printf("[enterprise!=OID]");
		asn1_print(&elem);
		return;
	}
	asn1_print(&elem);
	length -= count;
	np += count;

	putchar(' ');

	/* agent-addr (inetaddr) */
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_INETADDR) {
		printf("[agent-addr!=INETADDR]");
		asn1_print(&elem);
		return;
	}
	asn1_print(&elem);
	length -= count;
	np += count;

	/* generic-trap (Integer) */
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_INT) {
		printf("[generic-trap!=INT]");
		asn1_print(&elem);
		return;
	}
	generic = elem.data.integer;
	{
		char buf[10];
		printf(" %s", DECODE_GenericTrap(generic));
	}
	length -= count;
	np += count;

	/* specific-trap (Integer) */
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_INT) {
		printf("[specific-trap!=INT]");
		asn1_print(&elem);
		return;
	}
	if (generic != GT_ENTERPRISE) {
		if (elem.data.integer != 0)
			printf("[specific-trap(%d)!=0]", elem.data.integer);
	} else
		printf(" s=%d", elem.data.integer);
	length -= count;
	np += count;

	putchar(' ');

	/* time-stamp (TimeTicks) */
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_UNS) {			/* XXX */
		printf("[time-stamp!=TIMETICKS]");
		asn1_print(&elem);
		return;
	}
	asn1_print(&elem);
	length -= count;
	np += count;

	varbind_print (TRAP, np, length, 0);
	return;
}

/*
 * Decode SNMP header and pass on to PDU printing routines
 */
void
snmp_print (np, length)
	u_char *np;
	int length;
{
	struct be elem, pdu;
	int count = 0;

	putchar(' ');

	/* Sequence */
	length = snapend - np;
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_SEQ) {
		printf("[!init SEQ]");
		asn1_print(&elem);
		return;
	}
	if (count < length)
		printf("[%d extra after iSEQ]", length - count);
	/* descend */
	np = (u_char *)elem.data.raw;
	length = snapend - np;

	/* Version (Integer) */
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_INT) {
		printf("[version!=INT]");
		asn1_print(&elem);
		return;
	}
	/* only handle version==0 */
	if (elem.data.integer != DEF_VERSION) {
		printf("[version(%d)!=0]", elem.data.integer);
		return;
	}
	length -= count;
	np += count;

	/* Community (String) */
	count = asn1_parse(np, length, &elem);
	if (elem.type != BE_STR) {
		printf("[comm!=STR]");
		asn1_print(&elem);
		return;
	}
	/* default community */
	if (strncmp(elem.data.str, DEF_COMMUNITY, sizeof(DEF_COMMUNITY)-1))
		/* ! "public" */
		printf("C=%.*s ", elem.asnlen, elem.data.str);
	length -= count;
	np += count;

	/* PDU (Context) */
	count = asn1_parse(np, length, &pdu);
	if (pdu.type != BE_PDU) {
		printf("[no PDU]");
		return;
	}
	if (count < length)
		printf("[%d extra after PDU]", length - count);
	asn1_print(&pdu);
	/* descend into PDU */
	np = (u_char *)pdu.data.raw;
	length = snapend - np;

	switch (pdu.id) {
	case TRAP:
		trap_print(np, length);
		break;
	case GETREQ:
	case GETNEXTREQ:
	case GETRESP:
	case SETREQ:
		snmppdu_print(pdu.id, np, length);
		break;
	}
	return;
}
