(*#@(#)otree.h	4.1	Ultrix	7/17/90 *)
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
$Header: otree.h,v 1.3 84/05/19 11:43:54 powell Exp $
 ****************************************************************************)
function OptUnOpExpr(en : ExprNode; mode : EvalMode) : OptTime; external;
function OptBinOpExpr(en : ExprNode; mode : EvalMode) : OptTime; external;
function OptSubscriptExpr(pen : ExprNode; mode : EvalMode) : OptTime;
	external;
function OptDotExpr(pen : ExprNode; mode : EvalMode) : OptTime; external;
function OptDerefExpr(pen : ExprNode; mode : EvalMode) : OptTime; external;
function OptFuncProc(procExpr : ExprNode; params : ExprList; parent : ExprNode)
	: OptTime; external;
function OptFuncExpr(en : ExprNode; mode : EvalMode) : OptTime; external;
function OptExpr(en : ExprNode; pen : ExprNode; mode : EvalMode) : OptTime;
	external;
procedure OptAssignStmt(stn : StmtNode); external;
procedure OptProcStmt(stn : StmtNode); external;
procedure OptIfStmt(stn : StmtNode); external;
procedure OptCase(stn : StmtNode; caseNode : CaseNode); external;
procedure OptCaseStmt(stn : StmtNode); external;
procedure OptWhileStmt(stn : StmtNode); external;
procedure OptRepeatStmt(stn : StmtNode); external;
procedure OptLoopStmt(stn : StmtNode); external;
procedure OptForStmt(stn : StmtNode); external;
procedure OptWithStmt(stn : StmtNode); external;
procedure OptReturnStmt(stn : StmtNode); external;
procedure OptExitStmt(stn : StmtNode); external;
procedure OptStmt(stn : StmtNode); external;
procedure OptStmtList(stl : StmtList); external;
procedure OptProc(pn : ProcNode); external;
procedure OptModule(mn : ModuleNode); external;
function Latest(a,b : OptTime) : OptTime; external;
