/**
    win.c - Windows specific adaptions

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

#if CYGWIN
 #include "w32api/windows.h"
#endif

#if BIT_WIN_LIKE && !WINCE
/*********************************** Code *************************************/
/*
    Initialize the O/S platform layer
 */ 

PUBLIC int mprCreateOsService()
{
    WSADATA     wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return -1;
    }
    return 0;
}


PUBLIC int mprStartOsService()
{
    return 0;
}


PUBLIC void mprStopOsService()
{
    WSACleanup();
}


PUBLIC long mprGetInst()
{
    return (long) MPR->appInstance;
}


PUBLIC HWND mprGetHwnd()
{
    return MPR->waitService->hwnd;
}


PUBLIC int mprGetRandomBytes(char *buf, ssize length, bool block)
{
    HCRYPTPROV      prov;
    int             rc;

    rc = 0;
    if (!CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | 0x40)) {
        return mprGetError();
    }
    if (!CryptGenRandom(prov, (wsize) length, buf)) {
        rc = mprGetError();
    }
    CryptReleaseContext(prov, 0);
    return rc;
}


#if !BIT_STATIC
PUBLIC int mprLoadNativeModule(MprModule *mp)
{
    MprModuleEntry  fn;
    MprPath         info;
    char            *at, *baseName;
    void            *handle;

    assert(mp);

    if ((handle = (HANDLE) MPR->appInstance) == 0) {
        handle = GetModuleHandle(NULL);
    }
    if (!handle || !mp->entry || !GetProcAddress(handle, mp->entry)) {
        if ((at = mprSearchForModule(mp->path)) == 0) {
            mprError("Cannot find module \"%s\", cwd: \"%s\", search path \"%s\"", mp->path, mprGetCurrentPath(),
                mprGetModuleSearchPath());
            return MPR_ERR_CANT_ACCESS;
        }
        mp->path = at;
        mprGetPathInfo(mp->path, &info);
        mp->modified = info.mtime;
        baseName = mprGetPathBase(mp->path);
        mprLog(2, "Loading native module %s", baseName);
        if ((handle = GetModuleHandle(wide(baseName))) == 0 && (handle = LoadLibrary(wide(mp->path))) == 0) {
            mprError("Cannot load module %s\nReason: \"%d\"\n", mp->path, mprGetOsError());
            return MPR_ERR_CANT_READ;
        } 
        mp->handle = handle;

    } else if (mp->entry) {
        mprLog(2, "Activating native module %s", mp->name);
    }
    if (mp->entry) {
        if ((fn = (MprModuleEntry) GetProcAddress((HINSTANCE) handle, mp->entry)) == 0) {
            mprError("Cannot load module %s\nReason: can't find function \"%s\"\n", mp->name, mp->entry);
            FreeLibrary((HINSTANCE) handle);
            return MPR_ERR_CANT_ACCESS;
        }
        if ((fn)(mp->moduleData, mp) < 0) {
            mprError("Initialization for module %s failed", mp->name);
            FreeLibrary((HINSTANCE) handle);
            return MPR_ERR_CANT_INITIALIZE;
        }
    }
    return 0;
}


PUBLIC int mprUnloadNativeModule(MprModule *mp)
{
    assert(mp->handle);

    if (FreeLibrary((HINSTANCE) mp->handle) == 0) {
        return MPR_ERR_ABORTED;
    }
    return 0;
}
#endif /* !BIT_STATIC */


PUBLIC void mprSetInst(HINSTANCE inst)
{
    MPR->appInstance = inst;
}


PUBLIC void mprSetHwnd(HWND h)
{
    MPR->waitService->hwnd = h;
}


PUBLIC void mprSetSocketMessage(int socketMessage)
{
    MPR->waitService->socketMessage = socketMessage;
}


PUBLIC void mprNap(MprTicks timeout)
{
    Sleep((int) timeout);
}


PUBLIC void mprSleep(MprTicks timeout)
{
    mprYield(MPR_YIELD_STICKY);
    mprNap(timeout);
    mprResetYield();
}


PUBLIC void mprWriteToOsLog(cchar *message, int flags, int level)
{
    HKEY        hkey;
    void        *event;
    long        errorType;
    ulong       exists;
    char        buf[BIT_MAX_BUFFER], logName[BIT_MAX_BUFFER], *cp, *value;
    wchar       *lines[9];
    int         type;
    static int  once = 0;

    scopy(buf, sizeof(buf), message);
    cp = &buf[slen(buf) - 1];
    while (*cp == '\n' && cp > buf) {
        *cp-- = '\0';
    }
    type = EVENTLOG_ERROR_TYPE;
    lines[0] = wide(buf);
    lines[1] = 0;
    lines[2] = lines[3] = lines[4] = lines[5] = 0;
    lines[6] = lines[7] = lines[8] = 0;

    if (once == 0) {
        /*  Initialize the registry */
        once = 1;
        fmt(logName, sizeof(logName), "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s", mprGetAppName());
        hkey = 0;

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, wide(logName), 0, NULL, 0, KEY_ALL_ACCESS, NULL, 
                &hkey, &exists) == ERROR_SUCCESS) {
            value = "%SystemRoot%\\System32\\netmsg.dll";
            if (RegSetValueEx(hkey, UT("EventMessageFile"), 0, REG_EXPAND_SZ, 
                    (uchar*) value, (int) slen(value) + 1) != ERROR_SUCCESS) {
                RegCloseKey(hkey);
                return;
            }
            errorType = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
            if (RegSetValueEx(hkey, UT("TypesSupported"), 0, REG_DWORD, (uchar*) &errorType, 
                    sizeof(DWORD)) != ERROR_SUCCESS) {
                RegCloseKey(hkey);
                return;
            }
            RegCloseKey(hkey);
        }
    }

    event = RegisterEventSource(0, wide(mprGetAppName()));
    if (event) {
        /*
            3299 is the event number for the generic message in netmsg.dll.
            "%1 %2 %3 %4 %5 %6 %7 %8 %9" -- thanks Apache for the tip
         */
        ReportEvent(event, EVENTLOG_ERROR_TYPE, 0, 3299, NULL, sizeof(lines) / sizeof(wchar*), 0, lines, 0);
        DeregisterEventSource(event);
    }
}


