(*#@(#)ees.h	4.1	Ultrix	7/17/90 *)
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
$Header: ees.h,v 1.4 84/05/19 11:32:37 powell Exp $
 ****************************************************************************)
const
    EESSTACKSIZE = 32;	{ maximum size of the ees }
    NUMTEMPS = -100;	{ minus the number of temps }
    APOFF = -4;		{ offset of current ap in activation record }
    DISPOFF = -8;	{ offset of old display reg in activation record }
type
    Reg = (NULLREG, SAVEDREG, SAVED2REG,
	    r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11,
	    ap, fp, sp, pc,
	    r20, r21, r22, r23, r24, r25, r26, r27, r28, r29,
	    r30, r31, r32, r33, r34, r35, r36, r37, r38, r39, r40,
	    rt0, rt1, rt2, rt3, rt4, rt5, rt6, rt7, rt8, rt9);
	    { note: tregs cannot have multi-word values (no longreals!) }

const
    RETURNREG = r0;          { return values from procedures in this reg }
    FIRSTREG = r1;
    LASTREG = r11;
    FPREG = fp;
    FIRSTVREG = r20;
    LASTVREG = r40;
type
    RegState = (REGALLOC, REGFREE, REGRETURN, REGREG, REGTEMP, REGDISP, REGEES);
    RegKind = REGRETURN..REGEES;

    EESKind = (EESDATA, EESADDR, EESVAR);
    EESElement = NUMTEMPS..EESSTACKSIZE;
    {
    EESDATA:	constInt, dreg
    EESADDR:	breg, sreg, addrLevel, addrMemType, addrOffset, addrBlock
    EESVAR:	addrLevel, addrMemType, addrOffset, addrBlock, indirect
    }
    EESRecord = record
	ptype : pcodetype;
	size : sizerange;
	kind : EESKind;
	inUse : boolean;
	constInt : integer;
	dreg : Reg;
	breg : Reg;
	sreg : Reg;
	sunits : sizerange;
	addrLevel : Level;
	addrMemType : char;
	addrOffset : integer;
	addrBlock : integer;
	indirect : boolean;
	smemoffset : sizerange;
	smemsize : sizerange;
    end;
var
    ees : array [NUMTEMPS..EESSTACKSIZE] of EESRecord;
    top : EESElement;
    maxtoffset : integer;
    lastreg : Reg;
    firsttreg : Reg;
{ table for printing registers }
    regString : array [Reg] of packed array [1..3] of char;
    regStringSize : array [Reg] of integer;

procedure Push(kind : EESKind); external;
procedure Pop(num:integer); external;
procedure InitReg; external;
procedure InitEES; external;
procedure DumpReg; external;
procedure DumpEES; external;
procedure ClearEES(e : EESElement); external;
procedure ClearReg; external;
procedure Registerize(var r : Reg); external;
function AllocReg(state : RegState; e : EESElement; ptype : pcodetype) : Reg;
	external;
procedure FreeReg(var r : Reg); external;
procedure NeedDisp(level : integer; mt : char); external;
procedure FreeDisp; external;
procedure ClearDisp; external;
function AllocDisplay(level : integer; mt : char) : Reg; external;
function DispReg(level : integer; mt : char) : Reg; external;
procedure ClearAddress(e : EESElement); external;
procedure SaveRegs(below : EESElement); external;
procedure RestoreRegs(below : EESElement); external;
procedure SwapEES(a,b : EESElement); external;
procedure MoveReg(e : EESElement; r : Reg); external;
function MemTReg(offset : integer) : Reg; external;
function TRegReg(r : Reg) : Reg; external;
function ActiveReg(r : Reg) : boolean; external;
