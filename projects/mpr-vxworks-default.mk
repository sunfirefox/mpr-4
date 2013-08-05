#
#   mpr-vxworks-default.mk -- Makefile to build Multithreaded Portable Runtime for vxworks
#

PRODUCT            := mpr
VERSION            := 4.4.0
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
DFLAGS             += -DVXWORKS -DRW_MULTI_THREAD -D_GNU_TOOL -DCPU=PENTIUM $(patsubst %,-D%,$(filter BIT_%,$(MAKEFLAGS))) -DBIT_PACK_EST=$(BIT_PACK_EST) -DBIT_PACK_MATRIXSSL=$(BIT_PACK_MATRIXSSL) -DBIT_PACK_OPENSSL=$(BIT_PACK_OPENSSL) -DBIT_PACK_SSL=$(BIT_PACK_SSL) 
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
TARGETS            += $(CONFIG)/bin/libmprssl.out
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
	@[ ! -f $(CONFIG)/inc/bitos.h ] && cp src/bitos.h $(CONFIG)/inc/bitos.h ; true
	@if ! diff $(CONFIG)/inc/bitos.h src/bitos.h >/dev/null ; then\
		cp src/bitos.h $(CONFIG)/inc/bitos.h  ; \
	fi; true
	@[ ! -f $(CONFIG)/inc/bit.h ] && cp projects/mpr-vxworks-default-bit.h $(CONFIG)/inc/bit.h ; true
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
	rm -f "$(CONFIG)/bin/ca.crt"
	rm -f "$(CONFIG)/bin/benchMpr.out"
	rm -f "$(CONFIG)/bin/runProgram.out"
	rm -f "$(CONFIG)/bin/testMpr.out"
	rm -f "$(CONFIG)/bin/libmpr.out"
	rm -f "$(CONFIG)/bin/libmprssl.out"
	rm -f "$(CONFIG)/bin/manager.out"
	rm -f "$(CONFIG)/bin/makerom.out"
	rm -f "$(CONFIG)/bin/chargen.out"
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
	@echo 4.4.0-0

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
	$(CC) -c -o $(CONFIG)/obj/estLib.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/deps/est/estLib.c

ifeq ($(BIT_PACK_EST),1)
#
#   libest
#
DEPS_6 += $(CONFIG)/inc/est.h
DEPS_6 += $(CONFIG)/inc/bit.h
DEPS_6 += $(CONFIG)/inc/bitos.h
DEPS_6 += $(CONFIG)/obj/estLib.o

$(CONFIG)/bin/libest.out: $(DEPS_6)
	@echo '      [Link] $(CONFIG)/bin/libest.out'
	$(CC) -r -o $(CONFIG)/bin/libest.out $(LDFLAGS) $(LIBPATHS) "$(CONFIG)/obj/estLib.o" $(LIBS) 
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
	$(CC) -c -o $(CONFIG)/obj/async.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/async.c

#
#   atomic.o
#
DEPS_10 += $(CONFIG)/inc/bit.h
DEPS_10 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/atomic.o: \
    src/atomic.c $(DEPS_10)
	@echo '   [Compile] $(CONFIG)/obj/atomic.o'
	$(CC) -c -o $(CONFIG)/obj/atomic.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/atomic.c

#
#   buf.o
#
DEPS_11 += $(CONFIG)/inc/bit.h
DEPS_11 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/buf.o: \
    src/buf.c $(DEPS_11)
	@echo '   [Compile] $(CONFIG)/obj/buf.o'
	$(CC) -c -o $(CONFIG)/obj/buf.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/buf.c

#
#   cache.o
#
DEPS_12 += $(CONFIG)/inc/bit.h
DEPS_12 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/cache.o: \
    src/cache.c $(DEPS_12)
	@echo '   [Compile] $(CONFIG)/obj/cache.o'
	$(CC) -c -o $(CONFIG)/obj/cache.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/cache.c

#
#   cmd.o
#
DEPS_13 += $(CONFIG)/inc/bit.h
DEPS_13 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/cmd.o: \
    src/cmd.c $(DEPS_13)
	@echo '   [Compile] $(CONFIG)/obj/cmd.o'
	$(CC) -c -o $(CONFIG)/obj/cmd.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/cmd.c

#
#   cond.o
#
DEPS_14 += $(CONFIG)/inc/bit.h
DEPS_14 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/cond.o: \
    src/cond.c $(DEPS_14)
	@echo '   [Compile] $(CONFIG)/obj/cond.o'
	$(CC) -c -o $(CONFIG)/obj/cond.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/cond.c

#
#   crypt.o
#
DEPS_15 += $(CONFIG)/inc/bit.h
DEPS_15 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/crypt.o: \
    src/crypt.c $(DEPS_15)
	@echo '   [Compile] $(CONFIG)/obj/crypt.o'
	$(CC) -c -o $(CONFIG)/obj/crypt.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/crypt.c

#
#   disk.o
#
DEPS_16 += $(CONFIG)/inc/bit.h
DEPS_16 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/disk.o: \
    src/disk.c $(DEPS_16)
	@echo '   [Compile] $(CONFIG)/obj/disk.o'
	$(CC) -c -o $(CONFIG)/obj/disk.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/disk.c

#
#   dispatcher.o
#
DEPS_17 += $(CONFIG)/inc/bit.h
DEPS_17 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/dispatcher.o: \
    src/dispatcher.c $(DEPS_17)
	@echo '   [Compile] $(CONFIG)/obj/dispatcher.o'
	$(CC) -c -o $(CONFIG)/obj/dispatcher.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/dispatcher.c

#
#   encode.o
#
DEPS_18 += $(CONFIG)/inc/bit.h
DEPS_18 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/encode.o: \
    src/encode.c $(DEPS_18)
	@echo '   [Compile] $(CONFIG)/obj/encode.o'
	$(CC) -c -o $(CONFIG)/obj/encode.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/encode.c

#
#   epoll.o
#
DEPS_19 += $(CONFIG)/inc/bit.h
DEPS_19 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/epoll.o: \
    src/epoll.c $(DEPS_19)
	@echo '   [Compile] $(CONFIG)/obj/epoll.o'
	$(CC) -c -o $(CONFIG)/obj/epoll.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/epoll.c

#
#   event.o
#
DEPS_20 += $(CONFIG)/inc/bit.h
DEPS_20 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/event.o: \
    src/event.c $(DEPS_20)
	@echo '   [Compile] $(CONFIG)/obj/event.o'
	$(CC) -c -o $(CONFIG)/obj/event.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/event.c

#
#   file.o
#
DEPS_21 += $(CONFIG)/inc/bit.h
DEPS_21 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/file.o: \
    src/file.c $(DEPS_21)
	@echo '   [Compile] $(CONFIG)/obj/file.o'
	$(CC) -c -o $(CONFIG)/obj/file.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/file.c

