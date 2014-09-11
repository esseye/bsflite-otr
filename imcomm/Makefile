EXEC=test
#AR=i586-pc-msdosdjgpp-ar
#AR=mingw32-ar
AR=ar
#RANLIB=i586-pc-msdosdjgpp-ranlib
#RANLIB=mingw32-ranlib
RANLIB=ranlib
LIB=libimcomm.a
#CC=i586-pc-msdosdjgpp-gcc
#CC=mingw32-gcc
CC=gcc
#CFLAGS=-g -pipe -DDEBUG
CFLAGS=-g -pipe
RM=rm -f
#CFLAGS+=-I/usr/local/cross-tools/watt/inc
#
# Add -lsocket -lnsl for Solaris
#
LIBS=-limcomm 
#LIBS+=-lwatt -L/usr/local/cross-tools/watt/lib
LDFLAGS=-L.
SRCS=misc.c packet.c flap.c bos_signon.c imcomm.c snac.c md5.c proxy.c ssl.c hexdump.c
OBJECTS=misc.o packet.o flap.o bos_signon.o imcomm.o snac.o md5.o proxy.o ssl.o hexdump.o

$(EXEC):$(OBJECTS) test.o
	$(AR) rcv $(LIB) $(OBJECTS)
	$(RANLIB) $(LIB)
#	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXEC) test.o $(LIBS)

clean:
	$(RM) *.o *~ $(EXEC) $(LIB)

misc.o:		imcomm.h
packet.o:	imcomm.h
flap.o:		imcomm.h
bos_signon.o:	imcomm.h
imcomm.o:	imcomm.h
snac.o:		imcomm.h
md5.o:		imcomm.h
proxy.o:	imcomm.h
