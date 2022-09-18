(*#@(#)ocount.h	4.1	Ultrix	7/17/90 *)
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
$Header: ocount.h,v 1.4 84/05/19 11:42:34 powell Exp $
 ****************************************************************************)
type
    EvalState = (EVALNORMAL, EVALPRE, EVALPOST);

procedure InitTemps; external;
procedure UpdateTemps; external;
function AllocTemp : TempNumber; external;
procedure FreeTemp(t : TempNumber); external;
procedure ReduceExprNeededCounts(en : ExprNode; count : integer); external;
procedure ReduceNeededCounts(on : OptNode; count : integer); external;
procedure CommonSubExpr(en : ExprNode; mode : EvalMode; state : EvalState); external;
procedure CountProc (proc : ProcNode); external;
procedure CountStmtList(stmts : StmtList); external;
procedure CountStmt(stmt : StmtNode); external;
procedure CountVar(vn : VarNode; en : ExprNode; weight : integer); external;
procedure DoCountExpr(expr : ExprNode; mode : EvalMode); external;
procedure CountExpr(expr : ExprNode; mode : EvalMode); external;
