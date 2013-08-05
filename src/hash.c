/*
    hash.c - Fast hashing hash lookup module

    This hash hash uses a fast key lookup mechanism. Keys may be C strings or unicode strings. The hash value entries 
    are arbitrary pointers. The keys are hashed into a series of buckets which then have a chain of hash entries.
    The chain in in collating sequence so search time through the chain is on average (N/hashSize)/2.

    This module is not thread-safe. It is the callers responsibility to perform all thread synchronization.
    There is locking solely for the purpose of synchronization with the GC marker()

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

/********************************** Defines ***********************************/

#ifndef BIT_MAX_HASH
    #define BIT_MAX_HASH 23           /* Default initial hash size */
#endif

/********************************** Forwards **********************************/

static void *dupKey(MprHash *hash, cvoid *key);
static MprKey *lookupHash(int *index, MprKey **prevSp, MprHash *hash, cvoid *key);
static void manageHashTable(MprHash *hash, int flags);

/*********************************** Code *************************************/
/*
    Create a new hash hash of a given size. Caller should provide a size that is a prime number for the greatest efficiency.
    Can use hashSize -1, 0 to get a default hash.
 */
PUBLIC MprHash *mprCreateHash(int hashSize, int flags)
{
    MprHash     *hash;

    if ((hash = mprAllocObjNoZero(MprHash, manageHashTable)) == 0) {
        return 0;
    }
    if (hashSize < BIT_MAX_HASH) {
        hashSize = BIT_MAX_HASH;
    }
    if ((hash->buckets = mprAllocZeroed(sizeof(MprKey*) * hashSize)) == 0) {
        return NULL;
    }
    hash->flags = flags | MPR_OBJ_HASH;
    hash->size = hashSize;
    hash->length = 0;
    if (!(flags & MPR_HASH_OWN)) {
        hash->mutex = mprCreateLock();
    } else {
        hash->mutex = 0;
    }
#if BIT_CHAR_LEN > 1 && KEEP
    if (hash->flags & MPR_HASH_UNICODE) {
        if (hash->flags & MPR_HASH_CASELESS) {
            hash->fn = (MprHashProc) whashlower;
        } else {
            hash->fn = (MprHashProc) whash;
        }
    } else 
#endif
    {
        if (hash->flags & MPR_HASH_CASELESS) {
            hash->fn = (MprHashProc) shashlower;
        } else {
            hash->fn = (MprHashProc) shash;
        }
    }
    return hash;
}


static void manageHashTable(MprHash *hash, int flags)
{
    MprKey      *sp;
    int         i;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(hash->mutex);
        mprMark(hash->buckets);
        lock(hash);
        for (i = 0; i < hash->size; i++) {
            for (sp = (MprKey*) hash->buckets[i]; sp; sp = sp->next) {
                mprMark(sp);
                if (!(hash->flags & MPR_HASH_STATIC_VALUES)) {
#if BIT_DEBUG
                    if (sp->data && !mprIsValid(sp->data)) {
                        mprLog(0, "Data in key %s is not valid", sp->key);
                    }
                    assert(sp->data == 0 || mprIsValid(sp->data));
#endif
                    mprMark(sp->data);
                }
                if (!(hash->flags & MPR_HASH_STATIC_KEYS)) {
                    assert(mprIsValid(sp->key));
                    mprMark(sp->key);
                }
            }
        }
        unlock(hash);
    }
}


/*
    Insert an entry into the hash hash. If the entry already exists, update its value. 
    Order of insertion is not preserved.
 */
PUBLIC MprKey *mprAddKey(MprHash *hash, cvoid *key, cvoid *ptr)
{
    MprKey      *sp, *prevSp;
    int         index;

    if (hash == 0) {
        assert(hash);
        return 0;
    }
    lock(hash);
    if ((sp = lookupHash(&index, &prevSp, hash, key)) != 0) {
        if (hash->flags & MPR_HASH_UNIQUE) {
            unlock(hash);
            return 0;
        }
        /*
            Already exists. Just update the data.
         */
        sp->data = ptr;
        unlock(hash);
        return sp;
    }
    /*
        Hash entries are managed by manageHashTable
     */
    if ((sp = mprAllocStructNoZero(MprKey)) == 0) {
        unlock(hash);
        return 0;
    }
    sp->data = ptr;
    if (!(hash->flags & MPR_HASH_STATIC_KEYS)) {
        sp->key = dupKey(hash, key);
    } else {
        sp->key = (void*) key;
    }
    sp->type = 0;
    sp->bucket = index;
    sp->next = hash->buckets[index];
    hash->buckets[index] = sp;
    hash->length++;
    unlock(hash);
    return sp;
}


