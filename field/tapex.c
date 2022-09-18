
#ifndef lint
static char *sccsid = "@(#)tapex.c	4.2	ULTRIX	3/7/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989, 90 by			*
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
 * Compilation Instructions: cc -o tapex tapex.c -lerrlog
 *
 ************************************************************************/
/*
 * tapex - tape testing utility
 *
 * By: Tim Burke
 * Creation Date: Oct 28, 1988
 *
 * This program is used to test tape device functionality.
 * Functionality tested includes:
 *
 * - Basic writing and reading of tape records of various sizes
 * - Writing/Reading past the end of media
 * - Positioning tests.  Spacing forward/backward records/files and
 *   verifying data integrity.
 * - n-buffered I/O testing.
 * - tape content analysis.
 * - media loader testing.
 * - command timeout testing.
 *
 * For a more complete description of the usage of this program see the 
 * help messages which are displayed to the "-h" option.
 *
 *-----------------------------------------------------------------------
 *
 * Modification History
 *
 *	4/3/89	Tim Burke
 *	Added error recovery to the record and file movement tests to allow
 *	more graceful operation in the event of an error.
 *
 *	4/12/89 Tim Burke
 *	Modify eof, file and record positioning tests to use n-buffered I/O.
 *
 *	4/13/89 Tim Burke
 *	Add performance test to analyze "ideal" drive bandwidth.
 *
 *	4/21/89 - Tim Burke
 *	Add cycle_tape() which opens and closes the unit between tests to 
 *	clear MTCSE in the driver.  Use this routine between tests.
 *
 *	5/1/89  - Tim Burke
 *	Cosmetic cleanups which include:
 *		- Changed "-d" and "-D" to "-v" and "-V".
 *		- Arange options to help message in alphabetical order.
 *		- Log test start and stop times to error logger.
 *		- Added transportability test.
 *
 *	5/4/89  - Tim Burke
 *	Added EOF test.
 *	Added tape disection test - prints tape format.
 *
 *	5/15/89 - Tim Burke
 *	Added tests for MTFSR and MTBSR into the file position test to 
 *	verify that record movement fails when it hits a tape mark.
 *
 *	5/18/89 - Tim Burke
 *	Added cache flushing when the cache option has been specified.
 *
 *	6/1/89 - Tim Burke
 *	Added tar "r" test.  This tests appending to a media.
 *	Changed -t (tty output) to -T to allow -t to be used to represent
 *	test time in minutes to be compatible with other exercisers.
 *	For TMSCP tape devices use caching as the default mode of operation.
 *
 *	6/21/89 - Tim Burke
 *	Add "-w" to open tape read-only.  Some other cosmetic changes with
 *	debug output.  Added "-G" to do the file tests in read-only mode once
 *	the tape has already been written.
 *
 *	6/22/89 - Tim Burke
 *	Added media loader test.
 *	Increased maximum record size on mips scsi tapes to be the same as
 *	tmscp.  Now only vax scsi is limited to a maximum record size of 16K.
 *
 *	7/31/89 - Tim Burke
 *	Skip the write past end of media test when a write-once media device
 *	is being tested to prevent complete consumption of media capacity.
 *	Fixed how loaders are identified to properly handle the TQK7L.
 *
 *	9/13/89 - Tim Burke
 *	Added 3 new TA90 densities.  In the eom test print out the
 *	observed media capacity.  Added checks for repeated n-buf failures
 *	in nbuf_read and write.  Abort tests if n-buf errors persist.
 *
 *	11/15/89 - Tim Burke
 *	Call lseek(0) to reset the file offset when doing reverse positioning
 *	operations.  This prevents the file offset from wrapping to a negative
 *	value which would cause reads and writes to fail with errno of EINVAL.
 *	Added DEV_TF70L to the list of loaders supported in the media loader
 *	test.
 *
 *	12/19/89 - Tim Burke
 *	Added command timeout test, this is the "-q" option.
 *	Impose a maximum error threashold in the write past end of media
 *	test to get rid of a potential infinite loop.
 *
 *	2/9/91 - Brian Nadeau
 *	Add TA91 to the loader table and update calls to malloc.
 *
 *--------------------------------------------------------------------------
 *
 * TODO:
 *
 *	Save test summary to print out when all tests have completed.
 *
 *	Allow user to specify record sizes greater than MAX_RECORD_SIZE.
 *
 *	Allow most of the tests to run under time constraint.  Presently only
 *	the write/read record test looks at the -t time parameter.
 *
 */

/****************************************************************************
 * Include files.
 ****************************************************************************/
#include <stdio.h>
#include <sys/file.h>
#include <sys/mtio.h>
#include <sys/devio.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include <ctype.h>

/****************************************************************************
 * Defines
 ****************************************************************************/
#define DEFAULT_TAPE "/dev/rmt0h"
#define CHAR_MASK 0x7f		/* Aviod RISC sign extention probs with 0xff */

/*
 * Constants related to record sizes.  Used mainly in the read/write
 * record tests.
 */
#define MIN_RECORD_SIZE 1
#define MAX_RECORD_SIZE (63*1024) /* 63K, should be 64K-1 actually */
#define NUM_RECORDS 10000
#define DEF_MAX_RECORD 15360  /* 15K */
#define DEF_MIN_RECORD 512  
#define DEF_INC_RECORD 512  
#define DEF_RECORDS 200

#define MIN_RECORDS 1
#define MIN_INCREMENT 1
#define MAX_INCREMENT (MAX_RECORD_SIZE - MIN_INCREMENT)
#define MAX_RECORDS 100000

#define BUFFER_INIT 0			/* Init read buf elements to zero. */
#define ALT_BUF_INIT 0130		/* The letter "X" */

/*
 * Constants used when calling the read_record routine.
 */
#define VALIDATE 1
#define NO_VALIDATE 0

/*
 * Constants used in the write/read past end of tape tests.
 */
#define DEF_EOF_RECORD_SIZE 10240
#define PAST_EOF_RECORD 512
#define DEF_EOF_NUMREC 20
#define EOT_MIN_RECORDS 1
#define EOT_MAX_RECORDS 1000
#define MAX_EOM_ERR 29

/*
 * Constants used in the n-buffered I/O tests.
 */
#define MIN_NBUFFERS 1	
#define MAX_NBUFFERS 50
#define DEF_NBUFFERS 7
#define WRITE_IP     1
#define READ_IP      1
#define WRITING	     1
#define READING	     2
#define SLOW	     0
#define NO_INIT	     0
#define DO_INIT	     1
#define MAX_NBUF_ABORTS 20

/*
 * Constants used in the positioning tests and transportability test.
 */
#define DEF_POS_REC_SIZE 888
#define DEF_POS_NUMREC 700
#define FORWARD 0
#define BACKWARD 1
#define TEST_BOT 0
#define NO_READ 0
#define READ 1
#define NUM_RANDOM_TESTS 100
#define MIN_RANDOM 1
#define MAX_RANDOM 10000
#define MIN_FILES 1
#define MAX_FILES 10000
#define DEF_NUM_FILES 33
#define NUM_REC_FILE 100
#define MIN_REC_FILE 1
#define MAX_REC_FILE 10000
#define REC_SIZE_INC 10

/*
 * Timing constants
 */
#define MIN_TIME 0
#define MAX_TIME 10000

/*
 * Record size tests
 */
#define DEF_SIZE_RECORD 1000
#define OVER_READ 10
#define UNDER_READ 10

/*
 * Constants used in random record write/read tests.
 */
#define DEF_REC_RANDOM 1000		/* Default -Write this many records */
#define MIN_RAND_SIZE 1			/* Minimum number of tests to run */
#define DEF_MAX_RANDOM 5000		/* Maximum number of random tests */
#define DEF_RAND_MAX (1024*50)		/* 50k */
#define DEF_RAND_MIN 50
#define RAND_READ_SIZE (1024*40)	/* 40k */
/*
 * The VAX scsi driver imposes a maximum record size of 16K.  On the mips
 * side the program defined MAX_RECORD_SIZE limits the maximum transfer 
 * size.
 */
#ifdef vax
#define SCSI_MAX_XFER (1024*16)		
#else vax
#define SCSI_MAX_XFER (MAX_RECORD_SIZE)
#endif vax

/*
 * Constants used in the performance test.
 */
#define DEF_NUM_PERF_REC 600		/* Write this many records */
					/* Note: only 625 64K records fit on a
					 * 1600 BPI tape. */

/*
 * Constants used in the eof test.
 */
#define DEF_EOF_FILES  5		/* 5 files */
#define DEF_EOF_NUMRECORDS 10		/* 10 records in each file */
#define DEF_EOF_RECSIZE 1024		/* Each record is 1024 bytes.
					 * If the record size is made
					 * changable then the read buffer in
					 * the test must be made max.
					 */

/*
 * Constants used to be backward compatible with the old version of mtx.
 * These are use in the mtx front end routine.
 */
#define DEVOFFS 5			/* offset to device name */
#define MTX_DEF_SHORT  512		/* default short record size */
#define MTX_DEF_LONG   10240		/* default long record size */

/*
 * Constants used in the tar "r" media append test.
 */
#define TAR_BASE_RECS	20		/* initial writes */
#define TAR_ADD_RECS	20		/* append records to the media */
#define TAR_REC_SIZE	10240		/* tar default record size, 10K */

/*
 * Constants used in command timeout test "q".
 */
#define TIMEOUT_RS	10240		/* 10K record size		*/
#define TIMEOUT_NR	100		/* This many  records per file  */

/*
 * Actions related to the tape_file deviocget.
 */
#define DEVIO_NAME	1		/* Display device name */
#define DEVIO_STATUS	2		/* Display device status */
/*
 * Expected results on the record info tests.
 */
#define SUCCEEDS 1
#define FAILS 0

/*
 * Output status for printf's
 */
#define LOGFILE		0x1	/* Send output to a logfile */
#define TTYPRINT	0x2	/* Send output to standard out */
#define MAX_OUTPUT	10	/* Max error messages per test */
#define MAX_ERR_LINES	10000	/* Max error messages allowed */
#define MIN_ERR_LINES	1	/* Min error messages allowed */
#define OUT_STRING	"\nMaximum error output exceeded, terminate printing on this subtest.\n"

/*
 * Types of messages to log to the error logger.
 */
#define LOG_START	1	/* Start tests */
#define LOG_STOP	2	/* Stop tests */
#define LOG_ABORT	3	/* Abort tests */
#define LOG_TIMEUP	4	/* Time limit expired */
#define LOG_OFFLINE	5	/* Tape went offline */
#define LOG_REWFAIL	6	/* Rewind failed */
#define LOG_COMMAND	7	/* Something wrong with command line params */
#define LOG_NBUF	8	/* Repeated n-buf errors */
#define LOG_CACHE	9	/* Cache flush failure */
#define START_READTRANS 10	/* These tell the error logger to indicate    */
#define START_RANDREC   11	/* that the specified test is about to begin. */
#define START_FILEPOS   12
#define START_RECPOS    13
#define START_RECMOVE   14
#define START_EOM 	15
#define START_SIZE 	16
#define START_PERF 	17
#define START_EOF 	18
#define START_LOADER    19
#define START_TAR 	20
#define START_RDWR 	21
#define START_WRITETRAN 22
#define START_DISECT    23
#define LOG_NOTICE      24	/* This test may cause driver error logs */
#define START_TIMEOUT   25

/*
 * Loader attributes.  This describes the capabilities of a loader.  These
 * bits are used in the attributes field of the loader_struct structure.
 */
#define SEQUENTIAL_STACK	0x1	/* Sequential stack loader */
#define RANDOM_ACCESS		0x2	/* Random access loader */
/*
 * Commands that the loader_command routine can perform.
 */
#define NEXT_MEDIA		0x1	/* Load the next sequential media */
/*
 * Default parameters for the loader write test.
 */
#define DEF_LOAD_RS		10240		/* 10K record size */
#define DEF_LOAD_RECORD		10		/* 10 records */
/*
 * Status values returned by the load_command routine.
 */
#define LOAD_SUCCESS	0x0			/* Successful writable media */
#define LOAD_READONLY	0x1			/* Read-only media loaded */
#define LOAD_TIMEOUT	0x2			/* Load timed out */
#define LOAD_OPENFAIL	0x3			/* Open failure */

/****************************************************************************
 * Macros
 ****************************************************************************/
/*
 * Macros to facilitate logging to an output file.
 *
 * Print - printf with no args
 * Print1 - printf with 1 arg
 * Print2 - printf with 2 args
 * Print3 - printf with 3 args
 *
 * Error printfs; these are used with error messages to limit runaway output.
 * EPrint - printf with no args
 * EPrint1 - printf with 1 arg
 * EPrint2 - printf with 2 args
 * EPrint3 - printf with 3 args
 *
 * Log_print - used to echo interactive parameters to logfile.
 */
#define Log_print( string )						\
	{								\
			if (printstatus & LOGFILE) {			\
				fprintf(fd_logfile, string);		\
				fprintf(fd_logfile, "\n");		\
			}						\
	}
			
#define Print( string ) 						\
	{								\
			if (printstatus & LOGFILE) {			\
				fprintf(fd_logfile, string);		\
			}						\
			if (printstatus & TTYPRINT) {			\
				fprintf(stdout, string);		\
			}						\
	}
			
#define Print1( string, arg1 ) 						\
	{								\
			if (printstatus & LOGFILE) {			\
				fprintf(fd_logfile, string, arg1);	\
			}						\
			if (printstatus & TTYPRINT) {			\
				fprintf(stdout, string, arg1);		\
			}						\
	}
#define Print2( string, arg1, arg2 )					\
	{								\
			if (printstatus & LOGFILE) {			\
				fprintf(fd_logfile, string, arg1, arg2);\
			}						\
			if (printstatus & TTYPRINT) {			\
				fprintf(stdout, string, arg1, arg2);	\
			}						\
	}
#define Print3( string, arg1, arg2, arg3 )				\
	{								\
			if (printstatus & LOGFILE) {			\
				fprintf(fd_logfile, string, arg1, arg2, arg3);\
			}						\
			if (printstatus & TTYPRINT) {			\
				fprintf(stdout, string, arg1, arg2, arg3);\
			}						\
	}
#define EPrint( string ) 						\
	{								\
		if (lines_output <= def_err_lines) {			\
			if (lines_output == def_err_lines) {		\
				if (printstatus & LOGFILE) {            \
					fprintf(fd_logfile, OUT_STRING);\
				}					\
				if (printstatus & LOGFILE) {            \
					fprintf(stdout, OUT_STRING); 	\
				}					\
				flush_output();				\
			}						\
			lines_output++;					\
			if (printstatus & LOGFILE) {			\
				fprintf(fd_logfile, string);		\
			}						\
			if (printstatus & TTYPRINT) {			\
				fprintf(stdout, string);		\
			}						\
		}							\
	}
			
#define EPrint1( string, arg1 ) 						\
	{								\
		if (lines_output <= def_err_lines) {			\
			if (lines_output == def_err_lines) {		\
				if (printstatus & LOGFILE) {            \
					fprintf(fd_logfile, OUT_STRING);\
				}					\
				if (printstatus & LOGFILE) {            \
					fprintf(stdout, OUT_STRING); 	\
				}					\
				flush_output();				\
			}						\
			lines_output++;					\
			if (printstatus & LOGFILE) {			\
				fprintf(fd_logfile, string, arg1);	\
			}						\
			if (printstatus & TTYPRINT) {			\
				fprintf(stdout, string, arg1);		\
			}						\
		}							\
	}
#define EPrint2( string, arg1, arg2 )					\
	{								\
		if (lines_output <= def_err_lines) {			\
			if (lines_output == def_err_lines) {		\
				if (printstatus & LOGFILE) {            \
					fprintf(fd_logfile, OUT_STRING);\
				}					\
				if (printstatus & LOGFILE) {            \
					fprintf(stdout, OUT_STRING); 	\
				}					\
				flush_output();				\
			}						\
			lines_output++;					\
			if (printstatus & LOGFILE) {			\
				fprintf(fd_logfile, string, arg1, arg2);\
			}						\
			if (printstatus & TTYPRINT) {			\
				fprintf(stdout, string, arg1, arg2);	\
			}						\
		}							\
	}
#define EPrint3( string, arg1, arg2, arg3 )				\
	{								\
		if (lines_output <= def_err_lines) {			\
			if (lines_output == def_err_lines) {		\
				if (printstatus & LOGFILE) {            \
					fprintf(fd_logfile, OUT_STRING);\
				}					\
				if (printstatus & LOGFILE) {            \
					fprintf(stdout, OUT_STRING); 	\
				}					\
				flush_output();				\
			}						\
			lines_output++;					\
			if (printstatus & LOGFILE) {			\
				fprintf(fd_logfile, string, arg1, arg2, arg3);\
			}						\
			if (printstatus & TTYPRINT) {			\
				fprintf(stdout, string, arg1, arg2, arg3);\
			}						\
		}							\
	}
			
/****************************************************************************
 * Typedefs
 ****************************************************************************/
			
/*
 * Structure to hold error status.
 */
typedef struct {
	int buf_sz;
	int read_errors;
	int write_errors;
	int validate_errors;
} error_st;

/*
 * reads and writes are done in aligned buffers of this form.
 */
typedef union {
	/*
	 * Allign on a machine word boundary per release note.
 	 */
	int allignment;
	unsigned char buffer[MAX_RECORD_SIZE];
} rw_struct;

/*
 * Structure used in n-buffered I/O.
 */
typedef struct {
	char *buffer;
} nbuf_struct;

/*
 * Structure used for loader tests.  This lists supported loaders and
 * associated attributes.
 */
typedef struct {
	char *loader_name;		/* Dev name as in TA90, etc. */
	int  num_media;			/* How many tapes the unit serves */
	unsigned int attributes;	/* Functions the loader supports */
	int  max_load_min;		/* Maximum load time in minutes */
} loader_struct;

/****************************************************************************
 * Program Routines
 ****************************************************************************/
int tape_file();
int get_test_params();
int get_number();
int do_tape_op();
int sig_handler();
int eom_test();
int format_writebuf();
int tape_eot();
int nbuf_setup();
int position_record_test();
int position_file_test();
int move_record();
int Rmove_record();
int recover_position();
int help_message();
int write_record();
int read_record();
int convert_value();
int parse();
int pos_file_test();
int write_file();
int nbuf_write();
int nbuf_read();
int nbuf_disable();
int bounds_check();
int size_testing();
int rand_record();
int mtx_front();
int scsi_tape();
int tmscp_tape();
int worm_tape();
int performance_test();
int cycle_tape();
int print_time();
void do_errlog();
void eof_test();
void disect_tape();
void hop_tm();
int cache_flush();
void tar_r_test();
void cache_setup();
void loader_test();
int load_command();
int unload_media();
int load_write_test();
int timeout_test();

/****************************************************************************
 * Global Variables
 ****************************************************************************/
error_st *err_ptr;		/* String to pass to error logger */
int debug = 0;			/* Program output level */
char device[14] = "/dev/";	/* Device name to test */
/*
 * Assign default values for program parameters.  Cound be changed
 * interactively with the "-i" switch or specified directly in the
 * command line.
 */
unsigned int def_max_record = DEF_MAX_RECORD;
unsigned int def_min_record = DEF_MIN_RECORD;
unsigned int def_num_record = DEF_RECORDS;
unsigned int def_inc_record = DEF_INC_RECORD;
unsigned int def_eof_record = DEF_EOF_RECORD_SIZE;
unsigned int def_eof_rec_sz = PAST_EOF_RECORD;
unsigned int def_eof_numrec = DEF_EOF_NUMREC;
unsigned int def_nbuf_buffers = DEF_NBUFFERS;
unsigned int def_pos_record = DEF_POS_NUMREC;
unsigned int pos_rec_size = DEF_POS_REC_SIZE;
unsigned int def_num_random = NUM_RANDOM_TESTS;
unsigned int def_rec_file = NUM_REC_FILE;
unsigned int def_rec_inc = REC_SIZE_INC;
unsigned int def_num_files = DEF_NUM_FILES;
unsigned int def_time_delta = MIN_TIME;
unsigned int def_size_record = DEF_SIZE_RECORD;
unsigned int def_rand_tests = DEF_REC_RANDOM;
unsigned int def_rand_max = DEF_RAND_MAX;
unsigned int def_rand_min = DEF_RAND_MIN;
unsigned int max_record_size = MAX_RECORD_SIZE;
unsigned int def_init_char = BUFFER_INIT;
unsigned int def_err_lines = MAX_OUTPUT;
unsigned int def_perf_rec_size = MAX_RECORD_SIZE;
unsigned int def_num_perf_rec = DEF_NUM_PERF_REC;
unsigned int def_max_perf_rec = MAX_RECORD_SIZE;
unsigned int trans_num_files = DEF_NUM_FILES;
unsigned int trans_rec_file = NUM_REC_FILE;
unsigned int trans_record_size = DEF_POS_REC_SIZE;
unsigned int eof_num_files = DEF_EOF_FILES;
unsigned int eof_rec_file = DEF_EOF_NUMRECORDS;
unsigned int eof_record_size = DEF_EOF_RECSIZE;
unsigned int tar_base_recs = TAR_BASE_RECS;
unsigned int tar_add_recs = TAR_ADD_RECS;
unsigned int tar_rec_size = TAR_REC_SIZE;
int def_load_record = DEF_LOAD_RECORD;
int def_load_rs = DEF_LOAD_RS;
int def_timeout_rs = TIMEOUT_RS;
int def_timeout_nr = TIMEOUT_NR;


int  fd_save;
/*
 * These flags determine what tests to run.  They are modified
 * in the parse routine in response to flags specified on the command line.
 */
int  flag_a = 0;
int  flag_b = 0;
int  flag_B = 0;
int  flag_c = 0;
int  flag_C = 0;
int  flag_d = 0;
int  flag_e = 0;
int  flag_E = 0;
			/* -f is used */
int  flag_F = 0;
int  flag_g = 0;
int  flag_G = 0;
			/* -h is used */
int  flag_i = 0;
int  flag_j = 0;
int  flag_k = 0;
int  flag_l = 0;
int  flag_L = 0;
int  flag_m = 0;
int  flag_n = 1;
			/* -N is used */
			/* -o is used */
			/* -p is used */
int  flag_q = 0;
int  flag_r = 0;
int  flag_R = 0;
int  flag_s = 0;
int  flag_S = 0;
			/* -T is used */
			/* -t is used */
int  flag_v = 0;
int  flag_V = 0;
int  flag_w = 0;
int  flag_z = 0;
int  flag_Z = 0;
/*
 * Test in progress status.  This is used where different routines require
 * different behavior depending on which test is running.
 */
int eom_test_ip = 0;
int rw_test_ip = 0;
int perf_test_ip = 0;
int rand_rec_ip = 0;
int skip_rec_test_ip = 0;
int timeout_test_ip = 0;

/*
 * Error status info.  This is used to print out test status if an 
 * interrupt occurs.
 */
int pos_range_errs = 0;		
int pos_read_errs = 0;
int pos_validate_errs = 0;
int pos_move_errs = 0;
int file_range_errs = 0;
int file_move_errs = 0;
int file_read_errors = 0;
int file_val_errors = 0;
int results_init = 0;
int pass_back = 0;
int partial_read_size = 0;
int mtflush_unsupported = 0;
int nbuf_aborts = 0;

char *tape = DEFAULT_TAPE;	/* Device name to be tested. */
FILE *fd_logfile;		/* File to log to - if requested. */
int printstatus = TTYPRINT;	/* Where to log output to */
int lines_output = 0;		/* Number of error messages generated. */
nbuf_struct *list;		/* Start address of n-buf buffer space */
int expect_to_fail = 0;		/* Operation is expected to fail */
int openmode;			/* Modes to use in opening the tape. */
int write_to_tape;		/* Set if tape is writable */

/*
 * Table of supported loaders and associated attributes.
 */

loader_struct loaders[] = {
	{ DEV_TA91,			/* name */
	  6,				/* 6 tapes in input deck */
	  SEQUENTIAL_STACK,		/* attributes */
	  3				/* Max load time of 3 minutes */
	},
	{ DEV_TA90,			/* name */
	  6,				/* 6 tapes in input deck */
	  SEQUENTIAL_STACK,		/* attributes */
	  3				/* Max load time of 3 minutes */
	},
/* Source pool hack */
#ifdef DEV_TQL70
	{ DEV_TQL70,			/* name */
	  7,				/* 7 tapes in input deck */
	  SEQUENTIAL_STACK,		/* attributes */
	  3				/* Max load time of 3 minutes */
	},
#endif DEV_TQL70
#ifdef DEV_TBL70
	{ DEV_TBL70,			/* name */
	  7,				/* 7 tapes in input deck */
	  SEQUENTIAL_STACK,		/* attributes */
	  3				/* Max load time of 3 minutes */
	},
#endif DEV_TBL70
#ifdef DEV_TF70L
	{ DEV_TF70L,			/* name */
	  7,				/* 7 tapes in input deck */
	  SEQUENTIAL_STACK,		/* attributes */
	  3				/* Max load time of 3 minutes */
	},
#endif DEV_TF70L
	{ 0 }				/* End of table marker */
};



/****************************************************************************
 * Begin program routines here
 ****************************************************************************/

/*
 * main()
 *
 * Determine which tests are to be run.  Makes calls to individual
 * test routines.
 */
main(argc, argv)

	int argc;
	char *argv[];

{
	int  fd_read;
	char user_char;


        /*
         *      Parse the command line.
         */
        parse(argc, argv);

	/* 
	 * Make sure that some tests have been specified.
	 */
	if ((flag_e == 0) && (flag_R == 0) && (flag_F == 0) && (flag_s == 0)
		&& (flag_g == 0) && (flag_r == 0) && (flag_z == 0) &&
		(flag_a == 0) && (flag_j == 0) && (flag_k == 0) &&
		(flag_l == 0) && (flag_m == 0) && (flag_d == 0) &&
		(flag_L == 0) && (flag_q == 0) ) { 
		Print("No tests have been selected.  Type \"tapex -h\" for help.\n")
		exit(1);
	}
	/*
	 * Debug Mode
	 */
	debug = flag_v;
	if (flag_V)
		debug = 99 * flag_V;
#ifndef lint
	if (debug)
	    Print1("\nTAPEX - ULTRIX tape exerciser version:\n%s\n\n",sccsid);
#endif lint
	if (debug)
		Print1("Program output level is %d.\n",debug)

	/*
	 * Set the modes used to open the tape file.
	 */
	if (flag_w) {
		if (debug > 1) {
			Print("Opening the tape in read-only mode.\n");
		}
		openmode = O_RDONLY;
		write_to_tape = 0;
	}
	else {
		openmode = O_RDWR;
		write_to_tape = 1;
	}
	/*
	 * Open the tape device.  Use the O_NDELAY flag so that it can be
	 * determined if the device is offline.
	 */
	if ((fd_read  = open(tape,openmode|O_NDELAY)) < 0) {
		Print1("Cannot open %s\n", tape)
		if (debug > 10)
			Print1("Initial open failed, errno = %d\n",errno);
		do_errlog(LOG_ABORT);
		exit(1);
	}
	else { 
		Print1("\nTesting tape device %s  ",tape)
		/*
		 * Print out device info from DEVIOCGET
		 */
		tape_file(fd_read,DEVIO_NAME);
		Print("\n")
	}
	fd_save = fd_read;
	flush_output();

	/* 
	 * Verify that a tape file is specified and that it is online.
	 */
	if ((tape_file(fd_read,0)) == 0) {
		/*
		 * Give one chance to put the tape online.
		 */
		Print("Tape unit is not ready.  Try again?\n")
		Print("y/n > ")
		scanf("%c",&user_char);
		if ((user_char == 'y') || (user_char == '\n') ||
		    (user_char == 'Y')) {
			fd_read = cycle_tape(fd_read);
			if ((tape_file(fd_read,0)) == 0) {
				Print("\nTape is still offline.\n")
				do_errlog(LOG_OFFLINE);
				exit(1);
			}
		}
		else {
			Print("\n")
			do_errlog(LOG_OFFLINE);
			exit(1);
		}
	}

	/*
	 * Call scsi_tape to set max_record_size prior to prompting for
	 * params so that the upper bounds are known.
 	 */
	scsi_tape(fd_read);
	/*
	 * Interactive mode.
	 * Prompt user for test parameters.
 	 * This is purposely done after the open because a check is done
	 * in tape_file() to change the maximum record size for scsi tape
	 * drives.  Since the max record size is used in the bounds check of
	 * interactive parameters, the device must be opened first.
	 */
	if (flag_i) {
		get_test_params();
	}
	if (debug) {
	     Print1("Subtest error message threshold is %d\n",def_err_lines);
	}
	/*
	 * Exhaustive tests are to be run.
	 */
	if (flag_E) {
		Print("\nExhaustive tests will be performed.\n")
		Print("Due to the large number of tests, this will take\n")
		Print("a long time to complete.\n")
	}
	/*
	 * Initialize read buffer elements to be nonzero if set.
	 * Debug usage intended.  For example if you want to make sure that
	 * your driver is pulling in the data it may be useful to have the
	 * read buffer initialized to be nonzero to see who is filling in
	 * what within the read buffer.
	 */
	if (flag_Z) {
		def_init_char = ALT_BUF_INIT;
		Print1("Elements of the read buffer will be initilized to 0%o\n",def_init_char);
	}
	/*
	 * Enable cache. 
	 */
	if (flag_c) {
		cache_setup(fd_read);
	}
	else {
		if (debug > 1)
			Print("disabling tape cache\n")
		/*
		 * SCSI tapes are intended to operate with cache enabled.
		 * Apparently "cache" does not mean quite the same thing in
		 * the SCSI world as it does in the MSCP world.
		 */
		if (scsi_tape(fd_read)) {
			if (debug)
			Print("Leaving cache enabled for a SCSI tape.\n");
		}
		/*
		 * If this is a TMSCP device then use caching as the default
		 * mode of operation unless told otherwise.
		 */
		else if (tmscp_tape(fd_read) && (flag_C == 0)) {
			flag_c = 1;
			if (debug > 1)
				Print("Enabling cache on this TMSCP drive.\n");
			cache_setup(fd_read);
		} 
		else {
			do_tape_op(fd_read,MTNOCACHE,1);
		}
	}

	/*
	 * Catch signals to properly terminate testing.
	 */
	signal(SIGTERM,sig_handler);
	signal(SIGINT,sig_handler);

	flush_output();
	/*
	 * Between each test, zero out the number of lines output.  The
	 * intent of this variable is to limit the number of lines printed out
	 * per test.  In case a test goes bonkers to prevent the size of the
	 * logfile from getting out of hand.
	 */
	lines_output = 0;

	/*
	 * If this program is not run as root user then there will be no start /
 	 * stop messages sent to the error logger.  
	 */
	if (geteuid()) {
		Print("\nNOTE: No messages will be sent to the error logger\n");
		Print("because this program is not being run as user root.\n\n");
	}
	else {
		do_errlog(LOG_START);
	}
	/**********************************************************************
	 * BEGIN TESTING HERE
	 **********************************************************************/
	
	/*
	 * Read phase of tape transportability test.
	 *
	 * This MUST be the first test to prevent other tests from first
	 * writing over what was written by the write phase of this test.
	 */
	if (flag_k) {
                /*
                 * Close and reopen tape to avoid inter-test effects.
                 */
                fd_read = cycle_tape(fd_read);
		do_errlog(START_READTRANS);
		tr_read_test(fd_read);
	}
	flush_output();

	/*
	 * Random record size write/read no validate tests.
	 */
	if (flag_g) {
		/*
		 * Close and reopen tape to avoid inter-test effects.
		 */
		fd_read = cycle_tape(fd_read);
		do_errlog(START_RANDREC);
		rand_record(fd_read);
	}

	flush_output();

	/*
	 * Positioning tests.
	 * forward/backward skip files and records.
	 */
	if ((flag_R) || (flag_F)) {
		if (flag_F) {
			/*
		 	 * Close and reopen tape to avoid inter-test effects.
		 	 */
			fd_read = cycle_tape(fd_read);
			lines_output = 0;
			do_errlog(START_FILEPOS);
			position_file_test(fd_read);
		}
		if (flag_R) {
			/*
		 	 * Close and reopen tape to avoid inter-test effects.
		 	 */
			fd_read = cycle_tape(fd_read);
			lines_output = 0;
			do_errlog(START_RECPOS);
			position_record_test(fd_read);
		}
	}

	flush_output();
	/*
	 * Record positioning informational output.  This is not strictly a
	 * test, but outputs status of record movements.
	 */
	if (flag_z) {
		/*
		 * Close and reopen tape to avoid inter-test side effects.
		 */
		fd_read = cycle_tape(fd_read);
		lines_output = 0;
		do_errlog(START_RECMOVE);
		position_info(fd_read);
	}

	flush_output();
	/*
	 * If exhaustive tests have been specified and this is a worm media
	 * then skip the write past end of media test.  If this test was
	 * performed it would consume all the available media and prevent
	 * the other tests from being run.
	 */
	if (flag_E && worm_tape(fd_read)) {
		Print("\n");
		Print("The write past end of media test is being skipped\n");
		Print("because a sequence of tests have been requested to\n");
		Print("run on a write-once media.  If the write past end\n");
		Print("of media test was performed the media capacity would\n");
		Print("be consumed which would prevent the other tests from\n");
		Print("running.  The write past end of media test may be\n");
		Print("run on this device by individually selecting that\n");
		Print("test.\n");
		Print("For the same reason the command timeout test will\n");
		Print("also not be run.\n");
		Print("\n");
		flag_e = 0;
		flag_q = 0;
	}

	/*
	 * EOM test.  Write past the end of tape and verify writes.
	 */
	if (flag_e) {
		/*
		 * Close and reopen tape to avoid inter-test side effects.
		 */
		fd_read = cycle_tape(fd_read);
		lines_output = 0;
		do_errlog(START_EOM);
		eom_test(fd_read);
	}

	flush_output();
	/*
	 * Record sizing tests.
	 */
	if (flag_s) {
		/*
		 * Close and reopen tape to avoid inter-test side effects.
		 */
		fd_read = cycle_tape(fd_read);
		lines_output = 0;
		do_errlog(START_SIZE);
		size_testing(fd_read);
	}
	flush_output();
	/*
	 * Performance test.
	 */
	if (flag_a) {
		/*
		 * Close and reopen tape to avoid inter-test side effects.
		 */
		fd_read = cycle_tape(fd_read);
		lines_output = 0;
		do_errlog(START_PERF);
		performance_test(fd_read);
	}
	flush_output();

	/*
	 * End of file tape mark test.
	 */
	if (flag_l) {
		/*
		 * Close and reopen tape to avoid inter-test effects.
		 */
		fd_read = cycle_tape(fd_read);
		do_errlog(START_EOF);
		eof_test(fd_read);
	}

	/*
	 * Media loader testing.
	 */
	if (flag_L) {
		/*
		 * Close and reopen tape to avoid inter-test effects.
		 * This will also cause a read-only open if -w is set.
		 */
		fd_read = cycle_tape(fd_read);
		do_errlog(START_LOADER);
		loader_test(fd_read);
	}

	flush_output();
	/*
	 * tar "r" test.  Test the ability to append to the media.
	 */
	if (flag_d) {
		/*
		 * Close and reopen tape to avoid inter-test effects.
		 */
		fd_read = cycle_tape(fd_read);
		do_errlog(START_TAR);
		tar_r_test(fd_read);
	}

	flush_output();
	/*
	 * Command timeout test.
	 */
	if (flag_q) {
		/*
		 * Close and reopen tape to avoid inter-test effects.
		 */
		fd_read = cycle_tape(fd_read);
		do_errlog(START_TIMEOUT);
		timeout_test(fd_read);
	}

	flush_output();
	/*
	 * Write/read record test over a range of record sizes.
	 */
	if (flag_r) {
		/*
		 * Close and reopen tape to avoid inter-test side effects.
		 */
		fd_read = cycle_tape(fd_read);
		lines_output = 0;
		do_errlog(START_RDWR);
		read_write_test(fd_read);
	}

	/*
	 * Write phase of tape transportability test.
	 *
	 * This MUST be the last test to prevent other tests from 
	 * writing over what was written by the write phase of this test.
	 */
	if (flag_j) {
                /*
                 * Close and reopen tape to avoid inter-test effects.
                 */
                fd_read = cycle_tape(fd_read);
		do_errlog(START_WRITETRAN);
		tr_write_test(fd_read);
	}
	flush_output();

	/*
	 * Tape disection test.  Print out what's on the tape.
	 */
	if (flag_m) {
                /*
                 * Close and reopen tape to avoid inter-test effects.
                 */
                fd_read = cycle_tape(fd_read);
		do_errlog(START_DISECT);
		disect_tape(fd_read);
	}

	Print("\n----------------------------------------------------------\n");
	print_time();
	do_errlog(LOG_STOP);
	/*
	 * TODO: Print out a quick test summary here.
	 */
	Print("Tape exerciser completion.\n");
	flush_output();
	/*
	 * Close opened files.
	 */
	close (fd_read);
	/*
	 * Set exit status to 0 to indicate success.
	 */
	exit(0);
}

