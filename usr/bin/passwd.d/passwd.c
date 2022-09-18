#ifndef lint
static  char    *sccsid = "@(#)passwd.c	4.5  (ULTRIX)        2/1/91";
#endif lint

/************************************************************************

/************************************************************************
 *									*
 *			Copyright (c) 1988, 1989, 1990, 1991 by		*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * Modification history:
 *
 * 07-Aug-90	dlong
 *	Don't reauthenticate before checking minexp and change_pass.  Also,
 *	allow root to set any shell.
 *
 * 28-Nov-89	dlong
 *	Catch SIGINT and SIGTERM while echo is turned off.  Don't terminate if
 *	new password is too long, just warn and truncate.
 *
 * 6-Sep-89	dlong
 *	Fixed usage lines for chfn and chsh. Also, only reopen /dev/tty for
 *	passwd command.
 *
 * 22-Aug-88	dlong
 *	Check status from GETREQ to prevent infinite looping.
 *
 * 18-Aug-89	dlong
 *	Fixed check for user supplied password privilege when using -e.
 *	Also, don't issue message to eat password in -ea mode.
 *
 * 16-Aug-89	dws
 *	Added eopt and code to support extended Xprompter protocol.
 *	Cleaned up code in the fopt code path.
 *
 * 14-Aug-89 - Synced up hesupd with passwd
 *
 * 9-Aug-89	D. Long
 *	Do more checking on pasword form (CHECKCHARS).  Use optind for argv[1].
 *	Include privileged users on password length checks.  Don't allow chsh
 *	to work on restricted accounts.  Change SEC_TRANS to SEC_UPGRADE.
 *
 * 19-Jul-89 - D. Long
 *	Set seed random number generator before making salt.
 */

#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/svcinfo.h>
#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <sgtty.h>
#include <strings.h>
#include <pwd.h>
#include <grp.h>
#include <auth.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/lock.h>
#include <netinet/in.h>
#include <sys/param.h>
#ifdef AUTHEN
#include <krb.h>
#endif AUTHEN

#define	DEFSHELL		"/bin/sh"
#define	CHECKCHARS		3
#define ENTRY_SIZE		100
#define	AUD_BUF_LEN	(SYSCALL_MASK_LEN+TRUSTED_MASK_LEN)

#define         PASSCHG         1
#define         HESUPD_PORT     800
#define         HESUPDACK       6
#define         HESUPDNAK       9
#define         HESRETRY        3
                                                
char bindmaster[] = "bindmaster";
char defhome[] = "/var/dss/namedb/src";
char *homebase = defhome;
struct svcinfo *svcp;
int root=0;
char *index();

struct hesupdmsg {
                char    newcrypt[64];
                int     opcode;
                int     hesuid;
                char    oldpwd[32];
                };

                                                                     
#define	NUM_CHOICES	5
#define	PASSWD_DB	"/etc/passwd"
#define	ETCSHELLS	"/etc/shells"

extern int min_pw_len, max_pw_len, soft_exp, sec_level;
/*
  Privileged program for changing a users password.
*/

char *progname = "passwd";
static char salt_table[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./";

/*
  Function to read a password from the users terminal with character
  echoing turned off.
*/
static int setmodes = 0;
static struct sgttyb modes;
static int flags, fid;
static struct passwd *pwd = (struct passwd *) 0;
static struct sockaddr remote_hostaddr;
static int remote_hostflag = 0;

restoremodes(sig, code, scp)
int sig, code, *scp;
{
	if(setmodes) {
/*
  Restore original tty modes.
*/
		modes.sg_flags = flags;
		ioctl(fid, TIOCSETP, &modes);
		setmodes = 0;
	}
	exit(3);
}

unsigned char *getpass(prompt)
char *prompt;
{
	int last;
	unsigned char *cp;
	static unsigned char buff[80];

	bzero(buff, sizeof buff);
/*
  Get the current terminal modes.
*/
	fid = fileno(stdin);
	ioctl(fid, TIOCGETP, &modes);
	flags = modes.sg_flags;
/*
  If echo is on, turn it off, prompt for a password, read the password,
  and restore echo mode.
*/
	if(flags & ECHO) {
/*
  Turn off echo.
*/
		setmodes = 1;
		modes.sg_flags &= ~ECHO;
		ioctl(fid, TIOCSETP, &modes);
/*
  Prompt for and read the password.
*/
		fputs(prompt, stdout);
		cp = (unsigned char *) fgets(buff, sizeof buff, stdin);
/*
  Restore original tty modes.
*/
		modes.sg_flags = flags;
		ioctl(fid, TIOCSETP, &modes);
		setmodes = 0;
	} else {
/*
  Echo was already off.
  Prompt for and read the password.
*/
		fputs(prompt, stdout);
		cp = (unsigned char *) fgets(buff, sizeof buff, stdin);
	}
	if(cp == NULL)
/*
  Error reading input.
*/
		exit(1);
/*
  Clean up the password.
*/
	last = strlen(buff) - 1;
	if(buff[last] == '\n')
		buff[last] = '\0';
	return buff;
}

/*
  Function for converting to lower case alphabetics.
*/
char lcase(c)
char c;
{
	if(c >= 'A' && c <= 'Z')
		return c|040;
	else
		return c;
}

/*
 * Function to match a shell name against a pathname.
 */
path_match(path, shell)
char *path, *shell;
{
	char *cp;

	cp = strchr(path, '\n');
	if(cp)
		*cp = '\0';
	if(*shell != '/') {
		cp = strrchr(path, '/');
		if(!cp)
			cp = path;
		else
			cp++;
	} else
		cp = path;
	if(!strcmp(shell, cp)) {
		return 1;
	}
	return 0;
}

#include "../login.d/proto.h"	/* Extended protocol support.	*/
int eopt = 0;			/* Global for error macros. 	*/

/*
 * Print an error message.
 */
DO_ERROR(msg)
char *msg;
{
	int i;

	if (eopt) {
		REQ rbuf, *req = &rbuf;
		SENDREQ(req, ERROR, (msg), strlen((msg)));
		do {
			GETREQ(req, i);
			if(i <= 0)
				exit(2);
		} while (req->opcode != ACKNOWLEDGE);
	} else {
		fputs((msg), stderr);
		fflush(stderr);
	}
}

/*
 * Print an error message and exit.
 */
#define DO_EXIT(msg, stat) {						\
	audevent(msg, stat);						\
	DO_ERROR((msg)); 						\
	exit(stat);							\
}

/*
 * Generate an audit record.
 */
