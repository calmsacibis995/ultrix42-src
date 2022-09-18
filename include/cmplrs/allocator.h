/* --------------------------------------------------- */
/* | Copyright (c) 1986 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                            | */
/* --------------------------------------------------- */
/* $Header: allocator.h,v 2010.2.1.5 89/11/29 22:38:50 bettina Exp $ */
/*------------------------------------------------------------------*/
/*| Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights |*/
/*| Reserved.  This software contains proprietary and confidential |*/
/*| information of MIPS and its suppliers.  Use, disclosure or     |*/
/*| reproduction is prohibited without the prior express written   |*/
/*| consent of MIPS.                                               |*/
/*------------------------------------------------------------------*/
/*$Header: allocator.h,v 2010.2.1.5 89/11/29 22:38:50 bettina Exp $*/

function alloc_mark (
       var fheap : pointer)
   : integer;
  external;

procedure alloc_release (
	var fheap : pointer;
	    fmark : integer);
  external;

function alloc_new (
	   fsize : integer;
       var fheap : pointer)
   : pointer;
  external;

procedure alloc_dispose (
	    fptr : pointer;
	var fheap : pointer);
  external;

function alloc_resize (
	    fptr : pointer;
	    fsize : cardinal;
	var fheap : pointer)
   : pointer;
  external;

function malloc (
	   fsize : integer)
   : pointer;
  external;

procedure free (
	    fptr : pointer);
  external;

function realloc (
           fptr : pointer;
	   fsize : integer)
   : pointer;
  external;

