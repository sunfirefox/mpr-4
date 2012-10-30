/**
    mprModule.c - Dynamic module loading support.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

/********************************** Forwards **********************************/

static void manageModule(MprModule *mp, int flags);
static void manageModuleService(MprModuleService *ms, int flags);

/************************************* Code ***********************************/
/*
    Open the module service
 */
PUBLIC MprModuleService *mprCreateModuleService()
{
    MprModuleService    *ms;

    if ((ms = mprAllocObj(MprModuleService, manageModuleService)) == 0) {
        return 0;
    }
    ms->modules = mprCreateList(-1, 0);
    ms->mutex = mprCreateLock();
    MPR->moduleService = ms;
    mprSetModuleSearchPath(NULL);
    return ms;
}


static void manageModuleService(MprModuleService *ms, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ms->modules);
        mprMark(ms->searchPath);
        mprMark(ms->mutex);
    }
}


/*
    Call the start routine for each module
 */
PUBLIC int mprStartModuleService()
{
    MprModuleService    *ms;
    MprModule           *mp;
    int                 next;

    ms = MPR->moduleService;
    assure(ms);

    for (next = 0; (mp = mprGetNextItem(ms->modules, &next)) != 0; ) {
        if (mprStartModule(mp) < 0) {
            return MPR_ERR_CANT_INITIALIZE;
        }
    }
#if VXWORKS && BIT_DEBUG && SYM_SYNC_INCLUDED
    symSyncLibInit();
#endif
    return 0;
}


PUBLIC void mprStopModuleService()
{
    MprModuleService    *ms;
    MprModule           *mp;
    int                 next;

    ms = MPR->moduleService;
    assure(ms);
    mprLock(ms->mutex);
    for (next = 0; (mp = mprGetNextItem(ms->modules, &next)) != 0; ) {
        mprStopModule(mp);
    }
    mprUnlock(ms->mutex);
}


PUBLIC MprModule *mprCreateModule(cchar *name, cchar *path, cchar *entry, void *data)
{
    MprModuleService    *ms;
    MprModule           *mp;
    int                 index;

    ms = MPR->moduleService;
    assure(ms);

    if ((mp = mprAllocObj(MprModule, manageModule)) == 0) {
        return 0;
    }
    mp->name = sclone(name);
    mp->path = sclone(path);
    if (entry && *entry) {
        mp->entry = sclone(entry);
    }
    mp->moduleData = data;
    mp->lastActivity = mprGetTime();
    index = mprAddItem(ms->modules, mp);
    if (index < 0 || mp->name == 0) {
        return 0;
    }
    return mp;
}


static void manageModule(MprModule *mp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(mp->entry);
        mprMark(mp->name);
        mprMark(mp->path);
        mprMark(mp->moduleData);
    }
}


PUBLIC int mprStartModule(MprModule *mp)
{
    assure(mp);

    if (mp->start && !(mp->flags & MPR_MODULE_STARTED)) {
        if (mp->start(mp) < 0) {
            return MPR_ERR_CANT_INITIALIZE;
        }
    }
    mp->flags |= MPR_MODULE_STARTED;
    return 0;
}


PUBLIC int mprStopModule(MprModule *mp)
{
    assure(mp);

    if (mp->stop && (mp->flags & MPR_MODULE_STARTED) && !(mp->flags & MPR_MODULE_STOPPED)) {
        if (mp->stop(mp) < 0) {
            return MPR_ERR_NOT_READY;
        }
        mp->flags |= MPR_MODULE_STOPPED;
    }
    return 0;
}


/*
    See if a module is already loaded
 */
PUBLIC MprModule *mprLookupModule(cchar *name)
{
    MprModuleService    *ms;
    MprModule           *mp;
    int                 next;

    assure(name && name);

    ms = MPR->moduleService;
    assure(ms);

    for (next = 0; (mp = mprGetNextItem(ms->modules, &next)) != 0; ) {
        assure(mp->name);
        if (mp && strcmp(mp->name, name) == 0) {
            return mp;
        }
    }
    return 0;
}


