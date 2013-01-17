#
#   mpr-windows-debug.sh -- Build It Shell Script to build Multithreaded Portable Runtime
#

export PATH="$(SDK)/Bin:$(VS)/VC/Bin:$(VS)/Common7/IDE:$(VS)/Common7/Tools:$(VS)/SDK/v3.5/bin:$(VS)/VC/VCPackages;$(PATH)"
export INCLUDE="$(INCLUDE);$(SDK)/Include:$(VS)/VC/INCLUDE"
export LIB="$(LIB);$(SDK)/Lib:$(VS)/VC/lib"

ARCH="x86"
ARCH="`uname -m | sed 's/i.86/x86/;s/x86_64/x64/;s/arm.*/arm/;s/mips.*/mips/'`"
OS="windows"
PROFILE="debug"
CONFIG="${OS}-${ARCH}-${PROFILE}"
CC="cl.exe"
LD="link.exe"
CFLAGS="-nologo -GR- -W3 -Zi -Od -MDd -w"
DFLAGS="-D_REENTRANT -D_MT -DBIT_DEBUG"
IFLAGS="-I${CONFIG}/inc"
LDFLAGS="-nologo -nodefaultlib -incremental:no -debug -machine:x86"
LIBPATHS="-libpath:${CONFIG}/bin"
LIBS="ws2_32.lib advapi32.lib user32.lib kernel32.lib oldnames.lib msvcrt.lib shell32.lib"

[ ! -x ${CONFIG}/inc ] && mkdir -p ${CONFIG}/inc ${CONFIG}/obj ${CONFIG}/lib ${CONFIG}/bin

[ ! -f ${CONFIG}/inc/bit.h ] && cp projects/mpr-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h
[ ! -f ${CONFIG}/inc/bitos.h ] && cp ${SRC}/src/bitos.h ${CONFIG}/inc/bitos.h
if ! diff ${CONFIG}/inc/bit.h projects/mpr-${OS}-${PROFILE}-bit.h >/dev/null ; then
	cp projects/mpr-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h
fi

rm -rf ${CONFIG}/bin/ca.crt
cp -r src/deps/est/ca.crt ${CONFIG}/bin/ca.crt

rm -rf ${CONFIG}/inc/bitos.h
cp -r src/bitos.h ${CONFIG}/inc/bitos.h

rm -rf ${CONFIG}/inc/mpr.h
cp -r src/mpr.h ${CONFIG}/inc/mpr.h

"${CC}" -c -Fo${CONFIG}/obj/async.obj -Fd${CONFIG}/obj/async.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/async.c

"${CC}" -c -Fo${CONFIG}/obj/atomic.obj -Fd${CONFIG}/obj/atomic.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/atomic.c

"${CC}" -c -Fo${CONFIG}/obj/buf.obj -Fd${CONFIG}/obj/buf.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/buf.c

"${CC}" -c -Fo${CONFIG}/obj/cache.obj -Fd${CONFIG}/obj/cache.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cache.c

"${CC}" -c -Fo${CONFIG}/obj/cmd.obj -Fd${CONFIG}/obj/cmd.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cmd.c

"${CC}" -c -Fo${CONFIG}/obj/cond.obj -Fd${CONFIG}/obj/cond.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cond.c

"${CC}" -c -Fo${CONFIG}/obj/crypt.obj -Fd${CONFIG}/obj/crypt.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/crypt.c

"${CC}" -c -Fo${CONFIG}/obj/disk.obj -Fd${CONFIG}/obj/disk.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/disk.c

"${CC}" -c -Fo${CONFIG}/obj/dispatcher.obj -Fd${CONFIG}/obj/dispatcher.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/dispatcher.c

"${CC}" -c -Fo${CONFIG}/obj/encode.obj -Fd${CONFIG}/obj/encode.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/encode.c

"${CC}" -c -Fo${CONFIG}/obj/epoll.obj -Fd${CONFIG}/obj/epoll.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/epoll.c

