#
#   mpr-solaris-debug.sh -- Build It Shell Script to build Multithreaded Portable Runtime
#

ARCH="x86"
ARCH="`uname -m | sed 's/i.86/x86/;s/x86_64/x64/;s/arm.*/arm/;s/mips.*/mips/'`"
OS="solaris"
PROFILE="debug"
CONFIG="${OS}-${ARCH}-${PROFILE}"
CC="/usr/bin/gcc"
LD="/usr/bin/ld"
CFLAGS="-fPIC  -w"
DFLAGS="-D_REENTRANT -DPIC -DBIT_DEBUG"
IFLAGS="-I${CONFIG}/inc"
LDFLAGS="-g"
LIBPATHS="-L${CONFIG}/bin"
LIBS="-llxnet -lrt -lsocket -lpthread -lm -ldl"

[ ! -x ${CONFIG}/inc ] && mkdir -p ${CONFIG}/inc ${CONFIG}/obj ${CONFIG}/lib ${CONFIG}/bin

[ ! -f ${CONFIG}/inc/bit.h ] && cp projects/mpr-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h
[ ! -f ${CONFIG}/inc/bitos.h ] && cp src/bitos.h ${CONFIG}/inc/bitos.h
if ! diff ${CONFIG}/inc/bit.h projects/mpr-${OS}-${PROFILE}-bit.h >/dev/null ; then
	cp projects/mpr-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h
fi

rm -rf ${CONFIG}/inc/bitos.h
cp -r src/bitos.h ${CONFIG}/inc/bitos.h

rm -rf ${CONFIG}/inc/mpr.h
cp -r src/mpr.h ${CONFIG}/inc/mpr.h

${CC} -c -o ${CONFIG}/obj/async.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/async.c

${CC} -c -o ${CONFIG}/obj/atomic.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/atomic.c

${CC} -c -o ${CONFIG}/obj/buf.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/buf.c

${CC} -c -o ${CONFIG}/obj/cache.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cache.c

${CC} -c -o ${CONFIG}/obj/cmd.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cmd.c

${CC} -c -o ${CONFIG}/obj/cond.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cond.c

${CC} -c -o ${CONFIG}/obj/crypt.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/crypt.c

${CC} -c -o ${CONFIG}/obj/disk.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/disk.c

${CC} -c -o ${CONFIG}/obj/dispatcher.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/dispatcher.c

${CC} -c -o ${CONFIG}/obj/encode.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/encode.c

${CC} -c -o ${CONFIG}/obj/epoll.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/epoll.c

${CC} -c -o ${CONFIG}/obj/event.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/event.c

${CC} -c -o ${CONFIG}/obj/file.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/file.c

${CC} -c -o ${CONFIG}/obj/fs.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/fs.c

${CC} -c -o ${CONFIG}/obj/hash.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/hash.c

${CC} -c -o ${CONFIG}/obj/jSON.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/jSON.c

${CC} -c -o ${CONFIG}/obj/kqueue.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/kqueue.c

${CC} -c -o ${CONFIG}/obj/list.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/list.c

${CC} -c -o ${CONFIG}/obj/lock.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/lock.c

${CC} -c -o ${CONFIG}/obj/log.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/log.c

${CC} -c -o ${CONFIG}/obj/mem.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mem.c

${CC} -c -o ${CONFIG}/obj/mime.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mime.c

${CC} -c -o ${CONFIG}/obj/mixed.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mixed.c

${CC} -c -o ${CONFIG}/obj/module.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/module.c

${CC} -c -o ${CONFIG}/obj/mpr.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mpr.c

${CC} -c -o ${CONFIG}/obj/path.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/path.c

${CC} -c -o ${CONFIG}/obj/poll.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/poll.c

${CC} -c -o ${CONFIG}/obj/posix.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/posix.c

${CC} -c -o ${CONFIG}/obj/printf.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/printf.c

${CC} -c -o ${CONFIG}/obj/rom.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/rom.c

${CC} -c -o ${CONFIG}/obj/select.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/select.c

${CC} -c -o ${CONFIG}/obj/signal.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/signal.c

${CC} -c -o ${CONFIG}/obj/socket.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/socket.c

${CC} -c -o ${CONFIG}/obj/string.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/string.c

${CC} -c -o ${CONFIG}/obj/test.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/test.c

${CC} -c -o ${CONFIG}/obj/thread.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/thread.c

${CC} -c -o ${CONFIG}/obj/time.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/time.c

${CC} -c -o ${CONFIG}/obj/vxworks.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/vxworks.c

${CC} -c -o ${CONFIG}/obj/wait.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/wait.c

${CC} -c -o ${CONFIG}/obj/wide.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/wide.c

${CC} -c -o ${CONFIG}/obj/win.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/win.c

${CC} -c -o ${CONFIG}/obj/wince.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/wince.c

${CC} -c -o ${CONFIG}/obj/xml.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/xml.c

