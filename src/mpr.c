/*
    mpr.c - Multithreaded Portable Runtime (MPR). Initialization, start/stop and control of the MPR.

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

/**************************** Forward Declarations ****************************/

static void manageMpr(Mpr *mpr, int flags);
static void serviceEventsThread(void *data, MprThread *tp);

/************************************* Code ***********************************/
/*
    Create and initialize the MPR service.
 */
PUBLIC Mpr *mprCreate(int argc, char **argv, int flags)
{
    MprFileSystem   *fs;
    Mpr             *mpr;

    srand((uint) time(NULL));

    mprAtomicOpen();
    if ((mpr = mprCreateMemService((MprManager) manageMpr, flags)) == 0) {
        assert(mpr);
        return 0;
    }
    mpr->start = mprGetTime(); 
    mpr->exitStrategy = MPR_EXIT_NORMAL;
    mpr->emptyString = sclone("");
    mpr->oneString = sclone("1");
    mpr->exitTimeout = MPR_TIMEOUT_STOP;
    mpr->title = sclone(BIT_TITLE);
    mpr->version = sclone(BIT_VERSION);
    mpr->idleCallback = mprServicesAreIdle;
    mpr->mimeTypes = mprCreateMimeTypes(NULL);
    mpr->terminators = mprCreateList(0, MPR_LIST_STATIC_VALUES);

    mprCreateTimeService();
    mprCreateOsService();
    mpr->mutex = mprCreateLock();
    mpr->spin = mprCreateSpinLock();
    mpr->verifySsl = 1;

    fs = mprCreateFileSystem("/");
    mprAddFileSystem(fs);
    mprCreateLogService();

    if (argv) {
#if BIT_WIN_LIKE
        if (argc >= 2 && strstr(argv[1], "--cygroot") != 0) {
            /*
                Cygwin shebang is broken. It will catenate args into argv[1]
             */
            char *args, *arg0;
            int  i;
            args = argv[1];
            for (i = 2; i < argc; i++) {
                args = sjoin(args, " ", argv[i], NULL);
            }
            arg0 = argv[0];
            argc = mprMakeArgv(args, &mpr->argBuf, MPR_ARGV_ARGS_ONLY);
            argv = mpr->argBuf;
            argv[0] = arg0;
            mpr->argv = (cchar**) argv;
        } else {
            mpr->argv = mprAllocZeroed(sizeof(void*) * (argc + 1));
            memcpy((char*) mpr->argv, argv, sizeof(void*) * argc);
        }
#else
        mpr->argv = mprAllocZeroed(sizeof(void*) * (argc + 1));
        memcpy((char*) mpr->argv, argv, sizeof(void*) * argc);
#endif
        mpr->argc = argc;
        if (!mprIsPathAbs(mpr->argv[0])) {
            mpr->argv[0] = mprGetAppPath();
        } else {
            mpr->argv[0] = sclone(mprGetAppPath());
        }
    } else {
        mpr->name = sclone(BIT_PRODUCT);
        mpr->argv = mprAllocZeroed(2 * sizeof(void*));
        mpr->argv[0] = mpr->name;
        mpr->argc = 0;
    }
    mpr->name = mprTrimPathExt(mprGetPathBase(mpr->argv[0]));

    mpr->signalService = mprCreateSignalService();
    mpr->threadService = mprCreateThreadService();
    mpr->moduleService = mprCreateModuleService();
    mpr->eventService = mprCreateEventService();
    mpr->cmdService = mprCreateCmdService();
    mpr->workerService = mprCreateWorkerService();
    mpr->waitService = mprCreateWaitService();
    mpr->socketService = mprCreateSocketService();

    mpr->dispatcher = mprCreateDispatcher("main", 0);
    mpr->nonBlock = mprCreateDispatcher("nonblock", 0);
    mprSetDispatcherImmediate(mpr->nonBlock);

    mpr->pathEnv = sclone(getenv("PATH"));

    if (flags & MPR_USER_EVENTS_THREAD) {
        if (!(flags & MPR_NO_WINDOW)) {
            mprInitWindow();
        }
    } else {
        mprRunDispatcher(mpr->dispatcher);
        mprStartEventsThread();
    }
    mprStartGCService();

    if (MPR->hasError || mprHasMemError()) {
        return 0;
    }
    return mpr;
}


