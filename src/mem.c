/**
    mem.c - Memory Allocator and Garbage Collector. 

    This is the MPR memory allocation service. It provides an application specific memory allocator to use instead of malloc. 
    This allocator is tailored to the needs of embedded applications and is faster than most general purpose malloc allocators. 
    It is deterministic and allocates and frees in constant time O(1). It exhibits very low fragmentation and accurate
    coalescing.

    The allocator uses a garbage collector for freeing unused memory. The collector is a cooperative, non-compacting,
    parallel collector.  The allocator is optimized for frequent allocations of small blocks (< 4K) and uses a scheme
    of free queues for fast allocation.
    
    The allocator handles memory allocation errors globally. The application may configure a memory limit so that
    memory depletion can be proactively detected and handled before memory allocations actually fail.
   
    A memory block that is being used must be marked as active to prevent the garbage collector from reclaiming it.
    To mark a block as active, #mprMarkBlock must be called during each garbage collection cycle. When allocating
    non-temporal memory blocks, a manager callback can be specified via #mprAllocObj. This manager routine will be
    called by the collector so that dependent memory blocks can be marked as active.
  
    The collector performs the marking phase by invoking the manager routines for a set of root blocks. A block can be
    added to the set of roots by calling #mprAddRoot. Each root's manager routine will mark other blocks which will cause
    their manager routines to run and so on, until all active blocks have been marked. Non-marked blocks can then safely
    be reclaimed as garbage. A block may alternatively be permanently marked as active by calling #mprHold.
 
    The mark phase begins when all threads explicitly "yield" to the garbage collector. This cooperative approach ensures
    that user threads will not inadvertendly loose allocated blocks to the collector. Once all active blocks are marked,
    user threads are resumed and the garbage sweeper frees unused blocks in parallel with user threads.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

/********************************** Defines ***********************************/

#undef GET_MEM
#undef GET_PTR
#define GET_MEM(ptr)                ((MprMem*) (((char*) (ptr)) - sizeof(MprMem)))
#define GET_PTR(mp)                 ((char*) (((char*) mp) + sizeof(MprMem)))
#define GET_USIZE(mp)               ((size_t) (mp->size - sizeof(MprMem) - (mp->hasManager * sizeof(void*))))

/*
    These routines are stable and will work, lock-free regardless of block splitting or joining.
    There is be a race where GET_NEXT will skip a block if the allocator is splits mp.
 */
#define GET_NEXT(mp)                ((MprMem*) ((char*) mp + mp->size))
#define GET_REGION(mp)              ((MprRegion*) (((char*) mp) - MPR_ALLOC_ALIGN(sizeof(MprRegion))))

/*
    Memory checking and breakpoints
    BIT_MPR_ALLOC_DEBUG checks that blocks are valid and keeps track of the location where memory is allocated from.
 */
#if BIT_MPR_ALLOC_DEBUG
    /*
        Set this address to break when this address is allocated or freed
        Only used for debug, but defined regardless so we can have constant exports.
     */
    static MprMem *stopAlloc = 0;
    static int stopSeqno = -1;

    #define BREAKPOINT(mp)          breakpoint(mp)
    #define CHECK(mp)               if (mp) { mprCheckBlock((MprMem*) mp); } else
    #define CHECK_PTR(ptr)          CHECK(GET_MEM(ptr))
    #define CHECK_YIELDED()         checkYielded()
    #define SCRIBBLE(mp)            if (heap->scribble && mp != GET_MEM(MPR)) { \
                                        memset((char*) mp + MPR_ALLOC_MIN_BLOCK, 0xFE, mp->size - MPR_ALLOC_MIN_BLOCK); \
                                    } else
    #define SCRIBBLE_RANGE(ptr, size) if (heap->scribble) { \
                                        memset((char*) ptr, 0xFE, size); \
                                    } else
    #define SET_MAGIC(mp)           mp->magic = MPR_ALLOC_MAGIC
    #define SET_SEQ(mp)             mp->seqno = heap->nextSeqno++
    #define VALID_BLK(mp)           validBlk(mp)
    #define SET_NAME(mp, value)     mp->name = value

#else
    #define BREAKPOINT(mp)
    #define CHECK(mp)
    #define CHECK_PTR(mp)
    #define CHECK_YIELDED()
    #define SCRIBBLE(mp)
    #define SCRIBBLE_RANGE(ptr, size)
    #define SET_NAME(mp, value)
    #define SET_MAGIC(mp)
    #define SET_SEQ(mp)
    #define VALID_BLK(mp) 1
#endif

#if BIT_MPR_ALLOC_STATS
    #define ATOMIC_ADD(field, adj) mprAtomicAdd64((int64*) &heap->stats.field, adj)
    #define ATOMIC_INC(field) mprAtomicAdd64((int64*) &heap->stats.field, 1)
    #define INC(field) heap->stats.field++
#else
    #define ATOMIC_ADD(field, adj)
    #define ATOMIC_INC(field)
    #define INC(field)
#endif

#if LINUX || BIT_BSD_LIKE
    #define findFirstBit(word) ffsl((long) word)
#endif
#if MACOSX
    #define findLastBit(x) flsl((long) x)
#endif
#ifndef findFirstBit
    static MPR_INLINE int findFirstBit(size_t word);
#endif
#ifndef findLastBit
    static MPR_INLINE int findLastBit(size_t word);
#endif

#define YIELDED_THREADS     0x1         /* Resume threads that are yielded (only) */
#define WAITING_THREADS     0x2         /* Resume threads that are waiting for GC sweep to complete */

/********************************** Data **************************************/

#undef              MPR
PUBLIC Mpr          *MPR;
static MprHeap      *heap;
static MprMemStats  memStats;
static int          padding[] = { 0, MPR_MANAGER_SIZE };

/***************************** Forward Declarations ***************************/

static MPR_INLINE bool acquire(MprFreeQueue *freeq);
static void allocException(int cause, size_t size);
static MprMem *allocMem(size_t size);
static MPR_INLINE int cas(size_t *target, size_t expected, size_t value);
static MPR_INLINE bool claim(MprMem *mp);
static MPR_INLINE void clearbitmap(size_t *bitmap, int bindex);
static void dummyManager(void *ptr, int flags);
static size_t fastMemSize();
static void freeBlock(MprMem *mp);
static void getSystemInfo();
static MprMem *growHeap(size_t size);
static MPR_INLINE size_t qtosize(int qindex);
static MPR_INLINE bool linkBlock(MprMem *mp); 
static MPR_INLINE void linkSpareBlock(char *ptr, size_t size);
static MPR_INLINE void initBlock(MprMem *mp, size_t size, int first);
static int initQueues();
static void invokeDestructors();
static void markAndSweep();
static void markRoots();
static int pauseThreads();
static void printMemReport();
static MPR_INLINE void release(MprFreeQueue *freeq);
static void resumeThreads(int flags);
static MPR_INLINE void setbitmap(size_t *bitmap, int bindex);
static MPR_INLINE int sizetoq(size_t size);
static void sweep();
static void gc(void *unused, MprThread *tp);
static MPR_INLINE void triggerGC();
static MPR_INLINE void unlinkBlock(MprMem *mp);
static void *vmalloc(size_t size, int mode);
static void vmfree(void *ptr, size_t size);

#if BIT_WIN_LIKE
    static int winPageModes(int flags);
#endif
#if BIT_MPR_ALLOC_DEBUG
    static void breakpoint(MprMem *mp);
    static void checkYielded();
    static int validBlk(MprMem *mp);
    static void freeLocation(MprMem *mp);
#else
    #define freeLocation(mp)
#endif
#if BIT_MPR_ALLOC_STATS
    static void printQueueStats();
    static void printGCStats();
#endif
#if BIT_MPR_ALLOC_STACK
    static void monitorStack();
#else
    #define monitorStack()
#endif

/************************************* Code ***********************************/

