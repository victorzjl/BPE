#include "util.h"
#ifndef WIN32
#include <sys/socket.h>			//socket
#include <sys/ioctl.h>			//ioctl
#include <net/if.h>				//ifreq
#include <netinet/in.h>			//SIOCGIFADDR常量，ifconf
#include <unistd.h>   			//colse(socket) 需包含该函数
#include <arpa/inet.h>
#endif

#include "stdio.h"
#include "string.h"
#define MAX_PATH_MAX 512

static int get_eth_address (struct in_addr *addr)
{
	#ifndef WIN32
	struct ifreq req;
	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	int i;
	for(i=0; i<6; i++)
	{
		char szEth[10]={0};
		snprintf(szEth,sizeof(szEth),"eth%d",i);
		strncpy (req.ifr_name, szEth, IFNAMSIZ);
		if (0 == ioctl(sock, SIOCGIFADDR, &req))
		{
			break;
		}
	}
	close(sock);

	if (6 == i)
	{
		return -1;
	}

	memcpy (addr, &((struct sockaddr_in *) &req.ifr_addr)->sin_addr, sizeof (struct in_addr));
	#endif
	return 0;
}

int GetLocalAddress (char *szLocalAddr)
{
#ifndef WIN32
	struct in_addr addr;

	get_eth_address(&addr);

	char *szAddr = inet_ntoa(addr);
	memcpy(szLocalAddr, szAddr, strlen(szAddr));
#endif

	return 0;
}

unsigned long times33_hash_func(const char *arKey, unsigned int nKeyLength)
{
        register unsigned long hash = 5381;
 
        /* variant with the hash unrolled eight times */
        for (; nKeyLength >= 8; nKeyLength -= 8) {
                hash = ((hash << 5) + hash) + *arKey++;
                hash = ((hash << 5) + hash) + *arKey++;
                hash = ((hash << 5) + hash) + *arKey++;
                hash = ((hash << 5) + hash) + *arKey++;
                hash = ((hash << 5) + hash) + *arKey++;
                hash = ((hash << 5) + hash) + *arKey++;
                hash = ((hash << 5) + hash) + *arKey++;
                hash = ((hash << 5) + hash) + *arKey++;
        }
        switch (nKeyLength) {
                case 7: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
                case 6: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
                case 5: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
                case 4: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
                case 3: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
                case 2: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
                case 1: hash = ((hash << 5) + hash) + *arKey++; break;
                case 0: break;
        }
        return hash;
}


