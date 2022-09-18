#ifndef lint
static  char    *sccsid = "@(#)disk_grp.c	4.1  (ULTRIX)        7/17/90";
#endif lint

/*
 *  This file contains specific utilities encompassing
 *  lookup of MIB variables found in the our "special" group.
 *  The example illustrates a managed object that has instances
 *  Our special group is chosen to be sort of something from the "df"
 *  utility.
 */

#include "defs.h"

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>

#include "diskdef.h"

#define	GETMNTTIM	2		/* getmnt() timer */

extern	int notdaemon;
extern	int trace;
extern	int debug;
extern	int getmntcache;

struct	fs_data	*mntbuf;
#define	MSIZE	(NMOUNT*sizeof(struct fs_data))
jmp_buf	env;

/*
 * My special MIB:  (these info can also be obtained using df utility)
 * iso org(3) dod(6) internet(1) private(4) enterprise (1) YOU(15) me(1) ..
 *
 *			disk(2) ..
 *	INT		diskIndex(1)		1.3.6.1.4.1.15.1.2.1
 *	STR		diskDevDescr(2)		1.3.6.1.4.1.15.1.2.2
 *	STR		diskMountDescr(3)			"
 *	GAGUE		diskTotalKbytes(4)
 *	GAGUE		diskUsedKBytes(5)
 */
unsigned long Disk_Var[] = {		/* df */
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x02
};
unsigned long Disk_Index[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x02, 0x01
};
unsigned long Disk_DevDescr[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x02, 0x02
};
unsigned long Disk_MountDescr[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x02, 0x03
};
unsigned long Disk_TotalKbytes[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x02, 0x04
};
unsigned long Disk_UsedKbytes[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x02, 0x05
};

/*
 * This routine returns a disk mib variable to snmpd.
 */
ret_disk(reqoid, reqinst)
objident	*reqoid;
objident	*reqinst;
{
	struct	snmparspdat rsp;
	objident rspinst;
	int	attrtag;
	long	error;
	u_long	*p;
	u_long	reqmet;			/* request met */
	char	reqmetstr[SNMPSTRLEN];	/* request met */

	p = reqoid->cmp;
	p += DISK_VAR_SIZE;		/* Point to attr. tag */
	attrtag = *p++;
	bzero(reqmetstr, sizeof(reqmetstr));
	rsp.rspdat = NULL;

	switch (attrtag) {
	case DISK_INDEX:
		if (get_diskstat(attrtag, reqinst, &reqmet, &rspinst) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) printf("MIB: diskIndex: %lu  inst: %lu\n",
				   	 reqmet, rspinst.cmp[0]);
		if (endgetnxt(&rspinst) == 0) return(0);
		rsp.type = INT;
		rsp.octets = sizeof(reqmet);
		rsp.rspdat = (char *)malloc(sizeof(reqmet));
		*(u_long *)rsp.rspdat = reqmet;
		if (snmpextrespond(reqoid, &rspinst, &rsp) != GENSUC) {
			syslog(LOG_ERR,"disk_grp: respond failed");
			if (debug) printf("disk_grp: respond failed\n");
			error = GENERRS;
			break;
		}
		free(rsp.rspdat);
		return(0);
	case DISK_DEVDESCR:
		if (get_diskstat(attrtag, reqinst, reqmetstr, &rspinst) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) printf("MIB: diskDevDescr: %s  inst: %lu\n",
				   reqmetstr, rspinst.cmp[0]);
		if (endgetnxt(&rspinst) == 0) return(0);
		rsp.type = STR;
		rsp.octets = strlen(reqmetstr);
		rsp.rspdat = (char *)malloc(strlen(reqmetstr));
		bcopy(reqmetstr, rsp.rspdat, strlen(reqmetstr)+1);   /* '\0' */
		if (snmpextrespond(reqoid, &rspinst, &rsp) != GENSUC) {
			syslog(LOG_ERR,"disk_grp: respond failed");
			if (debug) printf("disk_grp: respond failed\n");
			error = GENERRS;
			break;
		}
		free(rsp.rspdat);
		return(0);
	case DISK_MOUNTDESCR:
		if (get_diskstat(attrtag, reqinst, reqmetstr, &rspinst) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) printf("MIB: diskMountDescr: %s  inst: %lu\n",
				   reqmetstr, rspinst.cmp[0]);
		if (endgetnxt(&rspinst) == 0) return(0);
		rsp.type = STR;
		rsp.octets = strlen(reqmetstr);
		rsp.rspdat = (char *)malloc(strlen(reqmetstr));
		bcopy(reqmetstr, rsp.rspdat, strlen(reqmetstr)+1);   /* '\0' */
		if (snmpextrespond(reqoid, &rspinst, &rsp) != GENSUC) {
			syslog(LOG_ERR,"disk_grp: respond failed");
			if (debug) printf("disk_grp: respond failed\n");
			error = GENERRS;
			break;
		}
		free(rsp.rspdat);
		return(0);
	case DISK_TOTALKBYTES:
		if (get_diskstat(attrtag, reqinst, &reqmet, &rspinst) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) printf("MIB: diskTotalKbytes: %u  inst: %lu\n",
				   	 reqmet, rspinst.cmp[0]);
		if (endgetnxt(&rspinst) == 0) return(0);
		rsp.type = CNTR;
		rsp.octets = sizeof(reqmet);
		rsp.rspdat = (char *)malloc(sizeof(reqmet));
		*(u_long *)rsp.rspdat = reqmet;
		if (snmpextrespond(reqoid, &rspinst, &rsp) != GENSUC) {
			syslog(LOG_ERR,"disk_grp: respond failed");
			if (debug) printf("disk_grp: respond failed\n");
			error = GENERRS;
			break;
		}
		free(rsp.rspdat);
		return(0);
	case DISK_USEDKBYTES:
		if (get_diskstat(attrtag, reqinst, &reqmet, &rspinst) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) printf("MIB: diskUsedKbytes: %u  inst: %lu\n",
				   	 reqmet, rspinst.cmp[0]);
		if (endgetnxt(&rspinst) == 0) return(0);
		rsp.type = CNTR;
		rsp.octets = sizeof(reqmet);
		rsp.rspdat = (char *)malloc(sizeof(reqmet));
		*(u_long *)rsp.rspdat = reqmet;
		if (snmpextrespond(reqoid, &rspinst, &rsp) != GENSUC) {
			syslog(LOG_ERR,"disk_grp: respond failed");
			if (debug) printf("disk_grp: respond failed\n");
			error = GENERRS;
			break;
		}
		free(rsp.rspdat);
		return(0);
	default:
		/*
		 * Log the error and respond to snmpd with an error.
		 */
		syslog(LOG_ERR, "disk_grp: invalid MIB reqest attribute");
		if (debug) printf("disk_grp: invalid MIB req attr (tag=%d)\n",
				   attrtag);
		(void) snmpexterror(NOSUCH);
		return(-1);
	} /* switch */
	/*
	 * Got here if failure encountered during the case processing.
	 */
	(void) snmpexterror(error);
	free(rsp.rspdat);
	return(-1);
}

