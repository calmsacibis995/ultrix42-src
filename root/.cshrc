#	@(#).cshrc	4.1	ULTRIX	7/2/90
if ($?prompt) then
	umask 022
	set cdpath = ( /sys /usr/spool )
	set notify
	set history = 100
	set path = ( /usr/ucb /bin /usr/bin /etc /etc/sec /usr/etc /usr/etc/sec /usr/local /usr/new /usr/hosts . )
endif
