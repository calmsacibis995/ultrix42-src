(*#@(#)strings.mod	4.1	Ultrix	7/17/90 *)
(* $Header: strings.mod,v 1.4 84/05/19 11:40:14 powell Exp $ *)
implementation module strings;

procedure Compare(a, op, b : array of Char) : Boolean;
type
    CharSet = set of Char;
    Comparison = (COMPEQ, COMPNE, COMPGT, COMPLT);
var
    i, length : Cardinal;
    comparison : Comparison;
    equalOK : boolean;
    lasta, lastb : Char;
begin
    (* encode comparison operator *)
    if number(op) >= 1 then
	if op[0] = '=' then
	    comparison := COMPEQ;
	    equalOK := TRUE;
	elsif op[0] = '#' then
	    comparison := COMPNE;
	    equalOK := FALSE;
	elsif op[0] = '<' then
	    if number(op) >= 2 then
		if op[1] = '>' then
		    comparison := COMPNE;
		    equalOK := FALSE;
		else
		    comparison := COMPLT;
		    equalOK := op[1] = '=';
		end;
	    else
		comparison := COMPLT;
		equalOK := FALSE;
	    end;
	elsif op[0] = '>' then
	    comparison := COMPGT;
	    equalOK := (number(op) >= 2) and (op[1] = '=');
	else
	    assert(false,"strings.Compare: bad comparison");
	end;
    else
	assert(false,"strings.Compare: bad comparison");
    end;
    (* figure out min of two lengths *)
    (* set lasta and lastb to be last characters of a and b if same length *)
    (* set to a null for the shorter one and the corresponding character for *)
    (* the longer one if the lengths differ (Note: chars are unsigned) *)
    length := Number(b);
    if length = Number(a) then
	if length = 0 then
	    return equalOK;
	end;
	lasta := a[length-1];
	lastb := b[length-1];
    elsif length < Number(a) then
	lasta := a[length];
	lastb := 0C;
    else
	length := Number(a);
	lasta := 0C;
	lastb := b[length];
    end;

    i := 0;
    (* scan through the strings, comparing for equal *)
    loop
	(* stop if reached the end *)
	if i >= length then
	    exit;
	end;
	(* stop at first non-equal character *)
	if a[i] # b[i] then
	    (* compare these characters instead of planned ones *)
	    lasta := a[i];
	    lastb := b[i];
	    exit;
	end;
	(* stop if both null (above if assured a[i] = b[i]) *)
	if a[i] = 0C then
	    (* ensure later code thinks strings are equal *)
	    lasta := 0C;
	    lastb := 0C;
	    exit;
	end;
	i := i + 1;
    end;
    (* do comparison requested *)
    case comparison of
	COMPEQ :
	    return lasta = lastb;
	|
	COMPNE :
	    return lasta # lastb;
	|
	COMPGT:
	    return (lasta > lastb) or (equalOK and (lasta = lastb));
	|
	COMPLT:
	    return (lasta < lastb) or (equalOK and (lasta = lastb));
	|
    end;
end Compare;

procedure Assign(var toString : array of Char; fromString : array of Char);
var
    i : cardinal;
begin
    i := 0;
    (* loop assigning characters *)
    loop
	(* stop at null or end of fromString *)
	if (i+1 > Number(fromString)) or (fromString[i] = 0C) then
	    exit;
	end;
	toString[i] := fromString[i];
	i := i + 1;
    end;
    (* if there is room in toString, insert null *)
    if i < Number(toString)-1 then
	toString[i] := 0C;
    end;
end Assign;

procedure Append(var toString : array of Char; fromString : array of Char);
var
    fi, ti : cardinal;
begin
    ti := 0;
    fi := 0;
    (* find end of toString *)
    while toString[ti] # 0C do
	ti := ti + 1;
    end;
    loop
	if (fi+1 > Number(fromString)) or (fromString[fi] = 0C) then
	    exit;
	end;
	toString[ti] := fromString[fi];
	fi := fi + 1;
	ti := ti + 1;
    end;
    if ti < Number(toString)-1 then
	toString[ti] := 0C;
    end;
end Append;

end strings.
