/**
    mprMixed.c - Mixed mode strings. Unicode results with ascii args.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

#if BIT_CHAR_LEN > 1 && KEEP
/********************************** Forwards **********************************/

PUBLIC int mcaselesscmp(wchar *str1, cchar *str2)
{
    return mncaselesscmp(str1, str2, -1);
}


PUBLIC int mcmp(wchar *s1, cchar *s2)
{
    return mncmp(s1, s2, -1);
}


PUBLIC wchar *mncontains(wchar *str, cchar *pattern, ssize limit)
{
    wchar   *cp, *s1;
    cchar   *s2;
    ssize   lim;

    assure(0 <= limit && limit < MAXSSIZE);

    if (limit < 0) {
        limit = MAXINT;
    }
    if (str == 0) {
        return 0;
    }
    if (pattern == 0 || *pattern == '\0') {
        return (wchar*) str;
    }
    for (cp = str; *cp && limit > 0; cp++, limit--) {
        s1 = cp;
        s2 = pattern;
        for (lim = limit; *s1 && *s2 && (*s1 == (uchar) *s2) && lim > 0; lim--) {
            s1++;
            s2++;
        }
        if (*s2 == '\0') {
            return cp;
        }
    }
    return 0;
}


PUBLIC wchar *mcontains(wchar *str, cchar *pattern)
{
    return mncontains(str, pattern, -1);
}


/*
    destMax and len are character counts, not sizes in bytes
 */
PUBLIC ssize mcopy(wchar *dest, ssize destMax, cchar *src)
{
    ssize       len;

    assure(src);
    assure(dest);
    assure(0 < destMax && destMax < MAXINT);

    len = slen(src);
    if (destMax <= len) {
        assure(!MPR_ERR_WONT_FIT);
        return MPR_ERR_WONT_FIT;
    }
    return mtow(dest, len + 1, src, len);
}


PUBLIC int mends(wchar *str, cchar *suffix)
{
    wchar   *cp;
    cchar   *sp;

    if (str == NULL || suffix == NULL) {
        return 0;
    }
    cp = &str[wlen(str) - 1];
    sp = &suffix[slen(suffix)];
    for (; cp > str && sp > suffix; ) {
        if (*cp-- != *sp--) {
            return 0;
        }
    }
    if (sp > suffix) {
        return 0;
    }
    return 1;
}


PUBLIC wchar *mfmt(cchar *fmt, ...)
{
    va_list     ap;
    char        *mresult;

    assure(fmt);

    va_start(ap, fmt);
    mresult = sfmtv(fmt, ap);
    va_end(ap);
    return amtow(mresult, NULL);
}


PUBLIC wchar *mfmtv(cchar *fmt, va_list arg)
{
    char    *mresult;

    assure(fmt);
    mresult = sfmtv(fmt, arg);
    return amtow(mresult, NULL);
}


/*
    Sep is ascii, args are wchar
 */
PUBLIC wchar *mjoin(wchar *str, ...)
{
    wchar       *result;
    va_list     ap;

    assure(str);

    va_start(ap, str);
    result = mjoinv(str, ap);
    va_end(ap);
    return result;
}


/*
    MOB - comment required. What does this do?
 */
PUBLIC wchar *mjoinv(wchar *buf, va_list args)
{
    va_list     ap;
    wchar       *dest, *str, *dp;
    int         required, len;

    assure(buf);

    va_copy(ap, args);
    required = 1;
    if (buf) {
        required += wlen(buf);
    }
    str = va_arg(ap, wchar*);
    while (str) {
        required += wlen(str);
        str = va_arg(ap, wchar*);
    }
    if ((dest = mprAlloc(required)) == 0) {
        return 0;
    }
    dp = dest;
    if (buf) {
        wcopy(dp, -1, buf);
        dp += wlen(buf);
    }
    va_copy(ap, args);
    str = va_arg(ap, wchar*);
    while (str) {
        wcopy(dp, required, str);
        len = wlen(str);
        dp += len;
        required -= len;
        str = va_arg(ap, wchar*);
    }
    *dp = '\0';
    return dest;
}


/*
    Case insensitive string comparison. Limited by length
 */
PUBLIC int mncaselesscmp(wchar *s1, cchar *s2, ssize n)
{
    int     rc;

    assure(0 <= n && n < MAXSSIZE);

    if (s1 == 0 || s2 == 0) {
        return -1;
    } else if (s1 == 0) {
        return -1;
    } else if (s2 == 0) {
        return 1;
    }
    for (rc = 0; n > 0 && *s1 && rc == 0; s1++, s2++, n--) {
        rc = tolower((uchar) *s1) - tolower((uchar) *s2);
    }
    if (rc) {
        return (rc > 0) ? 1 : -1;
    } else if (n == 0) {
        return 0;
    } else if (*s1 == '\0' && *s2 == '\0') {
        return 0;
    } else if (*s1 == '\0') {
        return -1;
    } else if (*s2 == '\0') {
        return 1;
    }
    return 0;
}



