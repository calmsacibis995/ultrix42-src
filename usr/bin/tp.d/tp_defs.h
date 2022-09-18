/*	@(#)tp_defs.h	4.1	12/18/82	*/
#include <sys/types.h>
#include <sys/ioctl.h>

#ifdef DEBUG
#include "mtio.h"
#include "devio.h"
#define DEVIOCGET 0
#else
#include <sys/mtio.h>
#include <sys/devio.h>
#endif
#include <sys/file.h>

char	mt[]	= DEFTAPE_RH;
char	tc[]	= "/dev/tapx";
int	flags	= flu;
char	mheader[] = "/usr/mdec/mboot";
char	theader[] = "/usr/mdec/tboot";
