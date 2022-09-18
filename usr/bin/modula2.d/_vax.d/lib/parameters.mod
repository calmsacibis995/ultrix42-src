(*#@(#)parameters.mod	4.1	Ultrix	7/17/90 *)
(* $Header: parameters.mod,v 1.5 84/05/19 11:40:05 powell Exp $ *)
IMPLEMENTATION MODULE parameters;
FROM SYSTEM IMPORT MAXINT;
TYPE
    ArgPtr = POINTER @NOCHECK TO ARRAY [0..MAXINT] OF CHAR;
VAR
    (* these variables  are set by runtime initialization *)
    argc : CARDINAL;
    argv, envp : POINTER @NOCHECK TO ARRAY [0..MAXINT] OF ArgPtr;

PROCEDURE GetParameter(num : CARDINAL; VAR value : ARRAY OF CHAR;
	VAR length: INTEGER);
VAR
    i : CARDINAL;
BEGIN
    IF num >= NumParameters THEN
	length := -1;	(* error, no such parameter *)
    ELSE
	i := 0;
	WHILE (i < NUMBER(value)) AND (argv^[num]^[i] # 0C) DO
	    value[i] := argv^[num]^[i];
	    i := i + 1;
	END;
	IF i < NUMBER(value) THEN
	    value[i] := 0C;
	END;
	length := i;
    END;
END GetParameter;

PROCEDURE GetEnvironment(name : ARRAY OF CHAR; VAR value : ARRAY OF CHAR;
	VAR length: INTEGER);
VAR
    v, i, l : CARDINAL;
BEGIN
    v := 0;
    LOOP
	IF envp^[v] = NIL THEN
	    length := -1;
	    RETURN;
	END;
	i := 0;
	WHILE (i < NUMBER(name)) AND (name[i] = envp^[v]^[i]) DO
	    i := i + 1;
	END;
	IF (envp^[v]^[i] = '=') AND
		((i >= NUMBER(name)) OR (name[i] = 0C))
	THEN
	    EXIT;
	END;
	v := v + 1;
    END;
    i := i + 1;
    l := 0;
    WHILE (l < NUMBER(value)) AND (envp^[v]^[i+l] # 0C) DO
	value[l] := envp^[v]^[i+l];
	l := l + 1;
    END;
    IF l < NUMBER(value) THEN
	value[l] := 0C;
    END;
    length := l;
END GetEnvironment;

BEGIN
    NumParameters := argc;
END parameters.
