#
#   mpr-linux-default.mk -- Makefile to build Multithreaded Portable Runtime for linux
#

PRODUCT           := mpr
VERSION           := 4.3.0
BUILD_NUMBER      := 0
PROFILE           := default
ARCH              := $(shell uname -m | sed 's/i.86/x86/;s/x86_64/x64/;s/arm.*/arm/;s/mips.*/mips/')
OS                := linux
CC                := /usr/bin/gcc
LD                := /usr/bin/ld
CONFIG            := $(OS)-$(ARCH)-$(PROFILE)
LBIN              := $(CONFIG)/bin

BIT_PACK_EST      := 1

CFLAGS            += -fPIC   -w
DFLAGS            += -D_REENTRANT -DPIC  $(patsubst %,-D%,$(filter BIT_%,$(MAKEFLAGS))) -DBIT_PACK_EST=$(BIT_PACK_EST) 
IFLAGS            += -I$(CONFIG)/inc
LDFLAGS           += '-Wl,--enable-new-dtags' '-Wl,-rpath,$$ORIGIN/' '-rdynamic'
LIBPATHS          += -L$(CONFIG)/bin
LIBS              += -lpthread -lm -lrt -ldl

DEBUG             := debug
CFLAGS-debug      := -g
DFLAGS-debug      := -DBIT_DEBUG
LDFLAGS-debug     := -g
DFLAGS-release    := 
CFLAGS-release    := -O2
LDFLAGS-release   := 
CFLAGS            += $(CFLAGS-$(DEBUG))
DFLAGS            += $(DFLAGS-$(DEBUG))
LDFLAGS           += $(LDFLAGS-$(DEBUG))

BIT_ROOT_PREFIX   := 
BIT_BASE_PREFIX   := $(BIT_ROOT_PREFIX)/usr/local
BIT_DATA_PREFIX   := $(BIT_ROOT_PREFIX)/
BIT_STATE_PREFIX  := $(BIT_ROOT_PREFIX)/var
BIT_APP_PREFIX    := $(BIT_BASE_PREFIX)/lib/$(PRODUCT)
BIT_VAPP_PREFIX   := $(BIT_APP_PREFIX)/$(VERSION)
BIT_BIN_PREFIX    := $(BIT_ROOT_PREFIX)/usr/local/bin
BIT_INC_PREFIX    := $(BIT_ROOT_PREFIX)/usr/local/include
BIT_LIB_PREFIX    := $(BIT_ROOT_PREFIX)/usr/local/lib
BIT_MAN_PREFIX    := $(BIT_ROOT_PREFIX)/usr/local/share/man
BIT_SBIN_PREFIX   := $(BIT_ROOT_PREFIX)/usr/local/sbin
BIT_ETC_PREFIX    := $(BIT_ROOT_PREFIX)/etc/$(PRODUCT)
BIT_WEB_PREFIX    := $(BIT_ROOT_PREFIX)/var/www/$(PRODUCT)-default
BIT_LOG_PREFIX    := $(BIT_ROOT_PREFIX)/var/log/$(PRODUCT)
BIT_SPOOL_PREFIX  := $(BIT_ROOT_PREFIX)/var/spool/$(PRODUCT)
BIT_CACHE_PREFIX  := $(BIT_ROOT_PREFIX)/var/spool/$(PRODUCT)/cache
BIT_SRC_PREFIX    := $(BIT_ROOT_PREFIX)$(PRODUCT)-$(VERSION)


ifeq ($(BIT_PACK_EST),1)
TARGETS           += $(CONFIG)/bin/libest.so
endif
TARGETS           += $(CONFIG)/bin/ca.crt
TARGETS           += $(CONFIG)/bin/benchMpr
TARGETS           += $(CONFIG)/bin/runProgram
TARGETS           += $(CONFIG)/bin/testMpr
TARGETS           += $(CONFIG)/bin/manager
TARGETS           += $(CONFIG)/bin/makerom
TARGETS           += $(CONFIG)/bin/chargen

unexport CDPATH

ifndef SHOW
.SILENT:
endif

all build compile: prep $(TARGETS)

.PHONY: prep

prep:
	@echo "      [Info] Use "make SHOW=1" to trace executed commands."
	@if [ "$(CONFIG)" = "" ] ; then echo WARNING: CONFIG not set ; exit 255 ; fi
	@if [ "$(BIT_APP_PREFIX)" = "" ] ; then echo WARNING: BIT_APP_PREFIX not set ; exit 255 ; fi
	@[ ! -x $(CONFIG)/bin ] && mkdir -p $(CONFIG)/bin; true
	@[ ! -x $(CONFIG)/inc ] && mkdir -p $(CONFIG)/inc; true
	@[ ! -x $(CONFIG)/obj ] && mkdir -p $(CONFIG)/obj; true
	@[ ! -f $(CONFIG)/inc/bit.h ] && cp projects/mpr-linux-default-bit.h $(CONFIG)/inc/bit.h ; true
	@[ ! -f $(CONFIG)/inc/bitos.h ] && cp src/bitos.h $(CONFIG)/inc/bitos.h ; true
	@if ! diff $(CONFIG)/inc/bit.h projects/mpr-linux-default-bit.h >/dev/null ; then\
		echo cp projects/mpr-linux-default-bit.h $(CONFIG)/inc/bit.h  ; \
		cp projects/mpr-linux-default-bit.h $(CONFIG)/inc/bit.h  ; \
	fi; true

