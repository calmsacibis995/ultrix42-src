#ifndef lint
static  char    *sccsid = "@(#)vm_grp.c	4.1  (ULTRIX)        7/17/90";
#endif lint

/*
 *  This file contains specific utilities encompassing
 *  lookup of MIB variables found in the our "special" group.
 *  The example illustrates a managed object that has no
 *  instances (i.e. instance always 0).  Our special group is
 *  chosen to be sort of something from the vm.
 */

#include "defs.h"

#include <errno.h>
#include <sys/vmmeter.h>
#include <nlist.h>
#include <syslog.h>

#include "vmdef.h"

#define	VMDESCR	"ULTRIX Virtual Memory"

extern	int trace;
extern	int debug;
extern	struct nlist nl[];
extern	char *community;
extern	struct snmpareg construct_oid();

/*
 * My special MIB:  (these info can also be obtained using vmstat utility)
 * iso org(3) dod(6) internet(1) private(4) enterprise (1) YOU(15) me(1) ..
 *
 *			vm(1) ..
 *	INT		vmTotalVM(1)		1.3.6.1.4.1.15.1.1.1
 *	STR		vmDescr(2)		1.3.6.1.4.1.15.1.1.2
 *	OBJ		vmMyObjectID(3)			"
 *	IPADDR		vmMyIPAddr(5)
 *	CNTR		vmAddrXLateFault(6)
 *	GAUGE		vmFreeMemPages(7)
 *	TIME		vmCurrentTime(8)
 */
unsigned long Vm_Var[] = {		/* vm */
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x01
};
unsigned long Vm_TotalVM[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x01, 0x01
};
unsigned long Vm_Descr[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x01, 0x02
};
unsigned long Vm_MyObjectID[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x01, 0x03
};
unsigned long Vm_MyIPAddr[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x01, 0x05
};
unsigned long Vm_AddrXLateFault[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x01, 0x06
};
unsigned long Vm_FreeMemPages[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x01, 0x07
};
unsigned long Vm_CurrentTime[] = {
	0x01, 0x03, 0x06, 0x01, 0x04, 0x01, 0x0f, 0x01, 0x01, 0x08
};

/*
 * This routine returns a vm mib variable to snmpd.
 */
