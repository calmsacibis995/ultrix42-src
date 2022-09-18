(*#@(#)porttab.mod	4.1	Ultrix	7/17/90 *)
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
$Header: porttab.mod,v 1.4 84/05/19 11:34:28 powell Exp $
 ****************************************************************************)
implementation module porttab;
from stringtab import AddText, NewString, AddChar, CopyString;
from io import writef, output;
from symtab import PortNode, ModuleNode, ParamNode, SetValue, FieldNode,
	Symbol, SymbolKind, DefineSymbol, NewScope, EnumNode, ParamKind,
	DataType, Scope, globalScope;
from list import AddToList;
const
    MODULESCOPE = 1;

var
    dumpSymLetter : array SymbolKind of char;
    printSymKind : array SymbolKind of array [1..10] of char;
    ioModule, systemModule, storageModule, bitoperationsModule, memoryModule,
	globalModule : ModuleNode;
    errorMessage : array [1..256] of char;

procedure ModuleError(msg : array of char; name : String; expMn, impMn : ModuleNode);
var
    expName, impName, idName : array [1..256] of char;
    fatal : boolean;
begin
    fatal := not autoMakeFlag or not impMn^.named or impMn^.watchErrors;
    if not expMn^.ignoreErrors or not impMn^.ignoreErrors then
	if fatal or logErrorsFlag then
	    CopyString(expMn^.name,expName);
	    CopyString(impMn^.name,impName);
	    CopyString(name,idName);
	    if not impMn^.outOfDate and impMn^.watchErrors then
		writef(output,"Could not correct inconsistency in %s\n",
		    impName);
	    end;
	    writef(output,"%s.%s: %s %s\n",expName,idName,msg,impName);
	end;
	inc(errorCount);
	impMn^.outOfDate := true;
	fatalErrorFlag := fatalErrorFlag or fatal;
    end;
end ModuleError;

procedure DefineModule(name : String; fileName : cardinal) : ModuleNode;
var
    mn : ModuleNode;
    sym : Symbol;
begin
    if DefineSymbol(sym,name,MODULESCOPE) then
	new(mn);
	mn^.name := name;
	mn^.scope := NewScope();
	mn^.exports := nil;
	mn^.defined := FALSE;
	mn^.outOfDate := FALSE;
	mn^.watchErrors := FALSE;
	mn^.ignoreErrors := FALSE;
	mn^.builtin := FALSE;
	mn^.named := FALSE;
	mn^.fileName := fileName;
	moduleList := AddToList(moduleList,mn);
	sym^.kind := SYMMODULE;
	sym^.symModule := mn;
    else
	mn := sym^.symModule;
	if (fileName <> 0) and (mn^.fileName = 0) then
	    mn^.fileName := fileName;
	end;
    end;
    return mn;
end DefineModule;

procedure DefinePort(mname, name : String; pn : PortNode;
	imported, unqual, extern : boolean; refModule : ModuleNode);
var
    mn : ModuleNode;
    sym : Symbol;
    scope : Scope;
    qpn : PortNode;
begin
    pn^.name := name;
    pn^.refModule := refModule;
    pn^.extern := extern;
    mn := DefineModule(mname,0);
    if unqual then
	scope := globalScope;
    else
	scope := mn^.scope;
    end;
    if DefineSymbol(sym,pn^.name,scope) then
	sym^.kind := pn^.kind;
	sym^.homeModule := mn;
	sym^.imported := nil;
	sym^.exported := nil;
	mn^.exports := AddToList(mn^.exports,sym);
    end;
    if tracePorttab then
	if imported then
	    writef(output,"Define import ");
	else
	    writef(output,"Define export ");
	end;
	WriteString(output,sym^.name);
	DumpPort(pn);
    end;
    if imported then
	sym^.imported := AddToList(sym^.imported,pn);
    elsif sym^.exported # nil then
	if sym^.exported^.refModule # pn^.refModule then
	    if unqual then
		ModuleError("identifier also exported unqualified from ",
		    sym^.name,sym^.exported^.refModule,pn^.refModule);
	    else
		ModuleError("identifier also exported from ",sym^.name,
		    sym^.exported^.refModule,pn^.refModule);
	    end;
	end;
    else
	sym^.exported := pn;
    end;
    if not imported and unqual then
	(* make unqualified export also be qualified export *)
	new(qpn);
	qpn^ := pn^;
	DefinePort(mname,name,qpn,imported,false,extern,refModule);
    end;
