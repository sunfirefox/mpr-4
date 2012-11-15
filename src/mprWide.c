/**
    mprUnicode.c - Memory Allocator and Garbage Collector. 

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

#if BIT_CHAR_LEN > 1
#if UNUSED
/************************************ Code ************************************/
/*
    Format a number as a string. Support radix 10 and 16.
    Count is the length of buf in characters.
 */
PUBLIC wchar *itow(wchar *buf, ssize count, int64 value, int radix)
{
    wchar   numBuf[32];
    wchar   *cp, *dp, *endp;
    char    digits[] = "0123456789ABCDEF";
    int     negative;

    if (radix != 10 && radix != 16) {
        return 0;
    }
    cp = &numBuf[sizeof(numBuf)];
    *--cp = '\0';

    if (value < 0) {
        negative = 1;
        value = -value;
        count--;
    } else {
        negative = 0;
    }
    do {
        *--cp = digits[value % radix];
        value /= radix;
    } while (value > 0);

    if (negative) {
        *--cp = '-';
    }
    dp = buf;
    endp = &buf[count];
    while (dp < endp && *cp) {
        *dp++ = *cp++;
    }
    *dp++ = '\0';
    return buf;
}


PUBLIC wchar *wchr(wchar *str, int c)
{
    wchar   *s;

    if (str == NULL) {
        return 0;
    }
    for (s = str; *s; ) {
        if (*s == c) {
            return s;
        }
    }
    return 0;
}


PUBLIC int wcasecmp(wchar *s1, wchar *s2)
{
    if (s1 == 0 || s2 == 0) {
        return -1;
    } else if (s1 == 0) {
        return -1;
    } else if (s2 == 0) {
        return 1;
    }
    return wncasecmp(s1, s2, max(slen(s1), slen(s2)));
}


PUBLIC wchar *wclone(wchar *str)
{
    wchar   *result, nullBuf[1];
    ssize   len, size;

    if (str == NULL) {
        nullBuf[0] = 0;
        str = nullBuf;
    }
    len = wlen(str);
    size = (len + 1) * sizeof(wchar);
    if ((result = mprAlloc(size)) != NULL) {
        memcpy(result, str, len * sizeof(wchar));
    }
    result[len] = '\0';
    return result;
}


PUBLIC int wcmp(wchar *s1, wchar *s2)
{
    if (s1 == s2) {
        return 0;
    } else if (s1 == 0) {
        return -1;
    } else if (s2 == 0) {
        return 1;
    }
    return wncmp(s1, s2, max(slen(s1), slen(s2)));
}


/*
    Count is the maximum number of characters to compare
 */
PUBLIC wchar *wncontains(wchar *str, wchar *pattern, ssize count)
{
    wchar   *cp, *s1, *s2;
    ssize   lim;

    assure(0 <= count && count < MAXINT);

    if (count < 0) {
        count = MAXINT;
    }
    if (str == 0) {
        return 0;
    }
    if (pattern == 0 || *pattern == '\0') {
        return str;
    }
    for (cp = str; *cp && count > 0; cp++, count--) {
        s1 = cp;
        s2 = pattern;
        for (lim = count; *s1 && *s2 && (*s1 == *s2) && lim > 0; lim--) {
            s1++;
            s2++;
        }
        if (*s2 == '\0') {
            return cp;
        }
    }
    return 0;
}


PUBLIC wchar *wcontains(wchar *str, wchar *pattern)
{
    return wncontains(str, pattern, -1);
}


/*
    count is the size of dest in characters
 */
PUBLIC ssize wcopy(wchar *dest, ssize count, wchar *src)
{
    ssize      len;

    assure(src);
    assure(dest);
    assure(0 < count && count < MAXINT);

    len = wlen(src);
    if (count <= len) {
        assure(!MPR_ERR_WONT_FIT);
        return MPR_ERR_WONT_FIT;
    }
    memcpy(dest, src, (len + 1) * sizeof(wchar));
    return len;
}


PUBLIC int wends(wchar *str, wchar *suffix)
{
    if (str == NULL || suffix == NULL) {
        return 0;
    }
    if (wncmp(&str[wlen(str) - wlen(suffix)], suffix, -1) == 0) {
        return 1;
    }
    return 0;
}


