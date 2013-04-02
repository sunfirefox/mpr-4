#
#   mpr-freebsd-default.mk -- Makefile to build Multithreaded Portable Runtime for freebsd
#

PRODUCT            := mpr
VERSION            := 4.3.0
BUILD_NUMBER       := 0
PROFILE            := default
ARCH               := $(shell uname -m | sed 's/i.86/x86/;s/x86_64/x64/;s/arm.*/arm/;s/mips.*/mips/')
OS                 := freebsd
CC                 := gcc
LD                 := link
CONFIG             := $(OS)-$(ARCH)-$(PROFILE)
LBIN               := $(CONFIG)/bin

BIT_PACK_EST       := 1
BIT_PACK_MATRIXSSL := 0
BIT_PACK_OPENSSL   := 0
BIT_PACK_SSL       := 1

ifeq ($(BIT_PACK_EST),1)
    BIT_PACK_SSL := 1
endif
ifeq ($(BIT_PACK_LIB),1)
    BIT_PACK_COMPILER := 1
endif
ifeq ($(BIT_PACK_MATRIXSSL),1)
    BIT_PACK_SSL := 1
endif
ifeq ($(BIT_PACK_NANOSSL),1)
    BIT_PACK_SSL := 1
endif
ifeq ($(BIT_PACK_OPENSSL),1)
    BIT_PACK_SSL := 1
endif

BIT_PACK_COMPILER_PATH    := gcc
BIT_PACK_DOXYGEN_PATH     := doxygen
BIT_PACK_DSI_PATH         := dsi
BIT_PACK_EST_PATH         := est
BIT_PACK_LIB_PATH         := ar
BIT_PACK_LINK_PATH        := link
BIT_PACK_MAN_PATH         := man
BIT_PACK_MAN2HTML_PATH    := man2html
BIT_PACK_MATRIXSSL_PATH   := /usr/src/matrixssl
BIT_PACK_MD5_PATH         := md5
BIT_PACK_NANOSSL_PATH     := /usr/src/nanossl
BIT_PACK_OPENSSL_PATH     := /usr/src/openssl
BIT_PACK_SSL_PATH         := ssl
BIT_PACK_UTEST_PATH       := utest

CFLAGS             += -fPIC  -w
DFLAGS             += -D_REENTRANT -DPIC  $(patsubst %,-D%,$(filter BIT_%,$(MAKEFLAGS))) -DBIT_PACK_EST=$(BIT_PACK_EST) -DBIT_PACK_MATRIXSSL=$(BIT_PACK_MATRIXSSL) -DBIT_PACK_OPENSSL=$(BIT_PACK_OPENSSL) -DBIT_PACK_SSL=$(BIT_PACK_SSL) 
IFLAGS             += -I$(CONFIG)/inc
LDFLAGS            += '-g'
LIBPATHS           += -L$(CONFIG)/bin
LIBS               += -lpthread -lm -ldl

DEBUG              := debug
CFLAGS-debug       := -g
DFLAGS-debug       := -DBIT_DEBUG
LDFLAGS-debug      := -g
DFLAGS-release     := 
CFLAGS-release     := -O2
LDFLAGS-release    := 
CFLAGS             += $(CFLAGS-$(DEBUG))
DFLAGS             += $(DFLAGS-$(DEBUG))
LDFLAGS            += $(LDFLAGS-$(DEBUG))

BIT_ROOT_PREFIX    := 
BIT_BASE_PREFIX    := $(BIT_ROOT_PREFIX)/usr/local
BIT_DATA_PREFIX    := $(BIT_ROOT_PREFIX)/
BIT_STATE_PREFIX   := $(BIT_ROOT_PREFIX)/var
BIT_APP_PREFIX     := $(BIT_BASE_PREFIX)/lib/$(PRODUCT)
BIT_VAPP_PREFIX    := $(BIT_APP_PREFIX)/$(VERSION)
BIT_BIN_PREFIX     := $(BIT_ROOT_PREFIX)/usr/local/bin
BIT_INC_PREFIX     := $(BIT_ROOT_PREFIX)/usr/local/include
BIT_LIB_PREFIX     := $(BIT_ROOT_PREFIX)/usr/local/lib
BIT_MAN_PREFIX     := $(BIT_ROOT_PREFIX)/usr/local/share/man
BIT_SBIN_PREFIX    := $(BIT_ROOT_PREFIX)/usr/local/sbin
BIT_ETC_PREFIX     := $(BIT_ROOT_PREFIX)/etc/$(PRODUCT)
BIT_WEB_PREFIX     := $(BIT_ROOT_PREFIX)/var/www/$(PRODUCT)-default
BIT_LOG_PREFIX     := $(BIT_ROOT_PREFIX)/var/log/$(PRODUCT)
BIT_SPOOL_PREFIX   := $(BIT_ROOT_PREFIX)/var/spool/$(PRODUCT)
BIT_CACHE_PREFIX   := $(BIT_ROOT_PREFIX)/var/spool/$(PRODUCT)/cache
BIT_SRC_PREFIX     := $(BIT_ROOT_PREFIX)$(PRODUCT)-$(VERSION)


ifeq ($(BIT_PACK_EST),1)
TARGETS            += $(CONFIG)/bin/libest.so
endif
TARGETS            += $(CONFIG)/bin/ca.crt
TARGETS            += $(CONFIG)/bin/benchMpr
TARGETS            += $(CONFIG)/bin/runProgram
TARGETS            += $(CONFIG)/bin/testMpr
TARGETS            += $(CONFIG)/bin/libmprssl.so
TARGETS            += $(CONFIG)/bin/manager
TARGETS            += $(CONFIG)/bin/makerom
TARGETS            += $(CONFIG)/bin/chargen

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
	@[ ! -f $(CONFIG)/inc/bit.h ] && cp projects/mpr-freebsd-default-bit.h $(CONFIG)/inc/bit.h ; true
	@[ ! -f $(CONFIG)/inc/bitos.h ] && cp src/bitos.h $(CONFIG)/inc/bitos.h ; true
	@if ! diff $(CONFIG)/inc/bitos.h src/bitos.h >/dev/null ; then\
		cp src/bitos.h $(CONFIG)/inc/bitos.h  ; \
	fi; true
	@if ! diff $(CONFIG)/inc/bit.h projects/mpr-freebsd-default-bit.h >/dev/null ; then\
		cp projects/mpr-freebsd-default-bit.h $(CONFIG)/inc/bit.h  ; \
	fi; true
	@if [ -f "$(CONFIG)/.makeflags" ] ; then \
		if [ "$(MAKEFLAGS)" != " ` cat $(CONFIG)/.makeflags`" ] ; then \
			echo "   [Warning] Make flags have changed since the last build: "`cat $(CONFIG)/.makeflags`"" ; \
		fi ; \
	fi
	@echo $(MAKEFLAGS) >$(CONFIG)/.makeflags

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
	rm -f "$(CONFIG)/obj/manager.o"
	rm -f "$(CONFIG)/obj/makerom.o"
	rm -f "$(CONFIG)/obj/charGen.o"

clobber: clean
	rm -fr ./$(CONFIG)



#
#   version
#
version: $(DEPS_1)
	@echo 4.3.0-0

#
#   est.h
#
$(CONFIG)/inc/est.h: $(DEPS_2)
	@echo '      [Copy] $(CONFIG)/inc/est.h'
	mkdir -p "$(CONFIG)/inc"
	cp src/deps/est/est.h $(CONFIG)/inc/est.h

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
	cp src/bitos.h $(CONFIG)/inc/bitos.h

#
#   estLib.o
#
DEPS_5 += $(CONFIG)/inc/bit.h
DEPS_5 += $(CONFIG)/inc/est.h
DEPS_5 += $(CONFIG)/inc/bitos.h

$(CONFIG)/obj/estLib.o: \
    src/deps/est/estLib.c $(DEPS_5)
	@echo '   [Compile] $(CONFIG)/obj/estLib.o'
	$(CC) -c -o $(CONFIG)/obj/estLib.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/deps/est/estLib.c

