(*#@(#)const.p	4.1	Ultrix	7/17/90 *)
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
$Header: const.p,v 1.5 84/05/19 11:37:35 powell Exp $
 ****************************************************************************)
#include "globals.h"
#include "scanner.h"
#include "const.h"
#include "decls.h"


function OrdOf{(cn : ConstNode) : cardinal};
var
    value : cardinal;
begin
    if cn = nil then begin
	value := 1;
    end else begin
	case cn^.kind of
	    DTINTEGER,
	    DTCARDINAL: value := cn^.cardVal;
	    DTCHAR: value := cn^.charVal;
	    DTBOOLEAN: value := ord(cn^.boolVal);
	    DTSTRING, DTSET, DTREAL, DTLONGREAL : begin
		ErrorName(stringDataType[cn^.kind],
		    'Constant type not indexable');
		value := 1;
	    end;
	    DTENUMERATION : begin
		value := cn^.enumVal^.enumOrd;
	    end;
	end;
    end;
    OrdOf := value;
end;

function CardinalConst{(value : cardinal) : ConstNode};
var
    cn : ConstNode;
begin
    if TraceCexpr then begin
	writeln(output,'CardinalConst(',value:1:0,')');
    end;
    new(cn);
    cn^.kind := DTCARDINAL;
    cn^.cardVal := value;
    if (value > MAXCARD) or (value < -MAXINT-1) then begin
	Error('Cardinal constant exceeds implementation limits');
    end;
    CardinalConst := cn;
end;


function SymConst{(names : IdentList) : ConstNode};
var
    value : ConstNode;
    sym : Symbol;
begin
    if TraceCexpr then begin
	writeln(output,'SymConst');
    end;
    sym := QualifiedName(names);
    if sym = nil then begin
	value := nil;
    end else if names^.first <> nil then begin
	ErrorName(sym^.name,'Qualification error on constant');	
	value := nil;
    end else if sym^.kind = SYMENUM then begin
	new(value);
	value^.kind := DTENUMERATION;
	value^.enumVal := sym^.symEnum;
    end else if sym^.kind <> SYMCONST then begin
	ErrorName(sym^.name,'Symbol not a constant');
	value := nil;
    end else begin
	dispose(names);
	new(value);
	value^ := sym^.symConst^;
    end;
    SymConst := value;
end;

function SetConst{(elementList : ConstSetList; setType : TypeNode) : ConstNode};
var
    cn : ConstNode;
    sv : SetValue;
    element, nextElement : ConstSetNode;
    elementType : TypeNode;
    i : integer;
    lowerBound, upperBound, low, upp : cardinal;
    ok : boolean;
begin
    new(cn);
    cn^.kind := DTSET;
    new(sv);
    if setType <> nil then begin
	if setType^.kind <> DTSET then begin
	    Error('Identifier preceding set constant must be set type name');
	    setType := nil;
	end;
    end;
    if setType = nil then begin
	setType := bitsetTypeNode;
    end;
    lowerBound := LowerBoundOf(setType^.setRange);
    upperBound := UpperBoundOf(setType^.setRange);
    sv^.setType := setType;
    elementType := BaseType(setType^.setRange);
    sv^.value := [];
    if elementList = nil then begin
	{ do nothing }
    end else begin
	element := elementList^.first;
	while element <> nil do begin
	    nextElement := element^.next;
	    ok := false;
	    if elementType^.kind in [DTINTEGER,DTCARDINAL] then begin
		if not(element^.lower^.kind in [DTINTEGER,DTCARDINAL])
		then begin
		    ok := false;
		end else if element^.upper = nil then begin
		    ok := true;
		end else if not(element^.lower^.kind in [DTINTEGER,DTCARDINAL])
		then begin
		    ok := false;
		end else begin
		    ok := true;
		end;
	    end else if elementType^.kind = DTENUMERATION then begin
		if element^.lower^.kind = DTENUMERATION then begin
		    ok := element^.lower^.enumVal^.enumType = elementType;
		end;
		if ok and (element^.upper <> nil) then begin
		    ok := element^.upper^.enumVal^.enumType = elementType;
		end;
	    end else if elementType^.kind in [DTBOOLEAN,DTCHAR] then begin
		ok := element^.lower^.kind = elementType^.kind;
		if ok and (element^.upper <> nil) then begin
		    ok := element^.upper^.kind = elementType^.kind;
		end;
	    end;
	    if not ok then begin
		Error('Constant types improper for set expression');
	    end else begin
		low := OrdOf(element^.lower);
		if element^.upper = nil then begin
		    upp := low;
		end else begin
		    upp := OrdOf(element^.upper);
		end;
		if low > upp then begin
		    Error('Set element range first greater than last');
		end else if (low < lowerBound) or (upp > upperBound) then begin
		    Error('Set elements not within set bounds');
		end else begin
		    for i := trunc(low-lowerBound) to trunc(upp-lowerBound)
		    do begin
			 sv^.value := sv^.value + [i];   
		    end;
		end;
	    end;
	    dispose(element);
	    element := nextElement;
	end;
    end;
    cn^.setVal := sv;
    SetConst := cn;