PUBLIC int mncmp(wchar *s1, cchar *s2, ssize n)
{
    assure(0 <= n && n < MAXSSIZE);

    if (s1 == 0 && s2 == 0) {
        return 0;
    } else if (s1 == 0) {
        return -1;
    } else if (s2 == 0) {
        return 1;
    }
    for (rc = 0; n > 0 && *s1 && rc == 0; s1++, s2++, n--) {
        rc = *s1 - (uchar) *s2;
    }
    if (rc) {
        return (rc > 0) ? 1 : -1;
    } else if (n == 0) {
        return 0;
    } else if (*s1 == '\0' && *s2 == '\0') {
        return 0;
    } else if (*s1 == '\0') {
        return -1;
    } else if (*s2 == '\0') {
        return 1;
    }
    return 0;
}


PUBLIC ssize mncopy(wchar *dest, ssize destMax, cchar *src, ssize len)
{
    assure(0 <= len && len < MAXSSIZE);
    assure(0 < destMax && destMax < MAXSSIZE);

    return mtow(dest, destMax, src, len);
}


PUBLIC wchar *mpbrk(wchar *str, cchar *set)
{
    cchar   *sp;
    int     count;

    if (str == NULL || set == NULL) {
        return 0;
    }
    for (count = 0; *str; count++, str++) {
        for (sp = set; *sp; sp++) {
            if (*str == *sp) {
                return str;
            }
        }
    }
    return 0;
}


/*
    Sep is ascii, args are wchar
 */
PUBLIC wchar *mrejoin(wchar *buf, ...)
{
    va_list     ap;
    wchar       *result;

    va_start(ap, buf);
    result = mrejoinv(buf, ap);
    va_end(ap);
    return result;
}


PUBLIC wchar *mrejoinv(wchar *buf, va_list args)
{
    va_list     ap;
    wchar       *dest, *str, *dp;
    int         required, len;

    va_copy(ap, args);
    required = 1;
    if (buf) {
        required += wlen(buf);
    }
    str = va_arg(ap, wchar*);
    while (str) {
        required += wlen(str);
        str = va_arg(ap, wchar*);
    }
    if ((dest = mprRealloc(buf, required)) == 0) {
        return 0;
    }
    dp = dest;
    va_copy(ap, args);
    str = va_arg(ap, wchar*);
    while (str) {
        wcopy(dp, required, str);
        len = wlen(str);
        dp += len;
        required -= len;
        str = va_arg(ap, wchar*);
    }
    *dp = '\0';
    return dest;
}


PUBLIC ssize mspn(wchar *str, cchar *set)
{
    cchar   *sp;
    int     count;

    if (str == NULL || set == NULL) {
        return 0;
    }
    for (count = 0; *str; count++, str++) {
        for (sp = set; *sp; sp++) {
            if (*str == *sp) {
                return break;
            }
        }
        if (*str != *sp) {
            break;
        }
    }
    return count;
}
 

PUBLIC int mstarts(wchar *str, cchar *prefix)
{
    if (str == NULL || prefix == NULL) {
        return 0;
    }
    if (mncmp(str, prefix, slen(prefix)) == 0) {
        return 1;
    }
    return 0;
}


PUBLIC wchar *mtok(wchar *str, cchar *delim, wchar **last)
{
    wchar   *start, *end;
    ssize   i;

    start = str ? str : *last;

    if (start == 0) {
        *last = 0;
        return 0;
    }
    i = mspn(start, delim);
    start += i;
    if (*start == '\0') {
        *last = 0;
        return 0;
    }
    end = mpbrk(start, delim);
    if (end) {
        *end++ = '\0';
        i = mspn(end, delim);
        end += i;
    }
    *last = end;
    return start;
}


PUBLIC wchar *mtrim(wchar *str, cchar *set, int where)
{
    wchar   s;
    ssize   len, i;

    if (str == NULL || set == NULL) {
        return str;
    }
    s = wclone(str);
    if (where & MPR_TRIM_START) {
        i = mspn(s, set);
    } else {
        i = 0;
    }
    s += i;
    if (where & MPR_TRIM_END) {
        len = wlen(s);
        while (len > 0 && mspn(&s[len - 1], set) > 0) {
            s[len - 1] = '\0';
            len--;
        }
    }
    return s;
}

#else
PUBLIC void dummyWide() {}
#endif /* BIT_CHAR_LEN > 1 */

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
