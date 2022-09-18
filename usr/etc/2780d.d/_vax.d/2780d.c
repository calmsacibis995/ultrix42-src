#ifdef lint
static char *sccsid = "@(#)2780d.c	4.1	ULTRIX	7/2/90";
#endif


/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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

#include <stdio.h>
#include <errno.h>
#include <sys/types.h> 
#include <ustat.h>
#include <sys/file.h> 
#include <sys/socket.h>
#include <net/if.h>
#include <netbsc/bsc_proto.h>
#include <netbsc/bsc.h>
#include <time.h>
#include "rje.h" 
#include <netbsc/bsc_messages.h>

/*
 * Types of notification messages
 */
#define READY 0			/* output ready */
#define SENT 1			/* job transmitted */
#define SQUEUED 2		/* file queued */
#define JOBACKED 3		/* job acknowledged */
#define NOACK 4			/* job not acknowledged */
#define UNDELIV 5		/* undeliverable output */
#define SOMIS 6			/* missing signon card */

/* 
 *  What files to get when getq is called
 */
#define OFCARDS 1      /* return a q of card files with files to send in them */
#define OFOUTP  2      /* return a q of card file containing files to be received (zf) */

/* etc */
#define BADMAIL "dead.letter"
#define QSIZE 6			/* xmitted job queue size */
#define NSEARCH  1000		/* number of lines (cards) to search 
				   when looking for the usr card */
#define TRIES  3		/* tries to reconn if line goes down */
#define STRSIZE 80		/* string size */

struct jobdat {
	char jd_jobnm[9];
	char jd_pgrmr[25];
	char jd_jobno[9];
	char jd_login[9];
	char jd_place[128];
 	short jd_lvl;
	char *jd_dir;
	unsigned jd_uid;
};

struct jobdat jobdat;
int trunc;



char  	*SD;				/*spool directory */
FILE *acctfp,*fp;			/* acctlog file point. & tmp rec fp */
unsigned char test;			/* chars read from input stream */
struct b3780_buff d_buff;               /* the recv buffer */
unsigned char buff[80];			/* line in data file */
int l_count;				/* current line count */
long    tempoff1,offset1;               /* position of last getentryincf*/
char *name;				/* program name */
int jclcardc=0;		 		/* # blocks sent or received */
int posinrec=0;				/* position in rec for tabbing */
int *tab;				/* tab template */

static char etoa[]={                    /* ebcdic to ascii table */
#	include "etoa.h"
};
static char atoe[]={                    /* ascii to ebcdic table */
#	include "atoe.h"
};



/* 	APPLICATION.C	*/

