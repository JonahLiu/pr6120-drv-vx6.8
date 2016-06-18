#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sioLib.h>
#include <ioLib.h>
#include <sockLib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <strings.h>
#include <signal.h>
#include "can.h"

static int fdbg;
static char msgbuf[256];

typedef int (*op_read)(void *handle, char *buffer, size_t maxbytes);
typedef int (*op_write)(void *handle, char *buffer, size_t nbytes);

typedef struct SerialPort{
	int fd;
}SerialPort_t;

typedef struct UdpPort{
	int fd;
	struct sockaddr_in sa_src;
	int sa_src_len;
	struct sockaddr_in sa_dst;
	int sa_dst_len;
}UdpPort_t;

typedef struct CanPort{
	int fdCtr;
	int fdTx;
	int fdRx;
	BOOL ext;
}CanPort_t;

typedef struct IOChannel{
	char name[128];
	//int type;
	void *handle;
	op_read read;
	op_write write;
	BOOL ready;
}IOChannel_t;

typedef struct IOPeer{
	int i;
	IOChannel_t *pCIn;
	IOChannel_t *pCOut;
	pthread_t thread;
}IOPeer_t;

static void* Dispatch(void *pdata)
{
	int n,ret;
	char buf[2048];
	IOPeer_t *pPeer;
	IOChannel_t *pCIn;
	IOChannel_t *pCOut;
	pPeer = (IOPeer_t *)pdata;
	pCIn = pPeer->pCIn;
	pCOut = pPeer->pCOut;
	sprintf(msgbuf,"CH%d(%s -> %s) Start \n",
			pPeer->i,pCIn->name,pCOut->name);
	write(fdbg,msgbuf,strlen(msgbuf));
	do{
		n=pCIn->read(pCIn->handle,buf,sizeof(buf)-1);		
		if(n>0){
			buf[n]=0;
			ret=pCOut->write(pCOut->handle,buf,strlen(buf));
			ret=snprintf(msgbuf,sizeof(msgbuf),"CH%d(%s -> %s): %s\n",
					pPeer->i, pCIn->name, pCOut->name, buf);
			ret=write(fdbg,msgbuf,strlen(msgbuf));
		}
	}while(n>=0);
	sprintf(msgbuf,"CH%d(%s -> %s) Exit\n",pPeer->i, pCIn->name, pCOut->name);
	write(fdbg,msgbuf,strlen(msgbuf));
	pthread_exit(NULL);
	return NULL;
}

static int SerialRead(void *handle, char *buffer, size_t maxbytes)
{
	int fd=((SerialPort_t*)handle)->fd;
	return read(fd, buffer, maxbytes);
}

static int SerialWrite(void *handle, char *buffer, size_t nbytes)
{
	int fd=((SerialPort_t*)handle)->fd;
	return write(fd, buffer, nbytes);
}

static int InitSerialChannel(IOChannel_t *pCh, char *fn, int baud, int parity)
{
	int fd;
	int flag;	
	SerialPort_t *serPort;
	
	pCh->ready=FALSE;
	
	fd=open(fn,O_RDWR);	
	if(fd==ERROR)
		return -1;
	
	//ioctl(fd, FIOSETOPTIONS, OPT_LINE);
	ioctl(fd, FIOSETOPTIONS, OPT_RAW);
	ioctl(fd, SIO_BAUD_SET, baud);
	flag = CS8;
	if(parity>0)
		flag |= PARENB;
	if(parity==1)
		flag |= PARODD;
	ioctl(fd, SIO_HW_OPTS_SET, flag);
	
	sprintf(pCh->name,"%s",fn);
	serPort=malloc(sizeof(SerialPort_t));
	serPort->fd=fd;
	pCh->handle=serPort;
	pCh->read=SerialRead;
	pCh->write=SerialWrite;
	pCh->ready=TRUE;
	return 0;
}

