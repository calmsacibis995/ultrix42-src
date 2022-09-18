(*#@(#)memref.h	4.1	Ultrix	7/17/90 *)
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
$Header: memref.h,v 1.3 84/05/19 11:34:02 powell Exp $
 ****************************************************************************)
type
    StackMemNode = ^ StackMemNodeRec;
    StackMemNodeRec = record
	next, prev : StackMemNode;
	offset : integer;
	size : integer;
    end;
var
    stackMem : StackMemNode;
    stackMemSize : sizerange;

procedure DispRef(level:integer; mt : char); external;
procedure GenAddress(e : EESElement); external;
procedure Opnd(e : EESElement); external;
procedure CheckRegs(e:EESElement; size:sizerange); external;
procedure CheckSub(e:EESElement; size:sizerange); external;
procedure Eval(e:EESElement); external;
procedure Check(e:EESElement; size:sizerange); external;
procedure Point(e:EESElement); external;
procedure Store(v,a:EESElement); external;
procedure MakeMultiWordTemp(e:EESElement); external;
procedure PushMultiWordTemp(ptype:pcodetype;size:sizerange); external;
procedure ClearStack; external;
procedure FreeStack(offset, size : sizerange); external;
procedure CheckTReg(var r : Reg; e : EESElement); external;
procedure TRegOpnd(var r : Reg; e : EESElement); external;
function AddrIsTReg(e : EESElement) : boolean; external;
