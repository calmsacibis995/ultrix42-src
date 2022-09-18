#  @(#)Makelocal_mips.mk	4.1  ULTRIX  7/3/90

$(OBJS):
	$(CCCMD) -G 0 ../$<
	$(MV) $*.o G0/$*.o
	$(CCCMD) ../$<

clean: cleanG0

cleanG0:
	-$(RM) G0/*
