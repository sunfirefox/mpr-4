/**
    thread.c - Primitive multi-threading support for Windows

    This module provides threading, mutex and condition variable APIs.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes **********************************/

#include    "mpr.h"

/*************************** Forward Declarations ****************************/

static void changeState(MprWorker *worker, int state);
static MprWorker *createWorker(MprWorkerService *ws, ssize stackSize);
static int getNextThreadNum(MprWorkerService *ws);
static void manageThreadService(MprThreadService *ts, int flags);
static void manageThread(MprThread *tp, int flags);
static void manageWorker(MprWorker *worker, int flags);
static void manageWorkerService(MprWorkerService *ws, int flags);
static void pruneWorkers(MprWorkerService *ws, MprEvent *timer);
static void threadProc(MprThread *tp);
static void workerMain(MprWorker *worker, MprThread *tp);

/************************************ Code ***********************************/

PUBLIC MprThreadService *mprCreateThreadService()
{
    MprThreadService    *ts;

    if ((ts = mprAllocObj(MprThreadService, manageThreadService)) == 0) {
        return 0;
    }
    if ((ts->cond = mprCreateCond()) == 0) {
        return 0;
    }
    if ((ts->threads = mprCreateList(-1, 0)) == 0) {
        return 0;
    }
    MPR->mainOsThread = mprGetCurrentOsThread();
    MPR->threadService = ts;
    ts->stackSize = MPR_DEFAULT_STACK;
    /*
        Don't actually create the thread. Just create a thread object for this main thread.
     */
    if ((ts->mainThread = mprCreateThread("main", NULL, NULL, 0)) == 0) {
        return 0;
    }
    ts->mainThread->isMain = 1;
    ts->mainThread->osThread = mprGetCurrentOsThread();
    return ts;
}


PUBLIC void mprStopThreadService()
{
}


static void manageThreadService(MprThreadService *ts, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ts->threads);
        mprMark(ts->mainThread);
        mprMark(ts->cond);

    } else if (flags & MPR_MANAGE_FREE) {
        mprStopThreadService();
    }
}


PUBLIC void mprSetThreadStackSize(ssize size)
{
    MPR->threadService->stackSize = size;
}


PUBLIC MprThread *mprGetCurrentThread()
{
    MprThreadService    *ts;
    MprThread           *tp;
    MprOsThread         id;
    int                 i;

    ts = MPR->threadService;
    id = mprGetCurrentOsThread();
    lock(ts->threads);
    for (i = 0; i < ts->threads->length; i++) {
        tp = mprGetItem(ts->threads, i);
        if (tp->osThread == id) {
            unlock(ts->threads);
            return tp;
        }
    }
    unlock(ts->threads);
    return 0;
}


PUBLIC cchar *mprGetCurrentThreadName()
{
    MprThread       *tp;

    if ((tp = mprGetCurrentThread()) == 0) {
        return 0;
    }
    return tp->name;
}


/*
    Return the current thread object
 */
PUBLIC void mprSetCurrentThreadPriority(int pri)
{
    MprThread       *tp;

    if ((tp = mprGetCurrentThread()) == 0) {
        return;
    }
    mprSetThreadPriority(tp, pri);
}


/*
    Create a main thread
 */
PUBLIC MprThread *mprCreateThread(cchar *name, void *entry, void *data, ssize stackSize)
{
    MprThreadService    *ts;
    MprThread           *tp;

    ts = MPR->threadService;
    tp = mprAllocObj(MprThread, manageThread);
    if (tp == 0) {
        return 0;
    }
    tp->data = data;
    tp->entry = entry;
    tp->name = sclone(name);
    tp->mutex = mprCreateLock();
    tp->cond = mprCreateCond();
    tp->pid = getpid();
    tp->priority = MPR_NORMAL_PRIORITY;

    if (stackSize == 0) {
        tp->stackSize = ts->stackSize;
    } else {
        tp->stackSize = stackSize;
    }
#if BIT_WIN_LIKE
    tp->threadHandle = 0;
#endif
    assert(ts);
    assert(ts->threads);
    if (mprAddItem(ts->threads, tp) < 0) {
        return 0;
    }
    return tp;
}


