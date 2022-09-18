/************************************************************/
/*                                                          */
/*   Definitions file for capsar utilities and libraries    */
/*                                                          */
/************************************************************/


#define NOTOK		(-1)		/* syscalls return this on error */
#define	OK		(0)		/* returned on success */
#define	DONE		(1)		/* job done */
#define	NULLCP		((char *)0)
#define NULLVP		((char **)0)
#define EOM		(-1)		/* emd of message */
#define EBFOUND		(0)		/* encapsaulation found */
#define EBSEP		(2)		/* separator found */
#define EBDASH		(3)		/* encapsulation boundery */
#define	ANYTEXT		(4)		/* any text */
#define DOT		(5)		/* dot character found */
#define HEADER		(6)		/* mail header line flag */
#define EOA		(7)		/* separator line */
#define CONT_POSS	(8)		/*header continuation line */
#define EBSMTP		(9)		/* separation line for SMTP */

#define NOTSMTP		(0)
#define SMTPLF		(1)
#define SMTPCRLF	(2)

#define TEXT		(0)
#define CDA		(1)

#define	TIDE_MARK_RATIO 0.0001		/* hide tide mark for memory 
					   allocation */
#define NO_LOGGING	(0)
#define STDERR_LOGGING  (1)
#define SYSLOG_LOGGING  (2)


/* mail message types */

#define	EMPTY		0		/* the message is empty */
#define	SIMPLE_MESSAGE	1		/* message has exactly 1 body part */
#define	COMPOUND_MESSAGE 2		/* two or more body parts */
#define MAIL_MESSAGE	3		/* content header + 1+ body part */
#define	MULTIPLE_FORMAT	4		/* 2+ representations of
					   same document eg ddif */

/* default settings */

#define ENABLE		1		/* if non zero conversion of
					   bodyparts is enabled */

#define	DEF_SEP 	"\n"
#define SMTP_SEP 	"\r\n"

/* internal representation of a mail message */

#define MM	struct mm
struct mm {
	char	*name;		/* name tag for message */
	char	*start;		/* string tag for message */
	char	*stop;		/* stop string tag */
	char	*body_type;	/* body type */
	int	complete;	/* completed string */
	int 	enable;		/* conversion enable */
	int	size;		/* size of message in bytes */
	int	message_type;	/* mail message type */
	char	*separator;	/* line separator for message */
	char	*dataptr;	/* pointer to message */
	char	*swapfile;	/* pointer to file on disk */
	int	fd;		/* file descriptor to file on disk */
	int	flat_fd;	/* file descriptor to flat mail file */
	int	offset;		/* offset in bytes from start of file */
	struct mm *mm_next;	/* link list of mail structures */
	struct mm *parent;	/* parent mail message */
};

#define COMPRESS_CMD 	"/usr/ucb/compress"
#define UUENCODE_CMD 	"/usr/bin/uuencode"
#define UNCOMPRESS_CMD 	"/usr/ucb/uncompress"
#define UUDECODE_CMD 	"/usr/bin/mhdecode"
#define CTOD 		"/usr/bin/ctod"
#define CAT 		"/bin/cat"

#define	BODY_TYPE_DEF 	"text"
#define DDIFTAG 	"DDIF"
#define DOTSTAG 	"DOTS"
#define DDIF_MAGICNO 	020037777777
#define DOTS_MAGICNO 	020037177777

/* useful functions used */
void	error ();
char	*getcpy ();

/* capsar routines */
MM	*capsar_new();
MM	*capsar_create();
MM	*capsar_parse_file();
int	capsar_unparse_file();
int	capsar_message_type();
int	capsar_set_name();
char	*capsar_get_name();
char	*capsar_get_body_type();
MM	**capsar_get_messages();
int	capsar_append_header();
int	capsar_delete_header();
char	**capsar_get_header_list();
char	*capsar_get_header();
int	capsar_append_message();
int	capsar_delete_message();
MM	*capsar_new_parse();
int	capsar_quick();

