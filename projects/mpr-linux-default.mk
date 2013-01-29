#
#   mpr-linux-default.mk -- Makefile to build Multithreaded Portable Runtime for linux
#

PRODUCT         ?= mpr
VERSION         ?= 4.3.0
BUILD_NUMBER    ?= 0
PROFILE         ?= default
ARCH            ?= $(shell uname -m | sed 's/i.86/x86/;s/x86_64/x64/;s/arm.*/arm/;s/mips.*/mips/')
OS              ?= linux
CC              ?= /usr/bin/gcc
LD              ?= /usr/bin/ld
CONFIG          ?= $(OS)-$(ARCH)-$(PROFILE)

CFLAGS          += -fPIC   -w
DFLAGS          += -D_REENTRANT -DPIC $(patsubst %,-D%,$(filter BIT_%,$(MAKEFLAGS)))
IFLAGS          += -I$(CONFIG)/inc
LDFLAGS         += '-Wl,--enable-new-dtags' '-Wl,-rpath,$$ORIGIN/' '-Wl,-rpath,$$ORIGIN/../bin' '-rdynamic'
LIBPATHS        += -L$(CONFIG)/bin
LIBS            += -lpthread -lm -lrt -ldl

DEBUG           ?= debug
CFLAGS-debug    := -g
DFLAGS-debug    := -DBIT_DEBUG
LDFLAGS-debug   := -g
DFLAGS-release  := 
CFLAGS-release  := -O2
LDFLAGS-release := 
CFLAGS          += $(CFLAGS-$(DEBUG))
DFLAGS          += $(DFLAGS-$(DEBUG))
LDFLAGS         += $(LDFLAGS-$(DEBUG))

ifeq ($(wildcard $(CONFIG)/inc/.prefixes*),$(CONFIG)/inc/.prefixes)
    include $(CONFIG)/inc/.prefixes
endif

all compile: prep \
        $(CONFIG)/bin/libest.so \
        $(CONFIG)/bin/ca.crt \
        $(CONFIG)/bin/benchMpr \
        $(CONFIG)/bin/runProgram \
        $(CONFIG)/bin/testMpr \
        $(CONFIG)/bin/libmpr.so \
        $(CONFIG)/bin/libmprssl.so \
        $(CONFIG)/bin/manager \
        $(CONFIG)/bin/chargen

.PHONY: prep

prep:
	@if [ "$(CONFIG)" = "" ] ; then echo WARNING: CONFIG not set ; exit 255 ; fi
	@[ ! -x $(CONFIG)/inc ] && mkdir -p $(CONFIG)/inc $(CONFIG)/obj $(CONFIG)/lib $(CONFIG)/bin ; true
	@[ ! -f $(CONFIG)/inc/bit.h ] && cp projects/mpr-$(OS)-$(PROFILE)-bit.h $(CONFIG)/inc/bit.h ; true
	@[ ! -f $(CONFIG)/inc/bitos.h ] && cp src/bitos.h $(CONFIG)/inc/bitos.h ; true
	@if ! diff $(CONFIG)/inc/bit.h projects/mpr-$(OS)-$(PROFILE)-bit.h >/dev/null ; then\
		echo cp projects/mpr-$(OS)-$(PROFILE)-bit.h $(CONFIG)/inc/bit.h  ; \
		cp projects/mpr-$(OS)-$(PROFILE)-bit.h $(CONFIG)/inc/bit.h  ; \
	fi; true
	@echo $(DFLAGS) $(CFLAGS) >projects/.flags

