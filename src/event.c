/*
    event.c - Event and dispatch services

    This module is thread-safe.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

/***************************** Forward Declarations ***************************/

static void initEvent(MprDispatcher *dispatcher, MprEvent *event, cchar *name, MprTicks period, void *proc, 
        void *data, int flgs);
static void initEventQ(MprEvent *q);
static void manageEvent(MprEvent *event, int flags);
static void queueEvent(MprEvent *prior, MprEvent *event);

/************************************* Code ***********************************/
/*
    Create and queue a new event for service. Period is used as the delay before running the event and as the period 
    between events for continuous events.
 */
PUBLIC MprEvent *mprCreateEventQueue()
{
    MprEvent    *queue;

    if ((queue = mprAllocObj(MprEvent, manageEvent)) == 0) {
        return 0;
    }
    initEventQ(queue);
    return queue;
}


/*
    Create and queue a new event for service. Period is used as the delay before running the event and as the period 
    between events for continuous events.
 */
PUBLIC MprEvent *mprCreateEvent(MprDispatcher *dispatcher, cchar *name, MprTicks period, void *proc, void *data, int flags)
{
    MprEvent    *event;

    if ((event = mprAllocObj(MprEvent, manageEvent)) == 0) {
        return 0;
    }
    if (dispatcher == 0) {
        dispatcher = (flags & MPR_EVENT_QUICK) ? MPR->nonBlock : MPR->dispatcher;
    }
    initEvent(dispatcher, event, name, period, proc, data, flags);
    if (!(flags & MPR_EVENT_DONT_QUEUE)) {
        mprQueueEvent(dispatcher, event);
    }
    return event;
}


static void manageEvent(MprEvent *event, int flags)
{
    assure(event->magic == MPR_EVENT_MAGIC);

    if (flags & MPR_MANAGE_MARK) {
        /*
            Events in dispatcher queues are marked by the dispatcher managers, not via event->next,prev
         */
        assure(event->dispatcher == 0 || event->dispatcher->magic == MPR_DISPATCHER_MAGIC);
        mprMark(event->name);
        mprMark(event->dispatcher);
        mprMark(event->handler);
        if (!(event->flags & MPR_EVENT_STATIC_DATA)) {
            mprMark(event->data);
        }

    } else if (flags & MPR_MANAGE_FREE) {
        if (event->next) {
            assure(event->dispatcher == 0 || event->dispatcher->magic == MPR_DISPATCHER_MAGIC);
            mprRemoveEvent(event);
            event->magic = 1;
        }
    }
}


static void initEvent(MprDispatcher *dispatcher, MprEvent *event, cchar *name, MprTicks period, void *proc, void *data, 
    int flags)
{
    assure(dispatcher);
    assure(event);
    assure(proc);
    assure(event->next == 0);
    assure(event->prev == 0);

    dispatcher->service->now = mprGetTicks();
    event->name = sclone(name);
    event->timestamp = dispatcher->service->now;
    event->proc = proc;
    event->period = period;
    event->due = event->timestamp + period;
    event->data = data;
    event->dispatcher = dispatcher;
    event->next = event->prev = 0;
    event->flags = flags;
#if UNUSED
    event->continuous = (flags & MPR_EVENT_CONTINUOUS) ? 1 : 0;
#endif
    event->magic = MPR_EVENT_MAGIC;
}


/*
    Create an interval timer
 */
PUBLIC MprEvent *mprCreateTimerEvent(MprDispatcher *dispatcher, cchar *name, MprTicks period, void *proc, 
    void *data, int flags)
{
    return mprCreateEvent(dispatcher, name, period, proc, data, MPR_EVENT_CONTINUOUS | flags);
}


PUBLIC void mprQueueEvent(MprDispatcher *dispatcher, MprEvent *event)
{
    MprEventService     *es;
    MprEvent            *prior, *q;

    assure(dispatcher);
    assure(event);
    assure(event->timestamp);
#if KEEP
    assure(dispatcher->flags & MPR_DISPATCHER_ENABLED);
#endif
    assure(!(dispatcher->flags & MPR_DISPATCHER_DESTROYED));
    assure(dispatcher->magic == MPR_DISPATCHER_MAGIC);
    assure(event->magic == MPR_EVENT_MAGIC);

    es = dispatcher->service;

    lock(es);
    q = dispatcher->eventQ;
    for (prior = q->prev; prior != q; prior = prior->prev) {
        if (event->due > prior->due) {
            break;
        } else if (event->due == prior->due) {
            break;
        }
    }
    assure(prior->next);
    assure(prior->prev);
    
    queueEvent(prior, event);
    event->dispatcher = dispatcher;
    es->eventCount++;
    if (dispatcher->flags & MPR_DISPATCHER_ENABLED) {
        mprScheduleDispatcher(dispatcher);
    }
    unlock(es);
}