"${CC}" -c -Fo${CONFIG}/obj/event.obj -Fd${CONFIG}/obj/event.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/event.c

"${CC}" -c -Fo${CONFIG}/obj/file.obj -Fd${CONFIG}/obj/file.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/file.c

"${CC}" -c -Fo${CONFIG}/obj/fs.obj -Fd${CONFIG}/obj/fs.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/fs.c

"${CC}" -c -Fo${CONFIG}/obj/hash.obj -Fd${CONFIG}/obj/hash.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/hash.c

"${CC}" -c -Fo${CONFIG}/obj/json.obj -Fd${CONFIG}/obj/json.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/json.c

"${CC}" -c -Fo${CONFIG}/obj/kqueue.obj -Fd${CONFIG}/obj/kqueue.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/kqueue.c

"${CC}" -c -Fo${CONFIG}/obj/list.obj -Fd${CONFIG}/obj/list.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/list.c

"${CC}" -c -Fo${CONFIG}/obj/lock.obj -Fd${CONFIG}/obj/lock.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/lock.c

"${CC}" -c -Fo${CONFIG}/obj/log.obj -Fd${CONFIG}/obj/log.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/log.c

"${CC}" -c -Fo${CONFIG}/obj/mem.obj -Fd${CONFIG}/obj/mem.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mem.c

"${CC}" -c -Fo${CONFIG}/obj/mime.obj -Fd${CONFIG}/obj/mime.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mime.c

"${CC}" -c -Fo${CONFIG}/obj/mixed.obj -Fd${CONFIG}/obj/mixed.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mixed.c

"${CC}" -c -Fo${CONFIG}/obj/module.obj -Fd${CONFIG}/obj/module.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/module.c

"${CC}" -c -Fo${CONFIG}/obj/mpr.obj -Fd${CONFIG}/obj/mpr.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/mpr.c

"${CC}" -c -Fo${CONFIG}/obj/path.obj -Fd${CONFIG}/obj/path.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/path.c

"${CC}" -c -Fo${CONFIG}/obj/poll.obj -Fd${CONFIG}/obj/poll.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/poll.c

"${CC}" -c -Fo${CONFIG}/obj/posix.obj -Fd${CONFIG}/obj/posix.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/posix.c

"${CC}" -c -Fo${CONFIG}/obj/printf.obj -Fd${CONFIG}/obj/printf.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/printf.c

"${CC}" -c -Fo${CONFIG}/obj/rom.obj -Fd${CONFIG}/obj/rom.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/rom.c

"${CC}" -c -Fo${CONFIG}/obj/select.obj -Fd${CONFIG}/obj/select.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/select.c

"${CC}" -c -Fo${CONFIG}/obj/signal.obj -Fd${CONFIG}/obj/signal.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/signal.c

"${CC}" -c -Fo${CONFIG}/obj/socket.obj -Fd${CONFIG}/obj/socket.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/socket.c

"${CC}" -c -Fo${CONFIG}/obj/string.obj -Fd${CONFIG}/obj/string.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/string.c

"${CC}" -c -Fo${CONFIG}/obj/test.obj -Fd${CONFIG}/obj/test.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/test.c

"${CC}" -c -Fo${CONFIG}/obj/thread.obj -Fd${CONFIG}/obj/thread.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/thread.c

"${CC}" -c -Fo${CONFIG}/obj/time.obj -Fd${CONFIG}/obj/time.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/time.c

"${CC}" -c -Fo${CONFIG}/obj/vxworks.obj -Fd${CONFIG}/obj/vxworks.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/vxworks.c

"${CC}" -c -Fo${CONFIG}/obj/wait.obj -Fd${CONFIG}/obj/wait.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/wait.c

"${CC}" -c -Fo${CONFIG}/obj/wide.obj -Fd${CONFIG}/obj/wide.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/wide.c

"${CC}" -c -Fo${CONFIG}/obj/win.obj -Fd${CONFIG}/obj/win.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/win.c

"${CC}" -c -Fo${CONFIG}/obj/wince.obj -Fd${CONFIG}/obj/wince.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/wince.c