clean:
	rm -rf $(CONFIG)/bin/libest.so
	rm -rf $(CONFIG)/bin/ca.crt
	rm -rf $(CONFIG)/bin/benchMpr
	rm -rf $(CONFIG)/bin/runProgram
	rm -rf $(CONFIG)/bin/testMpr
	rm -rf $(CONFIG)/bin/libmpr.so
	rm -rf $(CONFIG)/bin/libmprssl.so
	rm -rf $(CONFIG)/bin/manager
	rm -rf $(CONFIG)/bin/chargen
	rm -rf $(CONFIG)/obj/estLib.o
	rm -rf $(CONFIG)/obj/benchMpr.o
	rm -rf $(CONFIG)/obj/runProgram.o
	rm -rf $(CONFIG)/obj/testArgv.o
	rm -rf $(CONFIG)/obj/testBuf.o
	rm -rf $(CONFIG)/obj/testCmd.o
	rm -rf $(CONFIG)/obj/testCond.o
	rm -rf $(CONFIG)/obj/testEvent.o
	rm -rf $(CONFIG)/obj/testFile.o
	rm -rf $(CONFIG)/obj/testHash.o
	rm -rf $(CONFIG)/obj/testList.o
	rm -rf $(CONFIG)/obj/testLock.o
	rm -rf $(CONFIG)/obj/testMem.o
	rm -rf $(CONFIG)/obj/testMpr.o
	rm -rf $(CONFIG)/obj/testPath.o
	rm -rf $(CONFIG)/obj/testSocket.o
	rm -rf $(CONFIG)/obj/testSprintf.o
	rm -rf $(CONFIG)/obj/testThread.o
	rm -rf $(CONFIG)/obj/testTime.o
	rm -rf $(CONFIG)/obj/testUnicode.o
	rm -rf $(CONFIG)/obj/async.o
	rm -rf $(CONFIG)/obj/atomic.o
	rm -rf $(CONFIG)/obj/buf.o
	rm -rf $(CONFIG)/obj/cache.o
	rm -rf $(CONFIG)/obj/cmd.o
	rm -rf $(CONFIG)/obj/cond.o
	rm -rf $(CONFIG)/obj/crypt.o
	rm -rf $(CONFIG)/obj/disk.o
	rm -rf $(CONFIG)/obj/dispatcher.o
	rm -rf $(CONFIG)/obj/encode.o
	rm -rf $(CONFIG)/obj/epoll.o
	rm -rf $(CONFIG)/obj/event.o
	rm -rf $(CONFIG)/obj/file.o
	rm -rf $(CONFIG)/obj/fs.o
	rm -rf $(CONFIG)/obj/hash.o
	rm -rf $(CONFIG)/obj/json.o
	rm -rf $(CONFIG)/obj/kqueue.o
	rm -rf $(CONFIG)/obj/list.o
	rm -rf $(CONFIG)/obj/lock.o
	rm -rf $(CONFIG)/obj/log.o
	rm -rf $(CONFIG)/obj/mem.o
	rm -rf $(CONFIG)/obj/mime.o
	rm -rf $(CONFIG)/obj/mixed.o
	rm -rf $(CONFIG)/obj/module.o
	rm -rf $(CONFIG)/obj/mpr.o
	rm -rf $(CONFIG)/obj/path.o
	rm -rf $(CONFIG)/obj/poll.o
	rm -rf $(CONFIG)/obj/posix.o
	rm -rf $(CONFIG)/obj/printf.o
	rm -rf $(CONFIG)/obj/rom.o
	rm -rf $(CONFIG)/obj/select.o
	rm -rf $(CONFIG)/obj/signal.o
	rm -rf $(CONFIG)/obj/socket.o
	rm -rf $(CONFIG)/obj/string.o
	rm -rf $(CONFIG)/obj/test.o
	rm -rf $(CONFIG)/obj/thread.o
	rm -rf $(CONFIG)/obj/time.o
	rm -rf $(CONFIG)/obj/vxworks.o
	rm -rf $(CONFIG)/obj/wait.o
	rm -rf $(CONFIG)/obj/wide.o
	rm -rf $(CONFIG)/obj/win.o
	rm -rf $(CONFIG)/obj/wince.o
	rm -rf $(CONFIG)/obj/xml.o
	rm -rf $(CONFIG)/obj/est.o
	rm -rf $(CONFIG)/obj/matrixssl.o
	rm -rf $(CONFIG)/obj/mocana.o
	rm -rf $(CONFIG)/obj/openssl.o
	rm -rf $(CONFIG)/obj/ssl.o
	rm -rf $(CONFIG)/obj/manager.o
	rm -rf $(CONFIG)/obj/makerom.o
	rm -rf $(CONFIG)/obj/charGen.o

clobber: clean
	rm -fr ./$(CONFIG)

