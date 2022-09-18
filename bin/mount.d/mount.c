#ifndef lint
static	char	*sccsid = "@(#)mount.c	4.1	(ULTRIX)	7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986,87 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 *			Modification History
 *
 * 13-Jul-89 -- lebel
 *  Added check for malformed line in /etc/fstab since pmax will not
 *  follow null pointers.
 *
 * 13-Jul-88 -- prs
 *	Added parsing of the options field of /etc/fstab to the
 *	case where one parameter is passed in.
 *
 * 28-Apr-87 -- prs
 *	Fixed return status
 *
 * 15-Jan-87 -- prs
 *	Added code in main() to add the quota option to the option string
 *	if the -a option was specified, and the mode for the mount was
 *	rq
 *
 ************************************************************************/

/*
 * mount
 */

#include <sys/param.h>
#include <stdio.h>
#include <fstab.h>
#include <sys/mount.h>
#include <sys/fs_types.h>

#ifdef DEBUG
char *PATHHEAD = "IFS= PATH=/etc:/bin:/usr/bin ./";
#else DEBUG
char *PATHHEAD = "IFS= PATH=/etc:/bin:/usr/bin /etc/";
#endif DEBUG

struct fs_data mountbuffer[NMOUNT];
int errflag,aflag,oflag,rflag,tflag,vflag,Vflag;
int loc;
struct fstab *fsp;
char *malloc(),*strcpy(),*strcat(), *convert(), *index(), *calloc();
char *AT = "@";
char *COLON = ":";
FILE *fd = NULL;
int fdtype = -1;
void exit(),perror();

