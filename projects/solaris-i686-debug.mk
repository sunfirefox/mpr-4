#
#   build.mk -- Build It Makefile to build Multithreaded Portable Runtime for solaris on i686
#

PLATFORM  := solaris-i686-debug
CC        := cc
CFLAGS    := -fPIC -g -mcpu=i686
DFLAGS    := -DPIC
IFLAGS    := -I$(PLATFORM)/inc -Isrc
LDFLAGS   := -L/Users/mob/git/mpr/$(PLATFORM)/lib -g
LIBS      := -lpthread -lm

all: \
        $(PLATFORM)/bin/benchMpr \
        $(PLATFORM)/bin/runProgram \
        $(PLATFORM)/bin/testMpr \
        $(PLATFORM)/lib/libmpr.so \
        $(PLATFORM)/bin/manager \
        $(PLATFORM)/bin/makerom \
        $(PLATFORM)/bin/chargen

.PHONY: prep

prep:
	@if [ ! -x $(PLATFORM)/inc ] ; then \
		mkdir -p $(PLATFORM)/inc $(PLATFORM)/obj $(PLATFORM)/lib $(PLATFORM)/bin ; \
		cp src/buildConfig.default $(PLATFORM)/inc\
	fi

clean:
	rm -rf $(PLATFORM)/bin/benchMpr
	rm -rf $(PLATFORM)/bin/runProgram
	rm -rf $(PLATFORM)/bin/testMpr
	rm -rf $(PLATFORM)/lib/libmpr.so
	rm -rf $(PLATFORM)/lib/libmprssl.so
	rm -rf $(PLATFORM)/bin/manager
	rm -rf $(PLATFORM)/bin/makerom
	rm -rf $(PLATFORM)/bin/chargen
	rm -rf $(PLATFORM)/obj/benchMpr.o
	rm -rf $(PLATFORM)/obj/runProgram.o
	rm -rf $(PLATFORM)/obj/testArgv.o
	rm -rf $(PLATFORM)/obj/testBuf.o
	rm -rf $(PLATFORM)/obj/testCmd.o
	rm -rf $(PLATFORM)/obj/testCond.o
	rm -rf $(PLATFORM)/obj/testEvent.o
	rm -rf $(PLATFORM)/obj/testFile.o
	rm -rf $(PLATFORM)/obj/testHash.o
	rm -rf $(PLATFORM)/obj/testList.o
	rm -rf $(PLATFORM)/obj/testLock.o
	rm -rf $(PLATFORM)/obj/testMem.o
	rm -rf $(PLATFORM)/obj/testMpr.o
	rm -rf $(PLATFORM)/obj/testPath.o
	rm -rf $(PLATFORM)/obj/testSocket.o
	rm -rf $(PLATFORM)/obj/testSprintf.o
	rm -rf $(PLATFORM)/obj/testThread.o
	rm -rf $(PLATFORM)/obj/testTime.o
	rm -rf $(PLATFORM)/obj/testUnicode.o
	rm -rf $(PLATFORM)/obj/dtoa.o
	rm -rf $(PLATFORM)/obj/mpr.o
	rm -rf $(PLATFORM)/obj/mprAsync.o
	rm -rf $(PLATFORM)/obj/mprAtomic.o
	rm -rf $(PLATFORM)/obj/mprBuf.o
	rm -rf $(PLATFORM)/obj/mprCache.o
	rm -rf $(PLATFORM)/obj/mprCmd.o
	rm -rf $(PLATFORM)/obj/mprCond.o
	rm -rf $(PLATFORM)/obj/mprCrypt.o
	rm -rf $(PLATFORM)/obj/mprDisk.o
	rm -rf $(PLATFORM)/obj/mprDispatcher.o
	rm -rf $(PLATFORM)/obj/mprEncode.o
	rm -rf $(PLATFORM)/obj/mprEpoll.o
	rm -rf $(PLATFORM)/obj/mprEvent.o
	rm -rf $(PLATFORM)/obj/mprFile.o
	rm -rf $(PLATFORM)/obj/mprFileSystem.o
	rm -rf $(PLATFORM)/obj/mprHash.o
	rm -rf $(PLATFORM)/obj/mprJSON.o
	rm -rf $(PLATFORM)/obj/mprKqueue.o
	rm -rf $(PLATFORM)/obj/mprList.o
	rm -rf $(PLATFORM)/obj/mprLock.o
	rm -rf $(PLATFORM)/obj/mprLog.o
	rm -rf $(PLATFORM)/obj/mprMem.o
	rm -rf $(PLATFORM)/obj/mprMime.o
	rm -rf $(PLATFORM)/obj/mprMixed.o
	rm -rf $(PLATFORM)/obj/mprModule.o
	rm -rf $(PLATFORM)/obj/mprPath.o
	rm -rf $(PLATFORM)/obj/mprPoll.o
	rm -rf $(PLATFORM)/obj/mprPrintf.o
	rm -rf $(PLATFORM)/obj/mprRomFile.o
	rm -rf $(PLATFORM)/obj/mprSelect.o
	rm -rf $(PLATFORM)/obj/mprSignal.o
	rm -rf $(PLATFORM)/obj/mprSocket.o
	rm -rf $(PLATFORM)/obj/mprString.o
	rm -rf $(PLATFORM)/obj/mprTest.o
	rm -rf $(PLATFORM)/obj/mprThread.o
	rm -rf $(PLATFORM)/obj/mprTime.o
	rm -rf $(PLATFORM)/obj/mprUnix.o
	rm -rf $(PLATFORM)/obj/mprVxworks.o
	rm -rf $(PLATFORM)/obj/mprWait.o
	rm -rf $(PLATFORM)/obj/mprWide.o
	rm -rf $(PLATFORM)/obj/mprWin.o
	rm -rf $(PLATFORM)/obj/mprWince.o
	rm -rf $(PLATFORM)/obj/mprXml.o
	rm -rf $(PLATFORM)/obj/mprMatrixssl.o
	rm -rf $(PLATFORM)/obj/mprOpenssl.o
	rm -rf $(PLATFORM)/obj/mprSsl.o
	rm -rf $(PLATFORM)/obj/manager.o
	rm -rf $(PLATFORM)/obj/makerom.o
	rm -rf $(PLATFORM)/obj/charGen.o