PUBLIC Mpr *mprCreateMemService(MprManager manager, int flags)
{
    MprMem      *mp;
    MprRegion   *region;
    size_t      size, mprSize, spareSize, regionSize;

    getSystemInfo();
    size = MPR_PAGE_ALIGN(sizeof(MprHeap), memStats.pageSize);
    if ((heap = vmalloc(size, MPR_MAP_READ | MPR_MAP_WRITE)) == NULL) {
        return NULL;
    }
    memset(heap, 0, sizeof(MprHeap));
    heap->stats.numCpu = memStats.numCpu;
    heap->stats.pageSize = memStats.pageSize;
    heap->stats.maxHeap = (size_t) -1;
    heap->stats.warnHeap = ((size_t) -1) / 100 * 95;

    /*
        Hand-craft the Mpr structure from the first region. Free the remainder below.
     */
    mprSize = MPR_ALLOC_ALIGN(sizeof(MprMem) + sizeof(Mpr) + (MPR_MANAGER_SIZE * sizeof(void*)));
    regionSize = MPR_ALLOC_ALIGN(sizeof(MprRegion));
    size = max(mprSize + regionSize, BIT_MPR_ALLOC_REGION_SIZE);
    if ((region = mprVirtAlloc(size, MPR_MAP_READ | MPR_MAP_WRITE)) == NULL) {
        return NULL;
    }
    mp = region->start = (MprMem*) (((char*) region) + regionSize);
    region->end = (MprMem*) (((char*) region) + size);
    region->size = size;

    MPR = (Mpr*) GET_PTR(mp);
    initBlock(mp, mprSize, 1);
    SET_MANAGER(mp, manager);
    mprSetName(MPR, "Mpr");
    MPR->heap = heap;

    heap->flags = flags | MPR_THREAD_PATTERN;
    heap->nextSeqno = 1;
    heap->regionSize = BIT_MPR_ALLOC_REGION_SIZE;
    heap->stats.maxHeap = (size_t) -1;
    heap->stats.warnHeap = ((size_t) -1) / 100 * 95;
    heap->stats.cacheHeap = BIT_MPR_ALLOC_CACHE;
    heap->stats.lowHeap = max(BIT_MPR_ALLOC_CACHE / 8, BIT_MPR_ALLOC_REGION_SIZE);
    heap->workQuota = BIT_MPR_ALLOC_QUOTA;
    heap->enabled = !(heap->flags & MPR_DISABLE_GC);

    /* Internal testing use only */
    if (scmp(getenv("MPR_DISABLE_GC"), "1") == 0) {
        heap->enabled = 0;
    }
#if BIT_MPR_ALLOC_DEBUG
    if (scmp(getenv("MPR_SCRIBBLE_MEM"), "1") == 0) {
        heap->scribble = 1;
    }
    if (scmp(getenv("MPR_VERIFY_MEM"), "1") == 0) {
        heap->verify = 1;
    }
    if (scmp(getenv("MPR_TRACK_MEM"), "1") == 0) {
        heap->track = 1;
    }
#endif
    heap->stats.bytesAllocated += size;
    INC(allocs);
    initQueues();

    /*
        Free the remaining memory after MPR
     */
    spareSize = size - regionSize - mprSize;
    if (spareSize > 0) {
        linkSpareBlock(((char*) mp) + mprSize, spareSize);
        heap->regions = region;
    }
    heap->gcCond = mprCreateCond();
    heap->roots = mprCreateList(-1, MPR_LIST_STATIC_VALUES);
    mprAddRoot(MPR);
    return MPR;
}


static MPR_INLINE void initBlock(MprMem *mp, size_t size, int first)
{
    static MprMem empty = {0};

    *mp = empty;
    /* Implicit:  mp->free = 0; */
    mp->first = first;
    mp->mark = heap->mark;
    mp->size = (MprMemSize) size;
    SET_MAGIC(mp);
    SET_SEQ(mp);
    SET_NAME(mp, NULL);
    CHECK(mp);
}


PUBLIC void *mprAllocMem(size_t usize, int flags)
{
    MprMem      *mp;
    void        *ptr;
    size_t      size;
    int         padWords;

    assert(!heap->marking);

    padWords = padding[flags & MPR_ALLOC_PAD_MASK];
    size = usize + sizeof(MprMem) + (padWords * sizeof(void*));
    size = max(size, MPR_ALLOC_MIN_BLOCK);
    size = MPR_ALLOC_ALIGN(size);

    if ((mp = allocMem(size)) == NULL) {
        return NULL;
    }
    mp->hasManager = (flags & MPR_ALLOC_MANAGER) ? 1 : 0;
    ptr = GET_PTR(mp);
    if (flags & MPR_ALLOC_ZERO && !mp->fullRegion) {
        /* Regions are zeroed by vmalloc */
        memset(ptr, 0, GET_USIZE(mp));
    }
    CHECK(mp);
    monitorStack();
    return ptr;
}


/*
    Optimized allocation for blocks without managers or zeroing
 */
PUBLIC void *mprAllocFast(size_t usize)
{
    MprMem  *mp;
    size_t  size;

    size = usize + sizeof(MprMem);
    size = max(size, MPR_ALLOC_MIN_BLOCK);
    size = MPR_ALLOC_ALIGN(size);
    if ((mp = allocMem(size)) == NULL) {
        return NULL;
    }
    return GET_PTR(mp);
}


PUBLIC void *mprReallocMem(void *ptr, size_t usize)
{
    MprMem      *mp, *newb;
    void        *newptr;
    size_t      oldSize, oldUsize;

    assert(usize > 0);
    if (ptr == 0) {
        return mprAllocZeroed(usize);
    }
    mp = GET_MEM(ptr);
    CHECK(mp);

    oldUsize = GET_USIZE(mp);
    if (usize <= oldUsize) {
        return ptr;
    }
    if ((newptr = mprAllocMem(usize, mp->hasManager ? MPR_ALLOC_MANAGER : 0)) == NULL) {
        return 0;
    }
    newb = GET_MEM(newptr);
    if (mp->hasManager) {
        SET_MANAGER(newb, GET_MANAGER(mp));
    }
    oldSize = mp->size;
    memcpy(newptr, ptr, oldSize - sizeof(MprMem));
    /*
        New memory is zeroed
     */
    memset(&((char*) newptr)[oldUsize], 0, GET_USIZE(newb) - oldUsize);
    return newptr;
}


PUBLIC void *mprMemdupMem(cvoid *ptr, size_t usize)
{
    char    *newp;

    if ((newp = mprAllocMem(usize, 0)) != 0) {
        memcpy(newp, ptr, usize);
    }
    return newp;
}


PUBLIC int mprMemcmp(cvoid *s1, size_t s1Len, cvoid *s2, size_t s2Len)
{
    int         rc;

    assert(s1);
    assert(s2);
    assert(s1Len >= 0);
    assert(s2Len >= 0);

    if ((rc = memcmp(s1, s2, min(s1Len, s2Len))) == 0) {
        if (s1Len < s2Len) {
            return -1;
        } else if (s1Len > s2Len) {
            return 1;
        }
    }
    return rc;
}


/*
    mprMemcpy will support insitu copy where src and destination overlap
 */
PUBLIC size_t mprMemcpy(void *dest, size_t destMax, cvoid *src, size_t nbytes)
{
    assert(dest);
    assert(destMax <= 0 || destMax >= nbytes);
    assert(src);
    assert(nbytes >= 0);

    if (destMax > 0 && nbytes > destMax) {
        assert(!MPR_ERR_WONT_FIT);
        return 0;
    }
    if (nbytes > 0) {
        memmove(dest, src, nbytes);
        return nbytes;
    } else {
        return 0;
    }
}

/*************************** Allocator *************************/

static int initQueues() 
{
    MprFreeQueue    *freeq;
    int             qindex;

    for (freeq = heap->freeq, qindex = 0; freeq < &heap->freeq[MPR_ALLOC_NUM_QUEUES]; freeq++, qindex++) {
        /* Size includes MprMem header */
        freeq->minSize = (MprMemSize) qtosize(qindex);
#if (BIT_MPR_ALLOC_STATS && BIT_MPR_ALLOC_DEBUG)
        printf("Queue: %d, usize %u  size %u\n",
            (int) (freeq - heap->freeq), (int) freeq->minSize - (int) sizeof(MprMem), (int) freeq->minSize);
#endif
        assert(sizetoq(freeq->minSize) == qindex);
        freeq->next = freeq->prev = (MprFreeMem*) freeq;
        mprInitSpinLock(&freeq->lock);
    }
    return 0;
}


/*
    Memory allocator. This routine races with the sweeper.
 */
