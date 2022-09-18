(*#@(#)lib.mod	4.1	Ultrix	7/17/90 *)
MoDuLe testlib;
ImPoRt sYsTeM, bItOpErAtIoNs, Io, MeMoRy, Storage, unix, parameters, strings, math;
VaR
    f : iO.fIlE;
    i : InTeGeR;
    a : SyStEm.AdDrEsS;
BeGiN
    i := unix.open("test",0);
    f := Io.oPeN("test","r");
    MeMoRy.AlLoCaTe(a,100);
    Storage.ALLOCATE(a,100);
EnD testlib.
