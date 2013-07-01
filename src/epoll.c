/**
    epoll.c - Wait for I/O by using epoll on unix like systems.

    This module augments the mprWait wait services module by providing kqueue() based waiting support.
    Also see mprAsyncSelectWait and mprSelectWait. This module is thread-safe.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

#if MPR_EVENT_EPOLL
/********************************** Defines ***********************************/

#ifndef BIT_MAX_EPOLL
    #define BIT_MAX_EPOLL  32
#endif

/********************************** Forwards **********************************/

static int growEvents(MprWaitService *ws);
static void serviceIO(MprWaitService *ws, int count);

/************************************ Code ************************************/

PUBLIC int mprCreateNotifierService(MprWaitService *ws)
{
    struct epoll_event  ev;

    ws->eventsMax = BIT_MAX_EPOLL;
    ws->handlerMax = MPR_FD_MIN;
    ws->events = mprAllocZeroed(sizeof(struct epoll_event) * ws->eventsMax);
    ws->handlerMap = mprAllocZeroed(sizeof(MprWaitHandler*) * ws->handlerMax);
    if (ws->events == 0 || ws->handlerMap == 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }
    if ((ws->epoll = epoll_create(BIT_MAX_EPOLL)) < 0) {
        mprError("Call to epoll() failed");
        return MPR_ERR_CANT_INITIALIZE;
    }
    /*
        Initialize the "wakeup" pipe. This is used to wakeup the service thread if other threads need 
     *  to wait for I/O.
     */
    if (pipe(ws->breakPipe) < 0) {
        mprError("Cannot open breakout pipe");
        return MPR_ERR_CANT_INITIALIZE;
    }
    fcntl(ws->breakPipe[0], F_SETFL, fcntl(ws->breakPipe[0], F_GETFL) | O_NONBLOCK);
    fcntl(ws->breakPipe[1], F_SETFL, fcntl(ws->breakPipe[1], F_GETFL) | O_NONBLOCK);

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    ev.data.fd = ws->breakPipe[MPR_READ_PIPE];
    epoll_ctl(ws->epoll, EPOLL_CTL_ADD, ws->breakPipe[MPR_READ_PIPE], &ev);
    return 0;
}


PUBLIC void mprManageEpoll(MprWaitService *ws, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ws->events);
    
    } else if (flags & MPR_MANAGE_FREE) {
        if (ws->epoll) {
            close(ws->epoll);
            ws->epoll = 0;
        }
        if (ws->breakPipe[0] >= 0) {
            close(ws->breakPipe[0]);
        }
        if (ws->breakPipe[1] >= 0) {
            close(ws->breakPipe[1]);
        }
    }
}


static int growEvents(MprWaitService *ws)
{
    ws->eventsMax *= 2;
    if ((ws->events = mprRealloc(ws->events, sizeof(struct epoll_event) * ws->eventsMax)) == 0) {
        assert(!MPR_ERR_MEMORY);
        return MPR_ERR_MEMORY;
    }
    return 0;
}


PUBLIC int mprNotifyOn(MprWaitService *ws, MprWaitHandler *wp, int mask)
{
    struct epoll_event  ev;
    int                 fd, rc;

    assert(wp);
    fd = wp->fd;

    lock(ws);
    if (wp->desiredMask != mask) {
        memset(&ev, 0, sizeof(ev));
        ev.data.fd = fd;
        if (wp->desiredMask & MPR_READABLE) {
            ev.events |= EPOLLIN | EPOLLHUP;
        }
        if (wp->desiredMask & MPR_WRITABLE) {
            ev.events |= EPOLLOUT;
        }
        if (wp->desiredMask == (MPR_READABLE | MPR_WRITABLE)) {
            ev.events |= EPOLLHUP;
        }
        if (ev.events) {
            rc = epoll_ctl(ws->epoll, EPOLL_CTL_DEL, fd, &ev);
#if KEEP
            if (rc != 0) {
                mprError("Epoll del error %d on fd %d", errno, fd);
            }
#endif
        }
        ev.events = 0;
        if (mask & MPR_READABLE) {
            ev.events |= (EPOLLIN | EPOLLHUP);
        }
        if (mask & MPR_WRITABLE) {
            ev.events |= EPOLLOUT | EPOLLHUP;
        }
        if (ev.events) {
            rc = epoll_ctl(ws->epoll, EPOLL_CTL_ADD, fd, &ev);
            if (rc != 0) {
                mprError("Epoll add error %d on fd %d", errno, fd);
            }
        }
        if (mask && fd >= ws->handlerMax) {
            ws->handlerMax = fd + 32;
            if ((ws->handlerMap = mprRealloc(ws->handlerMap, sizeof(MprWaitHandler*) * ws->handlerMax)) == 0) {
                assert(!MPR_ERR_MEMORY);
                return MPR_ERR_MEMORY;
            }
        }
        assert(ws->handlerMap[fd] == 0 || ws->handlerMap[fd] == wp);
        wp->desiredMask = mask;
        if (wp->event) {
            mprRemoveEvent(wp->event);
            wp->event = 0;
        }
    }
    ws->handlerMap[fd] = (mask) ? wp : 0;
    unlock(ws);
    return 0;
}


