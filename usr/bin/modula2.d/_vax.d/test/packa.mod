(*#@(#)packa.mod	4.1	Ultrix	7/17/90 *)
module packtest;
type
    UShort = @size 16 [0..65535];
    Short = @size 16 [-32768..32767];
    UByte = @size 8 [0..255];
    Byte = @size 8 [-128..127];
    RecSmall = record
	a : Byte;
	b : Short;
    end;
    RecLarge = record
	a : UShort;
	b : RecSmall;
	c : Byte;
    end;
	
var
    shortArray : array [1..10] of Short;
    largeArray : array [1..10] of RecLarge;
    i : integer;
begin
    i := shortArray[1];
    shortArray[i] := i;
    i := largeArray[i].a;
    i := largeArray[i].b.a;
    i := largeArray[i].b.b;
    i := largeArray[i].c;
    largeArray[i].a := i;
    largeArray[i].b.a := i;
    largeArray[i].b.b := i;
    largeArray[i].c := i;
end packtest.
