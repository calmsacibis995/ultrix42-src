


#ifndef lint
static	char	*sccsid = "@(#)get.c	4.2	(ULTRIX)	12/20/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1988 by			*
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
 *
 *   Modification history:
 *
 *	7  Dec 84 -- depp
 *	   Fix UID problem so that get forks a child to manipulate
 *	   files in user directory
 *	14 Dec 84 -- depp
 *	   Fixed the return from wait(), after fork/exec to manipulate g-file,
 *	   to handle the failure to manipulate the g-file.
 *
 *	07 Mar 88 -- map
 *	   Add -u option which sets the time of the g-file to the time of
 *	   the s-file.  This results in a g-file with a time equal to the
 *	   last delta.  This is useful for build scripts which extract
 *	   all files from the SCCS database and then do a make.
 *	28 Jun 88 -- D. Long
 *	   Use atomic rename operation in xremove instead of link and unlink.
 *	   Also don't use return value from fputs, Ultrix is not SVID
 *	   compliant here and returns undefined.  Use ferror instead.
 *	   Also added error checking on output.
 */

# include 	<sys/wait.h>
# include	<time.h>
# include	"../hdr/defines.h"
# include	"../hdr/had.h"


struct stat Statbuf;
struct timeval tv[2];
char Null[1];
char Error[128];

int	Debug = 0;
int	had_pfile;
struct packet gpkt;
struct sid sid;
unsigned	Ser;
int	num_files;
char	had[26];
char	Pfilename[FILESIZE];
char	*ilist, *elist, *lfile;
char	*sid_ab(), *auxf(), *logname();
char	*sid_ba(), *date_ba();
long	cutoff = 0X7FFFFFFFL;	/* max positive long */
int verbosity;
char	Gfile[FILESIZE];
char	*Type;
int	Did_id;
FILE *fdfopen();

/* Beginning of modifications made for CMF system. */
#define CMRLIMIT 128
char	cmr[CMRLIMIT];
int		cmri = 0;
/* End of insertion */

main(argc,argv)
int argc;
register char *argv[];
{
	register int i;
	register char *p;
	char c;
	int testmore;
	extern int Fcnt;
	extern get();

	Fflags = FTLEXIT | FTLMSG | FTLCLN;
	for(i=1; i<argc; i++)
		if(argv[i][0] == '-' && (c=argv[i][1])) {
			p = &argv[i][2];
			testmore = 0;
			switch (c) {

    			case CMFFLAG:
				/* Concatenate the rest of this argument with the
				 * existing CMR list. */
				while (*p) {
					if (cmri == CMRLIMIT)
						fatal ("CMR list is too long.");
					cmr[cmri++] = *p++;
					}
				cmr[cmri] = NULL;
				break;

			case 'a':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				Ser = patoi(p);
				break;
			case 'r':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				chksid(sid_ab(p,&sid),&sid);
				if ((sid.s_rel < MINR) ||
					(sid.s_rel > MAXR))
					fatal("r out of range (ge22)");
				break;
			case 'c':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				if (date_ab(p,&cutoff))
					fatal("bad date/time (cm5)");
				break;
			case 'l':
				lfile = p;
				break;
			case 'i':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				ilist = p;
				break;
			case 'x':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				elist = p;
				break;
			case 'b':
			case 'g':
			case 'e':
			case 'p':
			case 'k':
			case 'm':
			case 'n':
			case 's':
			case 't':
			case 'u':
				testmore++;
				break;
			default:
				fatal("unknown key letter (cm1)");
			}

			if (testmore) {
				testmore = 0;
				if (*p) {
					sprintf(Error,
						"value after %c arg (cm8)",c);
					fatal(Error);
				}
			}
			if (had[c - 'a']++)
				fatal("key letter twice (cm2)");
			argv[i] = 0;
		}
		else num_files++;

	if(num_files == 0)
		fatal("missing file arg (cm3)");
	if (HADE && HADM)
		fatal("e not allowed with m (ge3)");
	if (HADE)
		HADK = 1;
	if (!HADS)
		verbosity = -1;
	if (HADE && ! logname())
		fatal("User ID not in password file (cm9)");
	setsig();
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if (p=argv[i])
			do_file(p,get);
	exit(Fcnt ? 1 : 0);
}


extern char *Sflags[];