static MprMem *allocMem(size_t required)
{
    MprFreeQueue    *freeq;
    MprFreeMem      *fp;
    MprMem          *mp;
    size_t          *bitmap, localMap;
    int             baseBindex, bindex, qindex, retryIndex;

    ATOMIC_INC(requests);

    if ((qindex = sizetoq(required)) >= 0) {
        /*
            Check if the requested size is the smallest possible size in a queue. If not the smallest, must look at the 
            next queue higher up to guarantee a block of sufficient size. This implements a Good-fit strategy.
         */
        freeq = &heap->freeq[qindex];
        if (required > freeq->minSize) {
            if (++qindex >= MPR_ALLOC_NUM_QUEUES) {
                qindex = -1;
            } else {
                assert(required < heap->freeq[qindex].minSize);
            }
        }
    }
    if (qindex >= 0) {
        heap->workDone += qindex;
    retry:
        retryIndex = -1;
        baseBindex = qindex / MPR_ALLOC_BITMAP_BITS;
        bitmap = &heap->bitmap[baseBindex];

        /*
            Non-blocking search for a free block. If contention of any kind, simply skip the queue and try the next queue.
         */
        for (bindex = baseBindex; bindex < MPR_ALLOC_NUM_BITMAPS; bitmap++, bindex++) {
            /* Mask queues lower than the base queue */
            localMap = heap->bitmap[bindex] & ((size_t) -1 << max(0, (qindex - (MPR_ALLOC_BITMAP_BITS * bindex))));

            while (localMap) {
                qindex = (bindex * MPR_ALLOC_BITMAP_BITS) + findFirstBit(localMap) - 1;
                freeq = &heap->freeq[qindex];
                ATOMIC_INC(trys);
                if (freeq->next != (MprFreeMem*) freeq) {
                    if (acquire(freeq)) {
                        if (freeq->next != (MprFreeMem*) freeq) {
                            /* Inline unlinkBlock for speed */
                            fp = freeq->next;
                            fp->prev->next = fp->next;
                            fp->next->prev = fp->prev;
                            fp->blk.qindex = 0;
                            fp->blk.mark = heap->mark;
                            fp->blk.free = 0;
                            if (--freeq->count == 0) {
                                clearbitmap(bitmap, qindex % MPR_ALLOC_BITMAP_BITS);
                            }
                            mp = (MprMem*) fp;
                            release(freeq);
                            mprAtomicAdd64((int64*) &heap->stats.bytesFree, -(int64) mp->size);

                            if (mp->size >= (size_t) (required + MPR_ALLOC_MIN_SPLIT)) {
                                linkSpareBlock(((char*) mp) + required, mp->size - required);
                                mp->size = (MprMemSize) required;
                                ATOMIC_INC(splits);
                            }
                            if (heap->workDone > heap->workQuota && 
                                    heap->stats.bytesFree < heap->stats.lowHeap && !heap->gcRequested) {
                                triggerGC();
                            }
                            ATOMIC_INC(reuse);
                            assert(mp->size >= required);
                            return mp;
                        } else {
                            /* Someone beat us to the last block */
                            release(freeq);
                        }
                    } else {
                        ATOMIC_INC(tryFails);
                        if (freeq->count > 0 && retryIndex < 0) {
                            retryIndex = qindex;
                        }
                    }
                }
                /* Refresh the bitmap incase other threads have split or depleted suitable queues. +1 to clear current queue */
                localMap = heap->bitmap[bindex] & ((size_t) ((uint64) -1 << max(0, (qindex + 1 - (MPR_ALLOC_BITMAP_BITS * bindex)))));
                ATOMIC_INC(qrace);
            }
        }
        if (retryIndex >= 0) {
            /* Avoid growHeap if there is a suitable block in the heap */
            ATOMIC_INC(retries);
            qindex = retryIndex;
            goto retry;
        }
    }
    return growHeap(required);
}


/*
    Grow the heap and return a block of the required size (unqueued)
 */
static MprMem *growHeap(size_t required)
{
    MprRegion   *region;
    MprMem      *mp;
    size_t      size, rsize, spareLen;

    if (required < MPR_ALLOC_MAX_BLOCK && (heap->workDone > heap->workQuota)) {
        triggerGC();
    }
    if (required >= MPR_ALLOC_MAX) {
        allocException(MPR_MEM_TOO_BIG, required);
        return 0;
    }
    rsize = MPR_ALLOC_ALIGN(sizeof(MprRegion));
    size = max((size_t) required + rsize, (size_t) heap->regionSize);
    if ((region = mprVirtAlloc(size, MPR_MAP_READ | MPR_MAP_WRITE)) == NULL) {
        allocException(MPR_MEM_TOO_BIG, size);
        return 0;
    }
    region->size = size;
    region->start = (MprMem*) (((char*) region) + rsize);
    region->end = (MprMem*) ((char*) region + size);
    region->freeable = 0;
    mp = (MprMem*) region->start;
    spareLen = size - required - rsize;

    /*
        If a block is big, don't split the block. This improves the chances it will be unpinned.
     */
    if (spareLen < MPR_ALLOC_MIN_BLOCK || required >= MPR_ALLOC_MAX_BLOCK) {
        required = size - rsize; 
        spareLen = 0;
    }
    initBlock(mp, required, 1);
    if (spareLen > 0) {
        assert(spareLen >= MPR_ALLOC_MIN_BLOCK);
        linkSpareBlock(((char*) mp) + required, spareLen);
    } else {
        mp->fullRegion = 1;
    }
    mprAtomicListInsert((void**) &heap->regions, (void**) &region->next, region);
    ATOMIC_ADD(bytesAllocated, size);
    CHECK(mp);
    ATOMIC_INC(allocs);
    return mp;
}


static void freeBlock(MprMem *mp)
{
    MprRegion   *region;

    assert(!mp->free);
    SCRIBBLE(mp);
    INC(swept);
    freeLocation(mp);
#if BIT_MPR_ALLOC_STATS
    heap->stats.freed += mp->size;
#endif
    if (mp->first) {
        region = GET_REGION(mp);
        if (GET_NEXT(mp) >= region->end) {
            if (mp->fullRegion || heap->stats.bytesFree >= heap->stats.cacheHeap) {
                region->freeable = 1;
                return;
            }
        }
    }
    linkBlock(mp);
}


/*
    Map a queue index to a block size. This size includes the MprMem header.
 */
static MPR_INLINE size_t qtosize(int qindex)
{
    size_t  size;
    int     high, low;

    high = qindex / MPR_ALLOC_NUM_QBITS;
    low = qindex % MPR_ALLOC_NUM_QBITS;
    if (high) {
        low += MPR_ALLOC_NUM_QBITS;
    }
    high = max(0, high - 1);
    size = (low << high) << BIT_MPR_ALLOC_ALIGN_SHIFT;
    size += sizeof(MprMem);
    return size;
}


/*
    Map a block size to a queue index. The block size includes the MprMem header. However, determine the free queue 
    based on user sizes (sans header). This permits block searches to avoid scanning the next highest queue for 
    common block sizes: eg. 1K.
 */
static MPR_INLINE int sizetoq(size_t size)
{
    size_t      asize;
    int         msb, shift, high, low, qindex;

    assert(MPR_ALLOC_ALIGN(size) == size);

    if (size > MPR_ALLOC_MAX_BLOCK) {
        /* Large block, don't put on queues */
        return -1;
    }
    size -= sizeof(MprMem);
    asize = (size >> BIT_MPR_ALLOC_ALIGN_SHIFT);
    msb = findLastBit(asize) - 1;
    high = max(0, msb - MPR_ALLOC_QBITS_SHIFT + 1);
    shift = max(0, high - 1);
    low = (asize >> shift) & (MPR_ALLOC_NUM_QBITS - 1);
    qindex = (high * MPR_ALLOC_NUM_QBITS) + low;
    assert(qindex < MPR_ALLOC_NUM_QUEUES);
    return qindex;
}


/*
    Add a block to a free q. Called by user threads from allocMem and by sweeper from freeBlock.
    WARNING: Must be called with the freelist not acquired. This is the opposite of unlinkBlock.
 */
static MPR_INLINE bool linkBlock(MprMem *mp) 
{
    MprFreeQueue    *freeq;
    MprFreeMem      *fp;
    ssize           size;
    int             qindex;

    CHECK(mp);

    size = mp->size;
    qindex = sizetoq(size);
    assert(qindex >= 0);
    freeq = &heap->freeq[qindex];

    /*
        Acquire the free queue. Racing with multiple-threads in allocMem(). If we fail to acquire, the sweeper
        will retry next time. Note: the bitmap is updated with the queue acquired to safeguard the integrity of 
        this queue's free bit.
     */
    if (!acquire(freeq)) {
        ATOMIC_INC(tryFails);
        mp->mark = !mp->mark;
        assert(!mp->free);
        return 0;
    }
    assert(qindex >= 0);
    mp->qindex = qindex;
    mp->free = 1;
    mp->hasManager = 0;
    fp = (MprFreeMem*) mp;
    fp->next = freeq->next;
    fp->prev = (MprFreeMem*) freeq;
    freeq->next->prev = fp;
    freeq->next = fp;
    freeq->count++;
    setbitmap(&heap->bitmap[mp->qindex / MPR_ALLOC_BITMAP_BITS], mp->qindex % MPR_ALLOC_BITMAP_BITS);
    release(freeq);
    mprAtomicAdd64((int64*) &heap->stats.bytesFree, size);
    return 1;
}


/*
    Remove a block from a free q.
    WARNING: Must be called with the freelist acquired.
 */
static MPR_INLINE void unlinkBlock(MprMem *mp) 
{
    MprFreeQueue    *freeq;
    MprFreeMem      *fp;

    fp = (MprFreeMem*) mp;
    fp->prev->next = fp->next;
    fp->next->prev = fp->prev;
    assert(mp->qindex);
    freeq = &heap->freeq[mp->qindex];
    freeq->count--;
    mp->qindex = 0;
#if BIT_MPR_ALLOC_DEBUG
    fp->next = fp->prev = NULL;
#endif
    mprAtomicAdd64((int64*) &heap->stats.bytesFree, -(int64) mp->size);
}


/*
    This must be robust. i.e. the block spare memory must end up on the freeq
 */
static MPR_INLINE void linkSpareBlock(char *ptr, size_t size)
{ 
    MprMem  *mp;
    size_t  len;

    assert(size >= MPR_ALLOC_MIN_BLOCK);
    mp = (MprMem*) ptr;
    len = size;

    while (size > 0) {
        initBlock(mp, len, 0);
        if (!linkBlock(mp)) {
            /* Break into pieces and try lesser queue */
            if (len >= (MPR_ALLOC_MIN_BLOCK * 8)) {
                len = MPR_ALLOC_ALIGN(len / 2);
                len = min(size, len);
            }
        } else {
            size -= len;
            mp = (MprMem*) ((char*) mp + len);
            len = size;
        }
    } 
    assert(size == 0);
}