$(PLATFORM)/obj/dtoa.o: \
        src/dtoa.c \
        $(PLATFORM)/inc/bit.h
	$(CC) -c -o $(PLATFORM)/obj/dtoa.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/dtoa.c

$(PLATFORM)/obj/mpr.o: \
        src/mpr.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mpr.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mpr.c

$(PLATFORM)/obj/mprAsync.o: \
        src/mprAsync.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprAsync.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprAsync.c

$(PLATFORM)/obj/mprAtomic.o: \
        src/mprAtomic.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprAtomic.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprAtomic.c

$(PLATFORM)/obj/mprBuf.o: \
        src/mprBuf.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprBuf.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprBuf.c

$(PLATFORM)/obj/mprCache.o: \
        src/mprCache.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprCache.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprCache.c

$(PLATFORM)/obj/mprCmd.o: \
        src/mprCmd.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprCmd.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprCmd.c

$(PLATFORM)/obj/mprCond.o: \
        src/mprCond.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprCond.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprCond.c

$(PLATFORM)/obj/mprCrypt.o: \
        src/mprCrypt.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprCrypt.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprCrypt.c

$(PLATFORM)/obj/mprDisk.o: \
        src/mprDisk.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprDisk.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprDisk.c

$(PLATFORM)/obj/mprDispatcher.o: \
        src/mprDispatcher.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprDispatcher.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprDispatcher.c

$(PLATFORM)/obj/mprEncode.o: \
        src/mprEncode.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprEncode.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprEncode.c

$(PLATFORM)/obj/mprEpoll.o: \
        src/mprEpoll.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprEpoll.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprEpoll.c

$(PLATFORM)/obj/mprEvent.o: \
        src/mprEvent.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprEvent.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprEvent.c

$(PLATFORM)/obj/mprFile.o: \
        src/mprFile.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprFile.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprFile.c

$(PLATFORM)/obj/mprFileSystem.o: \
        src/mprFileSystem.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprFileSystem.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprFileSystem.c

$(PLATFORM)/obj/mprHash.o: \
        src/mprHash.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprHash.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprHash.c

$(PLATFORM)/obj/mprJSON.o: \
        src/mprJSON.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprJSON.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprJSON.c

$(PLATFORM)/obj/mprKqueue.o: \
        src/mprKqueue.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprKqueue.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprKqueue.c

$(PLATFORM)/obj/mprList.o: \
        src/mprList.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprList.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprList.c

$(PLATFORM)/obj/mprLock.o: \
        src/mprLock.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprLock.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprLock.c

$(PLATFORM)/obj/mprLog.o: \
        src/mprLog.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprLog.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprLog.c

$(PLATFORM)/obj/mprMem.o: \
        src/mprMem.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprMem.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprMem.c

$(PLATFORM)/obj/mprMime.o: \
        src/mprMime.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprMime.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprMime.c

$(PLATFORM)/obj/mprMixed.o: \
        src/mprMixed.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprMixed.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprMixed.c

