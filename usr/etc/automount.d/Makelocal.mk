# @(#)Makelocal.mk	4.1   (ULTRIX)        7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

LOADLIBES = -lc

AOUT=automount

OBJS=	nfs_prot.o nfs_server.o nfs_trace.o many_cast.o auto_main.o\
	auto_look.o auto_proc.o auto_node.o auto_mount.o\
	auto_all.o auto_site.o strdup.o auto_subr.o

nfs_prot.o:	nfs_prot.c
nfs_server.o:	nfs_server.c
nfs_trace.o:	nfs_trace.c
many_cast.o:	many_cast.c
auto_main.o:	auto_main.c
auto_look.o:	auto_look.c
auto_proc.o:	auto_proc.c
auto_node.o:	auto_node.c
auto_mount.o:	auto_mount.c
auto_all.o:	auto_all.c
auto_site.o:	auto_site.c
strdup.o:	strdup.c
auto_subr.o:	auto_subr.c

install:
	$(INSTALL) -c -s automount $(DESTROOT)/usr/etc/automount


include $(GMAKERULES)
