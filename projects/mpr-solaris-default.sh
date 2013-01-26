#
#   mpr-solaris-default.sh -- Build It Shell Script to build Multithreaded Portable Runtime
#

PRODUCT="mpr"
VERSION="4.3.0"
BUILD_NUMBER="0"
PROFILE="default"
ARCH="x86"
ARCH="`uname -m | sed 's/i.86/x86/;s/x86_64/x64/;s/arm.*/arm/;s/mips.*/mips/'`"
OS="solaris"
CONFIG="${OS}-${ARCH}-${PROFILE}"
CC="/usr/bin/gcc"
LD="/usr/bin/ld"
CFLAGS="-fPIC -O2 -w"
DFLAGS="-D_REENTRANT -DPIC"
IFLAGS="-I${CONFIG}/inc"
LDFLAGS=""
LIBPATHS="-L${CONFIG}/bin"
LIBS="-llxnet -lrt -lsocket -lpthread -lm -ldl"

[ ! -x ${CONFIG}/inc ] && mkdir -p ${CONFIG}/inc ${CONFIG}/obj ${CONFIG}/lib ${CONFIG}/bin

[ ! -f ${CONFIG}/inc/bit.h ] && cp projects/mpr-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h
[ ! -f ${CONFIG}/inc/bitos.h ] && cp ${SRC}/src/bitos.h ${CONFIG}/inc/bitos.h
if ! diff ${CONFIG}/inc/bit.h projects/mpr-${OS}-${PROFILE}-bit.h >/dev/null ; then
	cp projects/mpr-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h
fi

rm -rf ${CONFIG}/inc/bitos.h
cp -r src/bitos.h ${CONFIG}/inc/bitos.h

rm -rf ${CONFIG}/inc/est.h
cp -r src/deps/est/est.h ${CONFIG}/inc/est.h

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/estLib.o -fPIC -O2 ${DFLAGS} -I${CONFIG}/inc src/deps/est/estLib.c

${LDFLAGS}${LDFLAGS}${CC} -shared -o ${CONFIG}/bin/libest.so ${LIBPATHS} ${CONFIG}/obj/estLib.o ${LIBS}

rm -rf ${CONFIG}/bin/ca.crt
cp -r src/deps/est/ca.crt ${CONFIG}/bin/ca.crt

rm -rf ${CONFIG}/inc/mpr.h
cp -r src/mpr.h ${CONFIG}/inc/mpr.h

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/async.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/async.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/atomic.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/atomic.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/buf.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/buf.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/cache.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cache.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/cmd.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cmd.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/cond.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cond.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/crypt.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/crypt.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/disk.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/disk.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/dispatcher.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/dispatcher.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/encode.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/encode.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/epoll.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/epoll.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/event.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/event.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/file.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/file.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/fs.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/fs.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/hash.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/hash.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/json.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/json.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/kqueue.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/kqueue.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/list.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/list.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/lock.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/lock.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/log.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/log.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/mem.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mem.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/mime.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mime.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/mixed.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mixed.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/module.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/module.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/mpr.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mpr.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/path.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/path.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/poll.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/poll.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/posix.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/posix.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/printf.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/printf.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/rom.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/rom.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/select.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/select.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/signal.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/signal.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/socket.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/socket.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/string.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/string.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/test.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/test.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/thread.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/thread.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/time.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/time.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/vxworks.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/vxworks.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/wait.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/wait.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/wide.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/wide.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/win.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/win.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/wince.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/wince.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/xml.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/xml.c

${LDFLAGS}${LDFLAGS}${CC} -shared -o ${CONFIG}/bin/libmpr.so ${LIBPATHS} ${CONFIG}/obj/async.o ${CONFIG}/obj/atomic.o ${CONFIG}/obj/buf.o ${CONFIG}/obj/cache.o ${CONFIG}/obj/cmd.o ${CONFIG}/obj/cond.o ${CONFIG}/obj/crypt.o ${CONFIG}/obj/disk.o ${CONFIG}/obj/dispatcher.o ${CONFIG}/obj/encode.o ${CONFIG}/obj/epoll.o ${CONFIG}/obj/event.o ${CONFIG}/obj/file.o ${CONFIG}/obj/fs.o ${CONFIG}/obj/hash.o ${CONFIG}/obj/json.o ${CONFIG}/obj/kqueue.o ${CONFIG}/obj/list.o ${CONFIG}/obj/lock.o ${CONFIG}/obj/log.o ${CONFIG}/obj/mem.o ${CONFIG}/obj/mime.o ${CONFIG}/obj/mixed.o ${CONFIG}/obj/module.o ${CONFIG}/obj/mpr.o ${CONFIG}/obj/path.o ${CONFIG}/obj/poll.o ${CONFIG}/obj/posix.o ${CONFIG}/obj/printf.o ${CONFIG}/obj/rom.o ${CONFIG}/obj/select.o ${CONFIG}/obj/signal.o ${CONFIG}/obj/socket.o ${CONFIG}/obj/string.o ${CONFIG}/obj/test.o ${CONFIG}/obj/thread.o ${CONFIG}/obj/time.o ${CONFIG}/obj/vxworks.o ${CONFIG}/obj/wait.o ${CONFIG}/obj/wide.o ${CONFIG}/obj/win.o ${CONFIG}/obj/wince.o ${CONFIG}/obj/xml.o ${LIBS}

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/benchMpr.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/benchMpr.c