#
#   fs.o
#
DEPS_22 += $(CONFIG)/inc/bit.h
DEPS_22 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/fs.o: \
    src/fs.c $(DEPS_22)
	@echo '   [Compile] $(CONFIG)/obj/fs.o'
	$(CC) -c -o $(CONFIG)/obj/fs.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/fs.c

#
#   hash.o
#
DEPS_23 += $(CONFIG)/inc/bit.h
DEPS_23 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/hash.o: \
    src/hash.c $(DEPS_23)
	@echo '   [Compile] $(CONFIG)/obj/hash.o'
	$(CC) -c -o $(CONFIG)/obj/hash.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/hash.c

#
#   json.o
#
DEPS_24 += $(CONFIG)/inc/bit.h
DEPS_24 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/json.o: \
    src/json.c $(DEPS_24)
	@echo '   [Compile] $(CONFIG)/obj/json.o'
	$(CC) -c -o $(CONFIG)/obj/json.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/json.c

#
#   kqueue.o
#
DEPS_25 += $(CONFIG)/inc/bit.h
DEPS_25 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/kqueue.o: \
    src/kqueue.c $(DEPS_25)
	@echo '   [Compile] $(CONFIG)/obj/kqueue.o'
	$(CC) -c -o $(CONFIG)/obj/kqueue.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/kqueue.c

#
#   list.o
#
DEPS_26 += $(CONFIG)/inc/bit.h
DEPS_26 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/list.o: \
    src/list.c $(DEPS_26)
	@echo '   [Compile] $(CONFIG)/obj/list.o'
	$(CC) -c -o $(CONFIG)/obj/list.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/list.c

#
#   lock.o
#
DEPS_27 += $(CONFIG)/inc/bit.h
DEPS_27 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/lock.o: \
    src/lock.c $(DEPS_27)
	@echo '   [Compile] $(CONFIG)/obj/lock.o'
	$(CC) -c -o $(CONFIG)/obj/lock.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/lock.c

#
#   log.o
#
DEPS_28 += $(CONFIG)/inc/bit.h
DEPS_28 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/log.o: \
    src/log.c $(DEPS_28)
	@echo '   [Compile] $(CONFIG)/obj/log.o'
	$(CC) -c -o $(CONFIG)/obj/log.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/log.c

#
#   mem.o
#
DEPS_29 += $(CONFIG)/inc/bit.h
DEPS_29 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mem.o: \
    src/mem.c $(DEPS_29)
	@echo '   [Compile] $(CONFIG)/obj/mem.o'
	$(CC) -c -o $(CONFIG)/obj/mem.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/mem.c

#
#   mime.o
#
DEPS_30 += $(CONFIG)/inc/bit.h
DEPS_30 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mime.o: \
    src/mime.c $(DEPS_30)
	@echo '   [Compile] $(CONFIG)/obj/mime.o'
	$(CC) -c -o $(CONFIG)/obj/mime.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/mime.c

#
#   mixed.o
#
DEPS_31 += $(CONFIG)/inc/bit.h
DEPS_31 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mixed.o: \
    src/mixed.c $(DEPS_31)
	@echo '   [Compile] $(CONFIG)/obj/mixed.o'
	$(CC) -c -o $(CONFIG)/obj/mixed.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/mixed.c

#
#   module.o
#
DEPS_32 += $(CONFIG)/inc/bit.h
DEPS_32 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/module.o: \
    src/module.c $(DEPS_32)
	@echo '   [Compile] $(CONFIG)/obj/module.o'
	$(CC) -c -o $(CONFIG)/obj/module.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/module.c

#
#   mpr.o
#
DEPS_33 += $(CONFIG)/inc/bit.h
DEPS_33 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/mpr.o: \
    src/mpr.c $(DEPS_33)
	@echo '   [Compile] $(CONFIG)/obj/mpr.o'
	$(CC) -c -o $(CONFIG)/obj/mpr.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/mpr.c

#
#   path.o
#
DEPS_34 += $(CONFIG)/inc/bit.h
DEPS_34 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/path.o: \
    src/path.c $(DEPS_34)
	@echo '   [Compile] $(CONFIG)/obj/path.o'
	$(CC) -c -o $(CONFIG)/obj/path.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/path.c

#
#   posix.o
#
DEPS_35 += $(CONFIG)/inc/bit.h
DEPS_35 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/posix.o: \
    src/posix.c $(DEPS_35)
	@echo '   [Compile] $(CONFIG)/obj/posix.o'
	$(CC) -c -o $(CONFIG)/obj/posix.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/posix.c

#
#   printf.o
#
DEPS_36 += $(CONFIG)/inc/bit.h
DEPS_36 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/printf.o: \
    src/printf.c $(DEPS_36)
	@echo '   [Compile] $(CONFIG)/obj/printf.o'
	$(CC) -c -o $(CONFIG)/obj/printf.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/printf.c

#
#   rom.o
#
DEPS_37 += $(CONFIG)/inc/bit.h
DEPS_37 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/rom.o: \
    src/rom.c $(DEPS_37)
	@echo '   [Compile] $(CONFIG)/obj/rom.o'
	$(CC) -c -o $(CONFIG)/obj/rom.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/rom.c

#
#   select.o
#
DEPS_38 += $(CONFIG)/inc/bit.h
DEPS_38 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/select.o: \
    src/select.c $(DEPS_38)
	@echo '   [Compile] $(CONFIG)/obj/select.o'
	$(CC) -c -o $(CONFIG)/obj/select.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/select.c

#
#   signal.o
#
DEPS_39 += $(CONFIG)/inc/bit.h
DEPS_39 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/signal.o: \
    src/signal.c $(DEPS_39)
	@echo '   [Compile] $(CONFIG)/obj/signal.o'
	$(CC) -c -o $(CONFIG)/obj/signal.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/signal.c

#
#   socket.o
#
DEPS_40 += $(CONFIG)/inc/bit.h
DEPS_40 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/socket.o: \
    src/socket.c $(DEPS_40)
	@echo '   [Compile] $(CONFIG)/obj/socket.o'
	$(CC) -c -o $(CONFIG)/obj/socket.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/socket.c

#
#   string.o
#
DEPS_41 += $(CONFIG)/inc/bit.h
DEPS_41 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/string.o: \
    src/string.c $(DEPS_41)
	@echo '   [Compile] $(CONFIG)/obj/string.o'
	$(CC) -c -o $(CONFIG)/obj/string.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/string.c

#
#   test.o
#
DEPS_42 += $(CONFIG)/inc/bit.h
DEPS_42 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/test.o: \
    src/test.c $(DEPS_42)
	@echo '   [Compile] $(CONFIG)/obj/test.o'
	$(CC) -c -o $(CONFIG)/obj/test.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/test.c

