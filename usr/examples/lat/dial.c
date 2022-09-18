#ifndef lint
static char *sccsid = "@(#)dial.c	4.1 (ULTRIX)	7/17/90";
#endif lint

/*
 * d i a l  
 *
 * Description: This sample program illustrates the use of a LAT Host 
 *              Initiated Connection.  It connects /dev/ttyxx to a DEC 
 *              SCHOLAR modem that is attached to the port "LAT_PORT" 
 *              on the DECserver 200 "LAT_SERVER".  After a successful 
 *              open, it autodials a phone number to a host computer 
 *              and emulates a terminal connected to the host computer.
 *
 * Setup:       Before invoking 'dial', LAT_SERVER and LAT_PORT must be 
 *              defined by the lcp command:
 *
 *              lcp -h /dev/ttyxx:LAT_SERVER:LAT_PORT
 *
 *              Access to '/dev/ttyxx' must be Read/Write for the user 
 *              of 'dial'.
 *
 * To compile:  cc -o dial dial.c
 *
 * Usage:       dial phone# /dev/ttyxx
 *
 * Comments:    In terminal emulation: 
 *                ^](CTRL/]) for escape character  
 *                ^]? for help
 *                ^]b to send break signal
 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sgtty.h>
#include <sys/types.h> 
#include <sys/file.h> 
#include "/sys/h/ioctl.h"


/* 
 * For DEC SCHOLAR modem (See SCHOLAR 2400 Modem Owner's Manual)
 * byte 1:     1 (CTRL/A) - autodialer
 * byte 2:     P - pulse dialing  T - tone dialing
 * last byte:    ! - start dialing
 */
u_char nl[20]={0x01, "P123-4567!"};  

int fd;    
void nodial();

main(argc,argv)
int argc;
char *argv[];
{
    char buf[BUFSIZ];    /* Read/write buffer */
    int len;

    /* 
     * Open reverse LAT device.  Set the O_NDELAY bit so
     * that we get an EBUSY error if the LAT_PORT is busy.
     * Without this, our request might get queued by the
     * terminal server (if the port is busy & queuing is on)
     * and we might sit waiting for a long time.
     */
    if ( (fd = open(argv[2],O_RDWR|O_NDELAY)) < 0 )
    {
    perror(argv[0]);
        goto doneonerror;
    }

    len = strlen(argv[1]);       /* get phone # */
    strcpy(&nl[2], argv[1]);
    nl[len+2] = '!';             /* ! for start dialing */
    write(0, "Dialing ", 8);     /* print 'Dialing phone#, wait...' */
    write(0, argv[1], len);
    write(0, ", wait... ", 10);  
    write(fd, nl, len+3);        /* send phone # to modem for autodial */

    signal(SIGALRM, nodial);          /* Give call 60 seconds to go thru */
    alarm(60);
    read(fd, buf, 80);                /* get echo of phone # */
    signal(SIGALRM, SIG_IGN);
    len = read(fd, buf, 80);          /* get return status */
    buf[len] = 0x00;
    printf("%s", buf);                /* print return status */
    if (buf[0] == 'A') termmain();    /* act as terminal emulator if */
                                      /* 'Attached', exit otherwise */

doneonerror:
    printf("Try later\n");
    exit(1);
}

void nodial()
{
    char buf[BUFSIZ];    /* Read/write buffer */

    printf("\nDial out failed\n");
    exit(1);
}

/*
 * The remainder of the this program is a terminal emulator.
 */

struct sgttyb Isgttyb, sgttyb, sgttyb1;
struct tchars Itchars, tchars1;
struct ltchars Iltchars, ltchars;
int fd, readfd, writefd, exception, outfile, ret, ret1;    

void resettty();