${CC} -shared -o ${CONFIG}/bin/libmpr.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/async.o ${CONFIG}/obj/atomic.o ${CONFIG}/obj/buf.o ${CONFIG}/obj/cache.o ${CONFIG}/obj/cmd.o ${CONFIG}/obj/cond.o ${CONFIG}/obj/crypt.o ${CONFIG}/obj/disk.o ${CONFIG}/obj/dispatcher.o ${CONFIG}/obj/encode.o ${CONFIG}/obj/epoll.o ${CONFIG}/obj/event.o ${CONFIG}/obj/file.o ${CONFIG}/obj/fs.o ${CONFIG}/obj/hash.o ${CONFIG}/obj/jSON.o ${CONFIG}/obj/kqueue.o ${CONFIG}/obj/list.o ${CONFIG}/obj/lock.o ${CONFIG}/obj/log.o ${CONFIG}/obj/mem.o ${CONFIG}/obj/mime.o ${CONFIG}/obj/mixed.o ${CONFIG}/obj/module.o ${CONFIG}/obj/mpr.o ${CONFIG}/obj/path.o ${CONFIG}/obj/poll.o ${CONFIG}/obj/posix.o ${CONFIG}/obj/printf.o ${CONFIG}/obj/rom.o ${CONFIG}/obj/select.o ${CONFIG}/obj/signal.o ${CONFIG}/obj/socket.o ${CONFIG}/obj/string.o ${CONFIG}/obj/test.o ${CONFIG}/obj/thread.o ${CONFIG}/obj/time.o ${CONFIG}/obj/vxworks.o ${CONFIG}/obj/wait.o ${CONFIG}/obj/wide.o ${CONFIG}/obj/win.o ${CONFIG}/obj/wince.o ${CONFIG}/obj/xml.o ${LIBS}

${CC} -c -o ${CONFIG}/obj/benchMpr.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/benchMpr.c

${CC} -o ${CONFIG}/bin/benchMpr ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/benchMpr.o -lmpr ${LIBS} ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/runProgram.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/runProgram.c

${CC} -o ${CONFIG}/bin/runProgram ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/runProgram.o ${LIBS} ${LDFLAGS}

rm -rf ${CONFIG}/inc/est.h
cp -r src/deps/est/est.h ${CONFIG}/inc/est.h

${CC} -c -o ${CONFIG}/obj/est.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/ssl/est.c

${CC} -c -o ${CONFIG}/obj/matrixssl.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/ssl/matrixssl.c

${CC} -c -o ${CONFIG}/obj/openssl.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/ssl/openssl.c

${CC} -c -o ${CONFIG}/obj/ssl.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/ssl/ssl.c

${CC} -shared -o ${CONFIG}/bin/libmprssl.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/est.o ${CONFIG}/obj/matrixssl.o ${CONFIG}/obj/openssl.o ${CONFIG}/obj/ssl.o -lmpr ${LIBS}

${CC} -c -o ${CONFIG}/obj/testArgv.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testArgv.c

${CC} -c -o ${CONFIG}/obj/testBuf.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testBuf.c

${CC} -c -o ${CONFIG}/obj/testCmd.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testCmd.c

${CC} -c -o ${CONFIG}/obj/testCond.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testCond.c

${CC} -c -o ${CONFIG}/obj/testEvent.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testEvent.c

${CC} -c -o ${CONFIG}/obj/testFile.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testFile.c

${CC} -c -o ${CONFIG}/obj/testHash.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testHash.c

${CC} -c -o ${CONFIG}/obj/testList.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testList.c

${CC} -c -o ${CONFIG}/obj/testLock.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testLock.c

${CC} -c -o ${CONFIG}/obj/testMem.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testMem.c

${CC} -c -o ${CONFIG}/obj/testMpr.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testMpr.c

${CC} -c -o ${CONFIG}/obj/testPath.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testPath.c

${CC} -c -o ${CONFIG}/obj/testSocket.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testSocket.c

${CC} -c -o ${CONFIG}/obj/testSprintf.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testSprintf.c

${CC} -c -o ${CONFIG}/obj/testThread.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testThread.c

${CC} -c -o ${CONFIG}/obj/testTime.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testTime.c

${CC} -c -o ${CONFIG}/obj/testUnicode.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testUnicode.c

${CC} -o ${CONFIG}/bin/testMpr ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/testArgv.o ${CONFIG}/obj/testBuf.o ${CONFIG}/obj/testCmd.o ${CONFIG}/obj/testCond.o ${CONFIG}/obj/testEvent.o ${CONFIG}/obj/testFile.o ${CONFIG}/obj/testHash.o ${CONFIG}/obj/testList.o ${CONFIG}/obj/testLock.o ${CONFIG}/obj/testMem.o ${CONFIG}/obj/testMpr.o ${CONFIG}/obj/testPath.o ${CONFIG}/obj/testSocket.o ${CONFIG}/obj/testSprintf.o ${CONFIG}/obj/testThread.o ${CONFIG}/obj/testTime.o ${CONFIG}/obj/testUnicode.o -lmprssl -lmpr ${LIBS} ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/manager.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/manager.c

${CC} -o ${CONFIG}/bin/manager ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/manager.o -lmpr ${LIBS} ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/makerom.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/utils/makerom.c

${CC} -o ${CONFIG}/bin/makerom ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/makerom.o -lmpr ${LIBS} ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/charGen.o -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/utils/charGen.c

${CC} -o ${CONFIG}/bin/chargen ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/charGen.o -lmpr ${LIBS} ${LDFLAGS}

