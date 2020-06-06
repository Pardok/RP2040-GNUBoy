prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin

CC = gcc
LD = $(CC)
AS = $(CC)
INSTALL = /usr/bin/install -c

CFLAGS =  -Wall -O3 -I./include
LDFLAGS = $(CFLAGS)  -s
ASFLAGS = $(CFLAGS)

TARGETS =  sdl2gnuboy

SYS_DEFS = -DIS_LITTLE_ENDIAN  -DIS_LINUX
SYS_OBJS = sys/nix/nix.o
SYS_INCS = -I/usr/local/include  -I./sys/nix

SDL_OBJS = sys/sdl2/sdl.o sys/sdl2/sdl-audio.o
SDL_LIBS = -L/usr/lib -lSDL2 -lpthread
SDL_CFLAGS = -I/usr/include/SDL2 -D_GNU_SOURCE=1 -D_REENTRANT


all: $(TARGETS)

XZ_OBJS = lib/xz/xz_crc32.o lib/xz/xz_crc64.o lib/xz/xz_dec_lzma2.o lib/xz/xz_dec_stream.o lib/xz/xz_dec_bcj.o

OBJS = ./src/lcd.o ./src/refresh.o ./src/lcdc.o ./src/palette.o ./src/cpu.o ./src/mem.o ./src/rtc.o ./src/hw.o ./src/sound.o \
	./src/events.o ./src/keytable.o \
	./src/loader.o ./src/save.o ./src/debug.o ./src/emu.o ./src/main.o \
	./src/rccmds.o ./src/rckeys.o ./src/rcvars.o ./src/rcfile.o ./src/exports.o \
	./src/split.o ./src/path.o ./src/inflate.o $(XZ_OBJS)

INCS = -I.

MYCC = $(CC) $(CFLAGS) $(INCS) $(SYS_INCS) $(SYS_DEFS)
MYAS = $(AS) $(ASFLAGS) $(INCS) $(SYS_INCS) $(SYS_DEFS)

main.o: Version

.c.o:
	$(MYCC) -c $< -o $@

.s.o:
	$(MYAS) -c $< -o $@

sdl2gnuboy: $(OBJS) $(SYS_OBJS) $(SDL_OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(SYS_OBJS) $(SDL_OBJS) -o $@ $(SDL_LIBS)

sys/sdl2/sdl.o: sys/sdl2/sdl.c
	$(MYCC) $(SDL_CFLAGS) -c $< -o $@


install: all
	$(INSTALL) -d $(bindir)
	$(INSTALL) -m 755 $(TARGETS) $(bindir)

clean:
	rm -f *gnuboy gnuboy.exe gmon.out *.o sys/*.o sys/*/*.o asm/*/*.o $(OBJS)

distclean: clean
	rm -f config.* sys/nix/config.h Makefile