main(argc,argv)
	int argc;
	char *argv[];
{
	struct sockaddr_bsc sin;	/* for socket, bind, and connect */
	register int g, s, n;
	struct queue **dirlist;		/* list of names in directory */
	int rerror=0,serror=0;		/* recv or send error */
	int linerror=0;
	int in_sendmode=0,in_recvmode=0,filetosend=0,recvoob;
	int pidno,i;

	struct stat sb;
	struct ustat usb;
	char *sendfilename;  		/* name of file to be sent */
	char *cardfilename;  		/* name of card/control file */
	char *oldcardname=NULL;		/* last control file sent out */
	char *donefilename;  /* file symlinked to cardfile after sending */
	char *mailnotif;     	     /* name of file, needed with mail   */
	char *transpflg;     	     /* Transparent data switch   */
	char *spacecomprs;		/* space compression entry */
	char *multibuf;     	     /* send files in multirecord format  */
	char *d3780;		     /* 3780e executed */
	char *signoncard;		/* contents of sign-on card */
	unsigned char sign_on[STRSIZE+1]; /* ebcdic signoncard + etx */

	char *getentryincardfile();
	register struct dsplog *dp;

	char *user;			/* owner of cardfile */		
	char *mailto; 			/* name of person who requested mail */
	int b_count;			/* current blocksize */
	unsigned char s_buff[b_3780_size+2];	/* largest size buffer to send with eof char and dle if trans data */
	int tx_count;		/* keeps track of where chs go in s_buff.tx[] */
	int lastrec;		/* last possible start of a new record */
	int r_count=0;		/* record count w/in a block */
	int mulbufmx;		/* max # of records w/in a buffer/block */
	unsigned char recsep=0;		/* record separator IRS or IUS */
	unsigned char eob;		/* end of block - ETB of ETX */
	FILE *fopen(), *rfp,*cfp,*sfp;

	SD = DEFSPOOL;
	name=argv[0];

	if (chdir(SD) < 0){
		printf("cannot chdir to %s",SD);	/* XXXX perr */
		exit(1);
	}


		/* .rjed has pid of daemon in it.  If it's an 
		 *  active process then the daemon is already
		 *  running.  If the file doesn't exist or it's
		 *  not an active process then we want to continue.
		 */

	rfp=fopen(".rjed","r");
	if (rfp != NULL ) {   /* file exists, check if active daemon */
		fscanf(rfp,"%d",&pidno);
		fclose(rfp);
/* what if pidno == 0,  garbage or empty file, bring up daemon */
		if (kill(pidno,0)== 0)  /* active daemon */
			exit(0);
		else if (errno==ESRCH)
			unlink(".rjed");
		else exit(0);   /* someother kind of error */
		
	}
/* no active daemon, continue */


	rfp=fopen(".rjed","w");
	if (!rfp){
		perr("cannot open .rjed");
		exit(1);
	}

	fprintf(rfp,"%d",getpid());
	fclose(rfp);




/* dp=(struct dsplog *)malloc(sizeof(struct dsplog ));
/*dp->d_un.x.d_uid=getuid();
		notify((char *) 0,dp->d_un.x.d_uid,SENT,dp->d_un.x.d_file);
		acctng(dp->d_un.x.d_file,dp->d_un.x.d_uid,dp->d_un.x.d_cnt);
		doresp();
		break;
*/

		/* only if acctlog exists */
	if(stat("acctlog",&sb) == 0)
		acctfp = fopen("acctlog","a");
		oldcardname=malloc(STRSIZE);
		if (oldcardname==NULL){ perr("out of memory"); exit;}

while(1) {	/*  while there are files in the spool area, or we're
		    receiving another file */
	linerror=0;
	if (( ! in_sendmode) && ( !in_recvmode)){
			/* get list of control files from the spool
			 * area.  If there are none then we have 
			 * nothing to do, so we leave.
			 */
			/* We keep doing getq in case a priority
			 * job has snuck its way to the head of 
  			 * the queue.
			 */
		if ((filetosend=getq(&dirlist,OFCARDS)) < 0){
			perr("can't scan spool directory");  /* log */
			exit(1);
		}
		if (filetosend)
			in_sendmode=1;
		else break;	/*  no filetosend so die */
	}

	if (in_sendmode) {	/* get needed info out of control file */
		stat(".",&sb);
		ustat(sb.st_dev,&usb);
		if (usb.f_tfree < 100){
			perr("not enough space in spool to do renames & links");
			break;
		}
		jclcardc=0;
		cardfilename=(*dirlist)->q_name;
		donefilename=malloc(STRSIZE);
		sprintf(donefilename,"z%s",&cardfilename[1]);
		cfp=fopen(cardfilename,"r");
		if (cfp==NULL){	/* can't open card file */    /* ???? */
			unlink(cardfilename);	
			in_sendmode=0;
			continue;
		}

/*		phonenum=getentryincardfile(cfp,'D');
		if (*phonenum == "")
			perr("no phone number given to connect");  */
/*              	for(i=0;i<14;i++)
				sin.sin_addr[i]=(*(phonenum+i));
*/
		user=getentryincardfile(cfp,'P');
		mailto=getentryincardfile(cfp,'M');
	 	transpflg=getentryincardfile(cfp,'X');
		if (d3780=getentryincardfile(cfp,'3')){
			b_size=512;
			lastrec=b_size-81;   	/* 80 text, 1 IRS */
			if (transpflg)	mulbufmx=1;  else  mulbufmx=7;
			recsep=IRS;
		}
		else {
			b_size=400;
			lastrec=b_size-80;	/* 80 text chars no IUS */
			mulbufmx=2;
			recsep=IUS;
		}

		spacecomprs=getentryincardfile(cfp,'C');

		if (multibuf=getentryincardfile(cfp,'B'))
			mulbufmx=7;

		signoncard=getentryincardfile(cfp,'S');
		sflushbuffer(sign_on,STRSIZE+1);
		if (signoncard == NULL){
			sfp=fopen("signon","r");
			if (sfp==NULL){
				perr("No signon card available");
				fclose(cfp);
				remjob(cardfilename);
				notify(user,0,SOMIS,cardfilename);
				in_sendmode=0;
				break;
			}
			n=0;
			while ((test=getc(sfp)) != EOL)
				sign_on[n++]=atoe[test];
			fclose(sfp);
		}
		else for(i=0; i<STRSIZE; i++)
			sign_on[i]=atoe[signoncard[i]];

		sign_on[STRSIZE]=ETX;
		sendfilename=getentryincardfile(cfp,'f');
		offset1=tempoff1;   
			/* if this card file no longer contains any
			 * files to be sent then rename it, incase 
			 * there are still files to be received listed
			 * in it.
			 */
		if (sendfilename==NULL) {
			rename(cardfilename,donefilename);
			in_sendmode=0;
			continue;
		}

		if (mailto)
			mailnotif=getentryincardfile(cfp,'N');

		fclose(cfp);

		fp=fopen(sendfilename,"r");
		if (fp==NULL){
			perr(strcat(sendfilename," cannot be opened"));
			Zoutbuf(cardfilename,'?');
			continue;
		}

	/* don't send a signon card for each sendfile
	 * within a cardfile, send it just once at the
	 * beginning of the user's job.  Watch out for
	 * those priority jobs that can slip in between
	 * sendfiles.  Another signon would be needed
	 * then.
	 */




	if (*cardfilename != *oldcardname){
	/* send the signon card */ 

		s = socket(AF_BSC, SOCK_DGRAM, 0);
		if (s < 0) {
			perr("socket");
			cleanup();
		}
		sin.sin_family = AF_BSC;

		bind(s,(char *)&sin,sizeof (sin));


		if (connect(s, &sin, sizeof(sin)) < 0) {
			perr("connect");
			(void) close(s);
			cleanup();
		}
	
		serror=send(s,sign_on,STRSIZE+1,0);
		if (serror < 0){
			perr("signon card no good");
			notify(user,0,NOACK,"signon card");
			remjob(cardfilename);
			in_sendmode=0;
			break;
		}

		(void)strcpy(oldcardname,cardfilename);


		recvoob=recv(s,d_buff.tx,sizeof(d_buff.tx),1);
		if (d_buff.tx[0]==ENQ) { 	
				in_sendmode=0;
				in_recvmode=1;
				fclose(fp);
				continue;
			}
		close(s);
	}
/* 
 * Hook to the socket (send line bid) and then read what's in the open file.
 */
	s = socket(AF_BSC, SOCK_DGRAM, 0);
	if (s < 0) {
		perr("socket");
		cleanup();
	}
	sin.sin_family = AF_BSC;


	bind(s,(char *)&sin,sizeof (sin));

		

	if (connect(s, &sin, sizeof(sin)) < 0) {
		perr("connect");
		(void) close(s);
		cleanup();
	}


		test=getc(fp);
		while (in_sendmode){
				/* send a buffer, check for OOB char.
				 * If none keep sending.  If OOB
				 * goto receive mode because the remote
				 * system wishes to send to us.
				 */
		/* read in a buffer */
			tx_count=0;		
			l_count=0;
			eob=ETB;

			if (transpflg)  /* let kernel know it's trans data */
				s_buff[tx_count++]=DLE; /* still have 512 for data */


			do {		/* fill up send buffer until
						card file is empty */

		/* fill up the buffer until EOL or buffer is full  **/
			if (transpflg)
				tfill(); 	/* add DLE chars */
			else 	nontfill(spacecomprs);

		/* if EOL & buffer is not full, pad out the buffer.
		 *      If EOF then put ETX in last position, otherwise
		 *	put an ETB there.
		 */
	
			if (test == EOL){
			    test=getc(fp);
				if (test == EOF) 
					eob = ETX;
			}
		/* copy that buffer into the send buffer  */
	/*  remember s-buff has chars in it not ints */

		
			for (i=0;(i<l_count);i++)
				s_buff[tx_count++]=buff[i];
			if (!transpflg)
				s_buff[tx_count++]=recsep;

			jclcardc++;
			l_count=0;
			r_count++;

			}while ((test!=EOF) && (r_count<mulbufmx) && (tx_count<=lastrec) );

			sflushbuffer(&s_buff[tx_count],b_size - tx_count);
			r_count=0;	
	
			if (!d3780 && !transpflg)	
				--tx_count;	/* replace IUS with eob */
			s_buff[tx_count++]=eob;
				/* send the buffer */

			serror=send(s,s_buff,tx_count,0);
			if (serror < 0){
				perr("error in sending");
				/* sfail++; */	
			        notify(user,0,UNDELIV,sendfilename);
				Zoutbuf(cardfilename,"Z");	
				fclose(fp);
				unlink(sendfilename);
/* zoutbuf will not find the right offset in the case where a send
   has started but the ibm side decides to send us something then I
   look at the usercard and it matches one in the queue. the offset
   will change to point at the outputfile not the sendfile,  so 
   put in a reinit for offset
*/

				linerror=1;	/* error on the line */
/*				if ((sfail >= 3))	 same block has tried
						   3 times to go out and
						   failed. file is prob.
						   no good or job not
						   being sent to 2780 or 3780
						   job will be removed.
						*/
				 in_sendmode=0;
				 cleanup();
				/*break; */     
			}

			sflushbuffer(s_buff,b_size+1);
			flushbuffer(d_buff.tx,sizeof(d_buff.tx));
			sflushbuffer(buff,80);

			if (test==EOF){
				fclose(fp);
				acctng(sendfilename,user,jclcardc);  /* not sure
which filename to use here */
				if (mailto != NULL)
					notify(mailto,0,SENT,mailnotif);
				in_sendmode=0;
				--filetosend;
					/* remove the datafile-the
					 * actual file that was sent */
				unlink(sendfilename);
					/* symbolically link cardfile
					 * with donefile incase an
					 * output file is recv'd before
					 * we're finished sending all the
					 * jobs included in this control
					 * file.  When a file is received
					 * from the remote host 2780d checks
					 * thru all the zfA.. files for
					 * possible output names.
					 */
				symlink(cardfilename,donefilename);
					/* in cardfile mark the datafile
					 * as sent so it won't try to
					 * send it again
					 */
				Zoutbuf(cardfilename,"Z");
			}

				/* check for OOB char */
			recvoob=recv(s,d_buff.tx,sizeof(d_buff.tx),1);
		/* if recvoob then error */
			if ((d_buff.tx[0]==RVI)||(d_buff.tx[0]==ENQ)) { 	
				in_sendmode=0;
				in_recvmode=1;
				fclose(fp);
				break;
			}
			else if (test==EOF)
				close(s);



		} /* while */
		free(donefilename);
	} /* if in */

	if (in_recvmode) {
		stat(".",&sb);
		ustat(sb.st_dev,&usb);
		if (usb.f_tfree < 1000){
			perr("No space in directory to receive");
			break;
		}
		jclcardc=0;
		posinrec=0;
		if (tab)
			free(tab);

			/* put the incoming file into rjetemp.out just
			 * temporarily until we can rename it 
			 */
		fp=fopen("rjetemp.out","w+");	/* use printf/scanf */

		
		if (fp==NULL){
			perr("can't open rjetemp.out");
			exit(1);
		}
		while (in_recvmode) {
			/* do - read a buffer, chg from ebcdic to ascii,
			 * write to rjetemp.out - until they're through
			 * transmitting.
			 */
			flushbuffer(d_buff.tx,sizeof(d_buff.tx));
			rerror=recv(s,d_buff.tx,sizeof(d_buff.tx),0);

			if (rerror <0){
				perr("error receiving");
/*				unlink("rjetemp.out");      hold off u. test */
				linerror=1;
				break;
			}
			jclcardc++;
/* if I read a whole buffer how do I know I didn't just read 2 cards
insted of one??*/  
			convrecv();

	/* how do I tell the difference between jcl cards and data */
			recvoob=recv(s,d_buff.tx,sizeof(d_buff.tx),1);
	/* if recvoob then an error occured */
			if (d_buff.tx[0] == EOT){	/* closefile char */
					/* check thru applicable control
					 * files (zfA..) for the owner
					 * of this file.  Move the temp
					 * file to the user's main dir
					 */
				findowner(fp,jclcardc);
				recvoob=recv(s,d_buff.tx,sizeof(d_buff.tx),1);
				/* check if new file coming in */
	/* error here if recvoob is true */
				if (d_buff.tx[0]!=ENQ){    /* no new file coming */
					in_recvmode=0;
					close(s);
				}
				break;		/* if no more files are
						 * coming in, go back to
						 * sending
						 */

			}
/* wonder if we receive something other than EOT */

		} /* eow */
	} /* eo if */

		/* if there has been an error on the line, try to
		 * reconnect but if we can't then just go home
		 */
	if (linerror){
perr(" there has been a linerror, will try to reconnect\n");
		oldcardname=" ";
		for (i=0; i< TRIES ; i++){     /* make howmany you want */
			close(s);		/* clean up */
			sleep(10);		/* sleep for 10 sec */
/*XXX 20? XXX*/
			/* 
 			 * Try to hook to the socket
 			 */
			s = socket(AF_BSC, SOCK_DGRAM, 0);
			if (s < 0) {
				perr("socket");
				return(0);
			}
			sin.sin_family = AF_BSC;


			bind(s,(char *)&sin,sizeof (sin));

		

			if (connect(s,&sin,sizeof(sin)) == 0){
				if (in_sendmode)
					close(s);	/* will reconn later */
				linerror=0;
				break;			/* out of for loop */
			}
		}
		if (linerror){  			/* i=TRIES */
			perr("rje line is down");
		      	break;   /* out of while(1) loop */	
		}
	}
} /* eow(1) */

close(s);
cleanup();
} /* eo main */