static void manageMpr(Mpr *mpr, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(mpr->logPath);
        mprMark(mpr->logFile);
        mprMark(mpr->mimeTypes);
        mprMark(mpr->timeTokens);
        mprMark(mpr->pathEnv);
        mprMark(mpr->name);
        mprMark(mpr->title);
        mprMark(mpr->version);
        mprMark(mpr->domainName);
        mprMark(mpr->hostName);
        mprMark(mpr->ip);
        mprMark(mpr->stdError);
        mprMark(mpr->stdInput);
        mprMark(mpr->stdOutput);
        mprMark(mpr->serverName);
        mprMark(mpr->appPath);
        mprMark(mpr->appDir);
        mprMark(mpr->cmdService);
        mprMark(mpr->eventService);
        mprMark(mpr->fileSystem);
        mprMark(mpr->moduleService);
        mprMark(mpr->osService);
        mprMark(mpr->signalService);
        mprMark(mpr->socketService);
        mprMark(mpr->threadService);
        mprMark(mpr->workerService);
        mprMark(mpr->waitService);
        mprMark(mpr->dispatcher);
        mprMark(mpr->nonBlock);
        mprMark(mpr->appwebService);
        mprMark(mpr->ediService);
        mprMark(mpr->ejsService);
        mprMark(mpr->espService);
        mprMark(mpr->httpService);
        mprMark(mpr->testService);
        mprMark(mpr->terminators);
        mprMark(mpr->mutex);
        mprMark(mpr->spin);
        mprMark(mpr->cond);
        mprMark(mpr->emptyString);
        mprMark(mpr->oneString);
        mprMark(mpr->argv);
        mprMark(mpr->argv[0]);
    }
}


/*
    Destroy the Mpr and all services
 */
PUBLIC void mprDestroy(int how)
{
    if (!(how & MPR_EXIT_DEFAULT)) {
        MPR->exitStrategy = how;
    }
    how = MPR->exitStrategy;
    if (how & MPR_EXIT_IMMEDIATE) {
        if (how & MPR_EXIT_RESTART) {
            mprRestart();
            /* No return */
            return;
        }
        exit(0);
    }
    mprYield(MPR_YIELD_STICKY);
    if (MPR->state < MPR_STOPPING) {
        mprTerminate(how, -1);
    }
    mprRequestGC(MPR_GC_FORCE | MPR_GC_COMPLETE);

    if (how & MPR_EXIT_GRACEFUL) {
        mprWaitTillIdle(MPR->exitTimeout);
    }
    MPR->state = MPR_STOPPING_CORE;
    MPR->exitStrategy &= MPR_EXIT_GRACEFUL;
    MPR->exitStrategy |= MPR_EXIT_IMMEDIATE;

    mprStopWorkers();
    mprStopCmdService();
    mprStopModuleService();
    mprStopEventService();
    mprStopSignalService();

    /* Final GC to run all finalizers */
    mprRequestGC(MPR_GC_FORCE | MPR_GC_COMPLETE);

    if (how & MPR_EXIT_RESTART) {
        mprLog(2, "Restarting\n\n");
    } else {
        mprLog(2, "Exiting");
    }
    MPR->state = MPR_FINISHED;
    mprStopGCService();
    mprStopThreadService();
    mprStopOsService();

    if (how & MPR_EXIT_RESTART) {
        mprRestart();
    }
}


/*
    Start termination of the Mpr. May be called by mprDestroy or elsewhere.
 */
