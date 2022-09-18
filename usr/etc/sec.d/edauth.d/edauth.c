#ifndef lint
static  char    *sccsid = "@(#)edauth.c	4.3  (ULTRIX)        11/13/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989, 1990 by			*
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
/*
 * Modification History
 *
 * 04-Sep-1990	dlong
 *	Added "-p protouser" option, support for AUDIT_USER, and use the
 *	library routine getopt().
 *
 * 12-Nov-1990	scott
 *	Added AUDIT_USR to output_aud
 */

#include <sys/wait.h>
#include <sys/file.h>
#include <sys/audit.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <nlist.h>
#include <limits.h>
#include <stdio.h>
#include <pwd.h>
#include <auth.h>

#define AUD_BUF_LEN (SYSCALL_MASK_LEN+TRUSTED_MASK_LEN)
#define	SUCCEED		1
#define	FAIL		0

/*
 * Code to convert a date back into time(2) format.
 */
#define	dysize(A) (((A)%4)? 365: 366)

static	int	dmsize[12] =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*
 * Add up the seconds since the epoch
 */
long julian_second(year, month, day, hour, minute, second)
int year, month, day, hour, minute, second;
{
	long jtime=0;
	int i;

	if(year < 100)
		year += 1900;
	for (i = 1970; i < year; i++)
		jtime += dysize(i);
	/* 
	 * Leap year 
	 */
	if (dysize(year) == 366 && month >= 3)
		jtime++;
	/*
	 * Do the current year
	 */
	while (--month)
		jtime += dmsize[month-1];
	jtime += day-1;
	jtime = 24*jtime + hour;
	jtime = 60*jtime + minute;
	jtime = 60*jtime + second;
	return jtime;
}

/*
 * Module-wide definitions and declarations.
 */
typedef struct passwd PWD;
extern char *strchr();

static char *progname = "???";
static AUTHORIZATION authsave;

struct nlist nlst[] = {
    { "_nsysent" },
#define X_NSYSENT      0
    { 0 },
};

char *special_event[] = {
    "shmget",
    "shmdt",
    "shmctl",
    "shmat"
};

/*
 * Function to check a string and return success if all digits.
 */
static int alldigits(s)
register char *s;
{
	register c;

	c = *s++;
	do {
		if (!isdigit(c))
			return (0);
	} while (c = *s++);
	return (1);
}

/*
 * Function to convert a username or uid into a uid.
 */
static int getuser(user)
char *user;
{
	PWD *pwd;
	int uid;

/*
 * Look it up first to see if it's in the passwd file.
 */
	setpwent();
	pwd = getpwnam(user);
	if(!pwd) {
/*
 * If not in the passwd file see if it's an integer uid.
 */
		if(alldigits(user))
			uid = atoi(user);
		else
			uid = -1;
	} else
/*
 * Otherwise return the uid field from the passwd file entry.
 */
		uid = pwd->pw_uid;
	return uid;
}

/*
 * Function to remove leading and trailing white space from a string.
 * Returns a pointer to the first non-white character in the string.
 */
static char *trim(s)
char *s;
{
	char *start, *end, *strpbrk();
	int i;

/*
 * Skip over leading white space.
 */
	start = s + strspn(s, " \t");
	i = strlen(start);
/*
 * Step from back to front while still encountering white-space characters.
 */
	for(end=start+i-1; i; i--) {
		if(!strchr(" \t", *end))
			break;
		*end-- = '\0';
	}
/*
 * Return pointer to first non-white character in string.
 */
	return start;
}

extern char *strtok();

/*
 * Function to read a number using the string initialized in strtok().
 * If successful returns non-zero and stores the number in the integer
 * pointed to by the argument.  Otherwise returns zero and strores zero in
 * the location pointed to by the argument.
 */
