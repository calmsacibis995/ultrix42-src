/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: exit.h,v 2010.2.1.5 89/11/29 22:38:57 bettina Exp $ */

#if PASTEL
procedure exit_(code:integer) options(refname: "exit"); external;
#define exit exit_
#else
procedure exit(code:integer); external;
#endif
