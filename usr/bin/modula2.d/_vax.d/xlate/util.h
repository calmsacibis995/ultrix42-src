(*#@(#)util.h	4.1	Ultrix	7/17/90 *)
(****************************************************************************
 *									    *
 *  Copyright (c) 1984 by						    *
 *  DIGITAL EQUIPMENT CORPORATION, Maynard, Massachusetts.		    *
 *  All rights reserved.						    *
 * 									    *
 *  This software is furnished under a license and may be used and copied   *
 *  only in  accordance with  the  terms  of  such  license  and with the   *
 *  inclusion of the above copyright notice. This software or  any  other   *
 *  copies thereof may not be provided or otherwise made available to any   *
 *  other person.  No title to and ownership of  the  software is  hereby   *
 *  transferred.							    *
 * 									    *
 *  The information in this software is  subject to change without notice   *
 *  and  should  not  be  construed as  a commitment by DIGITAL EQUIPMENT   *
 *  CORPORATION.							    *
 * 									    *
 *  DIGITAL assumes no responsibility for the use  or  reliability of its   *
 *  software on equipment which is not supplied by DIGITAL.		    *
 * 									    *
$Header: util.h,v 1.3 84/05/19 11:36:30 powell Exp $
 ****************************************************************************)
function Int(var opd:operandstring):integer; external;
procedure check1(n:integer;pset:pcodetypeset); external;
procedure check2(n:integer;pset:pcodetypeset); external;
procedure checkconstant(w:sizerange); external;
function NewLabel: LabelNumber; external;
function datasize(t:pcodetype;s:integer):integer; external;
procedure eliminatequotes(i:integer); external;
function logbase2(k:integer):integer; external;
function power(a,b:integer):integer; external;
function SameString( var string1, string2 : operandstring )
	    : boolean; external;
procedure StringCopy ( var string1, string2 : operandstring ); external;
function PowerOfTwo(n : integer) : integer; external;
