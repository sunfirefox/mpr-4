/** 
    benchMpr.c - Benchmark various MPR facilities
  
    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

/********************************** Locals ************************************/

typedef struct App {
    int      testAllocOnly;     /* Test alloc only  */
    int      iterations;        /* Benchmark iterations */
    int      workers;           /* Number of worker threads */
    MprCond  *complete;         /* Condition set when benchmark complete */
    MprMutex *mutex;            /* Test synchronization */
    int      markCount;         /* Flag set when benchmark complete */
} App;

static App *app;

/***************************** Forward Declarations ***************************/

static void     doBenchmark(void *thread);
static void     endMark(MprTime start, int count, char *msg);
static void     eventCallback(void *data, MprEvent *ep);
static void     manageApp(App *app, int flags);
static MprTime  startMark();
static void     testMalloc();
static void     timerCallback(void *data, MprEvent *ep);
volatile int    testComplete;

/*********************************** Code *************************************/

MAIN(benchMpr, int argc, char **argv, char **envp)
{
    MprThread       *thread;
    Mpr             *mpr;
    char            *argp;
    int             err, nextArg;

    if ((mpr = mprCreate(argc, argv, MPR_USER_EVENTS_THREAD)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if ((app = mprAllocObj(App, manageApp)) == 0) {
        return MPR_ERR_MEMORY;
    }
    mprAddRoot(app);
    app->mutex = mprCreateLock(mpr);
    app->complete = mprCreateCond();
    app->iterations = 5;
    err = 0;

    for (nextArg = 1; nextArg < argc; nextArg++) {
        argp = argv[nextArg];
        if (*argp != '-') {
            break;
        }
        if (strcmp(argp, "--iterations") == 0 || strcmp(argp, "-i") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                app->iterations = atoi(argv[++nextArg]);
            }

        } else if (strcmp(argp, "--alloc") == 0 || strcmp(argp, "-a") == 0) {
            app->testAllocOnly++;
        } else {
            err++;
        }
    }
    if (err) {
        mprPrintf("usage: bench [-a] [-i iterations] [-t workers]\n");
        mprRawLog(0, "usage: %s [options]\n"
            "    -a                  # Alloc test only\n"
            "    --iterations count  # Number of iterations to run the test\n"
            "    --workers count     # Set maximum worker threads\n",
            mprGetAppName(mpr));
        exit(2);
    }
    mprStart(mpr);
    thread = mprCreateThread("bench", (MprThreadProc) doBenchmark, (void*) MPR, 0);
    mprStartThread(thread);
    
    while (!testComplete) {
        mprServiceEvents(250, 0);
    }
    mprPrintMem("Memory Report", 0);
    mprDestroy(0);
    return 0;
}


static void manageApp(App *app, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(app->complete);
        mprMark(app->mutex);
    }
}


/*
    Do a performance benchmark
 */ 
