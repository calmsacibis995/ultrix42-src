/*#@(#)mod.c	4.1	Ultrix	7/17/90*/
/****************************************************************************
 *									    *
 *  Copyright (c) 1984 by						    *
 *  DIGITAL EQUIPMENT CORPORATION, Maynard, Massachusetts.		    *
 *  All rights reserved.						    *
 * 									    *
 *  This software is furnished under a license and may be used and copied   *
 *  only in  accordance with  the  terms  of  such  license  and with the   *
 *  inclusion of the above copyright notice. This software or  any  other   *
 *  copies thereof may not be provided or otherwise made available to any   *
 *  other person.  No title to and ownership of  the  software is  hereby   *
 *  transferred.							    *
 * 									    *
 *  The information in this software is  subject to change without notice   *
 *  and  should  not  be  construed as  a commitment by DIGITAL EQUIPMENT   *
 *  CORPORATION.							    *
 * 									    *
 *  DIGITAL assumes no responsibility for the use  or  reliability of its   *
 *  software on equipment which is not supplied by DIGITAL.		    *
 * 									    *
$Header: mod.c,v 1.4 84/05/19 11:41:54 powell Exp $
 ****************************************************************************/
#include <stdio.h>
#define MODPATH	"MODPATH"	/* environment variable (check "MODPATH=") */
#define MODPATHFILE ".modpath"	/* name of file to source */

#define ARGSIZE 2048
#define FILENAMESIZE	256
#define MAXENVIRONNAMES	1024
#define MAXARGS		1024
int cflag = 0;
int gflag = 0;
int iflag = 0;
int mflag = 0;
int nflag = 0;
int Pflag = 0;
int pflag = 0;
int rflag = 0;
int Sflag = 0;
int tflag = 0;
int vflag = 0;
int rmpcd, rms;
char *ldlistv[MAXARGS] = {0}, **ldlist = ldlistv;
char *modoptv[MAXARGS] = {0}, **modopt = modoptv;
char *mscoptv[MAXARGS] = {0}, **mscopt = mscoptv;
char *ldoptv[MAXARGS] = {0}, **ldopt = ldoptv;
char modpath[ARGSIZE] = "";
char execfileb[FILENAMESIZE] = "a.out", *execfile = execfileb;
char *commandv[MAXARGS], **command;
char filename[FILENAMESIZE];
char mfilename[FILENAMESIZE];
char pfilename[FILENAMESIZE];
char sfilename[FILENAMESIZE];
char ofilename[FILENAMESIZE];
char lfilename[FILENAMESIZE];
char moddir[FILENAMESIZE] = MODLIBDIR;
int usedefaultdir = 0;
char progname[FILENAMESIZE];
extern char **environ;
char *newenv[MAXENVIRONNAMES];
enum filekind {modfile, deffile, pcdfile, asmfile, objfile, libfile};
enum filekind kind;
char *myname;
char *malloc(), *strcpy(), *strcat(), *mktemp();
extern int errno;
extern char *sys_errlist[];