get(file)
char *file;
{
	register char *p;
	register unsigned ser;
	extern char had_dir, had_standinp;
	struct stats stats;
	struct idel *dodelt();
	char	str[32];
	char *idsubst(), *auxf();
	int i,status,holduid,holdgid;

	int retval;

	Fflags |= FTLMSG;
	if (setjmp(Fjmp))
		return;
	if (HADE) {
		had_pfile = 1;
		/*
		call `sinit' to check if file is an SCCS file
		but don't open the SCCS file.
		If it is, then create lock file.
		*/
		sinit(&gpkt,file,0);
		if (lockit(auxf(file,'z'),10,getpid()))
			fatal("cannot create lock file (cm4)");
	}
	/*
	Open SCCS file and initialize packet
	*/
	sinit(&gpkt,file,1);
	gpkt.p_ixuser = (HADI | HADX);
	gpkt.p_reqsid.s_rel = sid.s_rel;
	gpkt.p_reqsid.s_lev = sid.s_lev;
	gpkt.p_reqsid.s_br = sid.s_br;
	gpkt.p_reqsid.s_seq = sid.s_seq;
	gpkt.p_verbose = verbosity;
	gpkt.p_stdout = (HADP ? stderr : stdout);
	gpkt.p_cutoff = cutoff;
	gpkt.p_lfile = lfile;
	copy(auxf(gpkt.p_file,'g'),Gfile);

	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
	if (dodelt(&gpkt,&stats,(struct sid *) 0,0) == 0)
		fmterr(&gpkt);
	finduser(&gpkt);
	doflags(&gpkt);
	if (!HADA)
		ser = getser(&gpkt);
	else {
		if ((ser = Ser) > maxser(&gpkt))
			fatal("serial number too large (ge19)");
		gpkt.p_gotsid = gpkt.p_idel[ser].i_sid;
		if (HADR && sid.s_rel != gpkt.p_gotsid.s_rel) {
			zero(&gpkt.p_reqsid, sizeof(gpkt.p_reqsid));
			gpkt.p_reqsid.s_rel = sid.s_rel;
		}
		else
			gpkt.p_reqsid = gpkt.p_gotsid;
	}
	doie(&gpkt,ilist,elist,(char *) 0);
	setup(&gpkt,(int) ser);


	if((p = Sflags[LOCKGET - 'a'])) {
		retval = ck_lockget(p,&gpkt);
		if(retval)
			return(0);
	}


	if (!(Type = Sflags[TYPEFLAG - 'a']))
		Type = Null;
	if (!(HADP || HADG))
		if (exists(Gfile) && (S_IWRITE & Statbuf.st_mode)) {
			sprintf(Error,"writable `%s' exists (ge4)",Gfile);
			fatal(Error);
		}
	if (gpkt.p_verbose) {
		sid_ba(&gpkt.p_gotsid,str);
		fprintf(gpkt.p_stdout,"%s\n",str);
	}
	if (HADE) {
		if (HADC || gpkt.p_reqsid.s_rel == 0)
			gpkt.p_reqsid = gpkt.p_gotsid;
		newsid(&gpkt,Sflags[BRCHFLAG - 'a'] && HADB);
		permiss(&gpkt);
		if (exists(auxf(gpkt.p_file,'p')))
			mk_qfile(&gpkt);
		else had_pfile = 0;
		wrtpfile(&gpkt,ilist,elist);
	}
	if (!HADG || HADL) {
		if (gpkt.p_stdout)
			fflush(gpkt.p_stdout);
		if (HADG)
			exit(0);

		/* fork a child to manipulate the g-file */
		if ((i = fork()) < 0)
			fatal("Can not fork; try again");
		if (i == 0) {
			setuid(getuid());
			setgid(getgid());
			if (HADL)
				gen_lfile(&gpkt);
			flushto(&gpkt,EUSERTXT,1);
			idsetup(&gpkt);
			gpkt.p_chkeof = 1;
			Did_id = 0;
			/*
			call `xcreate' which deletes the old `g-file' and
			creates a new one except if the `p' keyletter is set in
			which case any old `g-file' is not disturbed.
			The mod of the file depends on whether or not the `k'
			keyletter has been set.
			*/

			if (gpkt.p_gout == 0) {
				if (HADP)
					gpkt.p_gout = stdout;
				else
					gpkt.p_gout = xfcreat(Gfile,HADK ? 0644 : 0444);
			}
			while(readmod(&gpkt)) {
				prfx(&gpkt);
				p = idsubst(&gpkt,gpkt.p_line);
				fputs(p,gpkt.p_gout);
				if(ferror(gpkt.p_gout))
					FAILPUT;
			}
			if (gpkt.p_gout)
				if(fflush(gpkt.p_gout)==EOF)
					FAILPUT;
			if (gpkt.p_gout && gpkt.p_gout != stdout)
				fclose(gpkt.p_gout);
			if (gpkt.p_verbose)
				fprintf(gpkt.p_stdout,"%u lines\n",gpkt.p_glnno);
			if (!Did_id && !HADK)
				if (Sflags[IDFLAG - 'a'])
					fatal("no id keywords (cm6)");
				else if (gpkt.p_verbose)
					fprintf(stderr,"No id keywords (cm7)\n");
			if (HADU) {
				exists(gpkt.p_file); 
				tv[0].tv_sec = tv[1].tv_sec = Statbuf.st_mtime;
				tv[0].tv_usec = tv[1].tv_usec = 0;
				utimes(Gfile, tv);
			}
			exit(0);
		}
		else {
			wait(&status);
			if (status) {
				Fflags &= ~FTLMSG;
				fatal();
			}
		}
	}
	if (gpkt.p_iop)
		fclose(gpkt.p_iop);

	/*
	remove 'q'-file because it is no longer needed
	*/
	unlink(auxf(gpkt.p_file,'q'));
	ffreeall();
	unlockit(auxf(gpkt.p_file,'z'),getpid());
}