static void manageThread(MprThread *tp, int flags)
{
    MprThreadService    *ts;

    ts = MPR->threadService;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(tp->name);
        mprMark(tp->data);
        mprMark(tp->cond);
        mprMark(tp->mutex);

    } else if (flags & MPR_MANAGE_FREE) {
        if (ts->threads) {
            mprRemoveItem(ts->threads, tp);
        }
#if BIT_WIN_LIKE
        if (tp->threadHandle) {
            CloseHandle(tp->threadHandle);
        }
#endif
    }
}


/*
    Entry thread function
 */ 
#if BIT_WIN_LIKE
static uint __stdcall threadProcWrapper(void *data) 
{
    threadProc((MprThread*) data);
    return 0;
}
#elif VXWORKS

static int threadProcWrapper(void *data) 
{
    threadProc((MprThread*) data);
    return 0;
}

#else
PUBLIC void *threadProcWrapper(void *data) 
{
    threadProc((MprThread*) data);
    return 0;
}

#endif


/*
    Thread entry
 */
static void threadProc(MprThread *tp)
{
    assert(tp);

    tp->osThread = mprGetCurrentOsThread();

#if VXWORKS
    tp->pid = tp->osThread;
#else
    tp->pid = getpid();
#endif
    (tp->entry)(tp->data, tp);
    mprRemoveItem(MPR->threadService->threads, tp);
}


/*
    Start a thread
 */
PUBLIC int mprStartThread(MprThread *tp)
{
    lock(tp);

#if BIT_WIN_LIKE
{
    HANDLE          h;
    uint            threadId;

#if WINCE
    h = (HANDLE) CreateThread(NULL, 0, threadProcWrapper, (void*) tp, 0, &threadId);
#else
    h = (HANDLE) _beginthreadex(NULL, 0, threadProcWrapper, (void*) tp, 0, &threadId);
#endif
    if (h == NULL) {
        unlock(tp);
        return MPR_ERR_CANT_INITIALIZE;
    }
    tp->osThread = (int) threadId;
    tp->threadHandle = (HANDLE) h;
}
#elif VXWORKS
{
    int     taskHandle, pri;

    taskPriorityGet(taskIdSelf(), &pri);
    taskHandle = taskSpawn(tp->name, pri, VX_FP_TASK, tp->stackSize, (FUNCPTR) threadProcWrapper, (int) tp, 
        0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (taskHandle < 0) {
        mprError("Cannot create thread %s\n", tp->name);
        unlock(tp);
        return MPR_ERR_CANT_INITIALIZE;
    }
}
#else /* UNIX */
{
    pthread_attr_t  attr;
    pthread_t       h;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, tp->stackSize);

    if (pthread_create(&h, &attr, threadProcWrapper, (void*) tp) != 0) { 
        assert(0);
        pthread_attr_destroy(&attr);
        unlock(tp);
        return MPR_ERR_CANT_CREATE;
    }
    pthread_attr_destroy(&attr);
}
#endif
    unlock(tp);
    return 0;
}


PUBLIC MprOsThread mprGetCurrentOsThread()
{
#if BIT_UNIX_LIKE
    return (MprOsThread) pthread_self();
#elif BIT_WIN_LIKE
    return (MprOsThread) GetCurrentThreadId();
#elif VXWORKS
    return (MprOsThread) taskIdSelf();
#endif
}


PUBLIC void mprSetThreadPriority(MprThread *tp, int newPriority)
{
    int     osPri;

    lock(tp);
    osPri = mprMapMprPriorityToOs(newPriority);

#if BIT_WIN_LIKE
    SetThreadPriority(tp->threadHandle, osPri);
#elif VXWORKS
    taskPrioritySet(tp->osThread, osPri);
#else
    setpriority(PRIO_PROCESS, (int) tp->pid, osPri);
#endif
    tp->priority = newPriority;
    unlock(tp);
}


static void manageThreadLocal(MprThreadLocal *tls, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
#if !BIT_UNIX_LIKE && !BIT_WIN_LIKE
        mprMark(tls->store);
#endif
    } else if (flags & MPR_MANAGE_FREE) {
#if BIT_UNIX_LIKE
        if (tls->key) {
            pthread_key_delete(tls->key);
        }
#elif BIT_WIN_LIKE
        if (tls->key >= 0) {
            TlsFree(tls->key);
        }
#endif
    }
}