clean:
	rm -f "$(CONFIG)/bin/libest.so"
	rm -f "$(CONFIG)/bin/ca.crt"
	rm -f "$(CONFIG)/bin/benchMpr"
	rm -f "$(CONFIG)/bin/runProgram"
	rm -f "$(CONFIG)/bin/testMpr"
	rm -f "$(CONFIG)/bin/libmpr.so"
	rm -f "$(CONFIG)/bin/libmprssl.so"
	rm -f "$(CONFIG)/bin/manager"
	rm -f "$(CONFIG)/bin/makerom"
	rm -f "$(CONFIG)/bin/chargen"
	rm -f "$(CONFIG)/obj/estLib.o"
	rm -f "$(CONFIG)/obj/benchMpr.o"
	rm -f "$(CONFIG)/obj/runProgram.o"
	rm -f "$(CONFIG)/obj/testArgv.o"
	rm -f "$(CONFIG)/obj/testBuf.o"
	rm -f "$(CONFIG)/obj/testCmd.o"
	rm -f "$(CONFIG)/obj/testCond.o"
	rm -f "$(CONFIG)/obj/testEvent.o"
	rm -f "$(CONFIG)/obj/testFile.o"
	rm -f "$(CONFIG)/obj/testHash.o"
	rm -f "$(CONFIG)/obj/testList.o"
	rm -f "$(CONFIG)/obj/testLock.o"
	rm -f "$(CONFIG)/obj/testMem.o"
	rm -f "$(CONFIG)/obj/testMpr.o"
	rm -f "$(CONFIG)/obj/testPath.o"
	rm -f "$(CONFIG)/obj/testSocket.o"
	rm -f "$(CONFIG)/obj/testSprintf.o"
	rm -f "$(CONFIG)/obj/testThread.o"
	rm -f "$(CONFIG)/obj/testTime.o"
	rm -f "$(CONFIG)/obj/testUnicode.o"
	rm -f "$(CONFIG)/obj/async.o"
	rm -f "$(CONFIG)/obj/atomic.o"
	rm -f "$(CONFIG)/obj/buf.o"
	rm -f "$(CONFIG)/obj/cache.o"
	rm -f "$(CONFIG)/obj/cmd.o"
	rm -f "$(CONFIG)/obj/cond.o"
	rm -f "$(CONFIG)/obj/crypt.o"
	rm -f "$(CONFIG)/obj/disk.o"
	rm -f "$(CONFIG)/obj/dispatcher.o"
	rm -f "$(CONFIG)/obj/encode.o"
	rm -f "$(CONFIG)/obj/epoll.o"
	rm -f "$(CONFIG)/obj/event.o"
	rm -f "$(CONFIG)/obj/file.o"
	rm -f "$(CONFIG)/obj/fs.o"
	rm -f "$(CONFIG)/obj/hash.o"
	rm -f "$(CONFIG)/obj/json.o"
	rm -f "$(CONFIG)/obj/kqueue.o"
	rm -f "$(CONFIG)/obj/list.o"
	rm -f "$(CONFIG)/obj/lock.o"
	rm -f "$(CONFIG)/obj/log.o"
	rm -f "$(CONFIG)/obj/manager.o"
	rm -f "$(CONFIG)/obj/mem.o"
	rm -f "$(CONFIG)/obj/mime.o"
	rm -f "$(CONFIG)/obj/mixed.o"
	rm -f "$(CONFIG)/obj/module.o"
	rm -f "$(CONFIG)/obj/mpr.o"
	rm -f "$(CONFIG)/obj/path.o"
	rm -f "$(CONFIG)/obj/poll.o"
	rm -f "$(CONFIG)/obj/posix.o"
	rm -f "$(CONFIG)/obj/printf.o"
	rm -f "$(CONFIG)/obj/rom.o"
	rm -f "$(CONFIG)/obj/select.o"
	rm -f "$(CONFIG)/obj/signal.o"
	rm -f "$(CONFIG)/obj/socket.o"
	rm -f "$(CONFIG)/obj/string.o"
	rm -f "$(CONFIG)/obj/test.o"
	rm -f "$(CONFIG)/obj/thread.o"
	rm -f "$(CONFIG)/obj/time.o"
	rm -f "$(CONFIG)/obj/vxworks.o"
	rm -f "$(CONFIG)/obj/wait.o"
	rm -f "$(CONFIG)/obj/wide.o"
	rm -f "$(CONFIG)/obj/win.o"
	rm -f "$(CONFIG)/obj/wince.o"
	rm -f "$(CONFIG)/obj/xml.o"
	rm -f "$(CONFIG)/obj/est.o"
	rm -f "$(CONFIG)/obj/matrixssl.o"
	rm -f "$(CONFIG)/obj/nanossl.o"
	rm -f "$(CONFIG)/obj/openssl.o"
	rm -f "$(CONFIG)/obj/ssl.o"
	rm -f "$(CONFIG)/obj/makerom.o"
	rm -f "$(CONFIG)/obj/charGen.o"

clobber: clean
	rm -fr ./$(CONFIG)



#
#   version
#
version: $(DEPS_1)
	@echo NN 4.3.0-0

#
#   est.h
#
$(CONFIG)/inc/est.h: $(DEPS_2)
	@echo '      [Copy] $(CONFIG)/inc/est.h'
	mkdir -p "$(CONFIG)/inc"
	cp "src/deps/est/est.h" "$(CONFIG)/inc/est.h"

#
#   bit.h
#
$(CONFIG)/inc/bit.h: $(DEPS_3)
	@echo '      [Copy] $(CONFIG)/inc/bit.h'

#
#   bitos.h
#
DEPS_4 += $(CONFIG)/inc/bit.h

$(CONFIG)/inc/bitos.h: $(DEPS_4)
	@echo '      [Copy] $(CONFIG)/inc/bitos.h'
	mkdir -p "$(CONFIG)/inc"
	cp "src/bitos.h" "$(CONFIG)/inc/bitos.h"

#
#   estLib.o
#
DEPS_5 += $(CONFIG)/inc/bit.h
DEPS_5 += $(CONFIG)/inc/est.h
DEPS_5 += $(CONFIG)/inc/bitos.h

