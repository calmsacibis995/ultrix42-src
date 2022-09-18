(*#@(#)error.p	4.1	Ultrix	7/17/90 *)
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
 *									    *
 ****************************************************************************)
#include "globals.h"

procedure Error{(s:ErrorString)};
var     i,j:integer;
begin

	ErrorStr('line ',5); ErrorI(line,1);
	ErrorStr(': error ',8); ErrorStr(s,ERRORSTRINGSIZE);
	ErrorEOL;
	for i := 1 to pclbsize do ErrorC(pclabel[i],1);
	ErrorStr('  ',2);
	for i := 1 to 3 do ErrorC(opcodestr[i],1);
	ErrorStr('  ',2);
	for i:=1 to opdcount do begin
	     for j:=1 to opdsizes^[i] do ErrorC(opd^[i][j],1);
	     if (i<opdcount) then ErrorC(',',1);
	end;
	ErrorEOL;
	DumpReg;
	DumpEES;
	errorcount := errorcount + 1;
end;