ifeq ($(BIT_PACK_EST),1)
#
#   libest
#
DEPS_6 += $(CONFIG)/inc/est.h
DEPS_6 += $(CONFIG)/inc/bit.h
DEPS_6 += $(CONFIG)/inc/bitos.h
DEPS_6 += $(CONFIG)/obj/estLib.o

$(CONFIG)/bin/libest.so: $(DEPS_6)
	@echo '      [Link] $(CONFIG)/bin/libest.so'
	$(CC) -shared -o $(CONFIG)/bin/libest.so $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/estLib.o $(LIBS) 
endif

#
#   ca-crt
#
DEPS_7 += src/deps/est/ca.crt

$(CONFIG)/bin/ca.crt: $(DEPS_7)
	@echo '      [Copy] $(CONFIG)/bin/ca.crt'
	mkdir -p "$(CONFIG)/bin"
	cp src/deps/est/ca.crt $(CONFIG)/bin/ca.crt

#
#   mpr.h
#
$(CONFIG)/inc/mpr.h: $(DEPS_8)
	@echo '      [Copy] $(CONFIG)/inc/mpr.h'
	mkdir -p "$(CONFIG)/inc"
	cp src/mpr.h $(CONFIG)/inc/mpr.h

#
#   async.o
#
DEPS_9 += $(CONFIG)/inc/bit.h
DEPS_9 += $(CONFIG)/inc/mpr.h
DEPS_9 += $(CONFIG)/inc/bitos.h

$(CONFIG)/obj/async.o: \
    src/async.c $(DEPS_9)
	@echo '   [Compile] $(CONFIG)/obj/async.o'
	$(CC) -c -o $(CONFIG)/obj/async.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/async.c

#
#   atomic.o
#
DEPS_10 += $(CONFIG)/inc/bit.h
DEPS_10 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/atomic.o: \
    src/atomic.c $(DEPS_10)
	@echo '   [Compile] $(CONFIG)/obj/atomic.o'
	$(CC) -c -o $(CONFIG)/obj/atomic.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/atomic.c

#
#   buf.o
#
DEPS_11 += $(CONFIG)/inc/bit.h
DEPS_11 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/buf.o: \
    src/buf.c $(DEPS_11)
	@echo '   [Compile] $(CONFIG)/obj/buf.o'
	$(CC) -c -o $(CONFIG)/obj/buf.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/buf.c

#
#   cache.o
#
DEPS_12 += $(CONFIG)/inc/bit.h
DEPS_12 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/cache.o: \
    src/cache.c $(DEPS_12)
	@echo '   [Compile] $(CONFIG)/obj/cache.o'
	$(CC) -c -o $(CONFIG)/obj/cache.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/cache.c

#
#   cmd.o
#
DEPS_13 += $(CONFIG)/inc/bit.h
DEPS_13 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/cmd.o: \
    src/cmd.c $(DEPS_13)
	@echo '   [Compile] $(CONFIG)/obj/cmd.o'
	$(CC) -c -o $(CONFIG)/obj/cmd.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/cmd.c

#
#   cond.o
#
DEPS_14 += $(CONFIG)/inc/bit.h
DEPS_14 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/cond.o: \
    src/cond.c $(DEPS_14)
	@echo '   [Compile] $(CONFIG)/obj/cond.o'
	$(CC) -c -o $(CONFIG)/obj/cond.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/cond.c

#
#   crypt.o
#
DEPS_15 += $(CONFIG)/inc/bit.h
DEPS_15 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/crypt.o: \
    src/crypt.c $(DEPS_15)
	@echo '   [Compile] $(CONFIG)/obj/crypt.o'
	$(CC) -c -o $(CONFIG)/obj/crypt.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/crypt.c

#
#   disk.o
#
DEPS_16 += $(CONFIG)/inc/bit.h
DEPS_16 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/disk.o: \
    src/disk.c $(DEPS_16)
	@echo '   [Compile] $(CONFIG)/obj/disk.o'
	$(CC) -c -o $(CONFIG)/obj/disk.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/disk.c

#
#   dispatcher.o
#
DEPS_17 += $(CONFIG)/inc/bit.h
DEPS_17 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/dispatcher.o: \
    src/dispatcher.c $(DEPS_17)
	@echo '   [Compile] $(CONFIG)/obj/dispatcher.o'
	$(CC) -c -o $(CONFIG)/obj/dispatcher.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/dispatcher.c

#
#   encode.o
#
DEPS_18 += $(CONFIG)/inc/bit.h
DEPS_18 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/encode.o: \
    src/encode.c $(DEPS_18)
	@echo '   [Compile] $(CONFIG)/obj/encode.o'
	$(CC) -c -o $(CONFIG)/obj/encode.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/encode.c

#
#   epoll.o
#
DEPS_19 += $(CONFIG)/inc/bit.h
DEPS_19 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/epoll.o: \
    src/epoll.c $(DEPS_19)
	@echo '   [Compile] $(CONFIG)/obj/epoll.o'
	$(CC) -c -o $(CONFIG)/obj/epoll.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/epoll.c

#
#   event.o
#
DEPS_20 += $(CONFIG)/inc/bit.h
DEPS_20 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/event.o: \
    src/event.c $(DEPS_20)
	@echo '   [Compile] $(CONFIG)/obj/event.o'
	$(CC) -c -o $(CONFIG)/obj/event.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/event.c

#
#   file.o
#
DEPS_21 += $(CONFIG)/inc/bit.h
DEPS_21 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/file.o: \
    src/file.c $(DEPS_21)
	@echo '   [Compile] $(CONFIG)/obj/file.o'
	$(CC) -c -o $(CONFIG)/obj/file.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/file.c

#
#   fs.o
#
DEPS_22 += $(CONFIG)/inc/bit.h
DEPS_22 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/fs.o: \
    src/fs.c $(DEPS_22)
	@echo '   [Compile] $(CONFIG)/obj/fs.o'
	$(CC) -c -o $(CONFIG)/obj/fs.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/fs.c

#
#   hash.o
#
DEPS_23 += $(CONFIG)/inc/bit.h
DEPS_23 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/hash.o: \
    src/hash.c $(DEPS_23)
	@echo '   [Compile] $(CONFIG)/obj/hash.o'
	$(CC) -c -o $(CONFIG)/obj/hash.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/hash.c

#
#   json.o
#
DEPS_24 += $(CONFIG)/inc/bit.h
DEPS_24 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/json.o: \
    src/json.c $(DEPS_24)
	@echo '   [Compile] $(CONFIG)/obj/json.o'
	$(CC) -c -o $(CONFIG)/obj/json.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/json.c

#
#   kqueue.o
#
DEPS_25 += $(CONFIG)/inc/bit.h
DEPS_25 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/kqueue.o: \
    src/kqueue.c $(DEPS_25)
	@echo '   [Compile] $(CONFIG)/obj/kqueue.o'
	$(CC) -c -o $(CONFIG)/obj/kqueue.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/kqueue.c

#
#   list.o
#
DEPS_26 += $(CONFIG)/inc/bit.h
DEPS_26 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/list.o: \
    src/list.c $(DEPS_26)
	@echo '   [Compile] $(CONFIG)/obj/list.o'
	$(CC) -c -o $(CONFIG)/obj/list.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/list.c

#
#   lock.o
#
DEPS_27 += $(CONFIG)/inc/bit.h
DEPS_27 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/lock.o: \
    src/lock.c $(DEPS_27)
	@echo '   [Compile] $(CONFIG)/obj/lock.o'
	$(CC) -c -o $(CONFIG)/obj/lock.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/lock.c

#
#   log.o
#
DEPS_28 += $(CONFIG)/inc/bit.h
DEPS_28 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/log.o: \
    src/log.c $(DEPS_28)
	@echo '   [Compile] $(CONFIG)/obj/log.o'
	$(CC) -c -o $(CONFIG)/obj/log.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/log.c