static int UdpRead(void *handle, char *buffer, size_t maxbytes)
{
	UdpPort_t *port=(UdpPort_t*)handle;
	int fd=port->fd;
	return recvfrom(fd, buffer, maxbytes, 0, 
			(struct sockaddr*)&port->sa_src, &port->sa_src_len);
}

static int UdpWrite(void *handle, char *buffer, size_t nbytes)
{
	UdpPort_t *port=(UdpPort_t*)handle;
	int fd=port->fd;
	return sendto(fd, buffer, nbytes, 0,
			(struct sockaddr*)&port->sa_dst, port->sa_dst_len);
}

static int InitUdpChannel(IOChannel_t *pCh, char *src_ip, int src_port, char *dst_ip, int dst_port)
{
	int fd;
	UdpPort_t *udpPort;
	
	pCh->ready = FALSE;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == ERROR)
		return fd;
	
	udpPort=malloc(sizeof(UdpPort_t));
	
	udpPort->sa_src.sin_family = AF_INET;
	udpPort->sa_src.sin_addr.s_addr = inet_addr(src_ip);
	udpPort->sa_src.sin_port = htons(src_port);
	udpPort->sa_src_len = sizeof(udpPort->sa_src);
	
	if( bind(fd,(struct sockaddr *)&udpPort->sa_src , udpPort->sa_src_len) < 0){
		close(fd);
		fd = ERROR;
		return fd;
	}
	
	udpPort->sa_dst.sin_family = AF_INET;
	udpPort->sa_dst.sin_addr.s_addr = inet_addr(dst_ip);
	udpPort->sa_dst.sin_port = htons(dst_port);
	udpPort->sa_dst_len = sizeof(udpPort->sa_dst);

	udpPort->fd=fd;
	pCh->handle=udpPort;
	pCh->read=UdpRead;
	pCh->write=UdpWrite;
	pCh->ready = TRUE;
	
	sprintf(pCh->name,"UDP%d",src_port);

	return 0;
}

static int CanRead(void *handle, char *buffer, size_t maxbytes)
{
	WNCAN_CHNMSG rxdata;
	WNCAN_BUSINFO businfo;
	struct fd_set readFds;
	int n;
	CanPort_t *port=(CanPort_t*)handle;
	int fd=port->fdRx;
	FD_ZERO(&readFds);
	FD_SET(fd, &readFds);
	select(fd+1, &readFds, NULL, NULL, NULL);
	//ioctl(port->fdCtr, WNCAN_BUSINFO_GET, (int)&businfo);
	//if(businfo.busStatus || businfo.busError)
	//	return -1;	
	
	n=read(fd, (char*)&rxdata, sizeof(rxdata));
	if(n==0)
		return 0;
	if(n!=sizeof(rxdata))
		return -1;

	if(rxdata.len)
		memcpy(buffer,rxdata.data,rxdata.len);
	return rxdata.len;
}

static int CanWrite(void *handle, char *buffer, size_t nbytes)
{
	WNCAN_CHNMSG txdata;
	size_t chksz,total;
	struct fd_set writeFds;
	CanPort_t *port=(CanPort_t*)handle;
	int fd=port->fdTx;
	txdata.id = (ULONG)handle; /* just for test */
	txdata.extId = port->ext;
	txdata.rtr = FALSE;
	total=0;
	FD_ZERO(&writeFds);
	FD_SET(fd, &writeFds);
	while(nbytes>0)
	{
		if(nbytes>8)
			chksz=8;
		else
			chksz=nbytes;
		memcpy(txdata.data,buffer,chksz);
		txdata.len=chksz;
		FD_SET(fd, &writeFds);
		select(fd+1, NULL, &writeFds, NULL, NULL);
		if (FD_ISSET(fd, &writeFds))
		{
			if(write(fd, (char*)&txdata, sizeof(txdata))!=sizeof(txdata))
			return -1;
			buffer+=chksz;
			nbytes-=chksz;
			total+=chksz;
			FD_CLR(fd, &writeFds);
		}		
	}
	return total;
}

