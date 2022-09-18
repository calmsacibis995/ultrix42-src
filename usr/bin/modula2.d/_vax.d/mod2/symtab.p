(*#@(#)symtab.p	4.1	Ultrix	7/17/90 *)
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
$Header: symtab.p,v 1.6 84/06/06 13:03:11 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "alloc.h"
#include "decls.h"

{ symbol table }
const
    SYMTABSIZE = 1000;
var
    symTab : array [0..SYMTABSIZE] of Symbol;


procedure InitSymTab;
var
    i : integer;
begin
    for i := 0 to SYMTABSIZE do begin
	symTab[i] := nil;
    end;
    withQualList := nil;
    loopActive := false;
    generateBlockNumber := 1;
    currScope := nil;
    { builtinScope is scope with builtin identifiers }
    builtinScope := StartScope(false);
    EndScope;
    { globalScope is scope with globally exported identifiers }
    globalScope := StartScope(false);
    { leave globalScope active }
end;

function StartScope {(open : boolean) : Scope};
var
    scope : Scope;
begin
    if TraceSymtab then begin
	writeln(output,'StartScope ',generateBlockNumber:1);
    end;
    new(scope);
    scope^.block := generateBlockNumber;
    scope^.open := open;
    scope^.enclosing := currScope;
    scope^.symbols := AddToSymbolList(nil,nil);
    currScope := scope;
    StartScope := scope;
    generateBlockNumber := generateBlockNumber + 1;
end;

procedure OpenScope {(scope : Scope; open : boolean)};
var
    newScope : Scope;
begin
    if TraceSymtab then begin
	writeln(output,'OpenScope ',scope^.block:1);
    end;
    new(newScope);
    newScope^.block := scope^.block;
    newScope^.open := open;
    newScope^.enclosing := currScope;
    currScope := newScope;
end;

procedure EndScope;
begin
    if TraceSymtab then begin
	writeln(output,'EndScope ',currScope^.block:1);
    end;
    currScope := currScope^.enclosing;
end;

function LookUpSymbol {(name : String; scope : Scope; symCase : SymbolCase)
	: Symbol};
var
    sym : Symbol;
    symName : String;
    found : boolean;
    hash : integer;
    originalScope : Scope;
begin
    originalScope := scope;
    if scope = nil then begin
	scope := currScope;
    end;
    found := false;
    repeat
	if TraceSymtab then begin
	    writeln(output,'LookUpSymbol block ',scope^.block:1);
	end;
	hash := (name^.hash + scope^.block) mod (SYMTABSIZE+1);
	sym := symTab[hash];
	while not found and (sym <> nil) do begin
	    if TraceSymtab then begin
		write(output,'LookUpSymbol ');
		WriteString(output,sym^.name);
		write(output,' ',sym^.block:1,' ');
		WriteString(output,name);
		writeln(output,' ',scope^.block:1,' ',hash:1);
	    end;
	    if sym^.block <> scope^.block then begin
	    end else if sym^.name = name then begin
		found := true;
	    end else if not standardKeywordFlag and
		    ((sym^.symCase = ANYCASE) or (symCase = ANYCASE))
	    then begin
		found := EqualAnyCase(sym^.name,name);
	    end;
	    if not found then begin
		sym := sym^.nextInTable;
	    end;
	end;
	if not found then begin
	    if scope^.open then begin
		scope := scope^.enclosing;
	    end else begin
		scope := nil;
	    end;
	end;
    until found or (scope = nil);
    if sym <> nil then begin
	{ found it }
    end else if originalScope = nil then begin
	scope := builtinScope;
	{ check for a standard identifier in a builtin scope ignoring case }
	hash := (name^.hash + scope^.block) mod (SYMTABSIZE+1);
	sym := symTab[hash];
	while (sym <> nil) and not found do begin
	    if TraceSymtab then begin
		write(output,'LookUpSymbol ');
		WriteString(output,sym^.name);
		write(output,' ',sym^.block:1,' ');
		WriteString(output,name);
		writeln(output,' ',scope^.block:1,' ',hash:1);
	    end;
	    symName := sym^.name;
	    if sym^.block <> scope^.block then begin
		{ wrong block }
	    end else if name = symName then begin
		found := true;
	    end else begin
		found := EqualAnyCase(name,symName);
	    end;
	    if not found then begin
		sym := sym^.nextInTable;
	    end;
	end;
    end;
    if (sym = nil) and TraceSymtab then begin
	write(output,'LookUpSymbol did not find ');
	WriteString(output,name);
	if originalScope = nil then begin
	    originalScope := currScope;
	end;
	writeln(output,' ',originalScope^.block:1,' ',hash:1);
    end;
    LookUpSymbol := sym;
end;

function DefineSymbol {(var sym : Symbol; name : String; scope : Scope;
	symCase : SymbolCase) : boolean};
var
    hash : integer;
    found : boolean;
begin
    if scope = nil then begin
	scope := currScope;
    end;
    if symCase = SCOPECASE then begin
	if (scope^.block <= MAXBUILTINSCOPES) and (scope^.block <> 0) then begin
	    symCase := ANYCASE;
	end else begin
	    symCase := ONECASE;
	end;
    end;
    hash := (name^.hash + scope^.block) mod (SYMTABSIZE+1);
    sym := symTab[hash];
    found := false;
    while (sym <> nil) and not found do begin
	if sym^.block <> scope^.block then begin
	    { not right block }
	end else if sym^.name = name then begin
	    found := true;
	end else if not standardKeywordFlag and
		((sym^.symCase = ANYCASE) or (symCase = ANYCASE))
	then begin
	    found := EqualAnyCase(sym^.name,name);
	end;
	if not found then begin
	    sym := sym^.nextInTable;
	end;
    end;
    if found then begin
	{ do nothing }
    end else begin
	new(sym);
	sym^.name := name;
	sym^.block := scope^.block;
	sym^.nextInTable := symTab[hash];
	symTab[hash] := sym;
	sym^.kind := SYMNULL;
	sym^.symCase := symCase;
	if TraceSymtab then begin
	    write(output,'DefineSymbol ');
	    WriteString(output,sym^.name);
	    writeln(output,' ',sym^.block:1,' ',hash:1);
	end;
	scope^.symbols := AddToSymbolList(scope^.symbols,sym);
    end;
    DefineSymbol := not found;
end;

procedure DumpSymTab;
var
    i : integer;
    sym : Symbol;
begin
    for i := 0 to SYMTABSIZE do begin
	sym := symTab[i];
	while sym <> nil do begin
	    write(output,'hv=',i:0,' b=',sym^.block:0,' ',sym^.kind:0,' ');
	    WriteString(output,sym^.name);
	    if sym^.kind = SYMMODULE then begin
		write(output,'s=',sym^.symModule^.scope^.block:1,
		' es=',sym^.symModule^.exportScope^.block:1);
	    end;
	    writeln(output);
	    sym := sym^.nextInTable;
	end;
    end;
end;

{ QualifiedName:  Looks up a possible qualified name }
{ Removes idents from names as it searches.  Leaves extras, if any, in names }
function QualifiedName{(names : IdentList) : Symbol};
var
    sym : Symbol;
    id, idnext : IdentNode;
    scope : Scope;
    continue : boolean;
begin
    assert (names <> nil);
    assert (names^.first <> nil);
    sym := nil;
    scope := nil;		{ default scope }
    id := names^.first;
    repeat
	sym := LookUpSymbol(id^.name,scope,ONECASE);
	if sym = nil then begin
	    continue := false;
	    ErrorName(id^.name,'Symbol not found');
	end else begin
	    if sym^.kind <> SYMMODULE then begin
		continue := false;
	    end else begin
		continue := true;
		scope := sym^.symModule^.scope;
	    end;
	    { remove module name from front of list }
	    idnext := id^.next;
	    names^.first := idnext;
	    dispose(id);
	    id := idnext;
	end;
    until not continue or (id = nil);
    QualifiedName := sym;
end;

function TypeOf {(names : IdentList) : TypeNode};
var
    tn : TypeNode;
    sym : Symbol;
begin
    sym := QualifiedName(names);
    if sym = nil then begin
	tn := anyTypeNode;
    end else if names^.first <> nil then begin
	ErrorName(sym^.name,'Qualification error on type');
	tn := anyTypeNode;
    end else if (sym^.kind <> SYMTYPE) then begin
	ErrorName(sym^.name,'Must be a type');
	tn := anyTypeNode;
    end else begin
	dispose(names);
	tn := ActualType(sym^.symType);
    end;
    TypeOf := tn;
end;

function Compatible {(var dtn : TypeNode; den : ExprNode; var stn : TypeNode;
	sen : ExprNode) : TypeNode};
var
    src, dst, tn, etn : TypeNode;
begin
    tn := nil;
    src := BaseType(stn);
    dst := BaseType(dtn);
    if (src = nil) or (dst = nil) then begin
	{ not much we can do }
{ compatible if same }
    end else if dst = src then begin
	tn := dst;
{  or any type allowed }
    end else if (dst = anyTypeNode) or (src = anyTypeNode) then begin
	tn := dst;
{  or constant with integer or cardinal }
    end else if (dst = cardIntTypeNode) and
	    (src^.kind in [DTINTEGER,DTCARDINAL])
    then begin
	tn := src;
    end else if (src = cardIntTypeNode) and
	    (dst^.kind in [DTINTEGER,DTCARDINAL])
    then begin
	tn := dst;
{  or constant with char }
    end else if (dst = charConstTypeNode) and (src = charTypeNode) then begin
	tn := src;
	dtn := src;
	if den <> nil then begin
	    if den^.kind = EXPRCONST then begin
		den^.constType := src;
		den^.exprType := src;
	    end;
	end;
    end else if (src = charConstTypeNode) and (dst = charTypeNode) then begin
	tn := dst;
	stn := dst;
	if sen <> nil then begin
	    if sen^.kind = EXPRCONST then begin
		sen^.constType := dst;
		sen^.exprType := dst;
	    end;
	end;
{  or constant with real or longreal }
    end else if (dst = realConstTypeNode) and
	    ((src = realTypeNode) or (src = longrealTypeNode))
    then begin
	tn := src;
	dtn := src;
	if den <> nil then begin
	    if den^.kind = EXPRCONST then begin
		den^.constType := src;
		den^.exprType := src;
	    end;
	end;
    end else if (src = realConstTypeNode) and
	    ((dst = realTypeNode) or (dst = longrealTypeNode))
    then begin
	tn := dst;
	stn := dst;
	if sen <> nil then begin
	    if sen^.kind = EXPRCONST then begin
		sen^.constType := dst;
		sen^.exprType := dst;
	    end;
	end;
{ or string with array of char }
    end else if (src^.kind = DTSTRING) and (dst^.kind = DTARRAY) then begin
	etn := BaseType(dst^.elementType);
	if (etn^.kind = DTCHAR) and (dst^.indexType <> nil) then begin
	    if NumberOf(dst^.indexType) >= src^.stringLength then begin
		tn := dst;
		stn := dst;
		if sen <> nil then begin
		    if sen^.kind = EXPRCONST then begin
			{ set string type to match array type }
			sen^.constType^.indexType := ActualType(dst^.indexType);
			sen^.constType^.size := dst^.size;
			sen^.exprType := sen^.constType;
		    end;
		end;
	    end;
	end;
    end else if (dst^.kind = DTSTRING) and (src^.kind = DTARRAY) then begin
	etn := BaseType(src^.elementType);
	if (etn^.kind = DTCHAR) and (src^.indexType <> nil) then begin
	    if NumberOf(src^.indexType) >= dst^.stringLength then begin
		tn := src;
		dtn := src;
		if den <> nil then begin
		    if den^.kind = EXPRCONST then begin
			{ set string type to match array type }
			den^.constType^.indexType := ActualType(src^.indexType);
			den^.constType^.size := src^.size;
			den^.exprType := den^.constType;
		    end;
		end;
	    end;
	end;
{ or char constant with array of char }
    end else if (src = charConstTypeNode) and (dst^.kind = DTARRAY) then begin
	if (BaseType(dst^.elementType) = charTypeNode) and
		    (dst^.indexType <> nil)
	then begin
	    { set char constant to string type to match array type }
	    tn := NewTypeNode(DTSTRING);
	    tn^.stringLength := 1;
	    tn^.size := dst^.size;
	    tn^.alignment := dst^.alignment;
	    stn := tn;
	    if sen <> nil then begin
		if sen^.kind = EXPRCONST then begin
		    sen^.constType := tn;
		    sen^.exprType := tn;
		end;
	    end;
	end;
    end else if (dst = charConstTypeNode) and (src^.kind = DTARRAY) then begin
	if (BaseType(src^.elementType) = charTypeNode) and
		(src^.indexType <> nil)
	then begin
	    { set char to string type to match array type }
	    tn := NewTypeNode(DTSTRING);
	    tn^.stringLength := 1;
	    tn^.size := src^.size;
	    tn^.alignment := src^.alignment;
	    dtn := tn;
	    if den <> nil then begin
		if den^.kind = EXPRCONST then begin
		    den^.constType := tn;
		    den^.exprType := tn;
		end;
	    end;
	end;
{ address and cardinal or address and pointer can be intermixed }
    end else if (dst=addressTypeNode) and (src^.kind in [DTPOINTER,DTCARDINAL,DTINTEGER])
    then begin
	tn := dst;
    end else if (src=addressTypeNode) and (dst^.kind in [DTPOINTER,DTCARDINAL,DTINTEGER])
    then begin
	tn := src;
    end;
    Compatible := tn;
end;

function Assignable {(dtn : TypeNode; var stn : TypeNode; sen : ExprNode) : TypeNode};
var
    src, dst, tn : TypeNode;
    same : boolean;
    srcpn, dstpn : ParamNode;
begin
    tn := Compatible(dtn,nil,stn,sen);
    src := BaseType(stn);
    dst := BaseType(dtn);
    if (src = nil) or (dst = nil) then begin
	{ not much we can do }
    end else if tn = nil then begin
{ check integer/cardinal operation }
	if ((dst = integerTypeNode) or (dst = cardinalTypeNode)
			or (dst = cardIntTypeNode))
	    and ((src = integerTypeNode) or (src = cardinalTypeNode)
			or (src = cardIntTypeNode))
	then begin
	    tn := dst;
{ allow word and any 1-word quantity }
(* not allowed by language
	end else if (dst = wordTypeNode) and (SizeOf(src) <= WORDSIZE)
	then begin
	    tn := src;
	end else if (src = wordTypeNode) and (SizeOf(dst) <= WORDSIZE)
	then begin
	    tn := dst;
*)
{ procedure constants to procedure variables }
	end else if (dst^.kind = DTPROC) and (src^.kind = DTPROC) then begin
	    if src^.paramList = nil then begin
		srcpn := nil;
	    end else begin
		srcpn := src^.paramList^.first;
	    end;
	    if dst^.paramList = nil then begin
		dstpn := nil;
	    end else begin
		dstpn := dst^.paramList^.first;
	    end;
	    same := ActualType(dst^.funcType) = ActualType(src^.funcType);
	    while same and (srcpn <> nil) and (dstpn <> nil) do begin
		same := SameTypeParam(dstpn^.paramType,srcpn^.paramType);
		srcpn := srcpn^.next;
		dstpn := dstpn^.next;
	    end;
	    if same and (srcpn = nil) and (dstpn = nil) then begin
		tn := dtn;
	    end;
	end;
    end;
    Assignable := tn;
end;

function Passable {(dtn: TypeNode; kind : ParamKind; var stn : TypeNode; sen : ExprNode) : boolean};
var
    src, dst, tn, etn : TypeNode;
begin
    if kind = PARAMVALUE then begin
	tn := Assignable(dtn,stn,sen);
    end else if kind = PARAMVAR then begin
	src := ActualType(stn);
	dst := ActualType(dtn);
	if src = dst then begin
	    tn := src;
	end else begin
	    tn := nil;
	end;
    end else begin
	tn := nil;
    end;
    src := BaseType(stn);
    dst := BaseType(dtn);
    if (src = nil) or (dst = nil) then begin
	{ not much we can do }
    end else if tn = nil then begin
{ check pass to a word or byte }
	if (dst = wordTypeNode) and (SizeOf(src) <= WORDSIZE) then begin
	    tn := src;
	end else if (dst = byteTypeNode) and (SizeOf(src) <= BYTESIZE) then begin
	    tn := src;
{ check pass to an address or pointer }
	end else if (dst = addressTypeNode) and (src^.kind = DTPOINTER)
	then begin
	    tn := src;
	end else if (src = addressTypeNode) and (dst^.kind = DTPOINTER)
	then begin
	    tn := dst;
{ check open array }
	end else if (dst^.kind = DTARRAY) and (src^.kind = DTARRAY) then begin
	    if (dst^.indexType = nil) and
		(ActualType(dst^.elementType) = ActualType(src^.elementType))
	    then begin
		tn := src;
	    end;
	end else if (dst^.kind = DTARRAY) and (src^.kind = DTSTRING) then begin
	    etn := BaseType(dst^.elementType);
	    if (dst^.indexType = nil) and (etn^.kind = DTCHAR) then begin
		tn := src;
	    end;
	end else if (dst^.kind = DTARRAY) and (src = charConstTypeNode)
	then begin
	    etn := BaseType(dst^.elementType);
	    if (dst^.indexType = nil) and (etn^.kind = DTCHAR) then begin
		{ make char constant into a string of one character }
		tn := NewTypeNode(DTSTRING);
		tn^.stringLength := 1;
		tn^.size := CHARSIZE;
		stn := tn;
		if sen <> nil then begin
		    if sen^.kind = EXPRCONST then begin
			sen^.constType := tn;
			sen^.exprType := tn;
		    end;
		end;
	    end;
	end;
{ array of word - any src is OK }
	if (tn = nil) and (dst^.kind = DTARRAY) then begin
	    if (dst^.indexType = nil) and
			(ActualType(dst^.elementType) = wordTypeNode)
	    then begin
		tn := src;	
	    end;
	end;
{ array of byte - any src is OK }
	if (tn = nil) and (dst^.kind = DTARRAY) then begin
	    if (dst^.indexType = nil) and
			(ActualType(dst^.elementType) = byteTypeNode)
	    then begin
		tn := src;	
	    end;
	end;
    end;
    Passable := tn <> nil;
end;

function Port{(sym : Symbol; scope : Scope):Symbol};
var
    nsym, esym, msym, savenext : Symbol;
    saveblock : BlockNumber;
    enum : EnumNode;
    tn : TypeNode;
    id : IdentNode;
    mn : ModuleNode;
begin
    if DefineSymbol(nsym,sym^.name,scope,sym^.symCase) then begin
	{ copy contents of symbol.  Watch out, we need to keep next and block }
	savenext := nsym^.nextInTable;
	saveblock := nsym^.block;
	nsym^ := sym^;
	nsym^.nextInTable := savenext;
	nsym^.block := saveblock;
	if sym^.kind = SYMTYPE then begin
	    tn := BaseType(sym^.symType);
	    if tn^.kind = DTENUMERATION then begin
		enum := tn^.enumList^.first;
		while enum <> nil do begin
		    esym := Port(enum^.enumSym,scope);
		    enum := enum^.next;
		end;
	    end;
	end else if sym^.kind = SYMMODULE then begin
	    mn := sym^.symModule;
	    if mn^.unqualExports <> nil then begin
		id := mn^.unqualExports^.first;
		while id <> nil do begin
		    msym := LookUpSymbol(id^.name,mn^.exportScope,ONECASE);
		    if msym = nil then begin
			ErrorName(sym^.name,'Port: Ported symbol not found?');
		    end else begin
			msym := Port(msym,scope);
		    end;
		    id := id^.next;
		end;
	    end;
	end;
    end else begin
	{ sym is being ported, but we found nsym, so nsym is considered to }
	{ be defined first }
	CheckEqualSym(nsym,sym);
    end;
    Port := nsym;
end;

procedure CheckEqualSym{(sym1, sym2 : Symbol)};
var
    error : boolean;
begin
    error := false;
    if sym1^.kind = sym2^.kind then begin
	case sym1^.kind of
	    SYMPROC :	CheckEqualProc(sym1^.symProc,sym2^.symProc^.procType);
	    SYMTYPE :	CheckEqualType(sym1,sym2^.symType);
	    SYMCONST:	error := sym1^.symConst <> sym2^.symConst;
	    SYMVAR:	error := sym1^.symConst <> sym2^.symConst;
	    SYMMODULE:	error := sym1^.symConst <> sym2^.symConst;
	    SYMFIELD:	error := sym1^.symConst <> sym2^.symConst;
	    SYMENUM:	error := sym1^.symConst <> sym2^.symConst;
	end;
    end else begin
	error := true;
    end;
    if error then begin
	ErrorName(sym1^.name,'Ported symbol redefined in block');
    end;
end;

function AddToCodeList {(list : CodeList; newOne : CodeNode)
	: CodeList};
begin
    if list = nil then begin
	new(list);
	list^.first := nil;
	list^.last := nil;
    end;
    if newOne = nil then begin
	{ empty statement, do nothing }
    end else if list^.last = nil then begin
	newOne^.next := nil;
	list^.first := newOne;
	list^.last := newOne;
    end else begin
	newOne^.next := nil;
	list^.last^.next := newOne;
	list^.last:= newOne;
    end;
    AddToCodeList := list;
end;

function AddToSymbolList {(list : SymbolList; newOne : SymbolNode)
	: SymbolList};
begin
    if list = nil then begin
	new(list);
	list^.first := nil;
	list^.last := nil;
    end;
    if newOne = nil then begin
	{ empty statement, do nothing }
    end else if list^.last = nil then begin
	newOne^.next := nil;
	list^.first := newOne;
	list^.last := newOne;
    end else begin
	newOne^.next := nil;
	list^.last^.next := newOne;
	list^.last:= newOne;
    end;
    AddToSymbolList := list;
end;