PUBLIC MprThreadLocal *mprCreateThreadLocal()
{
    MprThreadLocal      *tls;

    if ((tls = mprAllocObj(MprThreadLocal, manageThreadLocal)) == 0) {
        return 0;
    }
#if BIT_UNIX_LIKE
    if (pthread_key_create(&tls->key, NULL) != 0) {
        tls->key = 0;
        return 0;
    }
#elif BIT_WIN_LIKE
    if ((tls->key = TlsAlloc()) < 0) {
        return 0;
    }
#else
    if ((tls->store = mprCreateHash(0, MPR_HASH_STATIC_VALUES)) == 0) {
        return 0;
    }
#endif
    return tls;
}


PUBLIC int mprSetThreadData(MprThreadLocal *tls, void *value)
{
    bool    err;

#if BIT_UNIX_LIKE
    err = pthread_setspecific(tls->key, value) != 0;
#elif BIT_WIN_LIKE
    err = TlsSetValue(tls->key, value) != 0;
#else
    {
        char    key[32];
        itosbuf(key, sizeof(key), (int64) mprGetCurrentOsThread(), 10);
        err = mprAddKey(tls->store, key, value) == 0;
    }
#endif
    return (err) ? MPR_ERR_CANT_WRITE: 0;
}


PUBLIC void *mprGetThreadData(MprThreadLocal *tls)
{
#if BIT_UNIX_LIKE
    return pthread_getspecific(tls->key);
#elif BIT_WIN_LIKE
    return TlsGetValue(tls->key);
#else
    {
        char    key[32];
        itosbuf(key, sizeof(key), (int64) mprGetCurrentOsThread(), 10);
        return mprLookupKey(tls->store, key);
    }
#endif
}


#if BIT_WIN_LIKE
/*
    Map Mpr priority to Windows native priority. Windows priorities range from -15 to +15 (zero is normal). 
    Warning: +15 will not yield the CPU, -15 may get starved. We should be very wary going above +11.
 */

PUBLIC int mprMapMprPriorityToOs(int mprPriority)
{
    assert(mprPriority >= 0 && mprPriority <= 100);
 
    if (mprPriority <= MPR_BACKGROUND_PRIORITY) {
        return THREAD_PRIORITY_LOWEST;
    } else if (mprPriority <= MPR_LOW_PRIORITY) {
        return THREAD_PRIORITY_BELOW_NORMAL;
    } else if (mprPriority <= MPR_NORMAL_PRIORITY) {
        return THREAD_PRIORITY_NORMAL;
    } else if (mprPriority <= MPR_HIGH_PRIORITY) {
        return THREAD_PRIORITY_ABOVE_NORMAL;
    } else {
        return THREAD_PRIORITY_HIGHEST;
    }
}


/*
    Map Windows priority to Mpr priority
 */ 
PUBLIC int mprMapOsPriorityToMpr(int nativePriority)
{
    int     priority;

    priority = (45 * nativePriority) + 50;
    if (priority < 0) {
        priority = 0;
    }
    if (priority >= 100) {
        priority = 99;
    }
    return priority;
}


#elif VXWORKS
/*
    Map MPR priority to VxWorks native priority.
 */

PUBLIC int mprMapMprPriorityToOs(int mprPriority)
{
    int     nativePriority;

    assert(mprPriority >= 0 && mprPriority < 100);

    nativePriority = (100 - mprPriority) * 5 / 2;

    if (nativePriority < 10) {
        nativePriority = 10;
    } else if (nativePriority > 255) {
        nativePriority = 255;
    }
    return nativePriority;
}


/*
    Map O/S priority to Mpr priority.
 */ 
PUBLIC int mprMapOsPriorityToMpr(int nativePriority)
{
    int     priority;

    priority = (255 - nativePriority) * 2 / 5;
    if (priority < 0) {
        priority = 0;
    }
    if (priority >= 100) {
        priority = 99;
    }
    return priority;
}


#else /* UNIX */
/*
    Map MR priority to linux native priority. Unix priorities range from -19 to +19. Linux does -20 to +19. 
 */
PUBLIC int mprMapMprPriorityToOs(int mprPriority)
{
    assert(mprPriority >= 0 && mprPriority < 100);

    if (mprPriority <= MPR_BACKGROUND_PRIORITY) {
        return 19;
    } else if (mprPriority <= MPR_LOW_PRIORITY) {
        return 10;
    } else if (mprPriority <= MPR_NORMAL_PRIORITY) {
        return 0;
    } else if (mprPriority <= MPR_HIGH_PRIORITY) {
        return -8;
    } else {
        return -19;
    }
    assert(0);
    return 0;
}