end;

function UnOpConst{(oper : Token; opnd : ConstNode) : ConstNode};
var
    value : ConstNode;
begin
    if TraceCexpr then begin
	writeln(output,'UnOpConst(',oper:0,')');
    end;
    if opnd = nil then begin
	value := nil;
    end else begin
	new(value);
	value^ := opnd^;
	if (opnd^.kind = DTBOOLEAN) and (oper = TKNOT) then begin
	    value^.boolVal := not opnd^.boolVal;
	end else if (opnd^.kind in [DTREAL,DTLONGREAL,DTINTEGER,DTCARDINAL]) and
		(oper in [TKPLUS,TKMINUS])
	then begin
	    if oper = TKMINUS then begin
		case opnd^.kind of
		    DTCARDINAL : begin
			value^.kind := DTINTEGER;
			value^.cardVal := -opnd^.cardVal;
		    end;
		    DTINTEGER : value^.cardVal := -opnd^.cardVal;
		    DTREAL, DTLONGREAL : value^.realVal := -opnd^.realVal;
		end;
	    end;
	end else begin
	    ErrorName(stringToken[oper],'Invalid constant unary expression');
	end;
    end;
    UnOpConst := value;
end;

function BinOpConst{(oper : Token; opnd1, opnd2 : ConstNode; eval : boolean) : ConstNode};
var
	value, nopnd : ConstNode;
	cardVal : real;
	error : boolean;
