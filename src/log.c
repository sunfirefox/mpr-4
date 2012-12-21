/**
    log.c - Multithreaded Portable Runtime (MPR) Logging and error reporting.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

/********************************** Defines ***********************************/

#ifndef BIT_MAX_LOGLINE
    #define BIT_MAX_LOGLINE 8192           /* Max size of a log line */
#endif

/********************************** Forwards **********************************/

static void defaultLogHandler(int flags, int level, cchar *msg);
static void logOutput(int flags, int level, cchar *msg);

/************************************ Code ************************************/
/*
    Put first in file so it is easy to locate in a debugger
 */
PUBLIC void mprBreakpoint()
{
#if DEBUG_PAUSE
    {
        static int  paused = 1;
        int         i;
        printf("Paused to permit debugger to attach - will awake in 2 minutes\n");
        fflush(stdout);
        for (i = 0; i < 120 && paused; i++) {
            mprNap(1000);
        }
    }
#endif
}


PUBLIC void mprCreateLogService() 
{
    MPR->logFile = MPR->stdError;
}


PUBLIC int mprStartLogging(cchar *logSpec, int showConfig)
{
    MprFile     *file;
    MprPath     info;
    char        *levelSpec, *path;
    int         level, mode;

    level = -1;
    if (logSpec == 0) {
        logSpec = "stderr:0";
    }
    if (*logSpec && strcmp(logSpec, "none") != 0) {
        MPR->logPath = path = sclone(logSpec);
        if ((levelSpec = strrchr(path, ':')) != 0 && isdigit((uchar) levelSpec[1])) {
            *levelSpec++ = '\0';
            level = atoi(levelSpec);
        }
        if (strcmp(path, "stdout") == 0) {
            file = MPR->stdOutput;
        } else if (strcmp(path, "stderr") == 0) {
            file = MPR->stdError;
        } else {
            mode = (MPR->flags & MPR_LOG_APPEND)  ? O_APPEND : O_TRUNC;
            mode |= O_CREAT | O_WRONLY | O_TEXT;
            if (MPR->logBackup > 0) {
                mprGetPathInfo(path, &info);
                if (MPR->logSize <= 0 || (info.valid && info.size > MPR->logSize) || (MPR->flags & MPR_LOG_ANEW)) {
                    mprBackupLog(path, MPR->logBackup);
                }
            }
            if ((file = mprOpenFile(path, mode, 0664)) == 0) {
                mprError("Cannot open log file %s", path);
                return -1;
            }
        }
        if (level >= 0) {
            mprSetLogLevel(level);
        }
        mprSetLogFile(file);

        if (showConfig) {
            mprLogHeader();
        }
    }
    return 0;
}


PUBLIC void mprLogHeader()
{
    mprLog(MPR_CONFIG, "Configuration for %s", mprGetAppTitle());
    mprLog(MPR_CONFIG, "---------------------------------------------");
    mprLog(MPR_CONFIG, "Version:            %s-%s", BIT_VERSION, BIT_BUILD_NUMBER);
    mprLog(MPR_CONFIG, "BuildType:          %s", BIT_DEBUG ? "Debug" : "Release");
    mprLog(MPR_CONFIG, "CPU:                %s", BIT_CPU);
    mprLog(MPR_CONFIG, "OS:                 %s", BIT_OS);
    mprLog(MPR_CONFIG, "Host:               %s", mprGetHostName());
    mprLog(MPR_CONFIG, "Directory:          %s", mprGetCurrentPath());
    mprLog(MPR_CONFIG, "Configure:          %s", BIT_CONFIG_CMD);
    mprLog(MPR_CONFIG, "---------------------------------------------");
}


PUBLIC int mprBackupLog(cchar *path, int count)
{
    char    *from, *to;
    int     i;

    for (i = count - 1; i > 0; i--) {
        from = sfmt("%s.%d", path, i - 1);
        to = sfmt("%s.%d", path, i);
        unlink(to);
        rename(from, to);
    }
    from = sfmt("%s", path);
    to = sfmt("%s.0", path);
    unlink(to);
    if (rename(from, to) < 0) {
        return MPR_ERR_CANT_CREATE;
    }
    return 0;
}


PUBLIC void mprSetLogBackup(ssize size, int backup, int flags)
{
    MPR->logBackup = backup;
    MPR->logSize = size;
    MPR->flags |= (flags & (MPR_LOG_APPEND | MPR_LOG_ANEW));
}


PUBLIC void mprLog(int level, cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    if (level < 0 || level > mprGetLogLevel()) {
        return;
    }
    va_start(args, fmt);
    fmtv(buf, sizeof(buf), fmt, args);
    va_end(args);
    logOutput(MPR_LOG_SRC, level, buf);
}