newsid(pkt,branch)
register struct packet *pkt;
int branch;
{
	int chkbr;

	chkbr = 0;
	/* if branch value is 0 set newsid level to 1 */
	if (pkt->p_reqsid.s_br == 0) {
		pkt->p_reqsid.s_lev += 1;
		/*
		if the sid requested has been deltaed or the branch
		flag was set or the requested SID exists in the p-file
		then create a branch delta off of the gotten SID
		*/
		if (sidtoser(&pkt->p_reqsid,pkt) ||
			pkt->p_maxr > pkt->p_reqsid.s_rel || branch ||
			in_pfile(&pkt->p_reqsid,pkt)) {
				pkt->p_reqsid.s_rel = pkt->p_gotsid.s_rel;
				pkt->p_reqsid.s_lev = pkt->p_gotsid.s_lev;
				pkt->p_reqsid.s_br = pkt->p_gotsid.s_br + 1;
				pkt->p_reqsid.s_seq = 1;
				chkbr++;
		}
	}
	/*
	if a three component SID was given as the -r argument value
	and the b flag is not set then up the gotten SID sequence
	number by 1
	*/
	else if (pkt->p_reqsid.s_seq == 0 && !branch)
		pkt->p_reqsid.s_seq = pkt->p_gotsid.s_seq + 1;
	else {
		/*
		if sequence number is non-zero then increment the
		requested SID sequence number by 1
		*/
		pkt->p_reqsid.s_seq += 1;
		if (branch || sidtoser(&pkt->p_reqsid,pkt) ||
			in_pfile(&pkt->p_reqsid,pkt)) {
			pkt->p_reqsid.s_br += 1;
			pkt->p_reqsid.s_seq = 1;
			chkbr++;
		}
	}
	/*
	keep checking the requested SID until a good SID to be
	made is calculated or all possibilities have been tried
	*/
	while (chkbr) {
		--chkbr;
		while (in_pfile(&pkt->p_reqsid,pkt)) {
			pkt->p_reqsid.s_br += 1;
			++chkbr;
		}
		while (sidtoser(&pkt->p_reqsid,pkt)) {
			pkt->p_reqsid.s_br += 1;
			++chkbr;
		}
	}
	if (sidtoser(&pkt->p_reqsid,pkt) || in_pfile(&pkt->p_reqsid,pkt))
		fatal("bad SID calculated in newsid()");
}


enter(pkt,ch,n,sidp)
struct packet *pkt;
char ch;
int n;
struct sid *sidp;
{
	char str[32];
	register struct apply *ap;