$(PLATFORM)/obj/mprModule.o: \
        src/mprModule.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprModule.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprModule.c

$(PLATFORM)/obj/mprPath.o: \
        src/mprPath.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprPath.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprPath.c

$(PLATFORM)/obj/mprPoll.o: \
        src/mprPoll.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprPoll.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprPoll.c

$(PLATFORM)/obj/mprPrintf.o: \
        src/mprPrintf.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprPrintf.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprPrintf.c

$(PLATFORM)/obj/mprRomFile.o: \
        src/mprRomFile.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprRomFile.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprRomFile.c

$(PLATFORM)/obj/mprSelect.o: \
        src/mprSelect.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprSelect.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprSelect.c

$(PLATFORM)/obj/mprSignal.o: \
        src/mprSignal.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprSignal.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprSignal.c

$(PLATFORM)/obj/mprSocket.o: \
        src/mprSocket.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprSocket.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprSocket.c

$(PLATFORM)/obj/mprString.o: \
        src/mprString.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprString.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprString.c

$(PLATFORM)/obj/mprTest.o: \
        src/mprTest.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprTest.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprTest.c

$(PLATFORM)/obj/mprThread.o: \
        src/mprThread.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprThread.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprThread.c

$(PLATFORM)/obj/mprTime.o: \
        src/mprTime.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprTime.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprTime.c

$(PLATFORM)/obj/mprUnix.o: \
        src/mprUnix.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprUnix.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprUnix.c

$(PLATFORM)/obj/mprVxworks.o: \
        src/mprVxworks.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprVxworks.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprVxworks.c

$(PLATFORM)/obj/mprWait.o: \
        src/mprWait.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprWait.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprWait.c

$(PLATFORM)/obj/mprWide.o: \
        src/mprWide.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprWide.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprWide.c

$(PLATFORM)/obj/mprWin.o: \
        src/mprWin.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprWin.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprWin.c

$(PLATFORM)/obj/mprWince.o: \
        src/mprWince.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprWince.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprWince.c

$(PLATFORM)/obj/mprXml.o: \
        src/mprXml.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/mprXml.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/mprXml.c

$(PLATFORM)/lib/libmpr.so:  \
        $(PLATFORM)/obj/dtoa.o \
        $(PLATFORM)/obj/mpr.o \
        $(PLATFORM)/obj/mprAsync.o \
        $(PLATFORM)/obj/mprAtomic.o \
        $(PLATFORM)/obj/mprBuf.o \
        $(PLATFORM)/obj/mprCache.o \
        $(PLATFORM)/obj/mprCmd.o \
        $(PLATFORM)/obj/mprCond.o \
        $(PLATFORM)/obj/mprCrypt.o \
        $(PLATFORM)/obj/mprDisk.o \
        $(PLATFORM)/obj/mprDispatcher.o \
        $(PLATFORM)/obj/mprEncode.o \
        $(PLATFORM)/obj/mprEpoll.o \
        $(PLATFORM)/obj/mprEvent.o \
        $(PLATFORM)/obj/mprFile.o \
        $(PLATFORM)/obj/mprFileSystem.o \
        $(PLATFORM)/obj/mprHash.o \
        $(PLATFORM)/obj/mprJSON.o \
        $(PLATFORM)/obj/mprKqueue.o \
        $(PLATFORM)/obj/mprList.o \
        $(PLATFORM)/obj/mprLock.o \
        $(PLATFORM)/obj/mprLog.o \
        $(PLATFORM)/obj/mprMem.o \
        $(PLATFORM)/obj/mprMime.o \
        $(PLATFORM)/obj/mprMixed.o \
        $(PLATFORM)/obj/mprModule.o \
        $(PLATFORM)/obj/mprPath.o \
        $(PLATFORM)/obj/mprPoll.o \
        $(PLATFORM)/obj/mprPrintf.o \
        $(PLATFORM)/obj/mprRomFile.o \
        $(PLATFORM)/obj/mprSelect.o \
        $(PLATFORM)/obj/mprSignal.o \
        $(PLATFORM)/obj/mprSocket.o \
        $(PLATFORM)/obj/mprString.o \
        $(PLATFORM)/obj/mprTest.o \
        $(PLATFORM)/obj/mprThread.o \
        $(PLATFORM)/obj/mprTime.o \
        $(PLATFORM)/obj/mprUnix.o \
        $(PLATFORM)/obj/mprVxworks.o \
        $(PLATFORM)/obj/mprWait.o \
        $(PLATFORM)/obj/mprWide.o \
        $(PLATFORM)/obj/mprWin.o \
        $(PLATFORM)/obj/mprWince.o \
        $(PLATFORM)/obj/mprXml.o
	$(CC) -shared -o $(PLATFORM)/lib/libmpr.so -L$(PLATFORM)/lib -g $(PLATFORM)/obj/dtoa.o $(PLATFORM)/obj/mpr.o $(PLATFORM)/obj/mprAsync.o $(PLATFORM)/obj/mprAtomic.o $(PLATFORM)/obj/mprBuf.o $(PLATFORM)/obj/mprCache.o $(PLATFORM)/obj/mprCmd.o $(PLATFORM)/obj/mprCond.o $(PLATFORM)/obj/mprCrypt.o $(PLATFORM)/obj/mprDisk.o $(PLATFORM)/obj/mprDispatcher.o $(PLATFORM)/obj/mprEncode.o $(PLATFORM)/obj/mprEpoll.o $(PLATFORM)/obj/mprEvent.o $(PLATFORM)/obj/mprFile.o $(PLATFORM)/obj/mprFileSystem.o $(PLATFORM)/obj/mprHash.o $(PLATFORM)/obj/mprJSON.o $(PLATFORM)/obj/mprKqueue.o $(PLATFORM)/obj/mprList.o $(PLATFORM)/obj/mprLock.o $(PLATFORM)/obj/mprLog.o $(PLATFORM)/obj/mprMem.o $(PLATFORM)/obj/mprMime.o $(PLATFORM)/obj/mprMixed.o $(PLATFORM)/obj/mprModule.o $(PLATFORM)/obj/mprPath.o $(PLATFORM)/obj/mprPoll.o $(PLATFORM)/obj/mprPrintf.o $(PLATFORM)/obj/mprRomFile.o $(PLATFORM)/obj/mprSelect.o $(PLATFORM)/obj/mprSignal.o $(PLATFORM)/obj/mprSocket.o $(PLATFORM)/obj/mprString.o $(PLATFORM)/obj/mprTest.o $(PLATFORM)/obj/mprThread.o $(PLATFORM)/obj/mprTime.o $(PLATFORM)/obj/mprUnix.o $(PLATFORM)/obj/mprVxworks.o $(PLATFORM)/obj/mprWait.o $(PLATFORM)/obj/mprWide.o $(PLATFORM)/obj/mprWin.o $(PLATFORM)/obj/mprWince.o $(PLATFORM)/obj/mprXml.o $(LIBS)