end DefinePort;

procedure WriteType(f : File; tn : TypeNode);
var
    en : EnumNode;
    pn : ParamNode;
    fn : FieldNode;
begin
    if tracePorttab then
	writef(f,"(%x)",integer(tn));
    end;
    if tn = nil then
	writef(f,"nil");
    else
	case tn^.kind of
	| DTINTEGER :
	    writef(f,"integer");
	| DTCARDINAL :
	    writef(f,"cardinal");
	| DTLONGREAL :
	    writef(f,"longreal");
	| DTREAL :
	    writef(f,"real");
	| DTCHAR :
	    writef(f,"char");
	| DTBOOLEAN :
	    writef(f,"boolean");
	| DTSET :
	    writef(f,"set{");
	    WriteType(f,tn^.setRange);
	    writef(f,"}");
	| DTENUMERATION :
	    writef(f,"(");
	    if tn^.enumList # nil then
		en := tn^.enumList^.first;
		while en # nil do
		    WriteString(f,en^.name);
		    en := en^.next;
		    if en # nil then
			writef(f,",");
		    end;
		end;
	    end;
	    writef(f,")");
	| DTPOINTER :
	    writef(f,"pointer to ");
	| DTRECORD :
	    writef(f,"record ");
	    if tn^.fieldList # nil then
		fn := tn^.fieldList^.first;
		while fn # nil do
		    WriteString(f,fn^.name);
		    writef(f,":");
		    WriteType(f,fn^.fieldType);
		    fn := fn^.next;
		    writef(f,";");
		end;
	    end;
	    writef(f," end");
	| DTARRAY :
	    writef(f,"array ");
	    WriteType(f,tn^.indexType);
	    writef(f," of ");
	    WriteType(f,tn^.elementType);
	| DTWORD :
	    writef(f,"word");
	| DTBYTE :
	    writef(f,"byte");
	| DTSUBRANGE :
	    WriteType(f,tn^.baseType);
	    writef(f,"[%d..%d]",tn^.subMinOrd,tn^.subMaxOrd);
	| DTPROC :
	    writef(f,"procedure(");
	    if tn^.paramList # nil then
		pn := tn^.paramList^.first;
		while pn # nil do
		    if (pn^.kind = PARAMVAR) or (pn^.kind = PARAMARRAYVAR) then
			writef(f,"var ");
		    end;
		    WriteString(f,pn^.name);
		    writef(f,":");
		    WriteType(f,pn^.paramType);
		    pn := pn^.next;
		    if pn # nil then
			writef(f,";");
		    end;
		end;
	    end;
	    writef(f,")");
	    if tn^.retType # nil then
		writef(f,":");
		WriteType(f,tn^.retType);
	    end;
	| DTOPAQUE :
	    writef(f,"opaque");
	else
	    writef(f,"unknown %d",ord(tn^.kind));
	end;
    end;
end WriteType;

procedure WriteConst(f : File; cn : ConstNode);
begin
    case cn^.kind of
    | DTSTRING:
	writef(f,"'");
	WriteString(f,cn^.strVal);
	writef(f,"'");
    | DTINTEGER, DTCARDINAL :
	writef(f,"%d",cn^.cardVal);
    | DTREAL, DTLONGREAL :
	WriteString(f,cn^.realVal);
    | DTCHAR :
	writef(f,"'");
	writef(f,"%c",chr(cn^.charVal));
	writef(f,"'");
    | DTBOOLEAN :
	writef(f,"%d",cn^.boolVal);
    | DTSET :
	writef(f,"{");
	WriteString(f,cn^.setVal^.value);
	writef(f,"}");
    | DTENUMERATION :
	WriteString(f,cn^.enumVal^.name);
    end;
end WriteConst;