#
#   thread.o
#
DEPS_43 += $(CONFIG)/inc/bit.h
DEPS_43 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/thread.o: \
    src/thread.c $(DEPS_43)
	@echo '   [Compile] $(CONFIG)/obj/thread.o'
	$(CC) -c -o $(CONFIG)/obj/thread.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/thread.c

#
#   time.o
#
DEPS_44 += $(CONFIG)/inc/bit.h
DEPS_44 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/time.o: \
    src/time.c $(DEPS_44)
	@echo '   [Compile] $(CONFIG)/obj/time.o'
	$(CC) -c -o $(CONFIG)/obj/time.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/time.c

#
#   vxworks.o
#
DEPS_45 += $(CONFIG)/inc/bit.h
DEPS_45 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/vxworks.o: \
    src/vxworks.c $(DEPS_45)
	@echo '   [Compile] $(CONFIG)/obj/vxworks.o'
	$(CC) -c -o $(CONFIG)/obj/vxworks.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/vxworks.c

#
#   wait.o
#
DEPS_46 += $(CONFIG)/inc/bit.h
DEPS_46 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wait.o: \
    src/wait.c $(DEPS_46)
	@echo '   [Compile] $(CONFIG)/obj/wait.o'
	$(CC) -c -o $(CONFIG)/obj/wait.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/wait.c

#
#   wide.o
#
DEPS_47 += $(CONFIG)/inc/bit.h
DEPS_47 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wide.o: \
    src/wide.c $(DEPS_47)
	@echo '   [Compile] $(CONFIG)/obj/wide.o'
	$(CC) -c -o $(CONFIG)/obj/wide.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/wide.c

#
#   win.o
#
DEPS_48 += $(CONFIG)/inc/bit.h
DEPS_48 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/win.o: \
    src/win.c $(DEPS_48)
	@echo '   [Compile] $(CONFIG)/obj/win.o'
	$(CC) -c -o $(CONFIG)/obj/win.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/win.c

#
#   wince.o
#
DEPS_49 += $(CONFIG)/inc/bit.h
DEPS_49 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/wince.o: \
    src/wince.c $(DEPS_49)
	@echo '   [Compile] $(CONFIG)/obj/wince.o'
	$(CC) -c -o $(CONFIG)/obj/wince.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/wince.c

#
#   xml.o
#
DEPS_50 += $(CONFIG)/inc/bit.h
DEPS_50 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/xml.o: \
    src/xml.c $(DEPS_50)
	@echo '   [Compile] $(CONFIG)/obj/xml.o'
	$(CC) -c -o $(CONFIG)/obj/xml.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/xml.c

#
#   libmpr
#
DEPS_51 += $(CONFIG)/inc/bit.h
DEPS_51 += $(CONFIG)/inc/bitos.h
DEPS_51 += $(CONFIG)/inc/mpr.h
DEPS_51 += $(CONFIG)/obj/async.o
DEPS_51 += $(CONFIG)/obj/atomic.o
DEPS_51 += $(CONFIG)/obj/buf.o
DEPS_51 += $(CONFIG)/obj/cache.o
DEPS_51 += $(CONFIG)/obj/cmd.o
DEPS_51 += $(CONFIG)/obj/cond.o
DEPS_51 += $(CONFIG)/obj/crypt.o
DEPS_51 += $(CONFIG)/obj/disk.o
DEPS_51 += $(CONFIG)/obj/dispatcher.o
DEPS_51 += $(CONFIG)/obj/encode.o
DEPS_51 += $(CONFIG)/obj/epoll.o
DEPS_51 += $(CONFIG)/obj/event.o
DEPS_51 += $(CONFIG)/obj/file.o
DEPS_51 += $(CONFIG)/obj/fs.o
DEPS_51 += $(CONFIG)/obj/hash.o
DEPS_51 += $(CONFIG)/obj/json.o
DEPS_51 += $(CONFIG)/obj/kqueue.o
DEPS_51 += $(CONFIG)/obj/list.o
DEPS_51 += $(CONFIG)/obj/lock.o
DEPS_51 += $(CONFIG)/obj/log.o
DEPS_51 += $(CONFIG)/obj/mem.o
DEPS_51 += $(CONFIG)/obj/mime.o
DEPS_51 += $(CONFIG)/obj/mixed.o
DEPS_51 += $(CONFIG)/obj/module.o
DEPS_51 += $(CONFIG)/obj/mpr.o
DEPS_51 += $(CONFIG)/obj/path.o
DEPS_51 += $(CONFIG)/obj/posix.o
DEPS_51 += $(CONFIG)/obj/printf.o
DEPS_51 += $(CONFIG)/obj/rom.o
DEPS_51 += $(CONFIG)/obj/select.o
DEPS_51 += $(CONFIG)/obj/signal.o
DEPS_51 += $(CONFIG)/obj/socket.o
DEPS_51 += $(CONFIG)/obj/string.o
DEPS_51 += $(CONFIG)/obj/test.o
DEPS_51 += $(CONFIG)/obj/thread.o
DEPS_51 += $(CONFIG)/obj/time.o
DEPS_51 += $(CONFIG)/obj/vxworks.o
DEPS_51 += $(CONFIG)/obj/wait.o
DEPS_51 += $(CONFIG)/obj/wide.o
DEPS_51 += $(CONFIG)/obj/win.o
DEPS_51 += $(CONFIG)/obj/wince.o
DEPS_51 += $(CONFIG)/obj/xml.o

$(CONFIG)/bin/libmpr.out: $(DEPS_51)
	@echo '      [Link] $(CONFIG)/bin/libmpr.out'
	$(CC) -r -o $(CONFIG)/bin/libmpr.out $(LDFLAGS) $(LIBPATHS) "$(CONFIG)/obj/async.o" "$(CONFIG)/obj/atomic.o" "$(CONFIG)/obj/buf.o" "$(CONFIG)/obj/cache.o" "$(CONFIG)/obj/cmd.o" "$(CONFIG)/obj/cond.o" "$(CONFIG)/obj/crypt.o" "$(CONFIG)/obj/disk.o" "$(CONFIG)/obj/dispatcher.o" "$(CONFIG)/obj/encode.o" "$(CONFIG)/obj/epoll.o" "$(CONFIG)/obj/event.o" "$(CONFIG)/obj/file.o" "$(CONFIG)/obj/fs.o" "$(CONFIG)/obj/hash.o" "$(CONFIG)/obj/json.o" "$(CONFIG)/obj/kqueue.o" "$(CONFIG)/obj/list.o" "$(CONFIG)/obj/lock.o" "$(CONFIG)/obj/log.o" "$(CONFIG)/obj/mem.o" "$(CONFIG)/obj/mime.o" "$(CONFIG)/obj/mixed.o" "$(CONFIG)/obj/module.o" "$(CONFIG)/obj/mpr.o" "$(CONFIG)/obj/path.o" "$(CONFIG)/obj/posix.o" "$(CONFIG)/obj/printf.o" "$(CONFIG)/obj/rom.o" "$(CONFIG)/obj/select.o" "$(CONFIG)/obj/signal.o" "$(CONFIG)/obj/socket.o" "$(CONFIG)/obj/string.o" "$(CONFIG)/obj/test.o" "$(CONFIG)/obj/thread.o" "$(CONFIG)/obj/time.o" "$(CONFIG)/obj/vxworks.o" "$(CONFIG)/obj/wait.o" "$(CONFIG)/obj/wide.o" "$(CONFIG)/obj/win.o" "$(CONFIG)/obj/wince.o" "$(CONFIG)/obj/xml.o" $(LIBS) 

