/*
    dispatcher.c - Event dispatch services

    This module is thread-safe.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

/***************************** Forward Declarations ***************************/

static void dequeueDispatcher(MprDispatcher *dispatcher);
static int dispatchEvents(MprDispatcher *dispatcher);
static MprTicks getDispatcherIdleTicks(MprDispatcher *dispatcher, MprTicks timeout);
static MprTicks getIdleTicks(MprEventService *es, MprTicks timeout);
static MprDispatcher *getNextReadyDispatcher(MprEventService *es);
static void initDispatcher(MprDispatcher *q);
static int makeRunnable(MprDispatcher *dispatcher);
static void manageDispatcher(MprDispatcher *dispatcher, int flags);
static void manageEventService(MprEventService *es, int flags);
static void queueDispatcher(MprDispatcher *prior, MprDispatcher *dispatcher);
static void scheduleDispatcher(MprDispatcher *dispatcher);
static void serviceDispatcherMain(MprDispatcher *dispatcher);
static bool serviceDispatcher(MprDispatcher *dp);

#define isRunning(dispatcher) (dispatcher->parent == dispatcher->service->runQ)
#define isReady(dispatcher) (dispatcher->parent == dispatcher->service->readyQ)
#define isWaiting(dispatcher) (dispatcher->parent == dispatcher->service->waitQ)
#define isEmpty(dispatcher) (dispatcher->eventQ->next == dispatcher->eventQ)
#if KEEP
static int dqlen(MprDispatcher *dq);
#endif

/************************************* Code ***********************************/
/*
    Create the overall dispatch service. There may be many event dispatchers.
 */
PUBLIC MprEventService *mprCreateEventService()
{
    MprEventService     *es;

    if ((es = mprAllocObj(MprEventService, manageEventService)) == 0) {
        return 0;
    }
    MPR->eventService = es;
    es->now = mprGetTicks();
    es->mutex = mprCreateLock();
    es->waitCond = mprCreateCond();
    es->runQ = mprCreateDispatcher("running", 0);
    es->readyQ = mprCreateDispatcher("ready", 0);
    es->idleQ = mprCreateDispatcher("idle", 0);
    es->pendingQ = mprCreateDispatcher("pending", 0);
    es->waitQ = mprCreateDispatcher("waiting", 0);
    return es;
}


static void manageEventService(MprEventService *es, int flags)
{
    MprDispatcher   *dp;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(es->runQ);
        mprMark(es->readyQ);
        mprMark(es->waitQ);
        mprMark(es->idleQ);
        mprMark(es->pendingQ);
        mprMark(es->waitCond);
        mprMark(es->mutex);

        for (dp = es->runQ->next; dp != es->runQ; dp = dp->next) {
            mprMark(dp);
        }
        for (dp = es->readyQ->next; dp != es->readyQ; dp = dp->next) {
            mprMark(dp);
        }
        for (dp = es->waitQ->next; dp != es->waitQ; dp = dp->next) {
            mprMark(dp);
        }
        for (dp = es->idleQ->next; dp != es->idleQ; dp = dp->next) {
            mprMark(dp);
        }
        for (dp = es->pendingQ->next; dp != es->pendingQ; dp = dp->next) {
            mprMark(dp);
        }

    } else if (flags & MPR_MANAGE_FREE) {
        /* Needed for race with manageDispatcher */
        es->mutex = 0;
    }
}


PUBLIC void mprStopEventService()
{
    mprWakeDispatchers();
    mprWakeNotifier();
}


/*
    Create a disabled dispatcher. A dispatcher schedules events on a single dispatch queue.
 */
PUBLIC MprDispatcher *mprCreateDispatcher(cchar *name, int flags)
{
    MprEventService     *es;
    MprDispatcher       *dispatcher;

    if ((dispatcher = mprAllocObj(MprDispatcher, manageDispatcher)) == 0) {
        return 0;
    }
    dispatcher->name = sclone(name);
    dispatcher->cond = mprCreateCond();
    dispatcher->flags = flags;
    dispatcher->magic = MPR_DISPATCHER_MAGIC;
    es = dispatcher->service = MPR->eventService;
    dispatcher->eventQ = mprCreateEventQueue();
    dispatcher->currentQ = mprCreateEventQueue();
    if (flags & MPR_DISPATCHER_ENABLED) {
        queueDispatcher(es->idleQ, dispatcher);
    } else {
        initDispatcher(dispatcher);
    }
    return dispatcher;
}