$(CONFIG)/obj/estLib.o: \
    src/deps/est/estLib.c $(DEPS_5)
	@echo '   [Compile] src/deps/est/estLib.c'
	$(CC) -c -o $(CONFIG)/obj/estLib.o -fPIC $(DFLAGS) $(IFLAGS) src/deps/est/estLib.c

ifeq ($(BIT_PACK_EST),1)
#
#   libest
#
DEPS_6 += $(CONFIG)/inc/est.h
DEPS_6 += $(CONFIG)/obj/estLib.o

$(CONFIG)/bin/libest.so: $(DEPS_6)
	@echo '      [Link] libest'
	$(CC) -shared -o $(CONFIG)/bin/libest.so $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/estLib.o $(LIBS)
endif

#
#   ca-crt
#
DEPS_7 += src/deps/est/ca.crt

$(CONFIG)/bin/ca.crt: $(DEPS_7)
	@echo '      [Copy] $(CONFIG)/bin/ca.crt'
	mkdir -p "$(CONFIG)/bin"
	cp "src/deps/est/ca.crt" "$(CONFIG)/bin/ca.crt"

#
#   mpr.h
#
$(CONFIG)/inc/mpr.h: $(DEPS_8)
	@echo '      [Copy] $(CONFIG)/inc/mpr.h'
	mkdir -p "$(CONFIG)/inc"
	cp "src/mpr.h" "$(CONFIG)/inc/mpr.h"

#
#   async.o
#
DEPS_9 += $(CONFIG)/inc/bit.h
DEPS_9 += $(CONFIG)/inc/mpr.h
DEPS_9 += $(CONFIG)/inc/bitos.h

$(CONFIG)/obj/async.o: \
    src/async.c $(DEPS_9)
	@echo '   [Compile] src/async.c'
	$(CC) -c -o $(CONFIG)/obj/async.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/async.c

#
#   atomic.o
#
DEPS_10 += $(CONFIG)/inc/bit.h
DEPS_10 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/atomic.o: \
    src/atomic.c $(DEPS_10)
	@echo '   [Compile] src/atomic.c'
	$(CC) -c -o $(CONFIG)/obj/atomic.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/atomic.c

#
#   buf.o
#
DEPS_11 += $(CONFIG)/inc/bit.h
DEPS_11 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/buf.o: \
    src/buf.c $(DEPS_11)
	@echo '   [Compile] src/buf.c'
	$(CC) -c -o $(CONFIG)/obj/buf.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/buf.c

#
#   cache.o
#
DEPS_12 += $(CONFIG)/inc/bit.h
DEPS_12 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/cache.o: \
    src/cache.c $(DEPS_12)
	@echo '   [Compile] src/cache.c'
	$(CC) -c -o $(CONFIG)/obj/cache.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/cache.c

#
#   cmd.o
#
DEPS_13 += $(CONFIG)/inc/bit.h
DEPS_13 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/cmd.o: \
    src/cmd.c $(DEPS_13)
	@echo '   [Compile] src/cmd.c'
	$(CC) -c -o $(CONFIG)/obj/cmd.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/cmd.c

#
#   cond.o
#
DEPS_14 += $(CONFIG)/inc/bit.h
DEPS_14 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/cond.o: \
    src/cond.c $(DEPS_14)
	@echo '   [Compile] src/cond.c'
	$(CC) -c -o $(CONFIG)/obj/cond.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/cond.c

#
#   crypt.o
#
DEPS_15 += $(CONFIG)/inc/bit.h
DEPS_15 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/crypt.o: \
    src/crypt.c $(DEPS_15)
	@echo '   [Compile] src/crypt.c'
	$(CC) -c -o $(CONFIG)/obj/crypt.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/crypt.c

#
#   disk.o
#
DEPS_16 += $(CONFIG)/inc/bit.h
DEPS_16 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/disk.o: \
    src/disk.c $(DEPS_16)
	@echo '   [Compile] src/disk.c'
	$(CC) -c -o $(CONFIG)/obj/disk.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/disk.c

#
#   dispatcher.o
#
DEPS_17 += $(CONFIG)/inc/bit.h
DEPS_17 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/dispatcher.o: \
    src/dispatcher.c $(DEPS_17)
	@echo '   [Compile] src/dispatcher.c'
	$(CC) -c -o $(CONFIG)/obj/dispatcher.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/dispatcher.c

#
#   encode.o
#
DEPS_18 += $(CONFIG)/inc/bit.h
DEPS_18 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/encode.o: \
    src/encode.c $(DEPS_18)
	@echo '   [Compile] src/encode.c'
	$(CC) -c -o $(CONFIG)/obj/encode.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/encode.c

#
#   epoll.o
#
DEPS_19 += $(CONFIG)/inc/bit.h
DEPS_19 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/epoll.o: \
    src/epoll.c $(DEPS_19)
	@echo '   [Compile] src/epoll.c'
	$(CC) -c -o $(CONFIG)/obj/epoll.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/epoll.c

#
#   event.o
#
DEPS_20 += $(CONFIG)/inc/bit.h
DEPS_20 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/event.o: \
    src/event.c $(DEPS_20)
	@echo '   [Compile] src/event.c'
	$(CC) -c -o $(CONFIG)/obj/event.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/event.c

#
#   file.o
#
DEPS_21 += $(CONFIG)/inc/bit.h
DEPS_21 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/file.o: \
    src/file.c $(DEPS_21)
	@echo '   [Compile] src/file.c'
	$(CC) -c -o $(CONFIG)/obj/file.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/file.c

#
#   fs.o
#
DEPS_22 += $(CONFIG)/inc/bit.h
DEPS_22 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/fs.o: \
    src/fs.c $(DEPS_22)
	@echo '   [Compile] src/fs.c'
	$(CC) -c -o $(CONFIG)/obj/fs.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/fs.c