begin
    if TraceCexpr then begin
	writeln(output,'BinOpConst(',oper:0,')');
    end;
    error := false;
    if (opnd1 = nil) or (opnd2 = nil) then begin
	value := nil;
    end else begin
	if (opnd1^.kind <> opnd2^.kind) and not
		((opnd1^.kind in [DTINTEGER,DTCARDINAL])
		and (opnd2^.kind in [DTINTEGER,DTCARDINAL]))
	then begin
	    if eval then begin
		new(nopnd);
		nopnd^.kind := DTINTEGER;
		nopnd^.cardVal := OrdOf(opnd1);
		opnd1 := nopnd;
		new(nopnd);
		nopnd^.kind := DTINTEGER;
		nopnd^.cardVal := OrdOf(opnd2);
		opnd2 := nopnd;
	    end else begin
		ErrorName(stringToken[oper],
		    'Mixed mode operation in constant expression');
		error := true;
	    end;
	end;
	new(value);
	value^ := opnd1^;
	if error then begin
	    { do nothing }
	end else if (opnd1^.kind = DTBOOLEAN) and (oper in [TKAND,TKAMPERSAND,
		TKOR,TKEQUALS,TKSHARP,TKNOTEQUAL,TKLESS,TKLSEQUAL,TKGREATER,
		TKGREQUAL])
	then begin
	    case oper of
		TKAMPERSAND,
		TKAND : value^.boolVal := opnd1^.boolVal and opnd2^.boolVal;
		TKOR : value^.boolVal := opnd1^.boolVal or opnd2^.boolVal;
		TKEQUALS : value^.boolVal := opnd1^.boolVal = opnd2^.boolVal;
		TKSHARP,
		TKNOTEQUAL: value^.boolVal := opnd1^.boolVal <> opnd2^.boolVal;
		TKLESS : value^.boolVal := opnd1^.boolVal < opnd2^.boolVal;
		TKLSEQUAL : value^.boolVal := opnd1^.boolVal <= opnd2^.boolVal;
		TKGREATER : value^.boolVal := opnd1^.boolVal > opnd2^.boolVal;
		TKGREQUAL : value^.boolVal := opnd1^.boolVal >= opnd2^.boolVal;
	    end;
	end else if (opnd1^.kind in [DTINTEGER, DTCARDINAL]) and
		(oper in [TKPLUS, TKMINUS, TKASTERISK, TKDIV, TKMOD])
	then begin
	    case oper of
		TKPLUS : value^.cardVal := opnd1^.cardVal + opnd2^.cardVal;
		TKMINUS : value^.cardVal := opnd1^.cardVal - opnd2^.cardVal;
		TKASTERISK : value^.cardVal := opnd1^.cardVal * opnd2^.cardVal;
		TKDIV : value^.cardVal := CardDiv(opnd1^.cardVal,
							opnd2^.cardVal);
		TKMOD : value^.cardVal := CardMod(opnd1^.cardVal,
							opnd2^.cardVal);
	    end;
	    if (opnd1^.kind = DTINTEGER) or (opnd2^.kind = DTINTEGER) or
		    (value^.cardVal < 0)
	    then begin
		value^.kind := DTINTEGER;
	    end else begin
		value^.kind := DTCARDINAL;
	    end;
	end else if (opnd1^.kind in [DTINTEGER, DTCARDINAL]) and (oper in
		[TKEQUALS,TKSHARP,TKNOTEQUAL,TKLESS,TKLSEQUAL,TKGREATER,
		TKGREQUAL])
	then begin
	    value^.kind := DTBOOLEAN;
	    case oper of
		TKEQUALS : value^.boolVal := opnd1^.cardVal = opnd2^.cardVal;
		TKSHARP,
		TKNOTEQUAL: value^.boolVal := opnd1^.cardVal <> opnd2^.cardVal;
		TKLESS : value^.boolVal := opnd1^.cardVal < opnd2^.cardVal;
		TKLSEQUAL : value^.boolVal := opnd1^.cardVal <= opnd2^.cardVal;
		TKGREATER : value^.boolVal := opnd1^.cardVal > opnd2^.cardVal;
		TKGREQUAL : value^.boolVal := opnd1^.cardVal >= opnd2^.cardVal;
	    end;
	end else if (opnd1^.kind in [DTREAL,DTLONGREAL]) and
		(oper in [TKPLUS, TKMINUS, TKASTERISK, TKSLASH])
	then begin
	    case oper of
		TKPLUS : value^.realVal := opnd1^.realVal + opnd2^.realVal;
		TKMINUS : value^.realVal := opnd1^.realVal - opnd2^.realVal;
		TKASTERISK : value^.realVal := opnd1^.realVal * opnd2^.realVal;
		TKSLASH : value^.realVal := opnd1^.realVal / opnd2^.realVal;
	    end;
	end else if (opnd1^.kind = DTENUMERATION) and (oper in [TKEQUALS,
		TKSHARP,TKNOTEQUAL,TKLESS,TKLSEQUAL,TKGREATER,TKGREQUAL])
	then begin
	    value^.kind := DTBOOLEAN;
	    if opnd1^.enumVal^.enumType <> opnd2^.enumVal^.enumType then begin
		ErrorName(opnd1^.enumVal^.name,
			'Cannot compare different enumerations');
	    end else begin
		case oper of
		    TKEQUALS : value^.boolVal :=
			    opnd1^.enumVal^.enumOrd = opnd2^.enumVal^.enumOrd;
		    TKSHARP,
		    TKNOTEQUAL: value^.boolVal :=
			    opnd1^.enumVal^.enumOrd <> opnd2^.enumVal^.enumOrd;
		    TKLESS : value^.boolVal :=
		    	    opnd1^.enumVal^.enumOrd < opnd2^.enumVal^.enumOrd;
		    TKLSEQUAL : value^.boolVal :=
			    opnd1^.enumVal^.enumOrd <= opnd2^.enumVal^.enumOrd;
		    TKGREATER : value^.boolVal :=
			    opnd1^.enumVal^.enumOrd > opnd2^.enumVal^.enumOrd;
		    TKGREQUAL : value^.boolVal :=
			    opnd1^.enumVal^.enumOrd >= opnd2^.enumVal^.enumOrd;
		end;
	    end;
	end else if (opnd1^.kind = DTSET) and (oper in [TKEQUALS,TKSHARP,
		TKNOTEQUAL,TKLSEQUAL,TKGREQUAL])
	then begin
	    if opnd1^.setVal^.setType <> opnd2^.setVal^.setType then begin
		ErrorName(stringToken[oper],
		    'Set operand types do not match in constant expression');
	    end else begin
		value^.kind := DTBOOLEAN;
		case oper of
		    TKEQUALS : value^.boolVal := opnd1^.setVal^.value
						    = opnd2^.setVal^.value;
		    TKSHARP,
		    TKNOTEQUAL: value^.boolVal := opnd1^.setVal^.value
						    <> opnd2^.setVal^.value;
		    TKLSEQUAL : value^.boolVal := opnd1^.setVal^.value
						    <= opnd2^.setVal^.value;
		    TKGREQUAL : value^.boolVal := opnd1^.setVal^.value
						    >= opnd2^.setVal^.value;
		end;
	    end;
	end else if (opnd1^.kind = DTSET) and (oper in [TKPLUS,TKMINUS,
		TKASTERISK,TKSLASH])
	then begin
	    if opnd1^.setVal^.setType <> opnd2^.setVal^.setType then begin
		ErrorName(stringToken[oper],
		    'Set operand types do not match in constant expression');
	    end else begin
		new(value^.setVal);
		value^.setVal^.setType := opnd1^.setVal^.setType;
		case oper of
		    TKPLUS : value^.setVal^.value := opnd1^.setVal^.value
						    + opnd2^.setVal^.value;
		    TKMINUS: value^.setVal^.value := opnd1^.setVal^.value
						    - opnd2^.setVal^.value;
		    TKASTERISK : value^.setVal^.value := opnd1^.setVal^.value
						    * opnd2^.setVal^.value;
		    TKSLASH : value^.setVal^.value :=
			(opnd1^.setVal^.value + opnd2^.setVal^.value) -
			(opnd1^.setVal^.value * opnd2^.setVal^.value);
		end;
	    end;
	end else if (opnd1^.kind in [DTREAL,DTLONGREAL]) and (oper in [TKEQUALS,
		TKSHARP,TKNOTEQUAL,TKLESS,TKLSEQUAL,TKGREATER,TKGREQUAL])
	then begin
	    value^.kind := DTBOOLEAN;
	    case oper of
		TKEQUALS : value^.boolVal := opnd1^.realVal = opnd2^.realVal;
		TKSHARP,
		TKNOTEQUAL: value^.boolVal := opnd1^.realVal <> opnd2^.realVal;
		TKLESS : value^.boolVal := opnd1^.realVal < opnd2^.realVal;
		TKLSEQUAL : value^.boolVal := opnd1^.realVal <= opnd2^.realVal;
		TKGREATER : value^.boolVal := opnd1^.realVal > opnd2^.realVal;
		TKGREQUAL : value^.boolVal := opnd1^.realVal >= opnd2^.realVal;
	    end;
	end else begin
	    ErrorName(stringToken[oper],'Invalid binary constant expression');
	end;
	{ disposes can be done if we are still parsing }
	{ dispose(opnd1); }
	{ dispose(opnd2); }
    end;
    BinOpConst := value;
