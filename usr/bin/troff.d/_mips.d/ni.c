#ifndef lint
static char *sccsid = "@(#)ni.c	4.1	(ULTRIX)	7/17/90";
#endif lint
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: ni.c,v 1.4 86/11/12 19:06:18 dce Exp $ */

#include "tdef.h"
char obuf[OBUFSZ];
char *obufp = obuf;
int r[NN] = {
	PAIR('%',0),
	PAIR('n','l'),
	PAIR('y','r'),
	PAIR('h','p'),
	PAIR('c','t'),
	PAIR('d','n'),
	PAIR('m','o'),
	PAIR('d','y'),
	PAIR('d','w'),
	PAIR('l','n'),
	PAIR('d','l'),
	PAIR('s','t'),
	PAIR('s','b'),
	PAIR('c','.')};
int pto = 10000;
int pfrom = 1;
int print = 1;
char nextf[NS] = "/usr/lib/tmac/tmac.xxxxx";
int nfi = 19;
#ifdef NROFF
char termtab[NS] = "/usr/lib/term/tab37";
int tti = 17;
#endif
#ifndef NROFF
int oldbits = -1;
#endif
int init = 1;
int fc = IMP;
int eschar = '\\';
int pl = 11*INCH;
int po = PO;
int dfact = 1;
int dfactd = 1;
int res = 1;
int smnt = 4;
int ascii = ASCII;
int ptid = PTID;
char ptname[] = "/dev/cat";
int lg = LG;
int pnlist[NPN] = {-1};
int *pnp = pnlist;
int npn = 1;
int npnflg = 1;
int xflg = 1;
int dpn = -1;
int totout = 1;
int ulfont = 1;
int ulbit = 1<<9;
int tabch = TAB;
int ldrch = LEADER;
int xxx;
extern caseds(), caseas(), casesp(), caseft(), caseps(), casevs(),
casenr(), caseif(), casepo(), casetl(), casetm(), casebp(), casech(),
casepn(), tbreak(), caseti(), casene(), casenf(), casece(), casefi(),
casein(), caseli(), casell(), casens(), casemk(), casert(), caseam(),
casede(), casedi(), caseda(), casewh(), casedt(), caseit(), caserm(),
casern(), casead(), casers(), casena(), casepl(), caseta(), casetr(),
caseul(), caselt(), casenx(), caseso(), caseig(), casetc(), casefc(),
caseec(), caseeo(), caselc(), caseev(), caserd(), caseab(), casefl(),
done(), casess(), casefp(), casecs(), casebd(), caselg(), casehc(),
casehy(), casenh(), casenm(), casenn(), casesv(), caseos(), casels(),
casecc(), casec2(), caseem(), caseaf(), casehw(), casemc(), casepm(),
casecu(), casepi(), caserr(), caseuf(), caseie(), caseel(), casepc(),
caseht();
#ifndef NROFF
extern casefz();
#endif
extern casecf();
struct contab {
	int rq;
/*
	union {
 */
		int (*f)();
/*
		unsigned mx;
	}x;
 */
}contab[NM]= {
	PAIR('d','s'),caseds,
	PAIR('a','s'),caseas,
	PAIR('s','p'),casesp,
	PAIR('f','t'),caseft,
	PAIR('p','s'),caseps,
	PAIR('v','s'),casevs,
	PAIR('n','r'),casenr,
	PAIR('i','f'),caseif,
	PAIR('i','e'),caseie,
	PAIR('e','l'),caseel,
	PAIR('p','o'),casepo,
	PAIR('t','l'),casetl,
	PAIR('t','m'),casetm,
	PAIR('b','p'),casebp,
	PAIR('c','h'),casech,
	PAIR('p','n'),casepn,
	PAIR('b','r'),tbreak,
	PAIR('t','i'),caseti,
	PAIR('n','e'),casene,
	PAIR('n','f'),casenf,
	PAIR('c','e'),casece,
	PAIR('f','i'),casefi,
	PAIR('i','n'),casein,
	PAIR('l','i'),caseli,
	PAIR('l','l'),casell,
	PAIR('n','s'),casens,
	PAIR('m','k'),casemk,
	PAIR('r','t'),casert,
	PAIR('a','m'),caseam,
	PAIR('d','e'),casede,
	PAIR('d','i'),casedi,
	PAIR('d','a'),caseda,
	PAIR('w','h'),casewh,
	PAIR('d','t'),casedt,
	PAIR('i','t'),caseit,
	PAIR('r','m'),caserm,
	PAIR('r','r'),caserr,
	PAIR('r','n'),casern,
	PAIR('a','d'),casead,
	PAIR('r','s'),casers,
	PAIR('n','a'),casena,
	PAIR('p','l'),casepl,
	PAIR('t','a'),caseta,
	PAIR('t','r'),casetr,
	PAIR('u','l'),caseul,
	PAIR('c','u'),casecu,
	PAIR('l','t'),caselt,
	PAIR('n','x'),casenx,
	PAIR('s','o'),caseso,
	PAIR('i','g'),caseig,
	PAIR('t','c'),casetc,
	PAIR('f','c'),casefc,
	PAIR('e','c'),caseec,
	PAIR('e','o'),caseeo,
	PAIR('l','c'),caselc,
	PAIR('e','v'),caseev,
	PAIR('r','d'),caserd,
	PAIR('a','b'),caseab,
	PAIR('f','l'),casefl,
	PAIR('e','x'),done,
	PAIR('s','s'),casess,
	PAIR('f','p'),casefp,
	PAIR('c','s'),casecs,
	PAIR('b','d'),casebd,
	PAIR('l','g'),caselg,
	PAIR('h','c'),casehc,
	PAIR('h','y'),casehy,
	PAIR('n','h'),casenh,
	PAIR('n','m'),casenm,
	PAIR('n','n'),casenn,
	PAIR('s','v'),casesv,
	PAIR('o','s'),caseos,
	PAIR('l','s'),casels,
	PAIR('c','c'),casecc,
	PAIR('c','2'),casec2,
	PAIR('e','m'),caseem,
	PAIR('a','f'),caseaf,
	PAIR('h','w'),casehw,
	PAIR('m','c'),casemc,
	PAIR('p','m'),casepm,
#ifdef NROFF
	PAIR('p','i'),casepi,
#endif
	PAIR('u','f'),caseuf,
	PAIR('p','c'),casepc,
	PAIR('h','t'),caseht,
#ifndef NROFF
	PAIR('f','z'),casefz,
#endif
	PAIR('c', 'f'),casecf,
};

