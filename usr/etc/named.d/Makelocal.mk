# @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

KRBLIBS=-lckrb -lkrb -lknet -ldes

OBJS=	\
	$(NAMED_OBJS) $(XFER_OBJS)

NAMED_OBJS=	\
	db_dump.o db_load.o db_lookup.o db_reload.o db_save.o \
	db_update.o ns_forw.o ns_init.o ns_main.o ns_maint.o ns_req.o \
	ns_resp.o ns_sort.o ns_stats.o db_glue.o

XFER_OBJS= \
	xfer.o db_glue.o

all:	named xfer

named: $(OBJS)
	$(LDCMD) $(NAMED_OBJS) $(KRBLIBS)

xfer: $(XFER_OBJS)
	$(LDCMD) $(XFER_OBJS) $(KRBLIBS)
	
$(OBJS):
	$(CC) -c $(CFLAGS) -DDEBUG -DSTATS -DAUTHEN -DULTRIXFUNC -DNEW_SIG -I../../../../include ../$*.c
	-$(LD) -x -r $*.o
	mv a.out $*.o

db_dump.o:		db_dump.c
db_load.o:		db_load.c
db_lookup.o:		db_lookup.c
db_reload.o:		db_reload.c
db_save.o:		db_save.c
db_update.o:		db_update.c
ns_forw.o:		ns_forw.c
ns_init.o:		ns_init.c
ns_main.o:		ns_main.c
ns_maint.o:		ns_maint.c
ns_req.o:		ns_req.c
ns_resp.o:		ns_resp.c
ns_sort.o:		ns_sort.c
ns_stats.o:		ns_stats.c

install:
	(cd ../_$(MACHINE).b; \
	$(INSTALL) -c -s -o bin -g bin -m 755 named \
		$(DESTROOT)/usr/etc/named; \
	$(INSTALL) -c -s -o bin -g bin -m 755 xfer \
		$(DESTROOT)/usr/etc/named-xfer )

include $(GMAKERULES)