/*
 * Record reading and writing test.
 *
 * This test consists of writing records to the tape, then rewinding and
 * reading back the records to verify that what is read is the same as what
 * was written.  The process is repeated in a loop for a range of record
 * sizes.
 */
read_write_test(fd_read) 
	int fd_read;
{
	int  saveflg_s = 0;
	char *error_base;
	int  error_found = 0;
	int  errors_exist = 0;
	int  counter, record_number, record_size;
	long stoptime;
	unsigned long total_chars = 0;
	int index = 0;
	int pass_number = 0;

	flush_output();
	/*
	 * write/read record test
 	 * writes records in a range of sizes to the tape, rewinds then reads
	 * to verify that what was written is the same as what is read.
	 */
	if (flag_r) {
		/*
		 * Save this flag value to avoid errors with the read/write
		 * test which would occur in the read_record routine.
		 */
		saveflg_s = flag_s;
		flag_s = 0;

		Print("\n-------------------------------------------------\n")
		Print("\nWrite and read data records test.\n\n")
		if (flag_S) {
			Print1("Records of size %d bytes  will be written\n",
				def_min_record);
			/*
			 * Paranoia check to insure that a range of tests is
			 * not performed.
			 */
			def_max_record = def_min_record;
		}
		else {
			Print2("This will write records of size %d bytes to %d bytes.\n",
				def_min_record,def_max_record)
		}
		if (flag_b == 0)
		Print1("%d records of each size will be written and verified\n",
			def_num_record)
		if (flag_S == 0)
		     Print1("The record increment value is %d bytes.\n",def_inc_record)
		/*
		 * Calculate the number of characters written in total.
		 * For informational purposes only.
		 */
		for (counter = def_min_record; counter <= def_max_record; 
				counter += def_inc_record) {
			total_chars += (def_num_record * counter);
		}
		if (flag_b == 0)
		Print1("A total of %d bytes will be written and verified\n",
			total_chars)
	}


	flush_output();
	/*
	 * Use n-buffered I/O for reads and writes.
	 */
	if (flag_n) {
		nbuf_setup(fd_read, def_nbuf_buffers, 0);
	}

	/*
	 * Malloc space for resutls array.  This array will store any errors.
	 */
	if ((error_base = (char *)malloc(def_max_record*(sizeof(error_st)))) <= (char *)0){
		Print("Unable to allocate memory for buffer space.\n");
		Print("Exiting from this test.\n");
		return;
	}
	err_ptr = (error_st *)error_base;

	/*
	 * initialize error structures
 	 */
	for (counter = 0; counter < def_max_record; counter++) {
		err_ptr[counter].read_errors = 0;
		err_ptr[counter].write_errors = 0;
		err_ptr[counter].validate_errors = 0;
		err_ptr[counter].buf_sz = 0;
	}
	results_init = 1;

	
	/*
	 * Run these write/read tests for the specified time period.
	 */
	if (def_time_delta) {
		stoptime = time(0) + (def_time_delta * 60);
		if (debug)
			Print1("Testing will conclude in %d minutes\n",def_time_delta)
	}
	else if (debug > 50)
		Print("No time limit is being imposed on this test.\n")
	
	if (flag_b) { 
		Print("Write \\ Read tests will be run in a continuous busy mode.\n")
	}
	rw_test_ip = 1;
    /*
     * Start with a record size of def_min_record and go up to def_max_record.
     */
    for (record_size = def_min_record, index = 0; record_size <= def_max_record;
		record_size += def_inc_record, index++) {
	/*
	 * Before starting the next record size see if the time period has
	 * expired.  If so print out the stats on what has been tested.
	 */
	if (def_time_delta && (stoptime < time(0))) {
		Print1("\nTime interval of %d minutes has expired.\n",
				def_time_delta)
		do_errlog(LOG_TIMEUP);
		sig_handler();
	}
	err_ptr[index].buf_sz = record_size;
	if (debug)
		Print1("record size = %d bytes.\n",record_size)
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd_read,MTREW,1);

	/* 
	 * write the buffer out to tape.  Each write is a separate record.
	 */
	/*
	 * Synchronous writes, which don't use n-buffered I/O.
	 * Write out records one at a time.
	 */
	if (flag_n == 0) {
		if (debug > 2)
			Print("Writing record # ")
		for (counter = 0; counter < def_num_record; counter++) {
			if ((write_record(fd_read,record_size,counter)) == 0) {
				if (debug > 2)  {
					Print1("%d ",counter)
					flush_output();
				}
			}
			else {
				err_ptr[index].write_errors++;
			}
		}
		if (debug > 2)
			Print("\n")
	}
	/*
	 * n-buffered I/O writes.
	 */
	else {
		if (debug > 3)
			Print("Doing n-buffered writes\n")
		nbuf_write(fd_read, record_size, def_num_record, &err_ptr[index].write_errors,0);
	}
	cache_flush(fd_read);
	/*
	 * Make it a clean file by slapping down a tape mark.
	 * This helps make things obvious if too many reads are attempted.
	 * Pass a count of 2 to indicate logical end of media.
	 */
	do_tape_op(fd_read,MTWEOF,2);

	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd_read,MTREW,1);

	/*
	 * read in buffer from the tape.
	 */
	if (debug > 2) {
		Print("Reading record # ")
		flush_output();
	}
	/*
	 * Synchronous reads, which don't use n-buffered I/O.
	 * Read in records one at a time and validate read contents.
	 */
	if (flag_n == 0) {
	    for (record_number = 0; record_number < def_num_record; record_number++) {
		if (!(read_record(fd_read,record_size,record_number,VALIDATE,
		 &err_ptr[index].read_errors,&err_ptr[index].validate_errors,
		 NO_INIT))){
			if (debug > 2) {
				Print1("%d ",record_number)
				flush_output();
			}
		}
	    }
	}
	/*
	 * Do asychronous n-buffered I/O reads.
	 */
	else {
		if (debug > 10)
			Print("Perform n-buffered I/O reads.\n")
		nbuf_read(fd_read, record_size, def_num_record, 
		   &err_ptr[index].read_errors,&err_ptr[index].validate_errors,0);
	}
	if (debug > 2)
		Print("\n")
	/*
	 * Check to see if the busy flag is set.  This is used to indicate
	 * that the tests are to be run without end.  The assumption here is
	 * that the tape exerciser is intended to generate tape activity or
	 * a certian system load until a kill signal is received.
	 *
	 * See if the tests were about to exit.  If so bring things back to 
	 * the beginning for another round.
	 */
	if (flag_b) {
		if ((record_size + def_inc_record) > def_max_record) {
			pass_number++;
			if (debug) {
				Print("\nRestarting write / read tests.\n")
				Print1("Pass number: %d\n",pass_number);
			}
			record_size = def_min_record - def_inc_record;
			index = 0;
		}
	}
    }

	/*
	 * Now that all the reads and writes have completes, print out a header
	 * followed by the number of errors.	
	 * Even if the line limit is exceeded, still print out the results.
	 *
	 * Before printing out the header line, run through the table to see
	 * if any errors have occured.
	 */

	for (record_size = def_min_record,index = 0; 
		record_size <= def_max_record; 
		record_size += def_inc_record, index++) {
		if (err_ptr[index].read_errors || err_ptr[index].write_errors ||
			err_ptr[index].validate_errors) {
			errors_exist++;
			break;
		}
	}
	lines_output = 0;
    if ((errors_exist) || (debug > 150)) {
	Print("\nRECORD\tREAD\tWRITE\tVALIDATE")
	Print("\nSIZE\tErrors\tErrors\tErrors\n")
	for (record_size = def_min_record,index = 0; record_size <= def_max_record; 
			record_size += def_inc_record, index++) {
		if (err_ptr[index].read_errors || err_ptr[index].write_errors ||
			err_ptr[index].validate_errors) {
			Print2("%d\t%d\t",err_ptr[index].buf_sz,err_ptr[index].read_errors)
			Print2("%d\t%d\n",err_ptr[index].write_errors,err_ptr[index].validate_errors)
			error_found++;
		}
		else if (debug > 150) {
			Print2("%d\t%d\t",err_ptr[index].buf_sz,err_ptr[index].read_errors)
			Print2("%d\t%d\n",err_ptr[index].write_errors,err_ptr[index].validate_errors)
		}
	}
    }
    else {
	Print("\n");
    }
	if (error_found == 0) {
		Print("SUCCESS: no errors encountered in write/read test.\n");
	}
	else {
		Print1("Total number of flawed record sizes is %d\n",error_found);
	}
	flush_output();
	/*
	 * Disable n-buffered I/O.
	 */
	if (flag_n) {
		nbuf_disable(fd_read, def_nbuf_buffers);
	}
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd_read,MTREW,1);
	flush_output();
	/*
	 * Restore flags to the values they had prior to the start of the
	 * read/write test.
	 */
	flag_s = saveflg_s;
	/*
	 * Release allocated memory.
	 */
	free(error_base);
	rw_test_ip = 0;
}

/*
 * Verify that the file descriptor is a tape device and that it is online.
 * Returns: 0 = not an online tape, 1 = online tape.
 */
int
tape_file(fd_op,devio_action)
	int fd_op;
	int devio_action;
{
	struct devget dev_st;

	if ((ioctl(fd_op,DEVIOCGET,&dev_st)) < 0) {
		Print("DEVIOCGET failed\n")
		return (0);
        }
        else {
		/*
		 * A request has been made to print out the device name.
		 */
		if (devio_action == DEVIO_NAME) {
		    Print(" (")
                    if((strcmp(DEV_UNKNOWN,dev_st.interface) == 0) ||
                       (dev_st.ctlr_num == -1)) {
                            Print(" unknown device type ")
                    } else {
                            Print2("interface %s, controller #%d, ", dev_st.interface,
                                   dev_st.ctlr_num)
                    }
                    if(!(strcmp(DEV_UNKNOWN,dev_st.device) == 0)) {
                            Print1("%s", dev_st.device)
                    }
		    Print(")")
		}
		/*
		 * For debug puropses.  Print out contents of status fields.
		 */
		else if (devio_action == DEVIO_STATUS) {
			Print("\nDEVIOCGET status:\n");
			Print1("Soft error count = %d\n",dev_st.soft_count);
			Print1("Hard error count = %d\n",dev_st.hard_count);
			Print1("Stat = 0x%x\n",dev_st.stat);
			if (dev_st.stat) {
			Print("\tStat = \(");
			if (dev_st.stat & DEV_BOM)
				Print("DEV_BOM ");
			if (dev_st.stat & DEV_EOM)
				Print("DEV_EOM ");
			if (dev_st.stat & DEV_OFFLINE)
				Print("DEV_OFFLINE ");
			if (dev_st.stat & DEV_WRTLCK)
				Print("DEV_WRTLCK ");
			if (dev_st.stat & DEV_BLANK)
				Print("DEV_BLANK ");
			if (dev_st.stat & DEV_WRITTEN)
				Print("DEV_WRITTEN ");
			if (dev_st.stat & DEV_CSE)
				Print("DEV_CSE ");
			if (dev_st.stat & DEV_SOFTERR)
				Print("DEV_SOFTERR ");
			if (dev_st.stat & DEV_HARDERR)
				Print("DEV_HARDERR ");
			if (dev_st.stat & DEV_DONE)
				Print("DEV_DONE ");
			if (dev_st.stat & DEV_RETRY)
				Print("DEV_RETRY ");
			if (dev_st.stat & DEV_ERASED)
				Print("DEV_ERASED ");
			Print("\)\n");
			}
			Print1("Category_stat = 0x%x\n",dev_st.category_stat);
			if (dev_st.category_stat) {
			Print("\tCategory_stat = \(");
			if (dev_st.category_stat & DEV_TPMARK)
				Print("DEV_TPMARK ");
			if (dev_st.category_stat & DEV_SHRTREC)
				Print("DEV_SHRTREC ");
			if (dev_st.category_stat & DEV_RDOPP)
				Print("DEV_RDOPP ");
			if (dev_st.category_stat & DEV_RWDING)
				Print("DEV_RWDING ");
			if (dev_st.category_stat & DEV_800BPI)
				Print("DEV_800BPI ");
			if (dev_st.category_stat & DEV_1600BPI)
				Print("DEV_1600BPI ");
			if (dev_st.category_stat & DEV_6250BPI)
				Print("DEV_6250BPI ");
			if (dev_st.category_stat & DEV_6666BPI)
				Print("DEV_6666BPI ");
			if (dev_st.category_stat & DEV_10240BPI)
				Print("DEV_10240BPI ");
/* source pool hack, remove for v.pu */
#ifdef DEV_38000BPI
			if (dev_st.category_stat & DEV_38000BPI)
				Print("DEV_38000BPI ");
#endif DEV_38000BPI
#ifdef DEV_LOADER
			if (dev_st.category_stat & DEV_LOADER)
				Print("DEV_LOADER ");
#endif DEV_LOADER
#ifdef DEV_38000_CP
			if (dev_st.category_stat & DEV_38000_CP)
				Print("DEV_38000_CP ");
			if (dev_st.category_stat & DEV_76000BPI)
				Print("DEV_76000BPI ");
			if (dev_st.category_stat & DEV_76000_CP)
				Print("DEV_76000_CP ");
#endif DEV_38000_CP
			Print("\)\n");
			}
			Print("\n");
		}
		/*
		 * Make sure the tape device is ready for testing.  Return a
		 * 0 if the device is not ready.
		 */
		else {
			if (dev_st.category != DEV_TAPE) {
				Print("Not a tape device\n")
				return(0);
			}
			if (dev_st.stat & DEV_OFFLINE) {
				Print("Tape unit offline\n")
				return(0);
			}
			if (dev_st.stat & DEV_WRTLCK) {
				Print("Tape unit write protected\n")
				/*
				 * Write protect is a fatal error unless
				 * the unit is to be opened read-only which
				 * is the case when flag_w is set.
				 */
				if (flag_w == 0)
					return(0);
			}
		}
	}
	return(1);
}

/*
 * See if the end of tape has been encountered.  If so return 1.
 */
int 
tape_eot(fd_op)
	int fd_op;
{
	struct devget dev_st;

	if ((ioctl(fd_op,DEVIOCGET,&dev_st)) < 0) {
		Print("DEVIOCGET failed\n")
		return (0);
        }
        else {
		if (dev_st.category != DEV_TAPE) {
			Print("Not a tape device\n")
			return(0);
		}
		if (dev_st.stat & DEV_EOM) {
			if (debug > 9)
				Print("\nDEV_EOM encountered\n")
			return(1);
		}
	}
	return(0);
}

/*
 * Interactive mode.
 *
 * Prompt the user for test paramters.  Such as record size and the number
 * of records of each record size to put down onto the tape.
 *
 * This routine obviously needs to be rewritten to call a subroutine to
 * read in a number.
 */
get_test_params()
{

	Print("Interactive mode has been specified.  You will now be asked\n")
	Print("enter the test paramters.  A range of acceptable values is\n");
	Print("provided, followed by the default in [].  Enter only a\n");
	Print("carriage return to select the default value.\n\n");

    /*
     * write / read records test params.
     */
    if (flag_r) {
	/*
	 * flag_S means to test only a single record size and not a range of
	 * record sizes.
 	 */
	if (flag_S) {
		Print("Tests will be run on a single record size.\n")
		Print("Enter the record size to be tested\n")
		def_min_record = get_number(MIN_RECORD_SIZE, max_record_size, 
						def_min_record);
		if (def_max_record < def_min_record) {
			def_max_record = def_min_record + 100;
			if (def_max_record > max_record_size)
				def_max_record = max_record_size;
		}
		Print("\n")
		/*
		 * Skip examining a range of record sizes by making the upper
		 * limit equal to the lower limit.
		 */
		def_max_record = def_min_record;
	}
	else {
	    	Print("Records in a range of sizes will be tested.  First enter\n")
	    	Print("The MINIMUM record size to be tested\n")
	    	def_min_record = get_number(MIN_RECORD_SIZE, max_record_size, 
					def_min_record);
	    
	    	if (def_max_record < def_min_record) {
	    		def_max_record = def_min_record + 100;
	    		if (def_max_record > max_record_size)
	    			def_max_record = max_record_size;
	    	}
	    	Print("\n")
	    	Print("Enter the MAXIMUM record size.  This number should be in \n")
	    	Print2("the range of %d to %d\n",def_min_record, max_record_size)
	    	def_max_record = get_number(def_min_record, max_record_size, 
					def_max_record);
	    
	}
	Print("\n")
	Print("Enter the number of records of each size to be written.\n")
	Print2("The range is %d to %d\n",MIN_RECORDS, MAX_RECORDS)
	def_num_record = get_number(MIN_RECORDS, MAX_RECORDS, 
				def_num_record);
	    
	Print("\n")
	/*
	 * Handle the case where you want to test records of only one
	 * size.
	 */
	if (def_max_record == def_min_record) {
		def_inc_record = 1;
	}
	else {
		Print("Enter the size of the record increment\n")
		Print2("The range is %d to %d\n",MIN_INCREMENT, def_max_record - def_min_record)
		def_inc_record = get_number(MIN_INCREMENT, 
				def_max_record - def_min_record, MIN_INCREMENT);
 	}

	/*
	 * Use n-buffered I/O.
	 */
	if (flag_n) {
		Print("\n")
		Print("Enter the number of buffers to be used for n-buffered I/O\n")
		Print2("The range is %d to %d\n",MIN_NBUFFERS, MAX_NBUFFERS)
		def_nbuf_buffers = get_number(MIN_NBUFFERS, MAX_NBUFFERS, def_nbuf_buffers);
	}
	
	Print("\n")
	
	if (debug) {
		Print1("MIN record size = %d bytes.\n",def_min_record)
		Print1("MAX record size = %d bytes.\n",def_max_record)
		Print1("Number of records = %d\n",def_num_record)
		Print1("Record size increment = %d bytes.\n",def_inc_record)
		if (flag_n)
			Print1("Number of buffers for n-buffered I/O is %d\n",
				def_nbuf_buffers)
	}
    }
    /*
     * Write past end of media tests.
     */
    if (flag_e != 0) { 
	Print("Enter the size of the records to be written past EOT\n")
	def_eof_record = get_number(MIN_RECORD_SIZE, max_record_size, def_eof_record);
	Print("Enter the number of records to be written past EOT\n")
	def_eof_numrec = get_number(EOT_MIN_RECORDS, EOT_MAX_RECORDS, def_eof_numrec);

	if (debug) {
		Print1("Number of records past EOM = %d\n",def_eof_numrec)
		Print1("Size of records past EOM = %d\n",def_eof_record)
	}

    }
    /*
     * Positioning tests.
     */
    if (flag_F || flag_R) { 
	Print("Enter the record size used in record positioning tests\n")
	pos_rec_size = get_number(MIN_RECORD_SIZE,max_record_size,DEF_POS_REC_SIZE);
	Print("Enter the number of random position tests to run\n")
	def_num_random = get_number(MIN_RANDOM, MAX_RANDOM,NUM_RANDOM_TESTS);

	if (flag_R) {
	Print("Enter the number of records to be used in record positioning\n")
	def_pos_record = get_number(MIN_RECORDS,MAX_RECORDS,DEF_POS_NUMREC);
	}
	if (flag_F) {
	Print("Enter the number of files for the file positioning test\n")
	def_num_files = get_number(MIN_FILES,MAX_FILES,DEF_NUM_FILES);
	Print("Enter the number of records per file\n")
	def_rec_file = get_number(MIN_REC_FILE,MAX_REC_FILE,NUM_REC_FILE);
	}
    }
    /*
     * Record sizing tests.
     */
    if (flag_s) {
	Print("\nParameters for the record sizing test:\n")
	Print("Enter the number of records to be written.\n")
	Print2("The range is %d to %d\n",MIN_RECORDS, MAX_RECORDS)
	def_num_record = get_number(MIN_RECORDS, MAX_RECORDS, def_num_record);
	Print("Enter the record size for record size testing.\n")
	Print2("The range is %d to %d\n",MIN_RECORD_SIZE, max_record_size)
	def_size_record = get_number(MIN_RECORD_SIZE, max_record_size, def_size_record);
    }
    /*
     * Random record size test.  This parameter specifies the number of
     * records to write and read.
     */
    if (flag_g) {
	Print("\nParameters for random record size test:\n")
	Print("Enter the number of records to be tested.\n")
	Print2("The range is %d to %d\n",MIN_RAND_SIZE,DEF_MAX_RANDOM)
	def_rand_tests = get_number(MIN_RAND_SIZE, DEF_MAX_RANDOM, def_rand_tests);
    }
    /*
     * Performance test params.
     * Get record size and number of records.
     */
    if (flag_a) {
	Print("\nParameters for performance test:\n")
	Print("Enter the number of records to be tested.\n")
	Print2("The range is %d to %d\n",MIN_RECORDS,MAX_RECORDS)
	def_num_perf_rec = get_number(MIN_RECORDS,MAX_RECORDS,def_num_perf_rec);
	Print("Enter the records size.\n")
	Print2("The range is %d to %d\n",MIN_RECORD_SIZE,max_record_size)
	def_perf_rec_size = get_number(MIN_RECORD_SIZE, max_record_size, def_perf_rec_size);
    }
    /*
     * Transportability write/read test params.
     */
    if ((flag_j) || (flag_k)) {
	Print("\nParameters for the tape transportability test.\n");
	Print("NOTE: The same parameters MUST be used for both the\n");
	Print("write and read phase of this test.\n\n");
	Print("Enter the number of files for the file transportability test\n")
	trans_num_files = get_number(MIN_FILES,MAX_FILES,DEF_NUM_FILES);
	Print("Enter the number of records per file\n")
	trans_rec_file = get_number(MIN_REC_FILE,MAX_REC_FILE,NUM_REC_FILE);
	Print("Enter the record size.\n");
	trans_record_size = get_number(MIN_RECORD_SIZE,MAX_RECORD_SIZE,DEF_POS_REC_SIZE);
    }
    /*
     * Append to media (tar r) test parameters.
     */
    if (flag_d) {
	Print("\nParameters for the append to media test.\n");
	Print("Enter the number of records to write to the tape.\n");
	Print("This same number of records will be appended.\n");
	tar_base_recs = get_number(MIN_RECORDS,MAX_RECORDS,tar_base_recs);
	tar_add_recs = tar_base_recs;
	Print("Enter the record size.\n");
	tar_rec_size = get_number(MIN_RECORD_SIZE,max_record_size,tar_rec_size);
    }
	flush_output();
}

/*
 * Get_number
 * Prompt the user for a number in a specified range.
 * Use the def value if only a carriage return (no number) is entered.
 * If the input number is out of range, reprompt the user for another
 * number.
 */
get_number(min,max,def)
	int min, max, def;
{
	int num_scan, user_number;
	char user_input[200];
	char newline_input;

tryagain:
	Print3("range \(%d-%d\), [%d] ",min,max,def);
	/*
	 * read up to a newline.  The newline remains in the input stream.
	 */
	num_scan = scanf("%[^\n]",user_input);
	/*
	 * remove the newline from the input stream.
	 */
	scanf("%c",&newline_input);
	if (num_scan == 1) {
		num_scan = sscanf(user_input,"%d",&user_number);
		Log_print( user_input );/* echo input to log file if logging */
	}
	if (num_scan != 1) {
		user_number = def;
		Log_print( "\0" );/* echo input to log file if logging */
	}
	/*
	 * A valid number has been entered by the user.  Call convert_value
	 * to handle any factors. (for example 10k, or 5b)
	 */
	else {
		user_number = convert_value(user_input);
	}
	if (user_number < min) {
		Print1("Number must be greater than %d\n", min)
		goto tryagain;
	}
	if (user_number > max) {
		Print1("Number must be less than %d\n", max)
		goto tryagain;
	}
	return(user_number);
}

/*
 * Execute a tape motion command by calling MTIOCTOP.
 * Opcode and count must be setup prior to calling.
 */
do_tape_op(tape_fd,mt_command,mt_count)
	int mt_command, mt_count;
{
	struct mtop mt_comp;

	mt_comp.mt_op = mt_command;
	mt_comp.mt_count = mt_count;
	
	/*
	 * Reverse position hack:
	 *
	 * When reading and writing to a tape device the file
	 * offset just gets incremented; only to be cleared
	 * when the file is closed.  This causes the write/read
	 * test to fail when run in busy mode.  What happens is
	 * that the offset would eventually wrap around negative
	 * which causes the read/write system calls to fail with
	 * an errorno of EINVAL.  To avoid this problem use
	 * lseek to reset this file offset to 0.
	 */
	switch (mt_command) {
		case MTREW:
		case MTBSF:
		case MTBSR:
			lseek(tape_fd,0,0);
			if (debug > 200) {
				Print("lseek\(0\) to reset file offset.\n")
			}
			break;
	}
	if (debug > 10) {
		if (mt_comp.mt_op == MTREW)
			Print("rewinding tape\n")
	}
	/*
	 * Sanity check.  For the following ioctls where the count field
	 * is used, make sure it is not negative.
	 */
	switch (mt_command) {
                case MTFSF:     
                case MTBSF:    
                case MTFSR:   
                case MTBSR:  
			if (mt_count < 0) {
				Print1("WARNING: negative count on positioning, count = %d\n",mt_count);
			}
	}
	if ((ioctl(tape_fd,MTIOCTOP,&mt_comp)) < 0) {
	    	if (skip_rec_test_ip == 0) {
		Print2("MTIOCTOP failed, op = %x, count = %d\n",
			mt_comp.mt_op, mt_comp.mt_count)
		Print("Failed MTIOCTOP command is ")
		switch (mt_command) {
			case MTWEOF:	Print("MTWEOF"); break;
			case MTFSF:	Print("MTFSF"); break;
			case MTBSF:	Print("MTBSF"); break;
			case MTFSR:	Print("MTFSR"); break;
			case MTBSR:	Print("MTBSR"); break;
			case MTREW:	Print("MTREW"); break;
			case MTOFFL:	Print("MTOFFL"); break;
			case MTNOP:	Print("MTNOP"); break;
			case MTCACHE:	Print("MTCACHE"); break;
			case MTNOCACHE:	Print("MTNOCACHE"); break;
			case MTCSE:	Print("MTCSE"); break;
			case MTCLX:	Print("MTCLX"); break;
			case MTENAEOT:	Print("MTENAEOT"); break;
			case MTDISEOT:	Print("MTDISEOT"); break;
#ifdef MTFLUSH
			case MTFLUSH:   Print("MTFLUSH"); 
					if (errno == ENXIO)
						Print("\nThis drive does not support the MTFLUSH command.\n");
					break;
#endif MTFLUSH
			default:	Print("unknown");
		    }
		Print("\n");
		if (debug > 10)
			Print1("Failed tape operation, errno = %d\n",errno);
		}
		/*
		 * If debugging mode is specified, print out unit status
		 * to have a better idea of possible cause of error.
	  	 */
		if (((debug > 10) && expect_to_fail) || 
		    ((debug) && (expect_to_fail == 0)) ) {
		    tape_file(tape_fd,DEVIO_STATUS);  
		}
		/*
		 * See if the failure was caused by the tape going
		 * offline.  If so abort testing.
		 */
		if ((tape_file(tape_fd,0)) == 0) {
			Print("\n*****************************************\n");
			Print("* Aborting tests                        *\n");
			Print("* Tape went offline during testing.     *\n");
			Print("*****************************************\n");
			print_time();
			do_errlog(LOG_OFFLINE);
			exit(1);
		}
		return(1);
	}
	return(0);
}

