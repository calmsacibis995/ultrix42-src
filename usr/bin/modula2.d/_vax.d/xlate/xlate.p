(*#@(#)xlate.p	4.1	Ultrix	7/17/90 *)
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
$Header: xlate.p,v 1.5 84/05/19 11:36:47 powell Exp $
 ****************************************************************************)
program main(output);
#include "globals.h"
#include "opsubs.h"
#include "util.h"
#include "init.h"
#include "input.h"
#include "codesubs.h"

procedure Generate;
var skipit : boolean;
begin
    echopcodeline;
    skipit := FALSE;
    if (nextjump <> jcnone) then begin
	skipit := DoJumps;          { handle delayed jump }
    end;
    if not skipit then begin
	case opcode of
	    opabs: opabsc;
	    opadd: opaddc;
	    opand: opandc;
	    opbgn: opbgnc;
	    opcap: opcapc;
	    opcep: opcepc;
	    opchk: opchkc;
	    opchr: opchrc;
	    opcip: opcipc;
	    opcjp: opcjpc;
	    opcom: opcomc;
	    opcsp: opcspc;
	    opctr: opctrc;
	    opcts: opctsc;
	    opcup: opcupc;
	    opdec: opdecc;
	    opdef: opdefc;
	    opdif: opdifc;
	    opdiv: opdivc;
	    opdsp: opdspc;
	    opent: opentc;
	    opequ: opequc;
	    opexi: opexic;
	    opfjp: opfjpc;
	    opflt: opfltc;
	    opfor: opforc;
	    opgeq: opgeqc;
	    opgrt: opgrtc;
	    opinc: opincc;
	    opind: opindc;
	    opinn: opinnc;
	    opint: opintc;
	    opinx: opinxc;
	    opior: opiorc;
	    opixa: opixac;
	    oplab: oplabc;
	    oplao: oplaoc;
	    oplca: oplcac;
	    oplda: opldac;
	    opldc: opldcc;
	    opldo: opldoc;
	    opleq: opleqc;
	    oples: oplesc;
	    oploc: oplocc;
	    oplod: oplodc;
	    opmax: opmaxc;
	    opmin: opminc;
	    opmod: opmodc;
	    opmov: opmovc;
	    opmst: opmstc;
	    opmup: opmupc;
	    opmus: opmusc;
	    opmvn: opmvnc;
	    opnam: opnamc;
	    opneg: opnegc;
	    opneq: opneqc;
	    opnew: opnewc;
	    opnot: opnotc;
	    opodd: opoddc;
	    opord: opordc;
	    oppar: opparc;
	    opret: opretc;
	    opsal: opsalc;
	    opsav: opsavc;
	    opsdf: opsdfc;
	    opsex: opsexc;
	    opsgs: opsgsc;
	    opsin: opsinc;
	    opsml: opsmlc;
	    opsro: opsroc;
	    opstn: opstnc;
	    opsto: opstoc;
	    opstp: opstpc;
	    opstr: opstrc;
	    opsub: opsubc;
	    opsym: opsymc;
	    opsys: opsysc;
	    optjp: optjpc;
	    optrc: optrcc;
	    optyp: optypc;
	    opujp: opujpc;
	    opuni: opunic;
	    opuse: opusec;
	    opvde: opvdec;
	    opvin: opvinc;
	    opxjp: opxjpc;
	    opzer: opzerc;
	    opcmt: opcmtc;
	    opad2: opad2c;
	    opsb2: opsb2c;
	    opmp2: opmp2c;
	    opdv2: opdv2c;
	    opbit: opbitc;
	    opzzz: opzzzc;
	end;
    end;
    FreeDisp;
end;

{*** main program ***}
begin
    initialize;
    setoptions;
    opcode:=opzzz;
    nextjump := jcnone;
    PreRead;
    while ((opcode<>opstp) and not EOF) do
    begin
	Advance;
	PreRead;
	Generate;
	if (VERBOSE) then begin
	    if ((line mod 1000)=0) then begin
		ErrorStr('line: ',6);
		ErrorI(line,1);
		ErrorEOL;
	    end;
	end;
    end;
    line := line - 1;
    if (VERBOSE) then begin
	ErrorI(line,1);
	ErrorStr(' pcode instructions',19);
	ErrorEOL;
    end;
    exit(errorcount);
end.
