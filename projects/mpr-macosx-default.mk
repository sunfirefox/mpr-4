#
#   mpr-macosx-default.mk -- Makefile to build Multithreaded Portable Runtime for macosx
#

PRODUCT         := mpr
VERSION         := 4.3.0
BUILD_NUMBER    := 0
PROFILE         := default
ARCH            := $(shell uname -m | sed 's/i.86/x86/;s/x86_64/x64/;s/arm.*/arm/;s/mips.*/mips/')
OS              := macosx
CC              := /usr/bin/clang
LD              := /usr/bin/ld
CONFIG          := $(OS)-$(ARCH)-$(PROFILE)
LBIN            := $(CONFIG)/bin

BIT_ROOT_PREFIX       := /
BIT_BASE_PREFIX       := $(BIT_ROOT_PREFIX)/usr/local
BIT_DATA_PREFIX       := $(BIT_ROOT_PREFIX)/
BIT_STATE_PREFIX      := $(BIT_ROOT_PREFIX)/var
BIT_APP_PREFIX        := $(BIT_BASE_PREFIX)/lib/$(PRODUCT)
BIT_VAPP_PREFIX       := $(BIT_APP_PREFIX)/$(VERSION)
BIT_BIN_PREFIX        := $(BIT_ROOT_PREFIX)/usr/local/bin
BIT_INC_PREFIX        := $(BIT_ROOT_PREFIX)/usr/local/include
BIT_LIB_PREFIX        := $(BIT_ROOT_PREFIX)/usr/local/lib
BIT_MAN_PREFIX        := $(BIT_ROOT_PREFIX)/usr/local/share/man
BIT_SBIN_PREFIX       := $(BIT_ROOT_PREFIX)/usr/local/sbin
BIT_ETC_PREFIX        := $(BIT_ROOT_PREFIX)/etc/$(PRODUCT)
BIT_WEB_PREFIX        := $(BIT_ROOT_PREFIX)/var/www/$(PRODUCT)-default
BIT_LOG_PREFIX        := $(BIT_ROOT_PREFIX)/var/log/$(PRODUCT)
BIT_SPOOL_PREFIX      := $(BIT_ROOT_PREFIX)/var/spool/$(PRODUCT)
BIT_CACHE_PREFIX      := $(BIT_ROOT_PREFIX)/var/spool/$(PRODUCT)/cache
BIT_SRC_PREFIX        := $(BIT_ROOT_PREFIX)$(PRODUCT)-$(VERSION)

CFLAGS          += -w
DFLAGS          +=  $(patsubst %,-D%,$(filter BIT_%,$(MAKEFLAGS)))
IFLAGS          += -I$(CONFIG)/inc
LDFLAGS         += '-Wl,-rpath,@executable_path/' '-Wl,-rpath,@loader_path/'
LIBPATHS        += -L$(CONFIG)/bin
LIBS            += -lpthread -lm -ldl

DEBUG           := debug
CFLAGS-debug    := -g
DFLAGS-debug    := -DBIT_DEBUG
LDFLAGS-debug   := -g
DFLAGS-release  := 
CFLAGS-release  := -O2
LDFLAGS-release := 
CFLAGS          += $(CFLAGS-$(DEBUG))
DFLAGS          += $(DFLAGS-$(DEBUG))
LDFLAGS         += $(LDFLAGS-$(DEBUG))

unexport CDPATH

all compile: prep \
        $(CONFIG)/bin/libest.dylib \
        $(CONFIG)/bin/ca.crt \
        $(CONFIG)/bin/benchMpr \
        $(CONFIG)/bin/runProgram \
        $(CONFIG)/bin/testMpr \
        $(CONFIG)/bin/libmpr.dylib \
        $(CONFIG)/bin/libmprssl.dylib \
        $(CONFIG)/bin/manager \
        $(CONFIG)/bin/makerom \
        $(CONFIG)/bin/chargen

.PHONY: prep