#
#   benchMpr.o
#
DEPS_52 += $(CONFIG)/inc/bit.h
DEPS_52 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/benchMpr.o: \
    test/benchMpr.c $(DEPS_52)
	@echo '   [Compile] $(CONFIG)/obj/benchMpr.o'
	$(CC) -c -o $(CONFIG)/obj/benchMpr.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/benchMpr.c

#
#   benchMpr
#
DEPS_53 += $(CONFIG)/inc/bit.h
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
DEPS_53 += $(CONFIG)/obj/mem.o
DEPS_53 += $(CONFIG)/obj/mime.o
DEPS_53 += $(CONFIG)/obj/mixed.o
DEPS_53 += $(CONFIG)/obj/module.o
DEPS_53 += $(CONFIG)/obj/mpr.o
DEPS_53 += $(CONFIG)/obj/path.o
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
DEPS_53 += $(CONFIG)/bin/libmpr.out
DEPS_53 += $(CONFIG)/obj/benchMpr.o

$(CONFIG)/bin/benchMpr.out: $(DEPS_53)
	@echo '      [Link] $(CONFIG)/bin/benchMpr.out'
	$(CC) -o $(CONFIG)/bin/benchMpr.out $(LDFLAGS) $(LIBPATHS) "$(CONFIG)/obj/benchMpr.o" $(LIBS) -Wl,-r 

#
#   runProgram.o
#
DEPS_54 += $(CONFIG)/inc/bit.h

$(CONFIG)/obj/runProgram.o: \
    test/runProgram.c $(DEPS_54)
	@echo '   [Compile] $(CONFIG)/obj/runProgram.o'
	$(CC) -c -o $(CONFIG)/obj/runProgram.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/runProgram.c

#
#   runProgram
#
DEPS_55 += $(CONFIG)/inc/bit.h
DEPS_55 += $(CONFIG)/obj/runProgram.o

$(CONFIG)/bin/runProgram.out: $(DEPS_55)
	@echo '      [Link] $(CONFIG)/bin/runProgram.out'
	$(CC) -o $(CONFIG)/bin/runProgram.out $(LDFLAGS) $(LIBPATHS) "$(CONFIG)/obj/runProgram.o" $(LIBS) -Wl,-r 

#
#   testArgv.o
#
DEPS_56 += $(CONFIG)/inc/bit.h
DEPS_56 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testArgv.o: \
    test/testArgv.c $(DEPS_56)
	@echo '   [Compile] $(CONFIG)/obj/testArgv.o'
	$(CC) -c -o $(CONFIG)/obj/testArgv.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testArgv.c

#
#   testBuf.o
#
DEPS_57 += $(CONFIG)/inc/bit.h
DEPS_57 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testBuf.o: \
    test/testBuf.c $(DEPS_57)
	@echo '   [Compile] $(CONFIG)/obj/testBuf.o'
	$(CC) -c -o $(CONFIG)/obj/testBuf.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testBuf.c

#
#   testCmd.o
#
DEPS_58 += $(CONFIG)/inc/bit.h
DEPS_58 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testCmd.o: \
    test/testCmd.c $(DEPS_58)
	@echo '   [Compile] $(CONFIG)/obj/testCmd.o'
	$(CC) -c -o $(CONFIG)/obj/testCmd.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testCmd.c

#
#   testCond.o
#
DEPS_59 += $(CONFIG)/inc/bit.h
DEPS_59 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testCond.o: \
    test/testCond.c $(DEPS_59)
	@echo '   [Compile] $(CONFIG)/obj/testCond.o'
	$(CC) -c -o $(CONFIG)/obj/testCond.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testCond.c

#
#   testEvent.o
#
DEPS_60 += $(CONFIG)/inc/bit.h
DEPS_60 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testEvent.o: \
    test/testEvent.c $(DEPS_60)
	@echo '   [Compile] $(CONFIG)/obj/testEvent.o'
	$(CC) -c -o $(CONFIG)/obj/testEvent.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testEvent.c

#
#   testFile.o
#
DEPS_61 += $(CONFIG)/inc/bit.h
DEPS_61 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testFile.o: \
    test/testFile.c $(DEPS_61)
	@echo '   [Compile] $(CONFIG)/obj/testFile.o'
	$(CC) -c -o $(CONFIG)/obj/testFile.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testFile.c

#
#   testHash.o
#
DEPS_62 += $(CONFIG)/inc/bit.h
DEPS_62 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testHash.o: \
    test/testHash.c $(DEPS_62)
	@echo '   [Compile] $(CONFIG)/obj/testHash.o'
	$(CC) -c -o $(CONFIG)/obj/testHash.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testHash.c

#
#   testList.o
#
DEPS_63 += $(CONFIG)/inc/bit.h
DEPS_63 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testList.o: \
    test/testList.c $(DEPS_63)
	@echo '   [Compile] $(CONFIG)/obj/testList.o'
	$(CC) -c -o $(CONFIG)/obj/testList.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testList.c

#
#   testLock.o
#
DEPS_64 += $(CONFIG)/inc/bit.h
DEPS_64 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testLock.o: \
    test/testLock.c $(DEPS_64)
	@echo '   [Compile] $(CONFIG)/obj/testLock.o'
	$(CC) -c -o $(CONFIG)/obj/testLock.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testLock.c

#
#   testMem.o
#
DEPS_65 += $(CONFIG)/inc/bit.h
DEPS_65 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testMem.o: \
    test/testMem.c $(DEPS_65)
	@echo '   [Compile] $(CONFIG)/obj/testMem.o'
	$(CC) -c -o $(CONFIG)/obj/testMem.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testMem.c

#
#   testMpr.o
#
DEPS_66 += $(CONFIG)/inc/bit.h
DEPS_66 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testMpr.o: \
    test/testMpr.c $(DEPS_66)
	@echo '   [Compile] $(CONFIG)/obj/testMpr.o'
	$(CC) -c -o $(CONFIG)/obj/testMpr.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testMpr.c