PUBLIC void mprTerminate(int how, int status)
{
    MprTerminator   terminator;
    int             next;

    /*
        Set the stopping flag. Services should stop accepting new requests. Current requests should be allowed to
        complete if graceful exit strategy.
     */
    if (MPR->state >= MPR_STOPPING) {
        /* Already stopping and done the code below */
        return;
    }
    MPR->state = MPR_STOPPING;

    MPR->exitStatus = status;
    if (!(how & MPR_EXIT_DEFAULT)) {
        MPR->exitStrategy = how;
    }
    how = MPR->exitStrategy;
    if (how & MPR_EXIT_IMMEDIATE) {
        mprLog(3, "Immediate exit. Terminate all requests and services.");
        exit(status);

    } else if (how & MPR_EXIT_NORMAL) {
        mprLog(3, "Normal exit.");

    } else if (how & MPR_EXIT_GRACEFUL) {
        mprLog(3, "Graceful exit. Waiting for existing requests to complete.");

    } else {
        mprTrace(7, "mprTerminate: how %d", how);
    }
    /*
        Invoke terminators, set stopping state and wake up everybody
        Must invoke terminators before setting stopping state. Otherwise, the main app event loop will return from
        mprServiceEvents and starting calling destroy before we have completed this routine.
     */
    for (ITERATE_ITEMS(MPR->terminators, terminator, next)) {
        (terminator)(how, status);
    }
    mprStopWorkers();
    mprWakeDispatchers();
    mprWakeEventService();
}


PUBLIC int mprGetExitStatus()
{
    return MPR->exitStatus;
}


PUBLIC void mprAddTerminator(MprTerminator terminator)
{
    mprAddItem(MPR->terminators, terminator);
}


PUBLIC void mprRestart()
{
#if BIT_UNIX_LIKE
    int     i;

    for (i = 3; i < MPR_MAX_FILE; i++) {
        close(i);
    }
    execv(MPR->argv[0], (char*const*) MPR->argv);

    /*
        Last-ditch trace. Can only use stdout. Logging may be closed.
     */
    printf("Failed to exec errno %d: ", errno);
    for (i = 0; MPR->argv[i]; i++) {
        printf("%s ", MPR->argv[i]);
    }
    printf("\n");
#else
    mprError("mprRestart not supported on this platform");
#endif
}


PUBLIC int mprStart()
{
    int     rc;

    rc = mprStartOsService();
    rc += mprStartModuleService();
    rc += mprStartWorkerService();
    if (rc != 0) {
        mprError("Cannot start MPR services");
        return MPR_ERR_CANT_INITIALIZE;
    }
    MPR->state = MPR_STARTED;
    mprLog(3, "MPR services are ready");
    return 0;
}


PUBLIC int mprStartEventsThread()
{
    MprThread   *tp;

    if ((tp = mprCreateThread("events", serviceEventsThread, NULL, 0)) == 0) {
        MPR->hasError = 1;
    } else {
        MPR->threadService->eventsThread = tp;
        MPR->cond = mprCreateCond();
        mprStartThread(tp);
        mprWaitForCond(MPR->cond, MPR_TIMEOUT_START_TASK);
    }
    return 0;
}


static void serviceEventsThread(void *data, MprThread *tp)
{
    mprLog(MPR_INFO, "Service thread started");
    if (!(MPR->flags & MPR_NO_WINDOW)) {
        mprInitWindow();
    }
    mprSignalCond(MPR->cond);
    mprServiceEvents(-1, 0);
}


/*
    Services should call this to determine if they should accept new services
 */
PUBLIC bool mprShouldAbortRequests()
{
    return (mprIsStopping() && !(MPR->exitStrategy & MPR_EXIT_GRACEFUL));
}


PUBLIC bool mprShouldDenyNewRequests()
{
    return mprIsStopping();
}


PUBLIC bool mprIsStopping()
{
    return MPR->state >= MPR_STOPPING;
}


PUBLIC bool mprIsStoppingCore()
{
    return MPR->state >= MPR_STOPPING_CORE;
}


PUBLIC bool mprIsFinished()
{
    return MPR->state >= MPR_FINISHED;
}


