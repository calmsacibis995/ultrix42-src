(*#@(#)cexpr.h	4.1	Ultrix	7/17/90 *)
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
$Header: cexpr.h,v 1.5 84/06/06 12:52:02 powell Exp $
 ****************************************************************************)
function IsBadExpr(en : ExprNode) : boolean; external;

function IsAddressableExpr(en : ExprNode) : boolean; external;

function ConstType(cn : ConstNode) : TypeNode; external;

function ConstExpr(en : ExprNode; mode : EvalMode) : TypeNode; external;

function UnOpExpr(en : ExprNode; mode : EvalMode) : TypeNode; external;

procedure EvalConstBinOpExpr(en : ExprNode); external;

function VarExpr(en : ExprNode; mode : EvalMode) : TypeNode; external;

function DotExpr(en : ExprNode; mode : EvalMode) : TypeNode; external;

function DerefExpr(en : ExprNode; mode : EvalMode) : TypeNode; external;

function CheckFuncProc(isFunc : boolean; en, procExpr : ExprNode;
    var params : ExprList; var retType : TypeNode) : boolean; external;

function CheckExpr(en : ExprNode; mode : EvalMode) : TypeNode; external;

function CheckExprFunc(en : ExprNode; mode : EvalMode) : TypeNode; external;

function CheckExprType(en : ExprNode; mode : EvalMode) : TypeNode; external;

function Eval(en : ExprNode) : ConstNode; external;

function ExprSetToConstSet(esl : ExprSetList) : ConstSetList; external;

procedure ValueOrAddr(en : ExprNode; tn : TypeNode; mode : EvalMode);
	external;

procedure InsertCheckExpr(en : ExprNode; check : CheckKind; vn : VarNode;
	tn : TypeNode; lowerBound, upperBound : cardinal); external;

procedure RefOpenArray(en : ExprNode; tn : TypeNode); external;