#
#   hash.o
#
DEPS_23 += $(CONFIG)/inc/bit.h
DEPS_23 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/hash.o: \
    src/hash.c $(DEPS_23)
	@echo '   [Compile] src/hash.c'
	$(CC) -c -o $(CONFIG)/obj/hash.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/hash.c

#
#   json.o
#
DEPS_24 += $(CONFIG)/inc/bit.h
DEPS_24 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/json.o: \
    src/json.c $(DEPS_24)
	@echo '   [Compile] src/json.c'
	$(CC) -c -o $(CONFIG)/obj/json.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/json.c

#
#   kqueue.o
#
DEPS_25 += $(CONFIG)/inc/bit.h
DEPS_25 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/kqueue.o: \
    src/kqueue.c $(DEPS_25)
	@echo '   [Compile] src/kqueue.c'
	$(CC) -c -o $(CONFIG)/obj/kqueue.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/kqueue.c

#
#   list.o
#
DEPS_26 += $(CONFIG)/inc/bit.h
DEPS_26 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/list.o: \
    src/list.c $(DEPS_26)
	@echo '   [Compile] src/list.c'
	$(CC) -c -o $(CONFIG)/obj/list.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/list.c

#
#   lock.o
#
DEPS_27 += $(CONFIG)/inc/bit.h
DEPS_27 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/lock.o: \
    src/lock.c $(DEPS_27)
	@echo '   [Compile] src/lock.c'
	$(CC) -c -o $(CONFIG)/obj/lock.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/lock.c

#
#   log.o
#
DEPS_28 += $(CONFIG)/inc/bit.h
DEPS_28 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/log.o: \
    src/log.c $(DEPS_28)
	@echo '   [Compile] src/log.c'
	$(CC) -c -o $(CONFIG)/obj/log.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/log.c

#
#   manager.o
#
DEPS_29 += $(CONFIG)/inc/bit.h
DEPS_29 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/manager.o: \
    src/manager.c $(DEPS_29)
	@echo '   [Compile] src/manager.c'
	$(CC) -c -o $(CONFIG)/obj/manager.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/manager.c

#
#   mem.o
#
DEPS_30 += $(CONFIG)/inc/bit.h
DEPS_30 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mem.o: \
    src/mem.c $(DEPS_30)
	@echo '   [Compile] src/mem.c'
	$(CC) -c -o $(CONFIG)/obj/mem.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mem.c

#
#   mime.o
#
DEPS_31 += $(CONFIG)/inc/bit.h
DEPS_31 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mime.o: \
    src/mime.c $(DEPS_31)
	@echo '   [Compile] src/mime.c'
	$(CC) -c -o $(CONFIG)/obj/mime.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mime.c

#
#   mixed.o
#
DEPS_32 += $(CONFIG)/inc/bit.h
DEPS_32 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mixed.o: \
    src/mixed.c $(DEPS_32)
	@echo '   [Compile] src/mixed.c'
	$(CC) -c -o $(CONFIG)/obj/mixed.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mixed.c

#
#   module.o
#
DEPS_33 += $(CONFIG)/inc/bit.h
DEPS_33 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/module.o: \
    src/module.c $(DEPS_33)
	@echo '   [Compile] src/module.c'
	$(CC) -c -o $(CONFIG)/obj/module.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/module.c

#
#   mpr.o
#
DEPS_34 += $(CONFIG)/inc/bit.h
DEPS_34 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mpr.o: \
    src/mpr.c $(DEPS_34)
	@echo '   [Compile] src/mpr.c'
	$(CC) -c -o $(CONFIG)/obj/mpr.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mpr.c

#
#   path.o
#
DEPS_35 += $(CONFIG)/inc/bit.h
DEPS_35 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/path.o: \
    src/path.c $(DEPS_35)
	@echo '   [Compile] src/path.c'
	$(CC) -c -o $(CONFIG)/obj/path.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/path.c

#
#   poll.o
#
DEPS_36 += $(CONFIG)/inc/bit.h
DEPS_36 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/poll.o: \
    src/poll.c $(DEPS_36)
	@echo '   [Compile] src/poll.c'
	$(CC) -c -o $(CONFIG)/obj/poll.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/poll.c

#
#   posix.o
#
DEPS_37 += $(CONFIG)/inc/bit.h
DEPS_37 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/posix.o: \
    src/posix.c $(DEPS_37)
	@echo '   [Compile] src/posix.c'
	$(CC) -c -o $(CONFIG)/obj/posix.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/posix.c

#
#   printf.o
#
DEPS_38 += $(CONFIG)/inc/bit.h
DEPS_38 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/printf.o: \
    src/printf.c $(DEPS_38)
	@echo '   [Compile] src/printf.c'
	$(CC) -c -o $(CONFIG)/obj/printf.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/printf.c

#
#   rom.o
#
DEPS_39 += $(CONFIG)/inc/bit.h
DEPS_39 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/rom.o: \
    src/rom.c $(DEPS_39)
	@echo '   [Compile] src/rom.c'
	$(CC) -c -o $(CONFIG)/obj/rom.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/rom.c

#
#   select.o
#
DEPS_40 += $(CONFIG)/inc/bit.h
DEPS_40 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/select.o: \
    src/select.c $(DEPS_40)
	@echo '   [Compile] src/select.c'
	$(CC) -c -o $(CONFIG)/obj/select.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/select.c

#
#   signal.o
#
DEPS_41 += $(CONFIG)/inc/bit.h
DEPS_41 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/signal.o: \
    src/signal.c $(DEPS_41)
	@echo '   [Compile] src/signal.c'
	$(CC) -c -o $(CONFIG)/obj/signal.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/signal.c

#
#   socket.o
#
DEPS_42 += $(CONFIG)/inc/bit.h
DEPS_42 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/socket.o: \
    src/socket.c $(DEPS_42)
	@echo '   [Compile] src/socket.c'
	$(CC) -c -o $(CONFIG)/obj/socket.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/socket.c

