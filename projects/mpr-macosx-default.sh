#
#   mpr-macosx-default.sh -- Build It Shell Script to build Multithreaded Portable Runtime
#

PRODUCT="mpr"
VERSION="4.3.0"
BUILD_NUMBER="0"
PROFILE="default"
ARCH="x64"
ARCH="`uname -m | sed 's/i.86/x86/;s/x86_64/x64/;s/arm.*/arm/;s/mips.*/mips/'`"
OS="macosx"
CONFIG="${OS}-${ARCH}-${PROFILE}"
CC="/usr/bin/clang"
LD="/usr/bin/ld"
CFLAGS="-O3   -w"
DFLAGS=""
IFLAGS="-I${CONFIG}/inc"
LDFLAGS="-Wl,-rpath,@executable_path/ -Wl,-rpath,@loader_path/"
LIBPATHS="-L${CONFIG}/bin"
LIBS="-lpthread -lm -ldl"

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

${DFLAGS}${CC} -c -o ${CONFIG}/obj/estLib.o -arch x86_64 -O3 -I${CONFIG}/inc src/deps/est/estLib.c

${DFLAGS}${CC} -dynamiclib -o ${CONFIG}/bin/libest.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.3.0 -current_version 4.3.0 ${LIBPATHS} -install_name @rpath/libest.dylib ${CONFIG}/obj/estLib.o ${LIBS}

rm -rf ${CONFIG}/bin/ca.crt
cp -r src/deps/est/ca.crt ${CONFIG}/bin/ca.crt

rm -rf ${CONFIG}/inc/mpr.h
cp -r src/mpr.h ${CONFIG}/inc/mpr.h

${DFLAGS}${CC} -c -o ${CONFIG}/obj/async.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/async.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/atomic.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/atomic.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/buf.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/buf.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/cache.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/cache.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/cmd.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/cmd.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/cond.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/cond.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/crypt.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/crypt.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/disk.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/disk.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/dispatcher.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/dispatcher.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/encode.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/encode.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/epoll.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/epoll.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/event.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/event.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/file.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/file.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/fs.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/fs.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/hash.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/hash.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/json.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/json.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/kqueue.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/kqueue.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/list.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/list.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/lock.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/lock.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/log.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/log.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/mem.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/mem.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/mime.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/mime.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/mixed.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/mixed.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/module.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/module.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/mpr.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/mpr.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/path.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/path.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/poll.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/poll.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/posix.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/posix.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/printf.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/printf.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/rom.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/rom.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/select.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/select.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/signal.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/signal.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/socket.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/socket.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/string.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/string.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/test.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/test.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/thread.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/thread.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/time.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/time.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/vxworks.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/vxworks.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/wait.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/wait.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/wide.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/wide.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/win.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/win.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/wince.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/wince.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/xml.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/xml.c

${DFLAGS}${CC} -dynamiclib -o ${CONFIG}/bin/libmpr.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.3.0 -current_version 4.3.0 ${LIBPATHS} -install_name @rpath/libmpr.dylib ${CONFIG}/obj/async.o ${CONFIG}/obj/atomic.o ${CONFIG}/obj/buf.o ${CONFIG}/obj/cache.o ${CONFIG}/obj/cmd.o ${CONFIG}/obj/cond.o ${CONFIG}/obj/crypt.o ${CONFIG}/obj/disk.o ${CONFIG}/obj/dispatcher.o ${CONFIG}/obj/encode.o ${CONFIG}/obj/epoll.o ${CONFIG}/obj/event.o ${CONFIG}/obj/file.o ${CONFIG}/obj/fs.o ${CONFIG}/obj/hash.o ${CONFIG}/obj/json.o ${CONFIG}/obj/kqueue.o ${CONFIG}/obj/list.o ${CONFIG}/obj/lock.o ${CONFIG}/obj/log.o ${CONFIG}/obj/mem.o ${CONFIG}/obj/mime.o ${CONFIG}/obj/mixed.o ${CONFIG}/obj/module.o ${CONFIG}/obj/mpr.o ${CONFIG}/obj/path.o ${CONFIG}/obj/poll.o ${CONFIG}/obj/posix.o ${CONFIG}/obj/printf.o ${CONFIG}/obj/rom.o ${CONFIG}/obj/select.o ${CONFIG}/obj/signal.o ${CONFIG}/obj/socket.o ${CONFIG}/obj/string.o ${CONFIG}/obj/test.o ${CONFIG}/obj/thread.o ${CONFIG}/obj/time.o ${CONFIG}/obj/vxworks.o ${CONFIG}/obj/wait.o ${CONFIG}/obj/wide.o ${CONFIG}/obj/win.o ${CONFIG}/obj/wince.o ${CONFIG}/obj/xml.o ${LIBS}