end;

procedure CheckEqualConst{(sym : Symbol; con : ConstNode)};
var
    error : boolean;
begin
    error := true;
    if sym^.kind = SYMCONST then begin
	if sym^.symConst^.kind <> con^.kind then begin
	    case con^.kind of
		DTINTEGER,
		DTCARDINAL : error := sym^.symConst^.cardVal <> con^.cardVal;
		DTBOOLEAN : error := sym^.symConst^.boolVal <> con^.boolVal;
		DTCHAR : error := sym^.symConst^.charVal <> con^.charVal;
		DTREAL,
		DTLONGREAL : error := sym^.symConst^.realVal <> con^.realVal;
		DTENUMERATION : error := (sym^.symConst^.enumVal^.enumType <>
					    con^.enumVal^.enumType)
					or (sym^.symConst^.enumVal^.enumOrd <>
					    con^.enumVal^.enumOrd);
		DTSTRING,
		DTPROC : error := true;
	    end;
	end;
    end;
    if error then begin
	ErrorName(sym^.name,'Constant redefined');
    end;
end;

procedure WriteConstant{(var f:text; con : ConstNode)};
begin
    if con = nil then begin
	    write(f,'NIL CONSTANT');
    end else begin
	case con^.kind of
	    DTCARDINAL,
	    DTINTEGER: write(f,con^.cardVal:1:0);
	    DTREAL,
	    DTLONGREAL: write(f,con^.realVal);
	    DTBOOLEAN: write(f,con^.boolVal);
	    DTCHAR: begin
		if con^.charVal in [ord(' ')..ord('~')] then begin
		    write(f,chr(con^.charVal));
		end else begin
		    write(f,'chr(',con^.charVal:1,')');
		end;
	    end;
	    DTSTRING: WriteString(f,con^.strVal);
	    DTENUMERATION: WriteString(f,con^.enumVal^.name);
	    DTPROC: WriteString(f,con^.procVal^.name);
	    DTPOINTER: write(f,'nil');
	    DTSET: write(f,'set constant');
	end;
    end;
end;