#
#   testPath.o
#
DEPS_67 += $(CONFIG)/inc/bit.h
DEPS_67 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testPath.o: \
    test/testPath.c $(DEPS_67)
	@echo '   [Compile] $(CONFIG)/obj/testPath.o'
	$(CC) -c -o $(CONFIG)/obj/testPath.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testPath.c

#
#   testSocket.o
#
DEPS_68 += $(CONFIG)/inc/bit.h
DEPS_68 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testSocket.o: \
    test/testSocket.c $(DEPS_68)
	@echo '   [Compile] $(CONFIG)/obj/testSocket.o'
	$(CC) -c -o $(CONFIG)/obj/testSocket.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testSocket.c

#
#   testSprintf.o
#
DEPS_69 += $(CONFIG)/inc/bit.h
DEPS_69 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testSprintf.o: \
    test/testSprintf.c $(DEPS_69)
	@echo '   [Compile] $(CONFIG)/obj/testSprintf.o'
	$(CC) -c -o $(CONFIG)/obj/testSprintf.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testSprintf.c

#
#   testThread.o
#
DEPS_70 += $(CONFIG)/inc/bit.h
DEPS_70 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testThread.o: \
    test/testThread.c $(DEPS_70)
	@echo '   [Compile] $(CONFIG)/obj/testThread.o'
	$(CC) -c -o $(CONFIG)/obj/testThread.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testThread.c

#
#   testTime.o
#
DEPS_71 += $(CONFIG)/inc/bit.h
DEPS_71 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testTime.o: \
    test/testTime.c $(DEPS_71)
	@echo '   [Compile] $(CONFIG)/obj/testTime.o'
	$(CC) -c -o $(CONFIG)/obj/testTime.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testTime.c

#
#   testUnicode.o
#
DEPS_72 += $(CONFIG)/inc/bit.h
DEPS_72 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/testUnicode.o: \
    test/testUnicode.c $(DEPS_72)
	@echo '   [Compile] $(CONFIG)/obj/testUnicode.o'
	$(CC) -c -o $(CONFIG)/obj/testUnicode.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" test/testUnicode.c

#
#   testMpr
#
DEPS_73 += $(CONFIG)/inc/bit.h
DEPS_73 += $(CONFIG)/inc/bitos.h
DEPS_73 += $(CONFIG)/inc/mpr.h
DEPS_73 += $(CONFIG)/obj/async.o
DEPS_73 += $(CONFIG)/obj/atomic.o
DEPS_73 += $(CONFIG)/obj/buf.o
DEPS_73 += $(CONFIG)/obj/cache.o
DEPS_73 += $(CONFIG)/obj/cmd.o
DEPS_73 += $(CONFIG)/obj/cond.o
DEPS_73 += $(CONFIG)/obj/crypt.o
DEPS_73 += $(CONFIG)/obj/disk.o
DEPS_73 += $(CONFIG)/obj/dispatcher.o
DEPS_73 += $(CONFIG)/obj/encode.o
DEPS_73 += $(CONFIG)/obj/epoll.o
DEPS_73 += $(CONFIG)/obj/event.o
DEPS_73 += $(CONFIG)/obj/file.o
DEPS_73 += $(CONFIG)/obj/fs.o
DEPS_73 += $(CONFIG)/obj/hash.o
DEPS_73 += $(CONFIG)/obj/json.o
DEPS_73 += $(CONFIG)/obj/kqueue.o
DEPS_73 += $(CONFIG)/obj/list.o
DEPS_73 += $(CONFIG)/obj/lock.o
DEPS_73 += $(CONFIG)/obj/log.o
DEPS_73 += $(CONFIG)/obj/mem.o
DEPS_73 += $(CONFIG)/obj/mime.o
DEPS_73 += $(CONFIG)/obj/mixed.o
DEPS_73 += $(CONFIG)/obj/module.o
DEPS_73 += $(CONFIG)/obj/mpr.o
DEPS_73 += $(CONFIG)/obj/path.o
DEPS_73 += $(CONFIG)/obj/posix.o
DEPS_73 += $(CONFIG)/obj/printf.o
DEPS_73 += $(CONFIG)/obj/rom.o
DEPS_73 += $(CONFIG)/obj/select.o
DEPS_73 += $(CONFIG)/obj/signal.o
DEPS_73 += $(CONFIG)/obj/socket.o
DEPS_73 += $(CONFIG)/obj/string.o
DEPS_73 += $(CONFIG)/obj/test.o
DEPS_73 += $(CONFIG)/obj/thread.o
DEPS_73 += $(CONFIG)/obj/time.o
DEPS_73 += $(CONFIG)/obj/vxworks.o
DEPS_73 += $(CONFIG)/obj/wait.o
DEPS_73 += $(CONFIG)/obj/wide.o
DEPS_73 += $(CONFIG)/obj/win.o
DEPS_73 += $(CONFIG)/obj/wince.o
DEPS_73 += $(CONFIG)/obj/xml.o
DEPS_73 += $(CONFIG)/bin/libmpr.out
DEPS_73 += $(CONFIG)/obj/runProgram.o
DEPS_73 += $(CONFIG)/bin/runProgram.out
DEPS_73 += $(CONFIG)/obj/testArgv.o
DEPS_73 += $(CONFIG)/obj/testBuf.o
DEPS_73 += $(CONFIG)/obj/testCmd.o
DEPS_73 += $(CONFIG)/obj/testCond.o
DEPS_73 += $(CONFIG)/obj/testEvent.o
DEPS_73 += $(CONFIG)/obj/testFile.o
DEPS_73 += $(CONFIG)/obj/testHash.o
DEPS_73 += $(CONFIG)/obj/testList.o
DEPS_73 += $(CONFIG)/obj/testLock.o
DEPS_73 += $(CONFIG)/obj/testMem.o
DEPS_73 += $(CONFIG)/obj/testMpr.o
DEPS_73 += $(CONFIG)/obj/testPath.o
DEPS_73 += $(CONFIG)/obj/testSocket.o
DEPS_73 += $(CONFIG)/obj/testSprintf.o
DEPS_73 += $(CONFIG)/obj/testThread.o
DEPS_73 += $(CONFIG)/obj/testTime.o
DEPS_73 += $(CONFIG)/obj/testUnicode.o

$(CONFIG)/bin/testMpr.out: $(DEPS_73)
	@echo '      [Link] $(CONFIG)/bin/testMpr.out'
	$(CC) -o $(CONFIG)/bin/testMpr.out $(LDFLAGS) $(LIBPATHS) "$(CONFIG)/obj/testArgv.o" "$(CONFIG)/obj/testBuf.o" "$(CONFIG)/obj/testCmd.o" "$(CONFIG)/obj/testCond.o" "$(CONFIG)/obj/testEvent.o" "$(CONFIG)/obj/testFile.o" "$(CONFIG)/obj/testHash.o" "$(CONFIG)/obj/testList.o" "$(CONFIG)/obj/testLock.o" "$(CONFIG)/obj/testMem.o" "$(CONFIG)/obj/testMpr.o" "$(CONFIG)/obj/testPath.o" "$(CONFIG)/obj/testSocket.o" "$(CONFIG)/obj/testSprintf.o" "$(CONFIG)/obj/testThread.o" "$(CONFIG)/obj/testTime.o" "$(CONFIG)/obj/testUnicode.o" $(LIBS) -Wl,-r 

