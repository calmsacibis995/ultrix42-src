/* local.h - fine the -lndir include file */

#ifndef	BSD42
#include <sys/types.h>
#else	BSD42
#include <sys/param.h>
#endif	BSD42

#ifndef	BSD42
#ifndef NDIR
#include <dir.h>
#else	NDIR
#include <ndir.h>
#endif  NDIR
#else	BSD42
#include <sys/dir.h>
#endif	BSD42

#include <sys/stat.h>