static int getnum(i)
int *i;
{
	char *s;

	if(s=strtok(NULL, " \t\n")) {
		s = trim(s);
		if(*s && alldigits(s)) {
			*i = atoi(s);
			return SUCCEED;
		}
	}
	*i = 0;
	return FAIL;
}

/*
 * Internal function to safely get a line from the named stream.  If
 * the line is too long to fit into the buffer it is thrown away and a
 * new line is fetched until successful or end-of-file.
 */
static char *fgetline(string, len, file)
char *string;
int len;
FILE *file;
{
	register char *s;
	int c, i=0, j;

/*
 * Keep trying until successful or end-of-file.
 */
	while(s=fgets(&string[i], len, file)) {
		j = strlen(s);
		len -= j;
		i += j;
/*
 * If not a complete line throw it away and go to the next line.
 */
		if(!strchr(s, '\n')) {
			while((c=getc(file)) != EOF && c != '\n') ;
			break;
		}
/*
 * If line ends in backslash-newline, get next line and concatenate.
 */
		if(i >= 2 && s[j-2] == '\\') {
			i -= 2;
			len += 2;
		} else
			break;
	}
	if(s)
		return s;
	else
		return NULL;
}

/* Parsing functions */
/* set auth auditmask */

#define REQ_LEN 32

extern char *syscallnames[];
extern char *trustedevent[];

/*
 * Function to convert a value subfield from an audit event token into
 * the appropriate value.
 */
static int event_value(string)
char *string;
{
	switch(*string) {
	case '0':
		return FAIL;
	case '1':
		return SUCCEED;
	default:
		return -1;
	}
}

/*
 * Function to add an audit event into the appropriate audit mask.
 */
static int change_audit(amask, event)
char *amask;
char *event;
{
	char flags, *value;
	int nsyscalls;
	int i;

/*
 * Process event and get value fields if present.
 */
	if(value=strchr(event, ':')) {
		*value++ = '\0';
		if((i=event_value(value)) >= 0) {
			flags = i << 1;
			if(value=strchr(value, ':')) {
				value++;
				if((i=event_value(value)) >= 0)
					flags |= i;
				else {
					fputs("Error, bad value for event fail flag, must be 0 or 1\n", stderr);
					return FAIL;
				}
			}
		} else {
			fputs("Error, bad value for event success flag, must be 0 or 1\n", stderr);
			return FAIL;
		}
	} else
/*
 * No value fields present, default to both success and fail.
 */
		flags = 0x3;

/*
 * get # syscalls by reading kernel.
 */
	nsyscalls = getkval (X_NSYSENT);
	if (nsyscalls == -1) {
		perror ("failed");
		exit(1);
	}

/*
 * set syscall event.
 */
	for (i = 0; i < nsyscalls; i++)
		if (strcmp (event, syscallnames[i]) == 0) {
			A_PROCMASK_SET (amask, i, (flags&0x2)>>1, flags&0x1);
			return SUCCEED;
		}

/*
 * kludge for shmsys operations.
 */
	for (i = 0; i < 4; i++)
		if (strcmp (event, special_event[i]) == 0) {
			A_PROCMASK_SET (amask, (N_SYSCALLS-4+i), (flags&0x2)>>1, flags&0x1);
			return SUCCEED;
		}

/*
 * set trusted event.
 */
	for (i = 0; i < N_TRUSTED_EVENTS; i++)
		if (strcmp (event, trustedevent[i]) == 0) {
			A_PROCMASK_SET (amask, i+MIN_TRUSTED_EVENT, (flags&0x2)>>1, flags&0x1);
			return SUCCEED;
		}

/*
 * invalid event.
 */
	if (i == N_TRUSTED_EVENTS) {
		fputs("Error, can't find audit event \"", stderr);
		fputs(event, stderr);
		fputs("\"\n", stderr);
	}
	return FAIL;
}