/*
    Map O/S priority to Mpr priority.
 */ 
PUBLIC int mprMapOsPriorityToMpr(int nativePriority)
{
    int     priority;

    priority = (nativePriority + 19) * (100 / 40); 
    if (priority < 0) {
        priority = 0;
    }
    if (priority >= 100) {
        priority = 99;
    }
    return priority;
}

#endif /* UNIX */


PUBLIC MprWorkerService *mprCreateWorkerService()
{
    MprWorkerService      *ws;

    ws = mprAllocObj(MprWorkerService, manageWorkerService);
    if (ws == 0) {
        return 0;
    }
    ws->mutex = mprCreateLock();
    ws->minThreads = MPR_DEFAULT_MIN_THREADS;
    ws->maxThreads = MPR_DEFAULT_MAX_THREADS;

    /*
        Presize the lists so they cannot get memory allocation failures later on.
     */
    ws->idleThreads = mprCreateList(0, 0);
    mprSetListLimits(ws->idleThreads, ws->maxThreads, -1);
    ws->busyThreads = mprCreateList(0, 0);
    mprSetListLimits(ws->busyThreads, ws->maxThreads, -1);
    return ws;
}


static void manageWorkerService(MprWorkerService *ws, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ws->busyThreads);
        mprMark(ws->idleThreads);
        mprMark(ws->mutex);
        mprMark(ws->pruneTimer);
    }
}


PUBLIC int mprStartWorkerService()
{
    MprWorkerService    *ws;

    /*
        Create a timer to trim excess workers
     */
    ws = MPR->workerService;
    mprSetMinWorkers(ws->minThreads);
    ws->pruneTimer = mprCreateTimerEvent(NULL, "pruneWorkers", MPR_TIMEOUT_PRUNER, pruneWorkers, ws, MPR_EVENT_QUICK);
    return 0;
}


PUBLIC void mprWakeWorkers()
{
    MprWorkerService    *ws;
    MprWorker           *worker;
    int                 next;

    ws = MPR->workerService;
    lock(ws);
    if (ws->pruneTimer) {
        mprRemoveEvent(ws->pruneTimer);
    }
    /*
        Wake up all idle workers. Busy workers take care of themselves. An idle thread will wakeup, exit and be 
        removed from the busy list and then delete the thread. We progressively remove the last thread in the idle
        list. ChangeState will move the workers to the busy queue.
     */
    for (next = -1; (worker = (MprWorker*) mprGetPrevItem(ws->idleThreads, &next)) != 0; ) {
        changeState(worker, MPR_WORKER_BUSY);
    }
    unlock(ws);
}


/*
    Define the new minimum number of workers. Pre-allocate the minimum.
 */
PUBLIC void mprSetMinWorkers(int n)
{ 
    MprWorker           *worker;
    MprWorkerService    *ws;

    ws = MPR->workerService;
    lock(ws);
    ws->minThreads = n; 
    mprTrace(4, "Pre-start %d workers", ws->minThreads);
    
    while (ws->numThreads < ws->minThreads) {
        worker = createWorker(ws, ws->stackSize);
        ws->numThreads++;
        ws->maxUsedThreads = max(ws->numThreads, ws->maxUsedThreads);
        changeState(worker, MPR_WORKER_BUSY);
        mprStartThread(worker->thread);
    }
    unlock(ws);
}


/*
    Define a new maximum number of theads. Prune if currently over the max.
 */
PUBLIC void mprSetMaxWorkers(int n)
{
    MprWorkerService  *ws;

    ws = MPR->workerService;

    lock(ws);
    ws->maxThreads = n; 
    if (ws->numThreads > ws->maxThreads) {
        pruneWorkers(ws, 0);
    }
    if (ws->minThreads > ws->maxThreads) {
        ws->minThreads = ws->maxThreads;
    }
    unlock(ws);
}


PUBLIC int mprGetMaxWorkers()
{
    return MPR->workerService->maxThreads;
}


/*
    Return the current worker thread object
 */
PUBLIC MprWorker *mprGetCurrentWorker()
{
    MprWorkerService    *ws;
    MprWorker           *worker;
    MprThread           *thread;
    int                 next;

    ws = MPR->workerService;

    lock(ws);
    thread = mprGetCurrentThread();
    for (next = -1; (worker = (MprWorker*) mprGetPrevItem(ws->busyThreads, &next)) != 0; ) {
        if (worker->thread == thread) {
            unlock(ws);
            return worker;
        }
    }
    unlock(ws);
    return 0;
}


