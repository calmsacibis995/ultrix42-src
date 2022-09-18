(*#@(#)forloop.mod	4.1	Ultrix	7/17/90 *)
module forloop;
from io import writef, output;
procedure P();
var
    i : cardinal;
    c : char;
begin
    i := 0;
    for c := chr(0) to chr(255) do
	if i <> ord(c) then
	    writef(output,"char loop not equal %d %d\n",i,ord(c));
	end;
	inc(i);
    end;
    if i <> 256 then
	writef(output,"char loop failed %d\n",i);
    end;
end P;

begin
    P();
end forloop.
