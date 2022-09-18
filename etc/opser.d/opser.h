/*
 * 	@(#)opser.h	4.1	(ULTRIX)	7/2/90
 */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sgtty.h>
#include <time.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/dir.h>
#include <pwd.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <a.out.h>
#include <fstab.h>
#ifdef vax
#include <strings.h>
#endif
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/param.h>

#define HELP 0
#define QUESTION 1
#define USERS 2
#define SHUTDOWN 3
#define BACKUP 4
#define RESTART 5
#define EXIT 6
#define QUIT 7
#define BYE 8
#define SHELL 9
#define FSCK 10
#define HALT 11
#define DISMOUNT 12
#define NETWORK 13

#define N_HELP 0
#define N_QUESTION 1
#define N_BACKUP 2
#define N_RESTART 3
#define N_EXIT 4
#define N_QUIT 5
#define N_BYE 6
#define N_SHELL 7
#define N_L_SHELL 8
#define N_FSCK 9
#define N_HALT 10
#define N_DISMOUNT 11
#define N_SIZE 12
#define N_PASSWORD 13

#define NO_OPEN 1
#define NO_PTY 2
#define NO_TTY 3
#define LOST_CONNECT 4
#define FORK_FAIL 5
#define INV_CMD 6

#define DEBUG 0

#define SHUTDOWN_TIMEOUT 600

char *getpass(),*crypt();
