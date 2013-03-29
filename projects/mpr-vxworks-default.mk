#
#   mpr-vxworks-default.mk -- Makefile to build Multithreaded Portable Runtime for vxworks
#

PRODUCT            := mpr
VERSION            := 4.3.0
BUILD_NUMBER       := 0
PROFILE            := default
ARCH               := $(shell echo $(WIND_HOST_TYPE) | sed 's/-.*//')
CPU                := $(subst X86,PENTIUM,$(shell echo $(ARCH) | tr a-z A-Z))
OS                 := vxworks
CC                 := cc$(subst x86,pentium,$(ARCH))
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

BIT_PACK_COMPILER_PATH    := cc$(subst x86,pentium,$(ARCH))
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
BIT_PACK_VXWORKS_PATH     := $(WIND_BASE)

export WIND_HOME          := $(WIND_BASE)/..
export PATH               := $(WIND_GNU_PATH)/$(WIND_HOST_TYPE)/bin:$(PATH)

CFLAGS             += -fno-builtin -fno-defer-pop -fvolatile -w
DFLAGS             += -D_REENTRANT -DVXWORKS -DRW_MULTI_THREAD -D_GNU_TOOL -DCPU=$(CPU) $(patsubst %,-D%,$(filter BIT_%,$(MAKEFLAGS))) -DBIT_PACK_EST=$(BIT_PACK_EST) -DBIT_PACK_MATRIXSSL=$(BIT_PACK_MATRIXSSL) -DBIT_PACK_OPENSSL=$(BIT_PACK_OPENSSL) -DBIT_PACK_SSL=$(BIT_PACK_SSL) 
IFLAGS             += -I$(CONFIG)/inc -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/h/wrn/coreip
LDFLAGS            += '-Wl,-r'
LIBPATHS           += -L$(CONFIG)/bin
LIBS               += -lgcc

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

BIT_ROOT_PREFIX    := deploy
BIT_BASE_PREFIX    := $(BIT_ROOT_PREFIX)
BIT_DATA_PREFIX    := $(BIT_VAPP_PREFIX)
BIT_STATE_PREFIX   := $(BIT_VAPP_PREFIX)
BIT_BIN_PREFIX     := $(BIT_VAPP_PREFIX)
BIT_INC_PREFIX     := $(BIT_VAPP_PREFIX)/inc
BIT_LIB_PREFIX     := $(BIT_VAPP_PREFIX)
BIT_MAN_PREFIX     := $(BIT_VAPP_PREFIX)
BIT_SBIN_PREFIX    := $(BIT_VAPP_PREFIX)
BIT_ETC_PREFIX     := $(BIT_VAPP_PREFIX)
BIT_WEB_PREFIX     := $(BIT_VAPP_PREFIX)/web
BIT_LOG_PREFIX     := $(BIT_VAPP_PREFIX)
BIT_SPOOL_PREFIX   := $(BIT_VAPP_PREFIX)
BIT_CACHE_PREFIX   := $(BIT_VAPP_PREFIX)
BIT_APP_PREFIX     := $(BIT_BASE_PREFIX)
BIT_VAPP_PREFIX    := $(BIT_APP_PREFIX)
BIT_SRC_PREFIX     := $(BIT_ROOT_PREFIX)/usr/src/$(PRODUCT)-$(VERSION)


