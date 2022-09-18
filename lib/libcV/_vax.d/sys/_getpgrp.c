/* getpgrp.c 4.1 82/12/04 */

#include "SYS.h"

SYSCALL(_getpgrp)
	ret		# pgrp = getpgrp(pid);
