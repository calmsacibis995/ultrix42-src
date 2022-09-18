(*#@(#)cstmt.p	4.1	Ultrix	7/17/90 *)
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
$Header: cstmt.p,v 1.6 84/06/06 12:55:26 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "bexpr.h"
#include "cexpr.h"
#include "const.h"
#include "cstmt.h"
#include "decls.h"
#include "alloc.h"

procedure BadStmt(stn : StmtNode);
begin
    stn^.bad := true;
end;

procedure AssignStmt(stn : StmtNode);
var
    lhstn, rhstn : TypeNode;
begin
    if TraceNstmt then begin
	writeln(output,'AssignStmtNode');
    end;
    rhstn := CheckExpr(stn^.rhs,EVALGET);
    lhstn := CheckExpr(stn^.lhs,EVALPUT);
    stn^.lhsType := lhstn;
    if IsBadExpr(stn^.lhs) or IsBadExpr(stn^.rhs) then begin
	BadStmt(stn);
    end else if not IsAddressableExpr(stn^.lhs) then begin
	StmtError(stn,'Cannot assign to left hand side');
	BadStmt(stn);
    end else if Assignable(lhstn,rhstn,stn^.rhs) = nil then begin
	StmtError(stn,'Expression is not assignment compatible with variable');
	BadStmt(stn);
    end else begin
	if stn^.rhs^.kind = EXPRCONST then begin
	    stn^.rhs^.exprType := rhstn;
	end;
	lhstn := BaseType(lhstn);
	if genCheckFlag and (lhstn^.kind = DTSUBRANGE) then begin
	    InsertCheckExpr(stn^.rhs,CHECKRANGE,nil,lhstn,LowerBoundOf(lhstn),
		UpperBoundOf(lhstn));
	end;
    end;
end;

procedure ProcStmt(stn : StmtNode);
var
    retType, tn : TypeNode;
begin
    if TraceNstmt then begin
	writeln(output,'ProcStmtNode');
    end;
    tn := CheckExprFunc(stn^.proc,EVALPOINT);
    if IsBadExpr(stn^.proc) then begin
	BadStmt(stn);
    end else begin
	if stn^.proc^.kind <> EXPRCONST then begin
	    ValueOrAddr(stn^.proc,tn,EVALGET);
	end;
	if not CheckFuncProc(false,nil,stn^.proc,stn^.params,retType)
	then begin
	    BadStmt(stn);
	end;
    end;
end;

procedure IfStmt(stn : StmtNode);
var
    tn : TypeNode;
begin
    if TraceNstmt then begin
	writeln(output,'IfStmtNode');
    end;
    tn := CheckExpr(stn^.ifCond,EVALGET);
    if IsBadExpr(stn^.ifCond) then begin
	BadStmt(stn);
    end else if Assignable(booleanTypeNode,tn,stn^.ifCond) = nil
    then begin
	ExprErrorName(stn^.ifCond,stringDataType[stn^.ifCond^.exprType^.kind],
		'If condition not boolean expression');
	BadStmt(stn);
    end;
    CheckStmtList(stn^.thenList);
    CheckStmtList(stn^.elseList);
end;

function AddToCaseTree(var tree : CaseTreeNode; first, last : cardinal;
    caseNode : CaseNode) : boolean;
var
    result : boolean;
begin
    if TraceNstmt then begin
	writeln(output,'AddToCaseTree ',first:1:0,' ',last:1:0);
    end;
    if tree = nil then begin
	new(tree);
	tree^.first := first;
	tree^.last := last;
	tree^.caseNode := caseNode;
	tree^.higher := nil;
	tree^.lower := nil;
	result := true;
    end else if last < tree^.first then begin
	result := AddToCaseTree(tree^.lower,first,last,caseNode);
    end else if first > tree^.last then begin
	result := AddToCaseTree(tree^.higher,first,last,caseNode);
    end else begin
	result := false;
    end;
    AddToCaseTree := result;
end;

function CheckCase(stn : StmtNode; caseNode : CaseNode; tn : TypeNode)
	: boolean;
var
    labels : ConstSetNode;
    tree : CaseTreeNode;
    error : boolean;
    constType : TypeNode;
begin
    if TraceNstmt then begin
	writeln(output,'CheckCase');
    end;
    error := false;
    if tn = nil then begin
	{ do nothing }
    end else begin
	caseNode^.labelConsts := ExprSetToConstSet(caseNode^.labelExprs);
	labels := caseNode^.labelConsts^.first;
	while labels <> nil do begin
	    constType := ConstType(labels^.lower);
	    if Assignable(tn,constType,nil) = nil then begin
		StmtError(stn,'Case labels incompatible with selector');
		error := true;
	    end else begin
		if AddToCaseTree(stn^.caseTree,OrdOf(labels^.lower),
			OrdOf(labels^.upper),caseNode)
		then begin
		    { OK }
		end else if caseNode^.stmts^.first <> nil then begin
		    StmtError(caseNode^.stmts^.first,
			'Case labels duplicate some values');
		    error := true;
		end else begin
		    StmtError(stn,'Case statement has duplicate label values');
		    error := true;
		end;
	    end;
	    labels := labels^.next;
	end;
    end;
    CheckStmtList(caseNode^.stmts);
    CheckCase := not error;
end;

procedure CaseStmt(stn : StmtNode);
var
    bt : TypeNode;
    caseNode : CaseNode;
    error : boolean;
begin
    if TraceNstmt then begin
	writeln(output,'CaseStmtNode');
    end;
    bt := BaseType(CheckExpr(stn^.caseSel,EVALGET));
    error := true;
    if IsBadExpr(stn^.caseSel) then begin
	bt := nil;
	BadStmt(stn);
    end else begin
	if not (bt^.kind in indexableTypes) then begin
	    StmtErrorName(stn,stringDataType[bt^.kind],
		'Invalid expression type for case selector');
	end else begin
	    error := false;
	end;
    end;
    if stn^.cases = nil then begin
	{ do nothing }
    end else begin
	caseNode := stn^.cases^.first;
	while caseNode <> nil do begin
	    if not CheckCase(stn,caseNode,bt) then begin
		BadStmt(stn);
	    end;
	    caseNode := caseNode^.next;
	end;
	CheckStmtList(stn^.caseElse);
    end;
end;

procedure WhileStmt(stn : StmtNode);
var
    tn : TypeNode;
begin
    if TraceNstmt then begin
	writeln(output,'WhileStmtNode');
    end;
    tn := CheckExpr(stn^.whileCond,EVALGET);
    if IsBadExpr(stn^.whileCond) then begin
	BadStmt(stn);
    end else if Assignable(booleanTypeNode,tn,stn^.whileCond) = nil
    then begin
	ExprErrorName(stn^.whileCond,
	    stringDataType[stn^.whileCond^.exprType^.kind],
	    'While condition not boolean expression');
	BadStmt(stn);
    end;
    CheckStmtList(stn^.whileBody);
end;

procedure RepeatStmt(stn : StmtNode);
var
    tn : TypeNode;
begin
    if TraceNstmt then begin
	writeln(output,'RepeatStmtNode');
    end;
    tn := CheckExpr(stn^.repeatCond,EVALGET);
    if IsBadExpr(stn^.repeatCond) then begin
	BadStmt(stn);
    end else if Assignable(booleanTypeNode,tn,stn^.repeatCond) = nil
    then begin
	ExprErrorName(stn^.repeatCond,
	    stringDataType[stn^.repeatCond^.exprType^.kind],
	    'Repeat condition not boolean expression');
	BadStmt(stn);
    end;
    CheckStmtList(stn^.repeatBody);
end;

procedure LoopStmt(stn : StmtNode);
begin
    if TraceNstmt then begin
	writeln(output,'LoopStmtNode');
    end;
    stn^.saveLoopActive := loopActive;
    loopActive := true;
    CheckStmtList(stn^.loopBody);
    loopActive := stn^.saveLoopActive;
end;

procedure ForStmt(stn : StmtNode);
var
    sym : Symbol;
    bt, totn, fromtn, bytn : TypeNode;
    error : boolean;
    saveIndexVar : VarNode;
begin
    fromtn := CheckExpr(stn^.forFrom,EVALGET);
    totn := CheckExpr(stn^.forTo,EVALGET);
    if stn^.forBy = nil then begin
	stn^.forBy := ConstExprNode(CardinalConst(1));
    end;
    bytn := CheckExpr(stn^.forBy,EVALGET);
    error := true;
    if IsBadExpr(stn^.forFrom) or IsBadExpr(stn^.forTo) then begin
	{ do nothing }
    end else begin
	sym := LookUpSymbol(stn^.forIndexName,nil,ONECASE);
	if sym = nil then begin
	    StmtErrorName(stn,stn^.forIndexName,'For index unknown');
	end else if sym^.kind <> SYMVAR then begin
	    StmtErrorName(stn,sym^.name,'Loop index must be a variable');
	end else begin
	    bt := sym^.symVar^.varType;
	    stn^.forIndexType := bt;
	    if Assignable(bt,fromtn,stn^.forFrom) = nil then begin
		StmtError(stn,'For loop "from" value not assignable to index');
	    end else if Assignable(bt,totn,stn^.forTo) = nil then begin
		StmtError(stn,'For loop "to" value not assignable to index');
	    end else if stn^.forBy^.kind <> EXPRCONST then begin
		StmtError(stn,'For loop "by" value must be constant');
	    end else if not (stn^.forBy^.exprConst^.kind in
		    [DTINTEGER,DTCARDINAL])
	    then begin
		StmtError(stn,'For loop "by" value must be integer');
	    end else if stn^.forBy^.exprConst^.cardVal = 0 then begin
		StmtError(stn,'For loop "by" value must not be zero');
	    end else begin
		error := false;
	    end;
	end;
    end;
    if error then begin
	BadStmt(stn);
    end else begin
	case target of
	    TARGETVAX : stn^.forSaveAlloc := SaveAllocationNode(currProc^.mem);
	    TARGETTITAN : stn^.forSaveAlloc := SaveAllocationNode(currProc^.mem);
	end;
	if currProc^.containsProcs then begin
	    { safe strategy, because subroutines may use it }
	    stn^.forIndexVar := sym^.symVar;
	    stn^.forLimitVar := DefineVar(nil,sym^.symVar^.varType,MEMFAST,false);
	end else begin
	    { fast strategy, temporarily put index in temp }
	    saveIndexVar := sym^.symVar;
	    stn^.forIndexVar := DefineVar(nil,sym^.symVar^.varType,MEMFAST,false);
	    sym^.symVar := stn^.forIndexVar;
	    stn^.forLimitVar := DefineVar(nil,sym^.symVar^.varType,MEMFAST,false);
	end;
	sym^.symVar^.readonly := true;
    end;
    CheckStmtList(stn^.forBody);
    if not error then begin
	stn^.forIndexVar^.readonly := false;
	if not currProc^.containsProcs then begin
	    { restore index to proper place }
	    sym^.symVar := saveIndexVar;
	end;
	case target of
	    TARGETVAX : RestoreAllocationNode(currProc^.mem,stn^.forSaveAlloc);
	    TARGETTITAN : RestoreAllocationNode(currProc^.mem,stn^.forSaveAlloc);
	end;
    end;
end;

procedure WithStmt(stn : StmtNode);
var
    ptrToRec, rectn, recbtn : TypeNode;
    implQual : VarNode;
    wqn : WithQualNode;
    error : boolean;
begin
    error := true;
    rectn := CheckExpr(stn^.withQual,EVALPOINT);
    recbtn := BaseType(rectn);
    if IsBadExpr(stn^.withQual) then begin
	{ do nothing }
    end else if recbtn^.kind <> DTRECORD then begin
	StmtError(stn,'With designator must be of type record');
    end else begin
	error := false;
	{ save allocation; allocate a temporary for pointer to rec }
	case target of
	    TARGETVAX : stn^.withSaveAllocNode := SaveAllocationNode(currProc^.mem);
	    TARGETTITAN : stn^.withSaveAllocNode := SaveAllocationNode(currProc^.mem);
	end;
	ptrToRec := PointerType(rectn,TKATNONE);
	implQual := DefineVar(nil,ptrToRec,MEMFAST,false);

	{ push a with qualifier record on the stack }
	new(wqn);
	wqn^.implQual := implQual;
	wqn^.recType := rectn;
	wqn^.baseVar := stn^.withQual^.baseVar;
	wqn^.basePtrType := stn^.withQual^.basePtrType;
	wqn^.next := withQualList;
	withQualList := wqn;
	stn^.withQualNode := wqn;

	{ open record scope to allow field names to be accessed }
	OpenScope(recbtn^.recScope,true);
    end;
    CheckStmtList(stn^.withBody);
    if not error then begin
	case target of
	    TARGETVAX : RestoreAllocationNode(currProc^.mem,stn^.withSaveAllocNode);
	    TARGETTITAN : RestoreAllocationNode(currProc^.mem,stn^.withSaveAllocNode);
	end;
	EndScope;
	withQualList := withQualList^.next;
    end;
end;

procedure ReturnStmt(stn : StmtNode);
var
    error : boolean;
    tn : TypeNode;
begin
    if TraceNstmt then begin
	writeln(output,'ReturnStmtNode');
    end;
    returnSeen := true;
    error := false;
    { NOTE: returnVal = nil is not an error in a procedure (non-function) }
    if stn^.returnVal = nil then begin
	if currProc^.procType^.funcType <> nil then begin
	    StmtError(stn,'Return statement in a function requires a value');
	    error := true;
	end;
    end else if currProc^.procType^.funcType = nil then begin
	StmtError(stn,'Return statement in a non-function cannot have a value');
	error := true;
    end else begin
	tn := CheckExpr(stn^.returnVal,EVALGET);
	if Assignable(currProc^.procType^.funcType,tn,stn^.returnVal) = nil
	then begin
	    StmtError(stn,'Return value not assignable to function result');
	    error := true;
	end;
    end;
    if error then begin
	BadStmt(stn);
    end;
end;

procedure ExitStmt(stn : StmtNode);
begin
    if TraceNstmt then begin
	writeln(output,'ExitStmtNode');
    end;
    if not loopActive then begin
	StmtError(stn,'Exit statement is not contained in a loop');
	BadStmt(stn);
    end;
end;

procedure CheckStmt(stn : StmtNode);
begin
    if stn^.bad then begin
	StmtError(stn,'CheckStmt: stmt already bad?');
    end;
    currLine := stn^.lineNumber;
    currFile := stn^.fileName;
    case stn^.kind of
	STMTASSIGN :	AssignStmt(stn);
	STMTPROC :	ProcStmt(stn);
	STMTIF :	IfStmt(stn);
	STMTWHILE :	WhileStmt(stn);
	STMTREPEAT :	RepeatStmt(stn);
	STMTLOOP :	LoopStmt(stn);
	STMTFOR :	ForStmt(stn);
	STMTWITH :	WithStmt(stn);
	STMTEXIT :	ExitStmt(stn);
	STMTRETURN :	ReturnStmt(stn);
	STMTCASE :	CaseStmt(stn);
    end;
end;

procedure CheckStmtList{(stl : StmtList)};
var
    stn : StmtNode;
begin
    if stl = nil then begin
	{ do nothing }
    end else begin
	stn := stl^.first;
	while stn <> nil do begin
	    CheckStmt(stn);
	    stn := stn^.next;
	end;
    end;
end;

procedure CheckReturn{(proc : ProcNode)};
begin
    if not returnSeen or (proc^.body^.last^.kind in
	    [STMTASSIGN, STMTREPEAT, STMTWHILE, STMTFOR, STMTWITH])
    then begin
	StmtError(proc^.body^.last,'Function does not end with a return statement');
    end;
end;
