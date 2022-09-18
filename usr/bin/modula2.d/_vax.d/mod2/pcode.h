(*#@(#)pcode.h	4.1	Ultrix	7/17/90 *)
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
$Header: pcode.h,v 1.3 84/05/19 11:44:12 powell Exp $
 ****************************************************************************)
#include "pcodeops.h"
type
    PcodeStr = array [1..3] of char;
var
    pcodeStr : array [PcodeInst] of PcodeStr;
    memTypeChar : array [MemoryType] of char;
    labelNumber : LabelNumber;
    codeFile : text;

procedure InitPcode; external;

procedure GenOp(op:PcodeInst); external;

function NewLabel : LabelNumber; external;

procedure GenLabel(l:LabelNumber); external;

procedure GenT(tn:TypeNode); external;

procedure GenMt(mt:MemoryType); external;

procedure GenInteger(v:cardinal); external;

procedure GenOpL(op:PcodeInst); external;

procedure GenOpT(op:PcodeInst;tn:TypeNode); external;

procedure GenOpTL(op:PcodeInst;tn:TypeNode); external;

procedure GenString(s:String); external;

procedure GenText(s:ShortString); external;

procedure GenChar(c:char); external;

procedure GenReal(r:real); external;

procedure GenSet(s:SetValue); external;

procedure Comma; external;

procedure EndLine; external;
