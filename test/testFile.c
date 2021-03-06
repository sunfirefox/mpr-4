/**
    testFile.c - Unit tests for the mprFile module
  
    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

/*********************************** Locals **********************************/

#define NAME        ".testFile.tmp"
#define FILEMODE    0644

typedef struct TestFile {
    char        *name;
} TestFile;

static char *makePath(cchar *name);
static void manageTestFile(TestFile *tf, int flags);

/************************************ Code ************************************/
/*
    Initialization for this test module
 */
static int initFile(MprTestGroup *gp)
{
    TestFile     *ts;

    gp->data = mprAllocObj(TestFile, manageTestFile);
    if (gp->data == 0) {
        return MPR_ERR_MEMORY;
    }
    ts = (TestFile*) gp->data;

    ts->name = makePath(NAME);
    if (ts->name == 0) {
        gp->data = 0;
        return MPR_ERR_MEMORY;
    }

    /*
        Don't mind if these fail. We are just making sure they don't exist before we start the tests.
     */
    mprDeletePath(ts->name);
    return 0;
}


static void manageTestFile(TestFile *tf, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(tf->name);
    }
}


static int termFile(MprTestGroup *gp)
{
    return 0;
}


static void testBasicIO(MprTestGroup *gp)
{
    MprFile         *file;
    MprPath         info;
    MprOff          pos;
    ssize           len, rc;
    char            buf[512];
    TestFile        *ts;

    ts = (TestFile*) gp->data;
    
    rc = mprDeletePath(ts->name);
    tassert(!mprPathExists(ts->name, R_OK));
    
    file = mprOpenFile(ts->name, O_CREAT | O_TRUNC | O_RDWR, FILEMODE);
    tassert(file != 0);
    tassert(mprPathExists(ts->name, R_OK));

    len = mprWriteFile(file, "abcdef", 6);
    tassert(len == 6);
    mprCloseFile(file);

    tassert(mprPathExists(ts->name, R_OK));
    rc = mprGetPathInfo(ts->name, &info);
    tassert(rc == 0);

#if BIT_WIN_LIKE
    /*
        On windows, the size reflects the size on disk and is not updated until actually save to disk.
     */
#else
    tassert(info.size == 6);
    tassert(!info.isDir);
    tassert(info.isReg);
#endif
    
    file = mprOpenFile(ts->name, O_RDWR, FILEMODE);
    tassert(file != 0);

    pos = mprSeekFile(file, SEEK_SET, 1);
    len = mprReadFile(file, buf, sizeof(buf));
    tassert(len == 5);
    buf[len] = '\0';
    
    tassert(strcmp(buf, "bcdef") == 0);

    pos = mprSeekFile(file, SEEK_SET, 0);
    tassert(pos == 0);
    tassert(mprGetFilePosition(file) == 0);

    len = mprWriteFileString(file, "Hello\nWorld\n");
    tassert(len == 12);
    mprSeekFile(file, SEEK_SET, 0);

    rc = mprReadFile(file, buf, sizeof(buf));
    tassert(rc == 12);
    tassert(mprGetFilePosition(file) == 12);
    buf[12] = '\0';
    tassert(strcmp(buf, "Hello\nWorld\n") == 0);
    mprCloseFile(file);

    rc = mprDeletePath(ts->name);
    tassert(!mprPathExists(ts->name, R_OK));
}
    

static void testBufferedIO(MprTestGroup *gp)
{
    MprFile         *file;
    MprPath         info;
    TestFile        *ts;
    ssize           len;
    char            *str;
    int             rc, c;

    ts = (TestFile*) gp->data;
    
    rc = mprDeletePath(ts->name);
    tassert(!mprPathExists(ts->name, R_OK));
    
    file = mprOpenFile(ts->name, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, FILEMODE);
    if (file == 0) {
        tassert(file != 0);
        return;
    }
    tassert(mprPathExists(ts->name, R_OK));
    
    mprEnableFileBuffering(file, 0, 512);
    
    len = mprWriteFile(file, "abc", 3);
    tassert(len == 3);
    
    len = mprPutFileChar(file, 'd');
    tassert(len == 1);
    
    len = mprPutFileString(file, "ef\n");
    tassert(len == 3);
    
    tassert(mprPathExists(ts->name, R_OK));
    
    /*
        No data flushed yet so the length should be zero
     */
    rc = mprGetPathInfo(ts->name, &info);
    tassert(rc == 0);
    tassert(info.size == 0);
    
    rc = mprFlushFile(file);
    tassert(rc == 0);
    mprCloseFile(file);
    
    /*
        Now the length should be set
     */
    rc = mprGetPathInfo(ts->name, &info);
    tassert(rc == 0);

    /*
        TODO windows seems to delay setting this
     */
    if (info.size != 7) {
        mprSleep(2000);
        rc = mprGetPathInfo(ts->name, &info);
    }
    tassert(info.size == 7);

    file = mprOpenFile(ts->name, O_RDONLY | O_BINARY, FILEMODE);
    tassert(file != 0);
    
    mprSeekFile(file, SEEK_SET, 0);
    tassert(mprPeekFileChar(file) == 'a');
    c = mprGetFileChar(file);
    tassert(c == 'a');
    str = mprReadLine(file, 0, NULL);
    tassert(str != 0);
    len = strlen(str);
    tassert(len == 5);   
    tassert(strcmp(str, "bcdef") == 0);
    mprCloseFile(file);

    mprDeletePath(ts->name);
    tassert(!mprPathExists(ts->name, R_OK));
}
    

/*
    Make a unique filename for a given thread
 */
static char *makePath(cchar *name)
{
    char    *path;

    if ((path = sfmt("%s-%d-%s", name, getpid(), mprGetCurrentThreadName())) == 0) {
        return 0;
    }
    return path;
}


MprTestDef testFile = {
    "file", 0, initFile, termFile,
    {
        MPR_TEST(0, testBasicIO),
        MPR_TEST(0, testBufferedIO),
        MPR_TEST(0, 0),
    },
};


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