ifeq ($(BIT_PACK_EST),1)
TARGETS            += $(CONFIG)/bin/libest.out
endif
TARGETS            += $(CONFIG)/bin/ca.crt
TARGETS            += $(CONFIG)/bin/benchMpr.out
TARGETS            += $(CONFIG)/bin/runProgram.out
TARGETS            += $(CONFIG)/bin/testMpr.out
TARGETS            += $(CONFIG)/bin/manager.out
TARGETS            += $(CONFIG)/bin/makerom.out
TARGETS            += $(CONFIG)/bin/chargen.out

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
	@if [ "$(WIND_BASE)" = "" ] ; then echo WARNING: WIND_BASE not set. Run wrenv.sh. ; exit 255 ; fi
	@if [ "$(WIND_HOST_TYPE)" = "" ] ; then echo WARNING: WIND_HOST_TYPE not set. Run wrenv.sh. ; exit 255 ; fi
	@if [ "$(WIND_GNU_PATH)" = "" ] ; then echo WARNING: WIND_GNU_PATH not set. Run wrenv.sh. ; exit 255 ; fi
	@[ ! -x $(CONFIG)/bin ] && mkdir -p $(CONFIG)/bin; true
	@[ ! -x $(CONFIG)/inc ] && mkdir -p $(CONFIG)/inc; true
	@[ ! -x $(CONFIG)/obj ] && mkdir -p $(CONFIG)/obj; true
	@[ ! -f $(CONFIG)/inc/bit.h ] && cp projects/mpr-vxworks-default-bit.h $(CONFIG)/inc/bit.h ; true
	@[ ! -f $(CONFIG)/inc/bitos.h ] && cp src/bitos.h $(CONFIG)/inc/bitos.h ; true
	@if ! diff $(CONFIG)/inc/bitos.h src/bitos.h >/dev/null ; then\
		cp src/bitos.h $(CONFIG)/inc/bitos.h  ; \
	fi; true
	@if ! diff $(CONFIG)/inc/bit.h projects/mpr-vxworks-default-bit.h >/dev/null ; then\
		cp projects/mpr-vxworks-default-bit.h $(CONFIG)/inc/bit.h  ; \
	fi; true
	@if [ -f "$(CONFIG)/.makeflags" ] ; then \
		if [ "$(MAKEFLAGS)" != " ` cat $(CONFIG)/.makeflags`" ] ; then \
			echo "   [Warning] Make flags have changed since the last build: "`cat $(CONFIG)/.makeflags`"" ; \
		fi ; \
	fi
	@echo $(MAKEFLAGS) >$(CONFIG)/.makeflags
clean:
	rm -f "$(CONFIG)/bin/libest.out"
	rm -fr "$(CONFIG)/bin/ca.crt"
	rm -f "$(CONFIG)/bin/benchMpr.out"
	rm -f "$(CONFIG)/bin/runProgram.out"
	rm -f "$(CONFIG)/bin/testMpr.out"
	rm -f "$(CONFIG)/bin/libmpr.out"
	rm -f "$(CONFIG)/bin/libmprssl.out"
	rm -f "$(CONFIG)/bin/manager.out"
	rm -f "$(CONFIG)/bin/makerom.out"
	rm -f "$(CONFIG)/bin/chargen.out"
	rm -fr "$(CONFIG)/obj/estLib.o"
	rm -fr "$(CONFIG)/obj/benchMpr.o"
	rm -fr "$(CONFIG)/obj/runProgram.o"
	rm -fr "$(CONFIG)/obj/testArgv.o"
	rm -fr "$(CONFIG)/obj/testBuf.o"
	rm -fr "$(CONFIG)/obj/testCmd.o"
	rm -fr "$(CONFIG)/obj/testCond.o"
	rm -fr "$(CONFIG)/obj/testEvent.o"
	rm -fr "$(CONFIG)/obj/testFile.o"
	rm -fr "$(CONFIG)/obj/testHash.o"
	rm -fr "$(CONFIG)/obj/testList.o"
	rm -fr "$(CONFIG)/obj/testLock.o"
	rm -fr "$(CONFIG)/obj/testMem.o"
	rm -fr "$(CONFIG)/obj/testMpr.o"
	rm -fr "$(CONFIG)/obj/testPath.o"
	rm -fr "$(CONFIG)/obj/testSocket.o"
	rm -fr "$(CONFIG)/obj/testSprintf.o"
	rm -fr "$(CONFIG)/obj/testThread.o"
	rm -fr "$(CONFIG)/obj/testTime.o"
	rm -fr "$(CONFIG)/obj/testUnicode.o"
	rm -fr "$(CONFIG)/obj/async.o"
	rm -fr "$(CONFIG)/obj/atomic.o"
	rm -fr "$(CONFIG)/obj/buf.o"
	rm -fr "$(CONFIG)/obj/cache.o"
	rm -fr "$(CONFIG)/obj/cmd.o"
	rm -fr "$(CONFIG)/obj/cond.o"
	rm -fr "$(CONFIG)/obj/crypt.o"
	rm -fr "$(CONFIG)/obj/disk.o"
	rm -fr "$(CONFIG)/obj/dispatcher.o"
	rm -fr "$(CONFIG)/obj/encode.o"
	rm -fr "$(CONFIG)/obj/epoll.o"
	rm -fr "$(CONFIG)/obj/event.o"
	rm -fr "$(CONFIG)/obj/file.o"
	rm -fr "$(CONFIG)/obj/fs.o"
	rm -fr "$(CONFIG)/obj/hash.o"
	rm -fr "$(CONFIG)/obj/json.o"
	rm -fr "$(CONFIG)/obj/kqueue.o"
	rm -fr "$(CONFIG)/obj/list.o"
	rm -fr "$(CONFIG)/obj/lock.o"
	rm -fr "$(CONFIG)/obj/log.o"
	rm -fr "$(CONFIG)/obj/mem.o"
	rm -fr "$(CONFIG)/obj/mime.o"
	rm -fr "$(CONFIG)/obj/mixed.o"
	rm -fr "$(CONFIG)/obj/module.o"
	rm -fr "$(CONFIG)/obj/mpr.o"
	rm -fr "$(CONFIG)/obj/path.o"
	rm -fr "$(CONFIG)/obj/poll.o"
	rm -fr "$(CONFIG)/obj/posix.o"
	rm -fr "$(CONFIG)/obj/printf.o"
	rm -fr "$(CONFIG)/obj/rom.o"
	rm -fr "$(CONFIG)/obj/select.o"
	rm -fr "$(CONFIG)/obj/signal.o"
	rm -fr "$(CONFIG)/obj/socket.o"
	rm -fr "$(CONFIG)/obj/string.o"
	rm -fr "$(CONFIG)/obj/test.o"
	rm -fr "$(CONFIG)/obj/thread.o"
	rm -fr "$(CONFIG)/obj/time.o"
	rm -fr "$(CONFIG)/obj/vxworks.o"
	rm -fr "$(CONFIG)/obj/wait.o"
	rm -fr "$(CONFIG)/obj/wide.o"
	rm -fr "$(CONFIG)/obj/win.o"
	rm -fr "$(CONFIG)/obj/wince.o"
	rm -fr "$(CONFIG)/obj/xml.o"
	rm -fr "$(CONFIG)/obj/est.o"
	rm -fr "$(CONFIG)/obj/matrixssl.o"
	rm -fr "$(CONFIG)/obj/nanossl.o"
	rm -fr "$(CONFIG)/obj/openssl.o"
	rm -fr "$(CONFIG)/obj/ssl.o"
	rm -fr "$(CONFIG)/obj/manager.o"
	rm -fr "$(CONFIG)/obj/makerom.o"
	rm -fr "$(CONFIG)/obj/charGen.o"

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
	@echo '   [Compile] src/deps/est/estLib.c'
	$(CC) -c -o $(CONFIG)/obj/estLib.o -fno-builtin -fno-defer-pop -fvolatile $(DFLAGS) $(IFLAGS) src/deps/est/estLib.c

