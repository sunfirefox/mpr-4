/**
    testHash.c - Unit tests for the Hash class

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

/*********************************** Locals ***********************************/

#define HASH_COUNT  256                /* Number of items to enter */

/************************************ Code ************************************/

static void testCreateTable(MprTestGroup *gp)
{
    MprHash     *table;

    table = mprCreateHash(211, 0);
    tassert(table != 0);

    table = mprCreateHash(0, 0);
    tassert(table != 0);

    table = mprCreateHash(1, 0);
    tassert(table != 0);
}


static void testIsTableEmpty(MprTestGroup *gp)
{
    MprHash     *table;

    table = mprCreateHash(0, 0);
    tassert(table != 0);

    tassert(mprGetHashLength(table) == 0);
    tassert(mprGetFirstKey(table) == 0);
    tassert(mprLookupKey(table, "") == 0);
}


static void testInsertAndRemoveHash(MprTestGroup *gp)
{
    MprHash     *table;
    MprKey      *sp;
    cchar       *str;
    int         rc;

    table = mprCreateHash(0, MPR_HASH_STATIC_KEYS | MPR_HASH_STATIC_VALUES);
    tassert(table != 0);

    /*
        Single insert
     */
    sp = mprAddKey(table, "Peter", "123 Madison Ave");
    tassert(sp != 0);

    sp = mprGetFirstKey(table);
    tassert(sp != 0);
    tassert(sp->key != 0);
    tassert(strcmp(sp->key, "Peter") == 0);
    tassert(sp->data != 0);
    tassert(strcmp(sp->data, "123 Madison Ave") == 0);

    /*
        Lookup
     */
    str = mprLookupKey(table, "Peter");
    tassert(str != 0);
    tassert(strcmp(str, "123 Madison Ave") == 0);

    rc = mprRemoveKey(table, "Peter");
    tassert(rc == 0);

    tassert(mprGetHashLength(table) == 0);
    sp = mprGetFirstKey(table);
    tassert(sp == 0);

    str = mprLookupKey(table, "Peter");
    tassert(str == 0);
}


static void testHashScale(MprTestGroup *gp)
{
    MprHash     *table;
    MprKey      *sp;
    char        *str;
    char        name[80], *address;
    int         i;

    table = mprCreateHash(HASH_COUNT, 0);
    tassert(mprGetHashLength(table) == 0);

    /*
        All inserts below will insert allocated strings. We must free before
        deleting the table.
     */
    for (i = 0; i < HASH_COUNT; i++) {
        fmt(name, sizeof(name), "name.%d", i);
        address = sfmt("%d Park Ave", i);
        sp = mprAddKey(table, name, address);
        tassert(sp != 0);
    }
    tassert(mprGetHashLength(table) == HASH_COUNT);

    /*
        Check data entered into the hash
     */
    for (i = 0; i < HASH_COUNT; i++) {
        fmt(name, sizeof(name), "name.%d", i);
        str = mprLookupKey(table, name);
        tassert(str != 0);
        address = sfmt("%d Park Ave", i);
        tassert(strcmp(str, address) == 0);
    }
}


static void testIterateHash(MprTestGroup *gp)
{
    MprHash     *table;
    MprKey      *sp;
    char        name[80], address[80];
    cchar       *where;
    int         count, i, check[HASH_COUNT];

    table = mprCreateHash(HASH_COUNT, 0);

    memset(check, 0, sizeof(check));

    /*
        Fill the table
     */
    for (i = 0; i < HASH_COUNT; i++) {
        fmt(name, sizeof(name), "Bit longer name.%d", i);
        fmt(address, sizeof(address), "%d Park Ave", i);
        sp = mprAddKey(table, name, sclone(address));
        tassert(sp != 0);
    }
    tassert(mprGetHashLength(table) == HASH_COUNT);

    /*
        Check data entered into the table
     */
    sp = mprGetFirstKey(table);
    count = 0;
    while (sp) {
        tassert(sp != 0);
        where = sp->data;
        tassert(isdigit((int) where[0]) != 0);
        i = atoi(where);
        check[i] = 1;
        sp = mprGetNextKey(table, sp);
        count++;
    }
    tassert(count == HASH_COUNT);

    count = 0;
    for (i = 0; i < HASH_COUNT; i++) {
        if (check[i]) {
            count++;
        }
    }
    tassert(count == HASH_COUNT);
}


//  TODO -- test caseless and unicode

MprTestDef testHash = {
    "hash", 0, 0, 0,
    {
        MPR_TEST(0, testCreateTable),
        MPR_TEST(0, testIsTableEmpty),
        MPR_TEST(0, testInsertAndRemoveHash),
        MPR_TEST(0, testHashScale),
        MPR_TEST(0, testIterateHash),
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