static
perr(msg)
	char *msg;
{
	extern int sys_nerr;
	extern char *sys_errlist[];
	extern int errno;
	int err,errlog;
	char *tmp;
	FILE *pfp;
	char *ERRFP="/usr/adm/rjelog";  /*XXX */

/* only root can run this program or rjelog will not get written to */
	

	err=errno;
	errlog=open(ERRFP,O_CREAT|O_WRONLY|O_APPEND,0644);
	if (errlog==NULL) {
		errlog=open("errlog",O_CREAT|O_WRONLY|O_APPEND,0644);
	}   /* XXXXX */
	
	tmp=malloc(STRSIZE);
	sprintf(tmp,"%s: %s: ",name,msg);

	
	write(errlog,tmp,STRSIZE);
	if (err < sys_nerr)
		write(errlog,sys_errlist[err],strlen(sys_errlist[err]));
		write(errlog,"\n",1);
	free(tmp);
	close(errlog);
}

	
/* blank out send buffers */
/* hex 40 is blank in ebcdic */

sflushbuffer(buffer,size)
unsigned char buffer[];
int size;
{		
	int i;

	for (i=0; i < size; ++i)
		buffer[i]=atoe[' '];
}


/* blank out buffers */

flushbuffer(buffer,size)
char buffer[];
int size;
{
	int i;

	for (i=0; i < size; ++i)
		buffer[i]=' ';
}