/*
    Allocate virtual memory and check a memory allocation request against configured maximums and redlines. 
    An application-wide memory allocation failure routine can be invoked from here when a memory redline is exceeded. 
    It is the application's responsibility to set the red-line value suitable for the system.
    Memory is zereod on all platforms.
 */
PUBLIC void *mprVirtAlloc(size_t size, int mode)
{
    size_t      used;
    void        *ptr;

    used = fastMemSize();
    if (memStats.pageSize) {
        size = MPR_PAGE_ALIGN(size, memStats.pageSize);
    }
    if ((size + used) > heap->stats.maxHeap) {
        allocException(MPR_MEM_LIMIT, size);

    } else if ((size + used) > heap->stats.warnHeap) {
        allocException(MPR_MEM_WARNING, size);
    }
    if ((ptr = vmalloc(size, mode)) == 0) {
        allocException(MPR_MEM_FAIL, size);
        return 0;
    }
    return ptr;
}


PUBLIC void mprVirtFree(void *ptr, size_t size)
{
    vmfree(ptr, size);
}


static void *vmalloc(size_t size, int mode)
{
    void    *ptr;

#if BIT_MPR_ALLOC_VIRTUAL
    #if BIT_UNIX_LIKE
        if ((ptr = mmap(0, size, mode, MAP_PRIVATE | MAP_ANON, -1, 0)) == (void*) -1) {
            return 0;
        }
    #elif BIT_WIN_LIKE
        ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, winPageModes(mode));
    #else
        if ((ptr = malloc(size)) != 0) {
            memset(ptr, 0, size);
        }
    #endif
#else
    if ((ptr = malloc(size)) != 0) {
        memset(ptr, 0, size);
    }
#endif
    return ptr;
}


static void vmfree(void *ptr, size_t size)
{
#if BIT_MPR_ALLOC_VIRTUAL
    #if BIT_UNIX_LIKE
        if (munmap(ptr, size) != 0) {
            assert(0);
        }
    #elif BIT_WIN_LIKE
        VirtualFree(ptr, 0, MEM_RELEASE);
    #else
        if (heap->scribble) {
            memset(ptr, 0x11, size);
        }
        free(ptr);
    #endif
#else
    free(ptr);
#endif
}


/***************************************************** Garbage Colllector *************************************************/

PUBLIC void mprStartGCService()
{
    if (heap->enabled) {
        if (heap->flags & MPR_SWEEP_THREAD) {
            mprTrace(7, "DEBUG: startMemWorkers: start marker");
            if ((heap->gc = mprCreateThread("sweeper", gc, NULL, 0)) == 0) {
                mprError("Cannot create marker thread");
                MPR->hasError = 1;
            } else {
                mprStartThread(heap->gc);
            }
        }
    }
}


PUBLIC void mprStopGCService()
{
    int     i;

    mprWakeGCService();
    for (i = 0; heap->gc && i < MPR_TIMEOUT_STOP; i++) {
        mprNap(1);
    }
}


PUBLIC void mprWakeGCService()
{
    mprSignalCond(heap->gcCond);
}


static MPR_INLINE void triggerGC()
{
    if (!heap->gcRequested) {
        if ((heap->flags & MPR_SWEEP_THREAD) && heap->gcCond) {
            heap->gcRequested = 1;
            mprSignalCond(heap->gcCond);
        }
    }
}


/*
    Trigger a GC collection worthwhile. If MPR_GC_FORCE is set, force the collection regardless. Flags:

    MPR_CG_DEFAULT      0x0     run GC if necessary. Will yield and block for GC. Won't wait for GC to fully complete.
    MPR_GC_FORCE        0x1     force a GC whether it is required or not
    MPR_GC_NO_BLOCK     0x2     dont wait for the GC complete
 */
PUBLIC void mprRequestGC(int flags)
{
    mprTrace(7, "DEBUG: mprRequestGC");

    if ((flags & MPR_GC_FORCE) || (heap->workDone > heap->workQuota)) {
        triggerGC();
    }
    if (!(flags & MPR_GC_NO_BLOCK)) {
        mprYield((flags & MPR_GC_COMPLETE) ? MPR_YIELD_COMPLETE : 0);
    }
}


/*
    Sweeper main thread
 */
static void gc(void *unused, MprThread *tp)
{
    mprTrace(5, "Sweeper thread started");
    tp->stickyYield = 1;
    tp->yielded = 1;

    while (!mprIsFinished()) {
        if (!heap->mustYield) {
            mprWaitForCond(heap->gcCond, -1);
            if (mprIsFinished()) {
                break;
            }
        }
        markAndSweep();
    }
    invokeDestructors();
    heap->gc = 0;
    resumeThreads(YIELDED_THREADS | WAITING_THREADS);
}


/*
    The mark phase will run with all user threads yielded. The sweep phase then runs in parallel.
    The mark phase is relatively quick.
 */
static void markAndSweep()
{
    static int warnOnce = 0;

    mprTrace(7, "GC: mark started");
    heap->mustYield = 1;

    if (!pauseThreads()) {
        if (warnOnce++ == 0) {
            mprTrace(7, "GC synchronization timed out, some threads did not yield.");
            mprTrace(7, "This is most often caused by a thread doing a long running operation and not first calling mprYield.");
            mprTrace(7, "If debugging, run the process with -D to enable debug mode.");
        }
        heap->gcRequested = 0;
        resumeThreads(YIELDED_THREADS | WAITING_THREADS);
        return;
    }
    INC(collections);
    heap->gcRequested = 0;
    heap->priorWeightedCount = heap->workDone;
    heap->workDone = 0;
#if BIT_MPR_ALLOC_STATS
    heap->priorFree = heap->stats.bytesFree;
#endif

    /*
        Mark all roots
     */
    heap->mark = !heap->mark;
    MPR_MEASURE(BIT_MPR_ALLOC_LEVEL, "GC", "mark", markRoots());
    heap->sweeping = 1;
    mprAtomicBarrier();
    heap->marking = 0;

#if BIT_MPR_ALLOC_PARALLEL
    resumeThreads(YIELDED_THREADS);
#endif
    /*
        Sweep unused memory with user threads resumed
     */
    MPR_MEASURE(BIT_MPR_ALLOC_LEVEL, "GC", "sweep", sweep());
    heap->sweeping = 0;

#if BIT_MPR_ALLOC_PARALLEL
    resumeThreads(WAITING_THREADS);
#else
    resumeThreads(YIELDED_THREADS | WAITING_THREADS);
#endif
}


static void invokeDestructors()
{
    MprRegion   *region;
    MprMem      *mp;
    MprManager  mgr;

    for (region = heap->regions; region; region = region->next) {
        for (mp = region->start; mp < region->end; mp = GET_NEXT(mp)) {
            /*
                OPT - could optimize by requiring a separate flag for managers that implement destructors.
             */
            if (mp->mark != heap->mark && !mp->free && mp->hasManager && !mp->eternal) {
                mgr = GET_MANAGER(mp);
                if (mgr) {
                    (mgr)(GET_PTR(mp), MPR_MANAGE_FREE);
                    /* Retest incase the manager routine revied the object */
                    if (mp->mark != heap->mark) {
                        mp->hasManager = 0;
                    }
                }
            }
        }
    }
}


/*
    Claim a block from its freeq for the sweeper. This removes the block from the freeq and clears the "free" bit.
 */
static MPR_INLINE bool claim(MprMem *mp)
{
    MprFreeQueue    *freeq;
    int             qindex;

    if ((qindex = mp->qindex) == 0) {
        /* allocator won the race */
        return 0;
    }
    freeq = &heap->freeq[qindex];
    ATOMIC_INC(trys);
    if (!acquire(freeq)) {
        ATOMIC_INC(tryFails);
        return 0;
    }
    if (mp->qindex != qindex) {
        /* No on this queue. Allocator must have claimed this block */
        release(freeq);
        return 0;
    }
    unlinkBlock(mp);
    assert(mp->free);
    mp->free = 0;
    release(freeq);
    return 1;
}


