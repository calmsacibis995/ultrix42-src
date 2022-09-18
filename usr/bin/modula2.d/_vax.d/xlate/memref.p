(*#@(#)memref.p	4.1	Ultrix	7/17/90 *)
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

function AddrIsTReg{(e:EESElement) : boolean};
begin
    AddrIsTReg := (ees[e].addrBlock = curblockid) and
	    (ees[e].addrOffset < maxtoffset) and (ees[e].addrMemType = 't');
end;

function IsOnlyBaseReg(e:EESElement) : boolean;
begin
    IsOnlyBaseReg := (ees[e].breg <> NULLREG) and (ees[e].sreg = NULLREG)
	    and (ees[e].addrOffset = 0) and (ees[e].addrMemType = ' ')
	    and not ees[e].indirect;
end;

{ DispRef:  write out display register or global data reference }
{  level := relative display level (0 := current)               }
{  if global level, then write name of global area 'xxxxxxxx'   }
{  if not, then write name of display register '(rN)'           }
procedure DispRef{(level:integer; mt : char)};
begin
    if (level < 0) or (level > curlev) then begin
	Error('DispRef: bad level');
    end else if level = curlev then begin
	{ relative level curlev means local }
	if mt = 'p' then begin
	    write(output,'(ap)');
	end else begin
	    write(output,'(fp)');
	end;
    end else begin
	{ write register name as '(rN)' where N is the  }
	{  display register for that level }
	write(output,'(',DispReg(level,mt):1,')');
    end;
end;

procedure TRegOpnd{(var r : Reg; e : EESElement)};
var
    newr : Reg;
begin
    if r in [rt0..rt9] then begin
	newr := AllocReg(REGEES,e,ees[e].ptype);
	R(r); X; R(newr);
	r := newr;
    end else begin
	R(r); X; R(r);
    end;
end;

procedure CheckTReg{(var r : Reg; e : EESElement)};
var
    newr : Reg;
begin
    if r in [rt0..rt9] then begin
	newr := AllocReg(REGEES,e,ees[e].ptype);
	Op('movl'); R(r); X; R(newr); S(' #T'); L;
	r := newr;
    end;
end;

procedure GenAddress{(e : EESElement)};
begin
    if ees[e].addrOffset mod BYTESIZE <> 0 then begin
	Error('GenAddress: not on a byte boundary');
    end;
    if AddrIsTReg(e) then begin
	R(MemTReg(ees[e].addrOffset));
    end else if (ees[e].addrMemType = ' ') and (ees[e].addrOffset = 0)
    then begin
	{ avoid writing 0 if base reg, but write 0 if there isn't one }
	if ees[e].breg = NULLREG then begin
	    C('0');
	end;
    end else begin
	C('(');
	case ees[e].addrMemType of
	    'p': begin C('p'); I(ees[e].addrBlock); C('+'); end;
	    't': begin C('t'); I(ees[e].addrBlock); C('+'); end;
	    'm': begin C('m'); I(ees[e].addrBlock); C('+'); end;
	    's': begin C('s'); I(ees[e].addrBlock); C('+'); end;
	    'c': begin WriteName(ees[e].addrBlock); C('+'); end;
	    'k': begin C('k'); I(ees[e].addrBlock); C('+'); end;
	    ' ': begin end;
	end;
	I(ees[e].addrOffset div BYTESIZE); C(')');
	if ees[e].addrLevel <> 0 then begin
	    DispRef(ees[e].addrLevel,ees[e].addrMemType);
	end;
    end;
    if ees[e].breg <> NULLREG then begin
	C('('); R(ees[e].breg); C(')');
    end;
    if ees[e].sreg <> NULLREG then begin
	if (ees[e].breg = NULLREG) and (ees[e].addrLevel = 0) and
		(ees[e].sunits <= BYTESIZE)
	then begin
	    C('('); R(ees[e].sreg); C(')');
	end else begin
	    C('['); R(ees[e].sreg); C(']');
	end;
    end;
end;

procedure Opnd{(e : EESElement)};
begin
    case ees[e].kind of
	EESDATA : begin
	    if ees[e].indirect then begin
		Error('Indirect on DATA');
	    end;
	    if ees[e].dreg <> NULLREG then begin
		R(ees[e].dreg);
	    end else begin
		C('$'); I(ees[e].constInt);
	    end;
	end;
	EESADDR : begin
	    if ees[e].indirect then begin
		Error('Indirect on ADDR');
	    end;
	    GenAddress(e);
	end;
	EESVAR : begin
	    if ees[e].indirect then begin
		if AddrIsTReg(e) then begin
		    C('('); GenAddress(e); C(')'); 
		end else begin
		    C('*'); GenAddress(e);
		end;
	    end else begin
		GenAddress(e);
	    end;
	end;
    end;
end;

procedure CheckSub{(e:EESElement; size:sizerange)};
var
    shift : integer;
begin
    if ees[e].sreg <> NULLREG then begin
	if (ees[e].sunits mod size <> 0) then begin
	    Error('CheckSub: Unsupported array element size');
	end;
	if ees[e].sunits = size then begin
	    { already fixed up }
	end else if ees[e].sunits > size then begin
	    shift := PowerOfTwo(ees[e].sunits div size);
	    if shift = 1 then begin
		Op('addl3'); R(ees[e].sreg); X; TRegOpnd(ees[e].sreg,e); L;
	    end else if shift > 0 then begin
		Op('ashl'); C('$'); I(shift); X; TRegOpnd(ees[e].sreg,e); L;
	    end else begin
		Op('mull3'); C('$'); I(ees[e].sunits div size); X;
			TRegOpnd(ees[e].sreg,e); L;
	    end;
	    ees[e].sunits := size;
	end else begin
	    Error('CheckSub: Array element size too small');
	    Op('divl2'); C('$'); I(size div ees[e].sunits); X;
		    R(ees[e].sreg); L;
	    ees[e].sunits := size;
	end;
    end;
end;

procedure CheckRegs{(e:EESElement; size:sizerange)};
var
    r : Reg;
begin
    if ees[e].addrLevel <> 0 then begin
	NeedDisp(ees[e].addrLevel,ees[e].addrMemType);
    end;
    if ees[e].breg <> NULLREG then begin
	if ees[e].addrLevel <> 0 then begin
	    { need base reg and a display reg }
	    Op('addl3'); R(DispReg(ees[e].addrLevel,ees[e].addrMemType)); X;
		TRegOpnd(ees[e].breg,e); L;
	    ees[e].addrLevel := 0;
	end;
	Registerize(ees[e].breg);
    end;
    r := ees[e].breg;
    if ees[e].sreg <> NULLREG then begin
	Registerize(ees[e].sreg);
	CheckSub(e,size);
	Registerize(ees[e].sreg);
	if ees[e].sreg = r then begin
	    Error('CheckRegs: stole its own reg');
	end;
    end;
end;

procedure AddConst(e : EESElement; r : Reg);
begin
    if ees[e].constInt = 1 then begin
	Op('incl'); R(r); L;
    end else if ees[e].constInt = -1 then begin
	Op('decl'); R(r); L;
    end else if ees[e].constInt > 0 then begin
	Op('addl2'); C('$'); I(ees[e].constInt); X; R(r); L;
    end else if ees[e].constInt < 0 then begin
	Op('subl2'); C('$'); I(-ees[e].constInt); X; R(r); L;
    end;
    ees[e].constInt := 0;
end;

function BaseReg(e : EESElement) : Reg;
begin
    if ees[e].breg <> NULLREG then begin
	BaseReg := ees[e].breg;
    end else begin
	BaseReg := DispReg(ees[e].addrLevel,ees[e].addrMemType);
    end;
end;

procedure MoveAddress(e : EESElement);
var
    basereg, offset : boolean;
begin
    if ees[e].indirect then begin
	CheckRegs(e,WORDSIZE);
	ees[e].indirect := false;
	Op('movl'); Opnd(e); X;
    end else begin
	basereg := (ees[e].breg <> NULLREG) or (ees[e].addrLevel <> 0);
	if not basereg and (ees[e].sreg <> NULLREG) and
		(ees[e].sunits = BYTESIZE)
	then begin
	    ees[e].breg := ees[e].sreg;
	    ees[e].sreg := NULLREG;
	    basereg := true;
	end;
	offset := (ees[e].addrMemType <> ' ') or (ees[e].addrOffset <> 0);
	if ees[e].sreg = NULLREG then begin
	    CheckRegs(e,BYTESIZE);
	    if basereg then begin
		if offset then begin
		    Op('movab'); GenAddress(e); X;
		end else begin
		    Op('movl'); R(BaseReg(e)); X;
		end;
	    end else begin
		if offset then begin
		    Op('movl'); C('$'); GenAddress(e); X;
		end else begin
		    Op('clrl');
		end;
	    end;
	end else if (ees[e].sunits mod WORDSIZE) <> 0 then begin
	    CheckRegs(e,BYTESIZE);
	    { byte subscripting involved }
	    if basereg then begin
		if offset then begin
		    Op('movab'); GenAddress(e); X;
		end else begin
		    Op('addl3'); R(BaseReg(e)); X; R(ees[e].sreg); X;
		end;
	    end else begin
		ees[e].breg := ees[e].sreg;
		ees[e].sreg := NULLREG;
		if offset then begin
		    Op('movab'); GenAddress(e); X;
		end else begin
		    Op('movl'); R(ees[e].breg); X;
		end;
	    end;
	end else if ees[e].sunits = WORDSIZE then begin
	    CheckRegs(e,WORDSIZE);
	    { word subscripting involved }
	    Op('moval'); GenAddress(e); X;
	end else if ees[e].sunits = 2*WORDSIZE then begin
	    CheckRegs(e,WORDSIZE);
	    { double word subscripting involved }
	    Op('moval'); GenAddress(e); X;
	end else begin
	    { multi-word subscripting involved }
	    CheckRegs(e,BYTESIZE);
	    if basereg then begin
		if offset then begin
		    Op('movab'); GenAddress(e); X;
		end else begin
		    Op('addl3'); R(BaseReg(e)); X; R(ees[e].sreg); X;
		end;
	    end else begin
		ees[e].breg := ees[e].sreg;
		ees[e].sreg := NULLREG;
		if offset then begin
		    Op('movab'); GenAddress(e); X;
		end else begin
		    Op('movl'); R(ees[e].breg); X;
		end;
	    end;
	end;
    end;
end;

{
    The way this works:

    A stack element may be one of three things:
	DATA :	loaded or computed value
	ADDR :	address constant or expression
	VAR :	address constant or expression for a variable

    The three common routines to process these values are:
	Eval :	convert element to single register with no constant part
		if size <= WORDSIZE, make it DATA dreg
		if size > WORDSIZE, make it MultiWordTemp
	Check :	convert element to something that can appear as an operand
		if DATA, make it constant only or dreg only
		if ADDR, make it DATA dreg
		if VAR,
		    if size isn't right, do Eval
		    if size OK, fix subscript and be sure only one base reg
	Point :	like Eval, but assumes element is address and makes a var
		do Eval, if DATA dreg, make it breg
	Store :	store one element into another
		destination must be a VAR
		if size > WORDSIZE, copy from source to destination
		if sizes compatible for single instruction, do it
		if not, Eval source, then store
		
}

{ get value of e }
procedure Eval{(e:EESElement)};
var
    r : Reg;
    bitoff, mask : integer;
begin
    case ees[e].kind of
	EESDATA : begin
	    CheckRegs(e,ees[e].size);
	    { if no reg, allocate one }
	    if ees[e].dreg = NULLREG then begin
		r := AllocReg(REGEES,e,ees[e].ptype);
		Op('movl'); Opnd(e); X; R(r); L;
		ees[e].dreg := r;
	    end else begin
		AddConst(e,ees[e].dreg);
	    end;
	    ees[e].constInt := 0;
	end;
	EESADDR : begin
	    if IsOnlyBaseReg(e) then begin
		{ already just a base reg, just make it data reg }
		CheckTReg(ees[e].breg,e);
		ees[e].dreg := ees[e].breg;
		ees[e].breg := NULLREG;
	    end else begin
		{ compute address, put it in data reg }
		r := AllocReg(REGEES,e,taddress);
		MoveAddress(e); R(r); L;
		ClearAddress(e);
		FreeReg(ees[e].sreg);
		FreeReg(ees[e].breg);
		ees[e].dreg := r;
	    end;
	    ees[e].kind := EESDATA;
	    ees[e].constInt := 0;
	end;
	EESVAR : begin
	    { load variable }
	    if (ees[e].size > WORDSIZE) and (ees[e].ptype <> tlongreal)
	    then begin
		MakeMultiWordTemp(e);
	    end else begin
		CheckRegs(e,ees[e].size);
		r := AllocReg(REGEES,e,ees[e].ptype);
		{ get value in reg if less than or equal to a word }
		bitoff := ees[e].addrOffset mod BYTESIZE;
		if ees[e].ptype = tlongreal then begin
		    Op('movd'); Opnd(e); X; R(r); L;
		end else if (ees[e].size = WORDSIZE) and (bitoff = 0) then begin
		    if ees[e].constInt > 0 then begin
			Op('addl3'); C('$'); I(ees[e].constInt); X;
			ees[e].constInt := 0;
		    end else if ees[e].constInt < 0 then begin
			Op('subl3'); C('$'); I(-ees[e].constInt); X;
			ees[e].constInt := 0;
		    end else begin
			Op('movl');
		    end;
		    { ... }	Opnd(e); X; R(r); L;
		end else if (ees[e].size = BYTESIZE) and (bitoff = 0) then begin
		    if ees[e].ptype = tinteger then begin
			Op('cvtbl');
		    end else begin
			Op('movzbl');
		    end;
			Opnd(e); X; R(r); L;
		end else if (bitoff = 0) and (ees[e].sreg = NULLREG)
		    and (ees[e].ptype <> tinteger)
		then begin
			{ no subscripting, may be doing word operation on byte }
		    mask := power(2,ees[e].size);
		    if mask <> -maxint-1 then begin
			mask := -mask;
		    end;
		    Op('bicl3'); C('$'); I(mask); X; Opnd(e); X; R(r); L;
		end else begin
		    ees[e].addrOffset := ees[e].addrOffset div BYTESIZE * BYTESIZE;
		    CheckRegs(e,BYTESIZE); { instruction requires byte index }
		    if ees[e].ptype = tinteger then begin
			Op('extv');
		    end else begin
			Op('extzv');
		    end;
			C('$'); I(bitoff ); X; C('$');
			I(ees[e].size); X; Opnd(e); X; R(r); L;
		end;
		AddConst(e,r);
		{ now a data register }
		ees[e].kind := EESDATA;
		ees[e].constInt := 0;
		ees[e].dreg := r;
		if ees[e].ptype = tlongreal then begin
		    ees[e].size := 2*WORDSIZE;
		end else begin
		    ees[e].size := WORDSIZE;
		end;
		FreeReg(ees[e].sreg);
		FreeReg(ees[e].breg);
		ClearAddress(e);
	    end;
	end;
    end;
end;

{ ensure that e can be used as an operand }
procedure Check{(e:EESElement;size:sizerange)};
begin
    case ees[e].kind of
	EESDATA : begin
	    if ees[e].dreg <> NULLREG then begin
		if ees[e].size <> size then begin
		    if (ees[e].size = BYTESIZE) and (size = WORDSIZE) then begin
			if ees[e].ptype = tinteger then begin
			    Op('cvtbl');
			end else begin
			    Op('movzbl');
			end;
			R(ees[e].dreg); X; R(ees[e].dreg); L;
		    end;
		end;
		AddConst(e,ees[e].dreg);
	    end;
	end;
	EESADDR : begin
	    Eval(e);
	end;
	EESVAR : begin
	    if (ees[e].size <> size) or (ees[e].constInt <> 0) or
		(ees[e].addrOffset mod BYTESIZE <> 0)
	    then begin
		Eval(e);
	    end else begin
		CheckRegs(e,ees[e].size);
	    end;
	end;
    end;
end;

{ ensure that e can be used as a base address for a variable }
procedure Point{(e:EESElement)};
var
    r : Reg;
begin
    if (ees[e].kind = EESVAR) and (ees[e].size > WORDSIZE) then begin
	if IsOnlyBaseReg(e) then begin
	    { already just a base address }
	end else begin
	    r := AllocReg(REGEES,e,taddress);
	    MoveAddress(e); R(r); L;
	    { now a base register }
	    ees[e].kind := EESVAR;
	    ClearAddress(e);
	    FreeReg(ees[e].sreg);
	    FreeReg(ees[e].breg);
	    ees[e].breg := r;
	end;
    end else if (ees[e].kind = EESADDR) and AddrIsTReg(e) then begin
	{ address of t register is just the t reg }
	ees[e].kind := EESVAR;
	Error('Point: check address of t reg');
    end else begin
	Eval(e);
	if ees[e].kind = EESDATA then begin
	    ees[e].kind := EESVAR;
	    ees[e].breg := ees[e].dreg;
	    ees[e].dreg := NULLREG;
	    ClearAddress(e);
	end;
    end;
end;

procedure StoreWithConstWord(v,a:EESElement);
var
    k : integer;
begin
    { important special cases }
    k := ees[v].constInt;
    ees[v].constInt := 0;
    if (k = 0) then begin
	if (ees[v].kind = EESDATA) and (ees[v].dreg = NULLREG) then begin
	    { clear word }
	    Op('clrl'); Opnd(a); L;
	end else begin
	    { word move }
	    Op('movl'); Opnd(v); X; Opnd(a); L;
	end;
    end else if (ees[v].kind = EESDATA) and (ees[v].dreg = NULLREG) then begin
	{ constant move }
	Op('movl'); C('$'); I(k); X; Opnd(a); L;
    end else if (ees[v].kind = EESVAR) and (ees[a].kind = EESVAR)
	    and (ees[v].indirect=ees[a].indirect)
	    and (ees[v].addrOffset=ees[a].addrOffset)
	    and (ees[v].addrBlock=ees[a].addrBlock)
	    and (ees[v].addrMemType=ees[a].addrMemType)
	    and (ees[v].addrLevel=ees[a].addrLevel)
    	    and (ees[v].sreg = NULLREG) and (ees[a].sreg = NULLREG)
    	    and (ees[v].breg = NULLREG) and (ees[a].breg = NULLREG)
    then begin
	{ same variable! }
	if k = 1 then begin
	    Op('incl'); Opnd(v); L;
	end else if k = -1 then begin
	    Op('decl'); Opnd(v); L;
	end else if k > 0 then begin
	    Op('addl2'); C('$'); I(k); X; Opnd(v); L;
	end else if k < 0 then begin
	    Op('subl2'); C('$'); I(-k); X; Opnd(v); L;
	end;
    end else if k > 0 then begin
	Op('addl3'); C('$'); I(k); X; Opnd(v); X; Opnd(a); L;
    end else if k < 0 then begin
	Op('subl3'); C('$'); I(-k); X; Opnd(v); X; Opnd(a); L;
    end else begin
	Error('StoreWithConstWord: k=0?');
    end;
end;

procedure StoreWithConstByte(v,a:EESElement);
var
    k : integer;
begin
    { important special cases }
    k := ees[v].constInt;
    ees[v].constInt := 0;
    if (k = 0) then begin
	if (ees[v].kind = EESDATA) and (ees[v].dreg = NULLREG) then begin
	    { clear word }
	    Op('clrb'); Opnd(a); L;
	end else begin
	    { word move }
	    Op('movb'); Opnd(v); X; Opnd(a); L;
	end;
    end else if (ees[v].kind = EESDATA) and (ees[v].dreg = NULLREG) then begin
	{ constant move }
	Op('movb'); C('$'); I(k); X; Opnd(a); L;
    end else if (ees[v].kind = EESVAR) and (ees[a].kind = EESVAR)
	    and (ees[v].indirect=ees[a].indirect)
	    and (ees[v].addrOffset=ees[a].addrOffset)
	    and (ees[v].addrBlock=ees[a].addrBlock)
	    and (ees[v].addrMemType=ees[a].addrMemType)
	    and (ees[v].addrLevel=ees[a].addrLevel)
    	    and (ees[v].sreg = NULLREG) and (ees[a].sreg = NULLREG)
    	    and (ees[v].breg = NULLREG) and (ees[a].breg = NULLREG)
    then begin
	{ same variable! }
	if k = 1 then begin
	    Op('incb'); Opnd(v); L;
	end else if k = -1 then begin
	    Op('decb'); Opnd(v); L;
	end else if k > 0 then begin
	    Op('addb2'); C('$'); I(k); X; Opnd(v); L;
	end else if k < 0 then begin
	    Op('subb2'); C('$'); I(-k); X; Opnd(v); L;
	end;
    end else if k > 0 then begin
	Op('addb3'); C('$'); I(k); X; Opnd(v); X; Opnd(a); L;
    end else if k < 0 then begin
	Op('subb3'); C('$'); I(-k); X; Opnd(v); X; Opnd(a); L;
    end else begin
	Error('StoreWithConstByte: k=0?');
    end;
end;

procedure Store{(v,a:EESElement)};
var
    bitoffa, bitoffv, bitsize : integer;
    done : boolean;
    lab : LabelNumber;
begin
    done := false;
    if ees[a].kind <> EESVAR then begin
	Error('Store: address not a var');
    end;
    if ees[v].size <= WORDSIZE then begin
	{ make sure regs are OK }
	CheckRegs(a,ees[a].size);
	{ if destination is a variable, may be able to do it neatly }
	if ees[v].kind in [EESVAR,EESDATA] then begin
	    CheckRegs(v,ees[v].size);
	    bitoffa := ees[a].addrOffset mod BYTESIZE;
	    if ees[v].kind = EESDATA then begin
		bitoffv := 0;
	    end else begin
		bitoffv := ees[v].addrOffset mod BYTESIZE;
	    end;
	    if (bitoffa <> 0) or (bitoffv <> 0) then begin
		{ cannot handle as special case }
	    end else if (ees[a].size = WORDSIZE) and (ees[v].size = WORDSIZE)
	    then begin
		{ handle important special cases }
		StoreWithConstWord(v,a);
		done := true;
	    end else if (ees[a].size = BYTESIZE) and (ees[v].size = BYTESIZE)
	    then begin
		{ handle important special cases }
		StoreWithConstByte(v,a);
		done := true;
	    end else if ees[v].constInt <> 0 then begin
		{ cannot handle constant and different sizes, too }
	    end else if (ees[v].size = BYTESIZE) and (ees[a].size = WORDSIZE)
	    then begin
		{ byte to long, convert it }
		if ees[v].ptype = tinteger then begin
		    Op('cvtbl');
		end else begin
		    Op('movzbl');
		end;
		Opnd(v); X; Opnd(a); L;
		done := true;
	    end else if (ees[a].size = BYTESIZE) and (ees[v].size = WORDSIZE)
	    then begin
		{ long to byte, convert it }
		Op('cvtlb'); Opnd(v); X; Opnd(a); L;
		done := true;
	    end;
	end else if ees[v].kind = EESADDR then begin
	    { address is always a word and word aligned }
	    MoveAddress(v); Opnd(a); L;
	    done := true;
	end;
	if not done then begin
	    { couldn't do it the easy way, evaluate source }
	    Eval(v);
	    bitoffa := ees[a].addrOffset mod BYTESIZE;
	    if (ees[a].size = WORDSIZE) and (bitoffa = 0) then begin
		{ store it in a word }
		Op('movl'); Opnd(v); X; Opnd(a); L;
	    end else if (ees[a].size = BYTESIZE) and (bitoffa = 0) then begin
		{ store it in a byte }
		Op('cvtlb'); Opnd(v); X; Opnd(a); L;
	    end else begin
		ees[a].addrOffset := ees[a].addrOffset div BYTESIZE * BYTESIZE;
		CheckRegs(a,BYTESIZE); { instruction requires byte index }
		Op('insv'); Opnd(v); X; C('$'); I(bitoffa); X; C('$');
		    I(ees[a].size); X; Opnd(a); L;
	    end;
	end;
    end else if (ees[v].size = 2*WORDSIZE) and (ees[v].size = ees[a].size)
	    and (ees[v].kind in [EESVAR,EESDATA])
	    and (ees[v].sunits mod (2*WORDSIZE) = 0)
	    and (ees[a].sunits mod (2*WORDSIZE) = 0)
    then begin
	{ two word variable, do it neatly }
	{ make sure regs are OK }
	CheckRegs(v,ees[v].size);
	CheckRegs(a,ees[v].size);
	Op('movq'); Opnd(v); X; Opnd(a); L;
    end else begin
	{ multi-word, do a copy }
	Push(EESDATA);
	ees[top].size := WORDSIZE;
	ees[top].ptype := tinteger;
	ees[top].constInt := ees[v].size div WORDSIZE;
	Eval(top);
	Point(v);
	Point(a);
	lab := NewLabel;
	Lab(lab); C(':'); Op('movl'); Opnd(v); C('+'); X; Opnd(a); C('+'); L;
	Op('sobgtr'); Opnd(top); X; Lab(lab); L;
	Pop(1);
	bitsize := ees[v].size mod WORDSIZE;
	if bitsize <> 0 then begin
	    { some leftovers at the end }
	    { make vars of right size and recurse to move it }
	    ees[v].size := bitsize;
	    ees[a].size := bitsize;
	    ees[v].kind := EESVAR;
	    ees[v].kind := EESVAR;
	    Store(v,a);
	end;
    end;
end;

procedure ClearStack;
begin
    new(stackMem);
    stackMem^.offset := -1;
    stackMem^.size := 0;
    stackMem^.next := stackMem;
    stackMem^.prev := stackMem;
    stackMemSize := 0;
end;

procedure RemoveFromList(sm : StackMemNode);
begin
    sm^.next^.prev := sm^.prev;
    sm^.prev^.next := sm^.next;
    dispose(sm);
end;

function AllocStack(size : sizerange) : sizerange;
var
    sm : StackMemNode;
    found : boolean;
    offset : sizerange;
begin
    size := (size + WORDSIZE-1) div WORDSIZE;
    sm := stackMem^.next;
    found := false;
    while (sm <> stackMem) and not found do begin
	if size <= sm^.size then begin
	    found := true;
	end else begin
	    sm := sm^.next;
	end;
    end;
    if found then begin
	if size = sm^.size then begin
	    { take whole area }
	    offset := sm^.offset;
	    RemoveFromList(sm);
	end else begin
	    { take beginning, leave rest }
	    offset := sm^.offset;
	    sm^.offset := sm^.offset + size;
	    sm^.size := sm^.size - size;
	end;
    end else begin
	{ need to take more space }
	offset := stackMemSize;
	stackMemSize := stackMemSize + size;
    end;
    AllocStack := offset * WORDSIZE;
end;

procedure FreeStack{(offset, size : sizerange)};
var
    sm, nextsm : StackMemNode;
    found : boolean;
begin
    size := (size + WORDSIZE-1) div WORDSIZE;
    sm := stackMem^.next;
    found := false;
    while not found and (sm <> stackMem) do begin
	if (offset > sm^.offset) and (offset < sm^.offset+sm^.size) then begin
	    Error('FreeStack: stack screwed up');
	end;
	if (offset+size > sm^.offset) and (offset+size < sm^.offset+sm^.size)
	then begin
	    Error('FreeStack: stack screwed up');
	end;
	if (offset = sm^.offset+sm^.size) or (offset+size = sm^.offset)
		or (offset < sm^.offset)
	then begin
	    found := true;
	end else begin
	    sm := sm^.next;
	end;
	if offset+size = sm^.offset then begin
	    { add to beginning of area }
	    sm^.offset := sm^.offset - size;
	    sm^.size := sm^.size + size;
	end else if offset = sm^.offset+sm^.size then begin
	    { add to end of area }
	    sm^.size := sm^.size + size;
	    { check for plugging a hole }
	    if sm^.offset+sm^.size = sm^.next^.offset then begin
		sm^.size := sm^.size + sm^.next^.size;
		RemoveFromList(sm^.next);
	    end;
	end else begin
	    { no neighbors, add a new node }
	    nextsm := sm;
	    new(sm);
	    sm^.offset := offset;
	    sm^.size := size;
	    sm^.next := nextsm;
	    sm^.prev := nextsm^.prev;
	    sm^.prev^.next := sm;
	    nextsm^.prev := sm;
	end;
    end;
end;

procedure PushMultiWordTemp{(ptype:pcodetype;size : sizerange)};
var
    offset : sizerange;
begin
    Push(EESVAR);
    { allocate temp }
    offset := AllocStack(size);
    ees[top].smemoffset := offset;
    ees[top].smemsize := size;
    ees[top].size := size;
    ees[top].ptype := ptype;

    { make element point to temp }
    ees[top].addrLevel := curlev;
    ees[top].addrMemType := 's';
    ees[top].addrOffset := offset;
    ees[top].addrBlock := curblockid;
end;

procedure MakeMultiWordTemp{(e : EESElement)};
begin
    if ees[e].smemsize <> 0 then begin
	{ already a temp }
    end else begin
	{ get temp }
	PushMultiWordTemp(ees[e].ptype,ees[e].size);

	{ swap top and element }
	SwapEES(e,top);

	{ store old element into temp }
	Push(EESDATA);
	ees[top].ptype := tinteger;
	ees[top].size := WORDSIZE;
	ees[top].constInt := (ees[e].size + WORDSIZE-1) div WORDSIZE;
	MultiWordBinOp('movl',top-1,e);

	{ pop element }
	Pop(1);
    end;
end;
