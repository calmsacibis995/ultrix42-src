(*#@(#)opsubs.p	4.1	Ultrix	7/17/90 *)
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
$Header: opsubs.p,v 1.5 84/05/19 11:34:50 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "util.h"
#include "codesubs.h"
#include "opsubs.h"

procedure opabsc;
var lab : LabelNumber;
begin
    Eval(top);
    lab := NewLabel;
    if opd11 = 'r' then begin
	Op('tstf'); Opnd(top); L;
	Op('jgeq'); Lab(lab); L;
	Op('mnegf'); Opnd(top); X; Opnd(top); L;
    end else if opd11 = 'R' then begin
	Op('tstd'); Opnd(top); L;
	Op('jgeq'); Lab(lab); L;
	Op('mnegd'); Opnd(top); X; Opnd(top); L;
    end else begin
	Op('tstl'); Opnd(top); L;
	Op('jgeq'); Lab(lab); L;
	Op('mnegl'); Opnd(top); X; Opnd(top); L;
    end;
    Lab(lab); C(':'); L;
end;

procedure opaddc;
var
    r : Reg;
begin
    if opd11 = 'r' then begin
	TwoOrThree('addfN',top,top-1,treal,WORDSIZE);
    end else if opd11 = 'R' then begin
	TwoOrThree('adddN',top,top-1,tlongreal,2*WORDSIZE);
    end else if opd11 = 'a' then begin
	MakeBaseAddress(top);
	MakeBaseAddress(top-1);
	{ make sure subscripts are compatible }
	if ees[top].sreg <> NULLREG then begin
	    if ees[top-1].sreg <> NULLREG then begin
		if ees[top].sunits > ees[top-1].sunits then begin
		    CheckSub(top,ees[top-1].sunits);
		end else begin
		    CheckSub(top-1,ees[top].sunits);
		end;
	    end;
	end;
	ees[top-1].addrOffset := ees[top-1].addrOffset + ees[top].addrOffset;
	if ees[top].addrMemType <> ' ' then begin
	    if ees[top-1].addrMemType <> ' ' then begin
		Error('opaddc: add two base addresses');
	    end;
	    ees[top-1].addrMemType := ees[top].addrMemType;
	    ees[top-1].addrLevel := ees[top].addrLevel;
	    ees[top-1].addrBlock := ees[top].addrBlock;
	end;
	{ check for special case: no sreg, no constant }
	if (ees[top].sreg = NULLREG) and (ees[top-1].sreg = NULLREG) and
	    (ees[top-1].addrMemType = ' ') and (ees[top-1].addrOffset = 0)
	    and (ees[top].breg <> NULLREG) and (ees[top-1].breg <> NULLREG)
	then begin
	    { to use TwoOrThree, rearrange as dreg }
	    ees[top].kind := EESDATA;
	    ees[top].dreg := ees[top].breg;
	    ees[top].breg := NULLREG;
	    ees[top-1].kind := EESDATA;
	    ees[top-1].dreg := ees[top-1].breg;
	    ees[top-1].breg := NULLREG;
	    TwoOrThree('addlN',top,top-1,taddress,WORDSIZE);
	end else begin
	    if ees[top].breg <> NULLREG then begin
		if ees[top-1].breg <> NULLREG then begin
		    if not ActiveReg(ees[top-1].breg) and ActiveReg(ees[top].breg)
		    then begin
			{ switch them }
			MoveReg(top-1,ees[top].breg);
			MoveReg(top,ees[top-1].breg);
			r := ees[top].breg;
			ees[top].breg := ees[top-1].breg;
			ees[top-1].breg := r;
		    end;
		    Op('addl3'); R(ees[top].breg); X;
			    TRegOpnd(ees[top-1].breg,top-1); L;
		end else begin
		    ees[top-1].breg := ees[top].breg;
		    MoveReg(top-1,ees[top].breg);
		    ees[top].breg := NULLREG;
		end;
	    end;
	    if ees[top].sreg <> NULLREG then begin
		if ees[top-1].sreg <> NULLREG then begin
		    if not ActiveReg(ees[top-1].sreg) and ActiveReg(ees[top].sreg)
		    then begin
			{ switch them }
			MoveReg(top-1,ees[top].sreg);
			MoveReg(top,ees[top-1].sreg);
			r := ees[top].sreg;
			ees[top].sreg := ees[top-1].sreg;
			ees[top-1].sreg := r;
		    end;
		    Op('addl3'); R(ees[top].sreg); X;
			    TRegOpnd(ees[top-1].sreg,top-1); L;
		end else begin
		    ees[top-1].sreg := ees[top].sreg;
		    MoveReg(top-1,ees[top].sreg);
		    ees[top].sreg := NULLREG;
		    ees[top-1].sunits := ees[top].sunits;
		end;
	    end;
	    ees[top-1].ptype := taddress;
	    ees[top-1].size := WORDSIZE;
	end;
    end else if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG)
	and (ees[top-1].kind in [EESDATA,EESVAR])
    then begin
	{ top is constant }
	ees[top-1].constInt := Add(ees[top-1].constInt,ees[top].constInt);
    end else if (ees[top-1].kind = EESDATA) and (ees[top-1].dreg = NULLREG)
	and (ees[top].kind in [EESDATA,EESVAR])
    then begin
	{ top-1 is constant }
	SwapEES(top,top-1);	{ make top constant }
	ees[top-1].constInt := Add(ees[top-1].constInt,ees[top].constInt);
    end else begin
	TwoOrThree('addlN',top,top-1,tinteger,WORDSIZE);
    end;
    Pop(1);
end;

procedure opad2c;
var
    size : integer;
begin
    size := Int(opd^[2]);
    MakeVariable(top-1);
    Check(top-1,size);
    Check(top,size);
    ees[top-1].size := size;
    if (size <> WORDSIZE) and (size <> BYTESIZE) then begin
	Error('opad2: bad size');
    end;
    if opd11 = 'r' then begin
	ees[top-1].ptype := treal;
	Op('addf2'); Opnd(top); X; Opnd(top-1); L;
    end else if opd11 = 'R' then begin
	ees[top-1].ptype := tlongreal;
	Op('addd2'); Opnd(top); X; Opnd(top-1); L;
    end else begin
	if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) and
		(ees[top].constInt = 1)
	then begin
	    if size = BYTESIZE then begin
		Op('incb'); Opnd(top-1); L;
	    end else begin
		Op('incl'); Opnd(top-1); L;
	    end;
	end else begin
	    if size = BYTESIZE then begin
		Op('addb2');
	    end else begin
		Op('addl2');
	    end;
	    Opnd(top); X; Opnd(top-1); L;
	end;
    end;
    Pop(2);
end;

procedure opandc;
var r : Reg;
begin
    check2(2,[tboolean]);

    Check(top-1,BOOLSIZE);
    Check(top,BOOLSIZE);
    r := AllocReg(REGEES,top-1,tboolean);
    Op('xorb3'); Opnd(top-1); X; C('$'); I(1); X; R(r); L;
    Op('bicb3'); R(r); X; Opnd(top); X; R(r); L;
    Pop(2);
    Push(EESDATA);
    ees[top].dreg := r;
    ees[top].ptype := tboolean;
    ees[top].size := BOOLSIZE;
end;

procedure opbgnc;
var
    i : integer;
    r : Reg;
begin
    if (pclbsize=0) then Error('Missing program name');

    mainprogblockid := Int(opd^[2]);
    Op('.data'); L;
    Op('.comm'); S('_runtime__linenumber'); X; I(4); L;
    Op('.comm'); S('_runtime__filename'); X; I(4); L;
    Op('.comm'); S('_runtime__display'); X; I(4*(MAXDISPLEVEL+1)); L;
    for r := FIRSTVREG to LASTVREG do begin
	Op('.comm'); R(r); X; I(4); L;
    end;
    Op('.text'); L;
    Op('.align'); I(1); L;

    { use label as program name for separate compilation }
    for i := 1 to pclbsize do  progname[i]:=pclabel[i];
    prognmsize := pclbsize;
end;

procedure opcapc;
var
    lab : LabelNumber;
begin
    check1(1,[tchar]);

    Eval(top);
    lab := NewLabel;
    Op('cmpb'); Opnd(top); X; C('$'); I(ord('a')); L;
    Op('jlss'); Lab(lab); L;
    Op('cmpb'); Opnd(top); X; C('$'); I(ord('z')); L;
    Op('jgtr'); Lab(lab); L;
    Op('addb2'); C('$'); I(ord('A')-ord('a')); X; Opnd(top); L;
    Lab(lab); C(':'); L;
end;

procedure opcepc;
begin
	CallProcOp(opcep);
end;

procedure opcipc;
begin
	CallProcOp(opcip);
end;

procedure opcspc;
begin
    error(6);
end;

procedure opchkc;
    function GetValue(var i : integer) : integer;
    var
	result : integer;
    begin
	result := 0;
	while opd^[4][i] in ['0'..'9'] do begin
	    result := result * 10 + ord(opd^[4][i]) - ord('0');
	    i := i + 1;
	end;
	GetValue := result;
    end;
var
    labok, laberr, tmp : LabelNumber;
    min, max, value, offset, size, i : integer;
    ok, elsevariant, continue : boolean;
    op : ShortString;