$(CONFIG)/inc/bitos.h:  \
        $(CONFIG)/inc/bit.h
	rm -fr $(CONFIG)/inc/bitos.h
	cp -r src/bitos.h $(CONFIG)/inc/bitos.h

$(CONFIG)/inc/est.h:  \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/bitos.h
	rm -fr $(CONFIG)/inc/est.h
	cp -r src/deps/est/est.h $(CONFIG)/inc/est.h

$(CONFIG)/obj/estLib.o: \
        src/deps/est/estLib.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/est.h
	$(CC) -c -o $(CONFIG)/obj/estLib.o -fPIC $(DFLAGS) -I$(CONFIG)/inc src/deps/est/estLib.c

$(CONFIG)/bin/libest.so:  \
        $(CONFIG)/inc/est.h \
        $(CONFIG)/obj/estLib.o
	$(CC) -shared -o $(CONFIG)/bin/libest.so $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/estLib.o $(LIBS)

$(CONFIG)/bin/ca.crt: src/deps/est/ca.crt
	rm -fr $(CONFIG)/bin/ca.crt
	cp -r src/deps/est/ca.crt $(CONFIG)/bin/ca.crt

$(CONFIG)/inc/mpr.h:  \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/bitos.h
	rm -fr $(CONFIG)/inc/mpr.h
	cp -r src/mpr.h $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/async.o: \
        src/async.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/async.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/async.c

$(CONFIG)/obj/atomic.o: \
        src/atomic.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/atomic.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/atomic.c

$(CONFIG)/obj/buf.o: \
        src/buf.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/buf.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/buf.c

$(CONFIG)/obj/cache.o: \
        src/cache.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/cache.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/cache.c

$(CONFIG)/obj/cmd.o: \
        src/cmd.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/cmd.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/cmd.c

$(CONFIG)/obj/cond.o: \
        src/cond.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/cond.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/cond.c

$(CONFIG)/obj/crypt.o: \
        src/crypt.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/crypt.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/crypt.c

$(CONFIG)/obj/disk.o: \
        src/disk.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/disk.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/disk.c

$(CONFIG)/obj/dispatcher.o: \
        src/dispatcher.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/dispatcher.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/dispatcher.c

$(CONFIG)/obj/encode.o: \
        src/encode.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/encode.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/encode.c

$(CONFIG)/obj/epoll.o: \
        src/epoll.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/epoll.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/epoll.c

$(CONFIG)/obj/event.o: \
        src/event.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/event.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/event.c

$(CONFIG)/obj/file.o: \
        src/file.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/file.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/file.c

$(CONFIG)/obj/fs.o: \
        src/fs.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/fs.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/fs.c

$(CONFIG)/obj/hash.o: \
        src/hash.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/hash.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/hash.c

$(CONFIG)/obj/json.o: \
        src/json.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/json.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/json.c

$(CONFIG)/obj/kqueue.o: \
        src/kqueue.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/kqueue.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/kqueue.c

$(CONFIG)/obj/list.o: \
        src/list.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/list.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/list.c

$(CONFIG)/obj/lock.o: \
        src/lock.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/lock.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/lock.c

$(CONFIG)/obj/log.o: \
        src/log.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/log.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/log.c

$(CONFIG)/obj/mem.o: \
        src/mem.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/mem.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/mem.c

$(CONFIG)/obj/mime.o: \
        src/mime.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/mime.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/mime.c

$(CONFIG)/obj/mixed.o: \
        src/mixed.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/mixed.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/mixed.c

$(CONFIG)/obj/module.o: \
        src/module.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/module.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/module.c

$(CONFIG)/obj/mpr.o: \
        src/mpr.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/mpr.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/mpr.c

$(CONFIG)/obj/path.o: \
        src/path.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/path.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/path.c

$(CONFIG)/obj/poll.o: \
        src/poll.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/poll.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/poll.c

$(CONFIG)/obj/posix.o: \
        src/posix.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/posix.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/posix.c

$(CONFIG)/obj/printf.o: \
        src/printf.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/printf.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/printf.c

$(CONFIG)/obj/rom.o: \
        src/rom.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/rom.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/rom.c

$(CONFIG)/obj/select.o: \
        src/select.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/select.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/select.c