PUBLIC void *mprLookupModuleData(cchar *name)
{
    MprModule   *module;

    if ((module = mprLookupModule(name)) == NULL) {
        return NULL;
    }
    return module->moduleData;
}


PUBLIC void mprSetModuleTimeout(MprModule *module, MprTime timeout)
{
    assure(module);
    if (module) {
        module->timeout = timeout;
    }
}


PUBLIC void mprSetModuleFinalizer(MprModule *module, MprModuleProc stop)
{
    assure(module);
    if (module) {
        module->stop = stop;
    }
}


PUBLIC void mprSetModuleSearchPath(char *searchPath)
{
    MprModuleService    *ms;

    ms = MPR->moduleService;
    if (searchPath == 0) {
        ms->searchPath = sjoin(mprGetAppDir(), MPR_SEARCH_SEP, mprGetAppDir(), MPR_SEARCH_SEP, BIT_BIN_PREFIX, NULL);
    } else {
        ms->searchPath = sclone(searchPath);
    }
}


PUBLIC cchar *mprGetModuleSearchPath()
{
    return MPR->moduleService->searchPath;
}


/*
    Load a module. The module is located by searching for the filename by optionally using the module search path.
 */
PUBLIC int mprLoadModule(MprModule *mp)
{
#if BIT_HAS_DYN_LOAD
    assure(mp);

    if (mprLoadNativeModule(mp) < 0) {
        return MPR_ERR_CANT_READ;
    }
    mprStartModule(mp);
    return 0;
#else
    mprError("Product built without the ability to load modules dynamically");
    return MPR_ERR_BAD_STATE;
#endif
}


PUBLIC int mprUnloadModule(MprModule *mp)
{
    mprLog(6, "Unloading native module %s from %s", mp->name, mp->path);
    if (mprStopModule(mp) < 0) {
        return MPR_ERR_NOT_READY;
    }
#if BIT_HAS_DYN_LOAD
    if (mp->handle) {
        if (mprUnloadNativeModule(mp) != 0) {
            mprError("Can't unload module %s", mp->name);
        }
        mp->handle = 0;
    }
#endif
    mprRemoveItem(MPR->moduleService->modules, mp);
    return 0;
}


#if BIT_HAS_DYN_LOAD
/*
    Return true if the shared library in "file" can be found. Return the actual path in *path. The filename
    may not have a shared library extension which is typical so calling code can be cross platform.
 */
static char *probe(cchar *filename)
{
    char    *path;

    assure(filename && *filename);

    mprLog(7, "Probe for native module %s", filename);
    if (mprPathExists(filename, R_OK)) {
        return sclone(filename);
    }

    if (strstr(filename, BIT_SHOBJ) == 0) {
        path = sjoin(filename, BIT_SHOBJ, NULL);
        mprLog(7, "Probe for native module %s", path);
        if (mprPathExists(path, R_OK)) {
            return path;
        }
    }
    return 0;
}
#endif


/*
    Search for a module "filename" in the modulePath. Return the result in "result"
 */
PUBLIC char *mprSearchForModule(cchar *filename)
{
#if BIT_HAS_DYN_LOAD
    char    *path, *f, *searchPath, *dir, *tok;

    filename = mprNormalizePath(filename);

    /*
        Search for the path directly
     */
    if ((path = probe(filename)) != 0) {
        mprLog(6, "Found native module %s at %s", filename, path);
        return path;
    }

    /*
        Search in the searchPath
     */
    searchPath = sclone(mprGetModuleSearchPath());
    tok = 0;
    dir = stok(searchPath, MPR_SEARCH_SEP, &tok);
    while (dir && *dir) {
        f = mprJoinPath(dir, filename);
        if ((path = probe(f)) != 0) {
            mprLog(6, "Found native module %s at %s", filename, path);
            return path;
        }
        dir = stok(0, MPR_SEARCH_SEP, &tok);
    }
#endif /* BIT_HAS_DYN_LOAD */
    return 0;
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
