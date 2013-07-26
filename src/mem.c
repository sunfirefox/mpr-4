/**
    mem.c - Memory Allocator and Garbage Collector. 

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
    These routines are stable and will work, lock-freeregardless of block splitting or joining.
    There is be a race where GET_NEXT will skip a block if the allocator is split a block.
 */
#define GET_NEXT(mp)                ((MprMem*) ((char*) mp + mp->size))
#define GET_REGION(mp)              ((MprRegion*) (((char*) mp) - MPR_ALLOC_ALIGN(sizeof(MprRegion))))

/*
    Memory checking and breakpoints
 */
#if BIT_MEMORY_DEBUG
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
                                        memset((char*) mp + MPR_ALLOC_MIN, 0xFE, mp->size - MPR_ALLOC_MIN); \
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

#if BIT_MEMORY_STATS
    #define ATOMIC_INC(field) mprAtomicAdd64((int64*) &heap->stats.field, 1)
    #define INC(field) heap->stats.field++
#else
    #define ATOMIC_INC(field)
    #define INC(field)
#endif

/*
    The heap lock is used to synchronize access to regions (only)
 */
#define lockHeap()   mprSpinLock(&heap->heapLock);
#define unlockHeap() mprSpinUnlock(&heap->heapLock);

#define percent(a,b) ((int) ((a) * 100 / (b)))

/*
    Fast find first/last bit set
 */
#if LINUX
    #if BIT_MPR_ALLOC_BIG && BIT_64
        #define NEED_FLS 1
        #if BIT_CPU_ARCH == BIT_CPU_X86 || BIT_CPU_ARCH == BIT_CPU_X64
            #define USE_FLS_ASM_X86 1
        #endif
        static MPR_INLINE int fls(uint word);
    #else
        #define NEED_FLSL 1
        static MPR_INLINE int flsl(uint word);
    #endif

#elif BIT_WIN_LIKE
    #define NEED_FFS 1
    static MPR_INLINE int ffs(uint word);
    #if BIT_MPR_ALLOC_BIG && BIT_64
        #define NEED_FLS 1
        static MPR_INLINE int fls(uint word);
    #else
        #define NEED_FLSL 1
        static MPR_INLINE int flsl(uint word);
    #endif

#elif !BIT_BSD_LIKE
    #define NEED_FFS 1
    static MPR_INLINE int ffs(uint word);
    #if BIT_MPR_ALLOC_BIG && BIT_64
        #define NEED_FLS 1
        static MPR_INLINE int fls(uint word);
    #else
        #define NEED_FLSL 1
        static MPR_INLINE int flsl(uint word);
    #endif
#endif

/********************************** Data **************************************/

#undef              MPR
PUBLIC Mpr          *MPR;
static MprHeap      *heap;
static MprMemStats  memStats;
static int          padding[] = { 0, MPR_MANAGER_SIZE };

/***************************** Forward Declarations ***************************/

static inline bool acquire(MprFreeQueue *freeq);
static inline void acquireWait(MprFreeQueue *freeq);
static inline int cas(size_t *target, size_t expected, size_t value);
static inline bool claim(MprMem *mp);
static inline void initBlock(MprMem *mp, size_t size, int first);
static inline void release(MprFreeQueue *freeq);
static inline void unlinkBlock(MprMem *mp);

static void allocException(int cause, size_t size);
static void dummyManager(void *ptr, int flags);
static size_t fastMemSize();
static void *getNextRoot();
static void getSystemInfo();
static void invokeDestructors();
static void markAndSweep();
static void markRoots();
static int pauseThreads();
static void printMemReport();
static void resumeThreads(int swept);
static void sweep();
static void sweeper(void *unused, MprThread *tp);
static void triggerGC(int flags);

#if BIT_WIN_LIKE
    static int winPageModes(int flags);
#endif
#if BIT_MEMORY_DEBUG
    static void breakpoint(MprMem *mp);
    static void checkYielded();
    static int validBlk(MprMem *mp);
#endif
#if BIT_MEMORY_STATS
    static void freeLocation(cchar *name, size_t size);
    static void printQueueStats();
    static void printGCStats();
#endif
#if BIT_MEMORY_STACK
    static void monitorStack();
#else
    #define monitorStack()
#endif
static int initQueues();
static MprMem *allocMem(size_t size);
static void freeBlock(MprMem *mp);
static int getQueueIndex(size_t size, int roundup);
static MprMem *growHeap(size_t size);
static void linkBlock(MprMem *mp); 
static void *vmalloc(size_t size, int mode);
static void vmfree(void *ptr, size_t size);

/************************************* Code ***********************************/

PUBLIC Mpr *mprCreateMemService(MprManager manager, int flags)
{
    MprMem      *mp;
    MprMem      *spare;
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
    heap->stats.maxMemory = (size_t) -1;
    heap->stats.redLine = ((size_t) -1) / 100 * 95;
    mprInitSpinLock(&heap->heapLock);

    /*
        Hand-craft the Mpr structure from the first region. Free the remainder below.
     */
    mprSize = MPR_ALLOC_ALIGN(sizeof(MprMem) + sizeof(Mpr) + (MPR_MANAGER_SIZE * sizeof(void*)));
    regionSize = MPR_ALLOC_ALIGN(sizeof(MprRegion));
    size = max(mprSize + regionSize, BIT_MPR_ALLOC_MAX_REGION);
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
    heap->chunkSize = BIT_MPR_ALLOC_MAX_REGION;
    heap->stats.maxMemory = (size_t) -1;
    heap->stats.redLine = ((size_t) -1) / 100 * 95;
    heap->stats.cacheMemory = BIT_MPR_ALLOC_CACHE;
    heap->newQuota = BIT_MPR_ALLOC_QUOTA;
    heap->enabled = !(heap->flags & MPR_DISABLE_GC);

    if (scmp(getenv("MPR_DISABLE_GC"), "1") == 0) {
        heap->enabled = 0;
    }
    if (scmp(getenv("MPR_VERIFY_MEM"), "1") == 0) {
        heap->verify = 1;
    }
    if (scmp(getenv("MPR_SCRIBBLE_MEM"), "1") == 0) {
        heap->scribble = 1;
    }
    if (scmp(getenv("MPR_TRACK_MEM"), "1") == 0) {
        heap->track = 1;
    }
    heap->stats.bytesAllocated += size;
    heap->stats.regions++;
    INC(allocs);

    mprInitSpinLock(&heap->heapLock);
    mprInitSpinLock(&heap->rootLock);
    initQueues();

    /*
        Free the remaining memory after MPR
     */
    spareSize = size - regionSize - mprSize;
    if (spareSize > 0) {
        spare = (MprMem*) (((char*) mp) + mprSize);
        initBlock(spare, spareSize, 0);
        heap->regions = region;
        SCRIBBLE(spare);
        linkBlock(spare);
    }
    heap->sweeperCond = mprCreateCond();
    heap->roots = mprCreateList(-1, MPR_LIST_STATIC_VALUES);
    mprAddRoot(MPR);
    return MPR;
}


