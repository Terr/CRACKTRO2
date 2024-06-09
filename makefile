LIBPATH = C:\DEV\TC\LIB
#LIBPATH = C:\DEV\TCPP\LIB


vga9.exe: tro2v1.obj radlib.obj precalc.obj vga.obj
	tlink C:\DEV\TC\LIB\C0L @objects.rsp,vga9.exe,,$(LIBPATH)\EMU $(LIBPATH)\MATHL $(LIBPATH)\CL
	#tlink $(LIBPATH)\C0L radlib.obj pcztfar.obj tro2v1.obj,vga9.exe,,$(LIBPATH)\EMU $(LIBPATH)\MATHL $(LIBPATH)\CL
	#tlink $(LIBPATH)\C0L radlib.obj tro2v1.obj,vga9.exe,,$(LIBPATH)\EMU $(LIBPATH)\MATHL $(LIBPATH)\CL

a.exe: vga9.obj radlib.obj
	tlink LIB\C0L radlib.obj vga9.obj,a.exe,,EMU MATHL CL

radlib.obj: radlib.asm player.asm
	tasm /t /m9 radlib.asm

tro2v1.obj: tro2v1.c defines.h letters.h precalc.h vga.h
	tcc -c -C -ml -O -Z -G -B tro2v1.c
	#bcc -2 -c -C -ml -O -Om -Ol -Oi -Oe -Oa -Z -k- -G tro2v1.c

.c.obj:
	tcc -c -C -ml -O -Z -G $<