/*
 * Function to read and parse an ASCII duration value.  Duration may be
 * specified as time in seconds, minutes, hours, days, or a combination
 * of all of the preceeding.  The default for unlabelled time values is
 * seconds.  If the function returns non-zero if successful, zero otherwise.
 * The result is the time duration in seconds and is stored into the integer
 * variable pointed to by the argument.
 */
static int get_time(i)
int *i;
{
	char *s;
	int total=0, j;

	if(!getnum(&j))
		return FAIL;
	do {
		if(s=strtok(NULL, " \t\n")) {
			s = trim(s);
			switch(*s) {
/*
 * Days.
 */
			case 'd':
			case 'D':
				total += j * (60*60*24);
				break;
/*
 * Hours.
 */
			case 'h':
			case 'H':
				total += j * (60*60);
				break;
/*
 * Minutes.
 */
			case 'm':
			case 'M':
				total += j * 60;
				break;
/*
 * Seconds.
 */
			case 's':
			case 'S':
				total += j;
				break;
/*
 * Unrecognized value label.
 */
			default:
				return FAIL;
			}
		} else {
/*
 * Default to seconds.
 */
			total += j;
			break;
		}
	} while(getnum(&j));
	*i = total;
	return SUCCEED;
}

/*
 * Read in an ASCII representation of the auth record for a user, parsing it
 * into a binary representation.  The open file is the first argument, and the
 * binary output record is pointed to by the second argument.
 */