#
#   string.o
#
DEPS_43 += $(CONFIG)/inc/bit.h
DEPS_43 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/string.o: \
    src/string.c $(DEPS_43)
	@echo '   [Compile] src/string.c'
	$(CC) -c -o $(CONFIG)/obj/string.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/string.c

#
#   test.o
#
DEPS_44 += $(CONFIG)/inc/bit.h
DEPS_44 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/test.o: \
    src/test.c $(DEPS_44)
	@echo '   [Compile] src/test.c'
	$(CC) -c -o $(CONFIG)/obj/test.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/test.c

#
#   thread.o
#
DEPS_45 += $(CONFIG)/inc/bit.h
DEPS_45 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/thread.o: \
    src/thread.c $(DEPS_45)
	@echo '   [Compile] src/thread.c'
	$(CC) -c -o $(CONFIG)/obj/thread.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/thread.c

#
#   time.o
#
DEPS_46 += $(CONFIG)/inc/bit.h
DEPS_46 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/time.o: \
    src/time.c $(DEPS_46)
	@echo '   [Compile] src/time.c'
	$(CC) -c -o $(CONFIG)/obj/time.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/time.c

#
#   vxworks.o
#
DEPS_47 += $(CONFIG)/inc/bit.h
DEPS_47 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/vxworks.o: \
    src/vxworks.c $(DEPS_47)
	@echo '   [Compile] src/vxworks.c'
	$(CC) -c -o $(CONFIG)/obj/vxworks.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/vxworks.c

#
#   wait.o
#
DEPS_48 += $(CONFIG)/inc/bit.h
DEPS_48 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wait.o: \
    src/wait.c $(DEPS_48)
	@echo '   [Compile] src/wait.c'
	$(CC) -c -o $(CONFIG)/obj/wait.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/wait.c

#
#   wide.o
#
DEPS_49 += $(CONFIG)/inc/bit.h
DEPS_49 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wide.o: \
    src/wide.c $(DEPS_49)
	@echo '   [Compile] src/wide.c'
	$(CC) -c -o $(CONFIG)/obj/wide.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/wide.c

#
#   win.o
#
DEPS_50 += $(CONFIG)/inc/bit.h
DEPS_50 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/win.o: \
    src/win.c $(DEPS_50)
	@echo '   [Compile] src/win.c'
	$(CC) -c -o $(CONFIG)/obj/win.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/win.c

#
#   wince.o
#
DEPS_51 += $(CONFIG)/inc/bit.h
DEPS_51 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wince.o: \
    src/wince.c $(DEPS_51)
	@echo '   [Compile] src/wince.c'
	$(CC) -c -o $(CONFIG)/obj/wince.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/wince.c

#
#   xml.o
#
DEPS_52 += $(CONFIG)/inc/bit.h
DEPS_52 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/xml.o: \
    src/xml.c $(DEPS_52)
	@echo '   [Compile] src/xml.c'
	$(CC) -c -o $(CONFIG)/obj/xml.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/xml.c

#
#   libmpr
#
DEPS_53 += $(CONFIG)/inc/bitos.h
DEPS_53 += $(CONFIG)/inc/mpr.h
DEPS_53 += $(CONFIG)/obj/async.o
DEPS_53 += $(CONFIG)/obj/atomic.o
DEPS_53 += $(CONFIG)/obj/buf.o
DEPS_53 += $(CONFIG)/obj/cache.o
DEPS_53 += $(CONFIG)/obj/cmd.o
DEPS_53 += $(CONFIG)/obj/cond.o
DEPS_53 += $(CONFIG)/obj/crypt.o
DEPS_53 += $(CONFIG)/obj/disk.o
DEPS_53 += $(CONFIG)/obj/dispatcher.o
DEPS_53 += $(CONFIG)/obj/encode.o
DEPS_53 += $(CONFIG)/obj/epoll.o
DEPS_53 += $(CONFIG)/obj/event.o
DEPS_53 += $(CONFIG)/obj/file.o
DEPS_53 += $(CONFIG)/obj/fs.o
DEPS_53 += $(CONFIG)/obj/hash.o
DEPS_53 += $(CONFIG)/obj/json.o
DEPS_53 += $(CONFIG)/obj/kqueue.o
DEPS_53 += $(CONFIG)/obj/list.o
DEPS_53 += $(CONFIG)/obj/lock.o
DEPS_53 += $(CONFIG)/obj/log.o
DEPS_53 += $(CONFIG)/obj/manager.o
DEPS_53 += $(CONFIG)/obj/mem.o
DEPS_53 += $(CONFIG)/obj/mime.o
DEPS_53 += $(CONFIG)/obj/mixed.o
DEPS_53 += $(CONFIG)/obj/module.o
DEPS_53 += $(CONFIG)/obj/mpr.o
DEPS_53 += $(CONFIG)/obj/path.o
DEPS_53 += $(CONFIG)/obj/poll.o
DEPS_53 += $(CONFIG)/obj/posix.o
DEPS_53 += $(CONFIG)/obj/printf.o
DEPS_53 += $(CONFIG)/obj/rom.o
DEPS_53 += $(CONFIG)/obj/select.o
DEPS_53 += $(CONFIG)/obj/signal.o
DEPS_53 += $(CONFIG)/obj/socket.o
DEPS_53 += $(CONFIG)/obj/string.o
DEPS_53 += $(CONFIG)/obj/test.o
DEPS_53 += $(CONFIG)/obj/thread.o
DEPS_53 += $(CONFIG)/obj/time.o
DEPS_53 += $(CONFIG)/obj/vxworks.o
DEPS_53 += $(CONFIG)/obj/wait.o
DEPS_53 += $(CONFIG)/obj/wide.o
DEPS_53 += $(CONFIG)/obj/win.o
DEPS_53 += $(CONFIG)/obj/wince.o
DEPS_53 += $(CONFIG)/obj/xml.o