$(PLATFORM)/obj/benchMpr.o: \
        test/benchMpr.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/benchMpr.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/benchMpr.c

$(PLATFORM)/bin/benchMpr:  \
        $(PLATFORM)/lib/libmpr.so \
        $(PLATFORM)/obj/benchMpr.o
	$(CC) -o $(PLATFORM)/bin/benchMpr -L$(PLATFORM)/lib -g -L$(PLATFORM)/lib $(PLATFORM)/obj/benchMpr.o $(LIBS) -lmpr -L$(PLATFORM)/lib -g

$(PLATFORM)/obj/runProgram.o: \
        test/runProgram.c \
        $(PLATFORM)/inc/bit.h \
        $(PLATFORM)/inc/buildConfig.h
	$(CC) -c -o $(PLATFORM)/obj/runProgram.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/runProgram.c

$(PLATFORM)/bin/runProgram:  \
        $(PLATFORM)/obj/runProgram.o
	$(CC) -o $(PLATFORM)/bin/runProgram -L$(PLATFORM)/lib -g -L$(PLATFORM)/lib $(PLATFORM)/obj/runProgram.o $(LIBS) -L$(PLATFORM)/lib -g

$(PLATFORM)/obj/testArgv.o: \
        test/testArgv.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testArgv.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testArgv.c

$(PLATFORM)/obj/testBuf.o: \
        test/testBuf.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testBuf.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testBuf.c

$(PLATFORM)/obj/testCmd.o: \
        test/testCmd.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testCmd.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testCmd.c

$(PLATFORM)/obj/testCond.o: \
        test/testCond.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testCond.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testCond.c

$(PLATFORM)/obj/testEvent.o: \
        test/testEvent.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testEvent.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testEvent.c

$(PLATFORM)/obj/testFile.o: \
        test/testFile.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testFile.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testFile.c

$(PLATFORM)/obj/testHash.o: \
        test/testHash.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testHash.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testHash.c

$(PLATFORM)/obj/testList.o: \
        test/testList.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testList.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testList.c

$(PLATFORM)/obj/testLock.o: \
        test/testLock.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testLock.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testLock.c

$(PLATFORM)/obj/testMem.o: \
        test/testMem.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testMem.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testMem.c

