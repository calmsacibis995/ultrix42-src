(*#@(#)types.h	4.1	Ultrix	7/17/90 *)
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
 *									    *
 ****************************************************************************)
const
    SHORTSTRINGSIZE = 32;
type
    ShortString = packed array [1..SHORTSTRINGSIZE] of char;

    Buff = packed array [1..BUFFSIZE] of char;
    FileName = packed array[1..100] of char;

    operandstring = packed array [1..operandsize] of char;
    opdsizearray = array[1..maxoperands] of integer;
    opdsizeptr = ^opdsizearray;
    opdtype = array[1.. maxoperands] of operandstring;
    opdptr = ^opdtype;

    pcodetype = (taddress, tboolean, tchar, tinteger, tcardinal, tset, tstring,
			treal, tlongreal, tproc, trecord, tundefined);
    pcodetypeset = set of pcodetype;
    opcodes = (
	opabs, opadd, opand, opbgn, opcap, opcep, opchk, opchr, opcip, opcjp,
	opcom, opcsp, opctr, opcts, opcup, opdec, opdef, opdif, opdiv, opdsp,
	opent, opequ, opexi, opfjp, opflt, opfor, opgeq, opgrt, opinc, opind,
	opinn, opint, opinx, opior, opixa, oplab, oplao, oplca, oplda, opldc,
	opldo, opleq, oples, oploc, oplod, opmax, opmin, opmod, opmov, opmst,
	opmup, opmus, opmvn, opnam, opneg, opneq, opnew, opnot, opodd, opord,
	oppar, opret, opsal, opsav, opsdf, opsex, opsgs, opsin, opsml, opsro,
	opstn, opsto, opstp, opstr, opsub, opsym, opsys, optjp, optrc, optyp,
	opujp, opuni, opuse, opvde, opvin, opxjp, opzer, opcmt,
	opad2, opsb2, opmp2, opdv2, opbit,
	opzzz);

    Level = 0..MAXDISPLEVEL;
    LabelNumber = 10001..100000;
    sizerange = 0..65535;

    opcodestring = packed array [1.. opcodefieldsize] of char;
    opcodepair = record
	    oper :  opcodestring;
	    case boolean of
	    true : (code :  opcodes);
	    false : (intcode : integer);
    end;

    { stuff for delayed jump code }
    jumpcond = (jcnone, jceq, jcne, jcgt, jcle, jclt, jcge,
				jcgtu, jcleu, jcltu, jcgeu);

    { the structure of the opcode hash table }
    HashTable = array [ 0 .. MAXTABLESIZE ] of opcodepair;

const
    COMMNAMESIZE = 256;
type
    CommName = packed array [1..COMMNAMESIZE] of char;
    Comtable = record
      block  :  integer;
      name   :  CommName;
    end;
