/*
 *	@(#)bootblk.h	4.1	(ULTRIX)	7/2/90
 *
 * Define a bootblock.  The boot block always resides on block zero of the
 * partition if a disk, or the first block of a tape.
 */

struct bootblock{
  int reserved[2];
  int magic;               /* Must contain magic number for boot block */
  int type;                /* Boot type. */
  union{
    struct{
      unsigned ladr;         /* Load address. */
      unsigned sadr;         /* Start address. */
      int      bcnt;         /* Block count. */
      int      sblk;         /* Starting block.  Usually this block. */
    } bb0;
    struct{
      unsigned ladr;         /* Load address. */
      unsigned sadr;         /* Start address. */
      struct{
	int cnt;             /* Blocks in a span. */
	int blk;             /* Starting block of span */
      } spans[1];            /* Any number of spans, terminated by zero cnt.*/
    } bb1;
  } bb;
};

#define BB_MAGIC    161146
#define BB_CONTIGUOUS      0
#define BB_SCATTERED       1
