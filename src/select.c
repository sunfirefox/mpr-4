/**
    select.c - Wait for I/O by using select.

    This module provides I/O wait management for sockets on VxWorks and systems that use select(). Windows and Unix
    uses different mechanisms. See mprAsyncSelectWait and mprPollWait. This module is thread-safe.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */
/********************************* Includes ***********************************/

#include    "mpr.h"

#if MPR_EVENT_SELECT
/********************************** Forwards **********************************/

static void serviceIO(MprWaitService *ws, fd_set *readMask, fd_set *writeMask, int maxfd);
static void readPipe(MprWaitService *ws);

/************************************ Code ************************************/

PUBLIC int mprCreateNotifierService(MprWaitService *ws)
{
    int     rc, retries, breakPort, breakSock, maxTries;

    ws->highestFd = 0;
    if ((ws->handlerMap = mprCreateList(MPR_FD_MIN, 0)) == 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }
    FD_ZERO(&ws->readMask);
    FD_ZERO(&ws->writeMask);

    /*
        Try to find a good port to use to break out of the select wait
     */ 
    maxTries = 100;
    breakPort = BIT_WAKEUP_PORT;
    for (rc = retries = 0; retries < maxTries; retries++) {
        breakSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (breakSock < 0) {
            mprWarn("Cannot open port %d to use for select. Retrying.\n");
        }
#if BIT_UNIX_LIKE
        fcntl(breakSock, F_SETFD, FD_CLOEXEC);
#endif
        ws->breakAddress.sin_family = AF_INET;
#if CYGWIN || VXWORKS
        /*
            Cygwin & VxWorks don't work with INADDR_ANY
         */
        ws->breakAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
#else
        ws->breakAddress.sin_addr.s_addr = INADDR_ANY;
#endif
        ws->breakAddress.sin_port = htons((short) breakPort);
        rc = bind(breakSock, (struct sockaddr *) &ws->breakAddress, sizeof(ws->breakAddress));
        if (breakSock >= 0 && rc == 0) {
#if VXWORKS
            /* VxWorks 6.0 bug workaround */
            ws->breakAddress.sin_port = htons((short) breakPort);
#endif
            break;
        }
        if (breakSock >= 0) {
            closesocket(breakSock);
        }
        breakPort++;
    }
    if (breakSock < 0 || rc < 0) {
        mprWarn("Cannot bind any port to use for select. Tried %d-%d\n", breakPort, breakPort - maxTries);
        return MPR_ERR_CANT_OPEN;
    }
    ws->breakSock = breakSock;
    FD_SET(breakSock, &ws->readMask);
    ws->highestFd = breakSock;
    return 0;
}


PUBLIC void mprManageSelect(MprWaitService *ws, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ws->handlerMap);

    } else if (flags & MPR_MANAGE_FREE) {
        if (ws->breakSock >= 0) {
            close(ws->breakSock);
            ws->breakSock = 0;
        }
    }
}


PUBLIC int mprNotifyOn(MprWaitService *ws, MprWaitHandler *wp, int mask)
{
    int     fd;

    fd = wp->fd;
    if (fd >= FD_SETSIZE) {
        mprError("File descriptor exceeds configured maximum in FD_SETSIZE (%d vs %d)", fd, FD_SETSIZE);
        return MPR_ERR_CANT_INITIALIZE;
    }
    lock(ws);
    if (wp->desiredMask != mask) {
        if (wp->desiredMask & MPR_READABLE && !(mask & MPR_READABLE)) {
            FD_CLR(fd, &ws->readMask);
        }
        if (wp->desiredMask & MPR_WRITABLE && !(mask & MPR_WRITABLE)) {
            FD_CLR(fd, &ws->writeMask);
        }
        if (mask & MPR_READABLE) {
            FD_SET(fd, &ws->readMask);
        }
        if (mask & MPR_WRITABLE) {
            FD_SET(fd, &ws->writeMask);
        }
        wp->desiredMask = mask;
        ws->highestFd = max(fd, ws->highestFd);
        if (mask == 0 && fd == ws->highestFd) {
            while (--fd > 0) {
                if (FD_ISSET(fd, &ws->readMask) || FD_ISSET(fd, &ws->writeMask)) {
                    break;
                }
            }
            ws->highestFd = fd;
        }
        if (wp->event) {
            mprRemoveEvent(wp->event);
            wp->event = 0;
        }
        mprSetItem(ws->handlerMap, fd, mask ? wp : 0);
    }
    mprWakeNotifier();
    unlock(ws);
    return 0;
}


/*
    Wait for I/O on a single file descriptor. Return a mask of events found. Mask is the events of interest.
    timeout is in milliseconds.
 */