#
#   mem.o
#
DEPS_29 += $(CONFIG)/inc/bit.h
DEPS_29 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mem.o: \
    src/mem.c $(DEPS_29)
	@echo '   [Compile] $(CONFIG)/obj/mem.o'
	$(CC) -c -o $(CONFIG)/obj/mem.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/mem.c

#
#   mime.o
#
DEPS_30 += $(CONFIG)/inc/bit.h
DEPS_30 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mime.o: \
    src/mime.c $(DEPS_30)
	@echo '   [Compile] $(CONFIG)/obj/mime.o'
	$(CC) -c -o $(CONFIG)/obj/mime.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/mime.c

#
#   mixed.o
#
DEPS_31 += $(CONFIG)/inc/bit.h
DEPS_31 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mixed.o: \
    src/mixed.c $(DEPS_31)
	@echo '   [Compile] $(CONFIG)/obj/mixed.o'
	$(CC) -c -o $(CONFIG)/obj/mixed.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/mixed.c

#
#   module.o
#
DEPS_32 += $(CONFIG)/inc/bit.h
DEPS_32 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/module.o: \
    src/module.c $(DEPS_32)
	@echo '   [Compile] $(CONFIG)/obj/module.o'
	$(CC) -c -o $(CONFIG)/obj/module.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/module.c

#
#   mpr.o
#
DEPS_33 += $(CONFIG)/inc/bit.h
DEPS_33 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mpr.o: \
    src/mpr.c $(DEPS_33)
	@echo '   [Compile] $(CONFIG)/obj/mpr.o'
	$(CC) -c -o $(CONFIG)/obj/mpr.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/mpr.c

#
#   path.o
#
DEPS_34 += $(CONFIG)/inc/bit.h
DEPS_34 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/path.o: \
    src/path.c $(DEPS_34)
	@echo '   [Compile] $(CONFIG)/obj/path.o'
	$(CC) -c -o $(CONFIG)/obj/path.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/path.c

#
#   poll.o
#
DEPS_35 += $(CONFIG)/inc/bit.h
DEPS_35 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/poll.o: \
    src/poll.c $(DEPS_35)
	@echo '   [Compile] $(CONFIG)/obj/poll.o'
	$(CC) -c -o $(CONFIG)/obj/poll.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/poll.c

#
#   posix.o
#
DEPS_36 += $(CONFIG)/inc/bit.h
DEPS_36 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/posix.o: \
    src/posix.c $(DEPS_36)
	@echo '   [Compile] $(CONFIG)/obj/posix.o'
	$(CC) -c -o $(CONFIG)/obj/posix.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/posix.c

#
#   printf.o
#
DEPS_37 += $(CONFIG)/inc/bit.h
DEPS_37 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/printf.o: \
    src/printf.c $(DEPS_37)
	@echo '   [Compile] $(CONFIG)/obj/printf.o'
	$(CC) -c -o $(CONFIG)/obj/printf.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/printf.c

#
#   rom.o
#
DEPS_38 += $(CONFIG)/inc/bit.h
DEPS_38 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/rom.o: \
    src/rom.c $(DEPS_38)
	@echo '   [Compile] $(CONFIG)/obj/rom.o'
	$(CC) -c -o $(CONFIG)/obj/rom.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/rom.c

#
#   select.o
#
DEPS_39 += $(CONFIG)/inc/bit.h
DEPS_39 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/select.o: \
    src/select.c $(DEPS_39)
	@echo '   [Compile] $(CONFIG)/obj/select.o'
	$(CC) -c -o $(CONFIG)/obj/select.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/select.c

#
#   signal.o
#
DEPS_40 += $(CONFIG)/inc/bit.h
DEPS_40 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/signal.o: \
    src/signal.c $(DEPS_40)
	@echo '   [Compile] $(CONFIG)/obj/signal.o'
	$(CC) -c -o $(CONFIG)/obj/signal.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/signal.c

#
#   socket.o
#
DEPS_41 += $(CONFIG)/inc/bit.h
DEPS_41 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/socket.o: \
    src/socket.c $(DEPS_41)
	@echo '   [Compile] $(CONFIG)/obj/socket.o'
	$(CC) -c -o $(CONFIG)/obj/socket.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/socket.c

#
#   string.o
#
DEPS_42 += $(CONFIG)/inc/bit.h
DEPS_42 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/string.o: \
    src/string.c $(DEPS_42)
	@echo '   [Compile] $(CONFIG)/obj/string.o'
	$(CC) -c -o $(CONFIG)/obj/string.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/string.c

#
#   test.o
#
DEPS_43 += $(CONFIG)/inc/bit.h
DEPS_43 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/test.o: \
    src/test.c $(DEPS_43)
	@echo '   [Compile] $(CONFIG)/obj/test.o'
	$(CC) -c -o $(CONFIG)/obj/test.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/test.c

#
#   thread.o
#
DEPS_44 += $(CONFIG)/inc/bit.h
DEPS_44 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/thread.o: \
    src/thread.c $(DEPS_44)
	@echo '   [Compile] $(CONFIG)/obj/thread.o'
	$(CC) -c -o $(CONFIG)/obj/thread.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/thread.c

#
#   time.o
#
DEPS_45 += $(CONFIG)/inc/bit.h
DEPS_45 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/time.o: \
    src/time.c $(DEPS_45)
	@echo '   [Compile] $(CONFIG)/obj/time.o'
	$(CC) -c -o $(CONFIG)/obj/time.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/time.c

#
#   vxworks.o
#
DEPS_46 += $(CONFIG)/inc/bit.h
DEPS_46 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/vxworks.o: \
    src/vxworks.c $(DEPS_46)
	@echo '   [Compile] $(CONFIG)/obj/vxworks.o'
	$(CC) -c -o $(CONFIG)/obj/vxworks.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/vxworks.c

#
#   wait.o
#
DEPS_47 += $(CONFIG)/inc/bit.h
DEPS_47 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wait.o: \
    src/wait.c $(DEPS_47)
	@echo '   [Compile] $(CONFIG)/obj/wait.o'
	$(CC) -c -o $(CONFIG)/obj/wait.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/wait.c

#
#   wide.o
#
DEPS_48 += $(CONFIG)/inc/bit.h
DEPS_48 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wide.o: \
    src/wide.c $(DEPS_48)
	@echo '   [Compile] $(CONFIG)/obj/wide.o'
	$(CC) -c -o $(CONFIG)/obj/wide.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/wide.c

#
#   win.o
#
DEPS_49 += $(CONFIG)/inc/bit.h
DEPS_49 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/win.o: \
    src/win.c $(DEPS_49)
	@echo '   [Compile] $(CONFIG)/obj/win.o'
	$(CC) -c -o $(CONFIG)/obj/win.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/win.c

#
#   wince.o
#
DEPS_50 += $(CONFIG)/inc/bit.h
DEPS_50 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wince.o: \
    src/wince.c $(DEPS_50)
	@echo '   [Compile] $(CONFIG)/obj/wince.o'
	$(CC) -c -o $(CONFIG)/obj/wince.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/wince.c

#
#   xml.o
#
DEPS_51 += $(CONFIG)/inc/bit.h
DEPS_51 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/xml.o: \
    src/xml.c $(DEPS_51)
	@echo '   [Compile] $(CONFIG)/obj/xml.o'
	$(CC) -c -o $(CONFIG)/obj/xml.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/xml.c