main(argc, argv)
	int argc;
	char **argv;
{
	register int c, ret;
	register char *options = "", *progname, *name = "ufs";
	register struct fs_data *fs_data;
	extern char *optarg;
	extern int optind;

	progname = argv[0];
	while ((c = getopt(argc,argv,"ao:rt:vV")) != EOF) {
		switch (c) {
		case 'a':
			aflag++;
			break;
		case 'o':
			oflag++;
			options = optarg;
			break;
		case 'r':
			rflag++;
			break;
		case 't':
			tflag++;
			name = optarg;
			break;
		case 'v':
			vflag++;
			break;
		case 'V':
			Vflag++;
			break;
		case '?':
			errflag++;
		}
	}
	/* give usage message if any bad flags are used or if there are */
	/* if -a was on no more args allowed and no -o or -r */
	if (aflag) {
		if ((optind != argc) || oflag || rflag) errflag++;
	}
	if (!aflag) {
		switch (argc - optind) {
		case 0:
			if (oflag||tflag||rflag) errflag++;
			break;
		case 1:
			if (oflag||tflag) errflag++;
			break;
		case 2:
			break;
		default:
			errflag++;
			break;
		}
	}
	if (errflag) {
		(void) fprintf(stderr,"Usage: ");
		(void) fprintf(stderr,"%s [-v]\n",progname);
		(void) fprintf(stderr,"%s -a [-v]\n",progname);
		(void) fprintf(stderr,"%s -a -t type [-v]\n",progname);
		(void) fprintf(stderr,"%s [-v] [-r] ",progname);
		(void) fprintf(stderr,"special_name|path_name\n");
		(void) fprintf(stderr,"%s [-v] [-r] -t type ",progname);
		(void) fprintf(stderr,"[-o options_string] ");
		(void) fprintf(stderr,"special_name path_name\n");
		exit(1);
	}
	if (aflag) {
		if (setfsent() == 0) {
			perror(FSTAB);
			exit(1);
		}
		ret = 0;
		while ((fsp = getfsent()) != 0) {
			/* check for null first */
			if (fsp->fs_name == '\0') continue;

			if (strcmp(fsp->fs_file, "/") == 0)
				continue;
			if (tflag && strcmp(fsp->fs_name,name) != 0)
				continue;	/* all of one type */
			rflag = 0;
			if (!strcmp(fsp->fs_type, FSTAB_RO)) rflag = 1;
			else if (strcmp(fsp->fs_type, FSTAB_RW) &&
			         strcmp(fsp->fs_type, FSTAB_RQ)) continue;
			/*
			 * If the file system type is quota, add the
			 * quota option to the option string that
			 * will be passed to the file systems type
			 * mount routine.
			 */
			if (!strcmp(fsp->fs_type, FSTAB_RQ))
				strcat(fsp->fs_opts, (strlen(fsp->fs_opts)) ? ",quota" : "quota");
			oflag = 0;
			if (fsp->fs_opts[0] != '\0') oflag = 1;
			ret += mountfs(convert(fsp->fs_spec,AT,COLON),
				fsp->fs_file, fsp->fs_name,fsp->fs_opts);
		}
		exit(ret);
	}
	if (optind+1 == argc) {
		/* this handles the case where the user typed */
		/* "mount /dev/ra0g" or "mount /usr" or "mount -r /usr" */
		if (setfsent() == 0) {
			perror(FSTAB);
			exit(1);
		}
		fsp = getfsfile(argv[optind]);
		if (fsp == NULL) {
			fsp = getfsspec(convert(argv[optind],COLON,AT));
			if (fsp == NULL) {
				(void) fprintf(stderr,
						"Don't know how to mount %s\n",
						argv[optind]);
				exit(2);
			}
		}
		rflag = 0;
		if (!strcmp(fsp->fs_type, FSTAB_RO)) rflag = 1;
		/*
		 * If the file system type is quota, add the
		 * quota option to the option string that
		 * will be passed to the file systems type
		 * mount routine.
		 */
		if (!strcmp(fsp->fs_type, FSTAB_RQ))
			strcat(fsp->fs_opts, (strlen(fsp->fs_opts)) ? ",quota" : "quota");
		oflag = 0;
		if (fsp->fs_opts[0] != '\0') oflag = 1;
		ret = mountfs(convert(fsp->fs_spec,AT,COLON), fsp->fs_file,
						fsp->fs_name,fsp->fs_opts);
		exit(ret);
	}
	if (optind+2 == argc) {
		/* this handles "mount -r /dev/ra0g /usr" */
		ret = mountfs(convert(argv[optind],AT,COLON), argv[optind+1],
							name, options);
		exit(ret);
	}
	/* user wants mounted file systems printed */
	/* use this so that we don't hang if server's down with nfs file sys */
	ret = getmountent(&loc, mountbuffer, NMOUNT);
	if (ret == 0) {
		perror("getmountent");
		exit(3);
	}
	for (fs_data = mountbuffer; fs_data < &mountbuffer[ret]; fs_data++) {
		if (!Vflag) printfs(fs_data);
		else {
			int precise;
			printf("flags   = 0x%-6x ",fs_data->fd_flags);
			printf("mtsize  = %-8d ",fs_data->fd_mtsize);
			printf("otsize  = %-8d ",fs_data->fd_otsize);
			printf("bsize   = %-8d\n",fs_data->fd_bsize);
			if (fs_data->fd_fstype > 99) precise = 3;
			else if (fs_data->fd_fstype > 9) precise = 2;
			else precise = 1;
 			printf("fstype  = %*d(%-3.3s)%*s",
					precise,fs_data->fd_fstype,
					gt_names[fs_data->fd_fstype],3-precise,
					"   ");
			printf("gtot    = %-8d ",fs_data->fd_gtot);
			printf("gfree   = %-8d ",fs_data->fd_gfree);
			printf("btot    = %-8d\n",fs_data->fd_btot);
			printf("bfree   = %-8d ",fs_data->fd_bfree);
			printf("bfreen  = %-8d ",fs_data->fd_bfreen);
			printf("pgthresh= %-8d ",fs_data->fd_pgthresh);
			printf("uid     = %-8d\n",fs_data->fd_uid);
			printf("dev     = 0x%-6x ",fs_data->fd_dev);
			printf("devname = %s     ",fs_data->fd_devname);
			printf("pathname= %s\n\n",fs_data->fd_path);
		}
	}
	if(fd) {
		(void) pclose(fd);
	}
	exit(0);
}