/*
 * A kill or terminate signal has been received.
 * Printout results of what has been tested so far.
 * These results are relevant only to the write/read records test.
 */
sig_handler()
{
	int record_size, index, error_found;
	error_found = 0;
	Print("\nTests interrupted.\n");
	print_time();
    /*
     * Display results of the write/read record test if it was the one in
     * progress when an interrupt signal was received.
     */
    if (results_init) {
	Print("\nWrite and read test results summary\n");
	Print("\nRECORD\tREAD\tWRITE\tVALIDATE");
	Print("\nSIZE\tERRORS\tERRORS\tERRORS\n");
	for (record_size = def_min_record,index = 0; err_ptr[index].buf_sz != 0;
			record_size += def_inc_record, index++) {
		if (err_ptr[index].read_errors || err_ptr[index].write_errors ||
			err_ptr[index].validate_errors ) {
			Print2("%d\t%d\t",err_ptr[index].buf_sz,err_ptr[index].read_errors)
			Print2("%d\t%d\n",err_ptr[index].write_errors,err_ptr[index].validate_errors)
			error_found++;
		}
		else if (debug > 50) {
			Print2("%d\t%d\t",err_ptr[index].buf_sz,err_ptr[index].read_errors)
			Print2("%d\t%d\n",err_ptr[index].write_errors,err_ptr[index].validate_errors)
		}
	}
	if (error_found == 0)
		Print("SUCCESS: no errors encountered in the write\\read test.\n")
	else
		Print1("Total number of flawed record sizes is %d\n",error_found)
    }
    /*
     * Display results of the random record test if it was the one in
     * progress when an interrupt signal was received.
     *
     * Since the write_size and read_returned arrays are local to the
     * rand_record routine the status is not available here.  The test does
     * check the results of each iteration of the read loop.  Provided that
     * a full cycle through the loop has completed, any errors would have
     * been displayed already.  
     */
    else if (rand_rec_ip) {
	Print("\n");
	Print("Tests were interrupted during the random record test.\n");
	Print("If there were any errors, error messages would have\n");
	Print("been generated.\n");
    }
	
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd_save,MTREW,1);
	/*
	 * flush output.
	 */
	flush_output();
	if (printstatus & LOGFILE) {
		fflush(fd_logfile);
		fclose(fd_logfile);
	}
	do_errlog(LOG_ABORT);
	exit(0);
}
/*
 * EOF test
 *
 * Verify that writes can be done past the end of tape if so enabled.
 * Also verify that the writes will fail if writing past end of tape is
 * not enabled.
 */
eom_test(fd_tape)
	int fd_tape;
{
	int proceed;
	int counter;
	int eot_failures = 0;
	int eot_read_failures = 0;
	int eot_writefail = 0;
	int eot_goodwrites = 0;
	int eot_validate_fail = 0;
	int eot_readfail = 0;
	int eot_goodreads = 0;
	int seed = 0;
	int dummy_read, dummy_validate;
	int eof_pos_fail = 0;
	int save_flag_s = 0;
	int wrt_errs;
	int filler_record_size;

	/*
	 * Problems occur in the read record routine if flag_s is set
	 * during this test.  Save the flag and restore it at the end
	 * of the routine.
	 */
	if (flag_s) {
		save_flag_s = flag_s;
		flag_s = 0;
	}

	Print("\n---------------------------------------------------------\n");
	Print("\nWrite past end of media testing\n\n");
	Print("The tape will be written to until it encounters \n");
	Print("the end of media, EOM.  Upon encountering EOM the tape unit\n");
	Print("will be in an error state which should cause any writes and\n");
	Print("reads to fail.  To verify this behavior a number of writes and reads\n");
	Print("are attempted which should fail.  Finally writing past EOM\n");
	Print("is enabled by clearing the drive's error state. At this\n");
 	Print("point writes are done, and read back to verify the data.\n\n");
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd_tape,MTREW,1);
	/*
	 * Fill up the tape with data until the EOM is encountered.
	 */
	proceed = 1;
	counter = 1;
	if (debug)
		Print("writing records to the tape\n");
	eom_test_ip = 1;       /* EOM writes in progress, used in nbuf write */
    if (flag_n) {
	filler_record_size = max_record_size;
	/*
	 * Setup for n-buffered I/O operation to fill up the tape with data.
	 * The data before the end of media foil is never read back.
 	 */
	nbuf_setup(fd_tape, def_nbuf_buffers, 0);
	if (debug > 1) {
		Print("Doing n-buffered writes\n")
		Print1("Fill to EOM with records of size %d bytes.\n",filler_record_size);
	}
	nbuf_write(fd_tape, filler_record_size, 0, &wrt_errs,0);
	nbuf_disable(fd_tape, def_nbuf_buffers);
	/*
	 * Make sure the writes made it to EOM.  Write failures could get us
	 * to this point without being at the end of tape.
	 */
	if ((tape_eot(fd_tape)) == 0) {
		Print("ERROR: write failure on EOM test.\n");
		Print("Aborting EOM test.  Errors have occured in writing\n");
		Print1("to EOM.  The number of write errors is %d\n",wrt_errs);
		/*
	 	 * Rewind the tape as minimal error recovery.
		 * If this fails then the drive is messed up.
	 	 */
       		if (do_tape_op(fd_tape,MTREW,1)) {
			Print("FATAL ERROR: rewind failure.\n");
			Print("Exerciser terminating.\n");
			do_errlog(LOG_REWFAIL);
			exit(1);
		}
		return;
	}
    }
    else { /* Don't use n-buffered I/O */
	while (proceed) {
		if ((write_record(fd_tape,def_eof_record,1)) == 0) {
			if (debug > 1)  {
				Print1("%d ",counter++);
				if ((counter % 15) == 0)
					Print("\n");
				flush_output();
			}
		}
		else {
			/*
			 * A failure has occured in writing this record.
			 *
			 * An error will be returned from write_record when
			 * the eot is seen, don't abort testing if this
			 * is the cause of error.
			 */
			if ((tape_eot(fd_tape)) == 0) {
				Print("ERROR: write failure on EOM test.\n");
				Print("Aborting EOM test, errors have \n");
				Print("occured in writing to EOM.\n");
				/*
	 			 * Rewind the tape as minimal error recovery.
				 * If this fails then the drive is messed up.
	 			 */
       				if (do_tape_op(fd_tape,MTREW,1)) {
					Print("FATAL ERROR: rewind in EOM\n");
					Print("Exerciser terminating.\n");
					do_errlog(LOG_REWFAIL);
					exit(1);
				}
				return;
			}
			else {
				proceed = 0;	/* EOM encountered */
			}
		}
	}
    }
	/*
	 * At this point we should be at the end of media .
	 */
	if ((tape_eot(fd_tape)) == 0) { /* paranoia */
		Print("Fatal error: should be at EOM, aborting\n");
		do_errlog(LOG_ABORT);
		exit(1);
	}
	if (debug > 1) {
		Print1("All records past EOM are of size %d bytes.\n",def_eof_record);
	}
	/*
	 * Try a write and see if it fails.  The write should fail because the
	 * serious exception associated with encountering the end of media 
	 * hasn't been cleared yet.
	 */
	if (debug)
		Print("Attempt writes to tape to verify failure when not enabled\n");
	for (counter = 0; counter < 10; counter++) {
		if ((write_record(fd_tape,def_eof_record,1)) == 0) {
			if (debug)
				Print("non-enabled write passed eot succeeded\n");
			eot_failures++;
		}
		else if (debug > 9) {
			Print1("%d ",counter);
			flush_output();
		}
	}
	/*
	 * Try a read and see if it fails.
	 * The read should fail because the serious exception hasn't been 
	 * cleared yet.
	 */
	if (debug)
		Print("\nAttempt reads to tape to verify failure when not enabled\n");
	for (counter = 0; counter < 10; counter++) {
		if (!(read_record(fd_tape,def_eof_record,1,NO_VALIDATE,
		       &dummy_read, &dummy_validate,NO_INIT))){
			if (debug)
				Print("non-enabled read passed eot succeeded\n");
			eot_read_failures++;
		}
		else if (debug > 9) {
			Print1("%d ",counter);
			flush_output();
		}
	}
	/*
	 * Now enable writing past eot.  Write some records, and verify.
	 */
	if (debug)
		Print("\nWrite past EOM when enabled\n");
	seed = 0;
	for (counter = 0; counter < def_eof_numrec; counter++) {
		/*
		 * Clear the serious exception to allow the writes to proceed.
		 */
		do_tape_op(fd_tape,MTCSE,1);
		
		seed++;
		if ((write_record(fd_tape,def_eof_rec_sz,seed)) == 0) {
			eot_goodwrites++;
			if (debug > 1)  {
				Print1("%d ",counter);
				flush_output();
			}
		}
		else {
			eot_writefail++;
		}
	}
	if (debug > 1)
		Print("\n");
	if (debug)
		Print("Read past EOM when enabled\n");
	/*
	 * Read back the records to validate the data.
	 * First back up to where the writes began, then read in records.
	 */
	seed = 0;
	if (eot_goodwrites) {
		do_tape_op(fd_tape,MTCSE,1);
		if (do_tape_op(fd_tape,MTBSR,eot_goodwrites)) {
			/*
			 * The tape was unable to backup the specified
			 * number of records.  To avoid going off the end
			 * of the tape reel, skip the reads.
			 */
			eof_pos_fail++;
			if (debug) {
				Print1("MTBSR %d failed\n",eot_goodwrites);
				Print("Skip the EOM reads to avoid falling\n");
				Print("off the end of the reel.\n");
			}
		}
		else {
		    for (counter = 0; counter < eot_goodwrites; counter++) {
			/*
			 * Clear the serious exception to allow the reads
			 */
			do_tape_op(fd_tape,MTCSE,1);

			seed++;
			if (!(read_record(fd_tape,def_eof_rec_sz,seed,VALIDATE,
		 	    &eot_readfail,&eot_validate_fail,NO_INIT))){
				eot_goodreads++;
				if (debug > 1) {
					Print1("%d ",counter);
					flush_output();
				}
			}
		    }
		}
	}
	if (debug > 1)
		Print("\n");
	/*
	 * Report results
	 */
	if (eot_failures) {
		Print1("FAILURE: %d records written past EOM when not enabled\n"
			,eot_failures );
	}
	else {
		Print("SUCCESS: no records written past EOM when not enabled\n");
	}
	if (eot_read_failures) {
		Print1("FAILURE: %d records read past EOM when not enabled\n"
			,eot_read_failures );
	}
	else {
		Print("SUCCESS: no records read past EOM when not enabled\n");
	}
	if (eot_writefail) {
		Print1("FAILURE: %d failures in writing records past EOM\n",
			eot_writefail);
	}
	if ((eot_goodwrites != def_eof_numrec)  &&  (eot_goodwrites)) {
		Print2("%d out of %d writes past EOM succeeded\n",
			eot_goodwrites , def_eof_numrec);
	}
	else if (eot_goodwrites == def_eof_numrec) {
		Print("SUCCESS: all writes past EOM succeeded\n");
	}
	if (eot_readfail)
		Print1("FAILURE: %d failures on past EOM reads\n",
			eot_readfail);
	if (eot_validate_fail)
		Print1("FAILURE: %d read validate failures on read past EOM\n",
			eot_validate_fail);
	if ((eot_goodreads) && (eot_goodreads != eot_goodwrites)) {
		Print2("%d successful reads out of %d records\n",
			eot_goodreads, eot_goodwrites);
	}
	else if (eof_pos_fail) {
		Print("FAILURE: It was not possible to verify the data\n");
		Print("         written past EOM due to a repositioning\n");
		Print("         error.\n");
	}
	else if (eot_goodreads == def_eof_numrec) {
		Print("SUCCESS: all reads past EOM verified\n");
	}
	if (save_flag_s)
		flag_s = save_flag_s;

	eom_test_ip = 0;       /* EOM test completed */
}

/*
 * Format the transmit buffer with a pattern.
 */
format_writebuf(write_st, read_st, max_rec,seed,fast)
	register rw_struct *write_st;
	register rw_struct *read_st;
	register int max_rec;
	int fast;
{
	register int counter;

	/*
	 * The buffer will contain characters.  The buffer character cells
	 * will contain characters of increasing value.  The seed value
	 * determines what the first character will be.  Different seeds
	 * result in different first character in the buffer.  This is done
	 * so that the records on tape will be "relatively" unique
	 * modulo 256.
	 *
	 * CAUTION: many of the tests depend on the linear nature of the 
	 * record patterns!
	 */
    if (fast == WRITING) {
	/*
	 * Only format what is absolutely necessary.
	 */
	for (counter = 1; counter <= max_rec; counter++) {
		write_st->buffer[counter-1] = (char)((counter+seed)&CHAR_MASK);
	}
    }
    else if (fast == READING) {
	for (counter = 1; counter <= max_rec; counter++) {
		write_st->buffer[counter-1] = (char)((counter+seed)&CHAR_MASK);
		read_st->buffer[counter-1] = def_init_char; /* init */
	}
    }
    else if (fast == SLOW) {
	/*
	 * Paranoia, format everything!
	 *
	 * This really slows things down.  This will prevent n-buffered I/O
	 * from streaming.  Particularly because it zeroes out up to
	 * MAX_RECORD_SIZE.
	 */
	for (counter = 1; counter <= max_rec; counter++) {
		write_st->buffer[counter-1] = (char)((counter+seed)&CHAR_MASK);
		read_st->buffer[counter-1] = def_init_char; /* init */
	}
	for (counter = max_rec+1; counter <= MAX_RECORD_SIZE; counter++) {
		write_st->buffer[counter-1] = 0;
		read_st->buffer[counter-1] = def_init_char;
	}
    }
    else {
	Print1("Format_writebuf: invalid format %d\n",fast);
	do_errlog(LOG_ABORT);
	exit(1);
    }
}

/*
 * Setup for n-buffered I/O.  Declare the number of buffers used and init
 * data structures.
 */
nbuf_setup(fd, n_buffers, action)
	int fd;
	int n_buffers;
	int action;
{
	int i;
	int zero = 0;
	char *malloc_ret;

	if (n_buffers < 1) {
		Print("Error: running n-buffered I/O with less than 1 buffer\n");
		do_errlog(LOG_ABORT);
		exit(1);
	}
	/*
	 * Only allocate buffers the first time.
	 */
	if (action == 0) {
		/*
		 * Allocate space for the buffers.
		 */
		if ((list = (nbuf_struct *) malloc(sizeof(nbuf_struct) * n_buffers)) == 0) {
			Print("nbuf_setup: can\'t allocate buffer");
			do_errlog(LOG_ABORT);
			exit(1);
		}
		/*
		 * Initilize each data structure.
		 */
		for (i = 0; i < n_buffers; i++) {
			if ((malloc_ret = (char *)malloc(max_record_size)) == 0) {
				Print("nbuf_setup: 2 can\'t allocate buffer");
				do_errlog(LOG_ABORT);
				exit(1);
			}
			if ((int)malloc_ret & 0x3) {
				Print("malloc buffer not on longword boundary\n");
				do_errlog(LOG_ABORT);
				exit(1);
			}
			list[i].buffer = malloc_ret;
		}
	}
	/*
	 * Disable n-buffering to flush out any pending operations.
 	 */
	if (ioctl(fd,FIONBUF,&zero) < 0) {
		Print("nbuf_setup: FIONBUF 0 failed");
		do_errlog(LOG_ABORT);
		exit(1);
	}
	/*
	 * Declare the number of n-buffers to be used.
	 */
	if (ioctl(fd,FIONBUF,&n_buffers) < 0) {
		Print("nbuf_setup: FIONBUF failed");
		do_errlog(LOG_ABORT);
		exit(1);
	}
	else if ((debug > 2) && (action == 0)) {
		Print1("n-buffered I/O enabled with %d buffers\n",n_buffers);
	}
}
/*
 * Turn off n-buffered I/O.
 */
nbuf_disable(fd, n_buffers)
	int fd;
	int n_buffers;
{
	int zero = 0;
	int i;
	/*
	 * Disable n-buffering to flush out any pending operations.
 	 */
	if (ioctl(fd,FIONBUF,&zero) < 0) {
		Print("nbuf_setup: FIONBUF 0 failed");
		do_errlog(LOG_ABORT);
		exit(1);
	}
	/*
	 * Free associated data structs.
	 */
	for (i = 0; i < n_buffers; i++) {
		free(list[i].buffer);
	}
	free(list);
}

/*
 * Position tests.
 *
 * Record Movement Tests
 * 1) Write a large number of records to the tape, each record should have 
 *    unique contents for identification later.
 * 2) Move around forwards and backwards records and verify reads.
 *
 * File Movement Tests
 * 1) Write a number of files composed of several records.
 * 2) forward/backward skip files and verify file contents.
 */
position_record_test(fd)
	int fd;
{
	int position;
	int record_number, counter;
	int write_err = 0;
	int read_err = 0;
	int validate_err = 0;
	int quarter;
	long rand_position;
	int num_move;
	int saveflag_e;
	int subtest_failure = 0;

	/*
	 * Set this flag zero so that the test will abort if the end of
	 * media is unexpectedly encountered in the exhaustive test mode.
	 */
	saveflag_e = flag_e;
	flag_e = 0;


	Print("\n---------------------------------------------------------\n");
	Print("\nRecord Position Testing\n");
	Print2("%d records of size %d bytes will be written.\n\n",def_pos_record,
			pos_rec_size);
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	/* 
	 * write the buffer out to tape.  Each write is a separate record.
	 */
	if (debug > 2)
		Print("Writing record # ");
	if (flag_n == 0) {
		/*
		 * flag_n is clear; Don't use n-buffered I/O.
		 */
		for (position = 0; position < def_pos_record; position++) {
			if ((write_record(fd,pos_rec_size,position)) == 0) {
				if (debug > 2)  {
					Print1("%d ",position);
					flush_output();
				}
			}
			else {
				write_err++;
			}
		}
	}
	else {
		/*
		 * flag_n is set; use n-buffered I/O.
		 */
		nbuf_setup(fd, def_nbuf_buffers, 0);
		if (debug > 3)
			Print("Doing n-buffered writes\n")
		nbuf_write(fd, pos_rec_size, def_pos_record, &write_err, 0);
		nbuf_disable(fd, def_nbuf_buffers);
	}
	if (debug > 1)
		Print("\n");
	cache_flush(fd);
	/*
	 * Make it a clean file by slapping down a tape mark.
	 * Pass a count of 2 to indicate logical end of media.
	 */
	do_tape_op(fd,MTWEOF,2);

	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	/*
	 * Read through the whole tape first to validate the writes.
	 * Before examining random records it's helpful to know that they
	 * are all valid records.
	 */
	if (flag_n == 0) {
		/*
		 * flag_n is clear; Don't use n-buffered I/O
		 */
		if (debug > 1)
			Print("Reading record # ");
		for (record_number = 0; record_number < def_pos_record; record_number++) {
			if (!(read_record(fd,pos_rec_size,record_number,VALIDATE
		    		,&read_err,&validate_err,NO_INIT))){
				if (debug > 1) {
					Print1("%d ",record_number);
					flush_output();
				}
			}
		}
	}
	else {
		/*
		 * flag_n is set; use n-buffered I/O
		 */
		nbuf_setup(fd, def_nbuf_buffers, 0);
		if (debug > 10)
			Print("Perform n-buffered I/O reads.\n")
		nbuf_read(fd, pos_rec_size, def_pos_record, &read_err, &validate_err, 0);
		nbuf_disable(fd, def_nbuf_buffers);
	}
	if (debug > 1)
		Print("\n");
	if (write_err || read_err || validate_err) {
		Print("Positioning tests being aborted due to initial errors\n");
		Print("Before any positioning is done the records are written\n");
		Print("onto the tape and then read back to verify the tape\n");
		Print("contents.  The following errors occured during this\n");
		Print("phase of the test.\n");
		if (write_err)
			Print1("FAILURE: %d write errors\n",write_err);
		if (read_err)
			Print1("FAILURE: %d read errors\n",read_err);
		if (validate_err)
			Print1("FAILURE: %d validate errors\n",validate_err);
		flag_e = saveflag_e;
		return;
	}
	else if (debug) {
		Print("Initial writes and reads have validated.\n");
	}

	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	position = 0;

	/*
	 * Move to different records and read in to validate.
	 */
	/*
	 * First a very simple pattern.
	 */
	if (def_pos_record > 7) {		
		if (debug)
			Print("Simple positioning exercise\n");
		/*
		 * Don't do the backward movement if the forwards fail.
		 */
							             /* 0 */
		if ((Rmove_record(fd,FORWARD,3,&position,READ)==0)&& /*0+3+1=4*/
		    (Rmove_record(fd,FORWARD,1,&position,READ)==0)){ /*4+1+1=6*/
		     Rmove_record(fd,BACKWARD,4,&position,READ);     /*6-4+1=3*/
		}
		Rmove_record(fd,FORWARD,3,&position,READ); 	     /*3+3+1=7*/
	}

	/*
	 * Read every other record forwards.
	 */
	do_tape_op(fd,MTREW,1);
	position = 0;
	if (debug)
		Print("read every other record forwards\n");
	for (counter = 0 ; counter < (def_pos_record/2); counter++) {
		if (Rmove_record(fd,FORWARD,1,&position,READ)) {
			Print("Terminating subtest after record movement failure.\n");
			subtest_failure = 1;
			break;
		}
	}
	/*
	 * Read every other record backwards.
	 * Go backward 3 because the read goes forward 1.
	 *
	 * This test depends on the read every other record forwards test
	 * succeeding.  Therefore only perform the test if the previous
	 * passed.
	 */
	if (debug)
		Print("read every other record backwards\n");
	if (subtest_failure == 0) {
	    for (counter = 0 ; counter < ((def_pos_record/2)-1); counter++) {
		if (Rmove_record(fd,BACKWARD,3,&position,READ)) {
			Print("Terminating subtest after record movement failure.\n");
			subtest_failure = 1;
			break;
		}
	    }
	}
	else {
	    Print("Skipping the read every other record backwards test due\n");
	    Print("to errors in previous subtest.\n");
	}
	/*
	 * Split the tape up into quarters and issue reads.
	 */
	do_tape_op(fd,MTREW,1);
	position = 0;
	subtest_failure = 0;
	quarter = def_pos_record / 4;
	if (quarter > 0) {
		if (debug)
			Print("Split tape up into quarters\n");
		/*
	 	 * Goto the 3/4 point. Backup 1 after the read to be at 3/4.
	 	 */
		if (debug)
			Print("Go to the 3/4 point and verify\n");
		if (Rmove_record(fd,FORWARD,quarter*3,&position,READ)) {
			Print("Terminating subtest after record movement failure.\n");
			Print("Test is to verify at 3/4 point.\n");
			goto sub_bail1;
		}
		move_record(fd,BACKWARD,1,&position,NO_READ);
		/*
	 	 * Goto the 1/4 point. Backup 1 after the read to be at 1/4.
	 	 */
		if (debug)
			Print("Go to the 1/4 point and verify\n");
		if (Rmove_record(fd,BACKWARD,quarter*2,&position,READ)) {
			Print("Terminating subtest after record movement failure.\n");
			Print("Test is to verify at 1/4 point.\n");
			goto sub_bail1;
		}
		move_record(fd,BACKWARD,1,&position,NO_READ);
		/*
	 	 * Goto the 1/2 point. Backup 1 after the read to be at 1/2.
	 	 */
		if (debug)
			Print("Go to the 1/2 point and verify\n");
		if (Rmove_record(fd,FORWARD,quarter,&position,READ)) {
			Print("Terminating subtest after record movement failure.\n");
			Print("Test is to verify at 1/2 point.\n");
			goto sub_bail1;
		}
		move_record(fd,BACKWARD,1,&position,NO_READ);
	}
	flush_output();
sub_bail1:
	/*
	 * Random positioning test.
	 * Generate a random number in the range of the number of records.
	 * Move to this new position and verify record.
	 */
	if (debug)
		Print("Performing random record movement tests\n");
	do_tape_op(fd,MTREW,1);
	position = 0;
	subtest_failure = 0;
	for (counter = 0 ; counter < def_num_random; counter++) {
		/*
		 * Generate a random record number to move to.
		 */
		rand_position = random() % def_pos_record;
		/*
		 * Determine direction and move.
		 */
		if (position < rand_position) {
			num_move = rand_position - position;
			if (debug > 5)
				Print3("random: position = %d, random position = %d, move %d forward\n",
					position, rand_position, num_move);
			if (Rmove_record(fd,FORWARD,num_move,&position,READ)) {
				Print("Position failure in random position test.\n");
				Print("Perform position recovery.\n");
				if (Rmove_record(fd,FORWARD,rand_position,
						&position,READ)) {
					Print("Recovery failure.\n");
					Print("Aborting subtest.\n");
					goto sub_bail2;
				}
			}
		}
		else if (position > rand_position) {
			num_move = position - rand_position;
			if (debug > 5)
				Print3("random: position = %d, random position = %d, move %d backward\n",
					position, rand_position, num_move);
			if (Rmove_record(fd,BACKWARD,num_move,&position,READ)) {
				Print("Position failure in random position test.\n");
				Print("Perform position recovery.\n");
				if (Rmove_record(fd,FORWARD,rand_position,
						&position,READ)) {
					Print("Recovery failure.\n");
					Print("Aborting subtest.\n");
					goto sub_bail2;
				}
			}
		}
	}
sub_bail2:
	
	/*
	 * Even if the line limit is exceeded, still print out the results.
	 */
	lines_output = 0;
	if (pos_range_errs || pos_move_errs || pos_read_errs || pos_validate_errs) {
		Print("Position test failure:\n");
		if (pos_range_errs)
			Print1("ERROR: Record position range errors = %d\n",pos_range_errs);
		if (pos_move_errs)
			Print1("FAILURE: %d non-successful ioctl position commands occured.\n",pos_move_errs);
		if (pos_read_errs)
			Print1("FAILURE: %d non-successful reads occured.\n",pos_read_errs);
		if (pos_validate_errs)
			Print1("FAILURE: %d validation errors occured\n",pos_validate_errs);
	}
	else {
		Print("SUCCESS: Position tests completed successfully\n\n");
	}
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);
	/*
	 * Restore orig value.
	 */
	flag_e = saveflag_e;

}

/*
 * Front end routine to the move record command.  The intent is to recover
 * position after a move_record fails.  Recovery consists of rewind to BOT
 * and setting the position to be zero.
 */
Rmove_record(Rfd,Rdirection,Rcount,Rposition,Rdoread)
	int Rfd;
	int Rdirection;
	int Rcount;
	int *Rposition;
	int Rdoread;
{
	if (move_record(Rfd,Rdirection,Rcount,Rposition,Rdoread)) {
		Print("Performing record position recovery after failure\n");
		Print("of a record movement command.\n");
		Print1("Assumed position at time of failure is %d\n",*Rposition);
        	/*
        	 * Rewind to recover position.
         	 */
		recover_position(Rfd,Rposition);
		return(1);
	}
	return(0);
}
/*
 * Position recovery consists of rewind to BOT
 * and setting the position to be zero.
 */
recover_position(fd,position)
	int fd;
	int *position;
{
       	if (do_tape_op(fd,MTREW,1)) {
		Print("FATAL ERROR: rewind failed during position recovery.\n");
		Print("Exerciser terminating.\n");
		do_errlog(LOG_REWFAIL);
		exit(1);
	}
	*position = 0;
}
/*
 * move forward or backward the specified number of records.
 * Read in present record and validate.
 */
move_record(fd,direction,count,position,doread)
	int fd;
	int direction;
	int count;
	int *position;
	int doread;
{

	if (debug > 5)
		Print1("move_record: initial position is %d\n",*position);
	/*
	 * Initial sanity check to insure that the original position is
	 * reasonable.
	 */
	if (*position < 0) {
		Print1("Exerciser error: move_record, position = %d\n",
			*position);
		return(1);
	}
	
	if (direction == FORWARD) {
		if ((*position + count) >= def_pos_record) {
			Print2("move_record, position out of range, %d > %d\n",
					(*position + count), def_pos_record);
			pos_range_errs++;
			return(1);
		}
		/*
		 * The random tests could cause us to move nowhere (count=0).
	 	 */
		if (count > 0) {
			if (debug > 5)
				Print1("move forward %d records\n",count);
			if (do_tape_op(fd,MTFSR,count)) {
				pos_move_errs++;
				return(1);
			}
			*position += count;
		}
	}
	else if (direction == BACKWARD) {
		if ((*position - count) < TEST_BOT) {
			Print2("move_record, position out of range, %d < %d\n",
					(*position + count), TEST_BOT);
			pos_range_errs++;
			return(1);
		}
		/*
		 * The random tests could cause us to move nowhere (count=0).
	 	 */
		if (count > 0) {
			if (debug > 5)
				Print1("move backward %d records\n",count);
			if (do_tape_op(fd,MTBSR,count)) {
				pos_move_errs++;
				return(1);
			}
			*position -= count;
		}
	}
	else {
		Print("Invalid direction\n");
		return(1);
	}
	if (debug > 5)
		Print1("move_record: new position is %d\n",*position);
	
	/*
	 * The position has now changed to the desired spot.
	 * Read in a record and validate.
	 */
	if (doread == NO_READ) {
		if (debug > 5)
			Print("Skip read in move_record\n");
		goto skipread;
	}
	if (!(read_record(fd,pos_rec_size,*position,VALIDATE,
	     &pos_read_errs, &pos_validate_errs,NO_INIT))){
		*position = *position +1;
		if (debug > 2) {
			Print1("Read a full record at %d \n",*position);
			flush_output();
		}
	}
skipread:
	if (debug > 5) {
		Print1("Position after read is %d\n",*position);
		if (pos_read_errs)
			Print1("pos_read_errs = %d\n",pos_read_errs);
		if (pos_validate_errs)
			Print1("pos_validate_errs = %d\n",pos_validate_errs);
	}
	return(0);
}
/*
 * Print out a help message.
 */