getmnt_tmo()
{
	longjmp(env, ETIMEDOUT);
}

/*
 * Return the requested variable.
 */
get_diskstat(attr, reqinst, var, retinst)
int attr;
objident *reqinst;
char *var;
objident *retinst;
{
	struct	fs_data *fsd;
	struct	itimerval interval, disable;
	int	mode = NOSTAT_MANY;
	int	ndata;
	int	loc = 0;
	int	fs_data_index = 1;	/* SMI counts index from 1 */

	bzero(&interval, sizeof(struct itimerval));
	bzero(&disable, sizeof(struct itimerval));

	mntbuf = (struct fs_data *) malloc(MSIZE);
	if (mntbuf == NULL) {
		syslog(LOG_ERR,"get_diskstat: alloc mount buffer failed\n");
		if (debug) printf("get_diskstat: alloc mount buffer failed\n");
		return(-1);
	}
	/*
	 * getmnt() must complete within the specified number of seconds.
	 */
	if (setjmp(env) == ETIMEDOUT) {
		syslog(LOG_ERR,"get_diskstat: getmnt timeout");
		if (debug|notdaemon) printf("get_diskstat: getmnt timeout\n");
		free(mntbuf);
		return(-1);
	}	
	(void) signal(SIGALRM, getmnt_tmo);
	interval.it_value.tv_sec = GETMNTTIM;
	(void) setitimer(ITIMER_REAL, &interval, (struct itimerval *)NULL);
	if (getmntcache) mode = STAT_MANY;
	ndata = getmnt(&loc, mntbuf, MSIZE, mode, 0);
	if (ndata < 0 ) {
		syslog(LOG_ERR, "get_diskstat: getmnt: %m");
		if (debug) perror("get_diskstat: getmnt");
		return(-1);
	}
	(void) setitimer(ITIMER_REAL, &disable, (struct itimerval *)NULL);
	/*
	 * Get the data like that of "df" utility.
	 */
	for (fsd = mntbuf; fsd < &mntbuf[ndata]; fsd++) {
		if (reqinst->cmp[0]+1 == fs_data_index ||
		    reqinst->ncmp == 0) {
			switch (attr) {
			case DISK_INDEX:
				*(u_long *)var = fs_data_index;
				retinst->ncmp = 1;
				retinst->cmp[0] = fs_data_index;
				free(mntbuf);
				return(0);
			case DISK_DEVDESCR:
				bcopy(fsd->fd_devname, var,
				      strlen(fsd->fd_devname));
				retinst->ncmp = 1;
				retinst->cmp[0] = fs_data_index;
				free(mntbuf);
				return(0);
			case DISK_MOUNTDESCR:
				bcopy(fsd->fd_path, var,
				      strlen(fsd->fd_devname));
				retinst->ncmp = 1;
				retinst->cmp[0] = fs_data_index;
				free(mntbuf);
				return(0);
			case DISK_TOTALKBYTES:
				*(u_int *)var = fsd->fd_btot;
				retinst->ncmp = 1;
				retinst->cmp[0] = fs_data_index;
				free(mntbuf);
				return(0);
			case DISK_USEDKBYTES:
				*(u_int *)var = fsd->fd_bfree;
				retinst->ncmp = 1;
				retinst->cmp[0] = fs_data_index;
				free(mntbuf);
				return(0);
			} /* switch */
		}
		fs_data_index++;
	}
	/*
	 * Can't find the given instance
	 */
	*(u_long *)var = 0;
	retinst->ncmp = 0;
	retinst->cmp[0] = 0;
	free(mntbuf);
	return(0);
}

/*
 * This routine registers the disk group to snmpd.
 */
register_disk()
{
	register int	e = -1;

	if (reg_oid(Disk_Index, DISK_SIZE, OIDTYP_INST) == e) return(e);
	if (reg_oid(Disk_DevDescr, DISK_SIZE, OIDTYP_INST) == e) return(e);
	if (reg_oid(Disk_MountDescr, DISK_SIZE, OIDTYP_INST) == e) return(e);
	if (reg_oid(Disk_TotalKbytes, DISK_SIZE, OIDTYP_INST) == e) return(e);
	if (reg_oid(Disk_UsedKbytes, DISK_SIZE, OIDTYP_INST) == e) return(e);
	return(0);
}