ifeq ($(BIT_PACK_EST),1)
#
#   libest
#
DEPS_6 += $(CONFIG)/inc/est.h
DEPS_6 += $(CONFIG)/obj/estLib.o

$(CONFIG)/bin/libest.out: $(DEPS_6)
	@echo '      [Link] libest'
	$(CC) -r -o $(CONFIG)/bin/libest.out $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/estLib.o $(LIBS) 
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
#   mem.o
#
DEPS_29 += $(CONFIG)/inc/bit.h
DEPS_29 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mem.o: \
    src/mem.c $(DEPS_29)
	@echo '   [Compile] src/mem.c'
	$(CC) -c -o $(CONFIG)/obj/mem.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mem.c

#
#   mime.o
#
DEPS_30 += $(CONFIG)/inc/bit.h
DEPS_30 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mime.o: \
    src/mime.c $(DEPS_30)
	@echo '   [Compile] src/mime.c'
	$(CC) -c -o $(CONFIG)/obj/mime.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mime.c

#
#   mixed.o
#
DEPS_31 += $(CONFIG)/inc/bit.h
DEPS_31 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mixed.o: \
    src/mixed.c $(DEPS_31)
	@echo '   [Compile] src/mixed.c'
	$(CC) -c -o $(CONFIG)/obj/mixed.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mixed.c

#
#   module.o
#
DEPS_32 += $(CONFIG)/inc/bit.h
DEPS_32 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/module.o: \
    src/module.c $(DEPS_32)
	@echo '   [Compile] src/module.c'
	$(CC) -c -o $(CONFIG)/obj/module.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/module.c

