/*
    dispatcher.c - Event dispatch services

    This module is thread-safe.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

/***************************** Forward Declarations ***************************/

static MprDispatcher *createQhead(cchar *name);
static void dequeueDispatcher(MprDispatcher *dispatcher);
static int dispatchEvents(MprDispatcher *dispatcher);
static void dispatchEventsWorker(MprDispatcher *dispatcher);
static MprTicks getDispatcherIdleTicks(MprDispatcher *dispatcher, MprTicks timeout);
static MprTicks getIdleTicks(MprEventService *es, MprTicks timeout);
static MprDispatcher *getNextReadyDispatcher(MprEventService *es);
static void initDispatcher(MprDispatcher *q);
static void manageDispatcher(MprDispatcher *dispatcher, int flags);
static void manageEventService(MprEventService *es, int flags);
static void queueDispatcher(MprDispatcher *prior, MprDispatcher *dispatcher);

#define isRunning(dispatcher) (dispatcher->parent == dispatcher->service->runQ)
#define isReady(dispatcher) (dispatcher->parent == dispatcher->service->readyQ)
#define isWaiting(dispatcher) (dispatcher->parent == dispatcher->service->waitQ)
#define isEmpty(dispatcher) (dispatcher->eventQ->next == dispatcher->eventQ)

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
    es->runQ = createQhead("running");
    es->readyQ = createQhead("ready");
    es->idleQ = createQhead("idle");
    es->pendingQ = createQhead("pending");
    es->waitQ = createQhead("waiting");
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


PUBLIC void mprSetDispatcherImmediate(MprDispatcher *dispatcher)
{
    dispatcher->flags |= MPR_DISPATCHER_IMMEDIATE;
}


PUBLIC void mprStopEventService()
{
    mprWakeDispatchers();
    mprWakeNotifier();
}


static MprDispatcher *createQhead(cchar *name)
{
    MprDispatcher       *dispatcher;

    if ((dispatcher = mprAllocObj(MprDispatcher, manageDispatcher)) == 0) {
        return 0;
    }
    dispatcher->service = MPR->eventService;
    dispatcher->name = sclone(name);
    initDispatcher(dispatcher);
    return dispatcher;
}


PUBLIC MprDispatcher *mprCreateDispatcher(cchar *name, int flags)
{
    MprEventService     *es;
    MprDispatcher       *dispatcher;

    es = MPR->eventService;
    if ((dispatcher = mprAllocObj(MprDispatcher, manageDispatcher)) == 0) {
        return 0;
    }
    dispatcher->flags = flags;
    dispatcher->service = es;
    dispatcher->name = sclone(name);
    dispatcher->cond = mprCreateCond();
    dispatcher->eventQ = mprCreateEventQueue();
    dispatcher->currentQ = mprCreateEventQueue();
    queueDispatcher(es->idleQ, dispatcher);
    return dispatcher;
}


PUBLIC void mprDestroyDispatcher(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *q, *event, *next;

    if (dispatcher) {
        es = dispatcher->service;
        assert(es == MPR->eventService);
        lock(es);
        assert(dispatcher->service == MPR->eventService);
        q = dispatcher->eventQ;
        if (q) {
            for (event = q->next; event != q; event = next) {
                next = event->next;
                if (event->dispatcher) {
                    mprRemoveEvent(event);
                }
            }
        }
        dequeueDispatcher(dispatcher);
        dispatcher->owner = 0;
        dispatcher->flags = MPR_DISPATCHER_DESTROYED;
        unlock(es);
    }
}