help_message()
{
Print("\n\ntapex - tape exerciser program\n\n");
Print("This program is intended to test various aspects of tape driver functionality.\n");
Print("These tests provide more comprehensive functional coverage than the\n");
Print("mtx(8) utility which does simple start/stop oriented read/write testing.\n");
Print("Testing typically involves writing records onto a tape and verifying\n");
Print("that what is written equals what is read.  Other functional areas\n");
Print("which are tested include:\n\n");
Print("\t* Using records in a range of sizes.\n");
Print("\t* Writing and reading past the end of media.\n");
Print("\t* End of file testing.\n");
Print("\t* Positioning tests, where record and file movement is tested.\n");
Print("\t* n-buffered I/O testing. \n");
Print("\t* Record length testing.\n");
Print("\t* Random record size testing.\n");
Print("\t* Bandwidth performance analysis.\n");
Print("\t* Tape transportability testing.\n");
Print("\t* Reporting of tape contents.\n");
Print("\t* Media loader testing.\n");
Print("\nTape testing involves writing onto a tape.  For this reason when\n");
Print("the exerciser is run a writable tape should be loaded in the drive\n");
Print("being tested, the drive must be online.\n");
Print("\nThere are a number of options that tapex accepts.  These\n");
Print("options control which tests are performed and other parameters\n");
Print("\nThe following is a description of the available options:\n");
Print("\n");
Print("-a\tPerformance test.  Times how long reads and writes take and\n");
Print("\tdivide by the number of bytes transferred to calculate the bandwidth\n");
Print("\tin kilobytes per second.\n");
Print("\n");
Print("\n-b\tContinuously run the write / read tests until process killed.\n");
Print("\tThis flag can be used in conjunction with the -r or -g switch.\n");
Print("\n");
Print("-c\tEnables caching on the device where supported.  This does not\n");
Print("\tspecifically test caching, but it enables the use of caching on\n");
Print("\ta tape device while running the other tests.\n");
Print("\n");
Print("-C\tDisables caching on TMSCP tape devices.  If the tape device is\n");
Print("\ta TMSCP unit then caching is the default mode of test operation.\n");
Print("\tThis flag will cause the tests to be run in non-caching mode.\n");
Print("\n");
Print("-d\tThis tests the ability to append to the media.  The first step\n");
Print("\tis to write records to the tape.  Next a reposition back one\n");
Print("\trecord is done, then additional records are appended.  Finally a\n");
Print("\tread verification phase is done.  This test simulates the behavior\n");
Print("\tof the tar \"r\" switch.\n");
Print("\n");
Print("-e\tWrite/read past end of media tests.  This test first writes\n");
Print("\tdata to fill up a tape; which may take a long time for long tapes;\n");
Print("\tthen reads and writes are done which should fail.  Next writing\n");
Print("\tpast end of media is enabled and writes are performed and read\n");
Print("\tback for validation.\n");
Print("\n");
Print("-E\tRun a series of tests.  Certian tests will be run in sequential\n");
Print("\torder.  Due to the large number of tests, this\n");
Print("\toption takes a long time to complete.  Depending on tape type and\n");
Print("\tCPU type, this series of tests can take up to 10 hours to complete.\n");
Print("\n");
Print("-f /dev/rmt#?\t Used to specify the device special file.\n");
Print("\tThe \"#\" symbol represent the unit number and \"?\" could be the\n");
Print("\tletter \"h\" for the high density device or \"l\" for the low\n");
Print("\tdensity device.  For example /dev/rmt4h or /dev/rmt1l.\n");
Print1("\tThe default tape device is %s.\n",DEFAULT_TAPE);
Print("\n");
Print("-F\tFile positioning tests.  First files are written to the tape and\n");
Print("\tverified.  Next every other file on the tape is read, then the\n");
Print("\tother file are read by traversing the tape backwards.  Finally\n");
Print("\trandom numbers are generated and the tape is positioned to that\n");
Print("\tlocation and the data is verified.  Each file uses a\n");
Print("\tdifferent record size.\n");
Print("\n");
Print("-G\tRuns the file position tests on alredy written tape.  This flag\n");
Print("\tcan be used in conjunction with the -F flag to run the file position\n");
Print("\ttests on a tape which has already been written to by a previous\n");
Print("\tof the -F test.  In order for this to work, the same test parameters\n");
Print("\tsuch as record size and number of files must be used as when the\n");
Print("\tthe tape was written.  No other data should have been written to\n");
Print("\tthe tape after the previous -F test.\n");
Print("\n");
Print("-g\tRandom record size tests.  Writes records of random sizes.\n");
Print("\tReads in the tape specifying a large read size, however only\n");
Print("\tthe amount of data in the randomly sized record should be returned.\n");
Print("\tThis test only checks return values and does not validate record\n");
Print("\tcontents.\n");
Print("\n");
Print("-h\tDisplays this help message.\n");
Print("\n");
Print("-i\tInteractive mode.\tUnder this mode the user will be prompted\n");
Print("\tfor various test parameters.  Typical parameters include the record\n");
Print("\tsize and the number of records to write.\n");
Print("\tThe following scaling factors are allowed:\n");
Print("\t\tk or K\tkilobyte (1024 * n)\n");
Print("\t\tb or B\tblock (512 * n)\n");
Print("\t\tm or M\tmegabyte (1024 * 1024 * n)\n");
Print("\tFor example 10k would specify 10240 bytes.\n");
Print("\n");
Print("-j\tWrite phase of the tape transportability tests.  This test writes\n");
Print("\ta number of files to the tape then verifies the tape.  After\n");
Print("\ta successful verification the tape is brought offline to be\n");
Print("\tmoved to another tape unit to be read in with the -k option.\n");
Print("\tThe purpose of this test is to prove that a tape can be written\n");
Print("\ton one drive and read in by another drive.  Note that the test\n");
Print("\tparameters for the -k phase of the transportability test must\n");
Print("\tmatch the parameters of the -j test.  Any changes of test parameters\n");
Print("\tfrom the defaults should also be changed during the -k test.\n");
Print("\n");
Print("-k\tRead phase of the tape transportability tests.  This test reads\n");
Print("\ta tape which was written by the -j test and verifies that the expected\n");
Print("\tdata is read from the tape.  Success of this test proves that a tape\n");
Print("\tcan be written by one drive and read in by another.  Note: as\n");
Print("\tstated in the the description of the -j option, any parameters\n");
Print("\tchanged in the -j test must also be changed in the -k test.\n");
Print("\n");
Print("-L\tMedia loader testing.\n");
Print("\tThis test covers various aspects of media loaders.  For sequential\n");
Print("\tstack loaders, the media is loaded, written to and verified.  Next\n");
Print("\tthe media is unloaded and the test repeats on the next piece of\n");
Print("\tmedia.  This verifies that all the media in the input deck is\n");
Print("\twritable.  To run this test in read-only mode, also specify the\n");
Print("\t-w option.\n");
Print("\n");
Print("-l\tEnd of file testing.\n");
Print("\tThis test is aimed at verifying that a zero byte count is\n");
Print("\treturned when a tape mark is read, but another read will\n");
Print("\tfetch the first record of the next tape file.\n");
Print("\n");
Print("-m\tDisplay tape contents.\n");
Print("\tThis is not a test; rather it reads the tape to see what is on\n");
Print("\tit.  The tape is sequentially read to print out the number of\n");
Print("\tfiles, the number of records in each file and the size of the\n");
Print("\trecords within the file.  The contents of the tape records are\n");
Print("\tnot examined.\n");
Print("\n");
Print("-N\tDisables the usage of n-buffered I/O on tests which support\n");
Print("\tits usage.\n\n");
Print("\n");
Print("-o filename\tSends output to the specified filename.  The default\n");
Print("\tis to not create an output file and send output to the terminal.\n");
Print("\n");
Print("-p\tRuns both the record and file positioning tests.\n");
Print("\n");
Print("-q\tCommand timeout test.  This test verifies that the driver\n");
Print("\tallows enough time for completion of long operations.  The test\n");
Print("\tconsists of writing files to fill up the tape.  Next a rewind is\n");
Print("\tperformed followed by a forward skip out to the last file.  The\n");
Print("\ttest is successful if the forward skip operation completes without\n");
Print("\terror.\n");
Print("\n");
Print("-r\tWrite/read testing for a range of record sizes.\n");
Print("\tA number of records will be written to the tape and then verified.\n");
Print("\tThis process is repeated over a range of record sizes.\n");
Print("\n");
Print("-R\tRecord positioning tests.  First records are written to the tape and\n");
Print("\tverified.  Next every other record on the tape is read, then the\n");
Print("\tother records are read by traversing the tape backwards.  Finally\n");
Print("\trandom numbers are generated and the tape is positioned to that\n");
Print("\tlocation and the data is verified.\n");
Print("\n");
Print("-s\tRecord size testing.  Verifies that a record read will return\n");
Print("\tat most one record or the read size, whichever is less.\n");
Print("\n");
Print("-S\tWrite/read testing for a single record size.\n");
Print("\n");
Print("-T \tCopies output to standard output.  This flag is useful if\n");
Print("\tyou wanted to log output to a file with the -o option and also\n");
Print("\thave the output displayed on standard output.  This flag must be\n");
Print("\tspecified after the -o flag in the command line.\n");
Print("\n");
Print("-v\tVerbose mode.  This flag will cause more detailed terminal output of\n");
Print("\twhat the tape exerciser is doing.  Example output includes listing\n");
Print("\twhat operations the exerciser is performing such as record counts\n");
Print("\tand more detailed error information.\n");
Print("\n");
Print("-V\tVery verbose mode.  This flag will cause more output to be\n");
Print("\tgenerated than either the default mode or the -v flag.  The\n");
Print("\toutput consists of additional status information on exerciser\n");
Print("\toperation.\n");
Print("\n");
Print("-w\tOpen the tape read-only.  This mode is only useful for tests\n");
Print("\twhich do not write to the media.  For example it allows the -m test\n");
Print("\tto be run on a write-protected media.\n");
Print("\n");
Print1("-Z\tInitialize read buffer to the nonzero value 0%o.\n",ALT_BUF_INIT);
Print("\tThis may be useful for debuging purposes.  If the -Z flag is not\n");
Print1("\tspecified, all elements of the read buffer will be initialized to %o.\n",BUFFER_INIT);
Print("\tMany of the tests first initialize their read buffer and then\n");
Print("\tperform the read operation.  After reading a record from the tape\n");
Print("\tsome tests validate that the unused portions of the read buffer\n");
Print("\tremain at the value to which they were initialized.  As a debugging\n");
Print("\ttool, it may in some cases be useful to have this initialize value\n");
Print1("\tto be nonzero.  The arbitrary character 0%o can be used.\n",ALT_BUF_INIT);

Print("\n");
Print("CHANGING TEST PARAMETERS\n");
Print("\nIt is possible to change the default test parameters in two ways.\n");
Print("The first way is with the -i option as described above.  It is also\n");
Print("possible to directly specify the desired parameters in the command\n");
Print("line itself.  Described below are the parameters which are settable\n");
Print("in the command line; listed with the associated test.\n");
Print("To specify a value, type the parameter name followed by a space\n");
Print("and then the number.  For example \"-min_rs 512\".\n");
Print("\tThe following scaling factors are allowed:\n");
Print("\t\tk or K\tkilobyte (1024 * n)\n");
Print("\t\tb or B\tblock (512 * n)\n");
Print("\t\tm or M\tmegabyte (1024 * 1024 * n)\n");
Print("\tFor example 10k would specify 10240 bytes.\n");
Print("\nAs an example, -min_rs 10K, causes a minimum record size of 10240\n");
Print("\nbytes.");
Print("\nWrite / Read verification test parameters\n");
Print("\n\tThese parameters are associated with the following options: -r, -S\n");
Print("\t-min_rs \tminimum record size\n");
Print("\t-max_rs \tmaximum record size\n");
Print("\t-num_rec\tnumber of records\n");
Print("\t-inc    \trecord increment factor\n");
Print("\nEnd of media test parameters\n\n");
Print("\tNote: Specifying too much data to be written past EOM could cause\n")
Print("\ta reel-to-reel tape to go off the end.\n")
Print("\n\tThese parameters are associated with the following option: -e\n");
Print("\t-end_rs \trecord size\n");
Print("\t-end_num\tnumber or records written past EOM\n");
Print("\nRecord and file positioning test parameters\n");
Print("\n\tThese parameters are associated with the following options: -R, -F\n");
Print("\t-pos_rs \trecord size\n");
Print("\t-pos_num\tnumber of records - record position test\n");
Print("\t-pos_ra \tnumber of random repositions\n");
Print("\t-rec_fi \tnumber of records per file - file position test\n");
Print("\t-num_fi \tnumber of files - file position test\n");
Print("\nn-buffered I\\O usage\n");
Print("\n\tThis parameter is used in any test which supports n-buffered I\\O\n");
Print("\t-num_nbuf\tnumber of buffers to use\n");
Print("\nTime duration of the read \\ write verification test\n");
Print("\n\tThis parameter is associated with the following options: -r, -S\n");
Print("\n\t-t\t\tRun time in minutes.  The default is to run the\n");
Print("\t\t\tread \\ write tests to completion.\n");
Print("\nRecord size testing parameters\n");
Print("\n\tThese parameters are associated with the following option: -s\n");
Print("\t-num_rec\tnumber of records\n");
Print("\t-size_rec\trecord size\n");
Print("\nRandom record size parameters\n");
Print("\n\tThis parameter is associated with the following option: -g\n");
Print("\t-rand_num\tnumber of records to write and read.\n");
Print("\nNumber of allowed error messages per subtest.\n");
Print("\n\tThis parameter is associated with all tests.\n");
Print("\t-err_lines\tthreshold on error printouts.\n");
Print("\nPerformance test parameters\n");
Print("\n\tThese parameters are associated with the following option: -a\n");
Print("\t-perf_num\tthe number of records to write and read\n");
Print("\t-perf_rs\tthe size of records\n");
Print("\nAppend to media test parameters\n");
Print("\n\tThese parameters are associated with the following option: -d\n");
Print("\t-tar_num\tThe number of additional and appended records.\n");
Print("\t-tar_size\tThe record size for all records written in this test.\n");
Print("\nTransportability test parameters\n");
Print("\n\tThese parameters are associated with the following options: -j, -k\n");
Print("\t-tran_file\t the number of files to write or read.\n");
Print("\t-tran_rec\t the number of records contained in each file.\n");
Print("\t-tran_rs\t the size of each record.\n");
Print("\n");
Print("EXAMPLES\n");
Print("\n");
Print("The following are examples command lines to run tapex followed by a\n");
Print("brief description.\n");
Print("\n");
Print("tapex -f /dev/rmt1h -E -o tapex.out\n");
Print("\tRuns a series of tests on tape device rmt1h and places all output\n");
Print("\tin a log file called tapex.out\n");
Print("\n");
Print("tapex -f /dev/rmt4h -v -e\n");
Print("\tRuns the end of media test on tape device rmt4h.  Verbose mode is\n");
Print("\tspecified which will cause additional output.  No log file will be\n");
Print("\tcreated.  Output will be directed to the terminal.\n");
Print("\n");
Print("tapex -r\n");
Print("\tPerforms read/write record testing on the default tape device rmt0h.\n");
Print("\tNo log file will be created.  Output will be sent to the terminal.\n");
Print("\n");
Print("tapex -r -min_rs 10k -max_rs 20k\n");
Print("\tPerforms read/write record testing using record sizes in the range\n");
Print("\t10k to a maximum record size of 20k.  This test will be run on\n");
Print("\tthe default tape device /dev/rmt0h\n");
Print("\tNo log file will be created.  Output will be sent to the terminal.\n");
Print("\n");
Print("\n");
flush_output();
}

/*
 * write_record
 *
 * Write a record onto the tape.  The record contents are sequential numbers
 * starting at one plus the initial seed.
 * Returns the number of write errors.
 */
write_record(fd, record_size, seed)
	int fd;
	int record_size;
	int seed;
{
	int write_errors = 0;
	rw_struct read_struct, write_struct;
	int return_value;
	/*
 	 * Format the transmit buffer with a pattern.
	 * Check for errors in the write operation.
	 * 
	 * If the performance test is being done then it doesn't matter what
	 * the buffer contents are so there's no need to format it.
 	 */
	if (perf_test_ip == 0) {
		format_writebuf(&write_struct, &read_struct, record_size, seed,WRITING);
	}
	return_value = write(fd, write_struct.buffer, record_size); 
	if (return_value != record_size) {
		if (debug > 10) {
			Print1("write error, errno = %d\n",errno);
		}
		write_errors++;
		/*
		 * See if the tape went offline.  If so end the tests.
		 */
		if ((tape_file(fd,0)) == 0)
			sig_handler();
		/*
		 * Stop test if end of media encountered.
		 */
		if ((tape_eot(fd)) && (flag_e == 0)) {
			Print("End of media encountered.\n");
			sig_handler();
		}
	}
	return(write_errors);
}

/*
 * read_record
 *
 * Reads in a record off the tape and optionally validates the contents of
 * what was read.
 */
read_record(fd,record_size,seed,do_validate,read_er,valid_er,init_structs)
	int fd;
	int record_size;
	int seed;
	int do_validate;
	int *read_er;
	int *valid_er;
	int init_structs;
{
	int cntr;
        rw_struct read_struct, write_struct;
        int return_value;
	int errors = 0;
	int validate_length;
	int resid = 0;
	int format_info;

	/*
 	 * Format the transmit buffer with a pattern so that you have something
	 * to compare the read data to.
 	 */
	if (init_structs == DO_INIT) {
		format_info = SLOW;
	}
	else {
		format_info = READING;
	}
	/*
	 * No need to init buffers on performance test because the data is
	 * never compared.
	 */
	if (perf_test_ip == 0) {
	    format_writebuf(&write_struct, &read_struct, record_size,seed,format_info);

	    /*
	     * Paranoia check to see that the contents of the read buffer is
	     * zero.
	     */
	    if (format_info == SLOW) {
	    	check_read_buffer(&read_struct);
	    }
	}

	/*
	 * Do the actual read.
	 */
	return_value = read(fd, read_struct.buffer, record_size); 
	if (return_value == 0) {
		if (flag_z == 0) {
			*read_er = *read_er + 1;
			errors++;
		}
		if (debug)
			Print("read EOF encountered \n");
	}
	/*
	 * A full record should always be read except for the case where
	 * record size testing is being performed.
	 * Read errors are expected for pending reads during the EOM test.
	 */
	else if ((return_value != record_size) && (flag_s == 0)) {
		*read_er = *read_er + 1;
		errors++;
		if ((debug > 2) && (eom_test_ip == 0)) {
			Print1("read failed, return value = %d\n", return_value);
			if (debug > 10)
				Print1("read error, errno = %d\n",errno);
		}
		/*
		 * See if the tape went offline.  
		 */
		if ((tape_file(fd,0)) == 0)
			sig_handler();
		/*
		 * Stop test if end of media encountered.
		 */
		if ((tape_eot(fd)) && (eom_test_ip == 0))
			sig_handler();
	}
	if (do_validate == NO_VALIDATE)
		return(errors);
	/*
 	 * Verify what was read = what was written.
	 * Look at each byte to see that it is the expected value.
	 *
	 * If record size tests are being run, partial records may be read.
 	 */

	if (flag_s == 0)
		validate_length = record_size;
	else {
		/*
	 	 * At most one record should be read.  Anything more is an error
		 */
		if (return_value > def_size_record) {
			Print2("Record size error: record size = %d bytes, read returned %d bytes\n",
				def_size_record, return_value);
			*read_er = *read_er + 1;
		}
		/*
		 * Read should not return more than the requested bytes.
		 */
		else if (return_value > record_size) {
			Print2("Record size error: Read requested = %d, read returned %d\n",
				return_value, return_value);
			*read_er = *read_er + 1;
		}
		/*
		 * We requested to read more than the size of a record.  The
		 * read should return the number of bytes actually read.
		 */
		else if (return_value < record_size) {
			if (debug > 10) {
				Print2("size test: requested %d, returned %d\n",
					record_size,return_value)
			}
			if (return_value > 0) {
				resid = record_size - return_value;
				if (debug > 10)
					Print1("size test: resid = %d\n",resid)
			}
		}
		validate_length = return_value;
	}

	/*
	 * For record positioning info it is important to know when a tape
	 * mark has been encountered.  If some chars were read, then return
	 * the first character so that a check can be made to see which record
	 * it is from.
	 */
	if (flag_z) {
		validate_length = return_value;
		if (return_value == -1) 	/* read error */
			pass_back = -1;
		else if (return_value > 0) {	/* successful read */
			pass_back = read_struct.buffer[0];
			if ((pass_back == -1) || (pass_back == 0))
				Print("read_record pass_back conflict.\n");
		}
		else if (return_value == 0) {
			pass_back = 0;		/* EOF, tape mark */
		}
		else if (return_value < 0)
			pass_back = -1;		/* error */
	}
       	for (cntr = 0; cntr < validate_length; cntr++) {
		if (write_struct.buffer[cntr] !=
	    	     read_struct.buffer[cntr]) {
			*valid_er = *valid_er + 1;
			errors++;
			if (debug > 1) {
				EPrint3("ERROR: record size %d,record #%d, write[%d] = ",record_size,seed,cntr);
				EPrint3("%d, read[%d] = %d\n",write_struct.buffer[cntr], cntr,read_struct.buffer[cntr]);
			}
		}
		else if (debug > 100) {
				Print3("record size %d,record #%d, write[%d] = ",record_size,seed,cntr);
				Print3("%d, read[%d] = %d\n",write_struct.buffer[cntr], cntr,read_struct.buffer[cntr]);
		}
       	}
	/*
	 * Durnig the record size tests a read was done for more than the
	 * size of one record.  Validate the the remaining residual bytes in
	 * the read buffer are zero.
	 */
	if ((flag_s) && (resid)) {
	       	for (cntr = validate_length; cntr < validate_length+resid; cntr++) {
		    	if (read_struct.buffer[cntr] != def_init_char) {
				*valid_er = *valid_er + 1;
				errors++;
				if (debug) {
					EPrint3("ERROR: record size %d,record #%d, write[%d] = ",record_size,seed,cntr);
					EPrint3("%d, read[%d] = %d\n",0,
cntr, read_struct.buffer[cntr]);
				}
			}
			else if (debug > 100) {
				Print3("record size %d,record #%d, write[%d] = ",record_size,seed,cntr);
				Print3("%d, read[%d] = %d\n",0,cntr,
read_struct.buffer[cntr]);
			}
	       	}
	}
	return(errors);
}

/*
 *	The option expected another argument, which wasn't there.
 */
need_word(string)
	char *string;
{
	Print1("%s option needs additional argument.\n",string);
	do_errlog(LOG_COMMAND);
	exit(1);
}

/*
 *	Take the value of a "bs" or "nbufs" string and convert it
 *	into a useful number.  If the string is postfixed with a
 *	character scale the value.
 *
 *		k or K - Kilobyte
 *		b or B - Block (512 bytes)
 *		s or S - Sector (512 bytes)
 *		m or M - Megabyte (1024 * 1024 = 1048576 bytes)
 *
 */
convert_value(s)
char	*s ;
{
	int	length, factor ;
	char	*p ;

	if((length = strlen(s)) == 0 )
		return(0);

	p = s + length - 1;

	if( isdigit(*p))			/* return value */
		return(atoi(s));
	else if( *p == 'k' || *p == 'K' )	/* kilobyte */
		factor = 1024;
	else if( *p == 'b' || *p == 'B' )	/* "block" */
		factor = 512;
	else if( *p == 's' || *p == 'S' )	/* sector */
		factor = 512;
	else if( *p == 'm' || *p == 'M' )	/* Megabyte */
		factor = 1024 * 1024;
	else
		return(atoi(s));			/* return value */

	*p = '\0' ;

	return(atoi(s) * factor);
}

/*
 *	Parse the command line.  
 *	Set flags which dictate the test to be performed.
 *	Set parameters such as sizes and counts.
 */
parse(argc, argv)
	int	argc ;
	char	*argv[] ;
{
	int	i ;
        char *devptr;
        char *fileptr;
        int reclen;
        int maxrec;
	char *charptr;



	for(i = 1; i < argc; i++) {
		if( strcmp(argv[i], "-h") == 0 ) {
                        /* print help message and exit */
                        help_message();
                        exit(0);
		}
		else if( strcmp(argv[i], "-a") == 0 ) {
                        /* Performance analysis */
                        flag_a++;
		}
		else if( strcmp(argv[i], "-i") == 0 ) {
                        /* Interactive mode - specify test parameters */
                        flag_i++;
		}
		else if( strcmp(argv[i], "-g") == 0 ) {
                        /* Random record size write / read no validate test */
                        flag_g++;
		}
		else if( strcmp(argv[i], "-G") == 0 ) {
                        /* Run file position tests on already written tape */
                        flag_G++;
                        flag_F++;
		}
		else if( strcmp(argv[i], "-Z") == 0 ) {
                        /* Init read buf to a nonzero character. */
                        flag_Z++;
		}
		else if( strcmp(argv[i], "-r") == 0 ) {
                        /* read + write testing */
                        flag_r++;
		}
		else if( strcmp(argv[i], "-S") == 0 ) {
                        /* read + write testing, single block size
			 * instead of a range of records.
			 */
                        flag_S++;
			flag_r++;
		}
		else if( strcmp(argv[i], "-s") == 0 ) {
                        /* Record size testing */
                        flag_s++;
		}
		else if( strcmp(argv[i], "-v") == 0 ) {
                        /* Lower level debug , up the debug level by
                         * putting in more than one -v flag.
                         */
                        flag_v++;

		}
		else if( strcmp(argv[i], "-V") == 0 ) {
                        /* Higher level debug */
                        flag_V++;
		}
		else if( strcmp(argv[i], "-w") == 0 ) {
                        /* Open read only */
                        flag_w++;
		}
		else if( strcmp(argv[i], "-e") == 0 ) {
                        /* Write past EOM test */
                        flag_e++;
		}
		else if( strcmp(argv[i], "-c") == 0 ) {
                        /* Enable cache */
                        flag_c++;
		}
		else if( strcmp(argv[i], "-C") == 0 ) {
                        /* Disable caching by default on tmscp tapes */
                        flag_C++;
		}
		else if( strcmp(argv[i], "-d") == 0 ) {
                        /* media append test */
                        flag_d++;
		}
		else if( strcmp(argv[i], "-n") == 0 ) {
                        /* n-buffered I/O */
                        flag_n++;
		}
		else if( strcmp(argv[i], "-N") == 0 ) {
                        /* Disable n-buffered I/O */
                        flag_n = 0;
		}
		else if( strcmp(argv[i], "-p") == 0 ) {
                        /* positioning tests both files and records */
                        flag_R++;
                        flag_F++;
		}
		else if( strcmp(argv[i], "-F") == 0 ) {
                        /* positioning tests files */
                        flag_F++;
		}
		else if( strcmp(argv[i], "-R") == 0 ) {
                        /* positioning tests records */
                        flag_R++;
		}
		else if( strcmp(argv[i], "-z") == 0 ) {
                        /* positioning records - informational */
                        flag_z++;
		}
		else if( strcmp(argv[i], "-j") == 0 ) {
                        /* transportability test - write phase */
                        flag_j++;
		}
		else if( strcmp(argv[i], "-k") == 0 ) {
                        /* transportability test - read phase */
                        flag_k++;
		}
		else if( strcmp(argv[i], "-l") == 0 ) {
                        /* End of file tape mark testing. */
                        flag_l++;
		}
		else if( strcmp(argv[i], "-L") == 0 ) {
                        /* Media loader testing */
                        flag_L++;
		}
		else if( strcmp(argv[i], "-m") == 0 ) {
                        /* tape disection */
                        flag_m++;
		}
		else if( strcmp(argv[i], "-q") == 0 ) {
                        /* command timeout */
                        flag_q++;
		}
		else if( strcmp(argv[i], "-b") == 0 ) {
                        /* busy - continuous read write testing */
                        flag_b++; /* used in write-read test */
			flag_B++; /* used in random record size test */
		}
		else if( strcmp(argv[i], "-E") == 0 ) {
                        /* Run a set of tests */
			flag_E++;	/* All test flag */
			flag_n++;	/* n-buffered read write test */
			flag_r++;	/* read and write a range of records */
			flag_s++;	/* record size tests */
			flag_R++;	/* Position records */
			flag_F++;	/* Position files */
			flag_g++;	/* random record size write read */
			flag_a++;	/* performance analysis */
			flag_l++;	/* eof tape mark testing */
			flag_d++;	/* media append test */
			flag_e++;	/* end of media test */
			flag_q++;	/* command timeout test */
		}
		else if( strcmp(argv[i], "-f") == 0 ) {
                        /* Specify a tape file name */
			if( argc == ++i )
				need_word("-f") ;		/* exits */
			else
				tape = argv[i];
		}
		else if( strcmp(argv[i], "-o") == 0 ) {
                        /* Specify a log file name */
			if( argc == ++i )
				need_word("-o") ;		/* exits */
			else {
				setup_logfile(argv[i]);
			}
		}
		else if( strcmp(argv[i], "-T") == 0 ) {
			printstatus |= TTYPRINT;
		}
		/*
		 * Allow specification of sizes on the command line.
		 */
		/*
		 * These 4 are associated with the read/write for a
		 * range of record sizes test.
		 */
		else if( strcmp(argv[i], "-min_rs") == 0 ) {
			if( argc == ++i )
				need_word("-min_rs") ;		/* exits */
			else
				def_min_record = convert_value(argv[i]) ;
			bounds_check(def_min_record,MIN_RECORD_SIZE,max_record_size);
		}
		else if( strcmp(argv[i], "-max_rs") == 0 ) {
			if( argc == ++i )
				need_word("-max_rs") ;		/* exits */
			else
				def_max_record = convert_value(argv[i]) ;
			bounds_check(def_max_record,def_min_record,max_record_size);
		}
		else if( strcmp(argv[i], "-num_rec") == 0 ) {
			if( argc == ++i )
				need_word("-num_rec") ;		/* exits */
			else
				def_num_record = convert_value(argv[i]) ;
			bounds_check(def_num_record,MIN_RECORDS,MAX_RECORDS);
		}
		else if( strcmp(argv[i], "-err_lines") == 0 ) {
			if( argc == ++i )
				need_word("-err_lines") ;	/* exits */
			else
				def_err_lines = convert_value(argv[i]) ;
			bounds_check(def_err_lines,MIN_ERR_LINES,MAX_ERR_LINES);
		}
		else if( strcmp(argv[i], "-inc") == 0 ) {
			if( argc == ++i )
				need_word("-inc") ;		/* exits */
			else
				def_inc_record = convert_value(argv[i]) ;
			bounds_check(def_inc_record,MIN_INCREMENT,def_max_record - def_min_record);
		}
		/*
		 * These values are used in the writing past media end
		 * tests.
		 */
		else if( strcmp(argv[i], "-end_rs") == 0 ) {
			if( argc == ++i )
				need_word("-end_rs") ;		/* exits */
			else
				def_eof_record = convert_value(argv[i]) ;
			bounds_check(def_eof_record,MIN_RECORD_SIZE,max_record_size);
		}
		else if( strcmp(argv[i], "-end_num") == 0 ) {
			if( argc == ++i )
				need_word("-end_num") ;		/* exits */
			else
				def_eof_numrec = convert_value(argv[i]) ;
			bounds_check(def_eof_numrec,EOT_MIN_RECORDS,EOT_MAX_RECORDS);
		}
		/*
		 * Values used in the positioning tests.
		 */
		else if( strcmp(argv[i], "-pos_num") == 0 ) {
			if( argc == ++i )
				need_word("-pos_num") ;		/* exits */
			else
				def_pos_record = convert_value(argv[i]) ;
			bounds_check(def_pos_record,MIN_RECORDS,MAX_RECORDS);
		}
		else if( strcmp(argv[i], "-pos_rs") == 0 ) {
			if( argc == ++i )
				need_word("-pos_rs") ;		/* exits */
			else
				pos_rec_size = convert_value(argv[i]) ;
			bounds_check(pos_rec_size,MIN_RECORD_SIZE,max_record_size);
		}
		else if( strcmp(argv[i], "-pos_ra") == 0 ) {
			if( argc == ++i )
				need_word("-pos_ra") ;		/* exits */
			else
				def_num_random = convert_value(argv[i]) ;
			bounds_check(def_num_random,MIN_RANDOM,MAX_RANDOM);
		}
		else if( strcmp(argv[i], "-rec_fi") == 0 ) {
			if( argc == ++i )
				need_word("-rec_fi") ;		/* exits */
			else
				def_rec_file = convert_value(argv[i]) ;
			bounds_check(def_rec_file,MIN_REC_FILE,MAX_REC_FILE);
		}
		else if( strcmp(argv[i], "-num_fi") == 0 ) {
			if( argc == ++i )
				need_word("-num_fi") ;		/* exits */
			else
				def_num_files = convert_value(argv[i]) ;
			bounds_check(def_num_files,MIN_FILES,MAX_FILES);
		}
		/*
		 * Number of buffers to use in n-buffered I/O tests.
		 */
		else if( strcmp(argv[i], "-num_nbuf") == 0 ) {
			if( argc == ++i )
				need_word("-num_nbuf") ;	/* exits */
			else
				def_nbuf_buffers = convert_value(argv[i]) ;
			bounds_check(def_nbuf_buffers,MIN_NBUFFERS,MAX_NBUFFERS);
		}
		/*
		 * Number of minutes to run the read/write test
		 */
		else if( strcmp(argv[i], "-t") == 0 ) {
			if( argc == ++i )
				need_word("-t") ;	/* exits */
			else
				def_time_delta = convert_value(argv[i]) ;
			bounds_check(def_time_delta,MIN_TIME,MAX_TIME);
		}
		/*
	 	 * Parameters for the record size tests
		 */
		else if( strcmp(argv[i], "-size_rec") == 0 ) {
			if( argc == ++i )
				need_word("-size_rec") ;	/* exits */
			else
				def_size_record = convert_value(argv[i]) ;
			bounds_check(def_size_record,MIN_RECORD_SIZE,max_record_size);
		}
		/*
		 * Number of random records to write.
		 */
		else if( strcmp(argv[i], "-rand_num") == 0 ) {
			if( argc == ++i )
				need_word("-rand_num") ;	/* exits */
			else
				def_rand_tests = convert_value(argv[i]) ;
			bounds_check(def_rand_tests,1,DEF_MAX_RANDOM);
		}
		/*
		 * Performance test.  Number and size of records.
		 */
		else if( strcmp(argv[i], "-perf_num") == 0 ) {
			if( argc == ++i )
				need_word("-perf_num") ;	/* exits */
			else
				def_num_perf_rec = convert_value(argv[i]) ;
			bounds_check(def_num_perf_rec,MIN_RECORDS,MAX_RECORDS);
		}
		else if( strcmp(argv[i], "-perf_rs") == 0 ) {
			if( argc == ++i )
				need_word("-perf_rs") ;	/* exits */
			else
				def_perf_rec_size = convert_value(argv[i]) ;
			bounds_check(def_perf_rec_size,MIN_RECORD_SIZE,max_record_size);
		}
		/*
		 * Append to media test.  Number and size of records.
		 */
		else if( strcmp(argv[i], "-tar_num") == 0 ) {
			if( argc == ++i )
				need_word("-tar_num") ;	/* exits */
			else
				tar_base_recs = convert_value(argv[i]) ;
			bounds_check(tar_base_recs,MIN_RECORDS,MAX_RECORDS);
			tar_add_recs = tar_base_recs;
		}
		else if( strcmp(argv[i], "-tar_size") == 0 ) {
			if( argc == ++i )
				need_word("-tar_size") ;	/* exits */
			else
				tar_rec_size = convert_value(argv[i]) ;
			bounds_check(tar_rec_size,MIN_RECORD_SIZE,max_record_size);
		}
		/*
		 * Transportability test.
		 */
		else if( strcmp(argv[i], "-tran_file") == 0 ) {
			if( argc == ++i )
				need_word("-tran_file") ;	/* exits */
			else
				trans_num_files = convert_value(argv[i]) ;
			bounds_check(trans_num_files,MIN_FILES,MAX_FILES);
			Print("NOTE: the read and write phase must have identical parameters.\n");
		}
		else if( strcmp(argv[i], "-tran_rec") == 0 ) {
			if( argc == ++i )
				need_word("-tran_rec") ;	/* exits */
			else
				trans_rec_file = convert_value(argv[i]) ;
			bounds_check(trans_rec_file,MIN_RECORDS,MAX_RECORDS);
			Print("NOTE: the read and write phase must have identical parameters.\n");
		}
		else if( strcmp(argv[i], "-tran_rs") == 0 ) {
			if( argc == ++i )
				need_word("-tran_rs") ;	/* exits */
			else
				trans_record_size = convert_value(argv[i]) ;
			bounds_check(trans_record_size,MIN_RECORD_SIZE,max_record_size);
			Print("NOTE: the read and write phase must have identical parameters.\n");
		}
#ifdef 0
		/*
		 * Skip the mtx translation for now.
		 */
		/*
		 * Front end to map to old version of mtx. 
		 */
		else if (strncmp(argv[i], "-a", 2) == 0) {
			Print("-a option\n");
			/*
			 * This test is supposed to do short, long and
			 * variable record length tests.  Do the short
			 * and long record test with flag_r and a large
			 * record range;
			 */
			flag_g++;	/* variable record sizes */
			flag_r++;	/* read write tests */
			flag_n++;	/* use n-buffered IO */
			flag_b++;	/* cycle until killed */
			/*
			 * Setup the record size for a wide range.
			 */
			def_min_record = MTX_DEF_SHORT;
			def_max_record = MTX_DEF_LONG;
			/* retrieve and coordinate device path */
			devptr = device + DEVOFFS;
			charptr = argv[i];
		/*
			*charptr += 2;
			while (*devptr++ = *++*charptr)
				Print("*charptr = %c\n",*charptr);
	Print1("*charptr = %c\n",*charptr);
		 */
			tape = device;
			Print1("device = %s\n",tape);
			break;
		}
		else if (strncmp(argv[i], "-s", 2) == 0) {
			Print("-s option\n");
		}
		else if (strncmp(argv[i], "-l", 2) == 0) {
			Print("-l option\n");
		}
		else if (strncmp(argv[i], "-v", 2) == 0) {
			Print("-v option\n");
		}
		else if (strncmp(argv[i], "-t", 2) == 0) {
			Print("-t option\n");
		}
		else if (strncmp(argv[i], "-r", 2) == 0) {
			Print("-r option\n");
		}
		else if (strncmp(argv[i], "-o", 2) == 0) {
			Print("-o option\n");
		}
		else if (strncmp(argv[i], "-h", 2) == 0) {
			Print("-h option\n");
		}
		else if (strncmp(argv[i], "-f", 2) == 0) {
			Print("-f option\n");
		}
#endif 0
		/*
		 * Make this the last one!!
		 * Check for invalid parameter specifications.
		 */
		else {
			Print1("Invalid option %s specified\n",argv[i]);
			Print("Type \"tapex -h\" for help.\n");
			do_errlog(LOG_COMMAND);
			exit(1);
		}
	}
}

