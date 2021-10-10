EXTRACFLAGS=-Wall -O2
CC=gcc $(EXTRACFLAGS)
STRIP=strip

ifdef CONFIG_WINDOWS
    LDFLAGS=-Lmingw32-libs/lib -lwsock32 -lwinmm
    CC=x86_64-w64-mingw32-gcc
    STRIP=x86_64-w64-mingw32-strip
    CFLAGS=-g0 -D_DEBUG -D_WIN32_WINNT=0x0501 -D__USE_MINGW_ANSI_STDIO -Imingw32-libs/include -Wall -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-strict-aliasing -MMD ${EXTRACFLAGS}
endif

STRIPFLAGS=--strip-unneeded --remove-section=.comment

OBJS = cmd.o crc.o cvar.o forward.o forward_pending.o info.o main.o mdfour.o \
       msg.o net_utils.o parse.o qw.o source.o source_cmds.o sys.o build.o token.o httpsv.o httpsv_generate.o \
       cl_cmds.o fs.o ban.o udp.o sha3.o

qtv: $(OBJS) qtv.h qconst.h
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@.db -lm
	$(STRIP) $(STRIPFLAGS) $@.db -o $@.bin

qtv.exe: *.c *.h
	$(MAKE) qtv
	mv qtv.bin qtv.exe

clean:
	rm -rf qtv.bin qtv.exe qtv.db *.o
