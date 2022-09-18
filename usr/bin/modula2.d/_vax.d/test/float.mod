(*#@(#)float.mod	4.1	Ultrix	7/17/90 *)
module testfloat;
from io import writef, output;
var i : integer; c : cardinal; r : real; l : longreal;
begin
    i := -12345678;
    c := 12345678;
    r := float(i);
    if r <> -12345678.0 then
	writef(output,"i->r\n");
    end;
    r := float(c);
    if r <> 12345678.0 then
	writef(output,"c->r\n");
    end;
    l := longfloat(i);
    if l <> -12345678.0 then
	writef(output,"i->l\n");
    end;
    l := longfloat(c);
    if l <> 12345678.0 then
	writef(output,"c->l\n");
    end;
    r := 12345678.0;
    l := 12345678.9;
    i := trunc(r);
    if i <> 12345678 then
	writef(output,"r->i\n");
    end;
    c := trunc(r);
    if c <> 12345678 then
	writef(output,"r->c\n");
    end;
    i := trunc(l);
    if i <> 12345678 then
	writef(output,"l->i\n");
    end;
    c := trunc(l);
    if c <> 12345678 then
	writef(output,"l->c\n");
    end;
end testfloat.
