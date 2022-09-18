(*#@(#)machine.p	4.1	Ultrix	7/17/90 *)
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
$Header: machine.p,v 1.4 84/05/19 11:41:33 powell Exp $
 ****************************************************************************)
#include "globals.h"

{ CardDiv:  simulate div with real arithmetic }
{  Trick: only do divide if b >= 2 so trunc is guaranteed to work }
function CardDiv{(a,b : cardinal) : cardinal};
var
    q : cardinal;
begin
    if b = 0 then begin
	Error('Cardinal division by 0');
	q := 1;
    end else if a > b * MAXINT then begin
	Error('Cardinal division overflow');
	q := 1;
    end else begin
	q := trunc(a / b);
    end;
    CardDiv := q;
end;
function CardMod{(a,b : cardinal) : cardinal};
var
    q, r : cardinal;
begin
    q := CardDiv(a,b);
    r := a - (b*q);
    CardMod := r;
end;

