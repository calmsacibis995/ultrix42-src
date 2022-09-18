(*#@(#)cexpr.p	4.1	Ultrix	7/17/90 *)
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
$Header: cexpr.p,v 1.6 84/06/06 12:52:19 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "bexpr.h"
#include "cexpr.h"
#include "const.h"
#include "decls.h"
#include "builtin.h"
#include "alloc.h"

{
    Resolve, type-check and reconstruct expressions:

	Resolution is through the procedures NameExpr, SymExpr, RecordExpr,
	and ImplQualExpr.  These routines figure out what a name list means
	and call the regular type-checking routines.  They do no checking
	on their own.

	The regular type-checking routines take an ExprNode parameter and a
	mode indicating how the expression is used (principally, whether the
	value or address is of interest).  For address expressions, they are
	free to reconstruct the expression tree in terms of explicit address
	calculations.  For value expressions, they must insert a value node.
	Since the original expression nodes may be lost, having been converted
	to an address expression, these routines return a type which represents
	the logical type of the expression.  In reconstructing the expression
	tree, the argument ExprNode must remain the root of the expression,
	since there are pointers to it.

	After this pass, there should be no SUBSCRIPT, DOT, or DEREF nodes.
	NAME and SET nodes should also be gone.  SYM nodes may still exist for
	type names.  VAR nodes subsequently mean the address of the variable.

}
procedure BadExpr(en : ExprNode);
begin
    en^.kind := EXPRBAD;
    en^.exprType := anyTypeNode;
end;

function IsBadExpr{(en : ExprNode) : boolean};
begin
    if en = nil then begin
	IsBadExpr := true;
    end else begin
	IsBadExpr := en^.kind = EXPRBAD;
    end;
end;

function IsAddressableExpr{(en : ExprNode) : boolean};
var
    result : boolean;
begin
    result := false;
    if en = nil then begin
	{ do nothing }
    end else if (en^.kind in [EXPRVAL, EXPRVAR, EXPRBINOP, EXPRCHECK]) and
	(en^.exprType = addressTypeNode)
    then begin
	result := true;
    end;
    IsAddressableExpr := result;
end;

function ConstType{(cn : ConstNode) : TypeNode};
var
    tn : TypeNode;
begin
    if TraceNexpr then begin
	write(output,'ConstType(',cn^.kind:1,')=');
	WriteConstant(output,cn);
	writeln(output);
    end;
    case cn^.kind of
	DTINTEGER : tn := integerTypeNode;
	DTCARDINAL : begin
	    if cn^.cardVal > MAXINT then begin
		tn := cardinalTypeNode;
	    end else begin
		tn := cardIntTypeNode;
	    end;
	end;
	DTREAL : tn := realConstTypeNode;
	DTCHAR : tn := charConstTypeNode;
	DTBOOLEAN : tn := booleanTypeNode;
	DTSTRING : begin
	    tn := NewTypeNode(DTSTRING);
	    tn^.stringLength := cn^.strVal^.length;
	    tn^.size := tn^.stringLength * CHARSIZE;
	end;
	DTENUMERATION : tn := cn^.enumVal^.enumType;
	DTPOINTER : tn := addressTypeNode;
	DTPROC : tn := cn^.procVal^.procType;
	DTSET : tn := cn^.setVal^.setType;
    end;
    ConstType := tn;
end;

procedure InsertCheckExpr{(en : ExprNode; check : CheckKind; vn : VarNode;
	    tn : TypeNode; lowerBound, upperBound : cardinal)};
var
    oen : ExprNode;
begin
    oen := NewExprNode(en^.kind);
    oen^ := en^;
    en^.kind := EXPRCHECK;
    en^.exprCheck := check;
    en^.checkExpr := oen;
    en^.checkVar := vn;
    en^.checkType := tn;
    en^.checkField := nil;
    en^.checkLower := lowerBound;
    en^.checkUpper := upperBound;
end;

function ImplQualExpr(en : ExprNode; sym : Symbol; mode : EvalMode) : TypeNode;
var
    qen, aen : ExprNode;
    wqn : WithQualNode;
    fn : FieldNode;
    found : boolean;
    tn : TypeNode;
begin
    found := false;
    fn := sym^.symField;
    wqn := withQualList;
    while not found and (wqn <> nil) do begin
	if fn^.recType = wqn^.recType then begin
	    found := true;
	end else begin
	    wqn := wqn^.next;
	end;
    end;
    tn := nil;
    if not found then begin
	ExprErrorName(en,sym^.name,'Field used without qualification');
	BadExpr(en);
    end else begin
	{ found field in a with statemtent: generate implQual^.field }
	qen := NewExprNode(EXPRVAR);		{ node for implQual }
	SameExprLine(qen,en);
	qen^.exprVar := wqn^.implQual;
	aen := NewExprNode(EXPRDEREF);		{ node for implQual^ }
	SameExprLine(aen,en);
	aen^.ptr := qen;
	aen^.realPtr := false;
	en^.kind := EXPRDOT;			{ node for implQual^.field }
	en^.rec := aen;
	en^.field := fn;
	en^.fieldName := nil;
	tn := DotExpr(en,mode);
	{ this expression depends on the var in the with expression }
	en^.baseVar := wqn^.baseVar;
	en^.basePtrType := wqn^.basePtrType;
    end;
    ImplQualExpr := tn;
end;

function RecordExpr(en : ExprNode; sym : Symbol; names : IdentList;
	mode : EvalMode) : TypeNode;
var
    error : boolean;
    id, idnext : IdentNode;
    vt, rentn : TypeNode;
    ren, fen : ExprNode;
begin
    { get expression for record symbol }
    ren := NewExprNode(EXPRSYM);
    SameExprLine(ren,en);
    ren^.exprSym := sym;
    { add field qualifications to record }
    id := names^.first;
    while id <> nil do begin
	idnext := id^.next;
	if idnext = nil then begin
	    fen := en;	{ use original en for final field }
	    fen^.kind := EXPRDOT;
	end else begin
	    fen := NewExprNode(EXPRDOT);    { otherwise, get new one}
	    SameExprLine(fen,ren);
	end;
	fen^.rec := ren;
	fen^.fieldName := id^.name;
	ren := fen;
	dispose(id);
	id := idnext;
    end;
    dispose(names);
    assert (en = ren);
    en := ren;
    RecordExpr := DotExpr(en,mode);
end;

function SymExpr(en : ExprNode; mode : EvalMode) : TypeNode;
var
    sym : Symbol;
    cn : ConstNode;
    tn : TypeNode;
begin
    if TraceNexpr then begin
	writeln(output,'SymExpr');
    end;
    sym := en^.exprSym;
    tn := nil;
    if sym = nil then begin
	BadExpr(en);
    end else if sym^.kind = SYMCONST then begin
	en^.kind := EXPRCONST;
	en^.exprConst := sym^.symConst;
	tn := ConstExpr(en,mode);
    end else if sym^.kind = SYMVAR then begin
	en^.kind := EXPRVAR;
	en^.exprVar := sym^.symVar;
	tn := VarExpr(en,mode);
    end else if sym^.kind = SYMFIELD then begin
	tn := ImplQualExpr(en,sym,mode);
    end else if sym^.kind = SYMPROC then begin
	new(cn);
	cn^.kind := DTPROC;
	cn^.procVal := sym^.symProc;
	en^.kind := EXPRCONST;
	en^.exprConst := cn;
	tn := ConstExpr(en,mode);
	if mode = EVALGET then begin
	    sym^.symProc^.internalProc := false;
	end;
    end else if sym^.kind = SYMENUM then begin
	new(cn);
	cn^.kind := DTENUMERATION;
	cn^.enumVal := sym^.symEnum;
	en^.kind := EXPRCONST;
	en^.exprConst := cn;
	tn := ConstExpr(en,mode);
    end else if sym^.kind = SYMTYPE then begin
	en^.kind := EXPRSYM;
	en^.exprSym := sym;
	en^.exprType := ActualType(sym^.symType);
	tn := en^.exprType;
    end else begin
	ExprErrorName(en,sym^.name,'Symbol not valid in expression');
	BadExpr(en);
    end;
    SymExpr := tn;
end;

function NameExpr(en : ExprNode; mode : EvalMode) : TypeNode;
var
    sym : Symbol;
    cn : ConstNode;
    tn : TypeNode;
begin
    if TraceNexpr then begin
	writeln(output,'NameExpr');
    end;
    tn := nil;
    sym := QualifiedName(en^.exprName);
    if sym = nil then begin
	BadExpr(en);
    end else if en^.exprName^.first <> nil then begin
	{ more qualifiers, must be a record }
	tn := RecordExpr(en,sym,en^.exprName,mode);
    end else begin
	en^.kind := EXPRSYM;
	en^.exprSym := sym;
	tn := SymExpr(en,mode);
    end;
    NameExpr := tn;
end;

{ Insert a value node if necessary (mode = EVALGET). }
{  Change expression type to address.  }
procedure ValueOrAddr{(en : ExprNode; tn : TypeNode; mode : EvalMode)};
var
    nen : ExprNode;
begin
    if TraceNexpr then begin
	write(output,'ValueOrAddr ');
	WriteExpr(output,en);
	writeln(output);
    end;
    if mode = EVALGET then begin
	nen := NewExprNode(en^.kind);
	nen^ := en^;
	nen^.exprType := addressTypeNode;
	en^.kind := EXPRVAL;
	en^.exprVal := nen;
	en^.dependVar := nen^.baseVar;
	en^.dependPtrType := nen^.basePtrType;
	en^.exprType := tn;
    end else begin
	en^.exprType := addressTypeNode;
    end;
end;


{  Add a dereference if the expression is an open array }
procedure RefOpenArray{(en : ExprNode; tn : TypeNode)};
begin
    if tn^.kind = DTARRAY then begin
	if tn^.arrayKind = ARRAYOPEN then begin
	    ValueOrAddr(en,addressTypeNode,EVALGET);
	end;
    end;
end;

{ The following are the real expression manipulating routines. }
{   The above name-resolving routines "always" call them. }
{   ( exceptions are errors, type names, procedure names, etc.) }

function ConstExpr{(en : ExprNode; mode : EvalMode) : TypeNode};
begin
    if TraceNexpr then begin
	write(output,'ConstExpr(',en^.exprConst^.kind:1,')=');
	WriteConstant(output,en^.exprConst);
	writeln(output);
    end;
    if mode = EVALPUT then begin
	ExprErrorName(en,stringDataType[en^.exprConst^.kind],
		'Constant must not be changed');
	BadExpr(en);
    end else begin
	en^.exprType := ConstType(en^.exprConst);
	en^.constType := en^.exprType;
    end;
    ConstExpr := en^.exprType;
end;

function VarExpr{(en : ExprNode; mode : EvalMode) : TypeNode};
var
    tn : TypeNode;
    vn : VarNode;
begin
    vn := en^.exprVar;
    tn := ActualType(vn^.varType);
    if vn^.indirect then begin
	{ insert value node to get address of variable }
	en^.baseVar := vn;
	ValueOrAddr(en,addressTypeNode,EVALGET);
	{ indirection is not real value }
	en^.dependVar := nil;
	en^.dependPtrType := nil;
    end;
    if mode = EVALPUT then begin
	vn^.changed := true;
    end;
    en^.baseVar := vn;
    ValueOrAddr(en,tn,mode);
    VarExpr := tn;
end;

function UnOpExpr{(en : ExprNode; mode : EvalMode) : TypeNode};
var
    opndtn, et : TypeNode;
    opnd : ExprNode;
    con : ConstNode;
begin
    if TraceNexpr then begin
	writeln(output,'UnOpExpr(',en^.exprUnOp:0,',',mode:0,')');
    end;
    opndtn := BaseType(CheckExpr(en^.opnd,EVALGET));
    en^.unOperType := opndtn;
    opnd := en^.opnd;
    et := nil;
    if mode <> EVALGET then begin
	ExprErrorName(en,stringToken[en^.exprUnOp],
		'Expression must be used as a value');
    end else if opndtn = nil then begin
	{ do nothing }
    end else if en^.exprUnOp in [TKPLUS,TKMINUS] then begin
	if opndtn^.kind in [DTINTEGER, DTCARDINAL, DTREAL, DTLONGREAL]
	then begin
	    et := opndtn;
	end;
    end else if en^.exprUnOp = TKNOT then begin
	if opndtn^.kind = DTBOOLEAN then begin
	    et := opndtn;
	end;
    end else begin
	ExprErrorName(en,stringToken[en^.exprUnOp],'Unexpected unary operator');
    end;
    if et = nil then begin
	ExprErrorName(en,stringDataType[opnd^.exprType^.kind],
		'Invalid operand type for unary operator');
	BadExpr(en);
    end else begin
	en^.exprType := ActualType(et);
	if en^.opnd^.kind = EXPRCONST then begin
	    con := UnOpConst(en^.exprUnOp,en^.opnd^.exprConst);
	    en^.kind := EXPRCONST;
	    en^.exprConst := con;
	    en^.constType := en^.exprType;
	end;
    end;
    UnOpExpr := en^.exprType;
end;

procedure EvalConstBinOpExpr{(en : ExprNode)};
var
    con : ConstNode;
begin
    if (en^.opnd1^.kind = EXPRCONST) and (en^.opnd2^.kind = EXPRCONST)
    then begin
	con := BinOpConst(en^.exprBinOp,en^.opnd1^.exprConst,
		en^.opnd2^.exprConst,true);
	en^.kind := EXPRCONST;
	en^.exprConst := con;
	en^.constType := en^.exprType;
    end;
end;

{

    The following are legal binary operations.  In all cases except IN,
	operands must be compatible.
    
    e in s: result boolean
	e must be compatible with range of set s

    relations: result boolean
	    I  C  R  S  B  St Ch E  P  A  R
	=   +  +  +  +  +  +  +  +  +  +  +
	#   +  +  +  +  +  +  +  +  +  +  +
	/=  +  +  +  +  +  +  +  +  +  +  +
	>=  +  +  +  +  +  +  +  +
	>   +  +  +     +  +  +  +
	<=  +  +  +  +  +  +  +  +
	<   +  +  +     +  +  +  +

    arithmetic and set operations: result operand type
	    I  C  R  S
	+   +  +  +  +
	-   +  +  +  +
	*   +  +  +  +
	/         +  +
	div +  +
	mod +  +
    
    boolean operations: result boolean
	    B
	and +
	&   +
	or  +
}
function BinOpExpr(en : ExprNode; mode : EvalMode) : TypeNode;
var
    opndType, resultType, setType : TypeNode;
    opnd1, opnd2, nen : ExprNode;
    opnd1tn, opnd2tn : TypeNode;
    oper : Token;
    con : ConstNode;
    lowerBound : cardinal;
begin
    if TraceNexpr then begin
	writeln(output,'BinExprNode(',oper:0,',',mode:0,')');
    end;
    oper := en^.exprBinOp;;
    resultType := nil;
    opnd1tn := CheckExpr(en^.opnd1,EVALGET);
    opnd1 := en^.opnd1;
    opnd2tn := CheckExpr(en^.opnd2,EVALGET);
    opnd2 := en^.opnd2;
    if mode <> EVALGET then begin
	ExprErrorName(en,stringToken[en^.exprBinOp],
		'Expression must be used as a value');
    end else if IsBadExpr(opnd1) or IsBadExpr(opnd2) then begin
	{ do nothing }
    end else if oper = TKIN then begin
{ IN }
	setType := BaseType(opnd2tn);
	if setType^.kind <> DTSET then begin
	    ExprError(en,'Right operand of IN is not a set');
	end else begin
	    opndType := Compatible(opnd1tn,opnd1,setType^.setRange,nil);
	    if opndType = nil then begin
		ExprError(en,'Left operand of IN does not match type of set');
	    end else begin
		opndType := BaseType(opndType);
		en^.operType := opndType;
		lowerBound := LowerBoundOf(setType^.setRange);
		if lowerBound <> 0 then begin
		    { replace opnd1 with (opnd1 - lowerBound) }
		    nen := NewExprNode(opnd1^.kind);
		    nen^ := opnd1^;
		    opnd1^.kind := EXPRBINOP;
		    opnd1^.exprBinOp := TKMINUS;
		    opnd1^.opnd1 := nen;
		    opnd1^.opnd2 := ConstExprNode(CardinalConst(lowerBound));
		    opnd1^.opnd2^.exprType := nen^.exprType;
		    opnd1^.operType := nen^.exprType;
		end;
		resultType := booleanTypeNode;
	    end;
	end;
    end else begin
	opndType := Compatible(opnd1tn,opnd1,opnd2tn,opnd2);
	if opndType = nil then begin
	    ExprErrorName(en,stringToken[oper],
		    'Incompatible operands on binary operator');
	end else begin
	    opndType := BaseType(opndType);
	    en^.operType := opndType;
	    if oper in [TKEQUALS, TKNOTEQUAL, TKSHARP, TKLESS, TKGREATER,
		    TKLSEQUAL, TKGREQUAL]
	    then begin
{ RELATIONAL }
		if (opndType^.kind in [DTINTEGER, DTBOOLEAN, DTCHAR, DTSTRING,
			DTREAL, DTLONGREAL, DTCARDINAL, DTENUMERATION]) or
			(opndType = addressTypeNode)
		then begin
		    resultType := booleanTypeNode;
		end else if (opndType^.kind = DTSET) and
			not (oper in [TKLESS, TKGREATER])
		then begin
		    if oper = TKGREQUAL then begin
			{ change >= to <= with switched operands }
			en^.opnd1 := opnd2;
			en^.opnd2 := opnd1;
			en^.exprBinOp := TKLSEQUAL;
		    end;
		    resultType := booleanTypeNode;
		end else if (opndType^.kind = DTPOINTER)
			and (oper in [TKEQUALS, TKNOTEQUAL, TKSHARP])
		then begin
		    resultType := booleanTypeNode;
		end else if (opndType^.kind = DTARRAY)
			and (oper in [TKEQUALS, TKNOTEQUAL, TKSHARP])
		then begin
		    if ActualType(opndType^.elementType) = charTypeNode
		    then begin
			resultType := booleanTypeNode;
		    end;
		end;
		if resultType = nil then begin
		    ExprErrorName(en,stringToken[oper],
			    'Operand types invalid for operator');
		end;
	    end else if oper in [TKPLUS, TKMINUS, TKASTERISK, TKSLASH, TKDIV,
		    TKMOD]
	    then begin
{ ARITHMETIC }
		if ((opndType^.kind in [DTINTEGER, DTCARDINAL])
			    or (opndType = addressTypeNode))
		and
			(oper <> TKSLASH)
		then begin
		    resultType := opndType;
		end else if (opndType^.kind in [DTREAL,DTLONGREAL]) and
			not (oper in [TKDIV,TKMOD])
		then begin
		    resultType := opndType;
		end else if (opndType^.kind = DTSET) and
			not (oper in [TKDIV,TKMOD])
		then begin
		    resultType := opndType;
		end;
	    end else if oper in [TKAND, TKOR, TKAMPERSAND] then begin
		if opndType^.kind = DTBOOLEAN then begin
		    resultType := booleanTypeNode;
		end;
	    end;
	    if resultType = nil then begin
		ExprErrorName(en,stringToken[oper],
			'Operand types invalid for operator');
	    end;
	end;
    end;
    if resultType = nil then begin
	BadExpr(en);
    end else begin
	en^.exprType := ActualType(resultType);
	EvalConstBinOpExpr(en);
    end;
    BinOpExpr := en^.exprType;
end;

procedure FactorInConstants(var en : ExprNode; var total, multiple : cardinal;
	multiply : boolean);
var
    tmpen, olden : ExprNode;
begin
    olden := en;
    { en will be multiplied by multiple and added to total }
    { update en, total, and multiple to try to improve things }
    if en^.kind = EXPRBINOP then begin
	if en^.exprBinOp = TKPLUS then begin
	    if en^.opnd1^.kind = EXPRCONST then begin
		total := total + OrdOf(en^.opnd1^.exprConst) * multiple;
		en := en^.opnd2;
	    end else if en^.opnd2^.kind = EXPRCONST then begin
		total := total + OrdOf(en^.opnd2^.exprConst) * multiple;
		en := en^.opnd1;
	    end;
	end else if en^.exprBinOp = TKMINUS then begin
	    if en^.opnd2^.kind = EXPRCONST then begin
		total := total - OrdOf(en^.opnd2^.exprConst) * multiple;
		en := en^.opnd1;
	    end;
	end else if multiply and (en^.exprBinOp = TKASTERISK) then begin
	    if en^.opnd1^.kind = EXPRCONST then begin
		multiple := multiple * OrdOf(en^.opnd1^.exprConst);
		en := en^.opnd2;
	    end else if en^.opnd2^.kind = EXPRCONST then begin
		multiple := multiple * OrdOf(en^.opnd2^.exprConst);
		en := en^.opnd1;
	    end;
	end;
    end;
    if en <> olden then begin
	FactorInConstants(en,total,multiple,multiply);
    end;
end;

function SubscriptExpr(en : ExprNode; mode : EvalMode) : TypeNode;
var
    sen, sennext, multen, adden, newadden, aen : ExprNode;
    rowtn, sentn : TypeNode;
    subnum : integer;
    error : boolean;
    subOffset, subMultiple, rowsize, lowerBound, upperBound, baseOffset
	    : cardinal;
begin
    if TraceNexpr then begin
	writeln(output,'SubscriptExpr');
    end;
    error := false;
    aen := en^.arr;
    if mode = EVALPUT then begin
	rowtn := BaseType(CheckExpr(aen,EVALPUT));
    end else begin
	rowtn := BaseType(CheckExpr(aen,EVALPOINT));
    end;
    if IsBadExpr(aen) then begin
	{ do nothing }
	error := true;
    end else if rowtn^.kind <> DTARRAY then begin
	ExprError(en,'Subscripted expression not an array');
	error := true;
    end else begin
	RefOpenArray(aen,rowtn);
	subnum := 1;
	adden := aen;
	sen := en^.subscripts^.first;
	baseOffset := 0;
	while not error and (sen <> nil) and (rowtn <> nil) do begin
	    sennext := sen^.next;
	    sentn := CheckExpr(sen,EVALGET);
	    rowtn := BaseType(rowtn);
	    if rowtn^.kind <> DTARRAY then begin
		ExprErrorNumber(en,'Too many subscripts, subscript #',subnum);
		error := true;
	    end else if rowtn^.indexType = nil then begin
		{ open array, must be cardinal subscript }
		if Assignable(cardinalTypeNode,sentn,sen) = nil then begin
		    ExprErrorNumber(en,'Incompatible type, subscript #',subnum);
		    error := true;
		end;
	    end else begin
		if Assignable(rowtn^.indexType,sentn,sen) = nil then begin
		    ExprErrorNumber(en,'Incompatible type, subscript #',subnum);
		    error := true;
		end;
	    end;
	    if not error then begin
		{ lower bound of index }
		lowerBound := LowerBoundOf(rowtn^.indexType);
		upperBound := UpperBoundOf(rowtn^.indexType);
		subOffset := 0;
		subMultiple := 1;
		{ do not factor out constants on checked open array }
		if not genCheckFlag or (rowtn^.indexType <> nil) then begin
		    FactorInConstants(sen,subOffset,subMultiple,true);
		end;
		if genCheckFlag then begin
		    if rowtn^.indexType <> nil then begin
			{ ceiling on div of lower bound, floor on upper bound }
			InsertCheckExpr(sen,CHECKSUBSCR,nil,sentn,
			    CardDiv(lowerBound-subOffset+(subMultiple-1),
				    subMultiple),
			    CardDiv(upperBound-subOffset,subMultiple));
		    end else if rowtn^.nocount then begin
			{ no check }
		    end else begin
			{ for open array, always 0..count-1 }
			InsertCheckExpr(sen,CHECKSUBSCROPEN,aen^.baseVar,
				cardinalTypeNode,0,0);
		    end;
		end;
		rowtn := rowtn^.elementType;
		{ size of row }
		rowsize := SizeOf(rowtn);
		{ accumulate offset of base of the array }
		baseOffset := baseOffset + subOffset * rowsize
				- lowerBound * subMultiple * rowsize;
		rowsize := rowsize * subMultiple;
		{ multiply subscript by rowsize }
		multen := NewExprNode(EXPRBINOP);
		SameExprLine(multen,sen);
		multen^.exprBinOp := TKASTERISK;
		multen^.opnd1 := sen;
		multen^.opnd2 := ConstExprNode(CardinalConst(rowsize));
		multen^.opnd2^.exprType := addressTypeNode;
		EvalConstBinOpExpr(multen);
		multen^.exprType := addressTypeNode;
		multen^.operType := addressTypeNode;
		{ add to other subscripts }
		if adden = nil then begin
		    adden := multen;
		end else begin
		    newadden := NewExprNode(EXPRBINOP);
		    SameExprLine(newadden,sen);
		    newadden^.exprBinOp := TKPLUS;
		    newadden^.opnd1 := adden;
		    newadden^.opnd2 := multen;
		    adden := newadden;
		    EvalConstBinOpExpr(adden);
		    adden^.exprType := addressTypeNode;
		    adden^.operType := addressTypeNode;
		end;
	    end;
	    sen := sennext;
	end;
    end;
    if error then begin
	BadExpr(en);
	rowtn := nil;
    end else begin
	sennext := en^.next;
	en^ := adden^;
	en^.next := sennext;
	en^.baseVar := aen^.baseVar;
	en^.basePtrType := aen^.basePtrType;
	if baseOffset <> 0 then begin
	    { add in constant offset }
	    newadden := NewExprNode(EXPRBINOP);
	    newadden^ := en^;
	    en^.exprBinOp := TKPLUS;
	    en^.opnd1 := newadden;
	    en^.opnd2 := ConstExprNode(CardinalConst(baseOffset));
	    en^.opnd2^.exprType := addressTypeNode;
	    en^.baseVar := aen^.baseVar;
	    en^.basePtrType := aen^.basePtrType;
	    EvalConstBinOpExpr(en);
	    en^.exprType := addressTypeNode;
	    en^.operType := addressTypeNode;
	end;
	ValueOrAddr(en,rowtn,mode);
    end;
    SubscriptExpr := rowtn;
end;

function DotExpr{(en : ExprNode) : TypeNode};
var
    rectn, fieldtn : TypeNode;
    sym : Symbol;
    rec, saveNext : ExprNode;
    field : FieldNode;
    offset, multiple : MemoryOffset;
begin
    if TraceNexpr then begin
	writeln(output,'DotExpr');
    end;
    if mode = EVALPUT then begin
	rectn := BaseType(CheckExpr(en^.rec,EVALPUT));
    end else begin
	rectn := BaseType(CheckExpr(en^.rec,EVALPOINT));
    end;
    rec := en^.rec;
    field := nil;
    if IsBadExpr(rec) then begin
	{ do nothing }
    end else if rectn^.kind <> DTRECORD then begin
	ExprError(en,'Dot follows non-record expression');
    end else if en^.field <> nil then begin
	field := en^.field;
    end else begin
	sym := LookUpSymbol(en^.fieldName,rectn^.recScope,ONECASE);
	if sym = nil then begin
	    ExprErrorName(en,en^.fieldName,'Not a field of this record');
	end else begin
	    field := sym^.symField;
	end;
    end;
    if field = nil then begin
	BadExpr(en);
	fieldtn := nil;
    end else begin
	fieldtn := ActualType(field^.fieldType);
	{ combine field offset with any other constants }
	offset := field^.offset;
	multiple := 1;
	FactorInConstants(rec,offset,multiple,false);
	if genCheckFlag and (field^.containingVariant <> nil) then begin
	    if field^.containingVariant^.tagField <> nil then begin
		InsertCheckExpr(rec,CHECKVARIANT,nil,nil,0,0);
		rec^.checkField := field;
	    end;
	end;
	if offset = 0 then begin
	    { no need to do add }
	    saveNext := en^.next;
	    en^ := rec^;
	    en^.next := saveNext;
	end else begin
	    { add in offset }
	    en^.kind := EXPRBINOP;
	    en^.exprBinOp := TKPLUS;
	    en^.opnd1 := rec;
	    en^.opnd2 := ConstExprNode(CardinalConst(offset));
	    en^.opnd2^.exprType := addressTypeNode;
	    en^.baseVar := rec^.baseVar;
	    en^.basePtrType := rec^.basePtrType;
	    en^.exprType := addressTypeNode;
	    en^.operType := addressTypeNode;
	end;
	ValueOrAddr(en,fieldtn,mode);
    end;
    DotExpr := fieldtn;
end;

function DerefExpr{(en : ExprNode) : TypeNode};
var
    tn, ptrtn : TypeNode;
    ptr, saveNext : ExprNode;
begin
    if TraceNexpr then begin
	writeln(output,'DerefExpr');
    end;
    tn := BaseType(CheckExpr(en^.ptr,EVALGET));
    ptr := en^.ptr;
    ptrtn := nil;
    if tn = nil then begin
	{ do nothing }
    end else if tn^.kind = DTPOINTER then begin
	ptrtn := ActualType(tn^.toType);
	if ptrtn = nil then begin
	    ExprError(en,'Dereference of pointer to unknown type');
	end;
    end else begin
	ExprError(en,'Dereference of a non-pointer');
    end;
    if ptrtn = nil then begin
	BadExpr(en);
    end else begin
	{ convert deref expr into check of appropriate kind }
	en^.kind := EXPRCHECK;
	en^.exprCheck := tn^.ptrCheck;
	en^.checkExpr := ptr;
	en^.exprType := ptr^.exprType;
	en^.checkVar := nil;
	en^.checkType := nil;
	en^.checkField := nil;
	en^.checkLower := 0;
	en^.checkUpper := 0;
	ValueOrAddr(en,ptrtn,mode);
	en^.basePtrType := ptrtn;
    end;
    DerefExpr := ptrtn;
end;

procedure ExpandOpenArrayParam(pexp : ExprNode; pexptn, parmtn : TypeNode;
	    var newParams : ExprList; pnum : integer);
var
    numElements, elementSize, arraySize : cardinal;
    newpexp, varexp, valexp, mulexp, addexp, divexp : ExprNode;
    done : boolean;
begin
    done := false;
    arraySize := SizeOf(pexptn);
    pexptn := BaseType(pexptn);
    if parmtn^.nocount then begin
	{ just put address on list }
	RefOpenArray(pexp,pexptn);
	newParams := AddToExprList(newParams,pexp);
	done := true;
    end else if pexptn^.kind = DTSTRING then begin
	if parmtn^.elementType = wordTypeNode then begin
	     { calculate words for array of word }
	     numElements := CardDiv(RoundUp(pexptn^.stringLength*CHARSIZE,
						WORDSIZE),WORDSIZE);
	end else if parmtn^.elementType = byteTypeNode then begin
	     { number of bytes = number of chars }
	     numElements := pexptn^.stringLength;
	end else begin
	     numElements := pexptn^.stringLength;
	end;
	newpexp := ConstExprNode(CardinalConst(numElements));
	newpexp^.exprType := cardIntTypeNode;
	SameExprLine(newpexp,pexp);
	{ put address on list }
	newParams := AddToExprList(newParams,pexp);
	{ put number of elements on list }
	newParams := AddToExprList(newParams,newpexp);
	done := true;
    end else if pexptn^.kind = DTARRAY then begin
	if pexptn^.nocount then begin
	    ExprErrorNumber(pexp,'Cannot pass NOCOUNT array as an open array parameter #',pnum);
	    done := true;
	end else if pexptn^.arrayKind = ARRAYOPEN then begin
	    { it better be a variable }
	    if pexp^.kind <> EXPRVAR then begin
		ExprErrorNumber(pexp,'Open array actual parameter must be variable, parameter #',
				pnum);
	    end else begin
		{ open array parameter, need to get run-time size }
		{ get number of elements in open array }
		    { address of open array descriptor }
		varexp := NewExprNode(EXPRVAR);
		SameExprLine(varexp,pexp);
		varexp^.exprVar := pexp^.exprVar;
		varexp^.exprType := addressTypeNode;
		    { add wordsize to get address of number of elements }
		addexp := NewExprNode(EXPRBINOP);
		SameExprLine(addexp,pexp);
		addexp^.exprBinOp := TKPLUS;
		addexp^.opnd1 := varexp;
		addexp^.opnd2 := ConstExprNode(CardinalConst(WORDSIZE));
		addexp^.opnd2^.exprType := addressTypeNode;
		addexp^.exprType := addressTypeNode;
		addexp^.operType := addressTypeNode;
		    { get number of elements }
		valexp := NewExprNode(EXPRVAL);
		SameExprLine(valexp,pexp);
		valexp^.exprVal := addexp;
		valexp^.exprType := cardinalTypeNode;

		elementSize := SizeOf(pexptn^.elementType);
		if (pexptn^.elementType = wordTypeNode) and
		    (elementSize <> WORDSIZE)
		then begin
		    { need to scale size to words }
			{ multiply number of elements times size }
		    mulexp := NewExprNode(EXPRBINOP);
		    SameExprLine(mulexp,pexp);
		    mulexp^.exprBinOp := TKASTERISK;
		    mulexp^.opnd1 := valexp;
		    mulexp^.opnd2 := ConstExprNode(CardinalConst(elementSize));
		    mulexp^.opnd2^.exprType := addressTypeNode;
		    mulexp^.exprType := addressTypeNode;
		    mulexp^.operType := addressTypeNode;
			{ add wordsize - 1 }
		    addexp := NewExprNode(EXPRBINOP);
		    SameExprLine(addexp,pexp);
		    addexp^.exprBinOp := TKPLUS;
		    addexp^.opnd1 := mulexp;
		    addexp^.opnd2 := ConstExprNode(CardinalConst(WORDSIZE-1));
		    addexp^.opnd2^.exprType := addressTypeNode;
		    addexp^.exprType := addressTypeNode;
		    addexp^.operType := addressTypeNode;
			{ divide by wordsize }
		    divexp := NewExprNode(EXPRBINOP);
		    SameExprLine(divexp,pexp);
		    divexp^.exprBinOp := TKDIV;
		    divexp^.opnd1 := valexp;
		    divexp^.opnd2 := ConstExprNode(CardinalConst(WORDSIZE));
		    divexp^.opnd2^.exprType := addressTypeNode;
		    divexp^.exprType := cardinalTypeNode;
		    divexp^.operType := cardinalTypeNode;

		    newpexp := divexp;
		end else if (pexptn^.elementType = byteTypeNode) and
		    (elementSize <> BYTESIZE)
		then begin
		    { need to scale size to bytes }
		    { simpler than words, since always a multiple of bytes }
			{ multiply number of elements times size in bytes }
		    mulexp := NewExprNode(EXPRBINOP);
		    SameExprLine(mulexp,pexp);
		    mulexp^.exprBinOp := TKASTERISK;
		    mulexp^.opnd1 := valexp;
		    mulexp^.opnd2 := ConstExprNode(CardinalConst(
					CardDiv(elementSize,BYTESIZE)));
		    mulexp^.opnd2^.exprType := addressTypeNode;
		    mulexp^.exprType := addressTypeNode;
		    mulexp^.operType := addressTypeNode;

		    newpexp := mulexp;
		end else begin
		    { number of elements is right }
		    newpexp := valexp;
		end;
	    end;
	    RefOpenArray(pexp,pexptn);
	    { put address on list }
	    newParams := AddToExprList(newParams,pexp);
	    { put number of elements on list }
	    newParams := AddToExprList(newParams,newpexp);
	    done := true;
	end;
    end;
    if not done then begin
	{ fixed size variable is being passed }
	if parmtn^.elementType = wordTypeNode then begin
	    { calculate words for array of word }
	    numElements := CardDiv(RoundUp(arraySize,WORDSIZE),WORDSIZE);
	end else if parmtn^.elementType = byteTypeNode then begin
	    { calculate bytes for array of byte }
	    numElements := CardDiv(RoundUp(arraySize,BYTESIZE),BYTESIZE);
	end else if pexptn^.kind = DTARRAY then begin
	    numElements := NumberOf(pexptn^.indexType);
	end else begin
	    numElements := 0;
	    ExprErrorNumber(pexp,'Open array parameter must be an array, parameter #',
				pnum);
	end;
	newpexp := ConstExprNode(CardinalConst(numElements));
	newpexp^.exprType := cardinalTypeNode;
	SameExprLine(newpexp,pexp);
	{ put address on list }
	newParams := AddToExprList(newParams,pexp);
	{ put number of elements on list }
	newParams := AddToExprList(newParams,newpexp);
    end;
end;

function CheckFuncProc{(isFunc : boolean; en, procExpr : ExprNode;
    var params : ExprList; var retType : TypeNode) : boolean};
var
    parm : ParamNode;
    pexp, pexpnext : ExprNode;
    pnum : integer;
    tn : TypeNode;
    checked, error : boolean;
    proc : ProcNode;
    procType, pexptn, parmtn : TypeNode;
    newParams : ExprList;
    errorName : String;
    mode : EvalMode;
begin

    checked := false;
    error := true;
    procType := procExpr^.exprType;
    errorName := nil;
    if procExpr^.kind = EXPRSYM then begin
	errorName := procExpr^.exprSym^.name;
    end else if procExpr^.kind = EXPRCONST then begin
	if procExpr^.exprConst^.kind = DTPROC then begin
	    errorName := procExpr^.exprConst^.procVal^.name;
	end;
    end;
    
    { beware: type names can be used as both types and funcs }
    if procExpr^.kind = EXPRSYM then begin
	if procExpr^.exprSym^.kind = SYMTYPE then begin
	    retType := procExpr^.exprSym^.symType;
	    error := true;
	    if params^.first = nil then begin
		ExprErrorName(procExpr,errorName,
			'Type transfer function requires a parameter');
	    end else if params^.first^.next <> nil then begin
		ExprErrorName(procExpr,errorName,
			'Type transfer function must have only one parameter');
	    end else begin
		pexptn := CheckExpr(params^.first,EVALGET);
		if pexptn = nil then begin
		    { found an error }
		end else if WordSizeOf(pexptn) <> WordSizeOf(retType) then begin
		    ExprErrorName(procExpr,errorName,
			    'Type transfer function cannot change size');
		end else begin
		    error := false;
		end;
	    end;
	    checked := true;
	end;
    end;

    { check for builtin function (must be a constant) }
    if checked then begin
	{ do nothing }
    end else if procExpr^.kind = EXPRCONST then begin
	if procExpr^.exprConst^.kind = DTPROC then begin
	    proc := procExpr^.exprConst^.procVal;
	    if proc^.builtin <> BIPNOTBIP then begin
		{ pass proc constant by var in case CheckBuiltin updates it }
		error := not CheckBuiltin(isFunc,en,
		    procExpr^.exprConst^.procVal,params,retType);
		checked := true;
	    end;
	end;
    end;

    if checked then begin
	 { do nothing }
    end else if procType^.kind <> DTPROC then begin
	ExprErrorName(procExpr,errorName,
		'Non-procedure used as a procedure/function');
    end else begin
	if isFunc and (procType^.funcType = nil) then begin
	    ExprErrorName(procExpr,errorName,'Procedure used as a function');
	end else if not isFunc and (procType^.funcType <> nil) then begin
	    ExprErrorName(procExpr,errorName,'Function used as a procedure');
	end else begin
	    error := false;
	    if (params = nil) or (procType^.paramList = nil) then begin
		{ make sure they match }
		if params <> nil then begin
		    if params^.first <> nil then begin
			ExprErrorName(procExpr,errorName,
			    'Too many parameters on procedure/function');
			error := true;
		    end;
		end;
		if procType^.paramList <> nil then begin
		    if procType^.paramList^.first <> nil then begin
			ExprErrorName(procExpr,errorName,
			    'Not enough parameters on procedure/function');
			error := true;
		    end;
		end;
	    end else begin
		pnum := 0;
		pexp := params^.first;
		parm := procType^.paramList^.first;
		newParams := AddToExprList(nil,nil);
		while (pexp <> nil) and (parm <> nil) do begin
		    pnum := pnum + 1;
		    pexpnext := pexp^.next;
		    parmtn := BaseType(parm^.paramType);
		    { decide how parameter will be evaluated }
		    if parm^.kind in [PARAMVAR,PARAMARRAYVAR] then begin
			mode := EVALPUT;
		    end else if parm^.reference then begin
			mode := EVALPOINT;
		    end else begin
			mode := EVALGET;
		    end;
		    pexptn := CheckExpr(pexp,mode);
		    if IsBadExpr(pexp) then begin
			error := true;
		    end else begin
			if not Passable(parm^.paramType,parm^.kind,pexptn,pexp)
			then begin
			    ExprErrorNameNumber(pexp,errorName,
				    'Wrong type, parameter #',pnum);
			    error := true;
			end else if (parm^.kind in [PARAMVAR,PARAMARRAYVAR])
			    and not IsAddressableExpr(pexp)
			then begin
			    ExprErrorNameNumber(pexp,errorName,
				'VAR parameter not variable, parameter #',pnum);
			    error := true;
			end else if parm^.kind in [PARAMARRAYVALUE,
					PARAMARRAYVAR]
			then begin
			    { change open array parameter into two parameters }
			    ExpandOpenArrayParam(pexp,pexptn,parmtn,newParams,
					pnum);
			end else begin
			    if (parm^.kind = PARAMVALUE) and
				    (parm^.paramType^.kind = DTSUBRANGE)
			    then begin
				InsertCheckExpr(pexp,CHECKRANGE,nil,
				    pexptn,
				    LowerBoundOf(parm^.paramType),
				    UpperBoundOf(parm^.paramType));
			    end;
			    newParams := AddToExprList(newParams,pexp);
			end;
		    end;
		    pexp := pexpnext;
		    parm := parm^.next;
		end;
		params := newParams;
		if parm <> nil then begin
		    ExprErrorName(procExpr,errorName,
			'Not enough parameters on procedure/function');
		    error := true;
		end;
		if pexp <> nil then begin
		    ExprErrorName(procExpr,errorName,
			'Too many parameters on procedure/function');
		    error := true;
		end;
	    end;
	end;
	if not error then begin
	    retType := procType^.funcType;
	end;
    end;
    CheckFuncProc := not error;
end;

function FuncExpr(en : ExprNode; mode : EvalMode) : TypeNode;
var
    exprType, funcType : TypeNode;
    func : ExprNode;
begin
    if TraceNexpr then begin
	writeln(output,'FuncExpr');
    end;

    funcType := CheckExprFunc(en^.func,EVALPOINT);
    func := en^.func;
    if IsBadExpr(func) then begin
	BadExpr(en);
    end else begin
	if not (en^.func^.kind in [EXPRCONST,EXPRSYM]) then begin
	    { Get procedure address from variable }
	    ValueOrAddr(en^.func,funcType,EVALGET);
	end;
	if not CheckFuncProc(true,en,func,en^.params,exprType) then begin
	    BadExpr(en);
	end else begin
	    en^.exprType := ActualType(exprType);
	end;
    end;
    FuncExpr := en^.exprType;
end;

function SetExpr(en : ExprNode; mode : EvalMode) : TypeNode;
var
    csl : ConstSetList;
    tn : TypeNode;
    sym : Symbol;
begin
    if en^.setTypeName = nil then begin
	tn := bitsetTypeNode;
    end else begin
	sym := QualifiedName(en^.setTypeName);
	tn := nil;
	if sym = nil then begin
	    { do nothing }
	end else if en^.setTypeName^.first <> nil then begin
	    { more qualifiers remain }
	    ExprErrorName(en,sym^.name,'Invalid set type on set expression');
	end else if sym^.kind <> SYMTYPE then begin
	    ExprErrorName(en,sym^.name,'Symbol on set constant not a set type');
	end else if sym^.symType^.kind <> DTSET then begin
	    ExprErrorName(en,sym^.name,'Symbol on set constant not a set type');
	end else begin
	    tn := ActualType(sym^.symType);
	end;
    end;
    if tn = nil then begin
	BadExpr(en);
    end else begin
	csl := ExprSetToConstSet(en^.setValue);
	en^.kind := EXPRCONST;
	en^.exprConst := SetConst(csl,tn);
	en^.constType := tn;
	en^.exprType := tn;
    end;
    SetExpr := tn;
end;

function DoCheckExpr(en : ExprNode; mode : EvalMode) : TypeNode;
var
    tn : TypeNode;
begin
    tn := nil;
    if en = nil then begin
	ExprError(en,'CheckExpr: nil expression?');
    end else if en^.exprType <> nil then begin
	    { already checked }
	tn := en^.exprType;
	ExprError(en,'CheckExpr: already checked?');
    end else begin
	currLine := en^.lineNumber;
	currFile := en^.fileName;
	case en^.kind of
	    EXPRBAD :	ExprError(en,'CheckExpr: found EXPRBAD?');
	    EXPRNAME :	tn := NameExpr(en,mode);
	    EXPRSYM :	tn := SymExpr(en,mode);
	    EXPRVAR :	tn := VarExpr(en,mode);
	    EXPRCONST :	tn := ConstExpr(en,mode);
	    EXPRUNOP :	tn := UnOpExpr(en,mode);
	    EXPRBINOP :	tn := BinOpExpr(en,mode);
	    EXPRSUBSCR :tn := SubscriptExpr(en,mode);
	    EXPRDOT :	tn := DotExpr(en,mode);
	    EXPRDEREF :	tn := DerefExpr(en,mode);
	    EXPRFUNC :	tn := FuncExpr(en,mode);
	    EXPRSET :	tn := SetExpr(en,mode);
	end;
    end;
    DoCheckExpr := tn;
end;

function CheckExpr{(en : ExprNode; mode : EvalMode) : TypeNode};
var
    tn : TypeNode;
begin
    tn := DoCheckExpr(en,mode);
    if IsBadExpr(en) then begin
    end else if not (en^.kind in [EXPRVAL, EXPRVAR, EXPRBINOP, EXPRCHECK,
		EXPRUNOP, EXPRCONST, EXPRFUNC])
    then begin
	ExprError(en,'Expression is not a value or variable');
	BadExpr(en);
    end;
    CheckExpr := tn;
end;

function CheckExprFunc{(en : ExprNode; mode : EvalMode) : TypeNode};
var
    tn : TypeNode;
begin
    tn := DoCheckExpr(en,mode);
    if IsBadExpr(en) then begin
    end else if en^.kind <> EXPRSYM then begin
	{ any normal expression that is a procedure value is OK }
	if tn^.kind <> DTPROC then begin
	    ExprError(en,'Procedure/function name is not a procedure, function, or type');
	    BadExpr(en);
	end;
    end else if en^.exprSym^.kind <> SYMTYPE then begin
	ExprError(en,'Procedure/function name is not a procedure, function, or type');
	BadExpr(en);

    end;
    CheckExprFunc := tn;
end;

function CheckExprType{(en : ExprNode; mode : EvalMode) : TypeNode};
var
    tn : TypeNode;
begin
    tn := DoCheckExpr(en,mode);
    if IsBadExpr(en) then begin
    end else if en^.kind <> EXPRSYM then begin
	ExprError(en,'Expression found where type name expected');
	BadExpr(en);
    end else if en^.exprSym^.kind <> SYMTYPE then begin
	ExprError(en,'Type not found where type name expected');
	BadExpr(en);
    end;
    CheckExprType := tn;
end;

function Eval{(en : ExprNode) : ConstNode};
var
    cn : ConstNode;
begin
    if en = nil then begin
	ExprError(en,'Eval: nil expression?');
    end else if en^.kind in [EXPRUNOP,EXPRBINOP,EXPRCONST] then begin
	case en^.kind of
	    EXPRCONST :	cn := en^.exprConst;
	    EXPRUNOP :	cn := UnOpConst(en^.exprUnOp,Eval(en^.opnd));
	    EXPRBINOP :	cn := BinOpConst(en^.exprBinOp,Eval(en^.opnd1),
						Eval(en^.opnd2),true);
	end;
    end else begin
	ExprError(en,'Invalid constant expression');
	new(cn);
	cn^.kind := DTCARDINAL;
	cn^.cardVal := 1;
    end;
    Eval := cn;
end;

function ExprSetToConstSet{(esl : ExprSetList) : ConstSetList};
var
    esn : ExprSetNode;
    csl : ConstSetList;
    csn : ConstSetNode;
    tn : TypeNode;
begin
    if esl = nil then begin
	csl := nil;
    end else begin
	csl := nil;
	esn := esl^.first;
	while esn <> nil do begin
	    new(csn);
	    tn := CheckExpr(esn^.lower,EVALGET);
	    if IsBadExpr(esn^.lower) then begin
		csn^.lower := CardinalConst(0);
	    end else begin
		csn^.lower := Eval(esn^.lower);
	    end;
	    if esn^.lower = esn^.upper then begin
		csn^.upper := csn^.lower;
	    end else begin
		tn := CheckExpr(esn^.upper,EVALGET);
		if IsBadExpr(esn^.upper) then begin
		    csn^.upper := CardinalConst(0);
		end else begin
		    csn^.upper := Eval(esn^.upper);
		end;
	    end;
	    csl := AddToConstSetList(csl,csn);
	    esn := esn^.next;
	end;
    end;
    ExprSetToConstSet := csl;
end;