"${CC}" -c -Fo${CONFIG}/obj/xml.obj -Fd${CONFIG}/obj/xml.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/xml.c

"${LD}" -dll -out:${CONFIG}/bin/libmpr.dll -entry:_DllMainCRTStartup@12 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/async.obj ${CONFIG}/obj/atomic.obj ${CONFIG}/obj/buf.obj ${CONFIG}/obj/cache.obj ${CONFIG}/obj/cmd.obj ${CONFIG}/obj/cond.obj ${CONFIG}/obj/crypt.obj ${CONFIG}/obj/disk.obj ${CONFIG}/obj/dispatcher.obj ${CONFIG}/obj/encode.obj ${CONFIG}/obj/epoll.obj ${CONFIG}/obj/event.obj ${CONFIG}/obj/file.obj ${CONFIG}/obj/fs.obj ${CONFIG}/obj/hash.obj ${CONFIG}/obj/json.obj ${CONFIG}/obj/kqueue.obj ${CONFIG}/obj/list.obj ${CONFIG}/obj/lock.obj ${CONFIG}/obj/log.obj ${CONFIG}/obj/mem.obj ${CONFIG}/obj/mime.obj ${CONFIG}/obj/mixed.obj ${CONFIG}/obj/module.obj ${CONFIG}/obj/mpr.obj ${CONFIG}/obj/path.obj ${CONFIG}/obj/poll.obj ${CONFIG}/obj/posix.obj ${CONFIG}/obj/printf.obj ${CONFIG}/obj/rom.obj ${CONFIG}/obj/select.obj ${CONFIG}/obj/signal.obj ${CONFIG}/obj/socket.obj ${CONFIG}/obj/string.obj ${CONFIG}/obj/test.obj ${CONFIG}/obj/thread.obj ${CONFIG}/obj/time.obj ${CONFIG}/obj/vxworks.obj ${CONFIG}/obj/wait.obj ${CONFIG}/obj/wide.obj ${CONFIG}/obj/win.obj ${CONFIG}/obj/wince.obj ${CONFIG}/obj/xml.obj ${LIBS}

"${CC}" -c -Fo${CONFIG}/obj/benchMpr.obj -Fd${CONFIG}/obj/benchMpr.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/benchMpr.c

"${LD}" -out:${CONFIG}/bin/benchMpr.exe -entry:mainCRTStartup -subsystem:console ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/benchMpr.obj libmpr.lib ${LIBS}

"${CC}" -c -Fo${CONFIG}/obj/runProgram.obj -Fd${CONFIG}/obj/runProgram.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/runProgram.c

"${LD}" -out:${CONFIG}/bin/runProgram.exe -entry:mainCRTStartup -subsystem:console ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/runProgram.obj ${LIBS}

rm -rf ${CONFIG}/inc/est.h
cp -r src/deps/est/est.h ${CONFIG}/inc/est.h

"${CC}" -c -Fo${CONFIG}/obj/est.obj -Fd${CONFIG}/obj/est.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/ssl/est.c

"${CC}" -c -Fo${CONFIG}/obj/matrixssl.obj -Fd${CONFIG}/obj/matrixssl.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/ssl/matrixssl.c

"${CC}" -c -Fo${CONFIG}/obj/mocana.obj -Fd${CONFIG}/obj/mocana.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/ssl/mocana.c

"${CC}" -c -Fo${CONFIG}/obj/openssl.obj -Fd${CONFIG}/obj/openssl.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/ssl/openssl.c

"${CC}" -c -Fo${CONFIG}/obj/ssl.obj -Fd${CONFIG}/obj/ssl.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/ssl/ssl.c

"${LD}" -dll -out:${CONFIG}/bin/libmprssl.dll -entry:_DllMainCRTStartup@12 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/est.obj ${CONFIG}/obj/matrixssl.obj ${CONFIG}/obj/mocana.obj ${CONFIG}/obj/openssl.obj ${CONFIG}/obj/ssl.obj libmpr.lib ${LIBS}