#
#   libmpr
#
DEPS_52 += $(CONFIG)/inc/bit.h
DEPS_52 += $(CONFIG)/inc/bitos.h
DEPS_52 += $(CONFIG)/inc/mpr.h
DEPS_52 += $(CONFIG)/obj/async.o
DEPS_52 += $(CONFIG)/obj/atomic.o
DEPS_52 += $(CONFIG)/obj/buf.o
DEPS_52 += $(CONFIG)/obj/cache.o
DEPS_52 += $(CONFIG)/obj/cmd.o
DEPS_52 += $(CONFIG)/obj/cond.o
DEPS_52 += $(CONFIG)/obj/crypt.o
DEPS_52 += $(CONFIG)/obj/disk.o
DEPS_52 += $(CONFIG)/obj/dispatcher.o
DEPS_52 += $(CONFIG)/obj/encode.o
DEPS_52 += $(CONFIG)/obj/epoll.o
DEPS_52 += $(CONFIG)/obj/event.o
DEPS_52 += $(CONFIG)/obj/file.o
DEPS_52 += $(CONFIG)/obj/fs.o
DEPS_52 += $(CONFIG)/obj/hash.o
DEPS_52 += $(CONFIG)/obj/json.o
DEPS_52 += $(CONFIG)/obj/kqueue.o
DEPS_52 += $(CONFIG)/obj/list.o
DEPS_52 += $(CONFIG)/obj/lock.o
DEPS_52 += $(CONFIG)/obj/log.o
DEPS_52 += $(CONFIG)/obj/mem.o
DEPS_52 += $(CONFIG)/obj/mime.o
DEPS_52 += $(CONFIG)/obj/mixed.o
DEPS_52 += $(CONFIG)/obj/module.o
DEPS_52 += $(CONFIG)/obj/mpr.o
DEPS_52 += $(CONFIG)/obj/path.o
DEPS_52 += $(CONFIG)/obj/poll.o
DEPS_52 += $(CONFIG)/obj/posix.o
DEPS_52 += $(CONFIG)/obj/printf.o
DEPS_52 += $(CONFIG)/obj/rom.o
DEPS_52 += $(CONFIG)/obj/select.o
DEPS_52 += $(CONFIG)/obj/signal.o
DEPS_52 += $(CONFIG)/obj/socket.o
DEPS_52 += $(CONFIG)/obj/string.o
DEPS_52 += $(CONFIG)/obj/test.o
DEPS_52 += $(CONFIG)/obj/thread.o
DEPS_52 += $(CONFIG)/obj/time.o
DEPS_52 += $(CONFIG)/obj/vxworks.o
DEPS_52 += $(CONFIG)/obj/wait.o
DEPS_52 += $(CONFIG)/obj/wide.o
DEPS_52 += $(CONFIG)/obj/win.o
DEPS_52 += $(CONFIG)/obj/wince.o
DEPS_52 += $(CONFIG)/obj/xml.o

$(CONFIG)/bin/libmpr.so: $(DEPS_52)
	@echo '      [Link] $(CONFIG)/bin/libmpr.so'
	$(CC) -shared -o $(CONFIG)/bin/libmpr.so $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/async.o $(CONFIG)/obj/atomic.o $(CONFIG)/obj/buf.o $(CONFIG)/obj/cache.o $(CONFIG)/obj/cmd.o $(CONFIG)/obj/cond.o $(CONFIG)/obj/crypt.o $(CONFIG)/obj/disk.o $(CONFIG)/obj/dispatcher.o $(CONFIG)/obj/encode.o $(CONFIG)/obj/epoll.o $(CONFIG)/obj/event.o $(CONFIG)/obj/file.o $(CONFIG)/obj/fs.o $(CONFIG)/obj/hash.o $(CONFIG)/obj/json.o $(CONFIG)/obj/kqueue.o $(CONFIG)/obj/list.o $(CONFIG)/obj/lock.o $(CONFIG)/obj/log.o $(CONFIG)/obj/mem.o $(CONFIG)/obj/mime.o $(CONFIG)/obj/mixed.o $(CONFIG)/obj/module.o $(CONFIG)/obj/mpr.o $(CONFIG)/obj/path.o $(CONFIG)/obj/poll.o $(CONFIG)/obj/posix.o $(CONFIG)/obj/printf.o $(CONFIG)/obj/rom.o $(CONFIG)/obj/select.o $(CONFIG)/obj/signal.o $(CONFIG)/obj/socket.o $(CONFIG)/obj/string.o $(CONFIG)/obj/test.o $(CONFIG)/obj/thread.o $(CONFIG)/obj/time.o $(CONFIG)/obj/vxworks.o $(CONFIG)/obj/wait.o $(CONFIG)/obj/wide.o $(CONFIG)/obj/win.o $(CONFIG)/obj/wince.o $(CONFIG)/obj/xml.o $(LIBS) 

#
#   benchMpr.o
#
DEPS_53 += $(CONFIG)/inc/bit.h
DEPS_53 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/benchMpr.o: \
    test/benchMpr.c $(DEPS_53)
	@echo '   [Compile] $(CONFIG)/obj/benchMpr.o'
	$(CC) -c -o $(CONFIG)/obj/benchMpr.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/benchMpr.c

#
#   benchMpr
#
DEPS_54 += $(CONFIG)/inc/bit.h
DEPS_54 += $(CONFIG)/inc/bitos.h
DEPS_54 += $(CONFIG)/inc/mpr.h
DEPS_54 += $(CONFIG)/obj/async.o
DEPS_54 += $(CONFIG)/obj/atomic.o
DEPS_54 += $(CONFIG)/obj/buf.o
DEPS_54 += $(CONFIG)/obj/cache.o
DEPS_54 += $(CONFIG)/obj/cmd.o
DEPS_54 += $(CONFIG)/obj/cond.o
DEPS_54 += $(CONFIG)/obj/crypt.o
DEPS_54 += $(CONFIG)/obj/disk.o
DEPS_54 += $(CONFIG)/obj/dispatcher.o
DEPS_54 += $(CONFIG)/obj/encode.o
DEPS_54 += $(CONFIG)/obj/epoll.o
DEPS_54 += $(CONFIG)/obj/event.o
DEPS_54 += $(CONFIG)/obj/file.o
DEPS_54 += $(CONFIG)/obj/fs.o
DEPS_54 += $(CONFIG)/obj/hash.o
DEPS_54 += $(CONFIG)/obj/json.o
DEPS_54 += $(CONFIG)/obj/kqueue.o
DEPS_54 += $(CONFIG)/obj/list.o
DEPS_54 += $(CONFIG)/obj/lock.o
DEPS_54 += $(CONFIG)/obj/log.o
DEPS_54 += $(CONFIG)/obj/mem.o
DEPS_54 += $(CONFIG)/obj/mime.o
DEPS_54 += $(CONFIG)/obj/mixed.o
DEPS_54 += $(CONFIG)/obj/module.o
DEPS_54 += $(CONFIG)/obj/mpr.o
DEPS_54 += $(CONFIG)/obj/path.o
DEPS_54 += $(CONFIG)/obj/poll.o
DEPS_54 += $(CONFIG)/obj/posix.o
DEPS_54 += $(CONFIG)/obj/printf.o
DEPS_54 += $(CONFIG)/obj/rom.o
DEPS_54 += $(CONFIG)/obj/select.o
DEPS_54 += $(CONFIG)/obj/signal.o
DEPS_54 += $(CONFIG)/obj/socket.o
DEPS_54 += $(CONFIG)/obj/string.o
DEPS_54 += $(CONFIG)/obj/test.o
DEPS_54 += $(CONFIG)/obj/thread.o
DEPS_54 += $(CONFIG)/obj/time.o
DEPS_54 += $(CONFIG)/obj/vxworks.o
DEPS_54 += $(CONFIG)/obj/wait.o
DEPS_54 += $(CONFIG)/obj/wide.o
DEPS_54 += $(CONFIG)/obj/win.o
DEPS_54 += $(CONFIG)/obj/wince.o
DEPS_54 += $(CONFIG)/obj/xml.o
DEPS_54 += $(CONFIG)/bin/libmpr.so
DEPS_54 += $(CONFIG)/obj/benchMpr.o

LIBS_54 += -lmpr

$(CONFIG)/bin/benchMpr: $(DEPS_54)
	@echo '      [Link] $(CONFIG)/bin/benchMpr'
	$(CC) -o $(CONFIG)/bin/benchMpr $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/benchMpr.o $(LIBPATHS_54) $(LIBS_54) $(LIBS_54) $(LIBS) -lpthread -lm -ldl $(LDFLAGS) 

