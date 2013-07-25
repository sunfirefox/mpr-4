/**
    kevent.c - Wait for I/O by using kevent on MacOSX Unix systems.

    This module augments the mprWait wait services module by providing kqueue() based waiting support.
    Also see mprAsyncSelectWait and mprSelectWait. This module is thread-safe.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

#if MPR_EVENT_KQUEUE

/********************************** Forwards **********************************/

static void serviceIO(MprWaitService *ws, struct kevent *events, int count);

/************************************ Code ************************************/

PUBLIC int mprCreateNotifierService(MprWaitService *ws)
{
    struct kevent   ev;

    if ((ws->kq = kqueue()) < 0) {
        mprError("Call to kqueue() failed");
        return MPR_ERR_CANT_INITIALIZE;
    }
    EV_SET(&ev, 0, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, NULL);
    if (kevent(ws->kq, &ev, 1, NULL, 0, NULL) < 0) {
        mprError("Cannot issue notifier wakeup event, errno %d", errno);
        return MPR_ERR_CANT_INITIALIZE;
    }
    if ((ws->handlerMap = mprCreateList(MPR_FD_MIN, 0)) == 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }
    return 0;
}


PUBLIC void mprManageKqueue(MprWaitService *ws, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ws->handlerMap);

    } else if (flags & MPR_MANAGE_FREE) {
        if (ws->kq >= 0) {
            close(ws->kq);
            ws->kq = 0;
        }
    }
}