procedure DumpPort(pn : PortNode);
begin
    writef(output,'Port module ');
    WriteString(output,pn^.refModule^.name);
    writef(output,' %c %x ',dumpSymLetter[pn^.kind],integer(pn));
    case pn^.kind of
    | SYMCONST :
	WriteConst(output,pn^.symConst);
    | SYMTYPE :
	WriteType(output,pn^.symType);
    | SYMVAR :
	WriteType(output,pn^.symVar);
    | SYMPROC :
	WriteType(output,pn^.symProc);
    end;
    writef(output,'\n');
end DumpPort;

procedure DumpExports();
var
    mn : ModuleNode;
    pn : PortNode;
    sym : Symbol;
begin
    writef(output,"DumpExports:\n");
    mn := moduleList^.first;
    while mn # nil do
	writef(output,"Module ");
	WriteString(output,mn^.name);
	writef(output," scope=%d\n",mn^.scope);
	if mn^.exports # nil then
	    sym := mn^.exports^.first;
	    while sym # nil do
		writef(output,'Symbol ');
		WriteString(output,sym^.name);
		writef(output,' %c\n',dumpSymLetter[sym^.kind]);
		if sym^.exported # nil then
		    writef(output,"Exported ");
		    DumpPort(sym^.exported);
		end;
		if sym^.imported # nil then
		    pn := sym^.imported^.first;
		    while pn # nil do
			writef(output,"Imported ");
			DumpPort(pn);
			pn := pn^.next;
		    end;
		end;
		sym := sym^.next;
	    end;
	end;
	mn := mn^.next;
    end;
end DumpExports;

procedure SameConst(ecn : ConstNode; var icn : ConstNode) : boolean;
var
    result : boolean;
begin
    if ecn = icn then
	return TRUE;
    elsif ecn^.kind # icn^.kind then
	return FALSE;
    else
	result := TRUE;
	case ecn^.kind of
	| DTSTRING :
	    result := ecn^.strVal = icn^.strVal;
	| DTCHAR :
	    result := ecn^.charVal = icn^.charVal;
	| DTINTEGER, DTCARDINAL :
	    result := ecn^.cardVal = icn^.cardVal;
	| DTBOOLEAN :
	    result := ecn^.boolVal = icn^.boolVal;
	| DTREAL, DTLONGREAL :
	    result := ecn^.realVal = icn^.realVal;
	| DTSET :
	    result := SameType(ecn^.setVal^.setType,icn^.setVal^.setType)
			and (ecn^.setVal^.value = icn^.setVal^.value)
			and (ecn^.setVal^.size = icn^.setVal^.size);
	| DTENUMERATION :
	    result := SameType(ecn^.enumVal^.enumType,icn^.enumVal^.enumType)
			and (ecn^.enumVal^.enumOrd = icn^.enumVal^.enumOrd);
	else
	    writef(output,"SameConst: unexpected kind?");
	    result := false;
	end;
    end;
    if result then
	(*icn := ecn;*)
    end;
    return result;
end SameConst;

const
    TYPESTACKSIZE = 100;
var
    etnStack, itnStack : array [1..TYPESTACKSIZE] of TypeNode;
    typeStackPtr : cardinal;

procedure SameType(etn : TypeNode; var itn : TypeNode) : boolean;
var
    result : boolean;
    epn, ipn : ParamNode;
    efn, ifn : FieldNode;
    een, ien : EnumNode;
    i : cardinal;
