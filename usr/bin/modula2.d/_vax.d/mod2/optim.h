(*#@(#)optim.h	4.1	Ultrix	7/17/90 *)
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
$Header: optim.h,v 1.5 84/05/19 11:43:00 powell Exp $
 ****************************************************************************)
const
    NULLTEMP = -1;
type
    TempNumber = integer;

const
    MAXOPTBLOCKLEVEL = 255;
    MAXTAILPARAMS = 10;		{ maximum parameters for tail recursion }
type
    OptBlockLevel = 0..MAXOPTBLOCKLEVEL;
    OptNode = ^OptNodeRec;
    OptUsage = (OUSEINDIVIDUAL, OUSEGENERATE, OUSEFIRST, OUSEMIDDLE, OUSELAST);
    OptNodeRec = record
	parent : ExprNode;
	nextActive, prevActive : OptNode;
	nextAll, prevAll : OptNode;
	nextClass, prevClass : OptNode;
	nextCongruent, prevCongruent, rootCongruent : OptNode;
	nextEqual, prevEqual, rootEqual : OptNode;
	marked, joinMark, purged, removed : boolean;
	createLevel, markLevel, joinMarkLevel : OptBlockLevel;
	uniqueId : integer;
	nonTrivial, eligible, address : boolean;
	containedNonTrivial : OptNode;
	tailRecursion, loopConsidered : boolean;
	expr : ExprNode;
	{ only meaningful in rootEqual }
	tempNumber : TempNumber;
	usage : OptUsage;
	loopNest : integer;
	neededCount, referencedCount, usedCount : 0..MAXINT;
	{ only accurate in rootCongruent }
	defineTime : OptTime;
    end;
    SaveOptLevel = record
	level, ceiling, floor : OptBlockLevel;
	blockTime : integer;
    end;
const
    LOOPBIAS = 10;	{ weight for references inside loops }
var
    optTime, blockOptTime : integer;
    optLoopNest : integer;
    cseRootExpr : array [ExprKind] of OptNode;
    cseRootUnOp : array [Token] of OptNode;
    cseRootBinOp : array [Token] of OptNode;

procedure Optimize; external;
procedure StartOptProc; external;
procedure EndOptProc; external;
procedure StartOptSplit(var sol : SaveOptLevel); external;
procedure NextOptSplit(var sol : SaveOptLevel); external;
procedure EndOptSplit(var sol : SaveOptLevel); external;
procedure StartOptLoop(var sol : SaveOptLevel); external;
procedure EndOptLoop(var sol : SaveOptLevel; var preEval : ExprList); external;
procedure OptRecursionProc(proc : ProcNode; stl : StmtList); external;
procedure OptRecursionReturn(proc : ProcNode; stn : StmtNode); external;
procedure MarkOptExpr(en : ExprNode); external;
procedure MarkOptAll; external;
procedure MarkType(tn : TypeNode); external;
function CongruentExpr(a, b : ExprNode) : boolean; external;
function Congruent(a, b : OptNode) : boolean; external;
function EnterExpr(en, pen : ExprNode; minTime : OptTime) : OptTime; external;
procedure EnterClass(var root : OptNode; on : OptNode); external;
procedure AddToActiveList(prevOne, newOne : OptNode); external;
procedure AddToAllList(prevOne, newOne : OptNode); external;
procedure RemoveFromActiveList(oldOne : OptNode); external;
procedure AddToClassList(var prevOne : OptNode; newOne : OptNode); external;
procedure EnterCongruent(root : OptNode; on : OptNode); external;
procedure AddToCongruentList(prevOne, newOne : OptNode); external;
procedure AddToEqualList(prevOne, newOne : OptNode); external;
procedure PrintOptExpr(on : OptNode); external;
procedure DumpOptEqual(root : OptNode); external;
procedure DumpOptCongruent(root : OptNode); external;
procedure DumpOptClass(root : OptNode); external;
procedure DumpOptExprs; external;
function ExprToOpt(en : ExprNode) : OptNode; external;
procedure AnalyzeForLoop(ien : ExprNode; stn : StmtNode; sol : SaveOptLevel);
	external;

var
    optimFlag : boolean;
    TraceMark, TraceActions : boolean;
    currOptProc : ProcNode;
    OptNloop, OptNreg, OptNind, OptNtail, OptNcall : boolean;