static inline void initBlock(MprMem *mp, size_t size, int first)
{
    static MprMem empty = {};

    *mp = empty;
    /* Implicit:  mp->free = 0; */
    mp->first = first;
    mp->mark = heap->mark;
    mp->size = (MprMemSize) size;
    SET_MAGIC(mp);
    SET_SEQ(mp);
    SET_NAME(mp, NULL);
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
    size = max(size, MPR_ALLOC_MIN);
    size = MPR_ALLOC_ALIGN(size);

    if ((mp = allocMem(size)) == NULL) {
        return NULL;
    }
    mp->hasManager = (flags & MPR_ALLOC_MANAGER) ? 1 : 0;
    ptr = GET_PTR(mp);
    if (flags & MPR_ALLOC_ZERO) {
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
    size = max(size, usize + (size_t) MPR_ALLOC_MIN);
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
/*
    Initialize the free space map and queues.

    The free map is a two dimensional array of free queues. The first dimension is indexed by
    the most significant bit (MSB) set in the requested block size. The second dimension is the next 
    MPR_ALLOC_BUCKET_SHIFT (4) bits below the MSB.

    +-------------------------------+
    |       |MSB|  Bucket   | rest  |
    +-------------------------------+
    | 0 | 0 | 1 | 1 | 1 | 1 | ..... |
    +-------------------------------+
 */
static int initQueues() 
{
    MprFreeQueue    *freeq;
    int             numGroups, numQueues;

    numGroups = fls(BIT_MPR_ALLOC_MAX_REGION) - MPR_ALLOC_BUCKET_SHIFT - BIT_MPR_ALLOC_ALIGN;
    numQueues = numGroups * MPR_ALLOC_NUM_BUCKETS;
    if ((heap->freeq = mprVirtAlloc(numQueues * sizeof(MprFreeQueue) , MPR_MAP_READ | MPR_MAP_WRITE)) == NULL) {
        return MPR_ERR_MEMORY;
    }
    heap->freeEnd = &heap->freeq[numQueues];
    heap->numQueues = numQueues;
    heap->numGroups = numGroups;
    for (freeq = heap->freeq; freeq != heap->freeEnd; freeq++) {
        size_t  size, groupBits, bucketBits;
        int     index, group, bucket;
        index = (int) (freeq - heap->freeq);
        group = index / MPR_ALLOC_NUM_BUCKETS;
        bucket = index % MPR_ALLOC_NUM_BUCKETS;

        groupBits = (group != 0) << (group + MPR_ALLOC_BUCKET_SHIFT - 1);
        bucketBits = ((size_t) bucket) << (max(0, group - 1));

        size = groupBits | bucketBits;
        freeq->minSize = (int) (size << MPR_ALIGN_SHIFT);

#if (BIT_MEMORY_STATS && BIT_MEMORY_DEBUG && KEEP) || 0
        printf("Queue: %d, size %u (%x), group %d, bucket %d\n",
            (int) (freeq - heap->freeq), (int) freeq->minSize, (int) freeq->minSize, group, bucket);
#endif
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
    MprMem          *mp, *spare;
    size_t          bucketMap, groupMap, priorBucketMap, priorGroupMap;
    int             bucket, baseGroup, group, qindex, miss;

    qindex = getQueueIndex(required, 1);
    if (qindex >= 0) {
        baseGroup = qindex / MPR_ALLOC_NUM_BUCKETS;
        bucket = qindex % MPR_ALLOC_NUM_BUCKETS;
        heap->weightedCount += qindex;
        ATOMIC_INC(requests);
        /* Ignore races - does not need to be accurate */
        heap->freeq[qindex].demand++;
        miss = 0;

        /*
            Try each free queue in turn. If lock contention or block contention, abort and try the next queue.
            We never block here on any lock.
         */
        groupMap = heap->groupMap & ~((((size_t) 1) << baseGroup) - 1);
        while (groupMap) {
            /* ffs operates on ints, but to use cas(), groupMap and bucketMaps must be size_t */
            group = ffs((int) groupMap) - 1;
            if (groupMap & ((((size_t) 1) << group))) {
                bucketMap = heap->bucketMap[group];
                if (baseGroup == group) {
                    /* Mask buckets lower than the base bucket */
                    bucketMap &= ~((((size_t) 1) << bucket) - 1);
                }
                while (bucketMap) {
                    bucket = ffs((int) bucketMap) - 1;
                    qindex = (group * MPR_ALLOC_NUM_BUCKETS) + bucket;
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
                                freeq->count--;
                                mp = (MprMem*) fp;
                                release(freeq);
                                mprAtomicAdd64((int64*) &heap->stats.bytesFree, -(int64) mp->size);

                                if (mp->size >= (size_t) (required + MPR_ALLOC_MIN_SPLIT)) {
                                    spare = (MprMem*) ((char*) mp + required);
                                    initBlock(spare, mp->size - required, 0);
                                    linkBlock(spare);
                                    mp->size = (MprMemSize) required;
                                    ATOMIC_INC(splits);
                                }
                                if (miss > BIT_MPR_ALLOC_SCAN) {
                                    /* Tested empirically to trigger GC when searching too much for an allocation */
                                    triggerGC(MPR_GC_FORCE);
                                }
                                ATOMIC_INC(reuse);
                                return mp;
                            } else {
                                release(freeq);
                            }
                        } else {
                            ATOMIC_INC(tryFails);
                        }
                    }
                    priorBucketMap = bucketMap;
                    bucketMap &= ~(((size_t) 1) << bucket);
                    cas(&heap->bucketMap[group], priorBucketMap, bucketMap);
                }
                priorGroupMap = groupMap;
                groupMap &= ~(((size_t) 1) << group);
                cas(&heap->groupMap, priorGroupMap, groupMap);
                miss++;
            }
        }
    }
    return growHeap(required);
}


