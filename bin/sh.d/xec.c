#ifndef lint
static char sccsid[] = "@(#)xec.c	4.2 (ULTRIX) 8/13/90";
/* Original ID:  "@(#)xec.c	4.3 8/11/83" */
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

/*
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 * 05	David Lindner Wed Jun  6 13:33:27 EDT 1990
 *	- Added declaration of nsignal.
 *	- Replaced calls to signal with calls to nsignal.
 *	- Added bckg flag to await calls.
 *	- Added rwait flag, to indicate if we are waiting on a read.
 *
 * 04	David Lindner Wed Apr 11 10:31:42 EDT 1990
 *	- Check for return from fork, to fix fork bug.
 *	- Modified comment header.
 *	- Included errno.h
 *
 * 03	David Lindner Wed Jul 19 13:04:26 EDT 1989
 *	- Fixed set -e option.
 *
 * 02	Dave Lindner Fri Jun  9 14:18:47 EDT 1989
 *	- Added check for trap handler
 *
 * 01	19-Nov-85, Greg Tarsa
 *	- Fixed "continue n" code so that it works properly.  Thanks
 *	  to John Dustin and System V code.
 */


#include	"defs.h"
#include	"sym.h"
#include	<errno.h>	/* DJL 04 */ 

LOCAL INT	parent; 
extern void	(*nsignal())();		/* DJL 05 */
int		rwait;			/* DJL 05 */

SYSTAB		commands;



/* ========	command execution	========*/