static void doBenchmark(void *thread)
{
    MprTime         start;
    MprList         *list;
    int             count, i;
    MprMutex        *lock;
    MprSpin         *spin;

    mprPrintf("Group\t%-30s\t%13s\t%12s\n", "Benchmark", "Microsec", "Elapsed-sec");

    testMalloc();

    if (!app->testAllocOnly) {
        /*
            Locking primitives
         */
        mprPrintf("Lock Benchmarks\n");
        lock = mprCreateLock();
        count = 5000000 * app->iterations;
        start = startMark();
        for (i = 0; i < count; i++) {
            mprLock(lock);
            mprUnlock(lock);
        }
        endMark(start, count, "Mutex lock|unlock");
        
        /*
            Locking primitives
         */
        mprPrintf("Lock Benchmarks\n");
        spin = mprCreateSpinLock();
        count = 5000000 * app->iterations;
        start = startMark();
        for (i = 0; i < count; i++) {
            mprSpinLock(spin);
            mprSpinUnlock(spin);
        }
        endMark(start, count, "Spin lock|unlock");
        
        /*
            Condition signal / wait
         */
        mprPrintf("Cond Benchmarks\n");
        count = 1000000 * app->iterations;
        start = startMark();
        mprResetCond(app->complete);
        for (i = 0; i < count; i++) {
            mprSignalCond(app->complete);
            mprWaitForCond(app->complete, -1);
        }
        endMark(start, count, "Cond signal|wait");
        
        /*
            List
         */
        mprPrintf("List Benchmarks\n");
        count = 2000000 * app->iterations;
        list = mprCreateList(count, 0);
        start = startMark();
        for (i = 0; i < count; i++) {
            mprAddItem(list, (void*) (long) i);
            mprRemoveItem(list, (void*) (long) i);
        }
        endMark(start, count, "Link insert|remove");

        /*
            Events
         */
        mprPrintf("Event Benchmarks\n");
        mprResetCond(app->complete);
        count = 30000 * app->iterations;
        app->markCount = count;
        start = startMark();
        for (i = 0; i < count; i++) {
            mprCreateEvent(NULL, "eventBenchmark", 0, eventCallback, ITOP(i), MPR_EVENT_QUICK);
        }
        mprWaitForCond(app->complete, -1);
        endMark(start, count, "Event (create|run|delete)");

        /*
            Test timer creation, run and remove
            These create a new dispatcher and run a worker thread.
         */
        mprPrintf("Timer\n");
        mprResetCond(app->complete);
        count = 20000 * app->iterations;
        app->markCount = count;
        start = startMark();
        for (i = 0; i < count; i++) {
            mprCreateTimerEvent(NULL, "timerBenchmark", 0, timerCallback, (void*) (long) i, 0);
        }
        mprWaitForCond(app->complete, -1);
        endMark(start, count, "Timer (create|delete)");

        /*
            Alloc (1K)
         */
        mprPrintf("Alloc 1K Benchmarks\n");
        count = 2000000 * app->iterations;
        start = startMark();
        for (i = 0; i < count; i++) {
            mprAlloc(1024);
            if ((i % 128) == 0) {
                mprRequestGC(0);
            }
        }
        endMark(start, count, "Alloc mprAlloc(1K)");
    }
    testComplete = 1;
}