static void freeBlock(MprMem *mp)
{
    MprRegion   *region;
    size_t      size;

    assert(!mp->free);

    SCRIBBLE(mp);
    INC(swept);
    size = mp->size;

#if BIT_MEMORY_STATS
    heap->stats.freed += size;
    if (heap->track) {
        freeLocation(mp->name, size);
    }
#endif
    if (mp->first) {
        region = GET_REGION(mp);
        if (GET_NEXT(mp) >= region->end) {
            region->freeable = 1;
            return;
        }
    }
    linkBlock(mp);
}


/*
    Get the free queue index for a given block size.
    Roundup will be true when allocating because queues store blocks greater than a specified size. To guarantee the requested
    size will be satisfied, we may need to look at the next queue.
 */
static int getQueueIndex(size_t size, int roundup)
{
    size_t      usize, asize;
    int         aligned, bucket, group, qindex, msb;

    assert(MPR_ALLOC_ALIGN(size) == size);
    if (size >= BIT_MPR_ALLOC_MAX_REGION) {
        return -1;
    }
    /*
        Determine the free queue based on user sizes (sans header). This permits block searches to avoid scanning the next 
        highest queue for common block sizes: eg. 1K.
     */
    usize = (size - sizeof(MprMem));
    asize = (usize >> MPR_ALIGN_SHIFT);
    /* 
        Find the last (most) significant bit in the block size
     */
#if BIT_MPR_ALLOC_BIG && BIT_64
    msb = flsl(asize) - 1;
#else
    msb = fls((int) asize) - 1;
#endif
    group = max(0, msb - MPR_ALLOC_BUCKET_SHIFT + 1);
    bucket = (asize >> max(0, group - 1)) & (MPR_ALLOC_NUM_BUCKETS - 1);
    qindex = (group * MPR_ALLOC_NUM_BUCKETS) + bucket;
#if BIT_DEBUG
    if (qindex >= heap->numQueues) {
        /* Should never get here */
        assert(qindex < heap->numQueues);
        return -1;
    }
#endif
    assert(qindex < (heap->freeEnd - heap->freeq));
    assert(bucket < MPR_ALLOC_NUM_BUCKETS);

    if (roundup) {
        /*
            Good-fit strategy: check if the requested size is the smallest possible size in a queue. If not the smallest,
            must look at the next queue higher up to guarantee a block of sufficient size. Blocks in the queues of group 0
            and group 1 are are all sized the same. i.e. the queues of group zero differ in size by one word only.
         */
        if (group > 1) {
            size_t mask = (((size_t) 1) << (msb - MPR_ALLOC_BUCKET_SHIFT)) - 1;
            aligned = (asize & mask) == 0;
            if (!aligned) {
                qindex++;
            }
        }
    }
    return qindex;
}


/*
    Add a block to a free q. Called by user threads from allocMem and by sweeper from freeBlock.
    WARNING: Must be called with the freelist unlocked. This is the opposite of unlinkBlock.
 */
static void linkBlock(MprMem *mp) 
{
    MprFreeQueue    *freeq;
    MprFreeMem      *fp;
    size_t          bucketMap, groupMap, priorGroupMap, priorBucketMap;
    int             group, bucket;

    CHECK(mp);

    mp->qindex = getQueueIndex(mp->size, 0);
    mp->free = 1;
    mp->hasManager = 0;
    freeq = &heap->freeq[mp->qindex];
    fp = (MprFreeMem*) mp;

    /*
        Link onto front of free queue. Must be fast!
        MOB - but this is not lock free!!!
     */
    acquireWait(freeq);
    fp->next = freeq->next;
    fp->prev = (MprFreeMem*) freeq;
    freeq->next->prev = fp;
    freeq->next = fp;
    freeq->count++;
    release(freeq);

    mprAtomicAdd64((int64*) &heap->stats.bytesFree, mp->size);

    /*
        Updates group and bucket maps. This is done lock-free to reduce contention. Racing with multiple-threads in allocMem().
     */
    group = mp->qindex / MPR_ALLOC_NUM_BUCKETS;
    bucket = mp->qindex % MPR_ALLOC_NUM_BUCKETS;
    do {
        priorGroupMap = heap->groupMap;
        groupMap = priorGroupMap | (((size_t) 1) << group);
    } while (!cas(&heap->groupMap, priorGroupMap, groupMap));

    do {
        priorBucketMap = heap->bucketMap[group];
        bucketMap = priorBucketMap | (((size_t) 1) << bucket);
    } while (!cas(&heap->bucketMap[group], priorBucketMap, bucketMap));
}


/*
    Remove a block from a free q.
    WARNING: Must be called with the freelist locked.
 */
static inline void unlinkBlock(MprMem *mp) 
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
#if BIT_MEMORY_DEBUG
    fp->next = fp->prev = NULL;