execute(argt, execflg, pf1, pf2)
	TREPTR		argt;
	INT		*pf1, *pf2;
{
	/* `stakbot' is preserved by this routine */
	REG TREPTR	t;
	STKPTR		sav=savstak();

	sigchk();

	IF (t=argt) ANDF execbrk==0
	THEN	REG INT		treeflgs;
		INT		oldexit, type;
		REG STRING	*com;

		treeflgs = t->tretyp; type = treeflgs&COMMSK;
		oldexit=exitval; exitval=0;

		SWITCH type IN

		case TCOM:
			BEGIN
			STRING		a1;
			INT		argn, internal;
			ARGPTR		schain=gchain;
			IOPTR		io=t->treio;
			gchain=0;
			argn = getarg(t);
			com=scan(argn);
			a1=com[1]; gchain=schain;

			IF argn==0 ORF (internal=syslook(com[0],commands))
			THEN	setlist(t->comset, 0);
			FI

			IF argn ANDF (flags&noexec)==0
			THEN	/* print command if execpr */
				IF flags&execpr
				THEN	argn=0;	prs(execpmsg);
					WHILE com[argn]!=ENDARGS
					DO prs(com[argn++]); blank() OD
					newline();
				FI

				SWITCH internal IN

				case SYSDOT:
					IF a1
					THEN	REG INT		f;
	
						IF (f=pathopen(getpath(a1), a1)) < 0
						THEN failed(a1,notfound);
						ELSE execexp(0,f);
						FI
					FI
					break;
	
				case SYSTIMES:
					{
					L_INT	t[4]; times(t);
					prt(t[2]); blank(); prt(t[3]); newline();
					}
					break;
	
				case SYSEXIT:
					exitsh(a1?stoi(a1):oldexit);
	
				case SYSNULL:
					io=0;
					break;
	
				case SYSCONT:
#ifdef GT01_oldcode
					execbrk = -loopcnt; break; */
#endif oldcode
					IF loopcnt
					THEN
					    execbrk = breakcnt = 1;
					    IF a1
					    THEN breakcnt = stoi(a1);
					    FI

					    IF breakcnt > loopcnt
					    THEN breakcnt = loopcnt;
					    ELSE breakcnt = -breakcnt;
					    FI
					FI
					break;
	
				case SYSBREAK:
#ifdef GT01_oldcode
					IF (execbrk=loopcnt) ANDF a1
					THEN breakcnt=stoi(a1);
					FI
					break;
#endif oldcode
					IF loopcnt
					THEN
					    execbrk = breakcnt = 1;
					    
					    IF a1
					    THEN breakcnt = stoi(a1);
					    FI

					    IF breakcnt > loopcnt
					    THEN breakcnt = loopcnt;
					    FI
					FI
					break;	

				case SYSTRAP:
					IF a1
					THEN	BOOL	clear;
						IF (clear=digit(*a1))==0
						THEN	++com;
						FI
						WHILE *++com
						DO INT	i;
						   IF (i=stoi(*com))>=MAXTRAP ORF i<MINTRAP
						   THEN	failed(*com,badtrap);
						   ELIF clear
						   THEN	clrsig(i);
						   ELSE	replace(&trapcom[i],a1);
							IF *a1
							THEN	getsig(i);
							ELSE	ignsig(i);
							FI
						   FI
						OD
					ELSE	/* print out current traps */
						INT		i;
	
						FOR i=0; i<MAXTRAP; i++
						DO IF trapcom[i]
						   THEN	prn(i); prs(colon); prs(trapcom[i]); newline();
						   FI
						OD
					FI
					break;
	
				case SYSEXEC:
					com++;
					initio(io); ioset=0; io=0;
					IF a1==0 THEN break FI
	
				case SYSLOGIN:
					flags |= forked;
					oldsigs(); execa(com); done();
	
				case SYSCD:
					IF flags&rshflg
					THEN	failed(com[0],restricted);
					ELIF (a1==0 ANDF (a1=homenod.namval)==0) ORF chdir(a1)<0
					THEN	failed(a1,baddir);
					FI
					break;
	
				case SYSSHFT:
					IF dolc<1
					THEN	error(badshift);
					ELSE	dolv++; dolc--;
					FI
					assnum(&dolladr, dolc);
					break;
	
				case SYSWAIT:
					/* DJL 05 */
					await(a1 ? stoi(a1) : -1, 1);
					break;
	
				case SYSREAD:
					/* DJL 05 */
					rwait = 1;
					exitval=readvar(&com[1]);
					rwait = 0;
					break;

/*
				case SYSTST:
					exitval=testcmd(com);
					break;
*/

				case SYSSET:
					IF a1
					THEN	INT	argc;
						argc = options(argn,com);
						IF argc>1
						THEN	setargs(com+argn-argc);
						FI
					ELIF t->comset==0
					THEN	/*scan name chain and print*/
						namscan(printnam);
					FI
					break;
	
				case SYSRDONLY:
					exitval=N_RDONLY;
				case SYSXPORT:
					IF exitval==0 THEN exitval=N_EXPORT; FI
	
					IF a1
					THEN	WHILE *++com
						DO attrib(lookup(*com), exitval) OD
					ELSE	namscan(printflg);
					FI
					exitval=0;
					break;
	
				case SYSEVAL:
					IF a1
					THEN	execexp(a1,&com[2]);
					FI
					break;

                                case SYSUMASK:
                                        if (a1) {
                                                int c, i;
                                                i = 0;
                                                while ((c = *a1++) >= '0' &&
                                                        c <= '7')
                                                        i = (i << 3) + c - '0';
                                                umask(i);
                                        } else {
                                                int i, j;
                                                umask(i = umask(0));
                                                prc('0');
                                                for (j = 6; j >= 0; j -= 3)
                                                        prc(((i>>j)&07) + '0');
                                                newline();
                                        }
                                        break;
	
				default:
					internal=builtin(argn,com);
	
				ENDSW

				IF internal
				THEN	IF io THEN error(illegal) FI
					chktrap();
					break;
				FI
			ELIF t->treio==0
			THEN	break;
			FI
			END
	
		case TFORK:
			IF execflg ANDF (treeflgs&(FAMP|FPOU))==0
			THEN	parent=0;
			ELSE	
				/*
				 * DJL 04
				 * check to see if fork fails
				 */
				int forkcnt = 1;

				while ((parent = fork()) == -1 )
				{
					if ((forkcnt = (forkcnt * 2)) > FORKLIM)
					{
						switch (errno)
						{
						case ENOMEM:
							error(noswap);
							break;
						default:
						case EAGAIN:
							error(nofork);
							break;
						}
					}
					sigchk();
					alarm(forkcnt);
					pause();
				}
			FI

			IF parent
			THEN	/* This is the parent branch of fork;    */
				/* it may or may not wait for the child. */
				IF treeflgs&FPRS ANDF flags&ttyflg
				THEN	prn(parent); newline();
				FI
				IF treeflgs&FPCL THEN closepipe(pf1) FI
				IF (treeflgs&(FAMP|FPOU))==0
				THEN	await(parent, 0);	/* DJL 05 */
				ELIF (treeflgs&FAMP)==0
				THEN	post(parent);
				ELSE	assnum(&pcsadr, parent);
				FI

				chktrap();
				break;


			ELSE	/* this is the forked branch (child) of execute */
				flags |= forked; iotemp=0;
				postclr();
				settmp();

				/* Turn off INTR and QUIT if `FINT'  */
				/* Reset ramaining signals to parent */
				/* except for those `lost' by trap   */
				oldsigs();
				IF treeflgs&FINT
				/* DJL 05 */
				THEN	nsignal(INTR,1); nsignal(QUIT,1);
				FI

				/* pipe in or out */
				IF treeflgs&FPIN
				THEN	rename(pf1[INPIPE],0);
					close(pf1[OTPIPE]);
				FI
				IF treeflgs&FPOU
				THEN	rename(pf2[OTPIPE],1);
					close(pf2[INPIPE]);
				FI

				/* default std input for & */
				IF treeflgs&FINT ANDF ioset==0
				THEN	rename(chkopen(devnull),0);
				FI

				/* io redirection */
				initio(t->treio);
				IF type!=TCOM
				THEN	execute(t->forktre,1);
				ELIF com[0]!=ENDARGS
				THEN	setlist(t->comset,N_EXPORT);
					execa(com);
				FI
				done();
			FI

		case TPAR:
			rename(dup(2),output);
			execute(t->partre,execflg);
			done();

		case TFIL:
			BEGIN
			   INT pv[2]; chkpipe(pv);
			   IF execute(t->lstlef, 0, pf1, pv)==0
			   THEN	execute(t->lstrit, execflg, pv, pf2);
			   ELSE	closepipe(pv);
			   FI
			END
			break;

		case TLST:
			execute(t->lstlef,0);
			execute(t->lstrit,execflg);
			break;

		case TAND:
			IF execute(t->lstlef,0)==0
			THEN	execute(t->lstrit,execflg);
			FI
			break;

		case TORF:
			IF execute(t->lstlef,0)!=0
			THEN	execute(t->lstrit,execflg);
			FI
			break;

		case TFOR:
			BEGIN
			   NAMPTR	n = lookup(t->fornam);
			   STRING	*args;
			   DOLPTR	argsav=0;

			   IF t->forlst==0
			   THEN    args=dolv+1;
				   argsav=useargs();
			   ELSE	   ARGPTR	schain=gchain;
				   gchain=0;
				   trim((args=scan(getarg(t->forlst)))[0]);
				   gchain=schain;
			   FI
			   loopcnt++;
			   WHILE *args!=ENDARGS ANDF execbrk==0
			   DO	assign(n,*args++);
				execute(t->fortre,0);
#ifdef GT01_oldcode
				IF execbrk<0 THEN execbrk=0 FI
#endif oldcode
				IF breakcnt < 0
				THEN execbrk = (++breakcnt != 0);
				FI
			   OD
#ifdef GT01_oldcode
			   IF breakcnt THEN breakcnt-- FI
			   execbrk=breakcnt;
#endif
			    IF breakcnt > 0
			    THEN execbrk = (--breakcnt != 0);
			    FI

			   loopcnt--;
			   argfor=freeargs(argsav);
			END
			break;

		case TWH:
		case TUN:
			BEGIN
			   INT		i=0;

			   loopcnt++;
			   flags |= boolflg;	/* DJL 03 */
			   WHILE execbrk==0 ANDF (execute(t->whtre,0)==0)==(type==TWH)
			   DO flags &= ~boolflg;
			      i=execute(t->dotre,0);
			      flags |= boolflg;
#ifdef GT01_oldcode
			      IF execbrk<0 THEN execbrk=0 FI
#endif oldcode
			      IF breakcnt < 0
			      THEN execbrk = (++breakcnt != 0);
			      FI
			   OD

			   flags &= ~boolflg;
#ifdef GT01_oldcode
			   IF breakcnt THEN breakcnt-- FI
			   execbrk=breakcnt;
#endif oldcode
			   IF breakcnt > 0
			   THEN execbrk = (--breakcnt != 0);
			   FI

			   loopcnt--;
			   exitval=i;
			END
			break;

		case TIF:
			flags |= boolflg;	/* DJL 03 */
			IF execute(t->iftre,0) == 0
			THEN	flags &= ~boolflg;
				execute(t->thtre,execflg);
			ELSE 	flags &= ~boolflg;
				execute(t->eltre,execflg);
			FI
			break;

		case TSW:
			BEGIN
			   REG STRING	r = mactrim(t->swarg);
			   t=t->swlst;
			   WHILE t
			   DO	ARGPTR		rex=t->regptr;
				WHILE rex
				DO	REG STRING	s;
					IF gmatch(r,s=macro(rex->argval)) ORF (trim(s), eq(r,s))
					THEN	execute(t->regcom,0);
						t=0; break;
					ELSE	rex=rex->argnxt;
					FI
				OD
				IF t THEN t=t->regnxt FI
			   OD
			END
			break;
		ENDSW
		exitset();
	FI
	else chktrap();		/* DJL 02 */

	sigchk();
	tdystak(sav);
	return(exitval);
}


execexp(s,f)
	STRING		s;
	UFD		f;
{
	FILEBLK		fb;
	push(&fb);
	IF s
	THEN	estabf(s); fb.feval=f;
	ELIF f>=0
	THEN	initf(f);
	FI
	execute(cmd(NL, NLFLG|MTFLG),0);
	pop();
}