static void mprDestroyDispatcher(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *q, *event, *next;

    if (dispatcher && !(dispatcher->flags & MPR_DISPATCHER_DESTROYED)) {
        es = dispatcher->service;
        assert(es == MPR->eventService);
        lock(es);
        assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
        assert(dispatcher->service == MPR->eventService);
        assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
        q = dispatcher->eventQ;
        for (event = q->next; event != q; event = next) {
            next = event->next;
            if (event->dispatcher) {
                mprRemoveEvent(event);
            }
        }
        dequeueDispatcher(dispatcher);
        assert(dispatcher->parent == dispatcher);

        dispatcher->flags = MPR_DISPATCHER_DESTROYED;
        dispatcher->owner = 0;
        dispatcher->magic = MPR_DISPATCHER_FREE;
        unlock(es);
    }
}


static void manageDispatcher(MprDispatcher *dispatcher, int flags)
{
    MprEventService     *es;
    MprEvent            *q, *event, *next;

    es = dispatcher->service;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(dispatcher->name);
        mprMark(dispatcher->eventQ);
        mprMark(dispatcher->currentQ);
        mprMark(dispatcher->cond);
        mprMark(dispatcher->parent);
        mprMark(dispatcher->service);
        mprMark(dispatcher->requiredWorker);

        //  TODO - is this lock needed?  Surely all threads are stopped.
        lock(es);
        q = dispatcher->eventQ;
        for (event = q->next; event != q; event = next) {
            next = event->next;
            mprMark(event);
        }
        q = dispatcher->currentQ;
        for (event = q->next; event != q; event = next) {
            next = event->next;
            mprMark(event);
        }
        unlock(es);

    } else if (flags & MPR_MANAGE_FREE) {
        mprDestroyDispatcher(dispatcher);
    }
}


PUBLIC void mprDisableDispatcher(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *q, *event, *next;

    if (dispatcher && (dispatcher->flags & MPR_DISPATCHER_ENABLED)) {
        es = dispatcher->service;
        lock(es);
        assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
        assert(dispatcher->service == MPR->eventService);
        assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
        q = dispatcher->eventQ;
        for (event = q->next; event != q; event = next) {
            next = event->next;
            if (event->dispatcher) {
                mprRemoveEvent(event);
            }
        }
        dequeueDispatcher(dispatcher);
        assert(dispatcher->parent == dispatcher);
        dispatcher->flags &= ~MPR_DISPATCHER_ENABLED;
        unlock(es);
    }
}


PUBLIC void mprEnableDispatcher(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    int                 mustWake;

    if (dispatcher == 0) {
        dispatcher = MPR->dispatcher;
    }
    es = dispatcher->service;
    mustWake = 0;
    lock(es);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
    if (!(dispatcher->flags & MPR_DISPATCHER_ENABLED)) {
        dispatcher->flags |= MPR_DISPATCHER_ENABLED;
        mprTrace(7, "mprEnableDispatcher: %s", dispatcher->name);
        if (!isEmpty(dispatcher) && !isReady(dispatcher) && !isRunning(dispatcher)) {
            queueDispatcher(es->readyQ, dispatcher);
            if (es->waiting) {
                mustWake = 1;
            }
        }
    }
    unlock(es);
    if (mustWake) {
        mprWakeEventService();
    }
}


/*
    Schedule events. This can be called by any thread. Typically an app will dedicate one thread to be an event service 
    thread. This call will service events until the timeout expires or if MPR_SERVICE_ONE_THING is specified in flags, 
    after one event. This will service all enabled dispatcher queues and pending I/O events.
    @param dispatcher Primary dispatcher to service. This dispatcher is set to the running state and events on this
        dispatcher will be serviced without starting a worker thread. This can be set to NULL.
    @param timeout Time in milliseconds to wait. Set to zero for no wait. Set to -1 to wait forever.
    @returns Zero if not events occurred. Otherwise returns non-zero.
 */
