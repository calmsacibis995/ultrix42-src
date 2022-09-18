/* vhangup.c 4.3 82/12/29 */

#include "SYS.h"

#ifndef SYS_vhangup
#define SYS_vhangup 76
#endif SYS_Vhangup

SYSCALL(vhangup)
	ret
