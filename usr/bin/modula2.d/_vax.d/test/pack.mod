(*#@(#)pack.mod	4.1	Ultrix	7/17/90 *)
module packtest;
type
    UShort = @align 16 @size 16 [0..65535];
    Short = @align 16 @size 16 [-32768..32767];
    UByte = @align 8 @size 8 [0..255];
    Byte = @align 8 @size 8 [-128..127];
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
    sb : Byte;
    ub : UByte;
    i : integer;
begin
    i := ub;
    i := sb;
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
