(*#@(#)dbstab.p	4.1	Ultrix	7/17/90 *)
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
$Header: dbstab.p,v 1.5 84/05/19 11:37:56 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "pcode.h"
#include "optim.h"
#include "ocount.h"
#include "genpcf.h"
#include "genpc.h"
#include "dbstab.h"
#include "decls.h"

const
    STABNMOD2 = 80;	(* same as N_MOD2 in /usr/include/stab.h *)

    STABSOURCEFILE = 100;
    STABSYMBOL = 128;
    STABGLOBAL = 32;
    STABPARAM = 160;
    STABLINE = 68;
    STABPROC = 36;
    STABTOKENSPERLINE = 10;
    MAXBUILTINTYPES = 20;
var
    generateTypeNumber : TypeNumber;
    stabFileName : String;
    stabLineNumber : integer;
    stabTokenCount : integer;
    inTypeDef : boolean;

function NewTypeNumber : TypeNumber;
begin
    generateTypeNumber := generateTypeNumber + 1;
    NewTypeNumber := generateTypeNumber;
end;

procedure StabEndLine;
begin
    EndLine;
    stabTokenCount := 0;
end;

procedure StabComma;
begin
    GenChar(',');
end;

procedure StabSemicolon;
begin
    GenChar(';');
end;

procedure StartTypeDef;
begin
    GenOp(PCSYM); GenChar('t'); StabComma; GenChar('''');
end;

procedure EndTypeDef;
begin
    GenChar(''''); StabComma; GenInteger(STABSYMBOL); Comma;
    GenInteger(0); StabComma; GenInteger(0); Comma;
    GenInteger(0);
    StabEndLine;
end;

procedure StabCommaX;
begin
    GenChar(',');
    stabTokenCount := stabTokenCount + 1;
    if inTypeDef and (stabTokenCount > STABTOKENSPERLINE) then begin
	GenChar('?');
	EndTypeDef;
	StartTypeDef;
    end;
end;

procedure StabSemicolonX;
begin
    GenChar(';');
    stabTokenCount := stabTokenCount + 1;
    if inTypeDef and (stabTokenCount > STABTOKENSPERLINE) then begin
	GenChar('?');
	EndTypeDef;
	StartTypeDef;
    end;
end;

function NamedType(tn : TypeNode) : TypeNode;
var
    ntn : TypeNode;
    found : boolean;
begin
    found := false;
    ntn := tn;
    while not found and (ntn <> nil) do begin
	found := true;
	if (ntn^.opaqueName = nil) and (ntn^.module = nil) and
		(ntn^.kind = DTRENAME)
	then begin
	    if ntn^.renameType <> nil then begin
		ntn := ntn^.renameType;
		found := false;
	    end;
	end;
    end;
    NamedType := ntn;
end;

procedure InitStab;
var
    fileLabel : LabelNumber;
    tn : TypeNode;
begin
    generateTypeNumber := 0;
    stabTokenCount := 0;
    integerTypeNode^.number := NewTypeNumber;
    charTypeNode^.number := NewTypeNumber;
    booleanTypeNode^.number := NewTypeNumber;
    cardinalTypeNode^.number := NewTypeNumber;
    realTypeNode^.number := NewTypeNumber;
    longrealTypeNode^.number := NewTypeNumber;
    wordTypeNode^.number := NewTypeNumber;
    byteTypeNode^.number := NewTypeNumber;
    addressTypeNode^.number := NewTypeNumber;
    fileTypeNode^.number := NewTypeNumber;
    processTypeNode^.number := NewTypeNumber;
    cardIntTypeNode^.number := NewTypeNumber;
    generateTypeNumber := MAXBUILTINTYPES;
    inTypeDef := false;

    fileLabel := NewLabel;
    GenOp(PCSYM); GenChar('s'); StabComma; 
	GenChar(''''); GenString(mainFileName); GenChar(''''); StabComma;
	GenInteger(STABSOURCEFILE); StabComma; GenInteger(0); 
	StabComma; GenInteger(0); Comma; GenLabel(fileLabel); 
    StabEndLine;
    GenLabel(fileLabel); GenOpL(PCLAB);

end;

procedure StabFieldList(fl : FieldList);
var
    fn : FieldNode;
    vn : VariantNode;
begin
    fn := fl^.first;
    while fn <> nil do begin
	if fn^.name <> nil then begin
	    GenString(fn^.name);
	    GenChar(':');
	    StabTypeNumber(fn^.fieldType);
	    StabComma;
	    GenInteger(fn^.offset);
	    StabComma;
	    GenInteger(SizeOf(fn^.fieldType));
	    StabSemicolonX;
	end;
	if fn^.kind = FIELDVARIANT then begin
	    vn := fn^.variantList^.first;
	    while vn <> nil do begin
		StabFieldList(vn^.fieldList);
		vn := vn^.next;
	    end;
	end;
	fn := fn^.next;
    end;
end;

procedure StabProcType(tn : TypeNode);
var
    param : ParamNode;
    ptn : TypeNode;
begin
    if tn^.funcType = nil then begin
	GenChar('p');
    end else begin
	GenChar('f'); StabTypeNumber(tn^.funcType); Comma;
    end;
    GenInteger(tn^.numParams); StabSemicolon;
    if tn^.paramList <> nil then begin
	param := tn^.paramList^.first;
	while param <> nil do begin
	    if param^.name <> nil then begin
		GenString(param^.name); GenChar(':');
	    end;
	    ptn := param^.paramType;
	    StabTypeNumber(ptn);
	    StabComma;
	    GenInteger(ord(param^.kind)); StabSemicolon;
	    param := param^.next;
	end;
    end;
    StabSemicolon;
end;

procedure StabTypeDef{(tn : TypeNode)};
var
    enum : EnumNode;
begin
    tn := NamedType(tn);
    if tn^.opaqueName <> nil then begin
	GenChar('o');
	GenString(tn^.opaqueName);
	if tn^.kind <> DTOPAQUE then begin
	    StabComma;
	end;
    end else if tn^.module <> nil then begin
	{ indirect type name }
	GenChar('i');
	GenString(tn^.module^.name);
	GenChar(':');
	GenString(tn^.name);
	StabComma;
    end;
    case tn^.kind of
	DTINTEGER : StabTypeNumber(integerTypeNode);
	DTCHAR : StabTypeNumber(charTypeNode);
	DTBOOLEAN : StabTypeNumber(booleanTypeNode);
	DTREAL : StabTypeNumber(realTypeNode);
	DTLONGREAL : StabTypeNumber(longrealTypeNode);
	DTCARDINAL : StabTypeNumber(cardinalTypeNode);
	DTNULL, DTSTRING, DTANY, { should not occur }
	DTWORD : StabTypeNumber(wordTypeNode);
	DTBYTE : StabTypeNumber(byteTypeNode);
	DTOPAQUE : ;
	DTRENAME : begin { treat like opaque type }
	    StabTypeNumber(tn^.renameType);
	end;
	DTPOINTER : begin
	    GenChar('*');
	    StabTypeNumber(tn^.toType);
	end;
	DTPROC : begin
	    StabProcType(tn);
	end;
	DTSET :	begin
	    GenChar('S');
	    StabTypeNumber(tn^.setRange);
	end;
	DTSUBRANGE : begin
	    GenChar('r');
	    StabTypeNumber(tn^.baseType);
	    StabSemicolon;
	    GenInteger(tn^.subMinOrd);
	    StabSemicolon;
	    GenInteger(tn^.subMaxOrd);
	end;
	DTRECORD : begin
	    GenChar('s');
	    GenInteger(CardDiv(SizeOf(tn),BYTESIZE));
	    StabFieldList(tn^.fieldList);
	    StabSemicolon;
	end;
	DTARRAY : begin
	    if tn^.indexType = nil then begin
		GenChar('A');
		StabTypeNumber(tn^.elementType);
	    end else begin
		GenChar('a');
		StabTypeNumber(tn^.indexType);
		StabSemicolon;
		StabTypeNumber(tn^.elementType);
	    end;
	end;
	DTENUMERATION : begin
	    GenChar('e');
	    enum := tn^.enumList^.first;
	    while enum <> nil do begin
		GenString(enum^.name);
		GenChar(':');
		GenInteger(enum^.enumOrd);
		StabCommaX;
		enum := enum^.next;
	    end;
	    StabSemicolon;
	end;
    end;
    if tn^.module <> nil then begin
	StabSemicolon;
    end else if tn^.opaqueName <> nil then begin
	StabSemicolon;
    end;
end;

procedure StabCheckType(tn : TypeNode);
begin
    if TraceStab then begin
	write(output,'StabCheckType: ');
	if tn = nil then begin
	    write(output,'tn=nil');
	end else begin
	    write(output,tn^.kind,' ',tn^.number,' ');
	    WriteString(output,tn^.name);
	end;
	writeln(output);
    end;
    if tn = nil then begin
	{ do nothing }
    end else if tn^.number = 0 then begin
	StabNamedType(tn^.name,tn);
    end else if tn^.kind = DTRENAME then begin
	StabCheckType(tn^.renameType);
    end;
end;

procedure StabCheckFieldList(fl : FieldList);
var
    fn : FieldNode;
    vn : VariantNode;
begin
    fn := fl^.first;
    while fn <> nil do begin
	if TraceStab then begin
	    write(output,'StabCheckFieldList: ');
	    WriteString(output,fn^.name);
	    writeln(output);
	end;
	StabCheckType(fn^.fieldType);
	if fn^.kind = FIELDVARIANT then begin
	    vn := fn^.variantList^.first;
	    while vn <> nil do begin
		StabCheckFieldList(vn^.fieldList);
		vn := vn^.next;
	    end;
	end;
	fn := fn^.next;
    end;
end;

procedure StabCheckProcType(tn : TypeNode);
var
    pn : ParamNode;
begin
    StabCheckType(tn^.funcType);
    if tn^.paramList <> nil then begin
	pn := tn^.paramList^.first;
	while pn <> nil do begin
	    StabCheckType(pn^.paramType);
	    pn := pn^.next;
	end;
    end;
end;


{ try to make sure dependent types are output before this one is }
procedure StabNamedType{(name : String; tn : TypeNode)};
var
    enum : EnumNode;
begin
    if TraceStab then begin
	write(output,'StabNamedType: ',tn^.kind,' ',tn^.number,' ');
	WriteString(output,name);
	writeln(output);
    end;
    tn := NamedType(tn);
    if tn^.number = 0 then begin
	tn^.number := NewTypeNumber;
	case tn^.kind of
	    DTINTEGER,
	    DTCHAR,
	    DTBOOLEAN,
	    DTREAL,
	    DTLONGREAL,
	    DTCARDINAL,
	    DTNULL, DTSTRING, DTANY,
	    DTWORD, DTBYTE : begin
		{ these are already defined }
	    end;
	    DTOPAQUE,
	    DTENUMERATION : { nothing to do };
	    DTRENAME : begin
		StabCheckType(tn^.renameType);	
	    end;
	    DTPOINTER : begin
		StabCheckType(tn^.toType);
	    end;
	    DTPROC : begin
		StabCheckProcType(tn);
	    end;
	    DTSET : begin
		StabCheckType(tn^.setRange);
	    end;
	    DTSUBRANGE : begin
		StabCheckType(tn^.baseType);
	    end;
	    DTRECORD : begin
		StabCheckFieldList(tn^.fieldList);
	    end;
	    DTARRAY : begin
		if tn^.indexType <> nil then begin
		    StabCheckType(tn^.indexType);
		end;
		StabCheckType(tn^.elementType);
	    end;
	end;
	StartTypeDef;
	    GenString(name); GenChar(':'); GenChar('t');
	    GenInteger(tn^.number);
	    GenChar('=');
	    inTypeDef := true;
	    StabTypeDef(tn);
	    inTypeDef := false;
	EndTypeDef;
    end;
end;

procedure StabTypeNumber{(tn : TypeNode)};
begin
    if TraceStab then begin
	write(output,'StabTypeNumber: ',tn^.kind,' ',tn^.number,' ');
	WriteString(output,tn^.name);
	writeln(output);
    end;
    tn := NamedType(tn);
    if TraceStab then begin
	write(output,'StabTypeNumber(Actual): ',tn^.kind,' ',tn^.number,' ');
	WriteString(output,tn^.name);
	writeln(output);
    end;
    if tn = nil then begin
	GenInteger(0);
    end else if tn^.number <> 0 then begin
	GenInteger(tn^.number);
    end else begin
	tn^.number := NewTypeNumber;
	GenInteger(tn^.number);
	GenChar('=');
	StabTypeDef(tn);
    end;
end;

procedure StabQualifiers(module : ModuleNode; proc : ProcNode; last : boolean);
begin
    if (module = nil) or
	((module = globalModule) and ((proc = globalProc) or (proc = nil)))
    then begin
	{ do nothing }
    end else if proc = nil then begin
	{ global thing, just module qualifiers }
	StabQualifiers(module^.enclosing,proc,false);
	GenString(module^.name);
	if not last then begin
	    GenChar(':');
	end;
    end else if module^.enclosingProc = proc then begin
	{ next level is a module }
	StabQualifiers(module^.enclosing,proc,false);
	GenString(module^.name);
	if not last then begin
	    GenChar(':');
	end;
    end else if proc^.enclosingModule = module then begin
	{ next level is a proc }
	StabQualifiers(module,proc^.enclosing,false);
	GenString(proc^.name);
	if not last then begin
	    GenChar(':');
	end;
    end else begin
	ErrorName(module^.name,'Module/proc list confused');
	ErrorName(proc^.name,'Module/proc list confused');
    end;
end;

procedure StabProc{(proc : ProcNode)};
var
    tn : TypeNode;
    pn : ParamNode;
begin
    if genDebugInfoFlag then begin
	tn := proc^.procType^.funcType;
	GenOp(PCSYM); GenChar('F'); StabComma; GenChar('''');
	    if proc^.name = nil then begin
		GenText(MODULEINITNAME);
	    end else begin
		GenString(proc^.name);
	    end;
	    GenChar(':');
	    if tn <> nil then begin
		StabCheckType(tn);
		if proc^.internalProc then begin
		    GenChar('J');
		end else begin
		    GenChar('F');
		end;
		StabTypeNumber(tn);
	    end else begin
		if proc^.internalProc then begin
		    GenChar('I');
		end else begin
		    GenChar('P');
		end;
	    end;
	    StabComma;
	    GenProcName(proc);
	    StabComma;
	    if proc^.name = nil then begin
		GenString(proc^.enclosingModule^.name);
	    end else begin
		StabQualifiers(proc^.enclosingModule,proc^.enclosing,true);
	    end;
	    GenChar(''''); StabComma; GenInteger(STABPROC); Comma;
	    GenInteger(0); StabComma;
	    if tn <> nil then begin
		GenInteger(SizeOf(tn));
	    end else begin
		GenInteger(0);
	    end;
	    StabComma;
	    GenProcName(proc);
	StabEndLine;
	if proc^.procType^.paramList <> nil then begin
	    pn := proc^.procType^.paramList^.first;
	    while pn <> nil do begin
		GenOp(PCSYM); GenChar('p'); StabComma;
		    GenChar(''''); GenString(pn^.name); GenChar(':');
		    if pn^.reference then begin
			GenChar('v');
		    end else begin
			GenChar('p');
		    end;
		    StabTypeNumber(pn^.paramType); GenChar('''');
		    StabComma; GenInteger(STABPARAM); Comma;
		    GenInteger(0); StabComma; GenInteger(SizeOf(pn^.paramType));
		    StabComma; GenAddress(pn^.paramVar^.address);
		StabEndLine;
		pn := pn^.next;
	    end;
	end;
    end;
end;

procedure StabModule{(module : ModuleNode)};
begin
    if genDebugInfoFlag then begin
	GenOp(PCSYM); GenChar('m'); StabComma; GenChar('''');
	    GenString(module^.name); GenChar(':'); GenChar('m');
	    StabComma;
	    StabQualifiers(module^.enclosing,module^.enclosingProc,true);
	    GenChar(''''); StabComma; GenInteger(STABSYMBOL); Comma;
	    GenInteger(0); StabComma; GenInteger(0);
	    StabComma; GenInteger(0);
	StabEndLine;
    end;
end;


procedure StabScope{(scope : Scope)};
var
    sym : Symbol;
    tn : TypeNode;
begin
    if genDebugInfoFlag then begin
	sym := scope^.symbols^.first;
	while sym <> nil do begin
	    if sym^.kind = SYMVAR then begin
		tn := NamedType(sym^.symVar^.varType);
		StabCheckType(tn);
		case sym^.symVar^.address.kind of
		    MEMGLOBAL : begin
			GenOp(PCSYM); GenChar('G'); StabComma; GenChar('''');
			    GenString(sym^.name); GenChar(':');
			    if sym^.symVar^.address.gvn^.extern then begin
				GenChar('G');
			    end else begin
				GenChar('V');
			    end;
			    StabTypeNumber(tn); StabComma;
			    GenString(sym^.symVar^.address.gvn^.name);
			    GenChar(''''); StabComma; GenInteger(STABGLOBAL); Comma;
			    GenInteger(0); StabComma; GenInteger(SizeOf(tn));
			    StabComma; GenInteger(0);
			StabEndLine;
		    end;
		    MEMNORMAL, MEMFAST : begin
			GenOp(PCSYM); GenChar('v'); StabComma;
			    GenChar(''''); GenString(sym^.name); GenChar(':');
			    StabTypeNumber(tn); GenChar(''''); StabComma;
			    GenInteger(STABSYMBOL); StabComma; GenInteger(0); Comma;
			    GenInteger(SizeOf(tn)); StabComma;
			    GenAddress(sym^.symVar^.address);
			StabEndLine;
		    end;
		    MEMPARAM : begin
		    end;
		end;
	    end else if sym^.kind = SYMTYPE then begin
		StabNamedType(sym^.name,sym^.symType);
	    end;
	    sym := sym^.next;
	end;
    end;
end;

procedure StabGlobalPort;
var
    pn : PortNode;
    tn : TypeNode;
    cn : ConstNode;
    i : integer;
begin
    GenOp(PCSYM); GenChar('X'); StabComma;
	GenChar('''');
	GenString(compileModuleName); GenChar(':'); GenChar('X');
	GenInteger(0); GenInteger(0); GenInteger(0);
	GenChar('m'); GenChar(''''); StabComma;
	GenInteger(STABNMOD2); StabComma; GenInteger(0); Comma;
	GenInteger(0); StabComma; GenInteger(0);
    StabEndLine;
    pn := globalPortList^.first;
    while pn <> nil do begin
	if pn^.sym^.kind = SYMVAR then begin
	    tn := pn^.sym^.symVar^.varType;
	    StabCheckType(tn);
	    GenOp(PCSYM); GenChar('X'); StabComma;
		GenChar('''');
		GenString(pn^.module^.name); GenChar('_');
		GenString(pn^.sym^.name); GenChar(':'); GenChar('X');
		GenInteger(ord(pn^.qualified)); GenInteger(ord(pn^.export));
		GenInteger(ord(pn^.extern));
		GenChar('v'); StabTypeNumber(tn); GenChar(''''); StabComma;
		GenInteger(STABNMOD2); StabComma; GenInteger(0); Comma;
		GenInteger(0); StabComma; GenInteger(0);
	    StabEndLine;
	end else if pn^.sym^.kind = SYMPROC then begin
	    tn := pn^.sym^.symProc^.procType;
	    StabCheckProcType(tn);
	    GenOp(PCSYM); GenChar('X'); StabComma;
		GenChar('''');
		GenString(pn^.module^.name); GenChar('_');
		GenString(pn^.sym^.name); GenChar(':'); GenChar('X');
		GenInteger(ord(pn^.qualified)); GenInteger(ord(pn^.export));
		GenInteger(ord(pn^.extern));
		StabProcType(tn); GenChar(''''); StabComma;
		GenInteger(STABNMOD2); StabComma; GenInteger(0); Comma;
		GenInteger(0); StabComma; GenInteger(0);
	    StabEndLine;
	end else if pn^.sym^.kind = SYMTYPE then begin
	    tn := pn^.sym^.symType;
	    StabCheckType(tn);
	    GenOp(PCSYM); GenChar('X'); StabComma;
		GenChar('''');
		GenString(pn^.module^.name); GenChar('_');
		GenString(pn^.sym^.name); GenChar(':'); GenChar('X');
		GenInteger(ord(pn^.qualified)); GenInteger(ord(pn^.export));
		GenInteger(ord(pn^.extern));
		GenChar('t'); StabTypeNumber(tn); GenChar(''''); StabComma;
		GenInteger(STABNMOD2); StabComma; GenInteger(0); Comma;
		GenInteger(0); StabComma; GenInteger(0);
	    StabEndLine;
	end else if pn^.sym^.kind = SYMCONST then begin
	    cn := pn^.sym^.symConst;
	    if cn^.kind = DTENUMERATION then begin
		tn := cn^.enumVal^.enumType;
		StabCheckType(tn);
	    end else if cn^.kind = DTSET then begin
		tn := cn^.setVal^.setType;
		StabCheckType(tn);
	    end;
	    GenOp(PCSYM); GenChar('X'); StabComma;
		GenChar('''');
		GenString(pn^.module^.name); GenChar('_');
		GenString(pn^.sym^.name); GenChar(':'); GenChar('X');
		GenInteger(ord(pn^.qualified)); GenInteger(ord(pn^.export));
		GenInteger(ord(pn^.extern));
		GenChar('c'); GenChar('=');
		case cn^.kind of
		    DTREAL, DTLONGREAL : begin
			GenChar('r');
			GenReal(cn^.realVal);
		    end;
		    DTINTEGER, DTCARDINAL : begin
			GenChar('i');
			GenInteger(cn^.cardVal);
		    end;
		    DTBOOLEAN : begin
			GenChar('b');
			GenInteger(ord(cn^.boolVal));
		    end;
		    DTCHAR : begin
			GenChar('c');
			GenInteger(cn^.charVal);
		    end;
		    DTSTRING : begin
			GenChar('s');
			GenChar('''');
			GenChar('''');
			WriteStringConst(codeFile,cn^.strVal);
			GenChar('''');
			GenChar('''');
		    end;
		    DTENUMERATION : begin
			GenChar('e');
			StabTypeNumber(tn); StabComma;
			GenInteger(cn^.enumVal^.enumOrd);
		    end;
		    DTSET : begin
			GenChar('S');
			StabTypeNumber(tn); StabComma;
			GenSet(cn^.setVal);
		    end;
		end;
		GenChar(';');
		GenChar(''''); StabComma;
		GenInteger(STABNMOD2); StabComma; GenInteger(0); Comma;
		GenInteger(0); StabComma; GenInteger(0);
	    StabEndLine;
	end;
	pn := pn^.next;
    end;
    GenOp(PCSYM); GenChar('X'); StabComma;
	GenChar('''');
	GenString(compileModuleName); GenChar(':'); GenChar('X');
	GenInteger(0); GenInteger(0); GenInteger(0);
	GenChar('z'); GenChar(''''); StabComma;
	GenInteger(STABNMOD2); StabComma; GenInteger(0); Comma;
	GenInteger(0); StabComma; GenInteger(0);
    StabEndLine;
end;

procedure StabLine{(fileName:String; lineNumber : integer)};
begin
    if genDebugInfoFlag then begin
	if fileName <> stabFileName then begin
	    { stab file name }
	    stabLineNumber := -1;
	end;
	if lineNumber <> stabLineNumber then begin
	    GenOp(PCSYM);
	    GenChar('l');
	    StabComma;
	    GenInteger(STABLINE);
	    StabComma;
	    GenInteger(0);
	    StabComma;
	    GenInteger(lineNumber);
	    StabEndLine;
	    stabLineNumber := lineNumber;
	end;
    end;
end;

