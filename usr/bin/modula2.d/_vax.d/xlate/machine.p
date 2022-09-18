(*#@(#)machine.p	4.1	Ultrix	7/17/90 *)
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
$Header: machine.p,v 1.3 84/05/19 11:33:56 powell Exp $
 ****************************************************************************)
#include "globals.h"

procedure Op {(v : ShortString)};
var i : integer;
begin
    i := 4;
    while v[i] <> ' ' do begin
	i := i + 1;
    end;
    write(output,TAB,v:i-1,TAB);
end;

procedure S{(v : ShortString)};
var
    i : integer;
begin
    i := SHORTSTRINGSIZE;
    while (i > 1) and (v[i] = ' ') do begin
	i := i - 1;
    end;
    write(output,v:i);
end;

procedure ErrorI{(value, width: integer)};
begin
    WriteI(errorfile,value,width);
end;
procedure ErrorC{(value:char; width: integer)};
begin
    WriteC(errorfile,value,width);
end;
procedure ErrorStr{(value:ErrorString; width: integer)};
begin
    WriteS(errorfile,value,width);
end;
procedure ErrorEOL;
begin
    WriteEOL(errorfile);
end;

procedure RefillBuffer;
begin
    numchread := Read(inputfile,buff,BUFFSIZE); { fill buffer }
    chptr := 0;
    if numchread <= 0 then begin
	buff[1] := '?';	{ end of file reached }
	if EOF then begin
	    { second time is an error }
	    Error('*** Tried to read past end of file');
	    halt;
	end else begin
	    EOF := true;
	end;
    end;
end;

procedure setoptions;
var i, nexti, j, k : integer;
    str, nstr : FileName;
    endofoptions : boolean;
begin
{ initialize input file so a read will be done first time }
    str := STANDARDERROR;
    errorfile := Create(str);
    EOF := false;
    EOLN := false;
    chptr := 1;
    numchread := 0;
    str := STANDARDINPUT;
    inputfile := Open(str);
    i:=1;
    endofoptions := false;
    while (i < argc) and not endofoptions do begin
	{ check for non-trivial flag }
	argv(i,str);
	if (str[1] = '-') and (str[2] <> ' ') then begin
	    nexti := i + 1;
	    if nexti < argc then argv(nexti,nstr) else nstr := '  ';
	    j:=2;
	    while str[j] <> ' ' do  begin
		if str[j] ='e' then begin
		    ECHOPCODE := TRUE
		end else if str[j] ='r' then begin
		    RUNIDS := FALSE
		end else if str[j] ='v' then begin
		    VERBOSE := TRUE
		end else if str[j] ='p' then begin
		    PRINTNAMES := TRUE
		end else begin
		    ErrorStr('invalid option: ',16);
		    ErrorC(str[j],1);
		    ErrorEOL;
		end;
		j := j + 1;
	    end;
	    i := nexti;
	end else begin
	    endofoptions := true;
	end;
    end;
    if (argc > i) then begin
	{ use specified file }
	argv(i,str);
	{ terminate file name with a null }
	j := 1;
	while str[j] <> ' ' do j := j + 1;
	str[j] := chr(0);
	inputfile := Open(str);
	if inputfile = 0 then begin
	    ErrorStr('Cannot open input:',18);
	    for k := 1 to j-1 do begin
		ErrorC(str[k],1);
	    end;
	    ErrorEOL;
	    halt;
	end;
    end;
    dumch := nextchar;
#include "readch.i";
    i := i + 1;
    if (argc > i) then begin
	{ use specified file }
	argv(i,str);
	rewrite(output,str);
    end;
end;
