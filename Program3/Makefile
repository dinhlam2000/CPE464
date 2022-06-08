# udpCode makefile
# written by Hugh Smith - Feb 2021

CC = gcc
CFLAGS = -g -Wall


SRC = networks.c  gethostbyname.c safeUtil.c
OBJS = networks.o gethostbyname.o safeUtil.o

#uncomment next two lines if your using sendtoErr() library
LIBS += libcpe464.2.21.a -lstdc++ -ldl
CFLAGS += -D__LIBCPE464_

all:  rcopy server

test: test.c udp_helper.o window.o $(OBJS)
	$(CC) $(CFLAGS) -o test test.c udp_helper.o window.o $(OBJS) $(LIBS)

rcopy: rcopy.c udp_helper.o window.o srej.o safeUtil.o $(OBJS) 
	$(CC) $(CFLAGS) -o rcopy rcopy.c udp_helper.o window.o srej.o $(OBJS) $(LIBS)

server: server.c udp_helper.o window.o srej.o safeUtil.o $(OBJS) 
	$(CC) $(CFLAGS) -o server server.c udp_helper.o window.o srej.o $(OBJS) $(LIBS)

%.o: %.c *.h 
	gcc -c $(CFLAGS) $< -o $@ 

cleano:
	rm -f *.o

clean:
	rm -f server rcopy *.o