static void audevent(statustr, status)
char *statustr;
int status;
{
	char tmask[AUD_NPARAM];
	char *aud_arg[AUD_NPARAM];
	int i=0;

	tmask[i] = T_CHARP;
	aud_arg[i++] = progname;
	tmask[i] = T_CHARP;
	aud_arg[i++] = statustr;
/*
 * Determine if error or result token.
 */
	if(status == 0)
		tmask[i] = T_RESULT;
	else
		tmask[i] = T_ERROR;
	aud_arg[i++] = (char *) status;
/*
	if(remote_hostflag) {
		tmask[i] = T_HOSTADDR2;
		bcopy(remote_hostaddr.sa_data, aud_arg[i++], sizeof (long));
	}
*/
	if(pwd) {
		tmask[i] = T_CHARP;
		aud_arg[i++] = pwd->pw_name;
		tmask[i] = T_UID2;
		aud_arg[i++] = (char *) pwd->pw_uid;
	}
	tmask[i] = '\0';
/*
 * Generate audit record.
 */
        if(audgen(AUTH_EVENT, tmask, aud_arg) == -1 )
		perror("audgen" );
}


/*
  The main program.
*/
main(argc, argv)
int argc;
char *argv[];
{
	extern char *optarg;
	extern int optind;
	AUTHORIZATION *auth, *getauthuid(), *getauthuid_hesiod();
	PASSWORD old_pass, new_pass, gen_password[NUM_CHOICES];
	PASSWORD verify;
	CRYPT_PASSWORD crypt_pass;
	UID user_id, sso_id, our_id, euid;
	long period, deadline, now, time();
	int i, priv=0, owner=0, fopt=0, sopt=0, aopt=0, ypuid=0;
	char string[MAXPATHLEN+1], salt[3], *cp, *getlogin(), passave[9];
	unsigned char *cp2;
	char hyph_password[MAX_PASSWORD_LENGTH*2], *crypt(), *crypt16();
	char user_name[17], *usage;
	char *getentry();
	unsigned char *s;
        static struct hesupdmsg hupmsg;
#ifdef	CHECKCHARS
	int charset=0;
	static char chartab[256];
#endif
#ifdef AUTHEN
	char namebuf[ANAME_SZ];
	char *ptr;
#endif AUTHEN

	struct passwd *getpwnam_bind();
	struct group *gp;
	int auth_gid;
	int hesflg=0; /* set to initially no hesiod stuff */
	struct stat stat_buf;
	char audit_buf[AUD_BUF_LEN];


/* turn off auditing of all events except for AUTH events */

	if ( audcntl (SET_PROC_ACNTL, (char *)0, 0, AUDIT_AND, 0) == -1)
		perror ( "audcntl" );
	A_PROCMASK_SET ( audit_buf, AUTH_EVENT, 1, 1 );
	if (audcntl(SET_PROC_AMASK, audit_buf, AUD_BUF_LEN, 0, 0) == -1 )
		perror ( "audcntl" );

	progname = strrchr(argv[0], '/');
	if(progname)
		progname++;
	else
		progname = argv[0];
	signal(SIGINT, restoremodes);
	signal(SIGTERM, restoremodes);
	if(!strcmp(progname, "chfn")) {
		optind = 1;
		fopt = 1;
		usage = "usage: chfn [username]\n";
	} else if(!strcmp(progname, "chsh")) {
		optind = 1;
		sopt = 1;
		usage = "usage: chsh [username]\n";
	} else {
		usage = "usage: passwd [-aefs] [username]\n";
		while((i=getopt(argc, argv, "fsae")) != EOF)
			switch((char) i) {
			case 'f':
				fopt = 1;
				break;
			case 's':
				sopt = 1;
				break;
			case 'a':
				aopt = 1;
				break;
			case 'e':
				eopt = 1;
				break;
			case '?':
			default:
				DO_EXIT(usage, 1);
			}
	}
	if((eopt || aopt) && (sopt || fopt))
		DO_EXIT("passwd: illegal option combination.\n", 1);
	if(fopt && sopt)
		DO_EXIT("passwd: Only one of -f and -s allowed.\n", 1);
/*
  Check the command line.
*/
	if((argc-optind) > 1)
		DO_EXIT(usage, 1);
	our_id = getuid();
	if(our_id == 0)
		root=1;
/*
  Process the password configuration file.
*/
	config_auth();

#ifdef AUTHEN
        if((svcp = getsvc()) == NULL)
                {
                DO_EXIT("Cannot access security type\n", 2);
                }
        if(svcp->svcauth.seclevel >= SEC_UPGRADE)
                {
                for (i = 0 ; svcp->svcpath[SVC_AUTH][i] != SVC_LAST; i++)
                        if (svcp->svcpath[SVC_AUTH][i] == SVC_BIND) {
                                if(gethostname(namebuf, sizeof(namebuf)) == -1)
                                        {
                                        fputs("gethostname failure\n", stderr);
                                        }

			if((ptr = index(namebuf, '.')) != (char *)0)
				*ptr = '\0';

                        if(krb_svc_init("hesiod", namebuf, (char *)NULL, 0,
                                (char *)NULL, "/var/dss/kerberos/tkt/tkt.passwd")
                                        != RET_OK) {
                                fputs("Kerberos initialization failure\n", stderr);
                                }
                        }
                }
#endif AUTHEN

/*
  Use the login name as the username if none was suplied on the command line.
  Get the login name from the current real UID, not utmp.
*/
	if((argc-optind) > 0) {
		pwd = getpwnam(argv[optind]);
		if(!pwd && root) /* allow root to change a hesiod password */
			{
			root=2;
			pwd = getpwnam_bind(argv[optind]);
			hesflg=1;
			}
	} else
		pwd = getpwuid(our_id);
	if(svc_lastlookup == SVC_BIND)
		{
		hesflg=1;
		if(root)
			root=2;
		}
	if(svc_lastlookup == SVC_YP)
		{
		ypuid=1;	
		if(sec_level == SEC_UPGRADE)
			DO_EXIT("YP cannot be used in UPGRADE mode.\n", 2);
		}
	if(!pwd) 
		DO_EXIT("User not found in passwd data base.\n", 2);
	if(!pwd->pw_name)
		DO_EXIT("Error, no username.\n", 2);

	strncpy(user_name, pwd->pw_name, sizeof user_name);
	strncpy(passave, pwd->pw_passwd, sizeof passave);
/*
  The auth file is keyed by UID.  Use the usernames UID retrieved from the
  passwd file as the key.
*/
	user_id = pwd->pw_uid;
	if(user_id == our_id)
		owner = 1;
	else
		owner = 0;
/*
  Make sure we are talking to a tty line if not using extended protocol.
*/
	if (!eopt && !fopt && !sopt) {
		if(!freopen("/dev/tty", "w", stdout)) 
			DO_EXIT("Unable to write /dev/tty.\n", 2);
		if(!freopen("/dev/tty", "r", stdin)) 
			DO_EXIT("Unable to read /dev/tty.\n", 2);
	}
/*
  By definition the SSO is the owner of the auth file. (or the passwd file
  if we are running BSD compatible).
*/
	if(sec_level < SEC_UPGRADE) {
		if(stat(PASSWD_DB, &stat_buf) != 0) 
			DO_EXIT("Cannot stat passwd file.\n", 3);
	} else if(stat("/etc/auth.dir", &stat_buf) != 0) 
			DO_EXIT("Cannot stat auth file.\n", 3);
	sso_id = stat_buf.st_uid;
	euid = geteuid();
/*
	if(euid != sso_id) 
		DO_EXIT("Bad ownership on auth data base or passwd command.\n", 3);
*/
	if(our_id == sso_id || our_id == 0)
		priv = 1;
	else
		priv = 0;
/*
  Make sure the user is authorized to attempt changing this password.
*/
	if((!owner && !priv) || (!priv && eopt))
		DO_EXIT("Insufficient privilege.\n", 1);
	umask(022);
	if(sopt) {
		char shell[MAXPATHLEN+1], line[MAXPATHLEN+1];
		FILE *fp;

		if(!(fp=fopen(ETCSHELLS, "r"))) {
			char buf[MAXPATHLEN+50];

			strcpy(buf, "Unable to open ");
			strcat(buf, ETCSHELLS);
			strcat(buf, "\n");
			DO_EXIT(buf, 2);
		}
		fputs("Changing login shell for ", stdout);
		puts(pwd->pw_name);
		if(!priv && *pwd->pw_shell) {
			int ok = 0;

			while(fgets(line, sizeof line, fp)) {
				if(path_match(line, pwd->pw_shell)) {
					ok = 1;
					break;
				}
			}
			if(!ok) {
				char buf[MAXPATHLEN+50];

				strcpy(buf, "Cannot change from restricted shell ");
				strcat(buf, pwd->pw_shell);
				strcat(buf, "\n");
				DO_EXIT(buf, 1);
			}
			rewind(fp);
		}
		printf("Shell [%s]: ", pwd->pw_shell);
		if(!fgets(shell, sizeof shell, stdin)) {
			DO_EXIT("Login shell unchanged.\n", 0);
		}
		cp = strchr(shell, '\n');
		if(cp)
			*cp = '\0';
		if(!*shell) {
			DO_EXIT("Login shell unchanged.\n", 0);
		}
		while(fgets(line, sizeof line, fp)) {
			if(path_match(line, shell)) {
				strcpy(shell, line);
				break;
			}
		}
		if(strcmp(shell, line)) {
			if(priv) {
				DO_ERROR("Warning, shell not in /etc/shells\n");
			} else
				DO_EXIT("Invalid shell selection, shell unchanged.\n", 1);
		}
		if (access(shell, X_OK) < 0) {
			char buf[MAXPATHLEN+50];

			strcpy(buf, shell);
			strcat(buf, " is unavailable.\n");
			DO_EXIT(buf, 1);
		}
		if(!strcmp(shell, DEFSHELL))
			strcpy(shell, "");
		pwd->pw_shell = shell;
		if(storepwent(pwd)) {
			DO_EXIT("Shell not changed.\n", 1);
		} else
			audevent("Shell successfully changed.", 0);
	}
	if(fopt) {
		char *name, *office, *ophone, *hphone;
		char newname[ENTRY_SIZE], newoffice[ENTRY_SIZE], newophone[ENTRY_SIZE], newhphone[ENTRY_SIZE];
		char line[1024];

		fputs("Changing finger information for ", stdout);
		puts(pwd->pw_name);
		name = office = ophone = hphone = "";
		name = pwd->pw_gecos;

		if(name) {
		office = strpbrk(name, ",:\n");
		if(office) {
			if(*office == ',') {
				*office = '\0';
				office++;
				ophone = strpbrk(office, ",:\n");
				if(ophone) {
					if(*ophone == ',') {
						*ophone = '\0';
						ophone++;
						hphone = strpbrk(ophone, ",:\n");
						if(hphone) {
							if(*hphone == ',') {
								*hphone = '\0';
								hphone++;
							}
						} else
							hphone = "";
					}
				} else
					ophone = "";
			}
		} else
			office = "";
		}

		name = getentry("Name", name, newname, ENTRY_SIZE);
		office = getentry("Office number", office, newoffice, ENTRY_SIZE);
		ophone = getentry("Office Phone", ophone, newophone, ENTRY_SIZE);
		hphone = getentry("Home Phone", hphone, newhphone,  ENTRY_SIZE);
		sprintf(line, "%s,%s,%s,%s", name, office, ophone, hphone);
		pwd->pw_gecos = line;
		if(storepwent(pwd)) {
			DO_EXIT("Finger information not changed.\n", 1);
		} else
			audevent("Finger information successfully changed.", 0);
	}
	if(!sopt && !fopt) {
		if (!eopt) {
			fputs("Changing password for ", stdout);
			puts(pwd->pw_name);
		}
		umask(077);
		now = time((long *) 0);
/*
  If we are running with C2 or less security look for the password first in the
  passwd data base.
*/
		if(sec_level >= SEC_UPGRADE) {
/*
  Open the auth file and get the users auth entry using their UID as the key.
*/
			auth = getauthuid(user_id);
			if(auth == (AUTHORIZATION *) NULL && root == 2)
				auth = getauthuid_hesiod(user_id);
			if(auth == (AUTHORIZATION *) NULL) {
                                char buf[256];
                                sprintf(&buf[0], "Unable to retrieve auth information for %s.\n", user_name);
                                DO_EXIT(&buf[0], 2);
			}
		} else
			auth = (AUTHORIZATION *) NULL;
		if(ypuid && (sec_level == SEC_ENHANCED))  
			{  /* wierd case: Yp served uid at ENHANCED level */
			hesflg=1;	/* Assume hesiod served auth */
			if(root)
				root=2;
			}
/*
  Retrieve old encrypted password.
*/
		if(sec_level == SEC_BSD ||
		    sec_level <= SEC_UPGRADE && pwd->pw_passwd && strcmp(pwd->pw_passwd, "*"))
			strncpy(crypt_pass, pwd->pw_passwd, sizeof crypt_pass);
		else
			strncpy(crypt_pass, auth->a_password, sizeof crypt_pass);
/*
  If the invoker is not privileged (SSO or ROOT) require re-authentication,
  verify that they have CHANGE_PASSWORD privilege, and check the minimum
  expiration time.
*/
		if(!priv) {
/*
  Check CHANGE_PASSWORD privilege.
*/
			if(auth && !(auth->a_authmask & A_CHANGE_PASSWORD)) 
				DO_EXIT("You do not have the privilege to change your password.\n", 1);
		}
/*
  Check minimum password expiration time.
*/
		if(!priv && auth) {
			deadline = auth->a_pass_mod;
			period = auth->a_pw_minexp;
			deadline += period;
			if(now < deadline)
				DO_EXIT("Sorry, the minimum password lifetime has not expired yet.\n", 1);
		}

/*
  Reauthenticate.
*/
		if(!priv && *crypt_pass) {
			strncpy(salt, crypt_pass, 2);
			salt[2] = '\0';
			s = getpass("Old password: ");
			strncpy(old_pass, s, sizeof (PASSWORD));
			old_pass[MAX_PASSWORD_LENGTH] = '\0';
			putchar('\n');
			if(sec_level == SEC_BSD ||
			    sec_level <= SEC_UPGRADE && pwd->pw_passwd && strcmp(pwd->pw_passwd, "*"))
				cp = crypt(old_pass, salt);
			else
				cp = crypt16(old_pass, salt);
			if(!hesflg)
			 	bzero(old_pass, sizeof old_pass);
			if(strcmp(crypt_pass, cp)) {
				audevent("Failed authentication", 1);
				sleep(2);
				puts("Sorry");
				exit(1);
			}
		}

/*
 * The current prompter protocol does not support reauthentication; this
 * forces 'passwd -e' to be invokable only by root and to kludge the use of
 * priv when in extended protocol mode.  This has the side affect of including
 * root in further checks if 'passwd -e root'.
 */
		if (eopt) {
			priv = 0;
		}
/*
  Get the new password.
*/
		if(!auth) {
			max_pw_len = 8;
			if(min_pw_len > max_pw_len)
				min_pw_len = max_pw_len;
		} else
			if(!priv && !(auth->a_authmask & A_ENTER_PASSWORD))
				aopt = 1;

		if (eopt) {
			static REQ	request;
			REQ		*req = &request;
			LISTOFPAIRS	pairs;
			LISTOFPAIRS	*pair_list 	= &pairs;
			LISTOFPASSWORDS *passwd_list	= (LISTOFPASSWORDS *)req->data;
			int length;
	
			makeseed(&i, 4);
			srandom(i);
			
			/* Get password */
			do {
				bzero(pair_list, sizeof(LISTOFPAIRS));
				pair_list->count = RANDOM_WORDS;
				for(i=0; i < RANDOM_WORDS; i++) {
					randomword(pair_list->pairs[i].passwd.data, 
						pair_list->pairs[i].phonetic.data, 
						min_pw_len, max_pw_len, 0);
					pair_list->pairs[i].passwd.length = 
						strlen(pair_list->pairs[i].passwd.data);
					pair_list->pairs[i].phonetic.length = 
						strlen(pair_list->pairs[i].phonetic.data);
				}
				SENDREQ(req, CHZPWD, pair_list, sizeof(LISTOFPAIRS));
	
again:				SENDREQ(req, GETPWD, (char *)0, 0);
				GETREQ(req, i);
				if(i <= 0)
					exit(2);
				if (req->opcode != PASSWD) {
					DO_ERROR("Did not receive PASSWD request.\n");
					goto again;
				}

				if (passwd_list->count && auth && !(auth->a_authmask & A_ENTER_PASSWORD)) {
					for (i=0; i < RANDOM_WORDS; i++) {
						if (!strcmp(passwd_list->passwords[0].data, 
							pair_list->pairs[i].passwd.data))
							break;
					}
					if(i >= RANDOM_WORDS) {
						DO_ERROR("You must select from the list shown.\n");
						goto again;
					} 
				}
			} while (passwd_list->count == 0);

			/* Save password and verify string */
			length = passwd_list->passwords[0].length;
			if(length > MAX_PASSWORD_LENGTH)
				length = MAX_PASSWORD_LENGTH;
			strncpy(new_pass, passwd_list->passwords[0].data, 
				length);
			new_pass[length] = '\0';
			length = passwd_list->passwords[1].length;
			if(length > MAX_PASSWORD_LENGTH)
				length = MAX_PASSWORD_LENGTH;
			strncpy(verify, passwd_list->passwords[1].data, 
				length);
			s = (unsigned char *) passwd_list->passwords[0].data;

			/* Clear temp space */
			bzero(pair_list, sizeof(LISTOFPAIRS)); 

		} else if (aopt) {
			makeseed(&i, 4);
			srandom(i);
			do {
				puts("Here are some suggested passwords:\n");
				for(i=0; i < NUM_CHOICES; i++) {
					randomword(gen_password[i], hyph_password, min_pw_len, max_pw_len, 0);
					printf("%-16s %-16s\n", gen_password[i], hyph_password);
				}
				s = getpass("\nEnter new password: ");
				strncpy(new_pass, s, sizeof (PASSWORD));
				new_pass[MAX_PASSWORD_LENGTH] = '\0';
				putchar('\n');
				if(!priv && *new_pass && auth && !(auth->a_authmask & A_ENTER_PASSWORD)) {
					for(i=0; i < NUM_CHOICES; i++) {
						if(!strcmp(new_pass, gen_password[i]))
							break;
					}
					if(i >= NUM_CHOICES) {
						new_pass[0] = '\0';
						DO_ERROR("You must select from the list shown.\n");
					} else
						break;
				}
			} while(strlen(new_pass) == 0);
		} else {
			s = getpass("Enter new password: ");
			strncpy(new_pass, s, sizeof (PASSWORD));
			new_pass[MAX_PASSWORD_LENGTH] = '\0';
			putchar('\n');
		}
/*
  Check password length.
*/
		if(!*new_pass)
			DO_EXIT("Password unchanged.\n", 1);
		if((i=strlen(s)) < min_pw_len) {
			char buf[256];
			bzero(new_pass, sizeof new_pass);
			sprintf(&buf[0], "Password must be at least %d characters long, password unchanged.\n",
				min_pw_len);
			DO_EXIT(&buf[0], 1);
		}
		if(i > max_pw_len) {
			char buf[256];
			new_pass[max_pw_len] = '\0';
			if(sec_level <= SEC_BSD)
				sprintf(&buf[0], "Warning, only the first %d characters of the password are significant.\n",
					max_pw_len);
			else {
				sprintf(&buf[0], "%s, your password cannot be longer than %d characters.\n",
					eopt?"Error":"Warning", max_pw_len);
				if(eopt)
					DO_EXIT(&buf[0], 1);
			}
			DO_ERROR(&buf[0]);
		}
/*
  If the user is not privileged check to make sure their password meets
  some minimal requirements.
*/
		if(!priv) {
/*
  Make sure new password is not the same as old password.
*/
			if(*crypt_pass) {
				salt[0] = crypt_pass[0];
				salt[1] = crypt_pass[1];
				salt[2] = '\0';
				if(auth)
					cp = crypt16(new_pass, salt);
				else
					cp = crypt(new_pass, salt);
				if(!strcmp(crypt_pass, cp)) {
					bzero(new_pass, sizeof new_pass);
					DO_EXIT("Password is not different enough, unchanged.\n", 1);
				}
			}
/*
  Make sure the password does not equal the logname.
*/
			cp2 = new_pass;
			cp = pwd->pw_name;
			for(cp2=new_pass, cp=pwd->pw_name; *cp2 && *cp; cp2++, cp++)
				if(lcase(*cp) != lcase(*cp2))
					break;
			if(lcase(*cp) == lcase(*cp2))
				DO_EXIT("Password must be different than logname.\n", 1);
#ifdef		CHECKCHARS
/*
  Make sure password uses a rich enough character set.
*/
			for(cp2=new_pass; *cp2; cp2++)
				chartab[*cp2]++;
			for(i=0; i < sizeof chartab; i++)
				if(chartab[i])
					charset++;
			if(charset < CHECKCHARS) {
				char buf[256];
				bzero(new_pass, sizeof new_pass);
				bzero(chartab, sizeof chartab);
				charset = 0;
				sprintf(&buf[0], "Password must contain at least %d different characters.\n", CHECKCHARS);
				DO_EXIT(&buf[0], 1);
			}
#endif
		}
/*
  Verify password to prevent typos.
*/
		if (!eopt) {
			s = getpass("Verify: ");
			strncpy(verify, s, sizeof (PASSWORD));
			verify[MAX_PASSWORD_LENGTH] = '\0';
			putchar('\n');
			if((i=strlen(verify)) > max_pw_len)
				if(sec_level > SEC_BSD) {
					DO_EXIT("Error, password too long.\n", 1);
				} else
					verify[max_pw_len] = '\0';
		}
		if(strcmp(new_pass, verify))
			DO_EXIT("Verification failed, password unchanged.\n", 1);
		bzero(verify, sizeof verify);
/*
  Encrypt and store the new password.
*/
		srandom(time(0));
		salt[0] = salt_table[random() % 64];
		salt[1] = salt_table[random() % 64];
		if(sec_level == SEC_BSD || !auth)
			pwd->pw_passwd = crypt(new_pass, salt);
		else
			bcopy(crypt16(new_pass, salt), auth->a_password, CRYPT_PASSWORD_LENGTH);
		bzero(new_pass, sizeof new_pass);
/*
  If the user is privileged set the expiration periods.
*/
#ifdef	XYZ
		if(!eopt && priv && auth) {
/*
  Set the maximum expiration period.
*/
			printf("Enter expiration period in days (RET for %1d, 0 for none): ",
				DEF_EXP_PERIOD/60/60/24);
			fgets(string, sizeof string, stdin);
			if(string[0] == '\n')
				period = DEF_EXP_PERIOD;
			else
				period = atoi(string)*60*60*24;
			auth->a_pw_maxexp = period;
/*
  Set the minimum expiration period.
*/
			fputs("Enter minimum password lifetime in minutes (RET for 0): ", stdout);
			fgets(string, sizeof string, stdin);
			if(string[0] == '\n')
				period = 0;
			else
				period = atoi(string)*60;
			auth->a_pw_minexp = period;
		}
#endif
/************************************************************************/
/*                                                                      */
/*      The hesupd call processing					*/
/*                                                                      */
/************************************************************************/
		if(hesflg)
			{
			signal(SIGHUP, SIG_IGN);
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
			(void) umask(0);
		        bzero((char *)&hupmsg, sizeof(struct hesupdmsg));
	        	strcpy(hupmsg.oldpwd,old_pass);
			hupmsg.hesuid = pwd->pw_uid;
			hupmsg.opcode=PASSCHG;
			if(auth)
				strcpy(hupmsg.newcrypt,auth->a_password);
			else	strcpy(hupmsg.newcrypt,pwd->pw_passwd);
	        	if(hupdtp(&hupmsg))
				{
	                	DO_EXIT("Your distributed password is updated\n", 0);
				}
	        	else    {
				DO_EXIT("Your distributed password has not been changed\n", 2);
				}
			exit(0);
			/*****end of hesupd processing*********/
			}
		if(auth) {
/*
  Set the password modification time to now and store the new auth
  entry.
*/
			auth->a_pass_mod = now;
			if(storeauthent(auth))
				DO_EXIT("Unable to set new password.\n", 2);
			endauthent();
			if(gp=getgrnam("authread"))
				auth_gid = gp->gr_gid;
			else
				auth_gid = 9;
			if(our_id == 0)
				i = chown(AUTHORIZATION_DB, sso_id, auth_gid);
			else
				i = chown(AUTHORIZATION_DB, -1, auth_gid);
			if(i >= 0)
				chmod(AUTHORIZATION_DB, 0640);
			if(strcmp(passave, "*")) {
				pwd->pw_passwd = "*";
				umask(022);
				if(storepwent(pwd))
					DO_EXIT("Unable to change passwd file.\n", 2);
			}
		} else {
			umask(022);
			if(storepwent(pwd))
				DO_EXIT("Unable to set password.\n", 2);
		}
		if(aopt && !eopt)
			DO_ERROR("Remember to destroy any printed passwords.\n");
		audevent("Password successfully changed.", 0);
	}
	exit(0);
}
/************************************************************************/
/*									*/
/*	hesupd transaction processing module				*/
/*									*/
/************************************************************************/
int
hupdtp(hup)	
	struct hesupdmsg *hup;
{
	static struct sockaddr sock, sock2;
        struct hostent *hp;
	char host[34];
        struct sockaddr_in sin;
	struct  servent *sp;            /* service spec for tcp/hesupd */
        int hlen = 32;        
	int net, socklen, amount, i, t, j, retmsg=HESUPDNAK;
	int ans, port, retry;
	/* first check to see if we are the bind master */
        gethostname(host,hlen);
	hp = gethostbyname(bindmaster);
        if(hp == 0)
		return(0); /* cant lookup the bindmaster hostname */
        if(!strcmp(host,hp->h_name) && root == 2 )
		{	/* bingo - I am root and this is the bindmaster */
        	if (chdir(homebase) < 0) {
                	fprintf(stderr, "passwd can't chdir to %s\n",homebase);
                	return(0);
        		}
        	switch(svcp->svcauth.seclevel)
			{
			case	SEC_BSD:
                        	return(chpw_bsd(hup->hesuid,hup->oldpwd,hup->newcrypt));
			case	SEC_UPGRADE:
                        	return(chpw_upgrade(hup->hesuid,hup->oldpwd,hup->newcrypt));
			case	SEC_ENHANCED:
                        	return(chpw_enhanced(hup->hesuid,hup->oldpwd,hup->newcrypt));
			}
		}
	mix(hup);

