# ifndef lint
static char *sccsid = "@(#)dumpitime.c	4.1    (ULTRIX)        7/2/90";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

/* ------------------------------------------------------------------------
 * Modification History: /usr/src/etc/dump/dumpitime.c
 *
 * 29 Jan 86 -- fries
 *	Added Copyright notice and comments to code.
 *
 * ------------------------------------------------------------------------
 */
#include "dump.h"

char *prdate(d)
	time_t d;
{
	char *p;

	if(d == 0)
		return("the epoch");
	p = ctime(&d);
	p[24] = 0;
	return(p);
}

struct	idates	**idatev = 0;
int	nidates = 0;
int	idates_in = 0;
struct	itime	*ithead = 0;

inititimes()
{
			FILE	*df;
	register	int	i;
	register	struct	itime	*itwalk;
			int fd;

	if (idates_in)
		return;
	fd = open(increm, O_RDONLY); /* Open `/etc/dumpdates' file */
	if (fd < 0) {
 		msg("WARNING - ");
		perror(increm);
 		--uflag;
		return;
	}
	(void) flock(fd, LOCK_SH);
	if ((df = fdopen(fd, "r")) == NULL) {
		nidates = 0;
		ithead = 0;
	} else {
		do{
			/* get records from `/etc/dumpdates' file */
			itwalk=(struct itime *)calloc(1,sizeof (struct itime));
			if (getrecord(df, &(itwalk->it_value)) < 0)
				break;
			nidates++;
			itwalk->it_next = ithead; /* reverse link to
last */
			ithead = itwalk;
		} while (1);
		fclose(df);
	}

	idates_in = 1;
	/*
	 *	arrayify the list, leaving enough room for the additional
	 *	record that we may have to add to the idate structure
	 */
	/*  Put pointers to time data structures into array of pointers
*/
	idatev = (struct idates **)calloc(nidates + 1,sizeof (struct idates *));
	for (i = nidates-1, itwalk = ithead; i >= 0; i--, itwalk = itwalk->it_next)
		idatev[i] = &itwalk->it_value;
}

getitime()
{
	register	struct	idates	*ip;
	register	int	i;
			char	*fname;

	fname = disk;
#ifdef FDEBUG
	msg("Looking for name %s in increm = %s for delta = %c\n",
		fname, increm, incno);
#endif
	spcl.c_ddate = 0;
	lastincno = '0';

	inititimes();
	/*
	 *	Go find the entry with the same name for a lower increment
	 *	and older date
	 */
	ITITERATE(i, ip){
		if(strncmp(fname, ip->id_name,
				sizeof (ip->id_name)) != 0)
			continue;
		if (ip->id_incno >= incno)
			continue;
		if (ip->id_ddate <= spcl.c_ddate)
			continue;
		spcl.c_ddate = ip->id_ddate;
 		lastincno = ip->id_incno;
	} 
}

/* Write dump date/time stamp to the `/etc/dumpdates' file */
putitime()
{
	FILE		*df;
	register	struct	idates	*itwalk;
	register	int	i;
	int		fd;
	char		*fname;

	if(uflag == 0)
		return;
	fd = open(temp, O_RDWR|O_CREAT, 0600);
	if (fd < 0) {
		perror(temp);
		dumpabort();
	}
	(void) flock(fd, LOCK_EX);
	if ((df = fdopen(fd, "w")) == NULL) {
		perror(temp);
		dumpabort();
	}
	fname = disk;
	free(idatev);
	idatev = 0;
	nidates = 0;
	ithead = 0;
	idates_in = 0;
	inititimes();
	
	spcl.c_ddate = 0;
	ITITERATE(i, itwalk){
		if (strncmp(fname, itwalk->id_name,
				sizeof (itwalk->id_name)) != 0)
			continue;
		if (itwalk->id_incno != incno)
			continue;
		goto found;
	}
	/*
	 *	construct the new upper bound;
	 *	Enough room has been allocated.
	 */
	itwalk = idatev[nidates] =
		(struct idates *)calloc(1, sizeof(struct idates));
	nidates += 1;
  found:
	strncpy(itwalk->id_name, fname, sizeof (itwalk->id_name));
	itwalk->id_incno = incno;
	itwalk->id_ddate = spcl.c_date;

	ITITERATE(i, itwalk){
		recout(df, itwalk);
	}
	if (rename(temp, increm) < 0) {
		perror("rename");
		(void) unlink(temp);
		dumpabort();
	}
	(void) chmod(increm, 0644);
	(void) fclose(df);
	msg("level %c dump on %s\n", incno, prdate(spcl.c_date));
}

recout(file, what)
	FILE	*file;
	struct	idates	*what;
{

	fprintf(file, DUMPOUTFMT,
		what->id_name,
		what->id_incno,
		ctime(&(what->id_ddate))
	);
}

int	recno;
int getrecord(df, idatep)
	FILE	*df;
	struct	idates	*idatep;
{
	char		buf[BUFSIZ];

	recno = 0;
	if ( (fgets(buf, BUFSIZ, df)) != buf)
		return(-1);
	recno++;
	if (makeidate(idatep, buf) < 0)
		msg("Unknown intermediate format in %s, line %d\n",
			increm, recno);

#ifdef FDEBUG
	msg("getrecord: %s %c %s\n",
		idatep->id_name, idatep->id_incno, prdate(idatep->id_ddate));
#endif
	return(0);
}

time_t	unctime();

int makeidate(ip, buf)
	struct	idates	*ip;
	char	*buf;
{
	char	un_buf[128];

	sscanf(buf, DUMPINFMT, ip->id_name, &ip->id_incno, un_buf);
	ip->id_ddate = unctime(un_buf);
	if (ip->id_ddate < 0)
		return(-1);
	return(0);
}

/*
 * This is an estimation of the number of TP_BSIZE blocks in the file.
 * It assumes that there are no unallocated blocks; hence
 * the estimate may be high
 */
est(ip)
	struct dinode *ip;
{
	long s;

	esize++;
	/* calc number of TP_BSIZE blocks */
	s = howmany(ip->di_size, TP_BSIZE);
	if (ip->di_size > sblock->fs_bsize * NDADDR) {
		/* calc number of indirect blocks on the dump tape */
		s += howmany(s - NDADDR * sblock->fs_bsize / TP_BSIZE,
			TP_NINDIR);
	}
	esize += s;
}

/* Calculate # of tape blocks based on supplied bit map */
/* Adjust `esize' up to # of blocks used by map		*/
bmapest(map)
	char *map;
{
	register i, n;

	n = -1;
	for (i = 0; i < msiz; i++)
		if(map[i])
			n = i; /* set to highest index w/a bit set */
	if(n < 0)
		return;
	n++;
	esize++;
	esize += howmany(n * sizeof map[0], TP_BSIZE);
}