#
#   runProgram.o
#
DEPS_55 += $(CONFIG)/inc/bit.h

$(CONFIG)/obj/runProgram.o: \
    test/runProgram.c $(DEPS_55)
	@echo '   [Compile] $(CONFIG)/obj/runProgram.o'
	$(CC) -c -o $(CONFIG)/obj/runProgram.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/runProgram.c

#
#   runProgram
#
DEPS_56 += $(CONFIG)/inc/bit.h
DEPS_56 += $(CONFIG)/obj/runProgram.o

$(CONFIG)/bin/runProgram: $(DEPS_56)
	@echo '      [Link] $(CONFIG)/bin/runProgram'
	$(CC) -o $(CONFIG)/bin/runProgram $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/runProgram.o $(LIBS) -lpthread -lm -ldl $(LDFLAGS) 

#
#   testArgv.o
#
DEPS_57 += $(CONFIG)/inc/bit.h
DEPS_57 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testArgv.o: \
    test/testArgv.c $(DEPS_57)
	@echo '   [Compile] $(CONFIG)/obj/testArgv.o'
	$(CC) -c -o $(CONFIG)/obj/testArgv.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testArgv.c

#
#   testBuf.o
#
DEPS_58 += $(CONFIG)/inc/bit.h
DEPS_58 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testBuf.o: \
    test/testBuf.c $(DEPS_58)
	@echo '   [Compile] $(CONFIG)/obj/testBuf.o'
	$(CC) -c -o $(CONFIG)/obj/testBuf.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testBuf.c

#
#   testCmd.o
#
DEPS_59 += $(CONFIG)/inc/bit.h
DEPS_59 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testCmd.o: \
    test/testCmd.c $(DEPS_59)
	@echo '   [Compile] $(CONFIG)/obj/testCmd.o'
	$(CC) -c -o $(CONFIG)/obj/testCmd.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testCmd.c

#
#   testCond.o
#
DEPS_60 += $(CONFIG)/inc/bit.h
DEPS_60 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testCond.o: \
    test/testCond.c $(DEPS_60)
	@echo '   [Compile] $(CONFIG)/obj/testCond.o'
	$(CC) -c -o $(CONFIG)/obj/testCond.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testCond.c

#
#   testEvent.o
#
DEPS_61 += $(CONFIG)/inc/bit.h
DEPS_61 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testEvent.o: \
    test/testEvent.c $(DEPS_61)
	@echo '   [Compile] $(CONFIG)/obj/testEvent.o'
	$(CC) -c -o $(CONFIG)/obj/testEvent.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testEvent.c

#
#   testFile.o
#
DEPS_62 += $(CONFIG)/inc/bit.h
DEPS_62 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testFile.o: \
    test/testFile.c $(DEPS_62)
	@echo '   [Compile] $(CONFIG)/obj/testFile.o'
	$(CC) -c -o $(CONFIG)/obj/testFile.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testFile.c

#
#   testHash.o
#
DEPS_63 += $(CONFIG)/inc/bit.h
DEPS_63 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testHash.o: \
    test/testHash.c $(DEPS_63)
	@echo '   [Compile] $(CONFIG)/obj/testHash.o'
	$(CC) -c -o $(CONFIG)/obj/testHash.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testHash.c

#
#   testList.o
#
DEPS_64 += $(CONFIG)/inc/bit.h
DEPS_64 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testList.o: \
    test/testList.c $(DEPS_64)
	@echo '   [Compile] $(CONFIG)/obj/testList.o'
	$(CC) -c -o $(CONFIG)/obj/testList.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testList.c

#
#   testLock.o
#
DEPS_65 += $(CONFIG)/inc/bit.h
DEPS_65 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testLock.o: \
    test/testLock.c $(DEPS_65)
	@echo '   [Compile] $(CONFIG)/obj/testLock.o'
	$(CC) -c -o $(CONFIG)/obj/testLock.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testLock.c

#
#   testMem.o
#
DEPS_66 += $(CONFIG)/inc/bit.h
DEPS_66 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testMem.o: \
    test/testMem.c $(DEPS_66)
	@echo '   [Compile] $(CONFIG)/obj/testMem.o'
	$(CC) -c -o $(CONFIG)/obj/testMem.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testMem.c

#
#   testMpr.o
#
DEPS_67 += $(CONFIG)/inc/bit.h
DEPS_67 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testMpr.o: \
    test/testMpr.c $(DEPS_67)
	@echo '   [Compile] $(CONFIG)/obj/testMpr.o'
	$(CC) -c -o $(CONFIG)/obj/testMpr.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testMpr.c

#
#   testPath.o
#
DEPS_68 += $(CONFIG)/inc/bit.h
DEPS_68 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testPath.o: \
    test/testPath.c $(DEPS_68)
	@echo '   [Compile] $(CONFIG)/obj/testPath.o'
	$(CC) -c -o $(CONFIG)/obj/testPath.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testPath.c

#
#   testSocket.o
#
DEPS_69 += $(CONFIG)/inc/bit.h
DEPS_69 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testSocket.o: \
    test/testSocket.c $(DEPS_69)
	@echo '   [Compile] $(CONFIG)/obj/testSocket.o'
	$(CC) -c -o $(CONFIG)/obj/testSocket.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testSocket.c

#
#   testSprintf.o
#
DEPS_70 += $(CONFIG)/inc/bit.h
DEPS_70 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testSprintf.o: \
    test/testSprintf.c $(DEPS_70)
	@echo '   [Compile] $(CONFIG)/obj/testSprintf.o'
	$(CC) -c -o $(CONFIG)/obj/testSprintf.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testSprintf.c

#
#   testThread.o
#
DEPS_71 += $(CONFIG)/inc/bit.h
DEPS_71 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testThread.o: \
    test/testThread.c $(DEPS_71)
	@echo '   [Compile] $(CONFIG)/obj/testThread.o'
	$(CC) -c -o $(CONFIG)/obj/testThread.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testThread.c

#
#   testTime.o
#
DEPS_72 += $(CONFIG)/inc/bit.h
DEPS_72 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testTime.o: \
    test/testTime.c $(DEPS_72)
	@echo '   [Compile] $(CONFIG)/obj/testTime.o'
	$(CC) -c -o $(CONFIG)/obj/testTime.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testTime.c

#
#   testUnicode.o
#
DEPS_73 += $(CONFIG)/inc/bit.h
DEPS_73 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testUnicode.o: \
    test/testUnicode.c $(DEPS_73)
	@echo '   [Compile] $(CONFIG)/obj/testUnicode.o'
	$(CC) -c -o $(CONFIG)/obj/testUnicode.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) test/testUnicode.c

