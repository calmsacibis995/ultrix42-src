#ifndef ultrix
#include "machine/reg.h"
#include "machine/pte.h"
#include "machine/psl.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/socketvar.h"
#include "../h/vnode.h"
#include "../h/pathname.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/acct.h"
#include "../h/exec.h"
#include "../h/vfs.h"
#include "../h/conf.h"
#include "../mipsvme/vmevar.h"
#include "../fs/ufs/inode.h"
#include "../h/ioctl.h"
#include "../h/tty.h"

/* the entries here need to match the constants in ps_data.h */

int ps_data[] = {
	/* proc */		sizeof(struct proc),
	/* pte */		sizeof(struct pte),
	/* text */		sizeof(struct text),
	/* user */		sizeof(struct user),
	/* user.u_comm */	sizeof(up->u_comm),
	/* user.u_arg */	sizeof(up->u_arg),
	/* inode */		sizeof(struct inode),
	/* tty */		sizeof(struct tty),
	/* ucred */		sizeof(struct ucred),
	/* rusage */		sizeof(struct rusage),
	/* file */		sizeof(struct file),
	/* ucred */		sizeof(struct ucred),
	/* rusage */		sizeof(struct rusage),
	/* map */		sizeof(struct map),
	/* swdevt */		sizeof(struct swdevt),
	/* vme_device */	sizeof(struct vme_device),
	/* vme_driver */	sizeof(struct vme_driver),
	1
};
#endif not ultrix
