/*	tarsets.c -
 *		write media image production scripts.
 *
 *
 *			Copyright (c) 1985, 1988 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
 *
*/

/************************************************************************
 *
 *	Modification History:
 *
 *	000	afd	1984
 *	001	ccb	1-JUN-1988
 *		Re-Implemented to clean up several structural problems
 *		that made for difficult maintenance.
 *		No functional change.
 *
 ************************************************************************
*/

#ifndef lint
static	char *sccsid = "@(#)tarsets.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdio.h>

/*	externs -
 *		errno - standard
 *		sys_errlist - standard
*/

extern	errno;
extern	*sys_errlist[];

/*	constants -
 *
 *	BLKSIZ - size of a tar block.
 *	DIR - inventory char for IFDIR files
 *	FACLEN - maximum size of subset names
 *	HLINK - inventory char for hard links
 *	I_OVERSIZED - inventory flags file oversized flag.
 *	I_USED - inventory flags node used mask
 *	PCKSIZ - number of BLKSIZ blocks that fit a package
 *	PERFILECHRS - number of chars/file overhead in script.
 *	REG - inventory char for IFREG files
 *	SLINK - inventory char for IFLNK files
 *	TARPAD - tar overhead per file
 *	TARSIZ - tar blocking factor for tar's -b switch
 *	TOP - constant value at top recursion level
 *	TS_DIRTY - Volume in use mask
 *	TS_OVERSIZED - Oversized volume encountered mask
 *	TS_VOLREADY - Volume ready mask
*/
#define	BLKSIZ		512
#define	DIR		'd'
#define	FACLEN		16
#define	HLINK		'l'
#define	I_OVERSIZED	0x0002
#define	I_USED		0x0001
#define	PCKSIZ		350
#define	PERFILECHRS	4
#define	REG		'f'
#define	SLINK		's'
#define	TARPAD		1
#define	TARSIZ		20
#define	TOP		0
#define	TS_DIRTY	0x0001
#define	TS_OVERSIZED	0x0002
#define	TS_VOLREADY	0x0004

/*	macros -
 *
 *	BYTOBL -
 *		args: a number of bytes
 *		does: xlate file bytes to size of file in tar image
 *
 *	ISOPEN -
 *		args: a volume descriptor
 *		does: resolves to true if TS_VOLREADY is set
 *
 *	USAGE -
 *		args: none
 *		does: generate a usage message
*/
#define	BYTOBL(S)	( ( (S) + BLKSIZ ) / BLKSIZ + TARPAD)
#define	ISOPEN(V)	( (V)->v_flags & TS_VOLREADY )
#define	Usage()		fprintf(stderr,"%s: Usage: %s master-dir\n",prog, prog);
#define	reg		register

/*	TYPEDEFS
*/
typedef struct INV	/* record for list of distribution files */
{
	struct INV	*i_next;		/* link to next file record */
	char		i_path[MAXPATHLEN];	/* file name */
	struct INV	*i_hpl;			/* ptr to hard links */
	int		i_flags;		/* node flags */
	unsigned	i_chrs;			/* size of the file name */
	unsigned	i_blks;			/* # blocks in file */
	unsigned	i_tchrs;		/* # chrs for inode */
	unsigned	i_tblks;		/* # blocks for inode */
} INV;
typedef struct vip	/* Volume Info Pack */
{
	unsigned	v_num;		/* volume number */
	unsigned	v_chrs;		/* number of chars written to file */
	unsigned	v_blks;		/* number of disk blocks allocated */
	unsigned	v_flags;	/* flags word */
} VIP;

/*	SUBR TYPE DEFINITIONS
*/
char	*emalloc();		/* error checking malloc */
INV	*ReadInput();		/* Found Below */

/*	GLOBAL DATA
*/
char		buf[BUFSIZ];		/* scratch buffer */
char		subset[FACLEN];		/* global storage for subset name */
char		*prog;			/* global prog name for err strings */
char		mntpath[MAXPATHLEN];	/* mount path of build */
struct stat	stb;			/* global stat buffer */
int		debug = 0;		/* debug flag, default 0 */
FILE		*dbfp;			/* debug output FILE pointer */
char		*debugfile = "ts.dbg";	/* debug output file name */