prep:
	@if [ "$(CONFIG)" = "" ] ; then echo WARNING: CONFIG not set ; exit 255 ; fi
	@if [ "$(BIT_APP_PREFIX)" = "" ] ; then echo WARNING: BIT_APP_PREFIX not set ; exit 255 ; fi
	@[ ! -x $(CONFIG)/bin ] && mkdir -p $(CONFIG)/bin; true
	@[ ! -x $(CONFIG)/inc ] && mkdir -p $(CONFIG)/inc; true
	@[ ! -x $(CONFIG)/obj ] && mkdir -p $(CONFIG)/obj; true
	@[ ! -f $(CONFIG)/inc/bit.h ] && cp projects/mpr-macosx-default-bit.h $(CONFIG)/inc/bit.h ; true
	@[ ! -f $(CONFIG)/inc/bitos.h ] && cp src/bitos.h $(CONFIG)/inc/bitos.h ; true
	@if ! diff $(CONFIG)/inc/bit.h projects/mpr-macosx-default-bit.h >/dev/null ; then\
		echo cp projects/mpr-macosx-default-bit.h $(CONFIG)/inc/bit.h  ; \
		cp projects/mpr-macosx-default-bit.h $(CONFIG)/inc/bit.h  ; \
	fi; true
clean:
	rm -rf $(CONFIG)/bin/libest.dylib
	rm -rf $(CONFIG)/bin/ca.crt
	rm -rf $(CONFIG)/bin/benchMpr
	rm -rf $(CONFIG)/bin/runProgram
	rm -rf $(CONFIG)/bin/testMpr
	rm -rf $(CONFIG)/bin/libmpr.dylib
	rm -rf $(CONFIG)/bin/libmprssl.dylib
	rm -rf $(CONFIG)/bin/manager
	rm -rf $(CONFIG)/bin/makerom
	rm -rf $(CONFIG)/bin/chargen
	rm -rf $(CONFIG)/obj/removeFiles.o
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
	rm -rf $(CONFIG)/obj/manager.o
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
	rm -rf $(CONFIG)/obj/makerom.o
	rm -rf $(CONFIG)/obj/charGen.o

clobber: clean
	rm -fr ./$(CONFIG)

$(CONFIG)/inc/est.h: 
	mkdir -p "macosx-x64-default/inc"
	cp "src/deps/est/est.h" "macosx-x64-default/inc/est.h"

$(CONFIG)/inc/bit.h: 

$(CONFIG)/inc/bitos.h: \
    $(CONFIG)/inc/bit.h
	mkdir -p "macosx-x64-default/inc"
	cp "src/bitos.h" "macosx-x64-default/inc/bitos.h"

$(CONFIG)/obj/estLib.o: \
    src/deps/est/estLib.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/est.h \
    $(CONFIG)/inc/bitos.h
	$(CC) -c -o $(CONFIG)/obj/estLib.o $(DFLAGS) $(IFLAGS) src/deps/est/estLib.c

$(CONFIG)/bin/libest.dylib: \
    $(CONFIG)/inc/est.h \
    $(CONFIG)/obj/estLib.o
	$(CC) -dynamiclib -o $(CONFIG)/bin/libest.dylib $(LDFLAGS) -compatibility_version 4.3.0 -current_version 4.3.0 $(LIBPATHS) -install_name @rpath/libest.dylib $(CONFIG)/obj/estLib.o $(LIBS)

$(CONFIG)/bin/ca.crt: \
    src/deps/est/ca.crt
	mkdir -p "macosx-x64-default/bin"
	cp "src/deps/est/ca.crt" "macosx-x64-default/bin/ca.crt"

$(CONFIG)/inc/mpr.h: 
	mkdir -p "macosx-x64-default/inc"
	cp "src/mpr.h" "macosx-x64-default/inc/mpr.h"

$(CONFIG)/obj/async.o: \
    src/async.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h \
    $(CONFIG)/inc/bitos.h
	$(CC) -c -o $(CONFIG)/obj/async.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/async.c

$(CONFIG)/obj/atomic.o: \
    src/atomic.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/atomic.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/atomic.c

$(CONFIG)/obj/buf.o: \
    src/buf.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/buf.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/buf.c

$(CONFIG)/obj/cache.o: \
    src/cache.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/cache.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/cache.c

$(CONFIG)/obj/cmd.o: \
    src/cmd.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/cmd.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/cmd.c

$(CONFIG)/obj/cond.o: \
    src/cond.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/cond.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/cond.c

$(CONFIG)/obj/crypt.o: \
    src/crypt.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/crypt.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/crypt.c

