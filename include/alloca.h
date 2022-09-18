#include <ansi_compat.h>
#ifdef __mips
/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: alloca.h,v 2010.5.1.5 89/11/29 22:41:30 bettina Exp $ */
#ifndef __ALLOCA_H
#define __ALLOCA_H

/*
** Synopsis
**   #include <alloca.h>
**   void *alloca(integral_types);
**
** Description
**   This header is to be included if the built-in version
**   of alloca is desired. The built in version is more
**   efficient than the libc version, but it is less
**   flexible. The built-in version is implemented as an
**   operator and can only be applied to integral_types.
**   integral_types are comprised of type char, the signed
**   and unsigned integer types, and the enumerated types.
**   See also alloca(3).
*/


#define alloca  __builtin_alloca
#endif


#endif
