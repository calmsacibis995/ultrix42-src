(*#@(#)stringtab.p	4.1	Ultrix	7/17/90 *)
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
$Header: stringtab.p,v 1.5 84/05/19 11:44:53 powell Exp $
 ****************************************************************************)
#include "globals.h"

var
    stringTab : StringBlock;
    stringTabIndex, saveStringTabIndex : StringIndex;
    stringHash : array [0..STRINGHASHSIZE] of StringEntry;

procedure ExpandStringTable;
var
    newBlock : StringBlock;
    newTabIndex, i, currStringSize : StringIndex;
begin
    new(newBlock);
    newTabIndex := 0;
    { copy in any partial string in old table }
    currStringSize := stringTabIndex - saveStringTabIndex;
    for i := 0 to currStringSize-1 do begin
	newBlock^.s[newTabIndex] := stringTab^.s[saveStringTabIndex+i];
	newTabIndex := newTabIndex + 1;
    end;
    newBlock^.next := stringTab;
    stringTab := newBlock;
    stringTabIndex := newTabIndex;
    saveStringTabIndex := 0;
end;

procedure InitStringTable;
var sh : integer;
begin
    for sh := 0 to STRINGHASHSIZE do begin
	stringHash[sh] := nil;
    end;
    stringTabIndex := 0;
    saveStringTabIndex := 0;
    stringTab := nil;
    ExpandStringTable;
end;

function NewString{: String; external};
var
    newStr : StringEntry;
    i, length, hash : integer;
    found : boolean;
    ordc : integer;
begin
    hash := 0;
    for i := saveStringTabIndex to stringTabIndex-1 do begin
	if stringTab^.s[i] in [ord('A')..ord('Z')] then begin
	    ordc := stringTab^.s[i] - ord('A') + ord('a');
	end else begin
	    ordc := stringTab^.s[i];
	end;
	hash := (hash * 12345 + ordc) mod 111111;
    end;
    length := stringTabIndex - saveStringTabIndex;
    newStr := stringHash[hash mod STRINGHASHSIZE];
    found := false;
    while (newStr <> nil) and not found do begin
	if newStr^.hash = hash then begin
	    if newStr^.length = length then begin
		found := true;
		i := 0;
		while found and (i < length) do begin
		    if newStr^.block^.s[newStr^.index+i]
			    <> stringTab^.s[saveStringTabIndex+i]
		    then begin
			found := false;
		    end;
		    i := i + 1;
		end;
	    end;
	end;
	if not found then begin
	    newStr := newStr^.next;
	end;
    end;
    if not found then begin
	new(newStr);
	newStr^.next := stringHash[hash mod STRINGHASHSIZE];
	stringHash[hash mod STRINGHASHSIZE] := newStr;
	newStr^.block := stringTab;
	newStr^.index := saveStringTabIndex;
	newStr^.length := length;
	newStr^.hash := hash;
	saveStringTabIndex := stringTabIndex;
    end else begin
	stringTabIndex := saveStringTabIndex;
    end;
    NewString := newStr;
end;

function GetCharX{(s : String; charNum : StringLength) : character};
begin
    if charNum >= s^.length then begin
	Error('GetChar: String table error?');
	exit(999);
    end;
    GetCharX := s^.block^.s[s^.index+charNum];
end;

function GetChar{(s : String; charNum : StringLength) : char};
begin
    if charNum >= s^.length then begin
	Error('GetChar: String table error?');
	exit(999);
    end;
    GetChar := chr(s^.block^.s[s^.index+charNum]);
end;

procedure AddCharX{(c : character)};
begin
    if stringTabIndex >= STRINGBLOCKSIZE then begin
	ExpandStringTable;
    end;
    stringTab^.s[stringTabIndex] := c;
    stringTabIndex := stringTabIndex + 1;
end;

procedure AddChar{(c : char)};
begin
    if stringTabIndex >= STRINGBLOCKSIZE then begin
	ExpandStringTable;
    end;
    stringTab^.s[stringTabIndex] := ord(c);
    stringTabIndex := stringTabIndex + 1;
end;

procedure AddText{(s : ShortString)};
var
    i : integer;
begin
    i := 1;
    while s[i] <> ' ' do begin
	if stringTabIndex >= STRINGBLOCKSIZE then begin
	    ExpandStringTable;
	end;
	stringTab^.s[stringTabIndex] := ord(s[i]);
	stringTabIndex := stringTabIndex + 1;
	i := i + 1;
    end;
end;

procedure AddString{(str : String)};
var
    i : integer;
begin
    for i := 0 to str^.length-1 do begin
	if stringTabIndex >= STRINGBLOCKSIZE then begin
	    ExpandStringTable;
	end;
	stringTab^.s[stringTabIndex] := str^.block^.s[str^.index+i];
	stringTabIndex := stringTabIndex + 1;
    end;
end;

procedure WriteString{(var f : text; s : String); external};
var
    i : integer;
begin
    if s <> nil then begin
	for i := 0 to s^.length-1 do begin
	    if s^.block^.s[s^.index+i] in [ord(' ')..ord('~')] then begin
		write(f,chr(s^.block^.s[s^.index+i]));
	    end else begin
		write(f,'\',s^.block^.s[s^.index+i]:1 oct);
	    end;
	end;
    end;
end;

procedure WriteStringConst{(var f : text; s : String); external};
var
    i : integer;
begin
    if s <> nil then begin
	for i := 0 to s^.length-1 do begin
	    if s^.block^.s[s^.index+i] = ord('''') then begin
		write(f,'''''');
	    end else if s^.block^.s[s^.index+i] = ord('\') then begin
		write(f,'\\');
	    end else if s^.block^.s[s^.index+i] in [ord(' ')..ord('~')]
	    then begin
		write(f,chr(s^.block^.s[s^.index+i]));
	    end else if s^.block^.s[s^.index+i] = LINEFEEDCHAR then begin
		write(f,'\n');
	    end else if s^.block^.s[s^.index+i] = TABCHAR then begin
		write(f,'\t');
	    end else if s^.block^.s[s^.index+i] = BACKSPACECHAR then begin
		write(f,'\b');
	    end else begin
		write(f,'\',s^.block^.s[s^.index+i]:1,'\');
	    end;
	end;
    end;
end;

procedure StringToFileName{(s : String; var fn : FileName); external};
var
    i : integer;
begin
    fn := '  ';
    for i := 0 to s^.length-1 do begin
	fn[i] := chr(s^.block^.s[s^.index+i]);
    end;
end;

procedure DumpStringTab;
var
    i : integer;
    str : StringEntry;
begin
    for i := 0 to STRINGHASHSIZE do begin
	str := stringHash[i];
	while str <> nil do begin
	    write(output,'hv=',i:0,':');
	    WriteString(output,str);
	    writeln(output);
	    str := str^.next;
	end;
    end;
end;

function EqualAnyCase{(a,b : String) : boolean};
var
    equal : boolean;
    i : integer;
    ac, bc : character;
begin
    equal := (a^.hash = b^.hash) and (a^.length = b^.length);
    if equal then begin
	i := 0;
	while equal and (i<a^.length) do begin
	    ac := a^.block^.s[a^.index+i];
	    if ac in [ord('a')..ord('z')] then begin
		ac := ac-ord('a')+ord('A');
	    end;
	    bc := b^.block^.s[b^.index+i];
	    if bc in [ord('a')..ord('z')] then begin
		bc := bc-ord('a')+ord('A');
	    end;
	    equal := ac = bc;
	    i := i + 1;
	end;
    end;
    EqualAnyCase := equal;
end;