#endif
    mprAtomicAdd64((int64*) &heap->stats.bytesFree, -(int64) mp->size);
}


/*
    Grow the heap and return a block of the required size (unqueued)
 */
static MprMem *growHeap(size_t required)
{
    MprRegion   *region;
    MprMem      *mp, *spare;
    size_t      size, rsize, spareLen;

#if DIAGS && KEEP
    printMemReport();
    printQueueStats();
#endif
    /*
        Force a GC to run soon
     */
    triggerGC(MPR_GC_FORCE);
    
    if (required >= MPR_ALLOC_MAX) {
        allocException(MPR_MEM_TOO_BIG, required);
        return 0;
    }

    rsize = MPR_ALLOC_ALIGN(sizeof(MprRegion));
    size = max((size_t) required + rsize, (size_t) heap->chunkSize);
    size = MPR_PAGE_ALIGN(size, memStats.pageSize);
    if ((region = mprVirtAlloc(size, MPR_MAP_READ | MPR_MAP_WRITE)) == NULL) {
        allocException(MPR_MEM_TOO_BIG, size);
        return 0;
    }
    mprInitSpinLock(&((MprRegion*) region)->lock);
    region->size = size;
    region->start = (MprMem*) (((char*) region) + rsize);
    region->end = (MprMem*) ((char*) region + size);
    region->freeable = 0;
    mp = (MprMem*) region->start;
    spareLen = size - required - rsize;

    /*
        If a block is big, don't split the block. This improves the chances it will be unpinned.
     */
    if (spareLen < MPR_ALLOC_MIN || required > BIT_MPR_ALLOC_MAX_REGION) {
        required = size - rsize; 
        spareLen = 0;
    }
    initBlock(mp, required, 1);

    lockHeap();

    if (spareLen > 0) {
        assert(spareLen >= MPR_ALLOC_MIN);
        spare = (MprMem*) ((char*) mp + required);
        initBlock(spare, spareLen, 0);
        CHECK(spare);
        ATOMIC_INC(allocs);
        linkBlock(spare);
    } else {
        ATOMIC_INC(allocs);
    }
    /*
        Add region last once the mp block and possible spare are crafted
     */
    region->next = heap->regions;
    heap->regions = region;
    heap->stats.bytesAllocated += size;
    heap->stats.regions++;
    unlockHeap();
    CHECK(mp);
    return mp;
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
    if ((size + used) > heap->stats.maxMemory) {
        allocException(MPR_MEM_LIMIT, size);
    } else if ((size + used) > heap->stats.redLine) {
        allocException(MPR_MEM_REDLINE, size);
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
            if ((heap->sweeper = mprCreateThread("sweeper", sweeper, NULL, 0)) == 0) {
                mprError("Cannot create marker thread");
                MPR->hasError = 1;
            } else {
                mprStartThread(heap->sweeper);
            }
        }
    }
}


PUBLIC void mprStopGCService()
{
    int     i;

    mprWakeGCService();
    for (i = 0; heap->sweeper && i < MPR_TIMEOUT_STOP; i++) {
        mprNap(1);
    }
}


PUBLIC void mprWakeGCService()
{
    mprSignalCond(heap->sweeperCond);
}


static void triggerGC(int flags)
{
    if (!heap->gcRequested && ((flags & MPR_GC_FORCE) || (heap->weightedCount > heap->newQuota))) {
        heap->gcRequested = 1;
        if (heap->flags & MPR_SWEEP_THREAD && heap->sweeperCond) {
            mprSignalCond(heap->sweeperCond);
        }
    }
}


#if EXAMPLES
mprYield(0);                        //  Yield. Block for marking if GC required
mprYield(MPR_YIELD_BLOCK);          //  Yield and wait for GC to happen
mprYield(MPR_YIELD_COMPLETE);       //  Yield and wait for GC to happen and fully complete
mprYield(MPR_YIELD_STICKY);         //  Sticky yield and return

mprRequestGC(0);                    //  Request a GC if necessary. Yield and block if GC is run. Don't wait for completion of GC.
mprRequestGC(MPR_GC_FORCE);         //  Force a GC whether required or not
mprRequestGC(MPR_GC_NO_BLOCK);      //  Request a GC but do not block. Thread must yield sometime after.
mprRequestGC(MPR_GC_COMPLETE);      //  Request a GC and wait for it to fully complete.
mprRequestGC(MPR_GC_COMPACT);       //  Request a fully compacting GC that will coalesce and unpin all possible blocks
#endif

/*
    Trigger a GC collection worthwhile. If MPR_GC_FORCE is set, force the collection regardless. Flags:

    MPR_CG_DEFAULT      0x0     run GC if necessary. Will yield and block for GC. Won't wait for GC to fully complete.
    MPR_GC_FORCE        0x1     force a GC whether it is required or not
    MPR_GC_NO_BLOCK     0x2     dont wait for the GC complete
 */
PUBLIC void mprRequestGC(int flags)
{
    mprTrace(7, "DEBUG: mprRequestGC");

    if (flags & MPR_GC_COMPACT) {
        heap->compact = 1;
    }
    if ((flags & MPR_GC_FORCE) || (heap->weightedCount > heap->newQuota)) {
        triggerGC(MPR_GC_FORCE);
    }
    if (!(flags & MPR_GC_NO_BLOCK)) {
        mprYield((flags & MPR_GC_COMPLETE) ? MPR_YIELD_COMPLETE : 0);
    }
}


/*
    Sweeper main thread
 */
static void sweeper(void *unused, MprThread *tp)
{
    mprTrace(5, "Sweeper thread started");
    tp->stickyYield = 1;
    tp->yielded = 1;

    while (!mprIsFinished()) {
        if (!heap->mustYield) {
            mprWaitForCond(heap->sweeperCond, -1);
            if (mprIsFinished()) {
                break;
            }
        }
        markAndSweep();
    }
    invokeDestructors();
    heap->sweeper = 0;
    resumeThreads(1);
}


