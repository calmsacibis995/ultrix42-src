(*#@(#)symtab.mod	4.1	Ultrix	7/17/90 *)
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
$Header: symtab.mod,v 1.3 84/05/19 11:35:16 powell Exp $
 ****************************************************************************)
implementation module symtab;
from io import writef, output;
from stringtab import String, WriteString;
(* symbol table *)
const
    SYMTABSIZE = 1000;
var
    symTab : array [0..SYMTABSIZE] of Symbol;
    generateScope : cardinal;

procedure NewScope() : Scope;
begin
    generateScope := generateScope + 1;
    return generateScope;
end NewScope;

procedure LookUpSymbol(name : String; scope : Scope) : Symbol;
var
    sym : Symbol;
    symName : String;
    found : boolean;
    hash : cardinal;
begin
    if scope = 0 then
	scope := currScope;
    end;
    found := false;
    if traceSymtab then
	writef(output,'LookUpSymbol block %d\n',scope);
    end;
    hash := (name^.hash + scope) mod (SYMTABSIZE+1);
    sym := symTab[hash];
    while not found and (sym <> nil) do
	if traceSymtab then
	    writef(output,'LookUpSymbol ');
	    WriteString(output,sym^.name);
	    writef(output,' %d ',sym^.block);
	    WriteString(output,name);
	    writef(output,' %d %d\n',scope,hash);
	end;
	if (sym^.block = scope) and (sym^.name = name) then
	    found := true;
	else
	    sym := sym^.nextInTable;
	end;
    end;
    if (sym = nil) and traceSymtab then
	writef(output,'LookUpSymbol did not find ');
	WriteString(output,name);
    end;
    return sym;
end LookUpSymbol;

procedure DefineSymbol(var sym : Symbol; name : String; scope : Scope):boolean;
var
    hash : cardinal;
    found : boolean;
begin
    if scope = 0 then
	scope := currScope;
    end;
    hash := (name^.hash + scope) mod (SYMTABSIZE+1);
    sym := symTab[hash];
    found := false;
    while (sym <> nil) and not found do
	if (sym^.block = scope) and (sym^.name = name) then
	    found := true;
	else
	    sym := sym^.nextInTable;
	end;
    end;
    if found then
	(* do nothing *)
    else
	new(sym);
	sym^.name := name;
	sym^.block := scope;
	sym^.nextInTable := symTab[hash];
	symTab[hash] := sym;
	sym^.kind := SYMNULL;
	if traceSymtab then
	    writef(output,'DefineSymbol ');
	    WriteString(output,sym^.name);
	    writef(output,' %d %d\n',sym^.block,hash);
	end;
    end;
    return not found;
end DefineSymbol;

procedure DumpSymTab();
var
    i : cardinal;
    sym : Symbol;
begin
    for i := 0 to SYMTABSIZE do
	sym := symTab[i];
	while sym <> nil do
	    writef(output,'hv=%d b=%d k=%d ',i,sym^.block,ord(sym^.kind));
	    WriteString(output,sym^.name);
	    writef(output,'\n');
	    sym := sym^.nextInTable;
	end;
    end;
end DumpSymTab;

var
    i : cardinal;
begin
    for i := 0 to SYMTABSIZE do
	symTab[i] := nil;
    end;
    generateScope := 0;
    moduleScope := NewScope();
    builtinScope := NewScope();
    globalScope := NewScope();
    currScope := NewScope();

end symtab.
