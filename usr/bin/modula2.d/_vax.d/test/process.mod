(*#@(#)process.mod	4.1	Ultrix	7/17/90 *)
module testprocess;
from system import NewProcess, Process, Transfer, Word, TSize;
from io import writef, output;
const
    STACKSIZE = 1000;
    NUMPROCESSES = 3;
    NUMTIMES = 3;
type
    StackRec = array [1..STACKSIZE] of Word;
    Stack = pointer to StackRec;
    Name = array [1..10] of char;
    ProcessId = [0..NUMPROCESSES-1];
var
    p : array ProcessId of Process;
    s : array ProcessId of Stack;
    parent : Process;
    paramname : Name;
    paramid : ProcessId;
procedure P();
var
    myname : Name;
    myid : ProcessId;
    count : integer;
begin
    myname := paramname;
    myid := paramid;
    Transfer(p[myid],parent);
    for count := 1 to NUMTIMES do
	writef(output,"%s %d %d\n",myname,myid,count);
	Transfer(p[myid],p[(myid+1) mod NUMPROCESSES]);
    end;
	writef(output,"%s %d done\n",myname,myid);
    Transfer(p[myid],parent);
end P;
var
    i : ProcessId;
begin
    for i := first(ProcessId) to last(ProcessId) do
	New(s[i]);
	NewProcess(P,s[i],TSize(StackRec),p[i]);
	case i of
	| 0 : paramname := "Larry";
	| 1 : paramname := "Curly";
	| 2 : paramname := "Moe";
	else
	    paramname := "?";
	end;
	paramid := i;
	Transfer(parent,p[i]);
    end;
    Transfer(parent,p[0]);
    writef(output,"That's all, folks!\n");
end testprocess.
