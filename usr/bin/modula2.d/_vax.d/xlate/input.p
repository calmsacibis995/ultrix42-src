(*#@(#)input.p	4.1	Ultrix	7/17/90 *)
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
#include "input.h"
#include "util.h"

procedure Advance;
var
    j : integer;
    topd : opdptr;
    topdsizes : opdsizeptr;
begin
    line := line + 1;
    pclbsize := npclbsize;
    for j := 1 to pclbsize+1 do begin
	pclabel[j] := npclabel[j];
    end;
    opcode := nopcode;
    opcodestr := nopcodestr;
    opdcount := nopdcount;
    topd := opd;
    opd := nopd;
    nopd := topd;
    topdsizes := opdsizes;
    opdsizes := nopdsizes;
    nopdsizes := topdsizes;
    opd11 := opd^[1][1];
end;

procedure PreRead;
var
    i : integer;
begin
    if not EOF then begin
	if nextchar = '#' then begin
	    npclbsize := 0;
	    npclabel[1] := ' ';
	    nopcode := opcmt;
	    nopcodestr := '#   ';
	    i := 0;
	    while nextchar <> chr(10) do begin
		i := i + 1;
		nopd^[1][i] := nextchar;
#include "readch.i"
	    end;
#include "readch.i"
	    nopdsizes^[1] := i;
	    nopdcount := 1;
	end else begin
	    ReadLabel(npclabel,npclbsize);
	    ReadOpcode(nopcode,nopcodestr);
	    ReadOperands(nopd^,nopdsizes^,nopdcount);
	end;
    end;
end;

procedure ReadLabel{(var pclabel:operandstring; var pclbsize : integer)};
begin
    pclbsize:=0;
    while (nextchar<>TAB) and (nextchar<>' ') do begin
	pclbsize:= pclbsize+1;
	if pclbsize < operandsize then begin
	    pclabel[pclbsize] := nextchar;
#include "readch.i"
	end; 
    end;
    pclabel[pclbsize+1] := ' ';
end;            { end of readlabel }

var
    numcalls, numprobes : integer;

procedure ReadOpcode{(var opcode:opcodes; var opcodestr: opcodestring)};
var
    i, tableIndex : integer;
    done, found : boolean;
begin
    numcalls := numcalls + 1;
    { read opcode values into opcodestr in string form }
    while (nextchar=TAB) or (nextchar=' ') do begin
#include "readch.i"
    end;
    for i := 1 to opcodefieldsize-1 do begin
	opcodestr[i] := nextchar;
#include "readch.i"
    end;
    opcodestr[opcodefieldsize] := ' ';

{ look up the opcode in the hash table }

{ hash function }
    tableIndex := (ord(opcodestr[1]) * 10 + ord(opcodestr[2]) * 25
		    + ord(opcodestr[3]) * 64) mod (MAXTABLESIZE+1);
    found := hashTable[tableIndex].oper = opcodestr;
    done := found;
    while not done do begin
	tableIndex := (tableIndex + 1) mod (MAXTABLESIZE+1);
	found := hashTable[tableIndex].oper = opcodestr;
	done := found or (hashTable[tableIndex].oper = '    ');
	numprobes := numprobes + 1;
    end;
    if found then begin
	  opcode := hashTable[tableIndex].code;
    end else begin
	Error('Opcode not found');
	opcode := opzzz;
    end;
end;

procedure ReadOperand(var opd : operandstring; var opdsize : integer);
var
    k : integer;
begin
    k:=0;
    if (nextchar='''') then begin
	{ read in a string, ends with ', watch for '' }
	repeat
	    repeat
		k:=k+1;
		opd[k] := nextchar;
#include "readch.i"
	    until nextchar='''';
	    k:=k+1;
	    opd[k] := nextchar;
#include "readch.i"
	until nextchar<>'''';
    end else begin
	{ not a string operand }
	while (nextchar<>opdsep) and (nextchar <> chr(10)) do begin
	    k:=k+1;
	    opd[k] := nextchar;
#include "readch.i"
	end;
    end;
    if (checkrequested and (k>=operandsize)) then begin
	Error('rdoperands: operand too long');
    end;
    opd[k+1] := ' ';  
    opdsize:=k;
end;

procedure ReadOperands{(var opd : opdtype; var opdsizes : opdsizearray; var opdcount: integer)};
var
    linefinished : boolean;
begin
    opdcount:=0;
    linefinished := false;
    { skip blanks }
    while (nextchar=' ') or (nextchar=TAB) do begin
#include "readch.i"
    end;
    while (nextchar <> chr(10)) and not linefinished do begin
	opdcount := opdcount + 1 ;
	ReadOperand(opd[opdcount],opdsizes[opdcount]);
	{ skip operand separator }
	if (nextchar=opdsep) then begin
#include "readch.i"
	end else begin
	    linefinished := true;
	end;
    end;
    { flush rest of line }
    if not EOF then begin
	while nextchar <> chr(10) do begin
#include "readch.i"
	end;
#include "readch.i"
    end;
end;
