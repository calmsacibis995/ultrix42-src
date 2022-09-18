(*#@(#)otree.p	4.1	Ultrix	7/17/90 *)
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
$Header: otree.p,v 1.6 84/06/06 13:05:02 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "optim.h"
#include "bexpr.h"
#include "otree.h"
#include "ocount.h"
#include "const.h"
#include "builtin.h"

var
    exprDepth : integer;  { to suppress reordering of top level expressions }

{ compute latest time, preserving sign encoding of mark }
function Latest{(a,b : OptTime) : OptTime};
var
    latest : OptTime;
begin
    if abs(a) > abs(b) then begin
	latest := abs(a);
    end else begin
	latest := abs(b);
    end;
    if (a < 0) or (b < 0) then begin
	latest := -latest;
    end;
    Latest := latest;
end;

function InsertExpr(en : ExprNode) : ExprNode;
var
    ven : ExprNode;
begin
    { Insert a node above en in the tree }
    { This is tricky because there may be pointers to this node (e.g., it }
    {  may be on a parameter list), so it is necessary to }
    {  copy the old en to the new node and use en as the inserted node }
    ven := NewExprNode(en^.kind);
    { copy other data from first node }
    ven^ := en^;
    InsertExpr := ven;
end;

function OptVarExpr(en : ExprNode; mode : EvalMode):OptTime;
var
    time, timet : OptTime;
    vn : VarNode;
    tn : TypeNode;
begin
    vn := en^.exprVar;
    if vn^.markTime.proc <> currOptProc then begin
	time := 0;
    end else begin
	time := vn^.markTime.time;
    end;
    tn := vn^.varType;
    if tn^.markTime.proc <> currOptProc then begin
	timet := 0;
    end else begin
	timet := tn^.markTime.time;
    end;
    OptVarExpr := Latest(time,timet);
end;

function OptValExpr(en : ExprNode; mode : EvalMode):OptTime;
var
    time : OptTime;
begin
    time := OptExpr(en^.exprVal,en,mode);
    OptValExpr := time;
end;

function OptCheckExpr(en : ExprNode; mode : EvalMode):OptTime;
var
    time : OptTime;
begin
    time := OptExpr(en^.checkExpr,en,mode);
    OptCheckExpr := time;
end;

function OptUnOpExpr{(en : ExprNode; mode : EvalMode):OptTime};
var
    time : OptTime;
begin
    if mode <> EVALGET then begin
	ExprError(en,'OptUnOpExpr: unexpected mode');
	time := 0;
    end else begin
	time := OptExpr(en^.opnd,en,mode);
    end;
    OptUnOpExpr := time;
end;

function OptReorderExpr(en : ExprNode) : OptTime;
var
    opnd1, opnd2, opnd11, opnd12, opnd21, opnd22 : ExprNode;
    opndon1, opndon2, opndon11, opndon12, opndon21, opndon22 : OptNode;
    isbin1, isbin2 : boolean;
    time, time1, time2, time11, time12, time21, time22 : OptTime;
begin
    opnd1 := en^.opnd1;
    isbin1 := false;
    if opnd1^.kind = EXPRBINOP then begin
	if opnd1^.exprBinOp = en^.exprBinOp then begin
	    isbin1 := true;
	end;
    end;
    opnd2 := en^.opnd2;
    isbin2 := false;
    if opnd2^.kind = EXPRBINOP then begin
	if opnd2^.exprBinOp = en^.exprBinOp then begin
	    isbin2 := true;
	end;
    end;
    if TraceOptim then begin
	write(output,'Reorder ');
	WriteExpr(output,en);
    end;
    if not isbin1 and not isbin2 then begin
	time1 := OptExpr(opnd1,en,EVALGET);
	time2 := OptExpr(opnd2,en,EVALGET);
	opndon1 := ExprToOpt(opnd1);
	opndon2 := ExprToOpt(opnd2);
	if TraceOptim then begin
	    writeln(output,' 1=',opndon1^.rootCongruent^.defineTime:1,' 2=',
		    opndon2^.rootCongruent^.defineTime:1);
	end;
	if opndon1^.rootCongruent^.defineTime >
		opndon2^.rootCongruent^.defineTime
	then begin
	    en^.opnd1 := opnd2;
	    en^.opnd2 := opnd1;
	end;
    end else if isbin1 and isbin2 then begin
	time1 := OptReorderExpr(opnd1);
	time2 := OptReorderExpr(opnd2);
	opnd11 := opnd1^.opnd1;
	opndon11 := ExprToOpt(opnd11);
	opnd12 := opnd1^.opnd2;
	opndon12 := ExprToOpt(opnd12);
	opnd21 := opnd2^.opnd1;
	opndon21 := ExprToOpt(opnd21);
	opnd22 := opnd2^.opnd2;
	opndon22 := ExprToOpt(opnd22);
	if TraceOptim then begin
	    writeln(output,' 12=',opndon12^.rootCongruent^.defineTime:1,' 21=',
		opndon21^.rootCongruent^.defineTime:1);
	end;
	if opndon12^.rootCongruent^.defineTime >
		opndon21^.rootCongruent^.defineTime
	then begin
	    opnd2^.opnd1 := opnd12;
	    opnd1^.opnd2 := opnd21;
	    opndon12^.parent := opnd2;
	    opndon21^.parent := opnd1;
	    time1 := Latest(opndon11^.rootCongruent^.defineTime,
				opndon21^.rootCongruent^.defineTime);
	    time2 := Latest(opndon12^.rootCongruent^.defineTime,
				opndon22^.rootCongruent^.defineTime);
	end;
	time1 := EnterExpr(opnd1,en,time1);
	time2 := EnterExpr(opnd2,en,time2);
    end else if isbin1 then begin
	time1 := OptReorderExpr(opnd1);
	time2 := OptExpr(opnd2,en,EVALGET);
	opnd11 := opnd1^.opnd1;
	opndon11 := ExprToOpt(opnd11);
	opnd12 := opnd1^.opnd2;
	opndon12 := ExprToOpt(opnd12);
	opndon2 := ExprToOpt(opnd2);
	if TraceOptim then begin
	    writeln(output,' 12=',opndon12^.rootCongruent^.defineTime:1,' 2=',
		opndon2^.rootCongruent^.defineTime:1);
	end;
	{ leave constants near the top right }
	if (opnd2^.kind <> EXPRCONST) and (opndon12^.rootCongruent^.defineTime >
		opndon2^.rootCongruent^.defineTime)
	then begin
	    opnd1^.opnd2 := opnd2;
	    opndon2^.parent := opnd1;
	    en^.opnd2 := opnd12;
	    opndon12^.parent := en;
	    time1 := Latest(opndon11^.rootCongruent^.defineTime,
				opndon2^.rootCongruent^.defineTime);
	    time2 := opndon12^.rootCongruent^.defineTime;
	end;
	time1 := EnterExpr(opnd1,en,time1);
    end else if isbin2 then begin
	time1 := OptExpr(opnd1,en,EVALGET);
	time2 := OptReorderExpr(opnd2);
	opnd21 := opnd2^.opnd1;
	opndon21 := ExprToOpt(opnd21);
	opnd22 := opnd2^.opnd2;
	opndon22 := ExprToOpt(opnd22);
	opndon1 := ExprToOpt(opnd1);
	if TraceOptim then begin
	    writeln(output,' 1=',opndon1^.rootCongruent^.defineTime:1,' 21=',
		opndon21^.rootCongruent^.defineTime:1);
	end;
	if opndon21^.rootCongruent^.defineTime >
		opndon1^.rootCongruent^.defineTime
	then begin
	    opnd2^.opnd1 := opnd1;
	    opndon1^.parent := opnd2;
	    en^.opnd1 := opnd21;
	    opndon21^.parent := en;
	    time1 := opndon21^.rootCongruent^.defineTime;
	    time2 := Latest(opndon1^.rootCongruent^.defineTime,
				opndon22^.rootCongruent^.defineTime);
	end;
	time2 := EnterExpr(opnd2,en,time2);
    end;
    OptReorderExpr := Latest(time1,time2);
end;

function OptBinOpExpr{(en : ExprNode; mode : EvalMode):OptTime};
var
    sol : SaveOptLevel;
    time, time1, time2 : OptTime;
begin
    if en^.exprBinOp in [TKAND,TKOR,TKAMPERSAND] then begin
	{ conditional expression evaluation }
	time1 := OptExpr(en^.opnd1,en,EVALGET);
	StartOptSplit(sol);
	time2 := OptExpr(en^.opnd2,en,EVALGET);
	EndOptSplit(sol);
	time := Latest(time1,time2);
    end else begin
	if (exprDepth > 0) and (en^.exprBinOp in [TKPLUS,TKASTERISK]) then begin
	    time := OptReorderExpr(en);
	end else begin
	    time1 := OptExpr(en^.opnd1,en,EVALGET);
	    time2 := OptExpr(en^.opnd2,en,EVALGET);
	    time := Latest(time1,time2);
	end;
    end;
    OptBinOpExpr := time;
end;

function OptSubscriptExpr{(pen : ExprNode; mode : EvalMode):OptTime};
var
    time, timex : OptTime;
    subsen, subspen, addren, addrpen : ExprNode;
    offset : cardinal;
    tn : TypeNode;
begin
    tn := pen^.exprType;
    offset := pen^.subsOffset;
    if offset <> 0 then begin
	{ make offset an explicit add }
	pen^.subsOffset := 0;
	subsen := InsertExpr(pen);
	subspen := pen;
	pen^.kind := EXPRBINOP;
	pen^.exprBinOp := TKPLUS;
	pen^.opnd1 := subsen;
	pen^.opnd2 := ConstExprNode(CardinalConst(offset));
	pen^.exprType := addressTypeNode;
	pen^.operType := addressTypeNode;
	pen^.opnd1^.exprType := addressTypeNode;
	pen^.opnd2^.exprType := addressTypeNode;
	SameExprLine(pen^.opnd2,pen);
	if mode <> EVALPOINT then begin
	    { need to insert value node }
	    addren := InsertExpr(pen);
	    addrpen := pen;
	    addren^.exprType := addressTypeNode;
	    pen^.kind := EXPRVAL;
	    pen^.exprVal := addren;
	    pen^.exprType := tn;
	    subspen := addren;
	end;
    end else begin
	subsen := pen;
	if mode <> EVALPOINT then begin
	    { need to insert value node }
	    addren := InsertExpr(pen);
	    addrpen := pen;
	    addren^.exprType := addressTypeNode;
	    pen^.kind := EXPRVAL;
	    pen^.exprVal := addren;
	    pen^.exprType := tn;
	    subsen := addren;
	end;
    end;
    { Need address of array and value of subscript }
    time := OptExpr(subsen^.subsExpr,subsen,EVALGET);
    timex := OptExpr(subsen^.arr,subsen,EVALPOINT);
    time := Latest(time,timex);
    if offset <> 0 then begin
	time := EnterExpr(subsen,subspen,time);
	time := EnterExpr(subspen^.opnd2,subspen,time);
    end;
    if mode <> EVALPOINT then begin
	time := EnterExpr(addren,addrpen,time);
    end;
    OptSubscriptExpr := time;
end;

function OptDotExpr{(pen : ExprNode; mode : EvalMode):OptTime};
var
    time : OptTime;
    en : ExprNode;
begin
    if mode <> EVALPOINT then begin
	{ need to insert value node }
	en := InsertExpr(pen);
	pen^.kind := EXPRVAL;
	pen^.exprVal := en;
    end else begin
	en := pen;
    end;
    time := OptExpr(en^.rec,en,EVALPOINT);
    if mode <> EVALPOINT then begin
	time := EnterExpr(en,pen,time);
    end;
    OptDotExpr := time;
end;

function OptDerefExpr{(pen : ExprNode; mode : EvalMode):OptTime};
var
    time : OptTime;
    en : ExprNode;
begin
    if mode <> EVALPOINT then begin
	{ need to insert value node }
	en := InsertExpr(pen);
	pen^.kind := EXPRVAL;
	pen^.exprVal := en;
    end else begin
	en := pen;
    end;
    { want pointer value, even if store }
    time := OptExpr(en^.ptr,en,EVALGET);
    if mode <> EVALPOINT then begin
	time := EnterExpr(en,pen,time);
    end;
    OptDerefExpr := time;
end;

function OptFuncProc{(procExpr : ExprNode; params : ExprList; parent : ExprNode)
	:OptTime};
var
    pexp : ExprNode;
    pn : ParamNode;
    proc : ProcNode;
    procType : TypeNode;
    done : boolean;
    time, timep : OptTime;
begin
    procType := procExpr^.exprType;
    done := false;
    
    { beware: type names can be used as both types and funcs }
    if procExpr^.kind = EXPRSYM then begin
	if procExpr^.exprSym^.kind = SYMTYPE then begin
	    time := OptExpr(params^.first,parent,EVALGET);
	    done := true;
	end;
    end;

    { check for builtin function (must be a constant) }
    if done then begin
	{ do nothing }
    end else if procExpr^.kind = EXPRCONST then begin
	if procExpr^.exprConst^.kind = DTPROC then begin
	    proc := procExpr^.exprConst^.procVal;
	    if proc^.builtin <> BIPNOTBIP then begin
	    	time := OptBuiltin(procExpr,proc,params);
	    	done := true;
	    end;
	end;
    end;

    if done then begin
	{ do nothing }
    end else begin
	time := 0;
	if (params = nil) or (procType^.paramList = nil) then begin
	    { do nothing }
	end else begin
	    { first allow expressions to be referenced }
	    pexp := params^.first;
	    pn := procType^.paramList^.first;
	    while pexp <> nil do begin
		case pn^.kind of
		    PARAMVAR, PARAMVALUE : begin
			if pn^.reference then begin
			    timep := OptExpr(pexp,parent,EVALPOINT);
			end else begin
			    timep := OptExpr(pexp,parent,EVALGET);
			end;
		    end;
		    PARAMARRAYVAR,PARAMARRAYVALUE : begin
			timep := OptExpr(pexp,parent,EVALGET);
			time := Latest(time,timep);
			if not pn^.paramType^.nocount then begin
			    pexp := pexp^.next;
			    timep := OptExpr(pexp,parent,EVALGET);
			end;
		    end;
		end;
		time := Latest(time,timep);
		pexp := pexp^.next;
		pn := pn^.next;
	    end;
	    { then mark var parameters as modified }
	    pexp := params^.first;
	    pn := procType^.paramList^.first;
	    while pexp <> nil do begin
		if pn^.kind in [PARAMVAR,PARAMARRAYVAR] then begin
		    MarkOptExpr(pexp);
		end;
		if pn^.kind in [PARAMARRAYVAR,PARAMARRAYVALUE] then begin
		    if not pn^.paramType^.nocount then begin
			pexp := pexp^.next;
		    end;
		end;
		pexp := pexp^.next;
		pn := pn^.next;
	    end;
	end;
	{ for safety, mark everything as changed by procedure call }
	MarkOptAll;
	time := -optTime;
    end;
    OptFuncProc := time;
end;

function OptFuncExpr{(en : ExprNode; mode : EvalMode):OptTime};
var
    func : ExprNode;
    time, timep : OptTime;
begin
    time := OptExpr(en^.func,en,EVALGET);
    timep := OptFuncProc(en^.func,en^.params,en);
    OptFuncExpr := Latest(time,timep);
end;

function OptExpr{(en : ExprNode; pen : ExprNode; mode : EvalMode):OptTime};
var
    time : OptTime;
begin
    time := 0;
    if pen = nil then begin
	exprDepth := 0;
    end else begin
	exprDepth := exprDepth + 1;
    end;
    if en = nil then begin
	ExprError(en,'OptExpr: nil expression?');
    end else if en^.exprType = nil then begin
	ExprError(en,'OptExpr: exprType = nil?');
    end else begin
	case en^.kind of
	    EXPRBAD :	ExprError(en,'OptExpr: found EXPRBAD?');
	    EXPRNAME :	;
	    EXPRSYM :	;
	    EXPRVAR :	time := OptVarExpr(en,mode);
	    EXPRCONST :	;
	    EXPRUNOP :	time := OptUnOpExpr(en,mode);
	    EXPRBINOP :	time := OptBinOpExpr(en,mode);
	    {
	    EXPRSUBSCR :time := OptSubscriptExpr(en,mode);
	    EXPRDOT :	time := OptDotExpr(en,mode);
	    EXPRDEREF :	time := OptDerefExpr(en,mode);
	    }
	    EXPRFUNC :	time := OptFuncExpr(en,mode);
	    EXPRSET :	ExprError(en,'OptExpr: found EXPRSET');
	    EXPRVAL :	time := OptValExpr(en,mode);
	    EXPRCHECK :	time := OptCheckExpr(en,mode);
	end;
	time := EnterExpr(en,pen,time);
    end;
    OptExpr := time;
end;

procedure OptExprS(en : ExprNode; mode : EvalMode);
var
    ignore : OptTime;
begin
    ignore := OptExpr(en,nil,mode);
end;

procedure OptAssignStmt{(stn : StmtNode)};
begin
    OptExprS(stn^.rhs,EVALGET);
    OptExprS(stn^.lhs,EVALPUT);
    MarkOptExpr(stn^.lhs);
end;

procedure OptProcStmt{(stn : StmtNode)};
var
    ignore : OptTime;
begin
    OptExprS(stn^.proc,EVALGET);
    ignore := OptFuncProc(stn^.proc,stn^.params,nil);
end;

procedure OptIfStmt{(stn : StmtNode)};
var
    sol : SaveOptLevel;
begin
    OptExprS(stn^.ifCond,EVALGET);
    StartOptSplit(sol);
    OptStmtList(stn^.thenList);
    NextOptSplit(sol);
    OptStmtList(stn^.elseList);
    EndOptSplit(sol);
end;

procedure OptCase{(stn : StmtNode; caseNode : CaseNode)};
begin
    OptStmtList(caseNode^.stmts);
end;

procedure OptCaseStmt{(stn : StmtNode)};
var
    caseNode : CaseNode;
    sol : SaveOptLevel;
begin
    OptExprS(stn^.caseSel,EVALGET);
    if stn^.cases = nil then begin
	{ do nothing }
    end else begin
	StartOptSplit(sol);
	caseNode := stn^.cases^.first;
	while caseNode <> nil do begin
	    OptCase(stn,caseNode);
	    caseNode := caseNode^.next;
	    NextOptSplit(sol);
	end;
	OptStmtList(stn^.caseElse);
	EndOptSplit(sol);
    end;
end;

procedure OptWhileStmt{(stn : StmtNode)};
var
    sol : SaveOptLevel;
begin
    StartOptLoop(sol);
    OptExprS(stn^.whileCond,EVALGET);
    OptStmtList(stn^.whileBody);
    EndOptLoop(sol,stn^.whilePreEval);
end;

procedure OptRepeatStmt{(stn : StmtNode)};
var
    sol : SaveOptLevel;
begin
    StartOptLoop(sol);
    OptStmtList(stn^.repeatBody);
    OptExprS(stn^.repeatCond,EVALGET);
    EndOptLoop(sol,stn^.repeatPreEval);
end;

procedure OptLoopStmt{(stn : StmtNode)};
var
    sol : SaveOptLevel;
begin
    StartOptLoop(sol);
    OptStmtList(stn^.loopBody);
    EndOptLoop(sol,stn^.loopPreEval);
end;

procedure OptForStmt{(stn : StmtNode)};
var
    sym : Symbol;
    bt : TypeNode;
    error : boolean;
    ien : ExprNode;
    sol : SaveOptLevel;
begin
    OptExprS(stn^.forFrom,EVALGET);
    OptExprS(stn^.forTo,EVALGET);
    if stn^.forBy <> nil then begin
	OptExprS(stn^.forBy,EVALGET);
    end else begin
    end;
    { introduce expr node for loop index }
    ien := NewExprNode(EXPRVAR);
    SameExprLine(ien,stn^.forFrom);
    ien^.exprType := stn^.forIndexVar^.varType;
    ien^.exprVar := stn^.forIndexVar;
    ien^.baseVar := stn^.forIndexVar;
    OptExprS(ien,EVALPUT);
    { mark it to indicate assignment at start of loop }
    MarkOptExpr(ien);
    StartOptLoop(sol);
    OptStmtList(stn^.forBody);
    { look for induction expressions }
    if not OptNind then begin
	AnalyzeForLoop(ien,stn,sol);
    end;
    { mark it to indicate increment at end of loop }
    MarkOptExpr(ien);
    EndOptLoop(sol,stn^.forPreEval);
end;

procedure OptWithStmt{(stn : StmtNode)};
begin
    OptExprS(stn^.withQual,EVALPOINT);
    OptStmtList(stn^.withBody);
end;

procedure OptReturnStmt{(stn : StmtNode)};
begin
    if stn^.returnVal = nil then begin
    end else begin
	OptExprS(stn^.returnVal,EVALGET);
	OptRecursionReturn(currOptProc,stn);
    end;
end;

procedure OptExitStmt{(stn : StmtNode)};
begin
end;

procedure OptStmt{(stn : StmtNode)};
begin
    if stn^.bad then begin
	StmtError(stn,'OptStmt: stmt bad?');
    end;
    case stn^.kind of
	STMTASSIGN :	OptAssignStmt(stn);
	STMTPROC :	OptProcStmt(stn);
	STMTIF :	OptIfStmt(stn);
	STMTWHILE :	OptWhileStmt(stn);
	STMTREPEAT :	OptRepeatStmt(stn);
	STMTLOOP :	OptLoopStmt(stn);
	STMTFOR :	OptForStmt(stn);
	STMTWITH :	OptWithStmt(stn);
	STMTEXIT :	OptExitStmt(stn);
	STMTRETURN :	OptReturnStmt(stn);
	STMTCASE :	OptCaseStmt(stn);
    end;
end;

procedure OptStmtList{(stl : StmtList)};
var
    stn : StmtNode;
begin
    if stl = nil then begin
	{ do nothing }
    end else begin
	stn := stl^.first;
	while stn <> nil do begin
	    OptStmt(stn);
	    stn := stn^.next;
	end;
    end;
end;

procedure OptProc{(pn : ProcNode)};
begin
    currOptProc := pn;
    if pn^.body <> nil then begin
	if TraceOptim then begin
	    write(output,'OptProc ');
	    WriteString(output,pn^.name);
	    writeln(output);
	end;
	StartOptProc;
	OptStmtList(pn^.body);
	OptRecursionProc(pn,pn^.body);
	EndOptProc;
	if TraceOptim then begin
	    DumpOptExprs;
	end;
	CountProc(pn);
    end;
end;

procedure OptModule{(mn : ModuleNode)};
var
    submn : ModuleNode;
    pn : ProcNode;
begin
    submn := mn^.modules^.first;
    while submn <> nil do begin
	OptModule(submn);
	submn := submn^.next;
    end;
    pn := mn^.procs^.first;
    while pn <> nil do begin
	OptProc(pn);
	pn := pn^.next;
    end;
    if mn^.body <> nil then begin
	StartOptProc;
	OptStmtList(mn^.body);
	EndOptProc;
    end;
end;