	/*****						*****/
	/*****   Get port number of hesupd		*****/
	/*****						*****/
	sp=getservbyname("hesupd","tcp");
        if (sp == 0) {
                DO_ERROR("hesupd/tcp: unknown service\n");
                return(0);
        }
	/*****						*****/
	/*****    Get Bind master hostname		*****/
	/*****						*****/
	hp = gethostbyname(bindmaster);
	if (hp == NULL) {
		DO_ERROR("No registered hesiod master\n");
		return(0);
	}
	/*****						*****/
	/***** 		Setup the socket		*****/
	/*****						*****/
	bzero((char *)&sin, sizeof (sin));
        bcopy(hp->h_addr,(char *)&sin.sin_addr, hp->h_length);
/* Save the remote address for auditing */
/*
	bcopy(hp->h_addr, remote_hostaddr.sa_data, sizeof (long));
	remote_hostaddr.sa_family = hp->h_addrtype;
	remote_hostflag = 1;
*/

	sin.sin_family = hp->h_addrtype;
	sin.sin_port = sp->s_port;
	/*****						*****/
	/***** Try HESRETRY times to connect to hesupd	*****/
	/*****						*****/
	for(retry=HESRETRY; retry != 0; retry-- )
		{
        	net = socket(AF_INET, SOCK_STREAM, 0);
        	if (net < 0) 
			{
            		DO_ERROR("hesupd: socket error\n");
            		return(0);
			}
		/*****					*****/
		/***** Make connection to Hesupd	*****/
		/*****					*****/
		if (connect(net, (struct sockaddr *)&sin, sizeof (sin)) < 0) 
			{
			(void) close(net);
			net = -1;
			continue;
			}
		/*****					*****/
		/***** Send out hesupd message		*****/
		/*****					*****/
		amount = send(net, (char *) hup, sizeof(struct hesupdmsg), 0);
 		if(amount < sizeof(struct hesupdmsg))
			{
			(void) close(net);
			DO_ERROR("hesupd send error\n");
			return(0);
			}
		/*****					*****/
		/*****	Receive Ack or Nak from Hesupd  *****/
		/*****  This will hang if hesupd dies   *****/
		/*****					*****/
     		amount = recv(net, (char *)&retmsg, sizeof(retmsg), 0);
                if(amount < sizeof(retmsg))
                        {
			(void) close(net);
                        DO_ERROR("hesupd recv error\n");
                        return(0);
                        }
		else	break;
		}
	(void) close(net);
	net = -1;
	if(retmsg == HESUPDACK)
		return(1);
	return(0);
}