#
#   est.o
#
DEPS_74 += $(CONFIG)/inc/bit.h
DEPS_74 += $(CONFIG)/inc/mpr.h
DEPS_74 += $(CONFIG)/inc/est.h

$(CONFIG)/obj/est.o: \
    src/ssl/est.c $(DEPS_74)
	@echo '   [Compile] $(CONFIG)/obj/est.o'
	$(CC) -c -o $(CONFIG)/obj/est.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" "-I$(BIT_PACK_MATRIXSSL_PATH)" "-I$(BIT_PACK_MATRIXSSL_PATH)/matrixssl" "-I$(BIT_PACK_NANOSSL_PATH)/src" "-I$(BIT_PACK_OPENSSL_PATH)/include" src/ssl/est.c

#
#   matrixssl.o
#
DEPS_75 += $(CONFIG)/inc/bit.h
DEPS_75 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/matrixssl.o: \
    src/ssl/matrixssl.c $(DEPS_75)
	@echo '   [Compile] $(CONFIG)/obj/matrixssl.o'
	$(CC) -c -o $(CONFIG)/obj/matrixssl.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" "-I$(BIT_PACK_MATRIXSSL_PATH)" "-I$(BIT_PACK_MATRIXSSL_PATH)/matrixssl" "-I$(BIT_PACK_NANOSSL_PATH)/src" "-I$(BIT_PACK_OPENSSL_PATH)/include" src/ssl/matrixssl.c

#
#   nanossl.o
#
DEPS_76 += $(CONFIG)/inc/bit.h
DEPS_76 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/nanossl.o: \
    src/ssl/nanossl.c $(DEPS_76)
	@echo '   [Compile] $(CONFIG)/obj/nanossl.o'
	$(CC) -c -o $(CONFIG)/obj/nanossl.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" "-I$(BIT_PACK_MATRIXSSL_PATH)" "-I$(BIT_PACK_MATRIXSSL_PATH)/matrixssl" "-I$(BIT_PACK_NANOSSL_PATH)/src" "-I$(BIT_PACK_OPENSSL_PATH)/include" src/ssl/nanossl.c

#
#   openssl.o
#
DEPS_77 += $(CONFIG)/inc/bit.h
DEPS_77 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/openssl.o: \
    src/ssl/openssl.c $(DEPS_77)
	@echo '   [Compile] $(CONFIG)/obj/openssl.o'
	$(CC) -c -o $(CONFIG)/obj/openssl.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" "-I$(BIT_PACK_MATRIXSSL_PATH)" "-I$(BIT_PACK_MATRIXSSL_PATH)/matrixssl" "-I$(BIT_PACK_NANOSSL_PATH)/src" "-I$(BIT_PACK_OPENSSL_PATH)/include" src/ssl/openssl.c

#
#   ssl.o
#
DEPS_78 += $(CONFIG)/inc/bit.h
DEPS_78 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/ssl.o: \
    src/ssl/ssl.c $(DEPS_78)
	@echo '   [Compile] $(CONFIG)/obj/ssl.o'
	$(CC) -c -o $(CONFIG)/obj/ssl.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" "-I$(BIT_PACK_MATRIXSSL_PATH)" "-I$(BIT_PACK_MATRIXSSL_PATH)/matrixssl" "-I$(BIT_PACK_NANOSSL_PATH)/src" "-I$(BIT_PACK_OPENSSL_PATH)/include" src/ssl/ssl.c

#
#   libmprssl
#
DEPS_79 += $(CONFIG)/inc/bit.h
DEPS_79 += $(CONFIG)/inc/bitos.h
DEPS_79 += $(CONFIG)/inc/mpr.h
DEPS_79 += $(CONFIG)/obj/async.o
DEPS_79 += $(CONFIG)/obj/atomic.o
DEPS_79 += $(CONFIG)/obj/buf.o
DEPS_79 += $(CONFIG)/obj/cache.o
DEPS_79 += $(CONFIG)/obj/cmd.o
DEPS_79 += $(CONFIG)/obj/cond.o
DEPS_79 += $(CONFIG)/obj/crypt.o
DEPS_79 += $(CONFIG)/obj/disk.o
DEPS_79 += $(CONFIG)/obj/dispatcher.o
DEPS_79 += $(CONFIG)/obj/encode.o
DEPS_79 += $(CONFIG)/obj/epoll.o
DEPS_79 += $(CONFIG)/obj/event.o
DEPS_79 += $(CONFIG)/obj/file.o
DEPS_79 += $(CONFIG)/obj/fs.o
DEPS_79 += $(CONFIG)/obj/hash.o
DEPS_79 += $(CONFIG)/obj/json.o
DEPS_79 += $(CONFIG)/obj/kqueue.o
DEPS_79 += $(CONFIG)/obj/list.o
DEPS_79 += $(CONFIG)/obj/lock.o
DEPS_79 += $(CONFIG)/obj/log.o
DEPS_79 += $(CONFIG)/obj/mem.o
DEPS_79 += $(CONFIG)/obj/mime.o
DEPS_79 += $(CONFIG)/obj/mixed.o
DEPS_79 += $(CONFIG)/obj/module.o
DEPS_79 += $(CONFIG)/obj/mpr.o
DEPS_79 += $(CONFIG)/obj/path.o
DEPS_79 += $(CONFIG)/obj/posix.o
DEPS_79 += $(CONFIG)/obj/printf.o
DEPS_79 += $(CONFIG)/obj/rom.o
DEPS_79 += $(CONFIG)/obj/select.o
DEPS_79 += $(CONFIG)/obj/signal.o
DEPS_79 += $(CONFIG)/obj/socket.o
DEPS_79 += $(CONFIG)/obj/string.o
DEPS_79 += $(CONFIG)/obj/test.o
DEPS_79 += $(CONFIG)/obj/thread.o
DEPS_79 += $(CONFIG)/obj/time.o
DEPS_79 += $(CONFIG)/obj/vxworks.o
DEPS_79 += $(CONFIG)/obj/wait.o
DEPS_79 += $(CONFIG)/obj/wide.o
DEPS_79 += $(CONFIG)/obj/win.o
DEPS_79 += $(CONFIG)/obj/wince.o
DEPS_79 += $(CONFIG)/obj/xml.o
DEPS_79 += $(CONFIG)/bin/libmpr.out
DEPS_79 += $(CONFIG)/inc/est.h
DEPS_79 += $(CONFIG)/obj/estLib.o
ifeq ($(BIT_PACK_EST),1)
    DEPS_79 += $(CONFIG)/bin/libest.out
