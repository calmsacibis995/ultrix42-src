# ifndef lint
static char *sccsid = "@(#)opser.c	4.2	(ULTRIX)	11/15/90";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * opser.c
 *
 *	static char Sccsid[] = "@(#)opser.c     1.0     10/31/83";
 *
 *	Modification History:
 *	
 *	3/27/87 - fries
 *		Changed str[50] to str[MAXPATHLEN] to allow
 *		enough room for staging file name.
 *
 *      2/11/87 - fries
 *		Added code to confirm remote system "halt",
 *		disconnect socket connection if halt requested
 *		and return to Local Opser mode.
 *	
 *      1/28/87 - fries
 *		Removed socket lost timer. Added code to
 *		to perform keep alive messages on the socket.
 *	
 *     12/31/86 - fries
 *		Added code to get directory name on staging
 *		remote backup. Changed Slave System shutdown
 *		time from 5 to 3 minutes.
 *
 *     12/30/86 - fries
 *		Added code to enable echoing on the controlling
 *		terminal.
 *
 *     11/11/86 - fries
 *		Added code to insure that remote backup doesn't
 *		overwrite local file systems when dumping to  a
 *		direct device.
 *
 *     10/31/86 - fries
 *		Moved position where /etc/opseractive is created.
 *
 *     10/20/86 - fries
 *		Added code to allow /dev/ttyv0 as the console device.
 *
 *	9/25/86 - fries
 *		Added code to disallow the use of ^D to exit opser or
 *		remote opser. Added code to warn before quitting out of
 *		remote opser.
 *
 *	9/04/86 - fries
 *		Added code to put correct date at end of staging file.
 *
 *	9/04/86 - fries
 *		Added code to retry socked bind and modified textual
 *		messages to read as "Master /Slave".
 *
 *	8/22/86 - dike
 *		Added shell script support.
 *
 *      8/08/86 - dike
 *              Added support for network opser code.
 *
 *	4/28/86 - bjg
 *		Added option to umount file systems, as this is not done
 *		automatically upon shutdown, and should be done before
 *		fsck is run.
 *
 */
#include "opser.h"

struct fstab *p_fstab, *getfsent();
struct hostent *gethostbyname();
struct sgttyb tty, b_save;
struct tchars ctty;
struct stat statb;
unsigned ttype;
FILE *tfd, *flagf;
int got_tty = 0;
int debug = 0;
int valid;
int sflag = 0;
int sock_fd, pty_fd, tty_fd, script_fd = -1;
int remote_flag = 0,tilde_flag = 0;
char instr[80], new[80], *args[10];
char last_client_line[256];
#if defined(vax) || defined (mips)
char	buf[BUFSIZ];
char fsck[]="/etc/fsck";	/*command to do fsck*/
char arg1[]="-p";		/*argument for fsck*/
char multi[]="\004";		/*command to go multi-user*/
char active[]="/etc/opseractive";	/*file to flag opser active*/
char remote[] = "/etc/remoteopser";
int status, xpid, pid, client_sock;
struct 	sgttyb tty;
struct 	tchars ctty;
unsigned ttype;
#endif
#ifdef ultrix11
char instr[80], oustr[80], backfile[80];
#endif ultrix11

char **cmd;
char *cmds[] =
{	
	"help",
	"?",
	"users",
	"shutdown",
	"backup",
	"restart",
	"exit",
	"quit",
	"bye",
	"!sh",
	"fsck",
	"halt",
	"dismount",
	"network",
	0
};

char *network_cmds[] =
{	
	"help",
	"?",
	"backup",
	"restart",
	"exit",
	"quit",
	"bye",
	"!sh",
	"lsh",
	"fsck",
	"halt",
	"dismount",
	"size",
	"password",
	0
};

char	client_name[80] = "";
void	socket_alarm();
void	shutdown_alarm();
char	*ttynam;
char	*ttyname();
char	alt_cnsle[] = {"/dev/ttyv0"};
char	cnsle[] = {"/dev/console"};
char	*tsns = "\nCommand invalid unless time-sharing stopped\n";
#ifdef ultrix11
int	hltcode[4] = {
	0102, 0, 0106, 0};
char *proced, *bname;
int dirfile, found;
struct direct dir;
struct nlist nl[] = {{"_lks"},
		     {"trap"},
		     {""}
		     };
#endif ultrix11

