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
    char        *levelSpec, *path;
    int         level;

    level = -1;
    file = 0;
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
#if !BIT_ROM
        } else {
            MprPath     info;
            int         mode;
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
#endif
        }
        if (level >= 0) {
            mprSetLogLevel(level);
        }
        if (file) {
            mprSetLogFile(file);
        }
        if (showConfig) {
            mprLogHeader();
        }
    }
    return 0;
}


PUBLIC void mprLogHeader()
{
    mprLog(MPR_INFO, "Configuration for %s", mprGetAppTitle());
    mprLog(MPR_INFO, "---------------------------------------------");
    mprLog(MPR_INFO, "Version:            %s-%s", BIT_VERSION, BIT_BUILD_NUMBER);
    mprLog(MPR_INFO, "BuildType:          %s", BIT_DEBUG ? "Debug" : "Release");
    mprLog(MPR_INFO, "CPU:                %s", BIT_CPU);
    mprLog(MPR_INFO, "OS:                 %s", BIT_OS);
    mprLog(MPR_INFO, "Host:               %s", mprGetHostName());
    mprLog(MPR_INFO, "Directory:          %s", mprGetCurrentPath());
    mprLog(MPR_INFO, "Configure:          %s", BIT_CONFIG_CMD);
    mprLog(MPR_INFO, "---------------------------------------------");
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


PUBLIC void mprTraceProc(int level, cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    logOutput(MPR_LOG_MSG, level, fmtv(buf, sizeof(buf), fmt, args));
    va_end(args);
}


PUBLIC void mprLogProc(int level, cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    logOutput(MPR_LOG_MSG, level, fmtv(buf, sizeof(buf), fmt, args));
    va_end(args);
}


/*
    Warning: this will allocate
 */
PUBLIC void mprRawLog(int level, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    logOutput(MPR_RAW_MSG, level, sfmtv(fmt, args));
    va_end(args);
}


PUBLIC void mprError(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    logOutput(MPR_ERROR_MSG, MPR_ERROR, fmtv(buf, sizeof(buf), fmt, args));
    va_end(args);
    mprBreakpoint();
}


PUBLIC void mprInfo(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    logOutput(MPR_INFO_MSG, MPR_INFO, fmtv(buf, sizeof(buf), fmt, args));
    va_end(args);
    mprBreakpoint();
}


PUBLIC void mprWarn(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    logOutput(MPR_WARN_MSG, 0, fmtv(buf, sizeof(buf), fmt, args));
    va_end(args);
    mprBreakpoint();
}


#if DEPRECATED
PUBLIC void mprMemoryError(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    if (fmt == 0) {
        logOutput(MPR_ERROR_MSG, 0, "Memory allocation error");
    } else {
        va_start(args, fmt);
        fmtv(buf, sizeof(buf), fmt, args);
        va_end(args);
        logOutput(MPR_ERROR_MSG, MPR_WARN, buf);
    }
}


PUBLIC void mprUserError(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    fmtv(buf, sizeof(buf), fmt, args);
    va_end(args);
    logOutput(MPR_USER_MSG | MPR_ERROR_MSG, 0, buf);
}


PUBLIC void mprFatalError(cchar *fmt, ...)
{
    va_list     args;
    char        buf[BIT_MAX_LOGLINE];

    va_start(args, fmt);
    fmtv(buf, sizeof(buf), fmt, args);
    va_end(args);
    logOutput(MPR_USER_MSG | MPR_FATAL_MSG, 0, buf);
    exit(2);
}


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
#endif /* DEPRECATED */


PUBLIC void mprAssert(cchar *loc, cchar *msg)
{
#if BIT_MPR_TRACING
    char    buf[BIT_MAX_LOGLINE];

    if (loc) {
#if BIT_UNIX_LIKE
        snprintf(buf, sizeof(buf), "Assertion %s, failed at %s", msg, loc);
#else
        sprintf(buf, "Assertion %s, failed at %s", msg, loc);
#endif
        msg = buf;
    }
    mprTrace(0, "%s", buf);
    mprBreakpoint();
#if BIT_DEBUG_WATSON
    fprintf(stderr, "Pause for debugger to attach\n");
    mprSleep(24 * 3600 * 1000);
#endif
#endif
}


/*
    Output a log message to the log handler
 */
static void logOutput(int flags, int level, cchar *msg)
{
    MprLogHandler   handler;

    if (level < 0 || level > mprGetLogLevel()) {
        return;
    }
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
    char        *prefix, buf[BIT_MAX_LOGLINE], *tag;

    if ((file = MPR->logFile) == 0) {
        return;
    }
    prefix = MPR->name;

#if !BIT_ROM
    {
        static int  check = 0;
        MprPath     info;
        int         mode;
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
    }
#endif
    while (*msg == '\n') {
        mprWriteFile(file, "\n", 1);
        msg++;
    }
    if (flags & MPR_LOG_MSG) {
        fmt(buf, sizeof(buf), "%s: %d: %s\n", prefix, level, msg);
        mprWriteFileString(file, buf);

    } else if (flags & MPR_RAW_MSG) {
        mprWriteFileString(file, msg);

    } else {
        if (flags & MPR_FATAL_MSG) {
            tag = "Fatal";
        } else if (flags & MPR_WARN_MSG) {
           tag = "Warning";
        } else {
           tag = "Error";
        }
        fmt(buf, sizeof(buf), "%s: %s: %s\n", prefix, tag, msg);
        mprWriteToOsLog(buf, flags, level);
        fmt(buf, sizeof(buf), "%s: Error: %s\n", prefix, msg);
        mprWriteFileString(file, buf);
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


PUBLIC void mprSetOsError(int error)
{
#if BIT_WIN_LIKE
    SetLastError(error);
#elif BIT_UNIX_LIKE || VXWORKS
    errno = error;
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

    Copyright (c) Embedthis Software LLC, 2003-2013. All Rights Reserved.

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