/*
    Sweep up the garbage. The sweeper runs in parallel with the program. Dead blocks will have (MprMem.mark != heap->mark). 
*/
static void sweep()
{
    MprRegion   *region, *nextRegion, *prior, *rp;
    MprMem      *mp, *next;
    int         joinBlocks;

    if (!heap->enabled) {
        mprTrace(0, "DEBUG: sweep: Abort sweep - GC disabled");
        return;
    }
    mprTrace(7, "GC: sweep started");
#if BIT_MPR_ALLOC_STATS
    heap->stats.sweepVisited = 0;
    heap->stats.swept = 0;
    heap->stats.freed = 0;
#endif
    /*
        First run managers so that dependant memory blocks will still exist when the manager executes.
        Actually free the memory in a 2nd pass below. 
     */
    invokeDestructors();

    /*
        RACE: Racing with growHeap. This traverses the region list lock-free. growHeap() will insert new regions to 
        the front of heap->regions. This code is the only code that frees regions.
     */
    prior = NULL;
    for (region = heap->regions; region; region = nextRegion) {
        nextRegion = region->next;
        joinBlocks = heap->stats.bytesFree >= heap->stats.cacheHeap;

        for (mp = region->start; mp < region->end; mp = next) {
            next = GET_NEXT(mp);
            assert(next != mp);
            CHECK(mp);
            INC(sweepVisited);

            if (mp->eternal) {
                assert(!region->freeable);
                continue;
            } 
            if (mp->free && joinBlocks) {
                if (next < region->end && !next->free && next->mark != heap->mark && claim(mp)) {
                    mp->mark = !heap->mark;
                    INC(compacted);
                }
            }
            if (!mp->free && mp->mark != heap->mark) {
                if (joinBlocks) {
                    while (next < region->end && !next->eternal) {
                        if (next->free) {
                            if (!claim(next)) {
                                break;
                            }
                            mp->size += next->size;
                            freeLocation(next);
                            assert(!next->free);
                            SCRIBBLE_RANGE(next, MPR_ALLOC_MIN_BLOCK);
                            INC(joins);

                        } else if (next->mark != heap->mark) {
                            assert(!next->free);
                            assert(next->qindex == 0);
                            mp->size += next->size;
                            freeLocation(next);
                            SCRIBBLE_RANGE(next, MPR_ALLOC_MIN_BLOCK);
                            INC(joins);

                        } else {
                            break;
                        }
                        next = GET_NEXT(mp);
                    }
                }
                freeBlock(mp);
            }
        }
        if (region->freeable) {
            if (prior) {
                prior->next = nextRegion;
            } else {
                if (!mprAtomicCas((void**) &heap->regions, region, nextRegion)) {
                    prior = 0;
                    for (rp = heap->regions; rp != region; prior = rp, rp = rp->next) { }
                    assert(prior);
                    if (prior) {
                        prior->next = nextRegion;
                    }
                }
            }
            ATOMIC_ADD(bytesAllocated, -region->size);
            mprTrace(9, "DEBUG: Unpin %p to %p size %d, used %d", region, ((char*) region) + region->size, 
                region->size, fastMemSize());
            mprVirtFree(region, region->size);
            INC(unpins);
        } else {
            prior = region;
        }
    }
#if (BIT_MPR_ALLOC_STATS && BIT_MPR_ALLOC_DEBUG) && UNUSED
    printf("GC: Marked %lld / %lld, Swept %lld / %lld, freed %lld, bytesFree %lld (prior %lld)\n"
                 "    WeightedCount %d / %d, allocated blocks %lld allocated bytes %lld\n"
                 "    Unpins %lld, Collections %lld\n",
        heap->stats.marked, heap->stats.markVisited, heap->stats.swept, heap->stats.sweepVisited, 
        heap->stats.freed, heap->stats.bytesFree, heap->priorFree, heap->priorWeightedCount, heap->workQuota,
        heap->stats.sweepVisited - heap->stats.swept, heap->stats.bytesAllocated, heap->stats.unpins, 
        heap->stats.collections);
#endif
    if (heap->printStats) {
        printMemReport();
        heap->printStats = 0;
    }
}


static void markRoots()
{
    void    *root;
    int     next;

#if BIT_MPR_ALLOC_STATS
    heap->stats.markVisited = 0;
    heap->stats.marked = 0;
#endif
    mprMark(heap->roots);
    mprMark(heap->gcCond);

    for (ITERATE_ITEMS(heap->roots, root, next)) {
        mprMark(root);
    }
}


/*
    Permanent allocation. Immune to garbage collector.
 */
void *palloc(size_t size)
{
    void    *ptr;

    if ((ptr = mprAllocZeroed(size)) != 0) {
        mprHold(ptr);
    }
    return ptr;
}


/*
    Normal free. Note: this must not be called with a block allocated via "malloc".
    No harm in calling this on a block allocated with mprAlloc and not "palloc".
 */
PUBLIC void pfree(void *ptr)
{
    if (ptr) {
        mprRelease(ptr);

    }
}


PUBLIC void *prealloc(void *ptr, size_t size)
{
    if (ptr) {
        mprRelease(ptr);
    }
    if ((ptr =  mprRealloc(ptr, size)) != 0) {
        mprHold(ptr);
    }
    return ptr;
}


PUBLIC size_t psize(void *ptr)
{
    return mprGetBlockSize(ptr);
}


/* 
    WARNING: this does not mark component members. If that is required, use mprAddRoot.
 */
PUBLIC void mprHold(cvoid *ptr)
{
    MprMem  *mp;

    if (ptr) {
        mp = GET_MEM(ptr);
        if (!mp->free && VALID_BLK(mp)) {
            mp->eternal = 1;
        }
    }
}


PUBLIC void mprRelease(cvoid *ptr)
{
    MprMem  *mp;

    if (ptr) {
        mp = GET_MEM(ptr);
        if (!mp->free && VALID_BLK(mp)) {
            mp->eternal = 0;
        }
    }
}


PUBLIC int mprCreateEventOutside(MprDispatcher *dispatcher, void *proc, void *data)
{
    MprEvent    *event;

    mprAtomicAdd((int*) &heap->pauseGC, 1);
    mprAtomicBarrier();
    while (heap->mustYield) {
        mprNap(0);
    }
    event = mprCreateEvent(dispatcher, "relay", 0, proc, data, MPR_EVENT_STATIC_DATA);
    mprAtomicAdd((int*) &heap->pauseGC, -1);
    if (!event) {
        return MPR_ERR_CANT_CREATE;
    }
    return 0;
}


/*
    Called by user code to signify the thread is ready for GC and all object references are saved.  Flags:

        MPR_YIELD_DEFAULT   Yield and only if GC is required, block for GC. Otherwise return without blocking.
        MPR_YIELD_BLOCK     Yield and wait until the next GC starts and resumes user threads, regardless of whether GC is required.
        MPR_YIELD_COMPLETE  Yield and wait until the GC entirely complete including sweeper.
        MPR_YIELD_STICKY    Yield and remain yielded until reset. Does not block by default.
 */
PUBLIC void mprYield(int flags)
{
    MprThreadService    *ts;
    MprThread           *tp;

    ts = MPR->threadService;
    if ((tp = mprGetCurrentThread()) == 0) {
        mprError("Yield called from an unknown thread");
        /* Called from a non-mpr thread */
        return;
    }
    tp->yielded = 1;
    if (flags & MPR_YIELD_STICKY) {
        tp->stickyYield = 1;
    }
    tp->waitForGC = (flags & MPR_YIELD_COMPLETE) ? 1 : 0;

    if (flags & MPR_YIELD_COMPLETE) {
        flags |= MPR_YIELD_BLOCK;
    }
    while (tp->yielded && (heap->mustYield || (flags & MPR_YIELD_BLOCK))) {
        if (heap->flags & MPR_SWEEP_THREAD) {
            mprSignalCond(ts->cond);
        }
        if (tp->stickyYield) {
            return;
        }
        mprWaitForCond(tp->cond, -1);
        if (!tp->waitForGC) {
            flags &= ~MPR_YIELD_BLOCK;
        }
    }
    if (!tp->stickyYield) {
        tp->yielded = 0;
    }
    assert(!heap->marking);
}


PUBLIC void mprResetYield()
{
    MprThreadService    *ts;
    MprThread           *tp;

    ts = MPR->threadService;
    if ((tp = mprGetCurrentThread()) != 0) {
        tp->stickyYield = 0;
    }
    /*
        May have been sticky yielded and so marking could have started again. If so, must yield here regardless.
        If GC being requested, then do a blocking pause here.
     */
    lock(ts->threads);
    if (heap->mustYield && (heap->marking || (heap->sweeping && !BIT_MPR_ALLOC_PARALLEL))) {
        unlock(ts->threads);
        mprYield(0);
    } else {
        if (tp) {
            tp->yielded = 0;
        }
        unlock(ts->threads);
    }
}


/*
    Pause until all threads have yielded. Called by the GC marker only.
    NOTE: this functions differently if parallel. If so, then it will abort waiting. If !parallel, it waits for all
    threads to yield.
 */
static int pauseThreads()
{
    MprThreadService    *ts;
    MprThread           *tp;
    MprTicks            start;
    int                 i, allYielded, timeout;

#if BIT_MPR_TRACING
    uint64  hticks = mprGetHiResTicks();
#endif
    ts = MPR->threadService;
    timeout = MPR_TIMEOUT_GC_SYNC;

    mprTrace(7, "pauseThreads: wait for threads to yield, timeout %d", timeout);
    start = mprGetTicks();
    if (mprGetDebugMode()) {
        timeout = timeout * 500;
    }
    do {
        lock(ts->threads);
        if (!heap->pauseGC) {
            allYielded = 1;
            for (i = 0; i < ts->threads->length; i++) {
                tp = (MprThread*) mprGetItem(ts->threads, i);
                if (!tp->yielded) {
                    allYielded = 0;
                    if (mprGetElapsedTicks(start) > 1000) {
                        mprTrace(7, "Thread %s is not yielding", tp->name);
                    }
                    break;
                }
            }
            if (allYielded) {
                heap->marking = 1;
                unlock(ts->threads);
                break;
            }
        } else {
            allYielded = 0;
        }
        unlock(ts->threads);
        mprTrace(7, "pauseThreads: waiting for threads to yield");
        mprWaitForCond(ts->cond, 20);

    } while (!allYielded && mprGetElapsedTicks(start) < timeout);

#if BIT_MPR_TRACING
    mprTrace(7, "TIME: pauseThreads elapsed %,Ld msec, %,Ld hticks", mprGetElapsedTicks(start), mprGetHiResTicks() - hticks);
#endif
    if (allYielded) {
        CHECK_YIELDED();
    }
    return (allYielded) ? 1 : 0;
}


