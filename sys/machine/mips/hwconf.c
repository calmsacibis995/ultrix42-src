/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

#include "../h/param.h"
#include "../h/user.h"
#include "../machine/hwconf.h"

struct hw_config hwconf;

int
mipshwconf(option, info)
	int	option;
	struct	hw_config *info;
{
	char *prom_getenv();
	int prom_setenv();
	register int i;
	struct hw_config tmp;
	int error = 0;

	switch (option) {

	case HWCONF_GET:
		for (i=0; i < ENV_ENTRIES; i++) {
			strncpy(hwconf.promenv[i].value, 
				prom_getenv(hwconf.promenv[i].name), 
				ENV_MAXLEN);
		}
		error = copyout((caddr_t)&hwconf, (caddr_t)info, 
				sizeof(struct hw_config));
		break;

	case HWCONF_SET:
		if (!suser()) {
			error = EACCES;
			return(error);
		}
		error = copyin((caddr_t)info, (caddr_t)&tmp,
				sizeof(struct hw_config));
		if (error)
			return(error);
		for (i=0; i < ENV_ENTRIES; i++) {
		    if (!strcmp(tmp.promenv[i].name, hwconf.promenv[i].name))
			prom_setenv(tmp.promenv[i].name, tmp.promenv[i].value);
		}
		break;

	default:
		return(EINVAL);
	}
	return(error);
}

hwconf_init()
{
	strcpy(hwconf.promenv[0].name, "netaddr");
	strcpy(hwconf.promenv[1].name, "lbaud");
	strcpy(hwconf.promenv[2].name, "rbaud");
	strcpy(hwconf.promenv[3].name, "bootfile");
	strcpy(hwconf.promenv[4].name, "bootmode");
	strcpy(hwconf.promenv[5].name, "console");
}