PUBLIC int mprWaitTillIdle(MprTicks timeout)
{
    MprTicks    mark, remaining, lastTrace;

    lastTrace = mark = mprGetTicks(); 
    while (!mprIsIdle() && (remaining = mprGetRemainingTicks(mark, timeout)) > 0) {
        mprSleep(1);
        mprServiceEvents(10, MPR_SERVICE_ONE_THING);
        if ((lastTrace - remaining) > MPR_TICKS_PER_SEC) {
            mprLog(1, "Waiting for requests to complete, %d secs remaining ...", remaining / MPR_TICKS_PER_SEC);
            lastTrace = remaining;
        }
    }
    return mprIsIdle();
}


/*
    Test if the Mpr services are idle. Use mprIsIdle to determine if the entire process is idle.
 */
PUBLIC bool mprServicesAreIdle()
{
    bool    idle;

    /*
        Only test top level services. Dispatchers may have timers scheduled, but that is okay.
        If not, users can install their own idleCallback.
     */
    idle = mprGetListLength(MPR->workerService->busyThreads) == 0 && mprGetListLength(MPR->cmdService->cmds) == 0;
    if (!idle) {
        mprTrace(6, "Not idle: cmds %d, busy threads %d, eventing %d",
            mprGetListLength(MPR->cmdService->cmds), mprGetListLength(MPR->workerService->busyThreads), MPR->eventing);
    }
    return idle;
}


PUBLIC bool mprIsIdle()
{
    return (MPR->idleCallback)();
}


/*
    Parse the args and return the count of args. If argv is NULL, the args are parsed read-only. If argv is set,
    then the args will be extracted, back-quotes removed and argv will be set to point to all the args.
    NOTE: this routine does not allocate.
 */
PUBLIC int mprParseArgs(char *args, char **argv, int maxArgc)
{
    char    *dest, *src, *start;
    int     quote, argc;

    /*
        Example     "showColors" red 'light blue' "yellow white" 'Cannot \"render\"'
        Becomes:    ["showColors", "red", "light blue", "yellow white", "Cannot \"render\""]
     */
    for (argc = 0, src = args; src && *src != '\0' && argc < maxArgc; argc++) {
        while (isspace((uchar) *src)) {
            src++;
        }
        if (*src == '\0')  {
            break;
        }
        start = dest = src;
        if (*src == '"' || *src == '\'') {
            quote = *src;
            src++; 
            dest++;
        } else {
            quote = 0;
        }
        if (argv) {
            argv[argc] = src;
        }
        while (*src) {
            if (*src == '\\' && src[1] && (src[1] == '\\' || src[1] == '"' || src[1] == '\'')) { 
                src++;
            } else {
                if (quote) {
                    if (*src == quote && !(src > start && src[-1] == '\\')) {
                        break;
                    }
                } else if (*src == ' ') {
                    break;
                }
            }
            if (argv) {
                *dest++ = *src;
            }
            src++;
        }
        if (*src != '\0') {
            src++;
        }
        if (argv) {
            *dest++ = '\0';
        }
    }
    return argc;
}


/*
    Make an argv array. All args are in a single memory block of which argv points to the start.
    Set MPR_ARGV_ARGS_ONLY if not passing in a program name. 
    Always returns and argv[0] reserved for the program name or empty string.  First arg starts at argv[1].
 */
PUBLIC int mprMakeArgv(cchar *command, cchar ***argvp, int flags)
{
    char    **argv, *vector, *args;
    ssize   len;
    int     argc;

    assert(command);

    /*
        Allocate one vector for argv and the actual args themselves
     */
    len = slen(command) + 1;
    argc = mprParseArgs((char*) command, NULL, INT_MAX);
    if (flags & MPR_ARGV_ARGS_ONLY) {
        argc++;
    }
    if ((vector = (char*) mprAlloc(((argc + 1) * sizeof(char*)) + len)) == 0) {
        assert(!MPR_ERR_MEMORY);
        return MPR_ERR_MEMORY;
    }
    args = &vector[(argc + 1) * sizeof(char*)];
    strcpy(args, command);
    argv = (char**) vector;

    if (flags & MPR_ARGV_ARGS_ONLY) {
        mprParseArgs(args, &argv[1], argc);
        argv[0] = MPR->emptyString;
    } else {
        mprParseArgs(args, argv, argc);
    }
    argv[argc] = 0;
    *argvp = (cchar**) argv;
    return argc;
}


