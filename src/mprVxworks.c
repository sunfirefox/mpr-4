/**
    mprVxworks.c - Vxworks specific adaptions

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

#if VXWORKS
/*********************************** Code *************************************/

PUBLIC int mprCreateOsService()
{
    return 0;
}


PUBLIC int mprStartOsService()
{
    return 0;
}


PUBLIC void mprStopOsService()
{
}


PUBLIC int access(const char *path, int mode)
{
    struct stat sbuf;
    return stat((char*) path, &sbuf);
}


PUBLIC int mprGetRandomBytes(char *buf, int length, bool block)
{
    int     i;

    for (i = 0; i < length; i++) {
        buf[i] = (char) (mprGetTime() >> i);
    }
    return 0;
}


PUBLIC int mprLoadNativeModule(MprModule *mp)
{
    MprModuleEntry  fn;
    SYM_TYPE        symType;
    MprPath         info;
    char            *at;
    void            *handle;
    int             fd;

    assure(mp);
    fn = 0;
    handle = 0;

    if (!mp->entry || symFindByName(sysSymTbl, mp->entry, (char**) &fn, &symType) == -1) {
        if ((at = mprSearchForModule(mp->path)) == 0) {
            mprError("Cannot find module \"%s\", cwd: \"%s\", search path \"%s\"", mp->path, mprGetCurrentPath(),
                mprGetModuleSearchPath());
            return 0;
        }
        mp->path = at;
        mprGetPathInfo(mp->path, &info);
        mp->modified = info.mtime;

        mprLog(2, "Loading native module %s", mp->path);
        if ((fd = open(mp->path, O_RDONLY, 0664)) < 0) {
            mprError("Cannot open module \"%s\"", mp->path);
            return MPR_ERR_CANT_OPEN;
        }
        handle = loadModule(fd, LOAD_GLOBAL_SYMBOLS);
        if (handle == 0) {
            close(fd);
            if (handle) {
                unldByModuleId(handle, 0);
            }
            mprError("Cannot load module %s", mp->path);
            return MPR_ERR_CANT_READ;
        }
        close(fd);
        mp->handle = handle;

    } else if (mp->entry) {
        mprLog(2, "Activating module %s", mp->name);
    }
    if (mp->entry) {
        if (symFindByName(sysSymTbl, mp->entry, (char**) &fn, &symType) == -1) {
            mprError("Cannot find symbol %s when loading %s", mp->entry, mp->path);
            return MPR_ERR_CANT_READ;
        }
        if ((fn)(mp->moduleData, mp) < 0) {
            mprError("Initialization for %s failed.", mp->path);
            return MPR_ERR_CANT_INITIALIZE;
        }
    }
    return 0;
}


PUBLIC int mprUnloadNativeModule(MprModule *mp)
{
    unldByModuleId((MODULE_ID) mp->handle, 0);
    return 0;
}


PUBLIC void mprNap(MprTicks milliseconds)
{
    struct timespec timeout;
    int             rc;

    assure(milliseconds >= 0);
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_nsec = (milliseconds % 1000) * 1000000;
    do {
        rc = nanosleep(&timeout, &timeout);
    } while (rc < 0 && errno == EINTR);
}


PUBLIC void mprSleep(MprTicks timeout)
{
    mprYield(MPR_YIELD_STICKY);
    mprNap(timeout);
    mprResetYield();
}


PUBLIC void mprWriteToOsLog(cchar *message, int flags, int level)
{
}


PUBLIC uint mprGetpid(void) {
    return taskIdSelf();
}


PUBLIC int fsync(int fd) { 
    return 0; 
}


PUBLIC int ftruncate(int fd, off_t offset) { 
    return 0; 
}


PUBLIC int usleep(uint msec)
{
    struct timespec     timeout;
    int                 rc;

    timeout.tv_sec = msec / (1000 * 1000);
    timeout.tv_nsec = msec % (1000 * 1000) * 1000;
    do {
        rc = nanosleep(&timeout, &timeout);
    } while (rc < 0 && errno == EINTR);
    return 0;
}


PUBLIC int mprInitWindow()
{
    return 0;
}


/*
    Create a routine to pull in the GCC support routines for double and int64 manipulations for some platforms. Do this
    incase modules reference these routines. Without this, the modules have to reference them. Which leads to multiple 
    defines if two modules include them. (Code to pull in moddi3, udivdi3, umoddi3)
 */
double  __mpr_floating_point_resolution(double a, double b, int64 c, int64 d, uint64 e, uint64 f) {
    a = a / b; a = a * b; c = c / d; c = c % d; e = e / f; e = e % f;
    c = (int64) a; d = (uint64) a; a = (double) c; a = (double) e;
    return (a == b) ? a : b;
}


#endif /* VXWORKS */

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