#
#   testMpr
#
DEPS_74 += $(CONFIG)/inc/bit.h
DEPS_74 += $(CONFIG)/inc/bitos.h
DEPS_74 += $(CONFIG)/inc/mpr.h
DEPS_74 += $(CONFIG)/obj/async.o
DEPS_74 += $(CONFIG)/obj/atomic.o
DEPS_74 += $(CONFIG)/obj/buf.o
DEPS_74 += $(CONFIG)/obj/cache.o
DEPS_74 += $(CONFIG)/obj/cmd.o
DEPS_74 += $(CONFIG)/obj/cond.o
DEPS_74 += $(CONFIG)/obj/crypt.o
DEPS_74 += $(CONFIG)/obj/disk.o
DEPS_74 += $(CONFIG)/obj/dispatcher.o
DEPS_74 += $(CONFIG)/obj/encode.o
DEPS_74 += $(CONFIG)/obj/epoll.o
DEPS_74 += $(CONFIG)/obj/event.o
DEPS_74 += $(CONFIG)/obj/file.o
DEPS_74 += $(CONFIG)/obj/fs.o
DEPS_74 += $(CONFIG)/obj/hash.o
DEPS_74 += $(CONFIG)/obj/json.o
DEPS_74 += $(CONFIG)/obj/kqueue.o
DEPS_74 += $(CONFIG)/obj/list.o
DEPS_74 += $(CONFIG)/obj/lock.o
DEPS_74 += $(CONFIG)/obj/log.o
DEPS_74 += $(CONFIG)/obj/mem.o
DEPS_74 += $(CONFIG)/obj/mime.o
DEPS_74 += $(CONFIG)/obj/mixed.o
DEPS_74 += $(CONFIG)/obj/module.o
DEPS_74 += $(CONFIG)/obj/mpr.o
DEPS_74 += $(CONFIG)/obj/path.o
DEPS_74 += $(CONFIG)/obj/poll.o
DEPS_74 += $(CONFIG)/obj/posix.o
DEPS_74 += $(CONFIG)/obj/printf.o
DEPS_74 += $(CONFIG)/obj/rom.o
DEPS_74 += $(CONFIG)/obj/select.o
DEPS_74 += $(CONFIG)/obj/signal.o
DEPS_74 += $(CONFIG)/obj/socket.o
DEPS_74 += $(CONFIG)/obj/string.o
DEPS_74 += $(CONFIG)/obj/test.o
DEPS_74 += $(CONFIG)/obj/thread.o
DEPS_74 += $(CONFIG)/obj/time.o
DEPS_74 += $(CONFIG)/obj/vxworks.o
DEPS_74 += $(CONFIG)/obj/wait.o
DEPS_74 += $(CONFIG)/obj/wide.o
DEPS_74 += $(CONFIG)/obj/win.o
DEPS_74 += $(CONFIG)/obj/wince.o
DEPS_74 += $(CONFIG)/obj/xml.o
DEPS_74 += $(CONFIG)/bin/libmpr.so
DEPS_74 += $(CONFIG)/obj/runProgram.o
DEPS_74 += $(CONFIG)/bin/runProgram
DEPS_74 += $(CONFIG)/obj/testArgv.o
DEPS_74 += $(CONFIG)/obj/testBuf.o
DEPS_74 += $(CONFIG)/obj/testCmd.o
DEPS_74 += $(CONFIG)/obj/testCond.o
DEPS_74 += $(CONFIG)/obj/testEvent.o
DEPS_74 += $(CONFIG)/obj/testFile.o
DEPS_74 += $(CONFIG)/obj/testHash.o
DEPS_74 += $(CONFIG)/obj/testList.o
DEPS_74 += $(CONFIG)/obj/testLock.o
DEPS_74 += $(CONFIG)/obj/testMem.o
DEPS_74 += $(CONFIG)/obj/testMpr.o
DEPS_74 += $(CONFIG)/obj/testPath.o
DEPS_74 += $(CONFIG)/obj/testSocket.o
DEPS_74 += $(CONFIG)/obj/testSprintf.o
DEPS_74 += $(CONFIG)/obj/testThread.o
DEPS_74 += $(CONFIG)/obj/testTime.o
DEPS_74 += $(CONFIG)/obj/testUnicode.o

LIBS_74 += -lmpr

$(CONFIG)/bin/testMpr: $(DEPS_74)
	@echo '      [Link] $(CONFIG)/bin/testMpr'
	$(CC) -o $(CONFIG)/bin/testMpr $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/testArgv.o $(CONFIG)/obj/testBuf.o $(CONFIG)/obj/testCmd.o $(CONFIG)/obj/testCond.o $(CONFIG)/obj/testEvent.o $(CONFIG)/obj/testFile.o $(CONFIG)/obj/testHash.o $(CONFIG)/obj/testList.o $(CONFIG)/obj/testLock.o $(CONFIG)/obj/testMem.o $(CONFIG)/obj/testMpr.o $(CONFIG)/obj/testPath.o $(CONFIG)/obj/testSocket.o $(CONFIG)/obj/testSprintf.o $(CONFIG)/obj/testThread.o $(CONFIG)/obj/testTime.o $(CONFIG)/obj/testUnicode.o $(LIBPATHS_74) $(LIBS_74) $(LIBS_74) $(LIBS) -lpthread -lm -ldl $(LDFLAGS) 

#
#   est.o
#
DEPS_75 += $(CONFIG)/inc/bit.h
DEPS_75 += $(CONFIG)/inc/mpr.h
DEPS_75 += $(CONFIG)/inc/est.h

$(CONFIG)/obj/est.o: \
    src/ssl/est.c $(DEPS_75)
	@echo '   [Compile] $(CONFIG)/obj/est.o'
	$(CC) -c -o $(CONFIG)/obj/est.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) -I$(BIT_PACK_MATRIXSSL_PATH) -I$(BIT_PACK_MATRIXSSL_PATH)/matrixssl -I$(BIT_PACK_NANOSSL_PATH)/src -I$(BIT_PACK_OPENSSL_PATH)/include src/ssl/est.c

#
#   matrixssl.o
#
DEPS_76 += $(CONFIG)/inc/bit.h
DEPS_76 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/matrixssl.o: \
    src/ssl/matrixssl.c $(DEPS_76)
	@echo '   [Compile] $(CONFIG)/obj/matrixssl.o'
	$(CC) -c -o $(CONFIG)/obj/matrixssl.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) -I$(BIT_PACK_MATRIXSSL_PATH) -I$(BIT_PACK_MATRIXSSL_PATH)/matrixssl -I$(BIT_PACK_NANOSSL_PATH)/src -I$(BIT_PACK_OPENSSL_PATH)/include src/ssl/matrixssl.c

#
#   nanossl.o
#
DEPS_77 += $(CONFIG)/inc/bit.h
DEPS_77 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/nanossl.o: \
    src/ssl/nanossl.c $(DEPS_77)
	@echo '   [Compile] $(CONFIG)/obj/nanossl.o'
	$(CC) -c -o $(CONFIG)/obj/nanossl.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) -I$(BIT_PACK_MATRIXSSL_PATH) -I$(BIT_PACK_MATRIXSSL_PATH)/matrixssl -I$(BIT_PACK_NANOSSL_PATH)/src -I$(BIT_PACK_OPENSSL_PATH)/include src/ssl/nanossl.c

#
#   openssl.o
#
DEPS_78 += $(CONFIG)/inc/bit.h
DEPS_78 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/openssl.o: \
    src/ssl/openssl.c $(DEPS_78)
	@echo '   [Compile] $(CONFIG)/obj/openssl.o'
	$(CC) -c -o $(CONFIG)/obj/openssl.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) -I$(BIT_PACK_MATRIXSSL_PATH) -I$(BIT_PACK_MATRIXSSL_PATH)/matrixssl -I$(BIT_PACK_NANOSSL_PATH)/src -I$(BIT_PACK_OPENSSL_PATH)/include src/ssl/openssl.c

#
#   ssl.o
#
DEPS_79 += $(CONFIG)/inc/bit.h
DEPS_79 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/ssl.o: \
    src/ssl/ssl.c $(DEPS_79)
	@echo '   [Compile] $(CONFIG)/obj/ssl.o'
	$(CC) -c -o $(CONFIG)/obj/ssl.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) -I$(BIT_PACK_MATRIXSSL_PATH) -I$(BIT_PACK_MATRIXSSL_PATH)/matrixssl -I$(BIT_PACK_NANOSSL_PATH)/src -I$(BIT_PACK_OPENSSL_PATH)/include src/ssl/ssl.c