begin
    ok := false;
    if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG)
	and (opd11 in ['r','s'])
    then begin
	min := Int(opd^[3]);
	max := Int(opd^[4]);
	value := ees[top].constInt;
	if max < min then begin
	    { cardinal range that spans maxint, shift value and range down }
	    if value < 0 then begin
		value := value + maxint + 1;
	    end else begin
		value := value - maxint - 1;
	    end;
	    ok := (max+maxint+1 >= value) and (min-maxint-1 <= value);
	end else begin
	    ok := (max >= value) and (min <= value);
	end;
    end;
    if ok then begin
    end else begin
	case opd11 of
	    'a' : begin
		labok := NewLabel;
		Eval(top);
		case opd^[2][1] of
		    'm' : begin
			Op('cmpl'); Opnd(top); X; I(-4); C('('); Opnd(top);
					C(')'); L;
			Op('jeql'); Lab(labok); L;
		    end;
		    'n' : begin
			Op('tstl'); Opnd(top); L;
			Op('jneq'); Lab(labok); L;
		    end;
		    'p' : begin
			laberr := NewLabel;
			Op('cmpl'); Opnd(top); X; S('__maxptr'); L;
			Op('jgtru'); Lab(laberr); L;
			Op('cmpl'); Opnd(top); X; S('__minptr'); L;
			Op('jgequ'); Lab(labok); L;
			Lab(laberr); C(':'); L;
		    end;
		end;
		Op('calls'); S('$0,_runtime__erroraddr'); L;
		Lab(labok); C(':'); L;
	    end;
	    'r','s' : begin
		labok := NewLabel;
		laberr := NewLabel;
		min := Int(opd^[3]);
		max := Int(opd^[4]);
		Eval(top);
		if min = 0 then begin
		    { treat as logical value }
		end else begin
		    Op('cmpl'); Opnd(top); X; C('$'); I(min); L;
		    { if (signed!) max < min, then it's a cardinal range }
		    {  that spans maxint.  Only case where signed fails. }
		    if max < min then begin
			Op('jlssu'); Lab(laberr); L;
		    end else begin
			Op('jlss'); Lab(laberr); L;
		    end;
		end;
		Op('cmpl'); Opnd(top); X; C('$'); I(max); L;
		if min >= 0 then begin
		    Op('jlequ'); Lab(labok); L;
		end else begin
		    Op('jleq'); Lab(labok); L;
		end;
		Lab(laberr); C(':');
		Op('pushl'); C('$'); I(max); L;
		Op('pushl'); C('$'); I(min); L;
		Op('pushl'); Opnd(top); L;
		if opd11 = 's' then begin
		    Op('calls'); S('$3,_runtime__errorsubscript'); L;
		end else begin
		    Op('calls'); S('$3,_runtime__errorrange'); L;
		end;
		Lab(labok); C(':'); L;
	    end;
	    'A' : begin
		{ put condition on top }
		SwapEES(top,top-1);
		labok := NewLabel;
		Check(top,BOOLSIZE);
		Op('bitb'); S('$1'); X; Opnd(top); L;
		Op('jneq'); Lab(labok); L;
		Pop(1);		{ get rid of condition }
		CallProc(opcep,'p',0,1,RTERRORASSERT);
		Lab(labok); C(':'); L;
	    end;
	    'p' : begin
		Op('calls'); S('$0,_runtime__errornoreturn'); L;
		{ insert a few no-ops so dbx will print the right address }
		Op('.byte'); I(1); X; I(1); L;
	    end;
	    'c' : begin
		Op('calls'); S('$0,_runtime__errorcase'); L;
	    end;
	    'o' : begin
		labok := NewLabel;
		Eval(top-1);
		Increment(WORDSIZE);
		MakeVariable(top);
		ees[top].ptype := tcardinal;
		ees[top].size := WORDSIZE;
		Check(top,WORDSIZE);
		Op('cmpl'); Opnd(top-1); X; Opnd(top); L;
		Op('jlssu'); Lab(labok); L;
		Op('pushl'); Opnd(top); L;
		Op('pushl'); C('$'); I(0); L;
		Op('pushl'); Opnd(top-1); L;
		Op('calls'); S('$3,_runtime__errorsubscript'); L;
		Lab(labok); C(':'); L;
		Pop(1);
	    end;
	    'v' : begin
		Point(top);
		offset := Int(opd^[2]) div BYTESIZE;
		size := Int(opd^[3]);
		if (size <> BYTESIZE) and (size <> WORDSIZE) then begin
		    Error('opchk v: bad tag size');
		end;
		if size <= BYTESIZE then begin
		    op := 'cmpb';
		end else begin
		    op := 'cmpl';
		end;
		if opd^[4][1] = '~' then begin
		    i := 2;
		    elsevariant := true;
		end else begin
		    i := 1;
		    elsevariant := false;
		end;
		labok := NewLabel;
		laberr := NewLabel;
		repeat
		    min := GetValue(i);
		    if opd^[4][i] <> ':' then begin
			Op(op); I(offset); Opnd(top); X; C('$'); I(min); L;
			Op('jeql'); Lab(labok); L;
		    end else begin
			i := i + 1;
			max := GetValue(i);
			Op(op); I(offset); Opnd(top); X; C('$'); I(min); L;
			Op('jlss'); Lab(laberr); L;
			Op(op); I(offset); Opnd(top); X; C('$'); I(max); L;
			Op('jleq'); Lab(labok); L;
			Lab(laberr); C(':'); L;
			laberr := NewLabel;
		    end;
		    if opd^[4][i] = ';' then begin
			i := i + 1;
			continue := i <= opdsizes^[4];
		    end else begin
			continue := false;
		    end;
		until not continue;
		if elsevariant then begin
		    { on else, switch labels }
		    tmp := labok;
		    labok := laberr;
		    laberr := tmp;
		    Op('jbr'); Lab(labok); L;
		end;
		Lab(laberr); C(':'); L;
		if size <= BYTESIZE then begin
		    Op('movb'); I(offset); Opnd(top); X; R(RETURNREG); L;
		    Op('pushl'); R(RETURNREG); L;
		end else begin
		    Op('pushl'); I(offset); Opnd(top); L;
		end;
		Op('calls'); S('$1,_runtime__errorvariant'); L;
		Lab(labok); C(':'); L;
		{ Point converted from address to variable; switch back }
		ees[top].kind := EESADDR;
	    end;
	end;
    end;
end;

procedure opchrc;
begin
    check1(0,[tinteger,tcardinal]);

    ees[top].ptype := tchar;
    ees[top].size := CHARSIZE;
end;

procedure opcomc;
var
    i : integer;
begin
    if opdcount <> 3 then begin
	error(1);
    end;
    comtable[numcomblocks].block := Int(opd^[1]);
    for i := 1 to opdsizes^[2] do begin
	comtable[numcomblocks].name[i] := opd^[2][i];
    end;
    comtable[numcomblocks].name[opdsizes^[2]+1] := ' ';
    Op('.comm'); C('_'); Comm(comtable[numcomblocks].name); X;
			    I((Int(opd^[3])*WORDSIZE) div BYTESIZE); L;
    if numcomblocks < NUMCOMBLOCKS then begin
	numcomblocks := numcomblocks + 1;
    end;
end;

procedure opctsc;
begin
end;

procedure opctrc;
begin
end;

procedure opcupc;
begin
	CallProcOp(opcup);
end;

procedure opdecc;
begin
    if opdcount<>2 then error(9);
    Increment(-Int(opd^[2]));
end;

procedure opdefc;
var nump, numt, numm : integer;
    rbit, i : integer;
    r : Reg;
    defblk : integer;
begin
	{ define offsets for activation record
	    offsets are negative from fp
	    first two words are for the old display and the current ap
	 *  tNNN = -8-numt*4		offset of generally temporaries
	 *  mNNN = tNNN-numm*4		offset of most local variables
	 *  pNNN = 4			skip count word at start of list
	 *  vNNN = save mask for registers used as T storage
	 The following have been moved to opexic so def can be earlier
	 *  sNNN = mNNN-stackMemSize*4	offset of pcode translator temporaries
	 *  aNNN = -sNNN		total activation record size
	 }
	if opdcount<>5 then error(9);
	nump := Int(opd^[1]);
	numt := Int(opd^[2]);
	numm := Int(opd^[3]);
	defblk := Int(opd^[4]);
	if defblk <> curblockid then begin
	    Error('Unexpected block on def');
	end;
	tmemoff := -8 - numt*4;
	mmemoff := tmemoff - numm*4;
	pmemoff := 4;
	{ discourage use of these before they are defined }
	smemoff := -maxint-1;
	arsize := -maxint-1;
	regmask := 0;
	rbit := 1;
	for r := r1 to firsttreg do begin
	    rbit := rbit * 2;
	end;
	for i := 1 to maxtoffset div WORDSIZE do begin
	    if i <= numt then begin
		regmask := regmask + rbit;
	    end;
	    rbit := rbit * 2;
	end;
	Op('.set'); C('t'); I(defblk); X; I(tmemoff); L;
	Op('.set'); C('m'); I(defblk); X; I(mmemoff); L;
	Op('.set'); C('p'); I(defblk); X; I(pmemoff); L;
end;

procedure opexic;
begin
	{ define offsets for activation record
	    see opdefc for explanation
	}
	smemoff := mmemoff - 4*stackMemSize;
	arsize := -smemoff;
	Op('.set'); C('s'); I(curblockid); X; I(smemoff); L;
	Op('.set'); C('a'); I(curblockid); X; I(arsize); L;
	Op('.set'); C('v'); I(curblockid); X; I(regmask); L;
end;

procedure opdifc;
var
    wordsize : sizerange;
begin
    check2(2,[tset]);

    if ees[top].size <= WORDSIZE then begin
	TwoOrThree('biclN',top,top-1,tset,WORDSIZE);
    end else begin
	wordsize := (ees[top].size + WORDSIZE-1) div WORDSIZE;
	Eval(top-1);
	Push(EESDATA);
	ees[top].ptype := tinteger;
	ees[top].size := WORDSIZE;
	ees[top].constInt := wordsize;
	MultiWordBinOp('bicl2',top-1,top-2);
    end;
    Pop(1);
end;

procedure opdivc;
var
    k : integer;
begin
    if opd11 = 'r' then begin
	TwoOrThree('divfN',top,top-1,treal,WORDSIZE);
	Pop(1);
    end else if opd11 = 'R' then begin
	TwoOrThree('divdN',top,top-1,tlongreal,2*WORDSIZE);
	Pop(1);
    end else if opd11 = 'j' then begin
	if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) then begin
	    { divide by constant }
	    if ees[top].constInt = 1 then begin
		{ do nothing }
		Pop(1);
	    end else begin
		k := PowerOfTwo(ees[top].constInt);
		if k > 0 then begin
		    Eval(top-1);
		    { first clear low order bits that will be rotated around }
		    { (because value is a power of 2, value-1 is correct mask) }
		    Op('bicl2'); C('$'); I(ees[top].constInt-1); X;
				    Opnd(top-1); L;
		    Op('rotl'); C('$'); I(-k); X; Opnd(top-1); X;
			Opnd(top-1); L;
		    Pop(1);
		end else begin
		    CallProc(opcep,'i',WORDSIZE,2,RTLONGDIV);
		end;
	    end;
	end else begin
	    CallProc(opcep,'i',WORDSIZE,2,RTLONGDIV);
	end;
    end else begin
	TwoOrThree('divlN',top,top-1,tinteger,WORDSIZE);
	Pop(1);
    end;
end;

procedure opdv2c;
begin
    error(6);
end;

procedure opdspc;
var size : integer;
begin
    if (ees[top].ptype<>tinteger) then error(2);
    if (opdcount<>1) then error(9);
    if ees[top-1].ptype<>taddress then error(4);

    size := (Int(opd^[1]) + WORDSIZE-1) div WORDSIZE;
    if (size <> WORDSIZE) and (size <> BYTESIZE) then begin
	Error('opmp2: bad size');
    end;
    if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) then begin
	{ constant size }
	ees[top].constInt := ees[top].constInt * size;
    end else begin
	{ compute size }
	Eval(top);
	Op('mull2'); C('$'); I(size); X; Opnd(top); L;
    end;
    CallProc(opcep,'p',0,2,RTDISPOSE);
end;