begin
    if etn = itn then
	return TRUE;
    elsif (etn = nil) or (itn = nil) then
	return FALSE;
    elsif etn^.kind # itn^.kind then
	return (etn^.kind = DTOPAQUE) or (itn^.kind = DTOPAQUE);
    else
	result := TRUE;
	case etn^.kind of
	| DTNULL, DTINTEGER, DTBOOLEAN, DTCHAR, DTREAL, DTBYTE, DTCARDINAL,
		DTWORD, DTPROCESS, DTLONGREAL:
	    (* matching primitive types *)
	| DTPOINTER:
	    for i := typeStackPtr to 1 by -1 do
		if etnStack[i] = etn then
		    return itn = itnStack[i];
		end;
	    end;
	    inc(typeStackPtr);
	    etnStack[typeStackPtr] := etn;
	    itnStack[typeStackPtr] := itn;
	    result := SameType(etn^.toType,itn^.toType);
	    dec(typeStackPtr);
	| DTRECORD:
	    if etn^.fieldList # nil then
		efn := etn^.fieldList^.first;
	    else
		efn := nil;
	    end;
	    if itn^.fieldList # nil then
		ifn := itn^.fieldList^.first;
	    else
		ifn := nil;
	    end;
	    while result and (efn # nil) and (ifn # nil) do
		result := (efn^.name = ifn^.name)
			    and (efn^.size = ifn^.size)
			    and (efn^.offset = ifn^.offset)
			    and SameType(efn^.fieldType,ifn^.fieldType);
		efn := efn^.next;
		ifn := ifn^.next;
	    end;
	    result := result and (efn = nil) and (ifn = nil);
	| DTPROC:
	    result := SameType(etn^.retType,itn^.retType);
	    if etn^.paramList # nil then
		epn := etn^.paramList^.first;
	    else
		epn := nil;
	    end;
	    if itn^.paramList # nil then
		ipn := itn^.paramList^.first;
	    else
		ipn := nil;
	    end;
	    while result and (epn # nil) and (ipn # nil) do
		result := (epn^.name = ipn^.name)
			    and (epn^.kind = ipn^.kind)
			    and SameType(epn^.paramType,ipn^.paramType);
		epn := epn^.next;
		ipn := ipn^.next;
	    end;
	    result := result and (epn = nil) and (ipn = nil);
	| DTARRAY:
	    result := SameType(etn^.indexType,itn^.indexType)
			and SameType(etn^.elementType,itn^.elementType);
	| DTSET:
	    result := SameType(etn^.setRange,itn^.setRange);
	| DTOPAQUE:
	    result := true;
	| DTSUBRANGE:
	    result := (etn^.subMinOrd = itn^.subMinOrd) and
			(etn^.subMinOrd = itn^.subMinOrd) and
			SameType(etn^.baseType,itn^.baseType);
	| DTENUMERATION:
	    result := etn^.enumCount = itn^.enumCount;
	    if etn^.enumList # nil then
		een := etn^.enumList^.first;
	    else
		een := nil;
	    end;
	    if itn^.enumList # nil then
		ien := itn^.enumList^.first;
	    else
		ien := nil;
	    end;
	    while result and (een # nil) and (ien # nil) do
		result := (een^.name = ien^.name) and
			    (een^.enumOrd = ien^.enumOrd);
		een := een^.next;
		ien := ien^.next;
	    end;
	    result := result and (een = nil) and (ien = nil);
	else
	    writef(output,"SameType: unexpected type?");
	    result := false;
	end;
    end;
    if result then
	(*itn := etn;*)
    end;
    return result;
end SameType;

procedure CheckSame(exp, imp : PortNode);
begin
    if exp^.kind # imp^.kind then
	ModuleError("identifier is used a different kind of object in",
	    exp^.name,exp^.refModule,imp^.refModule);
    elsif exp^.extern <> imp^.extern then
	ModuleError("external attribute differs with that in",
	    exp^.name,exp^.refModule,imp^.refModule);
    else
	case exp^.kind of
	| SYMCONST:
	    if not SameConst(exp^.symConst,imp^.symConst) then
		ModuleError("constant differs with that in",
		    exp^.name,exp^.refModule,imp^.refModule);
	    end;
	| SYMPROC:
	    if not SameType(exp^.symProc,imp^.symProc) then
		ModuleError("procedure differs with that in",
		    exp^.name,exp^.refModule,imp^.refModule);
	    end;
	| SYMTYPE:
	    if not SameType(exp^.symType,imp^.symType) then
		ModuleError("type differs with that in",
		    exp^.name,exp^.refModule,imp^.refModule);
	    end;
	| SYMVAR:
	    if not SameType(exp^.symVar,imp^.symVar) then
		ModuleError("variable type differs with that in",
		    exp^.name,exp^.refModule,imp^.refModule);
	    end;
	end;
    end;
end CheckSame;

procedure CheckExports();
var
    mn : ModuleNode;
    pn, pnnext : PortNode;
    sym : Symbol;
begin
    if logErrorsFlag then
	writef(output,"CheckExports:\n");
    end;
    if moduleList = nil then
	return;
    end;
    mn := moduleList^.first;
    while mn # nil do
	if tracePorttab then
	    writef(output,"Module ");
	    WriteString(output,mn^.name);
	    writef(output," scope=%d\n",mn^.scope);
	end;
	if mn^.ignoreErrors then
	    (* don't worry about this module *)
	elsif mn^.exports # nil then
	    sym := mn^.exports^.first;
	    while sym # nil do
		if tracePorttab then
		    writef(output,'Symbol ');
		    WriteString(output,sym^.name);
		    writef(output,' %c\n',dumpSymLetter[sym^.kind]);
		end;
		if sym^.exported = nil then
		    if sym^.imported # nil then
			pn := sym^.imported^.first;
			while pn # nil do
			    pnnext := pn^.next;
			    ModuleError("not exported but imported by",
				sym^.name,sym^.homeModule,pn^.refModule);
			    pn := pnnext;
			end;
		    end;
		else
		    if sym^.imported # nil then
			pn := sym^.imported^.first;
			while pn # nil do
			    pnnext := pn^.next;
			    if tracePorttab then
				writef(output,"Imported ");
				DumpPort(pn);
			    end;
			    CheckSame(sym^.exported,pn);
			    pn := pnnext;
			end;
		    end;
		end;
		sym := sym^.next;
	    end;
	end;
	mn := mn^.next;
    end;
end CheckExports;

procedure WatchModule(arg : array of char);
var
    i : cardinal;
    mn : ModuleNode;
begin
    i := 2;	(* skip -M *)
    while (i < number(arg)) and (arg[i] # 0C) do
	while (i < number(arg)) and (arg[i] # 0C) and (arg[i] # ',') do
	    AddChar(arg[i]);
	    inc(i);
	end;
	mn := DefineModule(NewString(),0);
	mn^.watchErrors := TRUE;
	inc(i);
    end;
end WatchModule;

procedure IgnoreModule(arg : array of char);
var
    i : cardinal;
    mn : ModuleNode;
begin
    i := 2;	(* skip -N *)
    while (i < number(arg)) and (arg[i] # 0C) do
	while (i < number(arg)) and (arg[i] # 0C) and (arg[i] # ',') do
	    AddChar(arg[i]);
	    inc(i);
	end;
	mn := DefineModule(NewString(),0);
	mn^.ignoreErrors := TRUE;
	inc(i);
    end;
end IgnoreModule;

begin
    moduleList := nil;
    typeStackPtr := 0;
    errorCount := 0;
    dumpSymLetter[SYMNULL] := '?';
    dumpSymLetter[SYMMODULE] := 'M';
    dumpSymLetter[SYMVAR] := 'V';
    dumpSymLetter[SYMPROC] := 'P';
    dumpSymLetter[SYMCONST] := 'C';
    dumpSymLetter[SYMTYPE] := 'T';
    printSymKind[SYMNULL] := 'unknown';
    printSymKind[SYMMODULE] := 'module';
    printSymKind[SYMVAR] := 'variable';
    printSymKind[SYMPROC] := 'procedure';
    printSymKind[SYMCONST] := 'constant';
    printSymKind[SYMTYPE] := 'type';
    AddText("$GLOBAL$");
    globalModule := DefineModule(NewString(),0);
    globalModule^.scope := globalScope;
    AddText("IO");
    ioModule := DefineModule(NewString(),0);
    ioModule^.ignoreErrors := TRUE;
    ioModule^.builtin := TRUE;
    AddText("SYSTEM");
    systemModule := DefineModule(NewString(),0);
    systemModule^.ignoreErrors := TRUE;
    systemModule^.builtin := TRUE;
    AddText("MEMORY");
    memoryModule := DefineModule(NewString(),0);
    memoryModule^.ignoreErrors := TRUE;
    memoryModule^.builtin := TRUE;
    AddText("Storage");
    storageModule := DefineModule(NewString(),0);
    storageModule^.ignoreErrors := TRUE;
    storageModule^.builtin := TRUE;
    AddText("BITOPERATIONS");
    bitoperationsModule := DefineModule(NewString(),0);
    bitoperationsModule^.ignoreErrors := TRUE;
    bitoperationsModule^.builtin := TRUE;
end porttab.