PUBLIC void mprActivateWorker(MprWorker *worker, MprWorkerProc proc, void *data)
{
    MprWorkerService    *ws;

    ws = worker->workerService;

    lock(ws);
    worker->proc = proc;
    worker->data = data;
    changeState(worker, MPR_WORKER_BUSY);
    unlock(ws);
}


PUBLIC void mprSetWorkerStartCallback(MprWorkerProc start)
{
    MPR->workerService->startWorker = start;
}


PUBLIC void mprGetWorkerStats(MprWorkerStats *stats)
{
    MprWorkerService    *ws;
    MprWorker           *wp;
    int                 next;

    ws = MPR->workerService;

    lock(ws);
    stats->max = ws->maxThreads;
    stats->min = ws->minThreads;
    stats->maxUsed = ws->maxUsedThreads;

    stats->idle = (int) ws->idleThreads->length;
    stats->busy = (int) ws->busyThreads->length;

    stats->yielded = 0;
    for (ITERATE_ITEMS(ws->busyThreads, wp, next)) {
        /*
            Only count as yielded, those workers who call yield from their user code
            This excludes the yield in worker main
         */
        stats->yielded += (wp->thread->yielded && wp->running);
    }
    unlock(ws);
}


PUBLIC int mprAvailableWorkers()
{
    MprWorkerStats  wstats;
    int             activeWorkers, spareThreads, spareCores, result;

    mprGetWorkerStats(&wstats);
    spareThreads = wstats.max - wstats.busy - wstats.idle;
    activeWorkers = wstats.busy - wstats.yielded;
    spareCores = MPR->heap->stats.numCpu - activeWorkers;
    if (spareCores <= 0 || spareThreads <= 0) {
        return 0;
    }
    result = wstats.idle + min(spareThreads, spareCores);
#if KEEP
    printf("Avail %d, busy %d, yielded %d, idle %d, spare-threads %d, spare-cores %d, mustYield %d\n", result, wstats.busy,
        wstats.yielded, wstats.idle, spareThreads, spareCores, MPR->heap->mustYield);
#endif
    return result;
}


PUBLIC int mprStartWorker(MprWorkerProc proc, void *data)
{
    MprWorkerService    *ws;
    MprWorker           *worker;

    ws = MPR->workerService;
    lock(ws);

    /*
        Try to find an idle thread and wake it up. It will wakeup in workerMain(). If not any available, then add 
        another thread to the worker. Must account for workers we've already created but have not yet gone to work 
        and inserted themselves in the idle/busy queues. Get most recently used idle worker so we tend to reuse 
        active threads. This lets the pruner trim idle workers.
     */
    worker = mprGetLastItem(ws->idleThreads);
    if (worker) {
        worker->data = data;
        worker->proc = proc;
        changeState(worker, MPR_WORKER_BUSY);

    } else if (ws->numThreads < ws->maxThreads) {
        if (mprAvailableWorkers() == 0) {
            unlock(ws);
            return MPR_ERR_BUSY;
        }
        worker = createWorker(ws, ws->stackSize);
        ws->numThreads++;
        ws->maxUsedThreads = max(ws->numThreads, ws->maxUsedThreads);
        worker->data = data;
        worker->proc = proc;
        changeState(worker, MPR_WORKER_BUSY);
        mprStartThread(worker->thread);

    } else {
        unlock(ws);
        return MPR_ERR_BUSY;
    }
    unlock(ws);
    return 0;
}


/*
    Trim idle workers
 */
static void pruneWorkers(MprWorkerService *ws, MprEvent *timer)
{
    MprWorker     *worker;
    int           index, pruned;

    if (mprGetDebugMode()) {
        return;
    }
    lock(ws);
    pruned = 0;
    for (index = 0; index < ws->idleThreads->length; index++) {
        if (ws->numThreads <= ws->minThreads) {
            break;
        }
        worker = mprGetItem(ws->idleThreads, index);
        if ((worker->lastActivity + MPR_TIMEOUT_WORKER) < MPR->eventService->now) {
            changeState(worker, MPR_WORKER_PRUNED);
            pruned++;
            index--;
        }
    }
    if (pruned) {
        mprLog(2, "Pruned %d workers, pool has %d workers. Limits %d-%d.", 
            pruned, ws->numThreads - pruned, ws->minThreads, ws->maxThreads);
    }
    unlock(ws);
}