PUBLIC int mprServiceEvents(MprTicks timeout, int flags)
{
    MprEventService     *es;
    MprDispatcher       *dp;
    MprTicks            expires, delay;
    int                 beginEventCount, eventCount, justOne;

    if (MPR->eventing) {
        mprError("mprServiceEvents() called reentrantly");
        return 0;
    }
    MPR->eventing = 1;
    mprInitWindow();
    es = MPR->eventService;
    beginEventCount = eventCount = es->eventCount;

    es->now = mprGetTicks();
    expires = timeout < 0 ? MAXINT64 : (es->now + timeout);
    if (expires < 0) {
        expires = MAXINT64;
    }
    justOne = (flags & MPR_SERVICE_ONE_THING) ? 1 : 0;

    /*
        Stop serviceing events when doing final shutdown of the core
        Post-test for mprIsStopping so callers can pump remaining events once stopping has begun
     */
    while (es->now < expires) {
        eventCount = es->eventCount;
        if (MPR->signalService->hasSignals) {
            mprServiceSignals();
        }
        while ((dp = getNextReadyDispatcher(es)) != NULL) {
            assert(!(dp->flags & MPR_DISPATCHER_DESTROYED));
            assert(dp->magic == MPR_DISPATCHER_MAGIC);
            if (!serviceDispatcher(dp)) {
                queueDispatcher(es->pendingQ, dp);
                es->pendingCount++;
                continue;
            }
            if (justOne) {
                MPR->eventing = 0;
                return abs(es->eventCount - beginEventCount);
            }
        } 
        if (es->eventCount == eventCount) {
            /*
                No events serviced
             */
            lock(es);
            delay = getIdleTicks(es, expires - es->now);
            if (delay > 0) {
                if (mprIsStopping()) {
                    delay = 10;
                }
                es->willAwake = es->now + delay;
                es->waiting = 1;
                unlock(es);

                mprWaitForIO(MPR->waitService, delay);
            } else {
                unlock(es);
            }
        }
        es->now = mprGetTicks();
        if (justOne || mprIsStopping()) {
            break;
        }
    }
    MPR->eventing = 0;
    return abs(es->eventCount - beginEventCount);
}


PUBLIC void mprClearWaiting()
{
    MPR->eventService->waiting = 0;
}


PUBLIC void mprWakeEventService()
{
    if (MPR->eventService->waiting) {
        mprWakeNotifier();
    }
}


/*
    Wait for an event to occur and dispatch the event. This is not called by mprServiceEvents.
    Return Return 0 if an event was signalled. Return MPR_ERR_TIMEOUT if no event was seen before the timeout.
    WARNING: this will enable GC while sleeping
 */
PUBLIC int mprWaitForEvent(MprDispatcher *dispatcher, MprTicks timeout)
{
    MprEventService     *es;
    MprTicks            expires, delay;
    MprOsThread         thread;
    int                 claimed, signalled, wasRunning, runEvents;

    assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));

    es = MPR->eventService;
    es->now = mprGetTicks();

    if (dispatcher == NULL) {
        dispatcher = MPR->dispatcher;
    }
    assert(!(dispatcher->flags & MPR_DISPATCHER_WAITING));
    if (dispatcher->flags & MPR_DISPATCHER_WAITING) {
        return MPR_ERR_BUSY;
    }
    thread = mprGetCurrentOsThread();
    expires = timeout < 0 ? (es->now + MPR_MAX_TIMEOUT) : (es->now + timeout);
    claimed = signalled = 0;

    lock(es);
    /*
        Acquire dedicates the dispatcher to this thread. If acquire fails, another thread is servicing this dispatcher.
        makeRunnable() prevents mprServiceEvents from servicing this dispatcher
     */
    wasRunning = isRunning(dispatcher);
    runEvents = (!wasRunning || dispatcher->owner == thread);
    if (runEvents) {
        if (!wasRunning) {
            makeRunnable(dispatcher);
        }
        dispatcher->owner = thread;
    }
    unlock(es);

    while (es->now <= expires && !mprIsStoppingCore()) {
        assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
        if (runEvents) {
            makeRunnable(dispatcher);
            if (dispatchEvents(dispatcher)) {
                signalled++;
                break;
            }
        }
        lock(es);
        delay = getDispatcherIdleTicks(dispatcher, expires - es->now);
        dispatcher->flags |= MPR_DISPATCHER_WAITING;
        assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
        unlock(es);

        assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
        mprYield(MPR_YIELD_STICKY | MPR_YIELD_NO_BLOCK);
        assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);

        if (mprWaitForCond(dispatcher->cond, delay) == 0) {
            assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
            mprResetYield();
            dispatcher->flags &= ~MPR_DISPATCHER_WAITING;
            if (runEvents) {
                makeRunnable(dispatcher);
                dispatchEvents(dispatcher);
            }
            assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
            signalled++;
            break;
        }
        mprResetYield();
        assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
        dispatcher->flags &= ~MPR_DISPATCHER_WAITING;
        es->now = mprGetTicks();
    }
    if (!wasRunning) {
        scheduleDispatcher(dispatcher);
        if (claimed) {
            dispatcher->owner = 0;
        }
    }
    assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
    return signalled ? 0 : MPR_ERR_TIMEOUT;
}


