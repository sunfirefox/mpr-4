#
#   mpr-solaris-debug.sh -- Build It Shell Script to build Multithreaded Portable Runtime
#

ARCH="x86"
ARCH="`uname -m | sed 's/i.86/x86/;s/x86_64/x64/'`"
OS="solaris"
PROFILE="debug"
CONFIG="${OS}-${ARCH}-${PROFILE}"
CC="gcc"
LD="/usr/bin/ld"
CFLAGS="-Wall -fPIC -g"
DFLAGS="-D_REENTRANT -DPIC -DBIT_DEBUG"
IFLAGS="-I${CONFIG}/inc"
LDFLAGS="-g"
LIBPATHS="-L${CONFIG}/bin"
LIBS="-llxnet -lrt -lsocket -lpthread -lm -ldl"

[ ! -x ${CONFIG}/inc ] && mkdir -p ${CONFIG}/inc ${CONFIG}/obj ${CONFIG}/lib ${CONFIG}/bin

[ ! -f ${CONFIG}/inc/bit.h ] && cp projects/mpr-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h
if ! diff ${CONFIG}/inc/bit.h projects/mpr-${OS}-${PROFILE}-bit.h >/dev/null ; then
	cp projects/mpr-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h
fi

rm -rf ${CONFIG}/inc/mpr.h
cp -r src/mpr.h ${CONFIG}/inc/mpr.h

${CC} -c -o ${CONFIG}/obj/mpr.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mpr.c

${CC} -c -o ${CONFIG}/obj/mprAsync.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprAsync.c

${CC} -c -o ${CONFIG}/obj/mprAtomic.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprAtomic.c

${CC} -c -o ${CONFIG}/obj/mprBuf.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprBuf.c

${CC} -c -o ${CONFIG}/obj/mprCache.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprCache.c

${CC} -c -o ${CONFIG}/obj/mprCmd.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprCmd.c

${CC} -c -o ${CONFIG}/obj/mprCond.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprCond.c

${CC} -c -o ${CONFIG}/obj/mprCrypt.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprCrypt.c

${CC} -c -o ${CONFIG}/obj/mprDisk.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprDisk.c

${CC} -c -o ${CONFIG}/obj/mprDispatcher.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprDispatcher.c

${CC} -c -o ${CONFIG}/obj/mprEncode.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprEncode.c

${CC} -c -o ${CONFIG}/obj/mprEpoll.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprEpoll.c

${CC} -c -o ${CONFIG}/obj/mprEvent.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprEvent.c

${CC} -c -o ${CONFIG}/obj/mprFile.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprFile.c

${CC} -c -o ${CONFIG}/obj/mprFileSystem.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprFileSystem.c

${CC} -c -o ${CONFIG}/obj/mprHash.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprHash.c

${CC} -c -o ${CONFIG}/obj/mprJSON.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprJSON.c

${CC} -c -o ${CONFIG}/obj/mprKqueue.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprKqueue.c

${CC} -c -o ${CONFIG}/obj/mprList.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprList.c

${CC} -c -o ${CONFIG}/obj/mprLock.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprLock.c

${CC} -c -o ${CONFIG}/obj/mprLog.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprLog.c

${CC} -c -o ${CONFIG}/obj/mprMem.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprMem.c

${CC} -c -o ${CONFIG}/obj/mprMime.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprMime.c

${CC} -c -o ${CONFIG}/obj/mprMixed.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprMixed.c

${CC} -c -o ${CONFIG}/obj/mprModule.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprModule.c

${CC} -c -o ${CONFIG}/obj/mprPath.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprPath.c

${CC} -c -o ${CONFIG}/obj/mprPoll.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprPoll.c

${CC} -c -o ${CONFIG}/obj/mprPrintf.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprPrintf.c

${CC} -c -o ${CONFIG}/obj/mprRomFile.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprRomFile.c

${CC} -c -o ${CONFIG}/obj/mprSelect.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprSelect.c

${CC} -c -o ${CONFIG}/obj/mprSignal.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprSignal.c

${CC} -c -o ${CONFIG}/obj/mprSocket.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprSocket.c

${CC} -c -o ${CONFIG}/obj/mprString.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprString.c

${CC} -c -o ${CONFIG}/obj/mprTest.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprTest.c

${CC} -c -o ${CONFIG}/obj/mprThread.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprThread.c

${CC} -c -o ${CONFIG}/obj/mprTime.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprTime.c

${CC} -c -o ${CONFIG}/obj/mprUnix.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprUnix.c

${CC} -c -o ${CONFIG}/obj/mprVxworks.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprVxworks.c

${CC} -c -o ${CONFIG}/obj/mprWait.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprWait.c

${CC} -c -o ${CONFIG}/obj/mprWide.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprWide.c

${CC} -c -o ${CONFIG}/obj/mprWin.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprWin.c

${CC} -c -o ${CONFIG}/obj/mprWince.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprWince.c

${CC} -c -o ${CONFIG}/obj/mprXml.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprXml.c