$(CONFIG)/bin/libmpr.so: $(DEPS_53)
	@echo '      [Link] libmpr'
	$(CC) -shared -o $(CONFIG)/bin/libmpr.so $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/async.o $(CONFIG)/obj/atomic.o $(CONFIG)/obj/buf.o $(CONFIG)/obj/cache.o $(CONFIG)/obj/cmd.o $(CONFIG)/obj/cond.o $(CONFIG)/obj/crypt.o $(CONFIG)/obj/disk.o $(CONFIG)/obj/dispatcher.o $(CONFIG)/obj/encode.o $(CONFIG)/obj/epoll.o $(CONFIG)/obj/event.o $(CONFIG)/obj/file.o $(CONFIG)/obj/fs.o $(CONFIG)/obj/hash.o $(CONFIG)/obj/json.o $(CONFIG)/obj/kqueue.o $(CONFIG)/obj/list.o $(CONFIG)/obj/lock.o $(CONFIG)/obj/log.o $(CONFIG)/obj/manager.o $(CONFIG)/obj/mem.o $(CONFIG)/obj/mime.o $(CONFIG)/obj/mixed.o $(CONFIG)/obj/module.o $(CONFIG)/obj/mpr.o $(CONFIG)/obj/path.o $(CONFIG)/obj/poll.o $(CONFIG)/obj/posix.o $(CONFIG)/obj/printf.o $(CONFIG)/obj/rom.o $(CONFIG)/obj/select.o $(CONFIG)/obj/signal.o $(CONFIG)/obj/socket.o $(CONFIG)/obj/string.o $(CONFIG)/obj/test.o $(CONFIG)/obj/thread.o $(CONFIG)/obj/time.o $(CONFIG)/obj/vxworks.o $(CONFIG)/obj/wait.o $(CONFIG)/obj/wide.o $(CONFIG)/obj/win.o $(CONFIG)/obj/wince.o $(CONFIG)/obj/xml.o $(LIBS)

#
#   benchMpr.o
#
DEPS_54 += $(CONFIG)/inc/bit.h
DEPS_54 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/benchMpr.o: \
    test/benchMpr.c $(DEPS_54)
	@echo '   [Compile] test/benchMpr.c'
	$(CC) -c -o $(CONFIG)/obj/benchMpr.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/benchMpr.c

#
#   benchMpr
#
DEPS_55 += $(CONFIG)/bin/libmpr.so
DEPS_55 += $(CONFIG)/obj/benchMpr.o

LIBS_55 += -lmpr

$(CONFIG)/bin/benchMpr: $(DEPS_55)
	@echo '      [Link] benchMpr'
	$(CC) -o $(CONFIG)/bin/benchMpr $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/benchMpr.o $(LIBS_55) $(LIBS_55) $(LIBS) $(LDFLAGS)

#
#   runProgram.o
#
DEPS_56 += $(CONFIG)/inc/bit.h

$(CONFIG)/obj/runProgram.o: \
    test/runProgram.c $(DEPS_56)
	@echo '   [Compile] test/runProgram.c'
	$(CC) -c -o $(CONFIG)/obj/runProgram.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/runProgram.c

#
#   runProgram
#
DEPS_57 += $(CONFIG)/obj/runProgram.o

$(CONFIG)/bin/runProgram: $(DEPS_57)
	@echo '      [Link] runProgram'
	$(CC) -o $(CONFIG)/bin/runProgram $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/runProgram.o $(LIBS) $(LDFLAGS)

#
#   est.h
#
src/deps/est/est.h: $(DEPS_58)
	@echo '      [Copy] src/deps/est/est.h'

#
#   est.o
#
DEPS_59 += $(CONFIG)/inc/bit.h
DEPS_59 += $(CONFIG)/inc/mpr.h
DEPS_59 += src/deps/est/est.h
DEPS_59 += $(CONFIG)/inc/bitos.h

$(CONFIG)/obj/est.o: \
    src/ssl/est.c $(DEPS_59)
	@echo '   [Compile] src/ssl/est.c'
	$(CC) -c -o $(CONFIG)/obj/est.o $(CFLAGS) $(DFLAGS) $(IFLAGS) -Isrc/deps/est src/ssl/est.c

#
#   matrixssl.o
#
DEPS_60 += $(CONFIG)/inc/bit.h
DEPS_60 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/matrixssl.o: \
    src/ssl/matrixssl.c $(DEPS_60)
	@echo '   [Compile] src/ssl/matrixssl.c'
	$(CC) -c -o $(CONFIG)/obj/matrixssl.o $(CFLAGS) $(DFLAGS) $(IFLAGS) -Isrc/deps/est src/ssl/matrixssl.c

#
#   nanossl.o
#
DEPS_61 += $(CONFIG)/inc/bit.h
DEPS_61 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/nanossl.o: \
    src/ssl/nanossl.c $(DEPS_61)
	@echo '   [Compile] src/ssl/nanossl.c'
	$(CC) -c -o $(CONFIG)/obj/nanossl.o $(CFLAGS) $(DFLAGS) $(IFLAGS) -Isrc/deps/est src/ssl/nanossl.c

#
#   openssl.o
#
DEPS_62 += $(CONFIG)/inc/bit.h
DEPS_62 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/openssl.o: \
    src/ssl/openssl.c $(DEPS_62)
	@echo '   [Compile] src/ssl/openssl.c'
	$(CC) -c -o $(CONFIG)/obj/openssl.o $(CFLAGS) $(DFLAGS) $(IFLAGS) -Isrc/deps/est src/ssl/openssl.c

