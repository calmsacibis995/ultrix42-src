/*
 *
 * Copyright (C) 1985 by
 * Digital Equipment Corporation, Maynard, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 10-Jul-1985
 *      DECnet-ULTRIX   V1.0
 *
 */

/* @(#)decnet_types.h	4.2		9/4/90  */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

/*
 * Common type definitions for DECnet.
 */

typedef unsigned char field8;		/* 8-bit message field */
typedef unsigned short field16;		/* 16-bit message field */
typedef unsigned int field32;		/* 32-bit message field */
typedef unsigned char byte[1];		/* single byte field */
typedef unsigned char word[2];		/* 2 byte field */
typedef unsigned char longword[4];	/* 4 bytes field */

/*
 * Macros to access 8 and 16-bit fields within messages.
 */

#define GET8(p)		(*(p)++)
#define PUT8(p,v)	(*(p)++ = (v))

#ifdef __vax
#define GET16(p)	*(field16 *)(p); (p) += sizeof(field16)
#define PUT16(p,v)	*(field16 *)(p) = (v); (p) += sizeof(field16)
#define GET32(p)	*(field32 *)(p); (p) += sizeof(field32)
#define PUT32(p,v)	*(field32 *)(p) = (v); (p) += sizeof(field32)
#define EXT8(b)		(*(u_char *)(b))
#define EXT16(b)	(*(u_short *)(b))
#define EXT32(b)	(*(u_int *)(b))
#define EXTRACT(b,t)	(*(t *)(b))
#define INS8(b,v)	(*(u_char *)(b) = (v))
#define INS16(b,v)	(*(u_short *)(b) = (v))
#define INS32(b,v)	(*(u_int *)(b) = (v))
#define INSERT(b,t,v)	(*(t *)(b) = (v))
#endif /* __vax */

#ifdef __mips
#define GET16(p)	\
	(u_short)( *(u_char *)(p) | ((u_short)*((u_char *)(p)+1)) << 8 ); \
	(p) += sizeof(field16)
#define GET32(p)			\
	  (u_long)(*((u_char *)(p))	\
	| (*(((u_char *)(p))+1)<<8)	\
	| (*(((u_char *)(p))+2)<<16)	\
	| (*(((u_char *)(p))+3)<<24));	\
	(p) += sizeof(field32)
#define PUT16(p, v)	{ u_short tmp = (v); bcopy(&tmp, (p), 2); (p) += 2; }
#define PUT16S(p, v)	{ short tmp = (v); bcopy(&tmp, (p), 2); (p) += 2; }
#define PUT32(p, v)	{ u_long  tmp = (v); bcopy(&tmp, (p), 4); (p) += 4; }
#define INSERT(b,t,v)	{ u_int tmp = (v); bcopy(&tmp, (b), sizeof(t)); }
#define INSERT16(b,v)	{ u_short tmp = (v); bcopy(&tmp, (b), sizeof(field16)); }
#define INSERT32(b,v)	{ u_int tmp = (v); bcopy(&tmp, (b), sizeof(field32)); }

#define INS8(p, v)	{ ((u_char *)(p))[0] = (v); }
#define INS16(p, v)	{ register u_char *q = (u_char *) (p); \
			  if ((int)q & 1) { \
			      register int w = (v); \
			      *q++ = w & 0xFF; w >>= 8; \
			      *q   = w & 0xFF; \
			  } else { \
			      *(u_short *)q = (v); \
			  } \
			}
#define INS32(p, v)	{ register u_char *q = (u_char *) (p); \
			  register int w = (v); \
			  switch ((int)q & 3) { \
			      case 0: \
				  *(u_long *)q = w; \
				  break; \
			      case 2: \
				  *(u_short *)q = w & 0xFFFF; w >>= 16; q += 2; \
				  *(u_short *)q = w & 0xFFFF; \
				  break; \
			      default: \
				  *q++ = w & 0xFF; w >>= 8; \
				  *(u_short *)q = w & 0xFFFF; w >>= 16; q += 2; \
				  *q   = w & 0xFF; \
				  break; \
			  } \
			}
#define	EXT8(p)		*(p)
#define EXT16(p)	(u_short)(*((u_char *)(p)) | (*(((u_char *)(p))+1)<<8))
#define EXT32(p)			\
	  (u_long)(*((u_char *)(p))	\
	| (*(((u_char *)(p))+1)<<8)	\
	| (*(((u_char *)(p))+2)<<16)	\
	| (*(((u_char *)(p))+3)<<24))
#endif /* __mips */

/*
 * Counter access macros
 */
#define INC32(c)	((c) != (unsigned) 0xffffffff ? ++(c) : (c))
#define INC16(c)	((c) != (unsigned) 0xffff ? ++(c) : (c))
#define INC8(c)		((c) != (unsigned) 0xff ? ++(c) : (c))
#define ADD32(c,a)	(((c)+(a)) >= (c) ? ((c) += (a)) : ((c) = (unsigned) 0xffffffff))
#define ADD16(c,a)	(((c)+(a)) >= (c) ? ((c) += (a)) : ((c) = (unsigned) 0xffff))
#define ADD8(c,a)	(((c)+(a)) >= (c) ? ((c) += (a)) : ((c) = (unsigned) 0xff))

/*
 * Return pointer past end of data in MBUF
 */
#define mtoe(x,t)	((t)((char *)(x) + (x)->m_off + (x)->m_len))
