(*#@(#)genpc.p	4.1	Ultrix	7/17/90 *)
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
$Header: genpc.p,v 1.6 84/06/06 12:56:14 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "decls.h"
#include "pcode.h"
#include "optim.h"
#include "ocount.h"
#include "genpcf.h"
#include "genpc.h"
#include "alloc.h"
#include "const.h"
#include "dbstab.h"
#include "bexpr.h"
#include "cexpr.h"
#include "builtinpc.h"
#include "gencode.h"

var
    exitLabel : LabelNumber;
    currLevel : DisplayLevel;
    loopNestLevel : integer;
    genProc : ProcNode;

procedure GenComs;
var
    gvn : GlobalVarNode;
begin
    gvn := globalVarList^.first;
    while gvn <> nil do begin
	GenOp(PCCOM);
	GenInteger(gvn^.number);
	Comma;
	GenString(gvn^.name);
	Comma;
	GenInteger(CardDiv(RoundUp(gvn^.size,WORDSIZE),WORDSIZE));
	EndLine;
	gvn := gvn^.next;
    end;
end;

procedure GenTagList(csl : ConstSetList);
var
    csn : ConstSetNode;
    value : cardinal;
begin
    if csl <> nil then begin
	csn := csl^.first;
	while csn <> nil do begin
	    value := OrdOf(csn^.lower);
	    GenInteger(value);
	    if (csn^.upper <> nil) and (csn^.upper <> csn^.lower) then begin
		GenChar(':');
		value := OrdOf(csn^.upper);
		GenInteger(value);
	    end;
	    csn := csn^.next;
	    if csn <> nil then begin
		GenChar(';');
	    end;
	end;
    end;
end;

procedure GenExprCheck(expr : ExprNode; mode : EvalMode);
var
    checkFn, tagFn : FieldNode;
    vn : VariantNode;
    csn : ConstSetNode;
begin
    GenExpr(expr^.checkExpr,mode);
    if genCheckFlag then begin
	case expr^.exprCheck of
	    CHECKSUBSCR : begin
		GenOp(PCCHK); GenChar('s'); Comma; GenT(expr^.checkType); Comma;
			GenInteger(expr^.checkLower); Comma;
			GenInteger(expr^.checkUpper);
		EndLine;
	    end;
	    CHECKSUBSCROPEN : begin
		GenVar(expr^.checkVar,EVALPOINT);
		GenOp(PCCHK); GenChar('o'); EndLine;
	    end;
	    CHECKRANGE : begin
		GenOp(PCCHK); GenChar('r'); Comma; GenT(expr^.checkType); Comma;
			GenInteger(expr^.checkLower); Comma;
			GenInteger(expr^.checkUpper);
		EndLine;
	    end;
	    CHECKPTRMODULA : begin
		GenOp(PCCHK); GenChar('a'); Comma; GenChar('m'); EndLine;
	    end;
	    CHECKPTRPASCAL : begin
		GenOp(PCCHK); GenChar('a'); Comma; GenChar('p'); EndLine;
	    end;
	    CHECKPTRNIL : begin
		GenOp(PCCHK); GenChar('a'); Comma; GenChar('n'); EndLine;
	    end;
	    CHECKVARIANT : begin
		checkFn := expr^.checkField;
		vn := checkFn^.containingVariant;
		while vn <> nil do begin
		    tagFn := vn^.tagField;
		    if tagFn^.offset < 0 then begin
			{ no tag field }
		    end else begin
			GenOp(PCCHK); GenChar('v'); Comma;
			    GenInteger(tagFn^.offset); Comma;
			    GenInteger(SizeOf(tagFn^.fieldType)); Comma;
			if vn^.tag = nil then begin
			    { else }
			    GenChar('~');
			    vn := tagFn^.variantList^.first;
			    while vn <> nil do begin
				GenTagList(vn^.tag);
				vn := vn^.next;
				if vn <> nil then begin
				    GenChar(';');
				end;
			    end;
			end else begin
			    GenTagList(vn^.tag);
			end;
			EndLine;
		    end;
		    checkFn := tagFn;
		    vn := checkFn^.containingVariant;
		end;
	    end;
	    CHECKPTRNONE, CHECKPTRC : begin
	    end;
	end;
    end;
end;

procedure GenCode;
begin
    if TraceGenpc then begin
	writeln(output,'Beginning code generation');
    end;
    InitPcode;
    exitLabel := NULLLABEL;
    loopNestLevel := 0;
    rewrite(codeFile,codeFileName);
    InitStab;
    if globalProc^.globalName = nil then begin
	GenText(MAINPROGNAME); GenOp(PCBGN); GenText('Modula'); Comma;
	    GenInteger(globalProc^.block);
	EndLine;
    end else begin
	GenString(globalProc^.globalName); GenOp(PCBGN); GenText('Modula');
	    Comma; GenInteger(0);
	EndLine;
    end;
    GenComs;
    GenModule(globalModule);
    GenOpL(PCSTP);
    if TraceGenpc then begin
	writeln(output,'Ending code generation');
    end;
end;

procedure GenParamCopies(params : ParamList);
var
    param : ParamNode;
    size : cardinal;
begin
    param := params^.first;
    while param <> nil do begin
	if param^.docopy then begin
	    { if it is a value open array, must copy it onto stack }
	    if param^.kind = PARAMARRAYVALUE then begin
		{ point to parameter address }
		GenVarT(param^.paramVar,addressTypeNode,EVALPOINT);

		{ get number of elements }
		GenVar(param^.numElements,EVALGET);

		{ allocate space on stack, copy parameter value }
		size := SizeOf(param^.paramType^.elementType);
		GenOp(PCSAL); GenInteger(size); EndLine;
	    end else if param^.kind = PARAMVALUE then begin
		GenVarT(param^.paramVar,addressTypeNode,EVALPOINT);
		GenConstInteger(CardDiv(WordSizeOf(param^.paramType),WORDSIZE));
		{ allocate space on stack, copy parameter value }
		GenOp(PCSAL); GenInteger(WORDSIZE); EndLine;
	    end else begin
		Error('GenParamCopies: not open array or value param');
	    end;
	end;
	param := param^.next;
    end;
end;

procedure GenGlobalProc(proc: ProcNode);
var
    submod : ModuleNode;
    oklabel : LabelNumber;
begin
    if proc^.initFlagVar <> nil then begin
	GenVar(proc^.initFlagVar,EVALGET);
	oklabel := NewLabel;
	GenOp(PCFJP); GenLabel(oklabel); EndLine;
	GenOpTL(PCRET,procTypeNode);
	GenLabel(oklabel); GenOpL(PCLAB);
	GenConstBoolean(true);
	GenVar(proc^.initFlagVar,EVALPUT);
    end;

    { generate initialization calls if this is global proc }
    submod := globalModule^.modules^.first;
    while (submod <> nil) do begin
	if submod^.kind = MODDEFINITION then begin
	    GenOpL(PCMST);
	    AddString(submod^.name);
	    AddChar('_');
	    AddText(MODULEINITNAME);
	    GenCall(false,NewString,procTypeNode,0);
	end;
	submod := submod^.next;
    end;
end;

procedure GenProcName{(proc : ProcNode)};
begin
    if proc = globalProc then begin
	if proc^.globalName = nil then begin
	    { program module }
	    GenText(MAINPROGNAME);
	end else begin
	    { implementation module }
	    GenString(proc^.globalName);
	end;
    end else begin
	GenString(proc^.globalName);
    end;
end;

procedure GenProcEntry(proc : ProcNode);
var
    numParams : integer;
    param : ParamNode;
begin
    genProc := proc;
    StabProc(proc);
    StabLine(currFile,currLine);
    GenProcName(proc);
    GenOpT(PCENT,proc^.procType^.funcType);
    Comma;
    if proc^.procType^.funcType <> nil then begin
	GenInteger(SizeOf(proc^.procType^.funcType));
    end else begin
	GenInteger(0);
    end;
    Comma;
    GenInteger(proc^.displayLevel);
    Comma;
    GenInteger(proc^.block);
    Comma;
    numParams := 0;
    if proc^.procType^.paramList <> nil then begin
	param := proc^.procType^.paramList^.first;
	while param <> nil do begin
	    numParams := numParams + 1;
	    param := param^.next;
	end;
    end;
    GenInteger(numParams);
    Comma;
    if genDebugInfoFlag then begin
	GenInteger(proc^.lineNumber);
    end else begin
	GenInteger(0);
    end;
    Comma;
    if optimFlag then begin
	GenInteger(ord(not (proc^.displayLevel in proc^.containsUpLevel)));
    end else begin
	GenInteger(ord(not proc^.containsProcs));
    end;
    GenInteger(ord(proc^.internalProc));
    GenInteger(ord(gprofFlag));
    Comma;
    if (proc^.tempMap = nil) or OptNreg then begin
	GenInteger(-1);
    end else begin
	GenInteger(proc^.tempMap^.numReg+1);
    end;
    EndLine;
    GenOp(PCDEF);
    GenInteger(CardDiv(RoundUp(proc^.mem^.maximum[MEMPARAM],WORDSIZE),
		WORDSIZE));
    Comma;
    GenInteger(CardDiv(RoundUp(proc^.mem^.maximum[MEMFAST],WORDSIZE),
		WORDSIZE));
    Comma;
    GenInteger(CardDiv(RoundUp(proc^.mem^.maximum[MEMNORMAL],WORDSIZE),
		WORDSIZE));
    Comma;
    GenInteger(proc^.block);
    Comma;
    GenProcName(proc);
    EndLine;
    if proc <> globalProc then begin
	StabScope(proc^.scope);
    end;
    if proc^.tailRecursion then begin
	proc^.tailRecursionEntry := NewLabel;
	GenLabel(proc^.tailRecursionEntry); GenOpL(PCLAB);
    end;
    currLevel := proc^.displayLevel;
    if proc^.procType^.paramList <> nil then begin
	GenParamCopies(proc^.procType^.paramList);
    end;
    if proc = globalProc then begin
	GenGlobalProc(proc);
    end;
end;

procedure GenProcExit(proc : ProcNode);
begin
    if proc^.procType^.funcType = nil then begin
	GenOpTL(PCRET,nil);
    end else if genCheckFlag then begin
	GenOpTL(PCCHK,procTypeNode);
    end;
    GenOpL(PCEXI);
end;

procedure GenProc {(proc : ProcNode)};
var
    code : CodeNode;
begin
    GenText('#procedure ');
    GenString(proc^.globalName);
    EndLine;
    currFile := proc^.fileName;
    currLine := proc^.lineNumber;
    InitTemps;
    if proc^.code = nil then begin
	writeln(output,'No code in procedure');
    end else begin
	GenProcEntry(proc);
	code := proc^.code^.first;
	while code <> nil do begin
	    GenStmtList(code^.stmts);
	    code := code^.next;
	end;
	GenProcExit(proc);
    end;
end;

procedure GenModule {(module : ModuleNode)};
var
    proc : ProcNode;
    submod : ModuleNode;
begin
    if module^.kind <> MODDEFINITION then begin
	GenText('#module ');
	GenString(module^.name);
	EndLine;
	if module <> globalModule then begin
	    StabModule(module);
	    if module^.enclosing = globalModule then begin
		StabGlobalPort;
	    end;
	    StabScope(module^.scope);
	end;
	submod := module^.modules^.first;
	while submod <> nil do begin
	    GenModule(submod);
	    submod := submod^.next;
	end;
	proc := module^.procs^.first;
	while proc <> nil do begin
	    if proc^.builtin = BIPNOTBIP then begin
		GenProc(proc);
	    end;
	    proc := proc^.next;
	end;
    end;
end;

procedure GenStmtList{(stmts : StmtList)};
var
    stmt : StmtNode;
begin
    if stmts <> nil then begin
	stmt := stmts^.first;
	while stmt <> nil do begin
	    GenStmt(stmt);
	    stmt := stmt^.next;
	end;
    end;
end;

procedure GenStore{(expr : ExprNode; tn : TypeNode)};
begin
    if expr^.kind = EXPRVAR then begin
	GenVarT(expr^.exprVar,tn,EVALPUT);
    end else begin
	GenExpr(expr,EVALGET);
	GenIndirectVar(tn,EVALPUT);
    end;
end;

procedure GenStmtAssign(stmt : StmtNode);
begin
    if stmt^.assignOp = TKASSIGN then begin
	GenExpr(stmt^.rhs,EVALGET);
	GenStore(stmt^.lhs,stmt^.lhsType);
    end else begin
	GenExpr(stmt^.rhs^.opnd1^.exprVal,EVALPOINT);
	GenExpr(stmt^.rhs^.opnd2,EVALGET);
	case stmt^.assignOp of
	    TKPLUS: GenOp(PCAD2);
	    TKASTERISK: GenOp(PCMP2);
	    TKMINUS: GenOp(PCSB2);
	    TKDIV, TKSLASH: GenOp(PCDV2);
	end;
	GenT(stmt^.lhsType); Comma; GenInteger(SizeOf(stmt^.lhsType)); EndLine;
    end;
end;

function GenParamList(procType : TypeNode; procVariable : ExprNode;
	params : ExprList) : integer;
var
    pn : ParamNode;
    pen : ExprNode;
    numParams : integer;
    noCount : boolean;
begin
    numParams := 0;
    GenOpL(PCMST);
    if procVariable <> nil then begin
	{ invocation of procedure variable, make it first parameter }
	GenExpr(procVariable,EVALGET);
    end;
    if (params = nil) or (procType^.paramList = nil) then begin
	{ do nothing }
    end else begin
	pen := params^.first;
	pn := procType^.paramList^.first;
	while (pn <> nil) and (pen <> nil) do begin
	    case pn^.kind of
		PARAMARRAYVALUE, PARAMARRAYVAR : begin
		    GenExpr(pen,EVALPOINT);
		    noCount := false;
		    if pn^.paramType^.kind = DTARRAY then begin
			noCount :=  pn^.paramType^.nocount;
		    end;
		    if noCount then begin
		    end else if pen^.next = nil then begin
			ExprError(pen,'GenParamList: Open array, no length?');
		    end else begin
			pen := pen^.next;
			GenExpr(pen,EVALGET);
			numParams := numParams + 1;
		    end;
		end;
		PARAMVAR, PARAMVALUE : begin
		    if pn^.reference then begin
			GenExpr(pen,EVALPOINT);
		    end else begin
			GenExpr(pen,EVALGET);
		    end;
		end;
	    end;
	    numParams := numParams + 1;
	    pen := pen^.next;
	    pn := pn^.next;
	end;
    end;
    GenParamList := numParams;
end;

procedure GenCall{(internal : boolean; procName : String; procType : TypeNode;
	numParams : integer)};
begin
    if internal then begin
	GenOp(PCCUP);
    end else begin
	GenOp(PCCEP);
    end;
    GenT(procType^.funcType);
    Comma;
    if procType^.funcType <> nil then begin
	GenInteger(SizeOf(procType^.funcType));
    end else begin
	GenInteger(0);
    end;
    Comma;
    GenInteger(numParams);
    Comma;
    GenString(procName);
    EndLine;
end;

procedure GenTailRecursion(proc : ProcNode; params : ExprList);
var
    pn : ParamNode;
    pen : ExprNode;
    pnum : integer;
    temps : array [1..MAXTAILPARAMS] of integer;
    tempCount : integer;
    skip : boolean;
    sameVar : VarNode;
begin
    if (params <> nil) and (proc^.procType^.paramList <> nil) then begin
	{ first evaluate all parameters (except those that are the same) }
	pn := proc^.procType^.paramList^.first;
	pen := params^.first;
	pnum := 0;
	tempCount := 0;
	while pn <> nil do begin
	    pnum := pnum + 1;
	    skip := false;
	    case pn^.kind of
		PARAMARRAYVALUE, PARAMARRAYVAR : begin
		    if pen^.kind <> EXPRVAL then begin
			{ no good }
		    end else if pen^.exprVal^.kind <> EXPRVAR then begin
			{ no good }
		    end else if pen^.exprVal^.exprVar = pn^.paramVar then begin
			    skip := true;
		    end;
		    if skip then begin
			temps[pnum] := 0;
			pnum := pnum + 1;
			pen := pen^.next;
			temps[pnum] := 0;
		    end else begin
			GenExpr(pen,EVALPOINT);
			tempCount := tempCount + 1;
			temps[pnum] := tempCount;
			GenOp(PCSAV); GenInteger(temps[pnum]); Comma;
				GenChar('m'); EndLine;
			pen := pen^.next;
			pnum := pnum + 1;
			GenExpr(pen,EVALGET);
			tempCount := tempCount + 1;
			temps[pnum] := tempCount;
			GenOp(PCSAV); GenInteger(temps[pnum]); Comma;
				GenChar('m'); EndLine;
		    end;
		end;
		PARAMVAR : begin
		    if pen^.kind <> EXPRVAR then begin
			{ no good }
		    end else if pen^.exprVar = pn^.paramVar then begin
			    skip := true;
		    end;
		    if skip then begin
			temps[pnum] := 0;
		    end else begin
			GenExpr(pen,EVALPOINT);
			tempCount := tempCount + 1;
			temps[pnum] := tempCount;
			GenOp(PCSAV); GenInteger(temps[pnum]); Comma;
				GenChar('m'); EndLine;
		    end;
		end;
		PARAMVALUE : begin
		    if pen^.kind <> EXPRVAL then begin
			{ no good }
		    end else if pen^.exprVal^.kind <> EXPRVAR then begin
			{ no good }
		    end else if pen^.exprVal^.exprVar = pn^.paramVar then begin
			    skip := true;
		    end;
		    if skip then begin
			temps[pnum] := 0;
		    end else begin
			GenExpr(pen,EVALGET);
			tempCount := tempCount + 1;
			temps[pnum] := tempCount;
			GenOp(PCSAV); GenInteger(temps[pnum]); Comma;
				GenChar('m'); EndLine;
		    end;
		end;
	    end;
	    pen := pen^.next;
	    pn := pn^.next;
	end;
	{ now store them into the parameter list }
	pn := proc^.procType^.paramList^.first;
	pen := params^.first;
	pnum := 0;
	while pn <> nil do begin
	    pnum := pnum + 1;
	    skip := false;
	    case pn^.kind of
		PARAMARRAYVALUE, PARAMARRAYVAR : begin
		    if temps[pnum] <> 0 then begin
			GenOp(PCUSE); GenInteger(temps[pnum]); Comma;
				GenChar('m'); EndLine;
			GenVarT(pn^.paramVar,addressTypeNode,EVALPUT);
			pen := pen^.next;
			pnum := pnum + 1;
			GenOp(PCUSE); GenInteger(temps[pnum]); Comma;
				GenChar('m'); EndLine;
			GenVarT(pn^.numElements,cardinalTypeNode,EVALPUT);
		    end else begin
			pen := pen^.next;
		    end;
		end;
		PARAMVAR : begin
		    if temps[pnum] <> 0 then begin
			GenOp(PCUSE); GenInteger(temps[pnum]); Comma;
				GenChar('m'); EndLine;
			GenVarT(pn^.paramVar,addressTypeNode,EVALPUT);
		    end;
		end;
		PARAMVALUE : begin
		    if temps[pnum] <> 0 then begin
			GenOp(PCUSE); GenInteger(temps[pnum]); Comma;
				GenChar('m'); EndLine;
			GenVar(pn^.paramVar,EVALPUT);
		    end;
		end;
	    end;
	    pen := pen^.next;
	    pn := pn^.next;
	end;
    end;
    GenOp(PCUJP); GenLabel(proc^.tailRecursionEntry); EndLine;
end;

procedure GenFuncProc(procExpr : ExprNode; params : ExprList);
var
    proc : ProcNode;
    pn : ParamNode;
    generated : boolean;
    numParams : integer;
    on : OptNode;
begin
    generated := false;
    if procExpr^.kind = EXPRSYM then begin
	if procExpr^.exprSym^.kind = SYMTYPE then begin
	    GenExpr(params^.first,EVALGET);
	    GenOpTL(PCTYP,procExpr^.exprSym^.symType);
	    generated := true;
	end;
    end;
    if generated then begin
	{ do nothing }
    end else if procExpr^.kind <> EXPRCONST then begin
	numParams := GenParamList(procExpr^.exprType,procExpr,params);
	GenOpT(PCCIP,procExpr^.exprType^.funcType); Comma;
	    if procExpr^.exprType^.funcType <> nil then begin
		GenInteger(SizeOf(procExpr^.exprType^.funcType));
	    end else begin
		GenInteger(0);
	    end;
	    Comma;
	    GenInteger(numParams); EndLine;
    end else begin
	proc := procExpr^.exprConst^.procVal;
	if proc^.builtin <> BIPNOTBIP then begin
	    GenBuiltin(proc,params);
	end else begin
	    numParams := 0;
	    if proc^.procType^.paramList <> nil then begin
		pn := proc^.procType^.paramList^.first;
		while pn <> nil do begin
		    numParams := numParams + 1;
		    pn := pn^.next;
		end;
	    end;
	    if optimFlag then begin
		on := ExprToOpt(procExpr);
		if on^.tailRecursion then begin
		    GenTailRecursion(proc,params);
		    generated := true;
		end;
	    end;
	    if not generated then begin
		numParams := GenParamList(proc^.procType,nil,params);
		GenCall(proc^.internalProc,proc^.globalName,
		    proc^.procType,numParams);
	    end;
	end;
    end;
end;

procedure GenStmtProc(stmt : StmtNode);
begin
    GenFuncProc(stmt^.proc,stmt^.params);
end;


procedure GenStmtIf(stmt : StmtNode);
var
    elseLabel, endLabel : LabelNumber;
    elsePresent : boolean;
begin
    elsePresent := stmt^.elseList <> nil;
    if elsePresent then begin
	elsePresent := stmt^.elseList^.first <> nil;
    end;
    endLabel := NewLabel;
    if elsePresent then begin
	elseLabel := NewLabel;
	GenCondition(stmt^.ifCond,NULLLABEL,elseLabel);
	GenStmtList(stmt^.thenList);
	GenOp(PCUJP); GenLabel(endLabel); EndLine;
	GenLabel(elseLabel); GenOpL(PCLAB);
	GenStmtList(stmt^.elseList);
    end else begin
	GenCondition(stmt^.ifCond,NULLLABEL,endLabel);
	GenStmtList(stmt^.thenList);
    end;
    GenLabel(endLabel); GenOpL(PCLAB);
end;

procedure GenCaseTable(tree : CaseTreeNode; minval, maxval : cardinal;
	elseLabel : LabelNumber);
var
    i : cardinal;
begin
    if tree = nil then begin
	i := minval;
	while i <= maxval do begin
	    GenOp(PCCJP); GenLabel(elseLabel); EndLine;
	    i := i + 1;
	end;
    end else begin
	GenCaseTable(tree^.lower,minval,tree^.first-1,elseLabel);
	i := tree^.first;
	while i <= tree^.last do begin
	    GenOp(PCCJP); GenLabel(tree^.caseNode^.pcodeLabel); EndLine;
	    i := i + 1;
	end;
	GenCaseTable(tree^.higher,tree^.last+1,maxval,elseLabel);
    end;
end;

procedure GenStmtCase(stmt : StmtNode);
var
    caseNode : CaseNode;
    top, elseLabel, table, bottom : LabelNumber;
    node : CaseTreeNode;
    minval, maxval : cardinal;
begin
    top := NewLabel;
    elseLabel := NewLabel;
    table := NewLabel;
    bottom := NewLabel;
    writeln(codeFile,'# Case statement');
    GenOp(PCUJP); GenLabel(top); EndLine;
    caseNode := stmt^.cases^.first;
    while caseNode <> nil do begin
	caseNode^.pcodeLabel := NewLabel;
	GenLabel(caseNode^.pcodeLabel); GenOpL(PCLAB);
	GenStmtList(caseNode^.stmts);
	GenOp(PCUJP); GenLabel(bottom); EndLine;
	caseNode := caseNode^.next;
    end;
    GenLabel(elseLabel); GenOpL(PCLAB);
    if stmt^.caseElse = nil then begin
	if genCheckFlag then begin
	    GenOp(PCCHK); GenChar('c'); EndLine;
	end else begin
	    elseLabel := bottom;
	end;
    end else begin
	GenStmtList(stmt^.caseElse);
	GenOp(PCUJP); GenLabel(bottom); EndLine;
    end;
    node := stmt^.caseTree;
    repeat
	minval := node^.first;
	node := node^.lower;
    until node = nil;
    node := stmt^.caseTree;
    repeat
	maxval := node^.last;
	node := node^.higher;
    until node = nil;
    GenLabel(top); GenOpL(PCLAB);
    GenExpr(stmt^.caseSel,EVALGET);
    GenOp(PCXJP); GenLabel(table); Comma; GenLabel(elseLabel); Comma;
	GenInteger(minval); Comma; GenInteger(maxval); EndLine;
    GenLabel(table); GenOpL(PCLAB);
    GenCaseTable(stmt^.caseTree,minval,maxval,elseLabel);
    GenLabel(bottom); GenOpL(PCLAB);
end;

procedure GenPrePostEval(el : ExprList;state : EvalState);
var
    en : ExprNode;
begin
    if el = nil then begin
	{ do nothing }
    end else begin
	en := el^.first;
	while en <> nil do begin
	    OptGenExpr(en,EVALGET,state);
	    en := en^.next;
	end;
    end;
end;

procedure GenStmtWhile(stmt : StmtNode);
var
    top, bottom : LabelNumber;
begin
    top := NewLabel;
    bottom := NewLabel;

    GenCondition(stmt^.whileCond,NULLLABEL,bottom);
    GenPrePostEval(stmt^.whilePreEval,EVALPRE);
GenLabel(top); GenOp(PCLAB); GenChar('l'); EndLine;

    loopNestLevel := loopNestLevel + 1;
    GenStmtList(stmt^.whileBody);
    loopNestLevel := loopNestLevel - 1;

    GenCondition(stmt^.whileCond,top,NULLLABEL);

GenLabel(bottom); GenOpL(PCLAB);

    GenPrePostEval(stmt^.whilePreEval,EVALPOST);
end;


procedure GenStmtRepeat(stmt : StmtNode);
var
    top : LabelNumber;
begin
    top := NewLabel;

    GenPrePostEval(stmt^.repeatPreEval,EVALPRE);

GenLabel(top); GenOp(PCLAB); GenChar('l'); EndLine;

    loopNestLevel := loopNestLevel + 1;
    GenStmtList(stmt^.repeatBody);
    loopNestLevel := loopNestLevel - 1;

    GenCondition(stmt^.repeatCond,NULLLABEL,top);

    GenPrePostEval(stmt^.repeatPreEval,EVALPOST);
end;


procedure GenStmtLoop(stmt : StmtNode);
var
    top, bottom : LabelNumber;
    saveExitLabel : LabelNumber;
begin
    top := NewLabel;
    bottom := NewLabel;
    saveExitLabel := exitLabel;
    exitLabel := bottom;

    GenPrePostEval(stmt^.loopPreEval,EVALPRE);
GenLabel(top); GenOp(PCLAB); GenChar('l'); EndLine;

    loopNestLevel := loopNestLevel + 1;
    GenStmtList(stmt^.loopBody);
    loopNestLevel := loopNestLevel - 1;

    GenOp(PCUJP);
    GenLabel(top);
    EndLine;

    GenLabel(bottom);
    GenOpL(PCLAB);
    exitLabel := saveExitLabel;

    GenPrePostEval(stmt^.loopPreEval,EVALPOST);
end;


procedure GenStmtFor(stmt : StmtNode);
var
    top, bottom : LabelNumber;
    increment, first, last : cardinal;
    compareop : PcodeInst;
    cn : ConstNode;
    tn, atn : TypeNode;
    dotest : boolean;
begin
    top := NewLabel;
    bottom := NewLabel;

    if stmt^.forBy = nil then begin
	increment := 1;
    end else begin
	cn := Eval(stmt^.forBy);
	increment := OrdOf(cn);
    end;

    tn := stmt^.forIndexType;

    if increment > 0 then begin
	compareop := PCLEQ;
    end else begin
	compareop := PCGEQ;
    end;


    GenExpr(stmt^.forTo,EVALGET);
    GenVarT(stmt^.forLimitVar,tn,EVALPUT);
    GenExpr(stmt^.forFrom,EVALGET);
    GenVarT(stmt^.forIndexVar,tn,EVALPUT);

    dotest := true;
    if (stmt^.forFrom^.kind = EXPRCONST) and (stmt^.forTo^.kind = EXPRCONST)
    then begin
	first := OrdOf(Eval(stmt^.forFrom));
	last := OrdOf(Eval(stmt^.forTo));
	if first <= last then begin
	    dotest := compareop <> PCLEQ;
	end else begin
	    dotest := compareop <> PCGEQ;
	end;
    end;
    if dotest then begin
	{ see if loop ever executed }
	GenVarT(stmt^.forIndexVar,tn,EVALGET);
	GenVarT(stmt^.forLimitVar,tn,EVALGET);
	atn := BaseType(tn);
	if atn^.kind = DTCARDINAL then begin
	    GenOpTL(compareop,integerTypeNode);
	end else begin
	    GenOpTL(compareop,tn);
	end;
	GenOp(PCFJP);  GenLabel(bottom); EndLine;
    end;

    { evaluate invariants }
    GenPrePostEval(stmt^.forPreEval,EVALPRE);

GenLabel(top); GenOp(PCLAB); GenChar('l'); EndLine;

    loopNestLevel := loopNestLevel + 1;
    GenStmtList(stmt^.forBody);
    loopNestLevel := loopNestLevel - 1;

    GenVarT(stmt^.forIndexVar,tn,EVALPOINT);
    GenOpT(PCLDC,tn); Comma; GenInteger(SizeOf(tn)); Comma;
	    GenInteger(increment); EndLine;
    GenVarT(stmt^.forLimitVar,tn,EVALGET);
    GenOpT(PCFOR,tn); Comma; GenInteger(SizeOf(tn));
	    Comma; GenLabel(top); EndLine;

{*** old way to increment and test 
    GenVarT(stmt^.forIndexVar,tn,EVALGET);
    GenOpT(PCINC,tn); Comma; GenInteger(increment); EndLine;
    GenVarT(stmt^.forIndexVar,tn,EVALPUT);

    GenVarT(stmt^.forIndexVar,tn,EVALGET);
    GenVarT(stmt^.forLimitVar,tn,EVALGET);
    GenOpTL(compareop,tn);
    GenOp(PCTJP);  GenLabel(top); EndLine;
***}

GenLabel(bottom); GenOpL(PCLAB);

    GenPrePostEval(stmt^.forPreEval,EVALPOST);

end;


procedure GenStmtWith(stmt : StmtNode);
begin
    GenExpr(stmt^.withQual,EVALPOINT);
    GenVar(stmt^.withQualNode^.implQual,EVALPUT);
    GenStmtList(stmt^.withBody);
end;


procedure GenStmtReturn(stmt : StmtNode);
var
    on : OptNode;
    doreturn : boolean;
begin
    doreturn := true;
    if stmt^.returnVal <> nil then begin
	GenExpr(stmt^.returnVal,EVALGET);
	{ watch for tail recursion }
	if optimFlag then begin
	    if stmt^.returnVal^.kind = EXPRFUNC then begin
		on := ExprToOpt(stmt^.returnVal^.func);
		if on^.tailRecursion then begin
		    doreturn := false;
		end;
	    end;
	end;
    end;
    if doreturn then begin
	GenOpTL(PCRET,genProc^.procType^.funcType);
    end;
end;


procedure GenStmtExit(stmt : StmtNode);
begin
    GenOp(PCUJP); GenLabel(exitLabel); EndLine;
end;



procedure GenStmt{(stmt : StmtNode)};
begin
    if TraceGenpc then begin
	writeln(codeFile,'# statement ',stmt^.kind);
    end;
    currFile := stmt^.fileName;
    currLine := stmt^.lineNumber;
    StabLine(currFile,currLine);
    if stmt^.bad then begin
	StmtError(stmt,'GenStmt: bad statement?');
    end;
    case stmt^.kind of
	STMTASSIGN :	GenStmtAssign(stmt);
	STMTPROC :	GenStmtProc(stmt);
	STMTIF :	GenStmtIf(stmt);
	STMTCASE :	GenStmtCase(stmt);
	STMTWHILE :	GenStmtWhile(stmt);
	STMTREPEAT :	GenStmtRepeat(stmt);
	STMTLOOP :	GenStmtLoop(stmt);
	STMTFOR :	GenStmtFor(stmt);
	STMTWITH :	GenStmtWith(stmt);
	STMTRETURN :	GenStmtReturn(stmt);
	STMTEXIT :	GenStmtExit(stmt);
    end;
    UpdateTemps;
end;

procedure GenAddress{(address : Address)};
begin
    if address.kind = MEMGLOBAL then begin
	GenMt(address.kind);
	Comma;
	GenInteger(0);
	Comma;
	GenInteger(address.gvn^.number);
    end else if (address.kind = MEMFAST) and (address.proc^.tempMap <> nil)
    then begin
	GenMt(address.kind);
	Comma;
	GenInteger(address.proc^.tempMap^.map[trunc(address.offset) div WORDSIZE]);
	Comma;
	GenInteger(address.proc^.block);
    end else begin
	GenMt(address.kind);
	Comma;
	GenInteger(address.offset);
	Comma;
	GenInteger(address.proc^.block);
    end;
end;

procedure GenVarT{(vn : VarNode; tn : TypeNode; mode : EvalMode)};
begin
    if vn^.address.kind = MEMGLOBAL then begin
	case mode of
	    EVALGET : GenOp(PCLDO);
	    EVALPUT : GenOp(PCSRO);
	    EVALPOINT : GenOp(PCLAO);
	end;
	if mode <> EVALPOINT then begin
	    GenT(tn);
	    Comma;
	end;
	GenInteger(SizeOf(tn));
	Comma;
	GenAddress(vn^.address);
	EndLine;
    end else begin
	case mode of
	    EVALGET : GenOp(PCLOD);
	    EVALPUT : GenOp(PCSTR);
	    EVALPOINT : GenOp(PCLDA);
	end;
	if mode <> EVALPOINT then begin
	    GenT(tn);
	    Comma;
	end;
	GenInteger(SizeOf(tn));
	Comma;
	GenInteger(currLevel - vn^.address.level);
	Comma;
	GenAddress(vn^.address);
	EndLine;
    end;
end;

procedure GenVar{(vn : VarNode; mode : EvalMode)};
begin
    GenVarT(vn,vn^.varType,mode);
end;

procedure GenIndirectVar{(varType : TypeNode; mode : EvalMode)};
begin
    if mode in [EVALGET, EVALPUT] then begin
	if mode = EVALGET then begin
	    GenOp(PCIND);
	end else begin
	    GenOp(PCSTO);
	end;
	GenT(varType);
	Comma;
	GenInteger(SizeOf(varType));
	Comma;
	GenInteger(0);
	EndLine;
    end;
end;

procedure GenExprString(expr : ExprNode; mode : EvalMode);
var
    cn : ConstNode;
    i : integer;
begin
    if mode = EVALPUT then begin
	ExprError(expr,'GenExprString: Cannot store into a constant?');
    end else if expr^.exprConst^.kind <> DTSTRING then begin
	ExprError(expr,'GenExprString: not a string?');
    end else begin
	if mode = EVALGET then begin
	    GenOpT(PCLDC,stringTypeNode);
	end else begin
	    GenOpT(PCLCA,stringTypeNode);
	end;
	cn := expr^.exprConst;
	Comma;
	GenInteger(SizeOf(expr^.exprType));
	Comma;
	GenChar('''');
	WriteStringConst(codeFile,cn^.strVal);
	GenChar('''');
	EndLine;
    end;
end;

procedure GenExprConst(expr : ExprNode; mode : EvalMode);
var
    cn : ConstNode;
begin
    cn := expr^.exprConst;
    case cn^.kind of
	DTINTEGER, DTCARDINAL : begin
	    if expr^.exprType = addressTypeNode then begin
		GenOpT(PCLDC,addressTypeNode);
		Comma;
		GenInteger(WORDSIZE);
		Comma;
		GenInteger(cn^.cardVal);
		EndLine;
	    end else if cn^.cardVal < 0 then begin
		GenConstInteger(cn^.cardVal);
	    end else begin
		GenOpT(PCLDC,cardinalTypeNode);
		Comma;
		GenInteger(WORDSIZE);
		Comma;
		GenInteger(cn^.cardVal);
		EndLine;
	    end;
	end;
	DTCHAR : begin
	    if expr^.exprType^.kind <> DTSTRING then begin
		GenOpT(PCLDC,charTypeNode);
	    end else if mode = EVALGET then begin
		{ char masquerading as a string }
		GenOpT(PCLDC,stringTypeNode);
	    end else if mode = EVALPOINT then begin
		{ char masquerading as a string }
		GenOpT(PCLCA,stringTypeNode);
	    end;
	    Comma;
	    GenInteger(SizeOf(expr^.exprType));
	    Comma;
	    GenChar('''');
	    if cn^.charVal in [ord(' ')..ord('~')] then begin
		if chr(cn^.charVal) in ['''','\'] then begin
		    GenChar(chr(cn^.charVal));
		end;
		GenChar(chr(cn^.charVal));
	    end else begin
		GenChar('\');
		GenInteger(cn^.charVal);
		GenChar('\');
	    end;
	    GenChar('''');
	    EndLine;
	end;
	DTBOOLEAN : begin
	    GenConstBoolean(cn^.boolVal);
	end;
	DTREAL,
	DTLONGREAL : begin
	    GenOpT(PCLDC,expr^.exprType);
	    Comma;
	    GenInteger(SizeOf(expr^.exprType));
	    Comma;
	    GenReal(cn^.realVal);
	    EndLine;
	end;
	DTSTRING : begin
	    GenExprString(expr,mode);
	end;
	DTENUMERATION : begin
	    GenOpT(PCLDC,integerTypeNode);
	    Comma;
	    GenInteger(WORDSIZE);
	    Comma;
	    GenInteger(cn^.enumVal^.enumOrd);
	    EndLine;
	end;
	DTPROC : begin
	    GenOpT(PCLDC,procTypeNode);
	    Comma;
	    GenInteger(WORDSIZE);
	    Comma;
	    GenString(cn^.procVal^.globalName);
	    EndLine;
	end;
	DTPOINTER : begin
	    GenOp(PCLDC);
	    GenChar('n');
	    Comma;
	    GenInteger(WORDSIZE);
	    EndLine;
	end;
	DTSET : begin
	    GenOpT(PCLDC,bitsetTypeNode);
	    Comma;
	    GenSet(cn^.setVal);
	    EndLine;
	end;
    end;
end;

procedure GenExprUnOp(expr : ExprNode; mode : EvalMode);
begin
    GenExpr(expr^.opnd,EVALGET);
    if expr^.exprUnOp = TKMINUS then begin
	GenOpTL(PCNEG,expr^.unOperType);	{ unary subtract }
    end else if expr^.exprUnOp = TKPLUS then begin
	{ unary plus }
    end else begin
	GenOpTL(operPcode[expr^.exprUnOp],expr^.unOperType);
    end;
end;

procedure GenExprBinSetOp(expr : ExprNode);
var
    binOp : Token;
begin
    GenExpr(expr^.opnd1,EVALGET);
    GenExpr(expr^.opnd2,EVALGET);
    binOp := expr^.exprBinOp;
    case binOp of
	TKPLUS : begin
	    GenOpT(PCUNI,expr^.operType);
		Comma; GenInteger(SizeOf(expr^.operType));
	    EndLine;
	end;
	TKMINUS : begin
	    GenOpT(PCDIF,expr^.operType);
		Comma; GenInteger(SizeOf(expr^.operType));
	    EndLine;
	end;
	TKASTERISK : begin
	    GenOpT(PCINT,expr^.operType);
		Comma; GenInteger(SizeOf(expr^.operType));
	    EndLine;
	end;
	TKSLASH : begin
	    GenOpT(PCSDF,expr^.operType);
		Comma; GenInteger(SizeOf(expr^.operType));
	    EndLine;
	end;
	TKLSEQUAL : begin
	    GenOpT(PCDIF,expr^.operType);
		Comma; GenInteger(SizeOf(expr^.operType));
	    EndLine;
	    GenOpT(PCLDC,bitsetTypeNode); Comma;
		GenInteger(SizeOf(expr^.operType)); Comma; GenInteger(0);
	    EndLine;
	    GenOpT(PCEQU,expr^.operType);
		Comma; GenInteger(SizeOf(expr^.operType));
	    EndLine;
	end;
	TKEQUALS, TKSHARP, TKNOTEQUAL: begin
	    GenOpT(operPcode[binOp],expr^.operType); Comma;
		GenInteger(SizeOf(expr^.operType));
	    EndLine;
	end;
    end;
end;

procedure GenExprBinOp(expr : ExprNode; mode : EvalMode);
var
    trueLabel, bothLabel : LabelNumber;
    temp : TempNumber;
    minElement : cardinal;
begin
    if expr^.operType^.kind = DTSET then begin
	GenExprBinSetOp(expr);
    end else if expr^.exprBinOp in [TKAND, TKAMPERSAND, TKOR] then begin
	trueLabel := NewLabel;
	bothLabel := NewLabel;
	temp := AllocTemp;
	GenCondition(expr,trueLabel,NULLLABEL);
	GenConstBoolean(false);
	GenOp(PCSAV); GenInteger(temp); Comma; GenChar('m'); EndLine;
	GenOp(PCUJP); GenLabel(bothLabel); EndLine;
	GenLabel(trueLabel); GenOpL(PCLAB);
	GenConstBoolean(true);
	GenOp(PCSAV); GenInteger(temp); Comma; GenChar('r'); EndLine;
	GenLabel(bothLabel); GenOpL(PCLAB);
	GenOp(PCUSE); GenInteger(temp); Comma; GenChar('m'); EndLine;
	FreeTemp(temp);
    end else if expr^.exprBinOp = TKIN then begin
	GenExpr(expr^.opnd1,EVALGET);
	GenExpr(expr^.opnd2,EVALGET);
	GenOpTL(PCINN,expr^.operType);
    end else begin
	GenExpr(expr^.opnd1,EVALGET);
	GenExpr(expr^.opnd2,EVALGET);
	GenOpT(operPcode[expr^.exprBinOp],expr^.operType);
	    Comma; GenInteger(SizeOf(expr^.operType));
	EndLine;
    end;
end;

procedure GenExprVal(expr : ExprNode; mode : EvalMode);
begin
    if expr^.exprVal^.kind = EXPRVAR then begin
	GenVarT(expr^.exprVal^.exprVar,expr^.exprType,EVALGET);
    end else begin
	GenExpr(expr^.exprVal,EVALPOINT);
	GenIndirectVar(expr^.exprType,EVALGET);
    end;
end;

procedure GenExprFunc(expr : ExprNode);
begin
    GenFuncProc(expr^.func,expr^.params);
end;

procedure DoGenExpr{(expr : ExprNode; mode : EvalMode)};
begin
    case mode of
	EVALGET: begin
	    case expr^.kind of
		EXPRBAD : ;
		EXPRVAR :	GenVar(expr^.exprVar,EVALPOINT);
		EXPRCONST :	GenExprConst(expr,EVALGET);
		EXPRUNOP :	GenExprUnOp(expr,EVALGET);
		EXPRBINOP :	GenExprBinOp(expr,EVALGET);
		EXPRFUNC :	GenExprFunc(expr);
		EXPRVAL :	GenExprVal(expr,EVALGET);
		EXPRCHECK :	GenExprCheck(expr,EVALGET);
	    end;
	end;
	EVALPUT, EVALPOINT: begin
	    case expr^.kind of
		EXPRBAD : ;
		EXPRVAR :	GenVar(expr^.exprVar,mode);
		EXPRCONST :	GenExprConst(expr,mode);
		EXPRBINOP :	GenExprBinOp(expr,mode);
		EXPRVAL :	GenExprVal(expr,mode);
		EXPRFUNC :	GenExprFunc(expr);
		EXPRUNOP : begin
		    ExprError(expr,'GenExpr: address an expression?');
		end;
		EXPRCHECK :	GenExprCheck(expr,mode);
	    end;
	end;
    end;
end;

procedure GenExpr{(expr : ExprNode; mode : EvalMode)};
begin
    if expr = nil then begin
	writeln(codeFile,'# NIL expression ',mode);
    end else begin
	if TraceGenpc then begin
	    writeln(codeFile,'# expression ',expr^.kind,' ',mode);
	end;
	if expr^.exprType = nil then begin
	    ExprError(expr,'GenExpr: no type on expression');
	end;
	if optimFlag then begin
	    OptGenExpr(expr,mode,EVALNORMAL);
	end else begin
	    DoGenExpr(expr,mode);
	end;
    end;
end;

procedure GenConstInteger{(i : cardinal)};
begin
    GenOpT(PCLDC,integerTypeNode);
    Comma;
    GenInteger(WORDSIZE);
    Comma;
    GenInteger(i);
    EndLine;
end;

procedure GenConstBoolean{(b : boolean)};
begin
    GenOpT(PCLDC,booleanTypeNode);
    Comma;
    GenInteger(BOOLEANSIZE);
    Comma;
    GenInteger(ord(b));
    EndLine;
end;

procedure GenCondition{(expr : ExprNode; trueLabel,falseLabel : LabelNumber)};
var
    done : boolean;
    fallThrough : LabelNumber;
begin
    done := false;
    if expr^.kind = EXPRUNOP then begin
	if expr^.exprUnOp = TKNOT then begin
	    GenCondition(expr^.opnd,falseLabel,trueLabel);
	    done := true;
	end;
    end else if expr^.kind = EXPRBINOP then begin
	fallThrough := NULLLABEL;
	if expr^.exprBinOp in [TKAND, TKAMPERSAND] then begin
	    if falseLabel = NULLLABEL then begin
		fallThrough := NewLabel;
		falseLabel := fallThrough;
	    end;
	    GenCondition(expr^.opnd1,NULLLABEL,falseLabel);
	    GenCondition(expr^.opnd2,trueLabel,falseLabel);
	    if fallThrough <> NULLLABEL then begin
		GenLabel(fallThrough); GenOpL(PCLAB);
	    end;
	    done := true;
	end else if expr^.exprBinOp = TKOR then begin
	    if trueLabel = NULLLABEL then begin
		fallThrough := NewLabel;
		trueLabel := fallThrough;
	    end;
	    GenCondition(expr^.opnd1,trueLabel,NULLLABEL);
	    GenCondition(expr^.opnd2,trueLabel,falseLabel);
	    if fallThrough <> NULLLABEL then begin
		GenLabel(fallThrough); GenOpL(PCLAB);
	    end;
	    done := true;
	end;
    end;
    if not done then begin
	GenExpr(expr,EVALGET);
	if trueLabel <> NULLLABEL then begin
	    GenOp(PCTJP); GenLabel(trueLabel); EndLine;
	    if falseLabel <> NULLLABEL then begin
		GenOp(PCUJP); GenLabel(falseLabel); EndLine;
	    end;
	end else begin
	    GenOp(PCFJP); GenLabel(falseLabel); EndLine;
	end;
    end;
end;

procedure GenTemp(op : PcodeInst; on : OptNode; en : ExprNode);
var
    addr : Address;
    tn : TypeNode;
begin
    addr.kind := MEMFAST;
    addr.level := currLevel;
    addr.proc := genProc;
    addr.offset := (MAXTSTORAGE + on^.tempNumber) * WORDSIZE;
    if on^.address then begin
	tn := addressTypeNode;
    end else begin
	tn := en^.exprType;
    end;
    GenOpT(op,tn); Comma; GenInteger(WORDSIZE); Comma; GenInteger(0); Comma;
	GenAddress(addr); EndLine;
end;

procedure OptGenExpr{(en : ExprNode; mode : EvalMode; state : EvalState)};
var
    on, ron : OptNode;
begin
    on := ExprToOpt(en);
    ron := on^.rootEqual;
    if (on^.usage <> OUSEINDIVIDUAL) or
	    ((on^.usage = OUSEINDIVIDUAL) and (state <> EVALNORMAL))
    then begin
	if TraceOptim then begin
	    write(output,'OptGenExpr: usage=',on^.usage:1,', mode=',mode:1,
		', state=',state:1,', expr=');
	    WriteExpr(output,en);
	    writeln(output);
	end;
    end;
    case on^.usage of
	OUSEINDIVIDUAL : begin
	    { not subexpression, do normal evaluate }
	    if state = EVALNORMAL then begin
		DoGenExpr(en,mode);
	    end;
	end;
	OUSEGENERATE : begin
	    { either generation or discard of a value, depending on state }
	    if state = EVALPRE then begin
		{ calculate a value for later use }
		if ron^.address then begin
		    DoGenExpr(en,EVALPOINT);
		end else begin
		    DoGenExpr(en,mode);
		end;
		{GenOp(PCSAV); GenInteger(ron^.tempNumber); Comma; GenChar('m');
		EndLine;}
		GenTemp(PCSTR,ron,en);
	    end else begin
		{ finished with saved value }
		{GenOp(PCUSE); GenInteger(ron^.tempNumber); Comma; GenChar('d');
		EndLine;}
	    end;
	end;
	OUSEFIRST : begin
	    { first use of a value, evaluate and copy it }
	    if ron^.address then begin
		DoGenExpr(en,EVALPOINT);
	    end else begin
		DoGenExpr(en,mode);
	    end;
	    {GenOp(PCSAV); GenInteger(ron^.tempNumber); Comma; GenChar('c');
	    EndLine;}
	    GenTemp(PCSTN,ron,en);
	end;
	OUSEMIDDLE : begin
	    { reuse saved value }
	    {GenOp(PCUSE); GenInteger(ron^.tempNumber); Comma; GenChar('c');
	    EndLine;}
	    GenTemp(PCLOD,ron,en);
	end;
	OUSELAST : begin
	    { last use of saved value }
	    {GenOp(PCUSE); GenInteger(ron^.tempNumber); Comma; GenChar('m');
	    EndLine;}
	    GenTemp(PCLOD,ron,en);
	end;
    end;
end;