#
#   ssl.o
#
DEPS_63 += $(CONFIG)/inc/bit.h
DEPS_63 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/ssl.o: \
    src/ssl/ssl.c $(DEPS_63)
	@echo '   [Compile] src/ssl/ssl.c'
	$(CC) -c -o $(CONFIG)/obj/ssl.o $(CFLAGS) $(DFLAGS) $(IFLAGS) -Isrc/deps/est src/ssl/ssl.c

#
#   libmprssl
#
DEPS_64 += $(CONFIG)/bin/libmpr.so
DEPS_64 += $(CONFIG)/obj/est.o
DEPS_64 += $(CONFIG)/obj/matrixssl.o
DEPS_64 += $(CONFIG)/obj/nanossl.o
DEPS_64 += $(CONFIG)/obj/openssl.o
DEPS_64 += $(CONFIG)/obj/ssl.o

LIBS_64 += -lmpr
ifeq ($(BIT_PACK_EST),1)
    LIBS_64 += -lest
endif

$(CONFIG)/bin/libmprssl.so: $(DEPS_64)
	@echo '      [Link] libmprssl'
	$(CC) -shared -o $(CONFIG)/bin/libmprssl.so $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/est.o $(CONFIG)/obj/matrixssl.o $(CONFIG)/obj/nanossl.o $(CONFIG)/obj/openssl.o $(CONFIG)/obj/ssl.o $(LIBS_64) $(LIBS_64) $(LIBS) -lest

#
#   testArgv.o
#
DEPS_65 += $(CONFIG)/inc/bit.h
DEPS_65 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testArgv.o: \
    test/testArgv.c $(DEPS_65)
	@echo '   [Compile] test/testArgv.c'
	$(CC) -c -o $(CONFIG)/obj/testArgv.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testArgv.c

#
#   testBuf.o
#
DEPS_66 += $(CONFIG)/inc/bit.h
DEPS_66 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testBuf.o: \
    test/testBuf.c $(DEPS_66)
	@echo '   [Compile] test/testBuf.c'
	$(CC) -c -o $(CONFIG)/obj/testBuf.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testBuf.c

#
#   testCmd.o
#
DEPS_67 += $(CONFIG)/inc/bit.h
DEPS_67 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testCmd.o: \
    test/testCmd.c $(DEPS_67)
	@echo '   [Compile] test/testCmd.c'
	$(CC) -c -o $(CONFIG)/obj/testCmd.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testCmd.c

#
#   testCond.o
#
DEPS_68 += $(CONFIG)/inc/bit.h
DEPS_68 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testCond.o: \
    test/testCond.c $(DEPS_68)
	@echo '   [Compile] test/testCond.c'
	$(CC) -c -o $(CONFIG)/obj/testCond.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testCond.c

#
#   testEvent.o
#
DEPS_69 += $(CONFIG)/inc/bit.h
DEPS_69 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testEvent.o: \
    test/testEvent.c $(DEPS_69)
	@echo '   [Compile] test/testEvent.c'
	$(CC) -c -o $(CONFIG)/obj/testEvent.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testEvent.c

#
#   testFile.o
#
DEPS_70 += $(CONFIG)/inc/bit.h
DEPS_70 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testFile.o: \
    test/testFile.c $(DEPS_70)
	@echo '   [Compile] test/testFile.c'
	$(CC) -c -o $(CONFIG)/obj/testFile.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testFile.c

#
#   testHash.o
#
DEPS_71 += $(CONFIG)/inc/bit.h
DEPS_71 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testHash.o: \
    test/testHash.c $(DEPS_71)
	@echo '   [Compile] test/testHash.c'
	$(CC) -c -o $(CONFIG)/obj/testHash.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testHash.c

#
#   testList.o
#
DEPS_72 += $(CONFIG)/inc/bit.h
DEPS_72 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testList.o: \
    test/testList.c $(DEPS_72)
	@echo '   [Compile] test/testList.c'
	$(CC) -c -o $(CONFIG)/obj/testList.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testList.c

#
#   testLock.o
#
DEPS_73 += $(CONFIG)/inc/bit.h
DEPS_73 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testLock.o: \
    test/testLock.c $(DEPS_73)
	@echo '   [Compile] test/testLock.c'
	$(CC) -c -o $(CONFIG)/obj/testLock.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testLock.c

#
#   testMem.o
#
DEPS_74 += $(CONFIG)/inc/bit.h
DEPS_74 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testMem.o: \
    test/testMem.c $(DEPS_74)
	@echo '   [Compile] test/testMem.c'
	$(CC) -c -o $(CONFIG)/obj/testMem.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testMem.c

#
#   testMpr.o
#
DEPS_75 += $(CONFIG)/inc/bit.h
DEPS_75 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testMpr.o: \
    test/testMpr.c $(DEPS_75)
	@echo '   [Compile] test/testMpr.c'
	$(CC) -c -o $(CONFIG)/obj/testMpr.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testMpr.c

#
#   testPath.o
#
DEPS_76 += $(CONFIG)/inc/bit.h
DEPS_76 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testPath.o: \
    test/testPath.c $(DEPS_76)
	@echo '   [Compile] test/testPath.c'
	$(CC) -c -o $(CONFIG)/obj/testPath.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testPath.c

#
#   testSocket.o
#
DEPS_77 += $(CONFIG)/inc/bit.h
DEPS_77 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testSocket.o: \
    test/testSocket.c $(DEPS_77)
	@echo '   [Compile] test/testSocket.c'
	$(CC) -c -o $(CONFIG)/obj/testSocket.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testSocket.c

#
#   testSprintf.o
#
DEPS_78 += $(CONFIG)/inc/bit.h
DEPS_78 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testSprintf.o: \
    test/testSprintf.c $(DEPS_78)
	@echo '   [Compile] test/testSprintf.c'
	$(CC) -c -o $(CONFIG)/obj/testSprintf.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testSprintf.c