static void UpdateBaudRate(WNCAN_CONFIG *cfg, int baud)
{
	
//#  define CAN_TIM0_10K		  49
//#  define CAN_TIM1_10K		0x1c
//#  define CAN_TIM0_20K		  24	
//#  define CAN_TIM1_20K		0x1c
//#  define CAN_TIM0_40K		0x89	/* Old Bit Timing Standard of port */
//#  define CAN_TIM1_40K		0xEB	/* Old Bit Timing Standard of port */
//#  define CAN_TIM0_50K		   9
//#  define CAN_TIM1_50K		0x1c
//#  define CAN_TIM0_100K              4    /* sp 87%, 16 abtastungen, sjw 1 */
//#  define CAN_TIM1_100K           0x1c
//#  define CAN_TIM0_125K		   3
//#  define CAN_TIM1_125K		0x1c
//#  define CAN_TIM0_250K		   1
//#  define CAN_TIM1_250K		0x1c
//#  define CAN_TIM0_500K		   0
//#  define CAN_TIM1_500K		0x1c
//#  define CAN_TIM0_800K		   0
//#  define CAN_TIM1_800K		0x16
//#  define CAN_TIM0_1000K	   0
//#  define CAN_TIM1_1000K	0x14
	
	ULONG sysClkFreq;
	ULONG ui;	
	
	sysClkFreq = cfg->info.xtalfreq/2; /* CDR default to 0 */
	
	cfg->bittiming.oversample = FALSE;
	cfg->bittiming.sjw = 0;
	cfg->bittiming.tseg1 = 4;
	cfg->bittiming.tseg2 = 1;
	
	ui = (cfg->bittiming.tseg1+1)+(cfg->bittiming.tseg2+1)+1;
	
	cfg->bittiming.brp = sysClkFreq/ui/baud-1;
}

static int InitCanChannel(IOChannel_t *pCh, char *fn, int baud, int id, int mask, BOOL ext)
{
	CanPort_t *canPort;
	int fdCtr;
	int fdTx, fdRx;
	UCHAR txchan, rxchan;
	char chfn[128];
	WNCAN_CHNCONFIG chncfg;
	WNCAN_CONFIG devcfg;
	
	pCh->ready=FALSE;
	
	fdCtr=open(fn,O_RDWR,0);
	if(fdCtr==ERROR)
		return -1;
			
	/* Read and update device configuration */
	devcfg.flags = WNCAN_CFG_INFO | WNCAN_CFG_GBLFILTER | WNCAN_CFG_BITTIMING;
	
	if(ioctl(fdCtr, WNCAN_CONFIG_GET, (int)&devcfg) != OK)
		return -1;
	
	devcfg.flags = WNCAN_CFG_GBLFILTER | WNCAN_CFG_BITTIMING;
	devcfg.filter.mask = mask;
	devcfg.filter.extended = ext;
	
	UpdateBaudRate(&devcfg, baud);	
	
	if(ioctl(fdCtr, WNCAN_CONFIG_SET, (int)&devcfg) != OK)
			return -1;
	
	/* Get a Tx channel */
	if(ioctl(fdCtr, WNCAN_TXCHAN_GET, (int)&txchan) != OK)
		return -1;
	
	/* Get a Rx channel */
	if(ioctl(fdCtr, WNCAN_RXCHAN_GET, (int)&rxchan) != OK)
		return -1;
	
	/* Initialize Tx channel */
	sprintf(chfn,"%s/%d",fn,txchan);
	
	fdTx=open(chfn,O_WRONLY,0);
	if(fdTx==ERROR)
		return -1;
	
	ioctl(fdTx, WNCAN_CHN_ENABLE, TRUE);
	
	/* Initialize Rx channel */
	sprintf(chfn,"%s/%d",fn,rxchan);
	
	fdRx=open(chfn,O_RDONLY,0);
	if(fdRx==ERROR)
		return -1;
		
	chncfg.flags = WNCAN_CHNCFG_CHANNEL;
	chncfg.channel.id = id;
	chncfg.channel.extId = ext;
	chncfg.channel.len = 0;
	
	if(ioctl(fdRx, WNCAN_CHNCONFIG_SET, (int)&chncfg) != OK)
		return -1;

	ioctl(fdRx, WNCAN_CHN_ENABLE, TRUE);
	
	/* Start CAN device */
	ioctl(fdCtr, WNCAN_HALT, FALSE);
	
	/* Initialize port data */
	canPort=malloc(sizeof(CanPort_t));
	
	canPort->fdCtr = fdCtr;
	canPort->fdTx = fdTx;
	canPort->fdRx = fdRx;
	canPort->ext = ext;
	
	pCh->handle=canPort;
	sprintf(pCh->name,"%s",fn);
	pCh->read=CanRead;
	pCh->write=CanWrite;
	pCh->ready=TRUE;
	
	return 0;
}