#
#   libmprssl
#
DEPS_80 += $(CONFIG)/inc/bit.h
DEPS_80 += $(CONFIG)/inc/bitos.h
DEPS_80 += $(CONFIG)/inc/mpr.h
DEPS_80 += $(CONFIG)/obj/async.o
DEPS_80 += $(CONFIG)/obj/atomic.o
DEPS_80 += $(CONFIG)/obj/buf.o
DEPS_80 += $(CONFIG)/obj/cache.o
DEPS_80 += $(CONFIG)/obj/cmd.o
DEPS_80 += $(CONFIG)/obj/cond.o
DEPS_80 += $(CONFIG)/obj/crypt.o
DEPS_80 += $(CONFIG)/obj/disk.o
DEPS_80 += $(CONFIG)/obj/dispatcher.o
DEPS_80 += $(CONFIG)/obj/encode.o
DEPS_80 += $(CONFIG)/obj/epoll.o
DEPS_80 += $(CONFIG)/obj/event.o
DEPS_80 += $(CONFIG)/obj/file.o
DEPS_80 += $(CONFIG)/obj/fs.o
DEPS_80 += $(CONFIG)/obj/hash.o
DEPS_80 += $(CONFIG)/obj/json.o
DEPS_80 += $(CONFIG)/obj/kqueue.o
DEPS_80 += $(CONFIG)/obj/list.o
DEPS_80 += $(CONFIG)/obj/lock.o
DEPS_80 += $(CONFIG)/obj/log.o
DEPS_80 += $(CONFIG)/obj/mem.o
DEPS_80 += $(CONFIG)/obj/mime.o
DEPS_80 += $(CONFIG)/obj/mixed.o
DEPS_80 += $(CONFIG)/obj/module.o
DEPS_80 += $(CONFIG)/obj/mpr.o
DEPS_80 += $(CONFIG)/obj/path.o
DEPS_80 += $(CONFIG)/obj/poll.o
DEPS_80 += $(CONFIG)/obj/posix.o
DEPS_80 += $(CONFIG)/obj/printf.o
DEPS_80 += $(CONFIG)/obj/rom.o
DEPS_80 += $(CONFIG)/obj/select.o
DEPS_80 += $(CONFIG)/obj/signal.o
DEPS_80 += $(CONFIG)/obj/socket.o
DEPS_80 += $(CONFIG)/obj/string.o
DEPS_80 += $(CONFIG)/obj/test.o
DEPS_80 += $(CONFIG)/obj/thread.o
DEPS_80 += $(CONFIG)/obj/time.o
DEPS_80 += $(CONFIG)/obj/vxworks.o
DEPS_80 += $(CONFIG)/obj/wait.o
DEPS_80 += $(CONFIG)/obj/wide.o
DEPS_80 += $(CONFIG)/obj/win.o
DEPS_80 += $(CONFIG)/obj/wince.o
DEPS_80 += $(CONFIG)/obj/xml.o
DEPS_80 += $(CONFIG)/bin/libmpr.so
DEPS_80 += $(CONFIG)/inc/est.h
DEPS_80 += $(CONFIG)/obj/estLib.o
ifeq ($(BIT_PACK_EST),1)
    DEPS_80 += $(CONFIG)/bin/libest.so
endif
DEPS_80 += $(CONFIG)/obj/est.o
DEPS_80 += $(CONFIG)/obj/matrixssl.o
DEPS_80 += $(CONFIG)/obj/nanossl.o
DEPS_80 += $(CONFIG)/obj/openssl.o
DEPS_80 += $(CONFIG)/obj/ssl.o

ifeq ($(BIT_PACK_EST),1)
    LIBS_80 += -lest
endif
ifeq ($(BIT_PACK_MATRIXSSL),1)
    LIBS_80 += -lmatrixssl
    LIBPATHS_80 += -L$(BIT_PACK_MATRIXSSL_PATH)
endif
ifeq ($(BIT_PACK_NANOSSL),1)
    LIBS_80 += -lssls
    LIBPATHS_80 += -L$(BIT_PACK_NANOSSL_PATH)/bin
endif
ifeq ($(BIT_PACK_OPENSSL),1)
    LIBS_80 += -lssl
    LIBPATHS_80 += -L$(BIT_PACK_OPENSSL_PATH)
endif
ifeq ($(BIT_PACK_OPENSSL),1)
    LIBS_80 += -lcrypto
    LIBPATHS_80 += -L$(BIT_PACK_OPENSSL_PATH)
endif
LIBS_80 += -lmpr

$(CONFIG)/bin/libmprssl.so: $(DEPS_80)
	@echo '      [Link] $(CONFIG)/bin/libmprssl.so'
	$(CC) -shared -o $(CONFIG)/bin/libmprssl.so $(LDFLAGS) $(LIBPATHS)    $(CONFIG)/obj/est.o $(CONFIG)/obj/matrixssl.o $(CONFIG)/obj/nanossl.o $(CONFIG)/obj/openssl.o $(CONFIG)/obj/ssl.o $(LIBPATHS_80) $(LIBS_80) $(LIBS_80) $(LIBS) 

#
#   manager.o
#
DEPS_81 += $(CONFIG)/inc/bit.h
DEPS_81 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/manager.o: \
    src/manager.c $(DEPS_81)
	@echo '   [Compile] $(CONFIG)/obj/manager.o'
	$(CC) -c -o $(CONFIG)/obj/manager.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/manager.c

#
#   manager
#
DEPS_82 += $(CONFIG)/inc/bit.h
DEPS_82 += $(CONFIG)/inc/bitos.h
DEPS_82 += $(CONFIG)/inc/mpr.h
DEPS_82 += $(CONFIG)/obj/async.o
DEPS_82 += $(CONFIG)/obj/atomic.o
DEPS_82 += $(CONFIG)/obj/buf.o
DEPS_82 += $(CONFIG)/obj/cache.o
DEPS_82 += $(CONFIG)/obj/cmd.o
DEPS_82 += $(CONFIG)/obj/cond.o
DEPS_82 += $(CONFIG)/obj/crypt.o
DEPS_82 += $(CONFIG)/obj/disk.o
DEPS_82 += $(CONFIG)/obj/dispatcher.o
DEPS_82 += $(CONFIG)/obj/encode.o
DEPS_82 += $(CONFIG)/obj/epoll.o
DEPS_82 += $(CONFIG)/obj/event.o
DEPS_82 += $(CONFIG)/obj/file.o
DEPS_82 += $(CONFIG)/obj/fs.o
DEPS_82 += $(CONFIG)/obj/hash.o
DEPS_82 += $(CONFIG)/obj/json.o
DEPS_82 += $(CONFIG)/obj/kqueue.o
DEPS_82 += $(CONFIG)/obj/list.o
DEPS_82 += $(CONFIG)/obj/lock.o
DEPS_82 += $(CONFIG)/obj/log.o
DEPS_82 += $(CONFIG)/obj/mem.o
DEPS_82 += $(CONFIG)/obj/mime.o
DEPS_82 += $(CONFIG)/obj/mixed.o
DEPS_82 += $(CONFIG)/obj/module.o
DEPS_82 += $(CONFIG)/obj/mpr.o
DEPS_82 += $(CONFIG)/obj/path.o
DEPS_82 += $(CONFIG)/obj/poll.o
DEPS_82 += $(CONFIG)/obj/posix.o
DEPS_82 += $(CONFIG)/obj/printf.o
DEPS_82 += $(CONFIG)/obj/rom.o
DEPS_82 += $(CONFIG)/obj/select.o
DEPS_82 += $(CONFIG)/obj/signal.o
DEPS_82 += $(CONFIG)/obj/socket.o
DEPS_82 += $(CONFIG)/obj/string.o
DEPS_82 += $(CONFIG)/obj/test.o
DEPS_82 += $(CONFIG)/obj/thread.o
DEPS_82 += $(CONFIG)/obj/time.o
DEPS_82 += $(CONFIG)/obj/vxworks.o
DEPS_82 += $(CONFIG)/obj/wait.o
DEPS_82 += $(CONFIG)/obj/wide.o
DEPS_82 += $(CONFIG)/obj/win.o
DEPS_82 += $(CONFIG)/obj/wince.o
DEPS_82 += $(CONFIG)/obj/xml.o
DEPS_82 += $(CONFIG)/bin/libmpr.so
DEPS_82 += $(CONFIG)/obj/manager.o

LIBS_82 += -lmpr

$(CONFIG)/bin/manager: $(DEPS_82)
	@echo '      [Link] $(CONFIG)/bin/manager'
	$(CC) -o $(CONFIG)/bin/manager $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/manager.o $(LIBPATHS_82) $(LIBS_82) $(LIBS_82) $(LIBS) -lpthread -lm -ldl $(LDFLAGS) 

