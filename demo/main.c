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
#include <errno.h>
#include "can.h"

#define LogMsg(...) \
	do{	sprintf(msgbuf,__VA_ARGS__); \
	write(fdbg,msgbuf,strlen(msgbuf)); \
	}while(0)

#define MAX_CHANNEL	(32)
#define MAX_LINK		(64)

typedef enum {
	UNKNOWN_CHANNEL,
	SERIAL_CHANNEL,
	CAN_CHANNEL,
	UDP_CHANNEL,
}ChannelType_t;

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
	ChannelType_t type;
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
	BOOL running;
}IOPeer_t;

static int fdbg;
static char msgbuf[256];

static IOChannel_t ch[MAX_CHANNEL];
static IOPeer_t peer[MAX_LINK];

static int chIdx=0;
static int lnIdx=0;

static const char *stopMsg="!STOP";

static BOOL stop=FALSE;

static char defaultCfgFile[]="/ata1a/demo.cfg";

static char CfgText[4096]=
"SERIAL 2,115200,1 \n" 
"SERIAL 3,115200,1 \n" 
"SERIAL 4,115200,1 \n" 
"SERIAL 5,115200,1 \n" 
"CAN 0,1000000,0,0,0 \n"
"CAN 1,1000000,0,0,0 \n"
"UDP 10000,192.168.0.40,10000,192.168.0.255 \n"
"UDP 10001,192.168.0.40,10001,192.168.0.255 \n"
"UDP 10002,192.168.0.40,10002,192.168.0.255 \n"
"UDP 10003,192.168.0.40,10003,192.168.0.255 \n"
"UDP 10004,192.168.0.40,10004,192.168.0.255 \n"
"UDP 10005,192.168.0.40,10005,192.168.0.255 \n"
"LINK 0,6 \n"
"LINK 6,0 \n"
"LINK 1,7 \n"
"LINK 7,1 \n"
"LINK 2,8 \n"
"LINK 8,2 \n"
"LINK 3,9 \n"
"LINK 9,3 \n"
"LINK 4,10 \n"
"LINK 10,4 \n"
"LINK 5,11 \n"
"LINK 11,5 \n"
;

static size_t CfgSize;


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
	LogMsg("CH%d(%s -> %s) Start \n",
			pPeer->i,pCIn->name,pCOut->name);
	do{
		n=pCIn->read(pCIn->handle,buf,sizeof(buf)-1);		
		if(n>0){
			buf[n]=0;
			ret=pCOut->write(pCOut->handle,buf,strlen(buf));
			ret=snprintf(msgbuf,sizeof(msgbuf),"CH%d(%s -> %s): %s\n",
					pPeer->i, pCIn->name, pCOut->name, buf);
			ret=write(fdbg,msgbuf,strlen(msgbuf));
			buf[strlen(stopMsg)]=0;
			if(strcmp(buf, stopMsg)==0)
				stop=TRUE;
		}
	}while(n>=0 && stop==FALSE);
	LogMsg("CH%d(%s -> %s) Exit\n",pPeer->i, pCIn->name, pCOut->name);
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
	{
		LogMsg("Open serial port failed with error %d - %s\n",errno, strerror(errno));
		return -1;
	}
	
	ioctl(fd, FIOSETOPTIONS, OPT_LINE);
	//ioctl(fd, FIOSETOPTIONS, OPT_RAW);
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
	
	pCh->type = SERIAL_CHANNEL;
	pCh->handle=serPort;
	pCh->read=SerialRead;
	pCh->write=SerialWrite;
	pCh->ready=TRUE;
	return 0;
}

