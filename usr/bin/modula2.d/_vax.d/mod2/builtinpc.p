(*#@(#)builtinpc.p	4.1	Ultrix	7/17/90 *)
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
$Header: builtinpc.p,v 1.5 84/05/19 11:36:54 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "const.h"
#include "decls.h"
#include "bexpr.h"
#include "cexpr.h"
#include "builtin.h"
#include "builtinpc.h"
#include "pcode.h"
#include "optim.h"
#include "otree.h"
#include "ocount.h"
#include "genpc.h"
#include "genpcf.h"

const
    EOFOFFSET = 132;	{ offset of EOF flag in IO buffer }

procedure GenBuiltin{(proc : ProcNode; params : ExprList)};
var
    p, p1, p2, p3, p4 : ExprNode;
    pt1, pt2, tn : TypeNode;
    minElement : cardinal;
    procnum, ptrCheck : cardinal;
    numParams : integer;
    lab : LabelNumber;
begin
    p1 := nil;
    p2 := nil;
    pt1 := nil;
    pt2 := nil;
    if params = nil then begin
	{ do nothing }
    end else if params^.first = nil then begin
	{ do nothing }
    end else begin
	p1 := params^.first;
	pt1 := p1^.exprType;
	if p1^.next <> nil then begin
	    p2 := p1^.next;
	    pt2 := p2^.exprType
	end;
    end;

    case proc^.builtin of
	BIPABS : begin
	    GenExpr(p1,EVALGET);
	    GenOpTL(PCABS,pt1);
	end;
	BIPASSERT : begin
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALPOINT);
	    GenOp(PCCHK); GenChar('A'); EndLine;
	end;
	BIPCAP : begin
	    GenExpr(p1,EVALGET);
	    GenOpTL(PCCAP,pt1);
	end;
	BIPCHR : begin
	    GenExpr(p1,EVALGET);
	    GenOpTL(PCTYP,charTypeNode);
	end;
	BIPDEC, BIPINC, BIPINCL, BIPEXCL : begin
	    GenExpr(p1,EVALPOINT);
	    GenExpr(p2,EVALGET);
	    case proc^.builtin of
		BIPINC : GenOp(PCVIN);
		BIPDEC : GenOp(PCVDE);
		BIPINCL : GenOp(PCSIN);
		BIPEXCL : GenOp(PCSEX);
	    end;
	    p3 := p2^.next;
	    GenInteger(p3^.exprConst^.cardVal);
	    EndLine;
	end;
	BIPFLOAT : begin
	    GenExpr(p1,EVALGET);
	    GenOpTL(PCFLT,realTypeNode);
	end;
	BIPLONGFLOAT : begin
	    GenExpr(p1,EVALGET);
	    GenOpTL(PCFLT,longrealTypeNode);
	end;
	BIPHALT : begin
	    GenOpL(PCMST);
	    GenOpT(PCCEP,procTypeNode);
	    Comma;
	    GenInteger(0);
	    Comma;
	    GenInteger(0);
	    Comma;
	    GenText('exit');
	    EndLine;
	end;
	BIPHIGH, BIPNUMBER : begin
	    GenExpr(p1,EVALPOINT);
	    GenOpT(PCINC,addressTypeNode);
	    Comma;
	    GenInteger(WORDSIZE);
	    EndLine;
	    GenIndirectVar(integerTypeNode,EVALGET);
	    if proc^.builtin = BIPHIGH then begin
		GenOpT(PCDEC,integerTypeNode);
		Comma;
		GenInteger(1);
		EndLine;
	    end;
	end;
	BIPMAX : begin
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALGET);
	    GenOpTL(PCMAX,pt1);
	end;
	BIPMIN : begin
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALGET);
	    GenOpTL(PCMIN,pt1);
	end;
	BIPNEW : begin
	    p3 := p2^.next;
	    ptrCheck := p3^.exprConst^.cardVal;
	    p4 := p3^.next;
	    GenOpL(PCMST);
	    GenExpr(p1,EVALPOINT);
	    GenExpr(p2,EVALGET);
	    if p4 = nil then begin
		{ builtin new }
		GenOpT(PCCEP,procTypeNode); Comma; GenInteger(0);
		    Comma; GenInteger(2); Comma;
		if ptrCheck = ord(CHECKPTRPASCAL) then begin
		    GenText('NEW');
		end else if ptrCheck = ord(CHECKPTRC) then begin
		    GenText('modmalloc');
		end else begin
		    GenText('Storage_');
		    GenText('ALLOCATE');
		end;
		EndLine;
	    end else begin
		{ user defined allocate }
		GenCall(p4^.exprConst^.procVal^.internalProc and not OptNcall,
		    p4^.exprConst^.procVal^.globalName,
		    p4^.exprConst^.procVal^.procType,2);
	    end;
	    if ptrCheck = ord(CHECKPTRMODULA) then begin
		{ address was stored: load it }
		GenExpr(p1,EVALPOINT);
		GenIndirectVar(addressTypeNode,EVALGET);
		{ store address+wordsize at address }
		{ copy address }
		GenOp(PCSAV); GenInteger(0); Comma; GenChar('c'); EndLine;
		{ increment address }
		GenOpT(PCINC,addressTypeNode); Comma; GenInteger(WORDSIZE);
		EndLine;
		{ save incremented address }
		GenOp(PCSAV); GenInteger(1); Comma; GenChar('c'); EndLine;
		{ get original address }
		GenOp(PCUSE); GenInteger(0); Comma; GenChar('m'); EndLine;
		{ save incremented address at original address }
		GenIndirectVar(addressTypeNode,EVALPUT);
		{ get incremented address }
		GenOp(PCUSE); GenInteger(1); Comma; GenChar('m'); EndLine;
		{ store value }
		GenStore(p1,addressTypeNode);
	    end;
	end;
	BIPDISPOSE : begin
	    p3 := p2^.next;
	    ptrCheck := p3^.exprConst^.cardVal;
	    p4 := p3^.next;
	    if ptrCheck = ord(CHECKPTRMODULA) then begin
		{ fix up pointer before disposing it }
		{ address of pointer variable }
		GenExpr(p1,EVALPOINT);
		{ save variable address }
		GenOp(PCSAV); GenInteger(0); Comma; GenChar('c'); EndLine;
		{ get pointer value }
		GenIndirectVar(addressTypeNode,EVALGET);
		if genCheckFlag then begin
		    { make sure pointer is valid before disposing it }
		    GenOp(PCCHK); GenChar('a'); Comma; GenChar('m');
		    EndLine;
		end;
		{ decrement pointer to original beginning of area }
		GenOpT(PCDEC,addressTypeNode); Comma; GenInteger(WORDSIZE);
		EndLine;
		{ get address of pointer variable }
		GenOp(PCUSE); GenInteger(0); Comma; GenChar('m'); EndLine;
		{ put it back }
		GenIndirectVar(addressTypeNode,EVALPUT);
	    end;
	    GenOpL(PCMST);
	    GenExpr(p1,EVALPOINT);
	    GenExpr(p2,EVALGET);
	    if p4 = nil then begin
		{ builtin dispose }
		GenOpT(PCCEP,procTypeNode); Comma; GenInteger(0);
		    Comma; GenInteger(2); Comma;
		if ptrCheck = ord(CHECKPTRPASCAL) then begin
		    GenText('DISPOSE');
		end else if ptrCheck = ord(CHECKPTRC) then begin
		    GenText('free');
		end else begin
		    GenText('Storage_');
		    GenText('DEALLOCATE');
		end;
		EndLine;
	    end else begin
		{ user defined dispose }
		GenCall(p4^.exprConst^.procVal^.internalProc and not OptNcall,
		    p4^.exprConst^.procVal^.globalName,
		    p4^.exprConst^.procVal^.procType,2);
	    end;
	end;
	BIPALLOCATE, BIPDEALLOCATE : begin
	    GenOpL(PCMST);
	    GenExpr(p1,EVALPOINT);
	    GenExpr(p2,EVALGET);
	    if proc^.builtin = BIPALLOCATE then begin
		{ memory.allocate }
		GenOpT(PCCEP,procTypeNode); Comma; GenInteger(0);
		    Comma; GenInteger(2); Comma;
		    GenText('MEMORY_');
		    GenText('ALLOCATE');
		EndLine;
	    end else begin
		{ memory.deallocate }
		GenOpT(PCCEP,procTypeNode); Comma; GenInteger(0);
		    Comma; GenInteger(2); Comma;
		    GenText('MEMORY_');
		    GenText('DEALLOCATE');
		EndLine;
	    end;
	end;
	BIPODD : begin
	    GenExpr(p1,EVALGET);
	    GenOpL(PCODD);
	end;
	BIPORD : begin
	    GenExpr(p1,EVALGET);
	    GenOpL(PCORD);
	end;
	BIPTRUNC : begin
	    GenExpr(p1,EVALGET);
	    GenOpTL(PCTRC,p1^.exprType);
	end;
	BIPVAL : begin
	    GenExpr(p2,EVALGET);
	    GenOpTL(PCTYP,pt1);
	end;
	BIPADR : begin
	    GenExpr(p1,EVALPOINT);
	end;
	BIPSIZE, BIPTSIZE, BIPBYTESIZE, BIPTBYTESIZE, BIPFIRST, BIPLAST : begin
	    ExprError(p1,'GenBuiltin: Constant?');
	end;
	BIPUNIXCALL : begin
	    GenOpL(PCMST);
	    p := p2;
	    numParams := 0;
	    while p <> nil do begin
		numParams := numParams + 1;
		GenExpr(p,EVALGET);
		p := p^.next;
	    end;
	    procnum := OrdOf(p1^.exprConst);
	    GenOpT(PCCEP,integerTypeNode);
	    Comma;
	    GenInteger(WORDSIZE);
	    Comma;
	    GenInteger(numParams);
	    Comma;
	    GenText(unixProcNames[trunc(procnum)]);
	    EndLine;
	end;
	BIPWRITEF : begin
	    GenOpL(PCMST);
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALPOINT);
	    p := p2^.next;
	    numParams := 2;
	    while p <> nil do begin
		numParams := numParams + 1;
		if p^.exprType^.kind = DTARRAY then begin
		    GenExpr(p,EVALPOINT);
		end else begin
		    GenExpr(p,EVALGET);
		    if p^.exprType^.kind = DTREAL then begin
			{ make float be double }
			GenConstInteger(0);
			numParams := numParams + 1;
		    end;
		end;
		p := p^.next;
	    end;
	    GenOpT(PCCEP,procTypeNode);
	    Comma;
	    GenInteger(0);
	    Comma;
	    GenInteger(numParams);
	    Comma;
	    GenText('fprintf');
	    EndLine;
	end;
	BIPREADF : begin
	    GenOpL(PCMST);
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALPOINT);
	    p := p2^.next;
	    numParams := 2;
	    while p <> nil do begin
		numParams := numParams + 1;
		GenExpr(p,EVALPOINT);
		p := p^.next;
	    end;
	    GenOpT(PCCEP,integerTypeNode);
	    Comma;
	    GenInteger(WORDSIZE);
	    Comma;
	    GenInteger(numParams);
	    Comma;
	    GenText('fscanf');
	    EndLine;
	end;
	BIPWRITES : begin
	    GenOpL(PCMST);
	    GenExpr(p1,EVALPOINT);
	    GenExpr(p2,EVALPOINT);
	    p := p2^.next;
	    numParams := 2;
	    while p <> nil do begin
		numParams := numParams + 1;
		if p^.exprType^.kind = DTARRAY then begin
		    GenExpr(p,EVALPOINT);
		end else begin
		    GenExpr(p,EVALGET);
		    if p^.exprType^.kind = DTREAL then begin
			{ make float be double }
			GenConstInteger(0);
			numParams := numParams + 1;
		    end;
		end;
		p := p^.next;
	    end;
	    GenOpT(PCCEP,procTypeNode);
	    Comma;
	    GenInteger(0);
	    Comma;
	    GenInteger(numParams);
	    Comma;
	    GenText('sprintf');
	    EndLine;
	end;
	BIPREADS : begin
	    GenOpL(PCMST);
	    GenExpr(p1,EVALPOINT);
	    GenExpr(p2,EVALPOINT);
	    p := p2^.next;
	    numParams := 2;
	    while p <> nil do begin
		numParams := numParams + 1;
		GenExpr(p,EVALPOINT);
		p := p^.next;
	    end;
	    GenOpT(PCCEP,integerTypeNode);
	    Comma;
	    GenInteger(WORDSIZE);
	    Comma;
	    GenInteger(numParams);
	    Comma;
	    GenText('sscanf');
	    EndLine;
	end;
	BIPWRITEB : begin
	    p3 := p2^.next;
	    GenOpL(PCMST);
	    GenExpr(p2,EVALPOINT);
	    GenConstInteger(1);
	    GenExpr(p3,EVALGET);
	    GenExpr(p1,EVALGET);
	    GenOpT(PCCEP,procTypeNode);
	    Comma;
	    GenInteger(0);
	    Comma;
	    GenInteger(4);
	    Comma;
	    GenText('fwrite');
	    EndLine;
	end;
	BIPREADB : begin
	    p3 := p2^.next;
	    GenOpL(PCMST);
	    GenExpr(p2,EVALPOINT);
	    GenConstInteger(1);
	    GenExpr(p3,EVALGET);
	    GenExpr(p1,EVALGET);
	    GenOpT(PCCEP,integerTypeNode);
	    Comma;
	    GenInteger(WORDSIZE);
	    Comma;
	    GenInteger(4);
	    Comma;
	    GenText('fread');
	    EndLine;
	end;
	BIPWRITEC : begin
	    GenOpL(PCMST);
	    GenExpr(p2,EVALGET);
	    GenExpr(p1,EVALGET);
	    GenOpT(PCCEP,procTypeNode);
	    Comma;
	    GenInteger(0);
	    Comma;
	    GenInteger(2);
	    Comma;
	    GenText('fputc');
	    EndLine;
	end;
	BIPREADC : begin
	    GenOpL(PCMST);
	    GenExpr(p1,EVALGET);
	    GenOpT(PCCEP,charTypeNode);
	    Comma;
	    GenInteger(BYTESIZE);
	    Comma;
	    GenInteger(1);
	    Comma;
	    GenText('fgetc');
	    EndLine;
	    GenStore(p2,charTypeNode);
	    GenExpr(p1,EVALGET);
	    GenOpT(PCINC,addressTypeNode); Comma;
		GenInteger(EOFOFFSET); EndLine;
	    GenOpT(PCIND,cardinalTypeNode); Comma; GenInteger(1); EndLine;
	    GenOpTL(PCNEG,integerTypeNode);
	end;
	BIPOPENF : begin
	    GenOpL(PCMST);
	    GenExpr(p1,EVALPOINT);
	    GenExpr(p2,EVALPOINT);
	    GenOpT(PCCEP,fileTypeNode);
	    Comma;
	    GenInteger(WORDSIZE);
	    Comma;
	    GenInteger(2);
	    Comma;
	    GenText('fopen');
	    EndLine;
	end;
	BIPCLOSEF : begin
	    GenOpL(PCMST);
	    GenExpr(p1,EVALGET);
	    GenOpT(PCCEP,procTypeNode);
	    Comma;
	    GenInteger(0);
	    Comma;
	    GenInteger(1);
	    Comma;
	    GenText('fclose');
	    EndLine;
	end;
	BIPNEWPROCESS : begin
	    p3 := p2^.next;
	    p4 := p3^.next;
	    GenOpL(PCMST);
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALGET);
	    GenExpr(p3,EVALGET);
	    GenExpr(p4,EVALPOINT);
	    GenOpT(PCCEP,procTypeNode);
	    Comma;
	    GenInteger(0);
	    Comma;
	    GenInteger(4);
	    Comma;
	    GenText('SYSTEM_');
	    GenText('newprocess');
	    EndLine;
	end;
	BIPTRANSFER : begin
	    GenOpL(PCMST);
	    GenExpr(p1,EVALPOINT);
	    GenExpr(p2,EVALPOINT);
	    GenOpT(PCCEP,procTypeNode);
	    Comma;
	    GenInteger(0);
	    Comma;
	    GenInteger(2);
	    Comma;
	    GenText('SYSTEM_transfer');
	    EndLine;
	end;
	BIPCPUTIME : begin
	    GenOpL(PCMST);
	    GenOpT(PCCEP,integerTypeNode);
	    Comma;
	    GenInteger(WORDSIZE);
	    Comma;
	    GenInteger(0);
	    Comma;
	    GenText('SYSTEM_');
	    GenText('cputime');
	    EndLine;
	end;
	BIPBITNOT : begin
	    GenExpr(p1,EVALGET);
	    GenOp(PCBIT); GenChar('n'); EndLine;
	end;
	BIPBITAND : begin
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALGET);
	    GenOp(PCBIT); GenChar('a'); EndLine;
	end;
	BIPBITOR : begin
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALGET);
	    GenOp(PCBIT); GenChar('o'); EndLine;
	end;
	BIPBITXOR : begin
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALGET);
	    GenOp(PCBIT); GenChar('x'); EndLine;
	end;
	BIPBITSHIFTLEFT : begin
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALGET);
	    GenOp(PCBIT); GenChar('l'); EndLine;
	end;
	BIPBITSHIFTRIGHT : begin
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALGET);
	    GenOp(PCBIT); GenChar('r'); EndLine;
	end;
	BIPBITEXTRACT : begin
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALGET);
	    p3 := p2^.next;
	    GenExpr(p3,EVALGET);
	    GenOp(PCBIT); GenChar('e'); EndLine;
	end;
	BIPBITINSERT : begin
	    GenExpr(p1,EVALGET);
	    GenExpr(p2,EVALGET);
	    p3 := p2^.next;
	    GenExpr(p3,EVALGET);
	    p4 := p3^.next;
	    GenExpr(p4,EVALGET);
	    GenOp(PCBIT); GenChar('i'); EndLine;
	end;
    end;
end;