PUBLIC void mprRemoveEvent(MprEvent *event)
{
    MprEventService     *es;
    MprDispatcher       *dispatcher;

    dispatcher = event->dispatcher;
    if (dispatcher) {
        es = dispatcher->service;
        lock(es);
        if (event->next && !(event->flags & MPR_EVENT_RUNNING)) {
            mprDequeueEvent(event);
        }
        event->dispatcher = 0;
        event->flags &= ~MPR_EVENT_CONTINUOUS;
        if (dispatcher->flags & MPR_DISPATCHER_ENABLED && 
                event->due == es->willAwake && dispatcher->eventQ->next != dispatcher->eventQ) {
            mprScheduleDispatcher(dispatcher);
        }
        unlock(es);
    }
}


PUBLIC void mprRescheduleEvent(MprEvent *event, MprTicks period)
{
    MprEventService     *es;
    MprDispatcher       *dispatcher;

    assure(event->magic == MPR_EVENT_MAGIC);
    dispatcher = event->dispatcher;
    assure(dispatcher->magic == MPR_DISPATCHER_MAGIC);

    es = dispatcher->service;

    lock(es);
    event->period = period;
    event->timestamp = es->now;
    event->due = event->timestamp + period;
    if (event->next) {
        mprRemoveEvent(event);
    }
    unlock(es);
    mprQueueEvent(dispatcher, event);
}


PUBLIC void mprStopContinuousEvent(MprEvent *event)
{
    lock(event->dispatcher->service);
    event->flags &= ~MPR_EVENT_CONTINUOUS;
    unlock(event->dispatcher->service);
}


PUBLIC void mprRestartContinuousEvent(MprEvent *event)
{
    lock(event->dispatcher->service);
    event->flags |= MPR_EVENT_CONTINUOUS;
    unlock(event->dispatcher->service);
    mprRescheduleEvent(event, event->period);
}


PUBLIC void mprEnableContinuousEvent(MprEvent *event, int enable)
{
    lock(event->dispatcher->service);
    event->flags &= ~MPR_EVENT_CONTINUOUS;
    if (enable) {
        event->flags |= MPR_EVENT_CONTINUOUS;
    }
    unlock(event->dispatcher->service);
}


/*
    Get the next due event from the front of the event queue.
 */
PUBLIC MprEvent *mprGetNextEvent(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *event, *next;

    assure(dispatcher->magic == MPR_DISPATCHER_MAGIC);
    es = dispatcher->service;
    event = 0;
    lock(es);
    next = dispatcher->eventQ->next;
    if (next != dispatcher->eventQ) {
        if (next->due <= es->now) {
            event = next;
            queueEvent(dispatcher->currentQ, event);
            assure(event->magic == MPR_EVENT_MAGIC);
        }
    }
    unlock(es);
    return event;
}


PUBLIC int mprGetEventCount(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *event;
    int                 count;

    assure(dispatcher->magic == MPR_DISPATCHER_MAGIC);

    es = dispatcher->service;

    lock(es);
	count = 0;
    for (event = dispatcher->eventQ->next; event != dispatcher->eventQ; event = event->next) {
        assure(event->magic == MPR_EVENT_MAGIC);
        count++;
    }
    unlock(es);
    return count;
}


static void initEventQ(MprEvent *q)
{
    assure(q);

    q->next = q;
    q->prev = q;
    q->magic = MPR_EVENT_MAGIC;
}


/*
    Append a new event. Must be locked when called.
 */
static void queueEvent(MprEvent *prior, MprEvent *event)
{
    assure(prior);
    assure(event);
    assure(prior->next);
    assure(event->magic == MPR_EVENT_MAGIC);
    assure(event->dispatcher == 0 || event->dispatcher->magic == MPR_DISPATCHER_MAGIC);

    if (event->next) {
        mprDequeueEvent(event);
    }
    event->prev = prior;
    event->next = prior->next;
    prior->next->prev = event;
    prior->next = event;
}


/*
    Remove an event. Must be locked when called.
 */
PUBLIC void mprDequeueEvent(MprEvent *event)
{
    assure(event);
    assure(event->magic == MPR_EVENT_MAGIC);
    assure(event->dispatcher == 0 || event->dispatcher->magic == MPR_DISPATCHER_MAGIC);

    /* If a continuous event is removed, next may already be null */
    if (event->next) {
        event->next->prev = event->prev;
        event->prev->next = event->next;
        event->next = 0;
        event->prev = 0;
    }
}


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
