(*#@(#)codesubs.p	4.1	Ultrix	7/17/90 *)
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
 *									    *
 ****************************************************************************)
#include "globals.h"
#include "util.h"
#include "codesubs.h"
#include "input.h"

function DoJumps{:boolean};
var lab1, lab2 : LabelNumber; didit : boolean;
	{ if tjp, generate jump for nextjump condition }
	{ if fjp, generate jump for not nextjump condition }
	{ otherwise, generate code to set a boolean and push it on top }
	{ when done, indicate whether instruction was done }
begin
    if opcode = opnot then begin
	didit := TRUE;
	nextjump := jumpopposite[nextjump];
    end else if opcode = optjp then begin
	Op(jumpnames[nextjump]); writelabel(1); L;
	didit := TRUE;
	nextjump := jcnone;     { jump was handled }
    end else if opcode = opfjp then begin
	Op(jumpnames[jumpopposite[nextjump]]); writelabel(1); L;
	didit := TRUE;
	nextjump := jcnone;     { jump was handled }
    end else begin
	PushReg(tboolean,BOOLSIZE,AllocReg(REGEES,top+1,tboolean));
	lab1 := NewLabel;
	lab2 := NewLabel;
	Op(jumpnames[nextjump]); Lab(lab1); L;
	Op('clrl'); Opnd(top); L;
	Op('jbr'); Lab(lab2); L;
	Lab(lab1); C(':'); L;
	Op('movl'); S('$1'); X; Opnd(top); L;
	Lab(lab2); C(':'); L;
	didit := FALSE;
	nextjump := jcnone;     { jump was handled }
    end;

    DoJumps := didit;
end;

{ MultiWordBinOp:  top=count }
procedure MultiWordBinOp{(op : ShortString; left, right : EESElement)};
var
    lab : LabelNumber;
begin
    if ees[top].kind <> EESDATA then begin
	Error('MultiWordBinOp: expected data for count');
    end;
    ees[top].constInt := ees[top].constInt - 1;
    Eval(top);
    Point(left);
    Point(right);
    lab := NewLabel;
    Lab(lab); C(':'); Op(op); Opnd(left); C('['); Opnd(top); C(']'); X;
	    Opnd(right); C('['); Opnd(top); C(']'); L;
    Op('sobgeq'); Opnd(top); X; Lab(lab); L;
    Pop(1);
end;

{ MultiWordUnOp:  top=count }
procedure MultiWordUnOp{(op : ShortString; opnd : EESElement)};
var
    lab : LabelNumber;
begin
    if ees[top].kind <> EESDATA then begin
	Error('MultiWordUnOp: expected data for count');
    end;
    ees[top].constInt := ees[top].constInt - 1;
    Eval(top);
    Point(opnd);
    lab := NewLabel;
    Lab(lab); C(':'); Op(op); Opnd(opnd); C('['); Opnd(top); C(']'); L;
    Op('sobgeq'); Opnd(top); X; Lab(lab); L;
    Pop(1);
end;

procedure Compare{(ptype : pcodetype; size : sizerange)};
var
    loop, exit : LabelNumber;
