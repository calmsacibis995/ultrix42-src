(*#@(#)genpc.h	4.1	Ultrix	7/17/90 *)
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
$Header: genpc.h,v 1.4 84/05/19 11:38:44 powell Exp $
 ****************************************************************************)
const
    MAINPROGNAME = 'main';

procedure GenProc(proc : ProcNode); external;

procedure GenProcName(proc : ProcNode); external;

procedure GenModule(module : ModuleNode); external;

procedure GenStmtList(stmts : StmtList); external;

procedure GenStmt(stmt : StmtNode); external;

procedure DoGenExpr(expr : ExprNode; mode : EvalMode); external;

procedure GenExpr(expr : ExprNode; mode : EvalMode); external;

procedure GenVar(vn : VarNode; mode : EvalMode); external;

procedure GenVarT(vn : VarNode; tn : TypeNode; mode : EvalMode); external;

procedure GenIndirectVar(varType : TypeNode; mode : EvalMode); external;

procedure GenConstInteger(i : cardinal); external;

procedure GenCall(internal : boolean; procName : String; procType : TypeNode;
	numParams : integer); external;

procedure GenAddress(address : Address); external;

procedure GenCondition(expr : ExprNode; trueLabel,falseLabel : LabelNumber);
	external;

procedure GenConstBoolean(b : boolean); external;

procedure OptGenExpr(en : ExprNode; mode : EvalMode; state : EvalState);
	external;

procedure GenStore(expr : ExprNode; tn : TypeNode); external;
