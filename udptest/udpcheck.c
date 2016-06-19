#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static struct sockaddr_in sa;
static int sa_len;
static unsigned int idx=0;
static double lastTime;

void usage()
{
	fprintf(stderr,"Usage: udpcheck <port>\n");
}

size_t RecvMsg(int fd, char *buffer, size_t size)
{
	size_t n;
	n=recvfrom(fd,buffer,size,0,
		(struct sockaddr*)&sa, &sa_len);
	if(n>0)
	{
		buffer[n]=0;
		//fprintf(stderr,buffer);
		//puts(buffer);
	}
	return n;
}

void CheckMsg(char *buffer, size_t size)
{
	unsigned int i;
	double t;
	if(sscanf(buffer,"%d:%lf",&i,&t)!=2)
		fprintf(stderr,"Wrong message format\n");
	if(i>idx+1)
	{
		fprintf(stderr,"Lost %d in %lf sec\n", i-idx-1, t-lastTime);
	}
	idx=i;
	lastTime=t;
}


int main(int argc, char **argv)
{
	int fd;
	short src_port=29999;

	if(argc > 2)
	{
		fprintf(stderr,"Incorrect arguments\n");
		usage();
		return -1;
	}
	else if(argc > 1)
	{
		if(sscanf(argv[1],"%d",&src_port)!=1)
		{
			fprintf(stderr,"Incorrect port number\n");
			usage();
			return -1;
		}
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd<0)
	{
		fprintf(stderr,"Error open socket\n");
		return -1;
	}
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(src_port);
	sa_len = sizeof(sa);

	if(bind(fd, (struct sockaddr *)&sa, sa_len) < 0)
		return -1;

	while(1)
	{
		char buffer[2048];
		size_t n;
		n=RecvMsg(fd, buffer, sizeof(buffer));
		CheckMsg(buffer, n);
	}

	return 0;
}