/*
    RawLog will call alloc. 
 */
PUBLIC void mprRawLog(int level, cchar *fmt, ...)
{
    va_list     args;
    char        *buf;

    if (level > mprGetLogLevel()) {
        return;
    }
    va_start(args, fmt);
    buf = sfmtv(fmt, args);
    va_end(args);

    logOutput(MPR_RAW, 0, buf);
}


PUBLIC void mprError(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    fmtv(buf, sizeof(buf), fmt, args);
    va_end(args);
    logOutput(MPR_ERROR_MSG | MPR_ERROR_SRC, 0, buf);
    mprBreakpoint();
}


PUBLIC void mprWarn(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    fmtv(buf, sizeof(buf), fmt, args);
    va_end(args);
    logOutput(MPR_ERROR_MSG | MPR_WARN_SRC, 0, buf);
    mprBreakpoint();
}


PUBLIC void mprMemoryError(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    if (fmt == 0) {
        logOutput(MPR_ERROR_MSG | MPR_ERROR_SRC, 0, "Memory allocation error");
    } else {
        va_start(args, fmt);
        fmtv(buf, sizeof(buf), fmt, args);
        va_end(args);
        logOutput(MPR_ERROR_MSG | MPR_ERROR_SRC, 0, buf);
    }
}


PUBLIC void mprUserError(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    fmtv(buf, sizeof(buf), fmt, args);
    va_end(args);
    logOutput(MPR_USER_MSG | MPR_ERROR_SRC, 0, buf);
}


PUBLIC void mprFatalError(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    fmtv(buf, sizeof(buf), fmt, args);
    va_end(args);
    logOutput(MPR_USER_MSG | MPR_FATAL_SRC, 0, buf);
    exit(2);
}


/*
    Handle an error without allocating memory. Bypasses the logging mechanism.
 */
PUBLIC void mprStaticError(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    fmtv(buf, sizeof(buf), fmt, args);
    va_end(args);
#if BIT_UNIX_LIKE || VXWORKS
    if (write(2, (char*) buf, slen(buf)) < 0) {}
    if (write(2, (char*) "\n", 1) < 0) {}
#elif BIT_WIN_LIKE
    if (fprintf(stderr, "%s\n", buf) < 0) {}
    fflush(stderr);
#endif
    mprBreakpoint();
}


PUBLIC void mprAssure(cchar *loc, cchar *msg)
{
#if BIT_ASSERT
    char    buf[BIT_MAX_LOGLINE];

    if (loc) {
#if BIT_UNIX_LIKE
        snprintf(buf, sizeof(buf), "Assertion %s, failed at %s", msg, loc);
#else
        sprintf(buf, "Assertion %s, failed at %s", msg, loc);
#endif
        msg = buf;
    }
    mprLog(0, "%s", buf);
    mprBreakpoint();
#if WATSON_PAUSE
    printf("Stop for WATSON\n");
    mprNap(60 * 1000);
#endif
#endif
}


/*
    Output a log message to the log handler
 */
static void logOutput(int flags, int level, cchar *msg)
{
    MprLogHandler   handler;

    handler = MPR->logHandler;
    if (handler != 0) {
        (handler)(flags, level, msg);
        return;
    }
    defaultLogHandler(flags, level, msg);
}


static void defaultLogHandler(int flags, int level, cchar *msg)
{
    MprFile     *file;
    MprPath     info;
    char        *prefix, buf[BIT_MAX_LOGLINE];
    int         mode;
    static int  check = 0;

    if ((file = MPR->logFile) == 0) {
        return;
    }
    prefix = MPR->name;

    if (MPR->logBackup > 0 && MPR->logSize && (check++ % 1000) == 0) {
        mprGetPathInfo(MPR->logPath, &info);
        if (info.valid && info.size > MPR->logSize) {
            lock(MPR);
            mprSetLogFile(0);
            mprBackupLog(MPR->logPath, MPR->logBackup);
            mode = O_CREAT | O_WRONLY | O_TEXT;
            if ((file = mprOpenFile(MPR->logPath, mode, 0664)) == 0) {
                mprError("Cannot open log file %s", MPR->logPath);
                unlock(MPR);
                return;
            }
            mprSetLogFile(file);
            unlock(MPR);
        }
    }
    while (*msg == '\n') {
        mprWriteFile(file, "\n", 1);
        msg++;
    }
    if (flags & MPR_LOG_SRC) {
        fmt(buf, sizeof(buf), "%s: %d: %s\n", prefix, level, msg);
        mprWriteFileString(file, buf);

    } else if (flags & (MPR_WARN_SRC | MPR_ERROR_SRC)) {
        if (flags & MPR_WARN_SRC) {
            fmt(buf, sizeof(buf), "%s: Warning: %s\n", prefix, msg);
        } else {
            fmt(buf, sizeof(buf), "%s: Error: %s\n", prefix, msg);
        }
#if BIT_WIN_LIKE || BIT_UNIX_LIKE
        mprWriteToOsLog(buf, flags, level);
#endif
        fmt(buf, sizeof(buf), "%s: Error: %s\n", prefix, msg);
        mprWriteFileString(file, buf);

    } else if (flags & MPR_FATAL_SRC) {
        fmt(buf, sizeof(buf), "%s: Fatal: %s\n", prefix, msg);
        mprWriteToOsLog(buf, flags, level);
        mprWriteFileString(file, buf);
        
    } else if (flags & MPR_RAW) {
        mprWriteFileString(file, msg);
    }
}


