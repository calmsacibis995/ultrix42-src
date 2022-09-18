
#ifndef lint
static	char	*sccsid = "@(#)disktab.c	4.1	ULTRIX	7/3/90";
#endif lint

/**************************************************************************/
/*									  */
/*			MODIFICATION HISTORY				  */
/*									  */
/*	02	Tim Burke	Aug-30-1989				  */
/*		Added the creatediskbyname routine to dynamically create  */
/*		a disktab entry for a given disk by asking the device	  */
/*		driver for the parameters and defaulting on other params. */
/*									  */
/*	01	Bob Fontaine						  */
/*		Changed declaration of cp from static char to char. 	  */
/*		Fixes SMU-1883.						  */
/**************************************************************************/

#include <disktab.h>
#include <stdio.h>
#include <sys/devio.h>
#include <sys/file.h>
#include <sys/param.h>
#include <ufs/fs.h>
#include <sys/ioctl.h>

/*
 * Default values to assign to fields of the disktab structure when using the
 * creatediskbyname call.
 */
#define DEF_TYPE_WIN 		"winchester"
#define DEF_TYPE_REMOVABLE 	"removable"
#define DEF_SE			512 		
#define DEF_RM			3600 		
#define DEF_B_BLK_SIZE  	4096
#define DEF_BLK_SIZE    	8192
#define DEF_FRAG_SIZE   	1024
#define DEF_INV_SIZE    	-1

static	char *dgetstr();

struct disktab *
getdiskbyname(name)
	char *name;
{
	static struct disktab disk;
	static char localbuf[100];
	register struct	disktab *dp = &disk;
	register struct partition *pp;
	char p, psize[3], pbsize[3], pfsize[3];
	char buf[BUFSIZ], *cp = localbuf;

	if (dgetent(buf, name) <= 0)
		return ((struct disktab *)0);
	dp->d_name = cp;
	strcpy(cp, name);
	cp += strlen(name) + 1;
	dp->d_type = dgetstr("ty", &cp);
	dp->d_secsize = dgetnum("se");
	if (dp->d_secsize < 0)
		dp->d_secsize = 512;
	dp->d_ntracks = dgetnum("nt");
	dp->d_nsectors = dgetnum("ns");
	dp->d_ncylinders = dgetnum("nc");
	dp->d_rpm = dgetnum("rm");
	if (dp->d_rpm < 0)
		dp->d_rpm = 3600;
	strcpy(psize, "px");
	strcpy(pbsize, "bx");
	strcpy(pfsize, "fx");
	for (p = 'a'; p < 'i'; p++) {
		psize[1] = pbsize[1] = pfsize[1] = p;
		pp = &dp->d_partitions[p - 'a'];
		pp->p_size = dgetnum(psize);
		pp->p_bsize = dgetnum(pbsize);
		pp->p_fsize = dgetnum(pfsize);
	}
	return (dp);
}

#include <ctype.h>

static	char *tbuf;
static	char *dskip();
static	char *ddecode();

/*
 * Get an entry for disk name in buffer bp,
 * from the diskcap file.  Parse is very rudimentary;
 * we just notice escaped newlines.
 */
static
dgetent(bp, name)
	char *bp, *name;
{
	register char *cp;
	register int c;
	register int i = 0, cnt = 0;
	char ibuf[BUFSIZ];
	int tf;

	tbuf = bp;
	tf = open(DISKTAB, 0);
	if (tf < 0)
		return (-1);
	for (;;) {
		cp = bp;
		for (;;) {
			if (i == cnt) {
				cnt = read(tf, ibuf, BUFSIZ);
				if (cnt <= 0) {
					close(tf);
					return (0);
				}
				i = 0;
			}
			c = ibuf[i++];
			if (c == '\n') {
				if (cp > bp && cp[-1] == '\\'){
					cp--;
					continue;
				}
				break;
			}
			if (cp >= bp+BUFSIZ) {
				write(2,"Disktab entry too long\n", 23);
				break;
			} else
				*cp++ = c;
		}
		*cp = 0;

		/*
		 * The real work for the match.
		 */
		if (dnamatch(name)) {
			close(tf);
			return (1);
		}
	}
}

/*
 * Dnamatch deals with name matching.  The first field of the disktab
 * entry is a sequence of names separated by |'s, so we compare
 * against each such name.  The normal : terminator after the last
 * name (before the first field) stops us.
 */
static
dnamatch(np)
	char *np;
{
	register char *Np, *Bp;

	Bp = tbuf;
	if (*Bp == '#')
		return (0);
	for (;;) {
		for (Np = np; *Np && *Bp == *Np; Bp++, Np++)
			continue;
		if (*Np == 0 && (*Bp == '|' || *Bp == ':' || *Bp == 0))
			return (1);
		while (*Bp && *Bp != ':' && *Bp != '|')
			Bp++;
		if (*Bp == 0 || *Bp == ':')
			return (0);
		Bp++;
	}
}

/*
 * Skip to the next field.  Notice that this is very dumb, not
 * knowing about \: escapes or any such.  If necessary, :'s can be put
 * into the diskcap file in octal.
 */
static char *
dskip(bp)
	register char *bp;
{

	while (*bp && *bp != ':')
		bp++;
	if (*bp == ':')
		bp++;
	return (bp);
}

/*
 * Return the (numeric) option id.
 * Numeric options look like
 *	li#80
 * i.e. the option string is separated from the numeric value by
 * a # character.  If the option is not found we return -1.
 * Note that we handle octal numbers beginning with 0.
 */