"${CC}" -c -Fo${CONFIG}/obj/testArgv.obj -Fd${CONFIG}/obj/testArgv.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testArgv.c

"${CC}" -c -Fo${CONFIG}/obj/testBuf.obj -Fd${CONFIG}/obj/testBuf.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testBuf.c

"${CC}" -c -Fo${CONFIG}/obj/testCmd.obj -Fd${CONFIG}/obj/testCmd.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testCmd.c

"${CC}" -c -Fo${CONFIG}/obj/testCond.obj -Fd${CONFIG}/obj/testCond.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testCond.c

"${CC}" -c -Fo${CONFIG}/obj/testEvent.obj -Fd${CONFIG}/obj/testEvent.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testEvent.c

"${CC}" -c -Fo${CONFIG}/obj/testFile.obj -Fd${CONFIG}/obj/testFile.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testFile.c

"${CC}" -c -Fo${CONFIG}/obj/testHash.obj -Fd${CONFIG}/obj/testHash.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testHash.c

"${CC}" -c -Fo${CONFIG}/obj/testList.obj -Fd${CONFIG}/obj/testList.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testList.c

"${CC}" -c -Fo${CONFIG}/obj/testLock.obj -Fd${CONFIG}/obj/testLock.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testLock.c

"${CC}" -c -Fo${CONFIG}/obj/testMem.obj -Fd${CONFIG}/obj/testMem.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testMem.c

"${CC}" -c -Fo${CONFIG}/obj/testMpr.obj -Fd${CONFIG}/obj/testMpr.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testMpr.c

"${CC}" -c -Fo${CONFIG}/obj/testPath.obj -Fd${CONFIG}/obj/testPath.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testPath.c

"${CC}" -c -Fo${CONFIG}/obj/testSocket.obj -Fd${CONFIG}/obj/testSocket.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testSocket.c

"${CC}" -c -Fo${CONFIG}/obj/testSprintf.obj -Fd${CONFIG}/obj/testSprintf.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testSprintf.c

"${CC}" -c -Fo${CONFIG}/obj/testThread.obj -Fd${CONFIG}/obj/testThread.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testThread.c

"${CC}" -c -Fo${CONFIG}/obj/testTime.obj -Fd${CONFIG}/obj/testTime.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testTime.c

"${CC}" -c -Fo${CONFIG}/obj/testUnicode.obj -Fd${CONFIG}/obj/testUnicode.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testUnicode.c

"${LD}" -out:${CONFIG}/bin/testMpr.exe -entry:mainCRTStartup -subsystem:console ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/testArgv.obj ${CONFIG}/obj/testBuf.obj ${CONFIG}/obj/testCmd.obj ${CONFIG}/obj/testCond.obj ${CONFIG}/obj/testEvent.obj ${CONFIG}/obj/testFile.obj ${CONFIG}/obj/testHash.obj ${CONFIG}/obj/testList.obj ${CONFIG}/obj/testLock.obj ${CONFIG}/obj/testMem.obj ${CONFIG}/obj/testMpr.obj ${CONFIG}/obj/testPath.obj ${CONFIG}/obj/testSocket.obj ${CONFIG}/obj/testSprintf.obj ${CONFIG}/obj/testThread.obj ${CONFIG}/obj/testTime.obj ${CONFIG}/obj/testUnicode.obj libmprssl.lib libmpr.lib ${LIBS}

"${CC}" -c -Fo${CONFIG}/obj/manager.obj -Fd${CONFIG}/obj/manager.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/manager.c

"${LD}" -out:${CONFIG}/bin/manager.exe -entry:WinMainCRTStartup -subsystem:Windows ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/manager.obj libmpr.lib ${LIBS}

"${CC}" -c -Fo${CONFIG}/obj/charGen.obj -Fd${CONFIG}/obj/charGen.pdb ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/utils/charGen.c

"${LD}" -out:${CONFIG}/bin/chargen.exe -entry:mainCRTStartup -subsystem:console ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/charGen.obj libmpr.lib ${LIBS}