{
    Note: there are two kinds of linkage -- external (CALLS) and internal (JSB)
    The following shows what a stack frame looks like on internal call
    Attempts are made to make the stack frame look like CALLS frame for
	debugging purposes.
    
		Param 3  parameters		^
		Param 2				| higher addresses are up    |
		Param 1					    stack grows down V
	ap ->	Return address	(occupies parameter count position)
		Saved old fp
		Saved old ap
		Unused (occupies CALLS save mask position)
	fp ->	Unused (occupies CALLS condition handler pointer position)
		Saved new ap (for use in display)
		Saved old display register 
		T Memory	\
		M Memory	|- activation record
		 etc.		/
	sp1 ->	Last word of activation record
		Saved regs r11-r6 if necessary
	sp2 ->	Last saved reg
		Dynamic area for large value parameters
	sp3 ->	Last word of dynamic area

    Note:  sp will be left pointing at either sp1, sp2, or sp3 depending on
	whether or not there are any saved regs or dynamic area.
    
    For completeness, here is how CALLS leaves the stack (notice the family
	resemblance with the above)
		Param 3  parameters		^
		Param 2				| higher addresses are up    |
		Param 1					    stack grows down V
	ap ->	Number of parameter words
		Saved regs r11-r6 if necessary
		Return address
		Saved old fp
		Saved old ap
		Saved register bits and interrupt mask
	fp ->	Condition handler pointer
		Saved new ap (for use in display)
		Saved old display register 
		T Memory	\
		M Memory	|- activation record
		 etc.		/
	sp1 ->	Last word of activation record
		Dynamic area for large value parameters
	sp3 ->	Last word of dynamic area

    With CALLS, there won't be any registers saved down here.
}
procedure opentc;
const
    STABLINE = 68;
var i, j, parmcnt, linenum, lab : integer;
    r : Reg;
    gprof : boolean;