/*
 * Bounds check
 * Insures that an user specified parameter falls within the acceptable
 * range.  Exit if the value is out of range.
 */
bounds_check(val,min,max)
	int val,min,max;
{
	if (val < min) {
		Print2("User specified parameter %d is less than the minimum %d\n",val,min);
		do_errlog(LOG_COMMAND);
		exit(1);
	}
	else if (val > max) {
		Print2("User specified parameter %d is greater than the maximum %d\n",val,max);
		do_errlog(LOG_COMMAND);
		exit(1);
	}
}

/*
 * write_file
 *
 * Write a file onto the tape.  The file contents are a number of records
 * followed by a tape mark.  The seed is used to insure that each file
 * begins with different record contents to distinguish files.
 */
write_file(fd, record_size, num_records, seed)
	int fd;
	int record_size;
	int num_records;
	int seed;
{
	int write_errors = 0;
	int record_index;

	if (debug > 10) {
		Print1("Records in this file are of size %d bytes.\n",record_size);
		Print("record number: ");
	}
    if (flag_n == 0) {
	/*
	 * flag_n is clear; Don't use n-buffered I/O.
	 */
	for (record_index = 0 ; record_index < num_records; record_index++) {
		if (debug > 10) {
			Print1("%d ",record_index);
			flush_output();
		}
		write_errors += write_record(fd,record_size,seed+record_index);
	}
    }
    else {
	/*
	 * flag_n is set; use n-buffered I/O.
	 */
	nbuf_setup(fd, def_nbuf_buffers, 0);
	if (debug > 3)
		Print("Doing n-buffered writes\n")
	nbuf_write(fd, record_size, num_records, &write_errors, seed);
	nbuf_disable(fd, def_nbuf_buffers);
    }
	if (debug > 10) 
		Print("\n");
	/*
	 * Only pass a count of 1 to MTWEOF because this is not necessarily
	 * logical end of media.
	 */
	if ((do_tape_op(fd,MTWEOF,1) == 0) && (debug > 100)) {
		Print("End of file written.\n");
	}
	return(write_errors);
}

/*
 * read_file
 *
 * Read a file off the tape.
 */
read_file(fd, record_size, num_records, seed)
	int fd;
	int record_size;
	int num_records;
	int seed;
{
	int record_index;
	int tm_hit = 0;

	if (debug > 10)
		Print("reading record number: ");
    if (flag_n == 0) {
	for (record_index = 0 ; record_index < num_records; record_index++) {
		if (debug > 10) {
			Print1("%d ",record_index);
			if ((record_index%20) == 0)
				Print("\n");
			flush_output();
		}
		read_record(fd,record_size,seed+record_index,
			VALIDATE,&file_read_errors,&file_val_errors,NO_INIT);
	}
	/*
	 * Hop over the tape mark to be positioned at the beginning of the
	 * next file.
	 */
	hop_tm(fd);
    }
    else {
	/*
	 * flag_n is set; use n-buffered I/O.
	 */
	nbuf_setup(fd, def_nbuf_buffers, 0);
	if (debug > 10)
		Print("Perform n-buffered I/O reads.\n")
	tm_hit = nbuf_read(fd, record_size, num_records, &file_read_errors, 
				&file_val_errors, seed);
	nbuf_disable(fd, def_nbuf_buffers);
	/*
	 * Advance to the next file to allow n-buffered reading to proceed.
	 */
	if (tm_hit == 0) {
		do_tape_op(fd,MTFSF,1);
	}
	do_tape_op(fd,MTCSE,1);		/* Potential bad side effects */
    }
	if (debug > 10)
		Print("\n");
	return(file_read_errors + file_val_errors);
}

/*
 * Hop tape mark
 *
 * This routine is used to hop over tapemarks between files.
 */
void
hop_tm(fd)
	int fd;
{
	char buffer[600];
	int read_ret;
	/*
	 * This test must not be run with nbuf enabled.
 	 */
	read_ret = 0;
	if (ioctl(fd,FIONBUF,&read_ret) < 0) {
		Print("ERROR: unable to disable n-buffered IO\n");
		Print("The read phase of this test is being aborted.\n");
		flush_output();
		do_tape_op(fd,MTREW,1);
		return;
	}
	else if (debug) {
		Print("\nn-buffered IO has been disabled.\n");
	}
	/*
	 * Tape mark encountered.  Hop over the tape mark to allow
	 * future file reads to succeed.
	 * Do a read after n-buf is disabled to hop over the tape
	 * mark.   This read should return 0 to indicate EOF (tape
	 * mark encountered).
	 */
	if (debug) {
		Print("Hopping over tape mark.\n");
	}
	read_ret = read(fd,buffer, 512);
	if (read_ret != 0) {
		Print("Error, read to hop over tape mark failed.\n");
		Print1("The read returned %d instead of 0.\n",read_ret);
		if (debug > 10) {
		Print("No tape mark encountered after reading all the\n");
		Print("specified records.  This means that the file has not\n");
		Print("been completely read.\n");
		Print1("The read returned a value of %d\n",read_ret);
		Print("Skipping forward one file with MTFSF\n");
		}
		do_tape_op(fd,MTFSF,1);
		do_tape_op(fd,MTCSE,1);		/* Potential bad side effects */
	}
}
/*
 * Position file tests.
 *
 * File Movement Tests
 * 1) Write a large number of files to the tape, each file should have 
 *    unique contents for identification later.
 * 2) Move around forwards and backwards files and verify reads.
 */
position_file_test(fd)
	int fd;
{
	int position;
	int file_number, counter;
	int write_err = 0;
	int read_err = 0;
	int validate_err = 0;
	int quarter;
	long rand_position;
	int num_move;
	int file_record_size;
	int new_errs;
	int save_flag_s = 0;
	int saveflag_e;
	int subtest_failure = 0;
	int record_test_error = 0;
        int apparent_pos = 0;

	/*
	 * Set this flag zero so that the test will abort if the end of
	 * media is unexpectedly encountered in the exhaustive test mode.
	 */
	saveflag_e = flag_e;
	flag_e = 0;

	/*
	 * Problems occur in the read record routine if flag_s is set
	 * during this test.  Save the flag and restore it at the end
	 * of the routine.
	 */
	save_flag_s = flag_s;
	flag_s = 0;

	Print("\n---------------------------------------------------------\n");
	Print("\nFile Position Testing\n");
	Print1("\n%d files will be written.\n",def_num_files);
	Print1("Each file will consist of %d records.\n",def_rec_file);
	Print("The record size will be different for each file.  All \n");
	Print("records within a file will be of the same size.\n\n");
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

    /*
     * flag_G specifies a read only mode for this test.  This means that the
     * tape should already be setup and therefore there is no need to write
     * the tape and validate.  Skip directly to the testing.
     */
    if (flag_G == 0) {
	/* 
	 * Write a number of files onto the tape.  Just to make error
	 * recognition more likely, make each file contain records of
	 * different sizes.
	 */
	file_record_size = pos_rec_size;
	for (position = 0; position < def_num_files; position++) {
		if (debug > 1) {
			Print3("Writing file %d, consisting of %d records of size %d bytes.\n",
				position,def_rec_file,file_record_size);
			flush_output();
		}
		new_errs = write_file(fd,file_record_size,def_rec_file,
					position);
		if (new_errs) {
			write_err += new_errs;
		}
		file_record_size += def_rec_inc;
	}
	if (debug)
		Print("\n");
	cache_flush(fd);
	/*
	 * Two tape marks at the end of all the files.
	 * Pass a count of 2 to indicate logical end of media.
	 * Note that at this point there will be 3 tape marks on the 
	 * media because the write_file routine has already written one.
	 */
	do_tape_op(fd,MTWEOF,2);

	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	/*
	 * Read through the whole tape first to validate the writes.
	 */
	file_record_size = pos_rec_size;
	for (file_number = 0; file_number < def_num_files; file_number++) {
		if (debug > 1) {
			Print3("Reading file %d, %d records of size %d.\n",
				file_number,def_rec_file,file_record_size);
			flush_output();
		}
		new_errs = read_file(fd,file_record_size,def_rec_file,
					file_number);
		if (new_errs) {
			read_err += new_errs;
		}
		file_record_size += def_rec_inc;
	}
	if (debug > 1)
		Print("\n");
	if (write_err || read_err || validate_err) {
		Print("File Position test errors:\n");
		Print("The first step of this test is to write the files\n");
		Print("onto the tape and read in the tape as an initial\n");
		Print("verification before any positioning is done.\n");
		Print("Errors have occured during this initial step which\n");
		Print("prevents testing of file positioning.\n");
		if (write_err)
			Print1("FAILURE: %d write errors\n",write_err);
		if (read_err)
			Print1("FAILURE: %d read errors\n",read_err);
		if (validate_err)
			Print1("FAILURE: %d validate errors\n",validate_err);
		flag_s = save_flag_s;
		flag_e = saveflag_e;
		flush_output();
		return;
	}
	else if (debug) {
		Print("Initial file writes and reads have validated.\n");
	}
    }
    else {
	Print("Run file position tests on a tape which has already been\n");
	Print("written to by a previous running of the file position tests.\n");
	Print("In order for this to succeed, the test must be run with the\n");
	Print("same test parameters which were in use when the tape was\n");
	Print("written.  No other writes should have been posted to the tape\n");
	Print("since the last successful completion of the file position tests.\n");
    }
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);
	position = 0;

	Print("\n");

	/*
	 * Move to different files and read in to validate.
	 */
	/*
	 * First a very simple pattern.
	 */
	if (def_num_files > 7) {		
		if (debug)
			Print("Simple positioning exercise\n");
		/*
		 * Don't do the backward movement if the forwards fail.
		 */
							           /* 0 */
		if ((Rmove_file(fd,FORWARD,3,&position,READ)==0)&& /*0+3+1=4*/
		    (Rmove_file(fd,FORWARD,1,&position,READ)==0)){ /*4+1+1=6*/
		     Rmove_file(fd,BACKWARD,4,&position,READ);     /*6-4+1=3*/
		}
		Rmove_file(fd,FORWARD,3,&position,READ);           /*3+3+1=7*/
	}

	flush_output();
	/*
	 * Read every other file forwards.
	 */
	do_tape_op(fd,MTREW,1);
	position = 0;
	if (debug)
		Print("read every other file forwards\n");
	for (counter = 0 ; counter < ((def_num_files/2) - 1); counter++) {
		if (Rmove_file(fd,FORWARD,1,&position,READ)) {
			Print("Terminating subtest after file movement failure.\n");
			subtest_failure = 1;
			break;
		}
	}
	/*
	 * Read every other file backwards.
	 * Go backward 3 because the read goes forward 1.
         * This test depends on the read every other record forwards test
         * succeeding.  Therefore only perform the test if the previous
         * passed.
	 */
	if (debug)
		Print("read every other file backwards\n");
	if ((subtest_failure == 0) && (counter > 3)) {
	    for (counter = 0 ; counter < ((def_num_files/2)-2); counter++) {
		if (Rmove_file(fd,BACKWARD,3,&position,READ)) {
			Print("Terminating subtest after file movement failure.\n");
			break;
		}
	    }
	}
	else if (subtest_failure) {
	    Print("Skipping the read every other file backwards test due\n");
	    Print("to errors in previous subtest.\n");
	}
	flush_output();
	/*
	 * Split the tape up into quarters and issue reads.
	 */
	do_tape_op(fd,MTREW,1);
	position = 0;
        subtest_failure = 0;
	quarter = def_num_files / 4;
	if (quarter > 0) {
		if (debug)
			Print("Split tape up into quarters\n");
		/*
	 	 * Goto the 3/4 point. Backup 1 after the read to be at 3/4.
	 	 */
		if (debug)
			Print("Go to the 3/4 point and verify\n");
		if (Rmove_file(fd,FORWARD,quarter*3,&position,READ)) {
			Print("Terminating subtest after file movement failure.\n");
			Print("Test is to verify at 3/4 point.\n");
			goto sub_bail3;
		}
		move_file(fd,BACKWARD,1,&position,NO_READ);
		/*
	 	 * Goto the 1/4 point. Backup 1 after the read to be at 1/4.
	 	 */
		if (debug)
			Print("Go to the 1/4 point and verify\n");
		if (Rmove_file(fd,BACKWARD,quarter*2,&position,READ)) {
			Print("Terminating subtest after file movement failure.\n");
			Print("Test is to verify at 1/4 point.\n");
			goto sub_bail3;
		}
		move_file(fd,BACKWARD,1,&position,NO_READ);
		/*
	 	 * Goto the 1/2 point. Backup 1 after the read to be at 1/2.
	 	 */
		if (debug)
			Print("Go to the 1/2 point and verify\n");
		if (Rmove_file(fd,FORWARD,quarter,&position,READ)) {
			Print("Terminating subtest after file movement failure.\n");
			Print("Test is to verify at 1/2 point.\n");
			goto sub_bail3;
		}
		move_file(fd,BACKWARD,1,&position,NO_READ);
	}
	flush_output();
sub_bail3:
	/*
	 * Random positioning test.
	 * Generate a random number in the range of the number of files.
	 * Move to this new position and verify file.
	 */
	if (debug)
		Print1("Performing %d random file movement tests.\n",def_num_random);
	do_tape_op(fd,MTREW,1);
	position = 0;
	for (counter = 0 ; counter < (def_num_random - 1); counter++) {
		if (debug > 1) {
			Print1("Random position test number %d.\n",counter+1);
		}
		/*
		 * Generate a random file number to move to.
		 */
		rand_position = random() % def_num_files;
		/*
		 * Move to position 0 fails because there is no tape
		 * mark before the data to dance around.
		 */
		if (rand_position == 0)
			rand_position = 1;
		/*
		 * Determine direction and move.
		 */
		if (position < rand_position) {
			num_move = rand_position - position;
			if (debug > 1)
				Print3("random: position = %d, random position = %d, move %d forward\n",
					position, rand_position, num_move);
			if (Rmove_file(fd,FORWARD,num_move,&position,READ)) {
				Print("Position failure in random position test.\n");
				/*
				 * Go back out to desired location to allow
				 * tests to continue.
				 */
				if (Rmove_file(fd,FORWARD,rand_position,
						&position,READ)) {
					Print("Recovery failure.\n");
					Print("Aborting subtest.\n");
					goto sub_bail4;
				}
			}
			/*
			 * If the reposition didn't bring you to the 
			 * position you are expecting, then the record size
			 * will be incorrect.  Try to figure out where this
			 * really is by looking at the record size.
	 		 * The record size of each file is different.  
			 * It is computed as: initial record size +
	 		 * (position * record size increment factor).
			 */
			else if ((partial_read_size) && (debug)) {
				Print("Partial reads may indicate that the\n");
				Print("tape is not positioned where it is \n");
				Print("expected to be.  The tape seems to \n");
				Print("be positioned at file number ");
				apparent_pos = (partial_read_size -
					       pos_rec_size) / def_rec_inc;
				Print1("%d.\n",apparent_pos);
			}
		}
		else if (position > rand_position) {
			num_move = position - rand_position;
			if (debug > 1)
				Print3("random: position = %d, random position = %d, move %d backward\n",
					position, rand_position, num_move);
			if (Rmove_file(fd,BACKWARD,num_move,&position,READ)) {
				Print("Position failure in random position test.\n");
				if (Rmove_file(fd,FORWARD,rand_position,
						&position,READ)) {
					Print("Recovery failure.\n");
					Print("Aborting subtest.\n");
					goto sub_bail4;
				}
			}
			/*
			 * If the reposition didn't bring you to the 
			 * position you are expecting, then the record size
			 * will be incorrect.  Try to figure out where this
			 * really is by looking at the record size.
	 		 * The record size of each file is different.  
			 * It is computed as: initial record size +
	 		 * (position * record size increment factor).
			 */
			else if ((partial_read_size) && (debug)) {
				Print("Partial reads may indicate that the\n");
				Print("tape is not positioned where it is \n");
				Print("expected to be.  The tape seems to \n");
				Print("be positioned at file number ");
				apparent_pos = (partial_read_size -
					       pos_rec_size) / def_rec_inc;
				Print1("%d.\n",apparent_pos);
			}
		}
		else if (position == rand_position) {
			if (debug > 1)
				Print1("random: random position = present position = %d\n",rand_position);
		}
	}
sub_bail4:
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);
	position = 0;

	/*
	 * Not really a file positioning test, but while there are a number
	 * of records on the tape with tape marks, put in a test to see
	 * that a forward skip record command which hits a tape mark does
	 * not pass a tape mark.  Clear the serious exception state which
	 * results from encountering a tape mark.
	 */
	Print("\n");
	Print("Verify that a forward skip record command will fail if a\n");
	Print1("tape mark is encountered.  Each file contains %d records.\n",def_rec_file);
	Print1("Skip out %d records into the first file.\n",def_rec_file+5);
	skip_rec_test_ip = 1;
	/*
	 * Set expect_to_fail true to prevent unwanted output in the do_tape_op
	 * routine if debug is less than 10.
	 */
	expect_to_fail = 1;
	if (do_tape_op(fd,MTFSR,def_rec_file+5)) {
		if (debug > 100) {
			tape_file(fd,DEVIO_STATUS);
		}
		Print("SUCCESS: The forward skip record command failed as expected.\n");
		expect_to_fail = 0;
		do_tape_op(fd,MTCSE,1);
	}
	else {
		expect_to_fail = 0;
		record_test_error++;
		Print("ERROR: The forward skip record command should have failed.\n");
	}
	/*
	 * Verify that a BSR of too many records will also result in error.
	 * First skip out onto the tape a few files just in case the BSR
	 * spans a tape mark and it would hit BOT.
	 */
	skip_rec_test_ip = 0;	/* FSF must succeed */
	if ((def_num_files > 4) &&  (def_rec_file > 4) &&
		(do_tape_op(fd,MTFSF,3) == 0)) {
		Print("\n");
		Print("Skipping back over a tape mark should fail.\n");
		Print1("To test this case, go forward %d records into\n",3);
		Print1("a file.  Then go backwards %d records in\n",5);
		Print("the same file.\n");
		if (do_tape_op(fd,MTFSR,3) == 0) {
			if (debug > 1) {
				Print("Successfully skipped out 3 files.\n");
				Print("Successfully advanced 3 records.\n");
			}
			skip_rec_test_ip = 1;	/* This should fail */
			expect_to_fail = 1;
			if (do_tape_op(fd,MTBSR,10)) {
				if (debug > 100) {
					tape_file(fd,DEVIO_STATUS);
				}
				Print("SUCCESS: The backward skip record command failed as expected.\n");
				expect_to_fail = 0;
				do_tape_op(fd,MTCSE,1);
			}
			else {
				expect_to_fail = 0;
			 	Print("ERROR: The backward skip record command should have failed.\n");
			        record_test_error++;
			}
		}
		else {
			Print("ERROR: unable to skip forward 3 records.\n");
			record_test_error++;
		}
	}
	else {
		Print("Skipping reverse positioning subtest because there\n");
		Print("are less than 4 files and 4 records.\n");
	}
	skip_rec_test_ip = 0;	
	Print("\n");
	
	/*
	 * Even if the line limit is exceeded, still print out the results.
	 */
	lines_output = 0;
	if (file_range_errs || file_move_errs || file_read_errors || 
		file_val_errors || record_test_error) {
		Print("File position test failure:\n");
		if (record_test_error)
			Print1("ERROR: %d record positioning errors occured.\n",record_test_error);
		if (file_range_errs)
			Print1("ERROR: File position range errors = %d\n",file_range_errs);
		if (file_move_errs)
			Print1("FAILURE: %d non-successful ioctl position commands occured.\n",file_move_errs);
		if (file_read_errors)
			Print1("FAILURE: %d non-successful reads occured.\n",file_read_errors);
		if (file_val_errors)
			Print1("FAILURE: %d validation errors occured\n",file_val_errors);
	}
	else {
		Print("SUCCESS: File position tests completed successfully.\n\n");
	}
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	/*
	 * Set this flag zero so that the test will abort if the end of
	 * media is unexpectedly encountered in the exhaustive test mode.
	 */
	flag_s = save_flag_s;
	flag_e = saveflag_e;

}

/*
 * Front end routine to the move file command.  The intent is to recover
 * position after a move_file fails.  Recovery consists of rewind to BOT
 * and setting the position to be zero.
 */
Rmove_file(Rfd,Rdirection,Rcount,Rposition,Rdoread)
	int Rfd;
	int Rdirection;
	int Rcount;
	int *Rposition;
	int Rdoread;
{
	if (move_file(Rfd,Rdirection,Rcount,Rposition,Rdoread)) {
		Print("Performing file position recovery after failure\n");
		Print("of a file movement command.\n");
		Print1("Assumed position at time of failure is %d\n",*Rposition);
        	/*
        	 * Rewind to recover position.
         	 */
		recover_position(Rfd,Rposition);
		return(1);
	}
	return(0);
}

/*
 * move forward or backward the specified number of files.
 * Read in present file and validate.
 * Returns a value of 1 if the position would be out of range.
 */
move_file(fd,direction,count,position,doread)
	int fd;
	int direction;
	int count;
	int *position;
	int doread;
{
	int recsize;

	if (debug > 5)
		Print1("move_file: initial position is %d\n",*position);
	if (direction == FORWARD) {
		if ((*position + count) >= def_num_files) {
			if (debug) {
				Print2("move_file, position out of range, %d > %d\n",
					(*position + count), def_num_files);
			}
			file_range_errs++;
			return(1);
		}
		/*
		 * The random tests could cause us to move nowhere (count=0).
	 	 */
		if (count > 0) {
			if (debug > 5)
				Print1("move forward %d files\n",count);
			if (do_tape_op(fd,MTFSF,count)) {
				file_move_errs++;
				return(1);
			}
			*position += count;
		}
	}
	else if (direction == BACKWARD) {
		if ((*position - count) < TEST_BOT) {
			if (debug) {
				Print2("move_file, position out of range, %d < %d\n",
					(*position + count), TEST_BOT);
			}
			file_range_errs++;
			return(1);
		}
		/*
		 * The random tests could cause us to move nowhere (count=0).
	 	 */
		if (count > 0) {
			/*
			 * Going back files is a real trip.
			 * To go back X files you first have to go back
			 * X+1 files.  This is because the read_record
			 * routine does a FSR to get over a tapemark, so
			 * actually if you did a BSF 1 you would be on
			 * the same file.  When you go back files it
			 * puts you behind the tape mark.  Use the
			 * extra FSF 1 to hop over the mark.
			 * Now isn't that obvious.
			 */
			if (debug > 5)
				Print1("move backward %d files\n",count);
			if (do_tape_op(fd,MTBSF,count+1)) {
				file_move_errs++;
				return(1);
			}
			/*
			 * Hop to the other side of the tape mark.
			 */
			if (do_tape_op(fd,MTFSF,1)) {
				file_move_errs++;
				return(1);
			}
			*position -= count;
		}
	}
	else {
		Print("Invalid direction\n");
		return(1);
	}
	if (debug > 5)
		Print1("move_file: new position is %d\n",*position);
	
	/*
	 * The position has now changed to the desired spot.
	 * Read in a file and validate.
	 */
	if (doread == NO_READ) {
		if (debug > 5)
			Print("Skip read in move_file\n");
		goto skip2read;
	}
	/*
	 * The record size of each file is different.  It is computed as:
	 * initial record size + (position * record size increment factor).
	 */
	recsize = pos_rec_size + ((*position) * def_rec_inc);
	if (!(read_file(fd,recsize,def_rec_file,*position))) {
		if (debug > 2) {
			Print1("Read a full file at %d \n",*position);
			flush_output();
		}
		*position = *position +1;
	}
skip2read:
	if (debug > 5) {
		Print1("Position after read is %d\n",*position);
		if (pos_read_errs)
			Print1("pos_read_errs = %d\n",pos_read_errs);
		if (pos_validate_errs)
			Print1("pos_validate_errs = %d\n",pos_validate_errs);
	}
	return(0);
}

/*
 * Write a file to the tape using n-buffered I/O writes.
 */
nbuf_write(fd, size, num_rec, write_errs, seed)
	int fd;
	int size;		/* Record size */
	int num_rec;		/* Number of records in this file */
	int *write_errs;	/* Count of write errors encountered */
	int seed;		/* What to begin record with. */