ret_vm(reqoid)
objident	*reqoid;
{
	struct	snmparspdat rsp;
	objident oid;
	int	attrtag, error;
	u_long	*p;
	u_long	reqmet;			/* request met */
	char	reqmetstr[SNMPSTRLEN];	/* request met */

	p = reqoid->cmp;
	p += VM_VAR_SIZE;		/* Point to attr. tag */
	attrtag = *p++;
	bzero(reqmetstr, sizeof(reqmetstr));

	switch (attrtag) {
	case VM_TOTALVM:
		if (get_vmstat(attrtag, &reqmet) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) printf("MIB: vm_TotalVM: %lu\n", reqmet);
		rsp.type = INT;
		rsp.octets = sizeof(reqmet);
		rsp.rspdat = (char *)malloc(sizeof(reqmet));
		*(u_long *)rsp.rspdat = reqmet;
		if (snmpextrespond(reqoid, NULL, &rsp) != GENSUC) {
			syslog(LOG_ERR,"vm_grp: respond failed");
			if (debug) printf("vm_grp: respond failed\n");
			error = GENERRS;
			break;
		}
		free(rsp.rspdat);
		return(0);
	case VM_DESCR:
		if (get_vmstat(attrtag, reqmetstr) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) printf("MIB: vm_Descr: %s\n", reqmetstr);
		rsp.type = STR;
		rsp.octets = strlen(reqmetstr);
		rsp.rspdat = (char *)malloc(strlen(reqmetstr));
		bcopy(reqmetstr, rsp.rspdat, strlen(reqmetstr)+1);   /* '\0' */
		if (snmpextrespond(reqoid, NULL, &rsp) != GENSUC) {
			syslog(LOG_ERR,"vm_grp: respond failed");
			if (debug) printf("vm_grp: respond failed\n");
			error = BADVAL;
			break;
		}
		free(rsp.rspdat);
		return(0);
	case VM_MYOBJECTID:
		if (get_vmstat(attrtag, &oid) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) {
			printf("MIB: vm_MyObjectID: ");
			proid(&oid);
		}
		rsp.type = OBJ;
		rsp.octets = oid.ncmp*sizeof(u_long);
		rsp.rspdat = (char *)malloc(strlen(reqmetstr));
		bcopy(&oid, rsp.rspdat, oid.ncmp*sizeof(u_long));
		if (snmpextrespond(reqoid, NULL, &rsp) != GENSUC) {
			syslog(LOG_ERR,"vm_grp: respond failed");
			if (debug) printf("vm_grp: respond failed\n");
			error = GENERRS;
			break;
		}
		free(rsp.rspdat);
		return(0);
	case VM_MYIPADDR:
		if (get_vmstat(attrtag, &reqmet) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) printf("MIB: vm_MyIPAddr: %s\n",
					 inet_ntoa(reqmet));
		rsp.type = IPADD;
		rsp.octets = sizeof(struct in_addr);
		rsp.rspdat = (char *)malloc(sizeof(struct in_addr));
		bcopy(&reqmet, rsp.rspdat, sizeof(struct in_addr));
		if (snmpextrespond(reqoid, NULL, &rsp) != GENSUC) {
			syslog(LOG_ERR,"vm_grp: respond failed");
			if (debug) printf("vm_grp: respond failed\n");
			error = GENERRS;
			break;
		}
		free(rsp.rspdat);
		return(0);
	case VM_ADDRXLATEFAULT:
		if (get_vmstat(attrtag, &reqmet) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) printf("MIB: vm_AddrXLateFault: %lu\n",
					 reqmet);
		rsp.type = CNTR;
		rsp.octets = sizeof(int);
		rsp.rspdat = (char *)malloc(sizeof(reqmet));
		*(u_long *)rsp.rspdat = reqmet;
		if (snmpextrespond(reqoid, NULL, &rsp) != GENSUC) {
			syslog(LOG_ERR,"vm_grp: respond failed");
			if (debug) printf("vm_grp: respond failed\n");
			error = GENERRS;
		}
		free(rsp.rspdat);
		return(0);
	case VM_FREEMEMPAGES:
		if (get_vmstat(attrtag, &reqmet) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) printf("MIB: vm_FreeMemPages: %lu\n", reqmet);
		rsp.type = GAUGE;
		rsp.octets = sizeof(int);
		rsp.rspdat = (char *)malloc(sizeof(reqmet));
		*(u_long *)rsp.rspdat = reqmet;
		if (snmpextrespond(reqoid, NULL, &rsp) != GENSUC) {
			syslog(LOG_ERR,"vm_grp: respond failed");
			if (debug) printf("vm_grp: respond failed\n");
			error = GENERRS;
			break;
		}
		free(rsp.rspdat);
		return(0);
	case VM_CURRENTTIME:
		if (get_vmstat(attrtag, &reqmet) != 0) {
			error = BADVAL;
			break;
		}
		if (trace|debug) printf("MIB: vm_FreeMemPages: %lu\n", reqmet);
		rsp.type = TIME;
		rsp.octets = sizeof(int);
		rsp.rspdat = (char *)malloc(sizeof(reqmet));
		*(u_long *)rsp.rspdat = reqmet;
		if (snmpextrespond(reqoid, NULL, &rsp) != GENSUC) {
			syslog(LOG_ERR,"vm_grp: respond failed");
			if (debug) printf("vm_grp: respond failed\n");
			error = GENERRS;
			break;
		}
		free(rsp.rspdat);
		return(0);
	default:
		/*
		 * Log the error and respond to snmpd with an error.
		 */
		syslog(LOG_ERR, "vm_grp: invalid MIB reqest attribute");
		if (debug) printf("vm_grp: invalid MIB req attr (tag=%d)\n",
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

/*
 * Return the requested variable.
 */
get_vmstat(attr, var)
int attr;
char *var;
{
	struct	vmmeter sum;
	struct	vmtotal total;
	struct	hostent *hp;
	int	mf;
	time_t	t;
	char	hostname[80];

	mf = open("/dev/kmem", 0);
	if (mf < 0) {
		syslog(LOG_ERR, "get_vmstat: can't open /dev/kmem");
		if (debug) printf("get_vmstat: can't open /dev/kmem\n");
		return(-1);
	}
	if (lseek(mf, (long)nl[X_SUM].n_value, 0) == -1) {
		syslog(LOG_ERR, "get_vmstat: lseek: %m");
		if (debug) perror("get_vmstat: lseek");
		return(-1);
	}
	if (read(mf, &sum, sizeof(sum)) <= 0) {
		syslog(LOG_ERR, "get_vmstat: read: %m");
		if (debug) perror("get_vmstat: read");
		return(-1);
	}
	if (lseek(mf, (long)nl[X_TOTAL].n_value, 0) == -1) {
		syslog(LOG_ERR, "get_vmstat: lseek: %m");
		if (debug) perror("get_vmstat: lseek");
		return(-1);
	}
	if (read(mf, &total, sizeof(total)) <= 0) {
		syslog(LOG_ERR, "get_vmstat: read: %m");
		if (debug) perror("get_vmstat: read");
		return(-1);
	}
	(void) close(mf);

	switch (attr) {
	case VM_TOTALVM:
		bcopy(&total.t_vm, var, sizeof(int));
		break;
	case VM_DESCR:
		bcopy(VMDESCR, var, strlen(VMDESCR));
		break;
	case VM_MYOBJECTID:
		*(u_long *)var = VM_SIZE;
		bcopy(Vm_MyObjectID, var+4, VM_SIZE*sizeof(u_long));
		break;
	case VM_MYIPADDR:
		(void)gethostname(hostname, 80);
		if ((hp = gethostbyname(hostname)) == NULL) {
			if (debug) printf("get_vmstat: can't get host\n");
			return(-1);
		}
		bcopy(hp->h_addr, var, hp->h_length);
		break;
	case VM_ADDRXLATEFAULT:
		bcopy(&sum.v_faults, var, sizeof(long)); 
		break;
	case VM_FREEMEMPAGES:
		bcopy(&total.t_free, var, sizeof(long));
		break;
	case VM_CURRENTTIME:
		if ((t = time(0)) < 0) {
			if (debug) printf("get_vmstat: can't get time\n");
			return(-1);
		}
		bcopy(&t, var, sizeof(int));
		break;
	}
	return(0);
}

/*
 * This routine registers the vm group to the snmpd.
 */
register_vm()
{
	register int	e = -1;
	if (reg_oid(Vm_TotalVM, VM_SIZE, OIDTYP_NULLINST) == e) return(e);
	if (reg_oid(Vm_Descr, VM_SIZE, OIDTYP_NULLINST) == e) return(e);
	if (reg_oid(Vm_MyObjectID, VM_SIZE, OIDTYP_NULLINST) == e) return(e);
	if (reg_oid(Vm_MyIPAddr, VM_SIZE, OIDTYP_NULLINST) == e) return(e);
	if (reg_oid(Vm_AddrXLateFault,VM_SIZE,OIDTYP_NULLINST) == e) return(e);
	if (reg_oid(Vm_FreeMemPages, VM_SIZE, OIDTYP_NULLINST) == e) return(e);
	if (reg_oid(Vm_CurrentTime, VM_SIZE, OIDTYP_NULLINST) == e) return(e);
	return(0);
}

