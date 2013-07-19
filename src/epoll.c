/**
    epoll.c - Wait for I/O by using epoll on unix like systems.

    This module augments the mprWait wait services module by providing kqueue() based waiting support.
    Also see mprAsyncSelectWait and mprSelectWait. This module is thread-safe.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

#if MPR_EVENT_EPOLL
/********************************** Forwards **********************************/

static void serviceIO(MprWaitService *ws, struct epoll_event *events, int count);

/************************************ Code ************************************/

PUBLIC int mprCreateNotifierService(MprWaitService *ws)
{
    struct epoll_event  ev;

    if ((ws->handlerMap = mprCreateList(MPR_FD_MIN, MPR_LIST_STATIC_VALUES)) == 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }
    if ((ws->epoll = epoll_create(BIT_MAX_EVENTS)) < 0) {
        mprError("Call to epoll() failed");
        return MPR_ERR_CANT_INITIALIZE;
    }

#if defined(EFD_NONBLOCK)
    if ((ws->breakFd[MPR_READ_PIPE] = eventfd(0, 0)) < 0) {
        mprError("Cannot open breakout event");
        return MPR_ERR_CANT_INITIALIZE;
    }
#else
    /*
        Initialize the "wakeup" pipe. This is used to wakeup the service thread if other threads need 
     *  to wait for I/O.
     */
    if (pipe(ws->breakFd) < 0) {
        mprError("Cannot open breakout pipe");
        return MPR_ERR_CANT_INITIALIZE;
    }
    fcntl(ws->breakFd[0], F_SETFL, fcntl(ws->breakFd[0], F_GETFL) | O_NONBLOCK);
    fcntl(ws->breakFd[1], F_SETFL, fcntl(ws->breakFd[1], F_GETFL) | O_NONBLOCK);
#endif
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    ev.data.fd = ws->breakFd[MPR_READ_PIPE];
    epoll_ctl(ws->epoll, EPOLL_CTL_ADD, ws->breakFd[MPR_READ_PIPE], &ev);
    return 0;
}


PUBLIC void mprManageEpoll(MprWaitService *ws, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ws->handlerMap);

    } else if (flags & MPR_MANAGE_FREE) {
        if (ws->epoll) {
            close(ws->epoll);
            ws->epoll = 0;
        }
        if (ws->breakFd[0] >= 0) {
            close(ws->breakFd[0]);
        }
        if (ws->breakFd[1] >= 0) {
            close(ws->breakFd[1]);
        }
    }
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
            if ((rc = epoll_ctl(ws->epoll, EPOLL_CTL_DEL, fd, &ev)) != 0) {
                mprError("Epoll del error %d on fd %d", errno, fd);
            }
        }
        ev.events = 0;
        if (mask & MPR_READABLE) {
            ev.events |= (EPOLLIN | EPOLLHUP);
        }
        if (mask & MPR_WRITABLE) {
            ev.events |= EPOLLOUT | EPOLLHUP;
        }
        if (ev.events) {
            if ((rc = epoll_ctl(ws->epoll, EPOLL_CTL_ADD, fd, &ev)) != 0) {
                mprError("Epoll add error %d on fd %d", errno, fd);
            }
        }
        wp->desiredMask = mask;
        if (wp->event) {
            mprRemoveEvent(wp->event);
            wp->event = 0;
        }
    }
    mprSetItem(ws->handlerMap, fd, wp);
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
    if ((epfd = epoll_create(BIT_MAX_EVENTS)) < 0) {
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
    rc = epoll_wait(epfd, events, sizeof(events) / sizeof(struct epoll_event), timeout);
    close(epfd);

    result = 0;
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
    struct epoll_event  events[BIT_MAX_EVENTS];
    int                 nevents;

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

    if ((nevents = epoll_wait(ws->epoll, events, sizeof(events) / sizeof(struct epoll_event), timeout)) < 0) {
        if (errno != EINTR) {
            mprTrace(7, "epoll returned %d, errno %d", nevents, mprGetOsError());
        }
    }
    mprClearWaiting();
    mprResetYield();

    if (nevents > 0) {
        serviceIO(ws, events, nevents);
    }
    ws->wakeRequested = 0;
}


static void serviceIO(MprWaitService *ws, struct epoll_event *events, int count)
{
    MprWaitHandler      *wp;
    struct epoll_event  *ev;
    int                 fd, i, mask;

    lock(ws);
    for (i = 0; i < count; i++) {
        ev = &events[i];
        fd = ev->data.fd;
        if (fd == ws->breakFd[MPR_READ_PIPE]) {
            char buf[16];
            if (read(fd, buf, sizeof(buf)) < 0) {}
            continue;
        }
        if (fd < 0 || (wp = mprGetItem(ws->handlerMap, fd)) == 0) {
            mprError("WARNING: fd not in handler map. fd %d", fd);
            continue;
        }
        mask = 0;
        if (ev->events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
            mask |= MPR_READABLE;
        }
        if (ev->events & (EPOLLOUT | EPOLLHUP)) {
            mask |= MPR_WRITABLE;
        }
        wp->presentMask = mask & wp->desiredMask;

        if (wp->presentMask) {
            if (wp->flags & MPR_WAIT_IMMEDIATE) {
                (wp->proc)(wp->handlerData, NULL);
            } else {
                /*
                    Suppress further events while this event is being serviced. User must re-enable.
                 */
                mprNotifyOn(ws, wp, 0);
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

    ws = MPR->waitService;
    if (!ws->wakeRequested) {
        /*
            This code works for both eventfds and for pipes. We must write a value of 0x1 for eventfds.
         */
        ws->wakeRequested = 1;
#if defined(EFD_NONBLOCK)
        uint64 c = 1;
        if (write(ws->breakFd[MPR_READ_PIPE], &c, sizeof(c)) != sizeof(c)) {
            mprError("Cannot write to break port %d\n", errno);
        }
#else
        int c = 1;
        if (write(ws->breakFd[MPR_WRITE_PIPE], &c, 1) < 0) {}
#endif
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