{
	int record_number;
	int counter;
	int write_ret;
	int writemore;
	int wc;
	rw_struct read_st;
	int write_number;
	int write_pending[MAX_NBUFFERS];
	int failed_writes = 0;
	int failure_errno;
	int same_errno;
	int eom_write_errors = 0;

	/*
	 * Clear out any leftover I/O.
	 * Enable n-buffered I/O operation using def_nbuf_buffers buffers.
	 */
	nbuf_setup(fd, def_nbuf_buffers, 1);
	record_number = 0;
	write_number = 0;

	/*
	 * Initialize status.
	 */
	for(writemore = 0; writemore < MAX_NBUFFERS; writemore++) {
		write_pending[writemore] = 0;
	}
	/*
	 * Perform n-buffered writes.
	 */
	writemore = 1;
	if (debug > 2) {
		Print("writing record number: ");
		flush_output();
	}
	while (writemore) {
		for (counter=0; counter < def_nbuf_buffers; counter++){
		    /*
		     * Start a write using this buffer if it isn't already
		     * busy.  Only issue the number of records which is
		     * specified in the num_rec parameter, except for the
		     * end of media test where you write until the tape
		     * is filled up.
		     */
nbufwrite:
		    if (write_pending[counter] != WRITE_IP) {
		      if ((write_number < num_rec) || 
				(eom_test_ip || timeout_test_ip)) {
			/*
			 * The performance test can just write out garbage
			 * because it is never examined.  Ditto for the 
			 * timeout test.
			 */
			if ((perf_test_ip == 0) && (timeout_test_ip == 0)) {
			    format_writebuf(list[counter].buffer,&read_st,size,write_number+seed,WRITING);
			}
			write_ret = write(fd, list[counter].buffer,size);
			write_number++;
			if (debug > 100) {
				Print1("Started nbuf write number %d\n",write_number);
			}
			/*
			 * Don't mess with the write_errs array in the eof
			 * test because it doesn't have this array.
			 */
			if (write_ret < 0) {
				if  ((rw_test_ip) || (timeout_test_ip)) {
					*write_errs = *write_errs + 1;
				}
				if ((eom_test_ip) || (timeout_test_ip)) {
					if (tape_eot(fd)) {
						writemore = 0;
						if (debug > 10) {
							Print("EOM in nbuf write on eom test.\n");
						}
						if (eom_test_ip) {
						Print1("Observed media capacity is %d bytes.\n",record_number * size);
						}
						break;
					}
					else {
					    eom_write_errors++;
					    if (eom_write_errors > MAX_EOM_ERR){
						writemore = 0;
						Print("\n");
						Print("ERROR: Aborting the end of media test\n");
						Print("due to excessive write errors.\n");
						Print1("%d writes have failed.\n",eom_write_errors);
					    }
					    if (debug) {
						EPrint1("nbuf write %d failed, eom test.\n",counter);
					    }
					}
				}
				else {
				    if (debug) {
					EPrint1("nbuffered write %d failed\n",counter);
					if (debug > 10)
						Print1("Failed n-buf write, errno = %d\n",errno);
				    }
				    /*
				     * To avoid getting in an infinite loop if
				     * the write system call is failing, count
				     * the number of CONSECUTIVE write failures
				     * in failed_writes.  If this value exceeds
				     * twice the number of n-buffers then abort
				     * the writing.  Check for patterns of the
				     * same error number.
				     */
				    if (failed_writes == 0) {
					failure_errno = errno;
					same_errno = 1;
				    }
			  	    else if (errno == failure_errno) {
					same_errno++;
				    }
				    failed_writes++;
				    if (failed_writes > (def_nbuf_buffers*2)) {
					writemore = 0;
					Print1("ERROR: Aborting nbuf_write after %d consecutive write errors.\n",failed_writes);
					if (same_errno == failed_writes) {
					    Print1("The errno was %d every time.\n",failure_errno);
					}
					else {
					    Print2("The errno was %d in %d failures.\n",failure_errno, same_errno);
					}
					/*
					 * Only put up with these n-buf errors
					 * for so long then bail out.
					 */
					nbuf_aborts++;
					if (nbuf_aborts > MAX_NBUF_ABORTS) {
					    Print("\nERROR: aborting tapex exerciser due to repeated n-buf failures.\n");
					    Print1("%d n-buf aborts have occured.\n",nbuf_aborts);
					    do_errlog(LOG_NBUF);
					    exit(1);
					}
					break;
				    }

				}
			}
			/*
			 * A successful write.  Set flag so that the status
			 * of this operation will be examined later.
			 * failed_writes is a count of CONSECUTIVE failures;
			 * set to zero on a success case.
			 */
			else {
				write_pending[counter] = WRITE_IP;
				failed_writes = 0;
			}
		     }
		    }
		    /*
		     * The buffer is busy.  Check the status of a previous
		     * write operation.
		     */
		    else {
			if ((wc = ioctl(fd, FIONBDONE,
				&list[counter].buffer)) != -1) {
				if (wc == 0) {
					if (debug > 10)
						Print("nbuf write: eof detected\n");
					writemore = 0;
				}
				/*
				 * A partial record write.
			 	 * Don't mess with the write_errs array in the 
			 	 * eof test because it doesn't have this array.
				 */
				else if (wc != size) {
					if (debug)
						EPrint2("PARTIAL: nbuf_write, wc = %d, size = %d\n",wc, size);
					if (rw_test_ip)
						*write_errs = *write_errs + 1;
					/*
					 * Free up the buffer for later use.
				 	 */
					write_pending[counter] = 0;
					record_number++;
					if ((record_number >= num_rec) &&
						(eom_test_ip == 0)) {
						writemore = 0;
					}	
				}
				/*
				 * A full record has been written.
				 */
				else {
					/*
					 * Free up the buffer for later use.
				 	 */
					write_pending[counter] = 0;
					record_number++;
					if ((record_number >= num_rec) &&
						(eom_test_ip == 0)) {
						writemore = 0;
					}	
					if (debug > 2) {
						Print1("%d ",record_number);
						if ((record_number%20) == 0)
							Print("\n");
						flush_output();
					}
					/*
				 	 * Start up a new write with this
				 	 * freed buffer.
				 	 */
					goto nbufwrite;
				}
			}
			else {
				if (debug > 5) {
					Print1("nbuf_write #%d, returned -1\n", counter);
					if (debug > 10)
						Print1("FIONBDONE error, errno = %d\n",errno);
				}
				writemore = 0;
				/*
		 	 	 * See if the tape went offline.  
		 	 	 */
				if ((tape_file(fd,0)) == 0)
					sig_handler();
				/*
		 	 	 * Stop test if end of media encountered
		 	 	 */
				if (tape_eot(fd)) {
					if (eom_test_ip == 0) {
						Print("Fatal error: EOM in nbuf_write\n");
						Print("Unexpected end of tape encountered.\n");
						sig_handler();
					}
				}
			}
		    }
		}
	}
	if (debug > 2)
		Print("\n");
	
}

/*
 * Read a file off the tape using n-buffered I/O reads.
 */
nbuf_read(fd, size, num_rec, read_errs, validate_errs, seed)
	int fd;
	int size;		/* Record size */
	int num_rec;		/* Number of records in this file */
	int *read_errs; 	/* Count of read errors encountered */
	int *validate_errs;	/* Count of validate errors encountered */
	int seed;		/* What a record begins with, first byte */

{
	int record_number;
	int counter;
	int read_ret;
	int readmore;
	int rc;
	rw_struct write_st, read_st;
	int i;
	int read_number;
	int read_pending[MAX_NBUFFERS];
	int eof_encountered = 0;
	int failed_reads = 0;
	int failure_errno;
	int same_errno;

	/*
	 * Clear out any leftover I/O.
	 * Enable n-buffered I/O operation using def_nbuf_buffers buffers.
	 */
	nbuf_setup(fd, def_nbuf_buffers, 1);
	record_number = 0;
	read_number = 0;
        partial_read_size = 0;

	/*
	 * Initialize status.
	 */
	for(i = 0; i < MAX_NBUFFERS; i++)
		read_pending[i] = 0;
	/*
	 * Perform n-buffered reads.
	 */
	readmore = 1;
	if (debug > 2) {
		Print("reading record number: ");
		flush_output();
	}
	while (readmore) {
		for (counter=0; counter < def_nbuf_buffers; counter++){
		    /*
		     * Start a read using this buffer if it isn't already
		     * busy.  Only issue the number of records which is
		     * specified in the num_rec parameter.
		     */
nbufread:
		    if (read_pending[counter] != READ_IP) {
		      if (read_number < num_rec) {
			read_ret = read(fd, list[counter].buffer,size);
			read_number++;
			if (read_ret < 0) {
				*read_errs = *read_errs + 1;
				if (debug) {
					Print1("nbuffered read %d failed\n",counter);
					if (debug > 10)
						Print1("nbuf read error, errno = %d\n",errno);
				}
			 	/*
				 * To avoid getting in an infinite loop if
				 * the read system call is failing, count
				 * the number of CONSECUTIVE read failures
				 * in failed_reads.  If this value exceeds
				 * twice the number of n-buffers then abort
				 * the reading.  Check for patterns of the
				 * same error number.
				 */
				if (failed_reads == 0) {
					failure_errno = errno;
					same_errno = 1;
				}
			  	else if (errno == failure_errno) {
					same_errno++;
				}
				failed_reads++;
				if (failed_reads > (def_nbuf_buffers*2)) {
					Print1("ERROR: Aborting nbuf_read after %d consecutive read errors.\n",failed_reads);
					if (same_errno == failed_reads) {
					    Print1("The errno was %d every time.\n",failure_errno);
					}
					else {
					    Print2("The errno was %d in %d failures.\n",failure_errno, same_errno);
					}
					readmore = 0;
					/*
					 * Only put up with these n-buf errors
					 * for so long then bail out.
					 */
					nbuf_aborts++;
					if (nbuf_aborts > MAX_NBUF_ABORTS) {
					    Print("\nERROR: aborting tapex exerciser due to repeated n-buf failures.\n");
					    Print1("%d n-buf aborts have occured.\n",nbuf_aborts);
					    do_errlog(LOG_NBUF);
					    exit(1);
					}
					break;
		  		}
			}
			/*
			 * A successful read has been started.  Set in progress
			 * flag so that the status of this read will be
			 * examined later. The failed_reads variable is a 
			 * counter of CONSECUTIVE read failures.  Since this
			 * read did not fail, clear this failure counter.
			 */
			else {
				read_pending[counter] = READ_IP;
				failed_reads = 0;
				if (debug > 200) {
					Print1("start of read buffer %d\n",counter);
				}
			}
			if (debug)
				flush_output();
		      }
		    }
		    /*
		     * The buffer is busy.  Check the status of a previous
		     * read operation.
		     */
		    else  if (read_pending[counter] == READ_IP) {
			if ((rc = ioctl(fd, FIONBDONE,
				&list[counter].buffer)) != -1) {
				if (rc == 0) {
					if (debug > 10)
						Print("nbuf read: eof detected\n");
					readmore = 0;
					eof_encountered = 1;
				}
				/*
				 * A partial record read.
				 */
				else if (rc != size) {
					if (debug)
						Print2("PARTIAL: nbuf_read, read %d, requested %d bytes.\n",rc, size);
					*read_errs = *read_errs + 1;
					/*
					 * Stash away the partial read size
					 * for use in the file position test
					 * to get an idea where the tape
					 * actually is in the event of
					 * position messup.
					 */
					partial_read_size = rc;
					/* 
					 * Less than a full record has been
					 * read.  TMSCP tapes will now be in
					 * a serious exception state due to
					 * a  Record Data truncated condition.
					 * Clear the serious exception to allow
					 * tests to proceed.
					 * Note that n-buffered I/O is in use
					 * and that further reads may fail in
					 * the meantime before this cse is done.
					 */
					if (rc < size) {
						do_tape_op(fd,MTCSE,1);
					}
					/*
					 * Free up the buffer for later use.
				 	 */
					if (debug > 200) {
						Print1("Free buffer %d\n",counter);
						flush_output();
					}
					read_pending[counter] = 0;
					record_number++;
					if (record_number >= num_rec) {
						readmore = 0;
					}	
				}
				/*
				 * A full record has been read.
				 */
				else {
				    /* Skip validation on performance test */
				    if (perf_test_ip == 0) {
					/*
					 * Validate the contents of the read
					 * buffer.
					 */
					format_writebuf(&write_st,&read_st,size,record_number+seed,READING);
			        	for (i = 0; i < size; i++) {
						if (write_st.buffer[i] !=
						list[counter].buffer[i]) {
							*validate_errs = *validate_errs + 1;
							if (debug) {
								EPrint3("ERROR: record size %d,record #%d, write[%d] = ",size,record_number,i);
								EPrint3("%d, read[%d] = %d\n", write_st.buffer[i], i,list[counter].buffer[i]);
							}
						}
						else if (debug > 150) {
								Print3("record size %d,record #%d, write[%d] = ",size,record_number,i);
								Print3("%d, read[%d] = %d\n", write_st.buffer[i], i,list[counter].buffer[i]);
						}
			        	}
				    }
#ifdef 0
					/* 
					 * out of paranoia, zero out the read 
					 * buffer.  This can really slow things
					 * down.
					 */
			        	for (i = 0; i < def_max_record; i++) {
						list[counter].buffer[i] = def_init_char; 
					}
#endif 0

					/*
					 * Free up the buffer for later use.
				 	 */
					record_number++;
					if (debug > 2) {
						Print1("%d ",record_number);
						if ((record_number%20) == 0)
							Print("\n");
						flush_output();
					}
					read_pending[counter] = 0;
					if (record_number >= num_rec) {
						readmore = 0;
					}	
					/*
				 	 * Start up a new read with this
				 	 * freed buffer.
				 	 */
					goto nbufread;
				}
			}
			else {
				if (debug > 5) {
					Print1("nbuf_read #%d, returned -1\n", counter);
					if (debug > 10)
						Print1("FIONBDONE error, errno = %d\n",errno);
					*read_errs = *read_errs + 1;
				}
				readmore = 0;
				/*
		 	 	 * See if the tape went offline.  
		 	 	 */
				if ((tape_file(fd,0)) == 0)
					sig_handler();
				/*
		 	 	 * Stop test if end of media encountered
		 	 	 */
				if (tape_eot(fd))
					sig_handler();
			}
		    }
		}
	}
	if (debug > 2)
		Print("\n");
	return(eof_encountered);	
}

/*
 * Record size testing.
 * Writes out records of a given size.  Then reads in less than a full record.
 * Verify that the data read is correct and equal to the read count.
 * Next read greater than the full record length.  See that the value returned
 * by the read system call is equal to the record size and validate the data.
 */
size_testing(fd)
	int fd;
{
	int count_s;
	int write_err = 0;
	int read_err = 0;
	int validate_err = 0;
	int read_length;

	Print("\n---------------------------------------------------------\n");
	Print("\nPerforming record size testing.  This test verifies that\n");
	Print("at most one record is returned by a read system call.\n");
	
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	/*
	 * Write the records out onto the tape.
	 */
	if (debug > 1) {
		Print2("Writing %d records of size %d bytes.\n",def_num_record,
			def_size_record);
		if (debug > 2)
			Print("Writing record # ");
	}
	for (count_s = 0; count_s < def_num_record; count_s++) {
		if ((write_record(fd,def_size_record,count_s)) == 0) {
			if (debug > 2)  {
				Print1("%d ",count_s);
				flush_output();
			}
		}
		else {
			write_err++;
		}
	}
	if (debug > 2)
		Print("\n");
	cache_flush(fd);
	/*
	 * Make it a clean file by slapping down a tape mark.
	 * Pass a count of 2 to indicate logical end of media.
	 */
	do_tape_op(fd,MTWEOF,2);

	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	/*
	 * Read through the whole tape first to validate the writes.
	 * Before examining random records it's helpful to know that they
	 * are all valid records.
	 */
	if (debug > 2)
		Print("Reading record # ");
	for (count_s = 0; count_s < def_num_record; count_s++) {
		if (!(read_record(fd,def_size_record,count_s,VALIDATE,
		    &read_err,&validate_err,NO_INIT))){
			if (debug > 2) {
				Print1("%d ",count_s);
				flush_output();
			}
		}
	}
	if (debug > 2)
		Print("\n");
	if (write_err || read_err || validate_err) {
		Print("Record size tests being aborted due to initial errors.\n");
		Print("Before any testing is done the records are written\n");
		Print("onto the tape and then read back to verify the tape\n");
		Print("contents.  The following errors occured during this\n");
		Print("phase of the test.\n");
		if (write_err)
			Print1("FAILURE: %d write errors\n",write_err);
		if (read_err)
			Print1("FAILURE: %d read errors\n",read_err);
		if (validate_err)
			Print1("FAILURE: %d validate errors\n",validate_err);
		return;
	}
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	/*
	 * Issue reads of greater than one record in length.  These should
	 * return at most one record.
	 */
	read_length = def_size_record+OVER_READ;
	if (read_length > max_record_size) {
		if (def_size_record == max_record_size) {
			EPrint("ERROR: size testing, already at max record size\n");
			return;
		}
		else
			read_length = max_record_size;
	}
	Print("\nRecord size subtest #1:\n");
	Print("Test read requests larger than the record size.\n");
	Print2("Request a read of %d bytes to records of size %d bytes.\n",
			read_length, def_size_record);
	if (debug > 2) {
		Print("Reading record # ");
	}
	for (count_s = 0; count_s < def_num_record; count_s++) {
		if (!(read_record(fd,read_length,count_s,VALIDATE,
		    &read_err,&validate_err,DO_INIT))){
			if (debug > 2) {
				Print1("%d ",count_s);
				flush_output();
			}
		}
	}
	if (debug > 2)
		Print("\n");
	/*
	 * Even if the line limit is exceeded, still print out the results.
	 */
	lines_output = 0;
	if (write_err || read_err || validate_err) {
		Print("The following errors were encountered when trying to\n");
		Print("read more than a full record.  Read errors indicate\n");
		Print("that more than a full record has been returned.\n");
		Print("Read errors could also indicate that fewer bytes than\n");
		Print("requested were returned.\n");
		if (write_err)
			Print1("FAILURE: %d write errors\n",write_err);
		if (read_err)
			Print1("FAILURE: %d read errors\n",read_err);
		if (validate_err)
			Print1("FAILURE: %d validate errors\n",validate_err);
	}
	else {
		Print("SUCCESS: No errors encountered.\n");
	}

    if (scsi_tape(fd)) {
	/*
	 * Read less than a full record test.
	 *
	 * Presently the tmscp driver will return an error (-1) for reads of
	 * less than a full record.  There's no way to tell exactly how
	 * many bytes have been actually read.  Another issue is that a 
	 * serious exception will be raised which may need to be cleared.
	 * For this reason only run this test on scsi devices.
	 *
	 * Quoting from mtio(4): "If the record is long an error is returned."
	 * According to Fred, the SCSI hardware doesn't produce an error in
	 * this case, whereas the other types of tape hardware seems to.
	 *
	 * The TMSCP and SCSI drivers should be made consistent in this
	 * reguard!  Although this is complicated by the fact that the 
	 * hardware is not consistent.
	 */
	write_err = 0;
	read_err = 0;
	validate_err = 0;
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	/*
	 * Issue reads of less than one record in length.  These should
	 * return less than a full record.
	 */
	read_length = def_size_record-UNDER_READ;
	if (read_length <= 0)
		read_length = (def_size_record/2)+1;
	Print("\nRecord size subtest #2:\n");
	Print("Test read requests smaller than the record size\n");
	Print2("Request a read of %d bytes to records of size %d bytes\n",
			read_length, def_size_record);
	if (debug > 2) {
		Print("Reading record # ");
	}
	for (count_s = 0; count_s < def_num_record; count_s++) {
		if (!(read_record(fd,read_length,count_s,VALIDATE,
		    &read_err,&validate_err,DO_INIT))){
			if (debug > 2) {
				Print1("%d ",count_s);
				flush_output();
			}
		}
	}
	if (debug > 2)
		Print("\n");
	/*
	 * Even if the line limit is exceeded, still print out the results.
	 */
	lines_output = 0;
	if (write_err || read_err || validate_err) {
		Print("The following errors were encountered when trying to\n");
		Print("read less than a full record.  Read errors indicate\n");
		Print("that more bytes than requested may have been returned\n");
		Print("or that an error occurred on the read system call.\n");
		if (write_err)
			Print1("FAILURE: %d write errors\n",write_err);
		if (read_err)
			Print1("FAILURE: %d read errors\n",read_err);
		if (validate_err)
			Print1("FAILURE: %d validate errors\n",validate_err);
	}
	else {
		Print("SUCCESS: No errors encountered.\n");
	}
    }
}

/*
 * Random record size write/read no verify test.
 *
 * I got an SPR stating that when you do lots of quick reads to the
 * tape drive specifying a large record size that sometimes the value
 * returned by the read system call is not at all what you expecxt.
 * For example a 40k read on a 512 byte record was returning 10k.  They
 * claimed that this problem would only occur when reading quickly.  For this
 * reason I am not doing any data validation to make things go as quickly as
 * possible.
 */

rand_record(fd)
	int fd;
{
	register int record_number;
	int write_size[DEF_MAX_RANDOM];
	int read_returned[DEF_MAX_RANDOM];
	char *read_buffer;
	int rand_write_errors = 0;
	register int rand_read_errors = 0;
	register int read_return = 0;
	int rand_failure = 0;
	int saveflag_e = flag_e;
	int pass_number = 0;
	int pass_errors;

	/*
	 * Set this flag zero so that the test will abort if the end of
	 * media is unexpectedly encountered in the exhaustive test mode.
	 */
	flag_e = 0;

	Print("\n---------------------------------------------------------\n");
	Print("\nRandom record write/read tests.\n\n");
	Print1("This test writes %d records of random length,\n",def_rand_tests);
	Print("then reads the tape to verify record length.\n");
	Print("No data validation is done on this test.\n");
	if (flag_B) {
		Print("The busy flag is set.  For this reason this test\n");
		Print("will continuously loop until killed.\n");
	}
	/*
	 * Init data structures.
	 */
	if ((read_buffer = (char *)malloc( MAX_RECORD_SIZE )) <= (char *)0) {
		Print("Unable to allocate memory for buffer space.\n");
		Print("Exiting from this test.\n");
		return;
	}
	for (record_number=0; record_number < DEF_MAX_RANDOM; record_number++) {
		write_size[record_number] = 0;
		read_returned[record_number] = 0;
	}
	/*
	 * scsi tapes presently can't handle more than 16K.
 	 */
	if (scsi_tape(fd)) {
		if (def_rand_max >= SCSI_MAX_XFER) {
			def_rand_max = SCSI_MAX_XFER;
			Print1("Lowering max record size to %d for scsi device.\n",def_rand_max);
		}
	}
	Print2("Record sizes will be in the range of %d bytes to %d bytes.\n",
		def_rand_min,def_rand_max);
	Print1("All reads are of size %d bytes.\n",def_rand_max);
	Print("The read system call should return the number of bytes\n");
	Print("corresponding to the size of the written record.\n\n");
	/*
	 * Choose the random record lengths ahead of time.
	 */
	if (debug > 100) 
		Print("Random record sizes are :\n");
	for (record_number=0; record_number < def_rand_tests; record_number++) {
		write_size[record_number] = random() % def_rand_max;
		if (write_size[record_number] < def_rand_min)
			write_size[record_number] = def_rand_min;
		if (debug > 100) 
			Print1("%d ",write_size[record_number]);
	}
	if (debug > 100)
		Print("\n");

	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);
	rand_rec_ip = 1;
	/*
	 * Write out the records
	 * This could be faster, but the spr was not complaining about writes.
	 */
	if (debug > 2)
		Print("Writing record number:\n");
	for (record_number=0; record_number < def_rand_tests; record_number++) {
		if ((write_record(fd,write_size[record_number],record_number)) == 0) {
			if (debug > 2)  {
				if (debug > 50) {
					Print2("Rec #%d, \tsize %d\n",record_number,write_size[record_number]);
				}
				else {
					Print1("%d ",record_number);
				}
				flush_output();
			}
		}
		else {
			rand_write_errors++;
			EPrint1("ERROR: Write failed on record of size %d.\n",write_size[record_number]);
			flush_output();
		}
		
	}
	if (debug > 2)
		Print("\n");
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);
	
	/*
	 * Read in the records and stash the return value away for
	 * comparison later.  The size requested in the read system call
 	 * is larger than the actual record sizes.
	 */
	for (record_number=0; record_number < def_rand_tests; record_number++) {
		read_return = read(fd, read_buffer, def_rand_max); 
		if (read_return <= 0) {
			rand_read_errors++;
			if (debug > 10) {
				Print1("read error, errno = %d\n",errno);
			}
		}
		if (debug > 100)  {
			Print1("%d ",record_number);
			flush_output();
		}
		read_returned[record_number] = read_return;
		/*
		 * Busy flag.  This is to cycle until killed.
		 * If the test was about to end, start it over again.
		 * This is mainly for backward compatibility with the
		 * old mtx -v option.  This will redo the read phase only.
		 */
		if (flag_B) {
			if ((record_number + 1) == def_rand_tests) {
				int recno;
				if (debug)
					Print("restarting random read test.\n");
				/*
				 * Check results before restarting.
				 * Re-initialize read value to zero just to
				 * be sure.  When this is done set record 
				 * number to -1 because it will increment to
				 * record number 0 the next time it hits the
				 * for loop.
				 */
				pass_errors = 0;
				for (recno=0; recno < def_rand_tests; recno++) {
					if (write_size[recno] != read_returned[recno]) {
						rand_failure++;
						EPrint2("ERROR: wrote %d,\t read %d\n", write_size[recno], read_returned[recno]);
						read_returned[recno] = 0; 
					pass_errors++; 
					}
				}
				/*
	 		 	 * Rewind the tape.
	 			 */
				do_tape_op(fd,MTREW,1);
				record_number = -1;	/* Will inc to 0 */
				pass_number++;
				if (debug)
					Print("\n");
				Print1("Completed pass number %d ",pass_number);
				if (pass_errors) {
					Print1("with %d errors.\n",pass_errors);
				}
				else {
					Print("successfully.\n");
				}
				lines_output = 0;
			}
		}
	}
	if (debug > 100)
		Print("\n");
	/*
	 * Display test results.
	 */
	/*
	 * Even if the line limit is exceeded, still print out the results.
	 */
	lines_output = 0;
	Print("Random record size test results:\n")
	for (record_number=0; record_number < def_rand_tests; record_number++) {
		if (write_size[record_number] != read_returned[record_number]) {
			rand_failure++;
			Print2("ERROR: wrote %d,\t read %d\n",
				write_size[record_number], 
				read_returned[record_number])
		}
		else if (debug > 100) {
			Print2("wrote %d,\t read %d\n",
				write_size[record_number], 
				read_returned[record_number])
		}
	}
	if (rand_write_errors) 
		Print1("FAILURE: %d write errors occurred\n",rand_write_errors) 
	if (rand_read_errors) 
		Print1("FAILURE: %d read errors occurred\n",rand_read_errors) 
	if (rand_failure == 0) 
		Print("SUCCESS: all reads were of correct length.\n")
	else
		Print1("%d reads were of incorect length.\n",rand_failure)
	/*
	 * Restore orig value.
	 */
	flag_e = saveflag_e;
	rand_rec_ip = 0;
	flush_output();
	free(read_buffer);
}

#ifdef 0
/*
 * Skip the mtx translation for now.
 */
/*
 * Front end to be backward compatible with the earlier version of mtx.
 * Map old commands into the new format.
 */
mtx_front(argc2,argv2) 
	int argc2;
	char **argv2;
{

	char *devptr;
	char *fileptr;
	int reclen;
	int maxrec;
	
	/* handle input args */
	while (--argc2 > 0 && **++argv2 == '-') {
		switch (*++*argv2) {
		case 'a':		/* do all three tests */
			/*
			 * This test is supposed to do short, long and
			 * variable record length tests.  Do the short
			 * and long record test with flag_r and a large
			 * record range;
			 */
			flag_g++;	/* variable record sizes */
			flag_r++;	/* read write tests */
			flag_n++;	/* use n-buffered IO */
			flag_b++;	/* cycle until killed */
			/*
			 * Setup the record size for a wide range.
			 */
			def_min_record = MTX_DEF_SHORT;
			def_max_record = MTX_DEF_LONG;
			/* retrieve and coordinate device path */
			devptr = device + DEVOFFS;
			while (*devptr++ = *++*argv2);
			tape = device;
			break;

		case 's':		/* short record test */
			/*
			 * Setup the record size for a wide range.
			 */
			def_min_record = MTX_DEF_SHORT;
			def_max_record = MTX_DEF_SHORT;
			flag_r++;       /* read write tests */
			flag_n++;       /* use n-buffered IO */
			flag_b++;       /* cycle until killed */
			/* retrieve and coordinate device path */
			devptr = device + DEVOFFS;
			while (*devptr++ = *++*argv2);
			tape = device;
			break;

		case 'l':		/* long record exercise */
			def_min_record = MTX_DEF_LONG;
			def_max_record = MTX_DEF_LONG;
			flag_r++;       /* read write tests */
			flag_n++;       /* use n-buffered IO */
			flag_b++;       /* cycle until killed */
			/* retrieve and coordinate device path */
			devptr = device + DEVOFFS;
			while (*devptr++ = *++*argv2);
			tape = device;
			break;

		case 'v':		/* variable length record test */
			flag_g++;	/* variable record sizes */
			flag_B++;	/* cycle until killed */
			/* retrieve and coordinate device path */
			devptr = device + DEVOFFS;
			while (*devptr++ = *++*argv2);
			tape = device;
			break;

		case 't':		/* run time in minutes */
			def_time_delta = atoi(++*argv2);
			Print1("run tests for %d minutes\n",def_time_delta);
			break;

		case 'r':		/* Length of long record */
			reclen = atoi(++*argv2);
			if (reclen < 100 || reclen > 20480) {
				Print1("mtx: Invalid record length %s\n",*argv2);
				exit(0);
			}
			def_max_record = reclen;
			Print1("record length = %d\n",reclen);
			break;

		case 'o':		/* save output into file */
			fileptr = filename;
			while (*fileptr++ = *++*argv2);
			Print1("output file is %s\n",filename);
			break;

		case 'h':
			Print("HELP MESSAGE\n");
			exit(0);

		case 'f':
			if ((maxrec = atoi(++*argv2)) < 0) {
				break;
			}
			else if (maxrec == 0) {
				Print1("mtx: Invalid parameter %s\n",*argv2);
				exit(0);
			}
			def_num_record = maxrec;
			break;

		}
	}
}
#endif 0
/*
 * Record positioning information
 *
 * This is not a strict pass/fail test.  This is intended to provide status
 * output relating to record movements.  Specifically this should reveal what
 * happens when the forward/backward skip record command is issued when you
 * try to skip over tape marks.
 *
 * This will be run on all the tape drives to insure that they are consistent.
 */