PUBLIC MprIdleCallback mprSetIdleCallback(MprIdleCallback idleCallback)
{
    MprIdleCallback old;

    old = MPR->idleCallback;
    MPR->idleCallback = idleCallback;
    return old;
}


PUBLIC int mprSetAppName(cchar *name, cchar *title, cchar *version)
{
    char    *cp;

    if (name) {
        if ((MPR->name = (char*) mprGetPathBase(name)) == 0) {
            return MPR_ERR_CANT_ALLOCATE;
        }
        if ((cp = strrchr(MPR->name, '.')) != 0) {
            *cp = '\0';
        }
    }
    if (title) {
        if ((MPR->title = sclone(title)) == 0) {
            return MPR_ERR_CANT_ALLOCATE;
        }
    }
    if (version) {
        if ((MPR->version = sclone(version)) == 0) {
            return MPR_ERR_CANT_ALLOCATE;
        }
    }
    return 0;
}


PUBLIC cchar *mprGetAppName()
{
    return MPR->name;
}


PUBLIC cchar *mprGetAppTitle()
{
    return MPR->title;
}


/*
    Full host name with domain. E.g. "server.domain.com"
 */
PUBLIC void mprSetHostName(cchar *s)
{
    MPR->hostName = sclone(s);
}


/*
    Return the fully qualified host name
 */
PUBLIC cchar *mprGetHostName()
{
    return MPR->hostName;
}


/*
    Server name portion (no domain name)
 */
PUBLIC void mprSetServerName(cchar *s)
{
    MPR->serverName = sclone(s);
}


PUBLIC cchar *mprGetServerName()
{
    return MPR->serverName;
}


PUBLIC void mprSetDomainName(cchar *s)
{
    MPR->domainName = sclone(s);
}


PUBLIC cchar *mprGetDomainName()
{
    return MPR->domainName;
}


/*
    Set the IP address
 */
PUBLIC void mprSetIpAddr(cchar *s)
{
    MPR->ip = sclone(s);
}


/*
    Return the IP address
 */
PUBLIC cchar *mprGetIpAddr()
{
    return MPR->ip;
}


PUBLIC cchar *mprGetAppVersion()
{
    return MPR->version;
}


PUBLIC bool mprGetDebugMode()
{
    return MPR->debugMode;
}


PUBLIC void mprSetDebugMode(bool on)
{
    MPR->debugMode = on;
}


PUBLIC MprDispatcher *mprGetDispatcher()
{
    return MPR->dispatcher;
}


PUBLIC MprDispatcher *mprGetNonBlockDispatcher()
{
    return MPR->nonBlock;
}


PUBLIC cchar *mprCopyright()
{
    return  "Copyright (c) Embedthis Software LLC, 2003-2013. All Rights Reserved.\n"
            "Copyright (c) Michael O'Brien, 1993-2013. All Rights Reserved.";
}


PUBLIC int mprGetEndian()
{
    char    *probe;
    int     test;

    test = 1;
    probe = (char*) &test;
    return (*probe == 1) ? BIT_LITTLE_ENDIAN : BIT_BIG_ENDIAN;
}


PUBLIC char *mprEmptyString()
{
    return MPR->emptyString;
}


PUBLIC void mprSetExitStrategy(int strategy)
{
    MPR->exitStrategy = strategy;
}


PUBLIC void mprSetEnv(cchar *key, cchar *value)
{
#if !WINCE
#if BIT_UNIX_LIKE
    setenv(key, value, 1);
#else
    char *cmd = sjoin(key, "=", value, NULL);
    putenv(cmd);
#endif
#endif
    if (scaselessmatch(key, "PATH")) {
        MPR->pathEnv = sclone(value);
    }
}


PUBLIC void mprSetExitTimeout(MprTicks timeout)
{
    MPR->exitTimeout = timeout;
}


PUBLIC void mprNop(void *ptr) {
}

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