/* This is the main Opser code */
main(argc,argv)
	int argc;
	char *argv[];
{
	extern onintr();

    /* If not Superuser ID, then do not allow to run */
    if(getuid()){
	printf("\7\7\7You must have the root uid to run Opser\n");
	exit(0);
    }	

    ioctl(1, TIOCGETP, &tty);		/* get terminal param's */
    ioctl(1, TIOCGETC, &ctty);		/* get terminal special */
					/* characters           */
    tty.sg_erase = '\177';		/* set erase char. to   */
					/* the DELETE key       */
    tty.sg_kill = '\025';		/* set ^U to kill char. */
    tty.sg_flags &= ~CBREAK;		/* turn off CBREAK      */
    tty.sg_flags &= ~RAW;		/* turn off RAW         */
    tty.sg_flags |= ECHO;		/* turn on echoing      */
    ctty.t_intrc = '\003';		/* ^C signals SIGINT    */
    ioctl(0, TIOCSETC, &ctty);		/* set new special's    */
    ioctl(0, TIOCSETP, &tty);		/* set new parameters   */
    ttype = LPRTERA;
#if defined(vax) || defined(mips)
    ioctl(1, TIOCLBIS, &ttype);
#endif
#ifdef ultrix11
    ioctl(1, TIOCSETT, &ttype);
#endif ultrix11
    signal(SIGINT, onintr); 		/*specify interrupt routine*/

    /*print out header*/
#if defined(vax) || defined(mips)
	/* Check for existence of /etc/remoteopser file    */
	/* If it's there, then go into remote mode         */
	if (access(remote,0) == 0)
	   remote_opser();

	/* If "opser -s" then create file /etc/remoteopser */
	if((argc > 1)&& !strcmp(argv[1],"-s")){
	   if((flagf = fopen(remote,"w")) == 0 ){
		perror("Can't open /etc/remoteopser");
		opser_exit(1);
	   }
	   fprintf(flagf,"%s %s %s",argv[2],argv[3],argv[4]);
	   fclose(flagf);
	   sflag = 1;
	   sdown();
	}
    printf("\nULTRIX-32 Operator Services\n");
#endif
#ifdef ultrix11
    printf("\nULTRIX-11 Operator Services\n");
#endif ultrix11
    printf("\nLine editing: delete - erase one character, ");
    printf("^U - kill entire line\n");
    printf("\nFor help, type h followed by a return\n");
    menu();
}

