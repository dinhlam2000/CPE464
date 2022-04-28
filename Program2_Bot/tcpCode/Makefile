# Makefile for CPE464 tcp test code
# written by Hugh Smith - April 2019

CC= gcc 
CFLAGS= -g -Wall -O0
LIBS = 


all:   cclient server 



myClient: myClient.c networks.o gethostbyname.o cMessage.o pollLib.o safeUtil.o
	$(CC) $(CFLAGS) -o myCclient myClient.c networks.o gethostbyname.o cMessage.o pollLib.o safeUtil.o $(LIBS)

myServer: myServer.c networks.o gethostbyname.o cMessage.o
	$(CC) $(CFLAGS) -o myServer myServer.c networks.o gethostbyname.o cMessage.o pollLib.o safeUtil.o $(LIBS)

myAll: myClient myServer

cclient: cclient.c networks.o gethostbyname.o cMessage.o pollLib.o safeUtil.o sharedStuffs.o
	$(CC) $(CFLAGS) -o cclient cclient.c networks.o gethostbyname.o cMessage.o pollLib.o safeUtil.o sharedStuffs.o $(LIBS)

server: server.c networks.o gethostbyname.o cMessage.o sharedStuffs.o
	$(CC) $(CFLAGS) -o server server.c networks.o gethostbyname.o cMessage.o pollLib.o safeUtil.o sharedStuffs.o $(LIBS)


.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleano:
	rm -f *.o

clean:
	rm -f myServer myCclient cclient server *.o