static void ReleaseSerialChannel(IOChannel_t *pCh)
{
	if(pCh->ready)
	{
		SerialPort_t *serPort = pCh->handle;
		close(serPort->fd);
		free(serPort);
	}
	pCh->handle=NULL;
	pCh->read=NULL;
	pCh->write=NULL;
	pCh->ready=FALSE;
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
	{
		LogMsg("Open socket failed with error %d - %s\n",errno, strerror(errno));
		return fd;
	}
	
	udpPort=malloc(sizeof(UdpPort_t));
	
	udpPort->sa_src.sin_family = AF_INET;
	udpPort->sa_src.sin_addr.s_addr = inet_addr(src_ip);
	udpPort->sa_src.sin_port = htons(src_port);
	udpPort->sa_src_len = sizeof(udpPort->sa_src);
	
	if( bind(fd,(struct sockaddr *)&udpPort->sa_src , udpPort->sa_src_len) < 0){
		LogMsg("Bind failed with error %d - %s\n",errno, strerror(errno));
		close(fd);
		fd = ERROR;
		return fd;
	}
	
	udpPort->sa_dst.sin_family = AF_INET;
	udpPort->sa_dst.sin_addr.s_addr = inet_addr(dst_ip);
	udpPort->sa_dst.sin_port = htons(dst_port);
	udpPort->sa_dst_len = sizeof(udpPort->sa_dst);

	udpPort->fd=fd;
	
	pCh->type = UDP_CHANNEL;
	pCh->handle=udpPort;
	pCh->read=UdpRead;
	pCh->write=UdpWrite;
	pCh->ready = TRUE;
	
	sprintf(pCh->name,"UDP%d",src_port);

	return 0;
}

static void ReleaseUdpChannel(IOChannel_t *pCh)
{
	if(pCh->ready)
	{
		UdpPort_t *udpPort = pCh->handle;
		close(udpPort->fd);
		free(udpPort);
	}
	pCh->handle = NULL;
	pCh->read = NULL;
	pCh->write = NULL;
	pCh->ready = FALSE;
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
	{
		LogMsg("Open CAN device failed with error %d - %s\n",errno,strerror(errno));
		return -1;
	}
			
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
	{
		LogMsg("Open CAN channel failed with error %d - %s\n",errno,strerror(errno));
		return -1;
	}
	
	ioctl(fdTx, WNCAN_CHN_ENABLE, TRUE);
	
	/* Initialize Rx channel */
	sprintf(chfn,"%s/%d",fn,rxchan);
	
	fdRx=open(chfn,O_RDONLY,0);
	if(fdRx==ERROR)
	{
		LogMsg("Open CAN channel failed with error %d - %s\n",errno,strerror(errno));
		return -1;
	}
		
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
	
	pCh->type = CAN_CHANNEL;
	pCh->handle=canPort;
	sprintf(pCh->name,"%s",fn);
	pCh->read=CanRead;
	pCh->write=CanWrite;
	pCh->ready=TRUE;
	
	return 0;
}

static void ReleaseCanChannel(IOChannel_t *pCh)
{	
	if(pCh->ready)
	{
		CanPort_t *canPort=pCh->handle;
		ioctl(canPort->fdCtr, WNCAN_HALT, TRUE);
		close(canPort->fdTx);
		close(canPort->fdRx);
		close(canPort->fdCtr);
		free(canPort);
	}
	pCh->handle=NULL;
	pCh->read=NULL;
	pCh->write=NULL;
	pCh->ready=FALSE;
}

static void ReleaseChannel(IOChannel_t *pCh)
{
	if(pCh->ready)
	{
		switch(pCh->type){
		case SERIAL_CHANNEL:
			ReleaseSerialChannel(pCh);
			break;
		case CAN_CHANNEL:
			ReleaseCanChannel(pCh);
			break;
		case UDP_CHANNEL:
			ReleaseUdpChannel(pCh);
			break;
		}
	}
}

static int LinkIOChannel(IOPeer_t *pPeer, IOChannel_t *pChSrc, IOChannel_t *pChDst)
{
	pPeer->running=FALSE;
	pPeer->pCIn=pChSrc;
	pPeer->pCOut=pChDst;
	
	if(pChSrc->ready==FALSE || pChDst->ready==FALSE)
		return -1;	

	if(pthread_create(&pPeer->thread,NULL,Dispatch,pPeer))
		return -1;
	
	pPeer->running=TRUE;
	return 0;
}

