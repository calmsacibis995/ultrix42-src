#  @(#)Makelocal.mk	4.6	ULTRIX	2/28/91

include $(GMAKEVARS)

# These are alphabetical...

SUBDIRS=2780d.d ac.d accton.d arff.d arp.d automount.d bad144.d badsect.d \
	bindsetup.d biod.d bscconfig.d catman.d chown.d comsat.d \
	cron.d dcheck.d dgated.d diskpart.d dump.4.1.d dupterm.d \
	edquota.d fingerd.d flcopy.d ftpd.d gettable.d getty.d \
	hesupd.d implog.d inetd.d kern_loop.d kgconv.d kgmon.d kvar.d \
	lattelnet.d lcp.d lmf.d lockd.d miscd.d mkconsole.d \
	mkpasswd.d mkproto.d mountd.d named.d ncheck.d \
	netsetup.d nfsd.d nfsportmon.d nfssetup.d nfsstat.d \
	nfswatch.d ntalkd.d ntp.d pfconfig.d ping.d portmap.d presto.d \
	pstat.d quot.d quotacheck.d quotaon.d rarpd.d rdate.d \
	rdt.d renice.d repquota.d rexecd.d rlogind.d rmt.d \
	route.d routed.d rpc.bootparamd.d rpc.yppasswdd.d \
	rpcinfo.d rshd.d rwalld.d rwhod.d rxformat.d sa.d \
	savecore.d screend.d sec.d showmount.d sizer.d \
	slattach.d snmpd.d snmpsetup.d statd.d swapon.d \
	svcsetup.d tcpdump.d telnetd.d tftpd.d timed.d traceroute.d \
	trpt.d update.d vipw.d yp.d ypbind.d ypserv.d ypsetup.d

include $(GMAKERULES)