#
#   mpr.o
#
DEPS_33 += $(CONFIG)/inc/bit.h
DEPS_33 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mpr.o: \
    src/mpr.c $(DEPS_33)
	@echo '   [Compile] src/mpr.c'
	$(CC) -c -o $(CONFIG)/obj/mpr.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/mpr.c

#
#   path.o
#
DEPS_34 += $(CONFIG)/inc/bit.h
DEPS_34 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/path.o: \
    src/path.c $(DEPS_34)
	@echo '   [Compile] src/path.c'
	$(CC) -c -o $(CONFIG)/obj/path.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/path.c

#
#   poll.o
#
DEPS_35 += $(CONFIG)/inc/bit.h
DEPS_35 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/poll.o: \
    src/poll.c $(DEPS_35)
	@echo '   [Compile] src/poll.c'
	$(CC) -c -o $(CONFIG)/obj/poll.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/poll.c

#
#   posix.o
#
DEPS_36 += $(CONFIG)/inc/bit.h
DEPS_36 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/posix.o: \
    src/posix.c $(DEPS_36)
	@echo '   [Compile] src/posix.c'
	$(CC) -c -o $(CONFIG)/obj/posix.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/posix.c

#
#   printf.o
#
DEPS_37 += $(CONFIG)/inc/bit.h
DEPS_37 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/printf.o: \
    src/printf.c $(DEPS_37)
	@echo '   [Compile] src/printf.c'
	$(CC) -c -o $(CONFIG)/obj/printf.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/printf.c

#
#   rom.o
#
DEPS_38 += $(CONFIG)/inc/bit.h
DEPS_38 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/rom.o: \
    src/rom.c $(DEPS_38)
	@echo '   [Compile] src/rom.c'
	$(CC) -c -o $(CONFIG)/obj/rom.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/rom.c

#
#   select.o
#
DEPS_39 += $(CONFIG)/inc/bit.h
DEPS_39 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/select.o: \
    src/select.c $(DEPS_39)
	@echo '   [Compile] src/select.c'
	$(CC) -c -o $(CONFIG)/obj/select.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/select.c

#
#   signal.o
#
DEPS_40 += $(CONFIG)/inc/bit.h
DEPS_40 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/signal.o: \
    src/signal.c $(DEPS_40)
	@echo '   [Compile] src/signal.c'
	$(CC) -c -o $(CONFIG)/obj/signal.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/signal.c

#
#   socket.o
#
DEPS_41 += $(CONFIG)/inc/bit.h
DEPS_41 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/socket.o: \
    src/socket.c $(DEPS_41)
	@echo '   [Compile] src/socket.c'
	$(CC) -c -o $(CONFIG)/obj/socket.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/socket.c

#
#   string.o
#
DEPS_42 += $(CONFIG)/inc/bit.h
DEPS_42 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/string.o: \
    src/string.c $(DEPS_42)
	@echo '   [Compile] src/string.c'
	$(CC) -c -o $(CONFIG)/obj/string.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/string.c

#
#   test.o
#
DEPS_43 += $(CONFIG)/inc/bit.h
DEPS_43 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/test.o: \
    src/test.c $(DEPS_43)
	@echo '   [Compile] src/test.c'
	$(CC) -c -o $(CONFIG)/obj/test.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/test.c

#
#   thread.o
#
DEPS_44 += $(CONFIG)/inc/bit.h
DEPS_44 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/thread.o: \
    src/thread.c $(DEPS_44)
	@echo '   [Compile] src/thread.c'
	$(CC) -c -o $(CONFIG)/obj/thread.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/thread.c

#
#   time.o
#
DEPS_45 += $(CONFIG)/inc/bit.h
DEPS_45 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/time.o: \
    src/time.c $(DEPS_45)
	@echo '   [Compile] src/time.c'
	$(CC) -c -o $(CONFIG)/obj/time.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/time.c

#
#   vxworks.o
#
DEPS_46 += $(CONFIG)/inc/bit.h
DEPS_46 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/vxworks.o: \
    src/vxworks.c $(DEPS_46)
	@echo '   [Compile] src/vxworks.c'
	$(CC) -c -o $(CONFIG)/obj/vxworks.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/vxworks.c

#
#   wait.o
#
DEPS_47 += $(CONFIG)/inc/bit.h
DEPS_47 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wait.o: \
    src/wait.c $(DEPS_47)
	@echo '   [Compile] src/wait.c'
	$(CC) -c -o $(CONFIG)/obj/wait.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/wait.c

