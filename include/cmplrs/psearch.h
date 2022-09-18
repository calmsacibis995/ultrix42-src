/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: psearch.h,v 2010.2.1.5 89/11/29 22:39:08 bettina Exp $ */

/* Sort of like System V's bsearch, but not quite. "key" is a structure,
  "the_array" is an array of such structures, "elem_size" is the size in
  bytes of one element of the array, and "num_elems" is the number of
  elements in the array. Returns pointer to the element which matches
  "key", or pointer to the element which "key" ought to precede.
  Assumes array is already sorted. Both "prof" and "as1" need this. */
char *
psearch(/* char *key; char *the_array; int num_elems; int elem_size;
  procedure cmp */);
