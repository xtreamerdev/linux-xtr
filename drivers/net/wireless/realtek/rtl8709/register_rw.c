#include <errno.h>
//#include <sys/stropts.h>
//#include <sys/dlpi.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>              /* gethostbyname, getnetbyname */
#include <unistd.h>
#include <sys/socket.h>                 /* for "struct sockaddr" et al  */
#include <sys/time.h>           /* struct timeval */
//#include <net/if.h>                     /* for IFNAMSIZ and co... */
#include <net/ethernet.h>       /* struct ether_addr */

#include <linux/wireless.h>

//arg[0]-->a.out
//arg[1]-->wlan0
//arg[2]-->read/write
//arg[3]-->addr 	u8
//arg[4]-->value	u16

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

int main(int argc, char **argv)
{
	int skfd;
	struct iwreq iwr;
	unsigned short addr;
  	char *addr_temp;
	u32 value;
	char *value_temp;
	unsigned char i;
	unsigned char addr_bb;
	u32 data;

	//init ioctl	
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd<0) return -1;
	bzero(&iwr, sizeof(iwr));
	strncpy(iwr.ifr_name, argv[1], sizeof(iwr.ifr_name));

	if(argv[3])
		iwr.u.data.length = atoi(argv[3]);

	//modify my code for reading 0x?? format
	if(argv[4])
	{
		if(!strncmp("0x", argv[4], 2))
		{
			addr_temp = &argv[4][2];
			addr = (unsigned short)strtol(addr_temp, NULL, 16);
		}
		else addr = (unsigned short)atoi(argv[4]);
	}

	if(argv[5])
	{
		if(!strncmp("0x", argv[5], 2)){
			value_temp = &argv[5][2];
			value = (u32)strtol(value_temp, NULL, 16);
			printf("value = %d\n", value);
		}
		else value = (u32)atoi(argv[5]);

		data = value;
	}

	iwr.u.data.pointer = &data;
	iwr.u.data.flags = addr;

	if(!strcmp(argv[2], "readreg"))
	{
		printf("pointer=%x, flags=%x, length=%x\n", *(u32*)iwr.u.data.pointer, iwr.u.data.flags, iwr.u.data.length);
		ioctl(skfd,  SIOCIWFIRSTPRIV + 0x0, &iwr);
		printf("addr 0x%x=%8x \n", addr, *(u32*)(iwr.u.data.pointer) );
	}
	else if(!strcmp(argv[2], "writereg"))
	{
		ioctl(skfd,  SIOCIWFIRSTPRIV + 0x1, &iwr);
	}
	else if(!strcmp(argv[2], "readrf"))
	{
		ioctl(skfd,  SIOCIWFIRSTPRIV + 0x2, &iwr);
		printf("rf_a 0x%x=%8x \n", addr, *(u32*)(iwr.u.data.pointer) );
		ioctl(skfd,  SIOCIWFIRSTPRIV + 0x3, &iwr);
		printf("rf_b 0x%x=%8x \n", addr, *(u32*)(iwr.u.data.pointer) );
	}
	else
	{
		printf("./a.out wlan0 readreg 8/16/32 0x00 \n");
		printf("./a.out wlan0 writereg 8/16/32 0x00 0\n");
		printf("./a.out wlan0 readrf 0x00 0\n");
	}
	return 0;		
	
}