PUBLIC MprKey *mprAddKeyWithType(MprHash *hash, cvoid *key, cvoid *ptr, int type)
{
    MprKey  *kp;

    if ((kp = mprAddKey(hash, key, ptr)) != 0) {
        kp->type = type;
    }
    return kp;
}


PUBLIC MprKey *mprAddKeyFmt(MprHash *hash, cvoid *key, cchar *fmt, ...)
{
    va_list     ap;
    char        *value;

    va_start(ap, fmt);
    value = sfmtv(fmt, ap);
    va_end(ap);
    return mprAddKey(hash, key, value);
}


/*
    Multiple insertion. Insert an entry into the hash hash allowing for multiple entries with the same key.
    Order of insertion is not preserved. Lookup cannot be used to retrieve all duplicate keys, some will be shadowed. 
    Use enumeration to retrieve the keys.
 */
PUBLIC MprKey *mprAddDuplicateKey(MprHash *hash, cvoid *key, cvoid *ptr)
{
    MprKey      *sp;
    int         index;

    assert(hash);
    assert(key);

    if ((sp = mprAllocStructNoZero(MprKey)) == 0) {
        return 0;
    }
    sp->type = 0;
    sp->data = ptr;
    if (!(hash->flags & MPR_HASH_STATIC_KEYS)) {
        sp->key = dupKey(hash, key);
    } else {
        sp->key = (void*) key;
    }
    lock(hash);
    index = hash->fn(key, slen(key)) % hash->size;
    sp->bucket = index;
    sp->next = hash->buckets[index];
    hash->buckets[index] = sp;
    hash->length++;
    unlock(hash);
    return sp;
}


PUBLIC int mprRemoveKey(MprHash *hash, cvoid *key)
{
    MprKey      *sp, *prevSp;
    int         index;

    assert(hash);
    assert(key);

    lock(hash);
    if ((sp = lookupHash(&index, &prevSp, hash, key)) == 0) {
        unlock(hash);
        return MPR_ERR_CANT_FIND;
    }
    if (prevSp) {
        prevSp->next = sp->next;
    } else {
        hash->buckets[index] = sp->next;
    }
    hash->length--;
    unlock(hash);
    return 0;
}


PUBLIC MprHash *mprBlendHash(MprHash *hash, MprHash *extra)
{
    MprKey      *kp;

    if (hash == 0 || extra == 0) {
        return hash;
    }
    for (ITERATE_KEYS(extra, kp)) {
        mprAddKey(hash, kp->key, kp->data);
    }
    return hash;
}


PUBLIC MprHash *mprCloneHash(MprHash *master)
{
    MprKey      *kp;
    MprHash     *hash;

    assert(master);

    if ((hash = mprCreateHash(master->size, master->flags)) == 0) {
        return 0;
    }
    kp = mprGetFirstKey(master);
    while (kp) {
        mprAddKey(hash, kp->key, kp->data);
        kp = mprGetNextKey(master, kp);
    }
    return hash;
}


/*
    Lookup a key and return the hash entry
 */
PUBLIC MprKey *mprLookupKeyEntry(MprHash *hash, cvoid *key)
{
    return lookupHash(0, 0, hash, key);
}


/*
    Lookup a key and return the hash entry data
 */
PUBLIC void *mprLookupKey(MprHash *hash, cvoid *key)
{
    MprKey      *sp;

    if ((sp = lookupHash(0, 0, hash, key)) == 0) {
        return 0;
    }
    return (void*) sp->data;
}


/*
    Exponential primes
 */
static int hashSizes[] = {
     19, 29, 59, 79, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613, 0
};


static int getHashSize(int numKeys)
{
    int     i;

    for (i = 0; hashSizes[i]; i++) {
        if (numKeys < hashSizes[i]) {
            return hashSizes[i];
        }
    }
    return hashSizes[i - 1];
}


/*
    This is unlocked because it is read-only
 */
