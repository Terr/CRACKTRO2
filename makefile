LIBPATH = C:\DEV\TC\LIB


vga9.exe: tro2v1.obj radlib.obj pcztfar.obj
	tlink $(LIBPATH)\C0L radlib.obj pcztfar.obj tro2v1.obj,vga9.exe,,$(LIBPATH)\EMU $(LIBPATH)\MATHL $(LIBPATH)\CL

a.exe: vga9.obj radlib.obj
	tlink LIB\C0L radlib.obj vga9.obj,a.exe,,EMU MATHL CL

radlib.obj:
	tasm /t /m9 radlib.asm

tro2v1.obj:
	tcc -c -C -ml -O -Z -G tro2v1.c