/*
    The mark phase will run with all user threads yielded. The sweep phase then runs in parallel.
    The mark phase is relatively quick.
 */
static void markAndSweep()
{
    static int warnOnce = 0;

    mprTrace(7, "GC: mark started");
    heap->gcRequested = 0;
    heap->mustYield = 1;

    if (!pauseThreads()) {
        if (warnOnce++ == 0) {
            mprTrace(7, "GC synchronization timed out, some threads did not yield.");
            mprTrace(7, "This is most often caused by a thread doing a long running operation and not first calling mprYield.");
            mprTrace(7, "If debugging, run the process with -D to enable debug mode.");
        }
        return;
    }
    heap->priorWeightedCount = heap->weightedCount;
#if BIT_MEMORY_STATS
    heap->priorFree = heap->stats.bytesFree;
#endif
    heap->weightedCount = 0;

    /*
        Mark all roots
     */
    heap->mark = !heap->mark;
    MPR_MEASURE(BIT_MPR_ALLOC_LEVEL, "GC", "mark", markRoots());
    heap->marking = 0;
    heap->sweeping = 1;
    mprAtomicBarrier();

#if BIT_MPR_ALLOC_PARALLEL
    resumeThreads(0);
#endif
    /*
        Sweep unused memory with user threads resumed
     */
    MPR_MEASURE(BIT_MPR_ALLOC_LEVEL, "GC", "sweep", sweep());
    heap->sweeping = 0;
    resumeThreads(1);
}


static void invokeDestructors()
{
    MprRegion   *region;
    MprMem      *mp;
    MprManager  mgr;

    for (region = heap->regions; region; region = region->next) {
        for (mp = region->start; mp < region->end; mp = GET_NEXT(mp)) {
            if (mp->mark != heap->mark && !mp->free && mp->hasManager) {
                mgr = GET_MANAGER(mp);
                if (mgr) {
                    (mgr)(GET_PTR(mp), MPR_MANAGE_FREE);
                    mp->hasManager = 0;
                }
            }
        }
    }
}


/*
    Claim a block from its freeq for the sweeper. This removes the block from the freeq and clears the "free" bit.
 */
static inline bool claim(MprMem *mp)
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
    MprRegion       *region, *nextRegion, *prior;
    MprFreeQueue    *freeq;
    MprMem          *mp, *next;

    if (!heap->enabled) {
        mprTrace(0, "DEBUG: sweep: Abort sweep - GC disabled");
        return;
    }
    mprTrace(7, "GC: sweep started");
#if BIT_MEMORY_STATS
    heap->stats.sweepVisited = 0;
    heap->stats.swept = 0;
    heap->stats.freed = 0;
#endif

    /*
        First run managers so that dependant memory blocks will still exist when the manager executes.
        Actually free the memory in a 2nd pass below. OPT - could optimize by requiring a separate flag for managers 
        that implement destructors.
     */
    invokeDestructors();

    /*
        RACE: Racing with growHeap. This traverses the region list lock-free. However, growHeap() will append new regions to 
        the front of heap->regions and so will not race with this code. This code is the only code that frees regions.
     */
    prior = NULL;
    for (region = heap->regions; region; region = nextRegion) {
        nextRegion = region->next;

        for (mp = region->start; mp < region->end; mp = next) {
            next = GET_NEXT(mp);
            CHECK(mp);
            INC(sweepVisited);
            if (heap->compact && mp->free) {
                if (next < region->end && !next->free && next->mark != heap->mark && claim(mp)) {
                    mp->mark = !heap->mark;
                    INC(compacted);
                }
            } 
            if (!mp->free && mp->mark != heap->mark) {
                /*
                    Cache small blocks provided not first block in the regions (assists to unpin regions)
                 */
                if (mp->first && mp->size < BIT_MPR_ALLOC_SMALL && heap->stats.bytesFree < heap->stats.cacheMemory) {
                    INC(cached);
                    freeBlock(mp);
                    continue;
                }
                while (next < region->end) {
                    if (next->free) {
                        if (!claim(next)) {
                            break;
                        }
                        mp->size += next->size;
                        assert(!next->free);
                        SCRIBBLE_RANGE(next, MPR_ALLOC_MIN);
                        INC(joins);

                    } else if (next->mark != heap->mark) {
                        assert(!next->free);
                        assert(next->qindex == 0);
                        mp->size += next->size;
                        SCRIBBLE_RANGE(next, MPR_ALLOC_MIN);
                        INC(joins);

                    } else {
                        break;
                    }
                    next = GET_NEXT(mp);
                }
                freeBlock(mp);
            }
        }
        if (region->freeable) {
            lockHeap();
            INC(unpins);
            if (prior) {
                prior->next = nextRegion;
            } else {
                heap->regions = nextRegion;
            }
            heap->stats.regions--;
            heap->stats.bytesAllocated -= region->size;
            assert(heap->stats.bytesAllocated >= 0);
            unlockHeap();
            mprTrace(9, "DEBUG: Unpin %p to %p size %d, used %d", region, ((char*) region) + region->size, 
                region->size, fastMemSize());
            mprManageSpinLock(&region->lock, MPR_MANAGE_FREE);
            mprVirtFree(region, region->size);
        } else {
            prior = region;
        }
    }
#if BIT_MEMORY_STATS && BIT_MEMORY_DEBUG
    mprTrace(7, "GC: MARKED %,Ld/%,Ld, SWEPT %,Ld/%,Ld, freed %,Ld, bytesFree %,Ld (prior %,Ld), weightedCount %,Ld/%,Ld, " 
            "blocks %,Ld bytes %,Ld",
            heap->stats.marked, heap->stats.markVisited, heap->stats.swept, heap->stats.sweepVisited, 
            heap->stats.freed, heap->stats.bytesFree, heap->priorFree, heap->priorWeightedCount, heap->newQuota,
            heap->stats.sweepVisited - heap->stats.swept, heap->stats.bytesAllocated);