begin
    if (opdcount<6) then error(9);
    if (pclbsize=0) then error(19);

    curlev := Int(opd^[3]);
    curblockid := Int(opd^[4]);
    parmcnt := Int(opd^[5]);
    linenum := Int(opd^[6]);
    if opdcount >= 7 then begin
	nodisplay := opd^[7][1] = '1';
	internal := opd^[7][2] = '1';
	gprof := opd^[7][3] = '1';
    end else begin
	nodisplay := false;
	internal := false;
	gprof := false;
    end;
    maxtoffset := -1;
    lastreg := LASTREG;
    firsttreg := NULLREG;
    if opdcount >= 8 then begin
	i := Int(opd^[8]);
	if i > 6 then begin
	    i := 6;
	end;
	if i > 0 then begin
	    maxtoffset := i * WORDSIZE;
	    lastreg := LASTREG;
	    for j := 1 to i do begin
		lastreg := pred(lastreg);
	    end;
	    firsttreg := succ(lastreg);
	end;
    end;
    if (curlev>MAXDISPLEVEL) then Error('opent: display level to large');
    ClearStack;
    if PRINTNAMES then begin
	Op('.data'); I(1); L;
	S('name'); I(curblockid); C(':'); Op('.asciz');
		C('"'); SO(pclabel); C('"'); L;
	Op('.text'); L;
    end;
    Op('.align'); I(2); L;
    Op('.globl'); C('_'); SO(pclabel); L;
    C('_'); SO(pclabel); C(':'); L;
    if not internal then begin
	Op('.word'); C('v'); I(curblockid); L;
    end else begin
	{ internal entry sequence }
	Op('movl'); R(fp); X; S('-4(sp)'); L;
	Op('movl'); R(ap); X; S('-8(sp)'); L;
	Op('movl'); R(sp); X; R(ap); L;
	Op('movab'); S('-16(sp)'); X; R(fp); L;
    end;
    if linenum <> 0 then begin
	Op('.stabd'); I(STABLINE); X; I(0); X; I(linenum); L;
    end;
    if (curblockid = mainprogblockid) then begin
	    { Push argv and argc for modelinit (to pass to readcore }
	    Op('pushl'); S('12(ap)'); L;
	    Op('pushl'); S('8(ap)'); L;
	    Op('pushl'); S('4(ap)'); L;
	    Op('calls'); S('$3,_runtime__init'); L;
    end;
    if not nodisplay then begin
	{ save ap and old display }
	Op('movl'); R(ap); X; I(APOFF); S('(fp)'); L;
	Op('movl'); S('_runtime__display+'); I(curlev*4); X;
			I(DISPOFF); S('(fp)'); L;
	{ set up new display }
	Op('movl'); R(fp); X; S('_runtime__display+'); I(curlev*4); L;
    end;
    { allocate activation record }
    Op('subl3'); S('$a'); I(curblockid); X; R(fp); X; R(sp); L;
    if internal then begin
	if firsttreg <> NULLREG then begin
	    for r := firsttreg to LASTREG do begin
		Op('pushl'); R(r); L;
	    end;
	end;
    end;
    if PRINTNAMES then begin
	Op('pushal'); S('name'); I(curblockid); L;
	Op('calls'); S('$1'); X; S('_runtime__trace'); L;
    end;
    if gprof then begin
	lab := NewLabel;
	Op('movab'); Lab(lab); X; R(r0); L;
	Op('jsb'); S('mcount'); L;
	Op('.data'); L;
	Op('.align'); I(2); L;
	Lab(lab); C(':'); Op('.long'); I(0); L;
	Op('.text'); L;
    end;
    ClearReg;
end;

procedure opequc;
begin
    Compare(associatedType[opd11],Int(opd^[2]));
    nextjump := jceq;       { set up to do eq jump }
end;

procedure opfjpc;
begin
    if (ees[top].ptype<>tboolean) then error(2);
    if (opdcount<>1) then error(9);

    Check(top,BOOLSIZE);
    Op('bitb'); S('$1'); X; Opnd(top); L;
    Op('jeql'); writelabel(1); L;
    Pop(1);
end;

procedure opfltc;
var
    r : Reg;
    op : ShortString;
begin
    if not (ees[top].ptype in [tinteger, tcardinal, treal, tlongreal])
	then error(2);
    Check(top,WORDSIZE);
    if opd11 = 'r' then begin
	if ees[top].ptype in [tinteger,tcardinal] then begin
	    op := 'cvtlf';
	end else if ees[top].ptype in [treal,tlongreal] then begin
	    op := 'movl';	{ real - no op, long real - ignore second word }
	end else begin
	    Error('opfltc: unexpected type');
	end;
	if (ees[top].kind = EESDATA) and (ees[top].dreg <> NULLREG) then begin
	    Op(op); Opnd(top); X; Opnd(top); L;
	end else begin
	    r := AllocReg(REGEES,top,treal);
	    Op(op); Opnd(top); X; R(r); L;
	    ees[top].kind := EESDATA;
	    ClearAddress(top);
	    FreeReg(ees[top].dreg);
	    ees[top].dreg := r;
	end;
	ees[top].ptype := treal;
	ees[top].size := WORDSIZE;
    end else if opd11 = 'R' then begin
	if ees[top].ptype in [tinteger,tcardinal] then begin
	    op := 'cvtld';
	end else if ees[top].ptype = treal then begin
	    op := 'cvtfd';
	end else begin
	    op := 'movd';	{ longreal - no op }
	end;
	r := AllocReg(REGEES,top,tlongreal);
	Op(op); Opnd(top); X; R(r); L;
	ees[top].kind := EESDATA;
	ClearAddress(top);
	ees[top].ptype := tlongreal;
	ees[top].size := 2*WORDSIZE;
	FreeReg(ees[top].dreg);
	ees[top].dreg := r;
    end else begin
	Error('opfltc: not r or R');
    end;
end;

procedure opgeqc;
begin
    Compare(associatedType[opd11],Int(opd^[2]));
    if opd11 in ['c','j'] then begin
	nextjump := jcgeu;	{ set up to do geu jump }
    end else begin
	nextjump := jcge;	{ set up to do ge jump }
    end;
end;

procedure opgrtc;
begin
    Compare(associatedType[opd11],Int(opd^[2]));
    if opd11 in ['c','j'] then begin
	nextjump := jcgtu;	{ set up to do gtu jump }
    end else begin
	nextjump := jcgt;	{ set up to do gt jump }
    end;
end;

procedure opincc;
begin
	if opdcount<>2 then error(9);
	Increment(Int(opd^[2]));
end;

procedure opindc;
begin
    MakeVariable(top);
    ees[top].ptype := associatedType[opd11];
    ees[top].size := Int(opd^[2]);
end;

procedure opinnc;
var
    r : Reg;
    lab : LabelNumber;
    i, mask : integer;
begin
    if (ees[top].ptype<>tset) then error(2);
    if not (ees[top-1].ptype in [tinteger,tcardinal,tchar]) then begin
	error(4);
    end;

    if (ees[top-1].kind = EESDATA) and (ees[top-1].dreg = NULLREG)
	    and (ees[top].size <= WORDSIZE)
    then begin
	{ element is constant, set is word }
	{ subtract set from set containing only element }
	{ result is 0 if set contains element }
	mask := 1;
	for i := 1 to ees[top-1].constInt do begin
	    mask := Mult(mask,2);
	end;
	r := AllocReg(REGTEMP,0,tset);
	if ees[top].size = BYTESIZE then begin
	    Check(top,BYTESIZE);
	    Op('bicb3'); Opnd(top); X; C('$'); I(mask); X; R(r); L;
	end else begin
	    Check(top,WORDSIZE);
	    Op('bicl3'); Opnd(top); X; C('$'); I(mask); X; R(r); L;
	end;
	FreeReg(r);
	nextjump := jceq;       { set up to do eq jump }
	Pop(2);
    end else begin
	Check(top-1,WORDSIZE);
	lab := NewLabel;
	r := AllocReg(REGEES,top-1,tboolean);
	if (ees[top-1].kind = EESDATA) and (ees[top-1].dreg = NULLREG)
		and (ees[top-1].constInt < ees[top].size)
	then begin
	    { no test necessary }
	end else begin
	    Op('clrl'); R(r); L;
	    Op('cmpl'); Opnd(top-1); X; C('$'); I(ees[top].size); L;
	    Op('jgequ'); Lab(lab); L;
	end;
	if ees[top].size > WORDSIZE then begin
	    Point(top); { instruction requires address }
	end else if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG)
	then begin
	    Eval(top);	{ constant doesn't work in this instruction }
	end else begin
	    Check(top,WORDSIZE);
	    CheckRegs(top,BYTESIZE);	{ instruction requires byte index }
	end;
	Op('extzv'); Opnd(top-1); X; S('$1'); X; Opnd(top); X; R(r); L;
	Lab(lab); C(':'); L;
	Pop(2);
	Push(EESDATA);
	ees[top].ptype := tboolean;
	ees[top].size := BOOLSIZE;
	ees[top].dreg := r;
    end;
end;

procedure opintc;
var
    wordsize : sizerange;
begin
    check2(2,[tset]);

    if ees[top].size <= WORDSIZE then begin
	Eval(top);
	Op('mcoml'); Opnd(top); X; Opnd(top); L;
	TwoOrThree('biclN',top,top-1,tset,WORDSIZE);
    end else begin
	wordsize := (ees[top].size + WORDSIZE-1) div WORDSIZE;
	Eval(top);
	Push(EESDATA);
	ees[top].ptype := tinteger;
	ees[top].size := WORDSIZE;
	ees[top].constInt := wordsize;
	MultiWordBinOp('mcoml',top-1,top-1);
	Eval(top-1);
	Push(EESDATA);
	ees[top].ptype := tinteger;
	ees[top].size := WORDSIZE;
	ees[top].constInt := wordsize;
	MultiWordBinOp('bicl2',top-1,top-2);
    end;
    Pop(1);
end;

procedure opiorc;
begin
    check2(2,[tboolean]);

    TwoOrThree('bisbN',top,top-1,tboolean,BOOLSIZE);
    Pop(1);
end;

procedure opixac;
var
    sunits : sizerange;
    done : boolean;
begin
    if opdcount<>1 then error(9);

    done := false;
    sunits := Int(opd^[1]);
    if ees[top-1].indirect or (ees[top-1].kind in [EESDATA,EESVAR]) then begin
	{ get data value and make it a base reg }
	Eval(top-1);
	ees[top-1].breg := ees[top-1].dreg;
	ees[top-1].dreg := NULLREG;
	ees[top-1].kind := EESADDR;
	ees[top-1].size := WORDSIZE;
	ees[top-1].ptype := taddress;
	ClearAddress(top-1);
    end;
    { definitely have an address now }
    if ees[top].kind in [EESDATA,EESVAR] then begin
	{ fix up constant part }
	ees[top-1].addrOffset := ees[top-1].addrOffset
					+ sunits * ees[top].constInt;
	ees[top].constInt := 0;
	{ if only a constant, all done }
	done := (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG);
    end;
    if done then begin
	{ do nothing }
    end else begin
	if (ees[top].kind = EESVAR) and AddrIsTReg(top)
		and not ees[top].indirect
	then begin
	    { make it dreg: will become sreg below }
	    ees[top].dreg := MemTReg(ees[top].addrOffset);
	    ClearAddress(top);
	end else begin
	    Eval(top);
	end;
	if ees[top-1].sreg <> NULLREG then begin
	    { already a subscript, scale and add this one }
	    CheckSub(top-1,sunits);
	    Op('addl3'); R(ees[top].dreg); X;
		    TRegOpnd(ees[top-1].sreg,top-1); L;
	end else begin
	    { make this the subscript }
	    ees[top-1].sreg := ees[top].dreg;
	    MoveReg(top-1,ees[top].dreg);
	    ees[top].dreg := NULLREG;
	end;
	ees[top-1].sunits := sunits;
    end;
    Pop(1);
end;

procedure opinxc;
begin
end;

procedure oplabc;
var
    i : integer;
begin
    if pclbsize=0 then error(19);
    for i := 1 to pclbsize do opd^[1][i]:=pclabel[i];
    opdsizes^[1] := pclbsize;
    if opd11 = 'l' then begin
    end;
    writelabel(1); C(':'); L;
    ClearDisp;
end;

procedure oplaoc;
begin
    Push(EESADDR);
    ees[top].ptype := taddress;
    ees[top].size := ADDRSIZE;
    ees[top].addrLevel := 0;
    ees[top].addrMemType := opd^[2][1];
    ees[top].addrOffset := Int(opd^[3]);
    ees[top].addrBlock := Int(opd^[4]);
end;

procedure opldac;
begin
    Push(EESADDR);
    ees[top].ptype := taddress;
    ees[top].size := ADDRSIZE;
    ees[top].addrLevel := curlev - Int(opd^[2]);
    ees[top].addrMemType := opd^[3][1];
    ees[top].addrOffset := Int(opd^[4]);
    ees[top].addrBlock := Int(opd^[5]);
end;

procedure oplcac;
var
    allowable : boolean;
    size : sizerange;
    constlab : integer;
    i : integer;
    numChar : integer;
    c : char;
begin
    allowable := false;
    size := Int(opd^[2]);
    checkconstant(size);
    case opd11 of
	's': begin
	    eliminatequotes(3);
	    constlab := currentConstant;
	    currentConstant := currentConstant + 1;
	    Op('.data'); I(1); L;
	    Op('.align'); I(2); L;
	    C('k'); I(constlab); C(':'); L;
	    numChar := opdsizes^[3];
	    for i := 0 to numChar-1 do begin
		if i mod 8 = 0 then begin
		    Op('.byte');
		end;
		c := opd^[3][i+1];
		if c in [' '..'~'] then begin
		    C(''''); C(c);
		end else begin
		    I(ord(c));
		end;
		if (i mod 8 = 7) or (i = numChar-1) then begin
		    L;
		end else begin
		    C(',');
		end;
	    end;
	    Op('.space'); I((size+BYTESIZE-1) div BYTESIZE - numChar + 1); L;
	    Op('.text'); L;
	    Push(EESADDR);
	    ees[top].ptype := taddress;
	    ees[top].size := ADDRSIZE;
	    ees[top].addrMemType := 'k';
	    ees[top].addrBlock := constlab;
	    allowable := true;
	end;
	'S': begin
	    Push(EESADDR);
	    ees[top].ptype := taddress;
	    ees[top].size := ADDRSIZE;
	    SetConst(size,top);
	    allowable := true;
	end;
    end;
    if not allowable then error(1);
end;

procedure opldcc;
var
    allowable : boolean;
    size : sizerange;
    i : integer;
    numChar : integer;
    bits, bitpos : integer;
begin
    allowable := false;
    size := Int(opd^[2]);
    checkconstant(size);
    case opd11 of
	'a': begin
	    Push(EESADDR);
	    ees[top].ptype := taddress;
	    ees[top].size := ADDRSIZE;
	    ees[top].addrOffset := Int(opd^[3]);
	    allowable := true;
	end;
	'b': begin
	    Push(EESDATA);
	    ees[top].ptype := tboolean;
	    ees[top].size := BOOLSIZE;
	    ees[top].constInt := Int(opd^[3]);
	    allowable := true;
	end;
	'c': begin
	    eliminatequotes(3);
	    Push(EESDATA);
	    ees[top].ptype := tchar;
	    ees[top].size := BYTESIZE;
	    ees[top].constInt := ord(opd^[3][1]);
	    allowable := true;
	end;
	'i': begin
	    Push(EESDATA);
	    ees[top].ptype := tinteger;
	    ees[top].size := WORDSIZE;
	    ees[top].constInt := Int(opd^[3]);
	    allowable := true;
	end;
	'j': begin
	    Push(EESDATA);
	    ees[top].ptype := tinteger;
	    ees[top].size := WORDSIZE;
	    ees[top].constInt := Int(opd^[3]);
	    allowable := true;
	end;
	'n': begin
	    Push(EESDATA);
	    ees[top].ptype := taddress;
	    ees[top].size := WORDSIZE;
	    ees[top].constInt := 0;
	    allowable := true;
	end;
	'p': begin
	    Push(EESDATA);
	    ees[top].ptype := tproc;
	    ees[top].size := WORDSIZE;
	    ees[top].dreg := AllocReg(REGEES,top,taddress);
	    Op('movab'); C('_'); SO(opd^[3]); X; Opnd(top); L;
	    allowable := true;
	    end;
	'r': begin
	    Push(EESDATA);
	    ees[top].ptype := treal;
	    ees[top].size := WORDSIZE;
	    ees[top].dreg := AllocReg(REGEES,top,treal);
	    Op('movf'); S('$0f'); SO(opd^[3]); X; Opnd(top); L;
	    allowable := true;
	end;
	'R': begin
	    Push(EESDATA);
	    ees[top].ptype := tlongreal;
	    ees[top].size := 2*WORDSIZE;
	    ees[top].dreg := AllocReg(REGEES,top,tlongreal);
	    Op('movd'); S('$0f'); SO(opd^[3]); X; Opnd(top); L;
	    allowable := true;
	end;
	's': begin
	    { do same thing as lca, except make it a variable }
	    oplcac;
	    ees[top].kind := EESVAR;
	    ees[top].size := Int(opd^[2]);
	    ees[top].ptype := tstring;
	    allowable := true;
	end;
	'S': begin
	    if size > WORDSIZE then begin
		{ do same thing as lca, except make it a variable }
		oplcac;
		ees[top].kind := EESVAR;
		ees[top].size := Int(opd^[2]);
	    end else begin
		numChar := Int(opd^[3]);
		bits := 0;
		bitpos := 1;
		for i := 1 to numChar do begin
		    if opd^[4][i] = '1' then begin
			bits := bits + bitpos;
		    end;
		    bitpos := Mult(bitpos,2);
		end;
		Push(EESDATA);
		ees[top].constInt := bits;
		ees[top].kind := EESDATA;
		ees[top].size := WORDSIZE;
	    end;
	    ees[top].ptype := tset;
	    allowable := true;
	end;
    end;
    if not allowable then error(1);
end;

procedure opldoc;
begin
    Push(EESVAR);
    ees[top].ptype := associatedType[opd11];
    ees[top].size := Int(opd^[2]);
    ees[top].addrLevel := 0;
    ees[top].addrMemType := opd^[3][1];
    ees[top].addrOffset := Int(opd^[4]);
    ees[top].addrBlock := Int(opd^[5]);
end;

procedure opleqc;
begin
    Compare(associatedType[opd11],Int(opd^[2]));
    if opd11 in ['c','j'] then begin
	nextjump := jcleu;	{ set up to do leu jump }
    end else begin
	nextjump := jcle;	{ set up to do le jump }
    end;
end;

procedure oplesc;
begin
    Compare(associatedType[opd11],Int(opd^[2]));
    if opd11 in ['c','j'] then begin
	nextjump := jcltu;	{ set up to do ltu jump }
    end else begin
	nextjump := jclt;	{ set up to do lt jump }
    end;
end;

procedure oplocc;
begin
    DumpEES;
end; 

procedure oplodc;
begin
    Push(EESVAR);
    ees[top].ptype := associatedType[opd11];
    ees[top].size := Int(opd^[2]);
    ees[top].addrLevel := curlev - Int(opd^[3]);
    ees[top].addrMemType := opd^[4][1];
    ees[top].addrOffset := Int(opd^[5]);
    ees[top].addrBlock := Int(opd^[6]);
end;

procedure opmodc;
var
    r : Reg;
    k : integer;
    done : boolean;
begin
    done := false;
    if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) then begin
	k := PowerOfTwo(ees[top].constInt);
	if k > 0 then begin
	    Eval(top-1);
	    { just mask off high order bits }
	    { (because value is a power of 2, -value is correct mask) }
	    Op('bicl2'); C('$'); I(-ees[top].constInt); X; Opnd(top-1); L;
	    Pop(1);
	    done := true;
	end;
    end;
    if done then begin
	{ all done }
    end else begin
	if opd11 = 'j' then begin
	    CallProc(opcep,'i',WORDSIZE,2,RTLONGMOD);
	end else begin
	    Check(top,WORDSIZE);
	    Eval(top-1);
	    r := AllocReg(REGTEMP,0,tinteger);
	    Op('divl3'); Opnd(top); X; Opnd(top-1); X; R(r); L;
	    Op('mull2'); Opnd(top); X; R(r); L;
	    Op('subl2'); R(r); X; Opnd(top-1); L;
	    FreeReg(r);
	    Pop(1);
	end;
    end;
end;

procedure opmvnc;
var
    size : sizerange;
begin
    if (opdcount<>1) then error(9);
    size := Int(opd^[1]);
    Eval(top);
    Op('mull2'); C('$'); I(size); X; Opnd(top); L;
    Op('addl2'); C('$'); I(WORDSIZE div BYTESIZE - 1); X; Opnd(top); L;
    Op('divl2'); C('$'); I(WORDSIZE div BYTESIZE); X; Opnd(top); L;
    MultiWordBinOp('movl',top-1,top-2);
    Pop(2);
end;

procedure opmovc;
begin
    if (opdcount<>1) then error(9);
    Push(EESDATA);
    ees[top].ptype := tinteger;
    ees[top].size := WORDSIZE;
    ees[top].constInt := (Int(opd^[1]) + WORDSIZE-1) div WORDSIZE;
    MultiWordBinOp('movl',top-1,top-2);
    Pop(2);
end;
procedure opmstc;
begin
end;

procedure opmupc;
var
    k, saveconst : integer;
begin
    if opd11 = 'r' then begin
	TwoOrThree('mulfN',top,top-1,treal,WORDSIZE);
    end else if opd11 = 'R' then begin
	TwoOrThree('muldN',top,top-1,tlongreal,2*WORDSIZE);
    end else if opd11 = 'a' then begin
	if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG)
	    and (ees[top-1].kind = EESADDR) and (ees[top-1].breg = NULLREG)
	    and (ees[top-1].addrMemType = ' ') and (ees[top-1].sreg = NULLREG)
	then begin
	    { both operands are constant, top-1 is address }
	    ees[top-1].addrOffset := ees[top-1].addrOffset * ees[top].constInt;
	end else if (ees[top-1].kind = EESDATA) and (ees[top-1].dreg = NULLREG)
	    and (ees[top].kind = EESADDR) and (ees[top].breg = NULLREG)
	    and (ees[top].addrMemType = ' ') and (ees[top].sreg = NULLREG)
	then begin
	    { both operands are constant, top is address }
	    SwapEES(top,top-1);
	    ees[top-1].addrOffset := ees[top-1].addrOffset * ees[top].constInt;
	end else begin
	    if (ees[top-1].kind = EESADDR) and (ees[top-1].breg = NULLREG)
	    and (ees[top-1].addrMemType = ' ') and (ees[top-1].sreg = NULLREG)
	    then begin
		{ top-1 is constant }
		SwapEES(top,top-1);	{ make top constant }
	    end;
	    if (ees[top].kind <> EESADDR) or (ees[top].breg <> NULLREG)
		or (ees[top].addrMemType <> ' ') or (ees[top].sreg <> NULLREG)
		or (ees[top-1].kind = EESADDR)
	    then begin
		Error('opmupc: multiply by variable address');
	    end else begin
		if ees[top].constInt = BYTESIZE then begin
		    { do nothing }
		end else begin
		    saveconst := Mult(ees[top].addrOffset,ees[top-1].constInt);
		    ees[top-1].constInt := 0;
		    if (ees[top-1].kind = EESVAR) and not ees[top-1].indirect
			    and AddrIsTReg(top-1)
		    then begin
			{ make it dreg: will become sreg below }
			ees[top-1].dreg := MemTReg(ees[top-1].addrOffset);
			ClearAddress(top-1);
		    end else begin
			Eval(top-1);
		    end;
		    ees[top-1].kind := EESADDR;
		    ees[top-1].sunits := ees[top].addrOffset;
		    ees[top-1].sreg := ees[top-1].dreg;
		    ees[top-1].dreg := NULLREG;
		    ees[top-1].addrOffset := saveconst;
		end;
	    end;
	end;
	ees[top-1].ptype := taddress;
	ees[top-1].size := WORDSIZE;
    { else not address }
    end else if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG)
        and (ees[top-1].kind = EESDATA) and (ees[top-1].dreg = NULLREG)
    then begin
	{ both operands are constant }
	ees[top-1].constInt := Mult(ees[top-1].constInt,ees[top].constInt);
    end else begin
	if (ees[top-1].kind = EESDATA) and (ees[top-1].dreg = NULLREG)
	then begin
	    { top-1 is constant }
	    SwapEES(top,top-1);	{ make top constant }
	end;
	if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) then begin
	    { multiply by constant }
	    if ees[top].constInt = 1 then begin
		{ do nothing }
	    end else begin
		{ do constant part }
		saveconst := Mult(ees[top].constInt,ees[top-1].constInt);
		ees[top-1].constInt := 0;
		k := PowerOfTwo(ees[top].constInt);
		if k = 1 then begin
		    TwoOrThree('addlN',top-1,top-1,tinteger,WORDSIZE);
		end else if k = 2 then begin
		    Eval(top-1);
		    Op('moval'); S('0['); Opnd(top-1); C(']'); X;
			    Opnd(top-1); L;
		end else if k > 0 then begin
		    Eval(top-1);
		    Op('ashl'); C('$'); I(k); X; Opnd(top-1); X; Opnd(top-1); L;
		end else begin
		    TwoOrThree('mullN',top,top-1,tinteger,WORDSIZE);
		end;
		ees[top-1].constInt := saveconst;
	    end;
	end else begin
	    TwoOrThree('mullN',top,top-1,tinteger,WORDSIZE);
	end;
    end;
    Pop(1);
end;

procedure opmp2c;
var
    size, k : integer;
begin
    size := Int(opd^[2]);
    MakeVariable(top-1);
    Check(top-1,size);
    Check(top,size);
    ees[top-1].size := size;
    if (size <> WORDSIZE) and (size <> BYTESIZE) then begin
	Error('opmp2: bad size');
    end;
    if opd11 = 'r' then begin
	ees[top-1].ptype := treal;
	Op('mulf2'); Opnd(top); X; Opnd(top-1); L;
    end else if opd11 = 'R' then begin
	ees[top-1].ptype := tlongreal;
	Op('muld2'); Opnd(top); X; Opnd(top-1); L;
    end else begin
	if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) then begin
	    { multiply by constant }
	    if ees[top].constInt = 1 then begin
		{ do nothing }
	    end else begin
		k := PowerOfTwo(ees[top].constInt);
		if k = 1 then begin
		    Op('addl2'); Opnd(top-1); X; Opnd(top-1); L;
		end else if k > 0 then begin
		    Op('ashl'); C('$'); I(k); X; Opnd(top-1); X; Opnd(top-1); L;
		end else begin
		    Op('mull2'); Opnd(top); X; Opnd(top-1); L;
		end;
	    end;
	end else begin
	    Op('mull2'); Opnd(top); X; Opnd(top-1); L;
	end;
    end;
    Pop(2);
end;

procedure opmusc;
var
    size : sizerange;
    r : Reg;
begin
    size := Int(opd^[1]);
    if (opdcount<>1) then error(9);
    if not (ees[top].ptype in [tinteger,tcardinal,tchar]) then
	error(2);
    if ees[top-1].ptype<>ees[top].ptype
	then error(4);
    if size <= WORDSIZE then begin
	Check(top-1,WORDSIZE);
	Eval(top);
	{ top-1 is start bit, top is stop bit }
	{ make top be count (stop-start+1) }
	Op('subl2'); Opnd(top-1); X; Opnd(top); L;
	Op('incl2'); Opnd(top); L;
	r := AllocReg(REGEES,top-1,tset);
	{ clear result reg }
	Op('clrl'); R(r); L;
	{ select range from all 1's    start pos      count bits    result }
	Op('insv'); C('$'); I(-1); X; Opnd(top-1); X; Opnd(top); X; R(r); L;
        Pop(2);
	Push(EESDATA);
	ees[top].size := size;
	ees[top].ptype := tset;
	ees[top].dreg := r;
    end else begin
      { call routine to generate set }
      CallProc (opcep, 'S', size, 3, RTMAKESET);
    end;
end;

procedure opnamc;
begin
    DumpReg;
end;

procedure opnegc;
begin
    Eval(top);
    if opd11 = 'r' then begin
	Op('mnegf'); Opnd(top); X; Opnd(top); L;
    end else if opd11 = 'R' then begin
	Op('mnegd'); Opnd(top); X; Opnd(top); L;
    end else begin
	Op('mnegl'); Opnd(top); X; Opnd(top); L;
    end;
end;

procedure opneqc;
begin
    Compare(associatedType[opd11],Int(opd^[2]));
    nextjump := jcne;       { set up for ne jump }
end;

procedure opnewc;
var size : sizerange;
begin
    if (ees[top].ptype<>tinteger) then error(2);
    if (opdcount<>1) then error(9);

    size := (Int(opd^[1]) + WORDSIZE-1) div WORDSIZE;
    if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) then begin
	{ constant size }
	ees[top].constInt := ees[top].constInt * size;
    end else begin
	{ compute size }
	Eval(top);
	Op('mull2'); C('$'); I(size); X; Opnd(top); L;
    end;
    CallProc(opcep,'a',ADDRSIZE,1,RTNEW);
end;

procedure opnotc;
begin
    check1(1,[tboolean]);

    PushConst(tboolean,BOOLSIZE,1);
    TwoOrThree('xorbN',top,top-1,tboolean,BOOLSIZE);
    Pop(1);
end;

procedure opoddc;
begin
    check1(0,[tinteger,tcardinal]);

    PushConst(tinteger,WORDSIZE,-2);
    TwoOrThree('biclN',top,top-1,tboolean,WORDSIZE);
    Pop(1);
    ees[top].ptype := tboolean;
    ees[top].size := BOOLSIZE;
end;

procedure opordc;
begin
    ees[top].ptype := tcardinal;
end;

procedure opretc;
var
    r : Reg;
    off : integer;
begin
    if (opdcount<>1) then error(9);
    if (opdsizes^[1]<>1) then error(1);
    if ((opd11='p') and (top<>0)) or
	 (opd11<>'p') and (top<>1)
    then begin
	Error('Inconsistent stack on return');
    end;
    if (opd11<>'p') then begin
	if ees[top].ptype = tlongreal then begin
	    Check(top,2*WORDSIZE);
	    if (ees[top].kind <> EESDATA) or (ees[top].dreg <> RETURNREG)
	    then begin
		r := AllocReg(REGRETURN,0,ees[top].ptype);
		Op('movq'); Opnd(top); X; R(r); L;
		FreeReg(r);
	    end;
	end else if ees[top].size > WORDSIZE then begin
	    Point(top);
	    if ees[top].breg <> RETURNREG then begin
		r := AllocReg(REGRETURN,0,taddress);
		Op('movl'); R(ees[top].breg); X; R(r); L;
		FreeReg(r);
	    end;
	end else begin
	    Check(top,WORDSIZE);
	    if (ees[top].kind <> EESDATA) or (ees[top].dreg <> RETURNREG)
	    then begin
		r := AllocReg(REGRETURN,0,ees[top].ptype);
		Op('movl'); Opnd(top); X; R(r); L;
		FreeReg(r);
	    end;
	end;
	Pop(1);
    end;
    if not nodisplay then begin
	{ restore display }
	Op('movl'); I(DISPOFF); C('('); R(fp); C(')'); X;
		S('_runtime__display+'); I(curlev*4); L;
    end;
    if curblockid = mainprogblockid then begin
	Op('pushl'); S('$0'); L;
	Op('calls'); S('$1,_exit'); L;
    end else if not internal then begin
	Op('ret'); L;
    end else begin
	if firsttreg <> NULLREG then begin
	    off := 4;
	    { restore regs from after end of activation record }
	    {  (-arsize-4)(fp), (-arsize-8)(fp), etc. }
	    for r := firsttreg to LASTREG do begin
		Op('movl'); S('(-a'); I(curblockid); C('-'); I(off);
			S(')(fp)'); X; R(r); L;
		off := off + 4;
	    end;
	end;
	Op('movl'); S('8(fp)'); X; R(ap); L;
	Op('movab'); S('16(fp)'); X; R(sp); L;
	Op('movl'); S('12(fp)'); X; R(fp); L;
	Op('rsb'); L;
    end;
    top := 0;
end;

procedure opsalc;
var
    size : integer;
begin
    if (opdcount<>1) then error(9);
    size := Int(opd^[1]);
    if size <= 8 then begin
	size := 1;
    end else begin
	{ round to words, units of bytes }
	size := (size+WORDSIZE-1) div WORDSIZE * (WORDSIZE div BYTESIZE);
    end;

    { stack starts out top=<number of elements>, top-1=<address of pointer> }

    { multiply unit size by number on top of stack }
    Eval(top);
    Op('mull2'); C('$'); I(size); X; Opnd(top); L;

    { round up to words (in units of bytes) }
    Op('addl2'); C('$'); I(WORDSIZE div BYTESIZE - 1); X; Opnd(top); L;
    Op('bicl2'); C('$'); I(WORDSIZE div BYTESIZE - 1); X; Opnd(top); L;

    { subtract this amount from sp to allocate }
    Op('subl2'); Opnd(top); X; R(sp); L;

    { convert bytes to words }
    Op('divl2'); C('$'); I(WORDSIZE div BYTESIZE); X; Opnd(top); L;

    { get two elements for pointers }
    PushReg(taddress,WORDSIZE,AllocReg(REGREG,0,taddress));
    PushReg(taddress,WORDSIZE,AllocReg(REGREG,0,taddress));

    { top=<reg> top-1=<reg> top-2=<length> top-3=<address of pointer> }
    SwapEES(top,top-2);
    { top=<length> top-1=<reg> top-2=<reg> top-3=<address of pointer> }

    { address of data pointer }
    ees[top-3].kind := EESVAR;

    { source of data }
    Op('movl'); Opnd(top-3); X; Opnd(top-1); L;
    { destination of data }
    Op('movl'); R(sp); X; Opnd(top-2); L;

    { top=<length> top-1=<pointer> top-2=<new area> top-3=<address of pointer> }
    { copy data }
    MultiWordBinOp('movl',top-1,top-2);

    { top=<pointer> top-1=<new area> top-2=<address of pointer> }

    { set pointer to point to new area }
    Op('movl'); R(sp); X; Opnd(top-2); L;

    Pop(3);
end;

procedure opsdfc;
var
    wordsize : sizerange;
begin
    check2(2,[tset]);

    if ees[top].size <= WORDSIZE then begin
	TwoOrThree('xorlN',top,top-1,tset,WORDSIZE);
    end else begin
	wordsize := (ees[top].size + WORDSIZE-1) div WORDSIZE;
	Eval(top-1);
	Push(EESDATA);
	ees[top].ptype := tinteger;
	ees[top].size := WORDSIZE;
	ees[top].constInt := wordsize;
	MultiWordBinOp('xorl2',top-1,top-2);
    end;
    Pop(1);
end;

procedure opsgsc;
var
    size : sizerange;
    r : Reg;
begin
    size := Int(opd^[1]);
    if (ees[top].ptype<>tinteger) and (ees[top].ptype<>tchar) then
	error(2);
    if (opdcount<>1) then error(9);

    if size <= WORDSIZE then begin
	Check(top,WORDSIZE);
	r := AllocReg(REGEES,top,tset);
	Op('clrl'); R(r); L;
	Op('insv'); S('$1'); X; Opnd(top); X; S('$1'); X; R(r); L;
	Pop(1);
	Push(EESDATA);
	ees[top].ptype := tset;
	ees[top].size := size;
	ees[top].dreg := r;
    end else begin
	{ allocate space }
	PushMultiWordTemp(tset,size);
	{ clear it out }
	Push(EESDATA);
	ees[top].ptype := tinteger;
	ees[top].size := WORDSIZE;
	ees[top].constInt := (size + WORDSIZE-1) div WORDSIZE;
	MultiWordUnOp('clrl',top-1);

	{ element to add }
	Eval(top-1);
	Op('insv'); S('$1'); X; Opnd(top-1); X; S('$1'); X; Opnd(top); L;

	{ get rid of top-1 }
	SwapEES(top,top-1);
	Pop(1);
    end;
end;

procedure opsmlc;
var
    r : Reg;
begin
    if (opdcount<>0) then error(9);
    if (ees[top].ptype<>tset) then error(2);

    if (ees[top].size>WORDSIZE) then begin
	Push(EESDATA);
	ees[top].ptype := tinteger;
	ees[top].size := WORDSIZE;
	ees[top].constInt := ees[top].size;
	CallProc(opcep,'i',WORDSIZE,2,RTSMALLEST);
    end else begin
	Eval(top); { instruction requires register }
	r := AllocReg(REGEES,top,tinteger);
	Op('ffs'); S('$0'); X; C('$'); I(ees[top].size); X; Opnd(top);
			    X; R(r); L;
	Pop(1);
	Push(EESDATA);
	ees[top].ptype := tinteger;
	ees[top].size := WORDSIZE;
	ees[top].dreg := r;
    end;
end;

procedure opsroc;
var
    ptype : pcodetype;
    size : sizerange;
begin
    ptype := associatedType[opd11];
    size := Int(opd^[2]);
    Push(EESVAR);
    ees[top].ptype := ptype;
    ees[top].size := size;
    ees[top].addrLevel := 0;
    ees[top].addrMemType := opd^[3][1];
    ees[top].addrOffset := Int(opd^[4]);
    ees[top].addrBlock := Int(opd^[5]);
    Store(top-1,top);
    Pop(2);
end;

procedure opstnc;
begin
    Push(EESVAR);
    ees[top].ptype := associatedType[opd11];
    ees[top].size := Int(opd^[2]);
    ees[top].addrLevel := curlev - Int(opd^[3]);
    ees[top].addrMemType := opd^[4][1];
    ees[top].addrOffset := Int(opd^[5]);
    ees[top].addrBlock := Int(opd^[6]);
    if AddrIsTReg(top) then begin
	{ store into t reg }
	Store(top-1,top);
	{ use t reg as value on top of stack }
	SwapEES(top-1,top);
    end else begin
	{ force Eval of value to keep put it in a register }
	Eval(top-1);
	Store(top-1,top);
    end;
    Pop(1);
end;

procedure opstoc;
begin
    MakeVariable(top);
    ees[top].ptype := associatedType[opd11];
    ees[top].size := Int(opd^[2]);
    Store(top-1,top);
    Pop(2);
end;

procedure opstpc;
begin
    if (opdcount<>0) then error(9);
    if (top<>0) then error(24);
end;

procedure opstrc;
begin
    Push(EESVAR);
    ees[top].ptype := associatedType[opd11];
    ees[top].size := Int(opd^[2]);
    ees[top].addrLevel := curlev - Int(opd^[3]);
    ees[top].addrMemType := opd^[4][1];
    ees[top].addrOffset := Int(opd^[5]);
    ees[top].addrBlock := Int(opd^[6]);
    Store(top-1,top);
    Pop(2);
end;

procedure opsubc;
begin
    if opd11 = 'r' then begin
	TwoOrThree('subfN',top,top-1,treal,WORDSIZE);
    end else if opd11 = 'R' then begin
	TwoOrThree('subdN',top,top-1,tlongreal,2*WORDSIZE);
    end else if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG)
	and (ees[top-1].kind in [EESDATA,EESVAR])
    then begin
	{ top is constant }
	ees[top-1].constInt := ees[top-1].constInt - ees[top].constInt;
    end else begin
	TwoOrThree('sublN',top,top-1,tinteger,WORDSIZE);
    end;
    Pop(1);
end;

procedure opsb2c;
var
    size : integer;
begin
    size := Int(opd^[2]);
    MakeVariable(top-1);
    Check(top-1,size);
    Check(top,size);
    ees[top-1].size := size;
    if (size <> WORDSIZE) and (size <> BYTESIZE) then begin
	Error('opsb2: bad size');
    end;
    if opd11 = 'r' then begin
	ees[top-1].ptype := treal;
	Op('subf2'); Opnd(top); X; Opnd(top-1); L;
    end else if opd11 = 'R' then begin
	ees[top-1].ptype := tlongreal;
	Op('subd2'); Opnd(top); X; Opnd(top-1); L;
    end else begin
	if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) and
		(ees[top].constInt = 1)
	then begin
	    if size = BYTESIZE then begin
		Op('decb'); Opnd(top-1); L;
	    end else begin
		Op('decl'); Opnd(top-1); L;
	    end;
	end else begin
	    if size = BYTESIZE then begin
		Op('subb2');
	    end else begin
		Op('subl2');
	    end;
	    Opnd(top); X; Opnd(top-1); L;
	end;
    end;
    Pop(2);
end;

procedure opsymc;
var
    size, offset, block  : integer;
    stabkind : char;
begin
    stabkind := opd11;
    if stabkind = 'l' then begin
	if opdcount <> 4 then begin
	    error(501);
	end;
	Op('.stabd'); SO(opd^[2]); X; SO(opd^[3]); X; SO(opd^[4]); L;
    end else if stabkind = 'X' then begin
	eliminatequotes(2);
	Op('.stabs'); C('"'); SOEscape(opd^[2],opdsizes^[2]); C('"'); X;
		SO(opd^[3]); X; SO(opd^[4]); X; I(0); X; I(0); L;
    end else begin
	eliminatequotes(2);
	Op('.stabs'); C('"'); SOEscape(opd^[2],opdsizes^[2]); C('"'); X;
		SO(opd^[3]); X; SO(opd^[4]); X;
	if stabkind = 's' then begin
	    SO(opd^[5]); X; writelabel(6); L;
	end else if stabkind in ['t','G'] then begin
	    size := Int(opd^[5]);
	    size := (size + BYTESIZE - 1) div BYTESIZE;
	    I(size); X; SO(opd^[6]); L;
	end else if stabkind = 'p' then begin
	    size := Int(opd^[5]);
	    size := (size + BYTESIZE - 1) div BYTESIZE;
	    offset := Int(opd^[7]);
	    offset := (WORDSIZE + offset + BYTESIZE - 1) div BYTESIZE;
	    I(size); X; I(offset); L;
	end else if stabkind = 'v' then begin
	    size := Int(opd^[5]);
	    size := (size + BYTESIZE - 1) div BYTESIZE;
	    offset := Int(opd^[7]);
	    offset := (offset + BYTESIZE - 1) div BYTESIZE;
	    block := Int(opd^[8]);
	    I(size); X; C(opd^[6][1]); I(block); C('+'); I(offset); L;
	end else if stabkind = 'F' then begin
	    size := Int(opd^[5]);
	    size := (size + BYTESIZE - 1) div BYTESIZE;
	    I(size); X; C('_'); SO(opd^[6]); L;
	end else if stabkind = 'P' then begin
	    size := Int(opd^[5]);
	    size := (size + BYTESIZE - 1) div BYTESIZE;
	    I(size); X; I(0); L;
	end else if stabkind = 'm' then begin
	    I(0); X; I(0); L;
	end else begin
	    error(502);
	    L;
	end;
    end;
end;

procedure opsysc;
begin
    CallProc(opcep,'i',WORDSIZE,2,RTSYSTEM);
end;

procedure optjpc;
begin
    if (ees[top].ptype<>tboolean) then error(2);
    if (opdcount<>1) then error(9);

    Check(top,BOOLSIZE);
    Op('bitb'); S('$1'); X; Opnd(top); L;
    Op('jneq'); writelabel(1); L;
    Pop(1);
end;

procedure optrcc;
var
    r : Reg;
begin
    if opd11 = 'r' then begin
	Eval(top);
	Op('cvtfl'); Opnd(top); X; Opnd(top); L;
	ees[top].ptype := tinteger;
    end else if opd11 = 'R' then begin
	Eval(top);
	r := AllocReg(REGEES,top,tinteger);
	Op('cvtdl'); Opnd(top); X; R(r); L;
	FreeReg(ees[top].dreg);
	ees[top].dreg := r;
	ees[top].ptype := tinteger;
	ees[top].size := WORDSIZE;
    end else begin
	Error('Bad type');
    end;
end;

procedure optypc;
var
    ptype : pcodetype;
    size : integer;
begin
    if (opdcount<>1) then error(9);
    if (opdsizes^[1]<>1) then error(1);

    ptype := associatedType[opd11];
    size := datasize(ptype,ees[top].size);
    if (ees[top].kind in [EESDATA,EESVAR]) and
	    (size = ees[top].size)
	    { Experiment: only worry about size 
	    and (ees[top].ptype in [tinteger,tcardinal,taddress])
	    and (ptype in [tinteger,tcardinal,taddress])
	    }
    then begin
	{ don't need to eval }
    end else begin
	Eval(top);
    end;
    ees[top].ptype := ptype;
    ees[top].size := datasize(ees[top].ptype,ees[top].size);
end;

procedure opujpc;
begin
    if opdcount<>1 then error(9);
    Op('jbr'); writelabel(1); L;
end;

procedure opunic;
var
    wordsize : sizerange;
begin
    check2(2,[tset]);

    if ees[top].size <= WORDSIZE then begin
	TwoOrThree('bislN',top,top-1,tset,WORDSIZE);
    end else begin
	wordsize := (ees[top].size + WORDSIZE-1) div WORDSIZE;
	Eval(top-1);
	Push(EESDATA);
	ees[top].ptype := tinteger;
	ees[top].size := WORDSIZE;
	ees[top].constInt := wordsize;
	MultiWordBinOp('bisl2',top-1,top-2);
    end;
    Pop(1);
end;

procedure opxjpc;
var
    lb, ub : integer;
begin
    if not (ees[top].ptype in [tinteger,tcardinal,tchar]) then
	error(2);

    lb := Int(opd^[3]);
    ub := Int(opd^[4]);
    Eval(top);
    Op('cmpl'); Opnd(top); X; C('$'); I(lb); L;
    Op('jlss'); writelabel(2); L;
    Op('cmpl'); Opnd(top); X; C('$'); I(ub); L;
    Op('jgtr'); writelabel(2); L;
    Op('movl'); C('('); I(-lb*4); C('+'); writelabel(1); S(')[');
		    Opnd(top); C(']'); X; Opnd(top); L;
    Op('jmp'); C('('); Opnd(top); C(')'); L;
    Pop(1);
end;

procedure opzerc;
var
    size : sizerange;
begin
    Push(EESDATA);
    ees[top].ptype := tinteger;
    ees[top].size := WORDSIZE;
    size := Int(opd^[1]);
    if (opdcount<>1) then error(9);
    if ((size mod WORDSIZE)<>0) then error(1);

    ees[top].constInt := size div WORDSIZE;
    MultiWordUnOp('clrl',top-1);
    Pop(1);
end;

procedure opsavc;
var
    e : EESElement;
begin
    e := -Int(opd^[1])-1;
    if ees[e].inUse and (opd^[2][1] <> 'r') then begin
	Error('sav: in use');
    end;
    if not ees[e].inUse and (opd^[2][1] = 'r') then begin
	Error('sav r: in use');
    end;
    if opd^[2][1] = 'r' then begin
	{ replace a value already there }
	if ees[e].size <= WORDSIZE then begin
	    Eval(top);
	    Op('movl'); Opnd(top); X; Opnd(e); L;
	end else begin
	    Store(top,e);
	end;
	Pop(1);
    end else if opd^[2][1] = 'm' then begin
	{ move a value }
	Eval(top);
	SwapEES(top,e);
	Pop(1);
    end else if opd^[2][1] = 'c' then begin
	if ees[top].kind = EESDATA then begin
	    { ok value to save }
	end else if (ees[top].kind = EESADDR) and (ees[top].sreg = NULLREG) and
	    not ees[top].indirect
	then begin
	    { ok value to save }
	end else begin
	    Eval(top);
	end;
	SwapEES(top,e);
	{ copy stack element }
	ees[top] := ees[e];
	{ deallocate memory (temp has it) }
	ees[top].smemoffset := 0;
	ees[top].smemsize := 0;
	{ reallocate and copy register }
	if ees[top].dreg <> NULLREG then begin
	    ees[top].dreg := AllocReg(REGEES,top,ees[top].ptype);
	    Op('movl'); R(ees[e].dreg); X; R(ees[top].dreg); L;
	end;
	if ees[top].breg <> NULLREG then begin
	    ees[top].breg := AllocReg(REGEES,top,taddress);
	    Op('movl'); R(ees[e].breg); X; R(ees[top].breg); L;
	end;
    end else begin
	Error('sav: not r, c or m');
    end;
end;

procedure opusec;
var
    e : EESElement;
begin
    e := -Int(opd^[1])-1;
    if not ees[e].inUse then begin
	Error('use: not in use');
    end;
    Push(EESDATA);
    if opd^[2][1] = 'c' then begin
	{ copy stack element (will get type, etc., from saved element }
	ees[top] := ees[e];
	{ deallocate memory (temp has it) }
	ees[top].smemoffset := 0;
	ees[top].smemsize := 0;
	{ reallocate and copy register }
	if ees[top].dreg <> NULLREG then begin
	    ees[top].dreg := AllocReg(REGEES,top,ees[top].ptype);
	    Op('movl'); R(ees[e].dreg); X; R(ees[top].dreg); L;
	end;
	if ees[top].breg <> NULLREG then begin
	    ees[top].breg := AllocReg(REGEES,top,taddress);
	    Op('movl'); R(ees[e].breg); X; R(ees[top].breg); L;
	end;
    end else if opd^[2][1] = 'm' then begin
	SwapEES(e,top);
	ClearEES(e);
    end else if opd^[2][1] = 'd' then begin
	SwapEES(e,top);
	ClearEES(e);
	Pop(1);
    end else begin
	Error('use: not c, m or d');
    end;
end;

procedure opparc;
begin
end;

procedure opcjpc;
begin
    Op('.long'); writelabel(1); L;
end;

procedure opvinc;
var
    size : integer;
begin
    size := Int(opd^[1]);
    MakeVariable(top-1);
    ees[top-1].size := size;
    ees[top-1].ptype := tinteger;
    Check(top-1,size);
    Check(top,size);
    if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) and
	    (ees[top].constInt = 1)
    then begin
	if size = BYTESIZE then begin
	    Op('incb'); Opnd(top-1); L;
	end else if size = WORDSIZE then begin
	    Op('incl'); Opnd(top-1); L;
	end else begin
	    Error('opvinc: not BYTESIZE or WORDSIZE');
	end;
    end else begin
	if size = BYTESIZE then begin
	    Op('addb2');
	end else if size = WORDSIZE then begin
	    Op('addl2');
	end else begin
	    Error('opvinc: not BYTESIZE or WORDSIZE');
	end;
	Opnd(top); X; Opnd(top-1); L;
    end;
    Pop(2);
end;

procedure opvdec;
var
    size : integer;
begin
    size := Int(opd^[1]);
    MakeVariable(top-1);
    ees[top-1].size := size;
    ees[top-1].ptype := tinteger;
    Check(top-1,size);
    Check(top,size);
    if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) and
	    (ees[top].constInt = 1)
    then begin
	if size = BYTESIZE then begin
	    Op('decb'); Opnd(top-1); L;
	end else if size = WORDSIZE then begin
	    Op('decl'); Opnd(top-1); L;
	end else begin
	    Error('opdecc: not BYTESIZE or WORDSIZE');
	end;
    end else begin
	if size = BYTESIZE then begin
	    Op('subb2');
	end else if size = WORDSIZE then begin
	    Op('subl2');
	end else begin
	    Error('opdecc: not BYTESIZE or WORDSIZE');
	end;
	Opnd(top); X; Opnd(top-1); L;
    end;
    Pop(2);
end;

procedure opsinc;
var
    mask, size : integer;
begin
    if (ees[top-1].ptype<>taddress) then error(2);
    if not (ees[top].ptype in [tinteger,tcardinal,tchar]) then begin
	error(4);
    end;
    size := Int(opd^[1]);
    MakeVariable(top-1);
    ees[top-1].size := size;
    ees[top-1].ptype := tset;
    if ees[top-1].addrOffset mod BYTESIZE <> 0 then begin
	Eval(top);
	ees[top].constInt := ees[top].constInt +
			(ees[top-1].addrOffset mod BYTESIZE);
	ees[top-1].addrOffset :=
			(ees[top-1].addrOffset div BYTESIZE) * BYTESIZE;
    end else begin
	Check(top,WORDSIZE);
    end;
    Check(top-1,ees[top-1].size);
    if (ees[top-1].size <= WORDSIZE) and (ees[top].kind = EESDATA) and
	    (ees[top].dreg = NULLREG)
    then begin
	mask := power(2,ees[top].constInt);
	if ees[top-1].size <= BYTESIZE then begin
	    Op('bisb2'); C('$'); I(mask); X; Opnd(top-1); L;
	end else begin
	    Op('bisl2'); C('$'); I(mask); X; Opnd(top-1); L;
	end;
    end else begin
	CheckRegs(top-1,BYTESIZE);	{ instruction requires byte index }
	Op('insv'); S('$1'); X; Opnd(top); X; S('$1'); X; Opnd(top-1); L;
    end;
    Pop(2);
end;

procedure opsexc;
var
    mask, size : integer;
begin
    if (ees[top-1].ptype<>taddress) then error(2);
    if not (ees[top].ptype in [tinteger,tcardinal,tchar]) then begin
	error(4);
    end;
    size := Int(opd^[1]);
    MakeVariable(top-1);
    ees[top-1].size := size;
    ees[top-1].ptype := tset;
    if ees[top-1].addrOffset mod BYTESIZE <> 0 then begin
	Eval(top);
	ees[top].constInt := ees[top].constInt +
			(ees[top-1].addrOffset mod BYTESIZE);
	ees[top-1].addrOffset :=
			(ees[top-1].addrOffset div BYTESIZE) * BYTESIZE;
    end else begin
	Check(top,WORDSIZE);
    end;
    Check(top-1,ees[top-1].size);
    if (ees[top-1].size <= WORDSIZE) and (ees[top].kind = EESDATA) and
	    (ees[top].dreg = NULLREG)
    then begin
	mask := power(2,ees[top].constInt);
	if ees[top-1].size <= BYTESIZE then begin
	    Op('bicb2'); C('$'); I(mask); X; Opnd(top-1); L;
	end else begin
	    Op('bicl2'); C('$'); I(mask); X; Opnd(top-1); L;
	end;
    end else begin
	Check(top,WORDSIZE);
	CheckRegs(top-1,BYTESIZE);	{ instruction requires byte index }
	Op('insv'); S('$0'); X; Opnd(top); X; S('$1'); X; Opnd(top-1); L;
    end;
    Pop(2);
end;

procedure opforc;
var
    size : integer;
    ptype : pcodetype;
    downward : boolean;
    r : Reg;
begin
    ptype := associatedType[opd11];
    size := Int(opd^[2]);
    MakeVariable(top-2);	{ loop index }
    ees[top-2].size := size;
    ees[top-2].ptype := ptype;
    Check(top-2,size);
    Check(top,WORDSIZE);
    if (ees[top-1].kind = EESADDR) and (ptype = taddress) then begin
	if (ees[top-1].addrMemType <> ' ') or (ees[top-1].breg <> NULLREG)
	    or (ees[top-1].sreg <> NULLREG)
	    or (ees[top-1].addrOffset mod BYTESIZE <> 0)
	then begin
	    Error('opforc: non-constant address increment');
	end else begin
	    ees[top-1].kind := EESDATA;
	    ees[top-1].constInt := ees[top-1].addrOffset div BYTESIZE;
	    ClearAddress(top-1);
	end;
    end;
    if (ees[top-1].kind <> EESDATA) or (ees[top-1].dreg <> NULLREG) then begin
	Error('opforc: non-constant increment');
    end;
    downward := ees[top-1].constInt < 0;
    if size = WORDSIZE then begin
	if downward then begin
	    if ees[top-1].constInt = -1 then begin
		Op('decl'); Opnd(top-2); L;
	    end else begin
		ees[top-1].constInt := -ees[top-1].constInt;
		Op('subl2'); Opnd(top-1); X; Opnd(top-2); L;
	    end;
	end else begin
	    if ees[top-1].constInt = 1 then begin
		Op('incl'); Opnd(top-2); L;
	    end else begin
		Op('addl2'); Opnd(top-1); X; Opnd(top-2); L;
	    end;
	end;
	if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) and
	    (ees[top].constInt = 0)
	then begin
	    { no need to do compare }
	end else begin
	    Op('cmpl'); Opnd(top-2); X; Opnd(top); L;
	end;
	if downward then begin
	    Op('jgeq'); writelabel(3); L;
	end else begin
	    Op('jleq'); writelabel(3); L;
	end;
	{*** slower! should be generated only for variable increments
	    Op('acbl'); Opnd(top); X; Opnd(top-1); X; Opnd(top-2); X; 
			    writelabel(3); L;
	***}
    end else if size = BYTESIZE then begin
	{ do byte arithmetic as word to avoid overflow problems }
	r := AllocReg(REGTEMP,0,tinteger);
	if ptype in [tcardinal,tchar] then begin
	    Op('movzbl'); Opnd(top-2); X; R(r); L;
	end else begin
	    Op('cvtbl'); Opnd(top-2); X; R(r); L;
	end;
	Op('addl2'); Opnd(top-1); X; R(r); L;
	Op('movb'); R(r); X; Opnd(top-2); L;
	Op('cmpl'); R(r); X; Opnd(top); L;
	if downward then begin
	    Op('jgequ'); writelabel(3); L;
	end else begin
	    Op('jlequ'); writelabel(3); L;
	end;
    end else begin
	Error('opforc: not BYTESIZE or WORDSIZE');
    end;
    Pop(3);
end;

procedure opmaxc;
var lab : LabelNumber;
begin
    check2(1,[tinteger,tcardinal,treal,tlongreal]);

    Eval(top-1);
    lab := NewLabel;
    if opd11 = 'r' then begin
	Check(top,WORDSIZE);
	Op('cmpf'); Opnd(top-1); X; Opnd(top); L;
	Op('jgeq'); Lab(lab); L;
	Op('movf'); Opnd(top); X; Opnd(top-1); L;
    end else if opd11 = 'R' then begin
	Check(top,2*WORDSIZE);
	Op('cmpd'); Opnd(top-1); X; Opnd(top); L;
	Op('jgeq'); Lab(lab); L;
	Op('movd'); Opnd(top); X; Opnd(top-1); L;
    end else begin
	Check(top,WORDSIZE);
	Op('cmpl'); Opnd(top-1); X; Opnd(top); L;
	Op('jgeq'); Lab(lab); L;
	Op('movl'); Opnd(top); X; Opnd(top-1); L;
    end;
    Lab(lab); C(':'); L;
    Pop(1);
end;

procedure opminc;
var lab : LabelNumber;
begin
    check2(1,[tinteger,tcardinal,treal,tlongreal]);

    Eval(top-1);
    lab := NewLabel;
    if opd11 = 'r' then begin
	Check(top,WORDSIZE);
	Op('cmpf'); Opnd(top-1); X; Opnd(top); L;
	Op('jleq'); Lab(lab); L;
	Op('movf'); Opnd(top); X; Opnd(top-1); L;
    end else if opd11 = 'R' then begin
	Check(top,2*WORDSIZE);
	Op('cmpd'); Opnd(top-1); X; Opnd(top); L;
	Op('jleq'); Lab(lab); L;
	Op('movd'); Opnd(top); X; Opnd(top-1); L;
    end else begin
	Check(top,WORDSIZE);
	Op('cmpl'); Opnd(top-1); X; Opnd(top); L;
	Op('jleq'); Lab(lab); L;
	Op('movl'); Opnd(top); X; Opnd(top-1); L;
    end;
    Lab(lab); C(':'); L;
    Pop(1);
end;

procedure opbitc;
var
    i, m, n : integer;
    r : Reg;
begin
    if opd11 in ['a','o','x','l','r'] then begin
	Check(top,WORDSIZE);
	Eval(top-1);
	case opd11 of
	    'a' : begin
		Op('mcoml'); Opnd(top-1); X; Opnd(top-1); L;
		Op('bicl3'); Opnd(top-1); X; Opnd(top); X; Opnd(top-1); L;
	    end;
	    'o' : begin
		Op('bisl2'); Opnd(top); X; Opnd(top-1); L;
	    end;
	    'x' : begin
		Op('xorl2'); Opnd(top); X; Opnd(top-1); L;
	    end;
	    'l' : begin
		Op('ashl'); Opnd(top); X; Opnd(top-1); X; Opnd(top-1); L;
	    end;
	    'r' : begin
		if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG)
		then begin
		    Op('extzv'); C('$'); I(ees[top].constInt); X;
			C('$'); I(WORDSIZE-ees[top].constInt); X;
			Opnd(top-1); X; Opnd(top-1); L;
		end else begin
		    r := AllocReg(REGTEMP,0,tinteger);
		    Op('subl3'); Opnd(top); X; C('$'); I(WORDSIZE); X; R(r); L;
		    Op('extzv'); Opnd(top); X; R(r); X; Opnd(top-1);
			    X; Opnd(top-1); L;
		end;
	    end;
	end;
	Pop(1);
    end else if opd11 = 'n' then begin
	Eval(top);
	Op('mcoml'); Opnd(top); X; Opnd(top); L;
    end else if opd11 = 'e' then begin
	{ word, first, size }
	Eval(top-2);
	Check(top-1,WORDSIZE);
	Check(top,WORDSIZE);
	Op('extzv'); Opnd(top-1); X; Opnd(top); X; Opnd(top-2);
			    X; Opnd(top-2); L;
	Pop(2);
    end else if opd11 = 'i' then begin
	{ field first size word }
	SwapEES(top,top-3);
	{ word first size field }
	Eval(top-3);
	Check(top-2,WORDSIZE);
	Check(top-1,WORDSIZE);
	Check(top,WORDSIZE);
	Op('insv'); Opnd(top); X; Opnd(top-2); X; Opnd(top-1);
			    X; Opnd(top-3); L;
	Pop(3);
    end else begin
	Error('opbitc: unknown bit operation');
    end;
end;

procedure opcmtc;
var
    i : integer;
begin
    for i := 1 to opdsizes^[1] do begin
	C(opd^[1][i]);
    end;
    L;
end;

procedure opzzzc; begin error(6); end;
