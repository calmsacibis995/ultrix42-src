(*#@(#)bstmt.p	4.1	Ultrix	7/17/90 *)
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
$Header: bstmt.p,v 1.4 84/05/19 11:35:25 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "bexpr.h"
#include "bstmt.h"

function NewStmtNode(kind : StmtKind) : StmtNode;
var
    stn : StmtNode;
begin
    new(stn);
    stn^.kind := kind;
    stn^.fileName := currFile;
    stn^.lineNumber := currLine;
    stn^.bad := false;
    NewStmtNode := stn;
end;

procedure SameStmtLine(stn:StmtNode; en:ExprNode);
begin
    stn^.fileName := en^.fileName;
    stn^.lineNumber := en^.lineNumber;
end;

function AssignStmtNode(lhs, rhs : ExprNode) : StmtNode;
var
    stn : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'AssignStmtNode');
    end;
    stn := NewStmtNode(STMTASSIGN);
    stn^.lhs := lhs;
    stn^.rhs := rhs;
    stn^.assignOp := TKASSIGN;
    SameStmtLine(stn,lhs);
    AssignStmtNode := stn;
end;

function ProcStmtNode(proc : ExprNode; params : ExprList) : StmtNode;
var
    stn : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'ProcStmtNode');
    end;
    stn := NewStmtNode(STMTPROC);
    stn^.proc := proc;
    stn^.params := params;
    SameStmtLine(stn,proc);
    ProcStmtNode := stn;
end;

function IfStmtNode(cond : ExprNode; thenList, elseList : StmtList) : StmtNode;
var
    stn : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'IfStmtNode');
    end;
    stn := NewStmtNode(STMTIF);
    stn^.ifCond := cond;
    stn^.thenList := thenList;
    stn^.elseList := elseList;
    SameStmtLine(stn,cond);
    IfStmtNode := stn;
end;

function AddCase(stn : StmtNode; caseNode : CaseNode) : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'AddCase');
    end;
    stn^.cases := AddToCaseList(stn^.cases, caseNode);
    AddCase := stn;
end;

function AddCaseElse(stn : StmtNode; caseElse : StmtList) : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'AddElseCase');
    end;
    stn^.caseElse := caseElse;
    AddCaseElse := stn;
end;

function CaseStmtNode(caseSel : ExprNode) : StmtNode;
var
    stn : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'CaseStmtNode');
    end;
    stn := NewStmtNode(STMTCASE);
    stn^.caseSel := caseSel;
    stn^.cases := nil;
    stn^.caseTree := nil;
    stn^.caseElse := nil;
    SameStmtLine(stn,caseSel);
    CaseStmtNode := stn;
end;

function WhileStmtNode(cond : ExprNode; whileBody : StmtList) : StmtNode;
var
    stn : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'WhileStmtNode');
    end;
    stn := NewStmtNode(STMTWHILE);
    stn^.whileCond := cond;
    stn^.whileBody := whileBody;
    stn^.whilePreEval := nil;
    SameStmtLine(stn,cond);
    WhileStmtNode := stn;
end;

function RepeatStmtNode(repeatBody : StmtList; cond : ExprNode) : StmtNode;
var
    stn : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'RepeatStmtNode');
    end;
    stn := nil;
    stn := NewStmtNode(STMTREPEAT);
    stn^.repeatBody := repeatBody;
    stn^.repeatCond := cond;
    stn^.repeatPreEval := nil;
    SameStmtLine(stn,cond);
    RepeatStmtNode := stn;
end;

function StartLoop : StmtNode;
var
    stn : StmtNode;
begin
    stn := NewStmtNode(STMTLOOP);
    StartLoop := stn;
end;

function LoopStmtNode(stn : StmtNode; loopBody : StmtList) : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'LoopStmtNode');
    end;
    stn^.loopBody := loopBody;
    stn^.loopPreEval := nil;
    LoopStmtNode := stn;
end;

function ForStmtNode(stn : StmtNode; forBody : StmtList) : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'ForStmtNode');
    end;
    stn^.forBody := forBody;
    stn^.forPreEval := nil;
    ForStmtNode := stn;
end;

function StartFor(index : String; forFrom, forTo : ExprNode;
    forBy : ExprNode) : StmtNode;
var
    stn : StmtNode;
begin
    stn := NewStmtNode(STMTFOR);
    stn^.forIndexName := index;
    stn^.forFrom := forFrom;
    stn^.forTo := forTo;
    stn^.forBy := forBy;
    StartFor := stn;
end;

function StartWith (withQual : ExprNode) : StmtNode;
var
    stn : StmtNode;
begin
    stn := NewStmtNode(STMTWITH);
    stn^.withQual := withQual;
    StartWith := stn;
end;


function WithStmtNode(stn : StmtNode; withBody : StmtList) : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'WithStmtNode');
    end;
    stn^.withBody := withBody;
    WithStmtNode := stn;
end;

function ReturnStmtNode(returnVal : ExprNode) : StmtNode;
var
    stn : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'ReturnStmtNode');
    end;
    stn := NewStmtNode(STMTRETURN);
    stn^.returnVal := returnVal;
    ReturnStmtNode := stn;
end;