char **addto(l,s) char **l, *s; {
    *l++ = s;
    return(l);
}
char *newname(s) char *s; {
     char *p;
     p = malloc(FILENAMESIZE);
     strcpy(p,s);
     return(p);
}
char *finddot(s) char *s; {
    while(*s) {
	if (*s == '.') return(s);
	s++;
    }
    return(NULL);
}
outfile(saveit,filename,root,ext) int saveit; char *filename, *root, *ext; {
     if (saveit) {
	strcpy(filename,root);
     } else {
	strcpy(filename,"/tmp/mod");
	strcat(filename,"XXXXXX");
	(void) mktemp(filename);
     }
    strcat(filename,ext);
}
initexec(progname) char *progname; {
    command = commandv;
    *command++ = progname;
}
addarg(arg) char *arg; {
    if (arg == 0 || *arg == '\0') {
	fprintf(stderr,"%s: addarg: null string\n",myname);
    } else {
	*command++ = arg;
    }
}
addargv(argv) char *argv[]; {
    while (*argv != 0) {
	*command++ = *argv++;
    }
}
execute(progfile) char *progfile; {
    int pid, status, i;
    *command++ = 0;
    if (vflag || nflag) {
	fprintf(stderr,"%s: ",progfile);
	for (i=0;commandv[i]!=0;i++) {
	    fprintf(stderr,"%s ",commandv[i]);
	}
	fprintf(stderr,"\n");
    }
    status = 0;
    if (!nflag) {
	if ((pid=fork())==0) {
	    execve(progfile,commandv,newenv);
	    fprintf(stderr,"%s: Cannot execute %s (%s)\n",myname,progfile,
		sys_errlist[errno]);
	    exit(1);
	}
	while(wait(&status)!=pid);
	if (vflag) {
	    fprintf(stderr,"%s: %s exited, status=%d\n",myname,progfile,status);
	}
    }
    if (!iflag && status != 0) {
	if ((status & 0xff)== 0) {
	    status = status >> 8;
	}
	exit(status);
    }
    return(status);
}
remove(file) char *file; {
    if (vflag || nflag) {
	fprintf(stderr,"rm %s\n",file);
    }
    if (!nflag) {
	if (unlink(file)!=0) {
	    fprintf(stderr,"%s: Cannot remove %s (%s)\n",myname,file,
		sys_errlist[errno]);
	}
    }
}
nomoreopt(o,argv) char *o, *argv; {
    if (*o != '\0') {
	fprintf(stderr,"%s: %s not understood in option %s\n",myname,o,argv);
	exit(1);
    }
}
main(argc,argv) int argc; char **argv; {
    char *ext, **env, scanfbuff[ARGSIZE];
    int i, status;

    FILE *modpathfile;
    myname = argv[0];
    if (argc <= 1) {
	fprintf(stderr,"usage: %s [options] files.mod\n",myname);
	exit(1);
    }
    env = environ;
    newenv[0] = modpath;
    i = 1;
    while (*env!=NULL) {
	if (strncmp(*env,"MODPATH=",8)==0) {
	    strcpy(modpath,*env);
	} else {
	    newenv[i] = *env;
	    i++;
	}
	env++;
    }
    newenv[i] = NULL;
    modpathfile = fopen(MODPATHFILE,"r");
    if (modpathfile != NULL) {
	if (fscanf(modpathfile," %s ",scanfbuff)!=1) {
	    fprintf(stderr,"Bad .modpath file\n");
	    exit(1);
	}
	strcpy(modpath,"MODPATH=");
	strcat(modpath,scanfbuff);
    }
    if (modpath[0]=='\0') {
	usedefaultdir = 1;
	strcpy(modpath,"MODPATH=.:");
	strcat(modpath,moddir);
    }
    strcat(moddir,"/");
    for(i=1;i<argc;i++) {
	if (argv[i][0]=='-') {
	    switch (argv[i][1]) {
	    case 'c':
		cflag = 1;
		nomoreopt(&argv[i][2],argv[i]);
		break;
	    case 'P':
		Pflag = 1;
		nomoreopt(&argv[i][2],argv[i]);
		break;
	    case 'S':
		Sflag = 1;
		nomoreopt(&argv[i][2],argv[i]);
		break;
	    case 'r':
		rflag = 1;
		nomoreopt(&argv[i][2],argv[i]);
		break;
	    case 'g':
		gflag = 1;
		modopt = addto(modopt,"-g");
		nomoreopt(&argv[i][2],argv[i]);
		break;
	    case 'i':
		iflag = 1;
		nomoreopt(&argv[i][2],argv[i]);
		break;
	    case 'p':
		pflag = 1;
		modopt = addto(modopt,"-pg");
		if (argv[i][2] == 'g') {
		    nomoreopt(&argv[i][3],argv[i]);
		} else {
		    nomoreopt(&argv[i][2],argv[i]);
		}
		break;
	    case 'u':
		modopt = addto(modopt,"-u");
		nomoreopt(&argv[i][2],argv[i]);
		break;
	    case 'l':
		ldlist = addto(ldlist,argv[i]);
		break;
	    case 'L':
		mscopt = addto(mscopt,"-L");
		nomoreopt(&argv[i][2],argv[i]);
		break;
	    case 'N':
		mscopt = addto(mscopt,argv[i]);
		break;
	    case 'm':
		mflag = 1;
		mscopt = addto(mscopt,argv[i]);
		break;
	    case 'M':
		mflag = 1;
		break;
	    case 'o':
		nomoreopt(&argv[i][2],argv[i]);
		if (i+1>=argc) {
		    fprintf(stderr,"%s: Missing file for -o\n",myname);
		    exit(1);
		}
		ext = finddot(argv[i+1]);
		if (ext!=NULL) {
		    if (strcmp(ext,".mod")==0) {
			ext = NULL;
		    } else if (strcmp(ext,".def")==0) {
			ext = NULL;
		    } else if (strcmp(ext,".pcd")==0) {
			ext = NULL;
		    } else if (strcmp(ext,".i")==0) {
			ext = NULL;
		    } else if ((ext[1]=='p'||ext[1]=='c'||ext[1]=='o'||
			    ext[1]=='s'|| ext[1]=='f') && ext[2]=='\0') {
			ext = NULL;
		    }
		    if (ext==NULL) {
			fprintf(stderr,"%s: -o would overwrite source file %s\n",
				myname,argv[i+1]);
			exit(1);
		    }
		}
		strcpy(execfile,argv[i+1]);
		i = i+1;
		break;
	    case 'D':
		if (argv[i][2]=='\0') {
		    strcpy(moddir,".");
		} else {
		    strcpy(moddir,&argv[i][2]);
		}
		if (usedefaultdir) {
		    strcpy(modpath,"MODPATH=.:");
		    strcat(modpath,moddir);
		}
		strcat(moddir,"/");
		break;
	    case 'v':
		fprintf(stderr,"%s\n",modpath);
		vflag = 1;
		nomoreopt(&argv[i][2],argv[i]);
		modopt = addto(modopt,"-v");
		mscopt = addto(mscopt,"-v");
		break;
	    case 'n':
		nflag = 1;
		nomoreopt(&argv[i][2],argv[i]);
		break;
	    case 't':
		tflag = 1;
		nomoreopt(&argv[i][2],argv[i]);
		modopt = addto(modopt,"-t");
		break;
	    case 'W':
		mscopt = addto(mscopt,argv[i]);
		break;
	    default:
		modopt = addto(modopt,argv[i]);
		break;
	    }
	} else {
	    processfile(argv[i]);
	}
    }
    if (!Pflag && !Sflag && !tflag) {
	strcpy(lfilename,moddir);
	strcat(lfilename,"modlib");
	if (pflag) {
	    strcat(lfilename,"_p");
	}
	if (mflag) {
	    initexec("mod2.2");
	    addargv(mscoptv);
	    addargv(ldlistv);
	    strcpy(progname,moddir);
	    strcat(progname,"mod2.2");
	    status = execute(progname);
	}
	if (!cflag) {
	    initexec("ld");
	    addargv(ldoptv);
	    addarg("-X");
	    addarg("-o");
	    addarg(execfile);
	    if (pflag) {
		addarg("/usr/lib/gcrt0.o");
	    } else {
		addarg("/lib/crt0.o");
	    }
	    addargv(ldlistv);
	    addarg(lfilename);
	    if (gflag) {
		addarg("-lg");
	    }
	    if (pflag) {
		addarg("-lpc_p");
		addarg("-lm_p");
		addarg("-lc_p");
	    } else {
		addarg("-lpc");
		addarg("-lm");
		addarg("-lc");
	    }
	    status = execute("/bin/ld");
	    if (status != 0) exit(status);
	}
    }
    exit(0);
}
processfile(file) char *file; {
    char *ext, *pcdext, *phase;
    int status;

    if (tflag) {
	pcdext = ".i";
    } else {
	pcdext = ".pcd";
    }
    strcpy(filename,file);
    ext = finddot(filename);
    if (ext == NULL) kind = libfile;
    if (strcmp(ext,".mod")==0) kind = modfile;
    else if (strcmp(ext,".o")==0) kind = objfile;
    else if (strcmp(ext,pcdext)==0) kind = pcdfile;
    else if (strcmp(ext,".s")==0) kind = asmfile;
    else if (strcmp(ext,".def")==0) kind = deffile;
    else kind = libfile;
    
    if (kind != libfile) {
	*ext = '\0';
    }
    switch(kind) {
    case modfile:
	strcpy(mfilename,filename);
	strcat(mfilename,".mod");
	outfile(Pflag||rflag||tflag,pfilename,filename,pcdext);
	outfile(Sflag||rflag,sfilename,filename,".s");
	outfile(1,ofilename,filename,".o");
	break;
    case pcdfile:
	outfile(1,pfilename,filename,pcdext);
	outfile(Sflag||rflag,sfilename,filename,".s");
	outfile(1,ofilename,filename,".o");
	break;
    case asmfile:
	outfile(1,sfilename,filename,".s");
	outfile(1,ofilename,filename,".o");
	break;
    case objfile:
	outfile(1,ofilename,filename,".o");
	break;
    }
    rmpcd = 0;
    rms = 0;
    switch (kind) {
    case modfile:
	if (tflag) {
	    phase = "mod2.0t";
	} else {
	    phase = "mod2.0";
	}
	initexec(phase);
	addargv(modoptv);
	addarg("-o");
	addarg(pfilename);
	addarg(mfilename);
	strcpy(progname,moddir);
	strcat(progname,phase);
	status = execute(progname);
	if (Pflag || status != 0) return;
	rmpcd = !rflag;
    /* fall through to next phase */
    case pcdfile:
	if (tflag) {
	    phase = "mod2.1t";
	} else {
	    phase = "mod2.1";
	}
	initexec(phase);
	addarg(pfilename);
	if (!tflag) {
	    addarg(sfilename);
	}
	strcpy(progname,moddir);
	strcat(progname,phase);
	status = execute(progname);

	if (tflag) return;
	if (rmpcd) {
	    remove(pfilename);
	}
	if (Sflag || status != 0) return;
	rms = !rflag;
    /* fall through to next phase */
    case asmfile:
	initexec("as");
	addarg("-o");
	addarg(ofilename);
	addarg(sfilename);
	status = execute("/bin/as");
	if (rms) {
	    remove(sfilename);
	}
	if (status != 0) return;
    /* fall through to next phase */
    case objfile:
	ldlist = addto(ldlist,newname(ofilename));
	break;
    case libfile:
	ldlist = addto(ldlist,file);
	break;
    }
}
