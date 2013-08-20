/**
    posix.c - Posix specific adaptions

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

#if BIT_UNIX_LIKE
/*********************************** Code *************************************/

PUBLIC int mprCreateOsService()
{
    umask(022);

    /*
        Cleanup the environment. IFS is often a security hole
     */
    putenv("IFS=\t ");
    return 0;
}


PUBLIC int mprStartOsService()
{
    /* 
        Open a syslog connection
     */
#if SOLARIS
    openlog(mprGetAppName(), LOG_LOCAL0);
#else
    openlog(mprGetAppName(), 0, LOG_LOCAL0);
#endif
    return 0;
}


PUBLIC void mprStopOsService()
{
    closelog();
}


PUBLIC int mprGetRandomBytes(char *buf, ssize length, bool block)
{
    ssize   sofar, rc;
    int     fd;

    if ((fd = open((block) ? "/dev/random" : "/dev/urandom", O_RDONLY, 0666)) < 0) {
        return MPR_ERR_CANT_OPEN;
    }
    sofar = 0;
    do {
        rc = read(fd, &buf[sofar], length);
        if (rc < 0) {
            assert(0);
            return MPR_ERR_CANT_READ;
        }
        length -= rc;
        sofar += rc;
    } while (length > 0);
    close(fd);
    return 0;
}


#if BIT_HAS_DYN_LOAD && !BIT_STATIC
PUBLIC int mprLoadNativeModule(MprModule *mp)
{
    MprModuleEntry  fn;
    MprPath         info;
    char            *at;
    void            *handle;

    assert(mp);

    /*
        Search the image incase the module has been statically linked
     */
#ifdef RTLD_DEFAULT
    handle = RTLD_DEFAULT;
#else
#ifdef RTLD_MAIN_ONLY
    handle = RTLD_MAIN_ONLY;
#else
    handle = 0;
#endif
#endif
    if (!mp->entry || !dlsym(handle, mp->entry)) {
        if ((at = mprSearchForModule(mp->path)) == 0) {
            mprError("Cannot find module \"%s\", cwd: \"%s\", search path \"%s\"", mp->path, mprGetCurrentPath(),
                mprGetModuleSearchPath());
            return MPR_ERR_CANT_ACCESS;
        }
        mp->path = at;
        mprGetPathInfo(mp->path, &info);
        mp->modified = info.mtime;
        mprLog(2, "Loading native module %s", mprGetPathBase(mp->path));
        if ((handle = dlopen(mp->path, RTLD_LAZY | RTLD_GLOBAL)) == 0) {
            mprError("Cannot load module %s\nReason: \"%s\"", mp->path, dlerror());
            return MPR_ERR_CANT_OPEN;
        } 
        mp->handle = handle;

    } else if (mp->entry) {
        mprLog(2, "Activating native module %s", mp->name);
    }
    if (mp->entry) {
        if ((fn = (MprModuleEntry) dlsym(handle, mp->entry)) != 0) {
            if ((fn)(mp->moduleData, mp) < 0) {
                mprError("Initialization for module %s failed", mp->name);
                dlclose(handle);
                return MPR_ERR_CANT_INITIALIZE;
            }
        } else {
            mprError("Cannot load module %s\nReason: can't find function \"%s\"", mp->path, mp->entry);
            dlclose(handle);
            return MPR_ERR_CANT_READ;
        }
    }
    return 0;
}


PUBLIC int mprUnloadNativeModule(MprModule *mp)
{
    return dlclose(mp->handle);
}
#endif


/*
    This routine does not yield
 */
PUBLIC void mprNap(MprTicks timeout)
{
    MprTicks        remaining, mark;
    struct timespec t;
    int             rc;

    assert(timeout >= 0);

    mark = mprGetTicks();
    remaining = timeout;
    do {
        /* MAC OS X corrupts the timeout if using the 2nd paramater, so recalc each time */
        t.tv_sec = ((int) (remaining / 1000));
        t.tv_nsec = ((int) ((remaining % 1000) * 1000000));
        rc = nanosleep(&t, NULL);
        remaining = mprGetRemainingTicks(mark, timeout);
    } while (rc < 0 && errno == EINTR && remaining > 0);
}


PUBLIC void mprSleep(MprTicks timeout)
{
    mprYield(MPR_YIELD_STICKY);
    mprNap(timeout);
    mprResetYield();
}


/*
    Write a message in the O/S native log (syslog in the case of linux)
 */
PUBLIC void mprWriteToOsLog(cchar *message, int flags, int level)
{
    int     sflag;

    if (flags & MPR_FATAL_MSG) {
        sflag = LOG_ERR;
    } else if (flags & MPR_INFO_MSG) {
        sflag = LOG_WARNING;
    } else if (flags & MPR_ASSERT_MSG) {
        sflag = LOG_WARNING;
    } else if (flags & MPR_ERROR_MSG) {
        sflag = LOG_ERR;
    } else {
        sflag = LOG_WARNING;
    }
    syslog(sflag, "%s", message);
}


PUBLIC int mprInitWindow()
{
    return 0;
}


PUBLIC void mprSetFilesLimit(int limit)
{
    struct rlimit r;
    int           i;

    if (limit == 0 || limit == MAXINT) {
        /*
            We need to determine a reasonable maximum possible limit value.
            There is no #define we can use for this, so we test to determine it empirically
         */
        for (limit = 0x40000000; limit > 0; limit >>= 1) {
            r.rlim_cur = r.rlim_max = limit;
            if (setrlimit(RLIMIT_NOFILE, &r) == 0) {
                for (i = (limit >> 4) * 15; i > 0; i--) {
                    r.rlim_max = r.rlim_cur = limit + i;
                    if (setrlimit(RLIMIT_NOFILE, &r) == 0) {
                        limit = 0;
                        break;
                    }
                }
                break;
            }
        }
    } else {
        r.rlim_cur = r.rlim_max = limit;
        if (setrlimit(RLIMIT_NOFILE, &r) < 0) {
            mprError("Can't set file limit to %d", limit);
        }
    }
    getrlimit(RLIMIT_NOFILE, &r);
    mprTrace(6, "Set files limit to soft %d, max %d", r.rlim_cur, r.rlim_max);
}

#endif /* BIT_UNIX_LIKE */

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
