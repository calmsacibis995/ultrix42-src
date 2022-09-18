/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: uwri.h,v 2010.2.1.5 89/11/29 22:39:20 bettina Exp $ */

procedure inituwrite(
	var ObjectName : Filename);
  external;

function idlen(
	var Id	    : Identname)
   : integer;
  external;

procedure uwrite (var U: Bcrec);
  external;

function getdtyname(
	   Dtyp	    : Datatype)
   : char;
  external;

function getmtyname(
	   Mtyp	    : Memtype)
   : char;
  external;

procedure ucoid(
	    Tag	     : Identname);
  external;

procedure ucofname(
	    Fnam     : Filename);
  external;

procedure stopucode;
  external;

procedure uputinit(
	var ObjectName : Filename);
  external;

procedure uputint(
	i: integer);
  external;

procedure uputkill;
    external;

procedure uputclose;
    external;

PROCEDURE Ubittobyte (VAR U: Bcrec);
  external;

procedure Set_u_indent(lev: integer);
  external;
