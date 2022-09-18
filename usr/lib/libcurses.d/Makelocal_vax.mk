#  @(#)Makelocal_vax.mk	4.1  ULTRIX  7/2/90
# BSD curses(3x) package

all: $(CURSES) libcurses_p.a

libcurses_p.a: $(OBJECTS)
	@echo building profiled libcurses
	@cd profiled; ar cr ../libcurses_p.a $(OBJECTS)
	ranlib libcurses_p.a

$(MACHINE)install:
	$(INSTALL) -c -m 644 libcurses_p.a $(DESTROOT)/usr/lib/libcurses_p.a
	ranlib $(DESTROOT)/usr/lib/libcurses_p.a

clean$(MACHINE): cleanprofiled
cleanprofiled:
	-$(RM) -r profiled

.c.o:
	-@if [ ! -d profiled ] ; then \
		mkdir profiled ; \
		chmod 770 profiled ; \
	fi
	$(CC) -p -DVAX $(CFLAGS) -c ../$*.c
	-ld -X -r -o profiled/$*.o $*.o
	$(CC) -DVAX $(CFLAGS) -c ../$*.c
	-ld -x -r $*.o
	mv a.out $*.o