/*
troff environment block - IF YOU CHANGE THIS, MAKE SURE THAT YOU MAKE
	EQUIVALENT CHANGES TO THE ROUTINES AND STRUCT LATER IN THIS
	FILE.
*/

int block = 0;
int ics = ICS;
int ic = 0;
int icf = 0;
int chbits = 0;
int spbits = 0;
int nmbits = 0;
int apts = PS;
int apts1 = PS;
int pts = PS;
int pts1 = PS;
int font = FT;
int font1 = FT;
int sps = SPS;
int spacesz = SS;
int lss = VS;
int lss1 = VS;
int ls = 1;
int ls1 = 1;
int ll = LL;
int ll1 = LL;
int lt = LL;
int lt1 = LL;
int ad = 1;
int nms = 1;
int ndf = 1;
int fi = 1;
int cc = '.';
int c2 = '\'';
int ohc = OHC;
int tdelim = IMP;
int hyf = 1;
int hyoff = 0;
int un1 = -1;
int tabc = 0;
int dotc = '.';
int adsp = 0;
int adrem = 0;
int lastl = 0;
int nel = 0;
int admod = 0;
int *wordp = 0;
int spflg = 0;
int *linep = 0;
int *wdend = 0;
int *wdstart = 0;
int wne = 0;
int ne = 0;
int nc = 0;
int nb = 0;
int lnmod = 0;
int nwd = 0;
int nn = 0;
int ni = 0;
int ul = 0;
int cu = 0;
int ce = 0;
int in = 0;
int in1 = 0;
int un = 0;
int wch = 0;
int pendt = 0;
int *pendw = 0;
int pendnf = 0;
int spread = 0;
int it = 0;
int itmac = 0;
int lnsize = LNSIZE;
int *hyptr[NHYP] = {0};
int tabtab[NTAB] = {DTAB,DTAB*2,DTAB*3,DTAB*4,DTAB*5,DTAB*6,DTAB*7,DTAB*8,
	DTAB*9,DTAB*10,DTAB*11,DTAB*12,DTAB*13,DTAB*14,DTAB*15,0};