termmain()
{

    char buf[BUFSIZ];    /* Read/write buffer */
    char *bufptr;
    int on = 1;

    ioctl(0, TIOCGETP, &Isgttyb);
    ioctl(0, TIOCGETC, &Itchars);
    ioctl(0, TIOCGLTC, &Iltchars);

    /*
     * Set the terminal into CBREAK | NOECHO | -CRMOD mode so
     * that we can handle character buffering and echo ourselves. We will
     * also disable all special character handling except ^S and ^Q.
     */
    sgttyb = Isgttyb;
    sgttyb.sg_flags |= CBREAK;
    sgttyb.sg_flags &= ~(ECHO | CRMOD);
    ioctl(0, TIOCSETP, &sgttyb);
    tchars1 = Itchars;
    tchars1.t_intrc = tchars1.t_quitc = tchars1.t_eofc = tchars1.t_brkc = -1;
    ioctl(0, TIOCSETC, &tchars1);
    ltchars.t_suspc = ltchars.t_dsuspc = ltchars.t_rprntc = ltchars.t_flushc
                    = ltchars.t_werasc = ltchars.t_lnextc = -1;
    ioctl(0, TIOCSLTC, &ltchars);

    ioctl(fd, TIOCGETP, &sgttyb1);
    sgttyb1.sg_flags |= RAW; 
    sgttyb1.sg_flags &= ~ECHO;
    ioctl(fd, TIOCSETP, &sgttyb1);
    ioctl(fd, FIONBIO, &on);

    signal(SIGHUP, resettty);
    signal(SIGINT, resettty);
    signal(SIGQUIT, resettty);
    signal(SIGBUS, resettty);
    signal(SIGSEGV, resettty);

    printf("escape character: ^];   help: ^]?\r\n\n");
    for (;;)
    {
        readfd = exception = (1 << fd) + (1 << 0);
        if ((select(fd+1, &readfd, 0, &exception, 0)) > 0)
        {
            if (readfd & (1 << fd))
            {
                if ((ret = read(fd,buf,BUFSIZ)) <= 0) 
                {
                    printf("ret: %d\n", ret);
                    goto done;;
                }
                ret1 = write(0,buf,ret);     
                ret -= ret1;
                bufptr = buf + ret1;

                while (ret)
                {
                    writefd = 1 << 0;
                    select(fd+1, 0, &writefd, 0, 0);
                    if (writefd & (1 << 0))
                    {
                        ret1 = write(0,bufptr,ret);     
                        ret -= ret1;
                        bufptr = bufptr + ret1;
                    }
                }
            }
            if (readfd & (1 << 0))
            {
                ret = read(0,buf,BUFSIZ);
                if (*buf == 0x1d) 
                {
            if ( !(*buf = esccommands()))
                        continue;
                }
                write(fd,buf,ret);     
            }
            if (exception & (1 << fd)) 
            {
                printf("exception: \n");
                goto done;
            }
        }
        else 
        {
            perror("select: \n");
            goto done;
        }
    }

done:
    printf("\nEXIT! ");
    resettty();
}

void resettty()
{
    int off = 0;

    /*
     * Restore the terminal characteristics to their state before the
     * current session was entered.
     */
    ioctl(0, TIOCSETP, &Isgttyb);
    ioctl(0, TIOCSETC, &Itchars);
    ioctl(0, TIOCSLTC, &Iltchars);
    close(fd);
    printf("\nUltrix LAT dial out disconnected\n\n");
    exit(0);
}


/*
 *        e s c c o m m a n d s
 *
 * for input chatacter:
 * ?:        this menu
 * p:         escape to local command mode
 * b:         send a break
 * esc:     send ^]
 * all others:     exit esacape mode
 *
 */
esccommands()
{
    char ch;
    int ret;

    ret = read(0,&ch,1);
    switch(ch) 
    {
    case 'p':
        localcommands();
        break;

    case 'b':
            ioctl(fd, TIOCSBRK, 0);
        break;
      
    case 0x1b:
        return (0x1d);

    case '?':
        printf("\t?\tthis menu\r\n");
          printf("\tp\tescape to local command mode (? for help)\r\n");
        printf("\tb\tsend a break\r\n");
        printf("\tescape\tsend ^]\r\n");
        printf("\tothers\texit escape mode\r\n");

    }
    return(0);
}


/*
 *        l o c a l c o m m a n d s
 */
extern char **environ;
localcommands()
{
    char command[512];
    int notdone = 1,pid;

    /*
     * Reset the terminal to its original state.
     */
    ioctl(0, TIOCSETP, &Isgttyb);
    ioctl(0, TIOCSETC, &Itchars);
    ioctl(0, TIOCSLTC, &Iltchars);

    printf("\n");
    while (notdone)
    {
    printf("local command> ");
    if (gets(command) == NULL)
    {
            printf("\nEXIT! ");
        resettty();
    }
    switch (command[0])
    {
        case '?':
        printf("\tsuspend\tsuspends lat\n");
        printf("\texit\texits\n");
        printf("\t^D\texits\n");
        printf("\tcmd\tinvoke shell to execute command\n");
        printf("\t\tblank line resumes lat\n\n");

        case '\0':
        notdone = 0;
        break;

        default:
        /*
         * Check for special commands that we handle locally.
         */
        if (strcmp(command, "suspend") == 0)
        {
            kill(getpid(), SIGTSTP);
            break;
        }
        if (strcmp(command, "exit") == 0)
        {
                    printf("\nEXIT! ");
                resettty();
        }
        pid = fork();
        if (pid < 0)
        {
            perror("lat server - fork failed");
            break;
        }
        if (pid == 0)
        {
            if (execle(getenv("SHELL"), getenv("SHELL"), "-c", 
                               command, 0, environ) < 0)
            {
            perror("lat server - unable to exec shell");
            exit(1);
            }
        }
        wait(0);
        break;

    }
    }

    /*
     * Reset the terminal to its state on entry.
     */
    ioctl(0, TIOCSETP, &sgttyb);
    ioctl(0, TIOCSETC, &tchars1);
    ioctl(0, TIOCSLTC, &ltchars);
}