/*	Main() -
 *		main type stuff
 *
 *	args:	standard
 *	does:	parse command line.
 *		enable debugging if required and it's possible
 *		freopen stdout to an error file.
 *		call ReadInput() to internalize INV records comming
 *			on stdin
 *		call WriteScript() to handle backpacking
 *		call OverSized() to handle records bigger than a
 *			breadbox
 *		call TsEndVol() to close out any trailing records.
 *		exit.
 *	return:	0 on success, 1 on failure.
 *	fails:	on bad calling syntax, prints usage message.
*/

main (argc,argv)
int argc;
char *argv[];
{
	INV	*DataList;	/* main list */
	VIP	v;		/* Vol Descriptor */

	prog = argv[0];	/* save program name */

	/* decode args */
	switch( argc )
	{
	case 3:
		/* perhaps debug */
		if( strcmp("-d", argv[1] ) )
			goto error;

		++debug;
		++argv;

	case 2:
		/* no debug */
		if( !debug && !strcmp(argv[1], "-d") )
			goto error;

		strcpy(mntpath,argv[1]);
		break;

	default:
	error:
		Usage();
		exit(1);
	}

	if( debug )	/* open debug output file */
	{
		if( (dbfp = fopen(debugfile, "w")) != NULL )
			fprintf(stderr, "Debugging On, file %s\n", debugfile);
		else
		{
			/* can't open debug file.
			 *  print warning and turn off debugging.
			*/
			fprintf(stderr,
				"%s: Unable to open %s (%s)\nDebug Off.\n",
				prog, debugfile, sys_errlist[errno] );
			debug = 0;
		}
	}
	/* bind stderr to an error file
	*/
	fprintf(stderr, "Stderr is redirected to diagnostics file `stderr'\n");
	freopen("stderr", "a", stderr);

	bzero( (char *) &v, sizeof(VIP) );
	DataList = ReadInput();
	WriteScript(DataList, &v, TOP);
	OverSized(DataList, &v);
	TsEndVol(&v);
	exit(0);

}


/*	ReadInput() -
 *		build internal representation of the inventory
 *		being fed thru stdin.
 *
 *	args:	none
 *	does:	reads records from stdin. each record representing
 *		a file or link is scribbled into an INV structure and
 *		placed in a linked list.
 *	return:	a pointer to the head of the list.
*/

INV *ReadInput()
{
	reg INV	*p;	/* pointer to main list */
	reg INV	*q;	/* pointer to working record */

	char	linkto[MAXPATHLEN];	/* linkto name, current record */
	off_t	size;			/* file size, current record */
	char	type;			/* file type, current record */
	int	lncnt;			/* input line counter */

	for( p = q = (INV *) 0, lncnt = 1; gets(buf) != NULL; ++lncnt )
	{
		if(!q)
		{
			/* time to get a new node */
			q = (INV *) emalloc(sizeof(INV));
			bzero( (char *) q, sizeof(INV) );
		}

		if( sscanf(buf,"%*s%d%*d%*hd%*hd%*ho%*s%*s%1s%s%s%s",
			&size, &type, q->i_path, linkto, subset ) != 5 )
		{
			fprintf(stderr,
				"%s: Invalid Record on line %d: %s",
				lncnt, buf );
		}

		/* DIR == directory */
		if( type == DIR )
			continue;

		q->i_chrs = strlen(q->i_path) + PERFILECHRS;

		if( type == HLINK && strcmp(linkto, "none") )
		{
			/* Hard Link, not first instance of this
			 *  inode encountered. Set up it's INV
			 *  and call to have it linked into the
			 *  the list under it's inode-group.
			*/
			q->i_blks = TARPAD;
			AddHardLink(q,p,linkto);
		}
		else
		{
			/* real file, fill node and link */

			q->i_tchrs = q->i_chrs;
			q->i_blks = q->i_tblks = BYTOBL(size);
			q->i_next = p;
			p = q;
		}
		q = NULL;
	}
	return(p);
}