begin
    if ptype = tlongreal then begin
	Check(top,2*WORDSIZE);
	Check(top-1,2*WORDSIZE);
	Op('cmpd'); Opnd(top-1); X; Opnd(top); L;
	Pop(2);
    end else if size > WORDSIZE then begin
	PushConst(tinteger,WORDSIZE,size div WORDSIZE);
	Eval(top);
	Point(top-1);
	Point(top-2);
	loop := NewLabel;
	exit := NewLabel;
	Lab(loop); C(':'); 
	Op('cmpl'); Opnd(top-2); C('+'); X; Opnd(top-1); C('+'); L;
	Op('jneq'); Lab(exit); L;
	Op('sobgtr'); Opnd(top); X; Lab(loop); L;
	Pop(1);
	{ tricky: if size not a multiple, we are now pointing at remainder. }
	{ make pointer into variables and fix sizes, then recurse for compare }
	size := size - (size div WORDSIZE * WORDSIZE);
	if size > 0 then begin
	    ees[top].kind := EESVAR;
	    ees[top].size := size;
	    ees[top-1].kind := EESVAR;
	    ees[top-1].size := size;
	    Compare(ptype,size);
	end else begin
	    Pop(2);
	end;
	Lab(exit); C(':'); L;
    end else if size = BYTESIZE then begin
	Check(top,BYTESIZE);
	Check(top-1,BYTESIZE);
	Op('cmpb'); Opnd(top-1); X; Opnd(top); L;
	Pop(2);
    end else begin
	Check(top,WORDSIZE);
	Check(top-1,WORDSIZE);
	if ptype = treal then begin
	    Op('cmpf'); Opnd(top-1); X; Opnd(top); L;
	end else if (ees[top].kind = EESDATA) and (ees[top].dreg = NULLREG) and
	    (ees[top].constInt = 0)
	then begin
	    Op('tstl'); Opnd(top-1); L;
	end else begin
	    Op('cmpl'); Opnd(top-1); X; Opnd(top); L;
	end;
	Pop(2);
    end;
end;

procedure CallProcOp{(op : opcodes)};
var
    size : sizerange;
    numParams : integer;
begin
    size := Int(opd^[2]);
    numParams := Int(opd^[3]);
    if op = opcip then begin
	CallProc(op,opd11,size,numParams,{dummy parameter}opd^[1]);
    end else begin
	CallProc(op,opd11,size,numParams,opd^[4]);
    end;
end;

procedure CallProc{(op : opcodes; ctype : char; size : sizerange; numParams : integer; var procName : operandstring)};
var
    numParamWords : integer;
    i : integer;
    ptype : pcodetype;
    cipflag : integer;
begin
    if op = opcip then begin
	cipflag := 1;
    end else begin
	cipflag := 0;
    end;
    ptype := associatedType[ctype];
    SaveRegs(top-numParams-cipflag);
    numParamWords := 0;
    { push params on stack (backwards!) }
    for i := 1 to numParams do begin
	if ees[top].kind = EESADDR then begin
	    if (ees[top].sreg <> NULLREG) and
		    ((ees[top].sunits mod WORDSIZE) = 0)
	    then begin
		CheckRegs(top,WORDSIZE);
		Op('pushal'); Opnd(top); L;
	    end else begin
		CheckRegs(top,BYTESIZE);
		Op('pushab'); Opnd(top); L;
	    end;
	    numParamWords := numParamWords + 1;
	end else if ees[top].ptype = tlongreal then begin
	    { two-word real }
	    Check(top,2*WORDSIZE);
	    Op('movq'); Opnd(top); X; S('-(sp)'); L;
	    numParamWords := numParamWords + 2;
	end else if (ees[top].kind = EESDATA) or (ees[top].size <= WORDSIZE)
	then begin
	    { one word }
	    Check(top,WORDSIZE);
	    Op('pushl'); Opnd(top); L;
	    numParamWords := numParamWords + 1;
	end else begin
	    { must be a multi-word quantity.  Push address }
	    if ees[top].indirect then begin
		{ indirect, make it a variable address }
		ees[top].kind := EESVAR;
		ees[top].ptype := taddress;
		ees[top].size := ADDRSIZE;
		ees[top].indirect := false;
	    end;
	    { push parameter on stack }
	    Point(top);
	    Op('pushl'); R(ees[top].breg); L;
	    numParamWords := numParamWords + 1;
	end;
	Pop(1);
    end;
    if op = opcip then begin
	Point(top);
	Op('calls'); C('$'); I(numParamWords); X; Opnd(top); L;
	Pop(1);
    end else if op = opcup then begin
	Op('jsb'); C('_'); SO(procName); L;
	if numParamWords <> 0 then begin
	    Op('addl2'); C('$'); I(numParamWords*4); X; R(sp); L;
	end;
    end else begin
	Op('calls'); C('$'); I(numParamWords); X; C('_'); SO(procName); L;
    end;
    ClearDisp;
    if ptype <> tproc then begin
	PushReg(ptype,size,AllocReg(REGRETURN,top+1,ptype));
	if (size > WORDSIZE) and (ptype <> tlongreal) then begin
	    { reg is address of return value, make it base address }
	    ees[top].kind := EESVAR;
	    ees[top].breg := ees[top].dreg;
	    ees[top].dreg := NULLREG;
	    MakeMultiWordTemp(top);
	end;
    end;
    RestoreRegs(top);
