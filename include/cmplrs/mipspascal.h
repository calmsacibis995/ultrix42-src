/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: mipspascal.h,v 2010.2.1.5 89/11/29 22:39:05 bettina Exp $ */
#ifdef PASTEL
/* Define all these in uppercase as well?? */
#define lbound lowerbound
#define hbound upperbound
/* use "And" instead of "and" to prevent further expansion into "andif". */
#define bitand(_a,_b) ((_a) And (_b))
#define bitxor(_a,_b) ((_a) Xor (_b))
#define bitor(_a,_b) ((_a) Or (_b))
#define bitnot(_a) (Not (_a))
#define sizeof size
#define otherwise others
#define and andif
#define or orif
type
    double = longreal;
    /* These are not part of the MIPS Pascal standard.
        long = integer;
        ushort = 0..65535;
        short = -32768..32767;
    */
#define err ttyoutput
#include "/usr/local/include/pastel/pc-compatibility.h"
#endif
#ifdef PP
/* Define all these in uppercase as well?? */
/* use "And" instead of "and" to prevent further expansion into "andif". */
#define bitand(_a,_b) ((_a) And (_b))
#define bitxor(_a,_b) ((_a) Xor (_b))
#define bitor(_a,_b) ((_a) Or (_b))
#define bitnot(_a) (Not (_a))
#define otherwise others
#define and andif
#define or orif
#define assert(_b) begin if not (_b) then assertion_error end
#define argv(_i,_s) _s := argv_string(_i)
var
  argc: external cardinal;

function argv_string (
    in i: cardinal
    ): string;
  external;
function clock (): cardinal;
  external;
function sysclock (): cardinal;
  external;
function wallclock (): cardinal;
  external;
procedure remove (
    in filename: string );
  external;
procedure assertion_error;
  external;
procedure halt;
  external;
#endif
