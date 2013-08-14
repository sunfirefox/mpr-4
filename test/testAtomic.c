/**
    testAtomic.c - Unit tests for atomic primitives

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

/*********************************** Locals ***********************************/

#define ATOMIC_COUNT    (2000)

static MprMutex atomicLock;

/************************************ Code ************************************/

static int initAtomic(MprTestGroup *gp)
{
    mprInitLock(&atomicLock);
    return 0;
}

static void testAtomicAdd(MprTestGroup *gp)
{
    static int  number = 0;
    int         before, i;

    for (i = 0; i < ATOMIC_COUNT; i++) {
        before = number;
        mprAtomicAdd(&number, 1);
        assert(number > before);
    }
}


static void testAtomicAdd64(MprTestGroup *gp)
{
    static int64    number = 0;
    int64           before;
    int             i;

    for (i = 0; i < ATOMIC_COUNT; i++) {
        before = number;
        mprAtomicAdd64(&number, 1);
        assert(number > before);
    }
}


static void testAtomicCas(MprTestGroup *gp)
{
    char     *ptr;
    char     *before;

    mprLock(&atomicLock);

    /* Should succeed */
    ptr = 0;
    before = ptr;
    assert(mprAtomicCas((void**) &ptr, before, before + 1) == 1);
    assert(ptr == (void*) 1);

    /* Should fail */
    ptr = 0;
    before = (void*) 1;
    assert(mprAtomicCas((void**) &ptr, before, before + 1) == 0);
    assert(ptr == 0);
}


static void testAtomicBarrier(MprTestGroup *gp)
{
    static int  a = 0;
    static int  b = 0;
    int         i;

    /*
        Only one writer to a+b, but many readers below
        Assert: a >= b at all times.
     */
    mprLock(&atomicLock);
    for (i = 0; i < ATOMIC_COUNT; i++) {
        a = a + 1;
        mprAtomicBarrier();
        b = b + 1;
    }
    b = 0;
    mprAtomicBarrier();
    a = 0;
    mprUnlock(&atomicLock);
    /*
        Many readers checking values
     */
    for (i = 0; i < ATOMIC_COUNT; i++) {
        assert(a >= b);
    }
}


MprTestDef testAtomic = {
    "atomic", 0, initAtomic, 0,
    {
        MPR_TEST(0, testAtomicAdd),
        MPR_TEST(0, testAtomicAdd64),
        MPR_TEST(0, testAtomicCas),
        MPR_TEST(0, testAtomicBarrier),
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