static int LinkIOChannel(IOPeer_t *pPeer, IOChannel_t *pChSrc, IOChannel_t *pChDst)
{
	if(pChSrc->ready==FALSE || pChDst->ready==FALSE)
		return -1;
	
	pPeer->pCIn=pChSrc;
	pPeer->pCOut=pChDst;
	pthread_create(&pPeer->thread,NULL,Dispatch,pPeer);
	return 0;
}

int main(int argc, char **argv)
{
	int fout;
	int fsio[4];
	int fcio[2];
	int fnio[6];
	IOChannel_t chUart[4];
	IOChannel_t chUdp[6];
	IOChannel_t chCan[2];
	IOPeer_t peer[12];
	int i;
	
	fdbg=open("/pcConsole/0",O_RDWR);
	sprintf(msgbuf,"Start Test...\n");
	write(fdbg,msgbuf,strlen(msgbuf));
	
	for(i=0;i<12;i++)
	{
		peer[i].i=i;
	}
	
	InitSerialChannel(&chUart[0], "/tyCo/2", 115200, 1);
	InitSerialChannel(&chUart[1], "/tyCo/3", 115200, 1);
	InitSerialChannel(&chUart[2], "/tyCo/4", 115200, 1);
	InitSerialChannel(&chUart[3], "/tyCo/5", 115200, 1);
	
	InitCanChannel(&chCan[0],"/can/0",1000000,0,0,FALSE);
	InitCanChannel(&chCan[1],"/can/1",1000000,1,0,FALSE);
	
	for(i=0;i<6;i++)
	{
		InitUdpChannel(&chUdp[i],"192.168.0.40",10000+i,"192.168.0.255",10000+i);
	}
	
	LinkIOChannel(&peer[0], &chUart[0], &chUdp[0]);
	LinkIOChannel(&peer[1], &chUdp[0], &chUart[0]);
	LinkIOChannel(&peer[2], &chUart[1], &chUdp[1]);
	LinkIOChannel(&peer[3], &chUdp[1], &chUart[1]);
	LinkIOChannel(&peer[4], &chUart[2], &chUdp[2]);
	LinkIOChannel(&peer[5], &chUdp[2], &chUart[2]);
	LinkIOChannel(&peer[6], &chUart[3], &chUdp[3]);
	LinkIOChannel(&peer[7], &chUdp[3], &chUart[3]);
	LinkIOChannel(&peer[8], &chCan[0], &chUdp[4]);
	LinkIOChannel(&peer[9], &chUdp[4], &chCan[0]);
	LinkIOChannel(&peer[10], &chCan[1], &chUdp[5]);
	LinkIOChannel(&peer[11], &chUdp[5], &chCan[1]);	
	
	pthread_exit(NULL);
	return 0;
}