tfill()	/* transparent buffer fill routine */
	/* DLEs must be inserted before each BSC char */
{
	while ((l_count < 79) && (test !=EOL) ) {
		ckforBSCch(atoe[test]);
			/* chg to ebcdic char + put in buffer */
		test=getc(fp);
	}  
	sflushbuffer(&buff[l_count],80-l_count);
	l_count=80;

}

nontfill(spacecomprs)	/* non transparent data buffer fill up routine */
char *spacecomprs;	/* can fill in 80 because no DLEs need to be entered */
{
unsigned short spcnt=0;
int i;

	if (!spacecomprs) 
		while ((l_count < 80) && (test !=EOL)) {
			buff[l_count++]=atoe[test];
			  /* chg to ebcdic char + put in buffer */
			test=getc(fp);
		}  

	else {
		while ((l_count < 80) && (test !=EOL)) {
		   if ((test==' ') && (spcnt < 64) && (l_count < 79))
			spcnt++;
		   else {
			if (spcnt > 2) {
				spcnt=spcnt|0x40;
				buff[l_count++]=IGS;
				buff[l_count++]=(unsigned char) spcnt;
			}
			else
			   for (i=0;i<spcnt;i++)
				buff[l_count++]=atoe[' '];

			spcnt=0;

			if (l_count >= 80)  break;  /* buff full*/
			
			buff[l_count++]=atoe[test];
		   }
		test=getc(fp);
		}
	}
	if (test==EOL) {
		spcnt=80-l_count;
		if ((spacecomprs) && (spcnt > 2))   {
			spcnt=spcnt|0x40;
			buff[l_count++]=IGS;
			buff[l_count++]=(unsigned char) spcnt;
		}
		else {
			sflushbuffer(&buff[l_count],80-l_count);
			l_count=80;
		}
	}
}

