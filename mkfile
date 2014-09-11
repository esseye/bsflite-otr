</$objtype/mkfile
BIN=/$objtype/bin

TARG=bsflite
CC=pcc
CFLAGS=-I. -B -c -DPLAN9

OFILES= \
	aim.$O\
	away.$O\
	bsf.$O\
	cleaner.$O\
	config.$O\
	conn.$O\
	input.$O\
	log.$O\
	out.$O\
	p9win.$O\
	queue.$O\
	util.$O\
	imcomm/libimcomm.a$O\

HFILES= \
	bsf.h\

</sys/src/cmd/mkone

imcomm/libimcomm.a$O:
	cd imcomm
	mk

clean:
	cd imcomm
	mk clean
	cd ..
	rm -f *.[$OS] [$OS].out $TARG
