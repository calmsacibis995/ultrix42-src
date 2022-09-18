#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
include $(GMAKEVARS)

LOADLIBES=-lplot -lm

AOUT=graph
OBJS=graph.o

graph.o: graph.c

install:
	install -c -s graph ${DESTROOT}/usr/bin/graph

include $(GMAKERULES)