static void resumeThreads(int flags)
{
    MprThreadService    *ts;
    MprThread           *tp;
    int                 i;

    ts = MPR->threadService;
    lock(ts->threads);
    heap->mustYield = 0;
    for (i = 0; i < ts->threads->length; i++) {
        tp = (MprThread*) mprGetItem(ts->threads, i);
        if (tp && tp->yielded) {
            if (flags == WAITING_THREADS && !tp->waitForGC) {
                continue;
            }
            if (flags == YIELDED_THREADS && tp->waitForGC) {
                continue;
            }
            if (!tp->stickyYield) {
                tp->yielded = 0;
            }
            tp->waitForGC = 0;
            mprSignalCond(tp->cond);
        }
    }
    unlock(ts->threads);
}


PUBLIC bool mprEnableGC(bool on)
{
    bool    old;

    old = heap->enabled;
    heap->enabled = on;
    return old;
}


PUBLIC void mprAddRoot(cvoid *root)
{
    mprAddItem(heap->roots, root);
}


PUBLIC void mprRemoveRoot(cvoid *root)
{
    mprRemoveItem(heap->roots, root);
}


/****************************************************** Debug *************************************************************/

#if BIT_MPR_ALLOC_STATS
static void printQueueStats() 
{
    MprFreeQueue    *freeq;
    int             i;

    /*
        Note the total size is a minimum as blocks may be larger than minSize
     */
    printf("\nFree Queue Stats\n  Queue           Usize         Count          Total\n");
    for (i = 0, freeq = heap->freeq; freeq < &heap->freeq[MPR_ALLOC_NUM_QUEUES]; freeq++, i++) {
        if (freeq->count) {
            printf("%7d %14d %14d %14d\n", i, freeq->minSize - (int) sizeof(MprMem), freeq->count, 
                freeq->minSize * freeq->count);
        }
    }
}


#if BIT_MPR_ALLOC_DEBUG
static MprLocationStats sortLocations[MPR_TRACK_HASH];

static int sortLocation(cvoid *l1, cvoid *l2)
{
    MprLocationStats    *lp1, *lp2;

    lp1 = (MprLocationStats*) l1;
    lp2 = (MprLocationStats*) l2;
    if (lp1->count < lp2->count) {
        return -1;
    } else if (lp1->count == lp2->count) {
        return 0;
    }
    return 1;
}


static void printTracking() 
{
    MprLocationStats     *lp;
    cchar                **np;

    printf("\nAllocation Stats\n     Size Location\n");
    memcpy(sortLocations, heap->stats.locations, sizeof(sortLocations));
    qsort(sortLocations, MPR_TRACK_HASH, sizeof(MprLocationStats), sortLocation);

    for (lp = sortLocations; lp < &sortLocations[MPR_TRACK_HASH]; lp++) {
        if (lp->count) {
            for (np = &lp->names[0]; *np && np < &lp->names[MPR_TRACK_NAMES]; np++) {
                if (*np) {
                    if (np == lp->names) {
                        printf("%10d %-24s\n", (int) lp->count, *np);
                    } else {
                        printf("           %-24s\n", *np);
                    }
                }
            }
        }
    }
}
#endif /* BIT_MPR_ALLOC_DEBUG */


static void printGCStats()
{
    MprRegion   *region;
    MprMem      *mp;
    size_t      freeBytes, activeBytes, eternalBytes, regionBytes, available;
    char        *tag;
    int         regions, freeCount, activeCount, eternalCount, regionCount, empty;

    printf("\nRegion Stats\n");
    regions = 0;
    activeBytes = eternalBytes = freeBytes = 0;
    activeCount = eternalCount = freeCount = 0;

    for (region = heap->regions; region; region = region->next, regions++) {
        regionCount = 0;
        regionBytes = 0;
        empty = 1;

        for (mp = region->start; mp < region->end; mp = GET_NEXT(mp)) {
            if (mp->free) {
                freeBytes += mp->size;
                freeCount++;

            } else if (mp->eternal) {
                eternalBytes += mp->size;
                eternalCount++;
                regionCount++;
                regionBytes += mp->size;
                empty = 0;

            } else {
                activeBytes += mp->size;
                activeCount++;
                regionCount++;
                regionBytes += mp->size;
                empty = 0;
            }
        }
        available = region->size - regionBytes - MPR_ALLOC_ALIGN(sizeof(MprRegion));
        if (available == 0) {
            tag = "(fully used)";
        } else if (regionBytes == 0) {
            tag = "(empty)";
        } else {
            tag = "";
        }
        printf("  Region %3d size %d, allocated %4d blocks, %7d bytes free %s\n", regions, (int) region->size, 
            regionCount, (int) available, tag);
    }
    printf("\nGC Stats\n");
    printf("  Active:  %9d blocks, %12ld bytes\n", activeCount, activeBytes);
    printf("  Eternal: %9d blocks, %12ld bytes\n", eternalCount, eternalBytes);
    printf("  Free:    %9d blocks, %12ld bytes\n", freeCount, freeBytes);
}
#endif /* BIT_MPR_ALLOC_STATS */


PUBLIC void mprPrintMem(cchar *msg, int flags)
{
    int     gflags;

    printf("\n%s\n", msg);
    printf("-------------\n");
    heap->printStats = (flags & MPR_MEM_DETAIL) ? 2 : 1;
    gflags = MPR_GC_FORCE | MPR_GC_COMPLETE;
    mprRequestGC(gflags);
}


static void printMemReport()
{
    MprMemStats     *ap;

    ap = mprGetMemStats();

    printf("  Total app memory  %14u K\n",             (int) (mprGetMem() / 1024));
    printf("  Allocated memory  %14u K\n",             (int) (ap->bytesAllocated / 1024));
    printf("  Free heap memory  %14u K\n",             (int) (ap->bytesFree / 1024));

    if (ap->maxHeap == (size_t) -1) {
        printf("  Memory limit           unlimited\n");
        printf("  Memory redline         unlimited\n");
    } else {
        printf("  Heap max          %14u MB (%.2f %%)\n", 
            (int) (ap->maxHeap / (1024 * 1024)), ap->bytesAllocated * 100.0 / ap->maxHeap);
        printf("  Heap redline      %14u MB (%.2f %%)\n", 
            (int) (ap->warnHeap / (1024 * 1024)), ap->bytesAllocated * 100.0 / ap->warnHeap);
    }
    printf("  Heap cache        %14u MB (%.2f %%)\n",    (int) (ap->cacheHeap / (1024 * 1024)), ap->cacheHeap * 100.0 / ap->maxHeap);
    printf("  Allocation errors %14d\n",               (int) ap->errors);
    printf("\n");

#if BIT_MPR_ALLOC_STATS
    printf("  Memory requests   %14d\n",                (int) ap->requests);
    printf("  Region allocs     %14.2f %% (%d)\n",      ap->allocs * 100.0 / ap->requests, (int) ap->allocs);
    printf("  Region unpins     %14.2f %% (%d)\n",      ap->unpins * 100.0 / ap->requests, (int) ap->unpins);
    printf("  Reuse             %14.2f %%\n",           ap->reuse * 100.0 / ap->requests);
    printf("  Joins             %14.2f %% (%d)\n",      ap->joins * 100.0 / ap->requests, (int) ap->joins);
    printf("  Splits            %14.2f %% (%d)\n",      ap->splits * 100.0 / ap->requests, (int) ap->splits);
    printf("  Q races           %14.2f %% (%d)\n",      ap->qrace * 100.0 / ap->requests, (int) ap->qrace);
    printf("  Compacted         %14.2f %% (%d)\n",      ap->compacted * 100.0 / ap->requests, (int) ap->compacted);
    printf("  Freeq failures    %14.2f %% (%d / %d)\n", ap->tryFails * 100.0 / ap->trys, (int) ap->tryFails, (int) ap->trys);
    printf("  Alloc retries     %14.2f %% (%d / %d)\n", ap->retries * 100.0 / ap->requests, (int) ap->retries, (int) ap->requests);
    printf("  GC Collections    %14.2f %% (%d)\n",      ap->collections * 100.0 / ap->requests, (int) ap->collections);
    printf("  MprMem size       %14d\n",                (int) sizeof(MprMem));
    printf("  MprFreeMem size   %14d\n",                (int) sizeof(MprFreeMem));

    printGCStats();
    if (heap->printStats > 1) {
        printQueueStats();
#if BIT_MPR_ALLOC_DEBUG
        if (heap->track) {
            printTracking();
        }
#endif
    }
#endif /* BIT_MPR_ALLOC_STATS */
}


