(*#@(#)tokens.h	4.1	Ultrix	7/17/90 *)
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
$Header: tokens.h,v 1.3 84/05/19 11:43:32 powell Exp $
 ****************************************************************************)
type
    Token = (TKENDOFFILE, TKPLUS, TKMINUS, TKASTERISK, TKSLASH, TKASSIGN,
	    TKAMPERSAND, TKDOT, TKCOMMA, TKSEMICOLON, TKLPAREN, TKLBRACKET,
	    TKLBRACE, TKUPARROW,
	    TKEQUALS, TKSHARP, TKLESS, TKGREATER, TKNOTEQUAL, TKLSEQUAL,
	    TKGREQUAL, TKDOTDOT, TKCOLON, TKRPAREN, TKRBRACKET, TKRBRACE, TKBAR,
	    TKIDENT, TKNUMBER, TKUNUSED, TKCHARCONST, TKSTRCONST, TKBOOLCONST,
	    TKAND, TKARRAY, TKBEGIN, TKBY, TKCASE, TKCONST, TKLABEL, TKDIV,
	    TKDO, TKELSE, TKGOTO, TKEND, TKPACKED, TKFORWARD, TKFOR, TKFROM,
	    TKIF, TKFUNCTION , TKEXTERNAL, TKIN, TKDOWNTO, TKMOD, TKPROGRAM,
	    TKNOT, TKOF, TKOR, TKPOINTER, TKPROCEDURE, TKFILE, TKRECORD,
	    TKREPEAT, TKRETURN, TKSET, TKTHEN, TKTO, TKTYPE, TKUNTIL, TKVAR,
	    TKWHILE, TKWITH, TKINCLUDE, TKBAD, TKHEX);