	sid_ba(sidp,str);
	if (pkt->p_verbose)
		fprintf(pkt->p_stdout,"%s\n",str);
	ap = &pkt->p_apply[n];
	switch(ap->a_code) {

	case SX_EMPTY:
		if (ch == INCLUDE)
			condset(ap,APPLY,INCLUSER);
		else
			condset(ap,NOAPPLY,EXCLUSER);
		break;
	case APPLY:
		sid_ba(sidp,str);
		sprintf(Error,"%s already included (ge9)",str);
		fatal(Error);
		break;
	case NOAPPLY:
		sid_ba(sidp,str);
		sprintf(Error,"%s already excluded (ge10)",str);
		fatal(Error);
		break;
	default:
		fatal("internal error in get/enter() (ge11)");
		break;
	}
}


gen_lfile(pkt)
register struct packet *pkt;
{
	char *n;
	int reason;
	char str[32];
	char line[BUFSIZ];
	struct deltab dt;
	FILE *in;
	FILE *out;

	in = xfopen(pkt->p_file,0);
	if (*pkt->p_lfile)
		out = stdout;
	else
		out = xfcreat(auxf(pkt->p_file,'l'),0444);
	fgets(line,sizeof(line),in);
	while (fgets(line,sizeof(line),in) != NULL && line[0] == CTLCHAR && line[1] == STATS) {
		fgets(line,sizeof(line),in);
		del_ab(line,&dt,pkt);
		if (dt.d_type == 'D') {
			reason = pkt->p_apply[dt.d_serial].a_reason;
			if (pkt->p_apply[dt.d_serial].a_code == APPLY) {
				putc(' ',out);
				putc(' ',out);
			}
			else {
				putc('*',out);
				if (reason & IGNR)
					putc(' ',out);
				else
					putc('*',out);
			}
			switch (reason & (INCL | EXCL | CUTOFF)) {
	
			case INCL:
				putc('I',out);
				break;
			case EXCL:
				putc('X',out);
				break;
			case CUTOFF:
				putc('C',out);
				break;
			default:
				putc(' ',out);
				break;
			}
			putc(' ',out);
			sid_ba(&dt.d_sid,str);
			fprintf(out,"%s\t",str);
			date_ba(&dt.d_datetime,str);
			fprintf(out,"%s %s\n",str,dt.d_pgmr);
		}
		while ((n = fgets(line,sizeof(line),in)) != NULL)
			if (line[0] != CTLCHAR)
				break;
			else {
				switch (line[1]) {

				case EDELTAB:
					break;
				default:
					continue;
				case MRNUM:
				case COMMENTS:
					if (dt.d_type == 'D')
						fprintf(out,"\t%s",&line[3]);
					continue;
				}
				break;
			}
		if (n == NULL || line[0] != CTLCHAR)
			break;
		putc('\n',out);
	}
	fclose(in);
	if (out != stdout) {
		if(ferror(out))
			fatal("Unable to write lfile\n");
		fclose(out);
	}
}


char	Curdate[18];
char	*Curtime;
char	Gdate[9];
char	Chgdate[18];
char	*Chgtime;
char	Gchgdate[9];
char	Sid[32];
char	Mod[16];
char	Olddir[BUFSIZ];
char	Pname[BUFSIZ];
char	Dir[BUFSIZ];
char	*Qsect;

idsetup(pkt)
register struct packet *pkt;
{
	extern long Timenow;
	register int n;
	register char *p;

	date_ba(&Timenow,Curdate);
	Curtime = &Curdate[9];
	Curdate[8] = 0;
	makgdate(Curdate,Gdate);
	for (n = maxser(pkt); n; n--)
		if (pkt->p_apply[n].a_code == APPLY)
			break;
	if (n)
		date_ba(&pkt->p_idel[n].i_datetime,Chgdate);
	Chgtime = &Chgdate[9];
	Chgdate[8] = 0;
	makgdate(Chgdate,Gchgdate);
	sid_ba(&pkt->p_gotsid,Sid);
	if (p = Sflags[MODFLAG - 'a'])
		copy(p,Mod);
	else
		copy(Gfile,Mod);
	if (!(Qsect = Sflags[QSECTFLAG - 'a']))
		Qsect = Null;
}


makgdate(old,new)
register char *old, *new;
{
	if ((*new = old[3]) != '0')
		new++;
	*new++ = old[4];
	*new++ = '/';
	if ((*new = old[6]) != '0')
		new++;
	*new++ = old[7];
	*new++ = '/';
	*new++ = old[0];
	*new++ = old[1];
	*new = 0;
}


