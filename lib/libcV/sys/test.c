#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>

main()
{
	static int	on = 1;
	int		nread;
	char		buf[1];

	printf("fileno(stdin) = %d\n",fileno(stdin));
	if (ioctl(fileno(stdin),FIONBIO,&on) < 0)
		perror("ioctl");
	nread = read(fileno(stdin),buf,1);
	if (nread >= 0)
		printf("nread = %d\n",nread);
	else
		perror("read");
	on = 0;
	if (ioctl(fileno(stdin),FIONBIO,&on) < 0)
		perror("ioctl");
}
