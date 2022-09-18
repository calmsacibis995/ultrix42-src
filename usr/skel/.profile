tty -s
if test $? = 0
then
	stty dec crt
fi
PATH=$HOME/bin:/usr/ucb:/bin:/usr/bin:/usr/local:/usr/new:/usr/hosts:.
MAIL=/usr/spool/mail/$USER
tset -n -I
export TERM MAIL PATH
biff n