end;

procedure MakeBaseAddress{(e:EESElement)};
var
    saveoffset : integer;
begin
    case ees[e].kind of
	EESVAR, EESDATA : begin
	    saveoffset := ees[e].constInt * BYTESIZE;   
	    ees[e].constInt := 0;
	    if ees[e].kind = EESVAR then begin
		if not ees[e].indirect and AddrIsTReg(e) then begin
		    { make it dreg: will become breg below }
		    ees[e].dreg := MemTReg(ees[e].addrOffset);
		    ClearAddress(e);
		end else begin
		    Eval(e);
		end;
	    end;
	    ees[e].breg := ees[e].dreg;
	    ees[e].dreg := NULLREG;
	    ees[e].kind := EESADDR;
	    ees[e].addrOffset := saveoffset;
	end;
	EESADDR : begin
	end;
    end;
end;

procedure MakeVariable{(e:EESElement)};
var
    constOffset : integer;
begin
    case ees[e].kind of
	EESADDR : begin
	    { make address into variable }
	    ees[e].kind := EESVAR;
	end;
	EESVAR : begin
	    if (ees[e].sreg <> NULLREG) or (ees[e].constInt <> 0) or
		    ees[e].indirect
	    then begin
		{ get value, make base address of variable }
		constOffset := ees[e].constInt * BYTESIZE;
		ees[e].constInt := 0;
		if not ees[e].indirect and AddrIsTReg(e) then begin
		    { make it dreg: will become breg below }
		    ees[e].dreg := MemTReg(ees[e].addrOffset);
		    ClearAddress(e);
		end else begin
		    Eval(e);
		end;
		ees[e].breg := ees[e].dreg;
		ees[e].dreg := NULLREG;
		ClearAddress(e);
		ees[e].addrOffset := constOffset;
		ees[e].kind := EESVAR;
	    end else begin
		{ use indirect addressing (no subscripting allowed) }
		ees[e].indirect := true;
	    end;
	end;
	EESDATA : begin
	    { get value, make base address of variable }
	    ees[e].breg := ees[e].dreg;
	    ees[e].dreg := NULLREG;
	    ClearAddress(e);
	    ees[e].addrOffset := ees[e].constInt * BYTESIZE;
	    ees[e].kind := EESVAR;
	    ees[e].constInt := 0;
	end;
    end;
end;

procedure Increment{(value : integer)};
begin
    { if need to store bit offset, make into address first }
    if (opd11 = 'a') and (value mod BYTESIZE <> 0) and
	    (ees[top].kind <> EESADDR)
    then begin
	MakeBaseAddress(top);
    end;
    case ees[top].kind of
	EESDATA : begin
	    if opd11 = 'a' then begin
		{ value is in bits }
		ees[top].constInt := ees[top].constInt + value div BYTESIZE;
	    end else begin
		ees[top].constInt := ees[top].constInt + value;
	    end;
	end;
	EESVAR : begin
	    if ees[top].indirect then begin
		Eval(top);
	    end;
	    if opd11 = 'a' then begin
		{ value is in bits }
		ees[top].constInt := ees[top].constInt + value div BYTESIZE;
	    end else begin
		ees[top].constInt := ees[top].constInt + value;
	    end;
	end;
	EESADDR : begin
	    if ees[top].indirect then begin
		Eval(top);
	    end;
	    ees[top].addrOffset := ees[top].addrOffset + value;
	end;
    end;
end;

procedure SetConst{(size : sizerange; e : EESElement)};
const
    BITSPERSETWORD = 16;
var
    value: array [1..MAXSETWORDS] of integer;
    word, bit, index: integer;
    numbits : integer;
    constlab : integer;