PUBLIC void mprWakeDispatchers()
{
    MprEventService     *es;
    MprDispatcher       *runQ, *dp;

    es = MPR->eventService;
    lock(es);
    runQ = es->runQ;
    for (dp = runQ->next; dp != runQ; dp = dp->next) {
        mprSignalCond(dp->cond);
    }
    unlock(es);
}


PUBLIC int mprDispatchersAreIdle()
{
    MprEventService     *es;
    MprDispatcher       *runQ, *dispatcher;
    int                 idle;

    es = MPR->eventService;
    runQ = es->runQ;
    lock(es);
    dispatcher = runQ->next;
    if (dispatcher == runQ) {
        idle = 1;
    } else {
        idle = (dispatcher->eventQ == dispatcher->eventQ->next);
    }
    unlock(es);
    return idle;
}


/*
    Relay an event to a dispatcher. This invokes the callback proc as though it was invoked from the given dispatcher. 
 */
PUBLIC void mprRelayEvent(MprDispatcher *dispatcher, void *proc, void *data, MprEvent *event)
{
#if BIT_DEBUG
    MprThread   *tp = mprGetCurrentThread();
    mprNop(tp);
#endif
    assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));

    if (isRunning(dispatcher) && dispatcher->owner != mprGetCurrentOsThread()) {
        mprError("Relay to a running dispatcher owned by another thread");
    }
    if (event) {
        event->timestamp = dispatcher->service->now;
    }
    dispatcher->owner = mprGetCurrentOsThread();
    makeRunnable(dispatcher);
    ((MprEventProc) proc)(data, event);

    /*
        The event may have disabled the dispatcher. Don't reschedule if disabled
     */
    assert(dispatcher->flags & MPR_DISPATCHER_ENABLED);
    if (dispatcher->flags & MPR_DISPATCHER_ENABLED) {
        scheduleDispatcher(dispatcher);
        dispatcher->owner = 0;
    }
}


/*
    Schedule the dispatcher. If the dispatcher is already running then it is not modified. If the event queue is empty, 
    the dispatcher is moved to the idleQ. If there is a past-due event, it is moved to the readyQ. If there is a future 
    event pending, it is put on the waitQ.
 */
PUBLIC void mprScheduleDispatcher(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *event;
    int                 mustWakeWaitService, mustWakeCond;

    assert(dispatcher);
    assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
    assert(dispatcher->name);
    assert(dispatcher->cond);
    es = dispatcher->service;

    lock(es);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));

    if (isRunning(dispatcher) || !(dispatcher->flags & MPR_DISPATCHER_ENABLED)) {
        /* Wake up if waiting in mprWaitForIO */
        mustWakeWaitService = es->waiting;
        mustWakeCond = dispatcher->flags & MPR_DISPATCHER_WAITING;

    } else {
        if (isEmpty(dispatcher)) {
            queueDispatcher(es->idleQ, dispatcher);
            unlock(es);
            return;
        }
        event = dispatcher->eventQ->next;
        mustWakeWaitService = mustWakeCond = 0;
        if (event->due > es->now) {
            assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
            queueDispatcher(es->waitQ, dispatcher);
            if (event->due < es->willAwake) {
                mustWakeWaitService = 1;
                mustWakeCond = dispatcher->flags & MPR_DISPATCHER_WAITING;
            }
        } else {
            queueDispatcher(es->readyQ, dispatcher);
            mustWakeWaitService = es->waiting;
            mustWakeCond = dispatcher->flags & MPR_DISPATCHER_WAITING;
        }
    }
    unlock(es);
    if (mustWakeCond) {
        mprSignalDispatcher(dispatcher);
    }
    if (mustWakeWaitService) {
        mprWakeEventService();
    }
}


