#ifndef lint
static  char    *sccsid = "@(#)snmp.c	4.1  (ULTRIX)        7/17/90";
#endif lint

#include "defs.h"

#include <errno.h>
#include <syslog.h>

#include "vmdef.h"
#include "diskdef.h"

extern int vm_mib;
extern int disk_mib;
extern int trace;
extern int debug;


procreq(reqoid, reqinst)
objident	*reqoid;
objident	*reqinst;
{
	int  error = 0;

	if (trace|debug) {
		printf("procreq: request: ");
		proid(reqoid);
		if (reqinst->ncmp != 0) {
			printf("         instance: ");
			proid(reqinst);
		}
	}

	switch (whichmib(reqoid->cmp)) {
	case MIB_VM:
		return( ret_vm(reqoid) );
	case MIB_DISK:
		return( ret_disk(reqoid, reqinst) );
	default:
		syslog(LOG_ERR, "procreq: unknown MIB");
		if (debug) {
			printf("procreq: unknown MIB: ");
			proid(reqoid);
		}
		return(1);
	}
}

/*
 * This procedure determines the MIB of the request.
 */
extern u_long Vm_Var[];
extern u_long Disk_Var[];

int whichmib(var)
char *var;
{
	if (bcmp(var, Vm_Var, VM_VAR_SIZE*sizeof(long)) == 0)
		return(MIB_VM);
	else if (bcmp(var, Disk_Var, DISK_VAR_SIZE*sizeof(long)) == 0)
		return(MIB_DISK);
	return(-1);
}

