(*#@(#)bexpr.p	4.1	Ultrix	7/17/90 *)
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
$Header: bexpr.p,v 1.3 84/05/19 11:35:08 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "scanner.h"
#include "bexpr.h"
#include "const.h"

function NewExprNode{(kind : ExprKind) : ExprNode};
var
    en : ExprNode;
begin
    new(en);
    en^.kind := kind;
    if currFile <> nil then begin
	en^.fileName := currFile;
	en^.lineNumber := currLine;
    end else begin
	en^.fileName := nil;
	en^.lineNumber := 0;
    end;
    en^.exprType := nil;
    en^.baseVar := nil;
    en^.basePtrType := nil;
    en^.opt := 0;
    NewExprNode := en;
end;

procedure SameExprLine{(newOne, oldOne : ExprNode)};
begin
    newOne^.fileName := oldOne^.fileName;
    newOne^.lineNumber := oldOne^.lineNumber;
end;

function ConstExprNode{(cn : ConstNode) : ExprNode};
var
    en : ExprNode;
begin
    if TraceNexpr then begin
	write(output,'ConstExprNode(',cn^.kind:1,')=');
	WriteConstant(output,cn);
	writeln(output);
    end;
    en := NewExprNode(EXPRCONST);
    en^.exprConst := cn;
    ConstExprNode := en;
end;

function SymExprNode(names : IdentList) : ExprNode;
var
    en : ExprNode;
begin
    if TraceNexpr then begin
	writeln(output,'SymExprNode');
    end;
    en := NewExprNode(EXPRNAME);
    en^.exprName := names;
    SymExprNode := en;
end;

function DerefExprNode{(ptr : ExprNode) : ExprNode};
var
    en : ExprNode;
begin
    if TraceNexpr then begin
	writeln(output,'DerefExprNode');
    end;
    en := NewExprNode(EXPRDEREF);
    en^.ptr := ptr;
    en^.realPtr := true;
    DerefExprNode := en;
end;

function UnOpExprNode{(oper : Token; opnd : ExprNode) : ExprNode};
var
    en : ExprNode;
begin
    if TraceNexpr then begin
	writeln(output,'UnOpExprNode(',oper:0,')');
    end;
    en := NewExprNode(EXPRUNOP);
    en^.exprUnOp := oper;
    en^.opnd := opnd;
    UnOpExprNode := en;
end;

function BinOpExprNode(oper : Token; opnd1, opnd2 : ExprNode) : ExprNode;
var
    en : ExprNode;
begin
    if TraceNexpr then begin
	writeln(output,'BinExprNode(',oper:0,')');
    end;
    en := NewExprNode(EXPRBINOP);
    en^.exprBinOp := oper;
    en^.opnd1 := opnd1;
    en^.opnd2 := opnd2;
    SameExprLine(en,opnd1);
    BinOpExprNode := en;
end;

function SubscriptExprNode(arr: ExprNode; subsList : ExprList) : ExprNode;
var
    en : ExprNode;
begin
    if TraceNexpr then begin
	writeln(output,'SubscriptExprNode');
    end;
    en := NewExprNode(EXPRSUBSCR);
    en^.arr := arr;
    en^.subscripts := subsList;
    SubscriptExprNode := en;
end;

function DotExprNode{(rec : ExprNode; field : String) : ExprNode};
var
    en : ExprNode;
begin
    if TraceNexpr then begin
	writeln(output,'DotExprNode');
    end;
    en := NewExprNode(EXPRDOT);
    en^.rec := rec;
    en^.fieldName := field;
    en^.field := nil;
    SameExprLine(en,rec);
    DotExprNode := en;
end;

function FuncExprNode(func : ExprNode; params : ExprList) : ExprNode;
var
    en : ExprNode;
begin
    if TraceNexpr then begin
	writeln(output,'FuncExprNode');
    end;
    en := NewExprNode(EXPRFUNC);
    en^.func := func;
    en^.params := params;
    SameExprLine(en,func);
    FuncExprNode := en;
end;

procedure Indent{(var f : text; indent:integer)};
begin
    if indent > 0 then begin
	write(f,' ':indent);
    end;
end;


procedure PrintExpr{(en:ExprNode;indent:integer)};
var
    ten : ExprNode;
    id : IdentNode;
begin
    Indent(output,indent);
    write(output,'Expression ');
    if en = nil then begin
	writeln(output,'NIL');
    end else begin
	write(output,en^.kind,' ');
	case en^.kind of
	    EXPRBAD : begin
		writeln(output,'BAD');
	    end;
	    EXPRNAME : begin
		id := en^.exprName^.first;
		while id <> nil do begin
		    WriteString(output,id^.name);
		    id := id^.next;
		    if id <> nil then begin
			write(output,'.');
		    end;
		end;
		writeln(output);
	    end;
	    EXPRVAR : begin
		if en^.exprVar^.address.kind = MEMGLOBAL then begin
		    WriteString(output,en^.exprVar^.address.gvn^.name);
		end else begin
		    write(output,en^.exprVar^.address.kind,' ',
			en^.exprVar^.address.level:1,' ',
			en^.exprVar^.address.proc^.block:1, ' ',
			en^.exprVar^.address.offset:1:0);
		end;
		writeln(output);
	    end;
	    EXPRSYM : begin
		WriteString(output,en^.exprSym^.name);
		writeln(output);
	    end;
	    EXPRCONST : begin
		WriteConstant(output,en^.exprConst);
		writeln(output);
	    end;
	    EXPRUNOP : begin
		writeln(output);
		Indent(output,indent);
		writeln(output,en^.exprUnOp);
		PrintExpr(en^.opnd,indent+INDENT);
	    end;
	    EXPRBINOP : begin
		writeln(output);
		PrintExpr(en^.opnd1,indent+INDENT);
		Indent(output,indent);
		writeln(output,en^.exprBinOp);
		PrintExpr(en^.opnd2,indent+INDENT);
	    end;
	    EXPRSUBSCR : begin
		writeln(output);
		PrintExpr(en^.arr,indent+INDENT);
		ten := en^.subscripts^.first;
		while ten <> nil do begin
		    Indent(output,indent);
		    writeln(output,'****');
		    PrintExpr(ten,indent+INDENT);
		    ten := ten^.next;
		end;
	    end;
	    EXPRDOT : begin
		writeln(output);
		PrintExpr(en^.rec,indent+INDENT);
		Indent(output,indent+INDENT);
		if en^.field <> nil then begin
		    WriteString(output,en^.field^.name);
		end else begin
		    WriteString(output,en^.fieldName);
		end;
		writeln(output);
	    end;
	    EXPRDEREF : begin
		writeln(output);
		PrintExpr(en^.ptr,indent+INDENT);
	    end;
	    EXPRFUNC : begin
		writeln(output);
		PrintExpr(en^.func,indent+INDENT);
		ten := en^.params^.first;
		while ten <> nil do begin
		    Indent(output,indent);
		    writeln(output,'****');
		    PrintExpr(ten,indent+INDENT);
		    ten := ten^.next;
		end;
	    end;
	    EXPRVAL : begin
		writeln(output);
		PrintExpr(en^.exprVal,indent+INDENT);
	    end;
	    EXPRCHECK : begin
		writeln(output,en^.exprCheck);
		PrintExpr(en^.checkExpr,indent+INDENT);
	    end;
	    EXPRSET : begin
		writeln(output,'set expression');
	    end;
	end;
    end;
end;

procedure WriteExpr{(var f : text; en : ExprNode)};
var
    id : IdentNode;
    pen : ExprNode;
begin
    case en^.kind of
	EXPRBAD : begin
	    write(f,'BAD');
	end;
	EXPRNAME : begin
	    id := en^.exprName^.first;
	    while id <> nil do begin
		WriteString(f,id^.name);
		if id^.next <> nil then begin
		    write(f,'.');
		end;
		id := id^.next;
	    end;
	end;
	EXPRVAR : begin
	    write(f,'var ');
	    WriteString(f,en^.exprVar^.name);
	end;
	EXPRSUBSCR : begin
	    WriteExpr(f,en^.arr);
	    write(f,'[');
	    WriteExpr(f,en^.subsExpr);
	    write(f,']');
	end;
	EXPRFUNC : begin
	    WriteExpr(f,en^.func);
	    write(f,'(');
	    pen := en^.params^.first;
	    while pen<> nil do begin
		WriteExpr(f,pen);
		if pen^.next <> nil then begin
		write(f,',');
		end;
		pen := pen^.next;
	    end;
	    write(f,')');
	end;
	EXPRSET : begin
	    write(f,'[set]');
	end;
	EXPRSYM : begin
	    write(f,'sym ');
	    WriteString(f,en^.exprSym^.name);
	end;
	EXPRCONST :begin
	    WriteConstant(f,en^.exprConst);
	end;
	EXPRUNOP : begin
	    WriteString(f,stringToken[en^.exprUnOp]);
	    write(f,'(');
	    WriteExpr(f,en^.opnd);
	    write(f,')');
	end;
	EXPRBINOP : begin
	    write(f,'(');
	    WriteExpr(f,en^.opnd1);
	    WriteString(f,stringToken[en^.exprBinOp]);
	    WriteExpr(f,en^.opnd2);
	    write(f,')');
	end;
	EXPRDOT : begin
	    WriteExpr(f,en^.rec);
	    write(f,'.');
	    WriteString(f,en^.field^.name);
	end;
	EXPRDEREF : begin
	    WriteExpr(f,en^.ptr);
	    write(f,'^');
	end;
	EXPRVAL : begin
	    write(f,'value ');
	    WriteExpr(f,en^.exprVal);
	end;
	EXPRCHECK : begin
	    write(f,en^.exprCheck,' ');
	    WriteExpr(f,en^.checkExpr);
	end;
    end;
end;

function AddToExprList{(list : ExprList; newOne : ExprNode) : ExprList};
begin
    if list = nil then begin
	new(list);
	list^.first := nil;
	list^.last := nil;
    end;
    if newOne = nil then begin
	{ empty statement, do nothing }
    end else if list^.last = nil then begin
	newOne^.next := nil;
	list^.first := newOne;
	list^.last := newOne;
    end else begin
	newOne^.next := nil;
	list^.last^.next := newOne;
	list^.last:= newOne;
    end;
    AddToExprList := list;
end;

function AddToExprSetList (list : ExprSetList; newOne : ExprSetNode) : ExprSetList;
begin
    if list = nil then begin
	new(list);
	list^.first := nil;
	list^.last := nil;
    end;
    if newOne = nil then begin
	{ empty statement, do nothing }
    end else if list^.last = nil then begin
	newOne^.next := nil;
	list^.first := newOne;
	list^.last := newOne;
    end else begin
	newOne^.next := nil;
	list^.last^.next := newOne;
	list^.last:= newOne;
    end;
    AddToExprSetList := list;
end;

function SetExprNode(elementList : ExprSetList; setTypeName : IdentList)
	: ExprNode;
var
    en : ExprNode;
begin
    en := NewExprNode(EXPRSET);
    en^.setTypeName := setTypeName;
    en^.setValue := elementList;
    SetExprNode := en;
end;

function MakeExprSet (lower, upper : ExprNode) : ExprSetNode;
var
    cln : ExprSetNode;
    error : boolean;
begin
    error := false;
    if upper = nil then begin
	upper := lower;
    end;
    new(cln);
    cln^.lower := lower;
    cln^.upper := upper;
    MakeExprSet := cln;
end;
