CC=wcc386
CFLAGS=-q -6r -os -zc -bt=nt -dWATCOM_WIN32

LINKER=wlink
LIBARG=wlib.arg

LFLAGS=library wsock32.lib,imcomm.lib system nt

OBJS=bos_signon.obj flap.obj imcomm.obj md5.obj misc.obj packet.obj proxy.obj snac.obj test.obj

.c.obj: .autodepend
	$(CC) $(CFLAGS) $<

test.exe: $(OBJS) $(LIBARG)
	wlib -q -b -c imcomm.lib @wlib.arg
	$(LINKER) $(LFLAGS) name $@ file { test.obj } 

$(LIBARG):
	%create $^@
	for %f in ($(OBJS)) do %append $^@ +- %f

clean: .SYMBOLIC
	- @del *.obj
	- @del *~
	- @del imcomm.lib
	- @del test.exe
	- @del $(LIBARG)