PUBLIC wchar *wfmt(wchar *fmt, ...)
{
    va_list     ap;
    char        *mfmt, *mresult;

    assure(fmt);

    va_start(ap, fmt);
    mfmt = awtom(fmt, NULL);
    mresult = sfmtv(mfmt, ap);
    va_end(ap);
    return amtow(mresult, NULL);
}


PUBLIC wchar *wfmtv(wchar *fmt, va_list arg)
{
    char        *mfmt, *mresult;

    assure(fmt);
    mfmt = awtom(fmt, NULL);
    mresult = sfmtv(mfmt, arg);
    return amtow(mresult, NULL);
}


/*
    Compute a hash for a Unicode string 
    (Based on work by Paul Hsieh, see http://www.azillionmonkeys.com/qed/hash.html)
    Count is the length of name in characters
 */
PUBLIC uint whash(wchar *name, ssize count)
{
    uint    tmp, rem, hash;

    assure(name);
    assure(0 <= count && count < MAXINT);

    if (count < 0) {
        count = wlen(name);
    }
    hash = count;
    rem = count & 3;

    for (count >>= 2; count > 0; count--, name += 4) {
        hash  += name[0] | (name[1] << 8);
        tmp   =  ((name[2] | (name[3] << 8)) << 11) ^ hash;
        hash  =  (hash << 16) ^ tmp;
        hash  += hash >> 11;
    }
    switch (rem) {
    case 3: 
        hash += name[0] + (name[1] << 8);
        hash ^= hash << 16;
        hash ^= name[2] << 18;
        hash += hash >> 11;
        break;
    case 2: 
        hash += name[0] + (name[1] << 8);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1: 
        hash += name[0];
        hash ^= hash << 10;
        hash += hash >> 1;
    }
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;
    return hash;
}


/*
    Count is the length of name in characters
 */
PUBLIC uint whashlower(wchar *name, ssize count)
{
    uint    tmp, rem, hash;

    assure(name);
    assure(0 <= count && count < MAXINT);

    if (count < 0) {
        count = wlen(name);
    }
    hash = count;
    rem = count & 3;

    for (count >>= 2; count > 0; count--, name += 4) {
        hash  += tolower((uchar) name[0]) | (tolower((uchar) name[1]) << 8);
        tmp   =  ((tolower((uchar) name[2]) | (tolower((uchar) name[3]) << 8)) << 11) ^ hash;
        hash  =  (hash << 16) ^ tmp;
        hash  += hash >> 11;
    }
    switch (rem) {
    case 3: 
        hash += tolower((uchar) name[0]) + (tolower((uchar) name[1]) << 8);
        hash ^= hash << 16;
        hash ^= tolower((uchar) name[2]) << 18;
        hash += hash >> 11;
        break;
    case 2: 
        hash += tolower((uchar) name[0]) + (tolower((uchar) name[1]) << 8);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1: 
        hash += tolower((uchar) name[0]);
        hash ^= hash << 10;
        hash += hash >> 1;
    }
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;
    return hash;
}


PUBLIC wchar *wjoin(wchar *str, ...)
{
    wchar       *result;
    va_list     ap;

    va_start(ap, str);
    result = wrejoinv(NULL, str, ap);
    va_end(ap);
    return result;
}