#endif
    if (heap->printStats) {
        printMemReport();
        heap->printStats = 0;
    }
    for (freeq = heap->freeq; freeq != heap->freeEnd; freeq++) {
        freeq->demand /= 2;
    }
    heap->compact = 0;
}


static void markRoots()
{
    void    *root;

#if BIT_MEMORY_STATS
    heap->stats.markVisited = 0;
    heap->stats.marked = 0;
#endif
    mprMark(heap->roots);
    mprMark(heap->sweeperCond);

    heap->rootIndex = 0;
    while ((root = getNextRoot()) != 0) {
        mprMark(root);
    }
    heap->rootIndex = -1;
}


/*
    This is typically a macro.
    MOB - test size and speed difference
    MOB - otherwise remove this code
 */
#if !defined(mprMarkBlock)
PUBLIC void mprMarkBlock(cvoid *ptr)
{
    MprMem      *mp;
#if BIT_DEBUG
    static int  depth = 0;
#endif

    if (ptr == 0) {
        return;
    }
    mp = MPR_GET_MEM(ptr);
    assert(!mp->free);

#if BIT_DEBUG
    if (!mprIsValid(ptr)) {
        mprError("Memory block is either not dynamically allocated, or is corrupted");
        return;
    }
#endif
    CHECK(mp);
    INC(markVisited);

    if (mp->mark != heap->mark) {
        mp->mark = heap->mark;
        if (mp->hasManager) {
#if BIT_DEBUG
            if (++depth > 400) {
                fprintf(stderr, "WARNING: Possibly too much recursion. Marking depth exceeds 400\n");
                mprBreakpoint();
            }
#endif
            (GET_MANAGER(mp))((void*) ptr, MPR_MANAGE_MARK);
#if BIT_DEBUG
            --depth;
#endif
        }
        INC(marked);
    }
}
#endif


/*
    Permanent allocation. Immune to garbage collector.
 */
void *palloc(size_t size)
{
    void    *ptr;

    if ((ptr = mprAllocMem(size, 0)) != 0) {
        mprHold(ptr);
    }
    return ptr;
}


/*
    Normal free. Note: this must not be called with a block allocated via "malloc".
    No harm in calling this on a block allocated with mprAlloc and not "palloc".
 */
void pfree(void *ptr)
{
    if (ptr) {
        mprRelease(ptr);
    }
}


