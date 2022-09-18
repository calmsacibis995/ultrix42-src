
#ifndef lint
static	char	*sccsid = "@(#)delta.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1985, 1988 by		*
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
 * Modification History:
 *
 *	19 Nov 84 -- depp
 *	   Change method of finding "bdiff" path, added "paths.h"
 *	7  Dec 84 -- depp
 *	   Fix UID problem so that delta forks a child to manipulate
 *	   files in user directory
 *	14 Dec 84 -- depp
 *	   Removed diagnostic from wait() return after manipulation of "g" file
 *	28 Jun 88 -- D. Long
 *	   Use atomic rename operation in xremove instead of link and unlink.
 *	   Also don't use return value from fputs, Ultrix is not SVID
 *	   compliant here and returns undefined.  Use ferror instead.
 *	14 Sep 88 -- D. Long
 *	   Use new constant LINEWIDTH throughtout for maximum text line length
 *	   instead of hard coded constants and BUFSIZ.  Also check to make
 *	   sure input lines do not exceed this size.
 *	14 Sep 88 -- D. Long
 *	   Don't try and sprintf entire comment string onto 1K buffer.
 *
 */
# include	"defines.h"
# include	"had.h"
# include 	"paths.h"

struct stat Statbuf;
char Null[1];
char Error[128];


char	Diffpgm[] = PROGPATH(bdiff);
FILE	*Diffin;
FILE	*Gin;
int	Debug = 0;
struct packet gpkt;
struct sid sid;
int	num_files;
char	had[26];
char	*ilist, *elist, *glist;
char	*Comments,Cmrs[300],*Mrs,*Nsid;
char	*auxf(), *logname(), *sid_ba();
int	Domrs;
int verbosity;
int	Did_id;
long	Szqfile;
char	Pfilename[FILESIZE];
FILE	 *fdfopen();
char *Cassin;
extern FILE	*Xiop;
extern int	Xcreate;

