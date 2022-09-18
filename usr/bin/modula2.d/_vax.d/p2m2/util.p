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
$Header: util.p,v 1.4 84/05/19 11:43:37 powell Exp $
 ****************************************************************************)
(* "util.p" -- miscellaneous translation "helper" routines *)

#include "tokens.h"
#include "stringtab.h"
#include "error.h"
#include "globals.h"
#include "scanner.h"
#include "util.h"

type
    FuncPtr = ^FuncRec;
    FuncRec = record
	next : FuncPtr;
	name : String;
	params : String;
	containing : FuncPtr;
    end;
    FuncListRec = record
	first, last : FuncPtr;
    end;
    PortPtr = ^PortRec;
    PortRec = record
	next : PortPtr;
	name : String;
    end;
    PortListRec = record
	first, last : PortPtr;
    end;
var
    semiColonFlag : boolean;
    nestLevel : integer;
    saveBuffer : array [1..1000] of char;
    saveBufferIndex : integer;
    exportList, importList : PortListRec;
    funcList : FuncListRec;
    currFunction : FuncPtr;

procedure SourceError(var keyword: ShortString);
var i : integer;
begin
    write(outfile,'(*! p2m2: ');
    i := 1;
    while (i < SHORTSTRINGSIZE) and (keyword[i] <> chr(0)) do begin
	write(outfile,keyword[i]);
	i := i + 1;
    end;
    write(outfile,'!*)');
end;

procedure PrintStringConst(str: String);
begin
    WriteStringConst(outfile,str);
    semiColonFlag := false;
end;

procedure PrintString(var keyword: ShortString);
var i : integer;
begin
    i := 1;
    while (i < SHORTSTRINGSIZE) and (keyword[i] <> chr(0)) do begin
	OutChar(keyword[i]);
	i := i + 1;
    end;
    semiColonFlag := false;
    spaceFlag := TRUE;
end;

procedure PrintKeyword(var keyword: ShortString);
var i : integer;
    c : char;
begin
    if not spaceFlag then begin
	OutChar(' ');
    end;
    i := 1;
    while (i < SHORTSTRINGSIZE) and (keyword[i] <> chr(0)) do begin
	c := keyword[i];
	if not standardFlag and (c >= 'A') and (c <= 'Z') then begin
	    c := chr(ord(c) - ord('A') + ord('a'));
	end;
	OutChar(c);
	i := i + 1;
    end;
    semiColonFlag := false;
    spaceFlag := false;
end;

procedure PrintKeywordP(keyword:ShortString);
var
    i : integer;
begin
    i := 1;
    while (i < SHORTSTRINGSIZE) and (keyword[i] <> '$') do begin
	i := i + 1;
    end;
    keyword[i] := chr(0);
    PrintKeyword(keyword);
end;

procedure PrintSemi;
begin
    if not semiColonFlag then begin
	OutChar(';');
    end;
    semiColonFlag := true;
    spaceFlag := TRUE;
end;

procedure PrintIdent(tokStr: String);
begin
    semiColonFlag := false;
    if not spaceFlag then begin
	OutChar(' ');
    end;
    OutString(tokStr);
    spaceFlag := false;
end;


procedure PrintConst(con: String);
begin
    semiColonFlag := false;
    OutString(con);
end;


procedure InitPass0;
begin
    rewrite(outfile, TEMPFILE);
    nestLevel := 0;
    spaceFlag := TRUE;
    outFile := TRUE;
    outString := FALSE;
    exportList.first := nil;
    exportList.last := nil;
    importList.first := nil;
    importList.last := nil;
    funcList.first := nil;
    funcList.last := nil;
end;

procedure InitPass1;
begin
    rewrite(outfile, TEMPFILE);
    nestLevel := 0;
    spaceFlag := TRUE;
    outFile := TRUE;
    outString := FALSE;
    importList.first := nil;
    importList.last := nil;
end;

procedure PrintList(list : PortListRec);
var
    pp : PortPtr;
    i : integer;
