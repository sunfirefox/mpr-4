/**
    cond.c - Thread Conditional variables

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#include    "mpr.h"

/***************************** Forward Declarations ***************************/

static void manageCond(MprCond *cp, int flags);

/************************************ Code ************************************/
/*
    Create a condition variable for use by single or multiple waiters
 */

PUBLIC MprCond *mprCreateCond()
{
    MprCond     *cp;

    if ((cp = mprAllocObj(MprCond, manageCond)) == 0) {
        return 0;
    }
    cp->triggered = 0;
    cp->mutex = mprCreateLock();

#if BIT_WIN_LIKE
    cp->cv = CreateEvent(NULL, FALSE, FALSE, NULL);
#elif VXWORKS
    cp->cv = semCCreate(SEM_Q_PRIORITY, SEM_EMPTY);
#else
    pthread_cond_init(&cp->cv, NULL);
#endif
    return cp;
}


static void manageCond(MprCond *cp, int flags)
{
    assert(cp);

    if (flags & MPR_MANAGE_MARK) {
        mprMark(cp->mutex);

    } else if (flags & MPR_MANAGE_FREE) {
        assert(cp->mutex);
#if BIT_WIN_LIKE
        CloseHandle(cp->cv);
#elif VXWORKS
        semDelete(cp->cv);
#else
        pthread_cond_destroy(&cp->cv);
#endif
    }
}


/*
    Wait for the event to be triggered. Should only be used when there are single waiters. If the event is already
    triggered, then it will return immediately. Timeout of -1 means wait forever. Timeout of 0 means no wait.
    Returns 0 if the event was signalled. Returns < 0 for a timeout.

    WARNING: On unix, the pthread_cond_timedwait uses an absolute time (Ugh!). So time-warps for daylight-savings may
    cause waits to prematurely return.
 */
PUBLIC int mprWaitForCond(MprCond *cp, MprTicks timeout)
{
    MprTicks            now, expire;
    int                 rc;
#if BIT_UNIX_LIKE
    struct timespec     waitTill;
    struct timeval      current;
    int                 usec;
#endif

    /*
        Avoid doing a mprGetTicks() if timeout is < 0
     */
    rc = 0;
    if (timeout >= 0) {
        now = mprGetTicks();
        expire = now + timeout;
#if BIT_UNIX_LIKE
        gettimeofday(&current, NULL);
        usec = current.tv_usec + ((int) (timeout % 1000)) * 1000;
        waitTill.tv_sec = current.tv_sec + ((int) (timeout / 1000)) + (usec / 1000000);
        waitTill.tv_nsec = (usec % 1000000) * 1000;
#endif
    } else {
        expire = -1;
        now = 0;
    }
    mprLock(cp->mutex);
    /*
        NOTE: The WaitForSingleObject and semTake APIs keeps state as to whether the object is signalled.
        WaitForSingleObject and semTake will not block if the object is already signalled. However, pthread_cond_ 
        is different and does not keep such state. If it is signalled before pthread_cond_wait, the thread will 
        still block. Consequently we need to keep our own state in cp->triggered. This also protects against 
        spurious wakeups which can happen (on windows).
     */
    do {
#if BIT_WIN_LIKE
        /*
            Regardless of the state of cp->triggered, we must call WaitForSingleObject to consume the signalled
            internal state of the object.
         */
        mprUnlock(cp->mutex);
        rc = WaitForSingleObject(cp->cv, (int) (expire - now));
        mprLock(cp->mutex);
        if (rc == WAIT_OBJECT_0) {
            rc = 0;
            ResetEvent(cp->cv);
        } else if (rc == WAIT_TIMEOUT) {
            rc = MPR_ERR_TIMEOUT;
        } else {
            rc = MPR_ERR;
        }
#elif VXWORKS
        /*
            Regardless of the state of cp->triggered, we must call semTake to consume the semaphore signalled state
         */
        mprUnlock(cp->mutex);
        rc = semTake(cp->cv, (int) (expire - now));
        mprLock(cp->mutex);
        if (rc != 0) {
            if (errno == S_objLib_OBJ_UNAVAILABLE) {
                rc = MPR_ERR_TIMEOUT;
            } else {
                rc = MPR_ERR;
            }
        }

#elif BIT_UNIX_LIKE
        /*
            NOTE: pthread_cond_timedwait can return 0 (MAC OS X and Linux). The pthread_cond_wait routines will 
            atomically unlock the mutex before sleeping and will relock on awakening.
         */
        if (!cp->triggered) {
            if (now) {
                rc = pthread_cond_timedwait(&cp->cv, &cp->mutex->cs,  &waitTill);
            } else {
                rc = pthread_cond_wait(&cp->cv, &cp->mutex->cs);
            }
            if (rc == ETIMEDOUT) {
                rc = MPR_ERR_TIMEOUT;
            } else if (rc == EAGAIN) {
                rc = 0;
            } else if (rc != 0) {
                assert(rc == 0);
                mprError("pthread_cond_timedwait error rc %d", rc);
                rc = MPR_ERR;
            }
        }
#endif
    } while (!cp->triggered && rc == 0 && (now && (now = mprGetTicks()) < expire));

    if (cp->triggered) {
        cp->triggered = 0;
        rc = 0;
    } else if (rc == 0) {
        rc = MPR_ERR_TIMEOUT;
    }
    mprUnlock(cp->mutex);
    return rc;
}


