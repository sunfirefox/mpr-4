/**
    async.c - Wait for I/O on Windows.

    This module provides io management for sockets on Windows like systems. 

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

#if MPR_EVENT_ASYNC
/***************************** Forward Declarations ***************************/

static LRESULT msgProc(HWND hwnd, UINT msg, UINT wp, LPARAM lp);

/************************************ Code ************************************/

PUBLIC int mprCreateNotifierService(MprWaitService *ws)
{   
    ws->socketMessage = MPR_SOCKET_MESSAGE;
    return 0;
}


PUBLIC int mprNotifyOn(MprWaitService *ws, MprWaitHandler *wp, int mask)
{
    int     winMask;

    assert(ws->hwnd);

    lock(ws);
    winMask = 0;
    if (wp->desiredMask != mask) {
        if (mask & MPR_READABLE) {
            winMask |= FD_ACCEPT | FD_CONNECT | FD_CLOSE | FD_READ;
        }
        if (mask & MPR_WRITABLE) {
            winMask |= FD_WRITE;
        }
        wp->desiredMask = mask;
        WSAAsyncSelect(wp->fd, ws->hwnd, ws->socketMessage, winMask);
        if (wp->event) {
            mprRemoveEvent(wp->event);
            wp->event = 0;
        }
    }
    unlock(ws);
    return 0;
}


/*
    Wait for I/O on a single descriptor. Return the number of I/O events found. Mask is the events of interest.
    Timeout is in milliseconds.
 */
PUBLIC int mprWaitForSingleIO(int fd, int desiredMask, MprTicks timeout)
{
    HANDLE      h;
    int         winMask;

    if (timeout < 0 || timeout > MAXINT) {
        timeout = MAXINT;
    }
    winMask = 0;
    if (desiredMask & MPR_READABLE) {
        winMask |= FD_CLOSE | FD_READ;
    }
    if (desiredMask & MPR_WRITABLE) {
        winMask |= FD_WRITE;
    }
    h = CreateEvent(NULL, FALSE, FALSE, UT("mprWaitForSingleIO"));
    WSAEventSelect(fd, h, winMask);
    if (WaitForSingleObject(h, (DWORD) timeout) == WAIT_OBJECT_0) {
        CloseHandle(h);
        return desiredMask;
    }
    CloseHandle(h);
    return 0;
}


/*
    Wait for I/O on all registered descriptors. Timeout is in milliseconds. Return the number of events serviced.
    Should only be called by the thread that calls mprServiceEvents
 */
PUBLIC void mprWaitForIO(MprWaitService *ws, MprTicks timeout)
{
    MSG     msg;

    assert(ws->hwnd);

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
    SetTimer(ws->hwnd, 0, (UINT) timeout, NULL);

    mprYield(MPR_YIELD_STICKY | MPR_YIELD_NO_BLOCK);
    if (GetMessage(&msg, NULL, 0, 0) == 0) {
        mprResetYield();
        mprTerminate(MPR_EXIT_DEFAULT, -1);
    } else {
        mprResetYield();
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    ws->wakeRequested = 0;
}


PUBLIC void mprServiceWinIO(MprWaitService *ws, int sockFd, int winMask)
{
    MprWaitHandler      *wp;
    int                 index;

    lock(ws);

    for (index = 0; (wp = (MprWaitHandler*) mprGetNextItem(ws->handlers, &index)) != 0; ) {
        if (wp->fd == sockFd) {
            break;
        }
    }
    if (wp == 0) {
        /* If the server forcibly closed the socket, we may still get a read event. Just ignore it.  */
        unlock(ws);
        return;
    }
    /*
        Mask values: READ==1, WRITE=2, ACCEPT=8, CONNECT=10, CLOSE=20
     */
    wp->presentMask = 0;
    if (winMask & (FD_READ | FD_ACCEPT | FD_CLOSE)) {
        wp->presentMask |= MPR_READABLE;
    }
    if (winMask & (FD_WRITE | FD_CONNECT)) {
        wp->presentMask |= MPR_WRITABLE;
    }
    wp->presentMask &= wp->desiredMask;
    if (wp->presentMask) {
        if (wp->presentMask) {
            mprNotifyOn(ws, wp, 0);
            mprQueueIOEvent(wp);
        }
    }
    unlock(ws);
}


/*
    Wake the wait service. WARNING: This routine must not require locking. MprEvents in scheduleDispatcher depends on this.
 */
PUBLIC void mprWakeNotifier()
{
    MprWaitService  *ws;
   
    ws = MPR->waitService;
    if (!ws->wakeRequested && ws->hwnd) {
        ws->wakeRequested = 1;
        PostMessage(ws->hwnd, WM_NULL, 0, 0L);
    }
}


/*
    Create a default window if the application has not already created one.
 */ 
PUBLIC int mprInitWindow()
{
    MprWaitService  *ws;
    WNDCLASS        wc;
    HWND            hwnd;
	wchar			*name, *title;
    int             rc;

    ws = MPR->waitService;
    if (ws->hwnd) {
        return 0;
    }
	name = (wchar*) wide(mprGetAppName());
	title = (wchar*) wide(mprGetAppTitle());
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW+1);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = 0;
    wc.hIcon            = NULL;
    wc.lpfnWndProc      = (WNDPROC) msgProc;
    wc.lpszMenuName     = wc.lpszClassName = name;

    rc = RegisterClass(&wc);
    if (rc == 0) {
        mprError("Cannot register windows class");
        return MPR_ERR_CANT_INITIALIZE;
    }
    hwnd = CreateWindow(name, title, WS_OVERLAPPED, CW_USEDEFAULT, 0, 0, 0, NULL, NULL, 0, NULL);
    if (!hwnd) {
        mprError("Cannot create window");
        return -1;
    }
    ws->hwnd = hwnd;
    ws->socketMessage = MPR_SOCKET_MESSAGE;
    return 0;
}


/*
    Windows message processing loop for wakeup and socket messages
 */
static LRESULT msgProc(HWND hwnd, UINT msg, UINT wp, LPARAM lp)
{
    MprWaitService      *ws;
    int                 sock, winMask;

    ws = MPR->waitService;

    if (msg == WM_DESTROY || msg == WM_QUIT) {
        mprTerminate(MPR_EXIT_DEFAULT, -1);

    } else if (msg && msg == ws->socketMessage) {
        sock = wp;
        winMask = LOWORD(lp);
        mprServiceWinIO(MPR->waitService, sock, winMask);

    } else if (ws->msgCallback) {
        return ws->msgCallback(hwnd, msg, wp, lp);

    } else {
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}


PUBLIC void mprSetWinMsgCallback(MprMsgCallback callback)
{
    MprWaitService  *ws;

    ws = MPR->waitService;
    ws->msgCallback = callback;
}


#endif /* MPR_EVENT_ASYNC */

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
