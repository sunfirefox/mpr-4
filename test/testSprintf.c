/**
    testSprintf.c - Unit tests for the Sprintf class

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/
/*
    We support some non-standard PRINTF args
 */
#define PRINTF_ATTRIBUTE(a,b)

#include    "mpr.h"

/************************************ Code ************************************/

static void testBasicSprintf(MprTestGroup *gp)
{
    char    buf[256];
    char    *str;
    int     count;

    fmt(buf, sizeof(buf), "%d", 12345678);
    tassert(strlen(buf) == 8);
    tassert(strcmp(buf, "12345678") == 0);

    fmt(buf, sizeof(buf), "%d", -12345678);
    tassert(strlen(buf) == 9);
    tassert(strcmp(buf, "-12345678") == 0);

    str = sfmt("%d", 12345678);
    count = (int) strlen(str);
    tassert(count == 8);
    tassert(strcmp(str, "12345678") == 0);
}


static void testItos(MprTestGroup *gp)
{
    char    *s;

    s = itos(0);
    tassert(strcmp(s, "0") == 0);

    s = itos(1);
    tassert(strcmp(s, "1") == 0);

    s = itos(-1);
    tassert(strcmp(s, "-1") == 0);

    s = itos(12345678);
    tassert(strcmp(s, "12345678") == 0);

    s = itos(-12345678);
    tassert(strcmp(s, "-12345678") == 0);

    s = itosradix(0x1234, 16);
    tassert(strcmp(s, "1234") == 0);
}


/*
    We need to test quite a bit here. The general format of a sprintf spec is:
 *
        %[modifier][width][precision][bits][type]
 *
    The various character classes are:
        CLASS       Characters      Description
        NORMAL      [All other]     Normal characters
        PERCENT     [%]             Begin format
        MODIFIER    [-+ #,]         Modifiers
        ZERO        [0]             Special modifier
        STAR        [*]             Width supplied by arg
        DIGIT       [1-9]           Field widths
        DOT         [.]             Introduce precision
        BITS        [hlL]           Length bits
        TYPE        [cdfinopsSuxX]  Type specifiers
 */
static void testTypeOptions(MprTestGroup *gp)
{
    char    buf[256];
    int     count;

    fmt(buf, sizeof(buf), "Hello %c World", 'X');
    tassert(strcmp(buf, "Hello X World") == 0);

    fmt(buf, sizeof(buf), "%d", 12345678);
    tassert(strcmp(buf, "12345678") == 0);

    fmt(buf, sizeof(buf), "%3.2f", 1.77);
    tassert(strcmp(buf, "1.77") == 0);

    fmt(buf, sizeof(buf), "%i", 12345678);
    tassert(strcmp(buf, "12345678") == 0);

    fmt(buf, sizeof(buf), "%s%n", "Hello World", &count);
    tassert(count == 11);

    fmt(buf, sizeof(buf), "%o", 077);
    tassert(strcmp(buf, "77") == 0);

    fmt(buf, sizeof(buf), "%p", (void*) 0xdeadbeef);
    tassert(strcmp(buf, "0xdeadbeef") == 0);

    fmt(buf, sizeof(buf), "%s", "Hello World");
    tassert(strcmp(buf, "Hello World") == 0);

    fmt(buf, sizeof(buf), "%u", 0xffffffff);
    tassert(strcmp(buf, "4294967295") == 0);

    fmt(buf, sizeof(buf), "%x", 0xffffffff);
    tassert(strcmp(buf, "ffffffff") == 0);

    fmt(buf, sizeof(buf), "%X", (int64) 0xffffffff);
    tassert(strcmp(buf, "FFFFFFFF") == 0);
}


