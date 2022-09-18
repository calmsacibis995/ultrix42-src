/* 	@(#)ustat.h	4.1	(ULTRIX)	7/2/90 	*/

struct  ustat {
	daddr_t	f_tfree;	/* total free */
	ino_t	f_tinode;	/* total inodes free */
	char	f_fname[512];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
};
