/* @(#)dmap.h	4.1  (ULTRIX)        7/2/90     */

/*
 *
 *	Modification History
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 *  04-Dec-89	sekhar
 *      changes to track swap useage and wastage.
 *	The swap useage is now maintained in the kernel using 
 * 	the data structures and the macros defined here.
 *	swapu is the data structure used to hold information
 *	about swap useage and wastage. 
 *
 * 	Macros modified to track swap useage by segment type:
 *	ALLOC_SWAP and FREE_SWAP 
 *      New macros added to track swap wastage: 
 *	ALLOC_WASTED, FREE_WASTED, SWAPSHR_WASTAGE, SWAPEXP_WASTAGE
 *
 *
 *  12-Jun-89   gg
 *	changed the dmap structure completely for dynamic swap.
 *	Added two macros FREE_SWAP and ALLOC_SWAP. Also added
 *	a  structure to get the failure information.
 *
 */

/*
 * Definitions for the mapping of virtual swap
 * space to the physical swap area - the disk map.
 */

struct	dmap
{
	int 	dm_last;	/* last element in dm_map */
	int	dm_cnt;		/* count of elements  with swap */
	int	*dm_ptdaddr;	/* address of disk address of page table */
	swblk_t	dm_map[1];	/* first disk block number in each chunk */
};

/*
 * The following structure is that ``returned''
 * from a call to vstodb().
 */
struct	dblock
{
	swblk_t	db_base;	/* base of physical contig drum block */
	swblk_t	db_size;	/* size of block */
};

/* Swap useage data structure : */

struct swapu_t {
	int txt; 	/* swap space for text */
	int smem; 	/* swap space for shared memory */
	int total_used; /* total amount of swap space used */
	int wasted;	/* amount of swap space wasted */
};

struct swapu_t swapu;

struct swfail_stat {
	int data_ex_fail;
	int stack_ex_fail; 
	int fork_fail;
	int exec_fail;
	int uarea_fail;
	int lowswap_fail;
	int frag_fail;
	int text_dmap_fail;
	int text_swap_fail;
	int shm_dmap_fail;
	int shm_swap_fail;
};

extern int maxtsiz, maxdsiz, maxssiz;
extern int availswap, availvas;
extern int swapfrag;
extern int maxretry;	

#ifdef KERNEL

struct swfail_stat swfail_stat;

struct dmap *dmalloc();
int dmexpand(),  vsalloc(), *vsptalloc();
void vsfree(), vsptfree(), vrelswu(), vrelsw();
void dmapinit(), dmfree();

struct lock_t lk_totalswap;

#define ALLOC_VAS(siz, err) { \
	int s = splimp(); \
	smp_lock(&lk_totalswap, LK_RETRY); \
	if(!((err) = ((siz) > availvas))) \
		availvas -= (siz); \
	smp_unlock(&lk_totalswap); \
	(void)splx(s); \
}

#define FREE_VAS(siz)  {  \
	int s = splimp(); \
	smp_lock(&lk_totalswap, LK_RETRY); \
	availvas += (siz); \
	smp_unlock(&lk_totalswap); \
	(void)splx(s); \
}

/* Definition of variables in macros */
/* 	size    - amount of swap space to be allocated */
/* 	segtype - segment type - e.g. CTEXT */
/* 	segsize - size of the segment in core clicks */

#define ALLOC_SWAP(size, segtype)  { \
	register int s = splimp(); \
	smp_lock(&lk_totalswap, LK_RETRY); \
	availswap -= (size); \
	if (segtype == CTEXT)  \
		swapu.txt += (size); \
	else if (segtype == CSMEM) \
		swapu.smem += (size); \
	swapu.total_used  += (size); \
	smp_unlock(&lk_totalswap); \
	(void)splx(s); \
}

/* FREE_SWAP - used to free swap space for any fragment */

#define FREE_SWAP(size, segtype) { \
	register int s = splimp(); \
	smp_lock(&lk_totalswap,LK_RETRY); \
	availswap += (size); \
	if (segtype == CTEXT)  \
		swapu.txt -= (size); \
	else if (segtype == CSMEM) \
		swapu.smem -= (size); \
	swapu.total_used  -= (size); \
	smp_unlock(&lk_totalswap); \
	(void)splx(s); \
}

/* FREE_WASTED - used to update wasted swap space whenever swap space */
/* is freed for the last fragment i.e. dm_map[dm_last-1] fragment only */
/* swapfrag is assumed to be a power of 2 */
						
#define FREE_WASTED(segsize) { \
	register int s = splimp(); \
	int i; \
	smp_lock(&lk_totalswap, LK_RETRY);\
	if ((i = (ctod(segsize) & (swapfrag-1))) != 0) \
		swapu.wasted -= (swapfrag - i); \
	smp_unlock(&lk_totalswap); \
	(void)splx(s); \
}

/* ALLOC_WASTED - used to upate  wasted swap space whenever swap space */
/* is freed for the last fragment i.e. dm_map[dm_last-1] fragment only   */

#define ALLOC_WASTED(segsize) { \
	register int s = splimp(); \
	int i; \
	smp_lock(&lk_totalswap, LK_RETRY);\
	if ((i = (ctod(segsize) & (swapfrag-1))) != 0) \
		swapu.wasted += (swapfrag - i); \
	smp_unlock(&lk_totalswap); \
	(void)splx(s); \
}

/* SWAPEXP_WASTAGE - used by dmexpand on swap expansion to */
/* revaluate the amount of wasted space.  */
/* 	osegsize - segment size BEFORE exapansion in core clicks */
/* 	odment	 - the last dmap entry BEFORE expansion */

#define SWAPEXP_WASTAGE(osegsize, odment) { \
	if (odment != 0) \
		FREE_WASTED(osegsize); \
} 

/* SWAPSHR_WASTAGE - used by dmexpand on swap contraction to */
/* revaluate the amount of wasted space. */
/* 	osegsize - segment size BEFORE contraction in core clicks */
/* 	nsegsize - segment size AFTER contraction in core clicks */
/* 	odment	 - the last dmap entry BEFORE contraction */
/* 	ndment	 - the last dmap entry AFTER contraction */

#define SWAPSHR_WASTAGE(osegsize, odment, nsegsize, ndment) { \
	register int s; \
	int nwaste = 0; \
	int owaste = 0; \
	int i; \
	if (odment != 0) \
		owaste = (i = (ctod(osegsize) & (swapfrag-1))) ? swapfrag - i  : 0 ;\
	if (ndment != 0) \
		nwaste = (i = (ctod(nsegsize) & (swapfrag-1))) ? swapfrag - i : 0 ;\
	if (owaste || nwaste ) {  \
		s = splimp(); \
		smp_lock(&lk_totalswap, LK_RETRY); \
		swapu.wasted += nwaste - owaste ; \
		smp_unlock(&lk_totalswap); \
		(void) splx(s); \
	} \
} 

#endif /* KERNEL */
