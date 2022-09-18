(*#@(#)pcode.p	4.1	Ultrix	7/17/90 *)
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
$Header: pcode.p,v 1.5 84/05/19 11:44:15 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "pcode.h"
#include "decls.h"

const
    TAB = '	';	{ a tab character }
var
    typeChar : array [DataType] of char;

procedure InitPcode;
var
    t : Token;
begin
    pcodeStr[PCABS] := 'abs';
    pcodeStr[PCADD] := 'add';
    pcodeStr[PCAND] := 'and';
    pcodeStr[PCBGN] := 'bgn';
    pcodeStr[PCCAP] := 'cap';
    pcodeStr[PCCEP] := 'cep';
    pcodeStr[PCCHK] := 'chk';
    pcodeStr[PCCHR] := 'chr';
    pcodeStr[PCCIP] := 'cip';
    pcodeStr[PCCJP] := 'cjp';
    pcodeStr[PCCOM] := 'com';
    pcodeStr[PCCSP] := 'csp';
    pcodeStr[PCCTS] := 'cts';
    pcodeStr[PCCUP] := 'cup';
    pcodeStr[PCDEC] := 'dec';
    pcodeStr[PCDEF] := 'def';
    pcodeStr[PCDIF] := 'dif';
    pcodeStr[PCDIV] := 'div';
    pcodeStr[PCDSP] := 'dsp';
    pcodeStr[PCENT] := 'ent';
    pcodeStr[PCEQU] := 'equ';
    pcodeStr[PCEXI] := 'exi';
    pcodeStr[PCFJP] := 'fjp';
    pcodeStr[PCFLO] := 'flo';
    pcodeStr[PCFLT] := 'flt';
    pcodeStr[PCGEQ] := 'geq';
    pcodeStr[PCGRT] := 'grt';
    pcodeStr[PCINC] := 'inc';
    pcodeStr[PCIND] := 'ind';
    pcodeStr[PCINN] := 'inn';
    pcodeStr[PCINT] := 'int';
    pcodeStr[PCIOR] := 'ior';
    pcodeStr[PCIXA] := 'ixa';
    pcodeStr[PCLAB] := 'lab';
    pcodeStr[PCLAO] := 'lao';
    pcodeStr[PCLCA] := 'lca';
    pcodeStr[PCLDA] := 'lda';
    pcodeStr[PCLDC] := 'ldc';
    pcodeStr[PCLDO] := 'ldo';
    pcodeStr[PCLEQ] := 'leq';
    pcodeStr[PCLES] := 'les';
    pcodeStr[PCLOC] := 'loc';
    pcodeStr[PCLOD] := 'lod';
    pcodeStr[PCMAX] := 'max';
    pcodeStr[PCMIN] := 'min';
    pcodeStr[PCMOD] := 'mod';
    pcodeStr[PCMOV] := 'mov';
    pcodeStr[PCMST] := 'mst';
    pcodeStr[PCMUP] := 'mup';
    pcodeStr[PCMUS] := 'mus';
    pcodeStr[PCMVN] := 'mvn';
    pcodeStr[PCNAM] := 'nam';
    pcodeStr[PCNEG] := 'neg';
    pcodeStr[PCNEQ] := 'neq';
    pcodeStr[PCNEW] := 'new';
    pcodeStr[PCNOT] := 'not';
    pcodeStr[PCPAR] := 'par';
    pcodeStr[PCODD] := 'odd';
    pcodeStr[PCORD] := 'ord';
    pcodeStr[PCRET] := 'ret';
    pcodeStr[PCSAL] := 'sal';
    pcodeStr[PCSAV] := 'sav';
    pcodeStr[PCSDF] := 'sdf';
    pcodeStr[PCSGS] := 'sgs';
    pcodeStr[PCSML] := 'sml';
    pcodeStr[PCSRO] := 'sro';
    pcodeStr[PCSTO] := 'sto';
    pcodeStr[PCSTN] := 'stn';
    pcodeStr[PCSTP] := 'stp';
    pcodeStr[PCSTR] := 'str';
    pcodeStr[PCSUB] := 'sub';
    pcodeStr[PCSYM] := 'sym';
    pcodeStr[PCSYS] := 'sys';
    pcodeStr[PCTJP] := 'tjp';
    pcodeStr[PCTRC] := 'trc';
    pcodeStr[PCTYP] := 'typ';
    pcodeStr[PCUJP] := 'ujp';
    pcodeStr[PCUNI] := 'uni';
    pcodeStr[PCUSE] := 'use';
    pcodeStr[PCXJP] := 'xjp';
    pcodeStr[PCZER] := 'zer';
    pcodeStr[PCSIN] := 'sin';
    pcodeStr[PCSEX] := 'sex';
    pcodeStr[PCVIN] := 'vin';
    pcodeStr[PCVDE] := 'vde';
    pcodeStr[PCFOR] := 'for';
    pcodeStr[PCAD2] := 'ad2';
    pcodeStr[PCSB2] := 'sb2';
    pcodeStr[PCMP2] := 'mp2';
    pcodeStr[PCDV2] := 'dv2';
    pcodeStr[PCBIT] := 'bit';
    pcodeStr[PCZZZ] := 'zzz';

    typeChar[DTNULL] := '?';
    typeChar[DTPOINTER] := 'a';
    typeChar[DTRECORD] := 'q';
    typeChar[DTARRAY] := 's';
    typeChar[DTINTEGER] := 'i';
    typeChar[DTBOOLEAN] := 'b';
    typeChar[DTCHAR] := 'c';
    typeChar[DTRENAME] := '?';
    typeChar[DTOPAQUE] := '?';
    typeChar[DTSTRING] := 's';
    typeChar[DTREAL] := 'r';
    typeChar[DTLONGREAL] := 'R';
    typeChar[DTSET] := 'S';
    typeChar[DTCARDINAL] := 'j';
    typeChar[DTSUBRANGE] := '?';
    typeChar[DTENUMERATION] := 'j';
    typeChar[DTPROC] := 'p';
    typeChar[DTWORD] := 'i';
    typeChar[DTBYTE] := 'c';
    typeChar[DTANY] := '?';

    memTypeChar[MEMGLOBAL] := 'c';
    memTypeChar[MEMNORMAL] := 'm';
    memTypeChar[MEMFAST] := 't';
    memTypeChar[MEMPARAM] := 'p';

    for t := TKENDOFFILE to TKBAD do begin
	operPcode[t] := PCZZZ;
    end;

    operPcode[TKPLUS] :=	PCADD;
    operPcode[TKMINUS] :=	PCSUB;
    operPcode[TKASTERISK] :=	PCMUP;
    operPcode[TKSLASH] :=	PCDIV;
    operPcode[TKAMPERSAND] :=	PCAND;
    operPcode[TKEQUALS] :=	PCEQU;
    operPcode[TKSHARP] :=	PCNEQ;
    operPcode[TKLESS] :=	PCLES;
    operPcode[TKGREATER] :=	PCGRT;
    operPcode[TKNOTEQUAL] :=	PCNEQ;
    operPcode[TKLSEQUAL] :=	PCLEQ;
    operPcode[TKGREQUAL] :=	PCGEQ;
    operPcode[TKAND] :=		PCAND;
    operPcode[TKDIV] :=		PCDIV;
    operPcode[TKIN] :=		PCINN;
    operPcode[TKMOD] :=		PCMOD;
    operPcode[TKNOT] :=		PCNOT;
    operPcode[TKOR] :=		PCIOR;

    labelNumber := 10000;
end;

function NewLabel {: LabelNumber};
begin
    labelNumber := labelNumber + 1;
    NewLabel := labelNumber;
end;

procedure GenLabel{(l:LabelNumber)};
begin
    write(codeFile,'l',l:1);
end;

procedure GenOp{(op:PcodeInst)};
begin
    write(codeFile,TAB,pcodeStr[op],TAB);
end;

procedure GenT{(tn:TypeNode)};
var
    bt : TypeNode;
begin
    bt := BaseType(tn);
    if bt = nil then begin
	bt := procTypeNode;
    end else if bt = cardIntTypeNode then begin
	bt := cardinalTypeNode;
    end;
    write(codeFile,typeChar[bt^.kind]);
end;

procedure GenMt{(mt:MemoryType)};
begin
    write(codeFile,memTypeChar[mt]);
end;

procedure GenInteger{(v:cardinal)};
begin
    write(codeFile,v:1:0);
end;

procedure GenOpL{(op:PcodeInst)};
begin
    GenOp(op);
    EndLine;
end;

procedure GenOpTL{(op:PcodeInst;tn:TypeNode)};
begin
    GenOp(op);
    GenT(tn);
    EndLine;
end;

procedure GenOpT{(op:PcodeInst;tn:TypeNode)};
begin
    GenOp(op);
    GenT(tn);
end;

procedure GenChar{(c:char)};
begin
    if (c < ' ') or (c > '~') then begin
	write(codeFile,'\',ord(c):1,'\');
    end else begin
	write(codeFile,c:1);
    end;
end;

procedure GenReal{(r:real)};
begin
    write(codeFile,r:1:14);
end;

procedure GenString{(s:String)};
begin
    WriteString(codeFile,s);
end;

procedure GenText{(s:ShortString)};
var
    i : integer;
begin
    i := 1;
    while (i < SHORTSTRINGSIZE) and (s[i] <> ' ') do begin
	write(codeFile,s[i]);
	i := i + 1;
    end;
end;

procedure GenSet{(s:SetValue)};
var
    i, last : integer;
    setSize : cardinal;
    found : boolean;
    tn : TypeNode;
begin
    tn := BaseType(s^.setType);
    setSize := NumberOf(tn^.setRange);
    GenInteger(setSize);
    Comma;
    last := trunc(setSize) - 1;
    found := false;
    while not found and (last >= 0) do begin
	if last in s^.value then begin
	    found := true;
	end else begin
	    last := last - 1;
	end;
    end;
    GenInteger(last+1);
    Comma;
    for i := 0 to last do begin
	if i in s^.value then begin
	    GenChar('1');
	end else begin
	    GenChar('0');
	end;
    end;
end;

procedure Comma;
begin
    write(codeFile,',');
end;

procedure EndLine;
begin
    writeln(codeFile);
end;