static char Zkeywd[5] = "@(#)";

char *
idsubst(pkt,line)
register struct packet *pkt;
char line[];
{
	static char tline[BUFSIZ];
	static char str[32];
	register char *lp, *tp;
	char *trans();
	extern char *Type;
	extern char *Sflags[];

	if (HADK || !any('%',line))
		return(line);

	tp = tline;
	for(lp=line; *lp != 0; lp++) {
		if(lp[0] == '%' && lp[1] != 0 && lp[2] == '%') {
			switch(*++lp) {

			case 'M':
				tp = trans(tp,Mod);
				break;
			case 'Q':
				tp = trans(tp,Qsect);
				break;
			case 'R':
				sprintf(str,"%u",pkt->p_gotsid.s_rel);
				tp = trans(tp,str);
				break;
			case 'L':
				sprintf(str,"%u",pkt->p_gotsid.s_lev);
				tp = trans(tp,str);
				break;
			case 'B':
				sprintf(str,"%u",pkt->p_gotsid.s_br);
				tp = trans(tp,str);
				break;
			case 'S':
				sprintf(str,"%u",pkt->p_gotsid.s_seq);
				tp = trans(tp,str);
				break;
			case 'D':
				tp = trans(tp,Curdate);
				break;
			case 'H':
				tp = trans(tp,Gdate);
				break;
			case 'T':
				tp = trans(tp,Curtime);
				break;
			case 'E':
				tp = trans(tp,Chgdate);
				break;
			case 'G':
				tp = trans(tp,Gchgdate);
				break;
			case 'U':
				tp = trans(tp,Chgtime);
				break;
			case 'Z':
				tp = trans(tp,Zkeywd);
				break;
			case 'Y':
				tp = trans(tp,Type);
				break;
			case 'W':
				tp = trans(tp,Zkeywd);
				tp = trans(tp,Mod);
				*tp++ = '\t';
			case 'I':
				tp = trans(tp,Sid);
				break;
			case 'P':
				copy(pkt->p_file,Dir);
				dname(Dir);
				if(curdir(Olddir) != 0)
					fatal("curdir failed (ge20)");
				if(chdir(Dir) != 0)
					fatal("cannot change directory (ge21)");
				if(curdir(Pname) != 0)
					fatal("curdir failed (ge20)");
				if(chdir(Olddir) != 0)
					fatal("cannot change directory (ge21)");
				tp = trans(tp,Pname);
				*tp++ = '/';
				tp = trans(tp,(sname(pkt->p_file)));
				break;
			case 'F':
				tp = trans(tp,pkt->p_file);
				break;
			case 'C':
				sprintf(str,"%u",pkt->p_glnno);
				tp = trans(tp,str);
				break;
			case 'A':
				tp = trans(tp,Zkeywd);
				tp = trans(tp,Type);
				*tp++ = ' ';
				tp = trans(tp,Mod);
				*tp++ = ' ';
				tp = trans(tp,Sid);
				tp = trans(tp,Zkeywd);
				break;
			default:
				*tp++ = '%';
				*tp++ = *lp;
				continue;
			}
			lp++;
		}
		else
			*tp++ = *lp;
	}

	*tp = 0;
	return(tline);
}


char *
trans(tp,str)
register char *tp, *str;
{
	Did_id = 1;
	while(*tp++ = *str++)
		;
	return(tp-1);
}


prfx(pkt)
register struct packet *pkt;
{
	char str[32];

	if (HADN)
		fprintf(pkt->p_gout,"%s\t",Mod);
	if (HADM) {
		sid_ba(&pkt->p_inssid,str);
		fprintf(pkt->p_gout,"%s\t",str);
	}
	if(ferror(pkt->p_gout))
		FAILPUT;
}


clean_up(n)
{
	/*
	clean_up is only called from fatal() upon bad termination.
	*/
	if (gpkt.p_iop)
		fclose(gpkt.p_iop);
	if (gpkt.p_gout)
		fflush(gpkt.p_gout);
	if (gpkt.p_gout && gpkt.p_gout != stdout) {
		fclose(gpkt.p_gout);
		unlink(Gfile);
	}
	if (HADE) {
		if (! had_pfile) {
			unlink(auxf(gpkt.p_file,'p'));
		}
		else if (exists(auxf(gpkt.p_file,'q'))) {
			copy(auxf(gpkt.p_file,'p'),Pfilename);
			xrename(auxf(gpkt.p_file,'q'),Pfilename);
		     }
	}
	ffreeall();
	unlockit(auxf(gpkt.p_file,'z'),getpid());
}


