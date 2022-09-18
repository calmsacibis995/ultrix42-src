#ifndef lint
static  char    *sccsid = "@(#)geometry.c	4.1  (ULTRIX)        7/2/90";
#endif lint

/*
 * geometry.c
 *
 * Abstract:  This program is called to determine the number of sectors and
 *	      tracks of a disk which is targeted for installation.  This
 *	      information will be passed to the newfs command.
 *
 * Args:      Two arguments are passed in.  The first is the disk type (ra, rz),
 *	      the second is the logical unit number (ie 0). 
 *
 * Output:    Two numbers will be printed to stdout.  The first number is the
 *	      number of sectors, the second number is the number of tracks.
 *	      If the geometry can't be determined the values printed will be
 *	      -1.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/devio.h>
#include <sys/ioctl.h>

#define	UNKNOWN_SPEC	-1	/* Value assigned in failure cases */
#define NAME_LENGTH	20	/* Dev name, ie rra20a		   */

/*
 * Produce program output.
 */
#define Give_geometry(sectors, tracks) 				\
	printf("%d %d\n",sectors, tracks);
/*
 * An error condition exists.  Return an indication that 
 * the geometry has not been obtained.
 */
#define Error_exit {						\
		Give_geometry(UNKNOWN_SPEC, UNKNOWN_SPEC);	\
		exit(1);					\
	}
/*
 * The following macros produce major and minor numbers based on the
 * logical unit number.
 */
#define RZ_BASEMAJOR	56			/* Major number for rz */
#define RA_BASEMAJOR	60			/* First major # for ra */

#define RZMAJOR(unit)	RZ_BASEMAJOR
#define RZMINOR(unit)	(unit << 3)		/* Partition in low 3 bits */

#define RAMAJOR(unit)	(RA_BASEMAJOR + (unit/32))
#define RAMINOR(unit)	((unit%32) << 3)


main(argc, argv)
	int argc;
	char *argv[];
{
	char *devtype;
	int devunit;
	char *c;
	int not_number = 0;
	char devname[NAME_LENGTH];
	int dev;
	int fd;
	DEVGEOMST devgeom;

	if (argc != 3) {
		fprintf(stderr,"geometry: error, not 2 args.\n");
		Error_exit;
	}

	/*
	 * Assign the device name and unit number.  Verify that the unit
	 * number is a valid numeric.  Verify that the device name is supported
	 * present support includes "ra" and "rz".  Obtain device geometry
	 * information and produce output.
	 */
	devtype = argv[1];
	for (c = argv[2]; *c; c++) {
		if (!isdigit(*c)) {
			not_number++;
		}
	}
	if (not_number || ((devunit = atoi(argv[2])) < 0)) {
		fprintf(stderr, "geometry: error, invalid unit number %s\n",
			argv[2]);
		Error_exit;
	}

	if ((strncmp(devtype, "ra", 2) == 0) ||
	    (strncmp(devtype, "rz", 2) == 0)) {
		sprintf(devname,"r%sa",devtype);
		if (strncmp(devtype, "ra", 2) == 0)
			dev = makedev(RAMAJOR(devunit), RAMINOR(devunit));
		else 
			dev = makedev(RZMAJOR(devunit), RZMINOR(devunit));
		mknod(devname, 0020666, dev);
		fd = open(devname, O_RDONLY | O_NDELAY);
		if (fd < 0) {
			fprintf(stderr, "geometry: can\'t open %s\n",devname);
			unlink(devname);
			Error_exit;
		}
		if (ioctl(fd, DEVGETGEOM, (char *)&devgeom) < 0) {
			fprintf(stderr, "geometry: DEVGETGEOM failed.\n");
			unlink(devname);
			Error_exit;
		}
		unlink(devname);
		close(fd);
		Give_geometry(devgeom.geom_info.nsectors, devgeom.geom_info.ntracks)
		
	}
	else {
		fprintf(stderr, "geometry: error, non-supported disk type %s\n",
			devtype);
		Error_exit;
	}
	return(0);
}
