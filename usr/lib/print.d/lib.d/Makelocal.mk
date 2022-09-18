#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

# Makefile for lpr.d/lib.d
#
#
# AUTHOR:	Adrian Thoms
# DATE:		28th February 1989


include $(GMAKEVARS)

OBJS=argstrings.o assert.o page_sizes.o
CFILES=argstrings.c assert.c page_sizes.c

ARFILE=liblp.a

CINCLUDES =	-I. -I.. -I../../h.d -I$(SRCROOT)/usr/include

argstrings.o: argstrings.c
assert.o: assert.c
page_sizes.o: page_sizes.c


include $(GMAKERULES)
