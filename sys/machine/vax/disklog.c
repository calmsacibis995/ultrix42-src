#ifdef	DISKLOGGING
#include	"../h/time.h"
#include	"../vax/mfpr.h"

extern	struct	timeval	time;
#define	MAXLOG 1000
struct dklog {
	struct	timeval	tim;
	int	residue;
	int	blkno;
	short	sz;
	short	unit;
} dklog[MAXLOG], *cur_dklog;

disklog(bk,sz,unit)
{
	cur_dklog->blkno = bk;
	cur_dklog->sz = sz;
	cur_dklog->unit = unit;
	cur_dklog->tim = time;
	cur_dklog->residue = mfpr(ICR);
	if(cur_dklog == &dklog[MAXLOG-1] ) {
		cur_dklog = dklog;
	} else cur_dklog++;
}
#endif
