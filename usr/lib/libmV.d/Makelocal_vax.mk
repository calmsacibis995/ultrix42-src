#  @(#)Makelocal_vax.mk	4.1  ULTRIX  7/2/90

all: libmV.a libmVg.a libmV_p.a

$(OBJS):
	$(CC) -c -p $(CFLAGS) -Md ../$*.c
	-${LD} -X -r -o profiled/$*.o $*.o
	$(CC) -c $(CFLAGS) -Mg ../$*.c
	-${LD} -x -r -o gfloat/$*.o $*.o
	$(CC) -c $(CFLAGS) -Md ../$*.c
	-${LD} -x -r $*.o
	mv a.out $*.o

install: 
	$(INSTALL) -c libmVg.a $(DESTLIB)/libmVg.a
	ranlib $(DESTLIB)/libmVg.a
	$(INSTALL) -c libmV.a $(DESTLIB)
	ranlib $(DESTLIB)/libmV.a
	$(INSTALL) -c libmV_p.a $(DESTLIB)/libmV_p.a
	ranlib $(DESTLIB)/libmV_p.a

clean:	cleangfloat cleanprofiled

cleangfloat:
	$(RM) gfloat/*

cleanprofiled:
	$(RM) profiled/*