static	char	warn[] = "WARNING: being edited: `%s' (ge18)\n";

wrtpfile(pkt,inc,exc)
register struct packet *pkt;
char *inc, *exc;
{
	char line[64], str1[32], str2[32];
	char *user;
	FILE *in, *out;
	struct pfile pf;
	register char *p;
	int fd;
	int orig_umask;
	extern long Timenow;

	user = logname();
	if (exists(p = auxf(pkt->p_file,'p'))) {
		fd = xopen(p,2);
		in = fdfopen(fd,0);
		while (fgets(line,sizeof(line),in) != NULL) {
			p = line;
			p[length(p) - 1] = 0;
			pf_ab(p,&pf,0);
			if (!(Sflags[JOINTFLAG - 'a'])) {
				if ((pf.pf_gsid.s_rel == pkt->p_gotsid.s_rel &&
     				   pf.pf_gsid.s_lev == pkt->p_gotsid.s_lev &&
				   pf.pf_gsid.s_br == pkt->p_gotsid.s_br &&
				   pf.pf_gsid.s_seq == pkt->p_gotsid.s_seq) ||
				   (pf.pf_nsid.s_rel == pkt->p_reqsid.s_rel &&
				   pf.pf_nsid.s_lev == pkt->p_reqsid.s_lev &&
				   pf.pf_nsid.s_br == pkt->p_reqsid.s_br &&
				   pf.pf_nsid.s_seq == pkt->p_reqsid.s_seq)) {
					fclose(in);
					sprintf(Error,
					     "being edited: `%s' (ge17)",line);
					fatal(Error);
				}
				if (!equal(pf.pf_user,user))
					fprintf(stderr,warn,line);
			}
			else fprintf(stderr,warn,line);
		}
		out = fdfopen(dup(fd),1);
		fclose(in);
	}
	else
	{
		orig_umask = umask();
		umask(0000);
		out = xfcreat(p,0644);
		umask(orig_umask);
	}
	fseek(out,0L,2);
	sid_ba(&pkt->p_gotsid,str1);
	sid_ba(&pkt->p_reqsid,str2);
	date_ba(&Timenow,line);
	fprintf(out,"%s %s %s %s",str1,str2,user,line);
	if (inc)
		fprintf(out," -i%s",inc);
	if (exc)
		fprintf(out," -x%s",exc);
	if (cmrinsert () > 0)	/* if there are CMRS and they are okay */
		fprintf (out, " -z%s", cmr);
	fprintf(out,"\n");
	if(ferror(out) || fclose(out)==EOF)
		xmsg(auxf(pkt->p_file, 'p'),  "wrtpfile");
	if (pkt->p_verbose)
		fprintf(pkt->p_stdout,"new delta %s\n",str2);
}