$(CONFIG)/obj/signal.o: \
        src/signal.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/signal.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/signal.c

$(CONFIG)/obj/socket.o: \
        src/socket.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/socket.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/socket.c

$(CONFIG)/obj/string.o: \
        src/string.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/string.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/string.c

$(CONFIG)/obj/test.o: \
        src/test.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/test.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/test.c

$(CONFIG)/obj/thread.o: \
        src/thread.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/thread.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/thread.c

$(CONFIG)/obj/time.o: \
        src/time.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/time.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/time.c

$(CONFIG)/obj/vxworks.o: \
        src/vxworks.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/vxworks.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/vxworks.c

$(CONFIG)/obj/wait.o: \
        src/wait.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/wait.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/wait.c

$(CONFIG)/obj/wide.o: \
        src/wide.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/wide.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/wide.c

$(CONFIG)/obj/win.o: \
        src/win.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/win.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/win.c

$(CONFIG)/obj/wince.o: \
        src/wince.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/wince.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/wince.c

$(CONFIG)/obj/xml.o: \
        src/xml.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/xml.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/xml.c

$(CONFIG)/bin/libmpr.so:  \
        $(CONFIG)/inc/bitos.h \
        $(CONFIG)/inc/mpr.h \
        $(CONFIG)/obj/async.o \
        $(CONFIG)/obj/atomic.o \
        $(CONFIG)/obj/buf.o \
        $(CONFIG)/obj/cache.o \
        $(CONFIG)/obj/cmd.o \
        $(CONFIG)/obj/cond.o \
        $(CONFIG)/obj/crypt.o \
        $(CONFIG)/obj/disk.o \
        $(CONFIG)/obj/dispatcher.o \
        $(CONFIG)/obj/encode.o \
        $(CONFIG)/obj/epoll.o \
        $(CONFIG)/obj/event.o \
        $(CONFIG)/obj/file.o \
        $(CONFIG)/obj/fs.o \
        $(CONFIG)/obj/hash.o \
        $(CONFIG)/obj/json.o \
        $(CONFIG)/obj/kqueue.o \
        $(CONFIG)/obj/list.o \
        $(CONFIG)/obj/lock.o \
        $(CONFIG)/obj/log.o \
        $(CONFIG)/obj/mem.o \
        $(CONFIG)/obj/mime.o \
        $(CONFIG)/obj/mixed.o \
        $(CONFIG)/obj/module.o \
        $(CONFIG)/obj/mpr.o \
        $(CONFIG)/obj/path.o \
        $(CONFIG)/obj/poll.o \
        $(CONFIG)/obj/posix.o \
        $(CONFIG)/obj/printf.o \
        $(CONFIG)/obj/rom.o \
        $(CONFIG)/obj/select.o \
        $(CONFIG)/obj/signal.o \
        $(CONFIG)/obj/socket.o \
        $(CONFIG)/obj/string.o \
        $(CONFIG)/obj/test.o \
        $(CONFIG)/obj/thread.o \
        $(CONFIG)/obj/time.o \
        $(CONFIG)/obj/vxworks.o \
        $(CONFIG)/obj/wait.o \
        $(CONFIG)/obj/wide.o \
        $(CONFIG)/obj/win.o \
        $(CONFIG)/obj/wince.o \
        $(CONFIG)/obj/xml.o
	$(CC) -shared -o $(CONFIG)/bin/libmpr.so $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/async.o $(CONFIG)/obj/atomic.o $(CONFIG)/obj/buf.o $(CONFIG)/obj/cache.o $(CONFIG)/obj/cmd.o $(CONFIG)/obj/cond.o $(CONFIG)/obj/crypt.o $(CONFIG)/obj/disk.o $(CONFIG)/obj/dispatcher.o $(CONFIG)/obj/encode.o $(CONFIG)/obj/epoll.o $(CONFIG)/obj/event.o $(CONFIG)/obj/file.o $(CONFIG)/obj/fs.o $(CONFIG)/obj/hash.o $(CONFIG)/obj/json.o $(CONFIG)/obj/kqueue.o $(CONFIG)/obj/list.o $(CONFIG)/obj/lock.o $(CONFIG)/obj/log.o $(CONFIG)/obj/mem.o $(CONFIG)/obj/mime.o $(CONFIG)/obj/mixed.o $(CONFIG)/obj/module.o $(CONFIG)/obj/mpr.o $(CONFIG)/obj/path.o $(CONFIG)/obj/poll.o $(CONFIG)/obj/posix.o $(CONFIG)/obj/printf.o $(CONFIG)/obj/rom.o $(CONFIG)/obj/select.o $(CONFIG)/obj/signal.o $(CONFIG)/obj/socket.o $(CONFIG)/obj/string.o $(CONFIG)/obj/test.o $(CONFIG)/obj/thread.o $(CONFIG)/obj/time.o $(CONFIG)/obj/vxworks.o $(CONFIG)/obj/wait.o $(CONFIG)/obj/wide.o $(CONFIG)/obj/win.o $(CONFIG)/obj/wince.o $(CONFIG)/obj/xml.o $(LIBS)

