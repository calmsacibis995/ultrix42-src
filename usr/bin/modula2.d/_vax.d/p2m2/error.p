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
$Header: error.p,v 1.3 84/05/19 11:42:18 powell Exp $
 ****************************************************************************)
#include "stringtab.h"
#include "tokens.h"
#include "globals.h"
#include "error.h"
#include "scanner.h"

procedure MakeErrorString(var str : String; string : ShortString);
begin
    AddText(string);
    str := NewString;
end;

procedure ErrorAll(name : String; msg : ErrorString; number : integer;
	printNumber : boolean);
var
    i, j : integer;
begin
    write(output,'!!! ');
    if readingFile and (inFile <> nil) then begin
	if inFile^.name = nil then begin
	    write(output,'input');
	end else begin
	    WriteString(output,inFile^.name);
	end;
	write(output, ', line ',inFile^.lineNumber:1,': ');
    end else begin
	if currFile = nil then begin
	    write(output,'input');
	end else begin
	    WriteString(output,currFile);
	end;
	write(output, ', line ',currLine:1,': ');
    end;
    if name <> nil then begin
	WriteString(output,name);
	write(output,': ');
    end;
    i := ERRORSTRINGSIZE;
    while (i > 1) and (msg[i] = ' ') do begin
	i := i - 1;
    end;
    for j := 1 to i do begin
	write(output,msg[j]);
    end;
    if printNumber then begin
	write(output,number:1);
    end;
    writeln(output);
    if readingFile and (inFile <> nil) then begin
	LineWrite;
	for i := 1 to inFile^.ptr-1 do begin
	    if inFile^.line[i] = TAB then begin
		write(output,TAB);
	    end else begin
		write(output,' ');
	    end;
	end;
	writeln(output,'^');
    end;
    numberOfErrors := numberOfErrors + 1;
end;

procedure ErrorName{(name : String; msg : ErrorString)};
begin
    ErrorAll(name,msg,0,false);
end;

procedure ErrorNumber{(msg : ErrorString; number : integer)};
begin
    ErrorAll(nil,msg,number,true);
end;

procedure Error{(msg : ErrorString)};
begin
    ErrorAll(nil,msg,0,false);
end;

procedure GeneralError(fileName : String; lineNumber : integer;
    name : String; msg : ErrorString; number : integer; printNumber : boolean);
var
    i, j : integer;
begin
    write(output,'!!! ');
    WriteString(output,fileName);
    write(output,', line ',lineNumber:1,': ');
    if name <> nil then begin
	WriteString(output,name);
	write(output,': ');
    end;
    i := ERRORSTRINGSIZE;
    while (i > 1) and (msg[i] = ' ') do begin
	i := i - 1;
    end;
    for j := 1 to i do begin
	write(output,msg[j]);
    end;
    if printNumber then begin
	write(output,number:1);
    end;
    writeln(output);
    numberOfErrors := numberOfErrors + 1;
end;