endif
DEPS_79 += $(CONFIG)/obj/est.o
DEPS_79 += $(CONFIG)/obj/matrixssl.o
DEPS_79 += $(CONFIG)/obj/nanossl.o
DEPS_79 += $(CONFIG)/obj/openssl.o
DEPS_79 += $(CONFIG)/obj/ssl.o

ifeq ($(BIT_PACK_MATRIXSSL),1)
    LIBS_79 += -lmatrixssl
    LIBPATHS_79 += -L$(BIT_PACK_MATRIXSSL_PATH)
endif
ifeq ($(BIT_PACK_NANOSSL),1)
    LIBS_79 += -lssls
    LIBPATHS_79 += -L$(BIT_PACK_NANOSSL_PATH)/bin
endif
ifeq ($(BIT_PACK_OPENSSL),1)
    LIBS_79 += -lssl
    LIBPATHS_79 += -L$(BIT_PACK_OPENSSL_PATH)
endif
ifeq ($(BIT_PACK_OPENSSL),1)
    LIBS_79 += -lcrypto
    LIBPATHS_79 += -L$(BIT_PACK_OPENSSL_PATH)
endif

$(CONFIG)/bin/libmprssl.out: $(DEPS_79)
	@echo '      [Link] $(CONFIG)/bin/libmprssl.out'
	$(CC) -r -o $(CONFIG)/bin/libmprssl.out $(LDFLAGS) $(LIBPATHS)    "$(CONFIG)/obj/est.o" "$(CONFIG)/obj/matrixssl.o" "$(CONFIG)/obj/nanossl.o" "$(CONFIG)/obj/openssl.o" "$(CONFIG)/obj/ssl.o" $(LIBPATHS_79) $(LIBS_79) $(LIBS_79) $(LIBS) 

#
#   manager.o
#
DEPS_80 += $(CONFIG)/inc/bit.h
DEPS_80 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/manager.o: \
    src/manager.c $(DEPS_80)
	@echo '   [Compile] $(CONFIG)/obj/manager.o'
	$(CC) -c -o $(CONFIG)/obj/manager.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/manager.c

#
#   manager
#
DEPS_81 += $(CONFIG)/inc/bit.h
DEPS_81 += $(CONFIG)/inc/bitos.h
DEPS_81 += $(CONFIG)/inc/mpr.h
DEPS_81 += $(CONFIG)/obj/async.o
DEPS_81 += $(CONFIG)/obj/atomic.o
DEPS_81 += $(CONFIG)/obj/buf.o
DEPS_81 += $(CONFIG)/obj/cache.o
DEPS_81 += $(CONFIG)/obj/cmd.o
DEPS_81 += $(CONFIG)/obj/cond.o
DEPS_81 += $(CONFIG)/obj/crypt.o
DEPS_81 += $(CONFIG)/obj/disk.o
DEPS_81 += $(CONFIG)/obj/dispatcher.o
DEPS_81 += $(CONFIG)/obj/encode.o
DEPS_81 += $(CONFIG)/obj/epoll.o
DEPS_81 += $(CONFIG)/obj/event.o
DEPS_81 += $(CONFIG)/obj/file.o
DEPS_81 += $(CONFIG)/obj/fs.o
DEPS_81 += $(CONFIG)/obj/hash.o
DEPS_81 += $(CONFIG)/obj/json.o
DEPS_81 += $(CONFIG)/obj/kqueue.o
DEPS_81 += $(CONFIG)/obj/list.o
DEPS_81 += $(CONFIG)/obj/lock.o
DEPS_81 += $(CONFIG)/obj/log.o
DEPS_81 += $(CONFIG)/obj/mem.o
DEPS_81 += $(CONFIG)/obj/mime.o
DEPS_81 += $(CONFIG)/obj/mixed.o
DEPS_81 += $(CONFIG)/obj/module.o
DEPS_81 += $(CONFIG)/obj/mpr.o
DEPS_81 += $(CONFIG)/obj/path.o
DEPS_81 += $(CONFIG)/obj/posix.o
DEPS_81 += $(CONFIG)/obj/printf.o
DEPS_81 += $(CONFIG)/obj/rom.o
DEPS_81 += $(CONFIG)/obj/select.o
DEPS_81 += $(CONFIG)/obj/signal.o
DEPS_81 += $(CONFIG)/obj/socket.o
DEPS_81 += $(CONFIG)/obj/string.o
DEPS_81 += $(CONFIG)/obj/test.o
DEPS_81 += $(CONFIG)/obj/thread.o
DEPS_81 += $(CONFIG)/obj/time.o
DEPS_81 += $(CONFIG)/obj/vxworks.o
DEPS_81 += $(CONFIG)/obj/wait.o
DEPS_81 += $(CONFIG)/obj/wide.o
DEPS_81 += $(CONFIG)/obj/win.o
DEPS_81 += $(CONFIG)/obj/wince.o
DEPS_81 += $(CONFIG)/obj/xml.o
DEPS_81 += $(CONFIG)/bin/libmpr.out
DEPS_81 += $(CONFIG)/obj/manager.o

$(CONFIG)/bin/manager.out: $(DEPS_81)
	@echo '      [Link] $(CONFIG)/bin/manager.out'
	$(CC) -o $(CONFIG)/bin/manager.out $(LDFLAGS) $(LIBPATHS) "$(CONFIG)/obj/manager.o" $(LIBS) -Wl,-r 

#
#   makerom.o
#
DEPS_82 += $(CONFIG)/inc/bit.h
DEPS_82 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/makerom.o: \
    src/utils/makerom.c $(DEPS_82)
	@echo '   [Compile] $(CONFIG)/obj/makerom.o'
	$(CC) -c -o $(CONFIG)/obj/makerom.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/utils/makerom.c

