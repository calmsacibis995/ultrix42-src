(*#@(#)codesubs.h	4.1	Ultrix	7/17/90 *)
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
$Header: codesubs.h,v 1.3 84/05/19 11:32:16 powell Exp $
 ****************************************************************************)
function DoJumps : boolean; external;
procedure MultiWordBinOp(op : ShortString; left, right : EESElement); external;
procedure MultiWordUnOp(op : ShortString; opnd : EESElement); external;
procedure Compare(ptype : pcodetype; size : sizerange); external;
procedure CallProcOp(op : opcodes); external;
procedure CallProc(op : opcodes; ctype : char; size : sizerange;
	numParams : integer; var procName : operandstring); external;
procedure Increment(value : integer); external;
procedure SetConst(size : sizerange; e : EESElement); external;
procedure TwoOrThree(op:ShortString; source, dest : EESElement;
	ptype : pcodetype; size : sizerange); external;
procedure PushConst(ptype:pcodetype; size:sizerange;value:integer); external;
procedure PushReg(ptype:pcodetype; size:sizerange;dreg:Reg); external;
procedure MakeBaseAddress(e:EESElement); external;
procedure MakeVariable(e:EESElement); external;