#if BIT_MPR_ALLOC_DEBUG
static int validBlk(MprMem *mp)
{
    assert(mp->magic == MPR_ALLOC_MAGIC);
    assert(mp->size > 0);
    return (mp->magic == MPR_ALLOC_MAGIC) && (mp->size > 0);
}


PUBLIC void mprCheckBlock(MprMem *mp)
{
    BREAKPOINT(mp);
    if (mp->magic != MPR_ALLOC_MAGIC || mp->size == 0) {
        mprError("Memory corruption in memory block %x (MprBlk %x, seqno %d)\n"
            "This most likely happend earlier in the program execution.", GET_PTR(mp), mp, mp->seqno);
    }
}


static void breakpoint(MprMem *mp) 
{
    if (mp == stopAlloc || mp->seqno == stopSeqno) {
        mprBreakpoint();
    }
}


#if BIT_MPR_ALLOC_DEBUG
/*
    Called to set the memory block name when doing an allocation
 */
PUBLIC void *mprSetAllocName(void *ptr, cchar *name)
{
    MPR_GET_MEM(ptr)->name = name;

    if (heap->track) {
        MprLocationStats    *lp;
        cchar               **np, *n;
        int                 index;
        if (name == 0) {
            name = "";
        }
        index = shash(name, strlen(name)) % MPR_TRACK_HASH;
        lp = &heap->stats.locations[index];
        for (np = lp->names; np <= &lp->names[MPR_TRACK_NAMES]; np++) {
            n = *np;
            if (n == 0 || n == name || strcmp(n, name) == 0) {
                break;
            }
        }
        if (np < &lp->names[MPR_TRACK_NAMES]) {
            *np = (char*) name;
        }
        lp->count += GET_MEM(ptr)->size;
    }
    return ptr;
}


static void freeLocation(MprMem *mp)
{
    MprLocationStats    *lp;
    cchar               *name;
    int                 index, i;

    if (!heap->track) {
        return;
    }
    name = mp->name;
    if (name == 0) {
        name = "";
    }
    index = shash(name, strlen(name)) % MPR_TRACK_HASH;
    lp = &heap->stats.locations[index];
    if (lp->count >= mp->size) {
        lp->count -= mp->size;
    } else {
        lp->count = 0;
    }
    if (lp->count == 0) {
        for (i = 0; i < MPR_TRACK_NAMES; i++) {
            lp->names[i] = 0;
        }
    }
}
#endif


PUBLIC void *mprSetName(void *ptr, cchar *name) 
{
    MprMem  *mp = GET_MEM(ptr);
    if (mp->name) {
        freeLocation(mp);
        mprSetAllocName(ptr, name);
    }
    return ptr;
}


PUBLIC void *mprCopyName(void *dest, void *src) 
{
    return mprSetName(dest, mprGetName(src));
}
#endif

/********************************************* Misc ***************************************************/

static void allocException(int cause, size_t size)
{
    size_t  used;

    INC(errors);
    if (heap->stats.inMemException || mprIsStopping()) {
        return;
    }
    heap->stats.inMemException = 1;
    used = fastMemSize();

    if (cause == MPR_MEM_FAIL) {
        heap->hasError = 1;
        mprError("%s: Cannot allocate memory block of size %,Ld bytes.", MPR->name, size);

    } else if (cause == MPR_MEM_TOO_BIG) {
        heap->hasError = 1;
        mprError("%s: Cannot allocate memory block of size %,Ld bytes.", MPR->name, size);

    } else if (cause == MPR_MEM_WARNING) {
        mprError("%s: Memory request for %,Ld bytes exceeds memory red-line.", MPR->name, size);
        mprPruneCache(NULL);

    } else if (cause == MPR_MEM_LIMIT) {
        mprError("%s: Memory request for %,d bytes exceeds memory limit.", MPR->name, size);
    }
    mprError("%s: Memory used %,d, redline %,d, limit %,d.", MPR->name, (int) used, (int) heap->stats.warnHeap,
        (int) heap->stats.maxHeap);
    mprError("%s: Consider increasing memory limit.", MPR->name);

    if (heap->notifier) {
        (heap->notifier)(cause, heap->allocPolicy,  size, used);
    }
    if (cause & (MPR_MEM_TOO_BIG | MPR_MEM_FAIL)) {
        /*
            Allocation failed
         */
        mprError("Application exiting immediately due to memory depletion.");
        mprTerminate(MPR_EXIT_IMMEDIATE, 2);

    } else if (cause & MPR_MEM_LIMIT) {
        if (heap->allocPolicy == MPR_ALLOC_POLICY_RESTART) {
            mprError("Application restarting due to low memory condition.");
            mprTerminate(MPR_EXIT_GRACEFUL | MPR_EXIT_RESTART, 1);

        } else if (heap->allocPolicy == MPR_ALLOC_POLICY_EXIT) {
            mprError("Application exiting immediately due to memory depletion.");
            mprTerminate(MPR_EXIT_IMMEDIATE, 2);
        }
    }
    heap->stats.inMemException = 0;
}


static void getSystemInfo()
{
    memStats.numCpu = 1;

#if MACOSX
    #ifdef _SC_NPROCESSORS_ONLN
        memStats.numCpu = (uint) sysconf(_SC_NPROCESSORS_ONLN);
    #else
        memStats.numCpu = 1;
    #endif
    memStats.pageSize = (uint) sysconf(_SC_PAGESIZE);
#elif SOLARIS
{
    FILE *ptr;
    if  ((ptr = popen("psrinfo -p", "r")) != NULL) {
        fscanf(ptr, "%d", &alloc.numCpu);
        (void) pclose(ptr);
    }
    alloc.pageSize = sysconf(_SC_PAGESIZE);
}
#elif BIT_WIN_LIKE
{
    SYSTEM_INFO     info;

    GetSystemInfo(&info);
    memStats.numCpu = info.dwNumberOfProcessors;
    memStats.pageSize = info.dwPageSize;

}
#elif BIT_BSD_LIKE
    {
        int     cmd[2];
        size_t  len;

        cmd[0] = CTL_HW;
        cmd[1] = HW_NCPU;
        len = sizeof(memStats.numCpu);
        memStats.numCpu = 0;
        if (sysctl(cmd, 2, &memStats.numCpu, &len, 0, 0) < 0) {
            memStats.numCpu = 1;
        }
        memStats.pageSize = sysconf(_SC_PAGESIZE);
    }
#elif LINUX
    {
        static const char processor[] = "processor\t:";
        char    c;
        int     fd, col, match;

        fd = open("/proc/cpuinfo", O_RDONLY);
        if (fd < 0) {
            return;
        }
        match = 1;
        memStats.numCpu = 0;
        for (col = 0; read(fd, &c, 1) == 1; ) {
            if (c == '\n') {
                col = 0;
                match = 1;
            } else {
                if (match && col < (sizeof(processor) - 1)) {
                    if (c != processor[col]) {
                        match = 0;
                    }
                    col++;

                } else if (match) {
                    memStats.numCpu++;
                    match = 0;
                }
            }
        }
        if (memStats.numCpu <= 0) {
            memStats.numCpu = 1;
        }
        close(fd);
        memStats.pageSize = sysconf(_SC_PAGESIZE);
    }
#else
        memStats.pageSize = 4096;
#endif
    if (memStats.pageSize <= 0 || memStats.pageSize >= (16 * 1024)) {
        memStats.pageSize = 4096;
    }
}


#if BIT_WIN_LIKE
static int winPageModes(int flags)
{
    if (flags & MPR_MAP_EXECUTE) {
        return PAGE_EXECUTE_READWRITE;
    } else if (flags & MPR_MAP_WRITE) {
        return PAGE_READWRITE;
    }
    return PAGE_READONLY;
}
#endif


PUBLIC MprMemStats *mprGetMemStats()
{
#if LINUX
    char            buf[1024], *cp;
    size_t          len;
    int             fd;

    heap->stats.ram = MAXSSIZE;
    if ((fd = open("/proc/meminfo", O_RDONLY)) >= 0) {
        if ((len = read(fd, buf, sizeof(buf) - 1)) > 0) {
            buf[len] = '\0';
            if ((cp = strstr(buf, "MemTotal:")) != 0) {
                for (; *cp && !isdigit((uchar) *cp); cp++) {}
                heap->stats.ram = ((size_t) atoi(cp) * 1024);
            }
        }
        close(fd);
    }
#endif
#if BIT_BSD_LIKE
    size_t      len;
    int         mib[2];
#if FREEBSD
    size_t      ram, usermem;
    mib[1] = HW_MEMSIZE;
#else
    int64 ram, usermem;
    mib[1] = HW_PHYSMEM;
#endif
    mib[0] = CTL_HW;
    len = sizeof(ram);
    ram = 0;
    sysctl(mib, 2, &ram, &len, NULL, 0);
    heap->stats.ram = ram;

    mib[0] = CTL_HW;
    mib[1] = HW_USERMEM;
    len = sizeof(usermem);
    usermem = 0;
    sysctl(mib, 2, &usermem, &len, NULL, 0);
    heap->stats.user = usermem;
#endif
    heap->stats.rss = mprGetMem();
    return &heap->stats;
}