static void testModifierOptions(MprTestGroup *gp)
{
    char    buf[256];

    fmt(buf, sizeof(buf), "%-4d", 23);
    tassert(strcmp(buf, "23  ") == 0);
    fmt(buf, sizeof(buf), "%-4d", -23);
    tassert(strcmp(buf, "-23 ") == 0);

    fmt(buf, sizeof(buf), "%+4d", 23);
    tassert(strcmp(buf, " +23") == 0);
    fmt(buf, sizeof(buf), "%+4d", -23);
    tassert(strcmp(buf, " -23") == 0);

    fmt(buf, sizeof(buf), "% 4d", 23);
    tassert(strcmp(buf, "  23") == 0);
    fmt(buf, sizeof(buf), "% 4d", -23);
    tassert(strcmp(buf, " -23") == 0);

    fmt(buf, sizeof(buf), "%-+4d", 23);
    tassert(strcmp(buf, "+23 ") == 0);
    fmt(buf, sizeof(buf), "%-+4d", -23);
    tassert(strcmp(buf, "-23 ") == 0);
    fmt(buf, sizeof(buf), "%- 4d", 23);
    tassert(strcmp(buf, " 23 ") == 0);

    fmt(buf, sizeof(buf), "%#6x", 0x23);
    tassert(strcmp(buf, "  0x23") == 0);

    fmt(buf, sizeof(buf), "%,d", 12345678);
    tassert(strcmp(buf, "12,345,678") == 0);
}


static void testWidthOptions(MprTestGroup *gp)
{
    char    buf[256];

    fmt(buf, sizeof(buf), "%2d", 1234);
    tassert(strcmp(buf, "1234") == 0);

    fmt(buf, sizeof(buf), "%8d", 1234);
    tassert(strcmp(buf, "    1234") == 0);

    fmt(buf, sizeof(buf), "%-8d", 1234);
    tassert(strcmp(buf, "1234    ") == 0);

    fmt(buf, sizeof(buf), "%*d", 8, 1234);
    tassert(strcmp(buf, "    1234") == 0);

    fmt(buf, sizeof(buf), "%*d", -8, 1234);
    tassert(strcmp(buf, "1234    ") == 0);
}


static void testPrecisionOptions(MprTestGroup *gp)
{
    char    buf[256];

    fmt(buf, sizeof(buf), "%.2d", 1234);
    tassert(strcmp(buf, "1234") == 0);

    fmt(buf, sizeof(buf), "%.8d", 1234);
    tassert(strcmp(buf, "00001234") == 0);

    fmt(buf, sizeof(buf), "%8.6d", 1234);
    tassert(strcmp(buf, "  001234") == 0);

    fmt(buf, sizeof(buf), "%6.3d", 12345);
    tassert(strcmp(buf, " 12345") == 0);

    fmt(buf, sizeof(buf), "%6.3s", "ABCDEFGHIJ");
    tassert(strcmp(buf, "   ABC") == 0);

    fmt(buf, sizeof(buf), "%6.2f", 12.789);
    tassert(strcmp(buf, " 12.79") == 0);

    fmt(buf, sizeof(buf), "%8.2f", 1234.789);
    tassert(strcmp(buf, " 1234.79") == 0);
}


static void testBitOptions(MprTestGroup *gp)
{
    char    buf[256];

    fmt(buf, sizeof(buf), "%hd %hd", (short) 23, (short) 78);
    tassert(strcmp(buf, "23 78") == 0);

    fmt(buf, sizeof(buf), "%ld %ld", (long) 12, (long) 89);
    tassert(strcmp(buf, "12 89") == 0);

    fmt(buf, sizeof(buf), "%Ld %Ld", (int64) 66, (int64) 41);
    tassert(strcmp(buf, "66 41") == 0);

    fmt(buf, sizeof(buf), "%hd %Ld %hd %Ld", 
        (short) 123, (int64) 789, (short) 441, (int64) 558);
    tassert(strcmp(buf, "123 789 441 558") == 0);
}


static void testSprintf64(MprTestGroup *gp)
{
    char    buf[256];

    fmt(buf, sizeof(buf), "%Ld", INT64(9012345678));
    tassert(strlen(buf) == 10);
    tassert(strcmp(buf, "9012345678") == 0);

    fmt(buf, sizeof(buf), "%Ld", INT64(-9012345678));
    tassert(strlen(buf) == 11);
    tassert(strcmp(buf, "-9012345678") == 0);
}


static void testFloatingSprintf(MprTestGroup *gp)
{
}


/*
    TODO test:
    - decimal
 */
MprTestDef testSprintf = {
    "sprintf", 0, 0, 0,
    {
        MPR_TEST(0, testBasicSprintf),
        MPR_TEST(0, testItos),
        MPR_TEST(0, testTypeOptions),
        MPR_TEST(0, testModifierOptions),
        MPR_TEST(0, testWidthOptions),
        MPR_TEST(0, testPrecisionOptions),
        MPR_TEST(0, testBitOptions),
        MPR_TEST(0, testSprintf64),
        MPR_TEST(0, testFloatingSprintf),
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
