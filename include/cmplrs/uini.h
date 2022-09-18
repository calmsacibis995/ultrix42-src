/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: uini.h,v 2010.2.1.5 89/11/29 22:39:14 bettina Exp $ */

procedure uini;
  external;

procedure getutabrec(
	    Uopc     : Uopcode;
	var Utabr    : Utabrec);
  external;