$(CONFIG)/obj/benchMpr.o: \
        test/benchMpr.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/benchMpr.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/benchMpr.c

$(CONFIG)/bin/benchMpr:  \
        $(CONFIG)/bin/libmpr.so \
        $(CONFIG)/obj/benchMpr.o
	$(CC) -o $(CONFIG)/bin/benchMpr $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/benchMpr.o -lmpr $(LIBS) -lmpr -lpthread -lm -lrt -ldl $(LDFLAGS)

$(CONFIG)/obj/runProgram.o: \
        test/runProgram.c \
        $(CONFIG)/inc/bit.h
	$(CC) -c -o $(CONFIG)/obj/runProgram.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/runProgram.c

$(CONFIG)/bin/runProgram:  \
        $(CONFIG)/obj/runProgram.o
	$(CC) -o $(CONFIG)/bin/runProgram $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/runProgram.o $(LIBS) -lpthread -lm -lrt -ldl $(LDFLAGS)

$(CONFIG)/obj/est.o: \
        src/ssl/est.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h \
        src/deps/est/est.h
	$(CC) -c -o $(CONFIG)/obj/est.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc -Isrc/deps/est src/ssl/est.c

$(CONFIG)/obj/matrixssl.o: \
        src/ssl/matrixssl.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/matrixssl.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc -Isrc/deps/est src/ssl/matrixssl.c

$(CONFIG)/obj/mocana.o: \
        src/ssl/mocana.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/mocana.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc -Isrc/deps/est src/ssl/mocana.c

$(CONFIG)/obj/openssl.o: \
        src/ssl/openssl.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/openssl.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc -Isrc/deps/est src/ssl/openssl.c

$(CONFIG)/obj/ssl.o: \
        src/ssl/ssl.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/ssl.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc -Isrc/deps/est src/ssl/ssl.c

$(CONFIG)/bin/libmprssl.so:  \
        $(CONFIG)/bin/libmpr.so \
        $(CONFIG)/obj/est.o \
        $(CONFIG)/obj/matrixssl.o \
        $(CONFIG)/obj/mocana.o \
        $(CONFIG)/obj/openssl.o \
        $(CONFIG)/obj/ssl.o
	$(CC) -shared -o $(CONFIG)/bin/libmprssl.so $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/est.o $(CONFIG)/obj/matrixssl.o $(CONFIG)/obj/mocana.o $(CONFIG)/obj/openssl.o $(CONFIG)/obj/ssl.o -lmpr $(LIBS) -lest

$(CONFIG)/obj/testArgv.o: \
        test/testArgv.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testArgv.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testArgv.c

$(CONFIG)/obj/testBuf.o: \
        test/testBuf.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testBuf.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testBuf.c

$(CONFIG)/obj/testCmd.o: \
        test/testCmd.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testCmd.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testCmd.c

$(CONFIG)/obj/testCond.o: \
        test/testCond.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testCond.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testCond.c

$(CONFIG)/obj/testEvent.o: \
        test/testEvent.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testEvent.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testEvent.c

$(CONFIG)/obj/testFile.o: \
        test/testFile.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testFile.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testFile.c

$(CONFIG)/obj/testHash.o: \
        test/testHash.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testHash.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testHash.c

