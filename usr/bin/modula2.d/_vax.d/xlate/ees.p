(*#@(#)ees.p	4.1	Ultrix	7/17/90 *)
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
$Header: ees.p,v 1.4 84/05/19 11:32:44 powell Exp $
 ****************************************************************************)
#include "globals.h"

type
    RegRecord = record
	suitable : set of RegState;
	regGroup : (RGSINGLE, RGPAIR1, RGPAIR2);
	maskbit : integer;
	case state : RegState of
	    REGDISP : (level : Level; param : boolean; inUse : boolean);
	    REGEES : (eesElement : EESElement);
    end;

    IntRegRec = record case boolean of
	true : (int : integer);
	false : (reg : Reg);
    end;

var
    regTable : array [Reg] of RegRecord;

function ActiveReg{(r : Reg) : boolean};
begin
    ActiveReg := not (r in [NULLREG,rt0..rt9]);
end;

function MemTReg{(offset : integer) : Reg};
var
    ir : IntRegRec;
begin
    ir.int := ord(rt0) + offset div WORDSIZE;
    MemTReg := ir.reg;
end;

function TRegReg{(r : Reg) : Reg};
var
    ir : IntRegRec;
begin
    ir.int := ord(firsttreg) + ord(r) - ord(rt0);
    TRegReg := ir.reg;
end;

procedure ClearAddress{(e:EESElement)};
begin
    with ees[e] do begin
	{ees[e].}addrMemType := ' ';
	{ees[e].}addrOffset := 0;
	{ees[e].}addrBlock := 0;
	{ees[e].}addrLevel := 0;
	{ees[e].}indirect := false;
    end;
end;

procedure ClearEES{(e : EESElement)};
begin
    with ees[e] do begin
	{ees[e].}kind := EESDATA;
	{ees[e].}dreg := NULLREG;
	{ees[e].}breg := NULLREG;
	{ees[e].}sreg := NULLREG;
	{ees[e].}sunits := 0;
	{ees[e].}constInt := 0;
	{ees[e].}ptype:=tundefined;
	{ees[e].}size:=0;
	ClearAddress(e);
	{ees[e].}smemoffset:=0;
	{ees[e].}smemsize:=0;
	{ees[e].}inUse:=false;
	{ees[e].}indirect:=false;
    end;
end;

procedure Push{(kind : EESKind)};
begin
    if top >= EESSTACKSIZE then begin
	Error('Push: top > EESSTACKSIZE');
    end else begin
	top:=top+1;
	ClearEES(top);
	ees[top].kind := kind;
	ees[top].inUse := true;
    end;
end;

procedure Pop{(num:integer)};
var
    e : EESElement;
begin
    if (top<num) then begin
	Error('Pop: top < num');
    end else begin
	for e := top downto top-num+1 do begin
	    if ees[e].kind = EESDATA then begin
		FreeReg(ees[e].dreg);
	    end else if ees[e].kind in [EESADDR,EESVAR] then begin
		FreeReg(ees[e].sreg);
		FreeReg(ees[e].breg);
	    end;
	    if ees[e].smemsize <> 0 then begin
		FreeStack(ees[e].smemoffset,ees[e].smemsize);
	    end;
	    ClearEES(top);
	end;
	top := top - num;
    end;
end;

procedure InitEES;
var
    e : EESElement;
begin
    for e := NUMTEMPS to EESSTACKSIZE do begin
	ClearEES(e);
    end;
    top := 0;
end;

procedure InitReg;
var
    r, rr : Reg;
    mask : integer;
begin
    for r := NULLREG to LASTVREG do begin
	regTable[r].suitable := [];
	regTable[r].state := REGALLOC;
	regTable[r].inUse := false;
	regTable[r].regGroup := RGSINGLE;
	if r in [r6..r11] then begin
	    mask := 32;
	    for rr := r6 to r do begin
		mask := mask + mask;
	    end;
	    regTable[r].maskbit := mask;
	end else begin
	    regTable[r].maskbit := 0;
	end;
    end;
    regTable[RETURNREG].suitable := [REGRETURN];
    regTable[RETURNREG].state := REGFREE;
    for r := FIRSTREG to LASTREG do begin
	regTable[r].suitable := [REGTEMP,REGDISP,REGEES];
	regTable[r].state := REGFREE;
    end;
    for r := FIRSTVREG to LASTVREG do begin
	regTable[r].suitable := [REGEES];
	regTable[r].state := REGFREE;
    end;