void *prealloc(void *ptr, size_t size)
{
    mprRelease(ptr);
    if ((ptr =  mprRealloc(ptr, size)) != 0) {
        mprHold(ptr);
    }
    return ptr;
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


/*
    If dispatcher is 0, will use MPR->nonBlock if MPR_EVENT_QUICK else MPR->dispatcher
 */
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
    if (/* heap->mustYield || */ heap->marking) {
        unlock(ts->threads);
        mprYield(0);
    } else {
        tp->yielded = 0;
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


/*
    Resume user threads. If PARALLEL:
        if not-swept only wake theose threads not waiting for GC.
        if swept, only wake those threads waiting for GC.
 */
static void resumeThreads(int swept)
{
    MprThreadService    *ts;
    MprThread           *tp;
    int                 i;

    heap->mustYield = 0;
    ts = MPR->threadService;

    lock(ts->threads);
    for (i = 0; i < ts->threads->length; i++) {
        tp = (MprThread*) mprGetItem(ts->threads, i);
        if (tp && tp->yielded) {
            if (BIT_MPR_ALLOC_PARALLEL) {
                if (swept) {
                    if (!tp->waitForGC) {
                        continue;
                    }
                } else if (tp->waitForGC) {
                    continue;
                }
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
    /*
        Need to use root lock because mprAddItem may allocate
     */
    mprSpinLock(&heap->rootLock);
    mprAddItem(heap->roots, root);
    mprSpinUnlock(&heap->rootLock);
}


PUBLIC void mprRemoveRoot(cvoid *root)
{
    size_t  index;

    mprSpinLock(&heap->rootLock);
    index = mprRemoveItem(heap->roots, root);
    /*
        RemoveItem copies down. If the item was equal or before the current marker root, must adjust the marker rootIndex
        so we don't skip a root.
     */
    if (index <= heap->rootIndex && heap->rootIndex > 0) {
        heap->rootIndex--;
    }
    mprSpinUnlock(&heap->rootLock);
}


static void *getNextRoot()
{
    void    *root;

    mprSpinLock(&heap->rootLock);
    root = mprGetNextItem(heap->roots, &heap->rootIndex);
    mprSpinUnlock(&heap->rootLock);
    return root;
}


/****************************************************** Debug *************************************************************/

#if BIT_MEMORY_STATS
static void printQueueStats() 
{
    MprFreeQueue    *freeq;
    int             i;

    printf("\nFree Queue Stats\n Bucket           Size          Count         Demand          Total\n");
    for (i = 0, freeq = heap->freeq; freeq != heap->freeEnd; freeq++, i++) {
        if (freeq->count || freeq->demand) {
            printf("%7d %14d %14d %14d %14d\n", i, freeq->minSize, freeq->count, freeq->demand, freeq->minSize * freeq->count);
        }
    }
}


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


static void printGCStats()
{
    MprRegion   *region;
    MprMem      *mp;
    size_t      freeBytes, activeBytes, eternalBytes, regionBytes;
    int         regions, freeCount, activeCount, eternalCount, regionCount;

    printf("\nRegion Stats\n");
    regions = 0;
    activeBytes = eternalBytes = freeBytes = 0;
    activeCount = eternalCount = freeCount = 0;

    for (region = heap->regions; region; region = region->next, regions++) {
        regionCount = 0;
        regionBytes = 0;

        for (mp = region->start; mp < region->end; mp = GET_NEXT(mp)) {
            if (mp->free) {
                freeBytes += mp->size;
                freeCount++;

            } else if (mp->eternal) {
                eternalBytes += mp->size;
                eternalCount++;
                regionCount++;
                regionBytes += mp->size;

            } else {
                activeBytes += mp->size;
                activeCount++;
                regionCount++;
                regionBytes += mp->size;
            }
        }
        printf("  Region %3d size %d bytes allocated %4d blocks with %7ld bytes available\n", regions, (int) region->size, 
            regionCount, region->size - regionBytes);
    }
    printf("\nGC Stats\n");
    printf("  Active:  %9d blocks, %12ld bytes\n", activeCount, activeBytes);
    printf("  Eternal: %9d blocks, %12ld bytes\n", eternalCount, eternalBytes);
    printf("  Free:    %9d blocks, %12ld bytes\n", freeCount, freeBytes);
}
#endif /* BIT_MEMORY_STATS */


PUBLIC void mprPrintMem(cchar *msg, int flags)
{
    int     gflags;

    printf("\n%s\n", msg);
    printf("-------------\n");
    heap->printStats = (flags & MPR_MEM_DETAIL) ? 2 : 1;
    gflags = MPR_GC_FORCE | MPR_GC_COMPLETE;
    if (flags & MPR_MEM_COMPACT) {
        gflags |= MPR_GC_COMPACT;
    }
    mprRequestGC(gflags);
}


static void printMemReport()
{
    MprMemStats     *ap;

    ap = mprGetMemStats();

    printf("  Total app memory  %14u K\n",             (int) (mprGetMem() / 1024));
    printf("  Allocated memory  %14u K\n",             (int) (ap->bytesAllocated / 1024));
    printf("  Free heap memory  %14u K\n",             (int) (ap->bytesFree / 1024));

    if (ap->maxMemory == (size_t) -1) {
        printf("  Memory limit           unlimited\n");
        printf("  Memory redline         unlimited\n");
    } else {
        printf("  Memory limit      %14u MB (%d %%)\n",    (int) (ap->maxMemory / (1024 * 1024)),
           percent(ap->bytesAllocated / 1024, ap->maxMemory / 1024));
        printf("  Memory redline    %14u MB (%d %%)\n",    (int) (ap->redLine / (1024 * 1024)),
           percent(ap->bytesAllocated / 1024, ap->redLine / 1024));
        printf("  Memory cache      %14u MB (%d %%)\n",    (int) (ap->cacheMemory / (1024 * 1024)),
           percent(ap->cacheMemory / 1024, ap->maxMemory / 1024));
    }
    printf("  Allocation errors %14d\n",               (int) ap->errors);

#if BIT_MEMORY_STATS
    printf("  Memory requests   %14d\n",               (int) ap->requests);
    printf("  O/S allocations   %14d %%\n",            percent(ap->allocs, ap->requests));
    printf("  Region unpins     %14d %% (%d)\n",       percent(ap->unpins, ap->requests), (int) ap->unpins);
    printf("  Block reuse       %14d %%\n",            percent(ap->reuse, ap->requests));
    printf("  Joins             %14d %% (%d)\n",       percent(ap->joins, ap->requests), (int) ap->joins);
    printf("  Splits            %14d %% (%d)\n",       percent(ap->splits, ap->requests), (int) ap->splits);
    printf("  Cached            %14d %% (%d)\n",       percent(ap->cached, ap->requests), (int) ap->cached);
    printf("  Compacted         %14d %% (%d)\n",       percent(ap->compacted, ap->requests), (int) ap->compacted);
    printf("  Freeq failures    %14d %% (%d / %d)\n",  percent(ap->tryFails, ap->trys), (int) ap->tryFails, (int) ap->trys);
    printf("  MprMem            %14d\n",               (int) sizeof(MprMem));
    printf("  MprFreeMem        %14d\n",               (int) sizeof(MprFreeMem));

    printGCStats();
    if (heap->printStats > 1) {
        printQueueStats();
        if (heap->track) {
            printTracking();
        }
    }
#endif /* BIT_MEMORY_STATS */
}


PUBLIC void mprVerifyMem()
{
#if BIT_MEMORY_DEBUG
    MprRegion       *region;
    MprMem          *mp;
    MprFreeQueue    *freeq;
    MprFreeMem      *fp;
    int         i;

    if (!heap->verify) {
        return;
    }
    while (heap->sweeping) {
        mprNap(1);
    }
    //  MOB - have sweeper do the verify. Have here just trigger a GC with verify

    for (region = heap->regions; region; region = region->next) {
        for (mp = region->start; mp < region->end; mp = GET_NEXT(mp)) {
            CHECK(mp);
        }
    }
    for (i = 0, freeq = heap->freeq; freeq != heap->freeEnd; freeq++, i++) {
        for (fp = freeq->next; fp != (MprFreeMem*) freeq; fp = fp->next) {
            mp = (MprMem*) fp;
            CHECK(mp);
            assert(mp->free);
#if FUTURE
            uchar *ptr;
            int  usize;
            if (heap->verifyFree) {
                ptr = (uchar*) ((char*) mp + sizeof(MprFreeMem));
                usize = GET_USIZE(mp);
                if (HAS_MANAGER(mp)) {
                    usize -= sizeof(MprManager);
                }
                for (i = 0; i < usize; i++) {
                    if (ptr[i] != 0xFE) {
                        mprError("Free memory block %x has been modified at offset %d (MprBlk %x, seqno %d)"
                                       "Memory was last allocated by %s", GET_PTR(mp), i, mp, mp->seqno, mp->name);
                    }
                }
            }
#endif
        }
    }
#endif
}


#if BIT_MEMORY_DEBUG
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


/*
    Called to set the memory block name when doing an allocation
 */
PUBLIC void *mprSetAllocName(void *ptr, cchar *name)
{
    MPR_GET_MEM(ptr)->name = name;

#if BIT_MEMORY_STATS
    if (heap->track) {
        MprLocationStats    *lp;
        cchar               **np;
        int                 index;
        if (name == 0) {
            name = "";
        }
        index = shash(name, strlen(name)) % MPR_TRACK_HASH;
        lp = &heap->stats.locations[index];
        for (np = lp->names; np <= &lp->names[MPR_TRACK_NAMES]; np++) {
            if (*np == 0 || *np == name || strcmp(*np, name) == 0) {
                break;
            }
        }
        if (np < &lp->names[MPR_TRACK_NAMES]) {
            *np = (char*) name;
        }
        lp->count += GET_MEM(ptr)->size;
    }
#endif
    return ptr;
}


static void freeLocation(cchar *name, size_t size)
{
#if BIT_MEMORY_STATS
    MprLocationStats    *lp;
    int                 index, i;

    if (name == 0) {
        name = "";
    }
    index = shash(name, strlen(name)) % MPR_TRACK_HASH;
    lp = &heap->stats.locations[index];
    lp->count -= size;
    if (lp->count <= 0) {
        for (i = 0; i < MPR_TRACK_NAMES; i++) {
            lp->names[i] = 0;
        }
    }
#endif
}


PUBLIC void *mprSetName(void *ptr, cchar *name) 
{
#if BIT_MEMORY_STATS
    MprMem  *mp = GET_MEM(ptr);
    if (mp->name) {
        freeLocation(mp->name, mp->size);
        mprSetAllocName(ptr, name);
    }
#else
    MPR_GET_MEM(ptr)->name = name;
#endif
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

    lockHeap();
    heap->stats.errors++;
    if (heap->stats.inMemException || mprIsStopping()) {
        unlockHeap();
        return;
    }
    heap->stats.inMemException = 1;
    used = fastMemSize();
    unlockHeap();

    if (cause == MPR_MEM_FAIL) {
        heap->hasError = 1;
        mprError("%s: Cannot allocate memory block of size %,Ld bytes.", MPR->name, size);

    } else if (cause == MPR_MEM_TOO_BIG) {
        heap->hasError = 1;
        mprError("%s: Cannot allocate memory block of size %,Ld bytes.", MPR->name, size);

    } else if (cause == MPR_MEM_REDLINE) {
        mprError("%s: Memory request for %,Ld bytes exceeds memory red-line.", MPR->name, size);
        mprPruneCache(NULL);

    } else if (cause == MPR_MEM_LIMIT) {
        mprError("%s: Memory request for %,d bytes exceeds memory limit.", MPR->name, size);
    }
    mprError("%s: Memory used %,d, redline %,d, limit %,d.", MPR->name, (int) used, (int) heap->stats.redLine,
        (int) heap->stats.maxMemory);
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
#if UNUSED
    MprMem      blk;

    /*
        Determine the claim bit ordering
     */
    memset(&blk, 0, sizeof(blk));
    blk.claimed = 1;
    claimBit = *(size_t*) &blk;
#endif

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


#if NEED_FFS
/* 
    Find first bit set in word 
 */
#if USE_FFS_ASM_X86
static MPR_INLINE int ffs(uint x)
{
    long    r;

    asm("bsf %1,%0\n\t"
        "jnz 1f\n\t"
        "mov $-1,%0\n"
        "1:" : "=r" (r) : "rm" (x));
    return (int) r + 1;
}
#else
static MPR_INLINE int ffs(uint word)
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
#endif


#if NEED_FLS
/* 
    Find last bit set in word 
 */
#if USE_FFS_ASM_X86
static MPR_INLINE int fls(uint x)
{
    long r;

    asm("bsr %1,%0\n\t"
        "jnz 1f\n\t"
        "mov $-1,%0\n"
        "1:" : "=r" (r) : "rm" (x));
    return (int) r + 1;
}
#else /* USE_FLS_ASM_X86 */ 

static MPR_INLINE int fls(uint word)
{
    int     b;

    for (b = 0; word; word >>= 1, b++) ;
    return b;
}
#endif /* !USE_FLS_ASM_X86 */
#endif /* NEED_FLS */


#if NEED_FLSL
/* 
    Find last bit set in word 
 */
static MPR_INLINE int flsl(ulong word)
{
    int     b;

    for (b = 0; word; word >>= 1, b++) ;
    return b;
}
#endif /* NEED_FLSL */


/*
    Acquire the freeq. Note: this is only ever used by non-blocking algorithms.
 */
static inline bool acquire(MprFreeQueue *freeq)
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


//  MOB - remove
static inline void acquireWait(MprFreeQueue *freeq)
{
#if MACOSX
    OSSpinLockLock(&freeq->lock.cs);
#elif BIT_UNIX_LIKE && BIT_HAS_SPINLOCK
    pthread_spin_lock(&freeq->lock.cs);
#elif BIT_UNIX_LIKE
    pthread_mutex_lock(&freeq->lock.cs);
#elif BIT_WIN_LIKE
    EnterCriticalSection(&freeq->lock.cs);
#elif VXWORKS
    semTake(freeq->lock.cs, WAIT_FOREVER);
#endif
}


static inline void release(MprFreeQueue *freeq)
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


static inline int cas(size_t *target, size_t expected, size_t value)
{
    return mprAtomicCas((void**) target, (void*) expected, (cvoid*) value);
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


PUBLIC void mprSetMemLimits(ssize redLine, ssize maxMemory, ssize cacheMemory)
{
    if (redLine > 0) {
        heap->stats.redLine = redLine;
    }
    if (maxMemory > 0) {
        heap->stats.maxMemory = maxMemory;
    }
    if (cacheMemory >= 0) {
        heap->stats.cacheMemory = cacheMemory;
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
#if BIT_MEMORY_CHECK
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


#if BIT_MEMORY_DEBUG
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


#if BIT_MEMORY_STACK
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

#if !BIT_MEMORY_DEBUG
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