$(CONFIG)/obj/testList.o: \
        test/testList.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testList.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testList.c

$(CONFIG)/obj/testLock.o: \
        test/testLock.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testLock.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testLock.c

$(CONFIG)/obj/testMem.o: \
        test/testMem.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testMem.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testMem.c

$(CONFIG)/obj/testMpr.o: \
        test/testMpr.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testMpr.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testMpr.c

$(CONFIG)/obj/testPath.o: \
        test/testPath.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testPath.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testPath.c

$(CONFIG)/obj/testSocket.o: \
        test/testSocket.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testSocket.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testSocket.c

$(CONFIG)/obj/testSprintf.o: \
        test/testSprintf.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testSprintf.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testSprintf.c

$(CONFIG)/obj/testThread.o: \
        test/testThread.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testThread.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testThread.c

$(CONFIG)/obj/testTime.o: \
        test/testTime.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testTime.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testTime.c

$(CONFIG)/obj/testUnicode.o: \
        test/testUnicode.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testUnicode.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc test/testUnicode.c

$(CONFIG)/bin/testMpr:  \
        $(CONFIG)/bin/libmpr.so \
        $(CONFIG)/bin/libmprssl.so \
        $(CONFIG)/bin/runProgram \
        $(CONFIG)/obj/testArgv.o \
        $(CONFIG)/obj/testBuf.o \
        $(CONFIG)/obj/testCmd.o \
        $(CONFIG)/obj/testCond.o \
        $(CONFIG)/obj/testEvent.o \
        $(CONFIG)/obj/testFile.o \
        $(CONFIG)/obj/testHash.o \
        $(CONFIG)/obj/testList.o \
        $(CONFIG)/obj/testLock.o \
        $(CONFIG)/obj/testMem.o \
        $(CONFIG)/obj/testMpr.o \
        $(CONFIG)/obj/testPath.o \
        $(CONFIG)/obj/testSocket.o \
        $(CONFIG)/obj/testSprintf.o \
        $(CONFIG)/obj/testThread.o \
        $(CONFIG)/obj/testTime.o \
        $(CONFIG)/obj/testUnicode.o
	$(CC) -o $(CONFIG)/bin/testMpr $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/testArgv.o $(CONFIG)/obj/testBuf.o $(CONFIG)/obj/testCmd.o $(CONFIG)/obj/testCond.o $(CONFIG)/obj/testEvent.o $(CONFIG)/obj/testFile.o $(CONFIG)/obj/testHash.o $(CONFIG)/obj/testList.o $(CONFIG)/obj/testLock.o $(CONFIG)/obj/testMem.o $(CONFIG)/obj/testMpr.o $(CONFIG)/obj/testPath.o $(CONFIG)/obj/testSocket.o $(CONFIG)/obj/testSprintf.o $(CONFIG)/obj/testThread.o $(CONFIG)/obj/testTime.o $(CONFIG)/obj/testUnicode.o -lmprssl -lmpr $(LIBS) -lest -lmprssl -lmpr -lpthread -lm -lrt -ldl -lest $(LDFLAGS)

$(CONFIG)/obj/manager.o: \
        src/manager.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/manager.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/manager.c

$(CONFIG)/bin/manager:  \
        $(CONFIG)/bin/libmpr.so \
        $(CONFIG)/obj/manager.o
	$(CC) -o $(CONFIG)/bin/manager $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/manager.o -lmpr $(LIBS) -lmpr -lpthread -lm -lrt -ldl $(LDFLAGS)

$(CONFIG)/obj/charGen.o: \
        src/utils/charGen.c \
        $(CONFIG)/inc/bit.h \
        $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/charGen.o $(CFLAGS) $(DFLAGS) -I$(CONFIG)/inc src/utils/charGen.c

$(CONFIG)/bin/chargen:  \
        $(CONFIG)/bin/libmpr.so \
        $(CONFIG)/obj/charGen.o
	$(CC) -o $(CONFIG)/bin/chargen $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/charGen.o -lmpr $(LIBS) -lmpr -lpthread -lm -lrt -ldl $(LDFLAGS)

version: 
	@echo 4.3.0-0

