CC=gcc
CFLAGS=-pthread

all: udpsend.exe udpcheck.exe udpdump.exe

udpdump.exe: udpdump.o
	${CC} ${CFLAGS} -o $@ $^

udpcheck.exe: udpcheck.o
	${CC} ${CFLAGS} -o $@ $^

udpsend.exe: udpsend.o
	${CC} ${CFLAGS} -o $@ $^

clean:
	rm udpdump.o udpcheck.o udpsend.o
	rm udpdump.exe udpcheck.exe udpsend.exe