parse(file, auth)
FILE *file;
AUTHORIZATION *auth;
{
	char line[4097];
	char *s, pw_flag, pwmin_flag, pwmax_flag, pwmod_flag, am_flag;
	char aid_flag, ac_flag, uid_flag, fc_flag;
	int i;

/*
 * Initialize flags used to catch missing mandatory fields.
 */
	pw_flag = pwmin_flag = pwmax_flag = pwmod_flag = am_flag = 0;
	aid_flag = ac_flag = uid_flag = fc_flag = 0;
/*
 * Parse the file.
 */
	while(fgetline(line, sizeof line, file)) {
/*
 * Reject comment lines
 */
	s = line + strspn(line, " \t\n");
	if(!*s || *s == '#')
		continue;
/*
 * Get first token
 */
	if(!(s=strtok(line, "="))) {
		fputs("Error, bad line in file\n", stderr);
		return FAIL;
	}
#ifdef	DEBUG
fputs("first token = ", stderr);
fputs(s, stderr);
putc('\n', stderr);
fflush(stderr);
#endif	DEBUG
/*
 * Process token value(s)
 */
	s = trim(s);
	if(!strcmp(s, "fail_count")) {
		fc_flag = 1;
		if(!getnum(&i)) {
			i = authsave.a_fail_count;
			fprintf(stderr, "Warning bad or missing fail_count value, setting to %d\n", i);
		}
		auth->a_fail_count = i;
	} else
	if(!strcmp(s, "audit_id")) {
		aid_flag = 1;
		if(getnum(&i) && i > 0)
			auth->a_audit_id = i;
		else {
			fputs("Error, bad or missing audit_id value\n", stderr);
			return FAIL;
		}
	} else
	if(!strcmp(s, "audit_syscalls") || !strcmp(s, "audit_tevents")) {
		while(s=strtok(NULL, ",\n")) {
			s = trim(s);
			if(*s)
				if(!change_audit(auth->a_audit_mask, s)) {
					fputs("Error in audit event \"", stderr);
					fputs(s, stderr);
					fputs("\"\n", stderr);
					return FAIL;
				}
		}
	} else
	if(!strcmp(s, "uid")) {
		uid_flag = 1;
		if(getnum(&i) && i <= UID_MAX && i >= -UID_MAX)
			auth->a_uid = i;
		else {
			fputs("Error, bad or missing uid value\n", stderr);
			return FAIL;
		}
	} else
	if(!strcmp(s, "password")) {
		pw_flag = 1;
		if(s=strtok(NULL, " \t\n"))
			s = trim(s);
		else
			s = "";
		bzero(auth->a_password, sizeof auth->a_password);
		strncpy(auth->a_password, s, sizeof auth->a_password);
	} else
	if(!strcmp(s, "passmod")) {
		int month, day, year, hour, minute, second;
		long jtime;

		hour = minute = second = 0;
		pwmod_flag = 1;
		if(s=strtok(NULL, "\n"))
			s = trim(s);
		if(s && !strcmp(s, "epoch"))
			jtime = 0;
		else if(s && sscanf(s, "%d/%d/%d - %d:%d:%d",
		    &month, &day, &year, &hour, &minute, &second) >= 3) {
			extern long julian_second();
			extern struct tm *localtime();
			struct timeval tv;
			struct timezone tz;

			jtime = julian_second(year, month, day, hour, minute, second);
			gettimeofday(&tv, &tz);
			jtime += tz.tz_minuteswest*60;
			if(localtime(&jtime)->tm_isdst)
				jtime -= 60*60;
		} else {
			jtime = authsave.a_pass_mod;
			fputs("Warning, bad or missing passmod value, setting to ", stderr);
			if(jtime)
				fputs(ctime(&jtime), stderr);
			else
				fputs("epoch\n", stderr);
		}
		auth->a_pass_mod = jtime;
	} else
	if(!strcmp(s, "passlifemin")) {
		pwmin_flag = 1;
		if(!get_time(&i)) {
			i = authsave.a_pw_minexp;
			fprintf(stderr, "Warning, bad or missing passlifemin value, setting to %d\n", i);
		}
		auth->a_pw_minexp = i;
	} else
	if(!strcmp(s, "passlifemax")) {
		pwmax_flag = 1;
		if(!get_time(&i)) {
			i = authsave.a_pw_maxexp;
			fprintf(stderr, "Warning, bad or missing passlifemax value, setting to %d\n", i);
		}
		auth->a_pw_maxexp = i;
	} else
	if(!strcmp(s, "audit_control")) {
		ac_flag = 1;
		s = strtok(NULL, " \t\n");
		s = trim(s);
		if(!strcmp(s, "or"))
			auth->a_audit_control = AUDIT_OR;
		else if(!strcmp(s, "and"))
			auth->a_audit_control = AUDIT_AND;
		else if(!strcmp(s, "off"))
			auth->a_audit_control = AUDIT_OFF;
		else if(!strcmp(s, "user"))
			auth->a_audit_control = AUDIT_USR;
		else {
			fputs("Error, invalid audit_control value\n", stderr);
			return FAIL;
		}
	} else
	if(!strcmp(s, "authmask")) {
		am_flag = 1;
		while(s=strtok(NULL, ",\n")) {
			s = trim(s);
			if(*s)
				if(!strcmp(s, "login"))
					auth->a_authmask |= A_LOGIN;
				else if(!strcmp(s, "change_password"))
					auth->a_authmask |= A_CHANGE_PASSWORD;
				else if(!strcmp(s, "enter_password"))
					auth->a_authmask |= A_ENTER_PASSWORD;
				else {
					fputs("Error, invalid authmask value \"", stderr);
					fputs(s, stderr);
					fputs("\"\n", stderr);
					return FAIL;
				}
		}
	} else {
		fputs("Warning, unrecognized keyword \"", stderr);
		fputs(s, stderr);
		fputs("\" in temp file\n", stderr);
	}
	}
/*
 * Analyze results.  Report missing or inconsistant values.
 */
	if(!*auth->a_password)
		fputs("Warning, no password\n", stderr);
	if(pwmax_flag && !auth->a_pw_maxexp)
		fputs("Warning, no password expiration\n", stderr);
	if(!pwmin_flag) {
		fputs("Warning, missing passlifemin entry, setting to zero\n", stderr);
		auth->a_pw_minexp = 0;
	}
	if(!pwmod_flag) {
		fputs("Warning, missing passmod entry, setting to zero\n", stderr);
		auth->a_pass_mod = 0;
	}
	if(!fc_flag) {
		fputs("Warning, missing fail_count entry, setting to zero\n", stderr);
		auth->a_fail_count = 0;
	}
	if(!uid_flag) {
		fputs("Error, missing uid entry\n", stderr);
		return FAIL;
	}
	if(!aid_flag) {
		fputs("Error, missing audit_id entry\n", stderr);
		return FAIL;
	}
	if(!ac_flag) {
		fputs("Error, missing audit_control entry\n", stderr);
		return FAIL;
	}
	if(!pwmax_flag) {
		fputs("Error, missing passlifemax entry\n", stderr);
		return FAIL;
	}
	if(!am_flag) {
		fputs("Error, missing authmask entry\n", stderr);
		return FAIL;
	}
	return SUCCEED;
}