getser(pkt)
register struct packet *pkt;
{
	register struct idel *rdp;
	int n, ser, def;
	char *p;
	extern char *Sflags[];

	def = 0;
	if (pkt->p_reqsid.s_rel == 0) {
		if (p = Sflags[DEFTFLAG - 'a'])
			chksid(sid_ab(p, &pkt->p_reqsid), &pkt->p_reqsid);
		else {
			pkt->p_reqsid.s_rel = MAX;
			def = 1;
		}
	}
	ser = 0;
	if (pkt->p_reqsid.s_lev == 0) {
		for (n = maxser(pkt); n; n--) {
			rdp = &pkt->p_idel[n];
			if ((rdp->i_sid.s_br == 0 || HADT) &&
				pkt->p_reqsid.s_rel >= rdp->i_sid.s_rel &&
				rdp->i_sid.s_rel > pkt->p_gotsid.s_rel) {
					ser = n;
					pkt->p_gotsid.s_rel = rdp->i_sid.s_rel;
			}
		}
	}
	/*
	 * If had '-t' keyletter and R.L SID type, find
	 * the youngest SID
	*/
	else if ((pkt->p_reqsid.s_br == 0) && HADT) {
		for (n = maxser(pkt); n; n--) {
			rdp = &pkt->p_idel[n];
			if (rdp->i_sid.s_rel == pkt->p_reqsid.s_rel &&
			    rdp->i_sid.s_lev == pkt->p_reqsid.s_lev )
				break;
		}
		ser = n;
	}
	else if (pkt->p_reqsid.s_br && pkt->p_reqsid.s_seq == 0) {
		for (n = maxser(pkt); n; n--) {
			rdp = &pkt->p_idel[n];
			if (rdp->i_sid.s_rel == pkt->p_reqsid.s_rel &&
				rdp->i_sid.s_lev == pkt->p_reqsid.s_lev &&
				rdp->i_sid.s_br == pkt->p_reqsid.s_br)
					break;
		}
		ser = n;
	}
	else {
		ser = sidtoser(&pkt->p_reqsid,pkt);
	}
	if (ser == 0)
		fatal("nonexistent sid (ge5)");
	rdp = &pkt->p_idel[ser];
	pkt->p_gotsid = rdp->i_sid;
	if (def || (pkt->p_reqsid.s_lev == 0 && pkt->p_reqsid.s_rel == pkt->p_gotsid.s_rel))
		pkt->p_reqsid = pkt->p_gotsid;
	return(ser);
}


/* Null routine to satisfy external reference from dodelt() */

escdodelt()
{
}
/* NULL routine to satisfy external reference from dodelt() */
fredck()
{
}

in_pfile(sp,pkt)
struct	sid	*sp;
struct	packet	*pkt;
{
	struct	pfile	pf;
	char	line[BUFSIZ];
	char	*p;
	FILE	*in;

	if (Sflags[JOINTFLAG - 'a']) {
		if (exists(auxf(pkt->p_file,'p'))) {
			in = xfopen(auxf(pkt->p_file,'p'),0);
			while ((p = fgets(line,sizeof(line),in)) != NULL) {
				p[length(p) - 1] = 0;
				pf_ab(p,&pf,0);
				if (pf.pf_nsid.s_rel == sp->s_rel &&
					pf.pf_nsid.s_lev == sp->s_lev &&
					pf.pf_nsid.s_br == sp->s_br &&
					pf.pf_nsid.s_seq == sp->s_seq) {
						fclose(in);
						return(1);
				}
			}
			fclose(in);
		}
		else return(0);
	}
	else return(0);
}


mk_qfile(pkt)
register struct	packet *pkt;
{
	FILE	*in, *qout;
	char	line[BUFSIZ];

	in = xfopen(auxf(pkt->p_file,'p'),0);
	qout = xfcreat(auxf(pkt->p_file,'q'),0644);

	while ((fgets(line,sizeof(line),in) != NULL)) {
		fputs(line,qout);
		if(ferror(qout))
			FAILPUT;
	}
	fclose(in);
	if(fclose(qout)==EOF)
		FAILPUT;
}

/* cmrinsert -- insert CMR numbers in the p.file. */

cmrinsert ()
{
	extern char *strrchr (), *Sflags[];
	extern int	cmrcheck ();

	if (Sflags[CMFFLAG - 'a'] == 0)		/* CMFFLAG was not set. */
		return (0);

	if ( HADP && ( ! HADZ))	/* no CMFFLAG and no place to prompt. */
		fatal("Background CASSI get with no CMRs\n");

retry:
	if (cmr[0] == NULL) {					/* No CMR list.  Make one. */
		if(HADZ && ((!isatty(0)) || (!isatty(1))))
		{
			fatal("Background CASSI get with invalid CMR\n");
		}
		fprintf (stdout, "Input Comma Separated List of CMRs: ");
		fgets (cmr, CMRLIMIT, stdin);
		for (cmri = 0; cmri < CMRLIMIT; cmri++)
			if (cmr[cmri] == '\n') {
				cmr[cmri] = NULL;
				break;
				}
		if (cmri == CMRLIMIT) {
			fprintf (stdout, "?Too many CMRs.\n");
			cmr[0] = NULL;
			goto retry;					/* Entry was too long. */
			}
		}

	/* Now, check the comma seperated list of CMRs for accuracy. */

	if (cmrcheck (cmr, Sflags[CMFFLAG - 'a']) != 0) {
		cmr[0] = NULL;
		goto retry;
		}
	else
		return (1);
}