/**** stir and shake ****/
mix(hup)
struct hesupdmsg *hup;
{
	unsigned char *hesbuf= (unsigned char *) hup;
	unsigned char tmp1=0,tmp=0;
	int i,j,len;
	len = sizeof(struct hesupdmsg) - 1;
	for (i=0,j=11; i <=len; i++,j++)
		hesbuf[i]+= j % 15;
	tmp = (( hesbuf[0] >> 3) | (hesbuf[len] << 5));
	for (i=len;i > 0; i--)
		{
		hesbuf[i]= (( hesbuf[i] >> 3) | (hesbuf[i-1] << 5 ));
		}
	hesbuf[0] = tmp;
	for (i=0,j=3; i <=len; i++,j++)
		hesbuf[i]+= j % 5;

}


/*
 * Get a gecos entry from stdin.
 */
char *
getentry(prompt, def, buf, size)
char *prompt;
char *def;
char *buf;
int size;
{
	char *fgetline();

	do {
		(void) fprintf(stdout, "%s [%s]: ", prompt, def);
		(void) fgetline(buf, size, stdin);
	} while (checkentry(buf));

	if (*buf) {
		if (!strcmp(buf, "none")) bzero(buf, size);
		return(buf);
	} 
	return(def);
}


/*
 * Check validity of gecos entry.
 */