/* link with /sys/`machine`/BINARY/{syscalls.o,trustedevents.o} */

/* get integer value from kernel */
static int getkval(var)
int var;
{
	static int vm_fd = -1;
	static int km_fd;
	int i = 0;

	if(vm_fd == -1) {
		if((vm_fd = open("/vmunix", 0)) == -1)
			return -1;
		if((km_fd = open("/dev/kmem", 0)) == -1)
			return -1;
		nlist("/vmunix", nlst);
		if(nlst[0].n_type == 0)
			return -1;
	}

	if(lseek(km_fd, nlst[var].n_value, 0) == -1)
		return -1;
	if(read(km_fd, &i, sizeof (int)) != sizeof (int))
		return -1;
	else
		return i;
}

/*
 * Function to output an audit event value.
 */
static void put_audit_value(value, file)
int value;
FILE *file;
{
	switch(value) {
	case 3:
		break;
	case 2:
		fputs(":1", file);
		break;
	case 1:
		fputs(":0:1", file);
		break;
	case 0:
	default:
		fputs(":0:0", file);
		break;
	}
}

/*
 * Function to output the audit information of the record.
 */
static void output_aud(file, acntl, amask)
FILE *file;
char acntl;
char *amask;
{
	extern char *syscallnames[];
	extern char *trustedevent[];
	int nsyscalls, audit_flag;
	int i, j, k;

/*
 * Output acntl flag value.
 */
	fputs("audit_control = ", file);
	if(acntl & AUDIT_OFF)
		fputs("off\n", file);
	else if(acntl & AUDIT_AND)
		fputs("and\n", file);
	else if(acntl & AUDIT_USR)
		fputs("user\n", file);
	else
		fputs("or\n", file);
/*
 * Get # syscalls.
 */
	nsyscalls = getkval(X_NSYSENT);
	if ( nsyscalls == -1) {
		perror ( "nsyscalls failed");
		return;
	}
/*
 * Display syscall mask
 */
	fputs("audit_syscalls = ", file);
	i = 0;
	for(j = 0; j < SYSCALL_MASK_LEN; j++)
		for(k = 0; k < 8; k+=2) {
			if((j<<2)+(k>>1) >= nsyscalls)
				break;
			if(amask[j] &(0x3 <<(6-k%8))) {
				if(i)
					putc(',', file);
				if(!((++i)%6))
					fputs("\\\n\t", file);
				fputs(syscallnames[(j<<2)+(k>>1)], file);
				audit_flag = 0;
				audit_flag = amask[j] &(0x2 <<(6-k%8)) ? 2 : 0;
				audit_flag |= amask[j] &(0x1 <<(6-k%8)) ? 1 : 0;
				put_audit_value(audit_flag, file);
			}
		}
/*
 * Kludge for shmsys operations
 */
	j =(N_SYSCALLS*2)/8 - 1;
	for(k = 0; k < 8; k+=2)
		if(amask[j] &(0x3 <<(6-k%8))) {
			if(i)
				putc(',', file);
			if(!((++i)%6))
				fputs("\\\n\t", file);
			fputs(special_event[k>>1], file);
			audit_flag = 0;
			audit_flag = amask[j] &(0x2 <<(6-k%8)) ? 2 : 0;
			audit_flag |= amask[j] &(0x1 <<(6-k%8)) ? 1 : 0;
			put_audit_value(audit_flag, file);
		}
	putc('\n', file);
/*
 * Display trusted event mask
 */
	fputs("audit_tevents = ", file);
	i = 0;
	for(j = SYSCALL_MASK_LEN; j < SYSCALL_MASK_LEN+TRUSTED_MASK_LEN; j++)
		for(k = 0; k < 8; k+=2) {
			if(amask[j] &(0x3 <<(6-k%8))) {
				if(i)
					putc(',', file);
				if(!((++i)%8))
					fputs("\\\n\t", file);
				fputs(trustedevent[((j-SYSCALL_MASK_LEN)<<2)+(k>>1)], file);
				audit_flag = 0;
				audit_flag = amask[j] &(0x2 <<(6-k%8)) ? 2 : 0;
				audit_flag |= amask[j] &(0x1 <<(6-k%8)) ? 1 : 0;
				put_audit_value(audit_flag, file);
			}
		}
	putc('\n', file);
}