/* remove a job after a problem with the signon card.
 * delete all files to be sent that are contained within
 * the card file and then delete the card file.
 */
remjob(cf)
char *cf;

{
FILE *cfp;
char *dfile,*getentryincardfile();


	cfp=fopen(cf,"r");
	while ((dfile=getentryincardfile(cfp,'f')) != NULL)
		unlink(dfile);
	fclose(cfp);
	unlink(cf);
}

/*  ckforBSCch checks for BSC chars in the output stream if they're
*    there then a DLE will get put in the buffer before them.
*/
ckforBSCch(ch)
unsigned char ch;

{
	switch(ch){
		case DLE:
	        case SYN:
		case ENQ:
	        case STX: 
	        case SOH: 
	        case ETX:
		case ETB: 
	        case ITB: 
		case IRS:
		case NAK:
		case EOT:
			buff[l_count++] = DLE;
		default:
			buff[l_count++] = ch;
}
}
convrecv()
/*  converts the input stream (d_buff) from ebcdic to ascii, 
 *  expands IGS sequences, and does horizontal tab formatting,
 *  then writes it to the temporary file - fp 
 *  2780d can't tell the difference between recving transp or non transp
 *  data because all BSC chars are stripped off in the kernel.
 */
{
	unsigned char n_buff[2048];
	int length,i,j,k;
	int tabstop=0;
	unsigned int spcnt=0;

	length=sizeof(d_buff.tx);

	if (jclcardc==1){	/* 1st record in the transmission */
		printorpunch();  /* get rid of print or punch chars */
		escseq();	/* look for tab format record */
	}

	/*  copy the input buffer, if any IGSs fill in with spaces, 
         *  if any HTs then jump to next tab position */

	for(i=j=0;i<length;i++)
		switch (d_buff.tx[i]){
		    case IGS:
		 	spcnt=d_buff.tx[++i] & 0x003F;
			flushbuffer(&n_buff[j],spcnt);
			j=j+spcnt;
			posinrec=posinrec+spcnt;
			break;
		     case HT:
			tabstop=findnxtab(posinrec);
			flushbuffer(&n_buff[j],tabstop-posinrec);
		        j=j+tabstop-posinrec;
		 	posinrec=tabstop;
			break;
		     case IRS:         	 /* strip off? */
		     case NL:
		     case LF:
			posinrec=(-1);	 /* no break. posinrec will=0 */ 
		     default:
			n_buff[j++]=etoa[d_buff.tx[i]];
			posinrec++;
	/* posinrec is needed to use tabformatting correctly, it is the
 	 * position in the record within the block read (d_buff.tx).  j
	 * is the position in the new block, not the pos in the current
 	 * record of the new block. 
	 */
		}
	fwrite(n_buff,j,1,fp);
	flushbuffer(d_buff.tx,length);
	flushbuffer(n_buff,2048);  /* length is size of array,
nbuff=dbuff*/
}

/* find the next tab position */
findnxtab(pos)
int pos;
{
int *head;
int i,length;
					/* tab[] holds tab positions */
	length=sizeof(d_buff.tx);	/* = sizeof tab */
	if (tab==NULL){
	   perr("No tab format record given");
	   return(pos);
	}
	
	for (i=0,head=tab;(*head != NULL) && (i<length-1) && (pos >= *head);head++,i++);
	if ((*head !=NULL) && (i<length))
		return(*head);
	else return(pos);
/* free(head) */
}


printorpunch()
{
	struct b3780_buff di_buff;               /* temp buffer */
	int i=0;
	int len;

	len=sizeof(d_buff.tx);
	flushbuffer(di_buff.tx,len);

	switch (d_buff.tx[0]){
		case 0x11:    /* print */
		case 0x12:    /* punch */
		case 0x13:    /* punch */
			i++;  
		    	strncpy(di_buff.tx,&d_buff.tx[i],len-i);
		    	strncpy(d_buff.tx,di_buff.tx,len-i);
			d_buff.tx[len-1]='\0';
			break;
	}
}