int line[LNSIZE] = {0};
int word[WDSIZE] = {0};
int blockxxx[EVS-68-NHYP-NTAB-WDSIZE-LNSIZE] = {0};
/*spare 5 words*/
int oline[LNSIZE+1];

/*
 * The following mess is brought to you as a result of folks long ago
 * not having struct's. Basically, [nt]roff has a list of variables
 * called the environment. There are three environments, and by using
 * the .ev command, you can change to the one you need. Internally,
 * this was done by saving 3 copies of the environment into /tmp/taXXXXXX
 * and saving and restoring by addressing off of the first variable
 * in the list, "block". This doesn't work on smart compilers with
 * global register analysis and stuff.
 *
 * In order to avoid a lot of recoding, the saves and restores are
 * done through the following routines, which do a lot of copying.
 * The proper solution is to use all struct references, but we don't
 * have time, and maybe we'll get lucky and use System V nroff or
 * ditroff.
 *
 * Below is the structure. If you make changes to it, you must also
 * fix ni.c.
 */

struct env_block {
	int block; int ics; int ic; int icf; int chbits; int spbits;
	int nmbits; int apts; int apts1; int pts; int pts1; int font;
	int font1; int sps; int spacesz; int lss; int lss1; int ls;
	int ls1; int ll; int ll1; int lt; int lt1; int ad; int nms;
	int ndf; int fi; int cc; int c2; int ohc; int tdelim; int hyf;
	int hyoff; int un1; int tabc; int dotc; int adsp; int adrem;
	int lastl; int nel; int admod; int *wordp; int spflg; int *linep;
	int *wdend; int *wdstart; int wne; int ne; int nc; int nb;
	int lnmod; int nwd; int nn; int ni; int ul; int cu; int ce;
	int in; int in1; int un; int wch; int pendt; int *pendw;
	int pendnf; int spread; int it; int itmac; int lnsize;
	int *hyptr[NHYP]; int tabtab[NTAB]; int line[LNSIZE];
	int word[WDSIZE]; int blockxxx[EVS-68-NHYP-NTAB-WDSIZE-LNSIZE];
	/*spare 5 words*/
	int oline[LNSIZE+1];
};

static struct env_block S_env;

/*
 * The subroutine save_env() takes the data in the various environment
 * block variables and stores them in the I/O structure. This is just
 * a series of assignment statements and a couple of block copies.
 */

save_env()
{

/*
 * Simple variables
 */

	S_env.block = block; S_env.ics = ics; S_env.ic = ic; S_env.icf = icf;
	S_env.chbits = chbits; S_env.spbits = spbits; S_env.nmbits = nmbits;
	S_env.apts = apts; S_env.apts1 = apts1; S_env.pts = pts;
	S_env.pts1 = pts1; S_env.font = font; S_env.font1 = font1;
	S_env.sps = sps; S_env.spacesz = spacesz; S_env.lss = lss;
	S_env.lss1 = lss1; S_env.ls = ls; S_env.ls1 = ls1; S_env.ll = ll;
	S_env.ll1 = ll1; S_env.lt = lt; S_env.lt1 = lt1; S_env.ad = ad;
	S_env.nms = nms; S_env.ndf = ndf; S_env.fi = fi; S_env.cc = cc;
	S_env.c2 = c2; S_env.ohc = ohc; S_env.tdelim = tdelim;
	S_env.hyf = hyf; S_env.hyoff = hyoff; S_env.un1 = un1;
	S_env.tabc = tabc; S_env.dotc = dotc; S_env.adsp = adsp;
	S_env.adrem = adrem; S_env.lastl = lastl; S_env.nel = nel;
	S_env.admod = admod; S_env.wordp = wordp; S_env.spflg = spflg;
	S_env.linep = linep; S_env.wdend = wdend; S_env.wdstart = wdstart;
	S_env.wne = wne; S_env.ne = ne; S_env.nc = nc; S_env.nb = nb;
	S_env.lnmod = lnmod; S_env.nwd = nwd; S_env.nn = nn; S_env.ni = ni;
	S_env.ul = ul; S_env.cu = cu; S_env.ce = ce; S_env.in = in;
	S_env.in1 = in1; S_env.un = un; S_env.wch = wch; S_env.pendt = pendt;
	S_env.pendw = pendw; S_env.pendnf = pendnf; S_env.spread = spread;
	S_env.it = it; S_env.itmac = itmac; S_env.lnsize = lnsize;

/*
 * Arrays
 */

	bcopy(hyptr, S_env.hyptr, (NHYP * sizeof(int *)));
	bcopy(tabtab, S_env.tabtab, (NTAB * sizeof(int)));
	bcopy(line, S_env.line, (LNSIZE * sizeof(int)));
	bcopy(word, S_env.word, (WDSIZE * sizeof(int)));
	bcopy(blockxxx, S_env.blockxxx, ((EVS-68-NHYP-NTAB-WDSIZE-LNSIZE) * sizeof(int)));
	bcopy(oline, S_env.oline, ((LNSIZE + 1) * sizeof(int)));

}