begin
    for word := 1 to MAXSETWORDS do value[word] := 0;
    numbits := Int(opd^[3]);
    if numbits > MAXSETSIZE then begin
	Error('Too many bits in set');
    end;
    for index:=0 to numbits-1 do begin
	if index mod BITSPERSETWORD = 0 then bit := 1 else bit := 2 * bit;
	if (opd^[4][index+1]='1') then begin
	    value[1+index div BITSPERSETWORD] :=
			value[1+index div BITSPERSETWORD] + bit;
	end;
    end;
    constlab := currentConstant;
    currentConstant := currentConstant + 1;
    Op('.data'); I(1); L;
    C('k'); I(constlab); C(':'); L;
    for word:=1 to (size+BITSPERSETWORD-1) div BITSPERSETWORD do begin
	Op('.word'); I(value[word]); L;
    end;
    Op('.text'); L;
    ees[e].addrMemType := 'k';
    ees[e].addrBlock := constlab;
end;

{ TwoOrThree:
    if dest is data and not constant, use 2 operand form
	op2  source,dest
    if not, allocate reg, use 3 operand form, and make dest data
	op3  source,dest,r
}
procedure TwoOrThree{(op:ShortString; source, dest : EESElement; ptype : pcodetype; size : sizerange)};
var
    r : Reg;
    i : integer;
    nsize : sizerange;
    dostore : boolean;
begin
    i := 1;
    while op[i] <> 'N' do begin
	i := i + 1;
    end;
    Check(source,size);
    Check(dest,size);
    if (ees[dest].kind = EESDATA) and ActiveReg(ees[dest].dreg) then begin
	op[i] := '2';
	Op(op); Opnd(source); X; Opnd(dest); L;
    end else begin
	dostore := false;
	{ look ahead to see if next thing is a store }
	if nopcode in [opstr, opsro, opstn] then begin
	    { size must match, to avoid complications }
	    nsize := Int(nopd^[2]);
	    if nsize = size then begin
		dostore := true;
	    end;
	end;
	if dostore then begin
	    { make store be current instruction }
	    Advance;
	    PreRead;
	    Push(EESVAR);
	    ees[top].size := size;
	    ees[top].ptype := ptype;
	    { get address }
	    if opcode = opsro then begin
		ees[top].addrLevel := 0;
		ees[top].addrMemType := opd^[3][1];
		ees[top].addrOffset := Int(opd^[4]);
		ees[top].addrBlock := Int(opd^[5]);
	    end else if opcode in [opstr,opstn] then begin
		ees[top].addrLevel := curlev - Int(opd^[3]);
		ees[top].addrMemType := opd^[4][1];
		ees[top].addrOffset := Int(opd^[5]);
		ees[top].addrBlock := Int(opd^[6]);
	    end else begin
		Error('TwoOrThree: store is not str or sro');
	    end;
	    Check(top,size);
	    op[i] := '3';
	    Op(op); Opnd(source); X; Opnd(dest); X; Opnd(top); L;
	    if opcode = opstn then begin
		{ as in stn: pop original and leave T on stack }
		SwapEES(dest,top);
		Pop(1);
	    end else begin
		Pop(2);	{ as in store: one for the result, one for the address }
	    end;
	end else begin
	    r := AllocReg(REGEES,dest,ptype);
	    op[i] := '3';
	    Op(op); Opnd(source); X; Opnd(dest); X; R(r); L;
	    FreeReg(ees[dest].breg);
	    FreeReg(ees[dest].sreg);
	    ClearAddress(dest);
	    ees[dest].kind := EESDATA;
	    ees[dest].size := size;
	    ees[dest].ptype := ptype;
	    ees[dest].dreg := r;
	    ees[dest].constInt := 0;
	end;
    end;
end;

procedure PushConst{(ptype:pcodetype; size:sizerange;value:integer)};
begin
    Push(EESDATA);
    ees[top].ptype := ptype;
    ees[top].size := size;
    ees[top].constInt := value;
end;

procedure PushReg{(ptype:pcodetype; size:sizerange;dreg:Reg)};
begin
    Push(EESDATA);
    ees[top].ptype := ptype;
    ees[top].size := size;
    ees[top].dreg := dreg;
end;
