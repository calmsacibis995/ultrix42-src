(*#@(#)optim.p	4.1	Ultrix	7/17/90 *)
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
$Header: optim.p,v 1.7 84/06/06 13:04:29 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "const.h"
#include "bexpr.h"
#include "optim.h"
#include "otree.h"
#include "ocount.h"
#include "builtin.h"

const
    NUMOPTEXP = 10000;
var
    optBlockLevel, optBlockCeiling, optBlockFloor : OptBlockLevel;
    activeExprs : OptNode;	{ dummy root for all non-purged expressions }
    allExprs : OptNode;		{ dummy root for all expressions }
    generateUniqueId : integer;
    { the following table is the map from ExprNodes to OptNodes }
    { It is used ONLY to avoid recompilations during development }
    idToOpt : array [0..NUMOPTEXP] of OptNode;
    markItCount : integer;
    addressUnit : cardinal;

function ExprToOpt{(en : ExprNode) : OptNode};
begin
    if en = nil then begin
	ExprToOpt := nil;
    end else begin
	ExprToOpt := idToOpt[en^.opt];
    end;
end;

{ Set up new one expecting no matches }
function NewOptNode(expr, parent : ExprNode) : OptNode;
var
    newOne : OptNode;
begin
    new(newOne);
    newOne^.uniqueId := generateUniqueId;
    if TraceOptim then begin
	writeln('NewOptNode:',newOne^.uniqueId:1);
    end;
    expr^.opt := newOne^.uniqueId;	{ "backward pointer" }
    idToOpt[expr^.opt] := newOne;
    generateUniqueId := generateUniqueId + 1;
    newOne^.nextClass := newOne;
    newOne^.prevClass := newOne;
    newOne^.nextCongruent := newOne;
    newOne^.prevCongruent := newOne;
    newOne^.rootCongruent := newOne;
    newOne^.nextEqual := newOne;
    newOne^.prevEqual := newOne;
    newOne^.rootEqual := newOne;
    newOne^.createLevel := optBlockLevel;
    newOne^.markLevel := 0;
    newOne^.joinMarkLevel := 0;
    newOne^.marked := false;
    newOne^.joinMark := false;
    newOne^.purged := false;
    newOne^.removed := false;
    newOne^.expr := expr;
    newOne^.parent := parent;
    newOne^.tempNumber := NULLTEMP;
    newOne^.usage := OUSEINDIVIDUAL;
    newOne^.nonTrivial := false;
    newOne^.eligible := false;
    newOne^.address := false;
    newOne^.containedNonTrivial := nil;
    newOne^.tailRecursion := false;
    newOne^.loopConsidered := false;
    newOne^.neededCount := 1;
    newOne^.referencedCount := 1;
    newOne^.loopNest := optLoopNest;
    newOne^.usedCount := 0;
    newOne^.defineTime := 0;
    AddToAllList(allExprs^.prevAll,newOne);
    newOne^.nextActive := nil;
    newOne^.prevActive := nil;
    NewOptNode := newOne;
end;


procedure ResetOptimizer;
var
    ek : ExprKind;
    token : Token;
    i : integer;
begin
    for ek := EXPRBAD to EXPRCHECK do begin
	cseRootExpr[ek] := nil;
    end;
    for token := TKENDOFFILE to TKNULL do begin
	cseRootUnOp[token] := nil;
	cseRootBinOp[token] := nil;
    end;
    new(activeExprs);
    activeExprs^.nextActive := activeExprs;
    activeExprs^.prevActive := activeExprs;
    new(allExprs);
    allExprs^.nextAll := allExprs;
    allExprs^.prevAll := allExprs;
    optTime := 1;
end;

procedure InitOptimizer;
var
    i : integer;
begin
    case target of
	TARGETVAX : addressUnit := BYTESIZE;
	TARGETTITAN : addressUnit := WORDSIZE;
    end;
    for i := 0 to NUMOPTEXP do begin
	idToOpt[i] := nil;
    end;
    generateUniqueId := 1;
    optLoopNest := 1;
    ResetOptimizer;
end;

procedure OptJoin;
var
    ron, on : OptNode;
begin
    if TraceOpt then begin
	writeln(output,'Join ',optBlockLevel:1);
    end;
    on := activeExprs^.nextActive;
    markItCount := 0;
    while on <> activeExprs do begin
	if (on^.markLevel >= optBlockLevel) or
		(on^.joinMarkLevel >= optBlockLevel)
	then begin
	    on^.marked := true;
	    on^.markLevel := optBlockLevel;
	    on^.joinMark := false;
	    on^.joinMarkLevel := 0;
	    if TraceMark then begin
		write(output,'<+',on^.uniqueId:1,'>');
		markItCount := markItCount + 1;
		if markItCount > 20 then begin
		    writeln(output);
		    markItCount := 0;
		end;
	    end;
	end;
	on := on^.nextActive;
    end;
end;

procedure OptRefresh;
var
    on, nextActive : OptNode;
begin
    if TraceOpt then begin
	writeln(output,'Refresh ',optBlockLevel:1);
    end;
    markItCount := 0;
    on := activeExprs^.nextActive;
    while on <> activeExprs do begin
	nextActive := on^.nextActive;
	if on^.createLevel >= optBlockCeiling then begin
	    { purge expression }
	    on^.purged := true;
	    RemoveFromActiveList(on);
	    if TraceMark then begin
		write(output,'<X',on^.uniqueId:1,'>');
		markItCount := markItCount + 1;
		if markItCount > 20 then begin
		    writeln(output);
		    markItCount := 0;
		end;
	    end;
	end else if on^.markLevel >= optBlockCeiling then begin
	    on^.joinMark := on^.joinMark or on^.marked;
	    on^.joinMarkLevel := optBlockCeiling;
	    on^.marked := false;
	    on^.markLevel := 0;
	    if TraceMark then begin
		write(output,'<-',on^.uniqueId:1,'>');
		markItCount := markItCount + 1;
		if markItCount > 20 then begin
		    writeln(output);
		    markItCount := 0;
		end;
	    end;
	end;
	on := nextActive;
    end;
end;

{ CopyExpr: duplicate the expression tree (including associated OptNodes)}
{  Make the expressions be at the specified level }
function CopyExpr(en : ExprNode; sol : SaveOptLevel; parent : ExprNode)
	: ExprNode;
var
    on, eon, non, ron, search : OptNode;
    nen, pen, npen : ExprNode;
    npl : ExprList;
    found : boolean;
begin
    nen := NewExprNode(en^.kind);
    SameExprLine(nen,en);
    nen^.exprType := en^.exprType;
    nen^ := en^;	{ copy miscellaneous fields }
    case en^.kind of
	EXPRCONST :	nen^.exprConst := en^.exprConst;
	EXPRVAR :	nen^.exprVar := en^.exprVar;
	EXPRNAME,
	EXPRSYM,
	EXPRSET : ExprError(en,'CopyExpr: unexpected expr');
	EXPRUNOP : begin
	    nen^.opnd := CopyExpr(en^.opnd,sol,nen);
	    nen^.exprUnOp := en^.exprUnOp;
	end;
	EXPRBINOP : begin
	    nen^.opnd1 := CopyExpr(en^.opnd1,sol,nen);
	    nen^.opnd2 := CopyExpr(en^.opnd2,sol,nen);
	    nen^.exprBinOp := en^.exprBinOp;
	    nen^.operType := en^.operType;
	end;
	EXPRSUBSCR : begin
	    nen^.arr := CopyExpr(en^.arr,sol,nen);
	    nen^.subsExpr := CopyExpr(en^.subsExpr,sol,nen);
	    nen^.subsOffset := en^.subsOffset;
	end;
	EXPRDOT : begin
	    nen^.rec := CopyExpr(en^.rec,sol,nen);
	    nen^.fieldName := en^.fieldName;
	    nen^.field := en^.field;
	end;
	EXPRDEREF : begin
	    nen^.ptr := CopyExpr(en^.ptr,sol,nen);
	    nen^.realPtr := en^.realPtr;
	end;
	EXPRFUNC : begin
	    nen^.func := CopyExpr(en^.func,sol,nen);
	    npl := AddToExprList(nil,nil);
	    pen := en^.params^.first;
	    while pen <> nil do begin
		npen := CopyExpr(pen,sol,nen);
		npl := AddToExprList(npl,npen);
		pen := pen^.next;
	    end;
	    nen^.params := npl;
	end;
	EXPRVAL : begin
	    nen^.exprVal := CopyExpr(en^.exprVal,sol,nen);
	end;
	EXPRCHECK : begin
	    nen^.checkExpr := CopyExpr(en^.checkExpr,sol,nen);
	end;
    end;
    on := ExprToOpt(en);
    non := NewOptNode(nen,nil);
    { make it look as if it were created outside the loop }
    non^.createLevel := sol.level;
    non^.address := on^.address;
    non^.nonTrivial := on^.nonTrivial;
    non^.containedNonTrivial := on^.containedNonTrivial;
    non^.eligible := on^.eligible;
    non^.parent := parent;
    ron := on^.rootEqual;
    if TraceOptim then begin
	writeln(output,'CopyExpr: id=',on^.uniqueId:1,'/',
	    ron^.uniqueId:1,' create=',ron^.createLevel:1,
	    ' level=',sol.level:1);
    end;
    { search for an equal expression in the enclosing block }
    search := ron^.rootCongruent;
    found := false;
    repeat
	if (search^.rootEqual <> search) or search^.marked or search^.purged
		or (search^.createLevel > sol.ceiling)
		or (search^.createLevel < sol.floor)
	then begin
	    { not equal }
	    search := search^.nextCongruent;
	end else begin
	    { equal }
	    found := true;
	end;
    until found or (search = ron^.rootCongruent);
    if found then begin
	{ expression is already in outer block, make this another reference }
	AddToEqualList(search,non);
	if search <> ron then begin
	    { Consolidate this equal list with the outer one }
	    if TraceOptim then begin
		writeln(output,'CopyExpr: adding to list');
		PrintOptExpr(search);
		PrintOptExpr(ron);
		PrintOptExpr(non);
	    end;
	    { Add in counts from inner block expression }
	    search^.neededCount := search^.neededCount + ron^.neededCount;
	    search^.referencedCount := search^.referencedCount
					    + ron^.referencedCount;
	    { append the inner equal list to the outer one }
	    { fix up root pointer }
	    eon := ron;
	    repeat
		eon^.rootEqual := search;
		eon := eon^.nextEqual;
	    until eon = ron;
	    { append lists }
	    search^.prevEqual^.nextEqual := ron;
	    ron^.prevEqual^.nextEqual := search;
	    eon := search^.prevEqual;
	    search^.prevEqual := ron^.prevEqual;
	    ron^.prevEqual := eon;
	end;
    end else begin
	{ expression is needed once by pre-evaluation }
	non^.neededCount := ron^.neededCount + 1;
	{ consider pre-evaluation another reference }
	non^.referencedCount := ron^.referencedCount + 1;
	{ make this be the root equal expression }
	eon := ron;
	repeat
	    eon^.rootEqual := non;
	    eon := eon^.nextEqual;
	until eon = ron;
	{ insert new OptNode in front of rootEqual }
	non^.prevEqual := ron^.prevEqual;
	non^.nextEqual := ron;
	ron^.prevEqual^.nextEqual := non;
	ron^.prevEqual := non;
	AddToCongruentList(ron^.rootCongruent^.prevCongruent,non);
    end;
    CopyExpr := nen;
end;

procedure InductionExpr(indexen, patternen : ExprNode; var opnden : ExprNode;
    sol : SaveOptLevel; opnd1 : boolean);
var
    newen, olden, otheren : ExprNode;
    newon, opndon, otheron : OptNode;
    time : OptTime;
    constant : boolean;
    constVal : cardinal;
begin
    opndon := ExprToOpt(opnden);
    if opnd1 then begin
	constant := (patternen^.opnd2^.kind = EXPRCONST)
			and (opnden^.kind = EXPRCONST);
	if constant then begin
	    constVal := OrdOf(patternen^.opnd2^.exprConst);
	end;
    end else begin
	constant := (patternen^.opnd1^.kind = EXPRCONST)
			and (opnden^.kind = EXPRCONST);
	if constant then begin
	    constVal := OrdOf(patternen^.opnd1^.exprConst);
	end;
    end;
    if constant then begin
	case patternen^.exprBinOp of
	    TKPLUS : constVal := constVal + OrdOf(opnden^.exprConst);
	    TKMINUS : constVal := -constVal + OrdOf(opnden^.exprConst);
	    TKASTERISK : constVal := constVal * OrdOf(opnden^.exprConst);
	end;
	ReduceNeededCounts(opndon,1);
	opndon^.purged := true;
	RemoveFromActiveList(opndon);
	{ copy expression node so OptNodes will still be valid }
	olden := NewExprNode(EXPRCONST);
	olden^ := opnden^;
	opnden^.kind := EXPRCONST;
	opnden^.exprConst := CardinalConst(constVal);
	opnden^.exprType := patternen^.exprType;
	time := EnterExpr(opnden,nil,0);
	opndon^.expr := olden;
	opndon^.parent := nil;	{ expr is now detached }
    end else begin
	newen := NewExprNode(EXPRBINOP);
	SameExprLine(newen,opnden);
	newen^.exprBinOp := patternen^.exprBinOp;
	newen^.exprType := patternen^.exprType;
	newen^.operType := patternen^.operType;
	if opnd1 then begin
	    newen^.opnd2 := CopyExpr(patternen^.opnd2,sol,newen);
	    otheren := patternen^.opnd2;
	    newen^.opnd1 := opnden;
	end else begin
	    newen^.opnd1 := CopyExpr(patternen^.opnd1,sol,newen);
	    otheren := patternen^.opnd1;
	    newen^.opnd2 := opnden;
	end;
	otheron := ExprToOpt(otheren);
	opndon^.parent := newen;
	time := Latest(opndon^.rootCongruent^.defineTime,
					otheron^.rootCongruent^.defineTime);
	time := EnterExpr(newen,nil,time);
	newon := ExprToOpt(newen);
	newon^.createLevel := sol.level;
	opnden := newen;
    end;
end;

procedure AnalyzeForLoop{(ien : ExprNode; stn : StmtNode; sol : SaveOptLevel)};
var
    ion, on, opndon, pon, prooton, von, oon : OptNode;
    pen, en, olden, ven : ExprNode;
    possible : boolean;
    opnd1 : boolean;
    time : OptTime;
    itercount : integer;
begin
itercount := 0;
repeat
    if TraceOptim then begin
	if itercount > 0 then begin
	    writeln(output,'**** One More Time');
	end;
	writeln(output,'AnalyzeForLoop');
    end;
    itercount := itercount + 1;
    { index opt node }
    ion := ExprToOpt(ien);
    { parent rootEqual }
    prooton := nil;
    possible := true;
    on := ion;
    repeat
	if on^.parent = nil then begin
	    if TraceOptim then begin
		writeln(output,'nil parent');
		PrintOptExpr(on);
	    end;
	end else if on^.removed then begin
	    if TraceOptim then begin
		writeln(output,'removed');
		PrintOptExpr(on);
	    end;
	end else begin
	    { get assumed value expression }
	    von := ExprToOpt(on^.parent);
	    if von^.uniqueId < ion^.uniqueId then begin
		{ expression created before loop }
		if TraceOptim then begin
		    writeln(output,'irrelevant');
		    PrintOptExpr(von);
		end;
	    end else if (von^.expr^.kind <> EXPRVAL) or (von^.parent = nil)
	    then begin
		{ wrong kind of expression }
		possible := false;
	    end else begin
		{ get candidate induction expression }
		pon := ExprToOpt(von^.parent);
		if prooton <> nil then begin
		    if prooton <> pon^.rootCongruent then begin
			{ two different uses }
			possible := false;
		    end else begin
			{ another identical use }
		    end;
		end else if pon^.expr^.kind <> EXPRBINOP then begin
		    { candidates must be bin op }
		    possible := false;
		end else if not (pon^.expr^.exprBinOp in
			    [TKPLUS,TKASTERISK,TKMINUS])
		then begin
		    { only linear arithmetic operations are allowed }
		    possible := false;
		end else begin
		    opnd1 := pon^.expr^.opnd1 = von^.expr;
		    if opnd1 then begin
			oon := ExprToOpt(pon^.expr^.opnd2);
		    end else begin
			oon := ExprToOpt(pon^.expr^.opnd1);
		    end;
		    possible := not oon^.marked and
			(oon^.rootCongruent^.defineTime <= blockOptTime);
		    if possible then begin
			{ a winner! }
			prooton := pon^.rootCongruent;
		    end;
		end;
		if TraceOptim then begin
		    if possible then begin
			writeln(output,'potential ',pon^.rootCongruent^.defineTime:1,
				':',blockOptTime:1)
		    end else begin
			writeln(output,'ruining ',pon^.rootCongruent^.defineTime:1,
				':',blockOptTime:1)
		    end;
		    PrintOptExpr(pon);
		end;
	    end;
	end;
	on := on^.nextCongruent;
    until not possible or (on = ion);
    possible := possible and (prooton <> nil);
    if possible then begin
	opnd1 := false;
	if prooton^.expr^.opnd1^.kind = EXPRVAL then begin
	    if prooton^.expr^.opnd1^.exprVal^.kind = EXPRVAR then begin
		opnd1 := ien^.exprVar = prooton^.expr^.opnd1^.exprVal^.exprVar;
	    end;
	end;
	if not opnd1 then begin
	    { make sure opnd2 fits }
	    possible := false;
	    if prooton^.expr^.opnd2^.kind = EXPRVAL then begin
		if prooton^.expr^.opnd2^.exprVal^.kind = EXPRVAR then begin
		    possible := ien^.exprVar =
			    prooton^.expr^.opnd2^.exprVal^.exprVar;
		end;
	    end;
	    if not possible then begin
		ExprError(prooton^.expr,'AnalyzeForLoop: not opnd1 or opnd2?');
		PrintOptExpr(prooton);
	    end;
	end;
    end;
    if possible then begin
	if TraceOptim then begin
	    writeln(output,'AnalyzeForLoop: found induction expression');
	    PrintOptExpr(prooton);
	end;
	{ 
	    create a parent expression node
	    CopyExpr the non-index operand (of the binop) to outside the loop
	    add the forFrom expression as the other operand
	    EnterExpr the new parent expression
	    assign the new expr as the forFrom
	    repeat the above for the forTo expression
	    if the binop is *, repeat the above for the forBy expression

	    For each induction variable expression
		ReduceCounts on expression (will not be evaluated)
		Purge OptNode
		set expr and parent pointers to nil.
		    (must leave around because it might be rootCongruent)
		set expr^.kind to EXPRVAR, expr^.exprVar to indexVar
		EnterExpr new parent expression (parent pointer in old OptNode)
	}
	pen := prooton^.expr;
	{ change expression type of variable to match expression }
	ien^.exprType := pen^.exprType;
	InductionExpr(ien,pen,stn^.forFrom,sol,opnd1);
	InductionExpr(ien,pen,stn^.forTo,sol,opnd1);
	if pen^.exprBinOp = TKASTERISK then begin
	    InductionExpr(ien,pen,stn^.forBy,sol,opnd1);
	    { make index operation be expression type }
	    stn^.forIndexType := pen^.exprType;
	end;
	if TraceOptim then begin
	    write(output,'from ');
	    WriteExpr(output,stn^.forFrom);
	    write(output,' to ');
	    WriteExpr(output,stn^.forTo);
	    write(output,' by ');
	    WriteExpr(output,stn^.forBy);
	    writeln(output);
	end;
	on := prooton;
	repeat
	    if on^.uniqueId >= ion^.uniqueId then begin
		{ find expressions for this loop only }
		ReduceNeededCounts(on,1);
		on^.purged := true;
		RemoveFromActiveList(on);
		if opnd1 then begin
		    opndon := ExprToOpt(on^.expr^.opnd1);
		end else begin
		    opndon := ExprToOpt(on^.expr^.opnd2);
		end;
		opndon^.removed := true;
		en := on^.expr;	{ induction expression }
		{ copy expression node so OptNodes will still be valid }
		olden := NewExprNode(EXPRBINOP);
		olden^ := en^;
		{ replace en with value var index }
		en^.kind := EXPRVAL;
		ven := NewExprNode(EXPRVAR);
		SameExprLine(ven,en);
		en^.exprVal := ven;
		ven^.exprVar := ien^.exprVar;
		time := EnterExpr(ven,en,0);
		time := EnterExpr(en,on^.parent,0);
		on^.expr := olden;
		on^.parent := nil;	{ expr is now detached }
	    end;
	    on := on^.nextCongruent;
	until on = prooton;
    end;
until not possible;
end;

function ConsiderMove(on : OptNode; var preEval : ExprList; sol : SaveOptLevel)
	: boolean;
var
    pon : OptNode;
    didit : boolean;
    en, ien : ExprNode;
begin
    didit := false;
    if OptNloop or on^.loopConsidered then begin
	{ do nothing }
    end else begin
	if not on^.marked
	    and (on^.nonTrivial or (on^.containedNonTrivial <> nil))
	    and (on^.rootCongruent^.defineTime < blockOptTime)
	    and (on^.createLevel >= optBlockCeiling)
	then begin
	    { candidates for moving out are not marked, non-trivial, }
	    {  depend only on things outside the loop, }
	    {  and were calculated in this block }
	    { check parent first }
	    if on^.parent <> nil then begin
		pon := ExprToOpt(on^.parent);
		didit := ConsiderMove(pon,preEval,sol);
	    end;
	    if not didit then begin
		if TraceActions then begin
		    writeln(output,'Move out of loop ',blockOptTime:1);
		    PrintOptExpr(on);
		end;
		en := on^.expr;
		ien := CopyExpr(en,sol,nil);
		{ expression should be preserved through whole block }
		on^.rootEqual^.neededCount := on^.rootEqual^.neededCount + 1;
		on^.rootEqual^.eligible := true;
		preEval := AddToExprList(preEval,ien);
	    end;
	end;
	on^.loopConsidered := true;
    end;
    ConsiderMove := didit;
end;

procedure OptFinishLoop(var preEval : ExprList; sol : SaveOptLevel);
var
    on, prevActive : OptNode;
begin
    preEval := AddToExprList(nil,nil);
    on := activeExprs^.prevActive;
    { traverse list backwards to get outermost expressions first }
    while on <> activeExprs do begin
	prevActive := on^.prevActive;
	if on^.rootEqual <> on then begin
	    { not a rootEqual expression }
	    if TraceOpt then begin
		writeln(output,'OptFinishLoop: active but not rootEqual ',
		    on^.uniqueId:1);
	    end;
	end else if ConsiderMove(on,preEval,sol) then begin
	    { moved it out of loop }
	end else if on^.createLevel >= optBlockCeiling then begin
	    { purge expression }
	    on^.purged := true;
	    RemoveFromActiveList(on);
	end;
	on := prevActive;
    end;
end;

procedure Optimize;
begin
    InitOptimizer;
    OptModule(globalModule);
end;

procedure StartOptProc;
begin
    ResetOptimizer;
    optBlockLevel := 1;
    optBlockCeiling := 1;
    optBlockFloor := 1;
end;

procedure EndOptProc;
begin
end;

procedure StartOptSplit{(var sol : SaveOptLevel)};
begin
    sol.level := optBlockLevel;
    sol.floor := optBlockFloor;
    sol.ceiling := optBlockCeiling;
    sol.blockTime := blockOptTime;
    blockOptTime := optTime;
    optBlockLevel := optBlockLevel + 1;
    optBlockCeiling := optBlockLevel;
end;

procedure NextOptSplit{(var sol : SaveOptLevel)};
begin
    OptRefresh;
end;

procedure EndOptSplit{(var sol : SaveOptLevel)};
begin
    OptRefresh;
    optBlockLevel := sol.level;
    optBlockFloor := sol.floor;
    optBlockCeiling := sol.ceiling;
    blockOptTime := sol.blockTime;
    OptJoin;
end;

procedure StartOptLoop{(var sol : SaveOptLevel)};
begin
    sol.level := optBlockLevel;
    sol.floor := optBlockFloor;
    sol.ceiling := optBlockCeiling;
    sol.blockTime := blockOptTime;
    blockOptTime := optTime;
    optBlockLevel := optBlockLevel + 1;
    optBlockFloor := optBlockLevel;
    optBlockCeiling := optBlockLevel;
    optLoopNest := optLoopNest * LOOPBIAS;
end;

procedure EndOptLoop{(var sol : SaveOptLevel; var preEval : ExprList)};
begin
    optLoopNest := optLoopNest div LOOPBIAS;
    OptFinishLoop(preEval,sol);
    optBlockLevel := sol.level;
    optBlockFloor := sol.floor;
    optBlockCeiling := sol.ceiling;
    blockOptTime := sol.blockTime;
end;

procedure OptRecursionReturn{(proc : ProcNode; stn : StmtNode)};
var
    en : ExprNode;
    ok : boolean;
    pen : ExprNode;
    pn : ParamNode;
    on : OptNode;
    numParams : integer;
begin
    if OptNtail then begin
    end else begin
	en := stn^.returnVal;
	if en^.kind = EXPRFUNC then begin
	    if en^.func^.kind <> EXPRCONST then begin
		{ not a constant procedure name }
	    end else if en^.func^.exprConst^.kind <> DTPROC then begin
		{ not a procedure constant }
	    end else if proc <> en^.func^.exprConst^.procVal then begin
		{ not the right procedure }
	    end else begin
		if TraceOptim then begin
		    write(output,'OptRecursionReturn ');
		    WriteString(output,proc^.name);
		    writeln(output);
		end;
		ok := true;
		numParams := 0;
		if proc^.procType^.paramList <> nil then begin
		    pen := en^.params^.first;
		    pn := proc^.procType^.paramList^.first;
		    while ok and (pn <> nil) do begin
			if TraceOptim then begin
			    write(output,pn^.kind,' ');
			    WriteExpr(output,pen);
			end;
			if pn^.kind in [PARAMVAR,PARAMARRAYVAR] then begin
			    if pen^.baseVar = nil then begin
				ok := true;
				if TraceOptim then begin
				    write(output,' baseVar=nil');
				end;
			    end else if pen^.baseVar^.address.kind in
				    [MEMNORMAL,MEMFAST]
			    then begin
				ok := proc <> pen^.baseVar^.address.proc;
				if TraceOptim then begin
				    write(output,' block=',ok);
				end;
			    end else if pen^.baseVar^.address.kind = MEMPARAM
			    then begin
				ok := pen^.baseVar^.indirect or (proc <>
					pen^.baseVar^.address.proc);
				if TraceOptim then begin
				    write(output,' param=',ok);
				end;
			    end;
			end;
			if pn^.kind in [PARAMARRAYVALUE,PARAMARRAYVAR]
			then begin
			    if not pn^.paramType^.nocount then begin
				pen := pen^.next;
				numParams := numParams + 1;
			    end;
			end;
			if TraceOptim then begin
			    writeln(output);
			end;
			pen := pen^.next;
			numParams := numParams + 1;
			pn := pn^.next;
		    end;
		end;
		if ok and (numParams <= MAXTAILPARAMS) then begin
		    on := ExprToOpt(en^.func);
		    on^.tailRecursion := true;
		    proc^.tailRecursion := true;
		end;
	    end;
	end;
    end;
end;

procedure OptRecursionProc{(proc : ProcNode; stl : StmtList)};
var
    stn : StmtNode;
    ok : boolean;
    pen : ExprNode;
    pn : ParamNode;
    on : OptNode;
    cn : CaseNode;
begin
    if OptNtail or (stl^.first = nil) then begin
    end else begin
	stn := stl^.last;
	if stn^.kind = STMTPROC then begin
	    if stn^.proc^.kind <> EXPRCONST then begin
		{ not a constant procedure name }
	    end else if stn^.proc^.exprConst^.kind <> DTPROC then begin
		{ not a procedure constant }
	    end else if proc <> stn^.proc^.exprConst^.procVal then begin
		{ not the right procedure }
	    end else begin
		if TraceOptim then begin
		    write(output,'OptRecursionProc ');
		    WriteString(output,proc^.name);
		    writeln(output);
		end;
		ok := true;
		if proc^.procType^.paramList <> nil then begin
		    pen := stn^.params^.first;
		    pn := proc^.procType^.paramList^.first;
		    while ok and (pen <> nil) do begin
			if TraceOptim then begin
			    write(output,pn^.kind,' ');
			    WriteExpr(output,pen);
			end;
			if pn^.kind in [PARAMVAR,PARAMARRAYVAR] then begin
			    if pen^.baseVar = nil then begin
				ok := true;
				if TraceOptim then begin
				    write(output,' baseVar=nil');
				end;
			    end else if pen^.baseVar^.address.kind in
				    [MEMNORMAL,MEMFAST]
			    then begin
				ok := proc <> pen^.baseVar^.address.proc;
				if TraceOptim then begin
				    write(output,' block=',ok);
				end;
			    end else if pen^.baseVar^.address.kind = MEMPARAM
			    then begin
				ok := pen^.baseVar^.indirect or (proc <>
					pen^.baseVar^.address.proc);
				if TraceOptim then begin
				    write(output,' param=',ok);
				end;
			    end;
			end;
			if pn^.kind in [PARAMARRAYVALUE,PARAMARRAYVAR]
			then begin
			    if not pn^.paramType^.nocount then begin
				pen := pen^.next;
			    end;
			end;
			if TraceOptim then begin
			    writeln(output);
			end;
			pen := pen^.next;
			pn := pn^.next;
		    end;
		end;
		if ok then begin
		    on := ExprToOpt(stn^.proc);
		    on^.tailRecursion := true;
		    proc^.tailRecursion := true;
		end;
	    end;
	end else if stn^.kind = STMTIF then begin
	    OptRecursionProc(proc,stn^.thenList);
	    OptRecursionProc(proc,stn^.elseList);
	end else if stn^.kind = STMTWITH then begin
	    OptRecursionProc(proc,stn^.withBody);
	end else if stn^.kind = STMTCASE then begin
	    cn := stn^.cases^.first;
	    while cn <> nil do begin
		OptRecursionProc(proc,cn^.stmts);
		cn := cn^.next;
	    end;
	end;
    end;
end;

procedure MarkIt(on : OptNode);
begin
    on^.marked := true;
    if (on^.markLevel > optBlockLevel) or (on^.markLevel = 0) then begin
	on^.markLevel := optBlockLevel;
    end;
    on^.rootCongruent^.defineTime := optTime;
    if TraceMark then begin
	write(output,'<',on^.uniqueId:1,'>');
	if on <> on^.rootEqual then begin
	    write(output,'$');
	end;
	markItCount := markItCount + 1;
	if markItCount > 20 then begin
	    writeln(output);
	    markItCount := 0;
	end;
    end;
end;

procedure MarkOptAll;
var
    con, pon, mon : OptNode;
    pen, men : ExprNode;
    active, marked : integer;
begin
    if TraceOptim then begin
	writeln(output,'MarkOptAll: start');
    end;
    optTime := optTime + 1;
    markItCount := 0;
    active := 0;
    marked := 0;
    con := cseRootExpr[EXPRVAL];
    if con <> nil then begin
	repeat
	    mon := con;
	    repeat
		men := mon^.expr;
		{ Experiment:
		if men^.dependVar = nil then begin
		end else if men^.dependVar^.name <> nil then begin
		}
		    MarkIt(mon);
		    pen := mon^.parent;
		    while pen <> nil do begin
			pon := ExprToOpt(pen);
			if pon <> nil then begin
			    MarkIt(pon);
			    pen := pon^.parent;
			end else begin
			    pen := nil;
			end;
		    end;
		{ Experiment
		end;
		}
		mon := mon^.nextCongruent;
	    until mon = con;
	    con := con^.nextClass;
	until con = cseRootExpr[EXPRVAL];
    end;
    if TraceOptim then begin
	writeln(output,'MarkOptAll: done ',marked:1,':',active:1);
    end;
end;

procedure MarkOptExpr{(en : ExprNode)};
var
    baseVar : VarNode;
    basePtrType : TypeNode;
    con, mon, pon, von : OptNode;
    men, pen : ExprNode;
begin
    if TraceMark then begin
	write(output,'MarkOptExpr:');
	WriteExpr(output,en);
	writeln(output);
    end;
    markItCount := 0;
    optTime := optTime + 1;
    if en^.baseVar <> nil then begin
	baseVar := en^.baseVar;
	baseVar^.markTime.proc := currOptProc;
	baseVar^.markTime.time := optTime;
	if cseRootExpr[EXPRVAR] <> nil then begin
	     von := cseRootExpr[EXPRVAR];
	     repeat
		if von^.expr^.exprVar = baseVar then begin
		    von^.rootCongruent^.defineTime := optTime;
		end;
		von := von^.nextClass;
	     until (von = cseRootExpr[EXPRVAR]);
	end;
	con := cseRootExpr[EXPRVAL];
	if con <> nil then begin
	    repeat
		mon := con;
		repeat
		    men := mon^.expr;
		    if men^.dependVar = baseVar then begin
			MarkIt(mon);
			pen := mon^.parent;
			while pen <> nil do begin
			    pon := ExprToOpt(pen);
			    if pon <> nil then begin
				MarkIt(pon);
				pen := pon^.parent;
			    end else begin
				pen := nil;
			    end;
			end;
		    end;
		    mon := mon^.nextCongruent;
		until mon = con;
		con := con^.nextClass;
	    until con = cseRootExpr[EXPRVAL];
	end;
    end else if en^.basePtrType = nil then begin
	if TraceOptim then begin
	    write(output,'MarkOptExpr: both nil ');
	    WriteExpr(output,en);
	    writeln(output);
	end;
    end;
    if en^.basePtrType <> nil then begin
	if en^.baseVar <> nil then begin
	    if TraceOptim then begin
		write(output,'MarkOptExpr: both not nil ');
		WriteExpr(output,en);
		writeln(output);
	    end;
	end;
	basePtrType := en^.basePtrType;
	basePtrType^.markTime.proc := currOptProc;
	basePtrType^.markTime.time := optTime;
	mon := cseRootExpr[EXPRVAL];
	if mon <> nil then begin
	    repeat
		men := mon^.expr;
		if men^.dependPtrType = basePtrType then begin
		    MarkIt(mon);
		    pen := mon^.parent;
		    while pen <> nil do begin
			if pen^.basePtrType = basePtrType then begin
			    pon := ExprToOpt(pen);
			    if pon <> nil then begin
				MarkIt(pon);
				pen := pon^.parent;
			    end else begin
				pen := nil;
			    end;
			end else begin
			    pen := nil;
			end;
		    end;
		end;
		mon := mon^.nextCongruent;
	    until mon = cseRootExpr[EXPRVAL];
	end;
    end;
end;

function CongruentExpr{(a, b : ExprNode) : boolean};
begin
    CongruentExpr := Congruent(idToOpt[a^.opt],idToOpt[b^.opt]);
end;

function Congruent{(a, b : OptNode) : boolean};
var
    congruent : boolean;
    aen, ben : ExprNode;
begin
    aen := a^.expr;
    ben := b^.expr;
    if aen^.kind <> ben^.kind then begin
	congruent := false;
    end else if (a^.rootCongruent <> nil) and
	    (a^.rootCongruent = b^.rootCongruent)
    then begin
	congruent := true;
    end else begin
	congruent := false;
	case aen^.kind of
	    EXPRNAME,
	    EXPRSET : begin
		ExprError(aen,'Congruent: bad expr kind');
	    end;
	    EXPRSYM : begin
		congruent := aen^.exprSym = ben^.exprSym;
	    end;
	    EXPRCONST : begin
		if aen^.exprConst^.kind <> ben^.exprConst^.kind then begin
		    { not congruent }
		end else begin
		    case aen^.exprConst^.kind of
			DTCHAR : congruent :=
			    aen^.exprConst^.charVal = ben^.exprConst^.charVal;
			DTINTEGER,
			DTCARDINAL : congruent :=
			    aen^.exprConst^.cardVal = ben^.exprConst^.cardVal;
			DTBOOLEAN : congruent :=
			    aen^.exprConst^.boolVal = ben^.exprConst^.boolVal;
			DTREAL, DTLONGREAL : congruent :=
			    aen^.exprConst^.realVal = ben^.exprConst^.realVal;
			DTSET : congruent := (aen^.exprConst^.setVal^.setType =
						ben^.exprConst^.setVal^.setType)
					and (aen^.exprConst^.setVal^.value =
						ben^.exprConst^.setVal^.value);
			DTENUMERATION : congruent :=
			    (aen^.exprConst^.enumVal^.enumType =
					    ben^.exprConst^.enumVal^.enumType)
			    and (aen^.exprConst^.enumVal^.enumOrd =
					    ben^.exprConst^.enumVal^.enumOrd);
			DTSTRING : congruent :=
			    aen^.exprConst^.strVal = ben^.exprConst^.strVal;
			DTPROC : congruent :=
			    aen^.exprConst^.procVal = ben^.exprConst^.procVal;
			DTPOINTER : congruent := true; { nil pointers match }
		    end;
		end;
	    end;
	    EXPRVAR : congruent := aen^.exprVar = ben^.exprVar;
	    EXPRUNOP : begin
		if aen^.exprUnOp = ben^.exprUnOp then begin
		    congruent := CongruentExpr(aen^.opnd,ben^.opnd);
		end;
	    end;
	    EXPRBINOP : begin
		if aen^.exprBinOp = ben^.exprBinOp then begin
		    congruent := CongruentExpr(aen^.opnd1,ben^.opnd1);
		    if congruent then begin
			congruent := CongruentExpr(aen^.opnd2,ben^.opnd2);
		    end else if aen^.exprBinOp in [TKPLUS,TKASTERISK,TKEQUALS,
			TKSHARP,TKNOTEQUAL]
		    then begin
			{ check for reversed operands on commutative operator }
			congruent := CongruentExpr(aen^.opnd1,ben^.opnd2);
			if congruent then begin
			    congruent := CongruentExpr(aen^.opnd2,ben^.opnd1);
			end;
		    end;
		end;
	    end;
	    EXPRSUBSCR : begin
		congruent := CongruentExpr(aen^.arr,ben^.arr);
		if congruent then begin
		    congruent := CongruentExpr(aen^.subsExpr,ben^.subsExpr);
		end;
	    end;
	    EXPRDOT : begin
		congruent := aen^.field = ben^.field;
		if congruent then begin
		    congruent := CongruentExpr(aen^.rec,ben^.rec);
		end;
	    end;
	    EXPRDEREF : congruent := CongruentExpr(aen^.ptr,ben^.ptr);
	    EXPRFUNC : congruent := false;
	    EXPRVAL : begin
		congruent := aen^.exprType = ben^.exprType;
		if congruent then begin
		    congruent := CongruentExpr(aen^.exprVal,ben^.exprVal);
		end;
	    end;
	    EXPRCHECK : begin
		congruent := (aen^.exprCheck = ben^.exprCheck) and
			(aen^.checkVar = ben^.checkVar) and
			(aen^.checkType = ben^.checkType) and
			(aen^.checkLower = ben^.checkLower) and
			(aen^.checkUpper = ben^.checkUpper) and
			(aen^.checkField = ben^.checkField);
		if congruent then begin
		    congruent := CongruentExpr(aen^.checkExpr,ben^.checkExpr);
		end;
	    end;
	end;
    end;
    Congruent := congruent;
end;


function EnterExpr{(en : ExprNode; pen : ExprNode; minTime : OptTime): OptTime};
var
    on, ton : OptNode;
begin
    on := NewOptNode(en,pen);
    if en^.kind = EXPRBINOP then begin
	on^.nonTrivial := true;
	on^.containedNonTrivial := on;
	if en^.exprBinOp = TKPLUS then begin
	    case target of
		TARGETVAX : begin
		    if en^.opnd1^.kind in [EXPRCONST,EXPRVAR] then begin
			ton := ExprToOpt(en^.opnd2);
			on^.nonTrivial := false;
			on^.containedNonTrivial := ton^.containedNonTrivial;
		    end else if en^.opnd2^.kind in [EXPRCONST,EXPRVAR] then begin
			ton := ExprToOpt(en^.opnd1);
			on^.nonTrivial := false;
			on^.containedNonTrivial := ton^.containedNonTrivial;
		    end;
		end;
		TARGETTITAN : begin
		    if en^.opnd1^.kind in [EXPRCONST] then begin
			ton := ExprToOpt(en^.opnd2);
			on^.nonTrivial := false;
			on^.containedNonTrivial := ton^.containedNonTrivial;
		    end else if en^.opnd2^.kind in [EXPRCONST] then begin
			ton := ExprToOpt(en^.opnd1);
			on^.nonTrivial := false;
			on^.containedNonTrivial := ton^.containedNonTrivial;
		    end;
		end;
	    end;
	end else if en^.exprBinOp = TKASTERISK then begin
	    { don't count indexing by address unit size }
	    if en^.operType = addressTypeNode then begin
		if en^.opnd1^.kind = EXPRCONST then begin
		    if en^.opnd1^.exprConst^.kind = DTCARDINAL then begin
			if en^.opnd1^.exprConst^.cardVal = addressUnit
			then begin
			    ton := ExprToOpt(en^.opnd2);
			    on^.nonTrivial := false;
			    on^.containedNonTrivial := ton^.containedNonTrivial;
			end;
		    end;
		end else if en^.opnd2^.kind = EXPRCONST then begin
		    if en^.opnd2^.exprConst^.kind = DTCARDINAL then begin
			if en^.opnd2^.exprConst^.cardVal = addressUnit
			then begin
			    ton := ExprToOpt(en^.opnd1);
			    on^.nonTrivial := false;
			    on^.containedNonTrivial := ton^.containedNonTrivial;
			end;
		    end;
		end;
	    end;
	end;
	EnterClass(cseRootBinOp[en^.exprBinOp],on);
    end else if en^.kind = EXPRUNOP then begin
	on^.nonTrivial := true;
	on^.containedNonTrivial := on;
	EnterClass(cseRootUnOp[en^.exprUnOp],on);
    end else begin
	if en^.kind = EXPRVAR then begin
	    on^.nonTrivial := false;
	    on^.containedNonTrivial := nil;
	end else if en^.kind = EXPRVAL then begin
	    case target of
		TARGETVAX : begin
		    ton := ExprToOpt(en^.exprVal);
		    on^.nonTrivial := false;
		    on^.containedNonTrivial := ton^.containedNonTrivial;
		end;
		TARGETTITAN : begin
		    ton := ExprToOpt(en^.exprVal);
		    if ton^.containedNonTrivial <> nil then begin
			on^.nonTrivial := true;
			on^.containedNonTrivial := on;
		    end else begin
			on^.nonTrivial := false;
			on^.containedNonTrivial := ton^.containedNonTrivial;
		    end;
		end;
	    end;
	end else if en^.kind = EXPRCHECK then begin
	    on^.nonTrivial := true;
	    on^.containedNonTrivial := on;
	end;
	EnterClass(cseRootExpr[en^.kind],on);
    end;
(***
    if TraceOpt then begin
	writeln(output,'EnterExpr: min=',minTime:1,
	    ', def=', on^.rootCongruent^.defineTime:1);
	PrintOptExpr(on);
    end;
***)
    if minTime < 0 then begin
	minTime := -minTime;
	MarkIt(on^.rootEqual);
    end else if on^.rootCongruent^.defineTime < minTime then begin
	on^.rootCongruent^.defineTime := minTime;
    end;
    EnterExpr := on^.rootCongruent^.defineTime;
end;

{ EnterClass:
    Four possibilities:
	No other expr in that class => make sole element of class
	Does not match any element in class => AddToClassList
	Congruent to some element, but not equal => AddToCongruentList
	Equal to some element => AddToEqualList
}
procedure EnterClass{(var root : OptNode; on : OptNode)};
var
    found : boolean;
    search : OptNode;
begin
    if root = nil then begin
	{ make sole element of class }
	AddToClassList(root,on);
    end else begin
	search := root;
	repeat
	    if Congruent(on,search) then begin
		found := true;
	    end else begin
		search := search^.nextClass;
	    end;
	until found or (search = root);
	if not found then begin
	    { new class, add after end of list }
	    AddToClassList(root^.prevClass,on);
	end else begin
	    { found same class, look for congruent }
	    EnterCongruent(search,on);
	end;
    end;
end;

{ add newOne to doubly-linked All list, after prevOne }
procedure AddToAllList{(prevOne, newOne : OptNode)};
var
    nextOne : OptNode;
begin
    nextOne := prevOne^.nextAll;
    newOne^.nextAll := nextOne;
    newOne^.prevAll := prevOne;
    prevOne^.nextAll := newOne;
    nextOne^.prevAll := newOne;
end;

{ add newOne to doubly-linked Active list, after prevOne }
procedure AddToActiveList{(prevOne, newOne : OptNode)};
var
    nextOne : OptNode;
begin
    nextOne := prevOne^.nextActive;
    newOne^.nextActive := nextOne;
    newOne^.prevActive := prevOne;
    prevOne^.nextActive := newOne;
    nextOne^.prevActive := newOne;
end;

{ remove oldOne from doubly-linked Active list }
procedure RemoveFromActiveList{(oldOne : OptNode)};
var
    prevOne, nextOne : OptNode;
begin
    if oldOne^.nextActive <> nil then begin
	nextOne := oldOne^.nextActive;
	prevOne := oldOne^.prevActive;
	prevOne^.nextActive := nextOne;
	nextOne^.prevActive := prevOne;
	oldOne^.nextActive := nil;
	oldOne^.prevActive := nil;
    end;
end;

{ add newOne to doubly-linked Class list, after prevOne }
procedure AddToClassList{(var prevOne : OptNode; newOne : OptNode)};
var
    nextOne : OptNode;
begin
    if prevOne = nil then begin
	prevOne := newOne;
    end else begin
	nextOne := prevOne^.nextClass;
	newOne^.nextClass := nextOne;
	newOne^.prevClass := prevOne;
	prevOne^.nextClass := newOne;
	nextOne^.prevClass := newOne;
    end;
    AddToActiveList(activeExprs^.prevActive,newOne);
end;

procedure EnterCongruent{(root : OptNode; on : OptNode)};
var
    found : boolean;
    search : OptNode;
begin
    search := root;
    found := false;
    repeat
	if (search^.rootEqual <> search) or search^.marked or search^.purged
		or (search^.createLevel > optBlockCeiling)
		or (search^.createLevel < optBlockFloor)
	then begin
	    { not equal }
	    search := search^.nextCongruent;
	end else begin
	    { equal }
	    found := true;
	end;
    until found or (search = root);
    if found then begin
	{ equal }
	AddToEqualList(search,on);
    end else begin
	{ not equal }
	AddToCongruentList(root^.prevCongruent,on);
    end;
end;

{ add newOne to doubly-linked Congruent list, after prevOne }
procedure AddToCongruentList{(prevOne, newOne : OptNode)};
var
    nextOne : OptNode;
begin
    nextOne := prevOne^.nextCongruent;
    newOne^.nextCongruent := nextOne;
    newOne^.prevCongruent := prevOne;
    prevOne^.nextCongruent := newOne;
    nextOne^.prevCongruent := newOne;
    newOne^.rootCongruent := prevOne^.rootCongruent;
    AddToActiveList(activeExprs^.prevActive,newOne);
end;

{ add newOne to doubly-linked Equal list, after prevOne }
procedure AddToEqualList{(prevOne, newOne : OptNode)};
var
    nextOne : OptNode;
begin
    { add to equal list }
    nextOne := prevOne^.nextEqual;
    newOne^.nextEqual := nextOne;
    newOne^.prevEqual := prevOne;
    prevOne^.nextEqual := newOne;
    nextOne^.prevEqual := newOne;
    newOne^.rootEqual := prevOne^.rootEqual;

    { add to congruent list }
    nextOne := prevOne^.nextCongruent;
    newOne^.nextCongruent := nextOne;
    newOne^.prevCongruent := prevOne;
    prevOne^.nextCongruent := newOne;
    nextOne^.prevCongruent := newOne;
    newOne^.rootCongruent := prevOne^.rootCongruent;

    newOne^.rootEqual^.neededCount := newOne^.rootEqual^.neededCount + 1;
    newOne^.rootEqual^.referencedCount := newOne^.rootEqual^.referencedCount + 1;
end;

procedure PrintOptExpr{(on : OptNode)};
var
    en : ExprNode;
begin
    writeln(output,'Expr #',on^.uniqueId:1);
    if on^.parent = nil then begin
	write(output,'    Parent=nil');
    end else begin
	write(output,'    Parent=',on^.parent^.opt:1);
    end;
    writeln(output,', Congruent=',on^.rootCongruent^.uniqueId:1,
	    ', Equal=',on^.rootEqual^.uniqueId:1);
    writeln(output,'    marked=',on^.marked:1,', joinMark=',on^.joinMark:1,
	    ', purged=',on^.purged:1);
    writeln(output,'    createLevel=',on^.createLevel:1,', markLevel=',
	    on^.markLevel:1,', defineTime=',on^.defineTime:1,'/',
	    on^.rootCongruent^.defineTime:1,
	    ', loopNest=',on^.loopNest:1,', usage=',on^.usage:1);
    writeln(output,'    nonTrivial=',on^.nonTrivial:1,
	', eligible=',on^.eligible:1,
	', used=', on^.usedCount:1,', needed=', on^.neededCount:1,
	', referenced=', on^.referencedCount:1,
	', temp=',on^.tempNumber:1);
    if on^.containedNonTrivial <> nil then begin
	writeln(output,'    containedNonTrivial=',
			on^.containedNonTrivial^.uniqueId:1);
    end;
    en := on^.expr;
    if en = nil then begin
	writeln(output,'no expr node');
    end else begin
	write(output,'    line=',en^.lineNumber:1,', file=');
	    WriteString(output,en^.fileName);
	writeln(output);
	write(output,'    expr=');
	    WriteExpr(output,en);
	if en^.baseVar <> nil then begin
	    write(output,', baseVar =');
	    WriteString(output,en^.baseVar^.name);
	end;
	if en^.basePtrType <> nil then begin
	    write(output,', basePtrType =');
	    WriteString(output,en^.basePtrType^.name);
	end;
	writeln(output);
    end;
end;

procedure DumpOptEqual{(root : OptNode)};
var
    on : OptNode;
begin
    on := root;
    write(output,' ':11);
    repeat
	write(output,' ',on^.uniqueId);
	on := on^.nextEqual;
    until on = root;
    writeln(output);
end;

procedure DumpOptCongruent{(root : OptNode)};
var
    on : OptNode;
begin
    on := root;
    repeat
	writeln(output,' ':8,on^.uniqueId);
	DumpOptEqual(on);
	on := on^.nextCongruent;
    until on = root;
end;

procedure DumpOptClass{(root : OptNode)};
var
    on : OptNode;
begin
    on := root;
    repeat
	writeln(output,' ':4,on^.uniqueId);
	DumpOptCongruent(on);
	on := on^.nextClass;;
    until on = root;
end;

procedure DumpOptExprs;
var
    ek : ExprKind;
    token : Token;
    on : OptNode;
begin
(****
    for ek := EXPRBAD to EXPRSET do begin
	if cseRootExpr[ek] <> nil then begin
	    writeln(output,'Class Expr ',ek:1);
	    DumpOptClass(cseRootExpr[ek]);
	end;
    end;
    for token := TKENDOFFILE to TKNULL do begin
	if cseRootUnOp[token] <> nil then begin
	    writeln(output,'Class UnOp ',token:1);
	    DumpOptClass(cseRootUnOp[token]);
	end;
    end;
    for token := TKENDOFFILE to TKNULL do begin
	if cseRootBinOp[token] <> nil then begin
	    writeln(output,'Class BinOp ',token:1);
	    DumpOptClass(cseRootBinOp[token]);
	end;
    end;
    writeln(output);
****)
    writeln(output,'All exprs');
    on := allExprs^.nextAll;
    while on <> allExprs do begin
	PrintOptExpr(on);
	on := on^.nextAll;
    end;
end;