#
#   makerom.o
#
DEPS_83 += $(CONFIG)/inc/bit.h
DEPS_83 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/makerom.o: \
    src/utils/makerom.c $(DEPS_83)
	@echo '   [Compile] $(CONFIG)/obj/makerom.o'
	$(CC) -c -o $(CONFIG)/obj/makerom.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/utils/makerom.c

#
#   makerom
#
DEPS_84 += $(CONFIG)/inc/bit.h
DEPS_84 += $(CONFIG)/inc/bitos.h
DEPS_84 += $(CONFIG)/inc/mpr.h
DEPS_84 += $(CONFIG)/obj/async.o
DEPS_84 += $(CONFIG)/obj/atomic.o
DEPS_84 += $(CONFIG)/obj/buf.o
DEPS_84 += $(CONFIG)/obj/cache.o
DEPS_84 += $(CONFIG)/obj/cmd.o
DEPS_84 += $(CONFIG)/obj/cond.o
DEPS_84 += $(CONFIG)/obj/crypt.o
DEPS_84 += $(CONFIG)/obj/disk.o
DEPS_84 += $(CONFIG)/obj/dispatcher.o
DEPS_84 += $(CONFIG)/obj/encode.o
DEPS_84 += $(CONFIG)/obj/epoll.o
DEPS_84 += $(CONFIG)/obj/event.o
DEPS_84 += $(CONFIG)/obj/file.o
DEPS_84 += $(CONFIG)/obj/fs.o
DEPS_84 += $(CONFIG)/obj/hash.o
DEPS_84 += $(CONFIG)/obj/json.o
DEPS_84 += $(CONFIG)/obj/kqueue.o
DEPS_84 += $(CONFIG)/obj/list.o
DEPS_84 += $(CONFIG)/obj/lock.o
DEPS_84 += $(CONFIG)/obj/log.o
DEPS_84 += $(CONFIG)/obj/mem.o
DEPS_84 += $(CONFIG)/obj/mime.o
DEPS_84 += $(CONFIG)/obj/mixed.o
DEPS_84 += $(CONFIG)/obj/module.o
DEPS_84 += $(CONFIG)/obj/mpr.o
DEPS_84 += $(CONFIG)/obj/path.o
DEPS_84 += $(CONFIG)/obj/poll.o
DEPS_84 += $(CONFIG)/obj/posix.o
DEPS_84 += $(CONFIG)/obj/printf.o
DEPS_84 += $(CONFIG)/obj/rom.o
DEPS_84 += $(CONFIG)/obj/select.o
DEPS_84 += $(CONFIG)/obj/signal.o
DEPS_84 += $(CONFIG)/obj/socket.o
DEPS_84 += $(CONFIG)/obj/string.o
DEPS_84 += $(CONFIG)/obj/test.o
DEPS_84 += $(CONFIG)/obj/thread.o
DEPS_84 += $(CONFIG)/obj/time.o
DEPS_84 += $(CONFIG)/obj/vxworks.o
DEPS_84 += $(CONFIG)/obj/wait.o
DEPS_84 += $(CONFIG)/obj/wide.o
DEPS_84 += $(CONFIG)/obj/win.o
DEPS_84 += $(CONFIG)/obj/wince.o
DEPS_84 += $(CONFIG)/obj/xml.o
DEPS_84 += $(CONFIG)/bin/libmpr.so
DEPS_84 += $(CONFIG)/obj/makerom.o

LIBS_84 += -lmpr

$(CONFIG)/bin/makerom: $(DEPS_84)
	@echo '      [Link] $(CONFIG)/bin/makerom'
	$(CC) -o $(CONFIG)/bin/makerom $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/makerom.o $(LIBPATHS_84) $(LIBS_84) $(LIBS_84) $(LIBS) -lpthread -lm -ldl $(LDFLAGS) 

#
#   charGen.o
#
DEPS_85 += $(CONFIG)/inc/bit.h
DEPS_85 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/charGen.o: \
    src/utils/charGen.c $(DEPS_85)
	@echo '   [Compile] $(CONFIG)/obj/charGen.o'
	$(CC) -c -o $(CONFIG)/obj/charGen.o -fPIC $(LDFLAGS) $(DFLAGS) $(IFLAGS) src/utils/charGen.c

#
#   chargen
#
DEPS_86 += $(CONFIG)/inc/bit.h
DEPS_86 += $(CONFIG)/inc/bitos.h
DEPS_86 += $(CONFIG)/inc/mpr.h
DEPS_86 += $(CONFIG)/obj/async.o
DEPS_86 += $(CONFIG)/obj/atomic.o
DEPS_86 += $(CONFIG)/obj/buf.o
DEPS_86 += $(CONFIG)/obj/cache.o
DEPS_86 += $(CONFIG)/obj/cmd.o
DEPS_86 += $(CONFIG)/obj/cond.o
DEPS_86 += $(CONFIG)/obj/crypt.o
DEPS_86 += $(CONFIG)/obj/disk.o
DEPS_86 += $(CONFIG)/obj/dispatcher.o
DEPS_86 += $(CONFIG)/obj/encode.o
DEPS_86 += $(CONFIG)/obj/epoll.o
DEPS_86 += $(CONFIG)/obj/event.o
DEPS_86 += $(CONFIG)/obj/file.o
DEPS_86 += $(CONFIG)/obj/fs.o
DEPS_86 += $(CONFIG)/obj/hash.o
DEPS_86 += $(CONFIG)/obj/json.o
DEPS_86 += $(CONFIG)/obj/kqueue.o
DEPS_86 += $(CONFIG)/obj/list.o
DEPS_86 += $(CONFIG)/obj/lock.o
DEPS_86 += $(CONFIG)/obj/log.o
DEPS_86 += $(CONFIG)/obj/mem.o
DEPS_86 += $(CONFIG)/obj/mime.o
DEPS_86 += $(CONFIG)/obj/mixed.o
DEPS_86 += $(CONFIG)/obj/module.o
DEPS_86 += $(CONFIG)/obj/mpr.o
DEPS_86 += $(CONFIG)/obj/path.o
DEPS_86 += $(CONFIG)/obj/poll.o
DEPS_86 += $(CONFIG)/obj/posix.o
DEPS_86 += $(CONFIG)/obj/printf.o
DEPS_86 += $(CONFIG)/obj/rom.o
DEPS_86 += $(CONFIG)/obj/select.o
DEPS_86 += $(CONFIG)/obj/signal.o
DEPS_86 += $(CONFIG)/obj/socket.o
DEPS_86 += $(CONFIG)/obj/string.o
DEPS_86 += $(CONFIG)/obj/test.o
DEPS_86 += $(CONFIG)/obj/thread.o
DEPS_86 += $(CONFIG)/obj/time.o
DEPS_86 += $(CONFIG)/obj/vxworks.o
DEPS_86 += $(CONFIG)/obj/wait.o
DEPS_86 += $(CONFIG)/obj/wide.o
DEPS_86 += $(CONFIG)/obj/win.o
DEPS_86 += $(CONFIG)/obj/wince.o
DEPS_86 += $(CONFIG)/obj/xml.o
DEPS_86 += $(CONFIG)/bin/libmpr.so
DEPS_86 += $(CONFIG)/obj/charGen.o

LIBS_86 += -lmpr

$(CONFIG)/bin/chargen: $(DEPS_86)
	@echo '      [Link] $(CONFIG)/bin/chargen'
	$(CC) -o $(CONFIG)/bin/chargen $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/charGen.o $(LIBPATHS_86) $(LIBS_86) $(LIBS_86) $(LIBS) -lpthread -lm -ldl $(LDFLAGS) 

#
#   stop
#
stop: $(DEPS_87)

#
#   installBinary
#
installBinary: $(DEPS_88)

#
#   start
#
start: $(DEPS_89)

#
#   install
#
DEPS_90 += stop
DEPS_90 += installBinary
DEPS_90 += start

install: $(DEPS_90)
	

#
#   uninstall
#
DEPS_91 += stop

uninstall: $(DEPS_91)