main(argc,argv)
int argc;
register char *argv[];
{
	register int i;
	register char *p;
	char c;
	char *sid_ab();
	int testmore;
	extern delta();
	extern	char	*savecmt();
	extern int Fcnt;

	Fflags = FTLEXIT | FTLMSG | FTLCLN;
	for(i=1; i<argc; i++)
		if(argv[i][0] == '-' && (c=argv[i][1])) {
			p = &argv[i][2];
			testmore = 0;
			switch (c) {

			case 'r':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				chksid(sid_ab(p,&sid),&sid);
				break;
			case 'g':
				glist = p;
				break;
			case 'y':
				Comments = p;
				break;
			case 'm':
				Mrs = p;
				repl(Mrs,'\n',' ');
				break;
			case 'p':
			case 'n':
			case 's':
				testmore++;
				break;
			case 'z':
				Cassin = p;
				break;
			default:
				fatal("unknown key letter (cm1)");
			}

			if (testmore) {
				testmore = 0;
				if (*p) {
					sprintf(Error,
						"value after %c arg (cm7)",c);
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
	if (!HADS)
		verbosity = -1;
	setsig();
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if (p=argv[i])
			do_file(p,delta);
	exit(Fcnt ? 1 : 0);
}


delta(file)
char *file;
{
	static int first = 1;
	int n, linenum;
	char type;
	register int ser;
	extern char had_dir, had_standinp;
	extern char *Sflags[];
	char nsid[50];
	char dfilename[FILESIZE];
	char gfilename[FILESIZE];
	char line[LINEWIDTH+1];
	char *getline();
	FILE  *dodiff();
	struct stats stats;
	struct pfile *pp, *rdpfile();
	struct stat sbuf;
	struct idel *dodelt();
	int inserted, deleted, orig;
	int newser;
	int status, i;
	int diffloop;
	int difflim;
	int holduid,holdgid;
	if (setjmp(Fjmp))
		return;
	sinit(&gpkt,file,1);
	if (first) {
		first = 0;
		dohist(file);
	}
	if (lockit(auxf(gpkt.p_file,'z'),2,getpid()))
		fatal("cannot create lock file (cm4)");
	gpkt.p_reopen = 1;
	gpkt.p_stdout = stdout;
	copy(auxf(gpkt.p_file,'g'),gfilename);
	Gin = xfopen(gfilename,0);
	pp = rdpfile(&gpkt,&sid);
	if (pp->pf_cmrlist) {
		strcpy(Cmrs,pp->pf_cmrlist);
	} else {
		Cmrs[0] = '\0';
	}
	if(!pp->pf_nsid.s_br)
		{
		 sprintf(nsid,"%d.%d",pp->pf_nsid.s_rel,pp->pf_nsid.s_lev);
		}
	else
		{
			sprintf(nsid,"%d.%d.%d.%d",pp->pf_nsid.s_rel,pp->pf_nsid.s_lev,pp->pf_nsid.s_br,pp->pf_nsid.s_seq);
		}
	Nsid=nsid;
	gpkt.p_cutoff = pp->pf_date;
	ilist = pp->pf_ilist;
	elist = pp->pf_elist;

	if (dodelt(&gpkt,&stats,(struct sid *) 0,0) == 0)
		fmterr(&gpkt);
	if ((ser = sidtoser(&pp->pf_gsid,&gpkt)) == 0 ||
		sidtoser(&pp->pf_nsid,&gpkt))
			fatal("invalid sid in p-file (de3)");
	doie(&gpkt,ilist,elist,glist);
	setup(&gpkt,ser);
	finduser(&gpkt);
	doflags(&gpkt);
	gpkt.p_reqsid = pp->pf_nsid;
	permiss(&gpkt);
	flushto(&gpkt,EUSERTXT,1);
	gpkt.p_chkeof = 1;
	copy(auxf(gpkt.p_file,'d'),dfilename);
	gpkt.p_gout = xfcreat(dfilename,0444);
	while(readmod(&gpkt)) {
		chkid(gpkt.p_line);
		fputs(gpkt.p_line,gpkt.p_gout);
		if(ferror(gpkt.p_gout))
			FAILPUT;
	}
	if(fclose(gpkt.p_gout)==EOF)
		FAILPUT;
	orig = gpkt.p_glnno;
	gpkt.p_glnno = 0;
	gpkt.p_verbose = verbosity;
	Did_id = 0;
	while (fgets(line,sizeof(line),Gin) != NULL && !chkid(line))
		;
	fclose(Gin);
	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
	if (!Did_id)
		if (Sflags[IDFLAG - 'a'])
			fatal("no id keywords (cm6)");
		else if (gpkt.p_verbose)
			fprintf(stderr,"No id keywords (cm7)\n");

	/*
	The following while loop executes 'bdiff' on g-file and
	d-file. If 'bdiff' fails (usually because segmentation
	limit it is using is too large for 'diff'), it is
	invoked again, with a lower segmentation limit.
	*/
	difflim = 3500;
	diffloop = 0;
	while (1) {
		inserted = deleted = 0;
		gpkt.p_glnno = 0;
		gpkt.p_upd = 1;
		gpkt.p_wrttn = 1;
		getline(&gpkt);
		gpkt.p_wrttn = 1;
		newser = mkdelt(&gpkt,&pp->pf_nsid,&pp->pf_gsid,
						diffloop,orig);
		diffloop = 1;
		flushto(&gpkt,EUSERTXT,0);
		Diffin = dodiff(auxf(gpkt.p_file,'g'),dfilename,difflim);
		while (n = getdiff(&type,&linenum)) {
			if (type == INS) {
				inserted += n;
				insert(&gpkt,linenum,n,newser);
			}
			else {
				deleted += n;
				delete(&gpkt,linenum,n,newser);
			}
		}
		fclose(Diffin);
		if (gpkt.p_iop)
			while (readmod(&gpkt))
				;
		wait(&status);
		if (status) {		/* diff failed */
			/*
			Check top byte (exit code of child).
			*/
			if (((status >> 8) & 0377) == 32) { /* 'execl' failed */
				sprintf(Error,
					"cannot execute '%s' (de12)",Diffpgm);
				fatal(Error);
			}
			/*
			Re-try.
			*/
			if (difflim -= 500) {	/* reduce segmentation */
				fprintf(stderr,
			"'%s' failed, re-trying, segmentation = %d (de13)\n",
					Diffpgm,difflim);
				if(fclose(Xiop)==EOF)	/* set up */
					FAILPUT;
				Xiop = 0;	/* for new x-file */
				Xcreate = 0;
				/*
				Re-open s-file.
				*/
				gpkt.p_iop = xfopen(gpkt.p_file,0);
				setbuf(gpkt.p_iop,gpkt.p_buf);
				/*
				Reset counters.
				*/
				gpkt.p_slnno = 0;
				gpkt.p_ihash = 0;
				gpkt.p_chash = 0;
				gpkt.p_nhash = 0;
				gpkt.p_keep = 0;
			}
			else
				/* tried up to 500 lines, can't go on */
				fatal("diff failed (de4)");
		}
		else {		/* no need to try again, worked */
			break;			/* exit while loop */
		}
	}
	fgetchk(gfilename,&gpkt);
	unlink(dfilename);
	stats.s_ins = inserted;
	stats.s_del = deleted;
	stats.s_unc = orig - deleted;
	if (gpkt.p_verbose) {
		fprintf(gpkt.p_stdout,"%u inserted\n",stats.s_ins);
		fprintf(gpkt.p_stdout,"%u deleted\n",stats.s_del);
		fprintf(gpkt.p_stdout,"%u unchanged\n",stats.s_unc);
	}
	flushline(&gpkt,&stats);
	stat(gpkt.p_file,&sbuf);
	xrename(auxf(gpkt.p_file,'x'),gpkt.p_file);
	chown(gpkt.p_file,sbuf.st_uid,sbuf.st_gid);
	if (Szqfile)
		xrename(auxf(gpkt.p_file,'q'),Pfilename);
	else {
		xunlink(Pfilename);
		xunlink(auxf(gpkt.p_file,'q'));
	}
	clean_up(0);
	if (!HADN) {
		fflush(gpkt.p_stdout);
		if ((i = fork()) < 0)
			fatal("cannot fork, try again");
		if (i == 0) {
			setuid(getuid());
			setgid(getgid());
			unlink(gfilename);
			exit(0);
		}
		else 
			wait(0);
	}
}


mkdelt(pkt,sp,osp,diffloop,orig_nlines)
struct packet *pkt;
struct sid *sp, *osp;
int diffloop;
int orig_nlines;
{
	extern long Timenow;
	struct deltab dt;
	char str[LINEWIDTH+1];
	char *del_ba();
	int newser;
	extern char *Sflags[];
	register char *p;
	int ser_inc, opred, nulldel;

	if (!diffloop && pkt->p_verbose) {
		sid_ba(sp,str);
		fprintf(pkt->p_stdout,"%s\n",str);
		fflush(pkt->p_stdout);
	}
	sprintf(str,"%c%c00000\n",CTLCHAR,HEAD);
	putline(pkt,str);
	newstats(pkt,str,"0");
	dt.d_sid = *sp;

	/*
	Check if 'null' deltas should be inserted
	(only if 'null' flag is in file and
	releases are being skipped) and set
	'nulldel' indicator appropriately.
	*/
	if (Sflags[NULLFLAG - 'a'] && (sp->s_rel > osp->s_rel + 1) &&
			!sp->s_br && !sp->s_seq &&
			!osp->s_br && !osp->s_seq)
		nulldel = 1;
	else
		nulldel = 0;
	/*
	Calculate how many serial numbers are needed.
	*/
	if (nulldel)
		ser_inc = sp->s_rel - osp->s_rel;
	else
		ser_inc = 1;
	/*
	Find serial number of the new delta.
	*/
	newser = dt.d_serial = maxser(pkt) + ser_inc;
	/*
	Find old predecessor's serial number.
	*/
	opred = sidtoser(osp,pkt);
	if (nulldel)
		dt.d_pred = newser - 1;	/* set predecessor to 'null' delta */
	else
		dt.d_pred = opred;
	dt.d_datetime = Timenow;
	strncpy(dt.d_pgmr,logname(),LOGSIZE-1);
	dt.d_type = 'D';
	del_ba(&dt,str);
	putline(pkt,str);
	if (ilist)
		mkixg(pkt,INCLUSER,INCLUDE);
	if (elist)
		mkixg(pkt,EXCLUSER,EXCLUDE);
	if (glist)
		mkixg(pkt,IGNRUSER,IGNORE);
	if (Mrs) {
		if (!(p = Sflags[VALFLAG - 'a']))
			fatal("MRs not allowed (de8)");
		if (*p && !diffloop && valmrs(pkt,p))
			fatal("invalid MRs (de9)");
		putmrs(pkt);
	}
	else if (Sflags[VALFLAG - 'a'])
		fatal("MRs required (de10)");
/*
*
* CMF enhancement
*
*/
	if(Sflags[CMFFLAG - 'a'])
		{
		 if(Mrs)
			{
			 error("input CMR's ignored");
			 Mrs = "";
			}
		if(!deltack(pkt->p_file,Cmrs,Nsid,Sflags[CMFFLAG - 'a']))
			{
			 fatal("Delta denied due to CMR difficulties");
			}
		 putcmrs(pkt); /* this routine puts cmrs on the out put file */
		}
	sprintf(str,"%c%c ",CTLCHAR,COMMENTS);
	putline(pkt,str);
/*
	sprintf(str,"%s",savecmt(Comments));
	putline(pkt,str);
*/
	putline(pkt, savecmt(Comments));	/* D. Long - 9/14/88 */
	putline(pkt,"\n");
	sprintf(str,CTLSTR,CTLCHAR,EDELTAB);
	putline(pkt,str);
	if (nulldel)			/* insert 'null' deltas */
		while (--ser_inc) {
			sprintf(str,"%c%c %s/%s/%05u\n", CTLCHAR, STATS,
				"00000", "00000", orig_nlines);
			putline(pkt,str);
			dt.d_sid.s_rel -= 1;
			dt.d_serial -= 1;
			if (ser_inc != 1)
				dt.d_pred -= 1;
			else
				dt.d_pred = opred;	/* point to old pred */
			del_ba(&dt,str);
			putline(pkt,str);
			sprintf(str,"%c%c ",CTLCHAR,COMMENTS);
			putline(pkt,str);
			putline(pkt,"AUTO NULL DELTA\n");
			sprintf(str,CTLSTR,CTLCHAR,EDELTAB);
			putline(pkt,str);
		}
	return(newser);
}


mkixg(pkt,reason,ch)
struct packet *pkt;
int reason;
char ch;
{
	int n;
	char str[LINEWIDTH+1];

	sprintf(str,"%c%c",CTLCHAR,ch);
	putline(pkt,str);
	for (n = maxser(pkt); n; n--) {
		if (pkt->p_apply[n].a_reason == reason) {
			sprintf(str," %u",n);
			putline(pkt,str);
		}
	}
	putline(pkt,"\n");
}


# define	LENMR	60

putmrs(pkt)
struct packet *pkt;
{
	register char **argv;
	char str[LENMR+6];
	extern char **Varg;

	for (argv = &Varg[PWVSTART]; *argv; argv++) {
		sprintf(str,"%c%c %s\n",CTLCHAR,MRNUM,*argv);
		putline(pkt,str);
	}
}



/*
*
*	putcmrs takes the cmrs list on the Mrs line built by deltack
* 	and puts them in the packet
*	
*/
	putcmrs(pkt)    
	struct packet *pkt;
	{
		char str[510];
		sprintf(str,"%c%c %s\n",CTLCHAR,MRNUM,Cmrs);
		putline(pkt,str);
	}


static char ambig[] = "ambiguous `r' keyletter value (de15)";

struct pfile *
rdpfile(pkt,sp)
register struct packet *pkt;
struct sid *sp;
{
	char *user;
	struct pfile pf;
	static struct pfile goodpf;
	char line[LINEWIDTH+1];
	int cnt, uniq;
	FILE *in, *out;

	uniq = cnt = -1;
	user = logname();
	zero(&goodpf,sizeof(goodpf));
	in = xfopen(auxf(pkt->p_file,'p'),0);
	out = xfcreat(auxf(pkt->p_file,'q'),0644);
	while (fgets(line,sizeof(line),in) != NULL) {
		pf_ab(line,&pf,1);
		if (equal(pf.pf_user,user)) {
			if (sp->s_rel == 0) {
				if (++cnt) {
					if(fclose(out)==EOF)
						FAILPUT;
					fclose(in);
					fatal("missing -r argument (de1)");
				}
				goodpf = pf;
				continue;
			}
			else if ((sp->s_rel == pf.pf_nsid.s_rel &&
				sp->s_lev == pf.pf_nsid.s_lev &&
				sp->s_br == pf.pf_nsid.s_br &&
				sp->s_seq == pf.pf_nsid.s_seq) ||
				(sp->s_rel == pf.pf_gsid.s_rel &&
				sp->s_lev == pf.pf_gsid.s_lev &&
				sp->s_br == pf.pf_gsid.s_br &&
				sp->s_seq == pf.pf_gsid.s_seq)) {
					if (++uniq) {
						fclose(in);
						if(fclose(out)==EOF)
							FAILPUT;
						fatal(ambig);
					}
					goodpf = pf;
					continue;
			}
		}
		fputs(line,out);
		if(ferror(out))
			FAILPUT;
	}
	if(fflush(out)==EOF)
		FAILPUT;
	fstat(fileno(out),&Statbuf);
	Szqfile = Statbuf.st_size;
	copy(auxf(pkt->p_file,'p'),Pfilename);
	fclose(out);
	fclose(in);
	if (!goodpf.pf_user[0])
		fatal("login name or SID specified not in p-file (de2)");
	return(&goodpf);
}


FILE *
dodiff(newf,oldf,difflim)
char *newf, *oldf;
int difflim;
{
	register int i;
	int pfd[2];
	FILE *iop;
	extern char Diffpgm[];
	char num[10];

	xpipe(pfd);
	if ((i = fork()) < 0) {
		close(pfd[0]);
		close(pfd[1]);
		fatal("cannot fork, try again (de11)");
	}
	else if (i == 0) {
		close(pfd[0]);
		close(1);
		dup(pfd[1]);
		close(pfd[1]);
		for (i = 5; i < 15; i++)
			close(i);
		sprintf(num,"%d",difflim);
		execl(Diffpgm,Diffpgm,oldf,newf,num,"-s",0);
		close(1);
		exit(32);	/* tell parent that 'execl' failed */
	}
	else {
		close(pfd[1]);
		iop = fdfopen(pfd[0],0);
		return(iop);
	}
}


getdiff(type,plinenum)
register char *type;
register int *plinenum;
{
	char line[LINEWIDTH+1];
	register char *p;
	char *rddiff(), *linerange();
	int num_lines;
	static int chg_num, chg_ln;
	int lowline, highline;

	if ((p = rddiff(line, sizeof line)) == NULL)
		return(0);

	if (*p == '-') {
		*type = INS;
		*plinenum = chg_ln;
		num_lines = chg_num;
	}
	else {
		p = linerange(p,&lowline,&highline);
		*plinenum = lowline;

		switch(*p++) {
		case 'd':
			num_lines = highline - lowline + 1;
			*type = DEL;
			/* ch skipline -> skiplines -- depp */
			skiplines(line,num_lines);
			break;

		case 'a':
			linerange(p,&lowline,&highline);
			num_lines = highline - lowline + 1;
			*type = INS;
			break;

		case 'c':
			chg_ln = lowline;
			num_lines = highline - lowline + 1;
			linerange(p,&lowline,&highline);
			chg_num = highline - lowline + 1;
			*type = DEL;
			/* ch skipline -> skiplines -- depp */
			skiplines(line,num_lines);
			break;
		}
	}

	return(num_lines);
}


insert(pkt,linenum,n,ser)
register struct packet *pkt;
register int linenum;
register int n;
int ser;
{
	char str[LINEWIDTH+1];

	after(pkt,linenum);
	sprintf(str,"%c%c %u\n",CTLCHAR,INS,ser);
	putline(pkt,str);
	for (++n; --n; ) {
		rddiff(str,sizeof(str));
		putline(pkt,&str[2]);
	}
	sprintf(str,"%c%c %u\n",CTLCHAR,END,ser);
	putline(pkt,str);
}


delete(pkt,linenum,n,ser)
register struct packet *pkt;
register int linenum;
int n;
register int ser;
{
	char str[LINEWIDTH+1];

	before(pkt,linenum);
	sprintf(str,"%c%c %u\n",CTLCHAR,DEL,ser);
	putline(pkt,str);
	after(pkt,linenum + n - 1);
	sprintf(str,"%c%c %u\n",CTLCHAR,END,ser);
	putline(pkt,str);
}


after(pkt,n)
register struct packet *pkt;
register int n;
{
	before(pkt,n);
	if (pkt->p_glnno == n)
		putline(pkt,(char *) 0);
}


before(pkt,n)
register struct packet *pkt;
register int n;
{
	while (pkt->p_glnno < n) {
		if (!readmod(pkt))
			break;
	}
}


char *
linerange(cp,low,high)
register char *cp;
register int *low, *high;
{
	cp = satoi(cp,low);
	if (*cp == ',')
		cp = satoi(++cp,high);
	else
		*high = *low;

	return(cp);
}


skiplines(lp,num)
register char *lp;
register int num;
{
	for (++num;--num;)
		rddiff(lp, LINEWIDTH);
}


char *
rddiff(s,n)
register char *s;
register int n;
{
	register char *r;

	if ((r = fgets(s,n,Diffin)) != NULL && HADP) {
		fputs(s,gpkt.p_stdout);
		if(ferror(gpkt.p_stdout))
			FAILPUT;
	}
	return(r);
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
	ap = &pkt->p_apply[n];
	if (pkt->p_cutoff > pkt->p_idel[n].i_datetime)
		switch(ap->a_code) {
	
		case SX_EMPTY:
			switch (ch) {
			case INCLUDE:
				condset(ap,APPLY,INCLUSER);
				break;
			case EXCLUDE:
				condset(ap,NOAPPLY,EXCLUSER);
				break;
			case IGNORE:
				condset(ap,SX_EMPTY,IGNRUSER);
				break;
			}
			break;
		case APPLY:
			fatal("internal error in delta/enter() (de5)");
			break;
		case NOAPPLY:
			fatal("internal error in delta/enter() (de6)");
			break;
		default:
			fatal("internal error in delta/enter() (de7)");
			break;
		}
}


escdodelt()	/* dummy routine for dodelt() */
{
}
fredck()	/*dummy routine for dodelt()*/
{
}

clean_up(n)
{
	if (mylock(auxf(gpkt.p_file,'z'),getpid())) {
		if (gpkt.p_iop)
			fclose(gpkt.p_iop);
		if (Xiop) {
			fclose(Xiop);
			unlink(auxf(gpkt.p_file,'x'));
		}
		if(Gin)
			fclose(Gin);
		unlink(auxf(gpkt.p_file,'d'));
		unlink(auxf(gpkt.p_file,'q'));
		xrm(&gpkt);
		ffreeall();
		unlockit(auxf(gpkt.p_file,'z'),getpid());
	}
}


static char bd[] = "leading SOH char in line %d of file `%s' not allowed (de14)";

fgetchk(file,pkt)
register char	*file;
register struct packet *pkt;
{
	FILE	*iptr;
	char	line[LINEWIDTH+1];
	register int k;
	int i;

	iptr = xfopen(file,0);
	for (k = 1; fgets(line,sizeof(line),iptr); k++) {
		if (*line == CTLCHAR) {
			fclose(iptr);
			sprintf(Error,bd,k,auxf(pkt->p_file,'g'));
			fatal(Error);
		}
		i = strlen(line);
		if(i == ((sizeof line)-1) && line[i-1] != '\n') {
			sprintf(Error, "%s line %d is longer than %d characters", /* (de??) */
				file, k, LINEWIDTH);
			fatal(Error);
		}
		if(line[i-1] != '\n') {
			sprintf(Error, "%s line %d does not end with a newline", /* (de??) */
				file, k);
			fatal(Error);
		}
	}
	fclose(iptr);
}