/* This code requests an Opser command and dispatches */
/* to handle the requested command.                   */
menu()
{
    char c,**commands,command_buf[80];
    int pntr,i,first_arg;

    while(1){
get_cmd:
	/* If Local Opser */
	if(*client_name != '\0'){
	    commands = network_cmds;
	    printf("\n%s_opr> ",client_name);
	}
	/* Else Remote Opser */
	else {
	    commands = cmds;
	    printf("\n%sopr> ",client_name);
	}
	for(pntr = 0;; pntr++){
#if defined(vax) || defined(mips)
	    clearerr(stdin);
	    if(fread(&c, 1, 1, stdin) == NULL && (!ferror(stdin))){
#endif
#ifdef ultrix11
	    if(read(0,&c,1) == 0){
#endif ultrix11
	    printf("\n\n\7\7\7Invalid Command - type ");
	    if(*client_name == '\0')
              printf("\"q\" to quit");
	    else{
	      printf("\"r\" to restart slave system\n");
              printf("or \"q\" to leave Slave System shut down");
            }
	    goto get_cmd;
	    }

	    if(tilde_flag){
		instr[pntr-1] = '~';
		tilde_flag = 0;
	    }
	    if(c == '~') tilde_flag = 1;
	    else if(c == '\n') break;
	    if(!tilde_flag) instr[pntr] = c;	/*place request into instr[]*/
	}
	instr[pntr] = c = '\0'; 	/*terminate request*/

	/* Parse the command */
	i = parse(instr,commands,pntr);

	/* If Remote Opser */
	if(*client_name != '\0'){
	    switch(i){
		case N_BACKUP:
		    net_backup(args);
		    break;
		case N_RESTART:
		    net_restart(args);
		    break;
		case N_HELP:
		case N_QUESTION:
		    net_help();
		    break;
		case N_EXIT:
		case N_QUIT:
		case N_BYE:
		    do{
		       printf("\nThis will leave the Slave System shut down in single user mode.\n");
                       printf("Do you really want to do this ?(Enter y/n) ");
                       c = getchar();
                    }while((c != 'y') && (c != 'n'));
		    getchar();
		    if (c == 'n')
                        break; 
		    printf("Leaving network mode.\n");
		    client_name[0] = '\0';
    		    shutdown(client_sock,2);
		    close(client_sock);
		    break;
		case N_SHELL:
		    if(args[0] == NULL){
 			if(send_command("password",args) != 0){
 		            printf("\nIncorrect password.\n");
 			    break;
 		        }
 		        args[0] = "sh";
 		        args[1] = NULL;
		        printf("\ntype ^D to return to opser\n");
 		        send_command("!sh",args);
		    }
 		    else {
 			if((script_fd = open(args[0],O_RDONLY)) == -1){
 			    perror("Couldn't open file");
 			}
 			else {
 			    args[0] = "sh";
 			    args[1] = NULL;
 			    send_command("!sh",args);
 			}
 		    }
		    break;
		case N_L_SHELL:
		    if(args[0] == NULL){
			if(!get_passwd()){
		            printf("\nIncorrect password.\n");
			    break;
		        }
		        printf("type ^D to return to opser\n");
		        if(system("sh") == 127)
			    printf("Couldn't execute shell\n");
		    }
		    else {
			sprintf(command_buf,"sh < %s",args[0]);
		        if(system(command_buf) == 127)
			    printf("Couldn't execute shell\n");
		    }
		    break;
		case N_FSCK:
		    args[0] = "fsck";
		    args[1] = "-p";
		    args[2] = NULL;
		    send_command("fsck",args);
		    break;
		case N_HALT:
		    args[0] = NULL;
		    do{
		       printf("\nThis will leave the Slave System halted. You must then manually reboot\nthe Slave System.\n");
                       printf("Do you really want to do this ?(Enter y/n) ");
                       c = getchar();
                    }while((c != 'y') && (c != 'n'));
		    getchar();
		    if (c == 'n')
                        break; 
		    args[0] = NULL;
		    send_command("halt", args);
		    printf("Leaving network mode.\n");
		    client_name[0] = '\0';
    		    shutdown(client_sock,2);
		    close(client_sock);
		    break;
		case N_DISMOUNT:
		    args[0] = "umount";
		    args[1] = "-a";
		    args[2] = NULL;
		    send_command("dismount",args);
		    break;
		default:
		    printf("\n\7\7\7Invalid command\n");
		    printf("For help type h");
		    break;
	    }
	}
	else {
	    switch(i){
		case USERS:
		    users();
		    break;
		case SHUTDOWN:
		    sdown();
		    break;
		case BACKUP:
		    backup();
		    break;
		case RESTART:
		    restart();
		    break;
		case HELP:
		case QUESTION:
		    help();
		    break;
		case EXIT:
		case QUIT:
		case BYE:
		    goodbye();
		    break;
		case SHELL:
		    if(!get_passwd()){
		        printf("\nIncorrect password.\n");
			break;
		    }
		    printf("type ^D to return to opser\n");
		    if(system("sh") == 127) printf("Couldn't execute shell\n");
		    break;
		case FSCK:
	            do_fsck();
	            break;
		case HALT:
		/*if files found, time-sharing
		 *is already off so nothing to
		 *do; else halt time-sharing
		 */
#if defined(vax) || defined(mips)
		    if(access("/etc/sdnologin",0))
#endif
#ifdef ultrix11
		    if(access("/etc/sdloglock", 0) && access("/etc/loglock",0))
#endif ultrix11
			printf("%s", tsns);
		    else haltsys();
		    break;
	        case DISMOUNT:	/* dismount */
			/* supported for vax only */
		    dismount();
		    break;
	        case NETWORK:
	            network(args);
		    break;
	        default:
		    /*no match try again*/
		    printf("\n\7\7\7Invalid command\n");
		    printf("For help type h");
		    break;
	    }
	}
    }
}

/* This function creates a staging file name */
/* as systemname.~.date_time_stamp           */
get_filename(dir,filesys,filename)
char *dir,*filesys,*filename;
{
    char date[30];
    int len;
    
    strcpy(filename,dir);
    strcat(filename,"/");
    strcat(filename,client_name);
    strcat(filename,".");
    len = strlen(filename);
    strcat(filename,filesys);
    for(;len < strlen(filename);len++)
        if(filename[len] == '/') filename[len] = '~';
    strcat(filename,".");
    tm(date);
    strcat(filename,date);
}

/* Open and pre-allocates space for remote dump staging file */
get_big_file(filename,size)
char *filename;
int size;
{
    struct stat fileinfo;
    int fd;

    printf("\nAllocating %d bytes.\n",size);
    if((fd = open(filename,O_WRONLY | O_CREAT,0777)) < 0){
	perror("Can't open staging file");
	return(-1);
    }
    stat(filename,&fileinfo);
    while(size > 0){
	if(lseek(fd,fileinfo.st_blksize-1,L_XTND) == -1) return(-1);
	if(write(fd,"\000",1) == -1) return(-1);
 	size -= fileinfo.st_blksize;
    }
    close(fd);
    return(0);
}

/* Command handling code for Remote Opser "backup" command */
net_backup(args)
char **args;
{
    int no_more_args = 0, can_leave, backup_level, staging,error,size;
    char buf[80],file[80],filesys[80],hostname[256],*backup_level_str = " ";
    char *staging_str = " ",*level_str = " uf",str[MAXPATHLEN];
    char filename[256];
    
    if(args[0] != NULL){
	if((!isdigit(**args)) || (args[0][1] != '\0')){
	    printf("Invalid backup level.  Must be between 0 and 9.\n");
	    return;
	}
	else backup_level = **args - '0';
    }
    else {
	no_more_args = 1;
	while(1){
	    printf("Enter backup level (0 - 9): ");
	    gets(buf);
	    if((!isdigit(*buf)) || (buf[1] != '\0')){
	        printf("Invalid backup level.  Must be between 0 and 9.\n");
	    }
	    else {
		can_leave = 1;
		backup_level = *buf - '0';
		break;
	    }
	}
    }
    if(args[1] == NULL) no_more_args = 1;
    if(!no_more_args){
	if(args[1][1] == '\0'){
	    if(tolower(args[1][0]) == 's') staging = 1;
	    else if(tolower(args[1][0]) == 'd') staging = 0;
	    else error = 1;
	}
	else error = 1;
	if(error){
	    printf("Invalid staging switch.  Must be 's' or 'd'.\n");
	    return;
	}
    }
    else {
	while(1){
	    error = 0;
	    printf("Use staging file or go directly to device\
(s = staging/d = direct)? ");
	    gets(buf);
	    if(buf[1] == '\0'){
	        if(tolower(buf[0]) == 's') staging = 1;
	        else if(tolower(buf[0]) == 'd') staging = 0;
	        else error = 1;
	    }
	    else error = 1;
	    if(error) printf("Invalid choice.  Must be 's' or 'd'.\n");
	    else break;
	}
    }
    if(args[2] == NULL) no_more_args = 1;
    if(!no_more_args) strcpy(file,args[2]);
    else {
	if(staging)
	{
	  printf("Enter directory to place staging file: ");
	  gets(file);
	}
	else
	{
	     valid = 0;
	     do{
		printf("Enter device special file name: ");

	        /* Get name of remote dump device */
	        gets(file);

		/* See if file can be stat'd... */
        	if(stat(file, &statb)< 0){
		  printf("\7Device special file \042%s\042 not found\n", file);
		  continue;
		}

		/* If file found but not a special file... */
		if((statb.st_mode & S_IFMT) != S_IFCHR  &&
		  (statb.st_mode & S_IFMT) != S_IFBLK){
		  printf("\7You must enter the name of a special file\n");
		  continue;
		}

		/* If not a tape device... */
		if( strcmp(file, "/dev/rmt") != 0 &&
		   strcmp(file,"/dev/nrmt") != 0){

		   /* If a character special, then make it block */
		   if((statb.st_mode & S_IFMT) == S_IFCHR)
		      mkblk(file);
		   else strcpy(new, file);

		   setfsent();
		   p_fstab = getfsent();
		   do{
		   if(p_fstab){
		     if(strcmp(p_fstab->fs_spec, new) == 0){
			printf("\7Can not dump to \042%s\042 as it is an entry in \042/etc/fstab\042\n", file);
			break;
		     }
		   }
		   p_fstab = getfsent();
		   }while(p_fstab);
		if(p_fstab)continue;
		}
		valid++;
       }while(!valid);
    }
    if(args[3] == NULL) no_more_args = 1;
    if(!no_more_args) strcpy(filesys,args[3]);
    else {
	printf("Enter name of filesystem to dump: ");
	gets(filesys);
    }
    level_str[0] = backup_level + '0';
    gethostname(hostname,256);
    strcat(hostname,":");
    if(staging){
        get_filename(file,filesys,filename);
	strcat(hostname,filename);
	level_str[1] = 'S';
	args[0] = "dump";
        args[1] = level_str;
        args[2] = hostname;
        args[3] = filesys;
        args[4] = NULL;
	printf("\nSizing file system to determine number of bytes to pre-allocate.\n");
        if((size = send_command("size",args)) != 1) return;
	sscanf(last_client_line,
	       "  DUMP: Estimated %d bytes output to file named %s",&size,str);
	level_str[1] = 'u';
        if(get_big_file(filename,size) == -1){
	    printf("\n\007Couldn't allocate %d bytes.\n",size);
	    unlink(filename);
	    return;
        }
    }
    else strcat(hostname,file);
    args[0] = "rdump";
    args[1] = level_str;
    args[2] = hostname;
    args[3] = filesys;
    args[4] = NULL;
    send_command("backup",args);
}   
}

get_args(command)
char *command;
{
    char *ptr = command;
    int i = 0;

    while(1){
	while(isspace(*ptr)) ptr++;
	if(*ptr == '\0') break;
	args[i++] = ptr;
	while((!isspace(*ptr)) && (*ptr != '\0')) ptr++;
	if(*ptr == '\0') break;
	*ptr++ = '\0';
    }
    args[i] = NULL;
}   

/* Command handling code for Remote Opser "network" command */
network(args)
char **args;
{
    char rsh_str[300],hostname[256],c;
    int on = 1, s,size,readmask,retries,success;
    u_short temp_port;
    struct sockaddr_in name,addr,sock_name;
    struct hostent *host;
    struct timeval time;

    /* Socket Timeout */
    signal(SIGALRM,shutdown_alarm);

    if(args[0] == NULL){
	printf("Enter Slave System name: ");
	gets(client_name);
    }
    else strcpy(client_name,args[0]);

    /* Check /etc/hosts for cline name entry */
    if(gethostbyname(client_name) == NULL){
	printf("\7Slave system name not found in /etc/hosts file\n");
	client_name[0] = '\000';
	signal(SIGALRM,0);
	return;
    }

    /* Shut down Slave System */
    printf("Network:  slave = %s.\n",client_name);
    if((s = socket(AF_INET, SOCK_STREAM, 0)) == -1){
	perror("Can't create socket");
	client_name[0] = '\0';
	return;
    }
    gethostname(hostname,256);
    host = gethostbyname(hostname);
    sock_name.sin_family = host->h_addrtype;
    temp_port = (u_short) (5197 + getpid());
    temp_port = (temp_port % 31742) + 1025;
    sock_name.sin_port = temp_port;
    sock_name.sin_addr.s_addr = INADDR_ANY;
    retries = 0;
    success = 0;
    while((retries < 10) && !success){
        if(bind(s,&sock_name,sizeof(struct sockaddr_in)) != -1){
            success++;
	    break;
	}
	retries++;
	sock_name.sin_port = ((sock_name.sin_port + 1) % 31742) + 1025;
    }
    if(!success){
	perror("Couldn't bind socket");
	client_name[0] = '\0';
	return;
    }
    size = sizeof (struct sockaddr_in);
    if(getsockname(s,&name,&size) == -1){
	perror("Can't get socket name");
	client_name[0] = '\0';
	return;
    }
    retries = 0;
    success = 0;
    while((retries < 3) && !success){
	sprintf(rsh_str,"rsh %s echo \'Opser shutting down system\' \"|\"wall",
	        client_name);
	system(rsh_str);
        sprintf(rsh_str,"rsh %s /opr/opser -s %d %d %d",client_name,
    					    sock_name.sin_family,
					    *((int *) host->h_addr),
					    sock_name.sin_port);
	if(system(rsh_str) != 127) success++;
	retries++;
    }
    if(!success){
	printf("Couldn't connect to Remote System.\n");
	return;
    }

    /* enable shutdown timeout timer */
    alarm(SHUTDOWN_TIMEOUT);

    if(listen(s,5) == -1){
	perror("Can't listen");
	client_name[0] = '\0';
	return;
    }
    size = sizeof (struct sockaddr_in);
    if((client_sock = accept(s,&addr,&size)) == -1){
	perror("Can't accept");
	client_name[0] = '\0';
	return;
    }

    /* Set socket keep alive...insures socket connection */
    if(setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)) < 0){
	    perror("Can't set socket keep alive option\n");
            client_name[0] = '\0';
    }

    /* Set signal handler for lost socket connection */
    signal(SIGPIPE, socket_alarm);

    time.tv_sec = 0;
    time.tv_usec = 0;
    readmask = 1 << client_sock;
    select(client_sock+1,&readmask,0,0,&time);
    if(readmask){
	read(client_sock,&c,1);
	printf("Garbage in select: %c\n",c);
    }

    /* disable shutdown timeout timer */
    alarm(0);
}