begin
    write(outfile,'	');
    pp := list.first;
    i := 0;
    while (pp <> nil) do begin
	i := i + 1;
	OutString(pp^.name);
	pp := pp^.next;
	if pp <> nil then begin
	    if i = 5 then begin
		i := 0;
		writeln(outfile,',');
		write(outfile,'	'); { it's a tab }
	    end else begin
		write(outfile,', ');
	    end;
	end;
    end;
    writeln(outfile,';');
end;

procedure Pass2;
var fName: FileName;
    i, after: integer;
    c : char;
begin
    StringToFileName(moduleName, fName);

    after := 0;
    while fName[after] <> ' ' do begin
	after := after + 1;
    end;
    
    if sourceFileType = DEFNFILE then begin
	fName[after]   := '.';
	fName[after+1] := 'd';
	fName[after+2] := 'e';
	fName[after+3] := 'f';
    end else begin
	fName[after]   := '.';
	fName[after+1] := 'm';
	fName[after+2] := 'o';
	fName[after+3] := 'd';
    end;
    rewrite(outfile, fName);
    reset(infile, TEMPFILE);
    case sourceFileType of
	DEFNFILE: begin
	    PrintKeywordP('DEFINITION MODULE $');
	end;
	IMPLFILE: begin
	    PrintKeywordP('IMPLEMENTATION MODULE $');
	end;
	PROGFILE: begin
	    PrintKeywordP('MODULE $');
	end;
    end;
    for i := 0 to after-1 do begin
	write(outfile,fName[i]);
    end;
    writeln(outfile,';');
    if importList.first <> nil then begin
	spaceFlag := true;
	PrintKeywordP('IMPORT$');
	writeln(outfile);
	PrintList(importList);
    end;
    if exportList.first <> nil then begin
	spaceFlag := true;
	PrintKeywordP('EXPORT$');
	writeln(outfile);
	PrintList(exportList);
    end;

    { copy infile to outfile }
    while not eof(infile) do begin
	while not eoln(infile) do begin
	    read(infile,c);
	    write(outfile,c);
	end;
	readln(infile);
	writeln(outfile);
    end;
    spaceFlag := TRUE;
    PrintKeywordP('END $');
    for i := 0 to after-1 do begin
	write(outfile,fName[i]);
    end;
    writeln(outfile,'.');
end;

procedure AddPort(var list : PortListRec; name : String);
var
    pp : PortPtr;
begin
    new(pp);
    pp^.name := name;
    pp^.next := nil;
    if list.first = nil then begin
	list.first := pp;
    end else begin
	list.last^.next := pp;
    end;
    list.last := pp;
end;

procedure ProcessInclude(name: String);
var
    fileName : FileName;
    i, j : integer;
    importName : String;
begin
    StringToFileName(name,fileName);
    i := 1;
    while fileName[i] <> ' ' do begin
	i := i + 1;
    end;
    if (fileName[i-1] = 'h') and (fileName[i-2] = '.') then begin
	i := i - 2;
    end;
    for j := 0 to i-1 do begin
	AddChar(fileName[j]);
    end;
    importName := NewString;
    AddPort(importList,importName);
end;

procedure CheckExport(name : String);
begin
    if (nestLevel = 0) and (sourceFileType = DEFNFILE) then begin
	AddPort(exportList,name);
    end;
end;

procedure EnsureSpace;
begin
end;

procedure OutChar{(c : char)};
begin
    if outFile then begin
	write(outfile,c);
    end;
    if outString then begin
	saveBufferIndex := saveBufferIndex + 1;
	saveBuffer[saveBufferIndex] := c;
    end;
end;

procedure EchoChar{(c : char)};
begin
    if outString then begin
	saveBufferIndex := saveBufferIndex + 1;
	saveBuffer[saveBufferIndex] := c;
    end;
end;

procedure SetOutput{(toFile, toString : boolean)};
begin
    if toString then begin
	saveBufferIndex := 0;
    end;
    outString := toString;
    outFile := toFile;
end;

procedure DefineFunction(name : String);
var
    func : FuncPtr;
    found : boolean;
begin
    func := funcList.first;
    found := false;
    if nestLevel = 0 then begin
	while (func <> nil) and not found do begin
	    if func^.name = name then begin
		found := true;
	    end else begin
		func := func^.next;
	    end;
	end;
    end;
    if not found then begin
	new(func);
	func^.name := name;
	func^.params := nil;
	func^.next := nil;
	func^.containing := currFunction;
	if funcList.first = nil then begin
	    funcList.first := func;
	end else begin
	    funcList.last^.next := func;
	end;
	funcList.last := func;
	(*
	write(output,'Define new function: ');
	WriteString(output,name);
	writeln(output);
	*)
    end;
    currFunction := func;
    nestLevel := nestLevel + 1;
    SetOutput(false,true);
end;

procedure DefineParameters(keep : boolean);
var
    i : integer;
begin
    (*
    write(output,'Define parameters: ',keep,' ');
    WriteString(output,currFunction^.name);
    *)
    SetOutput(true,false);
    if keep then begin
	for i := 1 to saveBufferIndex do begin
	    OutChar(saveBuffer[i]);
	end;
	if currFunction^.params <> nil then begin
	    write(output,'Parameters specified twice? ');
	    WriteString(output,currFunction^.name);
	    write(output,' old = ');
	    WriteString(output,currFunction^.name);
	    writeln(output,' new = ',saveBuffer:saveBufferIndex);
	end else begin
	    for i := 1 to saveBufferIndex do begin
		AddChar(saveBuffer[i]);
	    end;
	    currFunction^.params := NewString; 
	    write(output,' save ');
	    WriteString(output,currFunction^.params);
	end;
    end else begin
	if currFunction^.params <> nil then begin
	    write(output,' use ');
	    OutString(currFunction^.params);
	    WriteString(output,currFunction^.params);
	end else begin
	    for i := 1 to saveBufferIndex do begin
		OutChar(saveBuffer[i]);
	    end;
	    write(output,' none');
	end;
    end;
    writeln(output);
    SetOutput(true,false);
end;

procedure CheckFunction(name : String);
begin
    if currFunction = nil then begin
    end else if name = currFunction^.name then begin
	PrintKeywordP('(*!RETURN!*)$');
    end;
end;

procedure EndFunction(name : String);
begin
    nestLevel := nestLevel - 1;
    currFunction := currFunction^.containing;
end;

procedure SetProgFile;
begin
    sourceFileType := PROGFILE;
end;
