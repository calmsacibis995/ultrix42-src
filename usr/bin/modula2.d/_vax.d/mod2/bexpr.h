(*#@(#)bexpr.h	4.1	Ultrix	7/17/90 *)
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
$Header: bexpr.h,v 1.3 84/05/19 11:35:03 powell Exp $
 ****************************************************************************)
function NewExprNode(kind : ExprKind) : ExprNode; external;

procedure SameExprLine(newOne, oldOne : ExprNode); external;


function ConstExprNode(cn : ConstNode) : ExprNode; external;

function UnOpExprNode(oper : Token; opnd : ExprNode) : ExprNode; external;

function DotExprNode(rec : ExprNode; field : String) : ExprNode; external;

function DerefExprNode(ptr : ExprNode) : ExprNode; external;

procedure Indent(var f : text; indent:integer); external;

procedure PrintExpr(en:ExprNode;indent:integer); external;

procedure WriteExpr(var f : text; en : ExprNode); external;

function AddToExprList(list : ExprList; newOne : ExprNode) : ExprList;
	external;