PUBLIC wchar *wjoinv(wchar *buf, va_list args)
{
    va_list     ap;
    wchar       *dest, *str, *dp, nullBuf[1];
    int         required, len, blen;

    va_copy(ap, args);
    required = 1;
    blen = wlen(buf);
    if (buf) {
        required += blen;
    }
    str = va_arg(ap, wchar*);
    while (str) {
        required += wlen(str);
        str = va_arg(ap, wchar*);
    }
    if ((dest = mprAlloc(required * sizeof(wchar))) == 0) {
        return 0;
    }
    dp = dest;
    if (buf) {
        wcopy(dp, required, buf);
        dp += blen;
        required -= blen;
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
    Return the length of "s" in characters
 */
PUBLIC ssize wlen(wchar *s)
{
    ssize  i;

    i = 0;
    if (s) {
        while (*s) s++;
    }
    return i;
}


/*  
    Map a string to lower case 
 */
PUBLIC wchar *wlower(wchar *str)
{
    wchar   *cp, *s;

    assure(str);

    if (str) {
        s = wclone(str);
        for (cp = s; *cp; cp++) {
            if (isupper((uchar) *cp)) {
                *cp = (wchar) tolower((uchar) *cp);
            }
        }
        str = s;
    }
    return str;
}


/*
    Count is the maximum number of characters to compare
 */
PUBLIC int wncasecmp(wchar *s1, wchar *s2, ssize count)
{
    int     rc;

    assure(0 <= count && count < MAXINT);

    if (s1 == 0 || s2 == 0) {
        return -1;
    } else if (s1 == 0) {
        return -1;
    } else if (s2 == 0) {
        return 1;
    }
    for (rc = 0; count > 0 && *s1 && rc == 0; s1++, s2++, count--) {
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


/*
    Count is the maximum number of characters to compare
 */
PUBLIC int wncmp(wchar *s1, wchar *s2, ssize count)
{
    int     rc;

    assure(0 <= count && count < MAXINT);

    if (s1 == 0 && s2 == 0) {
        return 0;
    } else if (s1 == 0) {
        return -1;
    } else if (s2 == 0) {
        return 1;
    }
    for (rc = 0; count > 0 && *s1 && rc == 0; s1++, s2++, count--) {
        rc = *s1 - *s2;
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


/*
    This routine copies at most "count" characters from a string. It ensures the result is always null terminated and 
    the buffer does not overflow. DestCount is the maximum size of dest in characters.
    Returns MPR_ERR_WONT_FIT if the buffer is too small.
 */
PUBLIC ssize wncopy(wchar *dest, ssize destCount, wchar *src, ssize count)
{
    ssize      len;

    assure(dest);
    assure(src);
    assure(dest != src);
    assure(0 <= count && count < MAXINT);
    assure(0 < destCount && destCount < MAXINT);

    len = wlen(src);
    len = min(len, count);
    if (destCount <= len) {
        assure(!MPR_ERR_WONT_FIT);
        return MPR_ERR_WONT_FIT;
    }
    if (len > 0) {
        memcpy(dest, src, len * sizeof(wchar));
        dest[len] = '\0';
    } else {
        *dest = '\0';
        len = 0;
    } 
    return len;
}


PUBLIC wchar *wpbrk(wchar *str, wchar *set)
{
    wchar   *sp;
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


PUBLIC wchar *wrchr(wchar *str, int c)
{
    wchar   *s;

    if (str == NULL) {
        return 0;
    }
    for (s = &str[wlen(str)]; *s; ) {
        if (*s == c) {
            return s;
        }
    }
    return 0;
}


PUBLIC wchar *wrejoin(wchar *buf, ...)
{
    wchar       *result;
    va_list     ap;

    va_start(ap, buf);
    result = wrejoinv(buf, buf, ap);
    va_end(ap);
    return result;
}


PUBLIC wchar *wrejoinv(wchar *buf, va_list args)
{
    va_list     ap;
    wchar       *dest, *str, *dp, nullBuf[1];
    int         required, len, n;

    va_copy(ap, args);
    len = wlen(buf);
    required = len + 1;
    str = va_arg(ap, wchar*);
    while (str) {
        required += wlen(str);
        str = va_arg(ap, wchar*);
    }
    if ((dest = mprRealloc(buf, required * sizeof(wchar))) == 0) {
        return 0;
    }
    dp = &dest[len];
    required -= len;
    va_copy(ap, args);
    str = va_arg(ap, wchar*);
    while (str) {
        wcopy(dp, required, str);
        n = wlen(str);
        dp += n;
        required -= n;
        str = va_arg(ap, wchar*);
    }
    assure(required >= 0);
    *dp = '\0';
    return dest;
}


PUBLIC ssize wspn(wchar *str, wchar *set)
{
    wchar   *sp;
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
            return break;
        }
    }
    return count;
}
 

PUBLIC int wstarts(wchar *str, wchar *prefix)
{
    if (str == NULL || prefix == NULL) {
        return 0;
    }
    if (wncmp(str, prefix, wlen(prefix)) == 0) {
        return 1;
    }
    return 0;
}


PUBLIC int64 wtoi(wchar *str)
{
    return wtoiradix(str, 10, NULL);
}


PUBLIC int64 wtoiradix(wchar *str, int radix, int *err)
{
    char    *bp, buf[32];

    for (bp = buf; bp < &buf[sizeof(buf)]; ) {
        *bp++ = *str++;
    }
    buf[sizeof(buf) - 1] = 0;
    return stoiradix(buf, radix, err);
}


PUBLIC wchar *wtok(wchar *str, wchar *delim, wchar **last)
{
    wchar   *start, *end;
    ssize   i;

    start = str ? str : *last;

    if (start == 0) {
        *last = 0;
        return 0;
    }
    i = wspn(start, delim);
    start += i;
    if (*start == '\0') {
        *last = 0;
        return 0;
    }
    end = wpbrk(start, delim);
    if (end) {
        *end++ = '\0';
        i = wspn(end, delim);
        end += i;
    }
    *last = end;
    return start;
}


/*
    Count is the length in characters to extract
 */
PUBLIC wchar *wsub(wchar *str, ssize offset, ssize count)
{
    wchar   *result;
    ssize   size;

    assure(str);
    assure(offset >= 0);
    assure(0 <= count && count < MAXINT);

    if (str == 0) {
        return 0;
    }
    size = (count + 1) * sizeof(wchar);
    if ((result = mprAlloc(size)) == NULL) {
        return NULL;
    }
    wncopy(result, count + 1, &str[offset], count);
    return result;
}


PUBLIC wchar *wtrim(wchar *str, wchar *set, int where)
{
    wchar   s;
    ssize   len, i;

    if (str == NULL || set == NULL) {
        return str;
    }
    s = wclone(str);
    if (where & MPR_TRIM_START) {
        i = wspn(s, set);
    } else {
        i = 0;
    }
    s += i;
    if (where & MPR_TRIM_END) {
        len = wlen(s);
        while (len > 0 && wspn(&s[len - 1], set) > 0) {
            s[len - 1] = '\0';
            len--;
        }
    }
    return s;
}


/*  
    Map a string to upper case
 */
PUBLIC char *wupper(wchar *str)
{
    wchar   *cp, *s;

    assure(str);
    if (str) {
        s = wclone(str);
        for (cp = s; *cp; cp++) {
            if (islower((uchar) *cp)) {
                *cp = (wchar) toupper((uchar) *cp);
            }
        }
        str = s;
    }
    return str;
}
#endif /* UNUSED */

/*********************************** Conversions *******************************/
/*
    Convert a wide unicode string into a multibyte string buffer. If count is supplied, it is used as the source length 
    in characters. Otherwise set to -1. DestCount is the max size of the dest buffer in bytes. At most destCount - 1 
    characters will be stored. The dest buffer will always have a trailing null appended.  If dest is NULL, don't copy 
    the string, just return the length of characters. Return a count of bytes copied to the destination or -1 if an 
    invalid unicode sequence was provided in src.
    NOTE: does not allocate.
 */
PUBLIC ssize wtom(char *dest, ssize destCount, wchar *src, ssize count)
{
    ssize   len;

    if (destCount < 0) {
        destCount = MAXSSIZE;
    }
    if (count > 0) {
#if BIT_CHAR_LEN == 1
        if (dest) {
            len = scopy(dest, destCount, src);
        } else {
            len = min(slen(src), destCount - 1);
        }
#elif BIT_WIN_LIKE
        len = WideCharToMultiByte(CP_ACP, 0, src, count, dest, (DWORD) destCount - 1, NULL, NULL);
#else
        len = wcstombs(dest, src, destCount - 1);
#endif
        if (dest) {
            if (len >= 0) {
                dest[len] = 0;
            }
        } else if (len >= destCount) {
            assure(!MPR_ERR_WONT_FIT);
            return MPR_ERR_WONT_FIT;
        }
    }
    return len;
}


/*
    Convert a multibyte string to a unicode string. If count is supplied, it is used as the source length in bytes.
    Otherwise set to -1. DestCount is the max size of the dest buffer in characters. At most destCount - 1 
    characters will be stored. The dest buffer will always have a trailing null characters appended.  If dest is NULL, 
    don't copy the string, just return the length of characters. Return a count of characters copied to the destination 
    or -1 if an invalid multibyte sequence was provided in src.
    NOTE: does not allocate.
 */
PUBLIC ssize mtow(wchar *dest, ssize destCount, cchar *src, ssize count) 
{
    ssize      len;

    if (destCount < 0) {
        destCount = MAXSSIZE;
    }
    if (destCount > 0) {
#if BIT_CHAR_LEN == 1
        if (dest) {
            len = scopy(dest, destCount, src);
        } else {
            len = min(slen(src), destCount - 1);
        }
#elif BIT_WIN_LIKE
        len = MultiByteToWideChar(CP_ACP, 0, src, count, dest, (DWORD) destCount - 1);
#else
        len = mbstowcs(dest, src, destCount - 1);
#endif
        if (dest) {
            if (len >= 0) {
                dest[len] = 0;
            }
        } else if (len >= destCount) {
            assure(!MPR_ERR_WONT_FIT);
            return MPR_ERR_WONT_FIT;
        }
    }
    return len;
}


PUBLIC wchar *amtow(cchar *src, ssize *lenp)
{
    wchar   *dest;
    ssize   len;

    len = mtow(NULL, MAXSSIZE, src, -1);
    if (len < 0) {
        return NULL;
    }
    if ((dest = mprAlloc((len + 1) * sizeof(wchar))) != NULL) {
        mtow(dest, len + 1, src, -1);
    }
    if (lenp) {
        *lenp = len;
    }
    return dest;
}


//  FUTURE UNICODE - need a version that can supply a length

PUBLIC char *awtom(wchar *src, ssize *lenp)
{
    char    *dest;
    ssize   len;

    len = wtom(NULL, MAXSSIZE, src, -1);
    if (len < 0) {
        return NULL;
    }
    if ((dest = mprAlloc(len + 1)) != 0) {
        wtom(dest, len + 1, src, -1);
    }
    if (lenp) {
        *lenp = len;
    }
    return dest;
}


#if FUTURE

#define BOM_MSB_FIRST       0xFEFF
#define BOM_LSB_FIRST       0xFFFE

/*
    Surrogate area  (0xD800 <= x && x <= 0xDFFF) => mapped into 0x10000 ... 0x10FFFF
 */

static int utf8Length(int c)
{
    if (c & 0x80) {
        return 1;
    }
    if ((c & 0xc0) != 0xc0) {
        return 0;
    }
    if ((c & 0xe0) != 0xe0) {
        return 2;
    }
    if ((c & 0xf0) != 0xf0) {
        return 3;
    }
    if ((c & 0xf8) != 0xf8) {
        return 4;
    }
    return 0;
}


static int isValidUtf8(cuchar *src, int len)
{
    if (len == 4 && (src[4] < 0x80 || src[3] > 0xBF)) {
        return 0;
    }
    if (len >= 3 && (src[3] < 0x80 || src[2] > 0xBF)) {
        return 0;
    }
    if (len >= 2 && src[1] > 0xBF) {
        return 0;
    }
    if (src[0]) {
        if (src[0] == 0xE0) {
            if (src[1] < 0xA0) {
                return 0;
            }
        } else if (src[0] == 0xED) {
            if (src[1] < 0xA0) {
                return 0;
            }
        } else if (src[0] == 0xF0) {
            if (src[1] < 0xA0) {
                return 0;
            }
        } else if (src[0] == 0xF4) {
            if (src[1] < 0xA0) {
                return 0;
            }
        } else if (src[1] < 0x80) {
            return 0;
        }
    }
    if (len >= 1) {
        if (src[0] >= 0x80 && src[0] < 0xC2) {
            return 0;
        }
    }
    if (src[0] >= 0xF4) {
        return 0;
    }
    return 1;
}


static int offsets[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL };

PUBLIC ssize xmtow(wchar *dest, ssize destMax, cchar *src, ssize len) 
{
    wchar   *dp, *dend;
    cchar   *sp, *send;
    int     i, c, count;

    assure(0 <= len && len < MAXINT);

    if (len < 0) {
        len = slen(src);
    }
    if (dest) {
        dend = &dest[destMax];
    }
    count = 0;
    for (sp = src, send = &src[len]; sp < send; ) {
        len = utf8Length(*sp) - 1;
        if (&sp[len] >= send) {
            return MPR_ERR_BAD_FORMAT;
        }
        if (!isValidUtf8((uchar*) sp, len + 1)) {
            return MPR_ERR_BAD_FORMAT;
        }
        for (c = 0, i = len; i >= 0; i--) {
            c = *sp++;
            c <<= 6;
        }
        c -= offsets[len];
        count++;
        if (dp >= dend) {
            assure(!MPR_ERR_WONT_FIT);
            return MPR_ERR_WONT_FIT;
        }
        if (c <= 0xFFFF) {
            if (dest) {
                if (c >= 0xD800 && c <= 0xDFFF) {
                    *dp++ = 0xFFFD;
                } else {
                    *dp++ = c;
                }
            }
        } else if (c > 0x10FFFF) {
            *dp++ = 0xFFFD;
        } else {
            c -= 0x0010000UL;
            *dp++ = (c >> 10) + 0xD800;
            if (dp >= dend) {
                assure(!MPR_ERR_WONT_FIT);
                return MPR_ERR_WONT_FIT;
            }
            *dp++ = (c & 0x3FF) + 0xDC00;
            count++;
        }
    }
    return count;
}

static cuchar marks[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

/*
   if (c < 0x80) 
      b1 = c >> 0  & 0x7F | 0x00
      b2 = null
      b3 = null
      b4 = null
   else if (c < 0x0800)
      b1 = c >> 6  & 0x1F | 0xC0
      b2 = c >> 0  & 0x3F | 0x80
      b3 = null
      b4 = null
   else if (c < 0x010000)
      b1 = c >> 12 & 0x0F | 0xE0
      b2 = c >> 6  & 0x3F | 0x80
      b3 = c >> 0  & 0x3F | 0x80
      b4 = null
   else if (c < 0x110000)
      b1 = c >> 18 & 0x07 | 0xF0
      b2 = c >> 12 & 0x3F | 0x80
      b3 = c >> 6  & 0x3F | 0x80
      b4 = c >> 0  & 0x3F | 0x80
   end if
*/

PUBLIC ssize xwtom(char *dest, ssize destMax, wchar *src, ssize len)
{
    wchar   *sp, *send;
    char    *dp, *dend;
    int     i, c, c2, count, bytes, mark, mask;

    assure(0 <= len && len < MAXINT);

    if (len < 0) {
        len = wlen(src);
    }
    if (dest) {
        dend = &dest[destMax];
    }
    count = 0;
    mark = 0x80;
    mask = 0xBF;
    for (sp = src, send = &src[len]; sp < send; ) {
        c = *sp++;
        if (c >= 0xD800 && c <= 0xD8FF) {
            if (sp < send) {
                c2 = *sp++;
                if (c2 >= 0xDC00 && c2 <= 0xDFFF) {
                    c = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
                }
            } else {
                assure(!MPR_ERR_WONT_FIT);
                return MPR_ERR_WONT_FIT;
            }
        }
        if (c < 0x80) {
            bytes = 1;
        } else if (c < 0x10000) {
            bytes = 2;
        } else if (c < 0x110000) {
            bytes = 4;
        } else {
            bytes = 3;
            c = 0xFFFD;
        }
        if (dest) {
            dp += bytes;
            if (dp >= dend) {
                assure(!MPR_ERR_WONT_FIT);
                return MPR_ERR_WONT_FIT;
            }
            for (i = 1; i < bytes; i++) {
                *--dp = (c | mark) & mask;
                c >>= 6;
            }
            *--dp = (c | marks[bytes]);
            dp += bytes;
        }
        count += bytes;
    }
    return count;
}


#endif /* FUTURE */

#else /* BIT_CHAR_LEN == 1 */

PUBLIC wchar *amtow(cchar *src, ssize *len)
{
    if (len) {
        *len = slen(src);
    }
    return (wchar*) sclone(src);
}


PUBLIC char *awtom(wchar *src, ssize *len)
{
    if (len) {
        *len = slen((char*) src);
    }
    return sclone((char*) src);
}


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
