(*#@(#)const.h	4.1	Ultrix	7/17/90 *)
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
$Header: const.h,v 1.3 84/05/19 11:37:31 powell Exp $
 ****************************************************************************)
procedure WriteConstant(var f:text; con : ConstNode); external;

procedure CheckEqualConst(sym : Symbol; con : ConstNode); external;

function OrdOf(cn : ConstNode) : cardinal; external;

function CardinalConst(value : cardinal) : ConstNode; external;

function SymConst(names : IdentList) : ConstNode; external;
function SetConst(elementList : ConstSetList; setType : TypeNode) : ConstNode;
	external;
function UnOpConst(oper : Token; opnd : ConstNode) : ConstNode; external;
function BinOpConst(oper : Token; opnd1, opnd2 : ConstNode; eval : boolean)
	: ConstNode; external;