/*
    Dispatch events for a dispatcher
 */
static int dispatchEvents(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *event;
    int                 count;

    assert(dispatcher->cond);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));

    es = dispatcher->service;
    /*
#### MOB OPT - mprGetNextEvent locks anyway, so should be able to get away without a lock here
     */
    mprTrace(7, "dispatchEvents for %s", dispatcher->name);
    lock(es);
    assert(dispatcher->cond);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
    assert(dispatcher->flags & MPR_DISPATCHER_ENABLED);
    /*
        Events are removed from the dispatcher queue and put onto the currentQ. This is so they will be marked for GC.
        If the callback calls mprRemoveEvent, it will not remove from the currentQ. If it was a continuous event, 
        mprRemoveEvent will clear the continuous flag.

        OPT - this could all be simpler if dispatchEvents was never called recursively. Then a currentQ would not be needed,
        and neither would a running flag. See mprRemoveEvent().
     */
    for (count = 0; (dispatcher->flags & MPR_DISPATCHER_ENABLED) && (event = mprGetNextEvent(dispatcher)) != 0; count++) {
        assert(!(event->flags & MPR_EVENT_RUNNING));
        unlock(es);

        mprTrace(7, "Call event %s", event->name);
        assert(event->proc);
        event->flags |= MPR_EVENT_RUNNING;
        (event->proc)(event->data, event);
        event->flags &= ~MPR_EVENT_RUNNING;

        lock(es);
        if (event->flags & MPR_EVENT_CONTINUOUS) {
            /* Reschedule if continuous */
            event->timestamp = dispatcher->service->now;
            event->due = event->timestamp + (event->period ? event->period : 1);
            mprQueueEvent(dispatcher, event);
        } else {
            /* Remove from currentQ - GC can then collect */
//### MOB - need locking inside this routine
            mprDequeueEvent(event);
        }
    }
    es->eventCount += count;
    unlock(es);
#if UNUSED && MOB
    if (count && es->waiting) {
        es->eventCount += count;
        mprWakeEventService();
    }
#endif
    return count;
}


static bool serviceDispatcher(MprDispatcher *dispatcher)
{
    assert(isRunning(dispatcher));
    assert(dispatcher->owner == 0);
    assert(dispatcher->cond);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
    dispatcher->owner = mprGetCurrentOsThread();

    if (dispatcher == MPR->nonBlock) {
        serviceDispatcherMain(dispatcher);

    } else if (dispatcher->requiredWorker) {
        mprActivateWorker(dispatcher->requiredWorker, (MprWorkerProc) serviceDispatcherMain, dispatcher);

    } else if (mprStartWorker((MprWorkerProc) serviceDispatcherMain, dispatcher) < 0) {
        return 0;
    }
    return 1;
}


static void serviceDispatcherMain(MprDispatcher *dispatcher)
{
    MprEventService     *es;

    assert(dispatcher->parent);
    es = dispatcher->service;
    lock(es);
    if (!(dispatcher->flags & MPR_DISPATCHER_ENABLED) || (dispatcher->flags & MPR_DISPATCHER_DESTROYED)) {
        /* Dispatcher may have been disabled after starting the worker */
        unlock(es);
        return;
    }
    unlock(es);
    assert(isRunning(dispatcher));
    assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
    assert(dispatcher->cond);
    assert(dispatcher->name);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
    assert(dispatcher->parent);

    dispatcher->owner = mprGetCurrentOsThread();
    dispatchEvents(dispatcher);
    /*
        The dispatcher may be disabled in an event above
     */
    if (dispatcher->flags & MPR_DISPATCHER_ENABLED) {
        dispatcher->owner = 0;
        scheduleDispatcher(dispatcher);
    }
}