#
#   testThread.o
#
DEPS_79 += $(CONFIG)/inc/bit.h
DEPS_79 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testThread.o: \
    test/testThread.c $(DEPS_79)
	@echo '   [Compile] test/testThread.c'
	$(CC) -c -o $(CONFIG)/obj/testThread.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testThread.c

#
#   testTime.o
#
DEPS_80 += $(CONFIG)/inc/bit.h
DEPS_80 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testTime.o: \
    test/testTime.c $(DEPS_80)
	@echo '   [Compile] test/testTime.c'
	$(CC) -c -o $(CONFIG)/obj/testTime.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testTime.c

#
#   testUnicode.o
#
DEPS_81 += $(CONFIG)/inc/bit.h
DEPS_81 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testUnicode.o: \
    test/testUnicode.c $(DEPS_81)
	@echo '   [Compile] test/testUnicode.c'
	$(CC) -c -o $(CONFIG)/obj/testUnicode.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testUnicode.c

#
#   testMpr
#
DEPS_82 += $(CONFIG)/bin/libmpr.so
DEPS_82 += $(CONFIG)/bin/libmprssl.so
DEPS_82 += $(CONFIG)/bin/runProgram
DEPS_82 += $(CONFIG)/obj/testArgv.o
DEPS_82 += $(CONFIG)/obj/testBuf.o
DEPS_82 += $(CONFIG)/obj/testCmd.o
DEPS_82 += $(CONFIG)/obj/testCond.o
DEPS_82 += $(CONFIG)/obj/testEvent.o
DEPS_82 += $(CONFIG)/obj/testFile.o
DEPS_82 += $(CONFIG)/obj/testHash.o
DEPS_82 += $(CONFIG)/obj/testList.o
DEPS_82 += $(CONFIG)/obj/testLock.o
DEPS_82 += $(CONFIG)/obj/testMem.o
DEPS_82 += $(CONFIG)/obj/testMpr.o
DEPS_82 += $(CONFIG)/obj/testPath.o
DEPS_82 += $(CONFIG)/obj/testSocket.o
DEPS_82 += $(CONFIG)/obj/testSprintf.o
DEPS_82 += $(CONFIG)/obj/testThread.o
DEPS_82 += $(CONFIG)/obj/testTime.o
DEPS_82 += $(CONFIG)/obj/testUnicode.o

LIBS_82 += -lmprssl
LIBS_82 += -lmpr
ifeq ($(BIT_PACK_EST),1)
    LIBS_82 += -lest
endif

$(CONFIG)/bin/testMpr: $(DEPS_82)
	@echo '      [Link] testMpr'
	$(CC) -o $(CONFIG)/bin/testMpr $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/testArgv.o $(CONFIG)/obj/testBuf.o $(CONFIG)/obj/testCmd.o $(CONFIG)/obj/testCond.o $(CONFIG)/obj/testEvent.o $(CONFIG)/obj/testFile.o $(CONFIG)/obj/testHash.o $(CONFIG)/obj/testList.o $(CONFIG)/obj/testLock.o $(CONFIG)/obj/testMem.o $(CONFIG)/obj/testMpr.o $(CONFIG)/obj/testPath.o $(CONFIG)/obj/testSocket.o $(CONFIG)/obj/testSprintf.o $(CONFIG)/obj/testThread.o $(CONFIG)/obj/testTime.o $(CONFIG)/obj/testUnicode.o $(LIBS_82) $(LIBS_82) $(LIBS) -lest $(LDFLAGS)

#
#   manager
#
DEPS_83 += $(CONFIG)/bin/libmpr.so
DEPS_83 += $(CONFIG)/obj/manager.o

LIBS_83 += -lmpr

$(CONFIG)/bin/manager: $(DEPS_83)
	@echo '      [Link] manager'
	$(CC) -o $(CONFIG)/bin/manager $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/manager.o $(LIBS_83) $(LIBS_83) $(LIBS) $(LDFLAGS)

#
#   makerom.o
#
DEPS_84 += $(CONFIG)/inc/bit.h
DEPS_84 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/makerom.o: \
    src/utils/makerom.c $(DEPS_84)
	@echo '   [Compile] src/utils/makerom.c'
	$(CC) -c -o $(CONFIG)/obj/makerom.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/utils/makerom.c

#
#   makerom
#
DEPS_85 += $(CONFIG)/bin/libmpr.so
DEPS_85 += $(CONFIG)/obj/makerom.o

LIBS_85 += -lmpr

$(CONFIG)/bin/makerom: $(DEPS_85)
	@echo '      [Link] makerom'
	$(CC) -o $(CONFIG)/bin/makerom $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/makerom.o $(LIBS_85) $(LIBS_85) $(LIBS) $(LDFLAGS)

#
#   charGen.o
#
DEPS_86 += $(CONFIG)/inc/bit.h
DEPS_86 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/charGen.o: \
    src/utils/charGen.c $(DEPS_86)
	@echo '   [Compile] src/utils/charGen.c'
	$(CC) -c -o $(CONFIG)/obj/charGen.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/utils/charGen.c

#
#   chargen
#
DEPS_87 += $(CONFIG)/bin/libmpr.so
DEPS_87 += $(CONFIG)/obj/charGen.o

LIBS_87 += -lmpr

$(CONFIG)/bin/chargen: $(DEPS_87)
	@echo '      [Link] chargen'
	$(CC) -o $(CONFIG)/bin/chargen $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/charGen.o $(LIBS_87) $(LIBS_87) $(LIBS) $(LDFLAGS)

#
#   stop
#
stop: $(DEPS_88)

#
#   installBinary
#
DEPS_89 += stop

installBinary: $(DEPS_89)

#
#   start
#
start: $(DEPS_90)

#
#   install
#
DEPS_91 += stop
DEPS_91 += installBinary
DEPS_91 += start

install: $(DEPS_91)
	

#
#   uninstall
#
DEPS_92 += stop

uninstall: $(DEPS_92)
	