/*
    Wait for I/O on a single file descriptor. Return a mask of events found. Mask is the events of interest.
    timeout is in milliseconds.
 */
PUBLIC int mprWaitForSingleIO(int fd, int mask, MprTicks timeout)
{
    struct epoll_event  ev, events[2];
    int                 epfd, rc, result;

    if (timeout < 0 || timeout > MAXINT) {
        timeout = MAXINT;
    }
    memset(&ev, 0, sizeof(ev));
    memset(events, 0, sizeof(events));
    ev.data.fd = fd;
    if ((epfd = epoll_create(BIT_MAX_EPOLL)) < 0) {
        mprError("Call to epoll() failed");
        return MPR_ERR_CANT_INITIALIZE;
    }
    ev.events = 0;
    if (mask & MPR_READABLE) {
        ev.events = EPOLLIN | EPOLLHUP;
    }
    if (mask & MPR_WRITABLE) {
        ev.events = EPOLLOUT | EPOLLHUP;
    }
    if (ev.events) {
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }
    result = 0;
    rc = epoll_wait(epfd, events, sizeof(events) / sizeof(struct epoll_event), timeout);
    close(epfd);
    if (rc < 0) {
        mprTrace(2, "Epoll returned %d, errno %d", rc, errno);
    } else if (rc > 0) {
        if (rc > 0) {
            if ((events[0].events & (EPOLLIN | EPOLLERR | EPOLLHUP)) && (mask & MPR_READABLE)) {
                result |= MPR_READABLE;
            }
            if ((events[0].events & (EPOLLOUT | EPOLLHUP)) && (mask & MPR_WRITABLE)) {
                result |= MPR_WRITABLE;
            }
        }
    }
    return result;
}


/*
    Wait for I/O on all registered file descriptors. Timeout is in milliseconds. Return the number of events detected. 
 */
PUBLIC void mprWaitForIO(MprWaitService *ws, MprTicks timeout)
{
    int     rc;

    if (timeout < 0 || timeout > MAXINT) {
        timeout = MAXINT;
    }
#if BIT_DEBUG
    if (mprGetDebugMode() && timeout > 30000) {
        timeout = 30000;
    }
#endif
    if (ws->needRecall) {
        mprDoWaitRecall(ws);
        return;
    }
    mprYield(MPR_YIELD_STICKY | MPR_YIELD_NO_BLOCK);
    rc = epoll_wait(ws->epoll, ws->events, ws->eventsMax, timeout);

    mprClearWaiting();
    mprResetYield();

    if (rc < 0) {
        if (errno != EINTR) {
            mprTrace(7, "epoll returned %d, errno %d", mprGetOsError());
        }
    } else if (rc > 0) {
        serviceIO(ws, rc);
        if (rc == ws->eventsMax) {
            growEvents(ws);
        }
    }
    ws->wakeRequested = 0;
}


static void serviceIO(MprWaitService *ws, int count)
{
    MprWaitHandler      *wp;
    struct epoll_event  *ev;
    int                 fd, i, mask;

    lock(ws);
    for (i = 0; i < count; i++) {
        ev = &ws->events[i];
        fd = ev->data.fd;
        assert(fd < ws->handlerMax);
        if ((wp = ws->handlerMap[fd]) == 0) {
            char    buf[128];
            if ((ev->events & (EPOLLIN | EPOLLERR | EPOLLHUP)) && (fd == ws->breakPipe[MPR_READ_PIPE])) {
                if (read(fd, buf, sizeof(buf)) < 0) {}
            }
            continue;
        }
        mask = 0;
        if (ev->events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
            mask |= MPR_READABLE;
        }
        if (ev->events & (EPOLLOUT | EPOLLHUP)) {
            mask |= MPR_WRITABLE;
        }
        if (mask == 0) {
            assert(mask);
            continue;
        }
        wp->presentMask = mask & wp->desiredMask;
        if (wp->presentMask) {
            mprTrace(7, "ServiceIO for wp %p", wp);
            if (wp->flags & MPR_WAIT_IMMEDIATE) {
                (wp->proc)(wp->handlerData, NULL);
            } else {
                struct epoll_event  ev;
                memset(&ev, 0, sizeof(ev));
                ev.data.fd = fd;
                wp->desiredMask = 0;
                ws->handlerMap[wp->fd] = 0;
                epoll_ctl(ws->epoll, EPOLL_CTL_DEL, wp->fd, &ev);
                mprQueueIOEvent(wp);
            }
        }
    }
    unlock(ws);
}


/*
    Wake the wait service. WARNING: This routine must not require locking. MprEvents in scheduleDispatcher depends on this.
    Must be async-safe.
 */
PUBLIC void mprWakeNotifier()
{
    MprWaitService  *ws;
    int             c;

    ws = MPR->waitService;
    if (!ws->wakeRequested) {
        ws->wakeRequested = 1;
        c = 0;
        if (write(ws->breakPipe[MPR_WRITE_PIPE], (char*) &c, 1) < 0) {};
    }
}

#else
void epollDummy() {}
#endif /* MPR_EVENT_EPOLL */

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