/*
    Return the amount of memory currently in use. This routine may open files and thus is not very quick on some 
    platforms. On FREEBDS it returns the peak resident set size using getrusage. If a suitable O/S API is not available,
    the amount of heap memory allocated by the MPR is returned.
 */
PUBLIC size_t mprGetMem()
{
    size_t  size = 0;

#if LINUX
    int fd;
    char path[BIT_MAX_PATH];
    sprintf(path, "/proc/%d/status", getpid());
    if ((fd = open(path, O_RDONLY)) >= 0) {
        char buf[BIT_MAX_BUFFER], *tok;
        int nbytes = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (nbytes > 0) {
            buf[nbytes] = '\0';
            if ((tok = strstr(buf, "VmRSS:")) != 0) {
                for (tok += 6; tok && isspace((uchar) *tok); tok++) {}
                size = stoi(tok) * 1024;
            }
        }
    }
    if (size == 0) {
        struct rusage rusage;
        getrusage(RUSAGE_SELF, &rusage);
        size = rusage.ru_maxrss * 1024;
    }
#elif MACOSX
    struct task_basic_info info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t) &info, &count) == KERN_SUCCESS) {
        size = info.resident_size;
    }
#elif BIT_BSD_LIKE
    struct rusage   rusage;
    getrusage(RUSAGE_SELF, &rusage);
    size = rusage.ru_maxrss;
#endif
    if (size == 0) {
        size = (size_t) heap->stats.bytesAllocated;
    }
    return size;
}


/*
    Fast routine to teturn the approximately the amount of memory currently in use. If a fast method is not available,
    use the amount of heap memory allocated by the MPR.
    WARNING: this routine must be FAST as it is used by the MPR memory allocation mechanism when more memory is allocated
    from the O/S (i.e. not on every block allocation).
 */
static size_t fastMemSize()
{
    size_t      size = 0;

#if LINUX
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
    size = rusage.ru_maxrss * 1024;
#elif MACOSX
    struct task_basic_info info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t) &info, &count) == KERN_SUCCESS) {
        size = info.resident_size;
    }
#endif
    if (size == 0) {
        size = (size_t) heap->stats.bytesAllocated;
    }
    return size;
}


#ifndef findFirstBit
static MPR_INLINE int findFirstBit(size_t word)
{
    int     b;
    for (b = 0; word; word >>= 1, b++) {
        if (word & 0x1) {
            b++;
            break;
        }
    }
    return b;
}
#endif


#ifndef findLastBit
static MPR_INLINE int findLastBit(size_t word)
{
    int     b;

    for (b = 0; word; word >>= 1, b++) ;
    return b;
}
#endif


/*
    Acquire the freeq. Note: this is only ever used by non-blocking algorithms.
 */
static MPR_INLINE bool acquire(MprFreeQueue *freeq)
{
#if MACOSX
    return OSSpinLockTry(&freeq->lock.cs);
#elif BIT_UNIX_LIKE && BIT_HAS_SPINLOCK
    return pthread_spin_trylock(&freeq->lock.cs) == 0;
#elif BIT_UNIX_LIKE
    return pthread_mutex_trylock(&freeq->lock.cs) == 0;
#elif BIT_WIN_LIKE
    return TryEnterCriticalSection(&freeq->lock.cs) != 0;
#elif VXWORKS
    return semTake(freeq->lock.cs, NO_WAIT) == OK;
#else
    #error "Operting system not supported in acquire()"
#endif
}


static MPR_INLINE void release(MprFreeQueue *freeq)
{
#if MACOSX
    OSSpinLockUnlock(&freeq->lock.cs);
#elif BIT_UNIX_LIKE && BIT_HAS_SPINLOCK
    pthread_spin_unlock(&freeq->lock.cs);
#elif BIT_UNIX_LIKE
    pthread_mutex_unlock(&freeq->lock.cs);
#elif BIT_WIN_LIKE
    LeaveCriticalSection(&freeq->lock.cs);
#elif VXWORKS
    semGive(freeq->lock.cs);
#endif
}


static MPR_INLINE int cas(size_t *target, size_t expected, size_t value)
{
    return mprAtomicCas((void**) target, (void*) expected, (cvoid*) value);
}


static MPR_INLINE void clearbitmap(size_t *bitmap, int bindex) 
{
    size_t  bit, prior;

    bit = (((size_t) 1) << bindex);
    do {
        prior = *bitmap;
        if (!(prior & bit)) {
            break;
        }
    } while (!cas(bitmap, prior, prior & ~bit));
}


static MPR_INLINE void setbitmap(size_t *bitmap, int bindex) 
{
    size_t  bit, prior;

    bit = (((size_t) 1) << bindex);
    do {
        prior = *bitmap;
        if (prior & bit) {
            break;
        }
    } while (!cas(bitmap, prior, prior | bit));
}


#if BIT_WIN_LIKE
PUBLIC Mpr *mprGetMpr()
{
    return MPR;
}
#endif


PUBLIC int mprGetPageSize()
{
    return memStats.pageSize;
}


PUBLIC size_t mprGetBlockSize(cvoid *ptr)
{
    MprMem      *mp;

    mp = GET_MEM(ptr);
    if (ptr == 0 || !VALID_BLK(mp)) {
        return 0;
    }
    CHECK(mp);
    return GET_USIZE(mp);
}


PUBLIC int mprGetHeapFlags()
{
    return heap->flags;
}


PUBLIC void mprSetMemNotifier(MprMemNotifier cback)
{
    heap->notifier = cback;
}


PUBLIC void mprSetMemLimits(ssize warnHeap, ssize maxHeap, ssize cacheHeap)
{
    if (warnHeap > 0) {
        heap->stats.warnHeap = warnHeap;
    }
    if (maxHeap > 0) {
        heap->stats.maxHeap = maxHeap;
    }
    if (cacheHeap >= 0) {
        heap->stats.cacheHeap = cacheHeap;
        heap->stats.lowHeap = cacheHeap ? cacheHeap / 8 : BIT_MPR_ALLOC_REGION_SIZE;
    }
}


PUBLIC void mprSetMemPolicy(int policy)
{
    heap->allocPolicy = policy;
}


PUBLIC void mprSetMemError()
{
    heap->hasError = 1;
}


PUBLIC bool mprHasMemError()
{
    return heap->hasError;
}


PUBLIC void mprResetMemError()
{
    heap->hasError = 0;
}


PUBLIC int mprIsValid(cvoid *ptr)
{
    MprMem      *mp;

    mp = GET_MEM(ptr);
    if (mp->free) {
        return 0;
    }
#if BIT_WIN
    if (isBadWritePtr(mp, sizeof(MprMem))) {
        return 0;
    }
    if (!VALID_BLK(GET_MEM(ptr)) {
        return 0;
    }
    if (isBadWritePtr(ptr, mp->size)) {
        return 0;
    }
    return 0;
#else
#if BIT_MPR_ALLOC_DEBUG
    return ptr && mp->magic == MPR_ALLOC_MAGIC && mp->size > 0;
#else
    return ptr && mp->size > 0;
#endif
#endif
}


static void dummyManager(void *ptr, int flags) 
{
}


PUBLIC void *mprSetManager(void *ptr, MprManager manager)
{
    MprMem      *mp;

    mp = GET_MEM(ptr);
    if (mp->hasManager) {
        if (!manager) {
            manager = dummyManager;
        }
        SET_MANAGER(mp, manager);
    }
    return ptr;
}


#if BIT_MPR_ALLOC_DEBUG
static void checkYielded()
{
    MprThreadService    *ts;
    MprThread           *tp;
    int                 i;

    ts = MPR->threadService;
    lock(ts->threads);
    for (i = 0; i < ts->threads->length; i++) {
        tp = (MprThread*) mprGetItem(ts->threads, i);
        assert(tp->yielded);
    }
    unlock(ts->threads);
}
#endif


#if BIT_MPR_ALLOC_STACK
static void monitorStack()
{
    MprThread   *tp;
    int         diff;

    if (MPR->threadService && (tp = mprGetCurrentThread()) != 0) {
        if (tp->stackBase == 0) {
            tp->stackBase = &tp;
        }
        diff = (int) ((char*) tp->stackBase - (char*) &diff);
        if (diff < 0) {
            tp->peakStack -= diff;
            tp->stackBase = (void*) &diff;
            diff = 0;
        }
        if (diff > tp->peakStack) {
            tp->peakStack = diff;
        }
    }
}
#endif

#if !BIT_MPR_ALLOC_DEBUG
#undef mprSetName
#undef mprCopyName
#undef mprSetAllocName

/*
    Re-instate defines for combo releases, where source will be appended below here
 */
#define mprCopyName(dest, src)
#define mprGetName(ptr) ""
#define mprSetAllocName(ptr, name) ptr
#define mprSetName(ptr, name)
#endif

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