checkentry(s)
char *s;
{
	char *cp;

	if (cp = strchr(s, '\n')) *cp = '\0';
	for (cp = s; *cp; cp++) {
		if (!isprint(*cp) || *cp == ':' || *cp == ',') {
			DO_ERROR("Illegal character in string.\n");
		return(1);
		}
	}
	return(0);
}


/*
 * fgetline is a modified fgets that will read a line of input
 * saving n characters in the buffer pointed to by s, as does
 * fgets, but will continue to read characters up to a newline.
 * n+ characters read, are discarded. 
 */
char *
fgetline(s, n, iop)
char *s;
register int n;
register FILE *iop;
{
	register c;
	register char *cs;
	
	cs = s;
	while ((c = getc(iop)) >= 0) {
		if (--n>0) *cs++ = c;
		if (c == '\n')
			break;
	}
	if (c<0 && cs==s)
		return(NULL);
	*cs++ = '\0';
	return(s);
}

/************************************************************************
 *                                                                      *
 *	Chpw_bsd checks the received password to assure its validity    *
 *	with respect to the associated uid. If all is well the new	*
 *	encrypted password string will replace the old encrypted        *
 *	password string in the homebase/passwd file. Hesiod make_passwd *
 *	is then called to update the hesiod.db. Once a new hesiod.db is *
 *	created the local "MASTER" named is restarted which starts the  *
 * 	distribution of the new password database.			*
 *									*
 ************************************************************************/

