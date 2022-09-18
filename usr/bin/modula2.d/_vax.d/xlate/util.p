(*#@(#)util.p	4.1	Ultrix	7/17/90 *)
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
#include "util.h"

function Int{(var opd:operandstring):integer};
begin
    Int := atoi(opd);
end;


procedure check1{(n:integer;pset:pcodetypeset)};
var
    ptype : pcodetype;
begin
    if n <> opdcount then begin
	error(0);
    end else if not (ees[top].ptype in pset) then begin
	error(2);
    end else if n > 0 then begin
	ptype := associatedType[opd11];
	if (ptype = taddress) and
		(ees[top].ptype in [taddress,tchar,tinteger,tcardinal])
	then begin
	    { ok }
	end else if ptype <> ees[top].ptype then begin
	    error(3);
	end;
    end;
end;



procedure check2{(n:integer;pset:pcodetypeset)};
var
    ptype : pcodetype;
begin
    check1(n,pset);
    if not (ees[top-1].ptype in pset) then begin
	error(4);
    end else if n > 0 then begin
	ptype := associatedType[opd11];
	if (ptype = taddress) and
		(ees[top-1].ptype in [taddress,tchar,tinteger,tcardinal])
	then begin
	    { ok }
	end else if ptype <> ees[top-1].ptype then begin
	    error(5);
	end;
    end;
end;            { end of check2 }


procedure checkconstant{(w:sizerange)};
var     k, j:integer;
begin

	case opd11 of
	   'a': begin  
		     k:=3;
		end;
	   'b': begin  
		     if (opdsizes^[3]<>1) then error(1);
		     if ( (opd^[3][1]<>'0') and (opd^[3][1]<>'1')) then error(1);
		     k:=3;
		end;
	   'c': begin
		     k:=3;
		end;
	   'n': k:=2;
	   'i': k:=3;
	   'j': k:=3;
	   'p': k:=3;
	   'R': k:=3;
	   'r': k:=3;
	   'S': begin
		    j := Int(opd^[3]);
		    if (j = 0) then begin
			k:=3;
		    end else begin
			if (j <> opdsizes^[4]) then error(1);
			k := 4;
		    end;
		end;
	   's': k := 3;
	end;
	if (opdcount<>k) then error(9);
end;    { end of checkconstant }

function NewLabel{: LabelNumber};
begin
    currentLabel := currentLabel + 1;
    NewLabel := currentLabel;
end;

function datasize{(t:pcodetype;s:integer):integer};
begin
    case t of
	taddress: datasize := ADDRSIZE; 
	tboolean: datasize := BOOLSIZE; 
	tchar   : datasize := CHARSIZE; 
	treal	: datasize := REALSIZE;
	tlongreal: datasize := 2*REALSIZE;
	tinteger: datasize := INTSIZE;       
	tcardinal: datasize := INTSIZE;       
	tproc: datasize := ADDRSIZE;       
	tset, tstring, trecord : datasize := s;        
	tundefined: datasize := s;       
    end;
end;


procedure eliminatequotes{(i:integer)};
var
    j, k, v : integer;
begin
    if (opd^[i][1]<>'''') then begin
	{ integer value }
	v := Int(opd^[i]);
	opd^[i][1] := chr(v);
	opdsizes^[i] := 1;
    end else begin
	j:=2;
	k:=0;
	while j < opdsizes^[i] do begin
	    k := k + 1;
	    if opd^[i][j]='''' then begin
		j := j + 1;
		if opd^[i][j] <> '''' then error(9);
		j := j + 1;
		opd^[i][k] := '''';
	    end else if opd^[i][j] = '\' then begin
		j := j + 1;
		if  opd^[i][j] in ['0'..'9'] then begin
		    v := 0;
		    while opd^[i][j] in ['0'..'9'] do begin
			v := v * 10 + ord(opd^[i][j]) - ord('0');
			j := j + 1;
		    end;
		    if opd^[i][j] <> '\' then begin
			error(9);
		    end else begin
			j := j + 1;
		    end;
		    opd^[i][k] := chr(v);
		end else if opd^[i][j] = 'n' then begin
		    opd^[i][k] := chr(10);
		    j := j + 1;
		end else if opd^[i][j] = 't' then begin
		    opd^[i][k] := chr(9);
		    j := j + 1;
		end else if opd^[i][j] = 'b' then begin
		    opd^[i][k] := chr(8);
		    j := j + 1;
		end else begin
		    opd^[i][k] := opd^[i][j];
		    j := j + 1;
		end;
	    end else begin
		opd^[i][k] := opd^[i][j];
		j := j + 1;
	    end;
	end;
	opdsizes^[i]:=k;
    end;
end;            { end of eliminatequotes }
	

function logbase2{(k:integer):integer};
var i, j, log2 : integer;
begin

	if (k<=0) then 
	begin
	     error(27);
	     log2:= (-1);
	end else begin
	     i:=k;
	     j:=0;
	     while not odd(i) do
	     begin
		  i:=i div 2;
		  j:=j+1;
	     end;
	     i:=i div 2;
	     if (i=0) then log2:=j else log2:= (-1);
	end;
	logbase2 := log2;
end;            { end of logbase2 }

function power{(a,b:integer):integer};
var value : integer;
begin
    if (b<0) then begin
	Error('power: exponent < 0');
    end else begin
	value := 1;
	while b <> 0 do begin
	    if odd(b) then begin
		value := Mult(value,a);
	    end;
	    b := b div 2;
	    a := Mult(a,a);
	end;
    end;
    power := value;
end;            { end of power }

function SameString{( var string1, string2 : operandstring ) : boolean};
var 
  index : integer;
begin
  index := 0;
  repeat
    index := index + 1;
  until ( string1[ index ]  <> string2[ index ] ) or (string1[ index ] = ' ');
  SameString :=  string1[ index ] = string2[ index ] ;

end; { function SameString }

procedure StringCopy {( var string1, string2 : operandstring )};
var 
  index : integer;

begin
  index := 0;
  repeat
    index := index + 1;
    string1 [ index ] := string2 [ index ];
  until string2 [ index ] = ' ' ;
end; { procedure StringCopy }

function PowerOfTwo{(n : integer) : integer};
var
    value, p : integer;
begin
    p := 0;
    value := 1;
    while (value < n) do begin
	value := Mult(value,2);
	p := p + 1;
    end;
    if value = n then begin
	PowerOfTwo := p;
    end else begin
	PowerOfTwo := -1;
    end;
end;