escseq()		/* does the input buffer contain a HT seq? */
{
unsigned char byte;
int i,k,numirss,length;


	if ((d_buff.tx[0] == ESC) && (d_buff.tx[1] == HT)) {
		tab=(int *)malloc(length=sizeof(d_buff.tx));
		for(i=0;i<length-1;i++)
			tab[i]=0;
		i=2;
		k=numirss=0;
		while(((byte=d_buff.tx[i]) != NL) && (byte != LF) && (i<length))
		{
			if (byte==IRS)	/* drop on floor, not included as spaces */
				numirss++;
			if (byte==HT)
				tab[k++]=i-2-numirss;
			   		/* the template holds where tab
					 * positions are.  -2 for ESC 
					 * and HT, -numirss so IRSs
					 * aren't included as spaces
					 */
			i++;
		}
	}
}
char *getentryincardfile(fp,firstchar)
FILE *fp;
char firstchar;
/*  search thru the control/card file for the line beginning with   
    firstchar;  Notice no seeks are used to get to the beginning of
    the file, that's ok because we always write the phonenumber 
    before the filename */
/*  Make sure we read from fp in the same order as we write to it
    in 2780e 
*/
{
	char c;
	char *buf = NULL;
	int error;
	long ftell();
		
	buf=malloc(STRSIZE);
if (buf==NULL) { perr("out of memory");   exit(1);}
	do {
		c=getc(fp);
		if (c==firstchar) {
			flushbuffer(buf,80);
			buf[0]='\0';
			tempoff1 =ftell(fp) - 1;
			readtoeol(fp,buf);
			return(buf);
		 }
		readtoeol(fp,buf);
	} while (c!=EOF);
	
/*	flushbuffer(buf,80); */
	buf=NULL;
	fseek(fp,0,0);		/* we couldn't find what we were looking
				 * for so the next time we call this procedure
				 * we won't be at the end of the file
				*/
	return(buf);
		
}


 

readtoeol(fp,buf)
FILE *fp;
char buf[80];
{
	int i=0;
	char c=' ';

	while ((c=getc(fp)) != EOL && c!=EOF) 
	     buf[i++]=c;
	
	buf[i]='\0';
}

checkqueue()
{
/*  look for control files in spool area.  af first then cf.  */
}

/* cleanup is done after all send & recv processing has completed
   to cleanup the spool area.  Needless cf and zf files are removed
   within the send and receiving sections.
*/
cleanup()
{
/*	unlink("rjetemp.out");
	don't do that here because if errors I want rjetemp to hold
file
*/
	unlink(".rjed");
	exit(0);
/* make sure I'm still in the spool area */
}

/*
 * Scan the current directory and make a list of card files. 
 * Return the number of entries and a pointer to the list.
 */
getq(namelist,ofwhat)
	struct queue *(*namelist[]);
	int ofwhat;
{
	register struct direct *d;
	register struct queue *q, **queue;
	register int nitems;
	struct stat stbuf;
	int arraysz, compar();
	DIR *dirp;


	if ((dirp = opendir(SD)) == NULL)
		return(-1);
	if (fstat(dirp->dd_fd, &stbuf) < 0)
		goto errdone;

	/*
	 * Estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
	arraysz = (stbuf.st_size / 24);
	queue = (struct queue **)malloc(arraysz * sizeof(struct queue *));
	if (queue == NULL)
		goto errdone;

	nitems = 0;
	while ((d = readdir(dirp)) != NULL) {
	   if (ofwhat==OFCARDS){
		if ((d->d_name[0] != 'c' && d->d_name[0] !='a') || d->d_name[1] != 'f') 
			continue;	/* daemon control files only */
	   }
	   else if (ofwhat==OFOUTP){
		if ((d->d_name[0] != 'z' ) || d->d_name[1] != 'f') 
			continue;	/* files containing filenames to recv */

	   }
	   /* else procedure has been called wrong */
		 if (stat(d->d_name, &stbuf) < 0)
			continue;	/* Doesn't exist */
		q = (struct queue *)malloc(sizeof(time_t)+strlen(d->d_name)+1);
		if (q == NULL)
			goto errdone;
		q->q_time = stbuf.st_mtime;
		strcpy(q->q_name, d->d_name);
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (++nitems > arraysz) {
			queue = (struct queue **)realloc((char *)queue,
				(stbuf.st_size/12) * sizeof(struct queue *));
			if (queue == NULL)
				goto errdone;
		}
		queue[nitems-1] = q;
	}
	closedir(dirp);
	if (nitems)
		qsort(queue,nitems,sizeof(struct queue *),compar);
	*namelist = queue;
	return(nitems);

errdone:
/* out of memory */
	closedir(dirp);
	return(-1);
}

/*   compare on file names.  want priority files (af..) to go before
 *   non su jobs (cf...)
 */

static
compar(p1,p2)
	register struct queue **p1,**p2;
{
	return(strcmp((*p1)->q_name,(*p2)->q_name));
}






/*
 * If FILE acctfp is open write an accounting
 * record into.  Format:
 *	month/day   hour:min:sec   file   uid   cnt
 */
acctng(file,user,cnt)
	char *file;		/* file received/transmitted */
	char *user;		/* sender/receiver */
	long cnt;		/* number of records */
{
	long t;			/* clock time */
	struct tm *tp;		/* pointer to time struct */
	struct tm *localtime();

	if(acctfp == NULL)
		return;
	time(&t);
	tp = localtime(&t);
	fprintf(acctfp,"%02d/%02d  %02d:%02d:%02d  %s  %s  %ld\n",
	tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,file,user,cnt);
	fflush(acctfp);
}





/*
 * Generate and send notification messages
I took out the level
 */