$(CONFIG)/obj/disk.o: \
    src/disk.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/disk.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/disk.c

$(CONFIG)/obj/dispatcher.o: \
    src/dispatcher.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/dispatcher.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/dispatcher.c

$(CONFIG)/obj/encode.o: \
    src/encode.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/encode.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/encode.c

$(CONFIG)/obj/epoll.o: \
    src/epoll.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/epoll.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/epoll.c

$(CONFIG)/obj/event.o: \
    src/event.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/event.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/event.c

$(CONFIG)/obj/file.o: \
    src/file.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/file.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/file.c

$(CONFIG)/obj/fs.o: \
    src/fs.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/fs.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/fs.c

$(CONFIG)/obj/hash.o: \
    src/hash.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/hash.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/hash.c

$(CONFIG)/obj/json.o: \
    src/json.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/json.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/json.c

$(CONFIG)/obj/kqueue.o: \
    src/kqueue.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/kqueue.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/kqueue.c

$(CONFIG)/obj/list.o: \
    src/list.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/list.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/list.c

$(CONFIG)/obj/lock.o: \
    src/lock.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/lock.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/lock.c

$(CONFIG)/obj/log.o: \
    src/log.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/log.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/log.c

$(CONFIG)/obj/manager.o: \
    src/manager.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/manager.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/manager.c

$(CONFIG)/obj/mem.o: \
    src/mem.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/mem.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mem.c

$(CONFIG)/obj/mime.o: \
    src/mime.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/mime.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mime.c

$(CONFIG)/obj/mixed.o: \
    src/mixed.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/mixed.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mixed.c

$(CONFIG)/obj/module.o: \
    src/module.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/module.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/module.c

$(CONFIG)/obj/mpr.o: \
    src/mpr.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/mpr.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mpr.c

$(CONFIG)/obj/path.o: \
    src/path.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/path.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/path.c

$(CONFIG)/obj/poll.o: \
    src/poll.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/poll.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/poll.c

$(CONFIG)/obj/posix.o: \
    src/posix.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/posix.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/posix.c

$(CONFIG)/obj/printf.o: \
    src/printf.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/printf.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/printf.c

$(CONFIG)/obj/rom.o: \
    src/rom.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/rom.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/rom.c

$(CONFIG)/obj/select.o: \
    src/select.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/select.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/select.c

$(CONFIG)/obj/signal.o: \
    src/signal.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/signal.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/signal.c

$(CONFIG)/obj/socket.o: \
    src/socket.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/socket.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/socket.c

$(CONFIG)/obj/string.o: \
    src/string.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/string.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/string.c

$(CONFIG)/obj/test.o: \
    src/test.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/test.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/test.c

$(CONFIG)/obj/thread.o: \
    src/thread.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/thread.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/thread.c

$(CONFIG)/obj/time.o: \
    src/time.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/time.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/time.c

$(CONFIG)/obj/vxworks.o: \
    src/vxworks.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/vxworks.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/vxworks.c

$(CONFIG)/obj/wait.o: \
    src/wait.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/wait.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/wait.c

$(CONFIG)/obj/wide.o: \
    src/wide.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/wide.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/wide.c

$(CONFIG)/obj/win.o: \
    src/win.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/win.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/win.c

$(CONFIG)/obj/wince.o: \
    src/wince.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/wince.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/wince.c

$(CONFIG)/obj/xml.o: \
    src/xml.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/xml.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/xml.c