PUBLIC void mprClaimDispatcher(MprDispatcher *dispatcher)
{
    assert(isRunning(dispatcher));
    dispatcher->owner = mprGetCurrentOsThread();
}


PUBLIC void mprWakePendingDispatchers()
{
    MprEventService *es;
    int             mustWake;

    es = MPR->eventService;
    lock(es);
    mustWake = es->pendingQ->next != es->pendingQ;
    unlock(es);
    if (mustWake) {
        mprWakeEventService();
    }
}


/*
    Get the next (ready) dispatcher off given runQ and move onto the runQ
 */
static MprDispatcher *getNextReadyDispatcher(MprEventService *es)
{
    MprDispatcher   *dp, *next, *pendingQ, *readyQ, *waitQ, *dispatcher;
    MprEvent        *event;

    waitQ = es->waitQ;
    readyQ = es->readyQ;
    pendingQ = es->pendingQ;
    dispatcher = 0;

    lock(es);
    if (pendingQ->next != pendingQ && mprAvailableWorkers() > 0) {
        dispatcher = pendingQ->next;
        dispatcher->service->pendingCount--;
        assert(dispatcher->service->pendingCount >= 0);
        assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
        queueDispatcher(es->runQ, dispatcher);
        assert(dispatcher->flags & MPR_DISPATCHER_ENABLED);
        dispatcher->owner = 0;

    } else if (readyQ->next == readyQ) {
        /*
            ReadyQ is empty, try to transfer a dispatcher with due events onto the readyQ
         */
        for (dp = waitQ->next; dp != waitQ; dp = next) {
            assert(dp->magic == MPR_DISPATCHER_MAGIC);
            assert(!(dp->flags & MPR_DISPATCHER_DESTROYED));
            next = dp->next;
            event = dp->eventQ->next;
            if (event->due <= es->now && dp->flags & MPR_DISPATCHER_ENABLED) {
                queueDispatcher(es->readyQ, dp);
                break;
            }
        }
    }
    if (!dispatcher && readyQ->next != readyQ) {
        dispatcher = readyQ->next;
        assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
        queueDispatcher(es->runQ, dispatcher);
        assert(dispatcher->flags & MPR_DISPATCHER_ENABLED);
        dispatcher->owner = 0;
    }
    unlock(es);
    assert(dispatcher == NULL || isRunning(dispatcher));
    assert(dispatcher == NULL || dispatcher->magic == MPR_DISPATCHER_MAGIC);
    assert(dispatcher == NULL || !(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
    assert(dispatcher == NULL || dispatcher->cond);
    return dispatcher;
}


/*
    Get the time to sleep till the next pending event. Must be called locked.
 */
static MprTicks getIdleTicks(MprEventService *es, MprTicks timeout)
{
    MprDispatcher   *readyQ, *waitQ, *dp;
    MprEvent        *event;
    MprTicks        delay;

    waitQ = es->waitQ;
    readyQ = es->readyQ;

    if (readyQ->next != readyQ) {
        delay = 0;
    } else if (mprIsStopping()) {
        delay = 10;
    } else {
        /*
            Examine all the dispatchers on the waitQ
         */
        delay = es->delay ? es->delay : MPR_MAX_TIMEOUT;
        for (dp = waitQ->next; dp != waitQ; dp = dp->next) {
            assert(dp->magic == MPR_DISPATCHER_MAGIC);
            assert(!(dp->flags & MPR_DISPATCHER_DESTROYED));
            assert(dp->flags & MPR_DISPATCHER_ENABLED);
            event = dp->eventQ->next;
            if (event != dp->eventQ) {
                delay = min(delay, (event->due - es->now));
                if (delay <= 0) {
                    break;
                }
            }
        }
        delay = min(delay, timeout);
        es->delay = 0;
    }
    return delay;
}


PUBLIC void mprSetEventServiceSleep(MprTicks delay)
{
    MPR->eventService->delay = delay;
}


static MprTicks getDispatcherIdleTicks(MprDispatcher *dispatcher, MprTicks timeout)
{
    MprEvent    *next;
    MprTicks    delay;

    assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);

    if (timeout < 0) {
        timeout = 0;
    } else {
        next = dispatcher->eventQ->next;
        delay = MPR_MAX_TIMEOUT;
        if (next != dispatcher->eventQ) {
            delay = (next->due - dispatcher->service->now);
            if (delay < 0) {
                delay = 0;
            }
        }
        timeout = min(delay, timeout);
    }
    return timeout;
}


static void initDispatcher(MprDispatcher *dispatcher)
{
    assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
    assert(!(dispatcher->flags == MPR_DISPATCHER_DESTROYED));

    dispatcher->next = dispatcher;
    dispatcher->prev = dispatcher;
    dispatcher->parent = dispatcher;
}


static void queueDispatcher(MprDispatcher *prior, MprDispatcher *dispatcher)
{
    assert(dispatcher->service == MPR->eventService);
    lock(dispatcher->service);

    assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
    assert(dispatcher->flags & MPR_DISPATCHER_ENABLED);

    if (dispatcher->parent) {
        dequeueDispatcher(dispatcher);
    }
    dispatcher->parent = prior->parent;
    dispatcher->prev = prior;
    dispatcher->next = prior->next;
    prior->next->prev = dispatcher;
    prior->next = dispatcher;
    assert(dispatcher->cond);
    unlock(dispatcher->service);
}


static void dequeueDispatcher(MprDispatcher *dispatcher)
{
    assert(dispatcher->service == MPR->eventService);
    lock(dispatcher->service);

    assert(dispatcher->magic == MPR_DISPATCHER_MAGIC);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));

    if (dispatcher->next) {
        dispatcher->next->prev = dispatcher->prev;
        dispatcher->prev->next = dispatcher->next;
        dispatcher->next = dispatcher;
        dispatcher->prev = dispatcher;
        dispatcher->parent = dispatcher;
    } else {
        assert(dispatcher->parent == dispatcher);
        assert(dispatcher->next == dispatcher);
        assert(dispatcher->prev == dispatcher);
    }
    assert(dispatcher->cond);
    unlock(dispatcher->service);
}