/* Command handling code for Local Opser "dismount" command */
dismount(){
#if defined(vax) || defined(mips)
    if(access("/etc/sdnologin",0)) printf("%s", tsns);
    else {
	pid = fork();
	if (pid == 0) { /*if child exe command*/
	    execl("/etc/umount","/etc/umount", "-a", (char *)0);
	    printf("Unable to execute /etc/umount.\n");
	    opser_exit(1);
		}
		while ((xpid = wait((int *)0) != pid)) ;
    }
#endif
}    

/* Command handling code for Local Opser "fsck" command */
do_fsck(){
#if defined(vax) || defined(mips)
    if(access("/etc/sdnologin",0))
#endif
#ifdef ultrix11
    if(access("/etc/sdloglock", 0) && access("/etc/loglock", 0))
#endif ultrix11
	printf("%s", tsns);
    else {
#if defined(vax) || defined(mips)
		/*fork off child to exec fsck; parent
		 *waits until child is done.
		 */
	pid = fork();
	if (pid == 0) { /*if child exe command*/
	    execl(fsck, fsck, arg1, (char *)0);
	    printf("Unable to execute command.\n");
	    opser_exit(1);
	}
	while ((xpid = wait((int *)0) != pid)) ;
    }
#endif
#ifdef ultrix11
    system("/bin/fsck -t /tmp/fcheck");
#endif ultrix11
}    