void SigHandler(int signum)
{
	stop=TRUE;
}

int tmp_main(int argc, char **argv)
{
	int fout;
	IOChannel_t ch[12];
	IOPeer_t peer[12];
	int i;
	
	fdbg=open("/pcConsole/0",O_RDWR);
	LogMsg("Start Test...\n");
	
	for(i=0;i<12;i++)
	{
		peer[i].i=i;
		peer[i].running=FALSE;
	}
	
	InitSerialChannel(&ch[0], "/tyCo/2", 115200, 1);
	InitSerialChannel(&ch[1], "/tyCo/3", 115200, 1);
	InitSerialChannel(&ch[2], "/tyCo/4", 115200, 1);
	InitSerialChannel(&ch[3], "/tyCo/5", 115200, 1);
	
	InitCanChannel(&ch[4],"/can/0",1000000,0,0,FALSE);
	InitCanChannel(&ch[5],"/can/1",1000000,1,0,FALSE);
	
	for(i=0;i<6;i++)
	{
		InitUdpChannel(&ch[6+i],"192.168.0.40",10000+i,"192.168.0.255",10000+i);
	}
	
	LinkIOChannel(&peer[0], &ch[4], &ch[10]);
	LinkIOChannel(&peer[1], &ch[10], &ch[4]);
	
	sleep(1);
	
	LogMsg("Test Running...\n");
	
	for(i=0;i<12;i++)
	{
		if(peer[i].running)
		{
			pthread_join(peer[i].thread,NULL);
		}
	}
	
	LogMsg("Stop Test...");
	
	for(i=0;i<12;i++)
	{
		ReleaseChannel(&ch[i]);
	}	
	
	LogMsg("Done\n");
	pthread_exit(NULL);
	return 0;
}


int ParseSerOpt(char *serOpt, size_t size)
{
	int port;
	int baud;
	int parity;
	char fnStr[128];
	if(sscanf(serOpt,"%d,%d,%d",&port,&baud,&parity)!=3)
		return -1;
	
	sprintf(fnStr,"/tyCo/%d",port);
	
	if(InitSerialChannel(&ch[chIdx], fnStr, baud, parity))
		return -1;
	
	chIdx++;
	
	return 0;	
}

int ParseCanOpt(char *canOpt, size_t size)
{
	int port;
	int baud;
	int id;
	int mask;
	int ext;
	char fnStr[128];
	if(sscanf(canOpt,"%d,%d,%d,%d,%d", &port, &baud, &id, &mask, &ext)!=5)
		return -1;
	
	sprintf(fnStr,"/can/%d",port);
	
	if(InitCanChannel(&ch[chIdx], fnStr, baud, id, mask, ext))
		return -1;
	
	chIdx++;
	
	return 0;	
}

int ParseUdpOpt(char *udpOpt, size_t size)
{
	int srcPort;
	int dstPort;
	char srcIp[32];
	char dstIp[32];
	char *pSplit;
	
	for(pSplit=index(udpOpt,',');pSplit;pSplit=index(pSplit,','))
		*pSplit='\n';
	
	pSplit=udpOpt;
	
	if(sscanf(pSplit,"%d",&srcPort)!=1)
		return -1;
	
	pSplit=index(pSplit,'\n');
	if(pSplit==NULL)
		return -1;
	pSplit++;
	
	if(sscanf(pSplit,"%s",srcIp)!=1)
		return -1;
	
	pSplit=index(pSplit,'\n');
	if(pSplit==NULL)
		return -1;
	pSplit++;
	
	if(sscanf(pSplit,"%d",&dstPort)!=1)
		return -1;
	
	pSplit=index(pSplit,'\n');
	if(pSplit==NULL)
		return -1;
	pSplit++;
	
	if(sscanf(pSplit,"%s",dstIp)!=1)
		return -1;

	if(InitUdpChannel(&ch[chIdx], srcIp, srcPort, dstIp, dstPort))
		return -1;
	
	chIdx++;
	
	return 0;		
}

