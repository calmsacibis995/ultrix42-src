/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: as1atof.h,v 2010.2.1.5 89/11/29 22:38:49 bettina Exp $ */
const
  maxfpstring = 32;

type
  strng = packed array [0..maxfpstring] of char; (* Standard string type.  *)

{ Must call this first to initialize the Hough/Rowen package }
procedure initializefp; external;

{ Hough/Rowen decimal ascii-to-binary conversions: strng is null-terminated,
  function returns mask of exceptions, where all except "inexact" are
  interesting:

  most     least
      ----x		inexact
      ---x-		underflow
      --x--		overflow
      -x---		divide by zero
      x----		invalid operation
}
function atofloat(var { for economy } cstring: strng; var r: integer): integer;
  external;

function atodouble(var { for economy } cstring: strng;
  var rmore, rless: integer): integer;
  external;

function atoextended(var { for economy } cstring: strng;
  var rmost, rmore, rless, rleast:
  integer): integer; external;