$(PLATFORM)/obj/testMpr.o: \
        test/testMpr.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testMpr.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testMpr.c

$(PLATFORM)/obj/testPath.o: \
        test/testPath.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testPath.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testPath.c

$(PLATFORM)/obj/testSocket.o: \
        test/testSocket.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testSocket.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testSocket.c

$(PLATFORM)/obj/testSprintf.o: \
        test/testSprintf.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testSprintf.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testSprintf.c

$(PLATFORM)/obj/testThread.o: \
        test/testThread.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testThread.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testThread.c

$(PLATFORM)/obj/testTime.o: \
        test/testTime.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testTime.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testTime.c

$(PLATFORM)/obj/testUnicode.o: \
        test/testUnicode.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/testUnicode.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc test/testUnicode.c

$(PLATFORM)/bin/testMpr:  \
        $(PLATFORM)/lib/libmpr.so \
        $(PLATFORM)/bin/runProgram \
        $(PLATFORM)/obj/testArgv.o \
        $(PLATFORM)/obj/testBuf.o \
        $(PLATFORM)/obj/testCmd.o \
        $(PLATFORM)/obj/testCond.o \
        $(PLATFORM)/obj/testEvent.o \
        $(PLATFORM)/obj/testFile.o \
        $(PLATFORM)/obj/testHash.o \
        $(PLATFORM)/obj/testList.o \
        $(PLATFORM)/obj/testLock.o \
        $(PLATFORM)/obj/testMem.o \
        $(PLATFORM)/obj/testMpr.o \
        $(PLATFORM)/obj/testPath.o \
        $(PLATFORM)/obj/testSocket.o \
        $(PLATFORM)/obj/testSprintf.o \
        $(PLATFORM)/obj/testThread.o \
        $(PLATFORM)/obj/testTime.o \
        $(PLATFORM)/obj/testUnicode.o
	$(CC) -o $(PLATFORM)/bin/testMpr -L$(PLATFORM)/lib -g -L$(PLATFORM)/lib $(PLATFORM)/obj/testArgv.o $(PLATFORM)/obj/testBuf.o $(PLATFORM)/obj/testCmd.o $(PLATFORM)/obj/testCond.o $(PLATFORM)/obj/testEvent.o $(PLATFORM)/obj/testFile.o $(PLATFORM)/obj/testHash.o $(PLATFORM)/obj/testList.o $(PLATFORM)/obj/testLock.o $(PLATFORM)/obj/testMem.o $(PLATFORM)/obj/testMpr.o $(PLATFORM)/obj/testPath.o $(PLATFORM)/obj/testSocket.o $(PLATFORM)/obj/testSprintf.o $(PLATFORM)/obj/testThread.o $(PLATFORM)/obj/testTime.o $(PLATFORM)/obj/testUnicode.o $(LIBS) -lmpr -L$(PLATFORM)/lib -g

$(PLATFORM)/obj/manager.o: \
        src/manager.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/manager.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/manager.c

$(PLATFORM)/bin/manager:  \
        $(PLATFORM)/lib/libmpr.so \
        $(PLATFORM)/obj/manager.o
	$(CC) -o $(PLATFORM)/bin/manager -L$(PLATFORM)/lib -g -L$(PLATFORM)/lib $(PLATFORM)/obj/manager.o $(LIBS) -lmpr -L$(PLATFORM)/lib -g

$(PLATFORM)/obj/makerom.o: \
        src/utils/makerom.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/makerom.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/utils/makerom.c

$(PLATFORM)/bin/makerom:  \
        $(PLATFORM)/lib/libmpr.so \
        $(PLATFORM)/obj/makerom.o
	$(CC) -o $(PLATFORM)/bin/makerom -L$(PLATFORM)/lib -g -L$(PLATFORM)/lib $(PLATFORM)/obj/makerom.o $(LIBS) -lmpr -L$(PLATFORM)/lib -g

$(PLATFORM)/obj/charGen.o: \
        src/utils/charGen.c \
        $(PLATFORM)/inc/bit.h \
        src/mpr.h
	$(CC) -c -o $(PLATFORM)/obj/charGen.o $(CFLAGS) $(DFLAGS) -I$(PLATFORM)/inc -Isrc src/utils/charGen.c

$(PLATFORM)/bin/chargen:  \
        $(PLATFORM)/lib/libmpr.so \
        $(PLATFORM)/obj/charGen.o
	$(CC) -o $(PLATFORM)/bin/chargen -L$(PLATFORM)/lib -g -L$(PLATFORM)/lib $(PLATFORM)/obj/charGen.o $(LIBS) -lmpr -L$(PLATFORM)/lib -g