${CC} -shared -o ${CONFIG}/bin/libmpr.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/mpr.o ${CONFIG}/obj/mprAsync.o ${CONFIG}/obj/mprAtomic.o ${CONFIG}/obj/mprBuf.o ${CONFIG}/obj/mprCache.o ${CONFIG}/obj/mprCmd.o ${CONFIG}/obj/mprCond.o ${CONFIG}/obj/mprCrypt.o ${CONFIG}/obj/mprDisk.o ${CONFIG}/obj/mprDispatcher.o ${CONFIG}/obj/mprEncode.o ${CONFIG}/obj/mprEpoll.o ${CONFIG}/obj/mprEvent.o ${CONFIG}/obj/mprFile.o ${CONFIG}/obj/mprFileSystem.o ${CONFIG}/obj/mprHash.o ${CONFIG}/obj/mprJSON.o ${CONFIG}/obj/mprKqueue.o ${CONFIG}/obj/mprList.o ${CONFIG}/obj/mprLock.o ${CONFIG}/obj/mprLog.o ${CONFIG}/obj/mprMem.o ${CONFIG}/obj/mprMime.o ${CONFIG}/obj/mprMixed.o ${CONFIG}/obj/mprModule.o ${CONFIG}/obj/mprPath.o ${CONFIG}/obj/mprPoll.o ${CONFIG}/obj/mprPrintf.o ${CONFIG}/obj/mprRomFile.o ${CONFIG}/obj/mprSelect.o ${CONFIG}/obj/mprSignal.o ${CONFIG}/obj/mprSocket.o ${CONFIG}/obj/mprString.o ${CONFIG}/obj/mprTest.o ${CONFIG}/obj/mprThread.o ${CONFIG}/obj/mprTime.o ${CONFIG}/obj/mprUnix.o ${CONFIG}/obj/mprVxworks.o ${CONFIG}/obj/mprWait.o ${CONFIG}/obj/mprWide.o ${CONFIG}/obj/mprWin.o ${CONFIG}/obj/mprWince.o ${CONFIG}/obj/mprXml.o ${LIBS}

${CC} -c -o ${CONFIG}/obj/benchMpr.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/benchMpr.c

${CC} -o ${CONFIG}/bin/benchMpr ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/benchMpr.o -lmpr ${LIBS} ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/runProgram.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/runProgram.c

${CC} -o ${CONFIG}/bin/runProgram ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/runProgram.o ${LIBS} ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/mprMatrixssl.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprMatrixssl.c

${CC} -c -o ${CONFIG}/obj/mprOpenssl.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprOpenssl.c

${CC} -c -o ${CONFIG}/obj/mprSsl.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mprSsl.c

${CC} -shared -o ${CONFIG}/bin/libmprssl.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/mprMatrixssl.o ${CONFIG}/obj/mprOpenssl.o ${CONFIG}/obj/mprSsl.o -lmpr ${LIBS}

${CC} -c -o ${CONFIG}/obj/testArgv.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testArgv.c

${CC} -c -o ${CONFIG}/obj/testBuf.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testBuf.c

${CC} -c -o ${CONFIG}/obj/testCmd.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testCmd.c

${CC} -c -o ${CONFIG}/obj/testCond.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testCond.c

${CC} -c -o ${CONFIG}/obj/testEvent.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testEvent.c

${CC} -c -o ${CONFIG}/obj/testFile.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testFile.c

${CC} -c -o ${CONFIG}/obj/testHash.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testHash.c

${CC} -c -o ${CONFIG}/obj/testList.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testList.c

${CC} -c -o ${CONFIG}/obj/testLock.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testLock.c

${CC} -c -o ${CONFIG}/obj/testMem.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testMem.c

${CC} -c -o ${CONFIG}/obj/testMpr.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testMpr.c

${CC} -c -o ${CONFIG}/obj/testPath.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testPath.c

${CC} -c -o ${CONFIG}/obj/testSocket.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testSocket.c

${CC} -c -o ${CONFIG}/obj/testSprintf.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testSprintf.c

${CC} -c -o ${CONFIG}/obj/testThread.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testThread.c

${CC} -c -o ${CONFIG}/obj/testTime.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testTime.c

${CC} -c -o ${CONFIG}/obj/testUnicode.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testUnicode.c

${CC} -o ${CONFIG}/bin/testMpr ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/testArgv.o ${CONFIG}/obj/testBuf.o ${CONFIG}/obj/testCmd.o ${CONFIG}/obj/testCond.o ${CONFIG}/obj/testEvent.o ${CONFIG}/obj/testFile.o ${CONFIG}/obj/testHash.o ${CONFIG}/obj/testList.o ${CONFIG}/obj/testLock.o ${CONFIG}/obj/testMem.o ${CONFIG}/obj/testMpr.o ${CONFIG}/obj/testPath.o ${CONFIG}/obj/testSocket.o ${CONFIG}/obj/testSprintf.o ${CONFIG}/obj/testThread.o ${CONFIG}/obj/testTime.o ${CONFIG}/obj/testUnicode.o -lmprssl -lmpr ${LIBS} ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/manager.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/manager.c

${CC} -o ${CONFIG}/bin/manager ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/manager.o -lmpr ${LIBS} ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/makerom.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/utils/makerom.c

${CC} -o ${CONFIG}/bin/makerom ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/makerom.o -lmpr ${LIBS} ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/charGen.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/utils/charGen.c

${CC} -o ${CONFIG}/bin/chargen ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/charGen.o -lmpr ${LIBS} ${LDFLAGS}

