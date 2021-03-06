/*
    wait.c - Wait for I/O service.

    This module provides wait management for sockets and other file descriptors and allows users to create wait
    handlers which will be called when I/O events are detected. Multiple backends (one at a time) are supported.

    This module is thread-safe.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

/***************************** Forward Declarations ***************************/

static void ioEvent(void *data, MprEvent *event);
static void manageWaitService(MprWaitService *ws, int flags);
static void manageWaitHandler(MprWaitHandler *wp, int flags);

/************************************ Code ************************************/
/*
    Initialize the service
 */
PUBLIC MprWaitService *mprCreateWaitService()
{
    MprWaitService  *ws;

    ws = mprAllocObj(MprWaitService, manageWaitService);
    if (ws == 0) {
        return 0;
    }
    MPR->waitService = ws;
    ws->handlers = mprCreateList(-1, MPR_LIST_STATIC_VALUES);
    ws->mutex = mprCreateLock();
    ws->spin = mprCreateSpinLock();
    mprCreateNotifierService(ws);
    return ws;
}


static void manageWaitService(MprWaitService *ws, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ws->handlers);
        mprMark(ws->handlerMap);
        mprMark(ws->mutex);
        mprMark(ws->spin);
    }
#if MPR_EVENT_ASYNC
    /* Nothing to manage */
#endif
#if MPR_EVENT_KQUEUE
    mprManageKqueue(ws, flags);
#endif
#if MPR_EVENT_EPOLL
    mprManageEpoll(ws, flags);
#endif
#if MPR_EVENT_SELECT
    mprManageSelect(ws, flags);
#endif
}


static MprWaitHandler *initWaitHandler(MprWaitHandler *wp, int fd, int mask, MprDispatcher *dispatcher, void *proc, 
    void *data, int flags)
{
    MprWaitService  *ws;

    assert(fd >= 0);

    ws = MPR->waitService;
    wp->fd              = fd;
    wp->notifierIndex   = -1;
    wp->dispatcher      = dispatcher;
    wp->proc            = proc;
    wp->flags           = 0;
    wp->handlerData     = data;
    wp->service         = ws;
    wp->flags           = flags;

    if (mprGetListLength(ws->handlers) >= FD_SETSIZE) {
        mprTrace(6, "io: Too many io handlers: %d", FD_SETSIZE);
        return 0;
    }
#if BIT_UNIX_LIKE || VXWORKS
    if (fd >= FD_SETSIZE) {
        mprError("File descriptor %d exceeds max io of %d", fd, FD_SETSIZE);
    }
#endif
    if (mask) {
        if (mprAddItem(ws->handlers, wp) < 0) {
            return 0;
        }
        mprNotifyOn(ws, wp, mask);
    }
    return wp;
}


PUBLIC MprWaitHandler *mprCreateWaitHandler(int fd, int mask, MprDispatcher *dispatcher, void *proc, void *data, int flags)
{
    MprWaitHandler  *wp;

    assert(fd >= 0);

    if ((wp = mprAllocObj(MprWaitHandler, manageWaitHandler)) == 0) {
        return 0;
    }
    return initWaitHandler(wp, fd, mask, dispatcher, proc, data, flags);
}


static void manageWaitHandler(MprWaitHandler *wp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(wp->handlerData);
        mprMark(wp->dispatcher);
        mprMark(wp->service);
        mprMark(wp->next);
        mprMark(wp->prev);
        mprMark(wp->requiredWorker);
        mprMark(wp->thread);
        mprMark(wp->callbackComplete);
        mprMark(wp->event);

    } else if (flags & MPR_MANAGE_FREE) {
        mprRemoveWaitHandler(wp);
    }
}


PUBLIC void mprRemoveWaitHandler(MprWaitHandler *wp)
{
    MprWaitService      *ws;

    if (wp == 0) {
        return;
    }
    ws = wp->service;
    if (ws == 0) {
        /* This wait handler was never initialized. */
        return;
    }
    lock(ws);
    if (wp->fd >= 0) {
        if (wp->desiredMask) {
            mprNotifyOn(ws, wp, 0);
        }
        mprRemoveItem(ws->handlers, wp);
        wp->fd = -1;
        if (wp->event) {
            mprRemoveEvent(wp->event);
            wp->event = 0;
        }
    }
    unlock(ws);
}


PUBLIC void mprQueueIOEvent(MprWaitHandler *wp)
{
    MprDispatcher   *dispatcher;
    MprEvent        *event;

    if (wp->flags & MPR_WAIT_NEW_DISPATCHER) {
        dispatcher = mprCreateDispatcher("IO", MPR_DISPATCHER_AUTO);
    } else if (wp->dispatcher) {
        dispatcher = wp->dispatcher;
    } else {
        dispatcher = mprGetDispatcher();
    }
    event = mprCreateEvent(dispatcher, "IOEvent", 0, ioEvent, wp->handlerData, MPR_EVENT_DONT_QUEUE);
    event->mask = wp->presentMask;
    event->handler = wp;
    wp->event = event;
    mprQueueEvent(dispatcher, event);
}


static void ioEvent(void *data, MprEvent *event)
{
    assert(event);
    assert(event->handler);

    event->handler->event = 0;
    event->handler->proc(data, event);
}


PUBLIC void mprWaitOn(MprWaitHandler *wp, int mask)
{
    lock(wp->service);
    if (mask != wp->desiredMask) {
        if (wp->flags & MPR_WAIT_RECALL_HANDLER) {
            wp->service->needRecall = 1;
        }
        mprNotifyOn(wp->service, wp, mask);
    }
    unlock(wp->service);
}


/*
    Set a handler to be recalled without further I/O
 */
PUBLIC void mprRecallWaitHandlerByFd(Socket fd)
{
    MprWaitService  *ws;
    MprWaitHandler  *wp;
    int             index;

    ws = MPR->waitService;
    lock(ws);
    for (index = 0; (wp = (MprWaitHandler*) mprGetNextItem(ws->handlers, &index)) != 0; ) {
        if (wp->fd == fd) {
            wp->flags |= MPR_WAIT_RECALL_HANDLER;
            ws->needRecall = 1;
            mprWakeEventService();
            break;
        }
    }
    unlock(ws);
}


PUBLIC void mprRecallWaitHandler(MprWaitHandler *wp)
{
    MprWaitService  *ws;

    if (wp) {
        ws = MPR->waitService;
        lock(ws);
        wp->flags |= MPR_WAIT_RECALL_HANDLER;
        ws->needRecall = 1;
        mprWakeEventService();
        unlock(ws);
    }
}


/*
    Recall a handler which may have buffered data. Only called by notifiers.
 */
PUBLIC void mprDoWaitRecall(MprWaitService *ws)
{
    MprWaitHandler      *wp;
    int                 index;

    lock(ws);
    ws->needRecall = 0;
    for (index = 0; (wp = (MprWaitHandler*) mprGetNextItem(ws->handlers, &index)) != 0; ) {
        if ((wp->flags & MPR_WAIT_RECALL_HANDLER) && (wp->desiredMask & MPR_READABLE)) {
            wp->presentMask |= MPR_READABLE;
            wp->flags &= ~MPR_WAIT_RECALL_HANDLER;
            mprNotifyOn(ws, wp, 0);
            mprQueueIOEvent(wp);
        }
    }
    unlock(ws);
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
