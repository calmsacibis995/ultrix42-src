(*#@(#)builtin.h	4.1	Ultrix	7/17/90 *)
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
$Header: builtin.h,v 1.4 84/05/19 11:35:32 powell Exp $
 ****************************************************************************)
procedure InitStandardTypes; external;

procedure InitBuiltin; external;

function CheckBuiltin(isFunc : boolean; procExpr : ExprNode;
	var proc : ProcNode; params : ExprList; var retType : TypeNode)
	: boolean; external;

function OptBuiltin(procExpr : ExprNode; proc : ProcNode; params : ExprList)
	: OptTime; external;

procedure CountBuiltin(proc : ProcNode; params : ExprList); external;

const
    NUMUNIXPROCS = 71;
var
    unixProcNames : array [1..NUMUNIXPROCS] of ShortString;

