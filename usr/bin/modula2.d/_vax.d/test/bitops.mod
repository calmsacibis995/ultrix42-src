(*#@(#)bitops.mod	4.1	Ultrix	7/17/90 *)
module bittest;
from bitoperations import bitand, bitor, bitxor, bitnot,
	bitshiftleft, bitshiftright, bitextract, bitinsert;
from io import writef, output;
const
    C1 = 01234567H;
    C2 = 33333333H;
    CNOT = 0fedcba98H;
    CAND = 01230123H;
    COR = 33337777H;
    CXOR = 32107654H;
    CSHIFTLEFT = 8d159c00H;
    CSHIFTRIGHT = 000048d1H;
    COUNT = 10;
    CEXTRACT = 08d1H;
    CINSERT = 012b3967H;
    FIELD = 0aceH;
    OFFSET = 10;
    SIZE = 12;
var
    i, j, k : cardinal;
begin
    k := bitnot(C1);
    if k <> CNOT then
	writef(output,"bitnot %x %x %x\n",C1,CNOT,k);
    end;

    k := bitand(C1,C2);
    if k <> CAND then
	writef(output,"bitand %x %x %x %x\n",C1,C2,CAND,k);
    end;

    k := bitor(C1,C2);
    if k <> COR then
	writef(output,"bitor %x %x %x %x\n",C1,C2,COR,k);
    end;

    k := bitxor(C1,C2);
    if k <> CXOR then
	writef(output,"bitxor %x %x %x %x\n",C1,C2,CXOR,k);
    end;

    k := bitshiftleft(C1,COUNT);
    if k <> CSHIFTLEFT then
	writef(output,"bitshiftleft %x %x %x %x\n",C1,COUNT,CSHIFTLEFT,k);
    end;
    
    k := bitshiftright(C1,COUNT);
    if k <> CSHIFTRIGHT then
	writef(output,"bitshiftright %x %x %x %x\n",C1,COUNT,CSHIFTRIGHT,k);
    end;
    
    k := bitextract(C1,OFFSET,SIZE);
    if k <> CEXTRACT then
	writef(output,"bitextract %x %x %x %x %x\n",C1,OFFSET,SIZE,CEXTRACT,k);
    end;
    
    k := bitinsert(FIELD,OFFSET,SIZE,C1);
    if k <> CINSERT then
	writef(output,"bitinsert %x %x %x %x %x %x\n",FIELD,OFFSET,SIZE,C1,
	    CINSERT,k);
    end;
    
end bittest.