int
chpw_bsd(ouid,opwd,ncrypt)
uid_t	ouid;		/* uid of user requesting a password change 	*/
char	*opwd;		/* Old passowrd of user(uid)			*/
char	*ncrypt;	/* New encrypted password of user(uid)		*/
{	
        FILE *tempfp, *filefp, *fp;
        int tempfd, i, len;
	int retval=0;
        void (*f1)(), (*f2)(), (*f3)();
        char buf[256], *p;
	char *pwptr;
        char cmdbuf[1024];
	char pwdbuf[64];
	struct passwd *pwent, *getpwuid_bind();
	pwent = getpwuid_bind(ouid);
	if(pwent == NULL) 
		return(retval);
	/***								 ***/
	/*** We now assume that the ouid is valid and set the new passwd ***/
	/***								 ***/
	        (void) umask(0);

        f1 = signal(SIGHUP, SIG_IGN);
        f2 = signal(SIGINT, SIG_IGN);
        f3 = signal(SIGQUIT, SIG_IGN);
        tempfd = open("hesupd_passwd", O_WRONLY|O_CREAT|O_EXCL, 0644);
        if (tempfd < 0) {
                	goto cleanup_noclose;
			}
        signal(SIGTSTP, SIG_IGN);
        if ((tempfp = fdopen(tempfd, "w")) == NULL) {
                goto cleanup;
        }
	
	/*
	 * Prepare to make new passwd file copy
	 * with new password.
	 */
	if ((filefp = fopen("passwd", "r")) == NULL) {
		goto cleanup;
	}
	/*				*/
	/* copy and check new password  */
 	/* into pwent struct		*/
	/*				*/
	bzero(pwdbuf, sizeof(pwdbuf));
        for (p = ncrypt, pwptr = pwdbuf; (*p != '\0'); p++,pwptr++)
		{
                if ((*p == ':') || !(isprint(*p)))
                        *pwptr = '$';       /* you lose buckwheat */
		else *pwptr = *p;	    /* the SUN way of doing it */
		}
	/*				*/
	/* copy and modify passwd file  */
	/*				*/
	len = strlen(pwent->pw_name);
	while (fgets(buf, sizeof(buf), filefp)) {
		p = index(buf, ':');
		if (p && p - buf == len
		    && strncmp(pwent->pw_name, buf, p - buf) == 0) {
			fprintf(tempfp,"%s:%s:%d:%d:%s:%s:%s\n",
			    pwent->pw_name,
			    pwdbuf,
			    pwent->pw_uid,
			    pwent->pw_gid,
			    pwent->pw_gecos,
			    pwent->pw_dir,
			    pwent->pw_shell);
		}
		else
			fputs(buf, tempfp);
	}
	bzero(pwdbuf, sizeof(pwdbuf));
	fclose(filefp);
	fclose(tempfp);
	/*				*/
	/* copy in new passwd file	*/
	/*				*/
	(void) umask(022);
	strcpy(cmdbuf,"cp ");
	strcat(cmdbuf,"hesupd_passwd");
	strcat(cmdbuf," ");
	strcat(cmdbuf,"passwd");
	system(cmdbuf);
	unlink("hesupd_passwd");
	/*				*/
	/* create the new hesiod.db 	*/
	/*				*/
	bzero(cmdbuf,sizeof(cmdbuf));
	strcpy(cmdbuf,"/var/dss/namedb/bin/make_passwd ");
	strcat(cmdbuf,"/var/dss/namedb/src");
	strcat(cmdbuf,"/passwd ");
	strcat(cmdbuf,"/var/dss/namedb/passwd.db");
	system(cmdbuf);
	/*				*/
	/* distribute the new hesiod.db */
	/*				*/
	bzero(cmdbuf,sizeof(cmdbuf));
	strcpy(cmdbuf,"/var/dss/namedb/bin/restart_named");
	system(cmdbuf);
	sleep(10);
	retval=1;
    cleanup:
	fclose(tempfp);
    cleanup_noclose:
        signal(SIGHUP, f1);
        signal(SIGINT, f2);
        signal(SIGQUIT, f3);
	return(retval);
}