/*	AddHardLink() -
 *		add hard link node to list
 *
 *	given:	pointer to unreferenced node.
 *		pointer to top node.
 *		pointer to linkto string.
 *	does:	associate the node with it's inode group.
 *		update inode lead blk, chr counters
 *	return:	Pointer to the parent of the inode group.
*/

AddHardLink(n,l,s)
reg INV *n, *l;
reg char *s;
{
	/* find node in l to which n should be linked */
	for(; l != (INV *) 0; l = l->i_next)
	{
		if(!strcmp(l->i_path, s))
		{
			/* got it, link into inode group... */
			n->i_hpl = l->i_hpl;
			l->i_hpl = n;

			/* ... and update parent counters */
			l->i_tchrs += n->i_chrs;
			l->i_tblks += n->i_blks;
			return;
		}
	}
	fprintf(stderr, "%s: %s -> %s link reference unresolved.\n",
		prog, n->i_path, s);
	exit(1);
}



/*	WriteScript() -
 *		write the output script.
 *
 *	given:	a pointer to the current head of the inv list.
 *		pointer to a volume descriptor.
 *	does:	arbitrates the output of tar commands such that no
 *		command would overflow a single diskette.
 *		and so that no command line in the resulting script
 *		exceeds NCARGS
 *	return:	no explicit return value.
 *
 *	NOTE:	recursion.
*/

WriteScript(p,v,level)
reg INV *p;
VIP *v;
int level;
{
	if( debug )
	{
		fprintf(dbfp,
			"%s: Entering WriteScript(%8x,%8x,%d)\n",
			prog, p, v, level);
	}

	while( p != (INV *) 0 )	/* Loop to end of linked list */
	{
		if( debug )
		{
			 fprintf(dbfp, "%s: Node %s\n", prog, p->i_path);
		}
		if( p->i_flags & I_USED )
		{
			if( debug )
			{
				fprintf(dbfp,"%s: In Use Node %s\n",
					prog, p->i_path);
			}
			/* this node already assigned to a volume */
			p = p->i_next;
			continue;
		}
		if( p->i_tblks > PCKSIZ )
		{
			/* oversized file.
			 *  print error message, set oversized flag
			 *  on the volume descriptor, go on to next volume.
			*/
			fprintf(stderr, "Warning: file %s is %d blocks",
				p->i_path, p->i_tblks);
			fprintf(stderr, "\ttoo large for diskette\n\n");
			if( debug )
			{
				fprintf(dbfp, "%s: Oversized Node %s\n",
					prog, p->i_path);
			}

			v->v_flags |= TS_OVERSIZED;
			p->i_flags |= (I_OVERSIZED|I_USED);
			p = p->i_next;
			continue;
		}


		/* Determine if current file fits 
		 *  in current volume, check is made against
		 *  the file and all of it's hard link overhead
		 *  as well as the number of characters that
		 *  will be required to print it
		*/
		if( (v->v_blks + p->i_tblks) > PCKSIZ ||
			(v->v_chrs + p->i_tchrs) >= NCARGS )

		{
			/* It doesn't. Recur to pick one that
			 *  fits.
			*/
			if( debug )
			{
				fprintf(dbfp,
					"%s: Node doesn't fit, will recur\n",
					prog );
			}

			WriteScript( p->i_next, v, level+1 );

			/* Return from recursion:
			 *  if level is !0, pop.
			*/
			if( level )
			{
				if( debug )
				{
					fprintf(dbfp,
						"%s: return level %d\n",
						prog, level);
				}
				return(0);
			}
			/* Level 0 return:
			 *  Volume assumed full to capacity, close.
			*/

			TsEndVol(v);
		}
		else
		{
			/* file fits, Write it. */
			if( debug )
			{
				fprintf(dbfp, "%s: Node fit, volume %d\n",
					prog, v->v_num);
			}
			Output(p,v);
			p = p->i_next;
		}

	}
	if( debug )
	{
		fprintf(dbfp,"%s: return level %d\n", prog, level);
	}
}



/*	OverSized() -
 *		write diskette records that are bigger than a
 *		single diskette.
 *
 *	args:	pointer to a list of INVs.
 *		pointer to the current volume structure.
 *
 *	does:	Create single volume for all files larger than one diskette.
 *
 *	return:	Nothing
*/

