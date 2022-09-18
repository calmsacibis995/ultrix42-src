(*#@(#)nocount.mod	4.1	Ultrix	7/17/90 *)
implementation module nocount;

type
    CString = array @nocount of char;

procedure X(s : UnixString);
begin
    s[0] := 'a';
    s[i] := c;
end X;

procedure Y(var s : UnixString);
begin
    s[0] := 'a';
    s[i] := c;
end Y;

var
    str : array [1..100] of char;
    i : integer;
    c : char;
begin
    P("xxx");
    P('a');
    P("");
    P(str);
    Q(str);
    R("xxx");
    R('a');
    R("");
    R(str);
end nocount.
