(*#@(#)decls.p	4.1	Ultrix	7/17/90 *)
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
$Header: decls.p,v 1.6 84/06/06 12:55:51 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "const.h"
#include "alloc.h"
#include "decls.h"
#include "bstmt.h"
#include "cstmt.h"
#include "scanner.h"
#include "optim.h"

function NewTypeNode {(kind : DataType) : TypeNode};
var
    tn : TypeNode;
begin
    new(tn);
    tn^.kind := kind;
    tn^.next := nil;
    tn^.number := 0;
    tn^.size := -1;
    tn^.alignment := -1;
    tn^.name := nil;
    tn^.opaqueName := nil;
    tn^.module := nil;
    NewTypeNode := tn;
end;

procedure InitGlobalModule;
begin
    new(globalModule);
    globalModule^.kind := MODGLOBAL;
    globalModule^.scope := currScope;
    globalModule^.exportScope := currScope;
    globalModule^.body := nil;
    globalModule^.qualExports := nil;
    globalModule^.unqualExports := nil;
    globalModule^.imports := nil;
    globalModule^.enclosing := nil;
    globalModule^.enclosingProc := nil;
    globalModule^.modules := AddToModuleList(nil,nil);
    currModule := globalModule;
    new(globalProc);
    globalProc^.name := nil;
    globalProc^.globalName := nil;
    globalProc^.procType := procTypeNode;
    globalProc^.body := nil;
    globalProc^.code := AddToCodeList(nil,nil);
    globalProc^.enclosing := nil;
    globalProc^.enclosingModule := globalModule;
    globalProc^.tempMap := nil;
    globalProc^.scope := currScope;
    globalProc^.block := currScope^.block;
    globalProc^.displayLevel := 1;
    globalProc^.mem := InitAllocationNode;
    globalProc^.initFlagVar := nil;
    globalProc^.containsProcs := false;
    globalProc^.containsUpLevel := [];
    globalProc^.doesUpLevel := [];
    globalModule^.procs := AddToProcList(nil,globalProc);
    currProc := globalProc;
    globalPortList := AddToPortList(nil,nil);
end;

procedure AddModuleName(module : ModuleNode; proc : ProcNode);
begin
    if (module = nil) or
	    ((module = globalModule) and ((proc = globalProc) or (proc = nil)))
    then begin
	{ do nothing }
    end else if proc = nil then begin
	{ global thing, just modules }
	AddModuleName(module^.enclosing,proc);
	AddString(module^.name);
	AddChar('_');
    end else if module^.enclosingProc = proc then begin
	{ next level is a module }
	AddModuleName(module^.enclosing,proc);
	AddString(module^.name);
	AddChar('_');
    end else if proc^.enclosingModule = module then begin
	{ next level is a proc }
	AddModuleName(module,proc^.enclosing);
	AddString(proc^.name);
	AddChar('_');
    end else begin
	ErrorName(module^.name,'Module/proc list confused');
	ErrorName(proc^.name,'Module/proc list confused');
    end;
end;

function GlobalName(name:String; module : ModuleNode; proc : ProcNode) : String;
var
    globalName : String;
begin
    if TraceDecls then begin
	write(output,'GlobalName(');
	WriteString(output,name);
	write(output,',');
	if module <> nil then begin
	    WriteString(output,module^.name);
	end;
	write(output,',');
	if proc <> nil then begin
	    WriteString(output,proc^.name);
	end;
	writeln(output,')');
    end;
    if name = nil then begin
	globalName := nil;
    end else begin
	AddModuleName(module,proc);
	AddString(name);
	globalName := NewString;
    end;
    GlobalName := globalName;
end;

procedure DefineConst{(name : String; value : ConstNode)};
var
    sym : Symbol;
begin
    if TraceDecls then begin
	write(output,'DefineConst(');
	WriteString(output,name);
	write(output,'=');
	WriteConstant(output,value);
	writeln(output,')');
    end;
    if DefineSymbol(sym,name,nil,SCOPECASE) then begin;
	sym^.kind := SYMCONST;
	sym^.symConst := value;
    end else begin
	CheckEqualConst(sym,value);
    end;
end;

procedure DefineType{(name : String; value : TypeNode)};
var
    sym : Symbol;
    tn, otn : TypeNode;
begin
    if TraceDecls then begin
	write(output,'DefineType(');
	WriteString(output,name);
	writeln(output,')');
    end;
    if value <> nil then begin
	if value^.name = nil then begin
	    value^.name := name;
	    if currModule^.kind = MODDEFINITION then begin
		value^.module := currModule;
	    end;
	end;
    end;
    if DefineSymbol(sym,name,nil,SCOPECASE) then begin
	sym^.kind := SYMTYPE;
	if value = nil then begin
	    { opaque type }
	    otn := NewTypeNode(DTOPAQUE);
	    otn^.size := WORDSIZE;
	    otn^.opaqueName := GlobalName(name,currModule,currProc);
	    otn^.name := name;
	    if currModule^.kind = MODDEFINITION then begin
		otn^.module := currModule;
	    end;
	    sym^.symType := otn;
	end else begin
	    sym^.symType := value;
	end;
    end else if (sym^.kind = SYMTYPE) and (value <> nil) then begin
	CheckEqualType(sym,value);
    end else begin
	ErrorName(name,'Type redefined');
    end;
    if TraceDecls then begin
	if value <> nil then begin
	    writeln(output,'size=',value^.size:1:0,', align=',
			value^.alignment:1:0);
	end else begin
	    writeln(output,'value=nil');
	end;
    end;
end;

function DefineVar{(name : String; varType : TypeNode; mt : MemoryType;
	global : boolean) : VarNode};
var
    sym : Symbol;
    vn : VarNode;
    globalName : String;
    createVar : boolean;
    atn : TypeNode;
begin
    if TraceDecls then begin
	write(output,'DefineVar(');
	WriteString(output,name);
	writeln(output,')');
    end;
    createVar := true;
    if name <> nil then begin
	if DefineSymbol(sym,name,nil,SCOPECASE) then begin
	    sym^.kind := SYMVAR;
	end else begin
	    { createVar := false; create it anyhow }
	    CheckEqualVar(sym,varType);
	end;
    end;
    if createVar then begin
	atn := ActualType(varType);
	if atn <> nil then begin
	    if atn^.kind = DTARRAY then begin
		if (atn^.arrayKind = ARRAYOPEN) and (mt <> MEMPARAM) then begin
		    ErrorName(name,'Open array type is valid only for parameters');
		end;
	    end;
	end;
	new(vn);
	vn^.varType := varType;
	vn^.readonly := false;
	vn^.indirect := false;
	vn^.changed := false;
	vn^.name := name;
	currProc^.varList := AddToVarList(currProc^.varList,vn);
	if mt = MEMNONE then begin
	    { don't allocate memory yet }
	    vn^.address.kind := MEMNONE;
	end else if (name <> nil) and (currProc = globalProc) and
		(mt in [MEMNORMAL,MEMFAST])
	then begin
	    if global then begin
		globalName := name;
	    end else begin
		globalName := GlobalName(name, currModule, currProc);
	    end;
	    AllocateGlobal(globalName,varType^.size,vn^.address);
	    vn^.address.gvn^.extern := global;
	end else if (name <> nil) and (mt = MEMNORMAL) and
	    (varType^.kind in [DTPOINTER,DTINTEGER,DTBOOLEAN,DTCHAR,DTREAL,
		    DTSET,DTCARDINAL,DTBYTE,DTWORD,DTSUBRANGE,DTENUMERATION])
	    and (varType^.size <= WORDSIZE)
	then begin
	    { try fast memory }
	    AllocateMemory(currProc^.mem,MEMFAST,SizeOf(varType),
		    AlignmentOf(varType),currProc,vn^.address);
	end else begin
	    AllocateMemory(currProc^.mem,mt,SizeOf(varType),
		    AlignmentOf(varType),currProc,vn^.address);
	end;
	if name <> nil then begin
	    sym^.symVar := vn;
	end;
    end;
    DefineVar := vn;
end;

procedure DefineVarList{(idList : IdentList; varType : TypeNode;
	global : Token)};
var
    id : IdentNode;
    sym : Symbol;
    vn : VarNode;
begin
    id := idList^.first;
    while id <> nil do begin
	vn := DefineVar(id^.name,varType,MEMNORMAL,global=TKEXTERNAL);
	id := id^.next;
    end;
end;

function DefineModule{(name : String; kind : Token) : ModuleNode};
var
    mn : ModuleNode;
    sym : Symbol;
    symCase : SymbolCase;
begin
    if 'i' in debugSet then begin
	write(output,'DefineModule ');
	WriteString(output,name);
	write(output,' : ');
	WriteString(output,currFile);
	writeln(output);
    end;
    new(mn);
    mn^.enclosing := currModule;
    mn^.enclosingProc := currProc;
    mn^.name := name;
    mn^.procs := AddToProcList(nil,nil);
    mn^.modules := AddToModuleList(nil,nil);
    mn^.unqualExports := nil;
    mn^.qualExports := nil;
    mn^.imports := nil;
    mn^.doingImport := false;
    currModule^.modules := AddToModuleList(currModule^.modules,mn);
    if kind = TKBEGIN then begin	{ kind = TKBEGIN for builtin modules }
	symCase := ANYCASE;
    end else begin
	symCase := ONECASE;
    end;
    if not DefineSymbol(sym,name,nil,symCase) then begin
	ErrorName(name,'Module name redefined');
    end;
    sym^.kind := SYMMODULE;
    sym^.symModule := mn;
    currModule := mn;
    case kind of
	TKMODULE : begin
	    mn^.kind := MODPROGRAM;
	    mn^.scope := StartScope(false);
	    if mn^.enclosing = globalModule then begin
		{ program module, use scope for global proc }
		globalProc^.scope := mn^.scope;
		globalProc^.block := mn^.scope^.block;
		globalProc^.enclosingModule := mn;
		compileModuleName := name;
	    end;
	end;
	TKBEGIN, TKDEFINITION : begin
	    mn^.kind := MODDEFINITION;
	    mn^.scope := StartScope(false);
	    mn^.doingImport := true;
	end;
	TKIMPLEMENTATION : begin
	    mn^.kind := MODIMPLEMENTATION;
	    mn^.scope := StartScope(false);
	    globalProc^.scope := mn^.scope;
	    globalProc^.block := mn^.scope^.block;
	    if mn^.enclosing <> globalModule then begin
		ErrorName(mn^.name,'Implementation modules must not be nested');
	    end else begin
		AddText(MODULEINITNAME);
		globalProc^.name := NewString;
		globalProc^.globalName := GlobalName(globalProc^.name,mn,nil);
		globalProc^.enclosingModule := mn;
		AddText(MODULEINITFLAG);
		globalProc^.initFlagVar := DefineVar(NewString,booleanTypeNode,
						MEMNORMAL,false);
		compileModuleName := name;
	    end;
	end;
    end;
    DefineModule := mn;
end;

procedure GlobalPort(sym : Symbol; mn, tomn : ModuleNode; qualified : boolean);
var
    pn : PortNode;
begin
    if (sym <> nil) and ((mn^.kind = MODIMPLEMENTATION) or
	    (tomn^.kind in [MODIMPLEMENTATION,MODPROGRAM]))
    then begin
	new(pn);
	pn^.sym := sym;
	pn^.module := mn;
	pn^.qualified := qualified;
	pn^.export := mn^.kind = MODIMPLEMENTATION;
	pn^.extern := false;
	if sym^.kind = SYMPROC then begin
	    pn^.extern := sym^.symProc^.extern;
	end else if sym^.kind = SYMVAR then begin
	    if sym^.symVar^.address.kind = MEMGLOBAL then begin
		pn^.extern := sym^.symVar^.address.gvn^.extern;
	    end;
	end;
	globalPortList := AddToPortList(globalPortList,pn);
	if 'g' in debugSet then begin
	    write(output,'GlobalPort: ');
	    WriteString(output,mn^.name);
	    write(output,'.');
	    WriteString(output,sym^.name);
	    write(output,' ',sym^.kind,' to ');
	    WriteString(output,tomn^.name);
	    writeln(output);
	end;
    end;
end;

procedure ExportExternalProcs(mn : ModuleNode);
var
    sym : Symbol;
    id : IdentNode;
begin
    { check qualified exports for procedures or modules }
    if mn^.qualExports <> nil then begin
	id := mn^.qualExports^.first;
	while id <> nil do begin
	    sym := LookUpSymbol(id^.name,nil,ONECASE);
	    if sym = nil then begin
		{ error, but already found }
	    end else if sym^.kind = SYMPROC then begin
		sym^.symProc^.internalProc := false;
	    end else if sym^.kind = SYMMODULE then begin
		ExportExternalProcs(sym^.symModule);
	    end;
	    GlobalPort(sym,mn,globalModule,false);
	    id := id^.next;
	end;
    end;

    { check unqualified exports for procedures or modules }
    if mn^.unqualExports <> nil then begin
	id := mn^.unqualExports^.first;
	while id <> nil do begin
	    sym := LookUpSymbol(id^.name,nil,ONECASE);
	    if sym = nil then begin
		{ error, but already found }
	    end else if sym^.kind = SYMPROC then begin
		sym^.symProc^.internalProc := false;
	    end else if sym^.kind = SYMMODULE then begin
		ExportExternalProcs(sym^.symModule);
	    end;
	    GlobalPort(sym,mn,globalModule,false);
	    id := id^.next;
	end;
    end;
end;

procedure ProcessExports(mn : ModuleNode);
var
    sym, nsym : Symbol;
    id : IdentNode;
    exportScope, enclScope : Scope;
begin
    if 'i' in debugSet then begin
	write(output,'ProcessExports ');
	WriteString(output,mn^.name);
	write(output,' : ');
	WriteString(output,currFile);
	writeln(output);
    end;
    { create scope for qualified exports }
    exportScope := StartScope(false);
    EndScope;
    mn^.exportScope := exportScope;

    { export qualified exports to module export scope }
    if mn^.qualExports <> nil then begin
	id := mn^.qualExports^.first;
	while id <> nil do begin
	    sym := LookUpSymbol(id^.name,nil,ONECASE);
	    if sym = nil then begin
		ErrorName(id^.name,'Exported identifier not found in module');
	    end else if sym^.kind in [SYMFIELD,SYMENUM] then begin
		ErrorName(id^.name,'Cannot export a field or enumeration constant');
	    end else begin
		if mn^.enclosing = globalModule then begin
		    if sym^.kind = SYMPROC then begin
			sym^.symProc^.internalProc := false;
		    end else if sym^.kind = SYMMODULE then begin
			ExportExternalProcs(sym^.symModule);
		    end;
		    GlobalPort(sym,mn,globalModule,true);
		end;
		nsym := Port(sym,exportScope);
	    end;
	    id := id^.next;
	end;
    end;

    { export unqualified exports to enclosing scope and module export scope }
    if mn^.unqualExports <> nil then begin
	enclScope := currScope^.enclosing;

	id := mn^.unqualExports^.first;
	while id <> nil do begin
	    sym := LookUpSymbol(id^.name,nil,ONECASE);
	    if sym = nil then begin
		ErrorName(id^.name,'Exported identifier not found in module');
	    end else if sym^.kind in [SYMFIELD,SYMENUM] then begin
		ErrorName(id^.name,'Cannot export a field or enumeration constant');
	    end else begin
		nsym := Port(sym,exportScope);
		nsym := Port(sym,enclScope);

		if mn^.enclosing = globalModule then begin
		    if sym^.kind = SYMPROC then begin
			sym^.symProc^.internalProc := false;
		    end else if sym^.kind = SYMMODULE then begin
			ExportExternalProcs(sym^.symModule);
		    end;
		    GlobalPort(sym,mn,globalModule,false);
		end;
	    end;
	    id := id^.next;
	end;
    end;
end;

procedure EndModule{(mn : ModuleNode; body : StmtList; name : String)};
var
    code : CodeNode;
begin

    if 'i' in debugSet then begin
	write(output,'EndModule ');
	WriteString(output,mn^.name);
	write(output,' : ');
	WriteString(output,currFile);
	writeln(output);
    end;
    if mn^.kind <> MODIMPLEMENTATION then begin
	ProcessExports(mn);
    end;
    mn^.body := body;
    mn^.doingImport := false;

    { put code for this module on enclosing procedures code list }
    new(code);
    code^.kind := CODEMODULE;
    code^.module := mn;
    code^.stmts := body;
    currProc^.code := AddToCodeList(currProc^.code,code);
    EndScope;
    currModule := mn^.enclosing;
    if (name <> nil) and (name <> mn^.name) then begin
	ErrorName(mn^.name,'Name of module does not appear on end');
    end;
end;

procedure GetDefinitionModule(mn : ModuleNode);
var
    fileString : String;
    i : integer;
begin
    if mn <> nil then begin
	{ read in the definition module }
	i := 0;
	while (i < mn^.name^.length)
	    and (i < FILENAMESIZE-4)
	do begin
	    AddChar(GetChar(mn^.name,i));
	    i := i + 1;
	end;
	AddText('.def');
	fileString := NewString;

	{ continue parsing with file }
	if not InitFile(fileString) then begin
	    ErrorName(fileString,
		    'Cannot find definition module for implementation module');
	    exit(999);
	end;
	if 'i' in debugSet then begin
	    write(output,'GetDefinitionFile ');
	    WriteString(output,mn^.name);
	    write(output,' : ');
	    WriteString(output,currFile);
	    writeln(output);
	end;
    end;
end;

function DefineProc{(name : String; global : Token): ProcNode};
var
    pn : ProcNode;
    sym : Symbol;
begin
    if DefineSymbol(sym,name,nil,SCOPECASE) then begin
	currProc^.containsProcs := true;
	sym^.kind := SYMPROC;
	new(pn);
	pn^.fileName := currFile;
	pn^.lineNumber := currLine;
	pn^.name := name;
	if global = TKEXTERNAL then begin
	    pn^.globalName := name;
	    pn^.extern := true;
	end else begin
	    pn^.globalName := GlobalName(name, currModule, currProc);
	    pn^.extern := false;
	end;
	pn^.procType := nil;
	pn^.builtin := BIPNOTBIP;
	pn^.body := nil;
	pn^.code := AddToCodeList(nil,nil);

	pn^.scope := StartScope(true);
	pn^.block := pn^.scope^.block;
	pn^.mem := InitAllocationNode;
	pn^.displayLevel := currProc^.displayLevel+1;
	pn^.enclosing := currProc;
	pn^.enclosingModule := currModule;
	pn^.containsProcs := false;
	pn^.internalProc := not OptNcall;
	pn^.tailRecursion := false;
	pn^.tempMap := nil;
	pn^.varList := nil;
	currProc := pn;
	sym^.symProc := pn;
    end else begin
	if sym^.kind <> SYMPROC then begin
	    ErrorName(name,'Symbol redefined');
	    pn := nil;
	end else if sym^.symProc^.builtin <> BIPNOTBIP then begin
	    ErrorName(name,'Builtin procedure redefined');
	    pn := nil;
	end else if sym^.symProc^.body <> nil then begin
	    ErrorName(sym^.symProc^.name,'Procedure redefined');
	    pn := nil;
	end else if (sym^.symProc^.name <> sym^.symProc^.globalName)
	    and (global = TKEXTERNAL)
	then begin
	    ErrorName(sym^.symProc^.name,'External must be specified in definition module');
	    pn := nil;
	end else begin
	    pn := sym^.symProc;
	    { put procedure in proper scope nesting }
	    pn^.scope^.enclosing := currScope;
	    currScope := pn^.scope;
	    currProc := pn;
	    pn^.fileName := currFile;
	    pn^.lineNumber := currLine;
	end;
    end;
    DefineProc := pn;
end;

function AddTypeToProc{(proc : ProcNode; procType : TypeNode): ProcNode};
begin
    if proc = nil then begin
	{ do nothing }
    end else if proc^.procType <> nil then begin
	CheckEqualProc(proc,procType);
    end else begin
	proc^.procType := procType;
    end;
    AddTypeToProc := proc;
end;

procedure EndProc{(proc : ProcNode; body : StmtList; name : String)};
var
    code : CodeNode;
    param : ParamNode;
    vn : VarNode;
    atn : TypeNode;
begin
    if proc = nil then begin
	{ do nothing }
    end else begin
	if body <> nil then begin
	    proc^.body := body;
	    currModule^.procs := AddToProcList(currModule^.procs,proc);
	    new(code);
	    code^.kind := CODEPROC;
	    code^.proc := proc;
	    code^.stmts := body;
	    proc^.code := AddToCodeList(proc^.code,code);
	    { allocate variables for parameters }
	    if proc^.procType^.paramList <> nil then begin
		param := proc^.procType^.paramList^.first;
		while param <> nil do begin
		    atn := ActualType(param^.paramType);
		    if atn <> nil then begin
			if atn^.kind = DTARRAY then begin
			    if atn^.nocount then begin
				ErrorName(param^.name,'Modula-2 routines may not have nocount parameters');
			    end;
			end;
		    end;
		    case param^.kind of
			PARAMVAR  : begin
			    { reference parameter: allocate address }
			    vn := DefineVar(param^.name,addressTypeNode,MEMPARAM,false);
			    vn^.varType := param^.paramType;
			    vn^.indirect := true;
			end;
			PARAMVALUE : begin
			    if param^.reference then begin
				{ multiword parameter: allocate address }
				vn := DefineVar(param^.name,addressTypeNode,
					MEMPARAM,false);
				vn^.varType := param^.paramType;
				vn^.indirect := true;
			    end else begin
				vn := DefineVar(param^.name,param^.paramType,
					MEMPARAM,false);
			    end;
			end;
			PARAMARRAYVAR, PARAMARRAYVALUE : begin
			    vn := DefineVar(param^.name,addressTypeNode,MEMPARAM,false);
			    vn^.varType := param^.paramType;
			    param^.numElements :=
					DefineVar(nil,integerTypeNode,MEMPARAM,false);
			end;
		    end;
		    param^.paramVar := vn;
		    param := param^.next;
		end;
	    end;
	end;
	if (name <> nil) and (name <> proc^.name) then begin
	    ErrorName(proc^.name,'Name of procedure does not appear on end');
	end;
	currProc := currProc^.enclosing;
	EndScope;
    end;
end;

procedure CheckProc{(pn : ProcNode)};
var
    saveScope : Scope;
    saveProc : ProcNode;
    param : ParamNode;
    vn : VarNode;
begin
    saveScope := currScope;
    saveProc := currProc;
    currScope := pn^.scope;
    currProc := pn;

    if pn^.body <> nil then begin
	if 'c' in debugSet then begin
	    write(output,'Unchecked statements for module ');
	    WriteString(output,pn^.name);
	    writeln(output);
	    PrintStmtList(pn^.body,0);
	end;
	returnSeen := false;
	CheckStmtList(pn^.body);
	if pn^.procType^.funcType <> nil then begin
	    CheckReturn(pn);
	end;
	if 'c' in debugSet then begin
	    write(output,'Checked statements for module ');
	    WriteString(output,pn^.name);
	    writeln(output);
	    PrintStmtList(pn^.body,0);
	end;
	currFile := pn^.fileName;
	currLine := pn^.lineNumber;
	{ allocate variables for parameters }
	if pn^.procType^.paramList <> nil then begin
	    param := pn^.procType^.paramList^.first;
	    while param <> nil do begin
		vn := param^.paramVar;
		case param^.kind of
		    PARAMARRAYVAR,
		    PARAMVAR  : begin
		    end;
		    PARAMVALUE : begin
			{ value parameter: check for modifications }
			if (vn^.varType^.size > WORDSIZE) and
			    (vn^.varType^.kind <> DTLONGREAL)
			then begin
			    { large and modified :  copy it }
			    param^.docopy := vn^.changed;
			    if param^.docopy and
				    (param^.paramType^.kind = DTARRAY)
			    then begin
				if param^.paramType^.nocount then begin
				    ErrorName(param^.name,
				    'NOCOUNT parameter must not require copying');
				end;
			    end;
			end;
		    end;
		    PARAMARRAYVALUE : begin
			if vn^.changed then begin
			    { modified value open array parameter:  must copy it }
			    param^.docopy := true;
			end;
		    end;
		end;
		param := param^.next;
	    end;
	end;
    end;
    currScope := saveScope;
    currProc := saveProc;
end;

procedure CheckModule{(mn : ModuleNode)};
var
    submn : ModuleNode;
    pn : ProcNode;
    saveScope : Scope;
    saveModule : ModuleNode;
    imp : ImportNode;
begin
    saveModule := currModule;
    saveScope := currScope;
    currModule := mn;
    currScope := mn^.scope;

    { look for any unresolved imports }
    if mn^.imports <> nil then begin
	imp := mn^.imports^.first;
	while imp <> nil do begin
	    ProcessImport(imp,true);
	    imp := imp^.next;
	end;
    end;

    submn := mn^.modules^.first;
    while submn <> nil do begin
	CheckModule(submn);
	submn := submn^.next;
    end;
    pn := mn^.procs^.first;
    while pn <> nil do begin
	CheckProc(pn);
	pn := pn^.next;
    end;
    if mn^.body <> nil then begin
	if 'c' in debugSet then begin
	    write(output,'Unchecked statements for module ');
	    WriteString(output,mn^.name);
	    writeln(output);
	    PrintStmtList(mn^.body,0);
	end;
	CheckStmtList(mn^.body);
	if 'c' in debugSet then begin
	    write(output,'Checked statements for module ');
	    WriteString(output,mn^.name);
	    writeln(output);
	    PrintStmtList(mn^.body,0);
	end;
    end;
    currModule := saveModule;
    currScope := saveScope;
end;

function MakeParamList{(kindToken : Token; idents : IdentList;
	paramType : TypeNode) : ParamList};
var
    pl : ParamList;
    pn : ParamNode;
    id : IdentNode;
    kind : ParamKind;
    reference : boolean;
begin
    reference := false;
    if kindToken = TKVAR then begin
	kind := PARAMVAR;
	if (paramType^.kind = DTARRAY) then begin
	    if paramType^.arrayKind = ARRAYOPEN then begin
		kind := PARAMARRAYVAR;
	    end;
	end;
	reference := true;
    end else begin
	kind := PARAMVALUE;
	if (paramType^.kind = DTARRAY) then begin
	    if paramType^.arrayKind = ARRAYOPEN then begin
		kind := PARAMARRAYVALUE;
		reference := true;
	    end;
	end;
	if (SizeOf(paramType) > WORDSIZE) and
		(BaseType(paramType) <> longrealTypeNode)
	then begin
	    reference := true;
	end;
    end;
    { put first param on list. }
    { Note: this works even if idents is nil, as in proc type definition }
    new(pn);
    if idents = nil then begin
	pn^.name := nil;
    end else begin
	pn^.name := idents^.first^.name;
    end;
    pn^.kind := kind;
    pn^.paramType := paramType;
    pn^.next := nil;
    pn^.docopy := false;
    pn^.reference := reference;
    new(pl);
    pl^.first := pn;
    pl^.last := pn;
    if idents <> nil then begin
	{ do additional parameters, if more idents }
	id := idents^.first^.next;
	while id <> nil do begin
	    new(pn);
	    pn^.name := id^.name;
	    pn^.kind := kind;
	    pn^.paramType := paramType;
	    pn^.next := nil;
	    pn^.docopy := false;
	    pn^.reference := reference;
	    pl^.last^.next := pn;
	    pl^.last := pn;
	    id := id^.next;
	end;
    end;
    MakeParamList := pl;
end;

function AppendIdentList{(some, more : IdentList) : IdentList};
begin
    if some = nil then begin
	some := more;
    end else if more = nil then begin
	{ nothing to do }
    end else if more^.first = nil then begin
	{ nothing to add }
	dispose(more);
    end else if some^.first = nil then begin
	{ nothing to add to }
	dispose(some);
	some := more;
    end else begin
	some^.last^.next := more^.first;
	some^.last := more^.last;
	dispose(more);
    end;
    AppendIdentList := some;
end;

function AppendParamList{(some, more : ParamList) : ParamList};
begin
    if some = nil then begin
	some := more;
    end else if more = nil then begin
	{ nothing to do }
    end else if more^.first = nil then begin
	{ nothing to add }
	dispose(more);
    end else if some^.first = nil then begin
	{ nothing to add to }
	dispose(some);
	some := more;
    end else begin
	some^.last^.next := more^.first;
	some^.last := more^.last;
	dispose(more);
    end;
    AppendParamList := some;
end;

function ProcType{(paramList : ParamList; funcType : Typenode) : TypeNode};
var
    tn : TypeNode;
    num : integer;
    pn : ParamNode;
begin
    tn := NewTypeNode(DTPROC);
    tn^.size := WORDSIZE;
    tn^.paramList := paramList;
    tn^.funcType := funcType;
    num := 0;
    if paramList <> nil then begin
	pn := paramList^.first;
	while pn <> nil do begin
	    num := num + 1;
	    pn := pn^.next;
	end;
    end;
    tn^.numParams := num;
    ProcType := tn;
end;

function PointerForwardType (name : String; option : Token) : TypeNode;
var
    tn, otn : TypeNode;
    sym : Symbol;
begin
    tn := nil;
    { look for ident.  If not found, create a forward reference to it }
    sym := LookUpSymbol(name,nil,ONECASE);
    if sym <> nil then begin
	if sym^.kind <> SYMTYPE then begin
	    ErrorName(name,'Must be a type for pointer type definition');
	end else begin
	    tn := PointerType(sym^.symType,option);
	end;
    end else begin
	if not DefineSymbol(sym,name,nil,SCOPECASE) then begin
	    ErrorName(name,'Unexpected error in PointerForwardType');
	end else begin
	    { treat toType as a rename (indirect) type }
	    otn := NewTypeNode(DTRENAME);
	    otn^.size := WORDSIZE;
	    otn^.renameType := nil;
	    tn := PointerType(otn,option);
	    sym^.kind := SYMTYPE;
	    sym^.symType := otn;
	end;
    end;
    PointerForwardType := tn;
end;

function PointerType {(toType : TypeNode; option : Token) : TypeNode};
var
    tn : TypeNode;
begin
    tn := NewTypeNode(DTPOINTER);
    tn^.size := WORDSIZE;
    tn^.toType := toType;
    if option = TKPOINTER then begin
	tn^.ptrCheck := CHECKPTRMODULA;
    end else if option = TKATPASCAL then begin
	tn^.ptrCheck := CHECKPTRPASCAL;
    end else if option = TKATC then begin
	tn^.ptrCheck := CHECKPTRC;
    end else if option = TKATNONE then begin
	tn^.ptrCheck := CHECKPTRNONE;
    end else if option = TKATNIL then begin
	tn^.ptrCheck := CHECKPTRNIL;
    end;
    PointerType := tn;
end;

function SetType {(setRange : TypeNode) : TypeNode};
var
    tn : TypeNode;
begin
    tn := NewTypeNode(DTSET);
    tn^.setRange := setRange;
    tn^.size := NumberOf(setRange);
    SetType := tn;
end;

function ArrayType {(indexType : TypeNode; elementType : TypeNode;
	kind, option : Token) : TypeNode};
var
    atn, btn : TypeNode;
begin
    atn := NewTypeNode(DTARRAY);
    atn^.indexType := indexType;
    atn^.elementType := elementType;
    if indexType = nil then begin
	{ open array parameter }
	atn^.size := 2 * WORDSIZE;	{ for address and number of elements }
	atn^.nocount := option = TKNOCOUNT;
	atn^.arrayKind := ARRAYOPEN;
    end else begin
	atn^.arrayKind := ARRAYNORMAL;
	atn^.nocount := false;
	case target of
	    TARGETVAX : begin
		atn^.size := NumberOf(indexType) * SizeOf(elementType);
		atn^.alignment := AlignmentOf(elementType);
	    end;
	    TARGETTITAN : begin
		btn := BaseType(elementType);
		if btn^.kind = DTCHAR then begin
		    atn^.size := RoundUp(NumberOf(indexType) * CHARSIZE,WORDSIZE);
		end else begin
		    atn^.size := NumberOf(indexType) * SizeOf(elementType);
		end;
		atn^.alignment := WORDSIZE;
	    end;
	end;
    end;
    ArrayType := atn;
end;

function MakeFieldList{(idents : IdentList; fieldType : TypeNode) : FieldList};
	var
    fl : FieldList;
    fn : FieldNode;
    id : IdentNode;
begin
    { put first field on list. }
    new(fn);
    fn^.kind := FIELDNORMAL;
    fn^.name := idents^.first^.name;
    fn^.fieldType := fieldType;
    fn^.containingVariant := nil;
    fn^.next := nil;
    new(fl);
    fl^.first := fn;
    fl^.last := fn;
    if idents <> nil then begin
	{ do additional fields, if more idents }
	id := idents^.first^.next;
	while id <> nil do begin
	    new(fn);
	    fn^.kind := FIELDNORMAL;
	    fn^.name := id^.name;
	    fn^.fieldType := fieldType;
	    fn^.containingVariant := nil;
	    fn^.next := nil;
	    fl^.last^.next := fn;
	    fl^.last := fn;
	    id := id^.next;
	end;
    end;
    MakeFieldList := fl;
end;


function AppendFieldList{(some, more : FieldList) : FieldList};
begin
    if some = nil then begin
	some := more;
    end else if more = nil then begin
	{ nothing to do }
    end else if more^.first = nil then begin
	{ nothing to add }
	dispose(more);
    end else if some^.first = nil then begin
	{ nothing to add to }
	dispose(some);
	some := more;
    end else begin
	some^.last^.next := more^.first;
	some^.last := more^.last;
	dispose(more);
    end;
    AppendFieldList := some;
end;

function MakeVariant {(tag : ConstSetList; fieldList : FieldList)
	: VariantNode};
var
    vn : VariantNode;
begin
    new(vn);
    vn^.tag := tag;
    vn^.fieldList := fieldList;
    vn^.tagField := nil;
    MakeVariant := vn;
end;

function MakeVariantField {(ident : String; fieldType : TypeNode;
	variantList : VariantList; elseVariant : VariantNode) : FieldList};
var
    fl : FieldList;
    fn : FieldNode;
begin
    new(fn);
    fn^.name := ident;
    fn^.fieldType := fieldType;
    fn^.kind := FIELDVARIANT;
    fn^.containingVariant := nil;
    fn^.variantList := AddToVariantList(variantList,elseVariant);
    new(fl);
    fl^.first := fn;
    fl^.last := fn;
    fn^.next := nil;
    MakeVariantField := fl;
end;

procedure DefineFields{(fieldList : FieldList; scope : Scope;
	an : AllocationNode; recType : TypeNode;
	containingVariant : VariantNode; var alignment : cardinal)};
var
    fn : FieldNode;
    vn : VariantNode;
    sym : Symbol;
    san : AllocationNode;
    address : Address;
    fieldAlign : cardinal;
    atn : TypeNode;
begin
    fn := fieldList^.first;
    while fn <> nil do begin
	fn^.recType := recType;
	fn^.containingVariant := containingVariant;
	if fn^.name <> nil then begin
	    if DefineSymbol(sym,fn^.name,scope,SCOPECASE) then begin
		atn := ActualType(fn^.fieldType);
		if atn <> nil then begin
		    if atn^.kind = DTARRAY then begin
			if atn^.arrayKind = ARRAYOPEN then begin
			    ErrorName(fn^.name,'Open array type is valid only for parameters');
			end;
		    end;
		end;
		sym^.kind := SYMFIELD;
		sym^.symField := fn;
		fieldAlign := AlignmentOf(fn^.fieldType);
		AllocateMemory(an,MEMNORMAL,SizeOf(fn^.fieldType),
		    fieldAlign,nil,address);
		fn^.offset := address.offset;
		if fieldAlign > alignment then begin
		    alignment := fieldAlign;
		end;
	    end else begin
		ErrorName(fn^.name,'Field name reused in record');
	    end;
	end else begin
	    fn^.offset := -1;
	end;
	if fn^.kind = FIELDVARIANT then begin
	    vn := fn^.variantList^.first;
	    while vn <> nil do begin
		vn^.tagField := fn;
		san := SaveAllocationNode(an);
		DefineFields(vn^.fieldList,scope,an,recType,vn,alignment);
		RestoreAllocationNode(an,san);
		vn := vn^.next;
	    end;
	    UpdateAllocationNode(an);
	end;
	fn := fn^.next;
    end;
end;

function RecordType {(fieldList : FieldList) : TypeNode};
var
    tn : TypeNode;
    an : AllocationNode;
begin
    tn := NewTypeNode(DTRECORD);
    tn^.fieldList := fieldList;
    tn^.recScope := StartScope(false);
    EndScope;
    an := InitAllocationNode;
    tn^.alignment := 1;
    DefineFields(fieldList, tn^.recScope, an, tn, nil,tn^.alignment);
    tn^.size := RoundUp(an^.maximum[MEMNORMAL],tn^.alignment);
    dispose(an);
    RecordType := tn;
end;

function MakeSubrange {(low, up : cardinal; baseType : Typenode)
	    : TypeNode};
var
    tn : TypeNode;
begin
    tn := NewTypeNode(DTSUBRANGE);
    tn^.subMaxOrd := up;
    tn^.subMinOrd := low;
    tn^.baseType := baseType;
    tn^.size := baseType^.size;
    tn^.alignment := baseType^.alignment;
    MakeSubrange := tn;
end;

function SubrangeType {(lower, upper : ConstNode) : TypeNode};
var
    tn, baseType : TypeNode;
    low, up : cardinal;
begin
    tn := nil;
    if (lower = nil) or (upper = nil) then begin
	{ do nothing }
    end else if (lower^.kind <> upper^.kind) and
	    (not (lower^.kind in [DTINTEGER,DTCARDINAL]) or
	    not (upper^.kind in [DTINTEGER,DTCARDINAL]))
    then begin
	Error('Subrange lower and upper bounds are not the same type');
    end else if not (lower^.kind in indexableTypes) then begin
	ErrorName(stringDataType[lower^.kind],'Invalid type for subrange');
    end else begin
	low := OrdOf(lower);
	up := OrdOf(upper);
	case lower^.kind of
	    DTINTEGER, DTCARDINAL: begin
		if low < 0 then begin
		    if (low < -MAXINT-1) or (up > MAXINT) then begin
			Error('Subrange bounds exceed implementation limits');
		    end;
		    baseType := integerTypeNode;
		end else if standardCardinalFlag or (up > MAXINT) then begin
		    if up > MAXCARD then begin
			Error('Subrange bounds exceed implementation limits');
		    end;
		    baseType := cardinalTypeNode;
		end else begin
		    baseType := cardIntTypeNode;
		end;
	    end;
	    DTCHAR: begin
		baseType := charTypeNode;
	    end;
	    DTENUMERATION: begin
		if lower^.enumVal^.enumType <> upper^.enumVal^.enumType
		then begin
		    Error('Subrange lower and upper bounds are not the same enumeration');
		end;
		baseType := lower^.enumVal^.enumType;
	    end;
	    DTBOOLEAN: begin
		baseType := booleanTypeNode;
	    end;
	end;
	if low > up then begin
	    Error('Start of subrange greater than end');
	end else begin
	    tn := MakeSubrange(low,up,baseType);
	end;
    end;
    SubrangeType := tn;
end;

function EnumerationType{(idList : IdentList) : TypeNode};
var
    tn : TypeNode;
    id, idnext : IdentNode;
    sym : Symbol;
    enum : EnumNode;
    enumList : EnumList;
    enumOrd : integer;
    redefinedType : TypeNode;
    error : boolean;
begin
    error := false;
    tn := NewTypeNode(DTENUMERATION);
    tn^.size := WORDSIZE;
    enumList := AddToEnumList(nil,nil);
    enumOrd := 0;
    redefinedType := nil;
    id := idList^.first;
    while id <> nil do begin
	if DefineSymbol(sym,id^.name,nil,SCOPECASE) then begin
	    sym^.kind := SYMENUM;
	    new(enum);
	    enum^.name := id^.name;
	    enum^.enumOrd := enumOrd;
	    enum^.enumType := tn;
	    enum^.enumSym := sym;
	    sym^.symEnum := enum;
	    enumList := AddToEnumList(enumList,enum);
	end else begin
	    if sym^.kind <> SYMENUM then begin
		ErrorName(id^.name,'Enumeration constant redefined');
		error := true;
	    end else if sym^.symEnum^.enumOrd <> enumOrd then begin
		ErrorName(id^.name,'Enumeration constant redefined');
		error := true;
	    end else if redefinedType = nil then begin
		redefinedType := sym^.symEnum^.enumType;
	    end else if sym^.symEnum^.enumType <> redefinedType then begin
		Error('Enumeration partially redefined');
		error := true;
	    end;
	end;
	enumOrd := enumOrd + 1;
	id := id^.next;
    end;
    id := idList^.first;
    while id <> nil do begin
	idnext := id^.next;
	dispose(id);
	id := idnext;
    end;
    dispose(idList);
    if error then begin
	dispose(tn);
	tn := nil;
    end else if redefinedType = nil then begin
	tn^.enumList := enumList;
	tn^.enumCount := enumOrd;
    end else begin
	dispose(tn);
	if enumOrd = redefinedType^.enumCount then begin
	    tn := redefinedType;
	end else begin
	    Error('Enumeration partially redefined');
	    tn := nil;
	end;
    end;
    EnumerationType := tn;
end;


function TypeWithSize(tn : TypeNode; size : ConstNode) : TypeNode;
var
    stn : TypeNode;
begin
    stn := NewTypeNode(DTRENAME);
    stn^.renameType := tn;
    stn^.size := OrdOf(size);
    TypeWithSize := stn;
end;

function TypeWithAlign(tn : TypeNode; alignment : ConstNode) : TypeNode;
var
    stn : TypeNode;
begin
    stn := NewTypeNode(DTRENAME);
    stn^.renameType := tn;
    stn^.size := tn^.size;
    stn^.alignment := OrdOf(alignment);
    TypeWithAlign := stn;
end;


function MakeIdent {(name : String) : IdentNode};
var
    idn : IdentNode;
begin
    new(idn);
    idn^.name := name;
    MakeIdent := idn;
end;

procedure PrintType{(tn:TypeNode;indent:integer)};
begin
    if tn = nil then begin
	writeln(output,' ':indent,'nil pointer');
    end else begin
	writeln(output,' ':indent,tn^.kind);
	if not(tn^.kind in [DTINTEGER, DTBOOLEAN, DTCHAR, DTREAL, DTLONGREAL,
		DTCARDINAL])
	then begin
	    case tn^.kind of
		DTPOINTER : begin
		    PrintType(tn^.toType,indent+INDENT);
		end;
		DTSET : begin
		    PrintType(tn^.setRange,indent+INDENT);
		end;
		DTRENAME : begin
		    PrintType(tn^.renameType,indent+INDENT);
		end;
		DTOPAQUE : begin
		    write(output,' ':indent+INDENT);
		    WriteString(output,tn^.opaqueName);
		    writeln(output);
		end;
		DTARRAY : begin
		    PrintType(tn^.indexType,indent+INDENT);
		    PrintType(tn^.elementType,indent+INDENT);
		end;
		DTRECORD : begin
		    writeln(output,' ':indent+INDENT,'Record fields');
		end;
		DTSUBRANGE : begin
		    writeln(output,' ':indent+INDENT,tn^.subMinOrd:1,'..',
			tn^.subMaxOrd:1);
		    PrintType(tn^.baseType,indent+INDENT);
		end;
	    end;
	end;
    end;
end;

function Import{(fromIdent : String; idents : IdentList) : ImportNode};
var
    imp : ImportNode;
begin
    if TraceDecls then begin
	writeln(output,'Import: start');
    end;
    new(imp);
    imp^.fileName := currFile;
    imp^.lineNumber := currLine;
    imp^.fromIdent := fromIdent;
    imp^.idents := idents;
    if fromIdent <> nil then begin
	imp^.searchList := AddToIdentList(nil,MakeIdent(fromIdent));
    end else begin
	imp^.searchList := idents;
    end;
    imp^.currSearch := imp^.searchList^.first;
    imp^.saveModule := nil;
    imp^.saveScope := nil;
    currModule^.imports := AddToImportList(currModule^.imports,imp);
    Import := imp;
end;

procedure EndImport(imp : ImportNode);
var
    modSym : Symbol;
begin
    if imp^.saveModule <> nil then begin
	{ just returned from a declaration module }
	{ restore to normal mode }
	if currModule <> globalModule then begin
	    ErrorName(imp^.currSearch^.name,'Missing end in imported module');
	end else begin
	    modSym := LookUpSymbol(imp^.currSearch^.name,currScope,ONECASE);
	    if modSym = nil then begin
		ErrorName(imp^.currSearch^.name,'Did not find expected module in import file');
	    end;
	end;
	currModule := imp^.saveModule;
	currScope := imp^.saveScope;
	imp^.saveModule := nil;
	imp^.saveScope := nil;
	imp^.currSearch := imp^.currSearch^.next;
    end;
end;

function ReadImport{(imp : ImportNode) : ImportNode};
var
    modSym : Symbol;
    fileString : String;
    fileName : FileName;
    inMemory : boolean;
    i : integer;
begin
    if TraceDecls then begin
	writeln(output,'ReadImport: continue=',imp^.saveModule<>nil);
    end;
    EndImport(imp);
    inMemory := true;
    while inMemory and (imp^.currSearch <> nil) do begin
	if TraceDecls then begin
	    write(output,'Import module ');
	    WriteString(output,imp^.currSearch^.name);
	    writeln(output);
	end;
	modSym := LookUpSymbol(imp^.currSearch^.name,currScope^.enclosing,ONECASE);
	if modSym <> nil then begin
	    { found, make sure it's a module }
	    if modSym^.kind = SYMMODULE then begin
		if modSym^.symModule^.doingImport then begin
		    ErrorName(modSym^.name,'Recursive import of module');
		end;
	    end;
	    imp^.currSearch := imp^.currSearch^.next;
	end else if currModule^.enclosing = globalModule then begin
	    { not found, look for file }
	    if TraceDecls then begin
		writeln(output,' Looking for file');
	    end;
	    assert(currProc = globalProc);

	    i := 0;
	    while (i < imp^.currSearch^.name^.length)
		and (i < FILENAMESIZE-4)
	    do begin
		AddChar(GetChar(imp^.currSearch^.name,i));
		i := i + 1;
	    end;
	    AddText('.def');
	    fileString := NewString;

	    { Look for external module }
	    if not InitFile(fileString) then begin
		ErrorName(fileString,'Cannot find file for imported module');
		imp^.currSearch := imp^.currSearch^.next;
	    end else begin
		{ save state of current module }
		imp^.saveModule := currModule;
		imp^.saveScope := currScope;
		currModule := globalModule;
		currScope := globalModule^.scope;
		{ continue parsing with definition module }
		inMemory := false;
	    end;
	end else begin
	    { non-global module, must be defined later }
	    imp^.currSearch := imp^.currSearch^.next;
	end;
    end;
    if TraceDecls then begin
	writeln(output,'ReadImport: exit');
    end;
    ReadImport := imp;
end;

procedure ProcessImport{(imp : ImportNode; complain : boolean)};
var
    id, idnext : IdentNode;
    sym, nsym, msym : Symbol;
    scope : Scope;
    remainder : IdentList;
    fromModule, mn : ModuleNode;
    qualified : boolean;
begin
    if TraceDecls then begin
	writeln(output,'ProcessImport: start');
    end;
    EndImport(imp);
    currFile := imp^.fileName;
    currLine := imp^.lineNumber;
    scope := nil;
    if imp^.fromIdent <> nil then begin
	{ import from }
	sym := LookUpSymbol(imp^.fromIdent,currScope^.enclosing,ONECASE);
	if sym = nil then begin
	    if complain then begin
		ErrorName(imp^.fromIdent,'Module not found for import');
	    end;
	end else if sym^.kind <> SYMMODULE then begin
	    if complain then begin
		ErrorName(imp^.fromIdent,'Import "from" is not a module');
	    end;
	end else begin
	    scope := sym^.symModule^.exportScope;
	    fromModule := sym^.symModule;
	    qualified := true;
	    { if module exists, complain on first pass, not on second }
	    complain := not complain;
	end;   
    end else begin
	scope := currScope^.enclosing;
	fromModule := currModule^.enclosing;
	qualified := false;
    end;
    if scope = nil then begin
	{ do nothing }
    end else if imp^.idents <> nil then begin
	remainder := nil;
	id := imp^.idents^.first;
	while id <> nil do begin
	    idnext := id^.next;
	    sym := LookUpSymbol(id^.name,scope,ONECASE);
	    if sym = nil then begin
		if complain then begin
		    ErrorName(id^.name,'Not found on import');
		end;
		remainder := AddToIdentList(remainder,id);
	    end else begin
		nsym := Port(sym,nil);    
		if currModule^.enclosing = globalModule then begin
		    GlobalPort(sym,fromModule,currModule,qualified);
		    if sym^.kind = SYMMODULE then begin
			mn := sym^.symModule;
			if mn^.qualExports <> nil then begin
			    id := mn^.qualExports^.first;
			    while id <> nil do begin
				msym := LookUpSymbol(id^.name,mn^.exportScope,
					    ONECASE);
				if msym = nil then begin
				    if complain then begin
					ErrorName(id^.name,'Not found on import');
				    end;
				end else begin
				    GlobalPort(msym,mn,currModule,true);
				end;
				id := id^.next;
			    end;
			end;
			if mn^.unqualExports <> nil then begin
			    id := mn^.unqualExports^.first;
			    while id <> nil do begin
				msym := LookUpSymbol(id^.name,mn^.exportScope,
					    ONECASE);
				if msym = nil then begin
				    if complain then begin
					ErrorName(id^.name,'Not found on import');
				    end;
				end else begin
				    GlobalPort(msym,mn,currModule,false);
				end;
				id := id^.next;
			    end;
			end;
		    end;
		end;
	    end;
	    id := idnext;
	end;
	imp^.idents := remainder;
    end;
end;

procedure Export{(idents : IdentList; qualToken : Token)};
begin
    if (qualToken = TKQUALIFIED) or
      ((currModule^.enclosing = globalModule) and (qualToken <> TKUNQUALIFIED))
    then begin
	currModule^.qualExports := AppendIdentList(currModule^.qualExports,
					idents);
    end else begin
	currModule^.unqualExports := AppendIdentList(currModule^.unqualExports,
					idents);
    end;
end;

function MakeConstSet {(lower, upper : ConstNode) : ConstSetNode};
var
    cln : ConstSetNode;
    error : boolean;
begin
    error := false;
    cln := nil;
    if lower = nil then begin
	{ do nothing }
    end else begin
	if upper = nil then begin
	    upper := lower;
	end else if (lower^.kind <> upper^.kind) and
	    not ((lower^.kind in [DTCARDINAL, DTINTEGER])
		and (upper^.kind in [DTCARDINAL,DTINTEGER]))
	then begin
	    error := true;
	end else if lower^.kind = DTENUMERATION then begin
	    if lower^.enumVal^.enumType <> upper^.enumVal^.enumType then begin
		error := true;
	    end;
	end;
	if error then begin
	    Error('Lower bound and upper bound of range must be same type');
	end else if not (lower^.kind in indexableTypes) then begin
	    ErrorName(stringDataType[lower^.kind],'Range type must be indexable');
	end else begin
	    new(cln);
	    cln^.lower := lower;
	    cln^.upper := upper;
	end;
    end;
    MakeConstSet := cln;
end;

function AddToConstSetList {(list : ConstSetList; newOne : ConstSetNode)
	: ConstSetList};
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
    AddToConstSetList := list;
end;

function AddToModuleList {(list : ModuleList; newOne : ModuleNode) : ModuleList};
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
    AddToModuleList := list;
end;

function AddToProcList {(list : ProcList; newOne : ProcNode) : ProcList};
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
    AddToProcList := list;
end;

function AddToFieldList {(list : FieldList; newOne : FieldNode) : FieldList};
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
    AddToFieldList := list;
end;

function AddToVariantList {(list : VariantList; newOne : VariantNode) : VariantList};
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
    AddToVariantList := list;
end;

function AddToEnumList {(list : EnumList; newOne : EnumNode) : EnumList};
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
    AddToEnumList := list;
end;

function AddToImportList {(list : ImportList; newOne : ImportNode) : ImportList};
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
    AddToImportList := list;
end;

function AddToIdentList {(list : IdentList; newOne : IdentNode) : IdentList};
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
    AddToIdentList := list;
end;

function AddToVarList {(list : VarList; newOne : VarNode) : VarList};
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
    AddToVarList := list;
end;

function AddToPortList {(list : PortList; newOne : PortNode) : PortList};
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
    AddToPortList := list;
end;

function BaseType {(tn : TypeNode) : TypeNode};
var
    bt : TypeNode;
    found : boolean;
begin
    found := false;
    bt := tn;
    while (bt <> nil) and not found do begin
	if bt^.kind = DTRENAME then begin
	    if bt^.renameType <> nil then begin
		bt := bt^.renameType;
	    end else begin
		found := true;
	    end;
	end else if bt^.kind = DTSUBRANGE then begin
	    bt := bt^.baseType;
	end else begin
	    found := true;
	end;
    end;
    BaseType := bt;
end;

function ActualType {(tn : TypeNode) : TypeNode};
var
    at : TypeNode;
    found : boolean;
begin
    found := false;
    at := tn;
    while not found and (at <> nil) do begin
	if at^.kind <> DTRENAME then begin
	    found := true;
	end else if at^.renameType = nil then begin
	    found := true;
	end else if (at^.size <> at^.renameType^.size) or
		((at^.alignment <> -1) and
			(at^.alignment <> at^.renameType^.alignment))
	then begin
	    found := true;
	end else begin
	    at := at^.renameType;
	end;
    end;
    ActualType := at;
end;

{ NumberOf returns the number of elements in a range }
{	0 if nil (unbounded array parameter), -1 if invalid type }
function NumberOf {(tn : TypeNode) : cardinal};
var
    at : TypeNode;
    count : cardinal;
    found : boolean;
begin
    found := false;
    at := tn;
    while (at <> nil) and not found do begin
	if at^.kind <> DTRENAME then begin
	    found := true;
	end else begin
	    at := at^.renameType;
	end;
    end;
    if at = nil then begin
	count := 0;
    end else begin
	if at^.kind = DTSUBRANGE then begin
	    count := at^.subMaxOrd - at^.subMinOrd + 1;
	end else if at^.kind = DTBOOLEAN then begin
	    count := 2;
	end else if at^.kind = DTCHAR then begin
	    count := 256;
	end else if at^.kind = DTENUMERATION then begin
	    count := at^.enumCount;
	end else begin
	    count := -1;
	end;
    end;
    NumberOf := count;
end;

{ LowerBoundOf returns the value of the first element in the range }
function LowerBoundOf {(tn : TypeNode) : cardinal};
var
    at : TypeNode;
    low : cardinal;
    found : boolean;
begin
    found := false;
    at := tn;
    while (at <> nil) and not found do begin
	if at^.kind <> DTRENAME then begin
	    found := true;
	end else begin
	    at := at^.renameType;
	end;
    end;
    if at = nil then begin
	low := 0;
    end else if at^.kind = DTSUBRANGE then begin
	low := at^.subMinOrd;
    end else if at^.kind = DTINTEGER then begin
	low := -MAXINT-1;
    end else if at^.kind in [DTCARDINAL, DTENUMERATION, DTCHAR, DTBOOLEAN]
    then begin
	low := 0;
    end else begin
	low := 0;
    end;
    LowerBoundOf := low;
end;

{ UpperBoundOf returns the value of the last element in a range }
function UpperBoundOf {(tn : TypeNode) : cardinal};
var
    at : TypeNode;
    up : cardinal;
    found : boolean;
begin
    found := false;
    at := tn;
    while (at <> nil) and not found do begin
	if at^.kind <> DTRENAME then begin
	    found := true;
	end else begin
	    at := at^.renameType;
	end;
    end;
    if at = nil then begin
	up := -1;
    end else if at^.kind = DTSUBRANGE then begin
	up := at^.subMaxOrd;
    end else if at^.kind = DTINTEGER then begin
	up := MAXINT;
    end else if at^.kind = DTCARDINAL then begin
	up := MAXCARD;
    end else if at^.kind = DTBOOLEAN then begin
	up := 1;
    end else if at^.kind = DTCHAR then begin
	up := 255;
    end else if at^.kind = DTENUMERATION then begin
	up := at^.enumCount-1;
    end else begin
	up := -1;
    end;
    UpperBoundOf := up;
end;

function AlignmentOf{(tn : TypeNode) : cardinal};
var
    alignment : cardinal;
    atn : TypeNode;
begin
    atn := tn;
    alignment := -1;
    while (atn <> nil) and (alignment = -1) do begin
	alignment := atn^.alignment;
	if atn^.kind = DTRENAME then begin
	    atn := atn^.renameType;
	end else if atn^.kind = DTSUBRANGE then begin
	    atn := atn^.baseType;
	end else begin
	    atn := nil;
	end;
    end;
    if alignment = -1 then begin
	case target of
	    TARGETVAX : begin
		if tn^.size >= WORDSIZE then begin
		    alignment := WORDSIZE;
		end else begin
		    alignment := 1;
		    while (alignment < tn^.size) do begin
			alignment := alignment * 2;
		    end;
		end;
	    end;
	    TARGETTITAN : begin
		alignment := WORDSIZE;
	    end;
	end;
    end;
    AlignmentOf := alignment;
end;

function SizeOf{(tn : TypeNode) : cardinal};
begin
    { fix AlignmentOf if this is changed }
    SizeOf := tn^.size;
end;

function WordSizeOf{(tn : TypeNode) : cardinal};
begin
    WordSizeOf := RoundUp(tn^.size,WORDSIZE);
end;


procedure CheckEqualType{(sym : Symbol; tn : TypeNode)};
var
    symTn : TypeNode;
begin
    symTn := ActualType(sym^.symType);
    tn := ActualType(tn);
    if symTn = tn then begin
	{ do nothing }
    end else if (symTn^.kind = DTRENAME) and (symTn^.renameType = nil)
    then begin
	symTn^.renameType := tn;
	symTn^.size := tn^.size;
	symTn^.alignment := tn^.alignment;
	if symTn^.name = tn^.name then begin
	    symTn^.name := nil;
	end;
    end else if symTn^.kind = DTOPAQUE then begin
	if tn^.size <> WORDSIZE then begin
	    ErrorName(sym^.name,'Size of actual type for opaque type must be one word');
	end;
	symTn^.kind := DTRENAME;
	symTn^.renameType := tn;
	if symTn^.name = tn^.name then begin
	    tn^.name := nil;
	end;
    end else begin
	ErrorName(sym^.name,'Redefined type');
    end;
end;

procedure CheckEqualVar{(sym : Symbol; tn : TypeNode)};
begin
    ErrorName(sym^.name,'Redefined variable');
end;

function SameTypeParam{(dst, src : Typenode) : boolean};
var
    same : boolean;
begin
    same := false;
    src := ActualType(src);
    dst := ActualType(dst);
    if dst = src then begin
	same := true;
    end else if (dst = nil) or (src = nil) then begin
	{ do nothing }
    end else if (dst^.kind = DTARRAY) and (src^.kind = DTARRAY) then begin
	if (dst^.elementType = src^.elementType) and (dst^.indexType = nil)
	    and (src^.indexType = nil)
	then begin
	    same := true;
	end;
    end; 
    SameTypeParam := same;
end;

procedure CheckEqualProc{(proc : ProcNode; procType : TypeNode)};
var
    pn1, pn2 : ParamNode;
    error : boolean;
begin
    error := false;
    if proc^.procType^.paramList = nil then begin
	pn1 := nil;
    end else begin
	pn1 := proc^.procType^.paramList^.first;
    end;
    if procType^.paramList = nil then begin
	pn2 := nil;
    end else begin
	pn2 := procType^.paramList^.first;
    end;
    while not error and (pn1 <> nil) and (pn2 <> nil) do begin
	if (pn1^.name <> pn2^.name) then begin
	    if not standardKeywordFlag then begin
		error := true;
	    end else begin
		pn1^.name := pn2^.name;
	    end;
	end;
	if (pn1^.kind <> pn2^.kind) or
		not SameTypeParam(pn1^.paramType,pn2^.paramType)
	then begin
	    error := true;
	end;
	pn1 := pn1^.next;
	pn2 := pn2^.next;
    end;
    error := error or (pn1 <> nil) or (pn2 <> nil) or
       (ActualType(proc^.procType^.funcType) <> ActualType(procType^.funcType));
    if error then begin
	ErrorName(proc^.name,
	    'Redefinition of procedure not identical to original');
    end;
end;

procedure ErrorMissingIdent;
begin
    Error('Missing identifier on procedure/module end');
end;

procedure ErrorExtraSemicolon;
begin
    Error('Extra semi-colon');
end;

procedure ErrorMissingSemicolon;
begin
    Error('Missing semi-colon');
end;

procedure ErrorModuleDot;
begin
    Error('Global module must end with a period.');
end;
