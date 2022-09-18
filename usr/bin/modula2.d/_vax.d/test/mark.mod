(*#@(#)mark.mod	4.1	Ultrix	7/17/90 *)
module test;
type
    nodeptr = pointer to node;
    node = record
	left,right: nodeptr;
	val:integer;
    end;
procedure Checktree(p: nodeptr): boolean;
var result: boolean;
begin
    result := true;
    WITH p^ do 
	if left<>nil then 
	    if left^.val <= val then result :=false;
	    else result := Checktree(left) and result; end; end;
	if right<>nil then
	    if right^.val >= val then result := false;
	    else result := Checktree(right) and result; end; end;
     end;
     return (result);
end Checktree;  (* checktree *)

end test.