${DFLAGS}${CC} -c -o ${CONFIG}/obj/benchMpr.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/benchMpr.c

${DFLAGS}${CC} -o ${CONFIG}/bin/benchMpr -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/benchMpr.o -lmpr ${LIBS}

${DFLAGS}${CC} -c -o ${CONFIG}/obj/runProgram.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/runProgram.c

${DFLAGS}${CC} -o ${CONFIG}/bin/runProgram -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/runProgram.o ${LIBS}

${DFLAGS}${CC} -c -o ${CONFIG}/obj/est.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc -Isrc/deps/est src/ssl/est.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/matrixssl.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc -Isrc/deps/est src/ssl/matrixssl.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/mocana.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc -Isrc/deps/est src/ssl/mocana.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/openssl.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc -Isrc/deps/est src/ssl/openssl.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/ssl.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc -Isrc/deps/est src/ssl/ssl.c

${DFLAGS}${CC} -dynamiclib -o ${CONFIG}/bin/libmprssl.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.3.0 -current_version 4.3.0 ${LIBPATHS} -install_name @rpath/libmprssl.dylib ${CONFIG}/obj/est.o ${CONFIG}/obj/matrixssl.o ${CONFIG}/obj/mocana.o ${CONFIG}/obj/openssl.o ${CONFIG}/obj/ssl.o -lmpr ${LIBS} -lest

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testArgv.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testArgv.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testBuf.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testBuf.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testCmd.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testCmd.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testCond.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testCond.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testEvent.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testEvent.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testFile.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testFile.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testHash.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testHash.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testList.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testList.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testLock.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testLock.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testMem.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testMem.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testMpr.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testMpr.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testPath.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testPath.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testSocket.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testSocket.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testSprintf.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testSprintf.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testThread.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testThread.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testTime.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testTime.c

${DFLAGS}${CC} -c -o ${CONFIG}/obj/testUnicode.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc test/testUnicode.c

${DFLAGS}${CC} -o ${CONFIG}/bin/testMpr -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/testArgv.o ${CONFIG}/obj/testBuf.o ${CONFIG}/obj/testCmd.o ${CONFIG}/obj/testCond.o ${CONFIG}/obj/testEvent.o ${CONFIG}/obj/testFile.o ${CONFIG}/obj/testHash.o ${CONFIG}/obj/testList.o ${CONFIG}/obj/testLock.o ${CONFIG}/obj/testMem.o ${CONFIG}/obj/testMpr.o ${CONFIG}/obj/testPath.o ${CONFIG}/obj/testSocket.o ${CONFIG}/obj/testSprintf.o ${CONFIG}/obj/testThread.o ${CONFIG}/obj/testTime.o ${CONFIG}/obj/testUnicode.o -lmprssl -lmpr ${LIBS} -lest

${DFLAGS}${CC} -c -o ${CONFIG}/obj/manager.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/manager.c

${DFLAGS}${CC} -o ${CONFIG}/bin/manager -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/manager.o -lmpr ${LIBS}

${DFLAGS}${CC} -c -o ${CONFIG}/obj/charGen.o -arch x86_64 ${CFLAGS} -I${CONFIG}/inc src/utils/charGen.c

${DFLAGS}${CC} -o ${CONFIG}/bin/chargen -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/charGen.o -lmpr ${LIBS}

#  Omit build script undefined
