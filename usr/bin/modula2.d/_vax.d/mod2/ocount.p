(*#@(#)ocount.p	4.1	Ultrix	7/17/90 *)
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
$Header: ocount.p,v 1.7 84/06/06 13:04:08 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "bexpr.h"
#include "optim.h"
#include "ocount.h"
#include "builtin.h"
#include "decls.h"

const
    MINCOUNTFORREG = 3;	{  number of references to get allocated to a register }
type
    StorageCount = record
	count : integer;
	offset : MemoryOffset;
	addressed : boolean;
    end;
    TempSet = set of 0..NUMTEMPS;
var
    refCount : array [0..MAXTEMPSTORAGE] of StorageCount;
    maxTemp : TempNumber;
    inUseTemps, inUseTempsToBeFreed : TempSet;

procedure InitTemps;
begin
    inUseTemps := [];
    inUseTempsToBeFreed := [];
    maxTemp := 0;
end;

function AllocTemp{ : TempNumber};
var
    t : TempNumber;
    found : boolean;
begin
    case target of
	TARGETVAX: begin
	    t := 1;
	    while not found and (t <= NUMTEMPS) do begin
		if not (t in inUseTemps) then begin
		    found := true;
		end else begin
		    t := t + 1;
		end;
	    end;
	    if not found then begin
		Error('AllocTemp: Too many temps?');
		t := NULLTEMP;
	    end else begin
		inUseTemps := inUseTemps + [t];
		if t > maxTemp then begin
		    maxTemp := t;
		end;
	    end;
	end;
	TARGETTITAN : begin
    { when unlimited temporaries works
	    maxTemp := maxTemp + 1;
	    t := maxTemp;
    }
	    t := 1;
	    while not found and (t <= NUMTEMPS) do begin
		if not (t in inUseTemps) then begin
		    found := true;
		end else begin
		    t := t + 1;
		end;
	    end;
	    if not found then begin
		Error('AllocTemp: Too many temps?');
		t := NULLTEMP;
	    end else begin
		inUseTemps := inUseTemps + [t];
		if t > maxTemp then begin
		    maxTemp := t;
		end;
	    end;
	end;
    end;
    AllocTemp := t;
end;

procedure FreeTemp{(t : TempNumber)};
begin
    case target of
	TARGETVAX : begin
	    if not (t in inUseTemps) or (t in inUseTempsToBeFreed) then begin
		ErrorNumber('FreeTemp: not in use? ',t);
	    end;
	    inUseTempsToBeFreed := inUseTempsToBeFreed + [t];
	end;
	TARGETTITAN : begin
	    if not (t in inUseTemps) or (t in inUseTempsToBeFreed) then begin
		ErrorNumber('FreeTemp: not in use? ',t);
	    end;
	    inUseTempsToBeFreed := inUseTempsToBeFreed + [t];
	end;
    end;
end;

procedure UpdateTemps;
begin
    inUseTemps := inUseTemps - inUseTempsToBeFreed;
    inUseTempsToBeFreed := [];
end;

procedure ReduceExprNeededCounts{(en : ExprNode; count : integer)};
var
    on : OptNode;
begin
    on := ExprToOpt(en);
    on := on^.rootEqual;
    on^.neededCount := on^.neededCount - count;
    on^.referencedCount := on^.referencedCount - count;
    if on^.tempNumber <> NULLTEMP then begin
	{ already a subexpression, children already reduced }
    end else begin
	ReduceNeededCounts(on,count);
    end;
end;

{ reduce use counts in all expression nodes used by this expression }
procedure ReduceNeededCounts{(on : OptNode; count : integer)};
var
    en, pen : ExprNode;
begin
    en := on^.expr;
    case en^.kind of
	EXPRCONST,
	EXPRVAR : { no descendents };
	EXPRNAME,
	EXPRSYM,
	EXPRSET : ExprError(en,'ReduceNeededCounts: unexpected expr');
	EXPRUNOP : begin
	    ReduceExprNeededCounts(en^.opnd,count);
	end;
	EXPRBINOP : begin
	    ReduceExprNeededCounts(en^.opnd1,count);
	    ReduceExprNeededCounts(en^.opnd2,count);
	end;
	EXPRSUBSCR : begin
	    ReduceExprNeededCounts(en^.arr,count);
	    ReduceExprNeededCounts(en^.subsExpr,count);
	end;
	EXPRDOT : begin
	    ReduceExprNeededCounts(en^.rec,count);
	end;
	EXPRDEREF : begin
	    ReduceExprNeededCounts(en^.ptr,count);
	end;
	EXPRFUNC : begin
	    ReduceExprNeededCounts(en^.func,count);
	    pen := en^.params^.first;
	    while pen <> nil do begin
		ReduceExprNeededCounts(pen,count);
		pen := pen^.next;
	    end;
	end;
	EXPRVAL : begin
	    ReduceExprNeededCounts(en^.exprVal,count);
	end;
	EXPRCHECK : begin
	    ReduceExprNeededCounts(en^.checkExpr,count);
	end;
    end;
end;

procedure CommonSubExpr{(en : ExprNode; mode : EvalMode; state : EvalState)};
var
    on, ron, cron : OptNode;
begin
    on := ExprToOpt(en);
    ron := on^.rootEqual;
    if not ron^.eligible and (ron^.neededCount > 1)
	and (ron^.nonTrivial or (ron^.containedNonTrivial <> nil))
    then begin
	{ got a possible subexpression }
	if SizeOf(en^.exprType) > WORDSIZE then begin
	    { cannot handle multiple word sub expression }
	end else if ron^.nonTrivial then begin
	    ron^.eligible := true;
	end else begin
	    { must contain a non-trivial expression }
	    cron := ron^.containedNonTrivial^.rootEqual;
	    { see if contained expression is needed only here }
	    if cron^.neededCount = ron^.neededCount then begin
		{ this is the only use of the nonTrivial expression }
		{ make this be the subexpression }
		ron^.eligible := true;
	    end;
	end;
    end;
    if ron^.eligible then begin
	if ron^.tempNumber <> NULLTEMP then begin
	    ron^.usedCount := ron^.usedCount + 1;
	    if state = EVALPRE then begin
		{ already calculated }
	    end else if state = EVALPOST then begin
	        if ron^.usedCount < ron^.neededCount then begin
		    { not finished with it yet }
		end else begin
		    if TraceOptim then begin
			write(output,'CSE: ',on^.uniqueId:1,' ',on^.usage,' ',
			    state:1,' freetemp=',ron^.tempNumber,' ');
			WriteExpr(output,on^.expr);
			writeln(output);
		    end;
		    { finished with saved value }
		    FreeTemp(ron^.tempNumber);
		end;
	    end else begin
		{ reuse saved value }
		if ron^.usedCount = ron^.neededCount then begin
		    on^.usage := OUSELAST;
		    refCount[MAXTSTORAGE+ron^.tempNumber].count :=
				    refCount[MAXTSTORAGE+ron^.tempNumber].count
				    + on^.loopNest;
		    if TraceOptim then begin
			write(output,'CSE: ',on^.uniqueId:1,' ',on^.usage,' ',
			    state:1,' freetemp=',ron^.tempNumber,' ');
			WriteExpr(output,on^.expr);
			writeln(output);
		    end;
		    FreeTemp(ron^.tempNumber);
		end else begin
		    on^.usage := OUSEMIDDLE;
		    refCount[MAXTSTORAGE+ron^.tempNumber].count :=
				    refCount[MAXTSTORAGE+ron^.tempNumber].count
				    + on^.loopNest;
		    if TraceOptim then begin
			write(output,'CSE: ',on^.uniqueId:1,' ',on^.usage,' ',state:1,' ');
			WriteExpr(output,on^.expr);
			writeln(output);
		    end;
		end;
	    end;
	end else begin
	    { allocate temp to save it in }
	    ron^.tempNumber := AllocTemp;
	    if ron^.tempNumber = NULLTEMP then begin
		{ couldn't allocate a temp }
		ron^.eligible := false;	{ can't make it a cse }
	    end else begin
		{ will save a value, make it look like it was used }
		if TraceOptim then begin
		    write(output,'Reduce: rc=',ron^.referencedCount:1,', nc=',
			ron^.neededCount:1,', uc=', ron^.usedCount:1,
			', temp=',ron^.tempNumber:1,', expr=');
		    WriteExpr(output,on^.expr);
		    writeln(output);
		end;
		ReduceNeededCounts(ron,ron^.referencedCount-1);
		{ evaluate expression }
		if ron^.address then begin
		    { cse is the address }
		    DoCountExpr(en,EVALPOINT);
		end else begin
		    DoCountExpr(en,mode);
		end;
		if state = EVALPRE then begin
		    on^.usage := OUSEGENERATE;
		    refCount[MAXTSTORAGE+ron^.tempNumber].count :=
				    refCount[MAXTSTORAGE+ron^.tempNumber].count
				    + on^.loopNest;
		    if TraceOptim then begin
			write(output,'CSE: ',on^.uniqueId:1,' ',on^.usage,' ',state:1,' ');
			WriteExpr(output,on^.expr);
			writeln(output);
		    end;
		end else begin
		    on^.usage := OUSEFIRST;
		    refCount[MAXTSTORAGE+ron^.tempNumber].count :=
				    refCount[MAXTSTORAGE+ron^.tempNumber].count
				    + on^.loopNest;
		    if TraceOptim then begin
			write(output,'CSE: ',on^.uniqueId:1,' ',on^.usage,' ',state:1,' ');
			WriteExpr(output,on^.expr);
			writeln(output);
		    end;
		end;
		{ count this as a use, too }
		ron^.usedCount := ron^.usedCount + 1;
	    end;
	end;
    end else begin
	{ not subexpression, do normal evaluate }
	DoCountExpr(en,mode);
    end;
end;

procedure InitCounts;
var
    index : integer;
begin
    InitTemps;
    for index := 0 to MAXTEMPSTORAGE do begin
	refCount[index].count := 0;
	refCount[index].offset := index * WORDSIZE;
	refCount[index].addressed := false;
    end;
end;

procedure ComputeCounts(proc : ProcNode);
var
    rooton, classon, equalon, on : OptNode;
    vn : VarNode;
    index : integer;
begin
    rooton := cseRootExpr[EXPRVAR];
    if rooton <> nil then begin
	classon := rooton;
	repeat
	    vn := classon^.expr^.exprVar;
	    if vn^.address.kind = MEMGLOBAL then begin
		{ don't worry about globals }
	    end else if (vn^.address.kind <> MEMFAST) or
		    (vn^.address.proc <> proc)
	    then begin
		{ doesn't qualify, but tally uplevel refs }
		proc^.doesUpLevel := proc^.doesUpLevel + [vn^.address.level]; 
	    end else begin
		{ candidate }
		index := trunc(vn^.address.offset) div WORDSIZE;
		if proc^.displayLevel in proc^.containsUpLevel then begin
		    { possible uplevel reference, assume worst }
		    refCount[index].addressed := true;
		end else begin
		    equalon := classon;
		    repeat
			if equalon^.rootEqual = equalon then begin
			    (* Experiment: just multiply by reference count 
			    on := equalon;
			    repeat
				refCount[index].count := refCount[index].count
						+ on^.loopNest;
				on := on^.nextEqual;
			    until on = equalon;
			    *)
			    refCount[index].count := refCount[index].count
				+ equalon^.loopNest * equalon^.referencedCount;
			end;
			equalon := equalon^.nextCongruent;
		    until equalon = classon;
		end;
	    end;
	    classon := classon^.nextClass;
	until classon = rooton;
    end;
    if proc^.enclosing <> nil then begin
	proc^.enclosing^.containsUpLevel := proc^.enclosing^.containsUpLevel +
		    proc^.doesUpLevel;
    end;
end;

procedure SortArray(l, r : integer);
var
    i, j : integer;
    tmp : StorageCount;
begin
    for i := l to r do begin
	if refCount[i].addressed then begin
	    refCount[i].count := -1000000-refCount[i].count;
	end;
    end;
    for i := l to r-1 do begin
	for j := i+1 to r do begin
	    if refCount[i].count < refCount[j].count then begin
		tmp := refCount[i];
		refCount[i] := refCount[j];
		refCount[j] := tmp;
	    end;
	end;
    end;
end;

procedure ReassignStorage(proc : ProcNode);
var
    index, maxindex, ireg : integer;
    tm : TempMapNode;
begin
    ComputeCounts(proc);

    case target of
	TARGETVAX : begin
	    maxindex := trunc(proc^.mem^.maximum[MEMFAST]) div WORDSIZE - 1;
	    for index := 1 to maxTemp do begin
		refCount[maxindex+index] := refCount[MAXTSTORAGE+index];
	    end;
	    maxindex := maxindex + maxTemp;
	    if TraceOptim then begin
		writeln(output,'Reassign storage before T',maxindex-maxTemp:1,
				' O',maxTemp:1);
		for index := 0 to maxindex do begin
		    writeln(output,index:3,refCount[index].count:5,
			    trunc(refCount[index].offset) div WORDSIZE:5,' ',
			    refCount[index].addressed);
		end;
	    end;
	    SortArray(0,maxindex);
	    if TraceOptim then begin
		writeln(output,'Reassign storage after');
		for index := 0 to maxindex do begin
		    writeln(output,index:3,refCount[index].count:5,
			    trunc(refCount[index].offset) div WORDSIZE:5,' ',
			    refCount[index].addressed);
		end;
	    end;
	    new(tm);
	    tm^.numReg := -1;
	    for index := 0 to MAXTEMPSTORAGE do begin
		tm^.map[index] := -1;
	    end;
	    for index := 0 to maxindex do begin
		ireg := trunc(refCount[index].offset) div WORDSIZE;
		tm^.map[ireg] := index * WORDSIZE;
		if (refCount[index].count >= MINCOUNTFORREG) and
			not refCount[index].addressed and (index > tm^.numReg)
		then begin
		    tm^.numReg := index;
		end;
	    end;
	    proc^.mem^.maximum[MEMFAST] := proc^.mem^.maximum[MEMFAST]
			    + maxTemp * WORDSIZE;
	    if TraceOptim then begin
		writeln(output,'Reassign storage map ',tm^.numReg:1);
		for index := 0 to MAXTEMPSTORAGE do begin
		    if tm^.map[index] <> -1 then begin
			if index < maxindex - maxTemp then begin
			    writeln(output,'T',index:3,
				trunc(tm^.map[index]) div WORDSIZE:5);
			end else begin
			    writeln(output,'O',index:3,
				trunc(tm^.map[index]) div WORDSIZE:5);
			end;
		    end;
		end;
	    end;
	    proc^.tempMap := tm;
	end;
	TARGETTITAN : begin
	    new(tm);
	    tm^.numReg := maxTemp;
	    proc^.tempMap := tm;
	end;
    end;
end;

procedure CountProc {(proc : ProcNode)};
var
    code : CodeNode;
begin
    InitCounts;
    code := proc^.code^.first;
    while code <> nil do begin
	CountStmtList(code^.stmts);
	code := code^.next;
    end;
    UpdateTemps;
    if inUseTemps <> [] then begin
	Error('CountProc: temps in use after processing');
    end;
    ReassignStorage(proc);
end;

procedure CountStmtList{(stmts : StmtList)};
var
    stmt : StmtNode;
begin
    if stmts <> nil then begin
	stmt := stmts^.first;
	while stmt <> nil do begin
	    CountStmt(stmt);
	    stmt := stmt^.next;
	end;
    end;
end;

procedure CountStmtAssign(stmt : StmtNode);
var
    done : boolean;
    lhson, rhson, teston : OptNode;
begin
    done := false;
    if stmt^.rhs^.kind <> EXPRBINOP then begin
	{ not a bin op }
    end else if not (stmt^.rhs^.exprBinOp in [TKPLUS,TKASTERISK,TKMINUS])
	or not (stmt^.rhs^.operType^.kind in [DTPOINTER,DTINTEGER,DTCARDINAL])
    then begin
	{ not a good bin op }
    end else if stmt^.rhs^.opnd1^.kind <> EXPRVAL then begin
	{ first operand not a variable }
    end else begin
	lhson := ExprToOpt(stmt^.lhs);
	rhson := ExprToOpt(stmt^.rhs);
	teston := ExprToOpt(stmt^.rhs^.opnd1^.exprVal);
	if rhson^.neededCount <> 1 then begin
	    { expression is reused }
	end else if lhson^.rootEqual = teston^.rootEqual then begin
	    { a := a op b }
	    stmt^.assignOp := stmt^.rhs^.exprBinOp;
	    { don't need to evaluate lhs }
	    ReduceExprNeededCounts(stmt^.lhs,1);
	    CountExpr(stmt^.rhs^.opnd1^.exprVal,EVALGET);
	    CountExpr(stmt^.rhs^.opnd2,EVALGET);
	    done := true;
	end;
    end;
    if not done then begin
	CountExpr(stmt^.rhs,EVALGET);
	CountExpr(stmt^.lhs,EVALPUT);
    end;
end;

procedure CountParamList(procType : TypeNode; procVariable : ExprNode;
	params : ExprList);
var
    pn : ParamNode;
    pen : ExprNode;
begin
    if procVariable <> nil then begin
	{ invocation of procedure variable, make it first parameter }
	CountExpr(procVariable,EVALGET);
    end;
    if (params = nil) or (procType^.paramList = nil) then begin
	{ do nothing }
    end else begin
	pen := params^.first;
	pn := procType^.paramList^.first;
	while pen <> nil do begin
	    case pn^.kind of
		PARAMVALUE : begin
		    if pn^.reference then begin
			CountExpr(pen,EVALPOINT);
		    end else begin
			CountExpr(pen,EVALGET);
		    end;
		end;
		PARAMVAR : begin
		    CountExpr(pen,EVALPOINT);
		end;
		PARAMARRAYVALUE, PARAMARRAYVAR : begin
		    CountExpr(pen,EVALGET);
		    if not pn^.paramType^.nocount then begin
			pen := pen^.next;
			CountExpr(pen,EVALGET);
		    end;
		end;
	    end;
	    pen := pen^.next;
	    pn := pn^.next;
	end;
    end;
end;

procedure CountFuncProc(procExpr : ExprNode; params : ExprList);
var
    proc : ProcNode;
    generated : boolean;
begin
    generated := false;
    if procExpr^.kind = EXPRSYM then begin
	if procExpr^.exprSym^.kind = SYMTYPE then begin
	    CountExpr(params^.first,EVALGET);
	    generated := true;
	end;
    end;
    if generated then begin
	{ do nothing }
    end else if procExpr^.kind <> EXPRCONST then begin
	CountParamList(procExpr^.exprType,procExpr,params);
    end else begin
	proc := procExpr^.exprConst^.procVal;
	if proc^.builtin <> BIPNOTBIP then begin
	    CountBuiltin(proc,params);
	end else begin
	    CountParamList(proc^.procType,nil,params);
	end;
    end;
end;

procedure CountStmtProc(stmt : StmtNode);
begin
    CountFuncProc(stmt^.proc,stmt^.params);
end;


procedure CountStmtIf(stmt : StmtNode);
begin
    CountExpr(stmt^.ifCond,EVALGET);
    CountStmtList(stmt^.thenList);
    CountStmtList(stmt^.elseList);
end;

procedure CountStmtCase(stmt : StmtNode);
var
    caseNode : CaseNode;
begin
    CountExpr(stmt^.caseSel,EVALGET);
    caseNode := stmt^.cases^.first;
    while caseNode <> nil do begin
	CountStmtList(caseNode^.stmts);
	caseNode := caseNode^.next;
    end;
    if stmt^.caseElse = nil then begin
    end else begin
	CountStmtList(stmt^.caseElse);
    end;
end;

procedure CountPrePostEval(el : ExprList;state : EvalState);
var
    en : ExprNode;
begin
    if el = nil then begin
	{ do nothing }
    end else begin
	en := el^.first;
	while en <> nil do begin
	    CommonSubExpr(en,EVALGET,state);
	    en := en^.next;
	end;
    end;
end;

procedure CountStmtWhile(stmt : StmtNode);
begin
    CountExpr(stmt^.whileCond,EVALGET);
    CountPrePostEval(stmt^.whilePreEval,EVALPRE);
    CountStmtList(stmt^.whileBody);
    CountPrePostEval(stmt^.whilePreEval,EVALPOST);
end;

procedure CountStmtRepeat(stmt : StmtNode);
begin
    CountPrePostEval(stmt^.repeatPreEval,EVALPRE);
    CountStmtList(stmt^.repeatBody);
    CountExpr(stmt^.repeatCond,EVALGET);
    CountPrePostEval(stmt^.repeatPreEval,EVALPOST);
end;

procedure CountStmtLoop(stmt : StmtNode);
begin
    CountPrePostEval(stmt^.loopPreEval,EVALPRE);
    CountStmtList(stmt^.loopBody);
    CountPrePostEval(stmt^.loopPreEval,EVALPOST);
end;

procedure CountStmtFor(stmt : StmtNode);
begin
    CountExpr(stmt^.forTo,EVALGET);
    CountVar(stmt^.forLimitVar,stmt^.forTo,1);
    CountExpr(stmt^.forFrom,EVALGET);
    CountVar(stmt^.forIndexVar,stmt^.forFrom,1);

    CountPrePostEval(stmt^.forPreEval,EVALPRE);
    CountStmtList(stmt^.forBody);
    CountVar(stmt^.forIndexVar,stmt^.forFrom,LOOPBIAS);
    CountVar(stmt^.forLimitVar,stmt^.forFrom,LOOPBIAS);
    CountPrePostEval(stmt^.forPreEval,EVALPOST);
end;

procedure CountStmtWith(stmt : StmtNode);
begin
    CountExpr(stmt^.withQual,EVALPOINT);
    CountVar(stmt^.withQualNode^.implQual,stmt^.withQual,1);
    CountStmtList(stmt^.withBody);
end;

procedure CountStmtReturn(stmt : StmtNode);
begin
    if stmt^.returnVal <> nil then begin
	CountExpr(stmt^.returnVal,EVALGET);
    end;
end;

procedure CountStmtExit(stmt : StmtNode);
begin
end;

procedure CountStmt{(stmt : StmtNode)};
begin
    case stmt^.kind of
	STMTASSIGN :	CountStmtAssign(stmt);
	STMTPROC :	CountStmtProc(stmt);
	STMTIF :	CountStmtIf(stmt);
	STMTCASE :	CountStmtCase(stmt);
	STMTWHILE :	CountStmtWhile(stmt);
	STMTREPEAT :	CountStmtRepeat(stmt);
	STMTLOOP :	CountStmtLoop(stmt);
	STMTFOR :	CountStmtFor(stmt);
	STMTWITH :	CountStmtWith(stmt);
	STMTRETURN :	CountStmtReturn(stmt);
	STMTEXIT :	CountStmtExit(stmt);
    end;
    UpdateTemps;
end;

procedure CountVar{(vn : VarNode; en : ExprNode; weight : integer)};
var
    on : OptNode;
    index : integer;
begin
    if vn^.address.kind = MEMFAST then begin
	index := trunc(vn^.address.offset) div WORDSIZE;
	on := ExprToOpt(en);
	refCount[index].count := refCount[index].count
		    + on^.loopNest * on^.referencedCount * weight;
    end;
end;

procedure CheckVarRef(vn : VarNode; mode : EvalMode);
var
    index : integer;
begin
    if (mode = EVALPOINT) and (vn^.address.kind = MEMFAST) then begin
	if TraceOptim then begin
	    write(output,'Addressed ');
	    WriteString(output,vn^.name);
	    writeln(output);
	end;
	index := trunc(vn^.address.offset) div WORDSIZE;
	refCount[index].addressed := true;
    end;
end;

procedure CountExprSym(expr : ExprNode; mode : EvalMode);
var
    sym : Symbol;
begin
    sym := expr^.exprSym;
    if sym^.kind = SYMVAR then begin
	ExprErrorName(expr,sym^.name,'CountExprSym: EXPRSYM is a SYMVAR?');
	CountVar(sym^.symVar,expr,1);
    end else begin
	ExprErrorName(expr,sym^.name,'CountExprSym: Symbol is not a variable?');
    end;
end;

procedure CountExprConst(expr : ExprNode; mode : EvalMode);
begin
end;

procedure CountExprUnOp(expr : ExprNode; mode : EvalMode);
begin
    CountExpr(expr^.opnd,EVALGET);
end;

procedure CountExprBinOp(expr : ExprNode);
begin
    CountExpr(expr^.opnd1,EVALGET);
    CountExpr(expr^.opnd2,EVALGET);
end;

procedure CountExprSubscr(expr : ExprNode; mode : EvalMode);
begin
    CountExpr(expr^.subsExpr,EVALGET);
    CountExpr(expr^.arr,EVALPOINT);
end;

procedure CountExprDot(expr : ExprNode; mode : EvalMode);
begin
    CountExpr(expr^.rec,EVALPOINT);
end;

procedure CountExprDeref(expr : ExprNode; mode : EvalMode);
begin
    CountExpr(expr^.ptr,EVALGET);
end;

procedure CountExprVal(expr : ExprNode; mode : EvalMode);
begin
    CountExpr(expr^.exprVal,EVALGET);
end;

procedure CountExprCheck(expr : ExprNode; mode : EvalMode);
begin
    CountExpr(expr^.checkExpr,mode);
end;

procedure CountExprFunc(expr : ExprNode);
begin
    CountFuncProc(expr^.func,expr^.params);
end;

procedure DoCountExpr{(expr : ExprNode; mode : EvalMode)};
begin
    case expr^.kind of
	EXPRBAD : ;
	EXPRVAR :	CheckVarRef(expr^.exprVar,mode);
	EXPRSYM :	CountExprSym(expr,EVALGET);
	EXPRCONST :	CountExprConst(expr,EVALGET);
	EXPRUNOP :	CountExprUnOp(expr,EVALGET);
	EXPRBINOP :	CountExprBinOp(expr);
	EXPRSUBSCR :	CountExprSubscr(expr,EVALGET);
	EXPRDOT :	CountExprDot(expr,EVALGET);
	EXPRDEREF :	CountExprDeref(expr,EVALGET);
	EXPRFUNC :	CountExprFunc(expr);
	EXPRVAL :	CountExprVal(expr,EVALGET);
	EXPRCHECK :	CountExprCheck(expr,EVALGET);
    end;
end;

procedure CountExpr{(expr : ExprNode; mode : EvalMode)};
begin
    if expr = nil then begin
	ExprError(expr,'CountExpr: nil expression');
    end else begin
	CommonSubExpr(expr,mode,EVALNORMAL);
    end;
end;