position_info(fd)
	int fd;
{
	int position;
	int num_files = 3;			/* 3 files */
	int rec_file = 10;			/* each file has 10 records */
	int record_size = 100;			/* record size */
	int new_errs = 0;
	int write_err = 0;
	int read_err = 0;

	Print("\n---------------------------------------------------------\n");
	Print("\nRecord Positioning Information\n\n");
	Print3("%d files containing %d records of size %d bytes will be written.\n",
		num_files, rec_file, record_size);
	/*
	 * Write the tape to look like:
	 * BOM | 10 records | TM | 10 records | TM | 10 records | TM | TM |
	 * Validate the writes.
	 */

	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	/* 
	 * Write a number of files onto the tape.  Just to make error
	 * recognition more likely, make each file contain records of
	 * different sizes.
	 */
	for (position = 0; position < num_files; position++) {
		if (debug > 1)
			Print1("Writing file %d\n",position);
		new_errs = write_file(fd,record_size,rec_file,
					(position * rec_file));
		if (new_errs) {
			write_err += new_errs;
		}
	}
	if (debug > 1)
		Print("\n");
	cache_flush(fd);
	/*
	 * Two tape marks at the end of all the files.
	 * Pass a count of 2 to indicate logical end of media.
	 */
	do_tape_op(fd,MTWEOF,2);

	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	/*
	 * Read through the whole tape first to validate the writes.
	 */
	for (position = 0; position < num_files; position++) {
		if (debug > 1) {
			Print2("Reading file %d, record size %d.\n",position,
				record_size);
			flush_output();
		}
		new_errs = read_file(fd,record_size,rec_file,
					(position * rec_file));
		if (new_errs) {
			read_err += new_errs;
		}
	}
	if (debug > 1)
		Print("\n");
	if (write_err || read_err) {
		Print("File Position test errors:\n");
		Print("The first step of this test is to write the files\n");
		Print("onto the tape and read in the tape as an initial\n");
		Print("verification before any positioning is done.\n");
		Print("Errors have occured during this initial step which\n");
		Print("prevents testing of file positioning.\n");
		if (write_err)
			Print1("FAILURE: %d write errors\n",write_err);
		if (read_err)
			Print1("FAILURE: %d read errors\n",read_err);
		return;
	}
	else if (debug) {
		Print("Initial file writes and reads have validated.\n");
	}
	/*
	 * Now that the tape has been setup for the test, do some record
	 * movements.
	 *
 	 * Based on the first byte read from the record you can tell where
	 * on the tape you are.
	 */

	Print("\nForward skip records within the file and read.\n");
	Print("These reads are expected to succeed.\n");
	for (position = 0; position < rec_file; position++) {
		do_tape_op(fd,MTREW,1);
		Print1("Forward skip %d records from BOT and read.\n", position);
		if (do_tape_op(fd,MTFSR,position))
			Print1("MTFSR %d from BOT failed.\n",position);
		info_record(fd, record_size, position, SUCCEEDS);
	}
	Print("\nForward skip records beyond the last record.\n");
	Print("These reads are expected to fail.\n");
	for (; position < rec_file + 5; position++) {
		do_tape_op(fd,MTREW,1);
		Print1("\nForward skip %d records from BOT and read.\n", position);
		if (do_tape_op(fd,MTFSR,position))
			Print1("MTFSR %d from BOT failed.\n",position);
		/*
		 * position -1 is used here because the tapemark itself occupies
		 * a tape object.
		 */
		info_record(fd, record_size, position-1, FAILS);
#ifdef 0
		/*
		 * Now try clearing the serious exception and retry the read.
		 */
		Print("Retry this read with a clear serious exception.\n"); 
	
		do_tape_op(fd,MTREW,1);
		Print1("Forward skip %d records from BOT and read.\n", position);
		if (do_tape_op(fd,MTFSR,position))
			Print1("MTFSR %d from BOT failed.\n",position);
		/*
		 * Clear the serious exception to allow the read to succeed.
		 */
		do_tape_op(fd,MTCSE,1);
		info_record(fd, record_size, position-1, SUCCEEDS);
#endif 0
	flush_output();
	}
}

/*
 * Read in a record for the record info tests and print results.
 */
info_record(fd, size,  rec_number, expected)
	int fd;
	int size;
	int rec_number;
	int expected;
{

	int readE = 0;
	int validE = 0;

		read_record(fd,size,rec_number,VALIDATE,&readE,&validE,NO_INIT);
		if (readE) {
			Print1("%d read errors\n",readE);
		}
		if (validE) {
			Print1("%d validate errors\n",validE);
		}
		/*
		 * The global variable pass_back (hack) will contain status
	 	 * on the read as follows:
		 * 
	 	 *	0	EOF, tape mark encountered, nothing read
		 *	-1	Read error.
		 *	x	The first character read.
		 *
		 * The first character read should be one more than the 
		 * position.
		 */
		if (pass_back == -1) {
			Print("Read failed.\n");
				Print("This read was expected to ");
			if (expected == SUCCEEDS) {
				Print("succeed.\n");
			}
			else {
				Print("fail.\n");
			}
		}
		else if (pass_back == 0) {
			Print("EOF on read (tape mark encountered).\n");
			Print("This read was expected to ");
			if (expected == SUCCEEDS) {
				Print("succeed.\n");
			}
			else {
				Print("fail.\n");
			}
		}
		else {
			/*
			 * This means that the record just read contains the
			 * expected data.
			 */
			if (rec_number == (pass_back-1)) {
				if (expected == SUCCEEDS) {
					if (debug)
						Print1("successful record read at position %d\n",rec_number);
				}
				else {
						Print1("Unexpected successful record read at position %d\n",rec_number);
				}
			}
			else {
				
				Print2("ERROR: expected record number %d, but instead got record number %d\n",
				rec_number, pass_back-1);
			}
		}
}
/*
 * scsi_tape
 *
 * Retruns a value of 1 if this is a scsi tape device.
 *
 * The maximum record size on vax scsi devices is 16k which is lower than tmscp
 * tapes.
 */
#ifndef MT_ISSCSI
#define MT_ISSCSI 0x08		/* Source pool hack */
#endif
int
scsi_tape(fd)
{
	struct mtget mtget;

	if (ioctl(fd,MTIOCGET,&mtget) >= 0) {
		switch (mtget.mt_type) {
			case MT_ISST:
			case MT_ISSCSI:
				if (debug > 1)
					Print("This is a scsi tape device.\n");
				max_record_size = SCSI_MAX_XFER;
				return(1);
			default:
				if (debug > 1)
					Print("Not a scsi tape device.\n");
		}
	}
	/*
	 * The ioctl failed!  Print out a warning message and return that this
	 * is not a scsi tape.
	 */
	else {
		Print("WARNING MTIOCGET failed\n");
	}
	return(0);
}
/*
 * tmscp_tape
 *
 * Retruns a value of 1 if this is a tmscp tape device.
 *
 */
int
tmscp_tape(fd)
{
	struct mtget mtget;

	if (ioctl(fd,MTIOCGET,&mtget) >= 0) {
		switch (mtget.mt_type) {
			case MT_ISTMSCP:
				if (debug > 1)
					Print("This is a tmscp tape device.\n");
				return(1);
			default:
				if (debug > 1)
					Print("Not a tmscp tape device.\n");
		}
	}
	/*
	 * The ioctl failed!  Print out a warning message and return that this
	 * is not a scsi tape.
	 */
	else {
		Print("WARNING MTIOCGET failed\n");
	}
	return(0);
}
/*
 * worm_tape
 *
 * Retruns a value of 1 if this is a write-once tape device.
 * The intent of this routine is to cause the write past end of media test
 * to be skipped on an RV20 unit.  This is being done because the media is
 * write-only and executing this test will chew up the media.
 *
 */
int
worm_tape(fd)
{
	struct devget devio_st;

	if (ioctl(fd,DEVIOCGET,&devio_st) >= 0) {
		if ((strcmp(devio_st.device,DEV_RV20)==0) ||
		    (strcmp(devio_st.device,DEV_RV60)==0)) {
			if (debug > 50) {
				Print("This is a write once tape device.\n");
			}
			return(1);
		}
		if (debug > 50) {
			Print("This is NOT a write once tape device.\n");
		}
	}
	/*
	 * The ioctl failed!  Print out a warning message and return that this
	 * is not a scsi tape.
	 */
	else {
		Print("WARNING MTIOCGET failed\n");
	}
	return(0);
}
/*
 * setup_logfile
 *
 * Open a logfile for writing.
 */
setup_logfile(logname)
	char *logname;
{
	if (debug)
		Print1("Enabling logging to file %s\n",logname);

	fd_logfile = fopen(logname, "w");
	if (fd_logfile == NULL) {
		Print("Unable to open log file; logging disabled.\n");
	}
	else {
		if (debug > 5)
			Print1("Successfully opened %s as a logfile.\n",
				logname);
		printstatus &= ~TTYPRINT;	/* Don't send to stdout */
		printstatus |= LOGFILE;		/* Enable logging */
	}
}	

/*
 * Sanity check to verify that the contents of the read buffer has been
 * zeroed out prior to performing a read.
 */

check_read_buffer(read_st)
	rw_struct *read_st;
{
	register int i;
	register int change;

	change = 0;

	for (i = 0; i < MAX_RECORD_SIZE; i++) {
		if (read_st->buffer[i] != def_init_char) {
			if (debug > 100)
				Print1("read_buffer[%d] = (octal)%o\n",
					read_st->buffer[i])
			change++; 
			read_st->buffer[i] = def_init_char;
		}
	}

	if (debug > 100) {
		if (change) {
			Print1("%d wrongly initialized read buf elements.\n",change)
		}
		else {
			Print("All elements of the read buffer are properly initialized.\n")
		}
	}
}

/*
 * Flush output to standard out.  Also flush to logfile if logging is
 * being done.
 */
flush_output()
{
	fflush(stdout);
	if (printstatus & LOGFILE) {
		fflush(fd_logfile);
	}
}

/*
 * Performance test
 *
 * Simply time how long it takes to write and read lots of records.
 *
 * NOTE: This test operates under rather "ideal" conditions and should not
 *	 be misconstrued to imply the acheivable bandwidth of system utilities
 *	 such as dump and tar.  Specifically, this test does not have to read
 *	 data off disk to write to tape.  Also no validation is done to
 *	 insure that what is read equals what it written.  The intent of this
 *	 test is to estimate the highest bandwidth that ULTRIX can expect
 *	 to see from the unit.
 */
performance_test(fd)
	int fd;
{
	int position;
	int record_number;
	int write_err = 0;
	int read_err = 0;
	int validate_err = 0;
	int motion_error = 0;
	int saveflag_e;

	long writestart;
	long writestop;
	long writetime;
	long readstart;
	long readstop;
	long readtime;

	long datasize;
	float writerate;
	float readrate;

	/*
	 * Set this flag zero so that the test will abort if the end of
	 * media is unexpectedly encountered in the exhaustive test mode.
	 */
	saveflag_e = flag_e;
	flag_e = 0;
	perf_test_ip = 1;

	/*
	 * Lower the max record size for scsi tapes.
	 */
	if ( (scsi_tape(fd)) && (def_perf_rec_size > max_record_size) ) {
		if (debug)
			Print2("Lowering record size to %d from %d.\n",def_perf_rec_size,max_record_size);
		def_perf_rec_size = max_record_size;
	}

	Print("\n---------------------------------------------------------\n");
	Print("\nPerformance Testing\n");
	Print("\nThis test consists of timing writes and reads and\n");
	Print("calculating data transfer rates.\n\n");
	Print2("%d records of size %d bytes will be written.\n\n",def_num_perf_rec,
			def_perf_rec_size);
	flush_output();
	/*
	 * Rewind the tape.
	 * Pass a count of 2 to indicate logical end of media.
	 */
	if (do_tape_op(fd,MTREW,2))
		motion_error++;

	/* 
	 * write the buffer out to tape.  Each write is a separate record.
	 */
	if (debug > 1)
		Print("Writing record # ");
	if (flag_n == 0) {
		/*
		 * flag_n is clear; Don't use n-buffered I/O.
		 */
		/*
		 * Start write timer.
		 */
		writestart = time(0);
		for (position = 0; position < def_num_perf_rec; position++) {
			if ((write_record(fd,def_perf_rec_size,position))==0) {
				if (debug > 1)  {
					Print1("%d ",position);
					flush_output();
				}
			}
			else {
				write_err++;
			}
		}
		/*
		 * Flush data from controller's write back cache down onto
		 * the media before stopping the timer.
		 */
		cache_flush(fd);
		/*
		 * Stop write timer.
		 */
		writestop = time(0);
	}
	else {
		/*
		 * flag_n is set; use n-buffered I/O.
		 */
		nbuf_setup(fd, def_nbuf_buffers, 0);
		if (debug > 3)
			Print("Doing n-buffered writes\n")
                /*
                 * Start write timer.
                 */
                writestart = time(0);
		nbuf_write(fd, def_perf_rec_size, def_num_perf_rec, &write_err, 0);
		/*
		 * Flush data from controller's write back cache down onto
		 * the media before stopping the timer.
		 */
		cache_flush(fd);
                /*
                 * Stop write timer.
                 */
                writestop = time(0);
		nbuf_disable(fd, def_nbuf_buffers);
	}
	if (debug > 1)
		Print("\n");
	/*
	 * Make it a clean file by slapping down a tape mark.
	 * Pass a count of 2 to indicate logical end of media.
	 */
	if (do_tape_op(fd,MTWEOF,2))
		motion_error++;

	/*
	 * Rewind the tape.
	 */
	if (do_tape_op(fd,MTREW,1))
		motion_error++;

	/*
	 * Read the tape.  No validations are done to insure that the
	 * expected data is there because it would slow down the operation.
	 */
	if (flag_n == 0) {
		/*
		 * flag_n is clear; Don't use n-buffered I/O
		 */
		if (debug > 1)
			Print("Reading record # ");
                /*
                 * Start read timer.
                 */
                readstart = time(0);

		for (record_number = 0; record_number < def_num_perf_rec; record_number++) {
			if (!(read_record(fd,def_perf_rec_size,record_number,NO_VALIDATE
		    		,&read_err,&validate_err,NO_INIT))){
				if (debug > 1) {
					Print1("%d ",record_number);
					flush_output();
				}
			}
		}
                /*
                 * Stop read timer.
                 */
                readstop = time(0);
	}
	else {
		/*
		 * flag_n is set; use n-buffered I/O
		 */
		nbuf_setup(fd, def_nbuf_buffers, 0);
		if (debug > 10)
			Print("Perform n-buffered I/O reads.\n")
                /*
                 * Start read timer.
                 */
                readstart = time(0);

		nbuf_read(fd, def_perf_rec_size, def_num_perf_rec, &read_err, &validate_err, 0);
                /*
                 * Stop read timer.
                 */
                readstop = time(0);
		nbuf_disable(fd, def_nbuf_buffers);
	}
	if (debug > 1)
		Print("\n");
	/*
	 * Don't calculate any performance numbers if any errors occured.
	 * The data may not be meaningful.
	 */
	if (write_err || read_err || validate_err || motion_error) {
		Print("Performance tests being aborted due to errors.\n");
		Print("The following errors occured during this test:\n");
		if (write_err)
			Print1("FAILURE: %d write errors\n",write_err);
		if (read_err)
			Print1("FAILURE: %d read errors\n",read_err);
		if (motion_error)
			Print1("FAILURE: %d errors in rewinds or eof writes.\n",motion_error);
		/*
		 * Something really went south if this happened since no
		 * validation is performed.
		 */
		if (validate_err)
			Print1("FAILURE: %d validate errors\n",validate_err);
		flag_e = saveflag_e;
		perf_test_ip = 0;
		return;
	}

	/*
	 * Do the calculations.
	 */
	/* Elapsed time is stop time - starting time. */
	writetime = writestop - writestart;
	readtime = readstop - readstart;

	/* The number of bytes transferred equals the record size * the
	 * number of records.
	 */
	datasize = def_perf_rec_size * def_num_perf_rec;

	/* Bandwidth is the number of bytes / the elapsed time. */
	/* Normalize results to KB per second */
	writerate = (float)((datasize / writetime) / 1024);
	readrate = (float) ((datasize / readtime) / 1024);

	/*
	 * Display results.
	 */
	lines_output = 0;
	Print("Performance test results:\n");
	Print2("Record size = %d bytes, %.2f KB\n",def_perf_rec_size,(float) (def_perf_rec_size/1024));
	Print1("Number of records used = %d\n",def_num_perf_rec);
	Print1("Number of bytes transferred = %d, ",datasize);
	if (((float) (datasize/1024)) > 1024) {
		Print1("%.2f MB\n",(float) (datasize/(1024*1024)));
	}
	else {
		Print1("%.2f KB\n",(float) (datasize/1024));
	}
	Print1("Elapsed writing time is %d seconds.\n",writetime);
	Print1("Elapsed reading time is %d seconds.\n",readtime);
	if (writerate > 1024) {
		Print1("Write bandwidth is %.2f MB/second\n",(float) (writerate/1024));
	}
	else {
		Print1("Write bandwidth is %.2f KB/second\n",writerate);
	}
	if (readrate > 1024) {
		Print1("Read  bandwidth is %.2f MB/second\n",(float) (readrate/1024));
	}
	else {
		Print1("Read  bandwidth is %.2f KB/second\n",readrate);
	}
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);
	/*
	 * Restore orig value.
	 */
	flag_e = saveflag_e;
	perf_test_ip = 0;

}

/*
 * To avoid unwanted side-effects between different tests, this routine is
 * called to close the drive and reopen it.  The specific side effect of 
 * interest is that once a MTCSE is issued the driver will keep this bit 
 * set.  The only way to clear it is by doing a close/open.  If this is not
 * done the EOF test would fail because it the writes wouldn't fail when
 * EOM is encountered becaus the previous test (file position test) was 
 * doing a MTCSE which prevented the driver from failing the EOF test's 
 * writes when EOT is hit.
 *
 * This routine returns a file descriptor for the open device.
 */
int
cycle_tape(old_fd)
	int old_fd;
{
	int new_fd;

	/*
	 * Flush controller's write back cache before closing.
	 */
	cache_flush(old_fd);
	/*
	 * Close the tape.
	 */
        if (close(old_fd,O_RDWR) < 0) {
                Print1("FATAL ERROR: Cannot close %s\n", tape)
		if (debug > 10)
			Print1("cycle_tape error, errno = %d\n",errno);
		do_errlog(LOG_ABORT);
                exit(1);
        }
	else if (debug > 1) {
		Print("Closed tape.\n");
	}

	/*
	 * Open the tape device. 
	 * Determine if the device is offline.
	 */
	if ((new_fd = open(tape,openmode)) < 0) {
		Print1("Cannot open %s\n", tape)
		if (debug > 10)
			Print1("cycle_tape error, errno = %d\n",errno);
		do_errlog(LOG_ABORT);
		exit(1);
	}
	else { 
		if (tape_file(new_fd,0)) {
			if (debug > 1)
				Print1("cycle_tape: %s reopened\n",tape);
		}
		else {
			Print1("FATAL ERROR: %s went offline\n",tape);
			do_errlog(LOG_OFFLINE);
			exit(1);
		}
	}
	/*
	 * Enable cache. 
	 */
	if (flag_c) {
		if (do_tape_op(new_fd,MTCACHE,1)) {
			Print("MTCACHE failed, caching is not being used.\n");
			flag_c = 0;
		}
		else if (debug > 10) {
			Print("MTCACHE succeeded, caching re-enabled.\n");
		}
	}
	else {
		if (debug > 1)
			Print("disabling tape cache\n")
		/*
		 * SCSI tapes are intended to operate with cache enabled.
		 * Apparently "cache" does not mean quite the same thing in
		 * the SCSI world as it does in the MSCP world.
		 */
		if (scsi_tape(new_fd)) {
			if (debug > 1)
			Print("Leaving cache enabled for a SCSI tape.\n");
		}
		else {
			do_tape_op(new_fd,MTNOCACHE,1);
		}
	}

	/*
	 * Since this routine is called between tests, print out the time
	 * to give an idea of how long each test takes.
	 */
	print_time();
	/*
	 * Return the new file descriptor for the tape device.
	 */
	return(new_fd);
}

/*
 * Print out the time of day.
 */
print_time()
{
	long datm;

	datm = time(0);
	Print1("%s",ctime(&datm));
}
/*
 * Log a start or stop message to the error logger.  It is useful to have
 * this timestamp within the error log to see if tape errors are generated
 * durnig testing.
 */
void
do_errlog(logtype)
	int logtype;
{
	char errbuf[255];	/* Max errbuf size is 256? */

	if (geteuid()) {
		if (debug > 200) {
			EPrint("No error log generated, not root.\n");
		}
	}
	else {
	    switch (logtype) {
		case LOG_START:
		    sprintf(errbuf,"Started tapex - testing: %s",tape);
		    break;
		case LOG_STOP:
		    sprintf(errbuf,"Completed tapex - testing: %s",tape);
		    break;
		case LOG_ABORT:
		    sprintf(errbuf,"Aborted tapex - testing: %s",tape);
		    break;
		case LOG_TIMEUP:
		    sprintf(errbuf,"Aborted tapex timer expired: %s",tape);
		    break;
		case LOG_OFFLINE:
		    sprintf(errbuf,"Aborted tapex tape offline: %s",tape);
		    break;
		case LOG_REWFAIL:
		    sprintf(errbuf,"Aborted tapex rewind failed: %s",tape);
		    break;
		case LOG_COMMAND:
		    sprintf(errbuf,"Command line parameter error: %s",tape);
		    break;
		case LOG_NBUF:
		    sprintf(errbuf,"Repeated n-buf errors: %s",tape);
		    break;
		case LOG_CACHE:
		    sprintf(errbuf,"Cache flush errors: %s",tape);
		    break;
		case START_READTRANS:
		    sprintf(errbuf,"Start of read transport test: %s",tape);
		    break;
		case START_RANDREC:
		    sprintf(errbuf,"Start of random record test: %s",tape);
		    break;
		case START_FILEPOS:
		    sprintf(errbuf,"Start of file position test: %s",tape);
		    break;
		case START_RECPOS:
		    sprintf(errbuf,"Start of record position test: %s",tape);
		    break;
		case START_RECMOVE:
		    sprintf(errbuf,"Start of record movement test: %s",tape);
		    break;
		case START_EOM:
		    sprintf(errbuf,"Start of end of media test: %s",tape);
		    break;
		case START_SIZE:
		    sprintf(errbuf,"Start of record size test: %s",tape);
		    break;
		case START_PERF:
		    sprintf(errbuf,"Start of performance test: %s",tape);
		    break;
		case START_EOF:
		    sprintf(errbuf,"Start of end of file test: %s",tape);
		    break;
		case START_LOADER:
		    sprintf(errbuf,"Start of media loader test: %s",tape);
		    break;
		case START_TAR:
		    sprintf(errbuf,"Start of append to media test: %s",tape);
		    break;
		case START_RDWR:
		    sprintf(errbuf,"Start of write-read test: %s",tape);
		    break;
		case START_WRITETRAN:
		    sprintf(errbuf,"Start of write trans test: %s",tape);
		    break;
		case START_DISECT:
		    sprintf(errbuf,"Start of media format test: %s",tape);
		    break;
		case LOG_NOTICE:
		    sprintf(errbuf,"This test may cause driver error logs: %s",tape);
		    break;
		case START_TIMEOUT:
		    sprintf(errbuf,"Start of command timeout test: %s",tape);
		    break;
		default:
		    Print1("do_errlog: invalid error log type %d\n",logtype);
		    return;
	    }
	    (void)logerr(ELMSGT_DIAG,errbuf);
	}
}

/*
 * Tape transportability tests.
 *
 * The intent of these tests is to write a tape on ond drive and move the
 * media to another drive to insure that it can be read.
 */
/*
 * Write phase of transportability test.
 * Write a number of files to the tape and then read in for initial
 * verification.
 */
tr_write_test(fd)
	int fd;
{
	int write_errs = 0;
	int read_errs = 0;
	int any_errs = 0;

	Print("\n----------------------------------------------------------\n");
	Print("\n");
	Print("Tape transportability test - write phase.\n\n");
	Print("The intent of the transportability test is to write the tape\n");
	Print("on one drive and then move the media to another drive to\n");
	Print("insure that it can be read in properly.\n\n");
	Print("This phase of the test consists of writing a number of\n");
	Print("files to the tape and then read back the tape for\n");
	Print("initial validation.\n\n");
	write_errs = trans_write(fd);
	if (write_errs) {
		Print1("ERROR: %d write errors occured.\n",write_errs);
		flush_output();
		any_errs++;
	}
	else {
		/*
		 * The write phase completed successfully, now do a read
		 * pass to validate.
		 */
		if (debug) {
			Print("Write phase completed without error.\n");
		}
		read_errs = trans_read(fd, 0);
		if (read_errs) {
			Print1("ERROR: %d read errors occured.\n", read_errs);
			any_errs++;
		}
		else if (debug) {
			Print("Read phase completed without error.\n");
		}
		flush_output();
	}
	if (any_errs) {
		Print("\nFailures have occured in the write phase of the\n");
		Print("transportability test.  Do not use this tape for the\n");
		Print("read phase until the write phase has completed\n");
		Print("successfully.\n");
		flush_output();
	}
	else {
		Print("\nSUCCESS: the write phase of the transportability\n");
		Print("test has completed without error.  This tape may now\n");
		Print("be used for the read phase of this test.\n");
		Print("\nThe tape has been brought offline to be moved to\n");
		Print("another drive to be used in the read phase.\n");
		flush_output();
		/*
		 * Bring the tape offline.  This is purposely done to 
		 * prevent further tests to be run on this test which would
		 * write to the tape and destroy the data on it which would
		 * cause failure of the read phase.
		 */
		unload_media(fd);
	}
}
/*
 * Read phase of transportability test.
 * Reads files from the tape which was written in the write phase and 
 * verifies.
 */
tr_read_test(fd)
	int fd;
{
	int read_errs = 0;

	Print("\n----------------------------------------------------------\n");
	Print("\n");
	Print("Tape transportability test - read phase.\n\n");
	Print("This phase of the transportability test reads files from\n");
	Print("a tape which should have been previously written to by the\n");
	Print("write phase of this test.  The files are validated to insure\n");
	Print("that the data matches that of the write phase.\n");
	Print("\n");
	flush_output();

        read_errs = trans_read(fd, 1);
        if (read_errs) {
                Print("ERROR: read errors occured.\n");
		Print("This tape has failed to match the data written\n");
		Print("by the write phase of the transportability test.\n");
		Print("\n");
		Print("Make sure that this tape was successfully written by\n");
		Print("the write phase of the transportability test.\n");
		Print("\n");
		Print("The read phase of this test must have the same\n");
		Print("test parameters as the write phase.  If the write\n");
		Print("phase was run with other than the default parameters\n");
		Print("then the read phase must have these same parameters.\n");
		Print("\n");
        }
	else {
		Print("SUCCESS: the read phase was able to read in and\n");
		Print("validate that the data matches what has been\n");
		Print("written in the write phase of this test.\n");
	}
	flush_output();
}
/*
 * Tape transportability test.  Write phase.
 *
 * The transportability tests are to see if a tape can be written on
 * one system, and brought to a different system for reading.
 *
 * Returns the number of write errors.
 */
int
trans_write(fd)
	int fd;
{
	int write_err = 0;
	int new_errs = 0;
        int save_flag_s;
        int saveflag_e;
	int position;

	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

        /*
         * Set this flag zero so that the test will abort if the end of
         * media is unexpectedly encountered in the exhaustive test mode.
         */
        saveflag_e = flag_e;
        flag_e = 0;

        /*
         * Problems occur in the read record routine if flag_s is set
         * during this test.  Save the flag and restore it at the end
         * of the routine.
         */
        save_flag_s = flag_s;
        flag_s = 0;

	/* 
	 * Write a number of files onto the tape.  Just to make error
	 * recognition more likely, make each file contain records of
	 * different sizes.
	 */
	for (position = 0; position < trans_num_files; position++) {
		if (debug > 1)
			Print1("Writing file %d\n",position);
		new_errs = write_file(fd,trans_record_size,trans_rec_file,
					position);
		if (new_errs) {
			write_err += new_errs;
		}
	}
	if (debug > 1)
		Print("\n");
	flush_output();
	cache_flush(fd);
	/*
	 * Two tape marks at the end of all the files.
	 * Pass a count of 2 to indicate logical end of media.
	 */
	do_tape_op(fd,MTWEOF,2);

	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	return(write_err);
}
/*
 * Tape transportability test.  Read phase.
 *
 * The transportability tests are to see if a tape can be written on
 * one system, and brought to a different system for reading.  This can
 * verify propper head allignment from one drive to another for example.
 *
 * This phase of the test writes a number of files to the tape.
 *
 * Returns the number of read errors.
 */
int
trans_read(fd, pass_number)
	int fd;
	int pass_number;
	/* 
	 * pass_number = 0 during validation of write phase.
	 * pass_number = 1 during read phase.
	 */
{
	int file_number;
	int new_errs = 0;
	int read_err = 0;
        int save_flag_s;
        int saveflag_e;

        /*
         * Set this flag zero so that the test will abort if the end of
         * media is unexpectedly encountered in the exhaustive test mode.
         */
        saveflag_e = flag_e;
        flag_e = 0;

        /*
         * Problems occur in the read record routine if flag_s is set
         * during this test.  Save the flag and restore it at the end
         * of the routine.
         */
        save_flag_s = flag_s;
        flag_s = 0;

	/*
	 * Read through the whole tape first to validate the writes.
	 */
	for (file_number = 0; file_number < trans_num_files; file_number++) {
		if (debug > 1) {
			Print2("Reading file %d, record size %d.\n",file_number,
				trans_record_size);
			flush_output();
		}
		new_errs = read_file(fd,trans_record_size,trans_rec_file,
					file_number);
		/*
		 * Break out of this loop in the event of failure.  This 
		 * way you don't keep on reading because that doesn't prove
		 * anything.  If it fails once the test failed.
		 */
		if (new_errs) {
			read_err += new_errs;
			break;
		}
	}
	flush_output();
	if (debug > 1)
		Print("\n");
	if (read_err) {
		if (pass_number == 0) {
			Print("ERROR: Tape transportability test failure:\n");
			Print("Tape failed initial validation.\n");
			Print("Do not use this tape for the read phase\n");
			Print("of the transportability tests.\n");
			Print1("The number of read errors is %d\n",read_err);
		}
	}
	else if (debug) {
		Print("successful read of tape for transportability tests.\n");
	}
	flush_output();

	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);
	flag_s = save_flag_s;
	flag_e = saveflag_e;
	return(read_err);
}

/*
 * This test is aimed at verifying that a zero byte count is
 * returned when a tape mark is read, but another read will
 * fetch the first record of the next tape file.
 *
 * Testing involves writing a number of files to the tape,
 * then reading back records expecting EOF on tape marks and the
 * next read should succeed.
 */