#
#   wide.o
#
DEPS_48 += $(CONFIG)/inc/bit.h
DEPS_48 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wide.o: \
    src/wide.c $(DEPS_48)
	@echo '   [Compile] src/wide.c'
	$(CC) -c -o $(CONFIG)/obj/wide.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/wide.c

#
#   win.o
#
DEPS_49 += $(CONFIG)/inc/bit.h
DEPS_49 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/win.o: \
    src/win.c $(DEPS_49)
	@echo '   [Compile] src/win.c'
	$(CC) -c -o $(CONFIG)/obj/win.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/win.c

#
#   wince.o
#
DEPS_50 += $(CONFIG)/inc/bit.h
DEPS_50 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wince.o: \
    src/wince.c $(DEPS_50)
	@echo '   [Compile] src/wince.c'
	$(CC) -c -o $(CONFIG)/obj/wince.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/wince.c

#
#   xml.o
#
DEPS_51 += $(CONFIG)/inc/bit.h
DEPS_51 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/xml.o: \
    src/xml.c $(DEPS_51)
	@echo '   [Compile] src/xml.c'
	$(CC) -c -o $(CONFIG)/obj/xml.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/xml.c

#
#   libmpr
#
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

$(CONFIG)/bin/libmpr.out: $(DEPS_52)
	@echo '      [Link] libmpr'
	$(CC) -r -o $(CONFIG)/bin/libmpr.out $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/async.o $(CONFIG)/obj/atomic.o $(CONFIG)/obj/buf.o $(CONFIG)/obj/cache.o $(CONFIG)/obj/cmd.o $(CONFIG)/obj/cond.o $(CONFIG)/obj/crypt.o $(CONFIG)/obj/disk.o $(CONFIG)/obj/dispatcher.o $(CONFIG)/obj/encode.o $(CONFIG)/obj/epoll.o $(CONFIG)/obj/event.o $(CONFIG)/obj/file.o $(CONFIG)/obj/fs.o $(CONFIG)/obj/hash.o $(CONFIG)/obj/json.o $(CONFIG)/obj/kqueue.o $(CONFIG)/obj/list.o $(CONFIG)/obj/lock.o $(CONFIG)/obj/log.o $(CONFIG)/obj/mem.o $(CONFIG)/obj/mime.o $(CONFIG)/obj/mixed.o $(CONFIG)/obj/module.o $(CONFIG)/obj/mpr.o $(CONFIG)/obj/path.o $(CONFIG)/obj/poll.o $(CONFIG)/obj/posix.o $(CONFIG)/obj/printf.o $(CONFIG)/obj/rom.o $(CONFIG)/obj/select.o $(CONFIG)/obj/signal.o $(CONFIG)/obj/socket.o $(CONFIG)/obj/string.o $(CONFIG)/obj/test.o $(CONFIG)/obj/thread.o $(CONFIG)/obj/time.o $(CONFIG)/obj/vxworks.o $(CONFIG)/obj/wait.o $(CONFIG)/obj/wide.o $(CONFIG)/obj/win.o $(CONFIG)/obj/wince.o $(CONFIG)/obj/xml.o $(LIBS) 

#
#   benchMpr.o
#
DEPS_53 += $(CONFIG)/inc/bit.h
DEPS_53 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/benchMpr.o: \
    test/benchMpr.c $(DEPS_53)
	@echo '   [Compile] test/benchMpr.c'
	$(CC) -c -o $(CONFIG)/obj/benchMpr.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/benchMpr.c

#
#   benchMpr
#
DEPS_54 += $(CONFIG)/bin/libmpr.out
DEPS_54 += $(CONFIG)/obj/benchMpr.o

$(CONFIG)/bin/benchMpr.out: $(DEPS_54)
	@echo '      [Link] benchMpr'
	$(CC) -o $(CONFIG)/bin/benchMpr.out $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/benchMpr.o $(LIBS) $(LDFLAGS) 

#
#   runProgram.o
#
DEPS_55 += $(CONFIG)/inc/bit.h

$(CONFIG)/obj/runProgram.o: \
    test/runProgram.c $(DEPS_55)
	@echo '   [Compile] test/runProgram.c'
	$(CC) -c -o $(CONFIG)/obj/runProgram.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/runProgram.c

#
#   runProgram
#
DEPS_56 += $(CONFIG)/obj/runProgram.o