printfs(fs_data)
register struct fs_data *fs_data;
{
	register type = fs_data->fd_fstype;
	char progname[MAXPATHLEN];
	
	if (type < 0 || type >= NUM_FS) {
		(void) fprintf(stderr,"file system type is bad %d\n",type);
		return;
	}
	(void) strcpy(progname,PATHHEAD);
	(void) strcat(progname,gt_names[type]);
	(void) strcat(progname,"_mount -p");
 	if (fd && fdtype != type) {
		(void) pclose(fd);
		fd = NULL;
	}
	if (fd == NULL) {
		if ((fd = popen(progname, "w")) == NULL) {
			perror("popen");
			return;
		}
	}
	fdtype = type;
	if(fwrite((char *)fs_data, sizeof(struct fs_data), 1, fd) != 1) {
		perror("fwrite on pipe");
	}
	(void) fflush(fd);
}

mountfs(spec, file, name, options)
	register char *spec, *file, *name, *options;
{
	char progname[BUFSIZ];
	register char *blank = " ";
	register char *singleq = "'";

	if (!tflag && remotename(spec))
		name = "nfs";

	(void) strcpy(progname, PATHHEAD);
	(void) strcat(progname,name);
	(void) strcat(progname,"_mount ");
	if (vflag) (void) strcat(progname,"-v ");
	if (rflag) (void) strcat(progname,"-r ");
	/* put single quotes in so shell doesn't touch args again */
	if (oflag) {
		(void) strcat(progname,"-o ");
		(void) strcat(progname,singleq);
		(void) strcat(progname,options);
		(void) strcat(progname,singleq);
		(void) strcat(progname,blank);
	}
	(void) strcat(progname,singleq);
	(void) strcat(progname,spec);
	(void) strcat(progname,singleq);
	(void) strcat(progname,blank);
	(void) strcat(progname,singleq);
	(void) strcat(progname,file);
	(void) strcat(progname,singleq);
	return(system(progname) >> 8);
}

/*
 *	routine to convert notation "/@abyss" to the notation
 *	abyss:/ and vice versa (abyss:/ to /@abyss)
 *	This lets you put /@abyss in /etc/fstab without changing
 *	fstab.c and it lets you use EITHER notation on the command line!
 *	Any other way of doing this is worse since the ':' is used as
 *	the field separator in /etc/fstab - rr
 */
char *convert(s,bad,good)
register char *s;
char *bad, *good;
{
	register char *t, *p;
	register int len1,len2,i;
	char *ptr;
	if ((p = index(s,*bad)) == NULL) {
		return(s);
	}
	ptr = t = calloc(MAXPATHLEN,1);
	len1 = p - s;
	len2 = strlen(s) - len1 - 1;
	p++;
	for(i=0;i<len2;i++) *t++ = p[i];
	*t++ = *good;
	for(i=0;i<len1;i++) *t++ = s[i];
	return(ptr);
}

/*
 *	Check to see if the "special" name probably refers to a remote
 *	mount point (eg: "orville:/usr/src").  We do this by checking
 *	to see if the name contains a colon.  This is done so that people
 *	who type "mount zot:/ /zot" instead of the ideologically correct
 *	"mount -t nfs zot:/ /zot" won't get the cryptic error message "No
 *	such file or directory".  Perverse individuals who insist on
 *	keeping device files with ":" in the pathname will just have to
 *	type "-t ufs" on their mount command lines.	- Chase
 */

int
remotename(spec)
	char *spec;
{
	char *i;
	i = index(spec, ':');
	if (i == NULL)
		return(0);
	return(1);
}