static MprKey *lookupHash(int *bucketIndex, MprKey **prevSp, MprHash *hash, cvoid *key)
{
    MprKey      *sp, *prev, *next;
    MprKey      **buckets;
    int         hashSize, i, index, rc;

    if (key == 0 || hash == 0) {
        return 0;
    }
    if (hash->length > hash->size) {
        hashSize = getHashSize(hash->length * 4 / 3);
        if (hash->size < hashSize) {
            if ((buckets = mprAllocZeroed(sizeof(MprKey*) * hashSize)) != 0) {
                hash->length = 0;
                for (i = 0; i < hash->size; i++) {
                    for (sp = hash->buckets[i]; sp; sp = next) {
                        next = sp->next;
                        assert(next != sp);
                        index = hash->fn(sp->key, slen(sp->key)) % hashSize;
                        if (buckets[index]) {
                            sp->next = buckets[index];
                        } else {
                            sp->next = 0;
                        }
                        buckets[index] = sp;
                        sp->bucket = index;
                        hash->length++;
                    }
                }
                hash->size = hashSize;
                hash->buckets = buckets;
            }
        }
    }
    index = hash->fn(key, slen(key)) % hash->size;
    if (bucketIndex) {
        *bucketIndex = index;
    }
    sp = hash->buckets[index];
    prev = 0;

    while (sp) {
#if BIT_CHAR_LEN > 1 && KEEP
        if (hash->flags & MPR_HASH_UNICODE) {
            wchar *u1, *u2;
            u1 = (wchar*) sp->key;
            u2 = (wchar*) key;
            rc = -1;
            if (hash->flags & MPR_HASH_CASELESS) {
                rc = wcasecmp(u1, u2);
            } else {
                rc = wcmp(u1, u2);
            }
        } else 
#endif
        if (hash->flags & MPR_HASH_CASELESS) {
            rc = scaselesscmp(sp->key, key);
        } else {
            rc = strcmp(sp->key, key);
        }
        if (rc == 0) {
            if (prevSp) {
                *prevSp = prev;
            }
            return sp;
        }
        prev = sp;
        assert(sp != sp->next);
        sp = sp->next;
    }
    return 0;
}


PUBLIC int mprGetHashLength(MprHash *hash)
{
    return hash->length;
}


/*
    Return the first entry in the hash.
 */
PUBLIC MprKey *mprGetFirstKey(MprHash *hash)
{
    MprKey      *sp;
    int         i;

    if (!hash) {
        return 0;
    }
    for (i = 0; i < hash->size; i++) {
        if ((sp = (MprKey*) hash->buckets[i]) != 0) {
            return sp;
        }
    }
    return 0;
}


/*
    Return the next entry in the hash
 */
PUBLIC MprKey *mprGetNextKey(MprHash *hash, MprKey *last)
{
    MprKey      *sp;
    int         i;

    if (hash == 0) {
        return 0;
    }
    if (last == 0) {
        return mprGetFirstKey(hash);
    }
    if (last->next) {
        return last->next;
    }
    for (i = last->bucket + 1; i < hash->size; i++) {
        if ((sp = (MprKey*) hash->buckets[i]) != 0) {
            return sp;
        }
    }
    return 0;
}


static void *dupKey(MprHash *hash, cvoid *key)
{
#if BIT_CHAR_LEN > 1 && KEEP
    if (hash->flags & MPR_HASH_UNICODE) {
        return wclone((wchar*) key);
    } else
#endif
        return sclone(key);
}


PUBLIC MprHash *mprCreateHashFromWords(cchar *str)
{
    MprHash     *hash;
    char        *word, *next;

    hash = mprCreateHash(0, 0);
    word = stok(sclone(str), ", \t\n\r", &next);
    while (word) {
        mprAddKey(hash, word, word);
        word = stok(NULL, " \t\n\r", &next);
    }
    return hash;
}


PUBLIC char *mprHashToString(MprHash *hash, cchar *join)
{
    MprBuf  *buf;
    MprKey  *kp;
    cchar   *item;

    if (!join) {
        join = ",";
    }
    buf = mprCreateBuf(0, 0);
    for (ITERATE_KEY_DATA(hash, kp, item)) {
        mprPutStringToBuf(buf, item);
        mprPutStringToBuf(buf, join);
    }
    mprAdjustBufEnd(buf, -1);
    mprAddNullToBuf(buf);
    return mprGetBufStart(buf);
}


PUBLIC char *mprHashKeysToString(MprHash *hash, cchar *join)
{
    MprBuf  *buf;
    MprKey  *kp;

    if (!join) {
        join = ",";
    }
    buf = mprCreateBuf(0, 0);
    for (ITERATE_KEYS(hash, kp)) {
        mprPutStringToBuf(buf, kp->key);
        mprPutStringToBuf(buf, join);
    }
    mprAdjustBufEnd(buf, -1);
    mprAddNullToBuf(buf);
    return mprGetBufStart(buf);
}


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
