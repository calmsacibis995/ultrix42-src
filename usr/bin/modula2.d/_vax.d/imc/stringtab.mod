(*#@(#)stringtab.mod	4.1	Ultrix	7/17/90 *)
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
$Header: stringtab.mod,v 1.3 84/05/19 11:34:58 powell Exp $
 ****************************************************************************)
implementation module stringtab;
from io import writec, writef, output, terminal;
type
    CharSet = set of char;
var
    stringTab : StringBlock;
    stringTabIndex, saveStringTabIndex : StringIndex;
    stringHash : array [0..STRINGHASHSIZE] of StringEntry;

procedure ExpandStringTable();
var
    newBlock : StringBlock;
    newTabIndex, currStringSize : StringIndex;
    i : integer;
begin
    new(newBlock);
    newTabIndex := 0;
    (* copy in any partial string in old table *)
    currStringSize := stringTabIndex - saveStringTabIndex;
    for i := 0 to currStringSize-1 do
	newBlock^.s[newTabIndex] := stringTab^.s[saveStringTabIndex+cardinal(i)];
	newTabIndex := newTabIndex + 1;
    end;
    newBlock^.next := stringTab;
    stringTab := newBlock;
    stringTabIndex := newTabIndex;
    saveStringTabIndex := 0;
end ExpandStringTable;

procedure NewString(): String;
var
    newStr : StringEntry;
    i, length, hash : cardinal;
    found : boolean;
    ordc : cardinal;
begin
    hash := 0;
    for i := saveStringTabIndex to stringTabIndex-1 do
	if stringTab^.s[i] in CharSet{'A'..'Z'} then
	    ordc := ord(stringTab^.s[i]) - ord('A') + ord('a');
	else
	    ordc := ord(stringTab^.s[i]);
	end;
	hash := (hash * 12345 + ordc) mod 111111;
    end;
    length := stringTabIndex - saveStringTabIndex;
    newStr := stringHash[hash mod STRINGHASHSIZE];
    found := false;
    while (newStr <> nil) and not found do
	if newStr^.hash = hash then
	    if newStr^.length = length then
		found := true;
		i := 0;
		while found and (i < length) do
		    if newStr^.block^.s[newStr^.index+i]
			    <> stringTab^.s[saveStringTabIndex+i]
		    then
			found := false;
		    end;
		    i := i + 1;
		end;
	    end;
	end;
	if not found then
	    newStr := newStr^.next;
	end;
    end;
    if not found then
	new(newStr);
	newStr^.next := stringHash[hash mod STRINGHASHSIZE];
	stringHash[hash mod STRINGHASHSIZE] := newStr;
	newStr^.block := stringTab;
	newStr^.index := saveStringTabIndex;
	newStr^.length := length;
	newStr^.hash := hash;
	saveStringTabIndex := stringTabIndex;
    else
	stringTabIndex := saveStringTabIndex;
    end;
    return newStr;
end NewString;

procedure GetChar(s : String; charNum : StringLength) : char;
begin
    if charNum >= s^.length then
	writef(terminal,'GetChar: String table error?');
	halt;
    end;
    return s^.block^.s[s^.index+charNum];
end GetChar;

procedure AddChar(c : char);
begin
    if stringTabIndex >= STRINGBLOCKSIZE then
	ExpandStringTable;
    end;
    stringTab^.s[stringTabIndex] := c;
    stringTabIndex := stringTabIndex + 1;
end AddChar;

procedure AddText(s : array of char);
var
    i : cardinal;
begin
    i := 0;
    while (i < number(s)) and (s[i] # 0C) do
	if stringTabIndex >= STRINGBLOCKSIZE then
	    ExpandStringTable;
	end;
	stringTab^.s[stringTabIndex] := s[i];
	stringTabIndex := stringTabIndex + 1;
	i := i + 1;
    end;
end AddText;

procedure AddString(str : String);
var
    i : cardinal;
begin
    for i := 0 to str^.length-1 do
	if stringTabIndex >= STRINGBLOCKSIZE then
	    ExpandStringTable;
	end;
	stringTab^.s[stringTabIndex] := str^.block^.s[str^.index+i];
	stringTabIndex := stringTabIndex + 1;
    end;
end AddString;

procedure CopyString(str : String; var arr : array of char);
var
    i : cardinal;
begin
    if str = nil then
	arr[0] := 0C;
    else
	for i := 0 to str^.length-1 do
	    arr[i] := str^.block^.s[str^.index+i];
	end;
	arr[str^.length] := 0C;
    end;
end CopyString;

procedure WriteString(f : file; s : String);
var
    i : cardinal;
begin
    if s <> nil then
	for i := 0 to s^.length-1 do
	    writec(f,s^.block^.s[s^.index+i]);
	end;
    end;
end WriteString;

procedure WriteStringConst(f : file; s : String);
var
    i : cardinal;
begin
    if s <> nil then
	for i := 0 to s^.length-1 do
	    if s^.block^.s[s^.index+i] = '''' then
		writef(f,'''''');
	    elsif s^.block^.s[s^.index+i] = '\\' then
		writef(f,'\\\\');
	    elsif s^.block^.s[s^.index+i] in CharSet{' '..'~'} then
		writec(f,s^.block^.s[s^.index+i]);
	    else
		writef(f,'\\%d\\',ord(s^.block^.s[s^.index+i]));
	    end;
	end;
    end;
end WriteStringConst;

procedure DumpStringTab();
var
    i : cardinal;
    str : StringEntry;
begin
    for i := 0 to STRINGHASHSIZE do
	str := stringHash[i];
	while str <> nil do
	    writef(output,'hv=%d:',i);
	    WriteString(output,str);
	    writef(output,'\n');
	    str := str^.next;
	end;
    end;
end DumpStringTab;

procedure EqualAnyCase(a,b : String) : boolean;
var
    equal : boolean;
    i : cardinal;
    ac, bc : char;
begin
    equal := (a^.hash = b^.hash) and (a^.length = b^.length);
    if equal then
	i := 0;
	while equal and (i<a^.length) do
	    ac := a^.block^.s[a^.index+i];
	    if ac in CharSet{'a'..'z'} then
		ac := chr(ord(ac)-ord('a')+ord('A'));
	    end;
	    bc := b^.block^.s[b^.index+i];
	    if bc in CharSet{'a'..'z'} then
		bc := chr(ord(bc)-ord('a')+ord('A'));
	    end;
	    equal := ac = bc;
	    i := i + 1;
	end;
    end;
    return equal;
end EqualAnyCase;

var sh : cardinal;
begin
    for sh := 0 to STRINGHASHSIZE do
	stringHash[sh] := nil;
    end;
    stringTabIndex := 0;
    saveStringTabIndex := 0;
    stringTab := nil;
    ExpandStringTable;

end stringtab.
