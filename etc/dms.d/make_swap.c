#ifdef lint
static char sccsid[]="@(#)make_swap.c	4.1  (ultrix)  7/2/90";
#endif

/*
 * Program Name: make_swap
 *
 * Function: Preallocate a swap file for client
 *
 * Input Arguments: path - pathname of swap file
 *		    size - size of swap file
 *
 * Modification History:
 *
 * Date		Author			Description
 * ====		======			===========
 * 31-Oct-86	Suzanne Logcher		Creation
 * 25-Feb-87    Lea Gottfredsen		No printout if successful
 */

#include <sys/file.h>
#include <limits.h>
#include <dial.h>

/*
 * Extract full pathname of swap file and integer for number of
 * half meg chunks for total swap file size.
 */

main(argc, argv)
	int argc;
	char *argv[];
{
	char *path;
	int size;

	if (--argc <= 0)
		usage();
	path = (char *)malloc(strlen(*argv++)+1);
	if (!path) {
		exit(FALSE);
	}
	strcpy(path, *argv);
	argv++;
	size = atoi(*argv);
	if (make_swap(path, size) == FALSE)
		printf("make_swap: unsuccessful\n");
}

/*
 * Write swap file in half meg chunks
 */

make_swap(path, size)
	char *path;
	int size;
{
	int fd;
	int i, j;
	char buffer[4096];
	int chunk = 4096;
	int munga = 128;
	int errno;

	if ((fd = open(path, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0) {
		return(FALSE);
	}
	for (i = 0; i < size; i++)
		for (j = 0; j < munga; j++) {
			if ((write(fd, buffer, chunk)) != chunk) {
				close(fd);
				return(FALSE);
			}
	}
	if (close(fd) < 0) {
		return(FALSE);
	}
	return(TRUE);
}

usage()
{
	printf("make_swap: usage: make_swap pathname size\n");
	exit(FALSE);
}