/* Command handling code for Local Opser "help" command */
help(){
    printf("\n() - may use first letter in place of full name\n");
    printf("Valid commands for Local Opser are:\n\n");
#if defined(vax) || defined(mips)
    printf("!sh\t\t- shell escape (execute ULTRIX-32 commands)\n");
#endif
#ifdef ultrix11
    printf("!sh\t\t- shell escape (execute ULTRIX-11 commands)\n");
#endif ultrix11
    printf("\t\t  (Type control d to return from shell)\n");
    printf("(u)sers\t\t- show logged in users\n");
    printf("(s)hutdown\t- stop time-sharing\n");

#if defined(vax) || defined(mips)
    printf("(d)ismount\t- unmount file systems\n");
#endif
    printf("(f)sck\t\t- file system checks\n");
    printf("(r)estart\t- restart time-sharing\n");
    printf("(h)elp\t\t- print this help message\n");
#if defined(vax) | defined(mips)
    printf("(b)ackup\t- file system backup\n");
#endif
#ifdef ultrix11
    printf("backup cfn\t- file system backup\n");
    printf("\t\t  (cfn = command file name)\n");
#endif ultrix11
    printf("halt\t\t- halt processor\n");
    printf("(n)etwork [Slave] - initiate Remote Opser\n");
    printf("(q)uit\t\t- exit from opser");
    printf("\n");
}

/* Command handling code for Remote Opser "help" command */
net_help(){
    printf("\n() - may use first letter in place of full name\n");
    printf("Valid commands for Remote Opser are:\n\n");
#if defined(vax) || defined(mips)
    printf("!sh\t\t- Slave System shell escape (execute ULTRIX-32 commands)\n");
#endif
#ifdef ultrix11
    printf("!sh\t\t- Slave System shell escape (execute ULTRIX-11 commands)\n");
#endif ultrix11
#if defined(vax) || defined(mips)
    printf("lsh\t\t- Master System shell escape (execute ULTRIX-32 commands)\n");
#endif
#ifdef ultrix11
    printf("lsh\t\t- Master System shell escape (execute ULTRIX-11 commands)\n");
#endif ultrix11
    printf("\t\t  (Type control d to return from shell)\n");
#if defined(vax) || defined(mips)
    printf("(d)ismount\t- unmount file systems\n");
#endif
    printf("(f)sck\t\t- file system checks\n");
    printf("(r)estart\t- restart time-sharing\n");
    printf("(h)elp\t\t- print this help message\n");
#if defined(vax) || defined (mips)
    printf("(b)ackup\t- file system backup\n");
#endif
#ifdef ultrix11
    printf("backup cfn\t- file system backup\n");
    printf("\t\t  (cfn = command file name)\n");
#endif ultrix11
    printf("halt\t\t- halt processor\n");
    printf("(q)uit\t\t- exit Remote Opser(do not restart Slave System)");
    printf("\n");
}