/*
    Signal a condition and wakeup the waiter. Note: this may be called prior to the waiter waiting.
 */
PUBLIC void mprSignalCond(MprCond *cp)
{
    mprLock(cp->mutex);
    if (!cp->triggered) {
        cp->triggered = 1;
#if BIT_WIN_LIKE
        SetEvent(cp->cv);
#elif VXWORKS
        semGive(cp->cv);
#else
        pthread_cond_signal(&cp->cv);
#endif
    }
    mprUnlock(cp->mutex);
}


PUBLIC void mprResetCond(MprCond *cp)
{
    mprLock(cp->mutex);
    cp->triggered = 0;
#if BIT_WIN_LIKE
    ResetEvent(cp->cv);
#elif VXWORKS
    semDelete(cp->cv);
    cp->cv = semCCreate(SEM_Q_PRIORITY, SEM_EMPTY);
#else
    pthread_cond_destroy(&cp->cv);
    pthread_cond_init(&cp->cv, NULL);
#endif
    mprUnlock(cp->mutex);
}


/*
    Wait for the event to be triggered when there may be multiple waiters. This routine may return early due to
    other signals or events. The caller must verify if the signalled condition truly exists. If the event is already
    triggered, then it will return immediately. This call will not reset cp->triggered and must be reset manually.
    A timeout of -1 means wait forever. Timeout of 0 means no wait.  Returns 0 if the event was signalled. 
    Returns < 0 for a timeout.

    WARNING: On unix, the pthread_cond_timedwait uses an absolute time (Ugh!). So time-warps for daylight-savings may
    cause waits to prematurely return.
 */
PUBLIC int mprWaitForMultiCond(MprCond *cp, MprTicks timeout)
{
    int         rc;
#if BIT_UNIX_LIKE
    struct timespec     waitTill;
    struct timeval      current;
    int                 usec;
#else
    MprTicks            now, expire;
#endif

    if (timeout < 0) {
        timeout = MAXINT;
    }
#if BIT_UNIX_LIKE
    gettimeofday(&current, NULL);
    usec = current.tv_usec + ((int) (timeout % 1000)) * 1000;
    waitTill.tv_sec = current.tv_sec + ((int) (timeout / 1000)) + (usec / 1000000);
    waitTill.tv_nsec = (usec % 1000000) * 1000;
#else
    now = mprGetTicks();
    expire = now + timeout;
#endif

#if BIT_WIN_LIKE
    rc = WaitForSingleObject(cp->cv, (int) (expire - now));
    if (rc == WAIT_OBJECT_0) {
        rc = 0;
    } else if (rc == WAIT_TIMEOUT) {
        rc = MPR_ERR_TIMEOUT;
    } else {
        rc = MPR_ERR;
    }
#elif VXWORKS
    rc = semTake(cp->cv, (int) (expire - now));
    if (rc != 0) {
        if (errno == S_objLib_OBJ_UNAVAILABLE) {
            rc = MPR_ERR_TIMEOUT;
        } else {
            rc = MPR_ERR;
        }
    }
#elif BIT_UNIX_LIKE
    mprLock(cp->mutex);
    rc = pthread_cond_timedwait(&cp->cv, &cp->mutex->cs,  &waitTill);
    if (rc == ETIMEDOUT) {
        rc = MPR_ERR_TIMEOUT;
    } else if (rc != 0) {
        assert(rc == 0);
        rc = MPR_ERR;
    }
    mprUnlock(cp->mutex);
#endif
    return rc;
}


/*
    Signal a condition and wakeup the all the waiters. Note: this may be called before or after to the waiter waiting.
 */
PUBLIC void mprSignalMultiCond(MprCond *cp)
{
    mprLock(cp->mutex);
#if BIT_WIN_LIKE
    /* Pulse event */
    SetEvent(cp->cv);
    ResetEvent(cp->cv);
#elif VXWORKS
    /* Reset sem count and then give once. Prevents accumulation */
    while (semTake(cp->cv, 0) == OK) ;
    semGive(cp->cv);
    semFlush(cp->cv);
#else
    pthread_cond_broadcast(&cp->cv);
#endif
    mprUnlock(cp->mutex);
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