static void scheduleDispatcher(MprDispatcher *dispatcher)
{
    MprEventService     *es;

    assert(dispatcher->service == MPR->eventService);
    es = dispatcher->service;

    lock(es);
    assert(dispatcher->cond);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
    dequeueDispatcher(dispatcher);
    mprScheduleDispatcher(dispatcher);
    unlock(es);
}


static int makeRunnable(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    int                 wasRunning;

    es = dispatcher->service;

    lock(es);
    assert(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
    wasRunning = isRunning(dispatcher);
    if (!isRunning(dispatcher)) {
        queueDispatcher(es->runQ, dispatcher);
    }
    unlock(es);
    return wasRunning;
}


#if KEEP
static int dqlen(MprDispatcher *dq)
{
    MprDispatcher   *dp;
    int             count;

    count = 0;
    for (dp = dq->next; dp != dq; dp = dp->next) {
        count++;
    }
    return count;
}
#endif


#if KEEP
/*
    Designate the required worker thread to run the event
 */
PUBLIC void mprDedicateWorkerToDispatcher(MprDispatcher *dispatcher, MprWorker *worker)
{
    dispatcher->requiredWorker = worker;
    mprDedicateWorker(worker);
}


PUBLIC void mprReleaseWorkerFromDispatcher(MprDispatcher *dispatcher, MprWorker *worker)
{
    dispatcher->requiredWorker = 0;
    mprReleaseWorker(worker);
}
#endif


PUBLIC void mprSignalDispatcher(MprDispatcher *dispatcher)
{
    if (dispatcher == NULL) {
        dispatcher = MPR->dispatcher;
    }
    mprSignalCond(dispatcher->cond);
}


PUBLIC bool mprDispatcherHasEvents(MprDispatcher *dispatcher)
{
    if (dispatcher == 0) {
        return 0;
    }
    return !isEmpty(dispatcher);
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