int ParseLinkOpt(char *linkOpt, size_t size)
{
	int srcCh;
	int dstCh;
	if(sscanf(linkOpt,"%d,%d",&srcCh,&dstCh)!=2)
		return -1;
		
	LinkIOChannel(&peer[lnIdx], &ch[srcCh], &ch[dstCh]);
	lnIdx++;
	
	return 0;	
	
}


int ParseLine(char *cfgLine, size_t size)
{
	char typeStr[16];
	char optStr[128];
	if(sscanf(cfgLine,"%s %s",typeStr, optStr)!=2)
		return -1;
	
	if(strcmp(typeStr,"SERIAL")==0)
		if(ParseSerOpt(optStr,strlen(optStr)))
			return -1;
		else
			return 0;
	
	if(strcmp(typeStr,"CAN")==0)
		if(ParseCanOpt(optStr,strlen(optStr)))
			return -1;
		else
			return 0;
	
	if(strcmp(typeStr,"UDP")==0)
		if(ParseUdpOpt(optStr,strlen(optStr)))
			return -1;
		else
			return 0;
		
	if(strcmp(typeStr,"LINK")==0)
		if(ParseLinkOpt(optStr,strlen(optStr)))
			return -1;
		else
			return 0;
	
	return -1;
}


int ParseConfig(char *cfgText, size_t size)
{
	char *pFirst=cfgText;
	char *pLast;
	while(*pFirst=='\n' || *pFirst=='\r' || *pFirst==' ' || *pFirst=='\t')
		pFirst++;	 
	while(*pFirst)
	{
		pLast=index(pFirst,'\n');
		if(pLast==NULL)
			pLast=cfgText+size;
		if(ParseLine(pFirst, pLast-pFirst))
		{
			*pLast=0;
			LogMsg("Failed: %s\n",pFirst);
			return -1;
		}
		pFirst=pLast;
		
		while(*pFirst=='\n' || *pFirst=='\r' || *pFirst==' ' || *pFirst=='\t')
			pFirst++;
	}
	return 0;
}


int main(int argc, char **argv)
{
	int fout;
	int fcfg;
	char *cfgFile;

	int i;
	/* BUG: vxWorks has a limit of 16 open files and can not be changed*/
	/* close some open files to reserve as many file descriptors */
	close(STDIN_FILENO);
	close(STDERR_FILENO);
	close(STDOUT_FILENO);
	fdbg=open("/pcConsole/0",O_RDWR);
	LogMsg("Start Test...\n");
	
	for(i=0;i<MAX_LINK;i++)
	{
		peer[i].i=i;
		peer[i].running=FALSE;
	}
	
	if(argc>2)
		return -1;
	else if(argc==2)
		cfgFile=argv[1];
	else
		cfgFile=defaultCfgFile;		
	
	fcfg=open(cfgFile,O_RDONLY);
	if(fcfg<0)
	{
		LogMsg("%s not found, using defaults...\n", cfgFile);
		CfgSize=strlen(CfgText);
				
	}else{
		CfgSize=read(fcfg,CfgText,sizeof(CfgText));
		if(CfgSize<=0)
		{
			LogMsg("Read %s error, using defaults...\n", cfgFile);
			CfgSize=strlen(CfgText);			
		}else{
			CfgText[CfgSize]=0;
		}
	}
	
	ParseConfig(CfgText, CfgSize);
	
	signal(SIGINT,SigHandler);
	
	sleep(1);
	
	LogMsg("Test Running...\n");
	
	for(i=0;i<lnIdx;i++)
	{
		if(peer[i].running)
		{
			pthread_join(peer[i].thread,NULL);
		}
	}
	
	LogMsg("Stop Test...");
	
	for(i=0;i<chIdx;i++)
	{
		ReleaseChannel(&ch[i]);
	}	
	
	LogMsg("Done\n");
	pthread_exit(NULL);
	return 0;
}