static void manageDispatcher(MprDispatcher *dispatcher, int flags)
{
    MprEvent        *q, *event, *next;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(dispatcher->name);
        mprMark(dispatcher->eventQ);
        mprMark(dispatcher->currentQ);
        mprMark(dispatcher->cond);
        mprMark(dispatcher->parent);
        mprMark(dispatcher->service);

        if ((q = dispatcher->eventQ) != 0) {
            for (event = q->next; event != q; event = next) {
                next = event->next;
                mprMark(event);
            }
        }
        if ((q = dispatcher->currentQ) != 0) {
            for (event = q->next; event != q; event = next) {
                next = event->next;
                mprMark(event);
            }
        }

    } else if (flags & MPR_MANAGE_FREE) {
        mprDestroyDispatcher(dispatcher);
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

    while (es->now < expires) {
        eventCount = es->eventCount;
        mprServiceSignals();

        while ((dp = getNextReadyDispatcher(es)) != NULL) {
            queueDispatcher(es->runQ, dp);
            if (dp->flags & MPR_DISPATCHER_IMMEDIATE) {
                dispatchEventsWorker(dp);

            } else if (mprStartWorker((MprWorkerProc) dispatchEventsWorker, dp) < 0) {
                queueDispatcher(es->pendingQ, dp);
                continue;
            }
            if (justOne) {
                expires = 0;
                break;
            }
        } 
        if (es->eventCount == eventCount) {
            lock(es);
            delay = getIdleTicks(es, expires - es->now);
            if (delay > 0) {
                es->willAwake = es->now + delay;
                es->waiting = 1;
                unlock(es);
                /*
                    Wait for something to happen
                 */
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


/*
    Wait for an event to occur and dispatch the event. This is not called by mprServiceEvents.
    Return 0 if an event was signalled. Return MPR_ERR_TIMEOUT if no event was seen before the timeout.
    WARNING: this will enable GC while sleeping.
 */
PUBLIC int mprWaitForEvent(MprDispatcher *dispatcher, MprTicks timeout)
{
    MprEventService     *es;
    MprTicks            expires, delay;
    MprOsThread         thread;
    int                 signalled, wasRunning, runEvents, nevents;

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
    signalled = 0;

    lock(es);
    wasRunning = isRunning(dispatcher);
    runEvents = (!wasRunning || !dispatcher->owner || dispatcher->owner == thread);
    if (runEvents && !wasRunning) {
        queueDispatcher(es->runQ, dispatcher);
    }
    unlock(es);

    for (; es->now <= expires && !mprIsStoppingCore(); es->now = mprGetTicks()) {
        if (runEvents) {
            if (dispatchEvents(dispatcher)) {
                signalled++;
                break;
            }
        }
        lock(es);
        delay = getDispatcherIdleTicks(dispatcher, expires - es->now);
        dispatcher->flags |= MPR_DISPATCHER_WAITING;
        unlock(es);

        mprYield(MPR_YIELD_STICKY);

        nevents = mprWaitForCond(dispatcher->cond, delay);
        mprResetYield();
        dispatcher->flags &= ~MPR_DISPATCHER_WAITING;

        if (nevents == 0) {
            if (runEvents) {
                dispatchEvents(dispatcher);
            }
            signalled++;
            break;
        }
        es->now = mprGetTicks();
    }
    if (runEvents && !wasRunning) {
        dequeueDispatcher(dispatcher);
        mprScheduleDispatcher(dispatcher);
    }
    return signalled ? 0 : MPR_ERR_TIMEOUT;
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
    idle = (dispatcher == runQ) ? 1 : (dispatcher->eventQ == dispatcher->eventQ->next);
    unlock(es);
    return idle;
}


/*
    Relay an event to a dispatcher. This invokes the callback proc as though it was invoked from the given dispatcher. 
 */
PUBLIC void mprRelayEvent(MprDispatcher *dispatcher, void *proc, void *data, MprEvent *event)
{
    MprOsThread     priorOwner;

    if (isRunning(dispatcher) && dispatcher->owner != mprGetCurrentOsThread()) {
        mprError("Relay to a running dispatcher owned by another thread");
    }
    if (event) {
        event->timestamp = dispatcher->service->now;
    }
    priorOwner = dispatcher->owner;
    queueDispatcher(dispatcher->service->runQ, dispatcher);

    dispatcher->owner = mprGetCurrentOsThread();
    ((MprEventProc) proc)(data, event);
    dispatcher->owner = priorOwner;

    dequeueDispatcher(dispatcher);
    mprScheduleDispatcher(dispatcher);
}


/*
    Internal use only. Run the "main" dispatcher if not using a user events thread. 
 */
PUBLIC int mprRunDispatcher(MprDispatcher *dispatcher)
{
    assert(dispatcher);
    if (isRunning(dispatcher) && dispatcher->owner != mprGetCurrentOsThread()) {
        mprError("Relay to a running dispatcher owned by another thread");
        return MPR_ERR_BAD_STATE;
    }
    queueDispatcher(dispatcher->service->runQ, dispatcher);
    return 0;
}


/*
    Schedule a dispatcher to run but don't disturb an already running dispatcher. If the event queue is empty, 
    the dispatcher is moved to the idleQ. If there is a past-due event, it is moved to the readyQ. If there is a future 
    event pending, it is put on the waitQ.
 */
PUBLIC void mprScheduleDispatcher(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *event;
    int                 mustWakeWaitService, mustWakeCond;

    assert(dispatcher);
    if (dispatcher->flags & MPR_DISPATCHER_DESTROYED) {
        return;
    }
    es = dispatcher->service;
    lock(es);
    if (isRunning(dispatcher)) {
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
    Run events for a dispatcher
 */
static int dispatchEvents(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *event;
    MprOsThread         priorOwner;
    int                 count;

    assert(isRunning(dispatcher));
    mprTrace(7, "dispatchEvents for %s", dispatcher->name);
    es = dispatcher->service;
    priorOwner = dispatcher->owner;
    dispatcher->owner = mprGetCurrentOsThread();

    /*
        Events are removed from the dispatcher queue and put onto the currentQ. This is so they will be marked for GC.
        If the callback calls mprRemoveEvent, it will not remove from the currentQ. If it was a continuous event, 
        mprRemoveEvent will clear the continuous flag.

        OPT - this could all be simpler if dispatchEvents was never called recursively. Then a currentQ would not be needed,
        and neither would a running flag. See mprRemoveEvent().
     */
    for (count = 0; (event = mprGetNextEvent(dispatcher)) != 0; count++) {
        mprTrace(7, "Call event %s", event->name);
        assert(!(event->flags & MPR_EVENT_RUNNING));
        event->flags |= MPR_EVENT_RUNNING;
        assert(event->proc);
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
            mprDequeueEvent(event);
        }
        es->eventCount++;
        unlock(es);
    }
    dispatcher->owner = priorOwner;
    return count;
}


/*
    Run events for a dispatcher in a worker thread. When complete, reschedule the dispatcher as required.
 */
static void dispatchEventsWorker(MprDispatcher *dispatcher)
{
    dispatchEvents(dispatcher);
    if (!(dispatcher->flags == MPR_DISPATCHER_DESTROYED)) {
        dequeueDispatcher(dispatcher);
        mprScheduleDispatcher(dispatcher);
    }
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

    } else if (readyQ->next == readyQ) {
        /*
            ReadyQ is empty, try to transfer a dispatcher with due events onto the readyQ
         */
        for (dp = waitQ->next; dp != waitQ; dp = next) {
            next = dp->next;
            event = dp->eventQ->next;
            if (event->due <= es->now) {
                queueDispatcher(es->readyQ, dp);
                break;
            }
        }
    }
    if (!dispatcher && readyQ->next != readyQ) {
        dispatcher = readyQ->next;
    }
    unlock(es);
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
    dispatcher->next = dispatcher;
    dispatcher->prev = dispatcher;
    dispatcher->parent = dispatcher;
}


static void queueDispatcher(MprDispatcher *prior, MprDispatcher *dispatcher)
{
    assert(dispatcher->service == MPR->eventService);
    lock(dispatcher->service);

    if (dispatcher->parent) {
        dequeueDispatcher(dispatcher);
    }
    dispatcher->parent = prior->parent;
    dispatcher->prev = prior;
    dispatcher->next = prior->next;
    prior->next->prev = dispatcher;
    prior->next = dispatcher;
    unlock(dispatcher->service);
}


static void dequeueDispatcher(MprDispatcher *dispatcher)
{
    lock(dispatcher->service);
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
    unlock(dispatcher->service);
}


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
