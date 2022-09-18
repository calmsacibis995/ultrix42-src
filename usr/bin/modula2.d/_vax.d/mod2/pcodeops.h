(*#@(#)pcodeops.h	4.1	Ultrix	7/17/90 *)
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
$Header: pcodeops.h,v 1.5 84/05/19 11:44:22 powell Exp $
 ****************************************************************************)
type
    PcodeInst = (PCABS, PCADD, PCAND, PCBGN, PCCAP, PCCEP, PCCHK, PCCHR, PCCIP,
		PCCJP, PCCOM, PCCSP, PCCTS, PCCUP, PCDEC, PCDEF, PCDIF, PCDIV,
		PCDSP, PCENT, PCEQU, PCFJP, PCFLO, PCFLT, PCGEQ, PCGRT, PCINC,
		PCIND, PCINN, PCINT, PCIOR, PCIXA, PCLAB, PCLAO, PCLCA, PCLDA,
		PCLDC, PCLDO, PCLEQ, PCLES, PCLOC, PCLOD, PCMOD, PCMOV, PCMST,
		PCMUP, PCMUS, PCMVN, PCNAM, PCNEG, PCNEQ, PCNEW, PCNOT, PCODD,
		PCORD, PCPAR, PCRET, PCSAL, PCSAV, PCSDF, PCSGS, PCSML, PCSRO,
		PCSTN, PCSTO, PCSTP, PCSTR, PCSUB, PCSYM, PCSYS, PCTJP, PCTRC,
		PCTYP, PCUJP, PCUNI, PCUSE, PCXJP, PCZER, PCSIN, PCSEX, PCVIN,
		PCVDE, PCFOR, PCEXI, PCMAX, PCMIN, PCAD2, PCSB2, PCMP2, PCDV2,
		PCBIT,
		PCZZZ) ;
var
    operPcode : array [Token] of PcodeInst;