notify(login,uid,type,arg)
	char *login;	/* login name (or NULL) */
	unsigned uid;	/* login uid if login==NULL */
/*	int level;	/* message level from USR card */
	int type;	/* type of message */
	char *arg;	/* argument for message */
{
	long tsec;	/* clock time */
	char times[10];	/* time string */
	char rjemesg[256];
	char *s;	/* perr() string */
	struct tm *tp,*localtime();
	struct passwd *pw,*getpwuid();
	register int n;

	s=malloc(STRSIZE);
	time(&tsec);
	tp = localtime(&tsec);
	sprintf(times,"%02d:%02d:%02d",tp->tm_hour,tp->tm_min,tp->tm_sec);
	switch(type) {
	case READY:
		sprintf(rjemesg,"\n%s %s job %s -- %s ready%s\n\n",
		times, jobdat.jd_jobnm, jobdat.jd_jobno, arg, 
		trunc? " (truncated)" : "");
		n = 6;
		break;
	case SENT:
		sprintf(rjemesg,"\n%s %s transmitted\n\n",times,arg);
		n = 3;
		break;
	case SQUEUED:
		sprintf(rjemesg,"\n%s %s job %s -- %s queued for execution\n\n",
		times,jobdat.jd_jobnm,jobdat.jd_jobno,arg);
		n = 6;
		break;
	case JOBACKED:
		sprintf(rjemesg,"\n%s %s job %s -- %s acknowledged\n\n",
		times,jobdat.jd_jobnm,jobdat.jd_jobno,arg);
		n = 5;
		break;
	case NOACK:
		sprintf(rjemesg,"\n%s %s not acknowledged\n\n",times,arg);
		n = 5;
		break;
	case SOMIS:
		sprintf(rjemesg,"\n%s %s missing signon card\n\n",times,arg);
		n = 5;
		break;
	case UNDELIV:
		sprintf(rjemesg,"\n%s job %s -- %s undeliverable\n\n",jobdat.jd_jobnm,jobdat.jd_jobno,arg);
		n = 6;
	}
	if(login == (char *) 0) {
		if((pw=getpwuid(uid)) == NULL) {
			(void) sprintf(s,"can't find uid %d\n",uid);
			perr(s);
			mail(rjemesg,"root");
			free(s);
			return;
		}
		login = pw->pw_name;
	}
/*	if(n >= (level%10) && ptty(rjemesg,login) >= 0)
		return;
	if(n >= (level/10))
*/
		if(mail(rjemesg,login) < 0) {
			unlink(BADMAIL);
			(void) sprintf(s,"can't mail to %s\n",login);
			perr(s);
			mail(rjemesg,"root");
		}
	free(s);
}

/*  I took out the ptty to write to a terminal in rjedisp */

/*
 * Mail mesg to login.  Return 0 on
 * success, -1 on failure.
 */
mail(mesg,login)
char *mesg,*login;
{
	int p[2],status,pid,i;

	if(pipe(p) < 0)
		return(-1);
	if((pid=fork()) == 0) {
		close(0);
		dup(p[0]);
		for(i=1; i < 20; i++)
			close(i);
		execl("/bin/mail","mail",login,0);
		exit(1);
	}
	if(pid < 0) {
		perr("can't fork");
		close(p[0]);
		close(p[1]);
		return(-1);
	}
	close(p[0]);
	write(p[1],mesg,strlen(mesg));
	close(p[1]);
	wait(&status);
	return((status==0)? 0:-1);
}



/* Zoutbuf marks the spot pointed to by offset with car, in other words -
    Zoutbuf will write the letter 'car' onto the file 'file' at the
    spot pointed to by offset1.  So that when getentryinfile is called
    looking for the letter that was originally there it won't find it
    on this line (offset).  It will look for the next occurance.
*/
Zoutbuf(file,car)
char *file,*car;
{
	FILE *fp;
	int error;

	fp=fopen(file,"r+");
	error=fseek(fp,offset1,0);
	fwrite(car,1,1,fp);
	fclose(fp);
}

/*  after receiving a file remotely  - must find the owner
     put the file in that person's directory, notify that 
     person by mail if requested, and make an acctlog entry
*/


/*  - findowner of this output file
    - find that persons cardfile (in spool) that expects this output file
    - see if the mailflag is in that cardfile?mailto=1:mailto=0
    - delete from the cardfile this output file name
    - if there are no more output files to be received, delete the
       whole cardfile from the spool area
    - move file to users area
*/

FILE *cfp;
char joblogin[9];
char jobplace[128];
int  uid;
char *dfname;