OverSized(p,v)
INV *p;
register VIP *v;
{
	if( v->v_flags & TS_OVERSIZED )
	{
		if( v->v_flags & (TS_VOLREADY|TS_DIRTY) )
			TsNewVol(v);

		fprintf(stderr, "Writing Oversized File Volume...\n");
		for( ;p != (INV *) 0; p = p->i_next)
		{
			if( p->i_flags & I_OVERSIZED )
				Output(p,v);
		}
	}
}



/*	Output() -
 *		print pathname from node.
 *
 *	given:	pointer to a node.
 *		pointer to a volume info pack
 *	does:	print a line for the script containing line
 *		line continuation and the node->path.
 *		track down all hard links to this node and
 *		do likewise for them. track tar padding
 *		for the hard link records.
 *	return:	number of blocks consumed by files and links for which
 *		output records were created.
*/

Output(p, v)
reg INV *p;
reg VIP *v;
{
	if( debug )
	{
		fprintf( dbfp, "%s: Entering Output()\n", prog);
	}
	if( !(v->v_flags & TS_VOLREADY) )
		TsNewVol(v);

	/* add all blocks and chars for this file into
	 *  the volume descriptor
	*/
	v->v_blks += p->i_tblks;
	v->v_chrs += p->i_tchrs;

	/* mark to volume as DIRTY
	*/
	v->v_flags |= TS_DIRTY;

	/* print the parent path and mark as used.
	*/
	printf( " \\\n'%s'", p->i_path );
	p->i_flags |= I_USED;

	/* loop thru and print paths for each member of
	 *  the inode group
	*/
	for(p = p->i_hpl; p != (INV *) 0; p = p->i_hpl)
		printf( " \\\n'%s'", p->i_path);

	if( debug )
	{
		fprintf( dbfp, "%s: Output() returning\n", prog );
	}
}


/*	TsNewVol() -
 *		start new volume.
 *
 *	args:	a pointer to a volume descriptor.
 *	does:	closes any pending open volume.
 *		writes tar command header.
 *		resets blocks and character count.
 *		arranges for volume file to be output.
*/

TsNewVol(v)
reg VIP *v;
{
	static char	volbuf[10];
	static char	*cmdfmt = "tar cpobf %d ${TO}/%s%d \\\n%s\\\n -C %s";

	TsEndVol(v);	/* close open volume */
	++(v->v_num);	/* increment volnum */

	/* calulate block requirement for Volume file,
	 *  placed in volume when the volume record is
	 *  first opened
	*/
	sprintf(volbuf,"Volume%d",v->v_num);
	stat(volbuf,&stb);
	v->v_blks = (stb.st_size + BLKSIZ ) / BLKSIZ + TARPAD;

	/* write beginning of tar command */
	sprintf( buf, cmdfmt, TARSIZ, subset, v->v_num, volbuf, mntpath);
	fputs( buf, stdout );

	/* account for chars just written + trailing newline
	 *  needed in TsEndVol()
	*/
	v->v_chrs = strlen(buf) + 1;

	/* mark volume ready */
	v->v_flags |= TS_VOLREADY;
}



/*	TsEndVol() -
 *		close out a volume segment on the output script.
 *
 *	Given:	a pointer to a volume descriptor
 *	Does:	Verifies that there the volume record is writable.
 *		Writes block sizes for volume to stderr.
 *		Writes a newline to the output, clears TS_VOLREADY
 *			in the descriptor flags.
 *	Return:	Unused
*/

TsEndVol(v)
reg VIP *v;
{
	if( debug )
	{
		fprintf(dbfp, "%s: TsEndVol()\n", prog);
	}
	if( v->v_flags & TS_VOLREADY )
	{
		/* write status record to stderr
		*/
		fprintf(stderr, "%d Blocks, %d Chars on Volume %s%d\n",
			v->v_blks, v->v_chrs, subset, v->v_num);

		fputs("\n", stdout);
		v->v_flags &= ~TS_VOLREADY;
		v->v_blks = v->v_chrs = 0;
	}
}