$(CONFIG)/bin/runProgram.out: $(DEPS_56)
	@echo '      [Link] runProgram'
	$(CC) -o $(CONFIG)/bin/runProgram.out $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/runProgram.o $(LIBS) $(LDFLAGS) 

#
#   est.o
#
DEPS_57 += $(CONFIG)/inc/bit.h
DEPS_57 += $(CONFIG)/inc/mpr.h
DEPS_57 += $(CONFIG)/inc/est.h

$(CONFIG)/obj/est.o: \
    src/ssl/est.c $(DEPS_57)
	@echo '   [Compile] src/ssl/est.c'
	$(CC) -c -o $(CONFIG)/obj/est.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/ssl/est.c

#
#   matrixssl.o
#
DEPS_58 += $(CONFIG)/inc/bit.h
DEPS_58 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/matrixssl.o: \
    src/ssl/matrixssl.c $(DEPS_58)
	@echo '   [Compile] src/ssl/matrixssl.c'
	$(CC) -c -o $(CONFIG)/obj/matrixssl.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/ssl/matrixssl.c

#
#   nanossl.o
#
DEPS_59 += $(CONFIG)/inc/bit.h
DEPS_59 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/nanossl.o: \
    src/ssl/nanossl.c $(DEPS_59)
	@echo '   [Compile] src/ssl/nanossl.c'
	$(CC) -c -o $(CONFIG)/obj/nanossl.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/ssl/nanossl.c

#
#   openssl.o
#
DEPS_60 += $(CONFIG)/inc/bit.h
DEPS_60 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/openssl.o: \
    src/ssl/openssl.c $(DEPS_60)
	@echo '   [Compile] src/ssl/openssl.c'
	$(CC) -c -o $(CONFIG)/obj/openssl.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/ssl/openssl.c

#
#   ssl.o
#
DEPS_61 += $(CONFIG)/inc/bit.h
DEPS_61 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/ssl.o: \
    src/ssl/ssl.c $(DEPS_61)
	@echo '   [Compile] src/ssl/ssl.c'
	$(CC) -c -o $(CONFIG)/obj/ssl.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/ssl/ssl.c

#
#   libmprssl
#
DEPS_62 += $(CONFIG)/bin/libmpr.out
DEPS_62 += $(CONFIG)/obj/est.o
DEPS_62 += $(CONFIG)/obj/matrixssl.o
DEPS_62 += $(CONFIG)/obj/nanossl.o
DEPS_62 += $(CONFIG)/obj/openssl.o
DEPS_62 += $(CONFIG)/obj/ssl.o

$(CONFIG)/bin/libmprssl.out: $(DEPS_62)
	@echo '      [Link] libmprssl'
	$(CC) -r -o $(CONFIG)/bin/libmprssl.out $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/est.o $(CONFIG)/obj/matrixssl.o $(CONFIG)/obj/nanossl.o $(CONFIG)/obj/openssl.o $(CONFIG)/obj/ssl.o $(LIBS) 

#
#   testArgv.o
#
DEPS_63 += $(CONFIG)/inc/bit.h
DEPS_63 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testArgv.o: \
    test/testArgv.c $(DEPS_63)
	@echo '   [Compile] test/testArgv.c'
	$(CC) -c -o $(CONFIG)/obj/testArgv.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testArgv.c

#
#   testBuf.o
#
DEPS_64 += $(CONFIG)/inc/bit.h
DEPS_64 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testBuf.o: \
    test/testBuf.c $(DEPS_64)
	@echo '   [Compile] test/testBuf.c'
	$(CC) -c -o $(CONFIG)/obj/testBuf.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testBuf.c

#
#   testCmd.o
#
DEPS_65 += $(CONFIG)/inc/bit.h
DEPS_65 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testCmd.o: \
    test/testCmd.c $(DEPS_65)
	@echo '   [Compile] test/testCmd.c'
	$(CC) -c -o $(CONFIG)/obj/testCmd.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testCmd.c

#
#   testCond.o
#
DEPS_66 += $(CONFIG)/inc/bit.h
DEPS_66 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testCond.o: \
    test/testCond.c $(DEPS_66)
	@echo '   [Compile] test/testCond.c'
	$(CC) -c -o $(CONFIG)/obj/testCond.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testCond.c