/*
 * Function to display an integer value followed by a unit label.
 */
static void put_unit(i, s, file)
int i;
char *s;
FILE *file;
{
	fprintf(file, " %d %s", i, s);
	if(i > 1)
		putc('s', file);
}

/*
 * Function to output a time duration value broken down into units.
 */
static void put_time(i, file)
int i;
FILE *file;
{
	int j;

/*
 * If less than a minute output unlabelled value (seconds).
 */
	if(i < 60) {
		fprintf(file, " %d\n", i);
		return;
	}
/*
 * If greater than a day output integral days.
 */
	if(j=i/(60*60*24)) {
		put_unit(j, "day", file);
		i %= (60*60*24);
	}
/*
 * If remainder is greater than an hour, output integral hours.
 */
	if(j=i/(60*60)) {
		put_unit(j, "hour", file);
		i %= (60*60);
	}
/*
 * If remainder is greater than a minute, output integral minutes.
 */
	if(j=i/60) {
		put_unit(j, "minute", file);
		i %= 60;
	}
/*
 * Output remaining seconds, if any.
 */
	if(i) {
		put_unit(i, "second", file);
	}
	putc('\n', file);
}

/*
 * Function to convert a binary auth record into a human readable but still
 * machine parsable ASCII representation.
 */
unparse(file, auth)
FILE *file;
AUTHORIZATION *auth;
{
	struct tm *tm;
	int i, j;

	fprintf(file, "uid = %d\n",  auth->a_uid);
	fputs("password = ", file);
	fputs(auth->a_password, file);
	putc('\n', file);
	fputs("passlifemin =", file);
	put_time(auth->a_pw_minexp, file);
	fputs("passlifemax =", file);
	put_time(auth->a_pw_maxexp, file);
	if(auth->a_pass_mod > 0) {
		tm = localtime(&auth->a_pass_mod);
		fprintf(file, "passmod = %02d/%02d/%02d - %02d:%02d:%02d\n",
		    tm->tm_mon+1, tm->tm_mday, tm->tm_year%100,
		    tm->tm_hour, tm->tm_min, tm->tm_sec);
	} else
		fputs("passmod = epoch\n", file);
	fputs("authmask = ", file);
	j = 0;
	for(i=0; i < (sizeof auth->a_authmask)*8; i++) {
		if(auth->a_authmask & (1<<i)) {
			if(j++)
				fputc(',',file);
			switch(i) {
			case 0:
				fputs("login", file);
				break;
			case 1:
				fputs("change_password", file);
				break;
			case 2:
				fputs("enter_password", file);
				break;
			default: ;
			}
		}
	}
	putc('\n', file);
	fprintf(file, "fail_count = %d\n", auth->a_fail_count);
	fprintf(file, "audit_id = %d\n", auth->a_audit_id);
	output_aud(file, auth->a_audit_control, auth->a_audit_mask);
#ifdef	PRIV
	fputs("privileges = ?\n", file);
#endif	PRIV
}

