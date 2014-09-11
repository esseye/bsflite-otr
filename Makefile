EXEC=bsflite
#EXEC=bsflite.exe
#CC=i586-pc-msdosdjgpp-gcc
#CC=mingw32-gcc
CC=gcc
#
# Add -DDUMP_PROFILE to dump all HTML profiles to a file.
# 	Edit PROFILE_DUMP_PATH in bsf.h to set the path.
# 	(This is useful if you'd like to see profiles properly
# 	 formatted with your web browser.)
#
CFLAGS=-g -pipe
#CFLAGS+=-I/usr/local/cross-tools/watt/inc
#
# Add -lsocket -lnsl for Solaris
# Add -lsocket -lbind for Zeta R1 (and potentially BeOS)
#     (thanks to Brennan Cleveland)
#
LIBS=-limcomm -lotr -lgcrypt -lpthread -lssl -lcrypto
#LIBS+=-lwatt
#LIBS+=-lwsock32
LDFLAGS=-Limcomm/
#LDFLAGS+=-L/usr/local/cross-tools/watt/lib

INSTALL=/usr/bin/install
INSTALL_PREFIX=/usr/local

SOURCES=aim.c away.c bsf.c cleaner.c config.c conn.c input.c log.c otr.c out.c queue.c util.c
OBJECTS=aim.o away.o bsf.o cleaner.o config.o conn.o input.o log.o otr.o out.o queue.o util.o

all:
	(cd imcomm && $(MAKE) $(MFLAGS))
	$(MAKE) $(MFLAGS) $(EXEC)

$(EXEC):$(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXEC) $(OBJECTS) $(LIBS)

clean:
	rm -f *.o *~ $(EXEC)

realclean:
	(cd imcomm && $(MAKE) clean)
	$(MAKE) clean

install:
	$(INSTALL) -s -m 755 bsflite $(INSTALL_PREFIX)/bin
	$(INSTALL) -m 644 bsflite.1 $(INSTALL_PREFIX)/man/man1

aim.o:		bsf.h colors.h imcomm/imcomm.h
away.o:		bsf.h colors.h imcomm/imcomm.h
bsf.o:		bsf.h colors.h imcomm/imcomm.h
cleaner.o:	bsf.h colors.h imcomm/imcomm.h
config.o:	bsf.h colors.h imcomm/imcomm.h
conn.o:		bsf.h colors.h imcomm/imcomm.h
input.o:	bsf.h colors.h imcomm/imcomm.h
log.o:		bsf.h colors.h imcomm/imcomm.h
out.o:		bsf.h colors.h imcomm/imcomm.h
queue.o:	bsf.h colors.h imcomm/imcomm.h
util.o:		bsf.h colors.h imcomm/imcomm.h
$(EXEC):	imcomm/libimcomm.a