$(CONFIG)/bin/libmpr.dylib: \
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
    $(CONFIG)/obj/manager.o \
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
	$(CC) -dynamiclib -o $(CONFIG)/bin/libmpr.dylib $(LDFLAGS) -compatibility_version 4.3.0 -current_version 4.3.0 $(LIBPATHS) -install_name @rpath/libmpr.dylib $(CONFIG)/obj/async.o $(CONFIG)/obj/atomic.o $(CONFIG)/obj/buf.o $(CONFIG)/obj/cache.o $(CONFIG)/obj/cmd.o $(CONFIG)/obj/cond.o $(CONFIG)/obj/crypt.o $(CONFIG)/obj/disk.o $(CONFIG)/obj/dispatcher.o $(CONFIG)/obj/encode.o $(CONFIG)/obj/epoll.o $(CONFIG)/obj/event.o $(CONFIG)/obj/file.o $(CONFIG)/obj/fs.o $(CONFIG)/obj/hash.o $(CONFIG)/obj/json.o $(CONFIG)/obj/kqueue.o $(CONFIG)/obj/list.o $(CONFIG)/obj/lock.o $(CONFIG)/obj/log.o $(CONFIG)/obj/manager.o $(CONFIG)/obj/mem.o $(CONFIG)/obj/mime.o $(CONFIG)/obj/mixed.o $(CONFIG)/obj/module.o $(CONFIG)/obj/mpr.o $(CONFIG)/obj/path.o $(CONFIG)/obj/poll.o $(CONFIG)/obj/posix.o $(CONFIG)/obj/printf.o $(CONFIG)/obj/rom.o $(CONFIG)/obj/select.o $(CONFIG)/obj/signal.o $(CONFIG)/obj/socket.o $(CONFIG)/obj/string.o $(CONFIG)/obj/test.o $(CONFIG)/obj/thread.o $(CONFIG)/obj/time.o $(CONFIG)/obj/vxworks.o $(CONFIG)/obj/wait.o $(CONFIG)/obj/wide.o $(CONFIG)/obj/win.o $(CONFIG)/obj/wince.o $(CONFIG)/obj/xml.o $(LIBS)

$(CONFIG)/obj/benchMpr.o: \
    test/benchMpr.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h \
    $(CONFIG)/inc/bitos.h
	$(CC) -c -o $(CONFIG)/obj/benchMpr.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/benchMpr.c

$(CONFIG)/bin/benchMpr: \
    $(CONFIG)/bin/libmpr.dylib \
    $(CONFIG)/obj/benchMpr.o
	$(CC) -o $(CONFIG)/bin/benchMpr -arch x86_64 $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/benchMpr.o -lmpr $(LIBS)

$(CONFIG)/obj/runProgram.o: \
    test/runProgram.c\
    $(CONFIG)/inc/bit.h
	$(CC) -c -o $(CONFIG)/obj/runProgram.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/runProgram.c

$(CONFIG)/bin/runProgram: \
    $(CONFIG)/obj/runProgram.o
	$(CC) -o $(CONFIG)/bin/runProgram -arch x86_64 $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/runProgram.o $(LIBS)

$(CONFIG)/obj/est.o: \
    src/ssl/est.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h \
    $(CONFIG)/inc/est.h
	$(CC) -c -o $(CONFIG)/obj/est.o $(CFLAGS) $(DFLAGS) $(IFLAGS) -Isrc/deps/est src/ssl/est.c

$(CONFIG)/obj/matrixssl.o: \
    src/ssl/matrixssl.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/matrixssl.o $(CFLAGS) $(DFLAGS) $(IFLAGS) -Isrc/deps/est src/ssl/matrixssl.c

$(CONFIG)/obj/mocana.o: \
    src/ssl/mocana.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/mocana.o $(CFLAGS) $(DFLAGS) $(IFLAGS) -Isrc/deps/est src/ssl/mocana.c

$(CONFIG)/obj/openssl.o: \
    src/ssl/openssl.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/openssl.o $(CFLAGS) $(DFLAGS) $(IFLAGS) -Isrc/deps/est src/ssl/openssl.c

$(CONFIG)/obj/ssl.o: \
    src/ssl/ssl.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/ssl.o $(CFLAGS) $(DFLAGS) $(IFLAGS) -Isrc/deps/est src/ssl/ssl.c

$(CONFIG)/bin/libmprssl.dylib: \
    $(CONFIG)/bin/libmpr.dylib \
    $(CONFIG)/obj/est.o \
    $(CONFIG)/obj/matrixssl.o \
    $(CONFIG)/obj/mocana.o \
    $(CONFIG)/obj/openssl.o \
    $(CONFIG)/obj/ssl.o
	$(CC) -dynamiclib -o $(CONFIG)/bin/libmprssl.dylib $(LDFLAGS) -compatibility_version 4.3.0 -current_version 4.3.0 $(LIBPATHS) -install_name @rpath/libmprssl.dylib $(CONFIG)/obj/est.o $(CONFIG)/obj/matrixssl.o $(CONFIG)/obj/mocana.o $(CONFIG)/obj/openssl.o $(CONFIG)/obj/ssl.o -lmpr $(LIBS) -lest

