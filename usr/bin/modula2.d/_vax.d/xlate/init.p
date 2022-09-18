(*#@(#)init.p	4.1	Ultrix	7/17/90 *)
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
#include "init.h"

procedure enteropcode{(codeString : opcodestring; codeType   : opcodes)};
var
   tableIndex : 0 .. MAXTABLESIZE;     { the index to the opcode look up table }
		     
begin
   { hash function }
	tableIndex := (ord(codeString[1]) * 10 + ord(codeString[2]) * 25
		      + ord(codeString[3]) * 64) mod (MAXTABLESIZE+1);

	while (hashTable[tableIndex].oper <> '    ') do begin
					 { the entry is already occurpy }
	    tableIndex := (tableIndex + 1) mod (MAXTABLESIZE + 1);
	end;

	with hashTable[tableIndex] do begin
	    oper := codeString;
	    code := codeType;
	end;

end;

procedure InitOperandString(var opd : operandstring; ss : ShortString);
var
    i : integer;
begin
    i := 1;
    while ss[i] <> ' ' do begin
	opd[i] := ss[i];
	i := i + 1;
    end;
    opd[i] := ' ';
end;

procedure initialize;
var i : integer;
    tempchar : char;
begin
	curblockid := -1;
{ initialize opcode lookup table }
	for i := 0 to MAXTABLESIZE do begin
	    hashTable[i].oper := '    ';
	end;

	enteropcode('abs ', opabs);
	enteropcode('add ', opadd);
	enteropcode('and ', opand);
	enteropcode('bgn ', opbgn);
	enteropcode('cap ', opcap);
	enteropcode('cep ', opcep);
	enteropcode('chk ', opchk);
	enteropcode('chr ', opchr);
	enteropcode('cip ', opcip);
	enteropcode('cjp ', opcjp);
	enteropcode('com ', opcom);
	enteropcode('csp ', opcsp);
	enteropcode('ctr ', opctr);
	enteropcode('cts ', opcts);
	enteropcode('cup ', opcup);
	enteropcode('dec ', opdec);
	enteropcode('def ', opdef);
	enteropcode('dif ', opdif);
	enteropcode('div ', opdiv);
	enteropcode('dsp ', opdsp);
	enteropcode('ent ', opent);
	enteropcode('equ ', opequ);
	enteropcode('exi ', opexi);
	enteropcode('fjp ', opfjp);
	enteropcode('flt ', opflt);
	enteropcode('geq ', opgeq);
	enteropcode('grt ', opgrt);
	enteropcode('inc ', opinc);
	enteropcode('ind ', opind);
	enteropcode('inn ', opinn);
	enteropcode('int ', opint);
	enteropcode('ior ', opior);
	enteropcode('ixa ', opixa);
	enteropcode('lab ', oplab);
	enteropcode('lao ', oplao);
	enteropcode('lca ', oplca);
	enteropcode('lda ', oplda);
	enteropcode('ldc ', opldc);
	enteropcode('ldo ', opldo);
	enteropcode('leq ', opleq);
	enteropcode('les ', oples);
	enteropcode('loc ', oploc);
	enteropcode('lod ', oplod);
	enteropcode('mod ', opmod);
	enteropcode('mov ', opmov);
	enteropcode('mst ', opmst);
	enteropcode('mup ', opmup);
	enteropcode('mus ', opmus);
	enteropcode('mvn ', opmvn);
	enteropcode('nam ', opnam);
	enteropcode('neg ', opneg);
	enteropcode('neq ', opneq);
	enteropcode('new ', opnew);
	enteropcode('not ', opnot);
	enteropcode('odd ', opodd);
	enteropcode('ord ', opord);
	enteropcode('par ', oppar);
	enteropcode('ret ', opret);
	enteropcode('sal ', opsal);
	enteropcode('sav ', opsav);
	enteropcode('sdf ', opsdf);
	enteropcode('sgs ', opsgs);
	enteropcode('sml ', opsml);
	enteropcode('sro ', opsro);
	enteropcode('stn ', opstn);
	enteropcode('sto ', opsto);
	enteropcode('stp ', opstp);
	enteropcode('str ', opstr);
	enteropcode('sub ', opsub);
	enteropcode('sym ', opsym);
	enteropcode('sys ', opsys);
	enteropcode('tjp ', optjp);
	enteropcode('trc ', optrc);
	enteropcode('typ ', optyp);
	enteropcode('ujp ', opujp);
	enteropcode('uni ', opuni);
	enteropcode('use ', opuse);
	enteropcode('xjp ', opxjp);
	enteropcode('zer ', opzer);
	enteropcode('inx ', opinx);
	enteropcode('sin ', opsin);
	enteropcode('sex ', opsex);
	enteropcode('vin ', opvin);
	enteropcode('vde ', opvde);
	enteropcode('for ', opfor);
	enteropcode('max ', opmax);
	enteropcode('min ', opmin);
	enteropcode('ad2 ', opad2);
	enteropcode('sb2 ', opsb2);
	enteropcode('mp2 ', opmp2);
	enteropcode('dv2 ', opdv2);
	enteropcode('bit ', opbit);
	enteropcode('zzz ', opzzz);

	{ number of errors encountered }
	errorcount := 0;

	{ file name table pointer }
	fntptr := 0;

	{ default options }
	ECHOPCODE := FALSE;
	RUNIDS := TRUE;
	VERBOSE := FALSE;
	PRINTNAMES := FALSE;
	{ stuff for delayed jump code }
	jumpopposite[jcnone] := jcnone;
	jumpopposite[jceq] := jcne;
	jumpopposite[jcne] := jceq;
	jumpopposite[jcgt] := jcle;
	jumpopposite[jcle] := jcgt;
	jumpopposite[jclt] := jcge;
	jumpopposite[jcge] := jclt;
	jumpopposite[jcgtu] := jcleu;
	jumpopposite[jcleu] := jcgtu;
	jumpopposite[jcltu] := jcgeu;
	jumpopposite[jcgeu] := jcltu;
	jumpnames[jcnone] := '????';
	jumpnames[jceq] := 'jeql';
	jumpnames[jcne] := 'jneq';
	jumpnames[jcgt] := 'jgtr';
	jumpnames[jcle] := 'jleq';
	jumpnames[jclt] := 'jlss';
	jumpnames[jcge] := 'jgeq';
	jumpnames[jcgtu] := 'jgtru';
	jumpnames[jcleu] := 'jlequ';
	jumpnames[jcltu] := 'jlssu';
	jumpnames[jcgeu] := 'jgequ';

	for tempchar:= chr(0) to chr(127) do
	   associatedType[tempchar]:=tundefined;
	associatedType['a']:=taddress;
	associatedType['b']:=tboolean;
	associatedType['c']:=tchar;
	associatedType['i']:=tinteger;
	associatedType['j']:=tcardinal;
	associatedType['p']:=tproc;
	associatedType['q']:=trecord;
	associatedType['r']:=treal;
	associatedType['R']:=tlongreal;
	associatedType['S']:=tset;
	associatedType['s']:=tstring;
	checkrequested:=true;
	currentLabel:=50001;
	currentConstant := 1;
	line:=1;
     
	numcomblocks := 0;
	

	regString[NULLREG] := 'NUL';	regStringSize[NULLREG] := 3;
	regString[SAVEDREG] := 'SR ';	regStringSize[SAVEDREG] := 2;
	regString[SAVED2REG] := 'S2R';	regStringSize[SAVED2REG] := 23;
	regString[r0] := 'r0 ';		regStringSize[r0] := 2;
	regString[r1] := 'r1 ';		regStringSize[r1] := 2;
	regString[r2] := 'r2 ';		regStringSize[r2] := 2;
	regString[r3] := 'r3 ';		regStringSize[r3] := 2;
	regString[r4] := 'r4 ';		regStringSize[r4] := 2;
	regString[r5] := 'r5 ';		regStringSize[r5] := 2;
	regString[r6] := 'r6 ';		regStringSize[r6] := 2;
	regString[r7] := 'r7 ';		regStringSize[r7] := 2;
	regString[r8] := 'r8 ';		regStringSize[r8] := 2;
	regString[r9] := 'r9 ';		regStringSize[r9] := 2;
	regString[r10] := 'r10';	regStringSize[r10] := 3;
	regString[r11] := 'r11';	regStringSize[r11] := 3;
	regString[ap] := 'ap ';		regStringSize[ap] := 2;
	regString[fp] := 'fp ';		regStringSize[fp] := 2;
	regString[sp] := 'sp ';		regStringSize[sp] := 2;
	regString[pc] := 'pc ';		regStringSize[pc] := 2;
	regString[r20] := 'r20';	regStringSize[r20] := 3;
	regString[r21] := 'r21';	regStringSize[r21] := 3;
	regString[r22] := 'r22';	regStringSize[r22] := 3;
	regString[r23] := 'r23';	regStringSize[r23] := 3;
	regString[r24] := 'r24';	regStringSize[r24] := 3;
	regString[r25] := 'r25';	regStringSize[r25] := 3;
	regString[r26] := 'r26';	regStringSize[r26] := 3;
	regString[r27] := 'r27';	regStringSize[r27] := 3;
	regString[r28] := 'r28';	regStringSize[r28] := 3;
	regString[r29] := 'r29';	regStringSize[r29] := 3;
	regString[r30] := 'r30';	regStringSize[r30] := 3;
	regString[r31] := 'r31';	regStringSize[r31] := 3;
	regString[r32] := 'r32';	regStringSize[r32] := 3;
	regString[r33] := 'r33';	regStringSize[r33] := 3;
	regString[r34] := 'r34';	regStringSize[r34] := 3;
	regString[r35] := 'r35';	regStringSize[r35] := 3;
	regString[r36] := 'r36';	regStringSize[r36] := 3;
	regString[r37] := 'r37';	regStringSize[r37] := 3;
	regString[r38] := 'r38';	regStringSize[r38] := 3;
	regString[r39] := 'r39';	regStringSize[r39] := 3;
	regString[r40] := 'r40';	regStringSize[r40] := 3;
	regString[rt0] := 'rt0';	regStringSize[rt0] := 3;
	regString[rt1] := 'rt1';	regStringSize[rt1] := 3;
	regString[rt2] := 'rt2';	regStringSize[rt2] := 3;
	regString[rt3] := 'rt3';	regStringSize[rt3] := 3;
	regString[rt4] := 'rt4';	regStringSize[rt4] := 3;
	regString[rt5] := 'rt5';	regStringSize[rt5] := 3;
	regString[rt6] := 'rt6';	regStringSize[rt6] := 3;
	regString[rt7] := 'rt7';	regStringSize[rt7] := 3;
	regString[rt8] := 'rt8';	regStringSize[rt8] := 3;
	regString[rt9] := 'rt9';	regStringSize[rt9] := 3;

	InitReg;
	InitEES;

	InitOperandString(RTDISPOSE,IRTDISPOSE);
	InitOperandString(RTNEW,IRTNEW);
	InitOperandString(RTSYSTEM,IRTSYSTEM);
	InitOperandString(RTMAKESET,IRTMAKESET);
	InitOperandString(RTSMALLEST,IRTSMALLEST);
	InitOperandString(RTLONGDIV,IRTLONGDIV);
	InitOperandString(RTLONGMOD,IRTLONGMOD);
	InitOperandString(RTERRORASSERT,IRTERRORASSERT);

	new(opd);
	new(opdsizes);
	new(nopd);
	new(nopdsizes);

end;    {end of initialize}

