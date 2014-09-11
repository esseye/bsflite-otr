CC=wcc386
CFLAGS=-q -6r -os -zc -bt=nt -dWATCOM_WIN32

LINKER=wlink
LINKARG=wlink.arg

LFLAGS=library wsock32.lib,imcomm/imcomm.lib system nt

OBJS=aim.obj away.obj bsf.obj cleaner.obj config.obj conn.obj input.obj log.obj out.obj queue.obj util.obj

.c.obj: .autodepend
	$(CC) $(CFLAGS) $<

bsflite.exe: $(OBJS) $(LINKARG)
	$(LINKER) $(LFLAGS) name $@ file { @wlink.arg } 

$(LINKARG):
	%create $^@
	for %f in ($(OBJS)) do %append $^@ %f

clean: .SYMBOLIC
	- @del *.obj
	- @del *~
	- @del bsflite.exe
	- @del wlink.arg