findowner(fp,jclcardc)
FILE *fp;
int jclcardc;
{
	struct queue **dirlist;
	int status=0,times,nred,nfiles,i,mailto=0;
	int renm=0;		/* outf correctly created or not */
	char  buf[132];     /* holds each line of the file,132 ave.
				print page length */
	char *datetime,*userid;
	long t;
	struct tm *tp,*localtime();
	char *mflg=NULL,*outputfname,*aname,*who=NULL,*job,*ofn;
	char *arg,*targ;
	char *tilde(),*maindir;

/* read the user and jobname from the incoming file.  The user card 
   is assumed to be within the first NSEARCH lines 
*/

	fseek(fp,0L,0);		/* start at the beginning fp->rjetemp  */

	for (times=0;((status==0) && (times<NSEARCH)); times++){
		nred=fread(buf,132,1,fp);
		status=usrcard(buf);
	}
	
/*  FIND DATE AND TIME */

	time(&t);
	tp=localtime(&t);
	datetime=malloc(9);
sprintf(datetime,"%02d%02d%02d%02d\0",tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec);

	/* if no usercard, put file in temp file */
	outputfname=malloc(MAXPATHLEN);
	(void) sprintf(outputfname,"Rje%s",datetime);
	dfname=outputfname;

	if (status)		
	{  /* get list of card files starting with zf */
	   nfiles=getq(&dirlist,OFOUTP);

	   if (nfiles< 0) {perr("can't scan spool directory"); exit(1); }
		/* get the user and jobname from recv file.  Look thru
		 * the zf control files for matches to user and jobname.
		 * if no match then recv fileis given a default name.
		 * If match is found.  A user specified output file is
		 * looked for within that cardfile.  If found will
		 * rename the file & put it in the users home dir
		 */
	   for(i=0;i<nfiles;i++){
	         aname=dirlist[i]->q_name;
		 cfp=fopen(aname,"r");
		 if (cfp==NULL) continue;	/* maybe unlink */
		 who=getentryincardfile(cfp,'P');
		 if (*who==*joblogin) {

		    while(((*(job=(arg=rindex(targ=getentryincardfile(cfp,'N'),'/'))?arg+1:targ)) != *jobplace) && (job!=NULL));
		    if (*job==*jobplace){	/* we found the owner */
		       fseek(cfp,0L,0);
		       userid=getentryincardfile(cfp,'I'); /* get uid for chown and notify*/

		       sscanf(userid,"%d",&uid);
		       mflg=getentryincardfile(cfp,'M');  /* do they want mail */
		       ofn=getentryincardfile(cfp,'O');
			/* get the 1st output filename */

		       offset1=tempoff1;	/* need the spot for zoutbuf */
		       fclose(cfp);		/* file has to be closed for zoutbuf */
		       if (ofn==NULL)	/* if no output file in here, delete ofn */
			   	unlink(aname);
		       /* there are no files to be received in this card file */
		       else {

			     maindir=malloc(MAXPATHLEN);
			     sprintf(maindir,"~%s",joblogin);
			     maindir=tilde(maindir);
			     /* that finds the main directory of the user */
			     sprintf(outputfname,"%s/%s",maindir,ofn);
			     Zoutbuf(aname,'o');	/* make sure I set offset */
			     break;	/* out of for loop */	
		       } /* ofn==NULL */
		    } /* job==jobplace */
		 } /* who==joblogin */
		 fclose(cfp);
		 /* get the next file in queue */
	    } /* eo for */
       } /* if status */
	renm=movetowner(fp,outputfname) ; /* give to owner or rename */

	mailto=(mflg==NULL)?0:1;

	if (mailto && (renm==0))   /* if they wanted mail and outf was
created not renamed */	
		notify(joblogin,uid,READY,outputfname);

	ofn=(arg=rindex(outputfname,'/'))?arg + 1:outputfname;
	acctng(ofn,joblogin,jclcardc);
}





usrcard(buf)
char *buf;
{
	static char usr[]={
		"'[Uu][Ss][Rr]=('$1,$2[,)]?)$3[,)]"
	};
	int n;
	char lvl[10];
	short joblvl;

	if ((n=nmatch(usr,buf,joblogin,jobplace,lvl)) >= 2){
		if (n==3)
			joblvl=atoi(lvl);
		else
			joblvl=54;
		return(1);
	}
	return(0);
}


/* movetowner will move the received file to its rightful owner
   if we know who it is, otherwise the file will just be renamed
   to a temp file.  The user would then have to look for her own file
   in the spool area
*/
movetowner(from,to)
FILE *from;
char *to;
{
	int fd,numbites,numrites;
	char msg[80];

	fseek(from,0L,0);
	
	if ((fd=open(to,O_WRONLY|O_CREAT|O_EXCL,00700)) < 0) {
		sprintf(msg,"could not create %s",to);
		numrites=(-1);	/* error cond */
	}
	else {
	
		do {
			flushbuffer(d_buff.tx,sizeof(d_buff.tx));
			numbites=fread(d_buff.tx,sizeof(d_buff.tx),1,from); 
			numrites=write(fd,d_buff.tx,sizeof(d_buff.tx)); 	
		} while (numbites && (numrites >= 0));
/* check out that loop looks q&d */
			/* write returns -1 on error  */
			/* read retruns 0 on eof or error */
		close(fd);
		chmod(to,00755);
		fclose(from);
		if (numrites < 0)
			sprintf(msg,"error trying to write to %s",to);
	}
	if (numrites < 0) {
			perr(msg);
			if (joblogin)
				notify(joblogin,uid,UNDELIV,to);
			fclose(from);
			/* may not be able to rename but we'll try */
			rename("rjetemp.out",dfname);
			return(-1);
	}
/* 		unlink("rjetemp.out");	uncomm. a. testing, make a
default name */
		if (joblogin)	/* if we know whose it is */
			chown(to,uid,-1);
		return(0); 	/* an ok condition */
}		
