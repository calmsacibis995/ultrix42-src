/* setpgrp.c 4.1 82/12/04 */

#include "SYS.h"

SYSCALL(_setpgrp)
	ret		# _setpgrp(pid, pgrp);