#
#   makerom
#
DEPS_83 += $(CONFIG)/inc/bit.h
DEPS_83 += $(CONFIG)/inc/bitos.h
DEPS_83 += $(CONFIG)/inc/mpr.h
DEPS_83 += $(CONFIG)/obj/async.o
DEPS_83 += $(CONFIG)/obj/atomic.o
DEPS_83 += $(CONFIG)/obj/buf.o
DEPS_83 += $(CONFIG)/obj/cache.o
DEPS_83 += $(CONFIG)/obj/cmd.o
DEPS_83 += $(CONFIG)/obj/cond.o
DEPS_83 += $(CONFIG)/obj/crypt.o
DEPS_83 += $(CONFIG)/obj/disk.o
DEPS_83 += $(CONFIG)/obj/dispatcher.o
DEPS_83 += $(CONFIG)/obj/encode.o
DEPS_83 += $(CONFIG)/obj/epoll.o
DEPS_83 += $(CONFIG)/obj/event.o
DEPS_83 += $(CONFIG)/obj/file.o
DEPS_83 += $(CONFIG)/obj/fs.o
DEPS_83 += $(CONFIG)/obj/hash.o
DEPS_83 += $(CONFIG)/obj/json.o
DEPS_83 += $(CONFIG)/obj/kqueue.o
DEPS_83 += $(CONFIG)/obj/list.o
DEPS_83 += $(CONFIG)/obj/lock.o
DEPS_83 += $(CONFIG)/obj/log.o
DEPS_83 += $(CONFIG)/obj/mem.o
DEPS_83 += $(CONFIG)/obj/mime.o
DEPS_83 += $(CONFIG)/obj/mixed.o
DEPS_83 += $(CONFIG)/obj/module.o
DEPS_83 += $(CONFIG)/obj/mpr.o
DEPS_83 += $(CONFIG)/obj/path.o
DEPS_83 += $(CONFIG)/obj/posix.o
DEPS_83 += $(CONFIG)/obj/printf.o
DEPS_83 += $(CONFIG)/obj/rom.o
DEPS_83 += $(CONFIG)/obj/select.o
DEPS_83 += $(CONFIG)/obj/signal.o
DEPS_83 += $(CONFIG)/obj/socket.o
DEPS_83 += $(CONFIG)/obj/string.o
DEPS_83 += $(CONFIG)/obj/test.o
DEPS_83 += $(CONFIG)/obj/thread.o
DEPS_83 += $(CONFIG)/obj/time.o
DEPS_83 += $(CONFIG)/obj/vxworks.o
DEPS_83 += $(CONFIG)/obj/wait.o
DEPS_83 += $(CONFIG)/obj/wide.o
DEPS_83 += $(CONFIG)/obj/win.o
DEPS_83 += $(CONFIG)/obj/wince.o
DEPS_83 += $(CONFIG)/obj/xml.o
DEPS_83 += $(CONFIG)/bin/libmpr.out
DEPS_83 += $(CONFIG)/obj/makerom.o

$(CONFIG)/bin/makerom.out: $(DEPS_83)
	@echo '      [Link] $(CONFIG)/bin/makerom.out'
	$(CC) -o $(CONFIG)/bin/makerom.out $(LDFLAGS) $(LIBPATHS) "$(CONFIG)/obj/makerom.o" $(LIBS) -Wl,-r 

#
#   charGen.o
#
DEPS_84 += $(CONFIG)/inc/bit.h
DEPS_84 += $(CONFIG)/inc/mpr.h

$(CONFIG)/obj/charGen.o: \
    src/utils/charGen.c $(DEPS_84)
	@echo '   [Compile] $(CONFIG)/obj/charGen.o'
	$(CC) -c -o $(CONFIG)/obj/charGen.o $(CFLAGS) $(DFLAGS) "-I$(CONFIG)/inc" "-I$(WIND_BASE)/target/h" "-I$(WIND_BASE)/target/h/wrn/coreip" src/utils/charGen.c

#
#   chargen
#
DEPS_85 += $(CONFIG)/inc/bit.h
DEPS_85 += $(CONFIG)/inc/bitos.h
DEPS_85 += $(CONFIG)/inc/mpr.h
DEPS_85 += $(CONFIG)/obj/async.o
DEPS_85 += $(CONFIG)/obj/atomic.o
DEPS_85 += $(CONFIG)/obj/buf.o
DEPS_85 += $(CONFIG)/obj/cache.o
DEPS_85 += $(CONFIG)/obj/cmd.o
DEPS_85 += $(CONFIG)/obj/cond.o
DEPS_85 += $(CONFIG)/obj/crypt.o
DEPS_85 += $(CONFIG)/obj/disk.o
DEPS_85 += $(CONFIG)/obj/dispatcher.o
DEPS_85 += $(CONFIG)/obj/encode.o
DEPS_85 += $(CONFIG)/obj/epoll.o
DEPS_85 += $(CONFIG)/obj/event.o
DEPS_85 += $(CONFIG)/obj/file.o
DEPS_85 += $(CONFIG)/obj/fs.o
DEPS_85 += $(CONFIG)/obj/hash.o
DEPS_85 += $(CONFIG)/obj/json.o
DEPS_85 += $(CONFIG)/obj/kqueue.o
DEPS_85 += $(CONFIG)/obj/list.o
DEPS_85 += $(CONFIG)/obj/lock.o
DEPS_85 += $(CONFIG)/obj/log.o
DEPS_85 += $(CONFIG)/obj/mem.o
DEPS_85 += $(CONFIG)/obj/mime.o
DEPS_85 += $(CONFIG)/obj/mixed.o
DEPS_85 += $(CONFIG)/obj/module.o
DEPS_85 += $(CONFIG)/obj/mpr.o
DEPS_85 += $(CONFIG)/obj/path.o
DEPS_85 += $(CONFIG)/obj/posix.o
DEPS_85 += $(CONFIG)/obj/printf.o
DEPS_85 += $(CONFIG)/obj/rom.o
DEPS_85 += $(CONFIG)/obj/select.o
DEPS_85 += $(CONFIG)/obj/signal.o
DEPS_85 += $(CONFIG)/obj/socket.o
DEPS_85 += $(CONFIG)/obj/string.o
DEPS_85 += $(CONFIG)/obj/test.o
DEPS_85 += $(CONFIG)/obj/thread.o
DEPS_85 += $(CONFIG)/obj/time.o
DEPS_85 += $(CONFIG)/obj/vxworks.o
DEPS_85 += $(CONFIG)/obj/wait.o
DEPS_85 += $(CONFIG)/obj/wide.o
DEPS_85 += $(CONFIG)/obj/win.o
DEPS_85 += $(CONFIG)/obj/wince.o
DEPS_85 += $(CONFIG)/obj/xml.o
DEPS_85 += $(CONFIG)/bin/libmpr.out
DEPS_85 += $(CONFIG)/obj/charGen.o

$(CONFIG)/bin/chargen.out: $(DEPS_85)
	@echo '      [Link] $(CONFIG)/bin/chargen.out'
	$(CC) -o $(CONFIG)/bin/chargen.out $(LDFLAGS) $(LIBPATHS) "$(CONFIG)/obj/charGen.o" $(LIBS) -Wl,-r 

#
#   stop
#
stop: $(DEPS_86)

#
#   installBinary
#
installBinary: $(DEPS_87)

#
#   start
#
start: $(DEPS_88)

#
#   install
#
DEPS_89 += stop
DEPS_89 += installBinary
DEPS_89 += start

install: $(DEPS_89)
	

#
#   uninstall
#
DEPS_90 += stop

uninstall: $(DEPS_90)