/************************************************************************
 *                                                                      *
 *	Chpw_trans checks the received password to assure its validity  *
 *	with respect to the associated uid. If all is well the new	*
 *	encrypted password string will replace the old encrypted        *
 *	password string in the homebase/passwd file. Hesiod make_passwd *
 *	is then called to update the hesiod.db. Once a new hesiod.db is *
 *	created the local "MASTER" named is restarted which starts the  *
 * 	distribution of the new password database.			*
 *									*
 ************************************************************************/
int
chpw_upgrade(ouid, opwd, ncrypt)
uid_t	ouid;		/* uid of user requesting a password change 	*/
char	*opwd;		/* Old passowrd of user(uid)			*/
char	*ncrypt;	/* New encrypted password of user(uid)		*/
{	
        FILE *afp=NULL, *oafp, *pfp=NULL, *opfp;
	long now, time();
        int pfd, afd, i, len, upgrade=0;
	int retval=0;
        void (*f1)(), (*f2)(), (*f3)();
        char buf[2048], *p, *pwptr;
	CRYPT_PASSWORD newpass, crypt_pass;
	char (*fp)();
        char cmdbuf[BUFSIZ];
	struct passwd *pwent, *getpwuid_bind();
	AUTHORIZATION *authent, *getauthuid(), *getauthuid_hesiod();

	pwent = getpwuid_bind(ouid);
	if(pwent == NULL)  {
		return(retval);
	}
	authent = getauthuid_hesiod(ouid);
	if(authent == NULL) {
		return retval;
	}
	if(!strcmp(pwent->pw_passwd, "*")) {
		pwptr = authent->a_password;
	} else {
		pwptr = pwent->pw_passwd;
		upgrade = 1;
		}
	if(!(authent->a_authmask & A_CHANGE_PASSWORD)) {
		return(retval);
	}
	now = time(0);
	if(now < authent->a_pass_mod+authent->a_pw_minexp) {
		return retval;
	}
/***								 ***/
/*** We now assume that the ouid is valid and set the new passwd ***/
/***								 ***/
	        (void) umask(0);

        f1 = signal(SIGHUP, SIG_IGN);
        f2 = signal(SIGINT, SIG_IGN);
        f3 = signal(SIGQUIT, SIG_IGN);
/*
 * Lock the auth data base.
 */
        afd = open("hesupd_auth", O_WRONLY|O_CREAT|O_EXCL, 0600);
        if (afd < 0) {
		fputs("auth file busy - try again.\n", stderr);
                goto cleanup;
	}
	afp = fdopen(afd, "w");
	if(afp == NULL) {
		goto cleanup;
	}
	if ((oafp = fopen("auth", "r")) == NULL) {
		goto cleanup;
	}
/*
 * Lock the passwd data base if we need to.
 */
	if(upgrade) {
		pfd = open("hesupd_passwd", O_WRONLY|O_CREAT|O_EXCL, 0644);
		if(pfd < 0) {
			goto cleanup;
		}
		pfp = fdopen(pfd, "w");
		if(pfp == NULL) {
			goto cleanup;
		}
		if((opfp = fopen("passwd", "r")) == NULL) {
			goto cleanup;
		}
	}
        signal(SIGTSTP, SIG_IGN);
/*
 * Make a local copy of the new password checking to make sure it's
 * not garbage.
 */
	len = strlen(ncrypt);
	if(len >= sizeof newpass) {
		goto cleanup;
	}
	pwptr = newpass;
	p = ncrypt;
	for(p=ncrypt; (*p != '\0'); p++) {
                if ((*p == ':') || !(isprint(*p)))
                        *pwptr++ = '$';       /* you lose buckwheat */
		else
			*pwptr++ = *p;
	}
	*pwptr = '\0';
	bcopy(newpass,authent->a_password, CRYPT_PASSWORD_LENGTH);
	authent->a_pass_mod = now;
/*
 * Copy and modify the auth file
 */
	while (fgets(buf, sizeof(buf), oafp)) {
		p = index(buf, ':');
		if (p && atoi(buf) == authent->a_uid) {
			fputs(asciiauth(authent), afp);
			putc('\n', afp);
		} else
			fputs(buf, afp);
	}
	fclose(oafp);
	fclose(afp);
	afp = NULL;
/*
 * Copy and modify the password file if we need to upgrade this entry.
 */
	len = strlen(pwent->pw_name);
	if(upgrade) {
		while (fgets(buf, sizeof(buf), opfp)) {
			p = index(buf, ':');
                if (p && p - buf == len
                    && strncmp(pwent->pw_name, buf, p - buf) == 0) 
				fprintf(pfp, "%s:*:%d:%d:%s:%s:%s\n",
				    pwent->pw_name,
				    pwent->pw_uid,
				    pwent->pw_gid,
				    pwent->pw_gecos,
				    pwent->pw_dir,
				    pwent->pw_shell);
			else
				fputs(buf, pfp);
		}
		fclose(opfp);
		fclose(pfp);
		pfp = NULL;
	}
/*
 * Copy in new auth file
 */
	(void) umask(077);
	strcpy(cmdbuf,"cp ");
	strcat(cmdbuf,"hesupd_auth");
	strcat(cmdbuf," ");
	strcat(cmdbuf,"auth");
	system(cmdbuf);
	unlink("hesupd_auth");
	(void) umask(022);
/*
 * Copy in new passwd file if necessary
 */
	if(upgrade) {
		strcpy(cmdbuf,"cp ");
		strcat(cmdbuf,"hesupd_passwd");
		strcat(cmdbuf," ");
		strcat(cmdbuf,"passwd");
		system(cmdbuf);
		unlink("hesupd_passwd");
	}
/*
 * Create the new passwd.db 
 */
	strcpy(cmdbuf, "/var/dss/namedb/bin/make_passwd ");
	strcat(cmdbuf, homebase);
	strcat(cmdbuf, "/passwd ");
	strcat(cmdbuf, "/var/dss/namedb/passwd.db");
	system(cmdbuf);
/*
 * Create the new auth.db 
 */
	strcpy(cmdbuf, "/var/dss/namedb/bin/make_auth ");
	strcat(cmdbuf, homebase);
	strcat(cmdbuf, "/auth ");
	strcat(cmdbuf, "/var/dss/namedb/auth.db");
	system(cmdbuf);
/*
 * Distrbute the new hesiod.db
 */
	strcpy(cmdbuf,"/var/dss/namedb/bin/restart_named");
	system(cmdbuf);
	sleep(10);
	retval=1;
cleanup:
	if(pfp)
		fclose(pfp);
	if(afp)
		fclose(afp);
        signal(SIGHUP, f1);
        signal(SIGINT, f2);
        signal(SIGQUIT, f3);
	return(retval);
}


