#ifndef lint
static	char	*sccsid = "@(#)capsar.c	4.1	(ULTRIX)	7/17/90";
#endif	lint

#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <capsar.h>

#define MAXPATHLEN	1024


/*-------*\
 Constants
\*-------*/

#define	FALSE	0
int	HELP;
char	*malloc();
int	rflag;
char	*rindex();
char	*strcat();
int	tflag=0;
int	fflag=0;
int	xflag=0;
int	Tflag=0;
int	hflag=0;
int	cflag=0;
int	DOTSflag=0;
int	DDIFflag=0;
char	*progname;

main(argc, argv)
	int	argc;
	char	*argv[];
{
/*------*\
  Locals
\*------*/

	char	*cp,
		*usefil,
		filetag[BUFSIZ],
		mtype[BUFSIZ],
		**pp,**pp0,
		*pps;
	int	i,
		c,
		errflg=0,
		fd;
	MM	*m,*m0;
extern	int	optind;
extern	char	*optarg;

/*------*\
   Code
\*------*/

	progname = argv[0];	/* Get our name*/


	if (argc < 2)
		usage();


	while((c = getopt(argc,argv,"ctx:")) !=EOF){
		switch(c){
			case 'c':	/* create option */
				cflag++;
				break;
			case 't':	/* check option */
				tflag++;
				break;
			case 'x':	/* extract option */
				xflag++;
				switch(*optarg){
					case 'h': /* headers */
						hflag++;
						break;
					case 'T': /* headers */
						Tflag++;
						break;
					case 'D': /* DOTS document */
						DOTSflag++;
						break;
					default:
						errflg++;
						break;
				}
				break;
			default :
				errflg++;
				break;
		}
	}

	if(errflg)
		usage();

	if(((argc-optind) <1)){
		if(xflag || tflag )
			fflag=0,
			usefil = getcpy("-");	
		else if(cflag)
			fprintf(stderr,"capsar: no file name specified\n"),
			usage();
	}
	else 
		fflag++,
		usefil = argv[optind];

	

	if((DOTSflag + DDIFflag + Tflag + hflag) != 1 && xflag)
		usage();

	if(!xflag && (hflag || DOTSflag || DDIFflag))
		usage();

	if(!xflag && !cflag && !tflag)usage();
	if(xflag && cflag )usage();
	if(xflag && tflag)usage();

/*	capsar_setlogging(STDERR_LOGGING); */
	
	if(cflag){
		strcpy(filetag,usefil);
		m = capsar_create(usefil,NULL);
		if(!m)
			error(" create failed \n",NULL);

		if(capsar_unparse_file(m,fileno(stdout))== NOTOK){
			error(" unparse failed \n");
		}
		capsar_Destroy(m);
		exit(1);
	}

	if(fflag){
		if(strcmp(usefil,"-")==0)
			fd = fileno(stdin);
		else {
			if((fd = open(usefil,O_RDONLY)) == -1)
				error("cannot open %s \n",argv[1]);
		}
	}
	else if(!fflag)
		fd = fileno(stdin);

	m=capsar_parse_file(fd,NOTSMTP);

	if(!m)
		error(" message parsing failed ",NULL);

	if(capsar_limitations(m,mtype) == NOTOK)
		(void) error("illegal message type \n");

	if(tflag){
		fputs(mtype,stdout);
		fputs("\n",stdout);
		capsar_Destroy(m);
		exit(1);
	}	

	if(xflag && hflag){
		if(m->message_type == MAIL_MESSAGE){
			pp = capsar_get_header_list(m);
			if(pp == NULL)exit(1);
			pp0 = pp;
			while(pps = *pp++){
				fputs(pps,stdout);
				free(pps);
			}
		}
		else 
			(void) error(" bad mail message \n");
	
		capsar_Destroy(m);
		exit(0);
	}

	m0=m;
	while (m0 != NULL){
		if(Tflag){
			if(strcmp(m0->body_type,BODY_TYPE_DEF)==0)
				capsar_extract(m0,stdout);
		}

		else if(DOTSflag){
			if(strcmp(m0->body_type,DOTSTAG)==0)
				capsar_extract(m0,stdout);
		}

		else if(DDIFflag){
			if(strcmp(m0->body_type,DDIFTAG)==0)
				capsar_extract(m0,stdout);
		}
		m0 = m0->mm_next;
	}

	capsar_Destroy(m);
	
	
}
usage()
{
	fprintf(stderr,"USAGE: capsar [-c] [-t] [-x[hTD]] [file] \n");
	exit(0);
}


		