/*
 * Function to unparse a binary auth record and put it into a new temporary
 * file.  The file name is returned as the value of the function.  If the
 * function fails it returns the NULL pointer.
 */
static char *putinfile(auth)
AUTHORIZATION *auth;
{
	static char *filename;
	extern char *mktemp();
	FILE *file;

	filename = mktemp("/tmp/edauth.XXXXXX");
	if(!(file=fopen(filename, "w")))
		return (char *) NULL;
	unparse(file, auth);
	if(fclose(file) == EOF)
		return (char *) NULL;
	else
		return filename;
}

/*
 * Function to read the named file and parse the data into a binary auth
 * record.  If the function is successful it returns a pointer to a static
 * area containing the auth record.  Otherwise it returns the NULL pointer.
 */
static AUTHORIZATION *getfromfile(filename)
char *filename;
{
	char line[1024];
	FILE *file;
	static AUTHORIZATION auth;

	if(!(file=fopen(filename, "r"))) {
		fputs("Unable to reopen tmp file.\n", stderr);
		exit(1);
	}
	if(parse(file, &auth)) {
		fclose(file);
		return &auth;
	} else {
		fclose(file);
		return (AUTHORIZATION *) NULL;
	}
}

/*
 * Function to edit an auth record.
 */
AUTHORIZATION *editit(auth)
AUTHORIZATION *auth;
{
	static char def_editor[] = "/bin/ed";
	union wait status;
	int pid;
	char *editor, *filename, *getenv();
	char line[BUFSIZ];

/*
 * Put ASCII representation of record into a new temporary file.
 */
	filename = putinfile(auth);
/*
 * Edit the file with the editor of choice until successful.
 */
	while(1) {
	fflush(stdout);
	fflush(stderr);
	signal(SIGINT, SIG_IGN);
	pid = fork();
	if(pid < 0) {
		perror(progname);
		exit(1);
	}
	if(pid) {
		wait(&status);
	} else {
		signal(SIGINT, SIG_DFL);
		if(!(editor=getenv("EDITOR")))
			editor = def_editor;
		execlp(editor, editor, filename, 0);
		exit(1);
	}
	signal(SIGINT, SIG_DFL);
/*
 * Get the record from the file and convert to binary.
 */
	auth = getfromfile(filename);
/*
 * If problems give the user another shot at fixing it up.
 */
	if(!auth) {
		fputs("Error in file, do you wish to resume editing?> ", stdout);
		if(fgetline(line, sizeof line, stdin))
			if(*line == 'y' || *line == 'Y')
				continue;
		break;
	} else
		break;
	}
/*
 * Dispose of temporary file.
 */
	unlink(filename);
	if(!auth || (status.w_status&0xffff))
		return (AUTHORIZATION *) NULL;
	else
		return auth;
}

/*
 * Print an error message.
 */
static void audevent(statustr, status)
char *statustr;
int status;
{
	
	char tmask[AUD_NPARAM];
	struct {
		char *a;
		int  b;
	} aud_arg;
	int i;

/*
 * Build token mask.
 */
	tmask[0] = T_CHARP;
	if(status == 0)
		tmask[1] = T_RESULT;
	else
		tmask[1] = T_ERROR;
	tmask[2] = '\0';


	aud_arg.a = statustr;
	aud_arg.b = status;
/*
 * Generate audit record.
 */
        if(audgen(AUTH_EVENT, tmask, &aud_arg) == -1 )
		perror("audgen" );
}

/*
 * Error termination function.
 */
static void getout(statustr, status)
int status;
char *statustr;
{
	if(statustr) {
		audevent(statustr, status);
		fputs(statustr, stderr);
		putc('\n', stderr);
	}
	exit(status);
}

