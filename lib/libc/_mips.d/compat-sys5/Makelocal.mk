#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS= memcmp.o memcpy.o memmove.o memset.o

memcmp.o:	memcmp.s
memcpy.o:	memcpy.s
memmove.o:	memmove.s
memset.o:	memset.s

$(OBJS):
	$(CCCMD) -G 0 ../$<
	$(MV) $*.o G0/$*.o
	$(CCCMD) ../$<

clean: cleanG0

cleanG0:
	-$(RM) G0/*

include $(GMAKERULES)