${LDFLAGS}${LDFLAGS}${CC} -o ${CONFIG}/bin/benchMpr ${LIBPATHS} ${CONFIG}/obj/benchMpr.o -lmpr ${LIBS} -lmpr -llxnet -lrt -lsocket -lpthread -lm -ldl 

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/runProgram.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/runProgram.c

${LDFLAGS}${LDFLAGS}${CC} -o ${CONFIG}/bin/runProgram ${LIBPATHS} ${CONFIG}/obj/runProgram.o ${LIBS} -llxnet -lrt -lsocket -lpthread -lm -ldl 

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/est.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/deps/est src/ssl/est.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/matrixssl.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/deps/est src/ssl/matrixssl.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/mocana.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/deps/est src/ssl/mocana.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/openssl.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/deps/est src/ssl/openssl.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/ssl.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/deps/est src/ssl/ssl.c

${LDFLAGS}${LDFLAGS}${CC} -shared -o ${CONFIG}/bin/libmprssl.so ${LIBPATHS} ${CONFIG}/obj/est.o ${CONFIG}/obj/matrixssl.o ${CONFIG}/obj/mocana.o ${CONFIG}/obj/openssl.o ${CONFIG}/obj/ssl.o -lmpr ${LIBS} -lest

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testArgv.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testArgv.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testBuf.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testBuf.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testCmd.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testCmd.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testCond.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testCond.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testEvent.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testEvent.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testFile.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testFile.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testHash.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testHash.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testList.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testList.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testLock.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testLock.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testMem.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testMem.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testMpr.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testMpr.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testPath.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testPath.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testSocket.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testSocket.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testSprintf.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testSprintf.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testThread.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testThread.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testTime.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testTime.c

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/testUnicode.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testUnicode.c

${LDFLAGS}${LDFLAGS}${CC} -o ${CONFIG}/bin/testMpr ${LIBPATHS} ${CONFIG}/obj/testArgv.o ${CONFIG}/obj/testBuf.o ${CONFIG}/obj/testCmd.o ${CONFIG}/obj/testCond.o ${CONFIG}/obj/testEvent.o ${CONFIG}/obj/testFile.o ${CONFIG}/obj/testHash.o ${CONFIG}/obj/testList.o ${CONFIG}/obj/testLock.o ${CONFIG}/obj/testMem.o ${CONFIG}/obj/testMpr.o ${CONFIG}/obj/testPath.o ${CONFIG}/obj/testSocket.o ${CONFIG}/obj/testSprintf.o ${CONFIG}/obj/testThread.o ${CONFIG}/obj/testTime.o ${CONFIG}/obj/testUnicode.o -lmprssl -lmpr ${LIBS} -lest -lmprssl -lmpr -llxnet -lrt -lsocket -lpthread -lm -ldl -lest 

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/manager.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/manager.c

${LDFLAGS}${LDFLAGS}${CC} -o ${CONFIG}/bin/manager ${LIBPATHS} ${CONFIG}/obj/manager.o -lmpr ${LIBS} -lmpr -llxnet -lrt -lsocket -lpthread -lm -ldl 

${LDFLAGS}${LDFLAGS}${CC} -c -o ${CONFIG}/obj/charGen.o ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/utils/charGen.c

${LDFLAGS}${LDFLAGS}${CC} -o ${CONFIG}/bin/chargen ${LIBPATHS} ${CONFIG}/obj/charGen.o -lmpr ${LIBS} -lmpr -llxnet -lrt -lsocket -lpthread -lm -ldl 

#  Omit build script undefined
