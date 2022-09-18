#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	htonl.o htons.o ntohl.o ntohs.o

htonl.o:	htonl.s
htons.o:	htons.s
ntohl.o:	ntohl.s
ntohs.o:	ntohs.s

$(OBJS):
	$(CCCMD) -G 0 ../$<
	$(MV) $*.o G0/$*.o
	$(CCCMD) ../$<

clean: cleanG0

cleanG0:
	-$(RM) G0/*

include $(GMAKERULES)
