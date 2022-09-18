/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: uscan.h,v 2010.2.1.5 89/11/29 22:39:17 bettina Exp $ */

procedure Abort;
  external;

procedure openstdout(
	var F	     : text);
  external;

procedure opnstdin(
	var F	     : text);
  external;

procedure openinput(
	var F	     : text;
	    Fname    : Filename);
  external;

procedure openoutput(
	var F	     : text;
	    Fname    : Filename);
  external;

function getclock
   : integer;
  external;

function eopage(
	var Fil	    : text)
   : boolean;
  external;

procedure readpage(
	var Fil	     : text);
  external;

procedure printdate(
	var Fil	     : text);
  external;

procedure printtime(
	var Fil	     : text);
  external;

#if 0
function max(
	   I, 
	   J	    : integer)
   : integer;
  external;

function min(
	   I, 
	   J	    : integer)
   : integer;
  external;
#endif