static int getNextThreadNum(MprWorkerService *ws)
{
    int     rc;

    lock(ws);
    rc = ws->nextThreadNum++;
    unlock(ws);
    return rc;
}


/*
    Define a new stack size for new workers. Existing workers unaffected.
 */
PUBLIC void mprSetWorkerStackSize(int n)
{
    MPR->workerService->stackSize = n; 
}


/*
    Create a new thread for the task
 */
static MprWorker *createWorker(MprWorkerService *ws, ssize stackSize)
{
    MprWorker   *worker;

    char    name[16];

    if ((worker = mprAllocObj(MprWorker, manageWorker)) == 0) {
        return 0;
    }
    worker->workerService = ws;
    worker->idleCond = mprCreateCond();

    fmt(name, sizeof(name), "worker.%u", getNextThreadNum(ws));
    mprLog(2, "Create %s, pool has %d workers. Limits %d-%d.", name, ws->numThreads + 1, ws->minThreads, ws->maxThreads);
    worker->thread = mprCreateThread(name, (MprThreadProc) workerMain, worker, stackSize);
    return worker;
}


static void manageWorker(MprWorker *worker, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(worker->data);
        mprMark(worker->thread);
        mprMark(worker->workerService);
        mprMark(worker->idleCond);
    }
}


static void workerMain(MprWorker *worker, MprThread *tp)
{
    MprWorkerService    *ws;

    ws = MPR->workerService;
    assert(worker->state == MPR_WORKER_BUSY);
    assert(!worker->idleCond->triggered);

    if (ws->startWorker) {
        (*ws->startWorker)(worker->data, worker);
    }
    /*
        Very important for performance to elimminate to locking the WorkerService
     */
    while (!(worker->state & MPR_WORKER_PRUNED)) {
        if (worker->proc) {
            worker->running = 1;
            (*worker->proc)(worker->data, worker);
            worker->running = 0;
        }
        worker->lastActivity = MPR->eventService->now;
        if (mprIsStopping()) {
            break;
        }
        assert(worker->cleanup == 0);
        if (worker->cleanup) {
            (*worker->cleanup)(worker->data, worker);
            worker->cleanup = NULL;
        }
        worker->proc = 0;
        worker->data = 0;
        changeState(worker, MPR_WORKER_IDLE);

        /*
            Sleep till there is more work to do. Yield for GC first.
         */
        mprYield(MPR_YIELD_STICKY | MPR_YIELD_NO_BLOCK);
        mprWaitForCond(worker->idleCond, -1);
        mprResetYield();
    }
    lock(ws);
    changeState(worker, 0);
    worker->thread = 0;
    ws->numThreads--;
    unlock(ws);
    mprLog(4, "Worker exiting. There are %d workers remaining in the pool.", ws->numThreads);
}


static void changeState(MprWorker *worker, int state)
{
    MprWorkerService    *ws;
    MprList             *lp;
    int                 wake;

    if (state == worker->state) {
        return;
    }
    wake = 0;
    lp = 0;
    ws = worker->workerService;
    lock(ws);

    switch (worker->state) {
    case MPR_WORKER_BUSY:
        lp = ws->busyThreads;
        break;

    case MPR_WORKER_IDLE:
        lp = ws->idleThreads;
        wake = 1;
        break;
        
    case MPR_WORKER_PRUNED:
        break;
    }

    /*
        Reassign the worker to the appropriate queue
     */
    if (lp) {
        mprRemoveItem(lp, worker);
    }
    lp = 0;
    switch (state) {
    case MPR_WORKER_BUSY:
        lp = ws->busyThreads;
        break;

    case MPR_WORKER_IDLE:
        lp = ws->idleThreads;
        mprWakePendingDispatchers();
        break;

    case MPR_WORKER_PRUNED:
        /* Don't put on a queue and the thread will exit */
        mprWakePendingDispatchers();
        break;
    }
    worker->state = state;

    if (lp) {
        if (mprAddItem(lp, worker) < 0) {
            unlock(ws);
            assert(!MPR_ERR_MEMORY);
            return;
        }
    }
    unlock(ws);
    if (wake) {
        mprSignalCond(worker->idleCond); 
    }
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
