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

OBJS = src/cmd.o src/crc.o src/cvar.o src/forward.o src/forward_pending.o src/info.o src/main.o src/mdfour.o \
       src/msg.o src/net_utils.o src/parse.o src/qw.o src/source.o src/source_cmds.o src/sys.o src/build.o src/token.o src/httpsv.o src/httpsv_generate.o \
       src/cl_cmds.o src/fs.o src/ban.o src/udp.o src/sha3.o

qtv: $(OBJS) src/qtv.h src/qconst.h
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@.db -lm
	$(STRIP) $(STRIPFLAGS) $@.db -o $@.bin

qtv.exe: *.c *.h
	$(MAKE) qtv
	mv qtv.bin qtv.exe

clean:
	rm -rf qtv.bin qtv.exe qtv.db *.o
