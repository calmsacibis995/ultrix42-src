(*#@(#)output.p	4.1	Ultrix	7/17/90 *)
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

procedure I {(v : integer)};
begin
    write(output,v:1);
end;

procedure C {(v : char)};
begin
    write(output,v:1);
end;

procedure R {(v : Reg)};
begin
    if v in [rt0..rt9] then begin
	write(output,TRegReg(v):1);
    end else begin
	write(output,v:1);
    end;
end;

procedure X;
begin
    write(output,',');
end;

procedure L;
begin
    writeln(output);
end;

procedure Lab {(v : LabelNumber)};
begin
    write(output,'L',v:1);
end;

procedure SO{(var s:operandstring)};
var i : integer;
begin
    i := 1;
    while (s[i] <> ' ') and (i < operandsize) do begin
	i := i + 1;
    end;
    write(output,s:i-1);
end;

procedure SOEscape{(var s:operandstring; len:integer)};
var i : integer;
begin
    for i := 1 to len do begin
	if (s[i] in ['\','"']) then begin
	    write(output,'\',s[i]);
	end else if s[i] = chr(10) then begin
	    writeln(output,'\');	{ backslash linefeed }
	end else if not (s[i] in [' '..'~']) then begin
	    write(output,'\',ord(s[i]):1 oct);
	end else begin
	    write(output,s[i]:1);
	end;
    end;
end;

procedure Comm{(var s:CommName)};
var i : integer;
begin
    i := 1;
    while (s[i] <> ' ') and (i < operandsize) do begin
	i := i + 1;
    end;
    write(output,s:i-1);
end;

procedure error{(n:integer)};
var     i,j:integer;
begin

	ErrorStr('line ',5); ErrorI(line,1);
	ErrorStr(': error ',8); ErrorI(n,1);
	ErrorStr('    ',4);
	for i := 1 to pclbsize do ErrorC(pclabel[i],1);
	ErrorStr('  ',2);
	for i := 1 to 3 do ErrorC(opcodestr[i],1);
	ErrorStr('  ',2);
	for i:=1 to opdcount do begin
	     for j:=1 to opdsizes^[i] do ErrorC(opd^[i][j],1);
	     if (i<opdcount) then ErrorC(',',1);
	end;
	ErrorEOL;
	errorcount := errorcount + 1;
end;    {end of error}


procedure echopcodeline;
var i, j : integer;
begin

	{
	writeln(output,'### top = ',top:1);
	}
	if (ECHOPCODE) then begin
		write(output,'# ');
		write(output,'line ',line:1,': ');
		if pclbsize > 0 then write(output,pclabel:pclbsize);
		write(output,TAB,'  ');
		write(output,' ',opcodestr:3,' ');
		write(output,TAB,'  ');
		for i:=1 to opdcount do
		begin
		     for j:=1 to opdsizes^[i] do
			if (opd^[i][j] >= ' ') then write(output,opd^[i][j]:1);
		     if (i<opdcount) then write(output,',');
		end;
		writeln(output);
		{dumpeesstack;}
	end;
end;    { end of echopcodeline }


procedure WriteName {( n : integer )};
var
  i  :  integer;
  index  :  integer;
  found  :  boolean;
begin
    i := 0;
    found := false;
    while (i < numcomblocks) and not found do begin
	if comtable[i].block = n then begin
	   index := i;
	   found := true;
	end;
	i := i + 1;
    end;
    if not found then begin
	Error('WriteName: name not found');
    end else begin
	write(output,'_');
	i := 1;
	while comtable[index].name[i] <> ' ' do begin
	    write(output,comtable[index].name[i]);
	    i := i + 1;
	end;
    end;
end;

procedure writelabel{(k:integer)};
var     i : integer;
begin
	write(output,'L');
	for i:=2 to opdsizes^[k]  do write(output,opd^[k][i]:1);
end;