#
#   testEvent.o
#
DEPS_67 += $(CONFIG)/inc/bit.h
DEPS_67 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testEvent.o: \
    test/testEvent.c $(DEPS_67)
	@echo '   [Compile] test/testEvent.c'
	$(CC) -c -o $(CONFIG)/obj/testEvent.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testEvent.c

#
#   testFile.o
#
DEPS_68 += $(CONFIG)/inc/bit.h
DEPS_68 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testFile.o: \
    test/testFile.c $(DEPS_68)
	@echo '   [Compile] test/testFile.c'
	$(CC) -c -o $(CONFIG)/obj/testFile.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testFile.c

#
#   testHash.o
#
DEPS_69 += $(CONFIG)/inc/bit.h
DEPS_69 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testHash.o: \
    test/testHash.c $(DEPS_69)
	@echo '   [Compile] test/testHash.c'
	$(CC) -c -o $(CONFIG)/obj/testHash.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testHash.c

#
#   testList.o
#
DEPS_70 += $(CONFIG)/inc/bit.h
DEPS_70 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testList.o: \
    test/testList.c $(DEPS_70)
	@echo '   [Compile] test/testList.c'
	$(CC) -c -o $(CONFIG)/obj/testList.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testList.c

#
#   testLock.o
#
DEPS_71 += $(CONFIG)/inc/bit.h
DEPS_71 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testLock.o: \
    test/testLock.c $(DEPS_71)
	@echo '   [Compile] test/testLock.c'
	$(CC) -c -o $(CONFIG)/obj/testLock.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testLock.c

#
#   testMem.o
#
DEPS_72 += $(CONFIG)/inc/bit.h
DEPS_72 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testMem.o: \
    test/testMem.c $(DEPS_72)
	@echo '   [Compile] test/testMem.c'
	$(CC) -c -o $(CONFIG)/obj/testMem.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testMem.c

#
#   testMpr.o
#
DEPS_73 += $(CONFIG)/inc/bit.h
DEPS_73 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testMpr.o: \
    test/testMpr.c $(DEPS_73)
	@echo '   [Compile] test/testMpr.c'
	$(CC) -c -o $(CONFIG)/obj/testMpr.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testMpr.c

#
#   testPath.o
#
DEPS_74 += $(CONFIG)/inc/bit.h
DEPS_74 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testPath.o: \
    test/testPath.c $(DEPS_74)
	@echo '   [Compile] test/testPath.c'
	$(CC) -c -o $(CONFIG)/obj/testPath.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testPath.c

#
#   testSocket.o
#
DEPS_75 += $(CONFIG)/inc/bit.h
DEPS_75 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testSocket.o: \
    test/testSocket.c $(DEPS_75)
	@echo '   [Compile] test/testSocket.c'
	$(CC) -c -o $(CONFIG)/obj/testSocket.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testSocket.c

#
#   testSprintf.o
#
DEPS_76 += $(CONFIG)/inc/bit.h
DEPS_76 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testSprintf.o: \
    test/testSprintf.c $(DEPS_76)
	@echo '   [Compile] test/testSprintf.c'
	$(CC) -c -o $(CONFIG)/obj/testSprintf.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testSprintf.c

#
#   testThread.o
#
DEPS_77 += $(CONFIG)/inc/bit.h
DEPS_77 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testThread.o: \
    test/testThread.c $(DEPS_77)
	@echo '   [Compile] test/testThread.c'
	$(CC) -c -o $(CONFIG)/obj/testThread.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testThread.c

#
#   testTime.o
#
DEPS_78 += $(CONFIG)/inc/bit.h
DEPS_78 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testTime.o: \
    test/testTime.c $(DEPS_78)
	@echo '   [Compile] test/testTime.c'
	$(CC) -c -o $(CONFIG)/obj/testTime.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testTime.c

#
#   testUnicode.o
#
DEPS_79 += $(CONFIG)/inc/bit.h
DEPS_79 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testUnicode.o: \
    test/testUnicode.c $(DEPS_79)
	@echo '   [Compile] test/testUnicode.c'
	$(CC) -c -o $(CONFIG)/obj/testUnicode.o $(CFLAGS) $(DFLAGS) $(IFLAGS) test/testUnicode.c

