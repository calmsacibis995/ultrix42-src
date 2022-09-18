/*
 *	Each constant in this file is an integer index into the ps_data
 *	array in /vmunix. This array can be read to get the size of
 *	individual structures even if the structure changes given that
 *	it changes at the end of the strucuture.
 */

/* the entries here need to match the constants in ps_data.c */

#define PS_DATA_proc		0
#define PS_DATA_pte		1
#define PS_DATA_text		2
#define PS_DATA_user		3
#define PS_DATA_user_u_comm	4
#define PS_DATA_user_u_arg	5
#define PS_DATA_inode		6
#define PS_DATA_tty		7
#define PS_DATA_ucred		8
#define PS_DATA_rusage		9
#define PS_DATA_file		10
#define PS_DATA_ucred		11
#define PS_DATA_rusage		12
#define PS_DATA_map		13
#define PS_DATA_swdevt		14
#define PS_DATA_vme_device	15
#define PS_DATA_vme_driver	16
