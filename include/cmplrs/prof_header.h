/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: prof_header.h,v 2010.2.1.5 89/11/29 22:39:07 bettina Exp $ */
/* Definition of the header for a "mon.out" profile data file, or for */
/* a "bbcounts/bbaddrs" pair of files.				      */

#define BBADDRS_MAGIC 0x0f0e0010
#define BBCOUNTS_MAGIC 0x0f0e0011
#define ICOUNTS_MAGIC 0x0f0e0012

#define ADDHASH(v, w) \
	((v) = ((v) << 5) ^ ((v) >> (32-5)) ^ (w))

#define MAGIC 0x0f0e0000
#define HAS_PC_SAMPLES 1
#define HAS_INV_COUNTS 2
#define HAS_BB_COUNTS 4

struct prof_header {
   /* A magic number which tells "mprof" what data to expect in the
     profile output file, using the "define"s above */
   int p_opt_value;
   /* Lower and upper limits of pc values for pc-sampling */
   char *low_pc;
   char *high_pc;
   /* Size of the pc-sampling array, bytes */
   int pc_buf_size;
   /* Size of the bb-counting array, bytes */
   int count_buf_size;
   };

#define BB_SCALE 2	/* bb array is always half the size of text segment */
#define SAMPLE_PERIOD 10.0e-3 /* interval between pc samples in seconds */

/*

A "mon.out" file consists of:

   struct prof_header the_header;
   unsigned pc_buffer[the_header.pc_buf_size];
   unsigned count_buffer[the_header.count_buf_size];

A "bbaddrs" file consists of:

   unsigned magic = BBADDRS_MAGIC;
   unsigned hash;
   unsigned addrs[n + 1];

A "bbcounts" file consists of:

   unsigned magic = BBCOUNTS_MAGIC;
   unsigned hash;
   unsigned count[n];

If the executable contains .init and .fini sections in addition to .text,
pixie and prof treat them as one continuous .text section.

To compute the hash, apply ADDHASH(hash, word) to each word of the text
section of the pre-pixie executable file. The addrs array gives the start
of each basic block, expressed as a word offset from the beginning of the
text. These offsets appear in increasing order.  The last element of the
array is the size of the text in words. The count array gives the number
of times the corresponding basic block was executed.

If any basic-block address in the addrs array is zero, then the
corresponding element of the count array is a branch-taken count.

*/