PUBLIC int mprWaitForSingleIO(int fd, int mask, MprTicks timeout)
{
    struct timeval  tval;
    fd_set          readMask, writeMask;
    int             rc, result;

    if (timeout < 0 || timeout > MAXINT) {
        timeout = MAXINT;
    }
    tval.tv_sec = (int) (timeout / 1000);
    tval.tv_usec = (int) ((timeout % 1000) * 1000);

    FD_ZERO(&readMask);
    if (mask & MPR_READABLE) {
        FD_SET(fd, &readMask);
    }
    FD_ZERO(&writeMask);
    if (mask & MPR_WRITABLE) {
        FD_SET(fd, &writeMask);
    }
    result = 0;
    if ((rc = select(fd + 1, &readMask, &writeMask, NULL, &tval)) < 0) {
        mprError("Select returned %d, errno %d", rc, mprGetOsError());

    } else if (rc > 0) {
        if (FD_ISSET(fd, &readMask)) {
            result |= MPR_READABLE;
        }
        if (FD_ISSET(fd, &writeMask)) {
            result |= MPR_WRITABLE;
        }
    }
    return result;
}


/*
    Wait for I/O on all registered file descriptors. Timeout is in milliseconds. Return the number of events detected.
 */
PUBLIC void mprWaitForIO(MprWaitService *ws, MprTicks timeout)
{
    struct timeval  tval;
    fd_set          readMask, writeMask;
    int             rc, maxfd;

    if (timeout < 0 || timeout > MAXINT) {
        timeout = MAXINT;
    }
#if BIT_DEBUG
    if (mprGetDebugMode() && timeout > 30000) {
        timeout = 30000;
    }
#endif
#if VXWORKS
    /* Minimize worst-case VxWorks task starvation */
    timeout = max(timeout, 50);
#endif
    tval.tv_sec = (int) (timeout / 1000);
    tval.tv_usec = (int) ((timeout % 1000) * 1000);

    if (ws->needRecall) {
        mprDoWaitRecall(ws);
        return;
    }
    lock(ws);
    readMask = ws->readMask;
    writeMask = ws->writeMask;
    maxfd = ws->highestFd + 1;
    unlock(ws);

    mprYield(MPR_YIELD_STICKY | MPR_YIELD_NO_BLOCK);
    rc = select(maxfd, &readMask, &writeMask, NULL, &tval);

    mprClearWaiting();
    mprResetYield();

    if (rc > 0) {
        serviceIO(ws, &readMask, &writeMask, maxfd);
    }
    ws->wakeRequested = 0;
}


static void serviceIO(MprWaitService *ws, fd_set *readMask, fd_set *writeMask, int maxfd)
{
    MprWaitHandler      *wp;
    int                 fd, mask;

    lock(ws);
    for (fd = 0; fd < maxfd; fd++) {
        mask = 0;
        if (FD_ISSET(fd, readMask)) {
            mask |= MPR_READABLE;
        }
        if (FD_ISSET(fd, writeMask)) {
            mask |= MPR_WRITABLE;
        }
        if (mask) {
            if (fd == ws->breakSock) {
                readPipe(ws);
                continue;
            }
            if (fd < 0 || (wp = mprGetItem(ws->handlerMap, fd)) == 0) {
                mprError("WARNING: fd not in handler map. fd %d", fd);
                continue;
            }
            wp->presentMask = mask & wp->desiredMask;
            if (wp->presentMask) {
                if (wp->flags & MPR_WAIT_IMMEDIATE) {
                    (wp->proc)(wp->handlerData, NULL);
                } else {
                    mprNotifyOn(ws, wp, 0);
                    mprQueueIOEvent(wp);
                }
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
    ssize           rc;
    int             c;

    ws = MPR->waitService;
    if (!ws->wakeRequested) {
        ws->wakeRequested = 1;
        c = 0;
        rc = sendto(ws->breakSock, (char*) &c, 1, 0, (struct sockaddr*) &ws->breakAddress, (int) sizeof(ws->breakAddress));
        if (rc < 0) {
            static int warnOnce = 0;
            if (warnOnce++ == 0) {
                mprError("Cannot send wakeup to breakout socket: errno %d", errno);
            }
        }
    }
}


static void readPipe(MprWaitService *ws)
{
    char        buf[128];

#if VXWORKS
    int len = sizeof(ws->breakAddress);
    (void) recvfrom(ws->breakSock, buf, (int) sizeof(buf), 0, (struct sockaddr*) &ws->breakAddress, (int*) &len);
#else
    socklen_t   len = sizeof(ws->breakAddress);
    (void) recvfrom(ws->breakSock, buf, (int) sizeof(buf), 0, (struct sockaddr*) &ws->breakAddress, (socklen_t*) &len);
#endif
}

#else
void selectDummy() {}

#endif /* MPR_EVENT_SELECT */

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