/*
 * The subroutine write_env() writes the current S_env data into the
 * desired environment slot in the given file.
 */

#include <sys/file.h>

write_env(fd, slot)
int fd;
int slot;
{

	(void) lseek(fd, (long) (slot * sizeof(struct env_block)), L_SET);
	write(fd, &S_env, (long) sizeof(struct env_block));
}


/*
 * The subroutine read_env() reads the requested environment from the
 * desired environment slot in the given file, and places this data into
 * the environment block variables.
 */

read_env(fd, slot)
int fd;
int slot;
{
	(void) lseek(fd, (long) (slot * sizeof(struct env_block)), L_SET);
	read(fd, &S_env, (long) sizeof(struct env_block));

/*
 * Simple variables
 */

	block = S_env.block;
	ics = S_env.ics;
	ic = S_env.ic;
	icf = S_env.icf;
	chbits = S_env.chbits;
	spbits = S_env.spbits;
	nmbits = S_env.nmbits;
	apts = S_env.apts;
	apts1 = S_env.apts1;
	pts = S_env.pts;
	pts1 = S_env.pts1;
	font = S_env.font;
	font1 = S_env.font1;
	sps = S_env.sps;
	spacesz = S_env.spacesz;
	lss = S_env.lss;
	lss1 = S_env.lss1;
	ls = S_env.ls;
	ls1 = S_env.ls1;
	ll = S_env.ll;
	ll1 = S_env.ll1;
	lt = S_env.lt;
	lt1 = S_env.lt1;
	ad = S_env.ad;
	nms = S_env.nms;
	ndf = S_env.ndf;
	fi = S_env.fi;
	cc = S_env.cc;
	c2 = S_env.c2;
	ohc = S_env.ohc;
	tdelim = S_env.tdelim;
	hyf = S_env.hyf;
	hyoff = S_env.hyoff;
	un1 = S_env.un1;
	tabc = S_env.tabc;
	dotc = S_env.dotc;
	adsp = S_env.adsp;
	adrem = S_env.adrem;
	lastl = S_env.lastl;
	nel = S_env.nel;
	admod = S_env.admod;
	wordp = S_env.wordp;
	spflg = S_env.spflg;
	linep = S_env.linep;
	wdend = S_env.wdend;
	wdstart = S_env.wdstart;
	wne = S_env.wne;
	ne = S_env.ne;
	nc = S_env.nc;
	nb = S_env.nb;
	lnmod = S_env.lnmod;
	nwd = S_env.nwd;
	nn = S_env.nn;
	ni = S_env.ni;
	ul = S_env.ul;
	cu = S_env.cu;
	ce = S_env.ce;
	in = S_env.in;
	in1 = S_env.in1;
	un = S_env.un;
	wch = S_env.wch;
	pendt = S_env.pendt;
	pendw = S_env.pendw;
	pendnf = S_env.pendnf;
	spread = S_env.spread;
	it = S_env.it;
	itmac = S_env.itmac;
	lnsize = S_env.lnsize;

/*
 * Arrays
 */

	bcopy(S_env.hyptr, hyptr, (NHYP * sizeof(int *)));
	bcopy(S_env.tabtab, tabtab, (NTAB * sizeof(int)));
	bcopy(S_env.line, line, (LNSIZE * sizeof(int)));
	bcopy(S_env.word, word, (WDSIZE * sizeof(int)));
	bcopy(S_env.blockxxx, blockxxx, ((EVS-68-NHYP-NTAB-WDSIZE-LNSIZE) * sizeof(int)));
	bcopy(S_env.oline, oline, ((LNSIZE + 1) * sizeof(int)));
}