static void testMalloc()
{
    MprTime     start;
    char        *ptr;
    int         count, i, pin;
#if KEEP
    ssize    base;
#endif

    mprPrintf("Alloc/Malloc overhead\n");
    count = 2000000 * app->iterations;
    pin = 0;
    mprRequestGC(MPR_GC_FORCE);
    
#if MALLOC
    /*
        malloc(1)
     */

    base = mprGetMem();
    start = startMark();
    for (i = 0; i < count; i++) {
        ptr = malloc(1);
        if (pin) memset(ptr, 0, 1);
    }
    endMark(start, count, "Alloc malloc(1)");
    mprPrintf("\tMalloc overhead per block %d\n\n", ((mprGetMem() - base) / count) - 1);

    /*
        malloc(8)
     */
    base = mprGetMem();
    start = startMark();
    for (i = 0; i < count; i++) {
        ptr = malloc(8);
        if (pin) memset(ptr, 0, 8);
    }
    endMark(start, count, "Alloc malloc(8)");
    mprPrintf("\tMalloc overhead per block %d (approx)\n\n", ((mprGetMem() - base) / count) - 8);

    /*
        malloc(16)
     */
    base = mprGetMem();
    start = startMark();
    for (i = 0; i < count; i++) {
        ptr = malloc(16);
        if (pin) memset(ptr, 0, 16);
    }
    endMark(start, count, "Alloc malloc(16)");
    mprPrintf("\tMalloc overhead per block %d (approx)\n\n", ((mprGetMem() - base) / count) - 16);

    /*
        malloc(32)
     */
    base = mprGetMem();
    start = startMark();
    for (i = 0; i < count; i++) {
        ptr = malloc(32);
        if (pin) memset(ptr, 0, 32);
    }
    endMark(start, count, "Alloc malloc(32)");
    mprPrintf("\tMalloc overhead per block %d (approx)\n\n", ((mprGetMem() - base) / count) - 32);

    /*
        malloc+free(8)
     */
    start = startMark();
    for (i = 0; i < count; i++) {
        ptr = malloc(8);
        if (pin) memset(ptr, 0, 8);
        mprNop(ptr);
        free(ptr);
    }
    endMark(start, count, "Alloc malloc+free(8)");
    mprPrintf("\n");
#endif

    /*
        mprAlloc(1)
     */
    // base = mprGetMem();
    start = startMark();
    for (i = 0; i < count; i++) {
        ptr = mprAlloc(1);
        if (pin) memset(ptr, 0, 1);
    }
    endMark(start, count, "Alloc mprAlloc(1)");
    // mprPrintf("\tMpr overhead per block %d (approx)\n\n", ((mprGetMem() - base) / count) - 1);

    /*
        mprAlloc(8)
     */
    // base = mprGetMem();
    start = startMark();
    for (i = 0; i < count; i++) {
        ptr = mprAlloc(8);
        if (pin) memset(ptr, 0, 8);
    }
    endMark(start, count, "Alloc mprAlloc(8)");
    // mprPrintf("\tMpr overhead per block %d (approx)\n\n", ((mprGetMem() - base) / count) - 8);

    /*
        mprAlloc(16)
     */
    // base = mprGetMem();
    start = startMark();
    for (i = 0; i < count; i++) {
        ptr = mprAlloc(16);
        if (pin) memset(ptr, 0, 16);
    }
    endMark(start, count, "Alloc mprAlloc(16)");
    // mprPrintf("\tMpr overhead per block %d (approx)\n\n", ((mprGetMem() - base) / count) - 16);

    /*
        mprAlloc(32)
     */
    // base = mprGetMem();
    start = startMark();
    for (i = 0; i < count; i++) {
        ptr = mprAlloc(32);
        if (pin) memset(ptr, 0, 32);
    }
    endMark(start, count, "Alloc mprAlloc(32)");
    // mprPrintf("\tMpr overhead per block %d (approx)\n\n", ((mprGetMem() - base) / count) - 32);

    /*
        mprAlloc(64)
     */
    // base = mprGetMem();
    start = startMark();
    for (i = 0; i < count; i++) {
        ptr = mprAlloc(64);
        if (pin) memset(ptr, 0, 64);
    }
    endMark(start, count, "Alloc mprAlloc(32)");
    // mprPrintf("\tMpr overhead per block %d (approx)\n\n", ((mprGetMem() - base) / count) - 64);

    mprPrintf("\n");
}


/*
    Event callback 
 */
static void eventCallback(void *data, MprEvent *event)
{
    //  TODO - should have atomic Inc
    mprLock(app->mutex);
    if (--app->markCount == 0) {
        mprSignalCond(app->complete);
    }
    mprRemoveEvent(event);
    mprUnlock(app->mutex);
}


/*
    Timer callback 
 */
static void timerCallback(void *data, MprEvent *event)
{
    mprLock(app->mutex);
    if (--app->markCount == 0) {
        mprSignalCond(app->complete);
    }
    mprRemoveEvent(event);
    mprUnlock(app->mutex);
}


static MprTime startMark()
{
    return mprGetTime();
}


static void endMark(MprTime start, int count, char *msg)
{
    MprTime     elapsed;

    elapsed = mprGetElapsedTime(start);
    mprPrintf("\t%-30s\t%13.2f\t%12.2f\n", msg, elapsed * 1000.0 / count, elapsed / 1000.0);
    mprRequestGC(MPR_GC_FORCE | MPR_GC_COMPLETE);
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