$(CONFIG)/obj/testArgv.o: \
    test/testArgv.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testArgv.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testArgv.c

$(CONFIG)/obj/testBuf.o: \
    test/testBuf.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testBuf.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testBuf.c

$(CONFIG)/obj/testCmd.o: \
    test/testCmd.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testCmd.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testCmd.c

$(CONFIG)/obj/testCond.o: \
    test/testCond.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testCond.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testCond.c

$(CONFIG)/obj/testEvent.o: \
    test/testEvent.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testEvent.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testEvent.c

$(CONFIG)/obj/testFile.o: \
    test/testFile.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testFile.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testFile.c

$(CONFIG)/obj/testHash.o: \
    test/testHash.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testHash.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testHash.c

$(CONFIG)/obj/testList.o: \
    test/testList.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testList.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testList.c

$(CONFIG)/obj/testLock.o: \
    test/testLock.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testLock.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testLock.c

$(CONFIG)/obj/testMem.o: \
    test/testMem.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testMem.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testMem.c

$(CONFIG)/obj/testMpr.o: \
    test/testMpr.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testMpr.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testMpr.c

$(CONFIG)/obj/testPath.o: \
    test/testPath.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testPath.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testPath.c

$(CONFIG)/obj/testSocket.o: \
    test/testSocket.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testSocket.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testSocket.c

$(CONFIG)/obj/testSprintf.o: \
    test/testSprintf.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testSprintf.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testSprintf.c

$(CONFIG)/obj/testThread.o: \
    test/testThread.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testThread.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testThread.c

$(CONFIG)/obj/testTime.o: \
    test/testTime.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testTime.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testTime.c

$(CONFIG)/obj/testUnicode.o: \
    test/testUnicode.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/testUnicode.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testUnicode.c

$(CONFIG)/bin/testMpr: \
    $(CONFIG)/bin/libmpr.dylib \
    $(CONFIG)/bin/libmprssl.dylib \
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
	$(CC) -o $(CONFIG)/bin/testMpr -arch x86_64 $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/testArgv.o $(CONFIG)/obj/testBuf.o $(CONFIG)/obj/testCmd.o $(CONFIG)/obj/testCond.o $(CONFIG)/obj/testEvent.o $(CONFIG)/obj/testFile.o $(CONFIG)/obj/testHash.o $(CONFIG)/obj/testList.o $(CONFIG)/obj/testLock.o $(CONFIG)/obj/testMem.o $(CONFIG)/obj/testMpr.o $(CONFIG)/obj/testPath.o $(CONFIG)/obj/testSocket.o $(CONFIG)/obj/testSprintf.o $(CONFIG)/obj/testThread.o $(CONFIG)/obj/testTime.o $(CONFIG)/obj/testUnicode.o -lmprssl -lmpr $(LIBS) -lest

$(CONFIG)/bin/manager: \
    $(CONFIG)/bin/libmpr.dylib \
    $(CONFIG)/obj/manager.o
	$(CC) -o $(CONFIG)/bin/manager -arch x86_64 $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/manager.o -lmpr $(LIBS)

$(CONFIG)/obj/makerom.o: \
    src/utils/makerom.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/makerom.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/utils/makerom.c

$(CONFIG)/bin/makerom: \
    $(CONFIG)/bin/libmpr.dylib \
    $(CONFIG)/obj/makerom.o
	$(CC) -o $(CONFIG)/bin/makerom -arch x86_64 $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/makerom.o -lmpr $(LIBS)

$(CONFIG)/obj/charGen.o: \
    src/utils/charGen.c\
    $(CONFIG)/inc/bit.h \
    $(CONFIG)/inc/mpr.h
	$(CC) -c -o $(CONFIG)/obj/charGen.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/utils/charGen.c

$(CONFIG)/bin/chargen: \
    $(CONFIG)/bin/libmpr.dylib \
    $(CONFIG)/obj/charGen.o
	$(CC) -o $(CONFIG)/bin/chargen -arch x86_64 $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/charGen.o -lmpr $(LIBS)

version: 
	@echo 4.3.0-0

stop: 
	

installBinary: stop


start: 
	

install: stop installBinary start
	

uninstall: stop