/* Command handling code for Local Opser "users" command */
users(){
    printf("The following users are logged in :\n\n");
    system("who");
}
#if defined(vax) || defined(mips)
#define START	1			/*start of time string*/

char shut[]="/etc/shutdown";
char option[]="-o";			/*specifies being done from opser*/
char shutdowntime[80]="+";			/*default time for shutdown*/
char sdlock[]="/etc/sdnologin";
#endif

/* Command handling code for Local Opser "shutdown" command */
sdown()
{					/*if time-sharing already stop do
					 *nothing; else shutdown time-sharing.
					 */
#if defined(vax) || defined(mips)
FILE *fopen();
int i,c,deltim;

	if(access("/etc/sdnologin",0) == 0){
#endif
#ifdef ultrix11
	if(access("/etc/sdloglock", 0) == 0
	    || access("/etc/loglock", 0) == 0){
#endif ultrix11
		fprintf(stdout, "\nTime-sharing already stopped\n");
		return;
	}
#if defined(vax) || defined(mips)
	ttynam = ttyname(0);
	if(sflag){
		printf("\7\7\7\nRemote Opser initiated Shutdown Started\n");
		printf("System will shut down in 5 minutes !!!\n");
	}
#ifdef vax
	else if (strcmp(cnsle,ttynam) && strcmp(alt_cnsle,ttynam)){
		printf("\nShutdown can only be run from the console device\n");
		return;
	}
#endif vax
#ifdef mips
	printf("\nShutdown should always be run from the system console\n");
#endif mips
	printf("The following users are logged into the system.\n\n");
	system("who");
	if(!sflag)
	for(;;){
		printf("\nHow many minutes until shutdown [1-99] ? ");
		for (i = 0; (c = getchar()) != '\n'; ++i)
			shutdowntime[START+i] = c;
		if (i){
			shutdowntime[START+i] = '\0';		/*terminate string*/
			deltim = atoi(&shutdowntime[0]);
			if (deltim > 99 || deltim < 1){
				printf("Improper time specified.\n");
				continue;
			}
		}
		else {
			shutdowntime[START] = '0\0';		/*default time*/
		}
		break;
	}
	else 
		shutdowntime[START] = '3';       /* Remote opser initiated shutdown */
		shutdowntime[START+1] = '\0';       /* Remote opser initiated shutdown */

        /* Create the "/etc/opseractive" file prior to shutdown    */
	/* to signal init.c to bring up opser when system restarts */
        if ((flagf = fopen(active,"w")) == 0) {
	    perror("Can't open /etc/opseractive");
	    opser_exit(1);
        }
        else fclose(flagf);	

	/* Create "/etc/sdnologin" file prior to shutdown */
	/* to signal time sharing stopped...              */
	fclose(fopen(sdlock,"w"));

	/* Sync in-memory disk info to disk */
	sync();

	/* Perform an "/etc/shutdown -o" to take place system */
	/* in single user mode.                               */
	execl(shut, shut, option, shutdowntime, (char *)0);

	system("rm -f /etc/sdnologin"); /*if take down not successful, then
					 *remove /etc/sdnologin
					 */
#endif
#ifdef ultrix11
	system("/opr/shutdown");
	sync();
#endif ultrix11
}

/* Command handling code for Local Opser "backup" command */
backup()
#if defined(vax) || defined(mips)
{
    system("/bin/sh /opr/backup");	/*execute backup script*/
}
#endif
#ifdef ultrix11
{
    /*set proced=beginning of request; skip
     *over "backup";if no backup file
     *specified then send err msg & return.
     */
    for(proced = instr; *proced != ' '; proced++){
	if(*proced == '\0'){
	    printf("\nBackup requires command file name\n");
	    printf("e.g.: backup daily or backup monthly\n");
	    return;
	}
    }
    proced++;
    strcpy(&backfile, "/opr/");
    strcat(&backfile, proced);
    strcat(&backfile, ".bak");
    if(access(&backfile, 0)){
	printf("%s not found\n", proced);
	if((dirfile = open(".", 0)) <= 0){
	    printf("Cannot open '.'\n");
	    return;
	}
	printf("Existing backup command file are :\n");
	found = 0;
	while((read(dirfile, &dir, sizeof(struct direct)))
    		== sizeof(struct direct)){
	    if(dir.d_ino == 0) continue;
	    if(bname = index(dir.d_name, '.')){
	        if(strcmp(bname, ".bak")) continue;
	        *bname = '\0';
	        printf("%s\n", dir.d_name);
	        found++;
	    }
        }
        if(found == 0)
        printf("No valid backup command files!\n");
        close(dirfile);
        return;
    }
    strcpy(&oustr, "sh ");
    strcat(&oustr, &backfile);
    system(&oustr);
}
#endif ultrix11

/* Command handling code for Remote Opser "restart" command */
net_restart(args)
char **args;
{
    args[0] = NULL;
    send_command("restart",args);
    client_name[0] = '\0';
    shutdown(client_sock,2);
    close(client_sock);
}

/* Command handling code for Local Opser "restart" command */
restart(){
#if defined(vax) || defined(mips)
			/*if time-sharing has been
			 *stopped, then restart it.
			 *else nothing to do.
			 */
    if(access("/etc/sdnologin",0)){
        printf("%s", tsns);
	return;
    }

    if(access("/etc/sdloglock", 0) && access("/etc/loglock", 0))
	printf("%s", tsns);
#endif
    else
#if defined(vax) || defined(mips)
	ttynam = ttyname(0);
	if ((!remote_flag) && (strcmp(cnsle, ttynam) && strcmp(alt_cnsle, ttynam))){
	    printf("\nRestart can only be run from the console device\n");
	    return;
	}
	if(remote_flag){
	   close(sock_fd);
	   close(pty_fd);
	   close(tty_fd);
	}   
	opser_exit(0); 			/*terminates opser and brings
					 *system to mutli-user.
					 */
#endif
#ifdef ultrix11
	system("sh /opr/restart");
#endif ultrix11
}

/* Control-C interrupt handling code */
onintr()
{
    signal(SIGINT, onintr);
    fflush(stdout);
    if((client_name[0] == '\0') || tilde_flag){
	tilde_flag = 0;
	sleep(1);
        menu();
    }
    else {
	send(client_sock,"\003",1,MSG_OOB);
	if(debug){
	  printf("Sending a control-C\n");
	  fflush(stdout);
	}
    }
}

/* Opser Terminating code */
goodbye(){
#if defined(vax) || defined(mips)
				/*if time-sharing is stopped
				 *tell operator
				 */
    if(access("/etc/sdnologin",0) == 0){
#endif
#ifdef ultrix11
    if(access("/etc/sdloglock", 0) == 0 || access("/etc/loglock", 0) == 0){
#endif ultrix11
	printf("\nTime-sharing stopped\n");
	return;
    }
    printf("\nOpser terminating\n");
    opser_exit(0);
}

#if defined(vax) || defined(mips)
char halt[]="/etc/halt";	/*halt command*/
#endif

/* Command handling code for Local Opser "halt" command */
haltsys()
{

#ifdef ultrix11
    int mem, tcnt, clkadr;
    char *coref;
#endif ultrixc11

    ttynam = ttyname(0);
    if (strcmp(cnsle, ttynam) && strcmp(alt_cnsle, ttynam)){
	printf("\nHalt can only be run from the console device\n");
	return;
    }
#if defined(vax) || defined(mips)
    system("rm -f /etc/sdnologin"); /*remove it just in case its there*/

    /*create file to flag opser as active.  */
    if ((flagf = fopen(active,"w")) == 0) {
        perror("Can't open /etc/opseractive");
        opser_exit(1);
    }
    /* All is ok, close file */
    else fclose(flagf);	

    /* Force in-memory disk info. to disk */
    sync();

    /* Halt the system */
    execl(halt, (char*)0);
}
#endif
#ifdef ultrix11
    nlist("/unix", nl);
    if (nl[0].n_type==0) {
	fprintf(stderr, "No namelist\n");
	return;
    }
    coref = "/dev/mem";
    if ((mem = open(coref, 2)) < 0) {
	fprintf(stderr, "No mem\n");
	return;
    }
    hltcode[0] = (nl[1].n_value + 022);
    hltcode[2] = (nl[1].n_value + 022);
    lseek(mem, (long)nl[0].n_value, 0);
    read(mem, &clkadr, sizeof(clkadr));
    lseek(mem, ((long)clkadr)&~0377000000000L, 0);
    printf("Ready to halt system ? ");
    gets(instr);
    if(instr[0] != 'y'){
	close(mem);
	return;
    }
    printf("Halting System in 1 second\n");
    sync();
    sleep(1);
    sleep(1);
    tcnt = 0;
    write(mem, &tcnt, sizeof(tcnt));	/* turn off clock. i hope */
    lseek(mem, (long)0100, 0);
    write(mem, hltcode, sizeof(hltcode));
    lseek(mem, ((long)clkadr)&~0377000000000L, 0);
    tcnt = 0100;
    write(mem, &tcnt, sizeof(tcnt));	/* turn on clock. i hope */
    for(tcnt = 0; tcnt > 0; tcnt++);
    close(mem);
}
#endif ultrix11

/* This function sends a command to Remote Opser */
send_command(command,args)
char *command,**args;
{
    char **ptr = args,inp_buf;
    int exit_code = 0,write_err = 0,readmask,nfds,res,num_esc = 0;
    int save_ptr = 0;
    struct sgttyb b;

    gtty(0,&b);
    b_save = b;
    got_tty = 1;
    b.sg_flags |= CBREAK;
    b.sg_flags &= ~ECHO;
    stty(0,&b);
    if(write(client_sock,"\033\033",2) == -1)
	  goto socket_error;
    if(write(client_sock,command,strlen(command)) == -1)
	  goto socket_error;
    while(*ptr){
	if(debug)printf("%s ",*ptr);
	if(write(client_sock," ",1) == -1)
	  goto socket_error;

	if(write(client_sock,*ptr,strlen(*ptr)) == -1)
	  goto socket_error;
	ptr++;
    }
    printf("\n");
    if(write(client_sock,"\033\033",2) == -1){
/************/
socket_error:
/************/
	perror("\n\7\7\7Socket write failed");
	stty(0,&b_save);
	client_name[0] = '\0';
	return;
    }
    while(1){
	readmask = (1 << client_sock) | 1;
 	if(script_fd != -1) readmask |= (1 << script_fd);
     	if(script_fd > client_sock) nfds = script_fd + 1;
 	else nfds = client_sock + 1;
	while(select(nfds,&readmask,0,0,0) == -1) ;
	if(readmask & 1){
	    if((res = read(0,&inp_buf,1)) == 0){
		clearerr(stdin);
		if(write(client_sock,"\004",1) == -1){
		   goto socket_error;
		}
	    }
	    else if(res == -1){
		perror("Read failed on stdin.\n");
	        stty(0,&b_save);
	        client_name[0] = '\0';
	        return;
	    }
	    if(tilde_flag){
		if(write(client_sock,"~",1) == -1)
			goto socket_error;
		tilde_flag = 0;
	    }
	    if(inp_buf == '~') tilde_flag = 1;
	    if(!tilde_flag){
		if(write(client_sock,&inp_buf,1) == -1){
			goto socket_error;
		}
	    }
	}
 	if(readmask & (1 << script_fd)){
 	    if((res = read(script_fd,&inp_buf,1)) == 0){
 		close(script_fd);
 		script_fd = -1;
 		if(write(client_sock,"\004",1) == -1){
 		    printf("Socket write failed.\n");
 		    stty(0,&b_save);
 		    exit(1);
 		}
 	    }
 	    else if(res == -1){
 		perror("Read failed on script file.\n");
 	        stty(0,&b_save);
 		exit(1);
 	    }
	    else if(write(client_sock,&inp_buf,1) == -1){
		printf("Socket write failed.\n");
		stty(0,&b_save);
		exit(1);
	    }
 	}	    
	if(readmask & (1 << client_sock)){
	    if((res = read(client_sock,&inp_buf,1)) == 0){
		printf("\nRestarting Slave System.\n");
	        stty(0,&b_save);
	client_name[0] = '\0';
	return;
	    }
	    else if(res == -1){
		perror("Read failed on socket.\n");
	        stty(0,&b_save);
	client_name[0] = '\0';
	return;
	    }
	    if((num_esc) && (num_esc != 2) && (inp_buf != '\033')){
		while(num_esc){
		    write(1,"\033",1);
		    num_esc -= 1;
		}
	    }
 	    if(inp_buf == '\n'){
 		 last_client_line[save_ptr] = '\0';
 		 save_ptr = 0;
	    }
 	    else if((num_esc == 0) && (inp_buf != '\0') &&(inp_buf != '\033'))
 		 last_client_line[save_ptr++] = inp_buf;
	    if(inp_buf == '\033') num_esc++;
	    if(num_esc == 0) write(1,&inp_buf,1);
	    else if((num_esc == 2) && (inp_buf != '\033')){
		exit_code = exit_code * 10 + inp_buf - '0';
	    }
	    else if(num_esc == 4){
		num_esc = 0;
		print_exit_stat(exit_code);
	        stty(0,&b_save);
		return(exit_code);
	    }
	}
    }
}

/* Prints Exit Status (debugging) */
print_exit_stat(exit)
int exit;
{
 if (debug){
    if(exit == 0) printf("Command completed successfully.\n");
    else printf("Command completed with error code %d.\n",exit);
 }
}

/* This function requests a password from the user */
/* validates it, and ret's a ok/ not ok response   */
int get_passwd()
{
	register char  *pp, *namep;
	struct	passwd *pwd;

	setpriority(PRIO_PROCESS, 0, -4);

	/* get password for root */
	pwd = getpwuid(0);

	/* Ask user to enter root password */
	pp = getpass("Password: ");

	/* Encrpt it */
	namep = crypt(pp, pwd->pw_passwd);

	setpriority(PRIO_PROCESS, 0, 0);

	/* Compare password entered           */
	/* to password from "etc/passwd" file */
	if (strcmp(namep, pwd->pw_passwd) == 0)
           return(1);
	else return(0);
}

/* Timeout detected on socket */
void socket_alarm()
	{
	 printf("\007\007\007\n");
         printf("Lost Connection to Slave Opser socket\n");
	 printf("Aborting Master Opser.\n");
         stty(0,&b_save);
	 client_name[0] = '\0';
	 return;
}

/* Prints error message if remote system did not shut down */
/* within allowed time                                     */
void shutdown_alarm()
	{
	 printf("\007\007\007\n");
	 printf("TIMEOUT - Did not get socket connection to Slave Opser\n");
         printf("Assumed Lost Connection to Slave Opser socket\n");
	 printf("Aborting Master Opser.\n");
	 client_name[0] = '\0';
	 return;
}

/* This is common exit code */
opser_exit(code)
   int code;
	{
	system("rm -f /etc/sdnologin");
	system("rm -f /etc/remoteopser");
        if(got_tty)stty(0,&b_save);
	exit(code);
}

/* formats time stamp for staging file */
tm(tim_out)
  char *tim_out;
{
long clock;
char *cp;

clock = time(0);

cp = ctime(&clock);

strcpy(tim_out,cp);
for(cp = tim_out; *cp > 0; cp++)
 if (*cp == ' ') *cp = '-';

*(cp-1) = 0;
}

/* Converts a Character Special Filename */
/* into a Character Block Filename       */
mkblk(file)
     char file[];
     {
     strcpy(new, "/dev/");
     strcat(&new[5], &file[6]);
}