end;

procedure StealReg(stealr, replacer : Reg);
begin
    if regTable[stealr].state in [REGDISP,REGFREE] then begin
	{ don't need to steal it }
    end else if regTable[stealr].regGroup <> RGSINGLE then begin
	if (regTable[stealr].regGroup <> RGPAIR1)
		or (regTable[stealr].regGroup <> RGPAIR2)
	then begin
	    Error('StealReg: error in reg pairing');
	end;
	ees[regTable[stealr].eesElement].dreg := replacer;
	{ steal reg pair, moving value to virtual reg }
	Op('movq'); R(stealr); X; R(replacer); L;
	regTable[replacer].state := regTable[stealr].state;
	regTable[replacer].eesElement := regTable[stealr].eesElement;
	regTable[replacer].regGroup := regTable[stealr].regGroup;
	regTable[succ(replacer)].state := regTable[succ(stealr)].state;
	regTable[succ(replacer)].eesElement := regTable[succ(stealr)].eesElement;
	regTable[succ(replacer)].regGroup := regTable[succ(stealr)].regGroup;
	{ free second reg of stolen pair }
	regTable[succ(stealr)].state := REGFREE;
	regTable[stealr].regGroup := RGSINGLE;
	regTable[succ(stealr)].regGroup := RGSINGLE;
    end else begin
	if ees[regTable[stealr].eesElement].dreg = stealr then begin
	    ees[regTable[stealr].eesElement].dreg := replacer;
	end else if ees[regTable[stealr].eesElement].breg = stealr
	then begin
	    ees[regTable[stealr].eesElement].breg := replacer;
	end else if ees[regTable[stealr].eesElement].sreg = stealr
	then begin
	    ees[regTable[stealr].eesElement].sreg := replacer;
	end else begin
	    Error('StealReg: could not find register usage');
	end;
	{ steal reg, moving value to virtual reg }
	Op('movl'); R(stealr); X; R(replacer); L;
	regTable[replacer].state := regTable[stealr].state;
	regTable[replacer].eesElement := regTable[stealr].eesElement;
    end;
end;

{ AllocReg:  allocate a register. }
{  Make sure it can handle specified type.  Allocate two regs if necessary }
{  Assign it to specified stack element }
{  NOTE:  All searching must be done from lowest to highest in order to handle }
{	register pairs properly }
function AllocReg{(state : RegState; e : EESElement; ptype : pcodetype) : Reg};
var
    r, stealr : Reg;
    found, tworegs : boolean;
begin
    tworegs := ptype = tlongreal;
    if state = REGRETURN then begin
	{ return reg must be in special place }
	if (regTable[RETURNREG].state <> REGFREE) or
		(tworegs and (regTable[succ(RETURNREG)].state <> REGFREE))
	then begin
	    Error('AllocReg: return reg not available');
	    r := RETURNREG;
	end else begin
	    r := RETURNREG;
	end;
    end else begin
	{ look for a reg in normal ones }
        r := FIRSTREG;
	found := false;
	while (r <= lastreg) and not found do begin
	    if (regTable[r].state = REGFREE) and
		(not tworegs or (regTable[succ(r)].state = REGFREE))
	    then begin
		found := true;
	    end else begin
		r := succ(r);
	    end;
	end;
	if not found then begin
	    { look for an unused display reg }
	    r := FIRSTREG;
	    found := false;
	    while (r <= lastreg) and not found do begin
		if ((regTable[r].state = REGDISP) and not regTable[r].inUse) or
			(regTable[r].state = REGFREE)
		then begin
		    if not tworegs then begin
			found := true;
		    end else if r = lastreg then begin
			{ can't split value between regs and memory }
		    end else if (regTable[succ(r)].state = REGFREE) or
			((regTable[succ(r)].state = REGDISP) and
			    not regTable[succ(r)].inUse)
		    then begin
			found := true;
		    end;
		end;
		if not found then begin
		    r := succ(r);
		end;
	    end;
	end;
	if not found then begin
	    { didn't find a real reg, allocate a virtual one }
	    r := FIRSTVREG;
	    found := false;
	    while (r <= LASTVREG) and not found do begin
		{ always make sure next reg is available, in case we need it }
		{   in order to steal the first of a pair }
		if (regTable[r].state = REGFREE) and
			(regTable[succ(r)].state = REGFREE)
		then begin
		    found := true;
		end else begin
		    r := succ(r);
		end;
	    end;
	    if not found then begin
		Error('AllocReg: virtual reg not available');
	    end else if state in [REGREG, REGDISP] then begin
		{ need a real reg, find one to steal }
		if tworegs then begin
		    Error('AllocReg: tworegs and REGREG');
		end;
		stealr := FIRSTREG;
		found := false;
		while (stealr <= lastreg) and not found do begin
		    if regTable[stealr].state = REGEES then begin
			found := true;
		    end else begin
			stealr := succ(stealr);
		    end;
		end;
		if not found then begin
		    Error('AllocReg: could not steal reg');
		end else begin
		    StealReg(stealr,r);
		    r := stealr;
		end;
	    end;
	end;
    end;
    regTable[r].state := state;
    regTable[r].eesElement := e;
    if regTable[r].maskbit <> 0 then begin
	if not odd(regmask div regTable[r].maskbit) then begin
	    regmask := regmask + regTable[r].maskbit;
	end;
    end;
    if tworegs then begin
	regTable[succ(r)].state := state;
	regTable[succ(r)].eesElement := e;
	if regTable[succ(r)].maskbit <> 0 then begin
	    if not odd(regmask div regTable[succ(r)].maskbit) then begin
		regmask := regmask + regTable[succ(r)].maskbit;
	    end;
	end;
	regTable[r].regGroup := RGPAIR1;
	regTable[succ(r)].regGroup := RGPAIR2;
    end;
    AllocReg := r;
end;

procedure FreeReg{(var r : Reg)};
begin
    if not ActiveReg(r) then begin
	r := NULLREG;
    end else if regTable[r].state = REGDISP then begin
	Error('FreeReg REGDISP');
    end else if regTable[r].regGroup = RGSINGLE then begin
	regTable[r].state := REGFREE;
	regTable[r].eesElement := 0;
	r := NULLREG;
    end else begin
	if regTable[r].regGroup <> RGPAIR1 then begin
	    Error('FreeReg: error in register pairing');
	end;
	regTable[r].state := REGFREE;
	regTable[r].regGroup := RGSINGLE;
	regTable[r].eesElement := 0;
	regTable[succ(r)].state := REGFREE;
	regTable[succ(r)].regGroup := RGSINGLE;
	regTable[succ(r)].eesElement := 0;
	r := NULLREG;
    end;
end;

procedure DumpEES;
var i : integer;
begin
    writeln(output,'# EES: top=',top:1);
    for i:=top downto NUMTEMPS do begin
	if (i >= 1) or (ees[i].inUse) then begin
	    writeln(output,'# ees[',i:1,']', ', type=',ees[i].ptype:1,
		', size=',ees[i].size:1, ', kind=',ees[i].kind:1);
	    writeln(output,'#  constInt=', ees[i].constInt:1,
		', dreg=',regString[ees[i].dreg]:regStringSize[ees[i].dreg],
		', breg=',regString[ees[i].breg]:regStringSize[ees[i].breg],
		', sreg=',regString[ees[i].sreg]:regStringSize[ees[i].sreg],
		', sunits=',ees[i].sunits:1);
	    writeln(output,'#  addrLevel=',ees[i].addrLevel:1,
		', addrMemType=',ees[i].addrMemType:1,
		', addrOffset=',ees[i].addrOffset:1,
		', addrBlock=',ees[i].addrBlock:1);
	    writeln(output,'#  indirect=',ees[i].indirect:1,
		', smemoffset=',ees[i].smemoffset:1,
		', smemsize=',ees[i].smemsize:1,
		', inUse=',ees[i].inUse:1);
	end;
    end;
end;            { end of dumpeesstack }

procedure DumpReg;
var
    r : Reg;
begin
    writeln(output,'# Registers');
    for r := RETURNREG to LASTREG do begin
	write(output,'#',regString[r],' ',regTable[r].state);
	if regTable[r].state = REGDISP then begin
	    writeln(output,'l=',regTable[r].level:1,',iu=',regTable[r].inUse:1,
		'p=',regTable[r].param:1);
	end else if regTable[r].state = REGEES then begin
	    writeln(output,' ',regTable[r].eesElement:1,
		', regGroup=',regTable[r].regGroup:1);
	end else begin
	    writeln(output);
	end;
    end;
end;

function AllocDisplay{(level : integer; mt : char) : Reg};
var
    r : Reg;
    found : boolean;
begin
    if level < 0 then begin
	Error('AllocDisplay: level < 0');
    end;
    found := false;
    r := FIRSTREG;
    while not found and (r <= lastreg) do begin
	if regTable[r].state = REGDISP then begin
	    if (regTable[r].level = level) and
		(regTable[r].param = (mt = 'p'))
	    then begin
		found := true;
		regTable[r].inUse := true;
	    end;
	end;
	if not found then begin
	    r := succ(r);
	end;
    end;
    if not found then begin
	r := AllocReg(REGDISP,0,taddress);
	regTable[r].level := level;
	regTable[r].param := mt = 'p';
	regTable[r].inUse := true;
	Op('movl'); S('_runtime__display+'); I(level*4); X; R(r); L;
	if mt = 'p' then begin
	    Op('movl'); I(APOFF); C('('); R(r); C(')'); X; R(r); L;
	end;
    end;
    AllocDisplay := r;
end;

function DispReg{(level : integer; mt : char) : Reg};
var
    r : Reg;
    found : boolean;
begin
    if level < 1 then begin
	Error('DispReg: level < 1');
    end;
    found := false;
    r := FIRSTREG;
    if level = curlev then begin
	found := true;
	if mt = 'p' then begin
	    r := ap;
	end else begin
	    r := fp;
	end;
    end;
    while not found and (r <= lastreg) do begin
	if regTable[r].state = REGDISP then begin
	    found := (regTable[r].level = level)
			and (regTable[r].param = (mt = 'p'));
	end;
	if not found then begin
	    r := succ(r);
	end;
    end;
    if not found then begin
	Error('DispReg: reg not found');
    end;
    DispReg := r;
end;

procedure NeedDisp{(level : integer; mt : char)};
var
    r : Reg;
begin
    if level < 0 then begin
	Error('NeedDisp: level < 0');
    end;
    if (level >= 0) and (level < curlev) then begin
	r := AllocDisplay(level,mt);
    end;
end;

procedure FreeDisp;
var
    r : Reg;
begin
    for r := FIRSTREG to lastreg do begin
	if regTable[r].state = REGDISP then begin
	    regTable[r].inUse := false;
	end;
    end;
end;

procedure ClearDisp;
var
    r : Reg;
begin
    for r := FIRSTREG to lastreg do begin
	if regTable[r].state = REGDISP then begin
	    regTable[r].state := REGFREE;
	end;
    end;
end;

procedure ClearReg;
begin
    InitReg;
end;

procedure Registerize{(var r : Reg)};
var
    realr : Reg;
begin
    if r in [rt0..rt9] then begin
	{ nothing to do }
    end else begin
	if r in [RETURNREG..LASTREG] then begin
	    regTable[r].state := REGREG;
	    if regTable[r].regGroup = RGPAIR1 then begin
		if regTable[succ(r)].regGroup <> RGPAIR2 then begin
		    Error('Registerize: bad register pairing');
		end;
		regTable[succ(r)].state := REGREG;
	    end;
	end else if regTable[r].regGroup <> RGSINGLE then begin
	    realr := AllocReg(REGREG,0,tlongreal);
	    Op('movq'); R(r); X; R(realr); L;
	    FreeReg(r);
	    r := realr;
	end else begin
	    realr := AllocReg(REGREG,0,taddress);
	    Op('movl'); R(r); X; R(realr); L;
	    FreeReg(r);
	    r := realr;
	end;
	if regTable[r].state <> REGREG then begin
	    Error('Registerize: failed');
	end;
    end;
end;

procedure SaveRegs{(below : EESElement)};
var
    e : EESElement;
begin
    for e := below downto 1 do begin
	if ActiveReg(ees[e].sreg) then begin
	    Op('pushl'); R(ees[e].sreg); L;
	    FreeReg(ees[e].sreg);
	    ees[e].sreg := SAVEDREG;
	end;
	if ActiveReg(ees[e].breg) then begin
	    Op('pushl'); R(ees[e].breg); L;
	    FreeReg(ees[e].breg);
	    ees[e].breg := SAVEDREG;
	end;
	if ActiveReg(ees[e].dreg) then begin
	    if regTable[ees[e].dreg].regGroup <> RGSINGLE then begin
		Op('pushl'); R(succ(ees[e].dreg)); L;
		Op('pushl'); R(ees[e].dreg); L;
		FreeReg(ees[e].dreg);
		ees[e].dreg := SAVED2REG;
	    end else begin
		Op('pushl'); R(ees[e].dreg); L;
		FreeReg(ees[e].dreg);
		ees[e].dreg := SAVEDREG;
	    end;
	end;
    end;
    for e := -1 downto NUMTEMPS do begin
	if ees[e].inUse then begin
	    Error('Temp in use on SaveRegs');
	end;
    end;
end;

procedure RestoreRegs{(below : EESElement)};
var
    e : EESElement;
begin
    for e := 1 to below do begin
	if ees[e].sreg = SAVEDREG then begin
	    ees[e].sreg := AllocReg(REGEES,e,taddress);
	    Op('movl'); S('(sp)+'); X; R(ees[e].sreg); L;
	end;
	if ees[e].breg = SAVEDREG then begin
	    ees[e].breg := AllocReg(REGEES,e,taddress);
	    Op('movl'); S('(sp)+'); X; R(ees[e].breg); L;
	end;
	if ees[e].dreg = SAVEDREG then begin
	    ees[e].dreg := AllocReg(REGEES,e,tinteger);
	    Op('movl'); S('(sp)+'); X; R(ees[e].dreg); L;
	end else if ees[e].dreg = SAVED2REG then begin
	    ees[e].dreg := AllocReg(REGEES,e,tlongreal);
	    Op('movq'); S('(sp)+'); X; R(ees[e].dreg); L;
	end;
    end;
end;

{ MoveReg: reassign reg r to element e }
procedure MoveReg{(e : EESElement; r : Reg)};
begin
    if ActiveReg(r) then begin
	regTable[r].eesElement := e;
	if regTable[r].regGroup <> RGSINGLE then begin
	    if regTable[r].regGroup <> RGPAIR1 then begin
		Error('MoveReg: error in register pairing');
	    end;
	    regTable[succ(r)].eesElement := e;
	end;
    end;
end;

procedure SwapEES{(a,b : EESElement)};
var
    tmp : EESRecord;
begin
    { reassign regs }
    MoveReg(a,ees[b].dreg);
    MoveReg(a,ees[b].breg);
    MoveReg(a,ees[b].sreg);
    MoveReg(b,ees[a].dreg);
    MoveReg(b,ees[a].breg);
    MoveReg(b,ees[a].sreg);
    tmp := ees[a];
    ees[a] := ees[b];
    ees[b] := tmp;
end;
