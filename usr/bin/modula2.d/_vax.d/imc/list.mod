(*#@(#)list.mod	4.1	Ultrix	7/17/90 *)
(****************************************************************************
 *									    *
 *  Copyright (c) 1984 by						    *
 *  DIGITAL EQUIPMENT CORPORATION, Maynard, Massachusetts.		    *
 *  All rights reserved.						    *
 * 									    *
 *  This software is furnished under a license and may be used and copied   *
 *  only in  accordance with  the  terms  of  such  license  and with the   *
 *  inclusion of the above copyright notice. This software or  any  other   *
 *  copies thereof may not be provided or otherwise made available to any   *
 *  other person.  No title to and ownership of  the  software is  hereby   *
 *  transferred.							    *
 * 									    *
 *  The information in this software is  subject to change without notice   *
 *  and  should  not  be  construed as  a commitment by DIGITAL EQUIPMENT   *
 *  CORPORATION.							    *
 * 									    *
 *  DIGITAL assumes no responsibility for the use  or  reliability of its   *
 *  software on equipment which is not supplied by DIGITAL.		    *
 * 									    *
$Header: list.mod,v 1.3 84/05/19 11:34:09 powell Exp $
 ****************************************************************************)
implementation module list;
type
    Element = pointer to ElementRec;
    ElementRec = record
	next : Element;
	(* other stuff *)
    end;

procedure AddToList(list : List; newOne : Address) : List;
var
    element, lastElement : Element;
begin
    element := newOne;
    if list = nil then
	new(list);
	list^.first := nil;
	list^.last := nil;
    end;
    if element = nil then
	(* no element, do nothing *)
    elsif list^.first = nil then
	element^.next := nil;
	list^.first := element;
	list^.last := element;
    else
	element^.next := nil;
	lastElement := list^.last;
	lastElement^.next := element;
	list^.last:= element;
    end;
    return list;
end AddToList;

procedure Next(element : Address) : Address;
var
    telement : Element;
begin
    telement := element;
    return telement^.next;
end Next;

end list.
