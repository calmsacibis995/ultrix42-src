(*#@(#)numtab.mod	4.1	Ultrix	7/17/90 *)
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
$Header: numtab.mod,v 1.3 84/05/19 11:34:18 powell Exp $
 ****************************************************************************)
implementation module numtab;
from io import writef, output;
from symtab import currScope, DataType, builtinScope;
(* number table *)
const
    NUMTABSIZE = 1000;
var
    numTab : array [0..NUMTABSIZE] of Number;


procedure LookUpNumber(name : integer; scope : Scope) : Number;
var
    num : Number;
    numName : integer;
    found : boolean;
    hash : integer;
begin
    if scope = 0 then
	scope := currScope;
    end;
    if name <= MAXBUILTINTYPES then
	scope := builtinScope;
    end;
    found := false;
    if traceNumtab then
	writef(output,'LookUpNumber block %d\n',scope);
    end;
    hash := (name*name + integer(scope)) mod (NUMTABSIZE+1);
    num := numTab[hash];
    while not found and (num <> nil) do
	if traceNumtab then
	    writef(output,'LookUpNumber ');
	    writef(output,'%d',num^.name);
	    writef(output,' %d ',num^.block);
	    writef(output,'%d',name);
	    writef(output,' %d %d\n',scope,hash);
	end;
	if (num^.block = scope) and (num^.name = name) then
	    found := true;
	else
	    num := num^.nextInTable;
	end;
    end;
    if (num = nil) and traceNumtab then
	writef(output,'LookUpNumber did not find ');
	writef(output,'%d',name);
    end;
    if traceNumtab then
	writef(output,'LookUpNumber %d ',name);
	if num = nil then
	    writef(output,'not found\n');
	else
	    writef(output,'%x\n',integer(num^.numType));
	end;
    end;
    return num;
end LookUpNumber;

procedure DefineNumber(var num : Number; name : integer; scope : Scope):boolean;
var
    hash : integer;
    found : boolean;
begin
    if scope = 0 then
	scope := currScope;
    end;
    hash := (name*name + integer(scope)) mod (NUMTABSIZE+1);
    num := numTab[hash];
    found := false;
    while (num <> nil) and not found do
	if (num^.block = scope) and (num^.name = name) then
	    found := true;
	else
	    num := num^.nextInTable;
	end;
    end;
    if found then
	(* do nothing *)
	if traceNumtab then
	    writef(output,'DefineNumber %d ',name);
	    if num = nil then
		writef(output,'not found\n');
	    else
		writef(output,'%x\n',integer(num^.numType));
	    end;
	end;
    else
	new(num);
	num^.name := name;
	num^.block := scope;
	num^.nextInTable := numTab[hash];
	numTab[hash] := num;
	if traceNumtab then
	    writef(output,'DefineNumber ');
	    writef(output,'%d',num^.name);
	    writef(output,' %d %d\n',num^.block,hash);
	end;
    end;
    return not found;
end DefineNumber;

procedure DumpNumTab();
var
    i : cardinal;
    num : Number;
begin
    for i := 0 to NUMTABSIZE do
	num := numTab[i];
	while num <> nil do
	    writef(output,'hv=%d b=%d k=%d ',i,num^.block,ord(num^.numType^.kind));
	    writef(output,'%d',num^.name);
	    writef(output,'\n');
	    num := num^.nextInTable;
	end;
    end;
end DumpNumTab;

var
    i : cardinal;
    tn : TypeNode;
    num : Number;
    error : boolean;
begin
    for i := 0 to NUMTABSIZE do
	numTab[i] := nil;
    end;
    error := false;
    i := 1;
    new(tn);
    tn^.kind := DTINTEGER;
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;
    new(tn);
    tn^.kind := DTCHAR;
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;
    new(tn);
    tn^.kind := DTBOOLEAN;
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;
    new(tn);
    tn^.kind := DTCARDINAL;
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;
    new(tn);
    tn^.kind := DTREAL;
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;
    new(tn);
    tn^.kind := DTLONGREAL;
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;
    new(tn);
    tn^.kind := DTWORD;
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;
    new(tn);
    tn^.kind := DTBYTE;
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;
    new(tn);
    tn^.kind := DTADDRESS;
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;
    new(tn);
    tn^.kind := DTFILE;
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;
    new(tn);
    tn^.kind := DTPROCESS;
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;
    new(tn);
    tn^.kind := DTINTEGER;	(* cardint *)
    error := error or not DefineNumber(num,i,builtinScope);
    num^.numType := tn;
    i := i + 1;

    if error then
	writef(output,"Could not define builtin types\n");
	halt;
    end;
end numtab.