static
dgetnum(id)
	char *id;
{
	register int i, base;
	register char *bp = tbuf;

	for (;;) {
		bp = dskip(bp);
		if (*bp == 0)
			return (-1);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return (-1);
		if (*bp != '#')
			continue;
		bp++;
		base = 10;
		if (*bp == '0')
			base = 8;
		i = 0;
		while (isdigit(*bp))
			i *= base, i += *bp++ - '0';
		return (i);
	}
}

/*
 * Handle a flag option.
 * Flag options are given "naked", i.e. followed by a : or the end
 * of the buffer.  Return 1 if we find the option, or 0 if it is
 * not given.
 */
static
dgetflag(id)
	char *id;
{
	register char *bp = tbuf;

	for (;;) {
		bp = dskip(bp);
		if (!*bp)
			return (0);
		if (*bp++ == id[0] && *bp != 0 && *bp++ == id[1]) {
			if (!*bp || *bp == ':')
				return (1);
			else if (*bp == '@')
				return (0);
		}
	}
}

/*
 * Get a string valued option.
 * These are given as
 *	cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * No checking on area overflow.
 */
static char *
dgetstr(id, area)
	char *id, **area;
{
	register char *bp = tbuf;

	for (;;) {
		bp = dskip(bp);
		if (!*bp)
			return (0);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return (0);
		if (*bp != '=')
			continue;
		bp++;
		return (ddecode(bp, area));
	}
}

/*
 * Tdecode does the grung work to decode the
 * string capability escapes.
 */
static char *
ddecode(str, area)
	register char *str;
	char **area;
{
	register char *cp;
	register int c;
	register char *dp;
	int i;

	cp = *area;
	while ((c = *str++) && c != ':') {
		switch (c) {

		case '^':
			c = *str++ & 037;
			break;

		case '\\':
			dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
			c = *str++;
nextc:
			if (*dp++ == c) {
				c = *dp++;
				break;
			}
			dp++;
			if (*dp)
				goto nextc;
			if (isdigit(c)) {
				c -= '0', i = 2;
				do
					c <<= 3, c |= *str++ - '0';
				while (--i && isdigit(*str));
			}
			break;
		}
		*cp++ = c;
	}
	*cp++ = 0;
	str = *area;
	*area = cp;
	return (str);
}

/*
 * This routine is creates a disktab type entry by obtaining as much 
 * information from the disk as possible.  The intent is that programs like
 * newfs will first search the disktab file for an entry.  If there is no
 * entry corresponding to this disk then try to construct one.
 */
struct disktab *
creatediskbyname(file_name)
	char *file_name;		/* Device special file, ie rra0a */
{
	static struct disktab disk;
	struct pt pt_struct;
	register struct	disktab *dp = &disk;
	register struct partition *pp;
	int fdes;
	DEVGEOMST devgeom;
	struct devget devget_st;
	char p;

	/*
	 * If there is no disktab file at all then do not try to fabricate
	 * an entry.  The disktab file may have been lost and valid entries
	 * could possibly exist for this name.
	 */
	fdes = open(DISKTAB, 0);
	if (fdes < 0)
		return ((struct disktab *)0);
	close(fdes);
	/*
	 * Assign default disk sector size.
	 */
	dp->d_secsize = DEF_SE;

	fdes = open(file_name, O_RDONLY);
	if(fdes < 0) {
		return ((struct disktab *)0);
	}
	/*
	 * Call DEVIOCGET to obtain the disk name; ie RA90.
	 */
	if (ioctl(fdes, DEVIOCGET, (char *)&devget_st) < 0) {
		dp->d_name = "UNKNOWN";
	}
	else {
		dp->d_name =  devget_st.device;
	}
	/*
	 * Call DEVGETGEOM to get device geometry information.
	 */
	if (ioctl(fdes, DEVGETGEOM, (char *)&devgeom) < 0) {
		return ((struct disktab *)0);
	}
	if (devgeom.geom_info.attributes & DEVGEOM_REMOVE) {
		dp->d_type = DEF_TYPE_REMOVABLE;
	}
	else {
		dp->d_type = DEF_TYPE_WIN;
	}
	dp->d_ntracks = devgeom.geom_info.ntracks;
	dp->d_nsectors = devgeom.geom_info.nsectors;
	dp->d_ncylinders = devgeom.geom_info.ncylinders; 
	dp->d_rpm = DEF_RM;
	/*
	 * Obtain partition information from the drive.
	 * The DIOCDGTPT will get the driver's default partition table
	 * values for the media.
	 */
	if (ioctl(fdes, DIOCDGTPT, (char *)&pt_struct) < 0) {
		return ((struct disktab *)0);
	}
	for (p = 'a'; p < 'i'; p++) {
		pp = &dp->d_partitions[p - 'a'];
		if (pt_struct.pt_part[p -'a'].pi_nblocks) {
			pp->p_size = pt_struct.pt_part[p -'a'].pi_nblocks;
			if (p == 'b')
				pp->p_bsize = DEF_B_BLK_SIZE;
			else
				pp->p_bsize = DEF_BLK_SIZE;
			pp->p_fsize = DEF_FRAG_SIZE;
		}
		else {
			pp->p_size =  DEF_INV_SIZE;
			pp->p_bsize = DEF_INV_SIZE;
			pp->p_fsize = DEF_INV_SIZE;
		}
	}
	return (dp);
}