function ExitStmtNode : StmtNode;
var
    stn : StmtNode;
begin
    if TraceNstmt then begin
	writeln(output,'ExitStmtNode');
    end;
    stn := NewStmtNode(STMTEXIT);
    ExitStmtNode := stn;
end;

function MakeCase (labels : ExprSetList; stmts : StmtList) : CaseNode;
var
    cn : CaseNode;
begin
    new(cn);
    cn^.labelExprs := labels;
    cn^.labelConsts := nil;
    cn^.stmts := stmts;
    MakeCase := cn;
end;

procedure PrintStmt{(stn:StmtNode;indent:integer)};
var
    ten : ExprNode;
    casen : CaseNode;
    cn : ConstNode;
    csn : ConstSetNode;
    esn : ExprSetNode;
begin
    if stn = nil then begin
	Indent(output,indent);
	writeln(output,'Statememt EMPTY');
    end else begin
	Indent(output,indent);
	writeln(output,'Statememt ',stn^.kind);
	case stn^.kind of
	    STMTASSIGN: begin
		PrintExpr(stn^.lhs,indent+INDENT);
		Indent(output,indent);
		writeln(output,':=');
		PrintExpr(stn^.rhs,indent+INDENT);
	    end;
	    STMTPROC : begin
		PrintExpr(stn^.proc,indent+INDENT);
		if stn^.params <> nil then begin
		    ten := stn^.params^.first;
		    while ten <> nil do begin
			Indent(output,indent);
			writeln(output,'****');
			PrintExpr(ten,indent+INDENT);
			ten := ten^.next;
		    end;
		end;
	    end;
	    STMTIF: begin
		PrintExpr(stn^.ifCond,indent+INDENT);
		Indent(output,indent);
		writeln(output,'then');
		PrintStmtList(stn^.thenList,indent+INDENT);
		Indent(output,indent);
		writeln(output,'else');
		PrintStmtList(stn^.elseList,indent+INDENT);
	    end;
	    STMTWHILE: begin
		PrintExpr(stn^.whileCond,indent+INDENT);
		Indent(output,indent);
		writeln(output,'do');
		PrintStmtList(stn^.whileBody,indent+INDENT);
	    end;
	    STMTREPEAT: begin
		PrintStmtList(stn^.repeatBody,indent+INDENT);
		Indent(output,indent);
		writeln(output,'until');
		PrintExpr(stn^.repeatCond,indent+INDENT);
	    end;
	    STMTLOOP: begin
		PrintStmtList(stn^.loopBody,indent+INDENT);
	    end;
	    STMTFOR: begin
		Indent(output,indent);
		write(output,'index ');
		WriteString(output,stn^.forIndexName);
		writeln(output);
		PrintExpr(stn^.forFrom,indent+INDENT);
		Indent(output,indent);
		writeln(output,'from');
		PrintExpr(stn^.forFrom,indent+INDENT);
		Indent(output,indent);
		writeln(output,'to');
		PrintExpr(stn^.forTo,indent+INDENT);
		Indent(output,indent);
		writeln(output,'by');
		PrintExpr(stn^.forBy,indent+INDENT);
		Indent(output,indent);
		writeln(output,'do');
		PrintStmtList(stn^.forBody,indent+INDENT);
	    end;
	    STMTWITH: begin
		PrintExpr(stn^.withQual,indent+INDENT);
		Indent(output,indent);
		writeln(output,'do');
		PrintStmtList(stn^.withBody,indent+INDENT);
	    end;
	    STMTRETURN: begin
		PrintExpr(stn^.returnVal,indent+INDENT);
	    end;
	    STMTCASE : begin
		PrintExpr(stn^.caseSel,indent+INDENT);
		if stn^.cases <> nil then begin
		    casen := stn^.cases^.first;
		    while casen<> nil do begin
			if casen^.labelExprs <> nil then begin
			    esn := casen^.labelExprs^.first;
			    while esn <> nil do begin
				PrintExpr(esn^.lower,indent);
				if esn^.upper <> esn^.lower then begin
				    Indent(output,indent);
				    writeln(output,'..');
				    PrintExpr(esn^.upper,indent);
				end;
				esn := esn^.next;
			    end;
			    PrintStmtList(casen^.stmts,indent+INDENT);
			end;
			casen := casen^.next;
		    end;
		end;
		if stn^.caseElse <> nil then begin
		    Indent(output,indent);
		    writeln(output,'else');
		    PrintStmtList(stn^.caseElse,indent+INDENT);
		end;
	    end;
	    STMTEXIT : begin
	    end;
	end;
    end;
end;

procedure PrintStmtList{(stl:StmtList;indent:integer)};
var
    stn : StmtNode;
begin
    if stl = nil then begin
	writeln(output,'NIL Statement list');
    end else begin
	stn := stl^.first;
	while stn <> nil do begin
	    PrintStmt(stn,indent+INDENT);
	    stn := stn^.next;
	end;
    end;
end;

function AddToStmtList (list : StmtList; newOne : StmtNode) : StmtList;
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
    AddToStmtList := list;
end;

function AddToCaseList{(list : CaseList; newOne : CaseNode) : CaseList};
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
    AddToCaseList := list;
end;