/*
    Return the raw O/S error code
 */
PUBLIC int mprGetOsError()
{
#if BIT_WIN_LIKE
    int     rc;
    rc = GetLastError();

    /*
        Client has closed the pipe
     */
    if (rc == ERROR_NO_DATA) {
        return EPIPE;
    }
    return rc;
#elif BIT_UNIX_LIKE || VXWORKS
    return errno;
#else
    return 0;
#endif
}


/*
    Return the mapped (portable, Posix) error code
 */
PUBLIC int mprGetError()
{
#if !BIT_WIN_LIKE
    return mprGetOsError();
#else
    int     err;

    err = mprGetOsError();

    switch (err) {
    case ERROR_SUCCESS:
        return 0;
    case ERROR_FILE_NOT_FOUND:
        return ENOENT;
    case ERROR_ACCESS_DENIED:
        return EPERM;
    case ERROR_INVALID_HANDLE:
        return EBADF;
    case ERROR_NOT_ENOUGH_MEMORY:
        return ENOMEM;
    case ERROR_PATH_BUSY:
    case ERROR_BUSY_DRIVE:
    case ERROR_NETWORK_BUSY:
    case ERROR_PIPE_BUSY:
    case ERROR_BUSY:
        return EBUSY;
    case ERROR_FILE_EXISTS:
        return EEXIST;
    case ERROR_BAD_PATHNAME:
    case ERROR_BAD_ARGUMENTS:
        return EINVAL;
    case WSAENOTSOCK:
        return ENOENT;
    case WSAEINTR:
        return EINTR;
    case WSAEBADF:
        return EBADF;
    case WSAEACCES:
        return EACCES;
    case WSAEINPROGRESS:
        return EINPROGRESS;
    case WSAEALREADY:
        return EALREADY;
    case WSAEADDRINUSE:
        return EADDRINUSE;
    case WSAEADDRNOTAVAIL:
        return EADDRNOTAVAIL;
    case WSAENETDOWN:
        return ENETDOWN;
    case WSAENETUNREACH:
        return ENETUNREACH;
    case WSAECONNABORTED:
        return ECONNABORTED;
    case WSAECONNRESET:
        return ECONNRESET;
    case WSAECONNREFUSED:
        return ECONNREFUSED;
    case WSAEWOULDBLOCK:
        return EAGAIN;
    }
    return MPR_ERR;
#endif
}


PUBLIC int mprGetLogLevel()
{
    Mpr     *mpr;

    /* Leave the code like this so debuggers can patch logLevel before returning */
    mpr = MPR;
    return mpr->logLevel;
}


PUBLIC MprLogHandler mprGetLogHandler()
{
    return MPR->logHandler;
}


PUBLIC int mprUsingDefaultLogHandler()
{
    return MPR->logHandler == defaultLogHandler;
}


PUBLIC MprFile *mprGetLogFile()
{
    return MPR->logFile;
}


PUBLIC void mprSetLogHandler(MprLogHandler handler)
{
    MPR->logHandler = handler;
}


PUBLIC void mprSetLogFile(MprFile *file)
{
    if (file != MPR->logFile && MPR->logFile != MPR->stdOutput && MPR->logFile != MPR->stdError) {
        mprCloseFile(MPR->logFile);
    }
    MPR->logFile = file;
}


PUBLIC void mprSetLogLevel(int level)
{
    MPR->logLevel = level;
}


PUBLIC bool mprSetCmdlineLogging(bool on)
{
    bool    wasLogging;

    wasLogging = MPR->cmdlineLogging;
    MPR->cmdlineLogging = on;
    return wasLogging;
}


PUBLIC bool mprGetCmdlineLogging()
{
    return MPR->cmdlineLogging;
}


#if MACOSX
/*
    Just for conditional breakpoints when debugging in Xcode
 */
PUBLIC int _cmp(char *s1, char *s2)
{
    return !strcmp(s1, s2);
}
#endif

/*
    @copy   default

    Copyright (c) Embedthis Software LLC, 2003-2012. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