#
#   testMpr
#
DEPS_80 += $(CONFIG)/bin/libmpr.out
DEPS_80 += $(CONFIG)/bin/libmprssl.out
DEPS_80 += $(CONFIG)/bin/runProgram.out
DEPS_80 += $(CONFIG)/obj/testArgv.o
DEPS_80 += $(CONFIG)/obj/testBuf.o
DEPS_80 += $(CONFIG)/obj/testCmd.o
DEPS_80 += $(CONFIG)/obj/testCond.o
DEPS_80 += $(CONFIG)/obj/testEvent.o
DEPS_80 += $(CONFIG)/obj/testFile.o
DEPS_80 += $(CONFIG)/obj/testHash.o
DEPS_80 += $(CONFIG)/obj/testList.o
DEPS_80 += $(CONFIG)/obj/testLock.o
DEPS_80 += $(CONFIG)/obj/testMem.o
DEPS_80 += $(CONFIG)/obj/testMpr.o
DEPS_80 += $(CONFIG)/obj/testPath.o
DEPS_80 += $(CONFIG)/obj/testSocket.o
DEPS_80 += $(CONFIG)/obj/testSprintf.o
DEPS_80 += $(CONFIG)/obj/testThread.o
DEPS_80 += $(CONFIG)/obj/testTime.o
DEPS_80 += $(CONFIG)/obj/testUnicode.o

$(CONFIG)/bin/testMpr.out: $(DEPS_80)
	@echo '      [Link] testMpr'
	$(CC) -o $(CONFIG)/bin/testMpr.out $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/testArgv.o $(CONFIG)/obj/testBuf.o $(CONFIG)/obj/testCmd.o $(CONFIG)/obj/testCond.o $(CONFIG)/obj/testEvent.o $(CONFIG)/obj/testFile.o $(CONFIG)/obj/testHash.o $(CONFIG)/obj/testList.o $(CONFIG)/obj/testLock.o $(CONFIG)/obj/testMem.o $(CONFIG)/obj/testMpr.o $(CONFIG)/obj/testPath.o $(CONFIG)/obj/testSocket.o $(CONFIG)/obj/testSprintf.o $(CONFIG)/obj/testThread.o $(CONFIG)/obj/testTime.o $(CONFIG)/obj/testUnicode.o $(LIBS) $(LDFLAGS) 

#
#   manager.o
#
DEPS_81 += $(CONFIG)/inc/bit.h
DEPS_81 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/manager.o: \
    src/manager.c $(DEPS_81)
	@echo '   [Compile] src/manager.c'
	$(CC) -c -o $(CONFIG)/obj/manager.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/manager.c

#
#   manager
#
DEPS_82 += $(CONFIG)/bin/libmpr.out
DEPS_82 += $(CONFIG)/obj/manager.o

$(CONFIG)/bin/manager.out: $(DEPS_82)
	@echo '      [Link] manager'
	$(CC) -o $(CONFIG)/bin/manager.out $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/manager.o $(LIBS) $(LDFLAGS) 

#
#   makerom.o
#
DEPS_83 += $(CONFIG)/inc/bit.h
DEPS_83 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/makerom.o: \
    src/utils/makerom.c $(DEPS_83)
	@echo '   [Compile] src/utils/makerom.c'
	$(CC) -c -o $(CONFIG)/obj/makerom.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/utils/makerom.c

#
#   makerom
#
DEPS_84 += $(CONFIG)/bin/libmpr.out
DEPS_84 += $(CONFIG)/obj/makerom.o

$(CONFIG)/bin/makerom.out: $(DEPS_84)
	@echo '      [Link] makerom'
	$(CC) -o $(CONFIG)/bin/makerom.out $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/makerom.o $(LIBS) $(LDFLAGS) 

#
#   charGen.o
#
DEPS_85 += $(CONFIG)/inc/bit.h
DEPS_85 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/charGen.o: \
    src/utils/charGen.c $(DEPS_85)
	@echo '   [Compile] src/utils/charGen.c'
	$(CC) -c -o $(CONFIG)/obj/charGen.o $(CFLAGS) $(DFLAGS) $(IFLAGS) src/utils/charGen.c

#
#   chargen
#
DEPS_86 += $(CONFIG)/bin/libmpr.out
DEPS_86 += $(CONFIG)/obj/charGen.o

$(CONFIG)/bin/chargen.out: $(DEPS_86)
	@echo '      [Link] chargen'
	$(CC) -o $(CONFIG)/bin/chargen.out $(LDFLAGS) $(LIBPATHS) $(CONFIG)/obj/charGen.o $(LIBS) $(LDFLAGS) 

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