/************************************************************************
 *                                                                      *
 *	Chpw_c2 checks the received password to assure its validity  	*
 *	with respect to the associated uid. If all is well the new	*
 *	encrypted password string will replace the old encrypted        *
 *	password string in the homebase/passwd file. Hesiod make_passwd *
 *	is then called to update the hesiod.db. Once a new hesiod.db is *
 *	created the local "MASTER" named is restarted which starts the  *
 * 	distribution of the new password database.			*
 *									*
 ************************************************************************/
int
chpw_enhanced(ouid, opwd, ncrypt)
uid_t	ouid;		/* uid of user requesting a password change 	*/
char	*opwd;		/* Old passowrd of user(uid)			*/
char	*ncrypt;	/* New encrypted password of user(uid)		*/
{	
        FILE *afp=NULL, *oafp;
	long now, time();
        int afd, i, len;
	int retval=0;
        void (*f1)(), (*f2)(), (*f3)();
        char buf[2048], *p, *pwptr;
	CRYPT_PASSWORD newpass;
        char cmdbuf[BUFSIZ];
	struct passwd *pwent, *getpwuid_bind();
	AUTHORIZATION *authent, *getauthuid_hesiod();

	pwent = getpwuid_bind(ouid);
	if(pwent == NULL)  {
		return(retval);
	}
	authent = getauthuid_hesiod(ouid);
	if(authent == NULL) {
		return retval;
	}
	pwptr = authent->a_password;
	if(!(authent->a_authmask & A_CHANGE_PASSWORD)) {
		return(retval);
	}
	now = time(0);
	if(now < authent->a_pass_mod+authent->a_pw_minexp) {
		return retval;
	}
/***								 ***/
/*** We now assume that the ouid is valid and set the new passwd ***/
/***								 ***/
	        (void) umask(0);

        f1 = signal(SIGHUP, SIG_IGN);
        f2 = signal(SIGINT, SIG_IGN);
        f3 = signal(SIGQUIT, SIG_IGN);
/*
 * Lock the auth data base.
 */
        afd = open("hesupd_auth", O_WRONLY|O_CREAT|O_EXCL, 0600);
        if (afd < 0) {
                goto cleanup;
	}
	afp = fdopen(afd, "w");
	if(afp == NULL) {
		goto cleanup;
	}
	if ((oafp = fopen("auth", "r")) == NULL) {
		goto cleanup;
	}
        signal(SIGTSTP, SIG_IGN);
/*
 * Make a local copy of the new password checking to make sure it's
 * not garbage.
 */
	len = strlen(ncrypt);
	if(len >= sizeof newpass) {
		goto cleanup;
	}
	pwptr = newpass;
	p = ncrypt;
	for(p=ncrypt; (*p != '\0'); p++) {
                if ((*p == ':') || !(isprint(*p)))
                        *pwptr++ = '$';       /* you lose buckwheat */
		else
			*pwptr++ = *p;
	}
	*pwptr = '\0';
	bcopy(newpass,authent->a_password, CRYPT_PASSWORD_LENGTH);
	authent->a_pass_mod = now;
/*
 * Copy and modify the auth file
 */
	while (fgets(buf, sizeof(buf), oafp)) {
		p = index(buf, ':');
		if (p && atoi(buf) == authent->a_uid) {
			fputs(asciiauth(authent), afp);
			putc('\n', afp);
		} else
			fputs(buf, afp);
	}
	fclose(oafp);
	fclose(afp);
	afp = NULL;
/*
 * Copy in new auth file
 */
	(void) umask(077);
	strcpy(cmdbuf,"cp ");
	strcat(cmdbuf,"hesupd_auth");
	strcat(cmdbuf," ");
	strcat(cmdbuf,"auth");
	system(cmdbuf);
	unlink("hesupd_auth");
	(void) umask(022);
/*
 * Create the new hesiod.db 
 */
	strcpy(cmdbuf, "/var/dss/namedb/bin/make_auth ");
	strcat(cmdbuf, homebase);
	strcat(cmdbuf, "/auth ");
	strcat(cmdbuf, "/var/dss/namedb/auth.db");
	system(cmdbuf);
/*
 * Distrbute the new hesiod.db
 */
	strcpy(cmdbuf,"/var/dss/namedb/bin/restart_named");
	system(cmdbuf);
	sleep(10);
	retval=1;
cleanup:
	if(afp)
		fclose(afp);
        signal(SIGHUP, f1);
        signal(SIGINT, f2);
        signal(SIGQUIT, f3);
	return(retval);
}
static AUTHORIZATION *getauthuid_hesiod(uid)
int uid;
{
	static AUTHORIZATION auth;
        char uidbuf[10], **pp;
	AUTHORIZATION *_auth = (AUTHORIZATION *) NULL;
        setent_bind(0);
        sprintf(uidbuf, "%u", uid);
        pp = (char **) hes_auth_resolve(uidbuf, "auth");
        endent_bind();
        if(pp != NULL)
                if(*pp) {
                        binauth(*pp, &auth);
                        while(*pp)
                                free(*pp++);
                        _auth = &auth;
                }
        else
                return(NULL);
        return _auth;
}