static char usage[] = "usage: edauth [-p protouser] username";

main(argc, argv)
/*
 * The main event.
 */
int argc;
char *argv[];
{
	AUTHORIZATION authorization, *auth;
	int uid, protouid, pflag=0, c;
	char user[17], protouser[17], buf[AUD_BUF_LEN];
	extern int optind;
	extern char *optarg;

/* turn off auditing of all events except for LOGIN and failed setgroups */

	if ( audcntl (SET_PROC_ACNTL, (char *)0, 0, AUDIT_AND, 0) == -1)
		perror ( "audcntl" );
	A_PROCMASK_SET ( buf, AUTH_EVENT, 1, 1 );
	if (audcntl(SET_PROC_AMASK, buf, AUD_BUF_LEN, 0, 0) == -1 )
		perror ( "audcntl" );

	if(argc > 0)
		progname = argv[0];
	while((c=getopt(argc, argv, "p:")) != EOF)
		switch(c) {
		case 'p':
			pflag = 1;
			strncpy(protouser, optarg, sizeof protouser);
			break;
		default:
			getout(usage, 1);
		}
	if(optind == (argc-1))
		strncpy(user, argv[optind], sizeof user);
	else {
		getout(usage, 1);
	}
	umask(077);
	if(pflag)
		if((protouid=getuser(protouser)) == -1)
			getout("Protouser not found in passwd data base.", 1);
	if((uid=getuser(user)) == -1) {
		getout("User not found in passwd data base.", 1);
	}
	if(open_auth(AUTHORIZATION_DB, O_RDWR)) {
		getout("Unable to open auth data base.", 1);
	}
/*
 * Get auth record.
 */
	if(pflag) {
		if(get_auth(protouid, &authorization) < 0)
			getout("Protouser not found in auth data base.", 1);
	} else
		if(get_auth(uid, &authorization) < 0)
			getout("User not found in auth data base.", 1);
/*
 * Save the record from the static area used in getauthent().
 */
	auth = &authorization;
	bcopy(auth, &authsave, sizeof (AUTHORIZATION));
/*
 * If creating from protouser, set the new UID and clear the audit ID.
 */
	if(pflag) {
		authorization.a_uid = uid;
		authorization.a_audit_id = 0;
		authorization.a_fail_count = 0;
	}
/*
 * Don't leave the auth files open when giving the user their editor.
 */
	close_auth();
	if(!(auth=editit(auth))) {
		getout("Edit failed.", 1);
	}
/*
 * If the record didn't change at all don't do anything.
 */
	if(!bcmp(auth, &authsave, sizeof (AUTHORIZATION))) {
		getout("Record unchanged.", 0);
	}
	if(open_auth(AUTHORIZATION_DB, O_RDWR)) {
		getout("Unable to reopen auth data base.", 1);
	}
/*
 * If the UID field of the record changed we have to take special action.
 * This is because the UID is the key into the data base.  If we just
 * stored out the new record the old one would also still be around.  So we
 * must delete the record stored undewr the old UID first.
 */
	if(auth->a_uid != authsave.a_uid) {
		char buf[100];

		sprintf(buf, "Warning, changing UID from %d to %d",
		    authsave.a_uid, auth->a_uid);
		fputs(buf, stderr);
		putc('\n', stderr);
		if(delete_auth(authsave.a_uid) < 0) {
			getout("Unable to remove old entry.", 1);
		}
		audevent(buf, 0);
	}
/*
 * Store out new record.
 */
	bcopy(auth, &authsave, sizeof (AUTHORIZATION));
	auth = &authsave;
	if(storeauthent(auth) < 0) {
		close_auth();
		getout("Failed to store new entry.", 1);
	} else {
		char buf[100];

		close_auth();
		sprintf(buf, "Updated auth entry for uid %d.", auth->a_uid);
		getout(buf, 0);
	}
}