PUBLIC int mprNotifyOn(MprWaitService *ws, MprWaitHandler *wp, int mask)
{
    struct kevent   interest[4], *kp;
    int             fd;

    assert(wp);
    fd = wp->fd;
    kp = &interest[0];

    lock(ws);
    mprTrace(7, "mprNotifyOn: fd %d, mask %x, old mask %x", wp->fd, mask, wp->desiredMask);
    if (wp->desiredMask != mask) {
        assert(fd >= 0);
        if (wp->desiredMask & MPR_READABLE && !(mask & MPR_READABLE)) {
            EV_SET(kp, fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
            kp++;
        }
        if (wp->desiredMask & MPR_WRITABLE && !(mask & MPR_WRITABLE)) {
            EV_SET(kp, fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
            kp++;
        }
        if (mask & MPR_READABLE) {
            EV_SET(kp, fd, EVFILT_READ, EV_ADD, 0, 0, 0);
            kp++;
        }
        if (mask & MPR_WRITABLE) {
            EV_SET(kp, fd, EVFILT_WRITE, EV_ADD, 0, 0, 0);
            kp++;
        }
        wp->desiredMask = mask;
        if (wp->event) {
            mprRemoveEvent(wp->event);
            wp->event = 0;
        }
        if (kevent(ws->kq, interest, (int) (kp - interest), NULL, 0, NULL) < 0) {
            /*
                Reissue and get results. Test for broken pipe case.
             */
            if (mask) {
                int rc = kevent(ws->kq, interest, 1, interest, 1, NULL);
                if (rc == 1 && interest[0].flags & EV_ERROR && interest[0].data == EPIPE) {
                    /* Broken PIPE - just ignore */
                } else {
                    mprError("Cannot issue notifier wakeup event, errno %d", errno);
                }
            }
        }
        mprSetItem(ws->handlerMap, fd, mask ? wp : 0);
    }
    unlock(ws);
    return 0;
}


/*
    Wait for I/O on a single file descriptor. Return a mask of events found. Mask is the events of interest.
    timeout is in milliseconds.
 */
PUBLIC int mprWaitForSingleIO(int fd, int mask, MprTicks timeout)
{
    struct timespec ts;
    struct kevent   interest[2], events[1];
    int             kq, interestCount, rc, result;

    if (timeout < 0) {
        timeout = MAXINT;
    }
    interestCount = 0; 
    if (mask & MPR_READABLE) {
        EV_SET(&interest[interestCount++], fd, EVFILT_READ, EV_ADD, 0, 0, 0);
    }
    if (mask & MPR_WRITABLE) {
        EV_SET(&interest[interestCount++], fd, EVFILT_WRITE, EV_ADD, 0, 0, 0);
    }
    if ((kq = kqueue()) < 0) {
        mprError("Kqueue returned %d, errno %d", kq, errno);
        return MPR_ERR_CANT_OPEN;
    }
    ts.tv_sec = ((int) (timeout / 1000));
    ts.tv_nsec = ((int) (timeout % 1000)) * 1000 * 1000;

    result = 0;
    if ((rc = kevent(kq, interest, interestCount, events, 1, &ts)) < 0) {
        mprError("Kevent returned %d, errno %d", rc, errno);

    } else if (rc > 0) {
        if (events[0].filter & EVFILT_READ) {
            result |= MPR_READABLE;
        }
        if (events[0].filter == EVFILT_WRITE) {
            result |= MPR_WRITABLE;
        }
    }
    close(kq);
    return result;
}


/*
    Wait for I/O on all registered file descriptors. Timeout is in milliseconds. Return the number of events detected.
 */
PUBLIC void mprWaitForIO(MprWaitService *ws, MprTicks timeout)
{
    struct timespec ts;
    struct kevent   events[BIT_MAX_EVENTS];
    int             nevents;

    assert(timeout > 0);

    if (ws->needRecall) {
        mprDoWaitRecall(ws);
        return;
    }
    if (timeout < 0) {
        timeout = MAXINT;
    }
#if BIT_DEBUG
    if (mprGetDebugMode() && timeout > 30000) {
        timeout = 30000;
    }
#endif
    ts.tv_sec = ((int) (timeout / 1000));
    ts.tv_nsec = ((int) ((timeout % 1000) * 1000 * 1000));

    mprYield(MPR_YIELD_STICKY);

    if ((nevents = kevent(ws->kq, NULL, 0, events, BIT_MAX_EVENTS, &ts)) < 0) {
        if (errno != EINTR) {
            mprTrace(7, "Kevent returned %d, errno %d", nevents, mprGetOsError());
        }
    }
    mprClearWaiting();
    mprResetYield();

    if (nevents > 0) {
        serviceIO(ws, events, nevents);
    }
    ws->wakeRequested = 0;
}


static void serviceIO(MprWaitService *ws, struct kevent *events, int count)
{
    MprWaitHandler      *wp;
    struct kevent       *kev;
    int                 fd, i, mask, prior, err;

    lock(ws);
    for (i = 0; i < count; i++) {
        kev = &events[i];
        fd = (int) kev->ident;
        if (kev->filter == EVFILT_USER) {
            continue;
        }
        if (fd < 0 || (wp = mprGetItem(ws->handlerMap, fd)) == 0) {
            /*
                This can happen if a writable event has been triggered (e.g. MprCmd command stdin pipe) and the pipe is closed.
                This thread may have waked from kevent before the pipe is closed and the wait handler removed from the map.
             */
            continue;
        }
        assert(mprIsValid(wp));
        mask = 0;
        if (kev->flags & EV_ERROR) {
            err = (int) kev->data;
            if (err == ENOENT) {
                /* 
                    File descriptor was closed and re-opened. Re-enable event.
                 */
                prior = wp->desiredMask;
                mprNotifyOn(ws, wp, 0);
                wp->desiredMask = 0;
                mprNotifyOn(ws, wp, prior);
                mprError("kqueue: file descriptor may have been closed and reopened, fd %d", wp->fd);
                continue;

            } else if (err == EBADF || err == EINVAL) {
                /* File descriptor was closed */
                mprNotifyOn(ws, wp, 0);
                mprError("kqueue: invalid file descriptor %d, fd %d", wp->fd);
                mask |= MPR_READABLE;
            }
        }
        if (kev->filter == EVFILT_READ) {
            mask |= MPR_READABLE;
        }
        if (kev->filter == EVFILT_WRITE) {
            mask |= MPR_WRITABLE;
        }
        assert(mprIsValid(wp));
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
    struct kevent   ev;

    ws = MPR->waitService;
    if (!ws->wakeRequested) {
        ws->wakeRequested = 1;
        EV_SET(&ev, 0, EVFILT_USER, 0, NOTE_TRIGGER, 0, NULL);
        if (kevent(ws->kq, &ev, 1, NULL, 0, NULL) < 0) {
            mprError("Cannot issue notifier wakeup event, errno %d", errno);
        }
    }
}

#else
void kqueueDummy() {}
#endif /* MPR_EVENT_KQUEUE */

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
