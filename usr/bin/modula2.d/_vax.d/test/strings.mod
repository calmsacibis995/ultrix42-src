(*#@(#)strings.mod	4.1	Ultrix	7/17/90 *)
module teststrings;
from io import writef, output;
from strings import Compare, Assign, Append;

const
    SHORTSTRINGSIZE = 3;
    LONGSTRINGSIZE = 100;
type
    ShortString = array [1..SHORTSTRINGSIZE] of char;
    LongString = array [0..LONGSTRINGSIZE-1] of char;
var
    ss1, ss2, ss3, ss4 : ShortString;
    ls1, ls2, ls3, ls4 : LongString;
begin
    (* < *)
    Assign(ss1,'a');
    Assign(ss2,'b');
    assert(Compare(ss1,'#',ss2),'ss1 # ss2');
    assert(Compare(ss1,'<>',ss2),'ss1 <> ss2');
    assert(Compare(ss1,'<',ss2),'ss1 < ss2');
    assert(Compare(ss1,'<=',ss2),'ss1 <= ss2');
    assert(not Compare(ss1,'=',ss2),'not ss1 = ss2');
    assert(not Compare(ss1,'>',ss2),'not ss1 > ss2');
    assert(not Compare(ss1,'>=',ss2),'not ss1 >= ss2');
    Assign(ss1,'a');
    Assign(ss2,'ab');
    assert(Compare(ss1,'#',ss2),'ss1 # ss2');
    assert(Compare(ss1,'<>',ss2),'ss1 <> ss2');
    assert(Compare(ss1,'<',ss2),'ss1 < ss2');
    assert(Compare(ss1,'<=',ss2),'ss1 <= ss2');
    assert(not Compare(ss1,'=',ss2),'not ss1 = ss2');
    assert(not Compare(ss1,'>',ss2),'not ss1 > ss2');
    assert(not Compare(ss1,'>=',ss2),'not ss1 >= ss2');
    Assign(ss1,'');
    Assign(ss2,' ');
    assert(Compare(ss1,'#',ss2),'ss1 # ss2');
    assert(Compare(ss1,'<>',ss2),'ss1 <> ss2');
    assert(Compare(ss1,'<',ss2),'ss1 < ss2');
    assert(Compare(ss1,'<=',ss2),'ss1 <= ss2');
    assert(not Compare(ss1,'=',ss2),'not ss1 = ss2');
    assert(not Compare(ss1,'>',ss2),'not ss1 > ss2');
    assert(not Compare(ss1,'>=',ss2),'not ss1 >= ss2');
    Assign(ss1,'123');
    Assign(ls2,'12345');
    assert(Compare(ss1,'#',ls2),'ss1 # ls2');
    assert(Compare(ss1,'<>',ls2),'ss1 <> ls2');
    assert(Compare(ss1,'<',ls2),'ss1 < ls2');
    assert(Compare(ss1,'<=',ls2),'ss1 <= ls2');
    assert(not Compare(ss1,'=',ls2),'not ss1 = ls2');
    assert(not Compare(ss1,'>',ls2),'not ss1 > ls2');
    assert(not Compare(ss1,'>=',ls2),'not ss1 >= ls2');

    (* = *)
    ss1 := 'abc';
    Assign(ss2,'abc');
    assert(not Compare(ss1,'#',ss2),'not ss1 # ss2');
    assert(not Compare(ss1,'<>',ss2),'not ss1 <> ss2');
    assert(not Compare(ss1,'<',ss2),'not ss1 < ss2');
    assert(Compare(ss1,'<=',ss2),'ss1 <= ss2');
    assert(Compare(ss1,'=',ss2),'ss1 = ss2');
    assert(not Compare(ss1,'>',ss2),'not ss1 > ss2');
    assert(Compare(ss1,'>=',ss2),'ss1 >= ss2');
    ss1 := 'abc';
    ss3 := 'a';
    ss4 := 'bc';
    Assign(ss2,ss3);
    Append(ss2,ss4);
    assert(not Compare(ss1,'#',ss2),'not ss1 # ss2');
    assert(not Compare(ss1,'<>',ss2),'not ss1 <> ss2');
    assert(not Compare(ss1,'<',ss2),'not ss1 < ss2');
    assert(Compare(ss1,'<=',ss2),'ss1 <= ss2');
    assert(Compare(ss1,'=',ss2),'ss1 = ss2');
    assert(not Compare(ss1,'>',ss2),'not ss1 > ss2');
    assert(Compare(ss1,'>=',ss2),'ss1 >= ss2');
    ls2 := 'abc';
    assert(not Compare(ss1,'#',ls2),'not ss1 # ls2');
    assert(not Compare(ss1,'<>',ls2),'not ss1 <> ls2');
    assert(not Compare(ss1,'<',ls2),'not ss1 < ls2');
    assert(Compare(ss1,'<=',ls2),'ss1 <= ls2');
    assert(Compare(ss1,'=',ls2),'ss1 = ls2');
    assert(not Compare(ss1,'>',ls2),'not ss1 > ls2');
    assert(Compare(ss1,'>=',ls2),'ss1 >= ls2');
end teststrings.