#endif /* BIT_WIN_LIKE */


#if (BIT_WIN_LIKE && !WINCE) || CYGWIN
/*
    Determine the registry hive by the first portion of the path. Return 
    a pointer to the rest of key path after the hive portion.
 */ 
static cchar *getHive(cchar *keyPath, HKEY *hive)
{
    char    key[BIT_MAX_PATH], *cp;
    ssize   len;

    assert(keyPath && *keyPath);

    *hive = 0;

    scopy(key, sizeof(key), keyPath);
    key[sizeof(key) - 1] = '\0';

    if ((cp = schr(key, '\\')) != 0) {
        *cp++ = '\0';
    }
    if (cp == 0 || *cp == '\0') {
        return 0;
    }
    if (!scaselesscmp(key, "HKEY_LOCAL_MACHINE") || !scaselesscmp(key, "HKLM")) {
        *hive = HKEY_LOCAL_MACHINE;
    } else if (!scaselesscmp(key, "HKEY_CURRENT_USER") || !scaselesscmp(key, "HKCU")) {
        *hive = HKEY_CURRENT_USER;
    } else if (!scaselesscmp(key, "HKEY_USERS")) {
        *hive = HKEY_USERS;
    } else if (!scaselesscmp(key, "HKEY_CLASSES_ROOT")) {
        *hive = HKEY_CLASSES_ROOT;
    } else {
        return 0;
    }
    if (*hive == 0) {
        return 0;
    }
    len = slen(key) + 1;
    return keyPath + len;
}


PUBLIC char *mprReadRegistry(cchar *key, cchar *name)
{
    HKEY        top, h;
    char        *value;
    ulong       type, size;

    assert(key && *key);

    /*
        Get the registry hive
     */
    if ((key = getHive(key, &top)) == 0) {
        return 0;
    }
    if (RegOpenKeyEx(top, wide(key), 0, KEY_READ, &h) != ERROR_SUCCESS) {
        return 0;
    }

    /*
        Get the type
     */
    if (RegQueryValueEx(h, wide(name), 0, &type, 0, &size) != ERROR_SUCCESS) {
        RegCloseKey(h);
        return 0;
    }
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        RegCloseKey(h);
        return 0;
    }
    if ((value = mprAlloc(size + 1)) == 0) {
        return 0;
    }
    if (RegQueryValueEx(h, wide(name), 0, &type, (uchar*) value, &size) != ERROR_SUCCESS) {
        RegCloseKey(h);
        return 0;
    }
    RegCloseKey(h);
    value[size] = '\0';
    return value;
}

PUBLIC int mprWriteRegistry(cchar *key, cchar *name, cchar *value)
{
    HKEY    top, h, subHandle;
    ulong   disposition;

    assert(key && *key);
    assert(value && *value);

    /*
        Get the registry hive
     */
    if ((key = getHive(key, &top)) == 0) {
        return MPR_ERR_CANT_ACCESS;
    }
    if (name && *name) {
        /*
            Write a registry string value
         */
        if (RegOpenKeyEx(top, wide(key), 0, KEY_ALL_ACCESS, &h) != ERROR_SUCCESS) {
            return MPR_ERR_CANT_ACCESS;
        }
        if (RegSetValueEx(h, wide(name), 0, REG_SZ, (uchar*) value, (int) slen(value) + 1) != ERROR_SUCCESS) {
            RegCloseKey(h);
            return MPR_ERR_CANT_READ;
        }

    } else {
        /*
            Create a new sub key
         */
        if (RegOpenKeyEx(top, wide(key), 0, KEY_CREATE_SUB_KEY, &h) != ERROR_SUCCESS){
            return MPR_ERR_CANT_ACCESS;
        }
        if (RegCreateKeyEx(h, wide(value), 0, NULL, REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS, NULL, &subHandle, &disposition) != ERROR_SUCCESS) {
            return MPR_ERR_CANT_ACCESS;
        }
        RegCloseKey(subHandle);
    }
    RegCloseKey(h);
    return 0;
}


#else
void winDummy() {}
#endif /* (BIT_WIN_LIKE && !WINCE) || CYGWIN */

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
