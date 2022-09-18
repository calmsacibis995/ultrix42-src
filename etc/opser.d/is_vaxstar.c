# ifndef lint
static char *sccsid = "@(#)is_vaxstar.c	4.1	(ULTRIX)	7/2/90";
# endif not lint
#ifdef vax
#include <sys/file.h>
#include <vax/rpb.h>
#include <sys/types.h>
#include <machine/cpuconf.h>

main(){
    struct rpb info;
    int fd;

    if((fd = open("/dev/kmem", O_RDONLY)) == -1){
	perror("Couldn't open /dev/kmem");
	exit(1);
    }
    lseek(fd, 0x80000000, 0);
    read(fd, &info, sizeof(struct rpb));
    if(info.cpu == VAXSTAR) exit(0);
    else exit(1);
}
#else
main(){
	exit(1);
}
#endif vax

