/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: uread.h,v 2010.2.1.5 89/11/29 22:39:16 bettina Exp $ */

procedure resetur;
  external;

PROCEDURE readuinstr (VAR U: Bcrec; strptr: stringtextptr);
  external;

procedure initur(
	var Uname    : Filename);
  external;

procedure ugetinit(
	var ObjectName : Filename);
  external;

procedure ugetclose;
  external;

function ugetint: integer;
  external;

function ugeteof: integer;
  external;

PROCEDURE Ubytetobit (VAR U: Bcrec);
  external;