void
eof_test(fd_eof)
	int fd_eof;
{

	int file_number;
	int recno;
	int retval;
	int write_errors = 0;
	int read_errors = 0;
	int read_size_errors = 0;
	int eof_not_there = 0;
	int unexpect_eof = 0;
	char *read_buffer;

	Print("\n----------------------------------------------------------\n");
	Print("\nEnd of file testing.\n\n");
	Print("This test is aimed at verifying that a zero byte count is\n");
	Print("returned when a tape mark is read, but another read will\n");
	Print("fetch the first record of the next tape file.\n\n");

	if ((read_buffer = (char *)malloc( MAX_RECORD_SIZE )) <= (char *)0) {
		Print("Unable to allocate memory for buffer space.\n");
		Print("Exiting from this test.\n");
		return;
	}
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd_eof,MTREW,1);

	/*
	 * Write a number of files to the tape.  This gives a series
	 * of records with tape marks inbetween to designate files.
	 */
	if (debug) {
		Print("Write phase of eof test.\n");
		Print1("Writing %d files.\n",eof_num_files);
		Print1("Each file consists of %d records.\n",eof_rec_file);
		Print1("Each record is of size %d bytes.\n",eof_record_size);
	}
	flush_output();
	for (file_number = 0 ; file_number < eof_num_files; file_number++) {
		if (debug > 1)
			Print1("Writing file number %d\n",file_number);
		write_errors += write_file(fd_eof, eof_record_size, 
						eof_rec_file,0);
		flush_output();
	}

	if (write_errors) {
		Print("ERROR: Write phase failure.  Due to write errors\n");
		Print("this test is being aborted.\n");
		Print1("%d write errors occured.\n",write_errors);
		do_tape_op(fd_eof,MTREW,1);
		flush_output();
		free(read_buffer);
		return;
	}
	else if (debug) {
		Print("Successful completion of write phase.\n\n");
		Print("Begin read phase of eof test.\n");
	}
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd_eof,MTREW,1);

	/*
	 * Read phase of the EOF test.
	 *
	 * This tests behavior of non-nbuffered I/O reads.  The behavior is
	 * different for n-buffered; namely n-buffered should always return
	 * 0 on EOF to allow the other reads to fail.
	 *
	 * For each file, read in the data records, no record content 
	 * validation is done.  This test only cares about return values.
	 * Next the tape mark between files should be read.  Once this
	 * tape mark is read the next read should succeed by being in the
	 * next file.
	 */
	/*
	 * This test must not be run with nbuf enabled.
 	 */
	file_number = 0;
	if (ioctl(fd_eof,FIONBUF,&file_number) < 0) {
		Print("ERROR: unable to disable n-buffered IO\n");
		Print("The read phase of this test is being aborted.\n");
		flush_output();
		do_tape_op(fd_eof,MTREW,1);
		free(read_buffer);
		return;
	}
	else if (debug) {
		Print("n-buffered IO has been disabled.\n");
	}
	for (file_number = 0 ; file_number < eof_num_files; file_number++) {
	    if (debug)
	    	Print("Reading record number: ");
	    for (recno = 0; recno < eof_rec_file; recno++) {
		if (debug)
			Print1("%d ",recno);
		retval = read (fd_eof, read_buffer, eof_record_size); 
		if (retval == 0) {
			if (debug)
				Print("Unexpected EOF\n");
			unexpect_eof++;
		}
		else if (retval > 0) {
			if (retval != eof_record_size) {
				if (debug)
					Print2("Expected %d bytes, read %d bytes.\n",eof_record_size,retval);
				read_size_errors++;
			}
		}	
		else {
			if (debug) {
				Print1("ERROR: read returned %d\n",retval);
			}
			read_errors++;
		}
		flush_output();
	    }
	    if (debug)
	    	Print("\n");
	    /*
	     * All the data records for this file have been read.  At this
	     * point the tape mark should be read in which should return
	     * a value of zero.
	     */
            retval = read (fd_eof, read_buffer, eof_record_size);
            if (retval != 0) {
                    if (debug)
                            Print1("EOF not found, read returned %d\n",retval);
                    eof_not_there++;
            }
	    else if (debug) {
		    Print("Correctly encountered EOT as expected.\n");
	    }
	    flush_output();
	}
	/*
	 * See how things went, print results.
	 */
	if ((read_errors == 0) && (read_size_errors == 0) &&
	    (eof_not_there == 0) && (unexpect_eof == 0)) {
		Print("\nSUCCESS: The eof test has completed without error.\n");
	}
	else {
		Print("\nERROR: The following failures have occured:\n");
		if (read_errors) {
			Print1("%d read system calls returned failure.\n",read_errors);
		}
		if (read_size_errors) {
			Print1("%d reads returned incorrect record sizes.\n",read_size_errors);
		}
		if (eof_not_there) {
			Print1("%d end of file tape marks were not found.\n",eof_not_there);
		}
		if (unexpect_eof) {
			Print1("%d unexpected end of file conditions occured.\n",unexpect_eof);
		}
	}
	flush_output();
	do_tape_op(fd_eof,MTREW,1);
	free(read_buffer);
}

/*
 * Read the tape to see what's on it.  Prints out the number of record
 * sizes and number of records in each file.
 */
void
disect_tape(fd)
	int fd;
{
	char *buffer;
	int read_ret;
	int file_number = 0;
	int num_eot = 0;
	int pres_record_size;
	int num_records;

	Print("\n----------------------------------------------------------\n");
	Print("Tape disection test.\n\n");
	Print("This test will read a tape and print out the format of\n");
	Print("what is on the tape.  Information includes the number and\n");
	Print("size of records in each file.  This test will fail if the\n");
	Print1("tape contains records of size larger than %d bytes.\n", max_record_size);

	if ((buffer = (char *)malloc( MAX_RECORD_SIZE )) <= (char *)0) {
		Print("Unable to allocate memory for buffer space.\n");
		Print("Exiting from this test.\n");
		return;
	}
	do_tape_op(fd,MTREW,1);

	if (debug)
		Print1("The read record size is %d\n",max_record_size);
	do_errlog(LOG_NOTICE);
newfile:
	file_number++;
	Print1("\nFile number %d:\n",file_number);
	flush_output();

	if (debug) {
		Print("\tRecord number: ");
		flush_output();
	}

	pres_record_size = 0; /* initialize */
	num_records = 0;      /* initialize */
next_record:
	read_ret = read(fd, buffer, max_record_size);
	if (read_ret < 0) {
		if (num_records > 0) {
			if (debug)
				Print("\n");
			Print2("\t%d records of size %d bytes.\n",num_records,pres_record_size);
		}
		if (tape_eot(fd)) {
			if (debug)
				Print("\n");
			Print("Physical end of media has been reached.\n");
			Print("This test is being terminated.\n");
		}
		else {
			if (debug)
				Print("\n");
			if ((num_records != 0) || (num_eot != 1)) {
				Print("Terminating this test due to read error.\n\n");
				if (debug) {
				Print1("The number of records is %d\n",num_records);
				Print1("The number of tape marks is %d\n",num_eot);
				}
			}
			else {
				/*
				 * If one tape mark has been seen and the
				 * read returns an error then the tape must
				 * not be properly terminated with 2 tape
				 * marks.
				 */
				if ((num_eot == 1) && (num_records == 0)) {
				    if (debug)
					Print("\n");
				    Print("A read error has occured after\n");
				    Print("reading a tape mark.  This probably\n");
				    Print("means that when the tape was written\n");
				    Print("it was not properly terminated with\n");
				    Print("two tape marks to indicate logical\n");
				    Print("end of media.\n");
				    Print("\nSuccessful completion of this test.\n");
				}
			}
		}
		free(buffer);
		do_tape_op(fd,MTREW,1);
		flush_output();
		return;
	}
	else if (read_ret == 0) {
		if (num_records > 0) {
			if (debug)
				Print("\n");
			Print2("\t%d records of size %d bytes.\n",num_records,pres_record_size);
		}
		num_eot++;
		if (num_eot == 1) {
			if (debug)
				Print("\n");
			Print("\tEnd of file tape mark encountered.\n");
			goto newfile;
		}
		else if (num_eot == 2) {
			if (debug)
				Print("\n");
			Print("Logical end of tape encountered.\n");
			Print("\nSuccessful completion of this test.\n");
			free(buffer);
			do_tape_op(fd,MTREW,1);
			flush_output();
			return;
		}
		else {
			Print1("Program error, num_eot = %d\n",num_eot);
		}
		flush_output();
	}
	else {
		/*
		 * Clear num_eot to indicate that a tape mark was not the 
		 * last tape object encountered.
		 */
		num_eot = 0;
		/*
		 * This is the frist record in the file.  Setup the present
		 * record size.
		 */
		if (pres_record_size == 0) {
			pres_record_size = read_ret;
			num_records = 1;
		}
		else {
			/*
			 * See if the record size is changing within the file!
			 */
			if (read_ret != pres_record_size) {
				if (debug)
					Print("\n");
				Print2("\t%d records of size %d bytes.\n",num_records,pres_record_size);
				pres_record_size = read_ret;
				num_records = 1;
			}
			/*
			 * Another record of the same size has been found.
			 */
			else {
				num_records++;
				if (debug) {
					Print1("%d ",num_records);
					if ((num_records%15) == 0) {
						Print("\n\t");
					}
				}
			}
		}
	}
	flush_output();
	goto next_record;
}

/*
 * Cache flush.  This routine is used to flush pending writes in the 
 * controller's write back cache down onto the media.  This should only be
 * done if caching has been enabled.
 *
 * Returns: 0 on successful cache flush, 1 otherwise.
 */
int
cache_flush(fd)
	int fd;
{
	int retvalue = 0;
/*
 * Source pool hack since MTFLUSH is new and not defined on all systems.
 */
#ifdef MTFLUSH
	/*
	 * Flush cache if caching has been enabled.
	 */
	if (flag_c) {
	    /*
	     * Don't do a cache flush if no writes are being done to the tape.
	     * This will prevent cache flush failures on write protected
	     * media (in the media loader test for example.
	     */
	    if (write_to_tape == 0) {
		if (debug > 100)
		    Print("Skip cache flush because write_to_tape is clear.\n");
	        return(retvalue);
	    }
	    /*
	     * Flush the controller's write back cache as long as it is 
	     * supported for this device.
	     */
	    if (mtflush_unsupported == 0) {
		retvalue = do_tape_op(fd,MTFLUSH,1);
		/*
		 * If the MTFLUSH command failed it could be for 2 reasons:
		 * 
		 * 1) The unit does not support the MTFLUSH command in which
		 *    case it would return an errno of ENXIO.
		 *
		 * 2) There really is a cache flush failure.  This should
		 *    return EIO.  This is a serious condition if failure 
		 *    occurs.  See if the unit went offline.
		 */
		if (retvalue) {
		    if (errno == ENXIO) {
			if (debug > 100)
				Print("Unit does not support MTFLUSH\n");
			/*
			 * Set this variable to indicate that the unit does not
			 * support MTFLUSH.  This is done to avoid doing the
			 * MTFLUSH many times over generating the same error
			 * messages.
			 */
			mtflush_unsupported = 1;
		    }
		    else {
			tape_file(fd,0);	/* Is it online */
			if (debug) {
			    tape_file(fd,DEVIO_STATUS);  /* Print unit status */
			}
			Print("\nERROR: A failure occured in flushing the\n");
			Print("controller\'s write back cache.  Aborting\n");
			Print("testing due to this failure.\n");
			do_tape_op(fd,MTREW,1);
		 	do_errlog(LOG_CACHE);
			exit(1);
		    }
		}
		else if (debug > 100) {
			Print("Controller cache was successfully flushed.\n");
		}
	    }
	    else if (debug > 200) {
		Print("Cache enabled, MTFLUSH unsupported, skip flush.\n");
	    }
	}
	else if (debug > 100) {
		Print("cache_flush: caching not enabled.\n");
	}
#else
	if ((flag_c) && (debug > 100)) {
		Print("cache_flush: MTFLUSH is not defined in mtio.h.\n");
		Print("cache_flush: Skip flush of controller\'s cache.\n");
	}
#endif
	flush_output();
	return(retvalue);
}

/*
 * tar_r_test()		tar "r" simulation test
 *
 * The "r" option of tar writes files to the end of the named archive.
 * As I understand it, the tape will consist of a tar saveset (one tape
 * file)  the last record of this file is all 0's.  This record does not
 * contain any data; rather it is a pad of some form.  When you want to
 * append files to a tar set, what you do is get positioned at the beginning
 * of this pad record and start writing records to the tape.  Apparently
 * this was failing on a certian 3rd party scsi device.  This failure is 
 * the result of the tape being positioned at the beginning of a data reigon
 * instead of being at a gap or filemark.
 *
 * This test consists of writing a number of records, backing up over the
 * last record and writing some more from there.  Then go and validate
 * that the "pad" record has been replaced.
 */
void
tar_r_test(fd)
	int fd;
{
	int counter = 0;
	int write_errors = 0;
	int seed;
	int read_errors = 0;
	int val_errors = 0;
        int save_flag_s;

	Print("\n----------------------------------------------------------\n");
	Print("\nAppend to media testing.\n\n");
	Print("\nThis test simulates the behavior of the \"tar r\"\n");
	Print1("command by writing %d records to the tape.\n",tar_base_recs);
	Print("Next the tape is repositioned back one record and then\n");
	Print1("%d more records are written.\n",tar_add_recs);
	Print1("All records are of size %d.\n",tar_rec_size);
	Print("Finally the resulting tape read in for verification.\n\n");
	
        /*
         * Rewind the tape.
         */
        do_tape_op(fd,MTREW,1);
	flush_output();

        /*
         * Problems occur in the read record routine if flag_s is set
         * during this test.  Save the flag and restore it at the end
         * of the routine.
         */
        save_flag_s = flag_s;
        flag_s = 0;

	/*
	 * Write some records to the tape.
	 */
        if (debug > 1) {
	        Print("Writing initial records to the tape.\n");
                Print("Writing record # ")
		flush_output();
	}
	for (counter = 0; counter < tar_base_recs; counter++) {
		if ((write_record(fd,tar_rec_size,counter)) == 0) {
			if (debug > 1)  {
				Print1("%d ",counter)
				flush_output();
			}
		}
		else {
			write_errors++;
		}
	}
	if (debug > 1) {
		Print("\n")
	}

	flush_output();
	if (write_errors) {
		Print("Aborting this test due to initial write errors.\n");
		Print1("ERROR: %d write errors occured.\n",write_errors);
		do_tape_op(fd,MTREW,1);
		return;
	}

	if (debug > 1) {
		Print("Reposition back one record.\n");
	}

	/*
	 * Back up one record.
	 */
	if (do_tape_op(fd,MTBSR,1)) {
		Print("ERROR: Aborting this test due to repositioning error.\n");
		do_tape_op(fd,MTREW,1);
		return;
	}

	flush_output();
	/*
	 * Write some additional records to the tape.
	 */
        if (debug > 1) {
	        Print("Appending records to the tape.\n");
                Print("Appending record # ")
	}
	flush_output();
	write_errors = 0;
	for (counter = 0; counter < tar_add_recs; counter++) {
		if ((write_record(fd,tar_rec_size,counter)) == 0) {
			if (debug > 1)  {
				Print1("%d ",counter)
				flush_output();
			}
		}
		else {
			write_errors++;
		}
	}
	if (debug > 1) {
		Print("\n")
	}

	if (write_errors) {
		Print("Aborting this test due write errors when trying to\n");
		Print("append records to the media.\n");
		Print1("ERROR: %d write errors occured.\n",write_errors);
		do_tape_op(fd,MTREW,1);
		return;
	}

	flush_output();
	/*
	 * Rewind and verify.
	 */
	if (debug > 1) {
		Print("Write phase completed successfuly.  Now rewind and\n");
		Print("read in for verification.\n");
	}
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);
	flush_output();

	/*
	 * read in buffer from the tape.
	 */
	if (debug > 1) {
		Print("Reading record # ")
		flush_output();
	}
	/*
	 * Read in records one at a time and validate read contents.
	 * Since we've backstepped over a record, the total number of records
 	 * on the tape is tar_base_recs+tar_add_recs-1.
	 *
 	 * The seed variable is used in validation to indicate what the 
	 * first byte looks like.  Since the counter is reset between the
	 * 2 write phases the seed will look something like this:
	 *
	 * First writes:  0 1 2 3 4 5 6 7
 	 * Second writes:               0 1 2 3 4 5 6 7 ..........
	 */
	 for (counter = 0; counter < tar_base_recs+tar_add_recs-1; counter++) {
		if (counter < (tar_base_recs -1)) {
			seed = counter;
		}
		else {
			seed = counter - tar_base_recs + 1;
		}
		if (!(read_record(fd,tar_rec_size,seed,VALIDATE,
		 &read_errors,&val_errors,NO_INIT))){
			if (debug > 1) {
				Print1("%d ",counter)
				flush_output();
			}
		}
	  }

	if (debug > 1)
		Print("\n");
	/*
	 * Print results
	 */
	if (read_errors || val_errors) {
		if (read_errors) {
			Print1("ERROR: %d read errors occured.\n",read_errors);
		}
		if (val_errors) {
			Print1("ERROR: %d validation errors occured.\n",val_errors);
		}
	}
	else {
		Print("SUCCESS: this test has completed without error.\n");
	}
	if (save_flag_s)
		flag_s = save_flag_s;
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);
	flush_output();
}
/*
 * Turn on caching.
 */
void
cache_setup(fd_read)
	int fd_read;
{
	if (do_tape_op(fd_read,MTCACHE,1)) {
		Print("MTCACHE failed, caching is not being used.\n");
		flag_c = 0;
	}
	else if (debug > 10) {
		Print("MTCACHE succeeded, caching is being used.\n");
		/*
		 * The driver may support caching, but not the MTFLUSH
		 * command.  If this is the case, set (mtflush_unsupported)
		 * to indicate that cache flushing should not be done.
		 */
		cache_flush(fd_read);
	}
}
/*
 * Media loader test.
 *
 * This test presently only supports sequential loaders.  It needs to be
 * updated for support of sequential loaders.
 */
void 
loader_test(fd)
	int fd;
{
	struct devget dev_st;
	char devname[100];
	loader_struct *ldptr;
	int num_media;
	unsigned int attributes;
	int media_count;
	int max_load_time;
	int load_status = LOAD_SUCCESS;
	int good_tapes = 0;
	int bad_tapes = 0;

	Print("\n----------------------------------------------------------\n");
	Print("\nMedia loader testing.\n\n");

	/*
	 * Find out if this "fd" is a supported loader by getting the devname
	 * and comparing it to the table of supported loaders.
	 */
	if ((ioctl(fd,DEVIOCGET,&dev_st)) < 0) {
		Print("DEVIOCGET failed\n")
		return;
        }
        else {
                strcpy(devname,dev_st.device);
		if (debug)
			Print1("The media name is %s.\n",devname);
	}
	/*
	 * See if there is a loader test developed for this device.
	 *
 	 * The interface field must be examined to see if a TQK7L is being
	 * used; this is necessary because the device field would just be TK70
	 * which can't be differentiated from TQK70 for example.  On the other
	 * hand, for the TA90 the interface field doesn't work because that
 	 * would say something like HSC70.  For the TA90 look at the device
	 * field which would say TA90.
	 */
	for (ldptr = loaders; ldptr->loader_name != NULL; ldptr++)
		if ((strncmp(dev_st.interface, ldptr->loader_name, 
					strlen(dev_st.interface)) == 0) ||
		    (strncmp(dev_st.device, ldptr->loader_name, 
					strlen(dev_st.device)) == 0))
			break;
	if (ldptr->loader_name == NULL) {
		Print("FAILURE: unable to perform any loader testing because\n");
		Print1("there are no tests developed for the %s loader.\n",devname);
		return;
	}
	Print("This test will load each piece of media.\n");
	if (flag_w) {
		Print("Since the -w flag has been specified, no writing \n");
		Print("will be done to the media.\n");
	}
	else {
		Print("Next a short data pattern will be written to the \n");
		Print("tape to verify that it is writable.  Then the tape\n");
		Print("will be read in to validate the data.  This process\n");
		Print("will be repeated for each piece of media.\n");
	}
	Print("\n");
	num_media = ldptr->num_media;
	attributes = ldptr->attributes;
	max_load_time = ldptr->max_load_min;
	if (num_media <= 1) {
		Print("FAILURE: This device supports only 1 piece of media.\n");
		Print("Skipping loader testing.\n");
		return;
	}
	Print2("Running tests on the %s loader.  This loader serves %d\n",devname,num_media);
	Print("pieces of media.\n\n");
#ifdef DEV_LOADER		/* source pool hack */
	if ((dev_st.category_stat & DEV_LOADER) == 0) {
		Print("Informational notice:\n");
		Print("This loader does not set the DEV_LOADER bit\n");
		Print("in the category_stat field of the devio structure.\n");
	}	
#endif DEV_LOADER

	flush_output();

	/*
	 * Repeat for each piece of media.
	 */
	for (media_count = 0; media_count < num_media; media_count++) {
		if (debug)
			Print1("\nTesting media piece number %d\n",media_count);
		/*
		 * Perform a write test on this piece of media.
		 * Skip the write test if the media is write-protected.
		 */
		if (flag_w == 0) {
			if (load_status == LOAD_SUCCESS) {
				if (load_write_test(fd, media_count) == 0)
					good_tapes++;
				else
					bad_tapes++;
			}
			else {
				bad_tapes++;
			 	if (debug) 
				    Print("Skip write test on write-protected media.\n");
			}
		}
		else {
			/*
			 * In read-only mode, all tapes are good tapes.
			 */
			good_tapes++;
		}
		/*
		 * Do write testing of the last piece of media, but don't try
		 * to load in the next tape because this is the last one.
		 */
		if (media_count < (num_media - 1)) {
			/*
			 * Load the next piece of media.
			 * Break out of the loop in the event of an unsuccessful
			 * loader command.
			 */
			if (debug > 10)
				Print1("Try to load media piece number %d.\n",media_count+1);
			load_status = load_command(fd,NEXT_MEDIA,max_load_time);
			if ((load_status == LOAD_TIMEOUT) ||
			    (load_status == LOAD_OPENFAIL))
				break;
			if (flag_w == 0) {
				if (load_status == LOAD_READONLY) {
					Print1("FAILURE: media piece number %d is write-protected.\n",media_count+1);
				}
				else if (load_status != LOAD_SUCCESS) {
					Print1("Unexpected load error, status = %x\n",load_status);
				}
			}
			else {
				if (load_status == LOAD_READONLY) {
					Print1("Media piece number %d is write-protected.\n",media_count+1);
				}

			}
		}
		else {
			/*
			 * Unload the last piece of media.
			 */
			if (debug > 10) 
				Print("Unload the last piece of media.\n");
			unload_media(fd);
			break;
		}
	}
	/*
	 * Print test results.
	 */
	if (good_tapes == num_media) {
	    if (flag_w == 0) {
		Print1("SUCCESS: %d pieces of writable media were loaded.\n",num_media);
	    }
	    else {
		Print1("SUCCESS: %d pieces of media were loaded.\n",num_media);
	    }
	}
	else {
	    if (good_tapes) {
		if (flag_w == 0) {
		    Print1("Successfully wrote to %d pieces of media.\n",good_tapes);
		}
		else {
		    Print1("Successfully loaded %d pieces of media.\n",good_tapes);
		}
	    }
	    if (bad_tapes)
		Print1("%d tapes were write-protected or experienced errors.\n",bad_tapes);
	    if ((good_tapes + bad_tapes) < num_media) {
		Print("The media loader was not full to capacity;\n");
		Print1("%d empty slots existed,",num_media-(good_tapes + bad_tapes));
		Print(" or the sequential loader was not\n");
		Print("initially loaded on the first tape.\n");
	    }
	}
}

/*
 * load_command: This routine performs loader commands.
 */
int
load_command(fd, command, max_load_time)
	int fd;
	int command;
	int max_load_time;		/* Passed in as MINUTES */
{
	int retries = 0;
	int load_status = LOAD_SUCCESS;
        struct devget dev_st;

	max_load_time *= 60;		/* Convert MINUTES to SECONDS */

	/*
	 * Sequential load of the next piece of media.
	 */
	if (command == NEXT_MEDIA) {
		/*
		 * Unload the presently online media.  This should cause
		 * the sequential loader to bring in the next piece of media.
		 */
		if (unload_media(fd)) {
			Print("FAILURE: unable to unload the presently\n");
			Print("online media.\n");
			return(1);	/* Failure status */
		}
		/*
		 * Just keep opening the unit until it succeeds.
		 * Only wait a limited timeout period.
		 */
		while (((fd  = open(tape,openmode)) < 0)  && 
			(retries < max_load_time)){
			retries++;
			sleep(1);
			if (debug > 100) {
				Print1("Waiting for media to load, seconds = %d\n",retries);
			}
		}
		/*
		 * The load did not succeed.  The input deck of new tapes
		 * may be empty.  Either that or the open failed for another
		 * reason (such as write protect for example.
		 */
		if (retries >= max_load_time) {
			load_status = LOAD_TIMEOUT;
			if (debug) {
				Print1("Media load timed out after %d seconds.\n",retries);
			}
			/*
			 * If the open failed it could be because the media
			 * is write-protected.
			 */
			if (flag_w == 0) {
				if ((fd = open(tape,O_RDONLY)) < 0) {
					load_status = LOAD_OPENFAIL;
				}
				else {
					if (debug > 10)
					    Print("Opened media read-only\n");
					load_status = LOAD_READONLY;
					write_to_tape = 0;
				}
			}
		}
		/*
		 * A successful load has been done.  If the tape is to be
		 * written to then set write_to_tape so that a cache flush
		 * will be done if necessary.
		 */
		else if (openmode == O_RDWR) {
			write_to_tape = 1;
		}
		/*
		 * If the unit has been opened one way or another call
		 * DEVIOCGET to get a second opinion on the write-lock status.
		 */
		if ((load_status != LOAD_OPENFAIL) && 
		    (load_status != LOAD_TIMEOUT)) {
			if ((ioctl(fd,DEVIOCGET,&dev_st)) < 0) {
				Print("load_command: DEVIOCGET failed.\n")
        		}
        		else {
				if (dev_st.stat & DEV_WRTLCK) {
					if ((load_status != LOAD_READONLY) &&
					   (debug > 100)) {
						Print1("Warning: load status mismatch, status = %x\n",load_status);
					}
					load_status = LOAD_READONLY;
				}
			}
		}
		flush_output();
	}
	return(load_status);
}
/*
 * unload_media - unloads the present piece of media.  This will bring the
 * drive offline.
 */
int
unload_media(fd)
	int fd;
{
	return(do_tape_op(fd, MTOFFL, 1));
}
/*
 * Perform a write and read back verification test to be used to determine
 * if the tape is writable.
 */
int
load_write_test(fd, media_count)
	int fd; 
	int media_count;		/* Which piece of media */
{
	int counter;
	int write_errors = 0;
	int read_errors = 0;
	int val_errors = 0;
	int ret_value;

	media_count++;			/* Tests start with tape 0 */
        /*
         * Rewind the tape.
         */
        do_tape_op(fd,MTREW,1);

	if (debug > 1) {
		Print("Write phase of media loader test.\n");
		Print2("This will write %d records of size %d bytes.\n",def_load_record,def_load_rs);
		flush_output();
	}
	/*
	 * Write some records to the tape.
	 * These don't use n-buffered I/O but this test isn't expected to
	 * move much data.
	 */
	if (debug > 5)
		Print("Writing record # ")
	for (counter = 0; counter < def_load_record; counter++) {
		if ((write_record(fd,def_load_rs,counter)) == 0) {
			if (debug > 5)  {
				Print1("%d ",counter)
				flush_output();
			}
		}
		else {
			write_errors++;
		}
	}
	if (debug > 5)
		Print("\n")
	cache_flush(fd);
	do_tape_op(fd,MTWEOF,2);
	do_tape_op(fd,MTREW,1);
	/*
	 * Read back from the tape and validate.
	 */
	if (debug > 5)
		Print("Reading record # ")
	 for (counter = 0; counter < def_load_record; counter++) {
		if (!(read_record(fd,def_load_rs,counter,VALIDATE,
		 &read_errors,&val_errors,NO_INIT))){
			if (debug > 5) {
				Print1("%d ",counter)
				flush_output();
			}
		}
	 }
	if (debug > 5)
		Print("\n")
	do_tape_op(fd,MTREW,1);
	/*
	 * Check results.
	 */
	if ((read_errors == 0) && (val_errors == 0) && (write_errors == 0)) {
		if (debug > 1)
			Print1("No errors in writing to media number %d.\n",media_count-1);
		ret_value = 0;
	}
	else {
		Print1("FAILURE: The following errors occured on media number %d.\n",media_count-1);
		if (write_errors)
			Print1("%d write errors.\n",write_errors);
		if (read_errors)
			Print1("%d read errors.\n",read_errors);
		if (val_errors)
			Print1("%d data validation errors.\n",val_errors);
		ret_value = 1;
	}
	flush_output();
	return(ret_value);
}

/*
 * Command timeout test.
 *
 * Test history: The TMSCP times each command and used to give a maximum of
 * 20 minutes for an end message to return.  On a TK70 for example it could
 * take up to 57 minutes to forward skip from beginning to end.  The 20 minute
 * time interval was not enough to allow this operation to complete.
 *
 * Test objective: To insure that the driver allows enough time for the longest
 * possible command to complete.  To accomplish this fill up the tape with 
 * files to EOT.  Count the number of files as you go.  When EOT is hit,
 * rewind the tape and then skip out to the "last" tape mark.  If this skip
 * succeeds then consider the test to be a success.
 */
timeout_test(fd)
	int fd;
{
	int file_number;
	int write_return = 0;
	int save_flag_s = 0;
	int saveflag_e;
	int keep_writing;
	int timeout_failed;
	long skip_start;
	long skip_stop;
	long skip_time;

	/*
	 * Set this flag zero so that the test will abort if the end of
	 * media is unexpectedly encountered in the exhaustive test mode.
	 */
	saveflag_e = flag_e;
	flag_e = 0;

	/*
	 * Problems occur in the read record routine if flag_s is set
	 * during this test.  Save the flag and restore it at the end
	 * of the routine.
	 */
	save_flag_s = flag_s;
	flag_s = 0;

	Print("\n---------------------------------------------------------\n");
	Print("\nCommand Timeout Testing\n");
	Print("\n");
	Print("This test verifies that a forward skip operation will\n");
	Print("be allowed ample time to complete.  The test first fills\n");
	Print("the media with files, rewinds and then skips out to the\n");
	Print("last file.  This test may take a long time to complete.\n");
	Print("\n");
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	if (debug) {
		Print2("Each file consists of %d records of size %d bytes.\n", def_timeout_nr, def_timeout_rs);
	}
	/* 
	 * Fill up the media with files.  If there is a write error see if
	 * we are at EOT.  Fail the test for any other form of error.
	 */
	timeout_test_ip = 1;
	keep_writing = 1;
	file_number = 0;
	while (keep_writing) {
		if (debug > 1) {
			Print1("Writing file number %d.\n", file_number);
			flush_output();
		}
		/*
		 * TODO: The last file to be written will get an error when
		 * trying to write the 2 terminating tape marks when end of
		 * media has been encountered.  This somewhat clouds up the
		 * output to the user and may be confusing because it is not a
		 * test error.
		 */
		write_return = write_file(fd,def_timeout_rs, def_timeout_nr,
						file_number);
		if (write_return) {
			keep_writing = 0;
			if (tape_eot) {
				if (debug) {
					Print("Encountered EOT.\n");
				}
			}
			else {
				Print("\nERROR: non-EOM error.\n");
			}
		}
		else {
			file_number++;
		}
	}
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);
	
	timeout_failed = 0;
	if (file_number > 1) {
		if (debug) {
			Print("\n");
			print_time();
			Print("\n");
			Print1("Forward skipping %d files.\n", file_number);
			Print("\n");
			flush_output();
		}
		skip_start = time(0);
		if (do_tape_op(fd,MTFSF,file_number)) {
			timeout_failed = 1;
		}
		skip_stop = time(0);
	}
	timeout_test_ip = 0;

	if (file_number > 1) {
	    skip_time = (skip_stop - skip_start) / 60;
	    Print1("The forward skip operation took %d minutes.\n",skip_time);
	    if (timeout_failed) {
		Print1("FAILURE: unable to skip out %d files", file_number);
	    }
	    else {
		Print1("SUCCESS: skipped out %d files without error.\n", file_number);
	    }
	}
	else {
		Print("ERROR: not more than one file was successfully written.\n");
	}
	flush_output();
	/*
	 * Rewind the tape.
	 */
	do_tape_op(fd,MTREW,1);

	/*
	 * Set this flag zero so that the test will abort if the end of
	 * media is unexpectedly encountered in the exhaustive test mode.
	 */
	flag_s = save_flag_s;
	flag_e = saveflag_e;

}
